
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

#define SHOWDEBUGSTR

#include "Header.h"
#include "About.h"
#include "AboutDlg.h"
#include "ConEmu.h"
#include "OptionsClass.h"
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

	INT_PTR WINAPI aboutProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);

	void OnInfo_DonateLink();
	void OnInfo_FlattrLink();

	struct BtnInfo
	{
		LPCWSTR ResId;
		HBITMAP hBmp;
		BITMAP bmp;
		LPWSTR pszUrl;
		UINT nCtrlId;
	} m_Btns[2] = {};

	void LoadResources();

	wchar_t gsDonatePage[] = CEDONATEPAGE;
	wchar_t gsFlattrPage[] = CEFLATTRPAGE;

	HWND hwndTip = NULL;
	void RegisterTip(HWND hDlg);
};

INT_PTR WINAPI ConEmuAbout::aboutProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
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

	INT_PTR lRc = 0;
	if (DonateBtns_Process(hDlg, messg, wParam, lParam, lRc))
	{
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lRc);
		return TRUE;
	}

	switch (messg)
	{
		case WM_INITDIALOG:
		{
			gpConEmu->OnOurDialogOpened();
			mh_AboutDlg = hDlg;

			if ((ghOpWnd && IsWindow(ghOpWnd)) || (WS_EX_TOPMOST & GetWindowLongPtr(ghWnd, GWL_EXSTYLE)))
			{
				SetWindowPos(hDlg, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
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
			SetWindowText(hDlg, szTitle);

			if (hClassIcon)
			{
				SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
				SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);
				SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)hClassIcon);
			}

			SetDlgItemText(hDlg, stConEmuAbout, pAboutTitle);
			SetDlgItemText(hDlg, stConEmuUrl, gsHomePage);

			DonateBtns_Add(hDlg, pIconCtrl, IDOK);

			HWND hTab = GetDlgItem(hDlg, tbAboutTabs);
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

			SetDlgItemText(hDlg, tAboutText, Pages[nPage].Text);

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
			if (HIWORD(wParam) == BN_CLICKED)
			{
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
					SetDlgItemText(hDlg, tAboutText, Pages[iPage].Text);
			}
			break;
		}

		case WM_CLOSE:
			//if (ghWnd == NULL)
			gpConEmu->OnOurDialogClosed();
			EndDialog(hDlg, IDOK);
			//else
			//	DestroyWindow(hDlg);
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
		L"You can show your appreciation and support future development by donating.\n\n"
		L"Open PayPal website?"
		//L"Donate (PayPal) button located on project website\r\n"
		//L"under ConEmu sketch (upper right of page).\r\n\r\n"
		//L"Open project website?"
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


void ConEmuAbout::OnInfo_About(LPCWSTR asPageName /*= NULL*/)
{
	InitCommCtrls();

	bool bOk = false;

	{
		DontEnable de;
		HWND hParent = (ghOpWnd && IsWindowVisible(ghOpWnd)) ? ghOpWnd : ghWnd;
		// Modal dialog
		INT_PTR iRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_ABOUT), hParent, aboutProc, (LPARAM)asPageName);
		bOk = (iRc != 0 && iRc != -1);

		ZeroStruct(m_Btns);
		mh_AboutDlg = NULL;

		#ifdef _DEBUG
		// Any problems with dialog resource?
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

void ConEmuAbout::LoadResources()
{
	struct ImgLoadInfo
	{
		LPCWSTR ResId; UINT nCtrlId; int nWidth; int nHeight;
	} Images[] = {
		{L"DONATE", pLinkDonate, 74, 21},
		{L"FLATTR", pLinkFlattr, 89, 18},
	};
	_ASSERTE(countof(m_Btns) == countof(Images));

	for (size_t i = 0; i < countof(Images); i++)
	{
		if (m_Btns[i].hBmp)
			continue; // Already loaded
		
		m_Btns[i].ResId = Images[i].ResId;
		m_Btns[i].nCtrlId = Images[i].nCtrlId;
		m_Btns[i].pszUrl = (Images[i].nCtrlId == pLinkDonate) ? gsDonatePage : gsFlattrPage;
		m_Btns[i].hBmp = (HBITMAP)LoadImage(g_hInstance, Images[i].ResId, IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT|LR_LOADMAP3DCOLORS);

		if (m_Btns[i].hBmp == NULL)
		{
			#ifdef _DEBUG
			DWORD nErr = GetLastError();
			DisplayLastError(L"Failed to load button image!", nErr);
			#endif
			m_Btns[i].bmp.bmWidth = Images[i].nWidth; m_Btns[i].bmp.bmHeight = Images[i].nHeight;
		}
		else
		{
			ZeroStruct(m_Btns[i].bmp);
			if (!GetObject(m_Btns[i].hBmp, sizeof(m_Btns[i].bmp), &m_Btns[i].bmp) || m_Btns[i].bmp.bmWidth <= 0 || m_Btns[i].bmp.bmHeight <= 0)
			{
				#ifdef _DEBUG
				DWORD nErr = GetLastError();
				DisplayLastError(L"GetObject() on button image failed!", nErr);
				#endif
				m_Btns[i].bmp.bmWidth = Images[i].nWidth; m_Btns[i].bmp.bmHeight = Images[i].nHeight;
			}
		}
	}
}

void ConEmuAbout::DonateBtns_Add(HWND hDlg, int AlignLeftId, int AlignVCenterId)
{
	if (!m_Btns[0].hBmp)
		LoadResources();

	RECT rcLeft = {}, rcTop = {};
	HWND hCtrl;

	hCtrl = GetDlgItem(hDlg, AlignLeftId);
	GetWindowRect(hCtrl, &rcLeft);
	MapWindowPoints(NULL, hDlg, (LPPOINT)&rcLeft, 2);
	hCtrl = GetDlgItem(hDlg, AlignVCenterId);
	GetWindowRect(hCtrl, &rcTop);
	MapWindowPoints(NULL, hDlg, (LPPOINT)&rcTop, 2);

	int nDisplayDpi = gpSetCls->QueryDpi();

	int X = rcLeft.left;

	for (size_t i = 0; i < countof(m_Btns); i++)
	{
		if (!m_Btns[i].hBmp)
			continue; // Image was failed

		TODO("Вертикальное центрирование по объекту AlignVCenterId");

		int nDispW = m_Btns[i].bmp.bmWidth * nDisplayDpi / 96;
		int nDispH = m_Btns[i].bmp.bmHeight * nDisplayDpi / 96;
		int nY = rcTop.top + ((rcTop.bottom - rcTop.top - nDispH + 1) / 2);

		hCtrl = CreateWindow(L"STATIC", m_Btns[i].ResId,
			WS_CHILD|WS_VISIBLE|SS_NOTIFY|SS_OWNERDRAW,
			X, nY, nDispW, nDispH,
			hDlg, (HMENU)m_Btns[i].nCtrlId, g_hInstance, NULL);

		#ifdef _DEBUG
		if (!hCtrl)
			DisplayLastError(L"Failed to create image button control");
		#endif

		X += nDispW + (10 * nDisplayDpi / 96);
	}

	RegisterTip(hDlg);

	UNREFERENCED_PARAMETER(hCtrl);
}

bool ConEmuAbout::DonateBtns_Process(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam, INT_PTR& lResult)
{
	size_t iBtnIdx;

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
			iBtnIdx = (LOWORD(wParam) == pLinkDonate)?0:1;
			if (m_Btns[iBtnIdx].hBmp)
			{
				bool bRc = false;
				DRAWITEMSTRUCT* p = (DRAWITEMSTRUCT*)lParam;
				HDC hdcComp = CreateCompatibleDC(p->hDC);
				if (hdcComp)
				{
					HBITMAP hOld = (HBITMAP)SelectObject(hdcComp, m_Btns[iBtnIdx].hBmp);
					bRc = (StretchBlt(p->hDC, p->rcItem.left, p->rcItem.top, p->rcItem.right-p->rcItem.left, p->rcItem.bottom-p->rcItem.top,
						hdcComp, 0, 0, m_Btns[iBtnIdx].bmp.bmWidth, m_Btns[iBtnIdx].bmp.bmHeight, SRCCOPY) != 0);
					SelectObject(hdcComp, hOld);
					DeleteDC(hdcComp);
				}
				if (bRc)
				{
					lResult = TRUE;
					return true;
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
