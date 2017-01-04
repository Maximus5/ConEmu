
/*
Copyright (c) 2014-2017 Maximus5
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

#include <windows.h>
#include "SetTypes.h"

typedef BYTE PasteLinesMode;
const PasteLinesMode
	plm_Default    = 1,
	plm_MultiLine  = 2,
	plm_SingleLine = 3,
	plm_FirstLine  = 4,
	plm_Nothing    = 0
;

struct AppSettings
{
	size_t   cchNameMax;
	wchar_t* AppNames; // "far.exe|far64.exe" и т.п.
	wchar_t* AppNamesLwr; // For internal use
	BYTE Elevated; // 00 - unimportant, 01 - elevated, 02 - nonelevated

	//const COLORREF* Palette/*[0x20]*/; // текущая палитра (Fade/не Fade)

	bool OverridePalette; // Palette+Extend
	wchar_t szPaletteName[128];
	//TODO: Тут хорошо бы индекс палитры хранить...
	int GetPaletteIndex() const;
	void SetPaletteName(LPCWSTR asNewPaletteName);
	void ResetPaletteIndex();

	//reg->Load(L"ExtendColors", isExtendColors);
	bool isExtendColors;
	bool ExtendColors() const;
	//reg->Load(L"ExtendColorIdx", nExtendColorIdx);
	BYTE nExtendColorIdx; // 0..15
	BYTE ExtendColorIdx() const;

	BYTE nTextColorIdx; // 0..15,16
	BYTE TextColorIdx() const;
	BYTE nBackColorIdx; // 0..15,16
	BYTE BackColorIdx() const;
	BYTE nPopTextColorIdx; // 0..15,16
	BYTE PopTextColorIdx() const;
	BYTE nPopBackColorIdx; // 0..15,16
	BYTE PopBackColorIdx() const;


	bool OverrideExtendFonts;
	//reg->Load(L"ExtendFonts", isExtendFonts);
	bool isExtendFonts;
	bool ExtendFonts() const;
	//reg->Load(L"ExtendFontNormalIdx", nFontNormalColor);
	BYTE nFontNormalColor; // 0..15
	BYTE FontNormalColor() const;
	//reg->Load(L"ExtendFontBoldIdx", nFontBoldColor);
	BYTE nFontBoldColor;   // 0..15
	BYTE FontBoldColor() const;
	//reg->Load(L"ExtendFontItalicIdx", nFontItalicColor);
	BYTE nFontItalicColor; // 0..15
	BYTE FontItalicColor() const;

	bool OverrideCursor;
	// *** Active ***
	////reg->Load(L"CursorType", isCursorType);
	////BYTE isCursorType; // 0 - Horz, 1 - Vert, 2 - Hollow-block
	//reg->Load(L"CursorTypeActive", CursorActive.Raw);
	//reg->Load(L"CursorTypeInactive", CursorActive.Raw);
	CECursorType CursorActive; // storage
	CECursorType CursorInactive; // storage
	CECursorType Cursor(bool bActive) const;
	CECursorStyle CursorStyle(bool bActive) const;
	////reg->Load(L"CursorBlink", isCursorBlink);
	////bool isCursorBlink;
	bool CursorBlink(bool bActive) const;
	////reg->Load(L"CursorColor", isCursorColor);
	////bool isCursorColor;
	bool CursorColor(bool bActive) const;
	////reg->Load(L"CursorBlockInactive", isCursorBlockInactive);
	////bool isCursorBlockInactive;
	////bool CursorBlockInactive() const;
	////reg->Load(L"CursorIgnoreSize", isCursorIgnoreSize);
	////bool isCursorIgnoreSize;
	bool CursorIgnoreSize(bool bActive) const;
	////reg->Load(L"CursorFixedSize", nCursorFixedSize);
	////BYTE nCursorFixedSize; // в процентах
	BYTE CursorFixedSize(bool bActive) const;
	//reg->Load(L"CursorMinSize", nCursorMinSize);
	//BYTE nCursorMinSize; // в пикселях
	BYTE CursorMinSize(bool bActive) const;

	bool OverrideClipboard;
	// *** Copying
	//reg->Load(L"ClipboardDetectLineEnd", isCTSDetectLineEnd);
	bool isCTSDetectLineEnd; // cbCTSDetectLineEnd
	bool CTSDetectLineEnd() const;
	//reg->Load(L"ClipboardBashMargin", isCTSBashMargin);
	bool isCTSBashMargin; // cbCTSBashMargin
	bool CTSBashMargin() const;
	//reg->Load(L"ClipboardTrimTrailing", isCTSTrimTrailing);
	BYTE isCTSTrimTrailing; // cbCTSTrimTrailing: 0 - нет, 1 - да, 2 - только для stream-selection
	BYTE CTSTrimTrailing() const;
	//reg->Load(L"ClipboardEOL", isCTSEOL);
	BYTE isCTSEOL; // cbCTSEOL: 0="CR+LF", 1="LF", 2="CR"
	BYTE CTSEOL() const;
	//reg->Load(L"ClipboardArrowStart", pApp->isCTSShiftArrowStart);
	bool isCTSShiftArrowStart;
	bool CTSShiftArrowStart() const;
	// *** Pasting
	//reg->Load(L"ClipboardAllLines", isPasteAllLines);
	PasteLinesMode isPasteAllLines;
	PasteLinesMode PasteAllLines() const;
	//reg->Load(L"ClipboardFirstLine", isPasteFirstLine);
	PasteLinesMode isPasteFirstLine;
	PasteLinesMode PasteFirstLine() const;
	// *** Prompt
	// cbCTSClickPromptPosition
	//reg->Load(L"ClipboardClickPromptPosition", isCTSClickPromptPosition);
	//0 - off, 1 - force, 2 - try to detect "ReadConsole" (don't use 2 in bash)
	BYTE isCTSClickPromptPosition; // cbCTSClickPromptPosition
	BYTE CTSClickPromptPosition() const;
	BYTE isCTSDeleteLeftWord; // cbCTSDeleteLeftWord
	BYTE CTSDeleteLeftWord() const;


	void SetNames(LPCWSTR asAppNames);

	void FreeApps();
};
