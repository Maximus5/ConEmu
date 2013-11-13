
/*
Copyright (c) 2009-2013 Maximus5
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

#undef SHOW_AUTOSCROLL

#define NEWMOUSESTYLE
#define NEWRUNSTYLE
//#undef NEWRUNSTYLE

#define APPDISTINCTBACKGROUND

//#define USEPORTABLEREGISTRY
#undef USEPORTABLEREGISTRY

#include <windows.h>
#include <Shlwapi.h>
//#include <vector>
//#if !defined(__GNUC__)
//#include <crtdbg.h>
//#endif

#include "../common/Memory.h"
#include "../common/MAssert.h"

#ifdef __GNUC__
#define wmemmove_s(d,ds,s,ss) wmemmove(d,s,ss)
#define SecureZeroMemory(p,s) memset(p,0,s)
#endif

#ifndef TimeGetTime
	#ifdef __GNUC__
		#define TimeGetTime GetTickCount
	#else
		#define TimeGetTime timeGetTime
	#endif
#endif

#ifdef KL_MEM
#include "c:\\lang\\kl.h"
#else
#include "kl_parts.h"
#endif

// Array sizes
#define MAX_CONSOLE_COUNT 30


#include "globals.h"
#include "resource.h"


#define DRAG_L_ALLOWED        0x0001
#define DRAG_R_ALLOWED        0x0002
#define DRAG_L_STARTED        0x0010
#define DRAG_R_STARTED        0x0020
#define MOUSE_R_LOCKED        0x0040
#define MOUSE_SIZING_BEGIN    0x0080
#define MOUSE_SIZING_TODO     0x0100
#define MOUSE_SIZING_DBLCKL   0x0200
#define MOUSE_DRAGPANEL_SPLIT 0x0400
#define MOUSE_DRAGPANEL_LEFT  0x0800
#define MOUSE_DRAGPANEL_RIGHT 0x1000
#define MOUSE_DRAGPANEL_SHIFT 0x2000
#define MOUSE_WINDOW_DRAG     0x4000

#define MOUSE_DRAGPANEL_ALL (MOUSE_DRAGPANEL_SPLIT|MOUSE_DRAGPANEL_LEFT|MOUSE_DRAGPANEL_RIGHT|MOUSE_DRAGPANEL_SHIFT)

#define MAX_TITLE_SIZE 0x400

#define CON_RECREATE_TIMEOUT 30000
#define CON_REDRAW_TIMOUT 5

#ifndef CLEARTYPE_NATURAL_QUALITY
#define CLEARTYPE_QUALITY       5
#define CLEARTYPE_NATURAL_QUALITY       6
#endif

#ifndef EVENT_CONSOLE_START_APPLICATION
#define EVENT_CONSOLE_START_APPLICATION 0x4006
#define EVENT_CONSOLE_END_APPLICATION   0x4007
#endif

#if !defined(CONSOLE_APPLICATION_16BIT)
#define CONSOLE_APPLICATION_16BIT       0x0001
#endif


#define TIMER_MAIN_ID 0
#define TIMER_MAIN_ELAPSE 500
#define TIMER_CONREDRAW_ID 1
#define TIMER_CAPTION_APPEAR_ID 3
#define TIMER_CAPTION_DISAPPEAR_ID 4
#define TIMER_RCLICKPAINT 5
#define TIMER_RCLICKPAINT_ELAPSE 20
#define TIMER_ADMSHIELD_ID 7
#define TIMER_ADMSHIELD_ELAPSE 1000
#define TIMER_RUNQUEUE_ID 8
#define TIMER_QUAKE_AUTOHIDE_ID 9
#define TIMER_QUAKE_AUTOHIDE_ELAPSE 100
#define TIMER_FAILED_TABBAR_ID 10 // FAILED_TABBAR_TIMERID
#define TIMER_FAILED_TABBAR_ELAPSE 3000 // FAILED_TABBAR_TIMEOUT 

#define ACTIVATE_TAB_CRITICAL  300
#define POST_UPDATE_TIMEOUT   2000

// Undocumented console message
#define WM_SETCONSOLEINFO           (WM_USER+201)
// and others
#define SC_RESTORE_SECRET 0x0000f122
#define SC_MAXIMIZE_SECRET 0x0000f032
#define SC_PROPERTIES_SECRET 0x0000fff7
#define SC_MARK_SECRET 0x0000fff2
#define SC_COPY_ENTER_SECRET 0x0000fff0
#define SC_PASTE_SECRET 0x0000fff1
#define SC_SELECTALL_SECRET 0x0000fff5
#define SC_SCROLL_SECRET 0x0000fff3
#define SC_FIND_SECRET 0x0000fff4
#define SC_SYSMENUPOPUP_SECRET 0x0000f093

#define MAX_CMD_HISTORY 100
#define MAX_CMD_HISTORY_SHOW 16
#define MAX_CMD_GROUP_SHOW 10

#define MAX_RENAME_TAB_LEN 128

#define MBox(rt) MessageBox(rt, /*MB_SYSTEMMODAL |*/ MB_ICONINFORMATION, Title) // {BOOL b = gbDontEnable; gbDontEnable = TRUE; (int)MessageBox(gbMessagingStarted ? ghWnd : NULL, rt, Title, /*MB_SYSTEMMODAL |*/ MB_ICONINFORMATION); gbDontEnable = b; }
#define MBoxA(rt) MessageBox(rt, /*MB_SYSTEMMODAL |*/ MB_ICONINFORMATION, NULL) // {BOOL b = gbDontEnable; gbDontEnable = TRUE; (int)MessageBox(gbMessagingStarted ? ghWnd : NULL, rt, _T("ConEmu"), /*MB_SYSTEMMODAL |*/ MB_ICONINFORMATION); gbDontEnable = b; }
#define MBoxError(rt) MessageBox(rt, /*MB_SYSTEMMODAL |*/ MB_ICONSTOP, NULL) // {BOOL b = gbDontEnable; gbDontEnable = TRUE; (int)MessageBox(gbMessagingStarted ? ghWnd : NULL, rt, _T("ConEmu"), /*MB_SYSTEMMODAL |*/ MB_ICONSTOP); gbDontEnable = b; }

//#define MBoxAssert(V) if ((V)==FALSE) { TCHAR szAMsg[MAX_PATH*2]; StringCchPrintf(szAMsg, countof(szAMsg), _T("Assertion (%s) at\n%s:%i"), _T(#V), _T(__FILE__), __LINE__); MBoxA(szAMsg); }
#define MBoxAssert(V) _ASSERTE(V)
//__inline BOOL isMeForeground() {
//	HWND h = GetForegroundWindow();
//	return h && (h == ghWnd || h == ghOpWnd || h == ghConWnd);
//}
//#endif
//#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)
//#define isDriveLetter(c) ((c>=L'A' && c<=L'Z') || (c>=L'a' && c<=L'z'))
//#define isDigit(c) (c>=L'0' && c<=L'9')

//#define PTDIFFTEST(C,D) (((abs(C.x-LOWORD(lParam)))<D) && ((abs(C.y-HIWORD(lParam)))<D))

//#define INVALIDATE() InvalidateRect(HDCWND, NULL, FALSE)

//#define SafeCloseHandle(h) { if ((h)!=NULL) { HANDLE hh = (h); (h) = NULL; if (hh!=INVALID_HANDLE_VALUE) CloseHandle(hh); } }
//#define SafeFree(p) { if ((p)!=NULL) { LPVOID pp = (p); (p) = NULL; free(pp); } }

#ifdef MSGLOGGER
BOOL POSTMESSAGE(HWND h,UINT m,WPARAM w,LPARAM l,BOOL extra);
LRESULT SENDMESSAGE(HWND h,UINT m,WPARAM w,LPARAM l);
#define SETWINDOWPOS(hw,hp,x,y,w,h,f) {MCHKHEAP; DebugLogPos(hw,x,y,w,h,"SetWindowPos"); SetWindowPos(hw,hp,x,y,w,h,f);}
#define MOVEWINDOW(hw,x,y,w,h,r) {MCHKHEAP; DebugLogPos(hw,x,y,w,h,"MoveWindow"); MoveWindow(hw,x,y,w,h,r);}
#define SETCONSOLESCREENBUFFERSIZERET(h,s,r) {MCHKHEAP; DebugLogBufSize(h,s); r=SetConsoleScreenBufferSize(h,s);}
#define SETCONSOLESCREENBUFFERSIZE(h,s) {BOOL lb; SETCONSOLESCREENBUFFERSIZERET(h,s,lb);}
#define DEBUGLOGFILE(m) DebugLogFile(m)
void DebugLogFile(LPCSTR asMessage);
void DebugLogBufSize(HANDLE h, COORD sz);
void DebugLogPos(HWND hw, int x, int y, int w, int h, LPCSTR asFunc);
void DebugLogMessage(HWND h, UINT m, WPARAM w, LPARAM l, BOOL posted, BOOL extra);
#else
#define POSTMESSAGE(h,m,w,l,e) PostMessage(h,m,w,l)
#define SENDMESSAGE(h,m,w,l) SendMessage(h,m,w,l)
#define SETWINDOWPOS(hw,hp,x,y,w,h,f) SetWindowPos(hw,hp,x,y,w,h,f)
#define MOVEWINDOW(hw,x,y,w,h,r) MoveWindow(hw,x,y,w,h,r)
#define SETCONSOLESCREENBUFFERSIZERET(h,s,r) r=SetConsoleScreenBufferSize(h,s)
#define SETCONSOLESCREENBUFFERSIZE(h,s) SetConsoleScreenBufferSize(h,s)
#define DEBUGLOGFILE(m)
#endif

void LogString(LPCWSTR asInfo, bool abWriteTime = true, bool abWriteLine = true);

//#if !defined(__GNUC__)
//#pragma warning (disable : 4005)
//#if !defined(_WIN32_WINNT)
//#define _WIN32_WINNT 0x0502
//#endif
//#endif




//------------------------------------------------------------------------
///| Code optimizing |////////////////////////////////////////////////////
//------------------------------------------------------------------------

//#if !defined(__GNUC__)
//#include <intrin.h>
//#pragma function(memset, memcpy)
//__forceinline void *memset(void *_Dst, int _Val, size_t _Size)
//{
//	__stosb((unsigned char*)_Dst, _Val, _Size);
//	return _Dst;
//}
//__forceinline void *memcpy(void *_Dst, const void *_Src, size_t _Size)
//{
//	__movsb((unsigned char*)_Dst, (unsigned const char*)_Src, _Size);
//	return _Dst;
//}
//#endif

extern BOOL gbInDisplayLastError;
int DisplayLastError(LPCTSTR asLabel, DWORD dwError = 0, DWORD dwMsgFlags = 0, LPCWSTR asTitle = NULL, HWND hParent = NULL);
RECT CenterInParent(RECT rcDlg, HWND hParent);

void ShutdownGuiStep(LPCWSTR asInfo, int nParm1 = 0, int nParm2 = 0, int nParm3 = 0, int nParm4 = 0);
void LogFocusInfo(LPCWSTR asInfo, int Level=1);

COORD /*__forceinline*/ MakeCoord(int X,int Y);
//{
//	COORD rc; rc.X=W; rc.Y=H;
//	return rc;
//}

POINT /*__forceinline*/ MakePoint(int X,int Y);

RECT /*__forceinline*/ MakeRect(int W,int H);
//{
//	RECT rc; rc.left=0; rc.top=0; rc.right=W; rc.bottom=H;
//	return rc;
//}

RECT /*__forceinline*/ MakeRect(int X1, int Y1,int X2,int Y2);
//{
//	RECT rc; rc.left=X1; rc.top=Y1; rc.right=X2; rc.bottom=Y2;
//	return rc;
//}

BOOL /*__forceinline*/ CoordInRect(const COORD& c, const RECT& r);
//{
//	return (c.X >= r.left && c.X <= r.right) && (c.Y >= r.top && c.Y <= r.bottom);
//}

BOOL IntersectSmallRect(RECT& rc1, SMALL_RECT& rc2);

wchar_t* GetDlgItemText(HWND hDlg, WORD nID);
size_t MyGetDlgItemText(HWND hDlg, WORD nID, size_t& cchMax, wchar_t*& pszText/*, bool bEscapes = false*/);
BOOL MySetDlgItemText(HWND hDlg, int nIDDlgItem, LPCTSTR lpString/*, bool bEscapes = false*/);
bool GetColorRef(LPCWSTR pszText, COLORREF* pCR);

extern const wchar_t* gsEscaped;
//wchar_t* EscapeString(bool bSet, LPCWSTR pszSrc);
void EscapeChar(bool bSet, LPCWSTR& pszSrc, LPWSTR& pszDst);

//#pragma warning(disable: 4311) // 'type cast' : pointer truncation from 'HBRUSH' to 'BOOL'

wchar_t* DupCygwinPath(LPCWSTR asWinPath, bool bAutoQuote);
wchar_t* SelectFolder(LPCWSTR asTitle, LPCWSTR asDefFolder = NULL, HWND hParent = ghWnd, bool bAutoQuote = true, bool bCygwin = false);
wchar_t* SelectFile(LPCWSTR asTitle, LPCWSTR asDefFile = NULL, HWND hParent = ghWnd, LPCWSTR asFilter = NULL, bool abAutoQuote = true, bool bCygwin = false, bool bSaveNewFile = false);

BOOL CreateProcessRestricted(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
							 LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
							 BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
							 LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
							 LPDWORD pdwLastError = NULL);


#include "../common/RConStartArgs.h"


bool isKey(DWORD wp,DWORD vk);
bool isConsoleService(LPCWSTR pszProcessName);
RealBufferScroll ParseBufferSizeParm(wchar_t* psNumArg, wchar_t** ppsEnd, COORD& rsBufferSize);

void RaiseTestException();

//------------------------------------------------------------------------
///| Registry |///////////////////////////////////////////////////////////
//------------------------------------------------------------------------

struct SettingsStorage
{
	wchar_t szType[8]; // CONEMU_CONFIGTYPE_REG, CONEMU_CONFIGTYPE_XML
	LPCWSTR pszFile;   // NULL или полный путь к xml-файлу
	LPCWSTR pszConfig; // »м€ конфигурации
};

#define CONEMU_ROOT_KEY L"Software\\ConEmu"
#define CONEMU_CONFIGTYPE_REG L"[reg]"
#define CONEMU_CONFIGTYPE_XML L"[xml]"
#define CONEMU_CONFIGTYPE_INI L"[ini]"

#include "Registry.h"

//------------------------------------------------------------------------
///| Global variables |///////////////////////////////////////////////////
//------------------------------------------------------------------------

//#include "VirtualConsole.h"
//#include "options.h"
//#include "DragDrop.h"
//#include "TrayIcon.h"
//#include "VConChild.h"
//#include "ConEmu.h"
//#include "ConEmuApp.h"
//#include "tabbar.h"
//#include "TrayIcon.h"
//#include "ConEmuPipe.h"
#include "../common/UnicodeChars.h"
#include "../common/defines.h"
#include "../common/WinObjects.h"
#include "../common/CmdLine.h"

#define IsWindowsXP ((gOSVer.dwMajorVersion >= 6) || (gOSVer.dwMajorVersion == 5 && gOSVer.dwMinorVersion > 0))
#define IsWindowsVista (gOSVer.dwMajorVersion >= 6)
#define IsWindows7 ((gOSVer.dwMajorVersion > 6) || (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion > 0))
#define IsWindows8 ((gOSVer.dwMajorVersion > 6) || (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion > 1))

#define SafeRelease(p) if ((p)!=NULL) { (p)->Release(); (p)=NULL; }

// GNU C HEADER PATCH
#ifdef __GNUC__
typedef BOOL (WINAPI* AlphaBlend_t)(HDC hdcDest, int xoriginDest, int yoriginDest, int wDest, int hDest, HDC hdcSrc, int xoriginSrc, int yoriginSrc, int wSrc, int hSrc, BLENDFUNCTION ftn);
typedef BOOL (WINAPI* SetLayeredWindowAttributes_t)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
typedef BOOL (WINAPI* CreateRestrictedToken_t)(HANDLE ExistingTokenHandle, DWORD Flags, DWORD DisableSidCount, PSID_AND_ATTRIBUTES SidsToDisable, DWORD DeletePrivilegeCount, PLUID_AND_ATTRIBUTES PrivilegesToDelete, DWORD RestrictedSidCount, PSID_AND_ATTRIBUTES SidsToRestrict, PHANDLE NewTokenHandle);
#endif
// GetLayeredWindowAttributes по€вилась только в XP
typedef BOOL (WINAPI* GetLayeredWindowAttributes_t)(HWND hwnd, COLORREF *pcrKey, BYTE *pbAlpha, DWORD *pdwFlags);

#ifdef __GNUC__
	#define MAPVK_VK_TO_VSC     (0)
	#define MAPVK_VSC_TO_VK     (1)
	#define MAPVK_VK_TO_CHAR    (2)
	#define MAPVK_VSC_TO_VK_EX  (3)
#endif

#ifndef SEE_MASK_NOASYNC
	#define SEE_MASK_NOASYNC    0x00000100   // block on the call until the invoke has completed, use for callers that exit after calling ShellExecuteEx()
#endif

#ifndef TTM_SETTITLEW
	#define TTM_SETTITLEW           (WM_USER + 33)  // wParam = TTI_*, lParam = wchar* szTitle
#endif
#ifndef TTM_SETTITLE
	#define TTM_SETTITLE            TTM_SETTITLEW
#endif
#ifndef TTI_WARNING
	#define TTI_WARNING             2
#endif

#ifndef INPUTLANGCHANGE_SYSCHARSET
#define INPUTLANGCHANGE_SYSCHARSET 0x0001
#endif

#ifndef DISABLE_MAX_PRIVILEGE
#define DISABLE_MAX_PRIVILEGE   0x1 
#endif


enum ConEmuMargins
{
	// –азница между размером всего окна и клиентской области окна (рамка + заголовок)
	CEM_FRAMEONLY = 0x0001,
	CEM_CAPTION = 0x0002,
	CEM_FRAMECAPTION = (CEM_FRAMEONLY|CEM_CAPTION),
	CEM_CLIENTSHIFT = 0x0004, // ≈сли клиентска€ часть "расширена" на рамку
	// ¬ысота таба (пока только .top)
	CEM_TAB = 0x0010,
	CEM_TABACTIVATE = 0x1010,   // ѕринудительно считать, что таб есть (при включении таба)
	CEM_TABDEACTIVATE = 0x2010, // ѕринудительно считать, что таба нет (при отключении таба)
	CEM_TAB_MASK = (CEM_TAB|CEM_TABACTIVATE|CEM_TABDEACTIVATE),
	CEM_SCROLL = 0x0020, // ≈сли полоса прокрутки всегда (!!!) видна - то ее ширина/высота
	CEM_STATUS = 0x0040, // ¬ысота строки статуса
	CEM_PAD = 0x0080, // Ўирина "отступа" от краев
	// ћаска дл€ получени€ всех отступов
	CEM_ALL_MARGINS = CEM_FRAMECAPTION|CEM_TAB|/*CEM_SCROLL|*/CEM_STATUS/*|CEM_PAD*/,
	CEM_CLIENT_MARGINS = CEM_TAB|/*CEM_SCROLL|*/CEM_STATUS/*|CEM_PAD*/,
};

enum ConEmuRect
{
	CER_MAIN = 0,   // ѕолный размер окна
	// ƒалее все координаты считаютс€ относительно клиенсткой области {0,0}
	CER_MAINCLIENT, // клиентска€ область главного окна (Ѕ≈« отрезани€ табов, прокруток, DoubleView и прочего. ÷еликом)
	CER_TAB,        // положение контрола с закладками (всего)
	CER_WORKSPACE,  // рабоча€ область ConEmu. ¬ ней располагаютс€ все видимые VCon/GUI apps. (ЅќЋ№Ў≈ чем CER_BACK при SplitScreen/DoubleView).
	CER_BACK,       // область, отведенна€ под VCon. “ут нужна вс€ область, без отрезани€ прокруток и округлений размеров под знакоместо
	CER_SCROLL,     // положение полосы прокрутки
	CER_DC,         // положение окна отрисовки
	CER_CONSOLE_ALL,// !!! _ размер в символах _ !!! размер всего пол€ (всех видимых сплитов)
	CER_CONSOLE_CUR,// !!! _ размер в символах _ !!! размер активной консоли (активный сплит)
	CER_CONSOLE_NTVDMOFF, // same as CER_CONSOLE, но во врем€ отключени€ режима 16бит
	CER_FULLSCREEN, // полный размер в pix текущего монитора (содержащего ghWnd)
	CER_MAXIMIZED,  // размер максимизированного окна на текущем мониторе (содержащего ghWnd)
	CER_RESTORE,    // размер "восстановленного" окна после максимизации (коррекци€ по размеру монитора?)
	CER_MONITOR,    // полный размер в pix рабочей области текущего монитора (содержащего ghWnd)
//	CER_CORRECTED   // скорректированное положение (чтобы окно было видно на текущем мониторе)
};

enum ConEmuBorders
{
	CEB_TOP = 1,
	CEB_LEFT = 2,
	CEB_BOTTOM = 4,
	CEB_RIGHT = 8,
	CEB_ALL = CEB_TOP|CEB_LEFT|CEB_BOTTOM|CEB_RIGHT,
	// Next means "place window OnScreen when it out of screen totally"
	CEB_ALLOW_PARTIAL = 16,
};

enum DragPanelBorder
{
	DPB_NONE = 0,
	DPB_SPLIT,    // драг влево/вправо
	DPB_LEFT,     // высота левой
	DPB_RIGHT,    // высота правой
};

enum TrackMenuPlace
{
	tmp_None = 0,
	tmp_System,
	tmp_VCon,
	tmp_Cmd, // Tasks & Last commands
	tmp_CmdPopup, // Task contents (RClick) -- 4del
	tmp_KeyBar,
	tmp_TabsList,
	tmp_PasteCmdLine,
	tmp_StatusBarCols,
	tmp_ChildSysMenu,
};

enum ConEmuWindowMode
{
	wmCurrent = 0,
	wmNotChanging = -1,
	wmNormal = rNormal,
	wmMaximized = rMaximized,
	wmFullScreen = rFullScreen,
};

enum ExpandTextRangeType
{
	etr_None = 0,
	etr_Word = 1,
	etr_FileAndLine = 2,
	etr_Url = 3,
};

enum BackgroundOp
{
	eUpLeft = 0,
	eStretch = 1,
	eTile = 2,
	eUpRight = 3,
	eDownLeft = 4,
	eDownRight = 5,
	eFit = 6, // Stretch aspect ratio (Fit)
	eFill = 7, // Stretch aspect ratio (Fill)
	eCenter = 8,
	//
	eOpLast = eCenter,
};

enum ToolbarMainBitmapIdx
{
	BID_FIST_CON = 0,
	BID_LAST_CON = (MAX_CONSOLE_COUNT-1),
	BID_NEWCON_IDX,
	BID_ALTERNATIVE_IDX,
	BID_MINIMIZE_IDX,
	BID_MAXIMIZE_IDX,
	BID_RESTORE_IDX,
	BID_APPCLOSE_IDX,
	BID_DUMMYBTN_IDX,
	BID_TOOLBAR_LAST_IDX,
};

enum ToolbarCommandIdx
{
	TID_ACTIVE_NUMBER = 1,
	TID_CREATE_CON,
	TID_ALTERNATIVE,
	TID_SCROLL,
	TID_MINIMIZE,
	TID_MAXIMIZE,
	TID_APPCLOSE,
	TID_COPYING,
	TID_MINIMIZE_SEP = 110,
};

enum SwitchGuiFocusOp
{
	sgf_None = 0,
	sgf_FocusSwitch,
	sgf_FocusGui,
	sgf_FocusChild,
	sgf_Last
};

enum CEStatusFlags
{
	csf_VertDelim           = 0x00000001,
	csf_HorzDelim           = 0x00000002,
	csf_SystemColors        = 0x00000004,
	csf_NoVerticalPad       = 0x00000008,
};

enum CESizeStyle
{
	ss_Standard = 0,
	ss_Pixels   = 1,
	ss_Percents = 2,
};

union CESize
{
	DWORD Raw;

	struct
	{
		int         Value: 24;
		CESizeStyle Style: 8;

		wchar_t TempSZ[12];
	};

	const wchar_t* AsString()
	{
		switch (Style)
		{
		case ss_Pixels:
			_wsprintf(TempSZ, SKIPLEN(countof(TempSZ)) L"%ipx", Value);
			break;
		case ss_Percents:
			_wsprintf(TempSZ, SKIPLEN(countof(TempSZ)) L"%i%%", Value);
			break;
		//case ss_Standard:
		default:
			_wsprintf(TempSZ, SKIPLEN(countof(TempSZ)) L"%i", Value);
		}
		return TempSZ;
	};

	bool IsValid(bool IsWidth) const
	{
		bool bValid;
		switch (Style)
		{
		case ss_Percents:
			bValid = (Value >= 1 && Value <= 100);
			break;
		case ss_Pixels:
			// Treat width/height as values for font size 4x2 (minimal)
			if (IsWidth)
				bValid = (Value >= (MIN_CON_WIDTH*4));
			else
				bValid = (Value >= (MIN_CON_HEIGHT*2));
			break;
		default:
			if (IsWidth)
				bValid = (Value >= MIN_CON_WIDTH);
			else
				bValid = (Value >= MIN_CON_HEIGHT);
		}
		return bValid;
	};

	bool Set(bool IsWidth, CESizeStyle NewStyle, int NewValue)
	{
		if (NewStyle == ss_Standard)
		{
			int nDef = IsWidth ? 80 : 25;
			int nMax = IsWidth ? 1000 : 500;
			if (NewValue <= 0) NewValue = nDef; else if (NewValue > nMax) NewValue = nMax;
		}
		else if (NewStyle == ss_Percents)
		{
			int nDef = IsWidth ? 50 : 30;
			int nMax = 100;
			if (NewValue <= 0) NewValue = nDef; else if (NewValue > nMax) NewValue = nMax;
		}

		if (!NewValue)
		{
			// Size can't be empty
			_ASSERTE(NewValue);
			// Fail
			return false;
		}

		Value = NewValue;
		Style = NewStyle;
		return true;
	};

	void SetFromRaw(bool IsWidth, DWORD Raw)
	{
		CESize v; v.Raw = Raw;
		if (v.Style == ss_Standard || v.Style == ss_Pixels || v.Style == ss_Percents)
		{
			this->Set(IsWidth, v.Style, v.Value);
		}
	};

	bool SetFromString(bool IsWidth, const wchar_t* sValue)
	{
		if (!sValue || !*sValue)
			return false;
		wchar_t* pszEnd = NULL;
		// Try to convert
		int NewValue = wcstol(sValue, &pszEnd, 10);
		if (!NewValue)
			return false;

		CESizeStyle NewStyle = ss_Standard;
		if (pszEnd)
		{
			switch (*SkipNonPrintable(pszEnd))
			{
			case L'%':
				NewStyle = ss_Percents; break;
			case L'p':
				NewStyle = ss_Pixels; break;
			}
		}
		// Done
		return Set(IsWidth, NewStyle, NewValue);
	};
};


bool CheckLockFrequentExecute(DWORD& Tick, DWORD Interval);
#define LockFrequentExecute(Interval,LastExecuteTick) if (CheckLockFrequentExecute(LastExecuteTick,Interval))
#define LockFrequentExecuteStatic(Interval) static DWORD LastExecuteTick; if (CheckLockFrequentExecute(LastExecuteTick,Interval))

extern const wchar_t* gsHomePage;    // = L"http://conemu-maximus5.googlecode.com";
extern const wchar_t* gsReportBug;   // = L"http://code.google.com/p/conemu-maximus5/issues/entry";
extern const wchar_t* gsReportCrash; // = L"http://code.google.com/p/conemu-maximus5/issues/entry";
extern const wchar_t* gsWhatsNew;    // = L"http://code.google.com/p/conemu-maximus5/wiki/Whats_New";

template <class T>
T GetMinMax(T a, int v1, int v2)
{
	if (a < (T)v1)
		a = (T)v1;
	else if (a > (T)v2)
		a = (T)v2;
	return a;
}

template <class T>
void MinMax(T &a, int v1, int v2)
{
	if (a < (T)v1)
		a = (T)v1;
	else if (a > (T)v2)
		a = (T)v2;
}

template <class T>
void MinMax(T &a, int v2)
{
	if (a > (T)v2)
		a = (T)v2;
}


#include <pshpack1.h>
typedef struct tagMYRGB
{
	union
	{
		COLORREF color;
		struct
		{
			BYTE    rgbBlue;
			BYTE    rgbGreen;
			BYTE    rgbRed;
			BYTE    rgbReserved;
		};
	};
} MYRGB, MYCOLORREF;
#include <poppack.h>

#define CmdFilePrefix     L'@'
#define DropLnkPrefix     L'?'
#define TaskBracketLeft   L'{'
#define TaskBracketRight  L'}'
#define AutoStartTaskName L"<Startup>"

bool NextLine(const wchar_t*& pszFrom, wchar_t** pszLine);
