
/*
Copyright (c) 2009-2010 Maximus5
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

typedef struct HookCallbackArg
{
	BOOL      bMainThread;
	// pointer to variable with result of original function
	LPVOID    lpResult;
	// arguments (or pointers to them) of original funciton, casted to DWORD_PTR
	DWORD_PTR lArguments[10];
} HookCallbackArg;

// PreCallBack may returns (FALSE) to skip original function calling,
//		in that case, caller got pArgs->lResult
typedef BOOL (WINAPI* HookItemPreCallback_t)(HookCallbackArg* pArgs);
// PostCallBack can modify pArgs->lResult only
typedef VOID (WINAPI* HookItemPostCallback_t)(HookCallbackArg* pArgs);
// ExceptCallBack can modify pArgs->lResult only
typedef VOID (WINAPI* HookItemExceptCallback_t)(HookCallbackArg* pArgs);

typedef struct HookItem
{
	// Calling function must set only this 3 fields
	// These fields must be valid for lifetime
    const void*    NewAddress;
    const char*    Name;
    const wchar_t* DllName;
	//
	DWORD   nOrdinal;      // Ordinal of the procedure // !!! Это не Ordinal, а Hint !
    
    
	// Next fields are for internal use only!
	HMODULE hDll;          // handle of DllName
    void*   OldAddress;    // Original address of function from hDll
    #ifdef _DEBUG
	BOOL    ReplacedInExe; // debug information only
	#endif
	BOOL    InExeOnly;     // replace only in Exe module (FAR sort functions)
	
	// Stored for internal use in GetOriginalAddress
	// Some other dll's may modify procaddress (Anamorphosis, etc.)
	void*   ExeOldAddress; // function address from executable module (may be replaced by other libraries)
	
	// 'll be called before and after 'real' function
	HookItemPreCallback_t	 PreCallBack;
	HookItemPostCallback_t	 PostCallBack;
	HookItemExceptCallback_t ExceptCallBack;
} HookItem;


#if defined(__GNUC__)
extern "C"{
#endif

bool __stdcall SetHookCallbacks( const char* ProcName, const wchar_t* DllName, HMODULE hCallbackModule,
								 HookItemPreCallback_t PreCallBack, HookItemPostCallback_t PostCallBack,
								 HookItemExceptCallback_t ExceptCallBack = NULL );

//void* __stdcall GetOriginalAddress( void* OurFunction, void* DefaultFunction, BOOL abAllowModified );

// apHooks->Name && apHooks->DllName MUST be static for a lifetime
bool __stdcall InitHooks( HookItem* apHooks );

// All *aszExcludedModules must be valid all time
bool __stdcall SetAllHooks( HMODULE ahOurDll, const wchar_t** aszExcludedModules = NULL );

#if defined(__GNUC__)
}
#endif

void __stdcall UnsetAllHooks( );


typedef VOID (WINAPI* OnLibraryLoaded_t)(HMODULE ahModule);
//extern OnLibraryLoaded_t gfOnLibraryLoaded;
void __stdcall SetLoadLibraryCallback(HMODULE ahCallbackModule, OnLibraryLoaded_t afOnLibraryLoaded, OnLibraryLoaded_t afOnLibraryUnLoaded);


#if __GNUC__
extern "C" {
	#ifndef GetConsoleAliases
		DWORD WINAPI GetConsoleAliasesA(LPSTR AliasBuffer, DWORD AliasBufferLength, LPSTR ExeName);
		DWORD WINAPI GetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);
		#define GetConsoleAliases  GetConsoleAliasesW
	#endif
}
#endif
