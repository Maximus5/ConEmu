
/*
Copyright (c) 2013-2017 Maximus5
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

#include "IconList.h"
#include "WinObjBase.h"

class CTabPanelBase : public CWinObjBase
{
private:
	HFONT      mh_TabFont;

protected:
	CTabBarClass* mp_Owner;
	HWND       mh_TabTip, mh_Balloon;
	TOOLINFO   tiBalloon;
	wchar_t    ms_TabErrText[512];
	int        mn_prevTab;
	int        mn_LBtnDrag;
	HCURSOR    mh_DragCursor;
	POINT      mpt_DragStart;
	LONG       mn_InSelChange;
	DWORD      mn_LastChangeTick;

protected:
	//void InitIconList();
	void InitTooltips(HWND hParent);
	void DoTabDrag(UINT uMsg);
	void EndTabDrag();

public:
	CTabPanelBase(CTabBarClass* ap_Owner);
	virtual ~CTabPanelBase();

public:
	LRESULT OnMouseRebar(UINT uMsg, int x, int y);
	LRESULT OnMouseTabbar(UINT uMsg, int nTabIdx, int x, int y);
	LRESULT OnMouseToolbar(UINT uMsg, int nCmdId, int x, int y);
public:
	bool OnSetCursorRebar();

public:
	virtual HWND ActivateSearchPaneInt(bool bActivate) = 0;
	virtual void AddTabInt(LPCWSTR text, int i, CEFarWindowType Flags, int iTabIcon) = 0;
	virtual void CreateRebar() = 0;
	virtual void DestroyRebar() = 0;
	virtual void DeleteItemInt(int I) = 0;
	virtual bool IsCreated() = 0;
	virtual bool IsSearchShownInt(bool bFilled) = 0;
	virtual bool IsTabbarCreated() = 0;
	virtual bool IsTabbarNotify(LPNMHDR nmhdr) = 0;
	virtual bool IsToolbarCreated() = 0;
	virtual bool IsToolbarCommand(WPARAM wParam, LPARAM lParam) = 0;
	virtual bool IsToolbarNotify(LPNMHDR nmhdr) = 0;
	virtual int  GetCurSelInt() = 0;
	virtual int  GetItemCountInt() = 0;
	virtual bool GetTabBarClientRect(RECT* rcTab) = 0;
	virtual int  GetTabFromPoint(POINT ptCur, bool bScreen = true, bool bOverTabHitTest = true) = 0;
	virtual bool GetTabRect(int nTabIdx, RECT* rcTab) = 0; // Screen-coordinates!
	virtual bool GetTabText(int nTabIdx, wchar_t* pszText, int cchTextMax) = 0;
	virtual bool GetToolBtnChecked(ToolbarCommandIdx iCmd) = 0;
	virtual bool GetToolBtnRect(int nCmd, RECT* rcBtnRect) = 0;
	virtual bool GetRebarClientRect(RECT* rc) = 0;
	virtual void HighlightTab(int iTab, bool bHighlight) = 0;
	virtual void InvalidateBar() = 0;
	virtual void OnCaptionHiddenChanged(bool bCaptionHidden) = 0;
	virtual void OnConsoleActivatedInt(int nConNumber) = 0;
	virtual bool OnNotifyInt(LPNMHDR nmhdr, LRESULT& lResult) = 0;
	virtual void OnWindowStateChanged(ConEmuWindowMode newMode) = 0;
	virtual int  QueryTabbarHeight() = 0;
	virtual void RePaintInt() = 0;
	virtual void RepositionInt() = 0;
	virtual int  SelectTabInt(int i) = 0;
	virtual void SetTabbarFont(HFONT hFont) = 0;
	virtual void SetToolBtnChecked(ToolbarCommandIdx iCmd, bool bChecked) = 0;
	virtual void ShowBar(bool bShow) = 0;
	virtual bool ShowSearchPane(bool bShow, bool bCtrlOnly = false) = 0;
	virtual void ShowTabsPane(bool bShow) = 0;
	virtual void ShowToolsPane(bool bShow) = 0;

public:
	//int  GetTabIcon(bool bAdmin);
	bool FarSendChangeTab(int tabIndex, CVConGuard* rpVCon = NULL);
	virtual bool OnTimerInt(WPARAM wParam);
	void ShowTabErrorInt(LPCTSTR asInfo, int tabIndex);
	void UpdateTabFontInt();
};
