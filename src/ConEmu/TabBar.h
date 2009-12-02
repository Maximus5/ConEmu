
#pragma once

#include <vector>
#include <commctrl.h>

#define CONEMUMSG_UPDATETABS _T("ConEmuMain::UpdateTabs")

class TabBarClass
{
private:
	// Пока - банально. VCon, номер в FAR
	typedef struct tag_FAR_WND_ID {
		CVirtualConsole* pVCon;
		int nFarWindowId;

		bool operator==(struct tag_FAR_WND_ID c) {
			return (this->pVCon==c.pVCon) && (this->nFarWindowId==c.nFarWindowId);
		};
		bool operator!=(struct tag_FAR_WND_ID c) {
			return (this->pVCon!=c.pVCon) || (this->nFarWindowId!=c.nFarWindowId);
		};
	} VConTabs;

private:
	HWND mh_Tabbar, mh_Toolbar, mh_Rebar, mh_TabTip;
	HIMAGELIST mh_TabIcons;
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
	void PrepareTab(ConEmuTab* pTab);
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

protected:
	static LRESULT CALLBACK TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static WNDPROC _defaultTabProc;
	static LRESULT CALLBACK BarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static WNDPROC _defaultBarProc;
	static LRESULT CALLBACK ReBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static WNDPROC _defaultReBarProc;
	
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
	void RequestPostUpdate();
	UINT mn_MsgUpdateTabs;
	int mn_CurSelTab;
	int GetIndexByTab(VConTabs tab);
	int mn_InUpdate;
	
	// Tab stack
	std::vector<VConTabs> m_TabStack;
	void CheckStack(); // Убьет из стека отсутствующих
	void AddStack(VConTabs tab); // Убьет из стека отсутствующих и поместит tab на верх стека

public:
	TabBarClass();
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
	void CreateRebar();
	void Deactivate();
	void RePaint();
	//void Update(ConEmuTab* tabs, int tabsCount);
	void Update(BOOL abPosted=FALSE);
	void UpdatePosition();
	void UpdateWidth();
	void OnConsoleActivated(int nConNumber/*, BOOL bAlternative*/);
	void OnBufferHeight(BOOL abBufferHeight);
	bool OnNotify(LPNMHDR nmhdr);
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
};
