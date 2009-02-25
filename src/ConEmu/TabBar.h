#pragma once

class TabBarClass
{
private:
	bool _active;
	HWND _hwndTab;
	int _tabHeight;
	//TCHAR _lastTitle[MAX_PATH];
	RECT m_Margins;
	bool _titleShouldChange;
	int _prevTab;
	BOOL mb_ChangeAllowed, mb_Enabled;
	void AddTab(wchar_t* text, int i);
	void SelectTab(int i);
	char FarTabShortcut(int tabIndex);
	void FarSendChangeTab(int tabIndex);
public:
	TabBarClass();
	void Enable(BOOL abEnabled);
	void Refresh(BOOL abFarActive);
	void Reset();
	void Invalidate();
	bool IsActive();
	bool IsShown();
	BOOL IsAllowed();
	//int Height();
	RECT GetMargins();
	void Activate();
	void Deactivate();
	void Update(ConEmuTab* tabs, int tabsCount);
	void UpdatePosition();
	void UpdateWidth();
	bool OnNotify(LPNMHDR nmhdr);
	void OnMouse(int message, int x, int y);
	void OnTimer();
};

