#pragma once

extern const TCHAR *const szClassName;
extern const TCHAR *const szClassNameParent;
extern const TCHAR *const szClassNameApp;
extern const TCHAR *const szClassNameBack;

class CVirtualConsole;
extern CVirtualConsole *pVCon;
extern HINSTANCE g_hInstance;
class CConEmuMain;
extern CConEmuMain gConEmu;
//extern TCHAR Title[0x400];
//extern bool isLBDown, /*isInDrag,*/ isDragProcessed;
extern HWND ghWnd, ghConWnd, ghWndDC, ghOpWnd, ghWndApp;
class TabBarClass;
extern TabBarClass TabBar;

extern const int TAB_FONT_HEIGTH;
extern wchar_t TAB_FONT_FACE[];


extern TCHAR szIconPath[MAX_PATH];
extern HICON hClassIcon, hClassIconSm;

class CSettings;
extern CSettings gSet;
class TrayIcon;
extern TrayIcon Icon;
extern TCHAR temp[MAX_PATH];

extern bool gbNoDblBuffer;

#ifdef _DEBUG
extern char gsz_MDEBUG_TRAP_MSG[3000];
extern char gsz_MDEBUG_TRAP_MSG_APPEND[2000];
extern HWND gh_MDEBUG_TRAP_PARENT_WND;
#endif


#ifdef MSGLOGGER
	extern bool bBlockDebugLog, bSendToDebugger, bSendToFile;
#endif

#ifdef _DEBUG
    int __stdcall _MDEBUG_TRAP(LPCSTR asFile, int anLine);
    extern int MDEBUG_CHK;
    extern char gsz_MDEBUG_TRAP_MSG_APPEND[2000];
    #define MDEBUG_TRAP1(S1) {strcpy(gsz_MDEBUG_TRAP_MSG_APPEND,(S1));_MDEBUG_TRAP(__FILE__,__LINE__);}
    #include <crtdbg.h>
    #define MCHKHEAP { MDEBUG_CHK=_CrtCheckMemory(); if (!MDEBUG_CHK) { MDEBUG_TRAP1("_CrtCheckMemory failed"); } }
    //#define MCHKHEAP
#else
    #define MCHKHEAP
#endif
