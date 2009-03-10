#include "Header.h"

#define VCURSORWIDTH 2
#define HCURSORWIDTH 2

CVirtualConsole::CVirtualConsole(/*HANDLE hConsoleOutput*/)
{
	pVCon = this;
	hConOut_ = NULL;

	TextWidth = TextHeight = Width = Height = 0;
	hDC = NULL; hBitmap = NULL;
	hBgDc = NULL; hBgBitmap = NULL; bgBmp.cx = 0; bgBmp.cy = 0;
	hFont = NULL; hFont2 = NULL; hSelectedFont = NULL; hOldFont = NULL;
	ConChar = NULL; ConAttr = NULL; ConCharX = NULL; tmpOem = NULL; TextParts = NULL;
	memset(&SelectionInfo, 0, sizeof(SelectionInfo));
	IsForceUpdate = false;
	hBrush0 = NULL; hSelectedBrush = NULL; hOldBrush = NULL;
	isEditor = false;
	memset(&csbi, 0, sizeof(csbi));
	m_LastMaxReadCnt = 0; m_LastMaxReadTick = 0;

	nSpaceCount = 1000;
	Spaces = (TCHAR*)malloc(nSpaceCount*sizeof(TCHAR));
	for (UINT i=0; i<nSpaceCount; i++) Spaces[i]=L' ';

	hOldBrush = NULL;
	hOldFont = NULL;
	
    if (gSet.wndWidth)
        TextWidth = gSet.wndWidth;
    if (gSet.wndHeight)
        TextHeight = gSet.wndHeight;
	

    if (gSet.isShowBgImage)
        gSet.LoadImageFrom(gSet.pBgImage);
}

CVirtualConsole::~CVirtualConsole()
{
	Free(true);
	if (Spaces) free(Spaces); Spaces = NULL;  nSpaceCount = 0;
	pVCon = NULL; //??? так на всякий случай
}

void CVirtualConsole::Free(bool bFreeFont)
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
		free(ConChar);
		ConChar = NULL;
	}
	if (ConAttr)
	{
		free(ConAttr);
		ConAttr = NULL;
	}
	if (ConCharX)
	{
		free(ConCharX);
		ConCharX = NULL;
	}
	if (tmpOem)
	{
		free(tmpOem);
		tmpOem = NULL;
	}
	if (TextParts)
	{
		free(TextParts);
		TextParts = NULL;
	}
}

HANDLE CVirtualConsole::hConOut()
{
	if (gSet.isUpdConHandle)
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

bool CVirtualConsole::InitFont(void)
{
	Free(true);
	hFont = CreateFontIndirectMy(&gSet.LogFont);
	return hFont != NULL;
}

bool CVirtualConsole::InitDC(BOOL abFull/*=TRUE*/)
{
	if (hFont)
		Free(false);
	else
		if (!InitFont())
			return false;
			
    if (abFull)
    {
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (!GetConsoleScreenBufferInfo(hConOut(), &csbi))
			return false;

		IsForceUpdate = true;
		TextWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		TextHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
		if ((int)TextWidth < csbi.dwSize.X)
			TextWidth = csbi.dwSize.X;

		ConChar = (TCHAR*)calloc((TextWidth * TextHeight * 2), sizeof(*ConChar));
		ConAttr = (WORD*)calloc((TextWidth * TextHeight * 2), sizeof(*ConAttr));
		ConCharX = (DWORD*)calloc((TextWidth * TextHeight), sizeof(*ConCharX));
		tmpOem = (char*)calloc((TextWidth + 5), sizeof(*tmpOem));
		TextParts = (struct _TextParts*)calloc((TextWidth + 2), sizeof(*TextParts));
		if (!ConChar || !ConAttr || !ConCharX || !tmpOem || !TextParts)
			return false;
	}
	SelectionInfo.dwFlags = 0;

	hSelectedFont = NULL;
	const HDC hScreenDC = GetDC(0);
	if (hDC = CreateCompatibleDC(hScreenDC))
	{
		SelectFont(hFont);
		TEXTMETRIC tm;
		GetTextMetrics(hDC, &tm);
		if (gSet.isForceMonospace)
			//Maximus - у Arial'а например MaxWidth слишком большой
			gSet.LogFont.lfWidth = gSet.FontSizeX3 ? gSet.FontSizeX3 : tm.tmMaxCharWidth;
		else
			gSet.LogFont.lfWidth = tm.tmAveCharWidth;
		gSet.LogFont.lfHeight = tm.tmHeight;

		if (abFull)
		{
			if (ghOpWnd) // устанавливать только при листании шрифта в настройке
				gSet.UpdateTTF ( (tm.tmMaxCharWidth - tm.tmAveCharWidth)>2 );

			Width = TextWidth * gSet.LogFont.lfWidth;
			Height = TextHeight * gSet.LogFont.lfHeight;

			hBitmap = CreateCompatibleBitmap(hScreenDC, Width, Height);
			SelectObject(hDC, hBitmap);
		}
	}
	ReleaseDC(0, hScreenDC);

	return hBitmap != NULL;
}

void CVirtualConsole::DumpConsole()
{
	OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = ghWnd;
	ofn.lpstrFilter = _T("ConEmu dumps (*.con)\0*.con\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = temp;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = _T("Dump console...");
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
			| OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	if (!GetSaveFileName(&ofn))
		return;
		
	HANDLE hFile = CreateFile(temp, GENERIC_WRITE, FILE_SHARE_READ,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		DisplayLastError(_T("Can't create file!"));
		return;
	}
	DWORD dw;
	LPCTSTR pszTitle = gConEmu.GetTitle();
	WriteFile(hFile, pszTitle, _tcslen(pszTitle)*sizeof(*pszTitle), &dw, NULL);
	swprintf(temp, _T("\r\nSize: %ix%i\r\n"), TextWidth, TextHeight);
	WriteFile(hFile, temp, _tcslen(temp)*sizeof(TCHAR), &dw, NULL);
	WriteFile(hFile, ConChar, TextWidth * TextHeight * 2, &dw, NULL);
	WriteFile(hFile, ConAttr, TextWidth * TextHeight * 2, &dw, NULL);
	CloseHandle(hFile);
}

HFONT CVirtualConsole::CreateFontIndirectMy(LOGFONT *inFont)
{
	memset(FontWidth, 0, sizeof(*FontWidth)*0x10000);
	//memset(Font2Width, 0, sizeof(*Font2Width)*0x10000);

    DeleteObject(pVCon->hFont2); pVCon->hFont2 = NULL;

    int width = gSet.FontSizeX2 ? gSet.FontSizeX2 : inFont->lfWidth;
    pVCon->hFont2 = CreateFont(abs(inFont->lfHeight), abs(width), 0, 0, FW_NORMAL,
        0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 0, gSet.LogFont2.lfFaceName);

    return CreateFontIndirect(inFont);
}

// Это символы рамок и др. спец. символы
//#define isCharBorder(inChar) (inChar>=0x2013 && inChar<=0x266B)
bool CVirtualConsole::isCharBorder(WCHAR inChar)
{
	if (inChar>=0x2013 && inChar<=0x266B)
		return true;
	else
		return false;
	//if (gSet.isFixFarBorders)
	//{
	//	//if (! (inChar > 0x2500 && inChar < 0x251F))
	//	if ( !(inChar > 0x2013/*En dash*/ && inChar < 0x266B/*Beamed Eighth Notes*/) /*&& inChar!=L'}'*/ )
	//		/*if (inChar != 0x2550 && inChar != 0x2502 && inChar != 0x2551 && inChar != 0x007D &&
	//		inChar != 0x25BC && inChar != 0x2593 && inChar != 0x2591 && inChar != 0x25B2 &&
	//		inChar != 0x2562 && inChar != 0x255F && inChar != 0x255A && inChar != 0x255D &&
	//		inChar != 0x2554 && inChar != 0x2557 && inChar != 0x2500 && inChar != 0x2534 && inChar != 0x2564) // 0x2520*/
	//		return false;
	//	else
	//		return true;
	//}
	//else
	//{
	//	if (inChar < 0x01F1 || inChar > 0x0400 && inChar < 0x045F || inChar > 0x2012 && inChar < 0x203D || /*? - not sure that optimal*/ inChar > 0x2019 && inChar < 0x2303 || inChar > 0x24FF && inChar < 0x266C)
	//		return false;
	//	else
	//		return true;
	//}
}

// А это только "рамочные" символы, в которых есть любая (хотя бы частичная) вертикальная черта + стрелки/штриховки
bool CVirtualConsole::isCharBorderVertical(WCHAR inChar)
{
	//if (inChar>=0x2502 && inChar<=0x25C4 && inChar!=0x2550)
	if (inChar==0x2502 || inChar==0x2503 || inChar==0x2506 || inChar==0x2507
		|| (inChar>=0x250A && inChar<=0x254B) || inChar==0x254E || inChar==0x254F
		|| (inChar>=0x2551 && inChar<=0x25C5)) // По набору символов Arial Unicode MS
		return true;
	else
		return false;
}

void CVirtualConsole::BlitPictureTo(int inX, int inY, int inWidth, int inHeight)
{
	// А вообще, имеет смысл отрисовывать?
	if (bgBmp.cx>inX && bgBmp.cy>inY)
		BitBlt(hDC, inX, inY, inWidth, inHeight, hBgDc, inX, inY, SRCCOPY);
	if (bgBmp.cx<(inX+inWidth) || bgBmp.cy<(inY+inHeight))
	{
		if (hBrush0==NULL) {
			hBrush0 = CreateSolidBrush(gSet.Colors[0]);
			SelectBrush(hBrush0);
		}

		RECT rect = {max(inX,bgBmp.cx), inY, inX+inWidth, inY+inHeight};
		if (!IsRectEmpty(&rect))
			FillRect(hDC, &rect, hBrush0);

		if (bgBmp.cx>inX) {
			rect.left = inX; rect.top = max(inY,bgBmp.cy); rect.right = bgBmp.cx;
			if (!IsRectEmpty(&rect))
				FillRect(hDC, &rect, hBrush0);
		}

		//DeleteObject(hBrush);
	}
}

void CVirtualConsole::SelectFont(HFONT hNew)
{
	if (!hNew) {
		if (hOldFont)
			SelectObject(hDC, hOldFont);
		hOldFont = NULL;
		hSelectedFont = NULL;
	} else
	if (hSelectedFont != hNew)
	{
		hSelectedFont = (HFONT)SelectObject(hDC, hNew);
		if (!hOldFont)
			hOldFont = hSelectedFont;
		hSelectedFont = hNew;
	}
}

void CVirtualConsole::SelectBrush(HBRUSH hNew)
{
	if (!hNew) {
		if (hOldBrush)
			SelectObject(hDC, hOldBrush);
		hOldBrush = NULL;
	} else
	if (hSelectedBrush != hNew)
	{
		hSelectedBrush = (HBRUSH)SelectObject(hDC, hNew);
		if (!hOldBrush)
			hOldBrush = hSelectedBrush;
		hSelectedBrush = hNew;
	}
}

bool CVirtualConsole::CheckSelection(const CONSOLE_SELECTION_INFO& select, SHORT row, SHORT col)
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

// Возвращает true, если процедура изменила отображаемый символ
bool CVirtualConsole::GetCharAttr(TCHAR ch, WORD atr, TCHAR& rch, BYTE& foreColorNum, BYTE& backColorNum)
{
	bool bChanged = false;
	foreColorNum = atr & 0x0F;
	backColorNum = atr >> 4 & 0x0F;
	rch = ch; // по умолчанию!
	if (isEditor && gSet.isVisualizer && ch==L' ' &&
		(backColorNum==gSet.nVizTab || backColorNum==gSet.nVizEOL || backColorNum==gSet.nVizEOF))
	{
		if (backColorNum==gSet.nVizTab)
			rch = gSet.cVizTab; else
		if (backColorNum==gSet.nVizEOL)
			rch = gSet.cVizEOL; else
		if (backColorNum==gSet.nVizEOF)
			rch = gSet.cVizEOF;
		backColorNum = gSet.nVizNormal;
		foreColorNum = gSet.nVizFore;
		bChanged = true;
	} else
	if (gSet.isExtendColors) {
		if (backColorNum==gSet.nExtendColor) {
			backColorNum = attrBackLast;
			foreColorNum += 0x10;
		} else {
			attrBackLast = backColorNum;
		}
	}
	return bChanged;
}

// Возвращает ширину символа, учитывает FixBorders
WORD CVirtualConsole::CharWidth(TCHAR ch)
{
	if (gSet.isForceMonospace)
		return gSet.LogFont.lfWidth;

	WORD nWidth = gSet.LogFont.lfWidth;
	bool isBorder = false; //, isVBorder = false;

	if (gSet.isFixFarBorders) {
		isBorder = isCharBorder(ch);
		//if (isBorder) {
		//	isVBorder = ch == 0x2551 /*Light Vertical*/ || ch == 0x2502 /*Double Vertical*/;
		//}
	}

	//if (isBorder) {
		//if (!Font2Width[ch]) {
		//	if (!isVBorder) {
		//		Font2Width[ch] = gSet.LogFont.lfWidth;
		//	} else {
		//		SelectFont(hFont2);
		//		SIZE sz;
		//		if (GetTextExtentPoint32(hDC, &ch, 1, &sz) && sz.cx)
		//			Font2Width[ch] = sz.cx;
		//		else
		//			Font2Width[ch] = gSet.LogFont2.lfWidth;
		//	}
		//}
		//nWidth = Font2Width[ch];
	//} else {
	if (!isBorder) {
		if (!FontWidth[ch]) {
			SelectFont(hFont);
			SIZE sz;
			if (GetTextExtentPoint32(hDC, &ch, 1, &sz) && sz.cx)
				FontWidth[ch] = sz.cx;
			else
				FontWidth[ch] = nWidth;
		}
		nWidth = FontWidth[ch];
	}
	if (!nWidth) nWidth = 1; // на всякий случай, чтобы деления на 0 не возникло
	return nWidth;
}

bool CVirtualConsole::CheckChangedTextAttr()
{
	textChanged = 0!=memcmp(ConChar + TextLen, ConChar, TextLen * sizeof(*ConChar));
	attrChanged = 0!=memcmp(ConAttr + TextLen, ConAttr, TextLen * sizeof(*ConAttr));
#ifdef _DEBUG
	COORD ch;
	if (textChanged) {
		for (UINT i=0,j=TextLen; i<TextLen; i++,j++) {
			if (ConChar[i] != ConChar[j]) {
				ch.Y = i % TextWidth;
				ch.X = i - TextWidth * ch.Y;
				break;
			}
		}
	}
	if (attrChanged) {
		for (UINT i=0,j=TextLen; i<TextLen; i++,j++) {
			if (ConAttr[i] != ConAttr[j]) {
				ch.Y = i % TextWidth;
				ch.X = i - TextWidth * ch.Y;
				break;
			}
		}
	}
#endif

	return textChanged || attrChanged;
}

bool CVirtualConsole::Update(bool isForce, HDC *ahDc)
{
    #ifdef _DEBUG
	DcDebug dbg(&hDC, ahDc); // для отладки - рисуем сразу на канвасе окна
	#endif

	MCHKHEAP
	bool lRes = false;
	
	if (!GetConsoleScreenBufferInfo(hConOut(), &csbi)) {
		DisplayLastError(_T("pVCon->Update:GetConsoleScreenBufferInfo"));
		return lRes;
	}

	// start timer before ReadConsoleOutput* calls, they do take time
	gSet.Performance(tPerfRead, FALSE);

	//if (gbNoDblBuffer) isForce = TRUE; // Debug, dblbuffer

	//------------------------------------------------------------------------
	///| Read console output and cursor info... |/////////////////////////////
	//------------------------------------------------------------------------
	UpdatePrepare(isForce, ahDc);
	
	gSet.Performance(tPerfRead, TRUE);

	gSet.Performance(tPerfRender, FALSE);

	//------------------------------------------------------------------------
	///| Drawing text (if there were changes in console) |////////////////////
	//------------------------------------------------------------------------
	bool updateText, updateCursor;
	if (isForce)
	{
		updateText = updateCursor = attrChanged = textChanged = true;
	}
	else
	{
		// Do we have to update changed text?
		updateText = doSelect || CheckChangedTextAttr();
			//(textChanged = 0!=memcmp(ConChar + TextLen, ConChar, TextLen * sizeof(TCHAR))) || 
			//(attrChanged = 0!=memcmp(ConAttr + TextLen, ConAttr, TextLen * 2));
		
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

		//------------------------------------------------------------------------
		///| Drawing modified text |//////////////////////////////////////////////
		//------------------------------------------------------------------------
		UpdateText(isForce, updateText, updateCursor);


		MCHKHEAP
		//------------------------------------------------------------------------
		///| Now, store data for further comparison |/////////////////////////////
		//------------------------------------------------------------------------
		if (updateText)
		{
			memcpy(ConChar + TextLen, ConChar, TextLen * sizeof(TCHAR));
			memcpy(ConAttr + TextLen, ConAttr, TextLen * 2);
		}

		MCHKHEAP
		gSet.Performance(tPerfRender, TRUE);
	}

	MCHKHEAP
	//------------------------------------------------------------------------
	///| Drawing cursor |/////////////////////////////////////////////////////
	//------------------------------------------------------------------------
	UpdateCursor(lRes);


	/* ***************************************** */
	/*       Finalization, release objects       */
	/* ***************************************** */
	SelectBrush(NULL);
	if (hBrush0) {
		DeleteObject(hBrush0); hBrush0=NULL;
	}
	SelectFont(NULL);
	MCHKHEAP
	return lRes;
}

void CVirtualConsole::UpdatePrepare(bool isForce, HDC *ahDc)
{
	attrBackLast = 0;
	isEditor = gConEmu.isEditor();
	isFilePanel = gConEmu.isFilePanel(true);

	winSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1; winSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	if (winSize.X < csbi.dwSize.X)
		winSize.X = csbi.dwSize.X;
	csbi.dwCursorPosition.X -= csbi.srWindow.Left;
	csbi.dwCursorPosition.Y -= csbi.srWindow.Top;
	isCursorValid =
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

	drawImage = gSet.isShowBgImage && gSet.isBackgroundImageValid;
	TextLen = TextWidth * TextHeight;
	coord.X = csbi.srWindow.Left; coord.Y = csbi.srWindow.Top;

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
	GetConsoleCursorInfo(hConOut(), &cinf);

	// get selection info in buffer mode
	
	doSelect = gConEmu.BufferHeight > 0;
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
}

enum CVirtualConsole::_PartType CVirtualConsole::GetCharType(TCHAR ch)
{
	enum _PartType cType = pNull;

	if (ch == L' ')
		cType = pSpace;
	//else if (ch == L'_')
	//	cType = pUnderscore;
	else if (isCharBorder(ch)) {
		if (isCharBorderVertical(ch))
			cType = pVBorder;
		else
			cType = pBorder;
	}
	else if (isFilePanel && ch == L'}')
		cType = pRBracket;
	else
		cType = pText;

	return cType;
}

// row - 0-based
void CVirtualConsole::ParseLine(int row, TCHAR *ConCharLine, WORD *ConAttrLine)
{
	UINT idx = 0;
	struct _TextParts *pStart=TextParts, *pEnd=TextParts;
	enum _PartType cType1, cType2;
	UINT i1=0, i2=0;
	
	pEnd->partType = pNull; // сразу ограничим строку
	
	TCHAR ch1, ch2;
	BYTE af1, ab1, af2, ab2;
	DWORD pixels;
	while (i1<TextWidth)
	{
		GetCharAttr(ConCharLine[i1], ConAttrLine[i1], ch1, af1, ab1);
		cType1 = GetCharType(ch1);
		if (cType1 == pRBracket) {
			if (!(row>=2 && isCharBorderVertical(ConChar[TextWidth+i1]))
				&& (((UINT)row)<=(TextHeight-4)))
				cType1 = pText;
		}
		pixels = CharWidth(ch1);

		i2 = i1+1;
		// в режиме Force Monospace отрисовка идет по одному символу
		if (!gSet.isForceMonospace && i2 < TextWidth && 
			(cType1 != pVBorder && cType1 != pRBracket))
		{
			GetCharAttr(ConCharLine[i2], ConAttrLine[i2], ch2, af2, ab2);
			// Получить блок символов с аналогичными цветами
			while (i2 < TextWidth && af2 == af1 && ab2 == ab1) {
				// если символ отличается от первого

				cType2 = GetCharType(ch2);
				if ((ch2 = ConCharLine[i2]) != ch1) {
					if (cType2 == pRBracket) {
						if (!(row>=2 && isCharBorderVertical(ConChar[TextWidth+i2]))
							&& (((UINT)row)<=(TextHeight-4)))
							cType2 = pText;
					}

					// и он вообще из другой группы
					if (cType2 != cType1)
						break; // то завершаем поиск
				}
				pixels += CharWidth(ch2); // добавить ширину символа в пикселях
				i2++; // следующий символ
				GetCharAttr(ConCharLine[i2], ConAttrLine[i2], ch2, af2, ab2);
				if (cType2 == pRBracket) {
					if (!(row>=2 && isCharBorderVertical(ConChar[TextWidth+i2]))
						&& (((UINT)row)<=(TextHeight-4)))
						cType2 = pText;
				}
			}
		}

		// при разборе строки будем смотреть, если нашли pText,pSpace,pText то pSpace,pText добавить в первый pText
		if (cType1 == pText && (pEnd - pStart) >= 2) {
			if (pEnd[-1].partType == pSpace && pEnd[-2].partType == pText &&
				pEnd[-1].attrBack == ab1 && pEnd[-1].attrFore == af1 &&
				pEnd[-2].attrBack == ab1 && pEnd[-2].attrFore == af1
				)
			{	
				pEnd -= 2;
				pEnd->i2 = i2 - 1;
				pEnd->iwidth = i2 - pEnd->i1;
				pEnd->width += pEnd[1].width + pixels;
				pEnd ++;
				i1 = i2;
				continue;
			}
		}
		pEnd->i1 = i1; pEnd->i2 = i2 - 1; // конец "включая"
		pEnd->partType = cType1;
		pEnd->attrBack = ab1; pEnd->attrFore = af1;
		pEnd->iwidth = i2 - i1;
		pEnd->width = pixels;
		if (gSet.isForceMonospace ||
			(gSet.isTTF && (cType1 == pVBorder || cType1 == pRBracket)))
		{
			pEnd->x1 = i1 * gSet.LogFont.lfWidth;
		} else {
			pEnd->x1 = -1;
		}

		pEnd ++; // блоков не может быть больше количества символов в строке, так что с размерностью все ОК
		i1 = i2;
	}
	// пока поставим конец блоков, потом, если ширины не хватит - добавим pDummy
	pEnd->partType = pNull;
}

void CVirtualConsole::UpdateText(bool isForce, bool updateText, bool updateCursor)
{
	SelectFont(hFont);

	// pointers
	TCHAR* ConCharLine;
	WORD* ConAttrLine;
	DWORD* ConCharXLine;
	// counters
	int i, pos, row;
	if (updateText)
	{
		i = TextLen - TextWidth; // TextLen = TextWidth/*80*/ * TextHeight/*25*/;
		pos = Height - gSet.LogFont.lfHeight; // Height = TextHeight * gSet.LogFont.lfHeight;
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
	ConCharXLine = ConCharX + i;
	

	if (/*gSet.isForceMonospace ||*/ !drawImage)
		SetBkMode(hDC, OPAQUE);
	else
		SetBkMode(hDC, TRANSPARENT);

	// rows
	//TODO: а зачем в isForceMonospace принудительно перерисовывать все?
	const bool skipNotChanged = !isForce /*&& !gSet.isForceMonospace*/;
	for (; pos >= 0; 
		ConCharLine -= TextWidth, ConAttrLine -= TextWidth, ConCharXLine -= TextWidth,
		pos -= gSet.LogFont.lfHeight, --row)
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
		int j2=j+1;
		for (; j < end; j = j2)
		{
			const WORD attr = ConAttrLine[j];
			WCHAR c = ConCharLine[j];
			BYTE attrFore, attrBack;
			bool isUnicode = isCharBorder(c/*ConCharLine[j]*/);
			bool lbS1 = false, lbS2 = false;
			int nS11 = 0, nS12 = 0, nS21 = 0, nS22 = 0;

			if (GetCharAttr(c, attr, c, attrFore, attrBack))
				isUnicode = true;

			MCHKHEAP
			// корректировка лидирующих пробелов и рамок
			if (gSet.isTTF && (c==0x2550 || c==0x2500)) // 'Box light horizontal' & 'Box double horizontal' - это всегда
			{
				lbS1 = true; nS11 = nS12 = j;
				while ((nS12 < end) && (ConCharLine[nS12+1] == c))
					nS12 ++;
				// Посчитать сколько этих же символов ПОСЛЕ текста
				if (nS12<end) {
					nS21 = nS12+1; // Это должен быть НЕ c 
					// ищем первый "рамочный" символ
					while ((nS21<end) && (ConCharLine[nS21] != c) && !isCharBorder(ConCharLine[nS21]))
						nS21 ++;
					if (nS21<end && ConCharLine[nS21]==c) {
						lbS2 = true; nS22 = nS21;
						while ((nS22 < end) && (ConCharLine[nS22+1] == c))
							nS22 ++;
					}
				}
			} MCHKHEAP
			// а вот для пробелов - когда их более одного
			/*else if (c==L' ' && j<end && ConCharLine[j+1]==L' ')
			{
				lbS1 = true; nS11 = nS12 = j;
				while ((nS12 < end) && (ConCharLine[nS12+1] == c))
					nS12 ++;
			}*/

			// Возможно это тоже нужно вынести в GetCharAttr?
			if (doSelect && CheckSelection(select2, row, j))
			{
				WORD tmp = attrFore;
				attrFore = attrBack;
				attrBack = tmp;
			}

			SetTextColor(hDC, gSet.Colors[attrFore]);

			// корректировка положения вертикальной рамки
			if (gSet.isTTF && j)
			{
				MCHKHEAP
				DWORD nPrevX = ConCharXLine[j-1];
				if (isCharBorderVertical(c)) {
					ConCharXLine[j-1] = (j-1) * gSet.LogFont.lfWidth;
				} else if (isFilePanel && c==L'}') {
					// Символом } фар помечает имена файлов вылезшие за пределы колонки...
					// Если сверху или снизу на той же позиции рамки (или тот же '}')
					// корректируем позицию
					if ((row>=2 && isCharBorderVertical(ConChar[TextWidth+j]))
						&& (((UINT)row)<=(TextHeight-4)))
					 ConCharXLine[j-1] = (j-1) * gSet.LogFont.lfWidth;
					//row = TextHeight - 1;
				} MCHKHEAP
				if (nPrevX < ConCharXLine[j-1]) {
					// Требуется зарисовать пропущенную область. пробелами что-ли?

					RECT rect;
					rect.left = nPrevX;
					rect.top = pos;
					rect.right = ConCharXLine[j-1];
					rect.bottom = rect.top + gSet.LogFont.lfHeight;

					if (gbNoDblBuffer) GdiFlush();
					if (! (drawImage && (attrBack) < 2)) {
						SetBkColor(hDC, gSet.Colors[attrBack]);
						int nCnt = ((rect.right - rect.left) / CharWidth(L' '))+1;

						UINT nFlags = ETO_CLIPPED; // || ETO_OPAQUE;
						ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect,
							Spaces, min(nSpaceCount, nCnt), 0);

					} else if (drawImage) {
						BlitPictureTo(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
					} MCHKHEAP
					if (gbNoDblBuffer) GdiFlush();
				}
			}

			ConCharXLine[j] = (j ? ConCharXLine[j-1] : 0)+CharWidth(c);
			MCHKHEAP


			if (gSet.isForceMonospace)
			{
				MCHKHEAP
				SetBkColor(hDC, gSet.Colors[attrBack]);

				j2 = j + 1;

				if (gSet.isFixFarBorders) {
					if (!isUnicode)
						SelectFont(hFont);
					else if (isUnicode)
						SelectFont(hFont2);
				}


				RECT rect;
				if (!gSet.isTTF)
					rect = MakeRect(j * gSet.LogFont.lfWidth, pos, j2 * gSet.LogFont.lfWidth, pos + gSet.LogFont.lfHeight);
				else {
					rect.left = j ? ConCharXLine[j-1] : 0;
					rect.top = pos;
					rect.right = (TextWidth>(UINT)j2) ? ConCharXLine[j2-1] : Width;
					rect.bottom = rect.top + gSet.LogFont.lfHeight;
				}
				UINT nFlags = ETO_CLIPPED | ((drawImage && (attrBack) < 2) ? 0 : ETO_OPAQUE);
				int nShift = 0;

				MCHKHEAP
				if (c != 0x20 && !isUnicode) {
					ABC abc;
					//This function succeeds only with TrueType fonts
					if (GetCharABCWidths(hDC, c, c, &abc))
					{
						
						if (abc.abcA<0) {
							// иначе символ наверное налезет на предыдущий?
							nShift = -abc.abcA;
						} else if (abc.abcA<(((int)gSet.LogFont.lfWidth-(int)abc.abcB-1)/2)) {
							// символ I, i, и др. очень тонкие - рисуем посередине
							nShift = ((gSet.LogFont.lfWidth-abc.abcB)/2)-abc.abcA;
						}
					}
				}

				MCHKHEAP
				if (! (drawImage && (attrBack) < 2)) {
					SetBkColor(hDC, gSet.Colors[attrBack]);
					//TODO: надо раскомментарить и куда-то приткнуть...
					if (nShift>0) ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect, L" ", 1, 0);
				} else if (drawImage) {
					BlitPictureTo(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
				}

				if (nShift>0) {
					rect.left += nShift; rect.right += nShift;
				}

				if (gbNoDblBuffer) GdiFlush();
				ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect, &c, 1, 0);
				if (gbNoDblBuffer) GdiFlush();
				MCHKHEAP

			}
			else if (gSet.isTTF && c==L' ')
			{
				j2 = j + 1; MCHKHEAP
				if (!doSelect) // doSelect инициализируется только для gConEmu.BufferHeight>0
				{
					TCHAR ch;
					while(j2 < end && ConAttrLine[j2] == attr && (ch=ConCharLine[j2]) == L' ')
					{
						ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
						j2++;
					} MCHKHEAP
					if (j2>=(int)TextWidth || isCharBorderVertical(ConCharLine[j2])) //ch может быть не инициализирован
					{
						ConCharXLine[j2-1] = (j2>=(int)TextWidth) ? Width : (j2) * gSet.LogFont.lfWidth; // или тут [j] должен быть?
						MCHKHEAP
						DWORD n1 = ConCharXLine[j];
						DWORD n3 = j2-j; // кол-во символов
						DWORD n2 = ConCharXLine[j2-1] - n1; // выделенная на пробелы ширина
						MCHKHEAP

						for (int k=j+1; k<(j2-1); k++) {
							ConCharXLine[k] = n1 + (n3 ? klMulDivU32(k-j, n2, n3) : 0);
							MCHKHEAP
						}
					} MCHKHEAP
				}
				if (gSet.isFixFarBorders)
					SelectFont(hFont);
			}
			else if (!isUnicode)
			{
				j2 = j + 1; MCHKHEAP
				if (!doSelect) // doSelect инициализируется только для gConEmu.BufferHeight>0
				{
					#ifndef DRAWEACHCHAR
					// Если этого не делать - в пропорциональных шрифтах буквы будут наезжать одна на другую
					TCHAR ch;
					while(j2 < end && ConAttrLine[j2] == attr && 
						!isCharBorder(ch = ConCharLine[j2]) 
						&& (!gSet.isTTF || !isFilePanel || (ch != L'}' && ch!=L' '))) // корректировка имен в колонках
					{
						ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
						j2++;
					}
					#endif
				}
				if (gSet.isFixFarBorders)
					SelectFont(hFont);
				MCHKHEAP
			}
			else //Border and specials
			{
				j2 = j + 1; MCHKHEAP
				if (!doSelect) // doSelect инициализируется только для gConEmu.BufferHeight>0
				{
					if (!gSet.isFixFarBorders)
					{
						TCHAR ch;
						while(j2 < end && ConAttrLine[j2] == attr && isCharBorder(ch = ConCharLine[j2]))
						{
							ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
							j2++;
						}
					}
					else
					{
						TCHAR ch;
						while(j2 < end && ConAttrLine[j2] == attr && 
							isCharBorder(ch = ConCharLine[j2]) && ch == ConCharLine[j2+1])
						{
							ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
							j2++;
						}
					}
				}
				if (gSet.isFixFarBorders)
					SelectFont(hFont2);
				MCHKHEAP
			}

			if (!gSet.isForceMonospace)
			{
				RECT rect;
				if (!gSet.isTTF)
					rect = MakeRect(j * gSet.LogFont.lfWidth, pos, j2 * gSet.LogFont.lfWidth, pos + gSet.LogFont.lfHeight);
				else {
					rect.left = j ? ConCharXLine[j-1] : 0;
					rect.top = pos;
					rect.right = (TextWidth>(UINT)j2) ? ConCharXLine[j2-1] : Width;
					rect.bottom = rect.top + gSet.LogFont.lfHeight;
				}

				MCHKHEAP
				if (! (drawImage && (attrBack) < 2))
					SetBkColor(hDC, gSet.Colors[attrBack]);
				else if (drawImage)
					BlitPictureTo(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);

				UINT nFlags = ETO_CLIPPED | ((drawImage && (attrBack) < 2) ? 0 : ETO_OPAQUE);

				MCHKHEAP
				if (gbNoDblBuffer) GdiFlush();
				if (gSet.isTTF && c == ' ') {
					int nCnt = ((rect.right - rect.left) / CharWidth(L' '))+1;
					ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect,
						Spaces, nCnt, 0);
				} else
				if (gSet.LogFont.lfCharSet == OEM_CHARSET && !isUnicode)
				{
					WideCharToMultiByte(CP_OEMCP, 0, ConCharLine + j, j2 - j, tmpOem, TextWidth+4, 0, 0);
					ExtTextOutA(hDC, rect.left, rect.top, nFlags,
						&rect, tmpOem, j2 - j, 0);
				}
				else
				{
					if ((j2-j)==1) // support visualizer change
					ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect,
						&c/*ConCharLine + j*/, 1, 0);
					else
					ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect,
						ConCharLine + j, j2 - j, 0);
				}
				if (gbNoDblBuffer) GdiFlush();
				MCHKHEAP
			}
		
			// stop if all is done
			if (!updateText)
				goto done;

			// skip next not changed symbols again
			if (skipNotChanged)
			{
				MCHKHEAP
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
	return;
}


void CVirtualConsole::UpdateCursor(bool& lRes)
{
	//------------------------------------------------------------------------
	///| Drawing cursor |/////////////////////////////////////////////////////
	//------------------------------------------------------------------------
	Cursor.isVisiblePrevFromInfo = cinf.bVisible;
	if ((lRes || Cursor.isVisible != Cursor.isVisiblePrev) && cinf.bVisible && isCursorValid)
	{
		lRes = true;

		SelectFont(hFont);

		if ((Cursor.x != csbi.dwCursorPosition.X || Cursor.y != csbi.dwCursorPosition.Y))
		{
			Cursor.isVisible = isMeForeground();
			gConEmu.cBlinkNext = 0;
		}

		int CurChar = csbi.dwCursorPosition.Y * TextWidth + csbi.dwCursorPosition.X;
		Cursor.ch[1] = 0;
		GetCharAttr(ConChar[CurChar], ConAttr[CurChar], Cursor.ch[0], Cursor.bgColorNum, Cursor.foreColorNum);
		Cursor.foreColor = gSet.Colors[Cursor.foreColorNum];
		Cursor.bgColor = gSet.Colors[Cursor.bgColorNum];

		/*Cursor.ch[0] = ConChar[CurChar];
		Cursor.foreColor = gSet.Colors[ConAttr[CurChar] >> 4 & 0x0F];
		Cursor.foreColorNum = ConAttr[CurChar] >> 4 & 0x0F;
		Cursor.bgColor = gSet.Colors[ConAttr[CurChar] & 0x0F];*/

		Cursor.isVisiblePrev = Cursor.isVisible;
		Cursor.x = csbi.dwCursorPosition.X;
		Cursor.y = csbi.dwCursorPosition.Y;

		COORD pix;
		pix.X = Cursor.x * gSet.LogFont.lfWidth;
		pix.Y = Cursor.y * gSet.LogFont.lfHeight;
		if (Cursor.x && ConCharX[CurChar-1])
			pix.X = ConCharX[CurChar-1];

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
				BlitPictureTo(pix.X, pix.Y, gSet.LogFont.lfWidth, gSet.LogFont.lfHeight);

			SetTextColor(hDC, Cursor.bgColor);
			SetBkColor(hDC, Cursor.foreColor);
			cinf.dwSize = 99;
		}

		RECT rect;
		if (!gSet.isCursorV)
		{
			if (gSet.isTTF) {
				rect.left = pix.X; /*Cursor.x * gSet.LogFont.lfWidth;*/
				rect.right = pix.X + gSet.LogFont.lfWidth; /*(Cursor.x+1) * gSet.LogFont.lfWidth;*/ //TODO: а ведь позиция следующего символа известна!
			} else {
				rect.left = Cursor.x * gSet.LogFont.lfWidth;
				rect.right = (Cursor.x+1) * gSet.LogFont.lfWidth;
			}
			//rect.top = (Cursor.y+1) * gSet.LogFont.lfHeight - MulDiv(gSet.LogFont.lfHeight, cinf.dwSize, 100);
			rect.bottom = (Cursor.y+1) * gSet.LogFont.lfHeight;
			rect.top = (Cursor.y * gSet.LogFont.lfHeight) /*+ 1*/;
			if (cinf.dwSize<50)
				rect.top = max(rect.top, (rect.bottom-HCURSORWIDTH));
		}
		else
		{
			if (gSet.isTTF) {
				rect.left = pix.X; /*Cursor.x * gSet.LogFont.lfWidth;*/
				//rect.right = rect.left/*Cursor.x * gSet.LogFont.lfWidth*/ //TODO: а ведь позиция следующего символа известна!
				//	+ klMax(1, MulDiv(gSet.LogFont.lfWidth, cinf.dwSize, 100) 
				//	+ (cinf.dwSize > 10 ? 1 : 0));
			} else {
				rect.left = Cursor.x * gSet.LogFont.lfWidth;
				//rect.right = Cursor.x * gSet.LogFont.lfWidth
				//	+ klMax(1, MulDiv(gSet.LogFont.lfWidth, cinf.dwSize, 100) 
				//	+ (cinf.dwSize > 10 ? 1 : 0));
			}
			rect.top = Cursor.y * gSet.LogFont.lfHeight;
			int nR = (gSet.isTTF && ConCharX[CurChar]) // правая граница
				? ConCharX[CurChar] : ((Cursor.x+1) * gSet.LogFont.lfWidth);
			if (cinf.dwSize>=50)
				rect.right = nR;
			else
				rect.right = min(nR, (rect.left+VCURSORWIDTH));
			//rect.right = rect.left/*Cursor.x * gSet.LogFont.lfWidth*/ //TODO: а ведь позиция следующего символа известна!
			//		+ klMax(1, MulDiv(gSet.LogFont.lfWidth, cinf.dwSize, 100) 
			//		+ (cinf.dwSize > 10 ? 1 : 0));
			rect.bottom = (Cursor.y+1) * gSet.LogFont.lfHeight;
		}

		if (gSet.LogFont.lfCharSet == OEM_CHARSET && !isCharBorder(Cursor.ch[0]))
		{
			if (gSet.isFixFarBorders)
				SelectFont(hFont);

			char tmp[2];
			WideCharToMultiByte(CP_OEMCP, 0, Cursor.ch, 1, tmp, 1, 0, 0);
			ExtTextOutA(hDC, pix.X, pix.Y,
				ETO_CLIPPED | ((drawImage && (Cursor.foreColorNum < 2) &&
				!Cursor.isVisible) ? 0 : ETO_OPAQUE),&rect, tmp, 1, 0);
		}
		else
		{
			if (gSet.isFixFarBorders && isCharBorder(Cursor.ch[0]))
				SelectFont(hFont2);
			else
				SelectFont(hFont);

			ExtTextOut(hDC, pix.X, pix.Y,
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
}
