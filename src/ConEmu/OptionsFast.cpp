
/*
Copyright (c) 2009-2014 Maximus5
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
#include "Options.h"
#include "OptionsClass.h"
#include "OptionsFast.h"
#include "OptionsHelp.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "../common/WFiles.h"
#include "../common/WRegistry.h"

#define FOUND_APP_PATH_CHR L'\1'
#define FOUND_APP_PATH_STR L"\1"

static bool bCheckHooks, bCheckUpdate, bCheckIme;
// Если файл конфигурации пуст, то после вызова CheckOptionsFast
// все равно будет SaveSettings(TRUE/*abSilent*/);
// Поэтому выбранные настройки здесь можно не сохранять (кроме StopWarningConIme)
static bool bVanilla;
static CDpiForDialog* gp_DpiAware = NULL;
static CEHelpPopup* gp_FastHelp = NULL;

static INT_PTR CALLBACK CheckOptionsFastProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (messg)
	{
	case WM_SETHOTKEY:
		gnWndSetHotkey = wParam;
		break;

	case WM_INITDIALOG:
		{
			SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);

			if (gp_DpiAware)
			{
				gp_DpiAware->Attach(hDlg, NULL);
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
			wchar_t* pszSettingsPlaces[] = {
				lstrdup(L"HKEY_CURRENT_USER\\Software\\ConEmu"),
				ExpandEnvStr(L"%APPDATA%\\ConEmu.xml"),
				ExpandEnvStr(L"%ConEmuBaseDir%\\ConEmu.xml"),
				ExpandEnvStr(L"%ConEmuDir%\\ConEmu.xml"),
				NULL
			};
			int iAllowed = 0;
			if (lstrcmp(Storage.szType, CONEMU_CONFIGTYPE_XML) == 0)
			{
				iAllowed = 1; // Реестр уже низя
				if (Storage.pszFile)
				{
					if (lstrcmpi(Storage.pszFile, pszSettingsPlaces[1]) == 0)
						iAllowed = 1; // OK, перебить может любой другой xml
					else if (lstrcmpi(Storage.pszFile, pszSettingsPlaces[2]) == 0)
						iAllowed = 2; // "Перебить" может только %APPDATA%
					else if (lstrcmpi(Storage.pszFile, pszSettingsPlaces[3]) == 0)
						iAllowed = 3; // Приоритетнее настроек нет
					else
					{
						// Этот xml мог быть указан в "/LoadCfgFile ..."
						SafeFree(pszSettingsPlaces[3]);
						pszSettingsPlaces[3] = lstrdup(Storage.pszFile);
						iAllowed = 3; // Приоритетнее настроек нет
					}
				}
			}
			while (pszSettingsPlaces[iAllowed])
			{
				SendDlgItemMessage(hDlg, lbStorageLocation, CB_ADDSTRING, 0, (LPARAM)pszSettingsPlaces[iAllowed]);
				iAllowed++;
			}
			SendDlgItemMessage(hDlg, lbStorageLocation, CB_SETCURSEL, 0, 0);
			for (int i = 0; pszSettingsPlaces[i]; i++)
			{
				SafeFree(pszSettingsPlaces[i]);
			}


			// continue
			CheckDlgButton(hDlg, cbSingleInstance, gpSetCls->IsSingleInstanceArg());

			CheckDlgButton(hDlg, cbUseKeyboardHooksFast, gpSet->isKeyboardHooks(true));



			// Debug purposes only. ConEmu.exe switch "/nokeyhooks"
			#ifdef _DEBUG
			EnableWindow(GetDlgItem(hDlg, cbUseKeyboardHooksFast), !gpConEmu->DisableKeybHooks);
			#endif

			CheckDlgButton(hDlg, cbInjectConEmuHkFast, gpSet->isUseInjects);

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

			SetFocus(GetDlgItem(hDlg, IDOK));
			return FALSE; // Set focus to OK
		}
	case WM_CTLCOLORSTATIC:

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

		break;

	case WM_SETCURSOR:
		{
			if (((HWND)wParam) == GetDlgItem(hDlg, stHomePage))
			{
				SetCursor(LoadCursor(NULL, IDC_HAND));
				SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
				return TRUE;
			}
			return FALSE;
		}
		break;

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

		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				{
					SettingsStorage CurStorage = {}; bool ReadOnly = false;
					gpSet->GetSettingsType(CurStorage, ReadOnly);
					LRESULT lSelStorage = SendDlgItemMessage(hDlg, lbStorageLocation, CB_GETCURSEL, 0, 0);
					if (lSelStorage > 0)
					{
						// Значит юзер выбрал "создать настройки" в другом месте
						wchar_t* pszNewPlace = GetDlgItemTextPtr(hDlg, lbStorageLocation);
						if (!gpConEmu->SetConfigFile(pszNewPlace, true))
						{
							// Ошибка уже показана
							SafeFree(pszNewPlace);
							return 1;
						}
						SafeFree(pszNewPlace);
					}


					/* Force Single instance mode */
					gpSet->isSingleInstance = IsDlgButtonChecked(hDlg, cbSingleInstance);

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
							if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
							{
								/* Force Single instance mode */
								reg->Save(L"SingleInstance", gpSet->isSingleInstance);

								/* Install Keyboard hooks */
								reg->Save(L"KeyboardHooks", gpSet->m_isKeyboardHooks);

								/* Inject ConEmuHk.dll */
								reg->Save(L"UseInjects", gpSet->isUseInjects);

								/* Auto Update settings */
								reg->Save(L"Update.CheckOnStartup", gpSet->UpdSet.isUpdateCheckOnStartup);
								reg->Save(L"Update.CheckHourly", gpSet->UpdSet.isUpdateCheckHourly);
								reg->Save(L"Update.ConfirmDownload", gpSet->UpdSet.isUpdateConfirmDownload);
								reg->Save(L"Update.UseBuilds", gpSet->UpdSet.isUpdateUseBuilds);

								// Fast configuration done
								reg->CloseKey();
							}

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
				}
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
			}
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
	if (gpConEmu->IsFastSetupDisabled())
	{
		gpConEmu->LogString(L"CheckOptionsFast was skipped due to '/basic' or '/resetdefault' switch");

		goto checkDefaults;
	}

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

	if (bCheckHooks || bCheckUpdate || bCheckIme)
	{
		// First ShowWindow forced to use nCmdShow. This may be weird...
		SkipOneShowWindow();

		gp_DpiAware = new CDpiForDialog();
		gp_FastHelp = new CEHelpPopup;

		// Modal dialog (CreateDialog)

		DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_FAST_CONFIG), NULL, CheckOptionsFastProc, (LPARAM)asTitle);
		SafeDelete(gp_FastHelp);
		SafeDelete(gp_DpiAware);
	}

checkDefaults:
	// Always check, if task list is empty - fill with defaults
	CreateDefaultTasks();
	// Some other settings, which must be filled with predefined values
	if (slfFlags & slf_DefaultSettings)
	{
		gpSet->CreatePredefinedPalettes(0);
	}
}





/* **************************************** */
/*         Creating default tasks           */
/* **************************************** */

// Search on asFirstDrive and all (other) fixed drive letters
// asFirstDrive may be letter ("C:") or network (\\server\share)
// asSearchPath is path to executable (\cygwin\bin\sh.exe)
static bool FindOnDrives(LPCWSTR asFirstDrive, LPCWSTR asSearchPath, CEStr& rsFound, bool& bNeedQuot)
{
	bool bFound = false;
	wchar_t* pszExpanded = NULL;
	wchar_t szDrive[4]; // L"C:"
	wchar_t szTemp[MAX_PATH+1];

	bNeedQuot = false;

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
					bNeedQuot = IsQuotationNeeded(pszExpanded);
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
			rsFound.Set(asSearchPath);
			bFound = true;
		}
		// Search in [HKCU|HKLM]\Software\Microsoft\Windows\CurrentVersion\App Paths
		if (SearchAppPaths(asSearchPath, rsFound, false/*abSetPath*/))
		{
			bNeedQuot = IsQuotationNeeded(asSearchPath);
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
		CEStr pszCmd = JoinPath(pszVerPath, L"Bin\\SetEnv.Cmd");
		if (pszCmd && FileExists(pszCmd))
		{
			CEStr pszFull = lstrmerge(L"cmd /V /K \"", pszCmd, L"\" -new_console:t:\"WinSDK ", pszVer, L"\"");
			// Create task
			if (pszFull)
			{
				CEStr pszName = lstrmerge(L"WinSDK ", pszVer);
				if (pszName)
				{
					gpSet->CmdTaskSet((*piCreatIdx)++, pszName, L"", pszFull);
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
				CEStr pszName = lstrmerge(L"VS ", pszVer, L" x86 tools prompt");
				CEStr pszFull = lstrmerge(L"cmd /k \"\"", pszVcVarsBat, L"\"\" x86 -new_console:t:\"VS ", pszVer, L"\"");
				gpSet->CmdTaskSet((*piCreatIdx)++, pszName, L"", pszFull);
			}
		}
	}

	return true; // continue reg enum
}

void CreateDefaultTask(LPCWSTR asDrive, int& iCreatIdx, LPCWSTR asName, LPCWSTR asArgs, LPCWSTR asPrefix, LPCWSTR asGuiArg, LPCWSTR asExePath, ...)
{
	va_list argptr;
	va_start(argptr, asExePath);

	CEStr szFound, szArgs;
	wchar_t szUnexpand[MAX_PATH+32], *pszFull = NULL; bool bNeedQuot = false;
	MArray<wchar_t*> lsList;

	while (asExePath)
	{
		LPCWSTR pszExePath = asExePath;
		asExePath = va_arg( argptr, LPCWSTR );

		// Return expanded env string
		if (!FindOnDrives(asDrive, pszExePath, szFound, bNeedQuot))
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
		if (PathUnExpandEnvStrings(szFound, szUnexpand, countof(szUnexpand)) && (lstrcmp(szFound, szUnexpand) != 0))
		{
			pszFound = szUnexpand;
		}
		else if (asDrive && (lstrcmpni(szFound, asDrive, lstrlen(asDrive)) == 0))
		{
			wcscpy_c(szUnexpand, L"%ConEmuDrive%");
			wcscat_c(szUnexpand, szFound+2);
			pszFound = szUnexpand;
		}

		if (asArgs && wcschr(asArgs, FOUND_APP_PATH_CHR))
		{
			CEStr szPath(lstrdup(pszFound));
			if (!szPath.IsEmpty() && szArgs.Set(asArgs))
			{
				wchar_t* ptr;
				ptr = wcsrchr(szPath.ms_Arg, L'\\');
				if (ptr) *ptr = 0;
				while ((ptr = wcschr(szArgs.ms_Arg, FOUND_APP_PATH_CHR)) != NULL)
				{
					*ptr = 0;
					LPCWSTR pszTail = ptr+1;
					ptr = (wcsncmp(pszTail, L"..\\", 3) != NULL) ? wcsrchr(szPath.ms_Arg, L'\\') : NULL;
					if (ptr)
					{
						*ptr = 0;
						pszTail += 3;
					}

					CEStr szTemp(JoinPath(szPath, pszTail));
					szArgs.Attach(lstrmerge(szArgs.ms_Arg, szTemp));

					if (ptr) *ptr = L'\\';
				}
			}
			// Succeeded?
			if (!szArgs.IsEmpty())
			{
				asArgs = szArgs.ms_Arg;
			}
		}

		// Spaces in path? (use expanded path)
		if (bNeedQuot)
			pszFull = lstrmerge(asPrefix, L"\"", pszFound, L"\"", asArgs);
		else
			pszFull = lstrmerge(asPrefix, pszFound, asArgs);

		// Create task
		if (pszFull)
		{
			gpSet->CmdTaskSet(iCreatIdx++, asName, asGuiArg, pszFull);
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

void CreateFarTasks(LPCWSTR asDrive, int& iCreatIdx)
{
	CreateDefaultTask(asDrive, iCreatIdx, L"Far Manager", NULL, NULL, NULL, L"far.exe", NULL);
}

void CreateDefaultTasks(bool bForceAdd /*= false*/)
{
	int iCreatIdx = 0;

	if (!bForceAdd)
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

	CEStr szFound;
	wchar_t *pszFull; bool bNeedQuot = false;

	// Far Manager
	CreateFarTasks(szConEmuDrive, iCreatIdx);

	// TakeCommand
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"TCC", NULL, NULL, NULL, L"tcc.exe", NULL);
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"TCC (Admin)", L" -new_console:a", NULL, NULL, L"tcc.exe", NULL);

	// NYAOS - !!!Registry TODO!!!
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"NYAOS", NULL, NULL, NULL, L"nyaos.exe", NULL);
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"NYAOS (Admin)", L" -new_console:a", NULL, NULL, L"nyaos.exe", NULL);

	// Windows internal
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"cmd", NULL, L"set PROMPT=$E[92m$P$E[90m$G$E[m$S & ", NULL, L"cmd.exe", NULL);
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"cmd (Admin)", L" -new_console:a", L"set PROMPT=$E[91m$P$E[90m$G$E[m$S & ", NULL, L"cmd.exe", NULL);
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"PowerShell", NULL, NULL, NULL, L"powershell.exe", NULL);
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"PowerShell (Admin)", L" -new_console:a", NULL, NULL, L"powershell.exe", NULL);
	// For 64bit Windows create task with splitted cmd 64/32
	if (IsWindows64())
		gpSet->CmdTaskSet(iCreatIdx++, L"cmd 64/32", L"", L"> set PROMPT=$E[92m$P$E[90m$G$E[m$S & \"%windir%\\system32\\cmd.exe\" /k ver & echo This is Native cmd.exe\r\n\r\nset PROMPT=$E[93m$P$E[90m$G$E[m$S & \"%windir%\\syswow64\\cmd.exe\" /k ver & echo This is 32 bit cmd.exe -new_console:s50V");

	// Bash

	// For cygwin we can check registry keys (todo?)
	// HKLM\SOFTWARE\Wow6432Node\Cygwin\setup\rootdir
	// HKLM\SOFTWARE\Cygwin\setup\rootdir
	// HKCU\Software\Cygwin\setup\rootdir
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"CygWin bash", L" --login -i", L"set CHERE_INVOKING=1 & ", NULL, L"\\CygWin\\bin\\sh.exe", NULL);
	//{L"CygWin mintty", L"\\CygWin\\bin\\mintty.exe", L" -"},
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"MinGW bash",  L" --login -i", L"set CHERE_INVOKING=1 & ", NULL, L"\\MinGW\\msys\\1.0\\bin\\sh.exe", NULL);
	//{L"MinGW mintty", L"\\MinGW\\msys\\1.0\\bin\\mintty.exe", L" -"},
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Git bash", L" --login -i", NULL, NULL,
		L"%ProgramFiles%\\Git\\bin\\sh.exe", L"%ProgramW6432%\\Git\\bin\\sh.exe",
		#ifdef _WIN64
		L"%ProgramFiles(x86)%\\Git\\bin\\sh.exe",
		#endif
		NULL);
	// Last chance for bash
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"bash", L" --login -i", L"set CHERE_INVOKING=1 & ", NULL, L"sh.exe", NULL);

	// Putty?
	CreateDefaultTask(szConEmuDrive, iCreatIdx, L"Putty", NULL, NULL, NULL, L"Putty.exe", NULL);

	// IRSSI
	// L"\"set PATH=C:\\irssi\\bin;%PATH%\" & set PERL5LIB=lib/perl5/5.8 & set TERMINFO_DIRS=terminfo & "
	// L"C:\\irssi\\bin\\irssi.exe"
	// L" -cur_console:d:\"C:\\irssi\""

	// Type ANSI color codes
	// cmd /k type "%ConEmuBaseDir%\Addons\AnsiColors16t.ans" -cur_console:n
	if (FindOnDrives(NULL, L"%ConEmuBaseDir%\\Addons\\AnsiColors16t.ans", szFound, bNeedQuot))
	{
		gpSet->CmdTaskSet(iCreatIdx++, L"Show ANSI colors", L"", L"cmd /k type \"%ConEmuBaseDir%\\Addons\\AnsiColors16t.ans\" -cur_console:n");
	}

	// Chocolatey gallery
	//-- gpSet->CmdTaskSet(iCreatIdx++, L"Chocolatey", L"", L"*cmd /k powershell -NoProfile -ExecutionPolicy unrestricted -Command \"iex ((new-object net.webclient).DownloadString('https://chocolatey.org/install.ps1'))\" && SET PATH=%PATH%;%systemdrive%\\chocolatey\\bin");
	// @echo If you don't know about Chocolatey - read about it here https://chocolatey.org/
	// powershell -NoProfile -ExecutionPolicy unrestricted -Command "iex ((new-object net.webclient).DownloadString('https://chocolatey.org/install.ps1'))" && SET PATH=%PATH%;%systemdrive%\chocolatey\bin
	//-- that will be too long and unfriendly
	// gpSet->CmdTaskSet(iCreatIdx++, L"Chocolatey (Admin)", L"", L"*cmd /k Title Chocolatey & @echo [1;32;40m******************************************************************** & @echo [1;32;40m** [0mIf you[1;31;40m don't know about Chocolatey [0m(apt-get style manager)     [1;32;40m** & @echo [1;32;40m** [1;31;40mread about it[0m here:[1;32;40m https://chocolatey.org/                    [1;32;40m** & @echo [1;32;40m** If you are sure about installing it, execute the following     [1;32;40m** & @echo [1;32;40m** [1;31;40mone-line command:                                              [1;32;40m** & @echo [1;32;40m** [1;37;40mpowershell -NoProfile -ExecutionPolicy unrestricted            [1;32;40m** & @echo [1;32;40m** [1;37;40m-Command @echo ^\"iex ((new-object net.webclient).DownloadString [1;32;40m** & @echo [1;32;40m** [1;37;40m('https://chocolatey.org/install.ps1'))^\"                       [1;32;40m** & @echo [1;32;40m** [1;37;40m^&^& SET PATH=^%PATH^%;^%systemdrive^%\\chocolatey\\bin                [1;32;40m** & @echo [1;32;40m********************************************************************[0m");
	pszFull = ExpandEnvStr(L"%ConEmuBaseDir%\\Addons\\ChocolateyAbout.cmd");
	if (pszFull && FileExists(pszFull))
		gpSet->CmdTaskSet(iCreatIdx++, L"Chocolatey (Admin)", L"", L"*cmd /k Title Chocolatey & \"%ConEmuBaseDir%\\Addons\\ChocolateyAbout.cmd\"");
	SafeFree(pszFull);

	// Windows SDK: HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows
	RegEnumKeys(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows", CreateWinSdkTasks, (LPARAM)&iCreatIdx);

	// Visual Studio prompt: HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio
	RegEnumKeys(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\VisualStudio", CreateVCTasks, (LPARAM)&iCreatIdx);
}
