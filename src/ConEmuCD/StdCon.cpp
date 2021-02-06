
/*
Copyright (c) 2015-present Maximus5
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

#include "StdCon.h"

#include "ConsoleMain.h"
#include "ConsoleState.h"
#include "Shutdown.h"
#include "WorkerBase.h"
#include "../common/ConEmuCheck.h"
#include "../common/EnvVar.h"
#include "../common/MProcess.h"
#include "../common/WObjects.h"

namespace StdCon
{
ReopenedHandles* gReopenedHandles = nullptr;
};

using StdCon::ReopenedHandles;

bool StdCon::AttachParentConsole(const DWORD parentPid)
{
	BOOL bAttach = FALSE;

	AttachConsole_t attachConsoleFn = nullptr;
	gpWorker->KernelModule().GetProcAddress("AttachConsole", attachConsoleFn);

	HWND hSaveCon = GetConsoleWindow();

	bool tryAttach = true;
	while (tryAttach)
	{
		tryAttach = false;

		if (attachConsoleFn)
		{
			// FreeConsole is required to call even if gState.realConWnd is already null
			// Otherwise the AttachConsole will return ERROR_ACCESS_DENIED
			FreeConsole();
			gState.realConWnd_ = nullptr;

			// Issue 998: Need to wait, while real console will appear
			// gpWorker->RootProcessHandle() еще не открыт
			HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, parentPid);
			while (hProcess && hProcess != INVALID_HANDLE_VALUE)
			{
				DWORD nConPid = 0;
				HWND hNewCon = FindWindowEx(nullptr, nullptr, RealConsoleClass, nullptr);
				while (hNewCon)
				{
					if (GetWindowThreadProcessId(hNewCon, &nConPid) && (nConPid == parentPid))
						break;
					hNewCon = FindWindowEx(nullptr, hNewCon, RealConsoleClass, nullptr);
				}

				if ((hNewCon != nullptr) || (WaitForSingleObject(hProcess, 100) == WAIT_OBJECT_0))
					break;
			}
			SafeCloseHandle(hProcess);

			// Sometimes conhost handles are created with lags, wait for a while
			const DWORD nStartTick = GetTickCount();
			const DWORD reattachDuration = 5000; // 5 sec
			while (!bAttach)
			{
				// The action
				bAttach = attachConsoleFn(parentPid);
				if (bAttach)
				{
					gState.realConWnd_ = GetConEmuHWND(2);
					gbVisibleOnStartup = IsWindowVisible(gState.realConWnd_);
					break;
				}
				// fails, try again?
				Sleep(50);
				const DWORD nDelta = (GetTickCount() - nStartTick);
				if (nDelta >= reattachDuration)
					break;
				LogString(L"Retrying AttachConsole after 50ms delay");
			}
		}
		else
		{
			SetLastError(ERROR_PROC_NOT_FOUND);
		}

		if (!bAttach)
		{
			const DWORD nErr = GetLastError();
			const size_t cchMsgMax = 10 * MAX_PATH;
			CEStr szMsg;
			szMsg.GetBuffer(cchMsgMax);
			wchar_t szTitle[MAX_PATH];
			HWND hFindConWnd = FindWindowEx(nullptr, nullptr, RealConsoleClass, nullptr);
			DWORD nFindConPid = 0;
			if (hFindConWnd) GetWindowThreadProcessId(hFindConWnd, &nFindConPid);

			PROCESSENTRY32 piCon = {}, piRoot = {};
			GetProcessInfo(parentPid, piRoot);
			if (nFindConPid == parentPid)
				piCon = piRoot;
			else if (nFindConPid)
				GetProcessInfo(nFindConPid, piCon);

			if (hFindConWnd)
				GetWindowText(hFindConWnd, szTitle, countof(szTitle));
			else
				szTitle[0] = 0;

			_wsprintf(szMsg.ms_Val, SKIPLEN(cchMsgMax)
				L"AttachConsole(PID=%u) failed, code=%u\n"
				L"[%u]: %s\n"
				L"Top console HWND=x%08X, PID=%u, %s\n%s\n---\n"
				L"Prev (self) console HWND=x%08X\n\n"
				L"Retry?",
				parentPid, nErr,
				parentPid, piRoot.szExeFile,
				LODWORD(hFindConWnd), nFindConPid, piCon.szExeFile, szTitle,
				LODWORD(hSaveCon)
			);

			swprintf_c(szTitle, L"%s: PID=%u", gsModuleName, GetCurrentProcessId());

			const int nBtn = MessageBox(nullptr, szMsg, szTitle, MB_ICONSTOP | MB_SYSTEMMODAL | MB_RETRYCANCEL);

			if (nBtn == IDRETRY)
			{
				tryAttach = true;
				continue;
			}

			return false;
		}
	}

	return bAttach;
}

void StdCon::SetWin32TermMode()
{
	//_ASSERTE(FALSE && "Continue to CECMD_STARTXTERM");
	if (!gnMainServerPID)
	{
		PrintBuffer(L"ERROR: ConEmuC was unable to detect console server PID\n");
		return;
	}

	CESERVER_REQ* pIn2 = ExecuteNewCmd(CECMD_STARTXTERM, sizeof(CESERVER_REQ_HDR) + 4 * sizeof(DWORD));
	if (pIn2)
	{
		pIn2->dwData[0] = tmc_TerminalType;
		pIn2->dwData[1] = te_win32;  // NOLINT(clang-diagnostic-array-bounds)
		pIn2->dwData[2] = GetCurrentProcessId();  // NOLINT(clang-diagnostic-array-bounds)
		// Block connector reading thread by our PID
		pIn2->dwData[3] = GetCurrentProcessId(); // NOLINT(clang-diagnostic-array-bounds)

		CESERVER_REQ* pOut = ExecuteSrvCmd(gnMainServerPID, pIn2, gState.realConWnd_);
		ExecuteFreeResult(pIn2);
		ExecuteFreeResult(pOut);
	}
}

// Restore "native" console functionality on cygwin/msys/wsl handles?
void StdCon::ReopenConsoleHandles(STARTUPINFO* si)
{
	// _ASSERTE(FALSE && "ReopenConsoleHandles");
	if (!gState.reopenStdHandles_)
		return;
	gState.reopenStdHandles_ = false; // do it only once
	if (!gReopenedHandles)
		gReopenedHandles = new ReopenedHandles();
	if (!gReopenedHandles->Reopen(si))
		PrintBuffer(L"ERROR: ConEmuC was unable to set STD console mode\n");
	else
		SetWin32TermMode();
	// #CONNECTOR On exit we shall return current mode (retrieved from server?)
}

void ReopenedHandles::Deleter(LPARAM lParam)
{
	SafeDelete(gReopenedHandles);
}

ReopenedHandles::ReopenedHandles()
{
	for (auto& h : handles_)
	{
		h.prevHandle_ = GetStdHandle(h.channel_);
	}
	Shutdown::RegisterEvent(Deleter, 0);
}

ReopenedHandles::~ReopenedHandles()
{
	for (auto& h : handles_)
	{
		if (h.handle_)
		{
			SetStdHandle(h.channel_, h.prevHandle_);
			delete h.handle_;
		}
	}
};

bool ReopenedHandles::Reopen(STARTUPINFO* si)
{
	bool success = true;
	// "Connect" to the real console
	if (success && !gState.realConWnd_)
	{
		_ASSERTE(GetConsoleWindow() == gState.realConWnd_);
		const CEStr srv_pid(GetEnvVar(ENV_CONEMUSERVERPID_VAR_W));
		const DWORD pid = srv_pid ? wcstoul(srv_pid.c_str(L""), nullptr, 10) : 0;
		success = pid ? AttachParentConsole(pid) : false;
		const DWORD err = success ? 0 : GetLastError();
		if (success)
		{
			if (!gnMainServerPID)
				gnMainServerPID = pid;
		}
		else
		{
			wchar_t szError[120];
			msprintf(szError, countof(szError),
				L"ConEmuC: AttachConsole failed, code=%u (x%04X), can't initialize ConIn/ConOut\n",
				err, err);
			PrintBuffer(szError);
		}
	}
	if (success)
	{
		for (auto& h : handles_)
		{
			if (!h.handle_)
				h.handle_ = new MConHandle(h.name_);
			if (!((success = (h.handle_->GetHandle() != INVALID_HANDLE_VALUE))))
				break;
		}
	}
	// Change Std handles
	if (success)
	{
		for (auto& h : handles_)
		{
			if (!((success = SetStdHandle(h.channel_, h.handle_->GetHandle()))))
				break;
		}
	}
	// And modify current si
	if (success && si)
	{
		si->hStdInput = handles_[0].handle_->GetHandle();
		si->hStdOutput = handles_[1].handle_->GetHandle();
		si->hStdError = handles_[2].handle_->GetHandle();
	}
	// On errors - reverts what was done
	if (!success)
	{
		for (auto& h : handles_)
		{
			SetStdHandle(h.channel_, h.prevHandle_);
			SafeDelete(h.handle_);
		}
	}
	return success;
}
