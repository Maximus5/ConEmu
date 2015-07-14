
#pragma once

#define HIDE_USE_EXCEPTION_INFO
#include "common.hpp"
#include "WThreads.h"

#if !defined(__GNUC__) || defined(__MINGW64_VERSION_MAJOR)
#include <intrin.h>
#else
#define _InterlockedIncrement InterlockedIncrement
#endif

#define THREADS_LOG_SIZE 256 // must be power of 2
#define THREAD_MAX_NAME_LEN 40
static struct ConEmuThreadInfo
{
	BOOL   bActive;
	DWORD  nThreadID;
	HANDLE hThread;
	LPTHREAD_START_ROUTINE pStartAddress;
	DWORD  nParentThreadID;
	DWORD  nStartTick;
	DWORD  nEndTick;
	DWORD  nExitCode;
	BOOL   bTerminated;
	char   sName[THREAD_MAX_NAME_LEN];
} g_Threads[THREADS_LOG_SIZE];

LONG g_ThreadsIdx = 0;
LONG g_TerminatedThreadIdx = -1; // -2 if terminated thread was not found in g_Threads

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
};

DWORD WINAPI apiThreadHelper(LPVOID lpParameter)
{
	ConEmuThreadStartArg* p = (ConEmuThreadStartArg*)lpParameter;
	SetThreadName((DWORD)-1, p->sName);

	ConEmuThreadInfo Info = {
		TRUE,
		GetCurrentThreadId(),
		p->hThread,
		p->lpStartAddress,
		p->nParentTID,
		GetTickCount()
	};
	lstrcpynA(Info.sName, p->sName, countof(Info.sName));

	ConEmuThreadInfo* pInfo = NULL;
	for (INT_PTR c = THREADS_LOG_SIZE; --c >= 0;)
	{
		LONG i = _InterlockedIncrement(&g_ThreadsIdx);
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

BOOL apiTerminateThread(HANDLE hThread, DWORD dwExitCode)
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
			g_TerminatedThreadIdx = c;
		}
	}

	BOOL bRc = ::TerminateThread(hThread, dwExitCode);

	if (pThread)
		pThread->bActive = FALSE;
	else
		g_TerminatedThreadIdx = -2;
	return bRc;
}


#if defined(_MSC_VER) && !defined(CONEMU_MINIMAL)
const DWORD MS_VC_EXCEPTION = 0x406D1388;

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
