
/*
Copyright (c) 2016-2017 Maximus5
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
#include "Common.h"
#include "MStrEsc.h"

/// Set escapes: wchar(13) --> "\\r", etc.
/// pszSrc and pszDst must point to different memory blocks
/// pszDst must have at least 5 bytes available (four "\\xFF" and final "\0")
void EscapeChar(LPCWSTR& pszSrc, LPWSTR& pszDst)
{
	_ASSERTE(pszSrc && pszDst);
	_ASSERTE(pszSrc != pszDst);

	LPCWSTR  pszCtrl = L"rntae\\\"";

	wchar_t wc = *pszSrc;
	switch (wc)
	{
	case L'"': // 34
		*(pszDst++) = L'\\';
		*(pszDst++) = L'"';
		pszSrc++;
		break;
	case L'\\': // 92
		*(pszDst++) = L'\\';
		pszSrc++;
		if (!*pszSrc || !wcschr(pszCtrl, *pszSrc))
			*(pszDst++) = L'\\';
		break;
	case L'\r': // 13
		*(pszDst++) = L'\\';
		*(pszDst++) = L'r';
		pszSrc++;
		break;
	case L'\n': // 10
		*(pszDst++) = L'\\';
		*(pszDst++) = L'n';
		pszSrc++;
		break;
	case L'\t': // 9
		*(pszDst++) = L'\\';
		*(pszDst++) = L't';
		pszSrc++;
		break;
	case L'\b': // 8 (BS)
		*(pszDst++) = L'\\';
		*(pszDst++) = L'b';
		pszSrc++;
		break;
	case 27: //ESC
		*(pszDst++) = L'\\';
		*(pszDst++) = L'e';
		pszSrc++;
		break;
	case L'\a': // 7 (BELL)
		*(pszDst++) = L'\\';
		*(pszDst++) = L'a';
		pszSrc++;
		break;
	default:
		// Escape (if possible) ASCII symbols with codes 01..31 (dec)
		if (wc < L' ')
		{
			wchar_t wcn = pszSrc[1];
			// If next string character is 'hexadecimal' digit - back conversion will be ambiguous
			if (!((wcn >= L'0' && wcn <= L'9') || (wcn >= L'a' && wcn <= L'f') || (wcn >= L'A' && wcn <= L'F')))
			{
				*(pszDst++) = L'\\';
				*(pszDst++) = L'x';
				// We've checked above for subsequent symbol to avoid ambiguous unescaping
				msprintf(pszDst, 3, L"%02X", (UINT)wc);
				pszDst+=2;
				break;
			}
		}
		*(pszDst++) = *(pszSrc++);
	}
}

/// Remove escapes: "\\r" --> wchar(13), etc.
void UnescapeChar(LPCWSTR& pszSrc, LPWSTR& pszDst)
{
	_ASSERTE(pszSrc && pszDst);

	if (*pszSrc == L'\\')
	{
		// -- Must be the same set in "Set escapes"
		switch (pszSrc[1])
		{
		case L'"':
			*(pszDst++) = L'"';
			pszSrc += 2;
			break;
		case L'\\':
			*(pszDst++) = L'\\';
			pszSrc += 2;
			break;
		case L'r': case L'R':
			*(pszDst++) = L'\r';
			pszSrc += 2;
			break;
		case L'n': case L'N':
			*(pszDst++) = L'\n';
			pszSrc += 2;
			break;
		case L't': case L'T':
			*(pszDst++) = L'\t';
			pszSrc += 2;
			break;
		case L'b': case L'B':
			*(pszDst++) = L'\b';
			pszSrc += 2;
			break;
		case L'e': case L'E':
			*(pszDst++) = 27; // ESC
			pszSrc += 2;
			break;
		case L'a': case L'A':
			*(pszDst++) = L'\a'; // BELL
			pszSrc += 2;
			break;
		case L'x':
		{
			// Up to four hexadecimal letters are allowed
			wchar_t sTemp[5] = L"", *pszEnd = NULL;
			lstrcpyn(sTemp, pszSrc+2, 5);
			UINT wc = wcstoul(sTemp, &pszEnd, 16);
			if (pszEnd > sTemp)
			{
				*(pszDst++) = LOWORD(wc);
				pszSrc += (pszEnd - sTemp) + 2;
			}
			else
			{
				*(pszDst++) = *(pszSrc++);
			}
			break;
		}
		default:
			*(pszDst++) = *(pszSrc++);
		}
	}
	else
	{
		*(pszDst++) = *(pszSrc++);
	}
}

bool CheckStrForSpecials(LPCWSTR pszStr, bool* pbSlash /*= NULL*/, bool* pbOthers /*= NULL*/)
{
	// Intended for GuiMacro representation
	// Если строка содержит из спец-символов ТОЛЬКО "\" (пути)
	// то ее удобнее показывать как "Verbatim", иначе - C-String
	// Однозначно показывать как C-String нужно те строки, которые
	// содержат переводы строк, Esc, табуляции и пр.

	bool bSlash = false, bOthers = false;

	if (pszStr && *pszStr)
	{
		const wchar_t szEscaped[] = L"\r\n\t\a\x1B\"";

		bSlash = (wcschr(pszStr, L'\\') != NULL);
		bOthers = (wcspbrk(pszStr, szEscaped) != NULL);
	}

	if (pbSlash)
		*pbSlash = bSlash;
	if (pbOthers)
		*pbOthers = bOthers;
	return (bSlash || bOthers);
}
