
/*
Copyright (c) 2016 Maximus5
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

#include "OptionsClass.h"
#include "SetPgComspec.h"
#include "SetPgIntegr.h"

CSetPgComspec::CSetPgComspec()
{
}

CSetPgComspec::~CSetPgComspec()
{
}

LRESULT CSetPgComspec::OnInitDialog(HWND hDlg, bool abInitial)
{
	_ASSERTE((rbComspecAuto+cst_Explicit)==rbComspecExplicit && (rbComspecAuto+cst_Cmd)==rbComspecCmd  && (rbComspecAuto+cst_EnvVar)==rbComspecEnvVar);
	checkRadioButton(hDlg, rbComspecAuto, rbComspecExplicit, rbComspecAuto+gpSet->ComSpec.csType);

	SetDlgItemText(hDlg, tComspecExplicit, gpSet->ComSpec.ComspecExplicit);
	SendDlgItemMessage(hDlg, tComspecExplicit, EM_SETLIMITTEXT, countof(gpSet->ComSpec.ComspecExplicit)-1, 0);

	_ASSERTE((rbComspec_OSbit+csb_SameApp)==rbComspec_AppBit && (rbComspec_OSbit+csb_x32)==rbComspec_x32);
	checkRadioButton(hDlg, rbComspec_OSbit, rbComspec_x32, rbComspec_OSbit+gpSet->ComSpec.csBits);

	checkDlgButton(hDlg, cbComspecUpdateEnv, gpSet->ComSpec.isUpdateEnv ? BST_CHECKED : BST_UNCHECKED);
	enableDlgItem(hDlg, cbComspecUpdateEnv, (gpSet->ComSpec.csType!=cst_EnvVar));

	checkDlgButton(hDlg, cbComspecUncPaths, gpSet->ComSpec.isAllowUncPaths ? BST_CHECKED : BST_UNCHECKED);

	// Cmd.exe output cp
	if (abInitial)
	{
		SendDlgItemMessage(hDlg, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Undefined");
		SendDlgItemMessage(hDlg, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Automatic");
		SendDlgItemMessage(hDlg, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Unicode (/U)");
		SendDlgItemMessage(hDlg, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"OEM (/A)");
	}
	SendDlgItemMessage(hDlg, lbCmdOutputCP, CB_SETCURSEL, gpSet->nCmdOutputCP, 0);

	// Autorun (autoattach) with "cmd.exe" or "tcc.exe"
	CSetPgIntegr* pIntgrPg = NULL;
	if (gpSetCls->GetPageObj(pIntgrPg))
		pIntgrPg->PageDlgProc(hDlg, UM_RELOAD_AUTORUN, UM_RELOAD_AUTORUN, 0);

	return 0;
}
