#pragma once
#include "kl_parts.h"

class CVirtualConsole
{
public:
	bool IsForceUpdate;
	uint TextWidth, TextHeight;
	uint Width, Height;
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
	} Cursor;
public:
	HANDLE  hConOut_;
    HANDLE  hConOut();
	HWND    hConWnd;
	HDC     hDC; //, hBgDc;
	HBITMAP hBitmap; //, hBgBitmap;
	HBRUSH  hBrush0, hOldBrush, hSelectedBrush;
	//SIZE	bgBmp;
	HFONT   /*hFont, hFont2,*/ hSelectedFont, hOldFont;

	bool isEditor, isFilePanel;
	BYTE attrBackLast;

	TCHAR *ConChar;
	WORD  *ConAttr;
	//WORD  FontWidth[0x10000]; //, Font2Width[0x10000];
	DWORD *ConCharX;
	TCHAR *Spaces; WORD nSpaceCount;

	CONSOLE_SELECTION_INFO SelectionInfo;

	CVirtualConsole(/*HANDLE hConsoleOutput = NULL*/);
	~CVirtualConsole();
	static CVirtualConsole* Create();

	bool InitDC(bool abNoDc);
	void InitHandlers();
	void DumpConsole();
	void Free();
	bool Update(bool isForce = false, HDC *ahDc=NULL);
	void SelectFont(HFONT hNew);
	void SelectBrush(HBRUSH hNew);
	//HFONT CreateFontIndirectMy(LOGFONT *inFont);
	bool isCharBorder(WCHAR inChar);
	bool isCharBorderVertical(WCHAR inChar);
	void BlitPictureTo(int inX, int inY, int inWidth, int inHeight);
	bool CheckSelection(const CONSOLE_SELECTION_INFO& select, SHORT row, SHORT col);
	bool GetCharAttr(TCHAR ch, WORD atr, TCHAR& rch, BYTE& foreColorNum, BYTE& backColorNum);
	void SetConsoleSize(const COORD& size);
	bool CheckBufferSize();

protected:
	i64 m_LastMaxReadCnt; DWORD m_LastMaxReadTick;
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
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	CONSOLE_CURSOR_INFO	cinf;
	COORD winSize, coord;
	CONSOLE_SELECTION_INFO select1, select2;
	uint TextLen;
	bool isCursorValid, drawImage, doSelect, textChanged, attrChanged;
	i64 tick, tick2;
	char *tmpOem;
	void UpdateCursor(bool& lRes);
	void UpdateCursorDraw(COORD pos, BOOL vis);
	bool UpdatePrepare(bool isForce, HDC *ahDc);
	void UpdateText(bool isForce, bool updateText, bool updateCursor);
	WORD CharWidth(TCHAR ch);
	bool CheckChangedTextAttr();
	void ParseLine(int row, TCHAR *ConCharLine, WORD *ConAttrLine);
	BOOL AttachPID(DWORD dwPID);
	BOOL StartProcess();
	typedef struct _ConExeProps {
		BOOL  bKeyExists;
		DWORD ScreenBufferSize; //Loword-Width, Hiword-Height
		DWORD WindowSize;
		DWORD WindowPosition;
		DWORD FontSize;
		DWORD FontFamily;
		TCHAR *FaceName;
		TCHAR *FullKeyName;
	} ConExeProps;
	void RegistryProps(BOOL abRollback, ConExeProps& props, LPCTSTR asExeName=NULL);
};
