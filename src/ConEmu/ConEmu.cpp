    
#include <windows.h>
//#include <wchar.h>
//#include <shobjidl.h>

#include "Header.h"

#define RCLICKAPPSTIMEOUT 300
#define RCLICKAPPSDELTA 3
#define DRAG_DELTA 5



#ifdef MSGLOGGER
#define POSTMESSAGE(h,m,w,l,e) {MCHKHEAP; DebugLogMessage(h,m,w,l,TRUE,e); PostMessage(h,m,w,l);}
#define SENDMESSAGE(h,m,w,l) {MCHKHEAP;  DebugLogMessage(h,m,w,l,FALSE,FALSE); SendMessage(h,m,w,l);}
#define SETWINDOWPOS(hw,hp,x,y,w,h,f) {MCHKHEAP; DebugLogPos(hw,x,y,w,h); SetWindowPos(hw,hp,x,y,w,h,f);}
#define MOVEWINDOW(hw,x,y,w,h,r) {MCHKHEAP; DebugLogPos(hw,x,y,w,h); MoveWindow(hw,x,y,w,h,r);}
#define SETCONSOLESCREENBUFFERSIZE(h,s) {MCHKHEAP; DebugLogBufSize(h,s); SetConsoleScreenBufferSize(h,s);}
bool bBlockDebugLog=false, bSendToDebugger=false, bSendToFile=true;
void DebugLogMessage(HWND h, UINT m, WPARAM w, LPARAM l, BOOL posted, BOOL extra);
void DebugLogFile(LPCSTR asMessage);
void DebugLogPos(HWND hw, int x, int y, int w, int h);
void DebugLogBufSize(HANDLE h, COORD sz);
#define DEBUGLOGFILE(m) DebugLogFile(m)
WCHAR *LogFilePath=NULL;
//HANDLE hLogFile=NULL;
#else
#define POSTMESSAGE(h,m,w,l,e) PostMessage(h,m,w,l)
#define SENDMESSAGE(h,m,w,l) SendMessage(h,m,w,l)
#define SETWINDOWPOS(hw,hp,x,y,w,h,f) SetWindowPos(hw,hp,x,y,w,h,f)
#define MOVEWINDOW(hw,x,y,w,h,r) MoveWindow(hw,x,y,w,h,r)
#define SETCONSOLESCREENBUFFERSIZE(h,s) SetConsoleScreenBufferSize(h,s)
#define DEBUGLOGFILE(m)
#endif

#define PTDIFFTEST(C,D) (((abs(C.x-LOWORD(lParam)))<D) && ((abs(C.y-HIWORD(lParam)))<D))

//externs
VirtualConsole *pVCon=NULL;
HWND ghWnd=NULL, ghWndDC=NULL, ghConWnd=NULL;
#ifndef _DEBUG
bool gbUseChildWindow = false;
#else
bool gbUseChildWindow = true;
#endif
const TCHAR *const szClassName = _T("VirtualConsoleClass");
const TCHAR *const szClassNameParent = _T("VirtualConsoleClassMain");
TCHAR temp[MAX_PATH];
TCHAR szIconPath[MAX_PATH];
uint cBlinkNext=0;
DWORD WindowMode=0;
gSettings gSet;
//local variables
//HANDLE hChildProcess=NULL;
HINSTANCE g_hInstance=NULL;

HANDLE hPipe=NULL;
HANDLE hPipeEvent=NULL;

bool isWndNotFSMaximized=false;
bool isShowConsole=false;
bool isNotFullDrag=false;
bool isLBDown=false, /*isInDrag=false,*/ isDragProcessed=false;
bool isRBDown=false, ibSkilRDblClk=false; DWORD dwRBDownTick=0;
bool isPiewUpdate = false; //true; --Maximus5
bool gbPostUpdateWindowSize = false;
HWND hPictureView = NULL; bool bPicViewSlideShow = false; DWORD dwLastSlideShowTick = 0;
bool gb_ConsoleSelectMode=false;
bool setParent = false;
int RBDownNewX=0, RBDownNewY=0;
POINT cursor, Rcursor;
WPARAM lastMMW=-1;
LPARAM lastMML=-1;
OSVERSIONINFO osver;

CDragDrop *DragDrop=NULL;
CProgressBars *ProgressBars=NULL;
ForwardedFileInfo *Files=NULL;
bool ffiresieved=false;
int ffiCount=0;

#define MAX_TITLE_SIZE 0x400
TCHAR Title[MAX_TITLE_SIZE], TitleCmp[MAX_TITLE_SIZE];

POINT cwShift; // difference between window size and client area size for main ConEmu window
POINT cwConsoleShift; // difference between window size and client area size for ghConWnd
COORD srctWindowLast; // console size after last resize (in columns and lines)
RECT  dcWindowLast; // Последний размер дочернего окна
uint cBlinkShift=0; // cursor blink counter threshold


class TrayIcon
{
    NOTIFYICONDATA IconData;

public:
    bool isWindowInTray;

    void HideWindowToTray()
    {
        GetWindowText(ghWnd, IconData.szTip, 127);
        Shell_NotifyIcon(NIM_ADD, &IconData);
        ShowWindow(ghWnd, SW_HIDE);
        isWindowInTray = true;
    }

    void RestoreWindowFromTray()
    {
        ShowWindow(ghWnd, SW_SHOW); 
        SetForegroundWindow(ghWnd);
        Shell_NotifyIcon(NIM_DELETE, &IconData);
        EnableMenuItem(GetSystemMenu(ghWnd, false), ID_TOTRAY, MF_BYCOMMAND | MF_ENABLED);
        isWindowInTray = false;
    }

    void LoadIcon(HWND inWnd, int inIconResource)
    {
        IconData.cbSize = sizeof(NOTIFYICONDATA);
        IconData.uID = 1;
        IconData.hWnd = inWnd;
        IconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        IconData.uCallbackMessage = WM_TRAYNOTIFY;
        //!!!ICON
        IconData.hIcon = (HICON)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(inIconResource), IMAGE_ICON, 
	        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    }

    void Delete()
    {
        Shell_NotifyIcon(NIM_DELETE, &IconData);
    }

} Icon;

// returns difference between window size and client area size of inWnd in outShift->x, outShift->y
void GetCWShift(HWND inWnd, POINT *outShift)
{
    RECT cRect, wRect;
    GetClientRect(inWnd, &cRect); // The left and top members are zero. The right and bottom members contain the width and height of the window.
    GetWindowRect(inWnd, &wRect); // screen coordinates of the upper-left and lower-right corners of the window
    outShift->x = wRect.right  - wRect.left - cRect.right;
    outShift->y = wRect.bottom - wRect.top  - cRect.bottom;
}

void ShowSysmenu(HWND Wnd, HWND Owner, int x, int y)
{
    bool iconic = IsIconic(Wnd);
    bool zoomed = IsZoomed(Wnd);
    bool visible = IsWindowVisible(Wnd);
    int style = GetWindowLong(Wnd, GWL_STYLE);

    HMENU systemMenu = GetSystemMenu(Wnd, false);
    if (!systemMenu)
        return;

    EnableMenuItem(systemMenu, SC_RESTORE, MF_BYCOMMAND | (iconic || zoomed ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(systemMenu, SC_MOVE, MF_BYCOMMAND | (!(iconic || zoomed) ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(systemMenu, SC_SIZE, MF_BYCOMMAND | (!(iconic || zoomed) && (style & WS_SIZEBOX) ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(systemMenu, SC_MINIMIZE, MF_BYCOMMAND | (!iconic && (style & WS_MINIMIZEBOX)? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(systemMenu, SC_MAXIMIZE, MF_BYCOMMAND | (!zoomed && (style & WS_MAXIMIZEBOX) ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(systemMenu, ID_TOTRAY, MF_BYCOMMAND | (visible ? MF_ENABLED : MF_GRAYED));

    SendMessage(Wnd, WM_INITMENU, (WPARAM)systemMenu, 0);
    SendMessage(Wnd, WM_INITMENUPOPUP, (WPARAM)systemMenu, MAKELPARAM(0, true));
    SetActiveWindow(Owner);

    int command = TrackPopupMenu(systemMenu, TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, x, y, 0, Owner, NULL);

    if (Icon.isWindowInTray)
        switch(command)
        {
        case SC_RESTORE:
        case SC_MOVE:
        case SC_SIZE:
        case SC_MINIMIZE:
        case SC_MAXIMIZE:
            SendMessage(Wnd, WM_TRAYNOTIFY, 0, WM_LBUTTONDOWN);
            break;
        }

    if (command)
        PostMessage(Wnd, WM_SYSCOMMAND, (WPARAM)command, 0);
}

HFONT CreateFontIndirectMy(LOGFONT *inFont)
{
    DeleteObject(pVCon->hFont2);

    int width = gSet.FontSizeX2 ? gSet.FontSizeX2 : inFont->lfWidth;
    pVCon->hFont2 = CreateFont(abs(inFont->lfHeight), abs(width), 0, 0, FW_NORMAL,
        0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 0, pVCon->LogFont2.lfFaceName);

    return CreateFontIndirect(inFont);
}

RECT ConsoleOffsetRect()
{
    RECT rect;
	/*if (gbUseChildWindow)
		rect.top = 0;
	else*/
	rect.top = TabBar.IsActive()?TabBar.Height():0;
    rect.left = 0;
    rect.bottom = 0;
    rect.right = 0;
    //#ifdef MSGLOGGER
    //  char szDbg[100]; wsprintfA(szDbg, "   ConsoleOffsetRect={%i,%i,%i,%i}\n", rect.left,rect.top,rect.right,rect.bottom);
    //  DEBUGLOGFILE(szDbg);
    //#endif
    return rect;
}

RECT DCClientRect(RECT* pClient=NULL)
{
    RECT rect;
	if (pClient)
		rect = *pClient;
	else
		GetClientRect(ghWnd, &rect);
	if (TabBar.IsActive())
		rect.top += TabBar.Height();

	if (pClient)
		*pClient = rect;
    return rect;
}

void SyncWindowToConsole()
{
    DEBUGLOGFILE("SyncWindowToConsole\n");
    
    RECT wndR;
    GetWindowRect(ghWnd, &wndR);
    POINT p;
    GetCWShift(ghWnd, &p);
    RECT consoleRect = ConsoleOffsetRect();

    #ifdef MSGLOGGER
        char szDbg[100]; wsprintfA(szDbg, "   pVCon:Size={%i,%i}\n", pVCon->Width,pVCon->Height);
        DEBUGLOGFILE(szDbg);
    #endif
    
    MOVEWINDOW(ghWnd, wndR.left, wndR.top, pVCon->Width + p.x + consoleRect.left, pVCon->Height + p.y + consoleRect.top, 1);
}

/* -- Maximus5. Вернул код из сборки alex_itd
void SyncConsoleToWindow()
{
    SetConsoleWindowSize(ConsoleSizeFromWindow(), true);
    RECT rect;
    GetClientRect(ghWnd, &rect);
    SyncConsoleToWindowRect(rect);
}

void SyncConsoleToWindowRect(const RECT& rect)
{
    COORD size;
    size.X = (rect.right - rect.left) / pVCon->LogFont.lfWidth;
    size.Y = (rect.bottom - rect.top) / pVCon->LogFont.lfHeight;
    SetConsoleWindowSize(size, true);
}
*/

// returns console size in columns and lines calculated from current window size
COORD ConsoleSizeFromWindow(RECT* arect = NULL, bool rectInWindow = false)
{
    RECT rect;
    if (arect == NULL)
    {
        GetClientRect(HDCWND, &rect);
    } 
    else
    {
        rect = *arect;
    }
    RECT consoleRect;
	if (gbUseChildWindow)
		memset(&consoleRect, 0, sizeof(consoleRect));
	else
		consoleRect = ConsoleOffsetRect();
    COORD size;
    size.X = (rect.right - rect.left - (rectInWindow ? cwShift.x : 0) - consoleRect.left) / pVCon->LogFont.lfWidth;
    size.Y = (rect.bottom - rect.top - (rectInWindow ? cwShift.y : 0) - consoleRect.top) / pVCon->LogFont.lfHeight;
    #ifdef MSGLOGGER
        char szDbg[100]; wsprintfA(szDbg, "   ConsoleSizeFromWindow={%i,%i}\n", size.X, size.Y);
        DEBUGLOGFILE(szDbg);
    #endif
    return size;
}

// return window size in pixels calculated from console size
RECT WindowSizeFromConsole(COORD consoleSize, bool rectInWindow = false)
{
    RECT rect;
    rect.top = 0;   
    rect.left = 0;
    RECT consoleRect = ConsoleOffsetRect();
    rect.bottom = consoleSize.Y * pVCon->LogFont.lfHeight + (rectInWindow ? cwShift.y : 0) + consoleRect.top;
    rect.right = consoleSize.X * pVCon->LogFont.lfWidth + (rectInWindow ? cwShift.x : 0) + consoleRect.left;
    #ifdef MSGLOGGER
        char szDbg[100]; wsprintfA(szDbg, "   WindowSizeFromConsole={%i,%i}\n", rect.right,rect.bottom);
        DEBUGLOGFILE(szDbg);
    #endif
    return rect;
}

// size in columns and lines
void SetConsoleWindowSize(const COORD& size, bool updateInfo)
{
    #ifdef MSGLOGGER
        static COORD lastSize1;
        if (lastSize1.Y>size.Y)
            lastSize1.Y=size.Y; //DEBUG
        lastSize1 = size;
        char szDbg[100]; wsprintfA(szDbg, "SetConsoleWindowSize({%i,%i},%i)\n", size.X, size.Y, updateInfo);
        DEBUGLOGFILE(szDbg);
    #endif

    if (isPictureView()) {
        isPiewUpdate = true;
        return;
    }

    // update size info
    if (updateInfo && !gSet.isFullScreen && !IsZoomed(ghWnd))
    {
        gSet.wndWidth = size.X;
        wsprintf(temp, _T("%i"), gSet.wndWidth);
        SetDlgItemText(ghOpWnd, tWndWidth, temp);

        gSet.wndHeight = size.Y;
        wsprintf(temp, _T("%i"), gSet.wndHeight);
        SetDlgItemText(ghOpWnd, tWndHeight, temp);
    }

    // case: simple mode
    if (pVCon->BufferHeight == 0)
    {
        MOVEWINDOW(ghConWnd, 0, 0, 1, 1, 0);
        SETCONSOLESCREENBUFFERSIZE(pVCon->hConOut(), size);
        MOVEWINDOW(ghConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
        return;
    }

    // global flag of the first call which is:
    // *) after getting all the settings
    // *) before running the command
    static bool s_isFirstCall = true;

    // case: buffer mode: change buffer
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(pVCon->hConOut(), &csbi))
        return;
    csbi.dwSize.X = size.X;
    if (s_isFirstCall)
    {
        // first call: buffer height = from settings
        s_isFirstCall = false;
        csbi.dwSize.Y = max(pVCon->BufferHeight, size.Y);
    }
    else
    {
        if (csbi.dwSize.Y == csbi.srWindow.Bottom - csbi.srWindow.Top + 1)
            // no y-scroll: buffer height = new window height
            csbi.dwSize.Y = size.Y;
        else
            // y-scroll: buffer height = old buffer height
            csbi.dwSize.Y = max(csbi.dwSize.Y, size.Y);
    }
    MOVEWINDOW(ghConWnd, 0, 0, 1, 1, 0);
    SETCONSOLESCREENBUFFERSIZE(pVCon->hConOut(), csbi.dwSize);
    
    // set console window
    if (!GetConsoleScreenBufferInfo(pVCon->hConOut(), &csbi))
        return;
    SMALL_RECT rect;
    rect.Top = csbi.srWindow.Top;
    rect.Left = csbi.srWindow.Left;
    rect.Right = rect.Left + size.X - 1;
    rect.Bottom = rect.Top + size.Y - 1;
    if (rect.Right >= csbi.dwSize.X)
    {
        int shift = csbi.dwSize.X - 1 - rect.Right;
        rect.Left += shift;
        rect.Right += shift;
    }
    if (rect.Bottom >= csbi.dwSize.Y)
    {
        int shift = csbi.dwSize.Y - 1 - rect.Bottom;
        rect.Top += shift;
        rect.Bottom += shift;
    }
    SetConsoleWindowInfo(pVCon->hConOut(), TRUE, &rect);
}

void SyncConsoleToWindow()
{
   DEBUGLOGFILE("SyncConsoleToWindow\n");
   SetConsoleWindowSize(ConsoleSizeFromWindow(), true);
}

bool SetWindowMode(uint inMode)
{
    static RECT wndNotFS;
    switch(inMode)
    {
    case rNormal:
        DEBUGLOGFILE("SetWindowMode(rNormal)\n");
    case rMaximized:
        DEBUGLOGFILE("SetWindowMode(rMaximized)\n");
        if (gSet.isFullScreen)
        {
         LONG style = pVCon->BufferHeight ? WS_VSCROLL : 0; // NightRoman
            style |= WS_OVERLAPPEDWINDOW | WS_VISIBLE;
            SetWindowLongPtr(ghWnd, GWL_STYLE, style);
            SETWINDOWPOS(ghWnd, HWND_TOP, wndNotFS.left, wndNotFS.top, wndNotFS.right - wndNotFS.left, wndNotFS.bottom - wndNotFS.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
        gSet.isFullScreen = false;
        SendMessage(ghWnd, WM_SYSCOMMAND, inMode == rNormal ? SC_RESTORE : SC_MAXIMIZE, 0);
        break;

    case rFullScreen:
        DEBUGLOGFILE("SetWindowMode(rFullScreen)\n");
        if (!gSet.isFullScreen)
        {
            gSet.isFullScreen = true;
            isWndNotFSMaximized = IsZoomed(ghWnd);
         if (isWndNotFSMaximized) // в сборке NightRoman эти две строки закомментарены
                SendMessage(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0);

            GetWindowRect(ghWnd, &wndNotFS);

            LONG style = pVCon->BufferHeight ? WS_VSCROLL : 0;
            style |= WS_POPUP | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE;
            SetWindowLongPtr(ghWnd, GWL_STYLE, style);
            SETWINDOWPOS(ghWnd, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED | SWP_SHOWWINDOW);

            CheckRadioButton(ghOpWnd, rNormal, rFullScreen, rFullScreen);
        }
        break;
    }

    bool canEditWindowSizes = inMode == rNormal;
    EnableWindow(GetDlgItem(ghOpWnd, tWndWidth), canEditWindowSizes);
    EnableWindow(GetDlgItem(ghOpWnd, tWndHeight), canEditWindowSizes);
    SyncConsoleToWindow();
    return true;
}

// Заголовок окна для PictureView вообще может пользователем настраиваться, так что
// рассчитывать на него при определения "Просмотра" - нельзя
bool isPictureView()
{
    bool lbRc = false;
    
    if (hPictureView && !IsWindow(hPictureView))
	    hPictureView = NULL;
	
	if (!hPictureView)
		hPictureView = FindWindowEx(HDCWND, NULL, L"FarPictureViewControlClass", NULL);

	lbRc = hPictureView!=NULL;

    // Если вызывали Help (F1) - окошко PictureView прячется
    if (!IsWindowVisible(hPictureView)) {
        lbRc = false;
        hPictureView = NULL;
    }
    if (bPicViewSlideShow && !hPictureView) {
        bPicViewSlideShow=false;
    }

    return lbRc;
}

bool isFilePanel()
{
    TCHAR* pszTitle=Title;
    if (Title[0]==_T('[') && isdigit(Title[1]) && (Title[2]==_T('/') || Title[3]==_T('/'))) {
	    // ConMan
	    pszTitle = _tcschr(Title, _T(']'));
	    if (!pszTitle)
		    return false;
		while (*pszTitle && *pszTitle!=_T('{'))
			pszTitle++;
    }
    
    if ((_tcsncmp(pszTitle, _T("{\\\\"), 3)==0) ||
	    (pszTitle[0] == _T('{') && isalpha(pszTitle[1]) && pszTitle[2] == _T(':') && pszTitle[3] == _T('\\')))
    {
	    TCHAR *Br = _tcsrchr(pszTitle, _T('}'));
	    if (Br && _tcscmp(Br, _T("} - Far"))==0)
		    return true;
    }
    //TCHAR *BrF = _tcschr(Title, '{'), *BrS = _tcschr(Title, '}'), *Slash = _tcschr(Title, '\\');
    //if (BrF && BrS && Slash && BrF == Title && (Slash == Title+1 || Slash == Title+3))
    //    return true;
    return false;
}

bool isConSelectMode()
{
    //TODO: По курсору, что-ли попробовать определять?
    return gb_ConsoleSelectMode;
}

/*
BOOL SetFocusRemote(HWND hwnd)
{
    DWORD   dwOwnerPid, dwCurProcId;
    HANDLE  hProcess=NULL;
    DWORD   dwLastError=0;
    
    //
    //  Open the process which "owns" the window
    //  
    dwCurProcId = GetCurrentProcessId();
    GetWindowThreadProcessId(hwnd, &dwOwnerPid);
    if (dwOwnerPid == dwCurProcId) {
        SetFocus(hwnd);
        return TRUE;
    }

    
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwOwnerPid);
    dwLastError = GetLastError();
    //OpenProcess не дает себя открыть? GetLastError возвращает 5
    if (hProcess==NULL) {
        return FALSE;
    }

    BOOL lbRc = TRUE;
    HANDLE  hThread=NULL;
    hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)SetFocus, hwnd, 0, 0);
    if (hThread) CloseHandle(hThread); else lbRc = FALSE;

    if ((dwOwnerPid!=dwCurProcId) && hProcess)
        CloseHandle(hProcess);

    return TRUE;
}*/

void ForceShowTabs()
{
    BOOL lbNeedInval = FALSE;
    if (!TabBar.IsActive() && gSet.isTabs)
    {
        TabBar.Activate();
        lbNeedInval = TRUE;
		ConEmuTab tab; memset(&tab, 0, sizeof(tab));
		tab.Pos=0;
		tab.Current=1;
		tab.Type = 1;
		TabBar.Update(&tab, 1);
		gbPostUpdateWindowSize = true;
    }
    //ConEmuTab* tabs = (ConEmuTab*)cds->lpData;
    //TabBar.Update(tabs, cds->dwData);
    if (lbNeedInval && pVCon->LogFont.lfWidth)
    {
        SyncConsoleToWindow();
        //RECT rc = ConsoleOffsetRect();
        //rc.bottom = rc.top; rc.top = 0;
        InvalidateRect(HDCWND, NULL, FALSE);
		//InvalidateRect(TabBar._hwndTab, NULL, FALSE);
    }
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    switch (messg)
    {
    case WM_NOTIFY:
        {
            result = TabBar.OnNotify((LPNMHDR)lParam);
            break;
        }
    case WM_COPYDATA:
    {
        PCOPYDATASTRUCT cds = PCOPYDATASTRUCT(lParam);
        BOOL lbNeedInval=FALSE;
        ConEmuTab* tabs = (ConEmuTab*)cds->lpData;
		if (!TabBar.IsActive() && gSet.isTabs && (cds->dwData>1 || tabs[0].Type!=1/*WTYPE_PANELS*/))
        {
            TabBar.Activate();
            lbNeedInval = TRUE;
        }
        TabBar.Update(tabs, cds->dwData);
        if (lbNeedInval)
        {
            SyncConsoleToWindow();
            //RECT rc = ConsoleOffsetRect();
            //rc.bottom = rc.top; rc.top = 0;
            InvalidateRect(ghWnd, NULL/*&rc*/, FALSE);
            if (!gSet.isFullScreen && !IsZoomed(ghWnd)) {
				//SyncWindowToConsole(); -- это делать нельзя, т.к. FAR еще не отработал изменение консоли!
				gbPostUpdateWindowSize = true;
			}
        }
        break;
    }
    
    case WM_ERASEBKGND:
		return 0;
		
	case WM_PAINT:
	{
		if (gbUseChildWindow) {
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			break;
		} else {
	        if (isPictureView())
	        {
				// TODO: если PictureView распахнуто не на все окно - отрисовать видимую часть консоли!
	            if (messg) result = DefWindowProc(hWnd, messg, wParam, lParam);
	            break;
	        }

	        PAINTSTRUCT ps;
	        HDC hDc = BeginPaint(hWnd, &ps);
	        //HDC hDc = GetDC(hWnd);

	        RECT rect;
	        HBRUSH hBrush = CreateSolidBrush(pVCon->Colors[0]); SelectObject(hDc, hBrush);
	        GetClientRect(hWnd, &rect);

	        RECT consoleRect = ConsoleOffsetRect();

	        // paint gaps between console and window client area with first color (actual for maximized and fullscreen modes)
	        rect.top = consoleRect.top; // right 
	        rect.left = pVCon->Width + consoleRect.left;
	        FillRect(hDc, &rect, hBrush);

	        rect.top = pVCon->Height + consoleRect.top; // bottom
	        rect.left = 0; 
	        rect.right = pVCon->Width + consoleRect.left;
	        FillRect(hDc, &rect, hBrush);

	        DeleteObject(hBrush);

	        BitBlt(hDc, consoleRect.left, consoleRect.top, pVCon->Width, pVCon->Height, pVCon->hDC, 0, 0, SRCCOPY);
	        EndPaint(hWnd, &ps);
	        //ReleaseDC(hWnd, hDc);
	        break;
	    }
	}
	
    case WM_TIMER:
    {
        switch (wParam)
        {
        case 0:
            HWND foreWnd = GetForegroundWindow();
            if (!isShowConsole && !gSet.isConVisible)
            {
                /*if (foreWnd == ghConWnd)
                    SetForegroundWindow(hWnd);*/
                if (IsWindowVisible(ghConWnd))
	                ShowWindow(ghConWnd, SW_HIDE);
            }

			//Maximus5. Hack - если какая-то зараза задизеблила окно
			DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
			if (dwStyle & WS_DISABLED)
				EnableWindow(ghWnd, TRUE);

			BOOL lbIsPicView = isPictureView();
            if (bPicViewSlideShow) {
                DWORD dwTicks = GetTickCount();
                DWORD dwElapse = dwTicks - dwLastSlideShowTick;
                if (dwElapse > gSet.nSlideShowElapse)
                {
                    if (IsWindow(hPictureView)) {
                        //
                        bPicViewSlideShow = false;
                        SendMessage(ghConWnd, WM_KEYDOWN, VK_NEXT, 0x01510001);
                        SendMessage(ghConWnd, WM_KEYUP, VK_NEXT, 0xc1510001);

                        // Окно могло измениться?
                        isPictureView();

                        dwLastSlideShowTick = GetTickCount();
                        bPicViewSlideShow = true;
                    } else {
                        hPictureView = NULL;
                        bPicViewSlideShow = false;
                    }
                }
            }

            if (cBlinkNext++ >= cBlinkShift)
            {
                cBlinkNext = 0;
                if (foreWnd == ghWnd || foreWnd == ghOpWnd)
                    // switch cursor
                    pVCon->Cursor.isVisible = !pVCon->Cursor.isVisible;
                else
                    // turn cursor off
                    pVCon->Cursor.isVisible = false;
            }

            DWORD ProcList[2];
            if(GetConsoleProcessList(ProcList,2)==1)
            {
              DestroyWindow(ghWnd);
              break;
            }

            GetWindowText(ghConWnd, TitleCmp, 1024);
            if (wcscmp(Title, TitleCmp))
            {
                wcscpy(Title, TitleCmp);
                SetWindowText(ghWnd, Title);
            }

            TabBar.OnTimer();
            ProgressBars->OnTimer();

            
            if (lbIsPicView)
            {
                bool lbOK = true;
                if (!setParent) {
                    // Проверка, может PictureView создался в консоли, а не в ConEmu?
                    HWND hPicView = FindWindowEx(ghConWnd, NULL, L"FarPictureViewControlClass", NULL);
                    if (!hPicView) {
                        lbOK = false; // смысла нет, все равно ничего не увидим
                    }
                }
                if (lbOK) {
                    isPiewUpdate = true;
                    if (pVCon->Update(false))
                        InvalidateRect(HDCWND, NULL, FALSE);
                    break;
                }
            } else 
            if (isPiewUpdate)
            {	// После скрытия/закрытия PictureView нужно передернуть консоль - ее размер мог измениться
                isPiewUpdate = false;
                SyncConsoleToWindow();
                InvalidateRect(HDCWND, NULL, FALSE);
            }

            // Проверить, может в консоли размер съехал? (хрен его знает из-за чего...)
            {
                CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
                GetConsoleScreenBufferInfo(pVCon->hConOut(), &inf);
                if (inf.dwSize.X>(inf.srWindow.Right-inf.srWindow.Left+1)) {
                    DEBUGLOGFILE("Wrong screen buffer width\n");
                    MOVEWINDOW(ghConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
                } else if ((pVCon->BufferHeight == 0) && (inf.dwSize.Y>(inf.srWindow.Bottom-inf.srWindow.Top+1))) {
                    DEBUGLOGFILE("Wrong screen buffer height\n");
                    MOVEWINDOW(ghConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
                }
            }

            if (pVCon->Update(false))
            {
                COORD c = ConsoleSizeFromWindow();
                if (gbPostUpdateWindowSize || c.X != pVCon->TextWidth || c.Y != pVCon->TextHeight)
                {
					gbPostUpdateWindowSize = false;
                    if (!gSet.isFullScreen && !IsZoomed(ghWnd))
                        SyncWindowToConsole();
                    else
                        SyncConsoleToWindow();
                }

                InvalidateRect(HDCWND, NULL, FALSE);

                // update scrollbar
                if (pVCon->BufferHeight)
                {
                    SCROLLINFO si;
                    ZeroMemory(&si, sizeof(si));
                    si.cbSize = sizeof(si);
                    si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
                    if (GetScrollInfo(ghConWnd, SB_VERT, &si))
                        SetScrollInfo(HDCWND, SB_VERT, &si, true);
                }
          //} -- в сборке NightRoman (isPiewUpdate) проверяется всегда

                /*if (!lbIsPicView && isPiewUpdate)
                {
                    isPiewUpdate = false;
                    SyncConsoleToWindow();
                    InvalidateRect(hWnd, NULL, FALSE);
                }*/
            }
        }
        break;
    }

    case WM_SIZING:
    {
        /*BOOL lbIsPicView = isPictureView();
        if (lbIsPicView) {
            isPiewUpdate = true;
            return TRUE;
        }*/

        RECT *pRect = (RECT*)lParam;
        COORD srctWindow = ConsoleSizeFromWindow(pRect, true /* rectInWindow */);
        // Минимально допустимые размеры консоли
		if (srctWindow.X<28) srctWindow.X=28;
		if (srctWindow.Y<9)  srctWindow.Y=9;

        if ((srctWindowLast.X != srctWindow.X || srctWindowLast.Y != srctWindow.Y) && !isNotFullDrag)
        {
            SetConsoleWindowSize(srctWindow, true);
            srctWindowLast = srctWindow;
        }

        //RECT consoleRect = ConsoleOffsetRect();
        RECT wndSizeRect = WindowSizeFromConsole(srctWindow, true /* rectInWindow */);
        RECT restrictRect;
        restrictRect.right = pRect->left + wndSizeRect.right;
        restrictRect.bottom = pRect->top + wndSizeRect.bottom;
        restrictRect.left = pRect->right - wndSizeRect.right;
        restrictRect.top = pRect->bottom - wndSizeRect.bottom;
        

        switch(wParam)
        {
        case WMSZ_RIGHT:
        case WMSZ_BOTTOM:
        case WMSZ_BOTTOMRIGHT:
         pRect->right = restrictRect.right; // NightRoman
        //  pRect->right =  srctWindow.X * pVCon->LogFont.lfWidth  + cwShift.x + pRect->left;
         pRect->bottom = restrictRect.bottom; // NightRoman
        //  pRect->bottom = srctWindow.Y * pVCon->LogFont.lfHeight + cwShift.y + pRect->top;
            break;
        case WMSZ_LEFT:
        case WMSZ_TOP:
        case WMSZ_TOPLEFT:
         pRect->left = restrictRect.left; // NightRoman
        //  pRect->left =  pRect->right - srctWindow.X * pVCon->LogFont.lfWidth  - cwShift.x;
         pRect->top = restrictRect.top; // NightRoman
        //  pRect->top  = pRect->bottom - srctWindow.Y * pVCon->LogFont.lfHeight - cwShift.y;
            break;
        case WMSZ_TOPRIGHT:
         pRect->right = restrictRect.right; // NightRoman
        //  pRect->right =  srctWindow.X * pVCon->LogFont.lfWidth  + cwShift.x + pRect->left;
         pRect->top = restrictRect.top; // NightRoman
        //  pRect->top  = pRect->bottom - srctWindow.Y * pVCon->LogFont.lfHeight - cwShift.y;
            break;
        case WMSZ_BOTTOMLEFT:
         pRect->left = restrictRect.left; // NightRoman
        //  pRect->left =  pRect->right - srctWindow.X * pVCon->LogFont.lfWidth  - cwShift.x;
         pRect->bottom = restrictRect.bottom; // NightRoman
        //  pRect->bottom = srctWindow.Y * pVCon->LogFont.lfHeight + cwShift.y + pRect->top;
            break;
        }
        result = true;
        break;
    }
    case WM_SIZE:
    {
        BOOL lbIsPicView = isPictureView();
        
        if (gbUseChildWindow)
		{
	        RECT rcNewCon; memset(&rcNewCon, 0, sizeof(rcNewCon));
			rcNewCon.right = LOWORD(lParam);
			rcNewCon.bottom = HIWORD(lParam);
			DCClientRect(&rcNewCon);
	        MoveWindow(ghWndDC, rcNewCon.left, rcNewCon.top, rcNewCon.right - rcNewCon.left, rcNewCon.bottom - rcNewCon.top, 0);
	        dcWindowLast = rcNewCon;
        }
        else
        {
            if (lbIsPicView) {
                isPiewUpdate = true;
                RECT rcClient; GetClientRect(HDCWND, &rcClient);
                MoveWindow(hPictureView, 0,0,rcClient.right,rcClient.bottom, 1);
                InvalidateRect(HDCWND, NULL, FALSE);
                //SetFocus(hPictureView); -- все равно фокус в другой процесс не передастся
            }
        }
        
        if (!lbIsPicView)
        {
            if (isNotFullDrag)
            {
                RECT pRect = {0, 0, LOWORD(lParam), HIWORD(lParam)};

                COORD srctWindow = ConsoleSizeFromWindow(&pRect);

                if ((srctWindowLast.X != srctWindow.X || srctWindowLast.Y != srctWindow.Y))
                {
                    SetConsoleWindowSize(srctWindow, true);
                    srctWindowLast = srctWindow;
                }
            }

            {
                static bool wPrevSizeMax = false;
                if ((wParam == SIZE_MAXIMIZED || (wParam == SIZE_RESTORED && wPrevSizeMax)) && ghConWnd)
                {
                    wPrevSizeMax = wParam == SIZE_MAXIMIZED;

                    RECT pRect;
                    GetWindowRect(ghWnd, &pRect);
                    pRect.right = LOWORD(lParam) + pRect.left + cwShift.x;
                    pRect.bottom = HIWORD(lParam) + pRect.top + cwShift.y;

                    //TODO: есть подозрение, что этот фейк вызывает глюк с левым кликом по рамке...
                    // fake WM_SIZING event to adjust console size to new window size after Maximize or Restore Down 
                    //SendMessage(hWnd, WM_SIZING, WMSZ_TOP, (LPARAM)&pRect);
                    //SendMessage(hWnd, WM_SIZING, WMSZ_RIGHT, (LPARAM)&pRect);
                    //Заменил на WndProc!
                    WndProc ( ghWnd, WM_SIZING, WMSZ_TOP, (LPARAM)&pRect );
                    WndProc ( ghWnd, WM_SIZING, WMSZ_RIGHT, (LPARAM)&pRect );
                }

                if (TabBar.IsActive())
                {
                    TabBar.UpdatePosition();
                }
            }
        }
        break;
    }

    case WM_KEYDOWN:
    case WM_KEYUP:

        if (wParam == VK_PAUSE && !isPressed(VK_CONTROL)) {
            if (isPictureView()) {
                if (messg == WM_KEYUP) {
                    bPicViewSlideShow = !bPicViewSlideShow;
                    if (bPicViewSlideShow) {
                        if (gSet.nSlideShowElapse<=500) gSet.nSlideShowElapse=500;
                        //SetTimer(hWnd, 3, gSet.nSlideShowElapse, NULL);
                        dwLastSlideShowTick = GetTickCount() - gSet.nSlideShowElapse;
                    //} else {
                    //  KillTimer(hWnd, 3);
                    }
                }
                break;
            }
        } else if (bPicViewSlideShow) {
            //KillTimer(hWnd, 3);
            if (wParam==0xbd/* -_ */ || wParam==0xbb/* =+ */) {
                if (messg == WM_KEYDOWN) {
                    if (wParam==0xbb)
                        gSet.nSlideShowElapse = 1.2 * gSet.nSlideShowElapse;
                    else {
                        gSet.nSlideShowElapse = gSet.nSlideShowElapse / 1.2;
                        if (gSet.nSlideShowElapse<=500) gSet.nSlideShowElapse=500;
                    }
                }
            } else {
                bPicViewSlideShow = false; // отмена слайдшоу
            }
        } else if (messg == WM_KEYDOWN && wParam == VK_NEXT) {
            lParam = lParam;
        } else if (messg == WM_KEYUP && wParam == VK_NEXT) {
            lParam = lParam;
        }

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:

    case WM_MOUSEWHEEL:

        /*if (hPictureView && IsWindow(hPictureView)) {
            HWND hFocus = GetFocus(), hFocus1=NULL;
            if (hFocus != hPictureView) {
                SetFocusRemote(hPictureView);
                hFocus1 = GetFocus();
            }
            hFocus = hFocus1;
        }*/

    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
    case WM_KILLFOCUS:
    case WM_SETFOCUS:

	    if (messg == WM_SETFOCUS) {
			if (ghWndDC && IsWindow(ghWndDC))
				SetFocus(ghWndDC);
		}
    
        /*if (messg == WM_SETFOCUS || messg == WM_KILLFOCUS) {
            if (hPictureView && IsWindow(hPictureView)) {
                break; // в FAR не пересылать
            }
        }*/

      // buffer mode: scroll with keys  -- NightRoman
        if (pVCon->BufferHeight && messg == WM_KEYDOWN && isPressed(VK_CONTROL))
        {
            switch(wParam)
            {
            case VK_DOWN:
                POSTMESSAGE(ghConWnd, WM_VSCROLL, SB_LINEDOWN, NULL, FALSE);
                break;
            case VK_UP:
                POSTMESSAGE(ghConWnd, WM_VSCROLL, SB_LINEUP, NULL, FALSE);
                break;
            case VK_NEXT:
                POSTMESSAGE(ghConWnd, WM_VSCROLL, SB_PAGEDOWN, NULL, FALSE);
                break;
            case VK_PRIOR:
                POSTMESSAGE(ghConWnd, WM_VSCROLL, SB_PAGEUP, NULL, FALSE);
                break;
            }
        }

        if (messg == WM_KEYDOWN && wParam == VK_SPACE && isPressed(VK_CONTROL) && isPressed(VK_LWIN) && isPressed(VK_MENU))
        {
            if (!IsWindowVisible(ghConWnd))
            {
                isShowConsole = true;
                ShowWindow(ghConWnd, SW_SHOWNORMAL);
                if (setParent) SetParent(ghConWnd, 0);
                EnableWindow(ghConWnd, true);
            }
            else
            {
                isShowConsole = false;
                if (!gSet.isConVisible) ShowWindow(ghConWnd, SW_HIDE);
                if (setParent) SetParent(ghConWnd, HDCWND);
                if (!gSet.isConVisible) EnableWindow(ghConWnd, false);
            }
            break;
        }

        if (gb_ConsoleSelectMode && messg == WM_KEYDOWN && ((wParam == VK_ESCAPE) || (wParam == VK_RETURN)))
            gb_ConsoleSelectMode = false; //TODO: может как-то по другому определять?

        // Основная обработка 
        {
            if (messg == WM_SYSKEYDOWN) 
                if (wParam == VK_INSERT && lParam & 29)
                    gb_ConsoleSelectMode = true;

            static bool isSkipNextAltUp = false;
            if (messg == WM_SYSKEYDOWN && wParam == VK_RETURN && lParam & 29)
            {
                if (gSet.isSentAltEnter)
                {
                    POSTMESSAGE(ghConWnd, WM_KEYDOWN, VK_MENU, 0, TRUE);
                    POSTMESSAGE(ghConWnd, WM_KEYDOWN, VK_RETURN, 0, TRUE);
                    POSTMESSAGE(ghConWnd, WM_KEYUP, VK_RETURN, 0, TRUE);
                    POSTMESSAGE(ghConWnd, WM_KEYUP, VK_MENU, 0, TRUE);
                }
                else
                {
                    if (isPressed(VK_SHIFT))
                        break;

                    if (!gSet.isFullScreen)
                        SetWindowMode(rFullScreen);
                    else
                        SetWindowMode(isWndNotFSMaximized ? rMaximized : rNormal);

                    isSkipNextAltUp = true;
                    //POSTMESSAGE(ghConWnd, messg, wParam, lParam);
                }
            }
            else if (messg == WM_SYSKEYDOWN && wParam == VK_SPACE && lParam & 29 && !isPressed(VK_SHIFT))
            {
                RECT rect, cRect;
                GetWindowRect(ghWnd, &rect);
                GetClientRect(ghWnd, &cRect);
                WINDOWINFO wInfo;   GetWindowInfo(ghWnd, &wInfo);
                ShowSysmenu(ghWnd, ghWnd, rect.right - cRect.right - wInfo.cxWindowBorders, rect.bottom - cRect.bottom - wInfo.cyWindowBorders);
            }
            else if (messg == WM_KEYUP && wParam == VK_MENU && isSkipNextAltUp) isSkipNextAltUp = false;
            else if (messg == WM_SYSKEYDOWN && wParam == VK_F9 && lParam & 29 && !isPressed(VK_SHIFT))
                SetWindowMode(IsZoomed(ghWnd) ? rNormal : rMaximized);
            else
                POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
        }
        break;

    case WM_MOUSEMOVE:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    {
        RECT conRect;
        GetClientRect(ghConWnd, &conRect);
        short winX = GET_X_LPARAM(lParam);
        short winY = GET_Y_LPARAM(lParam);
        RECT consoleRect;
		if (gbUseChildWindow)
			memset(&consoleRect, 0, sizeof(consoleRect));
		else
			consoleRect = ConsoleOffsetRect();
        winX -= consoleRect.left;
        winY -= consoleRect.top;
        short newX = MulDiv(winX, conRect.right, klMax<uint>(1, pVCon->Width));
        short newY = MulDiv(winY, conRect.bottom, klMax<uint>(1, pVCon->Height));

        if (newY<0 || newX<0)
            break;

        if (pVCon->BufferHeight)
        {
           // buffer mode: cheat the console window: adjust its position exactly to the cursor
           RECT win;
           GetWindowRect(ghWnd, &win);
           short x = win.left + winX - newX;
           short y = win.top + winY - newY;
           RECT con;
           GetWindowRect(ghConWnd, &con);
           if (con.left != x || con.top != y)
              MOVEWINDOW(ghConWnd, x, y, con.right - con.left + 1, con.bottom - con.top + 1, TRUE);
        }
        else if (messg == WM_MOUSEMOVE)
        {
            // WM_MOUSEMOVE вроде бы слишком часто вызывается даже при условии что курсор не двигается...
            if (wParam==lastMMW && lParam==lastMML) {
                break;
            }
            lastMMW=wParam; lastMML=lParam;

            /*if (isLBDown &&   (cursor.x-LOWORD(lParam)>DRAG_DELTA || 
                               cursor.x-LOWORD(lParam)<-DRAG_DELTA || 
                               cursor.y-HIWORD(lParam)>DRAG_DELTA || 
                               cursor.y-HIWORD(lParam)<-DRAG_DELTA))*/
            if (isLBDown && !PTDIFFTEST(cursor,DRAG_DELTA) && !isDragProcessed)
            {
	            // Если сначала фокус был на файловой панели, но после LClick он попал на НЕ файловую - отменить ShellDrag
	            if (!isFilePanel()) {
		            isLBDown = false;
		            break;
	            }
                // Иначе иногда срабатывает FAR'овский D'n'D
                //SENDMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ));     //посылаем консоли отпускание
				if (DragDrop)
					DragDrop->Drag(); //сдвинулись при зажатой левой
				//isDragProcessed=false; -- убрал, иначе при бросании в пассивную панель больших файлов дроп может вызваться еще раз???
                POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание
                break;
            }
            else if (gSet.isRClickSendKey && isRBDown)
            {
                //Если двинули мышкой, а была включена опция RClick - не вызывать
                //контекстное меню - просто послать правый клик
                if (!PTDIFFTEST(Rcursor, RCLICKAPPSDELTA))
                {
                    isRBDown=false;
                    POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, 0, MAKELPARAM( RBDownNewX, RBDownNewY ), TRUE);
                }
                break;
            }
            /*if (!isRBDown && (wParam==MK_RBUTTON)) {
                // Чтобы при выделении правой кнопкой файлы не пропускались
                if ((newY-RBDownNewY)>5) {// пока попробуем для режима сверху вниз
                    for (short y=RBDownNewY;y<newY;y+=5)
                        POSTMESSAGE(ghConWnd, WM_MOUSEMOVE, wParam, MAKELPARAM( RBDownNewX, y ), TRUE);
                }
                RBDownNewX=newX; RBDownNewY=newY;
            }*/
        } else {
            lastMMW=-1; lastMML=-1;

            if (messg == WM_LBUTTONDOWN)
            {
                if (isLBDown) ReleaseCapture(); // Вдруг сглючило?
                isLBDown=false;
                if (!isConSelectMode() && isFilePanel())
                {
                    SetCapture(ghWnd);
                    cursor.x = LOWORD(lParam);
                    cursor.y = HIWORD(lParam); 
                    isLBDown=true;
                    isDragProcessed=false;
                    POSTMESSAGE(ghConWnd, messg, wParam, MAKELPARAM( newX, newY ), FALSE); // было SEND
                    break;
                }
            }
            else if (messg == WM_LBUTTONUP)
            {
                if (isLBDown) {
                    isLBDown=false;
                    ReleaseCapture();
                }
            }
            else if (messg == WM_RBUTTONDOWN)
            {
                Rcursor.x = LOWORD(lParam);
                Rcursor.y = HIWORD(lParam);
                RBDownNewX=newX;
                RBDownNewY=newY;
                isRBDown=false;

                // Если ничего лишнего не нажато!
                if (gSet.isRClickSendKey && !(wParam&(MK_CONTROL|MK_LBUTTON|MK_MBUTTON|MK_SHIFT|MK_XBUTTON1|MK_XBUTTON2)))
                {
                    //TCHAR *BrF = _tcschr(Title, '{'), *BrS = _tcschr(Title, '}'), *Slash = _tcschr(Title, '\\');
                    //if (BrF && BrS && Slash && BrF == Title && (Slash == Title+1 || Slash == Title+3))
                    if (isFilePanel()) // Maximus5
                    {
                        //заведем таймер на .3
                        //если больше - пошлем apps
                        isRBDown=true; ibSkilRDblClk=false;
                        //SetTimer(hWnd, 1, 300, 0); -- Maximus5, откажемся от таймера
                        dwRBDownTick = GetTickCount();
                        break;
                    }
                }
            }
            else if (messg == WM_RBUTTONUP)
            {
                if (gSet.isRClickSendKey && isRBDown)
                {
                    isRBDown=false; // сразу сбросим!
                    if (PTDIFFTEST(Rcursor,RCLICKAPPSDELTA))
                    {
                        //держали зажатой <.3
                        //убьем таймер, кликнием правой кнопкой
                        //KillTimer(hWnd, 1); -- Maximus5, таймер более не используется
                        DWORD dwCurTick=GetTickCount();
                        DWORD dwDelta=dwCurTick-dwRBDownTick;
                        // Если держали дольше .3с, но не слишком долго :)
                        if ((gSet.isRClickSendKey==1) ||
                            (dwDelta>RCLICKAPPSTIMEOUT && dwDelta<10000))
                        {
                            // Сначала выделить файл под курсором
                            POSTMESSAGE(ghConWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM( RBDownNewX, RBDownNewY ), TRUE);
                            POSTMESSAGE(ghConWnd, WM_LBUTTONUP, 0, MAKELPARAM( RBDownNewX, RBDownNewY ), TRUE);
                        
                            pVCon->Update(true);
                            InvalidateRect(HDCWND, NULL, FALSE);

                            // А теперь можно и Apps нажать
							ibSkilRDblClk=true; // чтобы пока FAR думает в консоль не проскочило мышиное сообщение
                            POSTMESSAGE(ghConWnd, WM_KEYDOWN, VK_APPS, 0, TRUE);
                            break;
                        }
                    }
                    // Иначе нужно сначала послать WM_RBUTTONDOWN
                    POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, wParam, MAKELPARAM( newX, newY ), TRUE);
                }
                isRBDown=false; // чтобы не осталось случайно
            }
			else if (messg == WM_RBUTTONDBLCLK) {
				if (ibSkilRDblClk) {
					ibSkilRDblClk = false;
					break; // не обрабатывать, сейчас висит контекстное меню
				}
			}
        }

//      заменим даблклик вторым обычным
        POSTMESSAGE(ghConWnd, messg == WM_RBUTTONDBLCLK ? WM_RBUTTONDOWN : messg, wParam, MAKELPARAM( newX, newY ), FALSE);
        break;
    }

    case WM_CLOSE:
        //Icon.Delete(); - перенес в WM_DESTROY
        SENDMESSAGE(ghConWnd, WM_CLOSE, 0, 0);
        break;

    case WM_CREATE:
		{
			ghWnd = hWnd; // ставим сразу, чтобы функции могли пользоваться
			Icon.LoadIcon(hWnd, gSet.nIconID/*IDI_ICON1*/);

			if (gbUseChildWindow) {
				WNDCLASS wc = {CS_OWNDC|CS_DBLCLKS|CS_SAVEBITS, ChildWndProc, 0, 0, 
						g_hInstance, NULL, LoadCursor(NULL, IDC_ARROW), 
						NULL /*(HBRUSH)COLOR_BACKGROUND/**/, 
						NULL, szClassName};// | CS_DROPSHADOW
				if (!RegisterClass(&wc)) {
					ghWndDC = (HWND)-1; // чтобы родитель не ругался
					MBoxA(_T("Can't register DC window class!"));
					return -1;
				}
				DWORD style = WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS /*| WS_CLIPCHILDREN*/ | (pVCon->BufferHeight ? WS_VSCROLL : 0);
				RECT rc = DCClientRect();
				ghWndDC = CreateWindow(szClassName, 0, style, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hWnd, NULL, (HINSTANCE)g_hInstance, NULL);
				if (!ghWndDC) {
					ghWndDC = (HWND)-1; // чтобы родитель не ругался
					MBoxA(_T("Can't create DC window!"));
					return -1; //
				}
				dcWindowLast = rc; //TODO!!!
			}
		}
        break;

    case WM_SYSCOMMAND:
        switch(LOWORD(wParam))
        {
        case ID_SETTINGS:
            DialogBox((HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), 0, wndOpProc);
            break;
        case ID_HELP:
            MessageBoxA(ghOpWnd, pHelp, "About ConEmu...", MB_ICONQUESTION);
            break;
        case ID_TOTRAY:
            Icon.HideWindowToTray();
            break;
        case ID_CONPROP:
            #ifdef _DEBUG
            {
                HMENU hMenu = GetSystemMenu(ghConWnd, FALSE);
                MENUITEMINFO mii; TCHAR szText[255];
                for (int i=0; i<15; i++) {
                    memset(&mii, 0, sizeof(mii));
                    mii.cbSize = sizeof(mii); mii.dwTypeData=szText; mii.cch=255;
                    mii.fMask = MIIM_ID|MIIM_STRING;
                    if (GetMenuItemInfo(hMenu, i, TRUE, &mii)) {
                        mii.cbSize = sizeof(mii);
                    } else
                        break;
                }
            }
            #endif
            POSTMESSAGE(ghConWnd, WM_SYSCOMMAND, 65527, 0, TRUE);
            break;
        }

        switch(wParam)
        {
        case SC_MAXIMIZE_SECRET:
            SetWindowMode(rMaximized);
            break;
        case SC_RESTORE_SECRET:
            SetWindowMode(rNormal);
            break;
        case SC_CLOSE:
            Icon.Delete();
            SENDMESSAGE(ghConWnd, WM_CLOSE, 0, 0);
            break;

        case SC_MAXIMIZE:
            if (wParam == SC_MAXIMIZE)
                CheckRadioButton(ghOpWnd, rNormal, rFullScreen, rMaximized);
        case SC_RESTORE:
            if (wParam == SC_RESTORE)
                CheckRadioButton(ghOpWnd, rNormal, rFullScreen, rNormal);

        default:
            if (wParam != 0xF100)
            {
                POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
                result = DefWindowProc(hWnd, messg, wParam, lParam);
            }
        }
        break;

    case WM_NCRBUTTONUP:
        Icon.HideWindowToTray();
        break;

    case WM_TRAYNOTIFY:
        switch(lParam)
        {
        case WM_LBUTTONUP:
            Icon.RestoreWindowFromTray();
            break;
        case WM_RBUTTONUP:
            {
            POINT mPos;
            GetCursorPos(&mPos);
            SetForegroundWindow(hWnd);
            ShowSysmenu(hWnd, hWnd, mPos.x, mPos.y);
            PostMessage(hWnd, WM_NULL, 0, 0);
            }
            break;
        } 
        break; 


    case WM_DESTROY:
        Icon.Delete();
        if (DragDrop) {
	        delete DragDrop;
	        DragDrop = NULL;
        }
        if (ProgressBars) {
	        delete ProgressBars;
	        ProgressBars = NULL;
        }
        PostQuitMessage(0);
        break;
    
    case WM_INPUTLANGCHANGE:
    case WM_INPUTLANGCHANGEREQUEST:
    case WM_IME_NOTIFY:
    case WM_VSCROLL:
        POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
        
    default:
        if (messg) result = DefWindowProc(hWnd, messg, wParam, lParam);
    }
    return result;
}

LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    switch (messg)
    {
    case WM_COPYDATA:
		// если уж пришло сюда - передадим куда надо
		return WndProc ( ghWnd, messg, wParam, lParam );

    case WM_ERASEBKGND:
		return 0;
		
    case WM_PAINT:
    {
        if (isPictureView())
        {
			// TODO: если PictureView распахнуто не на все окно - отрисовать видимую часть консоли!
            if (messg) result = DefWindowProc(hWnd, messg, wParam, lParam);
            break;
        }

        PAINTSTRUCT ps;
        HDC hDc = BeginPaint(hWnd, &ps);
        //HDC hDc = GetDC(hWnd);

        RECT rect;
        HBRUSH hBrush = CreateSolidBrush(pVCon->Colors[0]); SelectObject(hDc, hBrush);
        GetClientRect(hWnd, &rect);

        //RECT consoleRect = ConsoleOffsetRect();

        // paint gaps between console and window client area with first color (actual for maximized and fullscreen modes)
        //rect.top = consoleRect.top; -- это не нужно, т.к. DC теперь рисуется в дочернем окне
        rect.left = pVCon->Width; //+ consoleRect.left;
        FillRect(hDc, &rect, hBrush);

        rect.top = 0; //pVCon->Height + consoleRect.top; // bottom
        rect.left = 0; 
        rect.right = pVCon->Width; //+ consoleRect.left;
        FillRect(hDc, &rect, hBrush);

        DeleteObject(hBrush);

        BitBlt(hDc, 0, 0, pVCon->Width, pVCon->Height, pVCon->hDC, 0, 0, SRCCOPY);
        EndPaint(hWnd, &ps);
        //ReleaseDC(hWnd, hDc);
        break;
    }

    case WM_SIZE:
    {
        BOOL lbIsPicView = FALSE;

		RECT rcNewClient; GetClientRect(hWnd,&rcNewClient);
        
        if (isPictureView())
        {
            if (hPictureView) {
                lbIsPicView = TRUE;
                isPiewUpdate = true;
                RECT rcClient; GetClientRect(hWnd, &rcClient);
                //TODO: а ведь PictureView может и в QuickView активироваться...
                MoveWindow(hPictureView, 0,0,rcClient.right,rcClient.bottom, 1);
                InvalidateRect(hWnd, NULL, FALSE);
                //SetFocus(hPictureView); -- все равно на другой процесс фокус передать нельзя...
            }
        }
        break;
    }
    

    case WM_CREATE:
        break;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_MOUSEWHEEL:
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
    case WM_KILLFOCUS:
    case WM_SETFOCUS:
    case WM_MOUSEMOVE:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_INPUTLANGCHANGE:
    case WM_INPUTLANGCHANGEREQUEST:
    case WM_IME_NOTIFY:
    case WM_VSCROLL:
        // Вся обработка в родителе
        result = WndProc(hWnd, messg, wParam, lParam);
        return result;

    default:
        if (messg) result = DefWindowProc(hWnd, messg, wParam, lParam);
    }
    return result;
}


BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    return (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT ? true : false);
}

#ifdef MSGLOGGER
void DebugLogMessage(HWND h, UINT m, WPARAM w, LPARAM l, BOOL posted, BOOL extra)
{
    if (bBlockDebugLog || (!bSendToFile && !IsDebuggerPresent()))
        return;

    static char szMess[32], szWP[32], szLP[32], szWhole[255];
    //static SYSTEMTIME st;
    szMess[0]=0; szWP[0]=0; szLP[0]=0; szWhole[0]=0;
    
    #define MSG1(s) case s: lstrcpyA(szMess, #s); break;
    #define MSG2(s) case s: lstrcpyA(szMess, #s);
    #define WP1(s) case s: lstrcpyA(szWP, #s); break;
    #define WP2(s) case s: lstrcpyA(szWP, #s);
    
    switch (m)
    {
    MSG1(WM_NOTIFY);
    MSG1(WM_COPYDATA);
    MSG1(WM_PAINT);
    MSG1(WM_TIMER);
    MSG2(WM_SIZING);
    {
        switch(w)
        {
        WP1(WMSZ_RIGHT);
        WP1(WMSZ_BOTTOM);
        WP1(WMSZ_BOTTOMRIGHT);
        WP1(WMSZ_LEFT);
        WP1(WMSZ_TOP);
        WP1(WMSZ_TOPLEFT);
        WP1(WMSZ_TOPRIGHT);
        WP1(WMSZ_BOTTOMLEFT);
        }
        break;
    }
    MSG1(WM_SIZE);
    MSG1(WM_KEYDOWN);
    MSG1(WM_KEYUP);
    MSG1(WM_SYSKEYDOWN);
    MSG1(WM_SYSKEYUP);
    MSG2(WM_MOUSEWHEEL);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_ACTIVATE);
		switch(w) {
			WP1(WA_ACTIVE);
			WP1(WA_CLICKACTIVE);
			WP1(WA_INACTIVE);
		}
        break;
    MSG2(WM_ACTIVATEAPP);
        if (w==0)
            break;
        break;
    MSG2(WM_KILLFOCUS);
        break;
    MSG1(WM_SETFOCUS);
    MSG2(WM_MOUSEMOVE);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_RBUTTONDOWN);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_RBUTTONUP);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_MBUTTONDOWN);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_MBUTTONUP);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_LBUTTONDOWN);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_LBUTTONUP);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_LBUTTONDBLCLK);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_MBUTTONDBLCLK);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_RBUTTONDBLCLK);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG1(WM_CLOSE);
    MSG1(WM_CREATE);
    MSG2(WM_SYSCOMMAND);
    {   
        switch(w)
        {
        WP1(SC_MAXIMIZE_SECRET);
        WP2(SC_RESTORE_SECRET);
            break;
        WP1(SC_CLOSE);
        WP1(SC_MAXIMIZE);
        WP2(SC_RESTORE);
            break;
        WP1(SC_PROPERTIES_SECRET);
        WP1(ID_SETTINGS);
        WP1(ID_HELP);
        WP1(ID_TOTRAY);
        WP1(ID_CONPROP);
        }
        break;
    }
    MSG1(WM_NCRBUTTONUP);
    MSG1(WM_TRAYNOTIFY);
    MSG1(WM_DESTROY);
    MSG1(WM_INPUTLANGCHANGE);
    MSG1(WM_INPUTLANGCHANGEREQUEST);
    MSG1(WM_IME_NOTIFY);
    MSG1(WM_VSCROLL);
    MSG1(WM_NULL);
    //default:
    //  return;
    }

    if (bSendToDebugger || bSendToFile) {
        if (!szMess[0]) wsprintfA(szMess, "%i", m);
        if (!szWP[0]) wsprintfA(szWP, "%i", w);
        if (!szLP[0]) wsprintfA(szLP, "%i", l);
        

        if (bSendToDebugger) {
            static SYSTEMTIME st;
            GetLocalTime(&st);
            wsprintfA(szWhole, "%02i:%02i.%03i %s%s(%s, %s, %s)\n", st.wMinute, st.wSecond, st.wMilliseconds,
                (posted ? "Post" : "Send"), (extra ? "+" : ""), szMess, szWP, szLP);

            OutputDebugStringA(szWhole);
        } else if (bSendToFile) {
            wsprintfA(szWhole, "%s%s(%s, %s, %s)\n",
                (posted ? "Post" : "Send"), (extra ? "+" : ""), szMess, szWP, szLP);

            DebugLogFile(szWhole);
        }
    }
}
void DebugLogPos(HWND hw, int x, int y, int w, int h)
{
    if (bBlockDebugLog || (!bSendToFile && !IsDebuggerPresent()))
        return;

    static char szPos[255];

    if (bSendToDebugger) {
        static SYSTEMTIME st;
        GetLocalTime(&st);
        wsprintfA(szPos, "%02i:%02i.%03i ChangeWindowPos(%s, %i,%i,%i,%i)\n",
            st.wMinute, st.wSecond, st.wMilliseconds,
            (hw==ghConWnd) ? "Con" : "Emu", x,y,w,h);

        OutputDebugStringA(szPos);
    } else if (bSendToFile) {
        wsprintfA(szPos, "ChangeWindowPos(%s, %i,%i,%i,%i)\n",
            (hw==ghConWnd) ? "Con" : "Emu", x,y,w,h);

        DebugLogFile(szPos);
    }
}
void DebugLogBufSize(HANDLE h, COORD sz)
{
    if (bBlockDebugLog || (!bSendToFile && !IsDebuggerPresent()))
        return;

    static char szSize[255];

    if (bSendToDebugger) {
        static SYSTEMTIME st;
        GetLocalTime(&st);
        wsprintfA(szSize, "%02i:%02i.%03i ChangeBufferSize(%i,%i)\n",
            st.wMinute, st.wSecond, st.wMilliseconds,
            sz.X, sz.Y);

        OutputDebugStringA(szSize);
    } else if (bSendToFile) {
        wsprintfA(szSize, "ChangeBufferSize(%i,%i)\n",
            sz.X, sz.Y);

        DebugLogFile(szSize);
    }
}
void DebugLogFile(LPCSTR asMessage)
{
    HANDLE hLogFile = INVALID_HANDLE_VALUE;

    if (LogFilePath==NULL) {
        //WCHAR LogFilePath[MAX_PATH+1];
        LogFilePath = (WCHAR*)calloc(MAX_PATH+1,sizeof(WCHAR));
        GetModuleFileNameW(0,LogFilePath,MAX_PATH);
        WCHAR* pszDot = wcsrchr(LogFilePath, L'.');
        lstrcpyW(pszDot, L".log");
        hLogFile = CreateFileW(LogFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    } else {
        hLogFile = CreateFileW(LogFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    if (hLogFile!=INVALID_HANDLE_VALUE) {
        DWORD dwSize=0;
        SetFilePointer(hLogFile, 0, 0, FILE_END);

        SYSTEMTIME st;
        GetLocalTime(&st);
        char szWhole[32];
        wsprintfA(szWhole, "%02i:%02i:%02i.%03i ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        WriteFile(hLogFile, szWhole, lstrlenA(szWhole), &dwSize, NULL);

        WriteFile(hLogFile, asMessage, lstrlenA(asMessage), &dwSize, NULL);

        CloseHandle(hLogFile);
    }
}
#endif
#ifdef _DEBUG
char gsz_MDEBUG_TRAP_MSG[3000];
char gsz_MDEBUG_TRAP_MSG_APPEND[2000];
HWND gh_MDEBUG_TRAP_PARENT_WND = NULL;
int __stdcall _MDEBUG_TRAP(LPCSTR asFile, int anLine)
{
    wsprintfA(gsz_MDEBUG_TRAP_MSG, "MDEBUG_TRAP\r\n%s(%i)\r\n", asFile, anLine);
    if (gsz_MDEBUG_TRAP_MSG_APPEND[0])
        lstrcatA(gsz_MDEBUG_TRAP_MSG,gsz_MDEBUG_TRAP_MSG_APPEND);
    MessageBoxA(ghWnd,gsz_MDEBUG_TRAP_MSG,"MDEBUG_TRAP",MB_OK|MB_ICONSTOP);
    return 0;
}
int MDEBUG_CHK = TRUE;
#endif


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifndef _DEBUG
    klInit();
#else
	//MessageBox(0,L"Started",L"ConEmu",0);
#endif

    g_hInstance = hInstance;

    pVCon = NULL;
    Title[0]=0; TitleCmp[0]=0;

    bool setParentDisabled=false;
    bool ClearTypePrm = false;
    bool FontPrm = false; TCHAR* FontVal;
    bool SizePrm = false; LONG SizeVal;
    bool BufferHeightPrm = false; int BufferHeightVal;
    bool ConfigPrm = false; TCHAR* ConfigVal;
	bool FontFilePrm = false; TCHAR* FontFile; //ADD fontname; by Mors
	bool WindowPrm = false;

    cBlinkShift = GetCaretBlinkTime()/15;

    memset(&osver, 0, sizeof(osver));
    osver.dwOSVersionInfoSize = sizeof(osver);
    GetVersionEx(&osver);

    //Windows7 - SetParent для консоли валится
    setParent = false; // PictureView теперь идет через Wrapper
    if ((osver.dwMajorVersion>6) || (osver.dwMajorVersion==6 && osver.dwMinorVersion>=1))
    {
        setParentDisabled = true;
    }

//------------------------------------------------------------------------
///| Allocating console |/////////////////////////////////////////////////
//------------------------------------------------------------------------

        
    AllocConsole();
    ghConWnd = GetConsoleWindow();
    /*DWORD dwStyle = 0;
	dwStyle = GetWindowLong(ghConWnd, GWL_STYLE);
	dwStyle &= ~(WS_OVERLAPPEDWINDOW|WS_OVERLAPPED);
	dwStyle |= WS_POPUPWINDOW;
	SetWindowLong(ghConWnd, GWL_STYLE, dwStyle);

	dwStyle = GetWindowLong(ghConWnd, GWL_EXSTYLE);
	dwStyle &= ~(WS_EX_OVERLAPPEDWINDOW|WS_EX_APPWINDOW);
	dwStyle |= WS_EX_TOOLWINDOW;
    SetWindowLong(ghConWnd, GWL_EXSTYLE, dwStyle);*/

	// Если в свойствах ярлыка указано "Максимизировано" - консоль разворачивается, а FAR при 
	// старте сам меняет размер буфера, в результате - ошибочно устанавливается размер окна
	if (nCmdShow != SW_SHOWNORMAL) ShowWindow(ghConWnd, SW_SHOWNORMAL);
    ShowWindow(ghConWnd, SW_HIDE);
    EnableWindow(ghConWnd, FALSE); // NightRoman
    SetConsoleSizeTo(ghConWnd, 4, 6);


//------------------------------------------------------------------------
///| Parsing the command line |///////////////////////////////////////////
//------------------------------------------------------------------------

    TCHAR* cmdNew = NULL;
    //STARTUPINFO stInf; -- Maximus5 - не используется
    //GetStartupInfo(&stInf);

    //Maximus5 - размер командной строки может быть и поболе...
    //_tcsncpy(cmdLine, GetCommandLine(), 0x1000); cmdLine[0x1000-1]=0;
    TCHAR *cmdLine = _tcsdup(GetCommandLine());

    {
        cmdNew = _tcsstr(cmdLine, _T("/cmd"));
        if (cmdNew)
        {
            *cmdNew = 0;
            cmdNew += 5;
        }
    }

    TCHAR *curCommand = cmdLine;
    {
#ifdef KL_MEM
        uint params = klSplitCommandLine(curCommand);
#else
        uint params; klSplitCommandLine(curCommand, &params);
#endif

        if(params < 2)
            curCommand = NULL;

        // Parse parameters.
        // Duplicated parameters are permitted, the first value is used.
        for (uint i = 1; i < params; i++)
        {
            curCommand += _tcslen(curCommand) + 1;
            if ( !klstricmp(curCommand, _T("/ct")) || !klstricmp(curCommand, _T("/cleartype")) )
            {
                ClearTypePrm = true;
            }
            else if ( !klstricmp(curCommand, _T("/font")) && i + 1 < params)
            {
                curCommand += _tcslen(curCommand) + 1;
                if (!FontPrm)
                {
                    FontPrm = true;
                    FontVal = curCommand;
                }
            }
            else if ( !klstricmp(curCommand, _T("/size")) && i + 1 < params)
            {
                curCommand += _tcslen(curCommand) + 1;
                if (!SizePrm)
                {
                    SizePrm = true;
                    SizeVal = klatoi(curCommand);
                }
            }
            //Start ADD fontname; by Mors
            else if ( !klstricmp(curCommand, _T("/fontfile")) && i + 1 < params)
            {
                curCommand += _tcslen(curCommand) + 1;
                if (!FontFilePrm)
                {
                    FontFilePrm = true;
                    FontFile = curCommand;
                }
            }
            //End ADD fontname; by Mors
            else if ( !klstricmp(curCommand, _T("/fs")))
            {
                WindowMode = rFullScreen; WindowPrm = true;
            }
            else if ( !klstricmp(curCommand, _T("/DontSetParent")) || !klstricmp(curCommand, _T("/Windows7")) )
            {
                setParentDisabled = true;
            }
            else if ( !klstricmp(curCommand, _T("/SetParent")) )
            {
                setParent = true;
            }
            else if (!klstricmp(curCommand, _T("/BufferHeight")) && i + 1 < params)
            {
                curCommand += _tcslen(curCommand) + 1;
                if (!BufferHeightPrm)
                {
                    BufferHeightPrm = true;
                    BufferHeightVal = klatoi(curCommand);
                    if (BufferHeightVal < 0)
                    {
                        //setParent = true; -- Maximus5 - нефиг, все ручками
                        BufferHeightVal = -BufferHeightVal;
                    }
                }
            }
            else if (!klstricmp(curCommand, _T("/Config")) && i + 1 < params)
            {
                curCommand += _tcslen(curCommand) + 1;
                if (!ConfigPrm)
                {
                    ConfigPrm = true;
                    const int maxConfigNameLen = 127;
                    int nLen = _tcslen(curCommand);
                    if (nLen > maxConfigNameLen)
                    {
	                    TCHAR* psz=(TCHAR*)calloc(nLen+100,sizeof(TCHAR));
	                    wsprintf(psz, _T("Too long /Config name (%i chars).\r\n"), nLen);
	                    _tcscat(psz, curCommand);
                        MBoxA(psz);
                        free(psz);
                        return -1;
                    }
                    ConfigVal = curCommand;
                }
            }
            else if ( !klstricmp(curCommand, _T("/?")))
            {
                MessageBoxA(NULL, pHelp, "About ConEmu...", MB_ICONQUESTION);
                return -1; // NightRoman
            }
        }
    }
    if (setParentDisabled && setParent)
        setParent=false;
    
//------------------------------------------------------------------------
///| Create VirtualConsole, load settings and apply parameters |//////////
//------------------------------------------------------------------------

#undef pVCon
    // create console
    pVCon = new VirtualConsole(0);

    // set config name before settings (i.e. where to load from)
    if (ConfigPrm)
    {
        _tcscat(pVCon->Config, _T("\\"));
        _tcscat(pVCon->Config, ConfigVal);
    }

    if (FontFilePrm) AddFontResourceEx(FontFile, FR_PRIVATE, NULL); //ADD fontname; by Mors

    // load
    LoadSettings();

    if (gSet.isConVisible) {
        ShowWindow(ghConWnd, SW_SHOWNA);
        EnableWindow(ghConWnd, true);
    }

    // set other parameters after settings (i.e. parameters overwrite default)
    if (cmdNew)
        _tcscpy(gSet.Cmd, cmdNew);
    if (ClearTypePrm)
        pVCon->LogFont.lfQuality = CLEARTYPE_NATURAL_QUALITY;
    if (FontPrm)
        _tcscpy(pVCon->LogFont.lfFaceName, FontVal);
    if (SizePrm)
        pVCon->LogFont.lfHeight = SizeVal;
    if (BufferHeightPrm)
        pVCon->BufferHeight = BufferHeightVal;
	if (!WindowPrm) {
		if (nCmdShow == SW_SHOWMAXIMIZED)
			WindowMode = rMaximized;
	}

    // set quick edit mode for buffer mode
    if (pVCon->BufferHeight > 0)
    {
        DWORD mode;
        if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) && (mode & 0x0040) == 0)
            SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), (mode | 0x0040));
    }

//------------------------------------------------------------------------
///| Creating window |////////////////////////////////////////////////////
//------------------------------------------------------------------------

    RECT cRect;
    GetWindowRect(ghConWnd, &cRect);

    //!!!ICON
    //TCHAR* lpszFilePart=NULL;
    //if (GetFullPathName(_T(".\\ConEmu.ico"), MAX_PATH, szIconPath, &lpszFilePart)>0)
    if (GetModuleFileName(0, szIconPath, MAX_PATH))
    {
	    TCHAR *lpszExt = _tcsrchr(szIconPath, _T('.'));
	    if (!lpszExt)
		    szIconPath[0] = 0;
		else {
			_tcscpy(lpszExt, _T(".ico"));
	        DWORD dwAttr = GetFileAttributes(szIconPath);
	        if (dwAttr==-1 || (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
	            szIconPath[0]=0;
	    }
    } else {
        szIconPath[0]=0;
    }
    
    HICON hClassIcon = NULL, hClassIconSm = NULL;
    if (szIconPath[0]) {
	    hClassIcon = (HICON)LoadImage(0, szIconPath, IMAGE_ICON, 
		    GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR|LR_LOADFROMFILE);
	    hClassIconSm = (HICON)LoadImage(0, szIconPath, IMAGE_ICON, 
		    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR|LR_LOADFROMFILE);
	}
    if (!hClassIcon) {
	    //hClassIcon = LoadIcon(GetModuleHandle(NULL), (LPCTSTR)MAKEINTRESOURCE(gSet.nIconID)/*IDI_ICON1*/);
	    hClassIcon = (HICON)LoadImage(GetModuleHandle(0), 
		    MAKEINTRESOURCE(gSet.nIconID), IMAGE_ICON, 
		    GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	    if (hClassIconSm) DestroyIcon(hClassIconSm);
	    hClassIconSm = (HICON)LoadImage(GetModuleHandle(0), 
		    MAKEINTRESOURCE(gSet.nIconID), IMAGE_ICON, 
		    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		//hClassIconSm = LoadIcon(GetModuleHandle(NULL), (LPCTSTR)MAKEINTRESOURCE(gSet.nIconID)/*IDI_ICON1*/);
    }
    WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_DBLCLKS, WndProc, 0, 0, 
		    g_hInstance, hClassIcon, LoadCursor(NULL, IDC_ARROW), 
		    NULL /*(HBRUSH)COLOR_BACKGROUND/**/, 
		    NULL, szClassName, hClassIconSm};// | CS_DROPSHADOW
	if (gbUseChildWindow) wc.lpszClassName = szClassNameParent;
    if (!RegisterClassEx(&wc))
        return -1;
    //ghWnd = CreateWindow(szClassName, 0, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, gSet.wndX, gSet.wndY, cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4, NULL, NULL, (HINSTANCE)g_hInstance, NULL);
    DWORD style = (pVCon->BufferHeight && !gbUseChildWindow) ? WS_VSCROLL : 0 ;
    style |= WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	ghWnd = CreateWindow(gbUseChildWindow ? szClassNameParent : szClassName, 0, style, gSet.wndX, gSet.wndY, cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4, NULL, NULL, (HINSTANCE)g_hInstance, NULL);
	if (!ghWnd) {
		if (!ghWndDC) MBoxA(_T("Can't create main window!"));
        return -1;
	}

    // set parent window of the console window:
    // *) it is used by ConMan and some FAR plugins, set it for standard mode or if /SetParent switch is set
    // *) do not set it by default for buffer mode because it causes unwanted selection jumps
    // WARP ItSelf опытным путем выяснил, что SetParent валит ConEmu в Windows7
    //if (!setParentDisabled && (setParent || pVCon->BufferHeight == 0))
    if (setParent)
        SetParent(ghConWnd, HDCWND); // в сборке alex_itd выполнялся всегда

    HMENU hwndMain = GetSystemMenu(ghWnd, FALSE);
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_TOTRAY, _T("Hide to &tray"));
    InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_HELP, _T("&About"));
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_CONPROP, _T("&Properties..."));
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_SETTINGS, _T("S&ettings..."));

    // adjust the console window and buffer to settings
    // *) after getting all the settings
    // *) before running the command
    COORD size = {gSet.wndWidth, gSet.wndHeight};
    SetConsoleWindowSize(size, false); // NightRoman
    
    if (gSet.isTabs==1)
	    ForceShowTabs();
    
//------------------------------------------------------------------------
///| создадим pipe для общения с плагином |///////////////////////////////
//------------------------------------------------------------------------

    WCHAR pipename[MAX_PATH];
    //Maximus5 - теперь имя пайпа - по ИД процесса FAR'а
    wsprintf(pipename, _T("\\\\.\\pipe\\ConEmuP%u"), /*ghWnd*/ ghConWnd );
    hPipe = CreateNamedPipe( 
      pipename,             // pipe name 
      PIPE_ACCESS_DUPLEX,       // read/write access 
      PIPE_TYPE_MESSAGE |       // message type pipe 
      PIPE_READMODE_MESSAGE |   // message-read mode 
      PIPE_WAIT,                // blocking mode 
      PIPE_UNLIMITED_INSTANCES, // max. instances  
      100,                  // output buffer size 
      100,                  // input buffer size 
      0,                        // client time-out 
      NULL);                    // default security attribute 
    if (hPipe == INVALID_HANDLE_VALUE) 
    {
      MessageBox(ghWnd, _T("CreatePipe failed"), NULL, 0); 
      return 100;
    }
    wsprintf(pipename, _T("ConEmuPEvent%u"), /*ghWnd*/ ghConWnd );
    hPipeEvent = CreateEvent(NULL, TRUE, FALSE, pipename);
    if ((hPipeEvent==NULL) || (hPipeEvent==INVALID_HANDLE_VALUE))
    {
      CloseHandle(hPipe);
      MessageBox(ghWnd, _T("CreatePipe failed"), NULL, 0); 
      return 100;
    }
    // Установить переменную среды с дескриптором окна
    wsprintf(pipename, L"0x%08x", HDCWND);
    SetEnvironmentVariable(L"ConEmuHWND", pipename);
    

//------------------------------------------------------------------------
///| Starting the process |///////////////////////////////////////////////
//------------------------------------------------------------------------
    
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    SetHandleInformation(GetStdHandle(STD_INPUT_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(GetStdHandle(STD_OUTPUT_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(GetStdHandle(STD_ERROR_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

    if (*gSet.Cmd)
    {
        if (!CreateProcess(NULL, gSet.Cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
            //MBoxA("Cannot execute the command.");
	        DWORD dwLastError = GetLastError();
            int nLen = _tcslen(gSet.Cmd);
	        TCHAR* pszErr=(TCHAR*)calloc(nLen+100,sizeof(TCHAR));
	        
		    if (0==FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		        NULL, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		        pszErr, 1024, NULL))
		    {
			    wsprintf(pszErr, _T("Unknown system error: 0x%x"), dwLastError);
		    }
	        
		    nLen += _tcslen(pszErr);
	        TCHAR* psz=(TCHAR*)calloc(nLen+100,sizeof(TCHAR));
	        
	        _tcscpy(psz, _T("Cannot execute the command.\r\n"));
	        _tcscat(psz, pszErr);
	        if (psz[_tcslen(psz)-1]!=_T('\n')) _tcscat(psz, _T("\r\n"));
	        _tcscat(psz, gSet.Cmd);
	        _tcscat(psz, _T("\r\n\r\n"));
	        _tcscat(psz, pVCon->BufferHeight == 0 ? _T("Do You want to simply start far?") : _T("Do You want to simply start cmd?"));
	        //MBoxA(psz);
	        int nBrc = MessageBox(NULL, psz, _T("ConEmu"), MB_YESNO|MB_ICONEXCLAMATION|MB_SETFOREGROUND);
	        free(psz); free(pszErr);
	        if (nBrc!=IDYES)
	            return -1;
	        *gSet.Cmd = 0; // Выполнить стандартную команду...
        }
    }
    
    if (!*gSet.Cmd)
    {
        // *) simple mode: try to start FAR and then cmd;
        // *) buffer mode: FAR is nonsense, try to start cmd only.
        _tcscpy(temp, pVCon->BufferHeight == 0 ? _T("far") : _T("cmd"));
        if (!CreateProcess(NULL, temp, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi) && pVCon->BufferHeight == 0)
        {
            _tcscpy(temp, _T("cmd"));
            if (!CreateProcess(NULL, temp, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
                //MBoxA("Cannot start Far or Cmd.");
		        DWORD dwLastError = GetLastError();
	            int nLen = _tcslen(gSet.Cmd);
		        TCHAR* pszErr=(TCHAR*)calloc(nLen+100,sizeof(TCHAR));
		        
			    if (0==FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			        NULL, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			        pszErr, 1024, NULL))
			    {
				    wsprintf(pszErr, _T("Unknown system error: 0x%x"), dwLastError);
			    }
		        
			    nLen += _tcslen(pszErr);
		        TCHAR* psz=(TCHAR*)calloc(nLen+100,sizeof(TCHAR));
		        
		        _tcscpy(psz, _T("Cannot start Far or Cmd.\r\n"));
		        _tcscat(psz, pszErr);
		        if (psz[_tcslen(psz)-1]!=_T('\n')) _tcscat(psz, _T("\r\n"));
		        _tcscat(psz, gSet.Cmd);
		        MBoxA(psz);
		        free(psz); free(pszErr);
	            return -1;
            }
        }
    }

    CloseHandle(pi.hThread); pi.hThread = NULL;
    CloseHandle(pi.hProcess); pi.hProcess = NULL;
    //hChildProcess = pi.hProcess;

//------------------------------------------------------------------------
///| Misc |///////////////////////////////////////////////////////////////
//------------------------------------------------------------------------

    OleInitialize (NULL); // как бы попробовать включать Ole только во время драга. кажется что из-за него глючит переключалка языка

    SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine, true);
    SetTimer(ghWnd, 0, 10, NULL);

    Registry reg;
    if (reg.OpenKey(_T("Control Panel\\Desktop"), KEY_READ))
    {
        reg.Load(_T("DragFullWindows"), &isNotFullDrag);
        reg.CloseKey();
    }

    
    /*PipeCmd cmd=SetConEmuHwnd;
    DWORD cbWritten=0;
    WriteFile(hPipe, &cmd, sizeof(cmd), &cbWritten, NULL); 
    //SetEvent(hPipeEvent);
    WriteFile(hPipe, &ghWnd, sizeof(ghWnd), &cbWritten, NULL);*/
    

    DragDrop=new CDragDrop(HDCWND);
    ProgressBars=new CProgressBars(ghWnd, g_hInstance);

    isRBDown=false;
    isLBDown=false;

    SetForegroundWindow(ghWnd);

    SetParent(ghWnd, GetParent(GetShellWindow()));
    GetCWShift(ghWnd, &cwShift);
    GetCWShift(ghConWnd, &cwConsoleShift);
    pVCon->InitDC();
    SyncWindowToConsole();

    SetWindowMode(WindowMode);

    MSG lpMsg;
    while (GetMessage(&lpMsg, NULL, 0, 0))
    {
        TranslateMessage(&lpMsg);
        DispatchMessage(&lpMsg);
    }
    KillTimer(ghWnd, 0);
    delete pVCon;
    //CloseHandle(hChildProcess); -- он более не требуется

    if (FontFilePrm) RemoveFontResourceEx(FontFile, FR_PRIVATE, NULL); //ADD fontname; by Mors

    FreeConsole();
    return 0;
}