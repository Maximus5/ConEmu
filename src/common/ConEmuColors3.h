
/*
Copyright (c) 2009-2017 Maximus5
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

class Far3Color
{
public:
	static int Max(int i1, int i2)
	{
		return (i1 > i2) ? i1 : i2;
	}

	static const COLORREF* GetStdPalette()
	{
		static COLORREF StdPalette[16] =
			{0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
			0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff};
		return StdPalette;
	}

	static int GetStdIndex(COLORREF Color, const COLORREF* apPalette = NULL)
	{
		const COLORREF* StdPalette = apPalette ? apPalette : GetStdPalette();
		for (size_t i = 0; i < 16; i++)
		{
			if (Color == StdPalette[i])
				return (int)i;
		}
		return -1;
	}

	static COLORREF GetStdColor(int Index, const COLORREF* apPalette = NULL)
	{
		const COLORREF* StdPalette = apPalette ? apPalette : GetStdPalette();
		if (Index >= 0 && Index < 16)
			return StdPalette[Index];
		return 0;
	}

	static void Color2FgIndex(COLORREF Color, WORD& Con, const COLORREF* apPalette = NULL)
	{
		int Index = - 1;
		static COLORREF LastColor;
		static int LastIndex;
		if (LastColor == Color)
		{
			Index = LastIndex;
		}
		else if (Index == -1)
		{
			Index = GetStdIndex(Color, apPalette);
		}

		if (Index == -1)
		{
			int B = (Color & 0xFF0000) >> 16;
			int G = (Color & 0xFF00) >> 8;
			int R = (Color & 0xFF);
			int nMax = Max(B,Max(R,G));

			Index =
				(((B+32) > nMax) ? 1 : 0) | //-V112
				(((G+32) > nMax) ? 2 : 0) | //-V112
				(((R+32) > nMax) ? 4 : 0); //-V112

			if (Index == 7)
			{
				if (nMax < 32) //-V112
					Index = 0;
				else if (nMax < 160)
					Index = 8;
				else if (nMax > 200)
					Index = 15;
			}
			else if (nMax > 220)
			{
				Index |= 8;
			}

			LastColor = Color;
			LastIndex = Index;
		}

		Con |= Index;
	}

	static void Color2BgIndex(COLORREF Color, BOOL Equal, WORD& Con, const COLORREF* apPalette = NULL)
	{
		int Index = -1;
		static COLORREF LastColor;
		static int LastIndex;
		if (LastColor == Color)
		{
			Index = LastIndex;
		}
		else if (Index == -1)
		{
			Index = GetStdIndex(Color, apPalette);
		}

		if (Index == -1)
		{
			int B = (Color & 0xFF0000) >> 16;
			int G = (Color & 0xFF00) >> 8;
			int R = (Color & 0xFF);
			int nMax = Max(B,Max(R,G));

			Index =
				(((B+32) > nMax) ? 1 : 0) | //-V112
				(((G+32) > nMax) ? 2 : 0) | //-V112
				(((R+32) > nMax) ? 4 : 0); //-V112

			if (Index == 7)
			{
				if (nMax < 32) //-V112
					Index = 0;
				else if (nMax < 160)
					Index = 8;
				else if (nMax > 200)
					Index = 15;
			}
			else if (nMax > 220)
			{
				Index |= 8;
			}

			LastColor = Color;
			LastIndex = Index;
		}

		if (Index == Con)
		{
			if (Con & 8)
				Index ^= 8;
			else
				Con |= 8;
		}

		Con |= (Index<<4);
	}
};

__inline BYTE FarColor_3_2(const FarColor& Color3)
{
	WORD Color2 = 0;

	DWORD nForeColor, nBackColor;
	if (Color3.Flags & FCF_FG_4BIT)
	{
		nForeColor = -1;
		Color2 |= (WORD)(Color3.ForegroundColor & 0xF);
	}
	else
	{
		//n |= 0x07;
		nForeColor = Color3.ForegroundColor & 0x00FFFFFF;
		Far3Color::Color2FgIndex(nForeColor, Color2);
	}

	if (Color3.Flags & FCF_BG_4BIT)
	{
		WORD bk = (WORD)(Color3.BackgroundColor & 0xF);
		// Коррекция яркости, если подобранные индексы совпали
		if (Color2 == bk)
		{
			if (Color2 & 8)
				bk ^= 8;
			else
				Color2 |= 8;
		}
		Color2 |= bk<<4;
	}
	else
	{
		nBackColor = Color3.BackgroundColor & 0x00FFFFFF;
		Far3Color::Color2BgIndex(nBackColor, nBackColor==nForeColor, Color2);
	}

	return (BYTE)(Color2 & 0xFF);
}
