
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

#define HIDE_USE_EXCEPTION_INFO
#include "defines.h"
#include "MRect.h"

/* RECT/SMALL_RECT/COORD encapsulation */

SHORT MakeShort(i32 X)
{
	_ASSERTE(X==LOSHORT(X));
	return LOSHORT(X);
}

USHORT MakeUShort(u32 X)
{
	_ASSERTE(X==LOWORD(X));
	return LOWORD(X);
}

COORD MakeCoord(int X,int Y)
{
	// Console support only SHORT as coordinates
	COORD cr = {MakeShort(X), MakeShort(Y)};
	return cr;
}

POINT MakePoint(int X,int Y)
{
	POINT pt = {X,Y};
	return pt;
}

RECT MakeRect(int W,int H)
{
	RECT rc = {0,0,W,H};
	return rc;
}

RECT MakeRect(int X1, int Y1,int X2,int Y2)
{
	RECT rc = {X1,Y1,X2,Y2};
	return rc;
}

SMALL_RECT MakeSmallRect(int X1, int Y1, int X2, int Y2)
{
	_ASSERTE(X1==LOSHORT(X1) && Y1==LOSHORT(Y1) && X2==LOSHORT(X2) && Y2==LOSHORT(Y2));
	SMALL_RECT rc = {LOSHORT(X1), LOSHORT(Y1), LOSHORT(X2), LOSHORT(Y2)};
	return rc;
}

bool CoordInRect(const COORD& c, const RECT& r)
{
	return (c.X >= r.left && c.X <= r.right) && (c.Y >= r.top && c.Y <= r.bottom);
}

bool IntersectSmallRect(const RECT& rc1, const SMALL_RECT& rc2, LPRECT lprcDest /*= NULL*/)
{
	RECT frc2 = {rc2.Left, rc2.Top, rc2.Right, rc2.Bottom};
	RECT tmp = {};
	BOOL lb = IntersectRect(&tmp, &rc1, &frc2);
	if (lprcDest)
		*lprcDest = tmp;
	return (lb != FALSE);
}

bool PtDiffTest(int x1, int y1, int x2, int y2, UINT maxDx, UINT maxDy)
{
	int nX = abs(x1 - x2);

	if (nX > (int)maxDx)
		return false;

	int nY = abs(y1 - y2);

	if (nY > (int)maxDy)
		return false;

	return true;
}

bool PtDiffTest(POINT C, int aX, int aY, UINT D)
{
	return PtDiffTest(C.x, C.y, aX, aY, D, D);
}

int CoordCompare(const COORD& cr1, const COORD& cr2)
{
	if ((cr1.Y < cr2.Y) || ((cr1.Y == cr2.Y) && (cr1.X < cr2.X)))
		return -1;
	else if ((cr1.Y > cr2.Y) || ((cr1.Y == cr2.Y) && (cr1.X > cr2.X)))
		return 1;
	else
		return 0;
}

bool CoordEqual(const COORD& cr1, const COORD& cr2)
{
	return ((cr1.X == cr2.X) && (cr1.Y == cr2.Y));
}

bool operator ==(const COORD& cr1, const COORD& cr2)
{
	return CoordEqual(cr1, cr2);
}

bool operator !=(const COORD& cr1, const COORD& cr2)
{
	return !CoordEqual(cr1, cr2);
}
