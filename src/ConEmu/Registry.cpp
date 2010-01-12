
/*
Copyright (c) 2009-2010 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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

#include <windows.h>
#include "header.h"
#include "registry.h"







SettingsRegistry::SettingsRegistry()
{
	regMy = NULL;
}
SettingsRegistry::~SettingsRegistry()
{
	CloseKey();
}



bool SettingsRegistry::OpenKey(HKEY inHKEY, const wchar_t *regPath, uint access)
{
	bool res = false;
	if (access == KEY_READ)
		res = RegOpenKeyEx(inHKEY, regPath, 0, KEY_READ, &regMy) == ERROR_SUCCESS;
	else
		res = RegCreateKeyEx(inHKEY, regPath, 0, NULL, 0, access, 0, &regMy, 0) == ERROR_SUCCESS;
	return res;
}
bool SettingsRegistry::OpenKey(const wchar_t *regPath, uint access)
{
	return OpenKey(HKEY_CURRENT_USER, regPath, access);
}
void SettingsRegistry::CloseKey()
{
	if (regMy) {
		RegCloseKey(regMy);
		regMy = NULL;
	}
}



bool SettingsRegistry::Load(const wchar_t *regName, LPBYTE value, DWORD nSize)
{
	_ASSERTE(nSize>0);
	if (RegQueryValueEx(regMy, regName, NULL, NULL, (LPBYTE)value, &nSize) == ERROR_SUCCESS)
		return true;
	return false;
}
bool SettingsRegistry::Load(const wchar_t *regName, wchar_t **value, LPDWORD pnSize /*= NULL*/)
{
	DWORD len = 0;
	if (*value) {free(*value); *value = NULL;}
	if (RegQueryValueExW(regMy, regName, NULL, NULL, NULL, &len) == ERROR_SUCCESS && len) {
		int nChLen = len/2;
		*value = (wchar_t*)malloc((nChLen+2)*sizeof(wchar_t));
		(*value)[nChLen] = 0; (*value)[nChLen+1] = 0;
		if (RegQueryValueExW(regMy, regName, NULL, NULL, (LPBYTE)(*value), &len) == ERROR_SUCCESS) {
			if (pnSize) *pnSize = len+1;
			return true;
		}
	} else {
		*value = (wchar_t*)malloc(sizeof(wchar_t)*2);
	}
	(*value)[0] = 0; (*value)[1] = 0; // На случай REG_MULTI_SZ
	return false;
}



void SettingsRegistry::Delete(const wchar_t *regName)
{
	RegDeleteValue(regMy, regName);
}



void SettingsRegistry::Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize)
{
	_ASSERTE(value && nSize);
	RegSetValueEx(regMy, regName, NULL, nType, (LPBYTE)value, nSize); 
}
void SettingsRegistry::Save(const wchar_t *regName, const wchar_t *value)
{
	if (!value) value = _T(""); // сюда мог придти и NULL
	RegSetValueEx(regMy, regName, NULL, REG_SZ, (LPBYTE)value, lstrlenW(value) * sizeof(wchar_t));
}
