#include "Header.h"
#include "../common/ConEmuCheck.h"


#ifdef MSGLOGGER
bool bBlockDebugLog=true, bSendToDebugger=false, bSendToFile=false;
WCHAR *LogFilePath=NULL;
#endif
#ifndef _DEBUG
bool gbNoDblBuffer = false;
#else
bool gbNoDblBuffer = false;
#endif


//externs
HINSTANCE g_hInstance=NULL;
HWND ghWnd=NULL, ghWndDC=NULL, ghConWnd=NULL, ghApp=NULL;
CConEmuMain gConEmu;
VirtualConsole *pVCon=NULL;
CSettings gSet;
TCHAR temp[MAX_PATH];
TCHAR szIconPath[MAX_PATH];
HICON hClassIcon = NULL, hClassIconSm = NULL;


const TCHAR *const szClassName = VirtualConsoleClass;
const TCHAR *const szClassNameParent = VirtualConsoleClassMain;
const TCHAR *const szClassNameApp = VirtualConsoleClassApp;
const TCHAR *const szClassNameBack = VirtualConsoleClassBack;


OSVERSIONINFO osver;


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
	if (!bSendToFile)
		return;
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

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    
	if (messg == WM_CREATE) {
		if (ghWnd == NULL)
			ghWnd = hWnd; // ставим сразу, чтобы функции могли пользоваться
		else if (ghWndDC == NULL)
			ghWndDC = hWnd; // ставим сразу, чтобы функции могли пользоваться
	}
	
	if (hWnd == ghWnd)
		result = gConEmu.WndProc(hWnd, messg, wParam, lParam);
    else if (hWnd == ghWndDC)
	    result = gConEmu.m_Child.ChildWndProc(hWnd, messg, wParam, lParam);
	else if (messg)
		result = DefWindowProc(hWnd, messg, wParam, lParam);

    return result;
}

BOOL CreateAppWindow()
{
    return TRUE;
}

BOOL CreateMainWindow()
{
	if (!gConEmu.Init())
		return FALSE; // Ошибка уже показана

    if (_tcscmp(VirtualConsoleClass,VirtualConsoleClassMain)) {
	    MBoxA(_T("Error: class names must be equal!"));
	    return FALSE;
    }


    //!!!ICON
    gConEmu.LoadIcons();
    
	WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_DBLCLKS|CS_OWNDC, MainWndProc, 0, 0, 
		    g_hInstance, hClassIcon, LoadCursor(NULL, IDC_ARROW), 
		    NULL /*(HBRUSH)COLOR_BACKGROUND*/, 
		    NULL, szClassNameParent, hClassIconSm};// | CS_DROPSHADOW
    if (!RegisterClassEx(&wc))
        return -1;
    //ghWnd = CreateWindow(szClassName, 0, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, gSet.wndX, gSet.wndY, cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4, NULL, NULL, (HINSTANCE)g_hInstance, NULL);
    DWORD style = 0;
    style |= WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    int nWidth=CW_USEDEFAULT, nHeight=CW_USEDEFAULT;
    
    if (gSet.wndWidth && gSet.wndHeight)
    {
	    if (gSet.LogFont.lfWidth==0)
		    pVCon->InitDC(FALSE); // инициализировать ширину шрифта по умолчанию
		    
	    COORD conSize; conSize.X=gSet.wndWidth; conSize.Y=gSet.wndHeight;
	    int nShiftX = GetSystemMetrics(SM_CXSIZEFRAME)*2;
	    int nShiftY = GetSystemMetrics(SM_CYSIZEFRAME)*2 + GetSystemMetrics(SM_CYCAPTION);
		nWidth  = conSize.X * gSet.LogFont.lfWidth + nShiftX + gSet.rcTabMargins.left+gSet.rcTabMargins.right;
	    nHeight = conSize.Y * gSet.LogFont.lfHeight + nShiftY + gSet.rcTabMargins.top+gSet.rcTabMargins.bottom;
    }
    
    // cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4; -- все равно это было не правильно
	ghWnd = CreateWindow(szClassNameParent, 0, style, gSet.wndX, gSet.wndY, nWidth, nHeight, NULL, NULL, (HINSTANCE)g_hInstance, NULL);
	if (!ghWnd) {
		if (!ghWndDC) MBoxA(_T("Can't create main window!"));
        return FALSE;
	}
	return TRUE;
}

BOOL CheckConIme()
{
    BOOL  lbStopWarning = FALSE;
    DWORD dwValue=1;
    Registry reg;
    if (reg.OpenKey(_T("Software\\ConEmu"), KEY_READ)) {
	    if (!reg.Load(_T("StopWarningConIme"), &lbStopWarning))
		    lbStopWarning = FALSE;
		reg.CloseKey();
    }
    if (!lbStopWarning)
    {
	    if (reg.OpenKey(_T("Console"), KEY_READ))
	    {
	        if (!reg.Load(_T("LoadConIme"), &dwValue))
				dwValue = 1;
	        reg.CloseKey();
	        if (dwValue!=0) {
		        if (IDCANCEL==MessageBox(0,_T("Unwanted value of 'LoadConIme' registry parameter!\r\nPress 'Cancel' to stop this message.\r\nTake a look at 'FAQ-ConEmu.txt'"), _T("ConEmu"),MB_OKCANCEL|MB_ICONEXCLAMATION))
			        lbStopWarning = TRUE;
		    }
	    } else {
		    if (IDCANCEL==MessageBox(0,_T("Can't determine a value of 'LoadConIme' registry parameter!\r\nPress 'Cancel' to stop this message.\r\nTake a look at 'FAQ-ConEmu.txt'"), _T("ConEmu"),MB_OKCANCEL|MB_ICONEXCLAMATION))
		        lbStopWarning = TRUE;
	    }
	    if (lbStopWarning)
	    {
		    if (reg.OpenKey(_T("Software\\ConEmu"), KEY_WRITE)) {
			    reg.Save(_T("StopWarningConIme"), lbStopWarning);
				reg.CloseKey();
		    }
		}
	}
	
	return TRUE;
}

LRESULT CALLBACK AppWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    switch (messg)
    {
    case WM_COMMAND:
	    break;
    default:
        if (messg) result = DefWindowProc(hWnd, messg, wParam, lParam);
    }
    return result;
}

void SetConsoleSizeTo(HWND inConWnd, int inSizeX, int inSizeY);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifndef _DEBUG
    klInit();
#else
	//MessageBox(0,L"Started",L"ConEmu",0);
#endif

    g_hInstance = hInstance;

    pVCon = NULL;

    bool setParentDisabled=false;
    bool ClearTypePrm = false;
    bool FontPrm = false; TCHAR* FontVal;
    bool SizePrm = false; LONG SizeVal;
    bool BufferHeightPrm = false; int BufferHeightVal;
    bool ConfigPrm = false; TCHAR* ConfigVal;
	bool FontFilePrm = false; TCHAR* FontFile; //ADD fontname; by Mors
	bool WindowPrm = false;

    gConEmu.cBlinkShift = GetCaretBlinkTime()/15;

    memset(&osver, 0, sizeof(osver));
    osver.dwOSVersionInfoSize = sizeof(osver);
    GetVersionEx(&osver);

    //Windows7 - SetParent для консоли валится
    gConEmu.setParent = false; // PictureView теперь идет через Wrapper
    if ((osver.dwMajorVersion>6) || (osver.dwMajorVersion==6 && osver.dwMinorVersion>=1))
    {
        setParentDisabled = true;
    }
    if (osver.dwMajorVersion>=6)
    {
	    CheckConIme();
    }
    
    gSet.InitSettings();


//------------------------------------------------------------------------
///| Parsing the command line |///////////////////////////////////////////
//------------------------------------------------------------------------

    TCHAR* cmdNew = NULL;
    //STARTUPINFO stInf; -- Maximus5 - не используется
    //GetStartupInfo(&stInf);

    //Maximus5 - размер командной строки может быть и поболе...
    //_tcsncpy(cmdLine, GetCommandLine(), 0x1000); cmdLine[0x1000-1]=0;
    TCHAR *cmdLine = _tcsdup(GetCommandLine());
	cmdNew = _tcschr(cmdLine, _T('/'));
	if (!cmdNew) cmdNew=(TCHAR*)"";
    SetEnvironmentVariableW(L"ConEmuArgs", cmdNew);
	cmdNew = NULL;

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
                gConEmu.WindowMode = rFullScreen; WindowPrm = true;
            }
            else if ( !klstricmp(curCommand, _T("/DontSetParent")) || !klstricmp(curCommand, _T("/Windows7")) )
            {
                setParentDisabled = true;
            }
            else if ( !klstricmp(curCommand, _T("/SetParent")) )
            {
                gConEmu.setParent = true;
            }
            else if ( !klstricmp(curCommand, _T("/SetParent2")) )
            {
                gConEmu.setParent = true; gConEmu.setParent2 = true;
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
                        free(psz); free(cmdLine);
                        return -1;
                    }
                    ConfigVal = curCommand;
                }
            }
            else if ( !klstricmp(curCommand, _T("/?")))
            {
                MessageBox(NULL, pHelp, L"About ConEmu...", MB_ICONQUESTION);
                free(cmdLine);
                return -1; // NightRoman
            }
        }
    }
    if (setParentDisabled && (gConEmu.setParent || gConEmu.setParent2)) {
        gConEmu.setParent=false; gConEmu.setParent2=false;
    }
        
        
//------------------------------------------------------------------------
///| load settings and apply parameters |/////////////////////////////////
//------------------------------------------------------------------------
        
    // set config name before settings (i.e. where to load from)
    if (ConfigPrm)
    {
        _tcscat(gSet.Config, _T("\\"));
        _tcscat(gSet.Config, ConfigVal);
    }

    // load settings from registry
    gSet.LoadSettings();

    // Установка параметров из командной строки
    if (cmdNew)
        _tcscpy(gSet.Cmd, cmdNew);
    if (ClearTypePrm)
        gSet.LogFont.lfQuality = CLEARTYPE_NATURAL_QUALITY;
    if (FontPrm)
        _tcscpy(gSet.LogFont.lfFaceName, FontVal);
    if (SizePrm)
        gSet.LogFont.lfHeight = SizeVal;
    if (BufferHeightPrm)
        gSet.BufferHeight = BufferHeightVal;
	if (!WindowPrm) {
		if (nCmdShow == SW_SHOWMAXIMIZED)
			gConEmu.WindowMode = rMaximized;
	}
	// Если запускается conman - принудительно включить флажок "Обновлять handle"
	//cmdNew = gSet.Cmd;
	//while (*cmdNew==L' ' || *cmdNew==L'"')
	if (StrStrI(gSet.Cmd, L"conman.exe"))
		gSet.isConMan = TRUE;

//------------------------------------------------------------------------
///| Create VirtualConsole |//////////////////////////////////////////////
//------------------------------------------------------------------------

	#undef pVCon
    // create console
    pVCon = new VirtualConsole();

//------------------------------------------------------------------------
///| Allocating console |/////////////////////////////////////////////////
//------------------------------------------------------------------------

        
    AllocConsole();
    ghConWnd = GetConsoleWindow();
	// Если в свойствах ярлыка указано "Максимизировано" - консоль разворачивается, а FAR при 
	// старте сам меняет размер буфера, в результате - ошибочно устанавливается размер окна
	if (nCmdShow != SW_SHOWNORMAL) ShowWindow(ghConWnd, SW_SHOWNORMAL);
    if (!gSet.isConVisible) {
	    ShowWindow(ghConWnd, SW_HIDE);
	    EnableWindow(ghConWnd, FALSE); // NightRoman
	}
    SetConsoleSizeTo(ghConWnd, 4, 6);
    
    // set quick edit mode for buffer mode
    if (gSet.BufferHeight > 0)
    {
        DWORD mode=0;
		if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode)) {
			if ((mode & 0x0040) == 0)
				SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), (mode | 0x0040));
		}
    }

    if (gSet.wndHeight && gSet.wndWidth)
    {
        COORD b = {gSet.wndWidth, gSet.wndHeight};
	    gConEmu.SetConsoleWindowSize(b,false); // Maximus5 - по аналогии с NightRoman
		//MoveWindow(hConWnd, 0, 0, 1, 1, 0);
		//SetConsoleScreenBufferSize(pVCon->hConOut(), b);
		//MoveWindow(hConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
    }

//------------------------------------------------------------------------
///| Create MainWindow (todo) |///////////////////////////////////////////
//------------------------------------------------------------------------
    
    if (FontFilePrm) AddFontResourceEx(FontFile, FR_PRIVATE, NULL); //ADD fontname; by Mors
    




//------------------------------------------------------------------------
///| Creating window |////////////////////////////////////////////////////
//------------------------------------------------------------------------

    //!!!ICON
    gConEmu.LoadIcons();
    
    if (!CreateMainWindow()) {
	    free(cmdLine);
	    return -1;
	}
	    

    // set parent window of the console window:
    // *) it is used by ConMan and some FAR plugins, set it for standard mode or if /SetParent switch is set
    // *) do not set it by default for buffer mode because it causes unwanted selection jumps
    // WARP ItSelf опытным путем выяснил, что SetParent валит ConEmu в Windows7
    //if (!setParentDisabled && (setParent || gSet.BufferHeight == 0))
    gConEmu.SetConParent();


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
    gConEmu.SetConsoleWindowSize(size, false); // NightRoman
    
    if (gSet.isTabs==1)
	    gConEmu.ForceShowTabs(TRUE);
    
//------------------------------------------------------------------------
///| создадим pipe для общения с плагином |///////////////////////////////
//------------------------------------------------------------------------

    /*WCHAR pipename[MAX_PATH];
    //Maximus5 - теперь имя пайпа - по ИД процесса FAR'а
    wsprintf(pipename, CONEMUPIPE, ghConWnd );
    gConEmu.hPipe = CreateNamedPipe( 
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
    if (gConEmu.hPipe == INVALID_HANDLE_VALUE) 
    {
      MessageBox(ghWnd, _T("CreatePipe failed"), NULL, 0);
      gConEmu.Destroy(); free(cmdLine);
      return 100;
    }*/
	/*wsprintf(pipename, _T("ConEmuPEvent%u"), / *ghWnd* / ghConWnd );
    gConEmu.hPipeEvent = CreateEvent(NULL, TRUE, FALSE, pipename);
    if ((gConEmu.hPipeEvent==NULL) || (gConEmu.hPipeEvent==INVALID_HANDLE_VALUE))
    {
      CloseHandle(gConEmu.hPipe);
      MessageBox(ghWnd, _T("CreatePipe failed"), NULL, 0); 
      gConEmu.Destroy(); free(cmdLine);
      return 100;
    }*/
    // Установить переменную среды с дескриптором окна
	WCHAR szVar[32];
    wsprintf(szVar, L"0x%08x", HDCWND);
    SetEnvironmentVariable(L"ConEmuHWND", szVar);
    

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
	        int nButtons = MB_OK|MB_ICONEXCLAMATION|MB_SETFOREGROUND;
	        
	        _tcscpy(psz, _T("Cannot execute the command.\r\n"));
	        _tcscat(psz, pszErr);
	        if (psz[_tcslen(psz)-1]!=_T('\n')) _tcscat(psz, _T("\r\n"));
	        _tcscat(psz, gSet.Cmd);
	        if (StrStrI(gSet.Cmd, gSet.BufferHeight == 0 ? _T("far.exe") : _T("cmd.exe"))==NULL) {
		        _tcscat(psz, _T("\r\n\r\n"));
		        _tcscat(psz, gSet.BufferHeight == 0 ? _T("Do You want to simply start far?") : _T("Do You want to simply start cmd?"));
		        nButtons |= MB_YESNO;
		    }
	        //MBoxA(psz);
	        int nBrc = MessageBox(NULL, psz, _T("ConEmu"), nButtons);
	        free(psz); free(pszErr);
	        if (nBrc!=IDYES) {
	            gConEmu.Destroy();
	            free(cmdLine);
	            return -1;
	        }
	        *gSet.Cmd = 0; // Выполнить стандартную команду...
        }
    }
    
    if (!*gSet.Cmd)
    {
        // *) simple mode: try to start FAR and then cmd;
        // *) buffer mode: FAR is nonsense, try to start cmd only.
        _tcscpy(temp, gSet.BufferHeight == 0 ? _T("far") : _T("cmd"));
        if (!CreateProcess(NULL, temp, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi) && gSet.BufferHeight == 0)
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
		        gConEmu.Destroy(); free(cmdLine);
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

    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CConEmuMain::HandlerRoutine, true);
    SetTimer(ghWnd, 0, 10, NULL);

    Registry reg;
    if (reg.OpenKey(_T("Control Panel\\Desktop"), KEY_READ))
    {
		WCHAR szValue[MAX_PATH];
        if (reg.Load(_T("DragFullWindows"), szValue))
			gConEmu.mb_FullWindowDrag = szValue[0]==L'1';
        reg.CloseKey();
    }


    gConEmu.DragDrop=new CDragDrop(HDCWND);
    gConEmu.ProgressBars=new CProgressBars(ghWnd, g_hInstance);

    gConEmu.isRBDown=false;
    gConEmu.isLBDown=false;

    SetForegroundWindow(ghWnd);

    SetParent(ghWnd, GetParent(GetShellWindow()));
    gConEmu.GetCWShift(ghWnd, &gConEmu.cwShift);
    gConEmu.GetCWShift(ghConWnd, &gConEmu.cwConsoleShift);
    pVCon->InitDC();
    gConEmu.SyncWindowToConsole();

    gConEmu.SetWindowMode(gConEmu.WindowMode);

    
//------------------------------------------------------------------------
///| Main message loop |//////////////////////////////////////////////////
//------------------------------------------------------------------------
    
    MSG lpMsg;
    while (GetMessage(&lpMsg, NULL, 0, 0))
    {
        TranslateMessage(&lpMsg);
        DispatchMessage(&lpMsg);
    }
    
//------------------------------------------------------------------------
///| Deinitialization |///////////////////////////////////////////////////
//------------------------------------------------------------------------
    
    KillTimer(ghWnd, 0);
    delete pVCon;
    //CloseHandle(hChildProcess); -- он более не требуется

    if (FontFilePrm) RemoveFontResourceEx(FontFile, FR_PRIVATE, NULL); //ADD fontname; by Mors

    FreeConsole();
    free(cmdLine);
    return 0;
}
