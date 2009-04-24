
#include "Header.h"
#include <commctrl.h>
#include "../common/ConEmuCheck.h"

#define COUNTER_REFRESH 5000

const u8 chSetsNums[19] = {0, 178, 186, 136, 1, 238, 134, 161, 177, 129, 130, 77, 255, 204, 128, 2, 222, 162, 163};
int upToFontHeight=0;
HWND ghOpWnd=NULL;

namespace Settings {
    const WCHAR* szKeys[] = {L"<None>", L"Left Ctrl", L"Right Ctrl", L"Left Alt", L"Right Alt", L"Left Shift", L"Right Shift"};
    const DWORD  nKeys[] =  {0,         VK_LCONTROL,  VK_RCONTROL,   VK_LMENU,    VK_RMENU,     VK_LSHIFT,     VK_RSHIFT};
    const BYTE   FSizes[] = {0, 8, 10, 12, 14, 16, 18, 20, 24, 26, 28, 30, 32, 34, 36, 40, 46, 50, 52, 72};
};

CSettings::CSettings()
{
    InitSettings();
    mb_IgnoreEditChanged = FALSE;
    mb_IgnoreTtfChange = TRUE;
    hMain = NULL; hColors = NULL; hInfo = NULL; hwndTip = NULL;
    QueryPerformanceFrequency((LARGE_INTEGER *)&mn_Freq);
    memset(mn_Counter, 0, sizeof(*mn_Counter)*(tPerfInterval-gbPerformance));
    memset(mn_CounterMax, 0, sizeof(*mn_CounterMax)*(tPerfInterval-gbPerformance));
    memset(mn_CounterTick, 0, sizeof(*mn_CounterTick)*(tPerfInterval-gbPerformance));
    hBgBitmap = NULL; bgBmp = MakeCoord(0,0); hBgDc = NULL; mh_Font = NULL; mh_Font2 = NULL;
    memset(FontWidth, 0, sizeof(FontWidth));
    nAttachPID = 0; hAttachConWnd = NULL;
    memset(&ourSI, 0, sizeof(ourSI));
    ourSI.cb = sizeof(ourSI);
    try {
        GetStartupInfoW(&ourSI);
    } catch(...) {
        memset(&ourSI, 0, sizeof(ourSI));
    }

    SetWindowThemeF = NULL;
    mh_Uxtheme = LoadLibrary(_T("UxTheme.dll"));
    if (mh_Uxtheme) {
        SetWindowThemeF = (SetWindowThemeT)GetProcAddress(mh_Uxtheme, "SetWindowTheme");
        EnableThemeDialogTextureF = (EnableThemeDialogTextureT)GetProcAddress(mh_Uxtheme, "EnableThemeDialogTexture");
        //if (SetWindowThemeF) { SetWindowThemeF(Progressbar1, _T(" "), _T(" ")); }
    }

	mn_MsgUpdateCounter = RegisterWindowMessage(_T("ConEmuSettings::Counter"));
}

CSettings::~CSettings()
{
    if (mh_Font) { DeleteObject(mh_Font); mh_Font = NULL; }
    if (mh_Font2) { DeleteObject(mh_Font2); mh_Font2 = NULL; }
    if (psCmd) {free(psCmd); psCmd = NULL;}
    if (psCurCmd) {free(psCurCmd); psCurCmd = NULL;}
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
    
    psCmd = NULL; psCurCmd = NULL;
    isConMan = false;

    nMainTimerElapse = 10;
    nAffinity = 3;
    //isAdvLangChange = true;
    isSkipFocusEvents = false;
    isLangChangeWsPlugin = false;
    BufferHeight = 0;
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
    isForceMonospace = false; isTTF = false;
    isMinToTray = false;
    
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
    _tcscpy(sBgImage, _T("c:\\back.bmp"));
    isFixFarBorders = true;
    bgImageDarker = 0;
    wndHeight = ntvdmHeight = 25; // NightRoman
    wndWidth = 80;  // NightRoman
    wndX = 0; wndY = 0; wndCascade = true;
    isConVisible = false;
    nSlideShowElapse = 2500;
    nIconID = IDI_ICON1;
    isRClickSendKey = 2;
    _tcscpy(szTabPanels, _T("Panels"));
    pszTabConsole = _tcscpy(szTabPanels+_tcslen(szTabPanels)+1, _T("Console"));
    _tcscpy(szTabEditor, _T("[%s]"));
    _tcscpy(szTabEditorModified, _T("[%s] *"));
    /* */ _tcscpy(szTabViewer, _T("{%s}"));
    nTabLenMax = 20;
    
    isCursorV = true;
    
    isTabs = 1;
    
    isVisualizer = false;
    nVizNormal = 1; nVizFore = 15; nVizTab = 15; nVizEOL = 8; nVizEOF = 12;
    cVizTab = 0x2192; cVizEOL = 0x2193; cVizEOF = 0x2640;

    isAllowDetach = 0;
    isCreateAppWindow = false;
    isScrollTitle = true;
    ScrollTitleLen = 22;
    
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
    
        reg.Load(_T("FontName"), inFont);
        reg.Load(_T("FontName2"), inFont2);
        reg.Load(_T("CmdLine"), &psCmd);
        reg.Load(_T("ConMan"), isConMan);
        reg.Load(_T("BackGround Image"), sBgImage);
        reg.Load(_T("bgImageDarker"), bgImageDarker);
        reg.Load(_T("FontSize"), inSize);
        reg.Load(_T("FontSizeX"), FontSizeX);
        reg.Load(_T("FontSizeX3"), FontSizeX3);
        reg.Load(_T("FontSizeX2"), FontSizeX2);
        reg.Load(_T("FontCharSet"), FontCharSet);
        reg.Load(_T("Anti-aliasing"), Quality);
        reg.Load(_T("WindowMode"), gConEmu.WindowMode);
        reg.Load(_T("ConWnd X"), wndX); /*if (wndX<-10) wndX = 0;*/
        reg.Load(_T("ConWnd Y"), wndY); /*if (wndY<-10) wndY = 0;*/
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
        reg.Load(_T("16it Height"), ntvdmHeight); if (ntvdmHeight<20) ntvdmHeight = 20; else if (ntvdmHeight>100) ntvdmHeight = 100;
        reg.Load(_T("CursorType"), isCursorV);
        reg.Load(_T("CursorColor"), isCursorColor);
        reg.Load(_T("Experimental"), isFixFarBorders);
        reg.Load(_T("RightClick opens context menu"), isRClickSendKey);
        reg.Load(_T("AltEnter"), isSentAltEnter);
        reg.Load(_T("Min2Tray"), isMinToTray);
        reg.Load(_T("BackGround Image show"), isShowBgImage);
        reg.Load(_T("FontBold"), isBold);
        reg.Load(_T("FontItalic"), isItalic);
        reg.Load(_T("ForceMonospace"), isForceMonospace);
        reg.Load(_T("Proportional"), isTTF);
        reg.Load(_T("Update Console handle"), isUpdConHandle);
        reg.Load(_T("Dnd"), isDragEnabled); 
        isDropEnabled = (BYTE)(isDragEnabled ? 1 : 0); // ранее "DndDrop" не было, поэтому ставим default
        reg.Load(_T("DndLKey"), nLDragKey);
        reg.Load(_T("DndRKey"), nRDragKey);
        reg.Load(_T("DndDrop"), isDropEnabled);
        reg.Load(_T("DefCopy"), isDefCopy);
        reg.Load(_T("DndSteps"), isDnDsteps);
        reg.Load(_T("GUIpb"), isGUIpb);
        reg.Load(_T("Tabs"), isTabs);
        reg.Load(_T("TabFrame"), isTabFrame);
        reg.Load(_T("TabMargins"), rcTabMargins);
        reg.Load(_T("ConVisible"), isConVisible);
        reg.Load(_T("SlideShowElapse"), nSlideShowElapse);
        reg.Load(_T("IconID"), nIconID);
        reg.Load(_T("TabPanels"), szTabPanels);
            WCHAR* pszVert = wcschr(szTabPanels, L'|');
            if (!pszVert) {
                if (wcslen(szTabPanels)>54) szTabPanels[54] = 0;
                pszVert = szTabPanels + wcslen(szTabPanels);
                wcscpy(pszVert+1, L"Console");
            }
            *pszVert = 0; pszTabConsole = pszVert+1;
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
        reg.Load(_T("LangChangeWsPlugin"), isLangChangeWsPlugin);
        
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

    mh_Font = CreateFontIndirectMy(&LogFont);

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
            reg.Save(_T("ConMan"), isConMan);
            reg.Save(_T("FontName"), LogFont.lfFaceName);
            reg.Save(_T("FontName2"), LogFont2.lfFaceName);
            reg.Save(_T("BackGround Image"), sBgImage);
            reg.Save(_T("bgImageDarker"), bgImageDarker);
            reg.Save(_T("FontSize"), LogFont.lfHeight);
            reg.Save(_T("FontSizeX"), FontSizeX);
            reg.Save(_T("FontSizeX2"), FontSizeX2);
            reg.Save(_T("FontSizeX3"), FontSizeX3);
            reg.Save(_T("FontCharSet"), LogFont.lfCharSet);
            reg.Save(_T("Anti-aliasing"), LogFont.lfQuality);
            reg.Save(_T("WindowMode"), isFullScreen ? rFullScreen : IsZoomed(ghWnd) ? rMaximized : rNormal);
            reg.Save(_T("CursorType"), isCursorV);
            reg.Save(_T("CursorColor"), isCursorColor);
            reg.Save(_T("Experimental"), isFixFarBorders);
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
            reg.Save(_T("BackGround Image show"), isShowBgImage);
            reg.Save(_T("FontBold"), LogFont.lfWeight == FW_BOLD);
            reg.Save(_T("FontItalic"), LogFont.lfItalic);
            reg.Save(_T("ForceMonospace"), isForceMonospace);
            reg.Save(_T("Proportional"), isTTF);
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

    SendDlgItemMessage(gSet.hMain, tFontFace, CB_ADDSTRING, 0, (LPARAM) lplf->lfFaceName);
    SendDlgItemMessage(gSet.hMain, tFontFace2, CB_ADDSTRING, 0, (LPARAM) lplf->lfFaceName);

    MCHKHEAP
    if (aiFontCount[0] || aiFontCount[1] || aiFontCount[2])
        return TRUE;
    else
        return FALSE;

    UNREFERENCED_PARAMETER( lplf );
    UNREFERENCED_PARAMETER( lpntm );
}

LRESULT CSettings::OnInitDialog()
{
    MCHKHEAP
    {
        TCITEM tie;
        HWND _hwndTab = GetDlgItem(ghOpWnd, tabMain);
        tie.mask = TCIF_TEXT;
        tie.iImage = -1; 
        tie.pszText = _T("Main");
        TabCtrl_InsertItem(_hwndTab, 0, &tie);
        tie.pszText = _T("Colors");
        TabCtrl_InsertItem(_hwndTab, 1, &tie);
        tie.pszText = _T("Info");
        TabCtrl_InsertItem(_hwndTab, 2, &tie);
        
        HFONT hFont = CreateFont(TAB_FONT_HEIGTH, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, 
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TAB_FONT_FACE);
        SendMessage(_hwndTab, WM_SETFONT, WPARAM (hFont), TRUE);
        
        RECT rcClient; GetWindowRect(_hwndTab, &rcClient);
        MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);
        TabCtrl_AdjustRect(_hwndTab, FALSE, &rcClient);
        
        hMain = CreateDialog((HINSTANCE)GetModuleHandle(NULL), 
            MAKEINTRESOURCE(IDD_DIALOG1), ghOpWnd, mainOpProc);
        MoveWindow(hMain, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 0);
        hColors = CreateDialog((HINSTANCE)GetModuleHandle(NULL), 
            MAKEINTRESOURCE(IDD_DIALOG2), ghOpWnd, colorOpProc);
        MoveWindow(hColors, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 0);
        hInfo = CreateDialog((HINSTANCE)GetModuleHandle(NULL), 
            MAKEINTRESOURCE(IDD_DIALOG3), ghOpWnd, infoOpProc);
        MoveWindow(hInfo, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 0);

        ShowWindow(hMain, SW_SHOW);
    }
    
    MCHKHEAP

    {
        HDC hdc = GetDC(ghOpWnd);
        int aFontCount[] = { 0, 0, 0 };
        EnumFontFamilies(hdc, (LPCTSTR) NULL, (FONTENUMPROC) EnumFamCallBack, (LPARAM) aFontCount);
        DeleteDC(hdc);
        
        TCHAR szTitle[MAX_PATH]; szTitle[0]=0;
        int nConfLen = _tcslen(Config);
        int nStdLen = strlen("Software\\ConEmu");
        if (nConfLen>(nStdLen+1))
            wsprintf(szTitle, _T("Settings (%s)..."), (Config+nStdLen+1));
        else
            _tcscpy(szTitle, _T("Settings..."));
        SetWindowText ( ghOpWnd, szTitle );
    }

    SendDlgItemMessage(hMain, tFontFace, CB_SELECTSTRING, -1, (LPARAM)LogFont.lfFaceName);
    SendDlgItemMessage(hMain, tFontFace2, CB_SELECTSTRING, -1, (LPARAM)LogFont2.lfFaceName);

    {
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
        if( SendDlgItemMessage(hMain, tFontSizeY, CB_SELECTSTRING, -1, (LPARAM)temp) == CB_ERR )
            SetDlgItemText(hMain, tFontSizeY, temp);

        wsprintf(temp, _T("%i"), FontSizeX);
        if( SendDlgItemMessage(hMain, tFontSizeX, CB_SELECTSTRING, -1, (LPARAM)temp) == CB_ERR )
            SetDlgItemText(hMain, tFontSizeX, temp);

        wsprintf(temp, _T("%i"), FontSizeX2);
        if( SendDlgItemMessage(hMain, tFontSizeX2, CB_SELECTSTRING, -1, (LPARAM)temp) == CB_ERR )
            SetDlgItemText(hMain, tFontSizeX2, temp);

        wsprintf(temp, _T("%i"), FontSizeX3);
        if( SendDlgItemMessage(hMain, tFontSizeX3, CB_SELECTSTRING, -1, (LPARAM)temp) == CB_ERR )
            SetDlgItemText(hMain, tFontSizeX3, temp);
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
    SetDlgItemText(hInfo, tCurCmdLine, GetCommandLine());
    SetDlgItemText(hMain, tBgImage, sBgImage);
    CheckDlgButton(hMain, rBgSimple, BST_CHECKED);

    TCHAR tmp[10];
    wsprintf(tmp, _T("%i"), bgImageDarker);
    SendDlgItemMessage(hMain, tDarker, EM_SETLIMITTEXT, 3, 0);
    SetDlgItemText(hMain, tDarker, tmp);

    SendDlgItemMessage(hMain, slDarker, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(0, 255));
    SendDlgItemMessage(hMain, slDarker, TBM_SETPOS  , (WPARAM) true, (LPARAM) bgImageDarker);

    if (isShowBgImage)
        CheckDlgButton(hMain, cbBgImage, BST_CHECKED);
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
    if (isFixFarBorders)   CheckDlgButton(hMain, cbFixFarBorders, BST_CHECKED);
    if (isCursorColor) CheckDlgButton(hMain, cbCursorColor, BST_CHECKED);
    if (isRClickSendKey) CheckDlgButton(hMain, cbRClick, (isRClickSendKey==1) ? BST_CHECKED : BST_INDETERMINATE);
    if (isSentAltEnter) CheckDlgButton(hMain, cbSendAE, BST_CHECKED);
    if (isMinToTray) CheckDlgButton(hMain, cbMinToTray, BST_CHECKED);

    if (isDragEnabled) {
        //CheckDlgButton(hMain, cbDragEnabled, BST_CHECKED);
        if (isDragEnabled & DRAG_L_ALLOWED) CheckDlgButton(hMain, cbDragL, BST_CHECKED);
        if (isDragEnabled & DRAG_R_ALLOWED) CheckDlgButton(hMain, cbDragR, BST_CHECKED);
    }
    if (isDropEnabled) CheckDlgButton(hMain, cbDropEnabled, (isDropEnabled==1) ? BST_CHECKED : BST_INDETERMINATE);
    if (isDefCopy) CheckDlgButton(hMain, cbDnDCopy, BST_CHECKED);
    {
        uint nKeyCount = sizeofarray(Settings::szKeys);
        u8 numL = 0, numR = 0;
        for (uint i=0; i<nKeyCount; i++) {
            SendDlgItemMessage(hMain, lbLDragKey, CB_ADDSTRING, 0, (LPARAM) Settings::szKeys[i]);
            SendDlgItemMessage(hMain, lbRDragKey, CB_ADDSTRING, 0, (LPARAM) Settings::szKeys[i]);
            if (Settings::nKeys[i] == nLDragKey) numL = i;
            if (Settings::nKeys[i] == nRDragKey) numR = i;
        }
        if (!numL) nLDragKey = 0; // если код клавиши неизвестен?
        if (!numR) nRDragKey = 0; // если код клавиши неизвестен?
        SendDlgItemMessage(hMain, lbLDragKey, CB_SETCURSEL, numL, 0);
        SendDlgItemMessage(hMain, lbRDragKey, CB_SETCURSEL, numR, 0);
    }
    

    if (isGUIpb) CheckDlgButton(hMain, cbGUIpb, BST_CHECKED);
    if (isTabs) CheckDlgButton(hMain, cbTabs, (isTabs==1) ? BST_CHECKED : BST_INDETERMINATE);
    if (isCursorV)
        CheckDlgButton(hMain, rCursorV, BST_CHECKED);
    else
        CheckDlgButton(hMain, rCursorH, BST_CHECKED);
    if (isForceMonospace)
        CheckDlgButton(hMain, cbMonospace, BST_CHECKED);
    if (!isTTF)
        CheckDlgButton(hMain, cbNonProportional, BST_CHECKED);
    if (isUpdConHandle)
        CheckDlgButton(ghOpWnd, cbAutoConHandle, BST_CHECKED);
    if (isConMan)
        CheckDlgButton(hMain, cbConMan, BST_CHECKED);

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
    }
    UpdatePos(wndX, wndY);
    CheckDlgButton(hMain, wndCascade ? rCascade : rFixed, BST_CHECKED);

    #define getR(inColorref) (byte)inColorref
    #define getG(inColorref) (byte)(inColorref >> 8)
    #define getB(inColorref) (byte)(inColorref >> 16)

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
    
    // Performance
    Performance(gbPerformance, TRUE);
    

    gConEmu.UpdateProcessDisplay(TRUE);
	gConEmu.UpdateSizes();


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
    
    RegisterTipsFor(hMain);
    RegisterTipsFor(hColors);
    RegisterTipsFor(hInfo);

    MCHKHEAP

    return 0;
}

LRESULT CSettings::OnButtonClicked(WPARAM wParam, LPARAM lParam)
{
    WORD CB = LOWORD(wParam);
    switch(wParam)
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
        LogFont.lfQuality = wParam == rNoneAA ? NONANTIALIASED_QUALITY : wParam == rStandardAA ? ANTIALIASED_QUALITY : CLEARTYPE_NATURAL_QUALITY;
        DeleteObject(mh_Font);
        mh_Font = CreateFontIndirectMy(&LogFont);
        if (FontSizeX) LogFont.lfWidth = FontSizeX;
        gConEmu.Update(true);
        break;

    case bSaveSettings:
        if (SaveSettings())
            SendMessage(ghOpWnd,WM_COMMAND,IDOK,0);
        break;

    case rNormal:
    case rFullScreen:
    case rMaximized:
        //gConEmu.SetWindowMode(wParam);
        EnableWindow(GetDlgItem(hMain, cbApplyPos), TRUE);
        break;
        
    case cbApplyPos:
	    if (SendDlgItemMessage(hMain, rNormal, BM_GETCHECK, 0, 0) == BST_CHECKED) {
	        DWORD newX, newY;
	        GetDlgItemText(hMain, tWndWidth, temp, MAX_PATH);  newX = klatoi(temp);
	        GetDlgItemText(hMain, tWndHeight, temp, MAX_PATH); newY = klatoi(temp);
		    SetFocus(GetDlgItem(hMain, rNormal));
		    if (IsZoomed(ghWnd) || IsIconic(ghWnd) || isFullScreen)
			    gConEmu.SetWindowMode(rNormal);
			// Установить размер
	        gConEmu.SetConsoleWindowSize(MakeCoord(newX, newY), true);
	    } else if (SendDlgItemMessage(hMain, rMaximized, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		    SetFocus(GetDlgItem(hMain, rMaximized));
		    if (!IsZoomed(ghWnd))
			    gConEmu.SetWindowMode(rMaximized);
	    } else if (SendDlgItemMessage(hMain, rFullScreen, BM_GETCHECK, 0, 0) == BST_CHECKED) {
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
        isFixFarBorders = !isFixFarBorders;

        gConEmu.Update(true);
        break;

    case cbCursorColor:
        isCursorColor = !isCursorColor;

        gConEmu.Update(true);
        break;
        
    case cbConMan:
        isConMan = !isConMan;
        break;

    case cbBold:
    case cbItalic:
        {
            if (wParam == cbBold)
                LogFont.lfWeight = SendDlgItemMessage(hMain, cbBold, BM_GETCHECK, 0, 0) == BST_CHECKED ? FW_BOLD : FW_NORMAL;
            else if (wParam == cbItalic)
                LogFont.lfItalic = SendDlgItemMessage(hMain, cbItalic, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

            LogFont.lfWidth = FontSizeX;
            HFONT hFont = CreateFontIndirectMy(&LogFont);
            if (hFont)
            {
                DeleteObject(mh_Font);
                mh_Font = hFont;

                gConEmu.Update(true);
                if (!isFullScreen && !IsZoomed(ghWnd))
                    gConEmu.SyncWindowToConsole();
                else
                    gConEmu.SyncConsoleToWindow();
                gConEmu.ReSize();
                //InvalidateRect(ghWnd, 0, 0);
            }
        }
        break;

    case cbBgImage:
        isShowBgImage = SendDlgItemMessage(hMain, cbBgImage, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
        EnableWindow(GetDlgItem(hMain, tBgImage), isShowBgImage);
        EnableWindow(GetDlgItem(hMain, tDarker), isShowBgImage);
        EnableWindow(GetDlgItem(hMain, slDarker), isShowBgImage);
        EnableWindow(GetDlgItem(hMain, bBgImage), isShowBgImage);

        gConEmu.Update(true);
        break;

    case cbRClick:
        //isRClickSendKey = !isRClickSendKey;
        switch(IsDlgButtonChecked(hMain, cbRClick)) {
            case BST_UNCHECKED:
                isRClickSendKey=0; break;
            case BST_CHECKED:
                isRClickSendKey=1; break;
            case BST_INDETERMINATE:
                isRClickSendKey=2; break;
        }
        break;

    case cbSendAE:
        isSentAltEnter = !isSentAltEnter;
        break;

    case cbMinToTray:
        isMinToTray = !isMinToTray;
        break;

    /*case cbDragEnabled:
        if (IsDlgButtonChecked(hMain, cbDragEnabled) == BST_UNCHECKED) {
            if (IsDlgButtonChecked(hMain, cbDragL)==BST_CHECKED) 
                CheckDlgButton(hMain, cbDragL, BST_UNCHECKED);
            if (IsDlgButtonChecked(hMain, cbDragR)==BST_CHECKED) 
                CheckDlgButton(hMain, cbDragR, BST_UNCHECKED);
        } else {
            if (IsDlgButtonChecked(hMain, cbDragL)!=BST_CHECKED) 
                CheckDlgButton(hMain, cbDragL, BST_CHECKED);
        }*/
    case cbDragL:
    case cbDragR:
        //isDragEnabled = !isDragEnabled;
        isDragEnabled = 
            ((IsDlgButtonChecked(hMain, cbDragL)==BST_CHECKED) ? DRAG_L_ALLOWED : 0) |
            ((IsDlgButtonChecked(hMain, cbDragR)==BST_CHECKED) ? DRAG_R_ALLOWED : 0);
        //if ( (isDragEnabled == 0) != (IsDlgButtonChecked(hMain, cbDragEnabled) == BST_UNCHECKED) )
        //    CheckDlgButton(hMain, cbDragEnabled, (isDragEnabled == 0) ? BST_UNCHECKED : BST_CHECKED);
        break;
    case cbDropEnabled:
        //isDropEnabled = !isDropEnabled;
        switch(IsDlgButtonChecked(hMain, cbDropEnabled)) {
            case BST_UNCHECKED:
                isDropEnabled=0; break;
            case BST_CHECKED:
                isDropEnabled=1; break;
            case BST_INDETERMINATE:
                isDropEnabled=2; break;
        }
        break;
    case cbDnDCopy:
        //switch(IsDlgButtonChecked(hMain, cbDnDCopy)) {
        //    case BST_UNCHECKED:
        //        isDefCopy=0; break;
        //    case BST_CHECKED:
        //        isDefCopy=1; break;
        //    case BST_INDETERMINATE:
        //        isDefCopy=2; break;
        //}
        isDefCopy = !isDefCopy;
        break;
    
    case cbGUIpb:
        isGUIpb = !isGUIpb;
        break;
    
    case cbTabs:
        switch(IsDlgButtonChecked(hMain, cbTabs)) {
            case BST_UNCHECKED:
                isTabs=0; break;
            case BST_CHECKED:
                isTabs=1; break;
            case BST_INDETERMINATE:
                isTabs=2; break;
        }
        //isTabs = !isTabs;
        break;

    case cbNonProportional:
        isTTF = !isTTF;
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
        isExtendColors = SendDlgItemMessage(hColors, cbExtendColors, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
        for (int i=16; i<32; i++)
            EnableWindow(GetDlgItem(hColors, 1100+i), isExtendColors);
        EnableWindow(GetDlgItem(hColors, lbExtendIdx), isExtendColors);
        if (lParam) {
            gConEmu.Update(true);
        }
        break;
    case cbVisualizer:
        isVisualizer = SendDlgItemMessage(hColors, cbVisualizer, BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
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
        GetDlgItemText(hMain, tBgImage, temp, MAX_PATH);
        if( LoadImageFrom(temp, true) )
        {
            gConEmu.Update(true);
        }
    }
    else if ( (TB == tWndWidth || TB == tWndHeight) && IsDlgButtonChecked(hMain, rNormal) == BST_CHECKED )
    {
	    EnableWindow(GetDlgItem(hMain, cbApplyPos), TRUE);
        /*
        DWORD newX, newY;
        GetDlgItemText(hMain, tWndWidth, temp, MAX_PATH);
        newX = klatoi(temp);
        GetDlgItemText(hMain, tWndHeight, temp, MAX_PATH);
        newY = klatoi(temp);

        if (newX > 24 && newY > 7)
        {
            wndWidth = newX;
            wndHeight = newY;

            COORD b = {};
            gConEmu.SetConsoleWindowSize(MakeCoord(wndWidth, wndHeight), false);
        }
        */
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
    if (wId == tFontFace || wId == tFontFace2)
    {
        LOGFONT* pLogFont = (wId == tFontFace) ? &LogFont : &LogFont2;
        int nID = (wId == tFontFace) ? tFontFace : tFontFace2;
        _tcscpy(temp, pLogFont->lfFaceName);
        if (HIWORD(wParam) == CBN_EDITCHANGE)
            GetDlgItemText(hMain, nID, pLogFont->lfFaceName, LF_FACESIZE);
        else
            SendDlgItemMessage(hMain, nID, CB_GETLBTEXT, SendDlgItemMessage(hMain, nID, CB_GETCURSEL, 0, 0), (LPARAM)pLogFont->lfFaceName);

        if (HIWORD(wParam) == CBN_EDITCHANGE)
        {
            LRESULT a = SendDlgItemMessage(hMain, nID, CB_FINDSTRINGEXACT, -1, (LPARAM)pLogFont->lfFaceName);
            if(a == CB_ERR)
            {
                _tcscpy(pLogFont->lfFaceName, temp);
                return -1;
            }
        }

        if (wId == tFontFace)
            mb_IgnoreTtfChange = FALSE;

        BYTE qWas = pLogFont->lfQuality;
        pLogFont->lfHeight = upToFontHeight;
        pLogFont->lfWidth = FontSizeX;
        HFONT hFont = CreateFontIndirectMy(&LogFont);
        if (hFont)
        {
            if (wId == tFontFace) {
                DeleteObject(mh_Font);
                mh_Font = hFont;
            } else {
                DeleteObject(hFont); hFont = NULL;
            }
            // else { -- hFont2 удаляется и создается автоматически в функции CreateFontIndirectMy
            //    DeleteObject(hFont2);
            //    hFont2 = hFont;
            //}

            gConEmu.Update(true);
            if (!isFullScreen && !IsZoomed(ghWnd))
                gConEmu.SyncWindowToConsole();
            else
                gConEmu.SyncConsoleToWindow();
            gConEmu.ReSize();
            //InvalidateRect(ghWnd, 0, 0);

            if (wId == tFontFace) {
                wsprintf(temp, _T("%i"), pLogFont->lfHeight);
                SetDlgItemText(hMain, tFontSizeY, temp);
            }
        }
        mb_IgnoreTtfChange = TRUE;
    }
    else if (wId == tFontSizeY || wId == tFontSizeX || 
        wId == tFontSizeX2 || wId == tFontSizeX3 || wId == tFontCharset)
    {
        int newSize = 0;
        if (wId == tFontSizeY || wId == tFontSizeX || 
            wId == tFontSizeX2 || wId == tFontSizeX3)
        {
            if (HIWORD(wParam) == CBN_EDITCHANGE)
                GetDlgItemText(hMain, wId, temp, MAX_PATH);
            else
                SendDlgItemMessage(hMain, wId, CB_GETLBTEXT, SendDlgItemMessage(hMain, wId, CB_GETCURSEL, 0, 0), (LPARAM)temp);

            newSize = klatoi(temp);
        }

        if (newSize > 4 && newSize < 200 || (newSize == 0 && *temp == '0') || wId == tFontCharset)
        {
            if (wId == tFontSizeY)
                LogFont.lfHeight = upToFontHeight = newSize;
            else if (wId == tFontSizeX)
                FontSizeX = newSize;
            else if (wId == tFontSizeX2)
                FontSizeX2 = newSize;
            else if (wId == tFontSizeX3)
                FontSizeX3 = newSize;
            else if (wId == tFontCharset)
            {
                int newCharSet = SendDlgItemMessage(hMain, tFontCharset, CB_GETCURSEL, 0, 0);
                if (newCharSet != CB_ERR && newCharSet >= 0 && newCharSet < 19)
                    LogFont.lfCharSet = chSetsNums[newCharSet];
                LogFont.lfHeight = upToFontHeight;
            }
            LogFont.lfWidth = FontSizeX;

            HFONT hFont = CreateFontIndirectMy(&LogFont);
            if (hFont)
            {
                DeleteObject(mh_Font);
                mh_Font = hFont;

                gConEmu.Update(true);
                if (!isFullScreen && !IsZoomed(ghWnd))
                    gConEmu.SyncWindowToConsole();
                else
                    gConEmu.SyncConsoleToWindow();
                gConEmu.ReSize();
                //InvalidateRect(ghWnd, 0, 0);
            }
        }
    } else 
    if (wId == lbLDragKey || wId == lbRDragKey) {
        int num = SendDlgItemMessage(hMain, wId, CB_GETCURSEL, 0, 0);
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
                SendDlgItemMessage(hMain, wId, CB_SETCURSEL, num=0, 0);
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

                if (nSel==0) {
                    ShowWindow(hMain, SW_SHOW);
                    ShowWindow(hColors, SW_HIDE);
                    ShowWindow(hInfo, SW_HIDE);
                    SetFocus(hMain);
                } else if (nSel==1) {
                    ShowWindow(hColors, SW_SHOW);
                    ShowWindow(hMain, SW_HIDE);
                    ShowWindow(hInfo, SW_HIDE);
                    SetFocus(hColors);
                } else {
                    ShowWindow(hInfo, SW_SHOW);
                    ShowWindow(hMain, SW_HIDE);
                    ShowWindow(hColors, SW_HIDE);
                    SetFocus(hInfo);
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
    DialogBox((HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOGM), 0, wndOpProc);
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
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            SendMessage(hWnd2, WM_CLOSE, 0, 0);
        break;

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
    case WM_DESTROY:
        if (gSet.hwndTip) {DestroyWindow(gSet.hwndTip); gSet.hwndTip = NULL;}
        EndDialog(hWnd2, TRUE);
        ghOpWnd = NULL;
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
        if (gSet.EnableThemeDialogTextureF)
            gSet.EnableThemeDialogTextureF(hWnd2, 6/*ETDT_ENABLETAB*/);
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
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            SendMessage(hWnd2, WM_CLOSE, 0, 0);
        break;

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
        if (gSet.EnableThemeDialogTextureF)
            gSet.EnableThemeDialogTextureF(hWnd2, 6/*ETDT_ENABLETAB*/);
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
        if (gSet.EnableThemeDialogTextureF)
            gSet.EnableThemeDialogTextureF(hWnd2, 6/*ETDT_ENABLETAB*/);
        break;

    default:
		if (messg == gSet.mn_MsgUpdateCounter) {
            wchar_t sTemp[64];
			if (gSet.mn_Freq!=0) {
				double v = (gSet.mn_CounterMax[wParam]/(double)gSet.mn_Freq)*1000;
				swprintf(sTemp, _T("%.1f"), v);
				SetDlgItemText(gSet.hInfo, wParam+tPerfRead, sTemp);
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

    isTTF = bNewTTF;
    /* */ CheckDlgButton(hMain, cbNonProportional, 
        gSet.isTTF ? BST_UNCHECKED : BST_CHECKED);
}

void CSettings::Performance(UINT nID, BOOL bEnd)
{
    if (nID == gbPerformance)
    {
		if (!gConEmu.isMainThread())
			return;

        if (ghOpWnd) {
            // Performance
            wchar_t sTemp[128];
            swprintf(sTemp, _T("Performance counters (%I64i)"), ((i64)(mn_Freq/1000)));
            SetDlgItemText(hInfo, nID, sTemp);
            
            for (nID=tPerfRead; mn_Freq && nID<=tPerfInterval; nID++) {
                //swprintf(sTemp, _T("%I64i"), mn_CounterMax[nID-tPerfRead]);
				SendMessage(gSet.hInfo, mn_MsgUpdateCounter, nID-tPerfRead, 0);
            }
        }
        return;
    }
    if (nID<tPerfRead || nID>tPerfInterval)
        return;

    if (!bEnd) {
        QueryPerformanceCounter((LARGE_INTEGER *)&(mn_Counter[nID-tPerfRead]));
        return;
    } else if (!mn_Counter[nID-tPerfRead] || !mn_Freq) {
        return;
    }
    
    i64 tick2;
    QueryPerformanceCounter((LARGE_INTEGER *)&tick2);
    i64 t = (tick2-mn_Counter[nID-tPerfRead]);
    
    if (mn_CounterMax[nID-tPerfRead]<t || 
        (GetTickCount()-mn_CounterTick[nID-tPerfRead])>COUNTER_REFRESH)
    {
        mn_CounterMax[nID-tPerfRead] = t;
        mn_CounterTick[nID-tPerfRead] = GetTickCount();
        
        if (ghOpWnd) {
			PostMessage(gSet.hInfo, mn_MsgUpdateCounter, nID-tPerfRead, 0);
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

    HWND hChild = NULL;
    
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
        }
    }

    SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)300);
}

HFONT CSettings::CreateFontIndirectMy(LOGFONT *inFont)
{
    memset(FontWidth, 0, sizeof(*FontWidth)*0x10000);

    DeleteObject(mh_Font2); mh_Font2 = NULL;

    int width = FontSizeX2 ? FontSizeX2 : inFont->lfWidth;
    mh_Font2 = CreateFont(abs(inFont->lfHeight), abs(width), 0, 0, FW_NORMAL,
        0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 0, LogFont2.lfFaceName);

    HFONT hFont = CreateFontIndirect(inFont);

    if (hFont) {
        HDC hScreenDC = GetDC(0);
        HDC hDC = CreateCompatibleDC(hScreenDC);
        MBoxAssert(hDC);
        if (hDC)
        {
            HFONT hOldF = (HFONT)SelectObject(hDC, hFont);
            #pragma message("TODO: Для пропорциональных шрифтов наверное имеет смысл сохранять в реестре оптимальный lfWidth")
            TEXTMETRIC tm;
            GetTextMetrics(hDC, &tm);
            if (isForceMonospace)
                //Maximus - у Arial'а например MaxWidth слишком большой
                LogFont.lfWidth = FontSizeX3 ? FontSizeX3 : tm.tmMaxCharWidth;
            else
                LogFont.lfWidth = tm.tmAveCharWidth;
            LogFont.lfHeight = tm.tmHeight;

            if (ghOpWnd) // устанавливать только при листании шрифта в настройке
                UpdateTTF ( (tm.tmMaxCharWidth - tm.tmAveCharWidth)>2 );

            SelectObject(hDC, hOldF);
            DeleteDC(hDC);
        }
        ReleaseDC(0, hScreenDC);
    }

    return hFont;
}

LPCTSTR CSettings::GetCmd()
{
    if (psCurCmd && *psCurCmd)
        return psCurCmd;
    if (psCmd && *psCmd)
        return psCmd;
    if (psCurCmd) free(psCurCmd);
    psCurCmd = _tcsdup(BufferHeight == 0 ? _T("far") : _T("cmd"));
    return psCurCmd;
}
