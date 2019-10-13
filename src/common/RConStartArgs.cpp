
/*
Copyright (c) 2009-present Maximus5
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

#include "defines.h"
#include "CmdLine.h"
#include "Common.h"
#include "EnvVar.h"
#include "MAssert.h"
#include "MStrDup.h"
#include "MStrSafe.h"
#include "Memory.h"
#include "RConStartArgs.h"
#include "WObjects.h"

#define DEBUGSTRPARSE(s) DEBUGSTR(s)

// Restricted in ConEmuHk!
#ifdef MCHKHEAP
#undef MCHKHEAP
#define MCHKHEAP PRAGMA_ERROR("Restricted in ConEmuHk")
#endif

#if defined(__GNUC__) && !defined(__MINGW64_VERSION_MAJOR)
#define SecureZeroMemory(p,s) memset(p,0,s)
#endif


void RConStartArgs::CleanPermissions()
{
	SafeFree(pszUserName);
	SafeFree(pszDomain);
	if (szUserPassword[0]) SecureZeroMemory(szUserPassword, sizeof(szUserPassword));

	RunAsAdministrator = crb_Undefined;
	RunAsSystem = crb_Undefined;
	RunAsRestricted = crb_Undefined;
	RunAsNetOnly = crb_Undefined;
	UseEmptyPassword = crb_Undefined;
	ForceUserDialog = crb_Undefined;
}

RConStartArgs::~RConStartArgs()
{
	SafeFree(pszSpecialCmd); // именно SafeFree
	SafeFree(pszStartupDir); // именно SafeFree
	SafeFree(pszAddGuiArg);
	SafeFree(pszRenameTab);
	SafeFree(pszIconFile);
	SafeFree(pszPalette);
	SafeFree(pszWallpaper);
	SafeFree(pszMntRoot);
	SafeFree(pszAnsiLog);

	CleanPermissions();
}


void RConStartArgs::AppendServerArgs(wchar_t* rsServerCmdLine, INT_PTR cchMax)
{
	switch (eConfirmation)
	{
	case eConfAlways:
		_wcscat_c(rsServerCmdLine, cchMax, L" /CONFIRM"); break;
	case eConfEmpty:
		_wcscat_c(rsServerCmdLine, cchMax, L" /ECONFIRM"); break;
	case eConfHalt:
		_wcscat_c(rsServerCmdLine, cchMax, L" /CONFHALT"); break;
	case eConfNever:
		_wcscat_c(rsServerCmdLine, cchMax, L" /NOCONFIRM"); break;
	case eConfDefault:
		break; // Don't add anything
	}

	if (InjectsDisable == crb_On)
		_wcscat_c(rsServerCmdLine, cchMax, L" /NOINJECT");
}


// Returns ">0" - when changes was made
//  0 - no changes
// -1 - error
// bForceCurConsole==true, если разбор параметров идет 
//   при запуске Tasks из GUI
int RConStartArgs::ProcessNewConArg(bool bForceCurConsole /*= false*/)
{
	NewConsole = crb_Undefined;

	if (!pszSpecialCmd || !*pszSpecialCmd)
	{
		_ASSERTE(pszSpecialCmd && *pszSpecialCmd);
		return -1;
	}

	int nChanges = 0;

	// 140219 - Stop processing if found any of these: ConEmu[.exe], ConEmu64[.exe], ConEmuC[.exe], ConEmuC64[.exe]
	LPCWSTR pszStopAt = NULL;
	{
		LPCWSTR pszTemp = pszSpecialCmd;
		LPCWSTR pszSave = pszSpecialCmd;
		LPCWSTR pszName;
		CmdArg szExe;
		LPCWSTR pszWords[] = {
			L"ConEmu", L"ConEmu.exe", L"ConEmu64", L"ConEmu64.exe",
			L"ConEmuC", L"ConEmuC.exe", L"ConEmuC64", L"ConEmuC64.exe",
			L"ConEmuPortable.exe", L"ConEmuPortable",
			L"DosKey", L"DosKey.exe",
			NULL};
		while (!pszStopAt && (pszTemp = NextArg(pszTemp, szExe)))
		{
			if (szExe.ms_Val[0] != L'-')
			{
				pszName = PointToName(szExe);
				for (size_t i = 0; pszWords[i]; i++)
				{
					if (lstrcmpi(pszName, pszWords[i]) == 0)
					{
						pszStopAt = pszSave;
						break;
					}
				}
			}
			pszSave = pszTemp;
		}
	}


	// 111211 - здесь может быть передан "-new_console:..."
	LPCWSTR pszNewCon = L"-new_console";
	// 120108 - или "-cur_console:..." для уточнения параметров запуска команд (из фара например)
	LPCWSTR pszCurCon = L"-cur_console";
	int nNewConLen = lstrlen(pszNewCon);
	_ASSERTE(lstrlen(pszCurCon)==nNewConLen);

	wchar_t* pszFrom = pszSpecialCmd;

	bool bStop = false;
	while (!bStop)
	{
		wchar_t* pszSwitch = wcschr(pszFrom, L'-');
		if (!pszSwitch)
			break;

		// Pre-validation (conditions to skip this switch)
		bool bValid =
			// -new_... or -cur_...
			((pszSwitch[1] == L'n') || (pszSwitch[1] == L'c'))
			&& (
				// If it is started from pszFrom - no need to check previous symbols
				(pszSwitch == pszFrom)
				// Skip parts like: \"-new...\"   or   `-new_...`
				|| (*(pszSwitch-1) == L' ')
				// If prev symbol was double-quote
				|| ((*(pszSwitch-1) == L'"')
					// and there is a space before double-quote
					&& (((pszSwitch-2) < pszFrom) || (*(pszSwitch-2) == L' '))
			));
		// Skip unsupported switches
		if (!bValid)
		{
			// Not ours
			pszSwitch = wcschr(pszSwitch+1, L' ');
			if (!pszSwitch)
				break;
			pszFrom = pszSwitch;
			continue;
		}

		// Continue checks
		wchar_t* pszFindNew = NULL;
		wchar_t* pszFind = NULL;
		wchar_t szTest[12]; lstrcpyn(szTest, pszSwitch+1, countof(szTest));

		if (lstrcmp(szTest, L"new_console") == 0)
			pszFindNew = pszFind = pszSwitch;
		else if (lstrcmp(szTest, L"cur_console") == 0)
			pszFind = pszSwitch;
		else
		{
			// Not ours
			pszSwitch = wcschr(pszSwitch+1, L' ');
			if (!pszSwitch)
				break;
			pszFrom = pszSwitch;
			continue;
		}

		if (!pszFind)
			break;
		if (pszStopAt && (pszFind >= pszStopAt))
			break;

		// Validate trailing char (separators, ':' subswitch delimiter, or EOL)
		_ASSERTE(pszFind >= pszSpecialCmd);
		if ((pszFind[nNewConLen] != L' ') && (pszFind[nNewConLen] != L':')
			&& (pszFind[nNewConLen] != L'"') && (pszFind[nNewConLen] != 0))
		{
			// Not ours
			pszFrom = pszFind+nNewConLen;
		}
		else
		{
			if (pszFindNew)
				NewConsole = crb_On;

			const int iQuotStart = (*(pszFind-1) == L'"') ? 1 : 0;
			int iQuotCount = iQuotStart;
			const wchar_t* pszEnd = pszFind+nNewConLen;

			if (*pszEnd == L'"')
			{
				pszEnd++;
			}
			else if (*pszEnd != L':')
			{
				// End of switch is expected
				_ASSERTE(*pszEnd == L' ' || *pszEnd == 0);
			}
			else
			{
				if (*pszEnd == L':')
				{
					pszEnd++;
				}
				else
				{
					_ASSERTE(*pszEnd == L':');
				}

				// Find the end of argument
				const wchar_t* pszArgEnd = pszEnd;
				bool lbLocalQuot = false;
				while (*pszArgEnd)
				{
					switch (*pszArgEnd)
					{
					case L'^':
						pszArgEnd++; // Skip control char, goto escaped char
						break;
					case L'"':
						if (*(pszArgEnd+1) == L'"')
						{
							pszArgEnd += 2; // Skip qoubled qouble-quotes
							continue;
						}
						if (!iQuotStart)
						{
							if (!lbLocalQuot && (*(pszArgEnd-1) == L':'))
							{
								lbLocalQuot = true;
								pszArgEnd++;
								continue;
							}
							if (lbLocalQuot)
							{
								if (*(pszArgEnd+1) != L':')
									goto EndFound;
								lbLocalQuot = false;
								pszArgEnd += 2;
								continue;
							}
						}
						goto EndFound;
					case L' ':
						if (!iQuotStart && !lbLocalQuot)
							goto EndFound;
						break;
					case 0:
						goto EndFound;
					}

					pszArgEnd++;
				}
				EndFound:

				// Process subswitches -new_console:xxx
				bool lbReady = false;
				while (!lbReady && *pszEnd)
				{
					_ASSERTE(pszEnd <= pszArgEnd);
					wchar_t cOpt = *(pszEnd++);

					switch (cOpt)
					{
					case L'"':
						// Assert was happened, for example, with: "/C \"ConEmu:run:Far.exe  -new_console:\""
						if (!iQuotCount && (pszEnd < pszArgEnd))
						{
							// For example: ""C:\Windows\system32\cmd.exe" /C "sign "FileZilla server.exe" FzGSS.dll -new_console:a""
							_ASSERTE(*(pszEnd-1)==L'"' && *(pszEnd)==L'"' && *(pszEnd+1)==0);
							pszEnd--;
						}
						else
						{
							// False assertion occurred for:
							//   `/C "C:\Temp\run_pause.cmd "-cur_console:o""`
							// _ASSERTE((pszEnd > pszArgEnd) && "Wrong quotation usage in -new_console?");
						}
						lbReady = true;
						break;
					case L' ':
					case 0:
						lbReady = true;
						break;

					case L':':
						// Just skip ':'. Delimiter between switches: -new_console:c:b:a
						// Revert stored value to lbQuot. We need to "cut" last double quote in the first two cases
						// cmd -cur_console:d:"C:\users":t:"My title" "-cur_console:C:C:\cmd.ico" -cur_console:P:"<PowerShell>":a /k ver
						iQuotCount = iQuotStart;
						break;

					case L'b':
						// b - background, не активировать таб
						BackgroundTab = crb_On; ForegroungTab = crb_Off;
						break;
					case L'f':
						// f - foreground, активировать таб (аналог ">" в Tasks)
						ForegroungTab = crb_On; BackgroundTab = crb_Off;
						break;

					case L'z':
						// z - don't use "Default terminal" feature
						NoDefaultTerm = crb_On;
						break;

					case L'a':
						// a - RunAs shell verb (as admin on Vista+, login/password in WinXP-)
						RunAsAdministrator = crb_On;
						break;

					case L'A':
						// A - Run console as System account
						RunAsSystem = crb_On;
						break;

					case L'e':
						// e - Run console wih NetOnly flag
						RunAsNetOnly = crb_On;
						break;

					case L'r':
						// r - run as restricted user
						RunAsRestricted = crb_On;
						break;

					case L'o':
						// o - disable "Long output" for next command (Far Manager)
						LongOutputDisable = crb_On;
						break;

					case L'w':
						// w[1] - enable "Overwrite" mode in console prompt
						// w0  - disable "Overwrite" mode in console prompt
						// This will affect only default ReadConsole API function
						if (isDigit(*pszEnd))
						{
							switch (*(pszEnd++))
							{
							case 0:
								OverwriteMode = crb_Off;
								break;
							default:
								OverwriteMode = crb_On;
							}
						}
						else
						{
							OverwriteMode = crb_On;
						}
						break;

					case L'p':
						if (isDigit(*pszEnd))
						{
							wchar_t* pszDigits = NULL;
							nPTY = wcstoul(pszEnd, &pszDigits, 10);
							if (pszDigits)
								pszEnd = pszDigits;
						}
						else
						{
							nPTY = pty_Default; // xterm mode
						}
						break;

					case L'i':
						// i - don't inject ConEmuHk into the starting application
						InjectsDisable = crb_On;
						break;

					case L'N':
						// N - Force new ConEmu window with Default terminal
						ForceNewWindow = crb_On;
						break;

					case 'R':
						// R - Force CheckHookServer (GitShowBranch.cmd, actually used with -cur_console)
						ForceHooksServer = crb_On;
						break;

					case L'h':
						// "h0" - disable backscroll, "h9999" - set backscroll 9999 lines (max 32766 lines)
						{
							BufHeight = crb_On;
							if (isDigit(*pszEnd))
							{
								wchar_t* pszDigits = NULL;
								nBufHeight = wcstoul(pszEnd, &pszDigits, 10);
								if (pszDigits)
									pszEnd = pszDigits;
							}
							else
							{
								nBufHeight = 0;
							}
						} // L'h':
						break;

					case L'n':
						// n - disable 'Press Enter or Esc to close console' confirmation
						eConfirmation = eConfNever;
						break;

					case L'c':
						if (!isDigit(*pszEnd))
						{
							// c - force enable 'Press Enter or Esc to close console' confirmation
							eConfirmation = eConfAlways;
						}
						else
						{
							switch (*(pszEnd++))
							{
							case L'0':
								// c0 - force wait for Enter or Esc without print message
								eConfirmation = eConfEmpty;
								break;
							case L'1':
								// c1 - don't close console at all (only via ConEmu interface / cross click)
								eConfirmation = eConfHalt;
								break;
							default:
								_ASSERTE(FALSE && "Unsupported option");
								eConfirmation = eConfAlways;
							}
						}
						break;

					case L'x':
						// x - Force using dosbox for .bat files
						ForceDosBox = crb_On;
						break;

					case L'I':
						// I - tell GuiMacro to execute new command inheriting active process state. This is only usage ATM.
						ForceInherit = crb_On;
						break;

					// "Long" code blocks below: 'd', 'u', 's' and so on (in future)

					case L's':
						// s[<SplitTab>T][<Percents>](H|V)
						// Example: "s3T30H" - split 3-d console. New pane will be created to the right, using 30% 3-d console width
						{
							SplitType newSplit = eSplitNone;
							UINT nTab = 0 /*active*/, nValue = /*half by default*/DefaultSplitValue/10;
							bool bDisableSplit = false;
							while (*pszEnd)
							{
								if (isDigit(*pszEnd))
								{
									wchar_t* pszDigits = NULL;
									UINT n = wcstoul(pszEnd, &pszDigits, 10);
									if (!pszDigits)
										break;
									pszEnd = pszDigits;
									if (*pszDigits == L'T')
									{
										nTab = n;
									}
									else if ((*pszDigits == L'H') || (*pszDigits == L'V'))
									{
										nValue = n;
										newSplit = (*pszDigits == L'H') ? eSplitHorz : eSplitVert;
									}
									else
									{
										break;
									}
									pszEnd++;
								}
								else if (*pszEnd == L'T')
								{
									nTab = 0;
									pszEnd++;
								}
								else if ((*pszEnd == L'H') || (*pszEnd == L'V'))
								{
									nValue = DefaultSplitValue/10;
									newSplit = (*pszEnd == L'H') ? eSplitHorz : eSplitVert;
									pszEnd++;
								}
								else if (*pszEnd == L'N')
								{
									bDisableSplit = true;
									pszEnd++;
									break;
								}
								else
								{
									break;
								}
							}

							if (eSplit != eSplitNone)
							{
								DEBUGSTRPARSE(L"Split option was skipped because of it's already set");
							}
							else if (bDisableSplit)
							{
								eSplit = eSplitNone; nSplitValue = DefaultSplitValue; nSplitPane = 0;
							}
							else
							{
								eSplit = newSplit ? newSplit : eSplitHorz;
								// User set the size of NEW part
								nSplitValue = 1000 - std::max<UINT>(1, std::min<UINT>(nValue*10, 999)); // percents
								_ASSERTE(nSplitValue>=1 && nSplitValue<1000);
								nSplitPane = nTab;
							}
						} // L's'
						break;



					// Following options (except of single 'u') must be placed on the end of "-new_console:..."
					// If one needs more that one option - use several "-new_console:..." switches

					case L'd':
						// d:<StartupDir>. MUST be last option
					case L't':
						// t:<TabName>. MUST be last option
					case L'u':
						// u - ConEmu choose user dialog (may be specified in the middle, if it is without ':' - user or pwd)
						// u:<user> - ConEmu choose user dialog with prefilled user field. MUST be last option
						// u:<user>:<pwd> - specify user/pwd in args. MUST be last option
					case L'C':
						// C:<IconFile>. MUST be last option
					case L'P':
						// P:<Palette>. MUST be last option
					case L'W':
						// W:<Wallpaper>. MUST be last option
					case L'L':
						// L:<AnsiLogDir>. MUST be last option
					case L'm':
						// m:<mntroot>. MUST be last option
						{
							if (cOpt == L'u')
							{
								// Show choose user dialog (may be specified in the middle, if it is without ':' - user or pwd)
								SafeFree(pszUserName);
								SafeFree(pszDomain);
								if (szUserPassword[0]) SecureZeroMemory(szUserPassword, sizeof(szUserPassword));
							}


							if (*pszEnd == L':')
							{
								pszEnd++;
							}
							else
							{
								if (cOpt == L'u')
								{
									ForceUserDialog = crb_On;
									break;
								}
							}

							const wchar_t* pszTab = pszEnd;
							// we need to find end of argument
							pszEnd = pszArgEnd;
							// temp buffer
							wchar_t* lpszTemp = NULL;

							wchar_t** pptr = NULL;
							switch (cOpt)
							{
							case L'd': pptr = &pszStartupDir; break;
							case L't': pptr = &pszRenameTab; break;
							case L'u': pptr = &lpszTemp; break;
							case L'C': pptr = &pszIconFile; break;
							case L'P': pptr = &pszPalette; break;
							case L'W': pptr = &pszWallpaper; break;
							case L'm': pptr = &pszMntRoot; break;
							case L'L': pptr = &pszAnsiLog; break;
							// don't forget to add the same `case` to the `switch` above!
							default:
								_ASSERTE(FALSE && "Unprocessed option");
								return -1;
							}

							if (pszEnd > pszTab)
							{
								size_t cchLen = pszEnd - pszTab;
								SafeFree(*pptr);
								*pptr = (wchar_t*)malloc((cchLen+1)*sizeof(**pptr));
								if (*pptr)
								{
									// We need to process escape sequences ("^>" -> ">", "^&" -> "&", etc.)
									//wmemmove(*pptr, pszTab, cchLen);
									wchar_t* pD = *pptr;
									const wchar_t* pS = pszTab;

									if (iQuotStart)
									{
										lbLocalQuot = false;
									}
									else if (*pS == L'"' && ((*(pS+1) != L'"')
											|| (*(pS+1) == L'"' && (!*(pS+2) || isSpace(*(pS+2)) || *(pS+2) == L':'))))
									{
										// Remember, that last processed switch was local-quoted
										++iQuotCount;
										// This item is local quoted. Example: -new_console:t:"My title"
										lbLocalQuot = true;
										pS++;
									}

									// There is enough room allocated
									while (pS < pszEnd)
									{
										if ((*pS == L'^') && ((pS + 1) < pszEnd))
										{
											pS++; // Skip control char, goto escaped char
										}
										else if (*pS == L'"')
										{
											if (((pS + 1) < pszEnd) && (*(pS+1) == L'"'))
											{
												pS++; // Skip qoubled qouble quote
											}
											else if (lbLocalQuot)
											{
												++iQuotCount;
												pszEnd = (pS+1);
												_ASSERTE(*pszEnd==L':' || *pszEnd==L' ' || *pszEnd==0);
												break; // End of local quoted argument: -new_console:d:"C:\User\super user":t:"My title"
											}
										}

										*(pD++) = *(pS++);
									}
									// Terminate with '\0'
									_ASSERTE(pD <= ((*pptr)+cchLen));
									*pD = 0;
								}
								// Additional processing
								switch (cOpt)
								{
								case L'd':
									// For example, "%USERPROFILE%"
									// TODO("А надо ли разворачивать их тут? Наверное при запуске под другим юзером некорректно? Хотя... все равно до переменных не доберемся");
									if (wcschr(pszStartupDir, L'%'))
									{
										wchar_t* pszExpand = NULL;
										if (((pszExpand = ExpandEnvStr(pszStartupDir)) != NULL))
										{
											SafeFree(pszStartupDir);
											pszStartupDir = pszExpand;
										}
									}
									break;
								case L'u':
									if (lpszTemp)
									{
										// Split in form:
										// [Domain\]UserName[:Password]
										wchar_t* pszPwd = wcschr(lpszTemp, L':');
										if (pszPwd)
										{
											// Password was specified, dialog prompt is not required
											ForceUserDialog = crb_Off;
											*pszPwd = 0;
											int nPwdLen = lstrlen(pszPwd+1);
											lstrcpyn(szUserPassword, pszPwd+1, countof(szUserPassword));
											if (nPwdLen > 0)
												SecureZeroMemory(pszPwd+1, nPwdLen);
											UseEmptyPassword = (nPwdLen == 0) ? crb_On : crb_Off;
										}
										else
										{
											// Password was NOT specified, dialog prompt IS required
											ForceUserDialog = crb_On;
											UseEmptyPassword = crb_Off;
										}
										wchar_t* pszSlash = wcschr(lpszTemp, L'\\');
										if (pszSlash)
										{
											*pszSlash = 0;
											pszDomain = lstrdup(lpszTemp);
											pszUserName = lstrdup(pszSlash+1);
										}
										else
										{
											pszUserName = lstrdup(lpszTemp);
										}
									}
									break;
								}
							}
							else
							{
								SafeFree(*pptr);
							}
							SafeFree(lpszTemp);
						} // L't':
						break;

					}
				}
			}

			if (pszEnd > pszFind)
			{
				bool bTrimTrailingSpace = false;

				// pszEnd must point at the end of -new_console[:...] / -cur_console[:...]
				// and include trailing double-quote (if argument was quoted)
				if (iQuotCount > 0)
				{
					if (*pszEnd == L'"' && *(pszEnd-1) != L'"')
					{
						++pszEnd;
						if ((iQuotStart > 0) && (pszFind > pszSpecialCmd))
						{
							--pszFind;
						}
					}
					else if ((pszEnd > pszFind) && (*(pszEnd-1) == L'"') && iQuotStart)
					{
						--pszFind;
					}
					else if (iQuotStart && (*pszEnd == L' ') && *(pszEnd-1) != L' ')
					{
						bTrimTrailingSpace = true;
					}
				}
				else
				{
					while (*(pszEnd-1) == L'"')
					{
						pszEnd--;
					}
				}

				// Trim extra LEADING spaces before -new_console[:...] / -cur_console[:...]
				// Must trim spaces from smth like this:
				//   cmd.exe /C "C:\Temp\run.cmd -cur_console:o"
				// but take care about this
				//   cmd.exe /C "C:\Temp\run.cmd -cur_console:o "abc""
				while (((pszFind - 1) >= pszSpecialCmd)
					&& (*(pszFind-1) == L' ')
					&& (((pszFind - 1) == pszSpecialCmd)
						|| (*(pszFind-2) == L' ')
						|| ((*pszEnd == L'"') && (*(pszEnd-1) != L' '))
						|| (*pszEnd == 0) || (*pszEnd == L' ')))
				{
					pszFind--;
				}
				// Skip extra TRAILING spaces (after -new_console[:...] / -cur_console[:...])
				// IF argument was AT THE BEGINNING of the command!
				if (bTrimTrailingSpace || (pszFind == pszSpecialCmd))
				{
					while (*pszEnd == L' ')
						pszEnd++;
				}

				// Advance pszStopAt
				if (pszStopAt)
					pszStopAt -= (pszEnd - pszFind);

				// Remove from the command processed argument
				wmemmove(pszFind, pszEnd, (lstrlen(pszEnd)+1));
				nChanges++;
			}
			else
			{
				_ASSERTE(pszEnd > pszFind);
				*pszFind = 0;
				nChanges++;
			}
		} // if ((((pszFind == pszFrom) ...
	} // while (!bStop)

	return nChanges;
} // int RConStartArgs::ProcessNewConArg(bool bForceCurConsole /*= false*/)
