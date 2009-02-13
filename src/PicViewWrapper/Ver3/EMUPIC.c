//#include <windows.h>

#define TRUE 1
#define FALSE 0
#define NULL 0

#define     GA_PARENT       1
#define     GA_ROOT         2
#define     GA_ROOTOWNER    3
#define     GWL_STYLE      -16
#define     WS_CHILD        1073741824
#define     WS_DISABLED     134217728
#define     MB_ICONSTOP     16

#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define WINBASEAPI
//#define WINBASEAPI __declspec(dllimport)

#if defined(__GNUC__)
   #define EXTCSTART extern "C" {
   #define EXTCEND };
//   #define DllMain DllMainCRTStartup
#else
   #define EXTCSTART
   #define EXTCEND
#endif

typedef int INT_PTR, *PINT_PTR;
typedef unsigned int UINT_PTR, *PUINT_PTR;
typedef long LONG_PTR, *PLONG_PTR;
typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;
typedef long LONG;
typedef unsigned long DWORD, *LPDWORD;
typedef void *LPVOID;
typedef long HWND;
typedef long HCURSOR;
typedef long HINSTANCE;
typedef const char* LPCTSTR;
typedef const char* LPCSTR;
typedef char* LPSTR;
typedef long BOOL;
typedef unsigned int UINT;
typedef void *HMODULE;
typedef short SHORT;
typedef long HICON;
typedef long HCURSOR;
typedef long HBRUSH;
typedef long HDC;
typedef long HMENU;
typedef unsigned char byte;
typedef byte BYTE;

typedef int (CALLBACK *FARPROC)();
typedef FARPROC TIMERPROC;

typedef struct _RECT { 
  LONG left; 
  LONG top; 
  LONG right; 
  LONG bottom; 
} RECT, *LPRECT;
typedef const RECT* LPCRECT;
typedef struct tagPOINT
{
  LONG x;
  LONG y;
} POINT, *LPPOINT;
typedef struct tagMSG {
  HWND   hwnd; 
  UINT   message; 
  WPARAM wParam; 
  LPARAM lParam; 
  DWORD  time; 
  POINT  pt; 
} MSG, *LPMSG;
typedef const MSG* LPCMSG;
typedef LRESULT (CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef struct _WNDCLASS { 
    UINT       style; 
    WNDPROC    lpfnWndProc; 
    int        cbClsExtra; 
    int        cbWndExtra; 
    HINSTANCE  hInstance; 
    HICON      hIcon; 
    HCURSOR    hCursor; 
    HBRUSH     hbrBackground; 
    LPCTSTR    lpszMenuName; 
    LPCTSTR    lpszClassName; 
} WNDCLASS, *PWNDCLASS;
typedef const WNDCLASS* LPCWNDCLASS;
typedef unsigned short WORD;
typedef WORD ATOM;
typedef struct _WINDOWPLACEMENT { 
    UINT  length; 
    UINT  flags; 
    UINT  showCmd; 
    POINT ptMinPosition; 
    POINT ptMaxPosition; 
    RECT  rcNormalPosition; 
} WINDOWPLACEMENT, *LPWINDOWPLACEMENT;
typedef struct tagPAINTSTRUCT { 
  HDC  hdc; 
  BOOL fErase; 
  RECT rcPaint; 
  BOOL fRestore; 
  BOOL fIncUpdate; 
  BYTE rgbReserved[32]; 
} PAINTSTRUCT, *LPPAINTSTRUCT;
typedef const PAINTSTRUCT* LPCPAINTSTRUCT;

// Kernel32 forwards
EXTCSTART
WINBASEAPI FARPROC WINAPI GetProcAddress( HMODULE hModule, LPCSTR lpProcName );
WINBASEAPI HMODULE WINAPI GetModuleHandleA( LPCSTR lpModuleName );
WINBASEAPI int WINAPI lstrcmpA( LPCSTR lpStr1, LPCSTR lpStr2 );
WINBASEAPI HWND WINAPI GetConsoleWindow();
WINBASEAPI DWORD GetEnvironmentVariableA(LPCSTR lpName,LPSTR lpBuffer,DWORD nSize);
EXTCEND

// True user32 forwards
//typedef int (WINAPI* FGetClassNameA)(HWND,LPSTR,int);
//FGetClassNameA fGetClassNameA=NULL;
typedef BOOL (WINAPI* FIsWindow)(HWND);
FIsWindow fIsWindow=NULL;
typedef BOOL (WINAPI* FIsWindowVisible)(HWND);
FIsWindowVisible fIsWindowVisible=NULL;
typedef BOOL (WINAPI* FGetWindowRect)(HWND,LPRECT);
FGetWindowRect fGetWindowRect=NULL;
//typedef BOOL (WINAPI* FMessageBoxA)(HWND,LPCSTR,LPCSTR,UINT);
//FMessageBoxA fMessageBoxA=NULL;

// === Globals ===
BOOL gbInitialized=FALSE;
HWND ghConEmu=NULL, ghConChild=NULL, ghConsole=NULL, ghConTabs=NULL, ghLastPicWnd=NULL;
HMODULE ghConEmuDll=NULL;
RECT grcShift;
BOOL gbTerminalMode=FALSE;

// === Forwards ===
void InitConEmu();
BOOL GetConEmuShift(LPRECT lprcShift);

#define _T(s) s
#include "../../common/ConEmuCheck.h"

//EXTCSTART
    //BOOL DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved);
//EXTCEND

#if !defined(__GNUC__)
BOOL DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
    return TRUE;
}
#endif

#define IMPLUSER(ret,fn) \
{ \
    ret result = NULL; \
    if (!gbInitialized) InitConEmu(); \
    if (!f##fn) \
        f##fn = (F##fn)GetProcAddress(GetModuleHandleA("user32.dll"),#fn);
#define IMPLUSERV(fn) \
{ \
    if (!f##fn) \
        f##fn = (F##fn)GetProcAddress(GetModuleHandleA("user32.dll"),#fn);

#define RUNUSER(fn) \
    if (f##fn) \
        result = f##fn();
#define RUNUSER1V(fn,a1) \
    if (f##fn) \
        f##fn(a1);
#define RUNUSER1(fn,a1) \
    if (f##fn) \
        result = f##fn(a1);
#define RUNUSER2(fn,a1,a2) \
    if (f##fn) \
        result = f##fn(a1,a2);
#define RUNUSER3(fn,a1,a2,a3) \
    if (f##fn) \
        result = f##fn(a1,a2,a3);
#define RUNUSER4(fn,a1,a2,a3,a4) \
    if (f##fn) \
        result = f##fn(a1,a2,a3,a4);
#define RUNUSER5(fn,a1,a2,a3,a4,a5) \
    if (f##fn) \
        result = f##fn(a1,a2,a3,a4,a5);
 
#define DECLUSER(ret,fn) \
    typedef ret (__stdcall *F##fn)(); \
    F##fn f##fn = NULL; \
    EXTCSTART \
    ret fn(); \
    EXTCEND \
    ret fn() IMPLUSER(ret,fn) RUNUSER(fn)
#define DECLUSER1V(fn,t1,a1) \
    typedef void (__stdcall *F##fn)(t1 a1); \
    F##fn f##fn = NULL; \
    EXTCSTART \
    void fn(t1 a1); \
    EXTCEND \
    void fn(t1 a1) IMPLUSERV(fn) RUNUSER1V(fn,a1)
#define DECLUSER1(ret,fn,t1,a1) \
    typedef ret (__stdcall *F##fn)(t1 a1); \
    F##fn f##fn = NULL; \
    EXTCSTART \
    ret fn(t1 a1); \
    EXTCEND \
    ret fn(t1 a1) IMPLUSER(ret,fn) RUNUSER1(fn,a1)
#define DECLUSER2(ret,fn,t1,a1,t2,a2) \
    typedef ret (__stdcall *F##fn)(t1 a1,t2 a2); \
    F##fn f##fn = NULL; \
    EXTCSTART \
    ret fn(t1 a1,t2 a2); \
    EXTCEND \
    ret fn(t1 a1,t2 a2) IMPLUSER(ret,fn) RUNUSER2(fn,a1,a2)
#define DECLUSER3(ret,fn,t1,a1,t2,a2,t3,a3) \
    typedef ret (__stdcall *F##fn)(t1 a1,t2 a2,t3 a3); \
    F##fn f##fn = NULL; \
    EXTCSTART \
    ret fn(t1 a1,t2 a2,t3 a3); \
    EXTCEND \
    ret fn(t1 a1,t2 a2,t3 a3) IMPLUSER(ret,fn) RUNUSER3(fn,a1,a2,a3)
#define DECLUSER4(ret,fn,t1,a1,t2,a2,t3,a3,t4,a4) \
    typedef ret (__stdcall *F##fn)(t1 a1,t2 a2,t3 a3,t4 a4); \
    F##fn f##fn = NULL; \
    EXTCSTART \
    ret fn(t1 a1,t2 a2,t3 a3,t4 a4); \
    EXTCEND \
    ret fn(t1 a1,t2 a2,t3 a3,t4 a4) IMPLUSER(ret,fn) RUNUSER4(fn,a1,a2,a3,a4)
#define DECLUSER5(ret,fn,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5) \
    typedef ret (__stdcall *F##fn)(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5); \
    F##fn f##fn = NULL; \
    EXTCSTART \
    ret fn(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5); \
    EXTCEND \
    ret fn(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5) IMPLUSER(ret,fn) RUNUSER5(fn,a1,a2,a3,a4,a5)

#define ENDDECL \
    return result; \
}
#define ENDDECLV \
    return; \
}


DECLUSER2(DWORD,GetWindowThreadProcessId,HWND,hwnd,LPDWORD,lpdwProcessId)
ENDDECL

DECLUSER3(int,GetClassNameA,HWND,hwnd,LPSTR,lpsClass,int,nMaxSize)
ENDDECL

DECLUSER3(BOOL,InvalidateRect,HWND,hwnd,LPCRECT,lpRect,BOOL,bErase)
ENDDECL

DECLUSER1(SHORT,GetAsyncKeyState,int,vKey)
ENDDECL

DECLUSER2(HWND,GetAncestor,HWND,hwnd,UINT,gaFlags)
    //// ≈сли без враппера - дл€ FAR'а возвращать родителем ConEmu
    //if (hwnd == ghConsole && gaFlags == GA_PARENT && ghConEmu && fIsWindow(ghConEmu))
    //    result = ghConEmu;
ENDDECL

DECLUSER2(BOOL,GetClientRect,HWND,hwnd,LPRECT,lpRect)
    if (hwnd && hwnd==ghConEmu && fIsWindow(ghConEmu) && fIsWindow(ghConChild))
    {
        /*RECT rc1,rc2,rc3;
        fGetClientRect(ghConChild, &rc1);
        GetConEmuShift(&rc2);
        rc3 = *lpRect;
        *lpRect = rc3;*/
        fGetClientRect(ghConChild, lpRect);
    }
ENDDECL

DECLUSER4(HWND,FindWindowExA,HWND,hwndParent,HWND,hwndChildAfter,LPCTSTR,lpszClass,LPCTSTR,lpszWindow)
ENDDECL

DECLUSER4(int,MapWindowPoints,HWND,hwndFrom,HWND,hwndTo,LPPOINT,lpPoints,UINT,cPoints)
    if (hwndFrom==ghConEmu && !hwndTo && GetConEmuShift(&grcShift))
    {
        UINT i=0;
        for (i=0; i<cPoints; i++)
        {
            lpPoints[i].x += grcShift.left;
            lpPoints[i].y += grcShift.top;
        }
    }
ENDDECL

DECLUSER2(HCURSOR,LoadCursorA,HINSTANCE,hInstance,LPCTSTR,lpCursorName)
ENDDECL

DECLUSER1(ATOM,RegisterClassA,LPCWNDCLASS,lpWndClass)
ENDDECL

DECLUSER(HWND,GetDesktopWindow)
ENDDECL

DECLUSER5(BOOL,PeekMessageA,LPMSG,lpMsg,HWND,hWnd,UINT,wMsgFilterMin,UINT,wMsgFilterMax,UINT,wRemoveMsg)
ENDDECL

typedef HWND (__stdcall *FCreateWindowExA)(DWORD dwExStyle,LPCTSTR lpClassName,LPCTSTR lpWindowName,DWORD dwStyle,int x,int y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam);
FCreateWindowExA fCreateWindowExA = NULL;
EXTCSTART
HWND CreateWindowExA(DWORD dwExStyle,LPCTSTR lpClassName,LPCTSTR lpWindowName,DWORD dwStyle,int x,int y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam);
EXTCEND
HWND CreateWindowExA(DWORD dwExStyle,LPCTSTR lpClassName,LPCTSTR lpWindowName,DWORD dwStyle,int x,int y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam)
{
    HWND result = NULL;
    BOOL lbOurWnd = FALSE;
    if (gbTerminalMode) {
	    return NULL;
    }
    if (!fCreateWindowExA)
        fCreateWindowExA = (FCreateWindowExA)GetProcAddress(GetModuleHandleA("user32.dll"),"CreateWindowExA");
    if (!fCreateWindowExA)
        return NULL;

    if (lpClassName && lstrcmpA(lpClassName,"FarPictureViewControlClass")==0
        && GetConEmuShift(&grcShift))
    {
        x += grcShift.left;
        y += grcShift.top;
        lbOurWnd = TRUE;
    }
    
    result = fCreateWindowExA(dwExStyle,lpClassName,lpWindowName,dwStyle,x,y,nWidth,nHeight,hWndParent,hMenu,hInstance,lpParam);
    if (lbOurWnd)
        ghLastPicWnd = result;
    return result;
}

DECLUSER1(BOOL,DestroyWindow,HWND,hWnd)
ENDDECL

DECLUSER2(HWND,SetParent,HWND,hWndChild,HWND,hWndNewParent)
ENDDECL

DECLUSER1(BOOL,SetForegroundWindow,HWND,hWnd)
ENDDECL

DECLUSER1(int,ShowCursor,BOOL,bShow)
ENDDECL

DECLUSER4(BOOL,PostMessageA,HWND,hWnd,UINT,Msg,WPARAM,wParam,LPARAM,lParam)
ENDDECL

DECLUSER2(BOOL,KillTimer,HWND,hWnd,UINT_PTR,uIDEvent)
ENDDECL

DECLUSER2(HWND,FindWindowA,LPCSTR,lpClassName,LPCSTR,lpWindowName)
ENDDECL

DECLUSER2(BOOL,ShowWindow,HWND,hWnd,int,nCmdShow)
ENDDECL

DECLUSER(HWND,GetForegroundWindow)
ENDDECL

DECLUSER2(BOOL,GetWindowPlacement,HWND,hWnd,LPWINDOWPLACEMENT,lpwndpl)
    //if (result && hWnd && hWnd==ghConEmu && GetConEmuShift(&grcShift))
    //{
    //    //lpwndpl->ptMaxPosition.x += grcShift.top;
    //    //lpwndpl->ptMaxPosition.y += grcShift.bottom;
    //    //lpwndpl->rcNormalPosition.top += grcShift.top;
    //    //lpwndpl->rcNormalPosition.left += grcShift.left;
    //    //lpwndpl->rcNormalPosition.right -= grcShift.right;
    //    //lpwndpl->rcNormalPosition.bottom -= grcShift.bottom;
    //}
ENDDECL

DECLUSER1(LRESULT,DispatchMessageA,LPCMSG,lpmsg)
ENDDECL

DECLUSER2(LONG,GetWindowLongA,HWND,hWnd,int,nIndex)
    //if (hWnd == ghConsole && nIndex == GWL_STYLE && ghConEmu && fIsWindow(ghConEmu))
    //    result = WS_CHILD | WS_DISABLED;
ENDDECL

typedef BOOL (__stdcall *FMoveWindow)(HWND hWnd,int X,int Y,int nWidth,int nHeight,BOOL bRepaint);
FMoveWindow fMoveWindow = NULL;
EXTCSTART
BOOL MoveWindow(HWND hWnd,int X,int Y,int nWidth,int nHeight,BOOL bRepaint);
EXTCEND
BOOL MoveWindow(HWND hWnd,int X,int Y,int nWidth,int nHeight,BOOL bRepaint)
{
    BOOL result = FALSE;
    if (!fMoveWindow)
        fMoveWindow = (FMoveWindow)GetProcAddress(GetModuleHandleA("user32.dll"),"MoveWindow");
    if (!fMoveWindow)
        return FALSE;
    // ¬ FullScreen хорошо бы не двигать окошко, а то заголовок окна виден, но как его отсечь?
	if (hWnd==ghLastPicWnd)
	{
		RECT rcClient; fGetClientRect(ghConEmu, &rcClient);
		if (nWidth<=rcClient.right && nHeight<=rcClient.bottom && GetConEmuShift(&grcShift))
		{
			DWORD dwStyle = GetWindowLongA(hWnd, GWL_STYLE);
			X += grcShift.left;
			Y += grcShift.top;
		}
    }
    result = fMoveWindow(hWnd,X,Y,nWidth,nHeight,bRepaint);
    return result;
}
  
DECLUSER1(BOOL,GetCursorPos,LPPOINT,lpPoint)
ENDDECL

DECLUSER2(BOOL,SetCursorPos,int,X,int,Y)
ENDDECL

DECLUSER2(BOOL,ValidateRect,HWND,hWnd,LPCRECT,lpRect)
ENDDECL

DECLUSER2(HDC,BeginPaint,HWND,hWnd,LPPAINTSTRUCT,lpPaint)
ENDDECL

DECLUSER3(BOOL,SubtractRect,LPRECT,lpDst,LPCRECT,lprcSrc1,LPCRECT,lprcSrc2)
ENDDECL

DECLUSER2(BOOL,EndPaint,HWND,hWnd,LPCPAINTSTRUCT,lpPaint)
ENDDECL

DECLUSER4(UINT_PTR,SetTimer,HWND,hWnd,UINT_PTR,nIDEvent,UINT,uElapse,TIMERPROC,lpTimerFunc)
ENDDECL

DECLUSER1(HWND,SetCapture,HWND,hWnd)
ENDDECL

DECLUSER(BOOL,ReleaseCapture)
ENDDECL

DECLUSER4(LRESULT,DefWindowProcA,HWND,hWnd,UINT,Msg,WPARAM,wParam,LPARAM,lParam)
ENDDECL

DECLUSER3(BOOL,GetUpdateRect,HWND,hWnd,LPRECT,lpRect,BOOL,bErase)
ENDDECL

DECLUSER1V(PostQuitMessage,int,nExitCode)
ENDDECLV
  
DECLUSER3(BOOL,IntersectRect,LPRECT,lpRcDst,LPCRECT,lprcSrc1,LPCRECT,lprcSrc2)
ENDDECL



//typedef HWND (WINAPI* FGetFarHWND2)();
//FGetFarHWND2 GetFarHWND2=NULL;

BOOL GetConEmuShift(LPRECT lprcShift)
{
    BOOL result = FALSE;
    
    if (ghConEmu && fIsWindow(ghConEmu) && fIsWindow(ghConChild))
    {
        RECT rcMain, rcChild;
        //char szDebug[255];
        fGetClientRect(ghConEmu, &rcMain);
        fGetClientRect(ghConChild, &rcChild);
        //sprintf(szDebug, "(%i-%i)x(%i-%i)\r\n",rcChild.left,rcChild.top,rcChild.right,rcChild.bottom);
        fMapWindowPoints(ghConChild, ghConEmu, (LPPOINT)&rcChild, 2);
        //sprintf(szDebug+strlen(szDebug), "(%i-%i)x(%i-%i)",rcChild.left,rcChild.top,rcChild.right,rcChild.bottom);
        //fMessageBoxA(0,   szDebug,"ConEmuDebug",0);
    
        if (rcChild.left || rcChild.top ||
            (rcChild.right!=rcMain.right) || (rcChild.bottom!=rcMain.bottom))
        {
            lprcShift->left = rcChild.left;
            lprcShift->top = rcChild.top;
            lprcShift->right = rcMain.right - rcChild.right;
            lprcShift->bottom = rcMain.bottom - rcChild.bottom;

            result = TRUE;
        }
    }
    
    return result;
}

void InitConEmu()
{
    HMODULE hUser32=NULL;
    char szClass[64];
    int nChk = 0;
    
    gbInitialized=TRUE;
    
    hUser32 = GetModuleHandleA("user32.dll");
    if (!hUser32) return; // ƒолжен быть!

    // User32 links    
    if (!fGetAncestor)
        fGetAncestor = (FGetAncestor)GetProcAddress(hUser32, "GetAncestor");
    if (!fGetAncestor) return;
    
    if (!fGetClassNameA)
        fGetClassNameA = (FGetClassNameA)GetProcAddress(hUser32, "GetClassNameA");
    if (!fGetClassNameA) return;
    
    if (!fFindWindowExA)
        fFindWindowExA = (FFindWindowExA)GetProcAddress(hUser32, "FindWindowExA");
    if (!fFindWindowExA) return;
    
    if (!fIsWindow)
        fIsWindow = (FIsWindow)GetProcAddress(hUser32, "IsWindow");
    if (!fIsWindow) return;

    if (!fIsWindowVisible)
        fIsWindowVisible = (FIsWindowVisible)GetProcAddress(hUser32, "IsWindowVisible");
    if (!fIsWindowVisible) return;

    if (!fGetWindowRect)
        fGetWindowRect = (FGetWindowRect)GetProcAddress(hUser32, "GetWindowRect");
    if (!fGetWindowRect) return;

    if (!fGetClientRect)
        fGetClientRect = (FGetClientRect)GetProcAddress(hUser32, "GetClientRect");
    if (!fGetClientRect) return;
    
    if (!fMapWindowPoints)
        fMapWindowPoints = (FMapWindowPoints)GetProcAddress(hUser32, "MapWindowPoints");
    if (!fMapWindowPoints) return;

    //if (!fMessageBoxA)
    //    fMessageBoxA= (FMessageBoxA)GetProcAddress(hUser32, "MessageBoxA");
    //if (!fMessageBoxA) return;
    
    
    // Check Terminal mode
    szClass[0] = 0;
    if (GetEnvironmentVariableA("TERM", szClass, 63)) {
	    gbTerminalMode = TRUE;
        //lstrcpy(gsTermMsg, _T("PictureView wrapper\nPicture viewing is not available\nin terminal mode ("));
        //lstrcat(gsTermMsg, szVarValue);
        //lstrcat(gsTermMsg, _T(")"));
    }

    // Real console window handle
    ghConsole = GetConsoleWindow();
    
    
    //ghConEmuDll = GetModuleHandleA("ConEmu.dll");
    //if (!ghConEmuDll) return;
    //GetFarHWND2 = (FGetFarHWND2)GetProcAddress(ghConEmuDll,"GetFarHWND2");
    //if (!GetFarHWND2) {
    //    // видимо старый ConEmu? не будем обрабатывать
    //    //if (!gbTerminalMode)
	//    //    fMessageBoxA(ghConChild, "ConEmu old version detected!\r\nPlease upgrade!", "PictureView wrapper", MB_ICONSTOP);
    //    return;
    //}

    //ghConChild = GetFarHWND2(TRUE);
    nChk = ConEmuCheck ( &ghConChild );
    if (nChk!=0 || !ghConChild)
	    return;
    
    ghConEmu = fGetAncestor(ghConChild, GA_PARENT);
    if (!ghConEmu) return;
    fGetClassNameA(ghConEmu, szClass, 63);
    if (lstrcmpA(szClass, VirtualConsoleClassMain)!=0) {
        // видимо старый ConEmu? не будем обрабатывать
        //if (!gbTerminalMode)
	    //    fMessageBoxA(ghConChild, "ConEmu old version detected!\r\nPlease upgrade!", "PictureView wrapper", MB_ICONSTOP);
        ghConEmu = NULL;
        return;
    }
    
    ghConTabs = fFindWindowExA(ghConEmu, 0, "SysTabControl32", 0);
    
    // ¬се, инициализаци€ завершена
    //GetConEmuShift(NULL);
}
