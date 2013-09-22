
/*
Copyright (c) 2012-2013 Maximus5
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
#include "TrayIcon.h"
#include "../ConEmuCD/ExitCodes.h"

#define StartupValueName L"ConEmuDefaultTerminal"

CDefaultTerminal::CDefaultTerminal()
{
	mh_LastWnd = mh_LastIgnoredWnd = mh_LastCall = NULL;
	mb_ReadyToHook = FALSE;
	mh_SignEvent = NULL;
	mn_PostThreadId = 0;
	mb_PostCreatedThread = false;
	mb_Initialized = false;
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
				continue; // Эта нить еще не закончила
			}
		}
		SafeCloseHandle(hProcess);

		m_Threads.erase(i);
	}
}

bool CDefaultTerminal::IsRegisteredOsStartup(wchar_t* rsValue, DWORD cchMax)
{
	LPCWSTR ValueName = StartupValueName;
	bool bCurState = false;
	HKEY hk;
	DWORD nSize, nType = 0;
	LONG lRc;

	if (0 == (lRc = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hk)))
	{
		nSize = cchMax*sizeof(*rsValue);
		if ((0 == (lRc = RegQueryValueEx(hk, ValueName, NULL, &nType, (LPBYTE)rsValue, &nSize)) && (nType == REG_SZ) && (nSize > sizeof(wchar_t))))
		{
			bCurState = true;
		}
		RegCloseKey(hk);
	}

	return bCurState;
}

void CDefaultTerminal::CheckRegisterOsStartup()
{
	LPCWSTR ValueName = StartupValueName;
	bool bCurState = false;
	bool bNeedState = gpSet->isSetDefaultTerminal && gpSet->isRegisterOnOsStartup;
	HKEY hk;
	DWORD nSize; //, nType = 0;
	wchar_t szCurValue[MAX_PATH*3] = {};
	wchar_t szNeedValue[MAX_PATH+80] = {};
	LONG lRc;

	_wsprintf(szNeedValue, SKIPLEN(countof(szNeedValue)) L"\"%s\" /SetDefTerm /Detached /MinTSA", gpConEmu->ms_ConEmuExe);

	if (IsRegisteredOsStartup(szCurValue, countof(szCurValue)-1) && *szCurValue)
	{
		bCurState = true;
	}

	if ((bCurState != bNeedState)
		|| (bNeedState && (lstrcmpi(szNeedValue, szCurValue) != 0)))
	{
		if (0 == (lRc = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hk, NULL)))
		{
			if (bNeedState)
			{
				nSize = ((DWORD)_tcslen(szNeedValue)+1) * sizeof(szNeedValue[0]);
				if (0 != (lRc = RegSetValueEx(hk, ValueName, 0, REG_SZ, (LPBYTE)szNeedValue, nSize)))
				{
					DisplayLastError(L"Failed to set ConEmuDefaultTerminal value in HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", lRc);
				}
			}
			else
			{
				if (0 != (lRc = RegDeleteValue(hk, ValueName)))
				{
					DisplayLastError(L"Failed to remove ConEmuDefaultTerminal value from HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", lRc);
				}
			}
			RegCloseKey(hk);
		}
		else
		{
			DisplayLastError(L"Failed to open HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run for write", lRc);
		}
	}
}

void CDefaultTerminal::OnHookedListChanged()
{
	if (!mb_Initialized)
		return;
	if (!isDefaultTerminalAllowed())
		return;

	EnterCriticalSection(&mcs);
	ClearProcessed(true);
	LeaveCriticalSection(&mcs);

	// Если проводника в списке НЕ было, а сейчас появился...
	// Проверить процесс шелла
	CheckShellWindow();
}

bool CDefaultTerminal::isDefaultTerminalAllowed()
{
	if (gpConEmu->DisableSetDefTerm || !gpSet->isSetDefaultTerminal)
		return false;
	return true;
}

void CDefaultTerminal::PostCreated(bool bWaitForReady /*= false*/, bool bShowErrors /*= false*/)
{
	if (!ghWnd)
	{
		// Main ConEmu window must be created
		// It is required for initialization of ConEmuHk.dll
		// wich will be injected into hooked processes
		_ASSERTE(ghWnd!=NULL);
		return;
	}

	if (!isDefaultTerminalAllowed())
	{
		_ASSERTE(bWaitForReady == false);
		if (bShowErrors && gpConEmu->DisableSetDefTerm)
		{
			DisplayLastError(L"Default terminal feature was blocked\n"
				L"with '/NoDefTerm' ConEmu command line argument!", -1);
		}
		return;
	}

	if (mb_PostCreatedThread)
	{
		if (!bShowErrors)
			return;
		ClearThreads(false);
		if (m_Threads.size() > 0)
		{
			Icon.ShowTrayIcon(L"Previous Default Terminal setup cycle was not finished yet", tsa_Default_Term);
			return;
		}
	}

	CheckRegisterOsStartup();

	mb_ReadyToHook = TRUE;

	// Этот процесс занимает некоторое время, чтобы не блокировать основной поток - запускаем фоновый
	mb_PostCreatedThread = true;
	DWORD  nWait = WAIT_FAILED;
	HANDLE hPostThread = CreateThread(NULL, 0, PostCreatedThread, this, 0, &mn_PostThreadId);
	
	if (hPostThread)
	{
		if (bWaitForReady)
		{
			// Wait for 30 seconds
			DWORD nStart = GetTickCount();
			SetCursor(LoadCursor(NULL, IDC_WAIT));
			nWait = WaitForSingleObject(hPostThread, 30000);
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			DWORD nDuration = GetTickCount() - nStart;
			if (nWait == WAIT_OBJECT_0)
			{
				CloseHandle(hPostThread);
				return;
			}
			else
			{
				//_ASSERTE(nWait == WAIT_OBJECT_0);
				DisplayLastError(L"PostCreatedThread was not finished in 30 seconds");
				UNREFERENCED_PARAMETER(nDuration);
			}
		}

		m_Threads.push_back(hPostThread);
	}
	else
	{
		if (bShowErrors)
		{
			DisplayLastError(L"Failed to start PostCreatedThread");
		}
		_ASSERTE(hPostThread!=NULL);
		mb_PostCreatedThread = false;
	}

	mb_Initialized = true;
}

DWORD CDefaultTerminal::PostCreatedThread(LPVOID lpParameter)
{
	CDefaultTerminal *pTerm = (CDefaultTerminal*)lpParameter;

	pTerm->CheckShellWindow();

	// Done
	pTerm->mb_PostCreatedThread = false;

	return 0;
}

// Поставить хук в процесс шелла (explorer.exe)
bool CDefaultTerminal::CheckShellWindow()
{
	bool bHooked = false;
	HWND hDesktop = GetDesktopWindow(); //csrss.exe on Windows 8
	HWND hShell = GetShellWindow();
	HWND hTrayWnd = FindWindowEx(NULL, NULL, L"Shell_TrayWnd", NULL);
	DWORD nDesktopPID = 0, nShellPID = 0, nTrayPID = 0;

	if (!bHooked && hShell)
	{
		if (GetWindowThreadProcessId(hShell, &nShellPID) && nShellPID)
		{
			bHooked = CheckForeground(hShell, nShellPID, false);
		}
	}

	if (!bHooked && hTrayWnd)
	{
		if (GetWindowThreadProcessId(hTrayWnd, &nTrayPID) && nTrayPID
			&& (nTrayPID != nShellPID))
		{
			bHooked = CheckForeground(hTrayWnd, nTrayPID, false);
		}
	}

	if (!bHooked && hDesktop)
	{
		if (GetWindowThreadProcessId(hDesktop, &nDesktopPID) && nDesktopPID
			&& (nDesktopPID != nTrayPID) && (nDesktopPID != nShellPID))
		{
			bHooked = CheckForeground(hDesktop, nDesktopPID, false);
		}
	}

	return bHooked;
}

// Чтобы избежать возможного зависания в процессе установки
// хука в процесс шелла (explorer.exe) при старте ConEmu
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

// Проверка окна переднего плана. Если оно принадлежит к хукаемым процесса - вставить хук.
// ДИАЛОГИ НЕ ПРОВЕРЯЮТСЯ
bool CDefaultTerminal::CheckForeground(HWND hFore, DWORD nForePID, bool bRunInThread /*= true*/)
{
	if (!isDefaultTerminalAllowed())
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

	// Если главное окно еще не создано
	if (!mb_ReadyToHook)
	{
		// Сразу выходим
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
		// Это окно уже проверялось
		lbRc = (hFore == mh_LastWnd);
		goto wrap;
	}

	if (bRunInThread && (hFore == mh_LastCall))
	{
		// Просто выйти. Это проверка на частые фоновые вызовы.
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
			//|| (lstrcmp(szClass, L"#32770") == 0) // Ignore dialogs // -- Process dialogs too (Application may be dialog-based)
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

	_ASSERTE(isDefaultTerminalAllowed());

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
		hProcess = NULL; // его закрывать НЕ нужно, сохранен в массиве
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
