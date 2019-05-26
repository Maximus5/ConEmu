
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


#define HIDE_USE_EXCEPTION_INFO

#include "Common.h"
#include "EnvVar.h"
#include "MStrDup.h"


wchar_t* ExpandEnvStr(LPCWSTR pszCommand)
{
	if (!pszCommand || !*pszCommand)
		return NULL;

	DWORD cchMax = ExpandEnvironmentStrings(pszCommand, NULL, 0);
	if (!cchMax)
		return lstrdup(pszCommand);

	wchar_t* pszExpand = (wchar_t*)malloc((cchMax+2)*sizeof(*pszExpand));
	if (pszExpand)
	{
		pszExpand[0] = 0;
		pszExpand[cchMax] = 0xFFFF;
		pszExpand[cchMax+1] = 0xFFFF;

		DWORD nExp = ExpandEnvironmentStrings(pszCommand, pszExpand, cchMax);
		if (nExp && (nExp <= cchMax) && *pszExpand)
			return pszExpand;

		SafeFree(pszExpand);
	}
	return NULL;
}

wchar_t* GetEnvVar(LPCWSTR VarName)
{
	if (!VarName || !*VarName)
	{
		return NULL;
	}

	DWORD cchMax, nRc, nErr;

	nRc = GetEnvironmentVariable(VarName, NULL, 0);
	if (nRc == 0)
	{
		// Weird. This may be empty variable or not existing variable
		nErr = GetLastError();
		if (nErr == ERROR_ENVVAR_NOT_FOUND)
			return NULL;
		return (wchar_t*)calloc(3,sizeof(wchar_t));
	}

	cchMax = nRc+2;
	wchar_t* pszVal = (wchar_t*)calloc(cchMax,sizeof(*pszVal));
	if (!pszVal)
	{
		_ASSERTE((pszVal!=NULL) && "GetEnvVar memory allocation failed");
		return NULL;
	}

	nRc = GetEnvironmentVariable(VarName, pszVal, cchMax);
	if ((nRc == 0) || (nRc >= cchMax))
	{
		_ASSERTE(nRc > 0 && nRc < cchMax);
		SafeFree(pszVal);
	}

	return pszVal;
}




CEnvStrings::CEnvStrings(LPWSTR pszStrings /* = GetEnvironmentStringsW() */)
	: ms_Strings(pszStrings)
	, mcch_Length(0)
	, mn_Count(0)
{
	// Must be ZZ-terminated
	if (pszStrings && *pszStrings)
	{
		// Parse variables block, determining MAX length
		LPCWSTR pszSrc = pszStrings;
		while (*pszSrc)
		{
			pszSrc += lstrlen(pszSrc) + 1;
			mn_Count++;
		}
		mcch_Length = pszSrc - pszStrings + 1;
		// mn_Count holds count of 'lines' like "name=value\0"
	}
}

CEnvStrings::~CEnvStrings()
{
	if (ms_Strings)
	{
		FreeEnvironmentStringsW((LPWCH)ms_Strings);
		ms_Strings = NULL;
	}
}



CEnvRestorer::~CEnvRestorer()
{
	if (mb_RestoreEnvVar && ms_VarName && ms_OldValue)
	{
		SetEnvironmentVariable(ms_VarName, ms_OldValue);
	}
}

void CEnvRestorer::Clear()
{
	mb_RestoreEnvVar = false;
	ms_VarName.Clear();
	ms_OldValue.Clear();
}

void CEnvRestorer::SavePathVar(const wchar_t* asCurPath)
{
	// Will restore environment variable "PATH" in destructor
	if (ms_OldValue.Set(asCurPath))
	{
		ms_VarName = L"PATH";
		mb_RestoreEnvVar = true;
	}
}

void CEnvRestorer::SaveEnvVar(const wchar_t*  asVarName, const wchar_t*  asNewValue)
{
	if (!asVarName || !*asVarName)
		return;

	_ASSERTE(!mb_RestoreEnvVar);
	ms_VarName = asVarName;
	ms_OldValue.Attach(GetEnvVar(asVarName));
	mb_RestoreEnvVar = true;

	SetEnvironmentVariable(asVarName, asNewValue);
}
