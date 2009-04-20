#include "Header.h"

#define VCURSORWIDTH 2
#define HCURSORHEIGHT 2

#define Assert(V) if ((V)==FALSE) { TCHAR szAMsg[MAX_PATH*2]; wsprintf(szAMsg, _T("Assertion (%s) at\n%s:%i"), _T(#V), _T(__FILE__), __LINE__); Box(szAMsg); }

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
        /*if (!pCon->StartProcess()) {
            delete pCon;
            return NULL;
        }*/
        // Вроде бы с запуском через нить клики мышкой в консоль начинают ходить сразу, но
        // часто окно конэму вообще не активируется
        pCon->mh_Thread = CreateThread(NULL, 0, StartProcessThread, (LPVOID)pCon, 0, &pCon->mn_ThreadID);
        if (pCon->mh_Thread == NULL) {
            DisplayLastError(_T("Can't create console thread!"));
            delete pCon;
            return NULL;
        }
    }

    /*if (gSet.wndHeight && gSet.wndWidth)
    {
        COORD b = {gSet.wndWidth, gSet.wndHeight};
        pCon->SetConsoleSize(b);
    }*/

    return pCon;
}

CVirtualConsole::CVirtualConsole(/*HANDLE hConsoleOutput*/)
{
	memset(Title,0,sizeof(Title)); memset(TitleCmp,0,sizeof(TitleCmp));
    SIZE_T nMinHeapSize = (1000 + (200 * 90 * 2) * 6 + MAX_PATH*2)*2 + 210*sizeof(*TextParts);
    mh_Heap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, nMinHeapSize, 0);
    mh_Thread = NULL; mn_ThreadID = 0;
    mh_TermEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    mh_ForceReadEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    mh_EndUpdateEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    //mh_ReqSetSize = CreateEvent(NULL,FALSE,FALSE,NULL);
	//mh_ReqSetSizeEnd = CreateEvent(NULL,FALSE,FALSE,NULL);
    //m_ReqSetSize = MakeCoord ( 0,0 );

    //InitializeCriticalSection(&csDC); ncsTDC = 0;
    InitializeCriticalSection(&csCON); ncsTCON = 0;
        
    //pVCon = this;
    //mh_ConOut = NULL; mb_ConHandleCreated = FALSE;
	mh_StdIn = NULL; mh_StdOut = NULL;
    
    mr_LeftPanel = mr_RightPanel = MakeRect(-1,-1);

#ifdef _DEBUG
    mn_BackColorIdx = 2;
#else
    mn_BackColorIdx = 0;
#endif
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
    hConWnd = NULL;

    nSpaceCount = 1000;
    Spaces = (TCHAR*)Alloc(nSpaceCount,sizeof(TCHAR));
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
    hDC = NULL;
    hBitmap = NULL;
    ConChar = NULL;
    ConAttr = NULL;
    ConCharX = NULL;
    tmpOem = NULL;
    TextParts = NULL;
}

CVirtualConsole::~CVirtualConsole()
{
    StopThread();
    
    MCHKHEAP
    if (hDC)
        { DeleteDC(hDC); hDC = NULL; }
    if (hBitmap)
        { DeleteObject(hBitmap); hBitmap = NULL; }
    if (ConChar)
        { Free(ConChar); ConChar = NULL; }
    if (ConAttr)
        { Free(ConAttr); ConAttr = NULL; }
    if (ConCharX)
        { Free(ConCharX); ConCharX = NULL; }
    if (tmpOem)
        { Free(tmpOem); tmpOem = NULL; }
    if (TextParts)
        { Free(TextParts); TextParts = NULL; }
    if (Spaces) 
        { Free(Spaces); Spaces = NULL; nSpaceCount = 0; }

    // Куча больше не нужна
    if (mh_Heap) {
        HeapDestroy(mh_Heap);
        mh_Heap = NULL;
    }

    //if (mb_ConHandleCreated && mh_ConOut && mh_ConOut!=INVALID_HANDLE_VALUE)
    //    CloseHandle(mh_ConOut);
    //mh_ConOut = NULL; mb_ConHandleCreated = FALSE;
	SafeCloseHandle(mh_StdIn); 
	SafeCloseHandle(mh_StdOut);

	//SafeCloseHandle(mh_ReqSetSize);
	//SafeCloseHandle(mh_ReqSetSizeEnd);
    
    //DeleteCriticalSection(&csDC);
    DeleteCriticalSection(&csCON);
}

HANDLE CVirtualConsole::hConIn()
{
	if (!this)
		return GetStdHandle(STD_INPUT_HANDLE);
	return mh_StdIn;
}

// Дабы избежать многоратных Recreate во время обновления - пересоздаем хэндл только в начале Update!
HANDLE CVirtualConsole::hConOut(BOOL abAllowRecreate/*=FALSE*/)
{
	// TODO: поддержка telnet'a (переоткрывать хэндл?)
	if (!this)
		return NULL; //GetStdHandle(STD_OUTPUT_HANDLE);
	if (ghConWnd != hConWnd)
		return NULL;
	return mh_StdOut;
	/*
    if(mh_ConOut && mh_ConOut!=INVALID_HANDLE_VALUE && !abAllowRecreate) {
        return mh_ConOut;
    }
    
    if (gSet.isUpdConHandle)
    {
        if(mh_ConOut && mh_ConOut!=INVALID_HANDLE_VALUE && mb_ConHandleCreated)
            CloseHandle(mh_ConOut);
        mh_ConOut = NULL; mb_ConHandleCreated = FALSE;

        mh_ConOut = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (mh_ConOut == INVALID_HANDLE_VALUE) {
            // Наверное лучше вернуть текущий хэндл, нежели вообще завалиться...
            mh_ConOut = hConOut();
            #ifdef _DEBUG
            __asm int 3;
            #endif
        } else {
            mb_ConHandleCreated = TRUE; // OK
        }
    } else if (mh_ConOut==NULL) {
        mh_ConOut = hConOut();
        mb_ConHandleCreated = FALSE;
    }
    
    if (mh_ConOut == INVALID_HANDLE_VALUE) {
        mh_ConOut = NULL;
        DisplayLastError(_T("CVirtualConsole::hConOut fails"));
    }
    
    return mh_ConOut;
	*/
}

// InitDC вызывается только при критических изменениях (размеры, шрифт, и т.п.) когда нужно пересоздать DC и Bitmap
bool CVirtualConsole::InitDC(bool abNoDc, bool abNoWndResize)
{
    CSection SCON(&csCON, &ncsTCON);
    //CSection SDC(&csDC, &ncsTDC);
    
    if (hDC)
        { DeleteDC(hDC); hDC = NULL; }
    if (hBitmap)
        { DeleteObject(hBitmap); hBitmap = NULL; }
    if (ConChar)
        { Free(ConChar); ConChar = NULL; }
    if (ConAttr)
        { Free(ConAttr); ConAttr = NULL; }
    if (ConCharX)
        { Free(ConCharX); ConCharX = NULL; }
    if (tmpOem)
        { Free(tmpOem); tmpOem = NULL; }
    if (TextParts)
        { Free(TextParts); TextParts = NULL; }

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConOut(), &csbi))
        return false;

    IsForceUpdate = true;
    TextWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    TextHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

#ifdef _DEBUG
	if (TextHeight < 5)
		__asm int 3;
#endif

    if (!TextWidth || !TextHeight) {
        Assert(TextWidth && TextHeight);
        return false;
    }

    if ((int)TextWidth < csbi.dwSize.X)
        TextWidth = csbi.dwSize.X;

    MCHKHEAP
    ConChar = (TCHAR*)Alloc((TextWidth * TextHeight * 2), sizeof(*ConChar));
    ConAttr = (WORD*)Alloc((TextWidth * TextHeight * 2), sizeof(*ConAttr));
    ConCharX = (DWORD*)Alloc((TextWidth * TextHeight), sizeof(*ConCharX));
    tmpOem = (char*)Alloc((TextWidth + 5), sizeof(*tmpOem));
    TextParts = (struct _TextParts*)Alloc((TextWidth + 2), sizeof(*TextParts));
    MCHKHEAP
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

            Assert ( gSet.LogFont.lfWidth && gSet.LogFont.lfHeight );

            BOOL lbWasInitialized = TextWidth && TextHeight;
            // Посчитать новый размер в пикселях
            Width = TextWidth * gSet.LogFont.lfWidth;
            Height = TextHeight * gSet.LogFont.lfHeight;

            if (ghOpWnd)
                gConEmu.UpdateSizes();

            //if (!lbWasInitialized) // если зовется InitDC значит размер консоли изменился
			if (!abNoWndResize) {
				if (gConEmu.isActive(this))
					gConEmu.OnSize(-1);
			}

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

#ifdef MSGLOGGER
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
#ifdef MSGLOGGER
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

    textChanged = 0!=memcmp(ConChar + TextLen, ConChar, TextLen * sizeof(*ConChar));
    attrChanged = 0!=memcmp(ConAttr + TextLen, ConAttr, TextLen * sizeof(*ConAttr));

    return textChanged || attrChanged;
}

bool CVirtualConsole::Update(bool isForce, HDC *ahDc)
{
    #pragma message("TODO: CVirtualConsole::Update - может только если hConWnd==ghConWnd?")
    if (!hConWnd)
        return false;

	if (!ahDc && GetCurrentThreadId() != mn_ThreadID) {
		SetEvent(mh_ForceReadEvent);
		WaitForSingleObject(mh_EndUpdateEvent, 1000);
		gConEmu.InvalidateAll();
		return false;
	}

    #ifdef MSGLOGGER
    DcDebug dbg(&hDC, ahDc); // для отладки - рисуем сразу на канвасе окна
    #endif

    // переоткрыть хэндл Output'а при необходимости
    if (!hConOut(TRUE)) {
        return false;
    }

    MCHKHEAP
    bool lRes = false;
    
    CSection SCON(&csCON, &ncsTCON);
    //CSection SDC(&csDC, &ncsTDC);

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
    
    MCHKHEAP
    
    //SDC.Leave();
    SCON.Leave();

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
    CSection S(&csCON, &ncsTCON);
    
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
        if (!InitDC(ahDc!=NULL, false))
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

    MCHKHEAP
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
    
    MCHKHEAP
    
    // Определить координаты панелей
    mr_LeftPanel = mr_RightPanel = MakeRect(-1,-1);
    if (gConEmu.isFar() && TextHeight >= 7 && TextWidth >= 28)
    {
		uint nY = 0;
		if (ConChar[0] == L' ')
			nY ++; // скорее всего, первая строка - меню
		else if (ConChar[0] == L'R' && ConChar[1] == L' ')
			nY ++; // скорее всего, первая строка - меню, при включенной записи макроса
			
		uint nIdx = nY*TextWidth;
		
		if (( (ConChar[nIdx] == L'[' && (ConChar[nIdx+1]>=L'0' && ConChar[nIdx+1]<=L'9')) // открыто несколько редакторов/вьюверов
		      || (ConChar[nIdx] == 0x2554 && ConChar[nIdx+1] == 0x2550) // доп.окон нет, только рамка
		    ) && ConChar[nIdx+TextWidth] == 0x2551)
		{
			LPCWSTR pszCenter = ConChar + nIdx;
			LPCWSTR pszLine = ConChar + nIdx;
			uint nCenter = 0;
			while ( (pszCenter = wcsstr(pszCenter+1, L"\x2557\x2554")) != NULL ) {
				nCenter = pszCenter - pszLine;
				if (ConChar[nIdx+TextWidth+nCenter] == 0x2551 && ConChar[nIdx+TextWidth+nCenter+1] == 0x2551) {
					break; // нашли
				}
			}
			
			uint nBottom = TextHeight - 1;
			while (nBottom > 4) {
				if (ConChar[TextWidth*nBottom] == 0x255A && ConChar[TextWidth*(nBottom-1)] == 0x2551)
					break;
				nBottom --;
			}
			
			if (pszCenter && nBottom > 4) {
				mr_LeftPanel.left = 1;
				mr_LeftPanel.top = nY + 2;
				mr_LeftPanel.right = nCenter - 1;
				mr_LeftPanel.bottom = nBottom - 3;
				
				mr_RightPanel.left = nCenter + 3;
				mr_RightPanel.top = nY + 2;
				mr_RightPanel.right = TextWidth - 2;
				mr_RightPanel.bottom = mr_LeftPanel.bottom;
			}
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

    MCHKHEAP
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
    if (CurChar < 0 || CurChar>=(int)(TextWidth * TextHeight))
        return; // может быть или глюк - или размер консоли был резко уменьшен и предыдущая позиция курсора пропала с экрана
    if (!ConCharX)
        return; // защита
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
        Cursor.isVisible = gConEmu.isMeForeground();
        if (Cursor.isVisible) lRes = true; //force, pos changed
        lbUpdateTick = TRUE;
    } else
    if ((GetTickCount() - Cursor.nLastBlink) > Cursor.nBlinkTime)
    {
        Cursor.isVisible = gConEmu.isMeForeground() && !Cursor.isVisible;
        lbUpdateTick = TRUE;
    }

    if ((lRes || Cursor.isVisible != Cursor.isVisiblePrev) && cinf.bVisible && isCursorValid)
    {
        lRes = true;

        SelectFont(gSet.mh_Font);

        if ((Cursor.x != csbi.dwCursorPosition.X || Cursor.y != csbi.dwCursorPosition.Y))
        {
            Cursor.isVisible = gConEmu.isMeForeground();
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

void CVirtualConsole::SetConsoleSizeInt(COORD size)
{
    const COLORREF DefaultColors[16] = 
    {
        0x00000000, 0x00800000, 0x00008000, 0x00808000,
        0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0, 
        0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00,
        0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
    };

    CONSOLE_INFO ci = { sizeof(ci) };
    int i;

    // get current size/position settings rather than using defaults..
    GetConsoleSizeInfo(&ci);
    ci.ScreenBufferSize = size;
    ci.WindowSize = size;

    // set these to zero to keep current settings
    ci.FontSize.X               = 4;
    ci.FontSize.Y               = 6;
    ci.FontFamily               = 0;//0x30;//FF_MODERN|FIXED_PITCH;//0x30;
    ci.FontWeight               = 0;//0x400;
    _tcscpy(ci.FaceName, _T("Lucida Console"));

    ci.CursorSize               = 25;
    ci.FullScreen               = FALSE;
    ci.QuickEdit                = FALSE;
    ci.AutoPosition             = 0x10000;
    ci.InsertMode               = TRUE;
    ci.ScreenColors             = MAKEWORD(0x7, 0x0);
    ci.PopupColors              = MAKEWORD(0x5, 0xf);

    ci.HistoryNoDup             = FALSE;
    ci.HistoryBufferSize        = 50;
    ci.NumberOfHistoryBuffers   = 4;

    // color table
    for(i = 0; i < 16; i++)
        ci.ColorTable[i] = DefaultColors[i];

    ci.CodePage                 = GetConsoleOutputCP();//0;//0x352;
    ci.Hwnd                     = hConWnd;

    *ci.ConsoleTitle = NULL;

    SetConsoleInfo(&ci);
}

bool CVirtualConsole::SetConsoleSize(COORD size)
{
    if (!hConWnd) {
        Box(_T("Console was not created (CVirtualConsole::SetConsoleSize)"));
        return false; // консоль пока не создана?
    }

    if (size.X<4) size.X = 4;
    if (size.Y<3) size.Y = 3;
    
    // Для ускорения обработки - сравниваем с текущим известным размером
    if (TextWidth == size.X && TextHeight == size.Y)
	    return TRUE;
    
    /*if (GetCurrentThreadId() != mn_ThreadID) {
	    m_ReqSetSize = size;
	    SetEvent ( mh_ReqSetSize );
		#ifdef _DEBUG
		if (WaitForSingleObject(mh_ReqSetSizeEnd, 2000)!=WAIT_OBJECT_0) {
			__asm int 3;
		}
		#endif
	    if (WaitForSingleObject(mh_ReqSetSizeEnd, 10000)!=WAIT_OBJECT_0) {
		    // Если не дождались окончания измения размеров - считаем что не получилось
		    ResetEvent ( mh_ReqSetSize );
		    m_ReqSetSize = MakeCoord(TextWidth,TextHeight);
		    return FALSE;
		}
		// Дождались окончания, но результат?
		return (TextWidth == size.X && TextHeight == size.Y);
	}*/

    CSection SCON(&csCON, &ncsTCON);

    RECT rcConPos; GetWindowRect(hConWnd, &rcConPos);

    // case: simple mode
    if (gConEmu.BufferHeight == 0)
    {
        //HANDLE hConsoleOut = hConOut();
        bool lbRc = true;
        HANDLE h = hConOut();
        BOOL lbNeedChange = FALSE;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(h, &csbi)) {
            lbNeedChange = (csbi.dwSize.X != size.X) || (csbi.dwSize.Y != size.Y);
        }
        if (lbNeedChange) {
            BOOL lbWS = FALSE; DWORD dwErr = 0;
			// Если этого не сделать - размер консоли нельзя УМЕНЬШИТЬ
            MOVEWINDOW(hConWnd, rcConPos.left, rcConPos.top, 1, 1, 1); // попробуем не дергаться?
			//specified width and height cannot be less than the width and height of the console screen buffer's window
            SETCONSOLESCREENBUFFERSIZERET(h, size, lbWS);
			dwErr = GetLastError();
            GetConsoleScreenBufferInfo(h, &csbi);
            if (csbi.dwSize.X != size.X || csbi.dwSize.Y != size.Y) {
                dwErr = GetLastError();
                
                lbRc = false;

                //SETCONSOLESCREENBUFFERSIZERET(hConsoleOut, size, lbWS);
                //GetConsoleScreenBufferInfo(h, &csbi);

                //SetConsoleSizeInt(size); -- вроде бы если уж консоль сглючила - не помогает
                //GetConsoleScreenBufferInfo(h, &csbi);

                //size.X--; size.Y--;
                //SETCONSOLESCREENBUFFERSIZERET(h, csbi.dwSize, lbWS);
                //size.X++; size.Y++;
                //SETCONSOLESCREENBUFFERSIZERET(h, size, lbWS);
            }
            #ifdef _DEBUG
            /*if (!GetConsoleScreenBufferInfo(h, &csbi))
                __asm int 3;
            else if (csbi.dwSize.X != size.X || csbi.dwSize.Y != size.Y)
                __asm int 3;*/
            #endif
            // Иногда, хз по какой причине, до консольного приложения не доходит правильный размер. дернем?
            /*INPUT_RECORD r = {WINDOW_BUFFER_SIZE_EVENT};
            r.Event.WindowBufferSizeEvent.dwSize = size;
            DWORD dwWritten = 0;
            if (!WriteConsoleInput(hConIn(), &r, 1, &dwWritten)) {
                #ifdef _DEBUG
                DisplayLastError(L"WindowBufferSizeEvent failed!");
                #endif
            }*/
        } else {
            #ifdef _DEBUG
            lbNeedChange = lbNeedChange;
            #endif
        }
        //TODO: если правый нижний край вылезет за пределы экрана?
        MOVEWINDOW(hConWnd, rcConPos.left, rcConPos.top, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
        if (lbRc) {
	        // Если размер изменили сразу обновим информацию
	        InitDC(false, true);
			// И перечитать данные (выполняется в дополнительной нити)
			Update(true);
        }
        return lbRc;
    }

    // global flag of the first call which is:
    // *) after getting all the settings
    // *) before running the command
    static bool s_isFirstCall = true;

    // case: buffer mode: change buffer
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConOut(), &csbi))
        return false;
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
    MOVEWINDOW(hConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
    SETCONSOLESCREENBUFFERSIZE(hConOut(), csbi.dwSize);
    //окошко раздвигаем только по ширине!
    GetWindowRect(hConWnd, &rcConPos);
    MOVEWINDOW(hConWnd, rcConPos.left, rcConPos.top, GetSystemMetrics(SM_CXSCREEN), rcConPos.bottom-rcConPos.top, 1);

    
    // set console window
    if (!GetConsoleScreenBufferInfo(hConOut(), &csbi))
        return false;
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
    return true;
}

BOOL CVirtualConsole::AttachPID(DWORD dwPID)
{
    #ifdef MSGLOGGER
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

    InitHandlers(FALSE);

    // Попытаться дернуть плагин для установки шрифта.
    CConEmuPipe pipe;
    
    OutputDebugString(_T("CheckProcesses\n"));
    gConEmu.CheckProcesses(0,TRUE);
    
    if (pipe.Init(_T("DefFont.in.attach"), TRUE))
        pipe.Execute(CMD_DEFFONT);

    return TRUE;
}

void CVirtualConsole::InitHandlers(BOOL abCreated)
{
    hConWnd = GetConsoleWindow();

    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CConEmuMain::HandlerRoutine, true);
    
    // наверное это имеет смысл только при создании консоли, а не при аттаче
    SetHandleInformation(GetStdHandle(STD_INPUT_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
	// DuplicateHandle смысле не имеет, при первом же FreeConsole - эти хэндлы отвалятся
	//if (!DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &mh_StdIn, 0, TRUE, DUPLICATE_SAME_ACCESS)) {
	//	dwErr = GetLastError();
	mh_StdIn = CreateFile(_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	//}
    SetHandleInformation(GetStdHandle(STD_OUTPUT_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
	//if (!DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &mh_StdOut, 0, TRUE, DUPLICATE_SAME_ACCESS)) {
	//	dwErr = GetLastError();
	mh_StdOut = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	//}
    SetHandleInformation(GetStdHandle(STD_ERROR_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

    DWORD mode = 0;
    BOOL lb = GetConsoleMode(hConIn(), &mode);
    if (!(mode & ENABLE_MOUSE_INPUT)) {
        mode |= ENABLE_MOUSE_INPUT;
        lb = SetConsoleMode(hConIn(), mode);
    }

    hConOut();

    if (abCreated) {
        SetConsoleFontSizeTo(4, 6);

        if (IsIconic(ghConWnd)) {
            // окошко нужно развернуть!
            WINDOWPLACEMENT wplMain, wplCon;
            memset(&wplMain, 0, sizeof(wplMain)); wplMain.length = sizeof(wplMain);
            memset(&wplCon, 0, sizeof(wplCon)); wplCon.length = sizeof(wplCon);
            GetWindowPlacement(ghWnd, &wplMain);
            GetWindowPlacement(ghConWnd, &wplCon);

            int n = wplMain.rcNormalPosition.left - wplCon.rcNormalPosition.left;
            wplCon.rcNormalPosition.left += n; wplCon.rcNormalPosition.right += n;
            n = wplMain.rcNormalPosition.top - wplCon.rcNormalPosition.top;
            wplCon.rcNormalPosition.top += n; wplCon.rcNormalPosition.bottom += n;
            wplCon.showCmd = SW_RESTORE;
            SetWindowPlacement(ghConWnd, &wplCon);
        }

        if (gSet.wndHeight && gSet.wndWidth)
        {
			// Скорректировать под размер экрана!
			RECT rcCon = MakeRect(gSet.wndWidth, gSet.wndHeight);
			RECT rcWnd = gConEmu.CalcRect(CER_MAIN, rcCon, CER_CONSOLE);
			rcWnd = gConEmu.CalcRect(CER_CORRECTED, rcWnd, gSet.isFullScreen ? CER_FULLSCREEN : CER_MAXIMIZED);
			rcCon = gConEmu.CalcRect(CER_CONSOLE, rcWnd, CER_MAIN);
			//
			COORD b = {min(((int)gSet.wndWidth),rcCon.right), min(((int)gSet.wndHeight),rcCon.bottom)};
            SetConsoleSize(b);
        }

        SetConsoleTitle(gSet.GetCmd());
    } else {
        SetConsoleFontSizeTo(4, 6);
    }

    gConEmu.ConsoleCreated(hConWnd);
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
            int nLen = _tcslen(gSet.ourSI.lpTitle);
            if (nLen>4 && _tcsicmp(gSet.ourSI.lpTitle+nLen-4, _T(".lnk"))==0) {
                props.FullKeyName = (TCHAR*)Alloc(10, sizeof(TCHAR));
                _tcscpy(props.FullKeyName, _T("Console"));
                pszExeName = props.FullKeyName+_tcslen(props.FullKeyName);
            } else {
                props.FullKeyName = (TCHAR*)Alloc(nLen+10, sizeof(TCHAR));
                _tcscpy(props.FullKeyName, _T("Console\\"));
                pszExeName = props.FullKeyName+_tcslen(props.FullKeyName);
                _tcscpy(pszExeName, gSet.ourSI.lpTitle);
            }
        } else
        if (asExeName && *asExeName) {
            props.FullKeyName = (TCHAR*)Alloc(_tcslen(asExeName)+10, sizeof(TCHAR));
            _tcscpy(props.FullKeyName, _T("Console\\"));
            pszExeName = props.FullKeyName+_tcslen(props.FullKeyName);
            _tcscpy(pszExeName, asExeName);
        } else {
            props.FullKeyName = (TCHAR*)Alloc(MAX_PATH+10, sizeof(TCHAR));
            _tcscpy(props.FullKeyName, _T("Console\\"));
            pszExeName = props.FullKeyName+_tcslen(props.FullKeyName);
            if (!GetModuleFileName(NULL, pszExeName, MAX_PATH+1)) {
                DisplayLastError(_T("Can't get module file name"));
                if (props.FullKeyName) { Free(props.FullKeyName); props.FullKeyName = NULL; }
                return;
            }
        }
        
        for (TCHAR* psz=pszExeName; pszExeName && *psz; psz++) {
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
        if (props.FullKeyName) { Free(props.FullKeyName); props.FullKeyName = NULL;}
        if (props.FaceName) { Free(props.FaceName); props.FaceName = NULL;}
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
            props.FaceName = (TCHAR*)Alloc(MAX_PATH+1,sizeof(TCHAR));
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
            if (props.FullKeyName) { Free(props.FullKeyName); props.FullKeyName = NULL;}
            if (props.FaceName) { Free(props.FaceName); props.FaceName = NULL;}
        }
        RegCloseKey(hkey);
    }
}

DWORD CVirtualConsole::StartProcessThread(LPVOID lpParameter)
{
    CVirtualConsole* pCon = (CVirtualConsole*)lpParameter;
    BOOL lbRc = pCon->StartProcess();
    
    //TODO: а тут мы будем читать консоль...
    HANDLE hEvents[2] = {pCon->mh_TermEvent, 
                         pCon->mh_ForceReadEvent};
	DWORD  nEvents = countof(hEvents);
    DWORD  nWait = 0;
    BOOL   bLoop = TRUE, bIconic = FALSE;

	DWORD nElapse = max(10,gSet.nMainTimerElapse);
    
    while (TRUE/*bLoop*/)
    {
	    gSet.Performance(tPerfInterval, TRUE); // именно обратный отсчет. Мы смотрим на промежуток МЕЖДУ таймерами

        bIconic = IsIconic(ghWnd);
        // в минимизированном режиме - сократить расходы
        nWait = WaitForMultipleObjects(nEvents, hEvents, FALSE, bIconic ? 1000 : nElapse);

		if (nWait == WAIT_OBJECT_0 /*|| !bLoop*/)
            break; // требование завершения нити
          
		// Размер консоли меняем в том треде, в котором это требуется. Иначе можем заблокироваться при Update (InitDC)
        // Требуется изменение размеров консоли
        /*if (nWait == (WAIT_OBJECT_0+2)) {
	        pCon->SetConsoleSize(pCon->m_ReqSetSize);
	        //SetEvent(pCon->mh_ReqSetSizeEnd);
	        //continue; -- и сразу обновим информацию о ней
        }*/
        
        bool bActive = gConEmu.isActive(pCon);

		DWORD dwT1 = GetTickCount();

        try {   
            ResetEvent(pCon->mh_EndUpdateEvent);


			if (GetWindowText(pCon->hConWnd, pCon->TitleCmp, countof(pCon->TitleCmp)-2)
				&& wcscmp(pCon->Title, pCon->TitleCmp))
			{
				wcscpy(pCon->Title, pCon->TitleCmp);
				if (bActive)
					gConEmu.UpdateTitle(pCon->TitleCmp);
			}

			bool lbForceUpdate = pCon->CheckBufferSize();
			if (!lbForceUpdate && (nWait == (WAIT_OBJECT_0+1)))
				lbForceUpdate = true;

            if (pCon->Update(lbForceUpdate) && bActive)
				gConEmu.m_Child.Invalidate();
                //InvalidateRect(ghWndDC, NULL, FALSE);
            
            SetEvent(pCon->mh_EndUpdateEvent);
        } catch(...) {
            bLoop = FALSE;
        }

		/*if (nWait == (WAIT_OBJECT_0+2)) {
	        SetEvent(pCon->mh_ReqSetSizeEnd);
		}*/

		DWORD dwT2 = GetTickCount();
		DWORD dwD = max(10,(dwT2 - dwT1));
		nElapse = nElapse*0.7 + dwD*0.3;

		if (!bLoop) {
			#ifdef _DEBUG
			__asm int 3;
			#endif
			pCon->Box(_T("Exception triggered in CVirtualConsole::StartProcessThread"));
			bLoop = true;
		}

	    gSet.Performance(tPerfInterval, FALSE);
    }
    
    
    // Finalize
    //SafeCloseHandle(pCon->mh_Thread);
    
    return lbRc ? 0 : 100;
}

BOOL CVirtualConsole::StartProcess()
{
    BOOL lbRc = FALSE;

    if (ghConWnd) {
        // Сначала нужно отцепиться от текущей консоли
        FreeConsole(); ghConWnd = NULL;
    }
    
    
    ConExeProps props;
    
    // Если запускались с ярлыка - это нихрена не поможет... информация похоже в .lnk сохраняется...
    RegistryProps(FALSE, props);

    if (!gConEmu.isShowConsole && !gSet.isConVisible
        #ifdef MSGLOGGER
        && !IsDebuggerPresent()
        #endif
        ) SetWindowPos(ghWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
    AllocConsole();

    InitHandlers(TRUE);

    if (!gConEmu.isShowConsole && !gSet.isConVisible
        #ifdef MSGLOGGER
        && !IsDebuggerPresent()
        #endif
        ) SetWindowPos(ghWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);

    SetForegroundWindow(ghWnd);
    
    RegistryProps(TRUE, props);

#pragma message("error: почему-то консоль создалась схлопнутой по ширине/высоте, хотя ее размеры были нормальные!")
	// TODO: перед Update нужно позвать установку размера консоли!

	Update(true);

    /*if (gSet.isConMan) {
        if (!gConEmu.InitConMan(gSet.GetCmd())) {
            // Иначе жестоко получается. ConEmu вообще будет сложно запустить...
            gSet.isConMan = false;
        } else {
            //SetForegroundWindow(ghWnd);
            return TRUE;
        }
    }*/ 
    
    //if (!gSet.isConMan)
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
            #ifdef MSGLOGGER
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
                //Box("Cannot execute the command.");
                DWORD dwLastError = GetLastError();
                OutputDebugString(_T("CreateProcess failed\n"));
                int nLen = _tcslen(gSet.GetCmd());
                TCHAR* pszErr=(TCHAR*)Alloc(nLen+100,sizeof(TCHAR));
                
                if (0==FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                    pszErr, 1024, NULL))
                {
                    wsprintf(pszErr, _T("Unknown system error: 0x%x"), dwLastError);
                }
                
                nLen += _tcslen(pszErr);
                TCHAR* psz=(TCHAR*)Alloc(nLen+100,sizeof(TCHAR));
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
                //Box(psz);
                int nBrc = MessageBox(NULL, psz, _T("ConEmu"), nButtons);
                Free(psz); Free(pszErr);
                if (nBrc!=IDYES) {
                    gConEmu.Destroy();
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

    CSection SCON(&csCON, &ncsTCON);
    
    //CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
    GetConsoleScreenBufferInfo(hConOut(), &csbi);
    if (csbi.dwSize.X>(csbi.srWindow.Right-csbi.srWindow.Left+1)) {
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
        if ( (csbi.dwSize.Y<(csbi.srWindow.Bottom-csbi.srWindow.Top+10)) && gConEmu.BufferHeight &&
             !gSet.BufferHeight /*&& (gConEmu.BufferHeight != csbi.dwSize.Y)*/)
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
        if ( (csbi.dwSize.Y>(csbi.srWindow.Bottom-csbi.srWindow.Top+10)) ||
             (gConEmu.BufferHeight && (gConEmu.BufferHeight != csbi.dwSize.Y)) )
        {
            // может быть консольная программа увеличила буфер самостоятельно?
            if (gConEmu.BufferHeight != csbi.dwSize.Y) {
                // TODO: Включить прокрутку!!!
                gConEmu.BufferHeight = csbi.dwSize.Y;
                lbForceUpdate = true;
            }
        }
        
        if ((gConEmu.BufferHeight == 0) && (csbi.dwSize.Y>(csbi.srWindow.Bottom-csbi.srWindow.Top+1))) {
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
        lb = GetConsoleMode(hConIn(), &mode);

        if (csbi.dwSize.Y>(csbi.srWindow.Bottom-csbi.srWindow.Top+1)) {
            // Буфер больше высоты окна
            mode |= ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE|ENABLE_EXTENDED_FLAGS;
        } else {
            // Буфер равен высоте окна (значит ФАР запустился)
            mode &= ~(ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE);
            mode |= ENABLE_EXTENDED_FLAGS;
        }

        lb = SetConsoleMode(hConIn(), mode);
    }
    
    return lbForceUpdate;
}

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL                  0x020E
#endif
void CVirtualConsole::SendMouseEvent(UINT messg, WPARAM wParam, int x, int y)
{
    if (!this || !ghConWnd)
        return;

    BOOL lbStdMode = FALSE;
    if (gConEmu.BufferHeight == 0)
        lbStdMode = TRUE;

    if (lbStdMode)
    {
        Assert ( gSet.LogFont.lfWidth && gSet.LogFont.lfHeight );
        if (!gSet.LogFont.lfWidth | !gSet.LogFont.lfHeight)
            return;
        #pragma message("TODO: X координаты нам известны, так что можно бы более корректно позицию определять...")
        
        INPUT_RECORD r; memset(&r, 0, sizeof(r));
        r.EventType = MOUSE_EVENT;
		#pragma message("TODO: а здесь хорошо бы получать известные координаты символов, а не простым делением")
        r.Event.MouseEvent.dwMousePosition = MakeCoord(x/gSet.LogFont.lfWidth, y/gSet.LogFont.lfHeight);
        
        // Mouse Buttons
        if (messg != WM_LBUTTONUP && (messg == WM_LBUTTONDOWN || messg == WM_LBUTTONDBLCLK || isPressed(VK_LBUTTON)))
            r.Event.MouseEvent.dwButtonState |= FROM_LEFT_1ST_BUTTON_PRESSED;
        if (messg != WM_RBUTTONUP && (messg == WM_RBUTTONDOWN || messg == WM_RBUTTONDBLCLK || isPressed(VK_RBUTTON)))
            r.Event.MouseEvent.dwButtonState |= RIGHTMOST_BUTTON_PRESSED;
        if (messg != WM_MBUTTONUP && (messg == WM_MBUTTONDOWN || messg == WM_MBUTTONDBLCLK || isPressed(VK_MBUTTON)))
            r.Event.MouseEvent.dwButtonState |= FROM_LEFT_2ND_BUTTON_PRESSED;

        // Key modifiers
        if (GetKeyState(VK_CAPITAL) & 1)
            r.Event.MouseEvent.dwControlKeyState |= CAPSLOCK_ON;
        if (GetKeyState(VK_NUMLOCK) & 1)
            r.Event.MouseEvent.dwControlKeyState |= NUMLOCK_ON;
        if (GetKeyState(VK_SCROLL) & 1)
            r.Event.MouseEvent.dwControlKeyState |= SCROLLLOCK_ON;
        if (isPressed(VK_LMENU))
            r.Event.MouseEvent.dwControlKeyState |= LEFT_ALT_PRESSED;
        if (isPressed(VK_RMENU))
            r.Event.MouseEvent.dwControlKeyState |= RIGHT_ALT_PRESSED;
        if (isPressed(VK_LCONTROL))
            r.Event.MouseEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;
        if (isPressed(VK_RCONTROL))
            r.Event.MouseEvent.dwControlKeyState |= RIGHT_CTRL_PRESSED;
        if (isPressed(VK_SHIFT))
            r.Event.MouseEvent.dwControlKeyState |= SHIFT_PRESSED;

        if (messg == WM_LBUTTONDBLCLK || messg == WM_RBUTTONDBLCLK || messg == WM_MBUTTONDBLCLK)
            r.Event.MouseEvent.dwEventFlags = DOUBLE_CLICK;
        else if (messg == WM_MOUSEMOVE)
            r.Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
        else if (messg == WM_MOUSEWHEEL) {
            r.Event.MouseEvent.dwEventFlags = MOUSE_WHEELED;
            r.Event.MouseEvent.dwButtonState |= (0xFFFF0000 & wParam);
        } else if (messg == WM_MOUSEHWHEEL) {
            r.Event.MouseEvent.dwEventFlags = 8/*MOUSE_HWHEELED*/;
            r.Event.MouseEvent.dwButtonState |= (0xFFFF0000 & wParam);
        }
        DWORD dwWritten = 0;
        if (!WriteConsoleInput(hConIn(), &r, 1, &dwWritten)) {
            DisplayLastError(L"SendMouseEvent failed!");
        }
    } else {
        /*Если пересылать так - лезут глюки:
        1) левый клик вообще не доходит до консоли
        2) правый клик вызывает "левую" консольную менюшку
        3) проблемы с активацией окна(?)*/
        /* НО! тогда не работает выделение мышкой */
        static bool bInitialized = false;
        if (!bInitialized && (messg == WM_LBUTTONDOWN || messg == WM_RBUTTONDOWN))
        {
	        bInitialized = true; // выполнить эту операцию один раз
	        if (gConEmu.isConmanAlternative()) {
		        POSTMESSAGE(hConWnd, WM_COMMAND, SC_PASTE_SECRET, 0, TRUE);
	        } else {
		        POSTMESSAGE(hConWnd, WM_COMMAND, (messg == WM_LBUTTONDOWN) ? SC_MARK_SECRET : SC_PASTE_SECRET, 0, TRUE);
		        if (messg == WM_RBUTTONDOWN)
			        return; // достаточно SC_PASTE_SECRET
			}
        }

        RECT conRect;
        GetClientRect(hConWnd, &conRect);
        short newX = MulDiv(x, conRect.right, klMax<uint>(1, Width));
        short newY = MulDiv(y, conRect.bottom, klMax<uint>(1, Height));

        POSTMESSAGE(hConWnd, messg, wParam, MAKELPARAM( newX, newY ), TRUE);
    }
}

LPVOID CVirtualConsole::Alloc(size_t nCount, size_t nSize)
{
#ifdef _DEBUG
    HeapValidate(mh_Heap, 0, NULL);
#endif
    size_t nWhole = nCount * nSize;
    LPVOID ptr = HeapAlloc ( mh_Heap, HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY, nWhole );
#ifdef _DEBUG
        HeapValidate(mh_Heap, 0, NULL);
#endif
    return ptr;
}

void CVirtualConsole::Free(LPVOID ptr)
{
    if (ptr && mh_Heap) {
#ifdef _DEBUG
        HeapValidate(mh_Heap, 0, NULL);
#endif
        HeapFree ( mh_Heap, 0, ptr );
#ifdef _DEBUG
        HeapValidate(mh_Heap, 0, NULL);
#endif
    }
}

void CVirtualConsole::StopSignal()
{
    SetEvent(mh_TermEvent);
}

void CVirtualConsole::StopThread()
{
#ifdef _DEBUG
        HeapValidate(mh_Heap, 0, NULL);
#endif

    if (mh_Thread) {
        // выставление флагов и завершение нити
        SetEvent(mh_TermEvent);
        if (WaitForSingleObject(mh_Thread, 300) != WAIT_OBJECT_0)
            TerminateThread(mh_Thread, 1);
    }
    
    SafeCloseHandle(mh_TermEvent);
    SafeCloseHandle(mh_ForceReadEvent);
    SafeCloseHandle(mh_EndUpdateEvent);
    
    SafeCloseHandle(mh_Thread);

#ifdef _DEBUG
        HeapValidate(mh_Heap, 0, NULL);
#endif
}

void CVirtualConsole::Paint()
{
    if (!ghWndDC)
        return;
    
    if (!this) {
        // Залить цветом 0
		#ifdef _DEBUG
			int nBackColorIdx = 2;
		#else
			int nBackColorIdx = 0;
		#endif
        HBRUSH hBr = CreateSolidBrush(gSet.Colors[nBackColorIdx]);
        RECT rcClient; GetClientRect(ghWndDC, &rcClient);
        PAINTSTRUCT ps;
        HDC hDc = BeginPaint(ghWndDC, &ps);
        FillRect(hDc, &rcClient, hBr);
        DeleteObject(hBr);
        EndPaint(ghWndDC, &ps);
        return;
    }

    BOOL lbExcept = FALSE;
    RECT client;
    PAINTSTRUCT ps;
    HDC hPaintDc = NULL;

    //CSection S(&csDC, &ncsTDC);
	CSection S(&csCON, &ncsTCON);
    if (!S.isLocked())
        return; // не удалось получить доступ к CS

    GetClientRect(ghWndDC, &client);

    if (!gConEmu.isNtvdm()) {
        // после глюков с ресайзом могут быть проблемы с размером окна DC
        if (client.right < (LONG)Width || client.bottom < (LONG)Height)
            gConEmu.OnSize();
    }
    
    try {
        hPaintDc = BeginPaint(ghWndDC, &ps);

        HBRUSH hBr = NULL;
        if (((ULONG)client.right) > Width) {
            if (!hBr) hBr = CreateSolidBrush(gSet.Colors[mn_BackColorIdx]);
            RECT rcFill = MakeRect(Width, 0, client.right, client.bottom);
            FillRect(hPaintDc, &rcFill, hBr);
            client.right = Width;
        }
        if (((ULONG)client.bottom) > Height) {
            if (!hBr) hBr = CreateSolidBrush(gSet.Colors[mn_BackColorIdx]);
            RECT rcFill = MakeRect(0, Height, client.right, client.bottom);
            FillRect(hPaintDc, &rcFill, hBr);
            client.bottom = Height;
        }
        if (hBr) { DeleteObject(hBr); hBr = NULL; }


        if (!gbNoDblBuffer) {
            // Обычный режим
            BitBlt(hPaintDc, 0, 0, client.right, client.bottom, hDC, 0, 0, SRCCOPY);
        } else {
            GdiSetBatchLimit(1); // отключить буферизацию вывода для текущей нити

            GdiFlush();
            // Рисуем сразу на канвасе, без буферизации
            Update(true, &hPaintDc);
        }

        if (gbNoDblBuffer) GdiSetBatchLimit(0); // вернуть стандартный режим
    } catch(...) {
        lbExcept = TRUE;
    }
    
    S.Leave();
    
    if (lbExcept)
        Box(_T("Exception triggered in CVirtualConsole::Paint"));

    if (hPaintDc && ghWndDC) {
        EndPaint(ghWndDC, &ps);
    }
}

void CVirtualConsole::UpdateInfo()
{
    if (!ghOpWnd || !this)
        return;

    if (!gConEmu.isMainThread()) {
        return;
    }

    TCHAR szSize[128];
    wsprintf(szSize, _T("%ix%i"), TextWidth, TextHeight);
    SetDlgItemText(gSet.hInfo, tConSizeChr, szSize);
    wsprintf(szSize, _T("%ix%i"), Width, Height);
    SetDlgItemText(gSet.hInfo, tConSizePix, szSize);

    wsprintf(szSize, _T("(%i, %i)-(%i, %i), %ix%i"), mr_LeftPanel.left+1, mr_LeftPanel.top+1, mr_LeftPanel.right+1, mr_LeftPanel.bottom+1, mr_LeftPanel.right-mr_LeftPanel.left+1, mr_LeftPanel.bottom-mr_LeftPanel.top+1);
    SetDlgItemText(gSet.hInfo, tPanelLeft, szSize);
    wsprintf(szSize, _T("(%i, %i)-(%i, %i), %ix%i"), mr_RightPanel.left+1, mr_RightPanel.top+1, mr_RightPanel.right+1, mr_RightPanel.bottom+1, mr_RightPanel.right-mr_RightPanel.left+1, mr_RightPanel.bottom-mr_RightPanel.top+1);
    SetDlgItemText(gSet.hInfo, tPanelRight, szSize);
}

void CVirtualConsole::Box(LPCTSTR szText)
{
#ifdef _DEBUG
    __asm int 3;
#endif
    MessageBox(NULL, szText, _T("ConEmu"), MB_ICONSTOP);
}

//
//  Fill the CONSOLE_INFO structure with information
//  about the current console window
//
void CVirtualConsole::GetConsoleSizeInfo(CONSOLE_INFO *pci)
{
    CSection SCON(&csCON, &ncsTCON);
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    HANDLE hConsoleOut = hConOut(); //hConOut();

    GetConsoleScreenBufferInfo(hConsoleOut, &csbi);

    pci->ScreenBufferSize = csbi.dwSize;
    pci->WindowSize.X     = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    pci->WindowSize.Y     = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    pci->WindowPosX       = csbi.srWindow.Left;
    pci->WindowPosY       = csbi.srWindow.Top;
}

//
//  Wrapper around WM_SETCONSOLEINFO. We need to create the
//  necessary section (file-mapping) object in the context of the
//  process which owns the console, before posting the message
//
BOOL CVirtualConsole::SetConsoleInfo(CONSOLE_INFO *pci)
{
    DWORD   dwConsoleOwnerPid, dwCurProcId;
    HANDLE  hProcess=NULL;
    HANDLE  hSection=NULL, hDupSection=NULL;
    PVOID   ptrView = 0;
    DWORD   dwLastError=0;
    WCHAR   ErrText[255];
    
    //
    //  Open the process which "owns" the console
    //  
    dwCurProcId = GetCurrentProcessId();
    dwConsoleOwnerPid = dwCurProcId;
    hProcess = GetCurrentProcess();
    
    // Если уж мы к консоли подцепились - считаем ее своей!
    /*GetWindowThreadProcessId(hwndConsole, &dwConsoleOwnerPid);
    
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwConsoleOwnerPid);
    dwLastError = GetLastError();
    //Какого черта OpenProcess не дает себя открыть? GetLastError возвращает 5
    if (hProcess==NULL) {
        if (dwConsoleOwnerPid == dwCurProcId) {
            hProcess = GetCurrentProcess();
        } else {
            wsprintf(ErrText, L"Can't open console process. ErrCode=%i", dwLastError);
            MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
            return FALSE;
        }
    }*/

    //
    // Create a SECTION object backed by page-file, then map a view of
    // this section into the owner process so we can write the contents 
    // of the CONSOLE_INFO buffer into it
    //
    hSection = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, pci->Length, 0);
    if (!hSection) {
        dwLastError = GetLastError();
        wsprintf(ErrText, L"Can't CreateFileMapping. ErrCode=%i", dwLastError);
        MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);

    } else {
        //
        //  Copy our console structure into the section-object
        //
        ptrView = MapViewOfFile(hSection, FILE_MAP_WRITE|FILE_MAP_READ, 0, 0, pci->Length);
        if (!ptrView) {
            dwLastError = GetLastError();
            wsprintf(ErrText, L"Can't MapViewOfFile. ErrCode=%i", dwLastError);
            MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);

        } else {
            memcpy(ptrView, pci, pci->Length);

            UnmapViewOfFile(ptrView);

            //
            //  Map the memory into owner process
            //
            if (!DuplicateHandle(GetCurrentProcess(), hSection, hProcess, &hDupSection, 0, FALSE, DUPLICATE_SAME_ACCESS)
                || !hDupSection)
            {
                dwLastError = GetLastError();
                wsprintf(ErrText, L"Can't DuplicateHandle. ErrCode=%i", dwLastError);
                MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);

            } else {
                //  Send console window the "update" message
                DWORD dwConInfoRc = 0;
                DWORD dwConInfoErr = 0;
                
                dwConInfoRc = SendMessage(hConWnd, WM_SETCONSOLEINFO, (WPARAM)hDupSection, 0);
                dwConInfoErr = GetLastError();
            }
        }
    }



    //
    // clean up
    //

    if (hDupSection) {
        if (dwConsoleOwnerPid == dwCurProcId) {
            // Если это наша консоль - зачем с нитями извращаться
            CloseHandle(hDupSection);
        } else {
            HANDLE  hThread=NULL;
            hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)CloseHandle, hDupSection, 0, 0);
            if (hThread) CloseHandle(hThread);
        }
    }

    if (hSection)
        CloseHandle(hSection);
    if ((dwConsoleOwnerPid!=dwCurProcId) && hProcess)
        CloseHandle(hProcess);

    return TRUE;
}

//VISTA support:
#ifndef ENABLE_AUTO_POSITION
typedef struct _CONSOLE_FONT_INFOEX {
    ULONG cbSize;
    DWORD nFont;
    COORD dwFontSize;
    UINT FontFamily;
    UINT FontWeight;
    WCHAR FaceName[LF_FACESIZE];
} CONSOLE_FONT_INFOEX, *PCONSOLE_FONT_INFOEX;
#endif


typedef BOOL (WINAPI *PGetCurrentConsoleFontEx)(__in HANDLE hConsoleOutput,__in BOOL bMaximumWindow,__out PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx);
typedef BOOL (WINAPI *PSetCurrentConsoleFontEx)(__in HANDLE hConsoleOutput,__in BOOL bMaximumWindow,__out PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx);


void CVirtualConsole::SetConsoleFontSizeTo(int inSizeX, int inSizeY)
{
    PGetCurrentConsoleFontEx GetCurrentConsoleFontEx = (PGetCurrentConsoleFontEx)
        GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "GetCurrentConsoleFontEx");
    PSetCurrentConsoleFontEx SetCurrentConsoleFontEx = (PSetCurrentConsoleFontEx)
        GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "SetCurrentConsoleFontEx");

    if (GetCurrentConsoleFontEx && SetCurrentConsoleFontEx) // We have Vista
    {
        CONSOLE_FONT_INFOEX cfi = {sizeof(CONSOLE_FONT_INFOEX)};
        //GetCurrentConsoleFontEx(hConOut(), FALSE, &cfi);
        cfi.dwFontSize.X = inSizeX;
        cfi.dwFontSize.Y = inSizeY;
        //TODO: А Люциду кто ставить будет???
        _tcscpy(cfi.FaceName, _T("Lucida Console"));
        SetCurrentConsoleFontEx(hConOut()/*hConOut()*/,
            FALSE, &cfi);
    }
    else // We have other NT
    {
        const COLORREF DefaultColors[16] = 
        {
            0x00000000, 0x00800000, 0x00008000, 0x00808000,
            0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0, 
            0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00,
            0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
        };

        CONSOLE_INFO ci = { sizeof(ci) };
        int i;

        // get current size/position settings rather than using defaults..
        GetConsoleSizeInfo(&ci);

        // set these to zero to keep current settings
        ci.FontSize.X               = inSizeX;
        ci.FontSize.Y               = inSizeY;
        ci.FontFamily               = 0;//0x30;//FF_MODERN|FIXED_PITCH;//0x30;
        ci.FontWeight               = 0;//0x400;
        _tcscpy(ci.FaceName, _T("Lucida Console"));

        ci.CursorSize               = 25;
        ci.FullScreen               = FALSE;
        ci.QuickEdit                = FALSE;
        ci.AutoPosition             = 0x10000;
        ci.InsertMode               = TRUE;
        ci.ScreenColors             = MAKEWORD(0x7, 0x0);
        ci.PopupColors              = MAKEWORD(0x5, 0xf);

        ci.HistoryNoDup             = FALSE;
        ci.HistoryBufferSize        = 50;
        ci.NumberOfHistoryBuffers   = 4;

        // color table
        for(i = 0; i < 16; i++)
            ci.ColorTable[i] = DefaultColors[i];

        ci.CodePage                 = GetConsoleOutputCP();//0;//0x352;
        ci.Hwnd                     = hConWnd;

        *ci.ConsoleTitle = NULL;

        //!!! чего-то не работает... а в
        // F:\VCProject\FarPlugin\ConEmu\080703\ConEmu\setconsoleinfo.cpp
        //вроде ок
        SetConsoleInfo(&ci);
    }
}

BOOL CVirtualConsole::isBufferHeight()
{
    if (!this)
        return FALSE;

    CSection SCON(&csCON, &ncsTCON);

    BOOL lbScrollMode = FALSE;
    CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
    GetConsoleScreenBufferInfo(hConOut(), &inf);
    if (inf.dwSize.Y>(inf.srWindow.Bottom-inf.srWindow.Top+1))
        lbScrollMode = TRUE;

    return lbScrollMode;
}

LPCTSTR CVirtualConsole::GetTitle()
{
	return Title;
}
