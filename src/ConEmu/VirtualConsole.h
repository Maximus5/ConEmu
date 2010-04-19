
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
	HWND GetView();
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
		bool isPrevBackground;
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
	BOOL    mb_ConDataChanged;
	HRGN    mh_TransparentRgn;
	//
	bool InitDC(bool abNoDc, bool abNoWndResize);
private:
	enum _PartType{
		pNull=0,     // конец строки/последний, неотображаемый элемент
		pSpace,      // при разборе строки будем смотреть, если нашли pText,pSpace,pText то pSpace,pText добавить в первый pText
		pBorder,     // символы, которые нужно рисовать шрифтом hFont2
		pFills,      // Progressbars & Scrollbars
		pText,       // Если шрифт НЕ OEM  (вывод через TextOutW)
		pOemText,    // Если шрифт OEM-ный (вывод нужно делать через TextOutA)
		pDummy  // дополнительные "пробелы", которые нужно отрисовать после конца строки
		//pUnderscore, // '_' прочерк. их тоже будем чикать в угоду тексту
	};
	enum _PartType GetCharType(TCHAR ch);
	typedef struct _TextParts {
		enum _PartType partType;
		int  nFontIdx;   // Индекс используемого шрифта
		COLORREF crFore; // Цвет текста
		COLORREF crBack; // !!! Используется только для отрисовки блочных симполов (прогрессов и пр.)
		uint i;     // индекс в текущей строке (0-based)
		uint n;     // количество символов в блоке
		int  x;     // координата начала строки (may be >0)
		uint width; // ширини символов блока в пикселях
		
		DWORD *pDX; // сдвиги для отрисовки (как-бы ширины знакомест). Это указатель на часть ConCharDX
	} TEXTPARTS;
	typedef struct _BgParts {
		// индекс ячейки
		uint i; // i строго больше 0 (0 - основной фон и он уже залит)
		// и количество ячеек
		uint n;
		// Картинка или фон?
		BOOL bBackIsPic;
		COLORREF nBackRGB;
	} BGPARTS;

	// Working pointers
	bool mb_PointersAllocated;
	wchar_t  *mpsz_ConChar, *mpsz_ConCharSave;   // nMaxTextWidth * nMaxTextHeight
	// CharAttr определен в RealConsole.h
	CharAttr *mpn_ConAttrEx, *mpn_ConAttrExSave; // nMaxTextWidth * nMaxTextHeight
	DWORD *ConCharX;      // nMaxTextWidth * nMaxTextHeight
	DWORD *ConCharDX;     // nMaxTextWidth
	char  *tmpOem;        // nMaxTextWidth
	TEXTPARTS *TextParts; // nMaxTextWidth + 1
	BGPARTS *BgParts;     // nMaxTextWidth
	POLYTEXT  *PolyText;  // nMaxTextWidth
	bool *pbLineChanged;  // nMaxTextHeight
	bool *pbBackIsPic;    // nMaxTextHeight :: заполняется если *pbLineChanged
	COLORREF* pnBackRGB;  // nMaxTextHeight :: заполняется если *pbLineChanged и НЕ *pbBackIsPic
	
	// функции выделения памяти
	void PointersInit();
	void PointersFree();
	bool PointersAlloc();
	void PointersZero();
	
	// *** Анализ строк ***
	// Заливка измененных строк основным фоном и заполнение pbLineChanged, pbBackIsPic, pnBackRGB
	void Update_CheckAndFill(bool isForce);
	// Разбор строки на составляющие (возвращает true, если есть ячейки с НЕ основным фоном)
	// Функция также производит распределение (заполнение координат и DX)
	bool Update_ParseTextParts(uint row, const wchar_t* ConCharLine, const CharAttr* ConAttrLine);
	// Заливка ячеек с НЕ основным фоном, отрисовка прогрессов и рамок
	void Update_FillAlternate(uint row, uint nY);
	// Вывод собственно текста (при необходимости Clipped)
	void Update_DrawText(uint row, uint nY);

	// распределение ширин
	void DistributeSpaces(wchar_t* ConCharLine, CharAttr* ConAttrLine, DWORD* ConCharXLine, const int j, const int j2, const int end);
	
	// PanelViews
	PanelViewInit m_LeftPanelView, m_RightPanelView;
	BOOL mb_LeftPanelRedraw, mb_RightPanelRedraw;
	SMALL_RECT mrc_LastDialogs[32]; int mn_LastDialogsCount;
	BOOL UpdatePanelView(BOOL abLeftPanel);
	BOOL UpdatePanelRgn(BOOL abLeftPanel, BOOL abTestOnly=FALSE, BOOL abOnRegister=FALSE);
	void PolishPanelViews();
	HRGN CreateConsoleRgn(int x1, int y1, int x2, int y2, BOOL abTestOnly);
	BOOL CheckDialogsChanged();
	BOOL mb_DialogsChanged;
	
public:
	bool isEditor, isViewer, isFilePanel, isFade, isForeground;
	BYTE attrBackLast;
	COLORREF *mp_Colors;

	//TCHAR *Spaces; WORD nSpaceCount;
	static wchar_t ms_Spaces[MAX_SPACES], ms_HorzDbl[MAX_SPACES], ms_HorzSingl[MAX_SPACES];
	// Для ускорения получения индексов цвета
	//BYTE  m_ForegroundColors[0x100], m_BackgroundColors[0x100];
	//HFONT mh_FontByIndex[0x100]; // содержит ссылки (не копии) на шрифты normal/bold/italic
	HFONT mh_FontByIndex[MAX_FONT_STYLES]; // ссылки на Normal/Bold/Italic/Bold&Italic/...Underline
	
	bool  mb_LastFadeFlag;

	//CONSOLE_SELECTION_INFO SelectionInfo;

	CVirtualConsole(/*HANDLE hConsoleOutput = NULL*/);
	~CVirtualConsole();
	static CVirtualConsole* CreateVCon(RConStartArgs *args);

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
	void Paint(HDC hPaintDc, RECT rcClient);
	void UpdateInfo();
	//void GetConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci) { mp_RCon->GetConsoleCursorInfo(ci); };
	//DWORD GetConsoleCP() { return mp_RCon->GetConsoleCP(); };
	//DWORD GetConsoleOutputCP() { return mp_RCon->GetConsoleOutputCP(); };
	//DWORD GetConsoleMode() { return mp_RCon->GetConsoleMode(); };
	//void GetConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO *sbi) { mp_RCon->GetConsoleScreenBufferInfo(sbi); };
	RECT GetRect();
	void OnFontChanged();
	COORD ClientToConsole(LONG x, LONG y);
	POINT ConsoleToClient(LONG x, LONG y);
	void OnConsoleSizeChanged();
	static void ClearPartBrushes();
	HRGN GetExclusionRgn(bool abTestOnly=false);
	COORD FindOpaqueCell();
	void ShowPopupMenu(POINT ptCur);
	BOOL RegisterPanelView(PanelViewInit* ppvi);

protected:
	//inline void GetCharAttr(WORD atr, BYTE& foreColorNum, BYTE& backColorNum, HFONT* pFont);
	wchar_t* mpsz_LogScreen; DWORD mn_LogScreenIdx;
	CONSOLE_SCREEN_BUFFER_INFO csbi; DWORD mdw_LastError;
	CONSOLE_CURSOR_INFO	cinf;
	COORD winSize, coord;
	//CONSOLE_SELECTION_INFO select1, select2;
	uint TextLen;
	bool isCursorValid, drawImage, textChanged, attrChanged;
	void UpdateCursorDraw(HDC hPaintDC, RECT rcClient, COORD pos, UINT dwSize);
	bool UpdatePrepare(bool isForce, HDC *ahDc, MSectionLock *pSDC);
	void UpdateText(bool isForce); //, bool updateText, bool updateCursor);
	BOOL CheckTransparentRgn();
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
	BOOL FindChanges(int &j, int &end, const wchar_t* ConCharLine, const CharAttr* ConAttrLine, const wchar_t* ConCharLine2, const CharAttr* ConAttrLine2);
	LONG nFontHeight, nFontWidth;
	BYTE nFontCharSet;
	BYTE nLastNormalBack;
    bool bExtendFonts, bExtendColors;
    BYTE nFontNormalColor, nFontBoldColor, nFontItalicColor, nExtendColor;
	struct _TransparentInfo {
		INT    nRectCount;
		POINT *pAllPoints;
		INT   *pAllCounts;
	} TransparentInfo;
	static HMENU mh_PopupMenu;
};
