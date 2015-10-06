
/*
Copyright (c) 2015 Maximus5
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

#include "../common/common.hpp"
#include "MainThread.h"

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
