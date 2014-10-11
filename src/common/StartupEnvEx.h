
/*
Copyright (c) 2009-2014 Maximus5
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

#include "StartupEnv.h"

class LoadStartupEnvEx : public LoadStartupEnv
{
protected:
	static BOOL CALLBACK LoadStartupEnv_FillMonitors(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
	{
		CEStartupEnv* p = (CEStartupEnv*)dwData;
		if (p->nMonitorsCount >= countof(p->Monitors))
			return FALSE;

		MONITORINFOEX mi;
		ZeroStruct(mi);
		mi.cbSize = sizeof(mi);
		if (GetMonitorInfo(hMonitor, &mi))
		{
			size_t i = p->nMonitorsCount++;
			p->Monitors[i].hMon = hMonitor;
			p->Monitors[i].rcMonitor = mi.rcMonitor;
			p->Monitors[i].rcWork = mi.rcWork;
			p->Monitors[i].dwFlags = mi.dwFlags;
			wcscpy_c(p->Monitors[i].szDevice, mi.szDevice);

			HDC hdc = CreateDC(mi.szDevice, mi.szDevice, NULL, NULL);
			if (hdc)
			{
				p->Monitors[i].dpis[0].x = GetDeviceCaps(hdc, LOGPIXELSX);
				p->Monitors[i].dpis[0].y = GetDeviceCaps(hdc, LOGPIXELSY);
				DeleteDC(hdc);
			}
			else
			{
				p->Monitors[i].dpis[0].x = p->Monitors[i].dpis[0].y = -1;
			}

			if (p->nMonitorsCount >= countof(p->Monitors))
				return FALSE;
		}

		return TRUE;
	}

	static wchar_t* LoadFonts()
	{
		int nLenMax = 2048;
		wchar_t* pszFonts = (wchar_t*)calloc(nLenMax,sizeof(*pszFonts));

		if (pszFonts)
		{
			wchar_t szName[64], szValue[64];
			HKEY hk;
			DWORD nRights = KEY_READ|WIN3264TEST((IsWindows64() ? KEY_WOW64_64KEY : 0),0);
			if (0 == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont", 0, nRights, &hk))
			{
				bool bIsDBCS = (GetSystemMetrics(SM_DBCSENABLED) != 0);
				DWORD idx = 0, cchName = countof(szName), cchValue = sizeof(szValue)-2, dwType;
				LONG iRc;
				wchar_t* psz = pszFonts;
				while ((iRc = RegEnumValue(hk, idx++, szName, &cchName, NULL, &dwType, (LPBYTE)szValue, &cchValue)) == 0)
				{
					szName[min(countof(szName)-1,cchName)] = 0; szValue[min(countof(szValue)-1,cchValue/2)] = 0;
					int nNameLen = lstrlen(szName);
					int nValLen = lstrlen(szValue);
					int nLen = nNameLen+nValLen+3;

					wchar_t* pszEndPtr = NULL;
					int cp = wcstol(szName, &pszEndPtr, 10);
					// Standard console fonts are stored with named "0", "00", "000", etc.
					// But some hieroglyph fonts can be stored for special codepages ("950", "936", etc)
					if (cp)
					{
						// Is codepage supported by our console host?
						// No way to check that...
						// All function reporting "Valid CP"
						// Checked: GetCPInfo, GetCPInfoEx, IsValidCodePage, WideCharToMultiByte
						if (!bIsDBCS)
							cp = -1; // No need to store this font
					}

					if ((cp != -1) && (nLen < nLenMax))
					{
						if (*pszFonts) *(psz++) = L'\t';
						lstrcpy(psz, szName); psz+= nNameLen;
						*(psz++) = L'\t';
						lstrcpy(psz, szValue); psz+= nValLen;
					}

					cchName = countof(szName); cchValue = sizeof(szValue)-2;
				}
				RegCloseKey(hk);
			}
		}

		return pszFonts;
	}

public:
	static CEStartupEnv* Create()
	{
		CEStartupEnv* pEnv = NULL;
		LPBYTE ptrEnd = NULL;

		wchar_t* pszFonts = LoadFonts();
		size_t cchFnt = pszFonts ? (lstrlen(pszFonts)+1) : 0;

		size_t cchTotal = cchFnt*sizeof(wchar_t);

		if (Load(cchTotal, pEnv, ptrEnd))
		{
			wchar_t* psz = (wchar_t*)ptrEnd;

			pEnv->nMonitorsCount = 0;
			EnumDisplayMonitors(NULL, NULL, LoadStartupEnv_FillMonitors, (LPARAM)pEnv);

			HDC hDC = CreateCompatibleDC(NULL);
			pEnv->nPixels = GetDeviceCaps(hDC, BITSPIXEL);
			DeleteDC(hDC);

			// These functions call AdvApi32 functions
			// But LoadStartupEnvEx is not used in ConEmuHk, so there will
			// no AdvApi32 loading during ConEmuHk initialization
			pEnv->bIsWine = IsWine() ? 1 : 0;
			pEnv->bIsWinPE = IsWinPE() ? 1 : 0;

			if (!pEnv->bIsReactOS && (pEnv->os.dwMajorVersion == 5))
			{
				LPCWSTR pszReactCompare = GetReactOsName();
				wchar_t szValue[64] = L"";
				HKEY hk;
				if (0 == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hk))
				{
					DWORD nSize = sizeof(szValue)-sizeof(*szValue);
					if (0 == RegQueryValueEx(hk, L"ProductName", NULL, NULL, (LPBYTE)szValue, &nSize))
					{
						pEnv->bIsReactOS = (lstrcmpi(szValue, pszReactCompare) == 0);
					}
					RegCloseKey(hk);
				}
			}

			if (pszFonts)
			{
				_wcscpy_c(psz, cchFnt, pszFonts);
				pEnv->pszRegConFonts = psz;
				psz += cchFnt;
			}
		}

		SafeFree(pszFonts);

		return pEnv;
	}
};
