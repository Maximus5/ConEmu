#pragma once
#include "kl_parts.h"
#include "../Common/common.hpp"

#include "RealConsole.h"


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
		TCHAR ch[2];
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

	wchar_t *mpsz_ConChar, *mpsz_ConCharSave;
	WORD  *mpn_ConAttr, *mpn_ConAttrSave;
	DWORD *ConCharX;
	TCHAR *Spaces; WORD nSpaceCount;

	CONSOLE_SELECTION_INFO SelectionInfo;

	CVirtualConsole(/*HANDLE hConsoleOutput = NULL*/);
	~CVirtualConsole();
	static CVirtualConsole* Create(bool abDetached, LPCWSTR asNewCmd = NULL);

	bool InitDC(bool abNoDc, bool abNoWndResize);
	void DumpConsole();
	BOOL Dump(LPCWSTR asFile);
	bool Update(bool isForce = false, HDC *ahDc=NULL);
	void UpdateCursor(bool& lRes);
	void SelectFont(HFONT hNew);
	void SelectBrush(HBRUSH hNew);
	bool isCharBorder(WCHAR inChar);
	bool isCharBorderVertical(WCHAR inChar);
	bool isCharProgress(WCHAR inChar);
	void BlitPictureTo(int inX, int inY, int inWidth, int inHeight);
	bool CheckSelection(const CONSOLE_SELECTION_INFO& select, SHORT row, SHORT col);
	bool GetCharAttr(TCHAR ch, WORD atr, TCHAR& rch, BYTE& foreColorNum, BYTE& backColorNum);
	void Paint();
	void UpdateInfo();
	void GetConsoleSelectionInfo(CONSOLE_SELECTION_INFO *sel) {	mp_RCon->GetConsoleSelectionInfo(sel); };
	void GetConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci) { mp_RCon->GetConsoleCursorInfo(ci); };
	DWORD GetConsoleCP() { return mp_RCon->GetConsoleCP(); };
	DWORD GetConsoleOutputCP() { return mp_RCon->GetConsoleOutputCP(); };
	DWORD GetConsoleMode() { return mp_RCon->GetConsoleMode(); };
	void GetConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO *sbi) { mp_RCon->GetConsoleScreenBufferInfo(sbi); };
	RECT GetRect();
	void OnFontChanged();
	COORD ClientToConsole(LONG x, LONG y);

protected:
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
	CONSOLE_SELECTION_INFO select1, select2;
	uint TextLen;
	bool isCursorValid, drawImage, doSelect, textChanged, attrChanged;
	char *tmpOem;
	void UpdateCursorDraw(HDC hPaintDC, COORD pos, UINT dwSize);
	bool UpdatePrepare(bool isForce, HDC *ahDc, MSectionLock *pSDC);
	void UpdateText(bool isForce, bool updateText, bool updateCursor);
	WORD CharWidth(TCHAR ch);
	bool CheckChangedTextAttr();
	void ParseLine(int row, TCHAR *ConCharLine, WORD *ConAttrLine);
	HANDLE mh_Heap;
	LPVOID Alloc(size_t nCount, size_t nSize);
	void Free(LPVOID ptr);
	MSection csDC;  /*DWORD ncsTDC;*/ BOOL mb_PaintRequested; BOOL mb_PaintLocked;
	MSection csCON; /*DWORD ncsTCON;*/
	int mn_BackColorIdx; //==0
	void Box(LPCTSTR szText);
	//BOOL RetrieveConsoleInfo(BOOL bShortOnly);
	typedef struct tag_PARTBRUSHES {
		wchar_t ch; // 0x2591 0x2592 0x2593 0x2588 - по увеличению плотности
		SHORT   nBackIdx;
		SHORT   nForeIdx;
		HBRUSH  hBrush;
	} PARTBRUSHES;
	std::vector<PARTBRUSHES> m_PartBrushes;
	HBRUSH PartBrush(wchar_t ch, SHORT nBackIdx, SHORT nForeIdx);
	BOOL mb_InPaintCall;
	//
	void DistributeSpaces(wchar_t* ConCharLine, WORD* ConAttrLine, DWORD* ConCharXLine, int &j, int &j2, int &end);
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
