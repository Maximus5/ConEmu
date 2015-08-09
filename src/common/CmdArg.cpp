
/*
Copyright (c) 2009-2015 Maximus5
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
#include "common.hpp"
#include "CmdArg.h"
#include "WObjects.h"

CmdArg::CmdArg()
{
	mn_MaxLen = 0; ms_Arg = NULL;
	mb_RestoreEnvVar = false;
	ms_RestoreVarName[0] = 0;
	Empty();
}

CmdArg::CmdArg(wchar_t* RVAL_REF asPtr)
{
	mn_MaxLen = 0; ms_Arg = NULL;
	mpsz_Dequoted = NULL;
	mb_RestoreEnvVar = false;
	ms_RestoreVarName[0] = 0;
	AttachInt(asPtr);
}

CmdArg& CmdArg::operator=(wchar_t* RVAL_REF asPtr)
{
	AttachInt(asPtr);
	return *this;
}

CmdArg& CmdArg::operator=(const wchar_t* asPtr)
{
	Set(asPtr);
	return *this;
}

CmdArg::~CmdArg()
{
	if (mb_RestoreEnvVar && *ms_RestoreVarName && !IsEmpty())
	{
		SetEnvironmentVariable(ms_RestoreVarName, ms_Arg);
	}

	SafeFree(ms_Arg);
}

wchar_t* CmdArg::GetBuffer(INT_PTR cchMaxLen)
{
	if (cchMaxLen <= 0)
	{
		_ASSERTE(cchMaxLen>0);
		return NULL;
	}

	INT_PTR nOldLen = (ms_Arg && (mn_MaxLen > 0)) ? (mn_MaxLen-1) : 0;

	if (!ms_Arg || (cchMaxLen >= mn_MaxLen))
	{
		INT_PTR nNewMaxLen = max(mn_MaxLen,cchMaxLen+1);
		if (ms_Arg)
		{
			ms_Arg = (wchar_t*)realloc(ms_Arg, nNewMaxLen*sizeof(*ms_Arg));
		}
		else
		{
			ms_Arg = (wchar_t*)malloc(nNewMaxLen*sizeof(*ms_Arg));
		}
		mn_MaxLen = nNewMaxLen;
	}

	if (ms_Arg)
	{
		_ASSERTE(cchMaxLen>0 && nOldLen>=0);
		ms_Arg[min(cchMaxLen,nOldLen)] = 0;
	}

	return ms_Arg;
}

wchar_t* CmdArg::Detach()
{
	wchar_t* psz = ms_Arg;
	ms_Arg = NULL;
	mn_MaxLen = 0;

	return psz;
}

LPCWSTR CmdArg::Attach(wchar_t* RVAL_REF asPtr)
{
	return AttachInt(asPtr);
}

LPCWSTR CmdArg::AttachInt(wchar_t*& asPtr)
{
	if (ms_Arg == asPtr)
	{
		return ms_Arg; // Already
	}

	Empty();
	SafeFree(ms_Arg);
	if (asPtr)
	{
		ms_Arg = asPtr;
		mn_MaxLen = lstrlen(asPtr)+1;
	}

	return ms_Arg;
}

void CmdArg::Empty()
{
	if (ms_Arg)
	{
		*ms_Arg = 0;
	}

	mn_TokenNo = 0;
	mn_CmdCall = cc_Undefined;
	mpsz_Dequoted = NULL;
	#ifdef _DEBUG
	ms_LastTokenEnd = NULL;
	ms_LastTokenSave[0] = 0;
	#endif
	mb_RestoreEnvVar = false;
	ms_RestoreVarName[0] = 0;
}

bool CmdArg::IsEmpty()
{
	return (!ms_Arg || !*ms_Arg);
}

LPCWSTR CmdArg::Set(LPCWSTR asNewValue, INT_PTR anChars /*= -1*/)
{
	if (asNewValue)
	{
		INT_PTR nNewLen = (anChars == -1) ? lstrlen(asNewValue) : anChars;
		if (nNewLen <= 0)
		{
			//_ASSERTE(FALSE && "Check, if caller really need to set empty string???");
			if (GetBuffer(1))
				ms_Arg[0] = 0;
		}
		else if (GetBuffer(nNewLen))
		{
			_ASSERTE(mn_MaxLen > nNewLen); // Must be set in GetBuffer
			_wcscpyn_c(ms_Arg, mn_MaxLen, asNewValue, nNewLen);
		}
	}
	else
	{
		Empty();
	}

	return ms_Arg;
}

void CmdArg::SavePathVar(LPCWSTR asCurPath)
{
	// Will restore environment variable "PATH" in destructor
	if (Set(asCurPath))
	{
		mb_RestoreEnvVar = true;
		lstrcpyn(ms_RestoreVarName, L"PATH", countof(ms_RestoreVarName));
	}
}

void CmdArg::SaveEnvVar(LPCWSTR asVarName, LPCWSTR asNewValue)
{
	if (!asVarName || !*asVarName)
		return;

	_ASSERTE(!mb_RestoreEnvVar);
	Empty();
	SafeFree(ms_Arg);
	Attach(GetEnvVar(asVarName));

	mb_RestoreEnvVar = true;
	lstrcpyn(ms_RestoreVarName, asVarName, countof(ms_RestoreVarName));
	SetEnvironmentVariable(asVarName, asNewValue);
}

void CmdArg::SetAt(INT_PTR nIdx, wchar_t wc)
{
	if (ms_Arg && (nIdx < mn_MaxLen))
	{
		ms_Arg[nIdx] = wc;
	}
}

void CmdArg::GetPosFrom(const CmdArg& arg)
{
	mpsz_Dequoted = arg.mpsz_Dequoted;
	mn_TokenNo = arg.mn_TokenNo;
	mn_CmdCall = arg.mn_CmdCall;
	#ifdef _DEBUG
	ms_LastTokenEnd = arg.ms_LastTokenEnd;
	lstrcpyn(ms_LastTokenSave, arg.ms_LastTokenSave, countof(ms_LastTokenSave));
	#endif
}
