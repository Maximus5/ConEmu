
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

#define SHOWDEBUGSTR
#define DEBUGSTRSIZE(x) DEBUGSTR(x)

#include "../common/defines.h"
#include "../common/MAssert.h"
#include "../common/MStrDup.h"
#include "../common/shlobj.h"
#include "../common/WConsole.h"
#include "../common/WObjects.h"
#include "../common/WUser.h"

#include "WorkerBase.h"


#include "ConEmuCmd.h"
#include "ConProcess.h"
#include "ConsoleArgs.h"
#include "ConsoleMain.h"
#include "ConsoleState.h"
#include "Debugger.h"
#include "DumpOnException.h"
#include "ExitCodes.h"
#include "ExportedFunctions.h"
#include "StartEnv.h"
#include "StdCon.h"
#include "../common/DefTermChildMap.h"
#include "../common/MProcess.h"
#include "../common/ProcessSetEnv.h"
#include "../common/SetEnvVar.h"

/* Console Handles */
MConHandle ghConOut(L"CONOUT$");

WorkerBase* gpWorker = nullptr;

namespace
{
bool IsWin10Build9879()
{
	if (!IsWin10())
		return false;
	_ASSERTE(gState.osVerInfo_.dwMajorVersion != 0);
	return gState.osVerInfo_.dwBuildNumber == 9879;
}
}

WorkerBase::~WorkerBase()
{
	DoneCreateDumpOnException();
}

WorkerBase::WorkerBase()
	: kernel32(L"kernel32.dll")
	, processes_(std::make_shared<ConProcess>(kernel32))
{
	kernel32.GetProcAddress("GetConsoleKeyboardLayoutNameW", pfnGetConsoleKeyboardLayoutName);
	kernel32.GetProcAddress("GetConsoleDisplayMode", pfnGetConsoleDisplayMode);

	gpLocalSecurity = LocalSecurity();

	// This could be nullptr when process was started as detached
	gState.realConWnd_ = GetConEmuHWND(2);
	gbVisibleOnStartup = IsWindowVisible(gState.realConWnd_);
	gnSelfPID = GetCurrentProcessId();
	gdwMainThreadId = GetCurrentThreadId();

	#ifdef _DEBUG
	if (gState.realConWnd_)
	{
		// This event could be used in debug version of Far Manager
		wchar_t szEvtName[64] = L"";
		swprintf_c(szEvtName, L"FARconEXEC:%08X", LODWORD(gState.realConWnd_));
		ghFarInExecuteEvent = CreateEvent(0, TRUE, FALSE, szEvtName);
	}
	#endif
}

void WorkerBase::Done(const int /*exitCode*/, const bool /*reportShutdown*/)
{
	dbgInfo.reset();
}

const char* WorkerBase::GetCurrentThreadLabel() const
{
	const auto dwId = GetCurrentThreadId();
	if (dwId == gdwMainThreadId)
		return "MainThread";
	return " <unknown thread>";
}

int WorkerBase::ProcessCommandLineArgs()
{
	LogFunction(L"ParseCommandLine{in-progress-base}");

	xf_check();

	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	int iRc = 0;

	if (gpConsoleArgs->isLogging_.exists)
	{
		CreateLogSizeFile(0);
	}

	if (!gpConsoleArgs->unknownSwitch_.IsEmpty())
	{
		if (gpConsoleArgs->command_.IsEmpty())
		{
			Help();
			_printf("\n\nParsing command line failed (/C argument not found):\n");
			_wprintf(gpConsoleArgs->fullCmdLine_.c_str(L""));

		}
		else
		{
			_printf("Parsing command line failed:\n");
			_wprintf(gpConsoleArgs->fullCmdLine_.c_str(L""));
		}
		_printf("\nUnknown parameter:\n");
		_wprintf(gpConsoleArgs->unknownSwitch_.GetStr());
		_printf("\n");
		return CERR_CMDLINEEMPTY;
	}

	if (gpConsoleArgs->needCdToProfileDir_)
	{
		CdToProfileDir();
	}

	if (gpConsoleArgs->parentFarPid_.exists)
	{
		SetParentFarPid(static_cast<DWORD>(gpConsoleArgs->parentFarPid_.GetInt()));
	}

	if (gState.attachMode_ & am_Auto)
	{
		if ((iRc = ParamAutoAttach()) != 0)
			return iRc;
	}

	if (gpConsoleArgs->attachGuiAppWnd_.exists)
	{
		if ((iRc = ParamAttachGuiApp()) != 0)
			return iRc;
	}

	if (gpConsoleArgs->rootPid_.exists)
	{
		if ((iRc = ParamAlienAttachProcess()) != 0)
			return iRc;
	}

	if (gpConsoleArgs->consoleColorIndexes_.exists)
	{
		if ((iRc = ParamColorIndexes()) != 0)
			return iRc;
	}

	if (gpConsoleArgs->conemuPid_.exists)
	{
		if ((iRc = ParamConEmuGuiPid()) != 0)
			return iRc;
	}

	if (gpConsoleArgs->requestNewGuiWnd_.exists)
	{
		if ((iRc = ParamConEmuGuiWnd()))
			return iRc;
	}

	if (gpConsoleArgs->debugPidList_.exists)
	{
		if ((iRc = ParamDebugPid()) != 0)
			return iRc;
	}

	if (gpConsoleArgs->debugExe_.exists || gpConsoleArgs->debugTree_.exists)
	{
		if ((iRc = ParamDebugExeOrTree()) != 0)
			return iRc;
	}

	if (gpConsoleArgs->debugDump_.exists)
	{
		if ((iRc = ParamDebugDump()) != 0)
			return iRc;
	}

	return 0;
}

int WorkerBase::PostProcessParams()
{
	int iRc = 0;

	xf_check();

	if ((gState.attachMode_ & am_DefTerm) && !gbParmVisibleSize)
	{
		// To avoid "small" and trimmed text after starting console
		_ASSERTE(gcrVisibleSize.X==80 && gcrVisibleSize.Y==25);
		gbParmVisibleSize = true;
	}

	if (gState.attachMode_)
	{
		if ((iRc = PostProcessCanAttach()) != 0)
			return iRc;
	}

	// Debugger or minidump requested?
	// Switches ‘/DEBUGPID=PID1[,PID2[...]]’ to debug already running process
	// or ‘/DEBUGEXE <your command line>’ or ‘/DEBUGTREE <your command line>’
	// to start new process and debug it (and its children if ‘/DEBUGTREE’)
	if (gpWorker->IsDebugProcess())
	{
		_ASSERTE(gState.runMode_ == RunMode::Undefined);
		// Run debugger thread and wait for its completion
		iRc = gpWorker->DbgInfo().RunDebugger();
		return iRc;
	}

	if ((iRc = PostProcessPrepareCommandLine()) != 0)
	{
		return iRc;
	}

	InitConsoleInputMode();

	return 0;
}

CProcessEnvCmd* WorkerBase::EnvCmdProcessor()
{
	if (!pSetEnv_)
		pSetEnv_ = std::make_shared<CProcessEnvCmd>();
	return pSetEnv_.get();
}

int WorkerBase::PostProcessPrepareCommandLine()
{
	int iRc = 0;

	// Validate Сonsole (find it may be) or ChildGui process we need to attach into ConEmu window
	if (((gState.runMode_ == RunMode::Server) || (gState.runMode_ == RunMode::AltServer)  || (gState.runMode_ == RunMode::AutoAttach))
		&& (gState.noCreateProcess_ && gState.attachMode_))
	{
		if (gState.runMode_ != RunMode::AltServer)
		{
			// Check running processes in our console and select one to be "root"
			const int nChk = CheckAttachProcess();

			if (nChk != 0)
				return nChk;
		}

		SafeFree(gpszRunCmd);
		gpszRunCmd = lstrdup(L"");

		if (!gpszRunCmd)
		{
			_printf("Can't allocate 1 wchar!\n");
			return CERR_NOTENOUGHMEM1;
		}

		return 0;
	}

	xf_check();

	if (gState.runMode_ == RunMode::Undefined)
	{
		LogString(L"CERR_CARGUMENT: Parsing command line failed (/C argument not found)");
		_printf("Parsing command line failed (/C argument not found):\n");
		_wprintf(gpConsoleArgs->fullCmdLine_.c_str(L""));
		_printf("\n");
		_ASSERTE(FALSE);
		return CERR_CARGUMENT;
	}

	xf_check();

	// Prepare our environment and GUI window

	if ((iRc = CheckGuiVersion()) != 0)
		return iRc;

	xf_check();

	LPCWSTR lsCmdLine = gpConsoleArgs->command_.c_str(L"");

	if (gState.runMode_ == RunMode::Server)
	{
		LogFunction(L"ProcessSetEnvCmd {set, title, chcp, etc.}");
		// Console may be started as follows:
		// "set PATH=C:\Program Files;%PATH%" & ... & cmd
		// Supported commands:
		//  set abc=val
		//  "set PATH=C:\Program Files;%PATH%"
		//  chcp [utf8|ansi|oem|<cp_no>]
		//  title "Console init title"
		auto* pSetEnv = EnvCmdProcessor();
		CStartEnvTitle setTitleVar(&gpszForcedTitle);
		ProcessSetEnvCmd(lsCmdLine, pSetEnv, &setTitleVar);
	}

	if (lsCmdLine && (lsCmdLine[0] == TaskBracketLeft) && wcschr(lsCmdLine, TaskBracketRight))
	{
		LogFunction(L"ExpandTaskCmd {bash} -> wsl.exe");
		// Allow smth like: ConEmuC -c {Far} /e text.txt
		// _ASSERTE(FALSE && "Continue to ExpandTaskCmd");
		gpConsoleArgs->taskCommand_ = ExpandTaskCmd(lsCmdLine);
		if (!gpConsoleArgs->taskCommand_.IsEmpty())
		{
			lsCmdLine = gpConsoleArgs->taskCommand_.c_str();

			LogFunction(L"ProcessSetEnvCmd {by task}");
			auto* pSetEnv = EnvCmdProcessor();
			CStartEnvTitle setTitleVar(&gpszForcedTitle);
			ProcessSetEnvCmd(lsCmdLine, pSetEnv, &setTitleVar);
		}
	}

	if (gState.runMode_ == RunMode::Comspec)
	{
		// New console was requested?
		if (IsNewConsoleArg(lsCmdLine))
		{
			auto* comspec = dynamic_cast<WorkerComspec*>(gpWorker);
			if (!comspec)
			{
				_ASSERTE(comspec != nullptr);
			}
			else
			{
				const auto iNewConRc = comspec->ProcessNewConsoleArg(lsCmdLine);
				if (iNewConRc != 0)
				{
					return iNewConRc;
				}
			}
		}
	}

	LPCWSTR pszArguments4EnvVar = nullptr;
	CEStr szExeTest;

	if (gState.runMode_ == RunMode::Comspec && (!lsCmdLine || !*lsCmdLine))
	{
		if (gpWorker->IsCmdK())
		{
			gState.runViaCmdExe_ = true;
		}
		else
		{
			// In Far Manager user may set up an empty association for file mask (execution)
			// e.g: *.ini -> "@"
			// in that case it looks like Far does not do anything, but it calls ComSpec
			gbNonGuiMode = TRUE;
			gState.DisableAutoConfirmExit();
			return CERR_EMPTY_COMSPEC_CMDLINE;
		}
	}
	else
	{
		NeedCmdOptions opt{};
		opt.alwaysConfirmExit = gState.alwaysConfirmExit_;

		pszCheck4NeedCmd_ = lsCmdLine;

		gState.runViaCmdExe_ = IsNeedCmd((gState.runMode_ == RunMode::Server), lsCmdLine, szExeTest, &opt);

		pszArguments4EnvVar = opt.arguments;
		gState.needCutStartEndQuot_ = opt.needCutStartEndQuot;
		gState.rootIsCmdExe_ = opt.rootIsCmdExe;

		if (gpConsoleArgs->confirmExitParm_ == RConStartArgs::eConfDefault)
		{
			gState.alwaysConfirmExit_ = opt.alwaysConfirmExit;
		}
	}


	#ifndef WIN64
	// Команды вида: C:\Windows\SysNative\reg.exe Query "HKCU\Software\Far2"|find "Far"
	// Для них нельзя отключать редиректор (wow.Disable()), иначе SysNative будет недоступен
	CheckNeedSkipWowChange(lsCmdLine);
	#endif

	int nCmdLine = lstrlenW(lsCmdLine);

	if (!gState.runViaCmdExe_)
	{
		nCmdLine += 1; // '\0' termination
		if (pszArguments4EnvVar && *szExeTest)
			nCmdLine += lstrlen(szExeTest) + 3;
	}
	else
	{
		gszComSpec[0] = 0;
		// ReSharper disable once CppLocalVariableMayBeConst
		LPCWSTR pszFind = GetComspecFromEnvVar(gszComSpec, countof(gszComSpec));
		if (!pszFind || !wcschr(pszFind, L'\\') || !FileExists(pszFind))
		{
			_ASSERTE("cmd.exe not found!");
			_printf("Can't find cmd.exe!\n");
			return CERR_CMDEXENOTFOUND;
		}

		nCmdLine += lstrlenW(gszComSpec) + 15; // "/C" + quotation + possible "/U"
	}

	// nCmdLine counts length of lsCmdLine + gszComSpec + something for "/C" and things
	const size_t nCchLen = size_t(nCmdLine) + 1;
	SafeFree(gpszRunCmd);
	gpszRunCmd = static_cast<wchar_t*>(calloc(nCchLen, sizeof(wchar_t)));

	if (!gpszRunCmd)
	{
		_printf("Can't allocate %i wchars!\n", (DWORD)nCmdLine);
		return CERR_NOTENOUGHMEM1;
	}

	// это нужно для смены заголовка консоли. при необходимости COMSPEC впишем ниже, после смены
	if (pszArguments4EnvVar && *szExeTest && !gState.runViaCmdExe_)
	{
		gpszRunCmd[0] = L'"';
		_wcscat_c(gpszRunCmd, nCchLen, szExeTest);
		if (*pszArguments4EnvVar)
		{
			_wcscat_c(gpszRunCmd, nCchLen, L"\" ");
			_wcscat_c(gpszRunCmd, nCchLen, pszArguments4EnvVar);
		}
		else
		{
			_wcscat_c(gpszRunCmd, nCchLen, L"\"");
		}
	}
	else
	{
		_wcscpy_c(gpszRunCmd, nCchLen, lsCmdLine);
	}
	// !!! gpszRunCmd может поменяться ниже!

	// ====
	if (gState.runViaCmdExe_)
	{
		// -- always quote
		gpszRunCmd[0] = L'"';
		_wcscpy_c(gpszRunCmd+1, nCchLen-1, gszComSpec);

		_wcscat_c(gpszRunCmd, nCchLen, gpWorker->IsCmdK() ? L"\" /K " : L"\" /C ");

		// The command line itself
		_wcscat_c(gpszRunCmd, nCchLen, lsCmdLine);
	}
	else if (gState.needCutStartEndQuot_)
	{
		// ""c:\arc\7z.exe -?"" - would not start!
		_wcscpy_c(gpszRunCmd, nCchLen, lsCmdLine+1);
		wchar_t *pszEndQ = gpszRunCmd + lstrlenW(gpszRunCmd) - 1;
		_ASSERTE(pszEndQ && *pszEndQ == L'"');

		if (pszEndQ && *pszEndQ == L'"') *pszEndQ = 0;
	}

	// Cut and process the "-new_console" / "-cur_console"
	SafeFree(args_.pszSpecialCmd);
	args_.pszSpecialCmd = gpszRunCmd;
	args_.ProcessNewConArg();
	args_.pszSpecialCmd = nullptr; // this point to global gpszRunCmd
	// Если указана рабочая папка
	if (args_.pszStartupDir && *args_.pszStartupDir)
	{
		SetCurrentDirectory(args_.pszStartupDir);
	}
	//
	gbRunInBackgroundTab = (args_.BackgroundTab == crb_On);
	if (args_.BufHeight == crb_On)
	{
		TODO("gcrBufferSize - и ширину буфера");
		gnBufferHeight = args_.nBufHeight;
		gbParmBufSize = TRUE;
	}
	// DosBox?
	if (args_.ForceDosBox == crb_On)
	{
		gState.dosbox_.use_ = TRUE;
	}

	#ifdef _DEBUG
	OutputDebugString(gpszRunCmd); OutputDebugString(L"\n");
	#endif

	return 0;
}

int WorkerBase::CheckGuiVersion()
{
	_ASSERTE(gState.runMode_ != RunMode::Server);
	return 0;
}

int WorkerBase::PostProcessCanAttach() const
{
	// Параметры из комстроки разобраны. Здесь могут уже быть известны
	// gState.hGuiWnd {/GHWND}, gState.conemuPid_ {/GPID}, gState.dwGuiAID {/AID}
	// gpStatus->attachMode_ для ключей {/ADMIN}, {/ATTACH}, {/AUTOATTACH}, {/GUIATTACH}

	// В принципе, gpStatus->attachMode_ включается и при "/ADMIN", но при запуске из ConEmu такого быть не может,
	// будут установлены и gState.hGuiWnd, и gState.conemuPid_

	// Issue 364, например, идет билд в VS, запускается CustomStep, в этот момент автоаттач нафиг не нужен
	// Теоретически, в Студии не должно бы быть запуска ConEmuC.exe, но он может оказаться в "COMSPEC", так что проверим.
	if (gState.attachMode_
		&& ((gState.runMode_ == RunMode::Server) || (gState.runMode_ == RunMode::AutoAttach))
		&& (gState.conemuPid_ == 0))
	{
		_ASSERTE(FALSE && "Continue to attach!");

		_ASSERTE(!gpConsoleArgs->debugPidList_.exists);

		BOOL lbIsWindowVisible = FALSE;
		if (!gState.realConWnd_
			|| !((lbIsWindowVisible = gpConsoleArgs->IsAutoAttachAllowed()))
			|| isTerminalMode() /*telnet*/)
		{
			if (gpLogSize)
			{
				if (!gState.realConWnd_) { LogFunction(L"!gState.realConWnd"); }
				else if (!lbIsWindowVisible) { LogFunction(L"!IsAutoAttachAllowed"); }
				else { LogFunction(L"isTerminalMode"); }
			}
			// Но это может быть все-таки наше окошко. Как проверить...
			// Найдем первый параметр
			// #Server suspicious: previously used in 200713 lsCmdLine should contain empty string on attach
			const auto* lsCmdLine = gpConsoleArgs->command_.c_str();
			LPCWSTR pszSlash = lsCmdLine ? wcschr(lsCmdLine, L'/') : nullptr;
			if (pszSlash)
			{
				LogFunction(pszSlash);
				// И сравним с используемыми у нас. Возможно потом еще что-то добавить придется
				if (wmemcmp(pszSlash, L"/DEBUGPID=", 10) != 0)
					pszSlash = nullptr;
			}
			if (pszSlash == nullptr)
			{
				// Не наше окошко, выходим
				gbInShutdown = TRUE;
				return CERR_ATTACH_NO_CONWND;
			}
		}

		if (!gpConsoleArgs->alternativeAttach_ && !(gState.attachMode_ & am_DefTerm) && !gpWorker->RootProcessId())
		{
			// В принципе, сюда мы можем попасть при запуске, например: "ConEmuC.exe /ADMIN /ROOT cmd"
			// Но только не при запуске "из ConEmu" (т.к. будут установлены gState.hGuiWnd, gState.conemuPid_)

			// Из батника убрал, покажем инфу тут
			PrintVersion();
			char szAutoRunMsg[128];
			sprintf_c(szAutoRunMsg, "Starting attach autorun (NewWnd=%s)\n",
				gpConsoleArgs->requestNewGuiWnd_ ? "YES" : "NO");
			_printf(szAutoRunMsg);
		}
	}

	return 0;
}

int WorkerBase::ParamDebugDump()
{
	if (gpConsoleArgs->debugMiniDump_.GetBool())
		SetDebugDumpType(DumpProcessType::MiniDump);
	else if (gpConsoleArgs->debugFullDump_.GetBool())
		SetDebugDumpType(DumpProcessType::FullDump);
	else if (gpConsoleArgs->debugAutoMini_.exists)
		SetDebugAutoDump(gpConsoleArgs->debugAutoMini_.GetStr());
	else // if just "/DUMP"
		SetDebugDumpType(DumpProcessType::AskUser);

	return 0;
}

int WorkerBase::ParamDebugExeOrTree()
{
	const bool debugTree = gpConsoleArgs->debugTree_.GetBool();
	const auto dbgRc = SetDebuggingExe(gpConsoleArgs->command_, debugTree);
	if (dbgRc != 0)
		return dbgRc;
	return 0;
}

int WorkerBase::ParamDebugPid()
{
	const auto dbgRc = SetDebuggingPid(gpConsoleArgs->debugPidList_.GetStr());
	if (dbgRc != 0)
		return dbgRc;
	return 0;
}

int WorkerBase::ParamConEmuGuiWnd() const
{
	_ASSERTE(gState.runMode_ == RunMode::AutoAttach
		|| gState.runMode_ == RunMode::Server || gState.runMode_ == RunMode::AltServer);

	if (gpConsoleArgs->requestNewGuiWnd_.GetBool())
	{
		gState.hGuiWnd = nullptr;
		_ASSERTE(gState.conemuPid_ == 0);
		gState.conemuPid_ = 0;
	}
	else
	{
		const HWND2 hGuiWnd{ LODWORD(gpConsoleArgs->conemuHwnd_.GetInt()) };
		const bool isWnd = hGuiWnd ? IsWindow(hGuiWnd) : FALSE;
		const DWORD nErr = hGuiWnd ? GetLastError() : 0;

		wchar_t szLog[120];
		swprintf_c(
			szLog, L"GUI HWND=0x%08X, %s, ErrCode=%u",
			LODWORD(hGuiWnd), isWnd ? L"Valid" : L"Invalid", nErr);
		LogString(szLog);

		if (!isWnd)
		{
			LogString(L"CERR_CARGUMENT: Invalid GUI HWND was specified in /GHWND arg");
			_printf("Invalid GUI HWND specified: /GHWND");
			_printf("\n" "Command line:\n");
			_wprintf(gpConsoleArgs->fullCmdLine_);
			_printf("\n");
			_ASSERTE(FALSE && "Invalid window was specified in /GHWND arg");
			return CERR_CARGUMENT;
		}

		DWORD nPid = 0;
		GetWindowThreadProcessId(hGuiWnd, &nPid);
		_ASSERTE(gState.conemuPid_ == 0 || gState.conemuPid_ == nPid);
		gState.conemuPid_ = nPid;
		gState.hGuiWnd = hGuiWnd;
	}

	return 0;
}

int WorkerBase::ParamConEmuGuiPid() const
{
	_ASSERTE(gState.runMode_ == RunMode::Server || gState.runMode_ == RunMode::AltServer);

	gState.conemuPid_ = LODWORD(gpConsoleArgs->conemuPid_.GetInt());

	if (gState.conemuPid_ == 0)
	{
		LogString(L"CERR_CARGUMENT: Invalid GUI PID specified");
		_printf("Invalid GUI PID specified:\n");
		_wprintf(gpConsoleArgs->fullCmdLine_);
		_printf("\n");
		_ASSERTE(FALSE);
		return CERR_CARGUMENT;
	}

	return 0;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
int WorkerBase::ParamColorIndexes() const
{
	const DWORD nColors = LODWORD(gpConsoleArgs->consoleColorIndexes_.GetInt());

	if (nColors)
	{
		const DWORD nTextIdx = (nColors & 0xFF);
		const DWORD nBackIdx = ((nColors >> 8) & 0xFF);
		const DWORD nPopTextIdx = ((nColors >> 16) & 0xFF);
		const DWORD nPopBackIdx = ((nColors >> 24) & 0xFF);

		if ((nTextIdx <= 15) && (nBackIdx <= 15) && (nTextIdx != nBackIdx))
			gnDefTextColors = MAKECONCOLOR(nTextIdx, nBackIdx);

		if ((nPopTextIdx <= 15) && (nPopBackIdx <= 15) && (nPopTextIdx != nPopBackIdx))
			gnDefPopupColors = MAKECONCOLOR(nPopTextIdx, nPopBackIdx);

		// ReSharper disable once CppLocalVariableMayBeConst
		HANDLE hConOut = ghConOut;
		CONSOLE_SCREEN_BUFFER_INFO csbi5 = {};
		GetConsoleScreenBufferInfo(hConOut, &csbi5);

		if (gnDefTextColors || gnDefPopupColors)
		{
			BOOL bPassed = FALSE;

			if (gnDefPopupColors && IsWin6())
			{
				MY_CONSOLE_SCREEN_BUFFER_INFOEX csbi = {};
				csbi.cbSize = sizeof(csbi);
				if (apiGetConsoleScreenBufferInfoEx(hConOut, &csbi))
				{
					// Microsoft bug? When console is started elevated - it does NOT show
					// required attributes, BUT GetConsoleScreenBufferInfoEx returns them.
					if (!(gState.attachMode_ & am_Admin)
						&& (!gnDefTextColors || ((csbi.wAttributes = gnDefTextColors)))
						&& (!gnDefPopupColors || ((csbi.wPopupAttributes = gnDefPopupColors))))
					{
						bPassed = TRUE; // nothing to change, console matches
					}
					else
					{
						if (gnDefTextColors)
							csbi.wAttributes = gnDefTextColors;
						if (gnDefPopupColors)
							csbi.wPopupAttributes = gnDefPopupColors;

						_ASSERTE(FALSE && "Continue to SetConsoleScreenBufferInfoEx");

						// Vista/Win7. _SetConsoleScreenBufferInfoEx unexpectedly SHOWS console window
						//if (gnOsVer == 0x0601)
						//{
						//	RECT rcGui = {};
						//	if (gState.hGuiWnd)
						//		GetWindowRect(gState.hGuiWnd, &rcGui);
						//	//SetWindowPos(gState.realConWnd, HWND_BOTTOM, rcGui.left+3, rcGui.top+3, 0,0, SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOZORDER);
						//	SetWindowPos(gState.realConWnd, nullptr, -30000, -30000, 0,0, SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOZORDER);
						//	apiShowWindow(gState.realConWnd, SW_SHOWMINNOACTIVE);
						//	#ifdef _DEBUG
						//	apiShowWindow(gState.realConWnd, SW_SHOWNORMAL);
						//	apiShowWindow(gState.realConWnd, SW_HIDE);
						//	#endif
						//}

						bPassed = apiSetConsoleScreenBufferInfoEx(hConOut, &csbi);

						// Fix problems of Windows 7
						if (!gbVisibleOnStartup)
						{
							apiShowWindow(gState.realConWnd_, SW_HIDE);
						}
					}
				}
			}


			if (!bPassed && gnDefTextColors)
			{
				SetConsoleTextAttribute(hConOut, gnDefTextColors);
				RefillConsoleAttributes(csbi5, csbi5.wAttributes, gnDefTextColors);
			}
		}
	}

	return 0;
}

int WorkerBase::ParamAlienAttachProcess()
{
	SetRootProcessId(LODWORD(gpConsoleArgs->rootPid_.GetInt()));

	if ((RootProcessId() == 0) && gpConsoleArgs->creatingHiddenConsole_)
	{
		SetRootProcessId(Processes().WaitForRootConsoleProcess(30000));
	}

	if (gpConsoleArgs->alternativeAttach_ && RootProcessId())
	{
		// if process was started "with console window"
		if (gState.realConWnd_)
		{
			DEBUGTEST(SafeCloseHandle(ghFarInExecuteEvent));
		}

		const BOOL bAttach = StdCon::AttachParentConsole(RootProcessId());
		if (!bAttach)
		{
			gbInShutdown = TRUE;
			gState.alwaysConfirmExit_ = FALSE;
			LogString(L"CERR_CARGUMENT: (gbAlternativeAttach && RootProcessId())");
			return CERR_CARGUMENT;
		}

		// Need to be set, because of new console === new handler
		SetConsoleCtrlHandler(HandlerRoutine, true);

		_ASSERTE(ghFarInExecuteEvent == nullptr);
		_ASSERTE(gState.realConWnd_ != nullptr);
	}
	else if (RootProcessId() == 0)
	{
		LogString("CERR_CARGUMENT: Attach to GUI was requested, but invalid PID specified");
		_printf("Attach to GUI was requested, but invalid PID specified:\n");
		_wprintf(gpConsoleArgs->fullCmdLine_);
		_printf("\n");
		_ASSERTE(FALSE && "Attach to GUI was requested, but invalid PID specified");
		return CERR_CARGUMENT;
	}

	return 0;
}

int WorkerBase::ParamAttachGuiApp()
{
	if (!gpConsoleArgs->attachGuiAppWnd_.IsValid())
	{
		LogString(L"CERR_CARGUMENT: Invalid Child HWND was specified in /GuiAttach arg");
		_printf("Invalid Child HWND specified: /GuiAttach");
		_printf("\n" "Command line:\n");
		_wprintf(gpConsoleArgs->fullCmdLine_);
		_printf("\n");
		_ASSERTE(FALSE && "Invalid window was specified in /GuiAttach arg");
		return CERR_CARGUMENT;
	}

	const HWND2 hAppWnd{ LODWORD(gpConsoleArgs->attachGuiAppWnd_.value) };
	if (IsWindow(hAppWnd))
		SetRootProcessGui(hAppWnd);

	return 0;
}

int WorkerBase::ParamAutoAttach() const
{
	//ConEmu autorun (c) Maximus5
	//Starting "%ConEmuPath%" in "Attach" mode (NewWnd=%FORCE_NEW_WND%)

	if (!gpConsoleArgs->IsAutoAttachAllowed())
	{
		if (gState.realConWnd_ && IsWindowVisible(gState.realConWnd_))
		{
			_printf("AutoAttach was requested, but skipped\n");
		}
		gState.DisableAutoConfirmExit();
		//_ASSERTE(FALSE && "AutoAttach was called while Update process is in progress?");
		return CERR_AUTOATTACH_NOT_ALLOWED;
	}

	return 0;
}

bool WorkerBase::IsCmdK() const
{
	_ASSERTE(gpConsoleArgs->cmdK_.GetBool() == false);
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

ConProcess& WorkerBase::Processes()
{
	if (!processes_)
	{
		_ASSERTE(FALSE && "processes_ should be set already");
		processes_.reset(new ConProcess(kernel32));
	}

	return *processes_;
}

const CONSOLE_SCREEN_BUFFER_INFO& WorkerBase::GetSbi() const
{
	return this->consoleInfo.sbi;
}

bool WorkerBase::LoadScreenBufferInfo(ScreenBufferInfo& sbi)
{
	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	const auto result = GetConsoleScreenBufferInfo(hOut, &sbi.csbi);
	sbi.crMaxSize = MyGetLargestConsoleWindowSize(hOut);
	return result;
}

MSectionLockSimple WorkerBase::LockConsoleReaders(DWORD anWaitTimeout)
{
	// Nothing to lock in base
	return MSectionLockSimple{};
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

	gState.noCreateProcess_ = TRUE;
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

	gState.noCreateProcess_ = TRUE;
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

bool WorkerBase::IsKeyboardLayoutChanged(DWORD& pdwLayout, LPDWORD pdwErrCode /*= nullptr*/)
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

const MModule& WorkerBase::KernelModule() const
{
	return kernel32;
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

			CESERVER_REQ* pOut = ExecuteGuiCmd(gState.realConWnd_, pIn, gState.realConWnd_);

			ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
	}
}

bool WorkerBase::CheckHwFullScreen()
{
	bool bFullScreenHw = false;

	// #Server this should be wrap in the method. and there is a note of a bug in Windows 10 b9879
	if (wasFullscreenMode_ && pfnGetConsoleDisplayMode)
	{
		DWORD nModeFlags = 0;
		pfnGetConsoleDisplayMode(&nModeFlags);
		// The bug of Windows 10 b9879
		if (IsWin10Build9879())
		{
			nModeFlags = 0;
		}

		gState.consoleDisplayMode_ = nModeFlags;
		if (nModeFlags & CONSOLE_FULLSCREEN_HARDWARE)
		{
			bFullScreenHw = true;
		}
		else
		{
			wasFullscreenMode_ = false;
		}
	}

	return bFullScreenHw;
}

bool WorkerBase::EnterHwFullScreen(PCOORD pNewSize /*= nullptr*/)
{
	if (!pfnSetConsoleDisplayMode)
	{
		if (!kernel32.GetProcAddress("SetConsoleDisplayMode", pfnSetConsoleDisplayMode))
		{
			SetLastError(ERROR_INVALID_FUNCTION);
			return false;
		}
	}

	COORD crNewSize = {};
	const bool result = pfnSetConsoleDisplayMode(ghConOut, 1/*CONSOLE_FULLSCREEN_MODE*/, &crNewSize);

	if (result)
	{
		wasFullscreenMode_ = true;
		if (pNewSize)
			*pNewSize = crNewSize;
	}

	return result;
}

// Called from SetConsoleSize for debugging purposes
void WorkerBase::PreConsoleSize(const int width, const int height)
{
	// Is visible area size too big?
	_ASSERTE(width <= 500);
	_ASSERTE(height <= 300);
	// And positive
	_ASSERTE(width > 0);
	_ASSERTE(height > 0);
}

// Called from SetConsoleSize for debugging purposes
void WorkerBase::PreConsoleSize(const COORD crSize)
{
	PreConsoleSize(crSize.X, crSize.Y);
}

bool WorkerBase::ApplyConsoleSizeSimple(
	const COORD& crNewSize, const CONSOLE_SCREEN_BUFFER_INFO& csbi, DWORD& dwErr, bool bForceWriteLog)
{
	bool lbRc = true;
	dwErr = 0;

	DEBUGSTRSIZE(L"SetConsoleSize: ApplyConsoleSizeSimple started");

	const bool lbNeedChange = (csbi.dwSize.X != crNewSize.X) || (csbi.dwSize.Y != crNewSize.Y)
		|| ((csbi.srWindow.Right - csbi.srWindow.Left + 1) != crNewSize.X)
		|| ((csbi.srWindow.Bottom - csbi.srWindow.Top + 1) != crNewSize.Y);

	RECT rcConPos = {};
	GetWindowRect(gState.realConWnd_, &rcConPos);

	SMALL_RECT rNewRect = {};

	#ifdef _DEBUG
	if (!lbNeedChange)
	{
		int nDbg = 0;
	}
	#endif

	if (lbNeedChange)
	{
		// Если этого не сделать - размер консоли нельзя УМЕНЬШИТЬ
		if (crNewSize.X <= (csbi.srWindow.Right-csbi.srWindow.Left) || crNewSize.Y <= (csbi.srWindow.Bottom-csbi.srWindow.Top))
		{
			rNewRect.Left = 0; rNewRect.Top = 0;
			rNewRect.Right = std::min<int>((crNewSize.X - 1), (csbi.srWindow.Right - csbi.srWindow.Left));
			rNewRect.Bottom = std::min<int>((crNewSize.Y - 1), (csbi.srWindow.Bottom - csbi.srWindow.Top));

			if (!SetConsoleWindowInfo(ghConOut, TRUE, &rNewRect))
			{
				// Last chance to shrink visible area of the console if ConApi was failed
				MoveWindow(gState.realConWnd_, rcConPos.left, rcConPos.top, 1, 1, 1);
			}
		}

		//specified width and height cannot be less than the width and height of the console screen buffer's window
		if (!SetConsoleScreenBufferSize(ghConOut, crNewSize))
		{
			lbRc = false;
			dwErr = GetLastError();
		}

		// #TODO: а если правый нижний край вылезет за пределы экрана?
		rNewRect.Left = 0; rNewRect.Top = 0;
		rNewRect.Right = crNewSize.X - 1;
		rNewRect.Bottom = crNewSize.Y - 1;
		if (!SetConsoleWindowInfo(ghConOut, TRUE, &rNewRect))
		{
			// Non-critical error?
			dwErr = GetLastError();
		}
	}

	LogSize(nullptr, 0, lbRc ? "ApplyConsoleSizeSimple OK" : "ApplyConsoleSizeSimple FAIL", bForceWriteLog);

	return lbRc;
}

bool WorkerBase::ApplyConsoleSizeBuffer(
	const USHORT BufferHeight, const COORD& crNewSize, const CONSOLE_SCREEN_BUFFER_INFO& csbi, DWORD& dwErr,
	bool bForceWriteLog)
{
	PreConsoleSize(crNewSize);
	_ASSERTE(FALSE && "ApplyConsoleSizeBuffer is not implemented in base");
	return false;
}

void WorkerBase::RefillConsoleAttributes(const CONSOLE_SCREEN_BUFFER_INFO& csbi5, const WORD wOldText, const WORD wNewText) const
{
	_ASSERTE(FALSE && "RefillConsoleAttributes is not implemented in base");
}

bool WorkerBase::SetConsoleSize(USHORT BufferHeight, COORD crNewSize, SMALL_RECT rNewRect, LPCSTR asLabel /*= nullptr*/, bool bForceWriteLog /*= false*/)
{
	PreConsoleSize(crNewSize);
	_ASSERTE(FALSE && "SetConsoleSize is not implemented in base");
	return false;
}

void WorkerBase::CdToProfileDir() const
{
	BOOL bRc = FALSE;
	wchar_t szPath[MAX_PATH] = L"";
	HRESULT hr = SHGetFolderPath(nullptr, CSIDL_PROFILE, nullptr, 0, szPath);
	if (FAILED(hr))
		GetEnvironmentVariable(L"USERPROFILE", szPath, countof(szPath));
	if (szPath[0])
		bRc = SetCurrentDirectory(szPath);
	// Write action to log file
	if (gpLogSize)
	{
		wchar_t* pszMsg = lstrmerge(bRc ? L"Work dir changed to %USERPROFILE%: " : L"CD failed to %USERPROFILE%: ", szPath);
		LogFunction(pszMsg);
		SafeFree(pszMsg);
	}
}

void WorkerBase::InitConsoleInputMode() const
{
	LogFunction(L"InitConsoleInputMode");

	BOOL bConRc = FALSE;
	wchar_t szErr[128];

	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	DWORD dwOldFlags = 0;
	bConRc = GetConsoleMode(h, &dwOldFlags);
	if (!bConRc)
	{
		swprintf_c(szErr, countof(szErr),
			L"ERROR: GetConsoleMode failed, handle=%p, error=%u",
			h, GetLastError());
		LogString(szErr);
	}

	LogModeChange(L"[GetConInMode]", 0, dwOldFlags);

	DWORD dwFlags = dwOldFlags;

	// For ComSpec mode it is just 0
	auto nConsoleModeFlags = static_cast<DWORD>(gpConsoleArgs->consoleModeFlags_.GetInt());

	// Overwrite mode in Prompt? This has priority.
	if (args_.OverwriteMode != crb_Undefined)
	{
		nConsoleModeFlags |= (ENABLE_INSERT_MODE << 16); // Mask
		if (args_.OverwriteMode == crb_On)
			nConsoleModeFlags &= ~ENABLE_INSERT_MODE; // Turn bit OFF
		else if (args_.OverwriteMode == crb_Off)
			nConsoleModeFlags |= ENABLE_INSERT_MODE; // Turn bit ON
	}

	// This can be passed with "/CINMODE=..."
	// oWerwrite mode could be enabled with "-cur_console:w" switch (processed in GUI)
	if (!nConsoleModeFlags)
	{
		// Use defaults (switch /CINMODE= was not specified)
		// DON'T turn on ENABLE_QUICK_EDIT_MODE by default, let console applications "use" mouse
		dwFlags |= (ENABLE_EXTENDED_FLAGS | ENABLE_INSERT_MODE);
	}
	else
	{
		const DWORD nMask = (nConsoleModeFlags & 0xFFFF0000) >> 16;
		const DWORD nOr   = (nConsoleModeFlags & 0xFFFF);
		dwFlags &= ~nMask;
		dwFlags |= (nOr | ENABLE_EXTENDED_FLAGS);
	}

	bConRc = SetConsoleMode(h, dwFlags); //-V519
	LogModeChange(L"[SetConInMode]", dwOldFlags, dwFlags);

	if (!bConRc)
	{
		swprintf_c(szErr, countof(szErr),
			L"ERROR: SetConsoleMode failed, handle=%p, flags=0x%X, error=%u",
			h, dwFlags, GetLastError());
		LogString(szErr);
	}
}

int WorkerBase::CheckAttachProcess()
{
	LogFunction(L"CheckAttachProcess");

	int liArgsFailed = 0;
	wchar_t szFailMsg[512]; szFailMsg[0] = 0;
	BOOL lbRootExists = FALSE;
	wchar_t szProc[255] = {}, szTmp[10] = {};
	DWORD nFindId = 0;

	if (this->RootProcessGui())
	{
		if (!IsWindow(this->RootProcessGui()))
		{
			swprintf_c(szFailMsg, L"Attach of GUI application was requested,\n"
				L"but required HWND(0x%08X) not found!", LODWORD(this->RootProcessGui()));
			LogString(szFailMsg);
			liArgsFailed = 1;
			// will return CERR_CARGUMENT
		}
		else
		{
			DWORD nPid = 0; GetWindowThreadProcessId(this->RootProcessGui(), &nPid);
			if (this->RootProcessId() && nPid && (this->RootProcessId() != nPid))
			{
				swprintf_c(szFailMsg, L"Attach of GUI application was requested,\n"
					L"but PID(%u) of HWND(0x%08X) does not match Root(%u)!",
					nPid, LODWORD(this->RootProcessGui()), this->RootProcessId());
				LogString(szFailMsg);
				liArgsFailed = 2;
				// will return CERR_CARGUMENT
			}
		}
	}
	else if (!this->Processes().IsConsoleProcessCountSupported())
	{
		wcscpy_c(szFailMsg, L"Attach to console app was requested, but required WinXP or higher!");
		LogString(szFailMsg);
		liArgsFailed = 3;
		// will return CERR_CARGUMENT
	}
	else
	{
		auto processes = this->Processes().GetAllProcesses();

		if ((processes.size() == 1) && gpConsoleArgs->creatingHiddenConsole_)
		{
			const DWORD nStart = GetTickCount();
			const DWORD nMaxDelta = 30000;
			DWORD nDelta = 0;
			while (nDelta < nMaxDelta)
			{
				Sleep(100);
				processes = this->Processes().GetAllProcesses();
				if (processes.size() > 1)
					break;
				nDelta = (GetTickCount() - nStart);
			}
		}

		// 2 процесса, потому что это мы сами и минимум еще один процесс в этой консоли,
		// иначе смысла в аттаче нет
		if (processes.size() < 2)
		{
			wcscpy_c(szFailMsg, L"Attach to console app was requested, but there is no console processes!");
			LogString(szFailMsg);
			liArgsFailed = 4;
			//will return CERR_CARGUMENT
		}
		// не помню, зачем такая проверка была введена, но (nProcCount > 2) мешает аттачу.
		// в момент запуска сервера (/ATTACH /PID=n) еще жив родительский (/ATTACH /NOCMD)
		//// Если cmd.exe запущен из cmd.exe (в консоли уже больше двух процессов) - ничего не делать
		else if ((this->RootProcessId() != 0) || (processes.size() > 2))
		{
			lbRootExists = (this->RootProcessId() == 0);
			// И ругаться только под отладчиком
			nFindId = 0;

			for (int n = (static_cast<int>(processes.size()) - 1); n >= 0; n--)
			{
				if (szProc[0]) wcscat_c(szProc, L", ");

				swprintf_c(szTmp, L"%i", processes[n]);
				wcscat_c(szProc, szTmp);

				if (this->RootProcessId())
				{
					if (!lbRootExists && processes[n] == this->RootProcessId())
						lbRootExists = TRUE;
				}
				else if ((nFindId == 0) && (processes[n] != gnSelfPID))
				{
					// Будем считать его корневым.
					// Собственно, кого считать корневым не важно, т.к.
					// сервер не закроется до тех пор пока жив хотя бы один процесс
					nFindId = processes[n];
				}
			}

			if ((this->RootProcessId() == 0) && (nFindId != 0))
			{
				this->SetRootProcessId(nFindId);
				lbRootExists = TRUE;
			}

			if ((this->RootProcessId() != 0) && !lbRootExists)
			{
				swprintf_c(szFailMsg, L"Attach to GUI was requested, but\n" L"root process (%u) does not exists", this->RootProcessId());
				LogString(szFailMsg);
				liArgsFailed = 5;
				//will return CERR_CARGUMENT
			}
			else if ((this->RootProcessId() == 0) && (processes.size() > 2))
			{
				swprintf_c(szFailMsg, L"Attach to GUI was requested, but\n" L"there is more than 2 console processes: %s\n", szProc);
				LogString(szFailMsg);
				liArgsFailed = 6;
				//will return CERR_CARGUMENT
			}
		}
	}

	if (liArgsFailed)
	{
		const DWORD nSelfPID = GetCurrentProcessId();
		PROCESSENTRY32 self = {}, parent = {};
		// Not optimal, needs refactoring
		if (GetProcessInfo(nSelfPID, self))
			GetProcessInfo(self.th32ParentProcessID, parent);

		LPCWSTR pszCmdLine = GetCommandLineW(); if (!pszCmdLine) pszCmdLine = L"";

		wchar_t szTitle[MAX_PATH*2];
		swprintf_c(szTitle,
			L"ConEmuC %s [%u], PID=%u, Code=%i" L"\r\n"
			L"ParentPID=%u: %s" L"\r\n"
			L"  ", // szFailMsg follows this
			gsVersion, WIN3264TEST(32,64), nSelfPID, liArgsFailed,
			self.th32ParentProcessID, parent.szExeFile[0] ? parent.szExeFile : L"<terminated>");

		CEStr lsMsg = lstrmerge(szTitle, szFailMsg, L"\r\nCommand line:\r\n  ", pszCmdLine);

		// Avoid automatic termination of ExitWaitForKey
		gbInShutdown = FALSE;

		// Force light-red on black for error message
		MSetConTextAttr setAttr(ghConOut, 12);

		const DWORD nAttachErrorTimeoutMessage = 15*1000; // 15 sec
		ExitWaitForKey(VK_RETURN|(VK_ESCAPE<<8), lsMsg, true, true, nAttachErrorTimeoutMessage);

		LogString(L"CheckAttachProcess: CERR_CARGUMENT after ExitWaitForKey");

		gbInShutdown = TRUE;
		return CERR_CARGUMENT;
	}

	return 0; // OK
}

void WorkerBase::CheckNeedSkipWowChange(LPCWSTR asCmdLine)
{
	// Only for 32-bit builds
#ifndef WIN64
	LogFunction(L"CheckNeedSkipWowChange");

	// Команды вида: C:\Windows\SysNative\reg.exe Query "HKCU\Software\Far2"|find "Far"
	// Для них нельзя отключать редиректор (wow.Disable()), иначе SysNative будет недоступен
	if (IsWindows64())
	{
		LPCWSTR pszTest = asCmdLine;
		CmdArg szApp;

		if ((pszTest = NextArg(pszTest, szApp)))
		{
			wchar_t szSysNative[MAX_PATH+32] = L"";
			int nLen = GetWindowsDirectory(szSysNative, MAX_PATH);

			if (nLen >= 2 && nLen < MAX_PATH)
			{
				AddEndSlash(szSysNative, countof(szSysNative));
				wcscat_c(szSysNative, L"Sysnative\\");
				nLen = lstrlenW(szSysNative);
				const int nAppLen = lstrlenW(szApp);

				if (nAppLen > nLen)
				{
					szApp.ms_Val[nLen] = 0;

					if (lstrcmpiW(szApp, szSysNative) == 0)
					{
						gState.bSkipWowChange_ = TRUE;
					}
				}
			}
		}
	}
#endif
}

void WorkerBase::SetConEmuWindows(HWND hRootWnd, HWND hDcWnd, HWND hBackWnd)
{
	LogFunction(L"SetConEmuWindows");

	char szInfo[100] = "";

	// Main ConEmu window
	if (hRootWnd && !IsWindow(hRootWnd))
	{
		_ASSERTE(FALSE && "hRootWnd is invalid");
		hRootWnd = nullptr;
	}
	// Changed?
	if (gState.conemuWnd_ != hRootWnd)
	{
		sprintf_c(szInfo+lstrlenA(szInfo), 30/*#SECURELEN*/, "ConEmuWnd=x%08X ", (DWORD)(DWORD_PTR)hRootWnd);
		gState.conemuWnd_ = hRootWnd;

		// Than check GuiPID
		DWORD nGuiPid = 0;
		if (!hRootWnd)
		{
			gState.conemuPid_ = 0;
		}
		else if (GetWindowThreadProcessId(hRootWnd, &nGuiPid) && nGuiPid)
		{
			gState.conemuPid_ = nGuiPid;
		}
	}
	// Do AllowSetForegroundWindow
	if (hRootWnd)
	{
		DWORD dwGuiThreadId = GetWindowThreadProcessId(hRootWnd, &gState.conemuPid_);
		AllowSetForegroundWindow(gState.conemuPid_);
	}
	// "ConEmuHWND"="0x%08X", "ConEmuPID"="%u"
	SetConEmuEnvVar(gState.conemuWnd_);

	// Drawing canvas & Background windows
	_ASSERTE(gState.conemuWnd_!=nullptr || (hDcWnd==nullptr && hBackWnd==nullptr));
	if (hDcWnd && !IsWindow(hDcWnd))
	{
		_ASSERTE(FALSE && "hDcWnd is invalid");
		hDcWnd = nullptr;
	}
	if (hBackWnd && !IsWindow(hBackWnd))
	{
		_ASSERTE(FALSE && "hBackWnd is invalid");
		hBackWnd = nullptr;
	}
	// Set new descriptors
	if ((gState.conemuWndDC_ != hDcWnd) || (gState.conemuWndBack_ != hBackWnd))
	{
		sprintf_c(szInfo+lstrlenA(szInfo), 60/*#SECURELEN*/, "WndDC=x%08X WndBack=x%08X", (DWORD)(DWORD_PTR)hDcWnd, (DWORD)(DWORD_PTR)hBackWnd);
		gState.conemuWndDC_ = hDcWnd;
		gState.conemuWndBack_ = hBackWnd;
		// "ConEmuDrawHWND"="0x%08X", "ConEmuBackHWND"="0x%08X"
		SetConEmuEnvVarChild(hDcWnd, hBackWnd);
	}

	if (gpLogSize && szInfo[0])
	{
		LogFunction(szInfo);
	}
}

// Allow smth like: ConEmuC -c {Far} /e text.txt
CEStr WorkerBase::ExpandTaskCmd(LPCWSTR asCmdLine) const
{
	if (!gState.realConWnd_)
	{
		_ASSERTE(gState.realConWnd_);
		return {};
	}
	if (!asCmdLine || (asCmdLine[0] != TaskBracketLeft))
	{
		_ASSERTE(asCmdLine && (asCmdLine[0] == TaskBracketLeft));
		return {};
	}
	LPCWSTR pszNameEnd = wcschr(asCmdLine, TaskBracketRight);
	if (!pszNameEnd)
	{
		return {};
	}
	pszNameEnd++;

	const size_t cchCount = (pszNameEnd - asCmdLine);
	const DWORD cbSize = DWORD(sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_TASK) + cchCount*sizeof(asCmdLine[0]));
	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_GETTASKCMD, cbSize);
	if (!pIn)
	{
		_ASSERTE(FALSE && "Failed to allocate CECMD_GETTASKCMD");
		return {};
	}
	wmemmove(pIn->GetTask.data, asCmdLine, cchCount);
	_ASSERTE(pIn->GetTask.data[cchCount] == 0);

	wchar_t* pszResult = nullptr;
	// #SERVER Use conemuPid_ instead of conemuWnd_
	CESERVER_REQ* pOut = ExecuteGuiCmd(gState.conemuWnd_, pIn, gState.realConWnd_);
	if (pOut && (pOut->DataSize() > sizeof(pOut->GetTask)) && pOut->GetTask.data[0])
	{
		const LPCWSTR pszTail = SkipNonPrintable(pszNameEnd);
		pszResult = lstrmerge(pOut->GetTask.data, (pszTail && *pszTail) ? L" " : nullptr, pszTail);
	}
	ExecuteFreeResult(pIn);
	ExecuteFreeResult(pOut);

	return pszResult;
}

void WorkerBase::CreateDefTermChildMapping(const PROCESS_INFORMATION& pi)
{
	if (!defTermChildMap_)
		defTermChildMap_ = std::make_shared<CDefTermChildMap>();

	if (gState.conemuPid_ == 0)
	{
		_ASSERTE(gState.conemuPid_ != 0);
		return;
	}

	defTermChildMap_->CreateChildMapping(pi.dwProcessId, pi.hProcess, gState.conemuPid_);

}

void WorkerBase::FreezeRefreshThread()
{
	_ASSERTE(FALSE && "FreezeRefreshThread is not implemented in base");
}

void WorkerBase::ThawRefreshThread()
{
	_ASSERTE(FALSE && "ThawRefreshThread is not implemented in base");
}

bool WorkerBase::IsRefreshFreezeRequests()
{
	return false;
}

void WorkerBase::UpdateConsoleMapHeader(LPCWSTR asReason)
{
	_ASSERTE(FALSE && "UpdateConsoleMapHeader is not implemented in base");
}

void WorkerBase::SetConEmuFolders(LPCWSTR asExeDir, LPCWSTR asBaseDir)
{
	_ASSERTE(FALSE && "SetConEmuFolders is not implemented in base");
}
