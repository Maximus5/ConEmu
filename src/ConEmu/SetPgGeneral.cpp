
/*
Copyright (c) 2018-present Maximus5
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
#include "OptionsFast.h"
#include "SetColorPalette.h"
#include "SetDlgLists.h"
#include "SetPgGeneral.h"

CSetPgGeneral::ColorOptions CSetPgGeneral::colorOptions = {};
void CSetPgGeneral::ColorOptions::Reset()
{
	palette = nullptr;
	boxProc = nullptr;
}


CSetPgGeneral::CSetPgGeneral()
{
	colorOptions.Reset();
}

CSetPgGeneral::~CSetPgGeneral()
{
	colorOptions.Reset();
}

LRESULT CSetPgGeneral::OnInitDialog(HWND hDlg, bool abInitial)
{
	// Languages
	if (gpLng)
	{
		MArray<const wchar_t*> languages;
		if (gpLng->getLanguages(languages))
		{
			SendDlgItemMessage(hDlg, lbInterfaceLanguage, CB_RESETCONTENT, 0, 0);
			for (INT_PTR nLang = 0; nLang < languages.size(); nLang++)
			{
				SendDlgItemMessage(hDlg, lbInterfaceLanguage, CB_ADDSTRING, 0, (LPARAM)languages[nLang]);
			}
			INT_PTR nIdx = SendDlgItemMessage(hDlg, lbInterfaceLanguage, CB_FINDSTRING, -1, (LPARAM)CLngRc::getLanguage());
			if (nIdx >= 0)
				SendDlgItemMessage(hDlg, lbInterfaceLanguage, CB_SETCURSEL, nIdx, 0);
		}
	}


	// Tasks
	SendDlgItemMessage(hDlg, lbStartupShellFast, CB_RESETCONTENT, 0, 0);
	if (gpSet->psStartSingleApp && *gpSet->psStartSingleApp)
	{
		SendDlgItemMessage(hDlg, lbStartupShellFast, CB_ADDSTRING, 0, (LPARAM)gpSet->psStartSingleApp);
	}
	if (gpSet->psStartTasksFile && *gpSet->psStartTasksFile)
	{
		wchar_t prefix[2] = {CmdFilePrefix};
		CEStr command(prefix, gpSet->psStartTasksFile);
		SendDlgItemMessage(hDlg, lbStartupShellFast, CB_ADDSTRING, 0, (LPARAM)command.c_str());
	}
	const CommandTasks* pGrp = NULL;
	for (int nGroup = 0; (pGrp = gpSet->CmdTaskGet(nGroup)) != NULL; nGroup++)
	{
		SendDlgItemMessage(hDlg, lbStartupShellFast, CB_ADDSTRING, 0, (LPARAM)pGrp->pszName);
	}
	RECT rcCombo = {}, rcButton = {};
	GetWindowRect(GetDlgItem(hDlg, lbStartupShellFast), &rcCombo);
	GetWindowRect(GetDlgItem(hDlg, cbStartupShellFast), &rcButton);
	MapWindowPoints(NULL, hDlg, (LPPOINT)&rcCombo, 2);
	MapWindowPoints(NULL, hDlg, (LPPOINT)&rcButton, 2);
	MoveWindow(GetDlgItem(hDlg, cbStartupShellFast), rcButton.left, rcCombo.top, RectWidth(rcButton), RectHeight(rcCombo), TRUE);

	// Show startup task or shell command line
	CEStr command;
	LPCWSTR pszStartup = FastConfig::GetStartupCommand(command);
	// Show startup command or task
	if (pszStartup && *pszStartup)
	{
		CSetDlgLists::SelectStringExact(hDlg, lbStartupShellFast, pszStartup);
	}


	// Palettes (console color sets)
	SendDlgItemMessage(hDlg, lbColorSchemeFast, CB_RESETCONTENT, 0, 0);
	const ColorPalette* pPal = NULL;
	for (int nPal = 0; (pPal = gpSet->PaletteGet(nPal)) != NULL; nPal++)
	{
		SendDlgItemMessage(hDlg, lbColorSchemeFast, CB_ADDSTRING, 0, (LPARAM)pPal->pszName);
	}
	// Show active (default) palette
	colorOptions.palette = gpSet->PaletteFindCurrent(true);
	if (colorOptions.palette)
	{
		CSetDlgLists::SelectStringExact(hDlg, lbColorSchemeFast, colorOptions.palette->pszName);
	}
	else
	{
		_ASSERTE(FALSE && "Current paletted was not defined?");
	}
	// Show its colors in box
	if (!colorOptions.boxProc)
	{
		HWND hChild = GetDlgItem(hDlg, stPalettePreviewFast);
		if (hChild)
			colorOptions.boxProc = (WNDPROC)SetWindowLongPtr(hChild, GWLP_WNDPROC, (LONG_PTR)CSetPgGeneral::ColorBoxProc);
	}


	// Single instance
	CheckDlgButton(hDlg, cbSingleInstance, gpSetCls->IsSingleInstanceArg());


	// Quake style and show/hide key
	CheckDlgButton(hDlg, cbQuakeFast, gpSet->isQuakeStyle ? BST_CHECKED : BST_UNCHECKED);
	const ConEmuHotKey* pHK = NULL;
	if (gpSet->GetHotkeyById(vkMinimizeRestore, &pHK) && pHK)
	{
		wchar_t szKey[128] = L"";
		SetDlgItemText(hDlg, tQuakeKeyFast, pHK->GetHotkeyName(szKey));
	}


	// Keyhooks required for Win+Number, Win+Arrows, etc.
	CheckDlgButton(hDlg, cbUseKeyboardHooksFast, gpSet->isKeyboardHooks(true));
	// Debug purposes only. ConEmu.exe switch "/nokeyhooks"
	#ifdef _DEBUG
	EnableWindow(GetDlgItem(hDlg, cbUseKeyboardHooksFast), !gpConEmu->DisableKeybHooks);
	#endif

	// Injects
	CheckDlgButton(hDlg, cbInjectConEmuHkFast, gpSet->isUseInjects);

	// Autoupdates
	if (!gpConEmu->isUpdateAllowed())
	{
		EnableWindow(GetDlgItem(hDlg, cbEnableAutoUpdateFast), FALSE);
		EnableWindow(GetDlgItem(hDlg, rbAutoUpdateStableFast), FALSE);
		EnableWindow(GetDlgItem(hDlg, rbAutoUpdatePreviewFast), FALSE);
		EnableWindow(GetDlgItem(hDlg, rbAutoUpdateDeveloperFast), FALSE);
		EnableWindow(GetDlgItem(hDlg, stEnableAutoUpdateFast), FALSE);
		CheckDlgButton(hDlg, cbEnableAutoUpdateFast, FALSE);
	}
	else
	{
		CheckDlgButton(hDlg, cbEnableAutoUpdateFast,
			(gpSet->UpdSet.isUpdateUseBuilds != ConEmuUpdateSettings::Builds::Undefined)
			&& (gpSet->UpdSet.isUpdateCheckOnStartup|gpSet->UpdSet.isUpdateCheckHourly));
	}
	CheckRadioButton(hDlg, rbAutoUpdateStableFast, rbAutoUpdateDeveloperFast,
		(gpSet->UpdSet.isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Stable) ? rbAutoUpdateStableFast
		: (gpSet->UpdSet.isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Preview) ? rbAutoUpdatePreviewFast
		: rbAutoUpdateDeveloperFast);

	return 0;
}

INT_PTR CSetPgGeneral::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	CEStr lsValue;

	switch (nCtrlId)
	{
	case lbInterfaceLanguage:
		if (gpLng && CSetDlgLists::GetSelectedString(hDlg, lbInterfaceLanguage, lsValue) > 0)
		{
			wchar_t* colon = wcschr(lsValue.ms_Val, L':');
			if (colon)
			{
				*colon = 0;
				if (gpConEmu->opt.Language.Exists)
					gpConEmu->opt.Language.SetStr(lsValue);
				else
					lstrcpyn(gpSet->Language, lsValue, countof(gpSet->Language));
				gpLng->Reload(true);

				bool lbWasPos = false;
				RECT rcWnd = {};
				if (ghOpWnd && IsWindow(ghOpWnd))
				{
					lbWasPos = true;
					GetWindowRect(ghOpWnd, &rcWnd);
					DestroyWindow(ghOpWnd);
					_ASSERTE(ghOpWnd == NULL);

					CSettings::Dialog(IDD_SPG_GENERAL);
					if (ghOpWnd)
						SetWindowPos(ghOpWnd, NULL, rcWnd.left, rcWnd.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
				}
			}
		}
		break;
	case lbColorSchemeFast:
		SendDlgItemMessage(hDlg, stPalettePreviewFast, UM_PALETTE_FAST_CHG, 0, 0);
		break;
	case lbStartupShellFast:
		FastConfig::DoStartupCommand(hDlg, lbStartupShellFast);
		break;
	default:
		_ASSERTE(FALSE && "ListBox was not processed");
	}

	return 0;
}

LRESULT CSetPgGeneral::ColorBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc = 0;

	switch (uMsg)
	{
		case WM_PAINT:
		{
			if (!colorOptions.palette)
			{
				_ASSERTE(colorOptions.palette!=NULL);
				break;
			}
			FastConfig::DoPaintColorBox(hwnd, *colorOptions.palette);
			goto wrap;
		} // WM_PAINT

		case UM_PALETTE_FAST_CHG:
		{
			CEStr lsValue;
			if (CSetDlgLists::GetSelectedString(GetParent(hwnd), lbColorSchemeFast, lsValue) > 0)
			{
				const ColorPalette* pPal = gpSet->PaletteGetByName(lsValue);
				if (pPal)
				{
					colorOptions.palette = pPal;
					InvalidateRect(hwnd, NULL, FALSE);

					gpSetCls->ChangeCurrentPalette(pPal, false);
				}
			}
			goto wrap;
		} // UM_PALETTE_FAST_CHG
	}

	if (colorOptions.boxProc)
		lRc = CallWindowProc(colorOptions.boxProc, hwnd, uMsg, wParam, lParam);
	else
		lRc = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
wrap:
	return lRc;
}
