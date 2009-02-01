
#include "Header.h"
#include "commctrl.h"

const u8 chSetsNums[19] = {0, 178, 186, 136, 1, 238, 134, 161, 177, 129, 130, 77, 255, 204, 128, 2, 222, 162, 163};
int upToFontHeight;
HWND hOpWnd;

extern void ForceShowTabs();

void SaveSettings()
{
    Registry reg;
    if (reg.OpenKey(_T("Console"), KEY_WRITE))
    {
        TCHAR ColorName[] = _T("ColorTable00");
        for (uint i = 0x10; i--;)
        {
            ColorName[10] = i/10 + '0';
            ColorName[11] = i%10 + '0';
            reg.Save(ColorName, (DWORD)pVCon->Colors[i]);
        }
        reg.CloseKey();

      if (reg.OpenKey(pVCon->Config, KEY_WRITE)) // NightRoman
        {
            GetDlgItemText(hOpWnd, tCmdLine, gSet.Cmd, MAX_PATH);
            RECT rect;
            GetWindowRect(hWnd, &rect);
            gSet.wndX = rect.left;
            gSet.wndY = rect.top;

            reg.Save(_T("CmdLine"), gSet.Cmd);
            reg.Save(_T("FontName"), pVCon->LogFont.lfFaceName);
            reg.Save(_T("BackGround Image"), gSet.pBgImage);
            reg.Save(_T("bgImageDarker"), gSet.bgImageDarker);
            reg.Save(_T("FontSize"), pVCon->LogFont.lfHeight);
            reg.Save(_T("FontSizeX"), gSet.FontSizeX);
            reg.Save(_T("FontCharSet"), pVCon->LogFont.lfCharSet);
            reg.Save(_T("Anti-aliasing"), pVCon->LogFont.lfQuality);
            reg.Save(_T("WindowMode"), gSet.isFullScreen ? rFullScreen : IsZoomed(hWnd) ? rMaximized : rNormal);
            reg.Save(_T("CursorType"), gSet.isCursorV);
            reg.Save(_T("CursorColor"), gSet.isCursorColor);
            reg.Save(_T("Experimental"), gSet.isFixFarBorders);
            reg.Save(_T("RightClick opens context menu"), gSet.isRClickSendKey);
            reg.Save(_T("AltEnter"), gSet.isSentAltEnter);
            reg.Save(_T("Dnd"), gSet.isDnD);
            reg.Save(_T("DefCopy"), gSet.isDefCopy);
            reg.Save(_T("GUIpb"), gSet.isGUIpb);
            reg.Save(_T("Tabs"), gSet.isTabs);
            reg.Save(_T("BackGround Image show"), gSet.isShowBgImage);
            reg.Save(_T("FontBold"), pVCon->LogFont.lfWeight == FW_BOLD);
            reg.Save(_T("FontItalic"), pVCon->LogFont.lfItalic);
            reg.Save(_T("ForceMonospace"), gSet.isForceMonospace);
            reg.Save(_T("Update Console handle"), gSet.isConMan);

            reg.Save(_T("ConWnd Width"), gSet.wndWidth);
            reg.Save(_T("ConWnd Height"), gSet.wndHeight);
            reg.Save(_T("ConWnd X"), gSet.wndX);
            reg.Save(_T("ConWnd Y"), gSet.wndY);

            reg.CloseKey();
            
            if (gSet.isTabs==1) ForceShowTabs();
            
            MessageBoxA(hOpWnd, "Saved.", "Information", MB_ICONINFORMATION);
            return;
        }
    }

    MessageBoxA(hOpWnd, "Failed", "Information", MB_ICONERROR);
}

void LoadSettings()
{
//------------------------------------------------------------------------
///| Default settings |///////////////////////////////////////////////////
//------------------------------------------------------------------------
    DWORD Quality = ANTIALIASED_QUALITY;
    WindowMode = rMaximized;
    DWORD inSize = 16;
    DWORD FontCharSet = DEFAULT_CHARSET;
    bool isBold = false, isItalic = false;
    TCHAR inFont[MAX_PATH], inFont2[MAX_PATH];
    _tcscpy(gSet.pBgImage, _T("c:\\back.bmp"));
    _tcscpy(inFont, _T("Lucida Console"));
    _tcscpy(inFont2, _T("Lucida Console"));
    gSet.bgImageDarker = 0;
    gSet.wndHeight = 25; // NightRoman
    gSet.wndWidth = 80;  // NightRoman
    gSet.isConVisible = false;
    gSet.isDefCopy = 2;
    gSet.nSlideShowElapse = 2500;
    gSet.nIconID = IDI_ICON1;
    gSet.isRClickSendKey = 2;

//------------------------------------------------------------------------
///| Loading from registry |//////////////////////////////////////////////
//------------------------------------------------------------------------
    Registry reg;
    if (reg.OpenKey(pVCon->Config, KEY_READ)) // NightRoman
    {
        reg.Load(_T("FontName"), inFont);
        reg.Load(_T("FontName2"), inFont2);
        reg.Load(_T("CmdLine"), gSet.Cmd);
        reg.Load(_T("BackGround Image"), gSet.pBgImage);
        reg.Load(_T("bgImageDarker"), &gSet.bgImageDarker);
        reg.Load(_T("FontSize"), &inSize);
        reg.Load(_T("FontSizeX"), &gSet.FontSizeX);
        reg.Load(_T("FontSizeX2"), &gSet.FontSizeX2);
        reg.Load(_T("FontCharSet"), &FontCharSet);
        reg.Load(_T("Anti-aliasing"), &Quality);
        reg.Load(_T("WindowMode"), &WindowMode);
        reg.Load(_T("ConWnd X"), &gSet.wndX);
        reg.Load(_T("ConWnd Y"), &gSet.wndY);
        reg.Load(_T("ConWnd Width"), &gSet.wndWidth);
        reg.Load(_T("ConWnd Height"), &gSet.wndHeight);
        reg.Load(_T("CursorType"), &gSet.isCursorV);
        reg.Load(_T("CursorColor"), &gSet.isCursorColor);
        reg.Load(_T("Experimental"), &gSet.isFixFarBorders);
        reg.Load(_T("RightClick opens context menu"), &gSet.isRClickSendKey);
        reg.Load(_T("AltEnter"), &gSet.isSentAltEnter);
        reg.Load(_T("BackGround Image show"), &gSet.isShowBgImage);
        reg.Load(_T("FontBold"), &isBold);
        reg.Load(_T("FontItalic"), &isItalic);
        reg.Load(_T("ForceMonospace"), &gSet.isForceMonospace);
        reg.Load(_T("Update Console handle"), &gSet.isConMan);
        reg.Load(_T("Dnd"), &gSet.isDnD);
        reg.Load(_T("DefCopy"), &gSet.isDefCopy);
        reg.Load(_T("GUIpb"), &gSet.isGUIpb);
        reg.Load(_T("Tabs"), &gSet.isTabs);
        reg.Load(_T("ConVisible"), &gSet.isConVisible);
        reg.Load(_T("SlideShowElapse"), &gSet.nSlideShowElapse);
        reg.Load(_T("IconID"), &gSet.nIconID);
        reg.CloseKey();
    }

    if (gSet.wndWidth)
        pVCon->TextWidth = gSet.wndWidth;
    if (gSet.wndHeight)
        pVCon->TextHeight = gSet.wndHeight;

    if (gSet.wndHeight && gSet.wndWidth)
    {
        COORD b = {gSet.wndWidth, gSet.wndHeight};
      SetConsoleWindowSize(b,false); // Maximus5 - по аналогии с NightRoman
      //MoveWindow(hConWnd, 0, 0, 1, 1, 0);
      //SetConsoleScreenBufferSize(pVCon->hConOut(), b);
      //MoveWindow(hConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
    }

    pVCon->LogFont.lfHeight = inSize;
    pVCon->LogFont.lfWidth = gSet.FontSizeX;
    _tcscpy(pVCon->LogFont.lfFaceName, inFont);
    _tcscpy(pVCon->LogFont2.lfFaceName, inFont2);
    pVCon->LogFont.lfQuality = Quality;
    if (isBold)
        pVCon->LogFont.lfWeight = FW_BOLD;
    pVCon->LogFont.lfCharSet = (BYTE) FontCharSet;
    if (isItalic)
        pVCon->LogFont.lfItalic = true;

    if (gSet.isShowBgImage)
        LoadImageFrom(gSet.pBgImage);
}

bool ShowColorDialog(HWND HWndOwner, COLORREF *inColor)
{
    CHOOSECOLOR cc;                 // common dialog box structure
    //static COLORREF acrCustClr[16]; // array of custom colors

    // Initialize CHOOSECOLOR
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = HWndOwner;
    cc.lpCustColors = (LPDWORD) pVCon->Colors;
    cc.rgbResult = *inColor;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&cc))
    {
        *inColor = cc.rgbResult;
        return true;
    }
    return false;
}

BOOL CALLBACK EnumFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount)
{
    int far * aiFontCount = (int far *) aFontCount;

    // Record the number of raster, TrueType, and vector
    // fonts in the font-count array.

    if (FontType & RASTER_FONTTYPE)
        aiFontCount[0]++;
    else if (FontType & TRUETYPE_FONTTYPE)
        aiFontCount[2]++;
    else
        aiFontCount[1]++;

    SendDlgItemMessage(hOpWnd, tFontFace, CB_ADDSTRING, 0, (LPARAM) lplf->lfFaceName);

    if (aiFontCount[0] || aiFontCount[1] || aiFontCount[2])
        return TRUE;
    else
        return FALSE;

    UNREFERENCED_PARAMETER( lplf );
    UNREFERENCED_PARAMETER( lpntm );
}

BOOL CALLBACK wndOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
    switch (messg)
    {
    case WM_INITDIALOG:
        hOpWnd = hWnd2;
        {
            HDC hdc = GetDC(hOpWnd);
            int aFontCount[] = { 0, 0, 0 };
            EnumFontFamilies(hdc, (LPCTSTR) NULL, (FONTENUMPROC) EnumFamCallBack, (LPARAM) aFontCount);
            DeleteDC(hdc);
        }

        SendDlgItemMessage(hWnd2, tFontFace, CB_SELECTSTRING, -1, (LPARAM)pVCon->LogFont.lfFaceName);

        {
            const BYTE FSizes[] = {0, 8, 10, 12, 14, 16, 18, 20, 24, 26, 28, 30, 32, 34, 36, 40, 46, 50, 52, 72};
            for (uint i=0; i < sizeofarray(FSizes); i++)
            {
                wsprintf(temp, _T("%i"), FSizes[i]);
                if (i > 0)
                    SendDlgItemMessage(hOpWnd, tFontSizeY, CB_ADDSTRING, 0, (LPARAM) temp);
                SendDlgItemMessage(hOpWnd, tFontSizeX, CB_ADDSTRING, 0, (LPARAM) temp);
            }

            wsprintf(temp, _T("%i"), pVCon->LogFont.lfHeight);
            upToFontHeight = pVCon->LogFont.lfHeight;
            if( SendDlgItemMessage(hWnd2, tFontSizeY, CB_SELECTSTRING, -1, (LPARAM)temp) == CB_ERR )
                SetDlgItemText(hWnd2, tFontSizeY, temp);

            wsprintf(temp, _T("%i"), gSet.FontSizeX);
            if( SendDlgItemMessage(hWnd2, tFontSizeX, CB_SELECTSTRING, -1, (LPARAM)temp) == CB_ERR )
                SetDlgItemText(hWnd2, tFontSizeX, temp);
        }

        {
            const char *ChSets[] = {"ANSI", "Arabic", "Baltic", "Chinese Big 5", "Default", "East Europe",
                "GB 2312", "Greek", "Hebrew", "Hangul", "Johab", "Mac", "OEM", "Russian", "Shiftjis",
                "Symbol", "Thai", "Turkish", "Vietnamese"};

            u8 num = 4;
            for (uint i=0; i < 19; i++)
            {
                SendDlgItemMessageA(hOpWnd, tFontCharset, CB_ADDSTRING, 0, (LPARAM) ChSets[i]);
                if (chSetsNums[i] == pVCon->LogFont.lfCharSet) num = i;
            }
            SendDlgItemMessage(hOpWnd, tFontCharset, CB_SETCURSEL, num, 0);
        }

        SetDlgItemText(hWnd2, tCmdLine, gSet.Cmd);
        SetDlgItemText(hWnd2, tBgImage, gSet.pBgImage);

        TCHAR tmp[10];
        wsprintf(tmp, _T("%i"), gSet.bgImageDarker);
        SendDlgItemMessage(hWnd2, tDarker, EM_SETLIMITTEXT, 3, 0);
        SetDlgItemText(hWnd2, tDarker, tmp);

        SendDlgItemMessage(hWnd2, slDarker, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(0, 255));
        SendDlgItemMessage(hWnd2, slDarker, TBM_SETPOS  , (WPARAM) true, (LPARAM) gSet.bgImageDarker);

        if (gSet.isShowBgImage)
            CheckDlgButton(hWnd2, cbBgImage, BST_CHECKED);
        else
        {
            EnableWindow(GetDlgItem(hWnd2, tBgImage), false);
            EnableWindow(GetDlgItem(hWnd2, tDarker), false);
            EnableWindow(GetDlgItem(hWnd2, slDarker), false);
        }

        switch(pVCon->LogFont.lfQuality)
        {
        case NONANTIALIASED_QUALITY:
            CheckDlgButton(hWnd2, rNoneAA, BST_CHECKED);
            break;
        case ANTIALIASED_QUALITY:
            CheckDlgButton(hWnd2, rStandardAA, BST_CHECKED);
            break;
        case CLEARTYPE_NATURAL_QUALITY:
            CheckDlgButton(hWnd2, rCTAA, BST_CHECKED);
            break;
        }
        if (gSet.isFixFarBorders)   CheckDlgButton(hWnd2, cbFixFarBorders, BST_CHECKED);
        if (gSet.isCursorColor) CheckDlgButton(hWnd2, cbCursorColor, BST_CHECKED);
        if (gSet.isRClickSendKey) CheckDlgButton(hWnd2, cbRClick, (gSet.isRClickSendKey==1) ? BST_CHECKED : BST_INDETERMINATE);
        if (gSet.isSentAltEnter) CheckDlgButton(hWnd2, cbSendAE, BST_CHECKED);
        
        if (gSet.isDnD)
        {
            CheckDlgButton(hWnd2, cbDnD, BST_CHECKED);
            EnableWindow(GetDlgItem(hOpWnd, cbDnDCopy), true);
        }
        else
            EnableWindow(GetDlgItem(hOpWnd, cbDnDCopy), false);
        if (gSet.isDefCopy) CheckDlgButton(hWnd2, cbDnDCopy, (gSet.isDefCopy==1) ? BST_CHECKED : BST_INDETERMINATE);

        if (gSet.isGUIpb) CheckDlgButton(hWnd2, cbGUIpb, BST_CHECKED);
        if (gSet.isTabs) CheckDlgButton(hWnd2, cbTabs, (gSet.isTabs==1) ? BST_CHECKED : BST_INDETERMINATE);
        if (gSet.isCursorV)
            CheckDlgButton(hWnd2, rCursorV, BST_CHECKED);
        else
            CheckDlgButton(hWnd2, rCursorH, BST_CHECKED);
        if (gSet.isForceMonospace)
            CheckDlgButton(hWnd2, cbMonospace, BST_CHECKED);
        if (gSet.isConMan)
            CheckDlgButton(hWnd2, cbIsConMan, BST_CHECKED);

        if (pVCon->LogFont.lfWeight == FW_BOLD) CheckDlgButton(hWnd2, cbBold, BST_CHECKED);
        if (pVCon->LogFont.lfItalic)            CheckDlgButton(hWnd2, cbItalic, BST_CHECKED);

        if (gSet.isFullScreen)
            CheckRadioButton(hOpWnd, rNormal, rFullScreen, rFullScreen);
        else if (IsZoomed(hWnd))
            CheckRadioButton(hOpWnd, rNormal, rFullScreen, rMaximized);
        else
            CheckRadioButton(hOpWnd, rNormal, rFullScreen, rNormal);

        wsprintf(temp, _T("%i"), gSet.wndWidth);
        SetDlgItemText(hWnd2, tWndWidth, temp);
        SendDlgItemMessage(hWnd2, tWndWidth, EM_SETLIMITTEXT, 3, 0);

        wsprintf(temp, _T("%i"), gSet.wndHeight);
        SetDlgItemText(hWnd2, tWndHeight, temp);
        SendDlgItemMessage(hWnd2, tWndHeight, EM_SETLIMITTEXT, 3, 0);

        if (!gSet.isFullScreen && !IsZoomed(hWnd))
        {
            EnableWindow(GetDlgItem(hOpWnd, tWndWidth), true);
            EnableWindow(GetDlgItem(hOpWnd, tWndHeight), true);

        }
        else
        {
            EnableWindow(GetDlgItem(hOpWnd, tWndWidth), false);
            EnableWindow(GetDlgItem(hOpWnd, tWndHeight), false);

        }

#define getR(inColorref) (byte)inColorref
#define getG(inColorref) (byte)(inColorref >> 8)
#define getB(inColorref) (byte)(inColorref >> 16)

        for (uint i = 0; i < 16; i++)
        {
            SendDlgItemMessage(hWnd2, 1100 + i, EM_SETLIMITTEXT, 11, 0);
            wsprintf(temp, _T("%i %i %i"), getR(pVCon->Colors[i]), getG(pVCon->Colors[i]), getB(pVCon->Colors[i]));
            SetDlgItemText(hWnd2, 1100 + i, temp);
        }

        {
            RECT rect;
            GetWindowRect(hWnd2, &rect);
            MoveWindow(hWnd2, GetSystemMetrics(SM_CXSCREEN)/2 - (rect.right - rect.left)/2, GetSystemMetrics(SM_CYSCREEN)/2 - (rect.bottom - rect.top)/2, rect.right - rect.left, rect.bottom - rect.top, false);
        }
        break;

        case WM_CTLCOLORSTATIC:
            for (uint i = 1000; i < 1016; i++)
                if (GetDlgItem(hWnd2, i) == (HWND)lParam)
                {
                    static HBRUSH KillBrush;
                    DeleteObject(KillBrush);
                    KillBrush = CreateSolidBrush(pVCon->Colors[i-1000]);
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
                    LoadImageFrom(gSet.pBgImage);
                    pVCon->Update(true);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                WORD CB = LOWORD(wParam);
                switch(wParam)
                {
                case IDOK:
					if (gSet.isTabs==1) ForceShowTabs();
                case IDCANCEL:
                case IDCLOSE:
                    SendMessage(hWnd2, WM_CLOSE, 0, 0);
                    break;

                case rNoneAA:
                case rStandardAA:
                case rCTAA:
                    pVCon->LogFont.lfQuality = wParam == rNoneAA ? NONANTIALIASED_QUALITY : wParam == rStandardAA ? ANTIALIASED_QUALITY : CLEARTYPE_NATURAL_QUALITY;
                    DeleteObject(pVCon->hFont);
                    pVCon->hFont = 0;
                    pVCon->LogFont.lfWidth = gSet.FontSizeX;
                    pVCon->Update(true);
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;

                case bSaveSettings:
                    SaveSettings();
                    break;

                case rNormal:
                case rFullScreen:
                case rMaximized:
                    SetWindowMode(wParam);
                    break;

                case cbFixFarBorders:
                    gSet.isFixFarBorders = !gSet.isFixFarBorders;

                    pVCon->Update(true);
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;

                case cbCursorColor:
                    gSet.isCursorColor = !gSet.isCursorColor;

                    pVCon->Update(true);
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;

                case cbBold:
                case cbItalic:
                    {
                        if (wParam == cbBold)
                            pVCon->LogFont.lfWeight = SendDlgItemMessage(hWnd2, cbBold, BM_GETCHECK, BST_CHECKED, 0) == BST_CHECKED ? FW_BOLD : FW_NORMAL;
                        else if (wParam == cbItalic)
                            pVCon->LogFont.lfItalic = SendDlgItemMessage(hWnd2, cbItalic, BM_GETCHECK, BST_CHECKED, 0) == BST_CHECKED ? true : false;

                        pVCon->LogFont.lfWidth = gSet.FontSizeX;
                        HFONT hFont = CreateFontIndirectMy(&pVCon->LogFont);
                        if (hFont)
                        {
                            DeleteObject(pVCon->hFont);
                            pVCon->hFont = hFont;

                            pVCon->Update(true);
                            if (!gSet.isFullScreen && !IsZoomed(hWnd))
                                SyncWindowToConsole();
                            else
                                SyncConsoleToWindow();
                            InvalidateRect(hWnd, 0, 0);
                        }
                    }
                    break;

                case cbBgImage:
                    gSet.isShowBgImage = SendDlgItemMessage(hWnd2, cbBgImage, BM_GETCHECK, BST_CHECKED, 0) == BST_CHECKED ? true : false;
                    EnableWindow(GetDlgItem(hOpWnd, tBgImage), gSet.isShowBgImage);
                    EnableWindow(GetDlgItem(hWnd2, tDarker), gSet.isShowBgImage);
                    EnableWindow(GetDlgItem(hWnd2, slDarker), gSet.isShowBgImage);

                    if (gSet.isShowBgImage && gSet.isBackgroundImageValid)
                        SetBkMode(pVCon->hDC, TRANSPARENT);
                    else
                        SetBkMode(pVCon->hDC, OPAQUE);

                    pVCon->Update(true);
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;

                case cbRClick:
                    //gSet.isRClickSendKey = !gSet.isRClickSendKey;
                    switch(IsDlgButtonChecked(hWnd2, cbRClick)) {
                        case BST_UNCHECKED:
                            gSet.isRClickSendKey=0; break;
                        case BST_CHECKED:
                            gSet.isRClickSendKey=1; break;
                        case BST_INDETERMINATE:
                            gSet.isRClickSendKey=2; break;
                    }
                    break;

                case cbSendAE:
                    gSet.isSentAltEnter = !gSet.isSentAltEnter;
                    break;

                case cbDnD:
                    gSet.isDnD = !gSet.isDnD;
                    EnableWindow(GetDlgItem(hWnd2, cbDnDCopy), gSet.isDnD);
                    break;
                
                case cbDnDCopy:
                    switch(IsDlgButtonChecked(hWnd2, cbDnDCopy)) {
                        case BST_UNCHECKED:
                            gSet.isDefCopy=0; break;
                        case BST_CHECKED:
                            gSet.isDefCopy=1; break;
                        case BST_INDETERMINATE:
                            gSet.isDefCopy=2; break;
                    }
                    //gSet.isDefCopy = !gSet.isDefCopy;
                    break;
                
                case cbGUIpb:
                    gSet.isGUIpb = !gSet.isGUIpb;
                    break;
                
                case cbTabs:
                    switch(IsDlgButtonChecked(hWnd2, cbTabs)) {
                        case BST_UNCHECKED:
                            gSet.isTabs=0; break;
                        case BST_CHECKED:
                            gSet.isTabs=1; break;
                        case BST_INDETERMINATE:
                            gSet.isTabs=2; break;
                    }
                    //gSet.isTabs = !gSet.isTabs;
                    break;

                case cbMonospace:
                    gSet.isForceMonospace = !gSet.isForceMonospace;

                    pVCon->Update(true);
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;

                case cbIsConMan:
                    gSet.isConMan = !gSet.isConMan;

                    pVCon->Update(true);
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;

                case rCursorH:
                case rCursorV:
                    if (wParam == rCursorV)
                        gSet.isCursorV = true;
                    else
                        gSet.isCursorV = false;

                    pVCon->Update(true);
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;

                default:
                    if (CB >= 1000 && CB <= 1015)
                    {
                        COLORREF color = pVCon->Colors[CB - 1000];
                        if( ShowColorDialog(hWnd2, &color) )
                        {
                            pVCon->Colors[CB - 1000] = color;
                            wsprintf(temp, _T("%i %i %i"), getR(color), getG(color), getB(color));
                            SetDlgItemText(hWnd2, CB + 100, temp);
                            InvalidateRect(GetDlgItem(hWnd2, CB), 0, 1);

                            pVCon->Update(true);
                            InvalidateRect(hWnd, NULL, FALSE);
                        }
                    }
                }
            }
            else if (HIWORD(wParam) == EN_CHANGE)
            {
                WORD TB = LOWORD(wParam);
                if (TB >= 1100 && TB <= 1115)
                {
                    int r, g, b;
                    GetDlgItemText(hWnd2, TB, temp, MAX_PATH);
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
                            if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && pVCon->Colors[TB - 1100] != RGB(r, g, b))
                            {
                                pVCon->Colors[TB - 1100] = RGB(r, g, b);
                                pVCon->Update(true);
                                InvalidateRect(hWnd, 0, 1);
                                InvalidateRect(GetDlgItem(hWnd2, TB - 100), 0, 1);
                            }
                        }
                    }
                }
                else if (TB == tBgImage)
                {
                    GetDlgItemText(hOpWnd, tBgImage, temp, MAX_PATH);
                    if( LoadImageFrom(temp) )
                    {
                        if (gSet.isShowBgImage && gSet.isBackgroundImageValid)
                            SetBkMode(pVCon->hDC, TRANSPARENT);
                        else
                            SetBkMode(pVCon->hDC, OPAQUE);

                        pVCon->Update(true);
                        InvalidateRect(hWnd, NULL, FALSE);
                    }
                }
                else if ( (TB == tWndWidth || TB == tWndHeight) && IsDlgButtonChecked(hWnd2, rNormal) == BST_CHECKED )
                {
                    DWORD newX, newY;
                    GetDlgItemText(hWnd2, tWndWidth, temp, MAX_PATH);
                    newX = klatoi(temp);
                    GetDlgItemText(hWnd2, tWndHeight, temp, MAX_PATH);
                    newY = klatoi(temp);

                    if (newX > 24 && newY > 7)
                    {
                        gSet.wndWidth = newX;
                        gSet.wndHeight = newY;

                        COORD b = {gSet.wndWidth, gSet.wndHeight};
                  SetConsoleWindowSize(b, false);  // NightRoman
                  //MoveWindow(hConWnd, 0, 0, 1, 1, 0);
                  //SetConsoleScreenBufferSize(pVCon->hConOut(), b);
                  //MoveWindow(hConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
                    }

                }
                else if (TB == tDarker)
                {
                    DWORD newV;
                    TCHAR tmp[10];
                    GetDlgItemText(hWnd2, tDarker, tmp, 10);
                    newV = klatoi(tmp);
                    if (newV < 256 && newV != gSet.bgImageDarker)
                    {
                        gSet.bgImageDarker = newV;
                        SendDlgItemMessage(hWnd2, slDarker, TBM_SETPOS, (WPARAM) true, (LPARAM) gSet.bgImageDarker);
                        LoadImageFrom(gSet.pBgImage);
                        pVCon->Update(true);
                        InvalidateRect(hWnd, NULL, FALSE);
                    }
                }

            }
            else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
            {
                if (LOWORD(wParam) == tFontFace)
                {
                    _tcscpy(temp, pVCon->LogFont.lfFaceName);
                    if (HIWORD(wParam) == CBN_EDITCHANGE)
                        GetDlgItemText(hWnd2, tFontFace, pVCon->LogFont.lfFaceName, LF_FACESIZE);
                    else
                        SendDlgItemMessage(hWnd2, tFontFace, CB_GETLBTEXT, SendDlgItemMessage(hWnd2, tFontFace, CB_GETCURSEL, 0, 0), (LPARAM)pVCon->LogFont.lfFaceName);

                    if (HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        LRESULT a = SendDlgItemMessage(hWnd2, tFontFace, CB_FINDSTRINGEXACT, -1, (LPARAM)pVCon->LogFont.lfFaceName);
                        if(a == CB_ERR)
                        {
                            _tcscpy(pVCon->LogFont.lfFaceName, temp);
                            break;
                        }
                    }

                    BYTE qWas = pVCon->LogFont.lfQuality;
                    pVCon->LogFont.lfHeight = upToFontHeight;
                    pVCon->LogFont.lfWidth = gSet.FontSizeX;
                    HFONT hFont = CreateFontIndirectMy(&pVCon->LogFont);
                    if (hFont)
                    {
                        DeleteObject(pVCon->hFont);
                        pVCon->hFont = hFont;

                        pVCon->Update(true);
                        if (!gSet.isFullScreen && !IsZoomed(hWnd))
                            SyncWindowToConsole();
                        else
                            SyncConsoleToWindow();
                        InvalidateRect(hWnd, 0, 0);

                        wsprintf(temp, _T("%i"), pVCon->LogFont.lfHeight);
                        SetDlgItemText(hWnd2, tFontSizeY, temp);
                    }
                }
                else if (LOWORD(wParam) == tFontSizeY || LOWORD(wParam) == tFontSizeX || LOWORD(wParam) == tFontCharset)
                {
                    int newSize;
                    if (LOWORD(wParam) == tFontSizeY || LOWORD(wParam) == tFontSizeX)
                    {
                        if (HIWORD(wParam) == CBN_EDITCHANGE)
                            GetDlgItemText(hWnd2, LOWORD(wParam), temp, MAX_PATH);
                        else
                            SendDlgItemMessage(hWnd2, LOWORD(wParam), CB_GETLBTEXT, SendDlgItemMessage(hWnd2, LOWORD(wParam), CB_GETCURSEL, 0, 0), (LPARAM)temp);

                        newSize = klatoi(temp);
                    }

                    if (newSize > 4 && newSize < 200 || (newSize == 0 && *temp == '0') || LOWORD(wParam) == tFontCharset)
                    {
                        if (LOWORD(wParam) == tFontSizeY)
                            pVCon->LogFont.lfHeight = upToFontHeight = newSize;
                        else if (LOWORD(wParam) == tFontSizeX)
                            gSet.FontSizeX = newSize;
                        else if (LOWORD(wParam) == tFontCharset)
                        {
                            int newCharSet = SendDlgItemMessage(hWnd2, tFontCharset, CB_GETCURSEL, 0, 0);
                            if (newCharSet != CB_ERR && newCharSet >= 0 && newCharSet < 19)
                                pVCon->LogFont.lfCharSet = chSetsNums[newCharSet];
                            pVCon->LogFont.lfHeight = upToFontHeight;
                        }
                        pVCon->LogFont.lfWidth = gSet.FontSizeX;

                        HFONT hFont = CreateFontIndirectMy(&pVCon->LogFont);
                        if (hFont)
                        {
                            DeleteObject(pVCon->hFont);
                            pVCon->hFont = hFont;

                            pVCon->Update(true);
                            if (!gSet.isFullScreen && !IsZoomed(hWnd))
                                SyncWindowToConsole();
                            else
                                SyncConsoleToWindow();
                            InvalidateRect(hWnd, 0, 0);
                        }
                    }
                }
            }
            break;
        case WM_CLOSE:
        case WM_DESTROY:
            EndDialog(hWnd2, TRUE);
            hOpWnd = 0;
            break;
        default:
            return 0;
    }
    return 0;
}

bool LoadImageFrom(TCHAR *inPath)
{
    TCHAR exPath[MAX_PATH + 2];
    if (!ExpandEnvironmentStrings(inPath, exPath, MAX_PATH))
        return false;

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
                    DeleteObject(pVCon->hBgBitmap);
                    DeleteDC(pVCon->hBgDc);

                    pVCon->hBgDc = hNewBgDc;
                    pVCon->hBgBitmap = hNewBgBitmap;

                    if(SelectObject(pVCon->hBgDc, pVCon->hBgBitmap))
                    {
                        gSet.isBackgroundImageValid = true;
                        pVCon->bgBmp.cx = *(u32*)(pBuf + 0x12);
                        pVCon->bgBmp.cy = *(i32*)(pBuf + 0x16);
                        if (klstricmp(gSet.pBgImage, inPath))
                        {
                            lRes = true;
                            _tcscpy(gSet.pBgImage, inPath);
                        }

                        struct bColor
                        {
                            u8 b;
                            u8 g;
                            u8 r;
                        };

                        bColor *bArray = new bColor[pVCon->bgBmp.cx * pVCon->bgBmp.cy];

                        BITMAPINFO bInfo;
                        bInfo.bmiHeader.biSize = sizeof(BITMAPINFO);
                        bInfo.bmiHeader.biWidth = pVCon->bgBmp.cx;
                        bInfo.bmiHeader.biHeight = pVCon->bgBmp.cy;
                        bInfo.bmiHeader.biPlanes = 1;
                        bInfo.bmiHeader.biBitCount = 24;
                        bInfo.bmiHeader.biCompression = BI_RGB;

                        if (!GetDIBits(pVCon->hBgDc, pVCon->hBgBitmap, 0, pVCon->bgBmp.cy, bArray, &bInfo, DIB_RGB_COLORS))
                            //MBoxA(_T("!")); //Maximus5 - Да, это очень информативно
                            MBoxA(_T("!GetDIBits"));


                        for (int i = 0; i < pVCon->bgBmp.cx * pVCon->bgBmp.cy; i++)
                        {
                            bArray[i].r = klMulDivU32(bArray[i].r, gSet.bgImageDarker, 255);
                            bArray[i].g = klMulDivU32(bArray[i].g, gSet.bgImageDarker, 255);
                            bArray[i].b = klMulDivU32(bArray[i].b, gSet.bgImageDarker, 255);
                        }


                        if (!SetDIBits(pVCon->hBgDc, pVCon->hBgBitmap, 0, pVCon->bgBmp.cy, bArray, &bInfo, DIB_RGB_COLORS))
                            MBoxA(_T("!SetDIBits"));

                        delete[] bArray;
                    }
                }
                else
                    DeleteDC(hNewBgDc);
            }

            ReleaseDC(0, hScreenDC);
        }
        file.Close();
    }

    return lRes;
}
