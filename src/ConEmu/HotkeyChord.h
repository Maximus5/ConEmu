
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


#pragma once

#include "../common/Common.h"

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
	static void ChordUnitTests();
	static void ValidateChordMod(ConEmuModifiers aMod);
	#endif

	void  Set(BYTE aVk = 0, ConEmuModifiers aMod = cvk_NULL);
	UINT  GetModifiers(BYTE (&Mods)[3]) const;
	bool  IsEmpty() const;
	bool  IsEqual(const ConEmuChord& Key) const;
	bool  IsEqual(BYTE aVk, ConEmuModifiers aMod) const;

private:
	static bool Compare(const ConEmuModifiers aMod1, const ConEmuModifiers aMod2);
};

extern const ConEmuChord NullChord;
