
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

	msprintf(aBuf, countof(aBuf), "Simple %c %s %S %u %i %%tail", 'X', "ansi", L"WIDE", 102U, -102);
	EXPECT_STREQ(aBuf, "Simple X ansi WIDE 102 -102 %tail");
	msprintf(wBuf, countof(aBuf), L"Simple %c %s %S %u %i %%tail", L'X', L"wide", "ANSI", 102U, -102);
	EXPECT_STREQ(wBuf, L"Simple X wide ANSI 102 -102 %tail");

	msprintf(aBuf, countof(aBuf), "%u %u %i %i %i",
		0U, (std::numeric_limits<uint32_t>::max)(),
		0, (std::numeric_limits<int32_t>::min)(), (std::numeric_limits<int32_t>::max)());
	EXPECT_STREQ(aBuf, "0 4294967295 0 -2147483648 2147483647");
	msprintf(wBuf, countof(wBuf), L"%u %u %i %i %i",
		0U, (std::numeric_limits<uint32_t>::max)(),
		0, (std::numeric_limits<int32_t>::min)(), (std::numeric_limits<int32_t>::max)());
	EXPECT_STREQ(wBuf, L"0 4294967295 0 -2147483648 2147483647");

	msprintf(aBuf, countof(aBuf), "%x %X", 0xF1E2B3A1, 0xF1E2B3A1);
	EXPECT_STREQ(aBuf, "f1e2b3a1 F1E2B3A1");
	msprintf(wBuf, countof(wBuf), L"%x %X", 0xF1E2B3A1, 0xF1E2B3A1);
	EXPECT_STREQ(wBuf, L"f1e2b3a1 F1E2B3A1");

	msprintf(aBuf, countof(aBuf), "%02x %02X %04x %04X %08x %08X", 15, 15, 14, 14, 13, 13);
	EXPECT_STREQ(aBuf, "0f 0F 000e 000E 0000000d 0000000D");
	msprintf(wBuf, countof(wBuf), L"%02x %02X %04x %04X %08x %08X", 15, 15, 14, 14, 13, 13);
	EXPECT_STREQ(wBuf, L"0f 0F 000e 000E 0000000d 0000000D");

	msprintf(aBuf, countof(aBuf), "%02u %02u %03u %03u %02X", 7, 12, 13, 7654, 0xFABC);
	EXPECT_STREQ(aBuf, "07 12 013 7654 FABC");
	msprintf(wBuf, countof(wBuf), L"%02u %02u %03u %03u %02X", 7, 12, 13, 7654, 0xFABC);
	EXPECT_STREQ(wBuf, L"07 12 013 7654 FABC");
}
