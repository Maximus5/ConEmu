#pragma once

class CSettings
{
public:
	CSettings();
	~CSettings();

	TCHAR Config[MAX_PATH];

	int BufferHeight;
	
	LOGFONT LogFont, LogFont2;
	COLORREF Colors[0x10];
	
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
    bool isForceMonospace;
    bool isConMan;
    bool isDnD;
    char isDefCopy;
    bool isGUIpb;
    char isTabs;
    DWORD wndWidth, wndHeight;
    int wndX, wndY;
    u8 bgImageDarker;
    bool isConVisible;
    DWORD nSlideShowElapse;
    DWORD nIconID;
    bool isScrollTitle;
    DWORD ScrollTitleLen;

	// Заголовки табов
	TCHAR szTabPanels[32];
	TCHAR szTabEditor[32];
	TCHAR szTabEditorModified[32];
	TCHAR szTabViewer[32];
	DWORD nTabLenMax;

	bool LoadImageFrom(TCHAR *inPath);
	static BOOL CALLBACK wndOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
	void LoadSettings();
	void InitSettings();
	BOOL SaveSettings();
	bool ShowColorDialog(HWND HWndOwner, COLORREF *inColor);
	static BOOL CALLBACK EnumFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount);
};



