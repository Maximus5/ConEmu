
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


// Общая функция, для обработки любого известного макроса
LPWSTR CConEmuMacro::ExecuteMacro(LPWSTR asMacro, CRealConsole* apRCon)
{
	// Skip white-spaces
	SkipWhiteSpaces(asMacro);

	if (!asMacro || !*asMacro)
	{
		return lstrdup(L"Empty Macro");
	}

	if (*asMacro == L'"')
	{
		return lstrdup(L"Invalid Macro (remove quotas)");
	}

	LPWSTR pszAllResult = NULL;

	while (*asMacro)
	{
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

					SkipWhiteSpaces(asMacro);

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
		else if (!lstrcmpi(szFunction, L"Close"))
			pszResult = Close(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"FindEditor"))
			pszResult = FindEditor(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"FindViewer"))
			pszResult = FindViewer(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"FindFarWindow"))
			pszResult = FindFarWindow(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"WindowFullscreen"))
			pszResult = WindowFullscreen(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"WindowMaximize"))
			pszResult = WindowMaximize(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"WindowMinimize"))
			pszResult = WindowMinimize(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"WindowMode"))
			pszResult = WindowMode(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"MsgBox"))
			pszResult = MsgBox(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"Menu"))
			pszResult = Menu(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"FontSetSize"))
			pszResult = FontSetSize(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"FontSetName"))
			pszResult = FontSetName(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"IsRealVisible"))
			pszResult = IsRealVisible(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"IsConsoleActive"))
			pszResult = IsConsoleActive(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"Paste"))
			pszResult = Paste(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"Print"))
			pszResult = Print(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"Progress"))
			pszResult = Progress(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"Rename"))
			pszResult = Rename(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"Shell") || !lstrcmpi(szFunction, L"ShellExecute"))
			pszResult = Shell(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"SetOption"))
			pszResult = SetOption(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"Tab") || !lstrcmpi(szFunction, L"Tabs") || !lstrcmpi(szFunction, L"TabControl"))
			pszResult = Tab(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"Task"))
			pszResult = Task(asMacro, apRCon);
		else if (!lstrcmpi(szFunction, L"Status") || !lstrcmpi(szFunction, L"StatusBar") || !lstrcmpi(szFunction, L"StatusControl"))
			pszResult = Status(asMacro, apRCon);
		else
			pszResult = lstrdup(L"UnknownMacro"); // Неизвестная функция

		if (!pszAllResult)
		{
			pszAllResult = pszResult;
		}
		else
		{
			TODO("Добавить результат через разделитель ';'");
		}

		TODO("обработать следующую функцию");
		break;
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
			LPWSTR pszFindQ = wcschr(rsString, L'"');

			/*
			12345678
			1""2""3
			1"2""3
			1"2"3
			12345678
			*/
			while (pszFindQ && (pszFindQ[1] == L'"'))
			{
				LPWSTR pszNext = wcschr(pszFindQ+2, L'"');

				if (!pszNext) pszNext = rsString+nLen;

				size_t nSkip = pszFindQ - pszSrc;
				pszDst += nSkip;
				size_t nCopy = pszNext - pszFindQ - 2;

				if (nCopy > 0)
				{
					wmemmove(pszDst, pszFindQ+1, nCopy+1);
				}

				pszDst ++;
				// Next
				pszSrc = pszFindQ+2;
				pszFindQ = pszNext;
			}

			size_t nLeft = _tcslen(pszSrc);

			if (nLeft > 0)
				pszDst += nLeft;

			_ASSERTE(*pszDst != 0);
			_ASSERTE((pszFindQ == NULL) || (*pszFindQ == 0));
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

LPWSTR CConEmuMacro::Close(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPWSTR pszResult = NULL;
	int nCmd = 0, nFlags = 0;

	if (GetNextInt(asArgs, nCmd))
		GetNextInt(asArgs, nFlags);

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
LPWSTR CConEmuMacro::WindowFullscreen(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPWSTR pszRc = WindowMode(NULL, NULL);

	gpConEmu->OnAltEnter();

	return pszRc;
}

// Fullscreen
LPWSTR CConEmuMacro::WindowMaximize(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPWSTR pszRc = WindowMode(NULL, NULL);

	gpConEmu->OnAltF9();

	return pszRc;
}

// Минимизировать окно (можно насильно в трей) // [int nForceToTray=0/1]
LPWSTR CConEmuMacro::WindowMinimize(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPWSTR pszRc = WindowMode(NULL, NULL);

	int nForceToTray = 0;
	GetNextInt(asArgs, nForceToTray);

	if (nForceToTray)
		Icon.HideWindowToTray();
	else
		PostMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);

	return pszRc;
}

// Вернуть текущий статус: NORMAL/MAXIMIZED/FULLSCREEN/MINIMIZED
// Или установить новый
LPWSTR CConEmuMacro::WindowMode(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPCWSTR pszRc = NULL;
	LPWSTR pszMode = NULL;

	LPCWSTR sFS  = L"FS";
	LPCWSTR sMAX = L"MAX";
	LPCWSTR sMIN = L"MIN";
	LPCWSTR sTSA = L"TSA";
	LPCWSTR sNOR = L"NOR";

	if (asArgs && GetNextString(asArgs, pszMode))
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
LPWSTR CConEmuMacro::Menu(LPWSTR asArgs, CRealConsole* apRCon)
{
	int nType = 0;
	GetNextInt(asArgs, nType);

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

// Paste (<Cmd>[,"<Text>"])
LPWSTR CConEmuMacro::Paste(LPWSTR asArgs, CRealConsole* apRCon)
{
	int nCommand = 0;
	LPWSTR pszText = NULL;

	if (!apRCon)
		return lstrdup(L"InvalidArg");

	if (GetNextInt(asArgs, nCommand))
	{
		bool bFirstLineOnly = (nCommand & 1) != 0;
		bool bNoConfirm = (nCommand & 2) != 0;

		wchar_t* pszChooseBuf = NULL;

		if (!(nCommand >= 0 && nCommand <= 8))
		{
			return lstrdup(L"InvalidArg");
		}

		if (GetNextString(asArgs, pszText))
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
LPWSTR CConEmuMacro::Print(LPWSTR asArgs, CRealConsole* apRCon)
{
	if (!apRCon)
		return lstrdup(L"InvalidArg");

	LPWSTR pszText = NULL;
	if (GetNextString(asArgs, pszText))
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
LPWSTR CConEmuMacro::Progress(LPWSTR asArgs, CRealConsole* apRCon)
{
	int nType = 0, nValue = 0;
	LPWSTR pszName = NULL;
	if (!apRCon)
		return lstrdup(L"InvalidArg");

	if (GetNextInt(asArgs, nType))
	{
		if (!(nType >= 0 && nType <= 5))
		{
			return lstrdup(L"InvalidArg");
		}

		if (nType <= 2)
			GetNextInt(asArgs, nValue);
		else if (nType == 4 || nType == 5)
			GetNextString(asArgs, pszName);

		apRCon->SetProgress(nType, nValue, pszName);

		return lstrdup(L"OK");
	}

	return lstrdup(L"InvalidArg");
}

// Rename(<Type>,"<Title>")
LPWSTR CConEmuMacro::Rename(LPWSTR asArgs, CRealConsole* apRCon)
{
	int nType = 0;
	LPWSTR pszTitle = NULL;

	if (!apRCon)
		return lstrdup(L"InvalidArg");

	if (GetNextInt(asArgs, nType))
	{
		if (!(nType == 0 || nType == 1))
		{
			return lstrdup(L"InvalidArg");
		}

		if (!GetNextString(asArgs, pszTitle) || !*pszTitle)
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

// SetOption("<Name>",<Value>)
LPWSTR CConEmuMacro::SetOption(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPWSTR pszName = NULL;
	int nValue = 0;

	if (!GetNextString(asArgs, pszName))
		return lstrdup(L"InvalidArg");

	if (!lstrcmpi(pszName, L"QuakeAutoHide"))
	{
		// SetOption("QuakeAutoHide",0) - Hide on demand only
		// SetOption("QuakeAutoHide",1) - Enable autohide on focus lose
		// SetOption("QuakeAutoHide",2) - Switch autohide
		GetNextInt(asArgs, nValue);
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
LPWSTR CConEmuMacro::Shell(LPWSTR asArgs, CRealConsole* apRCon)
{
	wchar_t* pszRc = NULL;
	LPWSTR pszOper = NULL, pszFile = NULL, pszParm = NULL, pszDir = NULL;
	LPWSTR pszBuf = NULL;
	bool bDontQuote = false;
	int nShowCmd = SW_SHOWNORMAL;
	
	if (GetNextString(asArgs, pszOper))
	{
		CVConGuard VCon(apRCon ? apRCon->VCon() : NULL);

		if (GetNextString(asArgs, pszFile))
		{
			if (!GetNextString(asArgs, pszParm))
				pszParm = NULL;
			else if (!GetNextString(asArgs, pszDir))
				pszDir = NULL;
			else if (!GetNextInt(asArgs, nShowCmd))
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
LPWSTR CConEmuMacro::Status(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPWSTR pszResult = NULL;
	int nStatusCmd = 0;
	int nParm = 0;
	LPWSTR pszText = NULL;

	if (!GetNextInt(asArgs, nStatusCmd))
		nStatusCmd = 0;

	switch (nStatusCmd)
	{
	case csc_ShowHide:
		GetNextInt(asArgs, nParm);
		gpConEmu->StatusCommand(csc_ShowHide, nParm);
		pszResult = lstrdup(L"OK");
		break;
	case csc_SetStatusText:
		if (apRCon)
		{
			GetNextString(asArgs, pszText);
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
LPWSTR CConEmuMacro::Tab(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPWSTR pszResult = NULL;

	int nTabCmd = 0;
	if (GetNextInt(asArgs, nTabCmd))
	{
		int nParm = 0;
		GetNextInt(asArgs, nParm);

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
LPWSTR CConEmuMacro::Task(LPWSTR asArgs, CRealConsole* apRCon)
{
	LPCWSTR pszResult = NULL;
	LPCWSTR pszName = NULL, pszDir = NULL;
	wchar_t* pszBuf = NULL;
	int nTaskIndex = 0;

	if (*asArgs == L'"')
	{
		LPWSTR pszArg = NULL;
		if (GetNextString(asArgs, pszArg))
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
		if (GetNextInt(asArgs, nTaskIndex) && (nTaskIndex > 0))
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
