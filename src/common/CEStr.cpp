
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
}

CEStr::operator const wchar_t*() const
{
	CESTRLOG0("CEStr::const wchar_t*()");
	return ms_Val;
}

CEStr::operator bool() const
{
	return (!IsEmpty());
}

const wchar_t* CEStr::c_str(const wchar_t* asNullSubstitute /*= NULL*/) const
{
	CESTRLOG0("CEStr::c_str()");
	return ms_Val ? ms_Val : asNullSubstitute;
}

// cchMaxCount - including terminating \0
const wchar_t* CEStr::Right(ssize_t cchMaxCount) const
{
	CESTRLOG1("CEStr::Right(%i)", (int)cchMaxCount);
	if (cchMaxCount <= 0)
	{
		_ASSERTE(cchMaxCount > 0);
		return NULL;
	}

	if (!ms_Val || !*ms_Val)
		return ms_Val;

	ssize_t iLen = GetLen();
	if (iLen >= cchMaxCount)
		return (ms_Val + (iLen - cchMaxCount + 1));
	return ms_Val;
}

const wchar_t* CEStr::Mid(ssize_t cchOffset) const
{
	CESTRLOG1("CEStr::Mid(%i)", cchOffset);

	if (!ms_Val || (cchOffset < 0))
	{
		_ASSERTE(cchOffset >= 0);
		return NULL;
	}

	ssize_t iLen = GetLen();
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

void CEStr::swap(CEStr& asStr)
{
	std::swap(ms_Val, asStr.ms_Val);
	std::swap(mn_MaxCount, asStr.mn_MaxCount);
}

ssize_t CEStr::GetLen() const
{
	if (!ms_Val || !*ms_Val)
		return 0;
	size_t iLen = wcslen(ms_Val);
	if ((ssize_t)iLen < 0)
	{
		_ASSERTE((ssize_t)iLen >= 0);
		return 0;
	}
	return (ssize_t)iLen;
}

ssize_t CEStr::GetMaxCount()
{
	if (ms_Val && (mn_MaxCount <= 0))
		mn_MaxCount = GetLen() + 1;
	return mn_MaxCount;
}

wchar_t* CEStr::GetBuffer(ssize_t cchMaxLen)
{
	CESTRLOG1("CEStr::GetBuffer(%i)", (int)cchMaxLen);

	if (cchMaxLen <= 0)
	{
		_ASSERTE(cchMaxLen>0);
		return NULL;
	}

	// if ms_Val was used externally (by lstrmerge for example),
	// than GetMaxCount() will update mn_MaxCount
	ssize_t nOldLen = (ms_Val && (GetMaxCount() > 0)) ? (mn_MaxCount-1) : 0;

	if (!ms_Val || (cchMaxLen >= mn_MaxCount))
	{
		ssize_t nNewMaxLen = std::max(mn_MaxCount,cchMaxLen+1);
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

const wchar_t* CEStr::Append(const wchar_t* asStr1, const wchar_t* asStr2 /*= NULL*/, const wchar_t* asStr3 /*= NULL*/, const wchar_t* asStr4 /*= NULL*/, const wchar_t* asStr5 /*= NULL*/, const wchar_t* asStr6 /*= NULL*/, const wchar_t* asStr7 /*= NULL*/, const wchar_t* asStr8 /*= NULL*/)
{
	lstrmerge(&ms_Val, asStr1, asStr2, asStr3, asStr4, asStr5, asStr6, asStr7, asStr8);
	return ms_Val;
}

const wchar_t* CEStr::Attach(wchar_t* RVAL_REF asPtr)
{
	CESTRLOG1("CEStr::Attach(wchar_t* RVAL_REF x%p)", ms_Val);
	return AttachInt(asPtr);
}

const wchar_t* CEStr::AttachInt(wchar_t*& asPtr)
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
		if ((ssize_t)len < 0)
		{
			_ASSERTE((ssize_t)len >= 0);
			return ms_Val;
		}

		ms_Val = asPtr;
		asPtr = NULL;
		mn_MaxCount = 1 + (ssize_t)len;
	}

	CESTRLOG1("  ms_Val=x%p", ms_Val);

	return ms_Val;
}

// Safe comparing function
int CEStr::Compare(const wchar_t* asText, bool abCaseSensitive /*= false*/) const
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

bool CEStr::IsEmpty() const
{
	return (!ms_Val || !*ms_Val);
}

const wchar_t* CEStr::Set(const wchar_t* asNewValue, ssize_t anChars /*= -1*/)
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

wchar_t CEStr::SetAt(const ssize_t nIdx, const wchar_t wc)
{
	wchar_t prev = 0;
	if (ms_Val && (nIdx < mn_MaxCount))
	{
		prev = ms_Val[nIdx];
		ms_Val[nIdx] = wc;
	}
	return prev;
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

ssize_t CEStrA::length() const
{
	return ms_Val ? strlen(ms_Val) : 0;
}

void CEStrA::clear()
{
	SafeFree(ms_Val);
}

// Reset the buffer to new empty data of required size
char* CEStrA::getbuffer(ssize_t cchMaxLen)
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

// CEStrConcat
void CEStrConcat::Append(const wchar_t* str)
{
	if (!str || !*str) return;
	CEStr new_str(str);
	ssize_t new_len = new_str.GetLen();
	total_ += new_len;
	strings_.push_back({new_len, std::move(new_str)});
}

CEStr CEStrConcat::GetData() const
{
	CEStr result;
	wchar_t* buffer = result.GetBuffer(total_);
	for (const auto& str : strings_)
	{
		wmemmove_s(buffer, str.first, str.second, str.first);
		buffer += str.first;
	}
	*buffer = 0;
	return result;
}
