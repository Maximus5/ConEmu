
/*
Copyright (c) 2012 Maximus5
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

#include "Header.h"
#include <Tlhelp32.h>
#include "DefaultTerm.h"
#include "ConEmu.h"
#include "Options.h"
#include "../ConEmuCD/ExitCodes.h"

CDefaultTerminal::CDefaultTerminal()
{
	mh_LastWnd = mh_LastIgnoredWnd = mh_LastCall = NULL;
	mb_ReadyToHook = FALSE;
	mh_SignEvent = NULL;
	mn_PostThreadId = 0;
	InitializeCriticalSection(&mcs);
}

CDefaultTerminal::~CDefaultTerminal()
{
	ClearThreads(true);

	ClearProcessed(true);

	DeleteCriticalSection(&mcs);

	SafeCloseHandle(mh_SignEvent);
}

void CDefaultTerminal::ClearThreads(bool bForceTerminate)
{
	for (INT_PTR i = m_Threads.size(); i--;)
	{
		HANDLE hProcess = m_Threads[i];
		if (WaitForSingleObject(hProcess, 0) != WAIT_OBJECT_0)
		{
			if (bForceTerminate)
			{
				TerminateThread(hProcess, 100);
			}
			else
			{
				continue; // Ёта нить еще не закончила
			}
		}
		SafeCloseHandle(hProcess);

		m_Threads.erase(i);
	}
}

void CDefaultTerminal::PostCreated()
{
	if (gpConEmu->DisableSetDefTerm)
		return;

	mb_ReadyToHook = TRUE;

	// Ётот процесс занимает некоторое врем€, чтобы не блокировать основной поток - запускаем фоновый
	HANDLE hPostThread = CreateThread(NULL, 0, PostCreatedThread, this, 0, &mn_PostThreadId);
	_ASSERTE(hPostThread!=NULL);
	if (hPostThread)
	{
		m_Threads.push_back(hPostThread);
	}
}

DWORD CDefaultTerminal::PostCreatedThread(LPVOID lpParameter)
{
	CDefaultTerminal *pTerm = (CDefaultTerminal*)lpParameter;
	bool bHooked = false;
	HWND hDesktop = GetDesktopWindow();
	HWND hTrayWnd = FindWindowEx(NULL, NULL, L"Shell_TrayWnd", NULL);
	DWORD nDesktopPID = 0, nTrayPID = 0;

	if (hDesktop)
	{
		if (GetWindowThreadProcessId(hDesktop, &nDesktopPID) && nDesktopPID)
		{
			bHooked = pTerm->CheckForeground(hDesktop, nDesktopPID, false);
		}
	}

	if (!bHooked && hTrayWnd)
	{
		if (GetWindowThreadProcessId(hTrayWnd, &nTrayPID) && nTrayPID && (nTrayPID != nDesktopPID))
		{
			bHooked = pTerm->CheckForeground(hTrayWnd, nTrayPID, false);
		}
	}

	return 0;
}

DWORD CDefaultTerminal::PostCheckThread(LPVOID lpParameter)
{
	bool bRc = false;
	ThreadArg* pArg = (ThreadArg*)lpParameter;
	if (pArg)
	{
		bRc = pArg->pTerm->CheckForeground(pArg->hFore, pArg->nForePID, false);
		free(pArg);
	}
	return bRc;
}

bool CDefaultTerminal::CheckForeground(HWND hFore, DWORD nForePID, bool bRunInThread /*= true*/)
{
	if (gpConEmu->DisableSetDefTerm)
		return false;

	bool lbRc = false;
	bool lbLocked = false;
	DWORD nResult = 0;
	wchar_t szClass[MAX_PATH]; szClass[0] = 0;
	PROCESSENTRY32 prc;
	bool bMonitored = false;
	const wchar_t* pszMonitored = NULL;
	HANDLE hProcess = NULL;
	int nBits = 0;
	wchar_t szCmdLine[MAX_PATH*3];
	wchar_t szName[64];
	PROCESS_INFORMATION pi = {};
	STARTUPINFO si = {sizeof(si)};
	BOOL bStarted = FALSE;

	// ≈сли не просили, или если главное окно еще не создано
	if (!gpSet->isSetDefaultTerminal || !mb_ReadyToHook)
	{
		// —разу выходим
		goto wrap;
	}

	//_ASSERTE(gpConEmu->isMainThread());
	if (!hFore || !nForePID)
	{
		_ASSERTE(hFore && nForePID);
		goto wrap;
	}

	if (hFore == mh_LastWnd || hFore == mh_LastIgnoredWnd)
	{
		// Ёто окно уже провер€лось
		lbRc = (hFore == mh_LastWnd);
		goto wrap;
	}

	if (bRunInThread && (hFore == mh_LastCall))
	{
		// ѕросто выйти. Ёто проверка на частые фоновые вызовы.
		goto wrap;
	}
	mh_LastCall = hFore;

	if (bRunInThread)
	{
		if (gpConEmu->isMainThread())
		{
			// Clear finished threads
			ClearThreads(false);
		}

		HANDLE hPostThread = NULL; DWORD nThreadId = 0;
		ThreadArg* pArg = (ThreadArg*)malloc(sizeof(ThreadArg));
		if (!pArg)
		{
			_ASSERTE(pArg);
			goto wrap;
		}
		pArg->pTerm = this;
		pArg->hFore = hFore;
		pArg->nForePID = nForePID;

		hPostThread = CreateThread(NULL, 0, PostCheckThread, pArg, 0, &nThreadId);
		_ASSERTE(hPostThread!=NULL);
		if (hPostThread)
		{
			m_Threads.push_back(hPostThread);
		}

		lbRc = (hPostThread != NULL); // вернуть OK?
		goto wrap;
	}

	EnterCriticalSection(&mcs);
	lbLocked = true;

	// Clear dead processes and windows
	ClearProcessed(false);

	// Check window class
	if (GetClassName(hFore, szClass, countof(szClass)))
	{
		if ((lstrcmp(szClass, VirtualConsoleClass) == 0)
			|| (lstrcmp(szClass, L"#32770") == 0) // Ignore dialogs
			|| isConsoleClass(szClass))
		{
			mh_LastIgnoredWnd = hFore;
			goto wrap;
		}
	}

	// Go and check
	if (!GetProcessInfo(nForePID, &prc))
	{
		mh_LastIgnoredWnd = hFore;
		goto wrap;
	}

	CharLowerBuff(prc.szExeFile, lstrlen(prc.szExeFile));

	if (lstrcmp(prc.szExeFile, L"csrss.exe") == 0)
	{
		// This is "System" process and may not be hooked
		mh_LastIgnoredWnd = hFore;
		goto wrap;
	}

	// Is it in monitored applications?
	pszMonitored = gpSet->GetDefaultTerminalAppsMSZ();
	if (pszMonitored)
	{
		// All strings are lower case
		const wchar_t* psz = pszMonitored;
		while (*psz)
		{
			if (_tcscmp(psz, prc.szExeFile) == 0)
			{
				bMonitored = true;
				break;
			}
			psz += _tcslen(psz)+1;
		}
	}

	// And how it is?
	if (!bMonitored)
	{
		mh_LastIgnoredWnd = hFore;
		goto wrap;
	}

	// Need to process
	for (INT_PTR i = m_Processed.size(); i--;)
	{
		if (m_Processed[i].nPID == nForePID)
		{
			bMonitored = false;
			break; // already hooked
		}
	}

	// May be hooked already?
	if (!bMonitored)
	{
		mh_LastWnd = hFore;
		lbRc = true;
		goto wrap;
	}

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|SYNCHRONIZE, FALSE, nForePID);
	if (!hProcess)
	{
		// Failed to hook
		mh_LastIgnoredWnd = hFore;
		goto wrap;
	}

	// Need to be hooked
	nBits = GetProcessBits(nForePID, hProcess);
	switch (nBits)
	{
	case 32:
		_wsprintf(szCmdLine, SKIPLEN(countof(szCmdLine)) L"\"%s\\%s\" /DEFTRM=%u",
			gpConEmu->ms_ConEmuBaseDir, L"ConEmuC.exe", nForePID);
		break;
	case 64:
		_wsprintf(szCmdLine, SKIPLEN(countof(szCmdLine)) L"\"%s\\%s\" /DEFTRM=%u",
			gpConEmu->ms_ConEmuBaseDir, L"ConEmuC64.exe", nForePID);
		break;
	}
	if (!*szCmdLine)
	{
		// Unsupported bitness?
		CloseHandle(hProcess);
		mh_LastIgnoredWnd = hFore;
		goto wrap;
	}

	// Prepare event
	_wsprintf(szName, SKIPLEN(countof(szName)) CEDEFAULTTERMHOOK, nForePID);
	SafeCloseHandle(mh_SignEvent);
	mh_SignEvent = CreateEvent(LocalSecurity(), FALSE, FALSE, szName);
	if (mh_SignEvent) SetEvent(mh_SignEvent); // May be excess, but if event already exists...

	// Run hooker
	si.dwFlags = STARTF_USESHOWWINDOW;
	bStarted = CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	if (!bStarted)
	{
		DisplayLastError(L"Failed to start hooking application!\nDefault terminal feature will not be available!");
		CloseHandle(hProcess);
		mh_LastIgnoredWnd = hFore;
		goto wrap;
	}
	CloseHandle(pi.hThread);
	// Waiting for result
	TODO("Show status in status line?");
	WaitForSingleObject(pi.hProcess, INFINITE);
	GetExitCodeProcess(pi.hProcess, &nResult);
	CloseHandle(pi.hProcess);
	// And what?
	if (nResult == (UINT)CERR_HOOKS_WAS_SET)
	{
		mh_LastWnd = hFore;
		ProcessInfo inf = {};
		inf.hProcess = hProcess;
		hProcess = NULL; // его закрывать Ќ≈ нужно, сохранен в массиве
		inf.nPID = nForePID;
		inf.nHookTick = GetTickCount();
		m_Processed.push_back(inf);
		lbRc = true;
		goto wrap;
	}
	// Failed, remember this
	CloseHandle(hProcess);
	mh_LastIgnoredWnd = hFore;
	_ASSERTE(lbRc == false);
wrap:
	if (lbLocked)
	{
		LeaveCriticalSection(&mcs);
	}
	return lbRc;
}

void CDefaultTerminal::ClearProcessed(bool bForceAll)
{
	//_ASSERTE(gpConEmu->isMainThread());

	for (INT_PTR i = m_Processed.size(); i--;)
	{
		bool bTerm = bForceAll;
		HANDLE hProcess = m_Processed[i].hProcess;

		if (hProcess)
		{
			if (!bTerm)
			{
				DWORD nWait = WaitForSingleObject(hProcess, 0);
				if (nWait == WAIT_OBJECT_0)
				{
					bTerm = true;
				}
			}

			if (bTerm)
			{
				CloseHandle(hProcess);
			}
		}

		if (bTerm)
		{
			m_Processed.erase(i);
		}
	}

	if (mh_LastWnd && !IsWindow(mh_LastWnd))
	{
		if (mh_LastIgnoredWnd == mh_LastWnd)
			mh_LastIgnoredWnd = NULL;
		mh_LastWnd = NULL;
	}
	if (mh_LastIgnoredWnd && !IsWindow(mh_LastIgnoredWnd))
	{
		mh_LastIgnoredWnd = NULL;
	}
}
