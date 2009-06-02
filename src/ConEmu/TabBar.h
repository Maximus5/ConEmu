
#pragma once

#include <vector>

#define CONEMUMSG_UPDATETABS _T("ConEmuMain::UpdateTabs")

class TabBarClass
{
private:
	bool _active;
	int _tabHeight;
	RECT m_Margins;
	bool _titleShouldChange;
	int _prevTab;
	BOOL mb_ChangeAllowed, mb_Enabled;
	void AddTab(LPCWSTR text, int i);
	void SelectTab(int i);
	CVirtualConsole* FarSendChangeTab(int tabIndex);
	HWND mh_Tabbar, mh_ConmanToolbar, mh_Rebar;
	LONG mn_LastToolbarWidth;
	void UpdateToolbarPos();
	void PrepareTab(ConEmuTab* pTab);
	BOOL GetVConFromTab(int nTabIdx, CVirtualConsole** rpVCon, DWORD* rpWndIndex);

protected:
	static LRESULT CALLBACK TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	// Пока - банально. VCon, номер в FAR
	typedef struct {
		CVirtualConsole* pVCon;
		int nFarWindowId;
	} VConTabs;
	std::vector<VConTabs> m_Tab2VCon;
	BOOL mb_PostUpdateCalled;
	UINT mn_MsgUpdateTabs;

public:
	TabBarClass();
	void Enable(BOOL abEnabled);
	void Refresh(BOOL abFarActive);
	void Retrieve();
	void Reset();
	void Invalidate();
	bool IsActive();
	bool IsShown();
	BOOL IsAllowed();
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
	void OnConman(int nConNumber, BOOL bAlternative);
	bool OnNotify(LPNMHDR nmhdr);
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void OnMouse(int message, int x, int y);
};
