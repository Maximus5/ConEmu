
/*
Copyright (c) 2016 Maximus5
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
#include "MGuiMacro.h"
#include "MStrDup.h"
#include "MStrEsc.h"

// Return Root XML
const wchar_t* CreateRootInfoXml(LPCWSTR asRootExeName, CESERVER_ROOT_INFO* pInfo, CEStr& rsInfo)
{
	wchar_t szTemp[MAX_PATH*4] = L"";

	rsInfo.Set(L"<Root\n");

	// Was process started?
	if (pInfo && pInfo->nPID)
		lstrmerge(&rsInfo.ms_Val, L"\tState=\"", pInfo->bRunning ? L"Running" : L"Exited", L"\"\n");
	else if (asRootExeName)
		lstrmerge(&rsInfo.ms_Val, L"\tState=\"", L"NotStarted", L"\"\n");
	else
		lstrmerge(&rsInfo.ms_Val, L"\tState=\"", L"Empty", L"\"\n");

	// Name="cmd.exe"
	// asRootExeName is NULL if there are no VCon at all
	if (asRootExeName)
	{
		szTemp[0] = 0;
		if (*asRootExeName)
		{
			LPCWSTR pszName = asRootExeName;
			LPWSTR pszReady = szTemp;
			_ASSERTE(wcslen(pszName)*4 < countof(szTemp));
			INT_PTR cchMax = countof(szTemp) - 1;
			while (*pszName && ((pszReady - szTemp) < cchMax))
			{
				EscapeChar(pszName, pszReady);
			}
			*pszReady = 0;
		}
		lstrmerge(&rsInfo.ms_Val, L"\tName=\"", szTemp[0] ? szTemp : L"<Unknown>", L"\"\n");
	}

	// Was process started?
	if (pInfo && pInfo->nPID)
	{
		lstrmerge(&rsInfo.ms_Val, L"\tPID=\"", _ultow(pInfo->nPID, szTemp, 10), L"\"\n");
		lstrmerge(&rsInfo.ms_Val, L"\tExitCode=\"", _ultow(pInfo->nExitCode, szTemp, 10), L"\"\n");
		lstrmerge(&rsInfo.ms_Val, L"\tUpTime=\"", _ultow(pInfo->nUpTime, szTemp, 10), L"\"\n");
	}

	lstrmerge(&rsInfo.ms_Val, L"/>\n");

	return rsInfo.ms_Val;
}
