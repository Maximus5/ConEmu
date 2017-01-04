
/*
Copyright (c) 2011-2017 Maximus5
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

#include "../common/CmdLine.h"
#include "../common/RConStartArgs.h"

struct RConStartArgs;

typedef DWORD ChangeExecFlags;
const ChangeExecFlags
	CEF_NEWCON_SWITCH   = 1, // If command line has "-new_console"
	CEF_NEWCON_PREPEND  = 2, // If we need to prepend command with "-new_console" to ensure that "start cmd" will be processed properly
	CEF_DEFAULT = 0;

class CShellProc
{
public:
	static bool  mb_StartingNewGuiChildTab;
	static DWORD mn_LastStartedPID;

private:
	RConStartArgs m_Args;

	UINT mn_CP; // = AreFileApisANSI() ? CP_ACP : CP_OEMCP;

	// Для конвертации параметров Ansi функций (работаем через Unicode для унификации)
	LPWSTR mpwsz_TempAction; // = str2wcs(asAction, nCP);
	LPWSTR mpwsz_TempFile; // = str2wcs(asFile, nCP);
	LPWSTR mpwsz_TempParam; // = str2wcs(asParam, nCP);

	LPSTR  mpsz_TempRetFile;
	LPSTR  mpsz_TempRetParam;
	LPSTR  mpsz_TempRetDir;
	LPWSTR mpwsz_TempRetFile;
	LPWSTR mpwsz_TempRetParam;
	LPWSTR mpwsz_TempRetDir;

	// Копии для ShellExecuteEx - менять мы можем только свою память
	LPSHELLEXECUTEINFOA mlp_ExecInfoA, mlp_SaveExecInfoA;
	LPSHELLEXECUTEINFOW mlp_ExecInfoW, mlp_SaveExecInfoW;

	// Информация о запускаемом процессе
	DWORD mn_ImageSubsystem, mn_ImageBits;
	CEStr ms_ExeTmp;
	BOOL mb_WasSuspended; // Если TRUE - значит при вызове CreateProcessXXX уже был флаг CREATE_SUSPENDED
	BOOL mb_NeedInjects;
	BOOL mb_DebugWasRequested;
	BOOL mb_HiddenConsoleDetachNeed;
	BOOL mb_PostInjectWasRequested;
	//BOOL mb_DosBoxAllowed;
	bool mb_Opt_DontInject; // ConEmuHooks=OFF
	bool mb_Opt_SkipNewConsole; // ConEmuHooks=NOARG
	bool mb_Opt_SkipCmdStart; // ConEmuHooks=NOSTART
	void CheckHooksDisabled();
	bool GetStartingExeName(LPCWSTR asFile, LPCWSTR asParam, CEStr& rsExeTmp);

	BOOL mb_isCurrentGuiClient;
	void CheckIsCurrentGuiClient();

	//static int mn_InShellExecuteEx;
	BOOL mb_InShellExecuteEx;

	CESERVER_CONSOLE_MAPPING_HDR m_SrvMapping;

	HWND mh_PreConEmuWnd, mh_PreConEmuWndDC;
	BOOL mb_TempConEmuWnd;

private:
	wchar_t* str2wcs(const char* psz, UINT anCP);
	char* wcs2str(const wchar_t* pwsz, UINT anCP);
	int PrepareExecuteParms(
				enum CmdOnCreateType aCmd,
				LPCWSTR asAction, LPCWSTR asFile, LPCWSTR asParam, LPCWSTR asDir,
				DWORD* anShellFlags, DWORD* anCreateFlags, DWORD* anStartFlags, DWORD* anShowCmd, // или Shell & Create флаги
				HANDLE* lphStdIn, HANDLE* lphStdOut, HANDLE* lphStdErr,
				LPWSTR* psFile, LPWSTR* psParam, LPWSTR* psStartDir);
	BOOL ChangeExecuteParms(enum CmdOnCreateType aCmd, bool bConsoleMode,
				LPCWSTR asFile, LPCWSTR asParam, LPCWSTR asExeFile,
				ChangeExecFlags Flags, const RConStartArgs& args,
				DWORD& ImageBits, DWORD& ImageSubsystem,
				LPWSTR* psFile, LPWSTR* psParam);
	BOOL FixShellArgs(DWORD afMask, HWND ahWnd, DWORD* pfMask, HWND* phWnd);
	HWND FindCheckConEmuWindow();
	void LogShellString(LPCWSTR asMessage);
	void RunInjectHooks(LPCWSTR asFrom, PROCESS_INFORMATION *lpPI);
public:
	CESERVER_REQ* NewCmdOnCreate(enum CmdOnCreateType aCmd,
				LPCWSTR asAction, LPCWSTR asFile, LPCWSTR asParam, LPCWSTR asDir,
				DWORD* anShellFlags, DWORD* anCreateFlags, DWORD* anStartFlags, DWORD* anShowCmd,
				int nImageBits, int nImageSubsystem,
				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr
				/*wchar_t (&szBaseDir)[MAX_PATH+2], BOOL& bDosBoxAllowed*/);
	BOOL LoadSrvMapping(BOOL bLightCheck = FALSE);
	BOOL GetLogLibraries();
	const RConStartArgs* GetArgs();
public:
	CShellProc();
	~CShellProc();
public:
	// Эти функции возвращают TRUE, если команда обрабатывается (нужно будет делать Inject)
	BOOL OnShellExecuteA(LPCSTR* asAction, LPCSTR* asFile, LPCSTR* asDir, LPCSTR* asParam, DWORD* anFlags, DWORD* anShowCmd);
	BOOL OnShellExecuteW(LPCWSTR* asAction, LPCWSTR* asFile, LPCWSTR* asDir, LPCWSTR* asParam, DWORD* anFlags, DWORD* anShowCmd);
	BOOL OnShellExecuteExA(LPSHELLEXECUTEINFOA* lpExecInfo);
	BOOL OnShellExecuteExW(LPSHELLEXECUTEINFOW* lpExecInfo);
	BOOL OnCreateProcessA(LPCSTR* asFile, LPCSTR* asCmdLine, LPCSTR* asDir, DWORD* anCreationFlags, LPSTARTUPINFOA lpSI);
	BOOL OnCreateProcessW(LPCWSTR* asFile, LPCWSTR* asCmdLine, LPCWSTR* asDir, DWORD* anCreationFlags, LPSTARTUPINFOW lpSI);
	// Вызывается после успешного создания процесса
	void OnCreateProcessFinished(BOOL abSucceeded, PROCESS_INFORMATION *lpPI);
	void OnShellFinished(BOOL abSucceeded, HINSTANCE ahInstApp, HANDLE ahProcess);
	// Used with DefTerm+VSDebugger
	static bool OnResumeDebugeeThreadCalled(HANDLE hThread, PROCESS_INFORMATION* lpPI = NULL);
protected:
	static PROCESS_INFORMATION m_WaitDebugVsThread;
public:
	// Helper
	bool GetLinkProperties(LPCWSTR asLnkFile, CEStr& rsExe, CEStr& rsArgs, CEStr& rsWorkDir);
	bool InitOle32();
protected:
	HMODULE hOle32;
	typedef HRESULT (WINAPI* CoInitializeEx_t)(LPVOID pvReserved, DWORD dwCoInit);
	typedef HRESULT (WINAPI* CoCreateInstance_t)(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);
	CoInitializeEx_t CoInitializeEx_f;
	CoCreateInstance_t CoCreateInstance_f;
};

// Service functions
typedef DWORD (WINAPI* GetProcessId_t)(HANDLE Process);
extern GetProcessId_t gfGetProcessId;

#ifdef _DEBUG
#ifndef CONEMU_MINIMAL
void TestShellProcessor();
#endif
#endif
