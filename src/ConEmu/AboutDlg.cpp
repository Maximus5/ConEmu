
// WM_DRAWITEM, SS_OWNERDRAW|SS_NOTIFY, Bmp & hBmp -> global vars
// DPI resize.

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

#undef HIDE_USE_EXCEPTION_INFO

#define SHOWDEBUGSTR

#include "Header.h"
#include "About.h"
#include "AboutDlg.h"
#include "ConEmu.h"
#include "DpiAware.h"
#include "DynDialog.h"
#include "OptionsClass.h"
#include "PushInfo.h"
#include "RealConsole.h"
#include "SearchCtrl.h"
#include "ToolImg.h"
#include "Update.h"
#include "VirtualConsole.h"
#include "VConChild.h"
#include "version.h"
#include "../common/WObjects.h"
#include "../common/StartupEnvEx.h"

namespace ConEmuAbout
{
	void InitCommCtrls();
	bool mb_CommCtrlsInitialized = false;
	HWND mh_AboutDlg = NULL;
	DWORD nLastCrashReported = 0;
	CDpiForDialog* mp_DpiAware;

	INT_PTR WINAPI aboutProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
	void searchProc(HWND hDlg, HWND hSearch, bool bReentr);

	void OnInfo_DonateLink();
	void OnInfo_FlattrLink();

	struct BtnInfo
	{
		LPCWSTR ResId;
		CToolImg* pImg;
		LPWSTR pszUrl;
		UINT nCtrlId;
	} m_Btns[2] = {};

	void LoadResources();

	wchar_t gsDonatePage[] = CEDONATEPAGE;
	wchar_t gsFlattrPage[] = CEFLATTRPAGE;

	HWND hwndTip = NULL;
	void RegisterTip(HWND hDlg);

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
	if (DonateBtns_Process(hDlg, messg, wParam, lParam, lRc)
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

			DonateBtns_Add(hDlg, pIconCtrl, IDOK);

			if (mp_DpiAware)
			{
				mp_DpiAware->Attach(hDlg, ghWnd, CDynDialog::GetDlgClass(hDlg));
			}

			RECT rect = {};
			if (GetWindowRect(hDlg, &rect))
			{
				CDpiAware::GetCenteredRect(ghWnd, rect);
				MoveWindowRect(hDlg, rect);
			}

			if ((ghOpWnd && IsWindow(ghOpWnd)) || (WS_EX_TOPMOST & GetWindowLongPtr(ghWnd, GWL_EXSTYLE)))
			{
				SetWindowPos(hDlg, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
			}

			LPCWSTR pszActivePage = (LPCWSTR)lParam;

			wchar_t* pszTitle = lstrmerge(gpConEmu->GetDefaultTitle(), L" About");
			if (pszTitle)
			{
				SetWindowText(hDlg, pszTitle);
				SafeFree(pszTitle);
			}

			if (hClassIcon)
			{
				SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
				SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)CreateNullIcon());
				SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)hClassIcon);
			}

			SetDlgItemText(hDlg, stConEmuAbout, pAboutTitle);
			SetDlgItemText(hDlg, stConEmuUrl, gsHomePage);

			EditIconHint_Set(hDlg, GetDlgItem(hDlg, tAboutSearch), true, L"Search", false, UM_SEARCH, IDOK);
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
					pszSet = lstrmerge(pszLabel, L" ", L"Please check for updates manually");
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

			for (size_t i = 0; i < countof(m_Btns); i++)
			{
				SafeDelete(m_Btns[i].pImg);
			}
			//else
			//	DestroyWindow(hDlg);
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
	CEStr szUrl(lstrmerge(CEWIKIBASE, asPageName ? asPageName : L"TableOfContents", L".html"));
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
		pszNewText = lsTemp.ms_Arg;
	}
	SetDlgItemText(hDlg, tAboutText, pszNewText);
}

void ConEmuAbout::LogStartEnvInt(LPCWSTR asText, LPARAM lParam, bool bFirst, bool bNewLine)
{
	lstrmerge(&gsSysInfo, asText, bNewLine ? L"\r\n" : NULL);
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
		CEStr szTitle = lstrmerge(gpConEmu->GetDefaultTitle(), L" About");
		DontEnable de;
		MSGBOXPARAMS mb = {sizeof(MSGBOXPARAMS), ghWnd, g_hInstance,
			pAbout,
			szTitle.ms_Arg,
			MB_USERICON, MAKEINTRESOURCE(IMAGE_ICON), 0, NULL, LANG_NEUTRAL
		};
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
		if (MsgBox(L"Are you sure?\nApplication will terminates after that!\nThrow exception in ConEmu's main thread?", MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2)==IDYES)
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
		if (MsgBox(L"Are you sure?\nApplication will terminates after that!\nThrow exception in ConEmu's monitor thread?", MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2)==IDYES)
		{
			CVConGuard VCon;
			if ((gpConEmu->GetActiveVCon(&VCon) >= 0) && VCon->RCon())
				VCon->RCon()->MonitorAssertTrap();
		}
	}
}

void ConEmuAbout::LoadResources()
{
	for (size_t i = 0; i < countof(m_Btns); i++)
	{
		if (m_Btns[i].pImg)
			continue; // Already loaded

		switch (i)
		{
		case 0:
			m_Btns[i].ResId = L"DONATE";
			SafeDelete(m_Btns[i].pImg);
			m_Btns[i].pImg = new CToolImg();
			if (m_Btns[i].pImg && !m_Btns[i].pImg->CreateDonateButton())
				SafeDelete(m_Btns[i].pImg);
			m_Btns[i].nCtrlId = pLinkDonate;
			m_Btns[i].pszUrl = gsDonatePage;
			break;
		case 1:
			m_Btns[i].ResId = L"FLATTR";
			SafeDelete(m_Btns[i].pImg);
			m_Btns[i].pImg = new CToolImg();
			if (m_Btns[i].pImg && !m_Btns[i].pImg->CreateFlattrButton())
				SafeDelete(m_Btns[i].pImg);
			m_Btns[i].nCtrlId = pLinkFlattr;
			m_Btns[i].pszUrl = gsFlattrPage;
			break;
		}

		#ifdef _DEBUG
		if (m_Btns[i].pImg == NULL)
		{
			DWORD nErr = GetLastError();
			DisplayLastError(L"Failed to load button image!", nErr);
		}
		#endif
	}
}

void ConEmuAbout::DonateBtns_Add(HWND hDlg, int AlignLeftId, int AlignVCenterId)
{
	if (!m_Btns[0].pImg)
		LoadResources();

	RECT rcLeft = {}, rcTop = {};
	HWND hCtrl;

	hCtrl = GetDlgItem(hDlg, AlignLeftId);
	GetWindowRect(hCtrl, &rcLeft);
	MapWindowPoints(NULL, hDlg, (LPPOINT)&rcLeft, 2);
	hCtrl = GetDlgItem(hDlg, AlignVCenterId);
	GetWindowRect(hCtrl, &rcTop);
	int nPreferHeight = rcTop.bottom - rcTop.top;
	MapWindowPoints(NULL, hDlg, (LPPOINT)&rcTop, 2);

	#ifdef _DEBUG
	DpiValue dpi;
	CDpiAware::QueryDpiForWindow(hDlg, &dpi);
	#endif

	int X = rcLeft.left;

	for (size_t i = 0; i < countof(m_Btns); i++)
	{
		if (!m_Btns[i].pImg)
			continue; // Image was failed

		TODO("Вертикальное центрирование по объекту AlignVCenterId");

		int nDispW = 0, nDispH = 0;
		if (!m_Btns[i].pImg->GetSizeForHeight(nPreferHeight, nDispW, nDispH))
		{
			_ASSERTE(FALSE && "Image not available for dpi?");
			continue; // Image was failed?
		}
		_ASSERTE(nDispW>0 && nDispH>0);
		int nY = rcTop.top + ((rcTop.bottom - rcTop.top - nDispH + 1) / 2);

		hCtrl = CreateWindow(L"STATIC", m_Btns[i].ResId,
			WS_CHILD|WS_VISIBLE|SS_NOTIFY|SS_OWNERDRAW,
			X, nY, nDispW, nDispH,
			hDlg, (HMENU)(DWORD_PTR)m_Btns[i].nCtrlId, g_hInstance, NULL);

		#ifdef _DEBUG
		if (!hCtrl)
			DisplayLastError(L"Failed to create image button control");
		#endif

		//X += nDispW + (10 * dpi.Ydpi / 96);
		X += nDispW + (nDispH / 3);
	}

	RegisterTip(hDlg);

	UNREFERENCED_PARAMETER(hCtrl);
}

bool ConEmuAbout::DonateBtns_Process(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam, INT_PTR& lResult)
{
	switch (messg)
	{
	case WM_SETCURSOR:
		{
			LONG_PTR lID = GetWindowLongPtr((HWND)wParam, GWLP_ID);
			if (lID == pLinkDonate || lID == pLinkFlattr)
			{
				SetCursor(LoadCursor(NULL, IDC_HAND));
				lResult = TRUE;
				return true;
			}
			return false;
		}
		break;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
			case pLinkDonate:
				ConEmuAbout::OnInfo_DonateLink();
				return true;
			case pLinkFlattr:
				ConEmuAbout::OnInfo_FlattrLink();
				return true;
			}
		}
		break;

	case WM_DRAWITEM:
		switch (LOWORD(wParam))
		{
		case pLinkDonate:
		case pLinkFlattr:
			for (int iBtnIdx = 0; iBtnIdx < countof(m_Btns); iBtnIdx++)
			{
				if (m_Btns[iBtnIdx].nCtrlId == LOWORD(wParam))
				{
					if (m_Btns[iBtnIdx].pImg)
					{
						DRAWITEMSTRUCT* p = (DRAWITEMSTRUCT*)lParam;
						bool bRc = m_Btns[iBtnIdx].pImg->PaintButton(-1, p->hDC, p->rcItem.left, p->rcItem.top, p->rcItem.right-p->rcItem.left, p->rcItem.bottom-p->rcItem.top);
						if (bRc)
						{
							lResult = TRUE;
							return true;
						}
					}
					break;
				}
			}
			break;
		}
		break;
	}

	return false;
}

void ConEmuAbout::RegisterTip(HWND hDlg)
{
	// Create the ToolTip.
	if (!hwndTip || !IsWindow(hwndTip))
	{
		hwndTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
		                         /* BalloonStyle() | */ WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		                         CW_USEDEFAULT, CW_USEDEFAULT,
		                         CW_USEDEFAULT, CW_USEDEFAULT,
		                         ghOpWnd, NULL,
		                         g_hInstance, NULL);
		if (!hwndTip)
			return;
		SetWindowPos(hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
		//SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);
	}

	for (size_t i = 0; i < countof(m_Btns); i++)
	{
		HWND hChild = GetDlgItem(hDlg, m_Btns[i].nCtrlId);
		if (!hChild)
			continue;

		TOOLINFO toolInfo = {44}; //sizeof(toolInfo); -- need to work on Win2k and compile with Vista+

		toolInfo.hwnd = hChild;
		toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
		toolInfo.uId = (UINT_PTR)hChild;
		toolInfo.lpszText = m_Btns[i].pszUrl;

		SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
	}
}
