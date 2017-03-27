
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

//#ifdef _DEBUG
//#define USE_LOCK_SECTION
//#endif

#define HIDE_USE_EXCEPTION_INFO
#include "Common.h"
#include "MModule.h"
#include "WSession.h"

BOOL apiQuerySessionID(DWORD nPID, DWORD& nSessionID)
{
	BOOL bSucceeded = FALSE;
	typedef BOOL (WINAPI* ProcessIdToSessionId_t)(DWORD dwProcessId, DWORD *pSessionId);
	ProcessIdToSessionId_t processIdToSessionId = NULL;
	MModule kernel32(L"kernel32.dll");
	if (kernel32.GetProcAddress("ProcessIdToSessionId", processIdToSessionId))
	{
		bSucceeded = processIdToSessionId(GetCurrentProcessId(), &nSessionID);
	}
	return bSucceeded;
}

LPCWSTR apiQuerySessionID()
{
	static wchar_t szSessionId[16] = L"";
	if (!*szSessionId)
	{
		DWORD nSessionId = 0;
		BOOL bSucceeded = apiQuerySessionID(GetCurrentProcessId(), nSessionId);
		if (bSucceeded)
			_wsprintf(szSessionId, SKIPCOUNT(szSessionId) L"%u", nSessionId);
		else
			wcscpy_c(szSessionId, L"?");
	}
	return szSessionId;
}

DWORD apiGetConsoleSessionID()
{
	DWORD nSessionId = 0;
	DWORD (WINAPI* wtsGetActiveConsoleSessionId)(void) = NULL;
	MModule kernel32(L"kernel32.dll");
	if (kernel32.GetProcAddress("WTSGetActiveConsoleSessionId", wtsGetActiveConsoleSessionId))
	{
		nSessionId = wtsGetActiveConsoleSessionId();
	}
	return nSessionId;
}
