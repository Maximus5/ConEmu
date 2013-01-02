
/*
Copyright (c) 2011 Maximus5
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
#include "RealConsole.h"
#include "VirtualConsole.h"
#include "Options.h"
#include "TrayIcon.h"
#include "ConEmu.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "ConEmuPipe.h"
#include "Macro.h"
#include "VConGroup.h"
#include "Menu.h"


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
				_wsprintf(szNum, SKIPLEN(countof(szNum)) L"%i", argv[i].Int);
				pszArgs[i] = lstrdup(szNum);
				cchTotal += _tcslen(szNum)+1;
				break;
			case gmt_Hex:
				_wsprintf(szNum, SKIPLEN(countof(szNum)) L"0x%08X", (DWORD)argv[i].Int);
				pszArgs[i] = lstrdup(szNum);
				cchTotal += _tcslen(szNum)+1;
				break;
			case gmt_Str:
				// + " ... ", + escaped (double len in maximum)
				pszArgs[i] = (wchar_t*)malloc((4+(argv[i].Str ? _tcslen(argv[i].Str) : 0))*sizeof(*argv[i].Str));
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
				pszArgs[i] = (wchar_t*)malloc((4+(argv[i].Str ? _tcslen(argv[i].Str) : 0))*sizeof(*argv[i].Str));
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
	if ((argc < idx)
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
	if ((argc < idx)
		|| (argv[idx].Type != gmt_Str && argv[idx].Type != gmt_VStr))
	{
		val = NULL;
		return false;
	}

	val = argv[idx].Str;
	return true;
}

bool GuiMacro::IsIntArg(size_t idx)
{
	if ((argc < idx)
		|| (argv[idx].Type != gmt_Int && argv[idx].Type != gmt_Hex))
	{
		return false;
	}
	return true;
}

bool GuiMacro::IsStrArg(size_t idx)
{
	if ((argc < idx)
		|| (argv[idx].Type != gmt_Str && argv[idx].Type != gmt_VStr))
	{
		return false;
	}
	return true;
}


LPWSTR CConEmuMacro::ConvertMacro(LPCWSTR asMacro, BYTE FromVersion, bool bShowErrorTip /*= true*/)
{
	_ASSERTE(FromVersion == 0);

	if (!asMacro || !*asMacro)
	{
		return lstrdup(L"");
	}

	wchar_t* pszBuf = lstrdup(asMacro);
	if (!pszBuf)
	{
		_ASSERTE(FALSE && "Memory allocation failed");
		return NULL;
	}

	wchar_t* pszMacro = pszBuf;
	wchar_t* pszError = NULL;
	GuiMacro* pMacro = GetNextMacro(pszMacro, true, &pszError);
	free(pszBuf);

	if (!pMacro)
	{
		if (bShowErrorTip)
		{
			LPCWSTR pszPrefix = L"Failed to convert macro\n";
			size_t cchMax = _tcslen(pszPrefix) + (pszError ? _tcslen(pszError) : 0) + _tcslen(asMacro) + 6;
			wchar_t* pszFull = (wchar_t*)malloc(cchMax*sizeof(pszFull));
			if (pszFull)
			{
				_wcscpy_c(pszFull, cchMax, pszPrefix);
				_wcscat_c(pszFull, cchMax, pszError);
				_wcscat_c(pszFull, cchMax, L"\n");
				_wcscat_c(pszFull, cchMax, asMacro);

				Icon.ShowTrayIcon(pszFull, tsa_Config_Error);

				SafeFree(pszFull);
			}
		}
		SafeFree(pszError);

		// Return unchanged macro-string
		return lstrdup(asMacro);
	}

	wchar_t* pszCvt = pMacro->AsString();
	if (!pszCvt)
	{
		_ASSERTE(FALSE && "pMacro->AsString failed!");
		pszCvt = lstrdup(asMacro);
	}
	free(pMacro);

	return pszCvt;
}

GuiMacro* CConEmuMacro::GetNextMacro(LPWSTR& asString, bool abConvert, wchar_t** rsErrMsg)
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
			*rsErrMsg = lstrdup(L"Invalid Macro (remove quotas)");
		return NULL;
	}

	// Extract function name

	wchar_t szFunction[64], chTerm = 0;
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
				asString = asString + i;
			}

			break;
		}

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
	}

	if (!lbFuncOk)
	{
		if (chTerm || (!asString || (*asString != 0)))
		{
			_ASSERTE(chTerm || (asString && (*asString == 0)));
			if (rsErrMsg)
				*rsErrMsg = lstrdup(L"Can't get function name");
			return NULL;
		}

		if (rsErrMsg)
			*rsErrMsg = lstrdup(L"Too long function name");
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
			if (*asString)
				asString++;
			// OK
			break;
		}

		if (isDigit(asString[0]) || (asString[0] == L'-') || (asString[0] == L'+'))
		{
			a.Type = (asString[0] == L'0' && (asString[1] == L'x' || asString[1] == L'X')) ? gmt_Hex : gmt_Int;

			if (!GetNextInt(asString, a.Int))
			{
				_wsprintf(szArgErr, SKIPLEN(countof(szArgErr)) L"Argument #%u failed (int)", Args.size()+1);
				break;
			}
		}
		else 
		{
			bool lbCvtVbt = false;
			a.Type = (asString[0] == L'@' && asString[1] == L'"') ? gmt_VStr : gmt_Str;

			_ASSERTE((asString[0] == L'"') || (asString[0] == L'@' && asString[1] == L'"') || (chTerm == L':'));

			if (abConvert && (asString[0] == L'"'))
			{
				// Старые макросы хранились как "Verbatim" но без префикса
				*(--asString) = L'@';
				lbCvtVbt = true;
				a.Type = gmt_VStr;
			}

			

			if (!GetNextString(asString, a.Str, (chTerm == L':')))
			{
				_wsprintf(szArgErr, SKIPLEN(countof(szArgErr)) L"Argument #%u failed (string)", Args.size()+1);
				break;
			}

			if (lbCvtVbt)
			{
				_ASSERTE(a.Type == gmt_VStr);
				// Если строка содержит из спец-символов ТОЛЬКО "\" (пути)
				// то ее удобнее показывать как "Verbatim", иначе - C-String
				// Однозначно показывать как C-String нужно те строки, которые
				// содержат переводы строк, Esc, табуляции и пр.
				_ASSERTE(gsEscaped[0] == L'\\');
				bool bSlash = (wcschr(a.Str, L'\\') != NULL);
				bool bEsc = (wcspbrk(a.Str, gsEscaped+1) != NULL) || (wcschr(a.Str, L'"') != NULL);
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


// Общая функция, для обработки любого известного макроса
LPWSTR CConEmuMacro::ExecuteMacro(LPWSTR asMacro, CRealConsole* apRCon)
{
	wchar_t* pszErr = NULL;
	GuiMacro* p = GetNextMacro(asMacro, false, &pszErr);
	if (!p)
	{
		return pszErr ? pszErr : lstrdup(L"Invalid Macro");
	}

	LPWSTR pszAllResult = NULL;

	while (p)
	{
		LPWSTR pszResult = NULL;
		LPCWSTR szFunction = p->szFunc;

		// Поехали
		if (!lstrcmpi(szFunction, L"IsConEmu"))
			pszResult = IsConEmu(p, apRCon);
		else if (!lstrcmpi(szFunction, L"Close"))
			pszResult = Close(p, apRCon);
		else if (!lstrcmpi(szFunction, L"FindEditor"))
			pszResult = FindEditor(p, apRCon);
		else if (!lstrcmpi(szFunction, L"FindViewer"))
			pszResult = FindViewer(p, apRCon);
		else if (!lstrcmpi(szFunction, L"FindFarWindow"))
			pszResult = FindFarWindow(p, apRCon);
		else if (!lstrcmpi(szFunction, L"WindowFullscreen"))
			pszResult = WindowFullscreen(p, apRCon);
		else if (!lstrcmpi(szFunction, L"WindowMaximize"))
			pszResult = WindowMaximize(p, apRCon);
		else if (!lstrcmpi(szFunction, L"WindowMinimize"))
			pszResult = WindowMinimize(p, apRCon);
		else if (!lstrcmpi(szFunction, L"WindowMode"))
			pszResult = WindowMode(p, apRCon);
		else if (!lstrcmpi(szFunction, L"MsgBox"))
			pszResult = MsgBox(p, apRCon);
		else if (!lstrcmpi(szFunction, L"Menu"))
			pszResult = Menu(p, apRCon);
		else if (!lstrcmpi(szFunction, L"FontSetSize"))
			pszResult = FontSetSize(p, apRCon);
		else if (!lstrcmpi(szFunction, L"FontSetName"))
			pszResult = FontSetName(p, apRCon);
		else if (!lstrcmpi(szFunction, L"IsRealVisible"))
			pszResult = IsRealVisible(p, apRCon);
		else if (!lstrcmpi(szFunction, L"IsConsoleActive"))
			pszResult = IsConsoleActive(p, apRCon);
		else if (!lstrcmpi(szFunction, L"Paste"))
			pszResult = Paste(p, apRCon);
		else if (!lstrcmpi(szFunction, L"Print"))
			pszResult = Print(p, apRCon);
		else if (!lstrcmpi(szFunction, L"Progress"))
			pszResult = Progress(p, apRCon);
		else if (!lstrcmpi(szFunction, L"Rename"))
			pszResult = Rename(p, apRCon);
		else if (!lstrcmpi(szFunction, L"Shell") || !lstrcmpi(szFunction, L"ShellExecute"))
			pszResult = Shell(p, apRCon);
		else if (!lstrcmpi(szFunction, L"Select"))
			pszResult = Select(p, apRCon);
		else if (!lstrcmpi(szFunction, L"SetOption"))
			pszResult = SetOption(p, apRCon);
		else if (!lstrcmpi(szFunction, L"Tab") || !lstrcmpi(szFunction, L"Tabs") || !lstrcmpi(szFunction, L"TabControl"))
			pszResult = Tab(p, apRCon);
		else if (!lstrcmpi(szFunction, L"Task"))
			pszResult = Task(p, apRCon);
		else if (!lstrcmpi(szFunction, L"Status") || !lstrcmpi(szFunction, L"StatusBar") || !lstrcmpi(szFunction, L"StatusControl"))
			pszResult = Status(p, apRCon);
		else
			pszResult = lstrdup(L"UnknownMacro"); // Неизвестная функция

		if (pszResult == NULL)
		{
			_ASSERTE(FALSE && "MacroFunction must returns anything");
			pszResult = lstrdup(L"");
		}

		if (!pszAllResult)
		{
			pszAllResult = pszResult;
		}
		else
		{
			// Добавить результат через разделитель ';'
			wchar_t* pszAll = lstrmerge(pszAllResult, L";", pszResult);
			if (!pszAll)
			{
				_ASSERTE(pszAll!=NULL);
				free(pszResult);
				break;
			}
			free(pszAllResult);
			free(pszResult);
			pszAllResult = pszAll;
		}

		p = GetNextMacro(asMacro, false, NULL/*&pszErr*/);
	}

	// Fin
	return pszAllResult;
}

void CConEmuMacro::SkipWhiteSpaces(LPWSTR& rsString)
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

// Получить следующий параметр (до следующей ',')
LPWSTR CConEmuMacro::GetNextArg(LPWSTR& rsArguments, LPWSTR& rsArg)
{
	rsArg = NULL; // Сброс

	if (!rsArguments || !*rsArguments)
		return NULL;

	// Пропустить white-space
	SkipWhiteSpaces(rsArguments);

	// Результат
	rsArg = rsArguments;
	// Пропустить все, что до следующей запятой
	bool lbNextFound = false;

	while (*rsArguments && !lbNextFound)
	{
		lbNextFound = (*rsArguments == L',');

		if (lbNextFound)
			*rsArguments = 0; // Саму строку нужно "закрыть"

		rsArguments++;
	}

	return (*rsArg) ? rsArg : NULL;
}
// Получить следующий числовой параметр
LPWSTR CConEmuMacro::GetNextInt(LPWSTR& rsArguments, int& rnValue)
{
	LPWSTR pszArg = NULL;
	rnValue = 0; // сброс

	if (!GetNextArg(rsArguments, pszArg))
		return NULL;

	LPWSTR pszEnd = NULL;

	// Допустим hex значения
	if (pszArg[0] == L'0' && (pszArg[1] == L'x' || pszArg[1] == L'X'))
		rnValue = wcstol(pszArg+2, &pszEnd, 16);
	else
		rnValue = wcstol(pszArg, &pszEnd, 10);

	return pszArg;
}
// Получить следующий строковый параметр
LPWSTR CConEmuMacro::GetNextString(LPWSTR& rsArguments, LPWSTR& rsString, bool bColonDelim /*= false*/)
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

	// C-style строки
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
	else
	{
		_ASSERTE(bColonDelim && "String without quotas!");
		rsString = rsArguments;
		rsArguments = rsArguments + _tcslen(rsArguments);
		goto wrap;
	}

	if (*rsArguments)
	{
		_ASSERTE(rsArguments>=pszArgStart && rsArguments<pszArgEnd);

		// Пропустить все, что до следующей запятой
		SkipWhiteSpaces(rsArguments);
		if (*rsArguments == L',')
			rsArguments++;

		_ASSERTE(rsArguments>=pszArgStart && rsArguments<pszArgEnd);
	}

wrap:
	// Тут уже NULL-а не будет, допускаются пустые строки ("")
	_ASSERTE(rsString!=NULL);
	return rsString;
}
/* ***  Теперь - собственно макросы  *** */

// Проверка, есть ли ConEmu GUI. Функцию мог бы плагин и сам обработать,
// но для "общности" возвращаем "Yes" здесь
LPWSTR CConEmuMacro::IsConEmu(GuiMacro* p, CRealConsole* apRCon)
{
	LPWSTR pszResult = lstrdup(L"Yes");
	return pszResult;
}

// Проверка, видима ли RealConsole
LPWSTR CConEmuMacro::IsRealVisible(GuiMacro* p, CRealConsole* apRCon)
{
	LPWSTR pszResult = NULL;

	if (apRCon && IsWindowVisible(apRCon->ConWnd()))
		pszResult = lstrdup(L"Yes");
	else
		pszResult = lstrdup(L"No");

	return pszResult;
}

// Проверка, активна ли RealConsole
LPWSTR CConEmuMacro::IsConsoleActive(GuiMacro* p, CRealConsole* apRCon)
{
	LPWSTR pszResult = NULL;

	if (apRCon && apRCon->isActive())
		pszResult = lstrdup(L"Yes");
	else
		pszResult = lstrdup(L"No");

	return pszResult;
}

LPWSTR CConEmuMacro::Close(GuiMacro* p, CRealConsole* apRCon)
{
	LPWSTR pszResult = NULL;
	int nCmd = 0, nFlags = 0;

	if (p->GetIntArg(0, nCmd))
		p->GetIntArg(1, nFlags);

	switch (nCmd)
	{
	case 0:
	case 1:
		if (apRCon)
		{
			apRCon->CloseConsole(nCmd==1, (nFlags & 1)==0);
			pszResult = lstrdup(L"OK");
		}
		break;
	case 2:
		if (CVConGroup::OnScClose())
			pszResult = lstrdup(L"OK");
		break;
	case 3:
		if (apRCon)
		{
			apRCon->CloseTab();
			pszResult = lstrdup(L"OK");
		}
		break;
	case 4:
		if (apRCon)
		{
			CVConGroup::CloseGroup(apRCon->VCon());
			pszResult = lstrdup(L"OK");
		}
		break;
	}

	if (!pszResult)
		lstrdup(L"Failed");

	return pszResult;
}

// Найти окно и активировать его. // LPWSTR asName
LPWSTR CConEmuMacro::FindEditor(GuiMacro* p, CRealConsole* apRCon)
{
	LPWSTR pszName = NULL;

	if (!p->GetStrArg(0, pszName))
		return lstrdup(L"");

	// Пустые строки не допускаются
	if (!pszName || !*pszName)
		return lstrdup(L"");

	return FindFarWindowHelper(3/*WTYPE_EDITOR*/, pszName, apRCon);
}
// Найти окно и активировать его. // LPWSTR asName
LPWSTR CConEmuMacro::FindViewer(GuiMacro* p, CRealConsole* apRCon)
{
	LPWSTR pszName = NULL;

	if (!p->GetStrArg(0, pszName))
		return lstrdup(L"");

	// Пустые строки не допускаются
	if (!pszName || !*pszName)
		return lstrdup(L"");

	return FindFarWindowHelper(2/*WTYPE_VIEWER*/, pszName, apRCon);
}
// Найти окно и активировать его. // int nWindowType, LPWSTR asName
LPWSTR CConEmuMacro::FindFarWindow(GuiMacro* p, CRealConsole* apRCon)
{
	int nWindowType = 0;
	LPWSTR pszName = NULL;

	if (!p->GetStrArg(0, pszName))
		return lstrdup(L"");

	// Пустые строки не допускаются
	if (!pszName || !*pszName)
		return lstrdup(L"");

	return FindFarWindowHelper(nWindowType, pszName, apRCon);
}
LPWSTR CConEmuMacro::FindFarWindowHelper(
    CEFarWindowType anWindowType/*Panels=1, Viewer=2, Editor=3, |(Elevated=0x100), |(NotElevated=0x200), |(Modal=0x400)*/,
    LPWSTR asName, CRealConsole* apRCon)
{
	CVConGuard VCon;
	int iFound = 0;
	bool bFound = gpConEmu->isFarExist(anWindowType|fwt_ActivateFound, asName, &VCon);

	if (!bFound && VCon.VCon())
		iFound = -1; // редактор есть, но заблокирован модальным диалогом/другим редактором
	else if (bFound)
		iFound = gpConEmu->isVConValid(VCon.VCon()); //1-based

	//CRealConsole* pRCon, *pActiveRCon = NULL;
	//CVirtualConsole* pVCon;
	//ConEmuTab tab;
	//int iFound = 0;
	//DWORD nElevateFlag = (anWindowType & 0x300);
	//pVCon = gpConEmu->ActiveCon();

	//if (pVCon)
	//	pActiveRCon = pVCon->RCon();

	//for (int i = 0; !iFound && (i < MAX_CONSOLE_COUNT); i++)
	//{
	//	if (!(pVCon = gpConEmu->GetVCon(i)))
	//		break;

	//	if (!(pRCon = pVCon->RCon()) || pRCon == pActiveRCon)
	//		continue;

	//	for (int j = 0; !iFound; j++)
	//	{
	//		if (!pRCon->GetTab(j, &tab))
	//			break;

	//		// Если передали 0 - интересует любое окно
	//		if (anWindowType)
	//		{
	//			if ((tab.Type & 0xFF) != (anWindowType & 0xFF))
	//				continue;

	//			if (nElevateFlag)
	//			{
	//				if ((nElevateFlag == 0x100) && !(tab.Type & 0x100))
	//					continue;

	//				if ((nElevateFlag == 0x200) && (tab.Type & 0x100))
	//					continue;
	//			}
	//		}

	//		if (lstrcmpi(tab.Name, asName) == 0)
	//		{
	//			if (pRCon->ActivateFarWindow(j))
	//			{
	//				iFound = i+1;
	//				gpConEmu->Activate(pVCon);
	//			}
	//			else
	//			{
	//				iFound = -1;
	//			}

	//			break;
	//		}
	//	}
	//}

	int cchSize = 32; //-V112
	LPWSTR pszResult = (LPWSTR)malloc(2*cchSize);

	if (iFound > 0)
		_wsprintf(pszResult, SKIPLEN(cchSize) L"Found:%i", iFound);
	else if (iFound == -1)
		lstrcpyn(pszResult, L"Blocked", cchSize);
	else
		lstrcpyn(pszResult, L"NotFound", cchSize);

	return pszResult;
}

// Fullscreen
LPWSTR CConEmuMacro::WindowFullscreen(GuiMacro* p, CRealConsole* apRCon)
{
	LPWSTR pszRc = WindowMode(NULL, NULL);

	gpConEmu->OnAltEnter();

	return pszRc;
}

// Fullscreen
LPWSTR CConEmuMacro::WindowMaximize(GuiMacro* p, CRealConsole* apRCon)
{
	LPWSTR pszRc = WindowMode(NULL, NULL);

	gpConEmu->OnAltF9();

	return pszRc;
}

// Минимизировать окно (можно насильно в трей) // [int nForceToTray=0/1]
LPWSTR CConEmuMacro::WindowMinimize(GuiMacro* p, CRealConsole* apRCon)
{
	LPWSTR pszRc = WindowMode(NULL, NULL);

	int nForceToTray = 0;
	p->GetIntArg(0, nForceToTray);

	if (nForceToTray)
		Icon.HideWindowToTray();
	else
		PostMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);

	return pszRc;
}

// Вернуть текущий статус: NORMAL/MAXIMIZED/FULLSCREEN/MINIMIZED
// Или установить новый
LPWSTR CConEmuMacro::WindowMode(GuiMacro* p, CRealConsole* apRCon)
{
	LPCWSTR pszRc = NULL;
	LPWSTR pszMode = NULL;

	LPCWSTR sFS  = L"FS";
	LPCWSTR sMAX = L"MAX";
	LPCWSTR sMIN = L"MIN";
	LPCWSTR sTSA = L"TSA";
	LPCWSTR sNOR = L"NOR";

	if (p->GetStrArg(0, pszMode))
	{
		if (lstrcmpi(pszMode, sFS) == 0)
		{
			pszRc = sFS;
			gpConEmu->SetWindowMode(wmFullScreen);
		}
		else if (lstrcmpi(pszMode, sMAX) == 0)
		{
			pszRc = sMAX;
			gpConEmu->SetWindowMode(wmMaximized);
		}
		else if (lstrcmpi(pszMode, sMIN) == 0)
		{
			pszRc = sMIN;
			PostMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		}
		else if (lstrcmpi(pszMode, sTSA) == 0)
		{
			pszRc = sTSA;
			Icon.HideWindowToTray();
		}
		else //if (lstrcmpi(pszMode, sNOR) == 0)
		{
			pszRc = sNOR;
			gpConEmu->SetWindowMode(wmNormal);
		}
	}

	pszRc = pszRc ? pszRc :
		!IsWindowVisible(ghWnd) ? sTSA :
		gpConEmu->isFullScreen() ? sFS :
		gpConEmu->isZoomed() ? sMAX :
		gpConEmu->isIconic() ? sMIN :
		sNOR;

	return lstrdup(pszRc);
}

// Menu(Type)
LPWSTR CConEmuMacro::Menu(GuiMacro* p, CRealConsole* apRCon)
{
	int nType = 0;
	p->GetIntArg(0, nType);

	POINT ptCur = {-32000,-32000};

	if (isPressed(VK_LBUTTON) || isPressed(VK_MBUTTON) || isPressed(VK_RBUTTON))
		GetCursorPos(&ptCur);

	switch (nType)
	{
	case 0:
		WARNING("учитывать gpCurrentHotKey, если оно по клику мышки - показывать в позиции курсора");
		gpConEmu->mp_Menu->ShowSysmenu(ptCur.x, ptCur.y);
		return lstrdup(L"OK");

	case 1:
		if (apRCon)
		{
			WARNING("учитывать gpCurrentHotKey, если оно по клику мышки - показывать в позиции курсора");
			if (ptCur.x == -32000)
				ptCur = gpConEmu->mp_Menu->CalcTabMenuPos(apRCon->VCon());
			gpConEmu->mp_Menu->ShowPopupMenu(apRCon->VCon(), ptCur);
			return lstrdup(L"OK");
		}
		break;
	}

	return lstrdup(L"InvalidArg");
}

// MessageBox(ConEmu,asText,asTitle,anType) // LPWSTR asText [, LPWSTR asTitle[, int anType]]
LPWSTR CConEmuMacro::MsgBox(GuiMacro* p, CRealConsole* apRCon)
{
	LPWSTR pszText = NULL, pszTitle = NULL;
	int nButtons = 0;

	if (p->GetStrArg(0, pszText))
		if (p->GetStrArg(1, pszTitle))
			p->GetIntArg(2, nButtons);

	int nRc = MessageBox(NULL, pszText ? pszText : L"", pszTitle ? pszTitle : L"ConEmu Macro", nButtons);

	switch(nRc)
	{
		case IDABORT:
			return lstrdup(L"Abort");
		case IDCANCEL:
			return lstrdup(L"Cancel");
		case IDIGNORE:
			return lstrdup(L"Ignore");
		case IDNO:
			return lstrdup(L"No");
		case IDOK:
			return lstrdup(L"OK");
		case IDRETRY:
			return lstrdup(L"Retry");
		case IDYES:
			return lstrdup(L"Yes");
	}

	return NULL;
}

// Изменить размер шрифта.
// int nRelative, int N ("+1" - увеличить на N пунктов, "-1" - уменьшить, "0" - N- указан размер), int N
// для nRelative==0: N - высота
// для nRelative==1: N - +-1, +-2
// Возвращает - OK или InvalidArg
LPWSTR CConEmuMacro::FontSetSize(GuiMacro* p, CRealConsole* apRCon)
{
	//bool lbSetFont = false;
	int nRelative = 0, nValue = 0;

	if (p->GetIntArg(0, nRelative)
		&& p->GetIntArg(1, nValue))
	{
		//lbSetFont = gpSet->AutoSizeFont(nRelative, nValue);
		//if (lbSetFont)
		gpConEmu->PostAutoSizeFont(nRelative, nValue);
		return lstrdup(L"OK");
	}

	//int cchSize = 32;
	//LPWSTR pszResult = (LPWSTR)malloc(2*cchSize);
	//_wsprintf(pszResult, cchSize, L"%i", gpSetCls->FontHeight());
	return lstrdup(L"InvalidArg");
}

// Изменить имя основного шрифта. string. Как бонус - можно сразу поменять и размер
// <FontName>[,<Height>[,<Width>]]
LPWSTR CConEmuMacro::FontSetName(GuiMacro* p, CRealConsole* apRCon)
{
	LPWSTR pszFontName = NULL;
	int nHeight = 0, nWidth = 0;

	if (p->GetStrArg(0, pszFontName))
	{
		if (!p->GetIntArg(1, nHeight))
			nHeight = 0;
		else if (!p->GetIntArg(2, nWidth))
			nWidth = 0;

		gpConEmu->PostMacroFontSetName(pszFontName, nHeight, nWidth, FALSE);

		return lstrdup(L"OK");
	}

	return lstrdup(L"InvalidArg");
}

// Paste (<Cmd>[,"<Text>"])
LPWSTR CConEmuMacro::Paste(GuiMacro* p, CRealConsole* apRCon)
{
	int nCommand = 0;
	LPWSTR pszText = NULL;

	if (!apRCon)
		return lstrdup(L"InvalidArg");

	if (p->GetIntArg(0, nCommand))
	{
		bool bFirstLineOnly = (nCommand & 1) != 0;
		bool bNoConfirm = (nCommand & 2) != 0;

		wchar_t* pszChooseBuf = NULL;

		if (!(nCommand >= 0 && nCommand <= 8))
		{
			return lstrdup(L"InvalidArg");
		}

		if (p->GetStrArg(1, pszText))
		{
			// Пустая строка допускается только при выборе файла/папки для вставки
			if (!*pszText && !((nCommand >= 4) && (nCommand <= 7)))
				return lstrdup(L"InvalidArg");
		}
		else
		{
			pszText = NULL;
		}

		if ((nCommand >= 4) && (nCommand <= 7))
		{
			if ((nCommand == 4) || (nCommand == 6))
				pszChooseBuf = SelectFile(L"Choose file pathname for paste", pszText, NULL, NULL, true, (nCommand == 6));
			else
				pszChooseBuf = SelectFolder(L"Choose folder path for paste", pszText, NULL, true, (nCommand == 7));

			if (!pszChooseBuf)
				return lstrdup(L"Cancelled");

			pszText = pszChooseBuf;
			bFirstLineOnly = true;
			bNoConfirm = true;
		}
		else if (nCommand == 8)
		{
			bFirstLineOnly = true;
			bNoConfirm = true;
		}

		apRCon->Paste(bFirstLineOnly, pszText, bNoConfirm, (nCommand == 8)/*abCygWin*/);

		SafeFree(pszChooseBuf);
		return lstrdup(L"OK");
	}

	return lstrdup(L"InvalidArg");
}

// print("<Text>") - alias for Paste(2,"<Text>")
LPWSTR CConEmuMacro::Print(GuiMacro* p, CRealConsole* apRCon)
{
	if (!apRCon)
		return lstrdup(L"InvalidArg");

	LPWSTR pszText = NULL;
	if (p->GetStrArg(0, pszText))
	{
		if (!*pszText)
			return lstrdup(L"InvalidArg");
	}
	else
	{
		pszText = NULL;
	}

	apRCon->Paste(false, pszText, true);

	return lstrdup(L"OK");
}

// Progress(<Type>[,<Value>])
LPWSTR CConEmuMacro::Progress(GuiMacro* p, CRealConsole* apRCon)
{
	int nType = 0, nValue = 0;
	LPWSTR pszName = NULL;
	if (!apRCon)
		return lstrdup(L"InvalidArg");

	if (p->GetIntArg(0, nType))
	{
		if (!(nType >= 0 && nType <= 5))
		{
			return lstrdup(L"InvalidArg");
		}

		if (nType <= 2)
			p->GetIntArg(1, nValue);
		else if (nType == 4 || nType == 5)
			p->GetStrArg(1, pszName);

		apRCon->SetProgress(nType, nValue, pszName);

		return lstrdup(L"OK");
	}

	return lstrdup(L"InvalidArg");
}

// Rename(<Type>,"<Title>")
LPWSTR CConEmuMacro::Rename(GuiMacro* p, CRealConsole* apRCon)
{
	int nType = 0;
	LPWSTR pszTitle = NULL;

	if (!apRCon)
		return lstrdup(L"InvalidArg");

	if (p->GetIntArg(0, nType))
	{
		if (!(nType == 0 || nType == 1))
		{
			return lstrdup(L"InvalidArg");
		}

		if (!p->GetStrArg(1, pszTitle) || !*pszTitle)
		{
			pszTitle = NULL;
		}

		switch (nType)
		{
		case 0:
			apRCon->RenameTab(pszTitle);
			break;
		case 1:
			apRCon->RenameWindow(pszTitle);
			break;
		}

		return lstrdup(L"OK");
	}

	return lstrdup(L"InvalidArg");
}

LPWSTR CConEmuMacro::Select(GuiMacro* p, CRealConsole* apRCon)
{
	if (!apRCon)
		return lstrdup(L"No console");

	if (apRCon->isSelectionPresent())
		return lstrdup(L"Selection was already started");

	int nType = 0; // 0 - text, 1 - block
	int nDX = 0, nDY = 0;

	p->GetIntArg(0, nType);
	p->GetIntArg(1, nDX);
	p->GetIntArg(2, nDY);

	bool bText = (nType == 0);

	COORD cr = {};
	apRCon->GetConsoleCursorInfo(NULL, &cr);
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	apRCon->GetConsoleScreenBufferInfo(&csbi);

	WARNING("TODO: buffered selection!");
	if (cr.X >= csbi.srWindow.Left)
	{
		cr.X -= csbi.srWindow.Left;
	}
	else
	{
		_ASSERTE(cr.X >= csbi.srWindow.Left);
	}

	if (cr.Y >= csbi.srWindow.Top)
	{
		cr.Y -= csbi.srWindow.Top;
	}
	else
	{
		_ASSERTE(cr.Y >= csbi.srWindow.Top);
	}

	if (nType == 0)
	{
		if ((nDX < 0) && cr.X)
			cr.X--;
	}

	apRCon->StartSelection(bText, cr.X, cr.Y);

	if ((nType == 1) && nDY)
	{
		apRCon->ExpandSelection(cr.X, cr.Y + nDY);
	}

	return lstrdup(L"OK");
}

// SetOption("<Name>",<Value>)
LPWSTR CConEmuMacro::SetOption(GuiMacro* p, CRealConsole* apRCon)
{
	LPWSTR pszName = NULL;
	int nValue = 0;

	if (!p->GetStrArg(0, pszName))
		return lstrdup(L"InvalidArg");

	if (!lstrcmpi(pszName, L"QuakeAutoHide"))
	{
		// SetOption("QuakeAutoHide",0) - Hide on demand only
		// SetOption("QuakeAutoHide",1) - Enable autohide on focus lose
		// SetOption("QuakeAutoHide",2) - Switch autohide
		p->GetIntArg(1, nValue);
		if (gpSet->isQuakeStyle)
		{
			switch (nValue)
			{
			case 0:
				gpConEmu->SetQuakeMode(1);
				break;
			case 1:
				gpConEmu->SetQuakeMode(2);
				break;
			case 2:
				gpConEmu->SetQuakeMode((gpSet->isQuakeStyle == 1) ? 2 : 1);
				break;
			}
		}
	}
	else
	{
		//TODO: More options on demand
	}
	
	return lstrdup(L"UnknownOption");
}

// ShellExecute
// <oper>,<app>[,<parm>[,<dir>[,<showcmd>]]]
LPWSTR CConEmuMacro::Shell(GuiMacro* p, CRealConsole* apRCon)
{
	wchar_t* pszRc = NULL;
	LPWSTR pszOper = NULL, pszFile = NULL, pszParm = NULL, pszDir = NULL;
	LPWSTR pszBuf = NULL;
	bool bDontQuote = false;
	int nShowCmd = SW_SHOWNORMAL;
	
	if (p->GetStrArg(0, pszOper))
	{
		CVConGuard VCon(apRCon ? apRCon->VCon() : NULL);

		if (p->GetStrArg(1, pszFile))
		{
			if (!p->GetStrArg(2, pszParm))
				pszParm = NULL;
			else if (!p->GetStrArg(3, pszDir))
				pszDir = NULL;
			else if (!p->GetIntArg(4, nShowCmd))
				nShowCmd = SW_SHOWNORMAL;
		}

		if (!(pszFile && *pszFile) && !(pszParm && *pszParm) && apRCon)
		{
			LPCWSTR pszCmd;

			if (pszOper && (wmemcmp(pszOper, L"new_console", 11) == 0))
			{
				wchar_t* pszAddArgs = lstrmerge(L"\"-", pszOper, L"\"");
				bool bOk = apRCon->DuplicateRoot(true, pszAddArgs);
				SafeFree(pszAddArgs);
				if (bOk)
				{
					pszRc = lstrdup(L"OK");
					goto wrap;
				}
			}

			pszCmd = apRCon->GetCmd();
			if (pszCmd && *pszCmd)
			{
				pszBuf = lstrdup(pszCmd);
				pszFile = pszBuf;
				bDontQuote = true;
			}
		}

		if (!pszFile)
		{
			pszBuf = lstrdup(L"");
			pszFile = pszBuf;
		}

		if ((pszFile && *pszFile) || (pszParm && *pszParm))
		{

			bool bNewTaskGroup = false;

			if (*pszFile == CmdFilePrefix || *pszFile == TaskBracketLeft)
			{
				wchar_t* pszDataW = gpConEmu->LoadConsoleBatch(pszFile);
				if (pszDataW)
				{
					bNewTaskGroup = true;
					SafeFree(pszDataW);
				}
			}
			
			// 120830 - пусть shell("","cmd") запускает новую вкладку в ConEmu
			bool bNewOper = bNewTaskGroup || (*pszOper == 0) || (wmemcmp(pszOper, L"new_console", 11) == 0);

			if (bNewOper || (pszParm && wcsstr(pszParm, L"-new_console")))
			{
				size_t nAllLen;
				RConStartArgs *pArgs = new RConStartArgs;

				if (bNewTaskGroup)
				{
					_ASSERTE(pszParm==NULL || *pszParm==0);
					pArgs->pszSpecialCmd = lstrdup(pszFile);
				}
				else
				{
					nAllLen = _tcslen(pszFile) + (pszParm ? _tcslen(pszParm) : 0) + 16;
					
					if (bNewOper)
					{
						size_t nOperLen = _tcslen(pszOper);
						if ((nOperLen > 11) && (pszOper[11] == L':'))
							nAllLen += (nOperLen + 6);
						else
							bNewOper = false;
					}
					
					pArgs->pszSpecialCmd = (wchar_t*)malloc(nAllLen*sizeof(wchar_t));
					pArgs->pszSpecialCmd[0] = 0;
					
					if (*pszFile)
					{
						if (!bDontQuote && (*pszFile != L'"') && (wcschr(pszFile, L' ') != NULL))
						{
							pArgs->pszSpecialCmd[0] = L'"';
							_wcscpy_c(pArgs->pszSpecialCmd+1, nAllLen-1, pszFile);
							_wcscat_c(pArgs->pszSpecialCmd, nAllLen, (pszParm && *pszParm) ? L"\" " : L"\"");
						}
						else
						{
							_wcscpy_c(pArgs->pszSpecialCmd, nAllLen, pszFile);
							if (pszParm && *pszParm)
							{
								_wcscat_c(pArgs->pszSpecialCmd, nAllLen, L" ");
							}
						}
					}
					
					if (pszParm && *pszParm)
					{
						_wcscat_c(pArgs->pszSpecialCmd, nAllLen, pszParm);
					}
					
					// Параметры запуска консоли могли передать в первом аргуементе (например "new_console:b")
					if (bNewOper)
					{
						_wcscat_c(pArgs->pszSpecialCmd, nAllLen, L" \"-");
						_wcscat_c(pArgs->pszSpecialCmd, nAllLen, pszOper);
						_wcscat_c(pArgs->pszSpecialCmd, nAllLen, L"\"");
					}
				}
				
				if (pszDir)
					pArgs->pszStartupDir = lstrdup(pszDir);
			
				gpConEmu->PostCreateCon(pArgs);
				
				pszRc = lstrdup(L"OK");
				goto wrap;
			}
			else
			{
				int nRc = (int)ShellExecuteW(ghWnd, pszOper, pszFile, pszParm, pszDir, nShowCmd);
				
				size_t cchSize = 16;
				pszRc = (LPWSTR)malloc(2*cchSize);

				if (nRc <= 32)
					_wsprintf(pszRc, SKIPLEN(cchSize) L"Failed:%i", nRc);
				else
					lstrcpyn(pszRc, L"OK", cchSize);
				
				goto wrap;
			}
		}
	}

wrap:
	SafeFree(pszBuf);
	return lstrdup(L"InvalidArg");
}

// Status(0[,<Parm>]) - Show/Hide status bar, Parm=1 - Show, Parm=2 - Hide
// Status(1[,"<Text>"]) - Set status bar text
LPWSTR CConEmuMacro::Status(GuiMacro* p, CRealConsole* apRCon)
{
	LPWSTR pszResult = NULL;
	int nStatusCmd = 0;
	int nParm = 0;
	LPWSTR pszText = NULL;

	if (!p->GetIntArg(0, nStatusCmd))
		nStatusCmd = 0;

	switch (nStatusCmd)
	{
	case csc_ShowHide:
		p->GetIntArg(1, nParm);
		gpConEmu->StatusCommand(csc_ShowHide, nParm);
		pszResult = lstrdup(L"OK");
		break;
	case csc_SetStatusText:
		if (apRCon)
		{
			p->GetStrArg(1, pszText);
			gpConEmu->StatusCommand(csc_ShowHide, 0, pszText, apRCon);
			pszResult = lstrdup(L"OK");
		}
		break;
	}

	return pszResult ? pszResult : lstrdup(L"InvalidArg");
}

// TabControl
// <Cmd>[,<Parm>]
// Cmd: 0 - Show/Hide tabs
//		1 - commit lazy changes
//		2 - switch next (eq. CtrlTab)
//		3 - switch prev (eq. CtrlShiftTab)
//      4 - switch tab direct (no recent mode), parm=(1,-1)
//      5 - switch tab recent, parm=(1,-1)
//      6 - switch console direct (no recent mode), parm=(1,-1)
//      7 - activate console by number, parm=(one-based console index)
//		8 - show tabs list menu (indiffirent Far/Not Far)
// Returns "OK" on success
LPWSTR CConEmuMacro::Tab(GuiMacro* p, CRealConsole* apRCon)
{
	LPWSTR pszResult = NULL;

	int nTabCmd = 0;
	if (p->GetIntArg(0, nTabCmd))
	{
		int nParm = 0;
		p->GetIntArg(1, nParm);

		switch (nTabCmd)
		{
		case ctc_ShowHide:
		case ctc_SwitchCommit: // commit lazy changes
		case ctc_SwitchNext:
		case ctc_SwitchPrev:
			gpConEmu->TabCommand((ConEmuTabCommand)nTabCmd);
			pszResult = lstrdup(L"OK");
			break;
		case ctc_SwitchDirect: // switch tab direct (no recent mode), parm=(1,-1)
			if (nParm == 1)
			{
				gpConEmu->mp_TabBar->SwitchNext(gpSet->isTabRecent);
				pszResult = lstrdup(L"OK");
			}
			else if (nParm == -1)
			{
				gpConEmu->mp_TabBar->SwitchPrev(gpSet->isTabRecent);
				pszResult = lstrdup(L"OK");
			}
			break;
		case ctc_SwitchRecent: // switch tab recent, parm=(1,-1)
			if (nParm == 1)
			{
				gpConEmu->mp_TabBar->SwitchNext(!gpSet->isTabRecent);
				pszResult = lstrdup(L"OK");
			}
			else if (nParm == -1)
			{
				gpConEmu->mp_TabBar->SwitchPrev(!gpSet->isTabRecent);
				pszResult = lstrdup(L"OK");
			}
			break;
		case ctc_SwitchConsoleDirect: // switch console direct (no recent mode), parm=(1,-1)
			{
				int nActive = gpConEmu->ActiveConNum(); // 0-based
				if (nParm == 1)
				{
					// Прокрутка вперед (циклически)
					if (gpConEmu->GetVCon(nActive+1))
					{
						gpConEmu->ConActivate(nActive+1);
						pszResult = lstrdup(L"OK");
					}
					else if (nActive)
					{
						gpConEmu->ConActivate(0);
						pszResult = lstrdup(L"OK");
					}
				}
				else if (nParm == -1)
				{
					// Прокрутка назад (циклически)
					if (nActive > 0)
					{
						gpConEmu->ConActivate(nActive-1);
						pszResult = lstrdup(L"OK");
					}
					else
					{
						int nNew = gpConEmu->GetConCount()-1;
						if (nNew > nActive)
						{
							gpConEmu->ConActivate(nNew);
							pszResult = lstrdup(L"OK");
						}
					}
				}
			}
			break;
		case ctc_ActivateConsole: // activate console by number, parm=(one-based console index)
			if (nParm >= 1 && gpConEmu->GetVCon(nParm-1))
			{
				gpConEmu->ConActivate(nParm-1);
				pszResult = lstrdup(L"OK");
			}
			break;
		case ctc_ShowTabsList: // 
			CConEmuCtrl::key_ShowTabsList(0, false, NULL, NULL/*чтобы не зависимо от фара показала меню*/);
			break;
		case ctc_CloseTab:
			if (apRCon)
			{
				apRCon->CloseTab();
				pszResult = lstrdup(L"OK");
			}
			break;
		case ctc_SwitchPaneDirect: // switch visible panes direct (no recent mode), parm=(1,-1), like ctc_SwitchConsoleDirect but for Splits
			if (apRCon && (nParm == 1 || nParm == -1))
			{
				bool bNext = (nParm == 1);
				if (CVConGroup::PaneActivateNext(bNext))
				{
					pszResult = lstrdup(L"OK");
				}
			}
			break;
		}
	}

	return pszResult ? pszResult : lstrdup(L"InvalidArg");
}

// Task(Index)
// Task("Name")
LPWSTR CConEmuMacro::Task(GuiMacro* p, CRealConsole* apRCon)
{
	LPCWSTR pszResult = NULL;
	LPCWSTR pszName = NULL, pszDir = NULL;
	wchar_t* pszBuf = NULL;
	int nTaskIndex = 0;

	if (p->argc < 1)
		return lstrdup(L"InvalidArg");

	if (p->IsStrArg(0))
	{
		LPWSTR pszArg = NULL;
		if (p->GetStrArg(0, pszArg))
		{
			if (*pszArg && (*pszArg != TaskBracketLeft))
			{
				size_t cchMax = _tcslen(pszArg)+3;
				pszBuf = (wchar_t*)malloc(cchMax*sizeof(*pszBuf));
				if (pszBuf)
				{
					*pszBuf = TaskBracketLeft;
					_wcscpy_c(pszBuf+1, cchMax-1, pszArg);
					pszBuf[cchMax-2] = TaskBracketRight;
					pszBuf[cchMax-1] = 0;
					pszName = pszBuf;
				}
			}
			else
			{
				pszName = pszArg;
			}
		}
	}
	else
	{
		if (p->GetIntArg(0, nTaskIndex) && (nTaskIndex > 0))
		{
			const Settings::CommandTasks* pTask = gpSet->CmdTaskGet(nTaskIndex - 1);
			if (pTask)
				pszName = pTask->pszName;
		}
	}

	if (pszName && *pszName)
	{
		RConStartArgs *pArgs = new RConStartArgs;
		pArgs->pszSpecialCmd = lstrdup(pszName);

		if (pszDir)
			pArgs->pszStartupDir = lstrdup(pszDir);

		gpConEmu->PostCreateCon(pArgs);

		pszResult = L"OK";
	}

	return pszResult ? lstrdup(pszResult) : lstrdup(L"InvalidArg");
}
