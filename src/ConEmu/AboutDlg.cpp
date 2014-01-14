
/*
Copyright (c) 2014 Maximus5
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

#define SHOWDEBUGSTR

#include "Header.h"
#include "About.h"
#include "AboutDlg.h"
#include "ConEmu.h"
#include "RealConsole.h"
#include "VirtualConsole.h"
#include "VConChild.h"
#include "version.h"

namespace ConEmuAbout
{
	void InitCommCtrls();
	bool mb_CommCtrlsInitialized = false;
	HWND mh_AboutDlg = NULL;
	DWORD nLastCrashReported = 0;

	INT_PTR WINAPI aboutProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
};

INT_PTR WINAPI ConEmuAbout::aboutProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	static struct {LPCWSTR Title; LPCWSTR Text;} Pages[] =
	{
		{L"About", pAbout},
		{L"Command line", pCmdLine},
		{L"Macro", pGuiMacro},
		{L"Console", pConsoleHelpFull},
		{L"-new_console", pNewConsoleHelpFull},
		{L"DosBox", pDosBoxHelpFull},
		{L"Contributors", pAboutContributors},
		{L"License", pAboutLicense},
	};

	switch (messg)
	{
		case WM_INITDIALOG:
		{
			gpConEmu->OnOurDialogOpened();
			mh_AboutDlg = hWnd2;

			if ((ghOpWnd && IsWindow(ghOpWnd)) || (WS_EX_TOPMOST & GetWindowLongPtr(ghWnd, GWL_EXSTYLE)))
			{
				SetWindowPos(hWnd2, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
			}

			LPCWSTR pszActivePage = (LPCWSTR)lParam;

			WCHAR szTitle[255];
			LPCWSTR pszBits = WIN3264TEST(L"x86",L"x64");
			LPCWSTR pszDebug = L"";
			#ifdef _DEBUG
			pszDebug = L"[DEBUG] ";
			#endif
			_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"About ConEmu (%02u%02u%02u%s %s%s)", 
				(MVV_1%100),MVV_2,MVV_3,_T(MVV_4a), pszDebug, pszBits);
			SetWindowText(hWnd2, szTitle);

			if (hClassIcon)
			{
				SendMessage(hWnd2, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
				SendMessage(hWnd2, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);
				SetClassLongPtr(hWnd2, GCLP_HICON, (LONG_PTR)hClassIcon);
			}

			SetDlgItemText(hWnd2, stConEmuAbout, pAboutTitle);
			SetDlgItemText(hWnd2, stConEmuUrl, gsHomePage);

			HWND hTab = GetDlgItem(hWnd2, tbAboutTabs);
			size_t nPage = 0;

			for (size_t i = 0; i < countof(Pages); i++)
			{
				TCITEM tie = {};
				tie.mask = TCIF_TEXT;
				tie.pszText = (LPWSTR)Pages[i].Title;
				TabCtrl_InsertItem(hTab, i, &tie);

				if (pszActivePage && (lstrcmpi(pszActivePage, Pages[i].Title) == 0))
					nPage = i;
			}

			SetDlgItemText(hWnd2, tAboutText, Pages[nPage].Text);

			if (nPage != 0)
			{
				TabCtrl_SetCurSel(hTab, (int)nPage);
			}
			else
			{
				_ASSERTE(pszActivePage==NULL && "Unknown page name?");
			}

			SetFocus(hTab);

			return FALSE;
		}

		case WM_CTLCOLORSTATIC:
			if (GetDlgItem(hWnd2, stConEmuUrl) == (HWND)lParam)
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
				if (((HWND)wParam) == GetDlgItem(hWnd2, stConEmuUrl))
				{
					SetCursor(LoadCursor(NULL, IDC_HAND));
					SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, TRUE);
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
					case IDCANCEL:
					case IDCLOSE:
						aboutProc(hWnd2, WM_CLOSE, 0, 0);
						return 1;
					case stConEmuUrl:
						ConEmuAbout::OnInfo_HomePage();
						return 1;
				}
			}
			break;

		case WM_NOTIFY:
		{
			LPNMHDR nmhdr = (LPNMHDR)lParam;
			if ((nmhdr->code == TCN_SELCHANGE) && (nmhdr->idFrom == tbAboutTabs))
			{
				int iPage = TabCtrl_GetCurSel(nmhdr->hwndFrom);
				if ((iPage >= 0) && (iPage < (int)countof(Pages)))
					SetDlgItemText(hWnd2, tAboutText, Pages[iPage].Text);
			}
			break;
		}

		case WM_CLOSE:
			//if (ghWnd == NULL)
			gpConEmu->OnOurDialogClosed();
			EndDialog(hWnd2, IDOK);
			//else
			//	DestroyWindow(hWnd2);
			break;

		case WM_DESTROY:
			mh_AboutDlg = NULL;
			break;

		default:
			return FALSE;
	}

	return FALSE;
}

void ConEmuAbout::InitCommCtrls()
{
	if (mb_CommCtrlsInitialized)
		return;

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC   = ICC_COOL_CLASSES|ICC_BAR_CLASSES|ICC_TAB_CLASSES|ICC_PROGRESS_CLASS; //|ICC_STANDARD_CLASSES|ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icex);

	mb_CommCtrlsInitialized = true;
}

void ConEmuAbout::OnInfo_Donate()
{
	int nBtn = MessageBox(
		L"You can show your appreciation and support future development by donating.\r\n\r\n"
		L"Donate (PayPal) button located on project website\r\n"
		L"under ConEmu sketch (upper right of page).\r\n\r\n"
		L"Open project website?",
		MB_YESNO|MB_ICONINFORMATION);

	if (nBtn == IDYES)
		OnInfo_HomePage();
}

void ConEmuAbout::OnInfo_About(LPCWSTR asPageName /*= NULL*/)
{
	InitCommCtrls();

	bool bOk = false;

	//if (ghWnd)
	//{
	//	HWND hAbout = NULL;
	//	if (mh_AboutDlg && IsWindow(mh_AboutDlg))
	//	{
	//		hAbout = mh_AboutDlg;
	//		apiShowWindow(hAbout, SW_SHOWNORMAL);
	//		SetFocus(hAbout);
	//	}
	//	else
	//	{
	//		hAbout = CreateDialog(g_hInstance, MAKEINTRESOURCE(IDD_ABOUT), NULL, aboutProc);
	//	}
	//	bOk = (hAbout != NULL);
	//}
	//else
	{
		DontEnable de;
		HWND hParent = (ghOpWnd && IsWindowVisible(ghOpWnd)) ? ghOpWnd : ghWnd;
		// Модальный?
		INT_PTR iRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_ABOUT), hParent, aboutProc, (LPARAM)asPageName);
		bOk = (iRc != 0 && iRc != -1);

		#ifdef _DEBUG
		if (!bOk) DisplayLastError(L"DialogBoxParam(IDD_ABOUT) failed");
		#endif
	}

	if (!bOk)
	{
		WCHAR szTitle[255];
		LPCWSTR pszBits = WIN3264TEST(L"x86",L"x64");
		LPCWSTR pszDebug = L"";
		#ifdef _DEBUG
		pszDebug = L"[DEBUG] ";
		#endif
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"About ConEmu (%02u%02u%02u%s %s%s)", 
			(MVV_1%100),MVV_2,MVV_3,_T(MVV_4a), pszDebug, pszBits);
		DontEnable de;
		MSGBOXPARAMS mb = {sizeof(MSGBOXPARAMS), ghWnd, g_hInstance,
			pAbout,
			szTitle,
			MB_USERICON, MAKEINTRESOURCE(IMAGE_ICON), 0, NULL, LANG_NEUTRAL
		};
		MessageBoxIndirectW(&mb);
		//MessageBoxW(ghWnd, pHelp, szTitle, MB_ICONQUESTION);
	}
}

void ConEmuAbout::OnInfo_WhatsNew(bool bLocal)
{
	wchar_t sFile[MAX_PATH+80];
	int iExec = -1;

	if (bLocal)
	{
		wcscpy_c(sFile, gpConEmu->ms_ConEmuBaseDir);
		wcscat_c(sFile, L"\\WhatsNew-ConEmu.txt");

		if (FileExists(sFile))
		{
			iExec = (int)ShellExecute(ghWnd, L"open", sFile, NULL, NULL, SW_SHOWNORMAL);
			if (iExec >= 32)
			{
				return;
			}
		}
	}

	wcscpy_c(sFile, gsWhatsNew);
	
	iExec = (int)ShellExecute(ghWnd, L"open", sFile, NULL, NULL, SW_SHOWNORMAL);
	if (iExec >= 32)
	{
		return;
	}

	DisplayLastError(L"File 'WhatsNew-ConEmu.txt' not found, go to web page failed", iExec);
}

void ConEmuAbout::OnInfo_Help()
{
	static HMODULE hhctrl = NULL;

	if (!hhctrl) hhctrl = GetModuleHandle(L"hhctrl.ocx");

	if (!hhctrl) hhctrl = LoadLibrary(L"hhctrl.ocx");

	if (hhctrl)
	{
		typedef BOOL (WINAPI* HTMLHelpW_t)(HWND hWnd, LPCWSTR pszFile, INT uCommand, INT dwData);
		HTMLHelpW_t fHTMLHelpW = (HTMLHelpW_t)GetProcAddress(hhctrl, "HtmlHelpW");

		if (fHTMLHelpW)
		{
			wchar_t szHelpFile[MAX_PATH*2];
			lstrcpy(szHelpFile, gpConEmu->ms_ConEmuChm);
			//wchar_t* pszSlash = wcsrchr(szHelpFile, L'\\');
			//if (pszSlash) pszSlash++; else pszSlash = szHelpFile;
			//lstrcpy(pszSlash, L"ConEmu.chm");
			// lstrcat(szHelpFile, L::/Intro.htm");
			#define HH_HELP_CONTEXT 0x000F
			#define HH_DISPLAY_TOC  0x0001
			//fHTMLHelpW(NULL /*чтобы окно не блокировалось*/, szHelpFile, HH_HELP_CONTEXT, contextID);
			fHTMLHelpW(NULL /*чтобы окно не блокировалось*/, szHelpFile, HH_DISPLAY_TOC, 0);
		}
	}
}

void ConEmuAbout::OnInfo_HomePage()
{
	DWORD shellRc = (DWORD)(INT_PTR)ShellExecute(ghWnd, L"open", gsHomePage, NULL, NULL, SW_SHOWNORMAL);
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}

void ConEmuAbout::OnInfo_ReportBug()
{
	DWORD shellRc = (DWORD)(INT_PTR)ShellExecute(ghWnd, L"open", gsReportBug, NULL, NULL, SW_SHOWNORMAL);
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}

void ConEmuAbout::OnInfo_ReportCrash(LPCWSTR asDumpWasCreatedMsg)
{
	if (nLastCrashReported)
	{
		// if previous gsReportCrash was opened less than 60 sec ago
		DWORD nLast = GetTickCount() - nLastCrashReported;
		if (nLast < 60000)
		{
			// Skip this time
			return;
		}
	}

	if (asDumpWasCreatedMsg && !*asDumpWasCreatedMsg)
	{
		asDumpWasCreatedMsg = NULL;
	}

	DWORD shellRc = (DWORD)(INT_PTR)ShellExecute(ghWnd, L"open", gsReportCrash, NULL, NULL, SW_SHOWNORMAL);
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
	else if (asDumpWasCreatedMsg)
	{
		MessageBox(asDumpWasCreatedMsg, MB_OK|MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
	}

	nLastCrashReported = GetTickCount();
}

void ConEmuAbout::OnInfo_ThrowTrapException(bool bMainThread)
{
	if (bMainThread)
	{
		if (MessageBox(L"Are you sure?\nApplication will terminates after that!\nThrow exception in ConEmu's main thread?", MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2)==IDYES)
		{
			//#ifdef _DEBUG
			//MyAssertTrap();
			//#else
			//DebugBreak();
			//#endif
			// -- trigger division by 0
			RaiseTestException();
		}
	}
	else
	{
		if (MessageBox(L"Are you sure?\nApplication will terminates after that!\nThrow exception in ConEmu's monitor thread?", MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2)==IDYES)
		{
			CVConGuard VCon;
			if ((gpConEmu->GetActiveVCon(&VCon) >= 0) && VCon->RCon())
				VCon->RCon()->MonitorAssertTrap();
		}
	}
}
