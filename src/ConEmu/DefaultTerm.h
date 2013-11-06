
/*
Copyright (c) 2012 Maximus5
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

#include "../common/MArray.h"

class CDefaultTerminal
{
public:
	CDefaultTerminal();
	~CDefaultTerminal();

	bool CheckForeground(HWND hFore, DWORD nForePID, bool bRunInThread = true);
	void PostCreated(bool bWaitForReady = false, bool bShowErrors = false);
	void CheckRegisterOsStartup();
	bool IsRegisteredOsStartup(wchar_t* rsValue, DWORD cchMax, bool* pbLeaveInTSA);
	void OnHookedListChanged();
	//void OnDefTermStarted(CESERVER_REQ* pIn);

	bool isDefaultTerminalAllowed();

private:
	struct ProcessInfo
	{
		HANDLE    hProcess;
		DWORD     nPID;
		//BOOL    bHooksSucceeded;
		DWORD     nHookTick; // informational field
	};
	struct ProcessIgnore
	{
		DWORD nPID;
		DWORD nTick;
	};
private:
	HWND mh_LastWnd, mh_LastIgnoredWnd, mh_LastCall;
	//HANDLE mh_SignEvent;
	BOOL mb_ReadyToHook;
	MArray<ProcessInfo> m_Processed;
	void ClearProcessed(bool bForceAll);
	static DWORD WINAPI PostCreatedThread(LPVOID lpParameter);
	static DWORD WINAPI PostCheckThread(LPVOID lpParameter);
	bool CheckShellWindow();
	bool mb_Initialized;
	DWORD mn_PostThreadId;
	bool  mb_PostCreatedThread;
	MArray<HANDLE> m_Threads;
	CRITICAL_SECTION mcs;
	void ClearThreads(bool bForceTerminate);
	struct ThreadArg
	{
		CDefaultTerminal* pTerm;
		HWND hFore;
		DWORD nForePID;
	};
};
