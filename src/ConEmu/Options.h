
#pragma once

class CSettings
{
public:
    CSettings();
    ~CSettings();

    TCHAR Config[MAX_PATH];

    int DefaultBufferHeight; bool ForceBufferHeight; bool AutoScroll; bool AutoBufferHeight;
    
	LONG FontWidth();
	LONG FontHeight();
	LONG BorderFontWidth();
	BYTE FontCharSet();
private:
    LOGFONT LogFont, LogFont2;
    BOOL mb_Name1Ok, mb_Name2Ok;
public:
	bool isAutoRegisterFonts;
	//wchar_t FontFile[MAX_PATH];
	LOGFONT ConsoleFont;
    COLORREF Colors[0x20];
    bool isExtendColors;
    char nExtendColor;

    /* Background image */
    WCHAR sBgImage[MAX_PATH];
    char isShowBgImage;
	bool isBackgroundImageValid;
	u8 bgImageDarker;

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

    DWORD FontSizeX;
    DWORD FontSizeX2;
    DWORD FontSizeX3;
    bool isFullScreen;
    char isFixFarBorders;
	bool isMouseSkipActivation, isMouseSkipMoving;
	struct tag_CharRanges {
		bool bUsed;
		wchar_t cBegin, cEnd;
	} icFixFarBorderRanges[10];
    BYTE isPartBrush75, isPartBrush50, isPartBrush25;
    bool isCursorV;
	bool isCursorBlink;
    bool isCursorColor;
    char isRClickSendKey;
    wchar_t *sRClickMacro;
    bool isSentAltEnter;
    bool isMinToTray;
    bool isForceMonospace, isProportional;
    bool isUpdConHandle;
	bool isRSelFix;
	//Drag
    BYTE isDragEnabled;
    BYTE isDropEnabled;
    DWORD nLDragKey, nRDragKey;
    bool isDefCopy;
	BYTE isDragOverlay;
	bool isDragShowIcons;
    //bool 
    bool isDebugSteps;
    bool isGUIpb; // GUI Progress bars
    char isTabs; bool isTabSelf, isTabRecent, isTabLazy;
	wchar_t *sTabCloseMacro;
    DWORD wndWidth, wndHeight, ntvdmHeight; // в символах
    int wndX, wndY; // в пикселях
    bool wndCascade;
    DWORD nSlideShowElapse;
    DWORD nIconID;
    bool isTryToCenter;
    RECT rcTabMargins;
    bool isTabFrame;
    bool isMulti; BYTE icMultiNew, icMultiNext, icMultiRecreate; bool isMultiNewConfirm;

    // Заголовки табов
    WCHAR szTabConsole[32];
    WCHAR szTabEditor[32];
    WCHAR szTabEditorModified[32];
    WCHAR szTabViewer[32];
    DWORD nTabLenMax;

    bool isVisualizer;
    char nVizNormal, nVizFore, nVizTab, nVizEOL, nVizEOF;
    wchar_t cVizTab, cVizEOL, cVizEOF;

    char isAllowDetach;
    bool isCreateAppWindow; 
    bool isScrollTitle;
    DWORD ScrollTitleLen;
    
    UINT nMainTimerElapse; // периодичность, с которой из консоли считывается текст
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
    HFONT   mh_Font, mh_Font2;
    WORD    CharWidth[0x10000]; //, Font2Width[0x10000];

    HWND hMain, hExt, hColors, hInfo;

    bool LoadImageFrom(TCHAR *inPath, bool abShowErrors=false);
    static INT_PTR CALLBACK wndOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK mainOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK extOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK colorOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK infoOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
    void LoadSettings();
    void InitSettings();
    BOOL SaveSettings();
    bool ShowColorDialog(HWND HWndOwner, COLORREF *inColor);
    static int CALLBACK EnumFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount);
    void UpdateMargins(RECT arcMargins);
    static void Dialog();
    void UpdatePos(int x, int y);
    void UpdateSize(UINT w, UINT h);
    void UpdateTTF(BOOL bNewTTF);
    void Performance(UINT nID, BOOL bEnd);
	void SetArgBufferHeight(int anBufferHeight);
	void InitFont(LPCWSTR asFontName=NULL, int anFontHeight=-1, int anQuality=-1);
	BOOL RegisterFont(LPCWSTR asFontFile, BOOL abDefault);
	void RegisterFonts();
	void UnregisterFonts();
	BOOL GetFontNameFromFile(LPCTSTR lpszFilePath, LPTSTR rsFontName);
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
    COLORREF acrCustClr[16]; // array of custom colors
    BOOL mb_IgnoreEditChanged, mb_IgnoreTtfChange;
    i64 mn_Freq;
    i64 mn_FPS[20]; int mn_FPS_CUR_FRAME;
    i64 mn_Counter[tPerfInterval-gbPerformance];
    i64 mn_CounterMax[tPerfInterval-gbPerformance];
    DWORD mn_CounterTick[tPerfInterval-gbPerformance];
    HWND hwndTip;
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
	int IsChecked(HWND hParent, WORD nCtrlId);
	int GetNumber(HWND hParent, WORD nCtrlId);
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
};
