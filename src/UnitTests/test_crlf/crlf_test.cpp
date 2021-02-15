
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

HANDLE hConOut;
int result = 0;

void write(const std::wstring& line)
{
	DWORD written = 0;
	WriteConsoleW(hConOut, line.c_str(), line.length(), &written, nullptr);
}

void test(const std::wstring& expected)
{
	DWORD outFlags = -1; GetConsoleMode(hConOut, &outFlags);
	wchar_t buffer[255] = L"";
	swprintf_s(buffer, 255, L"Current output console mode: 0x%08X (%s %s)\r\n", outFlags,
		(outFlags & ENABLE_VIRTUAL_TERMINAL_PROCESSING) ? L"ENABLE_VIRTUAL_TERMINAL_PROCESSING" : L"!ENABLE_VIRTUAL_TERMINAL_PROCESSING",
		(outFlags & DISABLE_NEWLINE_AUTO_RETURN) ? L"DISABLE_NEWLINE_AUTO_RETURN" : L"!DISABLE_NEWLINE_AUTO_RETURN");
	write(buffer);

	SetConsoleTextAttribute(hConOut, 14);
	write(L"Print(AAA\\nBBB\\r\\nCCC)\r\n");
	SetConsoleTextAttribute(hConOut, 7);

	write(L"AAA\nBBB\r\nCCC\r\n");

	if (expected.empty())
		return;

	CONSOLE_SCREEN_BUFFER_INFO csbi{};
	GetConsoleScreenBufferInfo(hConOut, &csbi);
	CHAR_INFO readBuffer[6 * 3] = {};
	SMALL_RECT rcRead = {0, csbi.dwCursorPosition.Y - 3, 5, csbi.dwCursorPosition.Y - 1};
	ReadConsoleOutputW(hConOut, readBuffer, COORD{6, 3}, COORD{0, 0}, &rcRead);
	std::wstring data;
	for (const auto& ci : readBuffer)
	{
		data += ci.Char.UnicodeChar;
	}
	if (data == expected)
	{
		SetConsoleTextAttribute(hConOut, 10);
		write(L"OK\r\n");
	}
	else
	{
		SetConsoleTextAttribute(hConOut, 12);
		write(L"FAILED: '");
		write(data);
		write(L"' != '");
		write(expected);
		write(L"'\r\n");
		result = 1;
	}
	SetConsoleTextAttribute(hConOut, 7);
}

int main()
{
	hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD outFlags = 0; GetConsoleMode(hConOut, &outFlags);
	const bool isDefaultMode = (outFlags == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT));
	CONSOLE_SCREEN_BUFFER_INFO csbi{};
	GetConsoleScreenBufferInfo(hConOut, &csbi);

	SetConsoleTextAttribute(hConOut, 7);

	test(isDefaultMode ? L"AAA   " L"BBB   " L"CCC   " : L"");

	if (!isDefaultMode)
	{
		SetConsoleMode(hConOut, (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT));
		test(L"AAA   " L"BBB   " L"CCC   ");
	}

	SetConsoleMode(hConOut, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
	test(L"AAA   " L"   BBB" L"CCC   ");

	SetConsoleMode(hConOut, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	test(L"AAA   " L"BBB   " L"CCC   ");

	SetConsoleMode(hConOut, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | DISABLE_NEWLINE_AUTO_RETURN);
	test(L"AAA   " L"   BBB" L"CCC   ");

	SetConsoleMode(hConOut, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
	test(L"AAA   " L"   BBB" L"CCC   ");

	SetConsoleMode(hConOut, outFlags);
	SetConsoleTextAttribute(hConOut, csbi.wAttributes);
	return result;
}
