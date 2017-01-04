
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
#include "OptionsClass.h"
#include "SetPgFeatures.h"

CSetPgFeatures::CSetPgFeatures()
{
}

CSetPgFeatures::~CSetPgFeatures()
{
}

LRESULT CSetPgFeatures::OnInitDialog(HWND hDlg, bool abInitial)
{
	checkDlgButton(hDlg, cbAutoRegFonts, gpSet->isAutoRegisterFonts);

	checkDlgButton(hDlg, cbDebugSteps, gpSet->isDebugSteps);

	checkDlgButton(hDlg, cbDebugLog, gpSet->isLogging());
	UpdateLogLocation();

	checkDlgButton(hDlg, cbMonitorConsoleLang, gpSet->isMonitorConsoleLang ? BST_CHECKED : BST_UNCHECKED);

	checkDlgButton(hDlg, cbSleepInBackground, gpSet->isSleepInBackground);

	checkDlgButton(hDlg, cbRetardInactivePanes, gpSet->isRetardInactivePanes);

	checkDlgButton(hDlg, cbVisible, gpSet->isConVisible);

	checkDlgButton(hDlg, cbUseInjects, gpSet->isUseInjects);

	checkDlgButton(hDlg, cbProcessAnsi, gpSet->isProcessAnsi);

	checkDlgButton(hDlg, cbAnsiLog, gpSet->isAnsiLog);
	SetDlgItemText(hDlg, tAnsiLogPath, gpSet->pszAnsiLog ? gpSet->pszAnsiLog : L"");

	checkDlgButton(hDlg, cbKillSshAgent, gpSet->isKillSshAgent);
	checkDlgButton(hDlg, cbProcessNewConArg, gpSet->isProcessNewConArg);
	checkDlgButton(hDlg, cbProcessCmdStart, gpSet->isProcessCmdStart);
	checkDlgButton(hDlg, cbProcessCtrlZ, gpSet->isProcessCtrlZ);

	checkDlgButton(hDlg, cbSkipFocusEvents, gpSet->isSkipFocusEvents);

	checkDlgButton(hDlg, cbSuppressBells, gpSet->isSuppressBells);

	checkDlgButton(hDlg, cbConsoleExceptionHandler, gpSet->isConsoleExceptionHandler);

	checkDlgButton(hDlg, cbUseClink, gpSet->isUseClink() ? BST_CHECKED : BST_UNCHECKED);

	checkDlgButton(hDlg, cbDosBox, gpConEmu->mb_DosBoxExists);
	// изменение пока запрещено
	// или чтобы ругнуться, если DosBox не установлен
	EnableWindow(GetDlgItem(hDlg, cbDosBox), !gpConEmu->mb_DosBoxExists);
	//EnableWindow(GetDlgItem(hDlg, bDosBoxSettings), FALSE); // изменение пока запрещено
	ShowWindow(GetDlgItem(hDlg, bDosBoxSettings), SW_HIDE);

	checkDlgButton(hDlg, cbDisableAllFlashing, gpSet->isDisableAllFlashing);

	return 0;
}

void CSetPgFeatures::UpdateLogLocation()
{
	// Cut log file to directory only
	CEStr lsLogPath(gpSet->GetLogFileName());
	LPCWSTR pszName = lsLogPath.IsEmpty() ? NULL : PointToName(lsLogPath.ms_Val);
	if (pszName)
		*(wchar_t*)pszName = 0;
	SetDlgItemText(gpSetCls->GetPage(thi_Features), tDebugLogDir, lsLogPath);
}

LRESULT CSetPgFeatures::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case tAnsiLogPath:
	{
		SafeFree(gpSet->pszAnsiLog);
		gpSet->pszAnsiLog = GetDlgItemTextPtr(hDlg, tAnsiLogPath);
	}
	break;

	default:
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}
