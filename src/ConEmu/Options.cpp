
#include "Header.h"
#include <commctrl.h>
extern "C" {
#include "../common/ConEmuCheck.h"
}

#define COUNTER_REFRESH 5000

const u8 chSetsNums[19] = {0, 178, 186, 136, 1, 238, 134, 161, 177, 129, 130, 77, 255, 204, 128, 2, 222, 162, 163};
int upToFontHeight=0;
HWND ghOpWnd=NULL;

namespace Settings {
    const WCHAR* szKeys[] = {L"<None>", L"Left Ctrl", L"Right Ctrl", L"Left Alt", L"Right Alt", L"Left Shift", L"Right Shift"};
    const DWORD  nKeys[] =  {0,         VK_LCONTROL,  VK_RCONTROL,   VK_LMENU,    VK_RMENU,     VK_LSHIFT,     VK_RSHIFT};
    const BYTE   FSizes[] = {0, 8, 10, 12, 14, 16, 18, 19, 20, 24, 26, 28, 30, 32, 34, 36, 40, 46, 50, 52, 72};
};

CSettings::CSettings()
{
    InitSettings();
    mb_IgnoreEditChanged = FALSE;
    mb_IgnoreTtfChange = TRUE;
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
    mh_Uxtheme = LoadLibrary(_T("UxTheme.dll"));
    if (mh_Uxtheme) {
        SetWindowThemeF = (SetWindowThemeT)GetProcAddress(mh_Uxtheme, "SetWindowTheme");
        EnableThemeDialogTextureF = (EnableThemeDialogTextureT)GetProcAddress(mh_Uxtheme, "EnableThemeDialogTexture");
        //if (SetWindowThemeF) { SetWindowThemeF(Progressbar1, _T(" "), _T(" ")); }
    }

	mn_MsgUpdateCounter = RegisterWindowMessage(_T("ConEmuSettings::Counter"));
	mn_MsgRecreateFont = RegisterWindowMessage(_T("CSettings::RecreateFont"));
}

CSettings::~CSettings()
{
    if (mh_Font) { DeleteObject(mh_Font); mh_Font = NULL; }
    if (mh_Font2) { DeleteObject(mh_Font2); mh_Font2 = NULL; }
    if (psCmd) {free(psCmd); psCmd = NULL;}
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
    _tcscpy(Config, _T("Software\\ConEmu"));

	FontFile[0] = 0;
    
    psCmd = NULL; psCurCmd = NULL; wcscpy(szDefCmd, L"far");
    isMulti = true; icMultiNew = 'W'; icMultiNext = 'Q'; icMultiRecreate = 192/*VK_тильда*/; isMultiNewConfirm = true;
    // Logging
    isAdvLogging = 0;
	//wcscpy(szDumpPackets, L"c:\\temp\\ConEmuVCon-%i-%i.dat");

    nMainTimerElapse = 10;
    nAffinity = 3;
    //isAdvLangChange = true;
    isSkipFocusEvents = false;
    //isLangChangeWsPlugin = false;
	isMonitorConsoleLang = 3;
    DefaultBufferHeight = 1000;
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
    _tcscpy(LogFont.lfFaceName, _T("Lucida Console"));
    _tcscpy(LogFont2.lfFaceName, _T("Lucida Console"));
    isTryToCenter = false;
    isTabFrame = true;
    isForceMonospace = false; isProportional = false;
    isMinToTray = false;

	ConsoleFont.lfHeight = 6;
	ConsoleFont.lfWidth = 4;
	_tcscpy(ConsoleFont.lfFaceName, _T("Lucida Console"));

    
    Registry RegConColors, RegConDef;
    if (RegConColors.OpenKey(_T("Console"), KEY_READ))
    {
        RegConDef.OpenKey(HKEY_USERS, _T(".DEFAULT\\Console"), KEY_READ);

        TCHAR ColorName[] = _T("ColorTable00");
        for (uint i = 0x10; i--;)
        {
            ColorName[10] = i/10 + '0';
            ColorName[11] = i%10 + '0';
            if (!RegConColors.Load(ColorName, Colors[i]))
                RegConDef.Load(ColorName, Colors[i]);
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
    _tcscpy(sBgImage, _T("c:\\back.bmp"));
	bgImageDarker = 0x46;

    isFixFarBorders = TRUE; isPartBrush75 = 0xC8; isPartBrush50 = 0x96; isPartBrush25 = 0x5A;
	memset(icFixFarBorderRanges, 0, sizeof(icFixFarBorderRanges));
	icFixFarBorderRanges[0].bUsed = true; icFixFarBorderRanges[0].cBegin = 0x2013; icFixFarBorderRanges[0].cEnd = 0x25C4;
    
    wndHeight = ntvdmHeight = 25; // NightRoman
    wndWidth = 80;  // NightRoman
    wndX = 0; wndY = 0; wndCascade = true;
    isConVisible = false;
    nSlideShowElapse = 2500;
    nIconID = IDI_ICON1;
    isRClickSendKey = 2;
    sRClickMacro = NULL;
    _tcscpy(szTabConsole, _T("%s"));
    //pszTabConsole = _tcscpy(szTabPanels+_tcslen(szTabPanels)+1, _T("Console"));
    _tcscpy(szTabEditor, _T("[%s]"));
    _tcscpy(szTabEditorModified, _T("[%s] *"));
    /* */ _tcscpy(szTabViewer, _T("{%s}"));
    nTabLenMax = 20;
    
    isCursorV = true;
	isCursorBlink = true;
	isCursorColor = false;
    
    isTabs = 1; isTabSelf = true; isTabRecent = true; isTabLazy = true;
	sTabCloseMacro = NULL;
    
    isVisualizer = false;
    nVizNormal = 1; nVizFore = 15; nVizTab = 15; nVizEOL = 8; nVizEOF = 12;
    cVizTab = 0x2192; cVizEOL = 0x2193; cVizEOF = 0x2640;

    isAllowDetach = 0;
    isCreateAppWindow = false;
    isScrollTitle = true;
    ScrollTitleLen = 22;
    
	isRSelFix = true;
    isDragEnabled = DRAG_L_ALLOWED; isDropEnabled = (BYTE)1;
    nLDragKey = 0; nRDragKey = VK_LCONTROL; isDnDsteps = true; isDefCopy = true;
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
    DWORD FontCharSet = LogFont.lfCharSet;
    bool isBold = (LogFont.lfWeight>=FW_BOLD), isItalic = (LogFont.lfItalic!=FALSE);
    
//------------------------------------------------------------------------
///| Loading from registry |//////////////////////////////////////////////
//------------------------------------------------------------------------
    Registry reg;
    if (reg.OpenKey(Config, KEY_READ)) // NightRoman
    {
        TCHAR ColorName[] = _T("ColorTable00");
        for (uint i = 0x20; i--;)
        {
            ColorName[10] = i/10 + '0';
            ColorName[11] = i%10 + '0';
            reg.Load(ColorName, Colors[i]);
        }
        memmove(acrCustClr, Colors, sizeof(COLORREF)*16);
        reg.Load(_T("ExtendColors"), isExtendColors);
        reg.Load(_T("ExtendColorIdx"), nExtendColor);
            if (nExtendColor<0 || nExtendColor>15) nExtendColor=14;

		// Debugging
		reg.Load(_T("ConVisible"), isConVisible);
		//reg.Load(_T("DumpPackets"), szDumpPackets);
        reg.Load(_T("FontName"), inFont);
        reg.Load(_T("FontName2"), inFont2);
        reg.Load(_T("CmdLine"), &psCmd);
        reg.Load(_T("Multi"), isMulti);
			reg.Load(_T("Multi.NewConsole"), icMultiNew);
			reg.Load(_T("Multi.Next"), icMultiNext);
			reg.Load(_T("Multi.Recreate"), icMultiRecreate);
			reg.Load(_T("Multi.NewConfirm"), isMultiNewConfirm);

        reg.Load(_T("FontSize"), inSize);
        reg.Load(_T("FontSizeX"), FontSizeX);
        reg.Load(_T("FontSizeX3"), FontSizeX3);
        reg.Load(_T("FontSizeX2"), FontSizeX2);
        reg.Load(_T("FontCharSet"), FontCharSet);
        reg.Load(_T("Anti-aliasing"), Quality);

		reg.Load(L"ConsoleFontName", ConsoleFont.lfFaceName);
		reg.Load(L"ConsoleCharWidth", ConsoleFont.lfWidth);
		reg.Load(L"ConsoleFontHeight", ConsoleFont.lfHeight);

        reg.Load(_T("WindowMode"), gConEmu.WindowMode);
        reg.Load(_T("ConWnd X"), wndX); /*if (wndX<-10) wndX = 0;*/
        reg.Load(_T("ConWnd Y"), wndY); /*if (wndY<-10) wndY = 0;*/
		// ЭТО не влияет на szDefCmd. Только прямое указание флажка "/BufferHeight N" 
		// может сменить (умолчательную) команду запуска на "cmd" или "far"
        reg.Load(_T("Cascaded"), wndCascade);
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
        reg.Load(_T("ConWnd Width"), wndWidth); if (!wndWidth) wndWidth = 80; else if (wndWidth>1000) wndWidth = 1000;
        reg.Load(_T("ConWnd Height"), wndHeight); if (!wndHeight) wndHeight = 25; else if (wndHeight>500) wndHeight = 500;
        //TODO: Эти два параметра не сохраняются
        reg.Load(_T("16it Height"), ntvdmHeight); if (ntvdmHeight<20) ntvdmHeight = 20; else if (ntvdmHeight>100) ntvdmHeight = 100;
		reg.Load(_T("DefaultBufferHeight"), DefaultBufferHeight); if (DefaultBufferHeight < 300) DefaultBufferHeight = 300;

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
        
        reg.Load(_T("PartBrush75"), isPartBrush75); if (isPartBrush75<5) isPartBrush75=5; else if (isPartBrush75>250) isPartBrush75=250;
        reg.Load(_T("PartBrush50"), isPartBrush50); if (isPartBrush50<5) isPartBrush50=5; else if (isPartBrush50>250) isPartBrush50=250;
        reg.Load(_T("PartBrush25"), isPartBrush25); if (isPartBrush25<5) isPartBrush25=5; else if (isPartBrush25>250) isPartBrush25=250;
        if (isPartBrush50>=isPartBrush75) isPartBrush50=isPartBrush75-10;
        if (isPartBrush25>=isPartBrush50) isPartBrush25=isPartBrush50-10;
        
        reg.Load(_T("RightClick opens context menu"), isRClickSendKey);
        	reg.Load(L"RightClickMacro", &sRClickMacro);
        reg.Load(_T("AltEnter"), isSentAltEnter);
        reg.Load(_T("Min2Tray"), isMinToTray);

        reg.Load(_T("BackGround Image show"), isShowBgImage);
			if (isShowBgImage!=0 && isShowBgImage!=1 && isShowBgImage!=2) isShowBgImage = 0;
		reg.Load(_T("BackGround Image"), sBgImage);
		reg.Load(_T("bgImageDarker"), bgImageDarker);

        reg.Load(_T("FontBold"), isBold);
        reg.Load(_T("FontItalic"), isItalic);
        reg.Load(_T("ForceMonospace"), isForceMonospace);
        reg.Load(_T("Proportional"), isProportional);
        reg.Load(_T("Update Console handle"), isUpdConHandle);
		reg.Load(_T("RSelectionFix"), isRSelFix);
        reg.Load(_T("Dnd"), isDragEnabled); 
        isDropEnabled = (BYTE)(isDragEnabled ? 1 : 0); // ранее "DndDrop" не было, поэтому ставим default
        reg.Load(_T("DndLKey"), nLDragKey);
        reg.Load(_T("DndRKey"), nRDragKey);
        reg.Load(_T("DndDrop"), isDropEnabled);
        reg.Load(_T("DefCopy"), isDefCopy);
        reg.Load(_T("DndSteps"), isDnDsteps);
        reg.Load(_T("GUIpb"), isGUIpb);
        reg.Load(_T("Tabs"), isTabs);
	        reg.Load(_T("TabSelf"), isTabSelf);
	        reg.Load(_T("TabLazy"), isTabLazy);
	        reg.Load(_T("TabRecent"), isTabRecent);
			if (!reg.Load(_T("TabCloseMacro"), &sTabCloseMacro) && sTabCloseMacro && !*sTabCloseMacro) { free(sTabCloseMacro); sTabCloseMacro = NULL; }
        reg.Load(_T("TabFrame"), isTabFrame);
        reg.Load(_T("TabMargins"), rcTabMargins);
        reg.Load(_T("SlideShowElapse"), nSlideShowElapse);
        reg.Load(_T("IconID"), nIconID);
        reg.Load(_T("TabConsole"), szTabConsole);
            //WCHAR* pszVert = wcschr(szTabPanels, L'|');
            //if (!pszVert) {
            //    if (wcslen(szTabPanels)>54) szTabPanels[54] = 0;
            //    pszVert = szTabPanels + wcslen(szTabPanels);
            //    wcscpy(pszVert+1, L"Console");
            //}
            //*pszVert = 0; pszTabConsole = pszVert+1;
        reg.Load(_T("TabEditor"), szTabEditor);
        reg.Load(_T("TabEditorModified"), szTabEditorModified);
        reg.Load(_T("TabViewer"), szTabViewer);
        reg.Load(_T("TabLenMax"), nTabLenMax);
        reg.Load(_T("ScrollTitle"), isScrollTitle);
        reg.Load(_T("ScrollTitleLen"), ScrollTitleLen);
        reg.Load(_T("TryToCenter"), isTryToCenter);
        //reg.Load(_T("CreateAppWindow"), isCreateAppWindow);
        //reg.Load(_T("AllowDetach"), isAllowDetach);
        
        reg.Load(_T("Visualizer"), isVisualizer);
        reg.Load(_T("VizNormal"), nVizNormal);
        reg.Load(_T("VizFore"), nVizFore);
        reg.Load(_T("VizTab"), nVizTab);
        reg.Load(_T("VizEol"), nVizEOL);
        reg.Load(_T("VizEof"), nVizEOF);
        reg.Load(_T("VizTabCh"), cVizTab);
        reg.Load(_T("VizEolCh"), cVizEOL);
        reg.Load(_T("VizEofCh"), cVizEOF);
        
        reg.Load(_T("MainTimerElapse"), nMainTimerElapse); if (nMainTimerElapse>1000) nMainTimerElapse = 1000;
        reg.Load(_T("AffinityMask"), nAffinity);
        //reg.Load(_T("AdvLangChange"), isAdvLangChange);
        reg.Load(_T("SkipFocusEvents"), isSkipFocusEvents);
        //reg.Load(_T("LangChangeWsPlugin"), isLangChangeWsPlugin);
		reg.Load(_T("MonitorConsoleLang"), isMonitorConsoleLang);
        
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
    LogFont.lfCharSet = (BYTE) FontCharSet;
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
	}
	if (anFontHeight!=-1) {
		LogFont.lfHeight = anFontHeight;
		LogFont.lfWidth = 0;
	}
	if (anQuality!=-1) {
		LogFont.lfQuality = ANTIALIASED_QUALITY;
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
        reg.Save(_T("TabMargins"), rcTabMargins);
        reg.CloseKey();
    }
}

BOOL CSettings::SaveSettings()
{
    Registry reg;
    //if (reg.OpenKey(_T("Console"), KEY_WRITE))
    //{
    //    TCHAR ColorName[] = _T("ColorTable00");
    //    for (uint i = 0x10; i--;)
    //    {
    //        ColorName[10] = i/10 + '0';
    //        ColorName[11] = i%10 + '0';
    //        reg.Save(ColorName, (DWORD)Colors[i]);
    //    }
    //    reg.CloseKey();

      if (reg.OpenKey(Config, KEY_WRITE)) // NightRoman
        {
            TCHAR ColorName[] = _T("ColorTable00");
            for (uint i = 0x20; i--;)
            {
                ColorName[10] = i/10 + '0';
                ColorName[11] = i%10 + '0';
                reg.Save(ColorName, (DWORD)Colors[i]);
            }
            reg.Save(_T("ExtendColors"), isExtendColors);
            reg.Save(_T("ExtendColorIdx"), nExtendColor);

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
            /*if (!isFullScreen && !IsZoomed(ghWnd) && !IsIconic(ghWnd))
            {
                RECT rcPos; GetWindowRect(ghWnd, &rcPos);
                wndX = rcPos.left;
                wndY = rcPos.top;
            }*/

            reg.Save(_T("CmdLine"), psCmd);
            reg.Save(_T("Multi"), isMulti);
				//reg.Save(_T("Multi.NewConsole"), icMultiNew);
				//reg.Save(_T("Multi.Next"), icMultiNext);
				//reg.Save(_T("Multi.NewConfirm"), isMultiNewConfirm);
            reg.Save(_T("FontName"), LogFont.lfFaceName);
            reg.Save(_T("FontName2"), LogFont2.lfFaceName);

            reg.Save(_T("BackGround Image"), sBgImage);
            reg.Save(_T("bgImageDarker"), bgImageDarker);
			reg.Save(_T("BackGround Image show"), isShowBgImage);

            reg.Save(_T("FontSize"), LogFont.lfHeight);
            reg.Save(_T("FontSizeX"), FontSizeX);
            reg.Save(_T("FontSizeX2"), FontSizeX2);
            reg.Save(_T("FontSizeX3"), FontSizeX3);
            reg.Save(_T("FontCharSet"), LogFont.lfCharSet);
            reg.Save(_T("Anti-aliasing"), LogFont.lfQuality);
            reg.Save(_T("WindowMode"), isFullScreen ? rFullScreen : IsZoomed(ghWnd) ? rMaximized : rNormal);
            
			reg.Save(_T("CursorType"), isCursorV);
            reg.Save(_T("CursorColor"), isCursorColor);
			reg.Save(L"CursorBlink", isCursorBlink);

            reg.Save(_T("FixFarBorders"), isFixFarBorders);
            reg.Save(_T("RightClick opens context menu"), isRClickSendKey);
            reg.Save(_T("AltEnter"), isSentAltEnter);
            reg.Save(_T("Min2Tray"), isMinToTray);
            reg.Save(_T("Dnd"), isDragEnabled);
            reg.Save(_T("DndLKey"), nLDragKey);
            reg.Save(_T("DndRKey"), nRDragKey);
            reg.Save(_T("DndDrop"), isDropEnabled);
            reg.Save(_T("DefCopy"), isDefCopy);

            reg.Save(_T("GUIpb"), isGUIpb);

            reg.Save(_T("Tabs"), isTabs);
		        reg.Save(_T("TabSelf"), isTabSelf);
		        reg.Save(_T("TabLazy"), isTabLazy);
		        reg.Save(_T("TabRecent"), isTabRecent);
            
            reg.Save(_T("FontBold"), LogFont.lfWeight == FW_BOLD);
            reg.Save(_T("FontItalic"), LogFont.lfItalic);
            reg.Save(_T("ForceMonospace"), isForceMonospace);
            reg.Save(_T("Proportional"), isProportional);
            reg.Save(_T("Update Console handle"), isUpdConHandle);

            reg.Save(_T("ConWnd Width"), wndWidth);
            reg.Save(_T("ConWnd Height"), wndHeight);
            reg.Save(_T("ConWnd X"), wndX);
            reg.Save(_T("ConWnd Y"), wndY);
            reg.Save(_T("Cascaded"), wndCascade);

            reg.Save(_T("ScrollTitle"), isScrollTitle);
            reg.Save(_T("ScrollTitleLen"), ScrollTitleLen);
            
            reg.Save(_T("Visualizer"), isVisualizer);
            reg.Save(_T("VizNormal"), nVizNormal);
            reg.Save(_T("VizFore"), nVizFore);
            reg.Save(_T("VizTab"), nVizTab);
            reg.Save(_T("VizEol"), nVizEOL);
            reg.Save(_T("VizEof"), nVizEOF);
            reg.Save(_T("VizTabCh"), cVizTab);
            reg.Save(_T("VizEolCh"), cVizEOL);
            reg.Save(_T("VizEofCh"), cVizEOF);
            
            
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

BOOL CALLBACK CSettings::EnumFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount)
{
    MCHKHEAP
    int far * aiFontCount = (int far *) aFontCount;

    // Record the number of raster, TrueType, and vector
    // fonts in the font-count array.

    if (FontType & RASTER_FONTTYPE)
        aiFontCount[0]++;
    else if (FontType & TRUETYPE_FONTTYPE)
        aiFontCount[2]++;
    else
        aiFontCount[1]++;

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

	RegisterTabs();

	mn_LastChangingFontCtrlId = 0;

	TCHAR szTitle[MAX_PATH]; szTitle[0]=0;
	int nConfLen = _tcslen(Config);
	int nStdLen = strlen("Software\\ConEmu");
	if (nConfLen>(nStdLen+1))
		wsprintf(szTitle, _T("Settings (%s)..."), (Config+nStdLen+1));
	else
		_tcscpy(szTitle, _T("Settings..."));
	SetWindowText ( ghOpWnd, szTitle );

	MCHKHEAP
    {
        TCITEM tie;
        HWND _hwndTab = GetDlgItem(ghOpWnd, tabMain);
        tie.mask = TCIF_TEXT;
        tie.iImage = -1; 
        tie.pszText = _T("Main");
        TabCtrl_InsertItem(_hwndTab, 0, &tie);
        tie.pszText = _T("Features");
        TabCtrl_InsertItem(_hwndTab, 1, &tie);
        tie.pszText = _T("Colors");
        TabCtrl_InsertItem(_hwndTab, 2, &tie);
        tie.pszText = _T("Info");
        TabCtrl_InsertItem(_hwndTab, 3, &tie);
        
        HFONT hFont = CreateFont(TAB_FONT_HEIGTH, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, 
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TAB_FONT_FACE);
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

	SetDlgItemText(hMain, tFontFace, LogFont.lfFaceName);
	SetDlgItemText(hMain, tFontFace2, LogFont2.lfFaceName);
	DWORD dwThId;
	mh_EnumThread = CreateThread(0,0,EnumFontsThread,0,0,&dwThId); // хэндл закроет сама нить

	{
		wchar_t temp[MAX_PATH];

		for (uint i=0; i < sizeofarray(Settings::FSizes); i++)
		{
			wsprintf(temp, _T("%i"), Settings::FSizes[i]);
			if (i > 0)
				SendDlgItemMessage(hMain, tFontSizeY, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(hMain, tFontSizeX, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(hMain, tFontSizeX2, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(hMain, tFontSizeX3, CB_ADDSTRING, 0, (LPARAM) temp);
		}

		wsprintf(temp, _T("%i"), LogFont.lfHeight);
		upToFontHeight = LogFont.lfHeight;
		SelectStringExact(hMain, tFontSizeY, temp);

		wsprintf(temp, _T("%i"), FontSizeX);
		SelectStringExact(hMain, tFontSizeX, temp);

		wsprintf(temp, _T("%i"), FontSizeX2);
		SelectStringExact(hMain, tFontSizeX2, temp);

		wsprintf(temp, _T("%i"), FontSizeX3);
		SelectStringExact(hMain, tFontSizeX3, temp);
	}

	{
		const char *ChSets[] = {"ANSI", "Arabic", "Baltic", "Chinese Big 5", "Default", "East Europe",
			"GB 2312", "Greek", "Hebrew", "Hangul", "Johab", "Mac", "OEM", "Russian", "Shiftjis",
			"Symbol", "Thai", "Turkish", "Vietnamese"};

		u8 num = 4;
		for (uint i=0; i < 19; i++)
		{
			SendDlgItemMessageA(hMain, tFontCharset, CB_ADDSTRING, 0, (LPARAM) ChSets[i]);
			if (chSetsNums[i] == LogFont.lfCharSet) num = i;
		}
		SendDlgItemMessage(hMain, tFontCharset, CB_SETCURSEL, num, 0);
	}

	MCHKHEAP

	SetDlgItemText(hMain, tCmdLine, psCmd ? psCmd : _T(""));

	SetDlgItemText(hMain, tBgImage, sBgImage);
	CheckDlgButton(hMain, rBgSimple, BST_CHECKED);

	TCHAR tmp[10];
	wsprintf(tmp, _T("%i"), bgImageDarker);
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
		
	if (isUpdConHandle)
		CheckDlgButton(ghOpWnd, cbAutoConHandle, BST_CHECKED);
		
	if (LogFont.lfWeight == FW_BOLD) CheckDlgButton(hMain, cbBold, BST_CHECKED);
	if (LogFont.lfItalic)            CheckDlgButton(hMain, cbItalic, BST_CHECKED);

	if (isFullScreen)
		CheckRadioButton(hMain, rNormal, rFullScreen, rFullScreen);
	else if (IsZoomed(ghWnd))
		CheckRadioButton(hMain, rNormal, rFullScreen, rMaximized);
	else
		CheckRadioButton(hMain, rNormal, rFullScreen, rNormal);

	//wsprintf(temp, _T("%i"), wndWidth);   SetDlgItemText(hMain, tWndWidth, temp);
	SendDlgItemMessage(hMain, tWndWidth, EM_SETLIMITTEXT, 3, 0);
	//wsprintf(temp, _T("%i"), wndHeight);  SetDlgItemText(hMain, tWndHeight, temp);
	SendDlgItemMessage(hMain, tWndHeight, EM_SETLIMITTEXT, 3, 0);
	UpdateSize(wndWidth, wndHeight);

	EnableWindow(GetDlgItem(hMain, cbApplyPos), FALSE);

	SendDlgItemMessage(hMain, tWndX, EM_SETLIMITTEXT, 6, 0);
	SendDlgItemMessage(hMain, tWndY, EM_SETLIMITTEXT, 6, 0);

	MCHKHEAP

	if (!isFullScreen && !IsZoomed(ghWnd))
	{
		EnableWindow(GetDlgItem(hMain, tWndWidth), true);
		EnableWindow(GetDlgItem(hMain, tWndHeight), true);
		EnableWindow(GetDlgItem(hMain, tWndX), true);
		EnableWindow(GetDlgItem(hMain, tWndY), true);
		EnableWindow(GetDlgItem(hMain, rFixed), true);
		EnableWindow(GetDlgItem(hMain, rCascade), true);
		if (!IsIconic(ghWnd)) {
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

	if (isDragEnabled) {
		//CheckDlgButton(hExt, cbDragEnabled, BST_CHECKED);
		if (isDragEnabled & DRAG_L_ALLOWED) CheckDlgButton(hExt, cbDragL, BST_CHECKED);
		if (isDragEnabled & DRAG_R_ALLOWED) CheckDlgButton(hExt, cbDragR, BST_CHECKED);
	}
	if (isDropEnabled) CheckDlgButton(hExt, cbDropEnabled, (isDropEnabled==1) ? BST_CHECKED : BST_INDETERMINATE);
	if (isDefCopy) CheckDlgButton(hExt, cbDnDCopy, BST_CHECKED);
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


	if (isTabs) CheckDlgButton(hExt, cbTabs, (isTabs==1) ? BST_CHECKED : BST_INDETERMINATE);

	if (isMulti)
		CheckDlgButton(hExt, cbConMan, BST_CHECKED);

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
		wsprintf(temp, _T("%i %i %i"), getR(Colors[i]), getG(Colors[i]), getB(Colors[i]));
		SetDlgItemText(hColors, 1100 + i, temp);
	}

	for (uint i=0; i < 16; i++)
	{
		wsprintf(temp, (i<10) ? _T("# %i") : _T("#%i"), i);
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
        if (isTabs==0) gConEmu.ForceShowTabs(FALSE); // там еще есть '==2', но его здесь не обрабатываем
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
		    if (IsZoomed(ghWnd) || IsIconic(ghWnd) || isFullScreen)
			    gConEmu.SetWindowMode(rNormal);
			// Установить размер
	        gConEmu.SetConsoleWindowSize(MakeCoord(newX, newY), true);
	    } else if (IsChecked(hMain, rMaximized) == BST_CHECKED) {
		    SetFocus(GetDlgItem(hMain, rMaximized));
		    if (!IsZoomed(ghWnd))
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

    case cbBold:
    case cbItalic:
        {
			PostMessage(hMain, gSet.mn_MsgRecreateFont, wParam, 0);
        }
        break;

    case cbBgImage:
        isShowBgImage = IsChecked(hMain, cbBgImage);
        EnableWindow(GetDlgItem(hMain, tBgImage), isShowBgImage);
        EnableWindow(GetDlgItem(hMain, tDarker), isShowBgImage);
        EnableWindow(GetDlgItem(hMain, slDarker), isShowBgImage);
        EnableWindow(GetDlgItem(hMain, bBgImage), isShowBgImage);

		if (isShowBgImage && bgImageDarker == 0) {
			if (MessageBox(ghOpWnd, 
				    L"Background image will NOT be visible\n"
					L"while 'Darkening' is 0. Increase it?",
					L"ConEmu", MB_YESNO|MB_ICONEXCLAMATION)!=IDNO)
			{
				bgImageDarker = 0x46;
				SendDlgItemMessage(hMain, slDarker, TBM_SETPOS  , (WPARAM) true, (LPARAM) bgImageDarker);
				TCHAR tmp[10];
				wsprintf(tmp, _T("%i"), gSet.bgImageDarker);
				SetDlgItemText(hMain, tDarker, tmp);
				gSet.LoadImageFrom(gSet.sBgImage);
			}
		}

        gConEmu.Update(true);
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
    
    case cbGUIpb: // GUI Progress Bars
        isGUIpb = IsChecked(hMain, cbGUIpb);
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

    case cbNonProportional:
        isProportional = !isProportional;
        mb_IgnoreEditChanged = TRUE;
        gConEmu.Update(true);
        mb_IgnoreEditChanged = FALSE;
        break;

    case cbMonospace:
        isForceMonospace = !isForceMonospace;

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
            ofn.lpstrFilter = _T("Bitmap images (*.bmp)\0*.bmp\0\0");
            ofn.nFilterIndex = 1;
            ofn.lpstrFile = temp;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrTitle = _T("Choose background image");
            ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
                    | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;
            if (GetOpenFileName(&ofn))
                SetDlgItemText(hMain, tBgImage, temp);
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
                wsprintf(temp, _T("%i %i %i"), getR(color), getG(color), getB(color));
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
    }
    
    gConEmu.Update(true);

    return 0;
}

LRESULT CSettings::OnComboBox(WPARAM wParam, LPARAM lParam)
{
    WORD wId = LOWORD(wParam);
    if (wId == tFontCharset)
    {
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
            MBoxA(_T("Invalid 'inPath' in CSettings::LoadImageFrom"));
        return false;
    }

    TCHAR exPath[MAX_PATH + 2];
    if (!ExpandEnvironmentStrings(inPath, exPath, MAX_PATH)) {
        if (abShowErrors) {
            TCHAR szError[MAX_PATH*2];
            DWORD dwErr = GetLastError();
            swprintf(szError, _T("Can't expand environment strings:\r\n%s\r\nError code=0x%08X\r\nImage loading failed"),
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
        if (*(u16*)pBuf == 'MB' && *(u32*)(pBuf + 0x0A) >= 0x36 && *(u32*)(pBuf + 0x0A) <= 0x436 && *(u32*)(pBuf + 0x0E) == 0x28 && !pBuf[0x1D] && !*(u32*)(pBuf + 0x1E))
            //if (*(u16*)pBuf == 'MB' && *(u32*)(pBuf + 0x0A) >= 0x36)
        {
            const HDC hScreenDC = GetDC(0);
            HDC hNewBgDc = CreateCompatibleDC(hScreenDC);
            HBITMAP hNewBgBitmap;
            if (hNewBgDc)
            {
                if(hNewBgBitmap = (HBITMAP)LoadImage(NULL, exPath, IMAGE_BITMAP,0,0,LR_LOADFROMFILE))
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
                            //MBoxA(_T("!")); //Maximus5 - Да, это очень информативно
                            MBoxA(_T("!GetDIBits"));


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
                            MBoxA(_T("!SetDIBits"));

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
                MBoxA(_T("Only BMP files supported as background!"));
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

BOOL CALLBACK CSettings::wndOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
    switch (messg)
    {
    case WM_INITDIALOG:
        ghOpWnd = hWnd2;
        #ifdef _DEBUG
        //if (IsDebuggerPresent())
        SetWindowPos(ghOpWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
        #endif
        SetClassLong(hWnd2, GCL_HICON, (LONG)hClassIcon);
        gSet.OnInitDialog();
        break;

    case WM_GETICON:
        if (wParam==ICON_BIG) {
            /*SetWindowLong(hWnd2, DWL_MSGRESULT, (LRESULT)hClassIcon);
            return 1;*/
        } else {
            SetWindowLong(hWnd2, DWL_MSGRESULT, (LRESULT)hClassIconSm);
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
                wsprintf(tmp, _T("%i"), gSet.bgImageDarker);
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

BOOL CALLBACK CSettings::mainOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
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
                wsprintf(tmp, _T("%i"), gSet.bgImageDarker);
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

BOOL CALLBACK CSettings::extOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
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

BOOL CALLBACK CSettings::colorOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
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

BOOL CALLBACK CSettings::infoOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
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
				    for (int i = 0; i < countof(gSet.mn_FPS); i++) {
					    if (gSet.mn_FPS[i] < tmin) tmin = gSet.mn_FPS[i];
					    if (gSet.mn_FPS[i] > tmax) tmax = gSet.mn_FPS[i];
				    }
				    if (tmax > tmin) {
					    v = ((double)20) / (tmax - tmin) * gSet.mn_Freq;
				    }
				}
				swprintf(sTemp, _T("%.1f"), v);
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

        wsprintf(temp, _T("%i"), gSet.wndX);
        SetDlgItemText(hMain, tWndX, temp);

        wsprintf(temp, _T("%i"), gSet.wndY);
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

        wsprintf(temp, _T("%i"), gSet.wndWidth);
        SetDlgItemText(hMain, tWndWidth, temp);

        wsprintf(temp, _T("%i"), gSet.wndHeight);
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

void CSettings::Performance(UINT nID, BOOL bEnd)
{
    if (nID == gbPerformance) //groupbox ctrl id
    {
		if (!gConEmu.isMainThread())
			return;

        if (ghOpWnd) {
            // Performance
            wchar_t sTemp[128];
            swprintf(sTemp, _T("Performance counters (%I64i)"), ((i64)(mn_Freq/1000)));
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
	    mn_FPS_CUR_FRAME++; if (mn_FPS_CUR_FRAME > countof(mn_FPS)) mn_FPS_CUR_FRAME = 0;
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

	LF.lfCharSet = LogFont.lfCharSet;
	int newCharSet = SendDlgItemMessage(hMain, tFontCharset, CB_GETCURSEL, 0, 0);
	if (newCharSet != CB_ERR && newCharSet >= 0 && newCharSet < 19)
		LogFont.lfCharSet = chSetsNums[newCharSet];

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
		if (!isFullScreen && !IsZoomed(ghWnd))
			gConEmu.SyncWindowToConsole();
		else
			gConEmu.SyncConsoleToWindow();
		gConEmu.ReSize();
	}

	if (wFromID == tFontFace) {
		wchar_t szSize[10];
		wsprintf(szSize, _T("%i"), LF.lfHeight);
		SetDlgItemText(hMain, tFontSizeY, szSize);
	}

	mb_IgnoreTtfChange = TRUE;
}

HFONT CSettings::CreateFontIndirectMy(LOGFONT *inFont)
{
    memset(CharWidth, 0, sizeof(*CharWidth)*0x10000);

    HFONT hFont = CreateFontIndirect(inFont);

    if (hFont) {
        HDC hScreenDC = GetDC(0);
        HDC hDC = CreateCompatibleDC(hScreenDC);
        MBoxAssert(hDC);
        if (hDC)
        {
            HFONT hOldF = (HFONT)SelectObject(hDC, hFont);
            TODO("Для пропорциональных шрифтов наверное имеет смысл сохранять в реестре оптимальный lfWidth")
            TEXTMETRIC tm;
            GetTextMetrics(hDC, &tm);
            if (isForceMonospace)
                //Maximus - у Arial'а например MaxWidth слишком большой
                inFont->lfWidth = FontSizeX3 ? FontSizeX3 : tm.tmMaxCharWidth;
            else
                inFont->lfWidth = tm.tmAveCharWidth;
            inFont->lfHeight = tm.tmHeight; TODO("Здесь нужно обновить реальный размер шрифта в диалоге настройки!");
			
            if (ghOpWnd) // устанавливать только при листании шрифта в настройке
                UpdateTTF ( (tm.tmMaxCharWidth - tm.tmAveCharWidth)>2 );

            SelectObject(hDC, hOldF);
            DeleteDC(hDC);
        }
        ReleaseDC(0, hScreenDC);

		if (mh_Font2) { DeleteObject(mh_Font2); mh_Font2 = NULL; }

		int width = FontSizeX2 ? FontSizeX2 : inFont->lfWidth;
		// Иначе рамки прерывистыми получаются... поставил NONANTIALIASED_QUALITY
		mh_Font2 = CreateFont(abs(inFont->lfHeight), abs(width), 0, 0, FW_NORMAL,
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
