
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

// ReSharper disable StringLiteralTypo
#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "defines.h"
#include "../UnitTests/gtest.h"

#include "MStrSafe.h"
#include <string>
#include <limits>

extern bool gbVerifyIgnoreAsserts;  // NOLINT(readability-redundant-declaration)

TEST(MStrSafe, CompareIgnoreCaseNullsA)
{
	EXPECT_EQ(-1, lstrcmpni("", "test", 1));
	EXPECT_EQ(-1, lstrcmpni(nullptr, "test", 1));
	EXPECT_EQ(1, lstrcmpni("test", "", 1));
	EXPECT_EQ(1, lstrcmpni("test", nullptr, 1));
	EXPECT_EQ(0, lstrcmpni("", "", 1));
	EXPECT_EQ(0, lstrcmpni(static_cast<const char*>(nullptr), nullptr, 1));
}

TEST(MStrSafe, CompareIgnoreCaseNullsW)
{
	EXPECT_EQ(-1, lstrcmpni(L"", L"test", 1));
	EXPECT_EQ(-1, lstrcmpni(nullptr, L"test", 1));
	EXPECT_LT(lstrcmpni(nullptr, L"test", 1), 0);
	EXPECT_EQ(1, lstrcmpni(L"test", L"", 1));
	EXPECT_EQ(1, lstrcmpni(L"test", nullptr, 1));
	EXPECT_GT(lstrcmpni(L"test", nullptr, 1), 0);
	EXPECT_EQ(0, lstrcmpni(L"", L"", 1));
	EXPECT_EQ(0, lstrcmpni(static_cast<const wchar_t*>(nullptr), nullptr, 1));
}

TEST(MStrSafe, CompareIgnoreCaseA)
{
	EXPECT_EQ(lstrcmpni("ABCDEF", "abcdef", 1), 0);
	EXPECT_GT(lstrcmpni("XBCDEF", "abcdef", 1), 0);
	EXPECT_LT(lstrcmpni("ABCDEF", "xbcdef", 1), 0);
	
	EXPECT_EQ(lstrcmpni("ABCDEF", "abcdef", 3), 0);
	EXPECT_EQ(lstrcmpni("ABCABC", "abcdef", 3), 0);

	EXPECT_EQ(lstrcmpni("GuiMacro:x1234", "GuiMacro=", 8), 0);

	const std::string upper64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKL";
	const std::string lower64 = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijkl";
	EXPECT_EQ(lstrcmpni(upper64.c_str(), lower64.c_str(), 63), 0);
	EXPECT_EQ(lstrcmpni(upper64.c_str(), lower64.c_str(), 64), 0);
	EXPECT_EQ(lstrcmpni((upper64 + "XYZ").c_str(), (lower64 + "abc").c_str(), 64), 0);
	EXPECT_GT(lstrcmpni((upper64 + "XYZ").c_str(), (lower64 + "abc").c_str(), 80), 0);
	EXPECT_LT(lstrcmpni((upper64 + "ABC").c_str(), (lower64 + "xyz").c_str(), 80), 0);
	EXPECT_EQ(lstrcmpni(upper64.c_str(), lower64.c_str(), 80), 0);
}

TEST(MStrSafe, CompareIgnoreCaseW)
{
	EXPECT_EQ(lstrcmpni(L"ABCDEF", L"abcdef", 1), 0);
	EXPECT_GT(lstrcmpni(L"XBCDEF", L"abcdef", 1), 0);
	EXPECT_LT(lstrcmpni(L"ABCDEF", L"xbcdef", 1), 0);
	
	EXPECT_EQ(lstrcmpni(L"ABCDEF", L"abcdef", 3), 0);
	EXPECT_EQ(lstrcmpni(L"ABCABC", L"abcdef", 3), 0);

	EXPECT_EQ(lstrcmpni(L"GuiMacro:x1234", L"GuiMacro=", 8), 0);

	const std::wstring upper64 = L"ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKL";
	const std::wstring lower64 = L"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijkl";
	EXPECT_EQ(lstrcmpni(upper64.c_str(), lower64.c_str(), 63), 0);
	EXPECT_EQ(lstrcmpni(upper64.c_str(), lower64.c_str(), 64), 0);
	EXPECT_EQ(lstrcmpni((upper64 + L"XYZ").c_str(), (lower64 + L"abc").c_str(), 64), 0);
	EXPECT_GT(lstrcmpni((upper64 + L"XYZ").c_str(), (lower64 + L"abc").c_str(), 80), 0);
	EXPECT_LT(lstrcmpni((upper64 + L"ABC").c_str(), (lower64 + L"xyz").c_str(), 80), 0);
	EXPECT_EQ(lstrcmpni(upper64.c_str(), lower64.c_str(), 80), 0);
}

TEST(MStrSafe, msprintf)
{
	char aBuf[100];
	wchar_t wBuf[100];

	gbVerifyIgnoreAsserts = true;

	EXPECT_EQ(nullptr, msprintf(aBuf, 0, "Simple %s", "ansi"));
	EXPECT_EQ(nullptr, msprintf(nullptr, countof(aBuf), "Simple %s", "ansi"));
	EXPECT_EQ(nullptr, msprintf(wBuf, 0, L"Simple %s", L"wide"));
	EXPECT_EQ(nullptr, msprintf(nullptr, countof(wBuf), L"Simple %s", L"wide"));

	EXPECT_NE(nullptr, msprintf(aBuf, countof(aBuf), "Simple %c %s %S %u %i %%tail", 'X', "ansi", L"WIDE", 102U, -102));
	EXPECT_STREQ(aBuf, "Simple X ansi WIDE 102 -102 %tail");
	EXPECT_NE(nullptr, msprintf(wBuf, countof(aBuf), L"Simple %c %s %S %u %i %%tail", L'X', L"wide", "ANSI", 102U, -102));
	EXPECT_STREQ(wBuf, L"Simple X wide ANSI 102 -102 %tail");

	EXPECT_NE(nullptr, msprintf(aBuf, countof(aBuf), "%u %u %i %i %i",
		0U, (std::numeric_limits<uint32_t>::max)(),
		0, (std::numeric_limits<int32_t>::min)(), (std::numeric_limits<int32_t>::max)()));
	EXPECT_STREQ(aBuf, "0 4294967295 0 -2147483648 2147483647");
	EXPECT_NE(nullptr, msprintf(wBuf, countof(wBuf), L"%u %u %i %i %i",
		0U, (std::numeric_limits<uint32_t>::max)(),
		0, (std::numeric_limits<int32_t>::min)(), (std::numeric_limits<int32_t>::max)()));
	EXPECT_STREQ(wBuf, L"0 4294967295 0 -2147483648 2147483647");

	EXPECT_NE(nullptr, msprintf(aBuf, countof(aBuf), "%x %X", 0xF1E2B3A1, 0xF1E2B3A1));
	EXPECT_STREQ(aBuf, "f1e2b3a1 F1E2B3A1");
	EXPECT_NE(nullptr, msprintf(wBuf, countof(wBuf), L"%x %X", 0xF1E2B3A1, 0xF1E2B3A1));
	EXPECT_STREQ(wBuf, L"f1e2b3a1 F1E2B3A1");

	EXPECT_NE(nullptr, msprintf(aBuf, countof(aBuf), "%02x %02X %04x %04X %08x %08X", 15, 15, 14, 14, 13, 13));
	EXPECT_STREQ(aBuf, "0f 0F 000e 000E 0000000d 0000000D");
	EXPECT_NE(nullptr, msprintf(wBuf, countof(wBuf), L"%02x %02X %04x %04X %08x %08X", 15, 15, 14, 14, 13, 13));
	EXPECT_STREQ(wBuf, L"0f 0F 000e 000E 0000000d 0000000D");

	EXPECT_NE(nullptr, msprintf(aBuf, countof(aBuf), "%02u %02u %03u %03u %02X", 7, 12, 13, 7654, 0xFABC));
	EXPECT_STREQ(aBuf, "07 12 013 7654 FABC");
	EXPECT_NE(nullptr, msprintf(wBuf, countof(wBuf), L"%02u %02u %03u %03u %02X", 7, 12, 13, 7654, 0xFABC));
	EXPECT_STREQ(wBuf, L"07 12 013 7654 FABC");

	EXPECT_NE(nullptr, msprintf(aBuf, countof(aBuf), "%02u %02u %03u %03u %02X", 7, 12, 13, 7654, 0xFABC));
	EXPECT_STREQ(aBuf, "07 12 013 7654 FABC");
	EXPECT_NE(nullptr, msprintf(wBuf, countof(wBuf), L"%02u %02u %03u %03u %02X", 7, 12, 13, 7654, 0xFABC));
	EXPECT_STREQ(wBuf, L"07 12 013 7654 FABC");

	// overflow check
	EXPECT_NE(nullptr, msprintf(aBuf, 10, "%xz", 0xF1E2B3A1));
	EXPECT_STREQ(aBuf, "f1e2b3a1z");
	EXPECT_NE(nullptr, msprintf(wBuf, 10, L"%xz", 0xF1E2B3A1));
	EXPECT_STREQ(wBuf, L"f1e2b3a1z");
	EXPECT_EQ(nullptr, msprintf(aBuf, 7, "%x %X", 0xF1E2B3A1));
	EXPECT_STREQ(aBuf, "f1e2b3");
	EXPECT_EQ(nullptr, msprintf(wBuf, 7, L"%x %X", 0xF1E2B3A1));
	EXPECT_STREQ(wBuf, L"f1e2b3");

	EXPECT_EQ(nullptr, msprintf(aBuf, 5, "%i", -123456));
	EXPECT_STREQ(aBuf, "-123");
	EXPECT_EQ(nullptr, msprintf(wBuf, 5, L"%i", -123456));
	EXPECT_STREQ(wBuf, L"-123");

	EXPECT_EQ(nullptr, msprintf(aBuf, 3, "%03u", 51));
	EXPECT_STREQ(aBuf, "05");
	EXPECT_EQ(nullptr, msprintf(wBuf, 3, L"%03u", 51));
	EXPECT_STREQ(wBuf, L"05");
	
	EXPECT_NE(nullptr, msprintf(aBuf, 9, "%s", "Abcdefgh"));
	EXPECT_STREQ(aBuf, "Abcdefgh");
	EXPECT_NE(nullptr, msprintf(wBuf, 9, L"%s", L"Abcdefgh"));
	EXPECT_STREQ(wBuf, L"Abcdefgh");
	EXPECT_EQ(nullptr, msprintf(aBuf, 4, "%s", "Abcdefgh"));
	EXPECT_STREQ(aBuf, "Abc");
	EXPECT_EQ(nullptr, msprintf(wBuf, 4, L"%s", L"Abcdefgh"));
	EXPECT_STREQ(wBuf, L"Abc");
	EXPECT_EQ(nullptr, msprintf(aBuf, 5, "%S", L"Abcdefgh"));
	EXPECT_STREQ(aBuf, "Abcd");
	EXPECT_EQ(nullptr, msprintf(wBuf, 5, L"%S", "Abcdefgh"));
	EXPECT_STREQ(wBuf, L"Abcd");
	
	gbVerifyIgnoreAsserts = false;
}

TEST(MStrSafe, wcscpyn_c)
{
	wchar_t data[16] = L"";

	wmemset(data, L' ', countof(data) - 1);
	_wcscpyn_c(data, countof(data), L"abc", 3);
	EXPECT_STREQ(data, L"abc");

	wmemset(data, L' ', countof(data) - 1);
	_wcscpyn_c(data, countof(data), L"abcd", 2);
	EXPECT_STREQ(data, L"ab");

	wmemset(data, L' ', countof(data) - 1);
	_wcscpyn_c(data, 4, L"123456", 6);
	EXPECT_STREQ(data, L"123");

	wmemset(data, L' ', countof(data) - 1);
	_wcscpyn_c(data, countof(data), L"qwerty", 10);
	EXPECT_STREQ(data, L"qwerty");
}

TEST(MStrSafe, strcpyn_c)
{
	gbVerifyIgnoreAsserts = true; // bypass debug asserts for invalid parameters

	char data[16] = "";

	memset(data, ' ', countof(data) - 1);
	_strcpyn_c(data, countof(data), "abc", 3);
	EXPECT_STREQ(data, "abc");

	memset(data, ' ', countof(data) - 1);
	_strcpyn_c(data, countof(data), "abcd", 2);
	EXPECT_STREQ(data, "ab");

	memset(data, ' ', countof(data) - 1);
	_strcpyn_c(data, 4, "123456", 6);
	EXPECT_STREQ(data, "123");

	memset(data, ' ', countof(data) - 1);
	_strcpyn_c(data, countof(data), "qwerty", 10);
	EXPECT_STREQ(data, "qwerty");

	gbVerifyIgnoreAsserts = false;
}

TEST(MStrSafe, wcscatn_c)
{
	wchar_t data[16] = L"";

	_wcscpyn_c(data, countof(data), L"abc", 3);
	_wcscatn_c(data, countof(data), L"qwerty", 3);
	EXPECT_STREQ(data, L"abcqwe");

	_wcscpyn_c(data, 10, L"abc", 3);
	EXPECT_STREQ(data, L"abc");
	_wcscatn_c(data, 10, L"qwerty123456", 10);
	EXPECT_STREQ(data, L"abcqwerty");

	// wmemset(data, L'x', countof(data) - 1);
	// data[countof(data) - 1] = L'\0';
	// EXPECT_THROW(_wcscatn_c(data, countof(data), L"qwerty", 3)); -- generates DebugBreak()
	// EXPECT_STREQ(data, L"ab");
}
