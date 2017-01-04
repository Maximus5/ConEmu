
/*
Copyright (c) 2014-2017 Maximus5
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

#include "HotkeyChord.h"

const ConEmuChord NullChord = {};

// Used to get KeyName or exact VK_xxx from Mod
// So, Mod must contain single value but not a mask
// Ctrl == cvk_Ctrl, LCtrl == cvk_LCtrl, and so on...
CEVkMatch gvkMatchList[] = {
	{VK_LWIN,     true,  cvk_Win},
	{VK_APPS,     true,  cvk_Apps},
	{VK_CONTROL,  false, cvk_Ctrl,  cvk_LCtrl, cvk_RCtrl},
	{VK_LCONTROL, true,  cvk_LCtrl},
	{VK_RCONTROL, true,  cvk_RCtrl},
	{VK_MENU,     false, cvk_Alt,   cvk_LAlt, cvk_RAlt},
	{VK_LMENU,    true,  cvk_LAlt},
	{VK_RMENU,    true,  cvk_RAlt},
	{VK_SHIFT,    false, cvk_Shift, cvk_LShift, cvk_RShift},
	{VK_LSHIFT,   true,  cvk_LShift},
	{VK_RSHIFT,   true,  cvk_RShift},
	{0}
};

// Internal conversions between VK_xxx and cvk_xxx
// Used only by ConEmuHotKey::SetVkMod, so it must
// return masked values.
// LCtrl => cvk_Ctrl|cvk_LCtrl (Distinct==true)
// Ctrl => cvk_Ctrl (Distinct==true and Left/Right values)
bool CEVkMatch::GetMatchByVk(BYTE Vk, CEVkMatch& Match)
{
	switch (Vk)
	{
	case VK_LCONTROL: Match.Set(Vk, true,  cvk_Ctrl|cvk_LCtrl);  break;
	case VK_RCONTROL: Match.Set(Vk, true,  cvk_Ctrl|cvk_RCtrl);  break;
	case VK_LMENU:    Match.Set(Vk, true,  cvk_Alt|cvk_LAlt);   break;
	case VK_RMENU:    Match.Set(Vk, true,  cvk_Alt|cvk_RAlt);   break;
	case VK_LSHIFT:   Match.Set(Vk, true,  cvk_Shift|cvk_LShift); break;
	case VK_RSHIFT:   Match.Set(Vk, true,  cvk_Shift|cvk_RShift); break;
	case VK_RWIN:  // The RightWin processed the same way as LeftWin
	case VK_LWIN:     Match.Set(Vk, true,  cvk_Win);    break;
	case VK_APPS:     Match.Set(Vk, true,  cvk_Apps);   break;
	case VK_CONTROL:  Match.Set(Vk, false, cvk_Ctrl,  cvk_LCtrl, cvk_RCtrl);   break;
	case VK_MENU:     Match.Set(Vk, false, cvk_Alt,   cvk_LAlt, cvk_RAlt);     break;
	case VK_SHIFT:    Match.Set(Vk, false, cvk_Shift, cvk_LShift, cvk_RShift); break;
	default:
		return false;
	}
	return true;
#if 0
	for (size_t i = 0; gvkMatchList[i].Vk; i++)
	{
		if (Vk == gvkMatchList[i].Vk)
		{
			Match = gvkMatchList[i];
			return true;
		}
	}
	return false;
#endif
}

void CEVkMatch::Set(BYTE aVk, bool aDistinct, ConEmuModifiers aMod, ConEmuModifiers aLeft /*= cvk_NULL*/, ConEmuModifiers aRight /*= cvk_NULL*/)
{
	Vk = aVk; Distinct = aDistinct; Mod = aMod; Left = aLeft; Right = aRight;
}

ConEmuModifiers CEVkMatch::GetFlagsFromMod(DWORD vkMods)
{
	CEVkMatch Match;
	ConEmuModifiers NewMod = cvk_NULL;
	DWORD vkLeft = vkMods;

	for (int i = 4; i--;)
	{
		if (CEVkMatch::GetMatchByVk(LOBYTE(vkLeft), Match))
		{
			NewMod |= Match.Mod;
		}
		vkLeft = (vkLeft >> 8);
	}

	return NewMod;
}

#ifdef _DEBUG
void ConEmuChord::ValidateChordMod(ConEmuModifiers aMod)
{
	// The idea - if cvk_Ctrl is NOT set, neither cvk_LCtrl nor cvk_RCtrl must be set too
	#if 0
	bool bCtrl = ((aMod & cvk_Ctrl) != cvk_NULL);
	bool bLCtrl = ((aMod & cvk_LCtrl) != cvk_NULL);
	bool bRCtrl = ((aMod & cvk_RCtrl) != cvk_NULL);
	_ASSERTE(bCtrl ? (bCtrl|bLCtrl|bRCtrl) : !(bLCtrl|bRCtrl));
	#endif
	_ASSERTE(((aMod & cvk_Ctrl) != cvk_NULL) || ((aMod & (cvk_LCtrl|cvk_RCtrl)) == cvk_NULL));
	_ASSERTE(((aMod & cvk_Alt) != cvk_NULL) || ((aMod & (cvk_LAlt|cvk_RAlt)) == cvk_NULL));
	_ASSERTE(((aMod & cvk_Shift) != cvk_NULL) || ((aMod & (cvk_LShift|cvk_RShift)) == cvk_NULL));
}
#else
#define ValidateChordMod(aMod)
#endif

void ConEmuChord::Set(BYTE aVk, ConEmuModifiers aMod)
{
	ValidateChordMod(aMod);
	Vk = aVk;
	Mod = aMod;
}

UINT ConEmuChord::GetModifiers(BYTE (&Mods)[3]) const
{
	UINT m = 0;
	for (size_t i = 0; gvkMatchList[i].Vk; i++)
	{
		if (Mod & gvkMatchList[i].Mod)
		{
			Mods[m] = gvkMatchList[i].Vk;
			m++;
			if (m >= countof(Mods))
				break;
		}
	}
	for (size_t k = m; k < countof(Mods); k++)
	{
		Mods[k] = 0;
	}
	return m;
}

bool ConEmuChord::IsEmpty() const
{
	return (Vk == 0);
}

bool ConEmuChord::IsEqual(const ConEmuChord& Key) const
{
	return IsEqual(Key.Vk, Key.Mod);
}

bool ConEmuChord::IsEqual(BYTE aVk, ConEmuModifiers aMod) const
{
	_ASSERTE(Vk && aVk);

	if (Vk != aVk)
		return false;

	// Правила проверки
	// Если требуется нажатие Ctrl, то неважно левый или правый нажат в действительности
	// Если требуется нажатие LCtrl/RCtrl, то именно этот модификатор должен присутствовать
	// При этом не должны быть нажаты другой модификатор (если не было задано явно Левый+Правый)
	// То же для Alt и Shift
	// Win и Apps проверяются без деления правый/левый

	ValidateChordMod(aMod);
	ValidateChordMod(Mod);

	if (!ConEmuChord::Compare(aMod, Mod))
		return false;

	// However, that is not enough
	// In some cases (both left/right modifiers set?)
	// we need to check flags in reverse mode too

	if (!ConEmuChord::Compare(Mod, aMod))
		return false;

	// OK, all tests passed, key match
	return true;
}

bool ConEmuChord::Compare(const ConEmuModifiers aMod1, const ConEmuModifiers aMod2)
{
	ConEmuModifiers nTest;

	for (size_t i = 0; i < gvkMatchList[i].Vk; i++)
	{
		const CEVkMatch& k = gvkMatchList[i];

		if (!k.Distinct)
		{	// Ctrl/LCtrl/RCtrl etc
			_ASSERTE((k.Left|k.Right) != cvk_NULL);
			nTest = (aMod1 & (k.Left|k.Right));
			if (nTest)
			{
				if ((aMod2 & (k.Left|k.Right)) != nTest)
				{
					// Allowed only in case of
					// 1) aMod2 has cvk_Ctrl (insensitive key)
					// 2) aMod1 has NOT both modifiers at once
					if ((aMod2 & k.Mod)
						&& (!(nTest & k.Left) || !(nTest & k.Right))
						&& (!(aMod2 & (k.Left|k.Right))))
					{
						i += 2; // skip next two keys (LCtr, RCtrl), they are already checked
						continue;
					}
					return false;
				}
				//PressTest |= nTest;
				//DePressMask &= ~(k.Mod|nTest); // Clear all Ctrl's bits
			}
			else if (aMod1 & k.Mod)
			{	// Insensitive (left or right)
				if (!(aMod2 & k.Mod))
					return false;
				//PressTest |= k.Mod;
				//DePressMask &= ~(k.Mod|k.Left|k.Right); // Clear all Ctrl's bits
			}
		}
		else
		{	// Win, Apps
			_ASSERTE((k.Left|k.Right) == cvk_NULL);
			if (aMod1 & k.Mod)
			{	// Insensitive key (no left/right)
				if (!(aMod2 & k.Mod))
					return false;
			}
		}

		if (!k.Distinct)
			i += 2; // skip next two keys (LCtr, RCtrl), they are already checked
	}

	return true;
}

#ifdef _DEBUG
/*
	Unit tests begin
*/
void ConEmuChord::ChordUnitTests()
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
			_ASSERTE(bMatch == Tests[i].Def[j].bEqual);
		}
	}
}
/*
	Unit tests end
*/
#endif
