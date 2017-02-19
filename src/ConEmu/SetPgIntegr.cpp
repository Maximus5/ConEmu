
/*
Copyright (c) 2016-2017 Maximus5
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
#define SHOWDEBUGSTR

#include "Header.h"
#include "../common/MArray.h"
#include "../common/MSetter.h"
#include "../common/WRegistry.h"

#include "ConEmu.h"
#include "Inside.h"
#include "OptionsClass.h"
#include "SetDlgLists.h"
#include "SetPgIntegr.h"

#define CONEMU_HERE_CMD  L"{cmd} -cur_console:n"
#define CONEMU_HERE_POSH L"{powershell} -cur_console:n"

struct Switch
{
	CEStr szSwitch;
	CEStr szOpt;

	// Format examples: `-single` or `-dir "..."`
	Switch(wchar_t* RVAL_REF asSwitch, wchar_t* RVAL_REF asOpt)
	{
		szSwitch.Attach((wchar_t* RVAL_REF)asSwitch);
		szOpt.Attach((wchar_t* RVAL_REF)asOpt);
	};

	~Switch()
	{
	};
};

struct SwitchParser
{
	MArray<Switch*> stdSwitches, ourSwitches;
	CEStr szCmd, szDirSync, szConfig;
	bool bCmdList; // `-runlist` was used

	static LPCWSTR* GetSkipSwitches()
	{
		static LPCWSTR ppszSkipStd[] = {
			L"-dir", L"-config",
			L"-inside", L"-inside:", L"-here",
			L"-multi", L"-nomulti",
			L"-Single", L"-NoSingle", L"-ReUse",
			L"-NoCascade", L"-DontCascade",
			NULL};
		return ppszSkipStd;
	};

	SwitchParser()
		: bCmdList(false)
	{
	};

	void ReleaseVectors()
	{
		Switch* p;
		while (stdSwitches.pop_back(p))
			SafeDelete(p);
		while (ourSwitches.pop_back(p))
			SafeDelete(p);
	};

	~SwitchParser()
	{
		ReleaseVectors();
	};

	bool IsIgnored(Switch* ps, LPCWSTR* ppszSkip)
	{
		if (!ps)
		{
			return true;
		}

		if (ppszSkip)
		{
			while (*ppszSkip)
			{
				if (ps->szSwitch.IsSwitch(*ppszSkip))
					return true;
				ppszSkip++;
			}
		}

		return false;
	};

	bool IsIgnored(Switch* ps, MArray<Switch*>& Skip)
	{
		if (!ps)
		{
			return true;
		}

		for (INT_PTR i = Skip.size()-1; i >= 0; i--)
		{
			if (!ps->szSwitch.IsSwitch(Skip[i]->szSwitch))
				continue;

			if (0 != ps->szOpt.Compare(Skip[i]->szOpt, true))
			{
				// Allow to add switch?
				return false;
			}
			return true;
		}

		return false;
	};

	Switch* GetNextSwitch(LPCWSTR& rpsz, CEStr& szArg)
	{
		LPCWSTR psz = rpsz;
		CEStr szNext;

		if ((0 == NextArg(&psz, szNext)) && !szNext.IsPossibleSwitch())
			rpsz = psz;
		else
			szNext.Clear();

		Switch* ps = new Switch(szArg.Detach(), szNext.Detach());
		return ps;
	};

	Switch* GetNextPair(LPCWSTR& rpsz)
	{
		LPCWSTR psz = rpsz;
		CEStr szArg;

		if (0 != NextArg(&psz, szArg))
		{
			return NULL;
		}

		// Invalid switch? or first argument (our executable)
		if (!szArg.IsPossibleSwitch())
		{
			rpsz = psz;
			return NULL;
		}

		rpsz = psz;

		return GetNextSwitch(rpsz, szArg);
	};

	// Parse switches stored in gpConEmu during initialization (AppendExtraArgs)
	// These are, for example, `-lngfile`, `-fontdir`, and so on.
	void ParseStdSwitches()
	{
		_ASSERTE(stdSwitches.empty());

		CEStr szArg, szNext, lsExta;
		LPCWSTR psz, pszExtraArgs;

		pszExtraArgs = gpConEmu->MakeConEmuStartArgs(lsExta);
		psz = pszExtraArgs;
		while (psz && *psz)
		{
			Switch* ps = GetNextPair(psz);
			if (!ps)
			{
				continue;
			}
			if (IsIgnored(ps, GetSkipSwitches()))
			{
				SafeDelete(ps);
				continue;
			}
			stdSwitches.push_back(ps);
		}
	};

	// pszFull comes from registry's [HKCR\Directory\shell\...\command]
	// Example:
	//	"C:\Tools\ConEmu.exe" -inside -LoadCfgFile "C:\Tools\ConEmu.xml" -FontDir C:\Tools\ConEmu
	//			-lngfile C:\Tools\ConEmu\ConEmu.l10n -lng ru  -dir "%1" -run {cmd} -cur_console:n
	// Strip switches which match current instance startup arguments
	// No sense to show them (e.g. "-lng ru") in the Integration dialog page
	void StripDupSwitches(LPCWSTR pszFull)
	{
		bCmdList = false;
		szCmd = L"";
		szDirSync = L"";
		szConfig = L"";
		ReleaseVectors();

		CEStr szArg, szNext;
		LPCWSTR psz;
		Switch* ps = NULL;

		// First, parse our extra args (passed to current ConEmu.exe)
		ParseStdSwitches();

		// Now parse new switches (command from registry or from field on Integration page)
		// Drop `-dir "..."` (especially from registry) always!

		psz = pszFull;
		while (0 == NextArg(&psz, szArg))
		{
			if (!szArg.IsPossibleSwitch())
				continue;

			if (szArg.OneOfSwitches(L"-inside", L"-here"))
			{
				// Nop
			}
			else if (szArg.IsSwitch(L"-inside:")) // Both "-inside:" and "-inside=" notations are supported
			{
				szDirSync.Set(szArg.Mid(8)); // may be empty!
			}
			else if (szArg.IsSwitch(L"-config"))
			{
				if (0 != NextArg(&psz, szArg))
					break;
				szConfig.Set(szArg);
			}
			else if (szArg.IsSwitch(L"-dir"))
			{
				if (0 != NextArg(&psz, szArg))
					break;
				_ASSERTE(lstrcmpi(szArg, L"%1")==0);
			}
			else if (szArg.OneOfSwitches(L"-Single", L"-NoSingle", L"-ReUse"))
			{
				ps = new Switch(szArg.Detach(), NULL);
				ourSwitches.push_back(ps);
			}
			else if (szArg.OneOfSwitches(L"-run", L"-cmd", L"-runlist", L"-cmdlist"))
			{
				// FIN! LAST SWITCH!
				szCmd.Set(psz);
				bCmdList = szArg.OneOfSwitches(L"-runlist",L"-cmdlist");
				break;
			}
			else if (NULL != (ps = GetNextSwitch(psz, szArg)))
			{
				if (IsIgnored(ps, GetSkipSwitches())
					|| IsIgnored(ps, stdSwitches))
				{
					SafeDelete(ps);
				}
				else
				{
					ourSwitches.push_back(ps);
				}
			}
		}
	};

	// Create the command to show on the Integration settings page
	LPCWSTR CreateCommand(CEStr& rsReady)
	{
		rsReady = L"";

		for (INT_PTR i = 0; i < ourSwitches.size(); i++)
		{
			Switch* ps = ourSwitches[i];
			_ASSERTE(ps && !ps->szSwitch.IsEmpty());
			bool bOpt = !ps->szOpt.IsEmpty();
			bool bQuot = (bOpt && IsQuotationNeeded(ps->szOpt));
			lstrmerge(&rsReady.ms_Val,
				ps->szSwitch,
				bOpt ? L" " : NULL,
				bQuot ? L"\"" : NULL,
				bOpt ? ps->szOpt.c_str() : NULL,
				bQuot ? L"\" " : L" ");
		}

		if (!rsReady.IsEmpty() || bCmdList)
		{
			lstrmerge(&rsReady.ms_Val,
				bCmdList ? L"-runlist " : L"-run ", szCmd);
		}
		else
		{
			rsReady.Set(szCmd);
		}

		return rsReady.c_str(L"");
	};
};



CSetPgIntegr::CSetPgIntegr()
{
}

CSetPgIntegr::~CSetPgIntegr()
{
}

LRESULT CSetPgIntegr::OnInitDialog(HWND hDlg, bool abInitial)
{
	MSetter lSkip(&mb_SkipSelChange);

	int iHere = 0, iInside = 0;
	ReloadHereList(&iHere, &iInside);

	// Возвращает NULL, если строка пустая
	wchar_t* pszCurInside = GetDlgItemTextPtr(hDlg, cbInsideName);
	_ASSERTE((pszCurInside==NULL) || (*pszCurInside!=0));
	wchar_t* pszCurHere   = GetDlgItemTextPtr(hDlg, cbHereName);
	_ASSERTE((pszCurHere==NULL) || (*pszCurHere!=0));

	wchar_t szIcon[MAX_PATH+32];
	_wsprintf(szIcon, SKIPLEN(countof(szIcon)) L"%s,0", gpConEmu->ms_ConEmuExe);

	if ((iInside > 0) && pszCurInside)
	{
		FillHereValues(cbInsideName);
	}
	else if (abInitial)
	{
		SetDlgItemText(hDlg, cbInsideName, L"ConEmu Inside");
		SetDlgItemText(hDlg, tInsideConfig, L"shell");
		SetDlgItemText(hDlg, tInsideShell, CONEMU_HERE_POSH);
		//SetDlgItemText(hDlg, tInsideIcon, szIcon);
		SetDlgItemText(hDlg, tInsideIcon, L"powershell.exe");
		checkDlgButton(hDlg, cbInsideSyncDir, gpConEmu->mp_Inside && gpConEmu->mp_Inside->mb_InsideSynchronizeCurDir);
		SetDlgItemText(hDlg, tInsideSyncDir, L""); // Auto
	}

	if ((iHere > 0) && pszCurHere)
	{
		FillHereValues(cbHereName);
	}
	else if (abInitial)
	{
		SetDlgItemText(hDlg, cbHereName, L"ConEmu Here");
		SetDlgItemText(hDlg, tHereConfig, L"");
		SetDlgItemText(hDlg, tHereShell, CONEMU_HERE_CMD);
		SetDlgItemText(hDlg, tHereIcon, szIcon);
	}

	SafeFree(pszCurInside);
	SafeFree(pszCurHere);

	return 0;
}

INT_PTR CSetPgIntegr::PageDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR iRc = 0;

	switch (messg)
	{
	case WM_NOTIFY:
		{
			LPNMHDR phdr = (LPNMHDR)lParam;

			if (phdr->code == TTN_GETDISPINFO)
			{
				return gpSetCls->ProcessTipHelp(hDlg, messg, wParam, lParam);
			}

			break;
		}
	case WM_INITDIALOG:
		{
			_ASSERTE(FALSE && "Unexpected, CSetPgIntegr::OnInitDialog must be called instead!");
		}
		break; // WM_INITDIALOG

	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case BN_CLICKED:
			{
				WORD CB = LOWORD(wParam);

				switch (CB)
				{
				case cbInsideSyncDir:
					if (gpConEmu->mp_Inside)
					{
						gpConEmu->mp_Inside->mb_InsideSynchronizeCurDir = isChecked2(hDlg, CB);
					}
					break;
				case bInsideRegister:
				case bInsideUnregister:
					ShellIntegration(hDlg, ShellIntgr_Inside, CB==bInsideRegister);
					ReloadHereList();
					if (CB==bInsideUnregister)
						FillHereValues(cbInsideName);
					break;
				case bHereRegister:
				case bHereUnregister:
					ShellIntegration(hDlg, ShellIntgr_Here, CB==bHereRegister);
					ReloadHereList();
					if (CB==bHereUnregister)
						FillHereValues(cbHereName);
					break;
				}
			}
			break; // BN_CLICKED
		case EN_CHANGE:
			{
				WORD EB = LOWORD(wParam);
				switch (EB)
				{
				case tInsideSyncDir:
					if (gpConEmu->mp_Inside)
					{
						SafeFree(gpConEmu->mp_Inside->ms_InsideSynchronizeCurDir);
						gpConEmu->mp_Inside->ms_InsideSynchronizeCurDir = GetDlgItemTextPtr(hDlg, tInsideSyncDir);
					}
					break;
				}
			}
			break; // EN_CHANGE
		case CBN_SELCHANGE:
			{
				WORD CB = LOWORD(wParam);
				switch (CB)
				{
				case cbInsideName:
				case cbHereName:
					if (!mb_SkipSelChange)
					{
						FillHereValues(CB);
					}
					break;
				}
			}
			break; // CBN_SELCHANGE
		} // switch (HIWORD(wParam))
		break; // WM_COMMAND

	}

	return iRc;
}

void CSetPgIntegr::RegisterShell(LPCWSTR asName, LPCWSTR asMode, LPCWSTR asConfig, LPCWSTR asCmd, LPCWSTR asIcon)
{
	if (!asName || !*asName)
		asName = L"ConEmu";

	if (!asCmd || !*asCmd)
		asCmd = CONEMU_HERE_CMD;

	asCmd = SkipNonPrintable(asCmd);

	bool isExtendedItem = isPressed(VK_SHIFT);

	#ifdef _DEBUG
	CEStr szMode(asMode);
	_ASSERTE(szMode.IsSwitch(L"-here") || szMode.IsSwitch(L"-inside") || szMode.IsSwitch(L"-inside:"));
	#endif

	CEStr lsExta;
	LPCWSTR pszExtraArgs = gpConEmu->MakeConEmuStartArgs(lsExta, asConfig);

	size_t cchMax = _tcslen(gpConEmu->ms_ConEmuExe)
		+ (asMode ? (_tcslen(asMode) + 3) : 0)
		+ (pszExtraArgs ? (_tcslen(pszExtraArgs) + 1) : 0)
		+ _tcslen(asCmd) + 32;
	wchar_t* pszCmd = (wchar_t*)malloc(cchMax*sizeof(*pszCmd));
	if (!pszCmd)
		return;

	//[HKEY_CURRENT_USER\Software\Classes\*\shell\ConEmu inside]
	//"Icon"="C:\\Program Files\\ConEmu\\ConEmu.exe,1"

	//[HKEY_CURRENT_USER\Software\Classes\*\shell\ConEmu inside\command]
	//@="C:\\Program Files\\ConEmu\\ConEmu.exe /inside /config shell /cmd {powershell} -cur_console:n"

	//[HKEY_CURRENT_USER\Software\Classes\Directory\Background\shell\ConEmu inside]
	//"Icon"="C:\\Program Files\\ConEmu\\ConEmu.exe,1"

	//[HKEY_CURRENT_USER\Software\Classes\Directory\Background\shell\ConEmu inside\command]
	//@="C:\\Program Files\\ConEmu\\ConEmu.exe /inside /config shell /cmd {powershell} -cur_console:n"

	//[HKEY_CURRENT_USER\Software\Classes\Directory\shell\ConEmu inside]
	//"Icon"="C:\\Program Files\\ConEmu\\ConEmu.exe,1"

	//[HKEY_CURRENT_USER\Software\Classes\Directory\shell\ConEmu inside\command]
	//@="C:\\Program Files\\ConEmu\\ConEmu.exe /inside /config shell /dir \"%1\" /cmd {powershell} -cur_console:n"

	//[HKEY_CURRENT_USER\Software\Classes\Drive\shell\ConEmu inside]
	//"Icon"="C:\\Program Files\\ConEmu\\ConEmu.exe,1"

	//[HKEY_CURRENT_USER\Software\Classes\Drive\shell\ConEmu inside\command]
	//@="C:\\Program Files\\ConEmu\\ConEmu.exe /inside /config shell /dir \"%1\" /cmd {powershell} -cur_console:n"

	int iSucceeded = 0;
	bool bHasLibraries = IsWindows7;

	for (int i = 1; i <= 6; i++)
	{
		_wcscpy_c(pszCmd, cchMax, L"\"");
		_wcscat_c(pszCmd, cchMax, gpConEmu->ms_ConEmuExe);
		_wcscat_c(pszCmd, cchMax, L"\" ");

		// `-here`, `-inside` or `-inside:cd ...`
		if (asMode && *asMode)
		{
			bool bQ = IsQuotationNeeded(asMode);
			if (bQ) _wcscat_c(pszCmd, cchMax, L"\"");
			_wcscat_c(pszCmd, cchMax, asMode);
			_wcscat_c(pszCmd, cchMax, bQ ? L"\" " : L" ");
		}

		// -FontDir, -LoadCfgFile, -Config, etc.
		if (pszExtraArgs && *pszExtraArgs)
		{
			_wcscat_c(pszCmd, cchMax, pszExtraArgs);
			_wcscat_c(pszCmd, cchMax, L" ");
		}

		LPCWSTR pszRoot = NULL;
		switch (i)
		{
		case 1:
			pszRoot = L"Software\\Classes\\*\\shell";
			break;
		case 2:
			pszRoot = L"Software\\Classes\\Directory\\Background\\shell";
			break;
		case 3:
			if (!bHasLibraries)
				continue;
			pszRoot = L"Software\\Classes\\LibraryFolder\\Background\\shell";
			break;
		case 4:
			pszRoot = L"Software\\Classes\\Directory\\shell";
			_wcscat_c(pszCmd, cchMax, L"-dir \"%1\" ");
			break;
		case 5:
			pszRoot = L"Software\\Classes\\Drive\\shell";
			_wcscat_c(pszCmd, cchMax, L"-dir \"%1\" ");
			break;
		case 6:
			// Issue 1191: ConEmu was launched instead of explorer from taskbar pinned library icon
			continue;
			//if (!bHasLibraries)
			//	continue;
			//pszRoot = L"Software\\Classes\\LibraryFolder\\shell";
			//_wcscat_c(pszCmd, cchMax, L"-dir \"%1\" ");
			//break;
		}

		bool bCmdKeyExist = false;

		if (*asCmd == L'/')
		{
			CEStr lsArg;
			LPCWSTR pszTemp = asCmd;
			while (0 == NextArg(&pszTemp, lsArg))
			{
				if (lsArg.OneOfSwitches(L"-run",L"-runlist",L"-cmd",L"-cmdlist"))
				{
					bCmdKeyExist = true; break;
				}
			}

		}

		if (!bCmdKeyExist)
			_wcscat_c(pszCmd, cchMax, L"-run ");
		_wcscat_c(pszCmd, cchMax, asCmd);

		HKEY hkRoot;
		if (0 == RegCreateKeyEx(HKEY_CURRENT_USER, pszRoot, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkRoot, NULL))
		{
			HKEY hkConEmu;
			if (0 == RegCreateKeyEx(hkRoot, asName, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkConEmu, NULL))
			{
				// Если задана "иконка"
				if (asIcon)
					RegSetValueEx(hkConEmu, L"Icon", 0, REG_SZ, (LPBYTE)asIcon, (lstrlen(asIcon)+1)*sizeof(*asIcon));
				else
					RegDeleteValue(hkConEmu, L"Icon");

				// Команда
				HKEY hkCmd;
				if (0 == RegCreateKeyEx(hkConEmu, L"command", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkCmd, NULL))
				{
					if (0 == RegSetValueEx(hkCmd, NULL, 0, REG_SZ, (LPBYTE)pszCmd, (lstrlen(pszCmd)+1)*sizeof(*pszCmd)))
						iSucceeded++;
					RegCloseKey(hkCmd);
				}

				// Extended item - Explorer shows them when 'Shift' is pressed
				wchar_t szDummy[1] = {0};
				if (isExtendedItem)
					RegSetValueEx(hkConEmu, L"Extended", 0, REG_SZ, reinterpret_cast<LPBYTE>(szDummy), 1*sizeof(szDummy[0]));
				else
					RegDeleteValue(hkConEmu, L"Extended");

				RegCloseKey(hkConEmu);
			}
			RegCloseKey(hkRoot);
		}
	}

	free(pszCmd);
}

void CSetPgIntegr::UnregisterShell(LPCWSTR asName)
{
	RegDeleteKeyRecursive(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\Background\\shell", asName);
	RegDeleteKeyRecursive(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell", asName);
	RegDeleteKeyRecursive(HKEY_CURRENT_USER, L"Software\\Classes\\Drive\\shell", asName);
	RegDeleteKeyRecursive(HKEY_CURRENT_USER, L"Software\\Classes\\*\\shell", asName);
	RegDeleteKeyRecursive(HKEY_CURRENT_USER, L"Software\\Classes\\LibraryFolder\\Background\\shell", asName);
	RegDeleteKeyRecursive(HKEY_CURRENT_USER, L"Software\\Classes\\LibraryFolder\\shell", asName);
}

// Issue 1191: ConEmu was launched instead of explorer from taskbar pinned library icon
void CSetPgIntegr::UnregisterShellInvalids()
{
	HKEY hkDir;

	if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Classes\\LibraryFolder\\shell", 0, KEY_READ, &hkDir))
	{
		int iOthers = 0;
		MArray<wchar_t*> lsNames;

		for (DWORD i = 0; i < 512; i++)
		{
			wchar_t szName[MAX_PATH+32] = {};
			wchar_t szCmd[MAX_PATH*4];
			DWORD cchMax = countof(szName) - 32;
			if (0 != RegEnumKeyEx(hkDir, i, szName, &cchMax, NULL, NULL, NULL, NULL))
				break;
			wchar_t* pszSlash = szName + _tcslen(szName);
			wcscat_c(szName, L"\\command");
			HKEY hkCmd = NULL;
			if (0 == RegOpenKeyEx(hkDir, szName, 0, KEY_READ, &hkCmd))
			{
				DWORD cbMax = sizeof(szCmd)-2;
				if (0 == RegQueryValueEx(hkCmd, NULL, NULL, NULL, (LPBYTE)szCmd, &cbMax))
				{
					szCmd[cbMax>>1] = 0;
					*pszSlash = 0;
					//LPCWSTR pszInside = StrStrI(szCmd, L"-inside");
					LPCWSTR pszConEmu = StrStrI(szCmd, L"conemu");
					if (pszConEmu)
						lsNames.push_back(lstrdup(szName));
					else
						iOthers++;
				}
				RegCloseKey(hkCmd);
			}
		}
		RegCloseKey(hkDir);


		wchar_t* pszName;
		while (lsNames.pop_back(pszName))
		{
			RegDeleteKeyRecursive(HKEY_CURRENT_USER, L"Software\\Classes\\LibraryFolder\\shell", pszName);
			SafeFree(pszName);
		}

		// If there is no other "commands" - try to delete "shell" subkey.
		// No worse if there are any other "non commands" - RegDeleteKey will do nothing.
		if ((iOthers == 0) && (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Classes\\LibraryFolder", 0, KEY_ALL_ACCESS, &hkDir)))
		{
			RegDeleteKey(hkDir, L"shell");
			RegCloseKey(hkDir);
		}
	}
}

void CSetPgIntegr::ShellIntegration(HWND hDlg, CSetPgIntegr::ShellIntegrType iMode, bool bEnabled, bool bForced /*= false*/)
{
	switch (iMode)
	{
	case ShellIntgr_Inside:
		{
			wchar_t szName[MAX_PATH] = {};
			GetDlgItemText(hDlg, cbInsideName, szName, countof(szName));
			if (bEnabled)
			{
				wchar_t szConfig[MAX_PATH] = {}, szShell[MAX_PATH] = {}, szIcon[MAX_PATH+16] = {}, szOpt[130] = {};
				GetDlgItemText(hDlg, tInsideConfig, szConfig, countof(szConfig));
				GetDlgItemText(hDlg, tInsideShell, szShell, countof(szShell));

				if (isChecked(hDlg, cbInsideSyncDir))
				{
					wcscpy_c(szOpt, L"-inside:");
					int nOL = lstrlen(szOpt); _ASSERTE(nOL==8);
					GetDlgItemText(hDlg, tInsideSyncDir, szOpt+nOL, countof(szShell)-nOL);
					if (szOpt[8] == 0)
					{
						szOpt[0] = 0;
					}
				}

				//_wsprintf(szIcon, SKIPLEN(countof(szIcon)) L"%s,0", gpConEmu->ms_ConEmuExe);
				GetDlgItemText(hDlg, tInsideIcon, szIcon, countof(szIcon));
				RegisterShell(szName, szOpt[0] ? szOpt : L"-inside", SkipNonPrintable(szConfig), SkipNonPrintable(szShell), szIcon);
			}
			else if (*szName)
			{
				UnregisterShell(szName);
			}
		}
		break;

	case ShellIntgr_Here:
		{
			wchar_t szName[MAX_PATH] = {};
			GetDlgItemText(hDlg, cbHereName, szName, countof(szName));
			if (bEnabled)
			{
				wchar_t szConfig[MAX_PATH] = {}, szShell[MAX_PATH] = {}, szIcon[MAX_PATH+16];
				GetDlgItemText(hDlg, tHereConfig, szConfig, countof(szConfig));
				GetDlgItemText(hDlg, tHereShell, szShell, countof(szShell));
				//_wsprintf(szIcon, SKIPLEN(countof(szIcon)) L"%s,0", gpConEmu->ms_ConEmuExe);
				GetDlgItemText(hDlg, tHereIcon, szIcon, countof(szIcon));
				RegisterShell(szName, L"-here", SkipNonPrintable(szConfig), SkipNonPrintable(szShell), szIcon);
			}
			else if (*szName)
			{
				UnregisterShell(szName);
			}
		}
		break;

	}
}

bool CSetPgIntegr::ReloadHereList(int* pnHere /*= NULL*/, int* pnInside /*= NULL*/)
{
	if (pnHere) *pnHere = 0;
	if (pnInside) *pnInside = 0;

	HKEY hkDir = NULL;
	size_t cchCmdMax = 65535;
	wchar_t* pszCmd = (wchar_t*)calloc(cchCmdMax,sizeof(*pszCmd));
	if (!pszCmd)
		return false;

	int iTotalCount = 0;

	// Возвращает NULL, если строка пустая
	wchar_t* pszCurInside = GetDlgItemTextPtr(mh_Dlg, cbInsideName);
	_ASSERTE((pszCurInside==NULL) || (*pszCurInside!=0));
	wchar_t* pszCurHere   = GetDlgItemTextPtr(mh_Dlg, cbHereName);
	_ASSERTE((pszCurHere==NULL) || (*pszCurHere!=0));

	MSetter lSkip(&mb_SkipSelChange);

	SendDlgItemMessage(mh_Dlg, cbInsideName, CB_RESETCONTENT, 0, 0);
	SendDlgItemMessage(mh_Dlg, cbHereName, CB_RESETCONTENT, 0, 0);

	if (0 == RegOpenKeyEx(HKEY_CLASSES_ROOT, L"Directory\\shell", 0, KEY_READ, &hkDir))
	{
		for (DWORD i = 0; i < 512; i++)
		{
			wchar_t szName[MAX_PATH+32] = {};
			DWORD cchMax = countof(szName) - 32;
			if (0 != RegEnumKeyEx(hkDir, i, szName, &cchMax, NULL, NULL, NULL, NULL))
				break;
			wchar_t* pszSlash = szName + _tcslen(szName);
			wcscat_c(szName, L"\\command");
			HKEY hkCmd = NULL;
			if (0 == RegOpenKeyEx(hkDir, szName, 0, KEY_READ, &hkCmd))
			{
				DWORD cbMax = (cchCmdMax-2) * sizeof(*pszCmd);
				if (0 == RegQueryValueEx(hkCmd, NULL, NULL, NULL, (LPBYTE)pszCmd, &cbMax))
				{
					pszCmd[cbMax>>1] = 0;
					*pszSlash = 0;

					bool bHasInside = false, bConEmu = false;
					LPCWSTR pszTemp = pszCmd; CEStr szArg;
					if (0 == NextArg(&pszTemp, szArg))
					{
						if ((bConEmu = IsConEmuGui(szArg)))
						{
							while (0 == NextArg(&pszTemp, szArg))
							{
								if (szArg.IsSwitch(L"-inside"))
								{
									bHasInside = true; break;
								}
								else if (szArg.OneOfSwitches(L"-run",L"-runlist",L"-cmd",L"-cmdlist"))
								{
									break; // stop checking
								}
							}
						}
					}

					if (bConEmu)
					{
						int* pnCounter = bHasInside ? pnInside : pnHere;
						if (pnCounter)
							++(*pnCounter);
						iTotalCount++;

						UINT nListID = bHasInside ? cbInsideName : cbHereName;
						SendDlgItemMessage(mh_Dlg, nListID, CB_ADDSTRING, 0, (LPARAM)szName);

						if ((bHasInside ? pszCurInside : pszCurHere) == NULL)
						{
							if (bHasInside)
								pszCurInside = lstrdup(szName);
							else
								pszCurHere = lstrdup(szName);
						}
					}
				}
				RegCloseKey(hkCmd);
			}
		}
		RegCloseKey(hkDir);
	}

	SetDlgItemText(mh_Dlg, cbInsideName, pszCurInside ? pszCurInside : L"");
	if (pszCurInside && *pszCurInside)
		CSetDlgLists::SelectStringExact(mh_Dlg, cbInsideName, pszCurInside);

	SetDlgItemText(mh_Dlg, cbHereName, pszCurHere ? pszCurHere : L"");
	if (pszCurHere && *pszCurHere)
		CSetDlgLists::SelectStringExact(mh_Dlg, cbHereName, pszCurHere);

	SafeFree(pszCurInside);
	SafeFree(pszCurHere);

	free(pszCmd);

	return (iTotalCount > 0);
}

void CSetPgIntegr::FillHereValues(WORD CB)
{
	CEStr szCmd;
	SwitchParser Switches;
	wchar_t *pszIco = NULL, *pszFull = NULL;

	INT_PTR iSel = SendDlgItemMessage(mh_Dlg, CB, CB_GETCURSEL, 0,0);
	if (iSel >= 0)
	{
		INT_PTR iLen = SendDlgItemMessage(mh_Dlg, CB, CB_GETLBTEXTLEN, iSel, 0);
		size_t cchMax = iLen+128;
		wchar_t* pszName = (wchar_t*)calloc(cchMax,sizeof(*pszName));
		if ((iLen > 0) && pszName)
		{
			_wcscpy_c(pszName, cchMax, L"Directory\\shell\\");
			SendDlgItemMessage(mh_Dlg, CB, CB_GETLBTEXT, iSel, (LPARAM)(pszName+_tcslen(pszName)));

			HKEY hkShell = NULL;
			if (0 == RegOpenKeyEx(HKEY_CLASSES_ROOT, pszName, 0, KEY_READ, &hkShell))
			{
				DWORD nType;
				DWORD nSize = MAX_PATH*2*sizeof(wchar_t);
				pszIco = (wchar_t*)calloc(nSize+2,1);
				if (0 != RegQueryValueEx(hkShell, L"Icon", NULL, &nType, (LPBYTE)pszIco, &nSize) || nType != REG_SZ)
					SafeFree(pszIco);
				HKEY hkCmd = NULL;
				if (0 == RegOpenKeyEx(hkShell, L"command", 0, KEY_READ, &hkCmd))
				{
					DWORD nSize = MAX_PATH*8*sizeof(wchar_t);
					pszFull = (wchar_t*)calloc(nSize+2,1);
					if (0 != RegQueryValueEx(hkCmd, NULL, NULL, &nType, (LPBYTE)pszFull, &nSize) || nType != REG_SZ)
					{
						SafeFree(pszIco);
					}
					else
					{
						Switches.StripDupSwitches(pszFull);
					}
					RegCloseKey(hkCmd);
				}
				RegCloseKey(hkShell);
			}
		}
		SafeFree(pszName);
	}

	SetDlgItemText(mh_Dlg, (CB==cbInsideName) ? tInsideConfig : tHereConfig,
		Switches.szConfig.c_str(L""));
	SetDlgItemText(mh_Dlg, (CB==cbInsideName) ? tInsideShell : tHereShell,
		Switches.CreateCommand(szCmd));
	SetDlgItemText(mh_Dlg, (CB==cbInsideName) ? tInsideIcon : tHereIcon,
		pszIco ? pszIco : L"");

	if (CB==cbInsideName)
	{
		SetDlgItemText(mh_Dlg, tInsideSyncDir, Switches.szDirSync.c_str(L""));
		checkDlgButton(mh_Dlg, cbInsideSyncDir, Switches.szDirSync ? BST_CHECKED : BST_UNCHECKED);
	}

	SafeFree(pszFull);
	SafeFree(pszIco);
}
