
#include "Header.h"
#include <commctrl.h>
#include "../common/ConEmuCheck.h"

#define COUNTER_REFRESH 5000
#define MAX_CMD_HISTORY 100

#ifdef _DEBUG
#define RASTER_FONTS_NAME L"@Raster fonts"
#endif

const u8 chSetsNums[] = {0, 178, 186, 136, 1, 238, 134, 161, 177, 129, 130, 77, 255, 204, 128, 2, 222, 162, 163};
const char *ChSets[] = {"ANSI", "Arabic", "Baltic", "Chinese Big 5", "Default", "East Europe",
		"GB 2312", "Greek", "Hebrew", "Hangul", "Johab", "Mac", "OEM", "Russian", "Shiftjis",
		"Symbol", "Thai", "Turkish", "Vietnamese"};
int upToFontHeight=0;
HWND ghOpWnd=NULL;

#ifdef _DEBUG
#define HEAPVAL HeapValidate(GetProcessHeap(), 0, NULL);
#else
#define HEAPVAL 
#endif


typedef struct tagCONEMUDEFCOLORS {
	const wchar_t* pszTitle;
	DWORD dwDefColors[0x10];
} CONEMUDEFCOLORS;

const CONEMUDEFCOLORS DefColors[] = {
	{ L"Default color scheme (Windows standard)", {
	0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0, 
	0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff}},
	{ L"Gamma 1 (for use with dark monitors)", {
	0x00000000, 0x00960000, 0x0000aa00, 0x00aaaa00, 0x000000aa, 0x00800080, 0x0000aaaa, 0x00c0c0c0, 
	0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff}},
    { L"Murena scheme", {
    0x00000000, 0x00644100, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
    0x00808080, 0x00ff0000, 0x0076c587, 0x00ffff00, 0x00004bff, 0x00d78ce6, 0x0000ffff, 0x00ffffff}},
    { L"tc-maxx", {
    0x00000000, RGB(11,27,59), RGB(0,128,0), RGB(0,90,135), RGB(106,7,28), RGB(128,0,128), RGB(128,128,0), RGB(40,150,177),
    RGB(128,128,128), RGB(0,0,255), RGB(0,255,0), RGB(0,215,243), RGB(190,7,23), RGB(255,0,255), RGB(255,255,0), RGB(255,255,255)}},
    
};
const DWORD *dwDefColors = DefColors->dwDefColors;
//const DWORD dwDefColors[0x10] = {
//	0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0, 
//	0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff};
//const DWORD dwDefColors1[0x10] = {
//	0x00000000, 0x00960000, 0x0000aa00, 0x00aaaa00, 0x000000aa, 0x00800080, 0x0000aaaa, 0x00c0c0c0, 
//	0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff};
//const DWORD dwDefColors1[0x10] = {
//    0x00000000, 0x00644100, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
//    0x00808080, 0x00ff0000, 0x0076c587, 0x00ffff00, 0x00004bff, 0x00d78ce6, 0x0000ffff, 0x00ffffff};
DWORD gdwLastColors[0x10] = {0};
BOOL  gbLastColorsOk = FALSE;






namespace Settings {
    const WCHAR* szKeys[] = {L"<None>", L"Left Ctrl", L"Right Ctrl", L"Left Alt", L"Right Alt", L"Left Shift", L"Right Shift"};
    const DWORD  nKeys[] =  {0,         VK_LCONTROL,  VK_RCONTROL,   VK_LMENU,    VK_RMENU,     VK_LSHIFT,     VK_RSHIFT};
    const BYTE   FSizes[] = {0, 8, 10, 12, 14, 16, 18, 19, 20, 24, 26, 28, 30, 32, 34, 36, 40, 46, 50, 52, 72};
};

CSettings::CSettings()
{
    InitSettings();
    mb_StopRegisterFonts = FALSE;
    mb_IgnoreEditChanged = FALSE;
    mb_IgnoreTtfChange = TRUE;
	mb_CharSetWasSet = FALSE;
	mb_TabHotKeyRegistered = FALSE;
    hMain = NULL; hExt = NULL; hColors = NULL; hInfo = NULL; hwndTip = NULL;
    QueryPerformanceFrequency((LARGE_INTEGER *)&mn_Freq);
    memset(mn_Counter, 0, sizeof(*mn_Counter)*(tPerfInterval-gbPerformance));
    memset(mn_CounterMax, 0, sizeof(*mn_CounterMax)*(tPerfInterval-gbPerformance));
    memset(mn_FPS, 0, sizeof(mn_FPS)); mn_FPS_CUR_FRAME = 0;
    memset(mn_CounterTick, 0, sizeof(*mn_CounterTick)*(tPerfInterval-gbPerformance));
    hBgBitmap = NULL; bgBmp = MakeCoord(0,0); hBgDc = NULL; mh_Font = NULL; mh_Font2 = NULL;
    memset(CharWidth, 0, sizeof(CharWidth));
    nAttachPID = 0; hAttachConWnd = NULL;
    memset(&ourSI, 0, sizeof(ourSI));
    ourSI.cb = sizeof(ourSI);
    try {
        GetStartupInfoW(&ourSI);
    } catch(...) {
        memset(&ourSI, 0, sizeof(ourSI));
    }
	mn_LastChangingFontCtrlId = 0;

    SetWindowThemeF = NULL;
    mh_Uxtheme = LoadLibrary(L"UxTheme.dll");
    if (mh_Uxtheme) {
        SetWindowThemeF = (SetWindowThemeT)GetProcAddress(mh_Uxtheme, "SetWindowTheme");
        EnableThemeDialogTextureF = (EnableThemeDialogTextureT)GetProcAddress(mh_Uxtheme, "EnableThemeDialogTexture");
        //if (SetWindowThemeF) { SetWindowThemeF(Progressbar1, L" ", L" "); }
    }

	mn_MsgUpdateCounter = RegisterWindowMessage(L"ConEmuSettings::Counter");
	mn_MsgRecreateFont = RegisterWindowMessage(L"CSettings::RecreateFont");
}

CSettings::~CSettings()
{
    if (mh_Font) { DeleteObject(mh_Font); mh_Font = NULL; }
    if (mh_Font2) { DeleteObject(mh_Font2); mh_Font2 = NULL; }
    if (psCmd) {free(psCmd); psCmd = NULL;}
	if (psCmdHistory) {free(psCmdHistory); psCmdHistory = NULL;}
    if (psCurCmd) {free(psCurCmd); psCurCmd = NULL;}
	if (sTabCloseMacro) {free(sTabCloseMacro); sTabCloseMacro = NULL;}
	if (sRClickMacro) {free(sRClickMacro); sRClickMacro = NULL;}
    if (mh_Uxtheme!=NULL) { FreeLibrary(mh_Uxtheme); mh_Uxtheme = NULL; }
}

void CSettings::InitSettings()
{
    MCHKHEAP
    //memset(&gSet, 0, sizeof(gSet)); // -- Class!!! лучше делать все ручками!

//------------------------------------------------------------------------
///| Moved from CVirtualConsole |/////////////////////////////////////////
//------------------------------------------------------------------------
    _tcscpy(Config, L"Software\\ConEmu");

	//FontFile[0] = 0;
	isAutoRegisterFonts = true;
    
    psCmd = NULL; psCurCmd = NULL; wcscpy(szDefCmd, L"far");
	psCmdHistory = NULL; nCmdHistorySize = 0;
    isMulti = true; icMultiNew = 'W'; icMultiNext = 'Q'; icMultiRecreate = 192/*VK_тильда*/; isMultiNewConfirm = true;
    // Logging
    isAdvLogging = 0;
	//wcscpy(szDumpPackets, L"c:\\temp\\ConEmuVCon-%i-%i.dat");

    nMainTimerElapse = 10;
    nAffinity = 0; // 0 - don't change default affinity
    //isAdvLangChange = true;
    isSkipFocusEvents = false;
    //isLangChangeWsPlugin = false;
	isMonitorConsoleLang = 3;
    DefaultBufferHeight = 1000; AutoBufferHeight = true;
	FarSyncSize = false;
	nCmdOutputCP = 0;
	ForceBufferHeight = false; /* устанавливается в true, из ком.строки /BufferHeight */
	AutoScroll = true;
    LogFont.lfHeight = 16;
    LogFont.lfWidth = 0;
    LogFont.lfEscapement = LogFont.lfOrientation = 0;
    LogFont.lfWeight = FW_NORMAL;
    LogFont.lfItalic = LogFont.lfUnderline = LogFont.lfStrikeOut = FALSE;
    LogFont.lfCharSet = DEFAULT_CHARSET;
    LogFont.lfOutPrecision = OUT_TT_PRECIS;
    LogFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    LogFont.lfQuality = ANTIALIASED_QUALITY;
    LogFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    _tcscpy(LogFont.lfFaceName, L"Lucida Console");
    _tcscpy(LogFont2.lfFaceName, L"Lucida Console");
    mb_Name1Ok = FALSE; mb_Name2Ok = FALSE;
    isTryToCenter = false;
    isTabFrame = true;
    isForceMonospace = false; isProportional = false;
    isMinToTray = false;

	ConsoleFont.lfHeight = 6;
	ConsoleFont.lfWidth = 4;
	_tcscpy(ConsoleFont.lfFaceName, L"Lucida Console");

    Registry RegConColors, RegConDef;
    if (RegConColors.OpenKey(L"Console", KEY_READ))
    {
        RegConDef.OpenKey(HKEY_USERS, L".DEFAULT\\Console", KEY_READ);

        TCHAR ColorName[] = L"ColorTable00";
		bool  lbBlackFound = false;
        for (uint i = 0x10; i--;)
        {
            ColorName[10] = i/10 + '0';
            ColorName[11] = i%10 + '0';
            if (!RegConColors.Load(ColorName, Colors[i]))
                if (!RegConDef.Load(ColorName, Colors[i]))
                	Colors[i] = dwDefColors[i];
			if (Colors[i] == 0) {
				if (!lbBlackFound)
					lbBlackFound = true;
				else if (lbBlackFound)
					Colors[i] = dwDefColors[i];
			}
            Colors[i+0x10] = Colors[i]; // Умолчания
        }

        RegConDef.CloseKey();
        RegConColors.CloseKey();
    }
    isExtendColors = false;
    nExtendColor = 14;

//------------------------------------------------------------------------
///| Default settings |///////////////////////////////////////////////////
//------------------------------------------------------------------------
	isShowBgImage = 0;
    _tcscpy(sBgImage, L"c:\\back.bmp");
	bgImageDarker = 0x46;

    isFixFarBorders = 2; isPartBrush75 = 0xC8; isPartBrush50 = 0x96; isPartBrush25 = 0x5A;
	memset(icFixFarBorderRanges, 0, sizeof(icFixFarBorderRanges));
	icFixFarBorderRanges[0].bUsed = true; icFixFarBorderRanges[0].cBegin = 0x2013; icFixFarBorderRanges[0].cEnd = 0x25C4;
    
    wndHeight = 25;
	ntvdmHeight = 0; // Подбирать автоматически
    wndWidth = 80;
    //WindowMode=rNormal; -- устанавливается в конструкторе CConEmuMain
    isFullScreen = false;
    isHideCaption = false;
    wndX = 0; wndY = 0; wndCascade = true;
    isConVisible = false;
    nSlideShowElapse = 2500;
    nIconID = IDI_ICON1;
    isRClickSendKey = 2;
    sRClickMacro = NULL;
    _tcscpy(szTabConsole, L"%s");
    //pszTabConsole = _tcscpy(szTabPanels+_tcslen(szTabPanels)+1, L"Console");
    _tcscpy(szTabEditor, L"[%s]");
    _tcscpy(szTabEditorModified, L"[%s] *");
    /* */ _tcscpy(szTabViewer, L"{%s}");
    nTabLenMax = 20;
    
    isCursorV = true;
	isCursorBlink = true;
	isCursorColor = false;
    
    isTabs = 1; isTabSelf = true; isTabRecent = true; isTabLazy = true;
    lstrcpyW(sTabFontFace, L"Tahoma"); nTabFontCharSet = ANSI_CHARSET; nTabFontHeight = 16;
	sTabCloseMacro = NULL;
    
    isVisualizer = false;
    nVizNormal = 1; nVizFore = 15; nVizTab = 15; nVizEOL = 8; nVizEOF = 12;
    cVizTab = 0x2192; cVizEOL = 0x2193; cVizEOF = 0x2640;

    isAllowDetach = 0;
    isCreateAppWindow = false;
    isScrollTitle = true;
    ScrollTitleLen = 22;
    lstrcpy(szAdminTitleSuffix, L" (Admin)");
    
	isRSelFix = true; isMouseSkipActivation = true; isMouseSkipMoving = true;

    isDragEnabled = DRAG_L_ALLOWED; isDropEnabled = (BYTE)1; isDefCopy = true;
    nLDragKey = 0; nRDragKey = VK_LCONTROL; 
	isDragOverlay = 1; isDragShowIcons = true;
	
	isDebugSteps = true; 
    MCHKHEAP
}

void CSettings::LoadSettings()
{
    MCHKHEAP
    DWORD inSize = LogFont.lfHeight;
    TCHAR inFont[MAX_PATH], inFont2[MAX_PATH];
    _tcscpy(inFont, LogFont.lfFaceName);
    _tcscpy(inFont2, LogFont2.lfFaceName);
    DWORD Quality = LogFont.lfQuality;
    //gConEmu.WindowMode = rMaximized;
    mn_LoadFontCharSet = LogFont.lfCharSet;
    bool isBold = (LogFont.lfWeight>=FW_BOLD), isItalic = (LogFont.lfItalic!=FALSE);
    
//------------------------------------------------------------------------
///| Loading from registry |//////////////////////////////////////////////
//------------------------------------------------------------------------
    Registry reg;
    if (reg.OpenKey(Config, KEY_READ)) // NightRoman
    {
        TCHAR ColorName[] = L"ColorTable00";
        for (uint i = 0x20; i--;)
        {
            ColorName[10] = i/10 + '0';
            ColorName[11] = i%10 + '0';
            reg.Load(ColorName, Colors[i]);
        }
        memmove(acrCustClr, Colors, sizeof(COLORREF)*16);
        reg.Load(L"ExtendColors", isExtendColors);
        reg.Load(L"ExtendColorIdx", nExtendColor);
            if (nExtendColor<0 || nExtendColor>15) nExtendColor=14;

		// Debugging
		reg.Load(L"ConVisible", isConVisible);
		//reg.Load(L"DumpPackets", szDumpPackets);
		
		reg.Load(L"AutoRegisterFonts", isAutoRegisterFonts);
		
		if (reg.Load(L"FontName", inFont))
			mb_Name1Ok = TRUE;
        if (reg.Load(L"FontName2", inFont2))
        	mb_Name2Ok = TRUE;
        if (!mb_Name1Ok || !mb_Name2Ok)
        	isAutoRegisterFonts = true;
        
        reg.Load(L"CmdLine", &psCmd);
		reg.Load(L"CmdLineHistory", &psCmdHistory, &nCmdHistorySize); HistoryCheck();
        reg.Load(L"Multi", isMulti);
			reg.Load(L"Multi.NewConsole", icMultiNew);
			reg.Load(L"Multi.Next", icMultiNext);
			reg.Load(L"Multi.Recreate", icMultiRecreate);
			reg.Load(L"Multi.NewConfirm", isMultiNewConfirm);

        reg.Load(L"FontSize", inSize);
        reg.Load(L"FontSizeX", FontSizeX);
        reg.Load(L"FontSizeX3", FontSizeX3);
        reg.Load(L"FontSizeX2", FontSizeX2);
        reg.Load(L"FontCharSet", mn_LoadFontCharSet);
        reg.Load(L"Anti-aliasing", Quality);

		reg.Load(L"ConsoleFontName", ConsoleFont.lfFaceName);
		reg.Load(L"ConsoleCharWidth", ConsoleFont.lfWidth);
		reg.Load(L"ConsoleFontHeight", ConsoleFont.lfHeight);

        reg.Load(L"WindowMode", gConEmu.WindowMode);
        reg.Load(L"HideCaption", isHideCaption);
        reg.Load(L"ConWnd X", wndX); /*if (wndX<-10) wndX = 0;*/
        reg.Load(L"ConWnd Y", wndY); /*if (wndY<-10) wndY = 0;*/
		// ЭТО не влияет на szDefCmd. Только прямое указание флажка "/BufferHeight N" 
		// может сменить (умолчательную) команду запуска на "cmd" или "far"
        reg.Load(L"Cascaded", wndCascade);
        if (wndCascade) {
	        HWND hPrev = FindWindow(VirtualConsoleClassMain, NULL);
	        while (hPrev) {
				if (IsIconic(hPrev) || IsZoomed(hPrev)) {
					hPrev = FindWindowEx(NULL, hPrev, VirtualConsoleClassMain, NULL);
					continue;
				}
		        RECT rcWnd; GetWindowRect(hPrev, &rcWnd);
		        int nShift = (GetSystemMetrics(SM_CYSIZEFRAME)+GetSystemMetrics(SM_CYCAPTION))*1.5;
		        wndX = rcWnd.left + nShift;
		        wndY = rcWnd.top + nShift;
				break;
	        }
        }
        reg.Load(L"ConWnd Width", wndWidth); if (!wndWidth) wndWidth = 80; else if (wndWidth>1000) wndWidth = 1000;
        reg.Load(L"ConWnd Height", wndHeight); if (!wndHeight) wndHeight = 25; else if (wndHeight>500) wndHeight = 500;
        //TODO: Эти два параметра не сохраняются
        reg.Load(L"16bit Height", ntvdmHeight);
			if (ntvdmHeight!=0 && ntvdmHeight!=25 && ntvdmHeight!=28 && ntvdmHeight!=43 && ntvdmHeight!=50) ntvdmHeight = 25;
		reg.Load(L"DefaultBufferHeight", DefaultBufferHeight);
			if (DefaultBufferHeight < 300) DefaultBufferHeight = 300;
		reg.Load(L"AutoBufferHeight", AutoBufferHeight);
		reg.Load(L"FarSyncSize", FarSyncSize);
		reg.Load(L"CmdOutputCP", nCmdOutputCP);

        reg.Load(L"CursorType", isCursorV);
        reg.Load(L"CursorColor", isCursorColor);
		reg.Load(L"CursorBlink", isCursorBlink);

		if (!reg.Load(L"FixFarBorders", isFixFarBorders))
			reg.Load(L"Experimental", isFixFarBorders);
		{
			wchar_t szCharRanges[120]; // max 10 ranges x 10 chars + a little ;)
			if (reg.Load(L"FixFarBordersRanges", szCharRanges)) {
				int n = 0, nMax = countof(icFixFarBorderRanges);
				wchar_t *pszRange = szCharRanges, *pszNext = NULL;
				wchar_t cBegin, cEnd;
				while (*pszRange && n < nMax) {
					cBegin = (wchar_t)wcstol(pszRange, &pszNext, 16);
					if (!cBegin || cBegin == 0xFFFF || *pszNext != L'-') break;
					pszRange = pszNext + 1;
					cEnd = (wchar_t)wcstol(pszRange, &pszNext, 16);
					if (!cEnd || cEnd == 0xFFFF) break;

					icFixFarBorderRanges[n].bUsed = true;
					icFixFarBorderRanges[n].cBegin = cBegin;
					icFixFarBorderRanges[n].cEnd = cEnd;

					n ++;
					if (*pszNext != L';') break;
					pszRange = pszNext + 1;
				}
				for ( ; n < nMax; n++)
					icFixFarBorderRanges[n].bUsed = false;
			}
		}
        
        reg.Load(L"PartBrush75", isPartBrush75); if (isPartBrush75<5) isPartBrush75=5; else if (isPartBrush75>250) isPartBrush75=250;
        reg.Load(L"PartBrush50", isPartBrush50); if (isPartBrush50<5) isPartBrush50=5; else if (isPartBrush50>250) isPartBrush50=250;
        reg.Load(L"PartBrush25", isPartBrush25); if (isPartBrush25<5) isPartBrush25=5; else if (isPartBrush25>250) isPartBrush25=250;
        if (isPartBrush50>=isPartBrush75) isPartBrush50=isPartBrush75-10;
        if (isPartBrush25>=isPartBrush50) isPartBrush25=isPartBrush50-10;
        
        reg.Load(L"RightClick opens context menu", isRClickSendKey);
        	reg.Load(L"RightClickMacro", &sRClickMacro);
        reg.Load(L"AltEnter", isSentAltEnter);
        reg.Load(L"Min2Tray", isMinToTray);

        reg.Load(L"BackGround Image show", isShowBgImage);
			if (isShowBgImage!=0 && isShowBgImage!=1 && isShowBgImage!=2) isShowBgImage = 0;
		reg.Load(L"BackGround Image", sBgImage);
		reg.Load(L"bgImageDarker", bgImageDarker);

        reg.Load(L"FontBold", isBold);
        reg.Load(L"FontItalic", isItalic);
        reg.Load(L"ForceMonospace", isForceMonospace);
        reg.Load(L"Proportional", isProportional);
        reg.Load(L"Update Console handle", isUpdConHandle);
		reg.Load(L"RSelectionFix", isRSelFix);
		reg.Load(L"MouseSkipActivation", isMouseSkipActivation);
		reg.Load(L"MouseSkipMoving", isMouseSkipMoving);
        reg.Load(L"Dnd", isDragEnabled); 
        isDropEnabled = (BYTE)(isDragEnabled ? 1 : 0); // ранее "DndDrop" не было, поэтому ставим default
        reg.Load(L"DndLKey", nLDragKey);
        reg.Load(L"DndRKey", nRDragKey);
        reg.Load(L"DndDrop", isDropEnabled);
        reg.Load(L"DefCopy", isDefCopy);
		reg.Load(L"DragOverlay", isDragOverlay);
		reg.Load(L"DragShowIcons", isDragShowIcons);
        reg.Load(L"DebugSteps", isDebugSteps);
        reg.Load(L"GUIpb", isGUIpb);
        reg.Load(L"Tabs", isTabs);
	        reg.Load(L"TabSelf", isTabSelf);
	        reg.Load(L"TabLazy", isTabLazy);
	        reg.Load(L"TabRecent", isTabRecent);
			if (!reg.Load(L"TabCloseMacro", &sTabCloseMacro) && sTabCloseMacro && !*sTabCloseMacro) { free(sTabCloseMacro); sTabCloseMacro = NULL; }
			reg.Load(L"TabFontFace", sTabFontFace);
			reg.Load(L"TabFontCharSet", nTabFontCharSet);
			reg.Load(L"TabFontHeight", nTabFontHeight);
        reg.Load(L"TabFrame", isTabFrame);
        reg.Load(L"TabMargins", rcTabMargins);
        reg.Load(L"SlideShowElapse", nSlideShowElapse);
        reg.Load(L"IconID", nIconID);
        reg.Load(L"TabConsole", szTabConsole);
            //WCHAR* pszVert = wcschr(szTabPanels, L'|');
            //if (!pszVert) {
            //    if (wcslen(szTabPanels)>54) szTabPanels[54] = 0;
            //    pszVert = szTabPanels + wcslen(szTabPanels);
            //    wcscpy(pszVert+1, L"Console");
            //}
            //*pszVert = 0; pszTabConsole = pszVert+1;
        reg.Load(L"TabEditor", szTabEditor);
        reg.Load(L"TabEditorModified", szTabEditorModified);
        reg.Load(L"TabViewer", szTabViewer);
        reg.Load(L"TabLenMax", nTabLenMax);
        reg.Load(L"ScrollTitle", isScrollTitle);
        reg.Load(L"ScrollTitleLen", ScrollTitleLen);
        reg.Load(L"AdminTitleSuffix", szAdminTitleSuffix); szAdminTitleSuffix[sizeofarray(szAdminTitleSuffix)-1] = 0;
        reg.Load(L"TryToCenter", isTryToCenter);
        //reg.Load(L"CreateAppWindow", isCreateAppWindow);
        //reg.Load(L"AllowDetach", isAllowDetach);
        
        reg.Load(L"Visualizer", isVisualizer);
        reg.Load(L"VizNormal", nVizNormal);
        reg.Load(L"VizFore", nVizFore);
        reg.Load(L"VizTab", nVizTab);
        reg.Load(L"VizEol", nVizEOL);
        reg.Load(L"VizEof", nVizEOF);
        reg.Load(L"VizTabCh", cVizTab);
        reg.Load(L"VizEolCh", cVizEOL);
        reg.Load(L"VizEofCh", cVizEOF);
        
        reg.Load(L"MainTimerElapse", nMainTimerElapse); if (nMainTimerElapse>1000) nMainTimerElapse = 1000;
        reg.Load(L"AffinityMask", nAffinity);
        //reg.Load(L"AdvLangChange", isAdvLangChange);
        reg.Load(L"SkipFocusEvents", isSkipFocusEvents);
        //reg.Load(L"LangChangeWsPlugin", isLangChangeWsPlugin);
		reg.Load(L"MonitorConsoleLang", isMonitorConsoleLang);
        
        reg.CloseKey();
    }

    /*if (wndWidth)
        pVCon->TextWidth = wndWidth;
    if (wndHeight)
        pVCon->TextHeight = wndHeight;*/

    /*if (wndHeight && wndWidth)
    {
        COORD b = {wndWidth, wndHeight};
      SetConsoleWindowSize(b,false); // Maximus5 - по аналогии с NightRoman
      //MoveWindow(hConWnd, 0, 0, 1, 1, 0);
      //SetConsoleScreenBufferSize(pVCon->hConOut(), b);
      //MoveWindow(hConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
    }*/

    LogFont.lfHeight = inSize;
    LogFont.lfWidth = FontSizeX;
    _tcscpy(LogFont.lfFaceName, inFont);
    _tcscpy(LogFont2.lfFaceName, inFont2);
    LogFont.lfQuality = Quality;
    if (isBold)
        LogFont.lfWeight = FW_BOLD;
    LogFont.lfCharSet = (BYTE) mn_LoadFontCharSet;
    if (isItalic)
        LogFont.lfItalic = true;

    // pVCon еще не создано!
    /*if (isShowBgImage && pVCon)
        LoadImageFrom(pBgImage);*/

	//2009-06-07 Размер шрифта может быть задан в командной строке, так что создаем шрифт не здесь
	//InitFont();

	MCHKHEAP
}

void CSettings::InitFont(LPCWSTR asFontName/*=NULL*/, int anFontHeight/*=-1*/, int anQuality/*=-1*/)
{
	if (asFontName && *asFontName) {
		lstrcpynW(LogFont.lfFaceName, asFontName, 32);
		mb_Name1Ok = TRUE;
	}
	if (anFontHeight!=-1) {
		LogFont.lfHeight = anFontHeight;
		LogFont.lfWidth = 0;
	}
	if (anQuality!=-1) {
		LogFont.lfQuality = ANTIALIASED_QUALITY;
	}
	
	std::vector<RegFont>::iterator iter;
	
	if (!mb_Name1Ok) {
		for (int i = 0; !mb_Name1Ok && (i < 3); i++)
		{
			for (iter = m_RegFonts.begin(); iter != m_RegFonts.end(); iter++)
			{
				switch (i) {
				case 0:
					if (!iter->bDefault || !iter->bUnicode) continue;
					break;
				case 1:
					if (!iter->bDefault) continue;
					break;
				case 2:
					if (!iter->bUnicode) continue;
					break;
				default:
					break;
				}
				
				lstrcpynW(LogFont.lfFaceName, iter->szFontName, 32);
				mb_Name1Ok = TRUE;
				break;
			}
		}
	}
	if (!mb_Name2Ok) {
		for (iter = m_RegFonts.begin(); iter != m_RegFonts.end(); iter++)
		{
			if (iter->bHasBorders) {
				lstrcpynW(LogFont2.lfFaceName, iter->szFontName, 32);
				mb_Name2Ok = TRUE;
				break;
			}
		}
	}

    mh_Font = CreateFontIndirectMy(&LogFont);
	//2009-06-07 Реальный размер созданного шрифта мог измениться

    MCHKHEAP
}

void CSettings::UpdateMargins(RECT arcMargins)
{
    if (memcmp(&arcMargins, &rcTabMargins, sizeof(rcTabMargins))==0)
        return;

    rcTabMargins = arcMargins;

    Registry reg;
    if (reg.OpenKey(Config, KEY_WRITE))
    {
        reg.Save(L"TabMargins", rcTabMargins);
        reg.CloseKey();
    }
}

BOOL CSettings::SaveSettings()
{
    Registry reg;
    //if (reg.OpenKey(L"Console", KEY_WRITE))
    //{
    //    TCHAR ColorName[] = L"ColorTable00";
    //    for (uint i = 0x10; i--;)
    //    {
    //        ColorName[10] = i/10 + '0';
    //        ColorName[11] = i%10 + '0';
    //        reg.Save(ColorName, (DWORD)Colors[i]);
    //    }
    //    reg.CloseKey();

      if (reg.OpenKey(Config, KEY_WRITE)) // NightRoman
        {
            TCHAR ColorName[] = L"ColorTable00";
            for (uint i = 0x20; i--;)
            {
                ColorName[10] = i/10 + '0';
                ColorName[11] = i%10 + '0';
                reg.Save(ColorName, (DWORD)Colors[i]);
            }
            reg.Save(L"ExtendColors", isExtendColors);
            reg.Save(L"ExtendColorIdx", nExtendColor);

            int nLen = SendDlgItemMessage(hMain, tCmdLine, WM_GETTEXTLENGTH, 0, 0);
            if (nLen<=0) {
                if (psCmd) {free(psCmd); psCmd = NULL;}
            } else {
                if (nLen > (int)(psCmd ? _tcslen(psCmd) : 0)) {
                    if (psCmd) {free(psCmd); psCmd = NULL;}
                    psCmd = (TCHAR*)calloc(nLen+1, sizeof(TCHAR));
                }
                GetDlgItemText(hMain, tCmdLine, psCmd, nLen+1);
            }
            /*if (!isFullScreen && !gConEmu.isZoomed() && !gConEmu.isIconic())
            {
                RECT rcPos; GetWindowRect(ghWnd, &rcPos);
                wndX = rcPos.left;
                wndY = rcPos.top;
            }*/

			reg.Save(L"ConVisible", isConVisible);
            reg.Save(L"CmdLine", psCmd);
            reg.Save(L"Multi", isMulti);
				reg.Save(L"Multi.NewConsole", icMultiNew);
				reg.Save(L"Multi.Next", icMultiNext);
				reg.Save(L"Multi.Recreate", icMultiRecreate);
				reg.Save(L"Multi.NewConfirm", isMultiNewConfirm);
            reg.Save(L"FontName", LogFont.lfFaceName);
            reg.Save(L"FontName2", LogFont2.lfFaceName);
            bool lbTest = isAutoRegisterFonts; // Если в реестре настройка есть, или изменилось значение
            if (reg.Load(L"AutoRegisterFonts", lbTest) || isAutoRegisterFonts != lbTest)
            	reg.Save(L"AutoRegisterFonts", isAutoRegisterFonts);

            reg.Save(L"BackGround Image", sBgImage);
            reg.Save(L"bgImageDarker", bgImageDarker);
			reg.Save(L"BackGround Image show", isShowBgImage);

            reg.Save(L"FontSize", LogFont.lfHeight);
            reg.Save(L"FontSizeX", FontSizeX);
            reg.Save(L"FontSizeX2", FontSizeX2);
            reg.Save(L"FontSizeX3", FontSizeX3);
            reg.Save(L"FontCharSet", mn_LoadFontCharSet = LogFont.lfCharSet);
            reg.Save(L"Anti-aliasing", LogFont.lfQuality);
            reg.Save(L"WindowMode", isFullScreen ? rFullScreen : gConEmu.isZoomed() ? rMaximized : rNormal);
            reg.Save(L"HideCaption", isHideCaption);
            
			reg.Save(L"DefaultBufferHeight", DefaultBufferHeight);
			reg.Save(L"AutoBufferHeight", AutoBufferHeight);
			reg.Save(L"CmdOutputCP", nCmdOutputCP);

			reg.Save(L"CursorType", isCursorV);
            reg.Save(L"CursorColor", isCursorColor);
			reg.Save(L"CursorBlink", isCursorBlink);

            reg.Save(L"FixFarBorders", isFixFarBorders);
            reg.Save(L"RightClick opens context menu", isRClickSendKey);
            reg.Save(L"AltEnter", isSentAltEnter);
            reg.Save(L"Min2Tray", isMinToTray);
            reg.Save(L"RSelectionFix", isRSelFix);
			reg.Save(L"MouseSkipActivation", isMouseSkipActivation);
			reg.Save(L"MouseSkipMoving", isMouseSkipMoving);
            reg.Save(L"Dnd", isDragEnabled);
            reg.Save(L"DndLKey", nLDragKey);
            reg.Save(L"DndRKey", nRDragKey);
            reg.Save(L"DndDrop", isDropEnabled);
            reg.Save(L"DefCopy", isDefCopy);
			reg.Save(L"DragOverlay", isDragOverlay);
			reg.Save(L"DragShowIcons", isDragShowIcons);
            reg.Save(L"DebugSteps", isDebugSteps);

            reg.Save(L"GUIpb", isGUIpb);

            reg.Save(L"Tabs", isTabs);
		        reg.Save(L"TabSelf", isTabSelf);
		        reg.Save(L"TabLazy", isTabLazy);
		        reg.Save(L"TabRecent", isTabRecent);
            
            reg.Save(L"FontBold", LogFont.lfWeight == FW_BOLD);
            reg.Save(L"FontItalic", LogFont.lfItalic);
            reg.Save(L"ForceMonospace", isForceMonospace);
            reg.Save(L"Proportional", isProportional);
            reg.Save(L"Update Console handle", isUpdConHandle);

            reg.Save(L"ConWnd Width", wndWidth);
            reg.Save(L"ConWnd Height", wndHeight);
			reg.Save(L"16bit Height", ntvdmHeight);
            reg.Save(L"ConWnd X", wndX);
            reg.Save(L"ConWnd Y", wndY);
            reg.Save(L"Cascaded", wndCascade);

            reg.Save(L"ScrollTitle", isScrollTitle);
            reg.Save(L"ScrollTitleLen", ScrollTitleLen);
            
            reg.Save(L"Visualizer", isVisualizer);
            reg.Save(L"VizNormal", nVizNormal);
            reg.Save(L"VizFore", nVizFore);
            reg.Save(L"VizTab", nVizTab);
            reg.Save(L"VizEol", nVizEOL);
            reg.Save(L"VizEof", nVizEOF);
            reg.Save(L"VizTabCh", cVizTab);
            reg.Save(L"VizEolCh", cVizEOL);
            reg.Save(L"VizEofCh", cVizEOF);
            
            reg.Save(L"SkipFocusEvents", isSkipFocusEvents);
    		reg.Save(L"MonitorConsoleLang", isMonitorConsoleLang);
            
            reg.CloseKey();
            
            //if (isTabs==1) ForceShowTabs();
            
            //MessageBoxA(ghOpWnd, "Saved.", "Information", MB_ICONINFORMATION);
            return TRUE;
        }
    //}

    MessageBoxA(ghOpWnd, "Failed", "Information", MB_ICONERROR);
    return FALSE;
}


bool CSettings::ShowColorDialog(HWND HWndOwner, COLORREF *inColor)
{
    CHOOSECOLOR cc;                 // common dialog box structure

    // Вернул. IMHO - бред. Добавили Custom Color, а меняется ФОН окна!

    // Initialize CHOOSECOLOR
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = HWndOwner;
    cc.lpCustColors = (LPDWORD) acrCustClr;
    cc.rgbResult = *inColor;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&cc))
    {
        *inColor = cc.rgbResult;
        return true;
    }
    return false;
}

int CSettings::EnumFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount)
{
    MCHKHEAP
    int far * aiFontCount = (int far *) aFontCount;

    // Record the number of raster, TrueType, and vector
    // fonts in the font-count array.

	if (FontType & RASTER_FONTTYPE) {
        aiFontCount[0]++;
		#ifdef _DEBUG
		OutputDebugString(L"Raster font: "); OutputDebugString(lplf->lfFaceName); OutputDebugString(L"\n");
		#endif
	} else if (FontType & TRUETYPE_FONTTYPE) {
        aiFontCount[2]++;
	} else {
        aiFontCount[1]++;
	}

	if (SendDlgItemMessage(gSet.hMain, tFontFace, CB_FINDSTRINGEXACT, -1, (LPARAM) lplf->lfFaceName)==-1) {
		SendDlgItemMessage(gSet.hMain, tFontFace, CB_ADDSTRING, 0, (LPARAM) lplf->lfFaceName);
		SendDlgItemMessage(gSet.hMain, tFontFace2, CB_ADDSTRING, 0, (LPARAM) lplf->lfFaceName);
	}

    MCHKHEAP
    if (aiFontCount[0] || aiFontCount[1] || aiFontCount[2])
        return TRUE;
    else
        return FALSE;

    UNREFERENCED_PARAMETER( lplf );
    UNREFERENCED_PARAMETER( lpntm );
}

DWORD CSettings::EnumFontsThread(LPVOID apArg)
{
	HDC hdc = GetDC(NULL);
	int aFontCount[] = { 0, 0, 0 };
	EnumFontFamilies(hdc, (LPCTSTR) NULL, (FONTENUMPROC) EnumFamCallBack, (LPARAM) aFontCount);
	DeleteDC(hdc);

	#ifdef _DEBUG
	SendDlgItemMessage(gSet.hMain, tFontFace, CB_INSERTSTRING, 0, (LPARAM)RASTER_FONTS_NAME);
	#endif
	

	wchar_t szName[MAX_PATH];
	GetDlgItemText(gSet.hMain, tFontFace, szName, MAX_PATH);
	gSet.SelectString(gSet.hMain, tFontFace, szName);
	GetDlgItemText(gSet.hMain, tFontFace2, szName, MAX_PATH);
	gSet.SelectString(gSet.hMain, tFontFace2, szName);

	SafeCloseHandle(gSet.mh_EnumThread)

	return 0;
}

LRESULT CSettings::OnInitDialog()
{
	_ASSERTE(!hMain && !hColors && !hInfo);
	hMain = NULL; hExt = NULL; hColors = NULL; hInfo = NULL;
	gbLastColorsOk = FALSE;

	RegisterTabs();

	mn_LastChangingFontCtrlId = 0;

	TCHAR szTitle[MAX_PATH]; szTitle[0]=0;
	int nConfLen = _tcslen(Config);
	int nStdLen = strlen("Software\\ConEmu");
	if (nConfLen>(nStdLen+1))
		wsprintf(szTitle, L"Settings (%s)...", (Config+nStdLen+1));
	else
		_tcscpy(szTitle, L"Settings...");
	SetWindowText ( ghOpWnd, szTitle );

	MCHKHEAP
    {
        TCITEM tie;
        wchar_t szTitle[32];
        HWND _hwndTab = GetDlgItem(ghOpWnd, tabMain);
        tie.mask = TCIF_TEXT;
        tie.iImage = -1; 
        tie.pszText = wcscpy(szTitle, L"Main");
        TabCtrl_InsertItem(_hwndTab, 0, &tie);
        tie.pszText = wcscpy(szTitle, L"Features");
        TabCtrl_InsertItem(_hwndTab, 1, &tie);
        tie.pszText = wcscpy(szTitle, L"Colors");
        TabCtrl_InsertItem(_hwndTab, 2, &tie);
        tie.pszText = wcscpy(szTitle, L"Info");
        TabCtrl_InsertItem(_hwndTab, 3, &tie);
        
        HFONT hFont = CreateFont(nTabFontHeight/*TAB_FONT_HEIGTH*/, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, nTabFontCharSet /*ANSI_CHARSET*/, OUT_DEFAULT_PRECIS, 
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, /* L"Tahoma" */ sTabFontFace);
        SendMessage(_hwndTab, WM_SETFONT, WPARAM (hFont), TRUE);
        
        RECT rcClient; GetWindowRect(_hwndTab, &rcClient);
        MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);
        TabCtrl_AdjustRect(_hwndTab, FALSE, &rcClient);
        
        hMain = CreateDialog((HINSTANCE)GetModuleHandle(NULL), 
            MAKEINTRESOURCE(IDD_DIALOG1), ghOpWnd, mainOpProc);
        MoveWindow(hMain, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 0);
		/*
        hColors = CreateDialog((HINSTANCE)GetModuleHandle(NULL), 
            MAKEINTRESOURCE(IDD_DIALOG2), ghOpWnd, colorOpProc);
        MoveWindow(hColors, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 0);
        hInfo = CreateDialog((HINSTANCE)GetModuleHandle(NULL), 
            MAKEINTRESOURCE(IDD_DIALOG3), ghOpWnd, infoOpProc);
        MoveWindow(hInfo, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 0);
		*/

		//OnInitDialog_Main();

        ShowWindow(hMain, SW_SHOW);
    }
    
    MCHKHEAP

	{
		RECT rect;
		GetWindowRect(ghOpWnd, &rect);

		BOOL lbCentered = FALSE;
		HMONITOR hMon = MonitorFromWindow(ghOpWnd, MONITOR_DEFAULTTONEAREST);
		if (hMon) {
			MONITORINFO mi; mi.cbSize = sizeof(mi);
			if (GetMonitorInfo(hMon, &mi)) {
				lbCentered = TRUE;
				MoveWindow(ghOpWnd, 
					(mi.rcWork.left+mi.rcWork.right-rect.right+rect.left)/2,
					(mi.rcWork.top+mi.rcWork.bottom-rect.bottom+rect.top)/2,
					rect.right - rect.left, rect.bottom - rect.top, false);
			}
		}

		if (!lbCentered)
			MoveWindow(ghOpWnd, GetSystemMetrics(SM_CXSCREEN)/2 - (rect.right - rect.left)/2, GetSystemMetrics(SM_CYSCREEN)/2 - (rect.bottom - rect.top)/2, rect.right - rect.left, rect.bottom - rect.top, false);
	}

    return 0;
}

LRESULT CSettings::OnInitDialog_Main()
{
	if (gSet.EnableThemeDialogTextureF)
		gSet.EnableThemeDialogTextureF(hMain, 6/*ETDT_ENABLETAB*/);
		
	if (isUpdConHandle)
		CheckDlgButton(ghOpWnd, cbAutoConHandle, BST_CHECKED);

	SetDlgItemText(hMain, tFontFace, LogFont.lfFaceName);
	SetDlgItemText(hMain, tFontFace2, LogFont2.lfFaceName);
	DWORD dwThId;
	mh_EnumThread = CreateThread(0,0,EnumFontsThread,0,0,&dwThId); // хэндл закроет сама нить

	{
		wchar_t temp[MAX_PATH];

		for (uint i=0; i < sizeofarray(Settings::FSizes); i++)
		{
			wsprintf(temp, L"%i", Settings::FSizes[i]);
			if (i > 0)
				SendDlgItemMessage(hMain, tFontSizeY, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(hMain, tFontSizeX, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(hMain, tFontSizeX2, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(hMain, tFontSizeX3, CB_ADDSTRING, 0, (LPARAM) temp);
		}

		wsprintf(temp, L"%i", LogFont.lfHeight);
		upToFontHeight = LogFont.lfHeight;
		SelectStringExact(hMain, tFontSizeY, temp);

		wsprintf(temp, L"%i", FontSizeX);
		SelectStringExact(hMain, tFontSizeX, temp);

		wsprintf(temp, L"%i", FontSizeX2);
		SelectStringExact(hMain, tFontSizeX2, temp);

		wsprintf(temp, L"%i", FontSizeX3);
		SelectStringExact(hMain, tFontSizeX3, temp);
	}

	{
		_ASSERTE(countof(chSetsNums) == countof(ChSets));
		u8 num = 4;
		for (uint i=0; i < countof(ChSets); i++)
		{
			SendDlgItemMessageA(hMain, tFontCharset, CB_ADDSTRING, 0, (LPARAM) ChSets[i]);
			if (chSetsNums[i] == LogFont.lfCharSet) num = i;
		}
		SendDlgItemMessage(hMain, tFontCharset, CB_SETCURSEL, num, 0);
	}

	MCHKHEAP

	SetDlgItemText(hMain, tCmdLine, psCmd ? psCmd : L"");

	SetDlgItemText(hMain, tBgImage, sBgImage);
	CheckDlgButton(hMain, rBgSimple, BST_CHECKED);

	TCHAR tmp[10];
	wsprintf(tmp, L"%i", bgImageDarker);
	SendDlgItemMessage(hMain, tDarker, EM_SETLIMITTEXT, 3, 0);
	SetDlgItemText(hMain, tDarker, tmp);

	SendDlgItemMessage(hMain, slDarker, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(0, 255));
	SendDlgItemMessage(hMain, slDarker, TBM_SETPOS  , (WPARAM) true, (LPARAM) bgImageDarker);

	if (isShowBgImage)
		CheckDlgButton(hMain, cbBgImage, (isShowBgImage == 1) ? BST_CHECKED : BST_INDETERMINATE);
	else
	{
		EnableWindow(GetDlgItem(hMain, tBgImage), false);
		EnableWindow(GetDlgItem(hMain, tDarker), false);
		EnableWindow(GetDlgItem(hMain, slDarker), false);
		EnableWindow(GetDlgItem(hMain, bBgImage), false);
	}

	switch(LogFont.lfQuality)
	{
	case NONANTIALIASED_QUALITY:
		CheckDlgButton(hMain, rNoneAA, BST_CHECKED);
		break;
	case ANTIALIASED_QUALITY:
		CheckDlgButton(hMain, rStandardAA, BST_CHECKED);
		break;
	case CLEARTYPE_NATURAL_QUALITY:
		CheckDlgButton(hMain, rCTAA, BST_CHECKED);
		break;
	}
	if (isCursorColor) CheckDlgButton(hMain, cbCursorColor, BST_CHECKED);
	if (isCursorBlink) CheckDlgButton(hMain, cbCursorBlink, BST_CHECKED);


	if (isGUIpb) CheckDlgButton(hMain, cbGUIpb, BST_CHECKED);
	
	if (isCursorV)
		CheckDlgButton(hMain, rCursorV, BST_CHECKED);
	else
		CheckDlgButton(hMain, rCursorH, BST_CHECKED);
		
	if (isForceMonospace)
		CheckDlgButton(hMain, cbMonospace, BST_CHECKED);
	if (!isProportional)
		CheckDlgButton(hMain, cbNonProportional, BST_CHECKED);
		
	if (LogFont.lfWeight == FW_BOLD) CheckDlgButton(hMain, cbBold, BST_CHECKED);
	if (LogFont.lfItalic)            CheckDlgButton(hMain, cbItalic, BST_CHECKED);
	
	if (isFixFarBorders)
		CheckDlgButton(hMain, cbFixFarBorders, (isFixFarBorders == 1) ? BST_CHECKED : BST_INDETERMINATE);

	if (isFullScreen)
		CheckRadioButton(hMain, rNormal, rFullScreen, rFullScreen);
	else if (gConEmu.isZoomed())
		CheckRadioButton(hMain, rNormal, rFullScreen, rMaximized);
	else
		CheckRadioButton(hMain, rNormal, rFullScreen, rNormal);

	//wsprintf(temp, L"%i", wndWidth);   SetDlgItemText(hMain, tWndWidth, temp);
	SendDlgItemMessage(hMain, tWndWidth, EM_SETLIMITTEXT, 3, 0);
	//wsprintf(temp, L"%i", wndHeight);  SetDlgItemText(hMain, tWndHeight, temp);
	SendDlgItemMessage(hMain, tWndHeight, EM_SETLIMITTEXT, 3, 0);
	UpdateSize(wndWidth, wndHeight);

	EnableWindow(GetDlgItem(hMain, cbApplyPos), FALSE);

	SendDlgItemMessage(hMain, tWndX, EM_SETLIMITTEXT, 6, 0);
	SendDlgItemMessage(hMain, tWndY, EM_SETLIMITTEXT, 6, 0);

	MCHKHEAP

	if (!isFullScreen && !gConEmu.isZoomed())
	{
		EnableWindow(GetDlgItem(hMain, tWndWidth), true);
		EnableWindow(GetDlgItem(hMain, tWndHeight), true);
		EnableWindow(GetDlgItem(hMain, tWndX), true);
		EnableWindow(GetDlgItem(hMain, tWndY), true);
		EnableWindow(GetDlgItem(hMain, rFixed), true);
		EnableWindow(GetDlgItem(hMain, rCascade), true);
		if (!gConEmu.isIconic()) {
			RECT rc; GetWindowRect(ghWnd, &rc);
			wndX = rc.left; wndY = rc.top;
		}
	}
	else
	{
		EnableWindow(GetDlgItem(hMain, tWndWidth), false);
		EnableWindow(GetDlgItem(hMain, tWndHeight), false);
		EnableWindow(GetDlgItem(hMain, tWndX), false);
		EnableWindow(GetDlgItem(hMain, tWndY), false);
		EnableWindow(GetDlgItem(hMain, rFixed), false);
		EnableWindow(GetDlgItem(hMain, rCascade), false);
	}
	UpdatePos(wndX, wndY);
	CheckDlgButton(hMain, wndCascade ? rCascade : rFixed, BST_CHECKED);

	mn_LastChangingFontCtrlId = 0;

	RegisterTipsFor(hMain);

	return 0;
}

LRESULT CSettings::OnInitDialog_Ext()
{
	if (gSet.EnableThemeDialogTextureF)
		gSet.EnableThemeDialogTextureF(hExt, 6/*ETDT_ENABLETAB*/);

	if (isRClickSendKey) CheckDlgButton(hExt, cbRClick, (isRClickSendKey==1) ? BST_CHECKED : BST_INDETERMINATE);
	if (isSentAltEnter) CheckDlgButton(hExt, cbSendAE, BST_CHECKED);
	if (isMinToTray) CheckDlgButton(hExt, cbMinToTray, BST_CHECKED);
	if (isAutoRegisterFonts) CheckDlgButton(hExt, cbAutoRegFonts, BST_CHECKED);
	if (isDebugSteps) CheckDlgButton(hExt, cbDebugSteps, BST_CHECKED);

	if (isDragEnabled) {
		//CheckDlgButton(hExt, cbDragEnabled, BST_CHECKED);
		if (isDragEnabled & DRAG_L_ALLOWED) CheckDlgButton(hExt, cbDragL, BST_CHECKED);
		if (isDragEnabled & DRAG_R_ALLOWED) CheckDlgButton(hExt, cbDragR, BST_CHECKED);
	}
	if (isDropEnabled) CheckDlgButton(hExt, cbDropEnabled, (isDropEnabled==1) ? BST_CHECKED : BST_INDETERMINATE);
	if (isDefCopy) CheckDlgButton(hExt, cbDnDCopy, BST_CHECKED);
	// просто группировка
	{
		uint nKeyCount = sizeofarray(Settings::szKeys);
		u8 numL = 0, numR = 0;
		for (uint i=0; i<nKeyCount; i++) {
			SendDlgItemMessage(hExt, lbLDragKey, CB_ADDSTRING, 0, (LPARAM) Settings::szKeys[i]);
			SendDlgItemMessage(hExt, lbRDragKey, CB_ADDSTRING, 0, (LPARAM) Settings::szKeys[i]);
			if (Settings::nKeys[i] == nLDragKey) numL = i;
			if (Settings::nKeys[i] == nRDragKey) numR = i;
		}
		if (!numL) nLDragKey = 0; // если код клавиши неизвестен?
		if (!numR) nRDragKey = 0; // если код клавиши неизвестен?
		SendDlgItemMessage(hExt, lbLDragKey, CB_SETCURSEL, numL, 0);
		SendDlgItemMessage(hExt, lbRDragKey, CB_SETCURSEL, numR, 0);
	}
	// Overlay
	if (isDragOverlay) CheckDlgButton(hExt, cbDragImage, (isDragOverlay==1) ? BST_CHECKED : BST_INDETERMINATE);
	if (isDragShowIcons) CheckDlgButton(hExt, cbDragIcons, BST_CHECKED);


	if (isTabs)
		CheckDlgButton(hExt, cbTabs, (isTabs==1) ? BST_CHECKED : BST_INDETERMINATE);
	if (isTabSelf)
		CheckDlgButton(hExt, cbTabSelf, BST_CHECKED);
	if (isTabRecent)
		CheckDlgButton(hExt, cbTabRecent, BST_CHECKED);
	if (isTabLazy)
		CheckDlgButton(hExt, cbTabLazy, BST_CHECKED);
	
	if (isRSelFix)
		CheckDlgButton(hExt, cbRSelectionFix, BST_CHECKED);

	if (isMouseSkipActivation)
		CheckDlgButton(hExt, cbSkipActivation, BST_CHECKED);
	if (isMouseSkipMoving)
		CheckDlgButton(hExt, cbSkipMove, BST_CHECKED);

	if (isMonitorConsoleLang)
		CheckDlgButton(hExt, cbMonitorConsoleLang, BST_CHECKED);
	
	if (isSkipFocusEvents)
		CheckDlgButton(hExt, cbSkipFocusEvents, BST_CHECKED);

	if (isMulti)
		CheckDlgButton(hExt, cbConMan, BST_CHECKED);
	if (isMultiNewConfirm)
		CheckDlgButton(hExt, cbNewConfirm, BST_CHECKED);
	if (AutoBufferHeight)
		CheckDlgButton(hExt, cbLongOutput, BST_CHECKED);
	wchar_t sz[16];
	SendDlgItemMessage(hExt, tLongOutputHeight, EM_SETLIMITTEXT, 5, 0);
	SetDlgItemText(hExt, tLongOutputHeight, _ltow(gSet.DefaultBufferHeight, sz, 10));
	EnableWindow(GetDlgItem(hExt, tLongOutputHeight), AutoBufferHeight);
	SendDlgItemMessage(hExt, hkNewConsole, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	SendDlgItemMessage(hExt, hkNewConsole, HKM_SETHOTKEY, icMultiNew, 0);
	SendDlgItemMessage(hExt, hkSwitchConsole, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	SendDlgItemMessage(hExt, hkSwitchConsole, HKM_SETHOTKEY, icMultiNext, 0);
	SendDlgItemMessage(hExt, hkCloseConsole, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	SendDlgItemMessage(hExt, hkCloseConsole, HKM_SETHOTKEY, icMultiRecreate, 0);
	// 16bit Height
	SendDlgItemMessage(hExt, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"Auto");
	SendDlgItemMessage(hExt, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"25 lines");
	SendDlgItemMessage(hExt, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"28 lines");
	SendDlgItemMessage(hExt, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"43 lines");
	SendDlgItemMessage(hExt, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"50 lines");
	SendDlgItemMessage(hExt, lbNtvdmHeight, CB_SETCURSEL, !ntvdmHeight ? 0 :
		((ntvdmHeight == 25) ? 1 : ((ntvdmHeight == 28) ? 2 : ((ntvdmHeight == 43) ? 3 : 4))), 0);
	// Cmd.exe output cp
	SendDlgItemMessage(hExt, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Undefined");
	SendDlgItemMessage(hExt, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Automatic");
	SendDlgItemMessage(hExt, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Unicode");
	SendDlgItemMessage(hExt, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"OEM");
	SendDlgItemMessage(hExt, lbCmdOutputCP, CB_SETCURSEL, nCmdOutputCP, 0);
	


	if (isConVisible)
		CheckDlgButton(hExt, cbVisible, BST_CHECKED);

	RegisterTipsFor(hExt);

	return 0;
}

LRESULT CSettings::OnInitDialog_Color()
{
	if (gSet.EnableThemeDialogTextureF)
		gSet.EnableThemeDialogTextureF(hColors, 6/*ETDT_ENABLETAB*/);

	#define getR(inColorref) (byte)inColorref
	#define getG(inColorref) (byte)(inColorref >> 8)
	#define getB(inColorref) (byte)(inColorref >> 16)

	wchar_t temp[MAX_PATH];

	for (uint i = 0; i < 32; i++)
	{
		SendDlgItemMessage(hColors, 1100 + i, EM_SETLIMITTEXT, 11, 0);
		wsprintf(temp, L"%i %i %i", getR(Colors[i]), getG(Colors[i]), getB(Colors[i]));
		SetDlgItemText(hColors, 1100 + i, temp);
	}

	for (uint i=0; i < 16; i++)
	{
		wsprintf(temp, (i<10) ? L"# %i" : L"#%i", i);
		SendDlgItemMessage(hColors, lbExtendIdx, CB_ADDSTRING, 0, (LPARAM) temp);
		SendDlgItemMessage(hColors, lbVisFore, CB_ADDSTRING, 0, (LPARAM) temp);
		SendDlgItemMessage(hColors, lbVisNormal, CB_ADDSTRING, 0, (LPARAM) temp);
		SendDlgItemMessage(hColors, lbVisTab, CB_ADDSTRING, 0, (LPARAM) temp);
		SendDlgItemMessage(hColors, lbVisEOL, CB_ADDSTRING, 0, (LPARAM) temp);
		SendDlgItemMessage(hColors, lbVisEOF, CB_ADDSTRING, 0, (LPARAM) temp);
	}
	SendDlgItemMessage(hColors, lbExtendIdx, CB_SETCURSEL, nExtendColor, 0);
	CheckDlgButton(hColors, cbExtendColors, isExtendColors ? BST_CHECKED : BST_UNCHECKED);
	OnColorButtonClicked(cbExtendColors, 0);

	// Default colors
	memmove(gdwLastColors, Colors, sizeof(gdwLastColors));
	SendDlgItemMessage(hColors, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM) L"<Current color scheme>");
	//SendDlgItemMessage(hColors, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM) L"Default color sheme (Windows standard)");
	//SendDlgItemMessage(hColors, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM) L"Gamma 1 (for use with dark monitors)");
	for (uint i=0; i<sizeofarray(DefColors); i++)
		SendDlgItemMessage(hColors, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM) DefColors[i].pszTitle);
	SendDlgItemMessage(hColors, lbDefaultColors, CB_SETCURSEL, 0, 0);
	gbLastColorsOk = TRUE;

	// Visualizer
	CheckDlgButton(hColors, cbVisualizer, isVisualizer ? BST_CHECKED : BST_UNCHECKED);
	SendDlgItemMessage(hColors, lbVisFore, CB_SETCURSEL, nVizFore, 0);
	SendDlgItemMessage(hColors, lbVisNormal, CB_SETCURSEL, nVizNormal, 0);
	SendDlgItemMessage(hColors, lbVisTab, CB_SETCURSEL, nVizTab, 0);
	SendDlgItemMessage(hColors, lbVisEOL, CB_SETCURSEL, nVizEOL, 0);
	SendDlgItemMessage(hColors, lbVisEOF, CB_SETCURSEL, nVizEOF, 0);
	OnColorButtonClicked(cbVisualizer, 0);

	RegisterTipsFor(hColors);

	return 0;
}

LRESULT CSettings::OnInitDialog_Info()
{
	if (gSet.EnableThemeDialogTextureF)
		gSet.EnableThemeDialogTextureF(hInfo, 6/*ETDT_ENABLETAB*/);

	SetDlgItemText(hInfo, tCurCmdLine, GetCommandLine());

	// Performance
	Performance(gbPerformance, TRUE);

	gConEmu.UpdateProcessDisplay(TRUE);
	gConEmu.UpdateSizes();

	UpdateFontInfo();

	UpdateConsoleMode(gConEmu.ActiveCon()->RCon()->GetConsoleStates());

	RegisterTipsFor(hInfo);

	return 0;
}

LRESULT CSettings::OnButtonClicked(WPARAM wParam, LPARAM lParam)
{
    WORD CB = LOWORD(wParam);
    switch(CB)
    {
    case IDOK:
    case IDCANCEL:
    case IDCLOSE:
        if (isTabs==1) gConEmu.ForceShowTabs(TRUE); else
        if (isTabs==0) gConEmu.ForceShowTabs(FALSE); else
			TabBar.Update();
        SendMessage(ghOpWnd, WM_CLOSE, 0, 0);
        break;

    case rNoneAA:
    case rStandardAA:
    case rCTAA:
		PostMessage(hMain, gSet.mn_MsgRecreateFont, wParam, 0);
		/*
        LogFont.lfQuality = wParam == rNoneAA ? NONANTIALIASED_QUALITY : wParam == rStandardAA ? ANTIALIASED_QUALITY : CLEARTYPE_NATURAL_QUALITY;
        DeleteObject(mh_Font);
        mh_Font = CreateFontIndirectMy(&LogFont);
        if (FontSizeX) LogFont.lfWidth = FontSizeX;
        gConEmu.Update(true);
		*/
        break;

    case bSaveSettings:
		if (IsWindowEnabled(GetDlgItem(hMain, cbApplyPos))) // были изменения в полях размера/положения
			OnButtonClicked(cbApplyPos, 0);
        if (SaveSettings())
            SendMessage(ghOpWnd,WM_COMMAND,IDOK,0);
        break;

    case rNormal:
    case rFullScreen:
    case rMaximized:
        //gConEmu.SetWindowMode(wParam);
        EnableWindow(GetDlgItem(hMain, cbApplyPos), TRUE);
		EnableWindow(GetDlgItem(hMain, tWndWidth), CB == rNormal);
		EnableWindow(GetDlgItem(hMain, tWndHeight), CB == rNormal);
		EnableWindow(GetDlgItem(hMain, tWndX), CB == rNormal);
		EnableWindow(GetDlgItem(hMain, tWndY), CB == rNormal);
		EnableWindow(GetDlgItem(hMain, rFixed), CB == rNormal);
		EnableWindow(GetDlgItem(hMain, rCascade), CB == rNormal);
        break;
        
    case cbApplyPos:
	    if (IsChecked(hMain, rNormal) == BST_CHECKED) {
	        DWORD newX, newY;
			wchar_t temp[MAX_PATH];
	        GetDlgItemText(hMain, tWndWidth, temp, MAX_PATH);  newX = klatoi(temp);
	        GetDlgItemText(hMain, tWndHeight, temp, MAX_PATH); newY = klatoi(temp);
		    SetFocus(GetDlgItem(hMain, rNormal));
		    if (gConEmu.isZoomed() || gConEmu.isIconic() || isFullScreen)
			    gConEmu.SetWindowMode(rNormal);
			// Установить размер
	        gConEmu.SetConsoleWindowSize(MakeCoord(newX, newY), true);
	    } else if (IsChecked(hMain, rMaximized) == BST_CHECKED) {
		    SetFocus(GetDlgItem(hMain, rMaximized));
		    if (!gConEmu.isZoomed())
			    gConEmu.SetWindowMode(rMaximized);
	    } else if (IsChecked(hMain, rFullScreen) == BST_CHECKED) {
		    SetFocus(GetDlgItem(hMain, rFullScreen));
		    if (!isFullScreen)
			    gConEmu.SetWindowMode(rFullScreen);
	    }
	    EnableWindow(GetDlgItem(hMain, cbApplyPos), FALSE);
	    SetForegroundWindow(ghOpWnd);
	    break;
        
    case rCascade:
    case rFixed:
	    wndCascade = CB == rCascade;
	    break;

    case cbFixFarBorders:
        //isFixFarBorders = !isFixFarBorders;
		switch(IsChecked(hMain, cbFixFarBorders)) {
		case BST_UNCHECKED:
			isFixFarBorders = 0; break;
		case BST_CHECKED:
			isFixFarBorders = 1; break;
		case BST_INDETERMINATE:
			isFixFarBorders = 2; break;
		}

        gConEmu.Update(true);
        break;

    case cbCursorColor:
        isCursorColor = IsChecked(hMain,cbCursorColor);
        gConEmu.Update(true);
        break;

	case cbCursorBlink:
		isCursorBlink = IsChecked(hMain,cbCursorBlink);
		break;
        
    case cbConMan:
        isMulti = IsChecked(hExt, cbConMan);
        break;

	case cbNewConfirm:
		isMultiNewConfirm = IsChecked(hExt, cbNewConfirm);
		break;

	case cbLongOutput:
		AutoBufferHeight = IsChecked(hExt, cbLongOutput);
		gConEmu.EnableComSpec(AutoBufferHeight);
		EnableWindow(GetDlgItem(hExt, tLongOutputHeight), AutoBufferHeight);
		break;

    case cbBold:
    case cbItalic:
        {
			PostMessage(hMain, gSet.mn_MsgRecreateFont, wParam, 0);
        }
        break;

    case cbBgImage:
		{
        isShowBgImage = IsChecked(hMain, cbBgImage);
        EnableWindow(GetDlgItem(hMain, tBgImage), isShowBgImage);
        EnableWindow(GetDlgItem(hMain, tDarker), isShowBgImage);
        EnableWindow(GetDlgItem(hMain, slDarker), isShowBgImage);
        EnableWindow(GetDlgItem(hMain, bBgImage), isShowBgImage);

		BOOL lbNeedLoad = (hBgBitmap == NULL);
		if (isShowBgImage && bgImageDarker == 0) {
			if (MessageBox(ghOpWnd, 
				    L"Background image will NOT be visible\n"
					L"while 'Darkening' is 0. Increase it?",
					L"ConEmu", MB_YESNO|MB_ICONEXCLAMATION)!=IDNO)
			{
				bgImageDarker = 0x46;
				SendDlgItemMessage(hMain, slDarker, TBM_SETPOS  , (WPARAM) true, (LPARAM) bgImageDarker);
				TCHAR tmp[10];
				wsprintf(tmp, L"%i", gSet.bgImageDarker);
				SetDlgItemText(hMain, tDarker, tmp);
				lbNeedLoad = TRUE;
			}
		}
		if (lbNeedLoad) {
			gSet.LoadImageFrom(gSet.sBgImage);
		}

        gConEmu.Update(true);
		}
        break;

    case cbRClick:
		isRClickSendKey = IsChecked(hExt, cbRClick); //0-1-2
        break;

    case cbSendAE:
        isSentAltEnter = IsChecked(hExt, cbSendAE);
        break;

    case cbMinToTray:
        isMinToTray = IsChecked(hExt, cbMinToTray);
        break;
        
    case cbAutoRegFonts:
    	isAutoRegisterFonts = IsChecked(hExt, cbAutoRegFonts);
    	break;
    	
    case cbDebugSteps:
    	isDebugSteps = IsChecked(hExt, cbDebugSteps);
    	break;

    case cbDragL:
    case cbDragR:
        isDragEnabled = 
            (IsChecked(hExt, cbDragL) ? DRAG_L_ALLOWED : 0) |
            (IsChecked(hExt, cbDragR) ? DRAG_R_ALLOWED : 0);
        break;
    case cbDropEnabled:
        isDropEnabled = IsChecked(hExt, cbDropEnabled);
        break;
    case cbDnDCopy:
        isDefCopy = IsChecked(hExt, cbDnDCopy) == BST_CHECKED;
        break;

	case cbDragImage:
		isDragOverlay = IsChecked(hExt, cbDragImage);
		break;
	case cbDragIcons:
		isDragShowIcons = IsChecked(hExt, cbDragIcons) == BST_CHECKED;
		break;
    
    case cbGUIpb: // GUI Progress Bars
        isGUIpb = IsChecked(hMain, cbGUIpb);
        if (isGUIpb)
        	gConEmu.CheckGuiBarsCreated();
        break;
    
    case cbTabs:
        switch(IsChecked(hExt, cbTabs)) {
            case BST_UNCHECKED:
                isTabs = 0; break;
            case BST_CHECKED:
                isTabs = 1; break;
            case BST_INDETERMINATE:
                isTabs = 2; break;
        }
        //isTabs = !isTabs;
        break;
    case cbTabSelf:
    	isTabSelf = IsChecked(hExt, cbTabSelf);
    	break;
	case cbTabRecent:
		isTabRecent = IsChecked(hExt, cbTabRecent);
		break;
	case cbTabLazy:
		isTabLazy = IsChecked(hExt, cbTabLazy);
		break;

	case cbRSelectionFix:
		isRSelFix = IsChecked(hExt, cbRSelectionFix);
		break;

	case cbSkipActivation:
		isMouseSkipActivation = IsChecked(hExt, cbSkipActivation);
		break;
	case cbSkipMove:
		isMouseSkipMoving = IsChecked(hExt, cbSkipMove);
		break;

	case cbMonitorConsoleLang:
		isMonitorConsoleLang = IsChecked(hExt, cbMonitorConsoleLang);
		break;
	
	case cbSkipFocusEvents:
		isSkipFocusEvents = IsChecked(hExt, cbSkipFocusEvents);
		break;

    case cbNonProportional:
        isProportional = !isProportional;
        mb_IgnoreEditChanged = TRUE;
        gConEmu.Update(true);
        mb_IgnoreEditChanged = FALSE;
        break;

    case cbMonospace:
        isForceMonospace = !isForceMonospace;
		RecreateFont(tFontSizeX3);
        gConEmu.Update(true);
        break;

    case cbAutoConHandle:
        isUpdConHandle = !isUpdConHandle;

        gConEmu.Update(true);
        break;

    case rCursorH:
    case rCursorV:
        if (wParam == rCursorV)
            isCursorV = true;
        else
            isCursorV = false;

        gConEmu.Update(true);
        break;

    case bBgImage:
        {
			wchar_t temp[MAX_PATH];
			GetDlgItemText(hMain, tBgImage, temp, MAX_PATH);
            OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
            ofn.lStructSize=sizeof(ofn);
            ofn.hwndOwner = ghOpWnd;
            ofn.lpstrFilter = L"Bitmap images (*.bmp)\0*.bmp\0\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFile = temp;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrTitle = L"Choose background image";
            ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
                    | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;
            if (GetOpenFileName(&ofn))
                SetDlgItemText(hMain, tBgImage, temp);
        }
        break;

	case cbVisible:
		isConVisible = IsChecked(hExt, cbVisible);
		if (isConVisible) {
			// Если показывать - то только текущую (иначе на экране мешанина консолей будет
			gConEmu.ActiveCon()->RCon()->ShowConsole(isConVisible);
		} else {
			// А если скрывать - то все сразу
			for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
				CVirtualConsole *pCon = gConEmu.GetVCon(i);
				if (pCon) pCon->RCon()->ShowConsole(FALSE);
			}
		}
		break;

    default:
        break;

    }
    return 0;
}

LRESULT CSettings::OnColorButtonClicked(WPARAM wParam, LPARAM lParam)
{
    WORD CB = LOWORD(wParam);
    switch(wParam)
    {
    case cbExtendColors:
        isExtendColors = IsChecked(hColors, cbExtendColors) == BST_CHECKED ? true : false;
        for (int i=16; i<32; i++)
            EnableWindow(GetDlgItem(hColors, 1100+i), isExtendColors);
        EnableWindow(GetDlgItem(hColors, lbExtendIdx), isExtendColors);
        if (lParam) {
            gConEmu.Update(true);
        }
        break;
    case cbVisualizer:
        isVisualizer = IsChecked(hColors, cbVisualizer) == BST_CHECKED ? true : false;
        EnableWindow(GetDlgItem(hColors, lbVisNormal), isVisualizer);
        EnableWindow(GetDlgItem(hColors, lbVisFore), isVisualizer);
        EnableWindow(GetDlgItem(hColors, lbVisTab), isVisualizer);
        EnableWindow(GetDlgItem(hColors, lbVisEOL), isVisualizer);
        EnableWindow(GetDlgItem(hColors, lbVisEOF), isVisualizer);
        if (lParam) {
            gConEmu.Update(true);
        }
        break;

    default:
        if (CB >= 1000 && CB <= 1031)
        {
            COLORREF color = Colors[CB - 1000];
			wchar_t temp[MAX_PATH];
			if( ShowColorDialog(ghOpWnd, &color) )
            {
                Colors[CB - 1000] = color;
                wsprintf(temp, L"%i %i %i", getR(color), getG(color), getB(color));
                SetDlgItemText(hColors, CB + 100, temp);
                InvalidateRect(GetDlgItem(hColors, CB), 0, 1);

                gConEmu.m_Back.Refresh();

                gConEmu.Update(true);
            }
        }
    }

    return 0;
}

LRESULT CSettings::OnColorEditChanged(WPARAM wParam, LPARAM lParam)
{
    WORD TB = LOWORD(wParam);
    if (TB >= 1100 && TB <= 1131)
    {
        int r, g, b;
		wchar_t temp[MAX_PATH];
        GetDlgItemText(hColors, TB, temp, MAX_PATH);
        TCHAR *sp1 = _tcschr(temp, ' '), *sp2;
        if (sp1 && *(sp1+1) && *(sp1+1) != ' ')
        {
            sp2 = _tcschr(sp1+1, ' ');
            if (sp2 && *(sp2+1) && *(sp2+1) != ' ')
            {
                *sp1 = 0;
                sp1++;
                *sp2 = 0;
                sp2++;
                r = klatoi(temp); g = klatoi(sp1), b = klatoi(sp2);
                if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && Colors[TB - 1100] != RGB(r, g, b))
                {
                    Colors[TB - 1100] = RGB(r, g, b);
                    gConEmu.Update(true);
                    InvalidateRect(GetDlgItem(hColors, TB - 100), 0, 1);
                }
            }
        }
    }
    return 0;
}

LRESULT CSettings::OnEditChanged(WPARAM wParam, LPARAM lParam)
{
    if (mb_IgnoreEditChanged)
        return 0;

    WORD TB = LOWORD(wParam);
    if (TB == tBgImage)
    {
		wchar_t temp[MAX_PATH];
		GetDlgItemText(hMain, tBgImage, temp, MAX_PATH);
		if (wcscmp(temp, sBgImage)) {
			if( LoadImageFrom(temp, true) )
			{
				gConEmu.Update(true);
			}
        }
    }
    else if ( (TB == tWndWidth || TB == tWndHeight) && IsChecked(hMain, rNormal) == BST_CHECKED )
    {
	    EnableWindow(GetDlgItem(hMain, cbApplyPos), TRUE);
    }
    else if (TB == tDarker)
    {
        DWORD newV;
        TCHAR tmp[10];
        GetDlgItemText(hMain, tDarker, tmp, 10);
        newV = klatoi(tmp);
        if (newV < 256 && newV != bgImageDarker)
        {
            bgImageDarker = newV;
            SendDlgItemMessage(hMain, slDarker, TBM_SETPOS, (WPARAM) true, (LPARAM) bgImageDarker);
            LoadImageFrom(sBgImage);
            gConEmu.Update(true);
        }
    }
	else if (TB == tLongOutputHeight) {
		BOOL lbOk = FALSE;
		wchar_t szTemp[16];
		UINT nNewVal = GetDlgItemInt(hExt, tLongOutputHeight, &lbOk, FALSE);
		if (lbOk)
		{
			if (nNewVal >= 300 && nNewVal <= 9999)
				DefaultBufferHeight = nNewVal;
			else if (nNewVal > 9999)
				SetDlgItemText(hExt, TB, _ltow(DefaultBufferHeight, szTemp, 10));
		} else {
			SetDlgItemText(hExt, TB, _ltow(DefaultBufferHeight, szTemp, 10));
		}
	}
	else if (TB == hkNewConsole || TB == hkSwitchConsole || TB == hkCloseConsole) {
		UINT nHotKey = 0xFF & SendDlgItemMessage(hExt, TB, HKM_GETHOTKEY, 0, 0);
		if (TB == hkNewConsole)
			icMultiNew = nHotKey;
		else if (TB == hkSwitchConsole)
			icMultiNext = nHotKey;
		else if (TB == hkCloseConsole)
			icMultiRecreate = nHotKey;
	}

    return 0;
}

LRESULT CSettings::OnColorComboBox(WPARAM wParam, LPARAM lParam)
{
    WORD wId = LOWORD(wParam);
    if (wId == lbExtendIdx) {
        nExtendColor = SendDlgItemMessage(hColors, wId, CB_GETCURSEL, 0, 0);
    } else if (wId==lbVisFore) {
        nVizFore = SendDlgItemMessage(hColors, wId, CB_GETCURSEL, 0, 0);
    } else if (wId==lbVisNormal) {
        nVizNormal = SendDlgItemMessage(hColors, wId, CB_GETCURSEL, 0, 0);
    } else if (wId==lbVisTab) {
        nVizTab = SendDlgItemMessage(hColors, wId, CB_GETCURSEL, 0, 0);
    } else if (wId==lbVisEOL) {
        nVizEOL = SendDlgItemMessage(hColors, wId, CB_GETCURSEL, 0, 0);
    } else if (wId==lbVisEOF) {
        nVizEOF = SendDlgItemMessage(hColors, wId, CB_GETCURSEL, 0, 0);
	} else if (wId==lbDefaultColors) {
		//int nIdx = SendDlgItemMessage(hColors, wId, CB_GETCURSEL, 0, 0);
		//EnableWindow(GetDlgItem(hColors, cbDefaultColors), nIdx>0 || (gbLastColorsOk && !nIdx));
		if (gbLastColorsOk) // только если инициализация палитр завершилась
		{
			const DWORD* pdwDefData = NULL;
			wchar_t temp[32];
			int nIdx = SendDlgItemMessage(hColors, lbDefaultColors, CB_GETCURSEL, 0, 0);
			if (nIdx == 0)
				pdwDefData = gdwLastColors;
			else if (nIdx >= 1 && nIdx <= (int)sizeofarray(DefColors))
				pdwDefData = DefColors[nIdx-1].dwDefColors;
			else
				return 0; // неизвестный набор
			uint nCount = sizeofarray(DefColors->dwDefColors);
			for (uint i = 0; i < nCount; i++)
			{
				Colors[i] = pdwDefData[i];
				wsprintf(temp, L"%i %i %i", getR(Colors[i]), getG(Colors[i]), getB(Colors[i]));
				SetDlgItemText(hColors, 1100 + i, temp);
				InvalidateRect(GetDlgItem(hColors, 1000+i), 0, 1);
			}
		} else return 0;
	}
    
    gConEmu.Update(true);

    return 0;
}

LRESULT CSettings::OnComboBox(WPARAM wParam, LPARAM lParam)
{
    WORD wId = LOWORD(wParam);
    if (wId == tFontCharset)
    {
		mb_CharSetWasSet = TRUE;
		PostMessage(hMain, gSet.mn_MsgRecreateFont, wParam, 0);
    }
    else if (wId == tFontFace || wId == tFontFace2 || 
		wId == tFontSizeY || wId == tFontSizeX || 
        wId == tFontSizeX2 || wId == tFontSizeX3)
    {
		if (HIWORD(wParam) == CBN_SELCHANGE)
			PostMessage ( hMain, mn_MsgRecreateFont, wId, 0 );
		else
			mn_LastChangingFontCtrlId = wId;
    } else 
    if (wId == lbLDragKey || wId == lbRDragKey) {
        int num = SendDlgItemMessage(hExt, wId, CB_GETCURSEL, 0, 0);
        int nKeyCount = sizeofarray(Settings::szKeys);
        if (num>=0 && num<nKeyCount) {
            if (wId == lbLDragKey)
                nLDragKey = Settings::nKeys[num];
            else
                nRDragKey = Settings::nKeys[num];
        } else {
            if (wId == lbLDragKey)
                nLDragKey = 0;
            else
                nRDragKey = 0;
            if (num) // Invalid index?
                SendDlgItemMessage(hExt, wId, CB_SETCURSEL, num=0, 0);
        }
	} else if (wId == lbNtvdmHeight) {
		int num = SendDlgItemMessage(hExt, wId, CB_GETCURSEL, 0, 0);
		ntvdmHeight = (num == 1) ? 25 : ((num == 2) ? 28 : ((num == 3) ? 43 : ((num == 4) ? 50 : 0)));
	} else if (wId == lbCmdOutputCP) {
		nCmdOutputCP = SendDlgItemMessage(hExt, wId, CB_GETCURSEL, 0, 0);
		if (nCmdOutputCP == -1) nCmdOutputCP = 0;
		gConEmu.EnableComSpec(AutoBufferHeight);
	}
    return 0;
}

LRESULT CSettings::OnTab(LPNMHDR phdr)
{
    switch (phdr->code) {
        case TCN_SELCHANGE:
            {
                int nSel = TabCtrl_GetCurSel(phdr->hwndFrom);

                HWND* phCurrent = NULL;
                UINT  nDlgRc = 0;
                DLGPROC dlgProc = NULL;
                
                if (nSel==0) {
                	phCurrent = &hMain;
                } else if (nSel==1) {
                	phCurrent = &hExt;
                	nDlgRc = IDD_DIALOG4;
                	dlgProc = extOpProc;
                } else if (nSel==2) {
                	phCurrent = &hColors;
                	nDlgRc = IDD_DIALOG2;
                	dlgProc = colorOpProc;
                } else {
                	phCurrent = &hInfo;
                	nDlgRc = IDD_DIALOG3;
                	dlgProc = infoOpProc;
                }
                
				if (*phCurrent == NULL && nDlgRc && dlgProc) {
					SetCursor(LoadCursor(NULL,IDC_WAIT));

					HWND _hwndTab = GetDlgItem(ghOpWnd, tabMain);
					RECT rcClient; GetWindowRect(_hwndTab, &rcClient);
					MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);
					TabCtrl_AdjustRect(_hwndTab, FALSE, &rcClient);

					*phCurrent = CreateDialog((HINSTANCE)GetModuleHandle(NULL), 
						MAKEINTRESOURCE(nDlgRc), ghOpWnd, dlgProc);
					MoveWindow(*phCurrent, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 0);
				}
                
                if (*phCurrent != NULL) {
                    ShowWindow(*phCurrent, SW_SHOW);
                    if (*phCurrent != hMain)   ShowWindow(hMain, SW_HIDE);
                    if (*phCurrent != hExt)    ShowWindow(hExt,  SW_HIDE);
                    if (*phCurrent != hColors) ShowWindow(hColors, SW_HIDE);
                    if (*phCurrent != hInfo)   ShowWindow(hInfo, SW_HIDE);
                    SetFocus(*phCurrent);
                }
            }
            break;
    }
    return 0;
}

bool CSettings::LoadImageFrom(TCHAR *inPath, bool abShowErrors)
{
    if (!inPath || _tcslen(inPath)>=MAX_PATH) {
        if (abShowErrors)
            MBoxA(L"Invalid 'inPath' in CSettings::LoadImageFrom");
        return false;
    }

    TCHAR exPath[MAX_PATH + 2];
    if (!ExpandEnvironmentStrings(inPath, exPath, MAX_PATH)) {
        if (abShowErrors) {
            TCHAR szError[MAX_PATH*2];
            DWORD dwErr = GetLastError();
            swprintf(szError, L"Can't expand environment strings:\r\n%s\r\nError code=0x%08X\r\nImage loading failed",
                inPath, dwErr);
            MBoxA(szError);
        }
        return false;
    }

    bool lRes = false;
    klFile file;
    if (file.Open(exPath))
    {
        char File[101];
        file.Read(File, 100);
        char *pBuf = File;
        if (pBuf[0] == 'B' && pBuf[1] == 'M' && *(u32*)(pBuf + 0x0A) >= 0x36 && *(u32*)(pBuf + 0x0A) <= 0x436 && *(u32*)(pBuf + 0x0E) == 0x28 && !pBuf[0x1D] && !*(u32*)(pBuf + 0x1E))
            //if (*(u16*)pBuf == 'MB' && *(u32*)(pBuf + 0x0A) >= 0x36)
        {
            const HDC hScreenDC = GetDC(0);
            HDC hNewBgDc = CreateCompatibleDC(hScreenDC);
            HBITMAP hNewBgBitmap;
            if (hNewBgDc)
            {
                if((hNewBgBitmap = (HBITMAP)LoadImage(NULL, exPath, IMAGE_BITMAP,0,0,LR_LOADFROMFILE)) != NULL)
                {
                    if (hBgBitmap) { DeleteObject(hBgBitmap); hBgBitmap=NULL; }
                    if (hBgDc) { DeleteDC(hBgDc); hBgDc=NULL; }

                    hBgDc = hNewBgDc;
                    hBgBitmap = hNewBgBitmap;

                    if(SelectObject(hBgDc, hBgBitmap))
                    {
                        isBackgroundImageValid = true;
                        bgBmp.X = *(u32*)(pBuf + 0x12);
                        bgBmp.Y = *(i32*)(pBuf + 0x16);
                        // Равняем на границу 4-х пикселов (WinXP SP2)
                        int nCxFixed = ((bgBmp.X+3)>>2)<<2;
                        if (klstricmp(sBgImage, inPath))
                        {
                            lRes = true;
                            _tcscpy(sBgImage, inPath);
                        }

                        struct bColor
                        {
                            u8 b;
                            u8 g;
                            u8 r;
                        };

                        MCHKHEAP
                            //GetDIBits памяти не хватает 
                        bColor *bArray = new bColor[(nCxFixed+10) * bgBmp.Y];
                        MCHKHEAP

                        BITMAPINFO bInfo; memset(&bInfo, 0, sizeof(bInfo));
                        bInfo.bmiHeader.biSize = sizeof(BITMAPINFO);
                        bInfo.bmiHeader.biWidth = nCxFixed/*bgBmp.X*/;
                        bInfo.bmiHeader.biHeight = bgBmp.Y;
                        bInfo.bmiHeader.biPlanes = 1;
                        bInfo.bmiHeader.biBitCount = 24;
                        bInfo.bmiHeader.biCompression = BI_RGB;

                        MCHKHEAP
                        if (!GetDIBits(hBgDc, hBgBitmap, 0, bgBmp.Y, bArray, &bInfo, DIB_RGB_COLORS))
                            //MBoxA(L"!"); //Maximus5 - Да, это очень информативно
                            MBoxA(L"!GetDIBits");


                        MCHKHEAP
                        for (int y=0; y<bgBmp.Y; y++)
                        {
                            int i = y*nCxFixed;
                            for (int x=0; x<bgBmp.X; x++, i++)
                            //for (int i = 0; i < bgBmp.X * bgBmp.Y; i++)
                            {
                                bArray[i].r = klMulDivU32(bArray[i].r, bgImageDarker, 255);
                                bArray[i].g = klMulDivU32(bArray[i].g, bgImageDarker, 255);
                                bArray[i].b = klMulDivU32(bArray[i].b, bgImageDarker, 255);
                            }
                        }

                        MCHKHEAP
                        if (!SetDIBits(hBgDc, hBgBitmap, 0, bgBmp.Y, bArray, &bInfo, DIB_RGB_COLORS))
                            MBoxA(L"!SetDIBits");

                        MCHKHEAP
                        delete[] bArray;
                        MCHKHEAP
                    }
                }
                else
                    DeleteDC(hNewBgDc);
            }

            ReleaseDC(0, hScreenDC);
        } else {
            if (abShowErrors)
                MBoxA(L"Only BMP files supported as background!");
        }
        file.Close();
    }

    return lRes;
}

void CSettings::Dialog()
{
	SetCursor(LoadCursor(NULL,IDC_WAIT));

	// Сначала обновить DC, чтобы некрасивостей не было
	UpdateWindow(ghWndDC);

	//2009-05-03. DialogBox создает МОДАЛЬНЫЙ Диалог
    //DialogBox((HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOGM), 0, wndOpProc);
	
    HWND hOpt = CreateDialog(g_hInstance, MAKEINTRESOURCE(IDD_DIALOGM), NULL, wndOpProc);
	if (!hOpt) {
		DisplayLastError(L"Can't create settings dialog!");
	} else {
		ShowWindow ( hOpt, SW_SHOWNORMAL );
		SetFocus ( hOpt );
	}
}

INT_PTR CSettings::wndOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
    switch (messg)
    {
    case WM_INITDIALOG:
        ghOpWnd = hWnd2;
        #ifdef _DEBUG
        //if (IsDebuggerPresent())
        SetWindowPos(ghOpWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
        #endif
        SetClassLongPtr(hWnd2, GCLP_HICON, (LONG)hClassIcon);
        gSet.OnInitDialog();
        break;

    case WM_GETICON:
        if (wParam==ICON_BIG) {
            /*SetWindowLong(hWnd2, DWL_MSGRESULT, (LRESULT)hClassIcon);
            return 1;*/
        } else {
            SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, (LRESULT)hClassIconSm);
            return 1;
        }
        return 0;

    case WM_CTLCOLORSTATIC:
        for (uint i = 1000; i < 1016; i++)
            if (GetDlgItem(hWnd2, i) == (HWND)lParam)
            {
                static HBRUSH KillBrush;
                DeleteObject(KillBrush);
                KillBrush = CreateSolidBrush(gSet.Colors[i-1000]);
                return (BOOL)KillBrush;
            }
            break;
    //case WM_KEYDOWN:
    //    if (wParam == VK_ESCAPE)
    //        SendMessage(hWnd2, WM_CLOSE, 0, 0);
    //    break;

    case WM_HSCROLL:
        {
            int newV = SendDlgItemMessage(hWnd2, slDarker, TBM_GETPOS, 0, 0);
            if (newV != gSet.bgImageDarker)
            {
                gSet.bgImageDarker = newV;
                TCHAR tmp[10];
                wsprintf(tmp, L"%i", gSet.bgImageDarker);
                SetDlgItemText(hWnd2, tDarker, tmp);
                gSet.LoadImageFrom(gSet.sBgImage);
                gConEmu.Update(true);
            }
        }
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            gSet.OnButtonClicked(wParam, lParam);
        }
        else if (HIWORD(wParam) == EN_CHANGE)
        {
            gSet.OnEditChanged(wParam, lParam);
        }
        else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
        {
            gSet.OnComboBox(wParam, lParam);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR phdr = (LPNMHDR)lParam;
            if (phdr->idFrom == tabMain)
                gSet.OnTab(phdr);
        } break;
    case WM_CLOSE:
        //if (gSet.hwndTip) {DestroyWindow(gSet.hwndTip); gSet.hwndTip = NULL;}
		DestroyWindow(hWnd2);
		break;
    case WM_DESTROY:
		gSet.UnregisterTabs();
        if (gSet.hwndTip) {DestroyWindow(gSet.hwndTip); gSet.hwndTip = NULL;}
        //EndDialog(hWnd2, TRUE);
        ghOpWnd = NULL; gSet.hMain = NULL; gSet.hExt = NULL; gSet.hColors = NULL; gSet.hInfo = NULL;
        gbLastColorsOk = FALSE;
        break;
	case WM_HOTKEY:
		if (wParam == 0x101) {
			// Переключиться на следующий таб
			int nCur = SendDlgItemMessage(ghOpWnd, tabMain, TCM_GETCURSEL,0,0);
			int nAll = SendDlgItemMessage(ghOpWnd, tabMain, TCM_GETITEMCOUNT,0,0);
			nCur ++; if (nCur>=nAll) nCur = 0;
			SendDlgItemMessage(ghOpWnd, tabMain, TCM_SETCURSEL,nCur,0);				 
			NMHDR hdr = {GetDlgItem(ghOpWnd, tabMain),tabMain,TCN_SELCHANGE};
			gSet.OnTab(&hdr);
		} else if (wParam == 0x102) {
			// Переключиться на предыдущий таб
			int nCur = SendDlgItemMessage(ghOpWnd, tabMain, TCM_GETCURSEL,0,0);
			int nAll = SendDlgItemMessage(ghOpWnd, tabMain, TCM_GETITEMCOUNT,0,0);
			nCur --; if (nCur<0) nCur = nAll - 1;
			SendDlgItemMessage(ghOpWnd, tabMain, TCM_SETCURSEL,nCur,0);				 
			NMHDR hdr = {GetDlgItem(ghOpWnd, tabMain),tabMain,TCN_SELCHANGE};
			gSet.OnTab(&hdr);
		}
	case WM_ACTIVATE:
		if (LOWORD(wParam) != 0)
			gSet.RegisterTabs();
		else
			gSet.UnregisterTabs();
		break;
    default:
        return 0;
    }
    return 0;
}

INT_PTR CSettings::mainOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
    switch (messg)
    {
    case WM_INITDIALOG:
		_ASSERTE(gSet.hMain==NULL || gSet.hMain==hWnd2);
		gSet.hMain = hWnd2;
        gSet.OnInitDialog_Main();
        break;

    case WM_CTLCOLORSTATIC:
        for (uint i = 1000; i < 1016; i++)
            if (GetDlgItem(hWnd2, i) == (HWND)lParam)
            {
                static HBRUSH KillBrush;
                DeleteObject(KillBrush);
                KillBrush = CreateSolidBrush(gSet.Colors[i-1000]);
                return (BOOL)KillBrush;
            }
            break;
    //case WM_KEYDOWN:
    //    if (wParam == VK_ESCAPE)
    //        SendMessage(hWnd2, WM_CLOSE, 0, 0);
    //    break;

    case WM_HSCROLL:
        {
            int newV = SendDlgItemMessage(hWnd2, slDarker, TBM_GETPOS, 0, 0);
            if (newV != gSet.bgImageDarker)
            {
                gSet.bgImageDarker = newV;
                TCHAR tmp[10];
                wsprintf(tmp, L"%i", gSet.bgImageDarker);
                SetDlgItemText(hWnd2, tDarker, tmp);
                gSet.LoadImageFrom(gSet.sBgImage);
                gConEmu.Update(true);
            }
        }
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            gSet.OnButtonClicked(wParam, lParam);
        }
        else if (HIWORD(wParam) == EN_CHANGE)
        {
            gSet.OnEditChanged(wParam, lParam);
        }
        else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
        {
            gSet.OnComboBox(wParam, lParam);
		} else if (HIWORD(wParam) == CBN_KILLFOCUS && gSet.mn_LastChangingFontCtrlId && LOWORD(wParam) == gSet.mn_LastChangingFontCtrlId) {
			PostMessage ( gSet.hMain, gSet.mn_MsgRecreateFont, gSet.mn_LastChangingFontCtrlId, 0 );
			gSet.mn_LastChangingFontCtrlId = 0;
		}
        break;
    default:
		if (messg == gSet.mn_MsgRecreateFont) {
			gSet.RecreateFont(wParam);
		}
        return 0;
    }
    return 0;
}

INT_PTR CSettings::extOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
    switch (messg)
    {
    case WM_INITDIALOG:
		_ASSERTE(gSet.hExt==NULL || gSet.hExt==hWnd2);
		gSet.hExt = hWnd2;
        gSet.OnInitDialog_Ext();
        break;

    case WM_CTLCOLORSTATIC:
        for (uint i = 1000; i < 1016; i++)
            if (GetDlgItem(hWnd2, i) == (HWND)lParam)
            {
                static HBRUSH KillBrush;
                DeleteObject(KillBrush);
                KillBrush = CreateSolidBrush(gSet.Colors[i-1000]);
                return (BOOL)KillBrush;
            }
            break;
    //case WM_KEYDOWN:
    //    if (wParam == VK_ESCAPE)
    //        SendMessage(hWnd2, WM_CLOSE, 0, 0);
    //    break;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            gSet.OnButtonClicked(wParam, lParam);
        }
        else if (HIWORD(wParam) == EN_CHANGE)
        {
            gSet.OnEditChanged(wParam, lParam);
        }
        else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
        {
            gSet.OnComboBox(wParam, lParam);
		}
        break;
    default:
        return 0;
    }
    return 0;
}

INT_PTR CSettings::colorOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
    switch (messg)
    {
    case WM_INITDIALOG:
		_ASSERTE(gSet.hColors==NULL || gSet.hColors==hWnd2);
		gSet.hColors = hWnd2;
		gSet.OnInitDialog_Color();
        break;

    case WM_CTLCOLORSTATIC:
        for (uint i = 1000; i < 1032; i++)
            if (GetDlgItem(hWnd2, i) == (HWND)lParam)
            {
                static HBRUSH KillBrush;
                DeleteObject(KillBrush);
                KillBrush = CreateSolidBrush(gSet.Colors[i-1000]);
                return (BOOL)KillBrush;
            }
            break;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            gSet.OnColorButtonClicked(wParam, lParam);
        }
        else if (HIWORD(wParam) == EN_CHANGE)
        {
            gSet.OnColorEditChanged(wParam, lParam);
        }
        else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
        {
            gSet.OnColorComboBox(wParam, lParam);
        }
        break;
    default:
        return 0;
    }
    return 0;
}

INT_PTR CSettings::infoOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
    switch (messg)
    {
    case WM_INITDIALOG:
		_ASSERTE(gSet.hInfo==NULL || gSet.hInfo==hWnd2);
		gSet.hInfo = hWnd2;
		gSet.OnInitDialog_Info();
        break;

    default:
		if (messg == gSet.mn_MsgUpdateCounter) {
            wchar_t sTemp[64];
			if (gSet.mn_Freq!=0) {
				double v = 0;
				if (wParam != 0) {
					v = (gSet.mn_CounterMax[wParam]/(double)gSet.mn_Freq)*1000;
				} else {
				    i64 tmin = 0, tmax = 0;
				    tmin = gSet.mn_FPS[0];
				    for (UINT i = 0; i < countof(gSet.mn_FPS); i++) {
					    if (gSet.mn_FPS[i] < tmin) tmin = gSet.mn_FPS[i];
					    if (gSet.mn_FPS[i] > tmax) tmax = gSet.mn_FPS[i];
				    }
				    if (tmax > tmin) {
					    v = ((double)20) / (tmax - tmin) * gSet.mn_Freq;
				    }
				}
				swprintf(sTemp, L"%.1f", v);
				SetDlgItemText(gSet.hInfo, wParam+tPerfFPS, sTemp);
			}
			return TRUE;
		}
        return 0;
    }
    return 0;
}

void CSettings::UpdatePos(int x, int y)
{
    TCHAR temp[32];

    gSet.wndX = x;
    gSet.wndY = y;
    
    if (ghOpWnd) 
    {
        mb_IgnoreEditChanged = TRUE;

        wsprintf(temp, L"%i", gSet.wndX);
        SetDlgItemText(hMain, tWndX, temp);

        wsprintf(temp, L"%i", gSet.wndY);
        SetDlgItemText(hMain, tWndY, temp);

        mb_IgnoreEditChanged = FALSE;
    }
}

void CSettings::UpdateSize(UINT w, UINT h)
{
    TCHAR temp[32];

    if (w<29 || h<9)
        return;
    if (w!=wndWidth || h!=wndHeight) {
        gSet.wndWidth = w;
        gSet.wndHeight = h;
    }

    if (ghOpWnd) {
        mb_IgnoreEditChanged = TRUE;

        wsprintf(temp, L"%i", gSet.wndWidth);
        SetDlgItemText(hMain, tWndWidth, temp);

        wsprintf(temp, L"%i", gSet.wndHeight);
        SetDlgItemText(hMain, tWndHeight, temp);

        mb_IgnoreEditChanged = FALSE;
    }
}

void CSettings::UpdateTTF(BOOL bNewTTF)
{
    if (mb_IgnoreTtfChange) return;

    isProportional = bNewTTF;
    /* */ CheckDlgButton(hMain, cbNonProportional, 
        gSet.isProportional ? BST_UNCHECKED : BST_CHECKED);
}

void CSettings::UpdateFontInfo()
{
	if (!ghOpWnd || !hInfo) return;

	wchar_t szTemp[32];
	wsprintf(szTemp, L"%ix%ix%i", LogFont.lfHeight, LogFont.lfWidth, tm.tmAveCharWidth);
	SetDlgItemText(hInfo, tRealFontMain, szTemp);
	wsprintf(szTemp, L"%ix%i", LogFont2.lfHeight, LogFont2.lfWidth);
	SetDlgItemText(hInfo, tRealFontBorders, szTemp);
}

void CSettings::Performance(UINT nID, BOOL bEnd)
{
    if (nID == gbPerformance) //groupbox ctrl id
    {
		if (!gConEmu.isMainThread())
			return;

        if (ghOpWnd) {
            // Performance
            wchar_t sTemp[128];
            //Нихрена это не мегагерцы. Например на "AMD Athlon 64 X2 1999 MHz" здесь отображается "0.004 GHz"
            //swprintf(sTemp, L"Performance counters (%.3f GHz)", ((double)(mn_Freq/1000)/1000000));
            swprintf(sTemp, L"Performance counters (%I64i)", ((i64)(mn_Freq/1000)));
            SetDlgItemText(hInfo, nID, sTemp);
            
            for (nID=tPerfFPS; mn_Freq && nID<=tPerfInterval; nID++) {
				SendMessage(gSet.hInfo, mn_MsgUpdateCounter, nID-tPerfFPS, 0);
            }
        }
        return;
    }
    if (nID<tPerfFPS || nID>tPerfInterval)
        return;

    if (nID == tPerfFPS) {
	    i64 tick2 = 0; //, tmin = 0, tmax = 0;
	    QueryPerformanceCounter((LARGE_INTEGER *)&tick2);
	    mn_FPS[mn_FPS_CUR_FRAME] = tick2;
	    mn_FPS_CUR_FRAME++;
		if (mn_FPS_CUR_FRAME >= (int)countof(mn_FPS)) mn_FPS_CUR_FRAME = 0;
	    //tmin = mn_FPS[0];
	    //for (int i = 0; i < countof(mn_FPS); i++) {
		//    if (mn_FPS[i] < tmin) tmin = mn_FPS[i];
		//    if (mn_FPS[i] > tmax) tmax = mn_FPS[i];
	    //}
	    //if (tmax > tmin)
        if (ghOpWnd) {
			PostMessage(gSet.hInfo, mn_MsgUpdateCounter, nID-tPerfFPS, 0);
        }
	    return;
	}        

    if (!bEnd) {
        QueryPerformanceCounter((LARGE_INTEGER *)&(mn_Counter[nID-tPerfFPS]));
        return;
    } else if (!mn_Counter[nID-tPerfFPS] || !mn_Freq) {
        return;
    }
    
    i64 tick2;
    QueryPerformanceCounter((LARGE_INTEGER *)&tick2);
    i64 t = (tick2-mn_Counter[nID-tPerfFPS]);
    
    if (mn_CounterMax[nID-tPerfFPS]<t || 
        (GetTickCount()-mn_CounterTick[nID-tPerfFPS])>COUNTER_REFRESH)
    {
        mn_CounterMax[nID-tPerfFPS] = t;
        mn_CounterTick[nID-tPerfFPS] = GetTickCount();
        
        if (ghOpWnd) {
			PostMessage(gSet.hInfo, mn_MsgUpdateCounter, nID-tPerfFPS, 0);
        }
    }
}

void CSettings::RegisterTipsFor(HWND hChildDlg)
{
    // Create the ToolTip.
    if (!hwndTip || !IsWindow(hwndTip)) {
        hwndTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
                              WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON | TTS_NOPREFIX,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              ghOpWnd, NULL, 
                              g_hInstance, NULL);
        SetWindowPos(hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);
    }
    if (!hwndTip) return; // не смогли создать

    HWND hChild = NULL, hEdit = NULL;
    
    BOOL lbRc = FALSE;
    TCHAR szText[0x200];
    while ((hChild = FindWindowEx(hChildDlg, hChild, NULL, NULL)) != NULL)
    {
        LONG wID = GetWindowLong(hChild, GWL_ID);
        if (wID == -1) continue;
        
        if (LoadString(g_hInstance, wID, szText, 0x200)) {
            // Associate the ToolTip with the tool.
            TOOLINFO toolInfo = { 0 };
            toolInfo.cbSize = 44; //sizeof(toolInfo);
            //GetWindowRect(hChild, &toolInfo.rect); MapWindowPoints(NULL, hChildDlg, (LPPOINT)&toolInfo.rect, 2);
            GetClientRect(hChild, &toolInfo.rect);
            toolInfo.hwnd = hChild; //hChildDlg;
            toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
            toolInfo.uId = (UINT_PTR)hChild;
            toolInfo.lpszText = szText;
            //toolInfo.hinst = g_hInstance;
            //toolInfo.lpszText = (LPTSTR)wID;
            lbRc = SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

			hEdit = FindWindowEx(hChild, NULL, L"Edit", NULL);
			if (hEdit) {
				toolInfo.uId = (UINT_PTR)hEdit;
				lbRc = SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
			}
        }
    }

    SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)300);
}

void CSettings::RecreateFont(WORD wFromID)
{
	if (wFromID == tFontFace)
		mb_IgnoreTtfChange = FALSE;

	LOGFONT LF = {0};
	LF.lfOutPrecision = OUT_TT_PRECIS;
	LF.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	LF.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;

	GetDlgItemText(hMain, tFontFace, LF.lfFaceName, countof(LF.lfFaceName));

	LF.lfHeight = GetNumber(hMain, tFontSizeY);
	LF.lfWidth = FontSizeX = GetNumber(hMain, tFontSizeX);
	LF.lfWeight = IsChecked(hMain, cbBold) ? FW_BOLD : FW_NORMAL;
	LF.lfItalic = IsChecked(hMain, cbItalic);

	LF.lfCharSet = mn_LoadFontCharSet;
	if (mb_CharSetWasSet) {
		mb_CharSetWasSet = FALSE;
		int newCharSet = SendDlgItemMessage(hMain, tFontCharset, CB_GETCURSEL, 0, 0);
		if (newCharSet != CB_ERR && newCharSet >= 0 && newCharSet < countof(chSetsNums))
			LF.lfCharSet = chSetsNums[newCharSet];
		else
			LF.lfCharSet = DEFAULT_CHARSET;
	}

	if (IsChecked(hMain, rNoneAA))
		LF.lfQuality = NONANTIALIASED_QUALITY;
	else if (IsChecked(hMain, rStandardAA))
		LF.lfQuality = ANTIALIASED_QUALITY;
	else if (IsChecked(hMain, rCTAA))
		LF.lfQuality = CLEARTYPE_NATURAL_QUALITY;

	GetDlgItemText(hMain, tFontFace2, LogFont2.lfFaceName, countof(LogFont2.lfFaceName));
	FontSizeX2 = GetNumber(hMain, tFontSizeX2);
	FontSizeX3 = GetNumber(hMain, tFontSizeX3);
	


	HFONT hf = CreateFontIndirectMy(&LF);
	if (hf) {
		HFONT hOldF = mh_Font;
		LogFont = LF;
		mh_Font = hf;
		DeleteObject(hOldF);

		gConEmu.Update(true);
		if (!isFullScreen && !gConEmu.isZoomed())
			gConEmu.SyncWindowToConsole();
		else
			gConEmu.SyncConsoleToWindow();
		gConEmu.ReSize();
	}

	if (wFromID == tFontFace) {
		wchar_t szSize[10];
		wsprintf(szSize, L"%i", LF.lfHeight);
		SetDlgItemText(hMain, tFontSizeY, szSize);
	}

	UpdateFontInfo();

	mb_IgnoreTtfChange = TRUE;
}

HFONT CSettings::CreateFontIndirectMy(LOGFONT *inFont)
{
    memset(CharWidth, 0, sizeof(*CharWidth)*0x10000);

	//lfOutPrecision = OUT_RASTER_PRECIS, 

    HFONT hFont = NULL;
	#ifdef _DEBUG
	if (wcscmp(inFont->lfFaceName, RASTER_FONTS_NAME)) {
		hFont = CreateFontIndirect(inFont);
	} else {
		LOGFONT lfRast = *inFont;
		lfRast.lfFaceName[0] = 0;
		lfRast.lfOutPrecision = OUT_RASTER_PRECIS;
		lfRast.lfQuality = PROOF_QUALITY;
		lfRast.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		hFont = CreateFontIndirect(&lfRast);
	}
	#else
		hFont = CreateFontIndirect(inFont);
	#endif

    if (hFont) {
        HDC hScreenDC = GetDC(0);
        HDC hDC = CreateCompatibleDC(hScreenDC);
        MBoxAssert(hDC);
        if (hDC)
        {
            HFONT hOldF = (HFONT)SelectObject(hDC, hFont);
            TODO("Для пропорциональных шрифтов наверное имеет смысл сохранять в реестре оптимальный lfWidth")
            ZeroStruct(tm);
            BOOL lbTM = GetTextMetrics(hDC, &tm);
			_ASSERTE(lbTM);
            if (isForceMonospace)
                //Maximus - у Arial'а например MaxWidth слишком большой
                inFont->lfWidth = FontSizeX3 ? FontSizeX3 : tm.tmMaxCharWidth;
            else
                inFont->lfWidth = tm.tmAveCharWidth;
            inFont->lfHeight = tm.tmHeight; TODO("Здесь нужно обновить реальный размер шрифта в диалоге настройки!");
			if (lbTM && tm.tmCharSet != DEFAULT_CHARSET) {
				inFont->lfCharSet = tm.tmCharSet;
				for (uint i=0; i < countof(ChSets); i++)
				{
					if (chSetsNums[i] == tm.tmCharSet) {
						SendDlgItemMessage(hMain, tFontCharset, CB_SETCURSEL, i, 0);
						break;
					}
				}
			}
			
            if (ghOpWnd) // устанавливать только при листании шрифта в настройке
                UpdateTTF ( (tm.tmMaxCharWidth - tm.tmAveCharWidth)>2 );

            SelectObject(hDC, hOldF);
            DeleteDC(hDC);
        }
        ReleaseDC(0, hScreenDC);

		if (mh_Font2) { DeleteObject(mh_Font2); mh_Font2 = NULL; }

		//int width = FontSizeX2 ? FontSizeX2 : inFont->lfWidth;
		LogFont2.lfWidth = FontSizeX2 ? FontSizeX2 : inFont->lfWidth;
		LogFont2.lfHeight = abs(inFont->lfHeight);
		// Иначе рамки прерывистыми получаются... поставил NONANTIALIASED_QUALITY
		mh_Font2 = CreateFont(LogFont2.lfHeight, LogFont2.lfWidth, 0, 0, FW_NORMAL,
			0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, 
			NONANTIALIASED_QUALITY/*ANTIALIASED_QUALITY*/, 0, LogFont2.lfFaceName);
    }

    return hFont;
}

// FALSE - выключена
// TRUE (BST_CHECKED) - включена
// BST_INDETERMINATE (2) - 3-d state
int CSettings::IsChecked(HWND hParent, WORD nCtrlId)
{
	#ifdef _DEBUG
	HWND hChild = GetDlgItem(hParent, nCtrlId);
	_ASSERTE(hChild!=NULL);
	#endif
	// Аналог IsDlgButtonChecked
	int nChecked = SendDlgItemMessage(hParent, nCtrlId, BM_GETCHECK, 0, 0);
	_ASSERTE(nChecked==0 || nChecked==1 || nChecked==2);
	if (nChecked!=0 && nChecked!=1 && nChecked!=2)
		nChecked = 0;
	return nChecked;
}

int CSettings::GetNumber(HWND hParent, WORD nCtrlId)
{
	#ifdef _DEBUG
	HWND hChild = GetDlgItem(hParent, nCtrlId);
	_ASSERTE(hChild!=NULL);
	#endif
	int nValue = 0;
	wchar_t szNumber[32] = {0};
	if (GetDlgItemText(hParent, nCtrlId, szNumber, countof(szNumber)))
		nValue = klatoi(szNumber);
	return nValue;
}

int CSettings::SelectString(HWND hParent, WORD nCtrlId, LPCWSTR asText)
{
	#ifdef _DEBUG
	HWND hChild = GetDlgItem(hParent, nCtrlId);
	_ASSERTE(hChild!=NULL);
	#endif
	// Осуществляет поиск по _началу_ (!) строки
	int nIdx = SendDlgItemMessage(hParent, nCtrlId, CB_SELECTSTRING, -1, (LPARAM)asText);
	return nIdx;
}

int CSettings::SelectStringExact(HWND hParent, WORD nCtrlId, LPCWSTR asText)
{
	#ifdef _DEBUG
	HWND hChild = GetDlgItem(hParent, nCtrlId);
	_ASSERTE(hChild!=NULL);
	#endif
	int nIdx = SendDlgItemMessage(hParent, nCtrlId, CB_FINDSTRINGEXACT, -1, (LPARAM)asText);
	if (nIdx < 0) {
		int nCount = SendDlgItemMessage(hParent, nCtrlId, CB_GETCOUNT, 0, 0);
		int nNewVal = _wtol(asText);
		wchar_t temp[MAX_PATH];
		for (int i = 0; i < nCount; i++) {
			if (!SendDlgItemMessage(hParent, nCtrlId, CB_GETLBTEXT, i, (LPARAM)temp)) break;
			int nCurVal = _wtol(temp);
			if (nCurVal == nNewVal) {
				nIdx = i;
				break;
			} else
			if (nCurVal > nNewVal) {
				nIdx = SendDlgItemMessage(hParent, nCtrlId, CB_INSERTSTRING, i, (LPARAM) asText);
				break;
			}
		}
		if (nIdx < 0)
			nIdx = SendDlgItemMessage(hParent, nCtrlId, CB_INSERTSTRING, 0, (LPARAM) asText);
	}
	if (nIdx >= 0)
		SendDlgItemMessage(hParent, nCtrlId, CB_SETCURSEL, nIdx, 0);
	else
		SetDlgItemText(hParent, nCtrlId, asText);
	return nIdx;
}

// "Умолчательная" высота буфера.
// + ConEmu стартует в буферном режиме
// + команда по умолчанию (если не задана в реестре или ком.строке) будет "cmd", а не "far"
void CSettings::SetArgBufferHeight(int anBufferHeight)
{
	_ASSERTE(anBufferHeight>=0);
	if (anBufferHeight>=0) DefaultBufferHeight = anBufferHeight;
	ForceBufferHeight = (DefaultBufferHeight != 0);
	wcscpy(szDefCmd, ForceBufferHeight ? L"far" : L"cmd");
}

LPCTSTR CSettings::GetDefaultCmd()
{
	return szDefCmd;
}

LPCTSTR CSettings::GetCmd()
{
    if (psCurCmd && *psCurCmd)
        return psCurCmd;
    if (psCmd && *psCmd)
        return psCmd;
    SafeFree(psCurCmd); // впринципе, эта строка скорее всего не нужна, но на всякий случай...
    psCurCmd = wcsdup(szDefCmd);
    return psCurCmd;
}

LONG CSettings::FontWidth()
{
	_ASSERTE(LogFont.lfWidth);
	return LogFont.lfWidth;
}

LONG CSettings::FontHeight()
{
	_ASSERTE(LogFont.lfHeight);
	return LogFont.lfHeight;
}

LONG CSettings::BorderFontWidth()
{
	_ASSERTE(LogFont2.lfWidth);
	return LogFont2.lfWidth;
}

BYTE CSettings::FontCharSet()
{
	return LogFont.lfCharSet;
}

void CSettings::RegisterTabs()
{
	if (!mb_TabHotKeyRegistered) {
		if (RegisterHotKey(ghOpWnd, 0x101, MOD_CONTROL, VK_TAB))
			mb_TabHotKeyRegistered = TRUE;
		RegisterHotKey(ghOpWnd, 0x102, MOD_CONTROL|MOD_SHIFT, VK_TAB);
	}
}

void CSettings::UnregisterTabs()
{
	if (mb_TabHotKeyRegistered) {
		UnregisterHotKey(ghOpWnd, 0x101);
		UnregisterHotKey(ghOpWnd, 0x102);
	}
	mb_TabHotKeyRegistered = FALSE;
}

// Если asFontFile НЕ NULL - значит его пользователь указал через /fontfile
BOOL CSettings::RegisterFont(LPCWSTR asFontFile, BOOL abDefault)
{
	// Обработка параметра /fontfile
	_ASSERTE(asFontFile && *asFontFile);
	
	if (mb_StopRegisterFonts) return FALSE;
	
	for (std::vector<RegFont>::iterator iter = m_RegFonts.begin(); iter != m_RegFonts.end(); iter++)
	{
		if (StrCmpI(iter->szFontFile, asFontFile) == 0) {
			// Уже добавлено
			if (abDefault && iter->bDefault == FALSE) iter->bDefault = TRUE;
			
			return TRUE;
		}
	}

	RegFont rf = {abDefault};
	
	if (!GetFontNameFromFile(asFontFile, rf.szFontName)) {
		//DWORD dwErr = GetLastError();
		TCHAR* psz=(TCHAR*)calloc(wcslen(asFontFile)+100,sizeof(TCHAR));
		lstrcpyW(psz, L"Can't retrieve font family from file:\n");
		lstrcatW(psz, asFontFile);
		lstrcatW(psz, L"\nContinue?");
		int nBtn = MessageBox(NULL, psz, L"ConEmu", MB_OKCANCEL|MB_ICONSTOP);
		free(psz);
		if (nBtn == IDCANCEL) {
			mb_StopRegisterFonts = TRUE;
			return FALSE;
		}
		return TRUE; // продолжить со следующим файлом
	}

	if (!AddFontResourceEx(asFontFile, FR_PRIVATE, NULL)) //ADD fontname; by Mors
	{
		TCHAR* psz=(TCHAR*)calloc(wcslen(asFontFile)+100,sizeof(TCHAR));
		lstrcpyW(psz, L"Can't register font:\n");
		lstrcatW(psz, asFontFile);
		lstrcatW(psz, L"\nContinue?");
		int nBtn = MessageBox(NULL, psz, L"ConEmu", MB_OKCANCEL|MB_ICONSTOP);
		free(psz);
		if (nBtn == IDCANCEL) {
			mb_StopRegisterFonts = TRUE;
			return FALSE;
		}
		return TRUE; // продолжить со следующим файлом
	}
	
	lstrcpy(rf.szFontFile, asFontFile);
	// Теперь его нужно добавить в вектор независимо от успешности определения рамок
	// будет нужен RemoveFontResourceEx(asFontFile, FR_PRIVATE, NULL);
	
	// Определить наличие рамок и "юникодности" шрифта
	HDC hdc = CreateCompatibleDC(0);
	if (hdc) {
		HFONT hf = CreateFont ( 18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
							    OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, 
								NONANTIALIASED_QUALITY/*ANTIALIASED_QUALITY*/, 0,
								rf.szFontName);
		if (hf) {
			HFONT hOldF = (HFONT)SelectObject(hdc, hf);
			
			LPGLYPHSET pSets = NULL;
			DWORD nSize = GetFontUnicodeRanges ( hdc, NULL );
			if (nSize) {
				pSets = (LPGLYPHSET)calloc(nSize,1);
				if (pSets) {
					pSets->cbThis = nSize;
					if (GetFontUnicodeRanges ( hdc, pSets )) {
						rf.bUnicode = (pSets->flAccel != 1/*GS_8BIT_INDICES*/);
						// Поиск рамок
						if (rf.bUnicode) {
							for (DWORD r = 0; r < pSets->cRanges; r++) {
								if (pSets->ranges[r].wcLow < ucBoxDblDownRight 
								    && (pSets->ranges[r].wcLow + pSets->ranges[r].cGlyphs - 1) > ucBoxDblDownRight)
								{
									rf.bHasBorders = TRUE; break;
								}
							}
						} else {
							_ASSERTE(rf.bUnicode);
						}
					}
					free(pSets);
				}
			}
			
			SelectObject(hdc, hOldF);
			DeleteObject(hf);
		}
		DeleteDC(hdc);
	}
	
	
	// Запомнить шрифт
	m_RegFonts.push_back(rf);
	return TRUE;
}

void CSettings::RegisterFonts()	
{
	if (!isAutoRegisterFonts)
		return; // Если поиск шрифтов не требуется

	// Регистрация шрифтов в папке ConEmu
	WIN32_FIND_DATA fnd;
	wchar_t szFind[MAX_PATH]; wcscpy(szFind, gConEmu.ms_ConEmuExe);
	wchar_t *pszSlash = wcsrchr(szFind, L'\\');

	if (!pszSlash) {
		MessageBox(NULL, L"ms_ConEmuExe does not contains '\\'", L"ConEmu", MB_OK|MB_ICONSTOP);
	} else {
		wcscpy(pszSlash, L"\\*.ttf");
		HANDLE hFind = FindFirstFile(szFind, &fnd);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
					pszSlash[1] = 0;
					if ((wcslen(fnd.cFileName)+wcslen(szFind)) >= MAX_PATH) {
						TCHAR* psz=(TCHAR*)calloc(wcslen(fnd.cFileName)+100,sizeof(TCHAR));
						lstrcpyW(psz, L"Too long full pathname for font:\n");
						lstrcatW(psz, fnd.cFileName);
						int nBtn = MessageBox(NULL, psz, L"ConEmu", MB_OKCANCEL|MB_ICONSTOP);
						free(psz);
						if (nBtn == IDCANCEL) break;
					} else {
						wcscat(szFind, fnd.cFileName);
						
						if (!RegisterFont(szFind, FALSE))
							break;
					}
				}
			} while (FindNextFile(hFind, &fnd));
			FindClose(hFind);
		}
	}
	
	// Теперь можно смотреть, зарегистрились ли какие-то шрифты... И выбрать из них подходящие
	// Это делается в InitFont
}

void CSettings::UnregisterFonts()
{
	for (std::vector<RegFont>::iterator iter = m_RegFonts.begin(); 
	     iter != m_RegFonts.end(); iter = m_RegFonts.erase(iter))
	{
		RemoveFontResourceEx(iter->szFontFile, FR_PRIVATE, NULL);
	}
}

BOOL CSettings::GetFontNameFromFile(LPCTSTR lpszFilePath, LPTSTR rsFontName/* [32] */)
{
	typedef struct _tagTT_OFFSET_TABLE{
		USHORT	uMajorVersion;
		USHORT	uMinorVersion;
		USHORT	uNumOfTables;
		USHORT	uSearchRange;
		USHORT	uEntrySelector;
		USHORT	uRangeShift;
	}TT_OFFSET_TABLE;

	typedef struct _tagTT_TABLE_DIRECTORY{
		char	szTag[4];			//table name
		ULONG	uCheckSum;			//Check sum
		ULONG	uOffset;			//Offset from beginning of file
		ULONG	uLength;			//length of the table in bytes
	}TT_TABLE_DIRECTORY;

	typedef struct _tagTT_NAME_TABLE_HEADER{
		USHORT	uFSelector;			//format selector. Always 0
		USHORT	uNRCount;			//Name Records count
		USHORT	uStorageOffset;		//Offset for strings storage, from start of the table
	}TT_NAME_TABLE_HEADER;

	typedef struct _tagTT_NAME_RECORD{
		USHORT	uPlatformID;
		USHORT	uEncodingID;
		USHORT	uLanguageID;
		USHORT	uNameID;
		USHORT	uStringLength;
		USHORT	uStringOffset;	//from start of storage area
	}TT_NAME_RECORD;

	#define SWAPWORD(x)		MAKEWORD(HIBYTE(x), LOBYTE(x))
	#define SWAPLONG(x)		MAKELONG(SWAPWORD(HIWORD(x)), SWAPWORD(LOWORD(x)))

	BOOL lbRc = FALSE;
	HANDLE f = NULL;
	wchar_t szRetVal[MAX_PATH];
	DWORD dwRead;

	//if(f.Open(lpszFilePath, CFile::modeRead|CFile::shareDenyWrite)){
	if ((f = CreateFile(lpszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL))!=INVALID_HANDLE_VALUE)
	{
		TT_OFFSET_TABLE ttOffsetTable;
		//f.Read(&ttOffsetTable, sizeof(TT_OFFSET_TABLE));
		if (ReadFile(f, &ttOffsetTable, sizeof(TT_OFFSET_TABLE), &(dwRead=0), NULL) && dwRead)
		{
			ttOffsetTable.uNumOfTables = SWAPWORD(ttOffsetTable.uNumOfTables);
			ttOffsetTable.uMajorVersion = SWAPWORD(ttOffsetTable.uMajorVersion);
			ttOffsetTable.uMinorVersion = SWAPWORD(ttOffsetTable.uMinorVersion);

			//check is this is a true type font and the version is 1.0
			if(ttOffsetTable.uMajorVersion != 1 || ttOffsetTable.uMinorVersion != 0)
				return FALSE;
			
			TT_TABLE_DIRECTORY tblDir;
			BOOL bFound = FALSE;
			
			for(int i=0; i< ttOffsetTable.uNumOfTables; i++){
				//f.Read(&tblDir, sizeof(TT_TABLE_DIRECTORY));
				if (ReadFile(f, &tblDir, sizeof(TT_TABLE_DIRECTORY), &(dwRead=0), NULL) && dwRead)
				{
					//strncpy(szRetVal, tblDir.szTag, 4); szRetVal[4] = 0;
					//if(lstrcmpi(szRetVal, L"name") == 0)
					//if (memcmp(tblDir.szTag, "name", 4) == 0)
					if (strnicmp(tblDir.szTag, "name", 4) == 0)
					{
						bFound = TRUE;
						tblDir.uLength = SWAPLONG(tblDir.uLength);
						tblDir.uOffset = SWAPLONG(tblDir.uOffset);
						break;
					}
				}
			}
			
			if(bFound){
				if (SetFilePointer(f, tblDir.uOffset, NULL, FILE_BEGIN)!=INVALID_SET_FILE_POINTER)
				{
					TT_NAME_TABLE_HEADER ttNTHeader;
					//f.Read(&ttNTHeader, sizeof(TT_NAME_TABLE_HEADER));
					if (ReadFile(f, &ttNTHeader, sizeof(TT_NAME_TABLE_HEADER), &(dwRead=0), NULL) && dwRead)
					{
						ttNTHeader.uNRCount = SWAPWORD(ttNTHeader.uNRCount);
						ttNTHeader.uStorageOffset = SWAPWORD(ttNTHeader.uStorageOffset);
						TT_NAME_RECORD ttRecord;
						bFound = FALSE;
						
						for(int i=0; i<ttNTHeader.uNRCount; i++){
							//f.Read(&ttRecord, sizeof(TT_NAME_RECORD));
							if (ReadFile(f, &ttRecord, sizeof(TT_NAME_RECORD), &(dwRead=0), NULL) && dwRead)
							{
								ttRecord.uNameID = SWAPWORD(ttRecord.uNameID);
								if(ttRecord.uNameID == 1){
									ttRecord.uStringLength = SWAPWORD(ttRecord.uStringLength);
									ttRecord.uStringOffset = SWAPWORD(ttRecord.uStringOffset);
									//int nPos = f.GetPosition();
									DWORD nPos = SetFilePointer(f, 0, 0, FILE_CURRENT);
									//f.Seek(tblDir.uOffset + ttRecord.uStringOffset + ttNTHeader.uStorageOffset, CFile::begin);
									if (SetFilePointer(f, tblDir.uOffset + ttRecord.uStringOffset + ttNTHeader.uStorageOffset, 0, FILE_BEGIN)!=INVALID_SET_FILE_POINTER)
									{
										if ((ttRecord.uStringLength + 1) < 33)
										{
											//f.Read(csTemp.GetBuffer(ttRecord.uStringLength + 1), ttRecord.uStringLength);
											//csTemp.ReleaseBuffer();
											char szName[MAX_PATH]; szName[ttRecord.uStringLength + 1] = 0;
											if (ReadFile(f, szName, ttRecord.uStringLength + 1, &(dwRead=0), NULL) && dwRead)
											{
												//if(csTemp.GetLength() > 0){
												if (szName[0]) {
													szName[ttRecord.uStringLength + 1] = 0;
													for (int j = ttRecord.uStringLength; j >= 0 && szName[j] == ' '; j--)
														szName[j] = 0;
													if (szName[0]) {
														MultiByteToWideChar(CP_ACP, 0, szName, -1, szRetVal, 32);
														szRetVal[31] = 0;
														lbRc = TRUE;
													}
													break;
												}
											}
										}
									}
									//f.Seek(nPos, CFile::begin);
									SetFilePointer(f, nPos, 0, FILE_BEGIN);
								}
							}
						}
					}
				}			
			}
		}
		CloseHandle(f);
	}
	
	if (lbRc)
		wcscpy(rsFontName, szRetVal);
	return lbRc;
}

void CSettings::HistoryCheck()
{
	if (!psCmdHistory || !*psCmdHistory) {
		nCmdHistorySize = 0;
	} else {
		const wchar_t* psz = psCmdHistory;
		while (*psz)
			psz += lstrlen(psz)+1;

		if (psz == psCmdHistory)
			nCmdHistorySize = 0;
		else
			nCmdHistorySize = (psz - psCmdHistory + 1)*sizeof(wchar_t);
	}
}

void CSettings::HistoryAdd(LPCWSTR asCmd)
{
	if (!asCmd || !*asCmd)
		return;
	if (psCmd && lstrcmp(psCmd, asCmd)==0)
		return;
	if (psCurCmd && lstrcmp(psCurCmd, asCmd)==0)
		return;

	HEAPVAL

	wchar_t *pszNewHistory, *psz, *pszOld;
	int nCount = 0;

	DWORD nNewSize = nCmdHistorySize + (DWORD)((lstrlen(asCmd) + 2)*sizeof(wchar_t));
	pszNewHistory = (wchar_t*)malloc(nNewSize);
	//wchar_t* pszEnd = pszNewHistory + nNewSize/sizeof(wchar_t);
	if (!pszNewHistory) return;

	lstrcpy(pszNewHistory, asCmd);
	psz = pszNewHistory + lstrlen(pszNewHistory) + 1;
	nCount++;
	pszOld = psCmdHistory;
	int nLen;
	HEAPVAL
	while (nCount < MAX_CMD_HISTORY && *pszOld /*&& psz < pszEnd*/) {
		const wchar_t *pszCur = pszOld;
		pszOld += lstrlen(pszOld) + 1;

		if (lstrcmp(pszCur, asCmd) == 0)
			continue;

		lstrcpy(psz, pszCur);
		psz += (nLen = (lstrlen(psz)+1));
		nCount ++;
	}
	*psz = 0;

	HEAPVAL

	free(psCmdHistory);
	psCmdHistory = pszNewHistory;
	nCmdHistorySize = (psz - pszNewHistory + 1)*sizeof(wchar_t);

	HEAPVAL

	// И сразу сохранить в настройках
	Registry reg;
	if (reg.OpenKey(Config, KEY_WRITE)) {
		HEAPVAL
		reg.SaveMSZ(L"CmdLineHistory", psCmdHistory, nCmdHistorySize);
		HEAPVAL
		reg.CloseKey();
	}
}

LPCWSTR CSettings::HistoryGet()
{
	if (psCmdHistory && *psCmdHistory)
		return psCmdHistory;
	return NULL;
}

// Показать в "Инфо" текущий режим консоли
void CSettings::UpdateConsoleMode(DWORD nMode)
{
	if (hInfo && IsWindow(hInfo)) {
		wchar_t szInfo[255];
		wsprintf(szInfo, L"Console states (0x%X)", nMode);
		SetDlgItemText(hInfo, IDC_CONSOLE_STATES, szInfo);
	}
}
