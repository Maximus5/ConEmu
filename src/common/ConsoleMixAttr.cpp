
/*
Copyright (c) 2013-2014 Maximus5
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

// declare AddConAttr values
#define DEFINE_ADDCONATTR

#include <windows.h>
#include "common.hpp"
#include "ConsoleMixAttr.h"

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

WORD AddConAttr[16] =
{
	CON_ATTR_PART_0,
	CON_ATTR_PART_1,
	CON_ATTR_PART_2,
	CON_ATTR_PART_3,
	CON_ATTR_PART_4,
	CON_ATTR_PART_5,
	CON_ATTR_PART_6,
	CON_ATTR_PART_7,
	CON_ATTR_PART_8,
	CON_ATTR_PART_9,
	CON_ATTR_PART_10,
	CON_ATTR_PART_11,
	CON_ATTR_PART_12,
	CON_ATTR_PART_13,
	CON_ATTR_PART_14,
	CON_ATTR_PART_15
};

WORD GetRowIdFromAttrs(const WORD* pnAttrs4)
{
	if (!(pnAttrs4[0] & CHANGED_CONATTR) && !(pnAttrs4[1] & CHANGED_CONATTR) && !(pnAttrs4[2] & CHANGED_CONATTR) && !(pnAttrs4[3] & CHANGED_CONATTR))
		return 0;

	WORD RowId = 0;
	for (size_t i = 0; i < ROWID_USED_CELLS; i++)
	{
		RowId = (RowId<<4);
		switch (pnAttrs4[i] & CHANGED_CONATTR)
		{
		case CON_ATTR_PART_0:
			break;
		case CON_ATTR_PART_1:
			RowId |= 1; break;
		case CON_ATTR_PART_2:
			RowId |= 2; break;
		case CON_ATTR_PART_3:
			RowId |= 3; break;
		case CON_ATTR_PART_4:
			RowId |= 4; break;
		case CON_ATTR_PART_5:
			RowId |= 5; break;
		case CON_ATTR_PART_6:
			RowId |= 6; break;
		case CON_ATTR_PART_7:
			RowId |= 7; break;
		case CON_ATTR_PART_8:
			RowId |= 8; break;
		case CON_ATTR_PART_9:
			RowId |= 9; break;
		case CON_ATTR_PART_10:
			RowId |= 10; break;
		case CON_ATTR_PART_11:
			RowId |= 11; break;
		case CON_ATTR_PART_12:
			RowId |= 12; break;
		case CON_ATTR_PART_13:
			RowId |= 13; break;
		case CON_ATTR_PART_14:
			RowId |= 14; break;
		case CON_ATTR_PART_15:
			RowId |= 15; break;
		default:
			// That may happen after tab completion, if cursor was at position 2-4,
			// than only part of our mark was erased
			_ASSERTE((i > 0) && ((pnAttrs4[i] & CHANGED_CONATTR) == 0) && "Unknown mark!");
			return 0;
		}
	}

	return RowId;
}

WORD ReadConsoleRowId(HANDLE hConOut, SHORT nRow, CEConsoleMark* pMark/*=NULL*/)
{
	_ASSERTE(ROWID_USED_CELLS==4);
	WORD  nAttrs[ROWID_USED_CELLS];
	COORD crLeft = {0, nRow};
	DWORD nRead = 0;

	if (!ReadConsoleOutputAttribute(hConOut, nAttrs, ROWID_USED_CELLS, crLeft, &nRead) || (nRead != ROWID_USED_CELLS))
		return 0;

	WORD RowId = GetRowIdFromAttrs(nAttrs);
	if (!RowId)
		return 0;

	if (pMark)
	{
		_ASSERTEX(countof(pMark->CellAttr)==countof(nAttrs));
		memmove(pMark->CellAttr, nAttrs, sizeof(pMark->CellAttr));
		pMark->RowId = RowId;
	}

	return RowId;
}

bool WriteConsoleRowId(HANDLE hConOut, SHORT nRow, WORD RowId, CEConsoleMark* pMark/*=NULL*/)
{
	_ASSERTE(ROWID_USED_CELLS==4);
	WORD  nAttrs[ROWID_USED_CELLS], nNewAttrs[ROWID_USED_CELLS];
	COORD crLeft = {0, nRow};
	DWORD nRead = 0, nWrite = 0;

	if (!ReadConsoleOutputAttribute(hConOut, nAttrs, ROWID_USED_CELLS, crLeft, &nRead) || (nRead != ROWID_USED_CELLS))
		return false;

	if (RowId)
	{
		WORD RowIdPart = RowId;
		for (size_t i = 0, j = (ROWID_USED_CELLS-1); i < ROWID_USED_CELLS; i++, j--)
		{
			nNewAttrs[j] = (nAttrs[j] & ~CHANGED_CONATTR) | AddConAttr[RowIdPart&0xF];
			RowIdPart = (RowIdPart>>4);
		}
	}
	else
	{
		for (size_t i = 0; i < ROWID_USED_CELLS; i++)
		{
			nNewAttrs[i] = (nAttrs[i] & ~CHANGED_CONATTR);
		}
	}

	if (!WriteConsoleOutputAttribute(hConOut, nNewAttrs, ROWID_USED_CELLS, crLeft, &nWrite) || (nWrite != ROWID_USED_CELLS))
		return false;

	if (pMark)
	{
		_ASSERTEX(countof(pMark->CellAttr)==countof(nAttrs));
		memmove(pMark->CellAttr, nNewAttrs, sizeof(pMark->CellAttr));
		pMark->RowId = RowId;
	}

	return true;
}

bool FindConsoleRowId(HANDLE hConOut, const CEConsoleMark& Mark, SHORT nFromRow, SHORT* pnRow/*=NULL*/)
{
	if (!Mark.RowId)
		return false;

	CEConsoleMark Test = {};
	COORD crLeft = {0, nFromRow};
	DWORD nRead = 0;

	_ASSERTE(countof(Test.CellAttr)==ROWID_USED_CELLS);

	for (; crLeft.Y >= 0; crLeft.Y--)
	{
		if (!ReadConsoleOutputAttribute(hConOut, Test.CellAttr, ROWID_USED_CELLS, crLeft, &nRead) || (nRead != ROWID_USED_CELLS))
			return false;

		if (Test.Raw == Mark.Raw)
		{
			if (pnRow)
				*pnRow = crLeft.Y;
			return true;
		}
	}

	return false;
}

bool FindConsoleRowId(HANDLE hConOut, SHORT nFromRow, SHORT* pnRow/*=NULL*/, CEConsoleMark* pMark/*=NULL*/)
{
	CEConsoleMark RowId = {};

	for (SHORT nRow = nFromRow; nRow >= 0; nRow--)
	{
		if (ReadConsoleRowId(hConOut, nRow, &RowId))
		{
			if (pnRow)
				*pnRow = nRow;
			if (pMark)
				*pMark = RowId;
			return true;
		}
	}

	return false;
}
