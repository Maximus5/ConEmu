
/*
Copyright (c) 2009-2010 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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
#include "kl_parts.h"
#include "../Common/common.hpp"

#include "Options.h"
#include "RealConsole.h"

#define MAX_COUNT_PART_BRUSHES 16*16*4
#define MAX_SPACES 0x400

class CVirtualConsole
{
private:
	// RealConsole
	CRealConsole *mp_RCon;
public:
	CRealConsole *RCon() { if (this) return mp_RCon; return NULL; };
public:
    WARNING("Сделать protected!");
	uint TextWidth, TextHeight; // размер в символах
	uint Width, Height; // размер в пикселях
private:
	uint nMaxTextWidth, nMaxTextHeight; // размер в символах
private:
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
		wchar_t ch;
		DWORD nBlinkTime, nLastBlink;
		RECT lastRect;
		UINT lastSize; // предыдущая высота курсора (в процентах)
	} Cursor;
	//
	bool    mb_IsForceUpdate; // Это устанавливается в InitDC, чтобы случайно isForce не потерялся
	bool    mb_RequiredForceUpdate; // Сменился шрифт, например...
private:
	HDC     hDC;
	HBITMAP hBitmap;
	HBRUSH  hBrush0, hOldBrush, hSelectedBrush;
	HFONT   hSelectedFont, hOldFont;
	#ifdef _DEBUG
	BOOL    mb_DebugDumpDC;
	#endif
public:
	bool isEditor, isViewer, isFilePanel;
	BYTE attrBackLast;

	wchar_t  *mpsz_ConChar, *mpsz_ConCharSave;
	CharAttr *mpn_ConAttrEx, *mpn_ConAttrExSave;
	DWORD *ConCharX;
	//TCHAR *Spaces; WORD nSpaceCount;
	static wchar_t ms_Spaces[MAX_SPACES], ms_HorzDbl[MAX_SPACES], ms_HorzSingl[MAX_SPACES];
	// Для ускорения получения индексов цвета
	//BYTE  m_ForegroundColors[0x100], m_BackgroundColors[0x100];
	//HFONT mh_FontByIndex[0x100]; // содержит ссылки (не копии) на шрифты normal/bold/italic
	HFONT mh_FontByIndex[MAX_FONT_STYLES]; // ссылки на Normal/Bold/Italic/Bold&Italic/...Underline

	//CONSOLE_SELECTION_INFO SelectionInfo;

	CVirtualConsole(/*HANDLE hConsoleOutput = NULL*/);
	~CVirtualConsole();
	static CVirtualConsole* CreateVCon(RConStartArgs *args);

	bool InitDC(bool abNoDc, bool abNoWndResize);
	void DumpConsole();
	BOOL Dump(LPCWSTR asFile);
	bool Update(bool isForce = false, HDC *ahDc=NULL);
	void UpdateCursor(bool& lRes);
	void SelectFont(HFONT hNew);
	void SelectBrush(HBRUSH hNew);
	inline bool isCharBorder(WCHAR inChar);
	static bool isCharBorderVertical(WCHAR inChar);
	static bool isCharProgress(WCHAR inChar);
	static bool isCharScroll(WCHAR inChar);
	void BlitPictureTo(int inX, int inY, int inWidth, int inHeight);
	bool CheckSelection(const CONSOLE_SELECTION_INFO& select, SHORT row, SHORT col);
	//bool GetCharAttr(TCHAR ch, WORD atr, TCHAR& rch, BYTE& foreColorNum, BYTE& backColorNum, FONT* pFont);
	void Paint(HDC hDc, RECT rcClient);
	void UpdateInfo();
	//void GetConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci) { mp_RCon->GetConsoleCursorInfo(ci); };
	//DWORD GetConsoleCP() { return mp_RCon->GetConsoleCP(); };
	//DWORD GetConsoleOutputCP() { return mp_RCon->GetConsoleOutputCP(); };
	//DWORD GetConsoleMode() { return mp_RCon->GetConsoleMode(); };
	//void GetConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO *sbi) { mp_RCon->GetConsoleScreenBufferInfo(sbi); };
	RECT GetRect();
	void OnFontChanged();
	COORD ClientToConsole(LONG x, LONG y);
	void OnConsoleSizeChanged();
	static void ClearPartBrushes();

protected:
	//inline void GetCharAttr(WORD atr, BYTE& foreColorNum, BYTE& backColorNum, HFONT* pFont);
	wchar_t* mpsz_LogScreen; DWORD mn_LogScreenIdx;
	enum _PartType{
		pNull,  // конец строки/последний, неотображаемый элемент
		pSpace, // при разборе строки будем смотреть, если нашли pText,pSpace,pText то pSpace,pText добавить в первый pText
		pUnderscore, // '_' прочерк. их тоже будем чикать в угоду тексту
		pBorder,
		pVBorder, // символы вертикальных рамок, которые нельзя сдвигать
		pRBracket, // символом '}' фар помечает файлы, вылезшие из колонки
		pText,
		pDummy  // дополнительные "пробелы", которые нужно отрисовать после конца строки
	};
	enum _PartType GetCharType(TCHAR ch);
	struct _TextParts {
		enum _PartType partType;
		BYTE attrFore, attrBack; // однотипными должны быть не только символы, но и совпадать атрибуты!
		WORD i1,i2;  // индексы в текущей строке, 0-based
		WORD iwidth; // количество символов в блоке
		DWORD width; // ширина текста в символах. для pSpace & pBorder может обрезаться в пользу pText/pVBorder

		int x1; // координата в пикселях (скорректированные)
	} *TextParts;
	CONSOLE_SCREEN_BUFFER_INFO csbi; DWORD mdw_LastError;
	CONSOLE_CURSOR_INFO	cinf;
	COORD winSize, coord;
	//CONSOLE_SELECTION_INFO select1, select2;
	uint TextLen;
	bool isCursorValid, drawImage, textChanged, attrChanged;
	char *tmpOem;
	void UpdateCursorDraw(HDC hPaintDC, RECT rcClient, COORD pos, UINT dwSize);
	bool UpdatePrepare(bool isForce, HDC *ahDc, MSectionLock *pSDC);
	void UpdateText(bool isForce); //, bool updateText, bool updateCursor);
	WORD CharWidth(TCHAR ch);
	void CharABC(TCHAR ch, ABC *abc);
	bool CheckChangedTextAttr();
	//void ParseLine(int row, TCHAR *ConCharLine, WORD *ConAttrLine);
	HANDLE mh_Heap;
	LPVOID Alloc(size_t nCount, size_t nSize);
	void Free(LPVOID ptr);
	MSection csDC;  /*DWORD ncsTDC;*/ BOOL mb_PaintRequested; BOOL mb_PaintLocked;
	MSection csCON; /*DWORD ncsTCON;*/
	int mn_BackColorIdx; //==0
	void Box(LPCTSTR szText);
	static char mc_Uni2Oem[0x10000];
	char Uni2Oem(wchar_t ch);
	//BOOL RetrieveConsoleInfo(BOOL bShortOnly);
	typedef struct tag_PARTBRUSHES {
		wchar_t ch; // 0x2591 0x2592 0x2593 0x2588 - по увеличению плотности
		COLORREF nBackCol;
		COLORREF nForeCol;
		HBRUSH  hBrush;
	} PARTBRUSHES;
	//std::vector<PARTBRUSHES> m_PartBrushes;
	static PARTBRUSHES m_PartBrushes[MAX_COUNT_PART_BRUSHES];
	//static HBRUSH PartBrush(wchar_t ch, SHORT nBackIdx, SHORT nForeIdx);
	static HBRUSH PartBrush(wchar_t ch, COLORREF nBackCol, COLORREF nForeCol);
	BOOL mb_InPaintCall;
	//
	void DistributeSpaces(wchar_t* ConCharLine, CharAttr* ConAttrLine, DWORD* ConCharXLine, const int j, const int j2, const int end);
	BOOL FindChanges(int &j, int &end, const wchar_t* ConCharLine, const CharAttr* ConAttrLine, const wchar_t* ConCharLine2, const CharAttr* ConAttrLine2);
	LONG nFontHeight, nFontWidth;
	BYTE nFontCharSet;
	BYTE nLastNormalBack;
    bool bExtendFonts, bExtendColors;
    BYTE nFontNormalColor, nFontBoldColor, nFontItalicColor, nExtendColor;
};

#include <pshpack1.h>
typedef struct tagMYRGB {
	union {
		COLORREF color;
		struct {
			BYTE    rgbBlue;
			BYTE    rgbGreen;
			BYTE    rgbRed;
			BYTE    rgbReserved;
		};
	};
} MYRGB;
#include <poppack.h>
