
/*
Copyright (c) 2013-2017 Maximus5
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

// AddConAttr used to "mark" lines in "real console" for their identification
extern WORD AddConAttr[16];

// There bits must be left unchanged in "real console"
#define UNCHANGED_CONATTR (0x00FF | COMMON_LVB_LEADING_BYTE | COMMON_LVB_TRAILING_BYTE)

// GCC headers fix
#ifndef COMMON_LVB_UNDERSCORE
#define COMMON_LVB_GRID_HORIZONTAL 0x0400 // DBCS: Grid attribute: top horizontal.
#define COMMON_LVB_GRID_LVERTICAL  0x0800 // DBCS: Grid attribute: left vertical.
#define COMMON_LVB_GRID_RVERTICAL  0x1000 // DBCS: Grid attribute: right vertical.
#define COMMON_LVB_REVERSE_VIDEO   0x4000 // DBCS: Reverse fore/back ground attribute.
#define COMMON_LVB_UNDERSCORE      0x8000 // DBCS: Underscore.
#endif

#define CON_ATTR_0 COMMON_LVB_REVERSE_VIDEO   // 0x4000 // DBCS: Reverse fore/back ground attribute.
#define CON_ATTR_1 COMMON_LVB_GRID_HORIZONTAL // 0x0400 // DBCS: Grid attribute: top horizontal.
#define CON_ATTR_2 COMMON_LVB_GRID_LVERTICAL  // 0x0800 // DBCS: Grid attribute: left vertical.
#define CON_ATTR_4 COMMON_LVB_GRID_RVERTICAL  // 0x1000 // DBCS: Grid attribute: right vertical.
#define CON_ATTR_8 COMMON_LVB_UNDERSCORE      // 0x8000 // DBCS: Underscore.

#define CHANGED_CONATTR (CON_ATTR_0|CON_ATTR_1|CON_ATTR_2|CON_ATTR_4|CON_ATTR_8)

#define CON_ATTR_PART_0  (CON_ATTR_0|CON_ATTR_8|CON_ATTR_4)
#define CON_ATTR_PART_1  (CON_ATTR_1)
#define CON_ATTR_PART_2  (CON_ATTR_2)
#define CON_ATTR_PART_3  (CON_ATTR_2|CON_ATTR_1)
#define CON_ATTR_PART_4  (CON_ATTR_4)
#define CON_ATTR_PART_5  (CON_ATTR_4|CON_ATTR_1)
#define CON_ATTR_PART_6  (CON_ATTR_4|CON_ATTR_2)
#define CON_ATTR_PART_7  (CON_ATTR_4|CON_ATTR_2|CON_ATTR_1)
#define CON_ATTR_PART_8  (CON_ATTR_0|CON_ATTR_8|CON_ATTR_2)
#define CON_ATTR_PART_9  (CON_ATTR_8|CON_ATTR_1)
#define CON_ATTR_PART_10 (CON_ATTR_8|CON_ATTR_2)
#define CON_ATTR_PART_11 (CON_ATTR_8|CON_ATTR_2|CON_ATTR_1)
#define CON_ATTR_PART_12 (CON_ATTR_8|CON_ATTR_4)
#define CON_ATTR_PART_13 (CON_ATTR_8|CON_ATTR_4|CON_ATTR_1)
#define CON_ATTR_PART_14 (CON_ATTR_8|CON_ATTR_4|CON_ATTR_2)
#define CON_ATTR_PART_15 (CON_ATTR_8|CON_ATTR_4|CON_ATTR_2|CON_ATTR_1)

struct CEConsoleMark
{
	union
	{
		unsigned __int64 Raw;
		WORD CellAttr[4];
	};
	WORD RowId;
};

#define ROWID_USED_CELLS 4

WORD ReadConsoleRowId(HANDLE hConOut, SHORT nRow, CEConsoleMark* pMark = NULL);
bool WriteConsoleRowId(HANDLE hConOut, SHORT nRow, WORD RowId, CEConsoleMark* pMark = NULL);
bool FindConsoleRowId(HANDLE hConOut, const CEConsoleMark& Mark, SHORT nFromRow, SHORT* pnRow = NULL);
bool FindConsoleRowId(HANDLE hConOut, SHORT nFromRow, bool bUpWard, SHORT* pnRow = NULL, CEConsoleMark* pMark = NULL);
WORD GetRowIdFromAttrs(const WORD* pnAttrs4); // count == ROWID_USED_CELLS
