
/*
Copyright (c) 2009-2012 Maximus5
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

#ifdef _DEBUG
#include "TabBarEx.h"
#endif

#if !defined(CONEMU_TABBAR_EX)

#include <commctrl.h>
#include "../common/MArray.h"
#include "../common/WinObjects.h"

#include "IconList.h"

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

#include "DwmHelper.h"


class TabBarClass
{
	private:
		// Пока - банально. VCon, номер в FAR
		typedef struct tag_FAR_WND_ID
		{
			CVirtualConsole* pVCon;
			int nFarWindowId;

			bool operator==(struct tag_FAR_WND_ID c)
			{
				return (this->pVCon==c.pVCon) && (this->nFarWindowId==c.nFarWindowId);
			};
			bool operator!=(struct tag_FAR_WND_ID c)
			{
				return (this->pVCon!=c.pVCon) || (this->nFarWindowId!=c.nFarWindowId);
			};
		} VConTabs;

	private:
		HWND mh_Tabbar, mh_Toolbar, mh_Rebar, mh_TabTip, mh_Balloon;
		HFONT mh_TabFont;
		TOOLINFO tiBalloon; wchar_t ms_TabErrText[512];
		//HIMAGELIST mh_TabIcons; int mn_AdminIcon;
		//int GetTabIcon(bool bAdmin);
		CIconList m_TabIcons;
		//struct CmdHistory
		//{
		//	int nCmd;
		//	LPCWSTR pszCmd;
		//	wchar_t szShort[32];
		//} m_CmdPopupMenu[MAX_CMD_HISTORY+1]; // структура для меню выбора команды новой консоли
		//bool mb_InNewConPopup, mb_InNewConRPopup;
		//int mn_FirstTaskID, mn_LastTaskID; // MenuItemID for Tasks, when mb_InNewConPopup==true
		bool _active, _visible;
		int _tabHeight;
		bool mb_ForceRecalcHeight;
		int mn_ThemeHeightDiff;
		//RECT m_Margins;
		bool _titleShouldChange;
		int _prevTab;
		BOOL mb_ChangeAllowed; //, mb_Enabled;
		void AddTab(LPCWSTR text, int i, bool bAdmin, int iTabIcon);
		void SelectTab(int i);
		CVirtualConsole* FarSendChangeTab(int tabIndex);
		LONG mn_LastToolbarWidth;
		void UpdateToolbarPos();
		int PrepareTab(ConEmuTab* pTab, CVirtualConsole *apVCon);
		BOOL GetVConFromTab(int nTabIdx, CVirtualConsole** rpVCon, DWORD* rpWndIndex);
		ConEmuTab m_Tab4Tip;
		WCHAR  ms_TmpTabText[MAX_PATH];
		LPCWSTR GetTabText(int nTabIdx);
		BOOL CanActivateTab(int nTabIdx);
		BOOL mb_InKeySwitching;
		int GetNextTab(BOOL abForward, BOOL abAltStyle=FALSE);
		int GetCurSel();
		int GetItemCount();
		void DeleteItem(int I);
		void AddTab2VCon(VConTabs& vct);
		void ShowTabError(LPCTSTR asInfo, int tabIndex = 0);
		//void CheckTheming();
		
	protected:
		static LRESULT CALLBACK TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static WNDPROC _defaultTabProc;
		static LRESULT CALLBACK ToolProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static WNDPROC _defaultToolProc;
		static LRESULT CALLBACK ReBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static WNDPROC _defaultReBarProc;
		static LRESULT TabHitTest(bool abForce = false, int* pnOverTabHit = NULL);

		//typedef union tag_FAR_WND_ID {
		//	struct {
		//		CVirtualConsole* pVCon;
		//		int nFarWindowId/*HighPart*/;
		//	};
		//	struct {
		//		CVirtualConsole* pVCon;
		//		int nFarWindowId/*HighPart*/;
		//	} u;
		//	ULONGLONG ID;
		//} VConTabs;
		MArray<VConTabs> m_Tab2VCon;
		BOOL mb_PostUpdateCalled, mb_PostUpdateRequested;
		DWORD mn_PostUpdateTick;
		void RequestPostUpdate();
		//UINT mn_MsgUpdateTabs;
		int mn_CurSelTab;
		int GetIndexByTab(VConTabs tab);
		int mn_InUpdate;

		//BOOL mb_ThemingEnabled;

		// Tab stack
		MArray<VConTabs> m_TabStack;
		void CheckStack(); // Убьет из стека отсутствующих
		void AddStack(VConTabs tab); // Убьет из стека отсутствующих и поместит tab на верх стека

		BOOL mb_DisableRedraw;

	public:
		TabBarClass();
		virtual ~TabBarClass();
		//virtual bool OnMenuSelected(HMENU hMenu, WORD nID, WORD nFlags);
		//void Enable(BOOL abEnabled);
		//void Refresh(BOOL abFarActive);
		void Retrieve();
		void Reset();
		void Invalidate();
		bool IsTabsActive();
		bool IsTabsShown();
		//BOOL IsAllowed();
		RECT GetMargins();
		void Activate(BOOL abPreSyncConsole=FALSE);
		HWND CreateToolbar();
		HWND CreateTabbar(bool abDummyCreate = false);
		HWND GetTabbar();
		int GetTabbarHeight();
		void CreateRebar();
		void Deactivate(BOOL abPreSyncConsole=FALSE);
		void RePaint();
		//void Update(ConEmuTab* tabs, int tabsCount);
		bool GetRebarClientRect(RECT* rc);
		void Update(BOOL abPosted=FALSE);
		int CreateTabIcon(LPCWSTR asIconDescr, bool bAdmin);
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
		LRESULT OnNotify(LPNMHDR nmhdr);
		void OnChooseTabPopup();
		//void OnNewConPopupMenu(POINT* ptWhere = NULL, DWORD nFlags = 0);
		//void OnNewConPopupMenuRClick(HMENU hMenu, UINT nItemPos);
		void OnCommand(WPARAM wParam, LPARAM lParam);
		void OnMouse(int message, int x, int y);
		LRESULT OnTimer(WPARAM wParam);
		// Переключение табов
		void Switch(BOOL abForward, BOOL abAltStyle=FALSE);
		void SwitchNext(BOOL abAltStyle=FALSE);
		void SwitchPrev(BOOL abAltStyle=FALSE);
		BOOL IsInSwitch();
		void SwitchCommit();
		void SwitchRollback();
		BOOL OnKeyboard(UINT messg, WPARAM wParam, LPARAM lParam);
		void SetRedraw(BOOL abEnableRedraw);
		//void PaintHeader(HDC hdc, RECT rcPaint);
		int  ActiveTabByName(int anType, LPCWSTR asName, CVirtualConsole** ppVCon);
		void GetActiveTabRect(RECT* rcTab);
		void OnShowButtonsChanged();

		// Из Samples\Tabs
		bool ProcessNcTabMouseEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult) { return false; };
		int GetHoverTab() { return -1; };
		void HoverTab(int anTab) {};
		void Toolbar_UnHover() {};
		bool ProcessTabKeyboardEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult) { return false; };
		void PaintTabs(const PaintDC& dc, const RECT &rcCaption, const RECT &rcTabs) {};
		int TabFromCursor(POINT point, DWORD *pnFlags = NULL) { return -1; };
		int TabBtnFromCursor(POINT point, DWORD *pnFlags = NULL) { return -1; };
		bool Toolbar_GetBtnRect(int nCmd, RECT* rcBtnRect);
};

#endif // #if !defined(CONEMU_TABBAR_EX)
