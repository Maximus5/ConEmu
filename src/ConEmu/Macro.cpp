
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
#include <strsafe.h>
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
LPWSTR CConEmuMacro::ExecuteMacro(LPWSTR asMacro)
{
	if (!asMacro)
		return NULL;
	// Skip white-spaces
	while (*asMacro == L' ' || *asMacro == L'\t' || *asMacro == L'\r' || *asMacro == L'\n')
		asMacro++;
	if (!*asMacro)
		return NULL;
	
	wchar_t szFunction[64], chTerm = 0;
	bool lbFuncOk = false;
	for (int i = 0; i < (countof(szFunction)-1); i++)
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
				while (*asMacro == L' ' || *asMacro == L'\t' || *asMacro == L'\r' || *asMacro == L'\n')
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
		pszResult = IsConEmu(asMacro);
	else if (!lstrcmpi(szFunction, L"FindEditor"))
		pszResult = FindEditor(asMacro);
	else if (!lstrcmpi(szFunction, L"FindViewer"))
		pszResult = FindViewer(asMacro);
	else if (!lstrcmpi(szFunction, L"FindFarWindow"))
		pszResult = FindFarWindow(asMacro);
	else if (!lstrcmpi(szFunction, L"WindowMinimize"))
		pszResult = WindowMinimize(asMacro);
	else if (!lstrcmpi(szFunction, L"MsgBox"))
		pszResult = MsgBox(asMacro);
	else if (!lstrcmpi(szFunction, L"FontSetSize"))
		pszResult = FontSetSize(asMacro);
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
		while (pszFind && (pszFind[1] == L'"'))
		{
			// Удвоенная кавычка, пропускаем
			pszFind = wcschr(pszFind+2, L'"');
		}
		if (!pszFind)
		{
			// До конца строки.
			rsArguments = rsArguments + lstrlen(rsArguments);
		}
		else
		{
			// Указатель нужно подвинуть к следующему параметру
			rsArguments = pszFind+1; _ASSERTE(*pszFind == L'"');
			// Саму строку нужно "закрыть"
			*pszFind = 0;
			// Пропустить все, что до следующей запятой
			bool lbNextFound = false;
			while (*rsArguments && !lbNextFound)
			{
				lbNextFound = (*rsArguments == L',');
				rsArguments++;
			}
		}
		// Если были удвоенные кавычки - их нужно вырезать
		if (lbRemoveDup)
		{
			int nLen = lstrlen(rsString);
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
			while (pszFind && (pszFind[1] == L'"'))
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
			int nLeft = lstrlen(pszSrc);
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
		rsArguments = rsArguments + lstrlen(rsArguments);
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
	while (*rsArguments == L' ' || *rsArguments == L'\t' || *rsArguments == L'\r' || *rsArguments == L'\n')
		rsArguments++;
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

/* ***  Теперь - собственно макросы  *** */

// Проверка, есть ли ConEmu GUI. Функцию мог бы плагин и сам обработать,
// но для "общности" возвращаем "Yes" здесь
LPWSTR CConEmuMacro::IsConEmu(LPWSTR asArgs)
{
	LPWSTR pszResult = _wcsdup(L"Yes");
	return pszResult;
}

// Найти окно и активировать его. // LPWSTR asName
LPWSTR CConEmuMacro::FindEditor(LPWSTR asArgs)
{
	LPWSTR pszName = NULL;
	if (!GetNextString(asArgs, pszName))
		return NULL;
	// Пустые строки не допускаются
	if (!pszName || !*pszName)
		return NULL;
	return FindFarWindowHelper(3/*WTYPE_EDITOR*/, pszName);
}
// Найти окно и активировать его. // LPWSTR asName
LPWSTR CConEmuMacro::FindViewer(LPWSTR asArgs)
{
	LPWSTR pszName = NULL;
	if (!GetNextString(asArgs, pszName))
		return NULL;
	// Пустые строки не допускаются
	if (!pszName || !*pszName)
		return NULL;
	return FindFarWindowHelper(2/*WTYPE_VIEWER*/, pszName);
}
// Найти окно и активировать его. // int nWindowType, LPWSTR asName
LPWSTR CConEmuMacro::FindFarWindow(LPWSTR asArgs)
{
	int nWindowType = 0;
	LPWSTR pszName = NULL;
	if (!GetNextString(asArgs, pszName))
		return NULL;
	// Пустые строки не допускаются
	if (!pszName || !*pszName)
		return NULL;
	return FindFarWindowHelper(nWindowType, pszName);
}
LPWSTR CConEmuMacro::FindFarWindowHelper(
		int anWindowType/*Panels=1, Viewer=2, Editor=3, |(Elevated=0x100), |(NotElevated=0x200)*/,
		LPWSTR asName)
{
	CRealConsole* pRCon, *pActiveRCon = NULL;
	CVirtualConsole* pVCon;
	ConEmuTab tab;
	int iFound = 0;
	DWORD nElevateFlag = (anWindowType & 0x300);

	pVCon = gConEmu.ActiveCon();
	if (pVCon)
		pActiveRCon = pVCon->RCon();
	
	for (int i = 0; !iFound && (i < MAX_CONSOLE_COUNT); i++)
	{
		if (!(pVCon = gConEmu.GetVCon(i)))
			break;
		if (!(pRCon = pVCon->RCon()) || pRCon == pActiveRCon)
			continue;
		
		for (int j = 0; !iFound; j++)
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
					gConEmu.Activate(pVCon);
				}
				else
				{
					iFound = -1;
				}
				break;
			}
		}
	}
	
	int cchSize = 32;
	LPWSTR pszResult = (LPWSTR)malloc(2*cchSize);
	if (iFound > 0)
		StringCchPrintf(pszResult, cchSize, L"Found:%i", iFound);
	else if (iFound == -1)
		StringCchCopy(pszResult, cchSize, L"Blocked");
	else
		StringCchCopy(pszResult, cchSize, L"NotFound");
	return pszResult;
}

// Минимизировать окно (можно насильно в трей) // [int nForceToTray=0/1]
LPWSTR CConEmuMacro::WindowMinimize(LPWSTR asArgs)
{
	int nForceToTray = 0;
	GetNextInt(asArgs, nForceToTray);
	if (nForceToTray)
		Icon.HideWindowToTray();
	else
		PostMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	return _wcsdup(L"OK");
}

// MessageBox(ConEmu,asText,asTitle,anType) // LPWSTR asText [, LPWSTR asTitle[, int anType]]
LPWSTR CConEmuMacro::MsgBox(LPWSTR asArgs)
{
	LPWSTR pszText = NULL, pszTitle = NULL;
	int nButtons = 0;
	if (GetNextString(asArgs, pszText))
		if (GetNextString(asArgs, pszTitle))
			GetNextInt(asArgs, nButtons);
	
	int nRc = MessageBox(NULL, pszText ? pszText : L"", pszTitle ? pszTitle : L"ConEmu Macro", nButtons);
	
	switch (nRc)
	{
		case IDABORT:
			return _wcsdup(L"Abort");
		case IDCANCEL:
			return _wcsdup(L"Cancel");
		case IDIGNORE:
			return _wcsdup(L"Ignore");
		case IDNO:
			return _wcsdup(L"No");
		case IDOK:
			return _wcsdup(L"OK");
		case IDRETRY:
			return _wcsdup(L"Retry");
		case IDYES:
			return _wcsdup(L"Yes");
	}
	
	return NULL;
}

// Изменить размер шрифта. "+N" - увеличить на N пунктов, "-N" - уменьшить, "N" - указан размер. Возвращает - предыдущий размер
LPWSTR CConEmuMacro::FontSetSize(LPWSTR asArgs)
{
	TODO("CConEmuMacro::FontSetSize");
	return NULL;
}
