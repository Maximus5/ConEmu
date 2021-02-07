
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include <thread>

#include "../common/defines.h"
#include "../common/CmdLine.h"
#include "../common/ConEmuCheck.h"
#include "../common/MHandle.h"
#include "../common/MModule.h"
#include "../common/MToolHelp.h"
#include "../ConEmuCD/ExitCodes.h"
#include "../UnitTests/gtest.h"
#include "../ConEmuCD/ExportedFunctions.h"


class OutputWasRedirected final : public std::runtime_error
{
public:
	using std::runtime_error::runtime_error;
};

struct ConsoleOutputCapture
{
	ConsoleOutputCapture(bool change = false)
	{
		hPrevOut = GetStdHandle(STD_OUTPUT_HANDLE);
		hPrevErr = GetStdHandle(STD_ERROR_HANDLE);
		hBuffer = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr);
		if (!hBuffer || hBuffer == INVALID_HANDLE_VALUE)
			throw std::runtime_error("CreateConsoleScreenBuffer, error=" + std::to_string(GetLastError()));
		SetConsoleScreenBufferSize(hBuffer, COORD{ 80, 25 });
		SetConsoleCursorPosition(hBuffer, COORD{ 0, 0 });
		//if (change)
		//{
		//	hPrevBuffer = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		//		nullptr, OPEN_EXISTING, 0, nullptr);
		//	changed = SetConsoleActiveScreenBuffer(hBuffer);
		//}
		SetStdHandle(STD_OUTPUT_HANDLE, hBuffer);
		SetStdHandle(STD_ERROR_HANDLE, hBuffer);
	}

	~ConsoleOutputCapture()
	{
		//if (hPrevBuffer && hPrevBuffer != INVALID_HANDLE_VALUE)
		//	SetConsoleActiveScreenBuffer(hPrevBuffer);
		SetStdHandle(STD_OUTPUT_HANDLE, hPrevOut);
		SetStdHandle(STD_ERROR_HANDLE, hPrevErr);
		CloseHandle(hBuffer);
	}

	std::wstring GetTopString() const
	{
		std::wstring result;
		result.resize(80);
		DWORD readChars = 0;
		if (!ReadConsoleOutputCharacterW(hBuffer, &(result[0]), 80, COORD{ 0, 0 }, &readChars))
		{
			OutputDebugStringW(L"GetTopString is empty\n");
			return {};
		}
		result.resize(readChars);
		const auto pos = result.find_last_not_of(L' ');
		if (pos == std::string::npos)
		{
			OutputDebugStringW(L"GetTopString is empty\n");
			return {};
		}
		result.resize(pos + 1);
		OutputDebugStringW((L"GetTopString=`" + result + L"`\n").c_str());
		return result;
	}

	static std::wstring GetPrevString()
	{
		// ReSharper disable once CppLocalVariableMayBeConst
		HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi{};
		if (!GetConsoleScreenBufferInfo(hStdOut, &csbi))
		{
			throw OutputWasRedirected("GetConsoleScreenBufferInfo failed, error=" + std::to_string(GetLastError()));
		}
		if (csbi.dwCursorPosition.Y <= 0 || csbi.dwCursorPosition.X != 0)
		{
			throw std::runtime_error("Unexpected cursor position={" + std::to_string(csbi.dwCursorPosition.X) + "," + std::to_string(csbi.dwCursorPosition.Y) + "}");
		}
		std::wstring result;
		result.resize(80);
		DWORD readChars = 0;
		if (!ReadConsoleOutputCharacterW(hStdOut, &(result[0]), 80, COORD{ 0, csbi.dwCursorPosition.Y - 1}, &readChars))
		{
			OutputDebugStringW(L"GetPrevString is empty\n");
			return {};
		}
		result.resize(readChars);
		const auto pos = result.find_last_not_of(L' ');
		if (pos == std::string::npos)
		{
			OutputDebugStringW(L"GetPrevString is empty\n");
			return {};
		}
		result.resize(pos + 1);
		OutputDebugStringW((L"GetPrevString=`" + result + L"`\n").c_str());
		return result;
	}

	ConsoleOutputCapture(const ConsoleOutputCapture&) = delete;
	ConsoleOutputCapture(ConsoleOutputCapture&&) = delete;
	ConsoleOutputCapture& operator=(const ConsoleOutputCapture&) = delete;
	ConsoleOutputCapture& operator=(ConsoleOutputCapture&&) = delete;

private:
	HANDLE hBuffer = nullptr;
	//HANDLE hPrevBuffer = nullptr;
	HANDLE hPrevOut = nullptr;
	HANDLE hPrevErr = nullptr;
	//bool changed = false;
};


class ServerDllTest : public testing::Test
{
protected:
	// ReSharper disable once CppInconsistentNaming
	MModule ConEmuCD;
	wchar_t szConEmuCD[MAX_PATH] = L"";
	wchar_t szResult[64] = L"";
	HWND hCurrentConEmu = nullptr;
	HWND hTopConEmu = nullptr;
	HWND hByConEmuHWND = nullptr;

public:
	void SetUp() override
	{
		ConEmuCD = LoadServerDll();
		EXPECT_NE(nullptr, wcsstr(szConEmuCD, ConEmuCD_DLL_3264));
		if (!ConEmuCD.IsValid())
		{
			FAIL() << L"Failed to load " << ConEmuCD_DLL_3264;
		}
		// ReSharper disable once CppLocalVariableMayBeConst
		HMODULE hConEmuCD = GetModuleHandle(ConEmuCD_DLL_3264);
		EXPECT_NE(nullptr, hConEmuCD);
		EXPECT_EQ(hConEmuCD, static_cast<HMODULE>(ConEmuCD));

		// ReSharper disable once CppLocalVariableMayBeConst
		hCurrentConEmu = GetCurrentConEmuRoot();
		// ReSharper disable once CppLocalVariableMayBeConst
		hTopConEmu = FindTopConEmu();
		cdbg() << "Test started " << (hCurrentConEmu ? "inside" : "outside") << " of ConEmu"
			<< ((hTopConEmu && hTopConEmu == hCurrentConEmu) ? ", this is top instance" : "")
			<< ((hTopConEmu && hTopConEmu != hCurrentConEmu) ? ", other ConEmu exists" : "")
			<< (!hTopConEmu ? ", other ConEmu doesn't exist" : "")
			<< std::endl;
		hByConEmuHWND = hCurrentConEmu ? hCurrentConEmu : hTopConEmu;
	}

	void TearDown() override
	{
		ConEmuCD.Free();
		// ReSharper disable once CppLocalVariableMayBeConst
		HMODULE hConEmuCD = GetModuleHandle(ConEmuCD_DLL_3264);
		EXPECT_EQ(nullptr, hConEmuCD);
	}

protected:
	LPCWSTR GetEnvVar()
	{
		const auto rc = GetEnvironmentVariable(CEGUIMACRORETENVVAR, szResult, LODWORD(std::size(szResult)));
		if (rc == 0 && ::GetLastError() == ERROR_ENVVAR_NOT_FOUND)
			return nullptr;
		return szResult;
	};

	MModule LoadServerDll()
	{
		if (!GetModuleFileName(nullptr, szConEmuCD, LODWORD(std::size(szConEmuCD))))
			return {};
		auto* slash = const_cast<wchar_t*>(PointToName(szConEmuCD));
		if (!slash)
			return {};
		wcscpy_s(slash, std::size(szConEmuCD) - (slash - szConEmuCD), L"\\ConEmu\\" ConEmuCD_DLL_3264);
		return MModule(szConEmuCD);
	}

	static HWND GetRealConsoleWindow()
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

	static HWND GetCurrentConEmuRoot()
	{
		CESERVER_CONSOLE_MAPPING_HDR srvMap{};
		if (!LoadSrvMapping(GetRealConsoleWindow(), srvMap))
			return nullptr;
		if (!srvMap.hConEmuWndDc || !IsWindow(srvMap.hConEmuWndDc))
			return nullptr;
		return srvMap.hConEmuRoot;
	}

	static BOOL CALLBACK FindTopGui(HWND hWnd, LPARAM lParam)
	{
		HWND* pResult = reinterpret_cast<HWND*>(lParam);
		wchar_t szClass[MAX_PATH];
		if (GetClassName(hWnd, szClass, countof(szClass)) < 1)
			return TRUE; // continue search

		const bool bConClass = isConsoleClass(szClass);
		if (bConClass)
			return TRUE; // continue search

		const bool bGuiClass = (lstrcmp(szClass, VirtualConsoleClassMain) == 0);
		if (!bGuiClass && !bConClass)
			return TRUE; // continue search

		*pResult = hWnd;
		return FALSE; // Found! stop search
	}

	static HWND FindTopConEmu()
	{
		HWND hConEmu = nullptr;
		EnumWindows(FindTopGui, reinterpret_cast<LPARAM>(&hConEmu));
		return hConEmu;
	}

};

TEST_F(ServerDllTest, RunGuiMacro_CMD_NoPrint)
{
	const ConsoleOutputCapture capture;

	ConsoleMain3_t consoleMain3 = nullptr;
	EXPECT_TRUE(ConEmuCD.GetProcAddress(FN_CONEMUCD_CONSOLE_MAIN_3_NAME, consoleMain3));
	if (!consoleMain3)
	{
		return; // nothing to check more, test failed
	}

	{
		SetEnvironmentVariable(CEGUIMACRORETENVVAR, nullptr);

		const auto macroRc = consoleMain3(ConsoleMainMode::GuiMacro, L"-GuiMacro IsConEmu");
		if (!hCurrentConEmu)
		{
			EXPECT_EQ(CERR_GUIMACRO_FAILED, macroRc) << "isConEmu==false";
			EXPECT_EQ(nullptr, GetEnvVar()) << "isConEmu==false";
		}
		else
		{
			EXPECT_EQ(0, macroRc) << "isConEmu==true";
			EXPECT_TRUE(capture.GetTopString().empty());
			EXPECT_STREQ(L"Yes", GetEnvVar()) << "isConEmu==true";
		}
	}

	// ReSharper disable once CppLocalVariableMayBeConst
	if (hByConEmuHWND)
	{
		SetEnvironmentVariable(CEGUIMACRORETENVVAR, nullptr);

		wchar_t command[64] = L"";
		swprintf_s(command, L"-GuiMacro:x%p IsConEmu", hByConEmuHWND);

		const auto macroRc = consoleMain3(ConsoleMainMode::GuiMacro, command);
		EXPECT_EQ(0, macroRc) << "hByConEmuHWND!=nullptr";
		EXPECT_TRUE(capture.GetTopString().empty());
		EXPECT_STREQ(L"Yes", GetEnvVar()) << "hByConEmuHWND!=nullptr";
	}
}

TEST_F(ServerDllTest, RunGuiMacro_CMD_Print)
{
	const ConsoleOutputCapture capture;

	ConsoleMain3_t consoleMain3 = nullptr;
	EXPECT_TRUE(ConEmuCD.GetProcAddress(FN_CONEMUCD_CONSOLE_MAIN_3_NAME, consoleMain3));
	if (!consoleMain3)
	{
		return; // nothing to check more, test failed
	}

	if (hByConEmuHWND)
	{
		SetEnvironmentVariable(CEGUIMACRORETENVVAR, nullptr);

		wchar_t command[64] = L"";
		swprintf_s(command, L"-GuiMacro:x%p IsConEmu", hByConEmuHWND);

		const auto macroRc = consoleMain3(ConsoleMainMode::Normal, command);
		EXPECT_EQ(0, macroRc) << "hByConEmuHWND!=nullptr";
		EXPECT_EQ(std::wstring(L"Yes"), capture.GetTopString()) << "should print to console";
		EXPECT_STREQ(L"Yes", GetEnvVar()) << "hByConEmuHWND!=nullptr";
	}
}

TEST_F(ServerDllTest, RunGuiMacro_API)
{
	const ConsoleOutputCapture capture;

	GuiMacro_t guiMacro = nullptr;
	EXPECT_TRUE(ConEmuCD.GetProcAddress(FN_CONEMUCD_GUIMACRO_NAME, guiMacro));
	if (!guiMacro)
	{
		return; // nothing to check more, test failed
	}


	wchar_t szInstance[128] = L"";

	// Expected to be execute in CURRENT ConEmu instance
	{
		SetEnvironmentVariable(CEGUIMACRORETENVVAR, nullptr);
		const auto macroRc = guiMacro(nullptr, L"IsConEmu", nullptr);
		if (!hCurrentConEmu)
		{
			EXPECT_EQ(CERR_GUIMACRO_FAILED, macroRc) << "isConEmu==false";
			EXPECT_EQ(nullptr, GetEnvVar()) << "isConEmu==false";
		}
		else
		{
			EXPECT_EQ(CERR_GUIMACRO_SUCCEEDED, macroRc) << "isConEmu==true";
			EXPECT_TRUE(capture.GetTopString().empty());
			EXPECT_STREQ(L"Yes", GetEnvVar()) << "isConEmu==true";
		}
	}

	// Execute by HWND instance
	if (!hByConEmuHWND)
	{
		cdbg() << "Skipping ByHWND test" << std::endl;
	}
	else
	{
		SetEnvironmentVariable(CEGUIMACRORETENVVAR, nullptr);
		swprintf_s(szInstance, L"0x%p", static_cast<void*>(hTopConEmu));
		const auto macroRc = guiMacro(szInstance, L"IsConEmu", nullptr);
		EXPECT_EQ(CERR_GUIMACRO_SUCCEEDED, macroRc) << "hByConEmuHWND!=nullptr";
		EXPECT_TRUE(capture.GetTopString().empty());
		EXPECT_STREQ(L"Yes", GetEnvVar()) << "hByConEmuHWND!=nullptr";
	}

	// With BSTR result
	if (hByConEmuHWND)
	{
		SetEnvironmentVariable(CEGUIMACRORETENVVAR, nullptr);
		BSTR result = nullptr;
		const auto macroRc = guiMacro(szInstance, L"IsConEmu", &result);
		EXPECT_EQ(CERR_GUIMACRO_SUCCEEDED, macroRc) << "hByConEmuHWND!=nullptr";
		EXPECT_EQ(nullptr, GetEnvVar()) << "hByConEmuHWND!=nullptr";
		EXPECT_TRUE(capture.GetTopString().empty());
		EXPECT_STREQ(L"Yes", result);
		SysFreeString(result);
	}

	// (-1)
	if (hByConEmuHWND)
	{
		SetEnvironmentVariable(CEGUIMACRORETENVVAR, nullptr);
		const auto macroRc = guiMacro(szInstance, L"IsConEmu", static_cast<BSTR*>(INVALID_HANDLE_VALUE));
		EXPECT_EQ(CERR_GUIMACRO_SUCCEEDED, macroRc) << "hByConEmuHWND!=nullptr";
		EXPECT_TRUE(capture.GetTopString().empty());
		EXPECT_EQ(nullptr, GetEnvVar()) << "hByConEmuHWND!=nullptr";
	}
}

TEST_F(ServerDllTest, RunCommand)
{
	/*
	cdbg() << "Sleeping 15 sec" << std::endl;
	const auto start = GetTickCount();
	while (!IsDebuggerPresent())
	{
		Sleep(100);
		const auto delay = GetTickCount() - start;
		if (delay >= 15000)
			break;
	}
	*/

	ConsoleMain3_t consoleMain3 = nullptr;
	EXPECT_TRUE(ConEmuCD.GetProcAddress(FN_CONEMUCD_CONSOLE_MAIN_3_NAME, consoleMain3));
	if (!consoleMain3)
	{
		return; // nothing to check more
	}

	const MHandle hStdOut(GetStdHandle(STD_OUTPUT_HANDLE));
	CONSOLE_SCREEN_BUFFER_INFO csbi{};
	if (!GetConsoleScreenBufferInfo(hStdOut, &csbi))
	{
		cdbg() << "GetConsoleScreenBufferInfo failed, error=" + std::to_string(GetLastError()) << std::endl;
	}

	std::atomic_bool guardStop{ false };
	auto hungGuardFn = [&guardStop]()
	{
		const auto started = std::chrono::steady_clock::now();
		const std::chrono::seconds hangDelay{ 60 };
		bool hang = false;
		while (!guardStop.load() && !hang)
		{
			hang = (std::chrono::steady_clock::now() - started) > hangDelay;
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
		}
		if (!hang)
			return;

		MToolHelpProcess processes;
		PROCESSENTRY32W pi{};
		if (processes.Find([](const PROCESSENTRY32W& item)
			{
				const auto* itemName = PointToName(item.szExeFile);
				if (!itemName)
					return false;
				if (item.th32ParentProcessID != GetCurrentProcessId())
					return false;
				const int iCmp = lstrcmpi(itemName, L"cmd.exe");
				return (iCmp == 0);
			}, pi))
		{
			const MHandle process(OpenProcess(MY_PROCESS_ALL_ACCESS | PROCESS_TERMINATE | SYNCHRONIZE, false, pi.th32ProcessID), CloseHandle);
			cdbg() << "Terminating cmd.exe PID=" << pi.th32ProcessID << std::endl;
			if (!TerminateProcess(process, 255))
				cdbg() << "TerminateProcess PID=" << pi.th32ProcessID << " failed, code=" << GetLastError() << std::endl;
		}
	};

	CEStr testExe;
	GetModulePathName(nullptr, testExe);
	auto* namePtr = const_cast<wchar_t*>(PointToName(testExe.c_str()));
	if (namePtr && namePtr > testExe.c_str())
	{
		*(namePtr - 1) = 0;
		SetEnvironmentVariableW(ENV_CONEMUDIR_VAR_W, testExe);
		const CEStr baseDir(testExe, L"\\ConEmu");
		SetEnvironmentVariableW(ENV_CONEMUBASEDIR_VAR_W, baseDir);
	}
	else
	{
		cdbg() << "GetModulePathName failed, name is null" << std::endl;
	}
	SetEnvironmentVariableW(ENV_CONEMUHWND_VAR_W, L"0");
	SetEnvironmentVariableW(ENV_CONEMUDRAW_VAR_W, L"0");
	SetEnvironmentVariableW(ENV_CONEMUBACK_VAR_W, L"0");
	SetEnvironmentVariableW(ENV_CONEMUWORKDIR_VAR_W, L"C:\\");

	auto executeRun = [&](const wchar_t* command, const int expectedRc, const wchar_t* expectedOutput)
	{
		guardStop.store(false);
		std::thread hangGuard2(hungGuardFn);

		const auto cmdRc1 = consoleMain3(ConsoleMainMode::Comspec, command);
		EXPECT_EQ(expectedRc, cmdRc1) << L"command=" << command;
		if (expectedOutput)
		{
			try
			{
				const auto prevString = ConsoleOutputCapture::GetPrevString();
				EXPECT_EQ(std::wstring(expectedOutput), prevString) << L"command=" << command;
			}
			catch (const OutputWasRedirected& ex)
			{
				cdbg() << "Skipping cmd echo output test: " << ex.what() << std::endl;
			}
			catch (const std::exception& ex)
			{
				FAIL() << L"command=" << command << "; exception=" << ex.what();
			}
		}

		guardStop.store(true);
		hangGuard2.join();
	};

	executeRun(L"-c cmd.exe /c echo echo from cmd.exe", 0, L"echo from cmd.exe");

	executeRun(L"-c \"%ConEmuBaseDir%\\cecho.cmd\" \"Hello\" \"World\"", 0, L"Hello");

	executeRun(L"-c \"\"%ConEmuBaseDir%\\" ConEmuC_EXE_3264 L"\" -echo Hello World\"", 0, L"Hello World");

	executeRun(L"-c cmd.exe /c exit 3", 3, nullptr);
}
