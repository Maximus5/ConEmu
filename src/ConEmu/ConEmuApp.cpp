﻿
/*
Copyright (c) 2009-2015 Maximus5
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
#include <dbghelp.h>
#include <shobjidl.h>
#include <propkey.h>
#include <taskschd.h>
#else
#include "../common/DbgHlpGcc.h"
#endif
#include "../common/ConEmuCheck.h"
#include "../common/WFiles.h"
#include "AboutDlg.h"
#include "Options.h"
#include "OptionsClass.h"
#include "ConEmu.h"
#include "DpiAware.h"
#include "HooksUnlocker.h"
#include "Inside.h"
#include "TaskBar.h"
#include "DwmHelper.h"
#include "ConEmuApp.h"
#include "Update.h"
#include "SetCmdTask.h"
#include "Recreate.h"
#include "DefaultTerm.h"
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

#define DEBUGSTRMOVE(s) //DEBUGSTR(s)
#define DEBUGSTRTIMER(s) //DEBUGSTR(s)
#define DEBUGSTRSETHOTKEY(s) //DEBUGSTR(s)
#define DEBUGSTRSHUTSTEP(s) //DEBUGSTR(s)
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
BOOL gbMessagingStarted = FALSE;



//externs
HINSTANCE g_hInstance=NULL;
HWND ghWnd=NULL, ghWndWork=NULL, ghWndApp=NULL, ghWndDrag=NULL;
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


#ifdef MSGLOGGER
void DebugLogMessage(HWND h, UINT m, WPARAM w, LPARAM l, BOOL posted, BOOL extra)
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
			MSG1(WM_SIZE);
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
		if (!szMess[0]) wsprintfA(szMess, "%i", m);

		if (!szWP[0]) wsprintfA(szWP, "%i", (DWORD)w);

		if (!szLP[0]) wsprintfA(szLP, "%i(0x%08X)", (int)l, (DWORD)l);

		if (bSendToDebugger)
		{
			static SYSTEMTIME st;
			GetLocalTime(&st);
			wsprintfA(szWhole, "%02i:%02i.%03i %s%s(%s, %s, %s)\n", st.wMinute, st.wSecond, st.wMilliseconds,
			          (posted ? "Post" : "Send"), (extra ? "+" : ""), szMess, szWP, szLP);
			OutputDebugStringA(szWhole);
		}
		else if (bSendToFile)
		{
			wsprintfA(szWhole, "%s%s(%s, %s, %s)\n",
			          (posted ? "Post" : "Send"), (extra ? "+" : ""), szMess, szWP, szLP);
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
	DebugLogMessage(h,m,w,l,TRUE,extra);
	return PostMessage(h,m,w,l);
}
LRESULT SENDMESSAGE(HWND h,UINT m,WPARAM w,LPARAM l)
{
	MCHKHEAP;
	DebugLogMessage(h,m,w,l,FALSE,FALSE);
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


COORD /*__forceinline*/ MakeCoord(int X,int Y)
{
	COORD cr = {(SHORT)X,(SHORT)Y};
	return cr;
}

POINT /*__forceinline*/ MakePoint(int X,int Y)
{
	POINT pt = {X,Y};
	return pt;
}

RECT /*__forceinline*/ MakeRect(int W,int H)
{
	RECT rc = {0,0,W,H};
	return rc;
}

RECT /*__forceinline*/ MakeRect(int X1, int Y1,int X2,int Y2)
{
	RECT rc = {X1,Y1,X2,Y2};
	return rc;
}

BOOL /*__forceinline*/ CoordInRect(const COORD& c, const RECT& r)
{
	return (c.X >= r.left && c.X <= r.right) && (c.Y >= r.top && c.Y <= r.bottom);
}

BOOL IntersectSmallRect(RECT& rc1, SMALL_RECT& rc2)
{
	RECT frc2 = {rc2.Left, rc2.Top, rc2.Right, rc2.Bottom};
	RECT tmp;
	BOOL lb = IntersectRect(&tmp, &rc1, &frc2);
	return lb;
}

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
	int nLen = MyGetDlgItemText(hDlg, nID, cchMax, szText.ms_Arg);
	if (lstrcmp(rszText, szText.ms_Arg) == 0)
		return false;
	lstrcpyn(rszText, szText.ms_Arg, size);
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

const wchar_t* gsEscaped = L"\\\r\n\t\a\x1B";

void EscapeChar(bool bSet, LPCWSTR& pszSrc, LPWSTR& pszDst)
{
	_ASSERTE(pszSrc && pszDst);

	LPCWSTR  pszCtrl = L"rntae\\\"";

	if (bSet)
	{
		// Set escapes: wchar(13) --> "\\r"

		_ASSERTE(pszSrc != pszDst);

		wchar_t wc = *pszSrc;
		switch (wc)
		{
			case L'"': // 34
				*(pszDst++) = L'\\';
				*(pszDst++) = L'"';
				pszSrc++;
				break;
			case L'\\': // 92
				*(pszDst++) = L'\\';
				pszSrc++;
				if (!*pszSrc || !wcschr(pszCtrl, *pszSrc))
					*(pszDst++) = L'\\';
				break;
			case L'\r': // 13
				*(pszDst++) = L'\\';
				*(pszDst++) = L'r';
				pszSrc++;
				break;
			case L'\n': // 10
				*(pszDst++) = L'\\';
				*(pszDst++) = L'n';
				pszSrc++;
				break;
			case L'\t': // 9
				*(pszDst++) = L'\\';
				*(pszDst++) = L't';
				pszSrc++;
				break;
			case L'\b': // 8 (BS)
				*(pszDst++) = L'\\';
				*(pszDst++) = L'b';
				pszSrc++;
				break;
			case 27: //ESC
				*(pszDst++) = L'\\';
				*(pszDst++) = L'e';
				pszSrc++;
				break;
			case L'\a': // 7 (BELL)
				*(pszDst++) = L'\\';
				*(pszDst++) = L'a';
				pszSrc++;
				break;
			default:
				// Escape (if possible) ASCII symbols with codes 01..31 (dec)
				if (wc < L' ')
				{
					wchar_t wcn = pszSrc[1];
					// If next string character is 'hexadecimal' digit - back conversion will be ambiguous
					if (!((wcn >= L'0' && wcn <= L'9') || (wcn >= L'a' && wcn <= L'f') || (wcn >= L'A' && wcn <= L'F')))
					{
						*(pszDst++) = L'\\';
						*(pszDst++) = L'x';
						msprintf(pszDst, 3, L"%02X", (UINT)wc);
						pszDst+=2;
						break;
					}
				}
				*(pszDst++) = *(pszSrc++);
		}
	}
	else
	{
		// Remove escapes: "\\r" --> wchar(13), etc.

		if (*pszSrc == L'\\')
		{
			// -- Must be the same set in "Set escapes"
			switch (pszSrc[1])
			{
				case L'"':
					*(pszDst++) = L'"';
					pszSrc += 2;
					break;
				case L'\\':
					*(pszDst++) = L'\\';
					pszSrc += 2;
					break;
				case L'r': case L'R':
					*(pszDst++) = L'\r';
					pszSrc += 2;
					break;
				case L'n': case L'N':
					*(pszDst++) = L'\n';
					pszSrc += 2;
					break;
				case L't': case L'T':
					*(pszDst++) = L'\t';
					pszSrc += 2;
					break;
				case L'b': case L'B':
					*(pszDst++) = L'\b';
					pszSrc += 2;
					break;
				case L'e': case L'E':
					*(pszDst++) = 27; // ESC
					pszSrc += 2;
					break;
				case L'a': case L'A':
					*(pszDst++) = L'\a'; // BELL
					pszSrc += 2;
					break;
				case L'x':
				{
					wchar_t sTemp[5] = L"", *pszEnd = NULL;
					lstrcpyn(sTemp, pszSrc+2, 5);
					UINT wc = wcstoul(sTemp, &pszEnd, 16);
					if (pszEnd > sTemp)
					{
						*(pszDst++) = LOWORD(wc);
						pszSrc += (pszEnd - sTemp) + 2;
					}
					else
					{
						*(pszDst++) = *(pszSrc++);
					}
					break;
				}
				default:
					*(pszDst++) = *(pszSrc++);
			}
		}
		else
		{
			*(pszDst++) = *(pszSrc++);
		}
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
	wchar_t* pszErr = lstrmerge(pszStep, szInfo, bsTaskName, L"\n", lpCommandLine);
	DisplayLastError(pszErr, hr);
	free(pszErr);
}
#endif

BOOL CreateProcessDemoted(LPWSTR lpCommandLine,
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

	// Issue 1897: Created task stopped after 72 hour, so use "/bypass"
	CEStr szExe;
	if (!GetModuleFileName(NULL, szExe.GetBuffer(MAX_PATH), MAX_PATH) || szExe.IsEmpty())
	{
		DisplayLastError(L"GetModuleFileName(NULL) failed");
		return FALSE;
	}
	CEStr szCommand(lstrmerge(L"/bypass /cmd ", lpCommandLine));
	LPCWSTR pszCmdArgs = szCommand;

	BOOL lbRc = FALSE;
	BOOL lbTryStdCreate = FALSE;

#if !defined(__GNUC__)

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

	VARIANT vtUsersSID = {VT_BSTR}; vtUsersSID.bstrVal = SysAllocString(L"S-1-5-32-545");
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
	TASK_STATE taskState;
	bool bCoInitialized = false;
	DWORD nTickStart, nDuration;
	const DWORD nMaxWindowWait = 30000;
	wchar_t szIndefinitely[] = L"PT0S";

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
	//  Using Well Known SID for \\Builtin\Users group
	hr = pRootFolder->RegisterTaskDefinition(bsTaskName, pTask, TASK_CREATE, vtUsersSID, vtEmpty, TASK_LOGON_GROUP, vtZeroStr, &pRegisteredTask);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Error registering the task instance.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	//  ------------------------------------------------------
	//  Run the task
	taskState = TASK_STATE_UNKNOWN;
	hr = pRegisteredTask->Run(vtEmpty, &pRunningTask);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Error starting the task instance.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	#ifdef _DEBUG
	HRESULT hrRun; hrRun = pRunningTask->get_State(&taskState);
	#endif

	//  ------------------------------------------------------
	// Success! Task successfully started. But our task may end
	// promptly because it just bypassing the command line
	{
		lbRc = TRUE;

		// Success! Program was started.
		// Give 30 seconds for new window appears and bring it to front
		LPCWSTR pszExeName = PointToName(szExe);
		if (lstrcmpi(pszExeName, L"ConEmu.exe") == 0 || lstrcmpi(pszExeName, L"ConEmu64.exe") == 0)
		{
			nTickStart = GetTickCount();
			nDuration = 0;
			_ASSERTE(hCreated==NULL);
			hCreated = NULL;

			while (nDuration <= nMaxWindowWait/*20000*/)
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
	VariantClear(&vtUsersSID);
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

	#if defined(__GNUC__)
	if (!lbRc)
	{
		lbTryStdCreate = TRUE;
	}
	#endif

#if 0
	// There is IShellDispatch2::ShellExecute but explorer does not create/connect OutProcess servers...

	LPCWSTR pszDefCmd = cmdNew;
	wchar_t szExe[MAX_PATH+1];
	if (0 != NextArg(&pszDefCmd, szExe))
	{
		DisplayLastError(L"Invalid cmd line. ConEmu.exe not exists", -1);
		return FALSE;
	}

	OleInitialize(NULL);

	b = FALSE;
	IShellDispatch2 *pshl;
	CLSCTX ctx = CLSCTX_INPROC_HANDLER; // CLSCTX_SERVER;
	HRESULT hr = CoCreateInstance(CLSID_Shell, NULL, ctx, IID_IShellDispatch2, (void**)&pshl);
	if (SUCCEEDED(hr))
	{
		BSTR bsFile = SysAllocString(szExe);
		VARIANT vtArgs; vtArgs.vt = VT_BSTR; vtArgs.bstrVal = SysAllocString(pszDefCmd);
		VARIANT vtDir; vtDir.vt = VT_BSTR; vtDir.bstrVal = SysAllocString(gpConEmu->ms_ConEmuExeDir);
		VARIANT vtOper; vtOper.vt = VT_BSTR; vtOper.bstrVal = SysAllocString(L"open");
		VARIANT vtShow; vtShow.vt = VT_I4; vtShow.lVal = SW_SHOWNORMAL;

		hr = pshl->ShellExecute(bsFile, vtArgs, vtDir, vtOper, vtShow);
		b = SUCCEEDED(hr);

		VariantClear(&vtArgs);
		VariantClear(&vtDir);
		VariantClear(&vtOper);
		SysFreeString(bsFile);

		pshl->Release();
	}
#endif


#if 0
	// CreateRestrictedToken method - fails
	// This starts GUI, but it fails when trying to start console server - weird error 0xC0000142

	HANDLE hToken = NULL, hTokenRest = NULL;

	if (OpenProcessToken(GetCurrentProcess(),
	                    TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_EXECUTE,
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
		//TOKEN_GROUPS grp = {1, {pAdmSid, SE_GROUP_USE_FOR_DENY_ONLY}};
		SID_AND_ATTRIBUTES sidsToDisable[] =
		{
			{pAdmSid, SE_GROUP_USE_FOR_DENY_ONLY}
		};

		#ifdef __GNUC__
		//HMODULE hAdvApi = GetModuleHandle(L"AdvApi32.dll");
		//CreateRestrictedToken_t CreateRestrictedToken = (CreateRestrictedToken_t)GetProcAddress(hAdvApi, "CreateRestrictedToken");
		#endif

		if (!CreateRestrictedToken(hToken, 0,
		                         countof(sidsToDisable), sidsToDisable,
		                         0, NULL,
								 0, NULL,
								 &hTokenRest))
		{
			if (pdwLastError) *pdwLastError = GetLastError();
		}
		//if (!DuplicateToken(hToken, SecurityImpersonation, &hTokenRest))
		//{
		//	if (pdwLastError) *pdwLastError = GetLastError();
		//}
		//else if (!AdjustTokenGroups(hTokenRest, TRUE, &grp, 0, NULL, NULL))
		//{
		//	if (pdwLastError) *pdwLastError = GetLastError();
		//}
		else
		{
			if (CreateProcessAsUserW(hTokenRest, NULL, lpCommandLine,
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
		}

		if (hTokenRest) CloseHandle(hTokenRest);

		free(pAdmSid);
		CloseHandle(hToken);
	}
	else
	{
		if (pdwLastError) *pdwLastError = GetLastError();
	}

	// End of CreateRestrictedToken method
#endif


#if 0
	// Trying to use Token from Shell (explorer/desktop)
	// Fails. Create process returns error.

	// GetDesktopWindow() - это не то, оно к CSRSS принадлежит (Win8 по крайней мере)
	HWND hDesktop = GetShellWindow();
	if (!hDesktop)
	{
		if (pdwLastError) *pdwLastError = GetLastError();
		DisplayLastError(L"Failed to find desktop window!");
		return FALSE;
	}

	DWORD nDesktopPID = 0; GetWindowThreadProcessId(hDesktop, &nDesktopPID);
	HANDLE hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, nDesktopPID);
	if (!hProcess)
	{
		DWORD nErrCode = GetLastError();
		if (pdwLastError) *pdwLastError = nErrCode;
		wchar_t szInfo[100];
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Failed to open handle for desktop process!\nDesktopWND=x%08X, PID=%u", (DWORD)hDesktop, nDesktopPID);
		DisplayLastError(szInfo, nErrCode);
		return FALSE;
	}

	HANDLE hTokenDesktop = NULL;
	if (OpenProcessToken(hProcess,
						TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE |
						TOKEN_ASSIGN_PRIMARY | TOKEN_EXECUTE | TOKEN_ADJUST_SESSIONID | TOKEN_READ | TOKEN_WRITE,
						&hTokenDesktop))
	{
		//CAdjustProcessToken Adjust;
		//Adjust.Enable(2, SE_ASSIGNPRIMARYTOKEN_NAME, SE_INCREASE_QUOTA_NAME);

		//SetLastError(0);
		//BOOL bImpRc = ImpersonateLoggedOnUser(hTokenDesktop);
		//DWORD nImpErr = GetLastError();

		//if (!bImpRc)
		//{
		//	if (pdwLastError) *pdwLastError = nImpErr;
		//	DisplayLastError(L"Failed to impersonate to desktop logon!", nImpErr);
		//}
		//else
		{
			lpStartupInfo->lpDesktop = L"winsta0\\default";

			#ifdef _DEBUG
			DWORD cbRet;
			TOKEN_TYPE tt;
			TOKEN_SOURCE ts;
			DWORD nDeskSessionId, nOurSessionId;
			GetTokenInformation(hTokenDesktop, TokenType, &tt, sizeof(tt), &cbRet);
			GetTokenInformation(hTokenDesktop, TokenSource, &ts, sizeof(ts), &cbRet);
			GetTokenInformation(hTokenDesktop, TokenSessionId, &nDeskSessionId, sizeof(nDeskSessionId), &cbRet);
			#endif

			lbRc =
				CreateProcessWithTokenW(hTokenDesktop, LOGON_WITH_PROFILE,
				//CreateProcessAsUserW(hTokenDesktop,
				//CreateProcessW(
					NULL, lpCommandLine,
					//lpProcessAttributes, lpThreadAttributes, bInheritHandles,
					dwCreationFlags, lpEnvironment,
					lpCurrentDirectory, lpStartupInfo, lpProcessInformation);

			RevertToSelf();

			if (!lbRc)
			{
				DWORD nErrCode = GetLastError();
				if (pdwLastError) *pdwLastError = nErrCode;
				DisplayLastError(L"Failed to impersonate to desktop logon!", nErrCode);
			}
			else
			{
				if (pdwLastError) *pdwLastError = 0;
			}


		}

		CloseHandle(hTokenDesktop);
	}
	else
	{
		if (pdwLastError) *pdwLastError = GetLastError();
	}

	CloseHandle(hProcess);
#endif

	// If all methods fails - try to execute "as is"?
	if (lbTryStdCreate)
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
		LPCWSTR pszMsg = _T("Can't create application window!");
		gpConEmu->LogString(pszMsg, false);
		MBoxA(pszMsg);
		return FALSE;
	}

	if (gpSetCls->isAdvLogging)
	{
		wchar_t szCreated[128];
		_wsprintf(szCreated, SKIPLEN(countof(szCreated)) L"App window created, HWND=0x%08X\r\n", (DWORD)ghWndApp);
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
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Skip window 0x%08X was created and destroyed", (DWORD)hSkip);
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

int MsgBox(LPCTSTR lpText, UINT uType, LPCTSTR lpCaption /*= NULL*/, HWND ahParent /*= (HWND)-1*/, bool abModal /*= true*/)
{
	DontEnable de(abModal);

	ghDlgPendingFrom = GetForegroundWindow();

	HWND hParent = gbMessagingStarted
		? ((ahParent == (HWND)-1) ? ghWnd :ahParent)
		: NULL;

	HooksUnlocker;

	int nBtn = MessageBox(hParent, lpText ? lpText : L"<NULL>", lpCaption ? lpCaption : gpConEmu->GetLastTitle(), uType);

	ghDlgPendingFrom = NULL;

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
				CEREPORTCRASH /* http://conemu.github.io/en/Issues.html... */) : NULL;
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

	WARNING("!!! Заменить MessageBox на WaitForSingleObject(CreateThread(out,Title,dwMsgFlags),INFINITE);");

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
	return MoveWindow(hWnd, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top, bRepaint);
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
	gbMessagingStarted = TRUE;

	#ifdef _DEBUG
	wchar_t szDbg[128];
	#endif

	while (GetMessage(&Msg, NULL, 0, 0))
	{
		#ifdef _DEBUG
		if (Msg.message == WM_TIMER)
		{
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_TIMER(0x%08X,%u)\n", (DWORD)Msg.hwnd, Msg.wParam);
			DEBUGSTRTIMER(szDbg);
		}
		#endif

		if (!ProcessMessage(Msg))
			break;
	}

	gbMessagingStarted = FALSE;
}

bool ProcessMessage(MSG& Msg)
{
	bool bRc = true;
	static bool bQuitMsg = false;

	if (Msg.message == WM_QUIT)
	{
		bQuitMsg = true;
		bRc = false;
		goto wrap;
	}

	// Может быть некоторые дублирование с логированием в самих функциях
	ConEmuMsgLogger::Log(Msg, ConEmuMsgLogger::msgCommon);

	if (gpConEmu)
	{
		if (gpConEmu->isDialogMessage(Msg))
			goto wrap;
		if ((Msg.message == WM_SYSCOMMAND) && gpConEmu->isSkipNcMessage(Msg))
			goto wrap;
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

/* С командной строкой (GetCommandLineW) у нас засада */
/*

ShellExecute("open", "ShowArg.exe", "\"test1\" test2");
GetCommandLineW(): "T:\XChange\VCProject\TestRunArg\ShowArg.exe" "test1" test2

CreateProcess("ShowArg.exe", "\"test1\" test2");
GetCommandLineW(): "test1" test2

CreateProcess(NULL, "\"ShowArg.exe\" \"test1\" test2");
GetCommandLineW(): "ShowArg.exe" "test1" test2

*/

//Result:
//  cmdLine - указатель на буфер с аргументами (!) он будет освобожден через free(cmdLine)
//  cmdNew  - то что запускается (после аргумента /cmd)
//  params  - количество аргументов
//            0 - ком.строка пустая
//            ((uint)-1) - весь cmdNew должен быть ПРИКЛЕЕН к строке запуска по умолчанию
BOOL PrepareCommandLine(TCHAR*& cmdLine, TCHAR*& cmdNew, bool& isScript, uint& params)
{
	params = 0;
	cmdNew = NULL;
	isScript = false;

	LPCWSTR pszCmdLine = GetCommandLine();
	int nInitLen = _tcslen(pszCmdLine);
	cmdLine = lstrdup(pszCmdLine);
	// Имя исполняемого файла (conemu.exe)
	const wchar_t* pszExeName = PointToName(gpConEmu->ms_ConEmuExe);
	wchar_t* pszExeNameOnly = lstrdup(pszExeName);
	wchar_t* pszDot = (wchar_t*)PointToExt(pszExeNameOnly);
	_ASSERTE(pszDot);
	if (pszDot) *pszDot = 0;

	wchar_t *pszNext = NULL, *pszStart = NULL, chSave = 0;

	if (*cmdLine == L' ')
	{
		// Исполняемого файла нет - сразу начинаются аргументы
		pszNext = NULL;
	}
	else if (*cmdLine == L'"')
	{
		// Имя между кавычками
		pszStart = cmdLine+1;
		pszNext = wcschr(pszStart, L'"');

		if (!pszNext)
		{
			MBoxA(L"Invalid command line: quotes are not balanced");
			SafeFree(pszExeNameOnly);
			return FALSE;
		}

		chSave = *pszNext;
		*pszNext = 0;
	}
	else
	{
		pszStart = cmdLine;
		pszNext = wcschr(pszStart, L' ');

		if (!pszNext) pszNext = pszStart + _tcslen(pszStart);

		chSave = *pszNext;
		*pszNext = 0;
	}

	if (pszNext)
	{
		wchar_t* pszFN = wcsrchr(pszStart, L'\\');
		if (pszFN) pszFN++; else pszFN = pszStart;

		// Если первый параметр - наш conemu.exe или его путь - нужно его выбросить
		if (!lstrcmpi(pszFN, pszExeName) || !lstrcmpi(pszFN, pszExeNameOnly))
		{
			// Нужно отрезать
			INT_PTR nCopy = (nInitLen - (pszNext - cmdLine)) * sizeof(wchar_t);
			TODO("Проверить, чтобы длину корректно посчитать");

			if (nCopy > 0)
				memmove(cmdLine, pszNext+1, nCopy);
			else
				*cmdLine = 0;
		}
		else
		{
			*pszNext = chSave;
		}
	}

	// AI. Если первый аргумент начинается НЕ с '/' - считаем что эта строка должна полностью передаваться в
	// запускаемую программу (прилепляться к концу ком.строки по умолчанию)
	pszStart = cmdLine;

	while (*pszStart == L' ' || *pszStart == L'"') pszStart++;  // пропустить пробелы и кавычки

	if (*pszStart == 0)
	{
		params = 0;
		*cmdLine = 0;
		cmdNew = NULL;
		// Эта переменная нужна для того, чтобы conemu можно было перезапустить
		// из cmd файла с теми же аргументами (selfupdate)
		SetEnvironmentVariableW(L"ConEmuArgs", L"");
	}
	else
	{
		// Эта переменная нужна для того, чтобы conemu можно было перезапустить
		// из cmd файла с теми же аргументами (selfupdate)
		gpConEmu->mpsz_ConEmuArgs = lstrdup(SkipNonPrintable(cmdLine));
		SetEnvironmentVariableW(L"ConEmuArgs", gpConEmu->mpsz_ConEmuArgs);

		// Теперь проверяем наличие слеша
		if (*pszStart != L'/' && *pszStart != L'-' && !wcschr(pszStart, L'/'))
		{
			params = (uint)-1;
			cmdNew = cmdLine;
		}
		else
		{
			CEStr lsArg;
			wchar_t* pszSrc = cmdLine;
			wchar_t* pszDst = cmdLine;
			wchar_t* pszArgStart = NULL;
			params = 0;
			// Strip quotes, de-escape arguments
			while (0 == NextArg((const wchar_t**)&pszSrc, lsArg, (const wchar_t**)&pszArgStart))
			{
				// If command line contains "/cmd" - the trailer is used (without changes!) to create new process
				// Optional "/cmdlist cmd1 | cmd2 | ..." can be used to start bunch of consoles at once
				if ((lsArg.ms_Arg[0] == L'-') || (lsArg.ms_Arg[0] == L'/'))
				{
					if (lstrcmpi(lsArg.ms_Arg+1, L"cmd") == 0)
					{
						cmdNew = (wchar_t*)SkipNonPrintable(pszSrc);
						*pszDst = 0;
						break;
					}
					else if (lstrcmpi(lsArg.ms_Arg+1, L"cmdlist") == 0)
					{
						isScript = true;
						cmdNew = (wchar_t*)SkipNonPrintable(pszSrc);
						*pszDst = 0;
						break;
					}
				}

				// Historically. Command line was splitted into "Arg1\0Arg2\0Arg3\0\0"
				int iLen = lstrlen(lsArg.ms_Arg);
				wmemcpy(pszDst, lsArg.ms_Arg, iLen+1); // Include trailing zero
				pszDst += iLen+1;
				params++;
			}
		}
	}
	SafeFree(pszExeNameOnly);

	return TRUE;
}

void ResetConman()
{
	HKEY hk = 0;
	DWORD dw = 0;

	// 24.09.2010 Maks - Только если ключ конмана уже создан!
	// сбросить CreateInNewEnvironment для ConMan
	//if (0 == RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\HoopoePG_2x"),
	//        NULL, NULL, NULL, KEY_ALL_ACCESS, NULL, &hk, &dw))
	if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\HoopoePG_2x"), 0, KEY_ALL_ACCESS, &hk))
	{
		RegSetValueEx(hk, _T("CreateInNewEnvironment"), 0, REG_DWORD,
		              (LPBYTE)&(dw=0), sizeof(dw));
		RegCloseKey(hk);
	}
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
		if (pszPrefix)
		{
			_wcscat_c(pszBuf, cchMax, pszPrefix);
			_wcscat_c(pszBuf, cchMax, L" ");
		}
		if (pszConEmuStartArgs)
		{
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
			CmdArg szTmp;
			wchar_t szIcon[MAX_PATH+1] = L"";
			LPCWSTR pszTemp = pszArguments, pszIcon = NULL;
			wchar_t* pszBatch = NULL;
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
						pszBatch = gpConEmu->LoadConsoleBatch(pszTemp);
					}

					if (pszBatch)
					{
						pszTemp = pszBatch;
					}

					szTmp.Empty();
					if (NextArg(&pszTemp, szTmp) == 0)
						pszIcon = szTmp;
					break;
				}
			}
			if (pszIcon && *pszIcon)
			{
				wchar_t* pszIconEx = ExpandEnvStr(pszIcon);
				if (pszIconEx) pszIcon = pszIconEx;
				DWORD n;
				wchar_t* pszFilePart;
				n = GetFullPathName(pszIcon, countof(szIcon), szIcon, &pszFilePart);
				if (!n || (n >= countof(szIcon)) || !FileExists(szIcon))
				{
					n = SearchPath(NULL, pszIcon, NULL, countof(szIcon), szIcon, &pszFilePart);
					if (!n || (n >= countof(szIcon)))
						n = SearchPath(NULL, pszIcon, L".exe", countof(szIcon), szIcon, &pszFilePart);
					if (!n || (n >= countof(szIcon)))
						szIcon[0] = 0;
				}
				SafeFree(pszIconEx);
			}
			SafeFree(pszBatch);
			psl->SetIconLocation(szIcon[0] ? szIcon : szAppPath, 0);

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

bool GetCfgParm(uint& i, TCHAR*& curCommand, bool& Prm, TCHAR*& Val, int nMaxLen, bool bExpandAndDup = false)
{
	if (!curCommand || !*curCommand)
	{
		_ASSERTE(curCommand && *curCommand);
		return false;
	}

	// Сохраним, может для сообщения об ошибке понадобится
	TCHAR* pszName = curCommand;

	curCommand += _tcslen(curCommand) + 1; i++;

	int nLen = _tcslen(curCommand);

	if (nLen >= nMaxLen)
	{
		int nCchSize = nLen+100+_tcslen(pszName);
		wchar_t* psz = (wchar_t*)calloc(nCchSize,sizeof(wchar_t));
		if (psz)
		{
			_wsprintf(psz, SKIPLEN(nCchSize) L"Too long %s value (%i chars).\r\n", pszName, nLen);
			_wcscat_c(psz, nCchSize, curCommand);
			MBoxA(psz);
			free(psz);
		}
		//free(cmdLine);
		return false;
	}

	// Ok
	Prm = true;

	// We need independent absolute file paths, Working dir changes during ConEmu session
	if (bExpandAndDup)
		Val = GetFullPathNameEx(curCommand);
	else
		Val = curCommand;

	return true;
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
		wcscat_c(szAdd, CEREPORTCRASH /* http://conemu.github.io/en/Issues.html... */);
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
			CmdArg szExe;
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	DEBUGSTRSTARTUP(L"WinMain entered");
	int iMainRc = 0;

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
	GetVersionEx(&gOSVer);
	gnOsVer = ((gOSVer.dwMajorVersion & 0xFF) << 8) | (gOSVer.dwMinorVersion & 0xFF);
	HeapInitialize();
	RemoveOldComSpecC();
	AssertMsgBox = MsgBox;
	gn_MainThreadId = GetCurrentThreadId();
	gfnSearchAppPaths = SearchAppPaths;

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
	g_hInstance = hInstance;
	ghWorkingModule = (u64)hInstance;
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

#if defined(SHOW_STARTED_MSGBOX)
	wchar_t szTitle[128]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"Conemu started, PID=%i", GetCurrentProcessId());
	MessageBox(NULL, GetCommandLineW(), szTitle, MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND|MB_SYSTEMMODAL);
#elif defined(WAIT_STARTED_DEBUGGER)
	while (!IsDebuggerPresent())
		Sleep(250);
	int nDbg = IsDebuggerPresent();
#else

#ifdef _DEBUG
	if (_tcsstr(GetCommandLine(), L"/debugi"))
	{
		if (!IsDebuggerPresent()) _ASSERT(FALSE);
	}
	else
#endif
		if (_tcsstr(GetCommandLine(), L"/debug"))
		{
			if (!IsDebuggerPresent()) MBoxA(L"Conemu started");
		}
#endif

#ifdef DEBUG_MSG_HOOKS
	ghDbgHook = SetWindowsHookEx(WH_CALLWNDPROC, DbgCallWndProc, NULL, GetCurrentThreadId());
#endif


	//pVCon = NULL;
	//bool setParentDisabled=false;
	bool ClearTypePrm = false; LONG ClearTypeVal = CLEARTYPE_NATURAL_QUALITY;
	bool FontPrm = false; TCHAR* FontVal = NULL;
	bool IconPrm = false;
	bool SizePrm = false; LONG SizeVal = 0;
	bool BufferHeightPrm = false; int BufferHeightVal = 0;
	bool ConfigPrm = false; TCHAR* ConfigVal = NULL;
	bool PalettePrm = false; TCHAR* PaletteVal = NULL;
	//bool FontFilePrm = false; TCHAR* FontFile = NULL; //ADD fontname; by Mors
	bool WindowPrm = false; int WindowModeVal = 0;
	bool ForceUseRegistryPrm = false;
	bool LoadCfgFilePrm = false; TCHAR* LoadCfgFile = NULL;
	bool SaveCfgFilePrm = false; TCHAR* SaveCfgFile = NULL;
	bool UpdateSrcSetPrm = false; TCHAR* UpdateSrcSet = NULL;
	bool AnsiLogPathPrm = false; TCHAR* AnsiLogPath = NULL;
	bool GuiMacroPrm = false; TCHAR* ExecGuiMacro = NULL;
	bool QuakePrm = false; BYTE QuakeMode = 0;
	bool SizePosPrm = false; TCHAR *sWndX = NULL, *sWndY = NULL, *sWndW = NULL, *sWndH = NULL;
	bool SetUpDefaultTerminal = false;
	bool ExitAfterActionPrm = false;
#if 0
	//120714 - аналогичные параметры работают в ConEmuC.exe, а в GUI они и не работали. убрал пока
	bool AttachPrm = false; LONG AttachVal=0;
#endif
	bool MultiConPrm = false, MultiConValue = false;
	bool VisPrm = false, VisValue = false;
	bool ResetSettings = false;
	//bool SingleInstance = false;

	_ASSERTE(gpSetCls->SingleInstanceArg == sgl_Default);
	gpSetCls->SingleInstanceArg = sgl_Default;
	gpSetCls->SingleInstanceShowHide = sih_None;

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
//------------------------------------------------------------------------
///| Parsing the command line |///////////////////////////////////////////
//------------------------------------------------------------------------
	TCHAR* cmdNew = NULL;
	TCHAR *cmdLine = NULL;
	TCHAR *psUnknown = NULL;
	CEStr szReady;
	uint  params = 0;
	bool  isScript = false;
	bool  ReqSaveHistory = false;

	DEBUGSTRSTARTUP(L"Parsing command line");

	// [OUT] params = (uint)-1, если первый аргумент не начинается с '/'
	// т.е. комстрока такая "ConEmu.exe c:\tools\far.exe",
	// а не такая "ConEmu.exe /cmd c:\tools\far.exe",
	if (!PrepareCommandLine(/*OUT*/cmdLine, /*OUT*/cmdNew, /*OUT*/isScript, /*OUT*/params))
		return 100;

	if (params && params != (uint)-1)
	{
		TCHAR *curCommand = cmdLine;
		TODO("Если первый (после запускаемого файла) аргумент начинается НЕ с '/' - завершить разбор параметров и не заменять '""' на пробелы");
		//if (params < 1) {
		//	curCommand = NULL;
		//}
		// Parse parameters.
		// Duplicated parameters are permitted, the first value is used.
		uint i = 0;

		while (i < params && curCommand && *curCommand)
		{
			bool lbNotFound = false;
			bool lbSwitchChanged = false;

			// ':' removed from checks because otherwise it will not warn
			// on invalid usage of "-new_console:a" for example
			if (*curCommand == L'-' && curCommand[1] && !wcspbrk(curCommand+1, L"\\//|.&<>^"))
			{
				// Seems this is to be the "switch" too
				*curCommand = L'/';
				lbSwitchChanged = true;
			}

			if (*curCommand == L'/')
			{
				if (!klstricmp(curCommand, _T("/autosetup")))
				{
					BOOL lbTurnOn = TRUE;

					if ((i + 1) >= params)
						return 101;

					curCommand += _tcslen(curCommand) + 1; i++;

					if (*curCommand == _T('0'))
						lbTurnOn = FALSE;
					else
					{
						if ((i + 1) >= params)
							return 101;

						curCommand += _tcslen(curCommand) + 1; i++;
						DWORD dwAttr = GetFileAttributes(curCommand);

						if (dwAttr == (DWORD)-1 || (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
							return 102;
					}

					HKEY hk = NULL; DWORD dw;
					int nSetupRc = 100;

					if (0 != RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Command Processor"),
										   0, NULL, 0, KEY_ALL_ACCESS, NULL, &hk, &dw))
						return 103;

					if (lbTurnOn)
					{
						size_t cchMax = _tcslen(curCommand);
						LPCWSTR pszArg1 = NULL;
						if ((i + 1) < params)
						{
							// Здесь может быть "/GHWND=NEW"
							pszArg1 = curCommand + cchMax + 1;
							if (!*pszArg1)
								pszArg1 = NULL;
							else
								cchMax += _tcslen(pszArg1);
						}
						cchMax += 16; // + кавычки и пробелы всякие

						wchar_t* pszCmd = (wchar_t*)calloc(cchMax, sizeof(*pszCmd));
						_wsprintf(pszCmd, SKIPLEN(cchMax) L"\"%s\"%s%s%s", curCommand,
							pszArg1 ? L" \"" : L"", pszArg1 ? pszArg1 : L"", pszArg1 ? L"\"" : L"");


						if (0 == RegSetValueEx(hk, _T("AutoRun"), 0, REG_SZ, (LPBYTE)pszCmd,
											(DWORD)sizeof(TCHAR)*(_tcslen(pszCmd)+1))) //-V220
							nSetupRc = 1;

						free(pszCmd);
					}
					else
					{
						if (0==RegDeleteValue(hk, _T("AutoRun")))
							nSetupRc = 1;
					}

					RegCloseKey(hk);
					// сбросить CreateInNewEnvironment для ConMan
					ResetConman();
					return nSetupRc;
				}
				else if (!klstricmp(curCommand, _T("/bypass")))
				{
					// Этот ключик был придуман для прозрачного запуска консоли
					// в режиме администратора
					// (т.е. чтобы окно UAC нормально всплывало, но не мелькало консольное окно)
					// Но не получилось, пока требуются хэндлы процесса, а их не получается
					// передать в НЕ приподнятый процесс (исходный ConEmu GUI).

					if (!cmdNew || !*cmdNew)
					{
						DisplayLastError(L"Invalid cmd line. '/bypass' exists, '/cmd' not", -1);
						return 100;
					}

					// Information
					#ifdef _DEBUG
					STARTUPINFO siOur = {sizeof(siOur)};
					GetStartupInfo(&siOur);
					#endif

					STARTUPINFO si = {sizeof(si)};
					si.dwFlags = STARTF_USESHOWWINDOW;

					PROCESS_INFORMATION pi = {};

					BOOL b = CreateProcess(NULL, cmdNew, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
					if (b)
					{
						CloseHandle(pi.hProcess);
						CloseHandle(pi.hThread);
						return 0;
					}

					// Failed
					DisplayLastError(cmdNew);
					return 100;
				}
				else if (!klstricmp(curCommand, _T("/demote")))
				{
					// Запуск процесса (ком.строка после "/demote") в режиме простого юзера,
					// когда текущий процесс уже запущен "под админом". "Понизить" текущие
					// привилегии просто так нельзя, поэтому запуск идет через TaskSheduler.

					if (!cmdNew || !*cmdNew)
					{
						DisplayLastError(L"Invalid cmd line. '/demote' exists, '/cmd' not", -1);
						return 100;
					}


					// Information
					#ifdef _DEBUG
					STARTUPINFO siOur = {sizeof(siOur)};
					GetStartupInfo(&siOur);
					#endif

					STARTUPINFO si = {sizeof(si)};
					PROCESS_INFORMATION pi = {};
					si.dwFlags = STARTF_USESHOWWINDOW;
					si.wShowWindow = SW_SHOWNORMAL;

					wchar_t szCurDir[MAX_PATH+1] = L"";
					GetCurrentDirectory(countof(szCurDir), szCurDir);

					BOOL b;
					DWORD nErr = 0;


					b = CreateProcessDemoted(cmdNew, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL,
							szCurDir, &si, &pi, &nErr);


					if (b)
					{
						SafeCloseHandle(pi.hProcess);
						SafeCloseHandle(pi.hThread);
						return 0;
					}

					// If the error was not shown yet
					if (nErr) DisplayLastError(cmdNew, nErr);
					return 100;
				}
				else if (!klstricmp(curCommand, _T("/multi")))
				{
					gpConEmu->AppendExtraArgs(curCommand);
					MultiConValue = true; MultiConPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/nomulti")))
				{
					gpConEmu->AppendExtraArgs(curCommand);
					MultiConValue = false; MultiConPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/visible")))
				{
					VisValue = true; VisPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/ct")) || !klstricmp(curCommand, _T("/cleartype"))
					|| !klstricmp(curCommand, _T("/ct0")) || !klstricmp(curCommand, _T("/ct1")) || !klstricmp(curCommand, _T("/ct2")))
				{
					ClearTypePrm = true;
					switch (curCommand[3])
					{
					case L'0':
						ClearTypeVal = NONANTIALIASED_QUALITY; break;
					case L'1':
						ClearTypeVal = ANTIALIASED_QUALITY; break;
					default:
						ClearTypeVal = CLEARTYPE_NATURAL_QUALITY;
					}
				}
				// имя шрифта
				else if (!klstricmp(curCommand, _T("/font")) && i + 1 < params)
				{
					curCommand += _tcslen(curCommand) + 1; i++;

					if (!FontPrm)
					{
						FontPrm = true;
						FontVal = curCommand;
						gpConEmu->AppendExtraArgs(L"/font", curCommand);
					}
				}
				// Высота шрифта
				else if (!klstricmp(curCommand, _T("/size")) && i + 1 < params)
				{
					curCommand += _tcslen(curCommand) + 1; i++;

					if (!SizePrm)
					{
						SizePrm = true;
						SizeVal = klatoi(curCommand);
					}
				}
				#if 0
				//120714 - аналогичные параметры работают в ConEmuC.exe, а в GUI они и не работали. убрал пока
				else if (!klstricmp(curCommand, _T("/attach")) /*&& i + 1 < params*/)
				{
					//curCommand += _tcslen(curCommand) + 1; i++;
					if (!AttachPrm)
					{
						AttachPrm = true; AttachVal = -1;

						if ((i + 1) < params)
						{
							TCHAR *nextCommand = curCommand + _tcslen(curCommand) + 1;

							if (*nextCommand != _T('/'))
							{
								curCommand = nextCommand; i++;
								AttachVal = klatoi(curCommand);
							}
						}

						// интеллектуальный аттач - если к текущей консоли уже подцеплена другая копия
						if (AttachVal == -1)
						{
							HWND hCon = GetForegroundWindow();

							if (!hCon)
							{
								// консоли нет
								return 100;
							}
							else
							{
								TCHAR sClass[128];

								if (GetClassName(hCon, sClass, 128))
								{
									if (_tcscmp(sClass, VirtualConsoleClassMain)==0)
									{
										// Сверху УЖЕ другая копия ConEmu
										return 1;
									}

									// Если на самом верху НЕ консоль - это может быть панель проводника,
									// или другое плавающее окошко... Поищем ВЕРХНЮЮ консоль
									if (isConsoleClass(sClass))
									{
										wcscpy_c(sClass, RealConsoleClass);
										hCon = FindWindow(RealConsoleClass, NULL);
										if (!hCon)
											hCon = FindWindow(WineConsoleClass, NULL);

										if (!hCon)
											return 100;
									}

									if (isConsoleClass(sClass))
									{
										// перебрать все ConEmu, может кто-то уже подцеплен?
										HWND hEmu = NULL;

										while ((hEmu = FindWindowEx(NULL, hEmu, VirtualConsoleClassMain, NULL)) != NULL)
										{
											if (hCon == (HWND)GetWindowLongPtr(hEmu, GWLP_USERDATA))
											{
												// к этой консоли уже подцеплен ConEmu
												return 1;
											}
										}
									}
									else
									{
										// верхнее окно - НЕ консоль
										return 100;
									}
								}
							}

							gpSetCls->hAttachConWnd = hCon;
						}
					}
				}
				#endif
				// ADD fontname; by Mors
				else if (!klstricmp(curCommand, _T("/fontfile")) && i + 1 < params)
				{
					bool bOk = false; TCHAR* pszFile = NULL;
					if (!GetCfgParm(i, curCommand, bOk, pszFile, MAX_PATH))
					{
						return 100;
					}
					gpConEmu->AppendExtraArgs(L"/fontfile", pszFile);
					gpSetCls->RegisterFont(pszFile, TRUE);
				}
				// Register all fonts from specified directory
				else if (!klstricmp(curCommand, _T("/fontdir")) && i + 1 < params)
				{
					bool bOk = false; TCHAR* pszDir = NULL;
					if (!GetCfgParm(i, curCommand, bOk, pszDir, MAX_PATH))
					{
						return 100;
					}
					gpConEmu->AppendExtraArgs(L"/fontdir", pszDir);
					gpSetCls->RegisterFontsDir(pszDir);
				}
				else if (!klstricmp(curCommand, _T("/fs")))
				{
					WindowModeVal = rFullScreen; WindowPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/max")))
				{
					WindowModeVal = rMaximized; WindowPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/min"))
					|| !klstricmp(curCommand, _T("/mintsa"))
					|| !klstricmp(curCommand, _T("/starttsa")))
				{
					gpConEmu->WindowStartMinimized = true;
					if (klstricmp(curCommand, _T("/min")) != 0)
					{
						gpConEmu->WindowStartTsa = true;
						gpConEmu->WindowStartNoClose = (klstricmp(curCommand, _T("/mintsa")) == 0);
					}
				}
				else if (!klstricmp(curCommand, _T("/tsa")) || !klstricmp(curCommand, _T("/tray")))
				{
					gpConEmu->ForceMinimizeToTray = true;
				}
				else if (!klstricmp(curCommand, _T("/detached")))
				{
					gpConEmu->m_StartDetached = crb_On;
				}
				else if (!klstricmp(curCommand, _T("/here")))
				{
					gpConEmu->mb_ConEmuHere = true;
					gpConEmu->StoreWorkDir();
				}
				else if (!klstricmp(curCommand, _T("/noupdate")))
				{
					gpConEmu->DisableAutoUpdate = true;
				}
				else if (!klstricmp(curCommand, _T("/nokeyhook"))
					|| !klstricmp(curCommand, _T("/nokeyhooks"))
					|| !klstricmp(curCommand, _T("/nokeybhook"))
					|| !klstricmp(curCommand, _T("/nokeybhooks")))
				{
					gpConEmu->DisableKeybHooks = true;
				}
				else if (!klstricmp(curCommand, _T("/nocloseconfirm")))
				{
					gpConEmu->DisableCloseConfirm = true;
				}
				else if (!klstricmp(curCommand, _T("/nomacro")))
				{
					gpConEmu->DisableAllMacro = true;
				}
				else if (!klstricmp(curCommand, _T("/nohotkey"))
					|| !klstricmp(curCommand, _T("/nohotkeys")))
				{
					gpConEmu->DisableAllHotkeys = true;
				}
				else if (!klstricmp(curCommand, _T("/nodeftrm"))
					|| !klstricmp(curCommand, _T("/nodefterm")))
				{
					gpConEmu->DisableSetDefTerm = true;
				}
				else if (!klstricmp(curCommand, _T("/noregfont"))
					|| !klstricmp(curCommand, _T("/noregfonts")))
				{
					gpConEmu->DisableRegisterFonts = true;
				}
				else if (!klstricmp(curCommand, _T("/inside"))
					|| !lstrcmpni(curCommand, _T("/inside="), 8))
				{
					bool bRunAsAdmin = isPressed(VK_SHIFT);
					bool bSyncDir = false;
					LPCWSTR pszSyncFmt = NULL;

					gpConEmu->mb_ConEmuHere = true;
					gpConEmu->StoreWorkDir();

					if (curCommand[7] == _T('='))
					{
						bSyncDir = true;
						pszSyncFmt = curCommand+8; // \eCD /d %1 - \e - ESC, \b - BS, \n - ENTER, %1 - "dir", %2 - "bash dir"
					}

					CConEmuInside::InitInside(bRunAsAdmin, bSyncDir, pszSyncFmt, 0, NULL);
				}
				else if (!klstricmp(curCommand, _T("/insidepid")) && ((i + 1) < params))
				{
					curCommand += _tcslen(curCommand) + 1; i++;

					bool bRunAsAdmin = isPressed(VK_SHIFT);

					wchar_t* pszEnd;
					// Здесь указывается PID, в который нужно внедриться.
					DWORD nInsideParentPID = wcstol(curCommand, &pszEnd, 10);
					if (nInsideParentPID)
					{
						CConEmuInside::InitInside(bRunAsAdmin, false, NULL, nInsideParentPID, NULL);
					}
				}
				else if (!klstricmp(curCommand, _T("/insidewnd")) && ((i + 1) < params))
				{
					curCommand += _tcslen(curCommand) + 1; i++;
					if (curCommand[0] == L'0' && (curCommand[1] == L'x' || curCommand[1] == L'X'))
						curCommand += 2;
					else if (curCommand[0] == L'x' || curCommand[0] == L'X')
						curCommand ++;

					bool bRunAsAdmin = isPressed(VK_SHIFT);

					wchar_t* pszEnd;
					// Здесь указывается HWND, в котором нужно создаваться.
					HWND hParent = (HWND)wcstol(curCommand, &pszEnd, 16);
					if (hParent && IsWindow(hParent))
					{
						CConEmuInside::InitInside(bRunAsAdmin, false, NULL, 0, hParent);
					}
				}
				else if (!klstricmp(curCommand, _T("/icon")) && ((i + 1) < params))
				{
					curCommand += _tcslen(curCommand) + 1; i++;

					if (!IconPrm && *curCommand)
					{
						IconPrm = true;
						gpConEmu->mps_IconPath = ExpandEnvStr(curCommand);
					}
				}
				else if (!klstricmp(curCommand, _T("/dir")) && i + 1 < params)
				{
					curCommand += _tcslen(curCommand) + 1; i++;

					if (*curCommand)
					{
						// Например, "%USERPROFILE%"
						wchar_t* pszExpand = NULL;
						if (wcschr(curCommand, L'%') && ((pszExpand = ExpandEnvStr(curCommand)) != NULL))
						{
							gpConEmu->StoreWorkDir(pszExpand);
							SafeFree(pszExpand);
						}
						else
						{
							gpConEmu->StoreWorkDir(curCommand);
						}
					}
				}
				else if (!klstricmp(curCommand, _T("/updatejumplist")))
				{
					// Copy current Task list to Win7 Jump list (Taskbar icon)
					gpConEmu->mb_UpdateJumpListOnStartup = true;
				}
				else if (!klstricmp(curCommand, L"/log") || !klstricmp(curCommand, L"/log0"))
				{
					gpSetCls->isAdvLogging = 1;
				}
				else if (!klstricmp(curCommand, L"/log1") || !klstricmp(curCommand, L"/log2")
					|| !klstricmp(curCommand, L"/log3") || !klstricmp(curCommand, L"/log4"))
				{
					gpSetCls->isAdvLogging = (BYTE)(curCommand[4] - L'0'); // 1..4
				}
				else if (!klstricmp(curCommand, _T("/single")) || !klstricmp(curCommand, _T("/reuse")))
				{
					// "/reuse" switch to be remastered
					gpConEmu->AppendExtraArgs(curCommand);
					gpSetCls->SingleInstanceArg = sgl_Enabled;
				}
				else if (!klstricmp(curCommand, _T("/nosingle")))
				{
					gpConEmu->AppendExtraArgs(curCommand);
					gpSetCls->SingleInstanceArg = sgl_Disabled;
				}
				else if (!klstricmp(curCommand, _T("/quake"))
					|| !klstricmp(curCommand, _T("/quakeauto"))
					|| !klstricmp(curCommand, _T("/noquake")))
				{
					QuakePrm = true;
					if (!klstricmp(curCommand, _T("/quake")))
						QuakeMode = 1;
					else if (!klstricmp(curCommand, _T("/quakeauto")))
						QuakeMode = 2;
					else
					{
						QuakeMode = 0;
						if (gpSetCls->SingleInstanceArg == sgl_Default)
							gpSetCls->SingleInstanceArg = sgl_Disabled;
					}
				}
				else if (!klstricmp(curCommand, _T("/showhide")) || !klstricmp(curCommand, _T("/showhideTSA")))
				{
					gpSetCls->SingleInstanceArg = sgl_Enabled;
					gpSetCls->SingleInstanceShowHide = !klstricmp(curCommand, _T("/showhide"))
						? sih_ShowMinimize : sih_ShowHideTSA;
				}
				else if (!klstricmp(curCommand, _T("/reset"))
					|| !klstricmp(curCommand, _T("/resetdefault"))
					|| !klstricmp(curCommand, _T("/basic")))
				{
					ResetSettings = true;
					if (!klstricmp(curCommand, _T("/resetdefault")))
					{
						gpSetCls->isFastSetupDisabled = true;
					}
					else if (!klstricmp(curCommand, _T("/basic")))
					{
						gpSetCls->isFastSetupDisabled = true;
						gpSetCls->isResetBasicSettings = true;
					}
				}
				else if (!klstricmp(curCommand, _T("/nocascade"))
					|| !klstricmp(curCommand, _T("/dontcascade")))
				{
					gpConEmu->AppendExtraArgs(curCommand);
					gpSetCls->isDontCascade = true;
				}
				else if (!klstricmp(curCommand, _T("/WndX")) || !klstricmp(curCommand, _T("/WndY"))
					|| !klstricmp(curCommand, _T("/WndW")) || !klstricmp(curCommand, _T("/WndWidth"))
					|| !klstricmp(curCommand, _T("/WndH")) || !klstricmp(curCommand, _T("/WndHeight")))
				{
					TCHAR ch = curCommand[4], *psz = NULL;
					CharUpperBuff(&ch, 1);
					if (!GetCfgParm(i, curCommand, SizePosPrm, psz, 32))
					{
						return 100;
					}

					// Direct X/Y implies /nocascade
					if (ch == _T('X') || ch == _T('Y'))
						gpSetCls->isDontCascade = true;

					switch (ch)
					{
					case _T('X'): sWndX = psz; break;
					case _T('Y'): sWndY = psz; break;
					case _T('W'): sWndW = psz; break;
					case _T('H'): sWndH = psz; break;
					}
				}
				else if ((!klstricmp(curCommand, _T("/Buffer")) || !klstricmp(curCommand, _T("/BufferHeight")))
					&& i + 1 < params)
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

						if (BufferHeightVal < LONGOUTPUTHEIGHT_MIN)
							BufferHeightVal = LONGOUTPUTHEIGHT_MIN;
						else if (BufferHeightVal > LONGOUTPUTHEIGHT_MAX)
							BufferHeightVal = LONGOUTPUTHEIGHT_MAX;
					}
				}
				else if (!klstricmp(curCommand, _T("/Config")) && i + 1 < params)
				{
					//if (!ConfigPrm) -- используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, ConfigPrm, ConfigVal, 127))
					{
						return 100;
					}
				}
				else if (!klstricmp(curCommand, _T("/Palette")) && i + 1 < params)
				{
					//if (!ConfigPrm) -- используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, PalettePrm, PaletteVal, MAX_PATH))
					{
						return 100;
					}
				}
				else if (!klstricmp(curCommand, _T("/LoadRegistry")))
				{
					gpConEmu->AppendExtraArgs(curCommand);
					ForceUseRegistryPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/LoadCfgFile")) && i + 1 < params)
				{
					// используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, LoadCfgFilePrm, LoadCfgFile, MAX_PATH, true))
					{
						return 100;
					}
				}
				else if (!klstricmp(curCommand, _T("/SaveCfgFile")) && i + 1 < params)
				{
					// используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, SaveCfgFilePrm, SaveCfgFile, MAX_PATH, true))
					{
						return 100;
					}
				}
				else if (!klstricmp(curCommand, _T("/GuiMacro")) && i + 1 < params)
				{
					// выполняется только последний
					if (!GetCfgParm(i, curCommand, GuiMacroPrm, ExecGuiMacro, 0x8000, false))
					{
						return 100;
					}
				}
				else if (!klstricmp(curCommand, _T("/UpdateSrcSet")) && i + 1 < params)
				{
					// используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, UpdateSrcSetPrm, UpdateSrcSet, MAX_PATH*4, false))
					{
						return 100;
					}
				}
				else if (!klstricmp(curCommand, _T("/AnsiLog")) && i + 1 < params)
				{
					// используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, AnsiLogPathPrm, AnsiLogPath, MAX_PATH-40, true))
					{
						return 100;
					}
				}
				else if (!klstricmp(curCommand, _T("/SetDefTerm")))
				{
					SetUpDefaultTerminal = true;
				}
				else if (!klstricmp(curCommand, _T("/Exit")))
				{
					ExitAfterActionPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/QuitOnClose")))
				{
					gpConEmu->mb_ForceQuitOnClose = true;
				}
				else if (!klstricmp(curCommand, _T("/Title")) && i + 1 < params)
				{
					bool bOk = false; TCHAR* pszTitle = NULL;
					if (!GetCfgParm(i, curCommand, bOk, pszTitle, 127))
					{
						return 100;
					}
					gpConEmu->SetTitleTemplate(pszTitle);
				}
				else if (!klstricmp(curCommand, _T("/FindBugMode")))
				{
					gpConEmu->mb_FindBugMode = true;
				}
				else if (!klstricmp(curCommand, _T("/debug")) || !klstricmp(curCommand, _T("/debugi")))
				{
					// These switches were already processed
				}
				else if (!klstricmp(curCommand, _T("/?")) || !klstricmp(curCommand, _T("/h")) || !klstricmp(curCommand, _T("/help")))
				{
					//MessageBox(NULL, pHelp, L"About ConEmu...", MB_ICONQUESTION);
					ConEmuAbout::OnInfo_About();
					free(cmdLine);
					return -1;
				}
				else
				{
					lbNotFound = true;

					if (lbSwitchChanged)
					{
						*curCommand = L'-';
					}
				}
			}

			if (lbNotFound && /*i>0 &&*/ !psUnknown && (*curCommand == L'-' || *curCommand == L'/'))
			{
				// ругнуться на неизвестную команду
				psUnknown = curCommand;
			}

			curCommand += _tcslen(curCommand) + 1; i++;
		}
	}

	if (gpSetCls->isAdvLogging)
	{
		DEBUGSTRSTARTUP(L"Creating log file");
		gpConEmu->CreateLog();
	}

	//if (setParentDisabled && (gpConEmu->setParent || gpConEmu->setParent2)) {
	//	gpConEmu->setParent=false; gpConEmu->setParent2=false;
	//}

	if (psUnknown)
	{
		DEBUGSTRSTARTUP(L"Unknown switch, exiting!");
		CEStr lsFail(lstrmerge(L"Unknown switch specified:\r\n", psUnknown));
		gpConEmu->LogString(lsFail, false, false);

		LPCWSTR pszNewConWarn = NULL;
		if (wcsstr(psUnknown, L"new_console") || wcsstr(psUnknown, L"cur_console"))
			pszNewConWarn = L"\r\n\r\n" L"Switch -new_console must be specified *after* /cmd or /cmdlist";

		CEStr lsMsg(lstrmerge(
			lsFail,
			pszNewConWarn,
			L"\r\n\r\n"
			L"Visit website to get thorough switches description:\r\n"
			CEGUIARGSPAGE
			L"\r\n\r\n"
			L"Or run ‘ConEmu.exe -?’ to get the brief."
			));

		MBoxA(lsMsg);
		return 100;
	}


//------------------------------------------------------------------------
///| load settings and apply parameters |/////////////////////////////////
//------------------------------------------------------------------------

	// set config name before settings (i.e. where to load from)
	if (ConfigPrm)
	{
		DEBUGSTRSTARTUP(L"Initializing configuration name");
		gpSetCls->SetConfigName(ConfigVal);
	}

	// Сразу инициализировать событие для SingleInstance. Кто первый схватит.
	DEBUGSTRSTARTUP(L"Checking for first instance");
	gpConEmu->isFirstInstance();

	// xml-using disabled? Forced to registry?
	if (ForceUseRegistryPrm)
	{
		gpConEmu->SetForceUseRegistry();
	}
	// special config file
	else if (LoadCfgFilePrm)
	{
		DEBUGSTRSTARTUP(L"Exact cfg file was specified");
		// При ошибке - не выходим, просто покажем ее пользователю
		gpConEmu->SetConfigFile(LoadCfgFile, false/*abWriteReq*/, true/*abSpecialPath*/);
		// Release mem
		SafeFree(LoadCfgFile);
	}

	// preparing settings
	bool bNeedCreateVanilla = false;
	SettingsLoadedFlags slfFlags = slf_None;
	if (ResetSettings)
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
	if (QuakePrm)
	{
		gpConEmu->SetQuakeMode(QuakeMode);
	}

	// Update package was dropped on ConEmu icon?
	// params == (uint)-1, если первый аргумент не начинается с '/'
	if (cmdNew && *cmdNew && (params == (uint)-1))
	{
		CmdArg szPath;
		LPCWSTR pszCmdLine = cmdNew;
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
	if (cmdNew)
	{
		int iArgRc = ProcessCmdArg(cmdNew, isScript, (params == (uint)-1), szReady, ReqSaveHistory);
		if (iArgRc != 0)
		{
			return iArgRc;
		}
	}

	// Settings are loaded, fixup
	slfFlags |= slf_OnStartupLoad | slf_AllowFastConfig
		| (bNeedCreateVanilla ? slf_NeedCreateVanilla : slf_None)
		| (gpSet->IsConfigPartial ? slf_DefaultTasks : slf_None)
		| ((ResetSettings || gpSet->IsConfigNew) ? slf_DefaultSettings : slf_None);
	// выполнить дополнительные действия в классе настроек здесь
	DEBUGSTRSTARTUP(L"Config loaded, post checks");
	gpSetCls->SettingsLoaded(slfFlags, cmdNew);

	// Для gpSet->isQuakeStyle - принудительно включается gpSetCls->SingleInstanceArg

	// When "/Palette <name>" is specified
	if (PalettePrm)
	{
		gpSet->PaletteSetActive(PaletteVal);
	}

	// Set another update location (-UpdateSrcSet <URL>)
	if (UpdateSrcSetPrm)
	{
		gpSet->UpdSet.SetUpdateVerLocation(UpdateSrcSet);
	}

	// Force "AnsiLog" feature
	if (AnsiLogPathPrm)
	{
		gpSet->isAnsiLog = (AnsiLogPath && *AnsiLogPath);
		SafeFree(gpSet->pszAnsiLog);
		gpSet->pszAnsiLog = AnsiLogPath;
		AnsiLogPath = NULL;
	}

	// Forced window size or pos
	// Call this AFTER SettingsLoaded because we (may be)
	// don't want to change ‘xml-stored’ values
	if (SizePosPrm)
	{
		if (sWndX)
			gpConEmu->SetWindowPosSizeParam(L'X', sWndX);
		if (sWndY)
			gpConEmu->SetWindowPosSizeParam(L'Y', sWndY);
		if (sWndW)
			gpConEmu->SetWindowPosSizeParam(L'W', sWndW);
		if (sWndH)
			gpConEmu->SetWindowPosSizeParam(L'H', sWndH);
	}

	DEBUGSTRSTARTUPLOG(L"SettingsLoaded");


//------------------------------------------------------------------------
///| Processing actions |/////////////////////////////////////////////////
//------------------------------------------------------------------------
	// gpConEmu->mb_UpdateJumpListOnStartup - Обновить JumpLists
	// SaveCfgFilePrm, SaveCfgFile - сохранить настройки в xml-файл (можно использовать вместе с ResetSettings)
	// ExitAfterActionPrm - сразу выйти после выполнения действий
	// не наколоться бы с isAutoSaveSizePos

	if (gpConEmu->mb_UpdateJumpListOnStartup && ExitAfterActionPrm)
	{
		DEBUGSTRSTARTUP(L"Updating Win7 task list");
		if (!UpdateWin7TaskList(true/*bForce*/, true/*bNoSuccMsg*/))
		{
			if (!iMainRc) iMainRc = 10;
		}
	}

	// special config file
	if (SaveCfgFilePrm)
	{
		// Сохранять конфиг только если получилось сменить путь (создать файл)
		DEBUGSTRSTARTUP(L"Force write current config to settings storage");
		if (!gpConEmu->SetConfigFile(SaveCfgFile, true/*abWriteReq*/, true/*abSpecialPath*/))
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
		// Release mem
		SafeFree(SaveCfgFile);
	}

	// Only when ExitAfterActionPrm, otherwise - it will be called from ConEmu's PostCreate
	if (SetUpDefaultTerminal)
	{
		_ASSERTE(!gpConEmu->DisableSetDefTerm);

		gpSet->isSetDefaultTerminal = true;
		gpSet->isRegisterOnOsStartup = true;

		if (ExitAfterActionPrm)
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
	if (ExitAfterActionPrm)
	{
		DEBUGSTRSTARTUP(L"Exit was requested");
		goto wrap;
	}

	if (GuiMacroPrm && ExecGuiMacro && *ExecGuiMacro)
	{
		gpConEmu->SetPostGuiMacro(ExecGuiMacro);
	}

//------------------------------------------------------------------------
///| Continue normal work mode  |/////////////////////////////////////////
//------------------------------------------------------------------------

	// Если в режиме "Inside" подходящего окна не нашли и юзер отказался от "обычного" режима
	// mh_InsideParentWND инициализируется вызовом InsideFindParent из Settings::LoadSettings()
	if (gpConEmu->mp_Inside && (gpConEmu->mp_Inside->mh_InsideParentWND == INSIDE_PARENT_NOT_FOUND))
	{
		DEBUGSTRSTARTUP(L"Bad InsideParentHWND, exiting");
		free(cmdLine);
		return 100;
	}


	// Проверить наличие необходимых файлов (перенес сверху, чтобы учитывался флажок "Inject ConEmuHk")
	if (!gpConEmu->CheckRequiredFiles())
	{
		DEBUGSTRSTARTUP(L"Required files were not found, exiting");
		free(cmdLine);
		return 100;
	}


	//#pragma message("Win2k: CLEARTYPE_NATURAL_QUALITY")
	//if (ClearTypePrm)
	//	gpSet->LogFont.lfQuality = CLEARTYPE_NATURAL_QUALITY;
	//if (FontPrm)
	//	_tcscpy(gpSet->LogFont.lfFaceName, FontVal);
	//if (SizePrm)
	//	gpSet->LogFont.lfHeight = SizeVal;
	if (BufferHeightPrm)
	{
		gpSetCls->SetArgBufferHeight(BufferHeightVal);
	}

	if (!WindowPrm)
	{
		if (nCmdShow == SW_SHOWMAXIMIZED)
			gpSet->_WindowMode = wmMaximized;
		else if (nCmdShow == SW_SHOWMINIMIZED || nCmdShow == SW_SHOWMINNOACTIVE)
			gpConEmu->WindowStartMinimized = true;
	}
	else
	{
		gpSet->_WindowMode = (ConEmuWindowMode)WindowModeVal;
	}

	if (MultiConPrm)
		gpSet->mb_isMulti = MultiConValue;

	if (VisValue)
		gpSet->isConVisible = VisPrm;

	// Если запускается conman (нафига?) - принудительно включить флажок "Обновлять handle"
	//TODO("Deprecated: isUpdConHandle использоваться не должен");

	if (gpSetCls->IsMulti() || StrStrI(gpConEmu->GetCmd(), L"conman.exe"))
	{
		//gpSet->isUpdConHandle = TRUE;

		// сбросить CreateInNewEnvironment для ConMan
		ResetConman();
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
		FontPrm ? FontVal : NULL,
		SizePrm ? SizeVal : -1,
		ClearTypePrm ? ClearTypeVal : -1
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

			if (gpConEmu->RunSingleInstance())
			{
				gpConEmu->LogString(L"Passed to first instance, exiting");
				return 0; // командная строка успешно запущена в существующем экземпляре
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
		free(cmdLine);
		//delete pVCon;
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

	free(cmdLine);
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

	ShutdownGuiStep(L"Gui terminated");

wrap:
	// HeapDeinitialize() - Нельзя. Еще живут глобальные объекты
	DEBUGSTRSTARTUP(L"WinMain exit");
	return iMainRc;
}
