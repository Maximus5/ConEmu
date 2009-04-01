#pragma once

#include <windows.h>
#include <Shlwapi.h>
#include <vector>

#ifdef KL_MEM
#include "c:\\lang\\kl.h"
#else
#include "kl_parts.h"
#endif

// Array sizes
#define MAX_CONSOLE_COUNT 12


#include "globals.h"
#include "resource.h"
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


#define DRAG_L_ALLOWED 0x01
#define DRAG_R_ALLOWED 0x02
#define DRAG_L_STARTED 0x10
#define DRAG_R_STARTED 0x20
#define MOUSE_SIZING   0x40
#define MOUSE_R_LOCKED 0x80



#ifndef CLEARTYPE_NATURAL_QUALITY
#define CLEARTYPE_QUALITY       5
#define CLEARTYPE_NATURAL_QUALITY       6
#endif

#ifndef EVENT_CONSOLE_START_APPLICATION
#define EVENT_CONSOLE_START_APPLICATION 0x4006
#define EVENT_CONSOLE_END_APPLICATION   0x4007
#if defined(_WIN64)
#define CONSOLE_APPLICATION_16BIT       0x0000
#else
#define CONSOLE_APPLICATION_16BIT       0x0001
#endif
#endif


#define MBox(rt) (int)MessageBox(gbMessagingStarted ? ghWnd : NULL, rt, Title, /*MB_SYSTEMMODAL |*/ MB_ICONINFORMATION)
#define MBoxA(rt) (int)MessageBox(gbMessagingStarted ? ghWnd : NULL, rt, _T("ConEmu"), /*MB_SYSTEMMODAL |*/ MB_ICONINFORMATION)
#define MBoxAssert(V) if ((V)==FALSE) { TCHAR szAMsg[MAX_PATH*2]; wsprintf(szAMsg, _T("Assertion (%s) at\n%s:%i"), _T(#V), _T(__FILE__), __LINE__); MBoxA(szAMsg); }
#define isMeForeground() (GetForegroundWindow() == ghWnd || GetForegroundWindow() == ghOpWnd)
#define isPressed(inp) HIBYTE(GetKeyState(inp))

#define PTDIFFTEST(C,D) (((abs(C.x-LOWORD(lParam)))<D) && ((abs(C.y-HIWORD(lParam)))<D))

#define INVALIDATE() InvalidateRect(HDCWND, NULL, FALSE)

#define SafeCloseHandle(h) { if ((h)!=NULL) { HANDLE hh = (h); (h) = NULL; if (hh!=INVALID_HANDLE_VALUE) CloseHandle(hh); } }

#ifdef MSGLOGGER
#define POSTMESSAGE(h,m,w,l,e) {MCHKHEAP; DebugLogMessage(h,m,w,l,TRUE,e); PostMessage(h,m,w,l);}
#define SENDMESSAGE(h,m,w,l) {MCHKHEAP;  DebugLogMessage(h,m,w,l,FALSE,FALSE); SendMessage(h,m,w,l);}
#define SETWINDOWPOS(hw,hp,x,y,w,h,f) {MCHKHEAP; DebugLogPos(hw,x,y,w,h); SetWindowPos(hw,hp,x,y,w,h,f);}
#define MOVEWINDOW(hw,x,y,w,h,r) {MCHKHEAP; DebugLogPos(hw,x,y,w,h); MoveWindow(hw,x,y,w,h,r);}
#define SETCONSOLESCREENBUFFERSIZE(h,s) {MCHKHEAP; DebugLogBufSize(h,s); SetConsoleScreenBufferSize(h,s);}
#define DEBUGLOGFILE(m) DebugLogFile(m)
#else
#define POSTMESSAGE(h,m,w,l,e) PostMessage(h,m,w,l)
#define SENDMESSAGE(h,m,w,l) SendMessage(h,m,w,l)
#define SETWINDOWPOS(hw,hp,x,y,w,h,f) SetWindowPos(hw,hp,x,y,w,h,f)
#define MOVEWINDOW(hw,x,y,w,h,r) MoveWindow(hw,x,y,w,h,r)
#define SETCONSOLESCREENBUFFERSIZE(h,s) SetConsoleScreenBufferSize(h,s)
#define DEBUGLOGFILE(m)
#endif


#if !defined(__GNUC__)
#pragma warning (disable : 4005)
#define _WIN32_WINNT 0x0502
#endif




//------------------------------------------------------------------------
///| Code optimizing |////////////////////////////////////////////////////
//------------------------------------------------------------------------

#if !defined(__GNUC__)
#include <intrin.h>
#pragma function(memset, memcpy)
__forceinline void *memset(void *_Dst, int _Val, size_t _Size)
{
	__stosb((unsigned char*)_Dst, _Val, _Size);
	return _Dst;
}
__forceinline void *memcpy(void *_Dst, const void *_Src, size_t _Size)
{
	__movsb((unsigned char*)_Dst, (unsigned const char*)_Src, _Size);
	return _Dst;
}
#endif

void __forceinline DisplayLastError(LPCTSTR asLabel, DWORD dwError=0)
{
	DWORD dw = dwError ? dwError : GetLastError();
	LPVOID lpMsgBuf = NULL;
	MCHKHEAP
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
	int nLen = _tcslen(asLabel)+64+(lpMsgBuf ? _tcslen((TCHAR*)lpMsgBuf) : 0);
	TCHAR *out = new TCHAR[nLen];
	wsprintf(out, _T("%s\nLastError=0x%08X\n%s"), asLabel, dw, lpMsgBuf);
	if (gbMessagingStarted) SetForegroundWindow(ghWnd);
	MessageBox(gbMessagingStarted ? ghWnd : NULL, out, gConEmu.GetTitle(), MB_SYSTEMMODAL | MB_ICONERROR);
	MCHKHEAP
	LocalFree(lpMsgBuf);
	delete out;
	MCHKHEAP
}

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

#pragma warning(disable: 4311) // 'type cast' : pointer truncation from 'HBRUSH' to 'BOOL'

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

	template <class T> bool Load(const TCHAR *regKey, T *value)
	{
		DWORD len = sizeof(T);
		if (RegQueryValueEx(regMy, regKey, NULL, NULL, (LPBYTE)(value), &len) == ERROR_SUCCESS)
			return true;
		return false;
	}
	bool Load(const TCHAR *regKey, TCHAR *value)
	{
		DWORD len = MAX_PATH * sizeof(TCHAR);
		if (RegQueryValueEx(regMy, regKey, NULL, NULL, (LPBYTE)(value), &len) == ERROR_SUCCESS)
			return true;
		return false;
	}
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


const wchar_t pHelp[] = L"\
Console emulation program.\n\
By default this program launches \"Far.exe\" from the same directory it is in.\n\
\n\
Command line switches:\n\
/? - This help screen.\n\
/ct - Clear Type anti-aliasing.\n\
/fs - Full screen mode.\n\
/font <fontname> - Specify the font name.\n\
/size <fontsize> - Specify the font size.\n\
/fontfile <fontfilename> - Loads fonts from file.\n\
/SetParent - force change parent of con.window\n\
/DontSetParent - disable of changing parent (aka /Windows7)\n\
/BufferHeight <lines> - may be used with cmd.exe\n\
/Attach [PID] - intercept console of specified process\n\
/cmd <commandline> - Command line to start. This must be the last used switch.\n\
\n\
Command line examples:\n\
ConEmu.exe /ct /font \"Lucida Console\" /size 16 /cmd far.exe \"c:\\1 2\\\"\n\
\n\
(c) 2006-2008, Zoin.\n\
NightRoman: drawing process optimization, BufferHeight and other fixes.\n\
dolzenko_: windows switching via GUI tabs\n\
DrKnS: Update plugin support\n\
alex_itd: Drag'n'Drop, RightClick, AltEnter and GUI bars.\n\
Mors: loading font from file\n\
Maximus5: PictureView support, bugfixes and customizations.\n\
(based on console emulator by SEt)";
