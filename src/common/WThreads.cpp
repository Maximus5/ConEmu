
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
#include "Common.h"
#include "WThreads.h"

#include <intrin.h>

struct ConEmuThreadStartArg;

#define THREADS_LOG_SIZE 256 // must be power of 2
#define THREAD_MAX_NAME_LEN 40
static struct ConEmuThreadInfo
{
	DWORD  nThreadID; // Let TID be first member to simplify debug watch
	BOOL   bActive;
	ConEmuThreadStartArg* p;
	HANDLE hThread;
	LPTHREAD_START_ROUTINE pStartAddress;
	DWORD  nParentThreadID;
	DWORD  nStartTick;
	DWORD  nEndTick;
	DWORD  nExitCode;
	BOOL   bTerminated;
	char   sName[THREAD_MAX_NAME_LEN];
	char   sKiller[THREAD_MAX_NAME_LEN];
} g_Threads[THREADS_LOG_SIZE];

LONG g_ThreadsIdx = 0;
LONG g_TerminatedThreadIdx = -1;
static HANDLE gh_UnknownTerminatedThread = NULL;

struct ConEmuThreadStartArg
{
public:
	LPTHREAD_START_ROUTINE lpStartAddress;
	LPVOID   lpParameter;
	HANDLE   hThread;
	DWORD    nParentTID;
	char     sName[THREAD_MAX_NAME_LEN];
public:
	ConEmuThreadStartArg(LPTHREAD_START_ROUTINE apStartAddress, LPVOID apParameter, DWORD anParentTID)
		: lpStartAddress(apStartAddress)
		, lpParameter(apParameter)
		, hThread(NULL)
		, nParentTID(anParentTID)
	{
		sName[0] = 0;
	};

	// !!! Do NOT add destructor here to free hThread !!!
	// !!! It's stored as value for informational purposes only !!!

};

#ifndef __GNUC__
#pragma warning( push )
#pragma warning( disable : 4748 )
#pragma optimize( "", off )
#endif

DWORD WINAPI apiThreadHelper(LPVOID lpParameter)
{
	ConEmuThreadStartArg* p = (ConEmuThreadStartArg*)lpParameter;

	#if defined(_MSC_VER) && !defined(CONEMU_MINIMAL)
	if (IsDebuggerPresent())
	{
		SetThreadName((DWORD)-1, p->sName);
	}
	#endif

	ConEmuThreadInfo Info = {
		GetCurrentThreadId(),
		TRUE,
		p,
		p->hThread,
		p->lpStartAddress,
		p->nParentTID,
		GetTickCount()
	};
	lstrcpynA(Info.sName, p->sName, countof(Info.sName));

	ConEmuThreadInfo* pInfo = NULL;
	for (INT_PTR c = THREADS_LOG_SIZE; --c >= 0;)
	{
		LONG i = (_InterlockedIncrement(&g_ThreadsIdx) & (THREADS_LOG_SIZE-1));
		if (!g_Threads[i].bActive)
		{
			g_Threads[i] = Info;
			pInfo = (g_Threads+i);
			break;
		}
	}
	if (!pInfo)
	{
		_ASSERTE(pInfo!=NULL && "Too many active threads?");
	}


	// Run the thread routine
	DWORD nRc = p->lpStartAddress(p->lpParameter);


	// Thread is finishing
	if (pInfo && pInfo->bActive && (pInfo->nThreadID == Info.nThreadID))
	{
		pInfo->nExitCode = nRc;
		pInfo->nEndTick = GetTickCount();
		pInfo->bActive = FALSE;
	}

	// Done
	delete p;
	return nRc;
}

#ifndef __GNUC__
#pragma optimize( "", on )
#pragma warning( pop )
#endif


HANDLE apiCreateThread(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, LPDWORD lpThreadId, LPCSTR asThreadNameFormat /*= NULL*/, int anFormatArg /*= 0*/)
{
	HANDLE hThread = NULL;
	ConEmuThreadStartArg* args = new ConEmuThreadStartArg(lpStartAddress, lpParameter, GetCurrentThreadId());
	if (!args)
	{
		// Not enough memory
		return NULL;
	}

	// Set user-friendly thread name for VisualStudio debugger
	if (asThreadNameFormat && *asThreadNameFormat)
	{
		if (strchr(asThreadNameFormat, '%') != NULL)
		{
			INT_PTR cchMax = strlen(asThreadNameFormat) + 64;
			char* pszNewName = new char[cchMax];
			if (pszNewName)
			{
				_wsprintfA(pszNewName, SKIPLEN(cchMax) asThreadNameFormat, anFormatArg);
				_ASSERTE(strlen(pszNewName) < THREAD_MAX_NAME_LEN);
				lstrcpynA(args->sName, pszNewName, countof(args->sName));
				delete[] pszNewName;
			}
		}

		if (args->sName[0] == 0)
		{
			_ASSERTE(strlen(asThreadNameFormat) < THREAD_MAX_NAME_LEN);
			lstrcpynA(args->sName, asThreadNameFormat, countof(args->sName));
		}
	}

	// Start the thread
	hThread = ::CreateThread(NULL, 0, apiThreadHelper, args, CREATE_SUSPENDED, lpThreadId);
	DWORD nLastError = GetLastError();
	if (hThread)
	{
		args->hThread = hThread;
		ResumeThread(hThread);
	}
	else
	{
		delete args;
	}

	// Done
	SetLastError(nLastError);
	return hThread;
}

BOOL apiTerminateThreadEx(HANDLE hThread, DWORD dwExitCode, LPCSTR asFile, int anLine)
{
	ConEmuThreadInfo* pThread = NULL;
	for (INT_PTR c = THREADS_LOG_SIZE; --c >= 0;)
	{
		if (g_Threads[c].bActive && (g_Threads[c].hThread == hThread))
		{
			pThread = (g_Threads+c);
			pThread->nExitCode = dwExitCode;
			pThread->bTerminated = TRUE;
			pThread->nEndTick = GetTickCount();
			g_TerminatedThreadIdx = LOLONG(c);
		}
	}

	#ifndef __GNUC__
	#pragma warning( push )
	#pragma warning( disable : 6258 )
	#endif

	if (!pThread)
	{
		//TODO: Log or Assert on terminate unknown thread
		_ASSERTE(FALSE && "TerminateThread-ing unknown thread")
		gh_UnknownTerminatedThread = hThread;
	}

	BOOL bRc = ::TerminateThread(hThread, dwExitCode);

	#ifndef __GNUC__
	#pragma warning( pop )
	#endif

	if (pThread)
	{
		LPCSTR pszName = strrchr(asFile, L'\\'); if (pszName) pszName++; else pszName = asFile;
		char szKiller[30]; lstrcpynA(szKiller, pszName, countof(szKiller));
		_ASSERTE((countof(szKiller)+8) < countof(pThread->sKiller));
		_wsprintfA(pThread->sKiller, SKIPCOUNT(pThread->sKiller) "%s:%i", szKiller, anLine);
		pThread->bActive = FALSE;
	}
	return bRc;
}

bool wasTerminateThreadCalled()
{
	return (g_TerminatedThreadIdx != -1);
}


#if defined(_MSC_VER) && !defined(CONEMU_MINIMAL)
const DWORD MS_VC_EXCEPTION = MS_VC_THREADNAME_EXCEPTION/*0x406D1388*/;

#pragma pack(push,8)
struct THREADNAME_INFO
{
   DWORD  dwType;     // Must be 0x1000.
   LPCSTR szName;     // Pointer to name (in user addr space).
   DWORD  dwThreadID; // Thread ID (-1=caller thread).
   DWORD  dwFlags;    // Reserved for future use, must be zero.
};
#pragma pack(pop)

void SetThreadName(DWORD dwThreadID, char* threadName)
{
   THREADNAME_INFO info = {0x1000, threadName, dwThreadID};

   __try
   {
      RaiseException(MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info);
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
   }
}
#endif
