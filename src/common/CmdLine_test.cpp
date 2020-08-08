
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

TEST(CmdLine, NextArg)
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
