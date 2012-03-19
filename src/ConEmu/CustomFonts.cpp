#include "CustomFonts.h"
#include <fstream>
#include <sstream>
#include <string>

class BDFFont : public CustomFont
{
private:
	int m_Width, m_Height;

	HDC hDC;
	HBITMAP hBitmap; // for GDI rendering
	bool* bpPixels;	 // for manual rendering
	BOOL m_HasUnicode, m_HasBorders;

	void CreateBitmap()
	{
		HDC hDDC = GetDC(NULL);
		hDC = CreateCompatibleDC(hDDC);
		ReleaseDC(NULL, hDDC);

		struct MyBitmap
		{
			BITMAPINFO bmi;
			RGBQUAD white;
		};

		MyBitmap b = {0};
		b.bmi.bmiHeader.biSize        = sizeof(b.bmi.bmiHeader);
		b.bmi.bmiHeader.biWidth       = m_Width * 256;
		b.bmi.bmiHeader.biHeight      = -m_Height * 256;
		b.bmi.bmiHeader.biPlanes      = 1;
		b.bmi.bmiHeader.biBitCount    = 1;
		b.bmi.bmiHeader.biCompression = BI_RGB;
		RGBQUAD white = {0xFF,0xFF,0xFF};
		b.white = white;
		void* pvBits;
		hBitmap = CreateDIBSection(hDC, &b.bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
		SelectObject(hDC, hBitmap);
		// pixels = cast(COLOR*)pvBits;

		bpPixels = new bool[m_Width*256 * m_Height*256](); // TODO?
	}

public:
	virtual ~BDFFont()
	{
		if (hDC)
			DeleteDC(hDC);
		if (hBitmap)
			DeleteObject(hBitmap);
		if (bpPixels)
			delete[] bpPixels;
	}

	static BDFFont* Load(std::istream &f)
	{
		BDFFont* b = new BDFFont;
		std::string line;

		int iCharIndex = -1;
		int iXOffset, iYOffset;
		int iCharWidth, iCharHeight, iCharXOffset, iCharYOffset;

		while (f.good())
		{
			getline(f, line);
			std::istringstream iss(line);
			std::string word;
			getline(iss, word, ' ');
			if (word == "FONTBOUNDINGBOX")
			{
				getline(iss, word, ' '); b->m_Width = atoi(word.c_str());
				getline(iss, word, ' '); b->m_Height = atoi(word.c_str());
				getline(iss, word, ' '); iXOffset = atoi(word.c_str());
				getline(iss, word, ' '); iYOffset = atoi(word.c_str());
				b->CreateBitmap();
			}
			else
			if (word == "ENCODING")
			{
				getline(iss, word, ' '); iCharIndex = atoi(word.c_str());
				if (iCharIndex > 0xFFFF)
					iCharIndex = -1;
				if (iCharIndex > 0xFF)
					b->m_HasUnicode = TRUE;
				if (iCharIndex >= 0x2500 && iCharIndex < 0x25A0)
					b->m_HasBorders = TRUE;
			}
			else
			if (word == "BBX")
			{
				getline(iss, word, ' '); iCharWidth = atoi(word.c_str());
				getline(iss, word, ' '); iCharHeight = atoi(word.c_str());
				getline(iss, word, ' '); iCharXOffset = atoi(word.c_str());
				getline(iss, word, ' '); iCharYOffset = atoi(word.c_str());
			}
			else
			if (word == "BITMAP")
			{
				if (iCharIndex < 0)
					continue;

				int x = b->m_Width  * (iCharIndex % 256) + iCharXOffset + iXOffset;
				int y = b->m_Height * (iCharIndex / 256) + b->m_Height - iCharYOffset + iYOffset - iCharHeight;

				for (int iRow=0; iRow<iCharHeight; iRow++)
				{
					int by = y*b->m_Width*256;
					getline(f, line);
					for (size_t i = 0; i+1 < line.size(); i += 2)
					{
						int n;
						std::istringstream(line.substr(i, 2)) >> std::hex >> n;
						for (int bit=0; bit<8; bit++)
						{
							if (n & (0x80 >> bit))
							{
								int px = x+(i/2)*8+bit;
								SetPixel(b->hDC, px, y, 0xFFFFFF);
								b->bpPixels[by + px] = true;
							}
						}
					}
					y++;
				}
			}
		}

		return b;
	}

	static bool GetFamilyName(std::istream &f, std::string &familyName)
	{
		std::string s;
		static const std::string FAMILY_NAME("FAMILY_NAME \"");
		while (f.good())
		{
			getline(f, s);
			if (s.compare(0, FAMILY_NAME.size(), FAMILY_NAME)==0)
			{
				familyName = s.substr(FAMILY_NAME.size(), s.size()-1-FAMILY_NAME.size());
				return true;
			}
		}
		return false;
	}

	// ...

	virtual BOOL IsMonospace()
	{
		return TRUE;
	}

	virtual BOOL HasUnicode()
	{
		return m_HasUnicode;
	}

	virtual BOOL HasBorders()
	{
		return m_HasBorders;
	}

	virtual void GetBoundingBox(long *pX, long *pY)
	{
		*pX = m_Width;
		*pY = m_Height;
	}

	virtual void DrawText(HDC hDC, int X, int Y, LPCWSTR lpString, UINT cbCount)
	{
		for (; cbCount; cbCount--, lpString++)
		{
			wchar_t ch = *lpString;
			BitBlt(hDC, X, Y, m_Width, m_Height, this->hDC, (ch%256)*m_Width, (ch/256)*m_Height, 0x00E20746);
			X += m_Width;
		}
	}

	virtual void DrawText(COLORREF* pDstPixels, size_t iDstStride, COLORREF cFG, COLORREF cBG, LPCWSTR lpString, UINT cbCount)
	{
		size_t iSrcSlack = m_Width * 255;
		size_t iDstSlack = iDstStride - m_Width;
		COLORREF colors[2] = {cBG, cFG};
		for (; cbCount; cbCount--, lpString++)
		{
			wchar_t ch = *lpString;
			int fx = m_Width *(ch%256);
			int fy = m_Height*(ch/256);
			bool* pSrcPos = bpPixels + fx + m_Width*256*fy;
			COLORREF* pDstPos = pDstPixels;
			if (cBG == CLR_INVALID) // transparent
			{
				for (int y=0; y<m_Height; y++)
				{
					for (int x=0; x<m_Width; x++)
						if (*pSrcPos++)
							*pDstPos++ = cFG;
						else
							pDstPos++;
					pSrcPos += iSrcSlack;
					pDstPos += iDstSlack;
				}
			}
			else // opaque
			{
				for (int y=0; y<m_Height; y++)
				{
					for (int x=0; x<m_Width; x++)
						*pDstPos++ = colors[*pSrcPos++];
					pSrcPos += iSrcSlack;
					pDstPos += iDstSlack;
				}
			}
			pDstPixels += m_Width;
		}
	}
};

BOOL BDF_GetFamilyName(LPCTSTR lpszFilePath, wchar_t (&rsFamilyName)[LF_FACESIZE])
{
	std::ifstream f(lpszFilePath);
	if (!f.is_open())
		return FALSE;

	std::string familyName;
	if (!BDFFont::GetFamilyName(f, familyName))
		return FALSE;

	if (familyName.size() >= LF_FACESIZE)
		return FALSE;
	std::copy(familyName.begin(), familyName.end(), rsFamilyName);
	rsFamilyName[familyName.size()] = 0;
	return TRUE;
}

CustomFont* BDF_Load( LPCTSTR lpszFilePath )
{
	std::ifstream f(lpszFilePath);
	if (!f.is_open())
		return NULL;

	return BDFFont::Load(f);
}

CachedSolidBrush::~CachedSolidBrush()
{
	if (m_Brush != NULL)
		DeleteObject(m_Brush);
}

HBRUSH CachedSolidBrush::Get(COLORREF c)
{
	if (c != m_Color)
	{
		DeleteObject(m_Brush);
		m_Brush = CreateSolidBrush(c);
	}
	return m_Brush;
}

CEFONT CEDC::SelectObject(CEFONT font)
{
	CEFONT oldFont = m_Font;
	m_Font = font;

	if (font.iType == CEFONT_GDI)
	{
		oldFont = CEFONT((HFONT)::SelectObject(hDC, font.hFont));
		if (m_TextColor != CLR_INVALID)
			::SetTextColor(hDC, m_TextColor);
		if (m_BkColor != CLR_INVALID)
			::SetBkColor(hDC, m_BkColor);
		if (m_BkMode != -1)
			::SetBkMode(hDC, m_BkColor);
	}
	return oldFont;
}

void CEDC::SetTextColor(COLORREF color)
{
	if (m_TextColor != color && m_Font.iType!=CEFONT_CUSTOM)
		::SetTextColor(hDC, color);
	m_TextColor = color;
}

void CEDC::SetBkColor(COLORREF color)
{
	if (m_BkColor != color && m_Font.iType!=CEFONT_CUSTOM)
		::SetBkColor(hDC, color);
	m_BkColor = color;
}

void CEDC::SetBkMode(int iBkMode)
{
	if (m_BkMode != iBkMode && m_Font.iType!=CEFONT_CUSTOM)
		::SetBkColor(hDC, iBkMode);
	m_BkMode = iBkMode;
}

static COLORREF FlipChannels(COLORREF c)
{
	return (c >> 16) | (c & 0x00FF00) | (c << 16);
}

BOOL CEDC::ExtTextOut(int X, int Y, UINT fuOptions, const RECT *lprc, LPCWSTR lpString, UINT cbCount, const INT *lpDx)
{
	switch (m_Font.iType)
	{
	case CEFONT_GDI:
	case CEFONT_NONE:
		return ::ExtTextOut(hDC, X, Y, fuOptions, lprc, lpString, cbCount, lpDx);
	case CEFONT_CUSTOM:
		if (pPixels)
		{
			m_Font.pCustomFont->DrawText(pPixels + X + Y*iWidth, iWidth,
				FlipChannels(m_TextColor), fuOptions & ETO_OPAQUE ? FlipChannels(m_BkColor) : CLR_INVALID, lpString, cbCount);
		}
		else
		{
			if (fuOptions & ETO_OPAQUE)
			{
				//hOldBrush = ::SelectObject(hDC, m_BgBrush.Get(m_BkColor));
				LONG w, h;
				m_Font.pCustomFont->GetBoundingBox(&w, &h);
				RECT r = {X, Y, X+w*cbCount, Y+h};
				FillRect(hDC, &r, m_BgBrush.Get(m_BkColor));
			}

			HBRUSH hOldBrush = (HBRUSH)::SelectObject(hDC, m_FgBrush.Get(m_TextColor));

			m_Font.pCustomFont->DrawText(hDC, X, Y, lpString, cbCount);

			::SelectObject(hDC, hOldBrush);
		}

		return TRUE;
	default:
		_ASSERT(0);
		return FALSE;
	}
}

BOOL CEDC::ExtTextOutA(int X, int Y, UINT fuOptions, const RECT *lprc, LPCSTR lpString, UINT cbCount, const INT *lpDx)
{
	switch (m_Font.iType)
	{
	case CEFONT_GDI:
	case CEFONT_NONE:
		return ::ExtTextOutA(hDC, X, Y, fuOptions, lprc, lpString, cbCount, lpDx);
	case CEFONT_CUSTOM:
		{
			wchar_t* lpWString = (wchar_t*)alloca(cbCount * sizeof(wchar_t));
			for (UINT cb=0; cb<cbCount; cb++)
				lpWString[cb] = lpString[cb];  // WideCharToMultiByte?
			return ExtTextOut(X, Y, fuOptions, lprc, lpWString, cbCount, lpDx);
		}
	default:
		_ASSERT(0);
		return FALSE;
	}
}

BOOL CEDC::GetTextExtentPoint32(LPCTSTR ch, int c, LPSIZE sz)
{
	switch (m_Font.iType)
	{
	case CEFONT_GDI:
	case CEFONT_NONE:
		return ::GetTextExtentPoint32(hDC, ch, c, sz);
	case CEFONT_CUSTOM:
		m_Font.pCustomFont->GetBoundingBox(&sz->cx, &sz->cy);
		return TRUE;
	default:
		_ASSERT(0);
		return FALSE;
	}
}
