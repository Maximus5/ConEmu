

/*
Copyright (c) 2009-2017 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

extern const TCHAR *const gsClassName;
extern const TCHAR *const gsClassNameParent;
extern const TCHAR *const gsClassNameWork;
extern const TCHAR *const gsClassNameBack;
extern const TCHAR *const gsClassNameApp;


struct Settings;
class CSettings;
class TrayIcon;
class TabBarClass;
class CVirtualConsole;
class CRealConsole;
class CConEmuMain;
class CConEmuChild;
class CConEmuBack;
class CDragDrop;


//extern CVirtualConsole *pVCon;
extern HINSTANCE g_hInstance;
extern CConEmuMain *gpConEmu;
//extern TCHAR Title[0x400];
//extern bool isLBDown, /*isInDrag,*/ isDragProcessed;
extern HWND ghWnd, ghWndWork, ghWndApp, ghWndDrag;
extern WPARAM gnWndSetHotkey, gnWndSetHotkeyOk;
extern HWND ghOpWnd;
#ifdef _DEBUG
extern HWND ghConWnd;
#endif

extern bool gbMessagingStarted;

class DontEnable
{
private:
	LONG nPrev; // Informational!
	static LONG gnDontEnable;
	static LONG gnDontEnableCount;
	bool bLocked;
public:
	DontEnable(bool abLock = true);
	~DontEnable();
public:
	static bool isDontEnable();
};

extern OSVERSIONINFO gOSVer;
extern WORD gnOsVer;
extern bool gbIsWine;
extern bool gbIsDBCS;
extern wchar_t gsDefGuiFont[32];
extern wchar_t gsAltGuiFont[32];
extern wchar_t gsDefConFont[32];
extern wchar_t gsAltConFont[32];
extern wchar_t gsDefMUIFont[32];
//extern SECURITY_ATTRIBUTES* gpLocalSecurity;
extern BOOL gbDebugLogStarted;
extern BOOL gbDebugShowRects;

//extern const int TAB_FONT_HEIGTH;
//extern wchar_t TAB_FONT_FACE[];


//extern TCHAR szIconPath[MAX_PATH];
extern HICON hClassIcon, hClassIconSm;

extern Settings  *gpSet;
extern CSettings *gpSetCls;
extern TrayIcon Icon;
//extern TCHAR temp[MAX_PATH];

extern BOOL gbNoDblBuffer;

#ifdef _DEBUG
extern char gsz_MDEBUG_TRAP_MSG[3000];
extern char gsz_MDEBUG_TRAP_MSG_APPEND[2000];
extern HWND gh_MDEBUG_TRAP_PARENT_WND;
#endif


#ifdef MSGLOGGER
extern BOOL bBlockDebugLog, bSendToDebugger, bSendToFile;
#endif

//#ifdef _DEBUG
//    int __stdcall _MDEBUG_TRAP(LPCSTR asFile, int anLine);
//    extern int MDEBUG_CHK;
//    extern char gsz_MDEBUG_TRAP_MSG_APPEND[2000];
//    #define MDEBUG_TRAP1(S1) {strcpy(gsz_MDEBUG_TRAP_MSG_APPEND,(S1));_MDEBUG_TRAP(__FILE__,__LINE__);}
//    #include <crtdbg.h>
//    #define MCHKHEAP { MDEBUG_CHK=_CrtCheckMemory(); if (!MDEBUG_CHK) { MDEBUG_TRAP1("_CrtCheckMemory failed"); } }
//    //#define MCHKHEAP
//#else
//    #define MCHKHEAP
//#endif

extern HWND ghLastForegroundWindow;
HWND getForegroundWindow();

extern HWND ghDlgPendingFrom;
extern LONG gnInMsgBox;
int MsgBox(LPCTSTR lpText, UINT uType, LPCTSTR lpCaption = NULL, HWND ahParent = (HWND)-1, bool abModal = true);
void AssertBox(LPCTSTR szText, LPCTSTR szFile, UINT nLine, LPEXCEPTION_POINTERS ExceptionInfo = NULL);
void PatchMsgBoxIcon(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);

#include "../ConEmu/version_stage.h"
#if ConEmuVersionStage == CEVS_STABLE
#define Assert(V)
#define AssertMsg(V)
#else
#define Assert(V) if ((V)==FALSE) { AssertBox(_T(#V), _T(__FILE__), __LINE__); }
#define AssertMsg(V) AssertBox(V, _T(__FILE__), __LINE__);
#endif

struct NestedCallCounter
{
	LONG& ncl;
	NestedCallCounter(LONG& NestedCallLevel)
		: ncl(NestedCallLevel)
	{
		InterlockedIncrement(&ncl);
	};
	~NestedCallCounter()
	{
		InterlockedDecrement(&ncl);
	};
};

#define NestedCallAssert(level) \
	static LONG NestedCallLevel = 0; \
	NestedCallCounter ncCounter(NestedCallLevel); \
	Assert(NestedCallLevel <= level);
