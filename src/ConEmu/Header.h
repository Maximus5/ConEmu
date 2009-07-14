#pragma once

#define NEWMOUSESTYLE
#define NEWRUNSTYLE
//#undef NEWRUNSTYLE

#include <windows.h>
#include <Shlwapi.h>
#include <vector>
#if !defined(__GNUC__)
#include <crtdbg.h>
#endif

#ifdef KL_MEM
#include "c:\\lang\\kl.h"
#else
#include "kl_parts.h"
#endif

// Array sizes
#define MAX_CONSOLE_COUNT 12


#include "globals.h"
#include "resource.h"


#define DRAG_L_ALLOWED 0x01
#define DRAG_R_ALLOWED 0x02
#define DRAG_L_STARTED 0x10
#define DRAG_R_STARTED 0x20
#define MOUSE_R_LOCKED 0x40
#define MOUSE_SIZING_BEGIN  0x080
#define MOUSE_SIZING_TODO   0x100
#define MOUSE_SIZING_DBLCKL 0x200
//#define MOUSE_SIZING   0x80

//#define CONMAN_FIRSTCONSOLE 0
//#define CONMAN_LASTCONSOLE (MAX_CONSOLE_COUNT-1)
//#define CONMAN_NEWCONSOLE 20
//#define CONMAN_NEXTCONSOLE 21
//#define CONMAN_PREVCONSOLE 22
//#define CONMAN_ALTCONSOLE 15
//
//#define CONMAN_RECREATE 100
//...
//#define CONMAN_RECREATELAST (CONMAN_RECREATE+MAX_CONSOLE_COUNT-1) // это все зарезервировано

#define CON_RECREATE_TIMEOUT 30000

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


#define MBox(rt) {BOOL b = gbDontEnable; gbDontEnable = TRUE; (int)MessageBox(gbMessagingStarted ? ghWnd : NULL, rt, Title, /*MB_SYSTEMMODAL |*/ MB_ICONINFORMATION); gbDontEnable = b; }
#define MBoxA(rt) {BOOL b = gbDontEnable; gbDontEnable = TRUE; (int)MessageBox(gbMessagingStarted ? ghWnd : NULL, rt, _T("ConEmu"), /*MB_SYSTEMMODAL |*/ MB_ICONINFORMATION); gbDontEnable = b; }
#define MBoxAssert(V) if ((V)==FALSE) { TCHAR szAMsg[MAX_PATH*2]; wsprintf(szAMsg, _T("Assertion (%s) at\n%s:%i"), _T(#V), _T(__FILE__), __LINE__); MBoxA(szAMsg); }
//__inline BOOL isMeForeground() {
//	HWND h = GetForegroundWindow();
//	return h && (h == ghWnd || h == ghOpWnd || h == ghConWnd);
//}
//#endif
#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)
#define isDriveLetter(c) ((c>=L'A' && c<=L'Z') || (c>=L'a' && c<=L'z'))
#define isDigit(c) (c>=L'0' && c<=L'9')

//#define PTDIFFTEST(C,D) (((abs(C.x-LOWORD(lParam)))<D) && ((abs(C.y-HIWORD(lParam)))<D))

//#define INVALIDATE() InvalidateRect(HDCWND, NULL, FALSE)

#define SafeCloseHandle(h) { if ((h)!=NULL) { HANDLE hh = (h); (h) = NULL; if (hh!=INVALID_HANDLE_VALUE) CloseHandle(hh); } }
#define SafeFree(p) { if ((p)!=NULL) { LPVOID pp = (p); (p) = NULL; free(pp); } }

#ifdef MSGLOGGER
#define POSTMESSAGE(h,m,w,l,e) {MCHKHEAP; DebugLogMessage(h,m,w,l,TRUE,e); PostMessage(h,m,w,l);}
#define SENDMESSAGE(h,m,w,l) {MCHKHEAP;  DebugLogMessage(h,m,w,l,FALSE,FALSE); SendMessage(h,m,w,l);}
#define SETWINDOWPOS(hw,hp,x,y,w,h,f) {MCHKHEAP; DebugLogPos(hw,x,y,w,h,"SetWindowPos"); SetWindowPos(hw,hp,x,y,w,h,f);}
#define MOVEWINDOW(hw,x,y,w,h,r) {MCHKHEAP; DebugLogPos(hw,x,y,w,h,"MoveWindow"); MoveWindow(hw,x,y,w,h,r);}
#define SETCONSOLESCREENBUFFERSIZERET(h,s,r) {MCHKHEAP; DebugLogBufSize(h,s); r=SetConsoleScreenBufferSize(h,s);}
#define SETCONSOLESCREENBUFFERSIZE(h,s) {BOOL lb; SETCONSOLESCREENBUFFERSIZERET(h,s,lb);}
#define DEBUGLOGFILE(m) DebugLogFile(m)
#else
#define POSTMESSAGE(h,m,w,l,e) PostMessage(h,m,w,l)
#define SENDMESSAGE(h,m,w,l) SendMessage(h,m,w,l)
#define SETWINDOWPOS(hw,hp,x,y,w,h,f) SetWindowPos(hw,hp,x,y,w,h,f)
#define MOVEWINDOW(hw,x,y,w,h,r) MoveWindow(hw,x,y,w,h,r)
#define SETCONSOLESCREENBUFFERSIZERET(h,s,r) r=SetConsoleScreenBufferSize(h,s)
#define SETCONSOLESCREENBUFFERSIZE(h,s) SetConsoleScreenBufferSize(h,s)
#define DEBUGLOGFILE(m)
#endif


//#if !defined(__GNUC__)
//#pragma warning (disable : 4005)
//#if !defined(_WIN32_WINNT)
//#define _WIN32_WINNT 0x0502
//#endif
//#endif


typedef struct tag_RConStartArgs {
	BOOL     bDetached;
	wchar_t* pszSpecialCmd;
	BOOL     bRunAsAdministrator;
	BOOL     bRecreate; // !!! Информационно !!!

	tag_RConStartArgs() {
		bDetached = FALSE; pszSpecialCmd = NULL; bRunAsAdministrator = FALSE; bRecreate = FALSE;
	};
	~tag_RConStartArgs() {
		SafeFree(pszSpecialCmd); // именно SafeFree
	};
} RConStartArgs;


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

void DisplayLastError(LPCTSTR asLabel, DWORD dwError = 0);

COORD __forceinline MakeCoord(int W,int H)
{
	COORD rc; rc.X=W; rc.Y=H;
	return rc;
}

RECT __forceinline MakeRect(int W,int H)
{
	RECT rc; rc.left=0; rc.top=0; rc.right=W; rc.bottom=H;
	return rc;
}

RECT __forceinline MakeRect(int X1, int Y1,int X2,int Y2)
{
	RECT rc; rc.left=X1; rc.top=Y1; rc.right=X2; rc.bottom=Y2;
	return rc;
}

BOOL __forceinline CoordInRect(COORD& c, RECT& r)
{
	return (c.X >= r.left && c.X <= r.right) && (c.Y >= r.top && c.Y <= r.bottom);
}

//#pragma warning(disable: 4311) // 'type cast' : pointer truncation from 'HBRUSH' to 'BOOL'



//------------------------------------------------------------------------
///| Registry |///////////////////////////////////////////////////////////
//------------------------------------------------------------------------

class Registry
{

public:
	HKEY regMy;
	bool OpenKey(HKEY inHKEY, const TCHAR *regPath, uint access)
	{
		bool res = false;
		if (access == KEY_READ)
			res = RegOpenKeyEx(inHKEY, regPath, 0, KEY_READ, &regMy) == ERROR_SUCCESS;
		else
			res = RegCreateKeyEx(inHKEY, regPath, 0, NULL, 0, access, 0, &regMy, 0) == ERROR_SUCCESS;
		return res;
	}
	bool OpenKey(const TCHAR *regPath, uint access)
	{
		return OpenKey(HKEY_CURRENT_USER, regPath, access);
	}
	void CloseKey()
	{
		RegCloseKey(regMy);
	}

	template <class T> void Save(const TCHAR *regKey, T value)
	{
		DWORD len = sizeof(T);
		RegSetValueEx(regMy, regKey, NULL, (len == 4) ? REG_DWORD : REG_BINARY, (LPBYTE)(&value), len); 
	}
	void Save(const TCHAR *regKey, const TCHAR *value)
	{
		if (!value) value = _T(""); // сюда мог придти и NULL
		RegSetValueEx(regMy, regKey, NULL, REG_SZ, (LPBYTE)(value), _tcslen(value) * sizeof(TCHAR));
	}
	void Save(const TCHAR *regKey, TCHAR *value)
	{
		Save(regKey, (const TCHAR *)value);
	}

	template <class T> bool Load(const TCHAR *regKey, T &value)
	{
		DWORD len = sizeof(T);
		if (RegQueryValueEx(regMy, regKey, NULL, NULL, (LPBYTE)&(value), &len) == ERROR_SUCCESS)
			return true;
		return false;
	}
	/*bool Load(const TCHAR *regKey, TCHAR *value, UINT nMaxLen)
	{
		DWORD len = nMaxLen * sizeof(TCHAR);
		if (RegQueryValueEx(regMy, regKey, NULL, NULL, (LPBYTE)(value), &len) == ERROR_SUCCESS)
			return true;
		return false;
	}*/
	bool Load(const TCHAR *regKey, TCHAR **value)
	{
		DWORD len = 0;
		if (*value) {free(*value); *value = NULL;}
		if (RegQueryValueEx(regMy, regKey, NULL, NULL, NULL, &len) == ERROR_SUCCESS && len) {
			*value = (TCHAR*)malloc(len); **value = 0;
			if (RegQueryValueEx(regMy, regKey, NULL, NULL, (LPBYTE)(*value), &len) == ERROR_SUCCESS) {
				return true;
			}
		} else {
			*value = (TCHAR*)malloc(sizeof(TCHAR));
			**value = 0;
		}
		return false;
	}
};

//------------------------------------------------------------------------
///| Global variables |///////////////////////////////////////////////////
//------------------------------------------------------------------------

#define pHelp \
L"Console emulation program.\n" \
L"By default this program launches \"Far.exe\" from the same directory it is in.\n" \
L"\n" \
L"Command line switches:\n" \
L"/? - This help screen.\n" \
L"/ct - Clear Type anti-aliasing.\n" \
L"/fs | /max - (Full screen) | (Maximized) mode.\n" \
L"/multi | /nomulti - enable or disable multiconsole features\n" \
L"/font <fontname> - Specify the font name.\n" \
L"/size <fontsize> - Specify the font size.\n" \
L"/fontfile <fontfilename> - Loads fonts from file.\n" \
L"/BufferHeight <lines> - may be used with cmd.exe\n" \
/* L"/Attach [PID] - intercept console of specified process\n" */ \
L"/cmd <commandline> - Command line to start. This must be the last used switch.\n" \
L"\n" \
L"Command line examples:\n" \
L"ConEmu.exe /ct /font \"Lucida Console\" /size 16 /cmd far.exe \"c:\\1 2\\\"\n" \
L"\n" \
L"\x00A9 2006-2008, Zoin (based on console emulator by SEt)\n" \
L"\x00A9 2009, Maximus5\n" \
L"NightRoman: drawing process optimization, BufferHeight and other fixes\n" \
L"dolzenko_: windows switching via GUI tabs\n" \
L"alex_itd: Drag'n'Drop, RightClick, AltEnter and GUI bars\n" \
L"Mors: loading font from file."


#include "VirtualConsole.h"
#include "options.h"
#include "DragDrop.h"
#include "progressbars.h"
#include "TrayIcon.h"
#include "ConEmuChild.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "tabbar.h"
#include "TrayIcon.h"
#include "ConEmuPipe.h"
#include "UnicodeChars.h"
