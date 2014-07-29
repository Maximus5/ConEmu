
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

CToolImg::CToolImg()
{
	mh_Bmp = NULL;
	mh_BmpDc = NULL;
	mh_OldBmp = NULL;
	mn_BtnWidth = 14; mn_BtnHeight = 14;
	mcr_Background = GetSysColor(COLOR_BTNFACE);
	mn_BtnCount = 0;
	mn_MaxBtnCount = 0;
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

	_ASSERTE(nBtnWidth == nBtnHeight);

	// Init vars
	mn_BtnWidth = nBtnHeight; mn_BtnHeight = nBtnHeight;
	mn_BtnCount = 0; mn_MaxBtnCount = nMaxCount;
	mcr_Background = clrBackground;

	// Create memory DC
	mh_BmpDc = CreateCompatibleDC(NULL);
	if (!mh_BmpDc)
	{
		return false;
	}

	// Create memory bitmap (WinXP and lower toolbar has some problem with true-color buttons, so - 8bit)
	mh_Bmp = CreateBitmap(nBtnWidth*nMaxCount, nBtnHeight, 1, (gnOsVer >= 0x600) ? 32 : 8, NULL);
	if (!mh_Bmp)
	{
		FreeDC();
		return false;
	}
	mh_OldBmp = (HBITMAP)SelectObject(mh_BmpDc, mh_Bmp);

	// Prefill with background
	RECT rcFill = {0, 0, nBtnWidth*nMaxCount, nBtnHeight};
	HBRUSH hbr = CreateSolidBrush(clrBackground);
	FillRect(mh_BmpDc, &rcFill, hbr);
	DeleteObject(hbr);

	return true;
}

int CToolImg::AddBitmap(HBITMAP hbm)
{
	BITMAP bm = {};
	int ccb = hbm ? GetObject(hbm, sizeof(bm), &bm) : -1;
	if ((ccb <= 0) || (bm.bmWidth <= 0) || (bm.bmHeight == 0))
	{
		_ASSERTE((ccb > 0) && "Invalid bitmap loaded?");
		return 0;
	}

	int iAdded = bm.bmWidth / _abs(bm.bmHeight);
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
	int iSrcWidth = iAdded * iSrcHeight;

	HDC hdc = CreateCompatibleDC(NULL);
	if (!hdc)
		return 0;
	HBITMAP hOld = (HBITMAP)SelectObject(hdc, hbm);

	// Where we must paint to
	int x = mn_BtnCount * mn_BtnWidth;

	// And go
	BOOL blitRc;
	if (bm.bmHeight != mn_BtnHeight)
	{
		// Need stretch
		blitRc = StretchBlt(mh_BmpDc, x, 0, iAdded*mn_BtnWidth, mn_BtnHeight, hdc, 0, 0, iSrcWidth, iSrcHeight, SRCCOPY);
	}
	else
	{
		// Just blit
		blitRc = BitBlt(mh_BmpDc, x, 0, iAdded*mn_BtnWidth, mn_BtnHeight, hdc, 0, 0, SRCCOPY);
	}

	// Check result
	if (blitRc)
		mn_BtnCount += iAdded;
	else
		iAdded = 0;

	_ASSERTE(mn_BtnCount <= mn_MaxBtnCount);

	// Done
	SelectObject(hdc, hOld);
	DeleteDC(hdc);
	return iAdded;
}

int CToolImg::AddButtons(HINSTANCE hinst, INT_PTR resId)
{
	int iAdded = 0;
	HBITMAP hbm = (HBITMAP)LoadImage(hinst, (LPCWSTR)resId, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	if (hbm)
	{
		iAdded = AddBitmap(hbm);
		DeleteObject(hbm);
	}
	else
	{
		_ASSERTE(FALSE && "LoadImage failed");
	}
	return iAdded;
}

int CToolImg::AddButtonsMapped(HINSTANCE hinst, INT_PTR resId, int iNumMaps, COLORREF from, COLORREF to, ...)
{
	int iAdded = 0;
	_ASSERTE(iNumMaps <= 1);
	COLORMAP colorMap = {from, to};
	HBITMAP hbm = CreateMappedBitmap(hinst, resId, 0, &colorMap, 1);
	if (hbm)
	{
		iAdded = AddBitmap(hbm);
		DeleteObject(hbm);
	}
	else
	{
		_ASSERTE(FALSE && "CreateMappedBitmap failed");
	}
	return iAdded;
}
