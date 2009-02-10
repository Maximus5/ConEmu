#pragma once

#include <windows.h>
#include <Shlwapi.h>

#ifdef KL_MEM
#include "c:\\lang\\kl.h"
#else
#include "kl_parts.h"
#endif

#include "globals.h"
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


#define MBox(rt) (int)MessageBox(NULL, rt, Title, MB_SYSTEMMODAL | MB_ICONINFORMATION)
#define MBoxA(rt) (int)MessageBox(NULL, rt, _T("ConEmu"), MB_SYSTEMMODAL | MB_ICONINFORMATION)
#define isMeForeground() (GetForegroundWindow() == ghWnd || GetForegroundWindow() == ghOpWnd)
#define isPressed(inp) HIBYTE(GetKeyState(inp))

#define PTDIFFTEST(C,D) (((abs(C.x-LOWORD(lParam)))<D) && ((abs(C.y-HIWORD(lParam)))<D))

#define INVALIDATE() InvalidateRect(HDCWND, NULL, FALSE)


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



#include "resource.h"

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

void __forceinline DisplayLastError()
{
	TCHAR out[200];
	DWORD dw = GetLastError();
	LPVOID lpMsgBuf;
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
	wsprintf(out, _T("Error code ''%d'':\n%s"), dw, lpMsgBuf);
	MessageBox(0, out, _T("Error occurred"), MB_SYSTEMMODAL | MB_ICONERROR);
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
		RegSetValueEx(regMy, regKey, NULL, REG_BINARY, (LPBYTE)(&value), sizeof(T)); 
	}
	void Save(const TCHAR *regKey, const TCHAR *value)
	{
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
Maximus5: PictureView support, some bugfixes and customizations.\n\
(based on console emulator by SEt)";

