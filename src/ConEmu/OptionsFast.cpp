
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

void Fast_FindStartupTask();
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

	RECT rect = {};
	if (GetWindowRect(hDlg, &rect))
	{
		CDpiAware::GetCenteredRect(NULL, rect);
		MoveWindowRect(hDlg, rect);
	}

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
			SetWindowText(h, gsHomePage);

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
			ConEmuAbout::OnInfo_HomePage();
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

static bool sbAppendMode = false;

static void CreateDefaultTask(int& iCreatIdx, LPCWSTR asName, LPCWSTR asGuiArg, LPCWSTR asCommands)
{
	// Don't add duplicates in the append mode
	if (sbAppendMode)
	{
		const CommandTasks* pTask = gpSet->CmdTaskGetByName(asName);
		if (pTask != NULL)
			return;
	}

	gpSet->CmdTaskSet(iCreatIdx++, asName, asGuiArg, asCommands);
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
	int* piCreatIdx = (int*)lParam;
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
					CreateDefaultTask(*piCreatIdx, szName, L"", szFull);
				}
			}
		}
	}

	return true; // continue reg enum
}

// Visual Studio C++
static bool WINAPI CreateVCTasks(HKEY hkVer, LPCWSTR pszVer, LPARAM lParam)
{
	int* piCreatIdx = (int*)lParam;
	//[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\11.0\Setup\VC]
	//"ProductDir"="C:\\Program Files (x86)\\Microsoft Visual Studio 11.0\\VC\\"
	//[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\12.0\Setup\VC]
	//"ProductDir"="C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\"
	// %comspec% /k ""C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"" x86

	CEStr pszDir;
	if (wcschr(pszVer, L'.'))
	{
		if (RegGetStringValue(hkVer, L"Setup\\VC", L"ProductDir", pszDir) > 0)
		{
			CEStr pszVcVarsBat = JoinPath(pszDir, L"vcvarsall.bat");
			if (FileExists(pszVcVarsBat))
			{
				CEStr pszName = lstrmerge(L"SDK::VS ", pszVer, L" x86 tools prompt");
				CEStr pszFull = lstrmerge(L"cmd /k \"\"", pszVcVarsBat, L"\"\" x86 -new_console:t:\"VS ", pszVer, L"\"");
				CreateDefaultTask(*piCreatIdx, pszName, L"", pszFull);
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

static void CreateDefaultTask(LPCWSTR asDrive, int& iCreatIdx, LPCWSTR asName, LPCWSTR asArgs, LPCWSTR asPrefix, LPCWSTR asGuiArg, LPCWSTR asExePath, ...)
{
	va_list argptr;
	va_start(argptr, asExePath);

	CEStr szFound, szArgs, szOptFull;
	wchar_t szUnexpand[MAX_PATH+32], *pszFull = NULL; bool bNeedQuot = false;
	MArray<wchar_t*> lsList;

	while (asExePath)
	{
		LPCWSTR pszExePath = asExePath;
		asExePath = va_arg( argptr, LPCWSTR );

		// Return expanded env string
		if (!FindOnDrives(asDrive, pszExePath, szFound, bNeedQuot, szOptFull))
			continue;

		// May be it was already found before?
		bool bAlready = false;
		for (INT_PTR j = 0; j < lsList.size(); j++)
		{
			LPCWSTR pszPrev = lsList[j];
			if (pszPrev && lstrcmpi(szFound, pszPrev) == 0)
			{
				bAlready = true; break;
			}
		}
		if (bAlready)
			continue;

		// Go
		lsList.push_back(lstrdup(szFound));

		// Try to use system env vars?
		LPCWSTR pszFound = szFound;
		// Don't use PathUnExpandEnvStrings because it do not do what we need
		if (UnExpandEnvStrings(szFound, szUnexpand, countof(szUnexpand)) && (lstrcmp(szFound, szUnexpand) != 0))
		{
			pszFound = szUnexpand;
		}

		LPCWSTR pszArgs = asArgs;
		if (pszArgs && wcschr(pszArgs, FOUND_APP_PATH_CHR))
		{
			if (pszFound && *pszFound && szArgs.Set(pszArgs))
			{
				CEStr szPath;
				wchar_t *ptrFound, *ptrAdd;
				while ((ptrAdd = wcschr(szArgs.ms_Arg, FOUND_APP_PATH_CHR)) != NULL)
				{
					*ptrAdd = 0;
					LPCWSTR pszTail = ptrAdd+1;

					szPath.Set(pszFound);
					ptrFound = wcsrchr(szPath.ms_Arg, L'\\');
					if (ptrFound) *ptrFound = 0;

					if (*pszTail == L'\\') pszTail ++;
					while (wcsncmp(pszTail, L"..\\", 3) == 0)
					{
						ptrAdd = wcsrchr(szPath.ms_Arg, L'\\');
						if (ptrAdd)
						{
							*ptrAdd = 0;
							pszTail += 3;
						}
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
		if (bNeedQuot)
			pszFull = lstrmerge(asPrefix, L"\"", pszFound, L"\"", pszArgs);
		else
			pszFull = lstrmerge(asPrefix, pszFound, pszArgs);

		// Create task
		if (pszFull)
		{
			CreateDefaultTask(iCreatIdx, asName, asGuiArg, pszFull);
			SafeFree(pszFull);
		}
	}

	for (INT_PTR j = 0; j < lsList.size(); j++)
	{
		wchar_t* p = lsList[j];
		SafeFree(p);
	}

	va_end(argptr);
}

class FarVerList
{
public:
	struct FarInfo
	{
		wchar_t* szFullPath;
		wchar_t szTaskName[64];
		FarVersion Ver; // bool LoadFarVersion(FarVersion& gFarVersion, wchar_t (&ErrText)[512])
	};
	MArray<FarInfo> Installed;

protected:
	wchar_t szFar32Name[16], szFar64Name[16];
	LPCWSTR FarExe[3]; // = { szFar64Name, szFar32Name, NULL };

protected:
	// This will load Far version and check its existence
	bool AddFarPath(LPCWSTR szPath, LPCWSTR pszOptFull)
	{
		bool bAdded = false;
		FarInfo FI = {};
		wchar_t ErrText[512];

		_ASSERTE(!pszOptFull || *pszOptFull);
		LPCWSTR pszPath = pszOptFull ? pszOptFull : szPath;

		if (LoadFarVersion(pszPath, FI.Ver, ErrText))
		{
			DWORD ImageSubsystem = 0, FileAttrs = 0;
			GetImageSubsystem(pszPath, ImageSubsystem, FI.Ver.dwBits, FileAttrs);

			// Far instance found, add it to Installed array?
			bool bAlready = false;
			for (INT_PTR a = 0; a < Installed.size(); a++)
			{
				if (lstrcmpi(Installed[a].szFullPath, szPath) == 0)
				{
					bAlready = true; break; // Do not add twice same path
				}
				else if (pszOptFull && (lstrcmpi(Installed[a].szFullPath, pszOptFull) == 0))
				{
					SafeFree(Installed[a].szFullPath);
					Installed[a].szFullPath = lstrdup(szPath);
					bAlready = true; break; // Do not add twice same path
				}
			}
			if (!bAlready)
			{
				wcscpy_c(FI.szTaskName, L"Far");
				FI.szFullPath = lstrdup(szPath);
				if (FI.szFullPath)
				{
					Installed.push_back(FI);
					bAdded = true;
				}
			}
		}

		return bAdded;
	}; // AddFarPath(LPCWSTR szPath)

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
								AddFarPath(szPath, NULL);
							}
						}
					}
				}
			}
		}
	}; // ScanRegistry()

public:
	void FindInstalledVersions(LPCWSTR asDrive)
	{
		CEStr szFound, szOptFull;
		bool bNeedQuot = false;
		INT_PTR i;

		const wchar_t szFarPrefix[] = L"Far Manager::";

		// Scan our program dir subfolders
		for (i = 0; FarExe[i]; i++)
		{
			if (FileExistSubDir(gpConEmu->ms_ConEmuExeDir, FarExe[i], 1, szFound))
				AddFarPath(szFound, NULL);
		}

		// Check registry
		ScanRegistry();

		// Find in %Path% and on drives
		for (i = 0; FarExe[i]; i++)
		{
			if (FindOnDrives(asDrive, FarExe[i], szFound, bNeedQuot, szOptFull))
				AddFarPath(szFound, szOptFull.IsEmpty() ? NULL : szOptFull.ms_Arg);
		}

		// [HKCU|HKLM]\Software\Microsoft\Windows\CurrentVersion\App Paths
		for (i = 0; FarExe[i]; i++)
		{
			if (SearchAppPaths(FarExe[i], szFound, false))
				AddFarPath(szFound, NULL);
		}

		// Done, create task names
		// If there is only one found instance - just use name {Far}
		if (Installed.size() > 1)
		{
			UINT idx = 0;
			LPCWSTR pszPrefix = (Installed.size() > 1) ? szFarPrefix : L"";

			struct impl {
				static int SortRoutine(FarInfo &e1, FarInfo &e2)
				{
					if (e1.Ver.dwVer < e2.Ver.dwVer)
						return 1;
					if (e1.Ver.dwVer > e2.Ver.dwVer)
						return -1;
					if (e1.Ver.dwBuild < e2.Ver.dwBuild)
						return 1;
					if (e1.Ver.dwBuild > e2.Ver.dwBuild)
						return -1;
					if (e1.Ver.dwBits < e2.Ver.dwBits)
						return 1;
					if (e1.Ver.dwBits > e2.Ver.dwBits)
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
					FarInfo& FI = Installed[i];
					wchar_t szPlatform[6]; wcscpy_c(szPlatform, (FI.Ver.dwBits == 64) ? L" x64" : (FI.Ver.dwBits == 32) ? L" x86" : L"");

					switch (u)
					{
					case 0: // Naked
						_wsprintf(FI.szTaskName, SKIPLEN(countof(FI.szTaskName)-16) L"%sFar %u.%u%s",
							pszPrefix, FI.Ver.dwVerMajor, FI.Ver.dwVerMinor, szPlatform);
						break;
					case 1: // Add Far Build no?
						_wsprintf(FI.szTaskName, SKIPLEN(countof(FI.szTaskName)-16) L"%sFar %u.%u.%u%s",
							pszPrefix, FI.Ver.dwVerMajor, FI.Ver.dwVerMinor, FI.Ver.dwBuild, szPlatform);
						break;
					case 2: // Add Build and index
						_wsprintf(FI.szTaskName, SKIPLEN(countof(FI.szTaskName)-16) L"%sFar %u.%u.%u%s (%u)",
							pszPrefix, FI.Ver.dwVerMajor, FI.Ver.dwVerMinor, FI.Ver.dwBuild, szPlatform, ++idx);
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

void CreateFarTasks(LPCWSTR asDrive, int& iCreatIdx)
{
	FarVerList Vers;
	Vers.FindInstalledVersions(asDrive);

	// Create Far tasks
	for (INT_PTR i = 0; i < Vers.Installed.size(); i++)
	{
		FarVerList::FarInfo& FI = Vers.Installed[i];
		bool bNeedQuot = (wcschr(FI.szFullPath, L' ') != NULL);
		LPWSTR pszFullPath = FI.szFullPath;
		wchar_t szUnexpanded[MAX_PATH];
		if (wcschr(pszFullPath, L'\\') && UnExpandEnvStrings(pszFullPath, szUnexpanded, countof(szUnexpanded)))
			pszFullPath = szUnexpanded;

		wchar_t* pszCommand = NULL;

		// We have to set FARHOME variable before starting Far (proper plugins folder)
		LPWSTR pszExeName = (LPWSTR)PointToName(pszFullPath);
		if (pszExeName)
		{
			if (pszExeName > pszFullPath)
			{
				wchar_t chSave = *(pszExeName-1);
				*(pszExeName-1) = 0;
				pszCommand = lstrmerge(L"set \"FARHOME=", pszFullPath, L"\" & \"%FARHOME%\\", pszExeName, L"\"");
				*(pszExeName-1) = chSave;
			}
			else
			{
				// No path? that is strange
				_ASSERTE(!bNeedQuot);
				// Reset 'FARHOME' env.var before starting far.exe!
				// Otherwise, we can inherit '%FARHOME%' from parent process and when far.exe starts
				// it will get already expanded command line which may has erroneous path.
				// That's very bad when running x64 Far, but %FARHOME% points to x86 Far.
				pszCommand = lstrmerge(L"set \"FARHOME=\" & \"", pszFullPath, L"\"");
			}

			if (pszCommand)
			{
				if (FI.Ver.dwVerMajor >= 2)
					lstrmerge(&pszCommand, L" /w");

				lstrmerge(&pszCommand, L" /p\"%ConEmuDir%\\Plugins\\ConEmu;%FARHOME%\\Plugins\"");

				if (gn_FirstFarTask == -1)
					gn_FirstFarTask = iCreatIdx;

				CreateDefaultTask(iCreatIdx, FI.szTaskName, NULL, pszCommand);
			}
		}

		// Release memory
		SafeFree(FI.szFullPath);
		SafeFree(pszCommand);
	}
	Vers.Installed.clear();
}

void CreateTccTasks(LPCWSTR asDrive, int& iCreatIdx)
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

	// Add tasks
	CreateDefaultTask(asDrive, iCreatIdx, L"Shells::TCC", NULL, NULL, NULL, pszTcc, NULL);
	CreateDefaultTask(asDrive, iCreatIdx, L"Shells::TCC (Admin)", L" -new_console:a", NULL, NULL, pszTcc, NULL);

	// separate x64 version?
	if (pszTcc64)
	{
		CreateDefaultTask(asDrive, iCreatIdx, L"Shells::TCC x64", NULL, NULL, NULL, pszTcc64, NULL);
		CreateDefaultTask(asDrive, iCreatIdx, L"Shells::TCC x64 (Admin)", L" -new_console:a", NULL, NULL, pszTcc64, NULL);
	}
}

void CreateDefaultTasks(SettingsLoadedFlags slfFlags)
{
	int iCreatIdx = 0;

	sbAppendMode = ((slfFlags & slf_AppendTasks) == slf_AppendTasks);
	gn_FirstFarTask = -1;

	if (!sbAppendMode)
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

	wchar_t szConEmuDrive[MAX_PATH] = L"";
	GetDrive(gpConEmu->ms_ConEmuExeDir, szConEmuDrive, countof(szConEmuDrive));
	_ASSERTE(szConEmuDrive[0] && szConEmuDrive[_tcslen(szConEmuDrive)-1] != L'\\'); // Supposed to be simple "C:"

	// Force use of "%ConEmuDrive%" instead of "%SystemDrive%"
	CEStr sysSave;
	sysSave.SaveEnvVar(L"SystemDrive", NULL);

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
	CreateFarTasks(szConEmuDrive, iCreatIdx);

	// TakeCommand
	CreateTccTasks(szConEmuDrive, iCreatIdx);

	// NYAOS - !!!Registry TODO!!!
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Shells::NYAOS", NULL, NULL, NULL, L"nyaos.exe", NULL);
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"NYAOS (Admin)", L" -new_console:a", NULL, NULL, L"nyaos.exe", NULL);

	// Windows internal: cmd
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Shells::cmd",
		L" /k \"%ConEmuBaseDir%\\CmdInit.cmd\"", NULL, NULL, L"cmd.exe", NULL);
	#if 0
	// Need to "set" ConEmuGitPath to full path to the git.exe
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Shells::cmd+git",
		L" /k \"%ConEmuBaseDir%\\CmdInit.cmd\" /git", NULL, NULL, L"cmd.exe", NULL);
	#endif
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Shells::cmd (Admin)",
		L" /k \"%ConEmuBaseDir%\\CmdInit.cmd\" -new_console:a", NULL, NULL, L"cmd.exe", NULL);
	// Windows internal: For 64bit Windows create task with splitted cmd 64/32 (Example)
	if (IsWindows64())
	{
		CreateDefaultTask(iCreatIdx, L"Shells::cmd 64/32", L"",
			L"> \"%windir%\\system32\\cmd.exe\" /k \"\"%ConEmuBaseDir%\\CmdInit.cmd\" & echo This is Native cmd.exe\""
			L"\r\n\r\n"
			L"\"%windir%\\syswow64\\cmd.exe\" /k \"\"%ConEmuBaseDir%\\CmdInit.cmd\" & echo This is 32 bit cmd.exe -new_console:s50V\"");
	}

	// Windows internal: PowerShell
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Shells::PowerShell",
		NULL, NULL, NULL, L"powershell.exe", NULL);
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Shells::PowerShell (Admin)",
		L" -new_console:a", NULL, NULL, L"powershell.exe", NULL);

	// Bash

	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Bash::Git bash 2",
		L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\..\\mingw32\\share\\git\\git-for-windows.ico\"", NULL, NULL,
		L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1:InstallLocation]\\usr\\bin\\sh.exe",
		L"%ProgramFiles%\\Git\\usr\\bin\\sh.exe", L"%ProgramW6432%\\Git\\usr\\bin\\sh.exe",
		#ifdef _WIN64
		L"%ProgramFiles(x86)%\\Git\\usr\\bin\\sh.exe",
		#endif
		NULL);
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Bash::Git bash",
		L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\etc\\git.ico\"", NULL, NULL,
		L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1:InstallLocation]\\bin\\sh.exe",
		L"%ProgramFiles%\\Git\\bin\\sh.exe", L"%ProgramW6432%\\Git\\bin\\sh.exe",
		#ifdef _WIN64
		L"%ProgramFiles(x86)%\\Git\\bin\\sh.exe",
		#endif
		NULL);
	// For cygwin we can check registry keys (todo?)
	// HKLM\SOFTWARE\Wow6432Node\Cygwin\setup\rootdir
	// HKLM\SOFTWARE\Cygwin\setup\rootdir
	// HKCU\Software\Cygwin\setup\rootdir
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Bash::CygWin bash",
		L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\Cygwin.ico\"", L"set CHERE_INVOKING=1 & ", NULL,
		L"\\CygWin\\bin\\sh.exe", NULL);
	//{L"CygWin mintty", L"\\CygWin\\bin\\mintty.exe", L" -"},
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Bash::MinGW bash",
		L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\msys.ico\"", L"set CHERE_INVOKING=1 & ", NULL,
		L"\\MinGW\\msys\\1.0\\bin\\sh.exe", NULL);
	//{L"MinGW mintty", L"\\MinGW\\msys\\1.0\\bin\\mintty.exe", L" -"},
	// MSys2 project: 'HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\MSYS2 32bit'
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Bash::Msys2-64",
		L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\..\\msys2.ico\"", L"set CHERE_INVOKING=1 & ", NULL,
		L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MSYS2 64bit:InstallLocation]\\usr\\bin\\sh.exe",
		L"msys64\\usr\\bin\\sh.exe",
		NULL);
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Bash::Msys2-32",
		L" --login -i -new_console:C:\"" FOUND_APP_PATH_STR L"\\..\\..\\msys2.ico\"", L"set CHERE_INVOKING=1 & ", NULL,
		L"[SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MSYS2 32bit:InstallLocation]\\usr\\bin\\sh.exe",
		L"msys32\\usr\\bin\\sh.exe",
		NULL);
	// Last chance for bash
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Bash::bash", L" --login -i", L"set CHERE_INVOKING=1 & ", NULL, L"sh.exe", NULL);

	// Putty?
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Putty", NULL, NULL, NULL, L"Putty.exe", NULL);

	// IRSSI
	// L"\"set PATH=C:\\irssi\\bin;%PATH%\" & set PERL5LIB=lib/perl5/5.8 & set TERMINFO_DIRS=terminfo & "
	// L"C:\\irssi\\bin\\irssi.exe"
	// L" -cur_console:d:\"C:\\irssi\""

	// Type ANSI color codes
	// cmd /k type "%ConEmuBaseDir%\Addons\AnsiColors16t.ans" -cur_console:n
	if (FindOnDrives(NULL, L"%ConEmuBaseDir%\\Addons\\AnsiColors16t.ans", szFound, bNeedQuot, szOptFull))
	{
		CreateDefaultTask(iCreatIdx, L"Tests::Show ANSI colors", L"", L"cmd /k type \"%ConEmuBaseDir%\\Addons\\AnsiColors16t.ans\" -cur_console:n");
	}

	// Chocolatey gallery
	//-- CreateDefaultTask(iCreatIdx, L"Chocolatey", L"", L"*cmd /k powershell -NoProfile -ExecutionPolicy unrestricted -Command \"iex ((new-object net.webclient).DownloadString('https://chocolatey.org/install.ps1'))\" && SET PATH=%PATH%;%systemdrive%\\chocolatey\\bin");
	// @echo If you don't know about Chocolatey - read about it here https://chocolatey.org/
	// powershell -NoProfile -ExecutionPolicy unrestricted -Command "iex ((new-object net.webclient).DownloadString('https://chocolatey.org/install.ps1'))" && SET PATH=%PATH%;%systemdrive%\chocolatey\bin
	//-- that will be too long and unfriendly
	// CreateDefaultTask(iCreatIdx, L"Chocolatey (Admin)", L"", L"*cmd /k Title Chocolatey & @echo [1;32;40m******************************************************************** & @echo [1;32;40m** [0mIf you[1;31;40m don't know about Chocolatey [0m(apt-get style manager)     [1;32;40m** & @echo [1;32;40m** [1;31;40mread about it[0m here:[1;32;40m https://chocolatey.org/                    [1;32;40m** & @echo [1;32;40m** If you are sure about installing it, execute the following     [1;32;40m** & @echo [1;32;40m** [1;31;40mone-line command:                                              [1;32;40m** & @echo [1;32;40m** [1;37;40mpowershell -NoProfile -ExecutionPolicy unrestricted            [1;32;40m** & @echo [1;32;40m** [1;37;40m-Command @echo ^\"iex ((new-object net.webclient).DownloadString [1;32;40m** & @echo [1;32;40m** [1;37;40m('https://chocolatey.org/install.ps1'))^\"                       [1;32;40m** & @echo [1;32;40m** [1;37;40m^&^& SET PATH=^%PATH^%;^%systemdrive^%\\chocolatey\\bin                [1;32;40m** & @echo [1;32;40m********************************************************************[0m");
	pszFull = ExpandEnvStr(L"%ConEmuBaseDir%\\Addons\\ChocolateyAbout.cmd");
	if (pszFull && FileExists(pszFull))
		CreateDefaultTask(iCreatIdx, L"Scripts::Chocolatey (Admin)", L"", L"*cmd /k Title Chocolatey & \"%ConEmuBaseDir%\\Addons\\ChocolateyAbout.cmd\"");
	SafeFree(pszFull);

	// Windows SDK: HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows
	RegEnumKeys(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows", CreateWinSdkTasks, (LPARAM)&iCreatIdx);

	// Visual Studio prompt: HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio
	RegEnumKeys(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\VisualStudio", CreateVCTasks, (LPARAM)&iCreatIdx);

	// Choose default startup command
	if (slfFlags & (slf_DefaultSettings|slf_DefaultTasks))
	{
		Fast_FindStartupTask();
	}
}

void Fast_FindStartupTask()
{
	const CommandTasks* pTask = NULL;

	if (gn_FirstFarTask != -1)
		pTask = gpSet->CmdTaskGet(gn_FirstFarTask);

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
