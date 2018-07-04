
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

#define HIDE_USE_EXCEPTION_INFO
#include "Common.h"
#include "CEStr.h"
#include "MStrDup.h"
#include "WObjects.h"

#if CE_UNIT_TEST==1
	#include <stdio.h>
	extern bool gbVerifyVerbose;
	#define CESTRLOG0(msg) if (gbVerifyVerbose) printf("  %s\n",(msg))
	#define CESTRLOG1(fmt,a1) if (gbVerifyVerbose) printf("  " fmt "\n",(a1))
	#define CESTRLOG2(fmt,a1,a2) if (gbVerifyVerbose) printf("  " fmt "\n",(a1),(a2))
	#define CESTRLOG3(fmt,a1,a2,a3) if (gbVerifyVerbose) printf("  " fmt "\n",(a1),(a2),(a3))
#else
	#define CESTRLOG0(msg)
	#define CESTRLOG1(fmt,a1)
	#define CESTRLOG2(fmt,a1,a2)
	#define CESTRLOG3(fmt,a1,a2,a3)
#endif


CEStr::CEStr()
{
	CESTRLOG0("CEStr::CEStr()");
	Empty();
}

CEStr::CEStr(const wchar_t* asStr1, const wchar_t* asStr2/*= NULL*/, const wchar_t* asStr3/*= NULL*/, const wchar_t* asStr4/*= NULL*/, const wchar_t* asStr5/*= NULL*/, const wchar_t* asStr6/*= NULL*/, const wchar_t* asStr7/*= NULL*/, const wchar_t* asStr8/*= NULL*/, const wchar_t* asStr9/*= NULL*/)
{
	CESTRLOG3("CEStr::CEStr(const wchar_t* x%p, x%p, x%p, ...)", asStr1, asStr2, asStr3);
	Empty();
	wchar_t* lpszMerged = lstrmerge(asStr1, asStr2, asStr3, asStr4, asStr5, asStr6, asStr7, asStr8, asStr9);
	AttachInt(lpszMerged);
}

CEStr::CEStr(CEStr&& asStr)
{
	std::swap(ms_Val, asStr.ms_Val);
	std::swap(mn_MaxCount, asStr.mn_MaxCount);
}

CEStr::CEStr(const CEStr& asStr)
{
	Set(asStr.ms_Val);
}

CEStr::CEStr(wchar_t*&& asPtr)
{
	CESTRLOG1("CEStr::CEStr(wchar_t* RVAL_REF x%p)", asPtr);
	Empty();
	AttachInt(asPtr);
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
	mb_Quoted = false;
	#ifdef _DEBUG
	ms_LastTokenEnd = NULL;
	ms_LastTokenSave[0] = 0;
	#endif
}

CEStr::operator LPCWSTR() const
{
	CESTRLOG0("CEStr::LPCWSTR()");
	return ms_Val;
}

CEStr::operator bool() const
{
	return (!IsEmpty());
}

LPCWSTR CEStr::c_str(LPCWSTR asNullSubstitute /*= NULL*/) const
{
	CESTRLOG0("CEStr::c_str()");
	return ms_Val ? ms_Val : asNullSubstitute;
}

// cchMaxCount - including terminating \0
LPCWSTR CEStr::Right(INT_PTR cchMaxCount) const
{
	CESTRLOG1("CEStr::Right(%i)", (int)cchMaxCount);
	if (cchMaxCount <= 0)
	{
		_ASSERTE(cchMaxCount > 0);
		return NULL;
	}

	if (!ms_Val || !*ms_Val)
		return ms_Val;

	INT_PTR iLen = GetLen();
	if (iLen >= cchMaxCount)
		return (ms_Val + (iLen - cchMaxCount + 1));
	return ms_Val;
}

LPCWSTR CEStr::Mid(INT_PTR cchOffset) const
{
	CESTRLOG1("CEStr::Mid(%i)", cchOffset);

	if (!ms_Val || (cchOffset < 0))
	{
		_ASSERTE(cchOffset >= 0);
		return NULL;
	}

	INT_PTR iLen = GetLen();
	if (iLen < cchOffset)
	{
		_ASSERTE(iLen >= cchOffset);
		return NULL;
	}

	return (ms_Val + cchOffset);
}

CEStr& CEStr::operator=(CEStr&& asStr)
{
	if (ms_Val == asStr.ms_Val)
		return *this;
	Clear();
	std::swap(ms_Val, asStr.ms_Val);
	std::swap(mn_MaxCount, asStr.mn_MaxCount);
	return *this;
}

CEStr& CEStr::operator=(const CEStr& asStr)
{
	if (ms_Val == asStr.ms_Val)
		return *this;
	Clear();
	Set(asStr.ms_Val);
	return *this;
}

CEStr& CEStr::operator=(wchar_t*&& asPtr)
{
	CESTRLOG1("CEStr::=(wchar_t* RVAL_REF x%p)", asPtr);
	AttachInt(asPtr);
	return *this;
}

CEStr& CEStr::operator=(const wchar_t* asPtr)
{
	CESTRLOG1("CEStr::=(const wchar_t* x%p)", asPtr);
	Set(asPtr);
	return *this;
}

CEStr::~CEStr()
{
	CESTRLOG1("CEStr::~CEStr(x%p)", ms_Val);

	#ifdef _DEBUG
	if (ms_Val)
		*ms_Val = 0;
	#endif
	SafeFree(ms_Val);
}

INT_PTR CEStr::GetLen() const
{
	if (!ms_Val || !*ms_Val)
		return 0;
	size_t iLen = wcslen(ms_Val);
	if ((INT_PTR)iLen < 0)
	{
		_ASSERTE((INT_PTR)iLen >= 0);
		return 0;
	}
	return (INT_PTR)iLen;
}

INT_PTR CEStr::GetMaxCount()
{
	if (ms_Val && (mn_MaxCount <= 0))
		mn_MaxCount = GetLen() + 1;
	return mn_MaxCount;
}

wchar_t* CEStr::GetBuffer(INT_PTR cchMaxLen)
{
	CESTRLOG1("CEStr::GetBuffer(%i)", (int)cchMaxLen);

	if (cchMaxLen <= 0)
	{
		_ASSERTE(cchMaxLen>0);
		return NULL;
	}

	// if ms_Val was used externally (by lstrmerge for example),
	// than GetMaxCount() will update mn_MaxCount
	INT_PTR nOldLen = (ms_Val && (GetMaxCount() > 0)) ? (mn_MaxCount-1) : 0;

	if (!ms_Val || (cchMaxLen >= mn_MaxCount))
	{
		INT_PTR nNewMaxLen = std::max(mn_MaxCount,cchMaxLen+1);
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
		ms_Val[std::min(cchMaxLen,nOldLen)] = 0;
	}

	CESTRLOG1("  ms_Val=x%p", ms_Val);

	return ms_Val;
}

wchar_t* CEStr::Detach()
{
	CESTRLOG1("CEStr::Detach()=x%p", ms_Val);

	wchar_t* psz = NULL;
	std::swap(psz, ms_Val);
	mn_MaxCount = 0;
	Empty();

	return psz;
}

void CEStr::Clear()
{
	wchar_t* ptr = Detach();
	SafeFree(ptr);
}

LPCWSTR CEStr::Append(const wchar_t* asStr1, const wchar_t* asStr2 /*= NULL*/, const wchar_t* asStr3 /*= NULL*/, const wchar_t* asStr4 /*= NULL*/, const wchar_t* asStr5 /*= NULL*/, const wchar_t* asStr6 /*= NULL*/, const wchar_t* asStr7 /*= NULL*/, const wchar_t* asStr8 /*= NULL*/)
{
	lstrmerge(&ms_Val, asStr1, asStr2, asStr3, asStr4, asStr5, asStr6, asStr7, asStr8);
	return ms_Val;
}

LPCWSTR CEStr::Attach(wchar_t* RVAL_REF asPtr)
{
	CESTRLOG1("CEStr::Attach(wchar_t* RVAL_REF x%p)", ms_Val);
	return AttachInt(asPtr);
}

LPCWSTR CEStr::AttachInt(wchar_t*& asPtr)
{
	if (ms_Val == asPtr)
	{
		return ms_Val; // Already
	}

	_ASSERTE(!asPtr || !ms_Val || ((asPtr+wcslen(asPtr)) < ms_Val) || ((ms_Val+wcslen(ms_Val)) < asPtr));

	Empty();
	SafeFree(ms_Val);
	if (asPtr)
	{
		size_t len = wcslen(asPtr);
		if ((INT_PTR)len < 0)
		{
			_ASSERTE((INT_PTR)len >= 0);
			return ms_Val;
		}

		ms_Val = asPtr;
		asPtr = NULL;
		mn_MaxCount = 1 + (INT_PTR)len;
	}

	CESTRLOG1("  ms_Val=x%p", ms_Val);

	return ms_Val;
}

// Safe comparing function
int CEStr::Compare(LPCWSTR asText, bool abCaseSensitive /*= false*/) const
{
	if (!ms_Val && asText)
		return -1;
	if (ms_Val && !asText)
		return 1;
	if (!ms_Val && !asText)
		return 0;

	int iCmp;
	if (abCaseSensitive)
		iCmp = lstrcmp(ms_Val, asText);
	else
		iCmp = lstrcmpi(ms_Val, asText);
	return iCmp;
}

bool CEStr::operator==(const wchar_t* asStr) const
{
	return Compare(asStr) == 0;
}

bool CEStr::IsPossibleSwitch() const
{
	// Nothing to compare?
	if (IsEmpty())
		return false;
	if ((ms_Val[0] != L'-') && (ms_Val[0] != L'/'))
		return false;

	// We do not care here about "-new_console:..." or "-cur_console:..."
	// They are processed by RConStartArgsEx

	// But ':' removed from checks, because otherwise ConEmu will not warn
	// on invalid usage of "-new_console:a" for example

	// Also, support smth like "-inside=\eCD /d %1"
	LPCWSTR pszDelim = wcspbrk(ms_Val+1, L"=:");
	LPCWSTR pszInvalids = wcspbrk(ms_Val+1, L"\\/|.&<>^");

	if (pszInvalids && (!pszDelim || (pszInvalids < pszDelim)))
		return false;

	// Well, looks like a switch (`-run` for example)
	return true;
}

bool CEStr::CompareSwitch(LPCWSTR asSwitch) const
{
	if ((asSwitch[0] == L'-') || (asSwitch[0] == L'/'))
	{
		asSwitch++;
	}
	else
	{
		_ASSERTE((asSwitch[0] == L'-') || (asSwitch[0] == L'/'));
	}

	int iCmp = lstrcmpi(ms_Val+1, asSwitch);
	if (iCmp == 0)
		return true;

	// Support partial comparison for L"-inside=..." when (asSwitch == L"-inside=")
	int len = lstrlen(asSwitch);
	if ((len > 1) && ((asSwitch[len-1] == L'=') || (asSwitch[len-1] == L':')))
	{
		iCmp = lstrcmpni(ms_Val+1, asSwitch, (len - 1));
		if ((iCmp == 0) && ((ms_Val[len] == L'=') || (ms_Val[len] == L':')))
			return true;
	}

	return false;
}

bool CEStr::IsSwitch(LPCWSTR asSwitch) const
{
	// Not a switch?
	if (!IsPossibleSwitch())
	{
		return false;
	}

	if (!asSwitch || !*asSwitch)
	{
		_ASSERTE(asSwitch && *asSwitch);
		return false;
	}

	return CompareSwitch(asSwitch);
}

// Stops on first NULL
bool CEStr::OneOfSwitches(LPCWSTR asSwitch1, LPCWSTR asSwitch2, LPCWSTR asSwitch3, LPCWSTR asSwitch4, LPCWSTR asSwitch5, LPCWSTR asSwitch6, LPCWSTR asSwitch7, LPCWSTR asSwitch8, LPCWSTR asSwitch9, LPCWSTR asSwitch10) const
{
	// Not a switch?
	if (!IsPossibleSwitch())
	{
		return false;
	}

	LPCWSTR switches[] = {asSwitch1, asSwitch2, asSwitch3, asSwitch4, asSwitch5, asSwitch6, asSwitch7, asSwitch8, asSwitch9, asSwitch10};

	for (size_t i = 0; (i < countof(switches)) && switches[i]; i++)
	{
		if (CompareSwitch(switches[i]))
			return true;
	}

	return false;

#if 0
	// Variable argument list is not so safe...

	bool bMatch = false;
	va_list argptr;
	va_start(argptr, asSwitch1);

	LPCWSTR pszSwitch = va_arg( argptr, LPCWSTR );
	while (pszSwitch)
	{
		if (CompareSwitch(pszSwitch))
		{
			bMatch = true; break;
		}
		pszSwitch = va_arg( argptr, LPCWSTR );
	}

	va_end(argptr);

	return bMatch;
#endif
}

bool CEStr::IsEmpty() const
{
	return (!ms_Val || !*ms_Val);
}

LPCWSTR CEStr::Set(LPCWSTR asNewValue, INT_PTR anChars /*= -1*/)
{
	CESTRLOG2("CEStr::Set(x%p,%i)", asNewValue, (int)anChars);

	if (asNewValue)
	{
		ssize_t nNewLen = (anChars < 0) ? (ssize_t)wcslen(asNewValue) : std::min<ssize_t>(anChars, wcslen(asNewValue));

		// Assign empty but NOT NULL string
		if (nNewLen <= 0)
		{
			//_ASSERTE(FALSE && "Check, if caller really need to set empty string???");
			if (GetBuffer(1))
				ms_Val[0] = 0;

		}
		// Self pointer assign?
		else if (ms_Val && (asNewValue >= ms_Val) && (asNewValue < (ms_Val + mn_MaxCount)))
		{
			_ASSERTE((asNewValue + nNewLen) < (ms_Val + mn_MaxCount));
			_ASSERTE(nNewLen < mn_MaxCount);

			if (asNewValue > ms_Val)
				wmemmove(ms_Val, asNewValue, nNewLen);
			ms_Val[nNewLen] = 0;

		}
		// New value
		else if (GetBuffer(nNewLen))
		{
			_ASSERTE(mn_MaxCount > nNewLen); // Must be set in GetBuffer
			_wcscpyn_c(ms_Val, mn_MaxCount, asNewValue, nNewLen);
		}
	}
	else
	{
		// Make it NULL
		Empty();
	}

	CESTRLOG1("  ms_Val=x%p", ms_Val);

	return ms_Val;
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
	mb_Quoted = arg.mb_Quoted;
	mn_TokenNo = arg.mn_TokenNo;
	mn_CmdCall = arg.mn_CmdCall;
	#ifdef _DEBUG
	ms_LastTokenEnd = arg.ms_LastTokenEnd;
	lstrcpyn(ms_LastTokenSave, arg.ms_LastTokenSave, countof(ms_LastTokenSave));
	#endif
}



// Minimalistic storage for ANSI/UTF8 strings
CEStrA::CEStrA()
	: ms_Val(nullptr)
{
}

CEStrA::CEStrA(const char* asPtr)
{
	ms_Val = asPtr ? lstrdup(asPtr) : nullptr;
}

CEStrA::CEStrA(char*&& asPtr)
	: ms_Val(nullptr)
{
	std::swap(ms_Val, asPtr);
}

CEStrA::CEStrA(const CEStrA& src)
{
	ms_Val = src.ms_Val ? lstrdup(src.ms_Val) : nullptr;
}

CEStrA::CEStrA(CEStrA&& src)
	: ms_Val(nullptr)
{
	std::swap(ms_Val, src.ms_Val);
}

CEStrA& CEStrA::operator=(const char* asPtr)
{
	SafeFree(ms_Val);
	ms_Val = asPtr ? lstrdup(asPtr) : nullptr;
	return *this;
}

CEStrA& CEStrA::operator=(char*&& asPtr)
{
	SafeFree(ms_Val);
	std::swap(ms_Val, asPtr);
	return *this;
}

CEStrA& CEStrA::operator=(const CEStrA& src)
{
	SafeFree(ms_Val);
	ms_Val = src.ms_Val ? lstrdup(src.ms_Val) : nullptr;
	return *this;
}

CEStrA& CEStrA::operator=(CEStrA&& src)
{
	SafeFree(ms_Val);
	std::swap(ms_Val, src.ms_Val);
	return *this;
}

CEStrA::operator const char*() const
{
	return ms_Val;
}

CEStrA::operator bool() const
{
	return (ms_Val && *ms_Val);
}

const char* CEStrA::c_str(const char* asNullSubstitute /*= NULL*/) const
{
	return ms_Val ? ms_Val : asNullSubstitute;
}

INT_PTR CEStrA::length() const
{
	return ms_Val ? strlen(ms_Val) : 0;
}

void CEStrA::clear()
{
	SafeFree(ms_Val);
}

// Reset the buffer to new empty data of required size
char* CEStrA::getbuffer(INT_PTR cchMaxLen)
{
	clear();
	if (cchMaxLen >= 0)
	{
		ms_Val = (char*)malloc(cchMaxLen+1);
		if (ms_Val)
			ms_Val[0] = 0;
	}
	return ms_Val;
}

// Detach the pointer
char* CEStrA::release()
{
	char* ptr = nullptr;
	std::swap(ptr, ms_Val);
	clear(); // JIC
	return ptr;
}
