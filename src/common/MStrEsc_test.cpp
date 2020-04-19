
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

#include "defines.h"
#include "../UnitTests/gtest.h"

#include "MStrEsc.h"

TEST(MStrEsc, CheckStrForSpecials)
{
	struct StringChecks { LPCWSTR pszStr; bool result, slash, others; };
	const StringChecks tests[] = {
		{L"", false, false, false},
		{L"empty", false, false, false},
		{L"c:\\dir\\file", true, true, false},
		{L"c:/dir/file", false, false, false},
		{L"c:\\dir\\file\n", true, true, true},
		{L"line\n", true, false, true},
	};
	for (size_t i = 0; i < std::size(tests); ++i)
	{
		bool slash = false, others = false;
		EXPECT_EQ(CheckStrForSpecials(tests[i].pszStr, &slash, &others), tests[i].result);
		EXPECT_EQ(slash, tests[i].slash);
		EXPECT_EQ(others, tests[i].others);
	}

	EXPECT_EQ(CheckStrForSpecials(nullptr, nullptr, nullptr), false);
}

TEST(MStrEsc, EscapeString)
{
	struct EscapeChecks { LPCWSTR pszSrc; LPCWSTR pszDst; };
	const EscapeChecks tests[] = {
		{L"", L""},
		{L"no escapes", L"no escapes"},
		{L"1\n2\t3\"4\\e", L"1\\n2\\t3\\\"4\\\\e"},
		{L"hello \\e tom", L"hello \\\\e tom"},
	};
	for (size_t i = 0; i < std::size(tests); ++i)
	{
		wchar_t szBuffer[128] = L"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
		LPCWSTR pszSrc = tests[i].pszSrc;
		LPWSTR pszDst = szBuffer;
		EXPECT_EQ(EscapeString(pszSrc, pszDst), true);
		EXPECT_STREQ(szBuffer, tests[i].pszDst);
		wchar_t szReverted[128] = L"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
		pszSrc = szBuffer;
		pszDst = szReverted;
		EXPECT_EQ(UnescapeString(pszSrc, pszDst), true);
		EXPECT_STREQ(szReverted, tests[i].pszSrc);
	}
}

TEST(MStrEsc, UnescapeString)
{
	struct UnescapeChecks { LPCWSTR pszSrc; LPCWSTR pszDst; };
	const UnescapeChecks tests[] = {
		{L"", L""},
		{L"no escapes", L"no escapes"},
		{L"1\\n2\\t3\\\"4\\\\e", L"1\n2\t3\"4\\e"},
		{L"hello \\\\e tom", L"hello \\e tom"},
		{L"hello \\e tom", L"hello \x1B tom"},
	};
	for (size_t i = 0; i < std::size(tests); ++i)
	{
		wchar_t szBuffer[128] = L"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
		LPCWSTR pszSrc = tests[i].pszSrc;
		LPWSTR pszDst = szBuffer;
		EXPECT_EQ(UnescapeString(pszSrc, pszDst), true);
		EXPECT_STREQ(szBuffer, tests[i].pszDst);
	}
}
