
/*
Copyright (c) 2013-2017 Maximus5
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
#include "DpiAware.h"
#include "DynDialog.h"
#include "LngRc.h"
#include "OptionsHelp.h"

//#define DEBUGSTRFONT(s) DEBUGSTR(s)

CEHelpPopup::CEHelpPopup()
{
	mh_Popup = NULL;
	mp_Dlg = NULL;
	mp_DpiAware = NULL;
}

bool CEHelpPopup::GetItemHelp(int wID /* = 0*/, HWND hCtrl, wchar_t* rsHelp, DWORD cchHelpMax)
{
	_ASSERTE(rsHelp && cchHelpMax);
	_ASSERTE((wID!=0 && wID!=-1) || (hCtrl!=NULL));

	*rsHelp = 0;

	if (!wID)
	{
		wID = GetDlgCtrlID(hCtrl);
	}

	bool bNotGroupRadio = false;

	wchar_t szClass[128];

	if (GetClassName(hCtrl, szClass, countof(szClass)))
	{
		if ((wID == -1) && (lstrcmpi(szClass, L"Static") == 0))
		{
			// If it is STATIC with IDC_STATIC - get next ctrl (edit/combo/so on)
			wID = GetWindowLong(FindWindowEx(GetParent(hCtrl), hCtrl, NULL, NULL), GWL_ID);
		}
		else if (lstrcmpi(szClass, L"Button") == 0)
		{
			DWORD dwStyle = GetWindowLong(hCtrl, GWL_STYLE);
			if ((dwStyle & (BS_AUTORADIOBUTTON|BS_RADIOBUTTON)) && !(dwStyle & WS_GROUP))
			{
				bNotGroupRadio = true;
			}
		}
		else if (lstrcmpi(szClass, L"Edit") == 0)
		{
			HWND hParent = GetParent(hCtrl);
			if (GetClassName(hParent, szClass, countof(szClass))
				&& (lstrcmpi(szClass, L"ComboBox") == 0))
			{
				wID = GetWindowLong(hParent, GWL_ID);
			}
		}
	}

	if (wID && (wID != -1))
	{
		// Is there hint for this control?
		if (!CLngRc::getHint(wID, rsHelp, cchHelpMax))
		{
			*rsHelp = 0;

			if (bNotGroupRadio)
			{
				TODO("Try to find first 'previous' control with WS_GROUP style, and may be for GroupBox if WS_GROUP has not tip");
			}
		}
	}

	return (*rsHelp != 0);
}

void CEHelpPopup::ShowItemHelp(HELPINFO* hi)
{
	ShowItemHelp(hi->iCtrlId, (HWND)hi->hItemHandle, hi->MousePos);
}

void CEHelpPopup::ShowItemHelp(int wID /* = 0*/, HWND hCtrl, POINT MousePos)
{
	wchar_t szHint[2000];

	if (!GetItemHelp(wID, hCtrl, szHint, countof(szHint)))
		return;

	if (!*szHint)
		return;

	if (!mh_Popup || !IsWindow(mh_Popup))
	{
		if (!mp_DpiAware)
			mp_DpiAware = new CDpiForDialog();
		// (CreateDialog)
		SafeDelete(mp_Dlg);
		mp_Dlg = CDynDialog::ShowDialog(IDD_HELP, ghOpWnd, helpProc, (LPARAM)this);
		mh_Popup = mp_Dlg ? mp_Dlg->mh_Dlg : NULL;
		if (!mh_Popup)
		{
			DisplayLastError(L"Can't create help text dialog", GetLastError());
			return;
		}

		SetWindowPos(mh_Popup, HWND_TOPMOST, MousePos.x + 10, MousePos.y + 20, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE);
	}

	if (mh_Popup && IsWindow(mh_Popup))
	{
		if (!IsWindowVisible(mh_Popup))
			ShowWindow(mh_Popup, SW_SHOWNA);
		//SetForegroundWindow(mh_Popup);
		SetDlgItemText(mh_Popup, IDC_HELP_DESCR, szHint);
		return;
	}
}

INT_PTR CEHelpPopup::helpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	CEHelpPopup* pPopup = NULL;

	switch (messg)
	{
		case WM_INITDIALOG:
		{
			pPopup = (CEHelpPopup*)lParam;
			SetWindowLongPtr(hWnd2, DWLP_USER, (LONG_PTR)pPopup);
			pPopup->mh_Popup = hWnd2;
			if (pPopup->mp_DpiAware)
				pPopup->mp_DpiAware->Attach(hWnd2, ghOpWnd, pPopup->mp_Dlg);
			helpProc(hWnd2, WM_SIZE, 0, 0);

			break;
		}

		case WM_SIZE:
		{
			RECT rcClient; GetClientRect(hWnd2, &rcClient);
			MoveWindowRect(GetDlgItem(hWnd2, IDC_HELP_DESCR), rcClient, TRUE);
			break;
		}

		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				switch (LOWORD(wParam))
				{
				case IDCLOSE:
				case IDCANCEL:
					DestroyWindow(hWnd2);
					return 0;
				default:
					return 0;
				}
			}
			break;

		case WM_CLOSE:
			DestroyWindow(hWnd2);
			break;

		case WM_DESTROY:
		{
			pPopup = (CEHelpPopup*)GetWindowLongPtr(hWnd2, DWLP_USER);
			if (pPopup)
			{
				if (pPopup->mp_DpiAware)
					pPopup->mp_DpiAware->Detach();
				pPopup->mh_Popup = NULL;
				SafeDelete(pPopup->mp_Dlg);
			}
			break;
		}

		default:
			if (pPopup && pPopup->mp_DpiAware && pPopup->mp_DpiAware->ProcessDpiMessages(hWnd2, messg, wParam, lParam))
			{
				return TRUE;
			}
	}

	return 0;
}
