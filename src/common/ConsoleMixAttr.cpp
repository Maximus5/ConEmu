
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

#define HIDE_USE_EXCEPTION_INFO

// declare AddConAttr values
#define DEFINE_ADDCONATTR

#include <windows.h>
#include "Common.h"
#include "ConsoleMixAttr.h"

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

bool FindConsoleRowId(HANDLE hConOut, SHORT nFromRow, bool bUpWard, SHORT* pnRow/*=NULL*/, CEConsoleMark* pMark/*=NULL*/)
{
	CEConsoleMark RowId = {};

	// While searching downward we need to know the size of the buffer
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!bUpWard && !GetConsoleScreenBufferInfo(hConOut, &csbi))
		return false;

	for (SHORT nRow = nFromRow;
		(bUpWard && (nRow >= 0)) || (!bUpWard && nRow < csbi.dwSize.Y);
		nRow += (bUpWard ? -1 : +1))
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
