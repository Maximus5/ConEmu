
/*
Copyright (c) 2009-2017 Maximus5
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

#include "../common/ConEmuCheck.h"
#include "../common/MSection.h"
#include "VConRelease.h"

#include "Options.h"
#include "FontPtr.h"
#include "RealConsole.h"
#include "VConChild.h"
#include "CustomFonts.h"
#include "SetColorPalette.h"

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
	vf_Maximized = 0x0008,
	vf_GroupSplit= 0x0010, // simultaneous input for active split group
	vf_GroupSet  = 0x0020, // simultaneous input for selected consoles
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
		bool SetFlags(VConFlags Set, VConFlags Mask, int index = -1);

	private:
		int            mn_Index;   // !!! Debug, Informational !!!
		wchar_t        ms_Idx[16]; // !!! Debug, Informational !!!
		int            mn_ID;      // !!! Debug, Unique per session id of VCon !!!
		wchar_t        ms_ID[16];  // !!! Debug, Informational !!!

	protected:
		struct VConRConSizes
		{
			uint TextWidth, TextHeight; // размер в символах
			uint Width, Height; // размер в пикселях
			uint nMaxTextWidth, nMaxTextHeight; // размер в символах
			uint LastPadSize;
			LONG nFontHeight, nFontWidth;
		} m_Sizes;


		//WARNING: Move to CFontCache object
		bool    isFontSizeChanged;


	protected:
		CVirtualConsole(CConEmuMain* pOwner, int index);
		bool Constructor(RConStartArgs *args);
	public:
		void InitGhost();
	protected:
		virtual ~CVirtualConsole();
		friend class CVConRelease;
	public:
		CConEmuMain* Owner();
		CRealConsole* RCon();
		HWND GuiWnd();
		void setFocus();
		HWND GhostWnd();
		bool isActive(bool abAllowGroup);
		bool isVisible();
		bool isGroup();
		EnumVConFlags isGroupedInput();
	public:
		int Index();
		LPCWSTR IndexStr();
		int ID();
		LPCWSTR IDStr();
	public:
		bool LoadConsoleData();
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
		bool    mb_PaintSkippedLogged;
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
		CEFontStyles m_SelectedFont;
		//CEFONT  mh_FontByIndex[MAX_FONT_STYLES_EX]; // pointers to Normal/Bold/Italic/Bold&Italic/...Underline
		CFontPtr m_UCharMapFont;
		SMALL_RECT mrc_UCharMap;
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
		void ResetOnStart();
		bool GetUCharMapFontPtr(CFontPtr& pFont);
	private:
		// Working pointers
		bool mb_PointersAllocated;
		wchar_t  *mpsz_ConChar, *mpsz_ConCharSave;   // nMaxTextWidth * nMaxTextHeight
		// CharAttr определен в "common/RgnDetect.h"
		CharAttr *mpn_ConAttrEx, *mpn_ConAttrExSave; // nMaxTextWidth * nMaxTextHeight
		DWORD *ConCharX;      // nMaxTextWidth * nMaxTextHeight
		bool *pbLineChanged;  // nMaxTextHeight
		bool *pbBackIsPic;    // nMaxTextHeight :: заполняется если *pbLineChanged
		COLORREF* pnBackRGB;  // nMaxTextHeight :: заполняется если *pbLineChanged и НЕ *pbBackIsPic
		ConEmuTextRange m_etr;// Подсветка URL's и строк-ошибок-компиляторов

		// функции выделения памяти
		void PointersFree();
		bool PointersAlloc();
		void PointersZero();

		// PanelViews
		PanelViewInit m_LeftPanelView, m_RightPanelView;
		// Set to `true`, if we have to update FarPanel sized on next Redraw
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


		const AppSettings* mp_Set;

	public:
		bool isEditor, isViewer, isFilePanel, isFade, isForeground;
		BYTE attrBackLast;
		COLORREF *mp_Colors;

		// In some cases (Win+G attach of external console)
		// we use original RealConsole palette instead of ConEmu's default one
		ColorPalette m_SelfPalette;
		void SetSelfPalette(WORD wAttributes, WORD wPopupAttributes, const COLORREF (&ColorTable)[16]);

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
		static bool UpdateCursorGroup(CVirtualConsole* pVCon, LPARAM lParam);
		CECursorType GetCursor(bool bActive);
		void UpdateThumbnail(bool abNoSnapshot = FALSE);
		void SelectFont(CEFontStyles newFont);
		void SelectBrush(HBRUSH hNew);
		void PaintBackgroundImage(const RECT& rcText, const COLORREF crBack);
		bool CheckSelection(const CONSOLE_SELECTION_INFO& select, SHORT row, SHORT col);
		//bool GetCharAttr(wchar_t ch, WORD atr, wchar_t& rch, BYTE& foreColorNum, BYTE& backColorNum, FONT* pFont);
		COLORREF* GetColors();
		COLORREF* GetColors(bool bFade);
		int GetPaletteIndex();
		bool ChangePalette(int aNewPaletteIdx);
		void PaintVCon(HDC hPaintDc);
		bool PrintClient(HDC hPrintDc, bool bAllowRepaint, const LPRECT PaintRect);
		bool Blit(HDC hPaintDC, int anX, int anY, int anShowWidth, int anShowHeight);
		bool StretchPaint(HDC hPaintDC, int anX, int anY, int anShowWidth, int anShowHeight);
		void UpdateInfo();
		LONG GetVConWidth();
		LONG GetVConHeight();
		LONG GetTextWidth();
		LONG GetTextHeight();
		SIZE GetCellSize();
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
		bool RegisterPanelView(PanelViewInit* ppvi);
		void OnPanelViewSettingsChanged();
		bool IsPanelViews();
		bool CheckTransparent();
		void OnTitleChanged();
		void SavePaneSnapshot();
		void OnTaskbarSettingsChanged();
		void OnTaskbarFocus();

		void OnAppSettingsChanged(int iAppId = -1);
		LONG mn_AppSettingsChangCount;

	protected:
		wchar_t* mpsz_LogScreen; DWORD mn_LogScreenIdx;
		CONSOLE_SCREEN_BUFFER_INFO csbi; DWORD mdw_LastError;
		CONSOLE_CURSOR_INFO	cinf;
		COORD winSize, coord;
		uint TextLen;
		bool isCursorValid, drawImage, textChanged, attrChanged;
		DWORD nBgImageColors;
		COORD bgBmpSize; HDC hBgDc; // Это только ссылка, для удобства отрисовки
		void PaintVConSimple(HDC hPaintDc, RECT rcClient, bool bGuiVisible);
		void PaintVConNormal(HDC hPaintDc, RECT rcClient);
		void PaintVConDebug(HDC hPaintDc, RECT rcClient);
		void UpdateCursorDraw(HDC hPaintDC, RECT rcClient, COORD pos, UINT dwSize);
		bool UpdatePrepare(HDC *ahDc, MSectionLock *pSDC, MSectionLock *pSCON);
		void UpdateText();
		void PatInvertRect(HDC hPaintDC, const RECT& rect, HDC hFromDC, bool bFill);
		WORD CharWidth(wchar_t ch, const CharAttr& attr);
		void SelectFont(const CFontPtr& hFontPtr);
		#if 0
		void CharABC(wchar_t ch, ABC *abc);
		#endif
		bool CheckChangedTextAttr();
		bool CheckTransparentRgn(bool abHasChildWindows);
		HANDLE mh_Heap;
		LPVOID Alloc(size_t nCount, size_t nSize);
		void Free(LPVOID ptr);
		MSection csCON;
		int mn_BackColorIdx; //==0
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
		bool FindChanges(int row, const wchar_t* ConCharLine, const CharAttr* ConAttrLine, const wchar_t* ConCharLine2, const CharAttr* ConAttrLine2);
		//bool bExtendFonts, bExtendColors;
		//BYTE nFontNormalColor, nFontBoldColor, nFontItalicColor, nExtendColorIdx;
		struct _TransparentInfo
		{
			INT    nRectCount;
			POINT *pAllPoints;
			INT   *pAllCounts;
		} TransparentInfo;
		//static HMENU mh_PopupMenu, mh_TerminatePopup, mh_DebugPopup, mh_EditPopup;

		struct HighlightRect
		{
			COLORREF crPatColor;
			DWORD    StockBrush;
			RECT     rcPaint;
		};
		struct _HighlightInfo
		{
			COORD m_Last;
			COORD m_Cur;
			// store last paint coords
			MArray<HighlightRect> LastRect;
			//RECT  mrc_LastRow, mrc_LastCol;
			//RECT  mrc_LastHyperlink;
			// true - if VCon visible & enabled in settings & highlight exist
			bool  mb_Exists;
			// true - if Invalidate was called, but UpdateHighlights still not
			bool  mb_ChangeDetected;
			bool  mb_SelfSettings;
			bool  mb_HighlightRow;
			bool  mb_HighlightCol;

			// required in VC12 coz MArray
			_HighlightInfo()
				: m_Last(), m_Cur()
				, mb_Exists(), mb_ChangeDetected()
				, mb_SelfSettings(), mb_HighlightRow(), mb_HighlightCol()
			{
			};
		} m_HighlightInfo;
		void ResetHighlightCoords();
		void ResetHighlightHyperlinks();
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
