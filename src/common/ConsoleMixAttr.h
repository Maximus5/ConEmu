
/*
Copyright (c) 2013-2015 Maximus5
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
bool FindConsoleRowId(HANDLE hConOut, SHORT nFromRow, SHORT* pnRow = NULL, CEConsoleMark* pMark = NULL);
WORD GetRowIdFromAttrs(const WORD* pnAttrs4); // count == ROWID_USED_CELLS
