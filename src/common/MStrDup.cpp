
/*
Copyright (c) 2009-2017 Maximus5
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
#include "MStrDup.h"

char* lstrdup(const char* asText)
{
	int nLen = asText ? lstrlenA(asText) : 0;
	char* psz = (char*)malloc(nLen+1);

	if (nLen)
		StringCchCopyA(psz, nLen+1, asText);
	else
		psz[0] = 0;

	return psz;
}

wchar_t* lstrdup(const wchar_t* asText, size_t cchExtraSizeAdd /* = 0 */)
{
	size_t nLen = asText ? lstrlenW(asText) : 0;
	wchar_t* psz = (wchar_t*)malloc((nLen+1+cchExtraSizeAdd) * sizeof(*psz));

	if (nLen)
		StringCchCopyW(psz, nLen+1, asText);

	// Ensure AsciiZ
	psz[nLen] = 0;

	return psz;
}

wchar_t* lstrdupW(const char* asText, UINT cp /*= CP_ACP*/)
{
	int nLen = asText ? lstrlenA(asText) : 0;
	wchar_t* psz = (wchar_t*)malloc((nLen+1) * sizeof(*psz));

	if (nLen)
		MultiByteToWideChar(cp, 0, asText, nLen+1, psz, nLen+1);
	else
		psz[0] = 0;

	return psz;
}

char* lstrdupA(const wchar_t* asText, UINT cp /*= CP_ACP*/, int* pnLen /*= NULL*/)
{
	if (!asText)
		return NULL;

	if (!*asText)
	{
		if (pnLen) *pnLen = 1;
		return lstrdup("");
	}

	int nLen = WideCharToMultiByte(cp, 0, asText, -1, NULL, 0, NULL, NULL);
	char* psz = NULL;

	if (nLen > 0)
	{
		psz = (char*)malloc(nLen);
		if (!psz)
			return NULL; // memory allocation failure
		nLen = WideCharToMultiByte(cp, 0, asText, -1, psz, nLen, NULL, NULL);
		if (nLen <= 1)
		{
			SafeFree(psz);
		}
		else
		{
			if (pnLen) *pnLen = nLen - 1; // Do not include termination zero
		}
	}

	return psz;
}

wchar_t* lstrmerge(const wchar_t* asStr1, const wchar_t* asStr2, const wchar_t* asStr3 /*= NULL*/, const wchar_t* asStr4 /*= NULL*/, const wchar_t* asStr5 /*= NULL*/, const wchar_t* asStr6 /*= NULL*/, const wchar_t* asStr7 /*= NULL*/, const wchar_t* asStr8 /*= NULL*/, const wchar_t* asStr9 /*= NULL*/)
{
	size_t cchMax = 1;
	const size_t Count = 9;
	size_t cch[Count] = {};
	const wchar_t* pszStr[Count] = {asStr1, asStr2, asStr3, asStr4, asStr5, asStr6, asStr7, asStr8, asStr9};

	for (size_t i = 0; i < Count; i++)
	{
		cch[i] = pszStr[i] ? lstrlen(pszStr[i]) : 0;
		cchMax += cch[i];
	}

	wchar_t* pszRet = (wchar_t*)malloc(cchMax*sizeof(*pszRet));
	if (!pszRet)
		return NULL;
	*pszRet = 0;
	wchar_t* psz = pszRet;

	for (size_t i = 0; i < Count; i++)
	{
		if (!cch[i])
			continue;

		_wcscpy_c(psz, cch[i]+1, pszStr[i]);
		psz += cch[i];
	}

	return pszRet;
}

bool lstrmerge(wchar_t** apsStr1, const wchar_t* asStr2, const wchar_t* asStr3 /*= NULL*/, const wchar_t* asStr4 /*= NULL*/, const wchar_t* asStr5 /*= NULL*/, const wchar_t* asStr6 /*= NULL*/, const wchar_t* asStr7 /*= NULL*/, const wchar_t* asStr8 /*= NULL*/, const wchar_t* asStr9 /*= NULL*/)
{
	_ASSERTE(apsStr1!=NULL);

	wchar_t* pszNew = lstrmerge(*apsStr1, asStr2, asStr3, asStr4, asStr5, asStr6, asStr7, asStr8, asStr9);
	if (!pszNew)
		return false;

	if (*apsStr1)
		free(*apsStr1);
	*apsStr1 = pszNew;

	return true;
}
