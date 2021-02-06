
/*
Copyright (c) 2009-present Maximus5
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

#include "../common/Common.h"
#include "../common/CmdLine.h"
#include "../common/MModule.h"

#include "../ConEmu/version.h"
#include "../ConEmuCD/ExitCodes.h"
#include "../ConEmuCD/ExportedFunctions.h"
#include "../ConEmuCD/ConsoleHelp.h"

#include "ConEmuC.h"
#include "Downloader.h"
#include "../common/MHandle.h"

#ifdef _DEBUG
#include <tuple>
#endif

PHANDLER_ROUTINE gfHandlerRoutine = nullptr;

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	if (gfHandlerRoutine)
	{
		// Call exported function in ConEmuCD.dll
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
	str.Buffer = reinterpret_cast<PWSTR>(((reinterpret_cast<DWORD_PTR>(buf) + 7) >> 3) << 3);
	lstrcpy(str.Buffer, L"kernel32.dll");
	str.Length = static_cast<USHORT>(wcslen(str.Buffer) * 2U);
	str.MaximumLength = str.Length + 2;

	typedef LONG (__stdcall* LdrGetDllHandleByName_t)(UNICODE_STRING* BaseDllName, UNICODE_STRING* FullDllName, PVOID *DllHandle);
	const MModule hNtDll(L"ntdll.dll");
	const MModule hKernel(GetModuleHandle(str.Buffer));
	LdrGetDllHandleByName_t fnLdrGetDllHandleByName = nullptr;
	hNtDll.GetProcAddress("LdrGetDllHandleByName", fnLdrGetDllHandleByName);
	LONG ntStatus = -1; LPBYTE ptrProc = nullptr; LONG_PTR nShift = -1;
	if (fnLdrGetDllHandleByName)
	{
		FARPROC fnLoadLibraryW = nullptr;
		if (!hKernel.GetProcAddress("LoadLibraryW", fnLoadLibraryW))
			goto err;
		nShift = reinterpret_cast<LPBYTE>(fnLoadLibraryW) - reinterpret_cast<LPBYTE>(static_cast<HMODULE>(hKernel));
		if (!fnLdrGetDllHandleByName(&str, nullptr, reinterpret_cast<PVOID*>(&ptrProc)))
			goto err;
		ntStatus = fnLdrGetDllHandleByName(&str, nullptr, reinterpret_cast<PVOID*>(&ptrProc));
	}
err:
	std::ignore = ntStatus;
	std::ignore = ptrProc;
	std::ignore = nShift;
}
#endif

bool IsOutputRedirected()
{
	static int isRedirected = 0;
	if (!isRedirected)
	{
		const MHandle hOut{ GetStdHandle(STD_OUTPUT_HANDLE) };

		CONSOLE_SCREEN_BUFFER_INFO sbi = {};
		const BOOL bIsConsole = GetConsoleScreenBufferInfo(hOut, &sbi);
		if (bIsConsole)
		{
			isRedirected = 1;
		}
		else
		{
			isRedirected = 2;
		}
	}

	return (isRedirected == 2);
}

void PrintBuffer(LPCWSTR asBuffer)
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
		int cchMax = WideCharToMultiByte(cp, 0, asBuffer, -1, nullptr, 0, nullptr, nullptr) + 1;
		char* pszOem = (cchMax > 1) ? (char*)malloc(cchMax) : nullptr;
		if (pszOem)
		{
			int nWrite = WideCharToMultiByte(cp, 0, asBuffer, -1, pszOem, cchMax, nullptr, nullptr);
			if (nWrite > 1)
			{
				// Don't write terminating '\0' to redirected output
				WriteFile(hOut, pszOem, nWrite-1, &dwWritten, 0);
			}
			free(pszOem);
		}
	}
}

void PrintBuffer(LPCSTR asBuffer)
{
	if (!asBuffer) return;

	int nAllLen = lstrlenA(asBuffer);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwWritten = 0;

	if (!IsOutputRedirected())
	{
		WriteConsoleA(hOut, asBuffer, nAllLen, &dwWritten, 0);
	}
	else
	{
		WriteFile(hOut, asBuffer, nAllLen, &dwWritten, 0);
	}
}

void PrintVersion()
{
	wchar_t szProgInfo[255], szVer[32];
	MultiByteToWideChar(CP_ACP, 0, CONEMUVERS, -1, szVer, countof(szVer));
	swprintf_c(szProgInfo,
		L"ConEmuC build %s %s. " CECOPYRIGHTSTRING_W L"\n",
		szVer, WIN3264TEST(L"x86",L"x64"));
	PrintBuffer(szProgInfo);
}

void Help()
{
	PrintVersion();

	// See definition in "ConEmuCD/ConsoleHelp.h"
	PrintBuffer(pConsoleHelp);
	PrintBuffer(pNewConsoleHelp);
}

static int gn_argc = 0;
static char** gp_argv = nullptr;

// The function exists in both "ConEmuC/ConEmuC.cpp" and "ConEmuCD/Actions.cpp"
// Version in "ConEmuC/ConEmuC.cpp" shows arguments from main(int argc, char** argv)
// Version in "ConEmuCD/Actions.cpp" perhaps would not be ever called
int DoParseArgs(LPCWSTR asCmdLine)
{
	char szLine[80];
	char szCLVer[32];
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	GetConsoleScreenBufferInfo(hOut, &csbi);

	struct Highlighter
	{
		HANDLE hOut;
		WORD defAttr;
		Highlighter(HANDLE ahOut, WORD adefAttr, WORD fore)
			: hOut(ahOut), defAttr(adefAttr)
		{
			SetConsoleTextAttribute(hOut, fore|(defAttr & 0xF0));
		}
		~Highlighter()
		{
			SetConsoleTextAttribute(hOut, defAttr);
		}
	};
	#undef HL
	#define HL(fore) Highlighter hl(hOut, csbi.wAttributes, fore)

	#if defined(__GNUC__)
	strcpy(szCLVer, "GNUC");
	#elif defined(_MSC_VER)
	sprintf_c(szCLVer, "VC %u.%u", (int)(_MSC_VER / 100), (int)(_MSC_VER % 100));
	#else
	strcpy(szCLVer, "<Unknown CL>");
	#endif

	{ HL(10); PrintBuffer("GetCommandLine():\n"); }
	{ HL(2);  PrintBuffer("  *: "); }
	{ HL(8); PrintBuffer("`"); }
	{ HL(15); PrintBuffer(GetCommandLineW()); }
	{ HL(8); PrintBuffer("`"); }
	PrintBuffer("\n");

	sprintf_c(szLine, "main arguments (count %i) {%s}\n", gn_argc, szCLVer);
	{ HL(10); PrintBuffer(szLine); }
	for (int j = 0; j < gn_argc; j++)
	{
		if (j >= 999)
		{
			HL(12);
			PrintBuffer("*** TOO MANY ARGUMENTS ***\n");
			break;
		}
		sprintf_c(szLine, "  %u: ", j + 1);
		{ HL(2); PrintBuffer(szLine); }
		if (!gp_argv)
		{
			HL(12);
			PrintBuffer("*NULL");
		}
		else if (!gp_argv[j])
		{
			HL(12);
			PrintBuffer("<NULL>");
		}
		else
		{
			{ HL(8); PrintBuffer("`"); }
			{ HL(15); PrintBuffer(gp_argv[j]); }
			{ HL(8); PrintBuffer("`"); }
		}
		PrintBuffer("\n");
	}

	{ HL(10); PrintBuffer("Parsing command"); }
	{ HL(8); PrintBuffer("\n  `"); }
	{ HL(15); PrintBuffer(asCmdLine); }
	{ HL(8); PrintBuffer("`\n"); }

	int iShellCount = 0;
	LPWSTR* ppszShl = CommandLineToArgvW(asCmdLine, &iShellCount);

	int i = 0;
	CmdArg szArg;
	{ HL(10); PrintBuffer("ConEmu `NextArg` splitter\n"); }
	while ((asCmdLine = NextArg(asCmdLine, szArg)))
	{
		if (szArg.m_bQuoted)
			DemangleArg(szArg, true);
		sprintf_c(szLine, "  %u: ", ++i);
		{ HL(2); PrintBuffer(szLine); }
		{ HL(8); PrintBuffer("`"); }
		{ HL(15); PrintBuffer(szArg); }
		{ HL(8); PrintBuffer("`\n"); }
	}
	sprintf_c(szLine, "  Total arguments parsed: %u\n", i);
	{ HL(8); PrintBuffer(szLine); }

	{ HL(10); PrintBuffer("Standard shell splitter\n"); }
	for (int j = 0; j < iShellCount; j++)
	{
		sprintf_c(szLine, "  %u: ", j + 1);
		{ HL(2); PrintBuffer(szLine); }
		{ HL(8); PrintBuffer("`"); }
		{ HL(15); PrintBuffer(ppszShl[j]); }
		{ HL(8); PrintBuffer("`\n"); }
	}
	sprintf_c(szLine, "  Total arguments parsed: %u\n", iShellCount);
	{ HL(8); PrintBuffer(szLine); }
	LocalFree(ppszShl);

	return i;
}

bool ProcessCommandLine(int& iRc, MModule& hConEmu)
{
	LPCWSTR pszCmdLine = GetCommandLineW();

	// If there is '-new_console' or '-cur_console' switches...
	if (IsNewConsoleArg(pszCmdLine) || IsNewConsoleArg(pszCmdLine, L"-cur_console"))
		return false;

	HeapInitialize();

	bool bProcessed = false;
	// Loop through switches to find supported
	{
		CmdArg lsArg;
		int iCount = 0;
		bool bHelpRequested = false;
		bool bFirst = true;
		while ((pszCmdLine = NextArg(pszCmdLine, lsArg)))
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
				if (pszName && (lstrcmpi(pszName, ConEmuC_EXE_3264) == 0))
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

			if ((lstrcmpi(lsArg, L"/Args") == 0) ||  (lstrcmpi(lsArg, L"/ParseArgs") == 0))
			{
				iRc = DoParseArgs(pszCmdLine);
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
				// Prefer Help from ConEmuCD.dll because ConEmuC.exe may be outdated (due to stability preference)
				hConEmu.Load(ConEmuCD_DLL_3264);

				// Show internal Help variant if only ConEmuCD.dll was failed to load
				if (!hConEmu)
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
	gn_argc = argc; gp_argv = argv;

	int iRc = 0;
	MModule hConEmu{};
	wchar_t szErrInfo[200];
	DWORD dwErr;
	ConsoleMain2_t lfConsoleMain2 = nullptr;

	#ifdef _DEBUG
	const MModule hConEmuHk{ GetModuleHandle(ConEmuHk_DLL_3264) };
	_ASSERTE(!hConEmuHk.IsValid() && "Hooks must not be loaded into ConEmuC[64].exe!");
	#endif

	#if defined(SHOW_STARTED_MSGBOX)
	if (!IsDebuggerPresent())
	{
		wchar_t szTitle[100]; swprintf_c(szTitle, WIN3264TEST(L"ConEmuC",L"ConEmuC64") L" Loaded (PID=%i)", GetCurrentProcessId());
		const wchar_t* pszCmdLine = GetCommandLineW();
		MessageBox(nullptr,pszCmdLine,szTitle,0);
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
		hConEmu.Load(ConEmuCD_DLL_3264);
	dwErr = GetLastError();

	if (!hConEmu)
	{
		swprintf_c(szErrInfo,
		           L"Can't load library \"%s\", ErrorCode=0x%08X\n",
		           ConEmuCD_DLL_3264,
		           dwErr);
		PrintBuffer(szErrInfo);
		_ASSERTE(FALSE && "LoadLibrary failed");
		iRc = CERR_CONEMUHK_NOTFOUND;
		goto wrap;
	}

	// Load exports from ConEmuHk
	hConEmu.GetProcAddress(FN_CONEMUCD_CONSOLE_MAIN_2_NAME, lfConsoleMain2);
	hConEmu.GetProcAddress(FN_CONEMUCD_HANDLER_ROUTINE_NAME, gfHandlerRoutine);


	if (!lfConsoleMain2 || !gfHandlerRoutine)
	{
		dwErr = GetLastError();
		swprintf_c(szErrInfo,
		           L"Procedure \"%s\"  not found in library \"%s\"",
		           lfConsoleMain2 ? _CRT_WIDE(FN_CONEMUCD_HANDLER_ROUTINE_NAME) : _CRT_WIDE(FN_CONEMUCD_CONSOLE_MAIN_2_NAME),
		           ConEmuCD_DLL_3264);
		PrintBuffer(szErrInfo);
		_ASSERTE(FALSE && "GetProcAddress failed");
		hConEmu.Free();
		iRc = CERR_CONSOLEMAIN_NOTFOUND;
		goto wrap;
	}

	// Main dll entry point for Server & ComSpec
	iRc = lfConsoleMain2(ConsoleMainMode::Normal);
	// Exiting
	gfHandlerRoutine = nullptr;

wrap:
	//-- bottle neck: relatively long deinitialization
	ExitProcess(iRc);
	return iRc;
}
