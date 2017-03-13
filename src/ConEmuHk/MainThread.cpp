
/*
Copyright (c) 2015-2017 Maximus5
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

#include "../common/Common.h"
#include "../common/MArray.h"
#include "../common/PipeServer.h"
#include "DbgHooks.h"
#include "hlpProcess.h"
#include "MainThread.h"

#ifdef FORCE_GETMAINTHREAD_PRINTF
#include "Ansi.h"
#endif

extern PipeServer<CESERVER_REQ> *gpHookServer;


/// gStartedThreads stores all thread IDs of current process
/// BOOL value is TRUE if thread was started by `CreateThread` function,
/// otherwise it is FALSE if thread was found from DLL_THREAD_DETACH,
/// or Thread32First/Next
MMap<DWORD,BOOL> gStartedThreads;


/// Main thread ID in the current process
DWORD gnHookMainThreadId = 0;

/// Returns ThreadID of main thread
///
/// If the process was started by standard ConEmuC/ConEmuHk functions,
/// this function is called with (bUseCurrentAsMain==true) from DllMain.
/// Otherwise we must enumerate **all** processes in system, there is no
/// way to enumerate only current process threads unfortunately.
/// Also, enumerating threads may cause noticeable lags, but we can't
/// do anything with that... However, this is rare situation, and in most
/// cases main thread ID is initialized with (bUseCurrentAsMain==true).
DWORD GetMainThreadId(bool bUseCurrentAsMain)
{
	// Найти ID основной нити
	if (!gnHookMainThreadId)
	{
		if (bUseCurrentAsMain)
		{
			// Only one thread is expected at the moment
			gnHookMainThreadId = GetCurrentThreadId();
		}
		else
		{
			DWORD dwPID = GetCurrentProcessId();

			#ifdef FORCE_GETMAINTHREAD_PRINTF
			wchar_t szInfo[160], szTail[32];
			msprintf(szInfo, countof(szInfo), L"\x1B[1;31;40m" L"*** [PID=%u %s] GetMainThreadId is using CreateToolhelp32Snapshot", dwPID, gsExeName);
			wcscpy_c(szTail, L"\x1B[1;31;40m" L" ***" L"\x1B[0m" L"\n");
			WriteProcessed2(szInfo, wcslen(szInfo), NULL, wps_Error);
			#endif

			// Unfortunately, dwPID is ignored in TH32CS_SNAPTHREAD
			HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwPID);

			if (snapshot != INVALID_HANDLE_VALUE)
			{
				THREADENTRY32 module = {sizeof(THREADENTRY32)};

				if (Thread32First(snapshot, &module))
				{
					// Don't stop enumeration on first thread, populate gStartedThreads
					do
					{
						if (module.th32OwnerProcessID == dwPID)
						{
							DWORD nTID = module.th32ThreadID;

							if (!gnHookMainThreadId)
								gnHookMainThreadId = nTID;

							if (!gStartedThreads.Get(nTID, NULL))
								gStartedThreads.Set(nTID, FALSE);
						}

					} while (Thread32Next(snapshot, &module));
				}

				CloseHandle(snapshot);
			}

			#ifdef FORCE_GETMAINTHREAD_PRINTF
			WriteProcessed2(szTail, wcslen(szTail), NULL, wps_Error);
			#endif
		}
	}

	#ifdef _DEBUG
	char szInfo[100];
	msprintf(szInfo, countof(szInfo), "GetMainThreadId()=%u, TID=%u\n", gnHookMainThreadId, GetCurrentThreadId());
	//OutputDebugStringA(szInfo);
	#endif

	_ASSERTE(gnHookMainThreadId!=0);
	return gnHookMainThreadId;
}


// The problem was in third-party hooking application on cygwin ssh.
// It does its injects with CreateRemoteThread when ConEmu is present,
// and when this thread finishes before ssh init is done, the crash occurres.
// FixSshThreads was created in commit 2a78d93

/// Defines the structure for pThInfo, used to Suspend/Resume
struct ThInfoStr { DWORD_PTR nTID; HANDLE hThread; };
/// Stores suspended threads IDs and handles
static MArray<ThInfoStr> *pThInfo = NULL;

/// Stores count of FixSshThreads(1) calls
long gnFixSshThreadsResumeOk = 0;

/// Workaround for cygwin's ssh crash when third-party hooking application exists
void FixSshThreads(int iStep)
{
	DLOG0("FixSshThreads",iStep);

	#ifdef _DEBUG
	char szInfo[120]; DWORD nErr;
	msprintf(szInfo, countof(szInfo), "FixSshThreads(%u) started\n", iStep);
	if (!(gnDllState & ds_DllProcessDetach)) OutputDebugStringA(szInfo);
	#endif

	switch (iStep)
	{
		// Resume suspended threads
		case 1:
		{
			// Was initialized?
			if (!pThInfo)
				break;
			// May occurs in several threads simultaneously
			long n = InterlockedIncrement(&gnFixSshThreadsResumeOk);
			if (n > 1)
				break;
			// Resume all suspended...
			for (INT_PTR i = 0; i < pThInfo->size(); i++)
				ResumeThread((*pThInfo)[i].hThread);
			break;
		}

		// Suspend all third-party threads
		case 0:
		{
			_ASSERTEX(gnHookMainThreadId!=0);
			pThInfo = new MArray<ThInfoStr>;
			HANDLE hThread = NULL, hSnap = NULL;
			DWORD nTID = 0, dwPID = GetCurrentProcessId();
			HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwPID);
			if (snapshot == INVALID_HANDLE_VALUE)
			{
				#ifdef _DEBUG
				nErr = GetLastError();
				msprintf(szInfo, countof(szInfo), "CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD) failed in FixSshThreads, code=%u\n", nErr);
				if (!(gnDllState & ds_DllProcessDetach)) OutputDebugStringA(szInfo);
				#endif
			}
			else
			{
				THREADENTRY32 module = {sizeof(THREADENTRY32)};
				if (!Thread32First(snapshot, &module))
				{
					#ifdef _DEBUG
					nErr = GetLastError();
					msprintf(szInfo, countof(szInfo), "Thread32First failed in FixSshThreads, code=%u\n", nErr);
					if (!(gnDllState & ds_DllProcessDetach)) OutputDebugStringA(szInfo);
					#endif
				}
				else do
				{
					if ((module.th32OwnerProcessID == dwPID) && (gnHookMainThreadId != module.th32ThreadID))
					{
						// JIC, add thread ID to our list.
						// In theory, all thread must be already initialized
						// either from DLL_THREAD_ATTACH, or from GetMainThreadId.
						if (!gStartedThreads.Get(module.th32ThreadID, NULL))
							gStartedThreads.Set(module.th32ThreadID, FALSE);

						// Don't freeze our own threads
						if (gpHookServer && gpHookServer->IsPipeThread(module.th32ThreadID))
							continue;

						hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, module.th32ThreadID);
						if (!hThread)
						{
							#ifdef _DEBUG
							nErr = GetLastError();
							msprintf(szInfo, countof(szInfo), "OpenThread(%u) failed in FixSshThreads, code=%u\n", module.th32ThreadID, nErr);
							if (!(gnDllState & ds_DllProcessDetach)) OutputDebugStringA(szInfo);
							#endif
						}
						else
						{
							DWORD nSC = SuspendThread(hThread);
							if (nSC == (DWORD)-1)
							{
								// Error!
								#ifdef _DEBUG
								nErr = GetLastError();
								msprintf(szInfo, countof(szInfo), "SuspendThread(%u) failed in FixSshThreads, code=%u\n", module.th32ThreadID, nErr);
								if (!(gnDllState & ds_DllProcessDetach)) OutputDebugStringA(szInfo);
								#endif
							}
							else
							{
								ThInfoStr th = {module.th32ThreadID, hThread};
								pThInfo->push_back(th);
								#ifdef _DEBUG
								msprintf(szInfo, countof(szInfo), "Thread %u was suspended\n", module.th32ThreadID);
								if (!(gnDllState & ds_DllProcessDetach)) OutputDebugStringA(szInfo);
								#endif
							}
						}
					}
				} while (Thread32Next(snapshot, &module));

				CloseHandle(snapshot);
			}
			break;
		}
	}

	#ifdef _DEBUG
	msprintf(szInfo, countof(szInfo), "FixSshThreads(%u) finished\n", iStep);
	if (!(gnDllState & ds_DllProcessDetach)) OutputDebugStringA(szInfo);
	#endif

	DLOGEND();
}


/// Structure for internal thread enumerator
struct HookThreadList
{
	INT_PTR iCount, iCurrent;
	LPDWORD pThreadIDs;
};

/// Creates internal thread enumerator handle
/// @param  dwFlags must be TH32CS_SNAPTHREAD
/// @param  th32ProcessID ignored, 0 is expected
/// @result INVALID_HANDLE_VALUE on errors, or pointer to HookThreadList
HANDLE WINAPI HookThreadListCreate(DWORD dwFlags, DWORD th32ProcessID)
{
	HookThreadList* p = (HookThreadList*)calloc(sizeof(HookThreadList),1);

	p->iCount = gStartedThreads.GetKeysValues(&p->pThreadIDs, NULL);

	if ((p->iCount <= 0) || (!p->pThreadIDs))
	{
		_ASSERTE(FALSE && "gStartedThreads.GetKeysValues fails");
		free(p);
		return INVALID_HANDLE_VALUE;
	}
	return (HANDLE)p;
}

BOOL WINAPI HookThreadListNext(HANDLE hSnapshot, LPTHREADENTRY32 lpte)
{
	if (hSnapshot && (hSnapshot != INVALID_HANDLE_VALUE))
	{
		HookThreadList* p = (HookThreadList*)hSnapshot;
		if (p->iCurrent < p->iCount)
		{
			// minhook requires only two fields
			lpte->th32ThreadID = p->pThreadIDs[p->iCurrent++];
			lpte->th32OwnerProcessID = GetCurrentProcessId();
			return TRUE;
		}
	}
	return FALSE;
}

BOOL WINAPI HookThreadListClose(HANDLE hSnapshot)
{
	if (hSnapshot && (hSnapshot != INVALID_HANDLE_VALUE))
	{
		HookThreadList* p = (HookThreadList*)hSnapshot;
		gStartedThreads.ReleasePointer(p->pThreadIDs);
		free(p);
		return TRUE;
	}
	return FALSE;
}
