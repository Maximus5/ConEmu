
/*
Copyright (c) 2013-present Maximus5
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

#include "Header.h"
#include "../UnitTests/gtest.h"

#include "HotkeyChord.h"
#include "SetPgBase.h"

#undef gsNoHotkey
#define gsNoHotkey L"<None>"

TEST(Hotkeys,Tests)
{
	// Key names order:
	//    Win, Apps, Ctrl, Alt, Shift
	// Modifiers:
	//    if "Ctrl" (any) - only cvk_Ctrl must be set
	//    if "RCtrl" (or L) - then cvk_Ctrl AND cvk_RCtrl must be set
	//    if both "LCtrl+RCtrl" - then ALL cvk_Ctrl, cvk_LCtrl, cvk_RCtrl must be set
	struct {
		BYTE Vk;
		BYTE Mod[3];
		ConEmuModifiers ModTest;
		LPCWSTR KeyName;
	} Tests[] = {
			{VK_OEM_5/*'|\'*/, {VK_LCONTROL, VK_RCONTROL}, cvk_Ctrl|cvk_LCtrl|cvk_RCtrl, L"LCtrl+RCtrl+\\"},
			{VK_SPACE, {VK_CONTROL, VK_LWIN, VK_MENU}, cvk_Ctrl|cvk_Alt|cvk_Win, L"Win+Ctrl+Alt+Space"},
			{'W', {VK_LWIN, VK_SHIFT}, cvk_Win|cvk_Shift, L"Win+Shift+W"},
			{'L', {VK_RCONTROL}, cvk_Ctrl|cvk_RCtrl, L"RCtrl+L"},
			{'C', {VK_CONTROL}, cvk_Ctrl, L"Ctrl+C"},
			{0, {}, cvk_Naked, gsNoHotkey},
	};

	const ConEmuHotKeyType HkType = chk_User;
	ConEmuChord HK = {};
	wchar_t szFull[128];

	for (size_t i = 0; i < countof(Tests); i++)
	{
		HK.SetHotKey(HkType, Tests[i].Vk, Tests[i].Mod[0], Tests[i].Mod[1], Tests[i].Mod[2]);
		EXPECT_EQ(HK.Mod, Tests[i].ModTest);
		EXPECT_STREQ(ConEmuChord::GetHotkeyName(szFull, HK.GetVkMod(HkType), HkType, true), Tests[i].KeyName);
	}
}

TEST(Hotkeys, GroupReplacement)
{
	wchar_t szFull[128];

	wcscpy_s(szFull, L"Left");
	CSetPgBase::ApplyHotkeyGroupName(szFull, L"Arrows");
	EXPECT_STREQ(szFull, L"Arrows");

	wcscpy_s(szFull, L"Win+Left");
	CSetPgBase::ApplyHotkeyGroupName(szFull, L"Arrows");
	EXPECT_STREQ(szFull, L"Win+Arrows");

	wcscpy_s(szFull, L"Ctrl+Alt+1");
	CSetPgBase::ApplyHotkeyGroupName(szFull, L"Numbers");
	EXPECT_STREQ(szFull, L"Ctrl+Alt+Numbers");
}

TEST(Hotkeys, LabelHotkeyReplacement)
{
	struct TestCases
	{
		LPCWSTR label;
		LPCWSTR hotkey;
		LPCWSTR from;
		LPCWSTR to;
		LPCWSTR expected;
	} tests[] = {
		{L"Paste mode #1 (Shift+Ins)", L"Ctrl+V", L"(", L")", L"Paste mode #1 (Ctrl+V)"},
		{L"Ctrl+Alt - drag ConEmu window", L"Win", nullptr, L" - ", L"Win - drag ConEmu window"},
		{L"Win+Number - activate console", L"Ctrl+Number", nullptr, L" - ", L"Ctrl+Number - activate console"},
	};
	for (const auto& test : tests)
	{
		CEStr label(test.label);
		const CEStr result = CSetPgBase::ApplyHotkeyToTitle(label, test.from, test.to, test.hotkey);
		EXPECT_STREQ(result.ms_Val, test.expected);
	}
}
