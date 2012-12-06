

/*
Copyright (c) 2009-2012 Maximus5
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

#include "../Common/ConsoleAnnotation.h"
#include "RealConsole.h"

// >> common.hpp
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

#include "Options.h"

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
	
	BOOL SetConsoleSize(USHORT sizeX, USHORT sizeY, USHORT sizeBuffer, DWORD anCmdID=CECMD_SETSIZESYNC);
	void SyncConsole2Window(USHORT wndSizeX, USHORT wndSizeY);
	
	BOOL isScroll(RealBufferScroll aiScroll=rbs_Any);
	BOOL isConsoleDataChanged();
	
	void InitSBI(CONSOLE_SCREEN_BUFFER_INFO* ap_sbi);
	void InitMaxSize(const COORD& crMaxSize);
	COORD GetMaxSize();
	
	bool isInitialized();
	bool isFarMenuOrMacro();
	
	BOOL PreInit();
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

	LRESULT OnScroll(int nDirection, short nTrackPos = -1, UINT nCount = 1);
	LRESULT OnSetScrollPos(WPARAM wParam);
	
	BOOL ApplyConsoleInfo();
	
	BOOL GetConWindowSize(const CONSOLE_SCREEN_BUFFER_INFO& sbi, int* pnNewWidth, int* pnNewHeight, DWORD* pnScroll);
	
	COORD ScreenToBuffer(COORD crMouse);
	COORD BufferToScreen(COORD crMouse, bool bVertOnly = false);
	bool ProcessFarHyperlink(UINT messg, COORD crFrom);
	bool ProcessFarHyperlink(UINT messg=WM_USER);
	ExpandTextRangeType GetLastTextRangeType();
	
	void ShowKeyBarHint(WORD nID);

	bool OnMouse(UINT messg, WPARAM wParam, int x, int y, COORD crMouse, bool abFromTouch = false);
	
	BOOL GetRBtnDrag(COORD* pcrMouse);
	void SetRBtnDrag(BOOL abRBtnDrag, const COORD* pcrMouse = NULL);

	bool OnMouseSelection(UINT messg, WPARAM wParam, int x, int y);
	void MarkFindText(int nDirection, LPCWSTR asText, bool abCaseSensitive, bool abWholeWords);
	void StartSelection(BOOL abTextMode, SHORT anX=-1, SHORT anY=-1, BOOL abByMouse=FALSE, UINT anFromMsg=0, COORD *pcrTo=NULL);
	void ExpandSelection(SHORT anX=-1, SHORT anY=-1);
	bool DoSelectionFinalize(bool abCopy, WPARAM wParam = 0);
	void DoSelectionStop();
	bool DoSelectionCopy();
	void UpdateSelection();
	BOOL isConSelectMode();
	BOOL isSelfSelectMode();
	BOOL isStreamSelection();
	bool OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars);
	
	COORD GetDefaultNtvdmHeight();
	
	const CONSOLE_SCREEN_BUFFER_INFO* GetSBI();
	
	//BOOL IsConsoleDataChanged();
	
	BOOL GetConsoleLine(int nLine, wchar_t** pChar, /*CharAttr** pAttr,*/ int* pLen, MSectionLock* pcsData = NULL);
	void GetConsoleData(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight);
	
	DWORD_PTR GetKeybLayout();
	void  SetKeybLayout(DWORD_PTR anNewKeyboardLayout);
	DWORD GetConMode();
	
	int GetStatusLineCount(int nLeftPanelEdge);
	
	bool isSelectionAllowed();
	bool isSelectionPresent();
	bool GetConsoleSelectionInfo(CONSOLE_SELECTION_INFO *sel);
	
	void ConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO* sbi);
	void ConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci);
	void ConsoleCursorPos(COORD* pcr);
	void GetCursorInfo(COORD* pcr, CONSOLE_CURSOR_INFO* pci);
	
	bool ConsoleRect2ScreenRect(const RECT &rcCon, RECT *prcScr);

	DWORD GetConsoleCP();
	DWORD GetConsoleOutputCP();

	void FindPanels();
	BOOL GetPanelRect(BOOL abRight, RECT* prc, BOOL abFull = FALSE, BOOL abIncludeEdges = FALSE);
	BOOL isLeftPanel();
	BOOL isRightPanel();

	bool LookupFilePath(LPCWSTR asFileOrPath, wchar_t* pszPath, size_t cchPathMax);

	const CRgnDetect* GetDetector();
	
private:
	BOOL SetConsoleSizeSrv(USHORT sizeX, USHORT sizeY, USHORT sizeBuffer, DWORD anCmdID=CECMD_SETSIZESYNC);
	BOOL InitBuffers(DWORD OneBufferSize);
	BOOL CheckBufferSize();
	BOOL IsTrueColorerBufferChanged();
	BOOL LoadDataFromSrv(DWORD CharCount, CHAR_INFO* pData);
	bool LoadDataFromDump(const CONSOLE_SCREEN_BUFFER_INFO& storedSbi, const CHAR_INFO* pData, DWORD cchMaxCellCount);

	// координаты панелей в символах
	RECT mr_LeftPanel, mr_RightPanel, mr_LeftPanelFull, mr_RightPanelFull; BOOL mb_LeftPanel, mb_RightPanel;
	
	void PrepareTransparent(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight);
	

	ExpandTextRangeType ExpandTextRange(COORD& crFrom/*[In/Out]*/, COORD& crTo/*[Out]*/, ExpandTextRangeType etr, wchar_t* pszText = NULL, size_t cchTextMax = 0);
	bool FindRangeStart(COORD& crFrom/*[In/Out]*/, COORD& crTo/*[In/Out]*/, bool& bUrlMode, LPCWSTR pszBreak, LPCWSTR pszUrlDelim, LPCWSTR pszSpacing, LPCWSTR pszUrl, LPCWSTR pszProtocol, LPCWSTR pChar, int nLen);
	bool CheckValidUrl(COORD& crFrom/*[In/Out]*/, COORD& crTo/*[In/Out]*/, bool& bUrlMode, LPCWSTR pszUrlDelim, LPCWSTR pszUrl, LPCWSTR pszProtocol, LPCWSTR pChar, int nLen);
	void StoreLastTextRange(ExpandTextRangeType etr);

	short CheckProgressInConsole(const wchar_t* pszCurLine);

	void PrepareColorTable(bool bExtendFonts, CharAttr (&lcaTableExt)[0x100], CharAttr (&lcaTableOrg)[0x100], const Settings::AppSettings* pApp = NULL);

	void ResetLastMousePos();

protected:
	CRealConsole* mp_RCon;
	
	/* ****************************************** */
	/* Поиск диалогов и пометка "прозрачных" мест */
	/* ****************************************** */
	CRgnDetect m_Rgn; DWORD mn_LastRgnFlags;

	BOOL mb_BuferModeChangeLocked;
	COORD mcr_LastMousePos;

	struct {
		CRITICAL_SECTION csLock;
		AnnotationInfo* mp_Cmp;
		int nCmpMax;
	} m_TrueMode;

	
	MSection csCON;
	// Эти переменные инициализируются в RetrieveConsoleInfo()
	struct RConInfo
	{
		CONSOLE_SELECTION_INFO m_sel;
		DWORD m_SelClickTick, m_SelDblClickTick;
		CONSOLE_CURSOR_INFO m_ci;
		DWORD m_dwConsoleCP, m_dwConsoleOutputCP, m_dwConsoleMode;
		CONSOLE_SCREEN_BUFFER_INFO m_sbi;
		COORD crMaxSize; // Максимальный размер консоли на текущем шрифте
		USHORT nTopVisibleLine; // может отличаться от m_sbi.srWindow.Top, если прокрутка заблокирована
		wchar_t *pConChar;
		WORD  *pConAttr;
		COORD mcr_FileLineStart, mcr_FileLineEnd; // Подсветка строк ошибок компиляторов
		//CESERVER_REQ_CONINFO_DATA *pCopy, *pCmp;
		CHAR_INFO *pDataCmp;
		int nTextWidth, nTextHeight, nBufferHeight;
		BOOL bLockChange2Text;
		int nChange2TextWidth, nChange2TextHeight;
		BOOL bBufferHeight; // TRUE, если есть прокрутка
		//DWORD nPacketIdx;
		DWORD_PTR dwKeybLayout;
		BOOL bRBtnDrag; // в консоль посылается драг правой кнопкой (выделение в FAR)
		COORD crRBtnDrag;
		BOOL bInSetSize; HANDLE hInSetSize;
		int DefaultBufferHeight;
		BOOL bConsoleDataChanged;
		//RClick4KeyBar
		BOOL bRClick4KeyBar;
		COORD crRClick4KeyBar;
		POINT ptRClick4KeyBar;
		int nRClickVK; // VK_F1..F12
		// Последний etr...
		ExpandTextRangeType etrLast;
	} con;
	
protected:
	struct ConDumpInfo
	{
		size_t    cbDataSize;
		LPBYTE    ptrData;
		wchar_t*  pszTitle;
		COORD     crSize, crCursor;
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
