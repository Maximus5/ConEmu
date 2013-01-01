
/*
Copyright (c) 2009-2012 Maximus5
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
#include <dbghelp.h>
#ifndef __GNUC__
#include <shobjidl.h>
#include <propkey.h>
#else
//
#endif
#include "../common/ConEmuCheck.h"
#include "../common/execute.h"
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
HWND ghWnd=NULL, ghWndWork=NULL, ghWndApp=NULL;
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
wchar_t gsDefGuiFont[32] = L"Lucida Console"; // gbIsWine ? L"Liberation Mono" : L"Lucida Console"
wchar_t gsDefConFont[32] = L"Lucida Console"; // DBCS ? L"Liberation Mono" : L"Lucida Console"


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

wchar_t* SelectFile(LPCWSTR asTitle, LPCWSTR asDefFile /*= NULL*/, HWND hParent /*= ghWnd*/, LPCWSTR asFilter /*= NULL*/, bool abAutoQuote /*= true*/, bool bCygwin /*= false*/)
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
	            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn))
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


wchar_t* lstrmerge(LPCWSTR asStr1, LPCWSTR asStr2, LPCWSTR asStr3 /*= NULL*/, LPCWSTR asStr4 /*= NULL*/)
{
	size_t cchMax = 1;
	const size_t Count = 4;
	size_t cch[Count] = {};
	LPCWSTR pszStr[Count] = {asStr1, asStr2, asStr3, asStr4};

	for (size_t i = 0; i < Count; i++)
	{
		cch[i] = pszStr[i] ? lstrlen(pszStr[i]) : 0;
		cchMax += cch[i];
	}

	wchar_t* pszRet = (wchar_t*)malloc(cchMax*sizeof(*pszRet));
	if (!pszRet)
		return NULL;
	*pszRet = 0;
	wchar_t* psz = pszRet;

	for (size_t i = 0; i < Count; i++)
	{
		if (!cch[i])
			continue;

		_wcscpy_c(psz, cch[i]+1, pszStr[i]);
		psz += cch[i];
	}

	return pszRet;
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
static HWND ghLastForegroundWindow = NULL;
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


//BOOL GetFontNameFromFile(LPCTSTR lpszFilePath, LPTSTR rsFontName)
//{
//	typedef struct _tagTT_OFFSET_TABLE{
//		USHORT	uMajorVersion;
//		USHORT	uMinorVersion;
//		USHORT	uNumOfTables;
//		USHORT	uSearchRange;
//		USHORT	uEntrySelector;
//		USHORT	uRangeShift;
//	}TT_OFFSET_TABLE;
//
//	typedef struct _tagTT_TABLE_DIRECTORY{
//		char	szTag[4];			//table name
//		ULONG	uCheckSum;			//Check sum
//		ULONG	uOffset;			//Offset from beginning of file
//		ULONG	uLength;			//length of the table in bytes
//	}TT_TABLE_DIRECTORY;
//
//	typedef struct _tagTT_NAME_TABLE_HEADER{
//		USHORT	uFSelector;			//format selector. Always 0
//		USHORT	uNRCount;			//Name Records count
//		USHORT	uStorageOffset;		//Offset for strings storage, from start of the table
//	}TT_NAME_TABLE_HEADER;
//
//	typedef struct _tagTT_NAME_RECORD{
//		USHORT	uPlatformID;
//		USHORT	uEncodingID;
//		USHORT	uLanguageID;
//		USHORT	uNameID;
//		USHORT	uStringLength;
//		USHORT	uStringOffset;	//from start of storage area
//	}TT_NAME_RECORD;
//
//	#define SWAPWORD(x)		MAKEWORD(HIBYTE(x), LOBYTE(x))
//	#define SWAPLONG(x)		MAKELONG(SWAPWORD(HIWORD(x)), SWAPWORD(LOWORD(x)))
//
//	BOOL lbRc = FALSE;
//	HANDLE f = NULL;
//	wchar_t szRetVal[MAX_PATH];
//	DWORD dwRead;
//
//	//if (f.Open(lpszFilePath, CFile::modeRead|CFile::shareDenyWrite)){
//	if ((f = CreateFile(lpszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL))!=INVALID_HANDLE_VALUE)
//	{
//		TT_OFFSET_TABLE ttOffsetTable;
//		//f.Read(&ttOffsetTable, sizeof(TT_OFFSET_TABLE));
//		if (ReadFile(f, &ttOffsetTable, sizeof(TT_OFFSET_TABLE), &(dwRead=0), NULL) && dwRead)
//		{
//			ttOffsetTable.uNumOfTables = SWAPWORD(ttOffsetTable.uNumOfTables);
//			ttOffsetTable.uMajorVersion = SWAPWORD(ttOffsetTable.uMajorVersion);
//			ttOffsetTable.uMinorVersion = SWAPWORD(ttOffsetTable.uMinorVersion);
//
//			//check is this is a true type font and the version is 1.0
//			if (ttOffsetTable.uMajorVersion != 1 || ttOffsetTable.uMinorVersion != 0)
//				return FALSE;
//
//			TT_TABLE_DIRECTORY tblDir;
//			BOOL bFound = FALSE;
//
//			for(int i=0; i< ttOffsetTable.uNumOfTables; i++){
//				//f.Read(&tblDir, sizeof(TT_TABLE_DIRECTORY));
//				if (ReadFile(f, &tblDir, sizeof(TT_TABLE_DIRECTORY), &(dwRead=0), NULL) && dwRead)
//				{
//					//strncpy(szRetVal, tblDir.szTag, 4); szRetVal[4] = 0;
//					//if (lstrcmpi(szRetVal, L"name") == 0)
//					//if (memcmp(tblDir.szTag, "name", 4) == 0)
//					if (strnicmp(tblDir.szTag, "name", 4) == 0)
//					{
//						bFound = TRUE;
//						tblDir.uLength = SWAPLONG(tblDir.uLength);
//						tblDir.uOffset = SWAPLONG(tblDir.uOffset);
//						break;
//					}
//				}
//			}
//
//			if (bFound){
//				if (SetFilePointer(f, tblDir.uOffset, NULL, FILE_BEGIN)!=INVALID_SET_FILE_POINTER)
//				{
//					TT_NAME_TABLE_HEADER ttNTHeader;
//					//f.Read(&ttNTHeader, sizeof(TT_NAME_TABLE_HEADER));
//					if (ReadFile(f, &ttNTHeader, sizeof(TT_NAME_TABLE_HEADER), &(dwRead=0), NULL) && dwRead)
//					{
//						ttNTHeader.uNRCount = SWAPWORD(ttNTHeader.uNRCount);
//						ttNTHeader.uStorageOffset = SWAPWORD(ttNTHeader.uStorageOffset);
//						TT_NAME_RECORD ttRecord;
//						bFound = FALSE;
//
//						for(int i=0; i<ttNTHeader.uNRCount; i++){
//							//f.Read(&ttRecord, sizeof(TT_NAME_RECORD));
//							if (ReadFile(f, &ttRecord, sizeof(TT_NAME_RECORD), &(dwRead=0), NULL) && dwRead)
//							{
//								ttRecord.uNameID = SWAPWORD(ttRecord.uNameID);
//								if (ttRecord.uNameID == 1){
//									ttRecord.uStringLength = SWAPWORD(ttRecord.uStringLength);
//									ttRecord.uStringOffset = SWAPWORD(ttRecord.uStringOffset);
//									//int nPos = f.GetPosition();
//									DWORD nPos = SetFilePointer(f, 0, 0, FILE_CURRENT);
//									//f.Seek(tblDir.uOffset + ttRecord.uStringOffset + ttNTHeader.uStorageOffset, CFile::begin);
//									if (SetFilePointer(f, tblDir.uOffset + ttRecord.uStringOffset + ttNTHeader.uStorageOffset, 0, FILE_BEGIN)!=INVALID_SET_FILE_POINTER)
//									{
//										if ((ttRecord.uStringLength + 1) < 33)
//										{
//											//f.Read(csTemp.GetBuffer(ttRecord.uStringLength + 1), ttRecord.uStringLength);
//											//csTemp.ReleaseBuffer();
//											char szName[MAX_PATH]; szName[ttRecord.uStringLength + 1] = 0;
//											if (ReadFile(f, szName, ttRecord.uStringLength + 1, &(dwRead=0), NULL) && dwRead)
//											{
//												//if (csTemp.GetLength() > 0){
//												if (szName[0]) {
//													szName[ttRecord.uStringLength + 1] = 0;
//													for (int j = ttRecord.uStringLength; j >= 0 && szName[j] == ' '; j--)
//														szName[j] = 0;
//													if (szName[0]) {
//														MultiByteToWideChar(CP_ACP, 0, szName, -1, szRetVal, 32);
//														szRetVal[31] = 0;
//														lbRc = TRUE;
//													}
//													break;
//												}
//											}
//										}
//									}
//									//f.Seek(nPos, CFile::begin);
//									SetFilePointer(f, nPos, 0, FILE_BEGIN);
//								}
//							}
//						}
//					}
//				}
//			}
//		}
//		CloseHandle(f);
//	}
//	return lbRc;
//}

//BOOL FindFontInFolder(wchar_t* szTempFontFam)
//{
//	BOOL lbRc = FALSE;
//
//	typedef BOOL (WINAPI* FGetFontResourceInfo)(LPCTSTR lpszFilename,LPDWORD cbBuffer,LPVOID lpBuffer,DWORD dwQueryType);
//	FGetFontResourceInfo GetFontResourceInfo = NULL;
//	HMODULE hGdi = LoadLibrary(L"gdi32.dll");
//	if (!hGdi) return FALSE;
//	GetFontResourceInfo = (FGetFontResourceInfo)GetProcAddress(hGdi, "GetFontResourceInfoW");
//	if (!GetFontResourceInfo) return FALSE;
//
//	WIN32_FIND_DATA fnd;
//	wchar_t szFind[MAX_PATH]; wcscpy(szFind, gpConEmu->ms_ConEmuExe);
//	wchar_t *pszSlash = wcsrchr(szFind, L'\\');
//
//	if (pszSlash) {
//		wcscpy(pszSlash, L"\\*.ttf");
//		HANDLE hFind = FindFirstFile(szFind, &fnd);
//		if (hFind != INVALID_HANDLE_VALUE) {
//			do {
//				if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
//					lbRc = TRUE; break;
//				}
//			} while (FindNextFile(hFind, &fnd));
//			FindClose(hFind);
//		}
//
//		if (lbRc) {
//			lbRc = FALSE;
//			pszSlash[1] = 0;
//			if ((_tcslen(fnd.cFileName)+_tcslen(szFind)) >= MAX_PATH) {
//				TCHAR* psz=(TCHAR*)calloc(_tcslen(fnd.cFileName)+100,sizeof(TCHAR));
//				lstrcpyW(psz, L"Too long full pathname for font:\n");
//				lstrcatW(psz, fnd.cFileName);
//				MessageBox(NULL, psz, gpConEmu->GetDefaultTitle(), MB_OK|MB_ICONSTOP);
//				free(psz);
//			} else {
//				wcscat(szFind, fnd.cFileName);
//				// Теперь нужно определить имя шрифта
//				DWORD dwSize = MAX_PATH;
//				//if (!AddFontResourceEx(szFind, FR_PRIVATE, NULL)) //ADD fontname; by Mors
//				//{
//				//	TCHAR* psz=(TCHAR*)calloc(_tcslen(szFind)+100,sizeof(TCHAR));
//				//	lstrcpyW(psz, L"Can't register font:\n");
//				//	lstrcatW(psz, szFind);
//				//	MessageBox(NULL, psz, gpConEmu->GetDefaultTitle(), MB_OK|MB_ICONSTOP);
//				//	free(psz);
//				//} else
//				//if (!GetFontResourceInfo(szFind, &dwSize, szTempFontFam, 1)) {
//				if (!gpSet->GetFontNameFromFile(szFind, szTempFontFam)) {
//					DWORD dwErr = GetLastError();
//					TCHAR* psz=(TCHAR*)calloc(_tcslen(szFind)+100,sizeof(TCHAR));
//					lstrcpyW(psz, L"Can't query font family for file:\n");
//					lstrcatW(psz, szFind);
//					wsprintf(psz+_tcslen(psz), L"\nErrCode=0x%08X", dwErr);
//					MessageBox(NULL, psz, gpConEmu->GetDefaultTitle(), MB_OK|MB_ICONSTOP);
//					free(psz);
//				} else {
//					lstrcpynW(gpSet->FontFile, szFind, countof(gpSet->FontFile));
//					lbRc = TRUE;
//				}
//			}
//		}
//	}
//
//	return lbRc;
//}

//extern void SetConsoleFontSizeTo(HWND inConWnd, int inSizeX, int inSizeY);

// Disables the IME for all threads in a current process.
//void DisableIME()
//{
//	typedef BOOL (WINAPI* ImmDisableIMEt)(DWORD idThread);
//	BOOL lbDisabled = FALSE;
//	HMODULE hImm32 = LoadLibrary(_T("imm32.dll"));
//	if (hImm32) {
//		ImmDisableIMEt ImmDisableIMEf = (ImmDisableIMEt)GetProcAddress(hImm32, "ImmDisableIME");
//		if (ImmDisableIMEf) {
//			lbDisabled = ImmDisableIMEf(-1);
//		}
//	}
//	return;
//}

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
BOOL PrepareCommandLine(TCHAR*& cmdLine, TCHAR*& cmdNew, uint& params)
{
	params = 0;
	cmdNew = NULL;
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
			cmdNew = wcsstr(cmdLine, L"/cmd");

			if (cmdNew)
			{
				*cmdNew = 0;
				cmdNew += 5;
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

bool GetCfgParm(uint& i, TCHAR*& curCommand, bool& Prm, TCHAR*& Val, int nMaxLen)
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

LONG WINAPI CreateDumpOnException(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	const wchar_t *pszError = NULL; DWORD dwErr = 0;
	INT_PTR nLen;
	wchar_t szFull[1024] = L"";
	MINIDUMP_TYPE dumpType = MiniDumpWithFullMemory;
	bool bDumpSucceeded = false;
	HANDLE hDmpFile = NULL;
	HMODULE hDbghelp = NULL;
	wchar_t dmpfile[MAX_PATH+64] = L"";
	typedef BOOL (WINAPI* MiniDumpWriteDump_t)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
	        PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	        PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
	MiniDumpWriteDump_t MiniDumpWriteDump_f = NULL;

	dwErr = SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, dmpfile);
	if (FAILED(dwErr))
	{
		pszError = L"CreateDumpOnException called, get desktop folder failed";
		goto wrap;
	}
	if (*dmpfile && dmpfile[_tcslen(dmpfile)-1] != L'\\')
		wcscat_c(dmpfile, L"\\");
	wcscat_c(dmpfile, L"ConEmuTrap");
	CreateDirectory(dmpfile, NULL);
	nLen = _tcslen(dmpfile);
	_wsprintf(dmpfile+nLen, SKIPLEN(countof(dmpfile)-nLen) L"\\Trap-%02u%02u%02u%s-%u.dmp", MVV_1, MVV_2, MVV_3,_T(MVV_4a), GetCurrentProcessId());
	
	hDmpFile = CreateFileW(dmpfile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH, NULL);
	if (hDmpFile == INVALID_HANDLE_VALUE)
	{
		pszError = L"CreateDumpOnException called, create dump file failed";
		goto wrap;
	}


	hDbghelp = LoadLibraryW(L"Dbghelp.dll");
	if (hDbghelp == NULL)
	{
		pszError = L"CreateDumpOnException called, LoadLibrary(Dbghelp.dll) failed";
		goto wrap;
	}

	MiniDumpWriteDump_f = (MiniDumpWriteDump_t)GetProcAddress(hDbghelp, "MiniDumpWriteDump");
	if (!MiniDumpWriteDump_f)
	{
		pszError = L"CreateDumpOnException called, MiniDumpWriteDump not found";
		goto wrap;
	}
	else
	{
		MINIDUMP_EXCEPTION_INFORMATION mei = {GetCurrentThreadId()};
		mei.ExceptionPointers = ExceptionInfo;
		mei.ClientPointers = FALSE;
		BOOL lbDumpRc = MiniDumpWriteDump_f(
		                    GetCurrentProcess(), GetCurrentProcessId(),
		                    hDmpFile,
		                    dumpType,
		                    &mei,
		                    NULL, NULL);

		if (!lbDumpRc)
		{
			pszError = L"CreateDumpOnException called, MiniDumpWriteDump failed";
			goto wrap;
		}
	}

#if 0
	wchar_t szCmdLine[MAX_PATH*2] = L"\"";
	wchar_t *pszSlash;
	INT_PTR nLen;
	PROCESS_INFORMATION pi = {};
	STARTUPINFO si = {sizeof(si)};

	if (!GetModuleFileName(NULL, szCmdLine+1, MAX_PATH))
	{
		pszError = L"CreateDumpOnException called, GetModuleFileName failed";
		goto wrap;
	}
	if ((pszSlash = wcsrchr(szCmdLine, L'\\')) == NULL)
	{
		pszError = L"CreateDumpOnException called, wcsrchr failed";
		goto wrap;
	}
	_wcscpy_c(pszSlash+1, 30, WIN3264TEST(L"ConEmu\\ConEmuC.exe",L"ConEmu\\ConEmuC64.exe"));
	if (!FileExists(szCmdLine+1))
	{
		_wcscpy_c(pszSlash+1, 30, WIN3264TEST(L"ConEmuC.exe",L"ConEmuC64.exe"));
		if (!FileExists(szCmdLine+1))
		{
			if (gpConEmu)
			{
				_wsprintf(szCmdLine, SKIPLEN(countof(szCmdLine)) L"\"%s\\%s", gpConEmu->ms_ConEmuBaseDir, WIN3264TEST(L"ConEmuC.exe",L"ConEmuC64.exe"));
			}
			if (!FileExists(szCmdLine+1))
			{
				pszError = L"CreateDumpOnException called, " WIN3264TEST(L"ConEmuC.exe",L"ConEmuC64.exe") L" not found";
				goto wrap;
			}
		}
	}

	nLen = _tcslen(szCmdLine);
	_wsprintf(szCmdLine+nLen, SKIPLEN(countof(szCmdLine)-nLen) L"\" /DEBUGPID=%u /FULL", GetCurrentProcessId());

	if (!CreateProcess(NULL, szCmdLine, NULL, NULL, TRUE, HIGH_PRIORITY_CLASS, NULL, NULL, &si, &pi))
	{
		pszError = L"CreateDumpOnException called, CreateProcess failed";
		goto wrap;
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
#endif

wrap:
	if (pszError)
	{
		lstrcpyn(szFull, pszError, 255);
		if (dmpfile)
		{
			wcscat_c(szFull, L"\r\n");
			wcscat_c(szFull, dmpfile);
		}
		if (!dwErr)
			dwErr = GetLastError();
	}
	else
	{
		_wsprintf(szFull, SKIPLEN(countof(szFull)) L"Exception was occurred in ConEmu %02u%02u%02u%s\r\n\r\n"
			L"Memory dump was saved to\r\n%s\r\n\r\n"
			L"Please Zip it and send to developer\r\n"
			L"http://code.google.com/p/conemu-maximus5/issues/entry",
			(MVV_1%100),MVV_2,MVV_3,_T(MVV_4a), dmpfile);
	}
	if (hDmpFile != INVALID_HANDLE_VALUE && hDmpFile != NULL)
	{
		CloseHandle(hDmpFile);
	}
	if (hDbghelp)
	{
		FreeLibrary(hDbghelp);
	}
	DisplayLastError(szFull, dwErr);
	return EXCEPTION_EXECUTE_HANDLER;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int iMainRc = 0;

	if (!IsDebuggerPresent())
	{
		SetUnhandledExceptionFilter(CreateDumpOnException);
	}

	_ASSERTE(sizeof(CESERVER_REQ_STARTSTOPRET) <= sizeof(CESERVER_REQ_STARTSTOP));
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
		wcscpy_c(gsDefGuiFont, L"Liberation Mono");

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
	gpSetCls->SingleInstanceArg = false;
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

	// [OUT] params = (uint)-1, если в первый аргумент не начинается с '/'
	// т.е. комстрока такая "ConEmu.exe c:\tools\far.exe", 
	// а не такая "ConEmu.exe /cmd c:\tools\far.exe", 
	if (!PrepareCommandLine(/*OUT*/cmdLine, /*OUT*/cmdNew, /*OUT*/params))
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

										while((hEmu = FindWindowEx(NULL, hEmu, VirtualConsoleClassMain, NULL)) != NULL)
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
						pszSyncFmt = curCommand; // \eCD /d %1 - \e - ESC, \b - BS, \n - ENTER, %1 - "dir", %2 - "bash dir"
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
							SetCurrentDirectory(pszExpand);
							SafeFree(pszExpand);
						}
						else
						{
							SetCurrentDirectory(curCommand);
						}
						gpConEmu->RefreshConEmuCurDir();
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
					gpSetCls->SingleInstanceArg = true;
				}
				else if (!klstricmp(curCommand, _T("/showhide")) || !klstricmp(curCommand, _T("/showhideTSA")))
				{
					gpSetCls->SingleInstanceArg = true;
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
					if (!GetCfgParm(i, curCommand, LoadCfgFilePrm, LoadCfgFile, MAX_PATH))
					{
						return 100;
					}
				}
				else if (!klstricmp(curCommand, _T("/SaveCfgFile")) && i + 1 < params)
				{
					// используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, SaveCfgFilePrm, SaveCfgFile, MAX_PATH))
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
	}

	// preparing settings
	if (!ResetSettings)
	{
		// load settings from registry
		gpSet->LoadSettings();
	}
	else
	{
		// раз загрузка не выполнялась, выполнить дополнительные действия в классе настроек здесь
		gpSetCls->SettingsLoaded();
	}
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
		gpSet->isMulti = MultiConValue;

	if (VisValue)
		gpSet->isConVisible = VisPrm;

	// Если запускается conman (нафига?) - принудительно включить флажок "Обновлять handle"
	//TODO("Deprecated: isUpdConHandle использоваться не должен");

	if (gpSet->isMulti || StrStrI(gpSet->GetCmd(), L"conman.exe"))
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

		MCHKHEAP
		SafeFree(gpSet->psCurCmd); // могло быть создано в gpSet->GetCmd()
		gpSet->psCurCmd = pszReady; pszReady = NULL;
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
	if (gpSetCls->SingleInstanceArg && !gpConEmu->isFirstInstance())
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
