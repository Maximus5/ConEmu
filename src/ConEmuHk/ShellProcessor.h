
/*
Copyright (c) 2011-present Maximus5
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

#include "../common/Common.h"
#include "../common/CmdLine.h"
#include "../common/RConStartArgs.h"
#include <memory>

struct RConStartArgs;

typedef DWORD ChangeExecFlags;
const ChangeExecFlags
	CEF_NEWCON_SWITCH   = 1, // If command line has "-new_console"
	CEF_NEWCON_PREPEND  = 2, // If we need to prepend command with "-new_console" to ensure that "start cmd" will be processed properly
	CEF_DEFAULT = 0;

enum CmdOnCreateType;

class CShellProc
{
public:
	static bool  mb_StartingNewGuiChildTab;
	static DWORD mn_LastStartedPID;

private:
	RConStartArgs m_Args;

	UINT mn_CP; // = AreFileApisANSI() ? CP_ACP : CP_OEMCP;

	// Для конвертации параметров Ansi функций (работаем через Unicode для унификации)
	LPWSTR mpwsz_TempAction = nullptr; // = str2wcs(asAction, nCP);
	LPWSTR mpwsz_TempFile = nullptr; // = str2wcs(asFile, nCP);
	LPWSTR mpwsz_TempParam = nullptr; // = str2wcs(asParam, nCP);

	LPSTR  mpsz_TempRetFile = nullptr;
	LPSTR  mpsz_TempRetParam = nullptr;
	LPSTR  mpsz_TempRetDir = nullptr;
	LPWSTR mpwsz_TempRetFile = nullptr;
	LPWSTR mpwsz_TempRetParam = nullptr;
	LPWSTR mpwsz_TempRetDir = nullptr;

	template<typename T>
	struct StructDeleter { // deleter
		void operator()(T* p) const
		{
			SafeFree(p);
		}
	};

	// Copies for ShellExecuteEx - we may change only our memory
	LPSHELLEXECUTEINFOA mlp_ExecInfoA = nullptr, mlp_SaveExecInfoA = nullptr;
	LPSHELLEXECUTEINFOW mlp_ExecInfoW = nullptr, mlp_SaveExecInfoW = nullptr;
	// Copies for CreateProcess
	std::unique_ptr<STARTUPINFOA, StructDeleter<STARTUPINFOA>> m_lpStartupInfoA;
	std::unique_ptr<STARTUPINFOW, StructDeleter<STARTUPINFOW>> m_lpStartupInfoW;

	// Information about starting process
	DWORD mn_ImageSubsystem = 0, mn_ImageBits = 0;
	CmdArg ms_ExeTmp;
	// if TRUE - than during CreateProcessXXX the flag CREATE_SUSPENDED was already set
	bool mb_WasSuspended = false;
	// Controls if we need to inject ConEmuHk into started executable (either original, or changed ConEmu.exe / ConEmuC.exe).
	// Modified via SetNeedInjects.
	bool mb_NeedInjects = false;
	// Controls if we have to inject ConEmuHk regardless of DefTerm settings
	// Modified via SetForceInjectOriginal.
	bool mb_ForceInjectOriginal = false;
	// Controls if we need to run console server, if value is false - running fo ConEmu.exe is allowed.
	// Modified via SetConsoleMode.
	bool mb_ConsoleMode = false;
	bool mb_DebugWasRequested = false;
	bool mb_HiddenConsoleDetachNeed = false;
	bool mb_PostInjectWasRequested = false;
	bool mb_Opt_DontInject = false; // ConEmuHooks=OFF
	bool mb_Opt_SkipNewConsole = false; // ConEmuHooks=NOARG
	bool mb_Opt_SkipCmdStart = false; // ConEmuHooks=NOSTART

	enum class WorkOptions : uint32_t
	{
		None = 0,
		DebugWasRequested = 0x0001,
		GuiApp = 0x0002,
		VsNetHostRequested = 0x0004,
		VsDebugConsole = 0x0008,
	};
	WorkOptions workOptions_ = WorkOptions::None;

	void CheckHooksDisabled();
	static bool GetStartingExeName(LPCWSTR asFile, LPCWSTR asParam, CEStr& rsExeTmp);

	BOOL mb_isCurrentGuiClient = FALSE;
	void CheckIsCurrentGuiClient();

	//static int mn_InShellExecuteEx;
	BOOL mb_InShellExecuteEx = FALSE;

	CESERVER_CONSOLE_MAPPING_HDR m_SrvMapping = {};

	HWND mh_PreConEmuWnd = nullptr, mh_PreConEmuWndDC = nullptr;
	BOOL mb_TempConEmuWnd = FALSE;

	enum class PrepareExecuteResult
	{
		// Restrict execution, leads to ERROR_FILE_NOT_FOUND
		Restrict = -1,
		// Bypass to WinAPI without modifications
		Bypass = 0,
		// Strings or flags were modified
		Modified = 1,
	};

	struct CreatePrepareData
	{
		bool consoleNoWindow;
		DWORD showCmd;
		DWORD defaultShowCmd;
	};

private:
	wchar_t* str2wcs(const char* psz, UINT anCP);
	char* wcs2str(const wchar_t* pwsz, UINT anCP);
	bool IsAnsiConLoader(LPCWSTR asFile, LPCWSTR asParam);
	static bool PrepareNewConsoleInFile(
				CmdOnCreateType aCmd, LPCWSTR& asFile, LPCWSTR& asParam,
				CEStr& lsReplaceFile, CEStr& lsReplaceParm, CEStr& exeName);
	bool CheckForDefaultTerminal(
				CmdOnCreateType aCmd, LPCWSTR asAction, const DWORD* anShellFlags, const DWORD* anCreateFlags,
				const DWORD* anShowCmd, bool& bIgnoreSuspended, bool& bDebugWasRequested, bool& lbGnuDebugger);
	void CheckForExeName(const CEStr& exeName, const DWORD* anCreateFlags, bool lbGnuDebugger,
		bool& bDebugWasRequested, bool& lbGuiApp, bool& bVsNetHostRequested);
	PrepareExecuteResult PrepareExecuteParams(
				enum CmdOnCreateType aCmd,
				LPCWSTR asAction, LPCWSTR asFile, LPCWSTR asParam, LPCWSTR asDir,
				DWORD* anShellFlags, DWORD* anCreateFlags, DWORD* anStartFlags, DWORD* anShowCmd, // или Shell & Create флаги
				HANDLE* lphStdIn, HANDLE* lphStdOut, HANDLE* lphStdErr,
				LPWSTR* psFile, LPWSTR* psParam, LPWSTR* psStartDir);
	BOOL ChangeExecuteParams(enum CmdOnCreateType aCmd,
				LPCWSTR asFile, LPCWSTR asParam,
				ChangeExecFlags Flags, const RConStartArgs& args,
				DWORD& ImageBits, DWORD& ImageSubsystem,
				LPWSTR* psFile, LPWSTR* psParam);
	BOOL FixShellArgs(DWORD afMask, HWND ahWnd, DWORD* pfMask, HWND* phWnd) const;
	HWND FindCheckConEmuWindow();
	void LogExitLine(int rc, int line) const;
	void LogShellString(LPCWSTR asMessage) const;
	void RunInjectHooks(LPCWSTR asFrom, PROCESS_INFORMATION *lpPI);
	// Controls if we need to inject ConEmuHk into started executable (either original, or changed ConEmu.exe / ConEmuC.exe)
	void SetNeedInjects(bool value);
	// Controls if we have to inject ConEmuHk regardless of DefTerm settings
	void SetForceInjectOriginal(bool value);
	// Controls if we need to run console server, if value is false - running fo ConEmu.exe is allowed
	void SetConsoleMode(bool value);
	static bool IsInterceptionEnabled();
	CreatePrepareData OnCreateProcessPrepare(const DWORD* anCreationFlags, DWORD dwFlags, WORD wShowWindow, DWORD dwX, DWORD dwY);
	void OnCreateProcessResult(PrepareExecuteResult prepareResult, const CreatePrepareData& state, DWORD* anCreationFlags, WORD& siShowWindow, DWORD& siFlags);
public:
	CESERVER_REQ* NewCmdOnCreate(CmdOnCreateType aCmd,
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
	BOOL OnCreateProcessA(LPCSTR* asFile, LPCSTR* asCmdLine, LPCSTR* asDir, DWORD* anCreationFlags, LPSTARTUPINFOA* ppStartupInfo);
	BOOL OnCreateProcessW(LPCWSTR* asFile, LPCWSTR* asCmdLine, LPCWSTR* asDir, DWORD* anCreationFlags, LPSTARTUPINFOW* ppStartupInfo);
	// Вызывается после успешного создания процесса
	void OnCreateProcessFinished(BOOL abSucceeded, PROCESS_INFORMATION *lpPI);
	void OnShellFinished(BOOL abSucceeded, HINSTANCE ahInstApp, HANDLE ahProcess);
	// Used with DefTerm+VSDebugger
	static bool OnResumeDebuggeeThreadCalled(HANDLE hThread, PROCESS_INFORMATION* lpPI = nullptr);
protected:
	static PROCESS_INFORMATION m_WaitDebugVsThread;
public:
	// Helper
	bool GetLinkProperties(LPCWSTR asLnkFile, CEStr& rsExe, CEStr& rsArgs, CEStr& rsWorkDir);
	bool InitOle32();
protected:
	HMODULE hOle32 = nullptr;
	typedef HRESULT (WINAPI* CoInitializeEx_t)(LPVOID pvReserved, DWORD dwCoInit);
	typedef HRESULT (WINAPI* CoCreateInstance_t)(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);
	CoInitializeEx_t CoInitializeEx_f = nullptr;
	CoCreateInstance_t CoCreateInstance_f = nullptr;
};

// Service functions
typedef DWORD (WINAPI* GetProcessId_t)(HANDLE Process);
extern GetProcessId_t gfGetProcessId;
