
/*
Copyright (c) 2021-present Maximus5
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

#include <Windows.h>
#include <string>
#include <tuple>

static int exeCounter = 1;

void write(const std::wstring& line)
{
	auto* hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD written = 0;
	WriteConsoleW(hConOut, line.c_str(), line.length(), &written, nullptr);
}

void create(const wchar_t* appName, const std::wstring& cmdLine)
{
	++exeCounter;
	write(L"#" + std::to_wstring(exeCounter) + L" CreateProcessW: name={" + (appName ? appName : L"<null>") + L"}, param={" + cmdLine + L"}\n");

	STARTUPINFOW si{};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi{};
	if (!CreateProcessW(appName, const_cast<wchar_t*>(cmdLine.c_str()), nullptr, nullptr, false, CREATE_DEFAULT_ERROR_MODE, nullptr, nullptr, &si, &pi))
	{
		const auto err = GetLastError();
		write(L"-- Failed, code=" + std::to_wstring(err) + L"\n\n");
		return;
	}

	const auto wait = WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD exitCode = 9999;
	GetExitCodeProcess(pi.hProcess, &exitCode);
	write(L"-- Succeeded, wait=" + std::to_wstring(wait) + L", exitCode=" + std::to_wstring(exitCode) + L"\n\n");
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

void shell(const std::wstring& appName, const std::wstring& cmdLine)
{
	++exeCounter;
	write(L"#" + std::to_wstring(exeCounter) + L" ShellExecuteExW: name={" + appName + L"}, param={" + cmdLine + L"}\n");

	SHELLEXECUTEINFOW sei{};
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
	sei.lpVerb = L"open";
	sei.lpFile = appName.c_str();
	sei.lpParameters = cmdLine.c_str();

	if (!ShellExecuteExW(&sei))
	{
		const auto err = GetLastError();
		write(L"-- Failed, code=" + std::to_wstring(err) + L"\n\n");
		return;
	}

	const auto wait = WaitForSingleObject(sei.hProcess, INFINITE);
	DWORD exitCode = 9999;
	GetExitCodeProcess(sei.hProcess, &exitCode);
	write(L"-- Succeeded, wait=" + std::to_wstring(wait) + L", exitCode=" + std::to_wstring(exitCode) + L"\n\n");
	CloseHandle(sei.hProcess);
}

std::pair<std::wstring, std::wstring> get_dir_name()
{
	wchar_t module[MAX_PATH] = L"";
	GetModuleFileNameW(nullptr, module, MAX_PATH);
	auto* slash = wcsrchr(module, L'\\');
	std::wstring name;
	if (slash)
	{
		name = slash + 1;
		*(slash + 1) = 0;
	}
	else
	{
		name = module;
	}
	return {module, name};
}

std::wstring get_cmd()
{
	wchar_t cmd[MAX_PATH] = L"";
	GetEnvironmentVariableW(L"ComSpec", cmd, MAX_PATH);
	return cmd;
}

int main()
{
	const auto [dir, name] = get_dir_name();

	if (name == L"cmd_printer.exe")
	{
		write(std::wstring(L"Module: {") + dir + name + L"}\n");
		write(std::wstring(L"Command: {") + GetCommandLineW() + L"}\n");
		write(std::wstring(L"\x1B]9;3;\"OK\"\x1B\\\n"));
		return 0;
	}

	SetCurrentDirectoryW(dir.c_str());

	const auto cmd = get_cmd();

	write(std::wstring(L"#1 Our command line: {") + GetCommandLineW() + L"}\n\n");

	create(nullptr, L"\"" + dir + L"1\\cmd_printer.exe\"" + L" -new_console \"some arguments\" ");
	create(nullptr, L"\"" + dir + L"1 @\\cmd_printer.exe\"" + L" -new_console \"some arguments\" ");
	create(nullptr, L"\"" + dir + L"!1&@\\cmd_printer.exe\"" + L" -new_console \"some arguments\" ");

	create(nullptr, L"\"" + dir + L"1\\cmd_printer.exe\"" + L" \"some arguments\" more arguments -new_console ");
	create(nullptr, L"\"" + dir + L"1 @\\cmd_printer.exe\"" + L" \"some arguments\" more arguments -new_console ");

	create((dir + L"1\\cmd_printer.exe").c_str(), L"cmd_printer.exe -new_console \"some arguments\" ");
	create((dir + L"1 @\\cmd_printer.exe").c_str(), L"cmd_printer.exe -new_console \"some arguments\" ");

	create((dir + L"1\\cmd_printer.exe").c_str(), L"cmd_printer \"some arguments\" more arguments -new_console ");
	create((dir + L"1 @\\cmd_printer.exe").c_str(), L"cmd_printer \"some arguments\" more arguments -new_console ");
	create((dir + L"!1&@\\cmd_printer.exe").c_str(), L"cmd_printer \"some arguments\" more arguments -new_console ");

	create((dir + L"1\\cmd_printer.exe").c_str(), L"cmd \"some arguments\" more arguments -new_console ");
	create((dir + L"1 @\\cmd_printer.exe").c_str(), L"cmd \"some arguments\" more arguments -new_console ");

	create(L"1\\cmd_printer.exe", L"cmd_printer \"some arguments\" -new_console more arguments ");
	create(L"1 @\\cmd_printer.exe", L"cmd_printer \"some arguments\" -new_console more arguments ");

	create(cmd.c_str(), L"/C \"" + dir + L"1\\cmd_printer.exe" + L" -new_console \"some arguments\"\" ");
	create(cmd.c_str(), L"/C \"\"" + dir + L"1 @\\cmd_printer.exe\"" + L" -new_console \"some arguments\"\" ");

	create(cmd.c_str(), L"/C \"" + dir + L"1\\cmd_printer.exe" + L" \"some arguments\" -new_console more arguments\" ");
	create(cmd.c_str(), L"/C \"\"" + dir + L"1 @\\cmd_printer.exe\"" + L" \"some arguments\"\" -new_console more arguments ");

	shell(cmd, L"/C \"" + dir + L"1\\cmd_printer.exe" + L" \"some arguments\" -new_console more arguments\" ");
	shell(cmd, L"/C \"\"" + dir + L"1 @\\cmd_printer.exe\"" + L" \"some arguments\"\" -new_console more arguments ");

	shell(dir + L"1\\cmd_printer.exe", L"cmd_printer.exe -new_console \"some arguments\" ");
	shell(dir + L"1 @\\cmd_printer.exe", L"cmd_printer.exe -new_console \"some arguments\" ");
	shell(dir + L"!1&@\\cmd_printer.exe", L"cmd_printer.exe \"some arguments\" -new_console ");

	write(L"### Failures in cmd are expected ##\n\n");

	// error 123 expected in raw conhost (The filename, directory name, or volume label syntax is incorrect.)
	// because lpApplicationName should not be quoted
	// but in ConEmu it's okay, when it's running throw ConEmuC
	create((L"\"" + dir + L"1\\cmd_printer.exe\"").c_str(), L"\"some arguments\" more arguments -new_console:t:fail? ");
	create((L"\"" + dir + L"1 @\\cmd_printer.exe\"").c_str(), L"\"some arguments\" more arguments -new_console:t:fail? ");

	// 'C:\conemu\src\UnitTests\executer\1\cmd_printer.exe" "some' is not recognized as an internal or external command,
	create(cmd.c_str(), L"/C \"" + dir + L"1\\cmd_printer.exe\"" + L" -new_console:t:fail \"some arguments\"");
	// 'C:\conemu\src\UnitTests\executer\1' is not recognized as an internal or external command,
	create(cmd.c_str(), L"/C \"" + dir + L"1 @\\cmd_printer.exe\"" + L" -new_console:t:fail \"some arguments\"");

	return 0;
}
