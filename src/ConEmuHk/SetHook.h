
/*
Copyright (c) 2009-2017 Maximus5
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

#include "../common/MMap.h"
#include "DbgHooks.h"

//#ifdef _DEBUG
//#define HOOK_ANSI_SEQUENCES
//#else
//	#undef HOOK_ANSI_SEQUENCES
//#endif

#define CSECTION_NON_RAISE


#ifndef ORIGINALSHOWCALL
	#ifdef _DEBUG
		#define LOG_ORIGINAL_CALL
		//#undef LOG_ORIGINAL_CALL
	#else
		#undef LOG_ORIGINAL_CALL
	#endif

	#ifdef LOG_ORIGINAL_CALL
		extern bool gbSuppressShowCall;
		void LogFunctionCall(LPCSTR asFunc, LPCSTR asFile, int anLine);
		#define ORIGINALSHOWCALL(n) LogFunctionCall(#n, __FILE__, __LINE__)
		#define SUPPRESSORIGINALSHOWCALL gbSuppressShowCall = true
		#define _ASSERTRESULT(x) //DWORD dwResultLastErr = GetLastError(); if (!(x) || (dwResultLastErr==ERROR_INVALID_DATA)) { _ASSERTEX((x) && dwResultLastErr==0); }
	#else
		#define ORIGINALSHOWCALL(n)
		#define SUPPRESSORIGINALSHOWCALL
		#define _ASSERTRESULT(x)
	#endif
#endif

//enum HookExeOnly
//{
//	HEO_Undefined  = 0,
//	HEO_Executable = 1,
//	HEO_Module     = 2,
//};

//extern const wchar_t *kernel32;// = L"kernel32.dll";
//extern const wchar_t *user32  ;// = L"user32.dll";
//extern const wchar_t *shell32 ;// = L"shell32.dll";
//#define kernel32 L"kernel32.dll"
//#define user32   L"user32.dll"
//#define shell32  L"shell32.dll"
//extern HMODULE ghKernel32, ghUser32, ghShell32;

extern const wchar_t *kernel32;// = L"kernel32.dll";
extern const wchar_t *user32  ;// = L"user32.dll";
extern const wchar_t *gdi32  ;// = L"gdi32.dll";
extern const wchar_t *shell32 ;// = L"shell32.dll";
extern const wchar_t *advapi32;// = L"Advapi32.dll";
extern HMODULE ghKernel32, ghUser32, ghGdi32, ghShell32, ghAdvapi32;

// Used by SETARGSx macros in ConEmuHooks.h
#define hkIsMainThread (GetCurrentThreadId() == gnHookMainThreadId)

#include "ConEmuHooks.h"
#include "hlpProcess.h"

#if defined(__GNUC__)
extern "C" {
#endif
	HWND WINAPI GetRealConsoleWindow();
	FARPROC WINAPI GetWriteConsoleW();
	int WINAPI RequestLocalServer(/*[IN/OUT]*/RequestLocalServerParm* Parm);
	FARPROC WINAPI GetLoadLibraryW();

	extern void __stdcall SetFarHookMode(struct HookModeFar *apFarMode);

	bool __stdcall SetHookCallbacks(const char* ProcName, const wchar_t* DllName, HMODULE hCallbackModule,
	                                HookItemPreCallback_t PreCallBack, HookItemPostCallback_t PostCallBack,
	                                HookItemExceptCallback_t ExceptCallBack = NULL);

#if defined(__GNUC__)
};
#endif


#define MAX_HOOKED_PROCS 255

struct HookItem
{
	HMODULE hDll;          // handle of DllName
	void*   HookedAddress; // Original exported function
	void*   CallAddress;   // Address to call original function

	// Calling function must set only this 3 fields
	// These fields must be valid for lifetime
	const void*     NewAddress;
	const char*     Name;
	const wchar_t*  DllName;
	// Some functions are not exported by name,
	// but only by ordinal, e.g. "ShellExecCmdLine"
	DWORD           NameOrdinal;
	// Pointer to function ID in our gpHooks
	LPDWORD         pnFnID;
	LONG            Counter;

	// *** following members are not initialized on InitHooks call ***

	char DllNameA[32];

	DWORD NameCRC;

#ifdef _WIN64
	DWORD Pad1;
#endif

	// 'll be called before and after 'real' function
	HMODULE                  hCallbackModule;
	HookItemPreCallback_t	 PreCallBack;
	HookItemPostCallback_t	 PostCallBack;
	HookItemExceptCallback_t ExceptCallBack;
};


// We can't use GetModuleFileName or CreateToolhelp32Snapshot during dll loading
// Also, we need to know which dll-s were processed already
struct HkModuleInfo
{
	HMODULE hModule;
	wchar_t sModuleName[128]; // If we need to know the name of module

	// For our modules we change imports to ease use of trampolines
	// 0-not yet, 1-imports were changed, 2-imports were reverted
	int     Hooked;
	UINT    nAdrUsed;
	// And the changed pointers
	struct ChangedFunctions {
		DWORD_PTR* ppAdr;
		#ifdef _DEBUG
		DWORD_PTR ppAdrCopy1, ppAdrCopy2;
		DWORD_PTR pModulePtr, nModuleSize;
		#endif
		DWORD_PTR  pOld;
		DWORD_PTR  pOur;
		union {
			BOOL bHooked;
			LPVOID Dummy;
		};
		#ifdef _DEBUG
		char sName[32];
		#endif
	} Addresses[MAX_HOOKED_PROCS];
};

typedef MMap<HMODULE,HkModuleInfo*,16> HkModuleMap;
extern HkModuleMap* gpHookedModules;

bool InitializeHookedModules();
void FinalizeHookedModules();


#define F(n) ((On##n##_t)f##n)

#define HOOK_FN_TYPE(n) On##n##_t
#define HOOK_FN_NAME(n) On##n
#define HOOK_FN_STR(n) #n
#define HOOK_FN_ID(n) g_##n##_ID

#ifdef DECLARE_CONEMU_HOOK_FUNCTION_ID
	#define HOOK_FUNCTION_ID(n) DWORD HOOK_FN_ID(n) = 0
#else
	#define HOOK_FUNCTION_ID(n) extern DWORD HOOK_FN_ID(n)
#endif

// Some kernel function must be hooked in the kernel32.dll itself (ExitProcess)
#define HOOK_ITEM_BY_LIBR(fn,dll,ord,hlib) \
	{hlib, NULL, NULL, (void*)HOOK_FN_NAME(fn), HOOK_FN_STR(fn), dll, ord, &HOOK_FN_ID(fn)}

// Some function may be hooked by ordinal only, there are no named exports (ShellExecCmdLine)
#define HOOK_ITEM_BY_ORDN(fn,dll,ord) \
	HOOK_ITEM_BY_LIBR(fn,dll,ord,NULL)

// Standard hook declaration for HookItem
#define HOOK_ITEM_BY_NAME(fn,dll) HOOK_ITEM_BY_ORDN(fn,dll,0)


// For example: HOOK_PROTOTYPE(BOOL,WINAPI,FlashWindow,(HWND hWnd, BOOL bInvert))
#define HOOK_PROTOTYPE(fn,ret,call,args) \
	HOOK_FUNCTION_ID(fn); \
	typedef ret (call* HOOK_FN_TYPE(fn)) args; \
	ret call HOOK_FN_NAME(fn) args

void* __cdecl GetOriginalAddress(void* OurFunction, DWORD nFnID = 0, void* ApiFunction = NULL, HookItem** ph = NULL, bool abAllowNulls = false);

bool StartupHooks();
void ShutdownHooks();

int  InitHooks(HookItem* apHooks);
bool SetAllHooks();
void UnsetAllHooks();

// following two are defined in ConEmuHooks.cpp, but declared here
bool InitHooksCommon();
bool InitHooksDefTerm();

bool SetImports(LPCWSTR asModule, HMODULE Module, BOOL abForceHooks);
void UnsetImports();

bool PrepareNewModule(HMODULE module, LPCSTR asModuleA, LPCWSTR asModuleW, BOOL abNoSnapshot = FALSE, BOOL abForceHooks = FALSE);
void UnprepareModule(HMODULE hModule, LPCWSTR pszModule, int iStep);

void ProcessOnLibraryLoadedW(HMODULE module);

//typedef VOID (WINAPI* OnLibraryLoaded_t)(HMODULE ahModule);
//extern OnLibraryLoaded_t gfOnLibraryLoaded;
#if defined(__GNUC__)
extern "C" {
#endif
	void __stdcall SetLoadLibraryCallback(HMODULE ahCallbackModule, OnLibraryLoaded_t afOnLibraryLoaded, OnLibraryLoaded_t afOnLibraryUnLoaded);
#if defined(__GNUC__)
};
#endif



// Originally from http://preshing.com/20120522/lightweight-in-memory-logging
namespace HookLogger
{
	#define HOOK_LOG_MAX 1024 // Must be a power of 2
	struct FnCall
	{
		DWORD nFnID;
		DWORD nTID;
	};
	extern FnCall g_calls[HOOK_LOG_MAX];
	extern LONG   g_callsidx;

	inline void Log(DWORD index)
	{
		// Get next index
		LONG i = _InterlockedIncrement(&g_callsidx);
		// Prepare values
		FnCall FN = {index, GetCurrentThreadId()};
		// Write at this index, Wrap to buffer size
		g_calls[i & (HOOK_LOG_MAX - 1)] = FN;
	}
}

#ifdef _DEBUG
class COriginalCallCount
{
protected:
	LONG* mp_ThreadId;
	LONG* mp_Count;
protected:
	static LONG mn_LastTID;
	static LONG mn_LastFnID;
public:
	COriginalCallCount(LONG* pThreadId, LONG* pCount, LONG nFnID, LPCSTR asFnName);
	~COriginalCallCount();
};
#define ORIGINALCALLCOUNT_(n,id) \
	static LONG cnt_thread_##n, cnt_##n; \
	COriginalCallCount cnt##n(&cnt_thread_##n, &cnt_##n, id, #n)
#define ORIGINALCALLCOUNT(n) \
	ORIGINALCALLCOUNT_(n, HOOK_FN_ID(n))
#else
#define ORIGINALCALLCOUNT(n)
#define ORIGINALCALLCOUNT_(n,id)
#endif

#define ORIGINAL(n,o) \
	HookItem *ph = NULL; \
	void* f##n = NULL; \
	extern DWORD HOOK_FN_ID(n); \
	ORIGINALCALLCOUNT(n); \
	ORIGINALSHOWCALL(n); \
	if ((f##n)==NULL) f##n = GetOriginalAddress((void*)(On##n), HOOK_FN_ID(n), (void*)(o), &ph, false); \
	_ASSERTE((void*)(On##n)!=(void*)(f##n) && (void*)(f##n)!=NULL);

#define ORIGINAL_KRNL(n) \
	ORIGINAL(n,n)

// Some functions were not present in Win2k, WinXP, ...
#define ORIGINAL_KRNL_EX(n) \
	ORIGINAL(n,NULL)

// Use this to avoid static link to functions
#define ORIGINAL_EX(n) \
	ORIGINAL(n,NULL)
