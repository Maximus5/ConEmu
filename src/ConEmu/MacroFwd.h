
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

#pragma once

#include "Macro.h"

/* ********************************* */
/* ****** Forward definitions ****** */
/* ********************************* */
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

	int Int;
	LPWSTR Str;
	GuiMacro* Macro;
};

struct GuiMacro
{
	size_t  cbSize;
	LPCWSTR szFunc;
	wchar_t chFuncTerm; // L'(', L':', L' ' - delimiter between func name and arguments

	size_t  argc;
	GuiMacroArg* argv; // No need to release mem, buffer allocated for the full GuiMacro data

	wchar_t* AsString();
	bool GetIntArg(size_t idx, int& val);
	bool GetStrArg(size_t idx, LPWSTR& val);
	bool GetRestStrArgs(size_t idx, LPWSTR& val);
	bool IsIntArg(size_t idx);
	bool IsStrArg(size_t idx);
};

namespace ConEmuMacro
{
	GuiMacro* GetNextMacro(LPWSTR& asString, bool abConvert, wchar_t** rsErrMsg);

	/* ****************************** */
	/* ****** Macros functions ****** */
	/* ****************************** */

	// Диалог About(["Tab"])
	LPWSTR About(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// AffinityPriority([Affinity,Priority])
	LPWSTR AffinityPriority(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// AltNumber([Base])
	LPWSTR AltNumber(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Attach - console or ChildGui by PID
	LPWSTR Attach(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Break (0=CtrlC; 1=CtrlBreak)
	LPWSTR Break(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Закрыть/прибить текущую консоль
	LPWSTR Close(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Change execution context (apRCon)
	LPWSTR Context(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Copy (<What>)
	LPWSTR Copy(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Debug (<Action>)
	LPWSTR Debug(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Detach
	LPWSTR Detach(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Environment variables
	LPWSTR EnvironmentReload(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	LPWSTR EnvironmentList(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Найти окно и активировать его. // int nWindowType/*Panels=1, Viewer=2, Editor=3*/, LPWSTR asName
	LPWSTR FindEditor(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	LPWSTR FindFarWindow(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	LPWSTR FindViewer(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
		LPWSTR FindFarWindowHelper(CEFarWindowType anWindowType/*Panels=1, Viewer=2, Editor=3*/, LPWSTR asName, CRealConsole* apRCon, bool abFromPlugin); // helper, это не макро-фукнция
	// Flash
	LPWSTR Flash(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Изменить имя основного шрифта. string
	LPWSTR FontSetName(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Изменить размер шрифта. int nRelative, int N
	LPWSTR FontSetSize(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// GetInfo("<Var1>"[,"<Var2>"[,...]])
	LPWSTR GetInfo(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// GetOption("<Name>")
	LPWSTR GetOption(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// GroupInput [<cmd>]
	LPWSTR GroupInput(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Change 'Highlight row/col' under mouse. Locally in current VCon.
	LPWSTR HighlightMouse(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Проверка, есть ли ConEmu GUI. Функцию мог бы и сам плагин обработать, но для "общности" возвращаем "Yes" здесь
	LPWSTR IsConEmu(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Проверка, активна ли RealConsole
	LPWSTR IsConsoleActive(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Проверка, видима ли RealConsole
	LPWSTR IsRealVisible(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Keys("<Combo1>"[,"<Combo2>"[...]])
	LPWSTR Keys(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Menu(Type)
	LPWSTR Menu(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// MsgBox(ConEmu,asText,asTitle,anType) // LPWSTR asText [, LPWSTR asTitle[, int anType]]
	LPWSTR MsgBox(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Palette([<Cmd>[,"<NewPalette>"]])
	LPWSTR Palette(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Paste (<Cmd>[,"<Text>"[,"<Text2>"[...]]])
	LPWSTR Paste(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// PasteExplorerPath (<DoCd>,<SetFocus>)
	LPWSTR PasteExplorerPath(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// PasteFile (<Cmd>[,"<File>"[,"<CommentMark>"]])
	LPWSTR PasteFile(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Pause
	LPWSTR Pause(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// print("<Text>") - alias for Paste(2,"<Text>")
	LPWSTR Print(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Progress(<Type>[,<Value>])
	LPWSTR Progress(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Recreate(<Action>[,<Confirm>[,<AsAdmin>]]), alias "Create"
	LPWSTR Recreate(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Rename(<Type>,"<Title>")
	LPWSTR Rename(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Scroll(<Type>,<Direction>,<Count=1>)
	LPWSTR Scroll(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Select(<Type>,<DX>,<DY>)
	LPWSTR Select(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// SetDpi(dpi)
	LPWSTR SetDpi(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// SetOption("<Name>",<Value>)
	LPWSTR SetOption(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// SetParentHWND(<NewParentHWND>)
	LPWSTR SetParentHWND(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Open Settings dialog
	LPWSTR Settings(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Shell (ShellExecute)
	LPWSTR Shell(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Sleep (ms)
	LPWSTR Sleep(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Split
	LPWSTR Split(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Status
	LPWSTR Status(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Tabs
	LPWSTR Tab(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Task
	LPWSTR Task(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// TaskAdd("Name","Commands"[,"GuiArgs"[,Flags]])
	LPWSTR TaskAdd(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// TermMode(<Mode>[,<Action>])
	LPWSTR TermMode(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Transparency
	LPWSTR Transparency(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
		LPWSTR TransparencyHelper(int nCmd, int nValue); // helper, это не макро-фукнция
	// Unfasten
	LPWSTR Unfasten(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Update
	LPWSTR Update(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Wiki(["Page"])
	LPWSTR Wiki(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Fullscreen
	LPWSTR WindowFullscreen(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Maximize
	LPWSTR WindowMaximize(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Минимизировать окно (можно насильно в трей) // [int nForceToTray=0/1]
	LPWSTR WindowMinimize(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Вернуть текущий статус: NOR/MAX/FS/MIN/TSA
	LPWSTR WindowMode(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Change window pos and size, same as ‘Apply’ button in the ‘Size & Pos’ page of the Settings dialog
	LPWSTR WindowPosSize(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Write all strings to console. Use carefully!
	LPWSTR Write(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
	// Установить Zoom для шрифта. 100% и т.п.
	LPWSTR Zoom(GuiMacro* p, CRealConsole* apRCon, bool abFromPlugin);
}
