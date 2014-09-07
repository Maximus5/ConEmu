
/*
Copyright (c) 2012-2014 Maximus5
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

#include "header.h"
#include "ConEmu.h"
#include "RealConsole.h"
#include "VConRelease.h"
#include "VirtualConsole.h"

#include "ConfirmDlg.h"

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
				ShellExecute(hwnd, L"open", pszHREF, NULL, NULL, SW_SHOWNORMAL);
			break;
		}
	}
	return S_OK;
}

typedef HRESULT (WINAPI* TaskDialogIndirect_t)(const TASKDIALOGCONFIG *pTaskConfig, int *pnButton, int *pnRadioButton, BOOL *pfVerificationFlagChecked);
static TaskDialogIndirect_t TaskDialogIndirect_f = NULL; //(TaskDialogIndirect_t)(hDll?GetProcAddress(hDll, "TaskDialogIndirect"):NULL);

HRESULT TaskDialog(TASKDIALOGCONFIG *pTaskConfig, int *pnButton, int *pnRadioButton, bool *pfVerificationFlagChecked)
{
	HRESULT hr = E_UNEXPECTED;
	BOOL bCheckBox = FALSE;

	if (!TaskDialogIndirect_f)
	{
		HMODULE hDll = GetModuleHandle(L"comctl32.dll");
		if (!hDll)
		{
			hDll = LoadLibrary(L"comctl32.dll");
		}
		TaskDialogIndirect_f = (TaskDialogIndirect_t)(hDll?GetProcAddress(hDll, "TaskDialogIndirect"):NULL);
	}

	if (TaskDialogIndirect_f)
	{
		if (!pTaskConfig->pfCallback)
			pTaskConfig->pfCallback = TaskDlgCallback;
		if (hClassIconSm)
			ghDlgPendingFrom = GetForegroundWindow();

		hr = TaskDialogIndirect_f(pTaskConfig, pnButton, pnRadioButton, &bCheckBox);

		ghDlgPendingFrom = NULL;
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

	wchar_t szText[512], *pszText;
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
	if (gOSVer.dwMajorVersion >= 6)
	{
		// must be already initialized: CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

		wchar_t szMessage[128];
		lstrcpyn(szMessage,
			Parm.asSingleConsole ? Parm.asSingleConsole : Parm.bForceKill ? L"Confirm killing?" : L"Confirm closing?",
			countof(szMessage));

		wchar_t szWWW[MAX_PATH]; _wsprintf(szWWW, SKIPLEN(countof(szWWW)) L"<a href=\"%s\">%s</a>", gsHomePage, gsHomePage);

		wchar_t szCloseAll[MAX_PATH*2]; wchar_t *pszText;
		if (Parm.asSingleConsole)
		{
			wcscpy_c(szCloseAll, L"Yes\n");
			pszText = szCloseAll + _tcslen(szCloseAll);
			lstrcpyn(pszText, Parm.asSingleTitle, min(MAX_PATH,(countof(szCloseAll)-(pszText-szCloseAll))));
			pszText += _tcslen(pszText);
		}
		else
		{
			_wsprintf(szCloseAll, SKIPLEN(countof(szCloseAll))
				(Parm.bGroup && (Parm.nConsoles>1))
					? ((Parm.bGroup == ConfirmCloseParam::eGroup)
						? L"Close group (%u console%s)"
						: L"Close (%u console%s)")
					: L"Close all %u console%s.",
				Parm.nConsoles, (Parm.nConsoles>1)?L"s":L"");
			pszText = szCloseAll + _tcslen(szCloseAll);
		}
		if ((Parm.asSingleConsole == NULL) || (Parm.nOperations || Parm.nUnsavedEditors))
		{
			//if (nOperations)
			{
				_wsprintf(pszText, SKIPLEN(countof(szCloseAll)-(pszText-szCloseAll)) L"\nIncomplete operations: %i", Parm.nOperations);
				pszText += _tcslen(pszText);
			}
			//if (nUnsavedEditors)
			{
				_wsprintf(pszText, SKIPLEN(countof(szCloseAll)-(pszText-szCloseAll)) L"\nUnsaved editor windows: %i", Parm.nUnsavedEditors);
				pszText += _tcslen(pszText);
			}
		}

		wchar_t szCloseOne[MAX_PATH];
		wcscpy_c(szCloseOne, L"Close active console only");
		if (Parm.nConsoles > 1)
		{
			CVConGuard VCon;
			int iCon = gpConEmu->GetActiveVCon(&VCon);
			if (iCon >= 0)
			{
				pszText = szCloseOne + _tcslen(szCloseOne);
				_wsprintf(pszText, SKIPLEN(countof(szCloseOne)-(pszText-szCloseOne)) L"\n#%u: ", (iCon+1));
				pszText += _tcslen(pszText);
				lstrcpyn(pszText, VCon->RCon()->GetTitle(true), countof(szCloseOne)-(pszText-szCloseOne));
			}
		}

		const wchar_t* szCancel = L"Cancel\nDon't close anything";


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
		config.hInstance                    = NULL /*g_hInstance*/;
		config.dwFlags                      = TDF_USE_HICON_MAIN|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION
		                                      |TDF_ENABLE_HYPERLINKS; //|TDIF_SIZE_TO_CONTENT|TDF_CAN_BE_MINIMIZED;
		//config.pszMainIcon                  = MAKEINTRESOURCE(IDI_ICON1);
		config.hMainIcon                    = hClassIcon;
		config.pszWindowTitle               = gpConEmu->GetDefaultTitle();
		config.pszMainInstruction           = szMessage;
		//config.pszContent                 = L"...";
		config.pButtons                     = buttons;
		config.nDefaultButton               = IDYES;
		config.pszFooter                    = szWWW;

		//{
		//	config.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;
		//	config.pszVerificationText = L"Text on checkbox";
		//}

		HRESULT hr = TaskDialog(&config, &nButtonPressed, NULL, NULL);

		if (hr == S_OK)
		{
			switch (nButtonPressed)
			{
			case IDCANCEL: // user cancelled the dialog
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

	// Иначе - через стандартный MessageBox

	if (Parm.asSingleConsole)
	{
		lstrcpyn(szText,
			Parm.asSingleConsole ? Parm.asSingleConsole : Parm.bForceKill ? L"Confirm killing?" : L"Confirm closing?",
			min(128,countof(szText)));
		wcscat_c(szText, L"\r\n\r\n");
		int nLen = lstrlen(szText);
		lstrcpyn(szText+nLen, Parm.asSingleTitle, countof(szText)-nLen);
	}
	else
	{
		_wsprintf(szText, SKIPLEN(countof(szText)) L"About to close %u console%s.\r\n", Parm.nConsoles, (Parm.nConsoles>1)?L"s":L"");
	}
	pszText = szText+_tcslen(szText);

	if (Parm.nOperations || Parm.nUnsavedEditors)
	{
		*(pszText++) = L'\r'; *(pszText++) = L'\n'; *(pszText) = 0;

		if (Parm.nOperations)
		{
			_wsprintf(pszText, SKIPLEN(countof(szText)-(pszText-szText)) L"Incomplete operations: %i\r\n", Parm.nOperations);
			pszText += _tcslen(pszText);
		}
		if (Parm.nUnsavedEditors)
		{
			_wsprintf(pszText, SKIPLEN(countof(szText)-(pszText-szText)) L"Unsaved editor windows: %i\r\n", Parm.nUnsavedEditors);
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
		nBtn = IDYES; // для однозначности
	}

wrap:
	InterlockedDecrement(&lCounter);
	return nBtn;
}
