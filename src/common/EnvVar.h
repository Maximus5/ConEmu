
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


#pragma once

#include "CEStr.h"

CEStr ExpandEnvStr(const wchar_t* command);
CEStrA ExpandEnvStr(const char* command);

CEStr GetEnvVar(const wchar_t* varName);
CEStrA GetEnvVar(const char* varName);


class CEnvStrings final
{
public:
	LPWSTR strings_{ nullptr };
	size_t cchLength_{ 0 };
	size_t count_{ 0 }; // Holds count of 'lines' like "name=value\0"
public:
	CEnvStrings(LPWSTR pszStrings /* = GetEnvironmentStringsW() */);
	~CEnvStrings();

	CEnvStrings(const CEnvStrings&) = delete;
	CEnvStrings& operator=(const CEnvStrings&) = delete;
	CEnvStrings(CEnvStrings&&) = default;
	CEnvStrings& operator=(CEnvStrings&&) = default;
};


class CEnvRestorer final
{
private:
	bool restoreEnvVar_ = false;
	CEStr varName_, oldValue_;

public:
	CEnvRestorer() = default;
	~CEnvRestorer();
	void Clear();
	void SavePathVar(const wchar_t*  asCurPath);
	void SaveEnvVar(const wchar_t*  asVarName, const wchar_t*  asNewValue);

	CEnvRestorer(const CEnvRestorer&) = delete;
	CEnvRestorer& operator=(const CEnvRestorer&) = delete;
	CEnvRestorer(CEnvRestorer&&) = default;
	CEnvRestorer& operator=(CEnvRestorer&&) = default;
};
