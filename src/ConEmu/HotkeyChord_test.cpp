
/*
Copyright (c) 2014-present Maximus5
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

#include <gtest/gtest.h>
#include "HotkeyChord.h"

TEST(ConEmuChord, ChordUnitTests)
{
	struct TestDef
	{
		bool bEqual;
		BYTE vk;
		ConEmuModifiers Mod;
	};
	struct UnitTests
	{
		BYTE vk;
		ConEmuModifiers Mod;
		TestDef Def[10];
	} Tests[] = {
		{'C', cvk_Ctrl, {
				{false, '0', cvk_Ctrl},
				{true,  'C', cvk_Ctrl},
				{true,  'C', cvk_Ctrl|cvk_LCtrl},
				{true,  'C', cvk_Ctrl|cvk_RCtrl},
				{false, 'C', cvk_Ctrl|cvk_LCtrl|cvk_RCtrl},
				{false, 'C', cvk_Ctrl|cvk_LCtrl|cvk_Alt},
				{},
		}},
		{'1', cvk_Ctrl|cvk_LCtrl, {
				{true,  '1', cvk_Ctrl},
				{true,  '1', cvk_Ctrl|cvk_LCtrl},
				{false, '1', cvk_Ctrl|cvk_RCtrl},
				{false, '1', cvk_Ctrl|cvk_LCtrl|cvk_Alt},
				{},
		}},
		{'1', cvk_Ctrl|cvk_LCtrl|cvk_RCtrl, {
				{true,  '1', cvk_Ctrl|cvk_LCtrl|cvk_RCtrl},
				{false, '1', cvk_Ctrl},
				{false, '1', cvk_Ctrl|cvk_LCtrl},
				{false, '1', cvk_Ctrl|cvk_RCtrl},
				{},
		}},
		{VK_F12, cvk_NULL, {
				{true,  VK_F12, cvk_NULL},
				{true,  VK_F12, cvk_Naked},
				{false, VK_F12, cvk_Shift},
				{},
		}},
		{VK_INSERT, cvk_Apps, {
				{true,  VK_INSERT, cvk_Apps},
				{false, VK_INSERT, cvk_NULL},
				{false, VK_INSERT, cvk_Apps|cvk_Shift},
				{},
		}},
		{VK_OEM_5/*'|\'*/, cvk_Ctrl|cvk_LCtrl|cvk_RCtrl, {
				{true,  VK_OEM_5/*'|\'*/, cvk_Ctrl|cvk_LCtrl|cvk_RCtrl},
				{false, VK_OEM_5/*'|\'*/, cvk_Ctrl|cvk_RCtrl},
				{},
		}},
		{},
	};

	ConEmuChord ch;
	bool bMatch;

	for (size_t i = 0; Tests[i].vk; i++)
	{
		ch.Set(Tests[i].vk, Tests[i].Mod);
		for (size_t j = 0; Tests[i].Def[j].vk; j++)
		{
			bMatch = ch.IsEqual(Tests[i].Def[j].vk, Tests[i].Def[j].Mod);
			EXPECT_EQ(bMatch, Tests[i].Def[j].bEqual);
		}
	}
}
