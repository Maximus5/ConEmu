
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
#include "../UnitTests/test_mock_file.h"
#include "../common/EnvVar.h"
#include "../common/execute.h"
#include "../common/WObjects.h"
#include "../ConEmuHk/MainThread.h"
#include "Ansi.h"
#include "ShellProcessor.h"
#include "GuiAttach.h"
#include "DefTermHk.h"
#include "DllOptions.h"
#include <memory>
#include <string>


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

class ShellProcessor : public testing::Test
{
public:
	void SetUp() override
	{
		ghConWnd = GetRealConsoleWindow();
		if (!ghConWnd)
		{
			cdbg() << "Test is not functional, GetRealConsoleWindow is nullptr" << std::endl;
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
				srvMap.Flags = ConEmu::ConsoleFlags::ProcessNewCon;
				srvMap.nProtocolVersion = CESERVER_REQ_VER;
				wcscpy_s(srvMap.sConEmuExe, L"C:\\Tools\\ConEmu.exe");
				wcscpy_s(srvMap.ComSpec.Comspec32, L"C:\\Windows\\System32\\cmd.exe");
				wcscpy_s(srvMap.ComSpec.Comspec64, L"C:\\Windows\\System32\\cmd.exe");
				wcscpy_s(srvMap.ComSpec.ConEmuExeDir, L"C:\\Tools");
				wcscpy_s(srvMap.ComSpec.ConEmuBaseDir, L"C:\\Tools\\ConEmu");

				mappingMock.SetFrom(&srvMap);
			}
		}

		SetEnvironmentVariableW(L"ConEmuBaseDirTest", srvMap.ComSpec.ConEmuBaseDir);
		SetEnvironmentVariableW(L"ConEmuLogTest", srvMap.nLogLevel ? L"/LOG " : L"");
		wchar_t processId[32] = L"";
		const auto pid = GetCurrentProcessId();
		SetEnvironmentVariableW(L"ConEmuTestPid", ltow_s(pid, processId, 10));

		setRoot = std::make_unique<VarSet<HWND>>(ghConEmuWnd, srvMap.hConEmuRoot);
		setBack = std::make_unique<VarSet<HWND>>(ghConEmuWndBack, srvMap.hConEmuWndBack);
		setDc = std::make_unique<VarSet<HWND>>(ghConEmuWndDC, srvMap.hConEmuWndDc);
		setSrvPid = std::make_unique<VarSet<DWORD>>(gnServerPID, srvMap.nServerPID);
		setMainTid = std::make_unique<VarSet<DWORD>>(gnHookMainThreadId, GetCurrentThreadId());
		#ifndef __GNUC__
		setModule = std::make_unique<VarSet<HANDLE2>>(ghWorkingModule, HANDLE2{ reinterpret_cast<uint64_t>(reinterpret_cast<HMODULE>(&__ImageBase)) });
		#endif

		// farMode - should be set up in the test

		fileMock = std::make_unique<test_mocks::FileSystemMock>();

		CEStr exeMock = JoinPath(srvMap.ComSpec.ConEmuExeDir, L"ConEmu.exe");
		fileMock->MockFile(exeMock.c_str(), 512, IMAGE_SUBSYSTEM_WINDOWS_GUI, 32);
		exeMock = JoinPath(srvMap.ComSpec.ConEmuExeDir, L"ConEmu64.exe");
		fileMock->MockFile(exeMock.c_str(), 512, IMAGE_SUBSYSTEM_WINDOWS_GUI, 64);
		exeMock = JoinPath(srvMap.ComSpec.ConEmuBaseDir, L"ConEmuC.exe");
		fileMock->MockFile(exeMock.c_str(), 512, IMAGE_SUBSYSTEM_WINDOWS_CUI, 32);
		exeMock = JoinPath(srvMap.ComSpec.ConEmuBaseDir, L"ConEmuC64.exe");
		fileMock->MockFile(exeMock.c_str(), 512, IMAGE_SUBSYSTEM_WINDOWS_CUI, 64);

		#ifdef _WIN64
		fileMock->MockFile(srvMap.ComSpec.Comspec64, 512, IMAGE_SUBSYSTEM_WINDOWS_CUI, 64);
		#else
		fileMock->MockFile(srvMap.ComSpec.Comspec32, 512, IMAGE_SUBSYSTEM_WINDOWS_CUI, 32);
		#endif
	}

	void TearDown() override
	{
		SetEnvironmentVariableW(L"ConEmuBaseDirTest", nullptr);
		SetEnvironmentVariableW(L"ConEmuLogTest", nullptr);
		SetEnvironmentVariableW(L"ConEmuTestPid", nullptr);

		mappingMock.CloseMap();

		setRoot.reset();
		setBack.reset();
		setDc.reset();
		setSrvPid.reset();
		setMainTid.reset();
		setModule.reset();
		farMode.reset();

		fileMock.reset();
	}

protected:
	MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> mappingMock;
	CESERVER_CONSOLE_MAPPING_HDR srvMap{};

	std::unique_ptr<VarSet<HWND>> setRoot;
	std::unique_ptr<VarSet<HWND>> setBack;
	std::unique_ptr<VarSet<HWND>> setDc;
	std::unique_ptr<VarSet<DWORD>> setSrvPid;
	std::unique_ptr<VarSet<DWORD>> setMainTid;
	std::unique_ptr<VarSet<HANDLE2>> setModule;
	std::unique_ptr<VarSet<HookModeFar>> farMode;

	std::unique_ptr<test_mocks::FileSystemMock> fileMock;
};


TEST_F(ShellProcessor, Far300)
{
	if (!srvMap.hConEmuWndDc || !IsWindow(srvMap.hConEmuWndDc))
	{
		cdbg() << "Test is not functional, hConEmuWndDc is nullptr";
		return;
	}

	FarVersion farVer{}; farVer.dwVerMajor = 3; farVer.dwVerMinor = 0; farVer.dwBuild = 5709;
	farMode = std::make_unique<VarSet<HookModeFar>>(gFarMode,
		HookModeFar{ sizeof(HookModeFar), true, 0, 0, 0, 0, true, farVer, nullptr });

	fileMock->MockFile(LR"(C:\mingw\bin\mingw32-make.exe)", 512, IMAGE_SUBSYSTEM_WINDOWS_CUI, 32);
	fileMock->MockFile(LR"(C:\DosGames\Prince\PRINCE.EXE)", 512, IMAGE_SUBSYSTEM_DOS_EXECUTABLE, 16);
	fileMock->MockFile(LR"(C:\1 @\a.cmd)", 128, IMAGE_SUBSYSTEM_BATCH_FILE, 32); // on bitness we decide ConEmuC.exe or ConEmuC64.exe to use, overrided by comspec settings

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
			nullptr,
			LR"("%ConEmuBaseDirTest%\ConEmuC.exe" %ConEmuLogTest%/PARENTFARPID=%ConEmuTestPid% /C C:\mingw\bin\mingw32-make.exe "1.cpp" )"},
		{Function::CreateW,
			LR"(C:\mingw\bin\mingw32-make.exe)", LR"("mingw32-make.exe" "1.cpp" )",
			nullptr,
			LR"("%ConEmuBaseDirTest%\ConEmuC.exe" %ConEmuLogTest%/PARENTFARPID=%ConEmuTestPid% /C "C:\mingw\bin\mingw32-make.exe" "1.cpp" )"},
		{Function::CreateW,
			LR"(C:\mingw\bin\mingw32-make.exe)", LR"("C:\mingw\bin\mingw32-make.exe" "1.cpp" )",
			nullptr,
			LR"("%ConEmuBaseDirTest%\ConEmuC.exe" %ConEmuLogTest%/PARENTFARPID=%ConEmuTestPid% /C "C:\mingw\bin\mingw32-make.exe" "1.cpp" )"},

		{Function::CreateW,
			nullptr, LR"(""C:\1 @\a.cmd"")",
			nullptr,
			LR"("%ConEmuBaseDirTest%\)" ConEmuC_32_EXE R"(" %ConEmuLogTest%/PARENTFARPID=%ConEmuTestPid% /C ""C:\1 @\a.cmd"")"},

		{Function::CreateW,
			LR"(C:\1 @\a.cmd)", nullptr, // important to have two double quotes, otherwise cmd will strip one and fail to find the file
			nullptr,
			LR"("%ConEmuBaseDirTest%\)" ConEmuC_32_EXE R"(" %ConEmuLogTest%/PARENTFARPID=%ConEmuTestPid% /C ""C:\1 @\a.cmd"")"},
		
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

	for (size_t i = 0; i < countof(tests); ++i)
	{
		const auto& test = tests[i];
		auto sp = std::make_shared<CShellProc>();
		LPCWSTR pszFile = test.file, pszParam = test.param;
		DWORD nCreateFlags = CREATE_DEFAULT_ERROR_MODE, nShowCmd = 0;
		STARTUPINFOW si = {};
		auto* psi = &si;
		si.cb = sizeof(si);

		CEStr expanded(ExpandEnvStr(test.expectParam));

		const std::wstring testInfo(L"#" + std::to_wstring(i)
			+ L": file=" + (test.file ? test.file : L"<null>")
			+ L"; param=" + (test.param ? test.param : L"<null>") + L";");

		switch (test.function)
		{
		case Function::CreateW:
			EXPECT_TRUE(sp->OnCreateProcessW(&pszFile, &pszParam, nullptr, &nCreateFlags, &psi));
			EXPECT_STREQ(pszFile, test.expectFile) << testInfo.c_str();
			EXPECT_STREQ(pszParam, expanded.c_str()) << testInfo.c_str();
			break;
		case Function::ShellW:
			EXPECT_TRUE(sp->OnShellExecuteW(nullptr, &pszFile, &pszParam, nullptr, nullptr, &nShowCmd));
			EXPECT_STREQ(pszFile, test.expectFile) << testInfo.c_str();
			EXPECT_STREQ(pszParam, expanded.c_str()) << testInfo.c_str();
			break;
		default:
			break;
		}
	}
}

TEST_F(ShellProcessor, Far175)
{
	if (!srvMap.hConEmuWndDc || !IsWindow(srvMap.hConEmuWndDc))
	{
		cdbg() << "Test is not functional, hConEmuWndDc is nullptr";
		return;
	}

	FarVersion farVer{}; farVer.dwVerMajor = 1; farVer.dwVerMinor = 75; farVer.dwBuild = 2634;
	farMode = std::make_unique<VarSet<HookModeFar>>(gFarMode,
		HookModeFar{ sizeof(HookModeFar), true, 0, 0, 0, 0, true, farVer, nullptr });

	fileMock->MockFile(LR"(C:\1 @\a.cmd)", 128, IMAGE_SUBSYSTEM_BATCH_FILE, 32); // on bitness we decide ConEmuC.exe or ConEmuC64.exe to use

	enum class Function { CreateA, ShellA };
	struct TestInfo
	{
		Function function;
		const char* file;
		const char* param;
		const char* expectFile;
		const char* expectParam;
	};

	TestInfo tests[] = {
		{Function::CreateA,
			nullptr, R"(""C:\1 @\a.cmd"")",
			nullptr,
			R"("%ConEmuBaseDirTest%\)" "ConEmuC.exe" R"(" %ConEmuLogTest%/PARENTFARPID=%ConEmuTestPid% /C ""C:\1 @\a.cmd"")"},
		{Function::CreateA,
			nullptr, R"(C:\Windows\system32\cmd.exe /K ""C:\1 @\a.cmd"")",
			nullptr,
			R"("%ConEmuBaseDirTest%\ConEmuC.exe" %ConEmuLogTest%/PARENTFARPID=%ConEmuTestPid% /K ""C:\1 @\a.cmd"")"},
		{Function::ShellA,
			R"(C:\Windows\system32\cmd.exe)", R"(/C ""C:\1 @\a.cmd" -new_console")",
			R"(%ConEmuBaseDirTest%\)" WIN3264TEST("ConEmuC.exe", "ConEmuC64.exe"),
			R"(%ConEmuLogTest%/PARENTFARPID=%ConEmuTestPid% /C "C:\Windows\system32\cmd.exe" /C ""C:\1 @\a.cmd" -new_console")"},
		{Function::CreateA,
			nullptr, R"("C:\Windows\system32\cmd.exe" /C ""C:\1 @\a.cmd"")",
			nullptr,
			R"("%ConEmuBaseDirTest%\ConEmuC.exe" %ConEmuLogTest%/PARENTFARPID=%ConEmuTestPid% /C ""C:\1 @\a.cmd"")"},
		{Function::CreateA,
			nullptr, R"("C:\Windows\system32\cmd.exe" /C ""C:\1 @\a.cmd""  )",
			nullptr,
			R"("%ConEmuBaseDirTest%\ConEmuC.exe" %ConEmuLogTest%/PARENTFARPID=%ConEmuTestPid% /C ""C:\1 @\a.cmd""  )"},
		{Function::CreateA,
			nullptr, R"(C:\Windows\system32\cmd.exe /C "set > res.log")",
			nullptr,
			R"("%ConEmuBaseDirTest%\)" WIN3264TEST("ConEmuC.exe","ConEmuC64.exe") R"(" %ConEmuLogTest%/PARENTFARPID=%ConEmuTestPid% /C "set > res.log")"},
	};

	for (size_t i = 0; i < countof(tests); ++i)
	{
		const auto& test = tests[i];
		auto sp = std::make_shared<CShellProc>();
		LPCSTR resultFile = test.file, resultParam = test.param;
		DWORD nCreateFlags = CREATE_DEFAULT_ERROR_MODE, nShowCmd = 0;
		STARTUPINFOA si = {};
		auto* psi = &si;
		si.cb = sizeof(si);

		CEStrA expectFile(ExpandEnvStr(test.expectFile));
		CEStrA expectParam(ExpandEnvStr(test.expectParam));

		const std::string testInfo("#" + std::to_string(i)
			+ ": file=" + (test.file ? test.file : "<null>")
			+ "; param=" + (test.param ? test.param : "<null>") + ";");

		switch (test.function)
		{
		case Function::CreateA:
			EXPECT_TRUE(sp->OnCreateProcessA(&resultFile, &resultParam, nullptr, &nCreateFlags, &psi));
			EXPECT_STREQ(resultFile, expectFile.c_str()) << testInfo.c_str();
			EXPECT_STREQ(resultParam, expectParam.c_str()) << testInfo.c_str();
			break;
		case Function::ShellA:
			EXPECT_TRUE(sp->OnShellExecuteA(nullptr, &resultFile, &resultParam, nullptr, nullptr, &nShowCmd));
			EXPECT_STREQ(resultFile, expectFile.c_str()) << testInfo.c_str();
			EXPECT_STREQ(resultParam, expectParam.c_str()) << testInfo.c_str();
			break;
		default:
			break;
		}
	}
}

TEST_F(ShellProcessor, Cmd)
{
	if (!srvMap.hConEmuWndDc || !IsWindow(srvMap.hConEmuWndDc))
	{
		cdbg() << "Test is not functional, hConEmuWndDc is nullptr";
		return;
	}

	farMode = std::make_unique<VarSet<HookModeFar>>(gFarMode, HookModeFar{});

	const CEStr comspec = GetEnvVar(L"ComSpec");

	fileMock->MockFile(LR"(C:\1 @\a.cmd)", 128, IMAGE_SUBSYSTEM_BATCH_FILE, 32); // on bitness we decide ConEmuC.exe or ConEmuC64.exe to use
	fileMock->MockFile(LR"(C:\1 @\util.exe)", 128, IMAGE_SUBSYSTEM_WINDOWS_CUI, 32);
	fileMock->MockFile(LR"(C:\1 &\a.cmd)", 128, IMAGE_SUBSYSTEM_BATCH_FILE, 32); // on bitness we decide ConEmuC.exe or ConEmuC64.exe to use
	fileMock->MockFile(LR"(C:\1 &\util.exe)", 128, IMAGE_SUBSYSTEM_WINDOWS_CUI, 32);

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
			comspec.c_str(), LR"(cmd /k ""C:\1 @\a.cmd" arg1 arg2" -new_console:t:"test")",
			nullptr, LR"("%ConEmuBaseDirTest%\ConEmuC.exe" %ConEmuLogTest%/K ""C:\1 @\a.cmd" arg1 arg2" -new_console:t:"test")"},
		{Function::CreateW,
			LR"(C:\1 @\util.exe)", LR"(util ""C:\1 @\a.cmd" arg1 arg2" -new_console:t:"test")",
			nullptr, LR"("%ConEmuBaseDirTest%\ConEmuC.exe" %ConEmuLogTest%/C "C:\1 @\util.exe" ""C:\1 @\a.cmd" arg1 arg2" -new_console:t:"test")"},
		{Function::CreateW,
			LR"(C:\1 @\util.exe)", LR"("util" ""C:\1 @\a.cmd" arg1 arg2" -new_console:t:"test")",
			nullptr, LR"("%ConEmuBaseDirTest%\ConEmuC.exe" %ConEmuLogTest%/C "C:\1 @\util.exe" ""C:\1 @\a.cmd" arg1 arg2" -new_console:t:"test")"},
		{Function::CreateW,
			LR"(C:\1 @\util.exe)", LR"(util.exe ""C:\1 @\a.cmd" arg1 arg2" -new_console:t:"test")",
			nullptr, LR"("%ConEmuBaseDirTest%\ConEmuC.exe" %ConEmuLogTest%/C "C:\1 @\util.exe" ""C:\1 @\a.cmd" arg1 arg2" -new_console:t:"test")"},
		{Function::CreateW,
			comspec.c_str(), LR"(cmd /k ""C:\1 &\a.cmd" arg1 arg2" -new_console:t:"test")",
			nullptr, LR"("%ConEmuBaseDirTest%\ConEmuC.exe" %ConEmuLogTest%/K ""C:\1 &\a.cmd" arg1 arg2" -new_console:t:"test")"},
		{Function::CreateW,
			LR"(C:\1 &\util.exe)", LR"(util ""C:\1 &\a.cmd" arg1 arg2" -new_console:t:"test")",
			nullptr, LR"("%ConEmuBaseDirTest%\ConEmuC.exe" %ConEmuLogTest%/C "C:\1 &\util.exe" ""C:\1 &\a.cmd" arg1 arg2" -new_console:t:"test")"},
		{Function::CreateW,
			LR"(C:\1 &\util.exe)", LR"("util" ""C:\1 &\a.cmd" arg1 arg2" -new_console:t:"test")",
			nullptr, LR"("%ConEmuBaseDirTest%\ConEmuC.exe" %ConEmuLogTest%/C "C:\1 &\util.exe" ""C:\1 &\a.cmd" arg1 arg2" -new_console:t:"test")"},
		{Function::CreateW,
			LR"(C:\1 &\util.exe)", LR"(util.exe ""C:\1 &\a.cmd" arg1 arg2" -new_console:t:"test")",
			nullptr, LR"("%ConEmuBaseDirTest%\ConEmuC.exe" %ConEmuLogTest%/C "C:\1 &\util.exe" ""C:\1 &\a.cmd" arg1 arg2" -new_console:t:"test")"},
	};

	for (size_t i = 0; i < countof(tests); ++i)
	{
		const auto& test = tests[i];
		auto sp = std::make_shared<CShellProc>();
		LPCWSTR pszFile = test.file, pszParam = test.param;
		DWORD nCreateFlags = CREATE_DEFAULT_ERROR_MODE, nShowCmd = 0;
		STARTUPINFOW si = {};
		auto* psi = &si;
		si.cb = sizeof(si);

		CEStr expanded(ExpandEnvStr(test.expectParam));

		const std::wstring testInfo(L"#" + std::to_wstring(i)
			+ L": file=" + (test.file ? test.file : L"<null>")
			+ L"; param=" + (test.param ? test.param : L"<null>") + L";");

		switch (test.function)
		{
		case Function::CreateW:
			EXPECT_TRUE(sp->OnCreateProcessW(&pszFile, &pszParam, nullptr, &nCreateFlags, &psi));
			EXPECT_STREQ(pszFile, test.expectFile) << testInfo.c_str();
			EXPECT_STREQ(pszParam, expanded.c_str()) << testInfo.c_str();
			break;
		case Function::ShellW:
			EXPECT_TRUE(sp->OnShellExecuteW(nullptr, &pszFile, &pszParam, nullptr, nullptr, &nShowCmd));
			EXPECT_STREQ(pszFile, test.expectFile) << testInfo.c_str();
			EXPECT_STREQ(pszParam, expanded.c_str()) << testInfo.c_str();
			break;
		default:
			break;
		}
	}
}

TEST_F(ShellProcessor, WorkOptions)
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
