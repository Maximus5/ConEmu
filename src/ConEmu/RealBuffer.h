
/*
Copyright (c) 2009-2016 Maximus5
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

#include "../common/ConsoleAnnotation.h"
#include "../common/MSection.h"
#include "RealConsole.h"

// >> Common.h
//enum RealBufferScroll
//{
//	rbs_None = 0,
//	rbs_Vert = 1,
//	rbs_Horz = 2,
//	rbs_Any  = 3,
//};

#ifndef CONSOLE_MOUSE_SELECTION
	#define CONSOLE_SELECTION_IN_PROGRESS   0x0001   // selection has begun
	#define CONSOLE_MOUSE_SELECTION         0x0004   // selecting with mouse
#endif

#ifndef WM_MOUSEHWHEEL
	#define WM_MOUSEHWHEEL                  0x020E
#endif

#include "Options.h"

class CMatch;

enum IntelligentSelectionState
{
	IS_None = 0,
	IS_LBtnDown,
	IS_LBtnReleased,
};

class CRealBuffer
{
public:
	CRealBuffer(CRealConsole* apRCon, RealBufferType aType=rbt_Primary);
	~CRealBuffer();

	RealBufferType m_Type;
public:
	void DumpConsole(HANDLE ahFile);
	bool LoadDumpConsole(LPCWSTR asDumpFile);

	bool LoadAlternativeConsole(LoadAltMode iMode = lam_Default);

	void ReleaseMem();
	void PreFillBuffers();

	bool isInResize();
	BOOL SetConsoleSize(SHORT sizeX, SHORT sizeY, USHORT sizeBuffer, DWORD anCmdID=CECMD_SETSIZESYNC);
	void SyncConsole2Window(SHORT wndSizeX, SHORT wndSizeY);

	BOOL isScroll(RealBufferScroll aiScroll=rbs_Any);
	BOOL isConsoleDataChanged();

	void InitSBI(CONSOLE_SCREEN_BUFFER_INFO* ap_sbi, BOOL abCurBufHeight);
	void InitMaxSize(const COORD& crMaxSize);
	COORD GetMaxSize();

	bool isInitialized();
	bool isFarMenuOrMacro();

	bool PreInit();
	void ResetBuffer();

	int BufferHeight(uint nNewBufferHeight=0);
	SHORT GetBufferWidth();
	SHORT GetBufferHeight();
	SHORT GetBufferPosX();
	SHORT GetBufferPosY();
	int GetTextWidth();
	int TextWidth();
	int GetTextHeight();
	int TextHeight();
	int GetWindowWidth();
	int GetWindowHeight();

	void SetBufferHeightMode(BOOL abBufferHeight, BOOL abIgnoreLock=FALSE);
	void ChangeBufferHeightMode(BOOL abBufferHeight);
	void SetChange2Size(int anChange2TextWidth, int anChange2TextHeight);

	BOOL isBuferModeChangeLocked();
	BOOL BuferModeChangeLock();
	void BuferModeChangeUnlock();
	BOOL BufferHeightTurnedOn(CONSOLE_SCREEN_BUFFER_INFO* psbi);
	void OnBufferHeight();

	LRESULT DoScrollBuffer(int nDirection, short nTrackPos = -1, UINT nCount = 1);
	void ResetTopLeft();

	BOOL ApplyConsoleInfo();

	BOOL GetConWindowSize(const CONSOLE_SCREEN_BUFFER_INFO& sbi, int* pnNewWidth, int* pnNewHeight, DWORD* pnScroll);

	COORD ScreenToBuffer(COORD crMouse);
	COORD BufferToScreen(COORD crMouse, bool bFixup = true, bool bVertOnly = false);
	bool ProcessFarHyperlink(UINT messg, COORD crFrom, bool bUpdateScreen);
	bool ProcessFarHyperlink(bool bUpdateScreen);
	void ResetHighlightHyperlinks();
	ExpandTextRangeType GetLastTextRangeType();

	void ShowKeyBarHint(WORD nID);

	bool OnMouse(UINT messg, WPARAM wParam, int x, int y, COORD crMouse);

	BOOL GetRBtnDrag(COORD* pcrMouse);
	void SetRBtnDrag(BOOL abRBtnDrag, const COORD* pcrMouse = NULL);

private:
	bool OnMouseSelection(UINT messg, WPARAM wParam, int x, int y);
	bool DoSelectionCopyInt(CECopyMode CopyMode, bool bStreamMode, int srSelection_X1, int srSelection_Y1, int srSelection_X2, int srSelection_Y2, BYTE nFormat = CTSFormatDefault, LPCWSTR pszDstFile = NULL, HGLOBAL* phUnicode = NULL);
	int  GetSelectionCharCount(bool bStreamMode, int srSelection_X1, int srSelection_Y1, int srSelection_X2, int srSelection_Y2, int* pnSelWidth, int* pnSelHeight, int nNewLineLen);
	bool PatchMouseCoords(int& x, int& y, COORD& crMouse);
	bool CanProcessHyperlink(const COORD& crMouse);
	SHELLEXECUTEINFO* mpp_RunHyperlink;

public:
	void OnTimerCheckSelection();
	void MarkFindText(int nDirection, LPCWSTR asText, bool abCaseSensitive, bool abWholeWords); // <<== CRealConsole::DoFindText
	void StartSelection(BOOL abTextMode, SHORT anX=-1, SHORT anY=-1, BOOL abByMouse=FALSE, UINT anFromMsg=0, COORD *pcrTo=NULL, DWORD anAnchorFlag = 0);
	void ChangeSelectionByKey(UINT vkKey, bool bCtrl, bool bShift);
	void ExpandSelection(SHORT anX, SHORT anY, bool bWasSelection);
	UINT CorrectSelectionAnchor();
	bool DoSelectionFinalize(bool abCopy, CECopyMode CopyMode = cm_CopySel, WPARAM wParam = 0, HGLOBAL* phUnicode = NULL);
	void DoCopyPaste(bool abCopy, bool abPaste);
	void DoSelectionStop();
	bool DoSelectionCopy(CECopyMode CopyMode = cm_CopySel, BYTE nFormat = CTSFormatDefault, LPCWSTR pszDstFile = NULL, HGLOBAL* phUnicode = NULL);
	void UpdateSelection();
	void UpdateHyperlink();
	bool isConSelectMode();
	bool isStreamSelection();

	bool OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars);
	void OnKeysSending();
	const ConEmuHotKey* ProcessSelectionHotKey(const ConEmuChord& VkState, bool bKeyDown, const wchar_t *pszChars);

	COORD GetDefaultNtvdmHeight();

	const CONSOLE_SCREEN_BUFFER_INFO* GetSBI();

	//BOOL IsConsoleDataChanged();

	BOOL GetConsoleLine(int nLine, wchar_t** pChar, /*CharAttr** pAttr,*/ int* pLen, MSectionLock* pcsData = NULL);
	void GetConsoleData(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, ConEmuTextRange& etr);

	void ResetConData();
	bool isConDataValid();

	DWORD_PTR GetKeybLayout();
	void  SetKeybLayout(DWORD_PTR anNewKeyboardLayout);

	int GetStatusLineCount(int nLeftPanelEdge);

	bool isSelectionAllowed();
	bool isSelectionPresent();
	bool isMouseSelectionPresent();
	bool isMouseInsideSelection(int x, int y);
	bool GetConsoleSelectionInfo(CONSOLE_SELECTION_INFO *sel);
	int  GetSelectionCellsCount();

	bool isPaused();
	void StorePausedState(CEPauseCmd state);

	void ConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO* psbi, SMALL_RECT* psrRealWindow = NULL, TOPLEFTCOORD* pTopLeft = NULL);
	void ConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci);
	void ConsoleCursorPos(COORD* pcr);
	void GetCursorInfo(COORD* pcr, CONSOLE_CURSOR_INFO* pci);

	bool ConsoleRect2ScreenRect(const RECT &rcCon, RECT *prcScr);

	DWORD GetConsoleCP();
	DWORD GetConsoleOutputCP();
	WORD  GetConInMode();
	WORD  GetConOutMode();

	void FindPanels();
	BOOL GetPanelRect(BOOL abRight, RECT* prc, BOOL abFull = FALSE, BOOL abIncludeEdges = FALSE);
	BOOL isLeftPanel();
	BOOL isRightPanel();

	const CRgnDetect* GetDetector();

private:
	void ApplyConsoleInfo(const CESERVER_REQ_CONINFO_INFO* pInfo, bool& bSetApplyFinished, bool& lbChanged, bool& bBufRecreate);
	BOOL SetConsoleSizeSrv(USHORT sizeX, USHORT sizeY, USHORT sizeBuffer, DWORD anCmdID=CECMD_SETSIZESYNC);
	BOOL InitBuffers(DWORD anCellCount = 0, int anWidth = 0, int anHeight = 0);
	BOOL CheckBufferSize();
	BOOL IsTrueColorerBufferChanged();
	BOOL LoadDataFromSrv(DWORD CharCount, CHAR_INFO* pData);
	bool LoadDataFromDump(const CONSOLE_SCREEN_BUFFER_INFO& storedSbi, const CHAR_INFO* pData, DWORD cchMaxCellCount);

	// координаты панелей в символах
	RECT mr_LeftPanel, mr_RightPanel, mr_LeftPanelFull, mr_RightPanelFull; BOOL mb_LeftPanel, mb_RightPanel;

	void PrepareTransparent(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight);

	ExpandTextRangeType ExpandTextRange(COORD& crFrom/*[In/Out]*/, COORD& crTo/*[Out]*/, ExpandTextRangeType etr, CEStr* psText = NULL);
	bool StoreLastTextRange(ExpandTextRangeType etr);

	short CheckProgressInConsole(const wchar_t* pszCurLine);

	void PrepareColorTable(bool bExtendFonts, CharAttr (&lcaTableExt)[0x100], CharAttr (&lcaTableOrg)[0x100], const AppSettings* pApp = NULL);

	bool ResetLastMousePos();

protected:
	CRealConsole* mp_RCon;

	/* ****************************************** */
	/* Поиск диалогов и пометка "прозрачных" мест */
	/* ****************************************** */
	CRgnDetect m_Rgn; DWORD mn_LastRgnFlags;

	BOOL mb_BuferModeChangeLocked;

	// Informational
	COORD mcr_LastMousePos;

	struct {
		MSectionSimple* pcsLock;
		AnnotationInfo* mp_Cmp;
		int nCmpMax;
	} m_TrueMode;


	MSection csCON;
	// Эти переменные инициализируются в RetrieveConsoleInfo()
	struct RConInfo
	{
		CONSOLE_SELECTION_INFO m_sel;
		DWORD m_SelClickTick, m_SelDblClickTick, m_SelLastScrollCheck;
		struct {
			IntelligentSelectionState State; // former mb_IntelliStored
			DWORD ClickTick; // To be sure if we need DblClick selection
			POINT LClickPt; // Save LBtnDown position for Intelligent selection
		} ISel; // Intelligent selection
		UINT mn_SkipNextMouseEvent; // Avoid posting LBtnUp in console, if we processed LBtnDn internally
		LONG mn_UpdateSelectionCalled;
		CONSOLE_CURSOR_INFO m_ci;
		DWORD m_dwConsoleCP, m_dwConsoleOutputCP;
		WORD m_dwConsoleInMode, m_dwConsoleOutMode;
		CONSOLE_SCREEN_BUFFER_INFO m_sbi; // srWindow - "видимое" в GUI окно
		SMALL_RECT srRealWindow; // Те реальные координаты, которые видимы в RealConsole (а не то, что видимо в GUI окне)
		COORD crMaxSize; // Максимальный размер консоли на текущем шрифте
		TOPLEFTCOORD TopLeft; // может отличаться от m_sbi.srWindow.Top, если прокрутка заблокирована
		DWORD TopLeftTick; BOOL InTopLeftSet;
		CECI_FLAGS Flags;
		DWORD FlagsUpdateTick;
		DWORD LastStartInitBuffersTick, LastEndInitBuffersTick, LastStartReadBufferTick, LastEndReadBufferTick;
		BOOL bInGetConsoleData;
		int nCreatedBufWidth, nCreatedBufHeight; // Informational
		size_t nConBufCells; // Max buffers size (cells!)
		bool mb_ConDataValid;
		wchar_t *pConChar;
		WORD  *pConAttr;
		CHAR_INFO *pDataCmp;
		// Sizes
		int nTextWidth, nTextHeight, nBufferHeight;
		// Resize (srv) in progress
		BOOL bLockChange2Text;
		int nChange2TextWidth, nChange2TextHeight;
		//
		BOOL bBufferHeight; // TRUE, если есть прокрутка
		//DWORD nPacketIdx;
		DWORD_PTR dwKeybLayout;
		BOOL bRBtnDrag; // в консоль посылается драг правой кнопкой (выделение в FAR)
		COORD crRBtnDrag;
		int DefaultBufferHeight;
		BOOL bConsoleDataChanged;
		//RClick4KeyBar
		BOOL bRClick4KeyBar;
		COORD crRClick4KeyBar;
		POINT ptRClick4KeyBar;
		int nRClickVK; // VK_F1..F12
		// Последний etr... (подсветка URL's и строк-ошибок-компиляторов)
		ConEmuTextRange etr; // etrLast, mcr_FileLineStart, mcr_FileLineEnd
		bool etrWasChanged;
	} con;
	bool SetTopLeft(int ay = -1, int ax = -1, bool abServerCall = false);

	CMatch* mp_Match;

protected:
	struct ConDumpInfo
	{
		size_t    cbDataSize;
		LPBYTE    ptrData;
		wchar_t*  pszTitle;
		COORD     crSize;
		COORD     crCursor;
		//BOOL      NeedApply;
		// ** Block 1 **
		BOOL      Block1;
		wchar_t*  pszBlock1;
		CharAttr* pcaBlock1;
		// ** Block 2 **
		// ** Block 3 **
		// TODO: Block 2, 3

		// *************
		void Close()
		{
			SafeFree(ptrData);
			memset(this, 0, sizeof(*this));
		}
	} dump;
};
