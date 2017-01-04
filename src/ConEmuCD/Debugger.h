
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

int AttachRootProcessHandle();
int RunDebugger();
void GenerateMiniDumpFromCtrlBreak();

struct CEDebugProcessInfo
{
	DWORD  nPID; // дублирование ключа
	BOOL   bWasBreak; // TRUE после первого EXCEPTION_BREAKPOINT
	HANDLE hProcess;
};

struct DebuggerInfo
{
	LPWSTR pszDebuggingCmdLine;
	BOOL   bDebuggerActive;
	BOOL   bDebuggerRequestDump;
	BOOL   bUserRequestDump;
	HANDLE hDebugThread;
	HANDLE hDebugReady;
	DWORD  dwDebugThreadId;

	MMap<DWORD,CEDebugProcessInfo>* pDebugTreeProcesses /*= NULL*/;
	MArray<DWORD>* pDebugAttachProcesses /*= NULL*/;
	BOOL  bDebugProcess /*= FALSE*/;
	BOOL  bDebugProcessTree /*= FALSE*/;
	int   nDebugDumpProcess /*= 0*/; // 1 - ask user, 2 - minidump, 3 - fulldump
	BOOL  bDebugMultiProcess; // Debugger was asked for multiple processes simultaneously
	int   nProcessCount;
	int   nWaitTreeBreaks;
	BOOL  bAutoDump; // For creating series of mini-dumps, useful for "snapshotting" running application
	DWORD nAutoInterval;
	LONG  nDumpsCounter;

	HMODULE hDbghelp;
	FARPROC MiniDumpWriteDump_f;
};
