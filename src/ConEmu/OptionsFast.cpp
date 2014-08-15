
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
#include "OptionsFast.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "../common/WinObjects.h"

static bool bCheckHooks, bCheckUpdate, bCheckIme;
// Если файл конфигурации пуст, то после вызова CheckOptionsFast
// все равно будет SaveSettings(TRUE/*abSilent*/);
// Поэтому выбранные настройки здесь можно не сохранять (кроме StopWarningConIme)
static bool bVanilla;


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

			LRESULT lbRc = FALSE;
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

					GetWindowRect(hDlg, &rcWnd);
					MoveWindow(hDlg, rcWnd.left, rcWnd.top+(nShift>>1), rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top-nShift, FALSE);
				}
			}

			return lbRc;
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
						wchar_t* pszNewPlace = GetDlgItemText(hDlg, lbStorageLocation);
						if (!gpConEmu->SetConfigFile(pszNewPlace, true))
						{
							// Ошибка уже показана
							return 1;
						}
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

		DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_FAST_CONFIG), NULL, CheckOptionsFastProc, (LPARAM)asTitle);
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
static bool FindOnDrives(LPCWSTR asFirstDrive, LPCWSTR asSearchPath, wchar_t (&rsFound)[MAX_PATH+1], bool& bNeedQuot)
{
	bool bFound = false;
	wchar_t* pszExpanded = NULL;
	wchar_t szDrive[4] = L"C:\\";
	bNeedQuot = false;

	// Using environment variables?
	if (wcschr(asSearchPath, L'%'))
	{
		pszExpanded = ExpandEnvStr(asSearchPath);
		if (pszExpanded && FileExists(pszExpanded))
		{
			bNeedQuot = IsPathNeedQuot(pszExpanded);
			lstrcpyn(rsFound, asSearchPath, countof(rsFound));
			bFound = true;
		}
		goto wrap;
	}

	// Only executable name was specified?
	if (!wcschr(asSearchPath, L'\\'))
	{
		DWORD nFind = SearchPath(NULL, asSearchPath, NULL, countof(rsFound), rsFound, NULL);
		if (nFind && nFind < countof(rsFound))
		{
			bNeedQuot = IsPathNeedQuot(asSearchPath);
			lstrcpyn(rsFound, asSearchPath, countof(rsFound));
			bFound = true;
		}
		goto wrap;
	}

	if (asFirstDrive && *asFirstDrive)
	{
		INT_PTR nDrvLen = _tcslen(asFirstDrive);
		lstrcpyn(rsFound, asFirstDrive, countof(rsFound));
		lstrcpyn(rsFound+nDrvLen, asSearchPath, countof(rsFound)-nDrvLen);
		if (FileExists(rsFound))
		{
			bNeedQuot = IsPathNeedQuot(rsFound);
			bFound = true;
			goto wrap;
		}
	}

	for (szDrive[0] = L'C'; szDrive[0] <= L'Z'; szDrive[0]++)
	{
		UINT nType = GetDriveType(szDrive);
		if (nType != DRIVE_FIXED)
			continue;
		lstrcpyn(rsFound, szDrive, countof(rsFound));
		lstrcpyn(rsFound+2, asSearchPath, countof(rsFound)-2);
		if (FileExists(rsFound))
		{
			bNeedQuot = IsPathNeedQuot(rsFound);
			bFound = true;
			goto wrap;
		}
	}

wrap:
	SafeFree(pszExpanded);
	return bFound;
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

	struct {
		LPCWSTR asName, asExePath, asArgs, asPrefix, asGuiArg;
		LPWSTR  pszFound;
	} FindTasks[] = {
		// Far Manager
		// -- {L"Far Manager",         L"%ConEmuDir%\\far.exe"}, 
		{L"Far Manager",         L"far.exe"},

		// TakeCommand
		{L"TCC",                 L"tcc.exe",                           NULL},
		{L"TCC (Admin)",         L"tcc.exe",                           L" -new_console:a"},

		// NYAOS - !!!TODO!!!
		{L"NYAOS",               L"nyaos.exe",                         NULL},
		{L"NYAOS (Admin)",       L"nyaos.exe",                         L" -new_console:a"},

		// Windows internal
		{L"cmd",                 L"cmd.exe",                           NULL},
		{L"cmd (Admin)",         L"cmd.exe",                           L" -new_console:a"},
		{L"PowerShell",          L"powershell.exe",                    NULL},
		{L"PowerShell (Admin)",  L"powershell.exe",                    L" -new_console:a"},

		// Bash

		// For cygwin we can check registry keys (todo?)
		// HKLM\SOFTWARE\Wow6432Node\Cygwin\setup\rootdir
		// HKLM\SOFTWARE\Cygwin\setup\rootdir
		// HKCU\Software\Cygwin\setup\rootdir
		{L"CygWin bash",    L"\\CygWin\\bin\\sh.exe",                  L" --login -i", L"set CHERE_INVOKING=1 & "},

//		{L"CygWin mintty",  L"\\CygWin\\bin\\mintty.exe",              L" -"},
		{L"MinGW bash",     L"\\MinGW\\msys\\1.0\\bin\\sh.exe",        L" --login -i"},
//		{L"MinGW mintty",   L"\\MinGW\\msys\\1.0\\bin\\mintty.exe",    L" -"},
		{L"Git bash",       L"%ProgramFiles%\\Git\\bin\\sh.exe",       L" --login -i"},
		{L"Git bash",       L"%ProgramW6432%\\Git\\bin\\sh.exe",       L" --login -i"},
		#ifdef _WIN64
		{L"Git bash",       L"%ProgramFiles(x86)%\\Git\\bin\\sh.exe",  L" --login -i"},
		#endif
		// Last chance for bash
		{L"bash",           L"sh.exe",                                 L" --login -i", L"set CHERE_INVOKING=1 & "},

		// Putty?
		{L"Putty",          L"Putty.exe",                              NULL},

		// C++ build environments (Visual Studio) - !!!TODO!!!

		// FIN
		{NULL}
	};

	wchar_t szFound[MAX_PATH+1], szUnexpand[MAX_PATH+1], *pszFull = NULL; bool bNeedQuot = false;
	for (int i = 0; FindTasks[i].asName; i++)
	{
		if (!FindOnDrives(szConEmuDrive, FindTasks[i].asExePath, szFound, bNeedQuot))
			continue;
		// May be it was already found before?
		bool bAlready = false;
		for (int j = 0; j < i; j++)
		{
			if (FindTasks[i].pszFound && lstrcmpi(szFound, FindTasks[i].pszFound) == 0)
			{
				bAlready = true; break;
			}
		}
		if (bAlready)
			continue;

		// Go
		FindTasks[i].pszFound = lstrdup(szFound);

		// Try to use system env vars?
		LPCWSTR pszFound = szFound;
		if (PathUnExpandEnvStrings(szFound, szUnexpand, countof(szUnexpand)))
			pszFound = szUnexpand;

		// Spaces in path? (use expanded path)
		if (bNeedQuot)
			pszFull = lstrmerge(FindTasks[i].asPrefix, L"\"", pszFound, L"\"", FindTasks[i].asArgs);
		else
			pszFull = lstrmerge(FindTasks[i].asPrefix, pszFound, FindTasks[i].asArgs);

		// Create task
		if (pszFull)
		{
			gpSet->CmdTaskSet(iCreatIdx++, FindTasks[i].asName, FindTasks[i].asGuiArg, pszFull);
			SafeFree(pszFull);
		}
	}

	// For 64bit Windows create task with splitted cmd 64/32
	if (IsWindows64())
	{
		gpSet->CmdTaskSet(iCreatIdx++, L"cmd 64/32", L"", L">\"%windir%\\system32\\cmd.exe\" /k ver & echo This is Native cmd.exe\r\n\r\n\"%windir%\\syswow64\\cmd.exe\" /k ver & echo This is 32 bit cmd.exe -new_console:s50V");
	}

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

	// Windows SDK
	HKEY hk;
	if (0 == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows", 0, KEY_READ, &hk))
	{
		UINT n = 0;
		wchar_t szVer[34] = L""; DWORD cchMax = countof(szVer) - 1;
		while (0 == RegEnumKeyEx(hk, n++, szVer, &cchMax, NULL, NULL, NULL, NULL))
		{
			HKEY hkVer;
			if (0 == RegOpenKeyEx(hk, szVer, 0, KEY_READ, &hkVer))
			{
				wchar_t szVerPath[MAX_PATH+1] = L"";
				DWORD cbSize = sizeof(szVerPath)-sizeof(szVerPath[0]);
				if (0 == RegQueryValueEx(hkVer, L"InstallationFolder", NULL, NULL, (LPBYTE)szVerPath, &cbSize))
				{
					wchar_t* pszCmd = lstrmerge(szVerPath, L"Bin\\SetEnv.Cmd");
					if (pszCmd && FileExists(pszCmd))
					{
						pszFull = lstrmerge(L"cmd /V /K \"", pszCmd, L"\"");
						// Create task
						if (pszFull)
						{
							wcscpy_c(szVerPath, L"WinSDK ");
							wcscat_c(szVerPath, szVer);
							gpSet->CmdTaskSet(iCreatIdx++, szVerPath, L"", pszFull);
							SafeFree(pszFull);
						}

					}
					SafeFree(pszCmd);
				}
				RegCloseKey(hkVer);
			}

			cchMax = countof(szVer) - 1;
		}
		RegCloseKey(hk);
	}

	for (int i = 0; FindTasks[i].asName; i++)
	{
		SafeFree(FindTasks[i].pszFound);
	}
}
