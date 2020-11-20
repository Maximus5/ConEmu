
/*
Copyright (c) 2016-present Maximus5
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

#include "defines.h"
#include "CEStr.h"

/// Set escapes: wchar(13) --> "\\r", etc.
/// pszSrc and pszDst must point to different memory blocks
/// pszDst must have at least 5 wchars available (four "\\xFF" and final "\0")
bool EscapeChar(LPCWSTR& pszSrc, LPWSTR& pszDst);

/// Remove escapes: "\\r" --> wchar(13), etc.
bool UnescapeChar(LPCWSTR& pszSrc, LPWSTR& pszDst);

/// Set escapes: wchar(13) --> "\\r", etc.
/// pszSrc and pszDst must point to different memory blocks
/// pszDst must have at least 1+4*len(pszSrc) wchars available (four "\\xFF" and final "\0")
bool EscapeString(LPCWSTR& pszSrc, LPWSTR& pszDst);

bool UnescapeString(LPCWSTR& pszSrc, LPWSTR& pszDst);

/// Intended for GuiMacro representation
/// If *pszStr* doesn't contain special symbols with ONLY exception of "\" (paths)
/// it's more convenient to represent it as *Verbatim* string, otherwise - *C-String*.
/// Always show as *C-String* those strings, which contain CRLF, Esc, tabs etc.
bool CheckStrForSpecials(LPCWSTR pszStr, bool* pbSlash = nullptr, bool* pbOthers = nullptr);

/// Bitmask flags for MakeOneLinerString
enum class MakeOneLinerFlags : int
{
	None = 0,
	TrimTailing = 1,
};

MakeOneLinerFlags operator|(MakeOneLinerFlags e1, MakeOneLinerFlags e2);
MakeOneLinerFlags operator&(MakeOneLinerFlags e1, MakeOneLinerFlags e2);

/// The function replaces all \r\n\t with spaces to paste the string safely in command prompt
CEStr MakeOneLinerString(const CEStr& source, MakeOneLinerFlags flags);
