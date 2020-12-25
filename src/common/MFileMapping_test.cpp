
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
#include "MFileMapping.h"
#include "Common.h"

extern bool gbVerifyIgnoreAsserts;  // NOLINT(readability-redundant-declaration)

#define TEST_FILE_MAP_NAME L"ConEmu.Test.FileMap.%u"

TEST(MFileMapping, Simple)
{
	EXPECT_GT(sizeof(CESERVER_CONSOLE_MAPPING_HDR), 0x1024U);
	EXPECT_LT(sizeof(CESERVER_CONSOLE_MAPPING_HDR), 0x2048U);
	CESERVER_CONSOLE_MAPPING_HDR data{};
	data.cbSize = sizeof(data);
	data.Flags = 1;
	MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> test;
	EXPECT_FALSE(test.IsValid());
	test.InitName(TEST_FILE_MAP_NAME L".Simple", GetCurrentProcessId());
	EXPECT_TRUE(test.Create());
	EXPECT_TRUE(test.IsValid());
	const auto* ptr = test.Ptr();
	EXPECT_NE(ptr, nullptr);
	EXPECT_NE(test.Ptr()->cbSize, data.cbSize);
	EXPECT_NE(test.Ptr()->Flags, data.Flags);
	EXPECT_TRUE(test.SetFrom(&data));
	EXPECT_EQ(test.Ptr()->cbSize, data.cbSize);
	EXPECT_EQ(test.Ptr()->Flags, data.Flags);
}

TEST(MFileMapping, Underflow)
{
	EXPECT_GT(sizeof(CESERVER_CONSOLE_MAPPING_HDR), 0x1024U);
	EXPECT_LT(sizeof(CESERVER_CONSOLE_MAPPING_HDR), 0x2048U);
	CESERVER_CONSOLE_MAPPING_HDR data{};
	data.cbSize = sizeof(data);
	data.Flags = 2;

	// Create a map smaller than size of structure
	MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> test1;
	{
		test1.InitName(TEST_FILE_MAP_NAME L".Underflow", GetCurrentProcessId());
		gbVerifyIgnoreAsserts = true;
		EXPECT_TRUE(test1.Create(0x800));
		gbVerifyIgnoreAsserts = false;
		EXPECT_TRUE(test1.IsValid());
		const auto* ptr = test1.Ptr();
		EXPECT_NE(ptr, nullptr);
		const size_t flagsOffset = reinterpret_cast<const char*>(&ptr->Flags) - reinterpret_cast<const char*>(ptr);
		EXPECT_LE(flagsOffset + sizeof(ptr->Flags), test1.GetMaxSize());
		EXPECT_NE(ptr->cbSize, data.cbSize);
		EXPECT_NE(ptr->Flags, data.Flags);
		const auto mapInfo = test1.GetMapInfo();
		EXPECT_GE(mapInfo.RegionSize, 0x800U);
		EXPECT_LT(mapInfo.RegionSize, sizeof(CESERVER_CONSOLE_MAPPING_HDR));
	}

	// Try to create second full sized map
	MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> test2;
	{
		test2.InitName(TEST_FILE_MAP_NAME L".Underflow", GetCurrentProcessId());
		EXPECT_FALSE(test2.Create());
		EXPECT_FALSE(test2.IsValid());
		EXPECT_EQ(test2.Ptr(), nullptr);
	}
}

TEST(MFileMapping, CreateRead)
{
	CESERVER_CONSOLE_MAPPING_HDR data{};
	data.cbSize = sizeof(data);
	data.Flags = 3;
	for (size_t i = 0; i + 1 < std::size(data.ComSpec.ConEmuExeDir); ++i)
	{
		data.ComSpec.ConEmuExeDir[i] = 33 + (i % 64);
	}

	MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> test1;
	{
		test1.InitName(TEST_FILE_MAP_NAME L".CreateRead", GetCurrentProcessId());
		EXPECT_TRUE(test1.Create());
		EXPECT_TRUE(test1.IsValid());
		const auto* ptr = test1.Ptr();
		EXPECT_NE(ptr, nullptr);
		EXPECT_NE(ptr->cbSize, data.cbSize);
		EXPECT_NE(ptr->Flags, data.Flags);
		EXPECT_STREQ(ptr->ComSpec.ConEmuExeDir, L"");
	}

	// Try to open second read-only map
	MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> test2;
	{
		test2.InitName(TEST_FILE_MAP_NAME L".CreateRead", GetCurrentProcessId());
		EXPECT_TRUE(test2.Open());
		EXPECT_TRUE(test2.IsValid());

		CESERVER_CONSOLE_MAPPING_HDR dataCopy{};
		EXPECT_TRUE(test2.GetTo(&dataCopy));
		EXPECT_NE(memcmp(&dataCopy, &data, sizeof(data)), 0);

		EXPECT_FALSE(test2.SetFrom(&data));
		EXPECT_EQ(test2.GetLastExceptionCode(), EXCEPTION_ACCESS_VIOLATION);
	}

	// Let's update maps
	{
		const auto* ptr1 = test1.Ptr();
		EXPECT_NE(ptr1, nullptr);
		const auto* ptr2 = test2.Ptr();
		EXPECT_NE(ptr2, nullptr);
		EXPECT_NE(ptr1, ptr2);

		EXPECT_TRUE(test1.SetFrom(&data));
		EXPECT_EQ(test1.GetLastExceptionCode(), 0);
		EXPECT_EQ(ptr1->cbSize, data.cbSize);
		EXPECT_EQ(ptr1->Flags, data.Flags);
		EXPECT_STREQ(ptr1->ComSpec.ConEmuExeDir, data.ComSpec.ConEmuExeDir);

		CESERVER_CONSOLE_MAPPING_HDR dataCopy{};
		EXPECT_TRUE(test2.GetTo(&dataCopy));
		EXPECT_EQ(test2.GetLastExceptionCode(), 0);
		EXPECT_EQ(memcmp(&dataCopy, &data, sizeof(data)), 0);
	}
}

TEST(MFileMapping, FailOpen)
{
	// Try to create second full sized map
	MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> test;
	{
		test.InitName(TEST_FILE_MAP_NAME L".FailOpen", GetCurrentProcessId());
		EXPECT_FALSE(test.Open());
		EXPECT_FALSE(test.IsValid());
		CESERVER_CONSOLE_MAPPING_HDR data{};
		EXPECT_FALSE(test.SetFrom(&data));
	}
}
