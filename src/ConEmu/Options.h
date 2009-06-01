#pragma once

class CSettings
{
public:
    CSettings();
    ~CSettings();

    TCHAR Config[MAX_PATH];

    int DefaultBufferHeight; bool ForceBufferHeight; bool AutoScroll;
    
    LOGFONT LogFont, LogFont2;
    COLORREF Colors[0x20];
    bool isExtendColors;
    char nExtendColor;

    /* Background image */
    WCHAR sBgImage[MAX_PATH];
    bool isShowBgImage, isBackgroundImageValid;

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
    bool isFixFarBorders;
    bool isCursorV;
    bool isCursorColor;
    char isRClickSendKey;
    bool isSentAltEnter;
    bool isMinToTray;
    bool isForceMonospace, isTTF;
    bool isUpdConHandle;
    BYTE isDragEnabled;
    BYTE isDropEnabled;
    DWORD nLDragKey, nRDragKey;
    bool isDefCopy;
    bool isDnDsteps;
    bool isGUIpb;
    char isTabs;
    DWORD wndWidth, wndHeight, ntvdmHeight; // в символах
    int wndX, wndY; // в пиксел€х
    bool wndCascade;
    u8 bgImageDarker;
    DWORD nSlideShowElapse;
    DWORD nIconID;
    bool isTryToCenter;
    RECT rcTabMargins;
    bool isTabFrame;
    bool isMulti; BYTE icMultiNew, icMultiNext, icMultiRecreate; bool isMultiNewConfirm;

    // «аголовки табов
    WCHAR szTabPanels[64]; LPCWSTR pszTabConsole;
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
    
    UINT nMainTimerElapse; // периодичность, с которой из консоли считываетс€ текст
    //bool isAdvLangChange; // в ¬исте без ConIme в самой консоли не мен€етс€ €зык, пока не послать WM_SETFOCUS. Ќо при этом исчезает диалог быстрого поиска
    bool isSkipFocusEvents;
    bool isLangChangeWsPlugin;
    
    DWORD nAffinity;

	// Debugging - "c:\\temp\\ConEmuVCon-%i-%i.dat"
	bool isAdvLogging;
	wchar_t szDumpPackets[MAX_PATH];
	// Debugging
	bool isConVisible;
    
    // Working variables...
    HBITMAP hBgBitmap;
    COORD   bgBmp;
    HDC     hBgDc;
    HFONT   mh_Font, mh_Font2;
    WORD    FontWidth[0x10000]; //, Font2Width[0x10000];

    HWND hMain, hColors, hInfo;

    bool LoadImageFrom(TCHAR *inPath, bool abShowErrors=false);
    static BOOL CALLBACK wndOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK mainOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK colorOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK infoOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
    void LoadSettings();
    void InitSettings();
    BOOL SaveSettings();
    bool ShowColorDialog(HWND HWndOwner, COLORREF *inColor);
    static BOOL CALLBACK EnumFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount);
    void UpdateMargins(RECT arcMargins);
    static void Dialog();
    void UpdatePos(int x, int y);
    void UpdateSize(UINT w, UINT h);
    void UpdateTTF(BOOL bNewTTF);
    void Performance(UINT nID, BOOL bEnd);
	void SetArgBufferHeight(int anBufferHeight);
public:
    LRESULT OnInitDialog();
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
    i64 mn_Counter[tPerfInterval-gbPerformance];
    i64 mn_CounterMax[tPerfInterval-gbPerformance];
    DWORD mn_CounterTick[tPerfInterval-gbPerformance];
    HWND hwndTip;
    void RegisterTipsFor(HWND hChildDlg);
    HFONT CreateFontIndirectMy(LOGFONT *inFont);
    // Theming
    HMODULE mh_Uxtheme;
    typedef HRESULT (STDAPICALLTYPE *SetWindowThemeT)(HWND hwnd,LPCWSTR pszSubAppName,LPCWSTR pszSubIdList);
    SetWindowThemeT SetWindowThemeF;
    typedef HRESULT (STDAPICALLTYPE *EnableThemeDialogTextureT)(HWND hwnd,DWORD dwFlags);
    EnableThemeDialogTextureT EnableThemeDialogTextureF;
	UINT mn_MsgUpdateCounter;
	wchar_t temp[MAX_PATH];
};
