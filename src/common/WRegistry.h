
/*
Copyright (c) 2014-present Maximus5
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

#include "defines.h"
#include "CmdLine.h"

typedef bool (WINAPI* RegEnumKeysCallback_t)(HKEY hk, const wchar_t* pszSubkeyName, LPARAM lParam);
int RegEnumKeys(HKEY hkRoot, const wchar_t* pszParentPath, RegEnumKeysCallback_t fn, LPARAM lParam);

typedef bool (WINAPI* RegEnumValuesCallback_t)(HKEY hk, const wchar_t* pszName, DWORD dwType, LPARAM lParam);
int RegEnumValues(HKEY hkRoot, const wchar_t* pszParentPath, RegEnumValuesCallback_t fn, LPARAM lParam, bool oneBitnessOnly = false);

int RegGetStringValue(HKEY hk, const wchar_t* pszSubKey, const wchar_t* pszValueName, CEStr& rszData, DWORD wow64Flags = 0);
LONG RegSetStringValue(HKEY hk, const wchar_t* pszSubKey, const wchar_t* pszValueName, const wchar_t* pszData, DWORD wow64Flags = 0);

bool RegDeleteKeyRecursive(HKEY hRoot, const wchar_t* asParent, const wchar_t* asName);
