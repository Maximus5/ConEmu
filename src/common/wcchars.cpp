
/*
Copyright (c) 2015-2017 Maximus5
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

#include "defines.h"
#include "wcchars.h"

// Convert surrogate pair into 32-bit unicode codepoint
ucs32 ucs32_from_wchar(const wchar_t* pch, bool& has_trail)
{
	if (!has_trail || !(has_trail = IS_SURROGATE_PAIR(pch[0], pch[1])))
		return *pch;

	_ASSERTE(sizeof(int) == 4);

	// constants
	const ucs32 SURROGATE_OFFSET = 0x10000 - (0xD800 << 10) - 0xDC00; // -0x35FDC00
	// computation
	ucs32 codepoint = (pch[0] << 10) + pch[1] + SURROGATE_OFFSET;

	// example: "𝔸" is pair of {0xD835 0xDD38} ==> 0x1D538 : MATHEMATICAL DOUBLE-STRUCK CAPITAL A

	return codepoint;
}

// Convert 32-bit unicode codepoint into two surrogate pair
const wchar_t* wchar_from_ucs32(const ucs32 codepoint, wchar_t (&buffer)[3])
{
	buffer[0] = (wchar_t)((codepoint >> 10) + 0xD7C0);
	buffer[1] = (wchar_t)((codepoint & 0x3FF) | 0xDC00);
	buffer[2] = 0;
	return buffer;
}
