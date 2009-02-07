#pragma once

class TabBarClass
{
	bool _active;
	HWND _hwndTab;
	int _tabHeight;
	TCHAR _lastTitle[MAX_PATH];
	bool _titleShouldChange;
	int _prevTab;
	void AddTab(wchar_t* text, int i);
	void SelectTab(int i);
	char FarTabShortcut(int tabIndex);
	void FarSendChangeTab(int tabIndex);
public:
	TabBarClass();
	bool IsActive();
	BOOL IsAllowed();
	int Height();
	void Activate();
	void Deactivate();
	void Update(ConEmuTab* tabs, int tabsCount);
	void UpdatePosition();
	bool OnNotify(LPNMHDR nmhdr);
	void OnMouse(int message, int x, int y);
	void OnTimer();
};

