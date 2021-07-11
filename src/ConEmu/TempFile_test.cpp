
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "../common/defines.h"
#include "../UnitTests/gtest.h"
#include "TempFile.h"
#include "../common/EnvVar.h"
#include "../common/WObjects.h"

namespace {
const wchar_t testTempDir[] = L"%TEMP%\\ConEmuTest";
}

class TempFileTest : public testing::Test
{
	static void EraseTempDir()
	{
		const CEStr eraseDir(ExpandEnvStr(testTempDir));
		if (eraseDir)
			RemoveDirectoryW(eraseDir);
	}
public:
	void SetUp() override
	{
		EraseTempDir();
	}

	void TearDown() override
	{
		EraseTempDir();
	}
};

TEST(TempFile, CreateDelete)
{
	CEStr fileName;
	{
		const TempFile file(testTempDir, L"TestFile.tmp");
		EXPECT_TRUE(file.IsValid());
		fileName.Set(file.GetFilePath());
		EXPECT_TRUE(FileExists(fileName));
		LARGE_INTEGER fileSize{};
		EXPECT_TRUE(GetFileSizeEx(file.GetHandle(), &fileSize));
		EXPECT_EQ(fileSize.QuadPart, 0);
	}
	EXPECT_FALSE(FileExists(fileName));
}

TEST(TempFile, CreateDontDelete)
{
	CEStr fileName;
	{
		TempFile file(testTempDir, L"TestFile.tmp");
		EXPECT_TRUE(file.IsValid());
		fileName.Set(file.GetFilePath());
		EXPECT_TRUE(FileExists(fileName));
		LARGE_INTEGER fileSize{};
		EXPECT_TRUE(GetFileSizeEx(file.GetHandle(), &fileSize));
		EXPECT_EQ(fileSize.QuadPart, 0);
		file.Release(false);
	}
	EXPECT_TRUE(FileExists(fileName));
	EXPECT_TRUE(DeleteFileW(fileName));
	EXPECT_FALSE(FileExists(fileName));
}

TEST(TempFile, ObjMove)
{
	CEStr fileName;
	{
		TempFile file(testTempDir, L"TestFile.tmp");
		EXPECT_TRUE(file.IsValid());
		fileName.Set(file.GetFilePath());
		EXPECT_TRUE(FileExists(fileName));

		const TempFile file2 = std::move(file);
		EXPECT_FALSE(file.IsValid());  // NOLINT(bugprone-use-after-move) -- intended check
		EXPECT_TRUE(file2.IsValid());
		EXPECT_TRUE(FileExists(fileName));
		EXPECT_STREQ(fileName.c_str(), file2.GetFilePath());
	}
	EXPECT_FALSE(FileExists(fileName));
}
