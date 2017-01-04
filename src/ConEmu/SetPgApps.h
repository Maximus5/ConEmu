
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

class CDpiForDialog;
class CDynDialog;

class CSetPgApps
	: public CSetPgBase
{
public:
	static CSetPgBase* Create() { return new CSetPgApps(); };
	static TabHwndIndex PageType() { return thi_Apps; };
	virtual TabHwndIndex GetPageType() override { return PageType(); };
public:
	CSetPgApps();
	virtual ~CSetPgApps();

public:
	// Methods
	virtual LRESULT OnInitDialog(HWND hDlg, bool abInitial) override;
	virtual void ProcessDpiChange(const CDpiForDialog* apDpi) override;
	// Events
	virtual INT_PTR OnButtonClicked(HWND hDlg, HWND hBtn, WORD nCtrlId) override;
	virtual LRESULT OnEditChanged(HWND hDlg, WORD nCtrlId) override;
	virtual INT_PTR OnComboBox(HWND hDlg, WORD nCtrlId, WORD code) override;

	virtual INT_PTR PageDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam) override;
	static INT_PTR CALLBACK pageOpProc_AppsChild(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);

	void DoFillControls(const AppSettings* pApp);
	void DoEnableControls(WORD nGroupCtrlId);

protected:
	// Methods
	bool CreateChildDlg();
	void DoReloadApps();
	void DoAppAdd();
	void DoAppDel();
	void DoAppMove(bool bUpward);
	void OnAppSelectionChanged();
	// Helper
	INT_PTR SetListAppName(const AppSettings* pApp, int nAppIndex/*1-based*/, INT_PTR iBefore = -1/*0-based*/);
	void NotifyVCon();

protected:
	// Members
	bool mb_SkipEditChange;
	bool mb_SkipEditSet;
	CDynDialog*    mp_DlgDistinct2;
	CDpiForDialog* mp_DpiDistinct2;
	HWND mh_Child;
	bool mb_Redraw, mb_Refill;
};
