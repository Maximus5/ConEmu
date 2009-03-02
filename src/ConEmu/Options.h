#pragma once

class CSettings
{
public:
	CSettings();
	~CSettings();

	TCHAR Config[MAX_PATH];

	int BufferHeight;
	
	LOGFONT LogFont, LogFont2;
	COLORREF Colors[0x20];
	bool isExtendColors;
	char nExtendColor;
	
    TCHAR Cmd[MAX_PATH], pBgImage[MAX_PATH];

    DWORD FontSizeX;
    DWORD FontSizeX2;
	DWORD FontSizeX3;
    bool isShowBgImage, isBackgroundImageValid;
    bool isFullScreen;
    bool isFixFarBorders;
    bool isCursorV;
    bool isCursorColor;
    char isRClickSendKey;
    bool isSentAltEnter;
    bool isForceMonospace, isTTF;
    bool isConMan;
    bool isDnD;
    char isDefCopy;
    bool isDnDsteps;
    bool isGUIpb;
    char isTabs;
    DWORD wndWidth, wndHeight;
    int wndX, wndY;
    u8 bgImageDarker;
    bool isConVisible;
    DWORD nSlideShowElapse;
    DWORD nIconID;
    bool isTryToCenter;
	RECT rcTabMargins;
	bool isTabFrame;

	// Заголовки табов
	TCHAR szTabPanels[32];
	TCHAR szTabEditor[32];
	TCHAR szTabEditorModified[32];
	TCHAR szTabViewer[32];
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
	//bool isSkipFocusEvents;
    
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
};
