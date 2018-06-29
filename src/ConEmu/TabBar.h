
/*
Copyright (c) 2009-present Maximus5
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

#if !defined(CONEMU_TABBAR_EX)

#include <commctrl.h>
#include "../common/MArray.h"
#include "../common/WObjects.h"

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
#include "TabID.h"

class CTabPanelBase;
class CVConGuard;

class CTabBarClass
{
	private:
		//// VCon, number in FAR
		//typedef struct tag_FAR_WND_ID
		//{
		//	CVirtualConsole* pVCon;
		//	int nFarWindowId;
		//	bool operator==(struct tag_FAR_WND_ID c)
		//	{
		//		return (this->pVCon==c.pVCon) && (this->nFarWindowId==c.nFarWindowId);
		//	};
		//	bool operator!=(struct tag_FAR_WND_ID c)
		//	{
		//		return (this->pVCon!=c.pVCon) || (this->nFarWindowId!=c.nFarWindowId);
		//	};
		//} VConTabs;

	public:
		HIMAGELIST GetTabIcons(int nTabItemHeight);
		int GetTabIcon(bool bAdmin);
		HICON GetTabIconByIndex(int IconIndex);

	private:
		CTabPanelBase* mp_Rebar = nullptr;
		CIconList      m_TabIcons;

		bool _active = false, _visible = false;
		std::atomic_int inActivate{0};
		int mn_CurSelTab = 0;
		bool mb_ForceRecalcHeight = false;
		ConEmuTab m_Tab4Tip = {};
		WCHAR  ms_TmpTabText[MAX_PATH] = {};
		bool mb_InKeySwitching = false;

		int  mn_InUpdate = 0;
		UINT mn_PostUpdateTick = 0;
		bool mb_PostUpdateCalled = false;
		bool mb_PostUpdateRequested = false;
		bool mb_DisableRedraw = false;

	protected:
		friend class CTabPanelBase;

		// Tab stack
		CTabStack m_Tabs; // Opened tabs
		MArray<CTabID*> m_TabStack; // Tabs history (for switching in Recent mode)
		CTabID* mp_DummyTab = nullptr; // Just to show something if there are no tabs at all

	private:

		enum UpdateAddTabFlags {
			uat_AnyTab             = 0x000,
			uat_PanelsOrModalsOnly = 0x001,
			uat_NonModals          = 0x002,
			uat_NonPanels          = 0x004,
			uat_PanelsOnly         = 0x008,
		};

		void SelectTab(int i);
		int  PrepareTab(CTab& pTab, CVirtualConsole *apVCon);
		bool CanActivateTab(int nTabIdx);
		int  GetNextTab(bool abForward, bool abAltStyle=false);
		int  GetNextTabHelper(int idxFrom, bool abForward, bool abRecent);
		int  GetItemCount();
		void DeleteItem(int I);
		int  UpdateAddTab(HANDLE hUpdate, int& tabIdx, int& nCurTab, bool& bStackChanged, CVirtualConsole* pVCon, DWORD nFlags/*UpdateAddTabFlags*/);

	public:
		// Tabs updating (populating)
		void Update(BOOL abPosted=FALSE);
		bool NeedPostUpdate();
		void PrintRecentStack();
		void ShowTabError(LPCTSTR asInfo, int tabIndex);

	private:

		void RequestPostUpdate();
		int  CountActiveTabs(int nMax = 0);

	protected:
		bool CheckStack(); // Убьет из стека отсутствующих
		bool AddStack(CTab& tab); // Убьет из стека отсутствующих и поместит tab на верх стека

		int  GetFirstLastVConTab(CVirtualConsole* pVCon, bool bFirst, int nFromTab = -1);

	#ifdef _DEBUG
	public:
		bool IsTabsCreated() const;
	#endif

	public:
		CTabBarClass();
		virtual ~CTabBarClass();

		void Activate(BOOL abPreSyncConsole=FALSE);
		HWND ActivateSearchPane(bool bActivate);
		int  ActiveTabByName(int anType, LPCWSTR asName, CVConGuard* rpVCon);
		int  ActivateTabByPoint(LPPOINT pptCur, bool bScreen = true, bool bOverTabHitTest = true);
		void CheckRebarCreated();
		int  CreateTabIcon(LPCWSTR asIconDescr, bool bAdmin, LPCWSTR asWorkDir);
		void Deactivate(BOOL abPreSyncConsole=FALSE);
		bool GetActiveTabRect(RECT* rcTab);
		int  GetCurSel();
		//RECT GetMargins(bool bIgnoreVisibility = false);
		bool GetRebarClientRect(RECT* rc);
		//int  GetTabbarHeight();
		int  GetTabFromPoint(LPPOINT pptCur, bool bScreen = true, bool bOverTabHitTest = true);
		bool GetVConFromTab(int nTabIdx, CVConGuard* rpVCon, DWORD* rpWndIndex);
		void HighlightTab(const CTabID* pTab, bool bHighlight);
		void Invalidate();
		bool IsTabsActive() const;
		//bool IsTabsShown();
		bool IsSearchShown(bool bFilled);
		void OnAlternative(BOOL abAlternative);
		void OnBufferHeight(BOOL abBufferHeight);
		void OnCaptionHidden();
		void OnChooseTabPopup();
		void OnCommand(WPARAM wParam, LPARAM lParam);
		void OnConsoleActivated(int nConNumber); //0-based
		bool OnKeyboard(UINT messg, WPARAM wParam, LPARAM lParam);
		bool OnNotify(LPNMHDR nmhdr, LRESULT& lResult);
		void OnShowButtonsChanged();
		bool OnTimer(WPARAM wParam);
		void OnWindowStateChanged();
		void Recreate();
		void RePaint();
		void Reposition();
		void Reset();
		void Retrieve();
		void SetRedraw(bool abEnableRedraw);
		bool ShowSearchPane(bool bShow, bool bCtrlOnly = false);
		void UpdatePosition();
		void UpdateToolConsoles(bool abForcePos=false);

		// Переключение табов
		bool IsInSwitch();
		void Switch(bool abForward, bool abAltStyle=false);
		void SwitchCommit();
		void SwitchNext(bool abAltStyle=false);
		void SwitchPrev(bool abAltStyle=false);
		void SwitchRollback();

		// Из Samples\Tabs
		bool ProcessTabKeyboardEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult) { return false; };
		bool Toolbar_GetBtnRect(int nCmd, RECT* rcBtnRect);
		int  GetHoverTab() { return -1; };
		int  TabBtnFromCursor(POINT point, DWORD *pnFlags = NULL) { return -1; };
		int  TabFromCursor(POINT point, DWORD *pnFlags = NULL) { return -1; };
		void HoverTab(int anTab) {};
		void PaintTabs(const PaintDC& dc, const RECT &rcCaption, const RECT &rcTabs) {};
		void Toolbar_UnHover() {};
};

#endif // #if !defined(CONEMU_TABBAR_EX)
