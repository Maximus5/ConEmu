
/*
Copyright (c) 2009-2012 Maximus5
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


#include "Header.h"
#include "Options.h"
#include "OptionsFast.h"
#include "ConEmu.h"

static bool bCheckHooks, bCheckUpdate, bCheckIme;


static INT_PTR CALLBACK CheckOptionsFastProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (messg)
	{
	case WM_INITDIALOG:
		{
			LRESULT lbRc = FALSE;
			wchar_t szTitle[128];
			wcscpy_c(szTitle, gpConEmu->GetDefaultTitle());
			wcscat_c(szTitle, L" fast configuration");
			SetWindowText(hDlg, szTitle);

			CheckDlgButton(hDlg, cbUseKeyboardHooksFast, gpSet->isKeyboardHooks());

			CheckDlgButton(hDlg, cbInjectConEmuHkFast, gpSet->isUseInjects);

			if (gpSet->UpdSet.isUpdateUseBuilds != 0)
				CheckDlgButton(hDlg, cbEnableAutoUpdateFast, gpSet->UpdSet.isUpdateCheckOnStartup|gpSet->UpdSet.isUpdateCheckHourly);
			CheckRadioButton(hDlg, rbAutoUpdateStableFast, rbAutoUpdateDeveloperFast, (gpSet->UpdSet.isUpdateUseBuilds == 1) ? rbAutoUpdateStableFast : rbAutoUpdateDeveloperFast);

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
					/* Install Keyboard hooks */
					gpSet->m_isKeyboardHooks = IsDlgButtonChecked(hDlg, cbUseKeyboardHooksFast) ? 1 : 2;

					/* Inject ConEmuHk.dll */
					gpSet->isUseInjects = (IsDlgButtonChecked(hDlg, cbInjectConEmuHkFast) == BST_CHECKED);

					/* Auto Update settings */
					gpSet->UpdSet.isUpdateCheckOnStartup = (IsDlgButtonChecked(hDlg, cbEnableAutoUpdateFast) == BST_CHECKED);
					if (bCheckUpdate)
					{	// При первом запуске - умолчания параметров
						gpSet->UpdSet.isUpdateCheckHourly = false;
						gpSet->UpdSet.isUpdateConfirmDownload = true;
					}
					gpSet->UpdSet.isUpdateUseBuilds = IsDlgButtonChecked(hDlg, rbAutoUpdateStableFast) ? 1 : 2;


					/* Save settings */
					SettingsBase* reg = NULL;
					
					if ((reg = gpSet->CreateSettings()) == NULL)
					{
						_ASSERTE(reg!=NULL);
					}
					else
					{
						if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
						{
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

						if ((reg = gpSet->CreateSettings()) != NULL)
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
				gpConEmu->OnInfo_HomePage();
				return 1;
			}
		}

		break;
	}

	return 0;
}


void CheckOptionsFast()
{
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
		
		SettingsBase* reg = gpSet->CreateSettings();
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
		DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_FAST_CONFIG), NULL, CheckOptionsFastProc);
	}
}
