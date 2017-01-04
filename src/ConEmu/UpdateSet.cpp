
/*
Copyright (c) 2011-2017 Maximus5
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
#include "header.h"
#include "UpdateSet.h"
#include "ConEmu.h"
#include "OptionsClass.h"

ConEmuUpdateSettings::ConEmuUpdateSettings()
{
	// struct, memset is valid
	memset(this, 0, sizeof(*this));
}

LPCWSTR ConEmuUpdateSettings::UpdateVerLocation()
{
	if (szUpdateVerLocation && *szUpdateVerLocation)
		return szUpdateVerLocation;
	return UpdateVerLocationDefault();
}

LPCWSTR ConEmuUpdateSettings::UpdateVerLocationDefault()
{
	static LPCWSTR pszDefault =
		//L"file://C:\\ConEmu-Update\\version.ini"
		//L"http://conemu.ru/version.ini"
		L"https://conemu.github.io/version.ini"
		;
	return pszDefault;
}

bool ConEmuUpdateSettings::IsVerLocationDeprecated(LPCWSTR asNewIniLocation)
{
	if (!asNewIniLocation || !*asNewIniLocation)
	{
		_ASSERTE(asNewIniLocation && *asNewIniLocation);
		return true;
	}

	// Allow to reset location via "ConEmu.exe -UpdateSrcSet -"
	if (lstrcmp(asNewIniLocation, L"-") == 0)
	{
		return true;
	}

	// Find domain
	LPCWSTR pszDomain = wcsstr(asNewIniLocation, L"://");
	if (!pszDomain || !*(pszDomain+3))
	{
		// Invalid location
		// If one needs to point on one's local drive,
		// proper format is: "file:///C:\path\version.ini"
		_ASSERTE(pszDomain != NULL);
		return true;
	}

	// Very old versions has following URL which is not available anymore
	// L"http://conemu-maximus5.googlecode.com/svn/trunk/ConEmu/version.ini"
	wchar_t szDeprecatedDomain[] = L"conemu-maximus5.googlecode.com";
	if (lstrcmpni(pszDomain+3, szDeprecatedDomain, lstrlen(szDeprecatedDomain)) == 0)
	{
		// Force new version to forget deprecated locations
		return true;
	}

	return false;
}

void ConEmuUpdateSettings::SetUpdateVerLocation(LPCWSTR asNewIniLocation)
{
	SafeFree(szUpdateVerLocation);

	if (asNewIniLocation && *asNewIniLocation
		&& !IsVerLocationDeprecated(asNewIniLocation))
	{
		szUpdateVerLocation = lstrdup(asNewIniLocation);
	}

	if (gpSetCls && ghOpWnd)
	{
		HWND hUpdate = gpSetCls->GetPage(thi_Update);
		if (hUpdate)
			SetDlgItemText(hUpdate, tUpdateVerLocation, gpSet->UpdSet.UpdateVerLocation());
	}
}

void ConEmuUpdateSettings::ResetToDefaults()
{
	// Указатели должны быть освобождены перед вызовом
	_ASSERTE(szUpdateExeCmdLine==NULL);

	szUpdateVerLocation = NULL;
	isUpdateCheckOnStartup = false;
	isUpdateCheckHourly = false;
	isUpdateConfirmDownload = true; // true-Show MsgBox, false-notify via TSA only
	isUpdateUseBuilds = 0; // 0-спросить пользователя при первом запуске, 1-stable only, 2-latest, 3-preview
	isUpdateInetTool = false;
	szUpdateInetTool = NULL;
	isUpdateUseProxy = false;
	szUpdateProxy = szUpdateProxyUser = szUpdateProxyPassword = NULL; // "Server:port"
	// Проверяем, была ли программа установлена через ConEmuSetup.exe?
	isUpdateDownloadSetup = 0; // 0-Auto, 1-Installer (ConEmuSetup.exe), 2-7z archive (ConEmu.7z), WinRar or 7z required
	isSetupDetected = 0; // 0-пока не проверялся, 1-установлено через Installer, пути совпали, 2-Installer не запускался
	isSetup64 = WIN3264TEST(false,true); // определяется вместе с isSetupDetected

	szUpdateExeCmdLineDef = lstrdup(L"\"%1\" /p:%3 /qr");
	SafeFree(szUpdateExeCmdLine);

	bool bWinRar = false;
	wchar_t* pszArcPath = NULL; BOOL bWin64 = IsWindows64();
	for (int i = 0; !(pszArcPath && *pszArcPath) && (i <= 5); i++)
	{
		SettingsRegistry regArc;
		switch (i)
		{
		case 0:
			if (regArc.OpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\7-Zip", KEY_READ|(bWin64?KEY_WOW64_32KEY:0)))
			{
				regArc.Load(L"Path", &pszArcPath);
			}
			break;
		case 1:
			if (bWin64 && regArc.OpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\7-Zip", KEY_READ|KEY_WOW64_64KEY))
			{
				regArc.Load(L"Path", &pszArcPath);
			}
			break;
		case 2:
			if (regArc.OpenKey(HKEY_CURRENT_USER, L"SOFTWARE\\7-Zip", KEY_READ))
			{
				regArc.Load(L"Path", &pszArcPath);
			}
			break;
		case 3:
			if (regArc.OpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WinRAR", KEY_READ|(bWin64?KEY_WOW64_32KEY:0)))
			{
				bWinRar = true;
				regArc.Load(L"exe32", &pszArcPath);
			}
			break;
		case 4:
			if (bWin64 && regArc.OpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WinRAR", KEY_READ|KEY_WOW64_64KEY))
			{
				bWinRar = true;
				regArc.Load(L"exe64", &pszArcPath);
			}
			break;
		case 5:
			if (regArc.OpenKey(HKEY_CURRENT_USER, L"SOFTWARE\\WinRAR", KEY_READ))
			{
				bWinRar = true;
				if (!regArc.Load(L"exe32", &pszArcPath) && bWin64)
				{
					regArc.Load(L"exe64", &pszArcPath);
				}
			}
			break;
		}
	}
	if (!pszArcPath || !*pszArcPath)
	{
		szUpdateArcCmdLineDef = lstrdup(L"\"%ProgramFiles%\\7-Zip\\7zg.exe\" x -y \"%1\""); // "%1"-archive file, "%2"-ConEmu base dir
	}
	else
	{
		LPCWSTR pszExt = PointToExt(pszArcPath);
		int cchMax = lstrlen(pszArcPath)+64;
		szUpdateArcCmdLineDef = (wchar_t*)malloc(cchMax*sizeof(wchar_t));
		if (szUpdateArcCmdLineDef)
		{
			if (pszExt && lstrcmpi(pszExt, L".exe") == 0)
			{
				_ASSERTE(bWinRar==true);
				//Issue 537: old WinRAR beta's fails
				//_wsprintf(szUpdateArcCmdLineDef, SKIPLEN(cchMax) L"\"%s\" x -y \"%%1\"%s", pszArcPath, bWinRar ? L" \"%%2\\\"" : L"");
				_wsprintf(szUpdateArcCmdLineDef, SKIPLEN(cchMax) L"\"%s\" x -y \"%%1\"", pszArcPath);
			}
			else
			{
				_ASSERTE(bWinRar==false);
				int nLen = lstrlen(pszArcPath);
				bool bNeedSlash = (*pszArcPath && (pszArcPath[nLen-1] != L'\\')) ? true : false;
				_wsprintf(szUpdateArcCmdLineDef, SKIPLEN(cchMax) L"\"%s%s7zg.exe\" x -y \"%%1\"", pszArcPath, bNeedSlash ? L"\\" : L"");
			}
		}
	}
	SafeFree(pszArcPath);
	SafeFree(szUpdateArcCmdLine);

	szUpdateDownloadPath = lstrdup(L"%TEMP%\\ConEmu");
	isUpdateLeavePackages = false;
	szUpdatePostUpdateCmd = lstrdup(L"echo Last successful update>ConEmuUpdate.info && date /t>>ConEmuUpdate.info && time /t>>ConEmuUpdate.info"); // Юзер может чего-то свое делать с распакованными файлами
}

ConEmuUpdateSettings::~ConEmuUpdateSettings()
{
	FreePointers();

	// Эти два параметра не освобождаются в FreePointers
	SafeFree(szUpdateExeCmdLineDef);
	SafeFree(szUpdateArcCmdLineDef);
}

void ConEmuUpdateSettings::FreePointers()
{
	SafeFree(szUpdateVerLocation);
	SafeFree(szUpdateInetTool);
	SafeFree(szUpdateProxy);
	SafeFree(szUpdateProxyUser);
	SafeFree(szUpdateProxyPassword);
	SafeFree(szUpdateExeCmdLine);
	//SafeFree(szUpdateExeCmdLineDef); -- нельзя
	SafeFree(szUpdateArcCmdLine);
	//SafeFree(szUpdateArcCmdLineDef); -- нельзя
	SafeFree(szUpdateDownloadPath);
	SafeFree(szUpdatePostUpdateCmd);
}

void ConEmuUpdateSettings::LoadFrom(ConEmuUpdateSettings* apFrom)
{
	FreePointers();

	szUpdateVerLocation = (apFrom->szUpdateVerLocation && *apFrom->szUpdateVerLocation) ? lstrdup(apFrom->szUpdateVerLocation) : NULL; // ConEmu latest version location info
	isUpdateCheckOnStartup = apFrom->isUpdateCheckOnStartup;
	isUpdateCheckHourly = apFrom->isUpdateCheckHourly;
	isUpdateConfirmDownload = apFrom->isUpdateConfirmDownload;
	isUpdateUseBuilds = (apFrom->isUpdateUseBuilds >= 1 && apFrom->isUpdateUseBuilds <= 3) ? apFrom->isUpdateUseBuilds : 2; // 1-stable only, 2-latest, 3-preivew
	isUpdateInetTool = apFrom->isUpdateInetTool;
	szUpdateInetTool = lstrdup(apFrom->szUpdateInetTool);
	isUpdateUseProxy = apFrom->isUpdateUseProxy;
	szUpdateProxy = lstrdup(apFrom->szUpdateProxy); // "Server:port"
	szUpdateProxyUser = lstrdup(apFrom->szUpdateProxyUser);
	szUpdateProxyPassword = lstrdup(apFrom->szUpdateProxyPassword);
	isUpdateDownloadSetup = apFrom->isUpdateDownloadSetup; // 0-Auto, 1-Installer (ConEmuSetup.exe), 2-7z archive (ConEmu.7z), WinRar or 7z required
	isSetupDetected = apFrom->isSetupDetected;
	// "%1"-archive or setup file, "%2"-ConEmu base dir, "%3"-x86/x64, "%4"-ConEmu PID
	szUpdateExeCmdLine = lstrdup(apFrom->szUpdateExeCmdLine);
	szUpdateExeCmdLineDef = lstrdup(apFrom->szUpdateExeCmdLineDef);
	szUpdateArcCmdLine = lstrdup(apFrom->szUpdateArcCmdLine);
	szUpdateArcCmdLineDef = lstrdup(apFrom->szUpdateArcCmdLineDef);
	szUpdateDownloadPath = lstrdup(apFrom->szUpdateDownloadPath); // "%TEMP%"
	isUpdateLeavePackages = apFrom->isUpdateLeavePackages;
	szUpdatePostUpdateCmd = lstrdup(apFrom->szUpdatePostUpdateCmd); // Юзер может чего-то свое делать с распакованными файлами
}

bool ConEmuUpdateSettings::UpdatesAllowed(wchar_t (&szReason)[128])
{
	szReason[0] = 0;

	if (!*UpdateVerLocation())
	{
		wcscpy_c(szReason, L"Update.VerLocation is empty");
		return false; // Не указано расположение обновления
	}

	if (isUpdateUseBuilds != 1 && isUpdateUseBuilds != 2 && isUpdateUseBuilds != 3)
	{
		wcscpy_c(szReason, L"Update.UseBuilds is not specified");
		return false; // Не указано, какие сборки можно загружать
	}

	wchar_t szCPU[4] = L"";

	switch (UpdateDownloadSetup())
	{
	case 1:
		if (!*UpdateExeCmdLine(szCPU))
		{
			wcscpy_c(szReason, L"Update.ExeCmdLine is not specified");
			return false; // Не указана строка запуска инсталлятора
		}
		break;
	case 2:
		{
			LPCWSTR pszCmd = UpdateArcCmdLine();
			if (!*pszCmd)
			{
				wcscpy_c(szReason, L"Update.ArcCmdLine is not specified");
				return false; // Не указана строка запуска архиватора
			}
			CEStr szExe;
			NextArg(&pszCmd, szExe);
			pszCmd = PointToName(szExe);
			if (!pszCmd || !*pszCmd)
			{
				wcscpy_c(szReason, L"Update.ArcCmdLine is invalid");
				return false; // Ошибка в строке запуска архиватора
			}
			if ((lstrcmpi(pszCmd, L"WinRar.exe") == 0) || (lstrcmpi(pszCmd, L"Rar.exe") == 0) || (lstrcmpi(pszCmd, L"UnRar.exe") == 0))
			{
				// Issue 537: AutoUpdate to the version 120509x64 unpacks to the wrong folder
				HKEY hk;
				DWORD nSubFolder = 0;
				if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\WinRAR\\Extraction\\Profile", 0, KEY_READ, &hk))
				{
					DWORD nSize = sizeof(nSubFolder);
					if (0 != RegQueryValueEx(hk, L"UnpToSubfolders", NULL, NULL, (LPBYTE)&nSubFolder, &nSize))
						nSubFolder = 0;
					RegCloseKey(hk);
				}

				if (nSubFolder)
				{
					wcscpy_c(szReason, L"Update.ArcCmdLine: Unwanted option\n[HKCU\\Software\\WinRAR\\Extraction\\Profile]\n\"UnpToSubfolders\"=1");
					return false; // Ошибка в настройке архиватора
				}
			}
		}
		break;
	default:
		wcscpy_c(szReason, L"Update.DownloadSetup is not specified");
		return false; // Не указан тип загружаемого пакета (exe/7z)
	}

	// Можно
	return true;
}

// 1-установлено через Installer, пути совпали, 2-Installer не запускался
BYTE ConEmuUpdateSettings::UpdateDownloadSetup()
{
	if (isUpdateDownloadSetup)
		return isUpdateDownloadSetup;

	// если 0 - пока не проверялся
	if (isSetupDetected == 0)
	{
		HKEY hk;
		LONG lRc;
		//bool bUseSetupExe = false;
		wchar_t szInstallDir[MAX_PATH+2], szExeDir[MAX_PATH+2];

		wcscpy_c(szExeDir, gpConEmu->ms_ConEmuExeDir);
		wcscat_c(szExeDir, L"\\");

		for (size_t i = 0; i <= 2; i++)
		{
			DWORD dwSam = KEY_READ | ((i == 0) ? 0 : (i == 1) ? KEY_WOW64_32KEY : KEY_WOW64_64KEY);
			bool x64 = ((i == 0) ? WIN3264TEST(false,true) : (i == 1) ? false : true);
			LPCWSTR pszName = x64 ? L"InstallDir_x64" : L"InstallDir";
			lRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\ConEmu", 0, dwSam, &hk);
			if (lRc == 0)
			{
				_ASSERTE(countof(szInstallDir)>(MAX_PATH+1));
				DWORD dwSize = MAX_PATH*sizeof(*szInstallDir);
				if (0 == RegQueryValueEx(hk, pszName, NULL, NULL, (LPBYTE)szInstallDir, &dwSize) && *szInstallDir)
				{
					size_t nLen = _tcslen(szInstallDir);
					if (szInstallDir[nLen-1] != L'\\')
						wcscat_c(szInstallDir, L"\\");
					if (lstrcmpi(szInstallDir, szExeDir) == 0)
					{
						isSetupDetected = 1;
						isSetup64 = x64;
					}
				}
				RegCloseKey(hk);
			}
		}

		if (!isSetupDetected)
			isSetupDetected = 2;
	}

	// Если признаки установки через "ConEmuSetup.exe" не найдены, или пути не совпали - грузим через 7z
	_ASSERTE(isSetupDetected!=0);
	return isSetupDetected ? isSetupDetected : 2;
}

LPCWSTR ConEmuUpdateSettings::UpdateExeCmdLine(wchar_t (&szCPU)[4])
{
	wcscpy_c(szCPU, isSetup64 ? L"x64" : L"x86");
	if (szUpdateExeCmdLine && *szUpdateExeCmdLine)
		return szUpdateExeCmdLine;
	return szUpdateExeCmdLineDef ? szUpdateExeCmdLineDef : L"";
}

LPCWSTR ConEmuUpdateSettings::UpdateArcCmdLine()
{
	if (szUpdateArcCmdLine && *szUpdateArcCmdLine)
		return szUpdateArcCmdLine;
	return szUpdateArcCmdLineDef ? szUpdateArcCmdLineDef : L"";;
}

LPCWSTR ConEmuUpdateSettings::GetUpdateInetToolCmd()
{
	static wchar_t szDefault[] = L"\"%ConEmuBaseDir%\\ConEmuC.exe\" -download %1 %2";
	LPCWSTR pszCommand = (isUpdateInetTool && szUpdateInetTool && *szUpdateInetTool)
		? szUpdateInetTool : szDefault;
	return pszCommand;
}

void ConEmuUpdateSettings::CheckHourlyUpdate()
{
	if (!dwLastUpdateCheck)
	{
		dwLastUpdateCheck = GetTickCount();
	}
	else
	{
		DWORD dwDelta = (GetTickCount() - dwLastUpdateCheck);
		if (dwDelta >= (60*60*1000))
		{
			gpConEmu->CheckUpdates(FALSE);
		}
	}
}
