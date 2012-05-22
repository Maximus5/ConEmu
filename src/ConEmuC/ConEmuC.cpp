
/*
Copyright (c) 2009-2012 Maximus5
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


#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после запуска процесса (conemuc.exe) показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
#elif defined(__GNUC__)
//  Раскомментировать, чтобы сразу после запуска процесса (conemuc.exe) показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
#else
//
#endif

#include <windows.h>

//#pragma warning( disable : 4995 )
#include "../common/common.hpp"

#include "../ConEmuCD/ExitCodes.h"

PHANDLER_ROUTINE gfHandlerRoutine = NULL;

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	if (gfHandlerRoutine)
	{
		// Вызов функи из ConEmuHk.dll
		gfHandlerRoutine(dwCtrlType);
	}

	return TRUE;
}

// 04.12.2009 Maks + "__cdecl"
int main(
    //#if defined(__GNUC__)
    int argc, char** argv
    //#else
    //HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow
    //#endif
)
{
	int iRc = 0;
	HMODULE hConEmu;
	char szErrInfo[512];
	DWORD dwErr, dwOut;
	typedef int (__stdcall* ConsoleMain2_t)(BOOL abAlternative);
	ConsoleMain2_t lfConsoleMain2;
	// Обязательно, иначе по CtrlC мы свалимся
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine, true);
#if defined(SHOW_STARTED_MSGBOX) || defined(SHOW_COMSPEC_STARTED_MSGBOX)

	if (!IsDebuggerPresent())
	{
		wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC Loaded (PID=%i)", GetCurrentProcessId());
		const wchar_t* pszCmdLine = GetCommandLineW();
		MessageBox(NULL,pszCmdLine,szTitle,0);
	}

#endif
	
	//wchar_t szSkipEventName[128];
	//_wsprintf(szSkipEventName, SKIPLEN(countof(szSkipEventName)) CEHOOKDISABLEEVENT, GetCurrentProcessId());
	//HANDLE hSkipEvent = CreateEvent(NULL, TRUE, TRUE, szSkipEventName);
#ifdef WIN64
	hConEmu = LoadLibrary(L"ConEmuCD64.dll");
#else
	hConEmu = LoadLibrary(L"ConEmuCD.dll");
#endif
	dwErr = GetLastError();
	//CloseHandle(hSkipEvent); hSkipEvent = NULL;

	if (!hConEmu)
	{
		_wsprintfA(szErrInfo, SKIPLEN(countof(szErrInfo))
		           "Can't load library \"%s\", ErrorCode=0x%08X\n",
#ifdef WIN64
		           "ConEmuCD64.dll",
#else
		           "ConEmuCD.dll",
#endif
		           dwErr);
		WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), szErrInfo, lstrlenA(szErrInfo), &dwOut, NULL);
		return CERR_CONEMUHK_NOTFOUND;
	}

	// Загрузить функи из ConEmuHk
	lfConsoleMain2 = (ConsoleMain2_t)GetProcAddress(hConEmu, "ConsoleMain2");
	gfHandlerRoutine = (PHANDLER_ROUTINE)GetProcAddress(hConEmu, "HandlerRoutine");

	if (!lfConsoleMain2 || !gfHandlerRoutine)
	{
		dwErr = GetLastError();
		_wsprintfA(szErrInfo, SKIPLEN(countof(szErrInfo))
		           "Procedure \"%s\"  not found in library \"%s\"",
		           lfConsoleMain2 ? "HandlerRoutine" : "ConsoleMain2",
#ifdef WIN64
		           "ConEmuHk64.dll"
#else
		           "ConEmuHk.dll"
#endif
		          );
		WriteConsoleW(GetStdHandle(STD_ERROR_HANDLE), szErrInfo, lstrlenA(szErrInfo), &dwOut, NULL);
		FreeLibrary(hConEmu);
		return CERR_CONSOLEMAIN_NOTFOUND;
	}

	// Main dll entry point for Server & ComSpec
	iRc = lfConsoleMain2(0/*WorkMode*/);
	// Exiting
	gfHandlerRoutine = NULL;
	//FreeLibrary(hConEmu); -- Shutdown Server/Comspec уже выполнен
	ExitProcess(iRc);
	return iRc;
}
