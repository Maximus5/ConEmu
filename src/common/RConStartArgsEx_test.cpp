
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

#include "MAssert.h"
#include "Memory.h"
#include "MStrDup.h"
#include "MStrSafe.h"
#include "RConStartArgsEx.h"
#include "Common.h"
#include "WObjects.h"
#include "CmdLine.h"
#include "EnvVar.h"

#define DEBUGSTRPARSE(s) DEBUGSTR(s)


// TODO Fix assert in the NextArg
// L"C:\\Windows\\system32\\cmd.exe /c echo moving \"\"..\\_VCBUILD\\final.ConEmuCD.32W.vc9\\ConEmuCD.pdb\"\" to \"..\\..\\Release\\ConEmu\\ConEmuCD.pdb\""
// L"\"C:\\Program Files\\Microsoft SDKs\\Windows\\v7.0\\bin\\rc.EXE\" /D__GNUC__ /l 0x409 /fo\"\"..\\_VCBUILD\\final.ConEmu.32W.vc9\\obj\\ConEmu.res\"\" /d NDEBUG ConEmu.rc"

TEST(RConStartArgsEx, ArgTests)
{
	CmdArg s;
	s.Set(L"Abcdef", 3);
	int nDbg = lstrcmp(s, L"Abc");
	EXPECT_EQ(nDbg, 0);
	s.Set(L"qwerty");
	nDbg = lstrcmp(s, L"qwerty");
	EXPECT_EQ(nDbg, 0);
	s.Clear();
	//s.Set(L""); // !! Set("") must trigger ASSERT !!
	nDbg = s.ms_Val ? lstrcmp(s, L"") : -2;
	EXPECT_EQ(nDbg, 0);

	struct { LPCWSTR pszWhole; LPCWSTR pszCmp[10]; } lsArgTest[] = {
		{L"\"C:\\ConEmu\\ConEmuC64.exe\"  /PARENTFARPID=1 /C \"C:\\GIT\\cmdw\\ad.cmd CE12.sln & ci -m \"Solution debug build properties\"\"",
			{L"C:\\ConEmu\\ConEmuC64.exe", L"/PARENTFARPID=1", L"/C", L"C:\\GIT\\cmdw\\ad.cmd", L"CE12.sln", L"&", L"ci", L"-m", L"Solution debug build properties"}},
		{L"/C \"C:\\ad.cmd file.txt & ci -m \"Commit message\"\"",
			{L"/C", L"C:\\ad.cmd", L"file.txt", L"&", L"ci", L"-m", L"Commit message"}},
		{L"\"This is test\" Next-arg \t\n \"Third Arg +++++++++++++++++++\" ++", {L"This is test", L"Next-arg", L"Third Arg +++++++++++++++++++"}},
		{L"\"\"cmd\"\"", {L"cmd"}},
		{L"\"\"c:\\Windows\\System32\\cmd.exe\" /?\"", {L"c:\\Windows\\System32\\cmd.exe", L"/?"}},
		// Following example is crazy, but quotation issues may happen
		//{L"First Sec\"\"ond \"Thi\"rd\" \"Fo\"\"rth\"", {L"First", L"Sec\"\"ond", L"Thi\"rd", L"Fo\"\"rth"}},
		{L"First \"Fo\"\"rth\"", {L"First", L"Fo\"rth"}},
		// Multiple commands
		{L"set ConEmuReportExe=VIM.EXE & SH.EXE", {L"set", L"ConEmuReportExe=VIM.EXE", L"&", L"SH.EXE"}},
		// Inside escaped arguments
		{L"reg.exe add \"HKCU\\MyCo\" /ve /t REG_EXPAND_SZ /d \"\\\"C:\\ConEmu\\ConEmuPortable.exe\\\" /Dir \\\"%V\\\" /cmd \\\"cmd.exe\\\" \\\"-new_console:nC:cmd.exe\\\" \\\"-cur_console:d:%V\\\"\" /f",
			// Для наглядности:
			// reg.exe add "HKCU\MyCo" /ve /t REG_EXPAND_SZ
			//    /d "\"C:\ConEmu\ConEmuPortable.exe\" /Dir \"%V\" /cmd \"cmd.exe\" \"-new_console:nC:cmd.exe\" \"-cur_console:d:%V\"" /f
			{L"reg.exe", L"add", L"HKCU\\MyCo", L"/ve", L"/t", L"REG_EXPAND_SZ", L"/d",
			 L"\\\"C:\\ConEmu\\ConEmuPortable.exe\\\" /Dir \\\"%V\\\" /cmd \\\"cmd.exe\\\" \\\"-new_console:nC:cmd.exe\\\" \\\"-cur_console:d:%V\\\"",
			 L"/f"}},
		// Passsing -GuiMacro
		{L"-GuiMacro \"print(\\\" echo abc \\\"); Context;\"",
			{L"-GuiMacro", L"print(\\\" echo abc \\\"); Context;"}},
		// After 'Inside escaped arguments' regression bug appears
		{L"/dir \"C:\\\" /icon \"cmd.exe\" /single", {L"/dir", L"C:\\", L"/icon", L"cmd.exe", L"/single"}},
		{L"cmd \"one.exe /dir \\\"C:\\\\\" /log\" \"two.exe /dir \\\"C:\\\" /log\" end", {L"cmd", L"one.exe /dir \\\"C:\\\\\" /log", L"two.exe /dir \\\"C:\\\" /log", L"end"}},
		{NULL}
	};
	for (int i = 0; lsArgTest[i].pszWhole; i++)
	{
		s.Clear();
		LPCWSTR pszTestCmd = lsArgTest[i].pszWhole;
		int j = -1;
		while (lsArgTest[i].pszCmp[++j])
		{
			if (!(pszTestCmd = NextArg(pszTestCmd, s)))
			{
				FAIL() << "Fails on token!";
			}
			else
			{
				DemangleArg(s, s.m_bQuoted);
				nDbg = lstrcmp(s, lsArgTest[i].pszCmp[j]);
				if (nDbg != 0)
				{
					EXPECT_EQ(nDbg, 0);
				}
			}
		}
	}

	bool bTest = true;
	for (size_t i = 0; bTest; i++)
	{
		RConStartArgs arg;
		nDbg;
		LPCWSTR pszCmp;

		switch (i)
		{
		case 26:
			// conemu-old-issues#1710: The un-eaten double quote
			pszCmp = LR"(powershell -new_console:t:"PoSh":d:"%USERPROFILE%")";
			arg.pszSpecialCmd = lstrdup(pszCmp);
			arg.ProcessNewConArg();
			EXPECT_TRUE(arg.pszRenameTab && lstrcmp(arg.pszRenameTab, L"PoSh")==0);
			EXPECT_TRUE(arg.pszStartupDir && CEStr(ExpandEnvStr(L"%USERPROFILE%")).Compare(arg.pszStartupDir)==0);
			break;
		case 25:
			pszCmp = LR"(cmd -new_console:u:"john:pass^"word^"")";
			arg.pszSpecialCmd = lstrdup(pszCmp);
			arg.ProcessNewConArg();
			EXPECT_TRUE(arg.pszUserName && lstrcmp(arg.pszUserName, L"john")==0);
			EXPECT_TRUE(arg.szUserPassword && lstrcmp(arg.szUserPassword, L"pass\"word\"")==0);
			break;
		case 24:
			pszCmp = L"/C \"-new_console test.cmd bla\"";
			arg.pszSpecialCmd = lstrdup(pszCmp);
			arg.ProcessNewConArg();
			EXPECT_TRUE(0==lstrcmp(arg.pszSpecialCmd, L"/C \"test.cmd bla\"") && arg.NewConsole==crb_On);
			break;
		case 23:
			pszCmp = L"-new_console test.cmd";
			arg.pszSpecialCmd = lstrdup(pszCmp);
			arg.ProcessNewConArg();
			EXPECT_TRUE(0==lstrcmp(arg.pszSpecialCmd, L"test.cmd") && arg.NewConsole==crb_On);
			break;
		case 22:
			pszCmp = L"bash -cur_console:m:\"\"";
			arg.pszSpecialCmd = lstrdup(pszCmp);
			arg.ProcessNewConArg();
			EXPECT_TRUE(0==lstrcmp(arg.pszSpecialCmd, L"bash") && arg.pszMntRoot && *arg.pszMntRoot==0);
			break;
		case 21:
			pszCmp = L"cmd '-new_console' `-new_console` \\\"-new_console\\\"";
			arg.pszSpecialCmd = lstrdup(pszCmp);
			arg.ProcessNewConArg();
			EXPECT_TRUE(0==lstrcmp(arg.pszSpecialCmd, pszCmp) && arg.NewConsole==crb_Undefined);
			break;
		case 20:
			arg.pszSpecialCmd = lstrdup(L"\"c:\\cmd.exe\" \"-new_console\" \"c:\\file.txt\"");
			arg.ProcessNewConArg();
			EXPECT_TRUE(0==lstrcmp(arg.pszSpecialCmd, L"\"c:\\cmd.exe\" \"c:\\file.txt\""));
			break;
		case 19:
			arg.pszSpecialCmd = lstrdup(L"\"c:\\cmd.exe\" -new_console:n \"c:\\file.txt\"");
			arg.ProcessNewConArg();
			EXPECT_TRUE(0==lstrcmp(arg.pszSpecialCmd, L"\"c:\\cmd.exe\" \"c:\\file.txt\""));
			break;
		case 18:
			arg.pszSpecialCmd = lstrdup(L"\"c:\\cmd.exe\" \"-new_console:n\" \"c:\\file.txt\"");
			arg.ProcessNewConArg();
			EXPECT_TRUE(0==lstrcmp(arg.pszSpecialCmd, L"\"c:\\cmd.exe\" \"c:\\file.txt\""));
			break;
		case 17:
			arg.pszSpecialCmd = lstrdup(L"c:\\cmd.exe \"-new_console:n\" \"c:\\file.txt\"");
			arg.ProcessNewConArg();
			EXPECT_TRUE(0==lstrcmp(arg.pszSpecialCmd, L"c:\\cmd.exe \"c:\\file.txt\""));
			break;
		case 16:
			arg.pszSpecialCmd = lstrdup(L"\"c:\\cmd.exe\" \"-new_console:n\" c:\\file.txt");
			arg.ProcessNewConArg();
			EXPECT_TRUE(0==lstrcmp(arg.pszSpecialCmd, L"\"c:\\cmd.exe\" c:\\file.txt"));
			break;
		case 15:
			arg.pszSpecialCmd = lstrdup(L"c:\\file.txt -cur_console");
			arg.ProcessNewConArg();
			EXPECT_TRUE(0==lstrcmp(arg.pszSpecialCmd, L"c:\\file.txt"));
			break;
		case 14:
			arg.pszSpecialCmd = lstrdup(L"\"c:\\file.txt\" -cur_console");
			arg.ProcessNewConArg();
			EXPECT_TRUE(0==lstrcmp(arg.pszSpecialCmd, L"\"c:\\file.txt\""));
			break;
		case 13:
			arg.pszSpecialCmd = lstrdup(L" -cur_console \"c:\\file.txt\"");
			arg.ProcessNewConArg();
			EXPECT_TRUE(0==lstrcmp(arg.pszSpecialCmd, L"\"c:\\file.txt\""));
			break;
		case 12:
			arg.pszSpecialCmd = lstrdup(L"-cur_console \"c:\\file.txt\"");
			arg.ProcessNewConArg();
			EXPECT_TRUE(0==lstrcmp(arg.pszSpecialCmd, L"\"c:\\file.txt\""));
			break;
		case 11:
			arg.pszSpecialCmd = lstrdup(L"-cur_console c:\\file.txt");
			arg.ProcessNewConArg();
			EXPECT_TRUE(0==lstrcmp(arg.pszSpecialCmd, L"c:\\file.txt"));
			break;
		case 10:
			pszCmp = L"reg.exe add \"HKCU\\command\" /ve /t REG_EXPAND_SZ /d \"\\\"C:\\ConEmu\\ConEmuPortable.exe\\\" /Dir \\\"%V\\\" /cmd \\\"cmd.exe\\\" \\\"-new_console:nC:cmd.exe\\\" \\\"-cur_console:d:%V\\\"\" /f";
			arg.pszSpecialCmd = lstrdup(pszCmp);
			arg.ProcessNewConArg();
			EXPECT_TRUE(lstrcmp(arg.pszSpecialCmd, pszCmp)==0 && arg.NewConsole==crb_Undefined);
			break;
		case 9:
			pszCmp = L"\"C:\\Windows\\system32\\cmd.exe\" /C \"\"C:\\Python27\\python.EXE\"\"";
			arg.pszSpecialCmd = lstrdup(pszCmp);
			arg.ProcessNewConArg();
			EXPECT_TRUE(lstrcmp(arg.pszSpecialCmd, pszCmp)==0);
			break;
		case 8:
			arg.pszSpecialCmd = lstrdup(L"cmd --new_console -cur_console:a");
			arg.ProcessNewConArg();
			EXPECT_TRUE(lstrcmp(arg.pszSpecialCmd, L"cmd --new_console")==0 && arg.NewConsole==crb_Undefined && arg.RunAsAdministrator==crb_On);
			break;
		case 7:
			arg.pszSpecialCmd = lstrdup(L"cmd -cur_console:d:\"C:\\My docs\":t:\"My title\" \"-cur_console:C:C:\\my cmd.ico\" -cur_console:P:\"<PowerShell>\":a /k ver");
			arg.ProcessNewConArg();
			pszCmp = L"cmd /k ver";
			EXPECT_TRUE(lstrcmp(arg.pszSpecialCmd, pszCmp)==0);
			EXPECT_TRUE(arg.pszRenameTab && arg.pszPalette && arg.pszIconFile && arg.pszStartupDir && arg.NewConsole==crb_Undefined && lstrcmp(arg.pszRenameTab, L"My title")==0 && lstrcmp(arg.pszPalette, L"<PowerShell>")==0 && lstrcmp(arg.pszStartupDir, L"C:\\My docs")==0 && lstrcmp(arg.pszIconFile, L"C:\\my cmd.ico")==0);
			break;
		case 6:
			arg.pszSpecialCmd = lstrdup(L"cmd -cur_console:b:P:\"^<Power\"\"Shell^>\":t:\"My title\" /k ConEmuC.exe -Guimacro print(\"-new_console:a\")");
			arg.ProcessNewConArg();
			pszCmp = L"cmd /k ConEmuC.exe -Guimacro print(\"-new_console:a\")";
			EXPECT_TRUE(lstrcmp(arg.pszSpecialCmd, pszCmp)==0);
			EXPECT_TRUE(arg.pszRenameTab && arg.pszPalette && arg.BackgroundTab==crb_On && arg.NewConsole==crb_Undefined && arg.RunAsAdministrator==crb_Undefined && lstrcmp(arg.pszRenameTab, L"My title")==0 && lstrcmp(arg.pszPalette, L"<Power\"Shell>")==0);
			break;
		case 5:
			arg.pszSpecialCmd = lstrdup(L"cmd \"-cur_console:t:My title\" /k ver");
			arg.ProcessNewConArg();
			EXPECT_TRUE(lstrcmp(arg.pszSpecialCmd, L"cmd /k ver")==0);
			EXPECT_TRUE(arg.pszRenameTab && lstrcmp(arg.pszRenameTab, L"My title")==0 && arg.NewConsole==crb_Undefined);
			break;
		case 4:
			arg.pszSpecialCmd = lstrdup(L"cmd \"-new_console:P:^<Power\"\"Shell^>\"");
			arg.ProcessNewConArg();
			EXPECT_TRUE(lstrcmp(arg.pszSpecialCmd, L"cmd")==0);
			nDbg = lstrcmp(arg.pszPalette, L"<Power\"Shell>");
			EXPECT_TRUE(nDbg==0 && arg.NewConsole==crb_On);
			break;
		case 3:
			arg.pszSpecialCmd = lstrdup(L"cmd -cur_console:u:Max:");
			arg.ProcessNewConArg();
			EXPECT_TRUE(lstrcmp(arg.pszSpecialCmd, L"cmd")==0);
			nDbg = lstrcmp(arg.pszUserName,L"Max");
			EXPECT_TRUE(nDbg==0 && arg.pszDomain==NULL && !*arg.szUserPassword && arg.ForceUserDialog==crb_Off && arg.NewConsole!=crb_On);
			break;
		case 2:
			arg.pszSpecialCmd = lstrdup(L"cmd -cur_console:u:Max -new_console");
			arg.ProcessNewConArg();
			EXPECT_TRUE(lstrcmp(arg.pszSpecialCmd, L"cmd")==0);
			nDbg = lstrcmp(arg.pszUserName,L"Max");
			EXPECT_TRUE(nDbg==0 && arg.pszDomain==NULL && !*arg.szUserPassword && arg.ForceUserDialog==crb_On && arg.NewConsole==crb_On);
			break;
		case 1:
			arg.pszSpecialCmd = lstrdup(L"cmd -new_console:u -cur_console:h0");
			arg.ProcessNewConArg();
			EXPECT_TRUE(lstrcmp(arg.pszSpecialCmd, L"cmd")==0);
			EXPECT_TRUE(arg.pszUserName==NULL && arg.pszDomain==NULL && arg.ForceUserDialog==crb_On && arg.NewConsole==crb_On && arg.BufHeight==crb_On && arg.nBufHeight==0);
			break;
		case 0:
			arg.pszSpecialCmd = lstrdup(L"cmd \"-new_console:d:C:\\John Doe\\Home\" ");
			arg.ProcessNewConArg();
			EXPECT_TRUE(lstrcmp(arg.pszSpecialCmd, L"cmd ")==0);
			nDbg = lstrcmp(arg.pszStartupDir, L"C:\\John Doe\\Home");
			EXPECT_TRUE(nDbg==0 && arg.NewConsole==crb_On);
			break;
		default:
			bTest = false; // Stop tests
		}
	}

	nDbg = -1;
}
