
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


#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после запуска процесса (conemuc.exe) показать MessageBox, чтобы прицепиться дебаггером
//	#define SHOW_STARTED_MSGBOX
#elif defined(__GNUC__)
//  Раскомментировать, чтобы сразу после запуска процесса (conemuc.exe) показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
#else
//
#endif

#include <windows.h>

//#pragma warning( disable : 4995 )
#include "../common/Common.h"
#include "../common/CmdLine.h"

#include "../ConEmu/version.h"
#include "../ConEmuCD/ExitCodes.h"
#include "../ConEmuCD/ConsoleHelp.h"

#include "ConEmuC.h"
#include "Downloader.h"

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

#ifdef _DEBUG
void UnitTests()
{
	typedef struct _UNICODE_STRING {
		USHORT Length;
		USHORT MaximumLength;
		PWSTR  Buffer;
	} UNICODE_STRING;
	UNICODE_STRING str = {};
	WCHAR buf[MAX_PATH];
	str.Buffer = (PWSTR)(((((DWORD_PTR)buf)+7)>>3)<<3);
	lstrcpy(str.Buffer, L"kernel32.dll");
	str.Length = lstrlen(str.Buffer)*2;
	str.MaximumLength = str.Length+2;

	typedef LONG (__stdcall* LdrGetDllHandleByName_t)(UNICODE_STRING* BaseDllName, UNICODE_STRING* FullDllName, PVOID *DllHandle);
	HMODULE hNtDll = LoadLibrary(L"ntdll.dll");
	HMODULE hKernel = GetModuleHandle(str.Buffer);
	LdrGetDllHandleByName_t LdrGetDllHandleByName_f = (LdrGetDllHandleByName_t)GetProcAddress(hNtDll, "LdrGetDllHandleByName");
	LONG ntStatus = -1; LPBYTE ptrProc = NULL;
	if (LdrGetDllHandleByName_f)
	{
		DWORD_PTR nShift = ((LPBYTE)GetProcAddress(hKernel,"LoadLibraryW")) - (LPBYTE)hKernel;
		ntStatus = LdrGetDllHandleByName_f(&str, NULL, (PVOID*)&ptrProc);
		ptrProc += 0x12345;
	}
}
#endif

bool IsOutputRedirected()
{
	static int isRedirected = 0;
	if (isRedirected)
	{
		return (isRedirected == 2);
	}

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO sbi = {};
	BOOL bIsConsole = GetConsoleScreenBufferInfo(hOut, &sbi);
	if (bIsConsole)
	{
		isRedirected = 1;
		return false;
	}
	else
	{
		isRedirected = 2;
		return true;
	}
}

void _wprintf(LPCWSTR asBuffer)
{
	if (!asBuffer) return;

	int nAllLen = lstrlenW(asBuffer);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwWritten = 0;

	if (!IsOutputRedirected())
	{
		WriteConsoleW(hOut, asBuffer, nAllLen, &dwWritten, 0);
	}
	else
	{
		UINT  cp = GetConsoleOutputCP();
		int cchMax = WideCharToMultiByte(cp, 0, asBuffer, -1, NULL, 0, NULL, NULL) + 1;
		char* pszOem = (cchMax > 1) ? (char*)malloc(cchMax) : NULL;
		if (pszOem)
		{
			int nWrite = WideCharToMultiByte(cp, 0, asBuffer, -1, pszOem, cchMax, NULL, NULL);
			if (nWrite > 1)
			{
				// Don't write terminating '\0' to redirected output
				WriteFile(hOut, pszOem, nWrite-1, &dwWritten, 0);
			}
			free(pszOem);
		}
	}
}

void PrintVersion()
{
	wchar_t szProgInfo[255], szVer[32];
	MultiByteToWideChar(CP_ACP, 0, CONEMUVERS, -1, szVer, countof(szVer));
	_wsprintf(szProgInfo, SKIPLEN(countof(szProgInfo))
		L"ConEmuC build %s %s. " CECOPYRIGHTSTRING_W L"\n",
		szVer, WIN3264TEST(L"x86",L"x64"));
	_wprintf(szProgInfo);
}

void Help()
{
	PrintVersion();

	// See definition in "ConEmuCD/ConsoleHelp.h"
	_wprintf(pConsoleHelp);
	_wprintf(pNewConsoleHelp);
}

bool ProcessCommandLine(int& iRc, HMODULE& hConEmu)
{
	LPCWSTR pszCmdLine = GetCommandLineW();

	// If there is '-new_console' or '-cur_console' switches...
	if (IsNewConsoleArg(pszCmdLine) || IsNewConsoleArg(pszCmdLine, L"-cur_console"))
		return false;

	HeapInitialize();

	bool bProcessed = false;
	// Loop through switches to find supported
	{
		CEStr lsArg;
		int iCount = 0;
		bool bHelpRequested = false;
		bool bFirst = true;
		while (NextArg(&pszCmdLine, lsArg) == 0)
		{
			if ((lsArg.ms_Val[0] == L'-') && lsArg.ms_Val[1] && !wcspbrk(lsArg.ms_Val+1, L"\\//|.&<>^"))
			{
				// Seems this is to be the "switch" too
				lsArg.ms_Val[0] = L'/';
			}

			bool bWasFirst = bFirst; bFirst = false;

			if ((lsArg.ms_Val[0] != L'/') && bWasFirst)
			{
				LPCWSTR pszName = PointToName(lsArg.ms_Val);
				if (pszName && (lstrcmpi(pszName, WIN3264TEST(L"ConEmuC.exe",L"ConEmuC64.exe")) == 0))
					continue;
			}

			iCount++;

			if ((lsArg.ms_Val[0] != L'/') && (iCount > 1))
			{
				// Some unknown (here) switch, goto full version
				break;
			}

			if (lstrcmpi(lsArg, L"/Download") == 0)
			{
				iRc = DoDownload(pszCmdLine);
				// Return '0' on download success for compatibility
				if (iRc == CERR_DOWNLOAD_SUCCEEDED)
					iRc = 0;
				bProcessed = true;
				break;
			}

			if ((lstrcmpi(lsArg, L"/?") == 0)
				|| (lstrcmpi(lsArg, L"/h") == 0)
				|| (lstrcmpi(lsArg, L"/help") == 0)
				|| (lstrcmpi(lsArg, L"/-help") == 0)
				)
			{
				bHelpRequested = true;
				break;
			};

			// ToDo: /IsConEmu may be processed partially?

			// TODO: Inject remote and standard, DefTerm
		}

		if (bHelpRequested || (iCount == 0))
		{
			if (!hConEmu)
			{
				// Prefere Help from ConEmuCD.dll because ConEmuC.exe may be outdated (due to stability preference)
				hConEmu = LoadLibrary(WIN3264TEST(L"ConEmuCD.dll",L"ConEmuCD64.dll"));

				// Show internal Help variant if only ConEmuCD.dll was failed to load
				if (hConEmu == NULL)
				{
					Help();
					iRc = CERR_HELPREQUESTED;
					bProcessed = true;
				}
			}
		}
	}

	HeapDeinitialize();

	return bProcessed;
}

int main(int argc, char** argv)
{
	int iRc = 0;
	HMODULE hConEmu = NULL;
	wchar_t szErrInfo[200];
	DWORD dwErr;
	typedef int (__stdcall* ConsoleMain2_t)(BOOL abAlternative);
	ConsoleMain2_t lfConsoleMain2;

	#if defined(SHOW_STARTED_MSGBOX)
	if (!IsDebuggerPresent())
	{
		wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) WIN3264TEST(L"ConEmuC",L"ConEmuC64") L" Loaded (PID=%i)", GetCurrentProcessId());
		const wchar_t* pszCmdLine = GetCommandLineW();
		MessageBox(NULL,pszCmdLine,szTitle,0);
	}
	#endif

	// Обязательно, иначе по CtrlC мы свалимся
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine, true);

	#ifdef _DEBUG
	UnitTests();
	#endif

	// Some command we can process internally
	if (ProcessCommandLine(iRc, hConEmu))
	{
		// Done, exiting
		goto wrap;
	}

	// Otherwise - do the full cycle
	if (!hConEmu)
		hConEmu = LoadLibrary(WIN3264TEST(L"ConEmuCD.dll",L"ConEmuCD64.dll"));
	dwErr = GetLastError();

	if (!hConEmu)
	{
		_wsprintf(szErrInfo, SKIPLEN(countof(szErrInfo))
		           L"Can't load library \"%s\", ErrorCode=0x%08X\n",
		           WIN3264TEST(L"ConEmuCD.dll",L"ConEmuCD64.dll"),
		           dwErr);
		_wprintf(szErrInfo);
		_ASSERTE(FALSE && "LoadLibrary failed");
		iRc = CERR_CONEMUHK_NOTFOUND;
		goto wrap;
	}

	// Загрузить функи из ConEmuHk
	lfConsoleMain2 = (ConsoleMain2_t)GetProcAddress(hConEmu, "ConsoleMain2");
	gfHandlerRoutine = (PHANDLER_ROUTINE)GetProcAddress(hConEmu, "HandlerRoutine");

	if (!lfConsoleMain2 || !gfHandlerRoutine)
	{
		dwErr = GetLastError();
		_wsprintf(szErrInfo, SKIPLEN(countof(szErrInfo))
		           L"Procedure \"%s\"  not found in library \"%s\"",
		           lfConsoleMain2 ? L"HandlerRoutine" : L"ConsoleMain2",
		           WIN3264TEST(L"ConEmuCD.dll",L"ConEmuCD64.dll"));
		_wprintf(szErrInfo);
		_ASSERTE(FALSE && "GetProcAddress failed");
		FreeLibrary(hConEmu);
		iRc = CERR_CONSOLEMAIN_NOTFOUND;
		goto wrap;
	}

	// Main dll entry point for Server & ComSpec
	iRc = lfConsoleMain2(0/*WorkMode*/);
	// Exiting
	gfHandlerRoutine = NULL;
	//FreeLibrary(hConEmu); -- Shutdown Server/Comspec уже выполнен
wrap:
	ExitProcess(iRc);
	return iRc;
}
