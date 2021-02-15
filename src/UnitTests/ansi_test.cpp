
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

#include "../common/defines.h"
#include "gtest.h"
#include "../common/ConEmuCheck.h"
#include "../common/WConsole.h"
#include "../common/MHandle.h"
#include "../common/WObjects.h"
#include "../common/EnvVar.h"
#include "../common/MWnd.h"

HWND WINAPI GetRealConsoleWindow();

namespace conemu {
namespace tests {
namespace {
DWORD GetMode(const MHandle& h)
{
	DWORD mode = 0;
	EXPECT_TRUE(GetConsoleMode(h, &mode));
	return mode;
}

void Write(const MHandle& hConOut, const std::wstring& line)
{
	DWORD written = 0;
	WriteConsoleW(hConOut, line.c_str(), static_cast<uint32_t>(line.length()), &written, nullptr);
}

bool IsConEmuMode(const MHandle& hConOut, const std::string& testName)
{
	if (!GetRealConsoleWindow())
	{
		cdbg() << "Test " << testName << " is not functional, GetRealConsoleWindow is nullptr" << std::endl;
		return false;
	}
	return true;
}

bool TestWrite(const MHandle& hConOut, const std::wstring& expected)
{
	const DWORD outFlags = GetMode(hConOut);
	wchar_t buffer[255] = L"";
	swprintf_s(buffer, 255, L"Current output console mode: 0x%08X (%s %s)\r\n", outFlags,
		(outFlags & ENABLE_VIRTUAL_TERMINAL_PROCESSING) ? L"ENABLE_VIRTUAL_TERMINAL_PROCESSING" : L"!ENABLE_VIRTUAL_TERMINAL_PROCESSING",
		(outFlags & DISABLE_NEWLINE_AUTO_RETURN) ? L"DISABLE_NEWLINE_AUTO_RETURN" : L"!DISABLE_NEWLINE_AUTO_RETURN");
	Write(hConOut, buffer);

	SetConsoleTextAttribute(hConOut, 14);
	Write(hConOut, L"Print(AAA\\nBBB\\r\\nCCC)\r\n");
	SetConsoleTextAttribute(hConOut, 7);

	Write(hConOut, L"AAA\nBBB\r\nCCC\r\n");

	bool result = true;
	if (!expected.empty())
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi{};
		GetConsoleScreenBufferInfo(hConOut, &csbi);
		CHAR_INFO readBuffer[6 * 3] = {};
		SMALL_RECT rcRead = { 0, static_cast<SHORT>(csbi.dwCursorPosition.Y - 3), 5, static_cast<SHORT>(csbi.dwCursorPosition.Y - 1) };
		ReadConsoleOutputW(hConOut, readBuffer, COORD{ 6, 3 }, COORD{ 0, 0 }, &rcRead);
		std::wstring data;
		for (const auto& ci : readBuffer)
		{
			data += ci.Char.UnicodeChar;
		}
		if (data == expected)
		{
			SetConsoleTextAttribute(hConOut, 10);
			Write(hConOut, L"OK\r\n");
		}
		else
		{
			SetConsoleTextAttribute(hConOut, 12);
			Write(hConOut, L"FAILED: '" + data + L"' != '" + expected + L"'\r\n");
			result = false;
		}
		SetConsoleTextAttribute(hConOut, 7);
	}
	return result;
}
}

int RunLineFeedTest()
{
	const MHandle hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!IsConEmuMode(hConOut, __FUNCTION__))
		return 0;

	int result = 0;
	const DWORD outFlags = GetMode(hConOut);
	const bool isDefaultMode = (outFlags == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT));
	CONSOLE_SCREEN_BUFFER_INFO csbi{};
	GetConsoleScreenBufferInfo(hConOut, &csbi);

	SetConsoleTextAttribute(hConOut, 7);

	if (!TestWrite(hConOut, isDefaultMode ? L"AAA   " L"BBB   " L"CCC   " : L""))
		result = 1;

	if (!isDefaultMode)
	{
		SetConsoleMode(hConOut, (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT));
		if (!TestWrite(hConOut, L"AAA   " L"BBB   " L"CCC   "))
			result = 1;
	}

	SetConsoleMode(hConOut, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
	if (!TestWrite(hConOut, L"AAA   " L"   BBB" L"CCC   "))
		result = 1;

	SetConsoleMode(hConOut, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	if (!TestWrite(hConOut, L"AAA   " L"BBB   " L"CCC   "))
		result = 1;

	SetConsoleMode(hConOut, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | DISABLE_NEWLINE_AUTO_RETURN);
	if (!TestWrite(hConOut, L"AAA   " L"   BBB" L"CCC   "))
		result = 1;

	SetConsoleMode(hConOut, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
	if (!TestWrite(hConOut, L"AAA   " L"   BBB" L"CCC   "))
		result = 1;

	SetConsoleMode(hConOut, outFlags);
	SetConsoleTextAttribute(hConOut, csbi.wAttributes);
	return result;
}

int RunLineFeedTestXTerm()
{
	const MHandle hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!IsConEmuMode(hConOut, __FUNCTION__))
		return 0;

	int result = 0;
	const DWORD outFlags = GetMode(hConOut);
	// #TODO remove EXPECT? we have not initialized gtest
	EXPECT_EQ((outFlags & (ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN)), 0);
	CONSOLE_SCREEN_BUFFER_INFO csbi{};
	GetConsoleScreenBufferInfo(hConOut, &csbi);

	Write(hConOut, L"\x1B]9;10\x07");

	Sleep(10 * 1000);

	if (!TestWrite(hConOut, L"AAA   " L"   BBB" L"CCC   "))
		result = 1;

	Sleep(10 * 1000);

	SetConsoleTextAttribute(hConOut, csbi.wAttributes);
	return result;
}

int RunLineFeedTestParent()
{
	const MHandle hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!IsConEmuMode(hConOut, __FUNCTION__))
		return 0;

	const DWORD outFlags = GetMode(hConOut);
	SetConsoleMode(hConOut, (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN));

	int result = 0;
	const DWORD testOutFlags = GetMode(hConOut);
	// #TODO remove EXPECT? we have not initialized gtest
	EXPECT_EQ((testOutFlags & (ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN)), (ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN));
	CONSOLE_SCREEN_BUFFER_INFO csbi{};
	GetConsoleScreenBufferInfo(hConOut, &csbi);

	STARTUPINFOW si{}; si.cb = sizeof(si);
	PROCESS_INFORMATION pi{};
	CEStr testExe;
	GetModulePathName(nullptr, testExe);
	const CEStr envCmdLine(L"\"", testExe, L"\" RunLineFeedTestChild");
	const CEStr cmdLine(ExpandEnvStr(envCmdLine));
	const auto created = CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, false, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi);
	if (!created)
	{
		const DWORD errCode = GetLastError();
		EXPECT_TRUE(created) << "create process failed, code=" << errCode << ", cmd=" << cmdLine.c_str() << std::endl;
		result = 1;
	}
	else
	{
		const auto wait = WaitForSingleObject(pi.hProcess, 1000 * 180);
		if (wait != WAIT_OBJECT_0)
		{
			EXPECT_EQ(wait, WAIT_OBJECT_0) << "process was not finished in time: " << cmdLine.c_str() << std::endl;
			TerminateProcess(pi.hProcess, 255);
			result = 2;
		}
		else
		{
			DWORD exitCode = 255;
			EXPECT_TRUE(GetExitCodeProcess(pi.hProcess, &exitCode));
			EXPECT_EQ(0, exitCode);
			result = exitCode;
		}
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	SetConsoleMode(hConOut, outFlags);
	SetConsoleTextAttribute(hConOut, csbi.wAttributes);
	return result;
}

int RunLineFeedTestChild()
{
	const MHandle hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!IsConEmuMode(hConOut, __FUNCTION__))
		return 0;

	int result = 0;
	const DWORD outFlags = GetMode(hConOut);
	EXPECT_EQ((outFlags & (ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN)), (ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN));
	CONSOLE_SCREEN_BUFFER_INFO csbi{};
	GetConsoleScreenBufferInfo(hConOut, &csbi);

	Sleep(1 * 1000);

	if (!TestWrite(hConOut, L"AAA   " L"   BBB" L"CCC   "))
		result = 1;

	Sleep(1 * 1000);

	SetConsoleTextAttribute(hConOut, csbi.wAttributes);
	return result;
}

// Set xterm mode for output and exit
int RunXTermTestChild()
{
	const MHandle hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!IsConEmuMode(hConOut, __FUNCTION__))
		return 0;

	const DWORD dwMode = GetMode(hConOut);
	EXPECT_TRUE(SetConsoleMode(hConOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING));
	return 0;
}

// Set xterm mode for output and run child process (which does the same thing).
// We should be still in the xterm mode on child exit.
int RunXTermTestParent()
{
	const MWnd realConWnd = GetRealConsoleWindow();
	if (!realConWnd)
	{
		cdbg() << "Test " << __FUNCTION__ << " is not functional, GetRealConsoleWindow is nullptr" << std::endl;
		return 0;
	}
	CESERVER_CONSOLE_MAPPING_HDR srvMap{};
	if (!LoadSrvMapping(realConWnd, srvMap) || !srvMap.hConEmuWndDc)
	{
		cdbg() << "Test " << __FUNCTION__ << " is not functional, LoadSrvMapping failed" << std::endl;
		return 0;
	}

	int result = 0;

	const MHandle hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	const MHandle hIn = GetStdHandle(STD_INPUT_HANDLE);

	const DWORD inModeOrig = GetMode(hIn);
	const DWORD outModeOrig = GetMode(hConOut);

	// Set output console mode
	DWORD outMode = outModeOrig;
	outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(hConOut, outMode);

	// Set input console mode
	DWORD inMode = inModeOrig;
	inMode &= ~ENABLE_LINE_INPUT;
	inMode &= ~ENABLE_ECHO_INPUT;
	inMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
	SetConsoleMode(hIn, inMode);

	// Start another process which sets xterm mode enabled
	if (result == 0)
	{
		STARTUPINFOW si{};
		PROCESS_INFORMATION pi{};
		si.cb = sizeof(si);
		CEStr testExe;
		GetModulePathName(nullptr, testExe);
		const CEStr envCmdLine("\"%ConEmuBaseDir%\\" ConEmuC_EXE_3264 L"\" -std -c \"", testExe, L"\" RunXTermTestChild");
		const CEStr cmdLine(ExpandEnvStr(envCmdLine));
		const bool created = CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
		if (!created)
		{
			EXPECT_TRUE(created) << "command: " << cmdLine.c_str() << std::endl;
			result = 1;
		}
		else
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
			Sleep(1000);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}

	if (result == 0)
	{
		DWORD mode = 0;
		EXPECT_TRUE(GetConsoleMode(hIn, &mode));
		if (mode != inMode)
		{
			EXPECT_EQ(mode, inMode);
			EXPECT_TRUE(SetConsoleMode(hIn, inMode));
			result = 2;
		}
		EXPECT_TRUE(GetConsoleMode(hConOut, &mode));
		if (mode != outMode)
		{
			EXPECT_EQ(mode, outMode);
			EXPECT_TRUE(SetConsoleMode(hConOut, outMode));
			result = 3;
		}
	}

	// Request for AppKeys mode (cursor keys are sent with `ESC O`)
	if (result == 0)
	{
		DWORD written = 0;
		const bool writeRc = WriteConsoleA(hConOut, "\033[?1h", 5, &written, nullptr);
		if (!writeRc || !written)
		{
			EXPECT_TRUE(writeRc);
			EXPECT_EQ(written, 5);
			result = 4;
		}
	}

	// Clean the input buffer
	if (result == 0)
	{
		INPUT_RECORD ir[32]{}; DWORD count = 0;
		while (PeekConsoleInputW(hIn, ir, 32, &count) && count)
		{
			if (!ReadConsoleInputW(hIn, ir, count, &count) || !count)
				break;
		}
	}

	// Check what is sent through input queue
	if (result == 0)
	{
		PostMessage(srvMap.hConEmuWndDc, WM_KEYDOWN, VK_UP, 0);
		PostMessage(srvMap.hConEmuWndDc, WM_KEYUP, VK_UP, (1U << 31) | (1U << 30));

		char buffer[32] = "";
		DWORD readCount = 0;
		EXPECT_TRUE(ReadConsoleA(hIn, buffer, sizeof(buffer), &readCount, nullptr));
		EXPECT_EQ(readCount, 3);
		if (strcmp(buffer, "\x1b\x4f\x41") != 0)
		{
			EXPECT_STREQ(buffer, "\x1b\x4f\x41");
			result = 5;
		}
	}

	SetConsoleMode(hIn, inModeOrig);
	SetConsoleMode(hConOut, outModeOrig);

	return result;
}
}
}

TEST(Ansi, CheckLineFeed)
{
	const MHandle hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!conemu::tests::IsConEmuMode(hConOut, __FUNCTION__))
		return;

	conemu::tests::InitConEmuPathVars();

	STARTUPINFOW si{}; si.cb = sizeof(si);
	PROCESS_INFORMATION pi{};
	CEStr testExe;
	GetModulePathName(nullptr, testExe);
	const CEStr envCmdLine("\"%ConEmuBaseDir%\\" ConEmuC_EXE_3264 L"\" -std -c \"", testExe, L"\" RunLineFeedTest");
	const CEStr cmdLine(ExpandEnvStr(envCmdLine));
	const auto created = CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, false, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi);
	if (!created)
	{
		const DWORD errCode = GetLastError();
		EXPECT_TRUE(created) << "create process failed, code=" << errCode << ", cmd=" << cmdLine.c_str() << std::endl;
		return;
	}

	const auto wait = WaitForSingleObject(pi.hProcess, 1000 * 180);
	if (wait != WAIT_OBJECT_0)
	{
		EXPECT_EQ(wait, WAIT_OBJECT_0) << "process was not finished in time: " << cmdLine.c_str() << std::endl;
		TerminateProcess(pi.hProcess, 255);
	}
	else
	{
		DWORD exitCode = 255;
		EXPECT_TRUE(GetExitCodeProcess(pi.hProcess, &exitCode));
		EXPECT_EQ(0, exitCode);
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

TEST(Ansi, CheckLineFeedXTerm)
{
	const MHandle hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!conemu::tests::IsConEmuMode(hConOut, __FUNCTION__))
		return;

	conemu::tests::InitConEmuPathVars();

	DWORD outFlags = 0; GetConsoleMode(hConOut, &outFlags);
	SetConsoleMode(hConOut, (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT));

	STARTUPINFOW si{}; si.cb = sizeof(si);
	PROCESS_INFORMATION pi{};
	CEStr testExe;
	GetModulePathName(nullptr, testExe);
	const CEStr envCmdLine("\"%ConEmuBaseDir%\\" ConEmuC_EXE_3264 L"\" -std -c \"", testExe, L"\" RunLineFeedTestXTerm");
	const CEStr cmdLine(ExpandEnvStr(envCmdLine));
	const auto created = CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, false, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi);
	if (!created)
	{
		const DWORD errCode = GetLastError();
		EXPECT_TRUE(created) << "create process failed, code=" << errCode << ", cmd=" << cmdLine.c_str() << std::endl;
		return;
	}

	const auto wait = WaitForSingleObject(pi.hProcess, 1000 * 180);
	if (wait != WAIT_OBJECT_0)
	{
		EXPECT_EQ(wait, WAIT_OBJECT_0) << "process was not finished in time: " << cmdLine.c_str() << std::endl;
		TerminateProcess(pi.hProcess, 255);
	}
	else
	{
		DWORD exitCode = 255;
		EXPECT_TRUE(GetExitCodeProcess(pi.hProcess, &exitCode));
		EXPECT_EQ(0, exitCode);
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	SetConsoleMode(hConOut, outFlags);
}

TEST(Ansi, CheckLineFeedChild)
{
	const MHandle hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!conemu::tests::IsConEmuMode(hConOut, __FUNCTION__))
		return;

	conemu::tests::InitConEmuPathVars();

	DWORD outFlags = 0; GetConsoleMode(hConOut, &outFlags);

	STARTUPINFOW si{}; si.cb = sizeof(si);
	PROCESS_INFORMATION pi{};
	CEStr testExe;
	GetModulePathName(nullptr, testExe);
	const CEStr envCmdLine("\"%ConEmuBaseDir%\\" ConEmuC_EXE_3264 L"\" -std -c \"", testExe, L"\" RunLineFeedTestParent");
	const CEStr cmdLine(ExpandEnvStr(envCmdLine));
	const auto created = CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, false, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi);
	if (!created)
	{
		const DWORD errCode = GetLastError();
		EXPECT_TRUE(created) << "create process failed, code=" << errCode << ", cmd=" << cmdLine.c_str() << std::endl;
		return;
	}

	const auto wait = WaitForSingleObject(pi.hProcess, 1000 * 180);
	if (wait != WAIT_OBJECT_0)
	{
		EXPECT_EQ(wait, WAIT_OBJECT_0) << "process was not finished in time: " << cmdLine.c_str() << std::endl;
		TerminateProcess(pi.hProcess, 255);
	}
	else
	{
		DWORD exitCode = 255;
		EXPECT_TRUE(GetExitCodeProcess(pi.hProcess, &exitCode));
		EXPECT_EQ(0, exitCode);
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	SetConsoleMode(hConOut, outFlags);
}

TEST(Ansi, CheckXTermInChain)
{
	const MHandle hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!conemu::tests::IsConEmuMode(hConOut, __FUNCTION__))
		return;

	STARTUPINFOW si{};
	PROCESS_INFORMATION pi{};
	si.cb = sizeof(si);
	CEStr testExe;
	GetModulePathName(nullptr, testExe);
	const CEStr envCmdLine("\"%ConEmuBaseDir%\\" ConEmuC_EXE_3264 L"\" -std -c \"", testExe, L"\" RunXTermTestParent");
	const CEStr cmdLine(ExpandEnvStr(envCmdLine));
	const bool created = CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
	if (!created)
	{
		EXPECT_TRUE(created) << "command: " << cmdLine.c_str() << std::endl;
	}
	else
	{
		WaitForSingleObject(pi.hProcess, INFINITE);

		DWORD exitCode = 999;
		EXPECT_TRUE(GetExitCodeProcess(pi.hProcess, &exitCode));
		EXPECT_EQ(0, exitCode);

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}
