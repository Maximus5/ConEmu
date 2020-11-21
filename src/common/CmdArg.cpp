
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
#include "CmdArg.h"
#include <tuple>


CmdArg::CmdArg()
{
}

CmdArg::CmdArg(const wchar_t* str)
	: CEStr(str)
{
}

CmdArg::~CmdArg()
{
	Empty();
}

CmdArg& CmdArg::operator=(const wchar_t* str)
{
	Set(str);
	return *this;
}

void CmdArg::Empty()
{
	CEStr::Empty();

	m_nTokenNo = 0;
	m_nCmdCall = CmdCall::Undefined;
	m_pszDequoted = nullptr;
	m_bQuoted = false;

	#ifdef _DEBUG
	m_sLastTokenEnd = nullptr;
	m_sLastTokenSave[0] = 0;
	#endif
}

void CmdArg::LoadPosFrom(const CmdArg& arg)
{
	m_pszDequoted = arg.m_pszDequoted;
	m_bQuoted = arg.m_bQuoted;
	m_nTokenNo = arg.m_nTokenNo;
	m_nCmdCall = arg.m_nCmdCall;
	#ifdef _DEBUG
	m_sLastTokenEnd = arg.m_sLastTokenEnd;
	std::ignore = lstrcpyn(m_sLastTokenSave, arg.m_sLastTokenSave, countof(m_sLastTokenSave));
	#endif
}

bool CmdArg::IsPossibleSwitch() const
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

	// Also, support something like "-inside=\eCD /d %1"
	const auto* delimiter = wcspbrk(ms_Val+1, L"=:");
	const auto* invalids = wcspbrk(ms_Val+1, L"\\/|.&<>^");

	if (invalids && (!delimiter || (invalids < delimiter)))
		return false;

	// Well, looks like a switch (`-run` for example)
	return true;
}

bool CmdArg::CompareSwitch(const wchar_t* asSwitch, const bool caseSensitive /*= false*/) const
{
	if ((asSwitch[0] == L'-') || (asSwitch[0] == L'/'))
	{
		asSwitch++;
	}
	else
	{
		_ASSERTE((asSwitch[0] == L'-') || (asSwitch[0] == L'/'));
	}

	int iCmp = caseSensitive
		? lstrcmp(ms_Val + 1, asSwitch)
		: lstrcmpi(ms_Val + 1, asSwitch);
	if (iCmp == 0)
		return true;

	// Support partial comparison for L"-inside=..." when (asSwitch == L"-inside=")
	const int len = lstrlen(asSwitch);
	if ((len > 1) && ((asSwitch[len-1] == L'=') || (asSwitch[len-1] == L':')))
	{
		iCmp = lstrcmpni(ms_Val+1, asSwitch, (len - 1));
		if ((iCmp == 0) && ((ms_Val[len] == L'=') || (ms_Val[len] == L':')))
			return true;
	}

	return false;
}

bool CmdArg::IsSwitch(const wchar_t* asSwitch, const bool caseSensitive /*= false*/) const
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

	return CompareSwitch(asSwitch, caseSensitive);
}

// Stops on first nullptr
bool CmdArg::OneOfSwitches(const wchar_t* asSwitch1, const wchar_t* asSwitch2, const wchar_t* asSwitch3, const wchar_t* asSwitch4, const wchar_t* asSwitch5, const wchar_t* asSwitch6, const wchar_t* asSwitch7, const wchar_t* asSwitch8, const wchar_t* asSwitch9, const wchar_t* asSwitch10) const
{
	// Not a switch?
	if (!IsPossibleSwitch())
	{
		return false;
	}

	const wchar_t* switches[] = {asSwitch1, asSwitch2, asSwitch3, asSwitch4, asSwitch5, asSwitch6, asSwitch7, asSwitch8, asSwitch9, asSwitch10};

	for (const auto& name : switches)
	{
		if (!name)
			break;
		if (CompareSwitch(name))
			return true;
	}

	return false;
}

const wchar_t* CmdArg::GetExtra() const
{
	if (!IsPossibleSwitch())
		return L"";
	const auto* delimiter = wcspbrk(ms_Val, L":=");
	if (!delimiter)
		return L"";
	return (delimiter + 1);
}
