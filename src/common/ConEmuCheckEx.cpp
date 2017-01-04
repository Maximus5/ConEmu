
/*
Copyright (c) 2009-2017 Maximus5
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
#include "MAssert.h"
#include "Common.h"
#include "Memory.h"
#include "WObjects.h"
#include "ConEmuCheckEx.h"
#include "MFileMapping.h"

BOOL LoadGuiMapping(HWND hConEmuWnd, ConEmuGuiMapping& GuiMapping)
{
	//if (m_GuiMapping.cbSize)
	//	return TRUE;

	// Проверим, а можно ли?
	DWORD dwGuiProcessId = 0;
	if (!hConEmuWnd || !GetWindowThreadProcessId(hConEmuWnd, &dwGuiProcessId))
		return FALSE;

	return LoadGuiMapping(dwGuiProcessId, GuiMapping);
}

// Вернуть путь к папке, содержащей ConEmuC.exe
BOOL IsConEmuExeExist(LPCWSTR szExePath, wchar_t (&rsConEmuExe)[MAX_PATH+1])
{
	BOOL lbExeFound = FALSE;
	BOOL isWin64 = WIN3264TEST(IsWindows64(),TRUE);

	//if (szExePath[lstrlen(szExePath)-1] != L'\\')
	//	wcscat_c(szExePath, L"\\");
	_ASSERTE(szExePath && *szExePath && (szExePath[lstrlen(szExePath)-1] == L'\\'));

	wcscpy_c(rsConEmuExe, szExePath);
	wchar_t* pszName = rsConEmuExe+lstrlen(rsConEmuExe);
	LPCWSTR szGuiExe[2] = {L"ConEmu64.exe", L"ConEmu.exe"};
	for (size_t s = 0; !lbExeFound && (s < countof(szGuiExe)); s++)
	{
		if (!s && !isWin64) continue;
		wcscpy_add(pszName, rsConEmuExe, szGuiExe[s]);
		lbExeFound = FileExists(rsConEmuExe);
	}

	return lbExeFound;
}

BOOL FindConEmuBaseDir(wchar_t (&rsConEmuBaseDir)[MAX_PATH+1], wchar_t (&rsConEmuExe)[MAX_PATH+1], HMODULE hPluginDll /*= NULL*/)
{
	// Сначала пробуем Mapping консоли (вдруг есть?)
	{
		MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> ConMap;
		ConMap.InitName(CECONMAPNAME, LODWORD(myGetConsoleWindow()));
		CESERVER_CONSOLE_MAPPING_HDR* p = ConMap.Open();
		if (p && p->ComSpec.ConEmuBaseDir[0])
		{
			// Успешно
			wcscpy_c(rsConEmuBaseDir, p->ComSpec.ConEmuBaseDir);
			wcscpy_c(rsConEmuExe, p->sConEmuExe);
			return TRUE;
		}
	}

	// Теперь - пробуем найти существующее окно ConEmu
	HWND hConEmu = FindWindow(VirtualConsoleClassMain, NULL);
	DWORD dwGuiPID = 0;
	if (hConEmu)
	{
		if (GetWindowThreadProcessId(hConEmu, &dwGuiPID) && dwGuiPID)
		{
			MFileMapping<ConEmuGuiMapping> GuiMap;
			GuiMap.InitName(CEGUIINFOMAPNAME, dwGuiPID);
			ConEmuGuiMapping* p = GuiMap.Open();
			if (p && p->ComSpec.ConEmuBaseDir[0])
			{
				wcscpy_c(rsConEmuBaseDir, p->ComSpec.ConEmuBaseDir);
				wcscpy_c(rsConEmuExe, p->sConEmuExe);
				return TRUE;
			}
		}
	}


	wchar_t szExePath[MAX_PATH+1];
	HKEY hkRoot[] = {NULL,HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE,HKEY_LOCAL_MACHINE};
	DWORD samDesired = KEY_QUERY_VALUE;

	BOOL isWin64 = WIN3264TEST(IsWindows64(),TRUE);
	DWORD RedirectionFlag = WIN3264TEST((isWin64 ? KEY_WOW64_64KEY : 0),KEY_WOW64_32KEY);
	//#ifdef _WIN64
	//	isWin64 = TRUE;
	//	RedirectionFlag = KEY_WOW64_32KEY;
	//#else
	//	isWin64 = IsWindows64();
	//	RedirectionFlag = isWin64 ? KEY_WOW64_64KEY : 0;
	//#endif

	for (size_t i = 0; i < countof(hkRoot); i++)
	{
		szExePath[0] = 0;

		if (i == 0)
		{
			// Запущенного ConEmu.exe нет, можно поискать в каталоге текущего приложения

			if (!GetModuleFileName(NULL, szExePath, countof(szExePath)-20))
				continue;
			wchar_t* pszName = wcsrchr(szExePath, L'\\');
			if (!pszName)
				continue;
			*(pszName+1) = 0;

			// Проверяем наличие файлов
			// LPCWSTR szGuiExe[2] = {L"ConEmu64.exe", L"ConEmu.exe"};
			if (!IsConEmuExeExist(szExePath, rsConEmuExe) && hPluginDll)
			{
				// Попробовать найти наш exe-шник от пути плагина?
				if (!GetModuleFileName(hPluginDll, szExePath, countof(szExePath)-1))
					continue;
				wchar_t* pszName = wcsrchr(szExePath, L'\\');
				if (!pszName)
					continue;
				*(pszName+1) = 0;
				int nLen = lstrlen(szExePath);
				LPCWSTR pszCompare = L"\\Plugins\\ConEmu\\";
				if (nLen <= lstrlen(pszCompare))
					continue;
				nLen = lstrlen(pszCompare);
				int iCmp = lstrcmpi(pszName-nLen+1, pszCompare);
				if (iCmp != 0)
					continue;
				*(pszName-nLen+2) = 0;
			}

		}
		else
		{
			// Остался последний шанс - если ConEmu установлен через MSI, то путь указан в реестре
			// [HKEY_LOCAL_MACHINE\SOFTWARE\ConEmu]
			// "InstallDir"="C:\\Utils\\Far180\\"

			if (i == (countof(hkRoot)-1))
			{
				if (RedirectionFlag)
					samDesired |= RedirectionFlag;
				else
					break;
			}

			HKEY hKey;
			if (RegOpenKeyEx(hkRoot[i], L"Software\\ConEmu", 0, samDesired, &hKey) != ERROR_SUCCESS)
				continue;
			memset(szExePath, 0, countof(szExePath));
			DWORD nType = 0, nSize = sizeof(szExePath)-20*sizeof(wchar_t);
			int RegResult = RegQueryValueEx(hKey, L"", NULL, &nType, (LPBYTE)szExePath, &nSize);
			RegCloseKey(hKey);
			if (RegResult != ERROR_SUCCESS)
				continue;
		}

		if (szExePath[0])
		{
			// Хоть и задано в реестре - файлов может не быть. Проверяем
			if (szExePath[lstrlen(szExePath)-1] != L'\\')
				wcscat_c(szExePath, L"\\");
			// Проверяем наличие файлов
			// LPCWSTR szGuiExe[2] = {L"ConEmu64.exe", L"ConEmu.exe"};
			BOOL lbExeFound = IsConEmuExeExist(szExePath, rsConEmuExe);

			// Если GUI-exe найден - ищем "base"
			if (lbExeFound)
			{
				wchar_t* pszName = szExePath+lstrlen(szExePath);
				LPCWSTR szSrvExe[4] = {L"ConEmuC64.exe", L"ConEmu\\ConEmuC64.exe", L"ConEmuC.exe", L"ConEmu\\ConEmuC.exe"};
				for (size_t s = 0; s < countof(szSrvExe); s++)
				{
					if ((s <=1) && !isWin64) continue;
					wcscpy_add(pszName, szExePath, szSrvExe[s]);
					if (FileExists(szExePath))
					{
						pszName = wcsrchr(szExePath, L'\\');
						if (pszName)
						{
							*pszName = 0; // БЕЗ слеша на конце!
							wcscpy_c(rsConEmuBaseDir, szExePath);
							return TRUE;
						}
					}
				}
			}
		}
	}

	// Не удалось
	return FALSE;
}
