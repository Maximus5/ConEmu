
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

#define MAX_DETECTED_DIALOGS 20

#include <pshpack1.h>
typedef struct tag_CharAttr
{
	TODO("OPTIMIZE: Заменить бы битовые поля на один DWORD, в котором хранить некий общий ИД стиля, заполняемый при формировании буфера");
	union
	{
		// Собственно цвета/шрифты
		struct
		{
			// *** Begin of All comparison
			unsigned int crForeColor : 24; // чтобы в ui64 поместился и nFontIndex
			unsigned int nFontIndex : 8; // 0=normal, or combination {1=bold,2=italic,4=underline}, or 8=UCharMap, others may be used internally in VCon (alternative font)
			unsigned int crBackColor : 24; // чтобы в ui64 поместился и Flags
			#define CharAttr_DialogPanel    1 // Far Manager panels
			#define CharAttr_DialogRect     2 // Flying dialog (over Panels or Editor/Viewer)
			#define CharAttr_DialogVBorder  4 // Left or Right dialog edge
			#define CharAttr_Transparent   16 // This cell may be marked as transparent (User Screen Transparency)
			unsigned int Flags : 8; // Warning! These flags are used in "All" comparison
			// *** End of All comparison
			#define CharAttr2_NonSpacing    1 // low-surrogate pair
			#define CharAttr2_Combining     2 // combining characters, drawn over previous one
			#define CharAttr2_DoubleSpaced  4 // DBCS glyphs used full (double) space
			#define CharAttr2_DialogCorner  8
			#define CharAttr2_Surrogate    16 // high-surrogate pair
			unsigned int Flags2 : 8; // вспомогательные флаги CharAttr_XXX
			unsigned int nForeIdx : 8; // Only 5 bits are used at the moment (0..15 base colors + 15..31 extended colors)
			unsigned int nBackIdx : 8; // Used to ExtendColors and ExtendFonts
			unsigned int crOrigForeColor : 24;
			unsigned int crOrigBackColor : 24; // Реальные цвета в консоли, crForeColor и crBackColor могут быть изменены колорером
			unsigned int ConAttr : 16; // original console attribute from CHAR_INFO
			//bool bDialog;
			//bool bDialogVBorder;
			//bool bDialogCorner;
			//bool bSomeFilled;
			//bool bTransparent; // UserScreen
			//unsigned int bDialog : 1;
			//unsigned int bDialogVBorder : 1;
			//unsigned int bDialogCorner : 1;
			//unsigned int bSomeFilled : 1;
			//unsigned int bTransparent : 1; // UserScreen
		};
		// А это для сравнения (поиск изменений)
		unsigned __int64 All;
		// для сравнения, когда фон не важен
		unsigned int ForeFont;
	};
	//
	//DWORD dwAttrubutes; // может когда понадобятся дополнительные флаги...
	//
	///**
	// * Used exclusively by ConsoleView to append annotations to each character
	// */
	//AnnotationInfo annotationInfo;
} CharAttr;
#include <poppack.h>

inline bool operator==(const CharAttr& s1, const CharAttr& s2)
{
	return s1.All == s2.All;
}


#define FR_FLAGS_MASK     0x00FF0000
#define FR_COMMONDLG_MASK 0x000000FF
#define FR_FREEDLG_MASK   0x0000FF00
#define FR_ALLDLG_MASK    (FR_COMMONDLG_MASK|FR_FREEDLG_MASK)
// Предопределенные ИД "регионов"
#define FR_LEFTPANEL      0x00000001 // Левая панель
#define FR_RIGHTPANEL     0x00000002 // Правая панель
#define FR_FULLPANEL      0x00000004 // Одна из панелей растянутая на весь экран
#define FR_MENUBAR        0x00000008 // Строка меню (верхнее)
#define FR_ACTIVEMENUBAR  (0x0000010|FR_MENUBAR) // Если MenuBar виден не всегда, или он активирован (т.е. панели недоступны)
#define FR_PANELTABS      0x00000020 // Строка под панелями (плагин PanelTabs)
#define FR_QSEARCH        0x00000040 // QSearch в панелях
#define FR_VIEWEREDITOR   0x00000080 // Редактор или просмотрщик
// ИД для свободных диалогов/меню/и пр.
#define FR_FIRSTDLGID     0x00000100
#define FR_LASTDLGID      0x0000FF00
// Дополнительные флаги
#define FR_ERRORCOLOR     0x00010000 // "Красненькие" диалоги
#define FR_MACRORECORDING 0x00020000 // Красная "R" или "MACRO" в левом верхнем углу
#define FR_HASBORDER      0x00040000 // Этот прямоугольник (регион) имеет рамку
#define FR_HASEXTENSION   0x00080000 // Вокруг прямоугольника диалога есть еще окантовка
// Идентификация некоторых дополнительных регионов
#define FR_UCHARMAP       0x01000000 // Содержимое диалога "Unicode CharMap"
#define FR_UCHARMAPGLYPH  0x02000000 // Блок символов в "Unicode CharMap"


typedef struct tag_DetectedDialog
{
	int   Count;
	DWORD AllFlags;
	SMALL_RECT Rects[MAX_DETECTED_DIALOGS];
	//bool bWasFrame[MAX_DETECTED_DIALOGS];
	DWORD DlgFlags[MAX_DETECTED_DIALOGS];
} DetectedDialogs;


class CRgnDetect
{
	public:
		// Initializers
		CRgnDetect();
		~CRgnDetect();

	public:
		// Public methods
		int GetDetectedDialogs(int anMaxCount, SMALL_RECT* rc, DWORD* rf, DWORD anMask=-1, DWORD anTest=-1) const;
		DWORD GetDialog(DWORD nDlgID, SMALL_RECT* rc) const;
		void PrepareTransparent(const CEFAR_INFO_MAPPING *apFarInfo, const COLORREF *apColors, const CONSOLE_SCREEN_BUFFER_INFO *apSbi, wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight);
		DWORD GetFlags() const;
		// Methods for plugins
		void PrepareTransparent(const CEFAR_INFO_MAPPING *apFarInfo, const COLORREF *apColors);
		void OnWindowSizeChanged();
		void OnWriteConsoleOutput(const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion, const COLORREF *apColors);
		BOOL InitializeSBI(const COLORREF *apColors);
		void SetFarRect(SMALL_RECT *prcFarRect);
		BOOL GetCharAttr(int x, int y, wchar_t& rc, CharAttr& ra);
		// Sizes
		int TextWidth();
		int TextHeight();

		//#ifdef _DEBUG
		const DetectedDialogs *GetDetectedDialogsPtr() const;
		//#endif

		void SetNeedTransparency(bool abNeed);

	protected:
		// Private methods
		bool DetectDialog(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int nFromX, int nFromY, int *pnMostRight=NULL, int *pnMostBottom=NULL);
		bool FindDialog_TopLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder);
		bool FindDialog_TopRight(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder);
		bool FindDialog_Left(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder);
		bool FindDialog_Right(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder);
		bool FindDialog_Any(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder);
		bool FindDialog_Inner(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY);
		bool FindFrame_TopLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nFrameX, int &nFrameY);
		bool FindFrameTop_ByRight(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostTop);
		bool FindFrameTop_ByLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostTop);
		bool FindFrameBottom_ByRight(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostBottom);
		bool FindFrameBottom_ByLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostBottom);
		bool FindFrameRight_ByTop(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight);
		bool FindFrameRight_ByBottom(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight);
		bool FindFrameLeft_ByTop(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostLeft);
		bool FindFrameLeft_ByBottom(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostLeft);
		// Последний шанс
		bool FindByBackground(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder);
		// Сервисная
		bool ExpandDialogFrame(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int nFrameX, int nFrameY, int &nMostRight, int &nMostBottom);
		int  MarkDialog(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int nX1, int nY1, int nX2, int nY2, UINT bMarkBorder = 0, bool bFindExterior = true, DWORD nFlags = -1);
		bool ConsoleRect2ScreenRect(const RECT &rcCon, RECT *prcScr);


	protected:
		// Members
		const CEFAR_INFO_MAPPING *mp_FarInfo;
		const COLORREF *mp_Colors;
		CONSOLE_SCREEN_BUFFER_INFO m_sbi;
		bool   mb_BufferHeight;
		bool   mb_NeedTransparency;

		DWORD   /*mn_AllFlags,*/ mn_NextDlgId;
		BOOL    mb_NeedPanelDetect;
		SMALL_RECT mrc_LeftPanel, mrc_RightPanel;

		int     mn_DetectCallCount;
		//struct {
		//	int Count;
		//	SMALL_RECT Rects[MAX_DETECTED_DIALOGS];
		//	//bool bWasFrame[MAX_DETECTED_DIALOGS];
		//	DWORD DlgFlags[MAX_DETECTED_DIALOGS];
		//}
		DetectedDialogs m_DetectedDialogs;
		DWORD mn_AllFlagsSaved; // чтобы не драться с другими нитями во время детекта

	protected:
		// Используется для собственноручного формирования буферов
		wchar_t   *mpsz_Chars;
		CharAttr  *mp_Attrs;
		CharAttr  *mp_AttrsWork;
		SMALL_RECT mrc_FarRect;
		int mn_CurWidth, mn_CurHeight, mn_MaxCells;
		bool mb_SBI_Loaded;
		CharAttr mca_Table[0x100];
		bool mb_TableCreated;

	protected:
		// Доп.переменные
		int nUserBackIdx;
		COLORREF crUserBack;
		int nMenuBackIdx;
		COLORREF crMenuTitleBack;
		COLORREF crPanelsBorderBack, crPanelsBorderFore;
		COLORREF crPanelsNumberBack, crPanelsNumberFore;
		int nDlgBorderBackIdx, nDlgBorderForeIdx;
		int nErrBorderBackIdx, nErrBorderForeIdx;
		// Для детекта наличия PanelTabs (цвет кнопки [+])
		int nPanelTabsBackIdx;
		int nPanelTabsForeIdx;
		BOOL bPanelTabsSeparate;
		// При bUseColorKey Если панель погашена (или панели) то
		// 1. UserScreen под ним заменяется на crColorKey
		// 2. а текст - на пробелы
		// Проверять наличие KeyBar по настройкам (Keybar + CmdLine)
		bool bShowKeyBar;
		int nBottomLines;
		// Проверять наличие MenuBar по настройкам
		// Или может быть меню сейчас показано?
		// 1 - при видимом сейчас или постоянно меню
		bool bAlwaysShowMenuBar;
		int nTopLines;
};

//#include <pshpack1.h>
class CRgnRects
{
	public:
		int nRectCount;
#define MAX_RGN_RECTS MAX_DETECTED_DIALOGS // 20.
		RECT  rcRect[MAX_RGN_RECTS]; // rcRect[0] - основной, rcRect[1...] - то что вычитается из rcRect[0]
		DWORD nRectOper[MAX_RGN_RECTS]; // RGN_AND or RGN_DIFF
		/*	Current region state:
			#define ERROR               0
			#define NULLREGION          1
			#define SIMPLEREGION        2
			#define COMPLEXREGION       3
			#define RGN_ERROR ERROR
		*/	int nRgnState;

		CRgnRects();
		~CRgnRects();

		// Сброс всего в NULLREGION
		void Reset();
		// Сбросить все прямоугольники и установить rcRect[0]
		void Init(LPRECT prcInit);
		// Combines the parts of rcRect[..] that are not part of prcAddDiff.
		int Diff(LPRECT prcAddDiff); // RGN_AND or RGN_DIFF
		int DiffSmall(SMALL_RECT *prcAddDiff); // RGN_AND or RGN_DIFF
		// Скопировать ИЗ pRgn, вернуть true - если были отличия
		bool LoadFrom(CRgnRects* pRgn);

		/*	Service variables for nRgnState testing */
		int nFieldMaxCells, nFieldWidth, nFieldHeight;
		bool* pFieldCells;
};
//#include <poppack.h>
