
/*
Copyright (c) 2009-2017 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define HIDE_USE_EXCEPTION_INFO
#include <windows.h>
#include "Common.h"
#include "RgnDetect.h"
#include "UnicodeChars.h"

#ifndef RGN_ERROR
#define RGN_ERROR 0
#endif

WARNING("Заменить *mp_FarInfo на instance копию m_FarInfo");


static bool gbInTransparentAssert = false;

//#ifdef _DEBUG
//	#undef _ASSERTE
//	#define _ASSERTE(expr) gbInTransparentAssert=true; _ASSERT_EXPR((expr), _CRT_WIDE(#expr)); gbInTransparentAssert=false;
//	#undef _ASSERT
//	#define _ASSERT(expr) gbInTransparentAssert=true; _ASSERT_EXPR((expr), _CRT_WIDE(#expr)); gbInTransparentAssert=false;
//#endif


CRgnDetect::CRgnDetect()
{
	mn_DetectCallCount = 0;
	mn_AllFlagsSaved = 0;
	memset(&m_DetectedDialogs, 0, sizeof(m_DetectedDialogs));
	mp_FarInfo = NULL;
	// Флаги
	mn_NextDlgId = 0; mb_NeedPanelDetect = TRUE;
	memset(&mrc_LeftPanel,0,sizeof(mrc_LeftPanel));
	memset(&mrc_RightPanel,0,sizeof(mrc_RightPanel));
	memset(&mrc_FarRect, 0, sizeof(mrc_FarRect)); // по умолчанию - не используется
	//
	mpsz_Chars = NULL;
	mp_Attrs = mp_AttrsWork = NULL;
	mn_CurWidth = mn_CurHeight = mn_MaxCells = 0;
	mb_SBI_Loaded = false;
	mb_TableCreated = false;
	mb_NeedTransparency = false;
	//
	nUserBackIdx = nMenuBackIdx = 0;
	crUserBack = crMenuTitleBack = 0;
	nPanelTabsBackIdx = nPanelTabsForeIdx = 0; bPanelTabsSeparate = TRUE;
	bShowKeyBar = true; nBottomLines = 2;
	bAlwaysShowMenuBar = false; nTopLines = 0;
}

CRgnDetect::~CRgnDetect()
{
	if (mpsz_Chars)
	{
		free(mpsz_Chars); mpsz_Chars = NULL;
	}

	if (mp_Attrs)
	{
		free(mp_Attrs); mp_Attrs = NULL;
	}

	if (mp_AttrsWork)
	{
		free(mp_AttrsWork); mp_AttrsWork = NULL;
	}
}

//#ifdef _DEBUG
const DetectedDialogs* CRgnDetect::GetDetectedDialogsPtr() const
{
	return &m_DetectedDialogs;
}
//#endif

int CRgnDetect::GetDetectedDialogs(int anMaxCount, SMALL_RECT* rc, DWORD* rf, DWORD anMask/*=-1*/, DWORD anTest/*=-1*/) const
{
	if (!this) return 0;

	int nCount = m_DetectedDialogs.Count;

	if (nCount > 0 && anMaxCount > 0 && (rc || rf))
	{
		_ASSERTE(sizeof(*rc) == sizeof(m_DetectedDialogs.Rects[0]));
		_ASSERTE(sizeof(*rf) == sizeof(m_DetectedDialogs.DlgFlags[0]));

		if (anMask == (DWORD)-1)
		{
			if (nCount > anMaxCount)
				nCount = anMaxCount;

			if (rc)
				memmove(rc, m_DetectedDialogs.Rects, nCount*sizeof(*rc));

			if (rf)
				memmove(rf, m_DetectedDialogs.DlgFlags, nCount*sizeof(*rf));
		}
		else
		{
			nCount = 0; DWORD nF;

			for(int i = 0; i < m_DetectedDialogs.Count && nCount < anMaxCount; i++)
			{
				nF = m_DetectedDialogs.DlgFlags[i];

				if ((nF & anMask) == anTest)
				{
					if (rc)
						rc[nCount] = m_DetectedDialogs.Rects[i];

					if (rf)
						rf[nCount] = m_DetectedDialogs.DlgFlags[i];

					nCount++;
				}
			}
		}
	}

	return nCount;
}

DWORD CRgnDetect::GetDialog(DWORD nDlgID, SMALL_RECT* rc) const
{
	if (!nDlgID)
	{
		_ASSERTE(nDlgID!=0);
		return 0;
	}

	//DWORD nMask = nDlgID;
	//if ((nDlgID & FR_FREEDLG_MASK) != 0) {
	//	nMask |= FR_FREEDLG_MASK;
	//}

	for(int i = 0; i < m_DetectedDialogs.Count; i++)
	{
		if ((m_DetectedDialogs.DlgFlags[i] & FR_ALLDLG_MASK) == nDlgID)
		{
			if (rc) *rc = m_DetectedDialogs.Rects[i];

			_ASSERTE(m_DetectedDialogs.DlgFlags[i] != 0);
			return m_DetectedDialogs.DlgFlags[i];
		}
	}

	// Такого нету
	return 0;
}

DWORD CRgnDetect::GetFlags() const
{
	// чтобы не драться с другими нитями во время детекта возвращаем сохраненную mn_AllFlagsSaved
	// а не то что лежит в m_DetectedDialogs.AllFlags (оно может сейчас меняться)
	//return m_DetectedDialogs.AllFlags;
	return mn_AllFlagsSaved;
}







/* ****************************************** */
/* Поиск диалогов и пометка "прозрачных" мест */
/* ****************************************** */


// Найти правую границу
bool CRgnDetect::FindFrameRight_ByTop(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight)
{
	wchar_t wcMostRight = 0;
	int n;
	int nShift = nWidth*nFromY;
	wchar_t wc = pChar[nShift+nFromX];
	nMostRight = nFromX;

	if (wc != ucBoxSinglDownRight && wc != ucBoxDblDownRight)
	{
		// нетривиальная ситуация - возможно диалог на экране неполный
		int nMostTop = nFromY;

		if (FindFrameTop_ByLeft(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostTop))
			nFromY = nMostTop; // этот диалог продолжается сверху
	}

	if (wc != ucBoxSinglDownRight && wc != ucBoxDblDownRight)
	{
		wchar_t c;

		// Придется попробовать идти просто до границы
		if (wc == ucBoxSinglVert || wc == ucBoxSinglVertRight)
		{
			while(++nMostRight < nWidth)
			{
				c = pChar[nShift+nMostRight];

				if (c == ucBoxSinglVert || c == ucBoxSinglVertLeft)
				{
					nMostRight++; break;
				}
			}
		}
		else if (wc == ucBoxDblDownRight)
		{
			while(++nMostRight < nWidth)
			{
				c = pChar[nShift+nMostRight];

				if (c == ucBoxDblVert || c == ucBoxDblVertLeft || c == ucBoxDblVertSinglLeft)
				{
					nMostRight++; break;
				}
			}
		}
	}
	else
	{
		if (wc == ucBoxSinglDownRight)
		{
			wcMostRight = ucBoxSinglDownLeft;
		}
		else if (wc == ucBoxDblDownRight)
		{
			wcMostRight = ucBoxDblDownLeft;
		}

		// Найти правую границу
		while(++nMostRight < nWidth)
		{
			n = nShift+nMostRight;

			//if (pAttr[n].crBackColor != nBackColor)
			//	break; // конец цвета фона диалога
			if (pChar[n] == wcMostRight)
			{
				nMostRight++;
				break; // закрывающая угловая рамка
			}
		}
	}

	nMostRight--;
	_ASSERTE(nMostRight<nWidth);
	return (nMostRight > nFromX);
}

// Найти левую границу
bool CRgnDetect::FindFrameLeft_ByTop(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostLeft)
{
	wchar_t wcMostLeft;
	int n;
	int nShift = nWidth*nFromY;
	wchar_t wc = pChar[nShift+nFromX];
	nMostLeft = nFromX;

	if (wc != ucBoxSinglDownLeft && wc != ucBoxDblDownLeft)
	{
		// нетривиальная ситуация - возможно диалог на экране неполный
		int nMostTop = nFromY;

		if (FindFrameTop_ByRight(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostTop))
			nFromY = nMostTop; // этот диалог продолжается сверху
	}

	if (wc != ucBoxSinglDownLeft && wc != ucBoxDblDownLeft)
	{
		wchar_t c;

		// Придется попробовать идти просто до границы
		if (wc == ucBoxSinglVert || wc == ucBoxSinglVertLeft)
		{
			while(--nMostLeft >= 0)
			{
				c = pChar[nShift+nMostLeft];

				if (c == ucBoxSinglVert || c == ucBoxSinglVertRight)
				{
					nMostLeft--; break;
				}
			}
		}
		else if (wc == ucBoxDblDownRight)
		{
			while(--nMostLeft >= 0)
			{
				c = pChar[nShift+nMostLeft];

				if (c == ucBoxDblVert || c == ucBoxDblVertLeft || c == ucBoxDblVertSinglLeft)
				{
					nMostLeft--; break;
				}
			}
		}
	}
	else
	{
		if (wc == ucBoxSinglDownLeft)
		{
			wcMostLeft = ucBoxSinglDownRight;
		}
		else if (wc == ucBoxDblDownLeft)
		{
			wcMostLeft = ucBoxDblDownRight;
		}
		else
		{
			_ASSERTE(wc == ucBoxSinglDownLeft || wc == ucBoxDblDownLeft);
			return false;
		}

		// Найти левую границу
		while(--nMostLeft >= 0)
		{
			n = nShift+nMostLeft;

			//if (pAttr[n].crBackColor != nBackColor)
			//	break; // конец цвета фона диалога
			if (pChar[n] == wcMostLeft)
			{
				nMostLeft--;
				break; // закрывающая угловая рамка
			}
		}
	}

	nMostLeft++;
	_ASSERTE(nMostLeft>=0);
	return (nMostLeft < nFromX);
}

bool CRgnDetect::FindFrameRight_ByBottom(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight)
{
	wchar_t wcMostRight;
	int n;
	int nShift = nWidth*nFromY;
	wchar_t wc = pChar[nShift+nFromX];
	nMostRight = nFromX;

	if (wc == ucBoxSinglUpRight || wc == ucBoxSinglHorz || wc == ucBoxSinglUpHorz || wc == ucBoxDblUpSinglHorz)
	{
		wcMostRight = ucBoxSinglUpLeft;
	}
	else if (wc == ucBoxDblUpRight || wc == ucBoxSinglUpDblHorz || wc == ucBoxDblUpDblHorz || wc == ucBoxDblHorz)
	{
		wcMostRight = ucBoxDblUpLeft;
	}
	else
	{
		return false; // найти правый нижний угол мы не сможем
	}

	// Найти правую границу
	while(++nMostRight < nWidth)
	{
		n = nShift+nMostRight;

		//if (pAttr[n].crBackColor != nBackColor)
		//	break; // конец цвета фона диалога
		if (pChar[n] == wcMostRight)
		{
			nMostRight++;
			break; // закрывающая угловая рамка
		}
	}

	nMostRight--;
	_ASSERTE(nMostRight<nWidth);
	return (nMostRight > nFromX);
}

bool CRgnDetect::FindFrameLeft_ByBottom(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostLeft)
{
	wchar_t wcMostLeft;
	int n;
	int nShift = nWidth*nFromY;
	wchar_t wc = pChar[nShift+nFromX];
	nMostLeft = nFromX;

	if (wc == ucBoxSinglUpLeft || wc == ucBoxSinglHorz || wc == ucBoxSinglUpHorz || wc == ucBoxDblUpSinglHorz)
	{
		wcMostLeft = ucBoxSinglUpRight;
	}
	else if (wc == ucBoxDblUpLeft || wc == ucBoxSinglUpDblHorz || wc == ucBoxDblUpDblHorz || wc == ucBoxDblHorz)
	{
		wcMostLeft = ucBoxDblUpRight;
	}
	else
	{
		return false; // найти левый нижний угол мы не сможем
	}

	// Найти левую границу
	while(--nMostLeft >= 0)
	{
		n = nShift+nMostLeft;

		//if (pAttr[n].crBackColor != nBackColor)
		//	break; // конец цвета фона диалога
		if (pChar[n] == wcMostLeft)
		{
			nMostLeft--;
			break; // закрывающая угловая рамка
		}
	}

	nMostLeft++;
	_ASSERTE(nMostLeft>=0);
	return (nMostLeft < nFromX);
}

// Диалог без окантовки. Все просто - идем по рамке
bool CRgnDetect::FindDialog_TopLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder)
{
	bMarkBorder = TRUE;
	#ifdef _DEBUG
	int nShift = nWidth*nFromY;
	#endif
	int nMostRightBottom;
	// Найти правую границу по верху
	nMostRight = nFromX;
	FindFrameRight_ByTop(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight);
	_ASSERTE(nMostRight<nWidth);
	// Найти нижнюю границу
	nMostBottom = nFromY;
	FindFrameBottom_ByLeft(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostBottom);
	_ASSERTE(nMostBottom<nHeight);
	// Найти нижнюю границу по правой стороне
	nMostRightBottom = nFromY;

	if (FindFrameBottom_ByRight(pChar, pAttr, nWidth, nHeight, nMostRight, nFromY, nMostRightBottom))
	{
		_ASSERTE(nMostRightBottom<nHeight);

		// Результатом считаем - наиБОЛЬШУЮ высоту
		if (nMostRightBottom > nMostBottom)
			nMostBottom = nMostRightBottom;
	}

	return true;
}

// Диалог без окантовки. Все просто - идем по рамке
bool CRgnDetect::FindDialog_TopRight(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder)
{
	bMarkBorder = TRUE;
	#ifdef _DEBUG
	int nShift = nWidth*nFromY;
	#endif
	int nX;
	nMostRight = nFromX;
	nMostBottom = nFromY;

	// Найти левую границу по верху
	if (FindFrameLeft_ByTop(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nX))
	{
		_ASSERTE(nX>=0);
		nFromX = nX;
	}

	// Найти нижнюю границу
	nMostBottom = nFromY;
	FindFrameBottom_ByRight(pChar, pAttr, nWidth, nHeight, nMostRight, nFromY, nMostBottom);
	_ASSERTE(nMostBottom<nHeight);

	// найти левую границу по низу
	if (FindFrameLeft_ByBottom(pChar, pAttr, nWidth, nHeight, nMostRight, nMostBottom, nX))
	{
		_ASSERTE(nX>=0);

		if (nFromX > nX) nFromX = nX;
	}

	return true;
}

// Это может быть нижний кусок диалога
// левая граница рамки
// Диалог без окантовки, но начинается не с угла
bool CRgnDetect::FindDialog_Left(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder)
{
	bMarkBorder = TRUE;
	//nBackColor = pAttr[nFromX+nWidth*nFromY].crBackColor; // но на всякий случай сохраним цвет фона для сравнения
	wchar_t wcMostRight, wcMostBottom, wcMostRightBottom, wcMostTop, wcNotMostBottom1;
	int nShift = nWidth*nFromY;
	int nMostTop, nY, nX;
	wchar_t wc = pChar[nShift+nFromX];

	if (wc == ucBoxSinglVert || wc == ucBoxSinglVertRight)
	{
		wcMostRight = ucBoxSinglUpLeft; wcMostBottom = ucBoxSinglUpRight; wcMostRightBottom = ucBoxSinglUpLeft; wcMostTop = ucBoxSinglDownLeft;

		// наткнулись на вертикальную линию на панели
		if (wc == ucBoxSinglVert)
		{
			wcNotMostBottom1 = ucBoxSinglUpHorz; //wcNotMostBottom2 = ucBoxSinglUpDblHorz;
			nMostBottom = nFromY;

			while(++nMostBottom < nHeight)
			{
				wc = pChar[nFromX+nMostBottom*nWidth];

				if (wc == wcNotMostBottom1 || wc == ucBoxDblUpSinglHorz || wc == ucBoxSinglUpDblHorz || wc == ucBoxDblUpDblHorz)
					return false;
			}
		}
	}
	else
	{
		wcMostRight = ucBoxDblUpLeft; wcMostBottom = ucBoxDblUpRight; wcMostRightBottom = ucBoxDblUpLeft; wcMostTop = ucBoxDblDownLeft;
	}

	// попытаться подняться вверх, может угол все-таки есть?
	if (FindFrameTop_ByLeft(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nY))
	{
		_ASSERTE(nY >= 0);
		nFromY = nY;
	}

	// Найти нижнюю границу
	nMostBottom = nFromY;
	FindFrameBottom_ByLeft(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostBottom);
	_ASSERTE(nMostBottom<nHeight);
	// Найти правую границу по верху
	nMostRight = nFromX;

	if (FindFrameRight_ByTop(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nX))
	{
		_ASSERTE(nX<nWidth);
		nMostRight = nX;
	}

	// попробовать найти правую границу по низу
	if (FindFrameRight_ByBottom(pChar, pAttr, nWidth, nHeight, nFromX, nMostBottom, nX))
	{
		_ASSERTE(nX<nWidth);

		if (nX > nMostRight) nMostRight = nX;
	}

	_ASSERTE(nMostRight>=0);

	// Попытаться подняться вверх по правой границе?
	if (FindFrameTop_ByRight(pChar, pAttr, nWidth, nHeight, nMostRight, nFromY, nMostTop))
	{
		_ASSERTE(nMostTop>=0);
		nFromY = nMostTop;
	}

	return true;
}

bool CRgnDetect::FindFrameTop_ByLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostTop)
{
	wchar_t c;
	// Попытаемся подняться вверх вдоль правой рамки до угла
	int nY = nFromY;

	while(nY > 0)
	{
		c = pChar[(nY-1)*nWidth+nFromX];

		if (c == ucBoxDblDownRight || c == ucBoxSinglDownRight  // двойной и одинарный угол (левый верхний)
		        || c == ucBoxDblVertRight || c == ucBoxDblVertSinglRight || c == ucBoxSinglVertRight
		        || c == ucBoxDblVert || c == ucBoxSinglVert
		  ) // пересечение (правая граница)
		{
			nY--; continue;
		}

		// Другие символы недопустимы
		break;
	}

	_ASSERTE(nY >= 0);
	nMostTop = nY;
	return (nMostTop < nFromY);
}

bool CRgnDetect::FindFrameTop_ByRight(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostTop)
{
	wchar_t c;
	// Попытаемся подняться вверх вдоль правой рамки до угла
	int nY = nFromY;

	while(nY > 0)
	{
		c = pChar[(nY-1)*nWidth+nFromX];

		if (c == ucBoxDblDownLeft || c == ucBoxSinglDownLeft  // двойной и одинарный угол (правый верхний)
		        || c == ucBoxDblVertLeft || c == ucBoxDblVertSinglLeft || c == ucBoxSinglVertLeft // пересечение (правая граница)
		        || c == ucBoxDblVert || c == ucBoxSinglVert
		        || c == L'}' // На правой границе панели могут быть признаки обрезанности имени файла
		        || (c >= ucBox25 && c <= ucBox75) || c == ucUpScroll || c == ucDnScroll) // полоса прокрутки может быть только справа
		{
			nY--; continue;
		}

		// Другие символы недопустимы
		break;
	}

	_ASSERTE(nY >= 0);
	nMostTop = nY;
	return (nMostTop < nFromY);
}

bool CRgnDetect::FindFrameBottom_ByRight(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostBottom)
{
	// Попытаемся спуститься вдоль правой рамки до угла
	int nY = nFromY;
	int nEnd = nHeight - 1;
	wchar_t c; //, cd = ucBoxDblVert;

	//// В случае с правой панелью в правом верхнем углу могут быть часы, а не собственно угол
	//// "<=1" т.к. надо учесть возможность постоянного меню
	//c = pChar[nY*nWidth+nFromX];
	//if (nFromY <= 1 && nFromX == (nWidth-1)) {
	//	if (isDigit(c)) {
	//		cd = c;
	//	}
	//}
	while(nY < nEnd)
	{
		c = pChar[(nY+1)*nWidth+nFromX];

		if (c == ucBoxDblUpLeft || c == ucBoxSinglUpLeft  // двойной и одинарный угол (правый нижний)
		        //|| c == cd // учесть часы на правой панели
		        || c == ucBoxDblVertLeft || c == ucBoxDblVertSinglLeft || c == ucBoxSinglVertLeft // пересечение (правая граница)
		        || c == ucBoxDblVert || c == ucBoxSinglVert
		        || (c >= ucBox25 && c <= ucBox75) || c == ucUpScroll || c == ucDnScroll) // полоса прокрутки может быть только справа
		{
			nY++; continue;
		}

		// Другие символы недопустимы
		break;
	}

	_ASSERTE(nY < nHeight);
	nMostBottom = nY;
	return (nMostBottom > nFromY);
}

bool CRgnDetect::FindFrameBottom_ByLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostBottom)
{
	// Попытаемся спуститься вдоль левой рамки до угла
	int nY = nFromY;
	int nEnd = nHeight - 1;
	wchar_t c;

	while(nY < nEnd)
	{
		c = pChar[(nY+1)*nWidth+nFromX];

		if (c == ucBoxDblUpRight || c == ucBoxSinglUpRight  // двойной и одинарный угол (левый нижний)
		        || c == ucBoxDblVertRight || c == ucBoxDblVertSinglRight || c == ucBoxSinglVertRight
		        || c == ucBoxDblVert || c == ucBoxSinglVert
		  ) // пересечение (правая граница)
		{
			nY++; continue;
		}

		// Другие символы недопустимы
		break;
	}

	_ASSERTE(nY < nHeight);
	nMostBottom = nY;
	return (nMostBottom > nFromY);
}

// Мы на правой границе рамки. Найти диалог слева
bool CRgnDetect::FindDialog_Right(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder)
{
	bMarkBorder = TRUE;
	int nY = nFromY;
	int nX = nFromX;
	nMostRight = nFromX;
	nMostBottom = nFromY; // сразу запомним

	// Попытаемся подняться вверх вдоль правой рамки до угла
	if (FindFrameTop_ByRight(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nY))
		nFromY = nY;

	// Попытаемся спуститься вдоль правой рамки до угла
	if (FindFrameBottom_ByRight(pChar, pAttr, nWidth, nHeight, nFromX, nMostBottom, nY))
		nMostBottom = nY;

	// Теперь можно искать диалог

	// по верху
	if (FindFrameLeft_ByTop(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nX))
		nFromX = nX;

	// по низу
	if (FindFrameLeft_ByBottom(pChar, pAttr, nWidth, nHeight, nFromX, nMostBottom, nX))
		if (nX < nFromX) nFromX = nX;

	_ASSERTE(nFromX>=0 && nFromY>=0);
	return true;
}

// Диалог может быть как слева, так и справа от вертикальной линии
bool CRgnDetect::FindDialog_Any(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder)
{
	wchar_t c;
	wchar_t wc = pChar[nFromY*nWidth+nFromX];

	// Так что сначала нужно подняться (или опуститься) по рамке до угла
	for(int ii = 0; ii <= 1; ii++)
	{
		int nY = nFromY;
		int nYEnd = (!ii) ? -1 : nHeight;
		int nYStep = (!ii) ? -1 : 1;
		wchar_t wcCorn1 = (!ii) ? ucBoxSinglDownLeft : ucBoxSinglUpLeft;
		wchar_t wcCorn2 = (!ii) ? ucBoxDblDownLeft : ucBoxDblUpLeft;
		wchar_t wcCorn3 = (!ii) ? ucBoxDblDownRight : ucBoxDblUpRight;
		wchar_t wcCorn4 = (!ii) ? ucBoxSinglDownRight : ucBoxSinglUpRight;

		// TODO: если можно - определим какой угол мы ищем (двойной/одинарный)

		// поехали
		while(nY != nYEnd)
		{
			c = pChar[nY*nWidth+nFromX];

			if (c == wcCorn1 || c == wcCorn2  // двойной и одинарный угол (правый верхний/нижний)
			        || c == ucBoxDblVertLeft || c == ucBoxDblVertSinglLeft || c == ucBoxSinglVertLeft // пересечение (правая граница)
			        || c == L'}' // На правой границе панели могут быть признаки обрезанности имени файла
			        || (c >= ucBox25 && c <= ucBox75) || c == ucUpScroll || c == ucDnScroll) // полоса прокрутки может быть только справа
			{
				if (FindDialog_Right(pChar, pAttr, nWidth, nHeight, nFromX, nY, nMostRight, nMostBottom, bMarkBorder))
				{
					nFromY = nY;
					return true;
				}

				return false; // непонятно что...
			}

			if (c == wcCorn3 || c == wcCorn4  // двойной и одинарный угол (левый верхний/нижний)
			        || c == ucBoxDblVertRight || c == ucBoxDblVertSinglRight || c == ucBoxSinglVertRight) // пересечение (правая граница)
			{
				if (FindDialog_Left(pChar, pAttr, nWidth, nHeight, nFromX, nY, nMostRight, nMostBottom, bMarkBorder))
				{
					nFromY = nY;
					return true;
				}

				return false; // непонятно что...
			}

			if (c != wc)
			{
				// Другие символы недопустимы
				break;
			}

			// Сдвигаемся (вверх или вниз)
			nY += nYStep;
		}
	}

	return false;
}

// Function marks Far Panel vertical single lines (column separators)
bool CRgnDetect::FindDialog_Inner(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY)
{
	// наткнулись на вертикальную линию на панели
	int nShift = nWidth*nFromY;
	wchar_t wc = pChar[nShift+nFromX];

	if (wc != ucBoxSinglVert)
	{
		_ASSERTE(wc == ucBoxSinglVert);
		return false;
	}

	// Поверх панели может быть диалог... хорошо бы это учитывать
	int nY = nFromY;

	while(++nY < nHeight)
	{
		wc = pChar[nFromX+nY*nWidth];

		switch(wc)
		{
				// на панелях вертикальная линия может прерываться '}' (когда имя файла в колонку не влезает)
			case ucBoxSinglVert:
			case L'}':
				continue;
				// Если мы натыкаемся на угловой рамочный символ - значит это часть диалога. выходим
			case ucBoxSinglUpRight:
			case ucBoxSinglUpLeft:
			case ucBoxSinglVertRight:
			case ucBoxSinglVertLeft:
				return false;
				// достигли низа панели
			case ucBoxSinglUpHorz:
			case ucBoxDblUpSinglHorz:
			case ucBoxSinglUpDblHorz:
			case ucBoxDblUpDblHorz:
				nY++; // пометить все сверху (включая)
				// иначе - прервать поиск и пометить все сверху (не включая)
			default:
				nY--;
				{
					// Пометить всю линию до верха (содержащую допустимые символы) как часть рамки
					CharAttr* p = pAttr+(nWidth*nY+nFromX);

					while(nY-- >= nFromY)
					{
						//_ASSERTE(p->bDialog);
						_ASSERTE(p >= pAttr);
						p->Flags |= CharAttr_DialogVBorder;
						p -= nWidth;
					}

					// мы могли начать не с верха панели
					while(nY >= 0)
					{
						wc = pChar[nFromX+nY*nWidth];

						if (wc != ucBoxSinglVert && wc != ucBoxSinglDownHorz 
							&& wc != ucBoxSinglDownDblHorz && wc != ucBoxDblDownDblHorz)
						{
							break;
						}

						//_ASSERTE(p->bDialog);
						_ASSERTE(p >= pAttr);
						p->Flags |= CharAttr_DialogVBorder;
						p -= nWidth;
						nY --;
					}
				}
				return true;
		}
	}

	return false;
}

// Попытаемся найти рамку?
bool CRgnDetect::FindFrame_TopLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nFrameX, int &nFrameY)
{
	// Попытаемся найти рамку?
	nFrameX = -1; nFrameY = -1;
	int nShift = nWidth*nFromY;
	int nFindFrom = nShift+nFromX;
	int nMaxAdd = min(5,(nWidth - nFromX - 1));
	wchar_t wc;

	// в этой же строке
	for(int n = 1; n <= nMaxAdd; n++)
	{
		wc = pChar[nFindFrom+n];

		if (wc == ucBoxSinglDownRight || wc == ucBoxDblDownRight)
		{
			nFrameX = nFromX+n; nFrameY = nFromY;
			return true;
		}
	}

	if (nFrameY == -1)
	{
		// строкой ниже
		nFindFrom = nShift+nWidth+nFromX;

		for(int n = 0; n <= nMaxAdd; n++)
		{
			wc = pChar[nFindFrom+n];

			if (wc == ucBoxSinglDownRight || wc == ucBoxDblDownRight)
			{
				nFrameX = nFromX+n; nFrameY = nFromY+1;
				return true;
			}
		}
	}

	return false;
}


bool CRgnDetect::ExpandDialogFrame(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int nFrameX, int nFrameY, int &nMostRight, int &nMostBottom)
{
	bool bExpanded = false;
	#ifdef _DEBUG
	int nStartRight = nMostRight;
	int nStartBottom = nMostBottom;
	#endif
	// Теперь расширить nMostRight & nMostBottom на окантовку
	int n, n2, nShift = nWidth*nFromY;
	wchar_t wc = pChar[nShift+nFromX];
	DWORD nColor = pAttr[nShift+nFromX].crBackColor;

	if (nFromX == nFrameX && nFromY == nFrameY)
	{
		if (wc != ucBoxDblDownRight && wc != ucBoxSinglDownRight)
			return false;

		//Сначала нужно пройти вверх и влево
		if (nFromY)    // Пробуем вверх
		{
			n = (nFromY-1)*nWidth+nFromX;
			n2 = (nFromY-1)*nWidth+nMostRight;

			if ((pAttr[n].crBackColor == nColor && (pChar[n] == L' ' || pChar[n] == ucNoBreakSpace))
			        && (pAttr[n2].crBackColor == nColor && (pChar[n2] == L' ' || pChar[n2] == ucNoBreakSpace)))
			{
				nFromY--; bExpanded = true;
			}
		}

		if (nFromX)    // Пробуем влево
		{

			int nMinMargin = nFromX-3; if (nMinMargin<0) nMinMargin = 0;

			n = nFromY*nWidth+nFromX;
			n2 = nMostBottom*nWidth+nFromX;

			while (nFromX > nMinMargin)
			{
				n--; n2--;

				// Проверяем цвет и символ
				if ((((pAttr[n].crBackColor == nColor) && ((pChar[n] == L' ') || (pChar[n] == ucNoBreakSpace)))
						// "\\" - пометка что диалог в Far двигается с клавиатуры (Ctrl+F5)
						|| (pChar[n] == L'\\'))
					// Строка n2 соответствует не самой нижней строке диалога,
					// а предпоследней строке на которой рисуется рамка.
					// Поэтому тут проверяем только на " "
					&& ((pAttr[n2].crBackColor == nColor) && ((pChar[n2] == L' ') || (pChar[n2] == ucNoBreakSpace))))
				{
					nFromX--;
				}
				else
				{
					break;
				}
			}

			bExpanded = (nFromX<nFrameX);
		}

		_ASSERTE(nFromX>=0 && nFromY>=0);
	}
	else
	{
		if (wc != ucSpace && wc != ucNoBreakSpace)
			return false;
	}

	if (nMostRight < (nWidth-1))
	{
		int nMaxMargin = 3+(nFrameX - nFromX);

		if ((nMaxMargin+nMostRight) >= nWidth) nMaxMargin = nWidth - nMostRight - 1;

		int nFindFrom = nShift+nWidth+nMostRight+1;
		n = 0;
		wc = pChar[nShift+nFromX];

		while(n < nMaxMargin)
		{
			if (pAttr[nFindFrom].crBackColor != nColor || (pChar[nFindFrom] != L' ' && pChar[nFindFrom] != ucNoBreakSpace))
				break;

			n++; nFindFrom++;
		}

		if (n)
		{
			nMostRight += n;
			bExpanded = true;
		}
	}

	_ASSERTE(nMostRight<nWidth);

	// nMostBottom
	if (nFrameY > nFromY && nMostBottom < (nHeight-1))
	{
		// Если в фаре диалог двигается с клавиатуры (Ctrl+F5)
		// то по углам диалога ставятся красные метки "\/"
		// Поэтому проверяем не по углу диалога, а "под углом рамки"
		n = (nMostBottom+1)*nWidth+nFrameX;

		if (pAttr[n].crBackColor == nColor && (pChar[n] == L' ' || pChar[n] == ucNoBreakSpace))
		{
			nMostBottom ++; bExpanded = true;
		}
	}

	_ASSERTE(nMostBottom<nHeight);
	return bExpanded;
}

// В идеале - сюда попадать не должны. Это может быть или кусок диалога
// другая часть которого или скрыта, или вытащена за границы экрана
bool CRgnDetect::FindByBackground(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder)
{
	// Придется идти просто по цвету фона
	// Это может быть диалог, рамка которого закрыта другим диалогом,
	// или вообще кусок диалога, у которого видна только часть рамки
	DWORD nBackColor = pAttr[nFromX+nWidth*nFromY].crBackColor;
	int n, nMostRightBottom, nShift = nWidth*nFromY;
	// Найти правую границу
	nMostRight = nFromX;

	while(++nMostRight < nWidth)
	{
		n = nShift+nMostRight;

		if (pAttr[n].crBackColor != nBackColor)
			break; // конец цвета фона диалога
	}

	nMostRight--;
	_ASSERTE(nMostRight<nWidth);
	//2010-06-27 все-таки лишнее. Если уж пошли "по фону" то не нужно на рамки ориентироваться, иначе зациклимся
#if 0
	wchar_t wc = pChar[nFromY*nWidth+nMostRight];

	if (wc >= ucBoxSinglHorz && wc <= ucBoxDblVertHorz)
	{
		switch(wc)
		{
			case ucBoxSinglDownRight: case ucBoxDblDownRight:
			case ucBoxSinglUpRight: case ucBoxDblUpRight:
			case ucBoxSinglDownLeft: case ucBoxDblDownLeft:
			case ucBoxSinglUpLeft: case ucBoxDblUpLeft:
			case ucBoxDblVert: case ucBoxSinglVert:
			{
				DetectDialog(pChar, pAttr, nWidth, nHeight, nMostRight, nFromY);

				if (pAttr[nShift+nFromX].bDialog)
					return false; // все уже обработано
			}
		}
	}
	else if (nMostRight && ((wc >= ucBox25 && wc <= ucBox75) || wc == ucUpScroll || wc == ucDnScroll))
	{
		int nX = nMostRight;

		if (FindDialog_Right(pChar, pAttr, nWidth, nHeight, nX, nFromY, nMostRight, nMostBottom, bMarkBorder))
		{
			nFromX = nX;
			return false;
		}
	}

#endif
	// Найти нижнюю границу
	nMostBottom = nFromY;

	while(++nMostBottom < nHeight)
	{
		n = nFromX+nMostBottom*nWidth;

		if (pAttr[n].crBackColor != nBackColor)
			break; // конец цвета фона диалога
	}

	nMostBottom--;
	_ASSERTE(nMostBottom<nHeight);
	// Найти нижнюю границу по правой стороне
	nMostRightBottom = nFromY;

	while(++nMostRightBottom < nHeight)
	{
		n = nMostRight+nMostRightBottom*nWidth;

		if (pAttr[n].crBackColor != nBackColor)
			break; // конец цвета фона диалога
	}

	nMostRightBottom--;
	_ASSERTE(nMostRightBottom<nHeight);

	// Результатом считаем - наибольшую высоту
	if (nMostRightBottom > nMostBottom)
		nMostBottom = nMostRightBottom;

	return true;
}

bool CRgnDetect::DetectDialog(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int nFromX, int nFromY, int *pnMostRight, int *pnMostBottom)
{
	if (nFromX >= nWidth || nFromY >= nHeight)
	{
		_ASSERTE(nFromX<nWidth);
		_ASSERTE(nFromY<nHeight);
		return false;
	}

#ifdef _DEBUG

	if (nFromX == 69 && nFromY == 12)
	{
		nFromX = nFromX;
	}

#endif

	// защита от переполнения стека (быть не должно)
	if (mn_DetectCallCount >= 3)
	{
		_ASSERTE(mn_DetectCallCount<3);
		return false;
	}

	/* *********************************************** */
	/* После этой строки 'return' использовать нельзя! */
	/* *********************************************** */
	bool lbDlgFound = false;
	mn_DetectCallCount++;
	wchar_t wc; //, wcMostRight, wcMostBottom, wcMostRightBottom, wcMostTop, wcNotMostBottom1, wcNotMostBottom2;
	int nMostRight, nMostBottom; //, nMostRightBottom, nMostTop, nShift, n;
	//DWORD nBackColor;
	bool bMarkBorder = false;
	// Самое противное - детект диалога, который частично перекрыт другим диалогом
	int nShift = nWidth*nFromY;
	wc = pChar[nShift+nFromX];
	WARNING("Доделать detect");
	/*
	Если нижний-левый угол диалога не нашли - он может быть закрыт другим диалогом?
	попытаться найти правый-нижний угол?
	*/

	/*
	Если у панелей закрыта середина нижней линии (меню плагинов) то ошибочно считается,
	что это один большой диалог. А ведь можно по верху определить.
	*/

	if (wc >= ucBoxSinglHorz && wc <= ucBoxDblVertHorz)
	{
		switch(wc)
		{
				// Верхний левый угол?
			case ucBoxSinglDownRight: case ucBoxDblDownRight:
			{
				// Диалог без окантовки. Все просто - идем по рамке
				if (!FindDialog_TopLeft(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight, nMostBottom, bMarkBorder))
					goto fin;

				goto done;
			}
			// Нижний левый угол?
			case ucBoxSinglUpRight: case ucBoxDblUpRight:
			{
				// Сначала нужно будет подняться по рамке вверх
				if (!FindDialog_TopLeft(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight, nMostBottom, bMarkBorder))
					goto fin;

				goto done;
			}
			// Верхний правый угол?
			case ucBoxSinglDownLeft: case ucBoxDblDownLeft:
			{
				// Диалог без окантовки. Все просто - идем по рамке
				if (!FindDialog_TopRight(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight, nMostBottom, bMarkBorder))
					goto fin;

				goto done;
			}
			// Нижний правый угол?
			case ucBoxSinglUpLeft: case ucBoxDblUpLeft:
			{
				// Сначала нужно будет подняться по рамке вверх
				if (!FindDialog_Right(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight, nMostBottom, bMarkBorder))
					goto fin;

				goto done;
			}
			case ucBoxDblVert: case ucBoxSinglVert:
			{
				// наткнулись на вертикальную линию на панели
				if (wc == ucBoxSinglVert)
				{
					if (FindDialog_Inner(pChar, pAttr, nWidth, nHeight, nFromX, nFromY))
						goto fin;
				}

				// Диалог может быть как слева, так и справа от вертикальной линии
				if (FindDialog_Any(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight, nMostBottom, bMarkBorder))
					goto done;
			}
		}
	}

	if (wc == ucSpace || wc == ucNoBreakSpace)
	{
		// Попытаемся найти рамку?
		int nFrameX = -1, nFrameY = -1;

		if (FindFrame_TopLeft(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nFrameX, nFrameY))
		{
			// Если угол нашли - ищем рамку по рамке :)
			DetectDialog(pChar, pAttr, nWidth, nHeight, nFrameX, nFrameY, &nMostRight, &nMostBottom);
			//// Теперь расширить nMostRight & nMostBottom на окантовку
			//ExpandDialogFrame(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nFrameX, nFrameY, nMostRight, nMostBottom);
			//
			goto done;
		}
	}

	// Придется идти просто по цвету фона
	// Это может быть диалог, рамка которого закрыта другим диалогом,
	// или вообще кусок диалога, у которого видна только часть рамки
	// 100627 - но только если это не вьювер/редактор. там слишком много мусора будет
	if ((m_DetectedDialogs.AllFlags & FR_VIEWEREDITOR))
		goto fin;

	if (!FindByBackground(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight, nMostBottom, bMarkBorder))
		goto fin; // значит уже все пометили, или диалога нет

done:
#ifdef _DEBUG

	if (nFromX<0 || nFromX>=nWidth || nMostRight<nFromX || nMostRight>=nWidth
	        || nFromY<0 || nFromY>=nHeight || nMostBottom<nFromY || nMostBottom>=nHeight)
	{
		//_ASSERT(FALSE);
		// Это происходит, если обновление внутренних буферов произошло ДО
		// завершения обработки диалогов (быстрый драг панелей, диалогов, и т.п.)
		goto fin;
	}

#endif
	// Забить атрибуты
	MarkDialog(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight, nMostBottom, (bMarkBorder ? CharAttr_DialogRect : 0));
	lbDlgFound = true;

	// Вернуть размеры, если просили
	if (pnMostRight) *pnMostRight = nMostRight;

	if (pnMostBottom) *pnMostBottom = nMostBottom;

fin:
	mn_DetectCallCount--;
	_ASSERTE(mn_DetectCallCount>=0);
	return lbDlgFound;
}

int CRgnDetect::MarkDialog(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int nX1, int nY1, int nX2, int nY2, UINT bMarkBorder /*= 0*/, bool bFindExterior /*= TRUE*/, DWORD nFlags /*= -1*/)
{
	if (nX1<0 || nX1>=nWidth || nX2<nX1 || nX2>=nWidth
	        || nY1<0 || nY1>=nHeight || nY2<nY1 || nY2>=nHeight)
	{
		_ASSERTE(nX1>=0 && nX1<nWidth);  _ASSERTE(nX2>=0 && nX2<nWidth);
		_ASSERTE(nY1>=0 && nY1<nHeight); _ASSERTE(nY2>=0 && nY2<nHeight);
		return -1;
	}

	int nDlgIdx = -1;
	DWORD DlgFlags = bMarkBorder ? FR_HASBORDER : 0;
	int nWidth_1 = nWidth - 1;
	int nHeight_1 = nHeight - 1;
	//RECT r = {nX1,nY1,nX2,nY2};

	if (nFlags != (DWORD)-1)
	{
		DlgFlags |= nFlags;
	}
	else if (!nX1 && !nY1 && !nY2 && nX2 == nWidth_1)
	{
		if ((mp_FarInfo->FarInterfaceSettings.AlwaysShowMenuBar) == 0)
		{
			DlgFlags |= FR_ACTIVEMENUBAR;
		}
		else
		{
			DlgFlags |= FR_MENUBAR;
			// Попытаться подхватить флаг FR_ACTIVEMENUBAR даже когда меню всегда видно
			BYTE btMenuInactiveFore = CONFORECOLOR(mp_FarInfo->nFarColors[col_HMenuText]);
			BYTE btMenuInactiveBack = CONBACKCOLOR(mp_FarInfo->nFarColors[col_HMenuText]);
			int nShift = nY1 * nWidth + nX1;

			for(int nX = nX1; nX <= nX2; nX++, nShift++)
			{
				if (pAttr[nShift].nForeIdx != btMenuInactiveFore
				        || pAttr[nShift].nBackIdx != btMenuInactiveBack)
				{
					DlgFlags |= FR_ACTIVEMENUBAR;
					break;
				}
			}
		}
	}
	else if (bMarkBorder && bFindExterior && mb_NeedPanelDetect) // должно быть так при пометке панелей
		//&& !(m_DetectedDialogs.AllFlags & FR_FULLPANEL) // если еще не нашли полноэкранную панель
		//&& ((m_DetectedDialogs.AllFlags & (FR_LEFTPANEL|FR_RIGHTPANEL)) != (FR_LEFTPANEL|FR_RIGHTPANEL)) // и не нашли или правую или левую панель
		//)
	{
		if ((!nY1 || ((m_DetectedDialogs.AllFlags & FR_MENUBAR) && (nY1 == 1)))  // условие для верхней границы панелей
		        && (nY2 >= (nY1 + 3))) // и минимальная высота панелей
		{
			// Console API uses SHORT
			_ASSERTE(!HIWORD(nX1) && !HIWORD(nY1) && !HIWORD(nX2) && !HIWORD(nY2));
			SMALL_RECT sr = {(SHORT)nX1, (SHORT)nY1, (SHORT)nX2, (SHORT)nY2};
			DWORD nPossible = 0;

			if (!nX1)
			{
				if (nX2 == nWidth_1)
				{
					nPossible |= FR_FULLPANEL;
					//r.Left = nX1; r.Top = nY1; r.Right = nX2; r.Bottom = nY2;
				}
				else if (nX2 < (nWidth-9))
				{
					nPossible |= FR_LEFTPANEL;
					//r.Left = nX1; r.Top = nY1; r.Right = nX2; r.Bottom = nY2;
				}
			}
			else if (nX1 >= 10 && nX2 == nWidth_1)
			{
				nPossible |= FR_RIGHTPANEL;
				//r.Left = nX1; r.Top = nY1; r.Right = nX2; r.Bottom = nY2;
			}

			if (nPossible)
			{
				// Нужно чтобы хотя бы в одном углу этого прямоугольника были цвета рамки панелей!
				// Иначе - считаем что вся панель перекрыта диалогами и не видима
				BYTE btPanelFore = CONFORECOLOR(mp_FarInfo->nFarColors[col_PanelBox]);
				BYTE btPanelBack = CONBACKCOLOR(mp_FarInfo->nFarColors[col_PanelBox]);
				int nShift = 0;

				for(int i = 0; i < 4; i++) //-V112
				{
					switch(i)
					{
						case 0: nShift = nY1 * nWidth + nX1; break;
						case 1: nShift = nY2 * nWidth + nX1; break;
						case 2: nShift = nY1 * nWidth + nX2; break;
						case 3: nShift = nY2 * nWidth + nX2; break;
					}

					// Если цвет угла совпал с рамкой панели - то считаем, что это она
					if (pAttr[nShift].nForeIdx == btPanelFore && pAttr[nShift].nBackIdx == btPanelBack)
					{
						if ((nPossible & FR_RIGHTPANEL))
							mrc_RightPanel = sr; // только правая
						else
							mrc_LeftPanel = sr;  // левая или полноэкранная

						DlgFlags |= nPossible;
						bFindExterior = false;

						// Может все панели уже нашли?
						if ((nPossible & FR_FULLPANEL))
							mb_NeedPanelDetect = FALSE;
						else if ((m_DetectedDialogs.AllFlags & (FR_LEFTPANEL|FR_RIGHTPANEL)) == (FR_LEFTPANEL|FR_RIGHTPANEL))
							mb_NeedPanelDetect = FALSE;

						break;
					}
				}
			}
		}
	}

	// Может быть это QSearch?
	if (!(DlgFlags & FR_COMMONDLG_MASK) && bMarkBorder && bFindExterior)
	{
		// QSearch начинается строго на нижней рамке панели, за исключением того случая,
		// когда KeyBar отключен и панель занимает (nHeight-1) строку
		SMALL_RECT *prc = NULL;

		if ((m_DetectedDialogs.AllFlags & FR_FULLPANEL))
		{
			prc = &mrc_LeftPanel;
		}
		else if ((m_DetectedDialogs.AllFlags & FR_LEFTPANEL) && nX2 < mrc_LeftPanel.Right)
		{
			prc = &mrc_LeftPanel;
		}
		else if ((m_DetectedDialogs.AllFlags & FR_RIGHTPANEL) && nX1 > mrc_RightPanel.Left)
		{
			prc = &mrc_RightPanel;
		}

		// проверяем
		if (prc)
		{
			if ((nY1+2) == nY2 && prc->Left < nX1 && prc->Right > nX2
			        && (nY1 == prc->Bottom || (nY1 == (prc->Bottom-1) && prc->Bottom == nHeight_1)))
			{
				DlgFlags |= FR_QSEARCH;
			}
		}

		if (!(DlgFlags & FR_QSEARCH))
		{
			// Ширина диалога:
			// г========== Unicode CharMap =======[¦]=¬  .
			if ((nX1+39) == nX2 && nY2 >= (nY1 + 23))
			{
				wchar_t* pchTitle = pChar + (nY1 * nWidth + nX1 + 12);

				if (!wmemcmp(pchTitle, L"Unicode CharMap", 15))
				{
					DlgFlags |= FR_UCHARMAP;
					int nSourceIdx = MarkDialog(pChar, pAttr, nWidth, nHeight, nX1+6, nY1+3, nX2-1, nY2-5, false/*bMarkBorder*/, false/*bFindExterior*/);

					if (nSourceIdx != -1)
					{
						m_DetectedDialogs.DlgFlags[nSourceIdx] |= FR_UCHARMAPGLYPH;
						m_DetectedDialogs.AllFlags |= FR_UCHARMAPGLYPH;
					}
				}
			}
		}
	}

	if (!(DlgFlags & FR_COMMONDLG_MASK))
	{
		mn_NextDlgId++;
		DlgFlags |= mn_NextDlgId<<8;
		// "Красненький" диалог?
		BYTE btWarnBack = CONBACKCOLOR(mp_FarInfo->nFarColors[col_WarnDialogBox]);

		if (pAttr[nY1 * nWidth + nX1].nBackIdx == btWarnBack)
		{
			BYTE btNormBack = CONBACKCOLOR(mp_FarInfo->nFarColors[col_DialogBox]);

			if (btNormBack != btWarnBack)
				DlgFlags |= FR_ERRORCOLOR;
		}
	}

	// Занести координаты в новый массив прямоугольников, обнаруженных в консоли
	if (m_DetectedDialogs.Count < MAX_DETECTED_DIALOGS)
	{
		nDlgIdx = m_DetectedDialogs.Count++;
		m_DetectedDialogs.Rects[nDlgIdx].Left = nX1;
		m_DetectedDialogs.Rects[nDlgIdx].Top = nY1;
		m_DetectedDialogs.Rects[nDlgIdx].Right = nX2;
		m_DetectedDialogs.Rects[nDlgIdx].Bottom = nY2;
		//m_DetectedDialogs.bWasFrame[nDlgIdx] = bMarkBorder;
		m_DetectedDialogs.DlgFlags[nDlgIdx] = DlgFlags;
	}

#ifdef _DEBUG

	if (nX1 == 57 && nY1 == 0)
	{
		nX2 = nX2;
	}

#endif

	if (bMarkBorder)
	{
		pAttr[nY1 * nWidth + nX1].Flags2 |= CharAttr2_DialogCorner;
		pAttr[nY1 * nWidth + nX2].Flags2 |= CharAttr2_DialogCorner;
		pAttr[nY2 * nWidth + nX1].Flags2 |= CharAttr2_DialogCorner;
		pAttr[nY2 * nWidth + nX2].Flags2 |= CharAttr2_DialogCorner;
	}

	_ASSERTE(!bMarkBorder || bMarkBorder==CharAttr_DialogPanel || bMarkBorder==CharAttr_DialogRect);

	for (int nY = nY1; nY <= nY2; nY++)
	{
		CharAttr* pAttrShift = pAttr + nY * nWidth + nX1;

		if (bMarkBorder & CharAttr_DialogRect)
		{
			// Mark vertical borders only if region was not marked as dialog yet
			// Otherwise we may harm nice text flow if one dialog is displayed
			// over Far panels, and font is proportional...
			if (!(pAttrShift->Flags & CharAttr_DialogRect))
				pAttrShift->Flags |= CharAttr_DialogVBorder;
			if (!(pAttrShift[nX2-nX1].Flags & CharAttr_DialogRect))
				pAttrShift[nX2-nX1].Flags |= CharAttr_DialogVBorder;
		}

		for(int nX = nX2 - nX1 + 1; nX > 0; nX--, pAttrShift++)
		{
			/*
			if (nY > 0 && nX >= 58)
			{
				nX = nX;
			}
			*/

			pAttrShift->Flags = (pAttrShift->Flags | bMarkBorder) & ~CharAttr_Transparent;
		}
	}

	// If dialog was marked by Pseudographic frame, try to detect dialog rim (spaces and shadow)
	if (bFindExterior && bMarkBorder)
	{
		int nMostRight = nX2, nMostBottom = nY2;
		int nNewX1 = nX1, nNewY1 = nY1;
		_ASSERTE(nMostRight<nWidth);
		_ASSERTE(nMostBottom<nHeight);

		if (ExpandDialogFrame(pChar, pAttr, nWidth, nHeight, nNewX1, nNewY1, nX1, nY1, nMostRight, nMostBottom))
		{
			_ASSERTE(nNewX1>=0 && nNewY1>=0);
			DlgFlags |= FR_HASEXTENSION;

			if (nDlgIdx >= 0)
				m_DetectedDialogs.DlgFlags[nDlgIdx] = DlgFlags;

			//Optimize: помечать можно только окантовку - сам диалог уже помечен
			MarkDialog(pChar, pAttr, nWidth, nHeight, nNewX1, nNewY1, nMostRight, nMostBottom, CharAttr_DialogRect, false);
			// Еще нужно "пометить" тень под диалогом
			if (((nMostBottom+1) < nHeight) && ((nNewX1+2) < nWidth))
				MarkDialog(pChar, pAttr, nWidth, nHeight, nNewX1+2, nMostBottom+1, min(nMostRight+2,nWidth-1), nMostBottom+1, CharAttr_DialogRect, false);
			// И справа от диалога
			if (((nMostRight+1) < nWidth) && ((nNewY1+1) < nHeight))
				MarkDialog(pChar, pAttr, nWidth, nHeight, nMostRight+1, nNewY1+1, min(nMostRight+2,nWidth-1), nMostBottom, CharAttr_DialogRect, false);
		}
	}

	if ((DlgFlags & (FR_LEFTPANEL|FR_RIGHTPANEL|FR_FULLPANEL)) != 0)
	{
		// Для детекта наличия PanelTabs
		bool bSeparateTabs = false;
		RECT r = {nX1,nY1,nX2,nY2};
		int nBottom = r.bottom;
		int nLeft = r.left;
		// SeparateTabs может быть не проинициализирован
		if (r.left /*&& mp_FarInfo->PanelTabs.SeparateTabs == 0*/
		        && (m_DetectedDialogs.AllFlags & FR_LEFTPANEL))
		{
			if (mrc_LeftPanel.Bottom > r.bottom
			        && nHeight > (nBottomLines+mrc_LeftPanel.Bottom+1))
			{
				nBottom = mrc_LeftPanel.Bottom;
				nLeft = 0;
			}
			else if (nHeight > (nBottomLines+mrc_LeftPanel.Bottom+1))
			{
				nLeft = 0;
			}
		}

		// SeparateTabs может быть не проинициализирован
		int nIdxLeft, nIdxRight;
		if (DlgFlags & FR_LEFTPANEL)
		{
			nIdxLeft = nWidth*(r.bottom+1)+r.right-1;
			nIdxRight = nWidth*(r.bottom+1)+(nWidth-2);
		}
		else
		{
			nIdxLeft = nWidth*(r.bottom+1)+r.left-2;
			nIdxRight = nWidth*(r.bottom+1)+r.right-1;
		}
		if ((pChar[nIdxRight-1] == 9616 && pChar[nIdxRight] == L'+' && pChar[nIdxRight+1] == 9616)
			&& !(pChar[nIdxLeft-1] == 9616 && pChar[nIdxLeft] == L'+' && pChar[nIdxLeft+1] == 9616))
			bSeparateTabs = false; // Есть только одна строка табов (одна на обе панели)
		else
			bSeparateTabs = true;

		if (!bSeparateTabs)
		{
			r.left = nLeft;
			r.bottom = nBottom;
		}

		if (nHeight > (nBottomLines+r.bottom+1))
		{
			int nIdx = nWidth*(r.bottom+1)+r.right-1;

			if (pChar[nIdx-1] == 9616 && pChar[nIdx] == L'+' && pChar[nIdx+1] == 9616
				/* -- может быть НЕ проинициализировано
			        && pAttr[nIdx].nBackIdx == nPanelTabsBackIdx
			        && pAttr[nIdx].nForeIdx == nPanelTabsForeIdx
				*/
					)
			{
				MarkDialog(pChar, pAttr, nWidth, nHeight, r.left, r.bottom+1, r.right, r.bottom+1,
				           false, false, FR_PANELTABS|(DlgFlags & (FR_LEFTPANEL|FR_RIGHTPANEL|FR_FULLPANEL)));
			}
			#ifdef _DEBUG
			else
			{
				int nDbg2 = 0;
			}
			#endif
		}
		#ifdef _DEBUG
		else if (!(DlgFlags & FR_PANELTABS))
		{
			int nDbg = 0;
		}
		#endif
	}

	m_DetectedDialogs.AllFlags |= DlgFlags;
	return nDlgIdx;
}

void CRgnDetect::OnWindowSizeChanged()
{
	mb_SBI_Loaded = false; // данные должны быть перечитаны
}

//lpBuffer
//The data to be written to the console screen buffer. This pointer is treated as the origin of a
//two-dimensional array of CHAR_INFO structures whose size is specified by the dwBufferSize parameter.
//The total size of the array must be less than 64K.
//
//dwBufferSize
//The size of the buffer pointed to by the lpBuffer parameter, in character cells.
//The X member of the COORD structure is the number of columns; the Y member is the number of rows.
//
//dwBufferCoord
//The coordinates of the upper-left cell in the buffer pointed to by the lpBuffer parameter.
//The X member of the COORD structure is the column, and the Y member is the row.
//
//lpWriteRegion
//A pointer to a SMALL_RECT structure. On input, the structure members specify the upper-left and lower-right
//coordinates of the console screen buffer rectangle to write to.
//On output, the structure members specify the actual rectangle that was used.
void CRgnDetect::OnWriteConsoleOutput(const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion, const COLORREF *apColors)
{
	mp_Colors = apColors;

	if (!mb_SBI_Loaded)
	{
		if (!InitializeSBI(apColors))
			return;
	}

	if (!mpsz_Chars || !mp_Attrs || !mp_AttrsWork)
		return; // буфер еще не был выделен

	if ((dwBufferCoord.X >= dwBufferSize.X) || (dwBufferCoord.Y >= dwBufferSize.Y))
	{
		_ASSERTE(dwBufferCoord.X < dwBufferSize.X);
		_ASSERTE(dwBufferCoord.Y < dwBufferSize.Y);
		return; // Ошибка в параметрах
	}

	_ASSERTE(mb_TableCreated);
	SMALL_RECT rcRegion;
	rcRegion.Left = lpWriteRegion->Left - m_sbi.srWindow.Left;
	rcRegion.Right = lpWriteRegion->Right - m_sbi.srWindow.Left;
	rcRegion.Top = lpWriteRegion->Top - m_sbi.srWindow.Top;
	rcRegion.Bottom = lpWriteRegion->Bottom - m_sbi.srWindow.Top;

	// На некорректных координатах - сразу выйдем
	if (rcRegion.Left >= mn_CurWidth || rcRegion.Top >= mn_CurHeight
	        || rcRegion.Left > rcRegion.Right || rcRegion.Top > rcRegion.Bottom
	        || rcRegion.Left < 0 || rcRegion.Top < 0)
	{
		_ASSERTE(rcRegion.Left < mn_CurWidth && rcRegion.Top < mn_CurHeight);
		_ASSERTE(rcRegion.Left <= rcRegion.Right && rcRegion.Top <= rcRegion.Bottom);
		_ASSERTE(rcRegion.Left >= 0 && rcRegion.Top >= 0);
		return;
	}

	// Расфуговка буфера CHAR_INFO на текст и атрибуты
	int nX1 = max(0,rcRegion.Left);
	int nX2 = min(rcRegion.Right,(mn_CurWidth-1));
	int nY1 = max(0,rcRegion.Top);
	int nY2 = min(rcRegion.Bottom,(mn_CurHeight-1));

	if ((dwBufferSize.X - dwBufferCoord.X - 1) < (nX2 - nX1))
		nX2 = nX1 + (dwBufferSize.X - dwBufferCoord.X - 1);

	if ((dwBufferSize.Y - dwBufferCoord.Y - 1) < (nY2 - nY1))
		nY2 = nY1 + (dwBufferSize.Y - dwBufferCoord.Y - 1);

	const CHAR_INFO *pSrcLine = lpBuffer + dwBufferCoord.Y * dwBufferSize.X;

	if ((mn_MaxCells+1) > (mn_CurWidth*mn_CurHeight))
		mpsz_Chars[mn_CurWidth*mn_CurHeight] = 0;

	for (int Y = nY1; Y <= nY2; Y++)
	{
		const CHAR_INFO *pCharInfo = pSrcLine + dwBufferCoord.X;
		wchar_t  *pChar = mpsz_Chars + Y * mn_CurWidth + rcRegion.Left;
		CharAttr *pAttr = mp_Attrs + Y * mn_CurWidth + rcRegion.Left;

		for (int X = nX1; X <= nX2; X++, pCharInfo++)
		{
			TODO("OPTIMIZE: *(lpAttr++) = lpCur->Attributes;");
			*(pAttr++) = mca_Table[pCharInfo->Attributes & 0xFF];
			TODO("OPTIMIZE: ch = lpCur->Char.UnicodeChar;");
			*(pChar++) = pCharInfo->Char.UnicodeChar;
		}

		pSrcLine += dwBufferSize.X;
	}
}

void CRgnDetect::SetFarRect(SMALL_RECT *prcFarRect)
{
	if (prcFarRect)
	{
		mrc_FarRect = *prcFarRect;

		// Скопировать в m_sbi.srWindow
		if (mrc_FarRect.Bottom)
		{
			// Сначала подправим вертикаль
			if (mrc_FarRect.Bottom == m_sbi.srWindow.Bottom)
			{
				if (mrc_FarRect.Top != m_sbi.srWindow.Top)
					m_sbi.srWindow.Top = mrc_FarRect.Top;
			}
			else if (mrc_FarRect.Top == m_sbi.srWindow.Top)
			{
				if (mrc_FarRect.Bottom != m_sbi.srWindow.Bottom)
					m_sbi.srWindow.Bottom = mrc_FarRect.Bottom;
			}

			// Теперь - горизонталь
			if (mrc_FarRect.Left == m_sbi.srWindow.Left)
			{
				if (mrc_FarRect.Right != m_sbi.srWindow.Right)
					m_sbi.srWindow.Right = mrc_FarRect.Right;
			}
			else if (mrc_FarRect.Right == m_sbi.srWindow.Right)
			{
				if (mrc_FarRect.Left != m_sbi.srWindow.Left)
					m_sbi.srWindow.Left = mrc_FarRect.Left;
			}

			// Иначе - считаем что фар выбился из окна, и панели будут скрыты
		}
	}
	else
	{
		memset(&mrc_FarRect, 0, sizeof(mrc_FarRect));
	}
}

BOOL CRgnDetect::InitializeSBI(const COLORREF *apColors)
{
	//if (mb_SBI_Loaded) - всегда. Если вызвали - значит нужно все перечитать
	//	return TRUE;
	m_DetectedDialogs.AllFlags = 0;
	mp_Colors = apColors;
	//if (!mb_TableCreated) - тоже всегда. цвета могли измениться
	{
		mb_TableCreated = true;
		// формирование умолчательных цветов, по атрибутам консоли
		int  nColorIndex = 0;
		CharAttr lca;

		for(int nBack = 0; nBack <= 0xF; nBack++)
		{
			for(int nFore = 0; nFore <= 0xF; nFore++, nColorIndex++)
			{
				memset(&lca, 0, sizeof(lca));
				lca.nForeIdx = nFore;
				lca.nBackIdx = nBack;
				lca.crForeColor = lca.crOrigForeColor = mp_Colors[lca.nForeIdx];
				lca.crBackColor = lca.crOrigBackColor = mp_Colors[lca.nBackIdx];
				mca_Table[nColorIndex] = lca;
			}
		}
	}
	HANDLE hStd = GetStdHandle(STD_OUTPUT_HANDLE);
	mb_SBI_Loaded = GetConsoleScreenBufferInfo(hStd, &m_sbi)!=0;

	if (!mb_SBI_Loaded)
		return FALSE;

	if (mrc_FarRect.Right && mrc_FarRect.Bottom)
	{
		// FAR2 /w - рисует в видимой части буфера
		TODO("Возможно, нужно в srWindow копировать mrc_FarRect?");
	}
	else
	{
		// Нужно скорректировать srWindow, т.к. ФАР в любом случае забивает на видимый регион и рисует на весь буфер.
		if (m_sbi.srWindow.Left)
			m_sbi.srWindow.Left = 0;

		if ((m_sbi.srWindow.Right+1) != m_sbi.dwSize.X)
			m_sbi.srWindow.Right = m_sbi.dwSize.X - 1;

		if (m_sbi.srWindow.Top)
			m_sbi.srWindow.Top = 0;

		if ((m_sbi.srWindow.Bottom+1) != m_sbi.dwSize.Y)
			m_sbi.srWindow.Bottom = m_sbi.dwSize.Y - 1;
	}

	int nTextWidth = TextWidth();
	int nTextHeight = TextHeight();

	// Выделить память, при необходимости увеличить
	if (!mpsz_Chars || !mp_Attrs || !mp_AttrsWork || (nTextWidth * nTextHeight) > mn_MaxCells)
	{
		if (mpsz_Chars) free(mpsz_Chars);

		mn_MaxCells = (nTextWidth * nTextHeight);
		// Чтобы безопасно использовать строковые функции - гарантированно делаем ASCIIZ. Хотя mpsz_Chars может и \0 содержать?
		mpsz_Chars = (wchar_t*)calloc(mn_MaxCells+1, sizeof(wchar_t));
		mp_Attrs = (CharAttr*)calloc(mn_MaxCells, sizeof(CharAttr));
		mp_AttrsWork = (CharAttr*)calloc(mn_MaxCells, sizeof(CharAttr));
	}

	mn_CurWidth = nTextWidth;
	mn_CurHeight = nTextHeight;

	if (!mpsz_Chars || !mp_Attrs || !mp_AttrsWork)
	{
		_ASSERTE(mpsz_Chars && mp_Attrs && mp_AttrsWork);
		return FALSE;
	}

	if (!mn_MaxCells)
	{
		_ASSERTE(mn_MaxCells>0);
		return FALSE;
	}

	CHAR_INFO *pCharInfo = (CHAR_INFO*)calloc(mn_MaxCells, sizeof(CHAR_INFO));

	if (!pCharInfo)
	{
		_ASSERTE(pCharInfo);
		return FALSE;
	}

	#ifdef _DEBUG
	CHAR_INFO *pCharInfoEnd = pCharInfo+mn_MaxCells;
	#endif
	BOOL bReadOk = FALSE;
	COORD bufSize, bufCoord;
	SMALL_RECT rgn;

	// Если весь буфер больше 30К - и пытаться не будем
	if ((mn_MaxCells*sizeof(CHAR_INFO)) < 30000)
	{
		bufSize.X = nTextWidth; bufSize.Y = nTextHeight;
		bufCoord.X = 0; bufCoord.Y = 0;
		rgn = m_sbi.srWindow;

		if (ReadConsoleOutput(hStd, pCharInfo, bufSize, bufCoord, &rgn))
			bReadOk = TRUE;
	}

	if (!bReadOk)
	{
		// Придется читать построчно
		bufSize.X = nTextWidth; bufSize.Y = 1;
		bufCoord.X = 0; bufCoord.Y = 0;
		CONSOLE_SCREEN_BUFFER_INFO sbi = m_sbi;
		_ASSERTE(sbi.srWindow.Right>sbi.srWindow.Left);
		_ASSERTE(sbi.srWindow.Bottom>sbi.srWindow.Top);
		rgn = sbi.srWindow;
		CHAR_INFO* pLine = pCharInfo;
		SMALL_RECT rcFar = mrc_FarRect;
		#ifdef _DEBUG
		int nFarWidth = rcFar.Right - rcFar.Left + 1;
		#endif

		for (SHORT y = sbi.srWindow.Top; y <= sbi.srWindow.Bottom; y++, rgn.Top++, pLine+=nTextWidth)
		{
			rgn.Bottom = rgn.Top;
			rgn.Right = rgn.Left+nTextWidth-1;
			ReadConsoleOutput(hStd, pLine, bufSize, bufCoord, &rgn);
		}
	}

	// Теперь нужно перекинуть данные в mpsz_Chars & mp_Attrs
	COORD crNul = {0,0};
	// Console API uses SHORT
	_ASSERTE(!HIWORD(nTextWidth) && !HIWORD(nTextHeight));
	COORD crSize = {(SHORT)nTextWidth, (SHORT)nTextHeight};
	OnWriteConsoleOutput(pCharInfo, crSize, crNul, &m_sbi.srWindow, mp_Colors);
	// Буфер CHAR_INFO больше не нужен
	free(pCharInfo);
	return TRUE;
}

int CRgnDetect::TextWidth()
{
	int nWidth = 0;

	if (mrc_FarRect.Right && mrc_FarRect.Bottom)
	{
		// FAR2 /w - рисует в видимой части буфера
		nWidth = m_sbi.srWindow.Right - m_sbi.srWindow.Left + 1;
	}
	else
	{
		nWidth = m_sbi.dwSize.X;
	}

	return nWidth;
}

int CRgnDetect::TextHeight()
{
	int nHeight = 0;

	if (mrc_FarRect.Right && mrc_FarRect.Bottom)
	{
		// FAR2 /w - рисует в видимой части буфера
		nHeight = m_sbi.srWindow.Bottom - m_sbi.srWindow.Top + 1;
	}
	else
	{
		nHeight = m_sbi.dwSize.Y;
	}

	return nHeight;
}

BOOL CRgnDetect::GetCharAttr(int x, int y, wchar_t& rc, CharAttr& ra)
{
	if (!mpsz_Chars || !mp_Attrs)
		return FALSE;

	if (x < 0 || x >= mn_CurWidth || y < 0 || y >= mn_CurHeight)
		return FALSE;

	rc = mpsz_Chars[x + y*mn_CurWidth];
	ra = mp_Attrs[x + y*mn_CurWidth];
	return TRUE;
}


// Эта функция вызывается из плагинов (ConEmuTh)
void CRgnDetect::PrepareTransparent(const CEFAR_INFO_MAPPING *apFarInfo, const COLORREF *apColors)
{
	if (gbInTransparentAssert)
		return;

	mp_FarInfo = apFarInfo;
	mp_Colors = apColors;
	// Сброс флагов и прямоугольников панелей
	m_DetectedDialogs.AllFlags = 0; mn_NextDlgId = 0; mb_NeedPanelDetect = TRUE;
	memset(&mrc_LeftPanel,0,sizeof(mrc_LeftPanel));
	memset(&mrc_RightPanel,0,sizeof(mrc_RightPanel));

	if (!mb_SBI_Loaded)
	{
		if (!InitializeSBI(apColors))
			return;
	}

	memmove(mp_AttrsWork, mp_Attrs, mn_CurWidth*mn_CurHeight*sizeof(*mp_AttrsWork));
	PrepareTransparent(apFarInfo, apColors, &m_sbi, mpsz_Chars, mp_AttrsWork, mn_CurWidth, mn_CurHeight);
}

// Эта функция вызывается из GUI
void CRgnDetect::PrepareTransparent(const CEFAR_INFO_MAPPING *apFarInfo, const COLORREF *apColors, const CONSOLE_SCREEN_BUFFER_INFO *apSbi,
                                    wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight)
{
	_ASSERTE(pAttr!=mp_Attrs);
	mp_FarInfo = apFarInfo;
	mp_Colors = apColors;
	_ASSERTE(mp_Colors && (mp_Colors[1] || mp_Colors[2]));
	_ASSERTE(pChar[nWidth*nHeight] == 0); // Должен быть ASCIIZ

	if (apSbi != &m_sbi)
		m_sbi = *apSbi;

	mb_BufferHeight = !mrc_FarRect.Bottom && (m_sbi.dwSize.Y > (m_sbi.srWindow.Bottom - m_sbi.srWindow.Top + 10));

	if (gbInTransparentAssert)
		return;

	// Сброс флагов и прямоугольников панелей
	m_DetectedDialogs.AllFlags = 0; mn_NextDlgId = 0; mb_NeedPanelDetect = TRUE;
	memset(&mrc_LeftPanel,0,sizeof(mrc_LeftPanel));
	memset(&mrc_RightPanel,0,sizeof(mrc_RightPanel));
	// !!! После "m_DetectedDialogs.AllFlags = 0" - никаких return !!!
	m_DetectedDialogs.Count = 0;
	//if (!mp_ConsoleInfo || !gSet.NeedDialogDetect())
	//	goto wrap;
	// !!! в буферном режиме - никакой прозрачности, но диалоги - детектим, пригодится при отрисовке
	// В редакторах-вьюверах тоже нужен детект
	//if (!mp_FarInfo->bFarPanelAllowed)
	//	goto wrap;
	//if (nCurFarPID && pRCon->mn_LastFarReadIdx != pRCon->mp_ConsoleInfo->nFarReadIdx) {
	//if (isPressed(VK_CONTROL) && isPressed(VK_SHIFT) && isPressed(VK_MENU))
	//	goto wrap;
	WARNING("Если информация в FarInfo не заполнена - брать умолчания!");
	WARNING("Учитывать возможность наличия номеров окон, символа записи 'R', и по хорошему, ч/б режима");
	//COLORREF crColorKey = gSet.ColorKey;
	// реальный цвет, заданный в фаре
	nUserBackIdx = CONBACKCOLOR(mp_FarInfo->nFarColors[col_CommandLineUserScreen]);
	crUserBack = mp_Colors[nUserBackIdx];
	nMenuBackIdx = CONBACKCOLOR(mp_FarInfo->nFarColors[col_HMenuText]);
	crMenuTitleBack = mp_Colors[nMenuBackIdx];
	// COL_PANELBOX
	int nPanelBox = CONBACKCOLOR(mp_FarInfo->nFarColors[col_PanelBox]);
	crPanelsBorderBack = mp_Colors[nPanelBox];
	nPanelBox = CONFORECOLOR(mp_FarInfo->nFarColors[col_PanelBox]);
	crPanelsBorderFore = mp_Colors[nPanelBox];
	// COL_PANELSCREENSNUMBER
	int nPanelNum = CONBACKCOLOR(mp_FarInfo->nFarColors[col_PanelScreensNumber]);
	crPanelsNumberBack = mp_Colors[nPanelNum];
	nPanelNum = CONFORECOLOR(mp_FarInfo->nFarColors[col_PanelScreensNumber]);
	crPanelsNumberFore = mp_Colors[nPanelNum];
	// Цвета диалогов
	nDlgBorderBackIdx = CONBACKCOLOR(mp_FarInfo->nFarColors[col_DialogBox]);
	nDlgBorderForeIdx = CONFORECOLOR(mp_FarInfo->nFarColors[col_DialogBox]);
	nErrBorderBackIdx = CONBACKCOLOR(mp_FarInfo->nFarColors[col_WarnDialogBox]);
	nErrBorderForeIdx = CONFORECOLOR(mp_FarInfo->nFarColors[col_WarnDialogBox]);
	// Для детекта наличия PanelTabs
	bPanelTabsSeparate = (mp_FarInfo->PanelTabs.SeparateTabs != 0);

	if (mp_FarInfo->PanelTabs.ButtonColor != -1)
	{
		nPanelTabsBackIdx = CONBACKCOLOR(mp_FarInfo->PanelTabs.ButtonColor);
		nPanelTabsForeIdx = CONFORECOLOR(mp_FarInfo->PanelTabs.ButtonColor);
	}
	else
	{
		nPanelTabsBackIdx = CONBACKCOLOR(mp_FarInfo->nFarColors[col_PanelText]);
		nPanelTabsForeIdx = CONFORECOLOR(mp_FarInfo->nFarColors[col_PanelText]);
	}

	// При bUseColorKey Если панель погашена (или панели) то
	// 1. UserScreen под ним заменяется на crColorKey
	// 2. а текст - на пробелы
	// Проверять наличие KeyBar по настройкам (Keybar + CmdLine)
	bShowKeyBar = (mp_FarInfo->FarInterfaceSettings.ShowKeyBar) != 0;
	nBottomLines = bShowKeyBar ? 2 : 1;
	// Проверять наличие MenuBar по настройкам
	// Или может быть меню сейчас показано?
	// 1 - при видимом сейчас или постоянно меню
	bAlwaysShowMenuBar = (mp_FarInfo->FarInterfaceSettings.AlwaysShowMenuBar) != 0;

	if (bAlwaysShowMenuBar)
		m_DetectedDialogs.AllFlags |= FR_MENUBAR; // ставим сразу, чтобы детектор панелей не запутался

	nTopLines = bAlwaysShowMenuBar ? 1 : 0;
	// Проверка теперь в другом месте (по плагину), да и левый нижний угол могут закрыть диалогом...
	//// Проверим, что фар загрузился
	//if (bShowKeyBar) {
	//	// в левом-нижнем углу должна быть цифра 1
	//	if (pChar[nWidth*(nHeight-1)] != L'1')
	//		goto wrap;
	//	// соответствующего цвета
	//	BYTE KeyBarNoColor = mp_FarInfo->nFarColors[col_KeyBarNum];
	//	if (pAttr[nWidth*(nHeight-1)].nBackIdx != ((KeyBarNoColor & 0xF0)>>4))
	//		goto wrap;
	//	if (pAttr[nWidth*(nHeight-1)].nForeIdx != (KeyBarNoColor & 0xF))
	//		goto wrap;
	//}
	// Теперь информация о панелях хронически обновляется плагином
	//if (mb_LeftPanel)
	//	MarkDialog(pAttr, nWidth, nHeight, mr_LeftPanelFull.left, mr_LeftPanelFull.top, mr_LeftPanelFull.right, mr_LeftPanelFull.bottom);
	//if (mb_RightPanel)
	//	MarkDialog(pAttr, nWidth, nHeight, mr_RightPanelFull.left, mr_RightPanelFull.top, mr_RightPanelFull.right, mr_RightPanelFull.bottom);
	// Пометить непрозрачными панели
	RECT r;
	bool lbLeftVisible = false, lbRightVisible = false, lbFullPanel = false;

	// Хотя информация о панелях хронически обновляется плагином, но это могут и отключить,
	// да и отрисовка на экране может несколько задержаться

	//if (mb_LeftPanel) {
	if (mp_FarInfo->bFarLeftPanel)
	{
		lbLeftVisible = true;
		//r = mr_LeftPanelFull;
		r = mp_FarInfo->FarLeftPanel.PanelRect;
	}
	// -- Ветка никогда не активировалась
	_ASSERTE(mp_FarInfo->bFarLeftPanel || !mp_FarInfo->FarLeftPanel.Visible);
	//else
	//{
	//	// Но если часть панели скрыта диалогами - наш детект панели мог не сработать
	//	if (mp_FarInfo->bFarLeftPanel && mp_FarInfo->FarLeftPanel.Visible)
	//	{
	//		// В "буферном" режиме размер панелей намного больше экрана
	//		lbLeftVisible = ConsoleRect2ScreenRect(mp_FarInfo->FarLeftPanel.PanelRect, &r);
	//	}
	//}

	//RECT rLeft = {0}, rRight = {0};

	if (lbLeftVisible)
	{
		if (r.right == (nWidth-1))
			lbFullPanel = true; // значит правой быть не может

		if (r.right >= nWidth || r.bottom >= nHeight)
		{
			if (r.right >= nWidth) r.right = nWidth - 1;

			if (r.bottom >= nHeight) r.bottom = nHeight - 1;
		}

		MarkDialog(pChar, pAttr, nWidth, nHeight, r.left, r.top, r.right, r.bottom, CharAttr_DialogPanel);
	}

	if (!lbFullPanel)
	{
		//if (mb_RightPanel) {
		if (mp_FarInfo->bFarRightPanel)
		{
			lbRightVisible = true;
			r = mp_FarInfo->FarRightPanel.PanelRect; // mr_RightPanelFull;
		}
		_ASSERTE(mp_FarInfo->bFarRightPanel || !mp_FarInfo->FarRightPanel.Visible);

		if (lbRightVisible)
		{
			if (r.right >= nWidth || r.bottom >= nHeight)
			{
				if (r.right >= nWidth) r.right = nWidth - 1;

				if (r.bottom >= nHeight) r.bottom = nHeight - 1;
			}

			MarkDialog(pChar, pAttr, nWidth, nHeight, r.left, r.top, r.right, r.bottom, CharAttr_DialogPanel);
		}
	}

	mb_NeedPanelDetect = TRUE;

	// Может быть первая строка - меню? постоянное или текущее
	if (bAlwaysShowMenuBar  // всегда
	        || (pAttr->crBackColor == crMenuTitleBack
	            && (pChar[0] == L' ' && pChar[1] == L' ' && pChar[2] == L' ' && pChar[3] == L' ' && pChar[4] != L' '))
	  )
	{
		MarkDialog(pChar, pAttr, nWidth, nHeight, 0, 0, nWidth-1, 0, false, false, FR_MENUBAR);
	}

	// Редактор/вьювер
	if (mp_FarInfo->bViewerOrEditor
	        && 0 == (m_DetectedDialogs.AllFlags & (FR_LEFTPANEL|FR_RIGHTPANEL|FR_FULLPANEL)))
	{
		// Покроем полностью, включая меню и кейбар
		MarkDialog(pChar, pAttr, nWidth, nHeight, 0, 0, nWidth-1, nHeight-1, false, false, FR_VIEWEREDITOR);
	}

	if (mn_DetectCallCount != 0)
	{
		_ASSERT(mn_DetectCallCount == 0);
	}

	wchar_t* pszDst = pChar;
	CharAttr* pnDst = pAttr;
	
	const wchar_t szCornerChars[] = {
			ucBoxSinglDownRight,ucBoxSinglDownLeft,ucBoxSinglUpRight,ucBoxSinglUpLeft,
			ucBoxDblDownRight,ucBoxDblDownLeft,ucBoxDblUpRight,ucBoxDblUpLeft,
			0}; // ASCIIZ
	

	for (int nY = 0; nY < nHeight; nY++)
	{
		if (nY >= nTopLines && nY < (nHeight-nBottomLines))
		{
			// ! первый cell - резервируем для скрытия/показа панелей
			int nX1 = 0;
			int nX2 = nWidth-1; // по умолчанию - на всю ширину

			//if (!mb_LeftPanel && mb_RightPanel) {
			if (!mp_FarInfo->bFarLeftPanel && mp_FarInfo->bFarRightPanel)
			{
				// Погашена только левая панель
				nX2 = /*mr_RightPanelFull*/ mp_FarInfo->FarRightPanel.PanelRect.left-1;
				//} else if (mb_LeftPanel && !mb_RightPanel) {
			}
			else if (mp_FarInfo->bFarLeftPanel && !mp_FarInfo->bFarRightPanel)
			{
				// Погашена только правая панель
				nX1 = /*mr_LeftPanelFull*/ mp_FarInfo->FarLeftPanel.PanelRect.right+1;
			}
			else
			{
				//Внимание! Панели могут быть, но они могут быть перекрыты PlugMenu!
			}

#ifdef _DEBUG

			if (nY == 16)
			{
				nY = nY;
			}

#endif
			#if 0
			int nShift = nY*nWidth+nX1;
			#endif
			//int nX = nX1;

			if (nY == nTopLines
				&& (m_DetectedDialogs.AllFlags == 0 || m_DetectedDialogs.AllFlags == FR_MENUBAR)
		        && ((*pszDst == L'[' && pnDst->crBackColor == crPanelsNumberBack && pnDst->crForeColor == crPanelsNumberFore)
		        	||	(!nY
						&& ((*pszDst == L'P' && (pnDst->nBackIdx & 7) == 0x2 && pnDst->nForeIdx == 0xF)
							|| (*pszDst == L'R' && (pnDst->nBackIdx & 7) == 0x4 && pnDst->nForeIdx == 0xF))))
		        && (pszDst[nWidth] == ucBoxDblVert && pnDst[nWidth].crBackColor == crPanelsBorderBack && pnDst[nWidth].crForeColor == crPanelsBorderFore)
			  )
			{
				// Принудительно сдетектить, как панель
				DetectDialog(pChar, pAttr, nWidth, nHeight, 0/*nX*/, nY+1);
			}

			//wchar_t* pszDstX = pszDst + nX;
			//CharAttr* pnDstX = pnDst + nX;

			wchar_t *pszFrom = pszDst;
			wchar_t *pszEnd = pszDst + nWidth;
			
			while (pszFrom < pszEnd)
			{
				//DWORD DstFlags = pnDst[nX].Flags;
				
				wchar_t cSave = pszDst[nWidth];
				pszDst[nWidth] = 0;
				wchar_t* pszCorner = wcspbrk(pszFrom, szCornerChars);
				// Если не нашли - может в консоли '\0' есть?
				while (!pszCorner)
				{
					pszFrom += lstrlen(pszFrom)+1;
					if (pszFrom >= (pszDst + nWidth))
					{
						break;
					}
					pszCorner = wcspbrk(pszFrom, szCornerChars);
				}
				pszDst[nWidth] = cSave;
				
				if (!pszCorner)
					break;
				pszFrom = pszCorner + 1; // сразу накрутим, чтобы не забыть
				int nX = (int)(pszCorner - pszDst);

				if (
						!(pnDst[nX].Flags2/*DstFlags*/ & CharAttr2_DialogCorner)
					)
				{
					switch (pszDst[nX])
					{
						case ucBoxSinglDownRight:
						case ucBoxSinglDownLeft:
						case ucBoxSinglUpRight:
						case ucBoxSinglUpLeft:
						case ucBoxDblDownRight:
						case ucBoxDblDownLeft:
						case ucBoxDblUpRight:
						case ucBoxDblUpLeft:

							// Это правый кусок диалога, который не полностью влез на экран
							// Пометить "рамкой" до его низа
							// 100627 - во вьювере(?) будем обрабатывать только цвета диалога, во избежание замусоривания
							if (!(m_DetectedDialogs.AllFlags & FR_VIEWEREDITOR) ||
							        (pnDst[nX].nBackIdx == nDlgBorderBackIdx && pnDst[nX].nForeIdx == nDlgBorderForeIdx) ||
							        (pnDst[nX].nBackIdx == nErrBorderBackIdx && pnDst[nX].nForeIdx == nErrBorderForeIdx)
							  )
								DetectDialog(pChar, pAttr, nWidth, nHeight, nX, nY);
					}
				}

				//nX++;
				//pszDstX++;
				//pnDstX++;
				#if 0
				nShift++;
				#endif
			}
		}

		pszDst += nWidth;
		pnDst += nWidth;
	}

	if (mn_DetectCallCount != 0)
	{
		_ASSERT(mn_DetectCallCount == 0);
	}

	if (mb_BufferHeight)  // при "far /w" mb_BufferHeight==false
		goto wrap; // в буферном режиме - никакой прозрачности

	if (!lbLeftVisible && !lbRightVisible)
	{
		if (isPressed(VK_CONTROL) && isPressed(VK_SHIFT) && isPressed(VK_MENU))
			goto wrap; // По CtrlAltShift - показать UserScreen (не делать его прозрачным)
	}

	// 0x0 должен быть непрозрачным
	//pAttr[0].bDialog = TRUE;
	pszDst = pChar;
	pnDst = pAttr;

	// Только если это реально нужно, т.к. занимает значительное время.
	// Возможно, стоит необходимые проверки уже при отрисовке проводить
	if (mb_NeedTransparency)
	{
		for(int nY = 0; nY < nHeight; nY++)
		{
			if (nY >= nTopLines && nY < (nHeight-nBottomLines))
			{
				// ! первый cell - резервируем для скрытия/показа панелей
				int nX1 = 0;
				int nX2 = nWidth-1; // по умолчанию - на всю ширину
				// Все-таки, если панели приподняты - делаем UserScreen прозрачным
				// Ведь остается возможность посмотреть его по CtrlAltShift
				//if (!mb_LeftPanel && mb_RightPanel) {
				//	// Погашена только левая панель
				//	nX2 = mr_RightPanelFull.left-1;
				//} else if (mb_LeftPanel && !mb_RightPanel) {
				//	// Погашена только правая панель
				//	nX1 = mr_LeftPanelFull.right+1;
				//} else {
				//	//Внимание! Панели могут быть, но они могут быть перекрыты PlugMenu!
				//}
				
				//TODO: Far Manager: blinking on startup? until panels don't appear? become transparent?

				CharAttr* pnDstShift = pnDst + /*nY*nWidth +*/ nX1;
				CharAttr* pnDstEnd = pnDst + nX2 + 1;

				while (pnDstShift < pnDstEnd)
				{
					// If it was not marked as "Dialog"
					if (!(pnDstShift->Flags & (CharAttr_DialogPanel|CharAttr_DialogRect)))
					{
						if (pnDstShift->crBackColor == crUserBack)
						{
							pnDstShift->Flags |= CharAttr_Transparent;
						}
					}

					pnDstShift++;
				}
			}

			pszDst += nWidth;
			pnDst += nWidth;
		}
	}

	// Некрасиво...
	//// 0x0 должен быть непрозрачным
	//pAttr[0].bTransparent = FALSE;
wrap:
	mn_AllFlagsSaved = m_DetectedDialogs.AllFlags;
}

// Преобразовать абсолютные координаты консоли в координаты нашего буфера
// (вычесть номер верхней видимой строки и скорректировать видимую высоту)
bool CRgnDetect::ConsoleRect2ScreenRect(const RECT &rcCon, RECT *prcScr)
{
	if (!this) return false;

	*prcScr = rcCon;
	int nTopVisibleLine = m_sbi.srWindow.Top;
	_ASSERTE(m_sbi.srWindow.Left==0); // при работе в ConEmu - пока всегда прижато к левому краю
	int nTextWidth = (m_sbi.srWindow.Right - m_sbi.srWindow.Left + 1);
	int nTextHeight = (m_sbi.srWindow.Bottom - m_sbi.srWindow.Top + 1);

	if (mb_BufferHeight && nTopVisibleLine)
	{
		prcScr->top -= nTopVisibleLine;
		prcScr->bottom -= nTopVisibleLine;
	}

	bool lbRectOK = true;

	if (prcScr->left == 0 && prcScr->right >= nTextWidth)
		prcScr->right = nTextWidth - 1;

	if (prcScr->left)
	{
		if (prcScr->left >= nTextWidth)
			return false;

		if (prcScr->right >= nTextWidth)
			prcScr->right = nTextWidth - 1;
	}

	if (prcScr->bottom < 0)
	{
		lbRectOK = false; // полностью уехал за границу вверх
	}
	else if (prcScr->top >= nTextHeight)
	{
		lbRectOK = false; // полностью уехал за границу вниз
	}
	else
	{
		// Скорректировать по видимому прямоугольнику
		if (prcScr->top < 0)
			prcScr->top = 0;

		if (prcScr->bottom >= nTextHeight)
			prcScr->bottom = nTextHeight - 1;

		lbRectOK = (prcScr->bottom > prcScr->top);
	}

	return lbRectOK;
}




CRgnRects::CRgnRects()
{
	nRectCount = 0;
	nRgnState = NULLREGION;
	nFieldMaxCells = nFieldWidth = nFieldHeight = 0;
	pFieldCells = NULL;
	memset(rcRect, 0, sizeof(rcRect));
}

CRgnRects::~CRgnRects()
{
	if (pFieldCells) free(pFieldCells);
}

// Сброс всего в NULLREGION
void CRgnRects::Reset()
{
	nRectCount = 0;
	nRgnState = NULLREGION;
}

// Сбросить все прямоугольники и установить rcRect[0]
void CRgnRects::Init(LPRECT prcInit)
{
	// Установим начальный прямоугольник
	nRectCount = 1;
	rcRect[0] = *prcInit;
	nRgnState = SIMPLEREGION;
	// готовим поле для проверки
	nFieldWidth = prcInit->right - prcInit->left + 1;
	nFieldHeight = prcInit->bottom - prcInit->top + 1;

	if (nFieldWidth<1 || nFieldHeight<1)
	{
		nRgnState = NULLREGION;
		return;
	}

	if (!nFieldMaxCells || !pFieldCells || nFieldMaxCells < (nFieldWidth*nFieldHeight))
	{
		if (pFieldCells) free(pFieldCells);

		nFieldMaxCells = nFieldWidth*nFieldHeight;
		_ASSERTE(sizeof(*pFieldCells)==1);
		pFieldCells = (bool*)calloc(nFieldMaxCells,1);

		if (!pFieldCells)
		{
			_ASSERTE(pFieldCells!=NULL);
			nRgnState = RGN_ERROR;
			return;
		}
	}
	else
	{
		memset(pFieldCells, 0, nFieldMaxCells);
	}
}

// Combines the parts of rcRect[..] that are not part of prcAddDiff.
int CRgnRects::Diff(LPRECT prcAddDiff)
{
	if (!pFieldCells || nRectCount>=MAX_RGN_RECTS || nRgnState <= NULLREGION)
		return nRgnState; // регион уже пустой, ничего делать не нужно

	if (prcAddDiff->left > prcAddDiff->right || prcAddDiff->top > prcAddDiff->bottom)
	{
		_ASSERTE(prcAddDiff->left <= prcAddDiff->right && prcAddDiff->top <= prcAddDiff->bottom);
		return nRgnState; // не валидный prcAddDiff
	}

	// поехали
	int X1 = rcRect[0].left, X2 = rcRect[0].right;

	if (prcAddDiff->left > X2 || prcAddDiff->right < X1)
		return nRgnState; // prcAddDiff не пересекается с rcRect[0]

	int iX1 = max(prcAddDiff->left,X1);
	int iX2 = min(prcAddDiff->right,X2);

	if (iX2 < iX1)
	{
		_ASSERTE(iX2 >= iX1);
		return nRgnState; // пустая область?
	}

	int Y1 = rcRect[0].top,  Y2 = rcRect[0].bottom;

	if (prcAddDiff->top > Y2 || prcAddDiff->bottom < Y1)
		return nRgnState; // prcAddDiff не пересекается с rcRect[0]

	int iY1 = max(prcAddDiff->top,Y1);
	int iY2 = min(prcAddDiff->bottom,Y2);
	// Ладно, добавим этот прямоугольник в список вычитаемых
	rcRect[nRectCount++] = *prcAddDiff;
	int Y, iy = iY1 - rcRect[0].top;
	int nSubWidth = iX2 - iX1 + 1;
	_ASSERTE(iy>=0);

	for(Y = iY1; Y <= iY2; Y++, iy++)
	{
		int ix = iX1 - X1;
		_ASSERTE(ix>=0);
		int ii = (iy*nFieldWidth) + ix;
		memset(pFieldCells+ii, 1, nSubWidth);
		//for (int X = iX1; X <= iX2; X++, ii++) {
		//	if (!pFieldCells[ii]) {
		//		pFieldCells[ii] = true;
		//	}
		//}
	}

	// Теперь проверить, а осталось ли еще что-то?
	Y2 = nFieldWidth * nFieldHeight;
	void* ptrCellLeft = memchr(pFieldCells, 0, Y2);
	//bool lbAreCellLeft = (ptrCellLeft != NULL);
	//for (Y = 0; Y < Y2; Y++) {
	//	if (!pFieldCells[ii]) {
	//		lbAreCellLeft = true;
	//		break;
	//	}
	//}

	if (ptrCellLeft != NULL)
	{
		nRgnState = COMPLEXREGION;
	}
	else
	{
		nRgnState = NULLREGION;
	}

	return nRgnState;
}

int CRgnRects::DiffSmall(SMALL_RECT *prcAddDiff)
{
	if (!prcAddDiff)
		return Diff(NULL);

	RECT rc = {prcAddDiff->Left,prcAddDiff->Top,prcAddDiff->Right,prcAddDiff->Bottom};
	return Diff(&rc);
}

// Скопировать ИЗ pRgn, вернуть true - если были отличия
bool CRgnRects::LoadFrom(CRgnRects* pRgn)
{
	bool lbChanges = false;

	if (!pRgn)
	{
		// Если до этого был НЕ пустой регион
		lbChanges = (nRgnState >= SIMPLEREGION);
		// сброс
		Reset();
	}
	else
	{
		if (nRectCount != pRgn->nRectCount || nRgnState != pRgn->nRgnState)
		{
			lbChanges = true;
		}
		else if (pRgn->nRectCount)
		{
			if (memcmp(rcRect, pRgn->rcRect, pRgn->nRectCount * sizeof(RECT)) != 0)
				lbChanges = true;
		}

		nRectCount = pRgn->nRectCount;
		nRgnState = pRgn->nRgnState;

		if (nRectCount > 0)
		{
			memmove(rcRect, pRgn->rcRect, nRectCount * sizeof(RECT));
		}
	}

	// Нас интересуют только прямоугольники
	if (pFieldCells)
	{
		free(pFieldCells); pFieldCells = NULL;
	}

	return lbChanges;
}

void CRgnDetect::SetNeedTransparency(bool abNeed)
{
	mb_NeedTransparency = abNeed;
}