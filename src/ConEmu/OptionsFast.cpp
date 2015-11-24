﻿
/*
Copyright (c) 2009-2015 Maximus5
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
#include "Header.h"
#include "AboutDlg.h"
#include "DynDialog.h"
#include "HotkeyDlg.h"
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
#include "../common/CmdArg.h"
#include "../common/execute.h"
#include "../common/FarVersion.h"
#include "../common/StartupEnvDef.h"
#include "../common/WFiles.h"
#include "../common/WRegistry.h"
#include "../common/WUser.h"

#define FOUND_APP_PATH_CHR L'\1'
#define FOUND_APP_PATH_STR L"\1"

static bool bCheckHooks, bCheckUpdate, bCheckIme;
// Если файл конфигурации пуст, то после вызова CheckOptionsFast
// все равно будет SaveSettings(TRUE/*abSilent*/);
// Поэтому выбранные настройки здесь можно не сохранять (кроме StopWarningConIme)
static bool bVanilla;
static CDpiForDialog* gp_DpiAware = NULL;
static CEHelpPopup* gp_FastHelp = NULL;
static int gn_FirstFarTask = -1;
static ConEmuHotKey ghk_MinMaxKey = {};
static int iCreatIdx = 0;
static CEStr szConEmuDrive;

void Fast_FindStartupTask(SettingsLoadedFlags slfFlags);
LPCWSTR Fast_GetStartupCommand(const CommandTasks*& pTask);

static const ColorPalette* gp_DefaultPalette = NULL;
static WNDPROC gpfn_DefaultColorBoxProc = NULL;
static LRESULT CALLBACK Fast_ColorBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			if (CSetDlgLists::GetSelectedString(GetParent(hwnd), lbColorSchemeFast, &lsValue.ms_Arg) > 0)
			{
				const ColorPalette* pPal = gpSet->PaletteGetByName(lsValue.ms_Arg);
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

static INT_PTR Fast_OnInitDialog(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);

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


	// lbStorageLocation
	SettingsStorage Storage = {}; bool ReadOnly = false;
	gpSet->GetSettingsType(Storage, ReadOnly);

	// Same priority as in CConEmuMain::ConEmuXml (reverse order)
	wchar_t* pszSettingsPlaces[] = {
		lstrdup(L"HKEY_CURRENT_USER\\Software\\ConEmu"),
		ExpandEnvStr(L"%APPDATA%\\ConEmu.xml"),
		ExpandEnvStr(L"%ConEmuBaseDir%\\ConEmu.xml"),
		ExpandEnvStr(L"%ConEmuDir%\\ConEmu.xml"),
		NULL
	};
	// Lets find first allowed item
	int iAllowed = 0;
	if (lstrcmp(Storage.szType, CONEMU_CONFIGTYPE_XML) == 0)
	{
		iAllowed = 1; // XML is used, registry is not allowed
		if (Storage.pszFile)
		{
			if (lstrcmpi(Storage.pszFile, pszSettingsPlaces[1]) == 0) // %APPDATA%
				iAllowed = 1; // Any other xml has greater priority
			else if (lstrcmpi(Storage.pszFile, pszSettingsPlaces[2]) == 0) // %ConEmuBaseDir%
				iAllowed = 2; // Only %ConEmuDir% has greater priority
			else if (lstrcmpi(Storage.pszFile, pszSettingsPlaces[3]) == 0) // %ConEmuDir%
				iAllowed = 3; // Most prioritized
			else
			{
				// Directly specified with "/LoadCfgFile ..."
				SafeFree(pszSettingsPlaces[3]);
				pszSettingsPlaces[3] = lstrdup(Storage.pszFile);
				iAllowed = 3; // Most prioritized
			}
		}
	}
	// Index of the default location (relative to listbox, but not a pszSettingsPlaces)
	// By default - suggest %APPDATA% or, if possible, %ConEmuDir%
	// Win2k does not have 'msxml3.dll'/'msxml3r.dll' libraries
	int iDefault = ((iAllowed == 0) && !IsWin2kEql()) ? (CConEmuUpdate::NeedRunElevation() ? 1 : 3) : 0;

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
	const CommandTasks* pTask = NULL;
	LPCWSTR pszStartup = Fast_GetStartupCommand(pTask);
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
		gpfn_DefaultColorBoxProc = (WNDPROC)SetWindowLongPtr(hChild, GWLP_WNDPROC, (LONG_PTR)Fast_ColorBoxProc);


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
		if (gpSet->UpdSet.isUpdateUseBuilds != 0)
			CheckDlgButton(hDlg, cbEnableAutoUpdateFast, gpSet->UpdSet.isUpdateCheckOnStartup|gpSet->UpdSet.isUpdateCheckHourly);
		CheckRadioButton(hDlg, rbAutoUpdateStableFast, rbAutoUpdateDeveloperFast,
			(gpSet->UpdSet.isUpdateUseBuilds == 1) ? rbAutoUpdateStableFast :
			(gpSet->UpdSet.isUpdateUseBuilds == 3) ? rbAutoUpdatePreviewFast : rbAutoUpdateDeveloperFast);
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

static INT_PTR Fast_OnCtlColorStatic(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
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

static INT_PTR Fast_OnSetCursor(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	if (((HWND)wParam) == GetDlgItem(hDlg, stHomePage))
	{
		SetCursor(LoadCursor(NULL, IDC_HAND));
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
		return TRUE;
	}

	return FALSE;
}

static INT_PTR Fast_OnButtonClicked(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
		case IDOK:
		{
			CEStr lsValue;

			SettingsStorage CurStorage = {}; bool ReadOnly = false;
			gpSet->GetSettingsType(CurStorage, ReadOnly);
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
			if (CSetDlgLists::GetSelectedString(hDlg, lbStartupShellFast, &lsValue.ms_Arg) > 0)
			{
				if (*lsValue.ms_Arg == TaskBracketLeft)
				{
					if (lsValue.ms_Arg[lstrlen(lsValue.ms_Arg)-1] != TaskBracketRight)
					{
						_ASSERTE(FALSE && "Doesn't match '{...}'");
					}
					else
					{
						gpSet->nStartType = 2;
						SafeFree(gpSet->psStartTasksName);
						gpSet->psStartTasksName = lstrdup(lsValue.ms_Arg);
					}
				}
				else if (lstrcmp(lsValue.ms_Arg, AutoStartTaskName) == 0)
				{
					// Not shown yet in list
					gpSet->nStartType = 3;
				}
				else if (*lsValue.ms_Arg == CmdFilePrefix)
				{
					gpSet->nStartType = 1;
					SafeFree(gpSet->psStartTasksFile);
					gpSet->psStartTasksFile = lstrdup(lsValue.ms_Arg);
				}
				else
				{
					gpSet->nStartType = 0;
					SafeFree(gpSet->psStartSingleApp);
					gpSet->psStartSingleApp = lstrdup(lsValue.ms_Arg);
				}
			}

			/* Default pallette changed? */
			if (CSetDlgLists::GetSelectedString(hDlg, lbColorSchemeFast, &lsValue.ms_Arg) > 0)
			{
				const ColorPalette* pPal = gpSet->PaletteGetByName(lsValue.ms_Arg);
				if (pPal)
				{
					gpSetCls->ChangeCurrentPalette(pPal, false);
				}
			}

			/* Force Single instance mode */
			gpSet->isSingleInstance = IsDlgButtonChecked(hDlg, cbSingleInstance);

			/* Quake mode? */
			gpSet->isQuakeStyle = IsDlgButtonChecked(hDlg, cbQuakeFast);

			/* Min/Restore key */
			gpSet->SetHotkeyById(vkMinimizeRestore, ghk_MinMaxKey.GetVkMod());

			/* Install Keyboard hooks */
			gpSet->m_isKeyboardHooks = IsDlgButtonChecked(hDlg, cbUseKeyboardHooksFast) ? 1 : 2;

			/* Inject ConEmuHk.dll */
			gpSet->isUseInjects = IsDlgButtonChecked(hDlg, cbInjectConEmuHkFast);

			/* Auto Update settings */
			gpSet->UpdSet.isUpdateCheckOnStartup = (IsDlgButtonChecked(hDlg, cbEnableAutoUpdateFast) == BST_CHECKED);
			if (bCheckUpdate)
			{	// При первом запуске - умолчания параметров
				gpSet->UpdSet.isUpdateCheckHourly = false;
				gpSet->UpdSet.isUpdateConfirmDownload = true; // true-Show MessageBox, false-notify via TSA only
			}
			gpSet->UpdSet.isUpdateUseBuilds = IsDlgButtonChecked(hDlg, rbAutoUpdateStableFast) ? 1 : IsDlgButtonChecked(hDlg, rbAutoUpdateDeveloperFast) ? 2 : 3;


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

static INT_PTR CALLBACK CheckOptionsFastProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (messg)
	{
	case WM_SETHOTKEY:
		gnWndSetHotkey = wParam;
		break;

	case WM_INITDIALOG:
		return Fast_OnInitDialog(hDlg, messg, wParam, lParam);

	case WM_CTLCOLORSTATIC:
		return Fast_OnCtlColorStatic(hDlg, messg, wParam, lParam);

	case WM_SETCURSOR:
		return Fast_OnSetCursor(hDlg, messg, wParam, lParam);

	case HELP_WM_HELP:
		break;
	case WM_HELP:
		if (gp_FastHelp && (wParam == 0) && (lParam != 0))
		{
			HELPINFO* hi = (HELPINFO*)lParam;
			gp_FastHelp->ShowItemHelp(hi);
		}
		return TRUE;

	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case BN_CLICKED:
			return Fast_OnButtonClicked(hDlg, messg, wParam, lParam);
		case CBN_SELCHANGE:
			switch (LOWORD(wParam))
			{
			case lbColorSchemeFast:
				SendDlgItemMessage(hDlg, stPalettePreviewFast, UM_PALETTE_FAST_CHG, wParam, lParam);
				break;
			}
			break;
		}

		break;

	case WM_SYSCOMMAND:
		if (wParam == SC_CLOSE)
		{
			BOOL b = gbMessagingStarted; gbMessagingStarted = TRUE;
			int iQuitBtn = MsgBox(L"Close dialog and terminate ConEmu?", MB_ICONQUESTION|MB_YESNO, NULL, hDlg);
			gbMessagingStarted = b;
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

		bCheckUpdate = (gpSet->UpdSet.isUpdateUseBuilds == 0);

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

		gp_DpiAware = new CDpiForDialog();
		gp_FastHelp = new CEHelpPopup;

		// Modal dialog (CreateDialog)

		CDynDialog::ExecuteDialog(IDD_FAST_CONFIG, NULL, CheckOptionsFastProc, (LPARAM)asTitle);

		SafeDelete(gp_FastHelp);
		SafeDelete(gp_DpiAware);
	}
}





/* **************************************** */
/*         Creating default tasks           */
/* **************************************** */

static SettingsLoadedFlags sAppendMode = slf_None;

static void CreateDefaultTask(LPCWSTR asName, LPCWSTR asGuiArg, LPCWSTR asCommands, CETASKFLAGS aFlags = CETF_DONT_CHANGE)
{
	_ASSERTE(asName && asName[0] && asName[0] != TaskBracketLeft && asName[wcslen(asName)-1] != TaskBracketRight);
	wchar_t szLeft[2] = {TaskBracketLeft}, szRight[2] = {TaskBracketRight};
	CEStr lsName = lstrmerge(szLeft, asName, szRight);

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

// Search on asFirstDrive and all (other) fixed drive letters
// asFirstDrive may be letter ("C:") or network (\\server\share)
// asSearchPath is path to executable (\cygwin\bin\sh.exe)
static bool FindOnDrives(LPCWSTR asFirstDrive, LPCWSTR asSearchPath, CEStr& rsFound, bool& bNeedQuot, CEStr& rsOptionalFull)
{
	bool bFound = false;
	wchar_t* pszExpanded = NULL;
	wchar_t szDrive[4]; // L"C:"
	wchar_t szTemp[MAX_PATH+1];

	bNeedQuot = false;

	rsOptionalFull.Empty();

	if (!asSearchPath || !*asSearchPath)
		goto wrap;

	// Using registry path?
	if ((asSearchPath[0] == L'[') && wcschr(asSearchPath+1, L']'))
	{
		// L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1:InstallLocation]\\bin\\sh.exe",
		//   "InstallLocation"="C:\\Utils\\Lans\\GIT\\"
		CEStr lsBuf, lsVal;
		lsBuf.Set(asSearchPath+1);
		wchar_t *pszFile = wcschr(lsBuf.ms_Arg, L']');
		if (pszFile)
		{
			*(pszFile++) = 0;
			wchar_t* pszValName = wcsrchr(lsBuf.ms_Arg, L':');
			if (pszValName) *(pszValName++) = 0;
			if (RegGetStringValue(NULL, lsBuf.ms_Arg, pszValName, lsVal) > 0)
			{
				rsFound.Attach(JoinPath(lsVal, pszFile));
				if (FileExists(rsFound))
				{
					bNeedQuot = IsQuotationNeeded(rsFound);
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
			bNeedQuot = IsQuotationNeeded(pszExpanded);
			rsOptionalFull.Set(pszExpanded);
			rsFound.Set(asSearchPath);
			bFound = true;
		}
		goto wrap;
	}

	// Only executable name was specified?
	if (!wcschr(asSearchPath, L'\\'))
	{
		DWORD nFind = SearchPath(NULL, asSearchPath, NULL, countof(szTemp), szTemp, NULL);
		if (nFind && (nFind < countof(szTemp)))
		{
			// OK, create task with just a name of exe file
			bNeedQuot = IsQuotationNeeded(asSearchPath);
			rsOptionalFull.Set(szTemp);
			rsFound.Set(asSearchPath);
			bFound = true;
		}
		// Search in [HKCU|HKLM]\Software\Microsoft\Windows\CurrentVersion\App Paths
		else if (SearchAppPaths(asSearchPath, rsFound, false/*abSetPath*/))
		{
			bNeedQuot = IsQuotationNeeded(rsFound);
			bFound = true;
		}
		goto wrap;
	}

	// Full path was specified? Check it.
	if (IsFilePath(asSearchPath, true)
		&& FileExists(asSearchPath))
	{
		bNeedQuot = IsQuotationNeeded(asSearchPath);
		rsFound.Set(asSearchPath);
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
			bNeedQuot = IsQuotationNeeded(rsFound);
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
			bNeedQuot = IsQuotationNeeded(rsFound);
			bFound = true;
			goto wrap;
		}
	}

wrap:
	SafeFree(pszExpanded);
	return bFound;
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
			CEStr szArgs(lstrmerge(L"-new_console:t:\"WinSDK ", pszVer, L"\":C:\"", szIcon, L"\""));
			CEStr szFull(lstrmerge(L"cmd /V /K ", szArgs, L" \"", szCmd, L"\""));
			// Create task
			if (szFull)
			{
				CEStr szName(lstrmerge(L"SDK::WinSDK ", pszVer));
				if (szName)
				{
					CreateDefaultTask(szName, L"", szFull);
				}
			}
		}
	}

	return true; // continue reg enum
}

// Visual Studio C++
static bool WINAPI CreateVCTasks(HKEY hkVer, LPCWSTR pszVer, LPARAM lParam)
{
	//[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\11.0\Setup\VC]
	//"ProductDir"="C:\\Program Files (x86)\\Microsoft Visual Studio 11.0\\VC\\"
	//[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\12.0\Setup\VC]
	//"ProductDir"="C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\"
	// %comspec% /k ""C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"" x86

	CEStr pszDir;
	if (wcschr(pszVer, L'.'))
	{
		int iVer = wcstol(pszVer, NULL, 10);

		if (RegGetStringValue(hkVer, L"Setup\\VC", L"ProductDir", pszDir) > 0)
		{
			CEStr pszVcVarsBat = JoinPath(pszDir, L"vcvarsall.bat");
			if (FileExists(pszVcVarsBat))
			{
				CEStr pszName = lstrmerge(L"SDK::VS ", pszVer, L" x86 tools prompt");
				CEStr pszFull = lstrmerge(L"cmd /k \"\"", pszVcVarsBat, L"\"\" x86 -new_console:t:\"VS ", pszVer, L"\"");

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
					case 14: pszIconSfx = L",33\""; break;
					case 12: pszIconSfx = L",28\""; break;
					case 11: pszIconSfx = L",23\""; break;
					case 10: pszIconSfx = L",16\""; break;
					case 9:  pszIconSfx = L",10\""; break;
					default: pszIconSfx = L"\"";
					}
					lstrmerge(&pszFull.ms_Arg, L" -new_console:C:\"", pszIconSource, pszIconSfx);
				}

				CreateDefaultTask(pszName, L"", pszFull);
			}
		}
	}

	return true; // continue reg enum
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
	// and %ProgramFiles% but it may fails on 64-bit OS due to bitness differences
	// - if (UnExpandEnvStrings(szFound, szUnexpand, countof(szUnexpand)) && (lstrcmp(szFound, szUnexpand) != 0)) ;
	if (!spVars)
	{
		_ASSERTE(spVars != NULL);
		return false;
	}

	if (!IsFilePath(asSource, true))
		return false;

	CEStr szTemp(lstrdup(asSource));
	wchar_t* ptrSrc = szTemp.ms_Arg;
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
		bool bNeedQuot;
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

protected:
	// This will load App version and check if it was already added
	virtual INT_PTR AddAppPath(LPCWSTR asName, LPCWSTR szPath, LPCWSTR pszOptFull, bool bNeedQuot,
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
			LoadAppVersion(pszPath, FI.Ver, ErrText);

			// App instance found, add it to Installed array?
			bool bAlready = false;
			for (INT_PTR a = 0; a < Installed.size(); a++)
			{
				AppInfo& ai = Installed[a];
				if (lstrcmpi(ai.szFullPath, szPath) == 0)
				{
					bAlready = true; break; // Do not add twice same path
				}
				else if (pszOptFull && (lstrcmpi(ai.szFullPath, pszOptFull) == 0))
				{
					// Store path with environment variables (unexpanded) or without path at all (just "Far.exe" for example)
					SafeFree(ai.szFullPath);
					ai.szFullPath = lstrdup(szPath);
					bAlready = true; break; // Do not add twice same path
				}
				else if (pszOptFull && ai.szExpanded && (lstrcmpi(ai.szExpanded, pszOptFull) == 0))
				{
					bAlready = true; break; // Do not add twice same path
				}
			}
			// New instance, add it
			if (!bAlready)
			{
				lstrcpyn(FI.szTaskName, asName, countof(FI.szTaskName));
				lstrcpyn(FI.szTaskBaseName, asName, countof(FI.szTaskBaseName));
				FI.szFullPath = lstrdup(szPath);
				FI.szExpanded = pszOptFull ? lstrdup(pszOptFull) : ExpandEnvStr(szPath);
				FI.bNeedQuot = bNeedQuot;
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

public:
	bool Add(LPCWSTR asName, LPCWSTR asArgs, LPCWSTR asPrefix, LPCWSTR asGuiArg, LPCWSTR asExePath, ...)
	{
		bool bCreated = false;
		va_list argptr;
		va_start(argptr, asExePath);
		CEStr szFound, szArgs, szOptFull;
		wchar_t szUnexpand[MAX_PATH+32];

		while (asExePath)
		{
			bool bNeedQuot = false;
			LPCWSTR pszExePath = asExePath;
			asExePath = va_arg( argptr, LPCWSTR );

			// Return expanded env string
			TODO("Repace with list of 'drives'");
			if (!FindOnDrives(szConEmuDrive, pszExePath, szFound, bNeedQuot, szOptFull))
				continue;

			LPCWSTR pszFound = szFound;
			// Don't use PathUnExpandEnvStrings because it do not do what we need
			if (UnExpandEnvStrings(szFound, szUnexpand, countof(szUnexpand)) && (lstrcmp(szFound, szUnexpand) != 0))
			{
				pszFound = szUnexpand;
			}

			if (AddAppPath(asName, pszFound, szOptFull.IsEmpty() ? szFound : szOptFull, bNeedQuot, asArgs, asPrefix, asGuiArg) >= 0)
			{
				bCreated = true;
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
		if (Installed.size() <= 0)
			return;

		// Ensure task names are unique
		UINT idx = 0;

		struct impl {
			static int SortRoutine(AppInfo &e1, AppInfo &e2)
			{
				int iNameCmp = lstrcmpi(e1.szTaskBaseName, e2.szTaskBaseName);
				if (iNameCmp)
					return (iNameCmp < 0) ? -1 : 1;
				if (e1.Ver.dwFileVersionMS < e2.Ver.dwFileVersionMS)
					return 1;
				if (e1.Ver.dwFileVersionMS > e2.Ver.dwFileVersionMS)
					return -1;
				if (e1.Ver.dwFileVersionLS < e2.Ver.dwFileVersionLS)
					return 1;
				if (e1.Ver.dwFileVersionLS > e2.Ver.dwFileVersionLS)
					return -1;
				if (e1.dwBits < e2.dwBits)
					return 1;
				if (e1.dwBits > e2.dwBits)
					return -1;
				return 0;
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

					// Check only tasks with the same base names
					if (lstrcmpi(FI.szTaskBaseName, FJ.szTaskBaseName) != 0)
						continue;
					bMatch = true;

					wchar_t szPlatform[6]; wcscpy_c(szPlatform, (FJ.dwBits == 64) ? L" x64" : (FJ.dwBits == 32) ? L" x86" : L"");

					switch (u)
					{
					case 1: // Naked, only add platform
						_wsprintf(FJ.szTaskName, SKIPLEN(countof(FJ.szTaskName)-16) L"%s%s",
							FJ.szTaskBaseName, szPlatform);
						break;

					case 2: // Add App version and platform
						if (FJ.Ver.dwFileVersionMS)
							_wsprintf(FJ.szTaskName, SKIPLEN(countof(FJ.szTaskName)-16) L"%s %u.%u%s",
								FJ.szTaskBaseName, HIWORD(FJ.Ver.dwFileVersionMS), LOWORD(FJ.Ver.dwFileVersionMS), szPlatform);
						else // If there was not VersionInfo in the exe file (same as u==1)
							_wsprintf(FJ.szTaskName, SKIPLEN(countof(FJ.szTaskName)-16) L"%s%s",
								FJ.szTaskBaseName, szPlatform);
						break;

					case 3: // Add App version, platform and index
						if (FJ.Ver.dwFileVersionMS)
							_wsprintf(FJ.szTaskName, SKIPLEN(countof(FJ.szTaskName)-16) L"%s %u.%u%s (%u)",
								FJ.szTaskBaseName, HIWORD(FJ.Ver.dwFileVersionMS), LOWORD(FJ.Ver.dwFileVersionMS), szPlatform, ++idx);
						else // If there was not VersionInfo in the exe file
							_wsprintf(FJ.szTaskName, SKIPLEN(countof(FI.szTaskName)-16) L"%s%s (%u)",
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
					while ((ptrAdd = wcschr(szArgs.ms_Arg, FOUND_APP_PATH_CHR)) != NULL)
					{
						*ptrAdd = 0;
						LPCWSTR pszTail = ptrAdd+1;

						szPath.Set(ai.szFullPath);
						ptrFound = wcsrchr(szPath.ms_Arg, L'\\');
						if (ptrFound) *ptrFound = 0;

						if (*pszTail == L'\\') pszTail ++;
						while (wcsncmp(pszTail, L"..\\", 3) == 0)
						{
							ptrAdd = wcsrchr(szPath.ms_Arg, L'\\');
							if (!ptrAdd)
								break;
							// szPath is a local copy, safe to change it
							*ptrAdd = 0;
							pszTail += 3;
						}

						CEStr szTemp(JoinPath(szPath, pszTail));
						szArgs.Attach(lstrmerge(szArgs.ms_Arg, szTemp));
					}
				}
				// Succeeded?
				if (!szArgs.IsEmpty())
				{
					pszArgs = szArgs.ms_Arg;
				}
			}

			// Spaces in path? (use expanded path)
			if (ai.bNeedQuot)
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
	AppFoundList()
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

		// Check registry
		ScanRegistry();

		// Find in %Path% and on drives
		for (i = 0; FarExe[i]; i++)
		{
			if (FindOnDrives(szConEmuDrive, FarExe[i], szFound, bNeedQuot, szOptFull))
				AddAppPath(L"Far", szFound, szOptFull.IsEmpty() ? NULL : szOptFull.ms_Arg, true);
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
				static int SortRoutine(AppInfo &e1, AppInfo &e2)
				{
					if (e1.FarVer.dwVer < e2.FarVer.dwVer)
						return 1;
					if (e1.FarVer.dwVer > e2.FarVer.dwVer)
						return -1;
					if (e1.FarVer.dwBuild < e2.FarVer.dwBuild)
						return 1;
					if (e1.FarVer.dwBuild > e2.FarVer.dwBuild)
						return -1;
					if (e1.dwBits < e2.dwBits)
						return 1;
					if (e1.dwBits > e2.dwBits)
						return -1;
					return 0;
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
					wchar_t szPlatform[6]; wcscpy_c(szPlatform, (FI.dwBits == 64) ? L" x64" : (FI.dwBits == 32) ? L" x86" : L"");

					switch (u)
					{
					case 0: // Naked
						_wsprintf(FI.szTaskName, SKIPLEN(countof(FI.szTaskName)-16) L"%sFar %u.%u%s",
							pszPrefix, FI.FarVer.dwVerMajor, FI.FarVer.dwVerMinor, szPlatform);
						break;
					case 1: // Add Far Build no?
						_wsprintf(FI.szTaskName, SKIPLEN(countof(FI.szTaskName)-16) L"%sFar %u.%u.%u%s",
							pszPrefix, FI.FarVer.dwVerMajor, FI.FarVer.dwVerMinor, FI.FarVer.dwBuild, szPlatform);
						break;
					case 2: // Add Build and index
						_wsprintf(FI.szTaskName, SKIPLEN(countof(FI.szTaskName)-16) L"%sFar %u.%u.%u%s (%u)",
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

void CreateFarTasks()
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

			// Force Far to use proper plugins folders
			lstrmerge(&pszCommand, L" /p\"%ConEmuDir%\\Plugins\\ConEmu;%FARHOME%\\Plugins\"");

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

void CreateTccTasks()
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

void CreateDefaultTasks(SettingsLoadedFlags slfFlags)
{
	iCreatIdx = 0;

	sAppendMode = slfFlags;
	gn_FirstFarTask = -1;

	if (!(slfFlags & slf_AppendTasks))
	{
		const CommandTasks* pExist = gpSet->CmdTaskGet(iCreatIdx);
		if (pExist != NULL)
		{
			// At least one task was already created
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
		Far Manager
		cmd.exe
		powershell.exe
		MinGW/GIT/CygWin bash (and GOW?)
		C:\cygwin\bin\bash.exe --login -i
		TCC/LE
		Visual C++ environment (2008/2010/2012) x86 platform
		PuTTY?
	*/

	CEStr szFound, szOptFull;
	wchar_t *pszFull; bool bNeedQuot = false;

	// Far Manager
	CreateFarTasks();

	// TakeCommand
	CreateTccTasks();

	// NYAOS - !!!Registry TODO!!!
	App.Add(L"Shells::NYAOS", NULL, NULL, NULL, L"nyaos.exe", NULL);
	App.Add(L"NYAOS (Admin)", L" -new_console:a", NULL, NULL, L"nyaos.exe", NULL);
	App.Commit();

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

	// Windows internal: PowerShell
	// Don't use 'App.Add' here, we are creating "powershell.exe" tasks directly
	CreateDefaultTask(L"Shells::PowerShell", L"",
		L"powershell.exe");
	CreateDefaultTask(L"Shells::PowerShell (Admin)", L"",
		L"powershell.exe -new_console:a");

	// *** Bash ***

	// From Git-for-Windows (aka msysGit v2)
	bool bGitBashExist = // No sense to add both `git-cmd.exe` and `bin/sh.exe`
	App.Add(L"Bash::Git bash",
		L" --no-cd --command=usr/bin/bash.exe -l -i", NULL, NULL,
		L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1:InstallLocation]\\git-cmd.exe",
		L"%ProgramFiles%\\Git\\git-cmd.exe", L"%ProgramW6432%\\Git\\git-cmd.exe",
		#ifdef _WIN64
		L"%ProgramFiles(x86)%\\Git\\git-cmd.exe",
		#endif
		NULL);
	App.Add(L"Bash::GitSDK bash",
		L" --no-cd --command=usr/bin/bash.exe -l -i", NULL, NULL,
		L"\\GitSDK\\git-cmd.exe",
		NULL);
	// From msysGit
	if (!bGitBashExist) // Skip if `git-cmd.exe` was already found
	App.Add(L"Bash::Git bash",
		L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\etc\\git.ico\"", NULL, NULL,
		L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1:InstallLocation]\\bin\\sh.exe",
		L"%ProgramFiles%\\Git\\bin\\sh.exe", L"%ProgramW6432%\\Git\\bin\\sh.exe",
		#ifdef _WIN64
		L"%ProgramFiles(x86)%\\Git\\bin\\sh.exe",
		#endif
		NULL);
	// For cygwin we can check registry keys
	// HKLM\SOFTWARE\Wow6432Node\Cygwin\setup\rootdir
	// HKLM\SOFTWARE\Cygwin\setup\rootdir
	// HKCU\Software\Cygwin\setup\rootdir
	App.Add(L"Bash::CygWin bash",
		L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\Cygwin.ico\"", L"set CHERE_INVOKING=1 & ", NULL,
		L"[SOFTWARE\\Cygwin\\setup:rootdir]\\bin\\sh.exe",
		L"\\CygWin\\bin\\sh.exe", NULL);
	//{L"CygWin mintty", L"\\CygWin\\bin\\mintty.exe", L" -"},
	App.Add(L"Bash::MinGW bash",
		L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\msys.ico\"", L"set CHERE_INVOKING=1 & ", NULL,
		L"\\MinGW\\msys\\1.0\\bin\\sh.exe", NULL);
	//{L"MinGW mintty", L"\\MinGW\\msys\\1.0\\bin\\mintty.exe", L" -"},
	// MSys2 project: 'HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\MSYS2 32bit'
	App.Add(L"Bash::Msys2-64",
		L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\..\\msys2.ico\"", L"set CHERE_INVOKING=1 & ", NULL,
		L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MSYS2 64bit:InstallLocation]\\usr\\bin\\sh.exe",
		L"msys64\\usr\\bin\\sh.exe",
		NULL);
	App.Add(L"Bash::Msys2-32",
		L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\..\\msys2.ico\"", L"set CHERE_INVOKING=1 & ", NULL,
		L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MSYS2 32bit:InstallLocation]\\usr\\bin\\sh.exe",
		L"msys32\\usr\\bin\\sh.exe",
		NULL);
	// Last chance for bash
	App.Add(L"Bash::bash", L" --login -i", L"set CHERE_INVOKING=1 & ", NULL, L"sh.exe", NULL);
	// Create all bash tasks
	App.Commit();

	// Putty?
	App.Add(L"Putty", NULL, NULL, NULL, L"Putty.exe", NULL);
	App.Commit();

	// IRSSI
	// L"\"set PATH=C:\\irssi\\bin;%PATH%\" & set PERL5LIB=lib/perl5/5.8 & set TERMINFO_DIRS=terminfo & "
	// L"C:\\irssi\\bin\\irssi.exe"
	// L" -cur_console:d:\"C:\\irssi\""

	// Type ANSI color codes
	// cmd /k type "%ConEmuBaseDir%\Addons\AnsiColors16t.ans" -cur_console:n
	if (FindOnDrives(NULL, L"%ConEmuBaseDir%\\Addons\\AnsiColors16t.ans", szFound, bNeedQuot, szOptFull))
	{
		// Don't use 'App.Add' here, we are creating "cmd.exe" tasks directly
		CreateDefaultTask(L"Tests::Show ANSI colors", L"", L"cmd.exe /k type \"%ConEmuBaseDir%\\Addons\\AnsiColors16t.ans\" -cur_console:n");
	}

	// Chocolatey gallery
	//-- Placing ANSI in Task commands will be too long and unfriendly
	//-- Also, automatic run of Chocolatey installation may harm user environment in some cases
	pszFull = ExpandEnvStr(L"%ConEmuBaseDir%\\Addons\\ChocolateyAbout.cmd");
	if (pszFull && FileExists(pszFull))
	{
		// Don't use 'App.Add' here, we are creating "cmd.exe" tasks directly
		CreateDefaultTask(L"Scripts::Chocolatey (Admin)", L"", L"*cmd.exe /k Title Chocolatey & \"%ConEmuBaseDir%\\Addons\\ChocolateyAbout.cmd\"");
	}
	SafeFree(pszFull);

	// Windows SDK: HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows
	RegEnumKeys(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows", CreateWinSdkTasks, 0);

	// Visual Studio prompt: HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio
	RegEnumKeys(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\VisualStudio", CreateVCTasks, 0);

	SafeFree(szConEmuDrive.ms_Arg);

	// Choose default startup command
	if (slfFlags & (slf_DefaultSettings|slf_DefaultTasks))
	{
		Fast_FindStartupTask(slfFlags);
	}
}

void Fast_FindStartupTask(SettingsLoadedFlags slfFlags)
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

LPCWSTR Fast_GetStartupCommand(const CommandTasks*& pTask)
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
