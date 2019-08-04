
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

#include "Hotkeys.h"
#include "ConEmu.h"
#include "LngRc.h"
#include "Options.h"
#include "OptionsClass.h"
#include "SetCmdTask.h"
#include "VirtualConsole.h"

TEST(ConEmuHotKey,Tests)
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

	ConEmuHotKey HK = {0, chk_User};
	wchar_t szFull[128];
	int iCmp;

	for (size_t i = 0; i < countof(Tests); i++)
	{
		HK.SetHotKey(Tests[i].Vk, Tests[i].Mod[0], Tests[i].Mod[1], Tests[i].Mod[2]);
		_ASSERTE(HK.Key.Mod == Tests[i].ModTest);
		iCmp = wcscmp(HK.GetHotkeyName(szFull, true), Tests[i].KeyName);
		_ASSERTE(iCmp == 0);
	}
}
