
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
#include "SetPgANSI.h"

CSetPgANSI::CSetPgANSI()
{
}

CSetPgANSI::~CSetPgANSI()
{
}

LRESULT CSetPgANSI::OnInitDialog(HWND hDlg, bool abInitial)
{
	checkDlgButton(hDlg, cbProcessAnsi, gpSet->isProcessAnsi);

	_ASSERTE(ansi_Allowed==0 && rbAnsiSecureCmd==(rbAnsiSecureAny+ansi_CmdOnly) && rbAnsiSecureOff==(rbAnsiSecureAny+ansi_Disabled));
	checkRadioButton(hDlg, rbAnsiSecureAny, rbAnsiSecureOff, rbAnsiSecureAny+gpSet->isAnsiExec);

	SetDlgItemText(hDlg, tAnsiSecure, gpSet->psAnsiAllowed ? gpSet->psAnsiAllowed : L"");

	return 0;
}

LRESULT CSetPgANSI::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case tAnsiSecure:
	{
		size_t cchMax = gpSet->psAnsiAllowed ? (_tcslen(gpSet->psAnsiAllowed)+1) : 0;
		MyGetDlgItemText(hDlg, tAnsiSecure, cchMax, gpSet->psAnsiAllowed);
	}
	break;

	default:
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}
