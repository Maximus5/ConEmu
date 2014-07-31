
/*
Copyright (c) 2014 Maximus5
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

#include <windows.h>
#include "header.h"
#include "ToolImg.h"

#ifdef _DEBUG
#include "LoadImg.h"
#endif

CToolImg::CToolImg()
{
	mh_Bmp = NULL;
	mh_BmpDc = NULL;
	mh_OldBmp = NULL;
	mn_BtnWidth = 14; mn_BtnHeight = 14;
	mcr_Background = GetSysColor(COLOR_BTNFACE);
	mn_BtnCount = 0;
	mn_MaxBtnCount = 0;
	mprc_Btns = NULL;
}

CToolImg::~CToolImg()
{
	FreeDC();
	FreeBMP();
}

HBITMAP CToolImg::GetBitmap()
{
	// Free memory allocated for temp DC
	FreeDC();
	// Bitmap is ready?
	return mh_Bmp;
}

void CToolImg::FreeDC()
{
	if (mh_BmpDc)
	{
		SelectObject(mh_BmpDc, mh_OldBmp);
		mh_OldBmp = NULL;
		DeleteDC(mh_BmpDc);
		mh_BmpDc = NULL;
	}

	SafeFree(mprc_Btns);
}

void CToolImg::FreeBMP()
{
	if (mh_Bmp)
	{
		DeleteObject(mh_Bmp);
		mh_Bmp = NULL;
	}

	mn_BtnCount = 0;
}

bool CToolImg::Create(int nBtnWidth, int nBtnHeight, int nMaxCount, COLORREF clrBackground)
{
	// Ensure there is no old handles
	FreeDC();
	FreeBMP();

	if (nBtnWidth <= 0 || nBtnHeight <= 0 || nMaxCount <= 0)
		return false;

	// Init vars
	mn_BtnWidth = nBtnHeight; mn_BtnHeight = nBtnHeight;
	mn_BtnCount = 0; mn_MaxBtnCount = nMaxCount;
	mcr_Background = clrBackground;

	return CreateField(nBtnWidth*nMaxCount, nBtnHeight, clrBackground);
}

bool CToolImg::CreateField(int nImgWidth, int nImgHeight, COLORREF clrBackground)
{
	// Create memory DC
	mh_BmpDc = CreateCompatibleDC(NULL);
	if (!mh_BmpDc)
	{
		return false;
	}

	// Create memory bitmap (WinXP and lower toolbar has some problem with true-color buttons, so - 8bit)
	mh_Bmp = CreateBitmap(nImgWidth, nImgHeight, 1, (gnOsVer >= 0x600) ? 32 : 8, NULL);
	if (!mh_Bmp)
	{
		FreeDC();
		return false;
	}
	mh_OldBmp = (HBITMAP)SelectObject(mh_BmpDc, mh_Bmp);

	// Prefill with background
	RECT rcFill = {0, 0, nImgWidth, nImgHeight};
	HBRUSH hbr = CreateSolidBrush(clrBackground);
	FillRect(mh_BmpDc, &rcFill, hbr);
	DeleteObject(hbr);

	return true;
}

bool CToolImg::CreateDonateButton(COLORREF clrBackground, int& nDefWidth, int& nDefHeight)
{
	ButtonFieldInfo Btns[] = {
		{L"DONATE21", 74, 21},
		{L"DONATE26", 92, 26},
		{L"DONATE42", 148,42},
	};

	return CreateButtonField(clrBackground, Btns, (int)countof(Btns), nDefWidth, nDefHeight);
}

bool CToolImg::CreateFlattrButton(COLORREF clrBackground, int& nDefWidth, int& nDefHeight)
{
	ButtonFieldInfo Btns[] = {
		{L"FLATTR", 89, 18},
	};

	return CreateButtonField(clrBackground, Btns, (int)countof(Btns), nDefWidth, nDefHeight);
}

bool CToolImg::CreateButtonField(COLORREF clrBackground, ButtonFieldInfo* pBtns, int nBtnCount, int& nDefWidth, int& nDefHeight)
{
	FreeDC();
	FreeBMP();

	nDefWidth = pBtns[0].nWidth;
	nDefHeight = pBtns[0].nHeight;

	mprc_Btns = (LPRECT)calloc(nBtnCount, sizeof(*mprc_Btns));
	if (!mprc_Btns)
		return false;
	mn_MaxBtnCount = nBtnCount;

	int nFieldWidth = pBtns[0].nWidth;
	int nFieldHeight = pBtns[0].nHeight;
	for (int i = 1; i < nBtnCount; i++)
	{
		nFieldWidth = max(nFieldWidth, pBtns[i].nWidth);
		nFieldHeight += 1 + pBtns[i].nHeight;
	}

	if (!CreateField(nFieldWidth, nFieldHeight, clrBackground))
		return false;

	COLORMAP colorMap = {0xC0C0C0, clrBackground/*GetSysColor(COLOR_BTNFACE)*/};

	bool bRc = true;

	_ASSERTE(mn_BtnCount == 0);

	int nDstY = 0;
	DWORD nErrCode = 0;

	for (int i = 0; i < nBtnCount; i++)
	{
		//HBITMAP hbm = CreateMappedBitmap(g_hInstance, (INT_PTR)pBtns[i].szName, 0, &colorMap, 1);
		HBITMAP hbm = (HBITMAP)LoadImage(g_hInstance, pBtns[i].szName, IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT|LR_LOADMAP3DCOLORS);
		if (hbm == NULL)
		{
			nErrCode = GetLastError();
			bRc = false;
			continue;
		}
		else
		{
			RECT rc = {0, nDstY, pBtns[i].nWidth, nDstY+pBtns[i].nHeight};
			mprc_Btns[i] = rc;

			if (!PaintBitmap(hbm, pBtns[i].nWidth, pBtns[i].nHeight, mh_BmpDc, 0, nDstY, pBtns[i].nWidth, pBtns[i].nHeight))
			{
				nErrCode = GetLastError();
				bRc = false;
			}

			DeleteObject(hbm);
		}

		nDstY += 1 + pBtns[i].nHeight;
	}

	#ifdef _DEBUG
	//SaveImageEx(L"T:\\BtnField.png", mh_Bmp);
	#endif

	mn_BtnCount = mn_MaxBtnCount;

	return true;
}

bool CToolImg::PaintButton(int iBtn, HDC hdcDst, int nDstX, int nDstY, int nDstWidth, int nDstHeight)
{
	if (!mh_BmpDc || !mh_Bmp || !mprc_Btns || (iBtn >= mn_BtnCount) || (mn_BtnCount <= 0))
		return false;

	if (iBtn < 0)
	{
		int iFoundHeight = 0;
		iBtn = mn_BtnCount - 1;

		for (int i = 0; i < mn_BtnCount; i++)
		{
			int h = (mprc_Btns[i].bottom - mprc_Btns[i].top);
			if ((h <= nDstHeight) && (h > iFoundHeight))
			{
				iBtn = i;
				iFoundHeight = h;
			}
			// Continue enum, may be better choice will be found
		}
	}

	RECT rc = mprc_Btns[iBtn];

	int y = nDstY;
	int w = min(nDstWidth, (rc.right - rc.left));
	int h = nDstHeight;

	if (nDstHeight > (rc.bottom - rc.top))
	{
		h = rc.bottom - rc.top;
		y += ((nDstHeight - h) >> 1);
	}

	if (!BitBlt(hdcDst, nDstX, y, w, h, mh_BmpDc, rc.left, rc.top, SRCCOPY))
		return false;

	return true;
}

int CToolImg::AddBitmap(HBITMAP hbm, int iNumBtns)
{
	BITMAP bm = {};
	int ccb = hbm ? GetObject(hbm, sizeof(bm), &bm) : -1;
	if ((ccb <= 0) || (bm.bmWidth <= 0) || (bm.bmHeight == 0))
	{
		_ASSERTE((ccb > 0) && "Invalid bitmap loaded?");
		return 0;
	}

	int iSrcBtnWidth = bm.bmWidth / iNumBtns;
	int iAdded = iNumBtns;
	if (iAdded > (mn_MaxBtnCount - mn_BtnCount))
	{
		iAdded = (mn_MaxBtnCount - mn_BtnCount);
		if (iAdded < 1)
		{
			_ASSERTE((mn_MaxBtnCount > mn_BtnCount) && "Button count overflow");
			return 0;
		}
	}
	int iSrcHeight = _abs(bm.bmHeight);
	int iSrcWidth = iAdded * iSrcBtnWidth;

	// Where we must paint to
	int x = mn_BtnCount * mn_BtnWidth;

	// And go
	bool blitRc = PaintBitmap(hbm, iSrcWidth, iSrcHeight, mh_BmpDc, x, 0, iAdded*mn_BtnWidth, mn_BtnHeight);

	// Check result
	if (blitRc)
		mn_BtnCount += iAdded;
	else
		iAdded = 0;

	_ASSERTE(mn_BtnCount <= mn_MaxBtnCount);

	// Done
	return iAdded;
}

bool CToolImg::PaintBitmap(HBITMAP hbmSrc, int nSrcWidth, int nSrcHeight, HDC hdcDst, int nDstX, int nDstY, int nDstWidth, int nDstHeight)
{
	HDC hdc = CreateCompatibleDC(NULL);
	if (!hdc)
		return false;
	HBITMAP hOld = (HBITMAP)SelectObject(hdc, hbmSrc);

	// And go
	BOOL blitRc;
	if (nSrcHeight != nDstHeight)
	{
		// Need stretch
		blitRc = StretchBlt(hdcDst, nDstX, nDstY, nDstWidth, nDstHeight, hdc, 0, 0, nSrcWidth, nSrcHeight, SRCCOPY);
	}
	else
	{
		// Just blit
		blitRc = BitBlt(hdcDst, nDstX, nDstY, nDstWidth, nDstHeight, hdc, 0, 0, SRCCOPY);
	}

	SelectObject(hdc, hOld);
	DeleteDC(hdc);

	return (blitRc != FALSE);
}

int CToolImg::AddButtons(HINSTANCE hinst, INT_PTR resId, int iNumBtns)
{
	int iAdded = 0;
	HBITMAP hbm = (HBITMAP)LoadImage(hinst, (LPCWSTR)resId, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	if (hbm)
	{
		iAdded = AddBitmap(hbm, iNumBtns);
		DeleteObject(hbm);
	}
	else
	{
		_ASSERTE(FALSE && "LoadImage failed");
	}
	return iAdded;
}

int CToolImg::AddButtonsMapped(HINSTANCE hinst, INT_PTR resId, int iNumBtns, int iNumMaps, COLORREF from, COLORREF to, ...)
{
	int iAdded = 0;
	_ASSERTE(iNumMaps <= 1);
	COLORMAP colorMap = {from, to};
	HBITMAP hbm = CreateMappedBitmap(hinst, resId, 0, &colorMap, 1);
	if (hbm)
	{
		iAdded = AddBitmap(hbm, iNumBtns);
		DeleteObject(hbm);
	}
	else
	{
		_ASSERTE(FALSE && "CreateMappedBitmap failed");
	}
	return iAdded;
}
