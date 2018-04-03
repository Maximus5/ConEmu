
/*
Copyright (c) 2013-present Maximus5
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
#include "OptionsClass.h"
#include "OptionsFast.h"
#include "OptionsHelp.h"

//#define DEBUGSTRFONT(s) DEBUGSTR(s)

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

bool CEHelpPopup::OpenSettingsWiki(HWND hDlg, WORD nCtrlId)
{
	CEStr lsUrl;
	wchar_t szId[20];
	msprintf(szId, countof(szId), L"#id%i", nCtrlId);

	//if (nCtrlId == tCmdGroupCommands)
	//{
	//	// Some controls are processed personally
	//	lsUrl.Attach(lstrmerge(CEWIKIBASE, L"NewConsole.html", szId));
	//}
	//else
	if (hDlg == ghOpWnd)
	{
		lsUrl.Attach(lstrmerge(CEWIKIBASE, L"Settings.html", szId));
	}
	else if (hDlg == FastConfig::ghFastCfg)
	{
		lsUrl.Attach(lstrmerge(CEWIKIBASE, L"SettingsFast.html", szId));
	}
	else if (gpSetCls->GetActivePageWiki(lsUrl))
	{
		lstrmerge(&lsUrl.ms_Val, szId);
	}

	if (lsUrl.IsEmpty())
		return false;
	return CDlgItemHelper::OpenHyperlink(lsUrl, ghOpWnd);
}
