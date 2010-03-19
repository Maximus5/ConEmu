
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

#include <commctrl.h>

#define MIN_ALPHA_VALUE 40
#define MAX_FONT_STYLES 8 //normal/bold|italic|underline


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
} MYRGB, MYCOLORREF;
#include <poppack.h>



class CSettings
{
public:
    CSettings();
    ~CSettings();

    wchar_t Config[MAX_PATH], Type[16];
	wchar_t szFontError[512];

    int DefaultBufferHeight;
	bool bForceBufferHeight; int nForceBufferHeight;
	bool AutoScroll; bool AutoBufferHeight;
	//bool FarSyncSize;
	int nCmdOutputCP;
    
	LONG FontWidth();
	LONG FontHeight();
	LONG BorderFontWidth();
	BYTE FontCharSet();
	BOOL FontItalic();
	BOOL FontClearType();
private:
    LOGFONT LogFont, LogFont2;
	LONG mn_AutoFontWidth, mn_AutoFontHeight; // размеры шрифтов, которые были запрошены при авторесайзе шрифта
	LONG mn_FontWidth, mn_FontHeight, mn_BorderFontWidth; // реальные размеры шрифтов
	BYTE mn_LoadFontCharSet; // То что загружено изначально (или уже сохранено в реестр)
	TEXTMETRIC tm[MAX_FONT_STYLES];
	LPOUTLINETEXTMETRIC otm[MAX_FONT_STYLES];
    BOOL mb_Name1Ok, mb_Name2Ok;
	void ResetFontWidth();
	void SaveFontSizes(LOGFONT *pCreated, bool bAuto);
	LPOUTLINETEXTMETRIC LoadOutline(HDC hDC, HFONT hFont);
	void DumpFontMetrics(LPCWSTR szType, HDC hDC, HFONT hFont, LPOUTLINETEXTMETRIC lpOutl = NULL);
public:
	bool isFontAutoSize;
	bool isAutoRegisterFonts;
	//wchar_t FontFile[MAX_PATH];
	LOGFONT ConsoleFont;
	COLORREF* GetColors(BOOL abFade = FALSE);
	COLORREF GetFadeColor(COLORREF cr);
	bool isUserScreenTransparent;
	bool NeedDialogDetect();
	//COLORREF ColorKey;
    bool isExtendColors;
    char nExtendColor;
    bool isExtendFonts, isTrueColorer;
    char nFontNormalColor, nFontBoldColor, nFontItalicColor;
    
    /* Background image */
    WCHAR sBgImage[MAX_PATH];
    char isShowBgImage;
	bool isBackgroundImageValid;
	u8 bgImageDarker;
	DWORD nBgImageColors;

    /* Transparency */
    u8 nTransparent;

	/* Command Line History (from start dialog) */
	LPWSTR psCmdHistory; DWORD nCmdHistorySize;

    /* Command Line (Registry) */
    LPTSTR psCmd;
    /* Command Line ("/cmd" arg) */
    LPTSTR psCurCmd;
	private:
	/* 'Default' command line (if nor Registry, nor /cmd specified) */
	WCHAR  szDefCmd[10];
	public:
    /* "Active" command line */
    LPCTSTR GetCmd();
	/* "Default" command line "far/cmd", based on /BufferHeight switch */
	LPCTSTR GetDefaultCmd();
    /* OUR(!) startup info */
    STARTUPINFOW ourSI;
    /* If Attach to PID requested */
    DWORD nAttachPID; HWND hAttachConWnd;

    DWORD FontSizeX;  // ширина основного шрифта
    DWORD FontSizeX2; // ширина для FixFarBorders (ширина создаваемого шрифта для отрисовки рамок, не путать со знакоместом)
    DWORD FontSizeX3; // ширина знакоместа при моноширном режиме (не путать с FontSizeX2)
    bool isFullScreen, isHideCaption;
    bool isHideCaptionAlways();
	BYTE nHideCaptionAlwaysFrame;
	DWORD nHideCaptionAlwaysDelay, nHideCaptionAlwaysDisappear;
    bool isAlwaysOnTop, isDesktopMode;
    char isFixFarBorders;
	bool isMouseSkipActivation, isMouseSkipMoving;
	bool isFarHourglass; DWORD nFarHourglassDelay;
protected:
	bool mb_HideCaptionAlways;
	typedef struct tag_CharRanges {
		bool bUsed;
		wchar_t cBegin, cEnd;
	} CharRanges;
	wchar_t mszCharRanges[120];
	CharRanges icFixFarBorderRanges[10];
	bool *mpc_FixFarBorderValues;
	BYTE m_isKeyboardHooks;
public:
	bool isKeyboardHooks();
	bool isCharBorder(wchar_t inChar);
    BYTE isPartBrush75, isPartBrush50, isPartBrush25;
    bool isCursorV;
	bool isCursorBlink;
    bool isCursorColor;
	bool isCursorBlockInactive;
    char isRClickSendKey;
    wchar_t *sRClickMacro;
    bool isSentAltEnter;
    bool isMinToTray;
    //bool isForceMonospace, isProportional;
	BYTE isMonospace, isMonospaceSelected; // 0 - proportional, 1 - monospace, 2 - forcemonospace
    bool isUpdConHandle;
	bool isRSelFix;
	//Drag
    BYTE isDragEnabled;
    BYTE isDropEnabled;
    DWORD nLDragKey, nRDragKey;
    bool isDefCopy;
	BYTE isDragOverlay;
	bool isDragShowIcons;
	BYTE isDragPanel; // изменение размера панелей мышкой
    //bool
    bool isDebugSteps;
    bool isEnhanceGraphics; // Progressbars and scrollbars (pseudographics)
    bool isFadeInactive;
    //DWORD nFadeInactiveMask;
    char isTabs; bool isTabSelf, isTabRecent, isTabLazy;
    wchar_t sTabFontFace[LF_FACESIZE]; DWORD nTabFontCharSet; int nTabFontHeight;
	wchar_t *sTabCloseMacro;
	int nToolbarAddSpace;
    DWORD wndWidth, wndHeight, ntvdmHeight; // в символах
    int wndX, wndY; // в пикселях
    bool wndCascade;
    DWORD nSlideShowElapse;
    DWORD nIconID;
    bool isTryToCenter;
    RECT rcTabMargins;
    bool isTabFrame;
    bool isMulti; BYTE icMultiNew, icMultiNext, icMultiRecreate, icMultiBuffer;
    bool IsHostkey(WORD vk);
	bool IsHostkeySingle(WORD vk);
	bool IsHostkeyPressed();
	WORD GetPressedHostkey();
    bool isMultiNewConfirm;
    bool isFARuseASCIIsort, isFixAltOnAltTab;

    // Заголовки табов
    WCHAR szTabConsole[32];
    WCHAR szTabEditor[32];
    WCHAR szTabEditorModified[32];
    WCHAR szTabViewer[32];
    DWORD nTabLenMax;

    //bool isVisualizer;
    //char nVizNormal, nVizFore, nVizTab, nVizEOL, nVizEOF;
    //wchar_t cVizTab, cVizEOL, cVizEOF;

    char isAllowDetach;
    bool isCreateAppWindow; 
    /*bool isScrollTitle;
    DWORD ScrollTitleLen;*/
    wchar_t szAdminTitleSuffix[64]; //" (Admin)"
    bool bAdminShield;
    
    DWORD nMainTimerElapse; // периодичность, с которой из консоли считывается текст
    //bool isAdvLangChange; // в Висте без ConIme в самой консоли не меняется язык, пока не послать WM_SETFOCUS. Но при этом исчезает диалог быстрого поиска
    bool isSkipFocusEvents;
    //bool isLangChangeWsPlugin;
	char isMonitorConsoleLang;
    
    DWORD nAffinity;

	// Debugging - "c:\\temp\\ConEmuVCon-%i-%i.dat"
	BYTE isAdvLogging;
	//wchar_t szDumpPackets[MAX_PATH];
	// Debugging
	bool isConVisible;
    
    // Working variables...
    HBITMAP hBgBitmap;
    COORD   bgBmp;
    HDC     hBgDc;
    HFONT   mh_Font[MAX_FONT_STYLES], mh_Font2;
    TODO("По хорошему, CharWidth & CharABC нужно разделять по шрифтам - у Bold ширина может быть больше");
    WORD    CharWidth[0x10000]; //, Font2Width[0x10000];
	ABC     CharABC[0x10000];

    HWND hMain, hExt, hColors, hInfo;

    bool LoadImageFrom(TCHAR *inPath, bool abShowErrors=false);
	static void CenterDialog(HWND hWnd2);
    static INT_PTR CALLBACK wndOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK mainOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK extOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK colorOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK infoOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK hideOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK hotkeysOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
    void LoadSettings();
    void InitSettings();
    BOOL SaveSettings();
    bool ShowColorDialog(HWND HWndOwner, COLORREF *inColor);
    static int CALLBACK EnumFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount);
    static int CALLBACK EnumFontCallBackEx(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam);
    void UpdateMargins(RECT arcMargins);
    static void Dialog();
    void UpdatePos(int x, int y);
    void UpdateSize(UINT w, UINT h);
    void UpdateTTF(BOOL bNewTTF);
	void UpdateFontInfo();
    void Performance(UINT nID, BOOL bEnd);
	void SetArgBufferHeight(int anBufferHeight);
	void InitFont(LPCWSTR asFontName=NULL, int anFontHeight=-1, int anQuality=-1);
	BOOL RegisterFont(LPCWSTR asFontFile, BOOL abDefault);
	void RegisterFonts();
	void UnregisterFonts();
	BOOL GetFontNameFromFile(LPCTSTR lpszFilePath, LPTSTR rsFontName);
	void HistoryCheck();
	void HistoryAdd(LPCWSTR asCmd);
	LPCWSTR HistoryGet();
	void UpdateConsoleMode(DWORD nMode);
	BOOL CheckConIme();
	SettingsBase* CreateSettings();
	bool AutoRecreateFont(int nFontW, int nFontH);
	bool CheckTheming();
protected:
    LRESULT OnInitDialog();
	LRESULT OnInitDialog_Main();
	LRESULT OnInitDialog_Ext();
	LRESULT OnInitDialog_Color();
	LRESULT OnInitDialog_Info();
    LRESULT OnButtonClicked(WPARAM wParam, LPARAM lParam);
    LRESULT OnColorButtonClicked(WPARAM wParam, LPARAM lParam);
    LRESULT OnEditChanged(WPARAM wParam, LPARAM lParam);
    LRESULT OnColorEditChanged(WPARAM wParam, LPARAM lParam);
    LRESULT OnComboBox(WPARAM wParam, LPARAM lParam);
    LRESULT OnColorComboBox(WPARAM wParam, LPARAM lParam);
    LRESULT OnTab(LPNMHDR phdr);
private:
	bool GetColorById(WORD nID, COLORREF* color);
	bool SetColorById(WORD nID, COLORREF color);
    COLORREF acrCustClr[16]; // array of custom colors
    BOOL mb_IgnoreEditChanged, mb_IgnoreTtfChange, mb_CharSetWasSet;
    i64 mn_Freq;
    i64 mn_FPS[20]; int mn_FPS_CUR_FRAME;
    i64 mn_RFPS[20]; int mn_RFPS_CUR_FRAME;
    i64 mn_Counter[tPerfInterval-gbPerformance];
    i64 mn_CounterMax[tPerfInterval-gbPerformance];
    DWORD mn_CounterTick[tPerfInterval-gbPerformance];
    HWND hwndTip, hwndBalloon;
	void ShowFontErrorTip(LPCTSTR asInfo);
	TOOLINFO tiBalloon;
    void RegisterTipsFor(HWND hChildDlg);
    HFONT CreateFontIndirectMy(LOGFONT *inFont);
	void RecreateFont(WORD wFromID);
    // Theming
    HMODULE mh_Uxtheme;
    typedef HRESULT (STDAPICALLTYPE *SetWindowThemeT)(HWND hwnd,LPCWSTR pszSubAppName,LPCWSTR pszSubIdList);
    SetWindowThemeT SetWindowThemeF;
    typedef HRESULT (STDAPICALLTYPE *EnableThemeDialogTextureT)(HWND hwnd,DWORD dwFlags);
    EnableThemeDialogTextureT EnableThemeDialogTextureF;
	UINT mn_MsgUpdateCounter;
	//wchar_t temp[MAX_PATH];
	UINT mn_MsgRecreateFont;
	static int IsChecked(HWND hParent, WORD nCtrlId);
	static int GetNumber(HWND hParent, WORD nCtrlId);
	int SelectString(HWND hParent, WORD nCtrlId, LPCWSTR asText);
	int SelectStringExact(HWND hParent, WORD nCtrlId, LPCWSTR asText);
	BOOL mb_TabHotKeyRegistered;
	void RegisterTabs();
	void UnregisterTabs();
	static DWORD CALLBACK EnumFontsThread(LPVOID apArg);
	HANDLE mh_EnumThread;
	WORD mn_LastChangingFontCtrlId;
	// Временно регистрируемые шрифты
	typedef struct tag_RegFont {
		BOOL    bDefault;             // Этот шрифт пользователь указал через /fontfile
		wchar_t szFontFile[MAX_PATH]; // полный путь
		wchar_t szFontName[32];       // Font Family
		BOOL    bUnicode;             // Юникодный?
		BOOL    bHasBorders;          // Имеет ли данный шрифт символы рамок
	} RegFont;
	std::vector<RegFont> m_RegFonts;
	BOOL mb_StopRegisterFonts;
	//
	COLORREF Colors[0x20];
	bool mb_FadeInitialized;
	BYTE mn_FadeLow, mn_FadeHigh;
	DWORD mn_FadeMul, mn_FadeDiv;
	COLORREF ColorsFade[0x20];
	BOOL GetColorRef(HWND hDlg, WORD TB, COLORREF* pCR);
	inline BYTE GetFadeColorItem(BYTE c);
	//
	bool mb_ThemingEnabled;
	//
	bool TestHostkeyModifiers();
	static BYTE CheckHostkeyModifier(BYTE vk);
	static void ReplaceHostkey(BYTE vk, BYTE vkNew);
	static void AddHostkey(BYTE vk);
	static void TrimHostkeys();
	static void SetupHotkeyChecks(HWND hWnd2);
	static bool MakeHostkeyModifier();
	static BYTE HostkeyCtrlId2Vk(WORD nID);
	DWORD nMultiHotkeyModifier;
	BYTE mn_HostModOk[15], mn_HostModSkip[15];
	bool isHostkeySingleLR(WORD vk, WORD vkC, WORD vkL, WORD vkR);
};
