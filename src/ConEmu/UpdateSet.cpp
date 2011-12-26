
/*
Copyright (c) 2011 Maximus5
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

#include "header.h"
#include "UpdateSet.h"

ConEmuUpdateSettings::ConEmuUpdateSettings()
{
	// struct, memset is valid
	memset(this, 0, sizeof(*this));
}

void ConEmuUpdateSettings::ResetToDefaults()
{
	szUpdateVerLocation = lstrdup(L"file://T:\\VCProject\\FarPlugin\\ConEmu\\Maximus5\\version.ini");
	isUpdateCheckOnStartup = true;
	isUpdateCheckHourly = false;
	isUpdateCheckNotifyOnly = true;
	isUpdateUseBuilds = 2; // 1-stable only, 2-latest
	isUpdateUseProxy = false;
	szUpdateProxy = szUpdateProxyUser = szUpdateProxyPassword = NULL; // "Server:port"
	isUpdateDownloadSetup = 1; // 1-Installer (ConEmuSetup.exe), 2-7z archieve (ConEmu.7z), WinRar or 7z required
	szUpdateExeCmdLine = lstrdup(L"\"%1\" /p:%3 /wait:%4 /qr");
	wchar_t* pszArcPath = NULL; BOOL bWin64 = IsWindows64(NULL);
	for (int i = 0; !(pszArcPath && *pszArcPath) && (i <= 5); i++)
	{
		SettingsRegistry regArc;
		switch (i)
		{
		case 0:
			if (regArc.OpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\7-Zip", KEY_READ|(bWin64?KEY_WOW64_32KEY:0)))
				regArc.Load(L"Path", &pszArcPath);
			break;
		case 1:
			if (bWin64 && regArc.OpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\7-Zip", KEY_READ|KEY_WOW64_64KEY))
				regArc.Load(L"Path", &pszArcPath);
			break;
		case 2:
			if (regArc.OpenKey(HKEY_CURRENT_USER, L"SOFTWARE\\7-Zip", KEY_READ))
				regArc.Load(L"Path", &pszArcPath);
			break;
		case 3:
			if (regArc.OpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WinRAR", KEY_READ|(bWin64?KEY_WOW64_32KEY:0)))
				regArc.Load(L"exe32", &pszArcPath);
			break;
		case 4:
			if (bWin64 && regArc.OpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WinRAR", KEY_READ|KEY_WOW64_64KEY))
				regArc.Load(L"exe64", &pszArcPath);
			break;
		case 5:
			if (regArc.OpenKey(HKEY_CURRENT_USER, L"SOFTWARE\\WinRAR", KEY_READ))
			{
				if (!regArc.Load(L"exe32", &pszArcPath) && bWin64)
					regArc.Load(L"exe64", &pszArcPath);
			}
			break;
		}
	}
	if (!pszArcPath || !*pszArcPath)
	{
		szUpdateArcCmdLine = lstrdup(L"\"%ProgramFiles%\\7-Zip\7zg.exe\" x \"%1\" \"%2\\\""); // "%1"-archive file, "%2"-ConEmu base dir
	}
	else
	{
		LPCWSTR pszExt = PointToExt(pszArcPath);
		int cchMax = lstrlen(pszArcPath)+64;
		szUpdateArcCmdLine = (wchar_t*)malloc(cchMax*sizeof(wchar_t));
		if (szUpdateArcCmdLine)
		{
			if (pszExt && lstrcmpi(pszExt, L".exe") == 0)
				_wsprintf(szUpdateArcCmdLine, SKIPLEN(cchMax) L"\"%s\" x \"%%1\" \"%%2\\\"", pszArcPath);
			else
				_wsprintf(szUpdateArcCmdLine, SKIPLEN(cchMax) L"\"%s\\7zg.exe\" x \"%%1\" \"%%2\\\"", pszArcPath);
		}
	}
	if (pszArcPath) free(pszArcPath);
	szUpdateDownloadPath = lstrdup(L"%TEMP%");
	isUpdateLeavePackages = false;
	szUpdatePostUpdateCmd = lstrdup(L"echo Last successful update>update.info && date /t>>update.info && time /t>>update.info"); // Юзер может чего-то свое делать с распакованными файлами
}

ConEmuUpdateSettings::~ConEmuUpdateSettings()
{
	FreePointers();
}

void ConEmuUpdateSettings::FreePointers()
{
	SafeFree(szUpdateVerLocation);
	SafeFree(szUpdateProxy);
	SafeFree(szUpdateProxyUser);
	SafeFree(szUpdateProxyPassword);
	SafeFree(szUpdateExeCmdLine);
	SafeFree(szUpdateArcCmdLine);
	SafeFree(szUpdateDownloadPath);
	SafeFree(szUpdatePostUpdateCmd);
}

void ConEmuUpdateSettings::LoadFrom(ConEmuUpdateSettings* apFrom)
{
	FreePointers();
	
	szUpdateVerLocation = lstrdup(apFrom->szUpdateVerLocation); // ConEmu latest version location info
	isUpdateCheckOnStartup = apFrom->isUpdateCheckOnStartup;
	isUpdateCheckHourly = apFrom->isUpdateCheckHourly;
	isUpdateCheckNotifyOnly = apFrom->isUpdateCheckNotifyOnly;
	isUpdateUseBuilds = apFrom->isUpdateUseBuilds; // 1-stable only, 2-latest
	isUpdateUseProxy = apFrom->isUpdateUseProxy;
	szUpdateProxy = lstrdup(apFrom->szUpdateProxy); // "Server:port"
	szUpdateProxyUser = lstrdup(apFrom->szUpdateProxyUser);
	szUpdateProxyPassword = lstrdup(apFrom->szUpdateProxyPassword);
	isUpdateDownloadSetup = apFrom->isUpdateDownloadSetup; // 1-Installer (ConEmuSetup.exe), 2-7z archieve (ConEmu.7z), WinRar or 7z required
	// "%1"-archive or setup file, "%2"-ConEmu base dir, "%3"-x86/x64, "%4"-ConEmu PID
	szUpdateExeCmdLine = lstrdup(apFrom->szUpdateExeCmdLine); 
	szUpdateArcCmdLine = lstrdup(apFrom->szUpdateArcCmdLine);
	szUpdateDownloadPath = lstrdup(apFrom->szUpdateDownloadPath); // "%TEMP%"
	isUpdateLeavePackages = apFrom->isUpdateLeavePackages;
	szUpdatePostUpdateCmd = lstrdup(apFrom->szUpdatePostUpdateCmd); // Юзер может чего-то свое делать с распакованными файлами
}

bool ConEmuUpdateSettings::UpdatesAllowed()
{
	if (!szUpdateVerLocation || !*szUpdateVerLocation)
		return false; // Не указано расположение обновления
	if (isUpdateUseBuilds != 1 && isUpdateUseBuilds != 2)
		return false; // Не указано, какие сборки можно загружать
	
	if (isUpdateDownloadSetup == 1)
	{
		if (!szUpdateExeCmdLine || !*szUpdateExeCmdLine)
			return false; // Не указана строка запуска инсталлятора
	}
	else if (isUpdateDownloadSetup == 2)
	{
		if (!szUpdateArcCmdLine || !*szUpdateArcCmdLine)
			return false; // Не указана строка запуска архиватора
	}
	else
	{
		return false; // Не указан тип загружаемого пакета (exe/7z)
	}

	// Можно
	return true;
}
