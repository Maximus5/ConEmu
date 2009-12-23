#include <windows.h>
#include <wchar.h>
#include "..\common\common.hpp"
#include "..\common\pluginW1007.hpp"
#include "PluginHeader.h"

// Можно бы добавить обработку Up/Down для перехода между пакетами
//  сразу "жать" Esc, вызвать RestoreScreen (но без коммита на экран)
//  и послать макрос "Down Enter" - фар сделает остальное
//
// Кнопочкой '*' при отображении дампа переключать режим
//  = нормальный (отображается цветной текст, как был в консоли)
//  = атрибутивный (отображаются ТОЛЬКО атрибуты, но как текст, а не как цвет)

extern struct PluginStartupInfo *InfoW995;
extern struct FarStandardFunctions *FSFW995;

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

	if (pr->EventType == KEY_EVENT) {
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
	switch (pReq->hdr.nCmd) {
		//case CECMD_GETSHORTINFO: pszEnd = L"CECMD_GETSHORTINFO"; break;
		//case CECMD_GETFULLINFO: pszEnd = L"CECMD_GETFULLINFO"; break;
		case CECMD_SETSIZE: pszEnd = L"CECMD_SETSIZE"; break;
		case CECMD_CMDSTARTSTOP: pszEnd = L"CECMD_CMDSTARTSTOP"; break;
		case CECMD_GETGUIHWND: pszEnd = L"CECMD_GETGUIHWND"; break;
//		case CECMD_RECREATE: pszEnd = L"CECMD_RECREATE"; break;
		case CECMD_TABSCHANGED: pszEnd = L"CECMD_TABSCHANGED"; break;
		case CECMD_CMDSTARTED: pszEnd = L"CECMD_CMDSTARTED"; break;
		case CECMD_CMDFINISHED: pszEnd = L"CECMD_CMDFINISHED"; break;
		default: pszEnd = L"???";
	}
	wsprintf(psz, L"Packet size: %i;  Command: %i (%s);  Version: %i\n",
		pReq->hdr.nSize, pReq->hdr.nCmd, pszEnd, pReq->hdr.nVersion);
	psz += lstrlenW(psz);

	//if (/*pReq->hdr.nCmd == CECMD_GETSHORTINFO ||*/ pReq->hdr.nCmd == CECMD_GETFULLINFO) {
	//	lstrcpyW(psz, L"\n"); psz ++;
	//	//LPBYTE ptr = pReq->Data;
	//	// 1
	//	wsprintf(psz, L"ConHwnd:    0x%08X\n", (DWORD)pReq->ConInfo.inf.hConWnd); psz += lstrlenW(psz);
	//	// 2 - GetTickCount последнего чтения
	//	wsprintf(psz, L"PacketID: %i\n", pReq->ConInfo.inf.nPacketId); psz += lstrlenW(psz);
	//	// 3
	//	lstrcpyW(psz, L"Processes:  {"); psz += lstrlenW(psz);
	//	//if (dw) { lstrcpyW(psz, L" {"); psz += lstrlenW(psz); }
	//	for (UINT n=0; n<countof(pReq->ConInfo.inf.nProcesses); n++) {
	//		if (pReq->ConInfo.inf.nProcesses[n] == 0) break;
	//		if (n) { lstrcpyW(psz, L","); psz++; }
	//		wsprintf(psz, L"%i", pReq->ConInfo.inf.nProcesses[n]); psz += lstrlenW(psz);
	//	}
	//	lstrcpyW(psz, L"}\n"); psz += lstrlenW(psz);
	//	// 4
	//	CONSOLE_CURSOR_INFO ci = {0};
	//	dw = pReq->ConInfo.inf.dwCiSize;
	//	if (dw>0) {
	//		memmove(&ci, &pReq->ConInfo.inf.ci, min(dw,sizeof(ci)));
	//		wsprintf(psz, L"CursorInf:  size=%i, visible=%i\n", ci.dwSize, ci.bVisible); psz += lstrlenW(psz);
	//	}
	//	// 5, 6, 7
	//	wsprintf(psz, L"ConsoleCP:  %i\n", pReq->ConInfo.inf.dwConsoleCP); psz += lstrlenW(psz);
	//	wsprintf(psz, L"OutputCP:   %i\n", pReq->ConInfo.inf.dwConsoleOutputCP); psz += lstrlenW(psz);
	//	wsprintf(psz, L"ConMode:    0x%08X\n", pReq->ConInfo.inf.dwConsoleMode); psz += lstrlenW(psz);
	//	// 8
	//	dw = pReq->ConInfo.inf.dwSbiSize;
	//	if (dw>0) {
	//		memmove(&sbi, &pReq->ConInfo.inf.sbi, min(dw,sizeof(sbi)));
	//		lstrcpyW(psz, L"\nConsole window layout\n"); psz += lstrlenW(psz);
	//		wsprintf(psz, L"  BufferSize: {%i x %i}%\n", sbi.dwSize.X, sbi.dwSize.Y); psz += lstrlenW(psz);
	//		wsprintf(psz, L"  CursorPos:  {%i x %i}%\n", sbi.dwCursorPosition.X, sbi.dwCursorPosition.Y); psz += lstrlenW(psz);
	//		wsprintf(psz, L"  MaxWndSize: {%i x %i}%\n", sbi.dwMaximumWindowSize.X, sbi.dwMaximumWindowSize.Y); psz += lstrlenW(psz);
	//		wsprintf(psz, L"  WindowSize: {L=%i, T=%i, R=%i, B=%i}\n", sbi.srWindow.Left, sbi.srWindow.Top, sbi.srWindow.Right, sbi.srWindow.Bottom); psz += lstrlenW(psz);
	//		lstrcpyW(psz, L"\n"); psz += lstrlenW(psz);
	//	}
	//	// 9
	//	dw = pReq->ConInfo.dwRgnInfoSize;
	//	if (dw>0) {
	//		if (dw >= sizeof(CESERVER_CHAR)) {
	//			pceChar = &pReq->ConInfo.RgnInfo.RgnInfo;
	//			lstrcpyW(psz, L"\nConsole region changes\n"); psz += lstrlenW(psz);
	//			sbi.dwSize.X = (pceChar->hdr.cr2.X-pceChar->hdr.cr1.X+1);
	//			sbi.dwSize.Y = (pceChar->hdr.cr2.Y-pceChar->hdr.cr1.Y+1);
	//			dwConDataBufSize = sbi.dwSize.X*sbi.dwSize.Y;
	//			wsprintf(psz, L"  Region:    {L=%i, T=%i, R=%i, B=%i}  {%i x %i}\n",
	//				pceChar->hdr.cr1.X, pceChar->hdr.cr1.Y, pceChar->hdr.cr2.X, pceChar->hdr.cr2.Y,
	//				sbi.dwSize.X, sbi.dwSize.Y); psz += lstrlenW(psz);
	//			wsprintf(psz, L"  FirstChar: '%c'\n", pceChar->data[0]); psz += lstrlenW(psz);
	//			lstrcpyW(psz, L"Press 'PgDn' to display it\n"); psz += lstrlenW(psz);
	//			pszConData = (wchar_t*)pceChar->data;
	//			pnConData = (WORD*)(pszConData+dwConDataBufSize);
	//		} else {
	//			pceChar = NULL;
	//			wsprintf(psz, L"\nInvalid length of CESERVER_CHAR (%i)\n", dw); psz += lstrlenW(psz);
	//		}
	//	} else {
	//		// 10
	//		dw = pReq->ConInfo.FullData.dwOneBufferSize;
	//		if (dw != 0) {
	//			dwConDataBufSize = dw;
	//			dw = sbi.dwSize.X*sbi.dwSize.Y*2;
	//			wsprintf(psz, L"Full console dump. Size: %i %s %i*%i*2\n", 
	//				dwConDataBufSize, (dw==dwConDataBufSize) ? L"==" : L"<>", sbi.dwSize.X, sbi.dwSize.Y); 
	//			psz += lstrlenW(psz);
	//			lstrcpyW(psz, L"Press 'PgDn' to display it\n"); psz += lstrlenW(psz);
	//			pszConData = (wchar_t*)pReq->ConInfo.FullData.Data;
	//			pnConData = (WORD*)((LPBYTE)pszConData)+dwConDataBufSize;
	//		}
	//	}
	//} else {
	//	int nMax = min(((UINT)(csbi.dwSize.X/4)),(pReq->hdr.nSize-sizeof(CESERVER_REQ_HDR)));
	//	lstrcpyW(psz, L"\n\nPacket data:\n"); psz += lstrlenW(psz);
	//	for (int i=0; i<nMax; i++) {
	//		wsprintf(psz, L"%02X ", (BYTE)pReq->Data[i]); psz += 3;
	//	}
	//	lstrcpyW(psz, L"\n");
	//}

	r->EventType = WINDOW_BUFFER_SIZE_EVENT;

	do {
		if (r->EventType == WINDOW_BUFFER_SIZE_EVENT) {
			r->EventType = 0; cr.X = 0; cr.Y = 0;
			lbNeedRedraw = TRUE;
		} else if (r->EventType == KEY_EVENT && r->Event.KeyEvent.bKeyDown) {
			if (r->Event.KeyEvent.wVirtualKeyCode == VK_NEXT) {
				lbNeedRedraw = TRUE;
				gnPage++;
				FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
			} else if (r->Event.KeyEvent.wVirtualKeyCode == VK_PRIOR) {
				lbNeedRedraw = TRUE;
				gnPage--;
			}
		}
		if (gnPage<0) gnPage = 1; else if (gnPage>1) gnPage = 0;

		if (lbNeedRedraw) {
			lbNeedRedraw = FALSE;
			cr.X = 0; cr.Y = 0;
			if (gnPage == 0) {
				FillConsoleOutputAttribute(hO, 7, csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);
				FillConsoleOutputCharacter(hO, L' ', csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);
				cr.X = 0; cr.Y = 0; psz = pszText;
				while (*psz && cr.Y<csbi.dwSize.Y) {
					pszEnd = wcschr(psz, L'\n');
					dwLen = min(((int)(pszEnd-psz)),csbi.dwSize.X);
					SetConsoleCursorPosition(hO, cr);
					if (dwLen>0)
						WriteConsoleOutputCharacter(hO, psz, dwLen, cr, &dw);
					cr.Y ++; psz = pszEnd + 1;
				}
				SetConsoleCursorPosition(hO, cr);
			} else if (gnPage == 1) {
				FillConsoleOutputAttribute(hO, 0x10, csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);
				FillConsoleOutputCharacter(hO, L' ', csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);

				int nMaxX = min(sbi.dwSize.X, csbi.dwSize.X);
				int nMaxY = min(sbi.dwSize.Y, csbi.dwSize.Y);
				if (pszConData && dwConDataBufSize) {
					wchar_t* pszSrc = pszConData;
					wchar_t* pszEnd = pszConData + dwConDataBufSize;
					WORD* pnSrc = pnConData;
					cr.X = 0; cr.Y = 0;
					while (cr.Y < nMaxY && pszSrc < pszEnd) {
						WriteConsoleOutputCharacter(hO, pszSrc, nMaxX, cr, &dw);
						WriteConsoleOutputAttribute(hO, pnSrc, nMaxX, cr, &dw);

						pszSrc += sbi.dwSize.X; pnSrc += sbi.dwSize.X; cr.Y++;
					}
				}
				cr.Y = nMaxY-1;
				SetConsoleCursorPosition(hO, cr);
			}
		}

	} while (CheckConInput(r));

	free(pszText);
}

void ShowConDump(wchar_t* pszText)
{
	INPUT_RECORD r[2];
	BOOL lbNeedRedraw = TRUE;
	HANDLE hO = GetStdHandle(STD_OUTPUT_HANDLE);
	//CONSOLE_SCREEN_BUFFER_INFO sbi = {{0,0}};
	COORD cr, crSize;
	WCHAR* pszBuffers[3];
	WORD*  pnBuffers[3];
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
	if ((pszRN = wcschr(pszSize, L'x'))==NULL) return;
	*pszRN = 0;
	crSize.X = (SHORT)wcstol(pszSize, &pszRN, 10);
	pszSize = pszRN + 1;
	if ((pszRN = wcschr(pszSize, L'\r'))==NULL) return;
	*pszRN = 0;
	crSize.Y = (SHORT)wcstol(pszSize, &pszRN, 10);

	pszTitle = (WCHAR*)calloc(lstrlenW(pszDumpTitle)+200,2);

	DWORD dwConDataBufSize = crSize.X * crSize.Y;
	pnBuffers[0] = ((WORD*)(pszBuffers[0])) + dwConDataBufSize;
	pszBuffers[1] = ((WCHAR*)(pnBuffers[0])) + dwConDataBufSize;
	pnBuffers[1] = ((WORD*)(pszBuffers[1])) + dwConDataBufSize;
	pszBuffers[2] = ((WCHAR*)(pnBuffers[1])) + dwConDataBufSize;
	pnBuffers[2] = ((WORD*)(pszBuffers[2])) + dwConDataBufSize;

	r->EventType = WINDOW_BUFFER_SIZE_EVENT;

	do {
		if (r->EventType == WINDOW_BUFFER_SIZE_EVENT) {
			r->EventType = 0;
			lbNeedRedraw = TRUE;
		} else if (r->EventType == KEY_EVENT && r->Event.KeyEvent.bKeyDown) {
			if (r->Event.KeyEvent.wVirtualKeyCode == VK_NEXT) {
				lbNeedRedraw = TRUE;
				gnPage++;
				FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
			} else if (r->Event.KeyEvent.wVirtualKeyCode == VK_PRIOR) {
				lbNeedRedraw = TRUE;
				gnPage--;
			} else if (r->Event.KeyEvent.uChar.UnicodeChar == L'*') {
				lbNeedRedraw = TRUE;
				gbShowAttrsOnly = !gbShowAttrsOnly;
			}
		}
		if (gnPage<0) gnPage = 3; else if (gnPage>3) gnPage = 0;

		if (lbNeedRedraw) {
			lbNeedRedraw = FALSE;
			cr.X = 0; cr.Y = 0;
			DWORD dw = 0;
			if (gnPage == 0) {
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
				
			} else if (gnPage >= 1 && gnPage <= 3) {
				FillConsoleOutputAttribute(hO, gbShowAttrsOnly ? 0xF : 0x10, csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);
				FillConsoleOutputCharacter(hO, L' ', csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);

				int nMaxX = min(crSize.X, csbi.dwSize.X);
				int nMaxY = min(crSize.Y, csbi.dwSize.Y);
				wchar_t* pszConData = pszBuffers[gnPage-1];
				WORD* pnConData = pnBuffers[gnPage-1];
				if (pszConData && dwConDataBufSize) {
					wchar_t* pszSrc = pszConData;
					wchar_t* pszEnd = pszConData + dwConDataBufSize;
					WORD* pnSrc = pnConData;
					cr.X = 0; cr.Y = 0;
					while (cr.Y < nMaxY && pszSrc < pszEnd) {
						if (!gbShowAttrsOnly) {
							WriteConsoleOutputCharacter(hO, pszSrc, nMaxX, cr, &dw);
							WriteConsoleOutputAttribute(hO, pnSrc, nMaxX, cr, &dw);
						} else {
							WriteConsoleOutputCharacter(hO, (wchar_t*)pnSrc, nMaxX, cr, &dw);
						}

						pszSrc += crSize.X; pnSrc += crSize.X; cr.Y++;
					}
				}
				cr.Y = nMaxY-1;
				SetConsoleCursorPosition(hO, cr);
			}
		}

	} while (CheckConInput(r));

	free(pszTitle);
}


HANDLE WINAPI OpenFilePluginW(const wchar_t *Name,const unsigned char *Data,int DataSize,int OpMode)
{
	//Name==NULL, когда Shift-F1
	if (!InfoW995) return INVALID_HANDLE_VALUE;
	if (OpMode || Name == NULL) return INVALID_HANDLE_VALUE; // только из панелей в обычном режиме
	const wchar_t* pszDot = wcsrchr(Name, L'.');
	if (!pszDot) return INVALID_HANDLE_VALUE;
	if (lstrcmpi(pszDot, L".con")) return INVALID_HANDLE_VALUE;
	if (DataSize < sizeof(CESERVER_REQ_HDR)) return INVALID_HANDLE_VALUE;

	HANDLE hFile = CreateFile(Name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		InfoW995->Message(InfoW995->ModuleNumber, FMSG_ALLINONE|FMSG_MB_OK|FMSG_WARNING,
			NULL, (const wchar_t* const*)L"ConEmu plugin\nCan't open file!", 0,0);
		return INVALID_HANDLE_VALUE;
	}

	DWORD dwSizeLow, dwSizeHigh = 0;
	dwSizeLow = GetFileSize(hFile, &dwSizeHigh);
	if (dwSizeHigh) {
		InfoW995->Message(InfoW995->ModuleNumber, FMSG_ALLINONE|FMSG_MB_OK|FMSG_WARNING,
			NULL, (const wchar_t* const*)L"ConEmu plugin\nFile too large!", 0,0);
		CloseHandle(hFile);
		return INVALID_HANDLE_VALUE;
	}

	wchar_t* pszData = (wchar_t*)calloc(dwSizeLow+2,1);
	if (!pszData) return INVALID_HANDLE_VALUE;
	if (!ReadFile(hFile, pszData, dwSizeLow, &dwSizeHigh, 0) || dwSizeHigh != dwSizeLow) {
		InfoW995->Message(InfoW995->ModuleNumber, FMSG_ALLINONE|FMSG_MB_OK|FMSG_WARNING,
			NULL, (const wchar_t* const*)L"ConEmu plugin\nCan't read file!", 0,0);
		return INVALID_HANDLE_VALUE;
	}
	CloseHandle(hFile);

	memset(&csbi, 0, sizeof(csbi));
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

	HANDLE h = InfoW995->SaveScreen(0,0,-1,-1);

	CESERVER_REQ* pReq = (CESERVER_REQ*)pszData;
	if (pReq->hdr.nSize == dwSizeLow)
	{
		if (pReq->hdr.nVersion != CESERVER_REQ_VER && pReq->hdr.nVersion < 40) {
			InfoW995->Message(InfoW995->ModuleNumber, FMSG_ALLINONE|FMSG_MB_OK|FMSG_WARNING,
				NULL, (const wchar_t* const*)L"ConEmu plugin\nUnknown version of packet", 0,0);
		} else {
			ShowConPacket(pReq);
		}
	} else if ((*(DWORD*)pszData) >= 0x200020) {
		ShowConDump(pszData);
	}

	InfoW995->RestoreScreen(NULL);
	InfoW995->RestoreScreen(h);
	InfoW995->Text(0,0,0,0);

	//InfoW995->Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0); 
	//InfoW995->Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0); 
	//INPUT_RECORD r = {WINDOW_BUFFER_SIZE_EVENT};
	//r.Event.WindowBufferSizeEvent.dwSize = csbi.dwSize;
	//WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwSizeLow);

	free(pszData);
	return INVALID_HANDLE_VALUE;
}
