#pragma once
#include "kl_parts.h"

class CVirtualConsole
{
public:
	bool IsForceUpdate;
	uint TextWidth, TextHeight;
	uint Width, Height;

	struct
	{
		bool isVisible;
		bool isVisiblePrev;
		bool isVisiblePrevFromInfo;
		short x;
		short y;
		COLORREF foreColor;
		COLORREF bgColor;
		BYTE foreColorNum, bgColorNum;
		TCHAR ch[2];
	} Cursor;

	HANDLE  hConOut_;
    HANDLE  hConOut();
	HDC     hDC, hBgDc;
	HBITMAP hBitmap, hBgBitmap;
	HBRUSH  hBrush0, hOldBrush, hSelectedBrush;
	SIZE	bgBmp;
	HFONT   hFont, hFont2, hSelectedFont, hOldFont;

	bool isEditor, isFilePanel;
	BYTE attrBackLast;

	TCHAR *ConChar;
	WORD  *ConAttr;
	WORD  FontWidth[0x10000], Font2Width[0x10000];
	DWORD *ConCharX;
	TCHAR *Spaces; WORD nSpaceCount;

	CONSOLE_SELECTION_INFO SelectionInfo;

	CVirtualConsole(/*HANDLE hConsoleOutput = NULL*/);
	~CVirtualConsole();

	bool InitFont(void);
	bool InitDC(BOOL abFull=TRUE);
	void Free(bool bFreeFont = true);
	bool Update(bool isForce = false, HDC *ahDc=NULL);
	void SelectFont(HFONT hNew);
	void SelectBrush(HBRUSH hNew);
	HFONT CreateFontIndirectMy(LOGFONT *inFont);
	bool isCharBorder(WCHAR inChar);
	bool isCharBorderVertical(WCHAR inChar);
	void BlitPictureTo(int inX, int inY, int inWidth, int inHeight);
	bool CheckSelection(const CONSOLE_SELECTION_INFO& select, SHORT row, SHORT col);
	bool GetCharAttr(TCHAR ch, WORD atr, TCHAR& rch, BYTE& foreColorNum, BYTE& backColorNum);

protected:
	struct _TextParts {
		enum {
			pNull,  // конец строки/последний, неотображаемый элемент
			pSpace, // при разборе строки будем смотреть, если нашли pText,pSpace,pText то pSpace,pText добавить в первый pText
			pBorder,
			pVBorder,
			pText
		} partType;
		BYTE attrFore, attrBack; // однотипными должны быть не только символы, но и совпадать атрибуты!
		WORD i1,i2;  // индексы в текущей строке, 0-based

		DWORD x,width; // координаты в пикселях (скорректированные)
	} *TextParts;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	CONSOLE_CURSOR_INFO	cinf;
	COORD winSize, coord;
	CONSOLE_SELECTION_INFO select1, select2;
	uint TextLen;
	bool isCursorValid, drawImage, doSelect, textChanged, attrChanged;
	i64 tick, tick2;
	char *tmpOem;
	void UpdateCursor(bool& lRes);
	void UpdatePrepare(bool isForce, HDC *ahDc);
	void UpdateText(bool isForce, bool updateText, bool updateCursor);
	WORD CharWidth(TCHAR ch);
	bool CheckChangedTextAttr();
};
