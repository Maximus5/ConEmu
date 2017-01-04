
/*
Copyright (c) 2009-2017 Maximus5
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

class CDpiForDialog;

class CRecreateDlg
{
protected:
	HWND   mh_Dlg;
	int    mn_DlgRc;
	RConStartArgs* mp_Args;
	HWND   mh_Parent;
	bool   mb_DontAutoSelCmd;
	// Defaults
	wchar_t* mpsz_DefCmd;
	wchar_t* mpsz_CurCmd;
	wchar_t* mpsz_SysCmd;
	wchar_t* mpsz_DefDir;
	// Work dirs
	CEStr ms_RConStartDir;
	CEStr ms_RConCurDir;
	// dpi support
	CDpiForDialog* mp_DpiAware;
	// Buffer
	wchar_t ms_CurUser[MAX_PATH*2+1];
protected:
	void InitVars();
	void FreeVars();
	void AddCommandList(LPCWSTR asCommand, INT_PTR iAfter = -1);
	void AddDirectoryList(LPCWSTR asDirectory, INT_PTR iAfter = -1);
protected:
	INT_PTR OnInitDialog(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnCtlColorStatic(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnFillCmdList(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnUserControls(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnSysCommand(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnButtonClicked(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnEditSetFocus(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnComboSetFocus(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
public:
	CRecreateDlg();
	~CRecreateDlg();

	int RecreateDlg(RConStartArgs* apArgs, bool abDontAutoSelCmd = false);
	HWND GetHWND();
	void Close();

	static INT_PTR CALLBACK RecreateDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
	static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
};
