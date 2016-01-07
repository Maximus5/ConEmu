
/*
Copyright (c) 2015 Maximus5
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
#include "OptionsClass.h"
#include "RealConsole.h"
#include "VConText.h"

#include "../common/wcchars.h"
#include "../common/wcwidth.h"


// Character substitues (ASCII 0..31)
const wchar_t gszAnalogues[32] =
{
	32, 9786, 9787, 9829, 9830, 9827, 9824, 8226, 9688, 9675, 9689, 9794, 9792, 9834, 9835, 9788,
	9658, 9668, 8597, 8252,  182,  167, 9632, 8616, 8593, 8595, 8594, 8592, 8735, 8596, 9650, 9660
};
//wchar_t getUnicodeAnalogue(uint inChar)
//{
//	return (inChar <= 31) ? gszAnalogues[inChar] : (wchar_t)inChar;
//}


// Это символы рамок и др. спец. символы
//#define isCharBorder(inChar) (inChar>=0x2013 && inChar<=0x266B)
bool isCharAltFont(wchar_t inChar)
{
	// Низя - нужно учитывать gpSet->isFixFarBorders там где это требуется, иначе пролетает gpSet->isEnhanceGraphics
	//if (!gpSet->isFixFarBorders)
	//	return false;
	return gpSet->CheckCharAltFont(inChar);
	//Settings::CharRanges *pcr = gpSet->icFixFarBorderRanges;
	//for (int i = 10; --i && pcr->bUsed; pcr++) {
	//	Settings::CharRanges cr = *pcr;
	//	if (inChar>=cr.cBegin && inChar<=cr.cEnd)
	//		return true;
	//}
	//return false;
	//if (inChar>=0x2013 && inChar<=0x266B)
	//    return true;
	//else
	//    return false;
	//if (gpSet->isFixFarBorders)
	//{
	//  //if (! (inChar > 0x2500 && inChar < 0x251F))
	//  if ( !(inChar > 0x2013/*En dash*/ && inChar < 0x266B/*Beamed Eighth Notes*/) /*&& inChar!=L'}'*/ )
	//      /*if (inChar != 0x2550 && inChar != 0x2502 && inChar != 0x2551 && inChar != 0x007D &&
	//      inChar != 0x25BC && inChar != 0x2593 && inChar != 0x2591 && inChar != 0x25B2 &&
	//      inChar != 0x2562 && inChar != 0x255F && inChar != 0x255A && inChar != 0x255D &&
	//      inChar != 0x2554 && inChar != 0x2557 && inChar != 0x2500 && inChar != 0x2534 && inChar != 0x2564) // 0x2520*/
	//      return false;
	//  else
	//      return true;
	//}
	//else
	//{
	//  if (inChar < 0x01F1 || inChar > 0x0400 && inChar < 0x045F || inChar > 0x2012 && inChar < 0x203D || /*? - not sure that optimal*/ inChar > 0x2019 && inChar < 0x2303 || inChar > 0x24FF && inChar < 0x266C)
	//      return false;
	//  else
	//      return true;
	//}
}

bool isCharPseudographics(wchar_t inChar)
{
	bool isPseudo = ((inChar >= 0x2013) && (inChar <= 0x25C4));
	return isPseudo;
}

// Some pseudographics characters may be shrinked freely
bool isCharPseudoFree(wchar_t inChar)
{
	bool isFree = (inChar == ucBoxSinglHorz) || (inChar == ucBoxDblHorz);
	return isFree;
}

// These are "frame" characters, which has either
// * any vertical (even partial) line
// * or Up/Down arrows (scrollers)
bool isCharBorderVertical(wchar_t inChar)
{
	bool isVert = ((inChar >= ucBoxSinglVert)
		&& ((inChar <= ucBoxSinglUpHorz)
			|| ((inChar >= ucBoxDblVert) && (inChar <= ucBoxDblVertHorz))
			|| ((inChar == ucArrowUp) || (inChar == ucArrowDown))
			));
	return isVert;
}

bool isCharProgress(wchar_t inChar)
{
	bool isProgress = (inChar == ucBox25 || inChar == ucBox50 || inChar == ucBox75 || inChar == ucBox100);
	return isProgress;
}

bool isCharScroll(wchar_t inChar)
{
	bool isScrollbar = (inChar == ucBox25 || inChar == ucBox50 || inChar == ucBox75 || inChar == ucBox100
	                    || inChar == ucUpScroll || inChar == ucDnScroll);
	return isScrollbar;
}

bool isCharSeparate(wchar_t inChar)
{
	// Здесь возвращаем те символы, которые нельзя рисовать вместе с обычными буквами.
	// Например, 0xFEFF на некоторых шрифтах вообще переключает GDI на какой-то левый шрифт O_O
	switch (inChar)
	{
		// TODO: Leave only ‘breaking’ glyphs here!
		case 0x135F:
		case 0x2060:
		case 0x3000:
		case 0x3099:
		case 0x309A:
		case 0xA66F:
		case 0xA670:
		case 0xA671:
		case 0xA672:
		case 0xA67C:
		case 0xA67D:
		case 0xFEFF:
			return true;
		// We must draw ‘combining’ characters with previous ‘letter’
		// Otherwise it will not combine or even may stay invisibe
			/*
		default:
			if (inChar>=0x0300)
			{
				if (inChar<=0x2DFF)
				{
					if ((inChar<=0x036F) // Combining/Accent/Acute/NonSpacing
						|| (inChar>=0x2000 && inChar<=0x200F)
						|| (inChar>=0x202A && inChar<=0x202E)
						|| (inChar>=0x0483 && inChar<=0x0489)
						|| (inChar>=0x07EB && inChar<=0x07F3)
						|| (inChar>=0x1B6B && inChar<=0x1B73)
						|| (inChar>=0x1DC0 && inChar<=0x1DFF)
						|| (inChar>=0x20D0 && inChar<=0x20F0)
						|| (inChar>=0x2DE0))
					{
						return true;
					}
				}
				else if (inChar>=0xFE20 && inChar<=0xFE26)
				{
					return true;
				}
			}
			*/
	}
	return false;

	/*
	wchar_t CharList[] = {0x135F, 0xFEFF, 0};
	__asm {
		MOV  ECX, ARRAYSIZE(CharList)
		REPZ SCAS CharList
	}
	*/
}

// All symbols, which may be displayed as "space"
// And we don't care (here) about Zero-Width spaces!
bool isCharSpace(wchar_t inChar)
{
	bool isSpace = (inChar == ucSpace || inChar == ucNoBreakSpace
		|| (((inChar >= 0x2000) && (inChar <= 0x3000))
			&& ((inChar <= 0x200A)      // 0x2000..0x200A - Different typographical non-zero spaces
				|| (inChar == 0x205F)   // Medium Math Space
				|| (inChar == 0x3000)   // CJK Wide Space
				)
			)
		//|| (inChar == 0x00B7) // MIDDLE DOT - Far Manager shows spaces that way in some cases
		//|| inChar == 0 // -- Zero is not intended to be here - only valid substitutes!
		);
	return isSpace;
}

// Same as isCharSpace, but without ‘CJK Wide Space’
bool isCharSpaceSingle(wchar_t inChar)
{
	bool isSpace = (inChar == ucSpace || inChar == ucNoBreakSpace
		|| (((inChar >= 0x2000) && (inChar <= 0x3000))
			&& ((inChar <= 0x200A)      // 0x2000..0x200A - Different typographical non-zero spaces
				|| (inChar == 0x205F)   // Medium Math Space
		//		|| (inChar == 0x3000)  // CJK Wide Space
				)
			)
		//|| (inChar == 0x00B7) // MIDDLE DOT - Far Manager shows spaces that way in some cases
		//|| inChar == 0 // -- Zero is not intended to be here - only valid substitutes!
		);
	return isSpace;
}

bool isCharRTL(wchar_t inChar)
{
	bool isRtl = (inChar >= 0x05BE)
		&& (((inChar <= 0x08AC)
			&& ((inChar == 0x05BE)
				|| (inChar == 0x05C0) || (inChar == 0x05C3) || (inChar == 0x05C6)
				|| ((inChar >= 0x05D0) && (inChar <= 0x05F4))
				|| (inChar == 0x0608) || (inChar == 0x060B) || (inChar == 0x060D)
				|| ((inChar >= 0x061B) && (inChar <= 0x064A))
				|| ((inChar >= 0x066D) && (inChar <= 0x066F))
				|| ((inChar >= 0x0671) && (inChar <= 0x06D5))
				|| ((inChar >= 0x06E5) && (inChar <= 0x06E6))
				|| ((inChar >= 0x06EE) && (inChar <= 0x06EF))
				|| ((inChar >= 0x06FA) && (inChar <= 0x0710))
				|| ((inChar >= 0x0712) && (inChar <= 0x072F))
				|| ((inChar >= 0x074D) && (inChar <= 0x07A5))
				|| ((inChar >= 0x07B1) && (inChar <= 0x07EA))
				|| ((inChar >= 0x07F4) && (inChar <= 0x07F5))
				|| ((inChar >= 0x07FA) && (inChar <= 0x0815))
				|| (inChar == 0x081A) || (inChar == 0x0824) || (inChar == 0x0828)
				|| ((inChar >= 0x0830) && (inChar <= 0x0858))
				|| ((inChar >= 0x085E) && (inChar <= 0x08AC))
			)) // end of ((inChar <= 0x08AC) && (inChar <= 0x08AC))
		|| (inChar == 0x200E)
		|| (inChar == 0x200F)
		|| ((inChar >= 0xFB1D) && (inChar <= 0xFEFC)
			&& ((inChar == 0xFB1D)
				|| ((inChar >= 0xFB1F) && (inChar <= 0xFB28))
				|| ((inChar >= 0xFB2A) && (inChar <= 0xFD3D))
				|| ((inChar >= 0xFD50) && (inChar <= 0xFDFC))
				|| ((inChar >= 0xFE70) && (inChar <= 0xFEFC))
			)) // end of ((inChar >= 0xFB1D) && (inChar <= 0xFEFC)
		//((inChar>=0x10800)&&(inChar<=0x1091B))||
		//((inChar>=0x10920)&&(inChar<=0x10A00))||
		//((inChar>=0x10A10)&&(inChar<=0x10A33))||
		//((inChar>=0x10A40)&&(inChar<=0x10B35))||
		//((inChar>=0x10B40)&&(inChar<=0x10C48))||
		//((inChar>=0x1EE00)&&(inChar<=0x1EEBB))
		);
	return isRtl;
}

bool isCharCJK(wchar_t inChar)
{
	return is_char_cjk(inChar);
}

bool isCharComining(wchar_t inChar)
{
	return is_char_combining(inChar);
}



//TODO: 1 pixel for all shrinkable characters? prefer some function of Length...
#define MIN_SIZEFREE_WIDTH 1
#define DEF_SINGLE_MUL 8
#define DEF_SINGLE_DIV 10
#define DEF_DOUBLE_MUL 7
#define DEF_DOUBLE_DIV 10

void VConTextPart::Init(uint anIndex, uint anCell, CVConLine* pLine)
{
	Flags = TRF_None;
	Length = 1; // initially, we'll enlarge it later
	TotalWidth = MinWidth = 0;
	Index = anIndex;
	Cell = anCell; // May differs from anIndex on DBCS systems
	PositionX = CellPosX = Cell * pLine->FontWidth;
	Chars = pLine->ConCharLine + anIndex;
	Attrs = pLine->ConAttrLine + anIndex;
	CharFlags = pLine->TempCharFlags + anIndex;
	CharWidth = pLine->TempCharWidth + anIndex;
}

void VConTextPart::Done(uint anLen, uint FontWidth)
{
	_ASSERTE(anLen>0 && FontWidth>0);
	Length = anLen;
	TotalWidth = MinWidth = 0;

	// May this part be shrinked freely?
	if (Flags & TRF_TextSpacing)
	{
		Flags |= TRF_SizeFree;
	}
	else if ((Flags & TRF_TextPseudograph) && isCharPseudoFree(*Chars))
	{
		wchar_t wc = *Chars; bool bDiffers = false;
		for (uint i = 1; i < Length; i++)
		{
			if (Chars[i] != wc)
			{
				bDiffers = true; break;
			}
		}
		if (!bDiffers)
			Flags |= TRF_SizeFree;
	}

	// Helper - double cell characters
	uint FontWidth2 = 2*FontWidth;
	uint FontWidth2M = FontWidth2 * DEF_DOUBLE_MUL / DEF_DOUBLE_DIV;
	uint FontWidthM = FontWidth * DEF_SINGLE_MUL / DEF_SINGLE_DIV;

	const wchar_t* pch = Chars;
	const CharAttr* pca = Attrs;
	TextCharType* pcf = CharFlags;
	uint* pcw = CharWidth;

	ZeroStruct(AllWidths);

	//TODO: Continuous spaces must be processed separately

	if (Flags & TRF_SizeFree)
	{
		VConTextPartWidth& aw = AllWidths[TCF_WidthFree];
		for (uint left = anLen; left--; pch++, pca++, pcf++, pcw++)
		{
			_ASSERTE(!(pca->Flags2 & (CharAttr2_NonSpacing|CharAttr2_Combining|CharAttr2_DoubleSpaced)));
			*pcw = FontWidth; // if non-shrinked
			*pcf = TCF_WidthFree;
			TotalWidth += FontWidth;
			aw.Count++;
			aw.Width += FontWidth;
		}
		aw.MinWidth += FontWidth;
	}
	else
	{
		for (uint left = anLen; left--; pch++, pca++, pcf++, pcw++)
		{
			if (pca->Flags2 & (CharAttr2_NonSpacing|CharAttr2_Combining))
			{
				// zero-width
				*pcw = 0;
				*pcf = TCF_WidthZero;
				VConTextPartWidth& aw = AllWidths[TCF_WidthZero];
				aw.Count++; // Informational
			}
			else if (pca->Flags2 & CharAttr2_DoubleSpaced)
			{
				*pcw = FontWidth2;
				*pcf = TCF_WidthDouble;
				TotalWidth += FontWidth2;
				// Even double-width space have to be same width as ‘normal’ CJK glyph
				MinWidth += FontWidth2M;
				VConTextPartWidth& aw = AllWidths[TCF_WidthDouble];
				aw.Count++;
				aw.Width += FontWidth2;
				aw.MinWidth += FontWidth2M;
			}
			else /*if (gpSet->isMonospace
					|| (gpSet->isFixFarBorders && isCharAltFont(ch))
					|| (gpSet->isEnhanceGraphics && isCharProgress(ch))
					)*/
			{
				//TODO: Caller must process fonts and set "real" character widths for proportinal fonts
				*pcw = FontWidth;
				*pcf = TCF_WidthNormal;
				VConTextPartWidth& aw = AllWidths[TCF_WidthNormal];
				aw.Count++;
				aw.Width += FontWidth;
				aw.MinWidth += FontWidthM;
			}
		}
	}

	TotalWidth = AllWidths[TCF_WidthFree].Width
		+  AllWidths[TCF_WidthNormal].Width
		+  AllWidths[TCF_WidthDouble].Width
		;
	MinWidth = AllWidths[TCF_WidthFree].MinWidth
		+  AllWidths[TCF_WidthNormal].MinWidth
		+  AllWidths[TCF_WidthDouble].MinWidth
		;
}


CVConLine::CVConLine(CRealConsole* apRCon)
	: mp_RCon(apRCon)
	, mn_DialogsCount(0)
	, mrc_Dialogs(NULL)
	, mn_DialogFlags(NULL)
	, mn_DialogAllFlags(0)
	, mrc_UCharMap()
	, isFilePanel(false)
	, TextWidth(0)
	, FontWidth(0)
	, isForce(true)
	, ConCharLine(NULL)
	, ConAttrLine(NULL)
	, MaxBufferSize(0)
	, PartsCount(0)
	, TextParts(NULL)
	, TempCharFlags(NULL)
	, TempCharWidth(NULL)
	, TotalLineWidth(0)
	, MinLineWidth(0)
	, isFixFrameCoord(true)
	, NextDialog(true)
	, NextDialogX(0)
	, CurDialogX1(0)
	, CurDialogX2(0)
	, CurDialogI(0)
	, CurDialogFlags(0)
{
}

CVConLine::~CVConLine()
{
	ReleaseMemory();
}

void CVConLine::SetDialogs(int anDialogsCount, SMALL_RECT* apDialogs, DWORD* apnDialogFlags, DWORD anDialogAllFlags, const SMALL_RECT& arcUCharMap)
{
	mn_DialogsCount = anDialogsCount;
	mrc_Dialogs = apDialogs;
	mn_DialogFlags = apnDialogFlags;
	mn_DialogAllFlags = anDialogAllFlags;
	mrc_UCharMap = arcUCharMap;
}

bool CVConLine::ParseLine(bool abForce, uint anTextWidth, uint anFontWidth, uint anRow, wchar_t* apConCharLine, CharAttr* apConAttrLine, const wchar_t* const ConCharLine2, const CharAttr* const ConAttrLine2)
{
	const bool bNeedAlloc = (MaxBufferSize < anTextWidth);
	TextWidth = anTextWidth;
	FontWidth = anFontWidth;
	row = anRow;
	ConCharLine = apConCharLine;
	ConAttrLine = apConAttrLine;
	PartsCount = 0;

	NextDialog = true;
	NextDialogX = 0;
	CurDialogX1 = CurDialogX2 = CurDialogI = 0;
	CurDialogFlags = 0;

	isFilePanel = (mn_DialogAllFlags & (FR_LEFTPANEL|FR_RIGHTPANEL|FR_FULLPANEL)) != 0;

	//TODO: Extend to other old-scool managers?
	isFixFrameCoord = mp_RCon->isFar();

	if (bNeedAlloc && !AllocateMemory())
		return false;

	const bool bEnhanceGraphics = gpSet->isEnhanceGraphics;
	const bool bProportional = gpSet->isMonospace == 0;
	const bool bForceMonospace = gpSet->isMonospace == 2;
	const bool bUseAlternativeFont = gpSet->isFixFarBorders;

	for (uint j = 0; j < TextWidth;)
	{
		//TODO: HI/LO SURROGATES
		wchar_t wc = ConCharLine[j];
		const CharAttr attr = ConAttrLine[j];

		VConTextPart* p = TextParts+(PartsCount++);

		uint j2 = j+1;

		//TODO: DBCS cell number, it may differs from j
		p->Init(j, j, this);

		// Process Far Dialogs to justify rectangles and frames
		TextPartFlags dlgBorder = isDialogBorderCoord(j);

		/* *** Now we split our text line into parts with characters one "family" *** */

		_ASSERTE(p->Flags == TRF_None);

		if (isCharSpaceSingle(wc))
		{
			// CJK Double-width space will be processed under isCharCJK
			p->Flags = TRF_TextSpacing;
			while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharSpaceSingle(ConCharLine[j2]))
				j2++;
		}
		else if (isCharSeparate(wc))
		{
			p->Flags = TRF_TextSeparate;
			while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharSeparate(ConCharLine[j2]))
				j2++;
		}
		else if (bEnhanceGraphics && isCharProgress(wc))
		{
			p->Flags = TRF_TextProgress
				| ((bUseAlternativeFont && isCharAltFont(wc)) ? TRF_TextAlternative : TRF_None)
				;
			while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharProgress(ConCharLine[j2]))
				j2++;
		}
		else if (bEnhanceGraphics && isCharScroll(wc))
		{
			p->Flags = TRF_TextScroll
				| ((bUseAlternativeFont && isCharAltFont(wc)) ? TRF_TextAlternative : TRF_None)
				;
			while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharScroll(ConCharLine[j2]))
				j2++;
		}
		else if (dlgBorder)
		{
			p->Flags = dlgBorder
				| (isCharBorderVertical(wc) ? TRF_TextPseudograph : TRF_None)
				| ((bUseAlternativeFont && isCharAltFont(wc)) ? TRF_TextAlternative : TRF_None)
				;
		}
		else if (isCharPseudographics(wc))
		{
			// Processed separately from isCharAltFont, because last may covert larger range
			p->Flags = TRF_TextPseudograph;
			if (bUseAlternativeFont && isCharAltFont(wc))
			{
				p->Flags |= TRF_TextAlternative;
				wchar_t wc2;
				while ((j2 < TextWidth) && (ConAttrLine[j2] == attr)
					&& isCharPseudographics((wc2=ConCharLine[j2]))
					&& !isCharBorderVertical(wc2)
					&& isCharAltFont(wc2))
					j2++;
			}
			else
			{
				while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharPseudographics(ConCharLine[j2]))
					j2++;
			}
		}
		else if (isCharCJK(wc)) // Double-width characters (CJK, etc.)
		{
			// Processed separately from isCharAltFont, because last may covert larger range
			p->Flags = TRF_TextCJK;
			if (bUseAlternativeFont && isCharAltFont(wc))
			{
				p->Flags |= TRF_TextAlternative;
				wchar_t wc2;
				while ((j2 < TextWidth) && (ConAttrLine[j2] == attr)
					&& isCharCJK((wc2=ConCharLine[j2]))
					&& isCharAltFont(wc2))
					j2++;
			}
			else
			{
				while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharCJK(ConCharLine[j2]))
					j2++;
			}
		}
		else if (bUseAlternativeFont && isCharAltFont(wc))
		{
			p->Flags = TRF_TextAlternative;
			while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharScroll(ConCharLine[j2]))
				j2++;
		}
		else
		{
			p->Flags = TRF_TextNormal;
			wchar_t wc2;
			// That's more complicated as previous branches,
			// we must break on all chars mentioned above
			while ((j2 < TextWidth) && (ConAttrLine[j2] == attr)
				&& !isCharSpaceSingle((wc2=ConCharLine[j2]))
				&& !isCharSeparate(wc2)
				&& !isCharProgress(wc2)
				&& !isCharScroll(wc2)
				&& !isCharPseudographics(wc2)
				&& (wc2 != L'}') // Far Manager Panels mark "file name is trimmed"
				&& !(bUseAlternativeFont && isCharAltFont(wc2))
				//TODO: RTL/LTR
				)
				j2++;
		}

		/* *** We shall prepare this token (default/initial widths must be set) *** */
		p->Done(j2-j, FontWidth);
		TotalLineWidth += p->TotalWidth;
		MinLineWidth += p->MinWidth;

		/* Next part */
		j = j2;
	}

	return true;
}

// For proportional fonts, caller MUST already change widths of all chars
// (this->TempCharWidth) from ‘default’ to exact values returned by GDI
void CVConLine::PolishParts(DWORD* pnXCoords)
{
	if (!TextParts || !PartsCount)
	{
		_ASSERTE(TextParts && PartsCount);
		return;
	}

	#ifdef _DEBUG
	// Validate structure
	for (uint k = 0; k < PartsCount; k++)
	{
		_ASSERTE(TextParts[k].Flags != TRF_None);
		_ASSERTE(TextParts[k].Length > 0);
		_ASSERTE(TextParts[k].Chars && TextParts[k].Attrs && TextParts[k].CharFlags);
	}
	#endif

	uint PosX, CharIdx;
	int prevFixedPart; // !SIGNED!

	// First, we need to check if part do not overlap
	// with *next* part, which may have *fixed* PosX
	PosX = 0; prevFixedPart = -1;
	for (uint k = 0; k < PartsCount; k++)
	{
		VConTextPart& part = TextParts[k];

		if (part.Flags & (TRF_PosFixed|TRF_PosRecommended))
		{
			// Overlaps?
			if (k && (part.PositionX != PosX))
			{
				_ASSERTE(k>0); // We can't get here for k==0
				if (k)
				{
					DistributeParts(prevFixedPart + 1, k - 1, part.PositionX);
				}
			}
			// Store updated coord
			prevFixedPart = k;
			PosX = part.PositionX + part.TotalWidth;
		}
		else
		{
			//if (part.Flags & TRF_SizeFree)
			//	TextParts[k-1].MinWidth = 1;
			part.PositionX = PosX;
			PosX += part.TotalWidth;
		}
	}
	// If last part goes out of screen - redistribute
	if (PosX != (TextWidth * FontWidth))
	{
		DistributeParts(prevFixedPart + 1, PartsCount - 1, (TextWidth * FontWidth));
	}


	/* Fill all X coordinates */
	PosX = CharIdx = 0;
	TotalLineWidth = MinLineWidth = 0;
	for (uint k = 0; k < PartsCount; k++)
	{
		VConTextPart& part = TextParts[k];
		_ASSERTE(part.Flags != TRF_None);
		_ASSERTE(((int)part.TotalWidth > 0) || ((part.Length==1) && (part.CharFlags[0]==TCF_WidthZero)));

		if (part.Flags & (TRF_PosFixed|TRF_PosRecommended))
		{
			_ASSERTE(PosX <= part.PositionX);
			if (k && PosX && PosX < part.PositionX)
				ExpandPart(TextParts[k-1], part.PositionX);
			PosX = part.PositionX;
		}
		else
		{
			part.PositionX = PosX;
		}
		TotalLineWidth += part.TotalWidth;
		MinLineWidth += part.MinWidth;

		uint charPosX = PosX;
		uint* ptrWidth = part.CharWidth;
		for (uint l = part.Length; l--; CharIdx++, ptrWidth++)
		{
			charPosX += *ptrWidth;
			pnXCoords[CharIdx] = charPosX;
		}

		PosX += part.TotalWidth;
		_ASSERTE(PosX >= charPosX);
	}

	//TODO: Optimization. Use linked-list for TextParts
	//TODO: we'll get memory gain and easy part deletion

	// Now we may combine all parts, which are displayed same style
	for (uint k = 0; k < PartsCount; k++)
	{
		VConTextPart& part = TextParts[k];
		if (part.Flags & (TRF_PosFixed|TRF_PosRecommended))
			continue;

		const CharAttr attr = ConAttrLine[part.Index];

		if (part.Flags & TRF_TextAlternative)
		{
			for (uint k2 = k+1; k2 < PartsCount; k2++)
			{
				VConTextPart& part2 = TextParts[k2];
				if ((part2.Flags & (TRF_PosFixed|TRF_PosRecommended))
					|| !(part2.Flags & TRF_TextAlternative))
					break;
				if (!(attr == ConAttrLine[part2.Index]))
					break;
				part.Length += part2.Length;
				part.TotalWidth += part2.TotalWidth;
				part.MinWidth += part2.MinWidth;
				// "Hide" this part from list
				part2.Flags = TRF_None;
				k = k2;
			}
		}
		else if (!(part.Flags & TRF_TextSpacing))
		{
			for (uint k2 = k+1; k2 < PartsCount; k2++)
			{
				VConTextPart& part2 = TextParts[k2];
				if ((part2.Flags & (TRF_PosFixed|TRF_PosRecommended|TRF_TextAlternative)))
					break;
				if (!(attr == ConAttrLine[part2.Index]))
					break;
				part.Length += part2.Length;
				part.TotalWidth += part2.TotalWidth;
				part.MinWidth += part2.MinWidth;
				// "Hide" this part from list
				part2.Flags = TRF_None;
				k = k2;
			}
		}
	}

	return;
}

// Shrink lengths of [part1..part2] (including)
// They must not exceed `right` X coordinate
void CVConLine::DistributeParts(uint part1, uint part2, uint right)
{
	if ((part1 > part2) || (part2 >= PartsCount))
	{
		_ASSERTE((part1 <= part2) && (part2 < PartsCount));
		return;
	}

	// If possible - shrink only last part
	// TRF_SizeFree - Spaces or horizontal frames. It does not matter, how many *real* characters we may write
	if ((TextParts[part2].Flags & TRF_SizeFree)
		&& ((TextParts[part2].PositionX + MIN_SIZEFREE_WIDTH) < right)
		)
		part1 = part2;

	if (right <= TextParts[part1].PositionX)
	{
		_ASSERTE(right > TextParts[part1].PositionX);
		return;
	}

	const uint suggMul = 4, suggDiv = 5;
	uint FontWidth2 = (FontWidth * 2);
	uint FontWidth2m = (FontWidth2 * suggMul / suggDiv);
	uint ReqWidth = (right - TextParts[part1].PositionX);

	// unused?
	uint FullWidth = (TextParts[part2].PositionX + TextParts[part2].TotalWidth - TextParts[part1].PositionX);

	// 1) shrink only TCF_WidthFree chars
	// 2) also shrink TCF_WidthDouble (if exist) a little
	// 3) shrink all possibilities

	WARNING("Change the algorithm");
	// a) count all TCF_WidthFree, TCF_WidthNormal, TCF_WidthDouble separatedly
	// b) so we able to find most suitable way to shrink either part with its own factor (especially useful when both CJK and single-cell chars exists)

	VConTextPartWidth AllWidths[TCF_WidthLast] = {};

	// Count all widths for parts and check if we may do shrink with TCF_WidthFree chars only
	bool bHasFreeOverlaps = HasFreeOverlaps(part1, part2, right, AllWidths);

	uint nAllWidths = (AllWidths[TCF_WidthFree].Width + AllWidths[TCF_WidthNormal].Width + AllWidths[TCF_WidthDouble].Width);
	_ASSERTE(nAllWidths == FullWidth); // At the moment, they must match

	// What we may to shrink?
	if (nAllWidths <= ReqWidth)
	{
		if ((nAllWidths != ReqWidth) && !bHasFreeOverlaps)
		{
			DistributePartsFree(part1, part2, right);
		}
		else
		{
			_ASSERTE(FALSE && "Already fit, nothing to shrink");
		}
		return;
	}

	// Only spaces and horizontal frames
	if (!bHasFreeOverlaps
		&& ((AllWidths[TCF_WidthFree].MinWidth + AllWidths[TCF_WidthNormal].Width + AllWidths[TCF_WidthDouble].Width) <= ReqWidth)
		)
	{
		DistributePartsFree(part1, part2, right);
		return;
	}

	uint nAllMin = AllWidths[TCF_WidthFree].MinWidth + AllWidths[TCF_WidthNormal].MinWidth + AllWidths[TCF_WidthDouble].MinWidth;
	if (!nAllMin)
	{
		_ASSERTE(nAllMin!=NULL); // Must not be zero!
		return;
	}

	// method and options
	uint nShrinkParts = (1 << TCF_WidthFree);

	if ((AllWidths[TCF_WidthFree].MinWidth + AllWidths[TCF_WidthNormal].Width + AllWidths[TCF_WidthDouble].Width) <= ReqWidth)
	{
		AllWidths[TCF_WidthFree].ReqWidth = ReqWidth - (AllWidths[TCF_WidthNormal].Width + AllWidths[TCF_WidthDouble].Width);
	}
	else if ((AllWidths[TCF_WidthFree].MinWidth + AllWidths[TCF_WidthNormal].Width + AllWidths[TCF_WidthDouble].MinWidth) <= ReqWidth)
	{
		// Spaces and double-space glyphs
		// Actually, with monospaced fonts we would have fit problems,
		// when double-space glyphs takes only one cell in console.
		// That is non-DBCS systems or UTF-8(?) on DBCS system
		nShrinkParts |= (1 << TCF_WidthDouble);
		AllWidths[TCF_WidthFree].ReqWidth = AllWidths[TCF_WidthFree].MinWidth;
		AllWidths[TCF_WidthDouble].ReqWidth = ReqWidth - (AllWidths[TCF_WidthNormal].Width + AllWidths[TCF_WidthFree].ReqWidth);
	}
	else
	{
		// Shrink all types
		nShrinkParts |= (1 << TCF_WidthNormal) | (1 << TCF_WidthDouble);

		// And we may apply a larger coefficient to dominating type of chars

		// Count each double-space glyph as 2 cells (preferred)
		AllWidths[TCF_WidthDouble].Count *= 2;
		// Count "all" cells
		int nAllCells = AllWidths[TCF_WidthDouble].Count + AllWidths[TCF_WidthNormal].Count + AllWidths[TCF_WidthFree].Count;

		// Sort types by cells used
		uint nMost = TCF_WidthFree;
		if ((AllWidths[TCF_WidthDouble].Count >= AllWidths[TCF_WidthNormal].Count) && (AllWidths[TCF_WidthDouble].Count >= AllWidths[TCF_WidthFree].Count))
			nMost = TCF_WidthDouble;
		else if ((AllWidths[TCF_WidthNormal].Count >= AllWidths[TCF_WidthDouble].Count) && (AllWidths[TCF_WidthNormal].Count >= AllWidths[TCF_WidthFree].Count))
			nMost = TCF_WidthNormal;
		uint nOther1 = (nMost != TCF_WidthDouble) ? TCF_WidthDouble : TCF_WidthNormal;
		uint nFlags = (1 << nMost) | (1 << nOther1);
		uint nOther2 = (!(nFlags & (1 << TCF_WidthDouble))) ? TCF_WidthDouble
			: (!(nFlags & (1 << TCF_WidthNormal))) ? TCF_WidthNormal
			: TCF_WidthFree;
		if (AllWidths[nOther1].Count < AllWidths[nOther2].Count)
			klSwap(nOther1, nOther2);

		uint nMostMul = (nMost == TCF_WidthDouble) ? 9 : 10;
		uint nMostDiv = 10;

		// Apply denominators
		AllWidths[nMost].ReqWidth = ReqWidth * AllWidths[nMost].Count * nMostMul / (nAllCells * nMostDiv);
		_ASSERTE(AllWidths[nMost].ReqWidth <= ReqWidth);

		// And prepare lesser types
		if ((ReqWidth - AllWidths[nMost].ReqWidth) >= (AllWidths[nOther1].Width + AllWidths[nOther2].MinWidth))
		{
			AllWidths[nOther1].ReqWidth = AllWidths[nOther1].Width;
			AllWidths[nOther2].ReqWidth = ReqWidth - (AllWidths[nMost].ReqWidth + AllWidths[nOther1].ReqWidth);
		}
		else if (AllWidths[nOther1].Count > 0 && AllWidths[nOther2].Count > 0)
		{
			_ASSERTE(AllWidths[nMost].Count > 0);
			AllWidths[nOther1].ReqWidth = ReqWidth * AllWidths[nOther1].Count / nAllCells;
			AllWidths[nOther2].ReqWidth = ReqWidth - (AllWidths[nMost].ReqWidth + AllWidths[nOther1].ReqWidth);
		}
		else if (AllWidths[nOther1].Count > 0)
		{
			_ASSERTE(AllWidths[nMost].Count > 0 && AllWidths[nOther2].Count == 0);
			AllWidths[nOther1].ReqWidth = ReqWidth - (AllWidths[nMost].ReqWidth);
			_ASSERTE(AllWidths[nOther2].Width == 0);
			AllWidths[nOther2].ReqWidth = 0;
		}
		else
		{
			_ASSERTE(AllWidths[nMost].Count > 0 && AllWidths[nOther1].Count == 0 && AllWidths[nOther2].Count == 0);
			_ASSERTE(AllWidths[nOther1].Width == 0 && AllWidths[nOther2].Width == 0);
			AllWidths[nOther1].ReqWidth = AllWidths[nOther2].ReqWidth = 0;
		}
		// Return *char* count
		AllWidths[TCF_WidthDouble].Count /= 2;
	}

	// Debug validations
	_ASSERTE((int)AllWidths[TCF_WidthFree].ReqWidth >= 0);
	_ASSERTE((int)AllWidths[TCF_WidthNormal].ReqWidth >= 0);
	_ASSERTE((int)AllWidths[TCF_WidthDouble].ReqWidth >= 0);

	// *Process* the shrink

	// Leftmost char coord
	uint PosX = TextParts[part1].PositionX;

	for (uint k = part1; k <= part2; k++)
	{
		VConTextPart& part = TextParts[k];
		if (!part.Flags)
		{
			_ASSERTE(part.Flags); // Part must not be dropped yet!
			continue;
		}

		// Update new leftmost coord for this part
		part.PositionX = PosX;

		// Prepare loops
		TextCharType* pcf = part.CharFlags; // character flags (zero/free/normal/double)
		uint* pcw = part.CharWidth; // pointer to character width

		// Run part shrink
		part.TotalWidth = 0;
		if ((part.Flags & TRF_SizeFree) && (nShrinkParts & (1 << TCF_WidthFree)))
		{
			VConTextPartWidth& aw = AllWidths[TCF_WidthFree];
			int iShrinkLeft = part.Length;
			_ASSERTE(AllWidths[TCF_WidthFree].Count>0);
			uint partReqWidth = (AllWidths[TCF_WidthFree].Count > 0) ? (aw.ReqWidth / AllWidths[TCF_WidthFree].Count) : 0;
			for (uint c = 0; c < part.Length; c++, pcf++, pcw++)
			{
				_ASSERTE(*pcf == TCF_WidthFree);
				DoShrink(*pcw, iShrinkLeft, partReqWidth, part.TotalWidth);
			}
		}
		else
		{
			for (uint c = 0; c < part.Length; c++, pcf++, pcw++)
			{
				if (nShrinkParts & (1 << *pcf))
				{
					VConTextPartWidth& aw = AllWidths[*pcf];
					DoShrink(*pcw, aw.Count, aw.ReqWidth, part.TotalWidth);
				}
				else
				{
					part.TotalWidth += *pcw;
				}
			}
		}

		PosX += part.TotalWidth;
	}

	// End of shrink (no more parts, no more methods)
	_ASSERTE(PosX <= right);
}

//TODO: Refactor
void CVConLine::DoShrink(uint& charWidth, int& ShrinkLeft, uint& NeedWidth, uint& TotalWidth)
{
	if (!NeedWidth)
	{
		// Only TRF_SizeFree is expected here...
		charWidth = 0;
		if (ShrinkLeft) ShrinkLeft--;
	}
	else if (ShrinkLeft > 0)
	{
		_ASSERTE(ShrinkLeft>0 && NeedWidth>0);
		uint nw = NeedWidth / ShrinkLeft;
		if (nw > charWidth)
			nw = charWidth;
		if (nw > NeedWidth)
		{
			_ASSERTE(nw <= NeedWidth);
			NeedWidth = 0;
		}
		else
		{
			NeedWidth -= nw;
		}
		ShrinkLeft--;
		charWidth = nw;
		TotalWidth += nw;
	}
	else
	{
		_ASSERTE(ShrinkLeft>0);
		TotalWidth += charWidth;
	}
}

void CVConLine::ExpandPart(VConTextPart& part, uint EndX)
{
	if (!part.Length || !part.Flags)
	{
		_ASSERTE(part.Length && part.Flags);
		return;
	}

	// Spaces and Horizontal frames
	if ((part.Flags & TRF_SizeFree) && (part.Length > 1))
	{
		uint NeedWidth = EndX - part.PositionX;
		int WidthLeft = NeedWidth;
		uint* pcw = part.CharWidth;
		part.TotalWidth = 0;
		for (int i = part.Length; i > 0; i--, pcw++)
		{
			uint nw = (uint)(WidthLeft - i);
			*pcw = nw;
			part.TotalWidth += nw;
			WidthLeft -= nw;
		}
		// rounding problem?
		if (WidthLeft > 0)
		{
			part.CharWidth[part.Length-1] += WidthLeft;
			part.TotalWidth += WidthLeft;
		}
	}
	else
	{
		// Don't break visual representation of string flow, just increase last symbol width
		size_t chrIdx = (part.Length - 1);
		//TODO: RTL?
		//if (isCharRTL(part.Chars[chrIdx]))
		//	chrIdx = 0;
		uint* pcw = part.CharWidth + chrIdx;
		int iDiff = EndX - part.PositionX - part.TotalWidth;
		_ASSERTE(iDiff > 0);
		*pcw += iDiff;
		part.TotalWidth += iDiff;
	}
}

bool CVConLine::GetNextPart(uint& partIndex, VConTextPart*& part, VConTextPart*& nextPart)
{
	if (!TextParts || !PartsCount)
	{
		_ASSERTE(TextParts && PartsCount);
		return false;
	}

	// Skip all ‘combined’ parts
	while ((partIndex < PartsCount) && (TextParts[partIndex].Flags == TRF_None))
		partIndex++;

	// No more parts?
	if (partIndex >= PartsCount)
	{
		nextPart = NULL;
		return false;
	}

	part = TextParts+partIndex;
	partIndex++;

	// Skip all next ‘combined’ parts
	while ((partIndex < PartsCount) && (TextParts[partIndex].Flags == TRF_None))
		partIndex++;
	nextPart = (partIndex < PartsCount) ? (TextParts+partIndex) : NULL;
	return true;
}

bool CVConLine::AllocateMemory()
{
	ReleaseMemory();

	if (!(TextParts = (VConTextPart*)malloc(TextWidth*sizeof(*TextParts))))
		return false;
	if (!(TempCharFlags = (TextCharType*)malloc(TextWidth*sizeof(*TempCharFlags))))
		return false;
	if (!(TempCharWidth = (uint*)malloc(TextWidth*sizeof(*TempCharWidth))))
		return false;

	return true;
}

void CVConLine::ReleaseMemory()
{
	MaxBufferSize = 0;
	SafeFree(TextParts);
	SafeFree(TempCharFlags);
	SafeFree(TempCharWidth);
}

TextPartFlags CVConLine::isDialogBorderCoord(uint j)
{
	//bool isDlgBorder = false, bMayBeFrame = false;
	TextPartFlags dlgBorder = TRF_None;

	if (NextDialog || (j == NextDialogX))
	{
		NextDialog = false;
		CurDialogX1 = -1;
		NextDialogX = CurDialogX2 = TextWidth;
		CurDialogI = -1; CurDialogFlags = 0;
		int nMax = TextWidth-1;
		// Process dialogs in reverse order, to ensure tops (Z-order) dialogs would be first
		for (int i = mn_DialogsCount-1; i >= 0; i--)
		{
			if (mrc_Dialogs[i].Top <= row && row <= mrc_Dialogs[i].Bottom)
			{
				int border1 = mrc_Dialogs[i].Left;
				int border2 = mrc_Dialogs[i].Right;

				// Looking for a dialog edge, on the current row, nearest to the current X-coord (j)
				if (j <= border1 && border1 < NextDialogX)
					NextDialogX = border1;
				else if (border2 < nMax && j <= border2 && border2 < NextDialogX)
					NextDialogX = border2;

				// Looking for a dialog (not a Far Manager Panels), covering current coord
				// TODO: It would be nice to process Far's column separators too, but there is no nice API
				if (!(mn_DialogFlags[i] & (FR_LEFTPANEL|FR_RIGHTPANEL|FR_FULLPANEL|FR_VIEWEREDITOR)))
				{
					if ((border1 <= j && j <= border2)
						&& (CurDialogX1 <= border1 && border2 <= CurDialogX2))
					{
						CurDialogX1 = border1;
						CurDialogX2 = border2;
						CurDialogFlags = mn_DialogFlags[i];
						_ASSERTE(CurDialogFlags!=0);
						CurDialogI = i;
					}
				}
			}
		}
	}

	if (j == NextDialogX)
	{
		NextDialog = true;
		// Is it visible dialog edge, or it's covered by other dialog?
		if (CurDialogFlags)
		{
			// Coord hits inside dialog space
			// treat right edge as frame too
			if ((j == CurDialogX1 || j == CurDialogX2))
				dlgBorder |= TRF_PosFixed;
		}
		else
		{
			// ???
			dlgBorder |= TRF_PosFixed;
		}
	}


	// Horizontal corrections of vertical frames: Far Manager Panels, etc. (Coord.X>0)
	if (!(dlgBorder & TRF_PosFixed) // if not marked as PosFixed yet...
		&& j && isFixFrameCoord)
	{
		wchar_t c = ConCharLine[j];

			// Frames (vertical parts, pseudographics)
		if (isCharBorderVertical(c)
			// Far Manager Panel scrollers
			|| isCharScroll(c)
			// Far marks with '}' symbols file names, gone out of column width (too long name)
			// TODO: Take into account other rows? Or even Far Panels Mode?
			// TODO: we can't just check upper and lower rows to compare for frame/}, because
			// TODO: they may be covered with another dialog...
			|| (isFilePanel && c==L'}')
			//TODO: vim/emacs/tmux/etc uses simple `|` as pane delimiter
			)
		{
			dlgBorder |= TRF_PosRecommended;

			#if 0
			// Пройти вверх и вниз от текущей строки, проверив,
			// есть ли в этой же X-координате вертикальная рамка (или угол)
			TODO("Хорошо бы для панелей определять границы колонок более корректно, лучше всего - через API?");
			//if (!bBord && !bFrame)
			if (!isDlgBorder)
			{
				int R;
				wchar_t prevC;
				bool bBord = false;

				for (R = (row-1); bMayBeFrame && !bBord && R>=0; R--)
				{
					prevC = mpsz_ConChar[R*TextWidth+j];
					bBord = isCharBorderVertical(prevC);
					bMayBeFrame = (bBord || isCharScroll(prevC) || (isFilePanel && prevC==L'}'));
				}

				for (R = (row+1); bMayBeFrame && !bBord && R < (int)TextHeight; R++)
				{
					prevC = mpsz_ConChar[R*TextWidth+j];
					bBord = isCharBorderVertical(prevC);
					bMayBeFrame = (bBord || isCharScroll(prevC) || (isFilePanel && prevC==L'}'));
				}
			}

			// Это разделитель колонок панели, полоса прокрутки или явно найденная граница диалога
			if (bMayBeFrame)
				ConCharXLine[j-1] = j * nFontWidth;
			#endif
		}

	}

	return dlgBorder;
}

bool CVConLine::HasFreeOverlaps(const uint part1, const uint part2, const uint right, VConTextPartWidth (&AllWidths)[TCF_WidthLast])
{
	bool bHasFreeOverlaps = false;
	ZeroStruct(AllWidths);

	// Count all widths for parts
	uint checkOverlapX = TextParts[part1].PositionX;
	for (uint k = part1; k <= part2; k++)
	{
		VConTextPart& part = TextParts[k];
		if (!part.Flags)
		{
			_ASSERTE(part.Flags); // Part must not be dropped yet!
			continue;
		}
		if (part.Flags & TRF_SizeFree)
		{
			// They are counted specially, series of spaces or horz.frames may be shrinked
			// forcedly up to one cell almost without meaning loss
			_ASSERTE(part.AllWidths[TCF_WidthFree].MinWidth && part.AllWidths[TCF_WidthFree].MinWidth <= FontWidth);
			AllWidths[TCF_WidthFree].Count += 1;
			AllWidths[TCF_WidthFree].Width += part.AllWidths[TCF_WidthFree].Width;
			AllWidths[TCF_WidthFree].MinWidth += part.AllWidths[TCF_WidthFree].MinWidth;
		}
		else for (uint t = TCF_WidthFree; t < TCF_WidthLast; t++)
		{
			AllWidths[t].Count += part.AllWidths[t].Count;
			AllWidths[t].Width += part.AllWidths[t].Width;
			AllWidths[t].MinWidth += part.AllWidths[t].MinWidth;
		}
		// If possible, shrink TCF_WidthFree in such a way that
		// we'll get next part placed on its preferred CellPosX
		if (!bHasFreeOverlaps)
		{
			if (!(part.Flags & TRF_SizeFree))
			{
				checkOverlapX += part.TotalWidth;
			}
			else
			{
				uint nextX = (k == part2) ? right : GetNextPartX(k);
				if ((checkOverlapX + MIN_SIZEFREE_WIDTH) > nextX)
				{
					// we can't shrink *only* free-size parts between fixed pos of others
					bHasFreeOverlaps = true;
				}
				else
				{
					checkOverlapX = nextX;
				}
			}
		}
	}

	return bHasFreeOverlaps;
}

void CVConLine::DistributePartsFree(uint part1, uint part2, uint right)
{
	// Try to enlarge only "free" parts with (Length>=3)?

	uint PosX = TextParts[part1].PositionX;

	for (uint k = part1; k <= part2; k++)
	{
		VConTextPart& part = TextParts[k];
		if (!part.Flags)
		{
			_ASSERTE(part.Flags); // Part must not be dropped yet!
			continue;
		}

		// Update new leftmost coord for this part
		part.PositionX = PosX;

		if (!(part.Flags & TRF_SizeFree))
		{
			PosX += part.TotalWidth;
			continue;
		}

		// Prepare loops
		TextCharType* pcf = part.CharFlags; // character flags (zero/free/normal/double)
		uint* pcw = part.CharWidth; // pointer to character width

		// Run part shrink
		uint X;
		uint nextPartX = (k == part2) ? right : GetNextPartX(k);
		if (part.PositionX >= nextPartX)
		{
			_ASSERTE(nextPartX > part.PositionX);
			return;
		}
		part.TotalWidth = 0;
		uint NeedPartWidth = nextPartX - part.PositionX;
		for (uint c = 0; c < part.Length; c++, pcf++, pcw++)
		{
			switch (*pcf)
			{
			case TCF_WidthFree:
				X = klMulDivU32(c+1, NeedPartWidth, part.Length);
				*pcw = X - part.TotalWidth;
			case TCF_WidthNormal:
			case TCF_WidthDouble:
				part.TotalWidth += *pcw;
				break;
			}
		}
		PosX = nextPartX;
	}
}

uint CVConLine::GetNextPartX(const uint part)
{
	uint nextPartX = ((part+1) < PartsCount) ? TextParts[part + 1].CellPosX
		: (TextWidth * FontWidth);
	return nextPartX;
}

#if 0
void CVirtualConsole::UpdateText()
{
	_ASSERTE((HDC)m_DC!=NULL);
	memmove(mh_FontByIndex, gpSetCls->mh_Font, MAX_FONT_STYLES*sizeof(mh_FontByIndex[0]));
	mh_FontByIndex[MAX_FONT_STYLES] = mh_UCharMapFont ? mh_UCharMapFont : mh_FontByIndex[0];
	SelectFont(mh_FontByIndex[0]);
	// pointers
	wchar_t* ConCharLine = NULL;
	CharAttr* ConAttrLine = NULL;
	DWORD* ConCharXLine = NULL;
	// counters
	int pos, row;
	{
		int i;
		i = 0; //TextLen - TextWidth; // TextLen = TextWidth/*80*/ * TextHeight/*25*/;
		pos = 0; //Height - nFontHeight; // Height = TextHeight * nFontHeight;
		row = 0; //TextHeight - 1;
		ConCharLine = mpsz_ConChar + i;
		ConAttrLine = mpn_ConAttrEx + i;
		ConCharXLine = ConCharX + i;
	}
	int nMaxPos = Height - nFontHeight;

	if (!ConCharLine || !ConAttrLine || !ConCharXLine)
	{
		MBoxAssert(ConCharLine && ConAttrLine && ConCharXLine);
		return;
	}


	if (/*gpSet->isForceMonospace ||*/ !drawImage)
		m_DC.SetBkMode(OPAQUE);
	else
		m_DC.SetBkMode(TRANSPARENT);

	int *nDX = (int*)malloc((TextWidth+1)*sizeof(int));

	bool bEnhanceGraphics = gpSet->isEnhanceGraphics;
	bool bProportional = gpSet->isMonospace == 0;
	bool bForceMonospace = gpSet->isMonospace == 2;
	bool bFixFarBorders = gpSet->isFixFarBorders;
	CEFONT hFont = gpSetCls->mh_Font[0];
	CEFONT hFont2 = gpSetCls->mh_Font2;

	CVConLine lp; // Line parser
	lp.SetDialogs(mn_DialogsCount, mrc_Dialogs, mn_DialogFlags, mn_DialogAllFlags, mrc_UCharMap);

	bool isFixFrameCoord = mp_RCon->isFar();

	_ASSERTE(((TextWidth * nFontWidth) >= Width));

	//BUGBUG: хорошо бы отрисовывать последнюю строку, даже если она чуть не влазит
	for (; pos <= nMaxPos;
			ConCharLine += TextWidth, ConAttrLine += TextWidth, ConCharXLine += TextWidth,
			pos += nFontHeight, row++)
	{
		if (row >= (int)TextHeight)
		{
			//_ASSERTE(row < (int)TextHeight);
			DEBUGSTRFAIL(L"############ _ASSERTE(row < (int)TextHeight) #############\n");
			break;
		}

		// the line
		const CharAttr* const ConAttrLine2 = mpn_ConAttrExSave + (ConAttrLine - mpn_ConAttrEx);
		const wchar_t* const ConCharLine2 = mpsz_ConCharSave + (ConCharLine - mpsz_ConChar);

		// If there were no changes in this line
		if (!isForce && !FindChanges(row, ConCharLine, ConAttrLine, ConCharLine2, ConAttrLine2))
			continue;

		// skip not changed symbols except the old cursor or selection
		int j = 0, end = TextWidth;

		// Was cursor visible on this line?
		if (Cursor.isVisiblePrev && row == Cursor.y
				&& (j <= Cursor.x && Cursor.x <= end))
			Cursor.isVisiblePrev = false;

		lp.ParseLine(isForce, TextWidth, ConCharLine, ConAttrLine, ConCharLine2, ConAttrLine2);

		// *) Now draw as much as possible in a row even if some symbols are not changed.
		// More calls for the sake of fewer symbols is slower, e.g. in panel status lines.
		int j2=j+1;

		bool NextDialog = true;
		int NextDialogX = -1, CurDialogX1 = 0, CurDialogX2 = 0, CurDialogI = 0;
		DWORD CurDialogFlags = 0;

		for (; j < end; j = j2)
		{
			if (NextDialog || (j == NextDialogX))
			{
				NextDialog = false;
				CurDialogX1 = -1;
				NextDialogX = CurDialogX2 = TextWidth;
				CurDialogI = -1; CurDialogFlags = 0;
				int nMax = TextWidth-1;
				// Идем "снизу вверх", чтобы верхние (по Z-order) диалоги обработались первыми
				for (int i = mn_DialogsCount-1; i >= 0; i--)
				{
					if (mrc_Dialogs[i].Top <= row && row <= mrc_Dialogs[i].Bottom)
					{
						int border1 = mrc_Dialogs[i].Left;
						int border2 = mrc_Dialogs[i].Right;

						// Ищем грань диалога, лежащую на этой строке, ближайший к текущей X-координате.
						if (border1 && j <= border1 && border1 < NextDialogX)
							NextDialogX = border1;
						else if (border2 < nMax && j <= border2 && border2 < NextDialogX)
							NextDialogX = border2;

						// Ищем диалог (не панели), лежащий на этой строке, содержащий текущую X-координату.
						TODO("Разделители колонок тоже хорошо бы обработать, но как бы не пересечься");
						if (!(mn_DialogFlags[i] & (FR_LEFTPANEL|FR_RIGHTPANEL|FR_FULLPANEL|FR_VIEWEREDITOR)))
						{
							if ((border1 <= j && j <= border2)
								&& (CurDialogX1 <= border1 && border2 <= CurDialogX2))
							{
								CurDialogX1 = border1;
								CurDialogX2 = border2;
								CurDialogFlags = mn_DialogFlags[i];
								_ASSERTE(CurDialogFlags!=0);
								CurDialogI = i;
							}
						}
					}
				}
			}

			TODO("OPTIMIZE: вынести переменные из цикла");
			TODO("OPTIMIZE: если символ/атрибут совпадает с предыдущим (j>0 !) то не звать повторно CharWidth, isChar..., isUnicode, ...");
			const CharAttr attr = ConAttrLine[j];
			wchar_t c = ConCharLine[j];
			//BYTE attrForeIdx, attrBackIdx;
			//COLORREF attrForeClr, attrBackClr;
			bool isUnicode, isDlgBorder = false;
			if (j == NextDialogX)
			{
				NextDialog = true;
				// Проверяем что это, видимая грань, или поверх координаты другой диалог лежит?
				if (CurDialogFlags)
				{
					// Координата попала в поле диалога. same as below (1)
					isUnicode = (gpSet->isFixFarBorders && isCharAltFont(c/*ConCharLine[j]*/));
					isDlgBorder = (j == CurDialogX1 || j == CurDialogX2); // чтобы правая грань пошла как рамка;
				}
				else
				{
					isDlgBorder = isUnicode = true;
				}
			}
			else
			{
				// same as above (1)
				isUnicode = (gpSet->isFixFarBorders && isCharAltFont(c/*ConCharLine[j]*/));
			}
			bool isProgress = false;
			bool isSpace = false;
			bool isUnicodeOrProgress = false;
			bool isSeparate = false;
			bool lbS1 = false, lbS2 = false;
			int nS11 = 0, nS12 = 0, nS21 = 0, nS22 = 0;
			//GetCharAttr(c, attr, c, attrFore, attrBack);
			//GetCharAttr(attr, attrFore, attrBack, &hFont);
			//if (GetCharAttr(c, attr, c, attrFore, attrBack))
			//    isUnicode = true;
			hFont = mh_FontByIndex[attr.nFontIndex];

			if (isUnicode || bEnhanceGraphics)
				isProgress = isCharProgress(c); // ucBox25 / ucBox50 / ucBox75 / ucBox100

			isUnicodeOrProgress = isUnicode || isProgress;

			if (!isUnicodeOrProgress)
			{
				isSeparate = isCharSeparate(c);
				isSpace = isCharSpace(c);
			}

			TODO("При позиционировании (наверное в DistrubuteSpaces) пытаться ориентироваться по координатам предыдущей строки");
			// Иначе окантовка диалогов съезжает на пропорциональных шрифтах
			HEAPVAL

			// корректировка лидирующих пробелов и рамок
			// 120616 - Chinese - off
			if (bProportional && isFixFrameCoord && (c==ucBoxDblHorz || c==ucBoxSinglHorz))
			{
				lbS1 = true; nS11 = nS12 = j;

				while ((nS12 < end) && (ConCharLine[nS12+1] == c))
					nS12 ++;

				// Посчитать сколько этих же символов ПОСЛЕ текста
				if (nS12<end)
				{
					nS21 = nS12+1; // Это должен быть НЕ c

					// ищем первый "рамочный" символ
					while ((nS21<end) && (ConCharLine[nS21] != c) && !isCharAltFont(ConCharLine[nS21]))
						nS21 ++;

					if (nS21<end && ConCharLine[nS21]==c)
					{
						lbS2 = true; nS22 = nS21;

						while ((nS22 < end) && (ConCharLine[nS22+1] == c))
							nS22 ++;
					}
				}
			} HEAPVAL

			m_DC.SetTextColor(attr.crForeColor);

			DWORD nPrevX = j ? ConCharXLine[j-1] : 0;

			// корректировка положения вертикальной рамки (Coord.X>0)
			if (j && isFixFrameCoord)
			{
				// Рамки (их вертикальные части) и полосы прокрутки;
				// Символом } фар помечает имена файлов вылезшие за пределы колонки...
				// Если сверху или снизу на той же позиции рамки (или тот же '}')
				// корректируем позицию
				//bool bBord = isDlgBorder || isCharBorderVertical(c);
				//TODO("Пока не будет учтено, что поверх рамок может быть другой диалог!");
				//bool bFrame = false; // mpn_ConAttrEx[row*TextWidth+j].bDialogVBorder;

				if (isDlgBorder || isCharBorderVertical(c) || isCharScroll(c) || (isFilePanel && c==L'}'))
				{
					//2009-04-21 было (j-1) * nFontWidth;
					//ConCharXLine[j-1] = j * nFontWidth;
					bool bMayBeFrame = true;

					// Пройти вверх и вниз от текущей строки, проверив,
					// есть ли в этой же X-координате вертикальная рамка (или угол)
					TODO("Хорошо бы для панелей определять границы колонок более корректно, лучше всего - через API?");
					//if (!bBord && !bFrame)
					if (!isDlgBorder)
					{
						int R;
						wchar_t prevC;
						bool bBord = false;

						for (R = (row-1); bMayBeFrame && !bBord && R>=0; R--)
						{
							prevC = mpsz_ConChar[R*TextWidth+j];
							bBord = isCharBorderVertical(prevC);
							bMayBeFrame = (bBord || isCharScroll(prevC) || (isFilePanel && prevC==L'}'));
						}

						for (R = (row+1); bMayBeFrame && !bBord && R < (int)TextHeight; R++)
						{
							prevC = mpsz_ConChar[R*TextWidth+j];
							bBord = isCharBorderVertical(prevC);
							bMayBeFrame = (bBord || isCharScroll(prevC) || (isFilePanel && prevC==L'}'));
						}
					}

					// Это разделитель колонок панели, полоса прокрутки или явно найденная граница диалога
					if (bMayBeFrame)
						ConCharXLine[j-1] = j * nFontWidth;
				}

			}

			/*
			// Other alignments? For non-Far-Manager applications?
			if (j && (ConCharXLine[j-1] < (j * nFontWidth))
				&& (isCharBorderVertical(c) || (isFilePanel && c == L'}')))
			{
				ConCharXLine[j-1] = (j * nFontWidth);
			}
			*/

			if (j)
			{
				if (nPrevX < ConCharXLine[j-1])
				{
					// Требуется зарисовать пропущенную область. пробелами что-ли?
					RECT rect;
					rect.left = nPrevX;
					rect.top = pos;
					rect.right = ConCharXLine[j-1];
					rect.bottom = rect.top + nFontHeight;

					if (gbNoDblBuffer) GdiFlush();

					const CharAttr PrevAttr = ConAttrLine[j-1];

					// Если не отрисовка фона картинкой
					if (!(drawImage && ISBGIMGCOLOR(attr.nBackIdx)))
					{
						wchar_t PrevC = ConCharLine[j-1];

						// Если текущий символ - вертикальная рамка, а предыдущий символ - рамка
						// нужно продлить рамку до текущего символа
						if (isCharBorderVertical(c) && isCharAltFont(PrevC))
						{
							//m_DC.SetBkColor(pColors[attrBack]);
							m_DC.SetBkColor(attr.crBackColor);
							wchar_t *pchBorder = (c == ucBoxDblDownLeft || c == ucBoxDblUpLeft
												  || c == ucBoxSinglDownDblHorz || c == ucBoxSinglUpDblHorz
												  || c == ucBoxDblDownDblHorz || c == ucBoxDblUpDblHorz
												  || c == ucBoxDblVertLeft
												  || c == ucBoxDblVertHorz) ? ms_HorzDbl : ms_HorzSingl;
							int nCnt = ((rect.right - rect.left) / CharWidth(pchBorder[0], attr))+1;

							if (nCnt > MAX_SPACES)
							{
								_ASSERTE(nCnt<=MAX_SPACES);
								nCnt = MAX_SPACES;
							}

							UINT nFlags = ETO_CLIPPED | ETO_OPAQUE;

							if (bFixFarBorders)
								SelectFont(hFont2);

							m_DC.SetTextColor(PrevAttr.crForeColor);
							m_DC.TextDraw(rect.left, rect.top, nFlags, &rect, pchBorder, nCnt, 0);
							m_DC.SetTextColor(attr.crForeColor); // Вернуть
						}
						else
						{
							HBRUSH hbr = PartBrush(L' ', PrevAttr.crBackColor, PrevAttr.crForeColor);
							FillRect((HDC)m_DC, &rect, hbr);
						}
					}
					else if (drawImage)
					{
						// Заливка пропущенной области
						RECT rcFill = {rect.left, rect.top, rect.right, rect.bottom};
						PaintBackgroundImage(rcFill, PrevAttr.crBackColor);
					} HEAPVAL

					if (gbNoDblBuffer) GdiFlush();
				}
			}

			WORD nCurCharWidth = CharWidth(c, attr);
			ConCharXLine[j] = (j ? ConCharXLine[j-1] : 0)+nCurCharWidth;
			HEAPVAL
			DEBUGTEST(nCurCharWidth=nCurCharWidth);

			{
				wchar_t* pszDraw = NULL;
				CharAttr* pDrawAttr = NULL;
				int      nDrawLen = -1;
				bool     bDrawReplaced = false;
				RECT rect;

				if (isSpace)
				{
					j2 = j + 1;

					while (j2 < end && ConAttrLine[j2] == attr && isCharSpace(ConCharLine[j2]))
						j2++;

					DistributeSpaces(ConCharLine, ConAttrLine, ConCharXLine, j, j2, end);
				}
				else if (isSeparate)
				{
					j2 = j + 1; HEAPVAL
					wchar_t ch;
					//int nLastNonSpace = -1;
					while (j2 < end && ConAttrLine[j2] == attr
					        && isCharSeparate(ch = ConCharLine[j2]))
					{
						WORD nCurCharWidth2 = CharWidth(ch, attr);
						ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+nCurCharWidth2;
						j2++;
					}
					SelectFont(hFont);
					HEAPVAL
				}
				else if (!isUnicodeOrProgress)
				{
					j2 = j + 1; HEAPVAL

					// Если этого не делать - в пропорциональных шрифтах буквы будут наезжать одна на другую
					wchar_t ch = 0;
					//int nLastNonSpace = -1;
					WARNING("*** сомнение в следующей строчке: (!gpSet->isProportional || !isFilePanel || (ch != L'}' && ch!=L' '))");
					TODO("при поиске по строке - обновлять nLastNonSpace");

					// We need to aligh Far Frames (Panels) after non-spacings, acutes, CJK and others...
					int j3 = 0;
					// Far Manager shows '}' instead of '│' or '║' when file/dir name is too long for column
					bool bAlignCurledFrames = isFixFrameCoord && isFilePanel;

					while (j2 < end && ConAttrLine[j2] == attr
							&& (ch = ConCharLine[j2], bFixFarBorders ? !isCharAltFont(ch) : (!bEnhanceGraphics || !isCharProgress(ch)))
							&& !isCharSeparate(ch)
							&& (!bAlignCurledFrames || (ch != L'}')))
					{
						if (isSpace(ch))
						{
							if (!j3) j3 = j2;
						}
						else if (j3)
						{
							j3 = 0;
						}
						WORD nCurCharWidth2 = CharWidth(ch, ConAttrLine[j2]);
						ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+nCurCharWidth2;
						j2++;
					}

					if (j3 && (isCharBorderVertical(ch) || ((j2 < end) && (isCharBorderVertical(ConCharLine[j2])))))
					{
						j2 = j3;
					}

					TODO("От хвоста - отбросить пробелы (если если есть nLastNonSpace)");
					SelectFont(hFont);
					HEAPVAL
				}
				else
				{
					// Сюда мы попадаем для символов, которые отрисовываются "рамочным" шрифтом
					// Однако, это могут быть не только рамки, но и много чего еще, что попросил
					// пользователь (настройка диапазонов -> gpSet->icFixFarBorderRanges)
					j2 = j + 1; HEAPVAL

					// блоки 25%, 50%, 75%, 100%
					if (bEnhanceGraphics && isProgress)
					{
						wchar_t ch;
						ch = c; // Графическая отрисовка прокрутки и прогресса

						while (j2 < end && ConAttrLine[j2] == attr && ch == ConCharLine[j2])
						{
							WORD nCurCharWidth2 = CharWidth(ch, attr);
							ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+nCurCharWidth2;
							j2++;
						}
					}
					else if (!bFixFarBorders)
					{
						// Сюда попадаем, если это символ рамки, но 'Fix Far borders' отключен.
						// Смысл в том, чтобы отрисовать рамку точно по знакоместу консоли.
						// -- Сюда вроде попадать не должны - тогда isUnicodeOrProgress должен быть false
						// -- т.к. в настройке отключено gpSet->isFixFarBorders
						// -- _ASSERTE(bFixFarBorders);
						// -- _ASSERTE(!isUnicodeOrProgress);
						wchar_t ch;

						while (j2 < end && ConAttrLine[j2] == attr && isCharAltFont(ch = ConCharLine[j2]))
						{
							WORD nCurCharWidth2 = CharWidth(ch, ConAttrLine[j2]);
							ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+nCurCharWidth2;
							j2++;
						}
					}
					else
					{
						//wchar_t ch;
						//WARNING("Тут обламывается на ucBoxDblVert в первой позиции. Ее ширину ставит в ...");
						// Для отрисовки рамок (ucBoxDblHorz) за один проход
						bool bIsHorzBorder = (c == ucBoxDblHorz || c == ucBoxSinglHorz);

						if (bIsHorzBorder)
						{
							DEBUGTEST(WORD nCurCharWidth2 = CharWidth(c, attr));
							_ASSERTE(nCurCharWidth2 == nCurCharWidth);
							while (j2 < end && ConAttrLine[j2] == attr && c == ConCharLine[j2])
							{
								ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+nCurCharWidth;
								j2++;
							}
						}

						// Why that was called? It breaks evaluated previously hieroglyphs positions on DBCS
						//DistributeSpaces(ConCharLine, ConAttrLine, ConCharXLine, j, j2, end);

						int nBorderWidth = nFontWidth;
						rect.left = j ? ConCharXLine[j-1] : 0;
						rect.right = (TextWidth>(UINT)j2) ? ConCharXLine[j2-1] : Width;
						int nCnt = klMin((long)TextWidth,(rect.right - rect.left + (nBorderWidth>>1)) / nBorderWidth);

						if (nCnt > (j2 - j))
						{
							if (c==ucBoxDblHorz || c==ucBoxSinglHorz)
							{
								_ASSERTE(nCnt <= MAX_SPACES);

								if (nCnt > MAX_SPACES) nCnt = MAX_SPACES;

								bDrawReplaced = true;
								nDrawLen = nCnt;
								pszDraw = (c==ucBoxDblHorz) ? ms_HorzDbl : ms_HorzSingl;
							}
							#ifdef _DEBUG
							else
							{
								//static bool bShowAssert = true;
								//if (bShowAssert) {
								_ASSERTE(!bIsHorzBorder || (c==ucBoxDblHorz || c==ucBoxSinglHorz));
								//}
							}
							#endif
						}

					}

					SelectFont(bFixFarBorders ? hFont2 : hFont);
					HEAPVAL
				}

				if (!pszDraw)
				{
					pszDraw = ConCharLine + j;
					pDrawAttr = ConAttrLine + j;
					nDrawLen = j2 - j;
					_ASSERTE(nDrawLen <= (int)TextWidth);
				}
				_ASSERTE(nDrawLen >= 0);

				if (j)
				{
					if (ConCharXLine[j-1])
					{
						rect.left = ConCharXLine[j-1];
					}
					else
					{
						_ASSERTE((ConCharXLine[j-1]!=0) && "Must be already filled");
						rect.left = j * nFontWidth;
					}
				}
				else
				{
					rect.left = 0;
				}
				rect.top = pos;
				if (TextWidth>(UINT)j2)
				{
					if (ConCharXLine[j2-1])
					{
						rect.right = ConCharXLine[j2-1];
					}
					else
					{
						_ASSERTE((ConCharXLine[j2-1]!=0) && "Must be already filled");
						rect.right = j2 * nFontWidth;
					}
				}
				else
				{
					rect.right = Width;
				}
				rect.bottom = rect.top + nFontHeight;
				_ASSERTE(rect.left > 0 || (j <= 2));
				//}
				HEAPVAL
				BOOL lbImgDrawn = FALSE;

				if (!(drawImage && ISBGIMGCOLOR(attr.nBackIdx)))
				{
					//m_DC.SetBkColor(pColors[attrBack]);
					m_DC.SetBkColor(attr.crBackColor);
				}
				else if (drawImage)
				{
					// Подготовка фона (BgImage) для отрисовки текста на нем
					RECT rcFill = {rect.left, rect.top, rect.right, rect.bottom};
					PaintBackgroundImage(rcFill, attr.crBackColor);
					lbImgDrawn = TRUE;
				}

				WARNING("ETO_CLIPPED вроде вообще не нарисует символ, если его часть не влезает?");
				UINT nFlags = ETO_CLIPPED | ((drawImage && ISBGIMGCOLOR(attr.nBackIdx)) ? 0 : ETO_OPAQUE);
				HEAPVAL

				if (gbNoDblBuffer) GdiFlush();

				// There is no sense to try ‘partial brushes’ if foreground is equal to background
				if (isProgress && bEnhanceGraphics
					// Don't use PartBrush if bg==fg unless it is 100% filled box
					&& ((attr.crBackColor != attr.crForeColor) || (c == ucBox100)))
				{
					HBRUSH hbr = PartBrush(c, attr.crBackColor, attr.crForeColor);
					FillRect((HDC)m_DC, &rect, hbr);
				}
				else if (/*gpSet->isProportional &&*/ isSpace/*c == ' '*/)
				{
					TODO("Проверить, что будет если картинка МЕНЬШЕ по ширине чем область отрисовки");

					if (!lbImgDrawn)
					{
						HBRUSH hbr = PartBrush(L' ', attr.crBackColor, attr.crForeColor);
						FillRect((HDC)m_DC, &rect, hbr);
					}
				}
				else
				{
					if (nFontCharSet == OEM_CHARSET && !(bFixFarBorders && isUnicode))
					{
						if (nDrawLen > (int)TextWidth)
						{
							_ASSERTE(nDrawLen <= (int)TextWidth);
							nDrawLen = TextWidth;
						}

						WideCharToMultiByte(CP_OEMCP, 0, pszDraw, nDrawLen, tmpOem, TextWidth, 0, 0);

						if (!bProportional)
						{
							for (int idx = 0; idx < nDrawLen; idx++)
							{
								WARNING("BUGBUG: что именно нужно передавать для получения ширины OEM символа?");
								nDX[idx] = CharWidth(tmpOem[idx], attr);
							}
							HEAPVALPTR(nDX);
						}

						m_DC.TextDrawOem(rect.left, rect.top, nFlags,
						            &rect, tmpOem, nDrawLen, bProportional ? 0 : nDX);
					}
					else
					{
						int nShift0 = 0, nPrevEdge = 0;
						ABC abc;

						if (nDrawLen > (int)TextWidth)
						{
							_ASSERTE(nDrawLen <= (int)TextWidth);
							nDrawLen = TextWidth;
						}

						// Для пропорциональных шрифтов - где рисовать каждый символ разбирается GDI
						if (!bProportional)
						{
							if (!bForceMonospace)
							{
								// В этом режиме символ отрисовывается в начале своей ячейки
								// Информация для GDI. Ширина отрисовки, т.е. сдвиг между текущим и следующим символом.
								if (!pDrawAttr)
								{
									for (int idx = 0, n = nDrawLen; n; idx++, n--)
										nDX[idx] = nFontWidth;
									HEAPVALPTR(nDX);
								}
								else
								{
									for (int idx = 0, n = nDrawLen; n; idx++, n--)
										nDX[idx] = CharWidth(pszDraw[idx], pDrawAttr[idx]);
									HEAPVALPTR(nDX);
								}
							}
							else
							{
								for (int idx = 0, n = nDrawLen; n; idx++, n--)
								{
									WARNING("Для RTL нужно переделать");
									// GDI учитывает nDX не по порядку следования символов в памяти,
									// а по порядку отрисовки (что логично), то есть нужно смотреть на строку,
									// брать кусок RTL и считать nDX для pszDraw в обратном порядке

									wchar_t ch = pszDraw[idx];
									LONG nCharWidth = CharWidth(ch, pDrawAttr ? pDrawAttr[idx] : attr);

									if (isCharSpace(ch)
										|| (attr.Flags2 & CharAttr2_DoubleSpaced)
										|| isCharAltFont(ch)
										|| isCharProgress(ch))
									{
										abc.abcA = abc.abcC = 0;
										abc.abcB = nCharWidth;
									}
									else
									{
										//Для НЕ TrueType вызывается wrapper (CharWidth)
										CharABC(ch, &abc);
									}

									int nDrawWidth = abc.abcA + abc.abcB + 1;

									#ifdef _DEBUG
									if (abc.abcA!=0)
									{
										int nDbgA = 0;
									}
									if (abc.abcC!=0)
									{
										// Для японских иероглифов - "1"
										int nDbgC = 0;
									}
									#endif

									if (nDrawWidth < nCharWidth)
									{
										int nEdge = ((nCharWidth - nDrawWidth) >> 1) - nPrevEdge;

										if (idx)
										{
											nDX[idx-1] += nEdge;
											_ASSERTE(nDX[idx-1]>=-100);
											HEAPVALPTR(nDX);
										}
										else
										{
											nShift0 += nEdge;
										}

										nPrevEdge += nEdge;

										nDX[idx] = nCharWidth;
										HEAPVALPTR(nDX);

									}
									else
									{
										// Ширина отрисовываемой части больше чем знакоместо,
										// но т.к. юзер хотел режим Monospace - принудительно
										// выставляем ширину ячейки
										nDX[idx] = nCharWidth; // abc.abcA + abc.abcB /*+ abc.abcC*/;
										if (nPrevEdge)
										{
											_ASSERTE(idx>0 && "Must be, cause of nPrevEdge");
											nDX[idx-1] -= nPrevEdge;
											nPrevEdge = 0;
										}
										HEAPVALPTR(nDX);
										//_ASSERTE(abc.abcC==0 && "Check what symbols can produce '!=0'");
									}
								}
							}
						}

						_ASSERTE(rect.left > 0 || (j <= 2));

						// nDX это сдвиги до начала следующего символа, с начала предыдущего
						m_DC.TextDraw(rect.left+nShift0, rect.top, nFlags, &rect,
						           pszDraw, nDrawLen, bProportional ? 0 : nDX);
					}
				}

				if (gbNoDblBuffer) GdiFlush();

				HEAPVAL
			}

			DUMPDC(L"F:\\vcon.png");
		}

		HEAPVALPTR(nDX);
	}

	if ((pos > nMaxPos) && (pos < (int)Height))
	{
		bool lbDelBrush = false;
		HBRUSH hBr = CreateBackBrush(false, lbDelBrush);

		RECT rcFill = {0, pos, Width, Height};
		FillRect((HDC)m_DC, &rcFill, hBr);

		if (lbDelBrush)
			DeleteObject(hBr);
	}

	free(nDX);


	// Screen updated, reset until next "UpdateHighlights()" call
	m_HighlightInfo.m_Last.X = m_HighlightInfo.m_Last.Y = -1;
	ZeroStruct(m_HighlightInfo.mrc_LastRow);
	ZeroStruct(m_HighlightInfo.mrc_LastCol);
	ZeroStruct(m_HighlightInfo.mrc_LastHyperlink);

	return;
}

// На самом деле не только пробелы, но и ucBoxSinglHorz/ucBoxDblVertHorz
void CVirtualConsole::DistributeSpaces(wchar_t* ConCharLine, CharAttr* ConAttrLine, DWORD* ConCharXLine, const int j, const int j2, const int end)
{
	//WORD attr = ConAttrLine[j];
	//wchar_t ch, c;
	//
	//if ((c=ConCharLine[j]) == ucSpace || c == ucNoBreakSpace || c == 0)
	//{
	//	while (j2 < end && ConAttrLine[j2] == attr
	//		// также и для &nbsp; и 0x00
	//		&& ((ch=ConCharLine[j2]) == ucSpace || ch == ucNoBreakSpace || ch == 0))
	//		j2++;
	//} else
	//if ((c=ConCharLine[j]) == ucBoxSinglHorz || c == ucBoxDblVertHorz)
	//{
	//	while (j2 < end && ConAttrLine[j2] == attr
	//		&& ((ch=ConCharLine[j2]) == ucBoxSinglHorz || ch == ucBoxDblVertHorz))
	//		j2++;
	//} else
	//if (isCharProgress(c=ConCharLine[j]))
	//{
	//	while (j2 < end && ConAttrLine[j2] == attr && (ch=ConCharLine[j2]) == c)
	//		j2++;
	//}
	_ASSERTE(j2 > j && j >= 0);

	// Ширину каждого "пробела" (или символа рамки) будем считать пропорционально занимаемому ИМИ месту

	if (j2>=(int)TextWidth)    // конец строки
	{
		ConCharXLine[j2-1] = Width;
	}
	else
	{
		//2009-09-09 Это некорректно. Ширина шрифта рамки может быть больше знакоместа
		//int nBordWidth = gpSet->BorderFontWidth(); if (!nBordWidth) nBordWidth = nFontWidth;
		int nBordWidth = nFontWidth;

		// Определить координату конца последовательности
		if (isCharBorderVertical(ConCharLine[j2]))
		{
			//2009-09-09 а это соответственно не нужно (пока nFontWidth == nBordWidth)
			//ConCharXLine[j2-1] = (j2-1) * nFontWidth + nBordWidth; // или тут [j] должен быть?
			ConCharXLine[j2-1] = j2 * nBordWidth;
		}
		else
		{
			TODO("Для пропорциональных шрифтов надо делать как-то по другому!");

			//CJK hieroglyps may take double width so we need to check if it will cover previous char/hieroglyps
			if ((j > 1) && (!gpSet->isMonospace || (gbIsDBCS && (ConCharXLine[j-1] > (DWORD)(j * nBordWidth)))))
			{
				//2009-12-31 нужно плясать от предыдущего символа!
				ConCharXLine[j2-1] = ConCharXLine[j-1] + (j2 - j) * nBordWidth;
			}
			else
			{
				ConCharXLine[j2-1] = j2 * nBordWidth;
			}
		}
	}

	if (j2 > (j + 1))
	{
		HEAPVAL
		DWORD n1 = (j ? ConCharXLine[j-1] : 0); // j? если j==0 то тут у нас 10 (правая граница 0го символа в строке)
		DWORD n3 = j2-j; // кол-во символов
		_ASSERTE(n3>0);
		DWORD n2 = ConCharXLine[j2-1] - n1; // выделенная на пробелы ширина
		HEAPVAL

		for (int k = j, l = 1; k < (j2-1); k++, l++)
		{
			#ifdef _DEBUG
			DWORD nn = n1 + (n3 ? klMulDivU32(l, n2, n3) : 0);

			if (nn != ConCharXLine[k])
				ConCharXLine[k] = nn;
			#endif

			ConCharXLine[k] = n1 + (n3 ? klMulDivU32(l, n2, n3) : 0);
			//n1 + (n3 ? klMulDivU32(k-j, n2, n3) : 0);
			HEAPVAL
		}
	}
}
#endif
