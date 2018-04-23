
/*
Copyright (c) 2009-present Maximus5
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

#include "../common/Common.h"
#include "../common/RgnDetect.h"
#ifdef _DEBUG
#pragma warning( disable : 4995 )
#endif
#include "../common/pluginW1761.hpp"
#ifdef _DEBUG
#pragma warning( default : 4995 )
#endif
#include "PluginHeader.h"

// Можно бы добавить обработку Up/Down для перехода между пакетами
//  сразу "жать" Esc, вызвать RestoreScreen (но без коммита на экран)
//  и послать макрос "Down Enter" - фар сделает остальное
//
// Кнопочкой '*' при отображении дампа переключать режим
//  = нормальный (отображается цветной текст, как был в консоли)
//  = атрибутивный (отображаются ТОЛЬКО атрибуты, но как текст, а не как цвет)

//extern struct PluginStartupInfo *InfoW995;
//extern struct FarStandardFunctions *FSFW995;

CONSOLE_SCREEN_BUFFER_INFO csbi;
int gnPage = 0;
bool gbShowAttrsOnly = false;

BOOL CheckConInput(INPUT_RECORD* pr)
{
	DWORD dwCount = 0;
	memset(pr, 0, sizeof(INPUT_RECORD));
	BOOL lbRc = ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), pr, 1, &dwCount);

	if (!lbRc)
		return FALSE;

	if (pr->EventType == KEY_EVENT)
	{
		if (pr->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
			return FALSE;
	}

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	return TRUE;
}

void ShowConPacket(CESERVER_REQ* pReq)
{
	// Где-то там с выравниванием проблема
	INPUT_RECORD *r = (INPUT_RECORD*)calloc(sizeof(INPUT_RECORD),2);
	HANDLE hO = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD cr;
	DWORD dw, dwLen;
	int nPage = 0;
	BOOL lbNeedRedraw = TRUE;
	wchar_t* pszText = (wchar_t*)calloc(200*100,2);
	wchar_t* psz = NULL, *pszEnd = NULL;
	CESERVER_CHAR *pceChar = NULL;
	wchar_t* pszConData = NULL;
	WORD* pnConData = NULL;
	DWORD dwConDataBufSize = 0;
	CONSOLE_SCREEN_BUFFER_INFO sbi = {{0,0}};
	SetWindowText(FarHwnd, L"ConEmu packet dump");
	psz = pszText;

	switch(pReq->hdr.nCmd)
	{
			//case CECMD_GETSHORTINFO: pszEnd = L"CECMD_GETSHORTINFO"; break;
			//case CECMD_GETFULLINFO: pszEnd = L"CECMD_GETFULLINFO"; break;
		case CECMD_SETSIZESYNC: pszEnd = L"CECMD_SETSIZESYNC"; break;
		case CECMD_CMDSTARTSTOP: pszEnd = L"CECMD_CMDSTARTSTOP"; break;
//		case CECMD_GETGUIHWND: pszEnd = L"CECMD_GETGUIHWND"; break;
//		case CECMD_RECREATE: pszEnd = L"CECMD_RECREATE"; break;
		case CECMD_TABSCHANGED: pszEnd = L"CECMD_TABSCHANGED"; break;
		case CECMD_CMDSTARTED: pszEnd = L"CECMD_CMDSTARTED"; break;
		case CECMD_CMDFINISHED: pszEnd = L"CECMD_CMDFINISHED"; break;
		default: pszEnd = L"???";
	}

	wsprintf(psz, L"Packet size: %i;  Command: %i (%s);  Version: %i\n",
	         pReq->hdr.cbSize, pReq->hdr.nCmd, pszEnd, pReq->hdr.nVersion);
	psz += lstrlenW(psz);
	r->EventType = WINDOW_BUFFER_SIZE_EVENT;

	do
	{
		if (r->EventType == WINDOW_BUFFER_SIZE_EVENT)
		{
			r->EventType = 0; cr.X = 0; cr.Y = 0;
			lbNeedRedraw = TRUE;
		}
		else if (r->EventType == KEY_EVENT && r->Event.KeyEvent.bKeyDown)
		{
			if (r->Event.KeyEvent.wVirtualKeyCode == VK_NEXT)
			{
				lbNeedRedraw = TRUE;
				gnPage++;
				FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
			}
			else if (r->Event.KeyEvent.wVirtualKeyCode == VK_PRIOR)
			{
				lbNeedRedraw = TRUE;
				gnPage--;
			}
		}
		if (gnPage<0) gnPage = 1; else if (gnPage>1) gnPage = 0;

		if (lbNeedRedraw)
		{
			lbNeedRedraw = FALSE;
			cr.X = 0; cr.Y = 0;

			if (gnPage == 0)
			{
				FillConsoleOutputAttribute(hO, 7, csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);
				FillConsoleOutputCharacter(hO, L' ', csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);
				cr.X = 0; cr.Y = 0; psz = pszText;

				while(*psz && cr.Y<csbi.dwSize.Y)
				{
					pszEnd = wcschr(psz, L'\n');
					dwLen = std::min<int>((pszEnd-psz), csbi.dwSize.X);
					SetConsoleCursorPosition(hO, cr);

					if (dwLen>0)
						WriteConsoleOutputCharacter(hO, psz, dwLen, cr, &dw);

					cr.Y ++; psz = pszEnd + 1;
				}

				SetConsoleCursorPosition(hO, cr);
			}
			else if (gnPage == 1)
			{
				FillConsoleOutputAttribute(hO, 0x10, csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);
				FillConsoleOutputCharacter(hO, L' ', csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);
				int nMaxX = std::min(sbi.dwSize.X, csbi.dwSize.X);
				int nMaxY = std::min(sbi.dwSize.Y, csbi.dwSize.Y);

				if (pszConData && dwConDataBufSize)
				{
					wchar_t* pszSrc = pszConData;
					wchar_t* pszEnd = pszConData + dwConDataBufSize;
					WORD* pnSrc = pnConData;
					cr.X = 0; cr.Y = 0;

					while(cr.Y < nMaxY && pszSrc < pszEnd)
					{
						WriteConsoleOutputCharacter(hO, pszSrc, nMaxX, cr, &dw);
						WriteConsoleOutputAttribute(hO, pnSrc, nMaxX, cr, &dw);
						pszSrc += sbi.dwSize.X; pnSrc += sbi.dwSize.X; cr.Y++;
					}
				}

				cr.Y = nMaxY-1;
				SetConsoleCursorPosition(hO, cr);
			}
		}
	}
	while(CheckConInput(r));

	free(pszText);
}

void ShowConDump(wchar_t* pszText)
{
	INPUT_RECORD r[2];
	BOOL lbNeedRedraw = TRUE;
	HANDLE hO = GetStdHandle(STD_OUTPUT_HANDLE);
	//CONSOLE_SCREEN_BUFFER_INFO sbi = {{0,0}};
	COORD cr, crSize, crCursor;
	WCHAR* pszBuffers[3];
	void*  pnBuffers[3];
	WCHAR* pszDumpTitle, *pszRN, *pszSize, *pszTitle = NULL;
	SetWindowText(FarHwnd, L"ConEmu screen dump");
	pszDumpTitle = pszText;
	pszRN = wcschr(pszDumpTitle, L'\r');

	if (!pszRN) return;

	*pszRN = 0;
	pszSize = pszRN + 2;

	if (wcsncmp(pszSize, L"Size: ", 6)) return;

	pszRN = wcschr(pszSize, L'\r');

	if (!pszRN) return;

	pszBuffers[0] = pszRN + 2;
	pszSize += 6;
	//if ((pszRN = wcschr(pszSize, L'x'))==NULL) return;
	//*pszRN = 0;
	crSize.X = (SHORT)wcstol(pszSize, &pszRN, 10);

	if (!pszRN || *pszRN!=L'x') return;

	pszSize = pszRN + 1;
	//if ((pszRN = wcschr(pszSize, L'\r'))==NULL) return;
	//*pszRN = 0;
	crSize.Y = (SHORT)wcstol(pszSize, &pszRN, 10);

	if (!pszRN || (*pszRN!=L' ' && *pszRN!=L'\r')) return;

	pszSize = pszRN;
	crCursor.X = 0; crCursor.Y = crSize.Y-1;

	if (*pszSize == L' ')
	{
		while(*pszSize == L' ') pszSize++;

		if (wcsncmp(pszSize, L"Cursor: ", 8)==0)
		{
			pszSize += 8;
			cr.X = (SHORT)wcstol(pszSize, &pszRN, 10);

			if (!pszRN || *pszRN!=L'x') return;

			pszSize = pszRN + 1;
			cr.Y = (SHORT)wcstol(pszSize, &pszRN, 10);

			if (cr.X>=0 && cr.Y>=0)
			{
				crCursor.X = cr.X; crCursor.Y = cr.Y;
			}
		}
	}

	pszTitle = (WCHAR*)calloc(lstrlenW(pszDumpTitle)+200,2);
	DWORD dwConDataBufSize = crSize.X * crSize.Y;
	DWORD dwConDataBufSizeEx = crSize.X * crSize.Y * sizeof(CharAttr);
	pnBuffers[0] = (void*)(((WORD*)(pszBuffers[0])) + dwConDataBufSize);
	pszBuffers[1] = (WCHAR*)(((LPBYTE)(pnBuffers[0])) + dwConDataBufSizeEx);
	pnBuffers[1] = (void*)(((WORD*)(pszBuffers[1])) + dwConDataBufSize);
	pszBuffers[2] = (WCHAR*)(((LPBYTE)(pnBuffers[1])) + dwConDataBufSizeEx);
	pnBuffers[2] = (void*)(((WORD*)(pszBuffers[2])) + dwConDataBufSize);
	r->EventType = WINDOW_BUFFER_SIZE_EVENT;

	do
	{
		if (r->EventType == WINDOW_BUFFER_SIZE_EVENT)
		{
			r->EventType = 0;
			lbNeedRedraw = TRUE;
		}
		else if (r->EventType == KEY_EVENT && r->Event.KeyEvent.bKeyDown)
		{
			if (r->Event.KeyEvent.wVirtualKeyCode == VK_NEXT)
			{
				lbNeedRedraw = TRUE;
				gnPage++;
				FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
			}
			else if (r->Event.KeyEvent.wVirtualKeyCode == VK_PRIOR)
			{
				lbNeedRedraw = TRUE;
				gnPage--;
			}
			else if (r->Event.KeyEvent.uChar.UnicodeChar == L'*')
			{
				lbNeedRedraw = TRUE;
				gbShowAttrsOnly = !gbShowAttrsOnly;
			}
		}
		if (gnPage<0) gnPage = 3; else if (gnPage>3) gnPage = 0;

		if (lbNeedRedraw)
		{
			lbNeedRedraw = FALSE;
			cr.X = 0; cr.Y = 0;
			DWORD dw = 0;

			if (gnPage == 0)
			{
				FillConsoleOutputAttribute(hO, 7, csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);
				FillConsoleOutputCharacter(hO, L' ', csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);
				cr.X = 0; cr.Y = 0; SetConsoleCursorPosition(hO, cr);
				//wprintf(L"Console screen dump viewer\nTitle: %s\nSize: {%i x %i}\n",
				//	pszDumpTitle, crSize.X, crSize.Y);
				LPCWSTR psz = L"Console screen dump viewer";
				WriteConsoleOutputCharacter(hO, psz, lstrlenW(psz), cr, &dw); cr.Y++;
				psz = L"Title: ";
				WriteConsoleOutputCharacter(hO, psz, lstrlenW(psz), cr, &dw); cr.X += lstrlenW(psz);
				WriteConsoleOutputCharacter(hO, pszDumpTitle, lstrlenW(pszDumpTitle), cr, &dw); cr.X = 0; cr.Y++;
				wchar_t szSize[64]; wsprintf(szSize, L"Size: {%i x %i}", crSize.X, crSize.Y);
				WriteConsoleOutputCharacter(hO, szSize, lstrlenW(szSize), cr, &dw); cr.Y++;
				SetConsoleCursorPosition(hO, cr);
			}
			else if (gnPage >= 1 && gnPage <= 3)
			{
				FillConsoleOutputAttribute(hO, gbShowAttrsOnly ? 0xF : 0x10, csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);
				FillConsoleOutputCharacter(hO, L' ', csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);
				int nMaxX = std::min(crSize.X, csbi.dwSize.X);
				int nMaxY = std::min(crSize.Y, csbi.dwSize.Y);
				wchar_t* pszConData = pszBuffers[gnPage-1];
				void* pnConData = pnBuffers[gnPage-1];
				LPBYTE pnTemp = (LPBYTE)malloc(nMaxX*2);
				CharAttr *pSrcEx = (CharAttr*)pnConData;

				if (pszConData && dwConDataBufSize)
				{
					wchar_t* pszSrc = pszConData;
					wchar_t* pszEnd = pszConData + dwConDataBufSize;
					LPBYTE pnSrc;
					DWORD nAttrLineSize;

					if (gnPage == 3)
					{
						pnSrc = (LPBYTE)pnConData;
						nAttrLineSize = crSize.X * 2;
					}
					else
					{
						pnSrc = pnTemp;
						nAttrLineSize = 0;
					}

					cr.X = 0; cr.Y = 0;

					while(cr.Y < nMaxY && pszSrc < pszEnd)
					{
						if (!gbShowAttrsOnly)
						{
							WriteConsoleOutputCharacter(hO, pszSrc, nMaxX, cr, &dw);

							if (gnPage < 3)
							{
								for(int i = 0; i < nMaxX; i++)
								{
									((WORD*)pnSrc)[i] = MAKECONCOLOR(pSrcEx[i].nForeIdx, pSrcEx[i].nBackIdx);
								}

								pSrcEx += crSize.X;
							}

							WriteConsoleOutputAttribute(hO, (WORD*)pnSrc, nMaxX, cr, &dw);
						}
						else
						{
							WriteConsoleOutputCharacter(hO, (wchar_t*)pnSrc, nMaxX, cr, &dw);
						}

						pszSrc += crSize.X;

						if (nAttrLineSize)
							pnSrc += nAttrLineSize; //-V102

						cr.Y++;
					}
				}

				free(pnTemp);
				//cr.Y = nMaxY-1;
				SetConsoleCursorPosition(hO, crCursor);
			}
		}
	}
	while(CheckConInput(r));

	free(pszTitle);
}


