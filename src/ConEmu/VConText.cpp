
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
#include "VConText.h"


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

// А это только "рамочные" символы, в которых есть любая (хотя бы частичная) вертикальная черта + стрелки/штриховки
bool isCharBorderVertical(wchar_t inChar)
{
	//if (inChar>=0x2502 && inChar<=0x25C4 && inChar!=0x2550)
	//2009-07-12 Для упрощения - зададим диапазон рамок за исключением горизонтальной линии
	//if (inChar==ucBoxSinglVert || inChar==0x2503 || inChar==0x2506 || inChar==0x2507
	//    || (inChar>=0x250A && inChar<=0x254B) || inChar==0x254E || inChar==0x254F
	//    || (inChar>=0x2551 && inChar<=0x25C5)) // По набору символов Arial Unicode MS
	TODO("ucBoxSinglHorz отсекать не нужно?");

	if ((inChar != ucBoxDblHorz && (inChar >= ucBoxSinglVert && inChar <= ucBoxDblVertHorz))
		|| (inChar == ucArrowUp || inChar == ucArrowDown))
		return true;
	else
		return false;
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

bool isCharNonSpacing(wchar_t inChar)
{
	// Здесь возвращаем те символы, которые нельзя рисовать вместе с обычными буквами.
	// Например, 0xFEFF на некоторых шрифтах вообще переключает GDI на какой-то левый шрифт O_O
	// Да и то, что они "не занимают места" в консоли некорректно. Даже если апостроф, по идее,
	// должен располагаться НАД буквой, рисовать его надо бы СЛЕВА от нее, т.к. он занимает
	// знакоместо в консоли!
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
	bool isRtl =
		(inChar == 0x05BE) || (inChar == 0x05C0) || (inChar == 0x05C3) || (inChar == 0x05C6) ||
		((inChar >= 0x05D0) && (inChar <= 0x05F4)) ||
		(inChar == 0x0608) || (inChar == 0x060B) || (inChar == 0x060D) ||
		((inChar >= 0x061B) && (inChar <= 0x064A)) ||
		((inChar >= 0x066D) && (inChar <= 0x066F)) ||
		((inChar >= 0x0671) && (inChar <= 0x06D5)) ||
		((inChar >= 0x06E5) && (inChar <= 0x06E6)) ||
		((inChar >= 0x06EE) && (inChar <= 0x06EF)) ||
		((inChar >= 0x06FA) && (inChar <= 0x0710)) ||
		((inChar >= 0x0712) && (inChar <= 0x072F)) ||
		((inChar >= 0x074D) && (inChar <= 0x07A5)) ||
		((inChar >= 0x07B1) && (inChar <= 0x07EA)) ||
		((inChar >= 0x07F4) && (inChar <= 0x07F5)) ||
		((inChar >= 0x07FA) && (inChar <= 0x0815)) ||
		(inChar == 0x081A) || (inChar == 0x0824) || (inChar == 0x0828) ||
		((inChar >= 0x0830) && (inChar <= 0x0858)) ||
		((inChar >= 0x085E) && (inChar <= 0x08AC)) ||
		(inChar == 0x200F) || (inChar == 0xFB1D) ||
		((inChar >= 0xFB1F) && (inChar <= 0xFB28)) ||
		((inChar >= 0xFB2A) && (inChar <= 0xFD3D)) ||
		((inChar >= 0xFD50) && (inChar <= 0xFDFC)) ||
		((inChar >= 0xFE70) && (inChar <= 0xFEFC))
		//((inChar>=0x10800)&&(inChar<=0x1091B))||
		//((inChar>=0x10920)&&(inChar<=0x10A00))||
		//((inChar>=0x10A10)&&(inChar<=0x10A33))||
		//((inChar>=0x10A40)&&(inChar<=0x10B35))||
		//((inChar>=0x10B40)&&(inChar<=0x10C48))||
		//((inChar>=0x1EE00)&&(inChar<=0x1EEBB))
		;
	return isRtl;
}
