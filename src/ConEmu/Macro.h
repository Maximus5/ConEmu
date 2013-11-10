
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

#define GUI_MACRO_VERSION 2

class CRealConsole;

struct GuiMacro;
struct GuiMacroArg;

enum GuiMacroArgType
{
	gmt_Int,
	gmt_Hex,
	gmt_Str,
	gmt_VStr,
	gmt_Fn, // Reserved
};

struct GuiMacroArg
{
	GuiMacroArgType Type;

	#ifdef _WIN64
	DWORD Pad;
	#endif

	union
	{
		int Int;
		LPWSTR Str;
		GuiMacro* Macro;
	};
};

struct GuiMacro
{
	size_t  cbSize;
	LPCWSTR szFunc;
	wchar_t chFuncTerm; // L'(', L':', L' ' - delimiter between func name and arguments

	size_t  argc;
	GuiMacroArg* argv;

	wchar_t* AsString();
	bool GetIntArg(size_t idx, int& val);
	bool GetStrArg(size_t idx, LPWSTR& val);
	bool IsIntArg(size_t idx);
	bool IsStrArg(size_t idx);
};


class CConEmuMacro
{
	public:
		// Вообще не используется, все статическое
		CConEmuMacro() {};
		~CConEmuMacro() {};
	public:
		// Общая функция, для обработки любого известного макроса
		static LPWSTR ExecuteMacro(LPWSTR asMacro, CRealConsole* apRCon, bool abFromPlugin = false);
		// Конвертация из "старого" в "новый" формат
		static LPWSTR ConvertMacro(LPCWSTR asMacro, BYTE FromVersion, bool bShowErrorTip = true);
	protected:
		// Функции для парсера параметров
		static LPWSTR GetNextString(LPWSTR& rsArguments, LPWSTR& rsString, bool bColonDelim = false);
		static LPWSTR GetNextArg(LPWSTR& rsArguments, LPWSTR& rsArg);
		static LPWSTR GetNextInt(LPWSTR& rsArguments, int& rnValue);
		static void SkipWhiteSpaces(LPWSTR& rsString);
		static GuiMacro* GetNextMacro(LPWSTR& asString, bool abConvert, wchar_t** rsErrMsg);
	public:
		// Теперь - собственно макросы

		// Закрыть/прибить текущую консоль
		static LPWSTR Close(GuiMacro* p, CRealConsole* apRCon);
		// Найти окно и активировать его. // int nWindowType/*Panels=1, Viewer=2, Editor=3*/, LPWSTR asName
		static LPWSTR FindEditor(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin = false);
		static LPWSTR FindViewer(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin = false);
		static LPWSTR FindFarWindow(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin = false);
		static LPWSTR FindFarWindowHelper(CEFarWindowType anWindowType/*Panels=1, Viewer=2, Editor=3*/, LPWSTR asName, CRealConsole* apRCon, bool abFromPlugin = false); // helper, это не макро-фукнция
		// Изменить имя основного шрифта. string
		static LPWSTR FontSetName(GuiMacro* p, CRealConsole* apRCon);
		// Изменить размер шрифта. int nRelative, int N
		static LPWSTR FontSetSize(GuiMacro* p, CRealConsole* apRCon);
		// Проверка, есть ли ConEmu GUI. Функцию мог бы и сам плагин обработать, но для "общности" возвращаем "Yes" здесь
		static LPWSTR IsConEmu(GuiMacro* p, CRealConsole* apRCon);
		// Проверка, активна ли RealConsole
		static LPWSTR IsConsoleActive(GuiMacro* p, CRealConsole* apRCon);
		// Проверка, видима ли RealConsole
		static LPWSTR IsRealVisible(GuiMacro* p, CRealConsole* apRCon);
		// Menu(Type)
		static LPWSTR Menu(GuiMacro* p, CRealConsole* apRCon);
		// MessageBox(ConEmu,asText,asTitle,anType) // LPWSTR asText [, LPWSTR asTitle[, int anType]]
		static LPWSTR MsgBox(GuiMacro* p, CRealConsole* apRCon);
		// Copy (<What>)
		static LPWSTR Copy(GuiMacro* p, CRealConsole* apRCon);
		// Paste (<Cmd>[,"<Text>"])
		static LPWSTR Paste(GuiMacro* p, CRealConsole* apRCon);
		// print("<Text>") - alias for Paste(2,"<Text>")
		static LPWSTR Print(GuiMacro* p, CRealConsole* apRCon);
		// Keys("<Combo1>"[,"<Combo2>"[...]])
		static LPWSTR Keys(GuiMacro* p, CRealConsole* apRCon);
		// Progress(<Type>[,<Value>])
		static LPWSTR Progress(GuiMacro* p, CRealConsole* apRCon);
		// Rename(<Type>,"<Title>")
		static LPWSTR Rename(GuiMacro* p, CRealConsole* apRCon);
		// Select(<Type>,<DX>,<DY>)
		static LPWSTR Select(GuiMacro* p, CRealConsole* apRCon);
		// SetOption("<Name>",<Value>)
		static LPWSTR SetOption(GuiMacro* p, CRealConsole* apRCon);
		// Shell (ShellExecute)
		static LPWSTR Shell(GuiMacro* p, CRealConsole* apRCon);
		// Split
		static LPWSTR Split(GuiMacro* p, CRealConsole* apRCon);
		// Status
		static LPWSTR Status(GuiMacro* p, CRealConsole* apRCon);
		// Tabs
		static LPWSTR Tab(GuiMacro* p, CRealConsole* apRCon);
		// Task
		static LPWSTR Task(GuiMacro* p, CRealConsole* apRCon);
		// Fullscreen
		static LPWSTR WindowFullscreen(GuiMacro* p, CRealConsole* apRCon);
		// Maximize
		static LPWSTR WindowMaximize(GuiMacro* p, CRealConsole* apRCon);
		// Минимизировать окно (можно насильно в трей) // [int nForceToTray=0/1]
		static LPWSTR WindowMinimize(GuiMacro* p, CRealConsole* apRCon);
		// Вернуть текущий статус: NOR/MAX/FS/MIN/TSA
		static LPWSTR WindowMode(GuiMacro* p, CRealConsole* apRCon);
};
