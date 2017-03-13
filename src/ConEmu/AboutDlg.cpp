﻿
// WM_DRAWITEM, SS_OWNERDRAW|SS_NOTIFY, Bmp & hBmp -> global vars
// DPI resize.

/*
Copyright (c) 2014-2017 Maximus5
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

#undef HIDE_USE_EXCEPTION_INFO

#define SHOWDEBUGSTR

#include "Header.h"
#include "About.h"
#include "AboutDlg.h"
#include "ConEmu.h"
#include "DpiAware.h"
#include "DynDialog.h"
#include "ImgButton.h"
#include "LngRc.h"
#include "OptionsClass.h"
#include "PushInfo.h"
#include "RealConsole.h"
#include "SearchCtrl.h"
#include "Update.h"
#include "VirtualConsole.h"
#include "VConChild.h"
#include "version.h"
#include "../common/MSetter.h"
#include "../common/WObjects.h"
#include "../common/StartupEnvEx.h"

namespace ConEmuAbout
{
	void InitCommCtrls();
	bool mb_CommCtrlsInitialized = false;
	HWND mh_AboutDlg = NULL;
	DWORD nLastCrashReported = 0;
	CDpiForDialog* mp_DpiAware = NULL;

	INT_PTR WINAPI aboutProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
	void searchProc(HWND hDlg, HWND hSearch, bool bReentr);

	void OnInfo_DonateLink();
	void OnInfo_FlattrLink();

	CImgButtons* mp_ImgBtn = NULL;

	static struct {LPCWSTR Title; LPCWSTR Text;} Pages[] =
	{
		{L"About", pAbout},
		{L"ConEmu /?", pCmdLine},
		{L"Tasks", pAboutTasks},
		{L"-new_console", pNewConsoleHelpFull},
		{L"ConEmuC /?", pConsoleHelpFull},
		{L"Macro", pGuiMacro},
		{L"DosBox", pDosBoxHelpFull},
		{L"Contributors", pAboutContributors},
		{L"License", pAboutLicense},
		{L"SysInfo", L""},
	};

	wchar_t sLastOpenTab[32] = L"";

	void TabSelected(HWND hDlg, int idx);

	wchar_t* gsSysInfo = NULL;
	void ReloadSysInfo();
	void LogStartEnvInt(LPCWSTR asText, LPARAM lParam, bool bFirst, bool bNewLine);

	DWORD nTextSelStart = 0, nTextSelEnd = 0;
};

INT_PTR WINAPI ConEmuAbout::aboutProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR lRc = 0;
	if ((mp_ImgBtn && mp_ImgBtn->Process(hDlg, messg, wParam, lParam, lRc))
		|| EditIconHint_Process(hDlg, messg, wParam, lParam, lRc))
	{
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lRc);
		return TRUE;
	}

	PatchMsgBoxIcon(hDlg, messg, wParam, lParam);

	switch (messg)
	{
		case WM_INITDIALOG:
		{
			gpConEmu->OnOurDialogOpened();
			mh_AboutDlg = hDlg;

			CDynDialog::LocalizeDialog(hDlg);

			_ASSERTE(mp_ImgBtn==NULL);
			SafeDelete(mp_ImgBtn);
			mp_ImgBtn = new CImgButtons(hDlg, pIconCtrl, IDOK);
			mp_ImgBtn->AddDonateButtons();

			if (mp_DpiAware)
			{
				mp_DpiAware->Attach(hDlg, ghWnd, CDynDialog::GetDlgClass(hDlg));
			}

			CDpiAware::CenterDialog(hDlg);

			if ((ghOpWnd && IsWindow(ghOpWnd)) || (WS_EX_TOPMOST & GetWindowLongPtr(ghWnd, GWL_EXSTYLE)))
			{
				SetWindowPos(hDlg, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
			}

			LPCWSTR pszActivePage = (LPCWSTR)lParam;

			LPCWSTR psStage = (ConEmuVersionStage == CEVS_STABLE) ? L"{Stable}"
							: (ConEmuVersionStage == CEVS_PREVIEW) ? L"{Preview}"
							: L"{Alpha}";
			CEStr lsTitle(
				CLngRc::getRsrc(lng_DlgAbout/*"About"*/),
				L" ",
				gpConEmu->GetDefaultTitle(),
				L" ",
				psStage,
				NULL);
			if (lsTitle)
			{
				SetWindowText(hDlg, lsTitle);
			}

			if (hClassIcon)
			{
				SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
				SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)CreateNullIcon());
				SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)hClassIcon);
			}

			// "Console Emulation program (local terminal)"
			SetDlgItemText(hDlg, stConEmuAbout, CLngRc::getRsrc(lng_AboutAppName));

			SetDlgItemText(hDlg, stConEmuUrl, gsHomePage);

			EditIconHint_Set(hDlg, GetDlgItem(hDlg, tAboutSearch), true,
				CLngRc::getRsrc(lng_Search/*"Search"*/),
				false, UM_SEARCH, IDOK);
			EditIconHint_Subclass(hDlg);

			wchar_t* pszLabel = GetDlgItemTextPtr(hDlg, stConEmuVersion);
			if (pszLabel)
			{
				wchar_t* pszSet = NULL;

				if (gpUpd)
				{
					wchar_t* pszVerInfo = gpUpd->GetCurVerInfo();
					if (pszVerInfo)
					{
						pszSet = lstrmerge(pszLabel, L" ", pszVerInfo);
						free(pszVerInfo);
					}
				}

				if (!pszSet)
				{
					pszSet = lstrmerge(pszLabel, L" ", CLngRc::getRsrc(lng_PleaseCheckManually/*"Please check for updates manually"*/));
				}

				if (pszSet)
				{
					SetDlgItemText(hDlg, stConEmuVersion, pszSet);
					free(pszSet);
				}

				free(pszLabel);
			}

			HWND hTab = GetDlgItem(hDlg, tbAboutTabs);
			INT_PTR nPage = -1;

			for (size_t i = 0; i < countof(Pages); i++)
			{
				TCITEM tie = {};
				tie.mask = TCIF_TEXT;
				tie.pszText = (LPWSTR)Pages[i].Title;
				TabCtrl_InsertItem(hTab, i, &tie);

				if (pszActivePage && (lstrcmpi(pszActivePage, Pages[i].Title) == 0))
					nPage = i;
			}


			if (nPage >= 0)
			{
				TabSelected(hDlg, nPage);
				TabCtrl_SetCurSel(hTab, (int)nPage);
			}
			else if (!pszActivePage)
			{
				TabSelected(hDlg, 0);
			}
			else
			{
				_ASSERTE(pszActivePage==NULL && "Unknown page name?");
			}

			SetFocus(hTab);

			return FALSE;
		}

		case WM_CTLCOLORSTATIC:
			if (GetWindowLongPtr((HWND)lParam, GWLP_ID) == stConEmuUrl)
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
				if (GetWindowLongPtr((HWND)wParam, GWLP_ID) == stConEmuUrl)
				{
					SetCursor(LoadCursor(NULL, IDC_HAND));
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
					return TRUE;
				}
				return FALSE;
			}
			break;

		case WM_COMMAND:
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
				switch (LOWORD(wParam))
				{
					case IDOK:
					case IDCANCEL:
					case IDCLOSE:
						aboutProc(hDlg, WM_CLOSE, 0, 0);
						return 1;
					case stConEmuUrl:
						ConEmuAbout::OnInfo_HomePage();
						return 1;
				} // BN_CLICKED
				break;
			case EN_SETFOCUS:
				switch (LOWORD(wParam))
				{
				case tAboutText:
					{
						// Do not autosel all text
						HWND hEdit = (HWND)lParam;
						DWORD nStart = 0, nEnd = 0;
						SendMessage(hEdit, EM_GETSEL, (WPARAM)&nStart, (LPARAM)&nEnd);
						if (nStart != nEnd)
						{
							SendMessage(hEdit, EM_SETSEL, nTextSelStart, nTextSelEnd);
						}
					}
					break;
				}
			} // switch (HIWORD(wParam))
			break;

		case WM_NOTIFY:
		{
			LPNMHDR nmhdr = (LPNMHDR)lParam;
			if ((nmhdr->code == TCN_SELCHANGE) && (nmhdr->idFrom == tbAboutTabs))
			{
				int iPage = TabCtrl_GetCurSel(nmhdr->hwndFrom);
				if ((iPage >= 0) && (iPage < (int)countof(Pages)))
					TabSelected(hDlg, iPage);
			}
			break;
		}

		case UM_SEARCH:
			searchProc(hDlg, (HWND)lParam, false);
			break;

		case UM_EDIT_KILL_FOCUS:
			SendMessage((HWND)lParam, EM_GETSEL, (WPARAM)&nTextSelStart, (LPARAM)&nTextSelEnd);
			break;

		case WM_CLOSE:
			//if (ghWnd == NULL)
			gpConEmu->OnOurDialogClosed();
			if (mp_DpiAware)
				mp_DpiAware->Detach();
			EndDialog(hDlg, IDOK);
			SafeDelete(mp_ImgBtn);
			break;

		case WM_DESTROY:
			mh_AboutDlg = NULL;
			break;

		default:
			if (mp_DpiAware && mp_DpiAware->ProcessDpiMessages(hDlg, messg, wParam, lParam))
			{
				return TRUE;
			}
	}

	return FALSE;
}

void ConEmuAbout::searchProc(HWND hDlg, HWND hSearch, bool bReentr)
{
	HWND hEdit = GetDlgItem(hDlg, tAboutText);
	wchar_t* pszPart = GetDlgItemTextPtr(hSearch, 0);
	wchar_t* pszText = GetDlgItemTextPtr(hEdit, 0);
	bool bRetry = false;

	if (pszPart && *pszPart && pszText && *pszText)
	{
		LPCWSTR pszFrom = pszText;

		DWORD nStart = 0, nEnd = 0;
		SendMessage(hEdit, EM_GETSEL, (WPARAM)&nStart, (LPARAM)&nEnd);

		size_t cchMax = wcslen(pszText);
		size_t cchFrom = max(nStart,nEnd);
		if (cchMax > cchFrom)
			pszFrom += cchFrom;

		LPCWSTR pszFind = StrStrI(pszFrom, pszPart);
		if (!pszFind && bReentr && (pszFrom != pszText))
			pszFind = StrStrI(pszText, pszPart);

		if (pszFind)
		{
			const wchar_t szBrkChars[] = L"()[]<>{}:;,.-=\\/ \t\r\n";
			LPCWSTR pszEnd = wcspbrk(pszFind, szBrkChars);
			INT_PTR nPartLen = wcslen(pszPart);
			if (!pszEnd || ((pszEnd - pszFind) > max(nPartLen,60)))
				pszEnd = pszFind + nPartLen;
			while ((pszFind > pszFrom) && !wcschr(szBrkChars, *(pszFind-1)))
				pszFind--;
			//SetFocus(hEdit);
			nTextSelStart = (DWORD)(pszEnd-pszText);
			nTextSelEnd = (DWORD)(pszFind-pszText);
			SendMessage(hEdit, EM_SETSEL, nTextSelStart, nTextSelEnd);
			SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
		}
		else if (!bReentr)
		{
			HWND hTab = GetDlgItem(hDlg, tbAboutTabs);
			int iPage = TabCtrl_GetCurSel(hTab);
			int iFound = -1;
			for (int s = 0; (iFound == -1) && (s <= 1); s++)
			{
				int iFrom = (s == 0) ? (iPage+1) : 0;
				int iTo = (s == 0) ? (int)countof(Pages) : (iPage-1);
				for (int i = iFrom; i < iTo; i++)
				{
					if (StrStrI(Pages[i].Title, pszPart)
						|| StrStrI(Pages[i].Text, pszPart))
					{
						iFound = i; break;
					}
				}
			}
			if (iFound >= 0)
			{
				TabSelected(hDlg, iFound);
				TabCtrl_SetCurSel(hTab, iFound);
				//SetFocus(hEdit);
				bRetry = true;
			}
		}
	}

	SafeFree(pszPart);
	SafeFree(pszText);

	if (bRetry)
	{
		searchProc(hDlg, hSearch, true);
	}
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

void ConEmuAbout::OnInfo_OnlineWiki(LPCWSTR asPageName /*= NULL*/)
{
	CEStr szUrl(CEWIKIBASE, asPageName ? asPageName : L"TableOfContents", L".html");
	DWORD shellRc = (DWORD)(INT_PTR)ShellExecute(ghWnd, L"open", szUrl, NULL, NULL, SW_SHOWNORMAL);
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}

void ConEmuAbout::OnInfo_Donate()
{
	int nBtn = MsgBox(
		L"You can show your appreciation and support future development by donating.\n\n"
		L"Open ConEmu's donate web page?"
		,MB_YESNO|MB_ICONINFORMATION);

	if (nBtn == IDYES)
	{
		OnInfo_DonateLink();
		//OnInfo_HomePage();
	}
}

void ConEmuAbout::OnInfo_DonateLink()
{
	DWORD shellRc = (DWORD)(INT_PTR)ShellExecute(ghWnd, L"open", gsDonatePage, NULL, NULL, SW_SHOWNORMAL);
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}
void ConEmuAbout::OnInfo_FlattrLink()
{
	DWORD shellRc = (DWORD)(INT_PTR)ShellExecute(ghWnd, L"open", gsFlattrPage, NULL, NULL, SW_SHOWNORMAL);
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}

void ConEmuAbout::TabSelected(HWND hDlg, int idx)
{
	if (idx < 0 || idx >= countof(Pages))
		return;

	wcscpy_c(sLastOpenTab, Pages[idx].Title);
	LPCWSTR pszNewText = Pages[idx].Text;
	CEStr lsTemp;
	if (gpConEmu->mp_PushInfo && gpConEmu->mp_PushInfo->mp_Active && gpConEmu->mp_PushInfo->mp_Active->pszFullMessage)
	{
		// EDIT control requires \r\n as line endings
		lsTemp = lstrmerge(gpConEmu->mp_PushInfo->mp_Active->pszFullMessage, L"\r\n\r\n\r\n", pszNewText);
		pszNewText = lsTemp.ms_Val;
	}
	SetDlgItemText(hDlg, tAboutText, pszNewText);
}

void ConEmuAbout::LogStartEnvInt(LPCWSTR asText, LPARAM lParam, bool bFirst, bool bNewLine)
{
	lstrmerge(&gsSysInfo, asText, bNewLine ? L"\r\n" : NULL);

	if (bFirst && gpConEmu)
	{
		lstrmerge(&gsSysInfo, L"  AppID: ", gpConEmu->ms_AppID, L"\r\n");
	}
}

void ConEmuAbout::ReloadSysInfo()
{
	if (!gpStartEnv)
		return;

	_ASSERTE(lstrcmp(Pages[countof(Pages)-1].Title, L"SysInfo") == 0);
	SafeFree(gsSysInfo);

	LoadStartupEnvEx::ToString(gpStartEnv, LogStartEnvInt, 0);

	Pages[countof(Pages)-1].Text = gsSysInfo;
}

void ConEmuAbout::OnInfo_About(LPCWSTR asPageName /*= NULL*/)
{
	InitCommCtrls();

	bool bOk = false;

	if (!asPageName && sLastOpenTab[0])
	{
		// Reopen last active tab
		asPageName = sLastOpenTab;
	}

	ReloadSysInfo();

	{
		DontEnable de;
		if (!mp_DpiAware)
			mp_DpiAware = new CDpiForDialog();
		HWND hParent = (ghOpWnd && IsWindowVisible(ghOpWnd)) ? ghOpWnd : ghWnd;
		// Modal dialog (CreateDialog)
		INT_PTR iRc = CDynDialog::ExecuteDialog(IDD_ABOUT, hParent, aboutProc, (LPARAM)asPageName);
		bOk = (iRc != 0 && iRc != -1);

		mh_AboutDlg = NULL;
		if (mp_DpiAware)
			mp_DpiAware->Detach();

		#ifdef _DEBUG
		// Any problems with dialog resource?
		if (!bOk) DisplayLastError(L"DialogBoxParam(IDD_ABOUT) failed");
		#endif
	}

	if (!bOk)
	{
		CEStr szTitle(gpConEmu->GetDefaultTitle(), L" ", CLngRc::getRsrc(lng_DlgAbout/*"About"*/));
		DontEnable de;
		MSGBOXPARAMS mb = {sizeof(MSGBOXPARAMS), ghWnd, g_hInstance,
			pAbout,
			szTitle.ms_Val,
			MB_USERICON, MAKEINTRESOURCE(IMAGE_ICON), 0, NULL, LANG_NEUTRAL
		};
		MSetter lInCall(&gnInMsgBox);
		// Use MessageBoxIndirect instead of MessageBox to show our icon instead of std ICONINFORMATION
		MessageBoxIndirect(&mb);
	}
}

void ConEmuAbout::OnInfo_WhatsNew(bool bLocal)
{
	wchar_t sFile[MAX_PATH+80];
	INT_PTR iExec = -1;

	if (bLocal)
	{
		wcscpy_c(sFile, gpConEmu->ms_ConEmuBaseDir);
		wcscat_c(sFile, L"\\WhatsNew-ConEmu.txt");

		if (FileExists(sFile))
		{
			iExec = (INT_PTR)ShellExecute(ghWnd, L"open", sFile, NULL, NULL, SW_SHOWNORMAL);
			if (iExec >= 32)
			{
				return;
			}
		}
	}

	wcscpy_c(sFile, gsWhatsNew);

	iExec = (INT_PTR)ShellExecute(ghWnd, L"open", sFile, NULL, NULL, SW_SHOWNORMAL);
	if (iExec >= 32)
	{
		return;
	}

	DisplayLastError(L"File 'WhatsNew-ConEmu.txt' not found, go to web page failed", (int)LODWORD(iExec));
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

void ConEmuAbout::OnInfo_DownloadPage()
{
	DWORD shellRc = (DWORD)(INT_PTR)ShellExecute(ghWnd, L"open", gsDownlPage, NULL, NULL, SW_SHOWNORMAL);
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}

void ConEmuAbout::OnInfo_FirstStartPage()
{
	DWORD shellRc = (DWORD)(INT_PTR)ShellExecute(ghWnd, L"open", gsFirstStart, NULL, NULL, SW_SHOWNORMAL);
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
		MsgBox(asDumpWasCreatedMsg, MB_OK|MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
	}

	nLastCrashReported = GetTickCount();
}

void ConEmuAbout::OnInfo_ThrowTrapException(bool bMainThread)
{
	if (bMainThread)
	{
		if (MsgBox(L"Are you sure?\nApplication will terminate after that!\nThrow exception in ConEmu's main thread?", MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2)==IDYES)
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
		if (MsgBox(L"Are you sure?\nApplication will terminate after that!\nThrow exception in ConEmu's monitor thread?", MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2)==IDYES)
		{
			CVConGuard VCon;
			if ((gpConEmu->GetActiveVCon(&VCon) >= 0) && VCon->RCon())
				VCon->RCon()->MonitorAssertTrap();
		}
	}
}
