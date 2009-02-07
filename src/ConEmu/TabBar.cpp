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
	PostMessage(ghConWnd, WM_KEYDOWN, VK_F14, 0);
	PostMessage(ghConWnd, WM_KEYUP, VK_F14, 0);
	PostMessage(ghConWnd, WM_KEYDOWN, FarTabShortcut(tabIndex), 0);
	PostMessage(ghConWnd, WM_KEYUP, FarTabShortcut(tabIndex), 0);
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

BOOL TabBarClass::IsAllowed()
{
	BOOL lbTabsAllowed = TRUE;
	if (gSet.BufferHeight) {
        CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
        GetConsoleScreenBufferInfo(pVCon->hConOut(), &inf);
        if (inf.dwSize.Y>(inf.srWindow.Bottom-inf.srWindow.Top+1))
			lbTabsAllowed = FALSE;
	}
	return lbTabsAllowed;
}

void TabBarClass::Activate()
{
	RECT rcClient; 
	GetClientRect(ghWnd, &rcClient); 
	InitCommonControls(); 
	_hwndTab = CreateWindow(WC_TABCONTROL, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | TCS_FOCUSNEVER, 0, 0, 
		rcClient.right, 0, ghWnd, NULL, g_hInstance, NULL);
	if (_hwndTab == NULL)
	{ 
		return; 
	}
	HFONT hFont = CreateFont(TAB_FONT_HEIGTH, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, 
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TAB_FONT_FACE);
	SendMessage(_hwndTab, WM_SETFONT, WPARAM (hFont), TRUE);
	
#pragma warning (disable : 4312)
	_defaultTabProc = (WNDPROC)SetWindowLongPtr(_hwndTab, GWL_WNDPROC, (LONG_PTR)TabProc);

	_active = true;
}

void TabBarClass::Deactivate()
{
	if (!_active /*|| !_tabHeight*/)
		return;

	_tabHeight = 0;
	UpdatePosition();
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
		TCHAR dummy[MAX_PATH*2];
		TCHAR fileName[MAX_PATH+4];
		TCHAR szFormat[32];
		wchar_t* tFileName=NULL; //--Maximus
		if (tabs[i].Name[0]==0 || tabs[i].Type == 1/*WTYPE_PANELS*/) {
			_tcscpy(tabs[i].Name, gSet.szTabPanels);
			tFileName = tabs[i].Name;
			_tcscpy(szFormat, _T("%s"));
		} else {
			GetFullPathName(tabs[i].Name, MAX_PATH*2, dummy, &tFileName);
			if (!tFileName)
				tFileName = tabs[i].Name;

			if (tabs[i].Type == 3/*WTYPE_EDITOR*/) {
				if (tabs[i].Modified)
					_tcscpy(szFormat, gSet.szTabEditorModified);
				else
					_tcscpy(szFormat, gSet.szTabEditor);
			} 
			else if (tabs[i].Type == 2/*WTYPE_VIEWER*/)
				_tcscpy(szFormat, gSet.szTabViewer);
		}
		// restrict length
		int origLength = _tcslen(tFileName);
		int nMaxLen = gSet.nTabLenMax - _tcslen(szFormat) + 2/* %s */;
		if (nMaxLen<15) nMaxLen=15; else
			if (nMaxLen>=MAX_PATH) nMaxLen=MAX_PATH-1;
		if (origLength > nMaxLen)
		{
			/*_tcsnset(fileName, _T('\0'), MAX_PATH);
			_tcsncat(fileName, tFileName, 10);
			_tcsncat(fileName, _T("..."), 3);
			_tcsncat(fileName, tFileName + origLength - 10, 10);*/
			int nSplit = nMaxLen*2/3;
			TCHAR szEllip[MAX_PATH+1];
			_tcsncpy(szEllip, tFileName, nSplit); szEllip[nSplit]=0;
			_tcscat(szEllip, _T("..."));
			_tcscat(szEllip, tFileName + origLength - (nMaxLen - nSplit));
			wsprintf(fileName, szFormat, szEllip);
		} else {
			wsprintf(fileName, szFormat, tFileName);
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
		GetClientRect(ghWnd, &rcClient); 
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
	GetClientRect(ghWnd, &client);
	if (ghWndDC) {
		RECT rc = client;
		rc.top = _tabHeight;
		RECT rcChild = gConEmu.WindowSizeFromConsole(
				gConEmu.ConsoleSizeFromWindow(&rc, false /* rectInWindow */, true /* alreadyClient */), 
			false /* rectInWindow */, true /* clientOnly */);
		
		MoveWindow(ghWndDC, rcChild.left, rcChild.top+_tabHeight, rcChild.right-rcChild.left, rcChild.bottom-rcChild.top, 1);

		gConEmu.SyncConsoleToWindow();
		//SyncWindowToConsole();

		//InvalidateRect(ghWnd, NULL, FALSE);
		gConEmu.PaintGaps();
	}
	MoveWindow(_hwndTab, 0, 0, client.right, _tabHeight, 1);
}

bool TabBarClass::OnNotify(LPNMHDR nmhdr)
{
	if (!_active)
	{
		return false;
	}

	SetFocus(ghWnd);
	if (nmhdr->code == TCN_SELCHANGING)
	{
		_prevTab = TabCtrl_GetCurSel(_hwndTab); 
		return false;
	}

	if (nmhdr->code == TCN_SELCHANGE)
	{ 
		_tcscpy(_lastTitle, gConEmu.Title);
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
		if (_tcscmp(_lastTitle, gConEmu.Title) == 0)
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

	SetFocus(ghWnd);
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
			PostMessage(ghConWnd, WM_KEYDOWN, VK_F10, 0);
			PostMessage(ghConWnd, WM_KEYUP, VK_F10, 0);
		}
	}
}	