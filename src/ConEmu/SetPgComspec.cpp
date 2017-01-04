
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

#include "ConEmu.h"
#include "OptionsClass.h"
#include "SetPgComspec.h"
#include "SetPgIntegr.h"
#include "VConGroup.h"

CSetPgComspec::CSetPgComspec()
{
}

CSetPgComspec::~CSetPgComspec()
{
}

LRESULT CSetPgComspec::OnInitDialog(HWND hDlg, bool abInitial)
{
	_ASSERTE((rbComspecAuto+cst_Explicit)==rbComspecExplicit && (rbComspecAuto+cst_Cmd)==rbComspecCmd  && (rbComspecAuto+cst_EnvVar)==rbComspecEnvVar);
	checkRadioButton(hDlg, rbComspecAuto, rbComspecExplicit, rbComspecAuto+gpSet->ComSpec.csType);

	SetDlgItemText(hDlg, tComspecExplicit, gpSet->ComSpec.ComspecExplicit);
	SendDlgItemMessage(hDlg, tComspecExplicit, EM_SETLIMITTEXT, countof(gpSet->ComSpec.ComspecExplicit)-1, 0);

	_ASSERTE((rbComspec_OSbit+csb_SameApp)==rbComspec_AppBit && (rbComspec_OSbit+csb_x32)==rbComspec_x32);
	checkRadioButton(hDlg, rbComspec_OSbit, rbComspec_x32, rbComspec_OSbit+gpSet->ComSpec.csBits);

	checkDlgButton(hDlg, cbComspecUpdateEnv, gpSet->ComSpec.isUpdateEnv ? BST_CHECKED : BST_UNCHECKED);
	enableDlgItem(hDlg, cbComspecUpdateEnv, (gpSet->ComSpec.csType!=cst_EnvVar));

	checkDlgButton(hDlg, cbComspecUncPaths, gpSet->ComSpec.isAllowUncPaths ? BST_CHECKED : BST_UNCHECKED);

	// Cmd.exe output cp
	if (abInitial)
	{
		SendDlgItemMessage(hDlg, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Undefined");
		SendDlgItemMessage(hDlg, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Automatic");
		SendDlgItemMessage(hDlg, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Unicode (/U)");
		SendDlgItemMessage(hDlg, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"OEM (/A)");
	}
	SendDlgItemMessage(hDlg, lbCmdOutputCP, CB_SETCURSEL, gpSet->nCmdOutputCP, 0);

	// Autorun (autoattach) with "cmd.exe" or "tcc.exe"
	ReloadAutorun();

	return 0;
}

INT_PTR CSetPgComspec::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	switch (nCtrlId)
	{
	case lbCmdOutputCP:
	{
		gpSet->nCmdOutputCP = SendDlgItemMessage(hDlg, nCtrlId, CB_GETCURSEL, 0, 0);

		if (gpSet->nCmdOutputCP == -1) gpSet->nCmdOutputCP = 0;

		CVConGroup::OnUpdateFarSettings();
		break;
	}

	default:
		_ASSERTE(FALSE && "ListBox was not processed");
	}

	return 0;
}

void CSetPgComspec::ReloadAutorun()
{
	wchar_t *pszCmd = NULL;

	BOOL bForceNewWnd = isChecked(mh_Dlg, cbCmdAutorunNewWnd);

	HKEY hkDir = NULL;
	if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor", 0, KEY_READ, &hkDir))
	{
		pszCmd = LoadAutorunValue(hkDir, false);
		if (pszCmd && *pszCmd)
		{
			bForceNewWnd = (StrStrI(pszCmd, L"/GHWND=NEW") != NULL);
		}
		RegCloseKey(hkDir);
	}

	SetDlgItemText(mh_Dlg, tCmdAutoAttach, pszCmd ? pszCmd : L"");
	checkDlgButton(mh_Dlg, cbCmdAutorunNewWnd, bForceNewWnd);

	SafeFree(pszCmd);
}

// Load current value of "HKCU\Software\Microsoft\Command Processor" : "AutoRun"
// (bClear==true) - remove from it our "... Cmd_Autorun.cmd ..." part
wchar_t* CSetPgComspec::LoadAutorunValue(HKEY hkCmd, bool bClear)
{
	size_t cchCmdMax = 65535;
	wchar_t *pszCmd = (wchar_t*)malloc(cchCmdMax*sizeof(*pszCmd));
	if (!pszCmd)
	{
		_ASSERTE(pszCmd!=NULL);
		return NULL;
	}

	_ASSERTE(hkCmd!=NULL);
	//HKEY hkCmd = NULL;
	//if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor", 0, KEY_READ, &hkCmd))

	DWORD cbMax = (cchCmdMax-2) * sizeof(*pszCmd);
	if (0 == RegQueryValueEx(hkCmd, L"AutoRun", NULL, NULL, (LPBYTE)pszCmd, &cbMax))
	{
		pszCmd[cbMax>>1] = 0;

		if (bClear && *pszCmd)
		{
			// Просили почистить от "... Cmd_Autorun.cmd ..."
			wchar_t* pszFind = StrStrI(pszCmd, L"\\ConEmu\\Cmd_Autorun.cmd");
			if (pszFind)
			{
				// "... Cmd_Autorun.cmd ..." found, need to find possible start and end of our part ('&' separated)
				wchar_t* pszStart = pszFind;
				while ((pszStart > pszCmd) && (*(pszStart-1) != L'&'))
					pszStart--;

				const wchar_t* pszEnd = wcschr(pszFind, L'&');
				if (!pszEnd)
				{
					pszEnd = pszFind + _tcslen(pszFind);
				}
				else
				{
					while (*pszEnd == L'&')
						pszEnd++;
				}

				// Ok, There are another commands?
				if ((pszStart > pszCmd) || *pszEnd)
				{
					// Possibilities
					if (!*pszEnd)
					{
						// app1.exe && Cmd_Autorun.cmd
						while ((pszStart > pszCmd) && ((*(pszStart-1) == L'&') || (*(pszStart-1) == L' ')))
							pszStart--;
						_ASSERTE(pszStart > pszCmd); // Command to left is empty?
						*pszStart = 0; // just trim
					}
					else
					{
						// app1.exe && Cmd_Autorun.cmd & app2.exe
						// app1.exe & Cmd_Autorun.cmd && app2.exe
						// Cmd_Autorun.cmd & app2.exe
						if (pszStart == pszCmd)
						{
							pszEnd = SkipNonPrintable(pszEnd);
						}
						size_t cchLeft = _tcslen(pszEnd)+1;
						// move command (from right) to the 'Cmd_Autorun.cmd' place
						memmove(pszStart, pszEnd, cchLeft*sizeof(wchar_t));
					}
				}
				else
				{
					// No, we are alone
					*pszCmd = 0;
				}
			}
		}

		// Skip spaces?
		LPCWSTR pszChar = SkipNonPrintable(pszCmd);
		if (!pszChar || !*pszChar)
		{
			*pszCmd = 0;
		}
	}
	else
	{
		*pszCmd = 0;
	}

	// Done
	if (pszCmd && (*pszCmd == 0))
	{
		SafeFree(pszCmd);
	}
	return pszCmd;
}

void CSetPgComspec::RegisterCmdAutorun(bool bEnabled, bool bForced /*= false*/)
{
	BOOL bForceNewWnd = isChecked(mh_Dlg, cbCmdAutorunNewWnd);
		//checkDlgButton(, (StrStrI(pszCmd, L"/GHWND=NEW") != NULL));

	HKEY hkCmd = NULL;
	if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor", 0, KEY_READ|KEY_WRITE, &hkCmd))
	{
		wchar_t* pszCur = NULL;
		wchar_t* pszBuf = NULL;
		LPCWSTR  pszSet = L"";
		wchar_t szCmd[MAX_PATH+128];

		if (bEnabled)
		{
			pszCur = LoadAutorunValue(hkCmd, true);
			pszSet = pszCur;

			_wsprintf(szCmd, SKIPLEN(countof(szCmd)) L"\"%s\\Cmd_Autorun.cmd", gpConEmu->ms_ConEmuBaseDir);
			if (FileExists(szCmd+1))
			{
				wcscat_c(szCmd, bForceNewWnd ? L"\" \"/GHWND=NEW\"" : L"\"");

				if (pszCur == NULL)
				{
					pszSet = szCmd;
				}
				else
				{
					// Current "AutoRun" is not empty, need concatenate
					size_t cchAll = _tcslen(szCmd) + _tcslen(pszCur) + 5;
					pszBuf = (wchar_t*)malloc(cchAll*sizeof(*pszBuf));
					_ASSERTE(pszBuf);
					if (pszBuf)
					{
						_wcscpy_c(pszBuf, cchAll, szCmd);
						_wcscat_c(pszBuf, cchAll, L" & "); // conveyer next command indifferent to %errorlevel%
						_wcscat_c(pszBuf, cchAll, pszCur);
						// Ok, Set
						pszSet = pszBuf;
					}
				}
			}
			else
			{
				MsgBox(szCmd, MB_ICONSTOP, L"File not found", ghOpWnd);

				pszSet = pszCur ? pszCur : L"";
			}
		}
		else
		{
			pszCur = bForced ? NULL : LoadAutorunValue(hkCmd, true);
			pszSet = pszCur ? pszCur : L"";
		}

		DWORD cchLen = _tcslen(pszSet)+1;
		RegSetValueEx(hkCmd, L"AutoRun", 0, REG_SZ, (LPBYTE)pszSet, cchLen*sizeof(wchar_t));

		RegCloseKey(hkCmd);

		if (pszBuf && (pszBuf != pszCur) && (pszBuf != szCmd))
			free(pszBuf);
		SafeFree(pszCur);
	}
}

LRESULT CSetPgComspec::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case tComspecExplicit:
		{
			GetDlgItemText(hDlg, tComspecExplicit, gpSet->ComSpec.ComspecExplicit, countof(gpSet->ComSpec.ComspecExplicit));
			break;
		} //case tComspecExplicit:

	case tCmdAutoAttach:
		break; // Read-Only

	default:
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}
