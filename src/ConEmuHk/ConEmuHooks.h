
/*
Copyright (c) 2009-2015 Maximus5
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


#pragma once

//#define DEBUGLOG(s) //DEBUGSTR(s)
//#define DEBUGLOGSIZE(s) DEBUGSTR(s)
//#define DEBUGLOGLANG(s) //DEBUGSTR(s) //; Sleep(2000)
//
//#ifdef _DEBUG
////CRITICAL_ SECTION gcsHeap;
////#define MCHKHEAP { Enter CriticalSection(&gcsHeap); int MDEBUG_CHK=_CrtCheckMemory(); _ASSERTE(MDEBUG_CHK); Leave Critical Section(&gcsHeap); }
////#define MCHKHEAP HeapValidate(ghHeap, 0, NULL);
////#define MCHKHEAP { int MDEBUG_CHK=_CrtCheckMemory(); _ASSERTE(MDEBUG_CHK); }
////#define HEAP_LOGGING
//#else
////#define MCHKHEAP
//#endif




#define CSECTION_NON_RAISE

#include <Windows.h>
#include <WinCon.h>


/*  Global  */
extern DWORD   gnSelfPID;
extern HWND    ghConWnd;
extern HWND    ghConEmuWnd; // Root! window
extern HWND    ghConEmuWndDC; // ConEmu DC window
extern DWORD   gnServerPID; // PID сервера (инициализируется на старте, при загрузке Dll)

#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"
#include "../common/WObjects.h"
#include "../common/InQueue.h"
#include "../common/MMap.h"
#include "../common/MFileMapping.h"

void SetServerPID(DWORD anMainSrvPID);

extern MFileMapping<CESERVER_CONSOLE_APP_MAPPING> *gpAppMap;
CESERVER_CONSOLE_MAPPING_HDR* GetConMap(BOOL abForceRecreate=FALSE);
void OnConWndChanged(HWND ahNewConWnd);
bool AttachServerConsole();
typedef BOOL (WINAPI* AttachConsole_t)(DWORD dwProcessId);
AttachConsole_t GetAttachConsoleProc();
void CheckAnsiConVar(LPCWSTR asName);

enum ConEmuHkDllState
{
	ds_Undefined = 0,
	ds_DllProcessAttach = 1,
	ds_DllMainThreadDetach = 2,
	ds_DllStop = 3,
	ds_DllProcessDetach = 4,
};
extern ConEmuHkDllState gnDllState;
extern int gnDllThreadCount;
extern BOOL gbDllStopCalled;

//int WINAPI RequestLocalServer(/*[IN/OUT]*/RequestLocalServerParm* Parm);
struct AnnotationHeader;
extern AnnotationHeader* gpAnnotationHeader;
extern HANDLE ghCurrentOutBuffer;

struct ReadConsoleInfo
{
	HANDLE hConsoleInput;
	DWORD InReadConsoleTID;
	DWORD LastReadConsoleTID;
	HANDLE hConsoleInput2;
	DWORD LastReadConsoleInputTID;
	BOOL  bIsUnicode;
	COORD crStartCursorPos;
	DWORD nConInMode;
	DWORD nConOutMode;
};
extern struct ReadConsoleInfo gReadConsoleInfo;
BOOL OnReadConsoleClick(SHORT xPos, SHORT yPos, bool bForce, bool bBashMargin);
BOOL OnPromptBsDeleteWord(bool bForce, bool bBashMargin);
BOOL OnExecutePromptCmd(LPCWSTR asCmd);

void CheckHookServer();
extern bool gbHookServerForcedTermination;

struct CpConv
{
	// for example, "git add -p" uses codepage 1252 while printing thunks to be staged
	// that forces the printed text to be converted to nToCP before printing (OnWriteConsoleW)
	DWORD nFromCP, nToCP;
	// that parm may be used for overriding default console CP
	DWORD nDefaultCP;
};
extern struct CpConv gCpConv;

/* ************ Globals for Far ************ */
extern bool    gbIsFarProcess;
extern InQueue gInQueue;
/* ************ Globals for Far ************ */

/* ************ Globals for cmd.exe/clink ************ */
extern bool     gbIsCmdProcess;
//extern size_t   gcchLastWriteConsoleMax;
//extern wchar_t *gpszLastWriteConsole;
extern int      gnCmdInitialized; // 0 - Not already, 1 - OK, -1 - Fail
extern bool     gbAllowClinkUsage;
extern bool     gbAllowUncPaths;
extern wchar_t* gszClinkCmdLine;
//extern bool     InitializeClink();
/* ************ Globals for cmd.exe/clink ************ */

/* ************ Globals for powershell ************ */
extern bool gbPowerShellMonitorProgress;
extern WORD gnConsolePopupColors;
extern int  gnPowerShellProgressValue;
/* ************ Globals for powershell ************ */

/* ************ Globals for Node.JS ************ */
extern bool gbIsNodeJSProcess;
/* ************ Globals for Node.JS ************ */

/* ************ Globals for cygwin/msys ************ */
extern bool gbIsBashProcess;
extern bool gbIsSshProcess;
extern bool gbIsLessProcess;
/* ************ Globals for cygwin/msys ************ */

/* ************ Globals for ViM ************ */
extern bool gbIsVimProcess;
extern bool gbIsVimAnsi;
/* ************ Globals for ViM ************ */

/* ************ Globals for MinTTY ************ */
extern bool gbIsMinTtyProcess;
/* ************ Globals for ViM ************ */

/* ************ Globals for HIEW32.EXE ************ */
extern bool gbIsHiewProcess;
/* ************ Globals for HIEW32.EXE ************ */

/* ************ Globals for DosBox.EXE ************ */
extern bool gbDosBoxProcess;
/* ************ Globals for DosBox.EXE ************ */

/* ************ Don't show VirtualAlloc errors ************ */
extern bool gbSkipVirtualAllocErr;
/* ************ Don't show VirtualAlloc errors ************ */

/* ************ Globals for "Default terminal ************ */
extern bool gbPrepareDefaultTerminal;
extern bool gbIsNetVsHost;
extern bool gbIsVStudio;
extern int  gnVsHostStartConsole;
extern bool gbIsGdbHost;
/* ************ Globals for "Default terminal ************ */

void GuiSetProgress(WORD st, WORD pr, LPCWSTR pszName = NULL);
BOOL GetConsoleScreenBufferInfoCached(HANDLE hConsoleOutput, PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo, BOOL bForced = FALSE);
BOOL GetConsoleModeCached(HANDLE hConsoleHandle, LPDWORD lpMode, BOOL bForced = FALSE);

#if defined(__GNUC__)
extern "C" {
#endif
	HWND WINAPI GetRealConsoleWindow();
	FARPROC WINAPI GetWriteConsoleW();
	int WINAPI RequestLocalServer(/*[IN/OUT]*/RequestLocalServerParm* Parm);
	FARPROC WINAPI GetLoadLibraryW();
	FARPROC WINAPI GetVirtualAlloc();
	FARPROC WINAPI GetTrampoline(LPCSTR pszName);
#if defined(__GNUC__)
};
#endif

void DoDllStop(bool bFinal, bool bFromTerminate = false);

#ifdef _DEBUG
	//#define USEHOOKLOG
	#undef USEHOOKLOG
	#ifdef USEHOOKLOG
		#define USEHOOKLOGANALYZE
	#else
		#undef USEHOOKLOGANALYZE
	#endif
#else
	#undef USEHOOKLOG
	#undef USEHOOKLOGANALYZE
#endif

#ifdef USEHOOKLOG
	#if !defined(__GNUC__) || defined(__MINGW64_VERSION_MAJOR)
	#include <intrin.h>
	#else
	#define _InterlockedIncrement InterlockedIncrement
	#endif

	#define getThreadId() WIN3264TEST(((DWORD*) __readfsdword(24))[9],GetCurrentThreadId())

	#define getTime GetTickCount

	// Originally from http://preshing.com/20120522/lightweight-in-memory-logging
	namespace HookLogger
	{
		struct Event
		{
			#ifdef _DEBUG
			DWORD tid;       // Thread ID
			DWORD dur;       // Step duration
			#endif
			const char* msg; // Info
			DWORD param;
			#ifdef _DEBUG
			LARGE_INTEGER cntr, cntr1; // PerformanceCounters
			#endif
		};
	 
		static const int BUFFER_SIZE = RELEASEDEBUGTEST(256,1024);   // Must be a power of 2
		extern Event g_events[BUFFER_SIZE];
		extern LONG g_pos;
		extern LARGE_INTEGER g_freq;

		static const int CRITICAL_BUFFER_SIZE = 256;
		struct CritInfo
		{
			u64 total;
			DWORD count;
			
			#ifdef _WIN64
			DWORD pad;
			#endif

			const char* msg;
		};
		extern CritInfo g_crit[CRITICAL_BUFFER_SIZE];
	 
		inline Event* Log(const char* msg, DWORD param)
		{
			// Get next event index
			LONG i = _InterlockedIncrement(&g_pos);
			// Write an event at this index
			Event* e = g_events + (i & (BUFFER_SIZE - 1));  // Wrap to buffer size
			#ifdef _DEBUG
			e->tid = getThreadId();
			//e->tick = getTime();
			QueryPerformanceCounter(&e->cntr);
			//e->tick = e->cntr.LowPart;
			//Event* ep = g_events + ((i-1) & (BUFFER_SIZE - 1));  // Wrap to buffer size
			//ep->dur = (DWORD)(e->cntr.QuadPart - ep->cntr.QuadPart);
			e->cntr1.QuadPart = 0;
			#endif
			e->msg = msg;
			e->param = param;
			return e;
		}

		extern void RunAnalyzer();
	}

	#undef getThreadId
	#undef getTime
#endif

#if defined(USEHOOKLOG) && !defined(SKIPHOOKLOG)
	#define HLOG0(m,p) HookLogger::Event* es = HookLogger::Log(m,p);
	#define HLOG(m,p) es = HookLogger::Log(m,p);
	#define HLOGEND() QueryPerformanceCounter(&es->cntr1);
	#define HLOG1(m,p) HookLogger::Event* es1 = HookLogger::Log(m,p);
	#define HLOG1_(m,p) es1 = HookLogger::Log(m,p);
	#define HLOGEND1() QueryPerformanceCounter(&es1->cntr1);
	#define HLOG2(m,p) HookLogger::Event* es2 = HookLogger::Log(m,p);
	#define HLOG2_(m,p) es2 = HookLogger::Log(m,p);
	#define HLOGEND2() QueryPerformanceCounter(&es2->cntr1);
#else
	#define HLOG0(m,p)
	#define HLOG(m,p)
	#define HLOGEND()
	#define HLOG1(m,p)
	#define HLOG1_(m,p)
	#define HLOGEND1()
	#define HLOG2(m,p)
	#define HLOG2_(m,p)
	#define HLOGEND2()
#endif

#if defined(USEHOOKLOG) && !defined(SKIPDLLLOG)
	#define DLOG0(m,p) HookLogger::Event* es = HookLogger::Log(m,p);
	#define DLOG(m,p) es = HookLogger::Log(m,p);
	#define DLOGEND() QueryPerformanceCounter(&es->cntr1);
	#define DLOG1(m,p) HookLogger::Event* es1 = HookLogger::Log(m,p);
	#define DLOG1_(m,p) es1 = HookLogger::Log(m,p);
	#define DLOGEND1() QueryPerformanceCounter(&es1->cntr1);
#else
	#define DLOG0(m,p)
	#define DLOG(m,p)
	#define DLOGEND()
	#define DLOG1(m,p)
	#define DLOG1_(m,p)
	#define DLOGEND1()
#endif
