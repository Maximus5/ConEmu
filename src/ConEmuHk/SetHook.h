
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

//#ifdef _DEBUG
//#define HOOK_ANSI_SEQUENCES
//#else
//	#undef HOOK_ANSI_SEQUENCES
//#endif


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

#ifdef _DEBUG
#include "DbgHooks.h"
#endif

extern const wchar_t *kernel32;// = L"kernel32.dll";
extern const wchar_t *user32  ;// = L"user32.dll";
extern const wchar_t *gdi32  ;// = L"gdi32.dll";
extern const wchar_t *shell32 ;// = L"shell32.dll";
extern const wchar_t *advapi32;// = L"Advapi32.dll";
extern const wchar_t *comdlg32;// = L"comdlg32.dll";
extern HMODULE ghKernel32, ghUser32, ghGdi32, ghShell32, ghAdvapi32, ghComdlg32;

typedef LONG (WINAPI* RegCloseKey_t)(HKEY hKey);
extern RegCloseKey_t RegCloseKey_f;
typedef LONG (WINAPI* RegOpenKeyEx_t)(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
extern RegOpenKeyEx_t RegOpenKeyEx_f;
typedef LONG (WINAPI* RegCreateKeyEx_t)(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
extern RegCreateKeyEx_t RegCreateKeyEx_f;

typedef BOOL (WINAPI* OnChooseColorA_t)(LPCHOOSECOLORA lpcc);
extern OnChooseColorA_t ChooseColorA_f;
typedef BOOL (WINAPI* OnChooseColorW_t)(LPCHOOSECOLORW lpcc);
extern OnChooseColorW_t ChooseColorW_f;

typedef struct HookCallbackArg
{
	BOOL         bMainThread;
	//HookExeOnly  IsExecutable;
	// pointer to variable with result of original function
	LPVOID       lpResult;
	// arguments (or pointers to them) of original funciton, casted to DWORD_PTR
	DWORD_PTR    lArguments[10];
} HookCallbackArg;

// PreCallBack may returns (FALSE) to skip original function calling,
//		in that case, caller got pArgs->lResult
typedef BOOL (WINAPI* HookItemPreCallback_t)(HookCallbackArg* pArgs);
// PostCallBack can modify pArgs->lResult only
typedef VOID (WINAPI* HookItemPostCallback_t)(HookCallbackArg* pArgs);
// ExceptCallBack can modify pArgs->lResult only
typedef VOID (WINAPI* HookItemExceptCallback_t)(HookCallbackArg* pArgs);


//struct HookItemLt
//{
//	// Calling function must set only this 3 fields
//	// These fields must be valid for lifetime
//	const void*     NewAddress;
//	const char*     Name;
//	const wchar_t*  DllName;
//};

struct HookItem
{
	// Calling function must set only this 3 fields
	// These fields must be valid for lifetime
	const void*     NewAddress;
	const char*     Name;
	const wchar_t*  DllName;
	// Some functions are not exported by name,
	// but only by ordinal, e.g. "ShellExecCmdLine"
	DWORD           NameOrdinal;

	// *** following members are not initialized on InitHooks call ***

	char DllNameA[32];

	DWORD NameCRC;

#ifdef _WIN64
	DWORD Pad1;
#endif
	
	HMODULE hDll;          // handle of DllName
	void*   HookedAddress; // Original exported function
	void*   CallAddress;   // Address to call original function

	// 'll be called before and after 'real' function
	HMODULE                  hCallbackModule;
	HookItemPreCallback_t	 PreCallBack;
	HookItemPostCallback_t	 PostCallBack;
	HookItemExceptCallback_t ExceptCallBack;
};

typedef VOID (WINAPI* OnLibraryLoaded_t)(HMODULE ahModule);

struct HookModeFar
{
	DWORD cbSize;            // размер структуры
	BOOL  bFarHookMode;      // устанавливается в TRUE из плагина ConEmu.dll !!! MUST BE FIRST BOOL !!!
	BOOL  bFARuseASCIIsort;  // -> OnCompareStringW
	BOOL  bShellNoZoneCheck; // -> OnShellExecuteXXX
	BOOL  bMonitorConsoleInput; // при (Read/Peek)ConsoleInput(A/W) послать инфу в GUI/Settings/Debug
	BOOL  bPopupMenuPos;     // при вызове EMenu показать меню в позиции мышиного курсора
	BOOL  bLongConsoleOutput; // при выполнении консольных программ из Far - увеличивать высоту буфера
	void  (WINAPI* OnCurDirChanged)();
};

#if defined(EXTERNAL_HOOK_LIBRARY) && !defined(DEFINE_HOOK_MACROS)

typedef bool (__stdcall* SetHookCallbacks_t)(const char* ProcName, const wchar_t* DllName, HMODULE hCallbackModule,
        HookItemPreCallback_t PreCallBack, HookItemPostCallback_t PostCallBack,
        HookItemExceptCallback_t ExceptCallBack);
extern SetHookCallbacks_t SetHookCallbacks; // = NULL;

typedef void (__stdcall* SetLoadLibraryCallback_t)(HMODULE ahCallbackModule,
        OnLibraryLoaded_t afOnLibraryLoaded, OnLibraryLoaded_t afOnLibraryUnLoaded);
extern SetLoadLibraryCallback_t SetLoadLibraryCallback; // = NULL;

typedef void (__stdcall* SetFarHookMode_t)(struct HookModeFar *apFarMode);
extern SetFarHookMode_t SetFarHookMode;

#else // #ifdef EXTERNAL_HOOK_LIBRARY

#if defined(__GNUC__)
extern "C" {
#endif
	extern void __stdcall SetFarHookMode(struct HookModeFar *apFarMode);

	bool __stdcall SetHookCallbacks(const char* ProcName, const wchar_t* DllName, HMODULE hCallbackModule,
	                                HookItemPreCallback_t PreCallBack, HookItemPostCallback_t PostCallBack,
	                                HookItemExceptCallback_t ExceptCallBack = NULL);
#if defined(__GNUC__)
}
#endif

int  InitHooks(HookItem* apHooks);
bool SetAllHooks();
void UnsetAllHooks();


//typedef VOID (WINAPI* OnLibraryLoaded_t)(HMODULE ahModule);
//extern OnLibraryLoaded_t gfOnLibraryLoaded;
#if defined(__GNUC__)
extern "C" {
#endif
	void __stdcall SetLoadLibraryCallback(HMODULE ahCallbackModule, OnLibraryLoaded_t afOnLibraryLoaded, OnLibraryLoaded_t afOnLibraryUnLoaded);
#if defined(__GNUC__)
};
#endif


#if __GNUC__
extern "C" {
#ifndef GetConsoleAliases
	DWORD WINAPI GetConsoleAliasesA(LPSTR AliasBuffer, DWORD AliasBufferLength, LPSTR ExeName);
	DWORD WINAPI GetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);
#define GetConsoleAliases  GetConsoleAliasesW
#endif
}
#endif


class CInFuncCall
{
	public:
		int *mpn_Counter;
	public:
		CInFuncCall();
		BOOL Inc(int* pnCounter);
		~CInFuncCall();
};


#ifdef DEFINE_HOOK_MACROS
void* __cdecl GetOriginalAddress(void* OurFunction, HookItem** ph, bool abAllowNulls = false);

#define ORIGINALFASTEX(n,o) \
	static HookItem *ph = NULL; \
	static void* f##n = NULL; \
	ORIGINALSHOWCALL(n); \
	if ((f##n)==NULL) f##n = (void*)GetOriginalAddress((void*)(On##n), &ph); \
	_ASSERTE((void*)(On##n)!=(void*)(f##n) && (void*)(f##n)!=NULL);

#define ORIGINALFAST(n) \
	ORIGINALFASTEX(n,n)

#define ORIGINAL(n) \
	BOOL bMainThread = (GetCurrentThreadId() == gnHookMainThreadId); \
	ORIGINALFAST(n)

#define F(n) ((On##n##_t)f##n)

#endif

#endif // #else // EXTERNAL_HOOK_LIBRARY

#define SETARGS(r) HookCallbackArg args = {bMainThread}; args.lpResult = (LPVOID)(r)
#define SETARGS1(r,a1) SETARGS(r); args.lArguments[0] = (DWORD_PTR)(a1)
#define SETARGS2(r,a1,a2) SETARGS1(r,a1); args.lArguments[1] = (DWORD_PTR)(a2)
#define SETARGS3(r,a1,a2,a3) SETARGS2(r,a1,a2); args.lArguments[2] = (DWORD_PTR)(a3)
#define SETARGS4(r,a1,a2,a3,a4) SETARGS3(r,a1,a2,a3); args.lArguments[3] = (DWORD_PTR)(a4)
#define SETARGS5(r,a1,a2,a3,a4,a5) SETARGS4(r,a1,a2,a3,a4); args.lArguments[4] = (DWORD_PTR)(a5)
#define SETARGS6(r,a1,a2,a3,a4,a5,a6) SETARGS5(r,a1,a2,a3,a4,a5); args.lArguments[5] = (DWORD_PTR)(a6)
#define SETARGS7(r,a1,a2,a3,a4,a5,a6,a7) SETARGS6(r,a1,a2,a3,a4,a5,a6); args.lArguments[6] = (DWORD_PTR)(a7)
#define SETARGS8(r,a1,a2,a3,a4,a5,a6,a7,a8) SETARGS7(r,a1,a2,a3,a4,a5,a6,a7); args.lArguments[7] = (DWORD_PTR)(a8)
#define SETARGS9(r,a1,a2,a3,a4,a5,a6,a7,a8,a9) SETARGS8(r,a1,a2,a3,a4,a5,a6,a7,a8); args.lArguments[8] = (DWORD_PTR)(a9)
#define SETARGS10(r,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) SETARGS9(r,a1,a2,a3,a4,a5,a6,a7,a8,a9); args.lArguments[9] = (DWORD_PTR)(a10)
// !!! WARNING !!! DWORD_PTR lArguments[10]; - пока максимум - 10 аргументов
