
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"
#include <commctrl.h>
#include <shlobj.h>
#include <tlhelp32.h>
#ifndef __GNUC__
#include <dbghelp.h>
#include <shobjidl.h>
#include <propkey.h>
#include <taskschd.h>
#else
#include "../common/DbgHlpGcc.h"
#endif
#include "../common/ConEmuCheck.h"
#include "../common/execute.h"
//#include "../common/TokenHelper.h"
#include "Options.h"
#include "ConEmu.h"
#include "Inside.h"
#include "TaskBar.h"
#include "DwmHelper.h"
#include "ConEmuApp.h"
#include "Update.h"
#include "Recreate.h"
#include "DefaultTerm.h"
#include "version.h"

#define FULL_STARTUP_ENV
#include "../common/StartupEnv.h"

#ifdef _DEBUG
//	#define SHOW_STARTED_MSGBOX
//	#define WAIT_STARTED_DEBUGGER
//	#define DEBUG_MSG_HOOKS
#endif

#define DEBUGSTRMOVE(s) //DEBUGSTR(s)
#define DEBUGSTRTIMER(s) //DEBUGSTR(s)
#define DEBUGSTRSETHOTKEY(s) DEBUGSTR(s)

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

#ifdef _DEBUG
wchar_t gszDbgModLabel[6] = {0};
#endif


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
BOOL gbDontEnable = FALSE;
BOOL gbDebugLogStarted = FALSE;
BOOL gbDebugShowRects = FALSE;
CEStartupEnv* gpStartEnv = NULL;


const TCHAR *const gsClassName = VirtualConsoleClass; // окна отрисовки
const TCHAR *const gsClassNameParent = VirtualConsoleClassMain; // главное окно
const TCHAR *const gsClassNameWork = VirtualConsoleClassWork; // Holder для всех VCon
const TCHAR *const gsClassNameBack = VirtualConsoleClassBack; // Подложка (со скроллерами) для каждого VCon
const TCHAR *const gsClassNameApp = VirtualConsoleClassApp;


OSVERSIONINFO gOSVer = {};
WORD gnOsVer = 0x500;
bool gbIsWine = false;
bool gbIsDBCS = false;
// Drawing console font face name (default)
wchar_t gsDefGuiFont[32] = L"Lucida Console"; // gbIsWine ? L"Liberation Mono" : L"Lucida Console"
// Set this font (default) in real console window to enable unicode support
wchar_t gsDefConFont[32] = L"Lucida Console"; // DBCS ? L"Liberation Mono" : L"Lucida Console"
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

void ShutdownGuiStep(LPCWSTR asInfo, int nParm1 /*= 0*/, int nParm2 /*= 0*/, int nParm3 /*= 0*/, int nParm4 /*= 0*/)
{
#ifdef _DEBUG
	static int nDbg = 0;
	if (!nDbg)
		nDbg = IsDebuggerPresent() ? 1 : 2;
	if (nDbg != 1)
		return;
	wchar_t szFull[512];
	msprintf(szFull, countof(szFull), L"%u:ConEmuG:PID=%u:TID=%u: ",
		GetTickCount(), GetCurrentProcessId(), GetCurrentThreadId());
	if (asInfo)
	{
		int nLen = lstrlen(szFull);
		msprintf(szFull+nLen, countof(szFull)-nLen, asInfo, nParm1, nParm2, nParm3, nParm4);
	}
	lstrcat(szFull, L"\n");
	OutputDebugString(szFull);
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

wchar_t* GetDlgItemText(HWND hDlg, WORD nID)
{
	wchar_t* pszText = NULL;
	size_t cchMax = 0;
	MyGetDlgItemText(hDlg, nID, cchMax, pszText);
	return pszText;
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

			//if (bEscapes)
			//{
			//	wchar_t* pszDst = EscapeString(false, pszText);
			//	if (pszDst)
			//	{
			//		_wcscpy_c(pszText, cchMax, pszDst);
			//		free(pszDst);
			//	}
			//}
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

		switch (*pszSrc)
		{
			case L'"':
				*(pszDst++) = L'\\';
				*(pszDst++) = L'"';
				pszSrc++;
				break;
			case L'\\':
				*(pszDst++) = L'\\';
				pszSrc++;
				if (!*pszSrc || !wcschr(pszCtrl, *pszSrc))
					*(pszDst++) = L'\\';
				break;
			case L'\r':
				*(pszDst++) = L'\\';
				*(pszDst++) = L'r';
				pszSrc++;
				break;
			case L'\n':
				*(pszDst++) = L'\\';
				*(pszDst++) = L'n';
				pszSrc++;
				break;
			case L'\t':
				*(pszDst++) = L'\\';
				*(pszDst++) = L't';
				pszSrc++;
				break;
			case 27: //ESC
				*(pszDst++) = L'\\';
				*(pszDst++) = L'e';
				pszSrc++;
				break;
			case L'\a': //BELL
				*(pszDst++) = L'\\';
				*(pszDst++) = L'a';
				pszSrc++;
				break;
			default:
				*(pszDst++) = *(pszSrc++);
		}
	}
	else
	{
		// Remove escapes: "\\r" --> wchar(13)

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
				case L'e': case L'E':
					*(pszDst++) = 27; // ESC
					pszSrc += 2;
					break;
				case L'a': case L'A':
					*(pszDst++) = L'\a'; // BELL
					pszSrc += 2;
					break;
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

	size_t cchLen = _tcslen(asWinPath) + (bAutoQuote ? 3 : 1);
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

wchar_t* SelectFolder(LPCWSTR asTitle, LPCWSTR asDefFolder /*= NULL*/, HWND hParent /*= ghWnd*/, bool bAutoQuote /*= true*/, bool bCygwin /*= false*/)
{
	wchar_t* pszResult = NULL;

	BROWSEINFO bi = {hParent};
	wchar_t szFolder[MAX_PATH+1] = {0};
	if (asDefFolder)
		wcscpy_c(szFolder, asDefFolder);
	bi.pszDisplayName = szFolder;
	wchar_t szTitle[100];
	bi.lpszTitle = wcscpy(szTitle, asTitle ? asTitle : L"Choose folder");
	bi.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS | BIF_VALIDATE;
	bi.lpfn = CRecreateDlg::BrowseCallbackProc;
	bi.lParam = (LPARAM)szFolder;
	LPITEMIDLIST pRc = SHBrowseForFolder(&bi);

	if (pRc)
	{
		if (SHGetPathFromIDList(pRc, szFolder))
		{
			if (bCygwin)
			{
				pszResult = DupCygwinPath(szFolder, bAutoQuote);
			}
			else if (bAutoQuote && (wcschr(szFolder, L' ') != NULL))
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

wchar_t* SelectFile(LPCWSTR asTitle, LPCWSTR asDefFile /*= NULL*/, HWND hParent /*= ghWnd*/, LPCWSTR asFilter /*= NULL*/, bool abAutoQuote /*= true*/, bool bCygwin /*= false*/, bool bSaveNewFile /*= false*/)
{
	wchar_t* pszResult = NULL;

	wchar_t temp[MAX_PATH+10] = {};
	if (asDefFile)
		_wcscpy_c(temp+1, countof(temp)-2, asDefFile);

	OPENFILENAME ofn = {sizeof(ofn)};
	ofn.hwndOwner = ghOpWnd;
	ofn.lpstrFilter = asFilter ? asFilter : L"All files (*.*)\0*.*\0Text files (*.txt,*.ini,*.log)\0*.txt;*.ini;*.log\0Executables (*.exe,*.com,*.bat,*.cmd)\0*.exe;*.com;*.bat;*.cmd\0Scripts (*.vbs,*.vbe,*.js,*.jse)\0*.vbs;*.vbe;*.js;*.jse\0\0";
	//ofn.lpstrFilter = L"All files (*.*)\0*.*\0\0";
	ofn.lpstrFile = temp+1;
	ofn.nMaxFile = countof(temp)-10;
	ofn.lpstrTitle = asTitle ? asTitle : L"Choose file";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
		| OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|(bSaveNewFile ? 0 : OFN_FILEMUSTEXIST);

	BOOL bRc = bSaveNewFile
		? GetSaveFileName(&ofn)
		: GetOpenFileName(&ofn);

	if (bRc)
	{
		LPCWSTR pszName = temp+1;

		if (bCygwin)
		{
			pszResult = DupCygwinPath(pszName, abAutoQuote);
		}
		else
		{
			if (abAutoQuote && (wcschr(pszName, L' ') != NULL))
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

bool NextLine(const wchar_t*& pszFrom, wchar_t** pszLine)
{
	bool bFound = false;
	_ASSERTE(pszLine && !*pszLine);

	while (*pszFrom == L'\r' || *pszFrom == L'\n' || *pszFrom == L'\t' || *pszFrom == L' ')
		pszFrom++;

	if (pszFrom && *pszFrom)
	{
		const wchar_t* pszEnd = wcschr(pszFrom, L'\n');
		if (pszEnd && (*(pszEnd-1) == L'\r'))
			pszEnd--;

		if (pszEnd)
		{
			size_t cchLen = pszEnd - pszFrom;
			*pszLine = (wchar_t*)malloc((cchLen+1)*sizeof(**pszLine));
			wmemcpy(*pszLine, pszFrom, cchLen);
			(*pszLine)[cchLen] = 0;
		}
		else
		{
			*pszLine = lstrdup(pszFrom);
		}

		bFound = true;
	}

	return bFound;
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


BOOL CreateProcessDemoted(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
							 LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
							 BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
							 LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
							 LPDWORD pdwLastError)
{
	BOOL lbRc = FALSE;
	BOOL lbTryStdCreate = FALSE;

	Assert(lpApplicationName==NULL);

	LPCWSTR pszCmdArgs = lpCommandLine;
	wchar_t szExe[MAX_PATH+1];
	if (0 != NextArg(&pszCmdArgs, szExe))
	{
		DisplayLastError(L"Invalid cmd line. Executable not exists", -1);
		return 100;
	}

	// Task не выносит окна созданных задач "наверх"
	HWND hPrevEmu = FindWindowEx(NULL, NULL, VirtualConsoleClassMain, NULL);
	HWND hCreated = NULL;
	


#if !defined(__GNUC__)
	// Really working method: Run cmd-line via task sheduler
	// But there is one caveat: Task sheduler may be disable on PC!

	wchar_t szUniqTaskName[128];
	_wsprintf(szUniqTaskName, SKIPLEN(countof(szUniqTaskName)) L"ConEmu %02u%02u%02u%s starter ParentPID=%u", (MVV_1%100),MVV_2,MVV_3,_T(MVV_4a), GetCurrentProcessId());

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
	IAction *pAction = NULL;
	IExecAction *pExecAction = NULL;
	IActionCollection *pActionCollection = NULL;
	ITrigger *pTrigger = NULL;
	ITriggerCollection *pTriggerCollection = NULL;
	ITaskDefinition *pTask = NULL;
	ITaskFolder *pRootFolder = NULL;
	ITaskService *pService = NULL;
	HRESULT hr;
	TASK_STATE taskState;
	bool bCoInitialized = false;
	DWORD nTickStart, nDuration;
	const DWORD nMaxTaskWait = 20000;
	const DWORD nMaxWindowWait = 20000;

	//  ------------------------------------------------------
	//  Initialize COM.
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		DisplayLastError(L"CoInitializeEx failed: 0x%08X", hr);
		goto wrap;
	}
	bCoInitialized = true;

	//  Set general COM security levels.
	hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);
	if (FAILED(hr))
	{
		DisplayLastError(L"CoInitializeSecurity failed: 0x%08X", hr);
		goto wrap;
	}

	//  ------------------------------------------------------
	//  Create an instance of the Task Service. 
	hr = CoCreateInstance( CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService );  
	if (FAILED(hr))
	{
		DisplayLastError(L"Failed to CoCreate an instance of the TaskService class: 0x%08X", hr);
		goto wrap;
	}

	//  Connect to the task service.
	hr = pService->Connect(vtEmpty, vtEmpty, vtEmpty, vtEmpty);
	if (FAILED(hr))
	{
		DisplayLastError(L"ITaskService::Connect failed: 0x%08X", hr);
		goto wrap;
	}

	//  ------------------------------------------------------
	//  Get the pointer to the root task folder.  This folder will hold the
	//  new task that is registered.
	hr = pService->GetFolder(bsRoot, &pRootFolder);
	if( FAILED(hr) )
	{
		DisplayLastError(L"Cannot get Root Folder pointer: 0x%08X", hr);
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
		DisplayLastError(L"Failed to CoCreate an instance of the TaskService class: 0x%08X", hr);
		goto wrap;
	}

	SafeRelease(pService);  // COM clean up.  Pointer is no longer used.

	//  ------------------------------------------------------
	//  Get the trigger collection to insert the registration trigger.
	hr = pTask->get_Triggers(&pTriggerCollection);
	if (FAILED(hr))
	{
		DisplayLastError(L"Cannot get trigger collection: 0x%08X", hr);
		goto wrap;
	}

	//  Add the registration trigger to the task.
	hr = pTriggerCollection->Create( TASK_TRIGGER_REGISTRATION, &pTrigger);
	if (FAILED(hr))
	{
		DisplayLastError(L"Cannot add registration trigger to the Task 0x%08X", hr);
		goto wrap;
	}
	SafeRelease(pTriggerCollection);  // COM clean up.  Pointer is no longer used.
	SafeRelease(pTrigger);

	//  ------------------------------------------------------
	//  Add an Action to the task.     

	//  Get the task action collection pointer.
	hr = pTask->get_Actions(&pActionCollection);
	if (FAILED(hr))
	{
		DisplayLastError(L"Cannot get Task collection pointer: 0x%08X", hr);
		goto wrap;
	}

	//  Create the action, specifying that it is an executable action.
	hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
	if (FAILED(hr))
	{
		DisplayLastError(L"pActionCollection->Create failed: 0x%08X", hr);
		goto wrap;
	}
	SafeRelease(pActionCollection);  // COM clean up.  Pointer is no longer used.

	hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
	if( FAILED(hr) )
	{
		DisplayLastError(L"pAction->QueryInterface failed: 0x%08X", hr);
		goto wrap;
	}
	SafeRelease(pAction);

	//  Set the path of the executable to the user supplied executable.
	hr = pExecAction->put_Path(bsExecutablePath);
	if (FAILED(hr))
	{
		DisplayLastError(L"Cannot set path of executable: 0x%08X", hr);
		goto wrap;
	}

	hr = pExecAction->put_Arguments(bsArgs);
	if (FAILED(hr))
	{
		DisplayLastError(L"Cannot set arguments of executable: 0x%08X", hr);
		goto wrap;
	}

	if (bsDir)
	{
		hr = pExecAction->put_WorkingDirectory(bsDir);
		if (FAILED(hr))
		{
			DisplayLastError(L"Cannot set working directory of executable: 0x%08X", hr);
			goto wrap;
		}
	}

	//  ------------------------------------------------------
	//  Save the task in the root folder.
	//  Using Well Known SID for \\Builtin\Users group
	hr = pRootFolder->RegisterTaskDefinition(bsTaskName, pTask, TASK_CREATE, vtUsersSID, vtEmpty, TASK_LOGON_GROUP, vtZeroStr, &pRegisteredTask);
	if (FAILED(hr))
	{
		DisplayLastError(L"Error saving the Task : 0x%08X", hr);
		goto wrap;
	}
	
	// Success! Task successfully registered.
	// Give 20 seconds for the task to start
	nTickStart = GetTickCount();
	nDuration = 0;
	taskState = TASK_STATE_UNKNOWN;
	while (nDuration <= nMaxTaskWait/*20000*/)
	{
		hr = pRegisteredTask->get_State(&taskState);
		if (taskState == TASK_STATE_RUNNING)
		{
			// Task is running
			break;
		}
		Sleep(100);
		nDuration = (GetTickCount() - nTickStart);
	}
	
	if (taskState != TASK_STATE_RUNNING)
	{
		wchar_t* pszErr = lstrmerge(L"Failed to start task in user mode, timeout!\n", lpCommandLine);
		DisplayLastError(pszErr, hr);
		free(pszErr);
	}
	else
	{	
		// OK, считаем что успешно запустились
		lbRc = TRUE;

		// Success! Program was started.
		// Give 20 seconds for new window appears and bring it to front
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
		DisplayLastError(L"Error deleting the Task : 0x%08X", hr);
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
	SafeRelease(pAction);
	SafeRelease(pExecAction);
	SafeRelease(pActionCollection);
	SafeRelease(pTrigger);
	SafeRelease(pTriggerCollection);
	SafeRelease(pTask);
	SafeRelease(pRootFolder);
	SafeRelease(pService);
	// Finalize
	if (bCoInitialized)
		CoUninitialize();
	if (pdwLastError) *pdwLastError = hr;
	// End of Task-sheduler mode
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
		return 100;
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
					lpApplicationName, lpCommandLine,
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
		lbRc = CreateProcess(lpApplicationName, lpCommandLine,
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

bool isConsoleService(LPCWSTR pszProcessName)
{
	LPCWSTR pszName = PointToName(pszProcessName);
	if (!pszName || !*pszName)
	{
		_ASSERTE(pszName && *pszName);
		return false;
	}

	LPCWSTR lsNameExt[] = {L"ConEmuC.exe", L"ConEmuC64.exe", L"csrss.exe", L"conhost.exe"};
	LPCWSTR lsName[] = {L"ConEmuC", L"ConEmuC64", L"csrss", L"conhost"};

	if (wcschr(pszName, L'.'))
	{
		for (size_t i = 0; i < countof(lsNameExt); i++)
		{
			if (lstrcmpi(pszName, lsNameExt[i]) == 0)
				return true;
		}
	}
	else
	{
		for (size_t i = 0; i < countof(lsName); i++)
		{
			if (lstrcmpi(pszName, lsName[i]) == 0)
				return true;
		}
	}

	return false;
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
	if (!ghWnd || gpConEmu->isMainThread())
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

int MessageBox(LPCTSTR lpText, UINT uType, LPCTSTR lpCaption /*= NULL*/, HWND hParent /*= NULL*/)
{
	DontEnable de;

	int nBtn = MessageBox(gbMessagingStarted ? (hParent ? hParent : ghWnd) : NULL,
		lpText, lpCaption ? lpCaption : gpConEmu->GetLastTitle(), uType);

	return nBtn;
}


// Возвращает текст с информацией о пути к сохраненному дампу
DWORD CreateDumpForReport(LPEXCEPTION_POINTERS ExceptionInfo, wchar_t (&szFullInfo)[1024]);
#include "../common/Dump.h"

void AssertBox(LPCTSTR szText, LPCTSTR szFile, UINT nLine, LPEXCEPTION_POINTERS ExceptionInfo /*= NULL*/)
{
#ifdef _DEBUG
//	_ASSERTE(FALSE);
#endif

	int nRet = IDRETRY;

	size_t cchMax = (szText ? _tcslen(szText) : 0) + (szFile ? _tcslen(szFile) : 0) + 255;
	wchar_t* pszFull = (wchar_t*)malloc(cchMax*sizeof(*pszFull));

	if (pszFull)
	{
		_wsprintf(pszFull, SKIPLEN(cchMax)
			L"Assertion\r\n%s\r\nat\r\n%s:%i\r\n\r\nPress <Retry> to copy text information to clipboard\r\nand report a bug (open project web page)",
			szText, szFile, nLine);

		nRet = MessageBox(NULL, pszFull, gpConEmu->GetDefaultTitle(), MB_RETRYCANCEL|MB_ICONSTOP|MB_SYSTEMMODAL);
	}

	if (nRet == IDRETRY)
	{
		bool bProcessed = false;

		#ifdef _DEBUG
		if (IsDebuggerPresent())
		{
			MyAssertTrap();
			bProcessed = true;
		}
		#endif

		wchar_t szFullInfo[1024] = L"";

		if (!bProcessed)
		{
			//-- Не нужно, да и дамп некорректно формируется, если "руками" ex формировать.
			//EXCEPTION_RECORD er0 = {0xC0000005}; er0.ExceptionAddress = AssertBox;
			//EXCEPTION_POINTERS ex0 = {&er0};
			//if (!ExceptionInfo) ExceptionInfo = &ex0;

			CreateDumpForReport(ExceptionInfo, szFullInfo);
		}

		if (szFullInfo[0])
		{
			wchar_t* pszAdd = lstrmerge(pszFull, L"\r\n\r\n", szFullInfo);
			CopyToClipboard(pszAdd ? pszAdd : szFullInfo);
			SafeFree(pszAdd);
		}
		else if (pszFull)
		{
			CopyToClipboard(pszFull);
		}

		gpConEmu->OnInfo_ReportCrash(szFullInfo[0] ? szFullInfo : NULL);
	}

	SafeFree(pszFull);
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
	nBtn = MessageBox(out ? out : asLabel, dwMsgFlags, asTitle, hParent);
	gbInDisplayLastError = lb;

	MCHKHEAP
	if (lpMsgBuf)
		LocalFree(lpMsgBuf);
	if (out)	
		delete [] out;
	MCHKHEAP
	return nBtn;
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
		// Может быть некоторые дублирование с логированием в самих функциях
		ConEmuMsgLogger::Log(Msg, ConEmuMsgLogger::msgCommon);

		#ifdef _DEBUG
		if (Msg.message == WM_TIMER)
		{
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_TIMER(0x%08X,%u)\n", (DWORD)Msg.hwnd, Msg.wParam);
			DEBUGSTRTIMER(szDbg);
		}
		#endif

		BOOL lbDlgMsg = FALSE;

		if (gpConEmu)
			lbDlgMsg = gpConEmu->isDialogMessage(Msg);

		//if (Msg.message == WM_INITDIALOG)
		//{
		//	SendMessage(Msg.hwnd, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
		//	SendMessage(Msg.hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);
		//}

		if (!lbDlgMsg)
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}

	gbMessagingStarted = FALSE;
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

void SplitCommandLine(wchar_t *str, uint *n)
{
	*n = 0;
	wchar_t *dst = str, ts;

	while(*str == ' ')
		str++;

	ts = ' ';

	while(*str)
	{
		if (*str == '"')
		{
			ts ^= 2; // ' ' <-> '"'
			str++;
		}

		while(*str && *str != '"' && *str != ts)
			*dst++ = *str++;

		if (*str == '"')
			continue;

		while(*str == ' ')
			str++;

		*dst++ = 0;
		(*n)++;
	}

	return;
}

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
			MBoxA(L"Invalid command line: quates are not balanced");
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

	while(*pszStart == L' ' || *pszStart == L'"') pszStart++;  // пропустить пробелы и кавычки

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
		SetEnvironmentVariableW(L"ConEmuArgs", cmdLine);
		//lstrcpyn(gpConEmu->ms_ConEmuArgs, cmdLine, ARRAYSIZE(gpConEmu->ms_ConEmuArgs));
		gpConEmu->mpsz_ConEmuArgs = lstrdup(cmdLine);

		// Теперь проверяем наличие слеша
		if (*pszStart != L'/' && *pszStart != L'-' && !wcschr(pszStart, L'/'))
		{
			params = (uint)-1;
			cmdNew = cmdLine;
		}
		else
		{
			// Если ком.строка содержит "/cmd" - все что после него используется для создания нового процесса
			// или "/cmdlist cmd1 | cmd2 | ..."
			cmdNew = wcsstr(cmdLine, L"/cmd");

			if (cmdNew)
			{
				*cmdNew = 0;
				cmdNew++;

				if (lstrcmpni(cmdNew, L"cmdlist", 7) == 0)
				{
					cmdNew += 7;
					isScript = true;
				}
				else
				{
					cmdNew += 3;
				}

				// Пропустить пробелы
				cmdNew = (wchar_t*)SkipNonPrintable(cmdNew);
			}

			// cmdLine готов к разбору
			SplitCommandLine(cmdLine, &params);
		}
	}

	return TRUE;
}

void ResetConman()
{
	HKEY hk = 0;
	DWORD dw = 0;

	// 24.09.2010 Maks - Только если ключ конмана уже создан!
	// сбрость CreateInNewEnvironment для ConMan
	//if (0 == RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\HoopoePG_2x"),
	//        NULL, NULL, NULL, KEY_ALL_ACCESS, NULL, &hk, &dw))
	if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\HoopoePG_2x"), 0, KEY_ALL_ACCESS, &hk))
	{
		RegSetValueEx(hk, _T("CreateInNewEnvironment"), 0, REG_DWORD,
		              (LPBYTE)&(dw=0), sizeof(dw));
		RegCloseKey(hk);
	}
}

#ifndef __GNUC__
// Creates a CLSID_ShellLink to insert into the Tasks section of the Jump List.  This type of Jump
// List item allows the specification of an explicit command line to execute the task.
HRESULT _CreateShellLink(PCWSTR pszArguments, PCWSTR pszPrefix, PCWSTR pszTitle, IShellLink **ppsl)
{
	if ((!pszArguments || !*pszArguments) && (!pszTitle || !*pszTitle))
	{
		return E_INVALIDARG;
	}

	LPCWSTR pszConfig = gpSetCls->GetConfigName();
	if (pszConfig && !*pszConfig)
		pszConfig = NULL;

	wchar_t* pszBuf = NULL;
	if (!pszArguments || !*pszArguments)
	{
		size_t cchMax = _tcslen(pszTitle)
			+ (pszPrefix ? _tcslen(pszPrefix) : 0)
			+ (pszConfig ? _tcslen(pszConfig) : 0)
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
		if (pszConfig)
		{
			_wcscat_c(pszBuf, cchMax, L"/config \"");
			_wcscat_c(pszBuf, cchMax, pszConfig);
			_wcscat_c(pszBuf, cchMax, L"\" ");
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
			wchar_t szIcon[MAX_PATH+1], szTmp[MAX_PATH+1]; szIcon[0] = 0; szTmp[0] = 0;
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

					if (NextArg(&pszTemp, szTmp) == 0)
						pszIcon = szTmp;
					break;
				}
			}
			if (pszIcon && *pszIcon)
			{
				DWORD n;
				wchar_t* pszFilePart;
				n = GetFullPathName(szTmp, countof(szIcon), szIcon, &pszFilePart);
				if (!n || (n >= countof(szIcon)) || !FileExists(szIcon))
				{
					n = SearchPath(NULL, pszIcon, NULL, countof(szIcon), szIcon, &pszFilePart);
					if (!n || (n >= countof(szIcon)))
						n = SearchPath(NULL, pszIcon, L".exe", countof(szIcon), szIcon, &pszFilePart);
					if (!n || (n >= countof(szIcon)))
						szIcon[0] = 0;
				}
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
	// Добаляем Tasks, они есть только в Win7+
	if (gnOsVer < 0x601)
		return false;

	// -- т.к. работа с TaskList занимает некоторое время - обновление будет делать только по запросу
	//if (!bForce && !gpSet->isStoreTaskbarkTasks && !gpSet->isStoreTaskbarCommands)
	if (!bForce)
		return false; // сохранять не просили

	bool bSucceeded = false;

#ifdef __GNUC__
	MBoxA(L"Sorry, UpdateWin7TaskList is not availbale in GCC!");
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
		LPCWSTR pszCommand = gpSet->HistoryGet();
		if (pszCommand)
		{
			while (*pszCommand && (nHistoryCount < countof(pszHistory)))
			{
				// Текущую - к pszCommand не добавляем. Ее в конец
				if (!pszCurCmdTitle || (lstrcmpi(pszCurCmdTitle, pszCommand) != 0))
				{
					pszHistory[nHistoryCount++] = pszCommand;
				}
				pszCommand += _tcslen(pszCommand)+1;
			}
		}

		if (pszCurCmdTitle)
			nHistoryCount++;
	}

	if (gpSet->isStoreTaskbarkTasks)
	{
		int nGroup = 0;
		const Settings::CommandTasks* pGrp = NULL;
		while ((pGrp = gpSet->CmdTaskGet(nGroup++)) && (nTasksCount < countof(pszTasks)))
		{
			if (pGrp->pszName && *pGrp->pszName)
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
									MessageBox(ghOpWnd, L"Taskbar jump list was updated successfully", gpConEmu->GetDefaultTitle(), MB_ICONINFORMATION);
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

#ifdef _DEBUG
void UnitMaskTests()
{
	struct {
		LPCWSTR asFileName, asMask;
		bool Result;
	} Tests[] = {
		{L"FileName.txt", L"FileName.txt", true},
		{L"FileName.txt", L"FileName", false},
		{L"FileName.txt", L"FileName.doc", false},
		{L"FileName.txt", L"*", true},
		{L"FileName.txt", L"FileName*", true},
		{L"FileName.txt", L"FileName*txt", true},
		{L"FileName.txt", L"FileName*.txt", true},
		{L"FileName.txt", L"FileName*e.txt", false},
		{L"FileName.txt", L"File*qqq", false},
		{L"FileName.txt", L"File*txt", true},
		{L"FileName.txt", L"Name*txt", false},
		{NULL}
	};
	bool bCheck;
	for (size_t i = 0; Tests[i].asFileName; i++)
	{
		bCheck = CompareFileMask(Tests[i].asFileName, Tests[i].asMask);
		_ASSERTE(bCheck == Tests[i].Result);
	}
	bCheck = true;
}

void UnitTests()
{
	RConStartArgs::RunArgTests();
	UnitMaskTests();
}
#endif

LONG WINAPI CreateDumpOnException(LPEXCEPTION_POINTERS ExceptionInfo)
{
	wchar_t szFull[1024] = L"";
	DWORD dwErr = CreateDumpForReport(ExceptionInfo, szFull);
	wchar_t szAdd[1200];
	wcscpy_c(szAdd, szFull);
	wcscat_c(szAdd, L"\r\n\r\nPress <Yes> to copy this text to clipboard\r\nand open project web page");

	int nBtn = DisplayLastError(szAdd, dwErr ? dwErr : -1, MB_YESNO|MB_ICONSTOP|MB_SYSTEMMODAL);
	if (nBtn == IDYES)
	{
		CopyToClipboard(szFull);
		gpConEmu->OnInfo_ReportCrash(NULL);
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


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int iMainRc = 0;

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

	/* *** DEBUG PURPOSES */
	gpStartEnv = LoadStartupEnv();
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
				if (*szVal == L'*')
				{
					lstrcpyn(gsDefConFont, szVal+1, countof(gsDefConFont));
				}
				else
				{
					lstrcpyn(gsDefConFont, szVal, countof(gsDefConFont));
				}
			}
			RegCloseKey(hk);
		}
	}


	gpSetCls = new CSettings;
	gpConEmu = new CConEmuMain;
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

	DEBUGTEST(UnitTests());

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
	bool LoadCfgFilePrm = false; TCHAR* LoadCfgFile = NULL;
	bool SaveCfgFilePrm = false; TCHAR* SaveCfgFile = NULL;
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
	//    setParentDisabled = true;
	//}
	//if (gOSVer.dwMajorVersion>=6)
	//{
	//    CheckConIme();
	//}
	//gpSet->InitSettings();
//------------------------------------------------------------------------
///| Parsing the command line |///////////////////////////////////////////
//------------------------------------------------------------------------
	TCHAR* cmdNew = NULL;
	TCHAR *cmdLine = NULL;
	TCHAR *psUnknown = NULL;
	uint  params = 0;
	bool  isScript = false;

	// [OUT] params = (uint)-1, если в первый аргумент не начинается с '/'
	// т.е. комстрока такая "ConEmu.exe c:\tools\far.exe", 
	// а не такая "ConEmu.exe /cmd c:\tools\far.exe", 
	if (!PrepareCommandLine(/*OUT*/cmdLine, /*OUT*/cmdNew, /*OUT*/isScript, /*OUT*/params))
		return 100;

	if (params && params != (uint)-1)
	{
		TCHAR *curCommand = cmdLine;
		TODO("Если первый (после запускаемого файла) аргумент начинается НЕ с '/' - завершить разбор параметров и не заменять '""' на пробелы");
		//uint params; SplitCommandLine(curCommand, &params);
		//if (params < 1) {
		//    curCommand = NULL;
		//}
		// Parse parameters.
		// Duplicated parameters are permitted, the first value is used.
		uint i = 0;

		while (i < params && curCommand && *curCommand)
		{
			bool lbNotFound = false;
			bool lbSwitchChanged = false;

			if (*curCommand == L'-' && curCommand[1] && !wcspbrk(curCommand+1, L"\\//|.&<>:^"))
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
						//BOOL bNeedFree = FALSE;

						//if (*curCommand!=_T('"') && _tcschr(curCommand, _T(' ')))
						//{
						//	TCHAR* psz = (TCHAR*)calloc(_tcslen(curCommand)+3, sizeof(TCHAR));
						//	*psz = _T('"');
						//	_tcscpy(psz+1, curCommand);
						//	_tcscat(psz, _T("\""));
						//	curCommand = psz;
						//}

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
					// сбрость CreateInNewEnvironment для ConMan
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

					BOOL b = CreateProcess(NULL, cmdNew, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
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
					// Этот ключик был придуман для "скрытого" запуска консоли
					// в режиме администратора
					// (т.е. чтобы окно UAC нормально всплывало, но не мелькало консольное окно)
					// Но не получилось, пока требуются хэндлы процесса, а их не получается
					// передать в НЕ приподнятый процесс (исходный ConEmu GUI).

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
					

					b = CreateProcessDemoted(NULL, cmdNew, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL,
							szCurDir, &si, &pi, &nErr);


					if (b)
					{
						SafeCloseHandle(pi.hProcess);
						SafeCloseHandle(pi.hThread);
						return 0;
					}

					// Failed
					DisplayLastError(cmdNew, nErr);
					return 100;
				}
				else if (!klstricmp(curCommand, _T("/multi")))
				{
					MultiConValue = true; MultiConPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/nomulti")))
				{
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
				//Start ADD fontname; by Mors
				else if (!klstricmp(curCommand, _T("/fontfile")) && i + 1 < params)
				{
					bool bOk = false; TCHAR* pszFile = NULL;
					if (!GetCfgParm(i, curCommand, bOk, pszFile, MAX_PATH))
					{
						return 100;
					}
					gpSetCls->RegisterFont(pszFile, TRUE);
				}
				//End ADD fontname; by Mors
				else if (!klstricmp(curCommand, _T("/fs")))
				{
					WindowModeVal = rFullScreen; WindowPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/max")))
				{
					WindowModeVal = rMaximized; WindowPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/min")) || !klstricmp(curCommand, _T("/mintsa")))
				{
					gpConEmu->WindowStartMinimized = true;
					gpConEmu->WindowStartTSA = (klstricmp(curCommand, _T("/mintsa")) == 0);
				}
				else if (!klstricmp(curCommand, _T("/tsa")) || !klstricmp(curCommand, _T("/tray")))
				{
					gpConEmu->ForceMinimizeToTray = true;
				}
				else if (!klstricmp(curCommand, _T("/detached")))
				{
					gpConEmu->mb_StartDetached = TRUE;
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
						gpConEmu->mps_IconPath = lstrdup(curCommand);
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
				else if (!klstricmp(curCommand, _T("/single")))
				{
					gpSetCls->SingleInstanceArg = sgl_Enabled;
				}
				else if (!klstricmp(curCommand, _T("/nosingle")))
				{
					gpSetCls->SingleInstanceArg = sgl_Disabled;
				}
				else if (!klstricmp(curCommand, _T("/showhide")) || !klstricmp(curCommand, _T("/showhideTSA")))
				{
					gpSetCls->SingleInstanceArg = sgl_Enabled;
					gpSetCls->SingleInstanceShowHide = !klstricmp(curCommand, _T("/showhide"))
						? sih_ShowMinimize : sih_ShowHideTSA;
				}
				else if (!klstricmp(curCommand, _T("/reset")))
				{
					ResetSettings = true;
				}
				//else if ( !klstricmp(curCommand, _T("/DontSetParent")) || !klstricmp(curCommand, _T("/Windows7")) )
				//{
				//    setParentDisabled = true;
				//}
				//else if ( !klstricmp(curCommand, _T("/SetParent")) )
				//{
				//    gpConEmu->setParent = true;
				//}
				//else if ( !klstricmp(curCommand, _T("/SetParent2")) )
				//{
				//    gpConEmu->setParent = true; gpConEmu->setParent2 = true;
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
				else if (!klstricmp(curCommand, _T("/SetDefTerm")))
				{
					SetUpDefaultTerminal = true;
				}
				else if (!klstricmp(curCommand, _T("/Exit")))
				{
					ExitAfterActionPrm = true;
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
				else if (!klstricmp(curCommand, _T("/?")) || !klstricmp(curCommand, _T("/h")) || !klstricmp(curCommand, _T("/help")))
				{
					//MessageBox(NULL, pHelp, L"About ConEmu...", MB_ICONQUESTION);
					gpConEmu->OnInfo_About();
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
		gpConEmu->CreateLog();
	}

	//if (setParentDisabled && (gpConEmu->setParent || gpConEmu->setParent2)) {
	//    gpConEmu->setParent=false; gpConEmu->setParent2=false;
	//}

	if (psUnknown)
	{
		TCHAR* psMsg = (TCHAR*)calloc(_tcslen(psUnknown)+100,sizeof(TCHAR));
		_tcscpy(psMsg, _T("Unknown switch specified:\r\n"));
		_tcscat(psMsg, psUnknown);
		gpConEmu->LogString(psMsg, false, false);
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
		//_tcscat(gpSet->Config, _T("\\"));
		//_tcscat(gpSet->Config, ConfigVal);
		gpSetCls->SetConfigName(ConfigVal);
	}

	// Сразу инициализировать событие для SingleInstance. Кто первый схватит.
	gpConEmu->isFirstInstance();

	// special config file
	if (LoadCfgFilePrm)
	{
		// При ошибке - не выходим, просто покажем ее пользователю
		gpConEmu->SetConfigFile(LoadCfgFile);
		// Release mem
		SafeFree(LoadCfgFile);
	}

	// preparing settings
	bool bNeedCreateVanilla = false;
	if (ResetSettings)
	{
		gpSet->IsConfigNew = true; // force this config as "new"
	}
	else
	{
		// load settings from registry
		gpSet->LoadSettings(&bNeedCreateVanilla);
	}
	// выполнить дополнительные действия в классе настроек здесь
	gpSetCls->SettingsLoaded(bNeedCreateVanilla, true, cmdNew);

	// Для gpSet->isQuakeStyle - принудительно включается gpSetCls->SingleInstanceArg

	// When "/Palette <name>" is specified
	if (PalettePrm)
	{
		gpSet->PaletteSetActive(PaletteVal);
	}

	gpConEmu->LogString(L"SettingsLoaded");


//------------------------------------------------------------------------
///| Processing actions |/////////////////////////////////////////////////
//------------------------------------------------------------------------
	// gpConEmu->mb_UpdateJumpListOnStartup - Обновить JumpLists
	// SaveCfgFilePrm, SaveCfgFile - сохранить настройки в xml-файл (можно использовать вместе с ResetSettings)
	// ExitAfterActionPrm - сразу выйти после выполнения действий
	// не наколоться бы с isAutoSaveSizePos

	if (gpConEmu->mb_UpdateJumpListOnStartup && ExitAfterActionPrm)
	{
		if (!UpdateWin7TaskList(true/*bForce*/, true/*bNoSuccMsg*/))
		{
			if (!iMainRc) iMainRc = 10;
		}
	}

	// special config file
	if (SaveCfgFilePrm)
	{
		// Сохранять конфиг только если получилось сменить путь (создать файл)
		if (!gpConEmu->SetConfigFile(SaveCfgFile, true/*abWriteReq*/))
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
		if (ExitAfterActionPrm)
		{
			MessageBox(L"'/Exit' switch can not be used together with '/SetDefTerm'!", MB_ICONSTOP);
		}
		else
		{
			gpSet->isSetDefaultTerminal = true;
			gpSet->isRegisterOnOsStartup = true;
		}
	}

	// Actions done
	if (ExitAfterActionPrm)
	{
		goto wrap;
	}


	// Update package was dropped on ConEmu icon?
	if (cmdNew && *cmdNew && (params == (uint)-1))
	{
		wchar_t szPath[MAX_PATH+1];
		LPCWSTR pszCmdLine = cmdNew;
		if (0 == NextArg(&pszCmdLine, szPath))
		{
			if (CConEmuUpdate::IsUpdatePackage(szPath))
			{
				// Чтобы при запуске НОВОЙ версии опять не пошло обновление - грохнуть ком-строку
				SafeFree(gpConEmu->mpsz_ConEmuArgs);

				// Создание скрипта обновления, запуск будет выполнен в деструкторе gpUpd
				CConEmuUpdate::LocalUpdate(szPath);

				// Перейти к завершению процесса и запуску обновления
				goto done;
			}
		}
	}


//------------------------------------------------------------------------
///| Continue normal work mode  |/////////////////////////////////////////
//------------------------------------------------------------------------

	// Если в режиме "Inside" подходящего окна не нашли и юзер отказался от "обычного" режима
	// mh_InsideParentWND инициализируется вызовом InsideFindParent из Settings::LoadSettings()
	if (gpConEmu->mp_Inside && (gpConEmu->mp_Inside->mh_InsideParentWND == INSIDE_PARENT_NOT_FOUND))
	{
		return 100;
	}


	// Проверить наличие необходимых файлов (перенес сверху, чтобы учитывался флажок "Inject ConEmuHk")
	if (!gpConEmu->CheckRequiredFiles())
	{
		return 100;
	}
	
	
	//#pragma message("Win2k: CLEARTYPE_NATURAL_QUALITY")
	//if (ClearTypePrm)
	//    gpSet->LogFont.lfQuality = CLEARTYPE_NATURAL_QUALITY;
	//if (FontPrm)
	//    _tcscpy(gpSet->LogFont.lfFaceName, FontVal);
	//if (SizePrm)
	//    gpSet->LogFont.lfHeight = SizeVal;
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

	if (gpSetCls->IsMulti() || StrStrI(gpSetCls->GetCmd(), L"conman.exe"))
	{
		//gpSet->isUpdConHandle = TRUE;

		// сбрость CreateInNewEnvironment для ConMan
		ResetConman();
	}

	// Установка параметров из командной строки
	if (cmdNew)
	{
		MCHKHEAP
		const wchar_t* pszDefCmd = NULL;
		wchar_t* pszReady = NULL;

		if (isScript)
		{
			pszReady = lstrdup(cmdNew);
			if (!pszReady)
			{
				MBoxAssert(pszReady!=NULL);
				return 100;
			}
		}
		else
		{
			int nLen = _tcslen(cmdNew)+8;

			// params = (uint)-1, если в первый аргумент не начинается с '/'
			// т.е. комстрока такая "ConEmu.exe c:\tools\far.exe"
			if ((params == (uint)-1)
				&& (gpSet->nStartType == 0)
				&& (gpSet->psStartSingleApp && *gpSet->psStartSingleApp))
			{
				// В psStartSingleApp может быть прописан путь к фару.
				// Тогда, если в проводнике набросили, например, txt файл
				// на иконку ConEmu, этот наброшенный путь прилепится
				// к строке запуска фара.
				pszDefCmd = gpSet->psStartSingleApp;
				wchar_t szExe[MAX_PATH+1];
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

			pszReady = (TCHAR*)malloc(nLen*sizeof(TCHAR));
			if (!pszReady)
			{
				MBoxAssert(pszReady!=NULL);
				return 100;
			}
			

			if (pszDefCmd)
			{
				lstrcpy(pszReady, pszDefCmd);
				lstrcat(pszReady, L" ");
				lstrcat(pszReady, SkipNonPrintable(cmdNew));

				if (pszReady[0] != L'/' && pszReady[0] != L'-')
				{
					// Запомним в истории!
					gpSet->HistoryAdd(pszReady);
				}
			}
			else if (params == (uint)-1)
			{
				*pszReady = DropLnkPrefix; // Признак того, что это передача набрасыванием на ярлык
				lstrcpy(pszReady+1, SkipNonPrintable(cmdNew));

				if (pszReady[1] != L'/' && pszReady[1] != L'-')
				{
					// Запомним в истории!
					gpSet->HistoryAdd(pszReady+1);
				}
			}
			else
			{
				lstrcpy(pszReady, SkipNonPrintable(cmdNew));

				if (pszReady[0] != L'/' && pszReady[0] != L'-')
				{
					// Запомним в истории!
					gpSet->HistoryAdd(pszReady);
				}
			}
		}

		MCHKHEAP

		// Запоминаем
		gpSetCls->SetCurCmd(pszReady, isScript);
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

	gpConEmu->LogString(L"Registering fonts");

	gpSetCls->RegisterFonts();
	gpSetCls->InitFont(
	    FontPrm ? FontVal : NULL,
	    SizePrm ? SizeVal : -1,
	    ClearTypePrm ? ClearTypeVal : -1
	);

///////////////////////////////////

	// Нет смысла проверять и искать, если наш экземпляр - первый.
	if (gpSetCls->IsSingleInstanceArg() && !gpConEmu->isFirstInstance())
	{
		gpConEmu->LogString(L"SingleInstanceArg");

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
			gpConEmu->LogString(L"Waiting for RunSingleInstance");

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

	gpConEmu->LogString(L"gpConEmu->Init");

	// Тут загружаются иконки, Affinity, и т.п.
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

	if (!gpConEmu->CreateMainWindow())
	{
		free(cmdLine);
		//delete pVCon;
		return 100;
	}

//------------------------------------------------------------------------
///| Misc |///////////////////////////////////////////////////////////////
//------------------------------------------------------------------------
	gpConEmu->PostCreate();
//------------------------------------------------------------------------
///| Main message loop |//////////////////////////////////////////////////
//------------------------------------------------------------------------
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	MessageLoop();

done:
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
	// Нельзя. Еще живут глобальные объекты
	//HeapDeinitialize();
	return iMainRc;
}
