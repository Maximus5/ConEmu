// NO BOM! Compiled with old gcc!

/*
Copyright (c) 2016-present Maximus5
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

#undef TEST_BUILD

#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>

#include "resource.h"
#include "VersionI.h"
#include "NextArg.h"

// String resources (messages)
#include "Setupper.h"

#ifdef __GNUC__
#include "gcc_fix.h"
#endif


// Global Variables:
HINSTANCE hInst;
OSVERSIONINFO gOSVer = {sizeof(OSVERSIONINFO)};
wchar_t gsTitle[128] = L""; // L"ConEmu %s installer"
wchar_t gsMessage[128] = L"";
wchar_t gsFull[1024] = L"";
wchar_t gsVer86[1024] = L"";
wchar_t gsVer64[1024] = L"";
wchar_t gsRunAsAdm[128] = L"";
wchar_t gsTempFolder[MAX_PATH-24] = L"";
wchar_t gsMsiFile[MAX_PATH] = L"";
wchar_t gsCabFile[MAX_PATH] = L"";
wchar_t gsExeFile[MAX_PATH] = L"";
bool gbExtractOnly = false;
bool gbUseElevation = false;
bool gbAlreadyAdmin = false;
bool gbAutoMode = false;
BOOL isWin64 = FALSE;

#define countof(a) (sizeof((a))/(sizeof(*(a))))



BOOL IsWindows64()
{
	BOOL is64bitOs = FALSE, isWow64process = FALSE;

	// Проверяем, где мы запущены
	isWow64process = FALSE;
	HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");

	if (hKernel)
	{
		typedef BOOL (WINAPI* IsWow64Process_t)(HANDLE hProcess, PBOOL Wow64Process);
		IsWow64Process_t IsWow64Process_f = (IsWow64Process_t)GetProcAddress(hKernel, "IsWow64Process");

		if (IsWow64Process_f)
		{
			BOOL bWow64 = FALSE;

			if (IsWow64Process_f(GetCurrentProcess(), &bWow64) && bWow64)
			{
				isWow64process = TRUE;
			}
		}
	}

	is64bitOs = isWow64process;

	return is64bitOs;
}

bool IsUserAdmin()
{
	// Проверять нужно только для висты и выше
	if (gOSVer.dwMajorVersion < 6)
		return FALSE;

	BOOL b;
	SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
	PSID AdministratorsGroup;
	b = AllocateAndInitializeSid(
	        &NtAuthority,
	        2,
	        SECURITY_BUILTIN_DOMAIN_RID,
	        DOMAIN_ALIAS_RID_ADMINS,
	        0, 0, 0, 0, 0, 0,
	        &AdministratorsGroup);

	if (b)
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
		{
			b = FALSE;
		}

		FreeSid(AdministratorsGroup);
	}

	return (b != 0);
}

HRESULT CALLBACK Callback(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData)
{
	switch (uNotification)
	{
		case TDN_CREATED:
		{
			if (!gbAlreadyAdmin)
			{
				SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver86, 1);
				SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver64, 1);
			}
			break;
		}
		case TDN_VERIFICATION_CLICKED:
		{
			SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver86, wParam);
			SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver64, wParam);
			break;
		}
		case TDN_HYPERLINK_CLICKED:
		{
			PCWSTR pszHREF = (PCWSTR)lParam;
			if (pszHREF && *pszHREF)
				ShellExecute(hwnd, L"open", pszHREF, NULL, NULL, SW_SHOWNORMAL);
			break;
		}
	}
	return S_OK;
}

int ChooseVersion()
{
	int nInstallVer = 0; // IDCANCEL/Ver86/Ver64

	#ifndef TEST_BUILD
	if (!isWin64 && (gOSVer.dwMajorVersion <= 5))
		nInstallVer = Ver86;
	#endif

	if (!nInstallVer && gOSVer.dwMajorVersion >= 6)
	{
	    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	    if (SUCCEEDED(hr))
	    {

			int nButtonPressed                  = 0;
			TASKDIALOGCONFIG config             = {0};
			const TASKDIALOG_BUTTON buttons[]   = {
			                                        { Ver64, gsVer64 },
			                                        { Ver86, gsVer86 },
			                                      };
			config.cbSize                       = sizeof(config);
			config.hInstance                    = hInst;
			config.dwFlags                      = TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION
			                                     |TDF_CAN_BE_MINIMIZED|TDF_ENABLE_HYPERLINKS; //|TDIF_SIZE_TO_CONTENT;
			config.pszMainIcon                  = MAKEINTRESOURCE(IDI_ICON1);
			config.pszWindowTitle               = gsTitle;
			config.pszMainInstruction           = gsMessage;
			//config.pszContent                 = L"Choose between x86 and x64 versions";
			config.pButtons                     = isWin64 ? buttons : buttons+1;
			config.cButtons                     = isWin64 ? countof(buttons) : countof(buttons)-1;
			config.nDefaultButton               = isWin64 ? Ver64 : Ver86;
			config.pszFooter                    = gsWWW;
			config.pfCallback                   = Callback;

			gbAlreadyAdmin = IsUserAdmin();

			if (!gbAlreadyAdmin)
			{
				config.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;
				config.pszVerificationText = gsRunAsAdm;
			}

			HMODULE hDll = LoadLibrary(L"comctl32.dll");
			typedef HRESULT (WINAPI* TaskDialogIndirect_t)(const TASKDIALOGCONFIG *pTaskConfig, int *pnButton, int *pnRadioButton, BOOL *pfVerificationFlagChecked);
			TaskDialogIndirect_t TaskDialogIndirect_f = (TaskDialogIndirect_t)(hDll?GetProcAddress(hDll, "TaskDialogIndirect"):NULL);

			BOOL lbCheckBox = TRUE;
			if (TaskDialogIndirect_f && TaskDialogIndirect_f(&config, &nButtonPressed, NULL, &lbCheckBox) == S_OK)
			{
				switch (nButtonPressed)
				{
			    case IDCANCEL: // user cancelled the dialog
				case Ver86:
				case Ver64:
					if (lbCheckBox)
						gbUseElevation = true;
					nInstallVer = nButtonPressed;
					break;
			    default:
			        break; // should never happen
				}
			}

			CoUninitialize();
		}
	}

	if (!nInstallVer)
	{
		// "Old" UI Controls dialog
		int nButtonPressed = MessageBox(NULL, gsFull, gsTitle, MB_ICONQUESTION|MB_YESNOCANCEL);
		switch (nButtonPressed)
		{
		case IDYES:
			nInstallVer = Ver64;
			break;
		case IDNO:
			nInstallVer = Ver86;
			break;
		default:
			nInstallVer = IDCANCEL;
			break;
		}
	}

	return nInstallVer;
}

// Helper class to log errors as UTF-8 text file
class CErrLog
{
protected:
	HANDLE mh_File;
	wchar_t ms_Temp[512];

	bool Valid()
	{
		return (mh_File && (mh_File != INVALID_HANDLE_VALUE));
	}

public:
	// UTF-8 is expected here
	void Write(LPCSTR asText)
	{
		if (!Valid() || !asText || !*asText)
			return;
		DWORD nLen = lstrlenA(asText);
		WriteFile(mh_File, asText, nLen, &nLen, NULL);
	};

	// Unicode
	void Write(LPCWSTR asText)
	{
		if (!Valid() || !asText || !*asText)
			return;
		DWORD nLen = lstrlen(asText);
		char* pszUtf8 = (char*)malloc(nLen*3+1);
		if (pszUtf8)
		{
			int nCvt = WideCharToMultiByte(CP_UTF8, 0, asText, nLen, pszUtf8, nLen*3, NULL, NULL);
			if (nCvt > 0)
			{
				pszUtf8[nCvt] = 0;
				Write(pszUtf8);
			}
			free(pszUtf8);
		}
	};

	void DumpEnvVars()
	{
		LPWCH pszEnvVars = GetEnvironmentStrings();
		if (pszEnvVars)
		{
			Write(L"Environment variables:\r\n");
			for (LPWCH psz = pszEnvVars; *psz; psz += (lstrlen(psz)+1))
			{
				Write(psz);
				Write(L"\r\n");
			}
		}
	};

public:
	CErrLog()
		: mh_File(NULL)
	{
		wchar_t szName[32] = L""; // ConEmu.160707.123456999.log
		SYSTEMTIME st = {}; GetLocalTime(&st);
		wsprintf(szName, L"ConEmu.%s.%02u%02u%02u%03u.log", CONEMUVERL, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		wchar_t szPath[MAX_PATH+1] = L"";
		if (GetModuleFileName(NULL, szPath, countof(szPath)-lstrlen(szName)-1) > 0)
		{
			wchar_t* pszSlash = (wchar_t*)wcsrchr(szPath, L'\\');
			if (!pszSlash)
				lstrcpyn(szPath, szName, countof(szPath));
			else
				lstrcpyn(pszSlash+1, szName, countof(szPath)-(pszSlash-szPath)-2);
			mh_File = CreateFile(szPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (Valid())
			{
				wsprintf(ms_Temp, L"ConEmu Setup %s on %u-%u-%u %02u:%02u:%02u\r\n", CONEMUVERL,
					st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
				Write(ms_Temp);

				wsprintf(ms_Temp, L"OS Version: %u.%u (build %u) %s\r\n",
					gOSVer.dwMajorVersion, gOSVer.dwMinorVersion, gOSVer.dwBuildNumber, gOSVer.szCSDVersion);
				Write(ms_Temp);

				Write(L"Command line: `");
				Write(GetCommandLine());
				Write(L"`\r\n");

				DWORD n = GetCurrentDirectory(countof(ms_Temp), ms_Temp);
				if (n && n < countof(ms_Temp))
				{
					Write(L"Current dir: `");
					Write(ms_Temp);
					Write(L"`\r\n");
				}

				n = GetTempPath(countof(ms_Temp), ms_Temp);
				if (n && n < countof(ms_Temp))
				{
					Write(L"GetTempPath: `");
					Write(ms_Temp);
					Write(L"`\r\n");
				}

				DumpEnvVars();

				Write(L"\r\n\r\n");
			}
		}
	};

	~CErrLog()
	{
		if (Valid())
			CloseHandle(mh_File);
	};
};

int ReportError(int nErr, LPCWSTR asLabel, void* pArg)
{
	wchar_t szMessage[2048];
	DWORD nLastErr = GetLastError();
	wsprintf(szMessage, asLabel, pArg);
	if (nLastErr != 0)
	{
		wsprintf(szMessage+lstrlen(szMessage), HIWORD(nLastErr) ? L"\nErrorCode=0x%08X" : L"\nErrorCode=%u", nLastErr);
	}

	// Log the problem to the file
	{
		// DO NOT WARN USER IN AUTOMATIC MODE !!! (gbAutoMode == true)
		// Instead, try to write time, title, message and nErr to "our-exe-name.log" file
		CErrLog eLog;
		eLog.Write(gsTitle);
		eLog.Write(L"\r\n");
		eLog.Write(szMessage);
		eLog.Write(L"\r\n");
		wchar_t szCode[80];
		wsprintf(szCode, L"ReportError code=%i\r\n", nErr);
		eLog.Write(szCode);
	}

	// Show UI message box in ‘manual’ mode only
	if (!gbAutoMode)
	{
		MessageBox(NULL, szMessage, gsTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
	}

	return nErr;
}

int ExportFile(int ResID, wchar_t* FilePath)
{
	HRSRC hResInfo = FindResource(hInst, MAKEINTRESOURCE(ResID), L"DATA");
	if (!hResInfo)
		return ReportError(exit_FindResource, msgFindResourceFailed, (void*)ResID);

	HGLOBAL hGbl = LoadResource(hInst, hResInfo);
	if (!hGbl)
		return ReportError(exit_LoadResource, msgLoadResourceFailed, (void*)ResID);

	DWORD nResSize = hResInfo ? SizeofResource(hInst, hResInfo) : 0;
	LPVOID pRes = hGbl ? LockResource(hGbl) : NULL;

	int iRc = 0;

	if (!nResSize || !pRes)
	{
		iRc = ReportError(exit_LockResource, msgLoadResourceFailed, (void*)ResID);
	}
	else
	{
		HANDLE hFile = CreateFile(FilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!hFile || hFile == INVALID_HANDLE_VALUE)
		{
			iRc = ReportError(exit_CreateFile, msgCreateFileFailed, FilePath);
		}
		else
		{
			DWORD nWritten = 0;
			if (!WriteFile(hFile, pRes, nResSize, &nWritten, NULL) || nWritten != nResSize)
			{
				iRc = ReportError(exit_WriteFile, msgWriteFileFailed, FilePath);
			}

			CloseHandle(hFile);

			if (iRc != 0)
				DeleteFile(FilePath);
		}
	}

	return iRc;
}

class CTempDir
{
protected:
	bool mb_RemoveDir;
	wchar_t ms_SelfPath[MAX_PATH+1];

public:
	CTempDir()
	{
		gsTempFolder[0] = 0;
		mb_RemoveDir = false;

		ms_SelfPath[0] = 0;
		DWORD n = GetModuleFileName(NULL, ms_SelfPath, countof(ms_SelfPath));
		if (!n || (n >= countof(ms_SelfPath)))
		{
			ms_SelfPath[0] = 0;
		}
		else
		{
			wchar_t *pch = wcsrchr(ms_SelfPath, L'\\');
			if (pch)
				*(pch+1) = 0;
			else
				ms_SelfPath[0] = 0;
		}
	};

	void DontRemove()
	{
		mb_RemoveDir = false;
	}

	bool IsDotsName(LPCWSTR asName)
	{
		return (asName && (asName[0] == L'.' && (asName[1] == 0 || (asName[1] == L'.' && asName[2] == 0))));
	};

	bool DirectoryExists(LPCWSTR asPath)
	{
		if (!asPath || !*asPath)
			return false;

		bool lbFound = false;

		HANDLE hFind = CreateFile(asPath, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (hFind && hFind != INVALID_HANDLE_VALUE)
		{
			BY_HANDLE_FILE_INFORMATION fi = {0};

			if (GetFileInformationByHandle(hFind, &fi) && (fi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				lbFound = true;
			}
			CloseHandle(hFind);

			return lbFound;
		}

		// Try to use FindFirstFile?
		WIN32_FIND_DATAW fnd = {0};
		hFind = FindFirstFile(asPath, &fnd);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		do
		{
			if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
			{
				// Each find returns "." and ".." items
				// We do not need them
				if (!IsDotsName(fnd.cFileName))
				{
					lbFound = TRUE;
					break;
				}
			}
		}
		while (FindNextFile(hFind, &fnd));

		FindClose(hFind);
		return (lbFound != FALSE);
	};

	bool Acquire()
	{
		if (gsTempFolder[0])
		{
			if (IsDotsName(gsTempFolder))
				return true;

			if (DirectoryExists(gsTempFolder))
				return true;

			if (CreateDirectory(gsTempFolder, NULL))
			{
				SetCurrentDirectory(gsTempFolder);
				mb_RemoveDir = true; // would be reset on success
				return true;
			}

			ReportError(exit_CreateDirectory, msgTempFolderFailed, gsTempFolder);
			if (gbExtractOnly || !gbAutoMode)
				return false;
		}
		else
		{
			// + "ConEmu160707" (CONEMUVERL) or "ConEmu160707_123456" (CONEMUVERL, hhmmss)
			DWORD cchMax = countof(gsTempFolder) - 20;
			//DWORD n = GetTempPath(cchMax, gsTempFolder);
			DWORD n = GetEnvironmentVariable(L"TEMP", gsTempFolder, cchMax);
			if (!n)
				n = GetTempPath(cchMax, gsTempFolder);

			if (n && ((n + 16) < cchMax))
			{
				CreateDirectory(gsTempFolder, NULL); // In case, `TEMP` variable points to non-existent folder
				wchar_t* pszSubDir = gsTempFolder+lstrlen(gsTempFolder);
				if (*(pszSubDir-1) != L'\\')
					*(pszSubDir++) = L'\\';
				lstrcpy(pszSubDir, L"ConEmu");
				pszSubDir += 6;
				lstrcpy(pszSubDir, CONEMUVERL);
				pszSubDir += lstrlen(pszSubDir);
				// If we succeeded with this folder `%TEMP%\ConEmuYYMMDD`
				if (CreateDirectory(gsTempFolder, NULL))
				{
					SetCurrentDirectory(gsTempFolder);
					mb_RemoveDir = true;
					return true;
				}

				// Another installer already started? Try this `%TEMP%\ConEmuYYMMDD_HHMMSS`
				SYSTEMTIME st = {}; GetLocalTime(&st);
				wsprintf(pszSubDir, L"_%02i%02i%02i", st.wHour, st.wMinute, st.wSecond);
				if (CreateDirectory(gsTempFolder, NULL))
				{
					SetCurrentDirectory(gsTempFolder);
                    mb_RemoveDir = true;
					return true;
				}
			}
		}

		ReportError(exit_CreateDirectory, msgTempFolderFailed, gsTempFolder);
		if (!gbAutoMode)
			return false;

		// Failed to get/create TEMP path?
		// Use our executable folder instead, set temp folder to L"."
		gsTempFolder[0] = L'.'; gsTempFolder[1] = 0;
		// And just change our current directory (it may be any folder ATM, e.g. system32)
		if (ms_SelfPath[0])
		{
			if (SetCurrentDirectory(ms_SelfPath))
			{
				DWORD n = GetCurrentDirectory(countof(gsTempFolder), gsTempFolder);
				if (!n || (n > countof(gsTempFolder)))
				{
					// Fail again?
					gsTempFolder[0] = L'.'; gsTempFolder[1] = 0;
				}
			}
		}
		// if the path is invalid - just do not change cd,
		// use current, set by user when they run our exe
		return true;
	};

	~CTempDir()
	{
		//MessageBox(NULL, L"~CTempDir()", gsTitle, MB_ICONINFORMATION);
		if (mb_RemoveDir
			&& (gsTempFolder[0])
			&& (!IsDotsName(gsTempFolder))
			)
		{
			// non-empty directory will not be removed,
			// that is expected behavior
			BOOL b;
			if (ms_SelfPath[0])
				b = SetCurrentDirectory(ms_SelfPath);
			b = RemoveDirectory(gsTempFolder);
			//if (!b)
			//{
			//	ReportError(99, L"RemoveDirectory(%s) failed", gsTempFolder);
			//}
		}
	};
};

void FillTempPath(const wchar_t* temp_path)
{
	if (!temp_path || !*temp_path)
	{
		gsTempFolder[0] = 0;
	}
	else if (*temp_path == L'"')
	{
		// quoted path, e.g.: /e:"C:\Temp Files"
		lstrcpyn(gsTempFolder, temp_path + 1, countof(gsTempFolder));
		const int len = lstrlen(gsTempFolder);
		if (len > 0 && gsTempFolder[len - 1] == L'"')
			gsTempFolder[len - 1] = L'\0';
	}
	else
	{
		// non-quoted path, e.g.: "/e:C:\Temp Files"
		lstrcpyn(gsTempFolder, temp_path, countof(gsTempFolder));
	}
}

#ifdef __GNUC__
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#endif
{
	hInst = hInstance;
	isWin64 = IsWindows64();

	GetVersionEx(&gOSVer);

	int nInstallVer = 0;

	wsprintf(gsTitle, msgConEmuInstaller, CONEMUVERL);
	lstrcpyn(gsRunAsAdm, msgRunSetupAsAdmin, countof(gsRunAsAdm));

	wchar_t szArg[MAX_PATH+1];
	const wchar_t* pszCmdToken = GetCommandLine();
	LPCWSTR pszCmdLineW = pszCmdToken;

	CTempDir temp_dir; // gsTempFolder[0] = 0;

	while ((0 == NextArg(&pszCmdToken, szArg)))
	{
		if (lstrcmp(szArg, L"/?") == 0 || lstrcmp(szArg, L"-?") == 0 || lstrcmp(szArg, L"-h") == 0
			|| lstrcmp(szArg, L"-help") == 0 || lstrcmp(szArg, L"--help") == 0)
		{
			MessageBox(NULL, msgUsageExample, gsTitle, MB_ICONINFORMATION);
			return exit_Cancelled;
		}

		// Special to stop processing of setupper switches
		if (lstrcmp(szArg, L"--") == 0)
		{
			break;
		}

		if ((*szArg == L'/') || (*szArg == L'-'))
		{
			// [/e[:<extract path>]]
			if (szArg[1] == L'e' || szArg[1] == L'E')
			{
				gbExtractOnly = true;
				if (szArg[2] == L':' && szArg[3])
					FillTempPath(szArg + 3);
				continue;
			}

			// [/t:<temp path>]
			// temp path to extract msi files
			if (szArg[1] == L't' || szArg[1] == L'T')
			{
				if (szArg[2] == L':' && szArg[3])
					FillTempPath(szArg + 3);
				continue;
			}

			// [/p:x86[,adm] | /p:x64[,adm]]
			// this should be last setupper switch, the tail goes to msi!
		    if (memcmp(szArg + 1, L"p:x", 3*sizeof(*szArg)) == 0)
		    {
		    	gbAlreadyAdmin = IsUserAdmin();
				if (lstrcmpi(szArg+4, L"86") == 0)
				{
					nInstallVer = Ver86;
				}
				else if (lstrcmpi(szArg+4, L"86,adm") == 0)
				{
					nInstallVer = Ver86;
					gbUseElevation = !gbAlreadyAdmin;
				}
				else if (lstrcmpi(szArg+4, L"64") == 0)
				{
					nInstallVer = Ver64;
				}
				else if (lstrcmpi(szArg+4, L"64,adm") == 0)
				{
					nInstallVer = Ver64;
					gbUseElevation = !gbAlreadyAdmin;
				}
				break;
			}

			// unknown switches goes to msi
			pszCmdToken = pszCmdLineW;
			break;
		}

		// Skip non-switches (e.g. executable name)
		pszCmdLineW = pszCmdToken;
	}

	if (!temp_dir.Acquire())
	{
		return exit_CreateDirectory;
	}

	if (!gbExtractOnly)
	{
		// If there are msi args (pszCmdToken is not empty)
		gbAutoMode = (pszCmdToken && *pszCmdToken);

		wchar_t szInstallPath[MAX_PATH+32];
		bool bInstalled;
		HKEY hk;

		lstrcpyn(gsMessage, msgChooseInstallVer, countof(gsMessage));

			szInstallPath[0] = 0; bInstalled = false;
			struct {HKEY hk; LPCWSTR path; LPCWSTR name; bool our;}
				Keys[] = {
					{HKEY_LOCAL_MACHINE,L"SOFTWARE\\ConEmu",L"InstallDir",true},
					//Current installer does not use FarManager installation dir anymore
					//{HKEY_LOCAL_MACHINE,L"SOFTWARE\\Far Manager",L"InstallDir"},
					//{HKEY_LOCAL_MACHINE,L"SOFTWARE\\Far2",L"InstallDir"},
					//{HKEY_LOCAL_MACHINE,L"SOFTWARE\\Far",L"InstallDir"},
				};
			for (size_t s = 0; s < countof(Keys); s++)
			{
				if (!RegOpenKeyEx(Keys[s].hk, Keys[s].path, 0, KEY_READ, &hk)
					|| !RegOpenKeyEx(Keys[s].hk, Keys[s].path, 0, KEY_READ|KEY_WOW64_32KEY, &hk))
				{
					wchar_t szPath[MAX_PATH+1] = {}; DWORD cbSize = sizeof(szPath)-2;
					LONG lRc = RegQueryValueEx(hk, Keys[s].name, NULL, NULL, (LPBYTE)szPath, &cbSize);
					RegCloseKey(hk);
					if (!lRc && *szPath)
					{
						bInstalled = Keys[s].our;
						lstrcpy(szInstallPath, szPath);
						cbSize = lstrlen(szInstallPath);
						if (szInstallPath[cbSize-1] == L'\\') szInstallPath[cbSize-1] = 0;
						break;
					}
				}
			}
			if (szInstallPath[0] == 0)
			{
				GetEnvironmentVariable(L"ProgramFiles", szInstallPath, MAX_PATH);
				int nLen = lstrlen(szInstallPath);
				lstrcat(szInstallPath, (nLen > 0 && szInstallPath[nLen-1] != L'\\') ? L"\\ConEmu" : L"ConEmu");
			}
			wsprintf(gsVer86, msgInstallFolderIs, CONEMUVERL, L"x86", bInstalled ? msgPathCurrent : msgPathDefault, szInstallPath);


		if (isWin64)
		{

				szInstallPath[0] = 0; bInstalled = false;
				struct {HKEY hk; LPCWSTR path; LPCWSTR name; bool our;}
					Keys[] = {
						{HKEY_LOCAL_MACHINE,L"SOFTWARE\\ConEmu",L"InstallDir_x64",true},
						//Current installer does not use FarManager installation dir anymore
						//{HKEY_LOCAL_MACHINE,L"SOFTWARE\\Far Manager",L"InstallDir_x64"},
						//{HKEY_LOCAL_MACHINE,L"SOFTWARE\\Far2",L"InstallDir_x64"},
						//{HKEY_LOCAL_MACHINE,L"SOFTWARE\\Far",L"InstallDir_x64"},
					};
				for (size_t s = 0; s < countof(Keys); s++)
				{
					if (!RegOpenKeyEx(Keys[s].hk, Keys[s].path, 0, KEY_READ|KEY_WOW64_64KEY, &hk))
					{
						wchar_t szPath[MAX_PATH+1] = {}; DWORD cbSize = sizeof(szPath)-2;
						LONG lRc = RegQueryValueEx(hk, Keys[s].name, NULL, NULL, (LPBYTE)szPath, &cbSize);
						RegCloseKey(hk);
						if (!lRc && *szPath)
						{
							bInstalled = Keys[s].our;
							lstrcpy(szInstallPath, szPath);
							cbSize = lstrlen(szInstallPath);
							if (szInstallPath[cbSize-1] == L'\\') szInstallPath[cbSize-1] = 0;
							break;
						}
					}
				}
				if (szInstallPath[0] == 0)
				{
					GetEnvironmentVariable(L"ProgramW6432", szInstallPath, MAX_PATH);
					int nLen = lstrlen(szInstallPath);
					lstrcat(szInstallPath, (nLen > 0 && szInstallPath[nLen-1] != L'\\') ? L"\\ConEmu" : L"ConEmu");
				}
				wsprintf(gsVer64, msgInstallFolderIs, CONEMUVERL, L"x64", bInstalled ? msgPathCurrent : msgPathDefault, szInstallPath);

			wsprintf(gsFull, msgInstallConfirm, gsMessage);
		}
		else
		{
			gsVer64[0] = 0;
		}
	}
	else
	{
		LPCWSTR szPath = gsTempFolder;
		lstrcpyn(gsMessage, msgChooseExtractVer, countof(gsMessage));
		wsprintf(gsVer86, msgExtractX86X64, CONEMUVERL, L"x86", szPath);
		wsprintf(gsVer64, msgExtractX86X64, CONEMUVERL, L"x64", szPath);
		wsprintf(gsFull, msgExtractConfirm, gsMessage, szPath);
	}

	if (nInstallVer == 0)
	{
		nInstallVer = ChooseVersion(); // IDCANCEL/Ver86/Ver64
	}

	if (nInstallVer != Ver86 && nInstallVer != Ver64)
	{
		return exit_Cancelled;
	}

	// Preparing full paths
	wsprintf(gsMsiFile, L"%s\\ConEmu.%s.%s.msi", gsTempFolder, CONEMUVERL, (nInstallVer == Ver86) ? L"x86" : L"x64");
	wsprintf(gsCabFile, L"%s\\ConEmu.cab", gsTempFolder);

	bool lbNeedExe = false;
	if (!gbExtractOnly && gOSVer.dwMajorVersion >= 6)
		lbNeedExe = true;

	if (!lbNeedExe)
		gsExeFile[0] = 0;
	else
		wsprintf(gsExeFile, L"%s\\ConEmuSetup.exe", gsTempFolder);

	int iExpMsi = ExportFile(nInstallVer, gsMsiFile);
	int iExpCab = (iExpMsi == 0) ? ExportFile(CABFILE, gsCabFile) : -1;
	int iExpExe = (!lbNeedExe) ? 0 : (iExpCab == 0) ? ExportFile(EXEFILE, gsExeFile) : -1;
	if (iExpMsi != 0 || iExpCab != 0 || iExpExe != 0)
	{
		DeleteFile(gsMsiFile);
		DeleteFile(gsCabFile);
		if (*gsExeFile)
			DeleteFile(gsExeFile);
		return (iExpMsi != 0) ? iExpMsi : iExpCab;
	}

	if (gbExtractOnly)
	{
		temp_dir.DontRemove();
		wchar_t szMessage[MAX_PATH*2];
		wsprintf(szMessage, msgExtractedSuccessfully, gsTempFolder);
		MessageBox(NULL, szMessage, gsTitle, MB_ICONINFORMATION);
		return exit_Succeeded;
	}

	int iInstRc = exit_Succeeded;
	SHELLEXECUTEINFO sei = {sizeof(sei)};
	wchar_t* pszParms = NULL;
	sei.fMask = SEE_MASK_NOCLOSEPROCESS|/*SEE_MASK_NOASYNC*/0x00000100; //|/*SEE_MASK_NOZONECHECKS*/0x00800000;
	sei.lpVerb = L"open";
	if (gOSVer.dwMajorVersion<=5 || !gbUseElevation)
	{
		sei.lpFile = gsMsiFile;
		sei.lpParameters = pszCmdToken;
	}
	else
	{
		// Executor has `<requestedExecutionLevel level="requireAdministrator" ...>` in manifest
		sei.lpFile = gsExeFile;
		int nMaxLen = lstrlen(gsMsiFile) + (pszCmdToken ? lstrlen(pszCmdToken) : 0) + 64;
		pszParms = (wchar_t*)malloc(nMaxLen*sizeof(wchar_t));
		wsprintf(pszParms, L"/i \"%s\" %s", gsMsiFile, pszCmdToken ? pszCmdToken : L"");
		sei.lpParameters = pszParms;
	}
	sei.lpDirectory = gsTempFolder;
	sei.nShow = SW_SHOWNORMAL;

	BOOL lbExecute = ShellExecuteEx(&sei);

	#if 0
	if (!lbExecute && lbNeedExe)
	{
		DWORD nErr = GetLastError();
		if (nErr == 1223)
		{
			// Отмена пользователем UAC, или правов не хватило?
			sei.fMask = SEE_MASK_NOCLOSEPROCESS|/*SEE_MASK_NOASYNC*/0x00000100; //|/*SEE_MASK_NOZONECHECKS*/0x00800000;
			sei.lpVerb = L"open";
			sei.lpFile = gsMsiFile;
			sei.lpParameters = pszCmdToken;
			sei.lpDirectory = gsTempFolder;
			sei.nShow = SW_SHOWNORMAL;

			lbExecute = ShellExecuteEx(&sei);
		}
	}
	#endif

	if (!lbExecute)
	{
		iInstRc = ReportError(exit_ShellExecuteEx, msgInstallerFailed, gsMsiFile);
	}
	else
	{
		if (!sei.hProcess)
		{
			iInstRc = ReportError(exit_NullProcess, msgInstallerFailed, gsMsiFile);
		}
		else
		{
			WaitForSingleObject(sei.hProcess, INFINITE);
			DWORD nCode = 0;
			SetLastError(0);
			wchar_t szFormat[256];
			if (GetExitCodeProcess(sei.hProcess, &nCode))
			{
				switch (nCode)
				{
				case 0:
					iInstRc = exit_Succeeded;
					break;
				case 1602: // cancelled by user
					iInstRc = exit_Cancelled; // don't show any errors
					break;
				case 3010: // reboot is required
					wsprintf(szFormat, msgRebootRequired, nCode);
					iInstRc = ReportError(exit_AddWin32Code+nCode, szFormat, gsMsiFile);
					break;
				default:
					wsprintf(szFormat, msgInstallerFailedEx, nCode);
					iInstRc = ReportError(exit_AddWin32Code+nCode, szFormat, gsMsiFile);
				}
			}
			else
			{
				lstrcpyn(szFormat, msgExitCodeFailed, countof(szFormat));
				iInstRc = ReportError(exit_ExitCodeProcess, szFormat, gsMsiFile);
			}
		}
	}

	DeleteFile(gsMsiFile);
	DeleteFile(gsCabFile);
	if (*gsExeFile)
		DeleteFile(gsExeFile);

	return iInstRc;
}
