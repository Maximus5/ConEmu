
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


#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"

#include "Options.h"
#include "SetAppSettings.h"

bool AppSettings::ExtendColors() const
{
	return (OverridePalette || !AppNames) ? isExtendColors : gpSet->AppStd.isExtendColors;
}

//reg->Load(L"ExtendColorIdx", nExtendColorIdx);
//BYTE nExtendColorIdx; // 0..15
BYTE AppSettings::ExtendColorIdx() const
{
	return (OverridePalette || !AppNames) ? nExtendColorIdx : gpSet->AppStd.nExtendColorIdx;
}

//BYTE nTextColorIdx; // 0..15,16
BYTE AppSettings::TextColorIdx() const
{
	return (OverridePalette || !AppNames) ? nTextColorIdx : gpSet->AppStd.nTextColorIdx;
}

//BYTE nBackColorIdx; // 0..15,16
BYTE AppSettings::BackColorIdx() const
{
	return (OverridePalette || !AppNames) ? nBackColorIdx : gpSet->AppStd.nBackColorIdx;
}

//BYTE nPopTextColorIdx; // 0..15,16
BYTE AppSettings::PopTextColorIdx() const
{
	return (OverridePalette || !AppNames) ? nPopTextColorIdx : gpSet->AppStd.nPopTextColorIdx;
}

//BYTE nPopBackColorIdx; // 0..15,16
BYTE AppSettings::PopBackColorIdx() const
{
	return (OverridePalette || !AppNames) ? nPopBackColorIdx : gpSet->AppStd.nPopBackColorIdx;
}

//bool OverrideExtendFonts;
//reg->Load(L"ExtendFonts", isExtendFonts);
//bool isExtendFonts;
bool AppSettings::ExtendFonts() const
{
	return (OverrideExtendFonts || !AppNames) ? isExtendFonts : gpSet->AppStd.isExtendFonts;
}

//reg->Load(L"ExtendFontNormalIdx", nFontNormalColor);
//BYTE nFontNormalColor; // 0..15
BYTE AppSettings::FontNormalColor() const
{
	return (OverrideExtendFonts || !AppNames) ? nFontNormalColor : gpSet->AppStd.nFontNormalColor;
}

//reg->Load(L"ExtendFontBoldIdx", nFontBoldColor);
//BYTE nFontBoldColor;   // 0..15
BYTE AppSettings::FontBoldColor() const
{
	return (OverrideExtendFonts || !AppNames) ? nFontBoldColor : gpSet->AppStd.nFontBoldColor;
}

//reg->Load(L"ExtendFontItalicIdx", nFontItalicColor);
//BYTE nFontItalicColor; // 0..15
BYTE AppSettings::FontItalicColor() const
{
	return (OverrideExtendFonts || !AppNames) ? nFontItalicColor : gpSet->AppStd.nFontItalicColor;
}

//bool OverrideCursor;
// *** Active ***
////reg->Load(L"CursorType", isCursorType);
////BYTE isCursorType; // 0 - Horz, 1 - Vert, 2 - Hollow-block
//reg->Load(L"CursorTypeActive", CursorActive.Raw);
//reg->Load(L"CursorTypeInactive", CursorActive.Raw);
//CECursorType CursorActive; // storage
//CECursorType CursorInactive; // storage

CECursorType AppSettings::Cursor(bool bActive) const
{
	return (OverrideCursor || !AppNames)
		? ((bActive || !CursorInactive.Used) ? CursorActive : CursorInactive)
		: ((bActive || !gpSet->AppStd.CursorInactive.Used) ? gpSet->AppStd.CursorActive : gpSet->AppStd.CursorInactive);
}

CECursorStyle AppSettings::CursorStyle(bool bActive) const
{
	return Cursor(bActive).CursorType;
}

////reg->Load(L"CursorBlink", isCursorBlink);
////bool isCursorBlink;
bool AppSettings::CursorBlink(bool bActive) const
{
	return Cursor(bActive).isBlinking;
}

////reg->Load(L"CursorColor", isCursorColor);
////bool isCursorColor;
bool AppSettings::CursorColor(bool bActive) const
{
	return Cursor(bActive).isColor;
}

////reg->Load(L"CursorBlockInactive", isCursorBlockInactive);
////bool isCursorBlockInactive;
////bool CursorBlockInactive() const { return (OverrideCursor || !AppNames) ? isCursorBlockInactive : gpSet->AppStd.isCursorBlockInactive; };
////reg->Load(L"CursorIgnoreSize", isCursorIgnoreSize);
////bool isCursorIgnoreSize;
bool AppSettings::CursorIgnoreSize(bool bActive) const
{
	return Cursor(bActive).isFixedSize;
}

////reg->Load(L"CursorFixedSize", nCursorFixedSize);
////BYTE nCursorFixedSize; // в процентах
BYTE AppSettings::CursorFixedSize(bool bActive) const
{
	return Cursor(bActive).FixedSize;
}

//reg->Load(L"CursorMinSize", nCursorMinSize);
//BYTE nCursorMinSize; // в пикселях
BYTE AppSettings::CursorMinSize(bool bActive) const
{
	return Cursor(bActive).MinSize;
}

//bool OverrideClipboard;
// *** Copying
//reg->Load(L"ClipboardDetectLineEnd", isCTSDetectLineEnd);
//bool isCTSDetectLineEnd; // cbCTSDetectLineEnd
bool AppSettings::CTSDetectLineEnd() const
{
	return (OverrideClipboard || !AppNames) ? isCTSDetectLineEnd : gpSet->AppStd.isCTSDetectLineEnd;
}

//reg->Load(L"ClipboardBashMargin", isCTSBashMargin);
//bool isCTSBashMargin; // cbCTSBashMargin
bool AppSettings::CTSBashMargin() const
{
	return (OverrideClipboard || !AppNames) ? isCTSBashMargin : gpSet->AppStd.isCTSBashMargin;
}

//reg->Load(L"ClipboardTrimTrailing", isCTSTrimTrailing);
//BYTE isCTSTrimTrailing; // cbCTSTrimTrailing: 0 - нет, 1 - да, 2 - только для stream-selection
BYTE AppSettings::CTSTrimTrailing() const
{
	return (OverrideClipboard || !AppNames) ? isCTSTrimTrailing : gpSet->AppStd.isCTSTrimTrailing;
}

//reg->Load(L"ClipboardEOL", isCTSEOL);
//BYTE isCTSEOL; // cbCTSEOL: 0="CR+LF", 1="LF", 2="CR"
BYTE AppSettings::CTSEOL() const
{
	return (OverrideClipboard || !AppNames) ? isCTSEOL : gpSet->AppStd.isCTSEOL;
}

//reg->Load(L"ClipboardArrowStart", pApp->isCTSShiftArrowStart);
//bool isCTSShiftArrowStart;
bool AppSettings::CTSShiftArrowStart() const
{
	return (OverrideClipboard || !AppNames) ? isCTSShiftArrowStart : gpSet->AppStd.isCTSShiftArrowStart;
}

// *** Pasting
//reg->Load(L"ClipboardAllLines", isPasteAllLines);
//bool isPasteAllLines;
PasteLinesMode AppSettings::PasteAllLines() const
{
	return (OverrideClipboard || !AppNames) ? isPasteAllLines : gpSet->AppStd.isPasteAllLines;
}

//reg->Load(L"ClipboardFirstLine", isPasteFirstLine);
//bool isPasteFirstLine;
PasteLinesMode AppSettings::PasteFirstLine() const
{
	return (OverrideClipboard || !AppNames) ? isPasteFirstLine : gpSet->AppStd.isPasteFirstLine;
}

// *** Prompt
// cbCTSClickPromptPosition
//reg->Load(L"ClipboardClickPromptPosition", isCTSClickPromptPosition);
//0 - off, 1 - force, 2 - try to detect "ReadConsole" (don't use 2 in bash)
//BYTE isCTSClickPromptPosition; // cbCTSClickPromptPosition
BYTE AppSettings::CTSClickPromptPosition() const
{
	return (OverrideClipboard || !AppNames) ? isCTSClickPromptPosition : gpSet->AppStd.isCTSClickPromptPosition;
}

//BYTE isCTSDeleteLeftWord; // cbCTSDeleteLeftWord
BYTE AppSettings::CTSDeleteLeftWord() const
{
	return (OverrideClipboard || !AppNames) ? isCTSDeleteLeftWord : gpSet->AppStd.isCTSDeleteLeftWord;
}

void AppSettings::SetNames(LPCWSTR asAppNames)
{
	size_t iLen = wcslen(asAppNames);

	if (!AppNames || !AppNamesLwr || (iLen >= cchNameMax))
	{
		SafeFree(AppNames);
		SafeFree(AppNamesLwr);

		cchNameMax = iLen+32;
		AppNames = (wchar_t*)malloc(cchNameMax*sizeof(wchar_t));
		AppNamesLwr = (wchar_t*)malloc(cchNameMax*sizeof(wchar_t));
		if (!AppNames || !AppNamesLwr)
		{
			_ASSERTE(AppNames!=NULL && AppNamesLwr!=NULL);
			return;
		}
	}

	_wcscpy_c(AppNames, iLen+1, asAppNames);
	_wcscpy_c(AppNamesLwr, iLen+1, asAppNames);
	CharLowerBuff(AppNamesLwr, iLen);
}

void AppSettings::FreeApps()
{
	SafeFree(AppNames);
	SafeFree(AppNamesLwr);
	cchNameMax = 0;
}
