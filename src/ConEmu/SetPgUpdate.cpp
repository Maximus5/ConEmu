
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

#include "SetPgUpdate.h"

CSetPgUpdate::CSetPgUpdate()
{
}

CSetPgUpdate::~CSetPgUpdate()
{
}

LRESULT CSetPgUpdate::OnInitDialog(HWND hDlg, bool abInitial)
{
	ConEmuUpdateSettings* p = &gpSet->UpdSet;

	SetDlgItemText(hDlg, tUpdateVerLocation, p->UpdateVerLocation());

	checkDlgButton(hDlg, cbUpdateCheckOnStartup, p->isUpdateCheckOnStartup);
	checkDlgButton(hDlg, cbUpdateCheckHourly, p->isUpdateCheckHourly);
	checkDlgButton(hDlg, cbUpdateConfirmDownload, !p->isUpdateConfirmDownload);
	checkRadioButton(hDlg, rbUpdateStableOnly, rbUpdateLatestAvailable,
		(p->isUpdateUseBuilds==1) ? rbUpdateStableOnly : (p->isUpdateUseBuilds==3) ? rbUpdatePreview : rbUpdateLatestAvailable);

	checkDlgButton(hDlg, cbUpdateInetTool, p->isUpdateInetTool);
	SetDlgItemText(hDlg, tUpdateInetTool, p->GetUpdateInetToolCmd());

	checkDlgButton(hDlg, cbUpdateUseProxy, p->isUpdateUseProxy);
	SetDlgItemText(hDlg, tUpdateProxy, p->szUpdateProxy);
	SetDlgItemText(hDlg, tUpdateProxyUser, p->szUpdateProxyUser);
	SetDlgItemText(hDlg, tUpdateProxyPassword, p->szUpdateProxyPassword);

	OnButtonClicked(hDlg, NULL, cbUpdateInetTool); // Enable/Disable command field, button '...' and ‘Proxy’ fields

	int nPackage = p->UpdateDownloadSetup(); // 1-exe, 2-7zip
	checkRadioButton(hDlg, rbUpdateUseExe, rbUpdateUseArc, (nPackage==1) ? rbUpdateUseExe : rbUpdateUseArc);
	wchar_t szCPU[4] = L"";
	SetDlgItemText(hDlg, tUpdateExeCmdLine, p->UpdateExeCmdLine(szCPU));
	SetDlgItemText(hDlg, tUpdateArcCmdLine, p->UpdateArcCmdLine());
	SetDlgItemText(hDlg, tUpdatePostUpdateCmd, p->szUpdatePostUpdateCmd);
	enableDlgItem(hDlg, (nPackage==1) ? tUpdateArcCmdLine : tUpdateExeCmdLine, FALSE);
	// Show used or preferred installer bitness
	CEStr szFormat, szTitle; INT_PTR iLen;
	if ((iLen = GetString(hDlg, rbUpdateUseExe, &szFormat.ms_Val)) > 0)
	{
		if (wcsstr(szFormat.ms_Val, L"%s") != NULL)
		{
			wchar_t* psz = szTitle.GetBuffer(iLen+4);
			if (psz)
			{
				_wsprintf(psz, SKIPLEN(iLen+4) szFormat.ms_Val, (nPackage == 1) ? szCPU : WIN3264TEST(L"x86",L"x64"));
				SetDlgItemText(hDlg, rbUpdateUseExe, szTitle);
			}
		}
	}

	checkDlgButton(hDlg, cbUpdateLeavePackages, p->isUpdateLeavePackages);
	SetDlgItemText(hDlg, tUpdateDownloadPath, p->szUpdateDownloadPath);

	return 0;
}

LRESULT CSetPgUpdate::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case tUpdateInetTool:
		if (gpSet->UpdSet.isUpdateInetTool)
			GetString(hDlg, nCtrlId, &gpSet->UpdSet.szUpdateInetTool);
		break;
	case tUpdateProxy:
		GetString(hDlg, nCtrlId, &gpSet->UpdSet.szUpdateProxy);
		break;
	case tUpdateProxyUser:
		GetString(hDlg, nCtrlId, &gpSet->UpdSet.szUpdateProxyUser);
		break;
	case tUpdateProxyPassword:
		GetString(hDlg, nCtrlId, &gpSet->UpdSet.szUpdateProxyPassword);
		break;
	case tUpdateDownloadPath:
		GetString(hDlg, nCtrlId, &gpSet->UpdSet.szUpdateDownloadPath);
		break;
	case tUpdateExeCmdLine:
		GetString(hDlg, nCtrlId, &gpSet->UpdSet.szUpdateExeCmdLine, gpSet->UpdSet.szUpdateExeCmdLineDef);
		break;
	case tUpdateArcCmdLine:
		GetString(hDlg, nCtrlId, &gpSet->UpdSet.szUpdateArcCmdLine, gpSet->UpdSet.szUpdateArcCmdLineDef);
		break;
	case tUpdatePostUpdateCmd:
		GetString(hDlg, nCtrlId, &gpSet->UpdSet.szUpdatePostUpdateCmd);
		break;

	default:
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}
