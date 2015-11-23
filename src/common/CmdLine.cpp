
/*
Copyright (c) 2009-2015 Maximus5
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
#include "common.hpp"
#include "CmdArg.h"
#include "CmdLine.h"
#include "WObjects.h"

#ifndef CONEMU_MINIMAL
// В ConEmuHk редиректор НЕ отключаем
#include "MWow64Disable.h"
#endif

// Function checks, if we need drop first and last quotation marks
// Example: ""7z.exe" /?"
// Using cmd.exe rules
bool IsNeedDequote(LPCWSTR asCmdLine, bool abFromCmdCK, LPCWSTR* rsEndQuote/*=NULL*/)
{
	if (rsEndQuote)
		*rsEndQuote = NULL;

	if (!asCmdLine)
		return false;

	bool bDeQu = false;
	LPCWSTR pszQE, pszSP;
	if (asCmdLine[0] == L'"')
	{
		bDeQu = (asCmdLine[1] == L'"');
		// Всегда - нельзя. Иначе парсинг строки запуска некорректно идет
		// L"\"C:\\ConEmu\\ConEmuC64.exe\"  /PARENTFARPID=1 /C \"C:\\GIT\\cmdw\\ad.cmd CE12.sln & ci -m \"Solution debug build properties\"\""
		if (!bDeQu)
		{
			size_t nLen = lstrlen(asCmdLine);
			if (abFromCmdCK)
			{
				bDeQu = ((asCmdLine[nLen-1] == L'"') && (asCmdLine[nLen-2] == L'"'));
			}
			if (!bDeQu && (asCmdLine[nLen-1] == L'"'))
			{
				pszSP = wcschr(asCmdLine+1, L' ');
				pszQE = wcschr(asCmdLine+1, L'"');
				if (pszSP && pszQE && (pszSP < pszQE)
					&& ((pszSP - asCmdLine) < MAX_PATH))
				{
					CmdArg lsTmp;
					lsTmp.Set(asCmdLine+1, pszSP-asCmdLine-1);
					bDeQu = (IsFilePath(lsTmp, true) && IsExecutable(lsTmp));
				}
			}
		}
	}
	if (!bDeQu)
		return false;

	// Don't dequote?
	pszQE = wcsrchr(asCmdLine+2, L'"');
	if (!pszQE)
		return false;

#if 0
	LPCWSTR pszQ1 = wcschr(asCmdLine+2, L'"');
	if (!pszQ1)
		return false;
	LPCWSTR pszQE = wcsrchr(pszQ1, L'"');
	// Only TWO quotes in asCmdLine?
	if (pszQE == pszQ1)
	{
		// Doesn't contains special symbols?
		if (!wcspbrk(asCmdLine+1, L"&<>()@^|"))
		{
			// Must contains spaces (doubt?)
			if (wcschr(asCmdLine+1, L' '))
			{
				// Cmd also checks this for executable file name. Skip this check?
				return false;
			}
		}
	}
#endif

	// Well, we get here
	_ASSERTE(asCmdLine[0]==L'"' && pszQE && *pszQE==L'"' && !wcschr(pszQE+1,L'"'));
	// Dequote it!
	if (rsEndQuote)
		*rsEndQuote = pszQE;
	return true;
}

// Returns 0 if succeeded, other number on error
int NextArg(const wchar_t** asCmdLine, CmdArg &rsArg, const wchar_t** rsArgStart/*=NULL*/)
{
	if (!asCmdLine || !*asCmdLine)
		return CERR_CMDLINEEMPTY;

	#ifdef _DEBUG
	if ((rsArg.mn_TokenNo==0) // first token
		|| ((rsArg.mn_TokenNo>0) && (rsArg.ms_LastTokenEnd==*asCmdLine)
			&& (wcsncmp(*asCmdLine,rsArg.ms_LastTokenSave,countof(rsArg.ms_LastTokenSave)-1))==0))
	{
		// OK, параметры корректны
	}
	else
	{
		_ASSERTE(FALSE && "rsArgs was not resetted before new cycle!");
	}
	#endif

	LPCWSTR psCmdLine = SkipNonPrintable(*asCmdLine), pch = NULL;
	if (!*psCmdLine)
		return CERR_CMDLINEEMPTY;

	// Remote surrounding quotes, in certain cases
	// Example: ""7z.exe" /?"
	// Example: "C:\Windows\system32\cmd.exe" /C ""C:\Python27\python.EXE""
	if ((rsArg.mn_TokenNo == 0) || (rsArg.mn_CmdCall == CmdArg::cc_CmdCK))
	{
		if (IsNeedDequote(psCmdLine, (rsArg.mn_CmdCall == CmdArg::cc_CmdCK), &rsArg.mpsz_Dequoted))
			psCmdLine++;
		if (rsArg.mn_CmdCall == CmdArg::cc_CmdCK)
			rsArg.mn_CmdCall = CmdArg::cc_CmdCommand;
	}

	size_t nArgLen = 0;
	bool lbQMode = false;

	// аргумент начинается с "
	if (*psCmdLine == L'"')
	{
		lbQMode = true;
		psCmdLine++;
		// ... /d "\"C:\ConEmu\ConEmuPortable.exe\" /Dir ...
		bool bQuoteEscaped = (psCmdLine[0] == L'\\' && psCmdLine[1] == L'"');
		pch = wcschr(psCmdLine, L'"');
		if (pch && (pch > psCmdLine))
		{
			// To be correctly parsed something like this:
			// reg.exe add "HKCU\MyCo" /ve /t REG_EXPAND_SZ /d "\"C:\ConEmu\ConEmuPortable.exe\" /Dir \"%V\" /cmd \"cmd.exe\" \"-new_console:nC:cmd.exe\" \"-cur_console:d:%V\"" /f
			// But must not fails with ‘simple’ command like (no escapes in "C:\"):
			// /dir "C:\" /icon "cmd.exe" /single

			// Prev version fails while getting strings for -GuiMacro, example:
			// ConEmu.exe -detached -GuiMacro "print(\" echo abc \"); Context;"

			pch = wcspbrk(psCmdLine, L"\\\"");
			while (pch)
			{
				// Escaped quotation?
				if ((*pch == L'\\') && (*(pch+1) == L'"'))
				{
					// It's allowed when:
					// a) at the beginning of the line (handled above, bQuoteEscaped);
					// b) after space, left bracket or colon (-GuiMacro)
					// c) when already was forced by bQuoteEscaped
					if ((((pch - 1) >= psCmdLine) && wcschr(L" (,", *(pch-1))) || bQuoteEscaped)
					{
						bQuoteEscaped = true;
						pch++; // Point to "
					}
				}
				else if (*pch == L'"')
					break;
				// Next entry AFTER pch
				pch = wcspbrk(pch+1, L"\\\"");
			}
		}

		if (!pch) return CERR_CMDLINE;

		while (pch[1] == L'"' && (!rsArg.mpsz_Dequoted || ((pch+1) < rsArg.mpsz_Dequoted)))
		{
			pch += 2;
			pch = wcschr(pch, L'"');

			if (!pch) return CERR_CMDLINE;
		}

		// Теперь в pch ссылка на последнюю "
	}
	else
	{
		// До конца строки или до первого пробела
		//pch = wcschr(psCmdLine, L' ');
		// 09.06.2009 Maks - обломался на: cmd /c" echo Y "
		pch = psCmdLine;

		// Ищем обычным образом (до пробела/кавычки)
		while (*pch && *pch!=L' ' && *pch!=L'"') pch++;

		//if (!pch) pch = psCmdLine + lstrlenW(psCmdLine); // до конца строки
	}

	nArgLen = pch - psCmdLine;

	// Вернуть аргумент
	if (!rsArg.Set(psCmdLine, nArgLen))
		return CERR_CMDLINE;
	rsArg.mn_TokenNo++;

	if (rsArgStart) *rsArgStart = psCmdLine;

	psCmdLine = pch;
	// Finalize
	if ((*psCmdLine == L'"') && (lbQMode || (rsArg.mpsz_Dequoted == psCmdLine)))
		psCmdLine++; // was pointed to closing quotation mark

	psCmdLine = SkipNonPrintable(psCmdLine);
	// When whole line was dequoted
	if ((*psCmdLine == L'"') && (rsArg.mpsz_Dequoted == psCmdLine))
		psCmdLine++;

	#ifdef _DEBUG
	rsArg.ms_LastTokenEnd = psCmdLine;
	lstrcpyn(rsArg.ms_LastTokenSave, psCmdLine, countof(rsArg.ms_LastTokenSave));
	#endif

	switch (rsArg.mn_CmdCall)
	{
	case CmdArg::cc_Undefined:
		// Если это однозначно "ключ" - то на имя файла не проверяем
		if (*rsArg.ms_Arg == L'/' || *rsArg.ms_Arg == L'-')
		{
			// Это для парсинга (чтобы ассертов не было) параметров из ShellExecute (там cmd.exe указывается в другом аргументе)
			if ((rsArg.mn_TokenNo == 1) && (lstrcmpi(rsArg.ms_Arg, L"/C") == 0 || lstrcmpi(rsArg.ms_Arg, L"/K") == 0))
				rsArg.mn_CmdCall = CmdArg::cc_CmdCK;
		}
		else
		{
			pch = PointToName(rsArg.ms_Arg);
			if (pch)
			{
				if ((lstrcmpi(pch, L"cmd") == 0 || lstrcmpi(pch, L"cmd.exe") == 0)
					|| (lstrcmpi(pch, L"ConEmuC") == 0 || lstrcmpi(pch, L"ConEmuC.exe") == 0)
					|| (lstrcmpi(pch, L"ConEmuC64") == 0 || lstrcmpi(pch, L"ConEmuC64.exe") == 0))
				{
					rsArg.mn_CmdCall = CmdArg::cc_CmdExeFound;
				}
			}
		}
		break;
	case CmdArg::cc_CmdExeFound:
		if (lstrcmpi(rsArg.ms_Arg, L"/C") == 0 || lstrcmpi(rsArg.ms_Arg, L"/K") == 0)
			rsArg.mn_CmdCall = CmdArg::cc_CmdCK;
		else if ((rsArg.ms_Arg[0] != L'/') && (rsArg.ms_Arg[0] != L'-'))
			rsArg.mn_CmdCall = CmdArg::cc_Undefined;
		break;
	}

	*asCmdLine = psCmdLine;
	return 0;
}

int NextLine(const wchar_t** asLines, CEStr &rsLine, NEXTLINEFLAGS Flags /*= NLF_TRIM_SPACES|NLF_SKIP_EMPTY_LINES*/)
{
	if (!asLines || !*asLines)
		return CERR_CMDLINEEMPTY;

	const wchar_t* psz = *asLines;
	//const wchar_t szSpaces[] = L" \t";
	//const wchar_t szLines[] = L"\r\n";
	//const wchar_t szSpacesLines[] = L" \t\r\n";

	if ((Flags & (NLF_TRIM_SPACES|NLF_SKIP_EMPTY_LINES)) == (NLF_TRIM_SPACES|NLF_SKIP_EMPTY_LINES))
		psz = SkipNonPrintable(psz);
	else if (Flags & NLF_TRIM_SPACES)
		while (*psz == L' ' || *psz == L'\t') psz++;
	else if (Flags & NLF_SKIP_EMPTY_LINES)
		while (*psz == L'\r' || *psz == L'\n') psz++;

	if (!*psz)
	{
		*asLines = psz;
		return CERR_CMDLINEEMPTY;
	}

	const wchar_t* pszEnd = wcspbrk(psz, L"\r\n");
	if (!pszEnd)
	{
		pszEnd = psz + lstrlen(psz);
	}

	const wchar_t* pszTrim = pszEnd;
	if (*pszEnd == L'\r') pszEnd++;
	if (*pszEnd == L'\n') pszEnd++;

	if (Flags & NLF_TRIM_SPACES)
	{
		while ((pszTrim > psz) && ((*(pszTrim-1) == L' ') || (*(pszTrim-1) == L'\t')))
			pszTrim--;
	}

	_ASSERTE(pszTrim >= psz);
	rsLine.Set(psz, pszTrim-psz);
	psz = pszEnd;

	*asLines = psz;
	return 0;
}

int AddEndSlash(wchar_t* rsPath, int cchMax)
{
	if (!rsPath || !*rsPath)
		return 0;
	int nLen = lstrlen(rsPath);
	if (rsPath[nLen-1] != L'\\')
	{
		if (cchMax >= (nLen+2))
		{
			rsPath[nLen++] = L'\\'; rsPath[nLen] = 0;
		}
	}
	return nLen;
}

const wchar_t* SkipNonPrintable(const wchar_t* asParams)
{
	if (!asParams)
		return NULL;
	const wchar_t* psz = asParams;
	while (*psz == L' ' || *psz == L'\t' || *psz == L'\r' || *psz == L'\n') psz++;
	return psz;
}

// One trailing (or middle) asterisk allowed
bool CompareFileMask(const wchar_t* asFileName, const wchar_t* asMask)
{
	if (!asFileName || !*asFileName || !asMask || !*asMask)
		return false;
	// Any file?
	if (*asMask == L'*' && *(asMask+1) == 0)
		return true;

	int iCmp = -1;

	wchar_t sz1[MAX_PATH+1], sz2[MAX_PATH+1];
	lstrcpyn(sz1, asFileName, countof(sz1));
	size_t nLen1 = lstrlen(sz1);
	CharUpperBuffW(sz1, (DWORD)nLen1);
	lstrcpyn(sz2, asMask, countof(sz2));
	size_t nLen2 = lstrlen(sz2);
	CharUpperBuffW(sz2, (DWORD)nLen2);

	wchar_t* pszAst = wcschr(sz2, L'*');
	if (!pszAst)
	{
		iCmp = lstrcmp(sz1, sz2);
	}
	else
	{
		*pszAst = 0;
		size_t nLen = pszAst - sz2;
		size_t nRight = lstrlen(pszAst+1);
		if (wcsncmp(sz1, sz2, nLen) == 0)
		{
			if (!nRight)
				iCmp = 0;
			else if (nLen1 >= (nRight + nLen))
				iCmp = lstrcmp(sz1+nLen1-nRight, pszAst+1);
		}
	}

	return (iCmp == 0);
}

LPCWSTR GetDrive(LPCWSTR pszPath, wchar_t* szDrive, int/*countof(szDrive)*/ cchDriveMax)
{
	if (!szDrive || cchDriveMax < 16)
		return NULL;

	if (pszPath[0] != L'\\' && pszPath[1] == L':')
	{
		lstrcpyn(szDrive, pszPath, 3);
	}
	else if (lstrcmpni(pszPath, L"\\\\?\\UNC\\", 8) == 0)
	{
		// UNC format? "\\?\UNC\Server\Share"
		LPCWSTR pszSlash = wcschr(pszPath+8, L'\\'); // point to end of server name
		pszSlash = pszSlash ? wcschr(pszSlash+1, L'\\') : NULL; // point to end of share name
		lstrcpyn(szDrive, pszPath, pszSlash ? (int)min((INT_PTR)cchDriveMax,pszSlash-pszPath+1) : cchDriveMax);
	}
	else if (lstrcmpni(pszPath, L"\\\\?\\", 4) == 0 && pszPath[4] && pszPath[5] == L':')
	{
		lstrcpyn(szDrive, pszPath, 7);
	}
	else if (pszPath[0] == L'\\' && pszPath[1] == L'\\')
	{
		// "\\Server\Share"
		LPCWSTR pszSlash = wcschr(pszPath+2, L'\\'); // point to end of server name
		pszSlash = pszSlash ? wcschr(pszSlash+1, L'\\') : NULL; // point to end of share name
		lstrcpyn(szDrive, pszPath, pszSlash ? (int)min((INT_PTR)cchDriveMax,pszSlash-pszPath+1) : cchDriveMax);
	}
	else
	{
		lstrcpyn(szDrive, L"", cchDriveMax);
	}
	return szDrive;
}

int GetDirectory(CmdArg& szDir)
{
	int iRc = 0;
	DWORD nLen, nMax;

	nMax = GetCurrentDirectory(MAX_PATH, szDir.GetBuffer(MAX_PATH));
	if (!nMax)
	{
		goto wrap;
	}
	else if (nMax > MAX_PATH)
	{
		nLen = GetCurrentDirectory(nMax, szDir.GetBuffer(nMax));
		if (!nLen || (nLen > nMax))
		{
			goto wrap;
		}
	}

	iRc = lstrlen(szDir);
wrap:
	return iRc;
}

// Команды, которые не нужно пытаться развернуть в exe?
// кроме того, если команда содержит "?" или "*" - тоже не пытаться.
const wchar_t* gsInternalCommands = L"ACTIVATE\0ALIAS\0ASSOC\0ATTRIB\0BEEP\0BREAK\0CALL\0CDD\0CHCP\0COLOR\0COPY\0DATE\0DEFAULT\0DEL\0DELAY\0DESCRIBE\0DETACH\0DIR\0DIRHISTORY\0DIRS\0DRAWBOX\0DRAWHLINE\0DRAWVLINE\0ECHO\0ECHOERR\0ECHOS\0ECHOSERR\0ENDLOCAL\0ERASE\0ERRORLEVEL\0ESET\0EXCEPT\0EXIST\0EXIT\0FFIND\0FOR\0FREE\0FTYPE\0GLOBAL\0GOTO\0HELP\0HISTORY\0IF\0IFF\0INKEY\0INPUT\0KEYBD\0KEYS\0LABEL\0LIST\0LOG\0MD\0MEMORY\0MKDIR\0MOVE\0MSGBOX\0NOT\0ON\0OPTION\0PATH\0PAUSE\0POPD\0PROMPT\0PUSHD\0RD\0REBOOT\0REN\0RENAME\0RMDIR\0SCREEN\0SCRPUT\0SELECT\0SET\0SETDOS\0SETLOCAL\0SHIFT\0SHRALIAS\0START\0TEE\0TIME\0TIMER\0TITLE\0TOUCH\0TREE\0TRUENAME\0TYPE\0UNALIAS\0UNSET\0VER\0VERIFY\0VOL\0VSCRPUT\0WINDOW\0Y\0\0";

bool IsNeedCmd(BOOL bRootCmd, LPCWSTR asCmdLine, CmdArg &szExe,
			   LPCWSTR* rsArguments /*= NULL*/, BOOL* rpbNeedCutStartEndQuot /*= NULL*/,
			   BOOL* rpbRootIsCmdExe /*= NULL*/, BOOL* rpbAlwaysConfirmExit /*= NULL*/, BOOL* rpbAutoDisableConfirmExit /*= NULL*/)
{
	bool rbNeedCutStartEndQuot = false;
	bool rbRootIsCmdExe = true;
	bool rbAlwaysConfirmExit = false;
	bool rbAutoDisableConfirmExit = false;

	wchar_t *pwszEndSpace;

	if (rsArguments) *rsArguments = NULL;

	bool lbRc = false;
	int iRc = 0;
	BOOL lbFirstWasGot = FALSE;
	LPCWSTR pwszCopy;
	int nLastChar;
	#ifdef _DEBUG
	CmdArg szDbgFirst;
	#endif

	if (!asCmdLine || !*asCmdLine)
	{
		_ASSERTE(asCmdLine && *asCmdLine);
		goto wrap;
	}

	#ifdef _DEBUG
	// Это минимальные проверки, собственно к коду - не относятся
	bool bIsBatch = false;
	{
		LPCWSTR psz = asCmdLine;
		NextArg(&psz, szDbgFirst);
		psz = PointToExt(szDbgFirst);
		if (lstrcmpi(psz, L".cmd")==0 || lstrcmpi(psz, L".bat")==0)
			bIsBatch = true;
	}
	#endif

	if (!szExe.GetBuffer(MAX_PATH))
	{
		_ASSERTE(FALSE && "Failed to allocate MAX_PATH");
		lbRc = true;
		goto wrap;
	}
	szExe.Empty();

	if (!asCmdLine || *asCmdLine == 0)
	{
		_ASSERTE(asCmdLine && *asCmdLine);
		lbRc = true;
		goto wrap;
	}


	pwszCopy = asCmdLine;
	// cmd /c ""c:\program files\arc\7z.exe" -?"   // да еще и внутри могут быть двойными...
	// cmd /c "dir c:\"
	nLastChar = lstrlenW(pwszCopy) - 1;

	if (pwszCopy[0] == L'"' && pwszCopy[nLastChar] == L'"')
	{
		//if (pwszCopy[1] == L'"' && pwszCopy[2])
		//{
		//	pwszCopy ++; // Отбросить первую кавычку в командах типа: ""c:\program files\arc\7z.exe" -?"

		//	if (rbNeedCutStartEndQuot) *rbNeedCutStartEndQuot = TRUE;
		//}
		//else
			// глючила на ""F:\VCProject\FarPlugin\#FAR180\far.exe  -new_console""
			//if (wcschr(pwszCopy+1, L'"') == (pwszCopy+nLastChar)) {
			//	LPCWSTR pwszTemp = pwszCopy;
			//	// Получим первую команду (исполняемый файл?)
			//	if ((iRc = NextArg(&pwszTemp, szArg)) != 0) {
			//		//Parsing command line failed
			//		lbRc = true; goto wrap;
			//	}
			//	pwszCopy ++; // Отбросить первую кавычку в командах типа: "c:\arc\7z.exe -?"
			//	lbFirstWasGot = TRUE;
			//	if (rbNeedCutStartEndQuot) *rbNeedCutStartEndQuot = TRUE;
			//} else
		{
			// Will be dequoted in 'NextArg' function. Examples
			// "C:\GCC\msys\bin\make.EXE -f "makefile" COMMON="../../../plugins/common""
			// ""F:\VCProject\FarPlugin\#FAR180\far.exe  -new_console""
			// ""cmd""
			// cmd /c ""c:\program files\arc\7z.exe" -?"   // да еще и внутри могут быть двойными...
			// cmd /c "dir c:\"

			LPCWSTR pwszTemp = pwszCopy;

			// Получим первую команду (исполняемый файл?)
			if ((iRc = NextArg(&pwszTemp, szExe)) != 0)
			{
				//Parsing command line failed
				#ifdef WARN_NEED_CMD
				_ASSERTE(FALSE);
				#endif
				lbRc = true; goto wrap;
			}

			if (lstrcmpiW(szExe, L"start") == 0)
			{
				// Команду start обрабатывает только процессор
				#ifdef WARN_NEED_CMD
				_ASSERTE(FALSE);
				#endif
				lbRc = true; goto wrap;
			}

			LPCWSTR pwszQ = pwszCopy + 1 + lstrlen(szExe);
			wchar_t* pszExpand = NULL;

			if (*pwszQ != L'"' && IsExecutable(szExe, &pszExpand))
			{
				pwszCopy ++; // отбрасываем
				lbFirstWasGot = TRUE;

				if (pszExpand)
				{
					szExe.Set(pszExpand);
					SafeFree(pszExpand);
					if (rsArguments)
						*rsArguments = pwszQ;
				}

				rbNeedCutStartEndQuot = true;
			}
		}
	}

	// Получим первую команду (исполняемый файл?)
	if (!lbFirstWasGot)
	{
		szExe.Empty();
		// 17.10.2010 - поддержка переданного исполняемого файла без параметров, но с пробелами в пути
		LPCWSTR pchEnd = pwszCopy + lstrlenW(pwszCopy);

		while (pchEnd > pwszCopy && *(pchEnd-1) == L' ') pchEnd--;

		if ((pchEnd - pwszCopy) < MAX_PATH)
		{

			szExe.Set(pwszCopy, (pchEnd - pwszCopy));
			_ASSERTE(szExe[(pchEnd - pwszCopy)] == 0);

			if (lstrcmpiW(szExe, L"start") == 0)
			{
				// Команду start обрабатывает только процессор
				#ifdef WARN_NEED_CMD
				_ASSERTE(FALSE);
				#endif
				lbRc = true; goto wrap;
			}

			// Обработка переменных окружения и поиск в PATH
			if (FileExistsSearch((LPCWSTR)szExe, szExe))
			{
				if (rsArguments)
					*rsArguments = pchEnd;
			}
			else
			{
				szExe.Empty();
			}
		}

		if (szExe[0] == 0)
		{
			if ((iRc = NextArg(&pwszCopy, szExe)) != 0)
			{
				//Parsing command line failed
				#ifdef WARN_NEED_CMD
				_ASSERTE(FALSE);
				#endif
				lbRc = true; goto wrap;
			}

			if (lstrcmpiW(szExe, L"start") == 0)
			{
				// Команду start обрабатывает только процессор
				#ifdef WARN_NEED_CMD
				_ASSERTE(FALSE);
				#endif
				lbRc = true; goto wrap;
			}

			// Обработка переменных окружения и поиск в PATH
			if (FileExistsSearch((LPCWSTR)szExe, szExe))
			{
				if (rsArguments)
					*rsArguments = pwszCopy;
			}
		}
	}

	if (!*szExe)
	{
		_ASSERTE(szExe[0] != 0);
	}
	else
	{
		// Если юзеру нужен редирект - то он должен явно указать ком.процессор
		// Иначе нереально (или достаточно сложно) определить, является ли символ
		// редиректом, или это просто один из аргументов команды...

		// "Левые" символы в имени файла (а вот в "первом аргументе" все однозначно)
		if (wcspbrk(szExe, L"?*<>|"))
		{
			rbRootIsCmdExe = TRUE; // запуск через "процессор"
			lbRc = true; goto wrap; // добавить "cmd.exe"
		}

		// если "путь" не указан
		if (wcschr(szExe, L'\\') == NULL)
		{
			bool bHasExt = (wcschr(szExe, L'.') != NULL);
			// Проверим, может это команда процессора (типа "DIR")?
			if (!bHasExt)
			{
				bool bIsCommand = false;
				wchar_t* pszUppr = lstrdup(szExe);
				if (pszUppr)
				{
					// избежать линковки на user32.dll
					//CharUpperBuff(pszUppr, lstrlen(pszUppr));
					for (wchar_t* p = pszUppr; *p; p++)
					{
						if (*p >= L'a' && *p <= 'z')
							*p -= 0x20;
					}

					const wchar_t* pszFind = gsInternalCommands;
					while (*pszFind)
					{
						if (lstrcmp(pszUppr, pszFind) == 0)
						{
							bIsCommand = true;
							break;
						}
						pszFind += lstrlen(pszFind)+1;
					}
					free(pszUppr);
				}
				if (bIsCommand)
				{
					#ifdef WARN_NEED_CMD
					_ASSERTE(FALSE);
					#endif
					rbRootIsCmdExe = TRUE; // запуск через "процессор"
					lbRc = true; goto wrap; // добавить "cmd.exe"
				}
			}

			// Пробуем найти "по путям" соответствующий exe-шник.
			INT_PTR nCchMax = szExe.mn_MaxLen; // выделить память, длинее чем szExe вернуть не сможем
			wchar_t* pszSearch = (wchar_t*)malloc(nCchMax*sizeof(wchar_t));
			if (pszSearch)
			{
				#ifndef CONEMU_MINIMAL
				MWow64Disable wow; wow.Disable(); // Отключить редиректор!
				#endif
				wchar_t *pszName = NULL;
				INT_PTR nRc = SearchPath(NULL, szExe, bHasExt ? NULL : L".exe", (DWORD)nCchMax, pszSearch, &pszName);
				if (nRc && (nRc < nCchMax))
				{
					// Нашли, возвращаем что нашли
					szExe.Set(pszSearch);
				}
				free(pszSearch);
			}
		} // end: if (wcschr(szExe, L'\\') == NULL)
	}

	// Если szExe не содержит путь к файлу - запускаем через cmd
	// "start "" C:\Utils\Files\Hiew32\hiew32.exe C:\00\Far.exe"
	if (!IsFilePath(szExe))
	{
		#ifdef WARN_NEED_CMD
		_ASSERTE(FALSE);
		#endif
		rbRootIsCmdExe = TRUE; // запуск через "процессор"
		lbRc = true; goto wrap; // добавить "cmd.exe"
	}

	//pwszCopy = wcsrchr(szArg, L'\\'); if (!pwszCopy) pwszCopy = szArg; else pwszCopy ++;
	pwszCopy = PointToName(szExe);
	//2009-08-27
	pwszEndSpace = szExe.ms_Arg + lstrlenW(szExe) - 1;

	while ((*pwszEndSpace == L' ') && (pwszEndSpace > szExe))
	{
		*(pwszEndSpace--) = 0;
	}

#ifndef __GNUC__
#pragma warning( push )
#pragma warning(disable : 6400)
#endif

	if (lstrcmpiW(pwszCopy, L"cmd")==0 || lstrcmpiW(pwszCopy, L"cmd.exe")==0
		|| lstrcmpiW(pwszCopy, L"tcc")==0 || lstrcmpiW(pwszCopy, L"tcc.exe")==0)
	{
		rbRootIsCmdExe = TRUE; // уже должен быть выставлен, но проверим
		rbAlwaysConfirmExit = TRUE; rbAutoDisableConfirmExit = FALSE;
		_ASSERTE(!bIsBatch);
		lbRc = false; goto wrap; // уже указан командный процессор, cmd.exe в начало добавлять не нужно
	}


	// Issue 1211: Decide not to do weird heuristic.
	//   If user REALLY needs redirection (root command, huh?)
	//   - he must call "cmd /c ..." directly
	// Если есть одна из команд перенаправления, или слияния - нужен CMD.EXE
	if (!bRootCmd)
	{
		if (wcschr(asCmdLine, L'&') ||
			wcschr(asCmdLine, L'>') ||
			wcschr(asCmdLine, L'<') ||
			wcschr(asCmdLine, L'|') ||
			wcschr(asCmdLine, L'^') // или экранирования
			)
		{
			#ifdef WARN_NEED_CMD
			_ASSERTE(FALSE);
			#endif
			lbRc = true; goto wrap;
		}
	}

	//if (lstrcmpiW(pwszCopy, L"far")==0 || lstrcmpiW(pwszCopy, L"far.exe")==0)
	if (IsFarExe(pwszCopy))
	{
		bool bFound = (wcschr(pwszCopy, L'.') != NULL);
		// Если указали при запуске просто "far" - это может быть батник, расположенный в %PATH%
		if (!bFound)
		{
			wchar_t szSearch[MAX_PATH+1], *pszPart = NULL;
			DWORD n = SearchPath(NULL, pwszCopy, L".exe", countof(szSearch), szSearch, &pszPart);
			if (n && (n < countof(szSearch)))
			{
				if (lstrcmpi(PointToExt(pszPart), L".exe") == 0)
					bFound = true;
			}
		}

		if (bFound)
		{
			rbAutoDisableConfirmExit = TRUE;
			rbRootIsCmdExe = FALSE; // FAR!
			_ASSERTE(!bIsBatch);
			lbRc = false; goto wrap; // уже указан исполняемый файл, cmd.exe в начало добавлять не нужно
		}
	}

	if (IsExecutable(szExe))
	{
		rbRootIsCmdExe = FALSE; // Для других программ - буфер не включаем
		_ASSERTE(!bIsBatch);
		lbRc = false; goto wrap; // Запускается конкретная консольная программа. cmd.exe не требуется
	}

	//Можно еще Доделать поиски с: SearchPath, GetFullPathName, добавив расширения .exe & .com
	//хотя фар сам формирует полные пути к командам, так что можно не заморачиваться
	#ifdef WARN_NEED_CMD
	_ASSERTE(FALSE);
	#endif
	rbRootIsCmdExe = TRUE;
#ifndef __GNUC__
#pragma warning( pop )
#endif

	lbRc = true;
wrap:
	if (rpbNeedCutStartEndQuot)
		*rpbNeedCutStartEndQuot = rbNeedCutStartEndQuot;
	if (rpbRootIsCmdExe)
		*rpbRootIsCmdExe = rbRootIsCmdExe;
	if (rpbAlwaysConfirmExit)
		*rpbAlwaysConfirmExit = rbAlwaysConfirmExit;
	if (rpbAutoDisableConfirmExit)
		*rpbAutoDisableConfirmExit = rbAutoDisableConfirmExit;
	return lbRc;
}

#ifndef __GNUC__
#pragma warning( push )
#pragma warning(disable : 6400)
#endif
bool IsExecutable(LPCWSTR aszFilePathName, wchar_t** rsExpandedVars /*= NULL*/)
{
#ifndef __GNUC__
#pragma warning( push )
#pragma warning(disable : 6400)
#endif

	wchar_t* pszExpand = NULL;

	for (int i = 0; i <= 1; i++)
	{
		LPCWSTR pwszDot = PointToExt(aszFilePathName);

		if (pwszDot)  // Если указан .exe или .com файл
		{
			if (lstrcmpiW(pwszDot, L".exe")==0 || lstrcmpiW(pwszDot, L".com")==0)
			{
				if (FileExists(aszFilePathName))
					return true;
			}
		}

		if (!i && wcschr(aszFilePathName, L'%'))
		{
			pszExpand = ExpandEnvStr(aszFilePathName);
			if (!pszExpand)
				break;
			aszFilePathName = pszExpand;
		}
	}

	if (rsExpandedVars)
	{
		*rsExpandedVars = pszExpand; pszExpand = NULL;
	}
	else
	{
		SafeFree(pszExpand);
	}

	return false;
}
#ifndef __GNUC__
#pragma warning( pop )
#endif

bool CompareProcessNames(LPCWSTR pszProcess1, LPCWSTR pszProcess2)
{
	LPCWSTR pszName1 = PointToName(pszProcess1);
	LPCWSTR pszName2 = PointToName(pszProcess2);
	if (!pszName1 || !*pszName1 || !pszName2 || !*pszName2)
		return false;

	LPCWSTR pszExt1 = wcsrchr(pszName1, L'.');
	LPCWSTR pszExt2 = wcsrchr(pszName2, L'.');

	CEStr lsName1, lsName2;
	if (!pszExt1)
	{
		lsName1 = lstrmerge(pszName1, L".exe");
		pszName1 = lsName1;
		if (!pszName1)
			return false;
	}
	if (!pszExt2)
	{
		lsName2 = lstrmerge(pszName1, L".exe");
		pszName2 = lsName1;
		if (!pszName2)
			return false;
	}

	int iCmp = lstrcmpi(pszName1, pszName2);
	return (iCmp == 0);
}

bool CheckProcessName(LPCWSTR pszProcessName, LPCWSTR* lsNameExt, LPCWSTR* lsName)
{
	//LPCWSTR lsNameExt[] = {L"ConEmuC.exe", L"ConEmuC64.exe", L"csrss.exe", L"conhost.exe", NULL};
	//LPCWSTR lsName[] = {L"ConEmuC", L"ConEmuC64", L"csrss", L"conhost", NULL};

	LPCWSTR pszName = PointToName(pszProcessName);
	if (!pszName || !*pszName)
	{
		_ASSERTE(pszName && *pszName);
		return false;
	}

	if (wcschr(pszName, L'.'))
	{
		for (size_t i = 0; lsNameExt[i]; i++)
		{
			if (lstrcmpi(pszName, lsNameExt[i]) == 0)
				return true;
		}
	}
	else
	{
		for (size_t i = 0; lsName[i]; i++)
		{
			if (lstrcmpi(pszName, lsName[i]) == 0)
				return true;
		}
	}

	return false;
}

bool IsConsoleService(LPCWSTR pszProcessName)
{
	LPCWSTR lsNameExt[] = {L"csrss.exe", L"conhost.exe", NULL};
	LPCWSTR lsName[] = {L"csrss", L"conhost", NULL};
	return CheckProcessName(pszProcessName, lsNameExt, lsName);
}

bool IsConsoleServer(LPCWSTR pszProcessName)
{
	LPCWSTR lsNameExt[] = {L"ConEmuC.exe", L"ConEmuC64.exe", NULL};
	LPCWSTR lsName[] = {L"ConEmuC", L"ConEmuC64", NULL};
	return CheckProcessName(pszProcessName, lsNameExt, lsName);
}

bool IsGitBashHelper(LPCWSTR pszProcessName)
{
	LPCWSTR lsNameExt[] = { L"git-bash.exe", L"git-cmd.exe", NULL };
	LPCWSTR lsName[] = { L"git-bash", L"git-cmd", NULL };
	return CheckProcessName(pszProcessName, lsNameExt, lsName);
}

bool IsFarExe(LPCWSTR asModuleName)
{
	LPCWSTR lsNameExt[] = {L"far.exe", L"far64.exe", NULL};
	LPCWSTR lsName[] = {L"far", L"far64", NULL};
	return CheckProcessName(asModuleName, lsNameExt, lsName);
}

bool IsCmdProcessor(LPCWSTR asModuleName)
{
	LPCWSTR lsNameExt[] = {L"cmd.exe", L"tcc.exe", NULL};
	LPCWSTR lsName[] = {L"cmd", L"tcc", NULL};
	return CheckProcessName(asModuleName, lsNameExt, lsName);
}

bool IsQuotationNeeded(LPCWSTR pszPath)
{
	bool bNeeded = false;
	if (pszPath)
	{
		bNeeded = (wcspbrk(pszPath, QuotationNeededChars) != 0);
	}
	return bNeeded;
}

wchar_t* MergeCmdLine(LPCWSTR asExe, LPCWSTR asParams)
{
	bool bNeedQuot = IsQuotationNeeded(asExe);
	if (asParams && !*asParams)
		asParams = NULL;

	wchar_t* pszRet;
	if (bNeedQuot)
		pszRet = lstrmerge(L"\"", asExe, asParams ? L"\" " : L"\"", asParams);
	else
		pszRet = lstrmerge(asExe, asParams ? L" " : NULL, asParams);

	return pszRet;
}

wchar_t* JoinPath(LPCWSTR asPath, LPCWSTR asPart1, LPCWSTR asPart2 /*= NULL*/)
{
	LPCWSTR psz1 = asPath, psz2 = NULL, psz3 = asPart1, psz4 = NULL, psz5 = asPart2;

	// Добавить слеши если их нет на гранях
	// удалить лишние, если они указаны в обеих частях

	if (asPart1)
	{
		bool bDirSlash1  = (psz1 && *psz1) ? (psz1[lstrlen(psz1)-1] == L'\\') : false;
		bool bFileSlash1 = (asPart1[0] == L'\\');

		if (bDirSlash1 && bFileSlash1)
			psz3++;
		else if (!bDirSlash1 && !bFileSlash1 && asPath && *asPath)
			psz2 = L"\\";

		if (asPart2)
		{
			bool bDirSlash2  = (psz3 && *psz3) ? (psz3[lstrlen(psz3)-1] == L'\\') : false;
			bool bFileSlash2 = (asPart2[0] == L'\\');

			if (bDirSlash2 && bFileSlash2)
				psz5++;
			else if (!bDirSlash2 && !bFileSlash2)
				psz4 = L"\\";
		}
	}

	return lstrmerge(psz1, psz2, psz3, psz4, psz5);
}

// Первичная проверка, может ли asFilePath быть путем
bool IsFilePath(LPCWSTR asFilePath, bool abFullRequired /*= false*/)
{
	if (!asFilePath || !*asFilePath)
		return false;

	// Если в пути встречаются недопустимые символы
	if (wcschr(asFilePath, L'"') ||
	        wcschr(asFilePath, L'>') ||
	        wcschr(asFilePath, L'<') ||
	        wcschr(asFilePath, L'|')
			// '/' не проверяем для совместимости с cygwin?
	  )
		return false;

	// Пропуск UNC "\\?\"
	if (asFilePath[0] == L'\\' && asFilePath[1] == L'\\' && asFilePath[2] == L'?' && asFilePath[3] == L'\\')
		asFilePath += 4; //-V112

	// Если asFilePath содержит два (и более) ":\"
	LPCWSTR pszColon = wcschr(asFilePath, L':');

	if (pszColon)
	{
		// Если есть ":", то это должен быть путь вида "X:\xxx", т.е. ":" - второй символ
		if (pszColon != (asFilePath+1))
			return false;

		if (!isDriveLetter(asFilePath[0]))
			return false;

		if (wcschr(pszColon+1, L':'))
			return false;
	}

	if (abFullRequired)
	{
		if ((asFilePath[0] == L'\\' && asFilePath[1] == L'\\' && asFilePath[2] && wcschr(asFilePath+3,L'\\'))
				|| (isDriveLetter(asFilePath[0]) && asFilePath[1] == L':' && asFilePath[2]))
			return true;
		return false;
	}

	// May be file path
	return true;
}

const wchar_t* PointToName(const wchar_t* asFileOrPath)
{
	if (!asFileOrPath)
	{
		_ASSERTE(asFileOrPath!=NULL);
		return NULL;
	}

	// Utilize both type of slashes
	const wchar_t* pszBSlash = wcsrchr(asFileOrPath, L'\\');
	const wchar_t* pszFSlash = wcsrchr(pszBSlash ? pszBSlash : asFileOrPath, L'/');

	const wchar_t* pszFile = pszFSlash ? pszFSlash : pszBSlash;
	if (!pszFile) pszFile = asFileOrPath; else pszFile++;

	return pszFile;
}

const char* PointToName(const char* asFileOrPath)
{
	if (!asFileOrPath)
	{
		_ASSERTE(asFileOrPath!=NULL);
		return NULL;
	}

	// Utilize both type of slashes
	const char* pszBSlash = strrchr(asFileOrPath, '\\');
	const char* pszFSlash = strrchr(pszBSlash ? pszBSlash : asFileOrPath, '/');

	const char* pszSlash = pszFSlash ? pszFSlash : pszBSlash;;

	if (pszSlash)
		return pszSlash+1;

	return asFileOrPath;
}

// Возвращает ".ext" или NULL в случае ошибки
const wchar_t* PointToExt(const wchar_t* asFullPath)
{
	const wchar_t* pszName = PointToName(asFullPath);
	if (!pszName)
		return NULL; // _ASSERTE уже был
	const wchar_t* pszExt = wcsrchr(pszName, L'.');
	return pszExt;
}

// !!! Меняет asParm !!!
// Cut leading and trailing quotas
const wchar_t* Unquote(wchar_t* asParm, bool abFirstQuote /*= false*/)
{
	if (!asParm)
		return NULL;
	if (*asParm != L'"')
		return asParm;
	wchar_t* pszEndQ = abFirstQuote ? wcschr(asParm+1, L'"') : wcsrchr(asParm+1, L'"');
	if (!pszEndQ)
	{
		*asParm = 0;
		return asParm;
	}
	*pszEndQ = 0;
	return (asParm+1);
}

// Does proper "-new_console" switch exist?
bool IsNewConsoleArg(LPCWSTR lsCmdLine, LPCWSTR pszArg /*= L"-new_console"*/)
{
	if (!lsCmdLine || !*lsCmdLine || !pszArg || !*pszArg)
		return false;

	int nArgLen = lstrlen(pszArg);
	LPCWSTR pwszCopy = (wchar_t*)wcsstr(lsCmdLine, pszArg);

	// Если после -new_console идет пробел, или это вообще конец строки
	// 111211 - после -new_console: допускаются параметры
	bool bFound = (pwszCopy
		// Must be started with space or double-quote or be the start of the string
		&& ((pwszCopy == lsCmdLine) || ((*(pwszCopy-1) == L' ') || (*(pwszCopy-1) == L'"')))
		// And check the end of parameter
		&& ((pwszCopy[nArgLen] == 0) || wcschr(L" :", pwszCopy[nArgLen])
			|| ((pwszCopy[nArgLen] == L'"') || (pwszCopy[nArgLen+1] == 0))));

	return bFound;
}
