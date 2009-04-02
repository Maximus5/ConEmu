#include "Header.h"

#define VCURSORWIDTH 2
#define HCURSORHEIGHT 2

CVirtualConsole* CVirtualConsole::Create()
{
    CVirtualConsole* pCon = new CVirtualConsole();
    #pragma message("TODO: CVirtualConsole::Create - создать процесс")
    
    if (gSet.nAttachPID) {
        // Attach - only once
        DWORD dwPID = gSet.nAttachPID; gSet.nAttachPID = 0;
        if (!pCon->AttachPID(dwPID)) {
            delete pCon;
            return NULL;
        }
    } else {
        if (!pCon->StartProcess()) {
            delete pCon;
            return NULL;
        }
    }

    if (gSet.wndHeight && gSet.wndWidth)
    {
        COORD b = {gSet.wndWidth, gSet.wndHeight};
        pCon->SetConsoleSize(b);
    }

    return pCon;
}

CVirtualConsole::CVirtualConsole(/*HANDLE hConsoleOutput*/)
{
    //pVCon = this;
    hConOut_ = NULL;

    memset(&Cursor, 0, sizeof(Cursor));
    Cursor.nBlinkTime = GetCaretBlinkTime();

    TextWidth = TextHeight = Width = Height = 0;
    hDC = NULL; hBitmap = NULL;
    //hBgDc = NULL; hBgBitmap = NULL; gSet.bgBmp.X = 0; gSet.bgBmp.Y = 0;
    //hFont = NULL; hFont2 = NULL; 
    hSelectedFont = NULL; hOldFont = NULL;
    ConChar = NULL; ConAttr = NULL; ConCharX = NULL; tmpOem = NULL; TextParts = NULL;
    memset(&SelectionInfo, 0, sizeof(SelectionInfo));
    IsForceUpdate = false;
    hBrush0 = NULL; hSelectedBrush = NULL; hOldBrush = NULL;
    isEditor = false;
    memset(&csbi, 0, sizeof(csbi));
    m_LastMaxReadCnt = 0; m_LastMaxReadTick = 0;

    nSpaceCount = 1000;
    Spaces = (TCHAR*)malloc(nSpaceCount*sizeof(TCHAR));
    for (UINT i=0; i<nSpaceCount; i++) Spaces[i]=L' ';

    hOldBrush = NULL;
    hOldFont = NULL;
    
    if (gSet.wndWidth)
        TextWidth = gSet.wndWidth;
    if (gSet.wndHeight)
        TextHeight = gSet.wndHeight;
    

    if (gSet.isShowBgImage)
        gSet.LoadImageFrom(gSet.sBgImage);


    // InitDC звать бессмысленно - консоль еще не создана
    Free();
    //InitDC(false);
}

CVirtualConsole::~CVirtualConsole()
{
    Free();
    if (Spaces) free(Spaces); Spaces = NULL;  nSpaceCount = 0;
    //pVCon = NULL; //??? так на всякий случай
}

void CVirtualConsole::Free()
{
    if (hDC)
    {
        DeleteDC(hDC);
        hDC = NULL;
    }
    if (hBitmap)
    {
        DeleteObject(hBitmap);
        hBitmap = NULL;
    }
    if (ConChar)
    {
        free(ConChar);
        ConChar = NULL;
    }
    if (ConAttr)
    {
        free(ConAttr);
        ConAttr = NULL;
    }
    if (ConCharX)
    {
        free(ConCharX);
        ConCharX = NULL;
    }
    if (tmpOem)
    {
        free(tmpOem);
        tmpOem = NULL;
    }
    if (TextParts)
    {
        free(TextParts);
        TextParts = NULL;
    }
}

// Дабы избежать многоратных Recreate во время обновления - пересоздаем хэндл только в начале Update!
HANDLE CVirtualConsole::hConOut(BOOL abAllowRecreate/*=FALSE*/)
{
    if(hConOut_ && hConOut_!=INVALID_HANDLE_VALUE && !abAllowRecreate) {
        return hConOut_;
    }
    
    if (gSet.isUpdConHandle)
    {
        if(hConOut_ && hConOut_!=INVALID_HANDLE_VALUE) {
            CloseHandle(hConOut_);
            hConOut_ = NULL;
        }

        hConOut_ = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hConOut_ == INVALID_HANDLE_VALUE) {
            // Наверное лучше вернуть текущий хэндл, нежели вообще завалиться...
            hConOut_ = GetStdHandle(STD_OUTPUT_HANDLE);
            //DisplayLastError(_T("CVirtualConsole::hConOut fails"));
            //hConOut_ = NULL;
        }
    } else if (hConOut_==NULL) {
        hConOut_ = GetStdHandle(STD_OUTPUT_HANDLE);
    }
    
    if (hConOut_ == INVALID_HANDLE_VALUE) {
        hConOut_ = NULL;
        DisplayLastError(_T("CVirtualConsole::hConOut fails"));
    }
    
    return hConOut_;
}

// InitDC вызывается только при критических изменениях (размеры, шрифт, и т.п.) когда нужно пересоздать DC и Bitmap
bool CVirtualConsole::InitDC(bool abNoDc)
{
    Free();

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConOut(), &csbi))
        return false;

    IsForceUpdate = true;
    TextWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    TextHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    if ((int)TextWidth < csbi.dwSize.X)
        TextWidth = csbi.dwSize.X;

    ConChar = (TCHAR*)calloc((TextWidth * TextHeight * 2), sizeof(*ConChar));
    ConAttr = (WORD*)calloc((TextWidth * TextHeight * 2), sizeof(*ConAttr));
    ConCharX = (DWORD*)calloc((TextWidth * TextHeight), sizeof(*ConCharX));
    tmpOem = (char*)calloc((TextWidth + 5), sizeof(*tmpOem));
    TextParts = (struct _TextParts*)calloc((TextWidth + 2), sizeof(*TextParts));
    if (!ConChar || !ConAttr || !ConCharX || !tmpOem || !TextParts)
        return false;

    SelectionInfo.dwFlags = 0;

    hSelectedFont = NULL;

    // Это может быть, если отключена буферизация (debug) и вывод идет сразу на экран
    if (!abNoDc)
    {
        const HDC hScreenDC = GetDC(0);
        if (hDC = CreateCompatibleDC(hScreenDC))
        {
            /*SelectFont(gSet.mh_Font);
            TEXTMETRIC tm;
            GetTextMetrics(hDC, &tm);
            if (gSet.isForceMonospace)
                //Maximus - у Arial'а например MaxWidth слишком большой
                gSet.LogFont.lfWidth = gSet.FontSizeX3 ? gSet.FontSizeX3 : tm.tmMaxCharWidth;
            else
                gSet.LogFont.lfWidth = tm.tmAveCharWidth;
            gSet.LogFont.lfHeight = tm.tmHeight;

            if (ghOpWnd) // устанавливать только при листании шрифта в настройке
                gSet.UpdateTTF ( (tm.tmMaxCharWidth - tm.tmAveCharWidth)>2 );*/

            MBoxAssert ( gSet.LogFont.lfWidth && gSet.LogFont.lfHeight );

            // Посчитать новый размер в пикселях
            Width = TextWidth * gSet.LogFont.lfWidth;
            Height = TextHeight * gSet.LogFont.lfHeight;

            hBitmap = CreateCompatibleBitmap(hScreenDC, Width, Height);
            SelectObject(hDC, hBitmap);
        }
        ReleaseDC(0, hScreenDC);

        return hBitmap!=NULL;
    }

    return true;
}

void CVirtualConsole::DumpConsole()
{
    OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize=sizeof(ofn);
    ofn.hwndOwner = ghWnd;
    ofn.lpstrFilter = _T("ConEmu dumps (*.con)\0*.con\0\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = temp;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = _T("Dump console...");
    ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
    if (!GetSaveFileName(&ofn))
        return;
        
    HANDLE hFile = CreateFile(temp, GENERIC_WRITE, FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        DisplayLastError(_T("Can't create file!"));
        return;
    }
    DWORD dw;
    LPCTSTR pszTitle = gConEmu.GetTitle();
    WriteFile(hFile, pszTitle, _tcslen(pszTitle)*sizeof(*pszTitle), &dw, NULL);
    swprintf(temp, _T("\r\nSize: %ix%i\r\n"), TextWidth, TextHeight);
    WriteFile(hFile, temp, _tcslen(temp)*sizeof(TCHAR), &dw, NULL);
    WriteFile(hFile, ConChar, TextWidth * TextHeight * 2, &dw, NULL);
    WriteFile(hFile, ConAttr, TextWidth * TextHeight * 2, &dw, NULL);
    CloseHandle(hFile);
}

/*HFONT CVirtualConsole::CreateFontIndirectMy(LOGFONT *inFont)
{
    memset(FontWidth, 0, sizeof(*FontWidth)*0x10000);
    //memset(Font2Width, 0, sizeof(*Font2Width)*0x10000);

    DeleteObject(hFont2); hFont2 = NULL;

    int width = gSet.FontSizeX2 ? gSet.FontSizeX2 : inFont->lfWidth;
    hFont2 = CreateFont(abs(inFont->lfHeight), abs(width), 0, 0, FW_NORMAL,
        0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 0, gSet.LogFont2.lfFaceName);

    return CreateFontIndirect(inFont);
}*/

// Это символы рамок и др. спец. символы
//#define isCharBorder(inChar) (inChar>=0x2013 && inChar<=0x266B)
bool CVirtualConsole::isCharBorder(WCHAR inChar)
{
    if (inChar>=0x2013 && inChar<=0x266B)
        return true;
    else
        return false;
    //if (gSet.isFixFarBorders)
    //{
    //  //if (! (inChar > 0x2500 && inChar < 0x251F))
    //  if ( !(inChar > 0x2013/*En dash*/ && inChar < 0x266B/*Beamed Eighth Notes*/) /*&& inChar!=L'}'*/ )
    //      /*if (inChar != 0x2550 && inChar != 0x2502 && inChar != 0x2551 && inChar != 0x007D &&
    //      inChar != 0x25BC && inChar != 0x2593 && inChar != 0x2591 && inChar != 0x25B2 &&
    //      inChar != 0x2562 && inChar != 0x255F && inChar != 0x255A && inChar != 0x255D &&
    //      inChar != 0x2554 && inChar != 0x2557 && inChar != 0x2500 && inChar != 0x2534 && inChar != 0x2564) // 0x2520*/
    //      return false;
    //  else
    //      return true;
    //}
    //else
    //{
    //  if (inChar < 0x01F1 || inChar > 0x0400 && inChar < 0x045F || inChar > 0x2012 && inChar < 0x203D || /*? - not sure that optimal*/ inChar > 0x2019 && inChar < 0x2303 || inChar > 0x24FF && inChar < 0x266C)
    //      return false;
    //  else
    //      return true;
    //}
}

// А это только "рамочные" символы, в которых есть любая (хотя бы частичная) вертикальная черта + стрелки/штриховки
bool CVirtualConsole::isCharBorderVertical(WCHAR inChar)
{
    //if (inChar>=0x2502 && inChar<=0x25C4 && inChar!=0x2550)
    if (inChar==0x2502 || inChar==0x2503 || inChar==0x2506 || inChar==0x2507
        || (inChar>=0x250A && inChar<=0x254B) || inChar==0x254E || inChar==0x254F
        || (inChar>=0x2551 && inChar<=0x25C5)) // По набору символов Arial Unicode MS
        return true;
    else
        return false;
}

void CVirtualConsole::BlitPictureTo(int inX, int inY, int inWidth, int inHeight)
{
    // А вообще, имеет смысл отрисовывать?
    if (gSet.bgBmp.X>inX && gSet.bgBmp.Y>inY)
        BitBlt(hDC, inX, inY, inWidth, inHeight, gSet.hBgDc, inX, inY, SRCCOPY);
    if (gSet.bgBmp.X<(inX+inWidth) || gSet.bgBmp.Y<(inY+inHeight))
    {
        if (hBrush0==NULL) {
            hBrush0 = CreateSolidBrush(gSet.Colors[0]);
            SelectBrush(hBrush0);
        }

        RECT rect = {max(inX,gSet.bgBmp.X), inY, inX+inWidth, inY+inHeight};
        if (!IsRectEmpty(&rect))
            FillRect(hDC, &rect, hBrush0);

        if (gSet.bgBmp.X>inX) {
            rect.left = inX; rect.top = max(inY,gSet.bgBmp.Y); rect.right = gSet.bgBmp.X;
            if (!IsRectEmpty(&rect))
                FillRect(hDC, &rect, hBrush0);
        }

        //DeleteObject(hBrush);
    }
}

void CVirtualConsole::SelectFont(HFONT hNew)
{
    if (!hNew) {
        if (hOldFont)
            SelectObject(hDC, hOldFont);
        hOldFont = NULL;
        hSelectedFont = NULL;
    } else
    if (hSelectedFont != hNew)
    {
        hSelectedFont = (HFONT)SelectObject(hDC, hNew);
        if (!hOldFont)
            hOldFont = hSelectedFont;
        hSelectedFont = hNew;
    }
}

void CVirtualConsole::SelectBrush(HBRUSH hNew)
{
    if (!hNew) {
        if (hOldBrush)
            SelectObject(hDC, hOldBrush);
        hOldBrush = NULL;
    } else
    if (hSelectedBrush != hNew)
    {
        hSelectedBrush = (HBRUSH)SelectObject(hDC, hNew);
        if (!hOldBrush)
            hOldBrush = hSelectedBrush;
        hSelectedBrush = hNew;
    }
}

bool CVirtualConsole::CheckSelection(const CONSOLE_SELECTION_INFO& select, SHORT row, SHORT col)
{
    if ((select.dwFlags & CONSOLE_SELECTION_NOT_EMPTY) == 0)
        return false;
    if (row < select.srSelection.Top || row > select.srSelection.Bottom)
        return false;
    if (col < select.srSelection.Left || col > select.srSelection.Right)
        return false;
    return true;
}

#ifdef _DEBUG
class DcDebug {
public:
    DcDebug(HDC* ahDcVar, HDC* ahPaintDC) {
        mb_Attached=FALSE; mh_OrigDc=NULL; mh_DcVar=NULL; mh_Dc=NULL;
        if (!ahDcVar || !ahPaintDC) return;
        mh_DcVar = ahDcVar;
        mh_OrigDc = *ahDcVar;
        mh_Dc = *ahPaintDC;
        *mh_DcVar = mh_Dc;
    };
    ~DcDebug() {
        if (mb_Attached && mh_DcVar) {
            mb_Attached = FALSE;
            *mh_DcVar = mh_OrigDc;
        }
    };
protected:
    BOOL mb_Attached;
    HDC mh_OrigDc, mh_Dc;
    HDC* mh_DcVar;
};
#endif

// Возвращает true, если процедура изменила отображаемый символ
bool CVirtualConsole::GetCharAttr(TCHAR ch, WORD atr, TCHAR& rch, BYTE& foreColorNum, BYTE& backColorNum)
{
    bool bChanged = false;
    foreColorNum = atr & 0x0F;
    backColorNum = atr >> 4 & 0x0F;
    rch = ch; // по умолчанию!
    if (isEditor && gSet.isVisualizer && ch==L' ' &&
        (backColorNum==gSet.nVizTab || backColorNum==gSet.nVizEOL || backColorNum==gSet.nVizEOF))
    {
        if (backColorNum==gSet.nVizTab)
            rch = gSet.cVizTab; else
        if (backColorNum==gSet.nVizEOL)
            rch = gSet.cVizEOL; else
        if (backColorNum==gSet.nVizEOF)
            rch = gSet.cVizEOF;
        backColorNum = gSet.nVizNormal;
        foreColorNum = gSet.nVizFore;
        bChanged = true;
    } else
    if (gSet.isExtendColors) {
        if (backColorNum==gSet.nExtendColor) {
            backColorNum = attrBackLast;
            foreColorNum += 0x10;
        } else {
            attrBackLast = backColorNum;
        }
    }
    return bChanged;
}

// Возвращает ширину символа, учитывает FixBorders
WORD CVirtualConsole::CharWidth(TCHAR ch)
{
    if (gSet.isForceMonospace)
        return gSet.LogFont.lfWidth;

    WORD nWidth = gSet.LogFont.lfWidth;
    bool isBorder = false; //, isVBorder = false;

    if (gSet.isFixFarBorders) {
        isBorder = isCharBorder(ch);
        //if (isBorder) {
        //  isVBorder = ch == 0x2551 /*Light Vertical*/ || ch == 0x2502 /*Double Vertical*/;
        //}
    }

    //if (isBorder) {
        //if (!Font2Width[ch]) {
        //  if (!isVBorder) {
        //      Font2Width[ch] = gSet.LogFont.lfWidth;
        //  } else {
        //      SelectFont(hFont2);
        //      SIZE sz;
        //      if (GetTextExtentPoint32(hDC, &ch, 1, &sz) && sz.cx)
        //          Font2Width[ch] = sz.cx;
        //      else
        //          Font2Width[ch] = gSet.LogFont2.lfWidth;
        //  }
        //}
        //nWidth = Font2Width[ch];
    //} else {
    if (!isBorder) {
        if (!gSet.FontWidth[ch]) {
            SelectFont(gSet.mh_Font);
            SIZE sz;
            if (GetTextExtentPoint32(hDC, &ch, 1, &sz) && sz.cx)
                gSet.FontWidth[ch] = sz.cx;
            else
                gSet.FontWidth[ch] = nWidth;
        }
        nWidth = gSet.FontWidth[ch];
    }
    if (!nWidth) nWidth = 1; // на всякий случай, чтобы деления на 0 не возникло
    return nWidth;
}

bool CVirtualConsole::CheckChangedTextAttr()
{
    textChanged = 0!=memcmp(ConChar + TextLen, ConChar, TextLen * sizeof(*ConChar));
    attrChanged = 0!=memcmp(ConAttr + TextLen, ConAttr, TextLen * sizeof(*ConAttr));
#ifdef _DEBUG
    COORD ch;
    if (textChanged) {
        for (UINT i=0,j=TextLen; i<TextLen; i++,j++) {
            if (ConChar[i] != ConChar[j]) {
                ch.Y = i % TextWidth;
                ch.X = i - TextWidth * ch.Y;
                break;
            }
        }
    }
    if (attrChanged) {
        for (UINT i=0,j=TextLen; i<TextLen; i++,j++) {
            if (ConAttr[i] != ConAttr[j]) {
                ch.Y = i % TextWidth;
                ch.X = i - TextWidth * ch.Y;
                break;
            }
        }
    }
#endif

    return textChanged || attrChanged;
}

bool CVirtualConsole::Update(bool isForce, HDC *ahDc)
{
    #ifdef _DEBUG
    DcDebug dbg(&hDC, ahDc); // для отладки - рисуем сразу на канвасе окна
    #endif

    // переоткрыть хэндл Output'а при необходимости
    if (!hConOut(TRUE)) {
        return false;
    }

    MCHKHEAP
    bool lRes = false;
    
    if (!GetConsoleScreenBufferInfo(hConOut(), &csbi)) {
        DisplayLastError(_T("Update:GetConsoleScreenBufferInfo"));
        return lRes;
    }

    // start timer before ReadConsoleOutput* calls, they do take time
    gSet.Performance(tPerfRead, FALSE);

    //if (gbNoDblBuffer) isForce = TRUE; // Debug, dblbuffer

    //------------------------------------------------------------------------
    ///| Read console output and cursor info... |/////////////////////////////
    //------------------------------------------------------------------------
    if (!UpdatePrepare(isForce, ahDc)) {
        gConEmu.DnDstep(_T("DC initialization failed!"));
        return false;
    }
    
    gSet.Performance(tPerfRead, TRUE);

    //gSet.Performance(tPerfRender, FALSE);

    //------------------------------------------------------------------------
    ///| Drawing text (if there were changes in console) |////////////////////
    //------------------------------------------------------------------------
    bool updateText, updateCursor;
    if (isForce)
    {
        updateText = updateCursor = attrChanged = textChanged = true;
    }
    else
    {
        // Do we have to update changed text?
        updateText = doSelect || CheckChangedTextAttr();
            //(textChanged = 0!=memcmp(ConChar + TextLen, ConChar, TextLen * sizeof(TCHAR))) || 
            //(attrChanged = 0!=memcmp(ConAttr + TextLen, ConAttr, TextLen * 2));
        
        // Do we have to update text under changed cursor?
        // Important: check both 'cinf.bVisible' and 'Cursor.isVisible',
        // because console may have cursor hidden and its position changed -
        // in this case last visible cursor remains shown at its old place.
        // Also, don't check foreground window here for the same reasons.
        // If position is the same then check the cursor becomes hidden.
        if (Cursor.x != csbi.dwCursorPosition.X || Cursor.y != csbi.dwCursorPosition.Y)
            // сменилась позиция. обновляем если курсор видим
            updateCursor = cinf.bVisible || Cursor.isVisible || Cursor.isVisiblePrevFromInfo;
        else
            updateCursor = Cursor.isVisiblePrevFromInfo && !cinf.bVisible;
    }
    
    gSet.Performance(tPerfRender, FALSE);

    if (updateText /*|| updateCursor*/)
    {
        lRes = true;

        //------------------------------------------------------------------------
        ///| Drawing modified text |//////////////////////////////////////////////
        //------------------------------------------------------------------------
        UpdateText(isForce, updateText, updateCursor);


        //MCHKHEAP
        //------------------------------------------------------------------------
        ///| Now, store data for further comparison |/////////////////////////////
        //------------------------------------------------------------------------
        //if (updateText)
        {
            memcpy(ConChar + TextLen, ConChar, TextLen * sizeof(TCHAR));
            memcpy(ConAttr + TextLen, ConAttr, TextLen * 2);
        }
    }

    //MCHKHEAP
    //------------------------------------------------------------------------
    ///| Drawing cursor |/////////////////////////////////////////////////////
    //------------------------------------------------------------------------
    UpdateCursor(lRes);

    gSet.Performance(tPerfRender, TRUE);

    /* ***************************************** */
    /*       Finalization, release objects       */
    /* ***************************************** */
    SelectBrush(NULL);
    if (hBrush0) {
        DeleteObject(hBrush0); hBrush0=NULL;
    }
    SelectFont(NULL);
    MCHKHEAP
    return lRes;
}

bool CVirtualConsole::UpdatePrepare(bool isForce, HDC *ahDc)
{
    attrBackLast = 0;
    isEditor = gConEmu.isEditor();
    isFilePanel = gConEmu.isFilePanel(true);

    winSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1; winSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    if (winSize.X < csbi.dwSize.X)
        winSize.X = csbi.dwSize.X;
    csbi.dwCursorPosition.X -= csbi.srWindow.Left;
    csbi.dwCursorPosition.Y -= csbi.srWindow.Top;
    isCursorValid =
        csbi.dwCursorPosition.X >= 0 && csbi.dwCursorPosition.Y >= 0 &&
        csbi.dwCursorPosition.X < winSize.X && csbi.dwCursorPosition.Y < winSize.Y;

    // Первая инициализация, или 
    if (isForce || !ConChar || TextWidth != winSize.X || TextHeight != winSize.Y) {
        if (!InitDC(ahDc!=NULL))
            return false;
    }

    // use and reset additional force flag
    if (IsForceUpdate)
    {
        isForce = IsForceUpdate;
        IsForceUpdate = false;
    }
    if (ahDc)
        isForce = true;

    drawImage = gSet.isShowBgImage && gSet.isBackgroundImageValid;
    TextLen = TextWidth * TextHeight;
    coord.X = csbi.srWindow.Left; coord.Y = csbi.srWindow.Top;

    // Get attributes (first) and text (second)
    // [Roman Kuzmin]
    // In FAR Manager source code this is mentioned as "fucked method". Yes, it is.
    // Functions ReadConsoleOutput* fail if requested data size exceeds their buffer;
    // MSDN says 64K is max but it does not say how much actually we can request now.
    // Experiments show that this limit is floating and it can be much less than 64K.
    // The solution below is not optimal when a user sets small font and large window,
    // but it is safe and practically optimal, because most of users set larger fonts
    // for large window and ReadConsoleOutput works OK. More optimal solution for all
    // cases is not that difficult to develop but it will be increased complexity and
    // overhead often for nothing, not sure that we really should use it.
    DWORD nbActuallyRead;
    if (!ReadConsoleOutputAttribute(hConOut(), ConAttr, TextLen, coord, &nbActuallyRead) ||
        !ReadConsoleOutputCharacter(hConOut(), ConChar, TextLen, coord, &nbActuallyRead))
    {
        WORD* ConAttrNow = ConAttr;
        TCHAR* ConCharNow = ConChar;
        for(int y = 0; y < (int)TextHeight; ++y)
        {
            ReadConsoleOutputAttribute(hConOut(), ConAttrNow, TextWidth, coord, &nbActuallyRead);
            ReadConsoleOutputCharacter(hConOut(), ConCharNow, TextWidth, coord, &nbActuallyRead);
            ConAttrNow += TextWidth;
            ConCharNow += TextWidth;
            ++coord.Y;
        }
    }

    // get cursor info
    GetConsoleCursorInfo(hConOut(), &cinf);

    // get selection info in buffer mode
    
    doSelect = gConEmu.BufferHeight > 0;
    if (doSelect)
    {
        select1 = SelectionInfo;
        GetConsoleSelectionInfo(&select2);
        select2.srSelection.Top -= csbi.srWindow.Top;
        select2.srSelection.Bottom -= csbi.srWindow.Top;
        SelectionInfo = select2;
        doSelect = (select1.dwFlags & CONSOLE_SELECTION_NOT_EMPTY) || (select2.dwFlags & CONSOLE_SELECTION_NOT_EMPTY);
        if (doSelect)
        {
            if (select1.dwFlags == select2.dwFlags &&
                select1.srSelection.Top == select2.srSelection.Top &&
                select1.srSelection.Left == select2.srSelection.Left &&
                select1.srSelection.Right == select2.srSelection.Right &&
                select1.srSelection.Bottom == select2.srSelection.Bottom)
            {
                doSelect = false;
            }
        }
    }

    return true;
}

enum CVirtualConsole::_PartType CVirtualConsole::GetCharType(TCHAR ch)
{
    enum _PartType cType = pNull;

    if (ch == L' ')
        cType = pSpace;
    //else if (ch == L'_')
    //  cType = pUnderscore;
    else if (isCharBorder(ch)) {
        if (isCharBorderVertical(ch))
            cType = pVBorder;
        else
            cType = pBorder;
    }
    else if (isFilePanel && ch == L'}')
        cType = pRBracket;
    else
        cType = pText;

    return cType;
}

// row - 0-based
void CVirtualConsole::ParseLine(int row, TCHAR *ConCharLine, WORD *ConAttrLine)
{
    UINT idx = 0;
    struct _TextParts *pStart=TextParts, *pEnd=TextParts;
    enum _PartType cType1, cType2;
    UINT i1=0, i2=0;
    
    pEnd->partType = pNull; // сразу ограничим строку
    
    TCHAR ch1, ch2;
    BYTE af1, ab1, af2, ab2;
    DWORD pixels;
    while (i1<TextWidth)
    {
        GetCharAttr(ConCharLine[i1], ConAttrLine[i1], ch1, af1, ab1);
        cType1 = GetCharType(ch1);
        if (cType1 == pRBracket) {
            if (!(row>=2 && isCharBorderVertical(ConChar[TextWidth+i1]))
                && (((UINT)row)<=(TextHeight-4)))
                cType1 = pText;
        }
        pixels = CharWidth(ch1);

        i2 = i1+1;
        // в режиме Force Monospace отрисовка идет по одному символу
        if (!gSet.isForceMonospace && i2 < TextWidth && 
            (cType1 != pVBorder && cType1 != pRBracket))
        {
            GetCharAttr(ConCharLine[i2], ConAttrLine[i2], ch2, af2, ab2);
            // Получить блок символов с аналогичными цветами
            while (i2 < TextWidth && af2 == af1 && ab2 == ab1) {
                // если символ отличается от первого

                cType2 = GetCharType(ch2);
                if ((ch2 = ConCharLine[i2]) != ch1) {
                    if (cType2 == pRBracket) {
                        if (!(row>=2 && isCharBorderVertical(ConChar[TextWidth+i2]))
                            && (((UINT)row)<=(TextHeight-4)))
                            cType2 = pText;
                    }

                    // и он вообще из другой группы
                    if (cType2 != cType1)
                        break; // то завершаем поиск
                }
                pixels += CharWidth(ch2); // добавить ширину символа в пикселях
                i2++; // следующий символ
                GetCharAttr(ConCharLine[i2], ConAttrLine[i2], ch2, af2, ab2);
                if (cType2 == pRBracket) {
                    if (!(row>=2 && isCharBorderVertical(ConChar[TextWidth+i2]))
                        && (((UINT)row)<=(TextHeight-4)))
                        cType2 = pText;
                }
            }
        }

        // при разборе строки будем смотреть, если нашли pText,pSpace,pText то pSpace,pText добавить в первый pText
        if (cType1 == pText && (pEnd - pStart) >= 2) {
            if (pEnd[-1].partType == pSpace && pEnd[-2].partType == pText &&
                pEnd[-1].attrBack == ab1 && pEnd[-1].attrFore == af1 &&
                pEnd[-2].attrBack == ab1 && pEnd[-2].attrFore == af1
                )
            {   
                pEnd -= 2;
                pEnd->i2 = i2 - 1;
                pEnd->iwidth = i2 - pEnd->i1;
                pEnd->width += pEnd[1].width + pixels;
                pEnd ++;
                i1 = i2;
                continue;
            }
        }
        pEnd->i1 = i1; pEnd->i2 = i2 - 1; // конец "включая"
        pEnd->partType = cType1;
        pEnd->attrBack = ab1; pEnd->attrFore = af1;
        pEnd->iwidth = i2 - i1;
        pEnd->width = pixels;
        if (gSet.isForceMonospace ||
            (gSet.isTTF && (cType1 == pVBorder || cType1 == pRBracket)))
        {
            pEnd->x1 = i1 * gSet.LogFont.lfWidth;
        } else {
            pEnd->x1 = -1;
        }

        pEnd ++; // блоков не может быть больше количества символов в строке, так что с размерностью все ОК
        i1 = i2;
    }
    // пока поставим конец блоков, потом, если ширины не хватит - добавим pDummy
    pEnd->partType = pNull;
}

void CVirtualConsole::UpdateText(bool isForce, bool updateText, bool updateCursor)
{
    if (!updateText)
        return;

    SelectFont(gSet.mh_Font);

    // pointers
    TCHAR* ConCharLine;
    WORD* ConAttrLine;
    DWORD* ConCharXLine;
    // counters
    int pos, row;
    {
        int i;
        if (updateText)
        {
            i = 0; //TextLen - TextWidth; // TextLen = TextWidth/*80*/ * TextHeight/*25*/;
            pos = 0; //Height - gSet.LogFont.lfHeight; // Height = TextHeight * gSet.LogFont.lfHeight;
            row = 0; //TextHeight - 1;
        }
        else
        { // по идее, сюда вообще не доходим
            i = TextWidth * Cursor.y;
            pos = gSet.LogFont.lfHeight * Cursor.y;
            row = Cursor.y;
        }
        ConCharLine = ConChar + i;
        ConAttrLine = ConAttr + i;
        ConCharXLine = ConCharX + i;
    }
    int nMaxPos = Height - gSet.LogFont.lfHeight;

    if (/*gSet.isForceMonospace ||*/ !drawImage)
        SetBkMode(hDC, OPAQUE);
    else
        SetBkMode(hDC, TRANSPARENT);

    // rows
    //TODO: а зачем в isForceMonospace принудительно перерисовывать все?
    const bool skipNotChanged = !isForce /*&& !gSet.isForceMonospace*/;
    for (; pos <= nMaxPos; 
        ConCharLine += TextWidth, ConAttrLine += TextWidth, ConCharXLine += TextWidth,
        pos += gSet.LogFont.lfHeight, row++)
    {
        // the line
        const WORD* const ConAttrLine2 = ConAttrLine + TextLen;
        const TCHAR* const ConCharLine2 = ConCharLine + TextLen;

        // skip not changed symbols except the old cursor or selection
        int j = 0, end = TextWidth;
        if (skipNotChanged)
        {
            // *) Skip not changed tail symbols.
            while(--end >= 0 && ConCharLine[end] == ConCharLine2[end] && ConAttrLine[end] == ConAttrLine2[end])
            {
                if (updateCursor && row == Cursor.y && end == Cursor.x)
                    break;
                if (doSelect)
                {
                    if (CheckSelection(select1, row, end))
                        break;
                    if (CheckSelection(select2, row, end))
                        break;
                }
            }
            if (end < j)
                continue;
            ++end;

            // *) Skip not changed head symbols.
            while(j < end && ConCharLine[j] == ConCharLine2[j] && ConAttrLine[j] == ConAttrLine2[j])
            {
                if (updateCursor && row == Cursor.y && j == Cursor.x)
                    break;
                if (doSelect)
                {
                    if (CheckSelection(select1, row, j))
                        break;
                    if (CheckSelection(select2, row, j))
                        break;
                }
                ++j;
            }
            if (j >= end)
                continue;
        }
        
        if (Cursor.isVisiblePrev && row == Cursor.y
            && (j <= Cursor.x && Cursor.x <= end))
            Cursor.isVisiblePrev = false;

        // *) Now draw as much as possible in a row even if some symbols are not changed.
        // More calls for the sake of fewer symbols is slower, e.g. in panel status lines.
        int j2=j+1;
        for (; j < end; j = j2)
        {
            const WORD attr = ConAttrLine[j];
            WCHAR c = ConCharLine[j];
            BYTE attrFore, attrBack;
            bool isUnicode = isCharBorder(c/*ConCharLine[j]*/);
            bool lbS1 = false, lbS2 = false;
            int nS11 = 0, nS12 = 0, nS21 = 0, nS22 = 0;

            if (GetCharAttr(c, attr, c, attrFore, attrBack))
                isUnicode = true;

            MCHKHEAP
            // корректировка лидирующих пробелов и рамок
            if (gSet.isTTF && (c==0x2550 || c==0x2500)) // 'Box light horizontal' & 'Box double horizontal' - это всегда
            {
                lbS1 = true; nS11 = nS12 = j;
                while ((nS12 < end) && (ConCharLine[nS12+1] == c))
                    nS12 ++;
                // Посчитать сколько этих же символов ПОСЛЕ текста
                if (nS12<end) {
                    nS21 = nS12+1; // Это должен быть НЕ c 
                    // ищем первый "рамочный" символ
                    while ((nS21<end) && (ConCharLine[nS21] != c) && !isCharBorder(ConCharLine[nS21]))
                        nS21 ++;
                    if (nS21<end && ConCharLine[nS21]==c) {
                        lbS2 = true; nS22 = nS21;
                        while ((nS22 < end) && (ConCharLine[nS22+1] == c))
                            nS22 ++;
                    }
                }
            } MCHKHEAP
            // а вот для пробелов - когда их более одного
            /*else if (c==L' ' && j<end && ConCharLine[j+1]==L' ')
            {
                lbS1 = true; nS11 = nS12 = j;
                while ((nS12 < end) && (ConCharLine[nS12+1] == c))
                    nS12 ++;
            }*/

            // Возможно это тоже нужно вынести в GetCharAttr?
            if (doSelect && CheckSelection(select2, row, j))
            {
                WORD tmp = attrFore;
                attrFore = attrBack;
                attrBack = tmp;
            }

            SetTextColor(hDC, gSet.Colors[attrFore]);

            // корректировка положения вертикальной рамки
            if (gSet.isTTF && j)
            {
                MCHKHEAP
                DWORD nPrevX = ConCharXLine[j-1];
                if (isCharBorderVertical(c)) {
                    ConCharXLine[j-1] = (j-1) * gSet.LogFont.lfWidth;
                } else if (isFilePanel && c==L'}') {
                    // Символом } фар помечает имена файлов вылезшие за пределы колонки...
                    // Если сверху или снизу на той же позиции рамки (или тот же '}')
                    // корректируем позицию
                    if ((row>=2 && isCharBorderVertical(ConChar[TextWidth+j]))
                        && (((UINT)row)<=(TextHeight-4)))
                     ConCharXLine[j-1] = (j-1) * gSet.LogFont.lfWidth;
                    //row = TextHeight - 1;
                } MCHKHEAP
                if (nPrevX < ConCharXLine[j-1]) {
                    // Требуется зарисовать пропущенную область. пробелами что-ли?

                    RECT rect;
                    rect.left = nPrevX;
                    rect.top = pos;
                    rect.right = ConCharXLine[j-1];
                    rect.bottom = rect.top + gSet.LogFont.lfHeight;

                    if (gbNoDblBuffer) GdiFlush();
                    if (! (drawImage && (attrBack) < 2)) {
                        SetBkColor(hDC, gSet.Colors[attrBack]);
                        int nCnt = ((rect.right - rect.left) / CharWidth(L' '))+1;

                        UINT nFlags = ETO_CLIPPED; // || ETO_OPAQUE;
                        ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect,
                            Spaces, min(nSpaceCount, nCnt), 0);

                    } else if (drawImage) {
                        BlitPictureTo(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
                    } MCHKHEAP
                    if (gbNoDblBuffer) GdiFlush();
                }
            }

            ConCharXLine[j] = (j ? ConCharXLine[j-1] : 0)+CharWidth(c);
            MCHKHEAP


            if (gSet.isForceMonospace)
            {
                MCHKHEAP
                SetBkColor(hDC, gSet.Colors[attrBack]);

                j2 = j + 1;

                if (gSet.isFixFarBorders) {
                    if (!isUnicode)
                        SelectFont(gSet.mh_Font);
                    else if (isUnicode)
                        SelectFont(gSet.mh_Font2);
                }


                RECT rect;
                if (!gSet.isTTF)
                    rect = MakeRect(j * gSet.LogFont.lfWidth, pos, j2 * gSet.LogFont.lfWidth, pos + gSet.LogFont.lfHeight);
                else {
                    rect.left = j ? ConCharXLine[j-1] : 0;
                    rect.top = pos;
                    rect.right = (TextWidth>(UINT)j2) ? ConCharXLine[j2-1] : Width;
                    rect.bottom = rect.top + gSet.LogFont.lfHeight;
                }
                UINT nFlags = ETO_CLIPPED | ((drawImage && (attrBack) < 2) ? 0 : ETO_OPAQUE);
                int nShift = 0;

                MCHKHEAP
                if (c != 0x20 && !isUnicode) {
                    ABC abc;
                    //This function succeeds only with TrueType fonts
                    if (GetCharABCWidths(hDC, c, c, &abc))
                    {
                        
                        if (abc.abcA<0) {
                            // иначе символ наверное налезет на предыдущий?
                            nShift = -abc.abcA;
                        } else if (abc.abcA<(((int)gSet.LogFont.lfWidth-(int)abc.abcB-1)/2)) {
                            // символ I, i, и др. очень тонкие - рисуем посередине
                            nShift = ((gSet.LogFont.lfWidth-abc.abcB)/2)-abc.abcA;
                        }
                    }
                }

                MCHKHEAP
                if (! (drawImage && (attrBack) < 2)) {
                    SetBkColor(hDC, gSet.Colors[attrBack]);
                    //TODO: надо раскомментарить и куда-то приткнуть...
                    if (nShift>0) ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect, L" ", 1, 0);
                } else if (drawImage) {
                    BlitPictureTo(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
                }

                if (nShift>0) {
                    rect.left += nShift; rect.right += nShift;
                }

                if (gbNoDblBuffer) GdiFlush();
                ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect, &c, 1, 0);
                if (gbNoDblBuffer) GdiFlush();
                MCHKHEAP

            }
            else if (gSet.isTTF && c==L' ')
            {
                j2 = j + 1; MCHKHEAP
                if (!doSelect) // doSelect инициализируется только для gConEmu.BufferHeight>0
                {
                    TCHAR ch;
                    while(j2 < end && ConAttrLine[j2] == attr && (ch=ConCharLine[j2]) == L' ')
                    {
                        ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
                        j2++;
                    } MCHKHEAP
                    if (j2>=(int)TextWidth || isCharBorderVertical(ConCharLine[j2])) //ch может быть не инициализирован
                    {
                        ConCharXLine[j2-1] = (j2>=(int)TextWidth) ? Width : (j2) * gSet.LogFont.lfWidth; // или тут [j] должен быть?
                        MCHKHEAP
                        DWORD n1 = ConCharXLine[j];
                        DWORD n3 = j2-j; // кол-во символов
                        DWORD n2 = ConCharXLine[j2-1] - n1; // выделенная на пробелы ширина
                        MCHKHEAP

                        for (int k=j+1; k<(j2-1); k++) {
                            ConCharXLine[k] = n1 + (n3 ? klMulDivU32(k-j, n2, n3) : 0);
                            MCHKHEAP
                        }
                    } MCHKHEAP
                }
                if (gSet.isFixFarBorders)
                    SelectFont(gSet.mh_Font);
            }
            else if (!isUnicode)
            {
                j2 = j + 1; MCHKHEAP
                if (!doSelect) // doSelect инициализируется только для gConEmu.BufferHeight>0
                {
                    #ifndef DRAWEACHCHAR
                    // Если этого не делать - в пропорциональных шрифтах буквы будут наезжать одна на другую
                    TCHAR ch;
                    while(j2 < end && ConAttrLine[j2] == attr && 
                        !isCharBorder(ch = ConCharLine[j2]) 
                        && (!gSet.isTTF || !isFilePanel || (ch != L'}' && ch!=L' '))) // корректировка имен в колонках
                    {
                        ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
                        j2++;
                    }
                    #endif
                }
                if (gSet.isFixFarBorders)
                    SelectFont(gSet.mh_Font);
                MCHKHEAP
            }
            else //Border and specials
            {
                j2 = j + 1; MCHKHEAP
                if (!doSelect) // doSelect инициализируется только для gConEmu.BufferHeight>0
                {
                    if (!gSet.isFixFarBorders)
                    {
                        TCHAR ch;
                        while(j2 < end && ConAttrLine[j2] == attr && isCharBorder(ch = ConCharLine[j2]))
                        {
                            ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
                            j2++;
                        }
                    }
                    else
                    {
                        TCHAR ch;
                        while(j2 < end && ConAttrLine[j2] == attr && 
                            isCharBorder(ch = ConCharLine[j2]) && ch == ConCharLine[j2+1])
                        {
                            ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
                            j2++;
                        }
                    }
                }
                if (gSet.isFixFarBorders)
                    SelectFont(gSet.mh_Font2);
                MCHKHEAP
            }

            if (!gSet.isForceMonospace)
            {
                RECT rect;
                if (!gSet.isTTF)
                    rect = MakeRect(j * gSet.LogFont.lfWidth, pos, j2 * gSet.LogFont.lfWidth, pos + gSet.LogFont.lfHeight);
                else {
                    rect.left = j ? ConCharXLine[j-1] : 0;
                    rect.top = pos;
                    rect.right = (TextWidth>(UINT)j2) ? ConCharXLine[j2-1] : Width;
                    rect.bottom = rect.top + gSet.LogFont.lfHeight;
                }

                MCHKHEAP
                if (! (drawImage && (attrBack) < 2))
                    SetBkColor(hDC, gSet.Colors[attrBack]);
                else if (drawImage)
                    BlitPictureTo(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);

                UINT nFlags = ETO_CLIPPED | ((drawImage && (attrBack) < 2) ? 0 : ETO_OPAQUE);

                MCHKHEAP
                if (gbNoDblBuffer) GdiFlush();
                if (gSet.isTTF && c == ' ') {
                    int nCnt = ((rect.right - rect.left) / CharWidth(L' '))+1;
                    ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect,
                        Spaces, nCnt, 0);
                } else
                if (gSet.LogFont.lfCharSet == OEM_CHARSET && !isUnicode)
                {
                    WideCharToMultiByte(CP_OEMCP, 0, ConCharLine + j, j2 - j, tmpOem, TextWidth+4, 0, 0);
                    ExtTextOutA(hDC, rect.left, rect.top, nFlags,
                        &rect, tmpOem, j2 - j, 0);
                }
                else
                {
                    if ((j2-j)==1) // support visualizer change
                    ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect,
                        &c/*ConCharLine + j*/, 1, 0);
                    else
                    ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect,
                        ConCharLine + j, j2 - j, 0);
                }
                if (gbNoDblBuffer) GdiFlush();
                MCHKHEAP
            }
        
            // stop if all is done
            if (!updateText)
                goto done;

            // skip next not changed symbols again
            if (skipNotChanged)
            {
                MCHKHEAP
                // skip the same except the old cursor
                while(j2 < end && ConCharLine[j2] == ConCharLine2[j2] && ConAttrLine[j2] == ConAttrLine2[j2])
                {
                    if (updateCursor && row == Cursor.y && j2 == Cursor.x)
                        break;
                    if (doSelect)
                    {
                        if (CheckSelection(select1, row, j2))
                            break;
                        if (CheckSelection(select2, row, j2))
                            break;
                    }
                    ++j2;
                }
            }
        }
        if (!updateText)
            break; // только обновление курсора? а нахрена тогда UpdateText вызывать...
    }
done:
    return;
}

void CVirtualConsole::UpdateCursorDraw(COORD pos, BOOL vis, UINT dwSize, LPRECT prcLast/*=NULL*/)
{
    int CurChar = pos.Y * TextWidth + pos.X;
    COORD pix;
    /*if (prcLast) {
        //pix = MakeCoord(prcLast->left, prcLast->top);
		blitSize = MakeCoord(prcLast->right-prcLast->left, prcLast->bottom-prcLast->top);
    } else */
	{
        pix.X = pos.X * gSet.LogFont.lfWidth;
        pix.Y = pos.Y * gSet.LogFont.lfHeight;
        if (pos.X && ConCharX[CurChar-1])
            pix.X = ConCharX[CurChar-1];
    }

    if (vis)
    {
        if (gSet.isCursorColor)
        {
            SetTextColor(hDC, Cursor.foreColor);
            SetBkColor(hDC, Cursor.bgColor);
        }
        else
        {
            SetTextColor(hDC, Cursor.foreColor);
            SetBkColor(hDC, Cursor.foreColorNum < 5 ? gSet.Colors[15] : gSet.Colors[0]);
        }
    }
    else
    {
        if (drawImage && (Cursor.foreColorNum < 2) && prcLast)
			BlitPictureTo(prcLast->left, prcLast->top, prcLast->right-prcLast->left, prcLast->bottom-prcLast->top);

        SetTextColor(hDC, Cursor.bgColor);
        SetBkColor(hDC, Cursor.foreColor);
        dwSize = 99;
    }

    RECT rect;
    
    if (prcLast) {
        rect = *prcLast;
    } else
    if (!gSet.isCursorV)
    {
        if (gSet.isTTF) {
            rect.left = pix.X; /*Cursor.x * gSet.LogFont.lfWidth;*/
            rect.right = pix.X + gSet.LogFont.lfWidth; /*(Cursor.x+1) * gSet.LogFont.lfWidth;*/ //TODO: а ведь позиция следующего символа известна!
        } else {
            rect.left = pos.X * gSet.LogFont.lfWidth;
            rect.right = (pos.X+1) * gSet.LogFont.lfWidth;
        }
        //rect.top = (Cursor.y+1) * gSet.LogFont.lfHeight - MulDiv(gSet.LogFont.lfHeight, cinf.dwSize, 100);
        rect.bottom = (pos.Y+1) * gSet.LogFont.lfHeight;
        rect.top = (pos.Y * gSet.LogFont.lfHeight) /*+ 1*/;
        //if (cinf.dwSize<50)
        int nHeight = 0;
        if (dwSize) {
            nHeight = MulDiv(gSet.LogFont.lfHeight, dwSize, 100);
            if (nHeight < HCURSORHEIGHT) nHeight = HCURSORHEIGHT;
        }
        //if (nHeight < HCURSORHEIGHT) nHeight = HCURSORHEIGHT;
        rect.top = max(rect.top, (rect.bottom-nHeight));
    }
    else
    {
        if (gSet.isTTF) {
            rect.left = pix.X; /*Cursor.x * gSet.LogFont.lfWidth;*/
            //rect.right = rect.left/*Cursor.x * gSet.LogFont.lfWidth*/ //TODO: а ведь позиция следующего символа известна!
            //  + klMax(1, MulDiv(gSet.LogFont.lfWidth, cinf.dwSize, 100) 
            //  + (cinf.dwSize > 10 ? 1 : 0));
        } else {
            rect.left = pos.X * gSet.LogFont.lfWidth;
            //rect.right = Cursor.x * gSet.LogFont.lfWidth
            //  + klMax(1, MulDiv(gSet.LogFont.lfWidth, cinf.dwSize, 100) 
            //  + (cinf.dwSize > 10 ? 1 : 0));
        }
        rect.top = pos.Y * gSet.LogFont.lfHeight;
        int nR = (gSet.isTTF && ConCharX[CurChar]) // правая граница
            ? ConCharX[CurChar] : ((pos.X+1) * gSet.LogFont.lfWidth);
        //if (cinf.dwSize>=50)
        //  rect.right = nR;
        //else
        //  rect.right = min(nR, (rect.left+VCURSORWIDTH));
        int nWidth = 0;
        if (dwSize) {
            nWidth = MulDiv((nR - rect.left), dwSize, 100);
            if (nWidth < VCURSORWIDTH) nWidth = VCURSORWIDTH;
        }
        rect.right = min(nR, (rect.left+nWidth));
        //rect.right = rect.left/*Cursor.x * gSet.LogFont.lfWidth*/ //TODO: а ведь позиция следующего символа известна!
        //      + klMax(1, MulDiv(gSet.LogFont.lfWidth, cinf.dwSize, 100) 
        //      + (cinf.dwSize > 10 ? 1 : 0));
        rect.bottom = (pos.Y+1) * gSet.LogFont.lfHeight;
    }
    
    if (!prcLast) Cursor.lastRect = rect;

    if (gSet.LogFont.lfCharSet == OEM_CHARSET && !isCharBorder(Cursor.ch[0]))
    {
        if (gSet.isFixFarBorders)
            SelectFont(gSet.mh_Font);

        char tmp[2];
        WideCharToMultiByte(CP_OEMCP, 0, Cursor.ch, 1, tmp, 1, 0, 0);
        ExtTextOutA(hDC, pix.X, pix.Y,
            ETO_CLIPPED | ((drawImage && (Cursor.foreColorNum < 2) &&
            !vis) ? 0 : ETO_OPAQUE),&rect, tmp, 1, 0);
    }
    else
    {
        if (gSet.isFixFarBorders && isCharBorder(Cursor.ch[0]))
            SelectFont(gSet.mh_Font2);
        else
            SelectFont(gSet.mh_Font);

        ExtTextOut(hDC, pix.X, pix.Y,
            ETO_CLIPPED | ((drawImage && (Cursor.foreColorNum < 2) &&
            !vis) ? 0 : ETO_OPAQUE),&rect, Cursor.ch, 1, 0);
    }
}

void CVirtualConsole::UpdateCursor(bool& lRes)
{
    //------------------------------------------------------------------------
    ///| Drawing cursor |/////////////////////////////////////////////////////
    //------------------------------------------------------------------------
    Cursor.isVisiblePrevFromInfo = cinf.bVisible;

    BOOL lbUpdateTick = FALSE;
    if ((Cursor.x != csbi.dwCursorPosition.X) || (Cursor.y != csbi.dwCursorPosition.Y)) 
    {
        Cursor.isVisible = isMeForeground();
        if (Cursor.isVisible) lRes = true; //force, pos changed
        lbUpdateTick = TRUE;
    } else
    if ((GetTickCount() - Cursor.nLastBlink) > Cursor.nBlinkTime)
    {
        Cursor.isVisible = isMeForeground() && !Cursor.isVisible;
        lbUpdateTick = TRUE;
    }

    if ((lRes || Cursor.isVisible != Cursor.isVisiblePrev) && cinf.bVisible && isCursorValid)
    {
        lRes = true;

        SelectFont(gSet.mh_Font);

        if ((Cursor.x != csbi.dwCursorPosition.X || Cursor.y != csbi.dwCursorPosition.Y))
        {
            Cursor.isVisible = isMeForeground();
            //gConEmu.cBlinkNext = 0;
        }

        ///++++
        if (Cursor.isVisiblePrev) {
            UpdateCursorDraw(MakeCoord(Cursor.x, Cursor.y), false, Cursor.lastSize, &Cursor.lastRect);
        }

        int CurChar = csbi.dwCursorPosition.Y * TextWidth + csbi.dwCursorPosition.X;
        Cursor.ch[1] = 0;
        GetCharAttr(ConChar[CurChar], ConAttr[CurChar], Cursor.ch[0], Cursor.bgColorNum, Cursor.foreColorNum);
        Cursor.foreColor = gSet.Colors[Cursor.foreColorNum];
        Cursor.bgColor = gSet.Colors[Cursor.bgColorNum];

        UpdateCursorDraw(csbi.dwCursorPosition, Cursor.isVisible, cinf.dwSize);

        Cursor.isVisiblePrev = Cursor.isVisible;
    }

    // update cursor anyway to avoid redundant updates
    Cursor.x = csbi.dwCursorPosition.X;
    Cursor.y = csbi.dwCursorPosition.Y;

    if (lbUpdateTick)
        Cursor.nLastBlink = GetTickCount();
}

void CVirtualConsole::SetConsoleSize(const COORD& size)
{
    if (!ghConWnd) {
        MBoxA(_T("Console was not created (CVirtualConsole::SetConsoleSize)"));
        return; // консоль пока не создана?
    }

    RECT rcConPos; GetWindowRect(ghConWnd, &rcConPos);

    // case: simple mode
    if (gConEmu.BufferHeight == 0)
    {
        HANDLE h = hConOut();
        BOOL lbNeedChange = FALSE;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(h, &csbi)) {
            lbNeedChange = (csbi.dwSize.X != size.X) || (csbi.dwSize.Y != size.Y);
        }
        if (lbNeedChange) {
            MOVEWINDOW(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
            SETCONSOLESCREENBUFFERSIZE(h, size);
        }
        MOVEWINDOW(ghConWnd, rcConPos.left, rcConPos.top, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
        return;
    }

    // global flag of the first call which is:
    // *) after getting all the settings
    // *) before running the command
    static bool s_isFirstCall = true;

    // case: buffer mode: change buffer
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConOut(), &csbi))
        return;
    csbi.dwSize.X = size.X;
    if (s_isFirstCall)
    {
        // first call: buffer height = from settings
        s_isFirstCall = false;
        csbi.dwSize.Y = max(gConEmu.BufferHeight, size.Y);
    }
    else
    {
        if (csbi.dwSize.Y == csbi.srWindow.Bottom - csbi.srWindow.Top + 1)
            // no y-scroll: buffer height = new window height
            csbi.dwSize.Y = size.Y;
        else
            // y-scroll: buffer height = old buffer height
            csbi.dwSize.Y = max(csbi.dwSize.Y, size.Y);
    }
    MOVEWINDOW(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
    SETCONSOLESCREENBUFFERSIZE(hConOut(), csbi.dwSize);
    //окошко раздвигаем только по ширине!
    GetWindowRect(ghConWnd, &rcConPos);
    MOVEWINDOW(ghConWnd, rcConPos.left, rcConPos.top, GetSystemMetrics(SM_CXSCREEN), rcConPos.bottom-rcConPos.top, 1);

    
    // set console window
    if (!GetConsoleScreenBufferInfo(hConOut(), &csbi))
        return;
    SMALL_RECT rect;
    rect.Top = csbi.srWindow.Top;
    rect.Left = csbi.srWindow.Left;
    rect.Right = rect.Left + size.X - 1;
    rect.Bottom = rect.Top + size.Y - 1;
    if (rect.Right >= csbi.dwSize.X)
    {
        int shift = csbi.dwSize.X - 1 - rect.Right;
        rect.Left += shift;
        rect.Right += shift;
    }
    if (rect.Bottom >= csbi.dwSize.Y)
    {
        int shift = csbi.dwSize.Y - 1 - rect.Bottom;
        rect.Top += shift;
        rect.Bottom += shift;
    }
    SetConsoleWindowInfo(hConOut(), TRUE, &rect);
}

BOOL CVirtualConsole::AttachPID(DWORD dwPID)
{
    #ifdef _DEBUG
        TCHAR szMsg[100]; wsprintf(szMsg, _T("Attach to process %i"), (int)dwPID);
        OutputDebugString(szMsg);
    #endif
    BOOL lbRc = AttachConsole(dwPID);
    if (!lbRc) {
        OutputDebugString(_T(" - failed\n"));
        BOOL lbFailed = TRUE;
        DWORD dwErr = GetLastError();
        if (dwErr == 0x1F && dwPID == -1)
        {
            // Если ConEmu запускается из FAR'а батником - то родительский процесс - CMD.EXE, а он уже скорее всего закрыт. то есть подцепиться не удастся
            HWND hConsole = FindWindowEx(NULL,NULL,_T("ConsoleWindowClass"),NULL);
            if (hConsole && IsWindowVisible(hConsole)) {
                DWORD dwCurPID = 0;
                if (GetWindowThreadProcessId(hConsole,  &dwCurPID)) {
                    if (AttachConsole(dwCurPID))
                        lbFailed = FALSE;
                }
            }
        }
        if (lbFailed) {
            TCHAR szErr[255];
            wsprintf(szErr, _T("AttachConsole failed (PID=%i)!"), dwPID);
            DisplayLastError(szErr, dwErr);
            return FALSE;
        }
    }
    OutputDebugString(_T(" - OK"));

    hConWnd = GetConsoleWindow();
    ghConWnd = hConWnd;
    
    InitHandlers();

    // Попытаться дернуть плагин для установки шрифта.
    CConEmuPipe pipe;
    
    OutputDebugString(_T("CheckProcesses\n"));
    gConEmu.CheckProcesses(0,TRUE);
    
    if (pipe.Init(_T("DefFont.in.attach"), TRUE))
        pipe.Execute(CMD_DEFFONT);

    return TRUE;
}

void CVirtualConsole::InitHandlers()
{
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CConEmuMain::HandlerRoutine, true);
    
    SetHandleInformation(GetStdHandle(STD_INPUT_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(GetStdHandle(STD_OUTPUT_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(GetStdHandle(STD_ERROR_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

	DWORD mode = 0;
    BOOL lb = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
	if (!(mode & ENABLE_MOUSE_INPUT)) {
	    mode |= ENABLE_MOUSE_INPUT;
		lb = SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
	}
    
    ghConWnd = GetConsoleWindow();
    gConEmu.ConsoleCreated(ghConWnd);
}

// asExeName может быть NULL, тогда ставим полный путь к ConEmu (F:_VCProject_FarPlugin_#FAR180_ConEmu.exe)
// а может быть как просто именем "FAR", так и с расширением "FAR.EXE" (зависит от командной строки)
void CVirtualConsole::RegistryProps(BOOL abRollback, ConExeProps& props, LPCTSTR asExeName/*=NULL*/)
{
    HKEY hkey = NULL;
    DWORD dwDisp = 0;
    TCHAR *pszExeName = NULL;
    
    if (!abRollback) {
        memset(&props, 0, sizeof(props));

        if (gSet.ourSI.lpTitle && *gSet.ourSI.lpTitle) {
            props.FullKeyName = (TCHAR*)calloc(_tcslen(gSet.ourSI.lpTitle)+10, sizeof(TCHAR));
            _tcscpy(props.FullKeyName, _T("Console\\"));
            pszExeName = props.FullKeyName+_tcslen(props.FullKeyName);
            _tcscpy(pszExeName, gSet.ourSI.lpTitle);
        } else
        if (asExeName && *asExeName) {
            props.FullKeyName = (TCHAR*)calloc(_tcslen(asExeName)+10, sizeof(TCHAR));
            _tcscpy(props.FullKeyName, _T("Console\\"));
            pszExeName = props.FullKeyName+_tcslen(props.FullKeyName);
            _tcscpy(pszExeName, asExeName);
        } else {
            props.FullKeyName = (TCHAR*)calloc(MAX_PATH+10, sizeof(TCHAR));
            _tcscpy(props.FullKeyName, _T("Console\\"));
            pszExeName = props.FullKeyName+_tcslen(props.FullKeyName);
            if (!GetModuleFileName(NULL, pszExeName, MAX_PATH+1)) {
                DisplayLastError(_T("Can't get module file name"));
                if (props.FullKeyName) { free(props.FullKeyName); props.FullKeyName = NULL; }
                return;
            }
        }
        
        for (TCHAR* psz=pszExeName; *psz; psz++) {
            if (*psz == _T('\\')) *psz = _T('_');
        }
    } else if (!props.FullKeyName) {
        return;
    }
    
    
    if (abRollback && !props.bKeyExists) {
        // Просто удалить подключ pszExeName
        if (0 == RegOpenKeyEx ( HKEY_CURRENT_USER, _T("Console"), NULL, DELETE , &hkey)) {
            RegDeleteKey(hkey, pszExeName);
            RegCloseKey(hkey);
        }
        if (props.FullKeyName) { free(props.FullKeyName); props.FullKeyName = NULL;}
        if (props.FaceName) { free(props.FaceName); props.FaceName = NULL;}
        return;
    }
    
    if (0 == RegCreateKeyEx ( HKEY_CURRENT_USER, props.FullKeyName, NULL, NULL, NULL, KEY_ALL_ACCESS, NULL, &hkey, &dwDisp)) {
        if (!abRollback) {
            props.bKeyExists = (dwDisp == REG_OPENED_EXISTING_KEY);
            // Считать значения
            DWORD dwSize, dwVal;
            if (0!=RegQueryValueEx(hkey, _T("ScreenBufferSize"), 0, NULL, (LPBYTE)&props.ScreenBufferSize, &(dwSize=sizeof(DWORD))))
                props.ScreenBufferSize = -1;
            if (0!=RegQueryValueEx(hkey, _T("WindowSize"), 0, NULL, (LPBYTE)&props.WindowSize, &(dwSize=sizeof(DWORD))))
                props.WindowSize = -1;
            if (0!=RegQueryValueEx(hkey, _T("WindowPosition"), 0, NULL, (LPBYTE)&props.WindowPosition, &(dwSize=sizeof(DWORD))))
                props.WindowPosition = -1;
            if (0!=RegQueryValueEx(hkey, _T("FontSize"), 0, NULL, (LPBYTE)&props.FontSize, &(dwSize=sizeof(DWORD))))
                props.FontSize = -1;
            if (0!=RegQueryValueEx(hkey, _T("FontFamily"), 0, NULL, (LPBYTE)&props.FontFamily, &(dwSize=sizeof(DWORD))))
                props.FontFamily = -1;
            props.FaceName = (TCHAR*)calloc(MAX_PATH+1,sizeof(TCHAR));
            if (0!=RegQueryValueEx(hkey, _T("FaceName"), 0, NULL, (LPBYTE)props.FaceName, &(dwSize=(sizeof(TCHAR)*(MAX_PATH+1)))))
                props.FaceName[0] = 0;
            
            // Установить требуемые умолчания
            dwVal = (gSet.wndWidth) | (gSet.wndHeight<<16);
            RegSetValueEx(hkey, _T("ScreenBufferSize"), 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(DWORD));
            RegSetValueEx(hkey, _T("WindowSize"), 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(DWORD));
            if (!ghWndDC) dwVal = 0; else {
                RECT rcWnd; GetWindowRect(ghWndDC, &rcWnd);
                rcWnd.left = max(0, (rcWnd.left & 0xFFFF));
                rcWnd.top = max(0, (rcWnd.top & 0xFFFF));
                dwVal = (rcWnd.left) | (rcWnd.top<<16);
            }
            RegSetValueEx(hkey, _T("WindowPosition"), 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(DWORD));
            dwVal = 0x00060000;
            RegSetValueEx(hkey, _T("FontSize"), 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(DWORD));
            dwVal = 0x00000036;
            RegSetValueEx(hkey, _T("FontFamily"), 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(DWORD));
            TCHAR szLucida[64]; _tcscpy(szLucida, _T("Lucida Console"));
            RegSetValueEx(hkey, _T("FaceName"), 0, REG_SZ, (LPBYTE)szLucida, sizeof(TCHAR)*(_tcslen(szLucida)+1));
        } else {
            // Вернуть значения
            if (props.ScreenBufferSize == -1)
                RegDeleteValue(hkey, _T("ScreenBufferSize"));
            else
                RegSetValueEx(hkey, _T("ScreenBufferSize"), 0, REG_DWORD, (LPBYTE)&props.ScreenBufferSize, sizeof(DWORD));
            if (props.WindowSize == -1)
                RegDeleteValue(hkey, _T("WindowSize"));
            else
                RegSetValueEx(hkey, _T("WindowSize"), 0, REG_DWORD, (LPBYTE)&props.WindowSize, sizeof(DWORD));
            if (props.WindowPosition == -1)
                RegDeleteValue(hkey, _T("WindowPosition"));
            else
                RegSetValueEx(hkey, _T("WindowPosition"), 0, REG_DWORD, (LPBYTE)&props.WindowPosition, sizeof(DWORD));
            if (props.FontSize == -1)
                RegDeleteValue(hkey, _T("FontSize"));
            else
                RegSetValueEx(hkey, _T("FontSize"), 0, REG_DWORD, (LPBYTE)&props.FontSize, sizeof(DWORD));
            if (props.FontFamily == -1)
                RegDeleteValue(hkey, _T("FontFamily"));
            else
                RegSetValueEx(hkey, _T("FontFamily"), 0, REG_DWORD, (LPBYTE)&props.FontFamily, sizeof(DWORD));
            if (props.FaceName && *props.FaceName)
                RegSetValueEx(hkey, _T("FaceName"), 0, REG_SZ, (LPBYTE)props.FaceName, sizeof(TCHAR)*(_tcslen(props.FaceName)+1));
            else
                RegDeleteValue(hkey, _T("FaceName"));
            
            // и освободить буфера
            if (props.FullKeyName) { free(props.FullKeyName); props.FullKeyName = NULL;}
            if (props.FaceName) { free(props.FaceName); props.FaceName = NULL;}
        }
        RegCloseKey(hkey);
    }
}

extern void SetConsoleFontSizeTo(HWND inConWnd, int inSizeX, int inSizeY);
BOOL CVirtualConsole::StartProcess()
{
    BOOL lbRc = FALSE;

    if (ghConWnd) {
        // Сначала нужно отцепиться от текущей консоли
        FreeConsole(); ghConWnd = NULL;
    }
    
    #ifdef _DEBUG
    //MBoxA(gSet.ourSI.lpTitle ? gSet.ourSI.lpTitle : L"Title was not specified");
    #endif
    
    ConExeProps props;
    
	// Если запускались с ярлыка - это нихрена не поможет... информация похоже в .lnk сохраняется...
    RegistryProps(FALSE, props);

    if (!gConEmu.isShowConsole && !gSet.isConVisible)
      SetWindowPos(ghWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
    AllocConsole();
    ghConWnd = GetConsoleWindow();
	SetConsoleFontSizeTo(ghConWnd, 4, 6);
    if (!gConEmu.isShowConsole && !gSet.isConVisible)
      SetWindowPos(ghWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
    SetConsoleTitle(gSet.GetCmd());
    
    RegistryProps(TRUE, props);
    
    InitHandlers();
    
    if (gSet.isConMan) {
        if (!gConEmu.InitConMan(gSet.GetCmd())) {
            //gConEmu.Destroy();
            //free(cmdLine);
            //return -1;
            // Иначе жестоко получается. ConEmu вообще будет сложно запустить...
            gSet.isConMan = false;
        } else {
            return TRUE;
        }
    } 
    
    if (!gSet.isConMan)
    {
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory( &si, sizeof(si) );
        si.cb = sizeof(si);
        ZeroMemory( &pi, sizeof(pi) );

        int nStep = 1;
        while (nStep <= 2)
        {
            /*if (!*gSet.GetCmd()) {
                gSet.psCurCmd = _tcsdup(gSet.BufferHeight == 0 ? _T("far") : _T("cmd"));
                nStep ++;
            }*/

            LPTSTR lpszCmd = (LPTSTR)gSet.GetCmd();
            #ifdef _DEBUG
            OutputDebugString(lpszCmd);OutputDebugString(_T("\n"));
            #endif
            try {
                lbRc = CreateProcess(NULL, lpszCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
                OutputDebugString(_T("CreateProcess finished\n"));
            } catch(...) {
                lbRc = FALSE;
            }
            if (lbRc)
            {
                OutputDebugString(_T("CreateProcess OK\n"));
                lbRc = TRUE;

                /*if (!AttachPID(pi.dwProcessId)) {
                    OutputDebugString(_T("AttachPID failed\n"));
                    return FALSE;
                }
                OutputDebugString(_T("AttachPID OK\n"));*/

                break; // OK, запустили
            } else {
                //MBoxA("Cannot execute the command.");
                DWORD dwLastError = GetLastError();
                OutputDebugString(_T("CreateProcess failed\n"));
                int nLen = _tcslen(gSet.GetCmd());
                TCHAR* pszErr=(TCHAR*)calloc(nLen+100,sizeof(TCHAR));
                
                if (0==FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                    pszErr, 1024, NULL))
                {
                    wsprintf(pszErr, _T("Unknown system error: 0x%x"), dwLastError);
                }
                
                nLen += _tcslen(pszErr);
                TCHAR* psz=(TCHAR*)calloc(nLen+100,sizeof(TCHAR));
                int nButtons = MB_OK|MB_ICONEXCLAMATION|MB_SETFOREGROUND;
                
                _tcscpy(psz, _T("Cannot execute the command.\r\n"));
                _tcscat(psz, pszErr);
                if (psz[_tcslen(psz)-1]!=_T('\n')) _tcscat(psz, _T("\r\n"));
                _tcscat(psz, gSet.GetCmd());
                if (!gSet.psCurCmd && StrStrI(gSet.GetCmd(), gSet.BufferHeight == 0 ? _T("far.exe") : _T("cmd.exe"))==NULL) {
                    _tcscat(psz, _T("\r\n\r\n"));
                    _tcscat(psz, gSet.BufferHeight == 0 ? _T("Do You want to simply start far?") : _T("Do You want to simply start cmd?"));
                    nButtons |= MB_YESNO;
                }
                //MBoxA(psz);
                int nBrc = MessageBox(NULL, psz, _T("ConEmu"), nButtons);
                free(psz); free(pszErr);
                if (nBrc!=IDYES) {
                    gConEmu.Destroy();
                    //free(cmdLine);
                    return FALSE;
                }
                // Выполнить стандартную команду...
                gSet.psCurCmd = _tcsdup(gSet.BufferHeight == 0 ? _T("far") : _T("cmd"));
                nStep ++;
            }
        }

        //TODO: а делать ли это?
        CloseHandle(pi.hThread); pi.hThread = NULL;
        CloseHandle(pi.hProcess); pi.hProcess = NULL;
    }
    
    return lbRc;
}

bool CVirtualConsole::CheckBufferSize()
{
    bool lbForceUpdate = false;

    if (!this)
        return false;
    
    CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
    GetConsoleScreenBufferInfo(hConOut(), &inf);
    if (inf.dwSize.X>(inf.srWindow.Right-inf.srWindow.Left+1)) {
        DEBUGLOGFILE("Wrong screen buffer width\n");
        // Окошко консоли почему-то схлопнулось по горизонтали
        MOVEWINDOW(ghConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
    } else {
        // может меняться и из плагина, во время работы фара...
        /*if (mn_ActiveStatus & CES_FARACTIVE) {
            if (BufferHeight) {
                BufferHeight = 0; // сброс на время активности фара
                lbForceUpdate = true;
            }
        } else*/
        if ( (inf.dwSize.Y<(inf.srWindow.Bottom-inf.srWindow.Top+10)) && gConEmu.BufferHeight &&
             !gSet.BufferHeight /*&& (gConEmu.BufferHeight != inf.dwSize.Y)*/)
        {
            // может быть консольная программа увеличила буфер самостоятельно?
            // TODO: отключить прокрутку!!!
            gConEmu.BufferHeight = 0;

            SCROLLINFO si;
            ZeroMemory(&si, sizeof(si));
            si.cbSize = sizeof(si);
            si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
            if (GetScrollInfo(ghConWnd, SB_VERT, &si))
                SetScrollInfo(gConEmu.m_Back.mh_Wnd, SB_VERT, &si, true);

            lbForceUpdate = true;
        } else 
        if ( (inf.dwSize.Y>(inf.srWindow.Bottom-inf.srWindow.Top+10)) ||
             (gConEmu.BufferHeight && (gConEmu.BufferHeight != inf.dwSize.Y)) )
        {
            // может быть консольная программа увеличила буфер самостоятельно?
            if (gConEmu.BufferHeight != inf.dwSize.Y) {
                // TODO: Включить прокрутку!!!
                gConEmu.BufferHeight = inf.dwSize.Y;
                lbForceUpdate = true;
            }
        }
        
        if ((gConEmu.BufferHeight == 0) && (inf.dwSize.Y>(inf.srWindow.Bottom-inf.srWindow.Top+1))) {
            #pragma message("TODO: это может быть консольная программа увеличила буфер самостоятельно!")
            DEBUGLOGFILE("Wrong screen buffer height\n");
            // Окошко консоли почему-то схлопнулось по вертикали
            MOVEWINDOW(ghConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
        }
    }

    // При выходе из FAR -> CMD с gConEmu.BufferHeight - смена QuickEdit режима
    DWORD mode = 0;
    BOOL lb = FALSE;
    if (gConEmu.BufferHeight) {
        //TODO: похоже, что для gConEmu.BufferHeight это вызывается постоянно?
        lb = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);

        if (inf.dwSize.Y>(inf.srWindow.Bottom-inf.srWindow.Top+1)) {
            // Буфер больше высоты окна
            mode |= ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE|ENABLE_EXTENDED_FLAGS;
        } else {
            // Буфер равен высоте окна (значит ФАР запустился)
            mode &= ~(ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE);
            mode |= ENABLE_EXTENDED_FLAGS;
        }

        lb = SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
    }
    
    return lbForceUpdate;
}
