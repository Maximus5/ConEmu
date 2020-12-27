
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

#if CE_UNIT_TEST==1
	#include <cstdio>
	extern bool gbVerifyVerbose;
	// ReSharper disable once IdentifierTypo
	#define CESTRLOG0(msg) if (gbVerifyVerbose) printf("  %s\n",(msg))
	// ReSharper disable once IdentifierTypo
	#define CESTRLOG1(fmt,a1) if (gbVerifyVerbose) printf("  " fmt "\n",(a1))
	// ReSharper disable once IdentifierTypo
	#define CESTRLOG2(fmt,a1,a2) if (gbVerifyVerbose) printf("  " fmt "\n",(a1),(a2))
	// ReSharper disable once IdentifierTypo
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
}

CEStr::CEStr(const wchar_t* asStr1, const wchar_t* asStr2/*= nullptr*/, const wchar_t* asStr3/*= nullptr*/,
	const wchar_t* asStr4/*= nullptr*/, const wchar_t* asStr5/*= nullptr*/, const wchar_t* asStr6/*= nullptr*/,
	const wchar_t* asStr7/*= nullptr*/, const wchar_t* asStr8/*= nullptr*/, const wchar_t* asStr9/*= nullptr*/)
{
	CESTRLOG3("CEStr::CEStr(const wchar_t* x%p, x%p, x%p, ...)", asStr1, asStr2, asStr3);
	wchar_t* lpszMerged = lstrmerge(asStr1, asStr2, asStr3, asStr4, asStr5, asStr6, asStr7, asStr8, asStr9);
	AttachInt(lpszMerged);
}

CEStr::CEStr(CEStr&& asStr) noexcept
{
	CESTRLOG1("CEStr::CEStr(CEStr&& x%p)", asStr.ms_Val);
	std::swap(ms_Val, asStr.ms_Val);
	std::swap(maxCount_, asStr.maxCount_);
}

CEStr::CEStr(const CEStr& asStr)
{
	CESTRLOG1("CEStr::CEStr(const CEStr& x%p)", asStr.ms_Val);
	Set(asStr.c_str());
}

CEStr::CEStr(wchar_t*&& asPtr) noexcept
{
	CESTRLOG1("CEStr::CEStr(wchar_t*&& x%p)", asPtr);
	AttachInt(asPtr);
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

// ReSharper disable once CppInconsistentNaming
const wchar_t* CEStr::c_str(const wchar_t* asNullSubstitute /*= nullptr*/) const
{
	CESTRLOG0("CEStr::c_str()");
	return ms_Val ? ms_Val : asNullSubstitute;
}

// ReSharper disable once CppInconsistentNaming
wchar_t* CEStr::data() const
{
	CESTRLOG0("CEStr::data()");
	return ms_Val;
}

// cchMaxCount - including terminating \0
const wchar_t* CEStr::Right(const ssize_t cchMaxCount) const
{
	CESTRLOG1("CEStr::Right(%i)", static_cast<int>(cchMaxCount));
	if (cchMaxCount <= 0)
	{
		_ASSERTE(cchMaxCount > 0);
		return nullptr;
	}

	if (!ms_Val || !*ms_Val)
		return ms_Val;

	const ssize_t iLen = GetLen();
	if (iLen >= cchMaxCount)
		return (ms_Val + (iLen - cchMaxCount + 1));
	return ms_Val;
}

const wchar_t* CEStr::Mid(const ssize_t cchOffset) const
{
	CESTRLOG1("CEStr::Mid(%i)", static_cast<int>(cchOffset));

	if (!ms_Val || (cchOffset < 0))
	{
		_ASSERTE(cchOffset >= 0);
		return L"";
	}

	const ssize_t iLen = GetLen();
	if (iLen < cchOffset)
	{
		_ASSERTE(iLen >= cchOffset);
		return L"";
	}

	return (ms_Val + cchOffset);
}

CEStr& CEStr::operator=(CEStr&& asStr) noexcept
{
	CESTRLOG1("CEStr::operator=(CEStr&& x%p)", asStr.ms_Val);
	if (ms_Val == asStr.ms_Val || this == &asStr)
		return *this;
	Release();
	std::swap(ms_Val, asStr.ms_Val);
	std::swap(maxCount_, asStr.maxCount_);
	return *this;
}

CEStr& CEStr::operator=(const CEStr& asStr)
{
	CESTRLOG1("CEStr::operator=(const CEStr& x%p)", asStr.ms_Val);
	if (ms_Val == asStr.ms_Val || this == &asStr)
		return *this;
	Release();
	Set(asStr.ms_Val);
	return *this;
}

CEStr& CEStr::operator=(wchar_t*&& asPtr) noexcept
{
	CESTRLOG1("CEStr::operator=(wchar_t*&& x%p)", asPtr);
	if (ms_Val == asPtr)
		return *this;
	AttachInt(asPtr);
	return *this;
}

CEStr& CEStr::operator=(const wchar_t* asPtr)
{
	CESTRLOG1("CEStr::operator=(const wchar_t* x%p)", asPtr);
	if (ms_Val == asPtr)
		return *this;
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
	
	Release();
}

void CEStr::swap(CEStr& asStr) noexcept
{
	CESTRLOG1("CEStr::swap(x%p)", asStr.ms_Val);
	std::swap(ms_Val, asStr.ms_Val);
	std::swap(maxCount_, asStr.maxCount_);
}

ssize_t CEStr::GetLen() const
{
	if (!ms_Val || !*ms_Val)
		return 0;
	const size_t iLen = wcslen(ms_Val);
	if (static_cast<ssize_t>(iLen) < 0)
	{
		_ASSERTE(static_cast<ssize_t>(iLen) >= 0);
		return 0;
	}
	return static_cast<ssize_t>(iLen);
}

ssize_t CEStr::GetMaxCount()
{
	if (ms_Val && (maxCount_ <= 0))
		maxCount_ = GetLen() + 1;
	return maxCount_;
}

wchar_t* CEStr::GetBuffer(const ssize_t cchMaxLen)
{
	CESTRLOG1("CEStr::GetBuffer(%i)", static_cast<int>(cchMaxLen));

	if (cchMaxLen <= 0)
	{
		_ASSERTE(cchMaxLen > 0);
		return nullptr;
	}

	// if ms_Val was used externally (by lstrmerge for example),
	// than GetMaxCount() will update mn_MaxCount
	const ssize_t nOldLen = (ms_Val && (GetMaxCount() > 0)) ? (maxCount_ - 1) : 0;

	if (!ms_Val || (cchMaxLen >= maxCount_))
	{
		const ssize_t newMaxCount = std::max(maxCount_, cchMaxLen + 1);
		if (ms_Val)
		{
			auto* newPtr = static_cast<wchar_t*>(realloc(ms_Val, newMaxCount * sizeof(*ms_Val)));
			if (!newPtr)
			{
				_ASSERTE(newPtr != nullptr && "Failed to allocate more space");
				return nullptr;
			}
			ms_Val = newPtr;
		}
		else
		{
			ms_Val = static_cast<wchar_t*>(malloc(newMaxCount * sizeof(*ms_Val)));
			if (!ms_Val)
			{
				_ASSERTE(FALSE && "Failed to allocate buffer");
				return nullptr;
			}
		}
		maxCount_ = newMaxCount;
	}

	if (ms_Val)
	{
		_ASSERTE(cchMaxLen > 0 && nOldLen >= 0 && std::min(cchMaxLen,nOldLen) < GetMaxCount());
		ms_Val[std::min(cchMaxLen,nOldLen)] = '\0';
	}

	CESTRLOG1("  ms_Val=x%p", ms_Val);

	return ms_Val;
}

wchar_t* CEStr::Detach()
{
	CESTRLOG1("CEStr::Detach()=x%p", ms_Val);

	wchar_t* psz = nullptr;
	std::swap(psz, ms_Val);
	maxCount_ = 0;

	_ASSERTE(IsEmpty());

	return psz;
}

void CEStr::Release()
{
	CESTRLOG1("CEStr::Release(x%p)", ms_Val);
	wchar_t* ptr = Detach();
	SafeFree(ptr);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CEStr::Clear()
{
	CESTRLOG1("CEStr::Clear(x%p)", ms_Val);
	if (ms_Val)
	{
		ms_Val[0] = 0;
	}
}

const wchar_t* CEStr::Append(const wchar_t* asStr1, const wchar_t* asStr2 /*= nullptr*/, const wchar_t* asStr3 /*= nullptr*/,
	const wchar_t* asStr4 /*= nullptr*/, const wchar_t* asStr5 /*= nullptr*/, const wchar_t* asStr6 /*= nullptr*/,
	const wchar_t* asStr7 /*= nullptr*/, const wchar_t* asStr8 /*= nullptr*/)
{
	CESTRLOG1("CEStr::Append:x%p(...)", ms_Val);
	lstrmerge(&ms_Val, asStr1, asStr2, asStr3, asStr4, asStr5, asStr6, asStr7, asStr8);
	return ms_Val;
}

const wchar_t* CEStr::Attach(wchar_t*&& asPtr)
{
	CESTRLOG1("CEStr::Attach(wchar_t*&& x%p)", ms_Val);
	return AttachInt(asPtr);
}

const wchar_t* CEStr::AttachInt(wchar_t*& asPtr)
{
	CESTRLOG1("CEStr::Attach(wchar_t*& x%p)", asPtr);
	if (ms_Val == asPtr)
	{
		return ms_Val; // Already
	}

	_ASSERTE(!asPtr || !ms_Val || ((asPtr+wcslen(asPtr)) < ms_Val) || ((ms_Val+wcslen(ms_Val)) < asPtr));

	Clear();
	SafeFree(ms_Val);
	if (asPtr)
	{
		const size_t len = wcslen(asPtr);
		if (static_cast<ssize_t>(len) < 0)
		{
			_ASSERTE(static_cast<ssize_t>(len) >= 0);
			return ms_Val;
		}

		std::swap(ms_Val, asPtr);
		maxCount_ = 1 + static_cast<ssize_t>(len);
	}

	CESTRLOG1("  ms_Val=x%p", ms_Val);

	return ms_Val;
}

// Safe comparing function
int CEStr::Compare(const wchar_t* asText, const bool abCaseSensitive /*= false*/) const
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

bool CEStr::IsNull() const
{
	return (ms_Val == nullptr);
}

const wchar_t* CEStr::Set(const wchar_t* asNewValue, const ssize_t anChars /*= -1*/)
{
	CESTRLOG2("CEStr::Set(x%p,%i)", asNewValue, static_cast<int>(anChars));

	if (asNewValue)
	{
		const ssize_t nNewLen = (anChars < 0)
			? ssize_t(wcslen(asNewValue))
			: std::min<ssize_t>(anChars, wcslen(asNewValue));

		// Assign empty but NOT nullptr string
		if (nNewLen <= 0)
		{
			//_ASSERTE(FALSE && "Check, if caller really need to set empty string???");
			if (GetBuffer(1))
				ms_Val[0] = 0;

		}
		// Self pointer assign?
		else if (ms_Val && (asNewValue >= ms_Val) && (asNewValue < (ms_Val + maxCount_)))
		{
			_ASSERTE((asNewValue + nNewLen) < (ms_Val + maxCount_));
			_ASSERTE(nNewLen < maxCount_);

			if (asNewValue > ms_Val)
				wmemmove(ms_Val, asNewValue, nNewLen);
			ms_Val[nNewLen] = 0;

		}
		// New value
		else if (GetBuffer(nNewLen))
		{
			_ASSERTE(maxCount_ > nNewLen); // Must be set in GetBuffer
			_wcscpyn_c(ms_Val, maxCount_, asNewValue, nNewLen);
			ms_Val[nNewLen] = 0;
		}
	}
	else
	{
		// Make it nullptr
		Clear();
	}

	CESTRLOG1("  ms_Val=x%p", ms_Val);

	return ms_Val;
}

// ReSharper disable once CppMemberFunctionMayBeConst
wchar_t CEStr::SetAt(const ssize_t nIdx, const wchar_t wc)
{
	wchar_t prev = 0;
	if (ms_Val && (nIdx >= 0) && (nIdx < maxCount_))
	{
		prev = ms_Val[nIdx];
		ms_Val[nIdx] = wc;
	}
	return prev;
}



// *********************************
//           CEStrConcat
// *********************************

CEStrA::CEStrA()
{
	CESTRLOG0("CEStrA::CEStrA()");
}

CEStrA::~CEStrA()
{
	CESTRLOG1("CEStrA::~CEStrA(x%p)", ms_Val);

	#ifdef _DEBUG
	if (ms_Val)
		*ms_Val = 0;
	#endif
	SafeFree(ms_Val);
}

CEStrA::CEStrA(const char* asPtr)
{
	CESTRLOG1("CEStrA::~CEStrA(x%p)", asPtr);

	ms_Val = asPtr ? lstrdup(asPtr) : nullptr;
}

CEStrA::CEStrA(char*&& asPtr) noexcept
{
	CESTRLOG1("CEStrA::CEStrA(char*&& x%p)", asPtr);

	ms_Val = asPtr;
}

CEStrA::CEStrA(const CEStrA& src)
{
	CESTRLOG1("CEStrA::CEStrA(const CEStrA& x%p)", src.ms_Val);

	ms_Val = src.ms_Val ? lstrdup(src.ms_Val) : nullptr;
}

CEStrA::CEStrA(CEStrA&& src) noexcept
{
	CESTRLOG1("CEStrA::CEStrA(CEStrA&& x%p)", src.ms_Val);

	std::swap(ms_Val, src.ms_Val);
}

CEStrA& CEStrA::operator=(const char* asPtr)
{
	CESTRLOG1("CEStrA::operator=(const char* x%p)", asPtr);
	if (ms_Val == asPtr)
		return *this;
	SafeFree(ms_Val);
	ms_Val = asPtr ? lstrdup(asPtr) : nullptr;
	return *this;
}

CEStrA& CEStrA::operator=(char*&& asPtr) noexcept
{
	CESTRLOG1("CEStrA::operator=(char*&& x%p)", asPtr);
	if (ms_Val == asPtr)
		return *this;
	SafeFree(ms_Val);
	std::swap(ms_Val, asPtr);
	return *this;
}

CEStrA& CEStrA::operator=(const CEStrA& src)
{
	CESTRLOG1("CEStrA::operator=(const CEStrA& x%p)", src.ms_Val);
	if (ms_Val == src.ms_Val || this == &src)
		return *this;
	SafeFree(ms_Val);
	ms_Val = src.ms_Val ? lstrdup(src.ms_Val) : nullptr;
	return *this;
}

CEStrA& CEStrA::operator=(CEStrA&& src) noexcept
{
	CESTRLOG1("CEStrA::operator=(CEStrA&& x%p)", src.ms_Val);
	if (ms_Val == src.ms_Val || this == &src)
		return *this;
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

// ReSharper disable once CppInconsistentNaming
const char* CEStrA::c_str(const char* asNullSubstitute /*= nullptr*/) const
{
	return ms_Val ? ms_Val : asNullSubstitute;
}

char* CEStrA::data() const
{
	return ms_Val;
}

ssize_t CEStrA::GetLen() const
{
	return ms_Val ? strlen(ms_Val) : 0;
}

void CEStrA::Release()
{
	CESTRLOG1("CEStrA::Release(x%p)", ms_Val);
	SafeFree(ms_Val);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CEStrA::Clear()
{
	CESTRLOG1("CEStrA::Clear(x%p)", ms_Val);
	if (ms_Val)
	{
		ms_Val[0] = 0;
	}
}

char* CEStrA::GetBuffer(const ssize_t cchMaxLen)
{
	CESTRLOG1("CEStrA::GetBuffer(%i)", static_cast<int>(cchMaxLen));
	Release();
	if (cchMaxLen >= 0)
	{
		ms_Val = static_cast<char*>(malloc(cchMaxLen + 1));
		if (ms_Val)
			ms_Val[0] = 0;
	}
	return ms_Val;
}

// Detach the pointer
char* CEStrA::Detach()
{
	CESTRLOG1("CEStrA::Detach()=x%p", ms_Val);
	char* ptr = nullptr;
	std::swap(ptr, ms_Val);
	Release(); // JIC
	return ptr;
}


// *********************************
//           CEStrConcat
// *********************************

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
