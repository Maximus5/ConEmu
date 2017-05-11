
/*
Copyright (c) 2014-2017 Maximus5
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
#include <windows.h>
#include "defines.h"
#include "Memory.h"
#include "MAssert.h"
#include "WObjects.h"
#include "WRegistry.h"

// typedef bool (WINAPI* RegEnumKeysCallback)(HKEY hk, LPCWSTR pszSubkeyName, LPARAM lParam);

int RegEnumKeys(HKEY hkRoot, LPCWSTR pszParentPath, RegEnumKeysCallback fn, LPARAM lParam)
{
	int iRc = -1;
	HKEY hk = NULL, hkChild = NULL;
	LONG lrc;
	bool bContinue = true;
	bool ib64 = IsWindows64();

	for (int s = 0; s < (ib64 ? 2 : 1) && bContinue; s++)
	{
		DWORD samDesired = KEY_READ;
		if (ib64)
		{
			if (s == 0)
				samDesired |= WIN3264TEST(KEY_WOW64_32KEY,KEY_WOW64_64KEY);
			else
				samDesired |= WIN3264TEST(KEY_WOW64_64KEY,KEY_WOW64_32KEY);
		}

		if (0 == (lrc = RegOpenKeyEx(hkRoot, pszParentPath, 0, samDesired, &hk)))
		{
			if (iRc < 0) iRc = 0;

			UINT n = 0;
			wchar_t szSubKey[MAX_PATH] = L""; DWORD cchMax = countof(szSubKey) - 1;

			while (0 == (lrc = RegEnumKeyEx(hk, n++, szSubKey, &cchMax, NULL, NULL, NULL, NULL)))
			{
				if (0 == (lrc = RegOpenKeyEx(hk, szSubKey, 0, samDesired, &hkChild)))
				{
					iRc++;
					if (fn != NULL)
					{
						if (!fn(hkChild, szSubKey, lParam))
						{
							bContinue = false;
							break;
						}
					}
					RegCloseKey(hkChild);
				}
				cchMax = countof(szSubKey) - 1;
			}

			RegCloseKey(hk);
		}
	}

	return iRc;
}

int RegEnumValues(HKEY hkRoot, LPCWSTR pszParentPath, RegEnumValuesCallback fn, LPARAM lParam)
{
	int iRc = -1;
	HKEY hk = NULL;
	LONG lrc;
	bool bContinue = true;
	bool ib64 = IsWindows64();

	for (int s = 0; s < (ib64 ? 2 : 1) && bContinue; s++)
	{
		DWORD samDesired = KEY_READ;
		if (ib64)
		{
			if (s == 0)
				samDesired |= WIN3264TEST(KEY_WOW64_32KEY,KEY_WOW64_64KEY);
			else
				samDesired |= WIN3264TEST(KEY_WOW64_64KEY,KEY_WOW64_32KEY);
		}

		if (0 == (lrc = RegOpenKeyEx(hkRoot, pszParentPath, 0, samDesired, &hk)))
		{
			if (iRc < 0) iRc = 0;

			CEStr lsName;
			const DWORD cchNameMax = 16383;
			wchar_t* pszName = lsName.GetBuffer(cchNameMax+1);
			DWORD idx = 0, cchName = cchNameMax, dwType = 0;

			while ((iRc = RegEnumValue(hk, idx++, pszName, &cchName, NULL, &dwType, NULL, NULL)) == 0)
			{
				iRc++;
				pszName[min(cchNameMax,cchName)] = 0;
				if (fn != NULL)
				{
					if (!fn(hk, pszName, dwType, lParam))
					{
						bContinue = FALSE;
						break;
					}
				}
				cchName = cchNameMax; dwType = 0;
			}

			RegCloseKey(hk);
		}
	}

	return iRc;
}

int RegGetStringValue(HKEY hk, LPCWSTR pszSubKey, LPCWSTR pszValueName, CEStr& rszData, DWORD Wow64Flags /*= 0*/)
{
	int iLen = -1;
	HKEY hkChild = hk;
	DWORD cbSize = 0;
	LONG lrc;

	rszData.Empty();

	if (pszSubKey && *pszSubKey)
	{
		if (hk == NULL)
		{
			lrc = RegGetStringValue(HKEY_CURRENT_USER, pszSubKey, pszValueName, rszData, 0);
			if (lrc < 0)
			{
				bool isWin64 = IsWindows64();
				lrc = RegGetStringValue(HKEY_LOCAL_MACHINE, pszSubKey, pszValueName, rszData, isWin64 ? KEY_WOW64_64KEY : 0);
				if ((lrc < 0) && isWin64)
				{
					lrc = RegGetStringValue(HKEY_LOCAL_MACHINE, pszSubKey, pszValueName, rszData, KEY_WOW64_32KEY);
				}
			}
			if (lrc > 0)
			{
				return lrc;
			}
		}

		if (0 != (lrc = RegOpenKeyEx(hk, pszSubKey, 0, KEY_READ|Wow64Flags, &hkChild)))
			hkChild = NULL;
	}

	if (hkChild && (0 == (lrc = RegQueryValueEx(hkChild, pszValueName, NULL, NULL, NULL, &cbSize))))
	{
		wchar_t* pszData = rszData.GetBuffer((cbSize>>1)+2); // +wchar_t+1byte (на возможные ошибки хранения данных в реестре)
		if (pszData)
		{
			pszData[cbSize>>1] = 0; // Make sure it will be 0-terminated
			if (0 == (lrc = RegQueryValueEx(hkChild, pszValueName, NULL, NULL, (LPBYTE)pszData, &cbSize)))
			{
				iLen = lstrlen(pszData);
			}
			else
			{
				rszData.Empty();
			}
		}
	}

	if (hkChild && (hkChild != hk))
	{
		RegCloseKey(hkChild);
	}

	return iLen;
}

LONG RegSetStringValue(HKEY hk, LPCWSTR pszSubKey, LPCWSTR pszValueName, LPCWSTR pszData, DWORD Wow64Flags /*= 0*/)
{
	LONG lrc = -1;
	HKEY hkChild = hk;
	DWORD cbSize;

	if (pszSubKey && *pszSubKey)
	{
		if (0 != (lrc = RegOpenKeyEx(hk, pszSubKey, 0, KEY_WRITE|Wow64Flags, &hkChild)))
			hkChild = NULL;
	}

	if (hkChild)
	{
		if (pszData)
		{
			cbSize = (lstrlen(pszData)+1)*sizeof(*pszData);
			lrc = RegSetValueEx(hkChild, pszValueName, 0, REG_SZ, (const BYTE*)pszData, cbSize);
		}
		else
		{
			lrc = RegDeleteValue(hkChild, pszValueName);
		}
	}

	if (hkChild && (hkChild != hk))
	{
		RegCloseKey(hkChild);
	}

	return lrc;
}

bool RegDeleteKeyRecursive(HKEY hRoot, LPCWSTR asParent, LPCWSTR asName)
{
	bool lbRc = false;
	HKEY hParent = NULL;
	HKEY hKey = NULL;

	if (!asName || !*asName || !hRoot)
		return false;

	if (asParent && *asParent)
	{
		if (0 != RegOpenKeyEx(hRoot, asParent, 0, KEY_ALL_ACCESS, &hParent))
			return false;
		hRoot = hParent;
	}

	if (0 == RegOpenKeyEx(hRoot, asName, 0, KEY_ALL_ACCESS, &hKey))
	{
		for (DWORD i = 0; i < 255; i++)
		{
			wchar_t szName[MAX_PATH];
			DWORD nMax = countof(szName);
			if (0 != RegEnumKeyEx(hKey, 0, szName, &nMax, 0, 0, 0, 0))
				break;

			if (!RegDeleteKeyRecursive(hKey, NULL, szName))
				break;
		}

		RegCloseKey(hKey);

		if (0 == RegDeleteKey(hRoot, asName))
			lbRc = true;
	}

	if (hParent)
		RegCloseKey(hParent);

	return lbRc;
}
