
/*
Copyright (c) 2013-2017 Maximus5
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

#pragma warning(disable: 4091)
#include <shlobj.h>
#pragma warning(default: 4091)

#if !defined(__GNUC__) || defined(__MINGW32__)
#pragma warning(disable: 4091)
#include <dbghelp.h>
#pragma warning(default: 4091)
#else
#include "../common/DbgHlpGcc.h"
#endif

#ifndef _T
#define __T(x)      L ## x
#define _T(x)       __T(x)
#endif

// Возвращает текст с информацией о пути к сохраненному дампу
DWORD CreateDumpForReport(LPEXCEPTION_POINTERS ExceptionInfo, wchar_t (&szFullInfo)[1024], wchar_t (&dmpfile)[MAX_PATH+64], LPWSTR pszComment = NULL)
{
	DWORD dwErr = 0;
	const wchar_t *pszError = NULL;
	INT_PTR nLen;
	MINIDUMP_TYPE dumpType = MiniDumpWithFullMemory;
	MINIDUMP_USER_STREAM_INFORMATION cmt;
	MINIDUMP_USER_STREAM cmt1;
	//bool bDumpSucceeded = false;
	HANDLE hDmpFile = NULL;
	HMODULE hDbghelp = NULL;
	ZeroStruct(dmpfile);
	typedef BOOL (WINAPI* MiniDumpWriteDump_t)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
	        PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	        PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
	MiniDumpWriteDump_t MiniDumpWriteDump_f = NULL;
	DWORD nSharingOption;
	wchar_t szVer4[8] = L"";

	// Sequential dumps must not overwrite each other
	static LONG nDumpIndex = 0;
	wchar_t szDumpIndex[16] = L"";
	if (nDumpIndex)
		_wsprintf(szDumpIndex, SKIPCOUNT(szDumpIndex) L"-%i", nDumpIndex);
	InterlockedIncrement(&nDumpIndex);

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	if (pszComment)
	{
		cmt1.Type = 11/*CommentStreamW*/;
		cmt1.BufferSize = (lstrlen(pszComment)+1)*sizeof(*pszComment);
		cmt1.Buffer = pszComment;
		cmt.UserStreamCount = 1;
		cmt.UserStreamArray = &cmt1;
	}

	memset(szFullInfo, 0, sizeof(szFullInfo));

	dwErr = SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0/*SHGFP_TYPE_CURRENT*/, dmpfile);
	if (FAILED(dwErr))
	{
		memset(dmpfile, 0, sizeof(dmpfile));
		if (!GetTempPath(MAX_PATH, dmpfile) || !*dmpfile)
		{
			pszError = L"CreateDumpForReport called, get desktop or temp folder failed";
			goto wrap;
		}
	}

	wcscat_c(dmpfile, (*dmpfile && dmpfile[lstrlen(dmpfile)-1] != L'\\') ? L"\\ConEmuTrap" : L"ConEmuTrap");
	CreateDirectory(dmpfile, NULL);

	nLen = lstrlen(dmpfile);
	lstrcpyn(szVer4, _T(MVV_4a), countof(szVer4));
	_wsprintf(dmpfile+nLen, SKIPLEN(countof(dmpfile)-nLen) L"\\Trap-%02u%02u%02u%s-%u%s.dmp", MVV_1, MVV_2, MVV_3, szVer4, GetCurrentProcessId(), szDumpIndex);
	
	nSharingOption = /*FILE_SHARE_READ;
	if (IsWindowsXP)
		nSharingOption =*/ FILE_SHARE_READ|FILE_SHARE_WRITE;
	hDmpFile = CreateFileW(dmpfile, GENERIC_READ|GENERIC_WRITE, nSharingOption, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL/*|FILE_FLAG_WRITE_THROUGH*/, NULL);
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
		                    pszComment ? &cmt : NULL,
		                    NULL);

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
		wchar_t szExeName[MAX_PATH+1] = L"";
		GetModuleFileName(NULL, szExeName, countof(szExeName)-1);
		LPCWSTR pszExeName = PointToName(szExeName);

		wchar_t what[100];
		if (ExceptionInfo && ExceptionInfo->ExceptionRecord)
		{
			_wsprintf(what, SKIPLEN(countof(what)) L"Exception 0x%08X", ExceptionInfo->ExceptionRecord->ExceptionCode);
			if (EXCEPTION_ACCESS_VIOLATION == ExceptionInfo->ExceptionRecord->ExceptionCode
				&& ExceptionInfo->ExceptionRecord->NumberParameters >= 2)
			{
				ULONG_PTR iType = ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
				ULONG_PTR iAddr = ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
				INT_PTR iLen = lstrlen(what);
				_wsprintf(what+iLen, SKIPLEN(countof(what)-iLen)
					WIN3264TEST(L" (%sx%08X)",L" (%s x%08X%08X)"),
					(iType==0) ? L"Read " : (iType==1) ? L"Write " : (iType==8) ? L"DEP " : L"",
					WIN3264WSPRINT(iAddr));
			}
		}
		else
		{
			wcscpy_c(what, L"Assertion");
		}

		_wsprintf(szFullInfo, SKIPLEN(countof(szFullInfo)) L"%s was occurred (%s, PID=%u)\r\nConEmu build %02u%02u%02u%s %s",
			what, pszExeName, GetCurrentProcessId(),
			(MVV_1%100),MVV_2,MVV_3,_T(MVV_4a), WIN3264TEST(L"",L"64"));
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
