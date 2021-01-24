
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
#include <limits>

namespace {

ssize_t StrLen(const char* str)
{
	return str ? strlen(str) : -1;
}

ssize_t StrLen(const wchar_t* str)
{
	return str ? wcslen(str) : -1;
}

template <typename String, typename CharType>
String ExpandEnvStr(const CharType* command, const CharType tailChar,
	DWORD(WINAPI* expandFunction)(const CharType* lpSrc, CharType* lpDst, DWORD nSize))
{
	if (!command || !*command)
	{
		return String{};
	}

	const DWORD cchMax = expandFunction(command, nullptr, 0);
	if (cchMax && (cchMax < static_cast<DWORD>((std::numeric_limits<int>::max)() - 3)))
	{
		String result{};
		const DWORD tailedSize = cchMax + 2;
		auto* pszExpand = result.GetBuffer(tailedSize);
		if (pszExpand)
		{
			result.SetAt(0, 0);
			result.SetAt(cchMax, 0);
			result.SetAt(tailedSize - 2, tailChar);
			result.SetAt(tailedSize - 1, tailChar);

			const DWORD nExp = expandFunction(command, pszExpand, cchMax);
			if (nExp && (nExp <= cchMax) && *pszExpand)
			{
				return result;
			}
		}
	}

	const ssize_t srcLen = StrLen(command);
	if (srcLen <= 0 || (srcLen >= ((std::numeric_limits<ssize_t>::max)() - 3)))
	{
		_ASSERTE(srcLen > 0 && (srcLen < ((std::numeric_limits<ssize_t>::max)() - 3)));
		return String{};
	}
	const ssize_t tailedSize = srcLen + 2;
	String result{};
	if (!result.GetBuffer(tailedSize))
	{
		_ASSERTE(FALSE && "failed to allocate tailedSize");
		return String{};
	}
	result.Set(command);
	result.SetAt(tailedSize - 1, tailChar);
	result.SetAt(tailedSize - 2, tailChar);
	return result;
}

template <typename String, typename CharType>
String GetEnvVar(const CharType* varName,
	DWORD(WINAPI* getEnvVarFunction)(const CharType* lpName, CharType* lpBuffer, DWORD nSize))
{
	if (!varName || !*varName)
	{
		return String{};
	}

	DWORD nErr = 0;
	const DWORD zzTail = 2; // to ensure +2 trailing '\0'
	String result{};

	DWORD nRc = getEnvVarFunction(varName, nullptr, 0);
	if (nRc == 0)
	{
		// This may be empty variable or not existing variable
		nErr = GetLastError();
		if (nErr == ERROR_ENVVAR_NOT_FOUND)
			return String{};
		result.GetBuffer(zzTail);
		result.SetAt(0, 0);
		result.SetAt(1, 0);
		return result;
	}
	// buffer overflow?
	if (nRc >= ((std::numeric_limits<int>::max)() - zzTail))
	{
		_ASSERTE(nRc < ((std::numeric_limits<int>::max)() - zzTail));
		return String{};
	}

	const DWORD cchMax = nRc + zzTail;
	auto* pszVal = result.GetBuffer(cchMax);
	if (!pszVal)
	{
		_ASSERTE((pszVal != nullptr) && "GetEnvVar memory allocation failed");
		return String{};
	}

	nRc = getEnvVarFunction(varName, pszVal, cchMax);
	if ((nRc == 0) || (nRc >= cchMax))
	{
		_ASSERTE(nRc > 0 && nRc < cchMax);
		return String{};
	}

	result.SetAt(nRc + 1, 0);
	result.SetAt(nRc + 2, 0);
	return result;
}

}

CEStr ExpandEnvStr(const wchar_t* command)
{
	return ExpandEnvStr<CEStr, wchar_t>(command, 0xFFFF, ExpandEnvironmentStringsW);
}

CEStrA ExpandEnvStr(const char* command)
{
	return ExpandEnvStr<CEStrA, char>(command, '\xFF', ExpandEnvironmentStringsA);
}

CEStr GetEnvVar(const wchar_t* varName)
{
	return GetEnvVar<CEStr, wchar_t>(varName, GetEnvironmentVariableW);
}

CEStrA GetEnvVar(const char* varName)
{
	return GetEnvVar<CEStrA, char>(varName, GetEnvironmentVariableA);
}



CEnvStrings::CEnvStrings(LPWSTR pszStrings /* = GetEnvironmentStringsW() */)
	: strings_(pszStrings)
{
	// Must be ZZ-terminated
	if (pszStrings && *pszStrings)
	{
		// Parse variables block, determining MAX length
		LPCWSTR pszSrc = pszStrings;
		while (*pszSrc)
		{
			pszSrc += wcslen(pszSrc) + 1;
			count_++;
		}
		cchLength_ = pszSrc - pszStrings + 1;
		// count_ holds count of 'lines' like "name=value\0"
	}
}

CEnvStrings::~CEnvStrings()
{
	if (strings_)
	{
		FreeEnvironmentStringsW(static_cast<LPWCH>(strings_));
		strings_ = nullptr;
	}
}



CEnvRestorer::~CEnvRestorer()
{
	if (restoreEnvVar_ && varName_ && oldValue_)
	{
		SetEnvironmentVariableW(varName_, oldValue_);
	}
}

void CEnvRestorer::Clear()
{
	restoreEnvVar_ = false;
	varName_.Release();
	oldValue_.Release();
}

void CEnvRestorer::SavePathVar(const wchar_t* asCurPath)
{
	// Will restore environment variable "PATH" in destructor
	if (oldValue_.Set(asCurPath))
	{
		varName_ = L"PATH";
		restoreEnvVar_ = true;
	}
}

void CEnvRestorer::SaveEnvVar(const wchar_t*  asVarName, const wchar_t*  asNewValue)
{
	if (!asVarName || !*asVarName)
		return;

	_ASSERTE(!restoreEnvVar_);
	varName_ = asVarName;
	oldValue_ = GetEnvVar(asVarName);
	restoreEnvVar_ = true;

	SetEnvironmentVariableW(asVarName, asNewValue);
}
