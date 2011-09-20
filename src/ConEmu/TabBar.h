
/*
Copyright (c) 2009-2011 Maximus5
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

#include <vector>
#include <commctrl.h>
#include "../common/WinObjects.h"

#define CONEMUMSG_UPDATETABS _T("ConEmuMain::UpdateTabs")

class TabBarClass : public CToolTip
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
		TOOLINFO tiBalloon; wchar_t ms_TabErrText[512];
		HIMAGELIST mh_TabIcons; int mn_AdminIcon;
		struct CmdHistory
		{
			int nCmd;
			LPCWSTR pszCmd;
			wchar_t szShort[32];
		} History[MAX_CMD_HISTORY+1]; // структура для меню выбора команды новой консоли
		bool mb_InNewConPopup;
		bool _active;
		int _tabHeight;
		int mn_ThemeHeightDiff;
		RECT m_Margins;
		bool _titleShouldChange;
		int _prevTab;
		BOOL mb_ChangeAllowed; //, mb_Enabled;
		void AddTab(LPCWSTR text, int i, bool bAdmin);
		void SelectTab(int i);
		CVirtualConsole* FarSendChangeTab(int tabIndex);
		LONG mn_LastToolbarWidth;
		void UpdateToolbarPos();
		void PrepareTab(ConEmuTab* pTab, CVirtualConsole *apVCon);
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
		static LRESULT TabHitTest();

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
		std::vector<VConTabs> m_Tab2VCon;
		BOOL mb_PostUpdateCalled, mb_PostUpdateRequested;
		DWORD mn_PostUpdateTick;
		void RequestPostUpdate();
		UINT mn_MsgUpdateTabs;
		int mn_CurSelTab;
		int GetIndexByTab(VConTabs tab);
		int mn_InUpdate;

		//BOOL mb_ThemingEnabled;

		// Tab stack
		std::vector<VConTabs> m_TabStack;
		void CheckStack(); // Убьет из стека отсутствующих
		void AddStack(VConTabs tab); // Убьет из стека отсутствующих и поместит tab на верх стека

		BOOL mb_DisableRedraw;

	public:
		TabBarClass();
		virtual ~TabBarClass();
		virtual bool OnMenuSelected(HMENU hMenu, WORD nID, WORD nFlags);
		//void Enable(BOOL abEnabled);
		//void Refresh(BOOL abFarActive);
		void Retrieve();
		void Reset();
		void Invalidate();
		bool IsActive();
		bool IsShown();
		//BOOL IsAllowed();
		RECT GetMargins();
		void Activate();
		HWND CreateToolbar();
		HWND CreateTabbar();
		HWND GetTabbar();
		int GetTabbarHeight();
		void CreateRebar();
		void Deactivate();
		void RePaint();
		//void Update(ConEmuTab* tabs, int tabsCount);
		void Update(BOOL abPosted=FALSE);
		BOOL NeedPostUpdate();
		void UpdatePosition();
		void UpdateWidth();
		void OnConsoleActivated(int nConNumber/*, BOOL bAlternative*/);
		void OnCaptionHidden();
		void OnWindowStateChanged();
		void OnBufferHeight(BOOL abBufferHeight);
		LRESULT OnNotify(LPNMHDR nmhdr);
		void OnNewConPopup();
		void OnCommand(WPARAM wParam, LPARAM lParam);
		void OnMouse(int message, int x, int y);
		// Переключение табов
		void Switch(BOOL abForward, BOOL abAltStyle=FALSE);
		void SwitchNext(BOOL abAltStyle=FALSE);
		void SwitchPrev(BOOL abAltStyle=FALSE);
		BOOL IsInSwitch();
		void SwitchCommit();
		void SwitchRollback();
		BOOL OnKeyboard(UINT messg, WPARAM wParam, LPARAM lParam);
		void SetRedraw(BOOL abEnableRedraw);
		void PaintHeader(HDC hdc, RECT rcPaint);
		int  ActiveTabByName(int anType, LPCWSTR asName, CVirtualConsole** ppVCon);
};
