
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


#pragma once

#include "../common/Common.h"

// Strings
#define gsNoHotkey L"<None>"

#define CEHOTKEY_MODMASK    0xFFFFFF00
#define CEHOTKEY_NUMHOSTKEY 0xFFFFFF00
#define CEHOTKEY_ARRHOSTKEY 0xFEFEFE00
#define CEHOTKEY_NOMOD      0x80808000

enum ConEmuHotKeyType
{
	chk_User = 0,  // generic configurable hotkey
	chk_Modifier,  // e.g. for dragging
	chk_Modifier2, // e.g. for dragging when you need more than one modifier
	chk_NumHost,   // system hotkey (<HostKey>-Number, and previously WAS <HostKey>-Arrows)
	chk_ArrHost,   // system hotkey (<HostKey>-Number, and previously WAS <HostKey>-Arrows)
	chk_System,    // predefined hotkeys, not configurable yet
	chk_Global,    // globally registered hotkey
	chk_Local,     // locally registered hotkey
	chk_Macro,     // GUI Macro
	chk_Task,      // Task hotkey
};

struct CEVkMatch
{
	BYTE Vk;
	bool Distinct;
	ConEmuModifiers Mod;
	ConEmuModifiers Left;
	ConEmuModifiers Right;

	void Set(BYTE aVk, bool aDistinct, ConEmuModifiers aMod, ConEmuModifiers aLeft = cvk_NULL, ConEmuModifiers aRight = cvk_NULL);
	static bool GetMatchByVk(BYTE Vk, CEVkMatch& Match);
	static ConEmuModifiers GetFlagsFromMod(DWORD vkMods);
};

struct ConEmuChord
{
public:
	// Main key (first chord)
	BYTE Vk;
	// Modifiers (Win, Alt, etc.)
	ConEmuModifiers Mod;

	#ifdef _DEBUG
	static void ValidateChordMod(ConEmuModifiers aMod);
	#endif

	// service initialization function, creates ready VkMod
	static DWORD MakeHotKey(BYTE Vk, BYTE vkMod1 = 0, BYTE vkMod2 = 0, BYTE vkMod3 = 0);

	// Return user-friendly key name (e.g. "Apps+Space")
	static LPCWSTR GetHotkeyName(wchar_t(&szFull)[128], const DWORD aVkMod, const ConEmuHotKeyType HkType, const bool bShowNone);
	// nHostMod three least significant bytes may contain VK (modifiers)
	// the function checks that they are unique
	static void TestHostkeyModifiers(DWORD& nHostMod);
	// set of flags MOD_xxx for RegisterHotKey
	static DWORD GetHotKeyMod(DWORD VkMod);
	// Return the VK
	static DWORD GetHotkey(DWORD VkMod);
	// Return key name (Apps, Win, Pause, ...)
	static void GetVkKeyName(BYTE vk, wchar_t(&szName)[32], bool bSingle = true);
	// Return VK by key name (Apps, Win, Pause, ...)
	static UINT GetVkByKeyName(LPCWSTR asName, int* pnScanCode = NULL, DWORD* pnControlState = NULL);
	// Has that (VkMod) hotkey - modifier Mod (VK)?
	static bool HasModifier(DWORD VkMod, BYTE Mod/*VK*/);
	// Set or Xor modifier in VkMod
	static DWORD SetModifier(DWORD VkMod, BYTE Mod/*VK*/, bool Xor = true);
	// Return modifier by index (idx = 1..3).
	// Supports specials CEHOTKEY_NUMHOSTKEY & CEHOTKEY_ARRHOSTKEY
	static DWORD GetModifier(DWORD VkMod, int idx/*1..3*/);

	void  SetHotKey(const ConEmuHotKeyType HkType, BYTE Vk, BYTE vkMod1 = 0, BYTE vkMod2 = 0, BYTE vkMod3 = 0);
	void  SetVkMod(const ConEmuHotKeyType HkType, DWORD VkMod, bool rawMod = false);
	DWORD GetVkMod(const ConEmuHotKeyType HkType) const;

	void  Set(BYTE aVk = 0, ConEmuModifiers aMod = cvk_NULL);
	UINT  GetModifiers(BYTE (&Mods)[3]) const;
	bool  IsEmpty() const;
	bool  IsEqual(const ConEmuChord& Key) const;
	bool  IsEqual(BYTE aVk, ConEmuModifiers aMod) const;

private:
	static bool Compare(const ConEmuModifiers aMod1, const ConEmuModifiers aMod2);
};

extern const ConEmuChord NullChord;
