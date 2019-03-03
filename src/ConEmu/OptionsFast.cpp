
/*
Copyright (c) 2009-present Maximus5
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
#include "DynDialog.h"
#include "HotkeyDlg.h"
#include "LngRc.h"
#include "Options.h"
#include "OptionsClass.h"
#include "OptionsFast.h"
#include "OptionsHelp.h"
#include "SetCmdTask.h"
#include "SetColorPalette.h"
#include "SetDlgLists.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "Update.h"
#include "../ConEmuCD/ExitCodes.h"
#include "../common/CEStr.h"
#include "../common/EnvVar.h"
#include "../common/execute.h"
#include "../common/FarVersion.h"
#include "../common/MSetter.h"
#include "../common/MStrDup.h"
#include "../common/MWow64Disable.h"
#include "../common/StartupEnvDef.h"
#include "../common/WFiles.h"
#include "../common/WRegistry.h"
#include "../common/WUser.h"

#define DEBUGSTRTASKS(s) DEBUGSTR(s)

namespace FastConfig
{

#define FOUND_APP_PATH_CHR L'\1'
#define FOUND_APP_PATH_STR L"\1"

HWND ghFastCfg = NULL;
static bool bCheckHooks, bCheckUpdate, bCheckIme;
// Если файл конфигурации пуст, то после вызова CheckOptionsFast
// все равно будет SaveSettings(TRUE/*abSilent*/);
// Поэтому выбранные настройки здесь можно не сохранять (кроме StopWarningConIme)
static bool bVanilla;
static CDpiForDialog* gp_DpiAware = NULL;
static int gn_FirstFarTask = -1;
static ConEmuHotKey ghk_MinMaxKey = {};
static int iCreatIdx = 0;
static CEStr szConEmuDrive;
static SettingsLoadedFlags sAppendMode = slf_None;

/* **************************************** */
/*             Helper functions             */
/* **************************************** */

// Special wrapper for FastConfiguration dialog,
// we can't use here standard MsgBox, because messaging was not started yet.
int FastMsgBox(LPCTSTR lpText, UINT uType, LPCTSTR lpCaption = NULL, HWND ahParent = (HWND)-1, bool abModal = true)
{
	MSetter lSet(&gbMessagingStarted);
	int iBtn = ::MsgBox(lpText, uType, lpCaption, ahParent, abModal);
	return iBtn;
}

void FindStartupTask(SettingsLoadedFlags slfFlags)
{
	const CommandTasks* pTask = NULL;

	// The idea is if user runs "ConEmu.exe -cmd {cmd}"
	// and this is new config - we must set {cmd} as default task
	// Same here with plain commands, at least show them in FastConfig dlg,
	// don't store in settings if command was passed with "-cmdlist ..."

	bool bIsCmdList = false;
	LPCWSTR pszCmdLine = gpConEmu->GetCurCmd(&bIsCmdList);
	if (pszCmdLine)
	{
		wchar_t cType = bIsCmdList ? CmdFilePrefix /*just for simplicity*/
			: gpConEmu->IsConsoleBatchOrTask(pszCmdLine);
		if (cType == TaskBracketLeft)
		{
			pTask = gpSet->CmdTaskGetByName(pszCmdLine);
		}
		else if (!cType)
		{
			// Don't set default task, use exact command specified by user
			if ((gpSet->nStartType == 0) && !gpSet->psStartSingleApp)
			{
				gpSet->psStartSingleApp = lstrdup(pszCmdLine);
			}
			return;
		}
	}

	if (!pTask && (gn_FirstFarTask != -1))
	{
		pTask = gpSet->CmdTaskGet(gn_FirstFarTask);
	}

	LPCWSTR DefaultNames[] = {
		//L"Far", -- no need to find "Far" it must be processed already with gn_FirstFarTask
		L"{TCC}",
		L"{NYAOS}",
		L"{cmd}",
		NULL
	};

	for (INT_PTR i = 0; !pTask && DefaultNames[i]; i++)
	{
		pTask = gpSet->CmdTaskGetByName(DefaultNames[i]);
	}

	if (pTask)
	{
		gpSet->nStartType = 2;
		SafeFree(gpSet->psStartTasksName);
		gpSet->psStartTasksName = lstrdup(pTask->pszName);
	}
}

LPCWSTR GetStartupCommand(CEStr& command)
{
	command.Clear();

	// Show startup task or shell command line
	switch (gpSet->nStartType)
	{
	case 0:
		command.Set(gpSet->psStartSingleApp);
		break;
	case 1:
		if (gpSet->psStartTasksFile)
		{
			wchar_t prefix[2] = {CmdFilePrefix};
			command = lstrmerge(prefix, gpSet->psStartTasksFile);
		}
		break;
	case 2:
		// Check if that task exists
		if (gpSet->psStartTasksName)
		{
			const CommandTasks* pTask = gpSet->CmdTaskGetByName(gpSet->psStartTasksName);
			if (pTask && pTask->pszName /*&& (lstrcmp(pTask->pszName, pszStartup) != 0)*/)
			{
				// Return pTask name because it may not match exactly with gpSet->psStartTasksName
				// because CmdTaskGetByName uses some fuzzy logic to find tasks
				command.Set(pTask->pszName);
			}
		}
		break;
	case 3:
		command.Set(AutoStartTaskName);
		break;
	}

	return command.ms_Val;
}




/* **************************************** */
/*        Fast Configuration Dialog         */
/* **************************************** */

void DoPaintColorBox(HWND hCtrl, const ColorPalette& pal)
{
	RECT rcClient = {};
	PAINTSTRUCT ps = {};
	if (BeginPaint(hCtrl, &ps))
	{
		GetClientRect(hCtrl, &rcClient);
		for (int i = 0; i < 16; i++)
		{
			int x = (i % 8);
			int y = (i == x) ? 0 : 1;
			RECT rc = {(x) * rcClient.right / 8, (y) * rcClient.bottom / 2,
				(x+1) * rcClient.right / 8, (y+1) * rcClient.bottom / 2};
			HBRUSH hbr = CreateSolidBrush(pal.Colors[i]);
			FillRect(ps.hdc, &rc, hbr);
			DeleteObject(hbr);
		}
		EndPaint(hCtrl, &ps);
	}
}

static const ColorPalette* gp_DefaultPalette = NULL;
static WNDPROC gpfn_DefaultColorBoxProc = NULL;

static LRESULT CALLBACK ColorBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			DoPaintColorBox(hwnd, *gp_DefaultPalette);
			goto wrap;
		} // WM_PAINT

		case UM_PALETTE_FAST_CHG:
		{
			CEStr lsValue;
			if (CSetDlgLists::GetSelectedString(GetParent(hwnd), lbColorSchemeFast, lsValue) > 0)
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



static INT_PTR OnInitDialog(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	ghFastCfg = hDlg;

	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);

	CDynDialog::LocalizeDialog(hDlg);

	if (gp_DpiAware)
	{
		gp_DpiAware->Attach(hDlg, NULL, CDynDialog::GetDlgClass(hDlg));
	}

	// Position dialog in the workarea center
	CDpiAware::CenterDialog(hDlg);

	if (lParam)
	{
		SetWindowText(hDlg, (LPCWSTR)lParam);
	}
	else
	{
		wchar_t szTitle[512];
		wcscpy_c(szTitle, gpConEmu->GetDefaultTitle());
		wcscat_c(szTitle, L" fast configuration");
		SetWindowText(hDlg, szTitle);
	}


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

	// lbStorageLocation
	SettingsStorage Storage = gpSet->GetSettingsType();

	// Same priority as in CConEmuMain::ConEmuXml (reverse order)
	wchar_t* pszSettingsPlaces[] = {
		lstrdup(L"HKEY_CURRENT_USER\\Software\\ConEmu"),
		ExpandEnvStr(L"%APPDATA%\\ConEmu.xml"),
		GetFullPathNameEx(L"%ConEmuBaseDir%\\ConEmu.xml"), // compact "C:\ConEmu\src\..\Debug\ConEmu.xml" to "C:\ConEmu\Debug\ConEmu.xml"
		GetFullPathNameEx(L"%ConEmuDir%\\ConEmu.xml"),
		NULL
	};
	// Lets find first allowed item
	int iAllowed = 0;
	if (Storage.Type == StorageType::XML)
	{
		iAllowed = 1; // XML is used, registry is not allowed
		if (Storage.File)
		{
			if (lstrcmpi(Storage.File, pszSettingsPlaces[1]) == 0) // %APPDATA%
				iAllowed = 1; // Any other xml has greater priority
			else if (lstrcmpi(Storage.File, pszSettingsPlaces[2]) == 0) // %ConEmuBaseDir%
				iAllowed = 2; // Only %ConEmuDir% has greater priority
			else if (lstrcmpi(Storage.File, pszSettingsPlaces[3]) == 0) // %ConEmuDir%
				iAllowed = 3; // Most prioritized
			else
			{
				// Directly specified with "/LoadCfgFile ..."
				SafeFree(pszSettingsPlaces[3]);
				pszSettingsPlaces[3] = lstrdup(Storage.File);
				iAllowed = 3; // Most prioritized
			}
		}
	}
	// Index of the default location (relative to listbox, but not a pszSettingsPlaces)
	// By default - suggest %APPDATA% or, if possible, %ConEmuDir%
	int iDefault = -1;
	// If registry was detected?
	if (iAllowed == 0)
	{
		if (Storage.Type == StorageType::REG)
		{
			SettingsBase* reg = gpSet->CreateSettings(&Storage);
			if (reg)
			{
				if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_READ))
				{
					iDefault = 0;
					reg->CloseKey();
				}
				delete reg;
			}
		}
		else
		{
			_ASSERTE(Storage.Type == StorageType::REG);
		}
	}
	// If still not decided - use xml if possible
	if (iDefault == -1)
	{
		iDefault = (iAllowed == 0) ? (CConEmuUpdate::NeedRunElevation() ? 1 : 3) : 0;
	}

	// Populate lbStorageLocation
	while (pszSettingsPlaces[iAllowed])
	{
		SendDlgItemMessage(hDlg, lbStorageLocation, CB_ADDSTRING, 0, (LPARAM)pszSettingsPlaces[iAllowed]);
		iAllowed++;
	}
	SendDlgItemMessage(hDlg, lbStorageLocation, CB_SETCURSEL, iDefault, 0);

	// Release memory
	for (int i = 0; pszSettingsPlaces[i]; i++)
	{
		SafeFree(pszSettingsPlaces[i]);
	}


	// Tasks
	const CommandTasks* pGrp = NULL;
	for (int nGroup = 0; (pGrp = gpSet->CmdTaskGet(nGroup)) != NULL; nGroup++)
	{
		SendDlgItemMessage(hDlg, lbStartupShellFast, CB_ADDSTRING, 0, (LPARAM)pGrp->pszName);
	}

	// Show startup task or shell command line
	CEStr command;
	LPCWSTR pszStartup = GetStartupCommand(command);
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
	gp_DefaultPalette = gpSet->PaletteFindCurrent(true);
	if (gp_DefaultPalette)
	{
		CSetDlgLists::SelectStringExact(hDlg, lbColorSchemeFast, gp_DefaultPalette->pszName);
	}
	else
	{
		_ASSERTE(FALSE && "Current paletted was not defined?");
	}
	// Show its colors in box
	HWND hChild = GetDlgItem(hDlg, stPalettePreviewFast);
	if (hChild)
		gpfn_DefaultColorBoxProc = (WNDPROC)SetWindowLongPtr(hChild, GWLP_WNDPROC, (LONG_PTR)ColorBoxProc);


	// Single instance
	CheckDlgButton(hDlg, cbSingleInstance, gpSetCls->IsSingleInstanceArg());


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
	}
	else
	{
		if (gpSet->UpdSet.isUpdateUseBuilds != ConEmuUpdateSettings::Builds::Undefined)
			CheckDlgButton(hDlg, cbEnableAutoUpdateFast, gpSet->UpdSet.isUpdateCheckOnStartup|gpSet->UpdSet.isUpdateCheckHourly);
		CheckRadioButton(hDlg, rbAutoUpdateStableFast, rbAutoUpdateDeveloperFast,
			(gpSet->UpdSet.isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Stable) ? rbAutoUpdateStableFast
			: (gpSet->UpdSet.isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Preview) ? rbAutoUpdatePreviewFast
			: rbAutoUpdateDeveloperFast);
	}

	// Vista - ConIme bugs
	if (!bCheckIme)
	{
		ShowWindow(GetDlgItem(hDlg, gbDisableConImeFast), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, cbDisableConImeFast), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, stDisableConImeFast1), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, stDisableConImeFast2), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, stDisableConImeFast3), SW_HIDE);
		RECT rcGroup, rcBtn, rcWnd;
		if (GetWindowRect(GetDlgItem(hDlg, gbDisableConImeFast), &rcGroup))
		{
			int nShift = (rcGroup.bottom-rcGroup.top);

			HWND h = GetDlgItem(hDlg, IDOK);
			GetWindowRect(h, &rcBtn); MapWindowPoints(NULL, hDlg, (LPPOINT)&rcBtn, 2);
			SetWindowPos(h, NULL, rcBtn.left, rcBtn.top - nShift, 0,0, SWP_NOSIZE|SWP_NOZORDER);

			h = GetDlgItem(hDlg, IDCANCEL);
			GetWindowRect(h, &rcBtn); MapWindowPoints(NULL, hDlg, (LPPOINT)&rcBtn, 2);
			SetWindowPos(h, NULL, rcBtn.left, rcBtn.top - nShift, 0,0, SWP_NOSIZE|SWP_NOZORDER);

			h = GetDlgItem(hDlg, stHomePage);
			GetWindowRect(h, &rcBtn); MapWindowPoints(NULL, hDlg, (LPPOINT)&rcBtn, 2);
			SetWindowPos(h, NULL, rcBtn.left, rcBtn.top - nShift, 0,0, SWP_NOSIZE|SWP_NOZORDER);
			SetWindowText(h, gsFirstStart);

			GetWindowRect(hDlg, &rcWnd);
			MoveWindow(hDlg, rcWnd.left, rcWnd.top+(nShift>>1), rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top-nShift, FALSE);
		}
	}

	// Done
	SetFocus(GetDlgItem(hDlg, IDOK));
	return FALSE; // Set focus to OK
}

static INT_PTR OnCtlColorStatic(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	if (GetDlgItem(hDlg, stDisableConImeFast1) == (HWND)lParam)
	{
		SetTextColor((HDC)wParam, 255);
		HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
		SetBkMode((HDC)wParam, TRANSPARENT);
		return (INT_PTR)hBrush;
	}
	else if (GetDlgItem(hDlg, stHomePage) == (HWND)lParam)
	{
		SetTextColor((HDC)wParam, GetSysColor(COLOR_HOTLIGHT));
		HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
		SetBkMode((HDC)wParam, TRANSPARENT);
		return (INT_PTR)hBrush;
	}
	else
	{
		SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
		HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
		SetBkMode((HDC)wParam, TRANSPARENT);
		return (INT_PTR)hBrush;
	}

	return 0;
}

static INT_PTR OnSetCursor(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	if (((HWND)wParam) == GetDlgItem(hDlg, stHomePage))
	{
		SetCursor(LoadCursor(NULL, IDC_HAND));
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
		return TRUE;
	}

	return FALSE;
}

void DoStartupCommand(HWND hDlg, WORD nCtrlId)
{
	CEStr lsValue(GetDlgItemTextPtr(hDlg, nCtrlId));
	if (lsValue)
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
				gpSet->psStartTasksName = lsValue.Detach();
			}
		}
		else if (lstrcmp(lsValue.ms_Val, AutoStartTaskName) == 0)
		{
			// Not shown yet in list
			gpSet->nStartType = 3;
		}
		else if (*lsValue.ms_Val == CmdFilePrefix)
		{
			gpSet->nStartType = 1;
			SafeFree(gpSet->psStartTasksFile);
			gpSet->psStartTasksFile = lsValue.Detach();
		}
		else
		{
			gpSet->nStartType = 0;
			SafeFree(gpSet->psStartSingleApp);
			gpSet->psStartSingleApp = lsValue.Detach();
		}
	}
}

static INT_PTR OnButtonClicked(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
		case IDOK:
		{
			CEStr lsValue;

			SettingsStorage CurStorage = gpSet->GetSettingsType();

			LRESULT lSelStorage = SendDlgItemMessage(hDlg, lbStorageLocation, CB_GETCURSEL, 0, 0);
			if (lSelStorage > 0)
			{
				// Значит юзер выбрал "создать настройки" в другом месте
				wchar_t* pszNewPlace = GetDlgItemTextPtr(hDlg, lbStorageLocation);
				if (!gpConEmu->SetConfigFile(pszNewPlace, true/*abWriteReq*/, false/*abSpecialPath*/))
				{
					// Ошибка уже показана
					SafeFree(pszNewPlace);
					return 1;
				}
				SafeFree(pszNewPlace);
			}

			/* Startup task */
			DoStartupCommand(hDlg, lbStartupShellFast);

			/* Default pallette changed? */
			if (CSetDlgLists::GetSelectedString(hDlg, lbColorSchemeFast, lsValue) > 0)
			{
				const ColorPalette* pPal = gpSet->PaletteGetByName(lsValue.ms_Val);
				if (pPal)
				{
					gpSetCls->ChangeCurrentPalette(pPal, false);
				}
			}

			/* Force Single instance mode */
			gpSet->isSingleInstance = CDlgItemHelper::isChecked2(hDlg, cbSingleInstance);

			/* Quake mode? */
			gpSet->isQuakeStyle = CDlgItemHelper::isChecked2(hDlg, cbQuakeFast);

			/* Min/Restore key */
			gpSet->SetHotkeyById(vkMinimizeRestore, ghk_MinMaxKey.GetVkMod());

			/* Install Keyboard hooks */
			gpSet->m_isKeyboardHooks = IsDlgButtonChecked(hDlg, cbUseKeyboardHooksFast) ? 1 : 2;

			/* Inject ConEmuHk.dll */
			gpSet->isUseInjects = CDlgItemHelper::isChecked2(hDlg, cbInjectConEmuHkFast);

			/* Auto Update settings */
			gpSet->UpdSet.isUpdateCheckOnStartup = (IsDlgButtonChecked(hDlg, cbEnableAutoUpdateFast) == BST_CHECKED);
			if (bCheckUpdate)
			{	// При первом запуске - умолчания параметров
				gpSet->UpdSet.isUpdateCheckHourly = true;
				gpSet->UpdSet.isUpdateConfirmDownload = true; // true-Show MessageBox, false-notify via TSA only
			}
			gpSet->UpdSet.isUpdateUseBuilds = IsDlgButtonChecked(hDlg, rbAutoUpdateStableFast) ? ConEmuUpdateSettings::Builds::Stable
				: IsDlgButtonChecked(hDlg, rbAutoUpdateDeveloperFast) ? ConEmuUpdateSettings::Builds::Alpha
				: ConEmuUpdateSettings::Builds::Preview;


			/* Save settings */
			SettingsBase* reg = NULL;

			if (!bVanilla)
			{
				if ((reg = gpSet->CreateSettings(NULL)) == NULL)
				{
					_ASSERTE(reg!=NULL);
				}
				else
				{
					gpSet->SaveVanilla(reg);
					delete reg;
				}
			}


			// Vista & ConIme.exe
			if (bCheckIme)
			{
				if (IsDlgButtonChecked(hDlg, cbDisableConImeFast))
				{
					HKEY hk = NULL;
					if (0 == RegCreateKeyEx(HKEY_CURRENT_USER, L"Console", 0, NULL, 0, KEY_WRITE, NULL, &hk, NULL))
					{
						DWORD dwValue = 0, dwType = REG_DWORD, nSize = sizeof(DWORD);
						RegSetValueEx(hk, L"LoadConIme", 0, dwType, (LPBYTE)&dwValue, nSize);
						RegCloseKey(hk);
					}
				}

				if ((reg = gpSet->CreateSettings(NULL)) != NULL)
				{
					// БЕЗ имени конфигурации!
					if (reg->OpenKey(CONEMU_ROOT_KEY, KEY_WRITE))
					{
						long  lbStopWarning = TRUE;
						reg->Save(_T("StopWarningConIme"), lbStopWarning);
						reg->CloseKey();
					}
					delete reg;
					reg = NULL;
				}
			}

			EndDialog(hDlg, IDOK);

			return 1;
		} // IDOK

		case IDCANCEL:
		case IDCLOSE:
		{
			if (!gpSet->m_isKeyboardHooks)
				gpSet->m_isKeyboardHooks = 2; // NO

			EndDialog(hDlg, IDCANCEL);
			return 1;
		}

		case stHomePage:
			ConEmuAbout::OnInfo_FirstStartPage();
			return 1;

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
	}

	return FALSE;
}	

static INT_PTR OnLanguageChanged(HWND hDlg)
{
	CEStr lsValue;
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

			CDynDialog::LocalizeDialog(hDlg);
		}
	}
	return 0;
}

static INT_PTR CALLBACK CheckOptionsFastProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (messg)
	{
	case WM_SETHOTKEY:
		gnWndSetHotkey = wParam;
		break;

	case WM_INITDIALOG:
		return OnInitDialog(hDlg, messg, wParam, lParam);

	case WM_CTLCOLORSTATIC:
		return OnCtlColorStatic(hDlg, messg, wParam, lParam);

	case WM_SETCURSOR:
		return OnSetCursor(hDlg, messg, wParam, lParam);

	case HELP_WM_HELP:
		break;
	case WM_HELP:
		if ((wParam == 0) && (lParam != 0))
		{
			HELPINFO* hi = (HELPINFO*)lParam;
			if (hi->cbSize >= sizeof(HELPINFO))
				CEHelpPopup::OpenSettingsWiki(hDlg, hi->iCtrlId);
		}
		return TRUE;

	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case BN_CLICKED:
			return OnButtonClicked(hDlg, messg, wParam, lParam);
		case CBN_SELCHANGE:
			switch (LOWORD(wParam))
			{
			case lbColorSchemeFast:
				SendDlgItemMessage(hDlg, stPalettePreviewFast, UM_PALETTE_FAST_CHG, 0, 0);
				break;
			case lbInterfaceLanguage:
				return OnLanguageChanged(hDlg);
			}
			break;
		}

		break;

	case WM_SYSCOMMAND:
		if (wParam == SC_CLOSE)
		{
			int iQuitBtn = FastMsgBox(L"Close dialog and terminate ConEmu?", MB_ICONQUESTION|MB_YESNO, NULL, hDlg);
			if (iQuitBtn == IDYES)
				TerminateProcess(GetCurrentProcess(), CERR_FASTCONFIG_QUIT);
			return TRUE;
		}
		break;


	default:
		if (gp_DpiAware && gp_DpiAware->ProcessDpiMessages(hDlg, messg, wParam, lParam))
		{
			return TRUE;
		}
	}

	return 0;
}



/* **************************************** */
/*    Main Entry for Fast Config routine    */
/* **************************************** */

void CheckOptionsFast(LPCWSTR asTitle, SettingsLoadedFlags slfFlags)
{
	bool bFastSetupDisabled = false;
	if (gpConEmu->IsFastSetupDisabled())
	{
		bFastSetupDisabled = true;
		gpConEmu->LogString(L"CheckOptionsFast was skipped due to '/basic' or '/resetdefault' switch");
	}
	else
	{
		bVanilla = (slfFlags & slf_NeedCreateVanilla) != slf_None;

		bCheckHooks = (gpSet->m_isKeyboardHooks == 0);

		bCheckUpdate = (gpSet->UpdSet.isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Undefined);

		bCheckIme = false;
		if (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 0)
		{
			//;; Q. В Windows Vista зависают другие консольные процессы.
			//	;; A. "Виноват" процесс ConIme.exe. Вроде бы он служит для ввода иероглифов
			//	;;    (китай и т.п.). Зачем он нужен, если ввод теперь идет в графическом окне?
			//	;;    Нужно запретить его автозапуск или вообще переименовать этот файл, например
			//	;;    в 'ConIme.ex1' (видимо это возможно только в безопасном режиме).
			//	;;    Запретить автозапуск: Внесите в реестр и перезагрузитесь
			long  lbStopWarning = FALSE;

			SettingsBase* reg = gpSet->CreateSettings(NULL);
			if (reg)
			{
				// БЕЗ имени конфигурации!
				if (reg->OpenKey(CONEMU_ROOT_KEY, KEY_READ))
				{
					if (!reg->Load(_T("StopWarningConIme"), lbStopWarning))
						lbStopWarning = FALSE;

					reg->CloseKey();
				}

				delete reg;
			}

			if (!lbStopWarning)
			{
				HKEY hk = NULL;
				DWORD dwValue = 1;

				if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Console", 0, KEY_READ, &hk))
				{
					DWORD dwType = REG_DWORD, nSize = sizeof(DWORD);

					if (0 != RegQueryValueEx(hk, L"LoadConIme", 0, &dwType, (LPBYTE)&dwValue, &nSize))
						dwValue = 1;

					RegCloseKey(hk);

					if (dwValue!=0)
					{
						bCheckIme = true;
					}
				}
				else
				{
					bCheckIme = true;
				}
			}
		}
	}

	// Tasks and palettes must be created before dialog, to give user opportunity to choose startup task and palette

	// Always check, if task list is empty - fill with defaults
	CreateDefaultTasks(slfFlags);

	// Some other settings, which must be filled with predefined values
	if (slfFlags & slf_DefaultSettings)
	{
		gpSet->CreatePredefinedPalettes(0);
	}

	if (!bFastSetupDisabled && (bCheckHooks || bCheckUpdate || bCheckIme))
	{
		// First ShowWindow forced to use nCmdShow. This may be weird...
		SkipOneShowWindow();

		if (gpSetCls->IsConfigNew && gpConEmu->opt.ExitAfterActionPrm.Exists)
		{
			CEStr lsMsg(
				L"Something is going wrong...\n\n"
				L"Automatic action is pending, but settings weren't initialized!\n"
				L"\n"
				L"To avoid this problem without need to create\n"
				L"settings file you may use \"-basic\" switch.\n"
				L"\n"
				L"Current command line:\n",
				(LPCWSTR)gpConEmu->opt.cmdLine,
				L"\n\n"
				L"Do you want to continue anyway?");
			int iBtn = FastMsgBox(lsMsg, MB_ICONEXCLAMATION|MB_YESNO, NULL, NULL);
			if (iBtn == IDNO)
				TerminateProcess(GetCurrentProcess(), CERR_FASTCONFIG_QUIT);
		}

		CDpiForDialog::Create(gp_DpiAware);

		// Modal dialog (CreateDialog)

		CDynDialog::ExecuteDialog(IDD_FAST_CONFIG, NULL, CheckOptionsFastProc, (LPARAM)asTitle);

		SafeDelete(gp_DpiAware);
		ghFastCfg = NULL;
	}
}





/* **************************************** */
/*         Creating default tasks           */
/* **************************************** */

static void CreateDefaultTask(LPCWSTR asName, LPCWSTR asGuiArg, LPCWSTR asCommands, CETASKFLAGS aFlags = CETF_DONT_CHANGE)
{
	_ASSERTE(asName && asName[0] && asName[0] != TaskBracketLeft && asName[wcslen(asName)-1] != TaskBracketRight);
	wchar_t szLeft[2] = {TaskBracketLeft}, szRight[2] = {TaskBracketRight};
	CEStr lsName(szLeft, asName, szRight);

	// Don't add duplicates in the append mode
	if ((sAppendMode & slf_AppendTasks))
	{
		CommandTasks* pTask = (CommandTasks*)gpSet->CmdTaskGetByName(lsName);
		if (pTask != NULL)
		{
			if ((sAppendMode & slf_RewriteExisting))
			{
				pTask->SetGuiArg(asGuiArg);
				pTask->SetCommands(asCommands);
			}
			return;
		}
	}

	gpSet->CmdTaskSet(iCreatIdx++, lsName, asGuiArg, asCommands, aFlags);
}

struct FoundFile
{
	wchar_t* rsFound;
	wchar_t* rsOptionalFull;
};

class FoundFiles : public MArray<FoundFile>
{
public:
	FoundFiles()
	{
	}

	~FoundFiles()
	{
		for (INT_PTR i = 0; i < size(); ++i)
		{
			FoundFile& f = (*this)[i];
			SafeFree(f.rsFound);
			SafeFree(f.rsOptionalFull);
		}
	}

	void Add(const wchar_t* asFound, const wchar_t* asOptionalFull)
	{
		if (!asFound || !*asFound)
		{
			_ASSERTE(asFound && *asFound);
			return;
		}
		for (INT_PTR i = 0; i < size(); ++i)
		{
			FoundFile& f = (*this)[i];
			if ((lstrcmpi(f.rsFound, asFound) == 0)
				|| (f.rsOptionalFull && asOptionalFull && (lstrcmpi(f.rsOptionalFull, asOptionalFull) == 0)))
				return;
		}
		FoundFile ff = {lstrdup(asFound), (asOptionalFull && *asOptionalFull) ? lstrdup(asOptionalFull) : NULL};
		this->push_back(ff);
	}
};

// Search on asFirstDrive and all (other) fixed drive letters
// asFirstDrive may be letter ("C:") or network (\\server\share)
// asSearchPath is path to executable (\cygwin\bin\bash.exe)
static size_t FindOnDrives(LPCWSTR asFirstDrive, LPCWSTR asSearchPath, FoundFiles& foundFiles)
{
	_ASSERTE(foundFiles.size() == 0);
	bool bFound = false;
	wchar_t* pszExpanded = NULL;
	wchar_t szDrive[4]; // L"C:"
	CEStr szTemp;

	CEStr rsFound;

	if (!asSearchPath || !*asSearchPath)
		goto wrap;

	// Using registry path?
	if ((asSearchPath[0] == L'[') && wcschr(asSearchPath+1, L']'))
	{
		// L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1:InstallLocation]\\bin\\bash.exe",
		//   "InstallLocation"="C:\\Utils\\Lans\\GIT\\"
		CEStr lsBuf, lsVal, lsValName;
		lsBuf.Set(asSearchPath+1);
		wchar_t *pszFile = wcschr(lsBuf.ms_Val, L']');
		if (pszFile)
		{
			MArray<wchar_t*> RegFiles;

			HKEY roots[] = {HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE};
			DWORD bits[] = {KEY_WOW64_64KEY, KEY_WOW64_32KEY, 0};

			*(pszFile++) = 0;
			wchar_t* pszValName = wcsrchr(lsBuf.ms_Val, L':');
			if (pszValName)
			{
				*pszValName = 0;
				lsValName.Set(pszValName+1);

				pszValName = wcsrchr(lsBuf.ms_Val, L':');
				if (pszValName && *(pszValName - 1) == L'*' && *(pszValName - 2) == L'\\')
				{
					// #DEF_TASK L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\*:DisplayName=MSYS2 64bit:InstallLocation]\\usr\\bin\\bash.exe",
					struct SearchImpl
					{
						LPCWSTR pszValName, pszFile;
						CEStr lsCheckName, lsCheckValue;
						MArray<wchar_t*>* RegFiles;

						static bool WINAPI Enum(HKEY hk, LPCWSTR pszSubkeyName, LPARAM lParam)
						{
							SearchImpl* i = (SearchImpl*)lParam;
							CEStr lsPath;
							if (RegGetStringValue(hk, NULL, i->lsCheckName, lsPath) <= 0)
								return true;
							if (lsPath.Compare(i->lsCheckValue) != 0)
								return true;
							if (RegGetStringValue(hk, NULL, i->pszValName, lsPath) > 0)
							{
								i->RegFiles->push_back(JoinPath(lsPath, i->pszFile));
							}
							return true;
						}
					} impl;
					impl.RegFiles = &RegFiles;
					impl.pszValName = lsValName;
					impl.pszFile = pszFile;

					*(pszValName - 2) = 0;
					impl.lsCheckName.Set(++pszValName);
					pszValName = wcschr(impl.lsCheckName.ms_Val, L'=');
					if (pszValName)
					{
						*pszValName = 0;
						impl.lsCheckValue.Set(++pszValName);

						for (size_t r = 0; r < countof(roots); ++r)
						{
							RegEnumKeys(roots[r], lsBuf, SearchImpl::Enum, (LPARAM)&impl);
						}
					}
				}
				else
				{
					// Evaluate HKLM, HKCU, 32bit and 64bit in all variants
					for (size_t r = 0; r < countof(roots); ++r)
					{
						for (size_t b = IsWindows64() ? 0 : 1; b < countof(bits); ++b)
						{
							if (RegGetStringValue(roots[r], lsBuf, lsValName, lsVal, bits[b]) > 0)
							{
								RegFiles.push_back(JoinPath(lsVal, pszFile));
							}
						}
					}
				}
			}

			// When keys population is done
			for (INT_PTR i = 0; i < RegFiles.size(); ++i)
			{
				rsFound.Attach(STD_MOVE(RegFiles[i]));
				if (FileExists(rsFound))
				{
					foundFiles.Add(rsFound, NULL);
					bFound = true;
				}
			}
		}
		goto wrap;
	}

	// Using environment variables?
	if (wcschr(asSearchPath, L'%'))
	{
		pszExpanded = ExpandEnvStr(asSearchPath);
		if (pszExpanded && FileExists(pszExpanded))
		{
			foundFiles.Add(asSearchPath, pszExpanded);
			bFound = true;
		}
		goto wrap;
	}

	// Only executable name was specified?
	if (!wcschr(asSearchPath, L'\\'))
	{
		if (apiSearchPath(NULL, asSearchPath, NULL, szTemp))
		{
			// OK, create task with just a name of exe file
			foundFiles.Add(asSearchPath, szTemp);
			bFound = true;
		}
		// Search in [HKCU|HKLM]\Software\Microsoft\Windows\CurrentVersion\App Paths
		else if (SearchAppPaths(asSearchPath, rsFound, false/*abSetPath*/))
		{
			// If app exists in "App Paths" we don't need to store its full path
			foundFiles.Add(asSearchPath, rsFound);
			bFound = true;
		}
		goto wrap;
	}

	// Full path was specified? Check it.
	if (IsFilePath(asSearchPath, true)
		&& FileExists(asSearchPath))
	{
		foundFiles.Add(asSearchPath, NULL);
		bFound = true;
		goto wrap;
	}

	// ConEmu's drive
	if (asFirstDrive && *asFirstDrive)
	{
		INT_PTR nDrvLen = _tcslen(asFirstDrive);
		rsFound.Attach(JoinPath(asFirstDrive, asSearchPath));
		if (FileExists(rsFound))
		{
			foundFiles.Add(rsFound, NULL);
			bFound = true;
			goto wrap;
		}
	}

	szDrive[1] = L':'; szDrive[2] = 0;
	for (szDrive[0] = L'C'; szDrive[0] <= L'Z'; szDrive[0]++)
	{
		if ((asFirstDrive && *asFirstDrive) && (lstrcmpi(szDrive, asFirstDrive) == 0))
			continue;
		UINT nType = GetDriveType(szDrive);
		if (nType != DRIVE_FIXED)
			continue;
		rsFound.Attach(JoinPath(szDrive, asSearchPath));
		if (FileExists(rsFound))
		{
			foundFiles.Add(rsFound, NULL);
			bFound = true;
			goto wrap;
		}
	}

wrap:
	SafeFree(pszExpanded);
	_ASSERTE(bFound == (foundFiles.size() != 0));
	return foundFiles.size();
}

class CVarDefs
{
public:
	struct VarDef
	{
		wchar_t* pszName;
		wchar_t* pszValue;
	};
	MArray<VarDef> Vars;

	void Store(wchar_t* asName, wchar_t* psValue)
	{
		if (!asName || !*asName || !psValue || !*psValue)
		{
			_ASSERTE(asName && *asName && psValue && *psValue);
			return;
		}

		VarDef v = {asName, psValue};
		Vars.push_back(v);
	};

	void Process(int nBackSteps, LPCWSTR asName)
	{
		wchar_t szName[80] = L"%"; wcscat_c(szName, asName); wcscat_c(szName, L"%");
		wchar_t* psVal = GetEnvVar(asName);
		while (psVal && *psVal)
		{
			wchar_t* pszSlash = wcsrchr(psVal, L'\\');
			while (pszSlash && (*(pszSlash+1) == 0))
			{
				_ASSERTE(*(pszSlash+1) != 0 && "Must not be the trailing slash!");
				*pszSlash = 0;
				pszSlash = wcsrchr(psVal, L'\\');
			}

			Store(lstrdup(szName), lstrdup(psVal));

			if ((--nBackSteps) < 0)
				break;
			if (!pszSlash)
				break;
			*pszSlash = 0;
			if (!wcsrchr(psVal, L'\\'))
				break;
			wcscat_c(szName, L"\\..");
		}
		SafeFree(psVal);
	}

	CVarDefs()
	{
		Process(0, L"ConEmuBaseDir");
		Process(3, L"ConEmuDir");
		Process(0, L"WinDir");
		Process(0, L"ConEmuDrive");
	};

	~CVarDefs()
	{
		VarDef v = {};
		while (Vars.pop_back(v))
		{
			SafeFree(v.pszName);
			SafeFree(v.pszValue);
		}
	};
};

static CVarDefs *spVars = NULL;

static bool UnExpandEnvStrings(LPCWSTR asSource, wchar_t* rsUnExpanded, INT_PTR cchMax)
{
	// Don't use PathUnExpandEnvStrings because it uses %SystemDrive% instead of %ConEmuDrive%,
	// and %ProgramFiles% but it may fail on 64-bit OS due to bitness differences
	// - if (UnExpandEnvStrings(szFound, szUnexpand, countof(szUnexpand)) && (lstrcmp(szFound, szUnexpand) != 0)) ;
	if (!spVars)
	{
		_ASSERTE(spVars != NULL);
		return false;
	}

	if (!IsFilePath(asSource, true))
		return false;

	CEStr szTemp((LPCWSTR)asSource);
	wchar_t* ptrSrc = szTemp.ms_Val;
	if (!ptrSrc)
		return false;
	int iCmpLen, iCmp, iLen = lstrlen(ptrSrc);

	for (INT_PTR i = 0; i < spVars->Vars.size(); i++)
	{
		CVarDefs::VarDef& v = spVars->Vars[i];
		iCmpLen = lstrlen(v.pszValue);
		if ((iCmpLen >= iLen) || !wcschr(L"/\\", ptrSrc[iCmpLen]))
			continue;

		wchar_t c = ptrSrc[iCmpLen]; ptrSrc[iCmpLen] = 0;
		iCmp = lstrcmpi(ptrSrc, v.pszValue);
		ptrSrc[iCmpLen] = c;

		if (iCmp == 0)
		{
			if (!szTemp.Attach(lstrmerge(v.pszName, asSource+iCmpLen)))
				return false;
			iLen = lstrlen(szTemp);
			if (iLen > cchMax)
				return false;
			_wcscpy_c(rsUnExpanded, cchMax, szTemp);
			return true;
		}
	}

	return false;
}

class AppFoundList
{
public:
	struct AppInfo
	{
		LPWSTR szFullPath, szExpanded;
		wchar_t szTaskName[64], szTaskBaseName[40];
		LPWSTR szArgs, szPrefix, szGuiArg;
		VS_FIXEDFILEINFO Ver; // bool LoadAppVersion(LPCWSTR FarPath, VS_FIXEDFILEINFO& Version, wchar_t (&ErrText)[512])
		DWORD dwSubsystem, dwBits;
		FarVersion FarVer; // ConvertVersionToFarVersion
		int  iStep;
		bool bForceQuot;
		bool isNeedQuot() const { return bForceQuot || IsQuotationNeeded(szFullPath); };
		bool bPrimary; // Do not rename this task while unifying
		void Free()
		{
			SafeFree(szFullPath);
			SafeFree(szExpanded);
			SafeFree(szArgs);
			SafeFree(szPrefix);
			SafeFree(szGuiArg);
		};
	};
	MArray<AppInfo> Installed;

	int mn_MaxFoundInstances;

protected:
	// This will load App version and check if it was already added
	virtual INT_PTR AddAppPath(LPCWSTR asName, LPCWSTR szPath, LPCWSTR pszOptFull, bool bForceQuot,
		LPCWSTR asArgs = NULL, LPCWSTR asPrefix = NULL, LPCWSTR asGuiArg = NULL)
	{
		INT_PTR iAdded = -1;
		AppInfo FI = {};
		wchar_t ErrText[512];
		DWORD FileAttrs = 0;
		_ASSERTE(!pszOptFull || *pszOptFull);
		LPCWSTR pszPath = pszOptFull ? pszOptFull : szPath;

		// Use GetImageSubsystem as condition because many exe-s may not have VersionInfo at all
		if (GetImageSubsystem(pszPath, FI.dwSubsystem, FI.dwBits, FileAttrs))
		{
			if (FI.dwSubsystem && FI.dwSubsystem <= IMAGE_SUBSYSTEM_WINDOWS_CUI)
				LoadAppVersion(pszPath, FI.Ver, ErrText);
			else
				ZeroStruct(FI.Ver);

			// App instance found, add it to Installed array?
			bool bAlready = false;
			for (INT_PTR a = 0; a < Installed.size(); a++)
			{
				AppInfo& ai = Installed[a];
				bool path_match = false;
				if (lstrcmpi(ai.szFullPath, szPath) == 0)
				{
					path_match = true;
				}
				else if (pszOptFull && (lstrcmpi(ai.szFullPath, pszOptFull) == 0))
				{
					// Store path with environment variables (unexpanded) or without path at all (just "Far.exe" for example)
					SafeFree(ai.szFullPath);
					ai.szFullPath = lstrdup(szPath);
					path_match = true;
				}
				else if (pszOptFull && ai.szExpanded && (lstrcmpi(ai.szExpanded, pszOptFull) == 0))
				{
					path_match = true;
				}
				// Do not add twice same path + args
				if (path_match
						&& (lstrcmp(ai.szArgs ? ai.szArgs : L"", asArgs ? asArgs : L"") == 0))
				{
					bAlready = true; break;
				}
			}
			// New instance, add it
			if (!bAlready)
			{
				lstrcpyn(FI.szTaskName, asName, countof(FI.szTaskName));
				lstrcpyn(FI.szTaskBaseName, asName, countof(FI.szTaskBaseName));
				FI.szFullPath = lstrdup(szPath);
				FI.szExpanded = pszOptFull ? lstrdup(pszOptFull) : ExpandEnvStr(szPath);
				FI.bForceQuot = bForceQuot;
				FI.szArgs = asArgs ? lstrdup(asArgs) : NULL;
				FI.szPrefix = asPrefix ? lstrdup(asPrefix) : NULL;
				FI.szGuiArg = asGuiArg ? lstrdup(asGuiArg) : NULL;
				if (FI.szFullPath)
				{
					iAdded = Installed.push_back(FI);
				}
			}
		}

		return iAdded;
	}; // AddAppPath(LPCWSTR szPath)

	void Clean()
	{
		for (INT_PTR i = 0; i < Installed.size(); i++)
		{
			AppInfo& FI = Installed[i];
			FI.Free();
		}

		Installed.clear();
	}

	bool Trim()
	{
		bool bLimit = ((mn_MaxFoundInstances > 0) && (Installed.size() >= mn_MaxFoundInstances));
		if (bLimit)
		{
			for (INT_PTR j = Installed.size()-1; j >= mn_MaxFoundInstances; j--)
			{
				Installed.erase(j);
			}
		}
		return bLimit;
	}

public:
	// asPrefix - some prefix before the command line, e.g. "set \"PATH=%ConEmuBaseDirShort%\\wsl;%PATH%\" & "
	bool Add(LPCWSTR asName, LPCWSTR asArgs, LPCWSTR asPrefix, LPCWSTR asGuiArg, LPCWSTR asExePath, ...)
	{
		bool bCreated = false;
		va_list argptr;
		va_start(argptr, asExePath);
		CEStr szArgs;
		wchar_t szUnexpand[MAX_PATH+32];

		LPCWSTR pszExePathNext = asExePath;
		while (pszExePathNext)
		{
			LPCWSTR pszExePath = pszExePathNext;
			pszExePathNext = va_arg( argptr, LPCWSTR );

			// Return expanded env string
			FoundFiles files;
			if (!FindOnDrives(szConEmuDrive, pszExePath, files))
				continue;
			for (INT_PTR i = 0; i < files.size(); ++i)
			{
				FoundFile& f = files[i];
				LPCWSTR szFound = f.rsFound;
				LPCWSTR szOptFull = f.rsOptionalFull;

				LPCWSTR pszFound = szFound;
				// Don't use PathUnExpandEnvStrings because it do not do what we need
				if (UnExpandEnvStrings(szFound, szUnexpand, countof(szUnexpand)) && (lstrcmp(szFound, szUnexpand) != 0))
				{
					pszFound = szUnexpand;
				}

				if (AddAppPath(asName, pszFound, (szOptFull && *szOptFull) ? szOptFull : szFound, false, asArgs, asPrefix, asGuiArg) >= 0)
				{
					bCreated = true;

					if (Trim())
					{
						break;
					}
				}
			}
		}

		va_end(argptr);

		return bCreated;
	}

	bool CheckUnique(LPCWSTR pszTaskBaseName)
	{
		bool bUnique = true;

		for (INT_PTR i = 0; i < Installed.size(); i++)
		{
			const AppInfo& FI = Installed[i];
			if (pszTaskBaseName && (lstrcmpi(pszTaskBaseName, FI.szTaskBaseName) != 0))
				continue;
			for (INT_PTR j = Installed.size() - 1; j > i; j--)
			{
				const AppInfo& FJ = Installed[j];
				if (pszTaskBaseName && (lstrcmpi(pszTaskBaseName, FJ.szTaskBaseName) != 0))
					continue;
				if (lstrcmpi(FI.szTaskName, FJ.szTaskName) == 0)
				{
					bUnique = false; break;
				}
			}
		}

		return bUnique;
	}

	virtual void MakeUnique()
	{
		if (Installed.size() <= 1)
			return;

		// Ensure task names are unique
		UINT idx = 0;

		struct impl {
			static bool SortRoutine(const AppInfo& e1, const AppInfo& e2)
			{
				// Compare task base name
				int iNameCmp = lstrcmpi(e1.szTaskBaseName, e2.szTaskBaseName);
				if (iNameCmp)
					return (iNameCmp < 0);

				// Primary task - to top
				if (e1.bPrimary && !e2.bPrimary)
					return true;
				else if (e2.bPrimary && !e1.bPrimary)
					return false;
				#ifdef _DEBUG
				else if (e1.bPrimary && e2.bPrimary)
					_ASSERTE(!e1.bPrimary || !e2.bPrimary); // Two primary tasks are not allowed!
				#endif

				// Compare exe version
				if (e1.Ver.dwFileVersionMS < e2.Ver.dwFileVersionMS)
					return false;
				if (e1.Ver.dwFileVersionMS > e2.Ver.dwFileVersionMS)
					return true;
				if (e1.Ver.dwFileVersionLS < e2.Ver.dwFileVersionLS)
					return false;
				if (e1.Ver.dwFileVersionLS > e2.Ver.dwFileVersionLS)
					return true;
				// And bitness
				if (e1.dwBits < e2.dwBits)
					return false;
				if (e1.dwBits > e2.dwBits)
					return true;

				// Equal?
				return false;
			};
		};
		Installed.sort(impl::SortRoutine);

		// To know if the task was already processed by task-base-name
		for (INT_PTR i = 0; i < Installed.size(); i++)
		{
			Installed[i].iStep = 0;
		}

		// All task names MUST be unique
		for (int u = 1; u <= 3; u++)
		{
			// Firstly check if all task names are already unique
			if (CheckUnique(NULL))
			{
				break; // Done, all task names are unique already
			}

			// Now we have to modify task-name by adding some unique suffix to the task-base-name
			for (INT_PTR i = 0; i < Installed.size(); i++)
			{
				const AppInfo& FI = Installed[i];
				if (FI.iStep == u)
					continue; // Already processed

				// Do we need to make this task-base-name unique?
				if (CheckUnique(FI.szTaskBaseName))
				{
					for (INT_PTR j = Installed.size() - 1; j >= i; j--)
					{
						AppInfo& FJ = Installed[j];
						if (lstrcmpi(FI.szTaskBaseName, FJ.szTaskBaseName) != 0)
							continue;
						FJ.iStep = u;
					}
					continue; // Don't
				}

				bool bMatch = false;

				for (INT_PTR j = i; j < Installed.size(); j++)
				{
					AppInfo& FJ = Installed[j];
					if (FJ.iStep == u)
						continue; // Already processed
					if (FJ.bPrimary)
					{
						// Don't change primary task name
						continue;
					}

					// Check only tasks with the same base names
					if (lstrcmpi(FI.szTaskBaseName, FJ.szTaskBaseName) != 0)
						continue;
					bMatch = true;

					wchar_t szPlatform[6]; wcscpy_c(szPlatform, (FJ.dwBits == 64) ? L" x64" : (FJ.dwBits == 32) ? L" x86" : L"");

					switch (u)
					{
					case 1: // Naked, only add platform
						swprintf_c(FJ.szTaskName, countof(FJ.szTaskName)-16/*#SECURELEN*/, L"%s%s",
							FJ.szTaskBaseName, szPlatform);
						break;

					case 2: // Add App version and platform
						if (FJ.Ver.dwFileVersionMS)
							swprintf_c(FJ.szTaskName, countof(FJ.szTaskName)-16/*#SECURELEN*/, L"%s %u.%u%s",
								FJ.szTaskBaseName, HIWORD(FJ.Ver.dwFileVersionMS), LOWORD(FJ.Ver.dwFileVersionMS), szPlatform);
						else // If there was not VersionInfo in the exe file (same as u==1)
							swprintf_c(FJ.szTaskName, countof(FJ.szTaskName)-16/*#SECURELEN*/, L"%s%s",
								FJ.szTaskBaseName, szPlatform);
						break;

					case 3: // Add App version, platform and index
						if (FJ.Ver.dwFileVersionMS)
							swprintf_c(FJ.szTaskName, countof(FJ.szTaskName)-16/*#SECURELEN*/, L"%s %u.%u%s (%u)",
								FJ.szTaskBaseName, HIWORD(FJ.Ver.dwFileVersionMS), LOWORD(FJ.Ver.dwFileVersionMS), szPlatform, ++idx);
						else // If there was not VersionInfo in the exe file
							swprintf_c(FJ.szTaskName, countof(FI.szTaskName)-16/*#SECURELEN*/, L"%s%s (%u)",
								FI.szTaskBaseName, szPlatform, ++idx);
						break;
					}

					// To know the task was processed
					FJ.iStep = u;
				}
			}

			if (CheckUnique(NULL))
			{
				break; // Done, all task names are unique
			}
		}
	}

	virtual bool Commit()
	{
		if (Installed.size() <= 0)
			return false;

		bool bCreated = false;

		// If limit for instance count was set
		Trim();

		// All task names MUST be unique
		MakeUnique();

		// Add them all
		for (INT_PTR i = 0; i < Installed.size(); i++)
		{
			CEStr szFull, szArgs;
			const AppInfo& ai = Installed[i];

			// FOUND_APP_PATH_CHR mark is used generally for locating ico files
			LPCWSTR pszArgs = ai.szArgs;
			if (pszArgs && wcschr(pszArgs, FOUND_APP_PATH_CHR))
			{
				if (ai.szFullPath && *ai.szFullPath && szArgs.Set(pszArgs))
				{
					CEStr szPath;
					wchar_t *ptrFound, *ptrAdd;
					while ((ptrAdd = wcschr(szArgs.ms_Val, FOUND_APP_PATH_CHR)) != NULL)
					{
						*ptrAdd = 0;
						LPCWSTR pszTail = ptrAdd+1;

						szPath.Set(ai.szFullPath);
						ptrFound = wcsrchr(szPath.ms_Val, L'\\');
						if (ptrFound) *ptrFound = 0;

						if (*pszTail == L'\\') pszTail ++;
						while (wcsncmp(pszTail, L"..\\", 3) == 0)
						{
							ptrAdd = wcsrchr(szPath.ms_Val, L'\\');
							if (!ptrAdd)
								break;
							// szPath is a local copy, safe to change it
							*ptrAdd = 0;
							pszTail += 3;
						}

						CEStr szTemp(JoinPath(szPath, pszTail));
						szArgs.Attach(lstrmerge(szArgs.ms_Val, szTemp));
					}
				}
				// Succeeded?
				if (!szArgs.IsEmpty())
				{
					pszArgs = szArgs.ms_Val;
				}
			}

			// Spaces in path? (use expanded path)
			if (ai.isNeedQuot())
				szFull = lstrmerge(ai.szPrefix, L"\"", ai.szFullPath, L"\"", pszArgs);
			else
				szFull = lstrmerge(ai.szPrefix, ai.szFullPath, pszArgs);

			// Create task
			if (!szFull.IsEmpty())
			{
				CreateDefaultTask(ai.szTaskName, ai.szGuiArg, szFull);
			}
		}

		Clean();

		return bCreated;
	};

public:
	AppFoundList(int anMaxFoundInstances = -1)
		: mn_MaxFoundInstances(anMaxFoundInstances)
	{
	};

	virtual ~AppFoundList()
	{
		Clean();
	};
};

class FarVerList : public AppFoundList
{
protected:
	wchar_t szFar32Name[16], szFar64Name[16];
	LPCWSTR FarExe[3]; // = { szFar64Name, szFar32Name, NULL };

protected:
	void ScanRegistry()
	{
		LPCWSTR Locations[] = {
			L"Software\\Far Manager",
			L"Software\\Far2",
			L"Software\\Far",
			NULL
		};
		LPCWSTR Names[] = {
			L"InstallDir_x64",
			L"InstallDir",
			NULL
		};

		int wow1, wow2;
		if (IsWindows64())
		{
			wow1 = 1; wow2 = 2;
		}
		else
		{
			wow1 = wow2 = 0;
		}

		for (int hk = 0; hk <= 1; hk++)
		{
			for (int loc = 0; Locations[loc]; loc++)
			{
				for (int nam = 0; Names[nam]; nam++)
				{
					for (int wow = wow1; wow <= wow2; wow++)
					{
						CEStr szKeyValue;
						HKEY hkParent = (hk == 0) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
						DWORD Wow64Flags = (wow == 0) ? 0 : (wow == 1) ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;
						if (RegGetStringValue(hkParent, Locations[loc], Names[nam], szKeyValue, Wow64Flags) > 0)
						{
							for (int fe = 0; FarExe[fe]; fe++)
							{
								CEStr szPath(JoinPath(szKeyValue, FarExe[fe]));
								// This will load Far version and check its existence
								AddAppPath(L"Far", szPath, NULL, true);
							}
						}
					}
				}
			}
		}
	}; // ScanRegistry()

public:
	void FindInstalledVersions()
	{
		CEStr szFound, szOptFull;
		bool bNeedQuot = false;
		INT_PTR i;
		wchar_t ErrText[512];

		const wchar_t szFarPrefix[] = L"Far Manager::";

		// Scan our program dir subfolders
		for (i = 0; FarExe[i]; i++)
		{
			if (FileExistSubDir(gpConEmu->ms_ConEmuExeDir, FarExe[i], 1, szFound))
				AddAppPath(L"Far", szFound, NULL, true);
		}

		// If Far was copied inside our (ConEmu) folder,
		// just leave far.exe found in our subdir as {Far}
		// Let portable installation (probably for testing) be friendly
		if (Installed.size() > 0)
		{
			Installed[0].bPrimary = true;
		}

		// Check registry
		ScanRegistry();

		// Find in %Path% and on drives
		for (i = 0; FarExe[i]; i++)
		{
			FoundFiles files;
			if (FindOnDrives(szConEmuDrive, FarExe[i], files))
			{
				for (INT_PTR i = 0; i < files.size(); ++i)
				{
					const FoundFile& f = files[i];
					AddAppPath(L"Far", f.rsFound, f.rsOptionalFull, true);
				}
			}
		}

		// [HKCU|HKLM]\Software\Microsoft\Windows\CurrentVersion\App Paths
		for (i = 0; FarExe[i]; i++)
		{
			if (SearchAppPaths(FarExe[i], szFound, false))
				AddAppPath(L"Far", szFound, NULL, true);
		}

		for (i = 0; i < Installed.size(); i++)
		{
			AppInfo& FI = Installed[i];
			if (LoadAppVersion(FI.szExpanded, FI.Ver, ErrText))
				ConvertVersionToFarVersion(FI.Ver, FI.FarVer);
			else
				SetDefaultFarVersion(FI.FarVer);
		}

		// Done, create task names
		// If there is only one found instance - just use name {Far}
		if (Installed.size() > 1)
		{
			UINT idx = 0;
			LPCWSTR pszPrefix = (Installed.size() > 1) ? szFarPrefix : L"";

			struct impl {
				static bool SortRoutine(const AppInfo& e1, const AppInfo& e2)
				{
					// Primary task - to top
					if (e1.bPrimary && !e2.bPrimary)
						return true;
					else if (e2.bPrimary && !e1.bPrimary)
						return false;
					#ifdef _DEBUG
					else if (e1.bPrimary && e2.bPrimary)
						_ASSERTE(!e1.bPrimary || !e2.bPrimary); // Two primary tasks are not allowed!
					#endif

					// Compare exe version
					if (e1.FarVer.dwVer < e2.FarVer.dwVer)
						return false;
					if (e1.FarVer.dwVer > e2.FarVer.dwVer)
						return true;
					if (e1.FarVer.dwBuild < e2.FarVer.dwBuild)
						return false;
					if (e1.FarVer.dwBuild > e2.FarVer.dwBuild)
						return true;
					if (e1.dwBits < e2.dwBits)
						return false;
					if (e1.dwBits > e2.dwBits)
						return true;

					// Equal?
					return false;
				};
			};
			Installed.sort(impl::SortRoutine);

			// All task names MUST be unique
			for (int u = 0; u <= 2; u++)
			{
				bool bUnique = true;

				for (i = 0; i < Installed.size(); i++)
				{
					AppInfo& FI = Installed[i];
					if (FI.bPrimary)
					{
						// Don't change name of primary task, add prefix only
						wcscpy_c(FI.szTaskName, pszPrefix);
						wcscat_c(FI.szTaskName, L"Far");
						continue;
					}

					wchar_t szPlatform[6]; wcscpy_c(szPlatform, (FI.dwBits == 64) ? L" x64" : (FI.dwBits == 32) ? L" x86" : L"");

					switch (u)
					{
					case 0: // Naked
						swprintf_c(FI.szTaskName, countof(FI.szTaskName)-16/*#SECURELEN*/, L"%sFar %u.%u%s",
							pszPrefix, FI.FarVer.dwVerMajor, FI.FarVer.dwVerMinor, szPlatform);
						break;
					case 1: // Add Far Build no?
						swprintf_c(FI.szTaskName, countof(FI.szTaskName)-16/*#SECURELEN*/, L"%sFar %u.%u.%u%s",
							pszPrefix, FI.FarVer.dwVerMajor, FI.FarVer.dwVerMinor, FI.FarVer.dwBuild, szPlatform);
						break;
					case 2: // Add Build and index
						swprintf_c(FI.szTaskName, countof(FI.szTaskName)-16/*#SECURELEN*/, L"%sFar %u.%u.%u%s (%u)",
							pszPrefix, FI.FarVer.dwVerMajor, FI.FarVer.dwVerMinor, FI.FarVer.dwBuild, szPlatform, ++idx);
						break;
					}

					for (INT_PTR j = 0; j < i; j++)
					{
						if (lstrcmpi(FI.szTaskName, Installed[j].szTaskName) == 0)
						{
							bUnique = false; break;
						}
					}

					if (!bUnique)
						break;
				}

				if (bUnique)
					break;
			}
		}
	};

public:
	FarVerList()
	{
		wcscpy_c(szFar32Name, L"far.exe");
		wcscpy_c(szFar64Name, L"far64.exe");
		INT_PTR i = 0;
		if (IsWindows64())
			FarExe[i++] = szFar64Name;
		FarExe[i++] = szFar32Name;
		FarExe[i] = NULL;
	};

	~FarVerList() {};
};

static void CreateFarTasks()
{
	FarVerList Vers;
	Vers.FindInstalledVersions();

	// Create Far tasks
	for (INT_PTR i = 0; i < Vers.Installed.size(); i++)
	{
		FarVerList::AppInfo& FI = Vers.Installed[i];
		bool bNeedQuot = (wcschr(FI.szFullPath, L' ') != NULL);
		LPWSTR pszFullPath = FI.szFullPath;
		wchar_t szUnexpanded[MAX_PATH];
		if (wcschr(pszFullPath, L'\\') && UnExpandEnvStrings(pszFullPath, szUnexpanded, countof(szUnexpanded)))
			pszFullPath = szUnexpanded;

		// Reset 'FARHOME' env.var before starting far.exe!
		// Otherwise, we may inherit '%FARHOME%' from parent process and when far.exe starts
		// it will get already expanded command line which may have erroneous path.
		// That's very bad when running x64 Far, but %FARHOME% points to x86 Far.
		// And don't preset FARHOME variable, it makes harder to find Tab icon.
		wchar_t* pszCommand = lstrmerge(L"set \"FARHOME=\" & \"", pszFullPath, L"\"");

		if (pszCommand)
		{
			if (FI.FarVer.dwVerMajor >= 2)
				lstrmerge(&pszCommand, L" /w");

			// Don't duplicate plugin folders (ConEmu) to avoid doubled lines in F11 (Far 1.x and Far 2.x problem)
			bool bDontDuplicate = false;
			if (FI.FarVer.dwVerMajor <= 2)
			{
				// .szExpanded is expected to be full path,
				// but .szFullPath may be even a "far.exe", if it exists in ConEmu folder
				LPCWSTR pszFarPath = FI.szExpanded ? FI.szExpanded : FI.szFullPath;
				LPCWSTR pszFarExeName = PointToName(pszFarPath);
				if (pszFarExeName && (pszFarExeName > pszFarPath))
				{
					CEStr lsFarPath; lsFarPath.Set(pszFarPath, (pszFarExeName - pszFarPath) - 1);
					_ASSERTE(lsFarPath.GetLen() > 0);
					int iCmp = lstrcmpi(gpConEmu->ms_ConEmuExeDir, lsFarPath);
					if (iCmp == 0)
					{
						bDontDuplicate = true;
					}
				}
			}

			// Force Far to use proper plugins folders
			if (!bDontDuplicate)
				lstrmerge(&pszCommand, L" /p\"%ConEmuDir%\\Plugins\\ConEmu;%FARHOME%\\Plugins;%FARPROFILE%\\Plugins\"");


			// Suggest this task as ConEmu startup default
			if (gn_FirstFarTask == -1)
				gn_FirstFarTask = iCreatIdx;

			CreateDefaultTask(FI.szTaskName, NULL, pszCommand);
		}

		// Release memory
		SafeFree(FI.szFullPath);
		SafeFree(pszCommand);
	}
	Vers.Installed.clear();
}

static void CreateTccTasks()
{
	ConEmuComspec tcc = {}; tcc.csType = cst_AutoTccCmd;
	FindComspec(&tcc, false/*bCmdAlso*/);
	bool bTccFound = false;

	LPCWSTR pszTcc = NULL, pszTcc64 = NULL;

	// Comspec may be "cmd.exe" or "tcc.exe", check it
	if (tcc.Comspec32[0] && (lstrcmpi(PointToName(tcc.Comspec32), L"tcc.exe") == 0))
	{
		pszTcc = tcc.Comspec32;
	}
	// It's possible that both x86 & x64 versions are found
	if (tcc.Comspec64[0] && (lstrcmpi(PointToName(tcc.Comspec64), L"tcc.exe") == 0))
	{
		if (tcc.Comspec32[0] && (lstrcmpi(tcc.Comspec32, tcc.Comspec64) != 0))
			pszTcc64 = tcc.Comspec64;
		else if (!pszTcc)
			pszTcc = tcc.Comspec64;
	}
	// Not found? Last chance
	if (!pszTcc) pszTcc = L"tcc.exe";

	AppFoundList App;

	// Add tasks
	App.Add(L"Shells::TCC", NULL, NULL, NULL, pszTcc, NULL);
	App.Add(L"Shells::TCC (Admin)", L" -new_console:a", NULL, NULL, pszTcc, NULL);
	App.Commit();

	// separate x64 version?
	if (pszTcc64)
	{
		App.Add(L"Shells::TCC x64", NULL, NULL, NULL, pszTcc64, NULL);
		App.Add(L"Shells::TCC x64 (Admin)", L" -new_console:a", NULL, NULL, pszTcc64, NULL);
		App.Commit();
	}
}

// Windows SDK
static bool WINAPI CreateWinSdkTasks(HKEY hkVer, LPCWSTR pszVer, LPARAM lParam)
{
	CEStr pszVerPath;

	if (RegGetStringValue(hkVer, NULL, L"InstallationFolder", pszVerPath) > 0)
	{
		CEStr szCmd(JoinPath(pszVerPath, L"Bin\\SetEnv.Cmd"));
		if (szCmd && FileExists(szCmd))
		{
			CEStr szIcon(JoinPath(pszVerPath, L"Setup\\setup.ico"));
			CEStr szArgs(L"-new_console:t:\"WinSDK ", pszVer, L"\":C:\"", szIcon, L"\"");
			CEStr szFull(L"cmd /V /K ", szArgs, L" \"", szCmd, L"\"");
			// Create task
			if (szFull)
			{
				CEStr szName(L"SDK::WinSDK ", pszVer);
				if (szName)
				{
					SettingsLoadedFlags old = sAppendMode;
					if (!(sAppendMode & slf_AppendTasks))
						sAppendMode = (slf_AppendTasks|slf_RewriteExisting);

					CreateDefaultTask(szName, L"", szFull);

					sAppendMode = old;
				}
			}
		}
	}

	return true; // continue reg enum
}

// Visual Studio C++
static void CreateVCTask(AppFoundList& App, LPCWSTR pszPlatform, LPCWSTR pszVer, LPCWSTR pszDir)
{
	// "12.0" = "C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\"
	// %comspec% /k ""C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"" x86

	// "15.0" = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Professional\\"
	// --> ...\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat [x86|x64]
	// --> ...\2017\Professional\VC\Auxiliary\Build\vcvars32.bat
	// --> ...\2017\Professional\VC\Auxiliary\Build\vcvars64.bat
	// --> ...\2017\Professional\VC\Auxiliary\Build\vcvarsamd64_x86.bat
	// --> ...\2017\Professional\VC\Auxiliary\Build\vcvarsx86_amd64.bat


	CEStr pszVcVarsBat;
	for (int i = 0;; ++i)
	{
		switch (i)
		{
		case 0:
			pszVcVarsBat.Attach(JoinPath(pszDir, L"vcvarsall.bat"));
			break;
		case 1:
			pszVcVarsBat.Attach(JoinPath(pszDir, L"VC\\Auxiliary\\Build\\vcvarsall.bat"));
			break;
		default:
			return;
		}

		if (FileExists(pszVcVarsBat))
			break;
	}

	int iVer = wcstol(pszVer, NULL, 10);
	CEStr pszPrefix(L"cmd /k \"");
	CEStr pszSuffix(L"-new_console:t:\"VS ", pszVer, L"\"");

	CEStr lsIcon; LPCWSTR pszIconSource = NULL;
	LPCWSTR pszIconSources[] = {
		L"%CommonProgramFiles(x86)%\\microsoft shared\\MSEnv\\VSFileHandler.dll",
		L"%CommonProgramFiles%\\microsoft shared\\MSEnv\\VSFileHandler.dll",
		NULL};
	for (int i = 0; pszIconSources[i]; i++)
	{
		if (lsIcon.Attach(ExpandEnvStr(pszIconSources[i]))
			&& FileExists(lsIcon))
		{
			pszIconSource = pszIconSources[i];
			break;
		}
	}

	if (iVer && pszIconSource)
	{
		LPCWSTR pszIconSfx;
		switch (iVer)
		{
		case 16: pszIconSfx = L",43\""; break;
		case 15: pszIconSfx = L",38\""; break;
		case 14: pszIconSfx = L",33\""; break;
		case 12: pszIconSfx = L",28\""; break;
		case 11: pszIconSfx = L",23\""; break;
		case 10: pszIconSfx = L",16\""; break;
		case 9:  pszIconSfx = L",10\""; break;
		default: pszIconSfx = L"\"";
		}
		lstrmerge(&pszSuffix.ms_Val, L" -new_console:C:\"", pszIconSource, pszIconSfx);
	}

	CEStr pszName(L"SDK::VS ", pszVer, L" ", pszPlatform, L" tools prompt");
	CEStr pszSuffixReady(L"\" ", pszPlatform, L" ", pszSuffix);
	App.Add(pszName, pszSuffixReady, pszPrefix, NULL/*asGuiArg*/, pszVcVarsBat, NULL);
}

// Visual Studio C++
static bool WINAPI CreateVCTasks(HKEY hkVer, LPCWSTR pszVer, LPARAM lParam)
{
	AppFoundList *App = (AppFoundList*)lParam;

	//[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\11.0\Setup\VC]
	//"ProductDir"="C:\\Program Files (x86)\\Microsoft Visual Studio 11.0\\VC\\"
	//[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\12.0\Setup\VC]
	//"ProductDir"="C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\"
	// %comspec% /k ""C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"" x86

	if (wcschr(pszVer, L'.') && isDigit(*pszVer))
	{
		CEStr pszDir;
		if (RegGetStringValue(hkVer, L"Setup\\VC", L"ProductDir", pszDir) > 0)
		{
			CreateVCTask(App[0], L"x86", pszVer, pszDir);
			CreateVCTask(App[1], L"x64", pszVer, pszDir);
		}
	}

	return true; // continue reg enum
}

static bool WINAPI CreateVCTasks(HKEY hkVS, LPCWSTR pszVer, DWORD dwType, LPARAM lParam)
{
	AppFoundList *App = (AppFoundList*)lParam;

	//[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\SxS\VS7]
	//"12.0"="C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\"
	//"15.0"="C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Professional\\"
	// --> ...\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat [x86|x64]
	// --> ...\2017\Professional\VC\Auxiliary\Build\vcvars32.bat
	// --> ...\2017\Professional\VC\Auxiliary\Build\vcvars64.bat
	// --> ...\2017\Professional\VC\Auxiliary\Build\vcvarsamd64_x86.bat
	// --> ...\2017\Professional\VC\Auxiliary\Build\vcvarsx86_amd64.bat

	if (wcschr(pszVer, L'.') && isDigit(*pszVer))
	{
		CEStr pszDir;
		if (RegGetStringValue(hkVS, NULL, pszVer, pszDir) > 0)
		{
			CreateVCTask(App[0], L"x86", pszVer, pszDir);
			CreateVCTask(App[1], L"x64", pszVer, pszDir);
		}
	}

	return true; // continue reg enum
}

static void CreateVisualStudioTasks()
{
	AppFoundList App[2];

	SettingsLoadedFlags old = sAppendMode;
	if (!(sAppendMode & slf_AppendTasks))
		sAppendMode = (slf_AppendTasks | slf_RewriteExisting);

	// Visual Studio prompt: HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio
	RegEnumKeys(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\VisualStudio", CreateVCTasks, (LPARAM)App);
	RegEnumValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VS7", CreateVCTasks, (LPARAM)App);

	App[0].Commit();
	App[1].Commit();

	sAppendMode = old;
}

static void CreateChocolateyTask()
{
	// Chocolatey gallery
	//-- Placing ANSI in Task commands will be too long and unfriendly
	//-- Also, automatic run of Chocolatey installation may harm user environment in some cases
	CEStr szFull(ExpandEnvStr(L"%ConEmuBaseDir%\\Addons\\ChocolateyAbout.cmd"));
	if (szFull && FileExists(szFull))
	{
		// Don't use 'App.Add' here, we are creating "cmd.exe" tasks directly
		CreateDefaultTask(L"Tools::Chocolatey (Admin)", L"", L"*cmd.exe /k Title Chocolatey & \"%ConEmuBaseDir%\\Addons\\ChocolateyAbout.cmd\"");
	}
}

// NYAOS & NYAGOS
static void CreateNyagosTask()
{
	AppFoundList App;

	// NYAOS
	App.Add(L"Shells::NYAOS", NULL, NULL, NULL, L"nyaos.exe", NULL);
	App.Add(L"Shells::NYAOS (Admin)", L" -new_console:a", NULL, NULL, L"nyaos.exe", NULL);

	// NYAGOS
	App.Add(L"Shells::NYAGOS", NULL, NULL, NULL, L"nyagos.exe", NULL);
	App.Add(L"Shells::NYAGOS (Admin)", L" -new_console:a", NULL, NULL, L"nyagos.exe", NULL);

	App.Commit();
}

// cmd.exe
static void CreateCmdTask()
{
	AppFoundList App;
	// Windows internal: cmd
	// Don't use 'App.Add' here, we are creating "cmd.exe" tasks directly
	CreateDefaultTask(L"Shells::cmd", L"",
		L"cmd.exe /k \"%ConEmuBaseDir%\\CmdInit.cmd\"", CETF_CMD_DEFAULT);
#if 0
	// Need to "set" ConEmuGitPath to full path to the git.exe
	CreateDefaultTask(L"Shells::cmd+git", L"",
		L"cmd.exe /k \"%ConEmuBaseDir%\\CmdInit.cmd\" /git");
#endif
	CreateDefaultTask(L"Shells::cmd (Admin)", L"",
		L"cmd.exe /k \"%ConEmuBaseDir%\\CmdInit.cmd\" -new_console:a");
	// On 64-bit OS we suggest more options
	if (IsWindows64())
	{
		// Add {cmd-32} task to run 32-bit cmd.exe
		CreateDefaultTask(L"Shells::cmd-32", L"",
			L"\"%windir%\\syswow64\\cmd.exe\" /k \"%ConEmuBaseDir%\\CmdInit.cmd\"");
		// Windows internal: For 64bit Windows create task with splitted cmd 64/32 (Example)
		CreateDefaultTask(L"Shells::cmd 64/32", L"",
			L"> \"%windir%\\system32\\cmd.exe\" /k \"\"%ConEmuBaseDir%\\CmdInit.cmd\" & echo This is Native cmd.exe\""
			L"\r\n\r\n"
			L"\"%windir%\\syswow64\\cmd.exe\" /k \"\"%ConEmuBaseDir%\\CmdInit.cmd\" & echo This is 32 bit cmd.exe -new_console:s50V\"");
	}
}

// powershell.exe
static void CreatePowerShellTask()
{
	AppFoundList App;
	// Windows internal: PowerShell
	// Don't use 'App.Add' here, we are creating "powershell.exe" tasks directly
	App.Add(L"Shells::PowerShell", NULL, NULL, NULL, L"powershell.exe", NULL);
	App.Add(L"Shells::PowerShell (Admin)", L" -new_console:a", NULL, NULL, L"powershell.exe", NULL);

	App.Add(L"Shells::PowerShell Core", NULL, NULL, NULL, L"pwsh.exe", NULL);
	App.Add(L"Shells::PowerShell Core (Admin)", L" -new_console:a", NULL, NULL, L"pwsh.exe", NULL);

	// #DefaultTasks pwsh.exe
	App.Commit();
}

static void CreatePuttyTask()
{
	AppFoundList App;
	App.Add(L"Putty", NULL, NULL, NULL, L"Putty.exe", NULL);
	App.Commit();
}

// AnsiColors16t.ans
static void CreateHelperTasks()
{
	CEStr szFound, szOptFull;
	bool bNeedQuot = false;

	// Type ANSI color codes
	// cmd /k type "%ConEmuBaseDir%\Addons\AnsiColors16t.ans" -cur_console:n
	FoundFiles files;
	if (FindOnDrives(NULL, L"%ConEmuBaseDir%\\Addons\\AnsiColors16t.ans", files))
	{
		// Don't use 'App.Add' here, we are creating "cmd.exe" tasks directly
		CreateDefaultTask(L"Helper::Show ANSI colors", L"", L"cmd.exe /k type \"%ConEmuBaseDir%\\Addons\\AnsiColors16t.ans\" -cur_console:n");
	}
}

// Miscellaneous BASH tasks (Git, Cygwin, Msys, whatever)
static void CreateBashTask()
{
	AppFoundList App;

	bool bash_found = false;

	// New Windows 10 feature (build 14316 and higher)
	//   User have to
	//   a) Turn on Windows' feature ‘Windows Subsystem for Linux’
	//      Control Panel / Programs / Turn Windows features on or off
	//   b) Select ‘Developer mode’
	//      Settings / Update & Security / For Developers
	if (IsWin10())
	{
		#ifndef _WIN64
		MWow64Disable wow; wow.Disable(); // We need 64-bit version of system32
		#endif
		wchar_t BashOnUbuntu[] = L"%windir%\\system32\\bash.exe";
		wchar_t WslLoader[] = L"%windir%\\system32\\wsl.exe";
		CEStr lsBashOnUbuntu(ExpandEnvStr(BashOnUbuntu));
		CEStr lsWslLoader(ExpandEnvStr(BashOnUbuntu));
		if (FileExists(lsWslLoader) || FileExists(lsBashOnUbuntu))
		{
			// Prefer to run WSL via connector+wsl_bridge
			bool use_bridge = false;
			CEStr lsConnector;
			const wchar_t* bridgeFiles[] = {L"wsl\\wslbridge.exe", L"wsl\\wslbridge-backend"};
			const wchar_t* apiFiles[] = {L"wsl\\msys-2.0.dll", L"wsl\\cygwin1.dll"};
			const wchar_t* connFiles32[] = {L"conemu-msys2-32.exe", L"conemu-cyg-32.exe"};
			const wchar_t* connFiles64[] = {L"conemu-msys2-64.exe", L"conemu-cyg-64.exe"};
			CEStr szBaseDir(ExpandEnvStr(L"%ConEmuBaseDir%"));
			// Check for wslbridge files
			DWORD bridge_bits = 0, bridge_api = -1;
			DWORD dwSubsystem = 0, dwBits = 0, FileAttrs = 0;
			for (size_t i = 0; i < countof(bridgeFiles); ++i)
			{
				CEStr lsPath(szBaseDir, L"\\", bridgeFiles[i]);
				if (!FileExists(lsPath))
				{
					break;
				}
				// for Windows binaries
				else if (wcschr(lsPath, L'.') != NULL)
				{
					if (GetImageSubsystem(lsPath, dwSubsystem, dwBits, FileAttrs) && (dwBits == 64 || dwBits == 32))
						bridge_bits = dwBits;
				}
			}
			// wslbridge.exe found?
			if (bridge_bits == 32 || bridge_bits == 64)
			{
				// Check for appropriate API (cygwin1 or msys2)
				for (size_t i = 0; i < countof(apiFiles); ++i)
				{
					CEStr lsPath(szBaseDir, L"\\", apiFiles[i]);
					if (GetImageSubsystem(lsPath, dwSubsystem, dwBits, FileAttrs) && (dwBits == bridge_bits))
					{
						LPCWSTR pszConnectorName = (dwBits == 32) ? connFiles32[i] : connFiles64[i];
						lsConnector.Attach(JoinPath(szBaseDir, pszConnectorName));
						if (FileExists(lsConnector))
						{
							bridge_api = (DWORD)i;
							use_bridge = true;
							lsConnector.Attach(JoinPath(L"%ConEmuBaseDirShort%", pszConnectorName));
						}
						break;
					}
				}
			}
			// Find the icon
			CEStr wslIcon;
			const wchar_t* iconFiles[] = {
				// legacy?
				L"%USERPROFILE%\\AppData\\Local\\lxss\\bash.ico",
				// Find via registry by "CanonicalGroupLimited"?
				// [HKEY_CLASSES_ROOT\ActivatableClasses\Package\CanonicalGroupLimited.UbuntuonWindows_1604.2017.922.0_x64__79rhkp1fndgsc]
				L"%ProgramW6432%\\WindowsApps\\CanonicalGroupLimited.UbuntuonWindows_1604.2017.922.0_x64__79rhkp1fndgsc\\images\\icon.ico",
				L"%ProgramFiles%\\WindowsApps\\CanonicalGroupLimited.UbuntuonWindows_1604.2017.922.0_x64__79rhkp1fndgsc\\images\\icon.ico"
			};
			for (size_t i = 0; i < countof(iconFiles); ++i)
			{
				CEStr lsPath(ExpandEnvStr(iconFiles[i]));
				if (FileExists(lsPath))
				{
					wslIcon.Attach(lstrmerge(L"-icon \"", iconFiles[i], L"\""));
					break;
				}
			}
			// Create the task
			bash_found |= App.Add(L"Bash::bash",
				use_bridge ? L" --wsl -cur_console:pm:/mnt" : L" -cur_console:pm:/mnt", // "--login -i" is not required yet
				use_bridge ? L"set \"PATH=%ConEmuBaseDirShort%\\wsl;%PATH%\" & " : NULL,
				wslIcon.c_str(),
				use_bridge ? lsConnector.ms_Val : BashOnUbuntu,
				NULL);
		}
	}

	// From Git-for-Windows (aka msysGit v2)
	bool bGitBashExist = // No sense to add both `git-cmd.exe` and `bin/bash.exe`
		App.Add(L"Bash::Git bash",
			L" --no-cd --command=/usr/bin/bash.exe -l -i", NULL, L"git",
			L"[SOFTWARE\\GitForWindows:InstallPath]\\git-cmd.exe",
			L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1:InstallLocation]\\git-cmd.exe",
			L"%ProgramFiles%\\Git\\git-cmd.exe", L"%ProgramW6432%\\Git\\git-cmd.exe",
			WIN3264TEST(NULL,L"%ProgramFiles(x86)%\\Git\\git-cmd.exe"),
			NULL);
	bash_found |= bGitBashExist;
	bash_found |= App.Add(L"Bash::GitSDK bash",
		L" --no-cd --command=/usr/bin/bash.exe -l -i", NULL, L"git",
		L"\\GitSDK\\git-cmd.exe",
		NULL);
	// From msysGit
	if (!bGitBashExist) // Skip if `git-cmd.exe` was already found (from MSYS2 or Git-for-Windows)
		bash_found |= App.Add(L"Bash::Git bash",
			L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\etc\\git.ico\"", NULL,  L"msys1",
			L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1:InstallLocation]\\bin\\bash.exe",
			L"%ProgramFiles%\\Git\\bin\\bash.exe", L"%ProgramW6432%\\Git\\bin\\bash.exe",
			WIN3264TEST(NULL,L"%ProgramFiles(x86)%\\Git\\bin\\bash.exe"),
			NULL);
	// For cygwin we can check registry keys
	// HKLM\SOFTWARE\Wow6432Node\Cygwin\setup\rootdir
	// HKLM\SOFTWARE\Cygwin\setup\rootdir
	// HKCU\Software\Cygwin\setup\rootdir
	bash_found |= App.Add(L"Bash::CygWin bash",
		L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\Cygwin.ico\"", L"set CHERE_INVOKING=1 & ", L"cygwin",
		L"[SOFTWARE\\Cygwin\\setup:rootdir]\\bin\\bash.exe",
		L"\\CygWin\\bin\\bash.exe", NULL);
	//{L"CygWin mintty", L"\\CygWin\\bin\\mintty.exe", L" -"},
	bash_found |= App.Add(L"Bash::MinGW bash",
		L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\msys.ico\"", L"set CHERE_INVOKING=1 & ", L"msys1",
		L"\\MinGW\\msys\\1.0\\bin\\bash.exe", NULL);
	//{L"MinGW mintty", L"\\MinGW\\msys\\1.0\\bin\\mintty.exe", L" -"},
	// MSys2 project: 'HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\MSYS2 32bit'
	// Perhaps for Msys2 we shall use "sh.exe" instead of "bash.exe"?
	//   the "bash.exe" may be is used for legacy emulation?
	bash_found |= App.Add(L"Bash::Msys2-64",
		L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\..\\msys2.ico\"", L"set CHERE_INVOKING=1 & ", L"msys64",
		L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\*:DisplayName=MSYS2 64bit:InstallLocation]\\usr\\bin\\bash.exe",
		L"msys64\\usr\\bin\\bash.exe",
		NULL);
	bash_found |= App.Add(L"Bash::Msys2-32",
		L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\..\\msys2.ico\"", L"set CHERE_INVOKING=1 & ", L"msys32",
		L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\*:DisplayName=MSYS2 32bit:InstallLocation]\\usr\\bin\\bash.exe",
		L"msys32\\usr\\bin\\bash.exe",
		NULL);
	// Last chance for bash
	if (!bash_found)
		App.Add(L"Bash::bash", L" --login -i", L"set CHERE_INVOKING=1 & ", NULL, L"bash.exe", NULL);

	// Force connector
	CEStr szBaseDir(ExpandEnvStr(L"%ConEmuBaseDir%"));
	bool bNeedQuot = IsQuotationNeeded(szBaseDir);
	for (INT_PTR i = 0; i < App.Installed.size(); ++i)
	{
		AppFoundList::AppInfo& ai = App.Installed[i];
		if (!ai.szGuiArg)
			continue;
		DWORD bits = ai.dwBits;
		CEStr szConnector;
		LPCWSTR szConnectorName = NULL;
		bool msys_git_2 = false;
		if (wcscmp(ai.szGuiArg, L"cygwin") == 0)
			szConnectorName = bits==32 ? L"conemu-cyg-32.exe"
				: bits==64 ? L"conemu-cyg-64.exe"
				: NULL;
		else if (wcscmp(ai.szGuiArg, L"msys1") == 0)
			szConnectorName = bits==32 ? L"conemu-msys-32.exe"
				: NULL;
		else if (wcscmp(ai.szGuiArg, L"msys32") == 0)
			szConnectorName = bits==32 ? L"conemu-msys2-32.exe"
				: NULL;
		else if (wcscmp(ai.szGuiArg, L"msys64") == 0)
			szConnectorName = bits==64 ? L"conemu-msys2-64.exe"
				: NULL;
		else if ((msys_git_2 = (wcscmp(ai.szGuiArg, L"git") == 0)))
			szConnectorName = bits==64 ? L"conemu-msys2-64.exe"
				: bits==32 ? L"conemu-msys2-32.exe"
				: NULL;
		else
			continue;

		SafeFree(ai.szGuiArg);

		if (szConnectorName)
		{
			CEStr szConnector(JoinPath(szBaseDir, szConnectorName));
			if (FileExists(szConnector))
			{
				// For git-cmd ai.szPrefix is empty by default
				_ASSERTE(!ai.szPrefix || (*ai.szPrefix && ai.szPrefix[wcslen(ai.szPrefix)-1]==L' '));

				CEStr szBinPath;
				szBinPath.Set(ai.szFullPath);
				wchar_t* ptrFound = wcsrchr(szBinPath.ms_Val, L'\\');
				if (ptrFound) *ptrFound = 0;

				if (!msys_git_2)
				{
					lstrmerge(&ai.szPrefix,
						// TODO: Optimize: Don't add PATH if required cygwin1.dll/msys2.dll is already on path
						L"set \"PATH=", szBinPath, L";%PATH%\" & ",
						// Change main executable
						/*bNeedQuot ? L"\"" :*/ L"",
						L"%ConEmuBaseDirShort%\\", szConnectorName,
						/*bNeedQuot ? L"\" " :*/ L" ",
						// Force xterm mode
						L"-new_console:p "
						);
				}
				else
				{
					_ASSERTE(ai.szArgs && wcsstr(ai.szArgs, L"--command=/usr/bin/bash.exe"));
					const wchar_t* cmd_ptr = L"--command=";
					wchar_t* pszCmd = wcsstr(ai.szArgs, cmd_ptr);
					if (pszCmd)
					{
						pszCmd += wcslen(cmd_ptr);
						_ASSERTE(ai.szPrefix == NULL || !*ai.szPrefix);
						lstrmerge(&ai.szPrefix,
							// TODO: Optimize: Don't add PATH if required cygwin1.dll/msys2.dll is already on path
							L"set \"PATH=", szBinPath, L"\\usr\\bin;%PATH%\" & ");
						// Insert connector between "--command=" and "/usr/bin/bash.exe"
						_ASSERTE(*pszCmd == L'/');
						*pszCmd = 0;
						CEStr lsArgs(
							// git-cmd options
							ai.szArgs,
							// Change main executable
							/*bNeedQuot ? L"\"" :*/ L"",
							L"%ConEmuBaseDirShort%\\", szConnectorName,
							/*bNeedQuot ? L"\" " :*/ L" ",
							// And the tail of the command: "/usr/bin/bash.exe -l -i"
							L"/", pszCmd+1,
							// Force xterm mode
							L" -new_console:p");
						SafeFree(ai.szArgs);
						ai.szArgs = lsArgs.Detach();
					}
				}
			}
		}
	}

	// Create all bash tasks
	App.Commit();
}

// Docker Toolbox
static void CreateDockerTask()
{
	CEStr szFull(ExpandEnvStr(L"%DOCKER_TOOLBOX_INSTALL_PATH%\\docker.exe"));
	if (szFull && FileExists(szFull))
	{
		AppFoundList App(1);
		App.Add(L"Tools::Docker",
			L"-l -i \"%DOCKER_TOOLBOX_INSTALL_PATH%\\start.sh\" -new_console:t:\"Docker\"", NULL,
			// There is a special icon file
			// "%DOCKER_TOOLBOX_INSTALL_PATH%\\docker-quickstart-terminal.ico"
			// but it's displayed badly in our tabs at the moment
			L"/dir \"%DOCKER_TOOLBOX_INSTALL_PATH%\" /icon \"%DOCKER_TOOLBOX_INSTALL_PATH%\\docker.exe\"",
			L"%DOCKER_TOOLBOX_INSTALL_PATH%\\..\\Git\\usr\\bin\\bash.exe",
			L"bash.exe",
			L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1:InstallLocation]\\usr\\bin\\bash.exe",
			L"%ProgramFiles%\\Git\\usr\\bin\\bash.exe", L"%ProgramW6432%\\Git\\usr\\bin\\bash.exe",
			#ifdef _WIN64
			L"%ProgramFiles(x86)%\\Git\\usr\\bin\\bash.exe",
			#endif
			L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MSYS2 64bit:InstallLocation]\\usr\\bin\\bash.exe",
			L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MSYS2 32bit:InstallLocation]\\usr\\bin\\bash.exe",
			L"[SOFTWARE\\Cygwin\\setup:rootdir]\\bin\\bash.exe",
			NULL);
		App.Commit();
	}
}

// *Create new* or *add absent* default tasks
void CreateDefaultTasks(SettingsLoadedFlags slfFlags)
{
	LogString(L"CreateDefaultTasks:: started");
	DEBUGSTRTASKS(L"CreateDefaultTasks:: started");

	iCreatIdx = 0;

	sAppendMode = slfFlags;
	gn_FirstFarTask = -1;

	if (!(slfFlags & slf_AppendTasks))
	{
		const CommandTasks* pExist = gpSet->CmdTaskGet(iCreatIdx);
		if (pExist != NULL)
		{
			// At least one task was already created
			LogString(L"CreateDefaultTasks:: tasks exist");
			DEBUGSTRTASKS(L"CreateDefaultTasks:: tasks exist");
			return;
		}
	}
	else
	{
		// Find LAST USED index
		while (gpSet->CmdTaskGet(iCreatIdx))
			iCreatIdx++;
	}

	CVarDefs Vars;
	spVars = &Vars;

	AppFoundList App;

	ZeroStruct(szConEmuDrive);
	wchar_t szTemp[MAX_PATH];
	GetDrive(gpConEmu->ms_ConEmuExeDir, szTemp, countof(szTemp));
	_ASSERTE(szTemp[0] && szTemp[_tcslen(szTemp)-1] != L'\\'); // Supposed to be simple "C:"
	szConEmuDrive.Set(szTemp);

	/*
	+ Far Manager
	+ TCC/LE (Take Command)
	+ NYAOS
	? NYAGOS
	+ cmd/Admin/x64 (/k CmdInit.cmd)
	? cmd+git (have to define/reload default %ConEmuGitPath%)
	+ PowerShell/Admin
	+ MinGW/GIT/CygWin bash (and GOW?)
	+ PuTTY
	+ Show ANSI colors
	+ WinSdkTasks
	+ VCTasks
	+ Tools::Docker
	+ ChocolateyAbout.cmd
	*/

	// Far Manager
	CreateFarTasks();

	// TakeCommand
	CreateTccTasks();

	// NYAOS - !!!Registry TODO!!!
	CreateNyagosTask();

	// Windows internal: cmd
	CreateCmdTask();

	// Windows internal: PowerShell
	CreatePowerShellTask();

	// Miscellaneous BASH tasks (Git, Cygwin, Msys, whatever)
	CreateBashTask();

	// Putty
	CreatePuttyTask();

	// IRSSI
	// L"\"set PATH=C:\\irssi\\bin;%PATH%\" & set PERL5LIB=lib/perl5/5.8 & set TERMINFO_DIRS=terminfo & "
	// L"C:\\irssi\\bin\\irssi.exe"
	// L" -cur_console:d:\"C:\\irssi\""

	// Some helpers (AnsiColors16t.ans)
	CreateHelperTasks();

	// Windows SDK: HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows
	RegEnumKeys(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows", CreateWinSdkTasks, 0);

	// Visual Studio prompt
	CreateVisualStudioTasks();

	// Docker Toolbox
	CreateDockerTask();

	// About Chocolatey
	CreateChocolateyTask();

	SafeFree(szConEmuDrive.ms_Val);

	// Choose default startup command
	if (slfFlags & (slf_DefaultSettings|slf_DefaultTasks))
	{
		FindStartupTask(slfFlags);
	}

	LogString(L"CreateDefaultTasks:: finished");
	DEBUGSTRTASKS(L"CreateDefaultTasks:: finished");
}

}; // namespace FastConfig
