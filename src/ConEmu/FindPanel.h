
/*
Copyright (c) 2014-2017 Maximus5
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

class CConEmuMain;
struct CEStr;

class CFindPanel
{
protected:
	CConEmuMain* mp_ConEmu;
	HWND mh_Pane;
	HWND mh_Edit;
	HFONT mh_Font;
	static ATOM mh_Class;
	WNDPROC mfn_EditProc;
	UINT mn_KeyDown;
	int mn_RebarHeight;
	CEStr* ms_PrevSearch;

public:
	CFindPanel(CConEmuMain* apConEmu);
	~CFindPanel();

	HWND CreatePane(HWND hParent, int nBarHeight);
	bool OnCreateFinished();
	HWND GetHWND();
	int  GetMinWidth();
	HWND Activate(bool bActivate);
	bool IsAvailable(bool bFilled);

protected:
	bool RegisterPaneClass();
	static LRESULT WINAPI FindPaneProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT WINAPI EditCtrlProc(HWND hCtrl, UINT uMsg, WPARAM wParam, LPARAM lParam);
	bool OnCreate(CREATESTRUCT* ps);
	void OnDestroy();
	void OnWindowPosChanging(WINDOWPOS* p);
	void OnSize(LPRECT prcEdit);
	void OnCreateFont();
	void OnSearch();
	bool OnKeyboard(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lRc);
	void StopSearch();
	void ShowMenu();
	void ResetSearch();
};
