
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


CConEmuMacro::CConEmuMacro()
{
	TODO("Регистрация функций для автоматической обработки?");
}

// Общая функция, для обработки любого известного макроса
LPWSTR CConEmuMacro::ExecuteMacro(LPWSTR asMacro, CRealConsole* apRCon)
{
	if (!asMacro)
		return NULL;

	// Skip white-spaces
	while(*asMacro == L' ' || *asMacro == L'\t' || *asMacro == L'\r' || *asMacro == L'\n')
		asMacro++;

	if (!*asMacro)
		return NULL;

	wchar_t szFunction[64], chTerm = 0;
	bool lbFuncOk = false;

	for (size_t i = 0; i < (countof(szFunction)-1); i++)
	{
		chTerm = asMacro[i];
		szFunction[i] = chTerm;

		if (chTerm < L' ' || chTerm > L'z')
		{
			if (chTerm == 0)
			{
				lbFuncOk = true;
				asMacro = asMacro + i;
			}

			break;
		}

		if (chTerm == L':' || chTerm == L'(' || chTerm == L' ')
		{
			// Skip white-spaces
			if (chTerm == L':' || chTerm == L'(')
			{
				asMacro = asMacro+i+1;
			}
			else
			{
				asMacro = asMacro+i;

				while(*asMacro == L' ' || *asMacro == L'\t' || *asMacro == L'\r' || *asMacro == L'\n')
					asMacro++;

				if (*asMacro == L'(')
				{
					chTerm = *asMacro;
					asMacro++;
				}
			}

			lbFuncOk = true;
			szFunction[i] = 0;
			break;
		}
	}

	if (!lbFuncOk)
	{
		if (chTerm || (!asMacro || (*asMacro != 0)))
		{
			_ASSERTE(chTerm || (asMacro && (*asMacro == 0)));
			return NULL;
		}
	}

	// Подготовить аргументы (отрезать закрывающую скобку)
	if (chTerm == L'(')
	{
		LPWSTR pszEnd = wcsrchr(asMacro, L')');

		if (pszEnd)
			*pszEnd = 0;
	}

	LPWSTR pszResult = NULL;

	// Поехали
	if (!lstrcmpi(szFunction, L"IsConEmu"))
		pszResult = IsConEmu(asMacro, apRCon);
	else if (!lstrcmpi(szFunction, L"FindEditor"))
		pszResult = FindEditor(asMacro, apRCon);
	else if (!lstrcmpi(szFunction, L"FindViewer"))
		pszResult = FindViewer(asMacro, apRCon);
	else if (!lstrcmpi(szFunction, L"FindFarWindow"))
		pszResult = FindFarWindow(asMacro, apRCon);
	else if (!lstrcmpi(szFunction, L"WindowMinimize"))
		pszResult = WindowMinimize(asMacro, apRCon);
	else if (!lstrcmpi(szFunction, L"MsgBox"))
		pszResult = MsgBox(asMacro, apRCon);
	else if (!lstrcmpi(szFunction, L"FontSetSize"))
		pszResult = FontSetSize(asMacro, apRCon);
	else if (!lstrcmpi(szFunction, L"FontSetName"))
		pszResult = FontSetName(asMacro, apRCon);
	else if (!lstrcmpi(szFunction, L"IsRealVisible"))
		pszResult = IsRealVisible(asMacro, apRCon);
	else if (!lstrcmpi(szFunction, L"IsConsoleActive"))
		pszResult = IsConsoleActive(asMacro, apRCon);
	else if (!lstrcmpi(szFunction, L"Shell"))
		pszResult = Shell(asMacro, apRCon);
	else
		pszResult = NULL; // Неизвестная функция

	// Fin
	return pszResult;
}

/* ***  Функции для парсера параметров  *** */

// Получить следующий строковый параметр
LPWSTR CConEmuMacro::GetNextString(LPWSTR& rsArguments, LPWSTR& rsString)
{
	rsString = NULL; // Сброс

	if (!rsArguments || !*rsArguments)
		return NULL;

	// Строки предполагаются как "verbatim string"
	if (*rsArguments == L'"')
	{
		rsString = rsArguments+1;
		LPWSTR pszFind = wcschr(rsString, L'"');
		bool lbRemoveDup = (pszFind && (pszFind[1] == L'"'));

		while(pszFind && (pszFind[1] == L'"'))
		{
			// Удвоенная кавычка, пропускаем
			pszFind = wcschr(pszFind+2, L'"');
		}

		if (!pszFind)
		{
			// До конца строки.
			rsArguments = rsArguments + _tcslen(rsArguments);
		}
		else
		{
			// Указатель нужно подвинуть к следующему параметру
			rsArguments = pszFind+1; _ASSERTE(*pszFind == L'"');
			// Саму строку нужно "закрыть"
			*pszFind = 0;
			// Пропустить все, что до следующей запятой
			bool lbNextFound = false;

			while(*rsArguments && !lbNextFound)
			{
				lbNextFound = (*rsArguments == L',');
				rsArguments++;
			}
		}

		// Если были удвоенные кавычки - их нужно вырезать
		if (lbRemoveDup)
		{
			size_t nLen = _tcslen(rsString);
			LPWSTR pszSrc = rsString;
			LPWSTR pszDst = rsString;
			LPWSTR pszFind = wcschr(rsString, L'"');

			/*
			12345678
			1""2""3
			1"2""3
			1"2"3
			12345678
			*/
			while(pszFind && (pszFind[1] == L'"'))
			{
				LPWSTR pszNext = wcschr(pszFind+2, L'"');

				if (!pszNext) pszNext = rsString+nLen;

				size_t nSkip = pszFind - pszSrc;
				pszDst += nSkip;
				size_t nCopy = pszNext - pszFind - 2;

				if (nCopy > 0)
				{
					wmemmove(pszDst, pszFind+1, nCopy+1);
				}

				pszDst ++;
				// Next
				pszSrc = pszFind+2;
				pszFind = pszNext;
			}

			size_t nLeft = _tcslen(pszSrc);

			if (nLeft > 0)
				pszDst += nLeft;

			_ASSERTE(*pszDst != 0);
			_ASSERTE((pszFind == NULL) || (*pszFind == 0));
			*pszDst = 0; // Закрыть строку, ее длина уменьшилась
		}
	}
	else
	{
		rsString = rsArguments;
		rsArguments = rsArguments + _tcslen(rsArguments);
	}

	// Тут уже NULL-а не будет, допускаются пустые строки ("")
	return rsString;
}
// Получить следующий параметр (до следующей ',')
LPWSTR CConEmuMacro::GetNextArg(LPWSTR& rsArguments, LPWSTR& rsArg)
{
	rsArg = NULL; // Сброс

	if (!rsArguments || !*rsArguments)
		return NULL;

	// Пропустить white-space
	while(*rsArguments == L' ' || *rsArguments == L'\t' || *rsArguments == L'\r' || *rsArguments == L'\n')
		rsArguments++;

	// Результат
	rsArg = rsArguments;
	// Пропустить все, что до следующей запятой
	bool lbNextFound = false;

	while(*rsArguments && !lbNextFound)
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

/* ***  Теперь - собственно макросы  *** */

// Проверка, есть ли ConEmu GUI. Функцию мог бы плагин и сам обработать,
// но для "общности" возвращаем "Yes" здесь
LPWSTR CConEmuMacro::IsConEmu(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPWSTR pszResult = lstrdup(L"Yes");
	return pszResult;
}

// Проверка, видима ли RealConsole
LPWSTR CConEmuMacro::IsRealVisible(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPWSTR pszResult = NULL;

	if (apRCon && IsWindowVisible(apRCon->ConWnd()))
		pszResult = lstrdup(L"Yes");
	else
		pszResult = lstrdup(L"No");

	return pszResult;
}

// Проверка, активна ли RealConsole
LPWSTR CConEmuMacro::IsConsoleActive(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPWSTR pszResult = NULL;

	if (apRCon && apRCon->isActive())
		pszResult = lstrdup(L"Yes");
	else
		pszResult = lstrdup(L"No");

	return pszResult;
}

// Найти окно и активировать его. // LPWSTR asName
LPWSTR CConEmuMacro::FindEditor(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPWSTR pszName = NULL;

	if (!GetNextString(asArgs, pszName))
		return NULL;

	// Пустые строки не допускаются
	if (!pszName || !*pszName)
		return NULL;

	return FindFarWindowHelper(3/*WTYPE_EDITOR*/, pszName, apRCon);
}
// Найти окно и активировать его. // LPWSTR asName
LPWSTR CConEmuMacro::FindViewer(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPWSTR pszName = NULL;

	if (!GetNextString(asArgs, pszName))
		return NULL;

	// Пустые строки не допускаются
	if (!pszName || !*pszName)
		return NULL;

	return FindFarWindowHelper(2/*WTYPE_VIEWER*/, pszName, apRCon);
}
// Найти окно и активировать его. // int nWindowType, LPWSTR asName
LPWSTR CConEmuMacro::FindFarWindow(LPWSTR asArgs, CRealConsole* apRCon)
{
	int nWindowType = 0;
	LPWSTR pszName = NULL;

	if (!GetNextString(asArgs, pszName))
		return NULL;

	// Пустые строки не допускаются
	if (!pszName || !*pszName)
		return NULL;

	return FindFarWindowHelper(nWindowType, pszName, apRCon);
}
LPWSTR CConEmuMacro::FindFarWindowHelper(
    int anWindowType/*Panels=1, Viewer=2, Editor=3, |(Elevated=0x100), |(NotElevated=0x200)*/,
    LPWSTR asName, CRealConsole* apRCon)
{
	CRealConsole* pRCon, *pActiveRCon = NULL;
	CVirtualConsole* pVCon;
	ConEmuTab tab;
	int iFound = 0;
	DWORD nElevateFlag = (anWindowType & 0x300);
	pVCon = gpConEmu->ActiveCon();

	if (pVCon)
		pActiveRCon = pVCon->RCon();

	for(int i = 0; !iFound && (i < MAX_CONSOLE_COUNT); i++)
	{
		if (!(pVCon = gpConEmu->GetVCon(i)))
			break;

		if (!(pRCon = pVCon->RCon()) || pRCon == pActiveRCon)
			continue;

		for(int j = 0; !iFound; j++)
		{
			if (!pRCon->GetTab(j, &tab))
				break;

			// Если передали 0 - интересует любое окно
			if (anWindowType)
			{
				if ((tab.Type & 0xFF) != (anWindowType & 0xFF))
					continue;

				if (nElevateFlag)
				{
					if ((nElevateFlag == 0x100) && !(tab.Type & 0x100))
						continue;

					if ((nElevateFlag == 0x200) && (tab.Type & 0x100))
						continue;
				}
			}

			if (lstrcmpi(tab.Name, asName) == 0)
			{
				if (pRCon->ActivateFarWindow(j))
				{
					iFound = i+1;
					gpConEmu->Activate(pVCon);
				}
				else
				{
					iFound = -1;
				}

				break;
			}
		}
	}

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

// Минимизировать окно (можно насильно в трей) // [int nForceToTray=0/1]
LPWSTR CConEmuMacro::WindowMinimize(LPWSTR asArgs, CRealConsole* apRCon)
{
	int nForceToTray = 0;
	GetNextInt(asArgs, nForceToTray);

	if (nForceToTray)
		Icon.HideWindowToTray();
	else
		PostMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);

	return lstrdup(L"OK");
}

// MessageBox(ConEmu,asText,asTitle,anType) // LPWSTR asText [, LPWSTR asTitle[, int anType]]
LPWSTR CConEmuMacro::MsgBox(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPWSTR pszText = NULL, pszTitle = NULL;
	int nButtons = 0;

	if (GetNextString(asArgs, pszText))
		if (GetNextString(asArgs, pszTitle))
			GetNextInt(asArgs, nButtons);

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
LPWSTR CConEmuMacro::FontSetSize(LPWSTR asArgs, CRealConsole* apRCon)
{
	//bool lbSetFont = false;
	int nRelative, nValue;

	if (GetNextInt(asArgs, nRelative) && GetNextInt(asArgs, nValue))
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
LPWSTR CConEmuMacro::FontSetName(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPWSTR pszFontName = NULL;
	int nHeight = 0, nWidth = 0;

	if (GetNextString(asArgs, pszFontName))
	{
		if (!GetNextInt(asArgs, nHeight))
			nHeight = 0;
		else if (!GetNextInt(asArgs, nWidth))
			nWidth = 0;
		gpConEmu->PostMacroFontSetName(pszFontName, nHeight, nWidth, FALSE);
		return lstrdup(L"OK");
	}

	return lstrdup(L"InvalidArg");
}

// ShellExecute
LPWSTR CConEmuMacro::Shell(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPWSTR pszOper = NULL, pszFile = NULL, pszParm = NULL, pszDir = NULL;
	int nShowCmd = SW_SHOWNORMAL;
	
	if (GetNextString(asArgs, pszOper))
	{
		if (GetNextString(asArgs, pszFile) && *pszFile)
		{
			if (!GetNextString(asArgs, pszParm))
				pszParm = NULL;
			else if (!GetNextString(asArgs, pszDir))
				pszDir = NULL;
			else if (!GetNextInt(asArgs, nShowCmd))
				nShowCmd = SW_SHOWNORMAL;
			
			if ((lstrcmpi(pszOper,L"new_console")==0) || (pszParm && wcsstr(pszParm, L"-new_console")))
			{
				RConStartArgs *pArgs = new RConStartArgs;
				
				size_t nAllLen = _tcslen(pszFile) + (pszParm ? _tcslen(pszParm) : 0) + 16;
				pArgs->pszSpecialCmd = (wchar_t*)malloc(nAllLen*sizeof(wchar_t));
				
				if (*pszFile != L'"')
				{
					pArgs->pszSpecialCmd[0] = L'"';
					_wcscpy_c(pArgs->pszSpecialCmd+1, nAllLen-1, pszFile);
					_wcscat_c(pArgs->pszSpecialCmd, nAllLen, L"\" ");
				}
				else
				{
					_wcscpy_c(pArgs->pszSpecialCmd, nAllLen, pszFile);
					_wcscat_c(pArgs->pszSpecialCmd, nAllLen, L" ");
				}
				
				if (pszParm && *pszParm)
				{
					_wcscat_c(pArgs->pszSpecialCmd, nAllLen, pszParm);
				}
				
				if (pszDir)
					pArgs->pszStartupDir = lstrdup(pszDir);
			
				gpConEmu->PostCreateCon(pArgs);
				
				return lstrdup(L"OK");
			}
			else
			{
				int nRc = (int)ShellExecuteW(ghWnd, pszOper, pszFile, pszParm, pszDir, nShowCmd);
				
				size_t cchSize = 16;
				LPWSTR pszResult = (LPWSTR)malloc(2*cchSize);

				if (nRc <= 32)
					_wsprintf(pszResult, SKIPLEN(cchSize) L"Failed:%i", nRc);
				else
					lstrcpyn(pszResult, L"OK", cchSize);
				
				return pszResult;
			}
		}
	}

	return lstrdup(L"InvalidArg");	
}
