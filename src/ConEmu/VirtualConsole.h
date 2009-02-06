#pragma once
#include "kl_parts.h"

struct VirtualConsole
{
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
		BYTE foreColorNum;
		TCHAR ch[2];
	} Cursor;

	HANDLE  hConOut_;
    HANDLE  hConOut();
	HDC     hDC, hBgDc;
	HBITMAP hBitmap, hBgBitmap;
	SIZE	bgBmp;
	HFONT   hFont, hFont2, hSelectedFont;

	TCHAR *ConChar;
	WORD  *ConAttr;
	CONSOLE_SELECTION_INFO SelectionInfo;

	VirtualConsole(/*HANDLE hConsoleOutput = NULL*/);
	~VirtualConsole();

	bool InitFont(void);
	bool InitDC(void);
	void Free(bool bFreeFont = true);
	bool Update(bool isForce = false, HDC *ahDc=NULL);
	void SelectFont(HFONT hNew);
};

extern VirtualConsole *pVCon;