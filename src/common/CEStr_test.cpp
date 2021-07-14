
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
#include "CEStr.h"
#include <string>

TEST(Strings, Empty)
{
	CEStr str;
	EXPECT_TRUE(str.IsEmpty());
	EXPECT_EQ(nullptr, str.c_str());
	EXPECT_NE(nullptr, str.c_str(L""));
	EXPECT_STREQ(L"", str.c_str(L""));
	EXPECT_STREQ(L"default", str.c_str(L"default"));

	EXPECT_NE(nullptr, str.Set(L"", -1));
	EXPECT_TRUE(str.IsEmpty());
	EXPECT_NE(nullptr, str.ms_Val);

	auto* ptr = str.Detach();
	EXPECT_EQ(nullptr, str.ms_Val);
	EXPECT_TRUE(str.IsEmpty());
	EXPECT_NE(nullptr, ptr);
	free(ptr);

	EXPECT_EQ(nullptr, str.Set(nullptr, -1));
	EXPECT_TRUE(str.IsEmpty());
	EXPECT_EQ(nullptr, str.ms_Val);
}

TEST(Strings, Constructors)
{
	{
		wchar_t someString[] = L"Some string";
		const CEStr str(someString);
		EXPECT_NE(str.data(), someString);
	}

	{
		CEStr str1(L"Sample text");
		EXPECT_FALSE(str1.IsEmpty());
		CEStr str2(std::move(str1));
		EXPECT_TRUE(str1.IsEmpty());  // NOLINT(bugprone-use-after-move)
		EXPECT_FALSE(str2.IsEmpty());
		str2.Release();
		EXPECT_TRUE(str2.IsEmpty());
	}

	{
		wchar_t* ptrNull = nullptr;
		const CEStr nullStr(ptrNull);
		EXPECT_TRUE(nullStr.IsNull());
		EXPECT_TRUE(nullStr.IsEmpty());
		EXPECT_EQ(nullptr, nullStr.data());
		EXPECT_EQ(nullptr, nullStr.c_str());
		EXPECT_NE(nullptr, nullStr.c_str(L""));
	}

	{
		wchar_t emptyPtr[] = L"";
		const CEStr emptyStr(emptyPtr);
		EXPECT_FALSE(emptyStr.IsNull());
		EXPECT_TRUE(emptyStr.IsEmpty());
		EXPECT_NE(nullptr, emptyStr.data());
		EXPECT_NE(nullptr, emptyStr.c_str());
		EXPECT_STREQ(L"", emptyStr.c_str());
	}
}

TEST(Strings, SetValue)
{
	CEStr str;
	EXPECT_TRUE(str.IsEmpty());

	const std::wstring shortStr = L"Redistributions";
	const std::wstring someStr = L"Redistributions of source code must retain the above copyright";

	// part of string for now
	EXPECT_NE(nullptr, str.Set(someStr.c_str(), shortStr.length()));
	EXPECT_FALSE(str.IsEmpty());
	EXPECT_STREQ(shortStr.c_str(), str.ms_Val);
	EXPECT_EQ(0, str.Compare(shortStr.c_str()));
	EXPECT_GT(0, str.Compare(someStr.c_str()));

	EXPECT_NE(nullptr, str.Set(someStr.c_str()));
	EXPECT_FALSE(str.IsEmpty());
	EXPECT_STREQ(someStr.c_str(), str.ms_Val);
	EXPECT_EQ(0, str.Compare(someStr.c_str()));
	EXPECT_LT(0, str.Compare(shortStr.c_str()));

	auto* ptr = str.Detach();
	EXPECT_EQ(nullptr, str.ms_Val);
	EXPECT_TRUE(str.IsEmpty());
	EXPECT_NE(nullptr, ptr);
	free(ptr);

	str.Set(nullptr);
	EXPECT_TRUE(str.IsNull());
	EXPECT_TRUE(str.IsEmpty());
	str.Set(L"");
	EXPECT_FALSE(str.IsNull());
	EXPECT_TRUE(str.IsEmpty());
	str.Set(nullptr);
	EXPECT_FALSE(str.IsNull());
	EXPECT_TRUE(str.IsEmpty());
}

TEST(Strings, Compare)
{
	CEStr str;
	EXPECT_TRUE(str.IsEmpty());

	const wchar_t camelCase[] = L"SomeVariable";
	// ReSharper disable once StringLiteralTypo
	const wchar_t lowerCase[] = L"somevariable";
	// ReSharper disable once StringLiteralTypo
	const wchar_t titleCase[] = L"SOMEVARIABLE";

	EXPECT_NE(nullptr, str.Set(camelCase));
	EXPECT_FALSE(str.IsEmpty());
	EXPECT_EQ(0, str.Compare(camelCase));
	EXPECT_EQ(0, str.Compare(lowerCase));
	EXPECT_EQ(0, str.Compare(titleCase));
	EXPECT_EQ(0, str.Compare(lowerCase, false));
	EXPECT_EQ(0, str.Compare(titleCase, false));
	EXPECT_NE(0, str.Compare(lowerCase, true));
	EXPECT_NE(0, str.Compare(titleCase, true));
}

TEST(Strings, Realloc)
{
	const wchar_t testStr[] = L"Part1";
	CEStr str;
	EXPECT_EQ(nullptr, str.data());
	EXPECT_EQ(0, str.GetMaxCount());

	const auto* ptr0 = str.GetBuffer(16);
	EXPECT_NE(nullptr, ptr0);
	EXPECT_EQ(17, str.GetMaxCount());
	const auto* ptr1 = str.data();
	EXPECT_EQ(ptr1, ptr0);
	EXPECT_EQ(ptr1, str.Set(testStr));
	EXPECT_STREQ(str.c_str(), testStr);

	const auto* ptr2 = str.GetBuffer(128);
	EXPECT_NE(nullptr, ptr2);
	EXPECT_EQ(129, str.GetMaxCount());
	EXPECT_NE(ptr1, ptr2);
	EXPECT_STREQ(str.c_str(), testStr);
}

TEST(Strings, Replace)
{
	EXPECT_STREQ(CEStr(L"").Replace(L"<what>", L"with").c_str(), L"");

	EXPECT_STREQ(CEStr(L"Text <what>").Replace(L"<what>", L"with-text").c_str(), L"Text with-text");
	EXPECT_STREQ(CEStr(L"<what> text").Replace(L"<what>", L"with-text").c_str(), L"with-text text");
	EXPECT_STREQ(CEStr(L"Text <what> text").Replace(L"<what>", L"with-text").c_str(), L"Text with-text text");
	EXPECT_STREQ(CEStr(L"Text <what> text").Replace(L"<what>", L"with").c_str(), L"Text with text");
}
