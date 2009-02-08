
#include "Header.h"
#include <commctrl.h>
#include "ConEmu.h"

const u8 chSetsNums[19] = {0, 178, 186, 136, 1, 238, 134, 161, 177, 129, 130, 77, 255, 204, 128, 2, 222, 162, 163};
int upToFontHeight=0;
HWND ghOpWnd=NULL;

CSettings::CSettings()
{
	InitSettings();
}

CSettings::~CSettings()
{
}

void CSettings::InitSettings()
{
    memset(&gSet, 0, sizeof(gSet)); // дабы мусора в дебаге не оставалось

//------------------------------------------------------------------------
///| Moved from VirtualConsole |//////////////////////////////////////////
//------------------------------------------------------------------------
	_tcscpy(gSet.Config, _T("Software\\ConEmu"));

	gSet.BufferHeight = 0;
	gSet.LogFont.lfHeight = 16;
	gSet.LogFont.lfWidth = 0;
	gSet.LogFont.lfEscapement = gSet.LogFont.lfOrientation = 0;
	gSet.LogFont.lfWeight = FW_NORMAL;
	gSet.LogFont.lfItalic = gSet.LogFont.lfUnderline = gSet.LogFont.lfStrikeOut = FALSE;
	gSet.LogFont.lfCharSet = DEFAULT_CHARSET;
	gSet.LogFont.lfOutPrecision = OUT_TT_PRECIS;
	gSet.LogFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	gSet.LogFont.lfQuality = ANTIALIASED_QUALITY;
	gSet.LogFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    _tcscpy(gSet.LogFont.lfFaceName, _T("Lucida Console"));
    _tcscpy(gSet.LogFont2.lfFaceName, _T("Lucida Console"));
	
	Registry RegConColors, RegConDef;
	if (RegConColors.OpenKey(_T("Console"), KEY_READ))
	{
		RegConDef.OpenKey(HKEY_USERS, _T(".DEFAULT\\Console"), KEY_READ);

		TCHAR ColorName[] = _T("ColorTable00");
		for (uint i = 0x10; i--;)
		{
			ColorName[10] = i/10 + '0';
			ColorName[11] = i%10 + '0';
			if (!RegConColors.Load(ColorName, (DWORD *)&gSet.Colors[i]))
				RegConDef.Load(ColorName, (DWORD *)&gSet.Colors[i]);
		}

		RegConDef.CloseKey();
		RegConColors.CloseKey();
	}

//------------------------------------------------------------------------
///| Default settings |///////////////////////////////////////////////////
//------------------------------------------------------------------------
    _tcscpy(gSet.pBgImage, _T("c:\\back.bmp"));
    gSet.isFixFarBorders = true;
    gSet.bgImageDarker = 0;
    gSet.wndHeight = 25; // NightRoman
    gSet.wndWidth = 80;  // NightRoman
    gSet.isConVisible = false;
    gSet.isDefCopy = 2;
    gSet.nSlideShowElapse = 2500;
    gSet.nIconID = IDI_ICON1;
    gSet.isRClickSendKey = 2;
	_tcscpy(gSet.szTabPanels, _T("Panels"));
	_tcscpy(gSet.szTabEditor, _T("[%s]"));
	_tcscpy(gSet.szTabEditorModified, _T("[%s] *"));
	_tcscpy(gSet.szTabViewer, _T("{%s}"));
	gSet.nTabLenMax = 20;
    gSet.isScrollTitle = true;
    gSet.ScrollTitleLen = 22;
}

void CSettings::LoadSettings()
{
    DWORD inSize = gSet.LogFont.lfHeight;
    TCHAR inFont[MAX_PATH], inFont2[MAX_PATH];
    _tcscpy(inFont, gSet.LogFont.lfFaceName);
    _tcscpy(inFont2, gSet.LogFont2.lfFaceName);
    DWORD Quality = gSet.LogFont.lfQuality;
    gConEmu.WindowMode = rMaximized;
    DWORD FontCharSet = gSet.LogFont.lfCharSet;
    bool isBold = (gSet.LogFont.lfWeight>=FW_BOLD), isItalic = (gSet.LogFont.lfItalic!=FALSE);
    
//------------------------------------------------------------------------
///| Loading from registry |//////////////////////////////////////////////
//------------------------------------------------------------------------
    Registry reg;
    if (reg.OpenKey(gSet.Config, KEY_READ)) // NightRoman
    {
        reg.Load(_T("FontName"), inFont);
        reg.Load(_T("FontName2"), inFont2);
        reg.Load(_T("CmdLine"), gSet.Cmd);
        reg.Load(_T("BackGround Image"), gSet.pBgImage);
        reg.Load(_T("bgImageDarker"), &gSet.bgImageDarker);
        reg.Load(_T("FontSize"), &inSize);
        reg.Load(_T("FontSizeX"), &gSet.FontSizeX);
		reg.Load(_T("FontSizeX3"), &gSet.FontSizeX3);
        reg.Load(_T("FontSizeX2"), &gSet.FontSizeX2);
        reg.Load(_T("FontCharSet"), &FontCharSet);
        reg.Load(_T("Anti-aliasing"), &Quality);
        reg.Load(_T("WindowMode"), &gConEmu.WindowMode);
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
		reg.Load(_T("TabPanels"), &gSet.szTabPanels);
		reg.Load(_T("TabEditor"), &gSet.szTabEditor);
		reg.Load(_T("TabEditorModified"), &gSet.szTabEditorModified);
		reg.Load(_T("TabViewer"), &gSet.szTabViewer);
		reg.Load(_T("TabLenMax"), &gSet.nTabLenMax);
		reg.Load(_T("ScrollTitle"), &gSet.isScrollTitle);
		reg.Load(_T("ScrollTitleLen"), &gSet.ScrollTitleLen);
        reg.CloseKey();
    }

    /*if (gSet.wndWidth)
        pVCon->TextWidth = gSet.wndWidth;
    if (gSet.wndHeight)
        pVCon->TextHeight = gSet.wndHeight;*/

    /*if (gSet.wndHeight && gSet.wndWidth)
    {
        COORD b = {gSet.wndWidth, gSet.wndHeight};
      SetConsoleWindowSize(b,false); // Maximus5 - по аналогии с NightRoman
      //MoveWindow(hConWnd, 0, 0, 1, 1, 0);
      //SetConsoleScreenBufferSize(pVCon->hConOut(), b);
      //MoveWindow(hConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
    }*/

    gSet.LogFont.lfHeight = inSize;
    gSet.LogFont.lfWidth = gSet.FontSizeX;
    _tcscpy(gSet.LogFont.lfFaceName, inFont);
    _tcscpy(gSet.LogFont2.lfFaceName, inFont2);
    gSet.LogFont.lfQuality = Quality;
    if (isBold)
        gSet.LogFont.lfWeight = FW_BOLD;
    gSet.LogFont.lfCharSet = (BYTE) FontCharSet;
    if (isItalic)
        gSet.LogFont.lfItalic = true;

    if (gSet.isShowBgImage)
        LoadImageFrom(gSet.pBgImage);
}

BOOL CSettings::SaveSettings()
{
    Registry reg;
    if (reg.OpenKey(_T("Console"), KEY_WRITE))
    {
        TCHAR ColorName[] = _T("ColorTable00");
        for (uint i = 0x10; i--;)
        {
            ColorName[10] = i/10 + '0';
            ColorName[11] = i%10 + '0';
            reg.Save(ColorName, (DWORD)gSet.Colors[i]);
        }
        reg.CloseKey();

      if (reg.OpenKey(gSet.Config, KEY_WRITE)) // NightRoman
        {
            GetDlgItemText(ghOpWnd, tCmdLine, gSet.Cmd, MAX_PATH);
            RECT rect;
            GetWindowRect(ghWnd, &rect);
            gSet.wndX = rect.left;
            gSet.wndY = rect.top;

            reg.Save(_T("CmdLine"), gSet.Cmd);
            reg.Save(_T("FontName"), gSet.LogFont.lfFaceName);
            reg.Save(_T("FontName2"), gSet.LogFont2.lfFaceName);
            reg.Save(_T("BackGround Image"), gSet.pBgImage);
            reg.Save(_T("bgImageDarker"), gSet.bgImageDarker);
            reg.Save(_T("FontSize"), gSet.LogFont.lfHeight);
            reg.Save(_T("FontSizeX"), gSet.FontSizeX);
            reg.Save(_T("FontSizeX2"), gSet.FontSizeX2);
			reg.Save(_T("FontSizeX3"), gSet.FontSizeX3);
            reg.Save(_T("FontCharSet"), gSet.LogFont.lfCharSet);
            reg.Save(_T("Anti-aliasing"), gSet.LogFont.lfQuality);
            reg.Save(_T("WindowMode"), gSet.isFullScreen ? rFullScreen : IsZoomed(ghWnd) ? rMaximized : rNormal);
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
            reg.Save(_T("FontBold"), gSet.LogFont.lfWeight == FW_BOLD);
            reg.Save(_T("FontItalic"), gSet.LogFont.lfItalic);
            reg.Save(_T("ForceMonospace"), gSet.isForceMonospace);
            reg.Save(_T("Update Console handle"), gSet.isConMan);

            reg.Save(_T("ConWnd Width"), gSet.wndWidth);
            reg.Save(_T("ConWnd Height"), gSet.wndHeight);
            reg.Save(_T("ConWnd X"), gSet.wndX);
            reg.Save(_T("ConWnd Y"), gSet.wndY);

			reg.Save(_T("ScrollTitle"), gSet.isScrollTitle);
			reg.Save(_T("ScrollTitleLen"), gSet.ScrollTitleLen);
            
            reg.CloseKey();
            
            //if (gSet.isTabs==1) ForceShowTabs();
            
            //MessageBoxA(ghOpWnd, "Saved.", "Information", MB_ICONINFORMATION);
            return TRUE;
        }
    }

    MessageBoxA(ghOpWnd, "Failed", "Information", MB_ICONERROR);
	return FALSE;
}


bool CSettings::ShowColorDialog(HWND HWndOwner, COLORREF *inColor)
{
    CHOOSECOLOR cc;                 // common dialog box structure
    //static COLORREF acrCustClr[16]; // array of custom colors

    // Initialize CHOOSECOLOR
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = HWndOwner;
    cc.lpCustColors = (LPDWORD) gSet.Colors;
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
    int far * aiFontCount = (int far *) aFontCount;

    // Record the number of raster, TrueType, and vector
    // fonts in the font-count array.

    if (FontType & RASTER_FONTTYPE)
        aiFontCount[0]++;
    else if (FontType & TRUETYPE_FONTTYPE)
        aiFontCount[2]++;
    else
        aiFontCount[1]++;

    SendDlgItemMessage(ghOpWnd, tFontFace, CB_ADDSTRING, 0, (LPARAM) lplf->lfFaceName);
    SendDlgItemMessage(ghOpWnd, tFontFace2, CB_ADDSTRING, 0, (LPARAM) lplf->lfFaceName);

    if (aiFontCount[0] || aiFontCount[1] || aiFontCount[2])
        return TRUE;
    else
        return FALSE;

    UNREFERENCED_PARAMETER( lplf );
    UNREFERENCED_PARAMETER( lpntm );
}

LRESULT CSettings::OnInitDialog()
{
	{
		HDC hdc = GetDC(ghOpWnd);
		int aFontCount[] = { 0, 0, 0 };
		EnumFontFamilies(hdc, (LPCTSTR) NULL, (FONTENUMPROC) EnumFamCallBack, (LPARAM) aFontCount);
		DeleteDC(hdc);
	    
		TCHAR szTitle[MAX_PATH]; szTitle[0]=0;
		int nConfLen = _tcslen(gSet.Config);
		int nStdLen = strlen("Software\\ConEmu");
		if (nConfLen>(nStdLen+1))
			wsprintf(szTitle, _T("Settings (%s)..."), (gSet.Config+nStdLen+1));
		else
			_tcscpy(szTitle, _T("Settings..."));
		SetWindowText ( ghOpWnd, szTitle );
	}

	SendDlgItemMessage(ghOpWnd, tFontFace, CB_SELECTSTRING, -1, (LPARAM)gSet.LogFont.lfFaceName);
	SendDlgItemMessage(ghOpWnd, tFontFace2, CB_SELECTSTRING, -1, (LPARAM)gSet.LogFont2.lfFaceName);

	{
		const BYTE FSizes[] = {0, 8, 10, 12, 14, 16, 18, 20, 24, 26, 28, 30, 32, 34, 36, 40, 46, 50, 52, 72};
		for (uint i=0; i < sizeofarray(FSizes); i++)
		{
			wsprintf(temp, _T("%i"), FSizes[i]);
			if (i > 0)
				SendDlgItemMessage(ghOpWnd, tFontSizeY, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(ghOpWnd, tFontSizeX, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(ghOpWnd, tFontSizeX2, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(ghOpWnd, tFontSizeX3, CB_ADDSTRING, 0, (LPARAM) temp);
		}

		wsprintf(temp, _T("%i"), gSet.LogFont.lfHeight);
		upToFontHeight = gSet.LogFont.lfHeight;
		if( SendDlgItemMessage(ghOpWnd, tFontSizeY, CB_SELECTSTRING, -1, (LPARAM)temp) == CB_ERR )
			SetDlgItemText(ghOpWnd, tFontSizeY, temp);

		wsprintf(temp, _T("%i"), gSet.FontSizeX);
		if( SendDlgItemMessage(ghOpWnd, tFontSizeX, CB_SELECTSTRING, -1, (LPARAM)temp) == CB_ERR )
			SetDlgItemText(ghOpWnd, tFontSizeX, temp);

		wsprintf(temp, _T("%i"), gSet.FontSizeX2);
		if( SendDlgItemMessage(ghOpWnd, tFontSizeX2, CB_SELECTSTRING, -1, (LPARAM)temp) == CB_ERR )
			SetDlgItemText(ghOpWnd, tFontSizeX2, temp);

		wsprintf(temp, _T("%i"), gSet.FontSizeX3);
		if( SendDlgItemMessage(ghOpWnd, tFontSizeX3, CB_SELECTSTRING, -1, (LPARAM)temp) == CB_ERR )
			SetDlgItemText(ghOpWnd, tFontSizeX3, temp);
	}

	{
		const char *ChSets[] = {"ANSI", "Arabic", "Baltic", "Chinese Big 5", "Default", "East Europe",
			"GB 2312", "Greek", "Hebrew", "Hangul", "Johab", "Mac", "OEM", "Russian", "Shiftjis",
			"Symbol", "Thai", "Turkish", "Vietnamese"};

		u8 num = 4;
		for (uint i=0; i < 19; i++)
		{
			SendDlgItemMessageA(ghOpWnd, tFontCharset, CB_ADDSTRING, 0, (LPARAM) ChSets[i]);
			if (chSetsNums[i] == gSet.LogFont.lfCharSet) num = i;
		}
		SendDlgItemMessage(ghOpWnd, tFontCharset, CB_SETCURSEL, num, 0);
	}

	SetDlgItemText(ghOpWnd, tCmdLine, gSet.Cmd);
	SetDlgItemText(ghOpWnd, tBgImage, gSet.pBgImage);

	TCHAR tmp[10];
	wsprintf(tmp, _T("%i"), gSet.bgImageDarker);
	SendDlgItemMessage(ghOpWnd, tDarker, EM_SETLIMITTEXT, 3, 0);
	SetDlgItemText(ghOpWnd, tDarker, tmp);

	SendDlgItemMessage(ghOpWnd, slDarker, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(0, 255));
	SendDlgItemMessage(ghOpWnd, slDarker, TBM_SETPOS  , (WPARAM) true, (LPARAM) gSet.bgImageDarker);

	if (gSet.isShowBgImage)
		CheckDlgButton(ghOpWnd, cbBgImage, BST_CHECKED);
	else
	{
		EnableWindow(GetDlgItem(ghOpWnd, tBgImage), false);
		EnableWindow(GetDlgItem(ghOpWnd, tDarker), false);
		EnableWindow(GetDlgItem(ghOpWnd, slDarker), false);
	}

	switch(gSet.LogFont.lfQuality)
	{
	case NONANTIALIASED_QUALITY:
		CheckDlgButton(ghOpWnd, rNoneAA, BST_CHECKED);
		break;
	case ANTIALIASED_QUALITY:
		CheckDlgButton(ghOpWnd, rStandardAA, BST_CHECKED);
		break;
	case CLEARTYPE_NATURAL_QUALITY:
		CheckDlgButton(ghOpWnd, rCTAA, BST_CHECKED);
		break;
	}
	if (gSet.isFixFarBorders)   CheckDlgButton(ghOpWnd, cbFixFarBorders, BST_CHECKED);
	if (gSet.isCursorColor) CheckDlgButton(ghOpWnd, cbCursorColor, BST_CHECKED);
	if (gSet.isRClickSendKey) CheckDlgButton(ghOpWnd, cbRClick, (gSet.isRClickSendKey==1) ? BST_CHECKED : BST_INDETERMINATE);
	if (gSet.isSentAltEnter) CheckDlgButton(ghOpWnd, cbSendAE, BST_CHECKED);

	if (gSet.isDnD)
	{
		CheckDlgButton(ghOpWnd, cbDnD, BST_CHECKED);
		EnableWindow(GetDlgItem(ghOpWnd, cbDnDCopy), true);
	}
	else
		EnableWindow(GetDlgItem(ghOpWnd, cbDnDCopy), false);
	if (gSet.isDefCopy) CheckDlgButton(ghOpWnd, cbDnDCopy, (gSet.isDefCopy==1) ? BST_CHECKED : BST_INDETERMINATE);

	if (gSet.isGUIpb) CheckDlgButton(ghOpWnd, cbGUIpb, BST_CHECKED);
	if (gSet.isTabs) CheckDlgButton(ghOpWnd, cbTabs, (gSet.isTabs==1) ? BST_CHECKED : BST_INDETERMINATE);
	if (gSet.isCursorV)
		CheckDlgButton(ghOpWnd, rCursorV, BST_CHECKED);
	else
		CheckDlgButton(ghOpWnd, rCursorH, BST_CHECKED);
	if (gSet.isForceMonospace)
		CheckDlgButton(ghOpWnd, cbMonospace, BST_CHECKED);
	if (gSet.isConMan)
		CheckDlgButton(ghOpWnd, cbIsConMan, BST_CHECKED);

	if (gSet.LogFont.lfWeight == FW_BOLD) CheckDlgButton(ghOpWnd, cbBold, BST_CHECKED);
	if (gSet.LogFont.lfItalic)            CheckDlgButton(ghOpWnd, cbItalic, BST_CHECKED);

	if (gSet.isFullScreen)
		CheckRadioButton(ghOpWnd, rNormal, rFullScreen, rFullScreen);
	else if (IsZoomed(ghWnd))
		CheckRadioButton(ghOpWnd, rNormal, rFullScreen, rMaximized);
	else
		CheckRadioButton(ghOpWnd, rNormal, rFullScreen, rNormal);

	wsprintf(temp, _T("%i"), gSet.wndWidth);
	SetDlgItemText(ghOpWnd, tWndWidth, temp);
	SendDlgItemMessage(ghOpWnd, tWndWidth, EM_SETLIMITTEXT, 3, 0);

	wsprintf(temp, _T("%i"), gSet.wndHeight);
	SetDlgItemText(ghOpWnd, tWndHeight, temp);
	SendDlgItemMessage(ghOpWnd, tWndHeight, EM_SETLIMITTEXT, 3, 0);

	if (!gSet.isFullScreen && !IsZoomed(ghWnd))
	{
		EnableWindow(GetDlgItem(ghOpWnd, tWndWidth), true);
		EnableWindow(GetDlgItem(ghOpWnd, tWndHeight), true);

	}
	else
	{
		EnableWindow(GetDlgItem(ghOpWnd, tWndWidth), false);
		EnableWindow(GetDlgItem(ghOpWnd, tWndHeight), false);

	}

	#define getR(inColorref) (byte)inColorref
	#define getG(inColorref) (byte)(inColorref >> 8)
	#define getB(inColorref) (byte)(inColorref >> 16)

	for (uint i = 0; i < 16; i++)
	{
		SendDlgItemMessage(ghOpWnd, 1100 + i, EM_SETLIMITTEXT, 11, 0);
		wsprintf(temp, _T("%i %i %i"), getR(gSet.Colors[i]), getG(gSet.Colors[i]), getB(gSet.Colors[i]));
		SetDlgItemText(ghOpWnd, 1100 + i, temp);
	}

	{
		RECT rect;
		GetWindowRect(ghOpWnd, &rect);
		MoveWindow(ghOpWnd, GetSystemMetrics(SM_CXSCREEN)/2 - (rect.right - rect.left)/2, GetSystemMetrics(SM_CYSCREEN)/2 - (rect.bottom - rect.top)/2, rect.right - rect.left, rect.bottom - rect.top, false);
	}

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
		if (gSet.isTabs==1) gConEmu.ForceShowTabs(TRUE); else
		if (gSet.isTabs==0) gConEmu.ForceShowTabs(FALSE); // там еще есть '==2', но его здесь не обрабатываем
        SendMessage(ghOpWnd, WM_CLOSE, 0, 0);
        break;

    case rNoneAA:
    case rStandardAA:
    case rCTAA:
        gSet.LogFont.lfQuality = wParam == rNoneAA ? NONANTIALIASED_QUALITY : wParam == rStandardAA ? ANTIALIASED_QUALITY : CLEARTYPE_NATURAL_QUALITY;
        DeleteObject(pVCon->hFont);
        pVCon->hFont = 0;
        gSet.LogFont.lfWidth = gSet.FontSizeX;
        pVCon->Update(true);
        InvalidateRect(ghWnd, NULL, FALSE);
        break;

    case bSaveSettings:
        if (gSet.SaveSettings())
			SendMessage(ghOpWnd,WM_COMMAND,IDOK,0);
        break;

    case rNormal:
    case rFullScreen:
    case rMaximized:
        gConEmu.SetWindowMode(wParam);
        break;

    case cbFixFarBorders:
        gSet.isFixFarBorders = !gSet.isFixFarBorders;

        pVCon->Update(true);
        InvalidateRect(ghWnd, NULL, FALSE);
        break;

    case cbCursorColor:
        gSet.isCursorColor = !gSet.isCursorColor;

        pVCon->Update(true);
        InvalidateRect(ghWnd, NULL, FALSE);
        break;

    case cbBold:
    case cbItalic:
        {
            if (wParam == cbBold)
                gSet.LogFont.lfWeight = SendDlgItemMessage(ghOpWnd, cbBold, BM_GETCHECK, BST_CHECKED, 0) == BST_CHECKED ? FW_BOLD : FW_NORMAL;
            else if (wParam == cbItalic)
                gSet.LogFont.lfItalic = SendDlgItemMessage(ghOpWnd, cbItalic, BM_GETCHECK, BST_CHECKED, 0) == BST_CHECKED ? true : false;

            gSet.LogFont.lfWidth = gSet.FontSizeX;
            HFONT hFont = pVCon->CreateFontIndirectMy(&gSet.LogFont);
            if (hFont)
            {
                DeleteObject(pVCon->hFont);
                pVCon->hFont = hFont;

                pVCon->Update(true);
				if (!gSet.isFullScreen && !IsZoomed(ghWnd))
                    gConEmu.SyncWindowToConsole();
				else
                    gConEmu.SyncConsoleToWindow();
				gConEmu.ReSize();
                InvalidateRect(ghWnd, 0, 0);
            }
        }
        break;

    case cbBgImage:
        gSet.isShowBgImage = SendDlgItemMessage(ghOpWnd, cbBgImage, BM_GETCHECK, BST_CHECKED, 0) == BST_CHECKED ? true : false;
        EnableWindow(GetDlgItem(ghOpWnd, tBgImage), gSet.isShowBgImage);
        EnableWindow(GetDlgItem(ghOpWnd, tDarker), gSet.isShowBgImage);
        EnableWindow(GetDlgItem(ghOpWnd, slDarker), gSet.isShowBgImage);

        if (gSet.isShowBgImage && gSet.isBackgroundImageValid)
            SetBkMode(pVCon->hDC, TRANSPARENT);
        else
            SetBkMode(pVCon->hDC, OPAQUE);

        pVCon->Update(true);
        InvalidateRect(ghWnd, NULL, FALSE);
        break;

    case cbRClick:
        //gSet.isRClickSendKey = !gSet.isRClickSendKey;
        switch(IsDlgButtonChecked(ghOpWnd, cbRClick)) {
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
        EnableWindow(GetDlgItem(ghOpWnd, cbDnDCopy), gSet.isDnD);
        break;
    
    case cbDnDCopy:
        switch(IsDlgButtonChecked(ghOpWnd, cbDnDCopy)) {
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
        switch(IsDlgButtonChecked(ghOpWnd, cbTabs)) {
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
        InvalidateRect(ghWnd, NULL, FALSE);
        break;

    case cbIsConMan:
        gSet.isConMan = !gSet.isConMan;

        pVCon->Update(true);
        InvalidateRect(ghWnd, NULL, FALSE);
        break;

    case rCursorH:
    case rCursorV:
        if (wParam == rCursorV)
            gSet.isCursorV = true;
        else
            gSet.isCursorV = false;

        pVCon->Update(true);
        InvalidateRect(ghWnd, NULL, FALSE);
        break;

    default:
        if (CB >= 1000 && CB <= 1015)
        {
            COLORREF color = gSet.Colors[CB - 1000];
            if( gSet.ShowColorDialog(ghOpWnd, &color) )
            {
                gSet.Colors[CB - 1000] = color;
                wsprintf(temp, _T("%i %i %i"), getR(color), getG(color), getB(color));
                SetDlgItemText(ghOpWnd, CB + 100, temp);
                InvalidateRect(GetDlgItem(ghOpWnd, CB), 0, 1);

                pVCon->Update(true);
                InvalidateRect(ghWnd, NULL, FALSE);
            }
        }
    }

	return 0;
}

LRESULT CSettings::OnEditChanged(WPARAM wParam, LPARAM lParam)
{
    WORD TB = LOWORD(wParam);
    if (TB >= 1100 && TB <= 1115)
    {
        int r, g, b;
        GetDlgItemText(ghOpWnd, TB, temp, MAX_PATH);
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
                if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && gSet.Colors[TB - 1100] != RGB(r, g, b))
                {
                    gSet.Colors[TB - 1100] = RGB(r, g, b);
                    if (pVCon) pVCon->Update(true);
                    if (ghWnd) InvalidateRect(ghWnd, 0, 1);
                    InvalidateRect(GetDlgItem(ghOpWnd, TB - 100), 0, 1);
                }
            }
        }
    }
    else if (TB == tBgImage)
    {
        GetDlgItemText(ghOpWnd, tBgImage, temp, MAX_PATH);
        if( gSet.LoadImageFrom(temp) )
        {
            if (gSet.isShowBgImage && gSet.isBackgroundImageValid)
                SetBkMode(pVCon->hDC, TRANSPARENT);
            else
                SetBkMode(pVCon->hDC, OPAQUE);

            pVCon->Update(true);
            InvalidateRect(ghWnd, NULL, FALSE);
        }
    }
    else if ( (TB == tWndWidth || TB == tWndHeight) && IsDlgButtonChecked(ghOpWnd, rNormal) == BST_CHECKED )
    {
        DWORD newX, newY;
        GetDlgItemText(ghOpWnd, tWndWidth, temp, MAX_PATH);
        newX = klatoi(temp);
        GetDlgItemText(ghOpWnd, tWndHeight, temp, MAX_PATH);
        newY = klatoi(temp);

        if (newX > 24 && newY > 7)
        {
            gSet.wndWidth = newX;
            gSet.wndHeight = newY;

            COORD b = {gSet.wndWidth, gSet.wndHeight};
      gConEmu.SetConsoleWindowSize(b, false);  // NightRoman
      //MoveWindow(hConWnd, 0, 0, 1, 1, 0);
      //SetConsoleScreenBufferSize(pVCon->hConOut(), b);
      //MoveWindow(hConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
        }

    }
    else if (TB == tDarker)
    {
        DWORD newV;
        TCHAR tmp[10];
        GetDlgItemText(ghOpWnd, tDarker, tmp, 10);
        newV = klatoi(tmp);
        if (newV < 256 && newV != gSet.bgImageDarker)
        {
            gSet.bgImageDarker = newV;
            SendDlgItemMessage(ghOpWnd, slDarker, TBM_SETPOS, (WPARAM) true, (LPARAM) gSet.bgImageDarker);
            gSet.LoadImageFrom(gSet.pBgImage);
            pVCon->Update(true);
            InvalidateRect(ghWnd, NULL, FALSE);
        }
    }

	return 0;
}

LRESULT CSettings::OnComboBox(WPARAM wParam, LPARAM lParam)
{
    if (LOWORD(wParam) == tFontFace || LOWORD(wParam) == tFontFace2)
    {
        LOGFONT* pLogFont = (LOWORD(wParam) == tFontFace) ? &gSet.LogFont : &gSet.LogFont2;
        int nID = (LOWORD(wParam) == tFontFace) ? tFontFace : tFontFace2;
        _tcscpy(temp, pLogFont->lfFaceName);
        if (HIWORD(wParam) == CBN_EDITCHANGE)
            GetDlgItemText(ghOpWnd, nID, pLogFont->lfFaceName, LF_FACESIZE);
        else
            SendDlgItemMessage(ghOpWnd, nID, CB_GETLBTEXT, SendDlgItemMessage(ghOpWnd, nID, CB_GETCURSEL, 0, 0), (LPARAM)pLogFont->lfFaceName);

        if (HIWORD(wParam) == CBN_EDITCHANGE)
        {
            LRESULT a = SendDlgItemMessage(ghOpWnd, nID, CB_FINDSTRINGEXACT, -1, (LPARAM)pLogFont->lfFaceName);
            if(a == CB_ERR)
            {
                _tcscpy(pLogFont->lfFaceName, temp);
                return -1;
            }
        }

        BYTE qWas = pLogFont->lfQuality;
        pLogFont->lfHeight = upToFontHeight;
        pLogFont->lfWidth = gSet.FontSizeX;
        HFONT hFont = pVCon->CreateFontIndirectMy(&gSet.LogFont);
        if (hFont)
        {
            if (LOWORD(wParam) == tFontFace) {
                DeleteObject(pVCon->hFont);
                pVCon->hFont = hFont;
            } else {
                DeleteObject(pVCon->hFont2);
                pVCon->hFont2 = hFont;
            }

            pVCon->Update(true);
            if (!gSet.isFullScreen && !IsZoomed(ghWnd))
                gConEmu.SyncWindowToConsole();
            else
                gConEmu.SyncConsoleToWindow();
			gConEmu.ReSize();
            InvalidateRect(ghWnd, 0, 0);

            if (LOWORD(wParam) == tFontFace) {
                wsprintf(temp, _T("%i"), pLogFont->lfHeight);
                SetDlgItemText(ghOpWnd, tFontSizeY, temp);
            }
        }
    }
    else if (LOWORD(wParam) == tFontSizeY || LOWORD(wParam) == tFontSizeX || 
		LOWORD(wParam) == tFontSizeX2 || LOWORD(wParam) == tFontSizeX3 || LOWORD(wParam) == tFontCharset)
    {
        int newSize = 0;
        if (LOWORD(wParam) == tFontSizeY || LOWORD(wParam) == tFontSizeX || 
			LOWORD(wParam) == tFontSizeX2 || LOWORD(wParam) == tFontSizeX3)
        {
            if (HIWORD(wParam) == CBN_EDITCHANGE)
                GetDlgItemText(ghOpWnd, LOWORD(wParam), temp, MAX_PATH);
            else
                SendDlgItemMessage(ghOpWnd, LOWORD(wParam), CB_GETLBTEXT, SendDlgItemMessage(ghOpWnd, LOWORD(wParam), CB_GETCURSEL, 0, 0), (LPARAM)temp);

            newSize = klatoi(temp);
        }

        if (newSize > 4 && newSize < 200 || (newSize == 0 && *temp == '0') || LOWORD(wParam) == tFontCharset)
        {
            if (LOWORD(wParam) == tFontSizeY)
                gSet.LogFont.lfHeight = upToFontHeight = newSize;
            else if (LOWORD(wParam) == tFontSizeX)
                gSet.FontSizeX = newSize;
            else if (LOWORD(wParam) == tFontSizeX2)
                gSet.FontSizeX2 = newSize;
            else if (LOWORD(wParam) == tFontSizeX3)
                gSet.FontSizeX3 = newSize;
            else if (LOWORD(wParam) == tFontCharset)
            {
                int newCharSet = SendDlgItemMessage(ghOpWnd, tFontCharset, CB_GETCURSEL, 0, 0);
                if (newCharSet != CB_ERR && newCharSet >= 0 && newCharSet < 19)
                    gSet.LogFont.lfCharSet = chSetsNums[newCharSet];
                gSet.LogFont.lfHeight = upToFontHeight;
            }
            gSet.LogFont.lfWidth = gSet.FontSizeX;

            HFONT hFont = pVCon->CreateFontIndirectMy(&gSet.LogFont);
            if (hFont)
            {
                DeleteObject(pVCon->hFont);
                pVCon->hFont = hFont;

                pVCon->Update(true);
                if (!gSet.isFullScreen && !IsZoomed(ghWnd))
                    gConEmu.SyncWindowToConsole();
                else
                    gConEmu.SyncConsoleToWindow();
				gConEmu.ReSize();
                InvalidateRect(ghWnd, 0, 0);
            }
        }
	}
	return 0;
}


bool CSettings::LoadImageFrom(TCHAR *inPath)
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

BOOL CALLBACK CSettings::wndOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
    switch (messg)
    {
    case WM_INITDIALOG:
        ghOpWnd = hWnd2;
		gSet.OnInitDialog();
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
                    gSet.LoadImageFrom(gSet.pBgImage);
                    pVCon->Update(true);
                    InvalidateRect(ghWnd, NULL, FALSE);
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
        case WM_CLOSE:
        case WM_DESTROY:
            EndDialog(hWnd2, TRUE);
            ghOpWnd = NULL;
            break;
        default:
            return 0;
    }
    return 0;
}
