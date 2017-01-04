
/*
Copyright (c) 2011-2017 Maximus5
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

#define GUI_MACRO_VERSION 2

struct CESERVER_REQ_GUIMACRO;

namespace ConEmuMacro
{
	// Общая функция, для обработки любого известного макроса
	LPWSTR ExecuteMacro(LPWSTR asMacro, CRealConsole* apRCon, bool abFromPlugin = false, CESERVER_REQ_GUIMACRO* Opt = NULL);
	// Конвертация из "старого" в "новый" формат
	// Старые макросы хранились как "Verbatim" но без префикса
	LPWSTR ConvertMacro(LPCWSTR asMacro, BYTE FromVersion, bool bShowErrorTip = true);
	// Some functions must be executed in main thread
	// but they may be called from console
	LRESULT ExecuteMacroSync(WPARAM wParam, LPARAM lParam);
	// Helper to concatenate macros.
	// They must be ‘concatenatable’, example: Print("abc"); Print("Qqq")
	// But following WILL NOT work: Print: Abd qqq
	LPCWSTR ConcatMacro(LPWSTR& rsMacroList, LPCWSTR asAddMacro);

	#ifdef _DEBUG
	void UnitTests();
	#endif
};
