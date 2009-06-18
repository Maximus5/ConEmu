#include "Header.h"
#include <commctrl.h>
extern "C" {
#include "../common/ConEmuCheck.h"
}


#ifdef MSGLOGGER
BOOL bBlockDebugLog=false, bSendToDebugger=true, bSendToFile=false;
WCHAR *LogFilePath=NULL;
#endif
#ifndef _DEBUG
BOOL gbNoDblBuffer = false;
#else
BOOL gbNoDblBuffer = false;
#endif
BOOL gbMessagingStarted = FALSE;

#ifdef _DEBUG
wchar_t gszDbgModLabel[6] = {0};
#endif


//externs
HINSTANCE g_hInstance=NULL;
HWND ghWnd=NULL, ghWndDC=NULL, ghConWnd=NULL, ghWndApp=NULL;
CConEmuMain gConEmu;
//CVirtualConsole *pVCon=NULL;
CSettings gSet;
//TCHAR temp[MAX_PATH]; -- низзя, очень велик шанс нарваться при многопоточности
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
        //wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        //break;
		return;
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
void DebugLogPos(HWND hw, int x, int y, int w, int h, LPCSTR asFunc)
{
	#ifdef MSGLOGGER
	if (!x && !y && hw == ghConWnd) {
		if (!IsDebuggerPresent() && !isPressed(VK_LBUTTON))
			x = x;
	}
	#endif
    if (bBlockDebugLog || (!bSendToFile && !IsDebuggerPresent()))
        return;

    static char szPos[255];

    if (bSendToDebugger) {
        static SYSTEMTIME st;
        GetLocalTime(&st);
        wsprintfA(szPos, "%02i:%02i.%03i %s(%s, %i,%i,%i,%i)\n",
            st.wMinute, st.wSecond, st.wMilliseconds, asFunc,
            (hw==ghConWnd) ? "Con" : "Emu", x,y,w,h);

        OutputDebugStringA(szPos);
    } else if (bSendToFile) {
        wsprintfA(szPos, "%s(%s, %i,%i,%i,%i)\n",
			asFunc,
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
	//__debugbreak();
	_ASSERT(FALSE);
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

LRESULT CALLBACK AppWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    
	if (messg == WM_CREATE) {
		if (ghWndApp == NULL)
			ghWndApp = hWnd;
	} else if (messg == WM_ACTIVATEAPP) {
		if (wParam && ghWnd)
			SetFocus(ghWnd);
	}
	
	result = DefWindowProc(hWnd, messg, wParam, lParam);

    return result;
}

BOOL CreateAppWindow()
{
	//2009-03-05 - нельзя этого делать. а то дочерние процессы с тем же Affinity запускаются...
	// На тормоза - не влияет. Но вроде бы на многопроцессорных из-зп глюков
	// в железе могут быть ошибки подсчета производительности, если этого не сделать
	SetProcessAffinityMask(GetCurrentProcess(), gSet.nAffinity);
	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	//SetThreadAffinityMask(GetCurrentThread(), 1);

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC   = ICC_COOL_CLASSES|ICC_BAR_CLASSES|ICC_TAB_CLASSES|ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&icex);

	/*DWORD dwErr = 0;
	HMODULE hInf = LoadLibrary(L"infis.dll");
	if (!hInf)
		dwErr = GetLastError();*/

    //!!!ICON
    gConEmu.LoadIcons();
    
    if (!gSet.isCreateAppWindow)
	    return TRUE;

	WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_DBLCLKS|CS_OWNDC, AppWndProc, 0, 0, 
		    g_hInstance, hClassIcon, LoadCursor(NULL, IDC_ARROW), 
		    NULL /*(HBRUSH)COLOR_BACKGROUND*/, 
		    NULL, szClassNameApp, hClassIconSm};// | CS_DROPSHADOW
    if (!RegisterClassEx(&wc))
        return -1;
    //ghWnd = CreateWindow(szClassName, 0, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, gSet.wndX, gSet.wndY, cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4, NULL, NULL, (HINSTANCE)g_hInstance, NULL);
    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
    int nWidth=100, nHeight=100, nX = -32000, nY = -32000;
    DWORD exStyle = WS_EX_APPWINDOW|WS_EX_ACCEPTFILES;
    
    
    // cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4; -- все равно это было не правильно
	ghWndApp = CreateWindowEx(exStyle, szClassNameApp, L"ConEmu", style, nX, nY, nWidth, nHeight, NULL, NULL, (HINSTANCE)g_hInstance, NULL);
	if (!ghWndApp) {
		MBoxA(_T("Can't create application window!"));
        return FALSE;
	}
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
    
	// 2009-06-11 Возможно, что CS_SAVEBITS приводит к глюкам отрисовки
	// банально непрорисовываются некоторые части экрана (драйвер видюхи глючит?)
	WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_DBLCLKS|CS_OWNDC/*|CS_SAVEBITS*/, MainWndProc, 0, 16, 
		    g_hInstance, hClassIcon, LoadCursor(NULL, IDC_ARROW), 
		    NULL /*(HBRUSH)COLOR_BACKGROUND*/, 
		    NULL, szClassNameParent, hClassIconSm};// | CS_DROPSHADOW
    if (!RegisterClassEx(&wc))
        return -1;
    //ghWnd = CreateWindow(szClassName, 0, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, gSet.wndX, gSet.wndY, cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4, NULL, NULL, (HINSTANCE)g_hInstance, NULL);
    DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    if (ghWndApp)
	    style |= WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	else
		style |= WS_OVERLAPPEDWINDOW;
    int nWidth=CW_USEDEFAULT, nHeight=CW_USEDEFAULT;
    
    if (gSet.wndWidth && gSet.wndHeight)
    {
	    //if (gSet.FontWidth()==0)
		//    pVCon->InitDC(FALSE); // инициализировать ширину шрифта по умолчанию

		MBoxAssert(gSet.FontWidth() && gSet.FontHeight());

	    COORD conSize; conSize.X=gSet.wndWidth; conSize.Y=gSet.wndHeight;
	    int nShiftX = GetSystemMetrics(SM_CXSIZEFRAME)*2;
	    int nShiftY = GetSystemMetrics(SM_CYSIZEFRAME)*2 + GetSystemMetrics(SM_CYCAPTION);
		nWidth  = conSize.X * gSet.FontWidth() + nShiftX + gSet.rcTabMargins.left+gSet.rcTabMargins.right;
	    nHeight = conSize.Y * gSet.FontHeight() + nShiftY + gSet.rcTabMargins.top+gSet.rcTabMargins.bottom;
    }
    
	//if (gConEmu.WindowMode == rMaximized) style |= WS_MAXIMIZE;
	//style |= WS_VISIBLE;
    // cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4; -- все равно это было не правильно
	ghWnd = CreateWindow(szClassNameParent, gSet.GetCmd(), style, 
		gSet.wndX, gSet.wndY, nWidth, nHeight, ghWndApp, NULL, (HINSTANCE)g_hInstance, NULL);
	if (!ghWnd) {
		if (!ghWndDC) MBoxA(_T("Can't create main window!"));
        return FALSE;
	}
	//if (gConEmu.WindowMode == rFullScreen || gConEmu.WindowMode == rMaximized)
	//	gConEmu.SetWindowMode(gConEmu.WindowMode);
	return TRUE;
}

BOOL CheckConIme()
{
    BOOL  lbStopWarning = FALSE;
    DWORD dwValue=1;
    Registry reg;
    if (reg.OpenKey(_T("Software\\ConEmu"), KEY_READ)) {
	    if (!reg.Load(_T("StopWarningConIme"), lbStopWarning))
		    lbStopWarning = FALSE;
		reg.CloseKey();
    }
    if (!lbStopWarning)
    {
	    if (reg.OpenKey(_T("Console"), KEY_READ))
	    {
	        if (!reg.Load(_T("LoadConIme"), dwValue))
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

//extern void SetConsoleFontSizeTo(HWND inConWnd, int inSizeX, int inSizeY);

// Disables the IME for all threads in a current process.
//void DisableIME()
//{
//	typedef BOOL (WINAPI* ImmDisableIMEt)(DWORD idThread);
//	BOOL lbDisabled = FALSE;
//	HMODULE hImm32 = LoadLibrary(_T("imm32.dll"));
//	if (hImm32) {
//		ImmDisableIMEt ImmDisableIMEf = (ImmDisableIMEt)GetProcAddress(hImm32, "ImmDisableIME");
//		if (ImmDisableIMEf) {
//			lbDisabled = ImmDisableIMEf(-1);
//		}
//	}
//	return;
//}

void MessageLoop()
{
	MSG Msg = {NULL};
	gbMessagingStarted = TRUE;
	while (GetMessage(&Msg, NULL, 0, 0))
	{
		BOOL lbDlgMsg = FALSE;
		if (ghOpWnd) {
			if (IsWindow(ghOpWnd))
				lbDlgMsg = IsDialogMessage(ghOpWnd, &Msg);
		}
		if (!lbDlgMsg)
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	gbMessagingStarted = FALSE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    g_hInstance = hInstance;

	if (_tcsstr(GetCommandLine(), L"/debugi")) {
		if (!IsDebuggerPresent()) _ASSERT(FALSE);
	} else if (_tcsstr(GetCommandLine(), L"/debug")) {
		if (!IsDebuggerPresent()) MBoxA(L"Conemu started");
	}

    //pVCon = NULL;

    //bool setParentDisabled=false;
    bool ClearTypePrm = false;
    bool FontPrm = false; TCHAR* FontVal = NULL;
    bool SizePrm = false; LONG SizeVal;
    bool BufferHeightPrm = false; int BufferHeightVal;
    bool ConfigPrm = false; TCHAR* ConfigVal = NULL;
	bool FontFilePrm = false; TCHAR* FontFile = NULL; //ADD fontname; by Mors
	bool WindowPrm = false; int WindowModeVal = 0;
	bool AttachPrm = false; LONG AttachVal=0;
	bool ConManPrm = false, ConManValue = false;
	bool VisPrm = false, VisValue = false;

    //gConEmu.cBlinkShift = GetCaretBlinkTime()/15;

    memset(&osver, 0, sizeof(osver));
    osver.dwOSVersionInfoSize = sizeof(osver);
    GetVersionEx(&osver);
    
    //DisableIME();

    //Windows7 - SetParent для консоли валится
    //gConEmu.setParent = false; // PictureView теперь идет через Wrapper
    //if ((osver.dwMajorVersion>6) || (osver.dwMajorVersion==6 && osver.dwMinorVersion>=1))
    //{
    //    setParentDisabled = true;
    //}
    if (osver.dwMajorVersion>=6)
    {
	    CheckConIme();
    }
    
    //gSet.InitSettings();


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


	TCHAR *psUnknown = NULL;
    TCHAR *curCommand = cmdLine;
    {
#ifdef KL_MEM
        uint params = klSplitCommandLine(curCommand);
#else
        uint params; klSplitCommandLine(curCommand, &params);
#endif

		if(params < 2) {
            curCommand = NULL;
		}
        // Parse parameters.
        // Duplicated parameters are permitted, the first value is used.
		uint i = 0; // ммать... curCommand увеличивался, а i НЕТ
        while (i < params && curCommand && *curCommand)
        {
	        if ( !klstricmp(curCommand, _T("/autosetup")) ) {
		        BOOL lbTurnOn = TRUE;
		        if ((i + 1) >= params)
			        return 101;
	        
		        curCommand += _tcslen(curCommand) + 1; i++;
		        if (*curCommand == _T('0'))
			        lbTurnOn = FALSE;
			    else {
			        if ((i + 1) >= params)
				        return 101;
				    curCommand += _tcslen(curCommand) + 1; i++;
				    DWORD dwAttr = GetFileAttributes(curCommand);
				    if (dwAttr == -1 || (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
					    return 102;
			    }

		        HKEY hk = NULL; DWORD dw;
		        int nSetupRc = 100;
		        if (0 != RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Command Processor"),
			        NULL, NULL, NULL, KEY_ALL_ACCESS, NULL, &hk, &dw))
			        return 103;
			    if (lbTurnOn) {
				    BOOL bNeedFree = FALSE;
				    if (*curCommand!=_T('"') && _tcschr(curCommand, _T(' '))) {
					    TCHAR* psz = (TCHAR*)calloc(_tcslen(curCommand)+3, sizeof(TCHAR));
					    *psz = _T('"');
					    _tcscpy(psz+1, curCommand);
					    _tcscat(psz, _T("\""));
					    curCommand = psz;
				    }
				    if (0==RegSetValueEx(hk, _T("AutoRun"), NULL, REG_SZ, (LPBYTE)curCommand,
						    sizeof(TCHAR)*(_tcslen(curCommand)+1)))
					    nSetupRc = 1;
					if (bNeedFree) 
						free(curCommand);
			    } else {
				    if (0==RegDeleteValue(hk, _T("AutoRun")))
					    nSetupRc = 1;
			    }
		        RegCloseKey(hk);
		        // сбрость CreateInNewEnvironment для ConMan
		        if (0 == RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\HoopoePG_2x"),
				        NULL, NULL, NULL, KEY_ALL_ACCESS, NULL, &hk, &dw))
				{
					RegSetValueEx(hk, _T("CreateInNewEnvironment"), NULL, REG_DWORD,
						(LPBYTE)&(dw=0), 4);
					RegCloseKey(hk);
				}
		        return nSetupRc;
	        }
			else if ( !klstricmp(curCommand, _T("/multi")) ) {
				ConManValue = true; ConManPrm = true;
			}
			else if ( !klstricmp(curCommand, _T("/nomulti")) ) {
				ConManValue = false; ConManPrm = true;
			}
			else if ( !klstricmp(curCommand, _T("/visible")) ) {
				VisValue = true; VisPrm = true;
			}
			else if ( !klstricmp(curCommand, _T("/detached")) ) {
				gConEmu.mb_StartDetached = TRUE;
			}
            else if ( !klstricmp(curCommand, _T("/ct")) || !klstricmp(curCommand, _T("/cleartype")) )
            {
                ClearTypePrm = true;
            }

			// имя шрифта
            else if ( !klstricmp(curCommand, _T("/font")) && i + 1 < params)
            {
                curCommand += _tcslen(curCommand) + 1; i++;
                if (!FontPrm)
                {
                    FontPrm = true;
                    FontVal = curCommand;
                }
            }

			// Высота шрифта
            else if ( !klstricmp(curCommand, _T("/size")) && i + 1 < params)
            {
                curCommand += _tcslen(curCommand) + 1; i++;
                if (!SizePrm)
                {
                    SizePrm = true;
                    SizeVal = klatoi(curCommand);
                }
            }
            else if ( !klstricmp(curCommand, _T("/attach")) /*&& i + 1 < params*/)
            {
                //curCommand += _tcslen(curCommand) + 1; i++;
                if (!AttachPrm)
                {
                    AttachPrm = true; AttachVal = -1;
                    if ((i + 1) < params)
                    {
	                    TCHAR *nextCommand = curCommand + _tcslen(curCommand) + 1;
	                    if (*nextCommand != _T('/')) {
		                    curCommand = nextCommand; i++;
		                    AttachVal = klatoi(curCommand);
		                }
	                }
	                // интеллектуальный аттач - если к текущей консоли уже подцеплена другая копия
	                if (AttachVal == -1) {
		                HWND hCon = GetForegroundWindow();
		                if (!hCon) {
							// консоли нет
							return 100;
						} else {
			                TCHAR sClass[128];
			                if (GetClassName(hCon, sClass, 128)) {
				                if (_tcscmp(sClass, VirtualConsoleClassMain)==0) {
					                // Сверху УЖЕ другая копия ConEmu
					                return 1;
				                }
								// Если на самом верху НЕ консоль - это может быть панель проводника, 
								// или другое плавающее окошко... Поищем ВЕРХНЮЮ консоль
								if (_tcscmp(sClass, _T("ConsoleWindowClass"))!=0) {
									_tcscpy(sClass, _T("ConsoleWindowClass"));
									hCon = FindWindow(_T("ConsoleWindowClass"), NULL);
									if (!hCon)
										return 100;
								}
				                if (_tcscmp(sClass, _T("ConsoleWindowClass"))==0) {
					                // перебрать все ConEmu, может кто-то уже подцеплен?
					                HWND hEmu = NULL;
					                while (hEmu = FindWindowEx(NULL, hEmu, VirtualConsoleClassMain, NULL))
					                {
						                if (hCon == (HWND)GetWindowLong(hEmu, GWL_USERDATA)) {
							                // к этой консоли уже подцеплен ConEmu
							                return 1;
						                }
					                }
								} else {
									// верхнее окно - НЕ консоль
									return 100;
								}
			                }
		                }
						gSet.hAttachConWnd = hCon;
	                }
                }
            }
            //Start ADD fontname; by Mors
            else if ( !klstricmp(curCommand, _T("/fontfile")) && i + 1 < params)
            {
                curCommand += _tcslen(curCommand) + 1; i++;
                if (!FontFilePrm)
                {
                    FontFilePrm = true;
					int nLen = _tcslen(curCommand);
					if (nLen >= MAX_PATH)
					{
						TCHAR* psz=(TCHAR*)calloc(nLen+100,sizeof(TCHAR));
						wsprintf(psz, _T("Too long /FontFile name (%i chars).\r\n"), nLen);
						_tcscat(psz, curCommand);
						MBoxA(psz);
						free(psz); free(cmdLine);
						return 100;
					}
                    FontFile = curCommand;
                }
            }
            //End ADD fontname; by Mors
            else if ( !klstricmp(curCommand, _T("/fs")))
            {
                WindowModeVal = rFullScreen; WindowPrm = true;
            }
            else if ( !klstricmp(curCommand, _T("/max")))
            {
                WindowModeVal = rMaximized; WindowPrm = true;
            }
            else if ( !klstricmp(curCommand, _T("/log")))
            {
	            gSet.isAdvLogging = 1;
	        }
            else if ( !klstricmp(curCommand, _T("/log0")))
            {
	            gSet.isAdvLogging = 2;
	        }
            //else if ( !klstricmp(curCommand, _T("/DontSetParent")) || !klstricmp(curCommand, _T("/Windows7")) )
            //{
            //    setParentDisabled = true;
            //}
            //else if ( !klstricmp(curCommand, _T("/SetParent")) )
            //{
            //    gConEmu.setParent = true;
            //}
            //else if ( !klstricmp(curCommand, _T("/SetParent2")) )
            //{
            //    gConEmu.setParent = true; gConEmu.setParent2 = true;
            //}
            else if (!klstricmp(curCommand, _T("/BufferHeight")) && i + 1 < params)
            {
                curCommand += _tcslen(curCommand) + 1; i++;
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
                curCommand += _tcslen(curCommand) + 1; i++;
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
                        return 100;
                    }
                    ConfigVal = curCommand;
                }
            }
            else if ( !klstricmp(curCommand, _T("/?")))
            {
                MessageBox(NULL, pHelp, L"About ConEmu...", MB_ICONQUESTION);
                free(cmdLine);
                return -1; // NightRoman
            } else if (i>0 && !psUnknown && *curCommand) {
	            // ругнуться на неизвестную команду
	            psUnknown = curCommand;
            }

			curCommand += _tcslen(curCommand) + 1; i++;
        }
    }
    //if (setParentDisabled && (gConEmu.setParent || gConEmu.setParent2)) {
    //    gConEmu.setParent=false; gConEmu.setParent2=false;
    //}

    if (psUnknown) {
	    TCHAR* psMsg = (TCHAR*)calloc(_tcslen(psUnknown)+100,sizeof(TCHAR));
	    _tcscpy(psMsg, _T("Unknown command specified:\r\n"));
	    _tcscat(psMsg, psUnknown);
	    MBoxA(psMsg);
	    free(psMsg);
	    return 100;
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
	if (cmdNew) {
		MCHKHEAP
		int nLen = _tcslen(cmdNew)+1;
		//TCHAR *pszSlash=NULL;
		//nLen += _tcslen(gConEmu.ms_ConEmuExe) + 20;
		gSet.psCurCmd = (TCHAR*)malloc(nLen*sizeof(TCHAR));
		_ASSERTE(gSet.psCurCmd);
		//wcscpy(gSet.psCurCmd, L"\"");
		//wcscat(gSet.psCurCmd, gConEmu.ms_ConEmuExe);
		//pszSlash = wcsrchr(gSet.psCurCmd, _T('\\'));
		//wcscpy(pszSlash+1, L"ConEmuC.exe\" /CMD ");
		wcscpy(gSet.psCurCmd, cmdNew);
		MCHKHEAP
	}
	//#pragma message("Win2k: CLEARTYPE_NATURAL_QUALITY")
    //if (ClearTypePrm)
    //    gSet.LogFont.lfQuality = CLEARTYPE_NATURAL_QUALITY;
    //if (FontPrm)
    //    _tcscpy(gSet.LogFont.lfFaceName, FontVal);
    //if (SizePrm)
    //    gSet.LogFont.lfHeight = SizeVal;
    if (BufferHeightPrm) {
        gSet.SetArgBufferHeight ( BufferHeightVal );
    }
	if (!WindowPrm) {
		if (nCmdShow == SW_SHOWMAXIMIZED)
			gConEmu.WindowMode = rMaximized;
	} else {
		gConEmu.WindowMode = WindowModeVal;
	}
	if (ConManPrm)
		gSet.isMulti = ConManValue;
	if (VisValue)
		gSet.isConVisible = VisPrm;
	// Если запускается conman (нафига?) - принудительно включить флажок "Обновлять handle"
	if (gSet.isMulti || StrStrI(gSet.GetCmd(), L"conman.exe"))
		gSet.isUpdConHandle = TRUE;


	if (FontFilePrm) {
		if (!AddFontResourceEx(FontFile, FR_PRIVATE, NULL)) //ADD fontname; by Mors
		{
			TCHAR* psz=(TCHAR*)calloc(wcslen(FontFile)+100,sizeof(TCHAR));
			lstrcpyW(psz, L"Can't register font:\n");
			lstrcatW(psz, FontFile);
			MessageBox(NULL, psz, L"ConEmu", MB_OK|MB_ICONSTOP);
			free(psz);
			return 100;
		}
		lstrcpynW(gSet.FontFile, FontFile, countof(gSet.FontFile));
	}


	gSet.InitFont(
		FontPrm ? FontVal : NULL,
		SizePrm ? SizeVal : -1,
		ClearTypePrm ? CLEARTYPE_NATURAL_QUALITY : -1
		);


    
//------------------------------------------------------------------------
///| Allocating console |/////////////////////////////////////////////////
//------------------------------------------------------------------------

	BOOL lbConsoleAllocated = FALSE;
    if (AttachPrm) {
	    if (!AttachVal) {
		    MBoxA(_T("Invalid <process id> specified in the /Attach argument"));
		    //delete pVCon;
		    return 100;
	    }
	    gSet.nAttachPID = AttachVal;
    }


//------------------------------------------------------------------------
///| Create taskbar window |//////////////////////////////////////////////
//------------------------------------------------------------------------

    // Тут загружаются иконки, и создается кнопка на таскбаре (если надо)
    if (!CreateAppWindow()) {
	    //delete pVCon;
	    return 100;
    }

//------------------------------------------------------------------------
///| Creating window |////////////////////////////////////////////////////
//------------------------------------------------------------------------

    if (!CreateMainWindow()) {
	    free(cmdLine);
	    //delete pVCon;
	    return 100;
	}

	//if (AttachPrm) {
	//	// Заодно, вызвать определение нового окна ConEmu
	//	TabBar.Retrieve();
	//}
	    

    // set parent window of the console window:
    // *) it is used by ConMan and some FAR plugins, set it for standard mode or if /SetParent switch is set
    // *) do not set it by default for buffer mode because it causes unwanted selection jumps
    // WARP ItSelf опытным путем выяснил, что SetParent валит ConEmu в Windows7
    //if (!setParentDisabled && (setParent || gConEmu.Buffer Height == 0))
    
    //gConEmu.SetConParent();

    //// adjust the console window and buffer to settings
    //// *) after getting all the settings
    //// *) before running the command
    //!!! TODO:
    //COORD size = {gSet.wndWidth, gSet.wndHeight};
    //gConEmu.SetConsoleWindowSize(size, false); // NightRoman
    
    


//------------------------------------------------------------------------
///| Misc |///////////////////////////////////////////////////////////////
//------------------------------------------------------------------------

	gConEmu.PostCreate();

   
//------------------------------------------------------------------------
///| Main message loop |//////////////////////////////////////////////////
//------------------------------------------------------------------------

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	MessageLoop();
    
//------------------------------------------------------------------------
///| Deinitialization |///////////////////////////////////////////////////
//------------------------------------------------------------------------
    
    //KillTimer(ghWnd, 0);
    //delete pVCon;
    //CloseHandle(hChildProcess); -- он более не требуется

    if (FontFilePrm) RemoveFontResourceEx(FontFile, FR_PRIVATE, NULL); //ADD fontname; by Mors

	if (lbConsoleAllocated)
		FreeConsole();

    free(cmdLine);

	//CoUninitialize();
	OleUninitialize();

    return 0;
}
