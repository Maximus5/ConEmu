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
	//char FarTabShortcut(int tabIndex);
	void FarSendChangeTab(int tabIndex);
	static LRESULT CALLBACK ToolWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	HWND mh_ToolbarParent, mh_Toolbar;
	//typedef BOOL _cdecl GetConsolesTitles_t( void* titles, DWORD* number );
	//typedef BOOL _cdecl ActivateConsole_t( DWORD number );
	//GetConsolesTitles_t* GetConsolesTitles;
	//ActivateConsole_t* ActivateConsole;
	typedef struct RegShortcut
	{
	    TCHAR* name;
	    char*  def;
	    char*  value;
	    DWORD  action;
	} RegShortcut;
	typedef bool (_cdecl * ConMan_KeyAction_t)( RegShortcut* shortcut );
	ConMan_KeyAction_t ConMan_KeyAction;
	typedef struct FarTitle
	{
		DWORD num;
		TCHAR title[MAX_PATH];
		bool  active;
	} FarTitle;
	void UpdateToolbarPos();

protected:
	static LRESULT CALLBACK TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
	//int Height();
	RECT GetMargins();
	void Activate();
	void CreateToolbar();
	void Deactivate();
	void Update(ConEmuTab* tabs, int tabsCount);
	void UpdatePosition();
	void UpdateWidth();
	void OnConman(int nConNumber, BOOL bAlternative);
	bool OnNotify(LPNMHDR nmhdr);
	void OnMouse(int message, int x, int y);
	void OnTimer();
};

