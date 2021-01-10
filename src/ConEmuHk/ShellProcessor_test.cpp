
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "../common/defines.h"
#include "../UnitTests/gtest.h"
#include "Ansi.h"
#include "ShellProcessor.h"
#include "GuiAttach.h"
#include "DefTermHk.h"
#include "DllOptions.h"
#include <memory>

#include "../common/execute.h"
#include "../ConEmuHk/MainThread.h"
#include "../UnitTests/test_mock_file.h"

#ifndef __GNUC__
extern "C" IMAGE_DOS_HEADER __ImageBase;
#endif

namespace {
template<typename T>
struct VarSet
{
	T& var_;
	const T save_;
	VarSet(T& var, T value)
		: var_(var), save_(var)
	{
		var = value;
	}
	~VarSet()
	{
		var_ = save_;
	}
	VarSet(const VarSet&) = delete;
	VarSet& operator=(const VarSet&) = delete;
	VarSet(VarSet&&) = delete;
	VarSet& operator=(VarSet&&) = delete;
};
}


TEST(ShellProcessor, Test)
{
	MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> mappingMock;
	CESERVER_CONSOLE_MAPPING_HDR srvMap{};
	ghConWnd = GetRealConsoleWindow();
	if (!ghConWnd)
	{
		cdbg() << "Test is not functional, GetRealConsoleWindow is nullptr";
		return;
	}
	if (!LoadSrvMapping(ghConWnd, srvMap))
	{
		mappingMock.InitName(CECONMAPNAME, LODWORD(ghConWnd));
		if (mappingMock.Create())
		{
			srvMap.cbSize = sizeof(srvMap);
			srvMap.hConEmuRoot = ghConWnd;
			srvMap.hConEmuWndBack = ghConWnd;
			srvMap.hConEmuWndDc = ghConWnd;
			srvMap.nServerPID = GetCurrentProcessId();
			srvMap.useInjects = ConEmuUseInjects::Use;
			srvMap.nProtocolVersion = CESERVER_REQ_VER;
			wcscpy_s(srvMap.sConEmuExe, L"C:\\Tools\\ConEmu.exe");
			wcscpy_s(srvMap.ComSpec.Comspec32, L"C:\\Windows\\System32\\cmd.exe");
			wcscpy_s(srvMap.ComSpec.Comspec64, L"C:\\Windows\\System32\\cmd.exe");
			wcscpy_s(srvMap.ComSpec.ConEmuExeDir, L"C:\\Tools");
			wcscpy_s(srvMap.ComSpec.ConEmuBaseDir, L"C:\\Tools\\ConEmu");

			mappingMock.SetFrom(&srvMap);
		}
	}

	VarSet<HWND> setRoot(ghConEmuWnd, srvMap.hConEmuRoot);
	VarSet<HWND> setBack(ghConEmuWndBack, srvMap.hConEmuWndBack);
	VarSet<HWND> setDc(ghConEmuWndDC, srvMap.hConEmuWndDc);
	VarSet<DWORD> setSrvPid(gnServerPID, srvMap.nServerPID);
	VarSet<DWORD> setMainTid(gnHookMainThreadId, GetCurrentThreadId());
	#ifndef __GNUC__
	VarSet<HANDLE2> setModule(ghWorkingModule, HANDLE2{ reinterpret_cast<uint64_t>(reinterpret_cast<HMODULE>(&__ImageBase)) });
	#endif

	FarVersion farVer{}; farVer.dwVerMajor = 3; farVer.dwVerMinor = 0; farVer.dwBuild = 5709;
	VarSet<HookModeFar> farMode(gFarMode,
		{ sizeof(HookModeFar), true, 0, 0, 0, 0, true, farVer, nullptr });

	if (!srvMap.hConEmuWndDc || !IsWindow(srvMap.hConEmuWndDc))
	{
		cdbg() << "Test is not functional, hConEmuWndDc is nullptr";
		return;
	}

	test_mocks::FileSystemMock fileMock;
	fileMock.MockFile(LR"(C:\mingw\bin\mingw32-make.exe)", 512, IMAGE_SUBSYSTEM_WINDOWS_CUI, 32);
	fileMock.MockFile(LR"(C:\DosGames\Prince\PRINCE.EXE)", 512, IMAGE_SUBSYSTEM_DOS_EXECUTABLE, 16);
	fileMock.MockFile(LR"(C:\1 @\a.cmd)", 128, IMAGE_SUBSYSTEM_BATCH_FILE, 32);
	fileMock.MockFile(LR"(C:\Tools\ConEmu.exe)", 512, IMAGE_SUBSYSTEM_WINDOWS_GUI, 32);
	fileMock.MockFile(LR"(C:\Tools\ConEmu64.exe)", 512, IMAGE_SUBSYSTEM_WINDOWS_GUI, 64);
	fileMock.MockFile(LR"(C:\Tools\ConEmu\ConEmuC.exe)", 512, IMAGE_SUBSYSTEM_WINDOWS_CUI, 32);
	fileMock.MockFile(LR"(C:\ToolsConEmu\ConEmuC64.exe)", 512, IMAGE_SUBSYSTEM_WINDOWS_CUI, 64);

	enum class Function { CreateW, ShellW };
	struct TestInfo
	{
		Function function;
		const wchar_t* file;
		const wchar_t* param;
		const wchar_t* expectFile;
		const wchar_t* expectParam;
	};

	TestInfo tests[] = {
		{Function::CreateW,
			LR"(C:\mingw\bin\mingw32-make.exe)", LR"(mingw32-make "1.cpp" )",
			nullptr, LR"("C:\Tools\ConEmu\ConEmuC.exe" /PARENTFARPID=%u /C "C:\mingw\bin\mingw32-make.exe" "1.cpp" )"},
		{Function::CreateW,
			LR"(C:\mingw\bin\mingw32-make.exe)", LR"("mingw32-make.exe" "1.cpp" )",
			nullptr, LR"("C:\Tools\ConEmu\ConEmuC.exe" /PARENTFARPID=%u /C "C:\mingw\bin\mingw32-make.exe" "1.cpp" )"},
		{Function::CreateW,
			LR"(C:\mingw\bin\mingw32-make.exe)", LR"("C:\mingw\bin\mingw32-make.exe" "1.cpp" )",
			nullptr, LR"("C:\Tools\ConEmu\ConEmuC.exe" /PARENTFARPID=%u /C ""C:\mingw\bin\mingw32-make.exe" "1.cpp" ")"},

		{Function::CreateW,
			nullptr, LR"("C:\1 @\a.cmd")",
			nullptr, LR"("C:\Tools\ConEmu\ConEmuC.exe" /PARENTFARPID=%u /C ""C:\1 @\a.cmd"")"},

		// #TODO: Add DosBox mock/test
		{Function::CreateW,
			LR"(C:\DosGames\Prince\PRINCE.EXE)", LR"(prince megahit)",
			LR"(C:\DosGames\Prince\PRINCE.EXE)", LR"(prince megahit)"},
		{Function::CreateW,
			nullptr, LR"( "C:\DosGames\Prince\PRINCE.EXE")",
			nullptr, LR"( "C:\DosGames\Prince\PRINCE.EXE")"},

		{Function::ShellW,
			LR"(C:\mingw\bin\mingw32-make.exe)", LR"( "1.cpp" )",
			LR"(C:\mingw\bin\mingw32-make.exe)", LR"( "1.cpp" )"},
	};

	for (const auto& test : tests)
	{
		MCHKHEAP;
		auto sp = std::make_shared<CShellProc>();
		LPCWSTR pszFile = test.file, pszParam = test.param;
		DWORD nCreateFlags = CREATE_DEFAULT_ERROR_MODE, nShowCmd = 0;
		STARTUPINFOW si = {};
		auto* psi = &si;
		si.cb = sizeof(si);

		wchar_t paramBuffer[1024] = L"";
		const auto pid = GetCurrentProcessId();
		msprintf(paramBuffer, countof(paramBuffer), test.expectParam, pid);

		const CEStr testInfo(L"file=", test.file ? test.file : L"<null>",
			L"; param=", test.param ? test.param : L"<null>", L";");

		switch (test.function)
		{
		case Function::CreateW:
			EXPECT_TRUE(sp->OnCreateProcessW(&pszFile, &pszParam, nullptr, &nCreateFlags, &psi));
			EXPECT_STREQ(pszFile, test.expectFile) << testInfo.c_str();
			EXPECT_STREQ(pszParam, paramBuffer) << testInfo.c_str();
			break;
		case Function::ShellW:
			EXPECT_TRUE(sp->OnShellExecuteW(nullptr, &pszFile, &pszParam, nullptr, &nCreateFlags, &nShowCmd));
			EXPECT_STREQ(pszFile, test.expectFile) << testInfo.c_str();
			EXPECT_STREQ(pszParam, paramBuffer) << testInfo.c_str();
			break;
		default:
			break;
		}
		MCHKHEAP;
	}
}

TEST(ShellProcessor, WorkOptions)
{
	const ShellWorkOptions options = ShellWorkOptions::ChildGui | ShellWorkOptions::VsNetHost
	| ShellWorkOptions::WasSuspended | ShellWorkOptions::WasDebug;
	EXPECT_EQ(static_cast<uint32_t>(options),
		static_cast<uint32_t>(ShellWorkOptions::ChildGui) | static_cast<uint32_t>(ShellWorkOptions::VsNetHost)
		| static_cast<uint32_t>(ShellWorkOptions::WasSuspended) | static_cast<uint32_t>(ShellWorkOptions::WasDebug));

	ShellWorkOptions ored = ShellWorkOptions::None;
	EXPECT_EQ(ored, ShellWorkOptions::None);
	ored |= ShellWorkOptions::ChildGui;
	EXPECT_EQ(ored, ShellWorkOptions::ChildGui);
	ored |= ShellWorkOptions::WasDebug;
	EXPECT_EQ(ored, static_cast<ShellWorkOptions>(static_cast<uint32_t>(ShellWorkOptions::ChildGui) | static_cast<uint32_t>(ShellWorkOptions::WasDebug)));

	EXPECT_TRUE(options & ShellWorkOptions::WasSuspended);
	EXPECT_TRUE(options & ShellWorkOptions::WasDebug);
	EXPECT_TRUE(options & (ShellWorkOptions::WasSuspended | ShellWorkOptions::WasDebug));
	EXPECT_FALSE(options & (ShellWorkOptions::WasSuspended | ShellWorkOptions::VsDebugConsole));
	EXPECT_FALSE(options & ShellWorkOptions::VsDebugConsole);
	EXPECT_TRUE(ShellWorkOptions::None & ShellWorkOptions::None);
}


// some global stubs
BOOL    gbAttachGuiClient = FALSE;
BOOL 	gbGuiClientAttached = FALSE;
GuiCui  gnAttachPortableGuiCui = e_Alone;
HWND 	ghAttachGuiClient = nullptr;
GetProcessId_t gfGetProcessId = nullptr;
DWORD gnHookMainThreadId = 0;
HANDLE CEAnsi::ghAnsiLogFile = nullptr;
LONG CEAnsi::gnEnterPressed = 0;

// some function mocks
void CheckHookServer()
{
}

void CEAnsi::ChangeTermMode(TermModeCommand mode, DWORD value, DWORD nPID /*= 0*/)
{
}

void CEAnsi::WriteAnsiLogW(LPCWSTR lpBuffer, DWORD nChars)
{
}

void CEAnsi::WriteAnsiLogFarPrompt()
{
}

bool CDefTermHk::IsDefTermEnabled()
{
	return false;
}

void CDefTermHk::DefTermLogString(LPCSTR asMessage, LPCWSTR asLabel)
{
}

void CDefTermHk::DefTermLogString(LPCWSTR asMessage, LPCWSTR asLabel)
{
}

bool CDefTermHk::LoadDefTermSrvMapping(CESERVER_CONSOLE_MAPPING_HDR& srvMapping)
{
	return false;
}

HWND CDefTermHk::AllocHiddenConsole(const bool /*bTempForVS*/)
{
	return nullptr;
}

size_t CDefTermHk::GetSrvAddArgs(bool bGuiArgs, bool forceInjects, CEStr& rsArgs, CEStr& rsNewCon)
{
	return 0;
}

DWORD CDefTermHk::GetConEmuInsidePid()
{
	return 0;
}

void CDefTermHk::CreateChildMapping(DWORD childPid, const MHandle& childHandle, DWORD conemuInsidePid)
{
}

HWND WINAPI GetRealConsoleWindow()
{
	const MModule ConEmuHk(GetModuleHandle(ConEmuHk_DLL_3264));
	if (ConEmuHk.IsValid())
	{
		HWND(WINAPI * getConsoleWindow)() = nullptr;
		EXPECT_TRUE(ConEmuHk.GetProcAddress("GetRealConsoleWindow", getConsoleWindow));
		if (getConsoleWindow)
		{
			wcdbg() << L"Calling GetRealConsoleWindow from " << ConEmuHk_DLL_3264 << std::endl;
			return getConsoleWindow();
		}
	}
	wcdbg() << L"Calling GetConsoleWindow from WinAPI" << std::endl;
	return GetConsoleWindow();
}

bool AttachServerConsole()
{
	return false;
}

CINJECTHK_EXIT_CODES InjectHooks(PROCESS_INFORMATION pi, DWORD imageBits, BOOL abLogProcess, LPCWSTR asConEmuHkDir, HWND hConWnd)
{
	return CIH_NtdllNotLoaded;
}
