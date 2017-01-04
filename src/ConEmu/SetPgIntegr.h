
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


#pragma once

#include <windows.h>

#include "SetPgBase.h"

class CSetPgIntegr
	: public CSetPgBase
{
public:
	static CSetPgBase* Create() { return new CSetPgIntegr(); };
	static TabHwndIndex PageType() { return thi_Integr; };
	virtual TabHwndIndex GetPageType() override { return PageType(); };
public:
	CSetPgIntegr();
	virtual ~CSetPgIntegr();

public:
	enum ShellIntegrType
	{
		ShellIntgr_Inside = 0,
		ShellIntgr_Here,
	};
	void ShellIntegration(HWND hDlg, ShellIntegrType iMode, bool bEnabled, bool bForced = false);

protected:
	bool ReloadHereList(int* pnHere = NULL, int* pnInside = NULL);
	void FillHereValues(WORD CB);

public:
	// Methods
	virtual LRESULT OnInitDialog(HWND hDlg, bool abInitial) override;

	virtual INT_PTR PageDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam) override;

public:
	// Helpers
	static void RegisterShell(LPCWSTR asName, LPCWSTR asMode, LPCWSTR asConfig, LPCWSTR asCmd, LPCWSTR asIcon);
	static void UnregisterShell(LPCWSTR asName);
	static void UnregisterShellInvalids();
};
