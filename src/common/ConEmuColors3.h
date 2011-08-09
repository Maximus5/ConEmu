
#pragma once

class Far3Color
{
public:
	static int Max(int i1, int i2)
	{
		return (i1 > i2) ? i1 : i2;
	}

	static int GetStdIndex(COLORREF Color)
	{
		COLORREF StdPalette[16] = 
			{0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
			0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff};
		for (int i = 0; i < ARRAYSIZE(StdPalette); i++)
		{
			if (Color == StdPalette[i])
				return i;
		}
		return -1;
	}

	static COLORREF GetStdColor(int Index)
	{
		COLORREF StdPalette[16] = 
			{0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
			0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff};
		if (Index >= 0 && Index < 16)
			return StdPalette[Index];
		return 0;
	}
	
	static void Color2FgIndex(COLORREF Color, WORD& Con)
	{
		int Index = - 1;
		static int LastColor, LastIndex;
		if (LastColor == Color)
		{
			Index = LastIndex;
		}
		else if (Index == -1)
		{
			Index = GetStdIndex(Color);
		}

		if (Index == -1)
		{
			int B = (Color & 0xFF0000) >> 16;
			int G = (Color & 0xFF00) >> 8;
			int R = (Color & 0xFF);
			int nMax = Max(B,Max(R,G));
			
			Index =
				(((B+32) > nMax) ? 1 : 0) |
				(((G+32) > nMax) ? 2 : 0) |
				(((R+32) > nMax) ? 4 : 0);

			if (Index == 7)
			{
				if (nMax < 32)
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

	static void Color2BgIndex(COLORREF Color, BOOL Equal, WORD& Con)
	{
		int Index = -1;
		static int LastColor, LastIndex;
		if (LastColor == Color)
		{
			Index = LastIndex;
		}
		else if (Index == -1)
		{
			Index = GetStdIndex(Color);
		}

		if (Index == -1)
		{
			int B = (Color & 0xFF0000) >> 16;
			int G = (Color & 0xFF00) >> 8;
			int R = (Color & 0xFF);
			int nMax = Max(B,Max(R,G));

			Index =
				(((B+32) > nMax) ? 1 : 0) |
				(((G+32) > nMax) ? 2 : 0) |
				(((R+32) > nMax) ? 4 : 0);

			if (Index == 7)
			{
				if (nMax < 32)
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
		Color2 |= (WORD)(Color3.BackgroundColor & 0xF)<<4;
	}
	else
	{
		nBackColor = Color3.BackgroundColor & 0x00FFFFFF;
		Far3Color::Color2BgIndex(nBackColor, nBackColor==nForeColor, Color2);
	}
	
	return (BYTE)(Color2 & 0xFF);
}
