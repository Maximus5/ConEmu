
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
#include "Common.h"
#include "CEStr.h"
#include "WObjects.h"

CEStr::CEStr()
{
	mn_MaxCount = 0; ms_Val = NULL;
	mb_RestoreEnvVar = false;
	ms_RestoreVarName[0] = 0;
	Empty();
}

CEStr::CEStr(wchar_t* RVAL_REF asPtr)
{
	mn_MaxCount = 0; ms_Val = NULL;
	mpsz_Dequoted = NULL;
	mb_RestoreEnvVar = false;
	ms_RestoreVarName[0] = 0;
	AttachInt(asPtr);
}

CEStr& CEStr::operator=(wchar_t* RVAL_REF asPtr)
{
	AttachInt(asPtr);
	return *this;
}

CEStr& CEStr::operator=(const wchar_t* asPtr)
{
	Set(asPtr);
	return *this;
}

CEStr::~CEStr()
{
	if (mb_RestoreEnvVar && *ms_RestoreVarName && !IsEmpty())
	{
		SetEnvironmentVariable(ms_RestoreVarName, ms_Val);
	}

	SafeFree(ms_Val);
}

wchar_t* CEStr::GetBuffer(INT_PTR cchMaxLen)
{
	if (cchMaxLen <= 0)
	{
		_ASSERTE(cchMaxLen>0);
		return NULL;
	}


	INT_PTR nOldLen = (ms_Val && (mn_MaxCount > 0)) ? (mn_MaxCount-1) : 0;

	if (!ms_Val || (cchMaxLen >= mn_MaxCount))
	{
		INT_PTR nNewMaxLen = max(mn_MaxCount,cchMaxLen+1);
		if (ms_Val)
		{
			ms_Val = (wchar_t*)realloc(ms_Val, nNewMaxLen*sizeof(*ms_Val));
		}
		else
		{
			ms_Val = (wchar_t*)malloc(nNewMaxLen*sizeof(*ms_Val));
		}
		mn_MaxCount = nNewMaxLen;
	}

	if (ms_Val)
	{
		_ASSERTE(cchMaxLen>0 && nOldLen>=0);
		ms_Val[min(cchMaxLen,nOldLen)] = 0;
	}

	return ms_Val;
}

wchar_t* CEStr::Detach()
{
	wchar_t* psz = ms_Val;
	ms_Val = NULL;
	mn_MaxCount = 0;

	return psz;
}

LPCWSTR CEStr::Attach(wchar_t* RVAL_REF asPtr)
{
	return AttachInt(asPtr);
}

LPCWSTR CEStr::AttachInt(wchar_t*& asPtr)
{
	if (ms_Val == asPtr)
	{
		return ms_Val; // Already
	}

	Empty();
	SafeFree(ms_Val);
	if (asPtr)
	{
		ms_Val = asPtr;
		mn_MaxCount = lstrlen(asPtr)+1;
	}

	return ms_Val;
}

void CEStr::Empty()
{
	if (ms_Val)
	{
		*ms_Val = 0;
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

bool CEStr::IsEmpty()
{
	return (!ms_Val || !*ms_Val);
}

LPCWSTR CEStr::Set(LPCWSTR asNewValue, INT_PTR anChars /*= -1*/)
{
	if (asNewValue)
	{
		INT_PTR nNewLen = (anChars == -1) ? lstrlen(asNewValue) : anChars;
		if (nNewLen <= 0)
		{
			//_ASSERTE(FALSE && "Check, if caller really need to set empty string???");
			if (GetBuffer(1))
				ms_Val[0] = 0;
		}
		else if (GetBuffer(nNewLen))
		{
			_ASSERTE(mn_MaxCount > nNewLen); // Must be set in GetBuffer
			_wcscpyn_c(ms_Val, mn_MaxCount, asNewValue, nNewLen);
		}
	}
	else
	{
		Empty();
	}

	return ms_Val;
}

void CEStr::SavePathVar(LPCWSTR asCurPath)
{
	// Will restore environment variable "PATH" in destructor
	if (Set(asCurPath))
	{
		mb_RestoreEnvVar = true;
		lstrcpyn(ms_RestoreVarName, L"PATH", countof(ms_RestoreVarName));
	}
}

void CEStr::SaveEnvVar(LPCWSTR asVarName, LPCWSTR asNewValue)
{
	if (!asVarName || !*asVarName)
		return;

	_ASSERTE(!mb_RestoreEnvVar);
	Empty();
	SafeFree(ms_Val);
	Attach(GetEnvVar(asVarName));

	mb_RestoreEnvVar = true;
	lstrcpyn(ms_RestoreVarName, asVarName, countof(ms_RestoreVarName));
	SetEnvironmentVariable(asVarName, asNewValue);
}

void CEStr::SetAt(INT_PTR nIdx, wchar_t wc)
{
	if (ms_Val && (nIdx < mn_MaxCount))
	{
		ms_Val[nIdx] = wc;
	}
}

void CEStr::GetPosFrom(const CEStr& arg)
{
	mpsz_Dequoted = arg.mpsz_Dequoted;
	mn_TokenNo = arg.mn_TokenNo;
	mn_CmdCall = arg.mn_CmdCall;
	#ifdef _DEBUG
	ms_LastTokenEnd = arg.ms_LastTokenEnd;
	lstrcpyn(ms_LastTokenSave, arg.ms_LastTokenSave, countof(ms_LastTokenSave));
	#endif
}
