
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


class CRealConsole;


class CConEmuMacro
{
	public:
		// Вообще не используется, все статическое
		CConEmuMacro() {};
		~CConEmuMacro() {};
	public:
		// Общая функция, для обработки любого известного макроса
		static LPWSTR ExecuteMacro(LPWSTR asMacro, CRealConsole* apRCon);
	protected:
		// Функции для парсера параметров
		static LPWSTR GetNextString(LPWSTR& rsArguments, LPWSTR& rsString);
		static LPWSTR GetNextArg(LPWSTR& rsArguments, LPWSTR& rsArg);
		static LPWSTR GetNextInt(LPWSTR& rsArguments, int& rnValue);
		static void SkipWhiteSpaces(LPWSTR& rsString);
	public:
		// Теперь - собственно макросы

		// Найти окно и активировать его. // int nWindowType/*Panels=1, Viewer=2, Editor=3*/, LPWSTR asName
		static LPWSTR FindEditor(LPWSTR asArgs, CRealConsole* apRCon);
		static LPWSTR FindViewer(LPWSTR asArgs, CRealConsole* apRCon);
		static LPWSTR FindFarWindow(LPWSTR asArgs, CRealConsole* apRCon);
		static LPWSTR FindFarWindowHelper(CEFarWindowType anWindowType/*Panels=1, Viewer=2, Editor=3*/, LPWSTR asName, CRealConsole* apRCon); // helper, это не макро-фукнция
		// Изменить имя основного шрифта. string
		static LPWSTR FontSetName(LPWSTR asArgs, CRealConsole* apRCon);
		// Изменить размер шрифта. int nRelative, int N
		static LPWSTR FontSetSize(LPWSTR asArgs, CRealConsole* apRCon);
		// Проверка, есть ли ConEmu GUI. Функцию мог бы и сам плагин обработать, но для "общности" возвращаем "Yes" здесь
		static LPWSTR IsConEmu(LPWSTR asArgs, CRealConsole* apRCon);
		// Проверка, активна ли RealConsole
		static LPWSTR IsConsoleActive(LPWSTR asArgs, CRealConsole* apRCon);
		// Проверка, видима ли RealConsole
		static LPWSTR IsRealVisible(LPWSTR asArgs, CRealConsole* apRCon);
		// MessageBox(ConEmu,asText,asTitle,anType) // LPWSTR asText [, LPWSTR asTitle[, int anType]]
		static LPWSTR MsgBox(LPWSTR asArgs, CRealConsole* apRCon);
		// Paste (<Cmd>[,"<Text>"])
		static LPWSTR Paste(LPWSTR asArgs, CRealConsole* apRCon);
		// Shell (ShellExecute)
		static LPWSTR Shell(LPWSTR asArgs, CRealConsole* apRCon);
		// Status
		static LPWSTR Status(LPWSTR asArgs, CRealConsole* apRCon);
		// Tabs
		static LPWSTR Tab(LPWSTR asArgs, CRealConsole* apRCon);
		// Task
		static LPWSTR Task(LPWSTR asArgs, CRealConsole* apRCon);
		// Fullscreen
		static LPWSTR WindowFullscreen(LPWSTR asArgs, CRealConsole* apRCon);
		// Maximize
		static LPWSTR WindowMaximize(LPWSTR asArgs, CRealConsole* apRCon);
		// Минимизировать окно (можно насильно в трей) // [int nForceToTray=0/1]
		static LPWSTR WindowMinimize(LPWSTR asArgs, CRealConsole* apRCon);
		// Вернуть текущий статус: NOR/MAX/FS/MIN/TSA
		static LPWSTR WindowMode(LPWSTR asArgs, CRealConsole* apRCon);
};
