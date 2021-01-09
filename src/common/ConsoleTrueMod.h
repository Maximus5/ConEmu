
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

#pragma once

#include <cstdint>
#include <pshpack1.h>  // NOLINT(clang-diagnostic-pragma-pack)

namespace ConEmu {

// Flags for color and styles applied on a cell (TrueColor memory mapped buffer)
enum class ColorFlags : uint64_t
{
	None = 0,

	// Cell contains 24bit foreground part
	Fg24Bit = 0x0000000000000001ULL,
	// Cell contains 24bit background part
	Bg24Bit = 0x0000000000000002ULL,

	// reserved. set in pos where "\t" was written
	TabChar = 0x0100000000000000ULL,
	// reserved. set in pos after TabChar (where spaces actually was written)
	TabSpace = 0x0200000000000000ULL,

	// Text is crossed out
	Crossed = 0x0800000000000000ULL,
	// Bold font is used
	Bold = 0x1000000000000000ULL,
	// Italic font is used
	Italic = 0x2000000000000000ULL,
	// Text is underlined
	Underline = 0x4000000000000000ULL,

	// Foreground and background colors are exchanged
	Reverse = 0x8000000000000000ULL,
};

inline ColorFlags operator|(const ColorFlags e1, const ColorFlags e2)
{
	return static_cast<ColorFlags>(static_cast<uint64_t>(e1) | static_cast<uint64_t>(e2));
}

inline ColorFlags operator|=(ColorFlags& e1, const ColorFlags e2)
{
	return e1 = static_cast<ColorFlags>(static_cast<uint64_t>(e1) | static_cast<uint64_t>(e2));
}

inline bool operator&(const ColorFlags e1, const ColorFlags e2)
{
	return static_cast<ColorFlags>(static_cast<uint64_t>(e1) & static_cast<uint64_t>(e2)) != ColorFlags::None;
}

struct Color
{
	ColorFlags Flags;
	COLORREF ForegroundColor;
	COLORREF BackgroundColor;
};

}

#include <poppack.h>  // NOLINT(clang-diagnostic-pragma-pack)
