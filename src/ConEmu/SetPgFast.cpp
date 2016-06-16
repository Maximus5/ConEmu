
/*
Copyright (c) 2016 Maximus5
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

#include "AboutDlg.h"
#include "ConEmu.h"
#include "HotkeyDlg.h"
#include "Options.h"
#include "OptionsClass.h"
#include "SetColorPalette.h"
#include "SetDlgButtons.h"
#include "SetDlgLists.h"
#include "SetPgFast.h"

ConEmuHotKey CSetPgFast::ghk_MinMaxKey = {};
const ColorPalette* CSetPgFast::gp_DefaultPalette = NULL;
WNDPROC CSetPgFast::gpfn_DefaultColorBoxProc = NULL;

CSetPgFast::CSetPgFast()
{
	gp_DefaultPalette = NULL;
	gpfn_DefaultColorBoxProc = NULL;
}

CSetPgFast::~CSetPgFast()
{
	gp_DefaultPalette = NULL;
	gpfn_DefaultColorBoxProc = NULL;
}

LRESULT CSetPgFast::OnInitDialog(HWND hDlg, bool abInitial)
{
	// Tasks
	SendDlgItemMessage(hDlg, lbStartupShellFast, CB_RESETCONTENT, 0,0);
	const CommandTasks* pGrp = NULL;
	for (int nGroup = 0; (pGrp = gpSet->CmdTaskGet(nGroup)) != NULL; nGroup++)
	{
		SendDlgItemMessage(hDlg, lbStartupShellFast, CB_ADDSTRING, 0, (LPARAM)pGrp->pszName);
	}

	// Show startup task or shell command line
	const CommandTasks* pTask = NULL;
	LPCWSTR pszStartup = GetStartupCommand(pTask);
	// Show startup command or task
	if (pszStartup && *pszStartup)
	{
		CSetDlgLists::SelectStringExact(hDlg, lbStartupShellFast, pszStartup);
	}



	// Palettes (console color sets)
	const ColorPalette* pPal = NULL;
	for (int nPal = 0; (pPal = gpSet->PaletteGet(nPal)) != NULL; nPal++)
	{
		SendDlgItemMessage(hDlg, lbColorSchemeFast, CB_ADDSTRING, 0, (LPARAM)pPal->pszName);
	}
	// Show active (default) palette
	const ColorPalette* pDefaultPalette = InitDefaultPalette();
	if (pDefaultPalette)
	{
		CSetDlgLists::SelectStringExact(hDlg, lbColorSchemeFast, pDefaultPalette->pszName);
	}
	else
	{
		_ASSERTE(FALSE && "Current paletted was not defined?");
	}
	// Show its colors in box
	if (abInitial)
	{
		InitPalettePreview(hDlg, stPalettePreviewFast);
	}



	// Single instance
	checkDlgButton(hDlg, cbSingleInstance, gpSetCls->IsSingleInstanceArg());


	// Quake style and show/hide key
	CheckDlgButton(hDlg, cbQuakeFast, gpSet->isQuakeStyle ? BST_CHECKED : BST_UNCHECKED);
	const ConEmuHotKey* pHK = NULL;
	if (gpSet->GetHotkeyById(vkMinimizeRestore, &pHK) && pHK)
	{
		wchar_t szKey[128] = L"";
		SetDlgItemText(hDlg, tQuakeKeyFast, pHK->GetHotkeyName(szKey));
		ghk_MinMaxKey.SetVkMod(pHK->GetVkMod());
	}


	// Keyhooks required for Win+Number, Win+Arrows, etc.
	checkDlgButton(hDlg, cbUseKeyboardHooksFast, gpSet->isKeyboardHooks(true));



	// Debug purposes only. ConEmu.exe switch "/nokeyhooks"
	#ifdef _DEBUG
	enableDlgItem(hDlg, cbUseKeyboardHooksFast, !gpConEmu->DisableKeybHooks);
	#endif

	// Injects
	checkDlgButton(hDlg, cbInjectConEmuHkFast, gpSet->isUseInjects);



	// Autoupdates
	if (!gpConEmu->isUpdateAllowed())
	{
		enableDlgItem(hDlg, cbEnableAutoUpdateFast, false);
		enableDlgItem(hDlg, rbAutoUpdateStableFast, false);
		enableDlgItem(hDlg, rbAutoUpdatePreviewFast, false);
		enableDlgItem(hDlg, rbAutoUpdateDeveloperFast, false);
		enableDlgItem(hDlg, stEnableAutoUpdateFast, false);
	}
	else
	{
		if (gpSet->UpdSet.isUpdateUseBuilds != 0)
			checkDlgButton(hDlg, cbEnableAutoUpdateFast, gpSet->UpdSet.isUpdateCheckOnStartup|gpSet->UpdSet.isUpdateCheckHourly);
		checkRadioButton(hDlg, rbAutoUpdateStableFast, rbAutoUpdateDeveloperFast,
			(gpSet->UpdSet.isUpdateUseBuilds == 1) ? rbAutoUpdateStableFast :
			(gpSet->UpdSet.isUpdateUseBuilds == 3) ? rbAutoUpdatePreviewFast : rbAutoUpdateDeveloperFast);
	}

	return 0;
}

const ColorPalette* CSetPgFast::InitDefaultPalette()
{
	gp_DefaultPalette = gpSet->PaletteFindCurrent(true);
	return gp_DefaultPalette;
}

void CSetPgFast::InitPalettePreview(HWND hDlg, WORD nCtrlId)
{
	// Show current palette colors in the box
	HWND hChild = GetDlgItem(hDlg, nCtrlId);
	if (!hChild)
	{
		_ASSERTE(FALSE && "Palette preview control was not found");
	}
	else
	{
		gpfn_DefaultColorBoxProc = (WNDPROC)SetWindowLongPtr(hChild, GWLP_WNDPROC, (LONG_PTR)CSetPgFast::ColorBoxProc);
	}
}

LRESULT CALLBACK CSetPgFast::ColorBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc = 0;

	switch (uMsg)
	{
		case WM_PAINT:
		{
			if (!gp_DefaultPalette)
			{
				_ASSERTE(gp_DefaultPalette!=NULL);
				break;
			}
			RECT rcClient = {};
			PAINTSTRUCT ps = {};
			if (BeginPaint(hwnd, &ps))
			{
				GetClientRect(hwnd, &rcClient);
				for (int i = 0; i < 16; i++)
				{
					int x = (i % 8);
					int y = (i == x) ? 0 : 1;
					RECT rc = {(x) * rcClient.right / 8, (y) * rcClient.bottom / 2,
						(x+1) * rcClient.right / 8, (y+1) * rcClient.bottom / 2};
					HBRUSH hbr = CreateSolidBrush(gp_DefaultPalette->Colors[i]);
					FillRect(ps.hdc, &rc, hbr);
					DeleteObject(hbr);
				}
				EndPaint(hwnd, &ps);
			}
			goto wrap;
		} // WM_PAINT

		case UM_PALETTE_FAST_CHG:
		{
			CEStr lsValue;
			if (CSetDlgLists::GetSelectedString(GetParent(hwnd), lbColorSchemeFast, &lsValue.ms_Val) > 0)
			{
				const ColorPalette* pPal = gpSet->PaletteGetByName(lsValue.ms_Val);
				if (pPal)
				{
					gp_DefaultPalette = pPal;
					InvalidateRect(hwnd, NULL, FALSE);
				}
			}
			goto wrap;
		} // UM_PALETTE_FAST_CHG
	}

	if (gpfn_DefaultColorBoxProc)
		lRc = CallWindowProc(gpfn_DefaultColorBoxProc, hwnd, uMsg, wParam, lParam);
	else
		lRc = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
wrap:
	return lRc;
}

LPCWSTR CSetPgFast::GetStartupCommand(const CommandTasks*& pTask)
{
	pTask = NULL;
	// Show startup task or shell command line
	LPCWSTR pszStartup = (gpSet->nStartType == 2) ? gpSet->psStartTasksName : (gpSet->nStartType == 0) ? gpSet->psStartSingleApp : NULL;
	// Check if that task exists
	if ((gpSet->nStartType == 2) && pszStartup)
	{
		pTask = gpSet->CmdTaskGetByName(gpSet->psStartTasksName);
		if (pTask && pTask->pszName && (lstrcmp(pTask->pszName, pszStartup) != 0))
		{
			// Return pTask name because it may not match exactly with gpSet->psStartTasksName
			// because CmdTaskGetByName uses some fuzzy logic to find tasks
			pszStartup = pTask->pszName;
		}
		else if (!pTask)
		{
			pszStartup = NULL;
		}
	}
	return pszStartup;
}

INT_PTR CSetPgFast::OnButtonClicked(HWND hDlg, HWND hBtn, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case cbQuakeKeyFast:
		{
			DWORD VkMod = ghk_MinMaxKey.GetVkMod();
			if (CHotKeyDialog::EditHotKey(hDlg, VkMod))
			{
				ghk_MinMaxKey.SetVkMod(VkMod);
				wchar_t szKey[128] = L"";
				SetDlgItemText(hDlg, tQuakeKeyFast, ghk_MinMaxKey.GetHotkeyName(szKey));
			}
			return 1;
		}

	case cbSingleInstance:
	case cbQuakeFast:
	case cbUseKeyboardHooksFast:
	case cbInjectConEmuHkFast:
		CSetDlgButtons::OnButtonClicked(hDlg, nCtrlId, 0);
		break;
	case cbEnableAutoUpdateFast:
		break;
	case rbAutoUpdateStableFast:
	case rbAutoUpdatePreviewFast:
	case rbAutoUpdateDeveloperFast:
		break;
	}

	return CSetPgBase::OnButtonClicked(hDlg, hBtn, nCtrlId);
}

INT_PTR CSetPgFast::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	CEStr lsValue;

	switch (nCtrlId)
	{
	case lbStartupShellFast:
	{
		if ((code == CBN_EDITCHANGE) || (code == CBN_SELCHANGE))
		{
			GetString(hDlg, nCtrlId, &lsValue.ms_Val, NULL, (code == CBN_SELCHANGE));

			if (!lsValue.IsEmpty())
			{
				if (*lsValue.ms_Val == TaskBracketLeft)
				{
					if (lsValue.ms_Val[lstrlen(lsValue.ms_Val)-1] != TaskBracketRight)
					{
						_ASSERTE(FALSE && "Doesn't match '{...}'");
					}
					else
					{
						gpSet->nStartType = 2;
						SafeFree(gpSet->psStartTasksName);
						gpSet->psStartTasksName = lstrdup(lsValue.ms_Val);
					}
				}
				else if (lstrcmp(lsValue.ms_Val, AutoStartTaskName) == 0)
				{
					// Not shown yet in list?
					gpSet->nStartType = 3;
				}
				else if (*lsValue.ms_Val == CmdFilePrefix)
				{
					gpSet->nStartType = 1;
					SafeFree(gpSet->psStartTasksFile);
					gpSet->psStartTasksFile = lstrdup(lsValue.ms_Val);
				}
				else
				{
					gpSet->nStartType = 0;
					SafeFree(gpSet->psStartSingleApp);
					gpSet->psStartSingleApp = lstrdup(lsValue.ms_Val);
				}
			}
		}
		break;
	} // lbStartupShellFast

	case lbColorSchemeFast:
	{
		if (code == CBN_SELCHANGE)
		{
			// Default pallette changed
			if (CSetDlgLists::GetSelectedString(hDlg, lbColorSchemeFast, &lsValue.ms_Val) > 0)
			{
				const ColorPalette* pPal = gpSet->PaletteGetByName(lsValue.ms_Val);
				if (pPal)
				{
					gpSetCls->ChangeCurrentPalette(pPal, false);
				}
			}
			SendDlgItemMessage(hDlg, stPalettePreviewFast, UM_PALETTE_FAST_CHG, 0, 0);
		}
		break;
	} // lbColorSchemeFast

	default:
		_ASSERTE(FALSE && "ComboBox was not processed");
	}

	return 0;
}
