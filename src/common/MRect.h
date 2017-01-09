
/*
Copyright (c) 2016-2017 Maximus5
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

#include <Windows.h>

SHORT MakeShort(i32 X);
USHORT MakeUShort(u32 X);

COORD MakeCoord(int X,int Y);
POINT MakePoint(int X,int Y);
RECT MakeRect(int W,int H);
RECT MakeRect(int X1, int Y1,int X2,int Y2);
SMALL_RECT MakeSmallRect(int X1, int Y1, int X2, int Y2);
bool CoordInRect(const COORD& c, const RECT& r);
bool IntersectSmallRect(const RECT& rc1, const SMALL_RECT& rc2, LPRECT lprcDest = NULL);

bool PtDiffTest(int x1, int y1, int x2, int y2, UINT maxDx, UINT maxDy);
bool PtDiffTest(POINT C, int aX, int aY, UINT D);

int  CoordCompare(const COORD& cr1, const COORD& cr2);
bool CoordEqual(const COORD& cr1, const COORD& cr2);
bool operator ==(const COORD& cr1, const COORD& cr2);
bool operator !=(const COORD& cr1, const COORD& cr2);
