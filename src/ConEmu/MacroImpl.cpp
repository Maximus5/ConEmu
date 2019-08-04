
/*
Copyright (c) 2011-present Maximus5
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
#include "MacroImpl.h"
#include "../common/MStrEsc.h"

// bAutoString==true : auto convert of gmt_String to gmt_VString
wchar_t* GuiMacro::AsString()
{
	if (!szFunc || !*szFunc)
	{
		_ASSERTE(szFunc && *szFunc);
		return lstrdup(L"");
	}

	// Можно использовать EscapeChar, только нужно учесть,
	// что кавычки ВНУТРИ Arg.Str нужно эскейпить...

	size_t cchTotal = _tcslen(szFunc) + 5;

	wchar_t** pszArgs = NULL;

	if (argc > 0)
	{
		wchar_t szNum[12];
		pszArgs = (wchar_t**)calloc(argc, sizeof(wchar_t*));
		if (!pszArgs)
		{
			_ASSERTE(FALSE && "Memory allocation failed");
			return lstrdup(L"");
		}

		for (size_t i = 0; i < argc; i++)
		{
			switch (argv[i].Type)
			{
			case gmt_Int:
				swprintf_c(szNum, L"%i", argv[i].Int);
				pszArgs[i] = lstrdup(szNum);
				cchTotal += _tcslen(szNum)+1;
				break;
			case gmt_Hex:
				swprintf_c(szNum, L"0x%08X", (DWORD)argv[i].Int);
				pszArgs[i] = lstrdup(szNum);
				cchTotal += _tcslen(szNum)+1;
				break;
			case gmt_Str:
				// + " ... ", + escaped (4x len in maximum for "\xFF" form)
				pszArgs[i] = (wchar_t*)malloc((3+(argv[i].Str ? _tcslen(argv[i].Str)*4 : 0))*sizeof(*argv[i].Str));
				if (pszArgs[i])
				{
					LPCWSTR pszSrc = argv[i].Str;
					LPWSTR pszDst = pszArgs[i];
					// Start
					*(pszDst++) = L'"';
					// Loop
					while (*pszSrc)
					{
						EscapeChar(true, pszSrc, pszDst);
					}
					// Done
					*(pszDst++) = L'"'; *(pszDst++) = 0;
					cchTotal += _tcslen(pszArgs[i])+1;
				}
				break;
			case gmt_VStr:
				// + @" ... ", + escaped (double len in maximum)
				pszArgs[i] = (wchar_t*)malloc((4+(argv[i].Str ? _tcslen(argv[i].Str)*2 : 0))*sizeof(*argv[i].Str));
				if (pszArgs[i])
				{
					LPCWSTR pszSrc = argv[i].Str;
					LPWSTR pszDst = pszArgs[i];
					// Start
					*(pszDst++) = L'@'; *(pszDst++) = L'"';
					// Loop
					while (*pszSrc)
					{
						if (*pszSrc == L'"')
							*(pszDst++) = L'"';
						*(pszDst++) = *(pszSrc++);
					}
					// Done
					*(pszDst++) = L'"'; *(pszDst++) = 0;
					cchTotal += _tcslen(pszArgs[i])+1;
				}
				break;
			default:
				_ASSERTE(FALSE && "Argument type not supported");
			}
		}
	}

	wchar_t* pszBuf = (wchar_t*)malloc(cchTotal*sizeof(*pszBuf));
	if (!pszBuf)
	{
		for (size_t i = 0; i < argc; i++)
		{
			SafeFree(pszArgs[i]);
		}
		SafeFree(pszArgs);
		_ASSERTE(FALSE && "Memory allocation failed");
		return lstrdup(L"");
	}

	wchar_t* pszDst = pszBuf;
	_wcscpy_c(pszDst, cchTotal, szFunc);
	pszDst += _tcslen(pszDst);
	//if (argc == 0)
	//	goto done;

	*(pszDst++) = L'(';

	for (size_t i = 0; i < argc; i++)
	{
		if (i)
			*(pszDst++) = L',';

		if (pszArgs[i])
		{
			_wcscpy_c(pszDst, cchTotal, pszArgs[i]);
			pszDst += _tcslen(pszDst);
			free(pszArgs[i]);
		}
	}

	// Done
	*(pszDst++) = L')';
//done:
	*(pszDst++) = 0;
	#ifdef _DEBUG
	size_t cchUsed = (pszDst - pszBuf);
	_ASSERTE(cchUsed <= cchTotal);
	#endif


	if (pszArgs)
	{
		free(pszArgs);
	}
	return pszBuf;
}

bool GuiMacro::GetIntArg(size_t idx, int& val)
{
	if ((idx >= argc)
		|| (argv[idx].Type != gmt_Int && argv[idx].Type != gmt_Hex))
	{
		val = 0;
		return false;
	}

	val = argv[idx].Int;
	return true;
}

bool GuiMacro::GetStrArg(size_t idx, LPWSTR& val)
{
	// It it is detected as "Int" but we need "Str" - return it as string
	if ((idx >= argc)
		|| (argv[idx].Type != gmt_Str && argv[idx].Type != gmt_VStr && argv[idx].Type != gmt_Int))
	{
		val = NULL;
		return false;
	}

	val = argv[idx].Str;
	return (val != NULL);
}

bool GuiMacro::GetRestStrArgs(size_t idx, LPWSTR& val)
{
	if (!GetStrArg(idx, val))
		return false;
	if (!IsStrArg(idx+1))
		return true; // No more string arguments!
	// Concatenate string (memory was allocated continuously for all arguments)
	LPWSTR pszEnd = val + _tcslen(val);
	for (size_t i = idx+1; i < argc; i++)
	{
		LPWSTR pszAdd;
		if (!GetStrArg(i, pszAdd))
			break;
		size_t iLen = _tcslen(pszAdd);
		if (pszEnd < pszAdd)
			wmemmove(pszEnd, pszAdd, iLen+1);
		pszEnd += iLen;
	}
	// Trim argument list, there is only one returned string left...
	argc = idx+1;
	return true;
}

bool GuiMacro::IsIntArg(size_t idx)
{
	if ((idx >= argc)
		|| (argv[idx].Type != gmt_Int && argv[idx].Type != gmt_Hex))
	{
		return false;
	}
	return true;
}

bool GuiMacro::IsStrArg(size_t idx)
{
	if ((idx >= argc)
		|| (argv[idx].Type != gmt_Str && argv[idx].Type != gmt_VStr))
	{
		return false;
	}
	return true;
}



/* ************************* */
/* ****** Realization ****** */
/* ************************* */
namespace ConEmuMacro {

GuiMacro* GetNextMacro(LPWSTR& asString, bool abConvert, wchar_t** rsErrMsg)
{
	// Skip white-spaces
	SkipWhiteSpaces(asString);

	if (!asString || !*asString)
	{
		if (rsErrMsg)
			*rsErrMsg = lstrdup(L"Empty Macro");
		return NULL;
	}

	if (*asString == L'"')
	{
		if (rsErrMsg)
		{
			*rsErrMsg = lstrmerge(
				L"Invalid Macro, remove surrounding quotes:\r\n",
				asString,
				NULL);
		}
		return NULL;
	}

	// Extract function name

	wchar_t szFunction[64] = L"", chTerm = 0;
	bool lbFuncOk = false;

	for (size_t i = 0; i < (countof(szFunction)-1); i++)
	{
		chTerm = asString[i];
		szFunction[i] = chTerm;

		if (chTerm < L' ' || chTerm > L'z')
		{
			if (chTerm == 0)
			{
				lbFuncOk = (i > 0);
				szFunction[i] = 0;
				asString = asString + i;
			}

			break;
		}

		// Don't use ‘switch(){}’ here, because of further break-s
		if (chTerm == L':' || chTerm == L'(' || chTerm == L' ')
		{
			// Skip white-spaces
			if (chTerm == L':' || chTerm == L'(')
			{
				asString = asString+i+1;

				SkipWhiteSpaces(asString);
			}
			else
			{
				asString = asString+i;

				SkipWhiteSpaces(asString);

				if (*asString == L'(')
				{
					chTerm = *asString;
					asString++;
				}
			}

			lbFuncOk = true;
			szFunction[i] = 0;
			break;
		}
		else if (chTerm == L';')
		{
			// Function without any args
			lbFuncOk = (i > 0);
			szFunction[i] = 0;
			asString = asString + i;
			break;
		}
	}

	if (!lbFuncOk)
	{
		if (chTerm || (!asString || (*asString != 0)))
		{
			_ASSERTE(chTerm || (asString && (*asString == 0)));
			if (rsErrMsg)
			{
				*rsErrMsg = lstrmerge(
					L"Invalid Macro, can't get function name:\r\n",
					asString,
					NULL);
			}
			return NULL;
		}

		if (rsErrMsg)
		{
			*rsErrMsg = lstrmerge(
				L"Invalid Macro, too long function name:\r\n",
				asString,
				NULL);
		}
		return NULL;
	}

	// Подготовить аргументы (отрезать закрывающую скобку)
	bool bEndBracket = (chTerm == L'(');

	// Аргументы
	MArray<GuiMacroArg> Args;
	size_t cbAddSize = 0;
	wchar_t szArgErr[32] = {};

	while (*asString)
	{
		GuiMacroArg a = {};

		// Пропустить white-space
		SkipWhiteSpaces(asString);

		// Last arg?
		if (!*asString
			|| (bEndBracket && (asString[0] == L')'))
			|| (!bEndBracket && (asString[0] == L';')))
		{
			// Skip macro end token ans WhiteSpaces
			while (*asString && wcschr(L"); \t\r\n", *asString))
				asString++;
			// OK
			break;
		}

		if ((asString[0] == L'-' || asString[0] == L'/') && (lstrcmpni(asString+1, L"GuiMacro", 8) == 0))
		{
			asString += 9;
			SkipWhiteSpaces(asString);
			// That will be next macro
			break;
		}

		bool bIntArg = false;
		if (isDigit(asString[0]) || (asString[0] == L'-') || (asString[0] == L'+'))
		{
			a.Type = (asString[0] == L'0' && (asString[1] == L'x' || asString[1] == L'X')) ? gmt_Hex : gmt_Int;

			// If it fails - just use as string
			bIntArg = (GetNextInt(asString, a) != NULL);
		}

		if (!bIntArg)
		{
			// Delimiter was ':' and argument was specified without quotas - return it "as is" to the end of macro string
			// Otherwise, if there are no ‘"’ or ‘@"’ - use powershell style (one word == one string arg)

			bool lbCvtVbt = false;
			a.Type = (asString[0] == L'@' && asString[1] == L'"') ? gmt_VStr : gmt_Str;

			_ASSERTE(asString[0]!=L':');

			if (abConvert && (asString[0] == L'"'))
			{
				// Старые макросы хранились как "Verbatim" но без префикса
				*(--asString) = L'@';
				lbCvtVbt = true;
				a.Type = gmt_VStr;
			}



			if (!GetNextString(asString, a.Str, (chTerm == L':'), bEndBracket))
			{
				swprintf_c(szArgErr, L"Argument #%u failed (string)", Args.size()+1);
				if (rsErrMsg)
					*rsErrMsg = lstrdup(szArgErr);
				return NULL;
			}

			if (lbCvtVbt)
			{
				_ASSERTE(a.Type == gmt_VStr);
				// Если строка содержит из спец-символов ТОЛЬКО "\" (пути)
				// то ее удобнее показывать как "Verbatim", иначе - C-String
				// Однозначно показывать как C-String нужно те строки, которые
				// содержат переводы строк, Esc, табуляции и пр.
				bool bSlash = false, bEsc = false;
				CheckStrForSpecials(a.Str, &bSlash, &bEsc);
				if (!(bEsc || bSlash) || bEsc)
					a.Type = gmt_Str;
			}

			cbAddSize += (_tcslen(a.Str)+1) * sizeof(*a.Str);
		}

		Args.push_back(a);
	}

	// Создать GuiMacro*
	size_t cbAllSize = sizeof(GuiMacro)
		+ (_tcslen(szFunction)+1) * sizeof(*szFunction)
		+ Args.size() * sizeof(GuiMacroArg)
		+ cbAddSize;
	GuiMacro* pMacro = (GuiMacro*)malloc(cbAllSize);
	if (!pMacro)
	{
		_ASSERTE(pMacro != NULL);
		return NULL;
	}

	pMacro->cbSize = cbAllSize;

	LPBYTE ptrTail = (LPBYTE)(pMacro + 1);

	// Fill GuiMacro*

	// Function name
	size_t cchLen = _tcslen(szFunction)+1;
	_wcscpy_c((wchar_t*)ptrTail, cchLen, szFunction);
	pMacro->szFunc = (wchar_t*)ptrTail;
	ptrTail += cchLen * sizeof(*pMacro->szFunc);

	pMacro->chFuncTerm = chTerm;

	pMacro->argc = Args.size();
	pMacro->argv = (GuiMacroArg*)ptrTail;
	ptrTail += Args.size() * (sizeof(GuiMacroArg));

	for (size_t i = 0; i < pMacro->argc; i++)
	{
		pMacro->argv[i] = Args[i];
		if (pMacro->argv[i].Type == gmt_Int || pMacro->argv[i].Type == gmt_Hex)
		{
			// Дополнительная память не выделяется
		}
		else if (pMacro->argv[i].Type == gmt_Str || pMacro->argv[i].Type == gmt_VStr)
		{
			_ASSERTE(pMacro->argv[i].Str);
			cchLen = _tcslen(pMacro->argv[i].Str)+1;
			_wcscpy_c((wchar_t*)ptrTail, cchLen, pMacro->argv[i].Str);
			pMacro->argv[i].Str = (wchar_t*)ptrTail;
			ptrTail += cchLen * sizeof(*pMacro->argv[i].Str);
		}
		else
		{
			_ASSERTE(FALSE && "TODO: Copy memory contents!");
		}
	}

	#ifdef _DEBUG
	size_t cbUsedSize = ptrTail - (LPBYTE)pMacro;
	_ASSERTE(cbUsedSize <= cbAllSize);
	#endif

	// Done
	return pMacro;
}



// Helper to concatenate macros.
// They must be ‘concatenatable’, example: Print("abc"); Print("Qqq")
// But following WILL NOT work: Print: Abd qqq
LPCWSTR ConcatMacro(LPWSTR& rsMacroList, LPCWSTR asAddMacro)
{
	if (!asAddMacro || !*asAddMacro)
	{
		return rsMacroList;
	}

	if (!rsMacroList || !*rsMacroList)
	{
		SafeFree(rsMacroList);
		rsMacroList = lstrdup(asAddMacro);
	}
	else
	{
		lstrmerge(&rsMacroList, L"; ", asAddMacro);
	}

	return rsMacroList;
}


void SkipWhiteSpaces(LPWSTR& rsString)
{
	if (!rsString)
	{
		_ASSERTE(rsString!=NULL);
		return;
	}

	// Skip white-spaces
	while (*rsString == L' ' || *rsString == L'\t' || *rsString == L'\r' || *rsString == L'\n')
		rsString++;
}

/* ***  Функции для парсера параметров  *** */

// Get next numerical argument (dec or hex)
// Delimiter - comma or space
LPWSTR GetNextInt(LPWSTR& rsArguments, GuiMacroArg& rnValue)
{
	LPWSTR pszArg = NULL, pszEnd = NULL;
	rnValue.Str = rsArguments;
	rnValue.Int = 0; // Reset

	SkipWhiteSpaces(rsArguments);

	if (!rsArguments || !*rsArguments)
		return NULL;

	// Результат
	pszArg = rsArguments;

	#ifdef _DEBUG
	// Returns all digits
	LPCWSTR pszTestEnd = rsArguments;
	if (*pszTestEnd == L'-' || *pszTestEnd == L'+')
		pszTestEnd++;
	if (pszTestEnd[0] == L'0' && (pszTestEnd[1] == L'x' || pszTestEnd[1] == L'X'))
	{
		pszTestEnd += 2;
		while (isHexDigit(*pszTestEnd))
			pszTestEnd++;
	} else {
		while (isDigit(*pszTestEnd))
			pszTestEnd++;
	}
	#endif

	// TODO: Support 64-bit integers?

	// Hex value?
	if (pszArg[0] == L'0' && (pszArg[1] == L'x' || pszArg[1] == L'X'))
		rnValue.Int = wcstol(pszArg+2, &pszEnd, 16);
	else
		rnValue.Int = wcstol(pszArg, &pszEnd, 10);

	_ASSERTE(pszEnd == pszTestEnd);

	if (pszEnd && *pszEnd)
	{
		// Check, if next symbol is correct delimiter
		if (wcschr(L" \t\r\n),;", *pszEnd) == NULL)
		{
			// We'll use it as string
			return NULL;
		}
		rsArguments = pszEnd;
		SkipWhiteSpaces(rsArguments);
		if (*rsArguments == L',')
			rsArguments++;
	}
	else
	{
		rsArguments = pszEnd;
	}
	return pszArg;
}

// Получить следующий строковый параметр
LPWSTR GetNextString(LPWSTR& rsArguments, LPWSTR& rsString, bool bColonDelim, bool& bEndBracket)
{
	rsString = NULL; // Сброс

	if (!rsArguments || !*rsArguments)
		return NULL;

	#ifdef _DEBUG
	LPWSTR pszArgStart = rsArguments;
	LPWSTR pszArgEnd = rsArguments+_tcslen(rsArguments);
	#endif

	// Пропустить white-space
	SkipWhiteSpaces(rsArguments);

	LPCWSTR pszSrc = NULL;

	// C-style strings
	if (rsArguments[0] == L'"')
	{
		rsString = rsArguments+1;
		pszSrc = rsString;
		LPWSTR pszDst = rsString;
		while (*pszSrc && (*pszSrc != L'"'))
		{
			EscapeChar(false, pszSrc, pszDst);
		}
		_ASSERTE((*pszSrc == L'"') || (*pszSrc == 0));
		rsArguments = (wchar_t*)((*pszSrc == L'"') ? (pszSrc+1) : pszSrc);
		_ASSERTE(rsArguments>pszDst || (rsArguments==pszDst && *rsArguments==0));
		*pszDst = 0;
	}
	// C-style strings, but quotes are escaped by '\' (may come from ConEmu -GuiMacro "...")
	else if (rsArguments[0] == L'\\' && rsArguments[1] == L'"')
	{
		rsString = rsArguments+2;
		pszSrc = rsString;
		LPWSTR pszDst = rsString;
		wchar_t szTemp[2], *pszTemp;
		while (*pszSrc && (*pszSrc != L'"'))
		{
			if ((pszSrc[0] == L'\\') && pszSrc[1])
			{
				if (pszSrc[1] == L'"')
				{
					pszSrc++;
					break;
				}
				else if (pszSrc[2])
				{
					DEBUGTEST(LPCWSTR pszStart = pszSrc);
					pszTemp = szTemp;
					EscapeChar(false, pszSrc, pszTemp);
					EscapeChar(false, pszSrc, pszTemp);
					_ASSERTE((pszTemp==(szTemp+2)) && (pszSrc==(pszStart+3) || pszSrc==(pszStart+4)));
					pszTemp = szTemp;
					LPCWSTR pszReent = szTemp;
					EscapeChar(false, pszReent, pszDst);
				}
				else
				{
					EscapeChar(false, pszSrc, pszTemp);
				}
			}
			else
			{
				EscapeChar(false, pszSrc, pszDst);
			}
		}
		_ASSERTE((*pszSrc == L'"') || (*pszSrc == 0));
		rsArguments = (wchar_t*)((*pszSrc == L'"') ? (pszSrc+1) : pszSrc);
		_ASSERTE(rsArguments>pszDst || (rsArguments==pszDst && *rsArguments==0));
		*pszDst = 0;
	}
	// "verbatim string"
	else if ((rsArguments[0] == L'@') && (rsArguments[1] == L'"'))
	{
		rsString = rsArguments+2;

		pszSrc = rsString;
		LPWSTR pszDst = rsString;
		while (*pszSrc)
		{
			if (pszSrc[0] == L'"')
			{
				if (pszSrc[1] != L'"')
				{
					break;
				}
				else
				{
					pszSrc++;
				}
			}

			*(pszDst++) = *(pszSrc++);
		}
		_ASSERTE((*pszSrc == L'"') || (*pszSrc == 0));
		rsArguments = (wchar_t*)((*pszSrc == L'"') ? (pszSrc+1) : pszSrc);
		_ASSERTE(rsArguments>pszDst || (rsArguments==pszDst && *rsArguments==0));
		*pszDst = 0;
	}
	else if (!bColonDelim)
	{
		// Powershell style (one word == one string arg)
		_ASSERTE(*rsArguments != L' ');
		rsString = rsArguments;
		rsArguments = wcspbrk(rsString+1, bEndBracket ? L" )" : L" ");
		if (rsArguments)
		{
			if (*rsArguments == L')')
			{
				// Print(abc);Print("def")
				*rsArguments = 0;
				if (*(rsArguments+1))
				{
					rsArguments++;
					bEndBracket = false;
				}
			}
			else if (*(rsArguments-1) == L';')
			{
				*(rsArguments-1) = 0;
				*rsArguments = L';'; // replace space with function-delimiter
			}
			else
			{
				*(rsArguments++) = 0;
			}
		}
		else
		{
			rsArguments = rsString + _tcslen(rsString);
		}
	}
	else
	{
		#if !defined(GOOGLETEST)
		_ASSERTE(bColonDelim && "String without quotas!");
		#endif
		rsString = rsArguments;
		rsArguments = rsArguments + _tcslen(rsArguments);
		goto wrap;
	}

	if (*rsArguments)
	{
		_ASSERTE(rsArguments>=pszArgStart && rsArguments<pszArgEnd);

		SkipWhiteSpaces(rsArguments);
		if (*rsArguments == L',')
			rsArguments++;

		_ASSERTE(rsArguments>=pszArgStart && rsArguments<=pszArgEnd);
	}

wrap:
	// Тут уже NULL-а не будет, допускаются пустые строки ("")
	_ASSERTE(rsString!=NULL);
	return rsString;
}

}  // namespace ConEmuMacro