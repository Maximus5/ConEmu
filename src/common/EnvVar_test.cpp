
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
#include "EnvVar.h"

#include "WObjects.h"
#include "WRegistry.h"

TEST(EnvVar, EmptyW)
{
	const auto emptyVar = GetEnvVar(L"SomeAbsentConEmuVarName");
	EXPECT_TRUE(emptyVar.IsEmpty());
	EXPECT_TRUE(emptyVar.IsNull());

	const auto expandVar = ExpandEnvStr(L"%SomeAbsentConEmuVarName%");
	EXPECT_STREQ(expandVar.c_str(), L"%SomeAbsentConEmuVarName%");
}

TEST(EnvVar, EmptyA)
{
	const auto emptyVar = GetEnvVar("SomeAbsentConEmuVarName");
	EXPECT_EQ(nullptr, emptyVar.data());

	const auto expandVar = ExpandEnvStr("%SomeAbsentConEmuVarName%");
	EXPECT_STREQ(expandVar.c_str(), "%SomeAbsentConEmuVarName%");
}

TEST(EnvVar, ExistingW)
{
	const auto winDir = GetEnvVar(L"windir");
	EXPECT_FALSE(winDir.IsEmpty());
	EXPECT_FALSE(winDir.IsNull());

	const auto userName = GetEnvVar(L"USERNAME");
	EXPECT_FALSE(userName.IsEmpty());
	EXPECT_FALSE(userName.IsNull());

	const auto expandVar = ExpandEnvStr(L"%windir%");
	EXPECT_STREQ(expandVar.c_str(), winDir.c_str());

	const auto extraVars = ExpandEnvStr(LR"(windir%windir%\%USERNAME%USERNAME)");
	const CEStr expected(L"windir", winDir, L"\\", userName, L"USERNAME");
	EXPECT_STREQ(extraVars.c_str(), expected.c_str());
}

TEST(EnvVar, ExistingA)
{
	const auto winDir = GetEnvVar("windir");
	EXPECT_NE(nullptr, winDir.data());

	const auto userName = GetEnvVar("USERNAME");
	EXPECT_NE(nullptr, userName.data());

	const auto expandVar = ExpandEnvStr("%windir%");
	EXPECT_STREQ(expandVar.c_str(), winDir.c_str());

	const auto extraVars = ExpandEnvStr(R"(windir%windir%\%USERNAME%USERNAME)");
	const CEStrA expected("windir", winDir, "\\", userName, "USERNAME");
	EXPECT_STREQ(extraVars.c_str(), expected.c_str());
}

TEST(EnvVar, RegTest)
{
	CEStr regProgramFiles;
	EXPECT_GT(RegGetStringValue(nullptr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion", L"ProgramFilesDir", regProgramFiles), 0);
	const CEStr envProgramFiles = GetEnvVar(IsWindows64() ? L"ProgramW6432" : L"ProgramFiles");
	EXPECT_FALSE(envProgramFiles.IsEmpty());
	EXPECT_STREQ(envProgramFiles.c_str(), regProgramFiles.c_str());
}
