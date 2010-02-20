
/*
Copyright (c) 2009-2010 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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

#define DROP_SETCP_ON_WIN2K3R2

// Иначе не опередяется GetConsoleAliases (хотя он должен быть доступен в Win2k)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <TCHAR.h>
#include <Tlhelp32.h>
#include <shlwapi.h>
#include "..\common\common.hpp"
#include "..\common\ConEmuCheck.h"
#include "Injects.h"

#ifdef _DEBUG
	#include <crtdbg.h>
#else
	#ifndef _ASSERTE
		#define _ASSERTE(x)
	#endif
#endif

#define DebugString(s) OutputDebugString(s)


extern DWORD gnMainThreadId;
extern BOOL gbFARuseASCIIsort;
extern DWORD gdwServerPID;

static bool gbConEmuInput = false; // TRUE если консоль спрятана и должна работать через очередь ConEmu

extern CESERVER_REQ_CONINFO_HDR *gpConsoleInfo;

//WARNING("Все SendMessage нужно переделать на PipeExecute, т.к. из 'Run as' в Win7 нифига не пошлется");

static void* GetOriginalAddress( HookItem *apHooks, void* OurFunction, void* DefaultFunction, BOOL abAllowModified );
static HMODULE WINAPI OnLoadLibraryW( const WCHAR* lpFileName );
static HMODULE WINAPI OnLoadLibraryA( const char* lpFileName );
static HMODULE WINAPI OnLoadLibraryExW( const WCHAR* lpFileName, HANDLE hFile, DWORD dwFlags );
static HMODULE WINAPI OnLoadLibraryExA( const char* lpFileName, HANDLE hFile, DWORD dwFlags );

#define ORIGINAL(n) \
	BOOL bMainThread = (GetCurrentThreadId() == gnMainThreadId); \
	void* f##n = GetOriginalAddress(Hooks, On##n, ##n, bMainThread);

static void GuiSetForeground(HWND hWnd)
{
	if (ConEmuHwnd) {
		CESERVER_REQ In, *pOut;
		ExecutePrepareCmd(&In, CECMD_SETFOREGROUND, sizeof(CESERVER_REQ_HDR)+sizeof(u64));
		In.qwData[0] = (u64)hWnd;
		HWND hConWnd = GetConsoleWindow();
		pOut = ExecuteGuiCmd(hConWnd, &In, hConWnd);
		if (pOut) ExecuteFreeResult(pOut);
	}
}

static void GuiFlashWindow(BOOL bSimple, HWND hWnd, BOOL bInvert, DWORD dwFlags, UINT uCount, DWORD dwTimeout)
{
	if (ConEmuHwnd) {
		CESERVER_REQ In, *pOut;
		ExecutePrepareCmd(&In, CECMD_FLASHWINDOW, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_FLASHWINFO));
		In.Flash.bSimple = bSimple;
		In.Flash.hWnd = hWnd;
		In.Flash.bInvert = bInvert;
		In.Flash.dwFlags = dwFlags;
		In.Flash.uCount = uCount;
		In.Flash.dwTimeout = dwTimeout;
		HWND hConWnd = GetConsoleWindow();
		pOut = ExecuteGuiCmd(hConWnd, &In, hConWnd);
		if (pOut) ExecuteFreeResult(pOut);
	}
}

static BOOL SrvSetConsoleCP(BOOL bSetOutputCP, DWORD nCP)
{
	_ASSERTE(ConEmuHwnd);

	BOOL lbRc = FALSE;

	if (gdwServerPID) {
		// Проверить живость процесса
		HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, gdwServerPID);
		if (hProcess) {
			if (WaitForSingleObject(hProcess,0) == WAIT_OBJECT_0)
				gdwServerPID = 0; // Процесс сервера завершился
			CloseHandle(hProcess);
		} else {
			gdwServerPID = 0;
		}
	}

	if (gdwServerPID) {
#ifndef DROP_SETCP_ON_WIN2K3R2
		CESERVER_REQ In, *pOut;
		ExecutePrepareCmd(&In, CECMD_SETCONSOLECP, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETCONCP));
		In.SetConCP.bSetOutputCP = bSetOutputCP;
		In.SetConCP.nCP = nCP;
		HWND hConWnd = GetConsoleWindow();
		pOut = ExecuteSrvCmd(gdwServerPID, &In, hConWnd);
		if (pOut) {
			if (pOut->hdr.nSize >= In.hdr.nSize) {
				lbRc = pOut->SetConCP.bSetOutputCP;
				if (!lbRc)
					SetLastError(pOut->SetConCP.nCP);
			}
			ExecuteFreeResult(pOut);
		}
#else
		lbRc = TRUE;
#endif
	} else {
		if (bSetOutputCP)
			lbRc = SetConsoleOutputCP(nCP);
		else
			lbRc = SetConsoleCP(nCP);
	}

	return lbRc;
}



static BOOL WINAPI OnTrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect)
{
	_ASSERTE(TrackPopupMenu!=OnTrackPopupMenu);
	
	#ifdef _DEBUG
		WCHAR szMsg[128]; wsprintf(szMsg, L"TrackPopupMenu(hwnd=0x%08X)\n", (DWORD)hWnd);
		DebugString(szMsg);
	#endif
	
	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		//UINT nSetFore = RegisterWindowMessage(CONEMUMSG_SETFOREGROUND);
		//DWORD_PTR dwRc = 0;
		//SendMessageTimeout(GetParent(ConEmuHwnd), nSetFore, 0, (LPARAM)hWnd, SMTO_NORMAL, 1000, &dwRc);
		GuiSetForeground(hWnd);
	}
	return TrackPopupMenu(hMenu, uFlags, x, y, nReserved, hWnd, prcRect);
}

static BOOL WINAPI OnTrackPopupMenuEx(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm)
{
	_ASSERTE(OnTrackPopupMenuEx != TrackPopupMenuEx);
	
	#ifdef _DEBUG
		WCHAR szMsg[128]; wsprintf(szMsg, L"TrackPopupMenuEx(hwnd=0x%08X)\n", (DWORD)hWnd);
		DebugString(szMsg);
	#endif
	
	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		//UINT nSetFore = RegisterWindowMessage(CONEMUMSG_SETFOREGROUND);
		//DWORD_PTR dwRc = 0;
		//SendMessageTimeout(GetParent(ConEmuHwnd), nSetFore, 0, (LPARAM)hWnd, SMTO_NORMAL, 1000, &dwRc);
		GuiSetForeground(hWnd);
	}
	return TrackPopupMenuEx(hmenu, fuFlags, x, y, hWnd, lptpm);
}

static BOOL WINAPI OnShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo)
{
	_ASSERTE(OnShellExecuteExA != ShellExecuteExA);
	
	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		if (!lpExecInfo->hwnd || lpExecInfo->hwnd == GetConsoleWindow())
			lpExecInfo->hwnd = GetParent(ConEmuHwnd);
	}
	return ShellExecuteExA(lpExecInfo);
}
static BOOL WINAPI OnShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo)
{
	_ASSERTE(OnShellExecuteExW != ShellExecuteExW);
	
	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		if (!lpExecInfo->hwnd || lpExecInfo->hwnd == GetConsoleWindow())
			lpExecInfo->hwnd = GetParent(ConEmuHwnd);
	}
	return ShellExecuteExW(lpExecInfo);
}

static HINSTANCE WINAPI OnShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd)
{
	_ASSERTE(OnShellExecuteA != ShellExecuteA);
	
	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		if (!hwnd || hwnd == GetConsoleWindow())
			hwnd = GetParent(ConEmuHwnd);
	}
	return ShellExecuteA(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
}
static HINSTANCE WINAPI OnShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	_ASSERTE(OnShellExecuteW != ShellExecuteW);
	
	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		if (!hwnd || hwnd == GetConsoleWindow())
			hwnd = GetParent(ConEmuHwnd);
	}
	return ShellExecuteW(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
}

static BOOL WINAPI OnFlashWindow(HWND hWnd, BOOL bInvert)
{
	_ASSERTE(FlashWindow != OnFlashWindow);

	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		GuiFlashWindow(TRUE, hWnd, bInvert, 0,0,0);
		return TRUE;
		//UINT nFlash = RegisterWindowMessage(CONEMUMSG_FLASHWINDOW);
		//DWORD_PTR dwRc = 0;
		//WPARAM wParam = (bInvert ? 2 : 1) << 25;
		//LRESULT lRc = SendMessageTimeout(GetParent(ConEmuHwnd), nFlash, wParam, (LPARAM)hWnd, SMTO_NORMAL, 1000, &dwRc);
		//return dwRc!=0;
	}
	return FlashWindow(hWnd, bInvert);
}
static BOOL WINAPI OnFlashWindowEx(PFLASHWINFO pfwi)
{
	_ASSERTE(FlashWindowEx != OnFlashWindowEx);

	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		GuiFlashWindow(FALSE, pfwi->hwnd, 0, pfwi->dwFlags, pfwi->uCount, pfwi->dwTimeout);
		return TRUE;
		//UINT nFlash = RegisterWindowMessage(CONEMUMSG_FLASHWINDOW);
		//DWORD_PTR dwRc = 0;
		//WPARAM wParam = ((pfwi->dwFlags & 0xF) << 24) | (pfwi->uCount & 0xFFFFFF);
		//LRESULT lRc = SendMessageTimeout(GetParent(ConEmuHwnd), nFlash, wParam, (LPARAM)pfwi->hwnd, SMTO_NORMAL, 1000, &dwRc);
		//return dwRc!=0;
	}
	return FlashWindowEx(pfwi);
}

static BOOL WINAPI OnSetForegroundWindow(HWND hWnd)
{
	_ASSERTE(SetForegroundWindow != OnSetForegroundWindow);
	
	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		GuiSetForeground(hWnd);
		//UINT nSetFore = RegisterWindowMessage(CONEMUMSG_SETFOREGROUND);
		//DWORD_PTR dwRc = 0;
		//LRESULT lRc = SendMessageTimeout(GetParent(ConEmuHwnd), nSetFore, 0, (LPARAM)hWnd, SMTO_NORMAL, 1000, &dwRc);
		//return lRc!=0;
	}
	
	return SetForegroundWindow(hWnd);
}

#ifndef NORM_STOP_ON_NULL
#define NORM_STOP_ON_NULL 0x10000000
#endif

//static int WINAPI OnCompareStringA(LCID Locale, DWORD dwCmpFlags, LPCSTR lpString1, int cchCount1, LPCSTR lpString2, int cchCount2)
//static int WINAPI OnlstrcmpiA(LPCSTR lpString1, LPCSTR lpString2)
//{
//	_ASSERTE(OnlstrcmpiA!=lstrcmpiA);
//	int nCmp = 0;
//	
//	if (gbFARuseASCIIsort)
//	{
//		char ch1 = *lpString1++, ch10 = 0, ch2 = *lpString2++, ch20 = 0;
//		while (!nCmp && ch1)
//		{
//			if (ch1==ch2) {
//				if (!ch1) break;
//			} else if (ch1<0x80 && ch2<0x80) {
//				if (ch1>='A' && ch1<='Z') ch1 |= 0x20;
//				if (ch2>='A' && ch2<='Z') ch2 |= 0x20;
//				nCmp = (ch1==ch2) ? 0 : (ch1<ch2) ? -1 : 1;
//			} else {
//				nCmp = CompareStringA(LOCALE_USER_DEFAULT, NORM_IGNORECASE|NORM_STOP_ON_NULL|SORT_STRINGSORT, &ch1, 1, &ch2, 1)-2;
//			}
//			if (!ch1 || !ch2 || !nCmp) break;
//			ch1 = *lpString1++;
//			ch2 = *lpString2++;
//		}
//	} else {
//		nCmp = lstrcmpiA(lpString1, lpString2);
//	}
//	return nCmp;
//}

static int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2)
{
	_ASSERTE(OnCompareStringW!=CompareStringW);
	int nCmp = -1;
	
	if (gbFARuseASCIIsort)
	{
		//if (dwCmpFlags == (NORM_IGNORECASE|SORT_STRINGSORT) && cchCount1 == -1 && cchCount2 == -1) {
		//	//int n = lstrcmpiW(lpString1, lpString2);
		//	//nCmp = (n<0) ? 1 : (n>0) ? 3 : 2;
		//	
		//} else
		
		if (dwCmpFlags == (NORM_IGNORECASE|NORM_STOP_ON_NULL|SORT_STRINGSORT) /*&& GetCurrentThreadId()==gnMainThreadId*/) {
			//size_t nLen1 = lstrlen(lpString1), nLen2 = lstrlen(lpString2);
			
			//bool bUseWcs = true; // выбор типа сравнивания
			
			//if (bUseWcs) {
				//nCmp = CompareStringW(Locale, dwCmpFlags, lpString1, cchCount1, lpString2, cchCount2);
				int n = 0;
				//WCHAR ch1[2], ch2[2]; ch1[1] = ch2[1] = 0;
				WCHAR ch1 = *lpString1++, ch10 = 0, ch2 = *lpString2++, ch20 = 0;
				int n1 = (cchCount1==cchCount2) ? cchCount1
					: (cchCount1!=-1 && cchCount2!=-1) ? -1 
						: (cchCount1!=-1) ? cchCount2 
							: (cchCount2!=-1) ? cchCount1 : min(cchCount1,cchCount2);
				while (!n && /*(ch1=*lpString1++) && (ch2=*lpString2++) &&*/ n1--)
				{
					if (ch1==ch2) {
						if (!ch1) break;
						// n = 0;
					} else if (ch1<0x80 && ch2<0x80) {
						if (ch1>=L'A' && ch1<=L'Z') ch1 |= 0x20;
						if (ch2>=L'A' && ch2<=L'Z') ch2 |= 0x20;
						n = (ch1==ch2) ? 0 : (ch1<ch2) ? -1 : 1;
					} else {
						n = CompareStringW(Locale, dwCmpFlags, &ch1, 1, &ch2, 1)-2;
					}
					if (!ch1 || !ch2 || !n1) break;
					ch1 = *lpString1++;
					ch2 = *lpString2++;
				}
				//if (!n && ((ch1!=0) ^ (ch2!=0)))
					//n = _wcsnicmp(ch1, ch2, 1);
					//nCmp = lstrcmpiW(ch1, ch2);
					//nCmp = StrCmpNI(lpString1++, lpString2++, 1);
				nCmp = (n<0) ? 1 : (n>0) ? 3 : 2;
			//}
			//nCmp += 2;
			//static WCHAR temp[MAX_PATH+1];
			//size_t nLen1 = lstrlen(lpString1), nLen2 = lstrlen(lpString2);
			//if (nLen1 && nLen2 && nLen1 <= MAX_PATH && nLen2 <= MAX_PATH) {
			//	if (nLen1 == nLen2) {
			//		nCmp = lstrcmpiW(lpString1, lpString2)+2;
			//	} else if (nLen1 < nLen2) {
			//		lstrcpyn(temp, lpString2, nLen1+1);
			//		nCmp = lstrcmpiW(lpString1, temp)+2;
			//	} else if (nLen1 > nLen2) {
			//		lstrcpyn(temp, lpString1, nLen2+1);
			//		nCmp = lstrcmpiW(temp, lpString2)+2;
			//	}
			//}
		}
	}
	
	if (nCmp == -1)
		nCmp = CompareStringW(Locale, dwCmpFlags, lpString1, cchCount1, lpString2, cchCount2);
	return nCmp;
}

static DWORD WINAPI OnGetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName)
{
	_ASSERTE(OnGetConsoleAliasesW!=GetConsoleAliasesW);
	DWORD nError = 0;
	DWORD nRc = GetConsoleAliasesW(AliasBuffer,AliasBufferLength,ExeName);
	if (!nRc) {
		nError = GetLastError();
		// финт ушами
		if (nError == ERROR_NOT_ENOUGH_MEMORY && gdwServerPID) {
			CESERVER_REQ_HDR In;
			ExecutePrepareCmd((CESERVER_REQ*)&In, CECMD_GETALIASES,sizeof(CESERVER_REQ_HDR));
			CESERVER_REQ* pOut = ExecuteSrvCmd(gdwServerPID, (CESERVER_REQ*)&In, GetConsoleWindow());
			if (pOut) {
				DWORD nData = min(AliasBufferLength,(pOut->hdr.nSize-sizeof(pOut->hdr)));
				if (nData) {
					memmove(AliasBuffer, pOut->Data, nData);
					nRc = TRUE;
				}
				ExecuteFreeResult(pOut);
			}
			if (!nRc)
				SetLastError(nError); // вернуть, вдруг какая функция его поменяла
		}
	}
	return nRc;
}

static BOOL WINAPI OnSetConsoleCP(UINT wCodePageID)
{
	_ASSERTE(OnSetConsoleCP!=SetConsoleCP);
	TODO("Виснет в 2k3R2 при 'chcp 866 <enter> chcp 20866 <enter>");
	BOOL lbRc = FALSE;
	if (gdwServerPID) {
		lbRc = SrvSetConsoleCP(FALSE/*bSetOutputCP*/, wCodePageID);
	} else {
		lbRc = SetConsoleCP(wCodePageID);
	}
	return lbRc;
}

static BOOL WINAPI OnSetConsoleOutputCP(UINT wCodePageID)
{
	_ASSERTE(OnSetConsoleOutputCP!=SetConsoleOutputCP);
	BOOL lbRc = FALSE;
	if (gdwServerPID) {
		lbRc = SrvSetConsoleCP(TRUE/*bSetOutputCP*/, wCodePageID);
	} else {
		lbRc = SetConsoleOutputCP(wCodePageID);
	}
	return lbRc;
}

static BOOL WINAPI OnGetNumberOfConsoleInputEvents(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents)
{
	_ASSERTE(OnGetNumberOfConsoleInputEvents!=GetNumberOfConsoleInputEvents);
	if (gpConsoleInfo) {
		if (GetCurrentThreadId() == gnMainThreadId)
			gpConsoleInfo->nFarReadIdx++;
	}
	BOOL lbRc = GetNumberOfConsoleInputEvents(hConsoleInput, lpcNumberOfEvents);
	return lbRc;
}

typedef BOOL (WINAPI* PeekConsoleInput_t)(HANDLE,PINPUT_RECORD,DWORD,LPDWORD);

static void TouchReadPeekConsoleInputs(bool Peek)
{
	_ASSERTE(gpConsoleInfo);
	if (!gpConsoleInfo) return;
	gpConsoleInfo->nFarReadIdx++;

#ifdef _DEBUG
	if ((GetKeyState(VK_SCROLL)&1) == 0)
		return;
	static DWORD nLastTick;
	DWORD nCurTick = GetTickCount();
	DWORD nDelta = nCurTick - nLastTick;
	static CONSOLE_SCREEN_BUFFER_INFO sbi;
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (nDelta > 1000) {
		GetConsoleScreenBufferInfo(hOut, &sbi);
		nCurTick = nCurTick;
	}
	static wchar_t Chars[] = L"-\\|/-\\|/";
	int nNextChar = 0;
	if (Peek) {
		static int nPeekChar = 0;
		nNextChar = nPeekChar++;
		if (nPeekChar >= 8) nPeekChar = 0;
	} else {
		static int nReadChar = 0;
		nNextChar = nReadChar++;
		if (nReadChar >= 8) nReadChar = 0;
	}
	CHAR_INFO chi;
	chi.Char.UnicodeChar = Chars[nNextChar];
	chi.Attributes = 15;
	COORD crBufSize = {1,1};
	COORD crBufCoord = {0,0};
	SMALL_RECT rc = {sbi.srWindow.Left+(Peek?0:1),sbi.srWindow.Bottom,sbi.srWindow.Left+(Peek?0:1),sbi.srWindow.Bottom};
	WriteConsoleOutputW(hOut, &chi, crBufSize, crBufCoord, &rc);
#endif
}

static BOOL WINAPI OnPeekConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	ORIGINAL(PeekConsoleInputA);
	_ASSERTE(OnPeekConsoleInputA!=fPeekConsoleInputA);
	if (gpConsoleInfo && bMainThread)
		TouchReadPeekConsoleInputs(true);
	BOOL lbRc = ((PeekConsoleInput_t)fPeekConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
	return lbRc;
}

static BOOL WINAPI OnPeekConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	ORIGINAL(PeekConsoleInputW);
	_ASSERTE(OnPeekConsoleInputW!=fPeekConsoleInputW);
	if (gpConsoleInfo && bMainThread)
		TouchReadPeekConsoleInputs(true);
	BOOL lbRc = ((PeekConsoleInput_t)fPeekConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
	return lbRc;
}

static BOOL WINAPI OnReadConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	_ASSERTE(OnReadConsoleInputA!=ReadConsoleInputA);
	if (gpConsoleInfo) {
		if (GetCurrentThreadId() == gnMainThreadId) {
			TouchReadPeekConsoleInputs(false);
		}
	}
	BOOL lbRc = ReadConsoleInputA(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
	return lbRc;
}

static BOOL WINAPI OnReadConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	_ASSERTE(OnReadConsoleInputW!=ReadConsoleInputW);
	if (gpConsoleInfo) {
		if (GetCurrentThreadId() == gnMainThreadId) {
			TouchReadPeekConsoleInputs(false);
		}
	}
	BOOL lbRc = ReadConsoleInputW(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
	return lbRc;
}

static HANDLE WINAPI OnCreateConsoleScreenBuffer(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData)
{
	_ASSERTE(OnCreateConsoleScreenBuffer!=CreateConsoleScreenBuffer);
	if ((dwShareMode & (FILE_SHARE_READ|FILE_SHARE_WRITE)) != (FILE_SHARE_READ|FILE_SHARE_WRITE))
		dwShareMode |= (FILE_SHARE_READ|FILE_SHARE_WRITE);
	HANDLE h = CreateConsoleScreenBuffer(dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwFlags, lpScreenBufferData);
	return h;
}



static TCHAR kernel32[] = _T("kernel32.dll");
static TCHAR user32[]   = _T("user32.dll");
static TCHAR shell32[]  = _T("shell32.dll");

//static BOOL bHooksWin2k3R2Only = FALSE;
//static HookItem HooksWin2k3R2Only[] = {
//	{OnSetConsoleCP, "SetConsoleCP", kernel32, 0},
//	{OnSetConsoleOutputCP, "SetConsoleOutputCP", kernel32, 0},
//	/* ************************ */
//	{0, 0, 0}
//};

static HookItem HooksFarOnly[] = {
//	{OnlstrcmpiA,      "lstrcmpiA",      kernel32, 0},
	{OnCompareStringW, "CompareStringW", kernel32, 0},
	/* ************************ */
	{0, 0, 0}
};

static HookItem Hooks[] = {
	// My
	{OnPeekConsoleInputW,	"PeekConsoleInputW",	kernel32, 0},
	{OnPeekConsoleInputA,	"PeekConsoleInputA",	kernel32, 0},
	{OnReadConsoleInputW,	"ReadConsoleInputW",	kernel32, 0},
	{OnReadConsoleInputA,	"ReadConsoleInputA",	kernel32, 0},
    {OnLoadLibraryA,        "LoadLibraryA",			kernel32, 0},
	{OnLoadLibraryW,        "LoadLibraryW",			kernel32, 0},
    {OnLoadLibraryExA,      "LoadLibraryExA",		kernel32, 0},
	{OnLoadLibraryExW,      "LoadLibraryExW",		kernel32, 0},
	{OnGetConsoleAliasesW,  "GetConsoleAliasesW",	kernel32, 0},
	{OnGetNumberOfConsoleInputEvents,
							"GetNumberOfConsoleInputEvents",
													kernel32, 0},
	{OnCreateConsoleScreenBuffer,
							"CreateConsoleScreenBuffer",
													kernel32, 0},
	{OnTrackPopupMenu,      "TrackPopupMenu",		user32,   0},
	{OnTrackPopupMenuEx,    "TrackPopupMenuEx",		user32,   0},
	{OnFlashWindow,         "FlashWindow",			user32,   0},
	{OnFlashWindowEx,       "FlashWindowEx",		user32,   0},
	{OnSetForegroundWindow, "SetForegroundWindow",	user32,   0},
	{OnShellExecuteExA,     "ShellExecuteExA",		shell32,  0},
	{OnShellExecuteExW,     "ShellExecuteExW",		shell32,  0},
	{OnShellExecuteA,       "ShellExecuteA",		shell32,  0},
	{OnShellExecuteW,       "ShellExecuteW",		shell32,  0},
	/* ************************ */
	{0, 0, 0}
};



static TCHAR* ExcludedModules[] = {
    //_T("conemu.dll"), _T("conemu.x64.dll"), // по идее не нужно - должно по ghPluginModule пропускаться. Но на всякий случай!
    _T("conemui.dll"), _T("conemui.x64.dll"), // по идее не нужно - должно по ghPluginModule пропускаться. Но на всякий случай!
    //_T("kernel32.dll"),
    // а user32.dll не нужно?
    0
};















// Эту функцию нужно позвать из DllMain плагина
BOOL StartupModule()
{
	HMODULE hKernel = GetModuleHandle( kernel32 );
	HMODULE hUser   = GetModuleHandle( user32 );
	HMODULE hShell  = GetModuleHandle( shell32 );
	if (!hShell) hShell = LoadLibrary ( shell32 );
	_ASSERTE(hKernel && hUser && hShell);
	if (!hKernel || !hUser || !hShell)
		return FALSE; // модули должны быть загружены ДО conemu.dll

	OSVERSIONINFOEX osv = {sizeof(OSVERSIONINFOEX)};
	GetVersionEx((LPOSVERSIONINFO)&osv);

	// Заполнить поле HookItem.OldAddress (реальные процедуры из внешних библиотек)
	InitHooks( Hooks );
	InitHooks( HooksFarOnly );
	//InitHooks( HooksWin2k3R2Only );

	//  Подменить Импортируемые функции в FAR.exe (пока это только сравнивание строк)
	SetHook( HooksFarOnly, ExcludedModules, NULL, TRUE );
	
	// Windows Server 2003 R2
	
	/*if (osv.dwMajorVersion==5 && osv.dwMinorVersion==2 && osv.wServicePackMajor>=2)
	{
		//DWORD dwBuild = GetSystemMetrics(SM_SERVERR2); // нихрена оно не возвращает. 0 тут :(
		bHooksWin2k3R2Only = TRUE;
		SetHook( HooksWin2k3R2Only, ExcludedModules, NULL );
	}*/
	
	// Подменить Импортируемые функции во всех модулях процесса, загруженных ДО conemu.dll
	SetHookEx( Hooks, ExcludedModules, ghPluginModule );

	// Заменить в модуле Module ЭКСпортируемые функции на подменяемые плагином нихрена
	// НЕ получится, т.к. в Win32 библиотека shell32 может быть загружена ПОСЛЕ conemu.dll
	//   что вызовет некорректные смещения функций,
	// а в Win64 смещения вообще должны быть 64битными, а структура модуля хранит только 32битные смещения
	
	return TRUE;
}

void ShutdownModule()
{
}
