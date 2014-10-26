#if 0


WNDCLASSEX wc;
...
wc.cbSize        = sizeof(wndclass);
wc.style         = 0;
wc.lpfnWndProc   = SplitterProc;
wc.cbClsExtra    = 0;
wc.cbWndExtra    = 0;
wc.hInstance     = hInstance;
wc.hIcon         = NULL;
wc.hCursor       = LoadCursor (hInstance, MAKEINTRESOURCE(IDC_HORZSPLITTER));
wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);   
wc.lpszMenuName  = NULL;
wc.lpszClassName = "Splitter";
wc.hIconSm       = NULL;

RegisterClassEx(&wc);


----------------------------------
Mouse button down (WM_LBUTTONDOWN)

splitpos  = GetCursorPos();
 
fDragMode = TRUE;      /* set dragging to active */
SetCapture(hwnd);      /* capture the mouse so we receive all messages */
 
hdc = GetDC(hwnd);     /* Draw the checkered resizing bar */
DrawXorBar(hdc, splitpos);
ReleaseDC(hwnd); 
 
old_splitpos = splitpos;


-----------------
Mouse move (WM_MOUSEMOVE)

if(fDragMode == FALSE) return;
 
splitpos  = GetCursorPos();
 
hdc = GetDC(hwnd);
             
DrawXorBar(hdc, old_splitpos);  /* Erase where the bar WAS */
DrawXorBar(hdc, splitpos);      /* Draw it where it IS now */
 
ReleaseDC(hwnd); 
 
old_splitpos = splitpos;


-------------------------
Mouse button up (WM_LBUTTONUP)

if(fDragMode == FALSE) return;
 
fDragMode = FALSE;     /* set dragging to unactive */
ReleaseCapture();      /* release the mouse capture: we don't need it */
hdc = GetDC(hwnd);     /* Draw the bar again to erase it */
 
DrawXorBar(hdc, old_splitpos);    
ReleaseDC(hwnd);

----------------------------
Drawing the feedback bar


void DrawXorBar(HDC hdc, int x1, int y1, int width, int height)
{
  static WORD _dotPatternBmp[8] = { 0x00aa, 0x0055, 0x00aa, 0x0055, 
                                    0x00aa, 0x0055, 0x00aa, 0x0055};
 
  HBITMAP hbm;
  HBRUSH  hbr, hbrushOld;
 
  /* create a monochrome checkered pattern */
  hbm = CreateBitmap(8, 8, 1, 1, _dotPatternBmp);
  hbr = CreatePatternBrush(hbm);
 
  SetBrushOrgEx(hdc, x1, y1, 0);
  hbrushOld = (HBRUSH)SelectObject(hdc, hbr);
 
  /* draw the checkered rectangle to the screen */
  PatBlt(hdc, x1, y1, width, height, PATINVERT);
   
  SelectObject(hdc, hbrushOld);
  DeleteObject(hbr);
  DeleteObject(hbm);
}

#endif


/*
Copyright (c) 2014 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"

#include "Splitter.h"

CSplitter::CSplitter()
{
}

CSplitter::~CSplitter()
{
}

