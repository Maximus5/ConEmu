
/*
Copyright (c) 2013-2014 Maximus5
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

class CTabPanelWin : public CTabPanelBase
{
private:
	HWND mh_Tabbar, mh_Toolbar, mh_Rebar;
	bool mb_ChangeAllowed;
	int  mn_LastToolbarWidth;
	int  mn_ThemeHeightDiff;
	int  mn_TabHeight;

public:
	CTabPanelWin(CTabBarClass* ap_Owner);
	virtual ~CTabPanelWin();

public:
	virtual void AddTabInt(LPCWSTR text, int i, bool bAdmin);
	virtual void CreateRebar();
	virtual void DeleteItemInt(int I);
	virtual bool IsCreated();
	virtual bool IsTabbarCreated();
	virtual bool IsTabbarNotify(LPNMHDR nmhdr);
	virtual bool IsToolbarCreated();
	virtual bool IsToolbarCommand(WPARAM wParam, LPARAM lParam);
	virtual bool IsToolbarNotify(LPNMHDR nmhdr);
	virtual int  GetCurSelInt();
	virtual int  GetItemCountInt();
	virtual bool GetTabBarClientRect(RECT* rcTab);
	virtual int  GetTabFromPoint(POINT ptCur, bool bScreen = true);
	virtual bool GetTabRect(int nTabIdx, RECT* rcTab); // Screen-coordinates!
	virtual bool GetTabText(int nTabIdx, wchar_t* pszText, int cchTextMax);
	virtual bool GetToolBtnChecked(ToolbarCommandIdx iCmd);
	virtual bool GetToolBtnRect(int nCmd, RECT* rcBtnRect);
	virtual bool GetRebarClientRect(RECT* rc);
	virtual void InvalidateBar();
	virtual void OnCaptionHiddenChanged(bool bCaptionHidden);
	virtual void OnConsoleActivatedInt(int nConNumber);
	virtual bool OnNotifyInt(LPNMHDR nmhdr, LRESULT& lResult);
	virtual void OnWindowStateChanged(ConEmuWindowMode newMode);
	virtual int  QueryTabbarHeight();
	virtual void RePaintInt();
	virtual void RepositionInt();
	virtual int  SelectTabInt(int i);
	virtual void SetTabbarFont(HFONT hFont);
	virtual void SetToolBtnChecked(ToolbarCommandIdx iCmd, bool bChecked);
	virtual void ShowBar(bool bShow);
	virtual void ShowToolbar(bool bShow);

private:
	static LRESULT CALLBACK _TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC defaultProc);
	//static WNDPROC _defaultTabProc;
	static LRESULT CALLBACK _ToolProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT ToolProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC defaultProc);
	//static WNDPROC _defaultToolProc;
	static LRESULT CALLBACK _ReBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT ReBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC defaultProc);
	//static WNDPROC _defaultReBarProc;

private:
	HWND CreateTabbar();
	HWND CreateToolbar();
	void UpdateToolbarPos();
	LRESULT TabHitTest(bool abForce = false);
};
