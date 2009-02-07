#include "Header.h"

CConEmuMain::CConEmuMain()
{
	gnLastProcessCount=0;
	cBlinkNext=0;
	WindowMode=0;
	hPipe=NULL;
	hPipeEvent=NULL;
	isWndNotFSMaximized=false;
	isShowConsole=false;
	isNotFullDrag=false;
	isLBDown=false;
	isDragProcessed=false;
	isRBDown=false;
	ibSkilRDblClk=false; 
	dwRBDownTick=0;
	isPiewUpdate = false; //true; --Maximus5
	gbPostUpdateWindowSize = false;
	hPictureView = NULL; 
	bPicViewSlideShow = false; 
	dwLastSlideShowTick = 0;
	gb_ConsoleSelectMode=false;
	setParent = false;
	RBDownNewX=0; RBDownNewY=0;
	cursor.x=0; cursor.y=0; Rcursor=cursor;
	lastMMW=-1;
	lastMML=-1;
	DragDrop=NULL;
	ProgressBars=NULL;
	cBlinkShift=0;
	Title[0]=0; TitleCmp[0]=0;
}

CConEmuMain::~CConEmuMain()
{
}

// returns difference between window size and client area size of inWnd in outShift->x, outShift->y
void CConEmuMain::GetCWShift(HWND inWnd, POINT *outShift)
{
    RECT cRect, wRect;
    GetClientRect(inWnd, &cRect); // The left and top members are zero. The right and bottom members contain the width and height of the window.
    GetWindowRect(inWnd, &wRect); // screen coordinates of the upper-left and lower-right corners of the window
    outShift->x = wRect.right  - wRect.left - cRect.right;
    outShift->y = wRect.bottom - wRect.top  - cRect.bottom;
}

void CConEmuMain::ShowSysmenu(HWND Wnd, HWND Owner, int x, int y)
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

RECT CConEmuMain::ConsoleOffsetRect()
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

RECT CConEmuMain::DCClientRect(RECT* pClient/*=NULL*/)
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

void CConEmuMain::SyncWindowToConsole()
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
// returns console size in columns and lines calculated from current window size
// rectInWindow - если true - с рамкой, false - только клиент
COORD CConEmuMain::ConsoleSizeFromWindow(RECT* arect /*= NULL*/, bool frameIncluded /*= false*/, bool alreadyClient /*= false*/)
{
    COORD size;

	if (!gSet.LogFont.lfWidth || !gSet.LogFont.lfHeight) {
		// размер шрифта еще не инициализирован! вернем текущий размер консоли! TODO:
		CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
		GetConsoleScreenBufferInfo(pVCon->hConOut(), &inf);
		size = inf.dwSize;
		return size; 
	}

    RECT rect, consoleRect;
    if (arect == NULL)
    {
        GetClientRect(HDCWND, &rect);
		if (gbUseChildWindow)
			memset(&consoleRect, 0, sizeof(consoleRect));
		else
			consoleRect = ConsoleOffsetRect();
    } 
    else
    {
        rect = *arect;
		if (alreadyClient)
			memset(&consoleRect, 0, sizeof(consoleRect));
		else
			consoleRect = ConsoleOffsetRect();
    }
    
    size.X = (rect.right - rect.left - (frameIncluded ? cwShift.x : 0) - consoleRect.left) / gSet.LogFont.lfWidth;
    size.Y = (rect.bottom - rect.top - (frameIncluded ? cwShift.y : 0) - consoleRect.top) / gSet.LogFont.lfHeight;
    #ifdef MSGLOGGER
        char szDbg[100]; wsprintfA(szDbg, "   ConsoleSizeFromWindow={%i,%i}\n", size.X, size.Y);
        DEBUGLOGFILE(szDbg);
    #endif
    return size;
}

// return window size in pixels calculated from console size
RECT CConEmuMain::WindowSizeFromConsole(COORD consoleSize, bool rectInWindow /*= false*/, bool clientOnly /*= false*/)
{
    RECT rect;
    rect.top = 0;   
    rect.left = 0;
    RECT offsetRect;
	if (clientOnly)
		memset(&offsetRect, 0, sizeof(RECT));
	else
		offsetRect = ConsoleOffsetRect();
    rect.bottom = consoleSize.Y * gSet.LogFont.lfHeight + (rectInWindow ? cwShift.y : 0) + offsetRect.top;
    rect.right = consoleSize.X * gSet.LogFont.lfWidth + (rectInWindow ? cwShift.x : 0) + offsetRect.left;
    #ifdef MSGLOGGER
        char szDbg[100]; wsprintfA(szDbg, "   WindowSizeFromConsole={%i,%i}\n", rect.right,rect.bottom);
        DEBUGLOGFILE(szDbg);
    #endif
    return rect;
}

// size in columns and lines
void CConEmuMain::SetConsoleWindowSize(const COORD& size, bool updateInfo)
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
    if (gSet.BufferHeight == 0)
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
        csbi.dwSize.Y = max(gSet.BufferHeight, size.Y);
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

void CConEmuMain::SyncConsoleToWindow()
{
   DEBUGLOGFILE("SyncConsoleToWindow\n");
   SetConsoleWindowSize(ConsoleSizeFromWindow(), true);
}

bool CConEmuMain::SetWindowMode(uint inMode)
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
         LONG style = gSet.BufferHeight ? WS_VSCROLL : 0; // NightRoman
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

            LONG style = gSet.BufferHeight ? WS_VSCROLL : 0;
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
bool CConEmuMain::isPictureView()
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

bool CConEmuMain::isFilePanel()
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

bool CConEmuMain::isConSelectMode()
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

void CConEmuMain::ForceShowTabs(BOOL abShow)
{
	if (!pVCon)
		return;

	BOOL lbTabsAllowed = abShow && TabBar.IsAllowed();

    if (abShow /*&& !TabBar.IsActive()*/ && gSet.isTabs && lbTabsAllowed)
    {
        TabBar.Activate();
		ConEmuTab tab; memset(&tab, 0, sizeof(tab));
		tab.Pos=0;
		tab.Current=1;
		tab.Type = 1;
		TabBar.Update(&tab, 1);
		gbPostUpdateWindowSize = true;
	} else if (!abShow) {
		TabBar.Deactivate();
		gbPostUpdateWindowSize = true;
	}

	if (gbPostUpdateWindowSize) { // значит мы что-то поменяли
		if (gbUseChildWindow) {
	        RECT rcNewCon; GetClientRect(ghWnd, &rcNewCon);
			DCClientRect(&rcNewCon);
	        MoveWindow(ghWndDC, rcNewCon.left, rcNewCon.top, rcNewCon.right - rcNewCon.left, rcNewCon.bottom - rcNewCon.top, 0);
	        dcWindowLast = rcNewCon;
	    }
		
	    if (gSet.LogFont.lfWidth)
	    {
	        SyncConsoleToWindow();
	        //RECT rc = ConsoleOffsetRect();
	        //rc.bottom = rc.top; rc.top = 0;
	        if (!gbUseChildWindow) {
		        InvalidateRect(ghWnd, NULL, FALSE);
	        }
		}
    }
}

void CConEmuMain::PaintGaps(HDC hDC/*=NULL*/)
{
	BOOL lbOurDc = (hDC==NULL);
    
	if (hDC==NULL)
		hDC = GetDC(ghWnd); // Главное окно!

	HBRUSH hBrush = CreateSolidBrush(gSet.Colors[0]); SelectObject(hDC, hBrush);

	RECT rcClient;
	GetClientRect(ghWnd, &rcClient); // Клиентская часть главного окна

	WINDOWPLACEMENT wpl; memset(&wpl, 0, sizeof(wpl)); wpl.length = sizeof(wpl);
	GetWindowPlacement(ghWndDC, &wpl); // Положение окна, в котором идет отрисовка

	RECT offsetRect = ConsoleOffsetRect(); // смещение с учетом табов

	// paint gaps between console and window client area with first color

	RECT rect;

	// top
	rect = rcClient;
	rect.top += offsetRect.top;
	rect.bottom = wpl.rcNormalPosition.top;
	if (!IsRectEmpty(&rect))
		FillRect(hDC, &rect, hBrush);
#ifdef _DEBUG
	GdiFlush();
#endif

	// right
	rect.left = wpl.rcNormalPosition.right;
	rect.bottom = rcClient.bottom;
	if (!IsRectEmpty(&rect))
		FillRect(hDC, &rect, hBrush);
#ifdef _DEBUG
	GdiFlush();
#endif

	// left
	rect.left = 0;
	rect.right = wpl.rcNormalPosition.left;
	rect.bottom = rcClient.bottom;
	if (!IsRectEmpty(&rect))
		FillRect(hDC, &rect, hBrush);
#ifdef _DEBUG
	GdiFlush();
#endif

	// bottom
	rect.left = 0;
	rect.right = rcClient.right;
	rect.top = wpl.rcNormalPosition.bottom;
	rect.bottom = rcClient.bottom;
	if (!IsRectEmpty(&rect))
		FillRect(hDC, &rect, hBrush);
#ifdef _DEBUG
	GdiFlush();
#endif

	if (lbOurDc)
		ReleaseDC(ghWnd, hDC);
}

void CConEmuMain::CheckBufferSize()
{
	// Высота буфера могла измениться после смены списка процессов
	BOOL  lbProcessChanged = FALSE;
    DWORD ProcList[2], ProcCount=0;
    ProcCount = GetConsoleProcessList(ProcList,2);
	if (gnLastProcessCount && ProcCount!=gnLastProcessCount)
		lbProcessChanged = TRUE;
	gnLastProcessCount = ProcCount;


    CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
    GetConsoleScreenBufferInfo(pVCon->hConOut(), &inf);
    if (inf.dwSize.X>(inf.srWindow.Right-inf.srWindow.Left+1)) {
        DEBUGLOGFILE("Wrong screen buffer width\n");
		// Окошко консоли почему-то схлопнулось по горизонтали
        MOVEWINDOW(ghConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
    } else if ((gSet.BufferHeight == 0) && (inf.dwSize.Y>(inf.srWindow.Bottom-inf.srWindow.Top+1))) {
        DEBUGLOGFILE("Wrong screen buffer height\n");
		// Окошко консоли почему-то схлопнулось по вертикали
        MOVEWINDOW(ghConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
    }

	// При выходе из FAR -> CMD с BufferHeight - смена QuickEdit режима
	DWORD mode = 0;
	BOOL lb = FALSE;
	if (gSet.BufferHeight) {
		lb = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);

		if (inf.dwSize.Y>(inf.srWindow.Bottom-inf.srWindow.Top+1)) {
			// Буфер больше высоты окна
			mode |= ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE|ENABLE_EXTENDED_FLAGS;
		} else {
			// Буфер равен высоте окна (значит ФАР запустился)
			mode &= ~(ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE);
			mode |= ENABLE_EXTENDED_FLAGS;
		}

		lb = SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
	}
}

LRESULT CALLBACK CConEmuMain::WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
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
		result = gConEmu.OnCopyData(cds);
        break;
    }
    
    case WM_ERASEBKGND:
		return 0;
		
	case WM_PAINT:
		result = gConEmu.OnPaint(wParam, lParam);
	
    case WM_TIMER:
		result = gConEmu.OnTimer(wParam, lParam);
        break;

    case WM_SIZING:
		result = gConEmu.OnSizing(wParam, lParam);
        break;

	case WM_SIZE:
		result = gConEmu.OnSize(wParam, lParam);
        break;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pInfo = (LPMINMAXINFO)lParam;
			result = gConEmu.OnGetMinMaxInfo(pInfo);
			break;
		}

    case WM_KEYDOWN:
    case WM_KEYUP:

        if (wParam == VK_PAUSE && !isPressed(VK_CONTROL)) {
            if (gConEmu.isPictureView()) {
                if (messg == WM_KEYUP) {
                    gConEmu.bPicViewSlideShow = !gConEmu.bPicViewSlideShow;
                    if (gConEmu.bPicViewSlideShow) {
                        if (gSet.nSlideShowElapse<=500) gSet.nSlideShowElapse=500;
                        //SetTimer(hWnd, 3, gSet.nSlideShowElapse, NULL);
                        gConEmu.dwLastSlideShowTick = GetTickCount() - gSet.nSlideShowElapse;
                    //} else {
                    //  KillTimer(hWnd, 3);
                    }
                }
                break;
            }
        } else if (gConEmu.bPicViewSlideShow) {
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
                gConEmu.bPicViewSlideShow = false; // отмена слайдшоу
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
        if (gSet.BufferHeight && messg == WM_KEYDOWN && isPressed(VK_CONTROL))
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
                gConEmu.isShowConsole = true;
                ShowWindow(ghConWnd, SW_SHOWNORMAL);
                if (gConEmu.setParent) SetParent(ghConWnd, 0);
                EnableWindow(ghConWnd, true);
            }
            else
            {
                gConEmu.isShowConsole = false;
                if (!gSet.isConVisible) ShowWindow(ghConWnd, SW_HIDE);
                if (gConEmu.setParent) SetParent(ghConWnd, HDCWND);
                if (!gSet.isConVisible) EnableWindow(ghConWnd, false);
            }
            break;
        }

        if (gConEmu.gb_ConsoleSelectMode && messg == WM_KEYDOWN && ((wParam == VK_ESCAPE) || (wParam == VK_RETURN)))
            gConEmu.gb_ConsoleSelectMode = false; //TODO: может как-то по другому определять?

        // Основная обработка 
        {
            if (messg == WM_SYSKEYDOWN) 
                if (wParam == VK_INSERT && lParam & 29)
                    gConEmu.gb_ConsoleSelectMode = true;

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
                        gConEmu.SetWindowMode(rFullScreen);
                    else
                        gConEmu.SetWindowMode(gConEmu.isWndNotFSMaximized ? rMaximized : rNormal);

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
                gConEmu.ShowSysmenu(ghWnd, ghWnd, rect.right - cRect.right - wInfo.cxWindowBorders, rect.bottom - cRect.bottom - wInfo.cyWindowBorders);
            }
            else if (messg == WM_KEYUP && wParam == VK_MENU && isSkipNextAltUp) isSkipNextAltUp = false;
            else if (messg == WM_SYSKEYDOWN && wParam == VK_F9 && lParam & 29 && !isPressed(VK_SHIFT))
                gConEmu.SetWindowMode(IsZoomed(ghWnd) ? rNormal : rMaximized);
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
			consoleRect = gConEmu.ConsoleOffsetRect();
        winX -= consoleRect.left;
        winY -= consoleRect.top;
        short newX = MulDiv(winX, conRect.right, klMax<uint>(1, pVCon->Width));
        short newY = MulDiv(winY, conRect.bottom, klMax<uint>(1, pVCon->Height));

        if (newY<0 || newX<0)
            break;

        if (gSet.BufferHeight)
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
            if (wParam==gConEmu.lastMMW && lParam==gConEmu.lastMML) {
                break;
            }
            gConEmu.lastMMW=wParam; gConEmu.lastMML=lParam;

            /*if (isLBDown &&   (cursor.x-LOWORD(lParam)>DRAG_DELTA || 
                               cursor.x-LOWORD(lParam)<-DRAG_DELTA || 
                               cursor.y-HIWORD(lParam)>DRAG_DELTA || 
                               cursor.y-HIWORD(lParam)<-DRAG_DELTA))*/
            if (gConEmu.isLBDown && !PTDIFFTEST(gConEmu.cursor,DRAG_DELTA) 
				&& !gConEmu.isDragProcessed)
            {
	            // Если сначала фокус был на файловой панели, но после LClick он попал на НЕ файловую - отменить ShellDrag
	            if (!gConEmu.isFilePanel()) {
		            gConEmu.isLBDown = false;
		            break;
	            }
                // Иначе иногда срабатывает FAR'овский D'n'D
                //SENDMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ));     //посылаем консоли отпускание
				if (gConEmu.DragDrop)
					gConEmu.DragDrop->Drag(); //сдвинулись при зажатой левой
				//isDragProcessed=false; -- убрал, иначе при бросании в пассивную панель больших файлов дроп может вызваться еще раз???
                POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание
                break;
            }
            else if (gSet.isRClickSendKey && gConEmu.isRBDown)
            {
                //Если двинули мышкой, а была включена опция RClick - не вызывать
                //контекстное меню - просто послать правый клик
                if (!PTDIFFTEST(gConEmu.Rcursor, RCLICKAPPSDELTA))
                {
                    gConEmu.isRBDown=false;
                    POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, 0, MAKELPARAM( gConEmu.RBDownNewX, gConEmu.RBDownNewY ), TRUE);
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
            gConEmu.lastMMW=-1; gConEmu.lastMML=-1;

            if (messg == WM_LBUTTONDOWN)
            {
                if (gConEmu.isLBDown) ReleaseCapture(); // Вдруг сглючило?
                gConEmu.isLBDown=false;
                if (!gConEmu.isConSelectMode() && gConEmu.isFilePanel())
                {
                    SetCapture(ghWnd);
                    gConEmu.cursor.x = LOWORD(lParam);
                    gConEmu.cursor.y = HIWORD(lParam); 
                    gConEmu.isLBDown=true;
                    gConEmu.isDragProcessed=false;
                    POSTMESSAGE(ghConWnd, messg, wParam, MAKELPARAM( newX, newY ), FALSE); // было SEND
                    break;
                }
            }
            else if (messg == WM_LBUTTONUP)
            {
                if (gConEmu.isLBDown) {
                    gConEmu.isLBDown=false;
                    ReleaseCapture();
                }
            }
            else if (messg == WM_RBUTTONDOWN)
            {
                gConEmu.Rcursor.x = LOWORD(lParam);
                gConEmu.Rcursor.y = HIWORD(lParam);
                gConEmu.RBDownNewX=newX;
                gConEmu.RBDownNewY=newY;
                gConEmu.isRBDown=false;

                // Если ничего лишнего не нажато!
                if (gSet.isRClickSendKey && !(wParam&(MK_CONTROL|MK_LBUTTON|MK_MBUTTON|MK_SHIFT|MK_XBUTTON1|MK_XBUTTON2)))
                {
                    //TCHAR *BrF = _tcschr(Title, '{'), *BrS = _tcschr(Title, '}'), *Slash = _tcschr(Title, '\\');
                    //if (BrF && BrS && Slash && BrF == Title && (Slash == Title+1 || Slash == Title+3))
                    if (gConEmu.isFilePanel()) // Maximus5
                    {
                        //заведем таймер на .3
                        //если больше - пошлем apps
                        gConEmu.isRBDown=true; gConEmu.ibSkilRDblClk=false;
                        //SetTimer(hWnd, 1, 300, 0); -- Maximus5, откажемся от таймера
                        gConEmu.dwRBDownTick = GetTickCount();
                        break;
                    }
                }
            }
            else if (messg == WM_RBUTTONUP)
            {
                if (gSet.isRClickSendKey && gConEmu.isRBDown)
                {
                    gConEmu.isRBDown=false; // сразу сбросим!
                    if (PTDIFFTEST(gConEmu.Rcursor,RCLICKAPPSDELTA))
                    {
                        //держали зажатой <.3
                        //убьем таймер, кликнием правой кнопкой
                        //KillTimer(hWnd, 1); -- Maximus5, таймер более не используется
                        DWORD dwCurTick=GetTickCount();
                        DWORD dwDelta=dwCurTick-gConEmu.dwRBDownTick;
                        // Если держали дольше .3с, но не слишком долго :)
                        if ((gSet.isRClickSendKey==1) ||
                            (dwDelta>RCLICKAPPSTIMEOUT && dwDelta<10000))
                        {
                            // Сначала выделить файл под курсором
                            POSTMESSAGE(ghConWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM( gConEmu.RBDownNewX, gConEmu.RBDownNewY ), TRUE);
                            POSTMESSAGE(ghConWnd, WM_LBUTTONUP, 0, MAKELPARAM( gConEmu.RBDownNewX, gConEmu.RBDownNewY ), TRUE);
                        
                            pVCon->Update(true);
                            INVALIDATE(); //InvalidateRect(HDCWND, NULL, FALSE);

                            // А теперь можно и Apps нажать
							gConEmu.ibSkilRDblClk=true; // чтобы пока FAR думает в консоль не проскочило мышиное сообщение
                            POSTMESSAGE(ghConWnd, WM_KEYDOWN, VK_APPS, 0, TRUE);
                            break;
                        }
                    }
                    // Иначе нужно сначала послать WM_RBUTTONDOWN
                    POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, wParam, MAKELPARAM( newX, newY ), TRUE);
                }
                gConEmu.isRBDown=false; // чтобы не осталось случайно
            }
			else if (messg == WM_RBUTTONDBLCLK) {
				if (gConEmu.ibSkilRDblClk) {
					gConEmu.ibSkilRDblClk = false;
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
				WNDCLASS wc = {CS_OWNDC|CS_DBLCLKS|CS_SAVEBITS, CConEmuChild::ChildWndProc, 0, 0, 
						g_hInstance, NULL, LoadCursor(NULL, IDC_ARROW), 
						NULL /*(HBRUSH)COLOR_BACKGROUND*/, 
						NULL, szClassName};// | CS_DROPSHADOW
				if (!RegisterClass(&wc)) {
					ghWndDC = (HWND)-1; // чтобы родитель не ругался
					MBoxA(_T("Can't register DC window class!"));
					return -1;
				}
				DWORD style = WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS /*| WS_CLIPCHILDREN*/ | (gSet.BufferHeight ? WS_VSCROLL : 0);
				RECT rc = gConEmu.DCClientRect();
				ghWndDC = CreateWindow(szClassName, 0, style, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hWnd, NULL, (HINSTANCE)g_hInstance, NULL);
				if (!ghWndDC) {
					ghWndDC = (HWND)-1; // чтобы родитель не ругался
					MBoxA(_T("Can't create DC window!"));
					return -1; //
				}
				gConEmu.dcWindowLast = rc; //TODO!!!
			}
		}
        break;

    case WM_SYSCOMMAND:
        switch(LOWORD(wParam))
        {
        case ID_SETTINGS:
	        if (ghOpWnd && IsWindow(ghOpWnd)) {
		        ShowWindow ( ghOpWnd, SW_SHOWNORMAL );
		        SetFocus ( ghOpWnd );
		        break; // А то открывались несколько окон диалогов :)
		    }
			DialogBox((HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), 0, CSettings::wndOpProc);
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
            gConEmu.SetWindowMode(rMaximized);
            break;
        case SC_RESTORE_SECRET:
            gConEmu.SetWindowMode(rNormal);
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
            gConEmu.ShowSysmenu(hWnd, hWnd, mPos.x, mPos.y);
            PostMessage(hWnd, WM_NULL, 0, 0);
            }
            break;
        } 
        break; 


    case WM_DESTROY:
        Icon.Delete();
        if (gConEmu.DragDrop) {
	        delete gConEmu.DragDrop;
	        gConEmu.DragDrop = NULL;
        }
        if (gConEmu.ProgressBars) {
	        delete gConEmu.ProgressBars;
	        gConEmu.ProgressBars = NULL;
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

BOOL WINAPI CConEmuMain::HandlerRoutine(DWORD dwCtrlType)
{
    return (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT ? true : false);
}


// Нужно вызывать после загрузки настроек!
void CConEmuMain::LoadIcons()
{
    if (hClassIcon)
	    return; // Уже загружены
	    
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
    
    if (szIconPath[0]) {
	    hClassIcon = (HICON)LoadImage(0, szIconPath, IMAGE_ICON, 
		    GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR|LR_LOADFROMFILE);
	    hClassIconSm = (HICON)LoadImage(0, szIconPath, IMAGE_ICON, 
		    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR|LR_LOADFROMFILE);
	}
    if (!hClassIcon) {
	    szIconPath[0]=0;
	    
	    hClassIcon = (HICON)LoadImage(GetModuleHandle(0), 
		    MAKEINTRESOURCE(gSet.nIconID), IMAGE_ICON, 
		    GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
		    
	    if (hClassIconSm) DestroyIcon(hClassIconSm);
	    hClassIconSm = (HICON)LoadImage(GetModuleHandle(0), 
		    MAKEINTRESOURCE(gSet.nIconID), IMAGE_ICON, 
		    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    }
}

LRESULT CConEmuMain::OnPaint(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	if (gbUseChildWindow) {
        PAINTSTRUCT ps;
        HDC hDc = BeginPaint(ghWnd, &ps);

		gConEmu.PaintGaps(hDc);

		EndPaint(ghWnd, &ps);
		result = DefWindowProc(ghWnd, WM_PAINT, wParam, lParam);

	} else {
        if (gConEmu.isPictureView())
        {
			// TODO: если PictureView распахнуто не на все окно - отрисовать видимую часть консоли!
            result = DefWindowProc(ghWnd, WM_PAINT, wParam, lParam);

		} else {
			PAINTSTRUCT ps;
			HDC hDc = BeginPaint(ghWnd, &ps);
			//HDC hDc = GetDC(ghWnd);
			//if (!gbNoDblBuffer)
			{
				RECT rect;
				HBRUSH hBrush = CreateSolidBrush(gSet.Colors[0]); SelectObject(hDc, hBrush);
				GetClientRect(ghWnd, &rect);

				RECT consoleRect = gConEmu.ConsoleOffsetRect();

				#ifdef _DEBUG
				if (gbNoDblBuffer)
					FillRect(hDc, &rect, hBrush); // -- если захочется на "чистую" рисовать
				#else

				// paint gaps between console and window client area with first color (actual for maximized and fullscreen modes)
				rect.top = consoleRect.top; // right 
				rect.left = pVCon->Width + consoleRect.left;
				FillRect(hDc, &rect, hBrush);

				rect.top = pVCon->Height + consoleRect.top; // bottom
				rect.left = 0; 
				rect.right = pVCon->Width + consoleRect.left;
				FillRect(hDc, &rect, hBrush);
				#endif

				DeleteObject(hBrush);

				#ifdef _DEBUG
				if (gbNoDblBuffer)
					pVCon->Update(false, &hDc);
				else
					BitBlt(hDc, consoleRect.left, consoleRect.top, pVCon->Width, pVCon->Height, pVCon->hDC, 0, 0, SRCCOPY);
				#else
				BitBlt(hDc, consoleRect.left, consoleRect.top, pVCon->Width, pVCon->Height, pVCon->hDC, 0, 0, SRCCOPY);
				#endif
			}
			EndPaint(ghWnd, &ps);
			//ReleaseDC(ghWnd, hDc);
			//gbInvalidating = false;
		}
    }

	return result;
}

LRESULT CConEmuMain::OnSize(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

    BOOL lbIsPicView = gConEmu.isPictureView();
    
    if (gbUseChildWindow)
	{
        RECT rcNewCon; memset(&rcNewCon, 0, sizeof(rcNewCon));
		rcNewCon.right = LOWORD(lParam);
		rcNewCon.bottom = HIWORD(lParam);
		gConEmu.DCClientRect(&rcNewCon);
        MoveWindow(ghWndDC, rcNewCon.left, rcNewCon.top, rcNewCon.right - rcNewCon.left, rcNewCon.bottom - rcNewCon.top, 0);
        gConEmu.dcWindowLast = rcNewCon;
    }
    else
    {
        if (lbIsPicView) {
            gConEmu.isPiewUpdate = true;
            RECT rcClient; GetClientRect(HDCWND, &rcClient);
            MoveWindow(gConEmu.hPictureView, 0,0,rcClient.right,rcClient.bottom, 1);
            INVALIDATE(); //InvalidateRect(HDCWND, NULL, FALSE);
            //SetFocus(hPictureView); -- все равно фокус в другой процесс не передастся
        }
    }
    
    if (!lbIsPicView)
    {
        if (gConEmu.isNotFullDrag)
        {
			// только клиентская часть
            RECT pRect = {0, 0, LOWORD(lParam), HIWORD(lParam)};

            COORD srctWindow = gConEmu.ConsoleSizeFromWindow(&pRect);

            if ((gConEmu.srctWindowLast.X != srctWindow.X 
				|| gConEmu.srctWindowLast.Y != srctWindow.Y))
            {
                gConEmu.SetConsoleWindowSize(srctWindow, true);
                gConEmu.srctWindowLast = srctWindow;
            }
        }

        {
            static bool wPrevSizeMax = false;
            if ((wParam == SIZE_MAXIMIZED || (wParam == SIZE_RESTORED && wPrevSizeMax)) && ghConWnd)
            {
                wPrevSizeMax = wParam == SIZE_MAXIMIZED;

                RECT pRect;
                GetWindowRect(ghWnd, &pRect);
                pRect.right = LOWORD(lParam) + pRect.left + gConEmu.cwShift.x;
                pRect.bottom = HIWORD(lParam) + pRect.top + gConEmu.cwShift.y;

                //TODO: есть подозрение, что этот фейк вызывает глюк с левым кликом по рамке...
                // fake WM_SIZING event to adjust console size to new window size after Maximize or Restore Down 
                //SendMessage(ghWnd, WM_SIZING, WMSZ_TOP, (LPARAM)&pRect);
                //SendMessage(ghWnd, WM_SIZING, WMSZ_RIGHT, (LPARAM)&pRect);
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

	return result;
}

LRESULT CConEmuMain::OnSizing(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = true;

#ifdef OnSizingENABLED
    RECT *pRect = (RECT*)lParam; // с рамкой
    COORD srctWindow = gConEmu.ConsoleSizeFromWindow(pRect, true /* rectInWindow */);
    // Минимально допустимые размеры консоли
	if (srctWindow.X<28) srctWindow.X=28;
	if (srctWindow.Y<9)  srctWindow.Y=9;

    if ((gConEmu.srctWindowLast.X != srctWindow.X 
		|| gConEmu.srctWindowLast.Y != srctWindow.Y) 
		&& !gConEmu.isNotFullDrag)
    {
        gConEmu.SetConsoleWindowSize(srctWindow, true);
        gConEmu.srctWindowLast = srctWindow;
    }

    //RECT consoleRect = ConsoleOffsetRect();
    RECT wndSizeRect = gConEmu.WindowSizeFromConsole(srctWindow, true /* rectInWindow */);
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
    //  pRect->right =  srctWindow.X * gSet.LogFont.lfWidth  + cwShift.x + pRect->left;
     pRect->bottom = restrictRect.bottom; // NightRoman
    //  pRect->bottom = srctWindow.Y * gSet.LogFont.lfHeight + cwShift.y + pRect->top;
        break;
    case WMSZ_LEFT:
    case WMSZ_TOP:
    case WMSZ_TOPLEFT:
     pRect->left = restrictRect.left; // NightRoman
    //  pRect->left =  pRect->right - srctWindow.X * gSet.LogFont.lfWidth  - cwShift.x;
     pRect->top = restrictRect.top; // NightRoman
    //  pRect->top  = pRect->bottom - srctWindow.Y * gSet.LogFont.lfHeight - cwShift.y;
        break;
    case WMSZ_TOPRIGHT:
     pRect->right = restrictRect.right; // NightRoman
    //  pRect->right =  srctWindow.X * gSet.LogFont.lfWidth  + cwShift.x + pRect->left;
     pRect->top = restrictRect.top; // NightRoman
    //  pRect->top  = pRect->bottom - srctWindow.Y * gSet.LogFont.lfHeight - cwShift.y;
        break;
    case WMSZ_BOTTOMLEFT:
     pRect->left = restrictRect.left; // NightRoman
    //  pRect->left =  pRect->right - srctWindow.X * gSet.LogFont.lfWidth  - cwShift.x;
     pRect->bottom = restrictRect.bottom; // NightRoman
    //  pRect->bottom = srctWindow.Y * gSet.LogFont.lfHeight + cwShift.y + pRect->top;
        break;
    }
    result = true;
#endif
	return result;
}

LRESULT CConEmuMain::OnTimer(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

    switch (wParam)
    {
    case 0:
        HWND foreWnd = GetForegroundWindow();
        if (!gConEmu.isShowConsole && !gSet.isConVisible)
        {
            /*if (foreWnd == ghConWnd)
                SetForegroundWindow(ghWnd);*/
            if (IsWindowVisible(ghConWnd))
                ShowWindow(ghConWnd, SW_HIDE);
        }

		//Maximus5. Hack - если какая-то зараза задизеблила окно
		DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
		if (dwStyle & WS_DISABLED)
			EnableWindow(ghWnd, TRUE);

		BOOL lbIsPicView = gConEmu.isPictureView();
        if (gConEmu.bPicViewSlideShow) {
            DWORD dwTicks = GetTickCount();
            DWORD dwElapse = dwTicks - gConEmu.dwLastSlideShowTick;
            if (dwElapse > gSet.nSlideShowElapse)
            {
                if (IsWindow(gConEmu.hPictureView)) {
                    //
                    gConEmu.bPicViewSlideShow = false;
                    SendMessage(ghConWnd, WM_KEYDOWN, VK_NEXT, 0x01510001);
                    SendMessage(ghConWnd, WM_KEYUP, VK_NEXT, 0xc1510001);

                    // Окно могло измениться?
                    gConEmu.isPictureView();

                    gConEmu.dwLastSlideShowTick = GetTickCount();
                    gConEmu.bPicViewSlideShow = true;
                } else {
                    gConEmu.hPictureView = NULL;
                    gConEmu.bPicViewSlideShow = false;
                }
            }
        }

        if (gConEmu.cBlinkNext++ >= gConEmu.cBlinkShift)
        {
            gConEmu.cBlinkNext = 0;
            if (foreWnd == ghWnd || foreWnd == ghOpWnd)
                // switch cursor
                pVCon->Cursor.isVisible = !pVCon->Cursor.isVisible;
            else
                // turn cursor off
                pVCon->Cursor.isVisible = false;
        }

        /*DWORD ProcList[2];
        if(GetConsoleProcessList(ProcList,2)==1)
        {
          DestroyWindow(ghWnd);
          break;
        }*/

        GetWindowText(ghConWnd, gConEmu.TitleCmp, 1024);
        if (wcscmp(gConEmu.Title, gConEmu.TitleCmp))
        {
            wcscpy(gConEmu.Title, gConEmu.TitleCmp);
            SetWindowText(ghWnd, gConEmu.Title);
        }

        TabBar.OnTimer();
        gConEmu.ProgressBars->OnTimer();

        
        if (lbIsPicView)
        {
            bool lbOK = true;
            if (!gConEmu.setParent) {
                // Проверка, может PictureView создался в консоли, а не в ConEmu?
                HWND hPicView = FindWindowEx(ghConWnd, NULL, L"FarPictureViewControlClass", NULL);
                if (!hPicView) {
                    lbOK = false; // смысла нет, все равно ничего не увидим
                }
            }
            if (lbOK) {
                gConEmu.isPiewUpdate = true;
                if (pVCon->Update(false))
                    INVALIDATE(); //InvalidateRect(HDCWND, NULL, FALSE);
                break;
            }
        } else 
        if (gConEmu.isPiewUpdate)
        {	// После скрытия/закрытия PictureView нужно передернуть консоль - ее размер мог измениться
            gConEmu.isPiewUpdate = false;
            gConEmu.SyncConsoleToWindow();
            INVALIDATE(); //InvalidateRect(HDCWND, NULL, FALSE);
        }

        // Проверить, может в консоли размер съехал? (хрен его знает из-за чего...)
		gConEmu.CheckBufferSize();
		if (gConEmu.gnLastProcessCount == 1) {
			DestroyWindow(ghWnd);
			break;
		}

        //if (!gbInvalidating && !gbInPaint)
        if (pVCon->Update(false/*gbNoDblBuffer*/))
        {
            COORD c = gConEmu.ConsoleSizeFromWindow();
            if (gConEmu.gbPostUpdateWindowSize 
				|| c.X != pVCon->TextWidth || c.Y != pVCon->TextHeight)
            {
				gConEmu.gbPostUpdateWindowSize = false;
                if (!gSet.isFullScreen && !IsZoomed(ghWnd))
                    gConEmu.SyncWindowToConsole();
                else
                    gConEmu.SyncConsoleToWindow();
            }

            INVALIDATE(); //InvalidateRect(HDCWND, NULL, FALSE);

            // update scrollbar
            if (gSet.BufferHeight)
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
                INVALIDATE(); //InvalidateRect(ghWnd, NULL, FALSE);
            }*/
        }
    }

	return result;
}


LRESULT CConEmuMain::OnCopyData(PCOPYDATASTRUCT cds)
{
	LRESULT result = 0;
    
	if (cds->dwData == 0) {
		gConEmu.ForceShowTabs(FALSE);

		//CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
		//GetConsoleScreenBufferInfo(pVCon->hConOut(), &inf);
		if ((gSet.BufferHeight > 0) /*&& (inf.dwSize.Y==(inf.srWindow.Bottom-inf.srWindow.Top+1))*/)
		{
			DWORD mode = 0;
			BOOL lb = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
			mode |= ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE|ENABLE_EXTENDED_FLAGS;
			lb = SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
		}

	} else {
		BOOL lbNeedInval=FALSE;
		ConEmuTab* tabs = (ConEmuTab*)cds->lpData;
		// Если не активен ИЛИ появлились edit/view ИЛИ табы просили отображать всегда
		// Это сообщение посылает плагин ConEmu при изменениях и входе в FAR
		if (!TabBar.IsActive() && gSet.isTabs && (cds->dwData>1 || tabs[0].Type!=1/*WTYPE_PANELS*/ || gSet.isTabs==1))
		{
			if ((gSet.BufferHeight > 0) /*&& (inf.dwSize.Y==(inf.srWindow.Bottom-inf.srWindow.Top+1))*/)
			{
				//CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
				//GetConsoleScreenBufferInfo(pVCon->hConOut(), &inf);
				DWORD mode = 0;
				BOOL lb = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
				mode &= ~(ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE);
				mode |= ENABLE_EXTENDED_FLAGS;
				lb = SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
			}

			TabBar.Activate();
			lbNeedInval = TRUE;
		}
		TabBar.Update(tabs, cds->dwData);
		if (lbNeedInval)
		{
			gConEmu.SyncConsoleToWindow();
			//RECT rc = ConsoleOffsetRect();
			//rc.bottom = rc.top; rc.top = 0;
			InvalidateRect(ghWnd, NULL/*&rc*/, FALSE);
			if (!gSet.isFullScreen && !IsZoomed(ghWnd)) {
				//SyncWindowToConsole(); -- это делать нельзя, т.к. FAR еще не отработал изменение консоли!
				gConEmu.gbPostUpdateWindowSize = true;
			}
		}
	}

	return result;
}

LRESULT CConEmuMain::OnGetMinMaxInfo(LPMINMAXINFO pInfo)
{
	LRESULT result = 0;

    POINT p = gConEmu.cwShift;
    RECT shiftRect = ConsoleOffsetRect();

    // Минимально допустимые размеры консоли
    COORD srctWindow; srctWindow.X=28; srctWindow.Y=9;

	pInfo->ptMinTrackSize.x = srctWindow.X * (gSet.LogFont.lfWidth ? gSet.LogFont.lfWidth : 4)
		+ p.x + shiftRect.left;

	pInfo->ptMinTrackSize.y = srctWindow.Y * (gSet.LogFont.lfHeight ? gSet.LogFont.lfHeight : 6)
		+ p.y + shiftRect.top;

	return result;
}
