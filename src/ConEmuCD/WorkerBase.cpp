
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

#include "../common/defines.h"
#include "../common/MAssert.h"
#include "WorkerBase.h"


#include "ConsoleMain.h"
#include "ConsoleState.h"
#include "Debugger.h"
#include "ExitCodes.h"

WorkerBase* gpWorker = nullptr;

WorkerBase::WorkerBase()
	: kernel32(GetModuleHandleW(L"kernel32.dll"))
{
	kernel32.GetProcAddress("GetConsoleKeyboardLayoutNameW", pfnGetConsoleKeyboardLayoutName);
}

void WorkerBase::Done(const int /*exitCode*/, const bool /*reportShutdown*/)
{
	dbgInfo.reset();
}

bool WorkerBase::IsCmdK() const
{
	return false;
}

void WorkerBase::SetCmdK(bool useCmdK)
{
	_ASSERTE(useCmdK == false);
	// nothing to do in base!
}

void WorkerBase::SetRootProcessId(const DWORD pid)
{
	_ASSERTE(pid != 0);
	this->rootProcess.processId = pid;
}

void WorkerBase::SetRootProcessHandle(HANDLE processHandle)
{
	this->rootProcess.processHandle = processHandle;
}

void WorkerBase::SetRootThreadId(const DWORD tid)
{
	this->rootProcess.threadId = tid;
}

void WorkerBase::SetRootThreadHandle(HANDLE threadHandle)
{
	this->rootProcess.threadHandle = threadHandle;
}

void WorkerBase::SetRootStartTime(const DWORD ticks)
{
	this->rootProcess.startTime = ticks;
}

void WorkerBase::SetParentFarPid(DWORD pid)
{
	this->farManagerInfo.dwParentFarPid = pid;
}

void WorkerBase::SetRootProcessGui(HWND hwnd)
{
	this->rootProcess.childGuiWnd = hwnd;
}

DWORD WorkerBase::RootProcessId() const
{
	return this->rootProcess.processId;
}

DWORD WorkerBase::RootThreadId() const
{
	return this->rootProcess.threadId;
}

DWORD WorkerBase::RootProcessStartTime() const
{
	return this->rootProcess.startTime;
}

DWORD WorkerBase::ParentFarPid() const
{
	return this->farManagerInfo.dwParentFarPid;
}

HANDLE WorkerBase::RootProcessHandle() const
{
	return this->rootProcess.processHandle;
}

HWND WorkerBase::RootProcessGui() const
{
	return this->rootProcess.childGuiWnd;
}

void WorkerBase::CloseRootProcessHandles()
{
	SafeCloseHandle(this->rootProcess.processHandle);
	SafeCloseHandle(this->rootProcess.threadHandle);
}

const CONSOLE_SCREEN_BUFFER_INFO& WorkerBase::GetSbi() const
{
	return this->consoleInfo.sbi;
}

void WorkerBase::EnableProcessMonitor(bool enable)
{
	// nothing to do in base
}

bool WorkerBase::IsDebuggerActive() const
{
	if (!dbgInfo)
		return false;
	return dbgInfo->bDebuggerActive;
}

bool WorkerBase::IsDebugProcess() const
{
	if (!dbgInfo)
		return false;
	return dbgInfo->bDebugProcess;
}

bool WorkerBase::IsDebugProcessTree() const
{
	if (!dbgInfo)
		return false;
	return dbgInfo->bDebugProcessTree;
}

bool WorkerBase::IsDebugCmdLineSet() const
{
	if (!dbgInfo)
		return false;
	return !dbgInfo->szDebuggingCmdLine.IsEmpty();
}

int WorkerBase::SetDebuggingPid(const wchar_t* const pidList)
{
	if (!dbgInfo)
		dbgInfo.reset(new DebuggerInfo);

	gpState->noCreateProcess_ = TRUE;
	dbgInfo->bDebugProcess = TRUE;
	dbgInfo->bDebugProcessTree = FALSE;

	wchar_t* pszEnd = nullptr;
	SetRootProcessId(wcstoul(pidList, &pszEnd, 10));

	if (RootProcessId() == 0)
	{
		// ReSharper disable twice StringLiteralTypo
		LogString(L"CERR_CARGUMENT: Debug of process was requested, but invalid PID specified");
		_printf("Debug of process was requested, but invalid PID specified:\n");
		_wprintf(GetCommandLineW());
		_printf("\n");
		_ASSERTE(FALSE && "Invalid PID specified for debugging");
		return CERR_CARGUMENT;
	}

	// "Comma" is a mark that debug/dump was requested for a bunch of processes
	if (pszEnd && (*pszEnd == L','))
	{
		dbgInfo->bDebugMultiProcess = TRUE;
		dbgInfo->pDebugAttachProcesses = new MArray<DWORD>;
		while (pszEnd && (*pszEnd == L',') && *(pszEnd + 1))
		{
			DWORD nPID = wcstoul(pszEnd + 1, &pszEnd, 10);
			if (nPID != 0)
			{
				dbgInfo->pDebugAttachProcesses->push_back(nPID);
			}
		}
	}

	return 0;
}

int WorkerBase::SetDebuggingExe(const wchar_t* const commandLine, const bool debugTree)
{
	_ASSERTE(gpWorker->IsDebugProcess() == false); // should not be set yet

	if (!dbgInfo)
		dbgInfo.reset(new DebuggerInfo);

	gpState->noCreateProcess_ = TRUE;
	dbgInfo->bDebugProcess = true;
	dbgInfo->bDebugProcessTree = debugTree;

	dbgInfo->szDebuggingCmdLine = SkipNonPrintable(commandLine);

	if (dbgInfo->szDebuggingCmdLine.IsEmpty())
	{
		// ReSharper disable twice StringLiteralTypo
		LogString(L"CERR_CARGUMENT: Debug of process was requested, but command was not found");
		_printf("Debug of process was requested, but command was not found\n");
		_ASSERTE(FALSE && "Invalid command line for debugger was passed");
		return CERR_CARGUMENT;
	}

	return 0;
}

void WorkerBase::SetDebugDumpType(DumpProcessType dumpType)
{
	if (!dbgInfo)
		dbgInfo.reset(new DebuggerInfo);

	dbgInfo->debugDumpProcess = dumpType;
}

void WorkerBase::SetDebugAutoDump(const wchar_t* interval)
{
	if (!dbgInfo)
		dbgInfo.reset(new DebuggerInfo);

	dbgInfo->debugDumpProcess = DumpProcessType::None;
	dbgInfo->bAutoDump = true;
	dbgInfo->nAutoInterval = 1000;

	if (interval && *interval && isDigit(interval[0]))
	{
		wchar_t* pszEnd = nullptr;
		DWORD nVal = wcstol(interval, &pszEnd, 10);
		if (nVal)
		{
			if (pszEnd && *pszEnd)
			{
				if (lstrcmpni(pszEnd, L"ms", 2) == 0)
				{
					// Already milliseconds
				}
				else if (lstrcmpni(pszEnd, L"s", 1) == 0)
				{
					nVal *= 60; // seconds
				}
				else if (lstrcmpni(pszEnd, L"m", 2) == 0)
				{
					nVal *= 60 * 60; // minutes
				}
			}
			dbgInfo->nAutoInterval = nVal;
		}
	}
}

DebuggerInfo& WorkerBase::DbgInfo()
{
	if (!dbgInfo)
	{
		_ASSERTE(FALSE && "dbgInfo should be set already");
		dbgInfo.reset(new DebuggerInfo);
	}

	return *dbgInfo;
}

bool WorkerBase::IsKeyboardLayoutChanged(DWORD& pdwLayout, LPDWORD pdwErrCode /*= NULL*/)
{
	bool bChanged = false;

	if (!gpWorker)
	{
		_ASSERTE(gpWorker!=nullptr);
		return false;
	}

	static bool bGetConsoleKeyboardLayoutNameImplemented = true;
	if (bGetConsoleKeyboardLayoutNameImplemented && pfnGetConsoleKeyboardLayoutName)
	{
		wchar_t szCurKeybLayout[32] = L"";

		//#ifdef _DEBUG
		//wchar_t szDbgKeybLayout[KL_NAMELENGTH/*==9*/];
		//BOOL lbGetRC = GetKeyboardLayoutName(szDbgKeybLayout); // -- не дает эффекта, поскольку "на процесс", а не на консоль
		//#endif

		// The expected result of GetConsoleKeyboardLayoutName is like "00000419"
		const BOOL bConApiRc = pfnGetConsoleKeyboardLayoutName(szCurKeybLayout) && szCurKeybLayout[0];

		const DWORD nErr = bConApiRc ? 0 : GetLastError();
		if (pdwErrCode)
			*pdwErrCode = nErr;

		/*
		if (!bConApiRc && (nErr == ERROR_GEN_FAILURE))
		{
			_ASSERTE(FALSE && "ConsKeybLayout failed");
			MModule kernel(GetModuleHandle(L"kernel32.dll"));
			BOOL (WINAPI* getLayoutName)(LPWSTR,int);
			if (kernel.GetProcAddress("GetConsoleKeyboardLayoutNameW", getLayoutName))
			{
				bConApiRc = getLayoutName(szCurKeybLayout, countof(szCurKeybLayout));
				nErr = bConApiRc ? 0 : GetLastError();
			}
		}
		*/

		if (!bConApiRc)
		{
			// If GetConsoleKeyboardLayoutName is not implemented in Windows, ERROR_MR_MID_NOT_FOUND or E_NOTIMPL will be returned.
			// (there is no matching DOS/Win32 error code for NTSTATUS code returned)
			// When this happens, we don't want to continue to call the function.
			if (nErr == ERROR_MR_MID_NOT_FOUND || LOWORD(nErr) == LOWORD(E_NOTIMPL))
			{
				bGetConsoleKeyboardLayoutNameImplemented = false;
			}

			if (this->szKeybLayout[0])
			{
				// Log only first error per session
				wcscpy_c(szCurKeybLayout, this->szKeybLayout);
			}
			else
			{
				wchar_t szErr[80];
				swprintf_c(szErr, L"ConsKeybLayout failed with code=%u forcing to GetKeyboardLayoutName or 0409", nErr);
				_ASSERTE(!bGetConsoleKeyboardLayoutNameImplemented && "ConsKeybLayout failed");
				LogString(szErr);
				if (!GetKeyboardLayoutName(szCurKeybLayout) || (szCurKeybLayout[0] == 0))
				{
					wcscpy_c(szCurKeybLayout, L"00000419");
				}
			}
		}

		if (szCurKeybLayout[0])
		{
			if (lstrcmpW(szCurKeybLayout, this->szKeybLayout))
			{
				#ifdef _DEBUG
				wchar_t szDbg[128];
				swprintf_c(szDbg,
				          L"ConEmuC: InputLayoutChanged (GetConsoleKeyboardLayoutName returns) '%s'\n",
				          szCurKeybLayout);
				OutputDebugString(szDbg);
				#endif

				if (gpLogSize)
				{
					char szInfo[128]; wchar_t szWide[128];
					swprintf_c(szWide, L"ConsKeybLayout changed from '%s' to '%s'", this->szKeybLayout, szCurKeybLayout);
					WideCharToMultiByte(CP_ACP,0,szWide,-1,szInfo,128,0,0);
					LogFunction(szInfo);
				}

				// was changed
				wcscpy_c(this->szKeybLayout, szCurKeybLayout);
				bChanged = true;
			}
		}
	}
	else if (pdwErrCode)
	{
		*pdwErrCode = static_cast<DWORD>(-1);
	}

	// The result, if possible
	{
		wchar_t *pszEnd = nullptr; //szCurKeybLayout+8;
		//WARNING("BUGBUG: 16 цифр не вернет"); -- тут именно 8 цифр. Это LayoutNAME, а не string(HKL)
		// LayoutName: "00000409", "00010409", ...
		// HKL differs, so we pass DWORD
		// HKL in x64 looks like: "0x0000000000020409", "0xFFFFFFFFF0010409"
		pdwLayout = wcstoul(this->szKeybLayout, &pszEnd, 16);
	}

	return bChanged;
}

//WARNING("BUGBUG: x64 US-Dvorak"); - done
void WorkerBase::CheckKeyboardLayout()
{
	if (!gpWorker)
	{
		_ASSERTE(gpWorker != nullptr);
		return;
	}

	if (pfnGetConsoleKeyboardLayoutName == nullptr)
	{
		return;
	}

	DWORD dwLayout = 0;

	//WARNING("BUGBUG: 16 цифр не вернет"); -- тут именно 8 цифр. Это LayoutNAME, а не string(HKL)
	// LayoutName: "00000409", "00010409", ...
	// А HKL от него отличается, так что передаем DWORD
	// HKL в x64 выглядит как: "0x0000000000020409", "0xFFFFFFFFF0010409"

	if (IsKeyboardLayoutChanged(dwLayout))
	{
		// Сменился, Отошлем в GUI
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_LANGCHANGE,sizeof(CESERVER_REQ_HDR)+sizeof(DWORD)); //-V119

		if (pIn)
		{
			//memmove(pIn->Data, &dwLayout, 4);
			pIn->dwData[0] = dwLayout;

			CESERVER_REQ* pOut = ExecuteGuiCmd(gpState->realConWnd_, pIn, gpState->realConWnd_);

			ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
	}
}
