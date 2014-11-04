﻿
/*
Copyright (c) 2009-2014 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
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
//#include "kl_parts.h"
//#include "../Common/common.hpp"
#include "../common/ConEmuCheck.h"
#include "../common/MSection.h"
#include "VConRelease.h"

#include "Options.h"
#include "RealConsole.h"
#include "VConChild.h"
#include "CustomFonts.h"

#define MAX_COUNT_PART_BRUSHES 16*16*4
#define MAX_SPACES 0x400

class CBackground;
class CTaskBarGhost;
class CVConGroup;
class CTab;
class CBackgroundInfo;

typedef DWORD VConFlags;
const VConFlags
	vf_Active    = 0x0001,
	vf_Visible   = 0x0002,
	vf_Minimized = 0x0004,
	vf_Maximized = 0x0008,
	vf_Grouped   = 0x0010,
	vf_None      = 0
;

class CVirtualConsole :
	public CVConRelease,
	public CConEmuChild
{
	private:
		// RealConsole
		CRealConsole  *mp_RCon;
		CConEmuMain   *mp_ConEmu;
		CTaskBarGhost *mp_Ghost;

	protected:
		friend class CVConGroup;
		void* mp_Group; // For internal use of CVConGroup
	protected:
		VConFlags      mn_Flags;
		void SetFlags(VConFlags flags);

	protected:
		CVirtualConsole(CConEmuMain* pOwner);
		bool Constructor(RConStartArgs *args);
	public:
		//static CVirtualConsole* CreateVCon(RConStartArgs *args); --> VConGroup
		void InitGhost();
	protected:
		virtual ~CVirtualConsole();
		friend class CVConRelease;
	public:
		CRealConsole *RCon();
		HWND GuiWnd();
		void setFocus();
		HWND GhostWnd();
		bool isActive(bool abAllowGroup);
		bool isVisible();
	public:
		WARNING("Сделать protected!");
		uint TextWidth, TextHeight; // размер в символах
		uint Width, Height; // размер в пикселях
		bool LoadConsoleData();
	private:
		uint nMaxTextWidth, nMaxTextHeight; // размер в символах
		uint LastPadSize;
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
		bool    isForce; // а это - сейчас (устанавливается по аргументу в Update)
		bool    isFontSizeChanged;
		DWORD   mn_LastBitsPixel;
	protected:
		friend class CConEmuChild;
		HDC GetIntDC();
	private:
		bool    mb_InUpdate;
		RECT    mrc_Client, mrc_Back;
		CEDC    m_DC;
		HBRUSH  hBrush0, hOldBrush, hSelectedBrush;
		HBRUSH  CreateBackBrush(bool bGuiVisible, bool& rbNonSystem, COLORREF *pColors = NULL);
		CEFONT  hSelectedFont, hOldFont;
		CEFONT  mh_FontByIndex[MAX_FONT_STYLES+1]; // ссылки на Normal/Bold/Italic/Bold&Italic/...Underline
		HFONT   mh_UCharMapFont; SMALL_RECT mrc_UCharMap;
		wchar_t ms_LastUCharMapFont[32];

		#ifdef _DEBUG
		bool    mb_DebugDumpDC;
		#endif

		bool    mb_ConDataChanged;
		HRGN    mh_TransparentRgn;
		//
		bool	mb_ChildWindowWasFound;
	public:
		bool InitDC(bool abNoDc, bool abNoWndResize, MSectionLock *pSDC, MSectionLock *pSCON);
	private:
		enum _PartType
		{
			pNull=0,     // конец строки/последний, неотображаемый элемент
			pSpace,      // при разборе строки будем смотреть, если нашли pText,pSpace,pText то pSpace,pText добавить в первый pText
			pBorder,     // символы, которые нужно рисовать шрифтом hFont2
			pFills,      // Progressbars & Scrollbars
			pText,       // Если шрифт НЕ OEM  (вывод через TextOutW)
			pOemText,    // Если шрифт OEM-ный (вывод нужно делать через TextOutA)
			pDummy  // дополнительные "пробелы", которые нужно отрисовать после конца строки
			//pUnderscore, // '_' прочерк. их тоже будем чикать в угоду тексту
		};
		enum _PartType GetCharType(wchar_t ch);
		typedef struct _TextParts
		{
			enum _PartType partType;
			int  nFontIdx;   // Индекс используемого шрифта
			COLORREF crFore; // Цвет текста
			COLORREF crBack; // !!! Используется только для отрисовки блочных симполов (прогрессов и пр.)
			uint i;     // индекс в текущей строке (0-based)
			uint n;     // количество символов в блоке
			int  x;     // координата начала строки (may be >0)
			uint width; // ширины символов блока в пикселях

			DWORD *pDX; // сдвиги для отрисовки (как-бы ширины знакомест). Это указатель на часть ConCharDX
		} TEXTPARTS;
		typedef struct _BgParts
		{
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
		ConEmuTextRange m_etr;// Подсветка URL's и строк-ошибок-компиляторов

		// функции выделения памяти
		void PointersInit();
		void PointersFree();
		bool PointersAlloc();
		void PointersZero();

		// *** Анализ строк ***
		// Заливка измененных строк основным фоном и заполнение pbLineChanged, pbBackIsPic, pnBackRGB
		void Update_CheckAndFill();
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
		bool mb_LeftPanelRedraw, mb_RightPanelRedraw;
		SMALL_RECT mrc_LastDialogs[MAX_DETECTED_DIALOGS]; int mn_LastDialogsCount; DWORD mn_LastDialogFlags[MAX_DETECTED_DIALOGS];
		SMALL_RECT mrc_Dialogs[MAX_DETECTED_DIALOGS]; int mn_DialogsCount; DWORD mn_DialogAllFlags, mn_DialogFlags[MAX_DETECTED_DIALOGS];
		bool UpdatePanelView(bool abLeftPanel, bool abOnRegister=false);
		CRgnRects m_RgnTest, m_RgnLeftPanel, m_RgnRightPanel;
		bool UpdatePanelRgn(bool abLeftPanel, bool abTestOnly=FALSE, bool abOnRegister=FALSE);
		void PolishPanelViews();
		bool CheckDialogsChanged();
		bool mb_DialogsChanged;
		UINT mn_ConEmuFadeMsg;
		UINT mn_ConEmuSettingsMsg;

		void CharAttrFromConAttr(WORD conAttr, CharAttr* pAttr);

		#ifdef __GNUC__
		AlphaBlend_t GdiAlphaBlend;
		#endif

	public:
		const PanelViewInit* GetPanelView(bool abLeftPanel);

	public:
		// Плагин к фару может установить свою "картинку" для панелей (например, нарисовать в фоне букву диска)
		//void FreeBackgroundImage(); // Освободить (если создан) HBITMAP для mp_BkImgData
		SetBackgroundResult SetBackgroundImageData(CESERVER_REQ_SETBACKGROUND* apImgData); // вызывается при получении нового Background
		bool HasBackgroundImage(LONG* pnBgWidth, LONG* pnBgHeight);
		//void NeedBackgroundUpdate();
		#ifdef APPDISTINCTBACKGROUND
		CBackgroundInfo* GetBackgroundObject();
		#endif
		void NeedBackgroundUpdate();
	protected:
		// Содержит текущий фон (из плагина или из файла-цвета по настройке)
		CBackground*     mp_Bg;
		#ifdef APPDISTINCTBACKGROUND
		CBackgroundInfo* mp_BgInfo; // RefRelease, global object list
		#endif

		//bool mb_NeedBgUpdate;
		//bool mb_BgLastFade;
		////CBackground* mp_PluginBg;
		//MSection *mcs_BkImgData;
		//size_t mn_BkImgDataMax;
		//CESERVER_REQ_SETBACKGROUND* mp_BkImgData; // followed by image data
		//bool mb_BkImgChanged; // Данные в mp_BkImgData были изменены плагином, требуется отрисовка
		//bool mb_BkImgExist; //, mb_BkImgDelete;
		//LONG mn_BkImgWidth, mn_BkImgHeight;
		//// Поддержка EMF
		//size_t mn_BkEmfDataMax;
		//CESERVER_REQ_SETBACKGROUND* mp_BkEmfData; // followed by EMF data
		//bool mb_BkEmfChanged; // Данные в mp_BkEmfData были изменены плагином, требуется отрисовка
		//UINT IsBackgroundValid(const CESERVER_REQ_SETBACKGROUND* apImgData, bool* rpIsEmf) const; // возвращает размер данных, или 0 при ошибке
		//bool PutBackgroundImage(CBackground* pBack, LONG X, LONG Y, LONG Width, LONG Height); // Положить в pBack свою картинку
		//bool PrepareBackground(HDC* phBgDc, COORD* pbgBmpSize);
//public:
		//MSection csBkImgData;

		const AppSettings* mp_Set;

	public:
		bool isEditor, isViewer, isFilePanel, isFade, isForeground;
		BYTE attrBackLast;
		COLORREF *mp_Colors;

		//wchar_t *Spaces; WORD nSpaceCount;
		static wchar_t ms_Spaces[MAX_SPACES], ms_HorzDbl[MAX_SPACES], ms_HorzSingl[MAX_SPACES];
		// Для ускорения получения индексов цвета
		//BYTE  m_ForegroundColors[0x100], m_BackgroundColors[0x100];
		//HFONT mh_FontByIndex[0x100]; // содержит ссылки (не копии) на шрифты normal/bold/italic

		bool  mb_LastFadeFlag;

		void DumpConsole();
		bool LoadDumpConsole();
		bool Dump(LPCWSTR asFile);
		bool Update(bool abForce = false, HDC *ahDc=NULL);
		void UpdateCursor(bool& lRes);
		void UpdateThumbnail(bool abNoSnapshoot = FALSE);
		void SelectFont(CEFONT hNew);
		void SelectBrush(HBRUSH hNew);
		inline bool isCharBorder(wchar_t inChar);
		static bool isCharBorderVertical(wchar_t inChar);
		static bool isCharProgress(wchar_t inChar);
		static bool isCharScroll(wchar_t inChar);
		static bool isCharNonSpacing(wchar_t inChar);
		static bool isCharSpace(wchar_t inChar);
		static bool isCharRTL(wchar_t inChar);
		void PaintBackgroundImage(const RECT& rcText, const COLORREF crBack);
		bool CheckSelection(const CONSOLE_SELECTION_INFO& select, SHORT row, SHORT col);
		//bool GetCharAttr(wchar_t ch, WORD atr, wchar_t& rch, BYTE& foreColorNum, BYTE& backColorNum, FONT* pFont);
		COLORREF* GetColors();
		int GetPaletteIndex();
		bool ChangePalette(int aNewPaletteIdx);
		void PaintVCon(HDC hPaintDc);
		bool PrintClient(HDC hPrintDc, bool bAllowRepaint, const LPRECT PaintRect);
		bool StretchPaint(HDC hPaintDC, int anX, int anY, int anShowWidth, int anShowHeight);
		void UpdateInfo();
		//void GetConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci) { mp_RCon->GetConsoleCursorInfo(ci); };
		//DWORD GetConsoleCP() { return mp_RCon->GetConsoleCP(); };
		//DWORD GetConsoleOutputCP() { return mp_RCon->GetConsoleOutputCP(); };
		//DWORD GetConsoleMode() { return mp_RCon->GetConsoleMode(); };
		//void GetConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO *sbi) { mp_RCon->GetConsoleScreenBufferInfo(sbi); };
		RECT GetRect();
		RECT GetDcClientRect();
		void OnFontChanged();
		COORD ClientToConsole(LONG x, LONG y, bool StrictMonospace=false);
		POINT ConsoleToClient(LONG x, LONG y);
		void OnConsoleSizeChanged();
		void OnConsoleSizeReset(USHORT sizeX, USHORT sizeY);
		static void ClearPartBrushes();
		HRGN GetExclusionRgn(bool abTestOnly=false);
		COORD FindOpaqueCell();
		//void ShowPopupMenu(POINT ptCur, DWORD Align = TPM_LEFTALIGN);
		//void ExecPopupMenuCmd(int nCmd);
		bool RegisterPanelView(PanelViewInit* ppvi);
		void OnPanelViewSettingsChanged();
		bool IsPanelViews();
		bool CheckTransparent();
		void OnTitleChanged();
		void SavePaneSnapshoot();
		void OnTaskbarSettingsChanged();
		void OnTaskbarFocus();

	protected:
		//inline void GetCharAttr(WORD atr, BYTE& foreColorNum, BYTE& backColorNum, HFONT* pFont);
		wchar_t* mpsz_LogScreen; DWORD mn_LogScreenIdx;
		CONSOLE_SCREEN_BUFFER_INFO csbi; DWORD mdw_LastError;
		CONSOLE_CURSOR_INFO	cinf;
		COORD winSize, coord;
		//CONSOLE_SELECTION_INFO select1, select2;
		uint TextLen;
		bool isCursorValid, drawImage, textChanged, attrChanged;
		DWORD nBgImageColors;
		COORD bgBmpSize; HDC hBgDc; // Это только ссылка, для удобства отрисовки
		void PaintVConSimple(HDC hPaintDc, RECT rcClient, BOOL bGuiVisible);
		void PaintVConNormal(HDC hPaintDc, RECT rcClient);
		void PaintVConDebug(HDC hPaintDc, RECT rcClient);
		void UpdateCursorDraw(HDC hPaintDC, RECT rcClient, COORD pos, UINT dwSize);
		bool UpdatePrepare(HDC *ahDc, MSectionLock *pSDC, MSectionLock *pSCON);
		void UpdateText();
		void PatInvertRect(HDC hPaintDC, const RECT& rect, HDC hFromDC, bool bFill);
		WORD CharWidth(wchar_t ch);
		void CharABC(wchar_t ch, ABC *abc);
		bool CheckChangedTextAttr();
		bool CheckTransparentRgn(bool abHasChildWindows);
		//void ParseLine(int row, wchar_t *ConCharLine, WORD *ConAttrLine);
		HANDLE mh_Heap;
		LPVOID Alloc(size_t nCount, size_t nSize);
		void Free(LPVOID ptr);
		MSection csCON;
		int mn_BackColorIdx; //==0
		//void Box(LPCTSTR szText);
		static char mc_Uni2Oem[0x10000];
		char Uni2Oem(wchar_t ch);
		typedef struct tag_PARTBRUSHES
		{
			wchar_t ch; // 0x2591 0x2592 0x2593 0x2588 - по увеличению плотности
			COLORREF nBackCol;
			COLORREF nForeCol;
			HBRUSH  hBrush;
		} PARTBRUSHES;
		//std::vector<PARTBRUSHES> m_PartBrushes;
		static PARTBRUSHES m_PartBrushes[MAX_COUNT_PART_BRUSHES];
		//static HBRUSH PartBrush(wchar_t ch, SHORT nBackIdx, SHORT nForeIdx);
		static HBRUSH PartBrush(wchar_t ch, COLORREF nBackCol, COLORREF nForeCol);
		bool mb_InPaintCall;
		bool mb_InConsoleResize;
		//
		bool FindChanges(int row, int &j, int &end, const wchar_t* ConCharLine, const CharAttr* ConAttrLine, const wchar_t* ConCharLine2, const CharAttr* ConAttrLine2);
		LONG nFontHeight, nFontWidth;
		BYTE nFontCharSet;
		BYTE nLastNormalBack;
		//bool bExtendFonts, bExtendColors;
		//BYTE nFontNormalColor, nFontBoldColor, nFontItalicColor, nExtendColorIdx;
		struct _TransparentInfo
		{
			INT    nRectCount;
			POINT *pAllPoints;
			INT   *pAllCounts;
		} TransparentInfo;
		//static HMENU mh_PopupMenu, mh_TerminatePopup, mh_DebugPopup, mh_EditPopup;

		struct _HighlightInfo
		{
			COORD m_Last;
			COORD m_Cur;
			// store last paint coords
			RECT  mrc_LastRow, mrc_LastCol;
			RECT  mrc_LastHyperlink;
			// true - if VCon visible & enabled in settings & highlight exist
			bool  mb_Exists;
			// true - if Invalidate was called, but UpdateHighlights still not
			bool  mb_ChangeDetected;
			bool  mb_SelfSettings;
			bool  mb_HighlightRow;
			bool  mb_HighlightCol;
		} m_HighlightInfo;
		void ResetHighlightCoords();
		void UpdateHighlights();
		void UpdateHighlightsRowCol();
		void UpdateHighlightsHyperlink();
		void UndoHighlights();
		bool CalcHighlightRowCol(COORD* pcrPos);
		bool WasHighlightRowColChanged();
		bool isHighlightAny();
		bool isHighlightMouseRow();
		bool isHighlightMouseCol();
		bool isHighlightHyperlink();
	public:
		void ChangeHighlightMouse(int nWhat, int nSwitch);
	protected:
		virtual void OnDestroy() override;
};
