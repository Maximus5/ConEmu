
/*
Copyright (c) 2016-2017 Maximus5
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
#include "LngRc.h"
#include "OptionsClass.h"
#include "SetColorPalette.h"
#include "SetDlgLists.h"
#include "SetDlgButtons.h"
#include "SetDlgColors.h"
#include "SetPgColors.h"

CSetPgColors::CSetPgColors()
{
}

CSetPgColors::~CSetPgColors()
{
}

LRESULT CSetPgColors::OnInitDialog(HWND hDlg, bool abInitial)
{
	#if 0
	if (gpSetCls->EnableThemeDialogTextureF)
		gpSetCls->EnableThemeDialogTextureF(hDlg, 6/*ETDT_ENABLETAB*/);
	#endif

	checkRadioButton(hDlg, rbColorRgbDec, rbColorBgrHex, rbColorRgbDec + gpSetCls->m_ColorFormat);

	for (int c = c0; c <= CSetDlgColors::MAX_COLOR_EDT_ID; c++)
	{
		ColorSetEdit(hDlg, c);
		InvalidateCtrl(GetDlgItem(hDlg, c), TRUE);
	}

	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbConClrText), CSetDlgLists::eColorIdxTh, gpSet->AppStd.nTextColorIdx, true);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbConClrBack), CSetDlgLists::eColorIdxTh, gpSet->AppStd.nBackColorIdx, true);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbConClrPopText), CSetDlgLists::eColorIdxTh, gpSet->AppStd.nPopTextColorIdx, true);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbConClrPopBack), CSetDlgLists::eColorIdxTh, gpSet->AppStd.nPopBackColorIdx, true);

	//WARNING("Отладка...");
	//if (gpSet->AppStd.nPopTextColorIdx <= 15 || gpSet->AppStd.nPopBackColorIdx <= 15
	//	|| RELEASEDEBUGTEST(FALSE,TRUE))
	//{
	//	EnableWindow(GetDlgItem(hDlg, lbConClrPopText), TRUE);
	//	EnableWindow(GetDlgItem(hDlg, lbConClrPopBack), TRUE);
	//}

	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbExtendIdx), CSetDlgLists::eColorIdxSh, gpSet->AppStd.nExtendColorIdx, true);
	checkDlgButton(hDlg, cbExtendColors, gpSet->AppStd.isExtendColors ? BST_CHECKED : BST_UNCHECKED);
	CSetDlgButtons::OnButtonClicked(hDlg, cbExtendColors, 0);
	checkDlgButton(hDlg, cbTrueColorer, gpSet->isTrueColorer ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbVividColors, gpSet->isVividColors ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbFadeInactive, gpSet->isFadeInactive ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemInt(hDlg, tFadeLow, gpSet->mn_FadeLow, FALSE);
	SetDlgItemInt(hDlg, tFadeHigh, gpSet->mn_FadeHigh, FALSE);

	// Palette
	const ColorPalette* pPal;

	// Default colors
	if (!abInitial && gbLastColorsOk)
	{
		// активация уже загруженной вкладки, текущую палитру уже запомнили
	}
	else if ((pPal = gpSet->PaletteGet(-1)) != NULL)
	{
		memmove(&gLastColors, pPal, sizeof(gLastColors));
		if (gLastColors.pszName == NULL)
		{
			_ASSERTE(gLastColors.pszName!=NULL);
			static wchar_t szCurrentScheme[64] = L"";
			lstrcpyn(szCurrentScheme, CLngRc::getRsrc(lng_CurClrScheme/*"<Current color scheme>"*/), countof(szCurrentScheme));
			gLastColors.pszName = szCurrentScheme;
		}
	}
	else
	{
		EnableWindow(hDlg, FALSE);
		MBoxAssert(pPal && "PaletteGet(-1) failed");
		return 0;
	}

	SendMessage(GetDlgItem(hDlg, lbDefaultColors), CB_RESETCONTENT, 0, 0);
	SendDlgItemMessage(hDlg, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM)gLastColors.pszName);

	for (int i = 0; (pPal = gpSet->PaletteGet(i)) != NULL; i++)
	{
		SendDlgItemMessage(hDlg, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM)pPal->pszName);
	}

	// Find saved palette with current colors and attributes
	pPal = gpSet->PaletteFindCurrent(true);
	if (pPal)
		CSetDlgLists::SelectStringExact(hDlg, lbDefaultColors, pPal->pszName);
	else
		SendDlgItemMessage(hDlg, lbDefaultColors, CB_SETCURSEL, 0, 0);

	bool bBtnEnabled = pPal && !pPal->bPredefined; // Save & Delete buttons
	EnableWindow(GetDlgItem(hDlg, cbColorSchemeSave), bBtnEnabled);
	EnableWindow(GetDlgItem(hDlg, cbColorSchemeDelete), bBtnEnabled);

	gbLastColorsOk = TRUE;

	return 0;
}

INT_PTR CSetPgColors::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	bool bDoUpdate = true;

	switch (nCtrlId)
	{
	case lbExtendIdx:
		{
			gpSet->AppStd.nExtendColorIdx = SendDlgItemMessage(hDlg, nCtrlId, CB_GETCURSEL, 0, 0);
			break;
		} // lbExtendIdx

	case lbConClrText:
		{
			gpSet->AppStd.nTextColorIdx = SendDlgItemMessage(hDlg, nCtrlId, CB_GETCURSEL, 0, 0);
			if (gpSet->AppStd.nTextColorIdx != gpSet->AppStd.nBackColorIdx)
				gpSetCls->UpdateTextColorSettings(TRUE, FALSE);
			break;
		} // lbConClrText

	case lbConClrBack:
		{
			gpSet->AppStd.nBackColorIdx = SendDlgItemMessage(hDlg, nCtrlId, CB_GETCURSEL, 0, 0);
			if (gpSet->AppStd.nTextColorIdx != gpSet->AppStd.nBackColorIdx)
				gpSetCls->UpdateTextColorSettings(TRUE, FALSE);
			break;
		} // lbConClrBack

	case lbConClrPopText:
		{
			gpSet->AppStd.nPopTextColorIdx = SendDlgItemMessage(hDlg, nCtrlId, CB_GETCURSEL, 0, 0);
			if (gpSet->AppStd.nPopTextColorIdx != gpSet->AppStd.nPopBackColorIdx)
				gpSetCls->UpdateTextColorSettings(FALSE, TRUE);
			break;
		} // lbConClrPopText

	case lbConClrPopBack:
		{
			gpSet->AppStd.nPopBackColorIdx = SendDlgItemMessage(hDlg, nCtrlId, CB_GETCURSEL, 0, 0);
			if (gpSet->AppStd.nPopTextColorIdx != gpSet->AppStd.nPopBackColorIdx)
				gpSetCls->UpdateTextColorSettings(FALSE, TRUE);
			break;
		} // lbConClrPopBack

	case lbDefaultColors:
		{
			HWND hList = GetDlgItem(hDlg, lbDefaultColors);
			INT_PTR nIdx = SendMessage(hList, CB_GETCURSEL, 0, 0);

			// Save & Delete buttons
			{
				bool bEnabled = false;
				wchar_t* pszText = NULL;

				if (code == CBN_EDITCHANGE)
				{
					INT_PTR nLen = GetWindowTextLength(hList);
					pszText = (nLen > 0) ? (wchar_t*)malloc((nLen+1)*sizeof(wchar_t)) : NULL;
					if (pszText)
						GetWindowText(hList, pszText, nLen+1);
				}
				else if ((code == CBN_SELCHANGE) && nIdx > 0) // 0 -- current color scheme. ее удалять/сохранять "нельзя"
				{
					INT_PTR nLen = SendMessage(hList, CB_GETLBTEXTLEN, nIdx, 0);
					pszText = (nLen > 0) ? (wchar_t*)malloc((nLen+1)*sizeof(wchar_t)) : NULL;
					if (pszText)
						SendMessage(hList, CB_GETLBTEXT, nIdx, (LPARAM)pszText);
				}

				if (pszText)
				{
					bEnabled = (wcspbrk(pszText, L"<>") == NULL);
					SafeFree(pszText);
				}

				EnableWindow(GetDlgItem(hDlg, cbColorSchemeSave), bEnabled);
				EnableWindow(GetDlgItem(hDlg, cbColorSchemeDelete), bEnabled);
			}

			// Юзер выбрал в списке другую палитру
			if ((code == CBN_SELCHANGE) && gbLastColorsOk)  // только если инициализация палитр завершилась
			{
				const ColorPalette* pPal = NULL;

				if (nIdx == 0)
					pPal = &gLastColors;
				else if ((pPal = gpSet->PaletteGet(nIdx-1)) == NULL)
					return 0; // неизвестный набор

				gpSetCls->ChangeCurrentPalette(pPal, false);
				bDoUpdate = false;
			}

			break;
		} // lbDefaultColors

	}

	if (bDoUpdate)
		gpConEmu->Update(true);

	return 0;
}

LRESULT CSetPgColors::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	COLORREF color = 0;

	if (nCtrlId == tFadeLow || nCtrlId == tFadeHigh)
	{
		BOOL lbOk = FALSE;
		UINT nVal = GetDlgItemInt(hDlg, nCtrlId, &lbOk, FALSE);

		if (lbOk && nVal <= 255)
		{
			if (nCtrlId == tFadeLow)
				gpSet->mn_FadeLow = nVal;
			else
				gpSet->mn_FadeHigh = nVal;

			gpSet->ResetFadeColors();
			//gpSet->mb_FadeInitialized = false;
			//gpSet->mn_LastFadeSrc = gpSet->mn_LastFadeDst = -1;
		}
	}
	else if (CSetDlgColors::GetColorById(nCtrlId - (tc0-c0), &color))
	{
		if (CSetDlgColors::GetColorRef(hDlg, nCtrlId, &color))
		{
			if (CSetDlgColors::SetColorById(nCtrlId - (tc0-c0), color))
			{
				gpConEmu->InvalidateAll();

				if (nCtrlId >= tc0 && nCtrlId <= tc31)
					gpConEmu->Update(true);

				InvalidateCtrl(GetDlgItem(hDlg, nCtrlId - (tc0-c0)), TRUE);
			}
		}
	}
	else
	{
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}

void CSetPgColors::ColorSchemeSaveDelete(WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbColorSchemeSave || CB==cbColorSchemeDelete);

	if (!mh_Dlg)
	{
		_ASSERTE(mh_Dlg!=NULL);
		return;
	}

	HWND hList = GetDlgItem(mh_Dlg, lbDefaultColors);
	int nLen = GetWindowTextLength(hList);
	if (nLen < 1)
		return;

	wchar_t* pszName = (wchar_t*)malloc((nLen+1)*sizeof(wchar_t));
	GetWindowText(hList, pszName, nLen+1);
	if (*pszName != L'<')
	{
		if (CB == cbColorSchemeSave)
			gpSet->PaletteSaveAs(pszName);
		else
			gpSet->PaletteDelete(pszName);
	}
	// Set focus in list, buttons may become disabled now
	SetFocus(hList);
	HWND hCB = GetDlgItem(mh_Dlg, CB);
	SetWindowLongPtr(hCB, GWL_STYLE, GetWindowLongPtr(hCB, GWL_STYLE) & ~BS_DEFPUSHBUTTON);
	// Refresh
	OnInitDialog(mh_Dlg, false);

	SafeFree(pszName);

} // cbColorSchemeSave || cbColorSchemeDelete
