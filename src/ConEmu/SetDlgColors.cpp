
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"

#include "ConEmu.h"
#include "OptionsClass.h"
#include "SetColorPalette.h"
#include "SetDlgColors.h"
#include "Status.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

const int CSetDlgColors::MAX_COLOR_EDT_ID = c31;

BOOL CSetDlgColors::gbLastColorsOk = FALSE;
ColorPalette CSetDlgColors::gLastColors = {};

HBRUSH CSetDlgColors::mh_CtlColorBrush = NULL;
COLORREF CSetDlgColors::acrCustClr[16] = {}; // array of custom colors, используется в ChooseColor(...)
MMap<HWND, WNDPROC> CSetDlgColors::gColorBoxMap;

bool CSetDlgColors::GetColorById(WORD nID, COLORREF* color)
{
	switch (nID)
	{
	case c32:
		*color = gpSet->ThSet.crBackground.ColorRGB;
		break;
	case c33:
		*color = gpSet->ThSet.crPreviewFrame.ColorRGB;
		break;
	case c34:
		*color = gpSet->ThSet.crSelectFrame.ColorRGB;
		break;
	case c35:
		*color = gpSet->nStatusBarBack;
		break;
	case c36:
		*color = gpSet->nStatusBarLight;
		break;
	case c37:
		*color = gpSet->nStatusBarDark;
		break;
	case c38:
		*color = gpSet->nColorKeyValue;
		break;

	default:
		if (nID <= c31)
			*color = gpSet->Colors[nID - c0];
		else
			return false;
	}

	return true;
}

bool CSetDlgColors::SetColorById(WORD nID, COLORREF color)
{
	switch (nID)
	{
	case c32:
		gpSet->ThSet.crBackground.ColorRGB = color;
		break;
	case c33:
		gpSet->ThSet.crPreviewFrame.ColorRGB = color;
		break;
	case c34:
		gpSet->ThSet.crSelectFrame.ColorRGB = color;
		break;
	case c35:
		gpSet->nStatusBarBack = color;
		gpConEmu->mp_Status->UpdateStatusBar(true);
		break;
	case c36:
		gpSet->nStatusBarLight = color;
		gpConEmu->mp_Status->UpdateStatusBar(true);
		break;
	case c37:
		gpSet->nStatusBarDark = color;
		gpConEmu->mp_Status->UpdateStatusBar(true);
		break;
	case c38:
		gpSet->nColorKeyValue = color;
		gpConEmu->OnTransparent();
		break;

	default:
		if (nID <= c31)
		{
			gpSet->Colors[nID - c0] = color;
			gpSet->mb_FadeInitialized = false;
		}
		else
		{
			_ASSERTE(FALSE && "Invalid cXX was specifed");
			return false;
		}
	}

	return true;
}

LRESULT CSetDlgColors::ColorBoxProc(HWND hCtrl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc = 0;
	WNDPROC pfnOldProc = NULL;
	gColorBoxMap.Get(hCtrl, &pfnOldProc);

	UINT CB = GetWindowLong(hCtrl, GWL_ID);

	static bool bHooked = false, bClicked = false;
	static COLORREF clrPrevValue = 0;

	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
		SetCapture(hCtrl);
		GetColorById(CB, &clrPrevValue);
		bHooked = bClicked = true;
		goto wrap;
	case WM_LBUTTONUP:
		if (bHooked)
		{
			bHooked = false;
			ReleaseCapture();
		}
		if (bClicked)
		{
			CSettings::OnBtn_ColorField(hCtrl, CB, 0);
		}
		goto wrap;
	case WM_RBUTTONDOWN:
		if (bHooked)
		{
			SetColorById(CB, clrPrevValue);
			ColorSetEdit(GetParent(hCtrl), CB);
			CSettings::InvalidateCtrl(hCtrl, TRUE);
			gpConEmu->InvalidateAll();
			gpConEmu->Update(true);
		}
		goto wrap;
	case WM_RBUTTONUP:
		goto wrap;
	case WM_MOUSEMOVE:
		if (bHooked)
		{
			POINT ptCur = {}; GetCursorPos(&ptCur);
			RECT rcSelf = {}; GetWindowRect(hCtrl, &rcSelf);
			if (!PtInRect(&rcSelf, ptCur))
			{
				bClicked = false;
				HDC hdc = GetDC(NULL);
				if (hdc)
				{
					COLORREF clr = GetPixel(hdc, ptCur.x, ptCur.y);
					if (clr != CLR_INVALID)
					{
						SetColorById(CB, clr);
						ColorSetEdit(GetParent(hCtrl), CB);
						CSettings::InvalidateCtrl(hCtrl, TRUE);
						gpConEmu->InvalidateAll();
						gpConEmu->Update(true);
					}
					ReleaseDC(NULL, hdc);
				}
			}
		}
		goto wrap;
	case WM_DESTROY:
		if (bHooked)
		{
			bHooked = false;
			ReleaseCapture();
		}
		gColorBoxMap.Get(hCtrl, NULL, true);
		break;
	}

	if (pfnOldProc)
		lRc = ::CallWindowProc(pfnOldProc, hCtrl, uMsg, wParam, lParam);
	else
		lRc = ::DefWindowProc(hCtrl, uMsg, wParam, lParam);
wrap:
	return lRc;
}

void CSetDlgColors::ColorSetEdit(HWND hWnd2, WORD c)
{
	_ASSERTE(hWnd2!=NULL);
	// Hook ctrl procedure
	if (!gColorBoxMap.Initialized())
		gColorBoxMap.Init(128);
	HWND hBox = GetDlgItem(hWnd2, c);
	if (!gColorBoxMap.Get(hBox, NULL))
	{
		WNDPROC pfnOld = (WNDPROC)SetWindowLongPtr(hBox, GWLP_WNDPROC, (LONG_PTR)ColorBoxProc);
		gColorBoxMap.Set(hBox, pfnOld);
	}
	// text box ID
	WORD tc = (tc0-c0) + c;
	// Well, 11 chars are enough for "255 255 255"
	// But sometimes it is interesting to copy/paste/cut when editing palettes, so x2
	SendDlgItemMessage(hWnd2, tc, EM_SETLIMITTEXT, 23, 0);
	COLORREF cr = 0;
	GetColorById(c, &cr);
	wchar_t temp[16];
	switch (gpSetCls->m_ColorFormat)
	{
	case CSettings::eRgbHex:
		_wsprintf(temp, SKIPLEN(countof(temp)) L"#%02x%02x%02x", getR(cr), getG(cr), getB(cr));
		break;
	case CSettings::eBgrHex:
		_wsprintf(temp, SKIPLEN(countof(temp)) L"0x%02x%02x%02x", getB(cr), getG(cr), getR(cr));
		break;
	default:
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i %i %i", getR(cr), getG(cr), getB(cr));
	}
	SetDlgItemText(hWnd2, tc, temp);
}

bool CSetDlgColors::ColorEditDialog(HWND hWnd2, WORD c)
{
	_ASSERTE(hWnd2!=NULL);
	bool bChanged = false;
	COLORREF color = 0;
	GetColorById(c, &color);
	//wchar_t temp[16];
	COLORREF colornew = color;

	if (ShowColorDialog(ghOpWnd, &colornew) && colornew != color)
	{
		SetColorById(c, colornew);
		//_wsprintf(temp, SKIPLEN(countof(temp)) L"%i %i %i", getR(colornew), getG(colornew), getB(colornew));
		//SetDlgItemText(hWnd2, c + (tc0-c0), temp);
		ColorSetEdit(hWnd2, c);
		CSettings::InvalidateCtrl(GetDlgItem(hWnd2, c), TRUE);
		bChanged = true;
	}

	return bChanged;
}

void CSetDlgColors::FillBgImageColors(HWND hWnd2)
{
	TCHAR tmp[255];
	DWORD nTest = gpSet->nBgImageColors;
	wchar_t *pszTemp = tmp; tmp[0] = 0;

	if (gpSet->nBgImageColors == (DWORD)-1)
	{
		*(pszTemp++) = L'*';
	}
	else for (int idx = 0; nTest && idx < 16; idx++)
	{
		if (nTest & 1)
		{
			if (pszTemp != tmp)
			{
				*pszTemp++ = L' ';
				*pszTemp = 0;
			}

			_wsprintf(pszTemp, SKIPLEN(countof(tmp)-(pszTemp-tmp)) L"#%i", idx);
			pszTemp += _tcslen(pszTemp);
		}

		nTest = nTest >> 1;
	}

	*pszTemp = 0;
	SetDlgItemText(hWnd2, tBgImageColors, tmp);
}

INT_PTR CSetDlgColors::ColorCtlStatic(HWND hWnd2, WORD c, HWND hItem)
{
	if (GetDlgItem(hWnd2, c) == hItem)
	{
		if (mh_CtlColorBrush) DeleteObject(mh_CtlColorBrush);

		COLORREF cr = 0;

		if (c >= c32 && c <= c34)
		{
			ThumbColor *ptc = NULL;

			if (c == c32) ptc = &gpSet->ThSet.crBackground;
			else if (c == c33) ptc = &gpSet->ThSet.crPreviewFrame;
			else ptc = &gpSet->ThSet.crSelectFrame;

			//
			if (ptc->UseIndex)
			{
				if (ptc->ColorIdx >= 0 && ptc->ColorIdx <= 15)
				{
					cr = gpSet->Colors[ptc->ColorIdx];
				}
				else
				{
					CVConGuard VCon;
					const CEFAR_INFO_MAPPING *pfi = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon()->GetFarInfo() : NULL;

					if (pfi && pfi->cbSize>=sizeof(CEFAR_INFO_MAPPING))
					{
						cr = gpSet->Colors[CONBACKCOLOR(pfi->nFarColors[col_PanelText])];
					}
					else
					{
						cr = gpSet->Colors[1];
					}
				}
			}
			else
			{
				cr = ptc->ColorRGB;
			}
		} // if (c >= c32 && c <= c34)
		else
		{
			GetColorById(c, &cr);
		}

		mh_CtlColorBrush = CreateSolidBrush(cr);
		return (INT_PTR)mh_CtlColorBrush;
	}

	return 0;
}

bool CSetDlgColors::ShowColorDialog(HWND HWndOwner, COLORREF *inColor)
{
	CHOOSECOLOR cc;                 // common dialog box structure
	// Вернул. IMHO - бред. Добавили Custom Color, а меняется ФОН окна!
	// Initialize CHOOSECOLOR
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = HWndOwner;
	cc.lpCustColors = (LPDWORD)acrCustClr;
	cc.rgbResult = *inColor;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColor(&cc))
	{
		*inColor = cc.rgbResult;
		return true;
	}

	return false;
}

// Возвращает TRUE, если значение распознано и отличается от старого
bool CSetDlgColors::GetColorRef(HWND hDlg, WORD TB, COLORREF* pCR)
{
	//bool result = false;
	//int r = 0, g = 0, b = 0;
	wchar_t temp[MAX_PATH];
	//wchar_t *pch, *pchEnd;

	if (!GetDlgItemText(hDlg, TB, temp, countof(temp)) || !*temp)
		return false;

	return ::GetColorRef(temp, pCR);
}

void CSetDlgColors::OnSettingsLoaded(const COLORREF (&Colors)[32])
{
	// Init custom palette for ColorSelection dialog
	memmove(acrCustClr, gpSet->Colors, sizeof(COLORREF)*16);
}

void CSetDlgColors::ReleaseHandles()
{
	gColorBoxMap.Reset();
	SafeDeleteObject(mh_CtlColorBrush);
	gbLastColorsOk = FALSE;
}
