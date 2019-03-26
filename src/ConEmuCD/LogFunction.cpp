
/*
Copyright (c) 2009-present Maximus5
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

#include "LogFunction.h"
#include "ConEmuSrv.h"
#include <algorithm>
#include <Windows.h>

std::atomic_int32_t CLogFunction::m_FnLevel{0}; // Simple, without per-thread devision

CLogFunction::CLogFunction(const char* asFnName)
{
	int nLen = MultiByteToWideChar(CP_ACP, 0, asFnName, -1, NULL, 0);
	wchar_t sBuf[80] = L"";
	wchar_t *pszBuf = NULL;
	if (nLen >= 80)
		pszBuf = (wchar_t*)calloc(nLen+1,sizeof(*pszBuf));
	else
		pszBuf = sBuf;

	MultiByteToWideChar(CP_ACP, 0, asFnName, -1, pszBuf, nLen+1);

	DoLogFunction(pszBuf);

	if (pszBuf != sBuf)
		SafeFree(pszBuf);
}

CLogFunction::CLogFunction(const wchar_t* asFnName)
{
	DoLogFunction(asFnName);
}

void CLogFunction::DoLogFunction(const wchar_t* asFnName)
{
	if (mb_Logged)
		return;

	const auto lLevel = std::min<int32_t>(++m_FnLevel, 20);
	mb_Logged = true;

	if (!gpLogSize) return;

	const int cchFnInfo = 120;
	wchar_t cFnInfo[cchFnInfo];
	wchar_t* pc = cFnInfo;
	for (LONG l = 1; l < lLevel; l++)
	{
		*(pc++) = L' '; *(pc++) = L' '; *(pc++) = L' ';
	}
	*pc = 0;

	const int nPrefix = static_cast<int>(pc - cFnInfo);
	const int cchSpaceLeft = cchFnInfo - nPrefix;
	const int nFnLen = lstrlen(asFnName);

	if (nFnLen < cchSpaceLeft)
	{
		lstrcpyn(pc, asFnName, cchSpaceLeft);
		LogString(cFnInfo);
	}
	else
	{
		LogString(CEStr(cFnInfo, asFnName));
	}
}

CLogFunction::~CLogFunction()
{
	if (!mb_Logged)
		return;
	--m_FnLevel;
}
