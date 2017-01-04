
/*
Copyright (c) 2014-2017 Maximus5
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

#pragma once

#include "../common/MMap.h"
#include "Options.h"

#define getR(inColorref) (byte)(inColorref)
#define getG(inColorref) (byte)((inColorref) >> 8)
#define getB(inColorref) (byte)((inColorref) >> 16)

class CSetPgBase;

class CSetDlgColors
{
public:
	static const int MAX_COLOR_EDT_ID; // c31

protected:
	static BOOL gbLastColorsOk; // FALSE
	static ColorPalette gLastColors; // {}

	static HBRUSH mh_CtlColorBrush;
	static COLORREF acrCustClr[16]; // array of custom colors, используется в ChooseColor(...)

public:
	static bool GetColorById(WORD nID, COLORREF* color);
	static bool GetColorRef(HWND hDlg, WORD TB, COLORREF* pCR);
	static bool SetColorById(WORD nID, COLORREF color);
	static void ColorSetEdit(HWND hWnd2, WORD c);
	static bool ColorEditDialog(HWND hWnd2, WORD c);
	static void FillBgImageColors(HWND hWnd2);
	static INT_PTR ColorCtlStatic(HWND hWnd2, WORD c, HWND hItem);
	static bool ShowColorDialog(HWND HWndOwner, COLORREF *inColor);
public:
	static void OnSettingsLoaded(const COLORREF (&Colors)[32]);
	static void ReleaseHandles();

protected:
	static MMap<HWND,WNDPROC> gColorBoxMap;
	static LRESULT CALLBACK ColorBoxProc(HWND hCtrl, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
