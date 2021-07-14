
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

#include "../common/CEStr.h"
#include "../common/MArray.h"
#include "../common/MMap.h"
#include "../common/MModule.h"

struct CEDebugProcessInfo
{
	DWORD  nPID; // дублирование ключа
	BOOL   bWasBreak; // TRUE после первого EXCEPTION_BREAKPOINT
	HANDLE hProcess;
};

enum class DumpProcessType
{
	None = 0,
	AskUser = 1,
	MiniDump = 2,
	FullDump = 3,
};

class DebuggerInfo
{
public:
	DebuggerInfo();
	~DebuggerInfo();

	DebuggerInfo(const DebuggerInfo&) = delete;
	DebuggerInfo(DebuggerInfo&&) = delete;
	DebuggerInfo& operator=(const DebuggerInfo&) = delete;
	DebuggerInfo& operator=(DebuggerInfo&&) = delete;

	static DWORD WINAPI DebugThread(LPVOID lpvParam);
	
	void WriteMiniDump(DWORD dwProcessId, DWORD dwThreadId, EXCEPTION_RECORD *pExceptionRecord, LPCSTR asConfirmText = NULL, BOOL bTreeBreak = FALSE);
	void GenerateTreeDebugBreak(DWORD nExcludePID);
	void PrintDebugInfo() const;
	void UpdateDebuggerTitle() const;
	DumpProcessType ConfirmDumpType(DWORD dwProcessId, LPCSTR asConfirmText /*= NULL*/) const;
	int RunDebugger();
	HANDLE GetProcessHandleForDebug(DWORD nPID, LPDWORD pnErrCode = nullptr) const;
	void AttachConHost(DWORD nConHostPID) const;
	bool IsDumpMulti() const;
	void FormatDumpName(wchar_t* DmpFile, size_t cchDmpMax, DWORD dwProcessId, bool bTrap, bool bFull) const;
	bool GetSaveDumpName(DWORD dwProcessId, bool bFull, wchar_t* dmpFile, DWORD cchMaxDmpFile) const;
	void ProcessDebugEvent();
	void GenerateMiniDumpFromCtrlBreak();

	CEStr  szDebuggingCmdLine;
	bool   bDebuggerActive = false;
	bool   bDebuggerRequestDump = false;
	bool   bUserRequestDump = false;
	HANDLE hDebugThread = nullptr;
	HANDLE hDebugReady = nullptr;
	DWORD  dwDebugThreadId = 0;

	MMap<DWORD,CEDebugProcessInfo>* pDebugTreeProcesses = nullptr;
	MArray<DWORD>* pDebugAttachProcesses = nullptr;
	bool  bDebugProcess = false;
	bool  bDebugProcessTree = false;
	DumpProcessType debugDumpProcess = DumpProcessType::None;
	bool  bDebugMultiProcess = false; // Debugger was asked for multiple processes simultaneously
	int   nProcessCount = 0;
	int   nWaitTreeBreaks = 0;
	bool  bAutoDump = false; // For creating series of mini-dumps, useful for "snapshotting" running application
	DWORD nAutoInterval = 0; // milliseconds
	LONG  nDumpsCounter = 0;

	MModule dbgHelpDll{};
	FARPROC MiniDumpWriteDump_f = nullptr;

	typedef BOOL (WINAPI *FDebugActiveProcessStop)(DWORD dwProcessId);
	FDebugActiveProcessStop pfnDebugActiveProcessStop = nullptr;

	typedef BOOL (WINAPI *FDebugSetProcessKillOnExit)(BOOL KillOnExit);
	FDebugSetProcessKillOnExit pfnDebugSetProcessKillOnExit = nullptr;
};
