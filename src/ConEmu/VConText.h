
/*
Copyright (c) 2015-2016 Maximus5
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
#include "../common/RgnDetect.h"
#include "../common/wcwidth.h"

typedef DWORD TextPartFlags;
const TextPartFlags
	// Position (fixed for Far Manager frames, pseudographics, etc.)
	TRF_PosFixed         = 0x0001,
	TRF_PosRecommended   = 0x0002,
	TRF_SizeFree         = 0x0004,
	TRF_SizeFixed        = 0x0008,
	// Text properties
	TRF_TextNormal       = 0x0010,
	TRF_TextAlternative  = 0x0020, // Alternative font (frames or CJK)
	TRF_TextPseudograph  = 0x0040, // frames, borders, lines, etc (Far Manager especially)
	TRF_TextCJK          = 0x0080, // if not a VCTF_TextAlternative
	TRF_TextRTL          = 0x0100, // As is -- TODO?
	TRF_TextSpacing      = 0x0200, // Special part, it painted together with previous part, but(!)
								   // 1) width is calculated separately
								   // 2) it may NOT be combined with TRF_TextPseudograph
	TRF_TextSeparate     = 0x0400, // May not be combined with other parts
	TRF_TextProgress     = 0x0800, // Progress (boxes with 25%, 50%, etc)
	TRF_TextScroll       = 0x1000, // Far Manager scroll bars (vertical especially)
	TRF_ExpandGraphics   = 0x2000, // set for parts, where caller must draw additional
								   // pseudographic/space parts (like '═' or '─' or ' ')
								   // to avoid dashed frames or broken dialog borders,
								   // especially with proportional fonts
	TRF_CompareMask = (
		TRF_PosFixed|TRF_PosRecommended
		|TRF_TextAlternative|TRF_TextPseudograph
		|TRF_TextSeparate|TRF_TextProgress|TRF_TextScroll
		|TRF_ExpandGraphics
		),
	// TODO: Ligatures
	// End of flags
	TRF_None = 0;

// Recommended width type
enum TextCharType
{
	TCF_WidthZero = 0, // combining characters: acutes, umlauts, direction-marks, etc.
	TCF_WidthFree,     // spaces, horizontal lines (frames), etc. They may be shrinked freely
	TCF_WidthNormal,   // letters, numbers, etc. They have exact width, lesser space may harm visual representation
	TCF_WidthDouble,   // CJK. Double width hieroglyphs and other ideographics
	TCF_WidthLast,     // NOT A TYPE, ONLY FOR ARRAY INDEXING
};

class CVConLine;
class CRealConsole;

// Used as array[TextCharType::TCF_WidthLast]
struct VConTextPartWidth
{
	int  Count;    // Count of this type of symbols
	uint Width;    // Default width of unshrinked symbols
	uint MinWidth; // Some parts (continuous spaces, horz frames) may be hard shrinked
	uint ReqWidth; // For internal purposes
};

struct VConTextPart
{
	TextPartFlags Flags;
	uint Length;

	// Pointers
	const wchar_t* Chars;
	const CharAttr* Attrs;

	// Index in CVConLine.ConCharLine
	uint Index;
	// Cell in the RealConsole. It may differs from Index on DBCS systems
	uint Cell;
	// Helper, to ensure our text parts fit in the terminal window
	uint TotalWidth, MinWidth;
	// Final position in VCon
	uint PositionX;
	// Preferred position calculated by Cell*FontWidth
	uint CellPosX;

	// Chars distribution
	VConTextPartWidth AllWidths[TCF_WidthLast];

	// CharFlags is a pointer to buffer+idx from CVConLine
	TextCharType* CharFlags;

	// CharWidth is a pointer to buffer+idx from CVConLine
	// If gpSet->isMonospace then CharWidth calculated according to TCF_WidthXXX
	// But for proportional fonts we have to call GDI's TextExtentPoint
	uint* CharWidth;


	void Init(uint anIndex, uint anCell, CVConLine* pLine);
	void Done(uint anLen, uint FontWidth);
};

class CVConLine
{
public:
	CVConLine(CRealConsole* apRCon);
	~CVConLine();

public:
	// Methods
	void SetDialogs(int anDialogsCount, SMALL_RECT* apDialogs, DWORD* apnDialogFlags, DWORD anDialogAllFlags, const SMALL_RECT& arcUCharMap);
	bool ParseLine(bool abForce, uint anTextWidth, uint anFontWidth, uint anRow, wchar_t* apConCharLine, CharAttr* apConAttrLine, const wchar_t* const ConCharLine2, const CharAttr* const ConAttrLine2);
	void PolishParts(DWORD* pnXCoords);
	bool GetNextPart(uint& partIndex, VConTextPart*& part, VConTextPart*& nextPart);

protected:
	// Methods
	bool AllocateMemory();
	void ReleaseMemory();

	TextPartFlags isDialogBorderCoord(uint j);
	void DistributeParts(uint part1, uint part2, uint right);
	void DistributePartsFree(uint part1, uint part2, uint right);
	void DoShrink(uint& charWidth, int& ShrinkLeft, uint& NeedWidth, uint& TotalWidth);
	void ExpandPart(VConTextPart& part, uint EndX);
	bool HasFreeOverlaps(const uint part1, const uint part2, const uint right, VConTextPartWidth (&AllWidths)[TCF_WidthLast]);
	uint GetNextPartX(const uint part);

protected:
	friend struct VConTextPart;
	// Members
	CRealConsole* mp_RCon;

	// This corresponds to "dialogs" - regions framed with preudographics
	int mn_DialogsCount;
	const SMALL_RECT* mrc_Dialogs/*[MAX_DETECTED_DIALOGS]*/;
	const DWORD* mn_DialogFlags/*[MAX_DETECTED_DIALOGS]*/;
	DWORD mn_DialogAllFlags;
	// Far Manager's UnicodeCharMap plugin
	SMALL_RECT mrc_UCharMap;
	// Some other Far Manager's flags
	bool isFilePanel;

	// Drawing parameters below
	uint TextWidth;
	uint FontWidth;
	// Full redraw was requested
	bool isForce;

	// What we are parsing now
	uint row;
	const wchar_t* ConCharLine/*[TextWidth]*/;
	const CharAttr* ConAttrLine/*[TextWidth]*/;

	// Size of buffers
	uint MaxBufferSize;

	// Buffer to parse our line
	uint PartsCount;
	VConTextPart* TextParts/*[MaxBufferSize]*/;

	// CharFlags is a pointer to buffer+idx from CVConLine
	TextCharType* TempCharFlags/*[MaxBufferSize]*/;

	// CharWidth is a pointer to buffer+idx from CVConLine
	// If gpSet->isMonospace then CharWidth calculated according to TCF_WidthXXX
	// But for proportional fonts we have to call GDI's TextExtentPoint
	uint* TempCharWidth/*[MaxBufferSize]*/;

	// Just for information
	uint TotalLineWidth, MinLineWidth;

	// Pseudographics alignments
	bool isFixFrameCoord;
	bool NextDialog;
	int NextDialogX, CurDialogX1, CurDialogX2, CurDialogI; // !!! SIGNED !!!
	DWORD CurDialogFlags;
};

extern const wchar_t gszAnalogues[32];
//wchar_t getUnicodeAnalogue(uint inChar);

bool isCharAltFont(ucs32 inChar);
bool isCharPseudographics(ucs32 inChar);
bool isCharPseudoFree(ucs32 inChar);
bool isCharBorderVertical(ucs32 inChar);
bool isCharProgress(ucs32 inChar);
bool isCharScroll(ucs32 inChar);
bool isCharSeparate(ucs32 inChar);
bool isCharSpace(ucs32 inChar);
bool isCharSpaceSingle(ucs32 inChar);
bool isCharRTL(ucs32 inChar);
bool isCharCJK(ucs32 inChar);
bool isCharComining(ucs32 inChar);
bool isCharNonWord(ucs32 inChar);
bool isCharPunctuation(ucs32 inChar);
