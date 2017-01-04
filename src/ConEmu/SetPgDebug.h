
/*
Copyright (c) 2016-2017 Maximus5
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

#include <windows.h>

#include "SetPgBase.h"

class CSetPgDebug
	: public CSetPgBase
{
public:
	static CSetPgBase* Create() { return new CSetPgDebug(); };
	static TabHwndIndex PageType() { return thi_Debug; };
	virtual TabHwndIndex GetPageType() override { return PageType(); };
public:
	CSetPgDebug();
	virtual ~CSetPgDebug();

public:
	// Methods
	virtual LRESULT OnInitDialog(HWND hDlg, bool abInitial) override;

	LRESULT OnActivityLogNotify(WPARAM wParam, LPARAM lParam);
	void OnSaveActivityLogFile();

public:
	// Definitions
	enum LogProcessColumns
	{
		lpc_Time = 0,
		lpc_PPID,
		lpc_Func,
		lpc_Oper,
		lpc_Bits,
		lpc_System,
		lpc_App,
		lpc_Params,
		lpc_Flags,
		lpc_StdIn,
		lpc_StdOut,
		lpc_StdErr,
	};
	enum LogInputColumns
	{
		lic_Time = 0,
		lic_Type,
		lic_Dup,
		lic_Event,
	};
	enum LogCommandsColumns
	{
		lcc_InOut = 0,
		lcc_Time,
		lcc_Duration,
		lcc_Command,
		lcc_Size,
		lcc_PID,
		lcc_Pipe,
		lcc_Extra,
	};
	enum LogAnsiColumns
	{
		lac_Time = 0,
		lac_Sequence,
	};
	struct LogCommandsData
	{
		BOOL  bInput, bMainThread;
		DWORD nTick, nDur, nCmd, nSize, nPID;
		wchar_t szPipe[64];
		wchar_t szExtra[128];
	};

	#define DBGMSG_LOG_ID (WM_APP+100)
	#define DBGMSG_LOG_SHELL_MAGIC 0xD73A34
	#define DBGMSG_LOG_INPUT_MAGIC 0xD73A35
	#define DBGMSG_LOG_CMD_MAGIC   0xD73A36

	struct DebugLogShellActivity
	{
		DWORD   nParentPID, nParentBits, nChildPID;
		wchar_t szFunction[32];
		wchar_t* pszAction;
		wchar_t* pszFile;
		wchar_t* pszParam;
		int     nImageSubsystem;
		int     nImageBits;
		DWORD   nShellFlags, nCreateFlags, nStartFlags, nShowCmd;
		BOOL    bDos;
		DWORD   hStdIn, hStdOut, hStdErr;
	};

public:
	// Helpers
	static GuiLoggingType GetActivityLoggingType();
	void SetLoggingType(GuiLoggingType aNewLogType);

	// Loggers
	static void debugLogCommand(CESERVER_REQ* pInfo, BOOL abInput, DWORD anTick, DWORD anDur, LPCWSTR asPipe, CESERVER_REQ* pResult = NULL);
	static void debugLogInfo(CESERVER_REQ_PEEKREADINFO* pInfo);
	static void debugLogShell(DWORD nParentPID, CESERVER_REQ_ONCREATEPROCESS* pInfo);

	// Called from CSetPgBase over DBGMSG_LOG_ID
	void debugLogCommand(LogCommandsData* apData);
	void debugLogShell(DebugLogShellActivity *pShl);

protected:
	// Members
	GuiLoggingType m_ActivityLoggingType;
	DWORD mn_ActivityCmdStartTick;

	// Helpers
	static void debugLogShellText(wchar_t* &pszParamEx, LPCWSTR asFile);
};
