
/*
Copyright (c) 2012-present Maximus5
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
#include "../common/MModule.h"

#include "ConEmu.h"
#include "DontEnable.h"
#include "RealConsole.h"
#include "VConRelease.h"
#include "VirtualConsole.h"

#include "ConfirmDlg.h"

#include "LngRc.h"

static HRESULT CALLBACK TaskDlgCallback(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData)
{
	switch (uNotification)
	{
		case TDN_CREATED:
		{
			DWORD nStyleEx = GetWindowLong(hwnd, GWL_EXSTYLE);
			if (!(nStyleEx & WS_EX_TOPMOST))
			{
				if ((ghWnd && ((nStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE)) & WS_EX_TOPMOST))
					|| (ghOpWnd && ((nStyleEx = GetWindowLong(ghOpWnd, GWL_EXSTYLE)) & WS_EX_TOPMOST)))
				{
					SetWindowPos(hwnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
				}
			}
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(CreateNullIcon()));
			//if (!gbAlreadyAdmin)
			//{
			//	SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver86, 1);
			//	SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver64, 1);
			//}
			break;
		}
		case TDN_VERIFICATION_CLICKED:
		{
			//SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver86, wParam);
			//SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver64, wParam);
			break;
		}
		case TDN_HYPERLINK_CLICKED:
		{
			PCWSTR pszHREF = (PCWSTR)lParam;
			if (pszHREF && *pszHREF)
				ShellExecute(hwnd, L"open", pszHREF, nullptr, nullptr, SW_SHOWNORMAL);
			break;
		}
	}
	return S_OK;
}

static MModule gComCtl32;
typedef HRESULT (WINAPI* TaskDialogIndirect_t)(const TASKDIALOGCONFIG *pTaskConfig, int *pnButton, int *pnRadioButton, BOOL *pfVerificationFlagChecked);
static TaskDialogIndirect_t TaskDialogIndirect_f = nullptr; //(TaskDialogIndirect_t)(hDll?GetProcAddress(hDll, "TaskDialogIndirect"):nullptr);

HRESULT TaskDialog(TASKDIALOGCONFIG *pTaskConfig, int *pnButton, int *pnRadioButton, bool *pfVerificationFlagChecked)
{
	HRESULT hr = E_UNEXPECTED;
	BOOL bCheckBox = FALSE;

	if (!TaskDialogIndirect_f)
	{
		if (gComCtl32.Load(L"comctl32.dll"))
		{
			gComCtl32.GetProcAddress("TaskDialogIndirect", TaskDialogIndirect_f);
		}
	}

	if (TaskDialogIndirect_f)
	{
		ghDlgPendingFrom = nullptr;
		if (!pTaskConfig->pfCallback)
			pTaskConfig->pfCallback = TaskDlgCallback;
		else if (hClassIconSm)
			ghDlgPendingFrom = GetForegroundWindow();

		hr = TaskDialogIndirect_f(pTaskConfig, pnButton, pnRadioButton, &bCheckBox);

		ghDlgPendingFrom = nullptr;
	}

	if (pfVerificationFlagChecked)
		*pfVerificationFlagChecked = (bCheckBox != FALSE);

	return hr;
}

// IDYES    - Close All consoles
// IDNO     - Close active console only
// IDCANCEL - As is
int ConfirmCloseConsoles(const ConfirmCloseParam& Parm)
{
	DontEnable de;

	wchar_t szText[512] = L"", *pszText = nullptr;
	int nBtn = IDCANCEL;

	static LONG lCounter = 0;
	LONG l = InterlockedIncrement(&lCounter);
	if (l > 1)
	{
		if (l == 2)
		{
			_ASSERTE(FALSE && "Confirm stack overflow!");
		}
		goto wrap;
	}

	if (Parm.rpLeaveConEmuOpened) *Parm.rpLeaveConEmuOpened = false;

	// Use TaskDialog?
	if (IsWin6())
	{
		// must be already initialized: CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

		CEStr btnYes, btnCancel;
		CLngRc::getControl(IDYES, btnYes, L"Yes");
		CLngRc::getControl(IDCANCEL, btnCancel, L"Cancel");

		wchar_t szMessage[128];
		if (Parm.asSingleConsole)
			lstrcpyn(szMessage, Parm.asSingleConsole, countof(szMessage));
		else if (Parm.bForceKill)
			wcscpy_c(szMessage, CLngRc::getRsrc(lng_ConfirmKillingQ/*"Confirm killing?"*/));
		else if (Parm.bGroup)
			wcscpy_c(szMessage, CLngRc::getRsrc(lng_ConfirmCloseGroupQ/*"Confirm closing group?"*/));
		else
			wcscpy_c(szMessage, CLngRc::getRsrc(lng_ConfirmClosingQ/*"Confirm closing?"*/));

		wchar_t webLink[MAX_PATH]; swprintf_c(webLink, L"<a href=\"%s\">%s</a>", gsHomePage, gsHomePage);

		wchar_t szCloseAll[MAX_PATH*2];
		if (Parm.asSingleConsole)
		{
			wcscpy_c(szCloseAll, btnYes);
			wcscat_c(szCloseAll, L"\n");
			pszText = szCloseAll + _tcslen(szCloseAll);
			const int iLen = lstrlen(Parm.asSingleTitle);
			const int iCozyLen = 40;
			const int cchMax = static_cast<int>(countof(szCloseAll) - (pszText - szCloseAll));
			if (iLen <= std::min(iCozyLen, cchMax))
			{
				lstrcpyn(pszText, Parm.asSingleTitle, cchMax);
			}
			else
			{
				int iMax = std::min(iCozyLen, cchMax)-3;
				lstrcpyn(pszText, Parm.asSingleTitle, iMax);
				if ((iMax + 4) < cchMax)
					_wcscat_c(pszText, cchMax, L"...");
			}
			pszText += _tcslen(pszText);
		}
		else
		{
			wchar_t szCount[16]; swprintf_c(szCount, L" %u ", Parm.nConsoles);
			const auto* closeGroupOf = CLngRc::getRsrc(lng_ConfirmCloseBtnGroup/*"Close group of"*/);
			const auto* close = CLngRc::getRsrc(lng_ConfirmCloseBtn/*"Close"*/);
			const auto* closeAll = CLngRc::getRsrc(lng_ConfirmCloseBtnAll/*"Close all"*/);
			const auto* consoles = CLngRc::getRsrc(lng_ConfirmCloseBtnConsoles/*"consoles"*/);
			if (Parm.bGroup && (Parm.nConsoles > 1))
			{
				if (Parm.bGroup == ConfirmCloseParam::eGroup)
					wcscpy_c(szCloseAll, CEStr(closeGroupOf, szCount, consoles));
				else
					wcscpy_c(szCloseAll, CEStr(close, szCount, consoles));
			}
			else
			{
				if (Parm.nConsoles > 1)
					wcscpy_c(szCloseAll, CEStr(closeAll, szCount, consoles));
				else
					wcscpy_c(szCloseAll, CLngRc::getRsrc(lng_ConfirmCloseBtnConsole/*"Close 1 console"*/));
			}
			pszText = szCloseAll + _tcslen(szCloseAll);
		}
		if ((Parm.asSingleConsole == nullptr) || (Parm.nOperations || Parm.nUnsavedEditors))
		{
			//if (nOperations)
			{
				swprintf_c(pszText, countof(szCloseAll)-(pszText-szCloseAll)/*#SECURELEN*/, L"\n%s: %i",
					CLngRc::getRsrc(lng_ConfirmIncompleteOperations/*"Incomplete operations"*/), Parm.nOperations);
				pszText += _tcslen(pszText);
			}
			//if (nUnsavedEditors)
			{
				swprintf_c(pszText, countof(szCloseAll)-(pszText-szCloseAll)/*#SECURELEN*/, L"\n%s: %i",
					CLngRc::getRsrc(lng_ConfirmUnsavedEditors/*"Unsaved editor windows"*/), Parm.nUnsavedEditors);
				pszText += _tcslen(pszText);
			}
		}

		wchar_t szCloseOne[MAX_PATH];
		wcscpy_c(szCloseOne, CLngRc::getRsrc(lng_ConfirmCloseActiveOnly/*"Close active console only"*/));
		if (Parm.nConsoles > 1)
		{
			CVConGuard VCon;
			int iCon = gpConEmu->GetActiveVCon(&VCon);
			if (iCon >= 0)
			{
				pszText = szCloseOne + _tcslen(szCloseOne);
				swprintf_c(pszText, countof(szCloseOne)-(pszText-szCloseOne)/*#SECURELEN*/, L"\n#%u: ", (iCon+1));
				pszText += _tcslen(pszText);
				const int cchMax = static_cast<int>(countof(szCloseOne) - (pszText - szCloseOne));
				lstrcpyn(pszText, VCon->RCon()->GetTitle(true), cchMax);
			}
		}

		const CEStr szCancel(btnCancel, L"\n", CLngRc::getRsrc(lng_ConfirmDontCloseAnything/*"Don't close anything"*/));


		int nButtonPressed                  = 0;
		TASKDIALOGCONFIG config             = {sizeof(config)};
		TASKDIALOG_BUTTON buttons[]   = {
			{ IDYES,    szCloseAll },
			{ IDNO,     szCloseOne },
			{ IDCANCEL, szCancel },
		};
		config.cButtons                     = countof(buttons);
		if (Parm.nConsoles <= 1)
		{
			buttons[1] = buttons[2];
			config.cButtons--;
		}

		config.hwndParent                   = ghWnd;
		config.hInstance                    = nullptr /*g_hInstance*/;
		config.dwFlags                      = TDF_USE_HICON_MAIN|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION
		                                      |TDF_ENABLE_HYPERLINKS; //|TDIF_SIZE_TO_CONTENT|TDF_CAN_BE_MINIMIZED;
		//config.pszMainIcon                  = MAKEINTRESOURCE(IDI_ICON1);
		config.hMainIcon                    = hClassIcon;
		config.pszWindowTitle               = gpConEmu->GetDefaultTitle();
		config.pszMainInstruction           = szMessage;
		//config.pszContent                 = L"...";
		config.pButtons                     = buttons;
		config.nDefaultButton               = IDYES;
		config.pszFooter                    = webLink;

		//{
		//	config.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;
		//	config.pszVerificationText = L"Text on checkbox";
		//}

		const HRESULT hr = TaskDialog(&config, &nButtonPressed, nullptr, nullptr);

		if (hr == S_OK)
		{
			switch (nButtonPressed)
			{
			case IDCANCEL: // user canceled the dialog
			case IDYES:
			case IDNO:
				nBtn = nButtonPressed;
				goto wrap;

			default:
				_ASSERTE(nButtonPressed==IDCANCEL||nButtonPressed==IDYES||nButtonPressed==IDNO);
				break; // should never happen
			}
		}
	}

	// TaskDialogs aren't available in current Windows version, use standart MsgBox instead

	if (Parm.asSingleConsole)
	{
		lstrcpyn(szText,
			Parm.asSingleConsole ? Parm.asSingleConsole : Parm.bForceKill ? L"Confirm killing?" : L"Confirm closing?",
			std::min<int>(128, countof(szText)));
		wcscat_c(szText, L"\r\n\r\n");
		const int nLen = lstrlen(szText);
		lstrcpyn(szText+nLen, Parm.asSingleTitle, countof(szText)-nLen);
	}
	else
	{
		swprintf_c(szText, L"About to close %u console%s.\r\n", Parm.nConsoles, (Parm.nConsoles>1)?L"s":L"");
	}
	// ReSharper disable once CppJoinDeclarationAndAssignment
	pszText = szText + _tcslen(szText);

	if (Parm.nOperations || Parm.nUnsavedEditors)
	{
		*(pszText++) = L'\r'; *(pszText++) = L'\n'; *(pszText) = 0;

		if (Parm.nOperations)
		{
			swprintf_c(pszText, countof(szText)-(pszText-szText)/*#SECURELEN*/, L"Incomplete operations: %i\r\n", Parm.nOperations);
			pszText += _tcslen(pszText);
		}
		if (Parm.nUnsavedEditors)
		{
			swprintf_c(pszText, countof(szText)-(pszText-szText)/*#SECURELEN*/, L"Unsaved editor windows: %i\r\n", Parm.nUnsavedEditors);
			pszText += _tcslen(pszText);
		}
	}

	if (Parm.nConsoles > 1)
	{
		wcscat_c(szText,
			L"\r\nPress button <No> to close active console only\r\n"
			L"\r\nProceed with close ConEmu?");
	}

	nBtn = MsgBox(szText, (/*rpPanes ? MB_OKCANCEL :*/ (Parm.nConsoles>1) ? MB_YESNOCANCEL : MB_OKCANCEL)|MB_ICONEXCLAMATION,
		gpConEmu->GetDefaultTitle(), ghWnd);

	if (nBtn == IDOK)
	{
		nBtn = IDYES; // for unambiguity
	}

wrap:
	InterlockedDecrement(&lCounter);
	return nBtn;
}

// uType - flags from MsgBox, i.e. MB_YESNOCANCEL
int ConfirmDialog(LPCWSTR asMessage,
	LPCWSTR asMainLabel, LPCWSTR asCaption, LPCWSTR asUrl, UINT uType, HWND ahParent,
	LPCWSTR asBtn1Name /*= nullptr*/, LPCWSTR asBtn1Hint /*= nullptr*/,
	LPCWSTR asBtn2Name /*= nullptr*/, LPCWSTR asBtn2Hint /*= nullptr*/,
	LPCWSTR asBtn3Name /*= nullptr*/, LPCWSTR asBtn3Hint /*= nullptr*/)
{
	DontEnable de;

	int nBtn = IDCANCEL, nBtnCount = 0;
	CEStr szText, szWebLink, lsBtn1, lsBtn2, lsBtn3;
	LPCWSTR pszName1 = nullptr, pszName2 = nullptr, pszName3 = nullptr;
	TASKDIALOG_BUTTON buttons[3] = {};

	CEStr btnOk, btnCancel, btnAbort, btnRetry, btnIgnore, btnTry, btnYes, btnNo, btnContinue;

	static LONG lCounter = 0;
	LONG l = InterlockedIncrement(&lCounter);
	if (l > 1)
	{
		if (l == 2)
		{
			_ASSERTE(FALSE && "Confirm stack overflow!");
		}
		goto wrap;
	}

	if (!asUrl || !*asUrl)
		asUrl = gsHomePage;
	szWebLink = CEStr(L"<a href=\"", asUrl, L"\">", asUrl, L"</a>");

	switch (uType & 0xF)
	{
	case MB_OK:
		pszName1 = CLngRc::getControl(IDOK, btnOk, L"OK");
		buttons[0].nButtonID = IDOK;
		nBtnCount = 1; break;
	case MB_OKCANCEL:
		pszName1 = CLngRc::getControl(IDOK, btnOk, L"OK");
		buttons[0].nButtonID = IDOK;
		pszName2 = CLngRc::getControl(IDCANCEL, btnCancel, L"Cancel");
		buttons[1].nButtonID = IDCANCEL;
		nBtnCount = 2; break;
	case MB_ABORTRETRYIGNORE:
		pszName1 = CLngRc::getControl(IDABORT, btnAbort, L"Abort");
		buttons[0].nButtonID = IDABORT;
		pszName2 = CLngRc::getControl(IDRETRY, btnRetry, L"Retry");
		buttons[1].nButtonID = IDRETRY;
		pszName3 = CLngRc::getControl(IDIGNORE, btnIgnore, L"Ignore");
		buttons[2].nButtonID = IDIGNORE;
		nBtnCount = 3; break;
	case MB_YESNOCANCEL:
		pszName1 = CLngRc::getControl(IDYES, btnYes, L"Yes");
		buttons[0].nButtonID = IDYES;
		pszName2 = CLngRc::getControl(IDNO, btnNo, L"No");
		buttons[1].nButtonID = IDNO;
		pszName3 = CLngRc::getControl(IDCANCEL, btnCancel, L"Cancel");
		buttons[2].nButtonID = IDCANCEL;
		nBtnCount = 3; break;
	case MB_YESNO:
		pszName1 = CLngRc::getControl(IDYES, btnYes, L"Yes");
		buttons[0].nButtonID = IDYES;
		pszName2 = CLngRc::getControl(IDNO, btnNo, L"No");
		buttons[1].nButtonID = IDNO;
		nBtnCount = 2; break;
	case MB_RETRYCANCEL:
		pszName1 = CLngRc::getControl(IDRETRY, btnRetry, L"Retry");
		buttons[0].nButtonID = IDRETRY;
		pszName2 = CLngRc::getControl(IDCANCEL, btnCancel, L"Cancel");
		buttons[1].nButtonID = IDCANCEL;
		nBtnCount = 2; break;
	case MB_CANCELTRYCONTINUE:
		pszName1 = CLngRc::getControl(IDCANCEL, btnCancel, L"Cancel");
		buttons[0].nButtonID = IDCANCEL;
		pszName2 = CLngRc::getControl(IDTRYAGAIN, btnTry, L"Try");
		buttons[1].nButtonID = IDTRYAGAIN;
		pszName3 = CLngRc::getControl(IDCONTINUE, btnContinue, L"Continue");
		buttons[2].nButtonID = IDCONTINUE;
		nBtnCount = 3; break;
	default:
		_ASSERTE(FALSE && "Flag not supported");
		goto wrap;
	}

	// Use TaskDialog?
	if (gOSVer.dwMajorVersion >= 6)
	{
		HRESULT hr = E_FAIL;
		int nButtonPressed = 0;
		TASKDIALOGCONFIG config = {sizeof(config)};

		config.cButtons = nBtnCount;

		// must be already initialized: CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

		switch (uType & 0xF00)
		{
		case MB_DEFBUTTON3:
			config.nDefaultButton = buttons[2].nButtonID; break;
		case MB_DEFBUTTON2:
			config.nDefaultButton = buttons[1].nButtonID; break;
		default:
			config.nDefaultButton = buttons[0].nButtonID;
		}

		if (config.cButtons >= 1)
		{
			lsBtn1 = CEStr(asBtn1Name ? asBtn1Name : pszName1, asBtn1Hint ? L"\n" : nullptr, asBtn1Hint);
			buttons[0].pszButtonText = lsBtn1.ms_Val;
		}
		if (config.cButtons >= 2)
		{
			lsBtn2 = CEStr(asBtn2Name ? asBtn2Name : pszName2, asBtn2Hint ? L"\n" : nullptr, asBtn2Hint);
			buttons[1].pszButtonText = lsBtn2.ms_Val;
		}
		if (config.cButtons >= 3)
		{
			lsBtn3 = CEStr(asBtn3Name ? asBtn3Name : pszName3, asBtn3Hint ? L"\n" : nullptr, asBtn3Hint);
			buttons[2].pszButtonText = lsBtn3.ms_Val;
		}

		config.hwndParent                   = ahParent;
		config.dwFlags                      = /*TDF_USE_HICON_MAIN|*/TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION
		                                      |TDF_ENABLE_HYPERLINKS; //|TDIF_SIZE_TO_CONTENT|TDF_CAN_BE_MINIMIZED;
		config.pszWindowTitle               = asCaption ? asCaption : gpConEmu->GetDefaultTitle();
		config.pszMainInstruction           = asMainLabel;
		config.pszContent                   = asMessage;
		config.pButtons                     = buttons;
		config.pszFooter                    = szWebLink;

		config.hInstance = g_hInstance;
		config.pszMainIcon = MAKEINTRESOURCE(IDI_ICON1);


		// Show dialog
		hr = TaskDialog(&config, &nButtonPressed, nullptr, nullptr);

		if (hr == S_OK)
		{
			switch (nButtonPressed)
			{
			case IDOK:
			case IDCANCEL:
			case IDABORT:
			case IDRETRY:
			case IDIGNORE:
			case IDYES:
			case IDNO:
			case IDTRYAGAIN:
			case IDCONTINUE:
				nBtn = nButtonPressed;
				goto wrap;

			default:
				_ASSERTE(FALSE && "Unsupported result value");
				break; // should never happen
			}
		}
	}


	// **********************************
	// Use standard message box otherwise
	// **********************************

	szText = CEStr(
		asMainLabel, asMainLabel ? L"\r\n" : nullptr,
		asMessage);

	// URL hint?
	if (asUrl)
	{
		szText.Append(L"\r\n\r\n", asUrl);
	}

	// Button hints?
	if (asBtn1Hint || asBtn2Hint || asBtn3Hint)
	{
		szText.Append(L"\r\n\r\n");
		if (asBtn1Hint)
			szText.Append(L"\r\n<", pszName1, L"> - ", asBtn1Hint);
		if (asBtn2Hint)
			szText.Append(L"\r\n<", pszName2, L"> - ", asBtn2Hint);
		if (asBtn3Hint)
			szText.Append(L"\r\n<", pszName3, L"> - ", asBtn3Hint);
	}

	// Show dialog
	nBtn = MsgBox(szText, uType, asCaption, ahParent);
wrap:
	InterlockedDecrement(&lCounter);
	return nBtn;
}
