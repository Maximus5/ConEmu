
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
#include "CmdLine.h"
#include "MStrSafe.h"
#include "WinObjects.h"

// Возвращает 0, если успешно, иначе - ошибка
int NextArg(const wchar_t** asCmdLine, wchar_t (&rsArg)[MAX_PATH+1], const wchar_t** rsArgStart/*=NULL*/)
{
	if (!asCmdLine)
		return CERR_CMDLINEEMPTY;

	LPCWSTR psCmdLine = *asCmdLine, pch = NULL;
	wchar_t ch = *psCmdLine;
	size_t nArgLen = 0;
	bool lbQMode = false;

	while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);

	if (ch == 0) return CERR_CMDLINEEMPTY;

	// аргумент начинается с "
	if (ch == L'"')
	{
		lbQMode = true;
		psCmdLine++;
		pch = wcschr(psCmdLine, L'"');

		if (!pch) return CERR_CMDLINE;

		while (pch[1] == L'"')
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

	if (nArgLen > MAX_PATH) return CERR_CMDLINE;

	// Вернуть аргумент
	memcpy(rsArg, psCmdLine, nArgLen*sizeof(*rsArg));

	if (rsArgStart) *rsArgStart = psCmdLine;

	rsArg[nArgLen] = 0;
	psCmdLine = pch;
	// Finalize
	ch = *psCmdLine; // может указывать на закрывающую кавычку

	if (lbQMode && ch == L'"') ch = *(++psCmdLine);

	while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);

	*asCmdLine = psCmdLine;
	return 0;
}

//int NextArg(const char** asCmdLine, char (&rsArg)[MAX_PATH+1], const char** rsArgStart/*=NULL*/)
//{
//	LPCSTR psCmdLine = *asCmdLine, pch = NULL;
//	char ch = *psCmdLine;
//	size_t nArgLen = 0;
//	bool lbQMode = false;
//
//	while (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') ch = *(++psCmdLine);
//
//	if (ch == 0) return CERR_CMDLINEEMPTY;
//
//	// аргумент начинается с "
//	if (ch == '"')
//	{
//		lbQMode = true;
//		psCmdLine++;
//		pch = strchr(psCmdLine, '"');
//
//		if (!pch) return CERR_CMDLINE;
//
//		while (pch[1] == '"')
//		{
//			pch += 2;
//			pch = strchr(pch, '"');
//
//			if (!pch) return CERR_CMDLINE;
//		}
//
//		// Теперь в pch ссылка на последнюю "
//	}
//	else
//	{
//		// До конца строки или до первого пробела
//		//pch = wcschr(psCmdLine, ' ');
//		// 09.06.2009 Maks - обломался на: cmd /c" echo Y "
//		pch = psCmdLine;
//
//		// Ищем обычным образом (до пробела/кавычки)
//		while (*pch && *pch!=' ' && *pch!='"') pch++;
//
//		//if (!pch) pch = psCmdLine + lstrlenW(psCmdLine); // до конца строки
//	}
//
//	nArgLen = pch - psCmdLine;
//
//	if (nArgLen > MAX_PATH) return CERR_CMDLINE;
//
//	// Вернуть аргумент
//	memcpy(rsArg, psCmdLine, nArgLen*sizeof(*rsArg));
//
//	if (rsArgStart) *rsArgStart = psCmdLine;
//
//	rsArg[nArgLen] = 0;
//	psCmdLine = pch;
//	// Finalize
//	ch = *psCmdLine; // может указывать на закрывающую кавычку
//
//	if (lbQMode && ch == '"') ch = *(++psCmdLine);
//
//	while(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') ch = *(++psCmdLine);
//
//	*asCmdLine = psCmdLine;
//	return 0;
//}

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
		lstrcpyn(szDrive, pszPath, pszSlash ? min(cchDriveMax,pszSlash-pszPath+1) : cchDriveMax);
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
		lstrcpyn(szDrive, pszPath, pszSlash ? min(cchDriveMax,pszSlash-pszPath+1) : cchDriveMax);
	}
	else
	{
		lstrcpyn(szDrive, L"", cchDriveMax);
	}
	return szDrive;
}

// Команды, которые не нужно пытаться развернуть в exe?
// кроме того, если команда содержит "?" или "*" - тоже не пытаться.
const wchar_t* gsInternalCommands = L"ACTIVATE\0ALIAS\0ASSOC\0ATTRIB\0BEEP\0BREAK\0CALL\0CDD\0CHCP\0COLOR\0COPY\0DATE\0DEFAULT\0DEL\0DELAY\0DESCRIBE\0DETACH\0DIR\0DIRHISTORY\0DIRS\0DRAWBOX\0DRAWHLINE\0DRAWVLINE\0ECHO\0ECHOERR\0ECHOS\0ECHOSERR\0ENDLOCAL\0ERASE\0ERRORLEVEL\0ESET\0EXCEPT\0EXIST\0EXIT\0FFIND\0FOR\0FREE\0FTYPE\0GLOBAL\0GOTO\0HELP\0HISTORY\0IF\0IFF\0INKEY\0INPUT\0KEYBD\0KEYS\0LABEL\0LIST\0LOG\0MD\0MEMORY\0MKDIR\0MOVE\0MSGBOX\0NOT\0ON\0OPTION\0PATH\0PAUSE\0POPD\0PROMPT\0PUSHD\0RD\0REBOOT\0REN\0RENAME\0RMDIR\0SCREEN\0SCRPUT\0SELECT\0SET\0SETDOS\0SETLOCAL\0SHIFT\0SHRALIAS\0START\0TEE\0TIME\0TIMER\0TITLE\0TOUCH\0TREE\0TRUENAME\0TYPE\0UNALIAS\0UNSET\0VER\0VERIFY\0VOL\0VSCRPUT\0WINDOW\0Y\0\0";

BOOL IsNeedCmd(BOOL bRootCmd, LPCWSTR asCmdLine, LPCWSTR* rsArguments, BOOL *rbNeedCutStartEndQuot,
			   wchar_t (&szExe)[MAX_PATH+1],
			   BOOL& rbRootIsCmdExe, BOOL& rbAlwaysConfirmExit, BOOL& rbAutoDisableConfirmExit)
{
	_ASSERTE(asCmdLine && *asCmdLine);
	rbRootIsCmdExe = TRUE;

	#ifdef _DEBUG
	// Это минимальные проверки, собственно к коду - не относятся
	wchar_t szDbgFirst[MAX_PATH+1];
	bool bIsBatch = false;
	{
		LPCWSTR psz = asCmdLine;
		NextArg(&psz, szDbgFirst);
		psz = PointToExt(szDbgFirst);
		if (lstrcmpi(psz, L".cmd")==0 || lstrcmpi(psz, L".bat")==0)
			bIsBatch = true;
	}
	#endif

	memset(szExe, 0, sizeof(szExe));

	if (!asCmdLine || *asCmdLine == 0)
	{
		_ASSERTE(asCmdLine && *asCmdLine);
		return TRUE;
	}
		
	//110202 перенес вниз, т.к. это уже может быть cmd.exe, и тогда у него сносит крышу
	//// Если есть одна из команд перенаправления, или слияния - нужен CMD.EXE
	//if (wcschr(asCmdLine, L'&') ||
	//        wcschr(asCmdLine, L'>') ||
	//        wcschr(asCmdLine, L'<') ||
	//        wcschr(asCmdLine, L'|') ||
	//        wcschr(asCmdLine, L'^') // или экранирования
	//  )
	//{
	//	return TRUE;
	//}

	//wchar_t szArg[MAX_PATH+10] = {0};
	int iRc = 0;
	BOOL lbFirstWasGot = FALSE;
	LPCWSTR pwszCopy = asCmdLine;
	// cmd /c ""c:\program files\arc\7z.exe" -?"   // да еще и внутри могут быть двойными...
	// cmd /c "dir c:\"
	int nLastChar = lstrlenW(pwszCopy) - 1;

	if (pwszCopy[0] == L'"' && pwszCopy[nLastChar] == L'"')
	{
		if (pwszCopy[1] == L'"' && pwszCopy[2])
		{
			pwszCopy ++; // Отбросить первую кавычку в командах типа: ""c:\program files\arc\7z.exe" -?"

			if (rbNeedCutStartEndQuot) *rbNeedCutStartEndQuot = TRUE;
		}
		else
			// глючила на ""F:\VCProject\FarPlugin\#FAR180\far.exe  -new_console""
			//if (wcschr(pwszCopy+1, L'"') == (pwszCopy+nLastChar)) {
			//	LPCWSTR pwszTemp = pwszCopy;
			//	// Получим первую команду (исполняемый файл?)
			//	if ((iRc = NextArg(&pwszTemp, szArg)) != 0) {
			//		//Parsing command line failed
			//		return TRUE;
			//	}
			//	pwszCopy ++; // Отбросить первую кавычку в командах типа: "c:\arc\7z.exe -?"
			//	lbFirstWasGot = TRUE;
			//	if (rbNeedCutStartEndQuot) *rbNeedCutStartEndQuot = TRUE;
			//} else
		{
			// отбросить первую кавычку в: "C:\GCC\msys\bin\make.EXE -f "makefile" COMMON="../../../plugins/common""
			LPCWSTR pwszTemp = pwszCopy + 1;

			// Получим первую команду (исполняемый файл?)
			if ((iRc = NextArg(&pwszTemp, szExe)) != 0)
			{
				//Parsing command line failed
				#ifdef WARN_NEED_CMD
				_ASSERTE(FALSE);
				#endif
				return TRUE;
			}
			
			if (lstrcmpiW(szExe, L"start") == 0)
			{
				// Команду start обрабатывает только процессор
				#ifdef WARN_NEED_CMD
				_ASSERTE(FALSE);
				#endif
				return TRUE;
			}

			LPCWSTR pwszQ = pwszCopy + 1 + lstrlen(szExe);
			wchar_t* pszExpand = NULL;

			if (*pwszQ != L'"' && IsExecutable(szExe, &pszExpand))
			{
				pwszCopy ++; // отбрасываем
				lbFirstWasGot = TRUE;

				if (pszExpand)
				{
					lstrcpyn(szExe, pszExpand, countof(szExe));
					SafeFree(pszExpand);
					if (rsArguments)
						*rsArguments = pwszQ;
				}

				if (rbNeedCutStartEndQuot) *rbNeedCutStartEndQuot = TRUE;
			}
		}
	}

	// Получим первую команду (исполняемый файл?)
	if (!lbFirstWasGot)
	{
		szExe[0] = 0;
		// 17.10.2010 - поддержка переданного исполняемого файла без параметров, но с пробелами в пути
		LPCWSTR pchEnd = pwszCopy + lstrlenW(pwszCopy);

		while (pchEnd > pwszCopy && *(pchEnd-1) == L' ') pchEnd--;

		if ((pchEnd - pwszCopy) < MAX_PATH)
		{
			memcpy(szExe, pwszCopy, (pchEnd - pwszCopy)*sizeof(wchar_t));
			szExe[(pchEnd - pwszCopy)] = 0;

			if (lstrcmpiW(szExe, L"start") == 0)
			{
				// Команду start обрабатывает только процессор
				#ifdef WARN_NEED_CMD
				_ASSERTE(FALSE);
				#endif
				return TRUE;
			}

			// Обработка переменных окружения и поиск в PATH
			if (FileExistsSearch(/*IN|OUT*/szExe, countof(szExe)))
			{
				if (rsArguments)
					*rsArguments = pchEnd;
			}
			else
			{
				szExe[0] = 0;
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
				return TRUE;
			}

			if (lstrcmpiW(szExe, L"start") == 0)
			{
				// Команду start обрабатывает только процессор
				#ifdef WARN_NEED_CMD
				_ASSERTE(FALSE);
				#endif
				return TRUE;
			}

			// Обработка переменных окружения и поиск в PATH
			if (FileExistsSearch(/*IN|OUT*/szExe, countof(szExe)))
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
			return TRUE; // добавить "cmd.exe"
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
					return TRUE; // добавить "cmd.exe"
				}
			}
			
			// Пробуем найти "по путям" соответствующий exe-шник.
			DWORD nCchMax = countof(szExe); // выделить память, длинее чем szExe вернуть не сможем
			wchar_t* pszSearch = (wchar_t*)malloc(nCchMax*sizeof(wchar_t));
			if (pszSearch)
			{
				#ifndef CONEMU_MINIMAL
				MWow64Disable wow; wow.Disable(); // Отключить редиректор!
				#endif
				wchar_t *pszName = NULL;
				DWORD nRc = SearchPath(NULL, szExe, bHasExt ? NULL : L".exe", nCchMax, pszSearch, &pszName);
				if (nRc && (nRc < nCchMax))
				{
					// Нашли, возвращаем что нашли
					wcscpy_c(szExe, pszSearch);
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
		return TRUE; // добавить "cmd.exe"
	}

	//pwszCopy = wcsrchr(szArg, L'\\'); if (!pwszCopy) pwszCopy = szArg; else pwszCopy ++;
	pwszCopy = PointToName(szExe);
	//2009-08-27
	wchar_t *pwszEndSpace = szExe + lstrlenW(szExe) - 1;

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
		return FALSE; // уже указан командный процессор, cmd.exe в начало добавлять не нужно
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
			return TRUE;
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
			return FALSE; // уже указан исполняемый файл, cmd.exe в начало добавлять не нужно
		}
	}

	if (IsExecutable(szExe))
	{
		rbRootIsCmdExe = FALSE; // Для других программ - буфер не включаем
		_ASSERTE(!bIsBatch);
		return FALSE; // Запускается конкретная консольная программа. cmd.exe не требуется
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
	return TRUE;
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

bool IsFarExe(LPCWSTR asModuleName)
{
	if (asModuleName && *asModuleName)
	{
		LPCWSTR pszName = PointToName(asModuleName);
		if (lstrcmpi(pszName, L"far.exe") == 0 || lstrcmpi(pszName, L"far") == 0
			|| lstrcmpi(pszName, L"far64.exe") == 0 || lstrcmpi(pszName, L"far64") == 0)
		{
			return true;
		}
	}
	return false;
}
