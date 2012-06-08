
/*
Copyright (c) 2009-2012 Maximus5
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

#define SHOWDEBUGSTR

#define DEBUGSTRSTAT(s) //DEBUGSTR(s)

#include <windows.h>
#include <commctrl.h>
#include "header.h"
#include "Options.h"
#include "ConEmu.h"
#include "RealConsole.h"
#include "VirtualConsole.h"
#include "Status.h"


#define STATUS_PAINT_DELAY 500


const wchar_t gsReady[] = L"Waiting...";

// Информация по колонкам
static struct StatusColInfo {
	CEStatusItems nID;
	LPCWSTR sSettingName;
	LPCWSTR sName; // Shown in menu (select active columns)
	LPCWSTR sHelp; // Shown in Info, when hover over item
} gStatusCols[] = {
	// строго первая, НЕ отключаемая
	{csi_Info,  NULL,   L"Show status bar",
						L"Hide status bar, you may restore it later from 'Settings...'"},
	// Далее - по настройкам
	{csi_ActiveVCon,	L"StatusBar.Hide.VCon",
						L"Active VCon",
						L"ActiveCon/TotalCount, left click to change"},

	{csi_CapsLock,		L"StatusBar.Hide.CapsL",
						L"Caps Lock state",
						L"Caps Lock state, left click to change"},

	{csi_NumLock,		L"StatusBar.Hide.NumL",
						L"Num Lock state",
						L"Num Lock state, left click to change"},

	{csi_ScrollLock,	L"StatusBar.Hide.ScrL",
						L"Scroll Lock state",
						L"Scroll Lock state, left click to change"},

	{csi_InputLocale,	L"StatusBar.Hide.Lang",
						L"Current input HKL",
						L"Active input locale identifier - GetKeyboardLayout"},

	{csi_WindowPos,		L"StatusBar.Hide.WPos",
						L"ConEmu window rectangle",
						L"(X1, Y1)-(X2, Y2): ConEmu window rectangle"},

	{csi_WindowSize,	L"StatusBar.Hide.WSize",
						L"ConEmu window size",
						L"Width x Height: ConEmu window size"},

	{csi_WindowClient,	L"StatusBar.Hide.WClient",
						L"ConEmu window client area size",
						L"Width x Height: ConEmu client area size (without caption and frame)"},

	{csi_WindowWork,	L"StatusBar.Hide.WWork",
						L"ConEmu window work area size",
						L"Width x Height: ConEmu work area size (virtual consoles place)"},

	{csi_ActiveBuffer,	L"StatusBar.Hide.ABuf",
						L"Active console buffer",
						L"PRI/ALT/SEL/FND/DMP: Active console buffer"},

	{csi_ConsolePos,	L"StatusBar.Hide.CPos",
						L"Console visible rectangle",
						L"(Left, Top)-(Right,Bottom): Console visible rect, 0-based"},

	{csi_ConsoleSize,	L"StatusBar.Hide.CSize",
						L"Console visible size",
						L"Width x Height: Console visible rect"},

	{csi_BufferSize,	L"StatusBar.Hide.BSize",
						L"Console buffer size",
						L"Width x Height: Console buffer size"},

	{csi_CursorX,		L"StatusBar.Hide.CurX",
						L"Cursor column",
						L"Col: Console cursor pos, 0-based"},

	{csi_CursorY,		L"StatusBar.Hide.CurY",
						L"Cursor row",
						L"Row: Console cursor pos, 0-based"},

	{csi_CursorSize,	L"StatusBar.Hide.CurS",
						L"Cursor size / visibility",
						L"Height (V|H): Console cursor size and visibility"},

	{csi_CursorInfo,	L"StatusBar.Hide.CurI",
						L"Cursor information",
						L"Col x Row, Height (visible|hidden): Console cursor, 0-based"},

	{csi_ConEmuPID,		L"StatusBar.Hide.ConEmuPID",
						L"ConEmu GUI PID",
						L"ConEmu GUI PID"},

	{csi_Server,		L"StatusBar.Hide.Srv",
						L"Console server PID",
						L"Server PID / AltServer PID"},

	{csi_Transparency,	L"StatusBar.Hide.Transparency",
						L"Transparency",
						L"Transparency, left click to change"},
};

static struct StatusTranspOptions {
	WORD nMenuID;
	LPCWSTR sText;
	int nValue;
} gTranspOpt[] = {
	{1, L"Transparent, 40%", 40},
	{2, L"Transparent, 50%", 50},
	{3, L"Transparent, 60%", 60},
	{4, L"Transparent, 70%", 70},
	{5, L"Transparent, 80%", 80},
	{6, L"Transparent, 90%", 90},
	{0},
	{7, L"UserScreen transparency", 1},
	{8, L"ColorKey transparency",   2},
	{9, L"Opaque, 100%",     100},
};


CStatus::CStatus()
{
	//mb_WasClick = false;
	mb_InPopupMenu = false;

	mb_DataChanged = true;
	//mb_Invalidated = false;
	mn_LastPaintTick = 0;

	m_ClickedItemDesc = csi_Last;
	mb_InSetupMenu = false;

	ZeroStruct(m_Items);
	ZeroStruct(m_Values);
	ZeroStruct(mrc_LastStatus);

	//mn_BmpSize; -- не важно
	mb_OldBmp = mh_Bmp = NULL; mh_MemDC = NULL;

	for (int i = csi_Info; i < csi_Last; i++)
	{
		for (size_t j = 0; j < countof(gStatusCols); j++)
		{
			if (i == gStatusCols[j].nID)
			{
				m_Values[i].sName = gStatusCols[j].sName;
				m_Values[i].sHelp = gStatusCols[j].sHelp ? gStatusCols[j].sHelp : gStatusCols[j].sName;
				m_Values[i].sSettingName = gStatusCols[j].sSettingName;
				wcscpy_c(m_Values[i].sText, L" ");
				wcscpy_c(m_Values[i].szFormat, L" ");
				break;
			}
		}
	}

	#ifdef _DEBUG
	for (int j = csi_Info+1; j < csi_Last; j++)
	{
		// Проверяем, все csi_xxx элементы должны быть описаны в gStatusCols
		_ASSERTE(m_Values[j].sHelp && m_Values[j].sName && m_Values[j].sSettingName);
	}
	#endif

	_wsprintf(m_Values[csi_ConEmuPID].sText, SKIPLEN(countof(m_Values[csi_ConEmuPID].sText)) L"%u", GetCurrentProcessId());

	// Init some values
	OnTransparency();
}

CStatus::~CStatus()
{
	if (mh_MemDC)
	{
		SelectObject(mh_MemDC, mb_OldBmp);
		DeleteObject(mh_Bmp);
		DeleteDC(mh_MemDC);
	}
}

// true = text changed
bool CStatus::FillItems()
{
	bool lbChanged = false;

	TODO("CStatus::FillItems()");

	return lbChanged;
}

void CStatus::PaintStatus(HDC hPaint, RECT rcStatus)
{
	// Сразу сбросим
	//mb_Invalidated = false;

	bool bDataChanged = mb_DataChanged;
	mb_DataChanged = false;

	int nStatusWidth = rcStatus.right-rcStatus.left+1;
	int nStatusHeight = rcStatus.bottom-rcStatus.top+1;

	// Проверить клавиатуру
	IsKeyboardChanged();

	if (!mh_MemDC || (nStatusWidth != mn_BmpSize.cx) || (nStatusHeight != mn_BmpSize.cy))
	{
		if (mh_MemDC)
		{
			SelectObject(mh_MemDC, mb_OldBmp);
			DeleteObject(mh_Bmp);
			DeleteDC(mh_MemDC);
		}

		mh_MemDC = CreateCompatibleDC(hPaint);
		mh_Bmp = CreateCompatibleBitmap(hPaint, nStatusWidth, nStatusHeight);
		mb_OldBmp = (HBITMAP)SelectObject(mh_MemDC, mh_Bmp);

		mn_BmpSize.cx = nStatusWidth; mn_BmpSize.cy = nStatusHeight;
	}

	bool lbFade = gpSet->isFadeInactive && !gpConEmu->isMeForeground(true);

	COLORREF crBack = lbFade ? gpSet->GetFadeColor(gpSet->nStatusBarBack) : gpSet->nStatusBarBack;
	COLORREF crText = lbFade ? gpSet->GetFadeColor(gpSet->nStatusBarLight) : gpSet->nStatusBarLight;
	COLORREF crDash = lbFade ? gpSet->GetFadeColor(gpSet->nStatusBarDark) : gpSet->nStatusBarDark;

	HBRUSH hBr = CreateSolidBrush(crBack);
	//HPEN hLight = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
	//HPEN hShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW));
	HPEN hDash = CreatePen(PS_SOLID, 1, crDash);
	HPEN hOldP = (HPEN)SelectObject(mh_MemDC, hDash);

	TODO("sStatusFontFace");
	HFONT hFont = CreateFont(gpSet->StatusBarFontHeight(), 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, gpSet->nStatusFontCharSet, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, gpSet->sStatusFontFace);
	HFONT hOldF = (HFONT)SelectObject(mh_MemDC, hFont);

	RECT rcFill = {0,0, nStatusWidth, nStatusHeight};
	FillRect(mh_MemDC, &rcFill, hBr);



	CRealConsole* pRCon = NULL;
	CVirtualConsole* pVCon = gpConEmu->ActiveCon();
	CVConGuard guard(pVCon);
	if (pVCon)
		pRCon = pVCon->RCon();

	// Сформировать элементы
	size_t nDrawCount = 0;
	//size_t nID = 1;
	LPCWSTR pszTmp;
	CONSOLE_CURSOR_INFO ci = {};
	CONSOLE_SCREEN_BUFFER_INFO sbi = {};
	RealBufferType tBuffer = rbt_Undefined;
	SIZE szTemp;

	if (pRCon)
	{
		pRCon->GetConsoleCursorInfo(&ci);
		pRCon->GetConsoleScreenBufferInfo(&sbi);
		tBuffer = pRCon->GetActiveBufferType();
	}

	int nGapWidth = 2;
	int nDashWidth = 1;
	int nMinInfoWidth = 80;
	int nTotalWidth = nGapWidth, nWidth = 0;
	int iInfoID = -1;

	//while (nID <= ces_Last)
	for (int i = csi_Info; i < csi_Last; i++)
	{
		CEStatusItems nID = (CEStatusItems)i;
		_ASSERTE(nID < countof(m_Values));

		// Info - показываем всегда!
		if (nID != csi_Info)
		{
			_ASSERTE(nID<countof(gpSet->isStatusColumnHidden));
			if (gpSet->isStatusColumnHidden[nID])
				continue; // просили не показывать
		}

		m_Items[nDrawCount].nID = nID;

		switch (nID)
		{
			case csi_Info:
			{
				pszTmp = ((mb_InSetupMenu && m_ClickedItemDesc == csi_Info)
						|| (m_ClickedItemDesc > csi_Info && m_ClickedItemDesc < csi_Last))
					? m_Values[m_ClickedItemDesc].sHelp : pRCon ? pRCon->GetConStatus() : NULL;
				
				if (pszTmp && *pszTmp)
				{
					lstrcpyn(m_Items[nDrawCount].sText, pszTmp, countof(m_Items[nDrawCount].sText));
				}
				/* -- вроде и так работает через pRCon->GetConStatus()
				else if (pRCon && pRCon->isSelectionPresent())
				{
					CONSOLE_SELECTION_INFO sel = {};
					pRCon->GetConsoleSelectionInfo(&sel);
				}
				*/
				else if (pRCon)
				{
					DWORD nPID = pRCon->GetActivePID();
					LPCWSTR pszName = pRCon->GetActiveProcessName();
					if (nPID)
					{
						TODO("Показать все дерево запущенных процессов");
						wchar_t* psz = m_Items[nDrawCount].sText;
						int nCchLeft = countof(m_Items[nDrawCount].sText);
						lstrcpyn(psz, (pszName && *pszName) ? pszName : L"???", 64);
						int nCurLen = lstrlen(psz);
						_wsprintf(psz+nCurLen, SKIPLEN(nCchLeft-nCurLen) _T(":%u « %s%s"), 
							nPID, gpConEmu->ms_ConEmuBuild, WIN3264TEST(L"x86",L"x64"));
					}
					else
						lstrcpyn(m_Items[nDrawCount].sText, gsReady, countof(m_Items[nDrawCount].sText));
				}
				else
				{
					wcscpy_c(m_Items[nDrawCount].sText, L" ");
				}
				wcscpy_c(m_Items[nDrawCount].szFormat, L"Ready");

				break;
			} // csi_Info

			case csi_CapsLock:
				wcscpy_c(m_Items[nDrawCount].sText, L"CAPS");
				wcscpy_c(m_Items[nDrawCount].szFormat, L"CAPS");
				break;
			case csi_NumLock:
				wcscpy_c(m_Items[nDrawCount].sText, L"NUM");
				wcscpy_c(m_Items[nDrawCount].szFormat, L"NUM");
				break;
			case csi_ScrollLock:
				wcscpy_c(m_Items[nDrawCount].sText, L"SCRL");
				wcscpy_c(m_Items[nDrawCount].szFormat, L"SCRL");
				break;
			case csi_InputLocale:
				// чтобы не задавали вопросов, нафига дублируется.
				if (LOWORD((DWORD)mhk_Locale) == HIWORD((DWORD)mhk_Locale))
				{
					_wsprintf(m_Items[nDrawCount].sText, SKIPLEN(countof(m_Items[nDrawCount].sText)-1) L"%04X", LOWORD((DWORD)mhk_Locale));
					wcscpy_c(m_Items[nDrawCount].szFormat, L"FFFF");
				}
				else
				{
					_wsprintf(m_Items[nDrawCount].sText, SKIPLEN(countof(m_Items[nDrawCount].sText)-1) L"%08X", (DWORD)mhk_Locale);
					wcscpy_c(m_Items[nDrawCount].szFormat, L"FFFFFFFF");
				}
				
				break;

			default:
				wcscpy_c(m_Items[nDrawCount].sText, m_Values[nID].sText);
				wcscpy_c(m_Items[nDrawCount].szFormat, m_Values[nID].szFormat);
		}

		m_Items[nDrawCount].nTextLen = lstrlen(m_Items[nDrawCount].sText);

		if ((nID != csi_Info) && !m_Items[nDrawCount].nTextLen)
		{
			continue;
		}

		if (!GetTextExtentPoint32(mh_MemDC, m_Items[nDrawCount].sText, m_Items[nDrawCount].nTextLen, &m_Items[nDrawCount].TextSize) || (m_Items[nDrawCount].TextSize.cx <= 0))
		{
			if (nID != csi_Info)
				continue;
		}
		if (*m_Items[nDrawCount].szFormat && GetTextExtentPoint32(mh_MemDC, m_Items[nDrawCount].szFormat, lstrlen(m_Items[nDrawCount].szFormat), &szTemp))
		{
			m_Items[nDrawCount].TextSize.cx = max(m_Items[nDrawCount].TextSize.cx, szTemp.cx);
		}
		
		if ((nID == csi_Info) && (m_Items[nDrawCount].TextSize.cx < nMinInfoWidth))
			m_Items[nDrawCount].TextSize.cx = nMinInfoWidth;

		//if (nDrawCount) // перед вторым и далее - добавить разделитель
		//	nTotalWidth += 2*nGapWidth + nDashWidth;

		if (nID == csi_Info) // csi_Info добавим потом
			iInfoID = (int)nDrawCount;
		//else
		//	nTotalWidth += m_Items[nDrawCount].TextSize.cx;

		m_Items[nDrawCount++].bShow = TRUE;

		//Next:
		//	nID = nID<<1;
	}

	if (iInfoID != 0)
	{
		_ASSERTE(iInfoID == 0);
		goto wrap;
	}

	//nTotalWidth += nGapWidth;

	int x = 0;

	// Сброс неиспользуемых ячеек
	for (size_t i = nDrawCount; i < countof(m_Items); i++)
	{
		m_Items[i].bShow = FALSE;
	}



	if (nDrawCount == 1)
	{
		m_Items[0].TextSize.cx = min((nStatusWidth - 2*nGapWidth - 1),m_Items[0].TextSize.cx);
	}
	else
	{
		// Подсчет допустимой ширины
		nTotalWidth = nMinInfoWidth + 2*nGapWidth + nDashWidth;

		for (size_t i = 1; i < nDrawCount; i++)
		{
			if (m_Items[i].bShow)
			{
				if ((nTotalWidth+m_Items[i].TextSize.cx+2*nGapWidth) > nStatusWidth)
				{
					m_Items[i].bShow = FALSE;
					// Но продолжим, может следующая ячейки будет уже и "влезет"
				}
				else
				{
					// Раз этот элемент показан
					nTotalWidth += (m_Items[i].TextSize.cx + 2*nGapWidth + nDashWidth);
				}
			}
			else
			{
				break; // дальше смысла нет - ячейки кончились
			}
		}


		//if (nMinInfoWidth < (nStatusWidth - nTotalWidth))
		//{
		//	//int nMaxInfoWidth = nStatusWidth - nTotalWidth - nGapWidth - 1;
		//	//m_Items[0].TextSize.cx = max(nMinInfoWidth,nMaxInfoWidth);
		//	m_Items[0].TextSize.cx = nStatusWidth - nTotalWidth;
		//}

		m_Items[0].TextSize.cx = max(nMinInfoWidth,(nStatusWidth - nTotalWidth + nMinInfoWidth));
	}

	

	SetTextColor(mh_MemDC, crText);
	SetBkColor(mh_MemDC, crBack);
	SetBkMode(mh_MemDC, TRANSPARENT);

	x = 0;
	for (size_t i = 0; i < nDrawCount; i++)
	{
		if (!m_Items[i].bShow || (m_Items[i].nID >= csi_Last) || (i && !m_Items[i].nID))
			continue;

		RECT rcField = {x, 0, x+m_Items[i].TextSize.cx+2*nGapWidth, nStatusHeight};
		if (rcField.right > nStatusWidth)
		{
			_ASSERTE(rcField.right <= nStatusWidth); // должно было отсечься на предыдущем цикле!
			break;
		}
		if ((i+1) == nDrawCount)
			rcField.right = nStatusWidth;


		// Запомнить, чтобы клики обрабатывать...
		m_Items[i].rcClient = MakeRect(rcField.left+rcStatus.left, rcStatus.top, rcField.right+rcStatus.left, rcStatus.bottom);

		switch (m_Items[i].nID)
		{
		case csi_CapsLock:
			SetTextColor(mh_MemDC, mb_Caps ? crText : crDash);
			break;
		case csi_NumLock:
			SetTextColor(mh_MemDC, mb_Num ? crText : crDash);
			break;
		case csi_ScrollLock:
			SetTextColor(mh_MemDC, mb_Scroll ? crText : crDash);
			break;
		}

		RECT rcText = {rcField.left+1, rcField.top+1, rcField.right-1, rcField.bottom-1};

		DrawText(mh_MemDC, m_Items[i].sText, m_Items[i].nTextLen, &rcText, 
			((m_Items[i].nID == csi_Info) ? DT_LEFT : DT_CENTER)
			|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);

		switch (m_Items[i].nID)
		{
		case csi_CapsLock:
		case csi_NumLock:
		case csi_ScrollLock:
			SetTextColor(mh_MemDC, crText);
			break;
		}

		if ((i+1) < nDrawCount)
		{
			MoveToEx(mh_MemDC, rcField.right, rcField.top, NULL);
			LineTo(mh_MemDC, rcField.right, rcField.bottom+1);
		}

		x = rcField.right + nDashWidth;
	}


wrap:
	BitBlt(hPaint, rcStatus.left, rcStatus.top, nStatusWidth, nStatusHeight, mh_MemDC, 0,0, SRCCOPY);


	SelectObject(mh_MemDC, hOldF);
	SelectObject(mh_MemDC, hOldP);

	DeleteObject(hBr);
	DeleteObject(hDash);
	DeleteObject(hFont);

	mrc_LastStatus = rcStatus;
	mn_LastPaintTick = GetTickCount();
}

// true = обновлять строго, сменился шрифт, размер, или еще что...
void CStatus::UpdateStatusBar(bool abForce /*= false*/)
{
	if (!gpSet->isStatusBarShow || !ghWnd)
		return;

	mb_DataChanged	= mb_DataChanged | abForce;

	//if (mb_Invalidated)
	//	return; // уже дернули

	if (!abForce)
	{
		// Чтобы не было слишком частой отрисовки и нагрузки на систему
		DWORD nDelta = (GetTickCount() - mn_LastPaintTick);
		if (nDelta < STATUS_PAINT_DELAY)
			return; // обновится по следующему таймеру
	}

	InvalidateStatusBar();
}

void CStatus::InvalidateStatusBar()
{
	RECT rcClient = {};
	if (!GetStatusBarClientRect(&rcClient))
		return;

	InvalidateRect(ghWnd, &rcClient, FALSE);
}

void CStatus::OnTimer()
{
	if (!gpSet->isStatusBarShow)
		return;

	// Если сейчас нет меню, и был показан hint для колонки - 
	// проверить позицию мышки
	if ((m_ClickedItemDesc != csi_Last) && !mb_InPopupMenu)
	{
		if (gpConEmu->isSizing())
		{
			SetClickedItemDesc(csi_Last);
		}
		else
		{
			POINT ptCurClient = {}; GetCursorPos(&ptCurClient);
			MapWindowPoints(NULL, ghWnd, &ptCurClient, 1);
			LRESULT l;
			ProcessStatusMessage(ghWnd, WM_MOUSEMOVE, 0, 0, ptCurClient, l);
		}
	}

	if (/*!mb_Invalidated &&*/ !mb_DataChanged && IsKeyboardChanged())
		mb_DataChanged = true; // Изменения в клавиатуре

	if (mb_DataChanged /*&& !mb_Invalidated*/)
		UpdateStatusBar(true);
}

void CStatus::SetClickedItemDesc(CEStatusItems anID)
{
	if (m_ClickedItemDesc != anID)
	{
		if (((anID > csi_Info && anID < csi_Last) || (mb_InSetupMenu && anID == csi_Info))
			&& m_Values[anID].sHelp)
			m_ClickedItemDesc = anID;
		else
			m_ClickedItemDesc = csi_Last;

		UpdateStatusBar(true);
	}
}

bool CStatus::ProcessStatusMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, POINT ptCurClient, LRESULT& lResult)
{
	if (!gpSet->isStatusBarShow)
		return false; // Разрешить дальнейшую обработку

	lResult = 0;

	if (gpConEmu->isSizing() || !PtInRect(&mrc_LastStatus, ptCurClient))
	{
		//bool bWasClick = mb_WasClick;
		//mb_WasClick = false;
		if (m_ClickedItemDesc != csi_Last)
		{
			m_ClickedItemDesc = csi_Last;
			UpdateStatusBar(true);
		}
		return false; //bWasClick;
	}

	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_MOUSEMOVE:
		{
			RECT rcClient = {};
			bool bFound = false;
			for (size_t i = 0; i < countof(m_Items); i++)
			{
				if (m_Items[i].bShow && PtInRect(&m_Items[i].rcClient, ptCurClient))
				{
					bFound = true;
					SetClickedItemDesc(m_Items[i].nID);
					rcClient = m_Items[i].rcClient;
					break;
				}
			} // for (size_t i = 0; i < countof(m_Items); i++)

			if (!bFound)
			{
				SetClickedItemDesc(csi_Last);
			}
			
			UpdateStatusBar(true);

			if ((uMsg == WM_LBUTTONDOWN) || (uMsg == WM_LBUTTONDBLCLK))
			{
				MapWindowPoints(ghWnd, NULL, (LPPOINT)&rcClient, 2);

				switch (m_ClickedItemDesc)
				{
				case csi_ActiveVCon:
					if (uMsg == WM_LBUTTONDOWN)
						ShowVConMenu(MakePoint(rcClient.left, rcClient.top));
					break;
				case csi_Transparency:
					if (uMsg == WM_LBUTTONDOWN)
						ShowTransparencyMenu(MakePoint(rcClient.right, rcClient.top));
					break;
				case csi_CapsLock:
					keybd_event(VK_CAPITAL, 0, 0, 0);
					keybd_event(VK_CAPITAL, 0, KEYEVENTF_KEYUP, 0);
					break;
				case csi_NumLock:
					keybd_event(VK_NUMLOCK, 0, 0, 0);
					keybd_event(VK_NUMLOCK, 0, KEYEVENTF_KEYUP, 0);
					break;
				case csi_ScrollLock:
					keybd_event(VK_SCROLL, 0, 0, 0);
					keybd_event(VK_SCROLL, 0, KEYEVENTF_KEYUP, 0);
					break;
				}
			}
		}
		break; // WM_LBUTTONDOWN

	//case WM_LBUTTONUP:
	//	if (mb_MouseCaptured)
	//	{
	//		ReleaseCapture();
	//		mb_WasClick = false;
	//		if (ms_ClickedItemDesc[0])
	//		{	
	//			ms_ClickedItemDesc[0] = 0;
	//			UpdateStatusBar(true);
	//		}
	//		return true;
	//	}
	//	break;

	case WM_RBUTTONUP:
		ShowStatusSetupMenu();
		break;
	}

	// Значит курсор над статусом
	return true;
}

void CStatus::ShowStatusSetupMenu()
{
	if (isSettingsOpened(IDD_SPG_STATUSBAR))
		return;

	mb_InPopupMenu = true;

	HMENU hPopup = CreatePopupMenu();

	POINT ptCur = {}; ::GetCursorPos(&ptCur);
	POINT ptClient = ptCur;
	MapWindowPoints(NULL, ghWnd, &ptClient, 1);

	int nClickedID = -1;

	// m_Items содержит только видимые элементы, поэтому индексы не совпадают, надо искать
	for (int j = 0; j < countof(m_Items); j++)
	{
		if (m_Items[j].bShow && PtInRect(&m_Items[j].rcClient, ptClient))
		{
			nClickedID = m_Items[j].nID;
			break;
		}
	}

	for (int i = 1; i <= (int)countof(gStatusCols); i++)
	{
		if (i == 1)
		{
			AppendMenu(hPopup, MF_STRING|(gpSet->isStatusBarShow ? MF_CHECKED : MF_UNCHECKED), i, gStatusCols[i-1].sName);
			AppendMenu(hPopup, MF_SEPARATOR, 0, L"");
		}
		else
		{
			AppendMenu(hPopup, MF_STRING|((gpSet->isStatusColumnHidden[gStatusCols[i-1].nID]) ? MF_UNCHECKED : MF_CHECKED), i, gStatusCols[i-1].sName);
		}

		// m_Items содержит только видимые элементы, поэтому индексы не совпадают
		if (nClickedID == gStatusCols[i-1].nID)
		{
			MENUITEMINFO mi = {sizeof(mi), MIIM_STATE|MIIM_ID};
			mi.wID = i;
			GetMenuItemInfo(hPopup, i, FALSE, &mi);
			mi.fState |= MF_DEFAULT;
			SetMenuItemInfo(hPopup, i, FALSE, &mi);
		}
	}

	AppendMenu(hPopup, MF_SEPARATOR, 0, L"");
	AppendMenu(hPopup, MF_STRING, (int)countof(gStatusCols)+1, L"Setup...");

	mb_InSetupMenu = true;
	int nCmd = gpConEmu->trackPopupMenu(tmp_StatusBarCols, hPopup, TPM_BOTTOMALIGN|TPM_LEFTALIGN|TPM_RETURNCMD, ptCur.x, ptCur.y, 0, ghWnd, NULL);
	mb_InSetupMenu = false;

	if (nCmd == ((int)countof(gStatusCols)+1))
	{
		CSettings::Dialog(IDD_SPG_STATUSBAR);
	}
	else if ((nCmd >= 1) && (nCmd <= countof(gStatusCols)))
	{
		if (nCmd == 1)
		{
			gpSet->isStatusBarShow = !gpSet->isStatusBarShow;
			// Должна справиться с reposition элементов
			gpConEmu->OnSize();
		}
		else
		{
			int nIdx = gStatusCols[nCmd-1].nID;
			gpSet->isStatusColumnHidden[nIdx] = !gpSet->isStatusColumnHidden[nIdx];
			
			UpdateStatusBar(true);
		}

		if (ghOpWnd && IsWindow(ghOpWnd))
		{
			_ASSERTE(L"Refresh status page!");
		}
		else
		{
			gpSet->mb_StatusSettingsWasChanged = true;
		}
	}

	// Done
	DestroyMenu(hPopup);
	mb_InPopupMenu = false;
}

void CStatus::OnDataChanged(CEStatusItems *ChangedID, size_t Count, bool abForceOnChange /*= false*/)
{
	//if (!mb_Invalidated && IsKeyboardChanged())
	//{
	//	// Изменения в клавиатуре - отрисовать сразу
	//	UpdateStatusBar(true);
	//}

	for (size_t k = 0; k < Count; k++)
	{
		for (size_t i = 0; i < countof(m_Items); i++)
		{
			if ((m_Items[i].nID == ChangedID[k]) && m_Items[i].bShow)
			{
				if (lstrcmp(m_Items[i].sText, m_Values[ChangedID[k]].sText) != 0)
				{
					UpdateStatusBar(abForceOnChange);
					return;
				}
			}
		}
	}
}

void CStatus::OnWindowReposition(const RECT *prcNew)
{
	if (!gpSet->isStatusBarShow)
		return;

	RECT rcTmp = {};
	if (prcNew)
	{
		rcTmp = *prcNew;
	}
	else
	{
		if (gpConEmu->isIconic())
			return; // все равно ничего не видно

		GetWindowRect(ghWnd, &rcTmp);
	}

	// csi_WindowPos
	_wsprintf(m_Values[csi_WindowPos].sText, SKIPLEN(countof(m_Values[csi_WindowPos].sText)-1)
		L"(%i,%i)-(%i,%i)", rcTmp.left, rcTmp.top, rcTmp.right, rcTmp.bottom);
	wcscpy_c(m_Values[csi_WindowPos].szFormat, m_Values[csi_WindowPos].sText /*L"(999,999)-(9999,9999)"*/);

	// csi_WindowSize:
	_wsprintf(m_Values[csi_WindowSize].sText, SKIPLEN(countof(m_Values[csi_WindowSize].sText)-1)
		L"%ix%i", (rcTmp.right-rcTmp.left), (rcTmp.bottom-rcTmp.top));
	wcscpy_c(m_Values[csi_WindowSize].szFormat, m_Values[csi_WindowSize].sText/*L"9999x9999"*/);

	// csi_WindowClient
	RECT rcClient = gpConEmu->CalcRect(CER_MAINCLIENT, rcTmp, CER_MAIN);
	_wsprintf(m_Values[csi_WindowClient].sText, SKIPLEN(countof(m_Values[csi_WindowClient].sText)-1)
		L"%ix%i", (rcClient.right-rcClient.left), (rcClient.bottom-rcClient.top));
	wcscpy_c(m_Values[csi_WindowClient].szFormat, m_Values[csi_WindowClient].sText/*L"9999x9999"*/);

	// csi_WindowClient
	RECT rcWork = gpConEmu->CalcRect(CER_WORKSPACE, rcTmp, CER_MAIN);
	_wsprintf(m_Values[csi_WindowWork].sText, SKIPLEN(countof(m_Values[csi_WindowWork].sText)-1)
		L"%ix%i", (rcWork.right-rcWork.left), (rcWork.bottom-rcWork.top));
	wcscpy_c(m_Values[csi_WindowWork].szFormat, m_Values[csi_WindowWork].sText/*L"9999x9999"*/);

	CEStatusItems Changed[] = {csi_WindowPos, csi_WindowSize, csi_WindowClient, csi_WindowWork};
	// -- обновляем сразу, иначе получаются странные эффекты "отставания" статуса от размера окна...
	OnDataChanged(Changed, countof(Changed), true);
}

// bForceUpdate надо ставить в true, после изменения размеров консоли! Чтобы при ресайзе
// актуальные данные показывать. Другие значения (скролл и курсор) можно и с задержкой
// отображать, чтобы лишнюю нагрузку не создавать.
void CStatus::OnConsoleChanged(const CONSOLE_SCREEN_BUFFER_INFO* psbi, const CONSOLE_CURSOR_INFO* pci, bool bForceUpdate)
{
	bool bValid = (psbi != NULL);

	// csi_ConsolePos:
	if (bValid)
	{
		_wsprintf(m_Values[csi_ConsolePos].sText, SKIPLEN(countof(m_Values[csi_ConsolePos].sText)-1) L"(%i,%i)-(%i,%i)",
			(int)psbi->srWindow.Left, (int)psbi->srWindow.Top, (int)psbi->srWindow.Right, (int)psbi->srWindow.Bottom);
		_wsprintf(m_Values[csi_ConsolePos].szFormat, SKIPLEN(countof(m_Values[csi_ConsolePos].szFormat)) L" (%i,%i)-(%i,%i) ",
			(int)psbi->srWindow.Left, (int)psbi->srWindow.Top, (int)psbi->dwSize.X, (int)psbi->dwSize.Y);
	}
	else
	{
		wcscpy_c(m_Values[csi_ConsolePos].sText, L" ");
		wcscpy_c(m_Values[csi_ConsolePos].szFormat, L"(0,0)-(199,199)"); // на самом деле может быть до 9999, но для уменьшения ширины - по умолчанию так
	}

	// csi_ConsoleSize:
	if (bValid)
		_wsprintf(m_Values[csi_ConsoleSize].sText, SKIPLEN(countof(m_Values[csi_ConsoleSize].sText)-1) L"%ix%i", (int)psbi->srWindow.Right-psbi->srWindow.Left+1, (int)psbi->srWindow.Bottom-psbi->srWindow.Top+1);
	else
		wcscpy_c(m_Values[csi_ConsoleSize].sText, L" ");
	//m_Values[csi_ConsoleSize].sFormat = L"999x999";
	//_wsprintf(m_Values[csi_ConsoleSize].szFormat, SKIPLEN(countof(m_Values[csi_ConsoleSize].szFormat)) L" %ix%i",
	//	(int)psbi->dwSize.X, max((int)psbi->dwSize.Y,(int)gpSet->DefaultBufferHeight));
	wcscpy_c(m_Values[csi_ConsoleSize].szFormat, m_Values[csi_ConsoleSize].sText);

	// csi_BufferSize:
	if (bValid)
	{
		_wsprintf(m_Values[csi_BufferSize].sText, SKIPLEN(countof(m_Values[csi_BufferSize].sText)-1) L"%ix%i", (int)psbi->dwSize.X, (int)psbi->dwSize.Y);
		_wsprintf(m_Values[csi_BufferSize].szFormat, SKIPLEN(countof(m_Values[csi_BufferSize].szFormat)) L" %ix%i",
			(int)psbi->dwSize.X, max((int)psbi->dwSize.Y,(int)gpSet->DefaultBufferHeight));
	}
	else
	{
		wcscpy_c(m_Values[csi_BufferSize].sText, L" ");
		//m_Values[csi_BufferSize].sFormat = L"999x9999"; // на самом деле может быть до 9999, но для уменьшения ширины - по умолчанию так
		wcscpy_c(m_Values[csi_BufferSize].szFormat, m_Values[csi_BufferSize].sText);
	}

	if (psbi && pci)
	{
		OnCursorChanged(&psbi->dwCursorPosition, pci, psbi->dwSize.X, psbi->dwSize.Y);
	}

	CEStatusItems Changed[] = {csi_ConsolePos, csi_ConsoleSize, csi_BufferSize};
	OnDataChanged(Changed, countof(Changed), bForceUpdate);
}

void CStatus::OnCursorChanged(const COORD* pcr, const CONSOLE_CURSOR_INFO* pci, int nMaxX /*= 0*/, int nMaxY /*= 0*/)
{
	bool bValid = (pcr != NULL) && (pci != NULL);

	// csi_CursorX:
	if (bValid)
	{
		_wsprintf(m_Values[csi_CursorX].sText, SKIPLEN(countof(m_Values[csi_CursorX].sText)-1) _T("%i"), (int)pcr->X);
		if (nMaxX)
			_wsprintf(m_Values[csi_CursorX].szFormat, SKIPLEN(countof(m_Values[csi_CursorX].szFormat)) L" %i", nMaxX);
	}
	else
	{
		wcscpy_c(m_Values[csi_CursorX].sText, L" ");
		wcscpy_c(m_Values[csi_CursorX].szFormat, L"99");
	}

	// csi_CursorY:
	if (bValid)
	{
		_wsprintf(m_Values[csi_CursorY].sText, SKIPLEN(countof(m_Values[csi_CursorY].sText)-1) _T("%i"), (int)pcr->Y);
		if (nMaxY)
			_wsprintf(m_Values[csi_CursorY].szFormat, SKIPLEN(countof(m_Values[csi_CursorY].szFormat)) L" %i", nMaxY);
	}
	else
	{
		wcscpy_c(m_Values[csi_CursorY].sText, L" ");
		wcscpy_c(m_Values[csi_CursorY].szFormat, L"99");
	}

	// csi_CursorSize:
	if (bValid)
	{
		_wsprintf(m_Values[csi_CursorSize].sText, SKIPLEN(countof(m_Values[csi_CursorSize].sText)-1) _T("%i%s"), pci->dwSize, pci->bVisible ? L"V" : L"H");
		wcscpy_c(m_Values[csi_CursorSize].szFormat, L"100V");
	}
	else
	{
		wcscpy_c(m_Values[csi_CursorSize].sText, L" ");
		wcscpy_c(m_Values[csi_CursorSize].szFormat, L"99x199,99V");
	}

	// csi_CursorInfo:
	if (bValid)
	{
		_wsprintf(m_Values[csi_CursorInfo].sText, SKIPLEN(countof(m_Values[csi_CursorInfo].sText)-1) _T("%ix%i,%i%s"), (int)pcr->X, (int)pcr->Y, pci->dwSize, pci->bVisible ? L"V" : L"H");
		if (nMaxX && nMaxY)
			_wsprintf(m_Values[csi_CursorInfo].szFormat, SKIPLEN(countof(m_Values[csi_CursorInfo].szFormat)) L" %ix%i,100V", nMaxX, nMaxY);
	}
	else
	{
		wcscpy_c(m_Values[csi_CursorInfo].sText, L" ");
		wcscpy_c(m_Values[csi_CursorInfo].szFormat, L"99x199,99V");
	}

	CEStatusItems Changed[] = {csi_CursorX, csi_CursorY, csi_CursorSize, csi_CursorInfo};
	OnDataChanged(Changed, countof(Changed));
}

void CStatus::OnConsoleBufferChanged(CRealConsole* pRCon)
{
	RealBufferType tBuffer = pRCon ? pRCon->GetActiveBufferType() : rbt_Undefined;
	TODO("isBufferWidth");
	bool bIsScroll = pRCon ? pRCon->isBufferHeight() : false;
	wchar_t cScroll = (gnOsVer>=0x0601/*Windows7*/) ? 0x2195 : L']'/*WinXP has not arrows*/; // Up/Down arrow

	wcscpy_c(m_Values[csi_ActiveBuffer].sText, 
			(tBuffer == rbt_Primary) ? L"PRI" :
			(tBuffer == rbt_Alternative) ? L"ALT" :
			(tBuffer == rbt_Selection) ? L"SEL" :
			(tBuffer == rbt_Find) ? L"FND" :
			(tBuffer == rbt_DumpScreen) ? L"DMP" :
			L" ");
	if (bIsScroll)
	{
		m_Values[csi_ActiveBuffer].sText[4] = 0;
		m_Values[csi_ActiveBuffer].sText[3] = cScroll;
	}

	wcscpy_c(m_Values[csi_ActiveBuffer].szFormat, L"DUMP");

	CEStatusItems Changed[] = {csi_ActiveBuffer};
	// -- обновляем сразу, иначе получаются странные эффекты "отставания" статуса от окна...
	OnDataChanged(Changed, countof(Changed), true);
}

void CStatus::OnActiveVConChanged(int nIndex/*0-based*/, CRealConsole* pRCon)
{
	//int nActive = gpConEmu->GetActiveVCon();
	int nCount = gpConEmu->GetConCount();

	if ((nCount > 0) && (nIndex >= 0))
	{
		_wsprintf(m_Values[csi_ActiveVCon].sText, SKIPLEN(countof(m_Items[csi_ActiveVCon].sText)) L"%i/%i",
			nIndex+1, nCount);
	}
	else
	{
		wcscpy_c(m_Values[csi_ActiveVCon].sText, L" ");
	}
	wcscpy_c(m_Values[csi_ActiveVCon].szFormat, L"99/99");

	//CEStatusItems Changed[] = {csi_ActiveVCon};
	//OnDataChanged(Changed, countof(Changed));

	OnConsoleBufferChanged(pRCon);

	if (pRCon)
	{
		DWORD nMainServerPID = pRCon->isServerCreated() ? pRCon->GetServerPID(true) : 0;
		DWORD nAltServerPID = nMainServerPID ? pRCon->GetServerPID(false) : 0;
		OnServerChanged(nMainServerPID, nAltServerPID);

		CONSOLE_SCREEN_BUFFER_INFO sbi = {};
		CONSOLE_CURSOR_INFO ci = {};
		pRCon->GetConsoleScreenBufferInfo(&sbi);
		pRCon->GetConsoleCursorInfo(&ci);
		OnConsoleChanged(&sbi, &ci, false);
	}
	else
	{
		OnServerChanged(0, 0);
		OnConsoleChanged(NULL, NULL, false);
	}

	// Чтобы уж точно обновилось - как минимум, меняется PID активного процесса
	UpdateStatusBar(true);
}

void CStatus::OnServerChanged(DWORD nMainServerPID, DWORD nAltServerPID)
{
	if (nMainServerPID)
	{
		if ((nMainServerPID == nAltServerPID) || !nAltServerPID)
		{
			_wsprintf(m_Values[csi_Server].sText, SKIPLEN(countof(m_Items[csi_Server].sText)) _T("%u"), nMainServerPID);
			wcscpy_c(m_Values[csi_Server].szFormat, L"99999");
		}
		else
		{
			_wsprintf(m_Values[csi_Server].sText, SKIPLEN(countof(m_Values[csi_Server].sText)) _T("%u/%u"), nMainServerPID, nAltServerPID);
			wcscpy_c(m_Values[csi_Server].szFormat, L"99999/99999");
		}
	}
	else
	{
		wcscpy_c(m_Values[csi_Server].sText, L" ");
		wcscpy_c(m_Values[csi_Server].szFormat, L"99999");
	}

	CEStatusItems Changed[] = {csi_Server};
	OnDataChanged(Changed, countof(Changed));
}

LPCWSTR CStatus::GetSettingName(CEStatusItems nID)
{
	_ASSERTE(this);
	if (nID <= csi_Info || nID >= csi_Last)
		return NULL;

	_ASSERTE(m_Values[nID].sSettingName!=NULL && m_Values[nID].sSettingName[0]!=0);
	return m_Values[nID].sSettingName;
}

bool CStatus::IsKeyboardChanged()
{
	if (gpSet->isStatusColumnHidden[csi_CapsLock]
		&& gpSet->isStatusColumnHidden[csi_NumLock]
		&& gpSet->isStatusColumnHidden[csi_ScrollLock]
		&& gpSet->isStatusColumnHidden[csi_InputLocale])
	{
		// Если ни одна из клавиатурных "кнопок" не показана - то и делать нечего
		return false;
	}

	bool bChanged = false;

	bool bCaps = (GetKeyState(VK_CAPITAL) & 1) == 1;
	bool bNum = (GetKeyState(VK_NUMLOCK) & 1) == 1;
	bool bScroll = (GetKeyState(VK_SCROLL) & 1) == 1;
	DWORD_PTR hkl = gpConEmu->GetActiveKeyboardLayout();

	if (bCaps != mb_Caps)
	{
		mb_Caps = bCaps; bChanged = true;
	}

	if (bNum != mb_Num)
	{
		mb_Num = bNum; bChanged = true;
	}

	if (bScroll != mb_Scroll)
	{
		mb_Scroll = bScroll; bChanged = true;
	}

	if (hkl != mhk_Locale)
	{
		mhk_Locale = hkl; bChanged = true;
	}

	return bChanged;
}

void CStatus::OnKeyboardChanged()
{
	if (/*!mb_Invalidated &&*/ IsKeyboardChanged())
	{
		// Изменения в клавиатуре - отрисовать сразу
		UpdateStatusBar(true);
	}
}

void CStatus::OnTransparency()
{
	_wsprintf(m_Values[csi_Transparency].sText, SKIPLEN(countof(m_Items[csi_Transparency].sText))
		_T("%u%%%s%s"), (UINT)(gpSet->nTransparent >= 255) ? 100 : (gpSet->nTransparent * 100 / 255),
		gpSet->isUserScreenTransparent ? L",USR" : L"",
		L""); // TODO: ColorKey transparency
	wcscpy_c(m_Values[csi_Transparency].szFormat, L"100%");

	CEStatusItems Changed[] = {csi_Transparency};
	OnDataChanged(Changed, countof(Changed), true);
}

void CStatus::ProcessMenuHighlight(HMENU hMenu, WORD nID, WORD nFlags)
{
	if (mb_InSetupMenu)
	{
		if ((nID > 0) && (nID <= countof(gStatusCols)))
			SetClickedItemDesc(gStatusCols[nID-1].nID);
		else
			SetClickedItemDesc(csi_Last);
	}
	else switch (m_ClickedItemDesc)
	{
		case csi_Transparency:
			{
				if (ProcessTransparentMenuId(nID, true))
					gpConEmu->OnTransparent();
			}
			break;
	}
}

void CStatus::ShowVConMenu(POINT pt)
{
	mb_InPopupMenu = true;

	_ASSERTE(m_ClickedItemDesc == csi_ActiveVCon);
	m_ClickedItemDesc = csi_ActiveVCon;

	gpConEmu->ChooseTabFromMenu(FALSE, pt, TPM_LEFTALIGN|TPM_BOTTOMALIGN);

	mb_InPopupMenu = false;
}

bool CStatus::ProcessTransparentMenuId(WORD nCmd, bool abAlphaOnly)
{
	bool bSelected = false;

	if (nCmd >= 1)
	{
		for (int i = 0; i < countof(gTranspOpt); i++)
		{
			if (gTranspOpt[i].nMenuID == nCmd)
			{
				// Change TEMPORARILY, without saving settings
				if (gTranspOpt[i].nValue >= 40)
				{
					if ((gTranspOpt[i].nValue < 100) || !abAlphaOnly)
					{
						gpSet->nTransparent = min(255,((gTranspOpt[i].nValue*255/100)+1));
						bSelected = true;
					}
				}
				else if (!abAlphaOnly)
				{
					switch (gTranspOpt[i].nValue)
					{
						case 1:
							gpSet->isUserScreenTransparent = !gpSet->isUserScreenTransparent;
							gpConEmu->OnHideCaption(); // при прозрачности - обязательно скрытие заголовка + кнопки
							gpConEmu->UpdateWindowRgn();
							// Отразить изменения в статусе
							OnTransparency();
							break;
						case 2:
							gpSet->isColorKeyTransparent = !gpSet->isColorKeyTransparent;
							bSelected = true;
							break;
					}
				}
				break;
			}
		}
	}

	if (bSelected)
	{
		// Отразить изменения в статусе
		OnTransparency();
	}

	return bSelected;
}

bool CStatus::isSettingsOpened(UINT nOpenPageID)
{
	if (ghOpWnd && IsWindow(ghOpWnd))
	{
		gpSetCls->Dialog(nOpenPageID);
		if (!nOpenPageID)
			MessageBox(ghOpWnd, L"Close settings dialog first, please.", gpConEmu->GetDefaultTitle(), MB_ICONEXCLAMATION);
		return true;
	}

	return false;
}

void CStatus::ShowTransparencyMenu(POINT pt)
{
	if (isSettingsOpened(IDD_SPG_COLORS))
		return;

	mb_InPopupMenu = true;
	HMENU hPopup = CreatePopupMenu();

	// меню было расчитано на min=40
	_ASSERTE(MIN_ALPHA_VALUE==40);

	int nCurrent = -1;

	for (int i = 0; i < countof(gTranspOpt); i++)
	{
		bool bChecked = false;
		if (!gTranspOpt[i].nMenuID)
		{
			AppendMenu(hPopup, MF_SEPARATOR, 0, L"");
		}
		else
		{
			if (gTranspOpt[i].nValue == 1)
				bChecked = gpSet->isUserScreenTransparent;
			else if (gTranspOpt[i].nValue == 2)
				bChecked = gpSet->isColorKeyTransparent;
			else
			{
				if (gTranspOpt[i].nValue == 100)
				{
					if (gpSet->nTransparent == 255)
						nCurrent = i;
				}
				else
				{
					_ASSERTE((i+1) < countof(gTranspOpt));
					UINT nVal = gTranspOpt[i].nValue * 255 / 100;
					UINT nNextVal = ((gTranspOpt[i+1].nValue<10)?255:(gTranspOpt[i+1].nValue * 255 / 100));
					if ((nVal <= gpSet->nTransparent) && (gpSet->nTransparent < nNextVal))
					{
						nCurrent = gTranspOpt[i].nMenuID;
					}
				}
			}

			AppendMenu(hPopup, MF_STRING|(bChecked ? MF_CHECKED : MF_UNCHECKED), gTranspOpt[i].nMenuID, gTranspOpt[i].sText);
		}
	}

	if (nCurrent != -1)
	{
		MENUITEMINFO mi = {sizeof(mi), MIIM_STATE|MIIM_ID};
		mi.wID = nCurrent;
		GetMenuItemInfo(hPopup, nCurrent, FALSE, &mi);
		mi.fState |= MF_DEFAULT;
		SetMenuItemInfo(hPopup, nCurrent, FALSE, &mi);
	}

	u8 nPrevAlpha = gpSet->nTransparent;
	//bool bPrevUserScreen = gpSet->isUserScreenTransparent; -- он идет через WindowRgn, а не Transparency
	bool bPrevColorKey = gpSet->isColorKeyTransparent;

	_ASSERTE(m_ClickedItemDesc == csi_Transparency);
	m_ClickedItemDesc = csi_Transparency;

	int nCmd = gpConEmu->trackPopupMenu(tmp_StatusBarCols, hPopup, TPM_BOTTOMALIGN|TPM_RIGHTALIGN|TPM_RETURNCMD, pt.x, pt.y, 0, ghWnd, NULL);

	bool bSelected = ProcessTransparentMenuId(nCmd, false);

	if (!bSelected)
	{
		// Если поменяли альфу при Hover мышкой...
		if ((nPrevAlpha != gpSet->nTransparent)
			//|| (bPrevUserScreen != gpSet->isUserScreenTransparent)
			|| (bPrevColorKey != gpSet->isColorKeyTransparent))
		{
			gpSet->nTransparent = nPrevAlpha;
			//gpSet->isUserScreenTransparent = bPrevUserScreen;
			gpSet->isColorKeyTransparent = bPrevColorKey;
			bSelected = true;
		}
	}

	if (bSelected)
	{
		gpConEmu->OnTransparent();
	}

	// Done
	DestroyMenu(hPopup);
	mb_InPopupMenu = false;

	// Отразить изменения в статусе
	OnTransparency();
}

void CStatus::UpdateStatusFont()
{
	if (!gpSet->isStatusBarShow)
		return;

	UpdateStatusBar(true);
	gpConEmu->OnSize();
}

// Прямоугольник в клиентских координатах ghWnd!
bool CStatus::GetStatusBarClientRect(RECT* rc)
{
	if (!gpSet->isStatusBarShow)
		return false;

	RECT rcClient = gpConEmu->GetGuiClientRect();

	int nHeight = gpSet->StatusBarHeight();
	if (nHeight >= (rcClient.bottom - rcClient.top))
		return false;

	rcClient.top = rcClient.bottom - 1 - nHeight;

	*rc = rcClient;
	return true;
}
