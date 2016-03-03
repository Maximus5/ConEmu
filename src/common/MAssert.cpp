
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
#include <windows.h>
#include <tchar.h>
#include "defines.h"
#include "MAssert.h"
#include "MStrSafe.h"
#include "Common.h"
#include "ConEmuCheck.h"
#ifdef ASSERT_PIPE_ALLOWED
#include "ConEmuCheck.h"
#endif
#ifdef _DEBUG
#include <ShlObj.h>
#include "../ConEmu/version.h"
#endif

#ifdef _DEBUG
static bool gbMyAssertSkip = false;
static bool MyAssertSkip(const wchar_t* pszFile, int nLine, const wchar_t* pszTest, bool abNoPipe)
{
	return gbMyAssertSkip; // just for breakpoints
}
#endif

AppMsgBox_t AssertMsgBox = NULL;

#ifdef _DEBUG
bool gbAllowChkHeap = false;
#endif

//typedef bool(*HooksUnlockerProc_t)(bool bUnlock);
HooksUnlockerProc_t gfnHooksUnlockerProc = NULL;

#ifdef _DEBUG

LONG CHooksUnlocker::mn_LockCount = 0;
bool CHooksUnlocker::mb_Processed = false;

CHooksUnlocker::CHooksUnlocker()
{
	mb_WasDebugger = (IsDebuggerPresent() != FALSE);

	if (InterlockedIncrement(&mn_LockCount) > 0)
	{
		if (gfnHooksUnlockerProc && !mb_Processed)
		{
			mb_Processed = gfnHooksUnlockerProc(true);
		}
	}
};

CHooksUnlocker::~CHooksUnlocker()
{
	if (InterlockedDecrement(&mn_LockCount) <= 0)
	{
		if (mb_Processed && !mb_WasDebugger
			&& IsDebuggerPresent())
		{
			// To avoid keyboard lags under VS debugger
			mb_Processed = false;
		}
		else if (mb_Processed)
		{
			mb_Processed = false;
			if (gfnHooksUnlockerProc)
			{
				gfnHooksUnlockerProc(false);
			}
		}
	}
};

#include "WThreads.h"

LONG gnInMyAssertTrap = 0;
LONG gnInMyAssertPipe = 0;
//bool gbAllowAssertThread = false;
CEAssertMode gAllowAssertThread = am_Default;
HANDLE ghInMyAssertTrap = NULL;
DWORD gnInMyAssertThread = 0;
#ifdef ASSERT_PIPE_ALLOWED
extern HWND ghConEmuWnd;
#endif

DWORD WINAPI MyAssertThread(LPVOID p)
{
	if (gnInMyAssertTrap > 0)
	{
		// Если уже в трапе - то повторно не вызывать, иначе может вывалиться серия окон, что затруднит отладку
		return IDCANCEL;
	}

	MyAssertInfo* pa = (MyAssertInfo*)p;

	if (ghInMyAssertTrap == NULL)
	{
		ghInMyAssertTrap = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	gnInMyAssertThread = GetCurrentThreadId();
	if (ghInMyAssertTrap)
		ResetEvent(ghInMyAssertTrap);
	InterlockedIncrement(&gnInMyAssertTrap);

	DWORD nRc = 
		#if defined(CONEMU_MINIMAL) && defined(ASSERT_PIPE_ALLOWED)
			GuiMessageBox(ghConEmuWnd, pa->szDebugInfo, pa->szTitle, MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_RETRYCANCEL);
		#else
			AssertMsgBox ? AssertMsgBox(pa->szDebugInfo, MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_RETRYCANCEL|MB_DEFBUTTON2, pa->szTitle, NULL, false) :
			MessageBox(NULL, pa->szDebugInfo, pa->szTitle, MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_RETRYCANCEL|MB_DEFBUTTON2);
		#endif
			

	InterlockedDecrement(&gnInMyAssertTrap);
	if (ghInMyAssertTrap)
		SetEvent(ghInMyAssertTrap);
	gnInMyAssertThread = 0;
	return nRc;
}
void MyAssertTrap()
{
	HooksUnlocker;
	_CrtDbgBreak();
}
void MyAssertDumpToFile(const wchar_t* pszFile, int nLine, const wchar_t* pszTest)
{
	wchar_t dmpfile[MAX_PATH+64] = L"", szVer4[8] = L"", szLine[64];

	typedef HRESULT (WINAPI* SHGetFolderPath_t)(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath);
	HMODULE hShell = LoadLibrary(L"shell32.dll");
	SHGetFolderPath_t shGetFolderPath = hShell ? (SHGetFolderPath_t)GetProcAddress(hShell, "SHGetFolderPathW") : NULL;
	HRESULT hrc = shGetFolderPath ? shGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0/*SHGFP_TYPE_CURRENT*/, dmpfile) : E_FAIL;
	if (hShell) FreeLibrary(hShell);
	if (FAILED(hrc))
	{
		memset(dmpfile, 0, sizeof(dmpfile));
		if (!GetTempPath(MAX_PATH, dmpfile) || !*dmpfile)
		{
			//pszError = L"CreateDumpForReport called, get desktop or temp folder failed";
			return;
		}
	}

	wcscat_c(dmpfile, (*dmpfile && dmpfile[lstrlen(dmpfile)-1] != L'\\') ? L"\\ConEmuTrap" : L"ConEmuTrap");
	CreateDirectory(dmpfile, NULL);

	int nLen = lstrlen(dmpfile);
	lstrcpyn(szVer4, _T(MVV_4a), countof(szVer4));
	static LONG snAutoIndex = 0;
	LONG nAutoIndex = InterlockedIncrement(&snAutoIndex);
	msprintf(dmpfile+nLen, countof(dmpfile)-nLen, L"\\Assert-%02u%02u%02u%s-%u-%u.txt", MVV_1, MVV_2, MVV_3, szVer4, GetCurrentProcessId(), nAutoIndex);

	HANDLE hFile = CreateFile(dmpfile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		//pszError = L"Failed to create dump file";
		return;
	}

	DWORD nWrite;

	msprintf(szLine, countof(szLine), L"CEAssert PID=%u TID=%u\r\nAssertion in ", GetCurrentProcessId(), GetCurrentThreadId());
	nWrite = lstrlen(szLine)*2;
	WriteFile(hFile, szLine, nWrite, &nWrite, NULL);
	if (!GetModuleFileNameW(NULL, dmpfile, countof(dmpfile)-2))
		lstrcpyn(dmpfile, L"<unknown>\r\n", countof(dmpfile));
	else
		wcscat_c(dmpfile, L"\r\n");
	nWrite = lstrlen(dmpfile)*2;
	WriteFile(hFile, dmpfile, nWrite, &nWrite, NULL);

	// File.cpp: 123\r\n
	if (pszFile)
	{
		nWrite = lstrlen(pszFile)*2;
		WriteFile(hFile, pszFile, nWrite, &nWrite, NULL);
		msprintf(szLine, countof(szLine), L": %i\r\n\r\n", nLine);
		nWrite = lstrlen(szLine)*2;
		WriteFile(hFile, szLine, nWrite, &nWrite, NULL);
	}

	if (pszTest)
	{
		nWrite = lstrlen(pszTest)*2;
		WriteFile(hFile, pszTest, nWrite, &nWrite, NULL);
		WriteFile(hFile, L"\r\n", 4, &nWrite, NULL);
	}

	CloseHandle(hFile);
}
int MyAssertProc(const wchar_t* pszFile, int nLine, const wchar_t* pszTest, bool abNoPipe)
{
	HooksUnlocker;

	#ifdef _DEBUG
	if (MyAssertSkip(pszFile, nLine, pszTest, abNoPipe))
		return 1;
	#endif

	MyAssertDumpToFile(pszFile, nLine, pszTest);

	HANDLE hHeap = GetProcessHeap();
	MyAssertInfo* pa = (MyAssertInfo*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(MyAssertInfo));
	if (!pa)
		return -1;
	wchar_t *szExeName = (wchar_t*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (MAX_PATH+1)*sizeof(wchar_t));
	if (szExeName && !GetModuleFileNameW(NULL, szExeName, MAX_PATH+1)) szExeName[0] = 0;
	pa->bNoPipe = abNoPipe;
	msprintf(pa->szTitle, countof(pa->szTitle), L"CEAssert PID=%u TID=%u", GetCurrentProcessId(), GetCurrentThreadId());
	wchar_t szVer4[2] = WSTRING(MVV_4a);
	msprintf(pa->szDebugInfo, countof(pa->szDebugInfo), L"Assertion in %s [%02u%02u%02u%s]\n%s\n\n%s: %i\n\nPress 'Retry' to trap.",
	                szExeName ? szExeName : L"<HeapAllocFailed>",
					MVV_1, MVV_2, MVV_3, szVer4,
					pszTest ? pszTest : L"", pszFile, nLine);
	DWORD dwCode = 0;

	if (gAllowAssertThread == am_Thread)
	{
		DWORD dwTID;
		HANDLE hThread = apiCreateThread(MyAssertThread, pa, &dwTID, "MyAssertThread");

		if (hThread == NULL)
		{
			dwCode = IDRETRY;
			goto wrap;
		}

		WaitForSingleObject(hThread, INFINITE);
		GetExitCodeThread(hThread, &dwCode);
		CloseHandle(hThread);
		goto wrap;
	}
	
#ifdef ASSERT_PIPE_ALLOWED
#ifdef _DEBUG
	if (!abNoPipe && (gAllowAssertThread == am_Pipe))
	{
		HWND hConWnd = GetConEmuHWND(2);
		HWND hGuiWnd = ghConEmuWnd;

		// -- искать - нельзя. Если мы НЕ в ConEmu - нельзя стучаться в другие копии!!!
		//#ifndef CONEMU_MINIMAL
		//if (hGuiWnd == NULL)
		//{
		//	hGuiWnd = FindWindowEx(NULL, NULL, VirtualConsoleClassMain, NULL);
		//}
		//#endif

		if (hGuiWnd && gnInMyAssertTrap <= 0)
		{
			InterlockedIncrement(&gnInMyAssertTrap);
			InterlockedIncrement(&gnInMyAssertPipe);
			gnInMyAssertThread = GetCurrentThreadId();
			ResetEvent(ghInMyAssertTrap);
			
			dwCode = GuiMessageBox(abNoPipe ? NULL : hGuiWnd, pa->szDebugInfo, pa->szTitle, MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_RETRYCANCEL);
			InterlockedDecrement(&gnInMyAssertTrap);
			InterlockedDecrement(&gnInMyAssertPipe);
			SetEvent(ghInMyAssertTrap);
			gnInMyAssertThread = 0;
			goto wrap;
		}
	}

	while (gnInMyAssertPipe>0 && (gnInMyAssertThread != GetCurrentThreadId()))
	{
		Sleep(250);
	}
#endif
#endif

	// В консольных приложениях попытка запустить CreateThread(MyAssertThread) может зависать
	dwCode = MyAssertThread(pa);

wrap:
	if (pa)
		HeapFree(hHeap, 0, pa);
	if (szExeName)
		HeapFree(hHeap, 0, szExeName);
	return (dwCode == IDRETRY) ? -1 : 1;
}

void MyAssertShutdown()
{
	if (ghInMyAssertTrap != NULL)
	{
		if (gnInMyAssertTrap > 0)
		{
			if (!gnInMyAssertThread)
			{
				_ASSERTE(gnInMyAssertTrap>0 && gnInMyAssertThread);
			}
			else
			{
				HANDLE hHandles[2]; 
				hHandles[0] = OpenThread(SYNCHRONIZE, FALSE, gnInMyAssertThread);
				if (hHandles[0] == NULL)
				{
					_ASSERTE(hHandles[0] != NULL);
				}
				else
				{
					hHandles[1] = ghInMyAssertTrap;
					WaitForMultipleObjects(2, hHandles, FALSE, INFINITE);
					CloseHandle(hHandles[0]);
				}
			}
		}

		CloseHandle(ghInMyAssertTrap);
		ghInMyAssertTrap = NULL;
	}
}

wchar_t gszDbgModLabel[6] = {0};

void _DEBUGSTR(LPCWSTR s)
{
	MCHKHEAP; CHEKCDBGMODLABEL;
	SYSTEMTIME st; GetLocalTime(&st);
	wchar_t szDEBUGSTRTime[1040];
	msprintf(szDEBUGSTRTime, countof(szDEBUGSTRTime), L"%u:%02u:%02u.%03u(%s.%u.%u) ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, gszDbgModLabel, GetCurrentProcessId(), GetCurrentThreadId());
	LPCWSTR psz = s; int nSLen = lstrlen(psz);
	if (nSLen < 999)
	{
		wcscat_c(szDEBUGSTRTime, s);
		if (nSLen && psz[nSLen-1]!=L'\n')
			wcscat_c(szDEBUGSTRTime, L"\n");
		OutputDebugString(szDEBUGSTRTime);
	}
	else
	{
		OutputDebugString(szDEBUGSTRTime);
		OutputDebugString(psz);
		if (nSLen && psz[nSLen-1]!=L'\n')
			OutputDebugString(L"\n");
	}
}
#endif
