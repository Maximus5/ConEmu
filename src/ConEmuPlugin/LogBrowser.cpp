#include <windows.h>
#include "..\common\common.hpp"
#include "..\common\pluginW789.hpp"
#include "PluginHeader.h"


extern struct PluginStartupInfo *InfoW789;
extern struct FarStandardFunctions *FSFW789;

CONSOLE_SCREEN_BUFFER_INFO csbi;
DWORD gdwPage = 0;

BOOL CheckConInput(INPUT_RECORD* pr)
{
	DWORD dwCount = 0;
	BOOL lbRc = ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), pr, 1, &dwCount);
	if (!lbRc)
		return FALSE;

	if (pr->EventType == KEY_EVENT) {
		if (pr->Event.KeyEvent.wVirtualScanCode == VK_ESCAPE)
			return FALSE;
	}

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	return TRUE;
}

void ShowConPacket(CESERVER_REQ* pReq)
{
	INPUT_RECORD r[2] = {WINDOW_BUFFER_SIZE_EVENT};
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
	switch (pReq->nCmd) {
		case CECMD_GETSHORTINFO: pszEnd = L"CECMD_GETSHORTINFO"; break;
		case CECMD_GETFULLINFO: pszEnd = L"CECMD_GETFULLINFO"; break;
		case CECMD_SETSIZE: pszEnd = L"CECMD_SETSIZE"; break;
		case CECMD_CMDSTARTSTOP: pszEnd = L"CECMD_CMDSTARTSTOP"; break;
		case CECMD_GETGUIHWND: pszEnd = L"CECMD_GETGUIHWND"; break;
		case CECMD_RECREATE: pszEnd = L"CECMD_RECREATE"; break;
		case CECMD_TABSCHANGED: pszEnd = L"CECMD_TABSCHANGED"; break;
		default: pszEnd = L"???";
	}
	wsprintf(psz, L"Packet size: %i;  Command: %i (%s);  Version: %i\n",
		pReq->nSize, pReq->nCmd, pszEnd, pReq->nVersion);
	psz += wcslen(psz);

	if (pReq->nCmd == CECMD_GETSHORTINFO || pReq->nCmd == CECMD_GETFULLINFO) {
		lstrcpyW(psz, L"\n"); psz ++;
		LPBYTE ptr = pReq->Data;
		// 1
		wsprintf(psz, L"ConHwnd:    0x%08X\n", *(DWORD*)ptr); ptr += 4; psz += wcslen(psz);
		// 2 - GetTickCount последнего чтения
		wsprintf(psz, L"PacketTick: %i\n", *(DWORD*)ptr); ptr += 4; psz += wcslen(psz);
		// 3
		dw = *(DWORD*)ptr;
		wsprintf(psz, L"Processes:  %i", dw); ptr += 4; psz += wcslen(psz);
		if (dw) { lstrcpyW(psz, L" {"); psz += wcslen(psz); }
		for (UINT n=0; n<dw; n++) {
			if (n) { lstrcpyW(psz, L","); psz++; }
			wsprintf(psz, L"%i", *(DWORD*)ptr); ptr += 4; psz += wcslen(psz);
		}
		lstrcpyW(psz, dw ? L"}\n" : L"\n"); psz += wcslen(psz);
		// 4
		CONSOLE_SELECTION_INFO sel = {0};
		dw = *(DWORD*)ptr; ptr += 4;
		if (dw>0) {
			memmove(&sel, ptr, min(dw,sizeof(sel))); ptr += dw;
		}
		// 5
		CONSOLE_CURSOR_INFO ci = {0};
		dw = *(DWORD*)ptr; ptr += 4;
		if (dw>0) {
			memmove(&ci, ptr, min(dw,sizeof(ci))); ptr += dw;
			wsprintf(psz, L"CursorInf:  size=%i, visible=%i\n", ci.dwSize, ci.bVisible); psz += wcslen(psz);
		}
		// 6, 7, 8
		wsprintf(psz, L"ConsoleCP:  %i\n", *(DWORD*)ptr); ptr += 4; psz += wcslen(psz);
		wsprintf(psz, L"OutputCP:   %i\n", *(DWORD*)ptr); ptr += 4; psz += wcslen(psz);
		wsprintf(psz, L"ConMode:    0x%08X\n", *(DWORD*)ptr); ptr += 4; psz += wcslen(psz);
		// 9
		dw = *(DWORD*)ptr; ptr += 4;
		if (dw>0) {
			memmove(&sbi, ptr, min(dw,sizeof(sbi))); ptr += dw;
			lstrcpyW(psz, L"\nConsole window layout\n"); psz += wcslen(psz);
			wsprintf(psz, L"  BufferSize: {%i x %i}%\n", sbi.dwSize.X, sbi.dwSize.Y); psz += wcslen(psz);
			wsprintf(psz, L"  CursorPos:  {%i x %i}%\n", sbi.dwCursorPosition.X, sbi.dwCursorPosition.Y); psz += wcslen(psz);
			wsprintf(psz, L"  MaxWndSize: {%i x %i}%\n", sbi.dwMaximumWindowSize.X, sbi.dwMaximumWindowSize.Y); psz += wcslen(psz);
			wsprintf(psz, L"  WindowSize: {L=%i, T=%i, R=%i, B=%i}\n", sbi.srWindow.Left, sbi.srWindow.Top, sbi.srWindow.Right, sbi.srWindow.Bottom); psz += wcslen(psz);
			lstrcpyW(psz, L"\n"); psz += wcslen(psz);
		}
		// 10
		dw = *(DWORD*)ptr; ptr += 4;
		if (dw>0) {
			if (dw >= sizeof(CESERVER_CHAR)) {
				pceChar = (CESERVER_CHAR*)ptr;
				lstrcpyW(psz, L"\nConsole region changes\n"); psz += wcslen(psz);
				wsprintf(psz, L"  Region:   {L=%i, T=%i, R=%i, B=%i}\n", pceChar->crStart.X, pceChar->crStart.Y, pceChar->crEnd.X, pceChar->crEnd.Y);
				wsprintf(psz, L"  NewChar:  '%c'\n", pceChar->data[0]);
			} else {
				pceChar = NULL;
				wsprintf(psz, L"\nInvalid length of CESERVER_CHAR (%i)\n", dw); psz += wcslen(psz);
			}
			ptr += dw;
		}
		// 11
		dw = *(DWORD*)ptr; ptr += 4;
		if (dw != 0) {
			dwConDataBufSize = dw;
			dw = sbi.dwSize.X*sbi.dwSize.Y*2;
			wsprintf(psz, L"Full console dump. Size: %i %s %i*%i*2\n", 
				dwConDataBufSize, (dw==dwConDataBufSize) ? L"==" : L"<>", sbi.dwSize.X, sbi.dwSize.Y); 
			psz += wcslen(psz);
			lstrcpyW(psz, L"Press 'PgDn' to display it\n"); psz += wcslen(psz);
			pszConData = (wchar_t*)ptr; ptr += dwConDataBufSize;
			pnConData = (WORD*)ptr;
		}
	} else {
		int nMax = min(((UINT)(csbi.dwSize.X/4)),(pReq->nSize-12));
		lstrcpyW(psz, L"\n\nPacket data:\n"); psz += wcslen(psz);
		for (int i=0; i<nMax; i++) {
			wsprintf(psz, L"%02X ", (BYTE)pReq->Data[i]); psz += 3;
		}
		lstrcpyW(psz, L"\n");
	}

	do {
		if (r->EventType == WINDOW_BUFFER_SIZE_EVENT) {
			r->EventType = 0; cr.X = 0; cr.Y = 0;
			lbNeedRedraw = TRUE;
		} else if (r->EventType == KEY_EVENT && r->Event.KeyEvent.bKeyDown) {
			if (r->Event.KeyEvent.wVirtualScanCode == VK_NEXT) {
				lbNeedRedraw = gdwPage!=1; gdwPage = 1;
			} else if (r->Event.KeyEvent.wVirtualScanCode == VK_PRIOR) {
				lbNeedRedraw = gdwPage!=0; gdwPage = 0;
			}
		}

		if (lbNeedRedraw) {
			lbNeedRedraw = FALSE;
			cr.X = 0; cr.Y = 0;
			if (gdwPage == 0) {
				FillConsoleOutputAttribute(hO, 7, csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);
				FillConsoleOutputCharacter(hO, L' ', csbi.dwSize.X*csbi.dwSize.Y, cr, &dw);
				cr.X = 0; cr.Y = 0; psz = pszText;
				while (*psz && cr.Y<csbi.dwSize.Y) {
					pszEnd = wcschr(psz, L'\n');
					dwLen = min((pszEnd-psz),csbi.dwSize.X);
					SetConsoleCursorPosition(hO, cr);
					if (dwLen>0)
						WriteConsoleOutputCharacter(hO, psz, dwLen, cr, &dw);
					cr.Y ++; psz = pszEnd + 1;
				}
				SetConsoleCursorPosition(hO, cr);
			} else if (gdwPage == 1) {
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
	INPUT_RECORD r = {0};

	
}


HANDLE WINAPI OpenFilePluginW(const wchar_t *Name,const unsigned char *Data,int DataSize,int OpMode)
{
	if (OpMode) return INVALID_HANDLE_VALUE; // только из панелей в обычном режиме
	const wchar_t* pszDot = wcsrchr(Name, L'.');
	if (!pszDot || lstrcmpi(pszDot, L".con")) return INVALID_HANDLE_VALUE;
	if (DataSize < 12) return INVALID_HANDLE_VALUE;

	HANDLE hFile = CreateFile(Name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		InfoW789->Message(InfoW789->ModuleNumber, FMSG_ALLINONE|FMSG_MB_OK|FMSG_WARNING,
			NULL, (const wchar_t* const*)L"ConEmu plugin\nCan't open file!", 0,0);
		return INVALID_HANDLE_VALUE;
	}

	DWORD dwSizeLow, dwSizeHigh = 0;
	dwSizeLow = GetFileSize(hFile, &dwSizeHigh);
	if (dwSizeHigh) {
		InfoW789->Message(InfoW789->ModuleNumber, FMSG_ALLINONE|FMSG_MB_OK|FMSG_WARNING,
			NULL, (const wchar_t* const*)L"ConEmu plugin\nFile too large!", 0,0);
		CloseHandle(hFile);
		return INVALID_HANDLE_VALUE;
	}

	wchar_t* pszData = (wchar_t*)calloc(dwSizeLow+2,1);
	if (!pszData) return INVALID_HANDLE_VALUE;
	if (!ReadFile(hFile, pszData, dwSizeLow, &dwSizeHigh, 0) || dwSizeHigh != dwSizeLow) {
		InfoW789->Message(InfoW789->ModuleNumber, FMSG_ALLINONE|FMSG_MB_OK|FMSG_WARNING,
			NULL, (const wchar_t* const*)L"ConEmu plugin\nCan't read file!", 0,0);
		return INVALID_HANDLE_VALUE;
	}
	CloseHandle(hFile);

	memset(&csbi, 0, sizeof(csbi));
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

	HANDLE h = InfoW789->SaveScreen(0,0,-1,-1);

	CESERVER_REQ* pReq = (CESERVER_REQ*)pszData;
	if (pReq->nSize == dwSizeLow)
	{
		if (pReq->nVersion != CESERVER_REQ_VER && pReq->nVersion < 20) {
			InfoW789->Message(InfoW789->ModuleNumber, FMSG_ALLINONE|FMSG_MB_OK|FMSG_WARNING,
				NULL, (const wchar_t* const*)L"ConEmu plugin\nUnknown version of packet", 0,0);
		} else {
			ShowConPacket(pReq);
		}
	} else if ((*(DWORD*)pszData) >= 0x200020) {
		ShowConDump(pszData);
	}

	InfoW789->RestoreScreen(NULL);
	InfoW789->RestoreScreen(h);
	InfoW789->Text(0,0,0,0);

	//InfoW789->Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0); 
	//InfoW789->Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0); 
	//INPUT_RECORD r = {WINDOW_BUFFER_SIZE_EVENT};
	//r.Event.WindowBufferSizeEvent.dwSize = csbi.dwSize;
	//WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwSizeLow);

	free(pszData);
	return INVALID_HANDLE_VALUE;
}
