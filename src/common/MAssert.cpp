
/*
Copyright (c) 2009-2014 Maximus5
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
#include "defines.h"
#include "MAssert.h"
#include "MStrSafe.h"
#include "common.hpp"
#include "ConEmuCheck.h"
#ifdef ASSERT_PIPE_ALLOWED
#include "ConEmuCheck.h"
#endif

#ifdef _DEBUG
bool gbInMyAssertTrap = false;
bool gbInMyAssertPipe = false;
//bool gbAllowAssertThread = false;
CEAssertMode gAllowAssertThread = am_Default;
HANDLE ghInMyAssertTrap = NULL;
DWORD gnInMyAssertThread = 0;
#ifdef ASSERT_PIPE_ALLOWED
extern HWND ghConEmuWnd;
#endif
AppMsgBox_t AssertMsgBox = NULL;
DWORD WINAPI MyAssertThread(LPVOID p)
{
	if (gbInMyAssertTrap)
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
	gbInMyAssertTrap = true;
	
	DWORD nRc = 
		#ifdef CONEMU_MINIMAL
			GuiMessageBox(ghConEmuWnd, pa->szDebugInfo, pa->szTitle, MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_RETRYCANCEL);
		#else
			AssertMsgBox ? AssertMsgBox(pa->szDebugInfo, MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_RETRYCANCEL, pa->szTitle, NULL, false) :
			MessageBox(NULL, pa->szDebugInfo, pa->szTitle, MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_RETRYCANCEL);
		#endif
			

	gbInMyAssertTrap = false;
	if (ghInMyAssertTrap)
		SetEvent(ghInMyAssertTrap);
	gnInMyAssertThread = 0;
	return nRc;
}
void MyAssertTrap()
{
	_CrtDbgBreak();
}
int MyAssertProc(const wchar_t* pszFile, int nLine, const wchar_t* pszTest, bool abNoPipe)
{
#ifdef _DEBUG
	static bool lbSkip = false;
	if (lbSkip)
		return 1;
#endif

	HANDLE hHeap = GetProcessHeap();
	MyAssertInfo* pa = (MyAssertInfo*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(MyAssertInfo));
	if (!pa)
		return -1;
	wchar_t *szExeName = (wchar_t*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (MAX_PATH+1)*sizeof(wchar_t));
	if (szExeName && !GetModuleFileNameW(NULL, szExeName, MAX_PATH+1)) szExeName[0] = 0;
	pa->bNoPipe = abNoPipe;
	msprintf(pa->szTitle, countof(pa->szTitle), L"CEAssert PID=%u TID=%u", GetCurrentProcessId(), GetCurrentThreadId());
	msprintf(pa->szDebugInfo, countof(pa->szDebugInfo), L"Assertion in %s\n%s\n\n%s: %i\n\nPress 'Retry' to trap.",
	                szExeName ? szExeName : L"", pszTest ? pszTest : L"", pszFile, nLine);
	DWORD dwCode = 0;

	if (gAllowAssertThread == am_Thread)
	{
		DWORD dwTID;
		HANDLE hThread = CreateThread(NULL, 0, MyAssertThread, pa, 0, &dwTID);

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

		if (hGuiWnd && !gbInMyAssertTrap)
		{
			gbInMyAssertTrap = true;
			gbInMyAssertPipe = true;
			gnInMyAssertThread = GetCurrentThreadId();
			ResetEvent(ghInMyAssertTrap);
			
			dwCode = GuiMessageBox(abNoPipe ? NULL : hGuiWnd, pa->szDebugInfo, pa->szTitle, MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_RETRYCANCEL);
			gbInMyAssertTrap = false;
			gbInMyAssertPipe = false;
			SetEvent(ghInMyAssertTrap);
			gnInMyAssertThread = 0;
			goto wrap;
		}
	}

	while (gbInMyAssertPipe && (gnInMyAssertThread != GetCurrentThreadId()))
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
		if (gbInMyAssertTrap)
		{
			if (!gnInMyAssertThread)
			{
				_ASSERTE(gbInMyAssertTrap && gnInMyAssertThread);
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
	_wsprintf(szDEBUGSTRTime, SKIPLEN(countof(szDEBUGSTRTime)) L"%i:%02i:%02i.%03i(%s.%i.%i) ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, gszDbgModLabel, GetCurrentProcessId(), GetCurrentThreadId());
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
