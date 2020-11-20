
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

#include "CmdLine.h"
#include "MStrDup.h"
#include "RConStartArgsEx.h"

TEST(CmdLine, NextArg_Switches)
{
	LPCWSTR pszCmd =
		L"conemu.exe /c/dir -run -inside=0x800 "
		L" /cmdlist \"-inside=\\eCD /d %1\" @\"C:\\long path\\args.tmp\" "
		L"@C:\\tmp\\args.tmp -bad|switch ";
	CmdArg ls;

	EXPECT_NE(nullptr, (pszCmd=NextArg(pszCmd,ls)));
	EXPECT_STREQ(ls.c_str(), L"conemu.exe");
	EXPECT_FALSE(ls.IsPossibleSwitch());

	EXPECT_NE(nullptr, (pszCmd=NextArg(pszCmd,ls)));
	EXPECT_STREQ(ls.c_str(), L"/c/dir");
	EXPECT_FALSE(ls.IsPossibleSwitch());

	EXPECT_NE(nullptr, (pszCmd=NextArg(pszCmd,ls)));
	EXPECT_STREQ(ls.c_str(), L"-run");
	EXPECT_TRUE(ls.OneOfSwitches(L"/cmd",L"/run"));
	EXPECT_FALSE(ls.OneOfSwitches(L"/cmd",L"/cmdlist"));
	EXPECT_TRUE(ls.IsSwitch(L"-run"));
	EXPECT_FALSE(ls.IsSwitch(L"-run:"));
	EXPECT_FALSE(ls.IsSwitch(L"-run="));
	EXPECT_STREQ(L"", ls.GetExtra());

	EXPECT_NE(nullptr, (pszCmd=NextArg(pszCmd,ls)));
	EXPECT_STREQ(ls.c_str(), L"-inside=0x800");
	EXPECT_TRUE(ls.IsSwitch(L"-inside="));
	EXPECT_TRUE(ls.IsSwitch(L"-inside:"));
	EXPECT_FALSE(ls.IsSwitch(L"-inside"));
	EXPECT_TRUE(ls.OneOfSwitches(L"-inside",L"-inside="));
	EXPECT_STREQ(L"0x800", ls.GetExtra());

	EXPECT_NE(nullptr, (pszCmd=NextArg(pszCmd,ls)));
	EXPECT_STREQ(ls.c_str(), L"/cmdlist");
	EXPECT_TRUE(ls.IsSwitch(L"-cmdlist"));

	EXPECT_NE(nullptr, (pszCmd=NextArg(pszCmd,ls)));
	EXPECT_STREQ(ls.c_str(), L"-inside=\\eCD /d %1");
	EXPECT_TRUE(ls.IsSwitch(L"-inside:"));
	EXPECT_STREQ(L"\\eCD /d %1", ls.GetExtra());

	// #NextArg '@' used in debugLogShell
	EXPECT_NE(nullptr, (pszCmd=NextArg(pszCmd,ls)));
	EXPECT_STREQ(ls.c_str(), L"@");
	EXPECT_NE(nullptr, (pszCmd=NextArg(pszCmd,ls)));
	EXPECT_STREQ(ls.c_str(), L"C:\\long path\\args.tmp");
	EXPECT_NE(nullptr, (pszCmd=NextArg(pszCmd,ls)));
	EXPECT_STREQ(ls.c_str(), L"@C:\\tmp\\args.tmp");
	
	EXPECT_NE(nullptr, (pszCmd=NextArg(pszCmd,ls)));
	EXPECT_STREQ(ls.c_str(), L"-bad|switch");
	EXPECT_FALSE(ls.IsPossibleSwitch());
}

TEST(CmdLine, NextArg_NeedCmd)
{
	struct StrTests { LPCWSTR pszCmd; bool expected; }
	tests[] = {
		{L"\"C:\\windows\\notepad.exe -f \"makefile\" COMMON=\"../../../plugins/common\"\"", FALSE},
		{L"\"\"C:\\windows\\notepad.exe  -new_console\"\"", FALSE},
		{L"\"\"cmd\"\"", FALSE},
		{L"cmd /c \"\"C:\\Program Files\\Windows NT\\Accessories\\wordpad.exe\" -?\"", FALSE},
		{L"cmd /c \"dir c:\\\"", FALSE},
		{L"abc.cmd", TRUE},
		// Do not do too many heuristic. If user really needs redirection (for 'root'!)
		// he must explicitly call "cmd /c ...". With only exception if first exe not found.
		{L"notepad text & start explorer", FALSE},
	};
	LPCWSTR psArgs;
	bool bNeedCut, bRootIsCmd, bAlwaysConfirm, bAutoDisable;
	CEStr szExe;
	for (const auto& test : tests)
	{
		szExe.Empty();
		RConStartArgsEx rcs; rcs.pszSpecialCmd = lstrdup(test.pszCmd);
		rcs.ProcessNewConArg();
		// ReSharper disable once CppJoinDeclarationAndAssignment
		const bool result = IsNeedCmd(TRUE, rcs.pszSpecialCmd, szExe, &psArgs, &bNeedCut, &bRootIsCmd, &bAlwaysConfirm, &bAutoDisable);
		EXPECT_EQ(result, test.expected) << L"cmd: " << test.pszCmd;
	}
}

namespace
{
// ReSharper disable once CppParameterMayBeConst
void TestIsNeedCmd(LPCWSTR testCommand, const bool expectedResult, const bool isServer,
				   // ReSharper disable twice CppParameterMayBeConst
				   LPCWSTR expectedExe, LPCWSTR expectedArgs, const bool expectedNeedCut, const bool expectedRootIsCmd,
				   const bool expectedAlwaysConfirm, const bool expectedAutoDisableConfirm)
{
	CEStr exe;
	LPCWSTR arguments = nullptr;
	bool needCutStartEndQuot = false, rootIsCmdExe = false;
	bool alwaysConfirmExit = false, autoDisableConfirmExit = false;
	const auto isNeedCmd = IsNeedCmd(isServer, testCommand, exe,
		&arguments, &needCutStartEndQuot, &rootIsCmdExe, &alwaysConfirmExit, &autoDisableConfirmExit);
	EXPECT_EQ(isNeedCmd, expectedResult) << "cmd: " << testCommand << ", srv: " << isServer;
	if (wcschr(expectedExe, L'\\') != nullptr)
	{
		EXPECT_STREQ(exe.c_str(L""), expectedExe) << "cmd: " << testCommand << ", srv: " << isServer;
	}
	else
	{
		EXPECT_STREQ(PointToName(exe.c_str(L"")), expectedExe) << "cmd: " << testCommand << ", srv: " << isServer;
	}
	EXPECT_STREQ(arguments ? arguments : L"", expectedArgs) << "cmd: " << testCommand << ", srv: " << isServer;
	EXPECT_EQ(needCutStartEndQuot, expectedNeedCut) << "cmd: " << testCommand << ", srv: " << isServer;
	EXPECT_EQ(rootIsCmdExe, expectedRootIsCmd) << "cmd: " << testCommand << ", srv: " << isServer;
	EXPECT_EQ(alwaysConfirmExit, expectedAlwaysConfirm) << "cmd: " << testCommand << ", srv: " << isServer;
	EXPECT_EQ(autoDisableConfirmExit, expectedAutoDisableConfirm) << "cmd: " << testCommand << ", srv: " << isServer;
}
}

TEST(CmdLine, IsNeedCmd)
{
	TestIsNeedCmd(L"set var1=val1 & set var2=val2 & pwsh.exe -noprofile", true, false,
		L"set", L"",
		false, true, false, false);
	TestIsNeedCmd(L"cmd.exe /c set var1=val1 & set var2=val2 & pwsh.exe -noprofile", false, false,
		L"cmd.exe", L" /c set var1=val1 & set var2=val2 & pwsh.exe -noprofile",
		false, true, true, false);
	TestIsNeedCmd(L"start", true, false,
		L"", L"",
		false, true, false, false);
	TestIsNeedCmd(L"start explorer", true, false,
		L"", L"",
		false, true, false, false);
	TestIsNeedCmd(L"start \"\" \"C:\\user data\\test.exe\"", true, false,
		L"", L"",
		false, true, false, false);
	TestIsNeedCmd(L"Call \"C:\\user scripts\\tool.cmd\" some args", true, false,
		L"Call", L"",
		false, true, false, false);
	TestIsNeedCmd(L"tool.exe < input > output", true, false,
		L"tool.exe", L"",
		false, true, false, false);
	TestIsNeedCmd(L"\"tool.exe\" < input > output", true, true,
		L"tool.exe", L"",
		false, true, false, false);
	TestIsNeedCmd(L"c:\\tool.exe < input > output", true, false,
		L"tool.exe", L"",
		false, true, false, false);
	// #FIX expectedResult should be true
	TestIsNeedCmd(L"%windir%\\system32\\cacls.exe < input > output", false, true,
		L"cacls.exe", L" < input > output",
		false, false, false, false);
	// #FIX expectedResult should be true
	TestIsNeedCmd(L"%comspec% < input > output", false, true,
		L"cmd.exe", L" < input > output",
		false, true, true, false);
	TestIsNeedCmd(L"%comspec% /c < input > output", false, true,
		L"cmd.exe", L" /c < input > output",
		false, true, true, false);
	TestIsNeedCmd(L"%comspec% /K < input > output", false, true,
		L"cmd.exe", L" /K < input > output",
		false, true, true, false);
	// #FIX expectedResult should be true
	TestIsNeedCmd(L"chkdsk < input > output", false, true,
		L"chkdsk.exe", L" < input > output",
		false, false, false, false);
	// #FIX expectedArgs should not end with \"
	// #FIX expectedNeedCut should be true
	TestIsNeedCmd(L"\"\"7z\" a test.7z ConEmu.exe \"", false, false,
		L"7z.exe" /* via search */, L"a test.7z ConEmu.exe \"",
		false, false, false, false);
	// #FIX expectedResult should be false
	TestIsNeedCmd(L"\"c:\\program files\\arc\\7z.exe\" -?", true, false,
		L"c:\\program files\\arc\\7z.exe", L"",
		false, true, false, false);
	// #FIX expectedResult should be false
	// #FIX expectedNeedCut should be true
	TestIsNeedCmd(L"\"\"c:\\program files\\arc\\7z.exe\" -?\"", true, false,
		L"c:\\program files\\arc\\7z.exe", L"",
		false, true, false, false);
	// #FIX expectedResult should be false
	// #FIX expectedExe should not contain "-?"
	TestIsNeedCmd(L"\"c:\\arc\\7z.exe -?\"", true, false,
		L"c:\\arc\\7z.exe -?", L"",
		false, true, false, false);
	// #FIX expectedResult should be false
	// #FIX expectedExe should not contain "-?"
	TestIsNeedCmd(L"\"c:\\far\\far.exe -new_console\"", true, false,
		L"c:\\far\\far.exe -new_console", L"",
		false, true, false, false);
	// #FIX expectedResult should be false
	// #FIX expectedExe should not contain "-?"
	TestIsNeedCmd(L"\"\"c:\\far\\far.exe -new_console\"\"", true, false,
		L"c:\\far\\far.exe -new_console", L"",
		false, true, false, false);
	// #FIX expectedResult should be false
	// #FIX expectedExe should not contain -f
	TestIsNeedCmd(L"\"C:\\msys\\bin\\make.EXE -f \"makefile\" COMMON=\"/../plugins\"\"", true, false,
		L"C:\\msys\\bin\\make.EXE -f", L"",
		false, true, false, false);
	// #FIX expectedResult should be false
	TestIsNeedCmd(L"C:\\msys\\bin\\make.EXE -f \"makefile\" COMMON=\"/../plugins\"", true, false,
		L"C:\\msys\\bin\\make.EXE", L"",
		false, true, false, false);
	// #FIX expectedAlwaysConfirm?
	TestIsNeedCmd(L"\"\"cmd\"\"", false, false,
		L"cmd.exe", L"",
		false, true, true, false);
	// #FIX expectedAlwaysConfirm?
	TestIsNeedCmd(L"cmd /c \"\"c:\\program files\\arc\\7z.exe\" -?\"", false, false,
		L"cmd.exe", L" /c \"\"c:\\program files\\arc\\7z.exe\" -?\"",
		false, true, true, false);
	// #FIX expectedAlwaysConfirm?
	TestIsNeedCmd(L"cmd /c \"dir c:\\\"", false, false,
		L"cmd.exe", L" /c \"dir c:\\\"",
		false, true, true, false);
}

TEST(CmdLine, DemangleArg)
{
	struct StrTests { wchar_t szTest[100], szCmp[100]; }
	tests[] = {
		{ L"\"Test1 & ^ \"\" Test2\"  Test3  \"Test \"\" 4\"", L"Test1 & ^ \" Test2\0Test3\0Test \" 4\0\0" }
	};

	CmdArg ls;
	for (auto& test : tests)
	{
		LPCWSTR command = test.szTest;
		LPCWSTR expected = test.szCmp;
		while ((command = NextArg(command, ls)))
		{
			DemangleArg(ls, ls.m_bQuoted);
			EXPECT_STREQ(ls.ms_Val, expected) << L"cmd: " << command;
			expected += wcslen(expected)+1;
		}
		EXPECT_EQ(*expected, 0);
	}
}
