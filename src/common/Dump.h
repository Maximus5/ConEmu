
/*
Copyright (c) 2013-present Maximus5
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

#include "../ConEmu/version.h"
#include "../common/shlobj.h"
#include "../common/MModule.h"


#if !defined(__GNUC__) || defined(__MINGW32__)
#pragma warning(push)
#pragma warning(disable: 4091)
#include <dbghelp.h>
#pragma warning(pop)
#else
#include "../common/DbgHlpGcc.h"
#endif

#ifndef _T
#define __T(x)      L ## x
#define _T(x)       __T(x)
#endif

#define EXCEPTION_CONEMU_MEMORY_DUMP 0xCE004345

struct ConEmuDumpInfo
{
	DWORD result = 0;
	wchar_t fullInfo[1024] = L"";
	wchar_t dumpFile[MAX_PATH + 64] = L"";
	LPWSTR comment = nullptr;
};

// Возвращает текст с информацией о пути к сохраненному дампу
inline DWORD CreateDumpForReport(LPEXCEPTION_POINTERS exceptionInfo, ConEmuDumpInfo& dumpInfo)
{
	// ReSharper disable CppJoinDeclarationAndAssignment
	DWORD dwErr = 0;
	const wchar_t *pszError = nullptr;
	INT_PTR nLen;
	const MINIDUMP_TYPE dumpType = MiniDumpWithFullMemory;
	MINIDUMP_USER_STREAM_INFORMATION cmt;
	MINIDUMP_USER_STREAM cmt1;
	//bool bDumpSucceeded = false;
	HANDLE hDmpFile = nullptr;
	MModule dbgHelp;
	ZeroStruct(dumpInfo.dumpFile);
	typedef BOOL (WINAPI* MiniDumpWriteDump_t)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
	        PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	        PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
	MiniDumpWriteDump_t MiniDumpWriteDump_f = nullptr;
	DWORD nSharingOption;
	wchar_t szVer4[8] = L"";

	// Sequential dumps must not overwrite each other
	static std::atomic_int nDumpIndex{ 0 };
	wchar_t szDumpIndex[16] = L"";
	const int currentIndex = nDumpIndex.fetch_add(1);
	if (currentIndex > 0)
		swprintf_c(szDumpIndex, L"-%i", currentIndex);

	SetCursor(LoadCursor(nullptr, IDC_WAIT));

	if (dumpInfo.comment)
	{
		cmt1.Type = 11/*CommentStreamW*/;
		cmt1.BufferSize = (lstrlen(dumpInfo.comment) + 1) * sizeof(*dumpInfo.comment);
		cmt1.Buffer = dumpInfo.comment;
		cmt.UserStreamCount = 1;
		cmt.UserStreamArray = &cmt1;
	}

	memset(dumpInfo.fullInfo, 0, sizeof(dumpInfo.fullInfo));

	dwErr = SHGetFolderPath(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0/*SHGFP_TYPE_CURRENT*/, dumpInfo.dumpFile);
	if (FAILED(dwErr))
	{
		memset(dumpInfo.dumpFile, 0, sizeof(dumpInfo.dumpFile));
		if (!GetTempPath(MAX_PATH, dumpInfo.dumpFile) || !*dumpInfo.dumpFile)
		{
			pszError = L"CreateDumpForReport called, get desktop or temp folder failed";
			goto wrap;
		}
	}

	wcscat_c(dumpInfo.dumpFile, (*dumpInfo.dumpFile && dumpInfo.dumpFile[lstrlen(dumpInfo.dumpFile) - 1] != L'\\')
		? L"\\ConEmuTrap" : L"ConEmuTrap");
	CreateDirectory(dumpInfo.dumpFile, nullptr);

	nLen = lstrlen(dumpInfo.dumpFile);
	lstrcpyn(szVer4, _T(MVV_4a), countof(szVer4));
	swprintf_c(dumpInfo.dumpFile+nLen, countof(dumpInfo.dumpFile)-nLen/*#SECURELEN*/,
		L"\\Trap-%02u%02u%02u%s-%u%s.dmp", MVV_1, MVV_2, MVV_3, szVer4, GetCurrentProcessId(), szDumpIndex);
	
	nSharingOption = /*FILE_SHARE_READ;
	if (IsWindowsXP)
		nSharingOption =*/ FILE_SHARE_READ|FILE_SHARE_WRITE;
	hDmpFile = CreateFileW(dumpInfo.dumpFile, GENERIC_READ|GENERIC_WRITE, nSharingOption,
		nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL/*|FILE_FLAG_WRITE_THROUGH*/, nullptr);
	if (hDmpFile == INVALID_HANDLE_VALUE)
	{
		pszError = L"CreateDumpForReport called, create dump file failed";
		goto wrap;
	}


	if (!dbgHelp.Load(L"dbghelp.dll"))
	{
		pszError = L"CreateDumpForReport called, LoadLibrary(Dbghelp.dll) failed";
		goto wrap;
	}

	if (!dbgHelp.GetProcAddress("MiniDumpWriteDump", MiniDumpWriteDump_f))
	{
		pszError = L"CreateDumpForReport called, MiniDumpWriteDump not found";
		goto wrap;
	}
	else
	{
		MINIDUMP_EXCEPTION_INFORMATION mei = {};
		mei.ThreadId = GetCurrentThreadId();
		// ExceptionInfo may be null
		mei.ExceptionPointers = exceptionInfo;
		mei.ClientPointers = FALSE;
		const BOOL lbDumpRc = MiniDumpWriteDump_f(
		                    GetCurrentProcess(), GetCurrentProcessId(),
		                    hDmpFile,
		                    dumpType,
		                    &mei,
		                    dumpInfo.comment ? &cmt : nullptr,
		                    nullptr);

		if (!lbDumpRc)
		{
			pszError = L"CreateDumpForReport called, MiniDumpWriteDump failed";
			goto wrap;
		}
	}

wrap:
	if (pszError)
	{
		lstrcpyn(dumpInfo.fullInfo, pszError, 255);
		if (dumpInfo.dumpFile[0])
		{
			wcscat_c(dumpInfo.fullInfo, L"\r\n");
			wcscat_c(dumpInfo.fullInfo, dumpInfo.dumpFile);
		}
		if (!dwErr)
			dwErr = GetLastError();
	}
	else
	{
		wchar_t szExeName[MAX_PATH+1] = L"";
		GetModuleFileName(nullptr, szExeName, countof(szExeName)-1);
		// ReSharper disable once CppLocalVariableMayBeConst
		LPCWSTR pszExeName = PointToName(szExeName);

		wchar_t what[100] = L"";
		if (exceptionInfo && exceptionInfo->ExceptionRecord
			&& exceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_CONEMU_MEMORY_DUMP)
		{
			swprintf_c(what, L"Exception 0x%08X", exceptionInfo->ExceptionRecord->ExceptionCode);
			if (EXCEPTION_ACCESS_VIOLATION == exceptionInfo->ExceptionRecord->ExceptionCode
				&& exceptionInfo->ExceptionRecord->NumberParameters >= 2)
			{
				const ULONG_PTR iType = exceptionInfo->ExceptionRecord->ExceptionInformation[0];
				const ULONG_PTR iAddr = exceptionInfo->ExceptionRecord->ExceptionInformation[1];
				const INT_PTR iLen = lstrlen(what);
				_wsprintf(what + iLen, SKIPLEN(countof(what) - iLen)
					WIN3264TEST(L" (%sx%08X)", L" (%s x%08X%08X)"),
					(iType == 0) ? L"Read " : (iType == 1) ? L"Write " : (iType == 8) ? L"DEP " : L"",
					WIN3264WSPRINT(iAddr));
			}
		}
		else
		{
			wcscpy_c(what, L"Assertion");
		}

		swprintf_c(dumpInfo.fullInfo, L"%s was occurred (%s, PID=%u)\r\nConEmu build %02u%02u%02u%s %s",
			what, pszExeName, GetCurrentProcessId(),
			(MVV_1 % 100), MVV_2, MVV_3, _T(MVV_4a), WIN3264TEST(L"", L"64"));
	}
	if (hDmpFile != INVALID_HANDLE_VALUE && hDmpFile != nullptr)
	{
		CloseHandle(hDmpFile);
	}

	SetCursor(LoadCursor(nullptr, IDC_ARROW));
	dumpInfo.result = dwErr;
	return dwErr;
	// ReSharper restore CppJoinDeclarationAndAssignment
}
