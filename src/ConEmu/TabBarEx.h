
/*
Copyright (c) 2012 Maximus5
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

//#if defined(_DEBUG) && !defined(CONEMU_TABBAR_EX)
//#define CONEMU_TABBAR_EX
//#endif


#if defined(CONEMU_TABBAR_EX)

#define USE_CONEMU_TOOLBAR


#include "Header.h"
#include "../common/WinObjects.h"
#include "TabID.h"
#include "DwmHelper.h"
#include "ToolBarClass.h"

//#define CONEMUMSG_UPDATETABS _T("ConEmuMain::UpdateTabs")

#define HT_CONEMUTAB HTBORDER

//enum ToolbarCommandIdx
//{
//	TID_ACTIVE_NUMBER = 1,
//	TID_CREATE_CON,
//	TID_ALTERNATIVE,
//	TID_SCROLL,
//	TID_MINIMIZE,
//	TID_MAXIMIZE,
//	TID_APPCLOSE,
//	TID_COPYING,
//	TID_MINIMIZE_SEP = 110,
//};

class TabBarClass
{
public:
	TabBarClass();
	virtual ~TabBarClass();

protected:
	void InitToolbar();
public:
	//bool OnMenuSelected(HMENU hMenu, WORD nID, WORD nFlags);
	void Retrieve();
	void Reset();
	void Invalidate();
	bool IsTabsActive();
	bool IsTabsShown();
	RECT GetMargins();
	void Activate(BOOL abPreSyncConsole=FALSE);
	//HWND CreateToolbar();
	//HWND CreateTabbar(bool abDummyCreate = false);
	//HWND GetTabbar();
	int GetTabbarHeight();
	void CreateRebar();
	void Deactivate(BOOL abPreSyncConsole=FALSE);
	void RePaint();
	bool GetRebarClientRect(RECT* rc);
	void Update(BOOL abPosted=FALSE);
	BOOL NeedPostUpdate();
	void UpdatePosition();
	void UpdateTabFont();
	void Reposition();
	void OnConsoleActivated(int nConNumber); //0-based
	void UpdateToolConsoles(bool abForcePos=false);
	void OnCaptionHidden();
	void OnWindowStateChanged();
	void OnBufferHeight(BOOL abBufferHeight);
	void OnAlternative(BOOL abAlternative);
	//LRESULT OnNotify(LPNMHDR nmhdr);
	void OnChooseTabPopup();
	//void OnNewConPopupMenu(POINT* ptWhere = NULL, DWORD nFlags = 0);
	//void OnNewConPopupMenuRClick(HMENU hMenu, UINT nItemPos);
	void OnCommand(WPARAM wParam, LPARAM lParam);
	bool OnMouse(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult);
	void OnShowButtonsChanged();

	// Переключение табов
	void Switch(BOOL abForward, BOOL abAltStyle=FALSE);
	void SwitchNext(BOOL abAltStyle=FALSE);
	void SwitchPrev(BOOL abAltStyle=FALSE);
	BOOL IsInSwitch();
	void SwitchCommit();
	void SwitchRollback();
	BOOL OnKeyboard(UINT messg, WPARAM wParam, LPARAM lParam);
	void SetRedraw(BOOL abEnableRedraw);

	int  ActiveTabByName(int anType, LPCWSTR asName, CVirtualConsole** ppVCon);
	void GetActiveTabRect(RECT* rcTab);

	// Из Samples\Tabs
	bool ProcessNcTabMouseEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult);
	int GetCurSel();
	int GetHoverTab();
	int GetItemCount();
	bool CanSelectTab(int anNewTab, const TabInfo& ti);
	void SelectTab(int anTab);
	void HoverTab(int anTab);
	#if defined(USE_CONEMU_TOOLBAR)
	void Toolbar_UnHover();
	#endif
	bool ProcessTabKeyboardEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult);
	void PaintTabs(const PaintDC& dc, const RECT &rcCaption, const RECT &rcTabs);
	int TabFromCursor(POINT point, DWORD *pnFlags = NULL);
	int TabBtnFromCursor(POINT point, DWORD *pnFlags = NULL);
	void ShowTabError(LPCTSTR asInfo, int tabIndex);
	bool Toolbar_GetBtnRect(int nCmd, RECT* rcBtnRect);

public:
	CToolBarClass* mp_Toolbar;
	//int mn_ToolbarBmp;
	//SIZE m_ToolbarBmpSize;
	//int mn_ToolPaneConsole, mn_ToolPaneOptions;
	//int mn_ToolCmdActiveCon, mn_ToolCmdNewCon, mn_ToolCmdBuffer;
	static void OnToolbarCommand(LPARAM lParam, int anPaneID, int anCmd, bool abArrow, POINT ptWhere, RECT rcBtnRect);
	static void OnToolbarMenu(LPARAM lParam, int anPaneID, int anCmd, POINT ptWhere, RECT rcBtnRect);
	static void OnToolBarDraw(LPARAM lParam, const PaintDC& dc, const RECT& rc, int nPane, int nCmd, DWORD nFlags);

protected:
	RECT mrc_TabsClient; // координаты в клиентской(!) области ConEmu панели табов и тулбара
	RECT mrc_Tabs, mrc_Caption, mrc_Toolbar;

	bool mb_Active; // режим автоскрытия табов?
	bool mb_UpdateModified;
	int mn_ActiveTab; // активная вкладка (белый фон)
	int mn_HoverTab;  // пассивная вкладка под мышиным курсором (более светлая, чем просто пассивная)
	CTabStack m_Tabs, m_TabStack;
	
	bool mb_ToolbarFit;
	int mn_InUpdate;
	int GetMainIcon(const CTabID* pTab);
	int GetStateIcon(const CTabID* pTab);
protected:
	void RepositionTabs(const RECT &rcCaption, const RECT &rcTabs);
	virtual HFONT CreateTabFont(int anTabHeight);
	void PaintFlush();
	void RequestPostUpdate();
	int GetNextTab(BOOL abForward, BOOL abAltStyle=FALSE);
private:
	int mn_TabWidth, mn_EdgeWidth, mn_TextShift;
	SIZE m_TabFontSize;

	enum TabColors {
		clrText = 0, //RGB(0,0,0);
		clrDisabledText, //RGB(0,0,0);
		clrDisabledTextShadow, //RGB(0,0,0);
		clrBorder, //RGB(107,165,189);
		clrHoverBorder, //RGB(148,148,165);
		clrInactiveBorder, //RGB(148,148,165);

		clrEdgeLeft, //RGB(255,255,247);
		clrEdgeRight, //RGB(255,255,247);
		clrInactiveEdgeLeft, //RGB(239,239,247);
		clrInactiveEdgeRight, //RGB(140,173,206);

		clrBackActive, //RGB(255,255,247);
		clrBackActiveBottom, //RGB(214,231,247);
		clrBackHover, //RGB(239,239,247);
		clrBackHoverBottom, //RGB(132,206,247);
		clrBackInactive, //RGB(239,239,247);
		clrBackInactiveBottom, //RGB(156,181,214);
		clrBackDisabled, //RGB(243,243,243);
		clrBackDisabledBottom, //RGB(189,189,189);

		clrColorIndexMax = clrBackDisabledBottom
	};
	COLORREF m_Colors[30];
	HPEN     mh_Pens[30], mh_OldPen;
	HBRUSH   mh_Brushes[30], mh_OldBrush;
	HFONT    mh_Font, mh_OldFont;
	void PreparePalette();
	void CreateStockObjects(HDC hdc, const RECT &rcTabs);
	void DeleteStockObjects(HDC hdc);
	//BITMAPINFOHEADER bi;
	COLORREF *pPixels, nColorMagic;
	FrameDrawStyle m_TabDrawStyle;

	HANDLE/*HTHEME*/ mh_Theme;

	void PaintCaption_Plain(const PaintDC& dc, const RECT &rcCaption, const RECT &rcTabs);
	void PaintCaption_2k(const PaintDC& dc, const RECT &rcCaption, const RECT &rcTabs);
	void PaintCaption_XP(const PaintDC& dc, const RECT &rcCaption, const RECT &rcTabs);
	void PaintCaption_Aero(const PaintDC& dc, const RECT &rcCaption, const RECT &rcTabs);
	void PaintCaption_Win8(const PaintDC& dc, const RECT &rcCaption, const RECT &rcTabs);
	void PaintCaption_Icon(const PaintDC& dc, int X, int Y);
	void PaintTabs_Common(const PaintDC& dc, const RECT &rcCaption, const RECT &rcTabs);
	void PaintTab_Common(const PaintDC& dc, RECT rcTab, struct TabDrawInfo* pTab, UINT anFlags, BOOL bCurrent, BOOL bHover);
	void PaintTab_VS2008(const PaintDC& dc, RECT rcTab, struct TabDrawInfo* pTab, UINT anFlags, BOOL bCurrent, BOOL bHover);
	void PaintTab_Win8(const PaintDC& dc, RECT rcTab, struct TabDrawInfo* pTab, UINT anFlags, BOOL bCurrent, BOOL bHover);
	void PaintTab_Text(const PaintDC& dc, RECT rcText, struct TabDrawInfo* pTab, UINT anFlags, BOOL bCurrent, BOOL bHover);

	BOOL mb_PostUpdateCalled, mb_PostUpdateRequested;
	DWORD mn_PostUpdateTick;
	HWND mh_Balloon;
	TOOLINFO tiBalloon; wchar_t ms_TabErrText[512];
	HIMAGELIST mh_TabIcons;
	BOOL mb_InKeySwitching;
	
	bool TabGetItemRect(int tabIndex, LPRECT rcTab);

	// вызывается при реакции на действия пользователя! при SelectTab(anTab) - не вызывается
	bool OnTabSelected(int anNewTab);
};


#endif // #if defined(CONEMU_TABBAR_EX)
