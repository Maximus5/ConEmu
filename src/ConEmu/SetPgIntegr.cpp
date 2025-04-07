
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

#include "Dark.h"

#define CONEMU_HERE_CMD  L"{cmd} -cur_console:n"
#define CONEMU_HERE_POSH L"{powershell} -cur_console:n"

struct Switch
{
	CmdArg switch_;
	CEStr opt_;

	Switch(const Switch&) = delete;
	Switch(Switch&&) = delete;
	Switch& operator=(const Switch&) = delete;
	Switch& operator=(Switch&&) = delete;

	// Format examples: `-single` or `-dir "..."`
	Switch(wchar_t*&& asSwitch, wchar_t*&& asOpt)
	{
		switch_.Attach(std::move(asSwitch));  // NOLINT(performance-move-const-arg)
		opt_.Attach(std::move(asOpt));
	}

	~Switch()
	{
	}
};

struct SwitchParser
{
	MArray<Switch*> stdSwitches_, ourSwitches_;
	CEStr cmd_, dirSync_, config_;
	bool cmdList_ = false; // `-runlist` was used

	SwitchParser(const SwitchParser&) = delete;
	SwitchParser(SwitchParser&&) = delete;
	SwitchParser& operator=(const SwitchParser&) = delete;
	SwitchParser& operator=(SwitchParser&&) = delete;

	static LPCWSTR* GetSkipSwitches()
	{
		// ReSharper disable once IdentifierTypo
		static LPCWSTR ppszSkipStd[] = {
			L"-dir", L"-config",
			L"-inside", L"-inside:", L"-here",
			L"-multi", L"-NoMulti",
			L"-Single", L"-NoSingle", L"-ReUse",
			L"-NoCascade", L"-DontCascade",
			nullptr};
		return ppszSkipStd;
	}

	SwitchParser()
	{
	}

	void ReleaseVectors()
	{
		Switch* p;
		while (stdSwitches_.pop_back(p))
			SafeDelete(p);
		while (ourSwitches_.pop_back(p))
			SafeDelete(p);
	}

	~SwitchParser()
	{
		ReleaseVectors();
	}

	// ReSharper disable once IdentifierTypo
	static bool IsIgnored(Switch* ps, LPCWSTR* ppszSkip)
	{
		if (!ps)
		{
			return true;
		}

		if (ppszSkip)
		{
			while (*ppszSkip)
			{
				if (ps->switch_.IsSwitch(*ppszSkip))
					return true;
				ppszSkip++;
			}
		}

		return false;
	}

	static bool IsIgnored(Switch* ps, MArray<Switch*>& Skip)
	{
		if (!ps)
		{
			return true;
		}

		for (INT_PTR i = Skip.size()-1; i >= 0; i--)
		{
			if (!ps->switch_.IsSwitch(Skip[i]->switch_))
				continue;

			if (0 != ps->opt_.Compare(Skip[i]->opt_, true))
			{
				// Allow to add switch?
				return false;
			}
			return true;
		}

		return false;
	}

	static Switch* GetNextSwitch(LPCWSTR& rpsz, CmdArg& szArg)
	{
		LPCWSTR psz = rpsz;
		CmdArg szNext;

		if (((psz = NextArg(psz, szNext))) && !szNext.IsPossibleSwitch())
			rpsz = psz;
		else
			szNext.Release();

		szArg.LoadPosFrom(szNext);
		auto* ps = new Switch(szArg.Detach(), szNext.Detach());
		return ps;
	}

	// ReSharper disable once IdentifierTypo
	static Switch* GetNextPair(LPCWSTR& rpsz)
	{
		LPCWSTR psz = rpsz;
		CmdArg szArg;

		if (!((psz = NextArg(psz, szArg))))
		{
			return nullptr;
		}

		// Invalid switch? or first argument (our executable)
		if (!szArg.IsPossibleSwitch())
		{
			rpsz = psz;
			return nullptr;
		}

		rpsz = psz;

		return GetNextSwitch(rpsz, szArg);
	}

	// Parse switches stored in gpConEmu during initialization (AppendExtraArgs)
	// These are, for example, `-lngfile`, `-fontdir`, and so on.
	void ParseStdSwitches()
	{
		_ASSERTE(stdSwitches_.empty());

		CEStr szArg, szNext, lsExtra;

		const LPCWSTR pszExtraArgs = gpConEmu->MakeConEmuStartArgs(lsExtra);
		LPCWSTR psz = pszExtraArgs;
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
			stdSwitches_.push_back(ps);
		}
	}

	// pszFull comes from registry's [HKCR\Directory\shell\...\command]
	// Example:
	//	"C:\Tools\ConEmu.exe" -inside -LoadCfgFile "C:\Tools\ConEmu.xml" -FontDir C:\Tools\ConEmu
	//			-lngfile C:\Tools\ConEmu\ConEmu.l10n -lng ru  -dir "%1" -run {cmd} -cur_console:n
	// Strip switches which match current instance startup arguments
	// No sense to show them (e.g. "-lng ru") in the Integration dialog page
	void LoadCommand(LPCWSTR pszFull)
	{
		cmdList_ = false;
		cmd_ = L"";
		dirSync_ = L"";
		config_ = L"";
		ReleaseVectors();

		CmdArg  szArg, szNext;
		Switch* ps = nullptr;

		// First, parse our extra args (passed to current ConEmu.exe)
		ParseStdSwitches();

		// Now parse new switches (command from registry or from field on Integration page)
		// Drop `-dir "..."` (especially from registry) always!

		LPCWSTR psz = pszFull;
		while ((psz = NextArg(psz, szArg)))
		{
			if (!szArg.IsPossibleSwitch())
				continue;

			if (szArg.OneOfSwitches(L"-inside", L"-here"))
			{
				// Nop
			}
			else if (szArg.IsSwitch(L"-inside:")) // Both "-inside:" and "-inside=" notations are supported
			{
				dirSync_.Set(szArg.Mid(8)); // may be empty!
			}
			else if (szArg.IsSwitch(L"-config"))
			{
				if (!((psz = NextArg(psz, szArg))))
					break;
				config_.Set(szArg);
			}
			else if (szArg.IsSwitch(L"-dir"))
			{
				if (!((psz = NextArg(psz, szArg))))
					break;
				_ASSERTE(lstrcmpi(szArg, L"%1")==0);
			}
			else if (szArg.OneOfSwitches(L"-Single", L"-NoSingle", L"-ReUse"))
			{
				ps = new Switch(szArg.Detach(), nullptr);
				ourSwitches_.push_back(ps);
			}
			else if (szArg.OneOfSwitches(L"-run", L"-cmd", L"-RunList", L"-CmdList"))
			{
				// FIN! LAST SWITCH!
				cmd_.Set(psz);
				cmdList_ = szArg.OneOfSwitches(L"-RunList",L"-CmdList");
				break;
			}
			else if (nullptr != (ps = GetNextSwitch(psz, szArg)))
			{
				if (IsIgnored(ps, GetSkipSwitches())
					|| IsIgnored(ps, stdSwitches_))
				{
					SafeDelete(ps);
				}
				else
				{
					ourSwitches_.push_back(ps);
				}
			}
		}
	}

	// Create the command to show on the Integration settings page
	LPCWSTR CreateCommand(CEStr& rsReady)
	{
		rsReady = L"";

		for (auto* ps : ourSwitches_)
		{
			_ASSERTE(ps && !ps->switch_.IsEmpty());
			const bool bOpt = !ps->opt_.IsEmpty();
			const bool bQuot = (bOpt && IsQuotationNeeded(ps->opt_));
			rsReady.Append(
				ps->switch_,
				bOpt ? L" " : nullptr,
				bQuot ? L"\"" : nullptr,
				bOpt ? ps->opt_.c_str() : nullptr,
				bQuot ? L"\" " : L" ");
		}

		if (!rsReady.IsEmpty() || cmdList_)
		{
			// Add "-run" without command to depict we run ConEmu "as default"
			rsReady.Append(
				cmdList_ ? L"-RunList " : L"-run ", cmd_);
		}
		else
		{
			rsReady.Set(cmd_);
		}

		return rsReady.c_str(L"");
	}
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

	// Возвращает nullptr, если строка пустая
	CEStr pszCurInside = GetDlgItemTextPtr(hDlg, cbInsideName);
	_ASSERTE((pszCurInside==nullptr) || (*pszCurInside!=0));
	CEStr pszCurHere   = GetDlgItemTextPtr(hDlg, cbHereName);
	_ASSERTE((pszCurHere==nullptr) || (*pszCurHere!=0));

	wchar_t szIcon[MAX_PATH+32];
	swprintf_c(szIcon, L"%s,0", gpConEmu->ms_ConEmuExe);

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
		checkDlgButton(hDlg, cbInsideSyncDir, gpConEmu->mp_Inside && gpConEmu->mp_Inside->IsSynchronizeCurDir());
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

	return 0;
}

INT_PTR CSetPgIntegr::PageDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	const INT_PTR iRc = 0;

	switch (messg)
	{
	case WM_NOTIFY:
		{
			// ReSharper disable once IdentifierTypo, CppLocalVariableMayBeConst
			LPNMHDR phdr = reinterpret_cast<LPNMHDR>(lParam);

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

	case WM_CTLCOLORDLG:
		if (gbUseDarkMode)
			return DarkOnCtlColorDlg((HDC)wParam);
		break;

	case WM_CTLCOLORSTATIC:
		if (gbUseDarkMode)
			return DarkOnCtlColorStatic((HWND)lParam, (HDC)wParam);
		break;

	case WM_CTLCOLORBTN:
		if (gbUseDarkMode)
			return DarkOnCtlColorBtn((HDC)wParam);
		break;

	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
		if (gbUseDarkMode)
			return DarkOnCtlColorEditColorListBox((HDC)wParam);
		break;

	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case BN_CLICKED:
			{
				const WORD cb = LOWORD(wParam);
				switch (cb)
				{
				case cbInsideSyncDir:
					if (gpConEmu->mp_Inside)
					{
						gpConEmu->mp_Inside->SetSynchronizeCurDir(isChecked2(hDlg, cb));
					}
					break;
				case bInsideRegister:
				case bInsideUnregister:
					ShellIntegration(hDlg, ShellIntegrType::Inside, cb==bInsideRegister);
					ReloadHereList();
					if (cb==bInsideUnregister)
						FillHereValues(cbInsideName);
					break;
				case bHereRegister:
				case bHereUnregister:
					ShellIntegration(hDlg, ShellIntegrType::Here, cb==bHereRegister);
					ReloadHereList();
					if (cb==bHereUnregister)
						FillHereValues(cbHereName);
					break;
				default:
					break;
				}
			}
			break; // BN_CLICKED
		case EN_CHANGE:
			{
				const WORD eb = LOWORD(wParam);
				switch (eb)  // NOLINT(hicpp-multiway-paths-covered)
				{
				case tInsideSyncDir:
					if (gpConEmu->mp_Inside)
					{
						const CEStr format(GetDlgItemTextPtr(hDlg, tInsideSyncDir));
						gpConEmu->mp_Inside->SetInsideSynchronizeCurDir(format);
					}
					break;
				default:
					break;
				}
			}
			break; // EN_CHANGE
		case CBN_SELCHANGE:
			{
				const WORD cb = LOWORD(wParam);
				switch (cb)
				{
				case cbInsideName:
				case cbHereName:
					if (!mb_SkipSelChange)
					{
						FillHereValues(cb);
					}
					break;
				default:
					break;
				}
			}
			break; // CBN_SELCHANGE
		} // switch (HIWORD(wParam))
		break; // WM_COMMAND

	default:
		break;
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

	const bool isExtendedItem = isPressed(VK_SHIFT);

	#ifdef _DEBUG
	const CmdArg szMode(asMode);
	_ASSERTE(szMode.IsSwitch(L"-here") || szMode.IsSwitch(L"-inside") || szMode.IsSwitch(L"-inside:"));
	#endif

	CEStr lsExtra;
	const LPCWSTR pszExtraArgs = gpConEmu->MakeConEmuStartArgs(lsExtra, asConfig);

	const size_t cchMax = _tcslen(gpConEmu->ms_ConEmuExe)
		+ (asMode ? (_tcslen(asMode) + 3) : 0)
		+ (pszExtraArgs ? (_tcslen(pszExtraArgs) + 1) : 0)
		+ _tcslen(asCmd) + 32;
	wchar_t* pszCmd = static_cast<wchar_t*>(malloc(cchMax*sizeof(*pszCmd)));
	if (!pszCmd)
		return;

	// Allow to run ConEmu as default, e.g. "ConEmu.exe -here -nosingle"
	// where the "-nosingle" comes via asCmd
	// ref: https://twitter.com/soekali/status/951338035374784512
	bool bAddRunSwitch = false;
	if (*asCmd == L'/' || *asCmd == L'-')
	{
		CmdArg lsArg;
		LPCWSTR pszTemp = asCmd;
		if ((pszTemp = NextArg(pszTemp, lsArg)))
		{
			bAddRunSwitch = !lsArg.IsPossibleSwitch();
		}
	}
	else
	{
		bAddRunSwitch = true;
	}

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
	const bool bHasLibraries = IsWin7();

	for (int i = 1; i <= 6; i++)
	{
		_wcscpy_c(pszCmd, cchMax, L"\"");
		_wcscat_c(pszCmd, cchMax, gpConEmu->ms_ConEmuExe);
		_wcscat_c(pszCmd, cchMax, L"\" ");

		// `-here`, `-inside` or `-inside:cd ...`
		if (asMode && *asMode)
		{
			const bool bQ = IsQuotationNeeded(asMode);
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

		LPCWSTR pszRoot = nullptr;
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
		default:
			break;
		}

		if (bAddRunSwitch)
			_wcscat_c(pszCmd, cchMax, L"-run ");
		_wcscat_c(pszCmd, cchMax, asCmd);

		HKEY hkRoot;
		if (0 == RegCreateKeyEx(HKEY_CURRENT_USER, pszRoot, 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hkRoot, nullptr))
		{
			HKEY hkConEmu;
			if (0 == RegCreateKeyEx(hkRoot, asName, 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hkConEmu, nullptr))
			{
				// gh-1395: Force Explorer to use hotkey if specified
				RegSetValueEx(hkConEmu, nullptr, 0, REG_SZ, LPBYTE(asName), (lstrlen(asName)+1)*sizeof(*asName));

				// The icon, if it was set
				if (asIcon)
					RegSetValueEx(hkConEmu, L"Icon", 0, REG_SZ, LPBYTE(asIcon), (lstrlen(asIcon)+1)*sizeof(*asIcon));
				else
					RegDeleteValue(hkConEmu, L"Icon");

				// The command
				HKEY hkCmd;
				if (0 == RegCreateKeyEx(hkConEmu, L"command", 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hkCmd, nullptr))
				{
					if (0 == RegSetValueEx(hkCmd, nullptr, 0, REG_SZ, reinterpret_cast<LPBYTE>(pszCmd), (lstrlen(pszCmd)+1)*sizeof(*pszCmd)))
						iSucceeded++;
					RegCloseKey(hkCmd);
				}

				// Extended item - Explorer shows them when 'Shift' is pressed
				wchar_t szDummy[2] = L"";
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
			if (0 != RegEnumKeyEx(hkDir, i, szName, &cchMax, nullptr, nullptr, nullptr, nullptr))
				break;
			wchar_t* pszSlash = szName + _tcslen(szName);
			wcscat_c(szName, L"\\command");
			HKEY hkCmd = nullptr;
			if (0 == RegOpenKeyEx(hkDir, szName, 0, KEY_READ, &hkCmd))
			{
				DWORD cbMax = sizeof(szCmd)-2;
				if (0 == RegQueryValueEx(hkCmd, nullptr, nullptr, nullptr, reinterpret_cast<LPBYTE>(szCmd), &cbMax))
				{
					szCmd[cbMax>>1] = 0;
					*pszSlash = 0;
					//LPCWSTR pszInside = StrStrI(szCmd, L"-inside");
					const LPCWSTR pszConEmu = StrStrI(szCmd, L"conemu");
					if (pszConEmu)
						lsNames.push_back(lstrdup(szName).Detach());
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

void CSetPgIntegr::ShellIntegration(HWND hDlg, const ShellIntegrType iMode, const bool bEnabled)
{
	switch (iMode)
	{
	case ShellIntegrType::Inside:
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
					const int optLen = lstrlen(szOpt); _ASSERTE(optLen==8);
					const auto cmdLen = GetDlgItemText(hDlg, tInsideSyncDir, szOpt + optLen, countof(szOpt) - optLen);
					if (cmdLen == 0)
					{
						wcscpy_s(szOpt + optLen, countof(szOpt) - optLen, INSIDE_DEFAULT_SYNC_DIR_CMD);
					}
				}

				//swprintf_c(szIcon, L"%s,0", gpConEmu->ms_ConEmuExe);
				GetDlgItemText(hDlg, tInsideIcon, szIcon, countof(szIcon));
				RegisterShell(szName, szOpt[0] ? szOpt : L"-inside", SkipNonPrintable(szConfig), SkipNonPrintable(szShell), szIcon);
			}
			else if (*szName)
			{
				UnregisterShell(szName);
			}
		}
		break;

	case ShellIntegrType::Here:
		{
			wchar_t szName[MAX_PATH] = {};
			GetDlgItemText(hDlg, cbHereName, szName, countof(szName));
			if (bEnabled)
			{
				wchar_t szConfig[MAX_PATH] = {}, szShell[MAX_PATH] = {}, szIcon[MAX_PATH+16];
				GetDlgItemText(hDlg, tHereConfig, szConfig, countof(szConfig));
				GetDlgItemText(hDlg, tHereShell, szShell, countof(szShell));
				//swprintf_c(szIcon, L"%s,0", gpConEmu->ms_ConEmuExe);
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

bool CSetPgIntegr::ReloadHereList(int* pnHere /*= nullptr*/, int* pnInside /*= nullptr*/)
{
	if (pnHere) *pnHere = 0;
	if (pnInside) *pnInside = 0;

	HKEY hkDir = nullptr;
	const size_t cchCmdMax = 65535;
	wchar_t* pszCmd = static_cast<wchar_t*>(calloc(cchCmdMax, sizeof(*pszCmd)));
	if (!pszCmd)
		return false;

	int iTotalCount = 0;

	// Возвращает nullptr, если строка пустая
	CEStr pszCurInside = GetDlgItemTextPtr(mh_Dlg, cbInsideName);
	_ASSERTE(pszCurInside.IsNull() || !pszCurInside.IsEmpty());
	CEStr pszCurHere = GetDlgItemTextPtr(mh_Dlg, cbHereName);
	_ASSERTE(pszCurHere.IsNull() || !pszCurHere.IsEmpty());

	MSetter lSkip(&mb_SkipSelChange);

	SendDlgItemMessage(mh_Dlg, cbInsideName, CB_RESETCONTENT, 0, 0);
	SendDlgItemMessage(mh_Dlg, cbHereName, CB_RESETCONTENT, 0, 0);

	if (0 == RegOpenKeyEx(HKEY_CLASSES_ROOT, L"Directory\\shell", 0, KEY_READ, &hkDir))
	{
		for (DWORD i = 0; i < 512; i++)
		{
			wchar_t szName[MAX_PATH+32] = {};
			DWORD cchMax = countof(szName) - 32;
			if (0 != RegEnumKeyEx(hkDir, i, szName, &cchMax, nullptr, nullptr, nullptr, nullptr))
				break;
			wchar_t* pszSlash = szName + _tcslen(szName);
			wcscat_c(szName, L"\\command");
			HKEY hkCmd = nullptr;
			if (0 == RegOpenKeyEx(hkDir, szName, 0, KEY_READ, &hkCmd))
			{
				DWORD cbMax = (cchCmdMax-2) * sizeof(*pszCmd);
				if (0 == RegQueryValueEx(hkCmd, nullptr, nullptr, nullptr, reinterpret_cast<LPBYTE>(pszCmd), &cbMax))
				{
					pszCmd[cbMax>>1] = 0;
					*pszSlash = 0;

					bool bHasInside = false, bConEmu = false;
					LPCWSTR pszTemp = pszCmd; CmdArg szArg;
					if ((pszTemp = NextArg(pszTemp, szArg)))
					{
						if ((bConEmu = IsConEmuGui(szArg)))
						{
							while ((pszTemp = NextArg(pszTemp, szArg)))
							{
								if (szArg.OneOfSwitches(L"-inside", L"-inside:"))
								{
									bHasInside = true;
									break;
								}
								if (szArg.OneOfSwitches(L"-run",L"-RunList",L"-cmd",L"-CmdList"))
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

						const UINT nListID = bHasInside ? cbInsideName : cbHereName;
						SendDlgItemMessage(mh_Dlg, nListID, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szName));

						if ((bHasInside ? pszCurInside.IsEmpty() : pszCurHere.IsEmpty()))
						{
							if (bHasInside)
								pszCurInside.Set(szName);
							else
								pszCurHere.Set(szName);
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

	free(pszCmd);

	return (iTotalCount > 0);
}

void CSetPgIntegr::FillHereValues(const WORD cb) const
{
	CEStr szCmd;
	SwitchParser Switches;
	wchar_t *pszIco = nullptr, *pszFull = nullptr;

	const INT_PTR iSel = SendDlgItemMessage(mh_Dlg, cb, CB_GETCURSEL, 0,0);
	if (iSel >= 0)
	{
		const INT_PTR iLen = SendDlgItemMessage(mh_Dlg, cb, CB_GETLBTEXTLEN, iSel, 0);
		const size_t cchMax = iLen+128;
		wchar_t* pszName = static_cast<wchar_t*>(calloc(cchMax, sizeof(*pszName)));
		if ((iLen > 0) && pszName)
		{
			_wcscpy_c(pszName, cchMax, L"Directory\\shell\\");
			SendDlgItemMessage(mh_Dlg, cb, CB_GETLBTEXT, iSel, reinterpret_cast<LPARAM>(pszName + _tcslen(pszName)));

			HKEY hkShell = nullptr;
			if (0 == RegOpenKeyEx(HKEY_CLASSES_ROOT, pszName, 0, KEY_READ, &hkShell))
			{
				DWORD nType;
				DWORD nSize = MAX_PATH * 2 * sizeof(wchar_t);
				pszIco = static_cast<wchar_t*>(calloc(nSize + sizeof(wchar_t), 1));
				if (0 != RegQueryValueEx(hkShell, L"Icon", nullptr, &nType, reinterpret_cast<LPBYTE>(pszIco), &nSize) || nType != REG_SZ)
					SafeFree(pszIco);

				HKEY hkCmd = nullptr;
				if (0 == RegOpenKeyEx(hkShell, L"command", 0, KEY_READ, &hkCmd))
				{
					nSize = MAX_PATH * 8 * sizeof(wchar_t);
					pszFull = static_cast<wchar_t*>(calloc(nSize + sizeof(wchar_t), 1));
					if (0 != RegQueryValueEx(hkCmd, nullptr, nullptr, &nType, reinterpret_cast<LPBYTE>(pszFull), &nSize) || nType != REG_SZ)
					{
						SafeFree(pszIco);
					}
					else
					{
						Switches.LoadCommand(pszFull);
					}
					RegCloseKey(hkCmd);
				}
				RegCloseKey(hkShell);
			}
		}
		SafeFree(pszName);
	}

	SetDlgItemText(mh_Dlg, (cb==cbInsideName) ? tInsideConfig : tHereConfig,
		Switches.config_.c_str(L""));
	SetDlgItemText(mh_Dlg, (cb==cbInsideName) ? tInsideShell : tHereShell,
		Switches.CreateCommand(szCmd));
	SetDlgItemText(mh_Dlg, (cb==cbInsideName) ? tInsideIcon : tHereIcon,
		pszIco ? pszIco : L"");

	if (cb==cbInsideName)
	{
		const wchar_t* syncCmd = Switches.dirSync_.c_str(L"");
		if (wcscmp(syncCmd, INSIDE_DEFAULT_SYNC_DIR_CMD) == 0)
			syncCmd = L"";
		SetDlgItemText(mh_Dlg, tInsideSyncDir, syncCmd);
		checkDlgButton(mh_Dlg, cbInsideSyncDir, Switches.dirSync_ ? BST_CHECKED : BST_UNCHECKED);
	}

	SafeFree(pszFull);
	SafeFree(pszIco);
}
