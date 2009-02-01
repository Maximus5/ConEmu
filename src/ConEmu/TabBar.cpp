#include <windows.h>
#include <commctrl.h>
#include "header.h"

TabBarClass TabBar;
const int TAB_FONT_HEIGTH = 16;
wchar_t TAB_FONT_FACE[] = L"Tahoma";
WNDPROC _defaultTabProc = NULL;

void TabBarClass::AddTab(wchar_t* text, int i)
{
	TCITEM tie;
	tie.mask = TCIF_TEXT;
	tie.iImage = -1; 
	tie.pszText = text ;

	TabCtrl_InsertItem(_hwndTab, i, &tie);
}

void TabBarClass::SelectTab(int i)
{
	TabCtrl_SetCurSel(_hwndTab, i);
}

char TabBarClass::FarTabShortcut(int tabIndex)
{
	return tabIndex < 10 ? '0' + tabIndex : 'A' + tabIndex - 10;
}

void TabBarClass::FarSendChangeTab(int tabIndex)
{
	PostMessage(hConWnd, WM_KEYDOWN, VK_F14, 0);
	PostMessage(hConWnd, WM_KEYUP, VK_F14, 0);
	PostMessage(hConWnd, WM_KEYDOWN, FarTabShortcut(tabIndex), 0);
	PostMessage(hConWnd, WM_KEYUP, FarTabShortcut(tabIndex), 0);
}

static LRESULT CALLBACK TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MBUTTONUP:
		{
			TabBar.OnMouse(uMsg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			break;
		}
	}
	return CallWindowProc(_defaultTabProc, hwnd, uMsg, wParam, lParam);
}


TabBarClass::TabBarClass()
{
	_active = false;
	_hwndTab = NULL;
	_tabHeight = NULL;
	_titleShouldChange = false;
	_prevTab = 0;
}

bool TabBarClass::IsActive()
{
	return _active;
}

int TabBarClass::Height()
{
	return _tabHeight;
}

void TabBarClass::Activate()
{
	RECT rcClient; 
	GetClientRect(hWnd, &rcClient); 
	InitCommonControls(); 
	_hwndTab = CreateWindow(WC_TABCONTROL, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | TCS_FOCUSNEVER, 0, 0, 
		rcClient.right, 0, hWnd, NULL, g_hInstance, NULL);
	if (_hwndTab == NULL)
	{ 
		return; 
	}
	HFONT hFont = CreateFont(TAB_FONT_HEIGTH, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, 
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TAB_FONT_FACE);
	SendMessage(_hwndTab, WM_SETFONT, WPARAM (hFont), TRUE);
	
	_defaultTabProc = (WNDPROC)SetWindowLongPtr(_hwndTab, GWL_WNDPROC, (LONG_PTR)TabProc);

	_active = true;
}

void TabBarClass::Update(ConEmuTab* tabs, int tabsCount)
{
	if (!_active)
	{
		return;
	}

	TabCtrl_DeleteAllItems(_hwndTab);
	for (int i = 0; i < tabsCount; i++)
	{
		// get file name
		TCHAR dummy[MAX_PATH];
		wchar_t* tFileName=NULL; //--Maximus
		GetFullPathName(tabs[i].Name, MAX_PATH, dummy, &tFileName);
		TCHAR fileName[MAX_PATH+4];
		if (tFileName)
			_tcscpy(fileName, tFileName);
		else
			_tcscpy(fileName, tabs[i].Name);
		// restrict length
		int origLength = _tcslen(fileName);
		if (origLength > 20)
		{
			_tcsnset(fileName, _T('\0'), MAX_PATH);
			_tcsncat(fileName, tFileName, 10);
			_tcsncat(fileName, _T("..."), 3);
			_tcsncat(fileName, tFileName + origLength - 10, 10);
		}

		AddTab(fileName, i);
	}
	for (int i = 0; i < tabsCount; i++)
	{
		if (tabs[i].Current != 0)
		{
			SelectTab(i);
		}
	}
	if (_tabHeight == NULL)
	{
		RECT rcClient; 
		GetClientRect(hWnd, &rcClient); 
		TabCtrl_AdjustRect(_hwndTab, FALSE, &rcClient);
		_tabHeight = rcClient.top;
		UpdatePosition();
	}
}

void TabBarClass::UpdatePosition()
{
	if (!_active)
	{
		return;
	}
	RECT client;
	GetClientRect(hWnd, &client);
	MoveWindow(_hwndTab, 0, 0, client.right, _tabHeight, 1);
}

bool TabBarClass::OnNotify(LPNMHDR nmhdr)
{
	if (!_active)
	{
		return false;
	}

	SetFocus(hWnd);
	if (nmhdr->code == TCN_SELCHANGING)
	{
		_prevTab = TabCtrl_GetCurSel(_hwndTab); 
		return false;
	}

	if (nmhdr->code == TCN_SELCHANGE)
	{ 
		_tcscpy(_lastTitle, Title);
		FarSendChangeTab(TabCtrl_GetCurSel(_hwndTab));
		// start waiting for title to change
		_titleShouldChange = true;
		return true;
	} 
	return false;
}



void TabBarClass::OnTimer()
{
	if (!_active)
	{
		return;
	}
	if (_titleShouldChange)
	{
		// title hasn't changed - tab switching was unsuccessful - return to the previous selected tab
		if (_tcscmp(_lastTitle, Title) == 0)
		{
			SelectTab(_prevTab);
		}
		_titleShouldChange = false;
	}
	return;
}

void TabBarClass::OnMouse(int message, int x, int y)
{
	if (!_active)
	{
		return;
	}

	SetFocus(hWnd);
	if (message == WM_MBUTTONUP)
	{
		TCHITTESTINFO htInfo;
		htInfo.pt.x = x;
		htInfo.pt.y = y;
		int iPage = TabCtrl_HitTest(_hwndTab, &htInfo);
		if (iPage != -1)
		{
			FarSendChangeTab(iPage);
			Sleep(50); // TODO
			PostMessage(hConWnd, WM_KEYDOWN, VK_F10, 0);
			PostMessage(hConWnd, WM_KEYUP, VK_F10, 0);
		}
	}
}	
