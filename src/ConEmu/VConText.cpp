
/*
Copyright (c) 2015-2017 Maximus5
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
#include <math.h>

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
bool isCharAltFont(ucs32 inChar)
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

bool isCharPseudographics(ucs32 inChar)
{
	bool isPseudo = ((inChar >= 0x2013) && (inChar <= 0x25C4));
	return isPseudo;
}

// Some pseudographics characters may be shrinked freely
bool isCharPseudoFree(ucs32 inChar)
{
	bool isFree = (inChar == ucBoxSinglHorz) || (inChar == ucBoxDblHorz);
	return isFree;
}

// These are "frame" characters, which has either
// * any vertical (even partial) line
// * or Up/Down arrows (scrollers)
bool isCharBorderVertical(ucs32 inChar)
{
	bool isVert = ((inChar >= ucBoxSinglVert)
		&& ((inChar <= ucBoxSinglUpHorz)
			|| ((inChar >= ucBoxDblVert) && (inChar <= ucBoxDblVertHorz))
			|| ((inChar == ucArrowUp) || (inChar == ucArrowDown))
			));
	return isVert;
}

bool isCharProgress(ucs32 inChar)
{
	bool isProgress = (inChar == ucBox25 || inChar == ucBox50 || inChar == ucBox75 || inChar == ucBox100);
	return isProgress;
}

bool isCharScroll(ucs32 inChar)
{
	bool isScrollbar = (inChar == ucBox25 || inChar == ucBox50 || inChar == ucBox75 || inChar == ucBox100
	                    || inChar == ucUpScroll || inChar == ucDnScroll);
	return isScrollbar;
}

bool isCharTriangles(ucs32 inChar)
{
	return ((inChar == ucTrgLeft) || (inChar == ucTrgRight));
}

bool isCharSeparate(ucs32 inChar)
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
bool isCharSpace(ucs32 inChar)
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
bool isCharSpaceSingle(ucs32 inChar)
{
	//TODO: Tabs (\t 0x09)?
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

bool isCharRTL(ucs32 inChar)
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

bool isCharCJK(ucs32 inChar)
{
	return is_char_cjk(inChar);
}

bool isCharComining(ucs32 inChar)
{
	return is_char_combining(inChar);
}

bool isCharNonWord(ucs32 inChar)
{
	if (isCharSpace(inChar)
		|| isCharSeparate(inChar)
		|| isCharPseudographics(inChar)
		)
		return true;
	return false;
}

bool isCharPunctuation(ucs32 inChar)
{
	// .,!:;?()<>[]{}¡¿
	return ((inChar <= 0xBF)
		&& ((inChar == L'.') || (inChar == L',') || (inChar == L'!') || (inChar == L':')
			|| (inChar == L';') || (inChar == L'?') || (inChar == L'¡') || (inChar == L'¿')
			|| (inChar == L'(') || (inChar == L')') || (inChar == L'{') || (inChar == L'}')
			|| (inChar == L'<') || (inChar == L'>') || (inChar == L'[') || (inChar == L']')
		));
}



//TODO: 1 pixel for all shrinkable characters? prefer some function of Length...
#define MIN_SIZEFREE_WIDTH ((FontWidth+1) >> 1)
#define DEF_SINGLE_MUL 8
#define DEF_SINGLE_DIV 10
#define DEF_DOUBLE_MUL 7
#define DEF_DOUBLE_DIV 10

void VConTextPart::Init(uint anIndex, uint anCell, CVConLine* pLine)
{
	Flags = TRF_None;
	Length = 1; // initially, we'll enlarge it later
	TotalWidth = 0;
	Index = anIndex;
	Cell = anCell; // May differ from anIndex on DBCS systems
	PositionX = CellPosX = Cell * pLine->FontWidth;
	Chars = pLine->ConCharLine + anIndex;
	Attrs = pLine->ConAttrLine + anIndex;
	CharFlags = pLine->TempCharFlags + anIndex;
	CharWidth = pLine->TempCharWidth + anIndex;
}

void VConTextPart::SetLen(uint anLen, uint FontWidth)
{
	_ASSERTE(anLen>0 && anLen<=0xFFFF);
	Length = anLen;

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

	uint FontWidth2 = 2*FontWidth;

	const CharAttr* pca = Attrs;
	TextCharType* pcf = CharFlags;
	uint* pcw = CharWidth;

	// Preliminary character widths estimation

	if (Flags & TRF_SizeFree)
	{
		// spaces, horizontal lines (frames), etc. They may be shrinked freely
		for (uint left = anLen; left--; pca++, pcf++)
		{
			*pcw = FontWidth; // if non-shrinked
			*pcf = TCF_WidthFree;
		}
	}
	else
	{
		//WARNING: We rely here on pca->Flags2, but not on self evaluated data?
		for (uint left = anLen; left--; pca++, pcf++, pcw++)
		{
			if (pca->Flags2 & (CharAttr2_NonSpacing|CharAttr2_Combining))
			{
				// zero-width
				*pcw = 0;
				*pcf = TCF_WidthZero;
			}
			else if (pca->Flags2 & CharAttr2_DoubleSpaced)
			{
				*pcw = FontWidth2;
				*pcf = TCF_WidthDouble;
			}
			else
			{
				*pcw = FontWidth;
				*pcf = TCF_WidthNormal;
			}
		}
	}
}

void VConTextPart::Done()
{
	_ASSERTE(Length>0 && Length<=0xFFFF);
	TotalWidth = 0;

	uint* pcw = CharWidth;
	for (uint left = Length; left--; pcw++)
	{
		TotalWidth += *pcw; // if non-shrinked
	}
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
	, row(0)
	, ConCharLine(NULL)
	, ConAttrLine(NULL)
	, MaxBufferSize(0)
	, PartsCount(0)
	, TextParts(NULL)
	, TempCharFlags(NULL)
	, TempCharWidth(NULL)
	, TotalLineWidth(0)
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
	const bool bUseAlternativeFont = _bool(gpSet->isFixFarBorders);

	for (uint j = 0; j < TextWidth;)
	{
		bool bPair = ((j+1) < TextWidth);
		ucs32 wc = ucs32_from_wchar(ConCharLine+j, bPair);
		uint j2 = j + (bPair ? 2 : 1);

		const CharAttr attr = ConAttrLine[j];

		VConTextPart* p = TextParts+(PartsCount++);

		//TODO: DBCS cell number, it may differ from j
		p->Init(j, j, this);

		// Process Far Dialogs to justify rectangles and frames
		TextPartFlags dlgBorder = j ? isDialogBorderCoord(j) : TRF_None;

		/* *** Now we split our text line into parts with characters one "family" *** */

		_ASSERTE(p->Flags == TRF_None);

		if (dlgBorder)
		{
			p->Flags = dlgBorder | TRF_SizeFixed
				| (isCharBorderVertical(wc) ? TRF_TextPseudograph : TRF_None)
				//| (isCharSpaceSingle(wc) ? TRF_TextSpacing : TRF_None)
				| ((bUseAlternativeFont && isCharAltFont(wc)) ? TRF_TextAlternative : TRF_None)
				| ((bEnhanceGraphics && isCharProgress(wc)) ? TRF_TextProgress : TRF_None)
				;
		}
		else if (isCharSpaceSingle(wc))
		{
			// CJK Double-width space will be processed under isCharCJK
			p->Flags = TRF_TextSpacing;
			if (!bPair)
			while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharSpaceSingle(ConCharLine[j2]))
				j2++;
		}
		else if (isCharSeparate(wc))
		{
			p->Flags = TRF_TextSeparate;
			if (!bPair)
			while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharSeparate(ConCharLine[j2]))
				j2++;
		}
		// isCharProgress must be checked before isCharScroll!
		else if (bEnhanceGraphics && isCharProgress(wc))
		{
			p->Flags = TRF_TextProgress
				| ((bUseAlternativeFont && isCharAltFont(wc)) ? TRF_TextAlternative : TRF_None)
				;
			if (!bPair)
			while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharProgress(ConCharLine[j2]))
				j2++;
		}
		// isCharScroll has wider range than isCharProgress, and comes after it
		else if (bEnhanceGraphics && isCharScroll(wc))
		{
			p->Flags = TRF_TextScroll
				| ((bUseAlternativeFont && isCharAltFont(wc)) ? TRF_TextAlternative : TRF_None)
				;
			if (!bPair)
			while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharScroll(ConCharLine[j2]))
				j2++;
		}
		else if (bEnhanceGraphics && isCharTriangles(wc))
		{
			p->Flags = TRF_TextTriangles;
		}
		// Miscellaneous borders
		else if (isCharPseudographics(wc))
		{
			// Processed separately from isCharAltFont, because last may cover larger range
			p->Flags = TRF_TextPseudograph;
			if (bUseAlternativeFont && isCharAltFont(wc))
			{
				p->Flags |= TRF_TextAlternative;
				wchar_t wc2;
				if (!bPair)
				while ((j2 < TextWidth) && (ConAttrLine[j2] == attr)
					&& isCharPseudographics((wc2=ConCharLine[j2]))
					&& (!bEnhanceGraphics || !isCharScroll(wc2))
					&& !isCharBorderVertical(wc2)
					&& isCharAltFont(wc2))
					j2++;
			}
			else
			{
				wchar_t wc2;
				if (!bPair)
				while ((j2 < TextWidth) && (ConAttrLine[j2] == attr)
					&& isCharPseudographics((wc2=ConCharLine[j2]))
					&& (!bEnhanceGraphics || !isCharScroll(wc2))
					&& !isCharBorderVertical(wc2)
					)
					j2++;
			}
		}
		else if (isCharCJK(wc)) // Double-width characters (CJK, etc.)
		{
			// Processed separately from isCharAltFont, because last may cover larger range
			p->Flags = TRF_TextCJK;
			if (bUseAlternativeFont && isCharAltFont(wc))
			{
				p->Flags |= TRF_TextAlternative;
				wchar_t wc2;
				if (!bPair)
				while ((j2 < TextWidth) && (ConAttrLine[j2] == attr)
					&& isCharCJK((wc2=ConCharLine[j2]))
					&& isCharAltFont(wc2))
					j2++;
			}
			else
			{
				if (!bPair)
				while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharCJK(ConCharLine[j2]))
					j2++;
			}
		}
		else if (bUseAlternativeFont && isCharAltFont(wc))
		{
			p->Flags = TRF_TextAlternative;
			if (!bPair)
			while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharScroll(ConCharLine[j2]))
				j2++;
		}
		else
		{
			p->Flags = TRF_TextNormal;
			wchar_t wc2;
			// That's more complicated as previous branches,
			// we must break on all chars mentioned above
			if (!bPair)
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
		_ASSERTE(j2>j);
		p->SetLen(j2-j, FontWidth);

		#if 0
		TotalLineWidth += p->TotalWidth;
		MinLineWidth += p->MinWidth;
		#endif

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

	// VirtualConsole has set proper character widths, adopt them
	TotalLineWidth = 0;
	for (uint k = 0; k < PartsCount; k++)
	{
		VConTextPart& part = TextParts[k];
		part.Done();
		TotalLineWidth += part.TotalWidth;
	}

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
			if (part.Flags & TRF_SizeFixed)
			{
				for (uint i = 0; i < part.Length; i++)
				{
					#define CellWidth FontWidth
					if (part.CharWidth[i] != CellWidth)
					{
						part.TotalWidth += (int)((int)CellWidth - (int)part.CharWidth[i]);
						part.CharWidth[i] = CellWidth;
					}
				}
			}
			// Overlaps? Or may be lesser?
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
	// If last part does not end on screen edge - redistribute
	if (PosX != (TextWidth * FontWidth))
	{
		DistributeParts(prevFixedPart + 1, PartsCount - 1, (TextWidth * FontWidth));
	}


	/* Fill all X coordinates */
	PosX = CharIdx = 0;
	TotalLineWidth = 0;
	for (uint k = 0; k < PartsCount; k++)
	{
		VConTextPart& part = TextParts[k];
		_ASSERTE(part.Flags != TRF_None);
		_ASSERTE(((int)part.TotalWidth > 0) || (part.Flags & TRF_Cropped) || ((part.Length==1) && (part.CharFlags[0]==TCF_WidthZero)));

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

		if (!(part.Flags & TRF_TextSpacing))
		{
			_ASSERTE(TRF_CompareMask & TRF_TextAlternative);
			for (uint k2 = k+1; k2 < PartsCount; k2++)
			{
				VConTextPart& part2 = TextParts[k2];
				if (part2.Flags & (TRF_PosFixed|TRF_PosRecommended))
					break;
				WARNING("At the moment we do not care about CJK and RTL differences, unless TRF_TextAlternative is specified");
				// Let GDI deal with them, seems like it successfully
				// process strings, containing all types of characters
				if ((part.Flags & TRF_CompareMask) != (part2.Flags & TRF_CompareMask))
					break;
				if (!(attr == ConAttrLine[part2.Index]))
					break;
				part.Length += part2.Length;
				part.TotalWidth += part2.TotalWidth;
				// "Hide" this part from list
				part2.Flags = TRF_None;
				k = k2;
			}
		}
	}

	return;
}

// gh-739: Crop overrun parts
void CVConLine::CropParts(uint part1, uint part2, uint right)
{
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
		#ifdef _DEBUG
		TextCharType* pcf = part.CharFlags; // character flags (zero/free/normal/double)
		#endif
		uint* pcw = part.CharWidth; // pointer to character width

		// Run part shrink
		part.TotalWidth = 0;

		_ASSERTE(!(part.Flags & TRF_Cropped));

		for (uint c = 0;
			(c < part.Length);
			c++,
			#ifdef _DEBUG
			pcf++,
			#endif
			pcw++)
		{
			if ((PosX + part.TotalWidth + *pcw) <= right)
			{
				part.TotalWidth += *pcw;
			}
			else if ((PosX + part.TotalWidth) < right)
			{
				*pcw = right - (PosX + part.TotalWidth);
				part.TotalWidth += *pcw;
			}
			else
			{
				*pcw = 0;
			}
		}
		if (part.TotalWidth == 0)
			part.Flags |= TRF_Cropped;

		PosX += part.TotalWidth;
	}

	// End of shrink (no more parts, no more methods)
	_ASSERTE(PosX <= right);
}

struct Shrinker
{
	uint cell_width;

	// constants for selected method
	uint indent_elast; // = 100;
	uint space_elast;  // = 100;
	uint s_tail_elast; // = 10;
	uint single_elast; // = 300;
	uint cjk_elast;    // = 101;
	uint ascii_elast;  // = 150;
	uint rigid_elast;  // = 10000;

	// Restrict compression
	float indent_min_shrink; // = 0.5;  // %cell
	float space_min_shrink;  // = 0.5;  // %cell
	float text_min_shrink;   // = 0.5;  // %part

	// Temporary storage for changed parts widths
	struct part_data
	{
		uint    old_width;
		uint    new_width;
		double  part_elast;
		float   delta;
	};
	size_t data_count;
	part_data* data;

	Shrinker()
	{
		data = NULL;

		// Default values;
		indent_elast = 100;
		space_elast  = 100;
		s_tail_elast = 50;
		single_elast = 300;
		cjk_elast    = 101;
		ascii_elast  = 150;
		rigid_elast  = 10000;
		//
		indent_min_shrink = 0.5; // %cell
		space_min_shrink = 0.5;  // %cell
		text_min_shrink = 0.5;   // %part
	};

	~Shrinker()
	{
		delete[] data;
	};

	void init(VConTextPart* parts, uint l, uint r, uint a_font_width)
	{
		if (data == NULL)
		{
			data_count = (r-l+1);
			data = new part_data[data_count];
		}
		#ifdef _DEBUG
		if (data_count != (r-l+1)) {
			_ASSERTE(data_count == (r-l+1));
		}
		#endif

		cell_width = a_font_width;
	};

	// Calculate value of the part
	float sum_k(float len1, float k1_base, float len2, float k2_base)
	{
		float k = 1 / ( (len1 / k1_base) + (len2 / k2_base) );
		return k;
	};

	void get_total(VConTextPart* parts, uint l, uint r, float& total_k, int& total_l)
	{
		_ASSERTE(data!=NULL);

		total_l = 0;
		double k_den = 0.0;
		for (int i = l; i <= r; i++)
		{
			// Empty or non-spacing part?
			if (!parts[i].Length || !parts[i].TotalWidth)
			{
				data[i-l].old_width = data[i-l].new_width = 0;
				data[i-l].part_elast = 0.0;
				continue;
			}
			data[i-l].old_width = data[i-l].new_width = parts[i].TotalWidth;

			// Restrict compression to some percentage using rigid_elast coefficient
			if ((i == 0) && (parts[i].Flags & TRF_TextSpacing))
				data[i-l].part_elast = sum_k(
						(indent_min_shrink*cell_width), rigid_elast,
						(parts[i].Length - indent_min_shrink)*cell_width, indent_elast
						);
			else if (parts[i].Flags & TRF_SizeFree)
				data[i-l].part_elast = sum_k(
						(space_min_shrink*cell_width), rigid_elast,
						(parts[i].Length - space_min_shrink)*cell_width, (i < r) ? space_elast : s_tail_elast
						);
			// Double-width (full-width, CJK, etc.)
			else if (parts[i].Flags & TRF_TextCJK)
				data[i-l].part_elast = sum_k(
					(text_min_shrink*parts[i].TotalWidth), rigid_elast,
					((1 - text_min_shrink)*parts[i].TotalWidth), cjk_elast
					);
			else
				data[i-l].part_elast = sum_k(
					(text_min_shrink*parts[i].TotalWidth), rigid_elast,
					((1 - text_min_shrink)*parts[i].TotalWidth), ascii_elast
					);

			k_den += (1.0 / data[i-l].part_elast);
			total_l += parts[i].TotalWidth;
		}
		_ASSERTE(total_l > 0 && k_den > 0);
		total_k = (k_den > 0.0) ? (1 / k_den) : 1;
		//printf("total: %i..%i: k=%.5f\n", l, r, total_k);
	};

	// returns `true` if there was critical compression ratio
	bool eval_compression(VConTextPart* parts, uint l, uint r, int a_req_len)
	{
		_ASSERTE(l<=r && a_req_len>0);

		if (l == r)
		{
			_ASSERTE(data!=NULL);
			data[0].old_width = parts[l].TotalWidth;
			data[0].new_width = a_req_len;
			return ((data[0].new_width * 2) < data[0].old_width);
		}

		bool bCritical = false;
		float total_k = 1.0; int total_l = 0; int delta;
		int req_len = a_req_len;

		get_total(parts, l, r, total_k, total_l);

		#ifdef _DEBUG
		if (!data || (data_count < (r-l+1))) {
			_ASSERTE(data && (data_count >= (r-l+1)));
		}
		#endif

		float F = total_k * (total_l - req_len);
		float add_half = (F > 0) ? 0.5 : -0.5;
		uint half_cell = (cell_width > 3) ? (cell_width/2) : 1;
		for (uint i = l; (i < r) && (req_len < total_l); i++)
		{
			if (!parts[i].TotalWidth)
			{
				_ASSERTE(parts[i].CharFlags[0] == TCF_WidthZero);
				continue;
			}
			data[i-l].delta = (F / data[i-l].part_elast);
			delta = floor(data[i-l].delta + add_half);
			//printf("#%i: float=%.3f delta=%u\n", i+1, d, delta);
			_ASSERTE((delta+half_cell-1) <= parts[i].TotalWidth);
			_ASSERTE(parts[i].TotalWidth > half_cell);
			if ((delta + half_cell) <= (parts[i].TotalWidth + 1))
			{
				data[i-l].new_width = (parts[i].TotalWidth - delta);
			}
			else
			{
				data[i-l].new_width = half_cell;
				delta = data[i-l].old_width - data[i-l].new_width;
				bCritical = true;
			}

			total_l -= delta;
		}
		// Let last part lasts to the end of dedicated space
		if (l < r)
		{
			data[r-l].delta = (F / data[r-l].part_elast);
			if (a_req_len < total_l)
			{
				delta = (total_l - a_req_len);
				if ((delta + half_cell) >= parts[r].TotalWidth)
				{
					bCritical = true;
					//_ASSERTE(delta < parts[r].TotalWidth);
				}
				data[r-l].new_width = (delta < parts[r].TotalWidth) ? (parts[r].TotalWidth - delta) : half_cell;
			}
		}

		return bCritical;
	};
};

// Shrink lengths of [part1..part2] (including)
// They must not exceed `right` X coordinate
void CVConLine::DistributeParts(uint part1, uint part2, uint right)
{
	if ((part1 > part2) || (part2 >= PartsCount))
	{
		_ASSERTE((part1 <= part2) && (part2 < PartsCount));
		return;
	}

	if (!gpSet->isCompressLongStrings)
	{
		// Do not shrink, just crop
		CropParts(part1, part2, right);
		return;
	}


	// If possible - shrink only last part
	// TRF_SizeFree - Spaces or horizontal frames. It does not matter, how many *real* characters we have to write
	// TODO: However, this may shift cells and "columns" would be not aligned
	if ((TextParts[part2].Flags & TRF_SizeFree)
		&& ((TextParts[part2].PositionX + MIN_SIZEFREE_WIDTH) <= right)
		)
		part1 = part2;

	if (right <= TextParts[part1].PositionX)
	{
		_ASSERTE(right > TextParts[part1].PositionX);
		return;
	}


	uint ReqWidth = (right - TextParts[part1].PositionX);
	uint FullWidth = (TextParts[part2].PositionX + TextParts[part2].TotalWidth - TextParts[part1].PositionX);

	// 1) ...
	// 2) ...
	// 3) ...

	Shrinker shrinker;
	shrinker.init(TextParts, part1, part2, FontWidth);
	shrinker.eval_compression(TextParts, part1, part2, right - TextParts[part1].PositionX);

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

		// Prepare loops
		TextCharType* pcf = part.CharFlags; // character flags (zero/free/normal/double)
		uint* pcw = part.CharWidth; // pointer to character width
		int ShrinkLeft = part.Length;
		uint NeedWidth = shrinker.data[k-part1].new_width;
		uint NewPartWidth = 0;
		for (uint c = 0; c < part.Length; c++, pcf++, pcw++)
		{
			DoShrink(*pcw, ShrinkLeft, NeedWidth, NewPartWidth);
		}
		part.TotalWidth = NewPartWidth;

		// Advance coord
		PosX += NewPartWidth;
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
		_ASSERTE((int)row >= 0 && (int)j >= 0);
		// Process dialogs in reverse order, to ensure tops (Z-order) dialogs would be first
		for (int i = mn_DialogsCount-1; i >= 0; i--)
		{
			if (mrc_Dialogs[i].Top <= (int)row && (int)row <= mrc_Dialogs[i].Bottom)
			{
				int border1 = mrc_Dialogs[i].Left;
				int border2 = mrc_Dialogs[i].Right;

				// Looking for a dialog edge, on the current row, nearest to the current X-coord (j)
				if ((int)j <= border1 && border1 < NextDialogX)
					NextDialogX = border1;
				else if (border2 < nMax && (int)j <= border2 && border2 < NextDialogX)
					NextDialogX = border2;

				// Looking for a dialog (not a Far Manager Panels), covering current coord
				// TODO: It would be nice to process Far's column separators too, but there is no nice API
				if (!(mn_DialogFlags[i] & (FR_LEFTPANEL|FR_RIGHTPANEL|FR_FULLPANEL|FR_VIEWEREDITOR)))
				{
					if ((border1 <= (int)j && (int)j <= border2)
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

		// Already marked as dialog VBorder?
		if ((ConAttrLine[j].Flags & CharAttr_DialogVBorder)
			// Frames (vertical parts, pseudographics)
			|| isCharBorderVertical(c)
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

uint CVConLine::GetNextPartX(const uint part)
{
	uint nextPartX = ((part+1) < PartsCount) ? TextParts[part + 1].CellPosX
		: (TextWidth * FontWidth);
	return nextPartX;
}
