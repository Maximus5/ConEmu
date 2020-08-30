
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

#include "../common/MModule.h"
#include "../common/MConHandle.h"
#include "WinConExports.h"

#include <memory>

struct ConProcess;
class DebuggerInfo;
enum class DumpProcessType;

extern MConHandle ghConOut;

// If we are in ChildGui mode (notepad, putty, etc.)
#define GUI_NOT_CREATED_YET (HWND(-1))

class WorkerBase
{
public:
	virtual ~WorkerBase();

	WorkerBase();

	WorkerBase(const WorkerBase&) = delete;
	WorkerBase(WorkerBase&&) = delete;
	WorkerBase& operator=(const WorkerBase&) = delete;
	WorkerBase& operator=(WorkerBase&&) = delete;

	virtual int Init() { return 0; };
	virtual void Done(int exitCode, bool reportShutdown = false);

	virtual int ProcessCommandLineArgs();

	virtual bool IsCmdK() const;
	virtual void SetCmdK(bool useCmdK);

	void SetRootProcessId(DWORD pid);
	void SetRootProcessHandle(HANDLE processHandle);
	void SetRootThreadId(DWORD tid);
	void SetRootThreadHandle(HANDLE threadHandle);
	void SetRootStartTime(DWORD ticks);
	void SetParentFarPid(DWORD pid);
	void SetRootProcessGui(HWND hwnd);
	DWORD RootProcessId() const;
	DWORD RootThreadId() const;
	DWORD RootProcessStartTime() const;
	DWORD ParentFarPid() const;
	HANDLE RootProcessHandle() const;
	HWND RootProcessGui() const;
	void CloseRootProcessHandles();
	ConProcess& Processes();

	const CONSOLE_SCREEN_BUFFER_INFO& GetSbi() const;

	/// Try to detect if our real console is in hardware fullscreen
	bool CheckHwFullScreen();
	/// <summary>
	/// Try to switch our real console to hardware fullscreen
	/// </summary>
	/// <param name="pNewSize">[OUT] dimension of new hardware console on success</param>
	/// <returns>true on success</returns>
	bool EnterHwFullScreen(PCOORD pNewSize = nullptr);

	virtual void EnableProcessMonitor(bool enable);

	bool IsDebuggerActive() const;
	bool IsDebugProcess() const;
	bool IsDebugProcessTree() const;
	bool IsDebugCmdLineSet() const;
	int SetDebuggingPid(const wchar_t* pidList);
	int SetDebuggingExe(const wchar_t* commandLine, bool debugTree);
	void SetDebugDumpType(DumpProcessType dumpType);
	void SetDebugAutoDump(const wchar_t* interval);
	DebuggerInfo& DbgInfo();

	void CdToProfileDir();

	void CheckKeyboardLayout();
	bool IsKeyboardLayoutChanged(DWORD& pdwLayout, LPDWORD pdwErrCode = nullptr);

	const MModule& KernelModule() const;

protected:
	int ParamAlienAttachProcess();
	int ParamAttachGuiApp();
	int ParamAutoAttach() const;
	
protected:
	MModule kernel32;
	
	struct RootProcessInfo
	{
		HANDLE processHandle;
		HANDLE threadHandle;
		DWORD processId;
		DWORD threadId;
		DWORD startTime;

		HWND childGuiWnd; // could be GUI_NOT_CREATED_YET
	} rootProcess = {};

	struct FarManagerInfo
	{
		DWORD dwParentFarPid;
	} farManagerInfo = {};

	struct ConsoleInfo
	{
		DWORD dwSbiRc;
		CONSOLE_SCREEN_BUFFER_INFO sbi; // MyGetConsoleScreenBufferInfo
	} consoleInfo = {};

	std::unique_ptr<DebuggerInfo> dbgInfo;

	// Full information about our console processes
	std::shared_ptr<ConProcess> processes_;

	// Keyboard layout name
	wchar_t szKeybLayout[KL_NAMELENGTH+1] = L"";

	FGetConsoleKeyboardLayoutName pfnGetConsoleKeyboardLayoutName = nullptr;
	
	FGetConsoleDisplayMode pfnGetConsoleDisplayMode = nullptr;
	SetConsoleDisplayMode_t pfnSetConsoleDisplayMode = nullptr;

	bool wasFullscreenMode_ = false;
};

extern WorkerBase* gpWorker;  // NOLINT(readability-redundant-declaration)
