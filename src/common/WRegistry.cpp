
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

#define HIDE_USE_EXCEPTION_INFO

#include "defines.h"
#include "WObjects.h"
#include "WRegistry.h"
#include <tuple>

// typedef bool (WINAPI* RegEnumKeysCallback)(HKEY hk, const wchar_t* pszSubkeyName, LPARAM lParam);

int RegEnumKeys(HKEY hkRoot, const wchar_t* pszParentPath, RegEnumKeysCallback_t fn, LPARAM lParam)
{
	int iRc = -1;
	LONG lrc = 0;
	HKEY hk = nullptr, hkChild = nullptr;
	bool bContinue = true;
	const bool isWin64 = IsWindows64();

	for (int s = 0; s < (isWin64 ? 2 : 1) && bContinue; s++)
	{
		DWORD samDesired = KEY_READ;
		if (isWin64)
		{
			if (s == 0)
				samDesired |= WIN3264TEST(KEY_WOW64_32KEY, KEY_WOW64_64KEY);
			else
				samDesired |= WIN3264TEST(KEY_WOW64_64KEY, KEY_WOW64_32KEY);
		}

		if (0 == (lrc = RegOpenKeyExW(hkRoot, pszParentPath, 0, samDesired, &hk)))
		{
			if (iRc < 0) iRc = 0;

			UINT n = 0;
			wchar_t szSubKey[MAX_PATH] = L""; DWORD cchMax = countof(szSubKey) - 1;

			while (0 == (lrc = RegEnumKeyExW(hk, n++, szSubKey, &cchMax, nullptr, nullptr, nullptr, nullptr)))
			{
				if (0 == (lrc = RegOpenKeyExW(hk, szSubKey, 0, samDesired, &hkChild)))
				{
					iRc++;
					if (fn != nullptr)
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

	std::ignore = lrc;
	return iRc;
}

int RegEnumValues(HKEY hkRoot, const wchar_t* pszParentPath, RegEnumValuesCallback_t fn, LPARAM lParam, const bool oneBitnessOnly)
{
	int count = -1;
	HKEY hk = nullptr;
	LONG lrc = -1;
	bool bContinue = true;
	const bool isWin64 = IsWindows64();

	for (int s = 0; s < (isWin64 ? 2 : 1) && bContinue; s++)
	{
		DWORD samDesired = KEY_READ;
		if (isWin64)
		{
			if (s == 0)
				samDesired |= WIN3264TEST(KEY_WOW64_32KEY, KEY_WOW64_64KEY);
			else
				samDesired |= WIN3264TEST(KEY_WOW64_64KEY, KEY_WOW64_32KEY);
		}

		if (0 == (lrc = RegOpenKeyExW(hkRoot, pszParentPath, 0, samDesired, &hk)))
		{
			if (count < 0)
				count = 0;

			CEStr lsName;
			const DWORD cchNameMax = 16383;
			if (lsName.GetBuffer(cchNameMax + 1))
			{
				DWORD idx = 0, cchName = cchNameMax, dwType = 0;

				while ((lrc = RegEnumValueW(hk, idx++, lsName.data(), &cchName, nullptr, &dwType, nullptr, nullptr)) == 0)
				{
					++count;
					lsName.SetAt(std::min(cchNameMax, cchName), 0);
					if (fn != nullptr)
					{
						if (!fn(hk, lsName, dwType, lParam))
						{
							bContinue = FALSE;
							break;
						}
					}
					cchName = cchNameMax; dwType = 0;
				}
			}

			RegCloseKey(hk);

			if (oneBitnessOnly)
				break;
		}
	}

	std::ignore = lrc;
	return count;
}

int RegGetStringValue(HKEY hk, const wchar_t* pszSubKey, const wchar_t* pszValueName, CEStr& rszData, const DWORD wow64Flags /*= 0*/)
{
	int iLen = -1;
	HKEY hkChild = hk;
	DWORD cbSize = 0;
	LONG lrc;

	rszData.Clear();

	if (pszSubKey && *pszSubKey)
	{
		if (hk == nullptr)
		{
			lrc = RegGetStringValue(HKEY_CURRENT_USER, pszSubKey, pszValueName, rszData, 0);
			if (lrc < 0)
			{
				const bool isWin64 = IsWindows64();
				lrc = RegGetStringValue(HKEY_LOCAL_MACHINE, pszSubKey, pszValueName, rszData, isWin64 ? KEY_WOW64_64KEY : 0);
				if ((lrc < 0) && isWin64)
				{
					lrc = RegGetStringValue(HKEY_LOCAL_MACHINE, pszSubKey, pszValueName, rszData, KEY_WOW64_32KEY);
				}
			}
			return lrc;
		}

		if (0 != (lrc = RegOpenKeyExW(hk, pszSubKey, 0, KEY_READ | wow64Flags, &hkChild)))
			hkChild = nullptr;
	}

	if (hkChild && (0 == (lrc = RegQueryValueExW(hkChild, pszValueName, nullptr, nullptr, nullptr, &cbSize))))
	{
		const size_t cchMax = static_cast<size_t>(cbSize >> 1) + 2U;
		if (cchMax < static_cast<size_t>((std::numeric_limits<ssize_t>::max)()))
		{
			wchar_t* pszData = rszData.GetBuffer(cchMax); // +wchar_t+1byte (на возможные ошибки хранения данных в реестре)
			if (pszData)
			{
				if (0 == (lrc = RegQueryValueExW(hkChild, pszValueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(pszData), &cbSize)))
				{
					rszData.SetAt(cbSize >> 1, 0); // Make sure it's 0-terminated
					iLen = lstrlen(pszData);
				}
				else
				{
					rszData.Clear();
				}
			}
		}
	}

	if (hkChild && (hkChild != hk))
	{
		RegCloseKey(hkChild);
	}

	return iLen;
}

LONG RegSetStringValue(HKEY hk, const wchar_t* pszSubKey, const wchar_t* pszValueName, const wchar_t* pszData, const DWORD wow64Flags /*= 0*/)
{
	LONG lrc = -1;
	HKEY hkChild = hk;
	// ReSharper disable once CppJoinDeclarationAndAssignment
	DWORD cbSize = -1;

	if (pszSubKey && *pszSubKey)
	{
		if (0 != (lrc = RegOpenKeyExW(hk, pszSubKey, 0, KEY_WRITE | wow64Flags, &hkChild)))
			hkChild = nullptr;
	}

	if (hkChild)
	{
		if (pszData)
		{
			const auto dataLen = wcslen(pszData);
			if (dataLen < ((std::numeric_limits<DWORD>::max)() / sizeof(*pszData)) - 1)
			{
				cbSize = static_cast<DWORD>((dataLen + 1) * sizeof(*pszData));
				lrc = RegSetValueExW(hkChild, pszValueName, 0, REG_SZ, reinterpret_cast<const BYTE*>(pszData), cbSize);
			}
		}
		else
		{
			lrc = RegDeleteValueW(hkChild, pszValueName);
		}
	}

	if (hkChild && (hkChild != hk))
	{
		RegCloseKey(hkChild);
	}

	return lrc;
}

bool RegDeleteKeyRecursive(HKEY hRoot, const wchar_t* asParent, const wchar_t* asName)
{
	bool lbRc = false;
	HKEY hParent = nullptr;
	HKEY hKey = nullptr;

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
			if (0 != RegEnumKeyExW(hKey, 0, szName, &nMax, nullptr, nullptr, nullptr, nullptr))
				break;

			if (!RegDeleteKeyRecursive(hKey, nullptr, szName))
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
