
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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


#define DROP_SETCP_ON_WIN2K3R2

// Иначе не опередяется GetConsoleAliases (хотя он должен быть доступен в Win2k)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <TCHAR.h>
#include <Tlhelp32.h>
#include <shlwapi.h>
#include "..\common\common.hpp"
#include "..\common\ConEmuCheck.h"
#include "..\common\SetHook.h"

#ifdef _DEBUG
	#include <crtdbg.h>
#else
	#ifndef _ASSERTE
		#define _ASSERTE(x)
	#endif
#endif


static TCHAR kernel32[] = _T("kernel32.dll");
//static TCHAR user32[]   = _T("user32.dll");
static TCHAR shell32[]  = _T("shell32.dll");

//static BOOL bHooksWin2k3R2Only = FALSE;
//static HookItem HooksWin2k3R2Only[] = {
//	{OnSetConsoleCP, "SetConsoleCP", kernel32, 0},
//	{OnSetConsoleOutputCP, "SetConsoleOutputCP", kernel32, 0},
//	/* ************************ */
//	{0, 0, 0}
//};

extern int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);

static HookItem HooksFarOnly[] = {
//	{OnlstrcmpiA,      "lstrcmpiA",      kernel32, 0},
	{(void*)OnCompareStringW, "CompareStringW", kernel32, 0},
	/* ************************ */
	{0, 0, 0}
};


//void UnsetAllHooks()
//{
//	UnsetHook( HooksFarOnly, NULL, TRUE );
//	
//	/*if (bHooksWin2k3R2Only) {
//		bHooksWin2k3R2Only = FALSE;
//		UnsetHook( HooksWin2k3R2Only, NULL, TRUE );
//	}*/
//	
//    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
//    if(snapshot != INVALID_HANDLE_VALUE)
//    {
//        MODULEENTRY32 module = {sizeof(module)};
//		BOOL lbExecutable = TRUE;
//
//        for(BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
//        {
//            if(module.hModule != ghPluginModule && !IsModuleExcluded(module.hModule))
//            {
//                DebugString( module.szModule );
//                UnsetHook( Hooks, module.hModule, lbExecutable );
//				if (lbExecutable) lbExecutable = FALSE;
//            }
//        }
//
//        CloseHandle(snapshot);
//    }
//}


extern BOOL WINAPI OnConsoleDetaching(HookCallbackArg* pArgs);
extern VOID WINAPI OnConsoleWasAttached(HookCallbackArg* pArgs);
extern BOOL WINAPI OnConsolePeekInput(HookCallbackArg* pArgs);
extern VOID WINAPI OnConsolePeekInputPost(HookCallbackArg* pArgs);
extern BOOL WINAPI OnConsoleReadInput(HookCallbackArg* pArgs);
extern VOID WINAPI OnConsoleReadInputPost(HookCallbackArg* pArgs);
extern BOOL WINAPI OnWriteConsoleOutput(HookCallbackArg* pArgs);
//extern VOID WINAPI OnWasWriteConsoleOutputA(HookCallbackArg* pArgs);
//extern VOID WINAPI OnWasWriteConsoleOutputW(HookCallbackArg* pArgs);
extern VOID WINAPI OnGetNumberOfConsoleInputEventsPost(HookCallbackArg* pArgs);
extern VOID WINAPI OnShellExecuteExW_Except(HookCallbackArg* pArgs);



// Эту функцию нужно позвать из DllMain плагина
BOOL StartupHooks(HMODULE ahOurDll)
{
	InitHooks( HooksFarOnly );
	
	SetHookCallbacks( "FreeConsole",  kernel32, OnConsoleDetaching, NULL );
	SetHookCallbacks( "AllocConsole", kernel32, NULL, OnConsoleWasAttached );

	SetHookCallbacks( "PeekConsoleInputA", kernel32, OnConsolePeekInput, OnConsolePeekInputPost );
	SetHookCallbacks( "PeekConsoleInputW", kernel32, OnConsolePeekInput, OnConsolePeekInputPost );
	
	SetHookCallbacks( "ReadConsoleInputA", kernel32, OnConsoleReadInput, OnConsoleReadInputPost );
	SetHookCallbacks( "ReadConsoleInputW", kernel32, OnConsoleReadInput, OnConsoleReadInputPost );

	SetHookCallbacks( "WriteConsoleOutputA", kernel32, OnWriteConsoleOutput, NULL );
	SetHookCallbacks( "WriteConsoleOutputW", kernel32, OnWriteConsoleOutput, NULL );
	
	SetHookCallbacks( "GetNumberOfConsoleInputEvents", kernel32, NULL, OnGetNumberOfConsoleInputEventsPost );

	SetHookCallbacks( "ShellExecuteExW", shell32, NULL, NULL, OnShellExecuteExW_Except );
	
	return SetAllHooks(ahOurDll);

	//HMODULE hKernel = GetModuleHandle( kernel32 );
	//HMODULE hUser   = GetModuleHandle( user32 );
	//HMODULE hShell  = GetModuleHandle( shell32 );
	//if (!hShell) hShell = LoadLibrary ( shell32 );
	//_ASSERTE(hKernel && hUser && hShell);
	//if (!hKernel || !hUser || !hShell)
	//	return FALSE; // модули должны быть загружены ДО conemu.dll
	//
	//OSVERSIONINFOEX osv = {sizeof(OSVERSIONINFOEX)};
	//GetVersionEx((LPOSVERSIONINFO)&osv);
	//
	//// Заполнить поле HookItem.OldAddress (реальные процедуры из внешних библиотек)
	//InitHooks( Hooks );
	//InitHooks( HooksFarOnly );
	////InitHooks( HooksWin2k3R2Only );
	//
	////  Подменить Импортируемые функции в FAR.exe (пока это только сравнивание строк)
	//SetHook( HooksFarOnly, NULL, TRUE );
	//
	//// Windows Server 2003 R2
	//
	///*if (osv.dwMajorVersion==5 && osv.dwMinorVersion==2 && osv.wServicePackMajor>=2)
	//{
	//	//DWORD dwBuild = GetSystemMetrics(SM_SERVERR2); // нихрена оно не возвращает. 0 тут :(
	//	bHooksWin2k3R2Only = TRUE;
	//	SetHook( HooksWin2k3R2Only, NULL );
	//}*/
	//
	//// Подменить Импортируемые функции во всех модулях процесса, загруженных ДО conemu.dll
	//SetHookEx( Hooks, ghPluginModule );
	//
	//// Заменить в модуле Module ЭКСпортируемые функции на подменяемые плагином нихрена
	//// НЕ получится, т.к. в Win32 библиотека shell32 может быть загружена ПОСЛЕ conemu.dll
	////   что вызовет некорректные смещения функций,
	//// а в Win64 смещения вообще должны быть 64битными, а структура модуля хранит только 32битные смещения
	//
	//return TRUE;
}


void ShutdownHooks()
{
	UnsetAllHooks();
}
