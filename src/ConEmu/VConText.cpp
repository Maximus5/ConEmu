
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
	bool isVert = ((inChar >= ucBoxSinglVert && inChar <= ucBoxSinglVertLeft)
		|| (inChar == ucBoxDblVertHorz)
		|| (inChar == ucArrowUp || inChar == ucArrowDown));
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
				|| (inChar == 0x3000))  // CJK Wide Space
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




void VConRange::Init(uint anIndex, CVConLine* pLine)
{
	Flags = TRF_None;
	Length = 1; // initially, we'll enlarge it later
	TotalWidth = MinWidth = PositionX = 0;
	Index = anIndex;
	Chars = pLine->ConCharLine + anIndex;
	Attrs = pLine->ConAttrLine + anIndex;
	CharFlags = pLine->TempCharFlags + anIndex;
	CharWidth = pLine->TempCharWidth + anIndex;
}

void VConRange::Done(uint anLen, uint FontWidth)
{
	_ASSERTE(anLen>0 && FontWidth>0);
	Length = anLen;
	TotalWidth = MinWidth = 0;
	bool MinCharWidth = ((Flags & TRF_TextSpacing) != TRF_None);
	uint FontWidth2 = 2*FontWidth;

	const wchar_t* pch = Chars;
	const CharAttr* pca = Attrs;
	TextCharFlags* pcf = CharFlags;
	uint* pcw = CharWidth;

	for (uint left = anLen; left--; pch++, pca++, pcf++, pcw++)
	{
		if (pca->Flags2 & (CharAttr2_NonSpacing|CharAttr2_Combining))
		{
			// zero-width
			*pcw = 0;
		}
		else if (pca->Flags & CharAttr_DoubleSpaced)
		{
			*pcw = FontWidth2;
			TotalWidth += FontWidth2;
			MinWidth += MinCharWidth ? 1 : FontWidth2;
		}
		else /*if (gpSet->isMonospace
				|| (gpSet->isFixFarBorders && isCharAltFont(ch))
				|| (gpSet->isEnhanceGraphics && isCharProgress(ch))
				)*/
		{
			// Caller must process fonts and set "real" character widths for proportinal fonts
			*pcw = FontWidth;
			TotalWidth += FontWidth;
			MinWidth += MinCharWidth ? 1 : FontWidth;
		}
	}
}


CVConLine::CVConLine(CRealConsole* apRCon)
	: mp_RCon(apRCon)
	, mn_DialogsCount(0)
	, mrc_Dialogs(NULL)
	, mn_DialogFlags(NULL)
	, mn_DialogAllFlags(0)
	, mrc_UCharMap()
	, TextWidth(0)
	, isForce(true)
	, ConCharLine(NULL)
	, ConAttrLine(NULL)
	, MaxBufferSize(0)
	, wcTempDraw(NULL)
	, cTempDraw(NULL)
	, PartsCount(0)
	, TextRanges(NULL)
	, TempCharFlags(NULL)
	, TempCharWidth(NULL)
	, TotalWidth(0)
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

bool CVConLine::ParseLine(bool abForce, uint anTextWidth, uint anFontWidth, wchar_t* apConCharLine, CharAttr* apConAttrLine, const wchar_t* const ConCharLine2, const CharAttr* const ConAttrLine2)
{
	const bool bNeedAlloc = (MaxBufferSize < anTextWidth);
	TextWidth = anTextWidth;
	FontWidth = anFontWidth;
	ConCharLine = apConCharLine;
	ConAttrLine = apConAttrLine;
	PartsCount = 0;

	if (bNeedAlloc && !AllocateMemory())
		return false;

	const bool bEnhanceGraphics = gpSet->isEnhanceGraphics;
	const bool bProportional = gpSet->isMonospace == 0;
	const bool bForceMonospace = gpSet->isMonospace == 2;
	const bool bUseAlternativeFont = gpSet->isFixFarBorders;
	const bool bFixFrameCoord = mp_RCon->isFar();

	for (uint j = 0; j < TextWidth;)
	{
		//TODO: HI/LO SURROGATES
		wchar_t wc = ConCharLine[j];
		const CharAttr attr = ConAttrLine[j];

		VConRange* p = TextRanges+(PartsCount++);

		uint j2 = j+1;

		p->Init(j, this);

		//TODO: Process Far Dialogs to justify rectangles and frames

		/* *** Now we split our text line into parts with characters one "family" *** */

		_ASSERTE(p->Flags == TRF_None);

		if (isCharSpace(wc))
		{
			p->Flags = TRF_TextSpacing;
			while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharSpace(ConCharLine[j2]))
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
			p->Flags = TRF_TextProgress;
			while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharProgress(ConCharLine[j2]))
				j2++;
		}
		else if (bEnhanceGraphics && isCharScroll(wc))
		{
			p->Flags = TRF_TextScroll;
			while ((j2 < TextWidth) && (ConAttrLine[j2] == attr) && isCharScroll(ConCharLine[j2]))
				j2++;
		}
		else if (isCharBorderVertical(wc))
		{
			p->Flags = TRF_PosFixed|TRF_TextPseudograph;
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
			p->Flags = TRF_TextPseudograph;
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
				&& !isCharSpace((wc2=ConCharLine[j2]))
				&& !isCharSeparate(wc2)
				&& !isCharProgress(wc2)
				&& !isCharScroll(wc2)
				&& !isCharPseudographics(wc2)
				&& !(bUseAlternativeFont && isCharAltFont(wc2))
				//TODO: RTL/LTR
				)
				j2++;
		}

		/* *** We shall prepare this token (default/initial widths must be set) *** */
		p->Done(j2-j, FontWidth);
		j = j2;
	}

	return true;
}

void CVConLine::PolishParts(DWORD* pnXCoords)
{
	_ASSERTE(TextRanges && PartsCount);

	//TODO: We may combine parts here
	//TODO: Set part->Flags to TRF_None to skip it
	//TODO: Distribute spaces, merge parts, shrink meaningless if required...

	/* Fill all X coordinates */
	uint PosX = 0, CharIdx = 0;
	TotalWidth = MinWidth = 0;
	for (uint k = 0; k < PartsCount; k++)
	{
		VConRange& part = TextRanges[k];
		part.PositionX = PosX;
		TotalWidth += part.TotalWidth;
		MinWidth += part.MinWidth;

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

	return;
}

bool CVConLine::GetNextPart(uint& partIndex, VConRange*& part, VConRange*& nextPart)
{
	_ASSERTE(TextRanges && PartsCount);
	if (!TextRanges || (partIndex >= PartsCount))
		return false;
	part = TextRanges+partIndex;
	nextPart = ((partIndex+1) < PartsCount) ? (part+1) : NULL;
	partIndex++;
	return true;
}

bool CVConLine::AllocateMemory()
{
	ReleaseMemory();

	if (!(wcTempDraw = (wchar_t*)malloc(TextWidth*sizeof(*wcTempDraw))))
		return false;
	if (!(cTempDraw = (char*)malloc(TextWidth*sizeof(*cTempDraw))))
		return false;
	if (!(TextRanges = (VConRange*)malloc(TextWidth*sizeof(*TextRanges))))
		return false;
	if (!(TempCharFlags = (TextCharFlags*)malloc(TextWidth*sizeof(*TempCharFlags))))
		return false;
	if (!(TempCharWidth = (uint*)malloc(TextWidth*sizeof(*TempCharWidth))))
		return false;

	return true;
}

void CVConLine::ReleaseMemory()
{
	MaxBufferSize = 0;
	SafeFree(wcTempDraw);
	SafeFree(cTempDraw);
	SafeFree(TextRanges);
	SafeFree(TempCharFlags);
	SafeFree(TempCharWidth);
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

	bool bFixFrameCoord = mp_RCon->isFar();

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
			if (bProportional && bFixFrameCoord && (c==ucBoxDblHorz || c==ucBoxSinglHorz))
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
			if (j && bFixFrameCoord)
			{
				HEAPVAL;
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

				HEAPVAL;
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
					j2 = j + 1; HEAPVAL;

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
					bool bAlignCurledFrames = bFixFrameCoord && isFilePanel;

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
										|| (attr.Flags & CharAttr_DoubleSpaced)
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

	HEAPVAL;

	// Screen updated, reset until next "UpdateHighlights()" call
	m_HighlightInfo.m_Last.X = m_HighlightInfo.m_Last.Y = -1;
	ZeroStruct(m_HighlightInfo.mrc_LastRow);
	ZeroStruct(m_HighlightInfo.mrc_LastCol);
	ZeroStruct(m_HighlightInfo.mrc_LastHyperlink);

	return;
}
#endif
