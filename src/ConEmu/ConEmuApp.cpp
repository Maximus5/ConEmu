
/*
Copyright (c) 2009-2016 Maximus5
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"
#include <commctrl.h>
#pragma warning(disable: 4091)
#include <shlobj.h>
#include <exdisp.h>
#pragma warning(default: 4091)
#include <tlhelp32.h>
#if !defined(__GNUC__) || defined(__MINGW32__)
#pragma warning(disable: 4091)
#include <dbghelp.h>
#pragma warning(default: 4091)
#include <shobjidl.h>
#include <propkey.h>
#include <taskschd.h>
#else
#include "../common/DbgHlpGcc.h"
#endif
#include "../common/ConEmuCheck.h"
#include "../common/MBSTR.h"
#include "../common/MSetter.h"
#include "../common/MStrEsc.h"
#include "../common/WFiles.h"
#include "AboutDlg.h"
#include "Options.h"
#include "OptionsClass.h"
#include "ConEmu.h"
#include "ConfirmDlg.h"
#include "DpiAware.h"
#include "HooksUnlocker.h"
#include "Inside.h"
#include "LngRc.h"
#include "TaskBar.h"
#include "DwmHelper.h"
#include "ConEmuApp.h"
#include "Update.h"
#include "SetCmdTask.h"
#include "Recreate.h"
#include "DefaultTerm.h"
#include "IconList.h"
#include "UnitTests.h"
#include "MyClipboard.h"
#include "version.h"

#include "../common/StartupEnvEx.h"

#include "../common/Monitors.h"

#ifdef _DEBUG
//	#define SHOW_STARTED_MSGBOX
//	#define WAIT_STARTED_DEBUGGER
//	#define DEBUG_MSG_HOOKS
#endif

#ifdef __CYGWIN__
//	#define SHOW_STARTED_MSGBOX
#endif

#define DEBUGSTRMOVE(s) //DEBUGSTR(s)
#define DEBUGSTRTIMER(s) //DEBUGSTR(s)
#define DEBUGSTRSETHOTKEY(s) //DEBUGSTR(s)
#define DEBUGSTRSHUTSTEP(s) DEBUGSTR(s)
#define DEBUGSTRSTARTUP(s) DEBUGSTR(WIN3264TEST(L"ConEmu.exe: ",L"ConEmu64.exe: ") s L"\n")
#define DEBUGSTRSTARTUPLOG(s) {if (gpConEmu) gpConEmu->LogString(s);} DEBUGSTRSTARTUP(s)

WARNING("Заменить все MBoxAssert, _ASSERT, _ASSERTE на WaitForSingleObject(CreateThread(out,Title,dwMsgFlags),INFINITE);");


#ifdef MSGLOGGER
BOOL bBlockDebugLog=false, bSendToDebugger=true, bSendToFile=false;
WCHAR *LogFilePath=NULL;
#endif
#ifndef _DEBUG
BOOL gbNoDblBuffer = false;
#else
BOOL gbNoDblBuffer = false;
#endif

bool gbMessagingStarted = false;


#if defined(__CYGWIN__)
const CLSID CLSID_ShellWindows = {0x9BA05972, 0xF6A8, 0x11CF, {0xA4, 0x42, 0x00, 0xA0, 0xC9, 0x0A, 0x8F, 0x39}};
const IID IID_IShellWindows = {0x85CB6900, 0x4D95, 0x11CF, {0x96, 0x0C, 0x00, 0x80, 0xC7, 0xF4, 0xEE, 0x85}};
#endif


//externs
HINSTANCE g_hInstance=NULL;
HWND ghWnd=NULL, ghWndWork=NULL, ghWndApp=NULL, ghWndDrag=NULL;
#ifdef _DEBUG
HWND gh__Wnd = NULL; // Informational, to be sure what handle had our window before been destroyd
#endif
// Если для ярлыка назначен shortcut - может случиться, что в главное окно он не дойдет
WPARAM gnWndSetHotkey = 0, gnWndSetHotkeyOk = 0;
#ifdef _DEBUG
HWND ghConWnd=NULL;
#endif
CConEmuMain *gpConEmu = NULL;
//CVirtualConsole *pVCon=NULL;
Settings  *gpSet = NULL;
CSettings *gpSetCls = NULL;
//TCHAR temp[MAX_PATH]; -- низзя, очень велик шанс нарваться при многопоточности
HICON hClassIcon = NULL, hClassIconSm = NULL;
BOOL gbDebugLogStarted = FALSE;
BOOL gbDebugShowRects = FALSE;
CEStartupEnv* gpStartEnv = NULL;

LONG DontEnable::gnDontEnable = 0;
LONG DontEnable::gnDontEnableCount = 0;
//LONG nPrev;   // Informational!
//bool bLocked; // Proceed only main thread
DontEnable::DontEnable(bool abLock /*= true*/)
{
	bLocked = abLock && isMainThread();
	if (bLocked)
	{
		_ASSERTE(gnDontEnable>=0);
		nPrev = InterlockedIncrement(&gnDontEnable) - 1;
	}
	InterlockedIncrement(&gnDontEnableCount);
};
DontEnable::~DontEnable()
{
	if (bLocked)
	{
		InterlockedDecrement(&gnDontEnable);
	}
	InterlockedDecrement(&gnDontEnableCount);
	_ASSERTE(gnDontEnable>=0);
};
bool DontEnable::isDontEnable()
{
	return (gnDontEnable > 0);
};


const TCHAR *const gsClassName = VirtualConsoleClass; // окна отрисовки
const TCHAR *const gsClassNameParent = VirtualConsoleClassMain; // главное окно
const TCHAR *const gsClassNameWork = VirtualConsoleClassWork; // Holder для всех VCon
const TCHAR *const gsClassNameBack = VirtualConsoleClassBack; // Подложка (со скроллерами) для каждого VCon
const TCHAR *const gsClassNameApp = VirtualConsoleClassApp;


MMap<HWND,CVirtualConsole*> gVConDcMap;
MMap<HWND,CVirtualConsole*> gVConBkMap;


OSVERSIONINFO gOSVer = {};
WORD gnOsVer = 0x500;
bool gbIsWine = false;
bool gbIsDBCS = false;
// Drawing console font face name (default)
wchar_t gsDefGuiFont[32] = L"Lucida Console"; // gbIsWine ? L"Liberation Mono" : L"Lucida Console"
wchar_t gsAltGuiFont[32] = L"Courier New"; // "Lucida Console" is not installed?
// Set this font (default) in real console window to enable unicode support
wchar_t gsDefConFont[32] = L"Lucida Console"; // DBCS ? L"Liberation Mono" : L"Lucida Console"
wchar_t gsAltConFont[32] = L"Courier New"; // "Lucida Console" is not installed?
// Use this (default) in ConEmu interface, where allowed (tabs, status, panel views, ...)
wchar_t gsDefMUIFont[32] = L"Tahoma";         // WindowsVista ? L"Segoe UI" : L"Tahoma"

LPCWSTR GetMouseMsgName(UINT msg)
{
	LPCWSTR pszName;
	switch (msg)
	{
	case WM_MOUSEMOVE: pszName = L"WM_MOUSEMOVE"; break;
	case WM_LBUTTONDOWN: pszName = L"WM_LBUTTONDOWN"; break;
	case WM_LBUTTONUP: pszName = L"WM_LBUTTONUP"; break;
	case WM_LBUTTONDBLCLK: pszName = L"WM_LBUTTONDBLCLK"; break;
	case WM_RBUTTONDOWN: pszName = L"WM_RBUTTONDOWN"; break;
	case WM_RBUTTONUP: pszName = L"WM_RBUTTONUP"; break;
	case WM_RBUTTONDBLCLK: pszName = L"WM_RBUTTONDBLCLK"; break;
	case WM_MBUTTONDOWN: pszName = L"WM_MBUTTONDOWN"; break;
	case WM_MBUTTONUP: pszName = L"WM_MBUTTONUP"; break;
	case WM_MBUTTONDBLCLK: pszName = L"WM_MBUTTONDBLCLK"; break;
	case 0x020A: pszName = L"WM_MOUSEWHEEL"; break;
	case 0x020B: pszName = L"WM_XBUTTONDOWN"; break;
	case 0x020C: pszName = L"WM_XBUTTONUP"; break;
	case 0x020D: pszName = L"WM_XBUTTONDBLCLK"; break;
	case 0x020E: pszName = L"WM_MOUSEHWHEEL"; break;
	default:
		{
			static wchar_t szTmp[32] = L"";
			_wsprintf(szTmp, SKIPCOUNT(szTmp) L"0x%X(%u)", msg, msg);
			pszName = szTmp;
		}
	}
	return pszName;
}

LONG gnMessageNestingLevel = 0;

#ifdef MSGLOGGER
void DebugLogMessage(HWND h, UINT m, WPARAM w, LPARAM l, int posted, BOOL extra)
{
	if (bBlockDebugLog || (!bSendToFile && !IsDebuggerPresent()))
		return;

	static char szMess[32], szWP[32], szLP[48], szWhole[255];
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

			MSG2(WM_SIZE);
			{
				wsprintfA(szLP, "{%ix%i}", GET_X_LPARAM(l), GET_Y_LPARAM(l));
				break;
			}
			MSG2(WM_MOVE);
			{
				wsprintfA(szLP, "{%i,%i}", GET_X_LPARAM(l), GET_Y_LPARAM(l));
				break;
			}
			MSG1(WM_GETMINMAXINFO);
			MSG2(WM_NCCALCSIZE);
			{
				if (l)
				{
					RECT r = w ? ((LPNCCALCSIZE_PARAMS)l)->rgrc[0] : *(LPRECT)l;
					wsprintfA(szLP, "{%i,%i} {%ix%i}", r.left, r.top, LOGRECTSIZE(r));
				}
				break;
			}

			MSG2(WM_WINDOWPOSCHANGING);
			{
				wsprintfA(szLP, "{%i,%i} {%ix%i}", ((LPWINDOWPOS)l)->x, ((LPWINDOWPOS)l)->y, ((LPWINDOWPOS)l)->cx, ((LPWINDOWPOS)l)->cy);
				break;
			}
			MSG2(WM_WINDOWPOSCHANGED);
			{
				wsprintfA(szLP, "{%i,%i} {%ix%i}", ((LPWINDOWPOS)l)->x, ((LPWINDOWPOS)l)->y, ((LPWINDOWPOS)l)->cx, ((LPWINDOWPOS)l)->cy);
				break;
			}

			MSG1(WM_KEYDOWN);
			MSG1(WM_KEYUP);
			MSG1(WM_SYSKEYDOWN);
			MSG1(WM_SYSKEYUP);
			MSG2(WM_MOUSEWHEEL);
			wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
			break;
			MSG2(WM_ACTIVATE);

			switch(w)
			{
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
				switch (w)
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
						WP1(ID_ABOUT);
						WP1(ID_DONATE_LINK);
						WP1(ID_TOTRAY);
						WP1(ID_TOMONITOR);
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

	if (bSendToDebugger || bSendToFile)
	{
		if (!szMess[0]) wsprintfA(szMess, "%i(x%02X)", m, m);

		if (!szWP[0]) wsprintfA(szWP, "%i", (DWORD)w);

		if (!szLP[0]) wsprintfA(szLP, "%i(0x%08X)", (int)l, (DWORD)l);

		LPCSTR pszSrc = (posted < 0) ? "Recv" : (posted ? "Post" : "Send");

		if (bSendToDebugger)
		{
			static SYSTEMTIME st;
			GetLocalTime(&st);
			wsprintfA(szWhole, "%02i:%02i.%03i %s%s: <%i> %s, %s, %s\n", st.wMinute, st.wSecond, st.wMilliseconds,
				pszSrc, (extra ? "+" : ""), gnMessageNestingLevel, szMess, szWP, szLP);
			OutputDebugStringA(szWhole);
		}
		else if (bSendToFile)
		{
			wsprintfA(szWhole, "%s%s: <%i> %s, %s, %s\n",
				pszSrc, (extra ? "+" : ""), gnMessageNestingLevel, szMess, szWP, szLP);
			DebugLogFile(szWhole);
		}
	}
}
void DebugLogPos(HWND hw, int x, int y, int w, int h, LPCSTR asFunc)
{
#ifdef MSGLOGGER

	if (!x && !y && hw == ghConWnd)
	{
		if (!IsDebuggerPresent() && !isPressed(VK_LBUTTON))
			x = x;
	}

#endif

	if (bBlockDebugLog || (!bSendToFile && !IsDebuggerPresent()))
		return;

	if (bSendToDebugger)
	{
		wchar_t szPos[255];
		static SYSTEMTIME st;
		GetLocalTime(&st);
		_wsprintf(szPos, SKIPLEN(countof(szPos)) L"%02i:%02i.%03i %s(%s, %i,%i,%i,%i)\n",
		          st.wMinute, st.wSecond, st.wMilliseconds, asFunc,
		          (hw==ghConWnd) ? L"Con" : L"Emu", x,y,w,h);
		DEBUGSTRMOVE(szPos);
	}
	else if (bSendToFile)
	{
		char szPos[255];
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

	if (bSendToDebugger)
	{
		static SYSTEMTIME st;
		GetLocalTime(&st);
		wsprintfA(szSize, "%02i:%02i.%03i ChangeBufferSize(%i,%i)\n",
		          st.wMinute, st.wSecond, st.wMilliseconds,
		          sz.X, sz.Y);
		OutputDebugStringA(szSize);
	}
	else if (bSendToFile)
	{
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

	if (LogFilePath==NULL)
	{
		//WCHAR LogFilePath[MAX_PATH+1];
		LogFilePath = (WCHAR*)calloc(MAX_PATH+1,sizeof(WCHAR));
		GetModuleFileNameW(0,LogFilePath,MAX_PATH);
		WCHAR* pszDot = wcsrchr(LogFilePath, L'.');
		lstrcpyW(pszDot, L".log");
		hLogFile = CreateFileW(LogFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	else
	{
		hLogFile = CreateFileW(LogFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}

	if (hLogFile!=INVALID_HANDLE_VALUE)
	{
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
BOOL POSTMESSAGE(HWND h,UINT m,WPARAM w,LPARAM l,BOOL extra)
{
	MCHKHEAP;
	DebugLogMessage(h,m,w,l,1,extra);
	return PostMessage(h,m,w,l);
}
LRESULT SENDMESSAGE(HWND h,UINT m,WPARAM w,LPARAM l)
{
	MCHKHEAP;
	DebugLogMessage(h,m,w,l,0,FALSE);
	return SendMessage(h,m,w,l);
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


void LogString(LPCWSTR asInfo, bool abWriteTime /*= true*/, bool abWriteLine /*= true*/)
{
	gpConEmu->LogString(asInfo, abWriteTime, abWriteLine);
}

LPCWSTR GetWindowModeName(ConEmuWindowMode wm)
{
	static wchar_t swmCurrent[] = L"wmCurrent";
	static wchar_t swmNotChanging[] = L"wmNotChanging";
	static wchar_t swmNormal[] = L"wmNormal";
	static wchar_t swmMaximized[] = L"wmMaximized";
	static wchar_t swmFullScreen[] = L"wmFullScreen";
	switch (wm)
	{
	case wmCurrent:
		return swmCurrent;
	case wmNotChanging:
		return swmNotChanging;
	case wmNormal:
		return swmNormal;
	case wmMaximized:
		return swmMaximized;
	case wmFullScreen:
		return swmFullScreen;
	}
	return L"INVALID";
}

void ShutdownGuiStep(LPCWSTR asInfo, int nParm1 /*= 0*/, int nParm2 /*= 0*/, int nParm3 /*= 0*/, int nParm4 /*= 0*/)
{
	wchar_t szFull[512];
	msprintf(szFull, countof(szFull), L"%u:ConEmuG:PID=%u:TID=%u: ",
		GetTickCount(), GetCurrentProcessId(), GetCurrentThreadId());
	if (asInfo)
	{
		int nLen = lstrlen(szFull);
		msprintf(szFull+nLen, countof(szFull)-nLen, asInfo, nParm1, nParm2, nParm3, nParm4);
	}

	LogString(szFull);

#ifdef _DEBUG
	static int nDbg = 0;
	if (!nDbg)
		nDbg = IsDebuggerPresent() ? 1 : 2;
	if (nDbg != 1)
		return;
	DEBUGSTRSHUTSTEP(szFull);
#endif
}

/* Используются как extern в ConEmuCheck.cpp */
/*
LPVOID _calloc(size_t nCount,size_t nSize) {
	return calloc(nCount,nSize);
}
LPVOID _malloc(size_t nCount) {
	return malloc(nCount);
}
void   _free(LPVOID ptr) {
	free(ptr);
}
*/


bool IntFromString(int& rnValue, LPCWSTR asValue, int anBase /*= 10*/, LPCWSTR* rsEnd /*= NULL*/)
{
	bool bOk = false;
	wchar_t* pszEnd = NULL;

	if (!asValue || !*asValue)
	{
		rnValue = 0;
	}
	else
	{
		// Skip hex prefix if exists
		if (anBase == 16)
		{
			if (asValue[0] == L'x' || asValue[0] == L'X')
				asValue += 1;
			else if (asValue[0] == L'0' && (asValue[1] == L'x' || asValue[1] == L'X'))
				asValue += 2;
		}

		rnValue = wcstol(asValue, &pszEnd, anBase);
		bOk = (pszEnd && (pszEnd != asValue));
	}

	if (rsEnd) *rsEnd = pszEnd;
	return true;
}

bool GetDlgItemSigned(HWND hDlg, WORD nID, int& nValue, int nMin /*= 0*/, int nMax /*= 0*/)
{
	BOOL lbOk = FALSE;
	int n = (int)GetDlgItemInt(hDlg, nID, &lbOk, TRUE);
	if (!lbOk)
		return false;
	if (nMin || nMax)
	{
		if (nValue < nMin)
			return false;
		if (nMax && nValue > nMax)
			return false;
	}
	nValue = n;
	return true;
}

bool GetDlgItemUnsigned(HWND hDlg, WORD nID, DWORD& nValue, DWORD nMin /*= 0*/, DWORD nMax /*= 0*/)
{
	BOOL lbOk = FALSE;
	DWORD n = GetDlgItemInt(hDlg, nID, &lbOk, FALSE);
	if (!lbOk)
		return false;
	if (nMin || nMax)
	{
		if (nValue < nMin)
			return false;
		if (nMax && nValue > nMax)
			return false;
	}
	nValue = n;
	return true;
}

wchar_t* GetDlgItemTextPtr(HWND hDlg, WORD nID)
{
	wchar_t* pszText = NULL;
	size_t cchMax = 0;
	MyGetDlgItemText(hDlg, nID, cchMax, pszText);
	return pszText;
}

template <size_t size>
int MyGetDlgItemText(HWND hDlg, WORD nID, wchar_t (&rszText)[size])
{
	CEStr szText;
	size_t cchMax = 0;
	int nLen = MyGetDlgItemText(hDlg, nID, cchMax, szText.ms_Val);
	if (lstrcmp(rszText, szText.ms_Val) == 0)
		return false;
	lstrcpyn(rszText, szText.ms_Val, size);
	return true;
}

size_t MyGetDlgItemText(HWND hDlg, WORD nID, size_t& cchMax, wchar_t*& pszText/*, bool bEscapes*/ /*= false*/)
{
	HWND hEdit;

	if (nID)
		hEdit = GetDlgItem(hDlg, nID);
	else
		hEdit = hDlg;

	if (!hEdit)
		return 0;

	//
	int nLen = GetWindowTextLength(hEdit);

	if (nLen > 0)
	{
		if (!pszText || (((UINT)nLen) >= cchMax))
		{
			SafeFree(pszText);
			cchMax = nLen+32;
			pszText = (wchar_t*)calloc(cchMax,sizeof(*pszText));
			_ASSERTE(pszText);
		}


		if (pszText)
		{
			pszText[0] = 0;
			GetWindowText(hEdit, pszText, nLen+1);
		}
	}
	else
	{
		_ASSERTE(nLen == 0);
		nLen = 0;

		if (pszText)
			*pszText = 0;
	}

	return nLen;
}

// TODO: Optimize: Now pszDst must be (4x len in maximum for "\xFF" form) for bSet==true
void EscapeChar(bool bSet, LPCWSTR& pszSrc, LPWSTR& pszDst)
{
	if (bSet)
	{
		// Set escapes: wchar(13) --> "\\r"
		EscapeChar(pszSrc, pszDst);
	}
	else
	{
		// Remove escapes: "\\r" --> wchar(13), etc.
		UnescapeChar(pszSrc, pszDst);
	}
}

#if 0
wchar_t* EscapeString(bool bSet, LPCWSTR pszSrc)
{
	wchar_t* pszBuf = NULL;

	if (!pszSrc || !*pszSrc)
	{
		return lstrdup(L"");
	}

	if (bSet)
	{
		// Set escapes: wchar(13) --> "\\r"
		pszBuf = (wchar_t*)malloc((_tcslen(pszSrc)*2+1)*sizeof(*pszBuf));
		if (!pszBuf)
		{
			MBoxAssert(pszBuf && "Memory allocation failed");
			return NULL;
		}

		// This func is used mostly for print("...") GuiMacro, So, we don't need to set escapes on quotas

		wchar_t* pszDst = pszBuf;

		LPCWSTR  pszCtrl = L"rntae\\\"";

		while (*pszSrc)
		{
			EscapeChar(bSet, pszSrc, pszDst);
		}
		*pszDst = 0;
	}
	else
	{
		// Remove escapes: "\\r" --> wchar(13)
		pszBuf = lstrdup(pszSrc);
		if (!pszBuf)
		{
			MBoxAssert(pszBuf && "Memory allocation failed");
			return NULL;
		}

		wchar_t* pszEsc = wcschr(pszBuf, L'\\');
		if (pszEsc)
		{
			wchar_t* pszDst = pszEsc;
			pszSrc = pszEsc;
			while (*pszSrc)
			{
				EscapeChar(bSet, pszSrc, pszDst);
			}
			*pszDst = 0;
		}
	}

	return pszBuf;
}
#endif

BOOL MySetDlgItemText(HWND hDlg, int nIDDlgItem, LPCTSTR lpString/*, bool bEscapes*/ /*= false*/)
{
	wchar_t* pszBuf = NULL;

	//// -- Must be the same set in MyGetDlgItemText
	//if (lpString && bEscapes /*&& wcspbrk(lpString, L"\r\n\t\\")*/)
	//{
	//	pszBuf = EscapeString(true, lpString);
	//	if (!pszBuf)
	//		return FALSE; // Уже ругнулись
	//	lpString = pszBuf;
	//}

	BOOL lbRc = SetDlgItemText(hDlg, nIDDlgItem, lpString);

	SafeFree(pszBuf);
	return lbRc;
}

bool GetColorRef(LPCWSTR pszText, COLORREF* pCR)
{
	if (!pszText || !*pszText)
		return false;

	bool result = false;
	int r = 0, g = 0, b = 0;
	const wchar_t *pch;
	wchar_t *pchEnd = NULL;
	COLORREF clr = 0;
	bool bHex = false;

	if ((pszText[0] == L'#') // #RRGGBB
		|| (pszText[0] == L'x' || pszText[0] == L'X') // xBBGGRR (COLORREF)
		|| (pszText[0] == L'0' && (pszText[1] == L'x' || pszText[1] == L'X'))) // 0xBBGGRR (COLORREF)
	{
		// Считаем значение 16-ричным rgb кодом
		pch = (pszText[0] == L'0') ? (pszText+2) : (pszText+1);
		clr = wcstoul(pch, &pchEnd, 16);
		bHex = true;
	}
	else if ((pszText[0] == L'0') && (pszText[1] == L'0')) // 00BBGGRR (COLORREF, copy from *.reg)
	{
		// Это может быть 8 цифр (тоже hex) скопированных из reg-файла
		pch = (pszText + 2);
		clr = wcstoul(pch, &pchEnd, 16);
		bHex = (pchEnd && ((pchEnd - pch) == 6));
	}

	if (bHex)
	{
		// Считаем значение 16-ричным rgb кодом
		if (clr && (pszText[0] == L'#'))
		{
			// "#rrggbb", обменять местами rr и gg, нам нужен COLORREF (bbggrr)
			clr = ((clr & 0xFF)<<16) | ((clr & 0xFF00)) | ((clr & 0xFF0000)>>16);
		}
		// Done
		if (pchEnd && (pchEnd > (pszText+1)) && (clr <= 0xFFFFFF) && (*pCR != clr))
		{
			*pCR = clr;
			result = true;
		}
	}
	else
	{
		pch = (wchar_t*)wcspbrk(pszText, L"0123456789");
		pchEnd = NULL;
		r = pch ? wcstol(pch, &pchEnd, 10) : 0;
		if (pchEnd && (pchEnd > pch))
		{
			pch = (wchar_t*)wcspbrk(pchEnd, L"0123456789");
			pchEnd = NULL;
			g = pch ? wcstol(pch, &pchEnd, 10) : 0;

			if (pchEnd && (pchEnd > pch))
			{
				pch = (wchar_t*)wcspbrk(pchEnd, L"0123456789");
				pchEnd = NULL;
				b = pch ? wcstol(pch, &pchEnd, 10) : 0;
			}

			// decimal format of UltraEdit?
			if ((r > 255) && !g && !b)
			{
				g = (r & 0xFF00) >> 8;
				b = (r & 0xFF0000) >> 16;
				r &= 0xFF;
			}

			// Достаточно ввода одной компоненты
			if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && *pCR != RGB(r, g, b))
			{
				*pCR = RGB(r, g, b);
				result = true;
			}
		}
	}

	return result;
}

wchar_t* DupCygwinPath(LPCWSTR asWinPath, bool bAutoQuote)
{
	if (!asWinPath)
	{
		_ASSERTE(asWinPath!=NULL);
		return NULL;
	}

	if (bAutoQuote)
	{
		if ((asWinPath[0] == L'"') || (wcschr(asWinPath, L' ') == NULL))
		{
			bAutoQuote = false;
		}
	}

	size_t cchLen = _tcslen(asWinPath) + (bAutoQuote ? 3 : 1) + 1/*Possible space-termination on paste*/;
	wchar_t* pszResult = (wchar_t*)malloc(cchLen*sizeof(*pszResult));
	if (!pszResult)
		return NULL;
	wchar_t* psz = pszResult;

	if (bAutoQuote)
	{
		*(psz++) = L'"';
	}
	else if (asWinPath[0] == L'"')
	{
		*(psz++) = *(asWinPath++);
	}

	if (asWinPath[0] && (asWinPath[1] == L':'))
	{
		*(psz++) = L'/';
		*(psz++) = asWinPath[0];
		asWinPath += 2;
	}
	else
	{
		// А bash понимает сетевые пути?
		_ASSERTE((psz[0] == L'\\' && psz[1] == L'\\') || (wcschr(psz, L'\\')==NULL));
	}

	while (*asWinPath)
	{
		if (*asWinPath == L'\\')
		{
			*(psz++) = L'/';
			asWinPath++;
		}
		else
		{
			*(psz++) = *(asWinPath++);
		}
	}

	if (bAutoQuote)
		*(psz++) = L'"';
	*psz = 0;

	return pszResult;
}

// Вернуть путь с обратными слешами, если диск указан в cygwin-формате - добавить двоеточие
// asAnyPath может быть полным или относительным путем, например
// C:\Src\file.c
// C:/Src/file.c
// /C/Src/file.c
// //server/share/file
// \\server\share/path/file
// /cygdrive/C/Src/file.c
// ..\folder/file.c
wchar_t* MakeWinPath(LPCWSTR asAnyPath)
{
	// Drop spare prefix, point to "/" after "/cygdrive"
	int iSkip = startswith(asAnyPath, L"/cygdrive/", true);
	LPCWSTR pszSrc = asAnyPath + ((iSkip > 0) ? (iSkip-1) : 0);

	// Prepare buffer
	int iLen = lstrlen(pszSrc);
	if (iLen < 1)
	{
		_ASSERTE(lstrlen(pszSrc) > 0);
		return NULL;
	}

	// Диск в cygwin формате?
	wchar_t cDrive = 0;
	if ((pszSrc[0] == L'/' || pszSrc[0] == L'\\')
		&& isDriveLetter(pszSrc[1])
		&& (pszSrc[2] == L'/' || pszSrc[2] == L'\\'))
	{
		cDrive = pszSrc[1];
		CharUpperBuff(&cDrive, 1);
		pszSrc += 2;
	}

	// Формируем буфер
	wchar_t* pszRc = (wchar_t*)malloc((iLen+1)*sizeof(wchar_t));
	if (!pszRc)
	{
		_ASSERTE(pszRc && "malloc failed");
		return NULL;
	}
	// Make path
	wchar_t* pszDst = pszRc;
	if (cDrive)
	{
		*(pszDst++) = cDrive;
		*(pszDst++) = L':';
		iLen -= 2;
	}
	_wcscpy_c(pszDst, iLen+1, pszSrc);
	// Convert slashes
	pszDst = wcschr(pszDst, L'/');
	while (pszDst)
	{
		*pszDst = L'\\';
		pszDst = wcschr(pszDst+1, L'/');
	}
	// Ready
	return pszRc;
}

wchar_t* MakeStraightSlashPath(LPCWSTR asWinPath)
{
	wchar_t* pszSlashed = lstrdup(asWinPath);
	wchar_t* p = wcschr(pszSlashed, L'\\');
	while (p)
	{
		*p = L'/';
		p = wcschr(p+1, L'\\');
	}
	return pszSlashed;
}

bool FixDirEndSlash(wchar_t* rsPath)
{
	int nLen = rsPath ? lstrlen(rsPath) : 0;
	// Do not cut slash from "C:\"
	if ((nLen > 3) && (rsPath[nLen-1] == L'\\'))
	{
		rsPath[nLen-1] = 0;
		return true;
	}
	else if ((nLen > 0) && (rsPath[nLen-1] == L':'))
	{
		// The root of drive must have end slash
		rsPath[nLen] = L'\\'; rsPath[nLen+1] = 0;
		return true;
	}
	return false;
}

wchar_t* SelectFolder(LPCWSTR asTitle, LPCWSTR asDefFolder /*= NULL*/, HWND hParent /*= ghWnd*/, DWORD/*CESelectFileFlags*/ nFlags /*= sff_AutoQuote*/)
{
	wchar_t* pszResult = NULL;

	BROWSEINFO bi = {hParent};
	wchar_t szFolder[MAX_PATH+1] = {0};
	if (asDefFolder)
		wcscpy_c(szFolder, asDefFolder);
	bi.pszDisplayName = szFolder;
	wchar_t szTitle[100];
	bi.lpszTitle = lstrcpyn(szTitle, asTitle ? asTitle : L"Choose folder", sizeof(szTitle));
	bi.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS | BIF_VALIDATE;
	bi.lpfn = CRecreateDlg::BrowseCallbackProc;
	bi.lParam = (LPARAM)szFolder;
	LPITEMIDLIST pRc = SHBrowseForFolder(&bi);

	if (pRc)
	{
		if (SHGetPathFromIDList(pRc, szFolder))
		{
			if (nFlags & sff_Cygwin)
			{
				pszResult = DupCygwinPath(szFolder, (nFlags & sff_AutoQuote));
			}
			else if ((nFlags & sff_AutoQuote) && (wcschr(szFolder, L' ') != NULL))
			{
				size_t cchLen = _tcslen(szFolder);
				pszResult = (wchar_t*)malloc((cchLen+3)*sizeof(*pszResult));
				if (pszResult)
				{
					pszResult[0] = L'"';
					_wcscpy_c(pszResult+1, cchLen+1, szFolder);
					pszResult[cchLen+1] = L'"';
					pszResult[cchLen+2] = 0;
				}
			}
			else
			{
				pszResult = lstrdup(szFolder);
			}
		}

		CoTaskMemFree(pRc);
	}

	return pszResult;
}

wchar_t* SelectFile(LPCWSTR asTitle, LPCWSTR asDefFile /*= NULL*/, LPCWSTR asDefPath /*= NULL*/, HWND hParent /*= ghWnd*/, LPCWSTR asFilter /*= NULL*/, DWORD/*CESelectFileFlags*/ nFlags /*= sff_AutoQuote*/)
{
	wchar_t* pszResult = NULL;

	wchar_t temp[MAX_PATH+10] = {};
	if (asDefFile)
		_wcscpy_c(temp+1, countof(temp)-2, asDefFile);

	OPENFILENAME ofn = {sizeof(ofn)};
	ofn.hwndOwner = hParent;
	ofn.lpstrFilter = asFilter ? asFilter : L"All files (*.*)\0*.*\0Text files (*.txt,*.ini,*.log)\0*.txt;*.ini;*.log\0Executables (*.exe,*.com,*.bat,*.cmd)\0*.exe;*.com;*.bat;*.cmd\0Scripts (*.vbs,*.vbe,*.js,*.jse)\0*.vbs;*.vbe;*.js;*.jse\0\0";
	//ofn.lpstrFilter = L"All files (*.*)\0*.*\0\0";
	ofn.lpstrFile = temp+1;
	ofn.lpstrInitialDir = asDefPath;
	ofn.nMaxFile = countof(temp)-10;
	ofn.lpstrTitle = asTitle ? asTitle : L"Choose file";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
		| OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|((nFlags & sff_SaveNewFile) ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
	// Append extension if user fails to type it
	if (asDefFile && (asDefFile[0] == L'*' && asDefFile[1] == L'.' && asDefFile[2]))
		ofn.lpstrDefExt = (asDefFile+2);

	BOOL bRc = (nFlags & sff_SaveNewFile)
		? GetSaveFileName(&ofn)
		: GetOpenFileName(&ofn);

	if (bRc)
	{
		LPCWSTR pszName = temp+1;

		if (nFlags & sff_Cygwin)
		{
			pszResult = DupCygwinPath(pszName, (nFlags & sff_AutoQuote));
		}
		else
		{
			if ((nFlags & sff_AutoQuote) && (wcschr(pszName, L' ') != NULL))
			{
				temp[0] = L'"';
				wcscat_c(temp, L"\"");
				pszName = temp;
			}
			else
			{
				temp[0] = L' ';
			}

			pszResult = lstrdup(pszName);
		}
	}

	return pszResult;
}

// pszWords - '|'separated
void StripWords(wchar_t* pszText, const wchar_t* pszWords)
{
	wchar_t dummy[MAX_PATH];
	LPCWSTR pszWord = pszWords;
	while (pszWord && *pszWord)
	{
		LPCWSTR pszNext = wcschr(pszWord, L'|');
		if (!pszNext) pszNext = pszWord + _tcslen(pszWord);

		int nLen = (int)(pszNext - pszWord);
		if (nLen > 0)
		{
			lstrcpyn(dummy, pszWord, min((int)countof(dummy),(nLen+1)));
			wchar_t* pszFound;
			while ((pszFound = StrStrI(pszText, dummy)) != NULL)
			{
				size_t nLeft = _tcslen(pszFound);
				size_t nCurLen = nLen;
				// Strip spaces after replaced token
				while (pszFound[nCurLen] == L' ')
					nCurLen++;
				if (nLeft <= nCurLen)
				{
					*pszFound = 0;
					break;
				}
				else
				{
					wmemmove(pszFound, pszFound+nCurLen, nLeft - nCurLen + 1);
				}
			}
		}

		if (!*pszNext)
			break;
		pszWord = pszNext + 1;
	}
}

void StripLines(wchar_t* pszText, LPCWSTR pszCommentMark)
{
	if (!pszText || !*pszText || !pszCommentMark || !*pszCommentMark)
		return;

	wchar_t* pszSrc = pszText;
	wchar_t* pszDst = pszText;
	INT_PTR iLeft = wcslen(pszText) + 1;
	INT_PTR iCmp = wcslen(pszCommentMark);

	while (iLeft > 1)
	{
		wchar_t* pszEOL = wcspbrk(pszSrc, L"\r\n");
		if (!pszEOL)
			pszEOL = pszSrc + iLeft;
		else if (pszEOL[0] == L'\r' && pszEOL[1] == L'\n')
			pszEOL += 2;
		else
			pszEOL ++;

		INT_PTR iLine = pszEOL - pszSrc;

		if (wcsncmp(pszSrc, pszCommentMark, iCmp) == 0)
		{
			// Drop this line
			if (iLeft <= iLine)
			{
				_ASSERTE(iLeft >= iLine);
				*pszDst = 0;
				break;
			}
			else
			{
				wmemmove(pszDst, pszEOL, iLeft - iLine);
				iLeft -= iLine;
			}
		}
		else
		{
			// Skip to next line
			iLeft -= iLine;
			pszSrc += iLine;
			pszDst += iLine;
		}
	}

	*pszDst = 0;
}

BOOL CreateProcessRestricted(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
							 LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
							 BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
							 LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
							 LPDWORD pdwLastError)
{
	BOOL lbRc = FALSE;
	HANDLE hToken = NULL, hTokenRest = NULL;

	if (OpenProcessToken(GetCurrentProcess(),
	                    TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY | TOKEN_EXECUTE,
	                    &hToken))
	{
		enum WellKnownAuthorities
		{
			NullAuthority = 0, WorldAuthority, LocalAuthority, CreatorAuthority,
			NonUniqueAuthority, NtAuthority, MandatoryLabelAuthority = 16
		};
		SID *pAdmSid = (SID*)calloc(sizeof(SID)+sizeof(DWORD)*2,1);
		pAdmSid->Revision = SID_REVISION;
		pAdmSid->SubAuthorityCount = 2;
		pAdmSid->IdentifierAuthority.Value[5] = NtAuthority;
		pAdmSid->SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;
		pAdmSid->SubAuthority[1] = DOMAIN_ALIAS_RID_ADMINS;
		SID_AND_ATTRIBUTES sidsToDisable[] =
		{
			{pAdmSid}
		};

		#ifdef __GNUC__
		HMODULE hAdvApi = GetModuleHandle(L"AdvApi32.dll");
		CreateRestrictedToken_t CreateRestrictedToken = (CreateRestrictedToken_t)GetProcAddress(hAdvApi, "CreateRestrictedToken");
		#endif

		if (CreateRestrictedToken(hToken, DISABLE_MAX_PRIVILEGE,
		                         countof(sidsToDisable), sidsToDisable,
		                         0, NULL, 0, NULL, &hTokenRest))
		{
			if (CreateProcessAsUserW(hTokenRest, lpApplicationName, lpCommandLine,
							 lpProcessAttributes, lpThreadAttributes,
							 bInheritHandles, dwCreationFlags, lpEnvironment,
							 lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
			{
				lbRc = TRUE;
			}
			else
			{
				if (pdwLastError) *pdwLastError = GetLastError();
			}

			CloseHandle(hTokenRest); hTokenRest = NULL;
		}
		else
		{
			if (pdwLastError) *pdwLastError = GetLastError();
		}

		free(pAdmSid);
		CloseHandle(hToken); hToken = NULL;
	}
	else
	{
		if (pdwLastError) *pdwLastError = GetLastError();
	}

	return lbRc;
}

#if !defined(__GNUC__)
static void DisplayShedulerError(LPCWSTR pszStep, HRESULT hr, LPCWSTR bsTaskName, LPCWSTR lpCommandLine)
{
	wchar_t szInfo[200] = L"";
	_wsprintf(szInfo, SKIPCOUNT(szInfo) L" Please check sheduler log.\n" L"HR=%u, TaskName=", (DWORD)hr);
	CEStr szErr(pszStep, szInfo, bsTaskName, L"\n", lpCommandLine);
	DisplayLastError(szErr, (DWORD)hr);
}
#endif

/// The function starts new process using Windows Task Sheduler
/// This allows to run process ‘Demoted’ (bAsSystem == false)
/// or under ‘System’ account (bAsSystem == true)
// TODO: Server started as bAsSystem is not working properly yet
BOOL CreateProcessSheduled(bool bAsSystem, LPWSTR lpCommandLine,
						   LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
						   BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
						   LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
						   LPDWORD pdwLastError)
{
	if (!lpCommandLine || !*lpCommandLine)
	{
		DisplayLastError(L"CreateProcessDemoted failed, lpCommandLine is empty", -1);
		return FALSE;
	}

	// This issue is not actual anymore, because we call put_ExecutionTimeLimit(‘Infinite’)
	// Issue 1897: Created task stopped after 72 hour, so use "/bypass"
	CEStr szExe;
	if (!GetModuleFileName(NULL, szExe.GetBuffer(MAX_PATH), MAX_PATH) || szExe.IsEmpty())
	{
		DisplayLastError(L"GetModuleFileName(NULL) failed");
		return FALSE;
	}
	CEStr szCommand(L"/bypass /cmd ", lpCommandLine);
	LPCWSTR pszCmdArgs = szCommand;

	BOOL lbRc = FALSE;

#if defined(__GNUC__)

	DisplayLastError(L"GNU <taskschd.h> does not have TaskSheduler support yet!", (DWORD)-1);

#else

	// Task не выносит окна созданных задач "наверх"
	HWND hPrevEmu = FindWindowEx(NULL, NULL, VirtualConsoleClassMain, NULL);
	HWND hCreated = NULL;


	// Really working method: Run cmd-line via task scheduler
	// But there is one caveat: Task scheduler may be disable on PC!

	wchar_t szUniqTaskName[128];
	wchar_t szVer4[8] = L""; lstrcpyn(szVer4, _T(MVV_4a), countof(szVer4));
	_wsprintf(szUniqTaskName, SKIPLEN(countof(szUniqTaskName)) L"ConEmu %02u%02u%02u%s starter ParentPID=%u", (MVV_1%100),MVV_2,MVV_3,szVer4, GetCurrentProcessId());

	BSTR bsTaskName = SysAllocString(szUniqTaskName);
	BSTR bsExecutablePath = SysAllocString(szExe);
	BSTR bsArgs = SysAllocString(pszCmdArgs ? pszCmdArgs : L"");
	BSTR bsDir = lpCurrentDirectory ? SysAllocString(lpCurrentDirectory) : NULL;
	BSTR bsRoot = SysAllocString(L"\\");

	// No need, using TASK_LOGON_INTERACTIVE_TOKEN now
	// -- VARIANT vtUsersSID = {VT_BSTR}; vtUsersSID.bstrVal = SysAllocString(L"S-1-5-32-545"); // Well Known SID for "\\Builtin\Users" group
	MBSTR szSystemSID(L"S-1-5-18");
	VARIANT vtSystemSID = {VT_BSTR}; vtSystemSID.bstrVal = szSystemSID;
	VARIANT vtZeroStr = {VT_BSTR}; vtZeroStr.bstrVal = SysAllocString(L"");
	VARIANT vtEmpty = {VT_EMPTY};

	const IID CLSID_TaskScheduler = {0x0f87369f, 0xa4e5, 0x4cfc, {0xbd, 0x3e, 0x73, 0xe6, 0x15, 0x45, 0x72, 0xdd}};
	const IID IID_IExecAction     = {0x4c3d624d, 0xfd6b, 0x49a3, {0xb9, 0xb7, 0x09, 0xcb, 0x3c, 0xd3, 0xf0, 0x47}};
	const IID IID_ITaskService    = {0x2faba4c7, 0x4da9, 0x4013, {0x96, 0x97, 0x20, 0xcc, 0x3f, 0xd4, 0x0f, 0x85}};

	IRegisteredTask *pRegisteredTask = NULL;
	IRunningTask *pRunningTask = NULL;
	IAction *pAction = NULL;
	IExecAction *pExecAction = NULL;
	IActionCollection *pActionCollection = NULL;
	ITaskDefinition *pTask = NULL;
	ITaskSettings *pSettings = NULL;
	ITaskFolder *pRootFolder = NULL;
	ITaskService *pService = NULL;
	HRESULT hr;
	TASK_STATE taskState, prevTaskState;
	bool bCoInitialized = false;
	DWORD nTickStart, nDuration;
	const DWORD nMaxWindowWait = 30000;
	const DWORD nMaxSystemWait = 30000;
	MBSTR szIndefinitely(L"PT0S");

	//  ------------------------------------------------------
	//  Initialize COM.
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"CoInitializeEx failed.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}
	bCoInitialized = true;

	//  Set general COM security levels.
	hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"CoInitializeSecurity failed.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	//  ------------------------------------------------------
	//  Create an instance of the Task Service.
	hr = CoCreateInstance( CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService );
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Failed to CoCreate an instance of the TaskService class.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	//  Connect to the task service.
	hr = pService->Connect(vtEmpty, vtEmpty, vtEmpty, vtEmpty);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"ITaskService::Connect failed.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	//  ------------------------------------------------------
	//  Get the pointer to the root task folder.  This folder will hold the
	//  new task that is registered.
	hr = pService->GetFolder(bsRoot, &pRootFolder);
	if( FAILED(hr) )
	{
		DisplayShedulerError(L"Cannot get Root Folder pointer.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	//  Check if the same task already exists. If the same task exists, remove it.
	hr = pRootFolder->DeleteTask(bsTaskName, 0);
	// We are creating "unique" task name, admitting it can't exists already, so ignore deletion error
	UNREFERENCED_PARAMETER(hr);

	//  Create the task builder object to create the task.
	hr = pService->NewTask(0, &pTask);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Failed to create an instance of the TaskService class.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	SafeRelease(pService);  // COM clean up.  Pointer is no longer used.

	//  ------------------------------------------------------
	//  Ensure there will be no execution time limit
	hr = pTask->get_Settings(&pSettings);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Cannot get task settings.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}
	hr = pSettings->put_ExecutionTimeLimit(szIndefinitely);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Cannot set task execution time limit.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	//  ------------------------------------------------------
	//  Add an Action to the task.

	//  Get the task action collection pointer.
	hr = pTask->get_Actions(&pActionCollection);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Cannot get task collection pointer.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	//  Create the action, specifying that it is an executable action.
	hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"pActionCollection->Create failed.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}
	SafeRelease(pActionCollection);  // COM clean up.  Pointer is no longer used.

	hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
	if( FAILED(hr) )
	{
		DisplayShedulerError(L"pAction->QueryInterface failed.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}
	SafeRelease(pAction);

	//  Set the path of the executable to the user supplied executable.
	hr = pExecAction->put_Path(bsExecutablePath);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Cannot set path of executable.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	hr = pExecAction->put_Arguments(bsArgs);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Cannot set arguments of executable.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	if (bsDir)
	{
		hr = pExecAction->put_WorkingDirectory(bsDir);
		if (FAILED(hr))
		{
			DisplayShedulerError(L"Cannot set working directory of executable.", hr, bsTaskName, lpCommandLine);
			goto wrap;
		}
	}

	//  ------------------------------------------------------
	//  Save the task in the root folder.
	hr = pRootFolder->RegisterTaskDefinition(bsTaskName, pTask, TASK_CREATE,
		//vtUsersSID, vtEmpty, TASK_LOGON_GROUP, // gh-571 - this may start process as wrong user
		bAsSystem ? vtSystemSID : vtEmpty, vtEmpty, bAsSystem ? TASK_LOGON_SERVICE_ACCOUNT : TASK_LOGON_INTERACTIVE_TOKEN,
		vtZeroStr, &pRegisteredTask);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Error registering the task instance.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	prevTaskState = (TASK_STATE)-1;

	//  ------------------------------------------------------
	//  Run the task
	hr = pRegisteredTask->Run(vtEmpty, &pRunningTask);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Error starting the task instance.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	HRESULT hrRun; hrRun = pRunningTask->get_State(&prevTaskState);

	//  ------------------------------------------------------
	// Success! Task successfully started. But our task may end
	// promptly because it just bypassing the command line
	lbRc = TRUE;

	if (bAsSystem)
	{
		nTickStart = GetTickCount();
		nDuration = 0;
		_ASSERTE(hCreated==NULL);
		hCreated = NULL;

		while (nDuration <= nMaxSystemWait/*30000*/)
		{
			hrRun = pRunningTask->get_State(&taskState);
			if (FAILED(hr))
				break;
			if (taskState == TASK_STATE_RUNNING)
				break; // OK, started
			if (taskState == TASK_STATE_READY)
				break; // Already finished?

			Sleep(100);
			nDuration = (GetTickCount() - nTickStart);
		}
	}
	else
	{
		// Success! Program was started.
		// Give 30 seconds for new window appears and bring it to front
		LPCWSTR pszExeName = PointToName(szExe);
		if (lstrcmpi(pszExeName, L"ConEmu.exe") == 0 || lstrcmpi(pszExeName, L"ConEmu64.exe") == 0)
		{
			nTickStart = GetTickCount();
			nDuration = 0;
			_ASSERTE(hCreated==NULL);
			hCreated = NULL;

			while (nDuration <= nMaxWindowWait/*30000*/)
			{
				HWND hTop = NULL;
				while ((hTop = FindWindowEx(NULL, hTop, VirtualConsoleClassMain, NULL)) != NULL)
				{
					if (!IsWindowVisible(hTop) || (hTop == hPrevEmu))
						continue;

					hCreated = hTop;
					SetForegroundWindow(hCreated);
					break;
				}

				if (hCreated != NULL)
				{
					// Window found, activated
					break;
				}

				Sleep(100);
				nDuration = (GetTickCount() - nTickStart);
			}
		}
		// Window activation end
	}

	//Delete the task when done
	hr = pRootFolder->DeleteTask(bsTaskName, NULL);
	if( FAILED(hr) )
	{
		DisplayShedulerError(L"CoInitializeEx failed.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}
	// Task successfully deleted

wrap:
	// Clean up
	SysFreeString(bsTaskName);
	SysFreeString(bsExecutablePath);
	SysFreeString(bsArgs);
	if (bsDir) SysFreeString(bsDir);
	SysFreeString(bsRoot);
	//VariantClear(&vtUsersSID);
	VariantClear(&vtZeroStr);
	SafeRelease(pRegisteredTask);
	SafeRelease(pRunningTask);
	SafeRelease(pAction);
	SafeRelease(pExecAction);
	SafeRelease(pActionCollection);
	SafeRelease(pTask);
	SafeRelease(pRootFolder);
	SafeRelease(pService);
	// Finalize
	if (bCoInitialized)
		CoUninitialize();
	if (pdwLastError) *pdwLastError = hr;
	// End of Task-scheduler mode
#endif

	return lbRc;
}

BOOL CreateProcessDemoted(LPWSTR lpCommandLine,
						  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
						  BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
						  LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
						  LPDWORD pdwLastError)
{
	BOOL lbRc;

	lbRc = CreateProcessSheduled(false, lpCommandLine,
						   lpProcessAttributes, lpThreadAttributes,
						   bInheritHandles, dwCreationFlags, lpEnvironment,
						   lpCurrentDirectory, lpStartupInfo, lpProcessInformation,
						   pdwLastError);

	// If all methods fails - try to execute "as is"?
	if (!lbRc)
	{
		lbRc = CreateProcess(NULL, lpCommandLine,
							 lpProcessAttributes, lpThreadAttributes,
							 bInheritHandles, dwCreationFlags, lpEnvironment,
							 lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
		if (pdwLastError) *pdwLastError = GetLastError();
	}

	return lbRc;
}



bool isKey(DWORD wp,DWORD vk)
{
	bool bEq = ((wp==vk)
		|| ((vk==VK_LSHIFT||vk==VK_RSHIFT)&&wp==VK_SHIFT)
		|| ((vk==VK_LCONTROL||vk==VK_RCONTROL)&&wp==VK_CONTROL)
		|| ((vk==VK_LMENU||vk==VK_RMENU)&&wp==VK_MENU));
	return bEq;
}

#ifdef DEBUG_MSG_HOOKS
HHOOK ghDbgHook = NULL;
LRESULT CALLBACK DbgCallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		CWPSTRUCT* p = (CWPSTRUCT*)lParam;
		if (p->message == WM_SETHOTKEY)
		{
			DEBUGSTRSETHOTKEY(L"WM_SETHOTKEY triggered");
		}
	}
	return CallNextHookEx(ghDbgHook, nCode, wParam, lParam);
}
#endif


LRESULT CALLBACK AppWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	if (gpSetCls->isAdvLogging >= 4)
	{
		gpConEmu->LogMessage(hWnd, messg, wParam, lParam);
	}

	if (messg == WM_SETHOTKEY)
	{
		gnWndSetHotkey = wParam;
	}

	if (messg == WM_CREATE)
	{
		if (ghWndApp == NULL)
			ghWndApp = hWnd;
	}
	else if (messg == WM_ACTIVATEAPP)
	{
		if (wParam && ghWnd)
			gpConEmu->setFocus();
	}

	result = DefWindowProc(hWnd, messg, wParam, lParam);
	return result;
}

// z120713 - В потоке CRealConsole::MonitorThread возвращаются
// отличные от основного потока HWND. В результате, а также из-за
// отложенного выполнения, UpdateServerActive передавал Thaw==FALSE
HWND ghLastForegroundWindow = NULL;
HWND getForegroundWindow()
{
	HWND h = NULL;
	if (!ghWnd || isMainThread())
	{
		ghLastForegroundWindow = h = ::GetForegroundWindow();
	}
	else
	{
		h = ghLastForegroundWindow;
		if (h && !IsWindow(h))
			h = NULL;
	}
	return h;
}

BOOL CheckCreateAppWindow()
{
	if (!gpSet->NeedCreateAppWindow())
	{
		// Если окно не требуется
		if (ghWndApp)
		{
			// Вызов DestroyWindow(ghWndApp); закроет и "дочернее" ghWnd
			_ASSERTE(ghWnd==NULL);
			if (ghWnd)
				gpConEmu->SetParent(NULL);
			DestroyWindow(ghWndApp);
			ghWndApp = NULL;
		}
		return TRUE;
	}

	WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_DBLCLKS|CS_OWNDC, AppWndProc, 0, 0,
	                 g_hInstance, hClassIcon, LoadCursor(NULL, IDC_ARROW),
	                 NULL /*(HBRUSH)COLOR_BACKGROUND*/,
	                 NULL, gsClassNameApp, hClassIconSm
	                };// | CS_DROPSHADOW

	if (!RegisterClassEx(&wc))
		return FALSE;


	gpConEmu->LogString(L"Creating app window", false);


	//ghWnd = CreateWindow(szClassName, 0, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, gpSet->wndX, gpSet->wndY, cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4, NULL, NULL, (HINSTANCE)g_hInstance, NULL);
	DWORD style = WS_OVERLAPPEDWINDOW | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
	int nWidth=100, nHeight=100, nX = -32000, nY = -32000;
	DWORD exStyle = WS_EX_TOOLWINDOW|WS_EX_ACCEPTFILES;
	// cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4; -- все равно это было не правильно
	ghWndApp = CreateWindowEx(exStyle, gsClassNameApp, gpConEmu->GetDefaultTitle(), style, nX, nY, nWidth, nHeight, NULL, NULL, (HINSTANCE)g_hInstance, NULL);

	if (!ghWndApp)
	{
		WarnCreateWindowFail(L"application window", NULL, GetLastError());
		return FALSE;
	}

	if (gpSetCls->isAdvLogging)
	{
		wchar_t szCreated[128];
		_wsprintf(szCreated, SKIPLEN(countof(szCreated)) L"App window created, HWND=0x%08X\r\n", LODWORD(ghWndApp));
		gpConEmu->LogString(szCreated, false, false);
	}

	return TRUE;
}

LRESULT CALLBACK SkipShowWindowProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	//static UINT nMsgBtnCreated = 0;

	if (messg == WM_SETHOTKEY)
	{
		gnWndSetHotkey = wParam;
	}

	switch (messg)
	{
	case WM_PAINT:
		{
			PAINTSTRUCT ps = {};
			BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
		}
		return 0;

	case WM_ERASEBKGND:
		return TRUE;

	//default:
	//	if (!nMsgBtnCreated)
	//	{
	//		nMsgBtnCreated = RegisterWindowMessage(L"TaskbarButtonCreated");
	//	}

	//	if (messg == nMsgBtnCreated)
	//	{
	//		gpConEmu->Taskbar_DeleteTabXP(hWnd);
	//	}
	}

	result = DefWindowProc(hWnd, messg, wParam, lParam);
	return result;
}

void SkipOneShowWindow()
{
	static bool bProcessed = false;
	if (bProcessed)
		return; // уже
	bProcessed = true;

	STARTUPINFO si = {sizeof(si)};
	GetStartupInfo(&si);
	if (si.wShowWindow == SW_SHOWNORMAL)
		return; // финты не требуются

	const wchar_t szSkipClass[] = L"ConEmuSkipShowWindow";
	WNDCLASSEX wc = {sizeof(WNDCLASSEX), 0, SkipShowWindowProc, 0, 0,
	                 g_hInstance, hClassIcon, LoadCursor(NULL, IDC_ARROW),
	                 NULL /*(HBRUSH)COLOR_BACKGROUND*/,
	                 NULL, szSkipClass, hClassIconSm
	                };// | CS_DROPSHADOW

	if (!RegisterClassEx(&wc))
		return;


	gpConEmu->LogString(L"SkipOneShowWindow");


	gpConEmu->Taskbar_Init();

	//ghWnd = CreateWindow(szClassName, 0, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, gpSet->wndX, gpSet->wndY, cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4, NULL, NULL, (HINSTANCE)g_hInstance, NULL);
	DWORD style = WS_OVERLAPPED | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	int nWidth=100, nHeight=100, nX = -32000, nY = -32000;
	DWORD exStyle = WS_EX_TOOLWINDOW;
	HWND hSkip = CreateWindowEx(exStyle, szSkipClass, L"", style, nX, nY, nWidth, nHeight, NULL, NULL, (HINSTANCE)g_hInstance, NULL);

	if (hSkip)
	{
		HRGN hRgn = CreateRectRgn(0,0,1,1);
		SetWindowRgn(hSkip, hRgn, FALSE);

		ShowWindow(hSkip, SW_SHOWNORMAL);
		gpConEmu->Taskbar_DeleteTabXP(hSkip);
		DestroyWindow(hSkip);

		if (gpSetCls->isAdvLogging)
		{
			wchar_t szInfo[128];
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Skip window 0x%08X was created and destroyed", LODWORD(hSkip));
			gpConEmu->LogString(szInfo);
		}
	}

	// Класс более не нужен
	UnregisterClass(szSkipClass, g_hInstance);

	return;
}

struct FindProcessWindowArg
{
	HWND  hwnd;
	DWORD nPID;
};

static BOOL CALLBACK FindProcessWindowEnum(HWND hwnd, LPARAM lParam)
{
	FindProcessWindowArg* pArg = (FindProcessWindowArg*)lParam;

	if (!IsWindowVisible(hwnd))
		return TRUE; // next window

	DWORD nPID = 0;
	if (!GetWindowThreadProcessId(hwnd, &nPID) || (nPID != pArg->nPID))
		return TRUE; // next window

	pArg->hwnd = hwnd;
	return FALSE; // found
}

HWND FindProcessWindow(DWORD nPID)
{
	FindProcessWindowArg args = {NULL, nPID};
	EnumWindows(FindProcessWindowEnum, (LPARAM)&args);
	return args.hwnd;
}

HWND ghDlgPendingFrom = NULL;
void PatchMsgBoxIcon(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	if (!ghDlgPendingFrom)
		return;

	HWND hFore = GetForegroundWindow();
	HWND hActive = GetActiveWindow();
	if (hFore && (hFore != ghDlgPendingFrom))
	{
		DWORD nPID = 0;
		GetWindowThreadProcessId(hFore, &nPID);
		if (nPID == GetCurrentProcessId())
		{
			wchar_t szClass[32] = L""; GetClassName(hFore, szClass, countof(szClass));
			if (lstrcmp(szClass, L"#32770") == 0)
			{
				// Reset immediately, to avoid stack overflow
				ghDlgPendingFrom = NULL;
				// And patch the icon
				SendMessage(hFore, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
				SendMessage(hFore, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);
			}
		}
	}
}

LONG gnInMsgBox = 0;
int MsgBox(LPCTSTR lpText, UINT uType, LPCTSTR lpCaption /*= NULL*/, HWND ahParent /*= (HWND)-1*/, bool abModal /*= true*/)
{
	DontEnable de(abModal);

	ghDlgPendingFrom = GetForegroundWindow();

	HWND hParent = gbMessagingStarted
		? ((ahParent == (HWND)-1) ? ghWnd :ahParent)
		: NULL;

	HooksUnlocker;
	MSetter lInCall(&gnInMsgBox);

	if (gpSetCls->isAdvLogging)
	{
		CEStr lsLog(lpCaption, lpCaption ? L":: " : NULL, lpText);
		LogString(lsLog);
	}

	// If there were problems with displaying error box, MessageBox will return default button
	// This may cause infinite loops in some cases
	SetLastError(0);
	int nBtn = MessageBox(hParent, lpText ? lpText : L"<NULL>", lpCaption ? lpCaption : gpConEmu->GetLastTitle(), uType);
	DWORD nErr = GetLastError();

	ghDlgPendingFrom = NULL;

	UNREFERENCED_PARAMETER(nErr);
	return nBtn;
}


// Возвращает текст с информацией о пути к сохраненному дампу
// DWORD CreateDumpForReport(LPEXCEPTION_POINTERS ExceptionInfo, wchar_t (&szFullInfo)[1024], LPCWSTR pszComment = NULL);
#include "../common/Dump.h"

void AssertBox(LPCTSTR szText, LPCTSTR szFile, UINT nLine, LPEXCEPTION_POINTERS ExceptionInfo /*= NULL*/)
{
	#ifdef _DEBUG
	//_ASSERTE(FALSE);
	#endif

	static bool bInAssert = false;

	int nRet = IDRETRY;

	DWORD    nPreCode = GetLastError();
	wchar_t  szAssertInfo[4096], szCodes[128];
	wchar_t  szFullInfo[1024] = L"";
	wchar_t  szDmpFile[MAX_PATH+64] = L"";
	size_t   cchMax = (szText ? _tcslen(szText) : 0) + (szFile ? _tcslen(szFile) : 0)
		+ (gpConEmu ? (
				(gpConEmu->ms_ConEmuExe ? _tcslen(gpConEmu->ms_ConEmuExe) : 0)
				+ (gpConEmu->ms_ConEmuBuild ? _tcslen(gpConEmu->ms_ConEmuBuild) : 0)) : 0)
		+ 300;
	wchar_t* pszFull = (cchMax <= countof(szAssertInfo)) ? szAssertInfo : (wchar_t*)malloc(cchMax*sizeof(*pszFull));
	wchar_t* pszDumpMessage = NULL;

	#ifdef _DEBUG
	MyAssertDumpToFile(szFile, nLine, szText);
	#endif

	LPCWSTR  pszTitle = gpConEmu ? gpConEmu->GetDefaultTitle() : NULL;
	if (!pszTitle || !*pszTitle) pszTitle = L"?ConEmu?";

	if (pszFull)
	{
		_wsprintf(pszFull, SKIPLEN(cchMax)
			L"Assertion in %s [%s]\r\n%s\r\nat\r\n%s:%i\r\n\r\n"
			L"Press <Abort> to throw exception, ConEmu will be terminated!\r\n\r\n"
			L"Press <Retry> to copy text information to clipboard\r\n"
			L"and report a bug (open project web page)",
			(gpConEmu && gpConEmu->ms_ConEmuExe) ? gpConEmu->ms_ConEmuExe : L"<NULL>",
			(gpConEmu && gpConEmu->ms_ConEmuBuild) ? gpConEmu->ms_ConEmuBuild : L"<NULL>",
			szText, szFile, nLine);

		DWORD nPostCode = (DWORD)-1;

		if (bInAssert)
		{
			nPostCode = (DWORD)-2;
			nRet = IDCANCEL;
		}
		else
		{
			bInAssert = true;
			nRet = MsgBox(pszFull, MB_ABORTRETRYIGNORE|MB_ICONSTOP|MB_SYSTEMMODAL|MB_DEFBUTTON3, pszTitle, NULL);
			bInAssert = false;
			nPostCode = GetLastError();
		}

		_wsprintf(szCodes, SKIPLEN(countof(szCodes)) L"\r\nPreError=%i, PostError=%i, Result=%i", nPreCode, nPostCode, nRet);
		_wcscat_c(pszFull, cchMax, szCodes);
	}

	if ((nRet == IDRETRY) || (nRet == IDABORT))
	{
		bool bProcessed = false;

		if (nRet == IDABORT)
		{
			RaiseTestException();
			bProcessed = true;
		}

		if (!bProcessed)
		{
			//-- Не нужно, да и дамп некорректно формируется, если "руками" ex формировать.
			//EXCEPTION_RECORD er0 = {0xC0000005}; er0.ExceptionAddress = AssertBox;
			//EXCEPTION_POINTERS ex0 = {&er0};
			//if (!ExceptionInfo) ExceptionInfo = &ex0;

			CreateDumpForReport(ExceptionInfo, szFullInfo, szDmpFile, pszFull);
		}

		if (szFullInfo[0])
		{
			wchar_t* pszFileMsg = szDmpFile[0] ? lstrmerge(L"\r\n\r\n" L"Memory dump was saved to\r\n", szDmpFile,
				L"\r\n\r\n" L"Please Zip it and send to developer (via DropBox etc.)\r\n",
				CEREPORTCRASH /* https://conemu.github.io/en/Issues.html... */) : NULL;
			pszDumpMessage = lstrmerge(pszFull, L"\r\n\r\n", szFullInfo, pszFileMsg);
			CopyToClipboard(pszDumpMessage ? pszDumpMessage : szFullInfo);
			SafeFree(pszFileMsg);
		}
		else if (pszFull)
		{
			CopyToClipboard(pszFull);
		}

		ConEmuAbout::OnInfo_ReportCrash(pszDumpMessage ? pszDumpMessage : pszFull);
	}

	if (pszFull && pszFull != szAssertInfo)
	{
		free(pszFull);
	}

	SafeFree(pszDumpMessage);
}

BOOL gbInDisplayLastError = FALSE;

int DisplayLastError(LPCTSTR asLabel, DWORD dwError /* =0 */, DWORD dwMsgFlags /* =0 */, LPCWSTR asTitle /*= NULL*/, HWND hParent /*= NULL*/)
{
	int nBtn = 0;
	DWORD dw = dwError ? dwError : GetLastError();
	wchar_t* lpMsgBuf = NULL;
	wchar_t *out = NULL;
	MCHKHEAP

	if (dw && (dw != (DWORD)-1))
	{
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL);
		INT_PTR nLen = _tcslen(asLabel)+64+(lpMsgBuf ? _tcslen(lpMsgBuf) : 0);
		out = new wchar_t[nLen];
		_wsprintf(out, SKIPLEN(nLen) _T("%s\nLastError=0x%08X\n%s"), asLabel, dw, lpMsgBuf);
	}

	if (gbMessagingStarted) apiSetForegroundWindow(hParent ? hParent : ghWnd);

	if (!dwMsgFlags) dwMsgFlags = MB_SYSTEMMODAL | MB_ICONERROR;

	BOOL lb = gbInDisplayLastError; gbInDisplayLastError = TRUE;
	nBtn = MsgBox(out ? out : asLabel, dwMsgFlags, asTitle, hParent);
	gbInDisplayLastError = lb;

	MCHKHEAP
	if (lpMsgBuf)
		LocalFree(lpMsgBuf);
	if (out)
		delete [] out;
	MCHKHEAP
	return nBtn;
}

void WarnCreateWindowFail(LPCWSTR pszDescription, HWND hParent, DWORD nErrCode)
{
	wchar_t szCreateFail[256];

	if (gpConEmu && gpConEmu->mp_Inside)
	{
		_wsprintf(szCreateFail, SKIPCOUNT(szCreateFail)
			L"Inside mode: Parent (%s): PID=%u ParentPID=%u HWND=x%p EXE=",
			(::IsWindow(gpConEmu->mp_Inside->mh_InsideParentWND) ? L"Valid" : L"Invalid"),
			gpConEmu->mp_Inside->m_InsideParentInfo.ParentPID,
			gpConEmu->mp_Inside->m_InsideParentInfo.ParentParentPID,
			(LPVOID)gpConEmu->mp_Inside->mh_InsideParentWND);
		CEStr lsLog(szCreateFail, gpConEmu->mp_Inside->m_InsideParentInfo.ExeName);
		LogString(lsLog);
	}

	_wsprintf(szCreateFail, SKIPCOUNT(szCreateFail)
		L"Create %s FAILED (code=%u)! Parent=x%p%s%s",
		pszDescription ? pszDescription : L"window", nErrCode, (LPVOID)hParent,
		(hParent ? (::IsWindow(hParent) ? L" Valid" : L" Invalid") : L""),
		(hParent ? (::IsWindowVisible(hParent) ? L" Visible" : L" Hidden") : L"")
		);
	LogString(szCreateFail);

	// Don't warn, if "Inside" mode was requested and parent was closed
	if (!gpConEmu || !gpConEmu->isInsideInvalid())
	{
		DisplayLastError(szCreateFail, nErrCode);
	}
}

RECT CenterInParent(RECT rcDlg, HWND hParent)
{
	RECT rcParent; GetWindowRect(hParent, &rcParent);

	int nWidth  = (rcDlg.right - rcDlg.left);
	int nHeight = (rcDlg.bottom - rcDlg.top);

	MONITORINFO mi = {sizeof(mi)};
	GetNearestMonitorInfo(&mi, NULL, &rcParent);

	RECT rcCenter = {
		max(mi.rcWork.left, rcParent.left + (rcParent.right - rcParent.left - nWidth) / 2),
		max(mi.rcWork.top, rcParent.top + (rcParent.bottom - rcParent.top - nHeight) / 2)
	};

	if (((rcCenter.left + nWidth) > mi.rcWork.right)
		&& (rcCenter.left > mi.rcWork.left))
	{
		rcCenter.left = max(mi.rcWork.left, (mi.rcWork.right - nWidth));
	}

	if (((rcCenter.top + nHeight) > mi.rcWork.bottom)
		&& (rcCenter.top > mi.rcWork.top))
	{
		rcCenter.top = max(mi.rcWork.top, (mi.rcWork.bottom - nHeight));
	}

	rcCenter.right = rcCenter.left + nWidth;
	rcCenter.bottom = rcCenter.top + nHeight;

	return rcCenter;
}

BOOL MoveWindowRect(HWND hWnd, const RECT& rcWnd, BOOL bRepaint)
{
	BOOL lbRc;

	if (gpConEmu && (ghWnd == hWnd))
		lbRc = gpConEmu->setWindowPos(NULL, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top, SWP_NOZORDER|(bRepaint?0:SWP_NOREDRAW));
	else
		lbRc = MoveWindow(hWnd, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top, bRepaint);

	return lbRc;
}

HICON CreateNullIcon()
{
	static HICON hNullIcon = NULL;

	if (!hNullIcon)
	{
		BYTE NilBits[16*16/8] = {};
		BYTE SetBits[16*16/8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
								0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
		hNullIcon = CreateIcon(NULL, 16, 16, 1, 1, SetBits, NilBits);
	}

	return hNullIcon;
}

void MessageLoop()
{
	MSG Msg = {NULL};
	gbMessagingStarted = true;

	#ifdef _DEBUG
	wchar_t szDbg[128];
	#endif

	while (GetMessage(&Msg, NULL, 0, 0))
	{
		#ifdef _DEBUG
		if (Msg.message == WM_TIMER)
		{
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_TIMER(0x%08X,%u)\n", LODWORD(Msg.hwnd), Msg.wParam);
			DEBUGSTRTIMER(szDbg);
		}
		#endif

		if (!ProcessMessage(Msg))
			break;
	}

	gbMessagingStarted = false;
}

bool PtMouseDblClickTest(const MSG& msg1, const MSG msg2)
{
	if (msg1.message != msg2.message)
		return false;

	// Maximum DblClick time is 5 sec
	UINT nCurTimeDiff = msg2.time - msg1.time;
	if (nCurTimeDiff > 5000)
	{
		return false;
	}
	else if (nCurTimeDiff)
	{
		UINT nTimeDiff = GetDoubleClickTime();
		if (nCurTimeDiff > nTimeDiff)
			return false;
	}

	// Check coord diff
	POINT pt1 = {(i16)LOWORD(msg1.lParam), (i16)HIWORD(msg1.lParam)};
	POINT pt2 = {(i16)LOWORD(msg2.lParam), (i16)HIWORD(msg2.lParam)};
	// Due to mouse captures hwnd may differ
	if (msg1.hwnd != msg2.hwnd)
	{
		ClientToScreen(msg1.hwnd, &pt1);
		ClientToScreen(msg2.hwnd, &pt2);
	}

	if ((pt1.x != pt2.x) || (pt1.y != pt2.y))
	{
		bool bDiffOk;
		int dx1 = GetSystemMetrics(SM_CXDOUBLECLK), dx2 = GetSystemMetrics(SM_CYDOUBLECLK);
		bDiffOk = PtDiffTest(pt1.x, pt1.y, pt2.x, pt2.y, dx1, dx2);
		if (!bDiffOk)
		{
			// May be fin in dpi*multiplied?
			int dpiDX = gpSetCls->EvalSize(dx1, esf_Horizontal|esf_CanUseDpi);
			int dpiDY = gpSetCls->EvalSize(dx2, esf_Vertical|esf_CanUseDpi);
			if (PtDiffTest(pt1.x, pt1.y, pt2.x, pt2.y, dpiDX, dpiDY))
				bDiffOk = true;
		}
		if (!bDiffOk)
			return false;
	}

	return true;
}

bool ProcessMessage(MSG& Msg)
{
	bool bRc = true;
	static bool bQuitMsg = false;
	static MSG MouseClickPatch = {};

	#ifdef _DEBUG
	static DWORD LastInsideCheck = 0;
	const  DWORD DeltaInsideCheck = 1000;
	#endif

	MSetter nestedLevel(&gnMessageNestingLevel);

	if (Msg.message == WM_QUIT)
	{
		bQuitMsg = true;
		bRc = false;
		goto wrap;
	}

	// Do minimal in-memory logging (small circular buffer)
	ConEmuMsgLogger::Log(Msg, ConEmuMsgLogger::msgCommon);

	if (gpConEmu)
	{
		#ifdef _DEBUG
		if (gpConEmu->mp_Inside)
		{
			DWORD nCurTick = GetTickCount();
			if (!LastInsideCheck)
			{
				LastInsideCheck = nCurTick;
			}
			else if ((LastInsideCheck - nCurTick) >= DeltaInsideCheck)
			{
				if (gpConEmu->isInsideInvalid())
				{
					// Show assertion once
					static bool bWarned = false;
					if (!bWarned)
					{
						bWarned = true;
						_ASSERTE(FALSE && "Parent was terminated, but ConEmu wasn't");
					}
				}
				LastInsideCheck = nCurTick;
			}
		}
		#endif

		if (gpConEmu->isDialogMessage(Msg))
			goto wrap;

		switch (Msg.message)
		{
		case WM_SYSCOMMAND:
			if (gpConEmu->isSkipNcMessage(Msg))
				goto wrap;
			break;
		case WM_HOTKEY:
			gpConEmu->OnWmHotkey(Msg.wParam, Msg.time);
			goto wrap;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
			// gh#470: If user holds Alt key, than we will not receive DblClick messages, only single clicks...
			if (PtMouseDblClickTest(MouseClickPatch, Msg))
			{
				// Convert Click to DblClick message
				_ASSERTE((WM_LBUTTONDBLCLK-WM_LBUTTONDOWN)==2 && (WM_RBUTTONDBLCLK-WM_RBUTTONDOWN)==2 && (WM_MBUTTONDBLCLK-WM_MBUTTONDOWN)==2);
				_ASSERTE((WM_XBUTTONDBLCLK-WM_XBUTTONDOWN)==2);
				Msg.message += (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN);
			}
			else
			{
				// Diffent mouse button was pressed, or not saved yet
				// Store mouse message
				MouseClickPatch = Msg;
			}
			break;
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDBLCLK:
			ZeroStruct(MouseClickPatch);
			break;
		}
	}

	TranslateMessage(&Msg);
	DispatchMessage(&Msg);

wrap:
	return bRc;
}

static DWORD gn_MainThreadId;
bool isMainThread()
{
	DWORD dwTID = GetCurrentThreadId();
	return (dwTID == gn_MainThreadId);
}

HWND FindTopExplorerWindow()
{
	wchar_t szClass[MAX_PATH] = L"";
	HWND hwndFind = NULL;

	while ((hwndFind = FindWindowEx(NULL, hwndFind, NULL, NULL)) != NULL)
	{
		if ((GetClassName(hwndFind, szClass, countof(szClass)) > 0)
			&& CDefTermBase::IsExplorerWindowClass(szClass))
			break;
	}

	return hwndFind;
}

wchar_t* getFocusedExplorerWindowPath()
{
#define FE_CHECK_OUTER_FAIL(statement) \
	if (!SUCCEEDED(statement)) goto outer_fail;

#define FE_CHECK_FAIL(statement) \
	if (!SUCCEEDED(statement)) goto fail;

#define FE_RELEASE(hnd) \
	if (hnd) { hnd->Release(); hnd = NULL; }

	wchar_t* ret = NULL;
	wchar_t szPath[MAX_PATH] = L"";

	IShellBrowser *psb = NULL;
	IShellView *psv = NULL;
	IFolderView *pfv = NULL;
	IPersistFolder2 *ppf2 = NULL;
	IDispatch  *pdisp = NULL;
	IWebBrowserApp *pwba = NULL;
	IServiceProvider *psp = NULL;
	IShellWindows *psw = NULL;

	VARIANT v;
	HWND hwndWBA;
	LPITEMIDLIST pidlFolder;

	BOOL fFound = FALSE;
	HWND hwndFind = FindTopExplorerWindow();

	FE_CHECK_OUTER_FAIL(CoCreateInstance(CLSID_ShellWindows, NULL, CLSCTX_ALL,
		IID_IShellWindows, (void**)&psw))

	V_VT(&v) = VT_I4;
	for (V_I4(&v) = 0; !fFound && psw->Item(v, &pdisp) == S_OK; V_I4(&v)++)
	{
		FE_CHECK_FAIL(pdisp->QueryInterface(IID_IWebBrowserApp, (void**)&pwba))
		FE_CHECK_FAIL(pwba->get_HWND((LONG_PTR*)&hwndWBA))

		if (hwndWBA != hwndFind)
			goto fail;

		fFound = TRUE;
		FE_CHECK_FAIL(pwba->QueryInterface(IID_IServiceProvider, (void**)&psp))
		FE_CHECK_FAIL(psp->QueryService(SID_STopLevelBrowser, IID_IShellBrowser, (void**)&psb))
		FE_CHECK_FAIL(psb->QueryActiveShellView(&psv))
		FE_CHECK_FAIL(psv->QueryInterface(IID_IFolderView, (void**)&pfv))
		FE_CHECK_FAIL(pfv->GetFolder(IID_IPersistFolder2, (void**)&ppf2))
		FE_CHECK_FAIL(ppf2->GetCurFolder(&pidlFolder))

		if (!SHGetPathFromIDList(pidlFolder, szPath) || !*szPath)
			goto fail;

		ret = lstrdup(szPath);

		CoTaskMemFree(pidlFolder);

		fail:
		FE_RELEASE(ppf2)
		FE_RELEASE(pfv)
		FE_RELEASE(psv)
		FE_RELEASE(psb)
		FE_RELEASE(psp)
		FE_RELEASE(pwba)
		FE_RELEASE(pdisp)
	}

	outer_fail:
	FE_RELEASE(psw)

	return ret;

#undef FE_CHECK_OUTER_FAIL
#undef FE_CHECK_FAIL
#undef FE_RELEASE
}

#ifndef __GNUC__
// Creates a CLSID_ShellLink to insert into the Tasks section of the Jump List.  This type of Jump
// List item allows the specification of an explicit command line to execute the task.
// Used only for JumpList creation...
static HRESULT _CreateShellLink(PCWSTR pszArguments, PCWSTR pszPrefix, PCWSTR pszTitle, IShellLink **ppsl)
{
	if ((!pszArguments || !*pszArguments) && (!pszTitle || !*pszTitle))
	{
		return E_INVALIDARG;
	}

	LPCWSTR pszConfig = gpSetCls->GetConfigName();
	if (pszConfig && !*pszConfig)
		pszConfig = NULL;

	CEStr lsTempBuf;
	LPCWSTR pszConEmuStartArgs = gpConEmu->MakeConEmuStartArgs(lsTempBuf);

	wchar_t* pszBuf = NULL;
	if (!pszArguments || !*pszArguments)
	{
		size_t cchMax = _tcslen(pszTitle)
			+ (pszPrefix ? _tcslen(pszPrefix) : 0)
			+ (pszConEmuStartArgs ? _tcslen(pszConEmuStartArgs) : 0)
			+ 32;

		pszBuf = (wchar_t*)malloc(cchMax*sizeof(*pszBuf));
		if (!pszBuf)
			return E_UNEXPECTED;

		pszBuf[0] = 0;
		if (pszPrefix && *pszPrefix)
		{
			_wcscat_c(pszBuf, cchMax, pszPrefix);
			_wcscat_c(pszBuf, cchMax, L" ");
		}
		if (pszConEmuStartArgs && *pszConEmuStartArgs)
		{
			_ASSERTE(pszConEmuStartArgs[_tcslen(pszConEmuStartArgs)-1]==L' ');
			_wcscat_c(pszBuf, cchMax, pszConEmuStartArgs);
		}
		_wcscat_c(pszBuf, cchMax, L"/cmd ");
		_wcscat_c(pszBuf, cchMax, pszTitle);
		pszArguments = pszBuf;
	}
	else if (pszPrefix)
	{
		size_t cchMax = _tcslen(pszArguments)
			+ _tcslen(pszPrefix)
			+ (pszConfig ? _tcslen(pszConfig) : 0)
			+ 32;
		pszBuf = (wchar_t*)malloc(cchMax*sizeof(*pszBuf));
		if (!pszBuf)
			return E_UNEXPECTED;

		pszBuf[0] = 0;
		_wcscat_c(pszBuf, cchMax, pszPrefix);
		_wcscat_c(pszBuf, cchMax, L" ");
		if (pszConfig)
		{
			_wcscat_c(pszBuf, cchMax, L"/config \"");
			_wcscat_c(pszBuf, cchMax, pszConfig);
			_wcscat_c(pszBuf, cchMax, L"\" ");
		}
		_wcscat_c(pszBuf, cchMax, L"/cmd ");
		_wcscat_c(pszBuf, cchMax, pszArguments);
		pszArguments = pszBuf;
	}

	IShellLink *psl;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&psl);
	if (SUCCEEDED(hr))
	{
		// Determine our executable's file path so the task will execute this application
		WCHAR szAppPath[MAX_PATH];
		if (GetModuleFileName(NULL, szAppPath, ARRAYSIZE(szAppPath)))
		{
			hr = psl->SetPath(szAppPath);

			// Иконка
			CEStr szTmp;
			CEStr szIcon; int iIcon = 0;
			CEStr szBatch;
			LPCWSTR pszTemp = pszArguments;
			LPCWSTR pszIcon = NULL;
			RConStartArgs args;

			while (NextArg(&pszTemp, szTmp) == 0)
			{
				if (lstrcmpi(szTmp, L"/icon") == 0)
				{
					if (NextArg(&pszTemp, szTmp) == 0)
						pszIcon = szTmp;
					break;
				}
				else if (lstrcmpi(szTmp, L"/cmd") == 0)
				{
					if ((*pszTemp == CmdFilePrefix)
						|| (*pszTemp == TaskBracketLeft) || (lstrcmp(pszTemp, AutoStartTaskName) == 0))
					{
						szBatch = gpConEmu->LoadConsoleBatch(pszTemp);
					}

					if (!szBatch.IsEmpty())
					{
						pszTemp = gpConEmu->ParseScriptLineOptions(szBatch, NULL, &args);

						// Icon may be defined in -new_console:C:...
						if (!pszIcon)
						{
							if (!args.pszIconFile)
							{
								_ASSERTE(args.pszSpecialCmd == NULL);
								args.pszSpecialCmd = lstrdup(pszTemp);
								args.ProcessNewConArg();
							}
							if (args.pszIconFile)
								pszIcon = args.pszIconFile;
						}
					}

					if (!pszIcon)
					{
						szTmp.Empty();
						if (NextArg(&pszTemp, szTmp) == 0)
							pszIcon = szTmp;
					}
					break;
				}
			}

			szIcon.Empty();
			if (pszIcon && *pszIcon)
			{
				CEStr lsTempIcon;
				lsTempIcon.Set(pszIcon);
				CIconList::ParseIconFileIndex(lsTempIcon, iIcon);

				CEStr szIconExp = ExpandEnvStr(lsTempIcon);
				LPCWSTR pszSearch = szIconExp.IsEmpty() ? lsTempIcon.ms_Val : szIconExp.ms_Val;

				if ((!apiGetFullPathName(pszSearch, szIcon)
						|| !FileExists(szIcon))
					&& !apiSearchPath(NULL, pszSearch, NULL, szIcon)
					&& !apiSearchPath(NULL, pszSearch, L".exe", szIcon))
				{
					szIcon.Empty();
					iIcon = 0;
				}
			}

			psl->SetIconLocation(szIcon.IsEmpty() ? szAppPath : szIcon.ms_Val, iIcon);

			DWORD n = GetCurrentDirectory(countof(szAppPath), szAppPath);
			if (n && (n < countof(szAppPath)))
				psl->SetWorkingDirectory(szAppPath);

			if (SUCCEEDED(hr))
			{
				hr = psl->SetArguments(pszArguments);
				if (SUCCEEDED(hr))
				{
					// The title property is required on Jump List items provided as an IShellLink
					// instance.  This value is used as the display name in the Jump List.
					IPropertyStore *pps;
					hr = psl->QueryInterface(IID_PPV_ARGS(&pps));
					if (SUCCEEDED(hr))
					{
						PROPVARIANT propvar = {VT_BSTR};
						//hr = InitPropVariantFromString(pszTitle, &propvar);
						propvar.bstrVal = ::SysAllocString(pszTitle);
						hr = pps->SetValue(PKEY_Title, propvar);
						if (SUCCEEDED(hr))
						{
							hr = pps->Commit();
							if (SUCCEEDED(hr))
							{
								hr = psl->QueryInterface(IID_PPV_ARGS(ppsl));
							}
						}
						//PropVariantClear(&propvar);
						::SysFreeString(propvar.bstrVal);
						pps->Release();
					}
				}
			}
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		psl->Release();
	}

	if (pszBuf)
		free(pszBuf);
	return hr;
}

// The Tasks category of Jump Lists supports separator items.  These are simply IShellLink instances
// that have the PKEY_AppUserModel_IsDestListSeparator property set to TRUE.  All other values are
// ignored when this property is set.
HRESULT _CreateSeparatorLink(IShellLink **ppsl)
{
	IPropertyStore *pps;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IPropertyStore, (void**)&pps);
	if (SUCCEEDED(hr))
	{
		PROPVARIANT propvar = {VT_BOOL};
		//hr = InitPropVariantFromBoolean(TRUE, &propvar);
		propvar.boolVal = VARIANT_TRUE;
		hr = pps->SetValue(PKEY_AppUserModel_IsDestListSeparator, propvar);
		if (SUCCEEDED(hr))
		{
			hr = pps->Commit();
			if (SUCCEEDED(hr))
			{
				hr = pps->QueryInterface(IID_PPV_ARGS(ppsl));
			}
		}
		//PropVariantClear(&propvar);
		pps->Release();
	}
	return hr;
}
#endif

bool UpdateWin7TaskList(bool bForce, bool bNoSuccMsg /*= false*/)
{
	// Добавляем Tasks, они есть только в Win7+
	if (gnOsVer < 0x601)
		return false;

	// -- т.к. работа с TaskList занимает некоторое время - обновление будет делать только по запросу
	//if (!bForce && !gpSet->isStoreTaskbarkTasks && !gpSet->isStoreTaskbarCommands)
	if (!bForce)
		return false; // сохранять не просили

	bool bSucceeded = false;

#ifdef __GNUC__
	MBoxA(L"Sorry, UpdateWin7TaskList is not available in GCC!");
#else
	SetCursor(LoadCursor(NULL, IDC_WAIT));

	LPCWSTR pszTasks[32] = {};
	LPCWSTR pszTasksPrefix[32] = {};
	LPCWSTR pszHistory[32] = {};
	LPCWSTR pszCurCmd = NULL, pszCurCmdTitle = NULL;
	size_t nTasksCount = 0, nHistoryCount = 0;

	if (gpSet->isStoreTaskbarCommands)
	{
		// gpConEmu->mpsz_ConEmuArgs хранит аргументы с "/cmd"
		pszCurCmd = SkipNonPrintable(gpConEmu->mpsz_ConEmuArgs);
		pszCurCmdTitle = pszCurCmd;
		if (pszCurCmdTitle && (*pszCurCmdTitle == L'/'))
		{
			if (StrCmpNI(pszCurCmdTitle, L"/cmd ", 5) == 0)
			{
				pszCurCmdTitle = SkipNonPrintable(pszCurCmdTitle+5);
			}
		}
		if (!pszCurCmdTitle || !*pszCurCmdTitle)
		{
			pszCurCmd = pszCurCmdTitle = NULL;
		}

		// Теперь команды из истории
		LPCWSTR pszCommand;
		while ((pszCommand = gpSet->HistoryGet(nHistoryCount)) && (nHistoryCount < countof(pszHistory)))
		{
			// Текущую - к pszCommand не добавляем. Ее в конец
			if (!pszCurCmdTitle || (lstrcmpi(pszCurCmdTitle, pszCommand) != 0))
			{
				pszHistory[nHistoryCount++] = pszCommand;
			}
			pszCommand += _tcslen(pszCommand)+1;
		}

		if (pszCurCmdTitle)
			nHistoryCount++;
	}

	if (gpSet->isStoreTaskbarkTasks)
	{
		int nGroup = 0;
		const CommandTasks* pGrp = NULL;
		while ((pGrp = gpSet->CmdTaskGet(nGroup++)) && (nTasksCount < countof(pszTasks)))
		{
			if (pGrp->pszName && *pGrp->pszName
				&& !(pGrp->Flags & CETF_NO_TASKBAR))
			{
				pszTasksPrefix[nTasksCount] = pGrp->pszGuiArgs;
				pszTasks[nTasksCount++] = pGrp->pszName;
			}
		}
	}


	//bool lbRc = false;

	// The visible categories are controlled via the ICustomDestinationList interface.  If not customized,
	// applications will get the Recent category by default.
	ICustomDestinationList *pcdl = NULL;
	HRESULT hr = CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_ICustomDestinationList, (void**)&pcdl);
	if (FAILED(hr) || !pcdl)
	{
		DisplayLastError(L"ICustomDestinationList create failed", (DWORD)hr);
	}
	else
	{
		UINT cMinSlots = 0;
		IObjectArray *poaRemoved = NULL;
		hr = pcdl->BeginList(&cMinSlots, IID_PPV_ARGS(&poaRemoved));
		if (FAILED(hr))
		{
			DisplayLastError(L"pcdl->BeginList failed", (DWORD)hr);
		}
		else
		{
			if (cMinSlots < 3)
				cMinSlots = 3;

			// Вся история и все команды - скорее всего в TaskList не поместятся. Нужно подрезать.
			if (cMinSlots < (nTasksCount + nHistoryCount + (pszCurCmdTitle ? 1 : 0)))
			{
				// Минимум одну позицию - оставить под историю/текущую команду
				if (nTasksCount && (cMinSlots < (nTasksCount + 1)))
				{
					nTasksCount = cMinSlots-1;
					if (nTasksCount < countof(pszTasks))
						pszTasks[nTasksCount] = NULL;
				}

				if ((nTasksCount + (pszCurCmdTitle ? 1 : 0)) >= cMinSlots)
					nHistoryCount = 0;
				else
					nHistoryCount = cMinSlots - (nTasksCount + (pszCurCmdTitle ? 1 : 0));

				if (nHistoryCount < countof(pszHistory))
					pszHistory[nHistoryCount] = NULL;
			}

			IObjectCollection *poc = NULL;
			hr = CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&poc));
			if (FAILED(hr) || !poc)
			{
				DisplayLastError(L"IObjectCollection create failed", (DWORD)hr);
			}
			else
			{
				IShellLink * psl = NULL;
				bool bNeedSeparator = false;
				bool bEmpty = true;

				// Если просили - добавляем наши внутренние "Tasks"
				if (SUCCEEDED(hr) && gpSet->isStoreTaskbarkTasks && nTasksCount)
				{
					for (size_t i = 0; (i < countof(pszTasks)) && pszTasks[i]; i++)
					{
						hr = _CreateShellLink(NULL, pszTasksPrefix[i], pszTasks[i], &psl);

						if (SUCCEEDED(hr))
						{
							hr = poc->AddObject(psl);
							psl->Release();
							if (SUCCEEDED(hr))
							{
								bNeedSeparator = true;
								bEmpty = false;
							}
						}

						if (FAILED(hr))
						{
							DisplayLastError(L"Add task (CmdGroup) failed", (DWORD)hr);
							break;
						}
					}
				}

				// И команды из истории
				if (SUCCEEDED(hr) && gpSet->isStoreTaskbarCommands && (nHistoryCount || pszCurCmdTitle))
				{
					if (bNeedSeparator)
					{
						bNeedSeparator = false; // один раз
						hr = _CreateSeparatorLink(&psl);
						if (SUCCEEDED(hr))
						{
							hr = poc->AddObject(psl);
							psl->Release();
							if (SUCCEEDED(hr))
								bEmpty = false;
						}
					}

					if (SUCCEEDED(hr) && pszCurCmdTitle)
					{
						hr = _CreateShellLink(pszCurCmd, NULL, pszCurCmdTitle, &psl);

						if (SUCCEEDED(hr))
						{
							hr = poc->AddObject(psl);
							psl->Release();
							if (SUCCEEDED(hr))
								bEmpty = false;
						}

						if (FAILED(hr))
						{
							DisplayLastError(L"Add task (pszCurCmd) failed", (DWORD)hr);
						}
					}

					for (size_t i = 0; SUCCEEDED(hr) && (i < countof(pszHistory)) && pszHistory[i]; i++)
					{
						hr = _CreateShellLink(NULL, NULL, pszHistory[i], &psl);

						if (SUCCEEDED(hr))
						{
							hr = poc->AddObject(psl);
							psl->Release();
							if (SUCCEEDED(hr))
								bEmpty = false;
						}

						if (FAILED(hr))
						{
							DisplayLastError(L"Add task (pszHistory) failed", (DWORD)hr);
							break;
						}
					}
				}


				if (SUCCEEDED(hr))
				{
					IObjectArray * poa = NULL;
					hr = poc->QueryInterface(IID_PPV_ARGS(&poa));
					if (FAILED(hr) || !poa)
					{
						DisplayLastError(L"poc->QueryInterface(IID_PPV_ARGS(&poa)) failed", (DWORD)hr);
					}
					else
					{
						// Add the tasks to the Jump List. Tasks always appear in the canonical "Tasks"
						// category that is displayed at the bottom of the Jump List, after all other
						// categories.
						hr = bEmpty ? S_OK : pcdl->AddUserTasks(poa);
						if (FAILED(hr))
						{
							DisplayLastError(L"pcdl->AddUserTasks(poa) failed", (DWORD)hr);
						}
						else
						{
							// Commit the list-building transaction.
							hr = pcdl->CommitList();
							if (FAILED(hr))
							{
								DisplayLastError(L"pcdl->CommitList() failed", (DWORD)hr);
							}
							else
							{
								if (!bNoSuccMsg)
								{
									MsgBox(L"Taskbar jump list was updated successfully", MB_ICONINFORMATION, gpConEmu->GetDefaultTitle(), ghOpWnd, true);
								}

								bSucceeded = true;
							}
						}
						poa->Release();
					}
				}
				poc->Release();
			}

			if (poaRemoved)
				poaRemoved->Release();
		}

		pcdl->Release();
	}



	// В Win7 можно также показывать в JumpList "документы" (ярлыки, пути, и т.п.)
	// Но это не то... Похоже, чтобы добавить такой "путь" в Recent/Frequent list
	// нужно создавать физический файл (например, с расширением ".conemu"),
	// и (!) регистрировать для него обработчиком conemu.exe
	#if 0
	//SHAddToRecentDocs(SHARD_PATHW, pszTemp);

	//HRESULT hres;
	//IShellLink* phsl = NULL;
	//// Get a pointer to the IShellLink interface.
	//hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
	//					   IID_IShellLink, (LPVOID*)&phsl);
	//if (SUCCEEDED(hres))
	//{
	//	STARTUPINFO si = {sizeof(si)};
	//	GetStartupInfo(&si);
	//	if (!si.wShowWindow)
	//		si.wShowWindow = SW_SHOWNORMAL;

	//	phsl->SetPath(gpConEmu->ms_ConEmuExe);
	//	phsl->SetDescription(pszTemp);
	//	phsl->SetArguments(pszTemp);
	//	phsl->SetShowCmd(si.wShowWindow);

	//	DWORD n = GetCurrentDirectory(countof(szExe), szExe);
	//	if (n && (n < countof(szExe)))
	//		phsl->SetWorkingDirectory(szExe);
	//}

	//if (phsl)
	//{
	//	//_ASSERTE(SHARD_SHELLITEM == 0x00000008L);
	//	SHAddToRecentDocs(0x00000008L/*SHARD_SHELLITEM*/, phsl);
	//	phsl->Release();
	//}
	#endif

	SetCursor(LoadCursor(NULL, IDC_ARROW));
#endif

	return bSucceeded;
}

// Set our unique application-defined Application User Model ID for Windows 7 TaskBar
// The function also fills the gpConEmu->AppID variable, so it's called regardless of OS ver
HRESULT UpdateAppUserModelID()
{
	bool bSpecialXmlFile = false;
	LPCWSTR pszConfigFile = gpConEmu->ConEmuXml(&bSpecialXmlFile);
	LPCWSTR pszConfigName = gpSetCls->GetConfigName();

	wchar_t szSuffix[64] = L"";

	// Don't change the ID if application was started without arguments changing:
	// ‘config-name’, ‘config-file’, ‘registry-use’, ‘basic-settings’, ‘quake/noquake’
	if (!gpSetCls->isResetBasicSettings
		&& !gpConEmu->opt.ForceUseRegistryPrm
		&& !bSpecialXmlFile
		&& !(pszConfigName && *pszConfigName)
		&& !gpConEmu->opt.QuakeMode.Exists
		)
	{
		if (IsWindows7)
			LogString(L"AppUserModelID was not changed due to special switches absence");
		gpConEmu->SetAppID(szSuffix);
		return S_FALSE;
	}

	// The MSDN says: An application must provide its AppUserModelID in the following form.
	// It can have no more than 128 characters and cannot contain spaces. Each section should be camel-cased.
	//    CompanyName.ProductName.SubProduct.VersionInformation
	// CompanyName and ProductName should always be used, while the SubProduct and VersionInformation portions are optional

	CEStr lsTempBuf;

	if (gpConEmu->opt.QuakeMode.Exists)
	{
		switch (gpConEmu->opt.QuakeMode.GetInt())
		{
		case 1: case 2:
			wcscat_c(szSuffix, L"::Quake"); break;
		default:
			wcscat_c(szSuffix, L"::NoQuake");
		}
	}

	// Config type/file + [.[No]Quake]
	if (gpSetCls->isResetBasicSettings)
	{
		lstrmerge(&lsTempBuf.ms_Val, L"::Basic", szSuffix);
	}
	else if (gpConEmu->opt.ForceUseRegistryPrm)
	{
		lstrmerge(&lsTempBuf.ms_Val, L"::Registry", szSuffix);
	}
	if (bSpecialXmlFile && pszConfigFile && *pszConfigFile)
	{
		lstrmerge(&lsTempBuf.ms_Val, L"::", pszConfigFile, szSuffix);
	}
	else
	{
		lstrmerge(&lsTempBuf.ms_Val, L"::Xml", szSuffix);
	}

	// Named configuration?
	if (!gpSetCls->isResetBasicSettings
		&& (pszConfigName && *pszConfigName))
	{
		lstrmerge(&lsTempBuf.ms_Val, L"::", pszConfigName);
	}

	// Create hash - AppID (will go to mapping)
	gpConEmu->SetAppID(lsTempBuf);

	// Further steps are required in Windows7+ only
	if (!IsWindows7)
	{
		return S_FALSE;
	}

	// Prepare the string
	lsTempBuf.Set(gpConEmu->ms_AppID);
	wchar_t* pszColon = wcschr(lsTempBuf.ms_Val, L':');
	if (pszColon)
	{
		_ASSERTE(pszColon[0]==L':' && pszColon[1]==L':' && isDigit(pszColon[2]) && "::<CESERVER_REQ_VER> is expected at the tail!");
		*pszColon = 0;
	}
	else
	{
		_ASSERTE(pszColon!=NULL && "::<CESERVER_REQ_VER> is expected at the tail!");
	}
	CEStr AppID(APP_MODEL_ID_PREFIX/*L"Maximus5.ConEmu."*/, lsTempBuf.ms_Val);

	// And update it
	HRESULT hr = E_NOTIMPL;
	typedef HRESULT (WINAPI* SetCurrentProcessExplicitAppUserModelID_t)(PCWSTR AppID);
	HMODULE hShell = GetModuleHandle(L"Shell32.dll");
	SetCurrentProcessExplicitAppUserModelID_t fnSetAppUserModelID = hShell
		? (SetCurrentProcessExplicitAppUserModelID_t)GetProcAddress(hShell, "SetCurrentProcessExplicitAppUserModelID")
		: NULL;
	if (fnSetAppUserModelID)
	{
		hr = fnSetAppUserModelID(AppID);
		_ASSERTE(hr == S_OK);
	}

	// Log the change
	wchar_t szLog[200];
	_wsprintf(szLog, SKIPCOUNT(szLog) L"AppUserModelID was changed to `%s` Result=x%08X", AppID.ms_Val, (DWORD)hr);
	LogString(szLog);

	return hr;
}

bool CheckLockFrequentExecute(DWORD& Tick, DWORD Interval)
{
	DWORD CurTick = GetTickCount();
	bool bUnlock = false;
	if ((CurTick - Tick) >= Interval)
	{
		Tick = CurTick;
		bUnlock = true;
	}
	return bUnlock;
}

LONG WINAPI CreateDumpOnException(LPEXCEPTION_POINTERS ExceptionInfo)
{
	wchar_t szFull[1024] = L"";
	wchar_t szDmpFile[MAX_PATH+64] = L"";
	DWORD dwErr = CreateDumpForReport(ExceptionInfo, szFull, szDmpFile);
	wchar_t szAdd[1500];
	wcscpy_c(szAdd, szFull);
	if (szDmpFile[0])
	{
		wcscat_c(szAdd, L"\r\n\r\n" L"Memory dump was saved to\r\n");
		wcscat_c(szAdd, szDmpFile);
		wcscat_c(szAdd, L"\r\n\r\n" L"Please Zip it and send to developer (via DropBox etc.)\r\n");
		wcscat_c(szAdd, CEREPORTCRASH /* https://conemu.github.io/en/Issues.html... */);
	}
	wcscat_c(szAdd, L"\r\n\r\nPress <Yes> to copy this text to clipboard\r\nand open project web page");

	int nBtn = DisplayLastError(szAdd, dwErr ? dwErr : -1, MB_YESNO|MB_ICONSTOP|MB_SYSTEMMODAL);
	if (nBtn == IDYES)
	{
		CopyToClipboard(szFull);
		ConEmuAbout::OnInfo_ReportCrash(NULL);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

void RaiseTestException()
{
	//Removed by optimizer in "Release", need to change...
	//OutputDebugString(L"ConEmu will now raise 'division by 0' exception by user request!\n");
	//int ii = 1, jj = 1; jj --; ii = 1 / jj;

	DebugBreak();
}

// Clear some rubbish in the environment
void ResetEnvironmentVariables()
{
	SetEnvironmentVariable(ENV_CONEMUFAKEDT_VAR_W, NULL);
	SetEnvironmentVariable(ENV_CONEMU_HOOKS_W, NULL);
}

int CheckZoneIdentifiers(bool abAutoUnblock)
{
	if (!gpConEmu)
	{
		_ASSERTE(gpConEmu!=NULL);
		return 0;
	}

	CEStr szZonedFiles;

	LPCWSTR pszDirs[] = {
		gpConEmu->ms_ConEmuExeDir,
		gpConEmu->ms_ConEmuBaseDir,
		NULL};
	LPCWSTR pszFiles[] = {
		L"ConEmu.exe", L"ConEmu64.exe",
		L"ConEmuC.exe", L"ConEmuC64.exe",
		L"ConEmuCD.dll", L"ConEmuCD64.dll",
		L"ConEmuHk.dll", L"ConEmuHk64.dll",
		NULL};

	for (int i = 0; i <= 1; i++)
	{
		if (i && (lstrcmpi(pszDirs[0], pszDirs[1]) == 0))
			break; // ms_ConEmuExeDir & ms_ConEmuBaseDir

		for (int j = 0; pszFiles[j]; j++)
		{
			CEStr lsFile(JoinPath(pszDirs[i], pszFiles[j]));
			int nZone = 0;
			if (HasZoneIdentifier(lsFile, nZone)
				&& (nZone != 0 /*LocalComputer*/))
			{
				lstrmerge(&szZonedFiles.ms_Val, szZonedFiles.ms_Val ? L"\r\n" : NULL, lsFile.ms_Val);
			}
		}
	}

	if (!szZonedFiles.ms_Val)
	{
		return 0; // All files are OK
	}

	CEStr lsMsg(
		L"ConEmu binaries were marked as ‘Downloaded from internet’:\r\n",
		szZonedFiles.ms_Val, L"\r\n\r\n"
		L"This may cause blocking or access denied errors!");

	int iBtn = abAutoUnblock ? IDYES
		: ConfirmDialog(lsMsg, L"Warning!", NULL, NULL, MB_YESNOCANCEL,
			L"Unblock and Continue", L"Let ConEmu try to unblock these files" L"\r\n" L"You may see SmartScreen and UAC confirmations",
			L"Visit home page and Exit", CEZONEID /* https://conemu.github.io/en/ZoneId.html */,
			L"Ignore and Continue", L"You may face further warnings");

	switch (iBtn)
	{
	case IDNO:
		ConEmuAbout::OnInfo_OnlineWiki(L"ZoneId");
		// Exit
		return -1;
	case IDYES:
		break; // Try to unblock
	default:
		// Ignore and continue;
		return 0;
	}

	DWORD nErrCode;
	LPCWSTR pszFrom = szZonedFiles.ms_Val;
	CEStr lsFile;
	bool bFirstRunAs = true;
	while (0 == NextLine(&pszFrom, lsFile))
	{
		if (!DropZoneIdentifier(lsFile, nErrCode))
		{
			if ((nErrCode == ERROR_ACCESS_DENIED)
				&& bFirstRunAs
				&& IsWin6() // UAC available?
				&& !IsUserAdmin()
				)
			{
				bFirstRunAs = false;

				// Let's try to rerun as Administrator
				SHELLEXECUTEINFO sei = {sizeof(sei)};
				sei.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS;
				sei.lpVerb = L"runas";
				sei.lpFile = gpConEmu->ms_ConEmuExe;
				sei.lpParameters = L" -ZoneId -Exit";
				sei.lpDirectory = gpConEmu->ms_ConEmuExeDir;
				sei.nShow = SW_SHOWNORMAL;

				if (ShellExecuteEx(&sei))
				{
					if (!sei.hProcess)
					{
						Sleep(500);
						_ASSERTE(sei.hProcess!=NULL);
					}
					if (sei.hProcess)
					{
						WaitForSingleObject(sei.hProcess, INFINITE);
					}

					int nZone = 0;
					if (!HasZoneIdentifier(lsFile, nZone)
						|| (nZone != 0 /*LocalComputer*/))
					{
						// Assuming that elevated copy has fixed all zone problems
						break;
					}
				}
			}

			lsMsg = lstrmerge(L"Failed to drop ZoneId in file:\r\n", lsFile, L"\r\n\r\n" L"Ignore error and continue?" L"\r\n");
			if (DisplayLastError(lsMsg, nErrCode, MB_ICONSTOP|MB_YESNO) != IDYES)
			{
				return -1; // Fails to change
			}
		}
	}

	return 0;
}

// 0 - Succeeded, otherwise - exit code
// isScript - several tabs or splits were requested via "-cmdlist ..."
// isBare - true if there was no switches, for example "ConEmu.exe c:\tools\far.exe". That is not correct command line actually
int ProcessCmdArg(LPCWSTR cmdNew, bool isScript, bool isBare, CEStr& szReady, bool& rbSaveHistory)
{
	rbSaveHistory = false;

	if (!cmdNew || !*cmdNew)
	{
		return 0; // Nothing to do
	}

	// Command line was specified by "-cmd ..." or "-cmdlist ..."
	DEBUGSTRSTARTUP(L"Preparing command line");

	MCHKHEAP
	const wchar_t* pszDefCmd = NULL;

	if (isScript)
	{
		szReady.Set(cmdNew);
		if (szReady.IsEmpty())
		{
			MBoxAssert(FALSE && "Memory allocation failed");
			return 100;
		}
	}
	else
	{
		int nLen = _tcslen(cmdNew)+8;

		// For example "ConEmu.exe c:\tools\far.exe"
		// That is not 'proper' command actually, but we may support this by courtesy
		if (isBare /*(params == (uint)-1)*/
			&& (gpSet->nStartType == 0)
			&& (gpSet->psStartSingleApp && *gpSet->psStartSingleApp))
		{
			// psStartSingleApp may have path to "far.exe" defined...
			// Then if user drops, for example, txt file on the ConEmu's icon,
			// we may concatenate this argument with Far command line.
			pszDefCmd = gpSet->psStartSingleApp;
			CEStr szExe;
			if (0 != NextArg(&pszDefCmd, szExe))
			{
				_ASSERTE(FALSE && "NextArg failed");
			}
			else
			{
				// Только если szExe это Far.
				if (IsFarExe(szExe))
					pszDefCmd = gpSet->psStartSingleApp;
				else
					pszDefCmd = NULL; // Запускать будем только то, что "набросили"
			}

			if (pszDefCmd)
			{
				nLen += 3 + _tcslen(pszDefCmd);
			}
		}

		wchar_t* pszReady = szReady.GetBuffer(nLen+1);
		if (!pszReady)
		{
			MBoxAssert(FALSE && "Memory allocation failed");
			return 100;
		}


		if (pszDefCmd)
		{
			lstrcpy(pszReady, pszDefCmd);
			lstrcat(pszReady, L" ");
			lstrcat(pszReady, SkipNonPrintable(cmdNew));

			if (pszReady[0] != L'/' && pszReady[0] != L'-')
			{
				// gpSet->HistoryAdd(pszReady);
				rbSaveHistory = true;
			}
		}
		// There was no switches
		else if (isBare)
		{
			*pszReady = DropLnkPrefix; // The sign we probably got command line by dropping smth on ConEmu's icon
			lstrcpy(pszReady+1, SkipNonPrintable(cmdNew));

			if (pszReady[1] != L'/' && pszReady[1] != L'-')
			{
				// gpSet->HistoryAdd(pszReady+1);
				rbSaveHistory = true;
			}
		}
		else
		{
			lstrcpy(pszReady, SkipNonPrintable(cmdNew));

			if (pszReady[0] != L'/' && pszReady[0] != L'-')
			{
				// gpSet->HistoryAdd(pszReady);
				rbSaveHistory = true;
			}
		}
	}

	MCHKHEAP

	// Store it
	gpConEmu->SetCurCmd(szReady, isScript);

	return 0;
}

// -debug, -debugi, -debugw
int CheckForDebugArgs(LPCWSTR asCmdLine)
{
	if (IsDebuggerPresent())
		return 0;

	BOOL nDbg = FALSE;
	bool debug = false;  // Just show a MessageBox with command line and PID
	bool debugw = false; // Silently wait until debugger is connected
	bool debugi = false; // _ASSERT(FALSE)
	UINT iSleep = 0;

	#if defined(SHOW_STARTED_MSGBOX)
	debugm = true;
	#elif defined(WAIT_STARTED_DEBUGGER)
	debugw = true;
	#endif

	LPCWSTR pszCmd = asCmdLine;
	CEStr lsArg;
	// First argument (actually, first would be our executable in most cases)
	for (int i = 0; i <= 1; i++)
	{
		if (NextArg(&pszCmd, lsArg) != 0)
			break;
		// Support both notations
		if (lsArg.ms_Val[0] == L'/') lsArg.ms_Val[0] = L'-';

		if (lstrcmpi(lsArg, L"-debug") == 0)
		{
			debug = true; break;
		}
		if (lstrcmpi(lsArg, L"-debugi") == 0)
		{
			debugi = true; break;
		}
		if (lstrcmpi(lsArg, L"-debugw") == 0)
		{
			debugw = true; break;
		}
	}

	if (debug)
	{
		wchar_t szTitle[128]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"Conemu started, PID=%i", GetCurrentProcessId());
		CEStr lsText(L"GetCommandLineW()\n", GetCommandLineW(), L"\n\n\n" L"lpCmdLine\n", asCmdLine);
		MessageBox(NULL, lsText, szTitle, MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND|MB_SYSTEMMODAL);
		nDbg = IsDebuggerPresent();
	}
	else if (debugw)
	{
		while (!IsDebuggerPresent())
			Sleep(250);
		nDbg = IsDebuggerPresent();
	}
	else if (debugi)
	{
		#ifdef _DEBUG
		if (!IsDebuggerPresent()) _ASSERT(FALSE);
		#endif
		nDbg = IsDebuggerPresent();
	}

	// To be able to do some actions (focus window?) before continue
	if (nDbg)
	{
		iSleep = 5000;
		Sleep(iSleep);
	}

	return nDbg;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	DEBUGSTRSTARTUP(L"WinMain entered");
	int iMainRc = 0;

	g_hInstance = hInstance;
	ghWorkingModule = (u64)hInstance;

#ifdef _DEBUG
	gbAllowChkHeap = true;
#endif

	if (!IsDebuggerPresent())
	{
		SetUnhandledExceptionFilter(CreateDumpOnException);
	}

	_ASSERTE(sizeof(CESERVER_REQ_STARTSTOPRET) <= sizeof(CESERVER_REQ_STARTSTOP));
	ZeroStruct(gOSVer);
	gOSVer.dwOSVersionInfoSize = sizeof(gOSVer);
	GetOsVersionInformational(&gOSVer);
	gnOsVer = ((gOSVer.dwMajorVersion & 0xFF) << 8) | (gOSVer.dwMinorVersion & 0xFF);
	HeapInitialize();
	AssertMsgBox = MsgBox;
	gfnHooksUnlockerProc = HooksUnlockerProc;
	gn_MainThreadId = GetCurrentThreadId();
	gfnSearchAppPaths = SearchAppPaths;

	#ifdef _DEBUG
	HMODULE hConEmuHk = GetModuleHandle(WIN3264TEST(L"ConEmuHk.dll",L"ConEmuHk64.dll"));
	_ASSERTE(hConEmuHk==NULL && "Hooks must not be loaded into ConEmu[64].exe!");
	#endif

	// On Vista and higher ensure our process will be
	// marked as fully dpi-aware, regardless of manifest
	if (gnOsVer >= 0x600)
	{
		CDpiAware::setProcessDPIAwareness();
	}

	/* *** DEBUG PURPOSES */
	gpStartEnv = LoadStartupEnvEx::Create();
	if (gnOsVer >= 0x600)
	{
		CDpiAware::UpdateStartupInfo(gpStartEnv);
	}
	/* *** DEBUG PURPOSES */

	gbIsWine = IsWine(); // В общем случае, на флажок ориентироваться нельзя. Это для информации.
	if (gbIsWine)
	{
		wcscpy_c(gsDefGuiFont, L"Liberation Mono");
	}
	else if (IsWindowsVista)
	{
		// Vista+ and ClearType? May be "Consolas" font need to be default in Console.
		BOOL bClearType = FALSE;
		if (SystemParametersInfo(SPI_GETCLEARTYPE, 0, &bClearType, 0) && bClearType)
		{
			wcscpy_c(gsDefGuiFont, L"Consolas");
		}
		// Default UI?
		wcscpy_c(gsDefMUIFont, L"Segoe UI");
	}


	gbIsDBCS = IsDbcs();
	if (gbIsDBCS)
	{
		HKEY hk = NULL;
		DWORD nOemCP = GetOEMCP();
		DWORD nRights = KEY_READ|WIN3264TEST((IsWindows64() ? KEY_WOW64_64KEY : 0),0);
		if (nOemCP && !RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont", 0, nRights, &hk))
		{
			wchar_t szName[64]; _wsprintf(szName, SKIPLEN(countof(szName)) L"%u", nOemCP);
			wchar_t szVal[64] = {}; DWORD cbSize = sizeof(szVal)-2;
			if (!RegQueryValueEx(hk, szName, NULL, NULL, (LPBYTE)szVal, &cbSize) && *szVal)
			{
				if (wcschr(szVal, L'?'))
				{
					// logging was not started yet... so we cant write to the log
					// Just leave default 'Lucida Console'
					// LogString(L"Invalid console font was registered in registry");
				}
				else
				{
					lstrcpyn(gsDefConFont, (*szVal == L'*') ? (szVal+1) : szVal, countof(gsDefConFont));
				}
			}
			RegCloseKey(hk);
		}
	}

	ResetEnvironmentVariables();

	DEBUGSTRSTARTUP(L"Environment checked");

	gpSetCls = new CSettings;
	gpConEmu = new CConEmuMain;
	CLngRc::Initialize();
	gVConDcMap.Init(MAX_CONSOLE_COUNT,true);
	gVConBkMap.Init(MAX_CONSOLE_COUNT,true);
	/*int nCmp;
	nCmp = StrCmpI(L" ", L"A"); // -1
	nCmp = StrCmpI(L" ", L"+");
	nCmp = StrCmpI(L" ", L"-");
	nCmp = wcscmp(L" ", L"-");
	nCmp = wcsicmp(L" ", L"-");
	nCmp = lstrcmp(L" ", L"-");
	nCmp = lstrcmpi(L" ", L"-");
	nCmp = StrCmpI(L" ", L"\\");*/
	gpLocalSecurity = LocalSecurity();

	#ifdef _DEBUG
	gAllowAssertThread = am_Thread;
	//wchar_t szDbg[64];
	//msprintf(szDbg, countof(szDbg), L"xx=0x%X.", 0);
	#endif

	#if defined(_DEBUG)
	DebugUnitTests();
	#endif
	#if defined(__GNUC__) || defined(_DEBUG)
	GnuUnitTests();
	#endif


	// If possible, open our windows on the monitor where user have clicked
	// our icon (shortcut on the desktop or TaskBar)
	gpStartEnv->hStartMon = GetStartupMonitor();

//#ifdef _DEBUG
//	wchar_t* pszShort = GetShortFileNameEx(L"T:\\VCProject\\FarPlugin\\ConEmu\\Maximus5\\Debug\\Far2x86\\ConEmu\\ConEmu.exe");
//	if (pszShort) free(pszShort);
//	pszShort = GetShortFileNameEx(L"\\\\MAX\\X-change\\GoogleDesktopEnterprise\\AdminGuide.pdf");
//	if (pszShort) free(pszShort);
//
//
//	DWORD ImageSubsystem, ImageBits;
//	GetImageSubsystem(ImageSubsystem,ImageBits);
//
//	//wchar_t szConEmuBaseDir[MAX_PATH+1], szConEmuExe[MAX_PATH+1];
//	//BOOL lbDbgFind = FindConEmuBaseDir(szConEmuBaseDir, szConEmuExe);
//
//	wchar_t szDebug[1024] = {};
//	msprintf(szDebug, countof(szDebug), L"Test %u %i %s 0x%X %c 0x%08X*",
//		987654321, -1234, L"abcdef", 0xAB1298, L'Z', 0xAB1298);
//	msprintf(szDebug, countof(szDebug), L"<%c%c>%u.%s",
//		'я', (wchar_t)0x44F, 0x44F, L"End");
//#endif

	// lpCmdLine is not a UNICODE string, that's why we have to use GetCommandLineW()
	// However, cygwin breaks normal way of creating Windows' processes,
	// and GetCommandLineW will be useless in cygwin's builds (returns only exe full path)
	CEStr lsCvtCmdLine;
	if (lpCmdLine && *lpCmdLine)
	{
		int iLen = lstrlenA(lpCmdLine);
		MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, lsCvtCmdLine.GetBuffer(iLen), iLen+1);
	}
	// Prepared command line
	CEStr lsCommandLine;
	#if !defined(__CYGWIN__)
	lsCommandLine.Set(GetCommandLineW());
	#else
	lsCommandLine.Set(lsCvtCmdLine.ms_Val);
	#endif
	if (lsCommandLine.IsEmpty())
	{
		lsCommandLine.Set(L"");
	}

	// -debug, -debugi, -debugw
	CheckForDebugArgs(lsCommandLine);

#ifdef DEBUG_MSG_HOOKS
	ghDbgHook = SetWindowsHookEx(WH_CALLWNDPROC, DbgCallWndProc, NULL, GetCurrentThreadId());
#endif

	_ASSERTE(gpSetCls->SingleInstanceArg == sgl_Default);
	gpSetCls->SingleInstanceArg = sgl_Default;
	gpSetCls->SingleInstanceShowHide = sih_None;

	CEStr szReady;
	bool  ReqSaveHistory = false;

	//gpConEmu->cBlinkShift = GetCaretBlinkTime()/15;
	//memset(&gOSVer, 0, sizeof(gOSVer));
	//gOSVer.dwOSVersionInfoSize = sizeof(gOSVer);
	//GetVersionEx(&gOSVer);
	//DisableIME();
	//Windows7 - SetParent для консоли валится
	//gpConEmu->setParent = false; // PictureView теперь идет через Wrapper
	//if ((gOSVer.dwMajorVersion>6) || (gOSVer.dwMajorVersion==6 && gOSVer.dwMinorVersion>=1))
	//{
	//	setParentDisabled = true;
	//}
	//if (gOSVer.dwMajorVersion>=6)
	//{
	//	CheckConIme();
	//}
	//gpSet->InitSettings();

	int iParseRc = 0;
	if (!gpConEmu->ParseCommandLine(lsCommandLine, iParseRc))
	{
		return iParseRc;
	}

	/* ******************************** */
	int iZoneCheck = CheckZoneIdentifiers(gpConEmu->opt.FixZoneId.GetBool());
	if (iZoneCheck < 0)
	{
		return CERR_ZONE_CHECK_ERROR;
	}
	if (gpConEmu->opt.FixZoneId.GetBool() && gpConEmu->opt.ExitAfterActionPrm.GetBool())
	{
		_ASSERTE(gpConEmu->opt.cmdNew.IsEmpty());
		return 0;
	}
	/* ******************************** */

//------------------------------------------------------------------------
///| load settings and apply parameters |/////////////////////////////////
//------------------------------------------------------------------------

	// set config name before settings (i.e. where to load from)
	if (gpConEmu->opt.ConfigVal.Exists)
	{
		DEBUGSTRSTARTUP(L"Initializing configuration name");
		gpSetCls->SetConfigName(gpConEmu->opt.ConfigVal.GetStr());
	}

	// xml-using disabled? Forced to registry?
	if (gpConEmu->opt.ForceUseRegistryPrm)
	{
		gpConEmu->SetForceUseRegistry();
	}
	// special config file
	else if (gpConEmu->opt.LoadCfgFile.Exists)
	{
		DEBUGSTRSTARTUP(L"Exact cfg file was specified");
		// При ошибке - не выходим, просто покажем ее пользователю
		gpConEmu->SetConfigFile(gpConEmu->opt.LoadCfgFile.GetStr(), false/*abWriteReq*/, true/*abSpecialPath*/);
	}

	//------------------------------------------------------------------------
	///| Set our own AppUserModelID for Win7 TaskBar |////////////////////////
	///| Call it always to store in gpConEmu->AppID  |////////////////////////
	//------------------------------------------------------------------------
	DEBUGSTRSTARTUP(L"UpdateAppUserModelID");
	UpdateAppUserModelID();

	//------------------------------------------------------------------------
	///| Set up ‘First instance’ event |//////////////////////////////////////
	//------------------------------------------------------------------------
	DEBUGSTRSTARTUP(L"Checking for first instance");
	gpConEmu->isFirstInstance();

	//------------------------------------------------------------------------
	///| Preparing settings |/////////////////////////////////////////////////
	//------------------------------------------------------------------------
	bool bNeedCreateVanilla = false;
	SettingsLoadedFlags slfFlags = slf_None;
	if (gpConEmu->opt.ResetSettings)
	{
		// force this config as "new"
		DEBUGSTRSTARTUP(L"Clear config was requested");
		gpSet->IsConfigNew = true;
		gpSet->InitVanilla();
	}
	else
	{
		// load settings from registry
		DEBUGSTRSTARTUP(L"Loading config from settings storage");
		gpSet->LoadSettings(bNeedCreateVanilla);
	}

	// Quake/NoQuake?
	if (gpConEmu->opt.QuakeMode.Exists)
	{
		gpConEmu->SetQuakeMode(gpConEmu->opt.QuakeMode.GetInt());
		gpSet->isRestore2ActiveMon = true;
	}

	// Update package was dropped on ConEmu icon?
	// params == (uint)-1, если первый аргумент не начинается с '/'
	if (gpConEmu->opt.cmdNew && *gpConEmu->opt.cmdNew && (gpConEmu->opt.params == -1))
	{
		CEStr szPath;
		LPCWSTR pszCmdLine = gpConEmu->opt.cmdNew;
		if (0 == NextArg(&pszCmdLine, szPath))
		{
			if (CConEmuUpdate::IsUpdatePackage(szPath))
			{
				DEBUGSTRSTARTUP(L"Update package was dropped on ConEmu, updating");

				// Чтобы при запуске НОВОЙ версии опять не пошло обновление - грохнуть ком-строку
				SafeFree(gpConEmu->mpsz_ConEmuArgs);

				// Создание скрипта обновления, запуск будет выполнен в деструкторе gpUpd
				CConEmuUpdate::LocalUpdate(szPath);

				// Перейти к завершению процесса и запуску обновления
				goto done;
			}
		}
	}

	// Store command line in our class variables to be able show it in "Fast Configuration" dialog
	if (gpConEmu->opt.cmdNew)
	{
		int iArgRc = ProcessCmdArg(gpConEmu->opt.cmdNew, gpConEmu->opt.isScript, (gpConEmu->opt.params == -1), szReady, ReqSaveHistory);
		if (iArgRc != 0)
		{
			return iArgRc;
		}
	}

	// Settings are loaded, fixup
	slfFlags |= slf_OnStartupLoad | slf_AllowFastConfig
		| (bNeedCreateVanilla ? slf_NeedCreateVanilla : slf_None)
		| (gpSet->IsConfigPartial ? slf_DefaultTasks : slf_None)
		| ((gpConEmu->opt.ResetSettings || gpSet->IsConfigNew) ? slf_DefaultSettings : slf_None);
	// выполнить дополнительные действия в классе настроек здесь
	DEBUGSTRSTARTUP(L"Config loaded, post checks");
	gpSetCls->SettingsLoaded(slfFlags, gpConEmu->opt.cmdNew);

	// Для gpSet->isQuakeStyle - принудительно включается gpSetCls->SingleInstanceArg

	// When "/Palette <name>" is specified
	if (gpConEmu->opt.PaletteVal.Exists)
	{
		gpSet->PaletteSetActive(gpConEmu->opt.PaletteVal.GetStr());
	}

	// Set another update location (-UpdateSrcSet <URL>)
	if (gpConEmu->opt.UpdateSrcSet.Exists)
	{
		gpSet->UpdSet.SetUpdateVerLocation(gpConEmu->opt.UpdateSrcSet.GetStr());
	}

	// Force "AnsiLog" feature
	if (gpConEmu->opt.AnsiLogPath.GetStr())
	{
		gpSet->isAnsiLog = true;
		SafeFree(gpSet->pszAnsiLog);
		gpSet->pszAnsiLog = lstrdup(gpConEmu->opt.AnsiLogPath.GetStr());
	}

	// Forced window size or pos
	// Call this AFTER SettingsLoaded because we (may be)
	// don't want to change ‘xml-stored’ values
	if (gpConEmu->opt.SizePosPrm)
	{
		if (gpConEmu->opt.sWndX.Exists)
			gpConEmu->SetWindowPosSizeParam(L'X', gpConEmu->opt.sWndX.GetStr());
		if (gpConEmu->opt.sWndY.Exists)
			gpConEmu->SetWindowPosSizeParam(L'Y', gpConEmu->opt.sWndY.GetStr());
		if (gpConEmu->opt.sWndW.Exists)
			gpConEmu->SetWindowPosSizeParam(L'W', gpConEmu->opt.sWndW.GetStr());
		if (gpConEmu->opt.sWndH.Exists)
			gpConEmu->SetWindowPosSizeParam(L'H', gpConEmu->opt.sWndH.GetStr());
	}

	DEBUGSTRSTARTUPLOG(L"SettingsLoaded");


//------------------------------------------------------------------------
///| Processing actions |/////////////////////////////////////////////////
//------------------------------------------------------------------------
	// gpConEmu->mb_UpdateJumpListOnStartup - Обновить JumpLists
	// SaveCfgFilePrm, SaveCfgFile - сохранить настройки в xml-файл (можно использовать вместе с ResetSettings)
	// ExitAfterActionPrm - сразу выйти после выполнения действий
	// не наколоться бы с isAutoSaveSizePos

	if (gpConEmu->mb_UpdateJumpListOnStartup && gpConEmu->opt.ExitAfterActionPrm)
	{
		DEBUGSTRSTARTUP(L"Updating Win7 task list");
		if (!UpdateWin7TaskList(true/*bForce*/, true/*bNoSuccMsg*/))
		{
			if (!iMainRc) iMainRc = 10;
		}
	}

	// special config file
	if (gpConEmu->opt.SaveCfgFile.Exists)
	{
		// Сохранять конфиг только если получилось сменить путь (создать файл)
		DEBUGSTRSTARTUP(L"Force write current config to settings storage");
		if (!gpConEmu->SetConfigFile(gpConEmu->opt.SaveCfgFile.GetStr(), true/*abWriteReq*/, true/*abSpecialPath*/))
		{
			if (!iMainRc) iMainRc = 11;
		}
		else
		{
			if (!gpSet->SaveSettings())
			{
				if (!iMainRc) iMainRc = 12;
			}
		}
	}

	// Only when ExitAfterActionPrm, otherwise - it will be called from ConEmu's PostCreate
	if (gpConEmu->opt.SetUpDefaultTerminal)
	{
		_ASSERTE(!gpConEmu->DisableSetDefTerm);

		gpSet->isSetDefaultTerminal = true;
		gpSet->isRegisterOnOsStartup = true;

		if (gpConEmu->opt.ExitAfterActionPrm)
		{
			if (gpConEmu->mp_DefTrm)
			{
				// Update registry with ‘DefTerm-...’ settings
				gpConEmu->mp_DefTrm->ApplyAndSave(true, true);
				// Hook all required processes and exit
				gpConEmu->mp_DefTrm->StartGuiDefTerm(true, true);
			}
			else
			{
				_ASSERTE(gpConEmu->mp_DefTrm);
			}

			// Exit now
			gpSetCls->ibDisableSaveSettingsOnExit = true;
		}
	}

	// Actions done
	if (gpConEmu->opt.ExitAfterActionPrm)
	{
		DEBUGSTRSTARTUP(L"Exit was requested");
		goto wrap;
	}

	if (gpConEmu->opt.ExecGuiMacro.GetStr())
	{
		gpConEmu->SetPostGuiMacro(gpConEmu->opt.ExecGuiMacro.GetStr());
	}

//------------------------------------------------------------------------
///| Continue normal work mode  |/////////////////////////////////////////
//------------------------------------------------------------------------

	// Если в режиме "Inside" подходящего окна не нашли и юзер отказался от "обычного" режима
	// mh_InsideParentWND инициализируется вызовом InsideFindParent из Settings::LoadSettings()
	if (gpConEmu->mp_Inside && (gpConEmu->mp_Inside->mh_InsideParentWND == INSIDE_PARENT_NOT_FOUND))
	{
		DEBUGSTRSTARTUP(L"Bad InsideParentHWND, exiting");
		return 100;
	}


	// Проверить наличие необходимых файлов (перенес сверху, чтобы учитывался флажок "Inject ConEmuHk")
	if (!gpConEmu->CheckRequiredFiles())
	{
		DEBUGSTRSTARTUP(L"Required files were not found, exiting");
		return 100;
	}


	//#pragma message("Win2k: CLEARTYPE_NATURAL_QUALITY")
	//if (ClearTypePrm)
	//	gpSet->LogFont.lfQuality = CLEARTYPE_NATURAL_QUALITY;
	//if (FontPrm)
	//	_tcscpy(gpSet->LogFont.lfFaceName, FontVal);
	//if (SizePrm)
	//	gpSet->LogFont.lfHeight = SizeVal;
	if (gpConEmu->opt.BufferHeightVal.Exists)
	{
		gpSetCls->SetArgBufferHeight(gpConEmu->opt.BufferHeightVal.GetInt());
	}

	if (!gpConEmu->opt.WindowModeVal.Exists)
	{
		if (nCmdShow == SW_SHOWMAXIMIZED)
			gpSet->_WindowMode = wmMaximized;
		else if (nCmdShow == SW_SHOWMINIMIZED || nCmdShow == SW_SHOWMINNOACTIVE)
			gpConEmu->WindowStartMinimized = true;
	}
	else
	{
		gpSet->_WindowMode = (ConEmuWindowMode)gpConEmu->opt.WindowModeVal.GetInt();
	}

	if (gpConEmu->opt.MultiConValue.Exists)
		gpSet->mb_isMulti = gpConEmu->opt.MultiConValue;

	if (gpConEmu->opt.VisValue.Exists)
		gpSet->isConVisible = gpConEmu->opt.VisValue;

	// Если запускается conman (нафига?) - принудительно включить флажок "Обновлять handle"
	//TODO("Deprecated: isUpdConHandle использоваться не должен");

	if (gpSetCls->IsMulti() || StrStrI(gpConEmu->GetCmd(), L"conman.exe"))
	{
		//gpSet->isUpdConHandle = TRUE;

		// сбросить CreateInNewEnvironment для ConMan
		gpConEmu->ResetConman();
	}

	// Need to add to the history?
	if (ReqSaveHistory && !szReady.IsEmpty())
	{
		LPCWSTR pszCommand = szReady;
		if (*pszCommand == DropLnkPrefix)
			pszCommand++;
		gpSet->HistoryAdd(pszCommand);
	}

	//if (FontFilePrm) {
	//	if (!AddFontResourceEx(FontFile, FR_PRIVATE, NULL)) //ADD fontname; by Mors
	//	{
	//		TCHAR* psz=(TCHAR*)calloc(_tcslen(FontFile)+100,sizeof(TCHAR));
	//		lstrcpyW(psz, L"Can't register font:\n");
	//		lstrcatW(psz, FontFile);
	//		MessageBox(NULL, psz, gpConEmu->GetDefaultTitle(), MB_OK|MB_ICONSTOP);
	//		free(psz);
	//		return 100;
	//	}
	//	lstrcpynW(gpSet->FontFile, FontFile, countof(gpSet->FontFile));
	//}
	// else if (gpSet->isSearchForFont && gpConEmu->ms_ConEmuExe[0]) {
	//	if (FindFontInFolder(szTempFontFam)) {
	//		// Шрифт уже зарегистрирован
	//		FontFilePrm = true;
	//		FontPrm = true;
	//		FontVal = szTempFontFam;
	//		FontFile = gpSet->FontFile;
	//	}
	//}

	DEBUGSTRSTARTUPLOG(L"Registering local fonts");

	gpSetCls->RegisterFonts();
	gpSetCls->InitFont(
		gpConEmu->opt.FontVal.GetStr(),
		gpConEmu->opt.SizeVal.Exists ? gpConEmu->opt.SizeVal.GetInt() : -1,
		gpConEmu->opt.ClearTypeVal.Exists ? gpConEmu->opt.ClearTypeVal.GetInt() : -1
	);

	if (gpSet->wndCascade)
	{
		gpConEmu->CascadedPosFix();
	}

///////////////////////////////////

	// Нет смысла проверять и искать, если наш экземпляр - первый.
	if (gpSetCls->IsSingleInstanceArg() && !gpConEmu->isFirstInstance())
	{
		DEBUGSTRSTARTUPLOG(L"Checking for existing instance");

		HWND hConEmuHwnd = FindWindowExW(NULL, NULL, VirtualConsoleClassMain, NULL);
		// При запуске серии закладок из cmd файла второму экземпляру лучше чуть-чуть подождать
		// чтобы успело "появиться" главное окно ConEmu
		if ((hConEmuHwnd == NULL) && (gpSetCls->SingleInstanceShowHide == sih_None))
		{
			// Если окна нет, и других процессов (ConEmu.exe, ConEmu64.exe) нет
			// то ждать смысла нет
			bool bOtherExists = false;

			gpConEmu->LogString(L"TH32CS_SNAPPROCESS");

			HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (h && (h != INVALID_HANDLE_VALUE))
			{
				PROCESSENTRY32 PI = {sizeof(PI)};
				DWORD nSelfPID = GetCurrentProcessId();
				if (Process32First(h, &PI))
				{
					do {
						if (PI.th32ProcessID != nSelfPID)
						{
							LPCWSTR pszName = PointToName(PI.szExeFile);
							if (pszName
								&& ((lstrcmpi(pszName, L"ConEmu.exe") == 0)
									|| (lstrcmpi(pszName, L"ConEmu64.exe") == 0)))
							{
								bOtherExists = true;
								break;
							}
						}
					} while (Process32Next(h, &PI));
				}

				CloseHandle(h);
			}

			// Ждать имеет смысл только если есть другие процессы "ConEmu.exe"/"ConEmu64.exe"
			if (bOtherExists)
			{
				Sleep(1000); // чтобы успело "появиться" главное окно ConEmu
			}
		}

		gpConEmu->LogString(L"isFirstInstance");

		// Поехали
		DWORD dwStart = GetTickCount();

		while (!gpConEmu->isFirstInstance())
		{
			DEBUGSTRSTARTUPLOG(L"Waiting for RunSingleInstance");

			int iRunRC = gpConEmu->RunSingleInstance();
			if (iRunRC > 0)
			{
				gpConEmu->LogString(L"Passed to first instance, exiting");
				return 0;
			}
			else if (iRunRC < 0)
			{
				gpConEmu->LogString(L"Reusing running instance is not allowed here");
				break;
			}

			// Если передать не удалось (может первый экземпляр еще в процессе инициализации?)
			Sleep(250);

			// Если ожидание длится более 10 секунд - запускаемся самостоятельно
			if ((GetTickCount() - dwStart) > 10*1000)
				break;
		}

		DEBUGSTRSTARTUPLOG(L"Existing instance was terminated, continue as first instance");
	}

//------------------------------------------------------------------------
///| Allocating console |/////////////////////////////////////////////////
//------------------------------------------------------------------------

#if 0
	//120714 - аналогичные параметры работают в ConEmuC.exe, а в GUI они и не работали. убрал пока
	if (AttachPrm)
	{
		if (!AttachVal)
		{
			MBoxA(_T("Invalid <process id> specified in the /Attach argument"));
			//delete pVCon;
			return 100;
		}

		gpSetCls->nAttachPID = AttachVal;
	}
#endif

//------------------------------------------------------------------------
///| Initializing |///////////////////////////////////////////////////////
//------------------------------------------------------------------------

	DEBUGSTRSTARTUPLOG(L"gpConEmu->Init");

	// Тут загружаются иконки, Affinity, SetCurrentDirectory и т.п.
	if (!gpConEmu->Init())
	{
		return 100;
	}

//------------------------------------------------------------------------
///| Create taskbar window |//////////////////////////////////////////////
//------------------------------------------------------------------------

	// Тут создается окошко чтобы не показывать кнопку на таскбаре
	if (!CheckCreateAppWindow())
	{
		return 100;
	}

//------------------------------------------------------------------------
///| Creating window |////////////////////////////////////////////////////
//------------------------------------------------------------------------

	DEBUGSTRSTARTUPLOG(L"gpConEmu->CreateMainWindow");

	if (!gpConEmu->CreateMainWindow())
	{
		return 100;
	}

//------------------------------------------------------------------------
///| Misc |///////////////////////////////////////////////////////////////
//------------------------------------------------------------------------
	DEBUGSTRSTARTUP(L"gpConEmu->PostCreate");
	gpConEmu->PostCreate();
//------------------------------------------------------------------------
///| Main message loop |//////////////////////////////////////////////////
//------------------------------------------------------------------------
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	MessageLoop();

	iMainRc = gpConEmu->mn_ShellExitCode;
	switch (iMainRc)
	{
	case STATUS_CONTROL_C_EXIT:
		DEBUGSTRSTARTUP(L"Shell has reported close by Ctrl+C. ConEmu is going to exit with code 0xC000013A."); break;
	case STILL_ACTIVE:
		DEBUGSTRSTARTUP(L"Shell has not reported its exit code. Abnormal termination or detached window? ConEmu is going to exit with code 259."); break;
	case 0:
		break;
	default:
		DEBUGSTRSTARTUP(L"ConEmu is going to exit with shell's exit code.");
	}

done:
	DEBUGSTRSTARTUP(L"Terminating");
	ShutdownGuiStep(L"MessageLoop terminated");
//------------------------------------------------------------------------
///| Deinitialization |///////////////////////////////////////////////////
//------------------------------------------------------------------------
	//KillTimer(ghWnd, 0);
	//delete pVCon;
	//CloseHandle(hChildProcess); -- он более не требуется
	//if (FontFilePrm) RemoveFontResourceEx(FontFile, FR_PRIVATE, NULL); //ADD fontname; by Mors
	gpSetCls->UnregisterFonts();

	//CoUninitialize();
	OleUninitialize();

	if (gpConEmu)
	{
		delete gpConEmu;
		gpConEmu = NULL;
	}

	if (gpSetCls)
	{
		delete gpSetCls;
		gpSetCls = NULL;
	}

	if (gpUpd)
	{
		delete gpUpd;
		gpUpd = NULL;
	}

	if (gpLng)
	{
		delete gpLng;
		gpLng = NULL;
	}

	ShutdownGuiStep(L"Gui terminated");

wrap:
	// HeapDeinitialize() - Нельзя. Еще живут глобальные объекты
	DEBUGSTRSTARTUP(L"WinMain exit");
	// If TerminateThread was called at least once,
	// normal process shutdown may hangs
	if (wasTerminateThreadCalled())
	{
		TerminateProcess(GetCurrentProcess(), iMainRc);
	}
	return iMainRc;
}
