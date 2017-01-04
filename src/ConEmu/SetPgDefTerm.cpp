
/*
Copyright (c) 2016-2017 Maximus5
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

#include "ConEmu.h"
#include "DefaultTerm.h"
#include "SetPgDefTerm.h"
#include "TrayIcon.h"

CSetPgDefTerm::CSetPgDefTerm()
{
}

CSetPgDefTerm::~CSetPgDefTerm()
{
}

LRESULT CSetPgDefTerm::OnInitDialog(HWND hDlg, bool abInitial)
{
	// Default terminal apps
	CheckDlgButton(hDlg, cbDefaultTerminal, gpSet->isSetDefaultTerminal);
	bool bLeaveInTSA = gpSet->isRegisterOnOsStartupTSA;
	bool bRegister = gpSet->isRegisterOnOsStartup || gpConEmu->mp_DefTrm->IsRegisteredOsStartup(NULL, &bLeaveInTSA);
	CheckDlgButton(hDlg, cbDefaultTerminalStartup, bRegister);
	CheckDlgButton(hDlg, cbDefaultTerminalTSA, bLeaveInTSA);
	EnableWindow(GetDlgItem(hDlg, cbDefaultTerminalTSA), bRegister);
	CheckDlgButton(hDlg, cbDefTermAgressive, gpSet->isRegisterAgressive);
	CheckDlgButton(hDlg, cbDefaultTerminalNoInjects, gpSet->isDefaultTerminalNoInjects);
	CheckDlgButton(hDlg, cbDefaultTerminalUseExisting, !gpSet->isDefaultTerminalNewWindow);
	CheckDlgButton(hDlg, cbDefaultTerminalDebugLog, gpSet->isDefaultTerminalDebugLog);
	CheckRadioButton(hDlg, rbDefaultTerminalConfAuto, rbDefaultTerminalConfNever, rbDefaultTerminalConfAuto+gpSet->nDefaultTerminalConfirmClose);
	wchar_t* pszApps = gpSet->GetDefaultTerminalApps();
	_ASSERTE(pszApps!=NULL);
	SetDlgItemText(hDlg, tDefaultTerminal, pszApps);
	if (wcschr(pszApps, L',') != NULL && wcschr(pszApps, L'|') == NULL)
		Icon.ShowTrayIcon(L"List of hooked executables must be delimited with \"|\" but not commas", tsa_Default_Term);
	SafeFree(pszApps);

	return 0;
}

LRESULT CSetPgDefTerm::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case tDefaultTerminal:
	{
		wchar_t* pszApps = GetDlgItemTextPtr(hDlg, tDefaultTerminal);
		if (!pszApps || !*pszApps)
		{
			SafeFree(pszApps);
			pszApps = lstrdup(DEFAULT_TERMINAL_APPS/*L"explorer.exe"*/);
			SetDlgItemText(hDlg, tDefaultTerminal, pszApps);
		}
		gpSet->SetDefaultTerminalApps(pszApps);
		SafeFree(pszApps);
	}
	break;

	default:
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}
