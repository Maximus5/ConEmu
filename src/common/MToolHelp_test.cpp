
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "defines.h"
#include "../UnitTests/gtest.h"
#include "MToolHelp.h"
#include <set>

TEST(MToolHelp, ProcessByName)
{
	wchar_t module[MAX_PATH] = L"";
	GetModuleFileNameW(nullptr, module, MAX_PATH);
	EXPECT_NE(module[0], L'\0');

	MToolHelpProcess th;
	PROCESSENTRY32W pi{};
	EXPECT_TRUE(th.Find(module, pi));
	EXPECT_EQ(pi.th32ProcessID, GetCurrentProcessId());
	const int iCmp = lstrcmpi(PointToName(module), PointToName(pi.szExeFile));
	EXPECT_EQ(iCmp, 0);
}

TEST(MToolHelp, ProcessByPid)
{
	wchar_t module[MAX_PATH] = L"";
	GetModuleFileNameW(nullptr, module, MAX_PATH);
	EXPECT_NE(module[0], L'\0');

	MToolHelpProcess th;
	PROCESSENTRY32W pi{};
	EXPECT_TRUE(th.Find(GetCurrentProcessId(), pi));
	EXPECT_EQ(pi.th32ProcessID, GetCurrentProcessId());
	const int iCmp = lstrcmpi(PointToName(module), PointToName(pi.szExeFile));
	EXPECT_EQ(iCmp, 0);
}

TEST(MToolHelp, ProcessForks)
{
	wchar_t module[MAX_PATH] = L"";
	GetModuleFileNameW(nullptr, module, MAX_PATH);
	EXPECT_NE(module[0], L'\0');

	struct Spawned {  // NOLINT(cppcoreguidelines-special-member-functions)
		std::set<DWORD> pids;
		std::set<HANDLE> handles;

		~Spawned()
		{
			for (const auto& process : handles)
			{
				if (process)
				{
					TerminateProcess(process, 100);
					CloseHandle(process);
				}
			}
		}
	} spawned;

	const size_t forkCount = 4;
	for (size_t i = 0; i < forkCount; ++i)
	{
		PROCESS_INFORMATION pi{};
		STARTUPINFO si{};
		si.cb = sizeof(si);
		if (CreateProcess(nullptr, module, nullptr, nullptr, false, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi))
		{
			if (pi.hProcess)
			{
				spawned.pids.insert(pi.dwProcessId);
				spawned.handles.insert(pi.hProcess);
			}
			if (pi.hThread)
			{
				CloseHandle(pi.hThread);
			}
		}
	}
	EXPECT_EQ(spawned.pids.size(), forkCount);

	MToolHelpProcess th;
	const auto forked = th.FindForks(GetCurrentProcessId());
	std::set<DWORD> forkedPids;
	for (const auto& info : forked)
	{
		forkedPids.insert(info.th32ProcessID);
	}
	EXPECT_EQ(forkedPids.size(), forkCount);
	EXPECT_EQ(forkedPids, spawned.pids);
}
