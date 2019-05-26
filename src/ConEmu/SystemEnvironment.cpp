
/*
Copyright (c) 2019-present Maximus5
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
#define SHOWDEBUGSTR

#include "Header.h"
#include <cwctype>

#include "../common/WRegistry.h"
#include "SystemEnvironment.h"

void SystemEnvironment::LoadFromRegistry()
{
	env_data.clear();
	RegEnumValues(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
		SysEnvValueCallback, (LPARAM)this, true);
	RegEnumValues(HKEY_CURRENT_USER, L"Environment",
		SysEnvValueCallback, (LPARAM)this, true);
}

std::set<std::wstring> SystemEnvironment::GetKeys() const
{
	std::set<std::wstring> keys;
	for (const auto& v : env_data)
	{
		keys.insert(v.first);
	}
	return keys;
}

std::wstring SystemEnvironment::MakeEnvName(const std::wstring& s)
{
	// Windows environment names are case-insensitive
	std::wstring lc_str; lc_str.reserve(s.size());
	for (const auto& c : s)
	{
		lc_str.push_back(std::towlower(c));
	}
	return lc_str;
}

bool SystemEnvironment::SysEnvValueCallback(HKEY hk, LPCWSTR pszName, DWORD dwType, LPARAM lParam)
{
	if (!pszName || !*pszName || !(dwType == REG_SZ || dwType == REG_EXPAND_SZ))
		return true;
	CEStr reg_data;
	if (RegGetStringValue(hk, nullptr, pszName, reg_data) < 0)
		return true;
	auto& env_data = ((SystemEnvironment*)lParam)->env_data;
	const auto key = MakeEnvName(pszName);
	auto& data = env_data[key];
	data.expandable = (dwType == REG_EXPAND_SZ);
	if (data.name.empty())
		data.name = pszName;
	if (key == L"path")
	{
		// concatenate "%PATH%" as "USER;SYSTEM"
		if (!reg_data.IsEmpty())
			data.data = reg_data.c_str(L"")
				+ std::wstring(data.data.empty() ? L"" : L";")
				+ data.data;
	}
	else
	{
		data.data = reg_data.c_str(L"");
	}
	return true;
}
