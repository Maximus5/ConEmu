
/*
Copyright (c) 2013 Maximus5
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

#include <shlobj.h>
#ifndef __GNUC__
#include <dbghelp.h>
#else
#include "../common/DbgHlpGcc.h"
#endif

#ifndef _T
#define __T(x)      L ## x
#define _T(x)       __T(x)
#endif

// Возвращает текст с информацией о пути к сохраненному дампу
DWORD CreateDumpForReport(LPEXCEPTION_POINTERS ExceptionInfo, wchar_t (&szFullInfo)[1024])
{
	DWORD dwErr = 0;
	const wchar_t *pszError = NULL;
	INT_PTR nLen;
	MINIDUMP_TYPE dumpType = MiniDumpWithFullMemory;
	//bool bDumpSucceeded = false;
	HANDLE hDmpFile = NULL;
	HMODULE hDbghelp = NULL;
	wchar_t dmpfile[MAX_PATH+64] = L"";
	typedef BOOL (WINAPI* MiniDumpWriteDump_t)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
	        PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	        PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
	MiniDumpWriteDump_t MiniDumpWriteDump_f = NULL;

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	memset(szFullInfo, 0, sizeof(szFullInfo));

	dwErr = SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, dmpfile);
	if (FAILED(dwErr))
	{
		pszError = L"CreateDumpForReport called, get desktop folder failed";
		goto wrap;
	}
	if (*dmpfile && dmpfile[lstrlen(dmpfile)-1] != L'\\')
		wcscat_c(dmpfile, L"\\");
	wcscat_c(dmpfile, L"ConEmuTrap");
	CreateDirectory(dmpfile, NULL);
	nLen = lstrlen(dmpfile);
	_wsprintf(dmpfile+nLen, SKIPLEN(countof(dmpfile)-nLen) L"\\Trap-%02u%02u%02u%s-%u.dmp", MVV_1, MVV_2, MVV_3,_T(MVV_4a), GetCurrentProcessId());
	
	hDmpFile = CreateFileW(dmpfile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL/*|FILE_FLAG_WRITE_THROUGH*/, NULL);
	if (hDmpFile == INVALID_HANDLE_VALUE)
	{
		pszError = L"CreateDumpForReport called, create dump file failed";
		goto wrap;
	}


	hDbghelp = LoadLibraryW(L"Dbghelp.dll");
	if (hDbghelp == NULL)
	{
		pszError = L"CreateDumpForReport called, LoadLibrary(Dbghelp.dll) failed";
		goto wrap;
	}

	MiniDumpWriteDump_f = (MiniDumpWriteDump_t)GetProcAddress(hDbghelp, "MiniDumpWriteDump");
	if (!MiniDumpWriteDump_f)
	{
		pszError = L"CreateDumpForReport called, MiniDumpWriteDump not found";
		goto wrap;
	}
	else
	{
		MINIDUMP_EXCEPTION_INFORMATION mei = {GetCurrentThreadId()};
		// ExceptionInfo may be null
		mei.ExceptionPointers = ExceptionInfo;
		mei.ClientPointers = FALSE;
		BOOL lbDumpRc = MiniDumpWriteDump_f(
		                    GetCurrentProcess(), GetCurrentProcessId(),
		                    hDmpFile,
		                    dumpType,
		                    &mei,
		                    NULL, NULL);

		if (!lbDumpRc)
		{
			pszError = L"CreateDumpForReport called, MiniDumpWriteDump failed";
			goto wrap;
		}
	}

#if 0
	wchar_t szCmdLine[MAX_PATH*2] = L"\"";
	wchar_t *pszSlash;
	INT_PTR nLen;
	PROCESS_INFORMATION pi = {};
	STARTUPINFO si = {sizeof(si)};

	if (!GetModuleFileName(NULL, szCmdLine+1, MAX_PATH))
	{
		pszError = L"CreateDumpForReport called, GetModuleFileName failed";
		goto wrap;
	}
	if ((pszSlash = wcsrchr(szCmdLine, L'\\')) == NULL)
	{
		pszError = L"CreateDumpForReport called, wcsrchr failed";
		goto wrap;
	}
	_wcscpy_c(pszSlash+1, 30, WIN3264TEST(L"ConEmu\\ConEmuC.exe",L"ConEmu\\ConEmuC64.exe"));
	if (!FileExists(szCmdLine+1))
	{
		_wcscpy_c(pszSlash+1, 30, WIN3264TEST(L"ConEmuC.exe",L"ConEmuC64.exe"));
		if (!FileExists(szCmdLine+1))
		{
			if (gpConEmu)
			{
				_wsprintf(szCmdLine, SKIPLEN(countof(szCmdLine)) L"\"%s\\%s", gpConEmu->ms_ConEmuBaseDir, WIN3264TEST(L"ConEmuC.exe",L"ConEmuC64.exe"));
			}
			if (!FileExists(szCmdLine+1))
			{
				pszError = L"CreateDumpForReport called, " WIN3264TEST(L"ConEmuC.exe",L"ConEmuC64.exe") L" not found";
				goto wrap;
			}
		}
	}

	nLen = _tcslen(szCmdLine);
	_wsprintf(szCmdLine+nLen, SKIPLEN(countof(szCmdLine)-nLen) L"\" /DEBUGPID=%u /FULL", GetCurrentProcessId());

	if (!CreateProcess(NULL, szCmdLine, NULL, NULL, TRUE, HIGH_PRIORITY_CLASS, NULL, NULL, &si, &pi))
	{
		pszError = L"CreateDumpForReport called, CreateProcess failed";
		goto wrap;
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
#endif

wrap:
	if (pszError)
	{
		lstrcpyn(szFullInfo, pszError, 255);
		if (dmpfile[0])
		{
			wcscat_c(szFullInfo, L"\r\n");
			wcscat_c(szFullInfo, dmpfile);
		}
		if (!dwErr)
			dwErr = GetLastError();
	}
	else
	{
		wchar_t what[64];
		if (ExceptionInfo && ExceptionInfo->ExceptionRecord)
			_wsprintf(what, SKIPLEN(countof(what)) L"Exception 0x%08X", ExceptionInfo->ExceptionRecord->ExceptionCode);
		else
			wcscpy_c(what, L"Assertion");

		_wsprintf(szFullInfo, SKIPLEN(countof(szFullInfo)) L"%s was occurred (ConEmu%s %02u%02u%02u%s)\r\n\r\n"
			L"Memory dump was saved to\r\n%s\r\n\r\n"
			L"Please Zip it and send to developer\r\n"
			L"%s" /*gsReportCrash -> http://code.google.com/p/conemu-maximus5/... */,
			what, WIN3264TEST(L"",L"64"), (MVV_1%100),MVV_2,MVV_3,_T(MVV_4a), dmpfile, CEREPORTCRASH);
	}
	if (hDmpFile != INVALID_HANDLE_VALUE && hDmpFile != NULL)
	{
		CloseHandle(hDmpFile);
	}
	if (hDbghelp)
	{
		FreeLibrary(hDbghelp);
	}

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return dwErr;
}
