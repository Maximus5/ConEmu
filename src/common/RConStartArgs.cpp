
/*
Copyright (c) 2009-2012 Maximus5
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
#include "defines.h"
#include "MAssert.h"
#include "Memory.h"
#include "MStrSafe.h"
#include "RConStartArgs.h"
#include "common.hpp"
#include "WinObjects.h"

#ifdef __GNUC__
#define SecureZeroMemory(p,s) memset(p,0,s)
#endif

#define DefaultSplitValue 500

#ifdef _DEBUG
void RConStartArgs::RunArgTests()
{
	struct { LPCWSTR pszArg, pszNeed; } cTests[] = {
		{
			L"\"c:\\cmd.exe\" \"-new_console\" \"c:\\file.txt\"",
			L"\"c:\\cmd.exe\" \"c:\\file.txt\""
		},
		{
			L"\"c:\\cmd.exe\" -new_console:n \"c:\\file.txt\"",
			L"\"c:\\cmd.exe\" \"c:\\file.txt\""
		},
		{
			L"\"c:\\cmd.exe\" \"-new_console:n\" \"c:\\file.txt\"",
			L"\"c:\\cmd.exe\" \"c:\\file.txt\""
		},
		{
			L"c:\\cmd.exe \"-new_console:n\" \"c:\\file.txt\"",
			L"c:\\cmd.exe \"c:\\file.txt\""
		},
		{
			L"\"c:\\cmd.exe\" \"-new_console:n\" c:\\file.txt",
			L"\"c:\\cmd.exe\" c:\\file.txt"
		},
		{
			L"c:\\file.txt -cur_console",
			L"c:\\file.txt"
		},
		{
			L"\"c:\\file.txt\" -cur_console",
			L"\"c:\\file.txt\""
		},
		{
			L" -cur_console \"c:\\file.txt\"",
			L" \"c:\\file.txt\""
		},
		{
			L"-cur_console \"c:\\file.txt\"",
			L"\"c:\\file.txt\""
		},
		{
			L"-cur_console c:\\file.txt",
			L"c:\\file.txt"
		},
	};

	for (size_t i = 0; i < countof(cTests); i++)
	{
		RConStartArgs arg;
		arg.pszSpecialCmd = lstrdup(cTests[i].pszArg);
		arg.ProcessNewConArg();
		if (lstrcmp(arg.pszSpecialCmd, cTests[i].pszNeed) != 0)
		{
			//_ASSERTE(FALSE && "arg.ProcessNewConArg failed");
			OutputDebugString(L"arg.ProcessNewConArg failed\n");
		}
		int nDbg = 0;
	}
}
#endif


// If you add some members - don't forget them in RConStartArgs::AssignFrom!
RConStartArgs::RConStartArgs()
{
	bDetached = bRunAsAdministrator = bRunAsRestricted = bNewConsole = FALSE;
	bForceUserDialog = bBackgroundTab = bForceDosBox = FALSE;
	eSplit = eSplitNone; nSplitValue = DefaultSplitValue; nSplitPane = 0;
	aRecreate = cra_CreateTab;
	pszSpecialCmd = pszStartupDir = pszUserName = pszDomain = /*pszUserPassword =*/ NULL;
	bBufHeight = FALSE; nBufHeight = 0; bLongOutputDisable = FALSE;
	bInjectsDisable = FALSE;
	eConfirmation = eConfDefault;
	szUserPassword[0] = 0;
	//hLogonToken = NULL;
}

bool RConStartArgs::AssignFrom(const struct RConStartArgs* args)
{
	_ASSERTE(args!=NULL);

	if (args->pszSpecialCmd)
	{
		SafeFree(this->pszSpecialCmd);

		_ASSERTE(args->bDetached == FALSE);
		this->pszSpecialCmd = lstrdup(args->pszSpecialCmd);

		if (!this->pszSpecialCmd)
			return false;
	}

	// Директория запуска. В большинстве случаев совпадает с CurDir в conemu.exe,
	// но может быть задана из консоли, если запуск идет через "-new_console"
	_ASSERTE(this->pszStartupDir==NULL);
	SafeFree(this->pszStartupDir);
	if (args->pszStartupDir)
	{
		this->pszStartupDir = lstrdup(args->pszStartupDir);

		if (!this->pszStartupDir)
			return false;
	}

	this->bRunAsRestricted = args->bRunAsRestricted;
	this->bRunAsAdministrator = args->bRunAsAdministrator;
	SafeFree(this->pszUserName); //SafeFree(this->pszUserPassword);
	SafeFree(this->pszDomain);

	//if (this->hLogonToken) { CloseHandle(this->hLogonToken); this->hLogonToken = NULL; }
	if (args->pszUserName)
	{
		this->pszUserName = lstrdup(args->pszUserName);
		if (args->pszDomain)
			this->pszDomain = lstrdup(args->pszDomain);
		lstrcpy(this->szUserPassword, args->szUserPassword);
		
		//SecureZeroMemory(args->szUserPassword, sizeof(args->szUserPassword));

		//this->pszUserPassword = lstrdup(args->pszUserPassword ? args->pszUserPassword : L"");
		//this->hLogonToken = args->hLogonToken; args->hLogonToken = NULL;
		// -- Do NOT fail when password is empty !!!
		if (!this->pszUserName /*|| !*this->szUserPassword*/)
			return false;
	}

	this->bBackgroundTab = args->bBackgroundTab;
	this->bBufHeight = args->bBufHeight;
	this->nBufHeight = args->nBufHeight;
	this->eConfirmation = args->eConfirmation;
	this->bForceUserDialog = args->bForceUserDialog;

	this->eSplit = args->eSplit;
	this->nSplitValue = args->nSplitValue;
    this->nSplitPane = args->nSplitPane;

	return true;
}

RConStartArgs::~RConStartArgs()
{
	SafeFree(pszSpecialCmd); // именно SafeFree
	SafeFree(pszStartupDir); // именно SafeFree
	SafeFree(pszUserName);
	SafeFree(pszDomain);

	//SafeFree(pszUserPassword);
	if (szUserPassword[0]) SecureZeroMemory(szUserPassword, sizeof(szUserPassword));

	//if (hLogonToken) { CloseHandle(hLogonToken); hLogonToken = NULL; }
}

wchar_t* RConStartArgs::CreateCommandLine(bool abForTasks /*= false*/)
{
	wchar_t* pszFull = NULL;
	size_t cchMaxLen =
				 (pszSpecialCmd ? (lstrlen(pszSpecialCmd) + 3) : 0); // только команда
	cchMaxLen += (pszStartupDir ? (lstrlen(pszStartupDir) + 20) : 0); // "-new_console:d:..."
	cchMaxLen += (bRunAsAdministrator ? 15 : 0); // -new_console:a
	cchMaxLen += (bRunAsRestricted ? 15 : 0); // -new_console:r
	cchMaxLen += (pszUserName ? (lstrlen(pszUserName) + 32 // "-new_console:u:<user>:<pwd>"
						+ (pszDomain ? lstrlen(pszDomain) : 0)
						+ (szUserPassword ? lstrlen(szUserPassword) : 0)) : 0);
	cchMaxLen += (bForceUserDialog ? 15 : 0); // -new_console:u
	cchMaxLen += (bBackgroundTab ? 15 : 0); // -new_console:b
	cchMaxLen += (bBufHeight ? 32 : 0); // -new_console:h<lines>
	cchMaxLen += (bLongOutputDisable ? 15 : 0); // -new_console:o
	cchMaxLen += (bInjectsDisable ? 15 : 0); // -new_console:i
	cchMaxLen += (eConfirmation ? 15 : 0); // -new_console:c / -new_console:n
	cchMaxLen += (bForceDosBox ? 15 : 0); // -new_console:x
	cchMaxLen += (eSplit ? 64 : 0); // -new_console:s[<SplitTab>T][<Percents>](H|V)

	pszFull = (wchar_t*)malloc(cchMaxLen*sizeof(*pszFull));
	if (!pszFull)
	{
		_ASSERTE(pszFull!=NULL);
		return NULL;
	}

	if (pszSpecialCmd)
	{
		if (bRunAsAdministrator && abForTasks)
			_wcscpy_c(pszFull, cchMaxLen, L"*");
		else
			*pszFull = 0;						

		// Не окавычиваем. Этим должен озаботиться пользователь
		_wcscat_c(pszFull, cchMaxLen, pszSpecialCmd);
		_wcscat_c(pszFull, cchMaxLen, L" ");
	}
	else
	{
		*pszFull = 0;
	}

	wchar_t szAdd[128] = L"";
	if (bRunAsAdministrator)
		wcscat_c(szAdd, L"a");
	else if (bRunAsRestricted)
		wcscat_c(szAdd, L"r");
	
	if (bForceUserDialog)
		wcscat_c(szAdd, L"u");
	if (bBackgroundTab)
		wcscat_c(szAdd, L"b");
	if (bForceDosBox)
		wcscat_c(szAdd, L"x");

	if (bForceDosBox)
		wcscat_c(szAdd, L"x");
	
	if (eConfirmation == eConfAlways)
		wcscat_c(szAdd, L"c");
	else if (eConfirmation == eConfNever)
		wcscat_c(szAdd, L"n");

	if (bLongOutputDisable)
		wcscat_c(szAdd, L"o");
	
	if (bInjectsDisable)
		wcscat_c(szAdd, L"i");
	
	if (bBufHeight)
	{
		if (nBufHeight)
			_wsprintf(szAdd+lstrlen(szAdd), SKIPLEN(16) L"h%u", nBufHeight);
		else
			wcscat_c(szAdd, L"h");
	}

	// -new_console:s[<SplitTab>T][<Percents>](H|V)
	if (eSplit)
	{
		wcscat_c(szAdd, L"s");
		if (nSplitPane)
			_wsprintf(szAdd+lstrlen(szAdd), SKIPLEN(16) L"%uT", nSplitPane);
		if ((int)(nSplitValue/10) != 0)
			_wsprintf(szAdd+lstrlen(szAdd), SKIPLEN(16) L"%u", (int)(nSplitValue/10));
		wcscat_c(szAdd, (eSplit == eSplitHorz) ? L"H" : L"V");
	}

	if (szAdd[0])
	{
		_wcscat_c(pszFull, cchMaxLen, bNewConsole ? L" -new_console:" : L" -cur_console:");
		_wcscat_c(pszFull, cchMaxLen, szAdd);
	}

	// "-new_console:d:..."
	if (pszStartupDir && *pszStartupDir)
	{
		bool bQuot = wcschr(pszStartupDir, L' ') != NULL;

		if (bQuot)
			_wcscat_c(pszFull, cchMaxLen, bNewConsole ? L" \"-new_console:d:" : L" \"-cur_console:d:");
		else
			_wcscat_c(pszFull, cchMaxLen, bNewConsole ? L" -new_console:d:" : L" -cur_console:d:");
			
		_wcscat_c(pszFull, cchMaxLen, pszStartupDir);

		if (bQuot)
			_wcscat_c(pszFull, cchMaxLen, L"\"");
	}

	// "-new_console:u:<user>:<pwd>"
	if (pszUserName && *pszUserName)
	{
		_wcscat_c(pszFull, cchMaxLen, bNewConsole ? L" \"-new_console:u:" : L" \"-cur_console:u:");
		if (pszDomain && *pszDomain)
		{
			_wcscat_c(pszFull, cchMaxLen, pszDomain);
			_wcscat_c(pszFull, cchMaxLen, L"\\");
		}
		_wcscat_c(pszFull, cchMaxLen, pszUserName);
		if (szUserPassword)
		{
			_wcscat_c(pszFull, cchMaxLen, L":");
			_wcscat_c(pszFull, cchMaxLen, szUserPassword);
		}
		_wcscat_c(pszFull, cchMaxLen, L"\"");
	}

	return pszFull;
}

BOOL RConStartArgs::CheckUserToken(HWND hPwd)
{
	//if (hLogonToken) { CloseHandle(hLogonToken); hLogonToken = NULL; }
	if (!pszUserName || !*pszUserName)
		return FALSE;

	//wchar_t szPwd[MAX_PATH]; szPwd[0] = 0;
	szUserPassword[0] = 0;

	if (!GetWindowText(hPwd, szUserPassword, MAX_PATH-1))
		return FALSE;

	SafeFree(pszDomain);
	wchar_t* pszSlash = wcschr(pszUserName, L'\\');
	if (pszSlash)
	{
		pszDomain = pszUserName;
		*pszSlash = 0;
		pszUserName = lstrdup(pszSlash+1);
	}

	HANDLE hLogonToken = NULL;
	BOOL lbRc = LogonUser(pszUserName, pszDomain, szUserPassword, LOGON32_LOGON_INTERACTIVE,
	                      LOGON32_PROVIDER_DEFAULT, &hLogonToken);
	//if (szUserPassword[0]) SecureZeroMemory(szUserPassword, sizeof(szUserPassword));

	if (!lbRc || !hLogonToken)
	{
		MessageBox(GetParent(hPwd), L"Invalid user name or password specified!", L"ConEmu", MB_OK|MB_ICONSTOP);
		return FALSE;
	}

	CloseHandle(hLogonToken);
	//hLogonToken may be used for CreateProcessAsUser
	return TRUE;
}

// Returns ">0" - when changes was made
//  0 - no changes
// -1 - error
// bForceCurConsole==true, если разбор параметров идет 
//   при запуске Tasks из GUI
int RConStartArgs::ProcessNewConArg(bool bForceCurConsole /*= false*/)
{
	bNewConsole = FALSE;

	if (!pszSpecialCmd || !*pszSpecialCmd)
	{
		_ASSERTE(pszSpecialCmd && *pszSpecialCmd);
		return -1;
	}

	int nChanges = 0;
	
	// 120115 - Если первый аргумент - "ConEmu.exe" или "ConEmu64.exe" - не обрабатывать "-cur_console" и "-new_console"
	{
		LPCWSTR pszTemp = pszSpecialCmd;
		wchar_t szExe[MAX_PATH+1];
		if (0 == NextArg(&pszTemp, szExe))
		{
			pszTemp = PointToName(szExe);
			if (lstrcmpi(pszTemp, L"ConEmu.exe") == 0
				|| lstrcmpi(pszTemp, L"ConEmu") == 0
				|| lstrcmpi(pszTemp, L"ConEmu64.exe") == 0
				|| lstrcmpi(pszTemp, L"ConEmu64") == 0)
			{
				return 0;
			}
		}
	}
	

	// 111211 - здесь может быть передан "-new_console:..."
	LPCWSTR pszNewCon = L"-new_console";
	// 120108 - или "-cur_console:..." для уточнения параметров запуска команд (из фара например)
	LPCWSTR pszCurCon = L"-cur_console";
	int nNewConLen = lstrlen(pszNewCon);
	_ASSERTE(lstrlen(pszCurCon)==nNewConLen);
	bool bStop = false;

	while (!bStop)
	{
		wchar_t* pszFindNew = wcsstr(pszSpecialCmd, pszNewCon);
		wchar_t* pszFind = pszFindNew ? pszFindNew : wcsstr(pszSpecialCmd, pszCurCon);
		if (pszFindNew)
			bNewConsole = TRUE;
		else if (!pszFind)
			break;

		// Проверка валидности
		_ASSERTE(pszFind >= pszSpecialCmd);
		if (((pszFind == pszSpecialCmd) || (*(pszFind-1) == L'"') || (*(pszFind-1) == L' ')) // начало аргумента
			&& (pszFind[nNewConLen] == L' ' || pszFind[nNewConLen] == L':' 
				|| pszFind[nNewConLen] == L'"' || pszFind[nNewConLen] == 0))
		{
			// -- не будем пока, мешает. например, при запуске задач
			//// По умолчанию, принудительно включить "Press Enter or Esc to close console"
			//if (!bForceCurConsole)
			//	eConfirmation = eConfAlways;
		
			bool lbQuot = (*(pszFind-1) == L'"');
			const wchar_t* pszEnd = pszFind+nNewConLen;
			//wchar_t szNewConArg[MAX_PATH+1];
			if (lbQuot)
				pszFind--;

			if (*pszEnd == L'"')
			{
				pszEnd++;
			}
			else if (*pszEnd != L':')
			{
				// Конец
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

				// Обработка доп.параметров -new_console:xxx
				bool lbReady = false;
				while (!lbReady && *pszEnd)
				{
					switch (*(pszEnd++))
					{
					//case L'-':
					//	bStop = true; // следующие "-new_console" - не трогать!
					//	break;
					case L'"':
					case L' ':
					case 0:
						lbReady = true;
						break;
						
					case L'b':
						// b - background, не активировать таб
						bBackgroundTab = TRUE;
						break;
						
					case L'a':
						// a - RunAs shell verb (as admin on Vista+, login/password in WinXP-)
						bRunAsAdministrator = TRUE;
						break;
						
					case L'r':
						// r - run as restricted user
						bRunAsRestricted = TRUE;
						break;
						
					case L'o':
						// o - disable "Long output" for next command (Far Manager)
						bLongOutputDisable = TRUE;
						break;

					case L'i':
						// i - don't inject ConEmuHk into the starting application
						bInjectsDisable = TRUE;
						break;

					case L'h':
						// "h0" - отключить буфер, "h9999" - включить буфер в 9999 строк
						{
							bBufHeight = TRUE;
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
						// n - отключить "Press Enter or Esc to close console"
						eConfirmation = eConfNever;
						break;
						
					case L'c':
						// c - принудительно включить "Press Enter or Esc to close console"
						eConfirmation = eConfAlways;
						break;
						
					case L'x':
						// x - Force using dosbox for .bat files
						bForceDosBox = TRUE;
						break;
						
					// "Long" code blocks below: 'd', 'u', 's' and so on (in future)
					case L'd':
						// d:<StartupDir>. MUST be last options
						{
							if (*pszEnd == L':')
								pszEnd++;
							const wchar_t* pszDir = pszEnd;
							while ((*pszEnd) && (lbQuot || *pszEnd != L' ') && (*pszEnd != L'"'))
								pszEnd++;
							if (pszEnd > pszDir)
							{
								size_t cchLen = pszEnd - pszDir;
								SafeFree(pszStartupDir);
								pszStartupDir = (wchar_t*)malloc((cchLen+1)*sizeof(*pszStartupDir));
								if (pszStartupDir)
								{
									wmemmove(pszStartupDir, pszDir, cchLen);
									pszStartupDir[cchLen] = 0;
									// Например, "%USERPROFILE%"
									if (wcschr(pszStartupDir, L'%'))
									{
										wchar_t* pszExpand = NULL;
										if (((pszExpand = ExpandEnvStr(pszStartupDir)) != NULL))
										{
											SafeFree(pszStartupDir);
											pszStartupDir = pszExpand;
										}
									}
								}
							}
						} // L'd':
						break;
						
					case L's':
						// s[<SplitTab>T][<Percents>](H|V)
						// Пример: "s3T30H" - разбить 3-ий таб. будет создан новый Pane справа, шириной 30% от 3-го таба.
						{
							UINT nTab = 0 /*active*/, nValue = /*пополам*/DefaultSplitValue/10;
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
                                    	eSplit = (*pszDigits == L'H') ? eSplitHorz : eSplitVert;
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
	                            	eSplit = (*pszEnd == L'H') ? eSplitHorz : eSplitVert;
	                            	pszEnd++;
								}
								else
								{
									break;
								}
							}
							if (!eSplit)
								eSplit = eSplitHorz;
							// Для удобства, пользователь задает размер НОВОЙ части
							nSplitValue = 1000-max(1,min(nValue*10,999)); // проценты
							_ASSERTE(nSplitValue>=1 && nSplitValue<1000);
							nSplitPane = nTab;
						} // L's'
						break;
						
					case L'u':
						{
							// u - ConEmu choose user dialog
							// u:<user>:<pwd> - specify user/pwd in args. MUST be last option
							
							lbReady = true; // последняя опция

							SafeFree(pszUserName);
							SafeFree(pszDomain);
							if (szUserPassword[0]) SecureZeroMemory(szUserPassword, sizeof(szUserPassword));
							
							if (*pszEnd == L':')
							{
								pszEnd++;
								
								wchar_t szUser[MAX_PATH], *p = szUser, *p2 = szUser+countof(szUser)-1;
								while (*pszEnd && (p < p2))
								{
									if ((*pszEnd == 0) || (*pszEnd == L':') || (*pszEnd == L'"'))
									{
										break;
									}
									//else if (*pszEnd == L'"' && *(pszEnd+1) == L'"')
									//{
									//	*(p++) = L'"'; pszEnd += 2;
									//}
									else if (*pszEnd == L'^')
									{
										pszEnd++;
										*(p++) = *(pszEnd++);
									}
									else
									{
										*(p++) = *(pszEnd++);
									}
								}
								*p = 0;

								wchar_t* pszSlash = wcschr(szUser, L'\\');
								if (pszSlash)
								{
									*pszSlash = 0;
									pszDomain = lstrdup(szUser);
									pszUserName = lstrdup(pszSlash+1);
								}
								else
								{
									pszUserName = lstrdup(szUser);
								}
								
								if (*pszEnd == L':')
								{
									pszEnd++;
									//lstrcpyn(szUserPassword, pszPwd, countof(szUserPassword));

									p = szUserPassword; p2 = szUserPassword+countof(szUserPassword)-1;
									while (*pszEnd && (p < p2))
									{
										if ((*pszEnd == 0) || (*pszEnd == L':') || (*pszEnd == L'"'))
										{
											break;
										}
										else if (*pszEnd == L'^')
										{
											pszEnd++;
											*(p++) = *(pszEnd++);
										}
										else
										{
											*(p++) = *(pszEnd++);
										}
									}
									*p = 0;

								}
							}
							else
							{
								bForceUserDialog = TRUE;
							}
						} // L'u'
						break;
					}
				}
			}

			if (pszEnd > pszFind)
			{
				// pszEnd должен указывать на конец -new_console[:...] / -cur_console[:...]
				// и включать обрамляющую кавычку, если он окавычен
				if (lbQuot)
				{
					if (*pszEnd == L'"' && *(pszEnd-1) != L'"')
						pszEnd++;
				}
				else
				{
					while (*(pszEnd-1) == L'"')
						pszEnd--;
				}

				// Откусить лишние пробелы, которые стоят ПЕРЕД -new_console[:...] / -cur_console[:...]
				while (((pszFind - 1) > pszSpecialCmd)
					&& (*(pszFind-1) == L' ')
					&& ((*(pszFind-2) == L' ') || (/**pszEnd == L'"' ||*/ *pszEnd == 0 || *pszEnd == L' ')))
				{
					pszFind--;
				}

				wmemmove(pszFind, pszEnd, (lstrlen(pszEnd)+1));
				nChanges++;
			}
			else
			{
				_ASSERTE(pszEnd > pszFind);
				*pszFind = 0;
				nChanges++;
			}
		}
	}

	return nChanges;
}
