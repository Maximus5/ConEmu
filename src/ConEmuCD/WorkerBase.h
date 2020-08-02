﻿
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

#pragma once

#include <memory>

class DebuggerInfo;
enum class DumpProcessType;

class WorkerBase
{
public:
	virtual ~WorkerBase() = default;

	WorkerBase() = default;

	WorkerBase(const WorkerBase&) = delete;
	WorkerBase(WorkerBase&&) = delete;
	WorkerBase& operator=(const WorkerBase&) = delete;
	WorkerBase& operator=(WorkerBase&&) = delete;

	virtual int Init() = 0;
	virtual void Done(int exitCode, bool reportShutdown = false);

	virtual bool IsCmdK() const;
	virtual void SetCmdK(bool useCmdK);

	void SetRootProcessId(DWORD pid);
	void SetRootProcessHandle(HANDLE processHandle);
	void SetRootThreadId(DWORD tid);
	void SetRootThreadHandle(HANDLE threadHandle);
	void SetRootStartTime(DWORD ticks);
	DWORD RootProcessId() const;
	DWORD RootThreadId() const;
	HANDLE RootProcessHandle() const;

	bool IsDebuggerActive() const;
	bool IsDebugProcess() const;
	bool IsDebugProcessTree() const;
	int SetDebuggingPid(const wchar_t* pidList);
	int SetDebuggingExe(const wchar_t* commandLine, bool debugTree);
	void SetDebugDumpType(DumpProcessType dumpType);
	void SetDebugAutoDump(const wchar_t* interval);
	DebuggerInfo& DbgInfo();

protected:
	HANDLE hRootProcess = nullptr, hRootThread = nullptr;
	DWORD dwRootProcess = 0, dwRootThread = 0, dwRootStartTime = 0;
	DWORD dwParentFarPID = 0;

	std::unique_ptr<DebuggerInfo> dbgInfo;
};

extern WorkerBase* gpWorker;