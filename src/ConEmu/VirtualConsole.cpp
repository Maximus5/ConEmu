
#include "Header.h"


HANDLE VirtualConsole::hConOut()
{
	if (gSet.isConMan)
	{
		if(hConOut_)
			CloseHandle(hConOut_);

		hConOut_ = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
			0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	} else if (hConOut_==NULL) {
		hConOut_ = GetStdHandle(STD_OUTPUT_HANDLE);
	}
	return hConOut_;
}

VirtualConsole::VirtualConsole(/*HANDLE hConsoleOutput*/)
{
	hConOut_ = NULL;

	TextWidth = TextHeight = Width = Height = 0;
	hDC = NULL;
	hBitmap = NULL;
	hFont = NULL;
	ConChar = NULL;
	ConAttr = NULL;
	
    if (gSet.wndWidth)
        TextWidth = gSet.wndWidth;
    if (gSet.wndHeight)
        TextHeight = gSet.wndHeight;
	
	//BufferHeight = 0;
	//_tcscpy(gSet.Config, _T("Software\\ConEmu"));
	//
	//gSet.LogFont.lfHeight = 16;
	//gSet.LogFont.lfWidth = 0;
	//gSet.LogFont.lfEscapement = gSet.LogFont.lfOrientation = 0;
	//gSet.LogFont.lfWeight = FW_NORMAL;
	//gSet.LogFont.lfItalic = gSet.LogFont.lfUnderline = gSet.LogFont.lfStrikeOut = FALSE;
	//gSet.LogFont.lfCharSet = DEFAULT_CHARSET;
	//gSet.LogFont.lfOutPrecision = OUT_TT_PRECIS;
	//gSet.LogFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	//gSet.LogFont.lfQuality = ANTIALIASED_QUALITY;
	//gSet.LogFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	//_tcscpy(gSet.LogFont.lfFaceName, _T("Lucida Console"));

	//Registry RegConColors, RegConDef;
	//if (RegConColors.OpenKey(_T("Console"), KEY_READ))
	//{
	//	RegConDef.OpenKey(HKEY_USERS, _T(".DEFAULT\\Console"), KEY_READ);
	//
	//	TCHAR ColorName[] = _T("ColorTable00");
	//	for (uint i = 0x10; i--;)
	//	{
	//		ColorName[10] = i/10 + '0';
	//		ColorName[11] = i%10 + '0';
	//		if (!RegConColors.Load(ColorName, (DWORD *)&gSet.Colors[i]))
	//			RegConDef.Load(ColorName, (DWORD *)&gSet.Colors[i]);
	//	}
	//
	//	RegConDef.CloseKey();
	//	RegConColors.CloseKey();
	//}
}

VirtualConsole::~VirtualConsole()
{
	Free(true);
}

void VirtualConsole::Free(bool bFreeFont)
{
	if (hDC)
	{
		DeleteDC(hDC);
		hDC = NULL;
	}
	if (hBitmap)
	{
		DeleteObject(hBitmap);
		hBitmap = NULL;
	}
	if (bFreeFont && hFont)
	{
		DeleteObject(hFont);
		DeleteObject(hFont2);
		hFont2 = NULL;
		hFont = NULL;
	}
	if (ConChar)
	{
		delete[] ConChar;
		ConChar = NULL;
	}
	if (ConAttr)
	{
		delete[] ConAttr;
		ConAttr = NULL;
	}
}

bool VirtualConsole::InitFont(void)
{
	Free(true);
	hFont = CreateFontIndirectMy(&gSet.LogFont);
	return hFont != NULL;
}

bool VirtualConsole::InitDC(void)
{
	if (hFont)
		Free(false);
	else
		if (!InitFont())
			return false;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(hConOut(), &csbi))
		return false;

	IsForceUpdate = true;
	TextWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	TextHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	if ((int)TextWidth < csbi.dwSize.X)
		TextWidth = csbi.dwSize.X;

	ConChar = new TCHAR[TextWidth * TextHeight * 2];
	ConAttr = new WORD [TextWidth * TextHeight * 2];
	if (!ConChar || !ConAttr)
		return false;
	SelectionInfo.dwFlags = 0;

	hSelectedFont = NULL;
	const HDC hScreenDC = GetDC(HDCWND); //Maximus5 - был 0
	if (hDC = CreateCompatibleDC(0/*hScreenDC*/))
	{
		SelectObject(hDC, hFont);
		TEXTMETRIC tm;
		GetTextMetrics(hDC, &tm);
		if (gSet.isForceMonospace)
			//Maximus - у Arial'а например MaxWidth слишком большой
			gSet.LogFont.lfWidth = gSet.FontSizeX3 ? gSet.FontSizeX3 : tm.tmMaxCharWidth;
		else
			gSet.LogFont.lfWidth = tm.tmAveCharWidth;
		gSet.LogFont.lfHeight = tm.tmHeight;

		Width = TextWidth * gSet.LogFont.lfWidth;
		Height = TextHeight * gSet.LogFont.lfHeight;

		hBitmap = CreateCompatibleBitmap(hScreenDC, Width, Height);
		SelectObject(hDC, hBitmap);
	}
	ReleaseDC(0, hScreenDC);

	return hBitmap != NULL;
}

//#define isCharUnicode(inChar) (inChar <= 0x2668 ? 0 : 1)
bool isCharUnicode(WCHAR inChar)
{
	//if (inChar <= 0x2668)
	if (gSet.isFixFarBorders)
	{
		//if (! (inChar > 0x2500 && inChar < 0x251F))
		if ( !(inChar > 0x2013 && inChar < 0x266B) )
			/*if (inChar != 0x2550 && inChar != 0x2502 && inChar != 0x2551 && inChar != 0x007D &&
			inChar != 0x25BC && inChar != 0x2593 && inChar != 0x2591 && inChar != 0x25B2 &&
			inChar != 0x2562 && inChar != 0x255F && inChar != 0x255A && inChar != 0x255D &&
			inChar != 0x2554 && inChar != 0x2557 && inChar != 0x2500 && inChar != 0x2534 && inChar != 0x2564) // 0x2520*/
			return false;
		else
			return true;
	}
	else
	{
		if (inChar < 0x01F1 || inChar > 0x0400 && inChar < 0x045F || inChar > 0x2012 && inChar < 0x203D || /*? - not sure that optimal*/ inChar > 0x2019 && inChar < 0x2303 || inChar > 0x24FF && inChar < 0x266C)
			return false;
		else
			return true;
	}
}

void BlitPictureTo(VirtualConsole *vc, int inX, int inY, int inWidth, int inHeight)
{
	BitBlt(vc->hDC, inX, inY, inWidth, inHeight, vc->hBgDc, inX, inY, SRCCOPY);
	if (vc->bgBmp.cx < (int)inWidth || vc->bgBmp.cy < (int)inHeight)
	{
		HBRUSH hBrush = CreateSolidBrush(gSet.Colors[0]); SelectObject(vc->hDC, hBrush);
		RECT rect = {vc->bgBmp.cx, inY, inWidth, inHeight};
		FillRect(vc->hDC, &rect, hBrush);

		rect.left = inX; rect.top = vc->bgBmp.cy; rect.right = vc->bgBmp.cx;
		FillRect(vc->hDC, &rect, hBrush);

		DeleteObject(hBrush);
	}
}

void VirtualConsole::SelectFont(HFONT hNew)
{
	if (hSelectedFont != hNew)
	{
		hSelectedFont = hNew;
		SelectObject(hDC, hNew);
	}
}

static bool CheckSelection(const CONSOLE_SELECTION_INFO& select, SHORT row, SHORT col)
{
	if ((select.dwFlags & CONSOLE_SELECTION_NOT_EMPTY) == 0)
		return false;
	if (row < select.srSelection.Top || row > select.srSelection.Bottom)
		return false;
	if (col < select.srSelection.Left || col > select.srSelection.Right)
		return false;
	return true;
}

#ifdef _DEBUG
class DcDebug {
public:
	DcDebug(HDC* ahDcVar, HDC* ahPaintDC) {
	    mb_Attached=FALSE; mh_OrigDc=NULL; mh_DcVar=NULL; mh_Dc=NULL;
		if (!ahDcVar || !ahPaintDC) return;
		mh_DcVar = ahDcVar;
		mh_OrigDc = *ahDcVar;
		mh_Dc = *ahPaintDC;
		*mh_DcVar = mh_Dc;
	};
	~DcDebug() {
		if (mb_Attached && mh_DcVar) {
			mb_Attached = FALSE;
			*mh_DcVar = mh_OrigDc;
		}
	};
protected:
	BOOL mb_Attached;
	HDC mh_OrigDc, mh_Dc;
	HDC* mh_DcVar;
};
#endif

bool VirtualConsole::Update(bool isForce, HDC *ahDc)
{
    #ifdef _DEBUG
	DcDebug dbg(&hDC, ahDc); // для отладки - рисуем сразу на канвасе окна
	#endif

	bool lRes = false;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(hConOut(), &csbi))
		return false;
	COORD winSize = { csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1 };
	if (winSize.X < csbi.dwSize.X)
		winSize.X = csbi.dwSize.X;
	csbi.dwCursorPosition.X -= csbi.srWindow.Left;
	csbi.dwCursorPosition.Y -= csbi.srWindow.Top;
	const bool isCursorValid =
		csbi.dwCursorPosition.X >= 0 && csbi.dwCursorPosition.Y >= 0 &&
		csbi.dwCursorPosition.X < winSize.X && csbi.dwCursorPosition.Y < winSize.Y;

	if (!ahDc && (isForce || !hDC || TextWidth != winSize.X || TextHeight != winSize.Y))
		InitDC();

	// use and reset additional force flag
	if (IsForceUpdate)
	{
		isForce = IsForceUpdate;
		IsForceUpdate = false;
	}
	if (ahDc)
		isForce = true;

	const bool drawImage = gSet.isShowBgImage && gSet.isBackgroundImageValid;
	const uint TextLen = TextWidth * TextHeight;
	COORD coord = {csbi.srWindow.Left, csbi.srWindow.Top};

	// start timer before ReadConsoleOutput* calls, they do take time
	i64 tick, tick2;
	if (ghOpWnd)
		QueryPerformanceCounter((LARGE_INTEGER*)&tick);

	// Get attributes (first) and text (second)
	// [Roman Kuzmin]
	// In FAR Manager source code this is mentioned as "fucked method". Yes, it is.
	// Functions ReadConsoleOutput* fail if requested data size exceeds their buffer;
	// MSDN says 64K is max but it does not say how much actually we can request now.
	// Experiments show that this limit is floating and it can be much less than 64K.
	// The solution below is not optimal when a user sets small font and large window,
	// but it is safe and practically optimal, because most of users set larger fonts
	// for large window and ReadConsoleOutput works OK. More optimal solution for all
	// cases is not that difficult to develop but it will be increased complexity and
	// overhead often for nothing, not sure that we really should use it.
	DWORD nbActuallyRead;
	if (!ReadConsoleOutputAttribute(hConOut(), ConAttr, TextLen, coord, &nbActuallyRead) ||
		!ReadConsoleOutputCharacter(hConOut(), ConChar, TextLen, coord, &nbActuallyRead))
	{
		WORD* ConAttrNow = ConAttr;
		TCHAR* ConCharNow = ConChar;
		for(int y = 0; y < (int)TextHeight; ++y)
		{
			ReadConsoleOutputAttribute(hConOut(), ConAttrNow, TextWidth, coord, &nbActuallyRead);
			ReadConsoleOutputCharacter(hConOut(), ConCharNow, TextWidth, coord, &nbActuallyRead);
			ConAttrNow += TextWidth;
			ConCharNow += TextWidth;
			++coord.Y;
		}
	}

	// get cursor info
	CONSOLE_CURSOR_INFO	cinf;
	GetConsoleCursorInfo(hConOut(), &cinf);

	// get selection info in buffer mode
	CONSOLE_SELECTION_INFO select1, select2;
	bool doSelect = gSet.BufferHeight > 0;
	if (doSelect)
	{
		select1 = SelectionInfo;
		GetConsoleSelectionInfo(&select2);
		select2.srSelection.Top -= csbi.srWindow.Top;
		select2.srSelection.Bottom -= csbi.srWindow.Top;
		SelectionInfo = select2;
		doSelect = (select1.dwFlags & CONSOLE_SELECTION_NOT_EMPTY) || (select2.dwFlags & CONSOLE_SELECTION_NOT_EMPTY);
		if (doSelect)
		{
			if (select1.dwFlags == select2.dwFlags &&
				select1.srSelection.Top == select2.srSelection.Top &&
				select1.srSelection.Left == select2.srSelection.Left &&
				select1.srSelection.Right == select2.srSelection.Right &&
				select1.srSelection.Bottom == select2.srSelection.Bottom)
			{
				doSelect = false;
			}
		}
	}

	//------------------------------------------------------------------------
	///| Drawing text (if there were changes in console) |////////////////////
	//------------------------------------------------------------------------
	bool updateText, updateCursor;
	if (isForce)
	{
		updateText = updateCursor = true;
	}
	else
	{
		// Do we have to update changed text?
		updateText = doSelect || memcmp(ConChar + TextLen, ConChar, TextLen * sizeof(TCHAR)) || memcmp(ConAttr + TextLen, ConAttr, TextLen * 2);
		
		// Do we have to update text under changed cursor?
		// Important: check both 'cinf.bVisible' and 'Cursor.isVisible',
		// because console may have cursor hidden and its position changed -
		// in this case last visible cursor remains shown at its old place.
		// Also, don't check foreground window here for the same reasons.
		// If position is the same then check the cursor becomes hidden.
		if (Cursor.x != csbi.dwCursorPosition.X || Cursor.y != csbi.dwCursorPosition.Y)
			updateCursor = cinf.bVisible || Cursor.isVisible;
		else
			updateCursor = Cursor.isVisiblePrevFromInfo && !cinf.bVisible;
	}
	if (updateText || updateCursor)
	{
		lRes = true;

		BOOL lbCurFont2 = FALSE;

		// counters
		TCHAR* ConCharLine;
		WORD* ConAttrLine;
		int i, pos, row;
		if (updateText)
		{
			i = TextLen - TextWidth;
			pos = Height - gSet.LogFont.lfHeight;
			row = TextHeight - 1;
		}
		else
		{
			i = TextWidth * Cursor.y;
			pos = gSet.LogFont.lfHeight * Cursor.y;
			row = Cursor.y;
		}
		ConCharLine = ConChar + i;
		ConAttrLine = ConAttr + i;

		if (gSet.isForceMonospace || !drawImage)
			SetBkMode(hDC, OPAQUE);
		else
			SetBkMode(hDC, TRANSPARENT);

		// rows
		const bool skipNotChanged = !isForce && !gSet.isForceMonospace;
		for (; pos >= 0; ConCharLine -= TextWidth, ConAttrLine -= TextWidth, pos -= gSet.LogFont.lfHeight, --row)
		{
			// the line
			const WORD* const ConAttrLine2 = ConAttrLine + TextLen;
			const TCHAR* const ConCharLine2 = ConCharLine + TextLen;

			// skip not changed symbols except the old cursor or selection
			int j = 0, end = TextWidth;
			if (skipNotChanged)
			{
				// *) Skip not changed tail symbols.
				while(--end >= 0 && ConCharLine[end] == ConCharLine2[end] && ConAttrLine[end] == ConAttrLine2[end])
				{
					if (updateCursor && row == Cursor.y && end == Cursor.x)
						break;
					if (doSelect)
					{
						if (CheckSelection(select1, row, end))
							break;
						if (CheckSelection(select2, row, end))
							break;
					}
				}
				if (end < j)
					continue;
				++end;

				// *) Skip not changed head symbols.
				while(j < end && ConCharLine[j] == ConCharLine2[j] && ConAttrLine[j] == ConAttrLine2[j])
				{
					if (updateCursor && row == Cursor.y && j == Cursor.x)
						break;
					if (doSelect)
					{
						if (CheckSelection(select1, row, j))
							break;
						if (CheckSelection(select2, row, j))
							break;
					}
					++j;
				}
				if (j >= end)
					continue;
			}

			// *) Now draw as much as possible in a row even if some symbols are not changed.
			// More calls for the sake of fewer symbols is slower, e.g. in panel status lines.
			for (int j2=0; j < end; j = j2)
			{
				const WORD attr = ConAttrLine[j];
				WCHAR c = ConCharLine[j];
				const bool isUnicode = isCharUnicode(c/*ConCharLine[j]*/);

				WORD attrFore = attr & 0x0F;
				WORD attrBack = attr >> 4 & 0x0F;
				if (doSelect && CheckSelection(select2, row, j))
				{
					WORD tmp = attrFore;
					attrFore = attrBack;
					attrBack = tmp;
				}

				SetTextColor(hDC, gSet.Colors[attrFore]);

				if (gSet.isForceMonospace)
				{
					SetTextColor(hDC, gSet.Colors[attrFore]);
					SetBkColor(hDC, gSet.Colors[attrBack]);

					//for (j2 = j + 1; j2 < end && ConAttrLine[j2] == ConAttrLine[j2 - 1]; j2++);
					//TextOut(hDC, j * gSet.LogFont.lfWidth, pos, ConCharLine + j, j2 - j);
					j2 = j + 1;
					/// ** /WCHAR c = ConCharLine[j];

					if (gSet.isFixFarBorders) {
						if (!isUnicode && lbCurFont2) {
							SelectObject(hDC, hFont);
							lbCurFont2 = FALSE;
						} else if (isUnicode && !lbCurFont2) {
							SelectObject(hDC, hFont2);
							lbCurFont2 = TRUE;
						}
					}

					/*
					Maximus - вообще закомментарил. нахрена это было сделано?
					if (c == 0x20 || !c) {
						//c = 0x3000;//0x2003; -- А это что за изврат?
					} else if (c <= 0x7E && c >= 0x21) {
						// Мля, и нахрена поднимать код.таблицу для нижней половины ASCII символов?
						//c += 0xFF01 - 0x21;
					}
					*/

					RECT rect = {j * gSet.LogFont.lfWidth, pos, j2 * gSet.LogFont.lfWidth, pos + gSet.LogFont.lfHeight};
					if (! (drawImage && (attrBack) < 2))
						SetBkColor(hDC, gSet.Colors[attrBack]);
					else if (drawImage)
						BlitPictureTo(this, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);

					UINT nFlags = ETO_CLIPPED | ETO_OPAQUE;
					if (c != 0x20 && !isUnicode) {
						#ifdef _DEBUG
						if (c == L't')
							c = c;
						#endif
						ABC abc;
						//This function succeeds only with TrueType fonts
						if (GetCharABCWidths(hDC, c, c, &abc))
						{
							int nShift = 0;
							if (abc.abcA<0) {
								// иначе символ наверное налезет на предыдущий?
								nShift = -abc.abcA;
							} else if (abc.abcA<(((int)gSet.LogFont.lfWidth-(int)abc.abcB-1)/2)) {
								// символ I, i, и др. очень тонкие - рисуем посередине
								nShift = ((gSet.LogFont.lfWidth-abc.abcB)/2)-abc.abcA;
							}
							if (nShift>0) {
								ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect, L" ", 1, 0);
								rect.left += nShift;
								rect.right += nShift;
								//nFlags = ETO_OPAQUE;
							}
						}
					}

					ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect, &c, 1, 0);

					//TextOut(hDC, j * gSet.LogFont.lfWidth, pos, &c, 1);
					//ABC abc;
					//GetCharABCWidths(hDC, c, c, &abc); // зашибись, вообще не использовалось
					//if (abc.abcA<0 || abc.abcC<0)
					//	c = c;
					OUTLINETEXTMETRIC otm[2];
					ZeroMemory(otm, sizeof(otm));
					otm->otmSize = sizeof(*otm);
					GetOutlineTextMetrics(hDC, sizeof(otm), otm);

					/*WCHAR xchar = 0;

					switch (ConCharLine[j])
					{
					case 0x255F:
						xchar = 0x2500; break;
					case 0x2554:
					case 0x255A:
					case 0x2564:
					case 0x2566:
					case 0x2567:
					case 0x2569:
					case 0x2550:
						xchar = 0x2550; break;
					case 0x2557:
					case 0x255D:
					case 0x2562:
					case 0x2551:
						xchar = 0x0020; break;
					case 0x2591:
						xchar = 0x2591; break;
					case 0x2592:
						xchar = 0x2592; break;
					case 0x2593:
						xchar = 0x2593; break;
					}
					if (xchar)
					{
						RECT rect = {j * gSet.LogFont.lfWidth + gSet.LogFont.lfWidth/2, pos, j2 * gSet.LogFont.lfWidth, pos + gSet.LogFont.lfHeight};
						ExtTextOut(hDC, rect.left, rect.top, ETO_CLIPPED | ETO_OPAQUE, &rect, &xchar, 1, 0);

						//TextOut(hDC, j * gSet.LogFont.lfWidth + gSet.LogFont.lfWidth/2, pos, &xchar, 1);
					}*/
				}
				else if (!isUnicode)
				{
					j2 = j + 1;
					if (!doSelect)
						while(j2 < end && ConAttrLine[j2] == attr && !isCharUnicode(*(ConCharLine+j2)))
							++j2;
					if (gSet.isFixFarBorders)
						SelectFont(hFont);
				}
				else //UNICODE
				{
					j2 = j + 1;
					if (!doSelect)
					{
						if (!gSet.isFixFarBorders)
						{
							while(j2 < end && ConAttrLine[j2] == attr && isCharUnicode(*(ConCharLine+j2)))
								++j2;
						}
						else
						{
							while(j2 < end && ConAttrLine[j2] == attr && isCharUnicode(*(ConCharLine+j2)) && *(ConCharLine+j2) == *(ConCharLine+j2+1))
								++j2;
						}
					}
					if (gSet.isFixFarBorders)
						SelectFont(hFont2);
				}

				if (!gSet.isForceMonospace)
				{
					RECT rect = {j * gSet.LogFont.lfWidth, pos, j2 * gSet.LogFont.lfWidth, pos + gSet.LogFont.lfHeight};
					if (! (drawImage && (attrBack) < 2))
						SetBkColor(hDC, gSet.Colors[attrBack]);
					else if (drawImage)
						BlitPictureTo(this, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);

					if (gSet.LogFont.lfCharSet == OEM_CHARSET && !isUnicode)
					{
						char *tmp = new char[end+5];
						WideCharToMultiByte(CP_OEMCP, 0, ConCharLine + j, j2 - j, tmp, end + 4, 0, 0);
						ExtTextOutA(hDC, rect.left, rect.top, ETO_CLIPPED | ((drawImage && (attrBack) < 2) ? 0 : ETO_OPAQUE),
							&rect, tmp, j2 - j, 0);
						delete[] tmp;
					}
					else
					{
						ExtTextOut(hDC, rect.left, rect.top, ETO_CLIPPED | ((drawImage && (attrBack) < 2) ? 0 : ETO_OPAQUE), &rect, ConCharLine + j, j2 - j, 0);
					}
				}
			
				// stop if all is done
				if (!updateText)
					goto done;

				// skip next not changed symbols again
				if (skipNotChanged)
				{
					// skip the same except the old cursor
					while(j2 < end && ConCharLine[j2] == ConCharLine2[j2] && ConAttrLine[j2] == ConAttrLine2[j2])
					{
						if (updateCursor && row == Cursor.y && j2 == Cursor.x)
							break;
						if (doSelect)
						{
							if (CheckSelection(select1, row, j2))
								break;
							if (CheckSelection(select2, row, j2))
								break;
						}
						++j2;
					}
				}
			}
		}
done:

		if (lbCurFont2) {
			SelectObject(hDC, hFont2);
		}

		// now copy the data for future comparison
		if (updateText)
		{
			memcpy(ConChar + TextLen, ConChar, TextLen * sizeof(TCHAR));
			memcpy(ConAttr + TextLen, ConAttr, TextLen * 2);
		}

		if (ghOpWnd)
		{
			QueryPerformanceCounter((LARGE_INTEGER *)&tick2);
			wsprintf(temp, _T("%i"), (tick2-tick)/100);
			SetDlgItemText(ghOpWnd, tRender, temp);
		}
	}

	//------------------------------------------------------------------------
	///| Drawing cursor |/////////////////////////////////////////////////////
	//------------------------------------------------------------------------
	Cursor.isVisiblePrevFromInfo = cinf.bVisible;
	if ((lRes || Cursor.isVisible != Cursor.isVisiblePrev) && cinf.bVisible && isCursorValid)
	{
		lRes = true;
		if ((Cursor.x != csbi.dwCursorPosition.X || Cursor.y != csbi.dwCursorPosition.Y))
		{
			Cursor.isVisible = isMeForeground();
			cBlinkNext = 0;
		}

		int CurChar = csbi.dwCursorPosition.Y * TextWidth + csbi.dwCursorPosition.X;
		Cursor.ch[0] = ConChar[CurChar];
		Cursor.ch[1] = 0;
		Cursor.foreColor = gSet.Colors[ConAttr[CurChar] >> 4 & 0x0F];
		Cursor.foreColorNum = ConAttr[CurChar] >> 4 & 0x0F;
		Cursor.bgColor = gSet.Colors[ConAttr[CurChar] & 0x0F];
		Cursor.isVisiblePrev = Cursor.isVisible;
		Cursor.x = csbi.dwCursorPosition.X;
		Cursor.y = csbi.dwCursorPosition.Y;

		if (Cursor.isVisible)
		{
			if (gSet.isCursorColor)
			{
				SetTextColor(hDC, Cursor.foreColor);
				SetBkColor(hDC, Cursor.bgColor);
			}
			else
			{
				SetTextColor(hDC, Cursor.foreColor);
				SetBkColor(hDC, Cursor.foreColorNum < 5 ? gSet.Colors[15] : gSet.Colors[0]);
			}
		}
		else
		{
			if (drawImage)
				BlitPictureTo(this, Cursor.x * gSet.LogFont.lfWidth, Cursor.y * gSet.LogFont.lfHeight, gSet.LogFont.lfWidth, gSet.LogFont.lfHeight);

			SetTextColor(hDC, Cursor.bgColor);
			SetBkColor(hDC, Cursor.foreColor);
			cinf.dwSize = 99;
		}

		RECT rect;
		if (!gSet.isCursorV)
		{
			rect.left = Cursor.x * gSet.LogFont.lfWidth;
			rect.top = (Cursor.y+1) * gSet.LogFont.lfHeight - MulDiv(gSet.LogFont.lfHeight, cinf.dwSize, 100);
			rect.right = (Cursor.x+1) * gSet.LogFont.lfWidth;
			rect.bottom = (Cursor.y+1) * gSet.LogFont.lfHeight;
		}
		else
		{
			rect.left = Cursor.x * gSet.LogFont.lfWidth;
			rect.top = Cursor.y * gSet.LogFont.lfHeight;
			rect.right = Cursor.x * gSet.LogFont.lfWidth + klMax(1, MulDiv(gSet.LogFont.lfWidth, cinf.dwSize, 100) + (cinf.dwSize > 10 ? 1 : 0));
			rect.bottom = (Cursor.y+1) * gSet.LogFont.lfHeight;
		}

		if (gSet.LogFont.lfCharSet == OEM_CHARSET && !isCharUnicode(Cursor.ch[0]))
		{
			if (gSet.isFixFarBorders)
				SelectFont(hFont);

			char tmp[2];
			WideCharToMultiByte(CP_OEMCP, 0, Cursor.ch, 1, tmp, 1, 0, 0);
			ExtTextOutA(hDC, Cursor.x * gSet.LogFont.lfWidth, Cursor.y * gSet.LogFont.lfHeight,
				ETO_CLIPPED | ((drawImage && (Cursor.foreColorNum < 2) &&
				!Cursor.isVisible) ? 0 : ETO_OPAQUE),&rect, tmp, 1, 0);
		}
		else
		{
			if (gSet.isFixFarBorders && isCharUnicode(Cursor.ch[0]))
				SelectFont(hFont2);
			else
				SelectFont(hFont);

			ExtTextOut(hDC, Cursor.x * gSet.LogFont.lfWidth, Cursor.y * gSet.LogFont.lfHeight,
				ETO_CLIPPED | ((drawImage && (Cursor.foreColorNum < 2) &&
				!Cursor.isVisible) ? 0 : ETO_OPAQUE),&rect, Cursor.ch, 1, 0);
		}
	}
	else
	{
		// update cursor anyway to avoid redundant updates
		Cursor.x = csbi.dwCursorPosition.X;
		Cursor.y = csbi.dwCursorPosition.Y;
	}
	return lRes;
}