
//TODO: Нотация. 
//TODO: XXX       - вернуть значение переменной, возможно скорректировав ее
//TODO: GetXXX    - вернуть значение переменной
//TODO: SetXXX    - установить значение переменной, команда в консоль не послыается!
//TODO: ChangeXXX - послать в консоль и установить значение переменной

/*
Copyright (c) 2009-2014 Maximus5
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
//#define ALLOWUSEFARSYNCHRO

#include "Header.h"
#include <Tlhelp32.h>

#include "../common/ConEmuCheck.h"
#include "../common/Execute.h"
#include "../common/RgnDetect.h"
#include "../common/WinConsole.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "ConEmuPipe.h"
#include "FindDlg.h"
#include "HtmlCopy.h"
#include "Macro.h"
#include "Menu.h"
#include "RealBuffer.h"
#include "RealConsole.h"
#include "Status.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

#define DEBUGSTRSIZE(s) //DEBUGSTR(s)
#define DEBUGSTRPKT(s) //DEBUGSTR(s)
#define DEBUGSTRCURSORPOS(s) //DEBUGSTR(s)
#define DEBUGSTRMOUSE(s) //DEBUGSTR(s)

// ANSI, without "\r\n"
#define IFLOGCONSOLECHANGE gpSetCls->isAdvLogging>=2
#define LOGCONSOLECHANGE(s) if (IFLOGCONSOLECHANGE) mp_RCon->LogString(s, TRUE)

#ifndef CONSOLE_MOUSE_DOWN
#define CONSOLE_MOUSE_DOWN 8
#endif

#define SELMOUSEAUTOSCROLLDELTA 25
#define SELMOUSEAUTOSCROLLPIX   2

#define Free SafeFree
#define Alloc calloc

#ifdef _DEBUG
#define HEAPVAL //MCHKHEAP
#else
#define HEAPVAL
#endif

//#define Assert(V) if ((V)==FALSE) { wchar_t szAMsg[MAX_PATH*2]; _wsprintf(szAMsg, SKIPLEN(countof(szAMsg)) L"Assertion (%s) at\n%s:%i", _T(#V), _T(__FILE__), __LINE__); CRealConsole::Box(szAMsg); }

// Character substitues (ASCII 0..31)
const wchar_t gszAnalogues[32] =
{
	32, 9786, 9787, 9829, 9830, 9827, 9824, 8226, 9688, 9675, 9689, 9794, 9792, 9834, 9835, 9788,
	9658, 9668, 8597, 8252,  182,  167, 9632, 8616, 8593, 8595, 8594, 8592, 8735, 8596, 9650, 9660
};

const DWORD gnKeyBarFlags[] = {0,
	LEFT_CTRL_PRESSED, LEFT_ALT_PRESSED, SHIFT_PRESSED,
	LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED, LEFT_CTRL_PRESSED|SHIFT_PRESSED,
	LEFT_ALT_PRESSED|SHIFT_PRESSED,
	RIGHT_CTRL_PRESSED, RIGHT_ALT_PRESSED,
	RIGHT_CTRL_PRESSED|RIGHT_ALT_PRESSED, RIGHT_CTRL_PRESSED|SHIFT_PRESSED,
	RIGHT_ALT_PRESSED|SHIFT_PRESSED
};


CRealBuffer::CRealBuffer(CRealConsole* apRCon, RealBufferType aType/*=rbt_Primary*/)
{
	mp_RCon = apRCon;
	m_Type = aType;
	mn_LastRgnFlags = -1;
	
	ZeroStruct(con);
	con.hInSetSize = CreateEvent(0,TRUE,TRUE,0);
	
	mb_BuferModeChangeLocked = FALSE;
	mcr_LastMousePos = MakeCoord(-1,-1);

	RECT rcNil = MakeRect(-1,-1);
	mr_LeftPanel = rcNil;
	mr_LeftPanelFull = rcNil;
	mr_RightPanel = rcNil;
	mr_RightPanel = rcNil;

	mb_LeftPanel = mb_RightPanel = FALSE;
	
	ZeroStruct(dump);

	InitializeCriticalSection(&m_TrueMode.csLock);
	m_TrueMode.mp_Cmp = NULL;
	m_TrueMode.nCmpMax = 0;
}

CRealBuffer::~CRealBuffer()
{
	Assert(con.bInGetConsoleData==FALSE);

	if (con.pConChar)
		{ Free(con.pConChar); con.pConChar = NULL; }

	if (con.pConAttr)
		{ Free(con.pConAttr); con.pConAttr = NULL; }

	if (con.hInSetSize)
		{ CloseHandle(con.hInSetSize); con.hInSetSize = NULL; }

	dump.Close();

	DeleteCriticalSection(&m_TrueMode.csLock);
	SafeFree(m_TrueMode.mp_Cmp);
}

void CRealBuffer::ReleaseMem()
{
	TODO("Освободить не используемую память");
	m_Type = rbt_Undefined;
	dump.Close();
}

// Вызывается после CVirtualConsole::Dump и CRealConsole::DumpConsole
// Это последний, третий блок в файле дампа
void CRealBuffer::DumpConsole(HANDLE ahFile)
{
	BOOL lbRc = FALSE;
	DWORD dw = 0;

	if (con.pConChar && con.pConAttr)
	{
		MSectionLock sc; sc.Lock(&csCON, FALSE);
		DWORD nSize = con.nTextWidth * con.nTextHeight * 2;
		lbRc = WriteFile(ahFile, con.pConChar, nSize, &dw, NULL);
		lbRc = WriteFile(ahFile, con.pConAttr, nSize, &dw, NULL); //-V519
	}

	UNREFERENCED_PARAMETER(lbRc);
}


bool CRealBuffer::LoadDumpConsole(LPCWSTR asDumpFile)
{
	bool lbRc = false;
	HANDLE hFile = NULL;
	LARGE_INTEGER liSize;
	COORD cr = {};
	wchar_t* pszDumpTitle, *pszRN, *pszSize;
	uint nX = 0;
	uint nY = 0;
	DWORD dwConDataBufSize = 0;
	DWORD dwConDataBufSizeEx = 0;
	
	con.m_sel.dwFlags = 0;
	dump.Close();

	if (!asDumpFile || !*asDumpFile)
	{
		_ASSERTE(asDumpFile!=NULL && *asDumpFile);
		goto wrap;
	}
	
	hFile = CreateFile(asDumpFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (!hFile || hFile == INVALID_HANDLE_VALUE)
	{
		DisplayLastError(L"Can't open dump file for reading");
		goto wrap;
	}
	if (!GetFileSizeEx(hFile, &liSize) || !liSize.LowPart || liSize.HighPart)
	{
		DisplayLastError(L"Invalid dump file size", -1);
		goto wrap;
	}
	
	dump.ptrData = (LPBYTE)malloc(liSize.LowPart);
	if (!dump.ptrData)
	{
		_ASSERTE(dump.ptrData!=NULL);
		goto wrap;
	}
	dump.cbDataSize = liSize.LowPart;
	
	if (!ReadFile(hFile, dump.ptrData, liSize.LowPart, (LPDWORD)&liSize.HighPart, NULL) || (liSize.LowPart != (DWORD)liSize.HighPart))
	{
		DisplayLastError(L"Dump file reading failed");
		goto wrap;
	}
	CloseHandle(hFile); hFile = NULL;

	// Поехали
	pszDumpTitle = (wchar_t*)dump.ptrData;
	
	pszRN = wcschr(pszDumpTitle, L'\r');
	if (!pszRN)
	{
		DisplayLastError(L"Dump file invalid format (title)", -1);
		goto wrap;
	}
	*pszRN = 0;
	pszSize = pszRN + 2;

	if (wcsncmp(pszSize, L"Size: ", 6))
	{
		DisplayLastError(L"Dump file invalid format (Size start)", -1);
		goto wrap;
	}

	pszRN = wcschr(pszSize, L'\r');

	if (!pszRN)
	{
		DisplayLastError(L"Dump file invalid format (Size line end)", -1);
		goto wrap;
	}

	// Первый блок
	dump.pszBlock1 = pszRN + 2;
	
	pszSize += 6;
	dump.crSize.X = (SHORT)wcstol(pszSize, &pszRN, 10);

	if (!pszRN || *pszRN!=L'x')
	{
		DisplayLastError(L"Dump file invalid format (Size.X)", -1);
		goto wrap;
	}

	pszSize = pszRN + 1;
	dump.crSize.Y = (SHORT)wcstol(pszSize, &pszRN, 10);

	if (!pszRN || (*pszRN!=L' ' && *pszRN!=L'\r'))
	{
		DisplayLastError(L"Dump file invalid format (Size.Y)", -1);
		goto wrap;
	}

	pszSize = pszRN;
	dump.crCursor.X = 0; dump.crCursor.Y = dump.crSize.Y-1;

	if (*pszSize == L' ')
	{
		while(*pszSize == L' ') pszSize++;

		if (wcsncmp(pszSize, L"Cursor: ", 8)==0)
		{
			pszSize += 8;
			cr.X = (SHORT)wcstol(pszSize, &pszRN, 10);

			if (!pszRN || *pszRN!=L'x')
			{
				DisplayLastError(L"Dump file invalid format (Cursor)", -1);
				goto wrap;
			}

			pszSize = pszRN + 1;
			cr.Y = (SHORT)wcstol(pszSize, &pszRN, 10);

			if (cr.X>=0 && cr.Y>=0)
			{
				dump.crCursor.X = cr.X; dump.crCursor.Y = cr.Y;
			}
		}
	}

	dwConDataBufSize = dump.crSize.X * dump.crSize.Y;
	dwConDataBufSizeEx = dump.crSize.X * dump.crSize.Y * sizeof(CharAttr);
	
	dump.pcaBlock1 = (CharAttr*)(((WORD*)(dump.pszBlock1)) + dwConDataBufSize);
	dump.Block1 = (((LPBYTE)(((LPBYTE)(dump.pcaBlock1)) + dwConDataBufSizeEx)) - dump.ptrData) <= (INT_PTR)dump.cbDataSize;
	
	if (!dump.Block1)
	{
		DisplayLastError(L"Dump file invalid format (Block1)", -1);
		goto wrap;
	}
	
	m_Type = rbt_DumpScreen;

	
	// При активации дополнительного буфера нельзя выполнять SyncWindow2Console.
	// Привести видимую область буфера к текущей.
	nX = mp_RCon->TextWidth();
	nY = mp_RCon->TextHeight();

	
	// Почти все готово, осталась инициализация
	ZeroStruct(con.m_sel);
	con.m_ci.dwSize = 15; con.m_ci.bVisible = TRUE;
	ZeroStruct(con.m_sbi);
	con.m_dwConsoleCP = con.m_dwConsoleOutputCP = 866; con.m_dwConsoleMode = 0;
	con.m_sbi.dwSize = dump.crSize;
	con.m_sbi.dwCursorPosition = dump.crCursor;
	con.m_sbi.wAttributes = 7;
	con.m_sbi.srWindow.Left = 0;
	con.m_sbi.srWindow.Top = 0;
	con.m_sbi.srWindow.Right = nX - 1;
	con.m_sbi.srWindow.Bottom = nY - 1;
	con.crMaxSize = mp_RCon->mp_RBuf->con.crMaxSize; //MakeCoord(max(dump.crSize.X,nX),max(dump.crSize.Y,nY));
	con.m_sbi.dwMaximumWindowSize = con.crMaxSize; //dump.crSize;
	con.nTopVisibleLine = 0;
	con.nTextWidth = nX/*dump.crSize.X*/;
	con.nTextHeight = nY/*dump.crSize.Y*/;
	con.nBufferHeight = dump.crSize.Y;
	con.bBufferHeight = TRUE;
	TODO("Горизонтальная прокрутка");
	
	//dump.NeedApply = TRUE;
	
	// Создание буферов
	if (!InitBuffers(0))
	{
		_ASSERTE(FALSE);
		goto wrap;
	}
	else
	// И копирование
	{
		wchar_t*  pszSrc = dump.pszBlock1;
		CharAttr* pcaSrc = dump.pcaBlock1;
		wchar_t*  pszDst = con.pConChar;
		TODO("Хорошо бы весь расширенный буфер тут хранить, а не только CHAR_ATTR");
		WORD*     pnaDst = con.pConAttr;
		
		wmemmove(pszDst, pszSrc, dwConDataBufSize);

		WARNING("Похоже, что это сейчас не требуется, все равно будет перечитано");
		// Расфуговка буфера CharAttr на консольные атрибуты
		for (DWORD n = 0; n < dwConDataBufSize; n++, pcaSrc++, pnaDst++)
		{
			*pnaDst = (pcaSrc->nForeIdx & 0xF) | ((pcaSrc->nBackIdx & 0xF) << 4);
		}
	}

	con.bConsoleDataChanged = TRUE;

	lbRc = true; // OK
wrap:
	if (hFile && hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if (!lbRc)
		dump.Close();
	return lbRc;
}

bool CRealBuffer::LoadDataFromDump(const CONSOLE_SCREEN_BUFFER_INFO& storedSbi, const CHAR_INFO* pData, DWORD cchMaxCellCount)
{
	bool lbRc = false;
	LARGE_INTEGER liSize;
	//COORD cr = {};
	uint nX = 0;
	uint nY = 0;

	dump.Close();

	dump.crSize = storedSbi.dwSize;
	dump.crCursor = storedSbi.dwCursorPosition;

	DWORD cchCellCount = storedSbi.dwSize.X * storedSbi.dwSize.Y;
	// Памяти выделяем "+1", чтобы wchar_t* был ASCIIZ
	liSize.QuadPart = (cchCellCount + 1) * (sizeof(CharAttr) + sizeof(wchar_t));
	if (!cchCellCount || !liSize.LowPart || liSize.HighPart)
	{
		DisplayLastError(L"Invalid dump file size", -1);
		goto wrap;
	}

	if (!cchMaxCellCount || (cchMaxCellCount < cchCellCount))
	{
		DisplayLastError(L"Invalid max cell count", -1);
		goto wrap;
	}
	
	dump.ptrData = (LPBYTE)malloc(liSize.LowPart);
	if (!dump.ptrData)
	{
		_ASSERTE(dump.ptrData!=NULL);
		goto wrap;
	}
	dump.cbDataSize = liSize.LowPart;

	// Поехали

	// Первый (и единственный) блок
	dump.pszBlock1 = (wchar_t*)dump.ptrData;
	dump.pszBlock1[cchCellCount] = 0;
	dump.pcaBlock1 = (CharAttr*)(dump.pszBlock1 + cchCellCount + 1);
	dump.Block1 = TRUE;

	
	m_Type = rbt_Alternative;

	
	// При активации дополнительного буфера нельзя выполнять SyncWindow2Console.
	// Привести видимую область буфера к текущей.
	nX = mp_RCon->TextWidth();
	nY = mp_RCon->TextHeight();

	
	// Почти все готово, осталась инициализация
	ZeroStruct(con.m_sel);
	con.m_ci.dwSize = 15; con.m_ci.bVisible = TRUE;
	ZeroStruct(con.m_sbi);
	con.m_dwConsoleCP = con.m_dwConsoleOutputCP = 866; con.m_dwConsoleMode = 0;
	con.m_sbi.dwSize = dump.crSize;
	con.m_sbi.dwCursorPosition = dump.crCursor;
	con.m_sbi.wAttributes = 7;

	con.m_sbi.srWindow.Right = nX - 1;
	con.m_sbi.srWindow.Left = 0;
	con.m_sbi.srWindow.Bottom = min((storedSbi.srWindow.Top + (int)nY - 1),(storedSbi.dwSize.Y - 1));
	con.nTopVisibleLine = con.m_sbi.srWindow.Top = max(0,con.m_sbi.srWindow.Bottom - nY + 1);
	
	con.crMaxSize = mp_RCon->mp_RBuf->con.crMaxSize; //MakeCoord(max(dump.crSize.X,nX),max(dump.crSize.Y,nY));
	con.m_sbi.dwMaximumWindowSize = con.crMaxSize; //dump.crSize;
	con.nTextWidth = nX/*dump.crSize.X*/;
	con.nTextHeight = nY/*dump.crSize.Y*/;
	con.bBufferHeight = (dump.crSize.Y > (int)nY);
	con.nBufferHeight = con.bBufferHeight ? dump.crSize.Y : 0;
	TODO("Горизонтальная прокрутка");
	
	//dump.NeedApply = TRUE;
	
	// Создание буферов
	if (!InitBuffers(0))
	{
		_ASSERTE(FALSE);
		goto wrap;
	}
	else
	// И копирование
	{
		TODO("Хорошо бы весь расширенный буфер тут хранить, а не только CHAR_INFO");

		const CHAR_INFO* ptrSrc = pData;
		CharAttr* pcaDst = dump.pcaBlock1;
		wchar_t*  pszDst = dump.pszBlock1;
		WORD*     pnaDst = con.pConAttr;
		wchar_t   ch;

		CharAttr lcaTableExt[0x100], lcaTableOrg[0x100]; // crForeColor, crBackColor, nFontIndex, nForeIdx, nBackIdx, crOrigForeColor, crOrigBackColor
		PrepareColorTable(false/*bExtendFonts*/, lcaTableExt, lcaTableOrg);

		DWORD nMax = min(cchCellCount,cchMaxCellCount);
		// Расфуговка буфера на консольные атрибуты
		for (DWORD n = 0; n < nMax; n++, ptrSrc++, pszDst++, pcaDst++, pnaDst++)
		{
			ch = ptrSrc->Char.UnicodeChar;
			//2009-09-25. Некоторые (старые?) программы умудряются засунуть в консоль символы (ASC<32)
			//            их нужно заменить на юникодные аналоги
			*pszDst = (ch < 32) ? gszAnalogues[(WORD)ch] : ch;

			*pcaDst = lcaTableOrg[ptrSrc->Attributes & 0xFF];

			//WARNING("Похоже, что это сейчас не требуется, все равно будет перечитано");
			//*pnaDst = ptrSrc->Attributes;
		}

		if (cchCellCount > cchMaxCellCount)
		{
			_ASSERTE(cchCellCount <= cchMaxCellCount);
			for (DWORD n = nMax; n < cchCellCount; n++, pszDst++, pcaDst++)
			{
				*pszDst = L' ';
				*pcaDst = lcaTableOrg[0];
			}
		}
		
		//WARNING("Похоже, что это сейчас не требуется, все равно будет перечитано");
		//wmemmove(con.pConChar, dump.pszBlock1, cchCellCount);
	}

	con.bConsoleDataChanged = TRUE;

	lbRc = true; // OK
wrap:
	if (!lbRc)
		dump.Close();
	return lbRc;
}

// 1 - Last long console output, 2 - current console "image", 3 - copy of rbt_Primary
bool CRealBuffer::LoadAlternativeConsole(LoadAltMode iMode /*= lam_Default*/)
{
	bool lbRc = false;
	
	con.m_sel.dwFlags = 0;
	dump.Close();

	if (iMode == lam_Default)
	{
		if (mp_RCon->isFar() && gpSet->AutoBufferHeight)
		{
			iMode = lam_LastOutput;
		}
		else
		{
			// При открытии Vim (например) буфер отключается и переходим в режим без прокрутки
			// А вот старый буфер как раз и хочется посмотреть
			bool lbAltBufSwitched = false;
			if (!mp_RCon->isBufferHeight())
			{
				mp_RCon->GetServerPID();

				CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_ALTBUFFERSTATE, sizeof(CESERVER_REQ_HDR));
				if (pIn)
				{
					CESERVER_REQ *pOut = ExecuteSrvCmd(mp_RCon->GetServerPID(), pIn, ghWnd);
					if (pOut && (pOut->DataSize() >= sizeof(DWORD)))
					{
						lbAltBufSwitched = (pOut->dwData[0]!=0);
					}
					ExecuteFreeResult(pOut);
					ExecuteFreeResult(pIn);
				}
			}

			iMode = lbAltBufSwitched ? lam_LastOutput : lam_FullBuffer;
		}
	}

	if (iMode == lam_LastOutput)
	{
		MFileMapping<CESERVER_CONSAVE_MAPHDR> StoredOutputHdr;
		MFileMapping<CESERVER_CONSAVE_MAP> StoredOutputItem;

		CESERVER_CONSAVE_MAPHDR* pHdr = NULL;
		CESERVER_CONSAVE_MAP* pData = NULL;
		CONSOLE_SCREEN_BUFFER_INFO storedSbi = {};
		DWORD cchMaxBufferSize = 0;
		size_t nMaxSize = 0;

		StoredOutputHdr.InitName(CECONOUTPUTNAME, (DWORD)mp_RCon->hConWnd); //-V205
		if (!(pHdr = StoredOutputHdr.Open()) || !pHdr->sCurrentMap[0])
		{
			DisplayLastError(L"Stored output mapping was not created!");
			goto wrap;
		}

		cchMaxBufferSize = min(pHdr->MaxCellCount, (DWORD)(pHdr->info.dwSize.X * pHdr->info.dwSize.Y));

		StoredOutputItem.InitName(pHdr->sCurrentMap); //-V205
		nMaxSize = sizeof(*pData) + cchMaxBufferSize * sizeof(pData->Data[0]);
		if (!(pData = StoredOutputItem.Open(FALSE,nMaxSize)))
		{
			DisplayLastError(L"Stored output data mapping was not created!");
			goto wrap;
		}

		if ((pData->hdr.nVersion != CESERVER_REQ_VER) || (pData->hdr.cbSize <= sizeof(CESERVER_CONSAVE_MAP)))
		{
			DisplayLastError(L"Invalid data in mapping header", -1);
			goto wrap;
		}

		storedSbi = pData->info;

		lbRc = LoadDataFromDump(storedSbi, pData->Data, cchMaxBufferSize);
	}
	else if (iMode == lam_FullBuffer)
	{
		CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_CONSOLEFULL, sizeof(CESERVER_REQ_HDR));
		if (pIn)
		{
			CESERVER_REQ *pOut = ExecuteSrvCmd(mp_RCon->GetServerPID(), pIn, ghWnd);
			if (pOut && (pOut->hdr.cbSize > sizeof(CESERVER_CONSAVE_MAP)))
			{
				const CESERVER_CONSAVE_MAP* pInfo = (const CESERVER_CONSAVE_MAP*)pOut;
				// Go
				lbRc = LoadDataFromDump(pInfo->info, pInfo->Data, pInfo->MaxCellCount);
			}
			ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
	}
	else
	{
		// Unsupported yet...
		_ASSERTE(iMode==lam_LastOutput || iMode==lam_FullBuffer);
	}

wrap:
	if (!lbRc)
		dump.Close();
	return lbRc;
}

BOOL CRealBuffer::SetConsoleSizeSrv(USHORT sizeX, USHORT sizeY, USHORT sizeBuffer, DWORD anCmdID/*=CECMD_SETSIZESYNC*/)
{
	if (!this) return FALSE;

	_ASSERTE(m_Type == rbt_Primary);
	#ifdef _DEBUG
	int nVConNo = gpConEmu->isVConValid(mp_RCon->VCon());
	#endif

	if (!mp_RCon->hConWnd || mp_RCon->ms_ConEmuC_Pipe[0] == 0)
	{
		// 19.06.2009 Maks - Она действительно может быть еще не создана
		//Box(_T("Console was not created (CRealConsole::SetConsoleSize)"));
		DEBUGSTRSIZE(L"SetConsoleSize skipped (!mp_RCon->hConWnd || !mp_RCon->ms_ConEmuC_Pipe)\n");

		if (gpSetCls->isAdvLogging) mp_RCon->LogString("SetConsoleSize skipped (!mp_RCon->hConWnd || !mp_RCon->ms_ConEmuC_Pipe)");

		return FALSE; // консоль пока не создана?
	}

	BOOL lbRc = FALSE;
	BOOL fSuccess = FALSE;
	//DWORD dwRead = 0;
	DWORD dwTickStart = 0;
	DWORD nCallTimeout = 0;
	DWORD nInSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETSIZE);
	DWORD nOutSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_RETSIZE);
	CESERVER_REQ* pIn = ExecuteNewCmd(anCmdID, nInSize);
	CESERVER_REQ* pOut = NULL; //ExecuteNewCmd(anCmdID, nOutSize);
	SMALL_RECT rect = {0};
	//bool bLargestReached = false;
	bool bSecondTry = false;

	_ASSERTE(anCmdID==CECMD_SETSIZESYNC || anCmdID==CECMD_SETSIZENOSYNC || anCmdID==CECMD_CMDSTARTED || anCmdID==CECMD_CMDFINISHED);
	//ExecutePrepareCmd(&lIn.hdr, anCmdID, lIn.hdr.cbSize);
	if (!pIn /*|| !pOut*/)
	{
		_ASSERTE(pIn);
		goto wrap;
	}

	// Для режима BufferHeight нужно передать еще и видимый прямоугольник (нужна только верхняя координата?)
	if (con.bBufferHeight)
	{
		// case: buffer mode: change buffer
		CONSOLE_SCREEN_BUFFER_INFO sbi = con.m_sbi;
		sbi.dwSize.X = sizeX; // new
		sizeBuffer = BufferHeight(sizeBuffer); // Если нужно - скорректировать
		_ASSERTE(sizeBuffer > 0);
		sbi.dwSize.Y = sizeBuffer;
		rect.Top = sbi.srWindow.Top;
		rect.Left = sbi.srWindow.Left;
		rect.Right = rect.Left + sizeX - 1;
		rect.Bottom = rect.Top + sizeY - 1;

		if (rect.Right >= sbi.dwSize.X)
		{
			int shift = sbi.dwSize.X - 1 - rect.Right;
			rect.Left += shift;
			rect.Right += shift;
		}

		if (rect.Bottom >= sbi.dwSize.Y)
		{
			int shift = sbi.dwSize.Y - 1 - rect.Bottom;
			rect.Top += shift;
			rect.Bottom += shift;
		}

		// В size передаем видимую область
		//sizeY = TextHeight(); -- sizeY уже(!) должен содержать требуемую высоту видимой области!
	}
	else
	{
		_ASSERTE(sizeBuffer == 0);
	}

	_ASSERTE(sizeY>0 && sizeY<200);
	// Устанавливаем параметры для передачи в ConEmuC
	pIn->SetSize.nBufferHeight = sizeBuffer; //con.bBufferHeight ? gpSet->Default BufferHeight : 0;
	pIn->SetSize.size.X = sizeX;
	pIn->SetSize.size.Y = sizeY;
	TODO("nTopVisibleLine должен передаваться при скролле, а не при ресайзе!");
	#ifdef SHOW_AUTOSCROLL
	pIn->SetSize.nSendTopLine = (gpSetCls->AutoScroll || !con.bBufferHeight) ? -1 : con.nTopVisibleLine;
	#else
	pIn->SetSize.nSendTopLine = mp_RCon->InScroll() ? con.nTopVisibleLine : -1;
	#endif
	pIn->SetSize.rcWindow = rect;
	pIn->SetSize.dwFarPID = (con.bBufferHeight && !mp_RCon->isFarBufferSupported()) ? 0 : mp_RCon->GetFarPID(TRUE);

	// Если размер менять не нужно - то и CallNamedPipe не делать
	//if (mp_ConsoleInfo) {

	// Если заблокирована верхняя видимая строка - выполнять строго
	if (anCmdID != CECMD_CMDFINISHED && pIn->SetSize.nSendTopLine == -1)
	{
		// иначе - проверяем текущее соответствие
		//CONSOLE_SCREEN_BUFFER_INFO sbi = mp_ConsoleInfo->sbi;
		bool lbSizeMatch = true;

		if (con.bBufferHeight)
		{
			if (con.m_sbi.dwSize.X != sizeX || con.m_sbi.dwSize.Y != sizeBuffer)
				lbSizeMatch = false;
			else if (con.m_sbi.srWindow.Top != rect.Top || con.m_sbi.srWindow.Bottom != rect.Bottom)
				lbSizeMatch = false;
		}
		else
		{
			if (con.m_sbi.dwSize.X != sizeX || con.m_sbi.dwSize.Y != sizeY)
				lbSizeMatch = false;
		}

		// fin
		if (lbSizeMatch && anCmdID != CECMD_CMDFINISHED)
		{
			lbRc = TRUE; // менять ничего не нужно
			goto wrap;
		}

		// При смене размера буфера - сбросить последнее мышиное событие
		ResetLastMousePos();
	}

	char szInfo[128], szSizeCmd[32];

	if (gpSetCls->isAdvLogging)
	{
		switch (anCmdID)
		{
		case CECMD_SETSIZESYNC:
			lstrcpynA(szSizeCmd, "CECMD_SETSIZESYNC", countof(szSizeCmd)); break;
		case CECMD_CMDSTARTED:
			lstrcpynA(szSizeCmd, "CECMD_CMDSTARTED", countof(szSizeCmd)); break;
		case CECMD_CMDFINISHED:
			lstrcpynA(szSizeCmd, "CECMD_CMDFINISHED", countof(szSizeCmd)); break;
		default:
			_wsprintfA(szSizeCmd, SKIPLEN(countof(szSizeCmd)) "SizeCmd=%u", anCmdID);
		}
		_wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "%s(Cols=%i, Rows=%i, Buf=%i, Top=%i)",
		           szSizeCmd, sizeX, sizeY, sizeBuffer, pIn->SetSize.nSendTopLine);
		mp_RCon->LogString(szInfo, TRUE);
	}

	#ifdef _DEBUG
	wchar_t szDbgCmd[128]; _wsprintf(szDbgCmd, SKIPLEN(countof(szDbgCmd)) L"SetConsoleSize.ExecuteCmd(cx=%i, cy=%i, buf=%i, cmd=%i)\n",
	                                 sizeX, sizeY, sizeBuffer, anCmdID);
	DEBUGSTRSIZE(szDbgCmd);
	#endif

	dwTickStart = timeGetTime();
	// С таймаутом
	nCallTimeout = RELEASEDEBUGTEST(500,30000);

	/*fSuccess = CallNamedPipe(mp_RCon->ms_ConEmuC_Pipe, pIn, pIn->hdr.cbSize, pOut, pOut->hdr.cbSize, &dwRead, nCallTimeout);*/
	_ASSERTE(pOut==NULL);
	pOut = ExecuteCmd(mp_RCon->ms_ConEmuC_Pipe, pIn, nCallTimeout, ghWnd);
	fSuccess = (pOut != NULL);

	if (fSuccess /*&& (dwRead == (DWORD)nOutSize)*/)
	{
		int nSetWidth = sizeX, nSetHeight = sizeY;
		if (GetConWindowSize(pOut->SetSizeRet.SetSizeRet, &nSetWidth, &nSetHeight, NULL))
		{
			// If change-size (enlarging) was failed
			if ((sizeX > (UINT)nSetWidth) || (sizeY > (UINT)nSetHeight))
			{
				// Check width
				if ((pOut->SetSizeRet.crMaxSize.X > nSetWidth) && (sizeX > (UINT)nSetWidth))
				{
					bSecondTry = true;
					pIn->SetSize.size.X = pOut->SetSizeRet.crMaxSize.X;
					pIn->SetSize.rcWindow.Right = pIn->SetSize.rcWindow.Left + pIn->SetSize.size.X - 1;
				}
				// And height
				if ((pOut->SetSizeRet.crMaxSize.Y > nSetHeight) && (sizeY > (UINT)nSetHeight))
				{
					bSecondTry = true;
					pIn->SetSize.size.Y = pOut->SetSizeRet.crMaxSize.Y;
					pIn->SetSize.rcWindow.Bottom = pIn->SetSize.rcWindow.Top + pIn->SetSize.size.Y - 1;
				}
				// Try again with lesser size?
				if (bSecondTry)
				{
					/*fSuccess = CallNamedPipe(mp_RCon->ms_ConEmuC_Pipe, pIn, pIn->hdr.cbSize, pOut, pOut->hdr.cbSize, &dwRead, nCallTimeout);*/
					ExecuteFreeResult(pOut);
					pOut = ExecuteCmd(mp_RCon->ms_ConEmuC_Pipe, pIn, nCallTimeout, ghWnd);
					fSuccess = (pOut != NULL);
				}
				// Inform user
				mp_RCon->LogString(L"Maximum real console size was reached, lesser size was applyed");
				Icon.ShowTrayIcon(L"Maximum real console size was reached\nDecrease font size in the real console properties", tsa_Console_Size);
			}
		}
	}

	gpSetCls->debugLogCommand(pIn, FALSE, dwTickStart, timeGetTime()-dwTickStart, mp_RCon->ms_ConEmuC_Pipe, pOut);

	if (!fSuccess || (pOut && (pOut->hdr.cbSize < nOutSize)))
	{
		if (gpSetCls->isAdvLogging)
		{
			DWORD dwErr = GetLastError();
			_wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "SetConsoleSizeSrv.ExecuteCmd FAILED!!! ErrCode=0x%08X, Bytes read=%i", dwErr, pOut ? pOut->hdr.cbSize : 0);
			mp_RCon->LogString(szInfo);
		}

		DEBUGSTRSIZE(L"SetConsoleSize.ExecuteCmd FAILED!!!\n");
	}
	else
	{
		_ASSERTE(mp_RCon->m_ConsoleMap.IsValid());
		bool bNeedApplyConsole = //mp_ConsoleInfo &&
		    mp_RCon->m_ConsoleMap.IsValid()
		    && (anCmdID == CECMD_SETSIZESYNC)
		    && (mp_RCon->mn_MonitorThreadID != GetCurrentThreadId());
		DEBUGSTRSIZE(L"SetConsoleSize.fSuccess == TRUE\n");

		if (pOut->hdr.nCmd != pIn->hdr.nCmd)
		{
			_ASSERTE(pOut->hdr.nCmd == pIn->hdr.nCmd);

			if (gpSetCls->isAdvLogging)
			{
				char szInfo[128]; _wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "SetConsoleSizeSrv FAILED!!! OutCmd(%i)!=InCmd(%i)", pOut->hdr.nCmd, pIn->hdr.nCmd);
				mp_RCon->LogString(szInfo);
			}
		}
		else
		{
			//con.nPacketIdx = lOut.SetSizeRet.nNextPacketId;
			//mn_LastProcessedPkt = lOut.SetSizeRet.nNextPacketId;
			CONSOLE_SCREEN_BUFFER_INFO sbi = {{0,0}};
			int nBufHeight;

			//_ASSERTE(mp_ConsoleInfo);
			if (bNeedApplyConsole /*&& mp_ConsoleInfo->nCurDataMapIdx && (HWND)mp_ConsoleInfo->mp_RCon->hConWnd*/)
			{
				// Если Apply еще не прошел - ждем
				DWORD nWait = -1;

				if (con.m_sbi.dwSize.X != sizeX || con.m_sbi.dwSize.Y != (sizeBuffer ? sizeBuffer : sizeY))
				{
					//// Проходит некоторое (короткое) время, пока обновится FileMapping в нашем процессе
					//_ASSERTE(mp_ConsoleInfo!=NULL);
					//COORD crCurSize = mp_ConsoleInfo->sbi.dwSize;
					//if (crCurSize.X != sizeX || crCurSize.Y != (sizeBuffer ? sizeBuffer : sizeY))
					//{
					//	DWORD dwStart = GetTickCount();
					//	do {
					//		Sleep(1);
					//		crCurSize = mp_ConsoleInfo->sbi.dwSize;
					//		if (crCurSize.X == sizeX && crCurSize.Y == (sizeBuffer ? sizeBuffer : sizeY))
					//			break;
					//	} while ((GetTickCount() - dwStart) < SETSYNCSIZEMAPPINGTIMEOUT);
					//}
					ResetEvent(mp_RCon->mh_ApplyFinished);
					mp_RCon->mn_LastConsolePacketIdx--;
					mp_RCon->SetMonitorThreadEvent();
					DWORD nWaitTimeout = SETSYNCSIZEAPPLYTIMEOUT;
					
					#ifdef _DEBUG
					nWaitTimeout = SETSYNCSIZEAPPLYTIMEOUT*4; //30000;
					nWait = WaitForSingleObject(mp_RCon->mh_ApplyFinished, nWaitTimeout);
					if (nWait == WAIT_TIMEOUT)
					{
						_ASSERTE(FALSE && "SETSYNCSIZEAPPLYTIMEOUT");
						//nWait = WaitForSingleObject(mp_RCon->mh_ApplyFinished, nWaitTimeout);
					}
					#else
					nWait = WaitForSingleObject(mp_RCon->mh_ApplyFinished, nWaitTimeout);
					#endif
					
					COORD crDebugCurSize = con.m_sbi.dwSize;

					if (crDebugCurSize.X != sizeX)
					{
						if (gpSetCls->isAdvLogging)
						{
							char szInfo[128]; _wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "SetConsoleSize FAILED!!! ReqSize={%ix%i}, OutSize={%ix%i}", sizeX, (sizeBuffer ? sizeBuffer : sizeY), crDebugCurSize.X, crDebugCurSize.Y);
							mp_RCon->LogString(szInfo);
						}

#ifdef _DEBUG
						_ASSERTE(crDebugCurSize.X == sizeX);
#endif
					}
				}

				if (gpSetCls->isAdvLogging)
				{
					mp_RCon->LogString(
					    (nWait == (DWORD)-1) ?
					    "SetConsoleSizeSrv: does not need wait" :
					    (nWait != WAIT_OBJECT_0) ?
					    "SetConsoleSizeSrv.WaitForSingleObject(mp_RCon->mh_ApplyFinished) TIMEOUT!!!" :
					    "SetConsoleSizeSrv.WaitForSingleObject(mp_RCon->mh_ApplyFinished) succeded");
				}

				sbi = con.m_sbi;
				nBufHeight = con.nBufferHeight;
			}
			else
			{
				sbi = pOut->SetSizeRet.SetSizeRet;
				nBufHeight = pIn->SetSize.nBufferHeight;

				if (gpSetCls->isAdvLogging)
					mp_RCon->LogString("SetConsoleSizeSrv.Not waiting for ApplyFinished");
			}

			WARNING("!!! Здесь часто возникают _ASSERTE'ы. Видимо высота буфера меняется в другой нити и con.bBufferHeight просто не успевает?");

			if (con.bBufferHeight)
			{
				_ASSERTE((sbi.srWindow.Bottom-sbi.srWindow.Top)<200);

				if (gpSetCls->isAdvLogging)
				{
					char szInfo[128]; _wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "Current size: Cols=%i, Buf=%i", sbi.dwSize.X, sbi.dwSize.Y);
					mp_RCon->LogString(szInfo, TRUE);
				}

				if (sbi.dwSize.X == sizeX && sbi.dwSize.Y == nBufHeight)
				{
					lbRc = TRUE;
				}
				else
				{
					if (gpSetCls->isAdvLogging)
					{
						char szInfo[128]; _wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "SetConsoleSizeSrv FAILED! Ask={%ix%i}, Cur={%ix%i}, Ret={%ix%i}",
						                             sizeX, sizeY,
						                             sbi.dwSize.X, sbi.dwSize.Y,
						                             pOut->SetSizeRet.SetSizeRet.dwSize.X, pOut->SetSizeRet.SetSizeRet.dwSize.Y
						                            );
						mp_RCon->LogString(szInfo);
					}

					lbRc = FALSE;
				}
			}
			else
			{
				if (sbi.dwSize.Y > 200)
				{
					_ASSERTE(sbi.dwSize.Y<200);
				}

				if (gpSetCls->isAdvLogging)
				{
					char szInfo[128]; _wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "Current size: Cols=%i, Rows=%i", sbi.dwSize.X, sbi.dwSize.Y);
					mp_RCon->LogString(szInfo, TRUE);
				}

				if (sbi.dwSize.X == sizeX && sbi.dwSize.Y == sizeY)
				{
					lbRc = TRUE;
				}
				else
				{
					if (gpSetCls->isAdvLogging)
					{
						char szInfo[128]; _wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "SetConsoleSizeSrv FAILED! Ask={%ix%i}, Cur={%ix%i}, Ret={%ix%i}",
						                             sizeX, sizeY,
						                             sbi.dwSize.X, sbi.dwSize.Y,
						                             pOut->SetSizeRet.SetSizeRet.dwSize.X, pOut->SetSizeRet.SetSizeRet.dwSize.Y
						                            );
						mp_RCon->LogString(szInfo);
					}

					lbRc = FALSE;
				}
			}

			//if (sbi.dwSize.X == size.X && sbi.dwSize.Y == size.Y) {
			//    con.m_sbi = sbi;
			//    if (sbi.dwSize.X == con.m_sbi.dwSize.X && sbi.dwSize.Y == con.m_sbi.dwSize.Y) {
			//        SetEvent(mh_ConChanged);
			//    }
			//    lbRc = true;
			//}
			if (lbRc)  // Попробуем сразу менять nTextWidth/nTextHeight. Иначе синхронизация размеров консоли глючит...
			{
				DEBUGSTRSIZE(L"SetConsoleSizeSrv.lbRc == TRUE\n");
				con.nChange2TextWidth = sbi.dwSize.X;
				con.nChange2TextHeight = con.bBufferHeight ? (sbi.srWindow.Bottom-sbi.srWindow.Top+1) : sbi.dwSize.Y;
#ifdef _DEBUG

				if (con.bBufferHeight)
					_ASSERTE(con.nBufferHeight == sbi.dwSize.Y);

#endif
				con.nBufferHeight = con.bBufferHeight ? sbi.dwSize.Y : 0;

				if (con.nChange2TextHeight > 200)
				{
					_ASSERTE(con.nChange2TextHeight<=200);
				}
			}
		}
	}

wrap:
	ExecuteFreeResult(pIn);
	ExecuteFreeResult(pOut);
	return lbRc;
}

BOOL CRealBuffer::SetConsoleSize(USHORT sizeX, USHORT sizeY, USHORT sizeBuffer, DWORD anCmdID/*=CECMD_SETSIZESYNC*/)
{
	if (!this) return FALSE;

	// Если была блокировка DC - сбросим ее
	mp_RCon->mp_VCon->LockDcRect(FALSE, NULL);

	if (m_Type != rbt_Primary)
	{
		// Все альтернативные буферы "меняются" виртуально
		con.nTextWidth = sizeX;
		con.nTextHeight = sizeY;
		con.nBufferHeight = sizeBuffer;
		con.bBufferHeight = (sizeBuffer != 0);
		ChangeScreenBufferSize(con.m_sbi, sizeX, sizeY, sizeX, sizeBuffer ? sizeBuffer : sizeY);

		if (mp_RCon->isActive())
		{
			gpConEmu->mp_Status->OnConsoleChanged(&con.m_sbi, &con.m_ci, true);
		}

		return TRUE;
	}

	//_ASSERTE(mp_RCon->hConWnd && mp_RCon->ms_ConEmuC_Pipe[0]);

	if (!mp_RCon->hConWnd || mp_RCon->ms_ConEmuC_Pipe[0] == 0)
	{
		// 19.06.2009 Maks - Она действительно может быть еще не создана
		//Box(_T("Console was not created (CRealConsole::SetConsoleSize)"));
		if (gpSetCls->isAdvLogging) mp_RCon->LogString("SetConsoleSize skipped (!mp_RCon->hConWnd || !mp_RCon->ms_ConEmuC_Pipe)");

		// Это должно быть только на этапе создания новой консоли (например, появилась панель табов)
		if (con.nTextWidth != sizeX || con.nTextHeight != sizeY)
		{
			_ASSERTE(mp_RCon->hConWnd==NULL || mp_RCon->mb_InCloseConsole);
			con.nTextWidth = sizeX;
			con.nTextHeight = sizeY;
			mp_RCon->VCon()->OnConsoleSizeReset(sizeX, sizeY);
		}

		DEBUGSTRSIZE(L"SetConsoleSize skipped (!mp_RCon->hConWnd || !mp_RCon->ms_ConEmuC_Pipe)\n");
		return false; // консоль пока не создана?
	}

	HEAPVAL
	_ASSERTE(sizeX>=MIN_CON_WIDTH && sizeY>=MIN_CON_HEIGHT);

	if (sizeX</*4*/MIN_CON_WIDTH)
		sizeX = /*4*/MIN_CON_WIDTH;

	if (sizeY</*3*/MIN_CON_HEIGHT)
		sizeY = /*3*/MIN_CON_HEIGHT;

	_ASSERTE(con.bBufferHeight || (!con.bBufferHeight && !sizeBuffer));

	if (con.bBufferHeight && !sizeBuffer)
		sizeBuffer = BufferHeight(sizeBuffer);

	_ASSERTE(!con.bBufferHeight || (sizeBuffer >= sizeY));
	BOOL lbRc = FALSE;
	#ifdef _DEBUG
	DWORD dwPID = mp_RCon->GetFarPID(TRUE);
	#endif
	//if (dwPID)
	//{
	//	// Если это СТАРЫЙ FAR (нет Synchro) - может быть не ресайзить через плагин?
	//	// Хотя тут плюс в том, что хоть активация и идет чуть медленнее, но
	//	// возврат из ресайза получается строго после обновления консоли
	//	if (!mb_PluginDetected || dwPID != mn_FarPID_PluginDetected)
	//		dwPID = 0;
	//}
	_ASSERTE(sizeY <= 300);
	/*if (!con.bBufferHeight && dwPID)
		lbRc = SetConsoleSizePlugin(sizeX, sizeY, sizeBuffer, anCmdID);
	else*/
	HEAPVAL;

	// Попробовать для консолей (cmd, и т.п.) делать ресайз после отпускания мышки
	if ((gpConEmu->mouse.state & MOUSE_SIZING_BEGIN)
		&& (!mp_RCon->GuiWnd() && !mp_RCon->GetFarPID()))
	{
		if (gpSetCls->isAdvLogging) mp_RCon->LogString("SetConsoleSize skipped until LMouseButton not released");
	}
	else
	{
		// Чтобы ВО время ресайза пакеты НЕ обрабатывались
		ResetEvent(con.hInSetSize); con.bInSetSize = TRUE;
		lbRc = SetConsoleSizeSrv(sizeX, sizeY, sizeBuffer, anCmdID);
		con.bInSetSize = FALSE; SetEvent(con.hInSetSize);
		HEAPVAL;
	}

#if 0
	if (lbRc && mp_RCon->isActive())
	{
		if (!mp_RCon->isNtvdm())
		{
			// update size info
			if (gpConEmu->isWindowNormal())
			{
				int nHeight = TextHeight();
				gpSetCls->UpdateSize(sizeX, nHeight);
			}
		}

		// -- должно срабатывать при ApplyData
		//gpConEmu->UpdateStatusBar();
	}
#endif

	HEAPVAL;
	DEBUGSTRSIZE(L"SetConsoleSize.finalizing\n");
	return lbRc;
}

void CRealBuffer::SyncConsole2Window(USHORT wndSizeX, USHORT wndSizeY)
{
	// Во избежание лишних движений да и зацикливания...
	if (con.nTextWidth != wndSizeX || con.nTextHeight != wndSizeY)
	{
		if (IFLOGCONSOLECHANGE)
		{
			char szInfo[128]; _wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "CRealBuffer::SyncConsole2Window(Cols=%i, Rows=%i, Current={%i,%i})", wndSizeX, wndSizeY, con.nTextWidth, con.nTextHeight);
			mp_RCon->LogString(szInfo);
		}

		#ifdef _DEBUG
		if (wndSizeX == 80)
		{
			int nDbg = wndSizeX;
		}
		#endif
		
		// Сразу поставим, чтобы в основной нити при синхронизации размер не слетел
		// Необходимо заблокировать RefreshThread, чтобы не вызывался ApplyConsoleInfo ДО ЗАВЕРШЕНИЯ SetConsoleSize
		con.bLockChange2Text = TRUE;
		con.nChange2TextWidth = wndSizeX; con.nChange2TextHeight = wndSizeY;
		SetConsoleSize(wndSizeX, wndSizeY, 0/*Auto*/);
		con.bLockChange2Text = FALSE;

		if (mp_RCon->isActive() && gpConEmu->isMainThread())
		{
			// Сразу обновить DC чтобы скорректировать Width & Height
			mp_RCon->mp_VCon->OnConsoleSizeChanged();
		}
	}
}

BOOL CRealBuffer::isScroll(RealBufferScroll aiScroll/*=rbs_Any*/)
{
	TODO("горизонтальная прокрутка");
	return con.bBufferHeight;
}

// Вызывается при аттаче (CRealConsole::AttachConemuC)
void CRealBuffer::InitSBI(CONSOLE_SCREEN_BUFFER_INFO* ap_sbi)
{
	con.m_sbi = *ap_sbi;
}

void CRealBuffer::InitMaxSize(const COORD& crMaxSize)
{
	con.crMaxSize = crMaxSize;
}

COORD CRealBuffer::GetMaxSize()
{
	return con.crMaxSize;
}

bool CRealBuffer::isInitialized()
{
	if (!con.pConChar || !con.nTextWidth || con.nTextHeight<2)
		return false; // консоль не инициализирована, ловить нечего
	return true;
}

bool CRealBuffer::isFarMenuOrMacro()
{
	BOOL lbMenuActive = false;
	MSectionLock cs; cs.Lock(&csCON);

	WARNING("CantActivateInfo: Хорошо бы при отображении хинта 'Can't activate tab' сказать 'почему'");

	if (con.pConChar && con.pConAttr)
	{
		TODO("Хорошо бы реально у фара узнать, выполняет ли он макрос");
		if (((con.pConChar[0] == L'R') && ((con.pConAttr[0] & 0xFF) == 0x4F))
			|| ((con.pConChar[0] == L'P') && ((con.pConAttr[0] & 0xFF) == 0x2F)))
		{
			// Запись макроса. Запретим наверное переключаться?
			lbMenuActive = true;
		}
		else if (con.pConChar[0] == L' ' && con.pConChar[con.nTextWidth] == ucBoxDblVert)
		{
			lbMenuActive = true;
		}
		else if (con.pConChar[0] == L' ' && (con.pConChar[con.nTextWidth] == ucBoxDblDownRight ||
		                                    (con.pConChar[con.nTextWidth] == '['
		                                     && (con.pConChar[con.nTextWidth+1] >= L'0' && con.pConChar[con.nTextWidth+1] <= L'9'))))
		{
			// Строка меню ВСЕГДА видна. Определим, активно ли меню.
			for(int x=1; !lbMenuActive && x<con.nTextWidth; x++)
			{
				if (con.pConAttr[x] != con.pConAttr[0])  // неактивное меню - не подсвечивается
					lbMenuActive = true;
			}
		}
		else
		{
			// Если строка меню ВСЕГДА не видна, а только всплывает
			wchar_t* pszLine = con.pConChar + con.nTextWidth;

			for(int x=1; !lbMenuActive && x<(con.nTextWidth-10); x++)
			{
				if (pszLine[x] == ucBoxDblDownRight && pszLine[x+1] == ucBoxDblHorz)
					lbMenuActive = true;
			}
		}
	}

	cs.Unlock();
	
	return lbMenuActive;
}

BOOL CRealBuffer::isConsoleDataChanged()
{
	if (!this) return FALSE;
	return con.bConsoleDataChanged;
}

BOOL CRealBuffer::PreInit()
{
	HEAPVAL;
	// Инициализировать переменные m_sbi, m_ci, m_sel
	//RECT rcWnd; Get ClientRect(ghWnd, &rcWnd);
	
	// mp_RCon->isBufferHeight использовать нельзя, т.к. mp_RBuf.m_sbi еще не инициализирован!
	bool bNeedBufHeight = (gpSetCls->bForceBufferHeight && gpSetCls->nForceBufferHeight>0)
	                      || (!gpSetCls->bForceBufferHeight && mp_RCon->mn_DefaultBufferHeight);

	if (bNeedBufHeight && !isScroll())
	{
		HEAPVAL;
		SetBufferHeightMode(TRUE);
		HEAPVAL;
		_ASSERTE(mp_RCon->mn_DefaultBufferHeight>0);
		BufferHeight(mp_RCon->mn_DefaultBufferHeight);
	}

	HEAPVAL;
	RECT rcCon;

	// "!(gpConEmu->isZoomed() || gpConEmu->isFullScreen())" дает не то...

	// Если настроенно-развернутое окно запускается минимизированным
	// то брать нужно "максимизированные" размеры, а не wndWidth/wndHeight
	if (gpConEmu->isWindowNormal() && gpConEmu->isIconic())
	{
		// Сюда попадаем только при wmNormal&Minimized
		//rcCon = MakeRect(gpConEmu->wndWidth, gpConEmu->wndHeight);
		SIZE sz = gpConEmu->GetDefaultSize(true);
		rcCon = MakeRect(sz.cx, sz.cy);
	}
	else
	{
		rcCon = gpConEmu->CalcRect(CER_CONSOLE_CUR, mp_RCon->mp_VCon);
	}

	_ASSERTE(rcCon.right!=0 && rcCon.bottom!=0);

	if (con.bBufferHeight)
	{
		_ASSERTE(mp_RCon->mn_DefaultBufferHeight>0);
		con.m_sbi.dwSize = MakeCoord(rcCon.right,mp_RCon->mn_DefaultBufferHeight);
	}
	else
	{
		con.m_sbi.dwSize = MakeCoord(rcCon.right,rcCon.bottom);
	}
	con.nTextWidth = rcCon.right;
	con.nTextHeight = rcCon.bottom;

	con.nTopVisibleLine = 0;

	con.m_sbi.wAttributes = 7;
	con.m_sbi.srWindow.Left = con.m_sbi.srWindow.Top = 0;
	con.m_sbi.srWindow.Right = rcCon.right-1; con.m_sbi.srWindow.Bottom = rcCon.bottom-1;
	con.m_sbi.dwMaximumWindowSize = con.m_sbi.dwSize;
	con.m_ci.dwSize = 15; con.m_ci.bVisible = TRUE;

	if (!InitBuffers(0))
		return FALSE;

	return TRUE;
}

BOOL CRealBuffer::InitBuffers(DWORD OneBufferSize)
{
	// Эта функция должна вызываться только в MonitorThread.
	// Тогда блокировка буфера не потребуется
	
	#ifdef _DEBUG
	DWORD dwCurThId = GetCurrentThreadId();
	_ASSERTE((mp_RCon->mn_MonitorThreadID==0 || dwCurThId==mp_RCon->mn_MonitorThreadID || mp_RCon->mb_WaitingRootStartup)
		|| ((m_Type==rbt_DumpScreen || m_Type==rbt_Alternative || m_Type==rbt_Selection || m_Type==rbt_Find) && gpConEmu->isMainThread()));
	#endif

	BOOL lbRc = FALSE;
	int nNewWidth = 0, nNewHeight = 0;
	DWORD nScroll = 0;
	HEAPVAL;

	if (!GetConWindowSize(con.m_sbi, &nNewWidth, &nNewHeight, &nScroll))
		return FALSE;

	if (OneBufferSize)
	{
		if ((nNewWidth * nNewHeight * sizeof(*con.pConChar)) != OneBufferSize)
		{
			// Это может случиться во время пересоздания консоли (когда фар падал)
			//// Это может случиться во время ресайза
			//nNewWidth = nNewWidth;
			_ASSERTE((nNewWidth * nNewHeight * sizeof(*con.pConChar)) == OneBufferSize);
		}
		else if (con.nTextWidth == nNewWidth && con.nTextHeight == nNewHeight)
		{
			// Не будем зря передергивать буферы и прочее
			if (con.pConChar!=NULL && con.pConAttr!=NULL && con.pDataCmp!=NULL)
			{
				return TRUE;
			}
		}

		//if ((nNewWidth * nNewHeight * sizeof(*con.pConChar)) != OneBufferSize)
		//    return FALSE;
	}

	// Если требуется увеличить или создать (первично) буфера
	if (!con.pConChar || (con.nTextWidth*con.nTextHeight) < (nNewWidth*nNewHeight))
	{
		// Exclusive(!) Lock
		MSectionLock sc; sc.Lock(&csCON, TRUE);

		HEAPVAL;
		con.LastStartInitBuffersTick = GetTickCount();

		Assert(con.bInGetConsoleData==FALSE);

		if (con.pConChar)
			{ Free(con.pConChar); con.pConChar = NULL; }

		if (con.pConAttr)
			{ Free(con.pConAttr); con.pConAttr = NULL; }

		if (con.pDataCmp)
			{ Free(con.pDataCmp); con.pDataCmp = NULL; }

		//if (con.pCmp)
		//	{ Free(con.pCmp); con.pCmp = NULL; }
		HEAPVAL;
		int cchCharMax = nNewWidth * nNewHeight * 2;
		con.pConChar = (TCHAR*)Alloc(cchCharMax, sizeof(*con.pConChar));
		con.pConAttr = (WORD*)Alloc(cchCharMax, sizeof(*con.pConAttr));
		con.pDataCmp = (CHAR_INFO*)Alloc((nNewWidth * nNewHeight)*sizeof(CHAR_INFO),1);
		//con.pCmp = (CESERVER_REQ_CONINFO_DATA*)Alloc((nNewWidth * nNewHeight)*sizeof(CHAR_INFO)+sizeof(CESERVER_REQ_CONINFO_DATA),1);

		BYTE nDefTextAttr = (mp_RCon->GetDefaultBackColorIdx()<<4)|(mp_RCon->GetDefaultTextColorIdx());
		wmemset((wchar_t*)con.pConAttr, nDefTextAttr, cchCharMax);

		con.LastEndInitBuffersTick = GetTickCount();

		Assert(con.bInGetConsoleData==FALSE);

		sc.Unlock();
		_ASSERTE(con.pConChar!=NULL);
		_ASSERTE(con.pConAttr!=NULL);
		_ASSERTE(con.pDataCmp!=NULL);
		//_ASSERTE(con.pCmp!=NULL);
		HEAPVAL
		lbRc = con.pConChar!=NULL && con.pConAttr!=NULL && con.pDataCmp!=NULL;
	}
	else if (con.nTextWidth!=nNewWidth || con.nTextHeight!=nNewHeight)
	{
		HEAPVAL
		MSectionLock sc; sc.Lock(&csCON);
		int cchCharMax = nNewWidth * nNewHeight * 2;
		memset(con.pConChar, 0, cchCharMax * sizeof(*con.pConChar));
		//memset(con.pConAttr, 0, cchCharMax * sizeof(*con.pConAttr));
		BYTE nDefTextAttr = (mp_RCon->GetDefaultBackColorIdx()<<4)|(mp_RCon->GetDefaultTextColorIdx());
		wmemset((wchar_t*)con.pConAttr, nDefTextAttr, cchCharMax);
		memset(con.pDataCmp, 0, (nNewWidth * nNewHeight) * sizeof(CHAR_INFO));
		//memset(con.pCmp->Buf, 0, (nNewWidth * nNewHeight) * sizeof(CHAR_INFO));
		sc.Unlock();
		HEAPVAL
		lbRc = TRUE;
	}
	else
	{
		lbRc = TRUE;
	}

	HEAPVAL
#ifdef _DEBUG

	if (nNewHeight == 158)
		nNewHeight = 158;

#endif
	con.nTextWidth = nNewWidth;
	con.nTextHeight = nNewHeight;
	// чтобы передернулись положения панелей и прочие флаги
	if (this == mp_RCon->mp_ABuf)
		mp_RCon->mb_DataChanged = TRUE;
	//else
	//{
	//	// Чтобы не забыть, что после переключения буфера надо флажок выставить
	//	_ASSERTE(this == mp_RCon->mp_ABuf);
	//}

	//InitDC(false,true);
	return lbRc;
}

void CRealBuffer::PreFillBuffers()
{
	if (con.pConChar && con.pConAttr)
	{
		MSectionLock sc; sc.Lock(&csCON, TRUE);

		size_t cchCharMax = (con.nTextWidth*con.nTextHeight) * 2;

		BYTE nDefTextAttr = (mp_RCon->GetDefaultBackColorIdx()<<4)|(mp_RCon->GetDefaultTextColorIdx());
		wmemset((wchar_t*)con.pConAttr, nDefTextAttr, cchCharMax);

		sc.Unlock();
	}
}

SHORT CRealBuffer::GetBufferWidth()
{
	return con.m_sbi.dwSize.X;
}

SHORT CRealBuffer::GetBufferHeight()
{
	return con.m_sbi.dwSize.Y;
}

SHORT CRealBuffer::GetBufferPosX()
{
	_ASSERTE(con.m_sbi.srWindow.Left==0);
	return con.m_sbi.srWindow.Left;
}

SHORT CRealBuffer::GetBufferPosY()
{
	//-- по факту - не интересно, ассерт часто возникал в процессе обильного скролла-вывода текста
	//-- (con.nTopVisibleLine и csbi.srWindow.Top оказывались рассинхронизированными)
	//#if defined(_DEBUG)
	#if 0
	USHORT nTop = con.nTopVisibleLine;
	CONSOLE_SCREEN_BUFFER_INFO csbi = con.m_sbi;
	bool bInScroll = mp_RCon->InScroll();
	if (nTop != csbi.srWindow.Top)
	{
		TODO("Пока не переделал скролл на пайп - данные могут приходить немного с запаздываением");
		_ASSERTE(nTop == csbi.srWindow.Top || bInScroll);
		bool bDbgShowConsole = false;
		if (bDbgShowConsole)
		{
			mp_RCon->ShowConsole(1);
			mp_RCon->ShowConsole(0);
		}
	}
	#endif

	return con.nTopVisibleLine;
}

int CRealBuffer::TextWidth()
{
	_ASSERTE(this!=NULL);

	if (!this) return MIN_CON_WIDTH;

	if (con.nChange2TextWidth!=-1 && con.nChange2TextWidth!=0)
		return con.nChange2TextWidth;

	_ASSERTE(con.nTextWidth>=MIN_CON_WIDTH);
	return con.nTextWidth;
}

int CRealBuffer::GetTextWidth()
{
	_ASSERTE(this!=NULL);

	if (!this) return MIN_CON_WIDTH;

	_ASSERTE(con.nTextWidth>=MIN_CON_WIDTH && con.nTextWidth<=400);
	return con.nTextWidth;
}

int CRealBuffer::TextHeight()
{
	_ASSERTE(this!=NULL);

	if (!this) return MIN_CON_HEIGHT;

	uint nRet = 0;
	#ifdef _DEBUG
	//struct RConInfo lcon = con;
	#endif

	if (con.nChange2TextHeight!=-1 && con.nChange2TextHeight!=0)
		nRet = con.nChange2TextHeight;
	else
		nRet = con.nTextHeight;

	#ifdef _DEBUG
	if (nRet <= MIN_CON_HEIGHT || nRet > 200)
	{
		_ASSERTE(nRet>=MIN_CON_HEIGHT && nRet<=200);
	}
	#endif

	return nRet;
}

int CRealBuffer::GetTextHeight()
{
	_ASSERTE(this!=NULL);

	if (!this) return MIN_CON_HEIGHT;
	
	_ASSERTE(con.nTextHeight>=MIN_CON_HEIGHT && con.nTextHeight<=200);
	return con.nTextHeight;
}

int CRealBuffer::GetWindowWidth()
{
	int nWidth = con.m_sbi.srWindow.Right - con.m_sbi.srWindow.Left + 1;
	_ASSERTE(nWidth>=MIN_CON_WIDTH && nWidth <= 400);
	return nWidth;
}

int CRealBuffer::GetWindowHeight()
{
	int nHeight = con.m_sbi.srWindow.Bottom - con.m_sbi.srWindow.Top + 1;
	_ASSERTE(nHeight>=MIN_CON_HEIGHT && nHeight <= 300);
	return nHeight;
}

int CRealBuffer::BufferHeight(uint nNewBufferHeight/*=0*/)
{
	int nBufferHeight = 0;

	if (con.bBufferHeight)
	{
		if (nNewBufferHeight)
		{
			nBufferHeight = nNewBufferHeight;
			con.nBufferHeight = nNewBufferHeight;
		}
		else if (con.nBufferHeight)
		{
			nBufferHeight = con.nBufferHeight;
		}
		else if (mp_RCon->mn_DefaultBufferHeight)
		{
			nBufferHeight = mp_RCon->mn_DefaultBufferHeight;
		}
		else
		{
			nBufferHeight = gpSet->DefaultBufferHeight;
		}
	}
	else
	{
		// После выхода из буферного режима сбрасываем запомненную высоту, чтобы
		// в следующий раз установить высоту из настроек (gpSet->DefaultBufferHeight)
		_ASSERTE(nNewBufferHeight == 0);
		con.nBufferHeight = 0;
	}

	return nBufferHeight;
}

BOOL CRealBuffer::GetConWindowSize(const CONSOLE_SCREEN_BUFFER_INFO& sbi, int* pnNewWidth, int* pnNewHeight, DWORD* pnScroll)
{
	TODO("Заменить на вызов ::GetConWindowSize из WinObjects.cpp");
	DWORD nScroll = 0; // enum RealBufferScroll
	int nNewWidth = 0, nNewHeight = 0;
	
	// Функция возвращает размер ОКНА (видимой области), то есть буфер может быть больше
	
	int nCurWidth = TextWidth();
	if (sbi.dwSize.X == nCurWidth)
	{
		nNewWidth = sbi.dwSize.X;
	}
	else
	{
		TODO("Добавить в con флажок горизонтальной прокрутки");
		TODO("и вообще, заменить на вызов ::GetConWindowSize из WinObjects.cpp");
		if (sbi.dwSize.X <= EvalBufferTurnOnSize(max(nCurWidth,con.crMaxSize.X)))
		{
			nNewWidth = sbi.dwSize.X;
		}
		else
		{
			// 111125 было "nNewWidth  = sbi.dwSize.X;", но так игнорируется горизонтальная прокрутка
			nNewWidth = sbi.srWindow.Right - sbi.srWindow.Left + 1;
			_ASSERTE(nNewWidth <= sbi.dwSize.X);
		}
	}
	// Флаги
	if (/*(sbi.dwSize.X > sbi.dwMaximumWindowSize.X) ||*/ (nNewWidth < sbi.dwSize.X))
	{
		// для проверки условий
		//_ASSERTE((sbi.dwSize.X > sbi.dwMaximumWindowSize.X) && (nNewWidth < sbi.dwSize.X));
		nScroll |= rbs_Horz;
	}


	int nCurHeight = TextHeight();
	if (sbi.dwSize.Y == nCurHeight)
	{
		nNewHeight = sbi.dwSize.Y;
	}
	else
	{
		if ((con.bBufferHeight && (sbi.dwSize.Y > nCurHeight))
			|| (sbi.dwSize.Y > EvalBufferTurnOnSize(nCurHeight)))
		{
			nNewHeight = sbi.srWindow.Bottom - sbi.srWindow.Top + 1;
		}
		else
		{
			nNewHeight = sbi.dwSize.Y;
		}
	}
	// Флаги
	if (/*(sbi.dwSize.Y > sbi.dwMaximumWindowSize.Y) ||*/ (nNewHeight < sbi.dwSize.Y))
	{
		// для проверки условий
		//_ASSERTE((sbi.dwSize.Y >= sbi.dwMaximumWindowSize.Y) && (nNewHeight < sbi.dwSize.Y));
		nScroll |= rbs_Vert;
	}

	// Validation
	if ((nNewWidth <= 0) || (nNewHeight <= 0))
	{
		Assert(nNewWidth>0 && nNewHeight>0);
		return FALSE;
	}
	
	// Result
	if (pnNewWidth)
		*pnNewWidth = nNewWidth;
	if (pnNewHeight)
		*pnNewHeight = nNewHeight;
	if (pnScroll)
		*pnScroll = nScroll;
	
	return TRUE;
	
	//BOOL lbBufferHeight = this->isScroll();

	//// Проверка режимов прокрутки
	//if (!lbBufferHeight)
	//{
	//	if (sbi.dwSize.Y > sbi.dwMaximumWindowSize.Y)
	//	{
	//		lbBufferHeight = TRUE; // однозначное включение прокрутки
	//	}
	//}

	//if (lbBufferHeight)
	//{
	//	if (sbi.srWindow.Top == 0
	//	        && sbi.dwSize.Y == (sbi.srWindow.Bottom + 1)
	//	  )
	//	{
	//		lbBufferHeight = FALSE; // однозначное вЫключение прокрутки
	//	}
	//}

	//// Теперь собственно размеры
	//if (!lbBufferHeight)
	//{
	//	nNewHeight =  sbi.dwSize.Y;
	//}
	//else
	//{
	//	// Это может прийти во время смены размера
	//	if ((sbi.srWindow.Bottom - sbi.srWindow.Top + 1) < MIN_CON_HEIGHT)
	//		nNewHeight = con.nTextHeight;
	//	else
	//		nNewHeight = sbi.srWindow.Bottom - sbi.srWindow.Top + 1;
	//}

	//WARNING("Здесь нужно выполнить коррекцию, если nNewHeight велико - включить режим BufferHeight");

	//if (pbBufferHeight)
	//	*pbBufferHeight = lbBufferHeight;

	//_ASSERTE(nNewWidth>=MIN_CON_WIDTH && nNewHeight>=MIN_CON_HEIGHT);

	//if (!nNewWidth || !nNewHeight)
	//{
	//	Assert(nNewWidth && nNewHeight);
	//	return FALSE;
	//}

	////if (nNewWidth < sbi.dwSize.X)
	////    nNewWidth = sbi.dwSize.X;
	//return TRUE;
}

// Изменение значений переменной (флаг включенного скролла)
void CRealBuffer::SetBufferHeightMode(BOOL abBufferHeight, BOOL abIgnoreLock/*=FALSE*/)
{
	if (mb_BuferModeChangeLocked)
	{
		if (!abIgnoreLock)
		{
			//_ASSERTE(mb_BuferModeChangeLocked==FALSE || abIgnoreLock);
			return;
		}
	}

	con.bBufferHeight = abBufferHeight;

	if (mp_RCon->isActive())
		OnBufferHeight();
}

// Вызывается из TabBar->ConEmu
void CRealBuffer::ChangeBufferHeightMode(BOOL abBufferHeight)
{
	if (abBufferHeight && gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 1)
	{
		// Win7 BUGBUG: Issue 192: падение Conhost при turn bufferheight ON
		// http://code.google.com/p/conemu-maximus5/issues/detail?id=192
		return;
		//const SHORT nMaxBuf = 600;
		//if (nNewBufHeightSize > nMaxBuf && mp_RCon->isFar())
		//	nNewBufHeightSize = nMaxBuf;
	}

	_ASSERTE(!mb_BuferModeChangeLocked);
	BOOL lb = mb_BuferModeChangeLocked; mb_BuferModeChangeLocked = TRUE;
	con.bBufferHeight = abBufferHeight;

	// Если при запуске было "conemu.exe /bufferheight 0 ..."
	if (abBufferHeight /*&& !con.nBufferHeight*/)
	{
		// Если пользователь меняет высоту буфера в диалоге настроек
		con.nBufferHeight = gpSet->DefaultBufferHeight;

		if (con.nBufferHeight<300)
		{
			_ASSERTE(con.nBufferHeight>=300);
			con.nBufferHeight = max(300,con.nTextHeight*2);
		}
	}

	ResetLastMousePos();

	USHORT nNewBufHeightSize = abBufferHeight ? con.nBufferHeight : 0;
	SetConsoleSize(TextWidth(), TextHeight(), nNewBufHeightSize, CECMD_SETSIZESYNC);
	mb_BuferModeChangeLocked = lb;
}

void CRealBuffer::SetChange2Size(int anChange2TextWidth, int anChange2TextHeight)
{
	#ifdef _DEBUG
	if (anChange2TextHeight > 200)
	{
		_ASSERTE(anChange2TextHeight <= 200);
	}
	#endif

	con.nChange2TextWidth = anChange2TextWidth;
	con.nChange2TextHeight = anChange2TextHeight;
}

BOOL CRealBuffer::isBuferModeChangeLocked()
{
	return mb_BuferModeChangeLocked;
}

BOOL CRealBuffer::BuferModeChangeLock()
{
	BOOL lbNeedUnlock = !mb_BuferModeChangeLocked;
	mb_BuferModeChangeLocked = TRUE;
	return lbNeedUnlock;
}

void CRealBuffer::BuferModeChangeUnlock()
{
	mb_BuferModeChangeLocked = FALSE;
}

// По con.m_sbi проверяет, включена ли прокрутка
BOOL CRealBuffer::CheckBufferSize()
{
	bool lbForceUpdate = false;

	if (!this)
		return false;

	if (mb_BuferModeChangeLocked)
		return false;

	//if (con.m_sbi.dwSize.X>(con.m_sbi.srWindow.Right-con.m_sbi.srWindow.Left+1)) {
	//  DEBUGLOGFILE("Wrong screen buffer width\n");
	//  // Окошко консоли почему-то схлопнулось по горизонтали
	//  WARNING("пока убрал для теста");
	//  //MOVEWINDOW(mp_RCon->hConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
	//} else {
	// BufferHeight может меняться и из плагина (например, DVDPanel), во время работы фара, или в других приложения (wmic)
	BOOL lbTurnedOn = BufferHeightTurnedOn(&con.m_sbi);

	if (!lbTurnedOn && con.bBufferHeight)
	{
		// может быть консольная программа увеличила буфер самостоятельно?
		// TODO: отключить прокрутку!!!
		SetBufferHeightMode(FALSE);
		//UpdateScrollInfo(); -- зовется в ApplyConsoleInfo()
		lbForceUpdate = true;
	}
	else if (lbTurnedOn && !con.bBufferHeight)
	{
		SetBufferHeightMode(TRUE);
		//UpdateScrollInfo(); -- зовется в ApplyConsoleInfo()
		lbForceUpdate = true;
	}

	//TODO: А если высота буфера вдруг сменилась из самой консольной программы?
	//if ((BufferHeight == 0) && (con.m_sbi.dwSize.Y>(con.m_sbi.srWindow.Bottom-con.m_sbi.srWindow.Top+1))) {
	//  TODO("это может быть консольная программа увеличила буфер самостоятельно!")
	//      DEBUGLOGFILE("Wrong screen buffer height\n");
	//  // Окошко консоли почему-то схлопнулось по вертикали
	//  WARNING("пока убрал для теста");
	//  //MOVEWINDOW(mp_RCon->hConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
	//}
	//TODO: Можно бы перенести в ConEmuC, если нужно будет
	//// При выходе из FAR -> CMD с BufferHeight - смена QuickEdit режима
	//DWORD mode = 0;
	//BOOL lb = FALSE;
	//if (BufferHeight) {
	//  //TODO: похоже, что для BufferHeight это вызывается постоянно?
	//  //lb = GetConsoleMode(hConIn(), &mode);
	//  mode = GetConsoleMode();
	//  if (con.m_sbi.dwSize.Y>(con.m_sbi.srWindow.Bottom-con.m_sbi.srWindow.Top+1)) {
	//      // Буфер больше высоты окна
	//      mode |= ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE|ENABLE_EXTENDED_FLAGS;
	//  } else {
	//      // Буфер равен высоте окна (значит ФАР запустился)
	//      mode &= ~(ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE);
	//      mode |= ENABLE_EXTENDED_FLAGS;
	//  }
	//  TODO("SetConsoleMode");
	//  //lb = SetConsoleMode(hConIn(), mode);
	//}
	return lbForceUpdate;
}

BOOL CRealBuffer::LoadDataFromSrv(DWORD CharCount, CHAR_INFO* pData)
{
	if (m_Type != rbt_Primary)
	{
		_ASSERTE(m_Type == rbt_Primary);
		//if (dump.NeedApply)
		//{
		//	dump.NeedApply = FALSE;

		//	// Создание буферов
		//	if (!InitBuffers(0))
		//	{
		//		_ASSERTE(FALSE);
		//	}
		//	else
		//	// И копирование
		//	{
		//		wchar_t*  pszSrc = dump.pszBlock1;
		//		CharAttr* pcaSrc = dump.pcaBlock1;
		//		wchar_t*  pszDst = con.pConChar;
		//		TODO("Хорошо бы весь расширенный буфер тут хранить, а не только CHAR_ATTR");
		//		WORD*     pnaDst = con.pConAttr;
		//		
		//		DWORD dwConDataBufSize = dump.crSize.X * dump.crSize.Y;
		//		wmemmove(pszDst, pszSrc, dwConDataBufSize);

		//		// Расфуговка буфера CharAttr на консольные атрибуты
		//		for (DWORD n = 0; n < dwConDataBufSize; n++, pcaSrc++, pnaDst++)
		//		{
		//			*pnaDst = (pcaSrc->nForeIdx & 0xF0) | ((pcaSrc->nBackIdx & 0xF0) << 4);
		//		}
		//	}

		//	return TRUE;
		//}
		return FALSE; // Изменений нет
	}

	HEAPVAL;
	BOOL lbScreenChanged = FALSE;
	wchar_t* lpChar = con.pConChar;
	WORD* lpAttr = con.pConAttr;

	if (mp_RCon->mb_ResetStatusOnConsoleReady)
	{
		mp_RCon->mb_ResetStatusOnConsoleReady = false;
		mp_RCon->ms_ConStatus[0] = 0;
	}


	_ASSERTE(sizeof(*con.pDataCmp) == sizeof(*pData));

	lbScreenChanged = (memcmp(con.pDataCmp, pData, CharCount*sizeof(*pData)) != 0);

	if (lbScreenChanged)
	{
		if (IFLOGCONSOLECHANGE)
		{
			char sInfo[128];
			_wsprintfA(sInfo, SKIPLEN(countof(sInfo)) "DataCmp was changed, width=%u, height=%u, count=%u", con.nTextWidth, con.nTextHeight, CharCount);

			const CHAR_INFO* lp1 = con.pDataCmp;
			const CHAR_INFO* lp2 = pData;
			INT_PTR idx = -1;

			for (DWORD n = 0; n < CharCount; n++, lp1++, lp2++)
			{
				if (memcmp(lp1, lp2, sizeof(*lp2)) != 0)
				{
					idx = (lp1 - con.pDataCmp);
					int y = con.nTextWidth ? (idx / con.nTextWidth) : 0;
					int x = con.nTextWidth ? (idx - y * con.nTextWidth) : idx;

					_wsprintfA(sInfo+strlen(sInfo), SKIPLEN(32) ", posY=%i, posX=%i", y, x);

					break;
				}
			}
            
            if (idx == -1)
            {
            	lstrcatA(sInfo, ", failed to find change idx");
            }

			LOGCONSOLECHANGE(sInfo);
		}

		//con.pCopy->crBufSize = con.pCmp->crBufSize;
		//memmove(con.pCopy->Buf, con.pCmp->Buf, CharCount*sizeof(CHAR_INFO));
		memmove(con.pDataCmp, pData, CharCount*sizeof(CHAR_INFO));
		HEAPVAL;
		CHAR_INFO* lpCur = con.pDataCmp;
		wchar_t ch;

		// Расфуговка буфера CHAR_INFO на текст и атрибуты
		for (DWORD n = 0; n < CharCount; n++, lpCur++)
		{
			TODO("OPTIMIZE: *(lpAttr++) = lpCur->Attributes;");
			*(lpAttr++) = lpCur->Attributes;
			TODO("OPTIMIZE: ch = lpCur->Char.UnicodeChar;");
			ch = lpCur->Char.UnicodeChar;
			//2009-09-25. Некоторые (старые?) программы умудряются засунуть в консоль символы (ASC<32)
			//            их нужно заменить на юникодные аналоги
			*(lpChar++) = (ch < 32) ? gszAnalogues[(WORD)ch] : ch;
		}

		HEAPVAL
	}

	return lbScreenChanged;
}

BOOL CRealBuffer::IsTrueColorerBufferChanged()
{
	BOOL lbChanged = FALSE;
	AnnotationHeader aHdr;
	int nCmp = 0;

	if (!gpSet->isTrueColorer || !mp_RCon->mp_TrueColorerData)
		goto wrap;

	// Проверка буфера TrueColor
	if (!mp_RCon->m_TrueColorerMap.GetTo(&aHdr, sizeof(aHdr)))
		goto wrap;

	// Check counter
	if (mp_RCon->m_TrueColorerHeader.flushCounter == aHdr.flushCounter)
		goto wrap;

	// Compare data
	EnterCriticalSection(&m_TrueMode.csLock);
	{
		int nCurMax = max(con.nTextWidth*con.nTextHeight,aHdr.bufferSize);
		size_t cbCurSize = nCurMax*sizeof(*m_TrueMode.mp_Cmp);

		if (!m_TrueMode.mp_Cmp || (nCurMax > m_TrueMode.nCmpMax))
		{
			SafeFree(m_TrueMode.mp_Cmp);
			m_TrueMode.nCmpMax = nCurMax;
            m_TrueMode.mp_Cmp = (AnnotationInfo*)malloc(cbCurSize);
            nCmp = 2;
		}
		else
		{
			nCmp = memcmp(m_TrueMode.mp_Cmp, mp_RCon->mp_TrueColorerData, cbCurSize);
		}

		if ((nCmp != 0) && m_TrueMode.mp_Cmp)
		{
			memcpy(m_TrueMode.mp_Cmp, mp_RCon->mp_TrueColorerData, cbCurSize);
		}
	}
	LeaveCriticalSection(&m_TrueMode.csLock);


	if (nCmp != 0)
	{
		lbChanged = TRUE;

		if (IFLOGCONSOLECHANGE)
		{
			char szDbgSize[128]; _wsprintfA(szDbgSize, SKIPLEN(countof(szDbgSize)) "ApplyConsoleInfo: TrueColorer.flushCounter(%u) -> changed(%u)", mp_RCon->m_TrueColorerHeader.flushCounter, aHdr.flushCounter);
			LOGCONSOLECHANGE(szDbgSize);
		}
	}

	// Save counter to avoid redundant checks
	mp_RCon->m_TrueColorerHeader = aHdr;
wrap:
	return lbChanged;
}

BOOL CRealBuffer::ApplyConsoleInfo()
{
	BOOL bBufRecreated = FALSE;
	BOOL lbChanged = FALSE;
	
	#ifdef _DEBUG
	if (mp_RCon->mb_DebugLocked)
		return FALSE;
	int nConNo = gpConEmu->isVConValid(mp_RCon->mp_VCon);
	nConNo = nConNo;
	#endif

	if (!mp_RCon->isServerAvailable())
	{
		// Сервер уже закрывается. попытка считать данные из консоли может привести к зависанию!
		SetEvent(mp_RCon->mh_ApplyFinished);
		return FALSE;
	}

	ResetEvent(mp_RCon->mh_ApplyFinished);
	const CESERVER_REQ_CONINFO_INFO* pInfo = NULL;
	CESERVER_REQ_HDR cmd; ExecutePrepareCmd(&cmd, CECMD_CONSOLEDATA, sizeof(cmd));

	if (mp_RCon->mb_SwitchActiveServer)
	{
		// Skip this step. Waiting for new console server.
	}
	else if (!mp_RCon->m_ConsoleMap.IsValid())
	{
		_ASSERTE(mp_RCon->m_ConsoleMap.IsValid());
	}
	else if (!mp_RCon->m_GetDataPipe.Transact(&cmd, sizeof(cmd), (const CESERVER_REQ_HDR**)&pInfo) || !pInfo)
	{
		#ifdef _DEBUG
		if (mp_RCon->m_GetDataPipe.GetErrorCode() == ERROR_PIPE_NOT_CONNECTED)
		{
			int nDbg = ERROR_PIPE_NOT_CONNECTED;
		}
		else if (mp_RCon->m_GetDataPipe.GetErrorCode() == ERROR_BAD_PIPE)
		{
			int nDbg = ERROR_BAD_PIPE;
		}
		else
		{
			wchar_t szInfo[128];
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"mp_RCon->m_GetDataPipe.Transact failed, code=%i\r\n", (int)mp_RCon->m_GetDataPipe.GetErrorCode());
			wchar_t* pszFull = lstrmerge(szInfo, mp_RCon->m_GetDataPipe.GetErrorText());
			MBoxA(pszFull);
			SafeFree(pszFull);
		}
		#endif
	}
	else if (pInfo->cmd.cbSize < sizeof(CESERVER_REQ_CONINFO_INFO))
	{
		_ASSERTE(pInfo->cmd.cbSize >= sizeof(CESERVER_REQ_CONINFO_INFO));
	}
	else
		//if (!mp_ConsoleInfo->mp_RCon->hConWnd || !mp_ConsoleInfo->nCurDataMapIdx) {
		//	_ASSERTE(mp_ConsoleInfo->mp_RCon->hConWnd && mp_ConsoleInfo->nCurDataMapIdx);
		//} else
	{
		//if (mn_LastConsoleDataIdx != mp_ConsoleInfo->nCurDataMapIdx) {
		//	ReopenMapData();
		//}
		DWORD nPID = GetCurrentProcessId();
		DWORD nMapGuiPID = mp_RCon->m_ConsoleMap.Ptr()->nGuiPID;

		if (nPID != nMapGuiPID)
		{
			// Если консоль запускалась как "-new_console" то nMapGuiPID может быть еще 0?
			// Хотя, это может случиться только если батник запущен из консоли НЕ прицепленной к GUI ConEmu.
			if (nMapGuiPID != 0)
			{
				_ASSERTE(nMapGuiPID == nPID);
			}
		}

		#ifdef _DEBUG
		HWND hWnd = pInfo->hConWnd;
		if (!hWnd || (hWnd != mp_RCon->hConWnd))
		{
			// Wine bug ? Incomplete packet?
			_ASSERTE(hWnd!=NULL);
			_ASSERTE(hWnd==mp_RCon->hConWnd);
		}
		#endif

		//if (mp_RCon->hConWnd != hWnd) {
		//    SetHwnd ( hWnd ); -- низя. Maps уже созданы!
		//}
		// 3
		// Здесь у нас реальные процессы консоли, надо обновиться
		if (mp_RCon->ProcessUpdate(pInfo->nProcesses, countof(pInfo->nProcesses)))
		{
			//120325 - нет смысла перерисовывать консоль, если данные в ней не менялись.
			//  это приводит 1) к лишнему мельканию; 2) глюкам отрисовки в запущенных консольных приложениях
			//lbChanged = TRUE; // если сменился статус (Far/не Far) - перерисовать на всякий случай
		}
		// Теперь нужно открыть секцию - начинаем изменение переменных класса
		MSectionLock sc;
		// 4
		DWORD dwCiSize = pInfo->dwCiSize;

		if (dwCiSize != 0)
		{
			_ASSERTE(dwCiSize == sizeof(con.m_ci));

			if (memcmp(&con.m_ci, &pInfo->ci, sizeof(con.m_ci))!=0)
			{
				LOGCONSOLECHANGE("ApplyConsoleInfo: CursorInfo changed");
				lbChanged = TRUE;
			}

			con.m_ci = pInfo->ci;
		}

		// 5, 6, 7
		con.m_dwConsoleCP = pInfo->dwConsoleCP;
		con.m_dwConsoleOutputCP = pInfo->dwConsoleOutputCP;

		if (con.m_dwConsoleMode != pInfo->dwConsoleMode)
		{
			if (ghOpWnd && mp_RCon->isActive())
				gpSetCls->UpdateConsoleMode(pInfo->dwConsoleMode);
		}

		con.m_dwConsoleMode = pInfo->dwConsoleMode;
		// 8
		DWORD dwSbiSize = pInfo->dwSbiSize;
		int nNewWidth = 0, nNewHeight = 0;

		if (dwSbiSize != 0)
		{
			HEAPVAL
			_ASSERTE(dwSbiSize == sizeof(con.m_sbi));

			if (memcmp(&con.m_sbi, &pInfo->sbi, sizeof(con.m_sbi))!=0)
			{
				LOGCONSOLECHANGE("ApplyConsoleInfo: ScreenBufferInfo changed");
				lbChanged = TRUE;

				//if (mp_RCon->isActive())
				//	gpConEmu->UpdateCursorInfo(&pInfo->sbi, pInfo->sbi.dwCursorPosition, pInfo->ci);
			}

			#ifdef _DEBUG
			wchar_t szCursorDbg[255]; szCursorDbg[0] = 0;
			if (pInfo->sbi.dwCursorPosition.X != con.m_sbi.dwCursorPosition.X || pInfo->sbi.dwCursorPosition.Y != con.m_sbi.dwCursorPosition.Y)
				_wsprintf(szCursorDbg, SKIPLEN(countof(szCursorDbg)) L"CursorPos changed to %ux%u. ", pInfo->sbi.dwCursorPosition.X, pInfo->sbi.dwCursorPosition.Y);
			else
				_wsprintf(szCursorDbg, SKIPLEN(countof(szCursorDbg)) L"CursorPos is %ux%u. ", pInfo->sbi.dwCursorPosition.X, pInfo->sbi.dwCursorPosition.Y);
			#endif

			CONSOLE_SCREEN_BUFFER_INFO lsbi = pInfo->sbi;
			// Если мышкой тащат ползунок скроллера - не менять TopVisible
			if (mp_RCon->InScroll())
			{
				UINT nY = lsbi.srWindow.Bottom - lsbi.srWindow.Top;
				lsbi.srWindow.Top = max(0,min(con.nTopVisibleLine,lsbi.dwSize.Y-nY-1));
				lsbi.srWindow.Bottom = lsbi.srWindow.Top + nY;
				#ifdef _DEBUG
				int l = lstrlen(szCursorDbg);
				_wsprintf(szCursorDbg+l, SKIPLEN(countof(szCursorDbg)-l) L"Visible rect locked to {%ux%u-%ux%u), Top=%u. ", lsbi.srWindow.Left, lsbi.srWindow.Top, lsbi.srWindow.Right, lsbi.srWindow.Bottom, con.nTopVisibleLine);
				#endif
			}
			#ifdef _DEBUG
			else if (memcmp(&pInfo->sbi.srWindow, &con.m_sbi.srWindow, sizeof(con.m_sbi.srWindow)))
			{
				if (pInfo->sbi.dwCursorPosition.X != con.m_sbi.dwCursorPosition.X || pInfo->sbi.dwCursorPosition.Y != con.m_sbi.dwCursorPosition.Y)
				{
					int l = lstrlen(szCursorDbg);
					_wsprintf(szCursorDbg+l, SKIPLEN(countof(szCursorDbg)-l) L"Visible rect changed to {%ux%u-%ux%u). ", pInfo->sbi.srWindow.Left, pInfo->sbi.srWindow.Top, pInfo->sbi.srWindow.Right, pInfo->sbi.srWindow.Bottom);
				}
			}

			if (szCursorDbg[0])
			{
				wcscat_c(szCursorDbg, L"\n");
				DEBUGSTRCURSORPOS(szCursorDbg);
			}
			#endif

			con.m_sbi = lsbi;

			DWORD nScroll;
			if (GetConWindowSize(con.m_sbi, &nNewWidth, &nNewHeight, &nScroll))
			{
				//if (con.bBufferHeight != (nNewHeight < con.m_sbi.dwSize.Y))
				BOOL lbTurnedOn = BufferHeightTurnedOn(&con.m_sbi);
				if (con.bBufferHeight != lbTurnedOn)
					SetBufferHeightMode(lbTurnedOn);

				//  TODO("Включить прокрутку? или оно само?");
				if (nNewWidth != con.nTextWidth || nNewHeight != con.nTextHeight)
				{
					if (IFLOGCONSOLECHANGE)
					{
						char szDbgSize[128]; _wsprintfA(szDbgSize, SKIPLEN(countof(szDbgSize)) "ApplyConsoleInfo: SizeWasChanged(cx=%i, cy=%i)", nNewWidth, nNewHeight);
						LOGCONSOLECHANGE(szDbgSize);
					}
					
					#ifdef _DEBUG
					wchar_t szDbgSize[128]; _wsprintf(szDbgSize, SKIPLEN(countof(szDbgSize)) L"ApplyConsoleInfo.SizeWasChanged(cx=%i, cy=%i)", nNewWidth, nNewHeight);
					DEBUGSTRSIZE(szDbgSize);
					#endif

					bBufRecreated = TRUE; // Смена размера, буфер пересоздается
					//sc.Lock(&csCON, TRUE);
					//WARNING("может не заблокировалось?");
					InitBuffers(nNewWidth*nNewHeight*2);
				}
			}


			#ifdef SHOW_AUTOSCROLL
			if (gpSetCls->AutoScroll)
				con.nTopVisibleLine = con.m_sbi.srWindow.Top;
			#else
			// Если мышкой тащат ползунок скроллера - не менять TopVisible
			if (!mp_RCon->InScroll())
				con.nTopVisibleLine = con.m_sbi.srWindow.Top;
			#endif

			HEAPVAL
		}

		//DWORD dwCharChanged = pInfo->ConInfo.RgnInfo.dwRgnInfoSize;
		//BOOL  lbDataRecv = FALSE;
		if (/*mp_ConsoleData &&*/ nNewWidth && nNewHeight)
		{
			// Это может случиться во время пересоздания консоли (когда фар падал)
			// или при изменении параметров экрана (Aero->Standard)
			// или при закрытии фара (он "восстанавливает" размер консоли)
			_ASSERTE(nNewWidth == pInfo->crWindow.X && nNewHeight == pInfo->crWindow.Y);
			// 10
			//DWORD MaxBufferSize = pInfo->nCurDataMaxSize;
			//if (MaxBufferSize != 0) {

			//// Не будем гонять зря данные по пайпу, если изменений нет
			//if (mp_RCon->mn_LastConsolePacketIdx != pInfo->nPacketId)

			// Если вместе с заголовком пришли измененные данные
			if (pInfo->nDataShift && pInfo->nDataCount)
			{
				LOGCONSOLECHANGE("ApplyConsoleInfo: Console contents received");

				mp_RCon->mn_LastConsolePacketIdx = pInfo->nPacketId;
				DWORD CharCount = pInfo->nDataCount;

				
				#ifdef _DEBUG
				if (CharCount != (nNewWidth * nNewHeight))
				{
					// Это может случиться во время пересоздания консоли (когда фар падал)
					_ASSERTE(CharCount == (nNewWidth * nNewHeight));
				}
				#endif


				DWORD OneBufferSize = CharCount * sizeof(wchar_t);
				CHAR_INFO *pData = (CHAR_INFO*)(((LPBYTE)pInfo) + pInfo->nDataShift);
				// Проверка размера!
				DWORD nCalcCount = (pInfo->cmd.cbSize - pInfo->nDataShift) / sizeof(CHAR_INFO);

				if (nCalcCount != CharCount)
				{
					_ASSERTE(nCalcCount == CharCount);

					if (nCalcCount < CharCount)
						CharCount = nCalcCount;
				}

				//MSectionLock sc2; sc2.Lock(&csCON);
				sc.Lock(&csCON);
				HEAPVAL;

				if (InitBuffers(OneBufferSize))
				{
					if (LoadDataFromSrv(CharCount, pData))
					{
						LOGCONSOLECHANGE("ApplyConsoleInfo: InitBuffers&LoadDataFromSrv -> changed");
						lbChanged = TRUE;
					}
				}

				HEAPVAL;
			}
		}

		TODO("Во время ресайза консоль может подглючивать - отдает не то что нужно...");
		//_ASSERTE(*con.pConChar!=ucBoxDblVert);

		// пока выполяется SetConsoleSizeSrv в другой нити Нельзя сбрасывать эти переменные!
		if (!con.bLockChange2Text)
		{
			con.nChange2TextWidth = -1;
			con.nChange2TextHeight = -1;
		}

#ifdef _DEBUG
		wchar_t szCursorInfo[60];
		_wsprintf(szCursorInfo, SKIPLEN(countof(szCursorInfo)) L"Cursor (X=%i, Y=%i, Vis:%i, H:%i)\n",
		          con.m_sbi.dwCursorPosition.X, con.m_sbi.dwCursorPosition.Y,
		          con.m_ci.bVisible, con.m_ci.dwSize);
		DEBUGSTRPKT(szCursorInfo);

		// Данные уже должны быть заполнены, и там не должно быть лажы
		if (con.pConChar)
		{
			BOOL lbDataValid = TRUE; uint n = 0;
			_ASSERTE(con.nTextWidth == con.m_sbi.dwSize.X);
			uint TextLen = con.nTextWidth * con.nTextHeight;

			while(n<TextLen)
			{
				if (con.pConChar[n] == 0)
				{
					lbDataValid = FALSE; break;
				}
				else if (con.pConChar[n] != L' ')
				{
					// 0 - может быть только для пробела. Иначе символ будет скрытым, чего по идее, быть не должно
					if (con.pConAttr[n] == 0)
					{
						lbDataValid = FALSE; break;
					}
				}

				n++;
			}
		}

		//_ASSERTE(lbDataValid);
		HEAPVAL;
#endif

		// Проверка буфера TrueColor
		if (!lbChanged && gpSet->isTrueColorer && mp_RCon->mp_TrueColorerData)
		{
			if (IsTrueColorerBufferChanged())
			{
				lbChanged = TRUE;
			}
		}

		if (lbChanged)
		{
			// По con.m_sbi проверяет, включена ли прокрутка
			CheckBufferSize();
			HEAPVAL;
		}

		sc.Unlock();
	}

	SetEvent(mp_RCon->mh_ApplyFinished);

	if (lbChanged)
	{
		mp_RCon->mb_DataChanged = TRUE; // Переменная используется внутри класса
		con.bConsoleDataChanged = TRUE; // А эта - при вызовах из CVirtualConsole

		//if (mp_RCon->isActive()) -- mp_RCon->isActive() проверит сама UpdateScrollInfo, а скроллбар может быть и в видимой но НЕ активной консоли
		mp_RCon->UpdateScrollInfo();
	}

	UNREFERENCED_PARAMETER(bBufRecreated);

	return lbChanged;
}

// По переданному CONSOLE_SCREEN_BUFFER_INFO определяет, включена ли прокрутка
// static
BOOL CRealBuffer::BufferHeightTurnedOn(CONSOLE_SCREEN_BUFFER_INFO* psbi)
{
	BOOL lbTurnedOn = FALSE;
	TODO("!!! Скорректировать");

	if (psbi->dwSize.Y <= (psbi->srWindow.Bottom - psbi->srWindow.Top + 1))
	{
		_ASSERTE(psbi->dwSize.Y == (psbi->srWindow.Bottom - psbi->srWindow.Top + 1))
		// высота окна == высоте буфера,
		lbTurnedOn = FALSE;
	}
	else
	{
		//Issue 509: Могут быть различные варианты, когда меняется ВИДИМАЯ область,
		//           но не меняется высота буфера. В этом случае, буфер включать нельзя!
		TODO("Тут нужно бы сравнивать не с TextHeight(), а с высотой буфера. Но она инициализируется пока только для режима с прокруткой.");
		int nHeight = TextHeight();
		// Высота буфера 'намного' больше высоты НАШЕГО окна
		if (con.m_sbi.dwSize.Y > EvalBufferTurnOnSize(nHeight))
			lbTurnedOn = TRUE;
	}
	//else if (con.m_sbi.dwSize.Y < (con.m_sbi.srWindow.Bottom-con.m_sbi.srWindow.Top+10))
	//{
	//	// Высота окна примерно равна высоте буфера
	//	lbTurnedOn = FALSE;
	//}
	//else if (con.m_sbi.dwSize.Y>(con.m_sbi.srWindow.Bottom-con.m_sbi.srWindow.Top+10))
	//{
	//	// Высота буфера 'намного' больше высоты окна
	//	lbTurnedOn = TRUE;
	//}

	// однако, если высота слишком велика для отображения в GUI окне - нужно включить BufferHeight
	if (!lbTurnedOn)
	{
		//TODO: однако, если высота слишком велика для отображения в GUI окне - нужно включить BufferHeight
	}

	return lbTurnedOn;
}

void CRealBuffer::OnBufferHeight()
{
	if (!this)
	{
		_ASSERTE(this);
		return;
	}

	// При смене высоты буфера - сбросить последнее мышиное событие
	ResetLastMousePos();

	gpConEmu->OnBufferHeight();
}

// Если включена прокрутка - скорректировать индекс ячейки из экранных в буферные
COORD CRealBuffer::ScreenToBuffer(COORD crMouse)
{
	if (!this)
		return crMouse;

	if (isScroll())
	{
		// Прокрутка может быть заблокирована?
		_ASSERTE((con.nTopVisibleLine == con.m_sbi.srWindow.Top) || (con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION));

		crMouse.X += con.m_sbi.srWindow.Left;
		crMouse.Y += con.m_sbi.srWindow.Top;
	}

	return crMouse;
}

COORD CRealBuffer::BufferToScreen(COORD crMouse, bool bVertOnly /*= false*/)
{
	if (!this)
		return crMouse;

	if (isScroll())
	{
		if (!bVertOnly)
			crMouse.X = max(0,crMouse.X-con.m_sbi.srWindow.Left);
		crMouse.Y = max(0,crMouse.Y-con.m_sbi.srWindow.Top);
	}

	return crMouse;
}

ExpandTextRangeType CRealBuffer::GetLastTextRangeType()
{
	if (!this)
		return etr_None;
	return con.etr.etrLast;
}

bool CRealBuffer::ResetLastMousePos()
{
	mcr_LastMousePos = MakeCoord(-1,-1);

	bool bChanged = (con.etr.etrLast != etr_None);

	if (bChanged)
		StoreLastTextRange(etr_None);

	return bChanged;
}

bool CRealBuffer::LookupFilePath(LPCWSTR asFileOrPath, wchar_t* pszPath, size_t cchPathMax)
{
	_ASSERTE(pszPath!=NULL && asFileOrPath!=NULL && cchPathMax>=MAX_PATH);

	lstrcpyn(pszPath, asFileOrPath, (int)cchPathMax);

	TODO("Проверка наличия собственно полного пути");
	TODO("попытка склейки с текущим путем в приложении");
	TODO("попытка поиска файла в подпапках текущего пути");
	TODO("Коррекция регистра символов, чтобы левота в историю фара не попадала");

	if (FileExists(pszPath))
		return true;

	return false;
}

bool CRealBuffer::ProcessFarHyperlink(bool bUpdateScreen)
{
	// Console may scrolls after last check time
	if (bUpdateScreen)
	{
		POINT ptCur = {}; GetCursorPos(&ptCur);
		RECT rcClient = {}; GetWindowRect(mp_RCon->VCon()->GetView(), &rcClient);
		if (!PtInRect(&rcClient, ptCur))
		{
			if (mcr_LastMousePos.X != -1)
				ResetLastMousePos();
		}
		else
		{
			ptCur.x -= rcClient.left; ptCur.y -= rcClient.top;
			COORD crMouse = ScreenToBuffer(mp_RCon->VCon()->ClientToConsole(ptCur.x, ptCur.y));
			mcr_LastMousePos = crMouse;
		}
	}

	if ((mcr_LastMousePos.X == -1) && (con.etr.etrLast != etr_None))
	{
		ResetLastMousePos();
	}

	return ProcessFarHyperlink(WM_MOUSEMOVE, mcr_LastMousePos, bUpdateScreen);
}

bool CRealBuffer::CanProcessHyperlink(const COORD& crMouse)
{
	if (!mp_RCon->isActive())
		return false;

	// Disallow hyperlinks on Far panels
	if (mp_RCon->isFar())
	{
		RECT rc;
		COORD crMap = BufferToScreen(crMouse);
		if (isLeftPanel() && GetPanelRect(FALSE, &rc, TRUE, TRUE) && CoordInRect(crMap, rc))
			return false;
		if (isRightPanel() && GetPanelRect(TRUE, &rc, TRUE, TRUE) && CoordInRect(crMap, rc))
			return false;
	}

	// Allow
	return true;
}

bool CRealBuffer::ProcessFarHyperlink(UINT messg, COORD crFrom, bool bUpdateScreen)
{
	if (!mp_RCon->IsFarHyperlinkAllowed(false))
		return false;
		
	bool lbProcessed = false;
	
	// переходим на абсолютные координаты
	COORD crStart = crFrom; // MakeCoord(crFrom.X - con.m_sbi.srWindow.Left, crFrom.Y - con.m_sbi.srWindow.Top);

	// Во время скролла координата может быстро улететь
	if ((crStart.Y < 0 || crStart.Y >= GetBufferHeight())
		|| (crStart.X < 0 || crStart.X >= GetBufferWidth()))
	{
		_ASSERTE((crStart.X==-1 && crStart.Y==-1) && "Must not get here");

		bool bChanged = ResetLastMousePos();
		return bChanged;
	}


	COORD crEnd = crStart;
	wchar_t szText[MAX_PATH+10];
	ExpandTextRangeType rc = CanProcessHyperlink(crStart)
		? ExpandTextRange(crStart, crEnd, etr_FileAndLine, szText, countof(szText))
		: etr_None;
	if (memcmp(&crStart, &con.etr.mcr_FileLineStart, sizeof(crStart)) != 0
		|| memcmp(&crEnd, &con.etr.mcr_FileLineEnd, sizeof(crStart)) != 0)
	{
		con.etr.mcr_FileLineStart = crStart;
		con.etr.mcr_FileLineEnd = crEnd;
		// bUpdateScreen если вызов идет из GetConsoleData для коррекции отдаваемых координат
		if (bUpdateScreen)
		{
			UpdateSelection(); // обновить на экране
		}
	}
	
	if ((rc == etr_FileAndLine) || (rc == etr_Url))
	{
		if ((messg == WM_LBUTTONDOWN) && *szText)
		{
			if (rc == etr_Url)
			{
				int iRc = (int)ShellExecute(ghWnd, L"open", szText, NULL, NULL, SW_SHOWNORMAL);
				if (iRc <= 32)
				{
					DisplayLastError(szText, iRc, MB_ICONSTOP, L"URL open failed");
				}
			}
			else if (rc == etr_FileAndLine)
			{
				// Найти номер строки
				CESERVER_REQ_FAREDITOR cmd = {sizeof(cmd)};
				int nLen = lstrlen(szText)-1;
				if (szText[nLen] == L')')
				{
					szText[nLen] = 0;
					nLen--;
				}
				while ((nLen > 0)
					&& (((szText[nLen-1] >= L'0') && (szText[nLen-1] <= L'9'))
						|| ((szText[nLen-1] == L',') && ((szText[nLen] >= L'0') && (szText[nLen] <= L'9')))))
				{
					nLen--;
				}
				if (nLen < 3)
				{
					_ASSERTE(nLen >= 3);
				}
				else
				{ // 1.c:3: 
					wchar_t* pszEnd;
					cmd.nLine = wcstol(szText+nLen, &pszEnd, 10);
					if (pszEnd && (*pszEnd == L',') && isDigit(*(pszEnd+1)))
						cmd.nColon = wcstol(pszEnd+1, &pszEnd, 10);
					if (cmd.nColon < 1)
						cmd.nColon = 1;
					szText[nLen-1] = 0;
					while ((pszEnd = wcschr(szText, L'/')) != NULL)
						*pszEnd = L'\\'; // заменить прямые слеши на обратные

					//lstrcpyn(cmd.szFile, szText, countof(cmd.szFile));
					LookupFilePath(szText/*name from console*/, cmd.szFile/*full path*/, countof(cmd.szFile));
					
					TODO("Для удобства, если лог открыт в редакторе, или пропустить мышь, или позвать макрос");
					// Только нужно учесть, что текущий таб может быть редактором, но открыт UserScreen (CtrlO)
					
					// Проверить, может уже открыт таб с этим файлом?
					LPCWSTR pszFileName = wcsrchr(cmd.szFile, L'\\');
					if (!pszFileName) pszFileName = cmd.szFile; else pszFileName++;
					CVirtualConsole* pVCon = NULL;

					//// Сброс подчерка, а то при возврате в консоль,
					//// когда модификатор уже отпущен, остает артефакт...
					//StoreLastTextRange(etr_None);
					//UpdateSelection();

					int liActivated = gpConEmu->mp_TabBar->ActiveTabByName(fwt_Editor|fwt_ActivateFound|fwt_PluginRequired, pszFileName, &pVCon);
					
					if (liActivated == -2)
					{
						// Нашли, но активировать нельзя, TabBar должен был показать всплывающую подсказку с ошибкой
						_ASSERTE(FALSE);
					}
					else
					{
						if (liActivated >= 0)
						{
							// Нашли, активировали, нужно только на строку перейти
							if (cmd.nLine > 0)
							{
								wchar_t szMacro[96];
								if (mp_RCon->m_FarInfo.FarVer.dwVerMajor == 1)
									_wsprintf(szMacro, SKIPLEN(countof(szMacro)) L"@$if(Editor) AltF8 \"%i:%i\" Enter $end", cmd.nLine, cmd.nColon);
								else
									_wsprintf(szMacro, SKIPLEN(countof(szMacro)) L"@$if(Editor) AltF8 print(\"%i:%i\") Enter $end", cmd.nLine, cmd.nColon);
								_ASSERTE(pVCon!=NULL);

								// -- Послать что-нибудь в консоль, чтобы фар ушел из UserScreen открытого через редактор?
								//PostMouseEvent(WM_LBUTTONUP, 0, crFrom);

								// Ок, переход на строку (макрос)
								pVCon->RCon()->PostMacro(szMacro, TRUE);
							}
						}
						else
						{
							CVConGuard VCon;
							if (!CVConGroup::isFarExist(fwt_NonModal|fwt_PluginRequired, NULL, &VCon))
							{
								if (gpSet->sFarGotoEditor && *gpSet->sFarGotoEditor)
								{
									wchar_t szRow[32], szCol[32];
									_wsprintf(szRow, SKIPLEN(countof(szRow)) L"%u", cmd.nLine);
									_wsprintf(szCol, SKIPLEN(countof(szCol)) L"%u", cmd.nColon);
									//LPCWSTR pszVar[] = {L"%1", L"%2", L"%3"};
									LPCWSTR pszVal[] = {szRow, szCol, cmd.szFile};
									//_ASSERTE(countof(pszVar)==countof(pszVal));
									wchar_t* pszCmd = ExpandMacroValues(gpSet->sFarGotoEditor, pszVal, countof(pszVal));
									if (!pszCmd)
									{
										DisplayLastError(L"Invalid command specified in \"External editor\"", -1);
									}
									else
									{
										RConStartArgs args;
										args.pszSpecialCmd = pszCmd; pszCmd = NULL;
										args.pszStartupDir = mp_RCon->m_Args.pszStartupDir ? lstrdup(mp_RCon->m_Args.pszStartupDir) : NULL;
										args.bRunAsAdministrator = mp_RCon->m_Args.bRunAsAdministrator;
										args.bForceUserDialog = mp_RCon->m_Args.bForceUserDialog || mp_RCon->m_Args.bRunAsRestricted || (mp_RCon->m_Args.pszUserName != NULL);
										args.bBufHeight = TRUE;
										//args.eConfirmation = RConStartArgs::eConfNever;

										gpConEmu->CreateCon(&args);
									}
								}
								else
								{
									DisplayLastError(L"Available Far Manager was not found in open tabs", -1);
								}
							}
							else
							{
								// -- Послать что-нибудь в консоль, чтобы фар ушел из UserScreen открытого через редактор?
								//PostMouseEvent(WM_LBUTTONUP, 0, crFrom);

								// Prepared, можно звать плагин
								VCon->RCon()->PostCommand(CMD_OPENEDITORLINE, sizeof(cmd), &cmd);
								gpConEmu->Activate(VCon.VCon());
							}
						}
					}
				}
			}
		}
		lbProcessed = true;
	}

	return lbProcessed;
}

void CRealBuffer::ShowKeyBarHint(WORD nID)
{
	if ((nID > 0) && (nID <= countof(gnKeyBarFlags)))
	{
		// Нужен какой-то безопасный способ "обновить" кейбар, но так,
		// чтобы не сработали макросы, назначенные на одиночные модификаторы!
		//INPUT_RECORD r = {KEY_EVENT};
		//r.Event.MouseEvent.dwMousePosition.X = nID;
		//r.Event.MouseEvent.dwControlKeyState = gnKeyBarFlags[nID - 1];
		//r.Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
		//mp_RCon->PostConsoleEvent(&r);
		mp_RCon->PostKeyPress(VK_RWIN, gnKeyBarFlags[nID - 1], 0);
	}
}

// If console IN selection - do autoscroll when mouse if outside or near VCon edges
// If NO selection present - ensure that coordinates are inside our VCon (otherwise - exit)
bool CRealBuffer::PatchMouseCoords(int& x, int& y, COORD& crMouse)
{
	// Хорошо бы скроллить выделение если мышка рядом с краем, а не только 'за'. Иначе в Fullsreen может быть сложно...
	bool bMouse = ((con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION) != 0);

	if ((crMouse.X >= con.m_sbi.srWindow.Left) && (crMouse.X <= con.m_sbi.srWindow.Right)
		&& ((!bMouse && (crMouse.Y >= con.m_sbi.srWindow.Top) && (crMouse.Y <= con.m_sbi.srWindow.Bottom))
		|| (bMouse && (crMouse.Y > con.m_sbi.srWindow.Top) && (crMouse.Y < con.m_sbi.srWindow.Bottom))))
	{
		DEBUGSTRMOUSE(L"Nothing need to be patched, coordinates are OK (1)\n");
		return true;
	}

	int nVConHeight = mp_RCon->VCon()->Height;

	if (bMouse
		&& ((crMouse.Y == con.m_sbi.srWindow.Top) && (y >= SELMOUSEAUTOSCROLLPIX))
			|| ((crMouse.Y == con.m_sbi.srWindow.Bottom) && (y <= (nVConHeight-SELMOUSEAUTOSCROLLPIX))))
	{
		DEBUGSTRMOUSE(L"Nothing need to be patched, coordinates are OK (2)\n");
		return true;
	}

	// In mouse selection only
	if (!(con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION))
	{
		// No selection, mouse outside of VCon, skip this message!
		DEBUGSTRMOUSE(L"!!! No mouse selection or mouse outside of VCon, message skipped !!!\n");
		return false;
	}

	// Avoid too fast scrolling
	DWORD nCurTick = GetTickCount();
	DWORD nDelta = (nCurTick - con.m_SelLastScrollCheck);
	if (nDelta < SELMOUSEAUTOSCROLLDELTA)
	{
		DEBUGSTRMOUSE(L"Mouse selection autoscroll skipped (waiting 100ms)\n");
		return false;
	}
	con.m_SelLastScrollCheck = nCurTick;

	// Lets scroll window content
	if ((crMouse.Y < con.m_sbi.srWindow.Top) || (y < SELMOUSEAUTOSCROLLPIX))
	{
		DEBUGSTRMOUSE(L"Autoscrolling buffer one line up\n");
		crMouse.Y = max(0,con.m_sbi.srWindow.Top-1);
		OnScroll(SB_LINEUP);
		y = 0;
	}
	else if ((crMouse.Y > con.m_sbi.srWindow.Bottom) || (y > (nVConHeight-SELMOUSEAUTOSCROLLPIX)))
	{
		DEBUGSTRMOUSE(L"Autoscrolling buffer one line down\n");
		crMouse.Y = min(con.m_sbi.srWindow.Bottom+1,con.m_sbi.dwSize.Y-1);
		OnScroll(SB_LINEDOWN);
		y = (nVConHeight - 1);
	}

	// And patch X coords (while we are not support X scrolling yet)
	if (crMouse.X < con.m_sbi.srWindow.Left)
	{
		crMouse.X = con.m_sbi.srWindow.Left;
		x = 0;
	}
	else if (crMouse.X > con.m_sbi.srWindow.Right)
	{
		crMouse.X = con.m_sbi.srWindow.Right;
		x = (mp_RCon->VCon()->Width - 1);
	}

	return true;
}

void CRealBuffer::OnTimerCheckSelection()
{
	// In mouse selection only
	if (!(con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION))
		return;

	POINT ptCur = {}; GetCursorPos(&ptCur);
	MapWindowPoints(NULL, mp_RCon->VCon()->GetView(), &ptCur, 1);
	int nVConHeight = mp_RCon->VCon()->Height;
	
	if ((ptCur.y < SELMOUSEAUTOSCROLLPIX) || (ptCur.y > (nVConHeight-SELMOUSEAUTOSCROLLPIX)))
	{
		COORD crMouse = ScreenToBuffer(mp_RCon->VCon()->ClientToConsole(ptCur.x, ptCur.y));
		int x = ptCur.x, y = ptCur.y;
		WPARAM wParam = (isPressed(VK_LBUTTON) ? MK_LBUTTON : 0)
			| (isPressed(VK_RBUTTON) ? MK_RBUTTON : 0)
			| (isPressed(VK_MBUTTON) ? MK_MBUTTON : 0);
		OnMouse(WM_MOUSEMOVE, wParam, x, y, crMouse);
	}
}

// x,y - экранные координаты
// crMouse - ScreenToBuffer
// Возвращает true, если мышку обработал "сам буфер"
bool CRealBuffer::OnMouse(UINT messg, WPARAM wParam, int x, int y, COORD crMouse, bool abFromTouch /*= false*/)
{
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL                  0x020E
#endif

	#ifdef _DEBUG
	wchar_t szDbgInfo[200]; _wsprintf(szDbgInfo, SKIPLEN(countof(szDbgInfo)) L"RBuf::OnMouse %s XY={%i,%i} CR={%i,%i}%s SelFlags=x%08X\n",
		messg==WM_MOUSEMOVE?L"WM_MOUSEMOVE":
		messg==WM_LBUTTONDOWN?L"WM_LBUTTONDOWN":messg==WM_LBUTTONUP?L"WM_LBUTTONUP":messg==WM_LBUTTONDBLCLK?L"WM_LBUTTONDBLCLK":
		messg==WM_RBUTTONDOWN?L"WM_RBUTTONDOWN":messg==WM_RBUTTONUP?L"WM_RBUTTONUP":messg==WM_RBUTTONDBLCLK?L"WM_RBUTTONDBLCLK":
		messg==WM_MBUTTONDOWN?L"WM_MBUTTONDOWN":messg==WM_MBUTTONUP?L"WM_MBUTTONUP":messg==WM_MBUTTONDBLCLK?L"WM_MBUTTONDBLCLK":
		messg==WM_MOUSEWHEEL?L"WM_MOUSEWHEEL":messg==WM_MOUSEHWHEEL?L"WM_MOUSEHWHEEL":
		L"{OtherMsg}", x,y, crMouse.X,crMouse.Y,(abFromTouch?L" Touch":L""), con.m_sel.dwFlags);
	DEBUGSTRMOUSE(szDbgInfo);
	#endif

	// Ensure that coordinates are correct
	if (!PatchMouseCoords(x, y, crMouse))
		return false;

	mcr_LastMousePos = crMouse;

	bool bSelAllowed = isSelectionAllowed();

	if (bSelAllowed)
	{
		if (messg == WM_LBUTTONDOWN)
		{
			// Начало обработки выделения
			if (OnMouseSelection(messg, wParam, x, y))
				return true;
		}

		// Если выделение еще не начато, но удерживается модификатор - игнорировать WM_MOUSEMOVE
		if (messg == WM_MOUSEMOVE && !con.m_sel.dwFlags)
		{
			if (gpConEmu->isSelectionModifierPressed(false))
			{
				// Пропустить, пользователь собирается начать выделение, не посылать движение мыши в консоль
				return true;
			}
		}

		if (((gpSet->isCTSRBtnAction == 2) || ((gpSet->isCTSRBtnAction == 3) && !isSelectionPresent()))
				&& (messg == WM_RBUTTONDOWN || messg == WM_RBUTTONUP)
		        && ((gpSet->isCTSActMode == 2 && mp_RCon->isBufferHeight() && !mp_RCon->isFarBufferSupported())
		            || (gpSet->isCTSActMode == 1 && gpSet->IsModifierPressed(vkCTSVkAct, true))))
		{
			if (messg == WM_RBUTTONUP) mp_RCon->Paste();

			return true;
		}

		if (((gpSet->isCTSMBtnAction == 2) || ((gpSet->isCTSMBtnAction == 3) && !isSelectionPresent()))
				&& (messg == WM_MBUTTONDOWN || messg == WM_MBUTTONUP)
		        && ((gpSet->isCTSActMode == 2 && mp_RCon->isBufferHeight() && !mp_RCon->isFarBufferSupported())
		            || (gpSet->isCTSActMode == 1 && gpSet->IsModifierPressed(vkCTSVkAct, true))))
		{
			if (messg == WM_MBUTTONUP) mp_RCon->Paste();

			return true;
		}
	}

	// Поиск и подсветка файлов с ошибками типа
	// .\realconsole.cpp(8104) : error ...
	if ((con.m_sel.dwFlags == 0) && mp_RCon->IsFarHyperlinkAllowed(false))
	{
		if (messg == WM_MOUSEMOVE || messg == WM_LBUTTONDOWN || messg == WM_LBUTTONUP || messg == WM_LBUTTONDBLCLK)
		{
			if (ProcessFarHyperlink(messg, crMouse, true))
			{
				// Пускать или нет событие мыши в консоль?
				// Лучше наверное не пускать, а то вьювер может заклинить на прокрутке, например
				return true;
			}
		}
	}

	bool lbFarBufferSupported = mp_RCon->isFarBufferSupported();
	bool lbMouseSendAllowed = mp_RCon->isSendMouseAllowed();
	bool lbMouseOverScroll = false;
	// Проверять мышку имеет смысл только если она пересылается в фар, а не работает на прокрутку
	if ((messg == WM_MOUSEWHEEL) || (messg == WM_MOUSEHWHEEL))
	{
		if (con.bBufferHeight && (m_Type == rbt_Primary) && lbFarBufferSupported && lbMouseSendAllowed)
		{
			lbMouseOverScroll = mp_RCon->mp_VCon->CheckMouseOverScroll(true);
		}
	}

	if (con.bBufferHeight && ((m_Type != rbt_Primary) || !lbFarBufferSupported || !lbMouseSendAllowed || lbMouseOverScroll))
	{
		if (messg == WM_MOUSEWHEEL)
		{
			SHORT nDir = (SHORT)HIWORD(wParam);
			BOOL lbCtrl = isPressed(VK_CONTROL);

			UINT nCount = abFromTouch ? 1 : gpConEmu->mouse.GetWheelScrollLines();

			if (nDir > 0)
			{
				OnScroll(lbCtrl ? SB_PAGEUP : SB_LINEUP, -1, nCount);
			}
			else if (nDir < 0)
			{
				OnScroll(lbCtrl ? SB_PAGEDOWN : SB_LINEDOWN, -1, nCount);
			}

			return true; // уже обработано
		}
		else if (messg == WM_MOUSEHWHEEL)
		{
			TODO("WM_MOUSEHWHEEL - горизонтальная прокрутка");
			_ASSERTE(FALSE && "Horz scrolling! WM_MOUSEHWHEEL");
			//return true; -- когда будет готово - return true;
		}

		if (!isConSelectMode())
		{
			// Пропустить в консоль, если это НЕ Far
			return (m_Type != rbt_Primary);
		}
	}

	//if (isConSelectMode()) -- это неправильно. она реагирует и на фаровский граббер (чтобы D&D не взлетал)
	if (con.m_sel.dwFlags != 0)
	{
		// Ручная обработка выделения, на консоль полагаться не следует...
		OnMouseSelection(messg, wParam, x, y);
		return true;
	}

	// При правом клике на KeyBar'е - показать PopupMenu с вариантами модификаторов F-клавиш
	TODO("Пока только для Far Manager?");
	if ((m_Type == rbt_Primary) && (gpSet->isKeyBarRClick)
		&& ((messg == WM_RBUTTONDOWN && (crMouse.Y == (GetTextHeight() - 1)) && mp_RCon->isFarKeyBarShown())
			|| ((messg == WM_MOUSEMOVE || messg == WM_RBUTTONUP) && con.bRClick4KeyBar)))
	{
		if (messg == WM_RBUTTONDOWN)
		{
			MSectionLock csData;
			wchar_t* pChar = NULL;
			int nLen = 0;
			if (GetConsoleLine(crMouse.Y, &pChar, &nLen, &csData) && (*pChar == L'1'))
			{
				// Т.к. ширина баров переменная, ищем
				int x, k, px = 0, vk = 0;
				for (x = 1, k = 2; x < nLen; x++)
				{
					if (pChar[x] < L'0' || pChar[x] > L'9')
						continue;
					if (k <= 9)
					{
						if ((((int)pChar[x] - L'0') == k) && (pChar[x-1] == L' ')
							&& (pChar[x+1] < L'0' || pChar[x+1] > L'9'))
						{
							if ((crMouse.X <= (x - 1)) && (crMouse.X >= px))
							{
								vk = VK_F1 + (k - 2);
								break;
							}
							px = x - 1;
							k++;
						}
					}
					else if (k <= 12)
					{
						if ((pChar[x] == L'1') && (((int)pChar[x+1] - L'0') == (k-10)) && (pChar[x-1] == L' ')
							&& (pChar[x+2] < L'0' || pChar[x+2] > L'9'))
						{
							if ((crMouse.X <= (x - 1)) && (crMouse.X >= px))
							{
								px++;
								vk = VK_F1 + (k - 2);
								break;
							}
							px = x - 1;
							k++;
						}
					}
					else
					{
						px++;
						vk = VK_F12;
						break;
					}
				}

				if (vk)
				{
					con.bRClick4KeyBar = TRUE;
					con.crRClick4KeyBar = crMouse;
					con.ptRClick4KeyBar = mp_RCon->mp_VCon->ConsoleToClient((vk==VK_F1)?(px+1):(px+2), crMouse.Y);
					ClientToScreen(mp_RCon->GetView(), &con.ptRClick4KeyBar);
					con.nRClickVK = vk;
					return true;
				}
			}
		}
		else if (messg == WM_RBUTTONUP)
		{
			_ASSERTE(con.bRClick4KeyBar);
			//Run!
			HMENU h = CreatePopupMenu();
			wchar_t szText[128];
			for (size_t i = 0; i < countof(gnKeyBarFlags); i++)
			{
				*szText = 0;
				if (gnKeyBarFlags[i] & LEFT_CTRL_PRESSED)
					wcscat_c(szText, L"Ctrl-");
				if (gnKeyBarFlags[i] & LEFT_ALT_PRESSED)
					wcscat_c(szText, L"Alt-");
				if (gnKeyBarFlags[i] & RIGHT_CTRL_PRESSED)
					wcscat_c(szText, L"RCtrl-");
				if (gnKeyBarFlags[i] & RIGHT_ALT_PRESSED)
					wcscat_c(szText, L"RAlt-");
				if (gnKeyBarFlags[i] & SHIFT_PRESSED)
					wcscat_c(szText, L"Shift-");
				_wsprintf(szText+lstrlen(szText), SKIPLEN(8) L"F%i", (con.nRClickVK - VK_F1 + 1));

				AppendMenu(h, MF_STRING|((!(i % 4)) ? MF_MENUBREAK : 0), i+1, szText);
			}

			RECT rcExcl = {con.ptRClick4KeyBar.x-1,con.ptRClick4KeyBar.y-1,con.ptRClick4KeyBar.x+1,con.ptRClick4KeyBar.y+1};

			int i = gpConEmu->mp_Menu->trackPopupMenu(tmp_KeyBar, h, TPM_LEFTALIGN|TPM_BOTTOMALIGN|/*TPM_NONOTIFY|*/TPM_RETURNCMD|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
						con.ptRClick4KeyBar.x, con.ptRClick4KeyBar.y, ghWnd, &rcExcl);
			DestroyMenu(h);

			if ((i > 0) && (i <= (int)countof(gnKeyBarFlags)))
			{
				i--;
				mp_RCon->PostKeyPress(con.nRClickVK, gnKeyBarFlags[i], 0);
			}

			//Done
			con.bRClick4KeyBar = FALSE;
			return true;
		}
		else if (messg == WM_MOUSEMOVE)
		{
			_ASSERTE(con.bRClick4KeyBar);
			TODO("«Отпустить» если был сдвиг?");
			return true; // не пропускать в консоль
		}
	}
	else if (con.bRClick4KeyBar)
	{
		con.bRClick4KeyBar = FALSE;
	}
	
	// Пропускать мышь в консоль только если буфер реальный
	return (m_Type != rbt_Primary);
}

BOOL CRealBuffer::GetRBtnDrag(COORD* pcrMouse)
{
	if (pcrMouse)
		*pcrMouse = con.crRBtnDrag;
	return con.bRBtnDrag;
}

void CRealBuffer::SetRBtnDrag(BOOL abRBtnDrag, const COORD* pcrMouse)
{
	con.bRBtnDrag = abRBtnDrag;

	if (pcrMouse)
		con.crRBtnDrag = *pcrMouse;
}

// x,y - экранные координаты
bool CRealBuffer::OnMouseSelection(UINT messg, WPARAM wParam, int x, int y)
{
	if (TextWidth()<=1 || TextHeight()<=1)
	{
		_ASSERTE(TextWidth()>1 && TextHeight()>1);
		return false;
	}

	// Получить известные координаты символов
	COORD crScreen = mp_RCon->mp_VCon->ClientToConsole(x,y);
	MinMax(crScreen.X, 0, TextWidth()-1);
	MinMax(crScreen.Y, 0, TextHeight()-1);

	COORD cr = ScreenToBuffer(crScreen);


	if (messg == WM_LBUTTONDOWN)
	{
		BOOL lbStreamSelection = FALSE;
		BYTE vkMod = 0; // Если удерживается модификатор - его нужно "отпустить" в консоль
		bool bTripleClick = (con.m_sel.dwFlags & CONSOLE_DBLCLICK_SELECTION) && ((GetTickCount() - con.m_SelDblClickTick) <= GetDoubleClickTime());

		if (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION|CONSOLE_BLOCK_SELECTION))
		{
			// Выделение запущено из меню
			lbStreamSelection = (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION)) == CONSOLE_TEXT_SELECTION;
		}
		else
		{
			DWORD nPressed = gpConEmu->isSelectionModifierPressed(true);
			if (nPressed)
			{
				// OK
				lbStreamSelection = ((nPressed & CONSOLE_TEXT_SELECTION) != 0);
				vkMod = gpSet->GetHotkeyById(lbStreamSelection ? vkCTSVkText : vkCTSVkBlock);
			}
			else
			{
				return false; // модификатор не разрешен
			}
		}

		con.m_sel.dwFlags &= ~CONSOLE_KEYMOD_MASK;
		con.m_sel.dwFlags |= ((DWORD)vkMod) << 24;

		COORD crTo = cr;
		if (bTripleClick)
		{
			cr.X = 0;
			crTo.X = GetBufferWidth()-1;
		}

		// Если дошли сюда - значит или модификатор нажат, или из меню выделение запустили
		StartSelection(lbStreamSelection, cr.X, cr.Y, TRUE, bTripleClick ? WM_LBUTTONDBLCLK : WM_LBUTTONDOWN, bTripleClick ? &crTo : NULL);

		//WARNING!!! После StartSelection - ничего не делать! Мог смениться буфер!

		return true;
	}
	else if ((messg == WM_LBUTTONDBLCLK) && (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION|CONSOLE_BLOCK_SELECTION)))
	{
		// Выделить слово под курсором (как в обычной консоли)
		BOOL lbStreamSelection = (con.m_sel.dwFlags & CONSOLE_TEXT_SELECTION) == CONSOLE_TEXT_SELECTION;
		
		// Нужно получить координаты слова
		COORD crFrom = cr, crTo = cr;
		ExpandTextRange(crFrom/*[In/Out]*/, crTo/*[Out]*/, etr_Word);
		
		// Выполнить выделение
		StartSelection(lbStreamSelection, crFrom.X, crFrom.Y, TRUE, WM_LBUTTONDBLCLK, &crTo);

		// Сейчас кнопка мышки отпущена, сброс
		con.m_sel.dwFlags &= ~CONSOLE_MOUSE_DOWN;

		//WARNING!!! После StartSelection - ничего не делать! Мог смениться буфер!
		return true;
	}
	else if (
		((messg == WM_MOUSEMOVE) && (con.m_sel.dwFlags & CONSOLE_MOUSE_DOWN))
		|| ((messg == WM_LBUTTONUP) && (con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION))
		)
	{
		// При LBtnUp может быть несколько вариантов
		// 1. Cick. После WM_LBUTTONUP нужно дождаться таймера и позвать DoSelectionCopy.
		// 2. DblClick. Генерятся WM_LBUTTONUP WM_LBUTTONDBLCLK. Выделение "Слова".
		// 3. TripleCLick. Генерятся WM_LBUTTONUP WM_LBUTTONDBLCLK WM_LBUTTONDOWN WM_LBUTTONUP. Выделение "Строки".

		//TODO("Горизонтальная прокрутка?");
		//// Если мыша за пределами окна консоли - скорректировать координаты (MinMax)
		//if (cr.X<0 || cr.X>=(int)TextWidth())
		//	cr.X = GetMinMax(cr.X, 0, TextWidth());
		//if (cr.Y<0 || cr.Y>=(int)TextHeight())
		//	cr.Y = GetMinMax(cr.Y, 0, TextHeight());

		// Теперь проверки Double/Triple.
		if ((messg == WM_LBUTTONUP)
			&& ((((con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION)
					&& ((GetTickCount() - con.m_SelClickTick) <= GetDoubleClickTime()))
				|| ((con.m_sel.dwFlags & CONSOLE_DBLCLICK_SELECTION)
					&& ((GetTickCount() - con.m_SelDblClickTick) <= GetDoubleClickTime())))
				)
			)
			//&& ((messg == WM_LBUTTONUP)
			//	|| ((messg == WM_MOUSEMOVE)
			//		&& (memcmp(&cr,&con.m_sel.srSelection.Left,sizeof(cr)) || memcmp(&cr,&con.m_sel.srSelection.Right,sizeof(cr)))
			//		)
			//	)
			//)
		{
			// Ignoring due DoubleClickTime
			int nDbg = 0; UNREFERENCED_PARAMETER(nDbg);
		}
		else
		{
			if (con.m_sel.dwFlags & CONSOLE_DBLCLICK_SELECTION)
			{
				con.m_sel.dwFlags &= ~CONSOLE_DBLCLICK_SELECTION;
			}
			else
			{
				ExpandSelection(cr.X, cr.Y);
			}

		}

		if (messg == WM_LBUTTONUP)
		{
			con.m_sel.dwFlags &= ~CONSOLE_MOUSE_DOWN;

			if (gpSet->isCTSAutoCopy)
			{
				//if ((con.m_sel.srSelection.Left != con.m_sel.srSelection.Right) || (con.m_sel.srSelection.Top != con.m_sel.srSelection.Bottom))
				DWORD nPrevTick = (con.m_sel.dwFlags & CONSOLE_DBLCLICK_SELECTION) ? con.m_SelDblClickTick : con.m_SelClickTick;
				if ((GetTickCount() - nPrevTick) > GetDoubleClickTime())
				{
					// Если длительность удержания кнопки мышки превышает DblClickTime
					// то можно (и нужно) сразу выполнить копирование
					_ASSERTE(nPrevTick!=0);
					DoSelectionFinalize(true, 0);
				}
				else
				{
					// Иначе - таймер, чтобы не перекрыть возможность DblClick/TripleClick
					// Скорректировать отсечку таймера на отпускание
					mp_RCon->VCon()->SetAutoCopyTimer(true);
				}
			}
		}

		return true;
	}
	else if ((messg == WM_RBUTTONUP || messg == WM_MBUTTONUP) && (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION|CONSOLE_BLOCK_SELECTION)))
	{
		BYTE bAction = (messg == WM_RBUTTONUP) ? gpSet->isCTSRBtnAction : gpSet->isCTSMBtnAction; // 0-off, 1-copy, 2-paste, 3-auto

		// Только Copy. Делать Paste при наличии выделения - глупо. Так что только Copy.
		bool bDoCopy = (bAction == 1) || (bAction == 3);
		bool bCopyOk = DoSelectionFinalize(bDoCopy);

		if (bCopyOk && (bAction == 3))
		{
			// Immediately paste into console ('Auto' mode)?
			mp_RCon->Paste(pm_OneLine);
		}

		return true;
	}

	return false;
}

void CRealBuffer::MarkFindText(int nDirection, LPCWSTR asText, bool abCaseSensitive, bool abWholeWords)
{
	bool bFound = false;
	COORD crStart = {}, crEnd = {};
	LPCWSTR pszFrom = NULL, pszDataStart = NULL, pszEnd = NULL;
	LPCWSTR pszFrom1 = NULL, pszEnd1 = NULL;
	size_t nWidth = 0, nHeight = 0;

	if (m_Type == rbt_Primary)
	{
		pszDataStart = pszFrom = pszFrom1 = con.pConChar;
		nWidth = this->GetTextWidth();
		nHeight = this->GetTextHeight();
		_ASSERTE(pszFrom[nWidth*nHeight] == 0); // Должно быть ASCIIZ
		pszEnd = pszEnd1 = pszFrom + (nWidth * nHeight);
	}
	else if (dump.pszBlock1)
	{
		//WARNING("Доработать для режима с прокруткой");
		nWidth = dump.crSize.X;
		nHeight = dump.crSize.Y;
		_ASSERTE(dump.pszBlock1[nWidth*nHeight] == 0); // Должно быть ASCIIZ

		pszDataStart = dump.pszBlock1;
		pszEnd = pszDataStart + (nWidth * nHeight);

		pszFrom = pszFrom1 = dump.pszBlock1 + con.m_sbi.srWindow.Top * nWidth;

		// Search in whole buffer
		//nHeight = min((dump.crSize.Y-con.m_sbi.srWindow.Top),(con.m_sbi.srWindow.Bottom - con.m_sbi.srWindow.Top + 1));
		// But remember visible area
		pszEnd1 = pszFrom + (nWidth * min((dump.crSize.Y-con.m_sbi.srWindow.Top),(con.m_sbi.srWindow.Bottom - con.m_sbi.srWindow.Top + 1)));
	}

	WARNING("TODO: bFindNext");
	
	if (pszFrom && asText && *asText && nWidth && nHeight)
	{
		int nFindLen = lstrlen(asText);
		const wchar_t* szWordDelim = L"~!%^&*()+|{}:\"<>?`-=\\[];',./";
		LPCWSTR pszFound = NULL;
		int nStepMax = 0;
		INT_PTR nFrom = -1;

		if (nDirection > 1)
			nDirection = 1;
		else if (nDirection < -1)
			nDirection = -1;

		if (isSelectionPresent())
		{
			COORD crSelScrn = BufferToScreen(MakeCoord(con.m_sel.srSelection.Left, con.m_sel.srSelection.Top));
			nFrom = crSelScrn.X + (crSelScrn.Y * nWidth);
			_ASSERTE(nFrom>=0);

			if (nDirection >= 0)
			{
				if ((nFrom + nDirection) >= (INT_PTR)(nWidth * nHeight))
					goto done; // считаем, что не нашли
				pszFrom += (nFrom + nDirection);
			}
			else if (nDirection < 0)
			{
				pszEnd = pszFrom + nFrom;
			}
			nStepMax = ((m_Type == rbt_Primary) || (nDirection >= 0)) ? 1 : 2;
		}

		for (int i = 0; i <= nStepMax; i++)
		{
			while (pszFrom && (pszFrom < pszEnd) && *pszFrom)
			{
				if (abCaseSensitive)
					pszFrom = StrStr(pszFrom, asText);
				else
					pszFrom = StrStrI(pszFrom, asText);

				if (pszFrom)
				{
					if (abWholeWords)
					{
						#define isWordDelim(ch) (!ch || (wcschr(szWordDelim,ch)!=NULL) || (ch>=0x2100 && ch<0x2800) || (ch<=32))
						if (pszFrom > con.pConChar)
						{
							if (!isWordDelim(*(pszFrom-1)))
							{
								pszFrom++;
								continue;
							}
						}
						if (!isWordDelim(pszFrom[nFindLen]))
						{
							pszFrom++;
							continue;
						}
					}

					if (nDirection < 0)
					{
						if (pszFrom < pszEnd)
						{
							pszFound = pszFrom;
							pszFrom++;
							bFound = true;
							continue;
						}
						else
						{
							pszFrom = NULL;
						}
					}
					bFound = true;
					break; // OK, подходит
				}
			}

			if ((nDirection < 0) && bFound && pszFound)
			{
				pszFrom = pszFound;
			}
			if (pszFrom && bFound)
				break;

			if (!nStepMax)
				break;


			pszFrom = pszDataStart;
			pszEnd = pszDataStart + (nWidth * nHeight);
			if ((nDirection < 0) && (nFrom >= 0))
			{
				if (i == 0)
				{
					pszEnd = pszFrom1 + nFrom;
				}
				else
				{
					pszFrom = pszFrom1 + nFrom + 1;
				}
			}
		}


		if (pszFrom && bFound)
		{
			// Нашли
			size_t nCharIdx = (pszFrom - pszDataStart);

			WARNING("Переделать выделение на поддержку прокрутки, а не только видимой области");

			// А пока - возможная коррекция видимой области, если нашли "за пределами"
			if (m_Type != rbt_Primary)
			{
				int nRows = con.m_sbi.srWindow.Bottom - con.m_sbi.srWindow.Top; // Для удобства

				if (pszFrom < pszFrom1)
				{
					// Прокрутить буфер вверх
					con.nTopVisibleLine = con.m_sbi.srWindow.Top = max(0,((int)(nCharIdx / nWidth))-1);
					con.m_sbi.srWindow.Bottom = min((con.m_sbi.srWindow.Top + nRows), con.m_sbi.dwSize.Y-1);
				}
				else if (pszFrom >= pszEnd1)
				{
					// Прокрутить буфер вниз
					con.m_sbi.srWindow.Bottom = min((nCharIdx / nWidth)+1, (UINT)con.m_sbi.dwSize.Y-1);
					con.nTopVisibleLine = con.m_sbi.srWindow.Top = max(0, con.m_sbi.srWindow.Bottom-nRows);
				}

				if (nCharIdx >= (size_t)(con.m_sbi.srWindow.Top*nWidth))
				{
					nCharIdx -= con.m_sbi.srWindow.Top*nWidth;
				}
			}
			else
			{
				// Для rbt_Primary - прокрутки быть не должно (не рассчитано здесь на это)
				_ASSERTE(pszDataStart == pszFrom1);
			}

			WARNING("тут бы на ширину буфера ориентироваться, а не видимой области...");
			if (nCharIdx < (nWidth*nHeight))
			{
				bFound = true;
				crStart.Y = nCharIdx / nWidth;
				crStart.X = nCharIdx - (nWidth * crStart.Y);
				crEnd.Y = (nCharIdx + nFindLen - 1) / nWidth;
				crEnd.X = (nCharIdx + nFindLen - 1) - (nWidth * crEnd.Y);
			}
		}

	} // if (pszFrom && asText && *asText && nWidth && nHeight)

done:
	if (!bFound)
	{
		DoSelectionStop();
	}
	else
	{
		con.m_sel.dwFlags = CONSOLE_SELECTION_IN_PROGRESS | CONSOLE_TEXT_SELECTION;
		COORD crStartBuf = ScreenToBuffer(crStart);
		COORD crEndBuf = ScreenToBuffer(crEnd);
		con.m_sel.dwSelectionAnchor = crStartBuf;
		con.m_sel.srSelection.Left = crStartBuf.X;
		con.m_sel.srSelection.Top = crStartBuf.Y;
		con.m_sel.srSelection.Right = crEndBuf.X;
		con.m_sel.srSelection.Bottom = crEndBuf.Y;
	}

	UpdateSelection();
}

void CRealBuffer::StartSelection(BOOL abTextMode, SHORT anX/*=-1*/, SHORT anY/*=-1*/, BOOL abByMouse/*=FALSE*/, UINT anFromMsg/*=0*/, COORD *pcrTo/*=NULL*/)
{
	_ASSERTE(anY==-1 || anY>=con.nTopVisibleLine);

	// Если начинается выделение - запретить фару начинать драг, а то подеремся
	if (abByMouse)
	{
		Assert(!(gpConEmu->mouse.state & (DRAG_L_STARTED|DRAG_R_STARTED)));
		gpConEmu->mouse.state &= ~(DRAG_L_ALLOWED | DRAG_R_STARTED);
	}

	if (!(con.m_sel.dwFlags & (CONSOLE_BLOCK_SELECTION|CONSOLE_TEXT_SELECTION)) && gpSet->isCTSFreezeBeforeSelect)
	{
		if (m_Type == rbt_Primary)
		{
			if (mp_RCon->LoadAlternativeConsole(lam_FullBuffer) && (mp_RCon->mp_ABuf != this))
			{
				// Сменился буфер (переключились на альтернативную консоль)
				// Поэтому дальнейшие действия - в этом буфере

				DoSelectionStop();

				_ASSERTE(mp_RCon->mp_ABuf->m_Type==rbt_Alternative);
				mp_RCon->mp_ABuf->m_Type = rbt_Selection; // Изменить, чтобы по завершении выделения - буфер закрыть

				mp_RCon->mp_ABuf->StartSelection(abTextMode, anX, anY, abByMouse, anFromMsg);
				return;
			}
		}
	}

	mp_RCon->VCon()->SetAutoCopyTimer(false);

	WARNING("Доработать для режима с прокруткой - выделение протяжкой, как в обычной консоли");
	if (anX == -1 && anY == -1)
	{
		// Absolute coordinates now!
		anX = con.m_sbi.dwCursorPosition.X; // - con.m_sbi.srWindow.Left;
		anY = con.m_sbi.dwCursorPosition.Y; // - con.m_sbi.srWindow.Top;
	}
	
	// Если начало выделение не видимо - ставим ближайший угол
	if (anX < 0)
	{
		_ASSERTE(FALSE && "Started (x) in not visible area?");
		anX = 0;
	}
	else if (anX >= GetBufferWidth())
	{
		_ASSERTE(FALSE && "Started beyond right margin");
		anX = GetBufferWidth()-1;
	}

	if (anY < 0)
	{
		_ASSERTE(FALSE && "Started (y) in not visible area?");
		anY = anX = 0;
	}
	else if (anY >= GetBufferHeight())
	{
		_ASSERTE(FALSE && "Started beyond bottom margin");
		anX = 0;
		anY = GetBufferHeight()-1;
	}

	COORD cr = {anX,anY};

	if (cr.X<0 || cr.X>=GetBufferWidth() || cr.Y<0 || cr.Y>=GetBufferHeight())
	{
		_ASSERTE(cr.X>=0 && cr.X<GetBufferWidth());
		_ASSERTE(cr.Y>=0 && cr.Y<GetBufferHeight());
		return; // Ошибка в координатах
	}

	DWORD vkMod = con.m_sel.dwFlags & CONSOLE_KEYMOD_MASK;
	if (vkMod && !abByMouse)
	{
		DoSelectionStop(); // Чтобы Фар не думал, что все еще нажат модификатор
	}

	con.m_sel.dwFlags = CONSOLE_SELECTION_IN_PROGRESS
	                    | (abByMouse ? (CONSOLE_MOUSE_SELECTION|CONSOLE_MOUSE_DOWN) : 0)
	                    | (abTextMode ? CONSOLE_TEXT_SELECTION : CONSOLE_BLOCK_SELECTION)
						| (abByMouse ? vkMod : 0);
	con.m_sel.dwSelectionAnchor = cr;
	con.m_sel.srSelection.Left = con.m_sel.srSelection.Right = cr.X;
	con.m_sel.srSelection.Top = con.m_sel.srSelection.Bottom = cr.Y;
	_ASSERTE(cr.Y>=con.nTopVisibleLine && cr.Y<GetBufferHeight());

	UpdateSelection();

	if ((anFromMsg == WM_LBUTTONDBLCLK) || (pcrTo && (con.m_sel.dwFlags & CONSOLE_DBLCLICK_SELECTION)))
	{
		if (pcrTo)
			ExpandSelection(pcrTo->X, pcrTo->Y);
		con.m_sel.dwFlags |= CONSOLE_DBLCLICK_SELECTION;

		_ASSERTE(anFromMsg == WM_LBUTTONDBLCLK);
		//if (anFromMsg == WM_LBUTTONDBLCLK)
		con.m_SelDblClickTick = GetTickCount();

		if (gpSet->isCTSAutoCopy)
		{
			mp_RCon->VCon()->SetAutoCopyTimer(true);
		}
	}
	else if (abByMouse)
	{
		con.m_SelClickTick = GetTickCount();

		if (gpSet->isCTSAutoCopy)
		{
			mp_RCon->VCon()->SetAutoCopyTimer(true);
		}
	}
}

void CRealBuffer::ExpandSelection(SHORT anX, SHORT anY)
{
	_ASSERTE(con.m_sel.dwFlags!=0);
	// Добавил "-3" чтобы на прокрутку не ругалась
	_ASSERTE(anY==-1 || anY>=(con.nTopVisibleLine-3));

	// 131017 Scroll content if selection cursor goes out of visible screen
	if (anY < con.nTopVisibleLine)
	{
		OnScroll(SB_LINEUP);
	}
	else if (anY >= (con.nTopVisibleLine + con.nTextHeight))
	{
		OnScroll(SB_LINEDOWN);
	}

	COORD cr = {anX,anY};

	if (cr.X<0 || cr.X>=GetBufferWidth() || cr.Y<0 || cr.Y>=GetBufferHeight())
	{
		_ASSERTE(cr.X>=0 && cr.X<GetBufferWidth());
		_ASSERTE(cr.Y>=0 && cr.Y<GetBufferHeight());
		return; // Ошибка в координатах
	}

	BOOL lbStreamSelection = (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION)) == CONSOLE_TEXT_SELECTION;

	if (!lbStreamSelection)
	{
		if (cr.X < con.m_sel.dwSelectionAnchor.X)
		{
			con.m_sel.srSelection.Left = cr.X;
			con.m_sel.srSelection.Right = con.m_sel.dwSelectionAnchor.X;
		}
		else
		{
			con.m_sel.srSelection.Left = con.m_sel.dwSelectionAnchor.X;
			con.m_sel.srSelection.Right = cr.X;
		}
	}
	else
	{
		if ((cr.Y > con.m_sel.dwSelectionAnchor.Y)
		        || ((cr.Y == con.m_sel.dwSelectionAnchor.Y) && (cr.X > con.m_sel.dwSelectionAnchor.X)))
		{
			con.m_sel.srSelection.Left = con.m_sel.dwSelectionAnchor.X;
			con.m_sel.srSelection.Right = cr.X;
		}
		else
		{
			con.m_sel.srSelection.Left = cr.X;
			con.m_sel.srSelection.Right = con.m_sel.dwSelectionAnchor.X;
		}
	}

	if (cr.Y < con.m_sel.dwSelectionAnchor.Y)
	{
		con.m_sel.srSelection.Top = cr.Y;
		con.m_sel.srSelection.Bottom = con.m_sel.dwSelectionAnchor.Y;
	}
	else
	{
		con.m_sel.srSelection.Top = con.m_sel.dwSelectionAnchor.Y;
		con.m_sel.srSelection.Bottom = cr.Y;
	}

	UpdateSelection();
}

void CRealBuffer::DoSelectionStop()
{
	BYTE vkMod = HIBYTE(HIWORD(con.m_sel.dwFlags));

	if (vkMod)
	{
		// Но чтобы ФАР не запустил макрос (если есть макро на RAlt например...)
		if (vkMod == VK_CONTROL || vkMod == VK_LCONTROL || vkMod == VK_RCONTROL)
			mp_RCon->PostKeyPress(VK_SHIFT, LEFT_CTRL_PRESSED, 0);
		else if (vkMod == VK_MENU || vkMod == VK_LMENU || vkMod == VK_RMENU)
			mp_RCon->PostKeyPress(VK_SHIFT, LEFT_ALT_PRESSED, 0);
		else
			mp_RCon->PostKeyPress(VK_CONTROL, SHIFT_PRESSED, 0);

		// "Отпустить" в консоли модификатор
		mp_RCon->PostKeyUp(vkMod, 0, 0);
	}

	con.m_sel.dwFlags = 0;
}

bool CRealBuffer::DoSelectionCopy(bool bCopyAll /*= false*/, BYTE nFormat /*= 0xFF*/ /* use gpSet->isCTSHtmlFormat */)
{
	bool bRc = false;

	RealBufferType bufType = m_Type;

	if (!con.m_sel.dwFlags)
	{
		MBoxAssert(con.m_sel.dwFlags != 0);
	}
	else
	{
		bool  lbStreamMode = (con.m_sel.dwFlags & CONSOLE_TEXT_SELECTION) == CONSOLE_TEXT_SELECTION;
		bool  lbProcessed = false;

		// Сначала проверим, помещается ли "выделенная область" в "ВИДИМУЮ область"
		if (m_Type == rbt_Primary)
		{
			COORD crStart = BufferToScreen(MakeCoord(con.m_sel.srSelection.Left, con.m_sel.srSelection.Top));
			COORD crEnd = BufferToScreen(MakeCoord(con.m_sel.srSelection.Right, con.m_sel.srSelection.Bottom));

			int nTextWidth = this->GetTextWidth();
			int nTextHeight = this->GetTextHeight();

			if ((crStart.X < 0) || (crStart.X >= nTextWidth)
				|| (crStart.Y < 0) || (crStart.Y >= nTextHeight)
				|| (crEnd.X < 0) || (crEnd.X >= nTextWidth)
				|| (crEnd.Y < 0) || (crEnd.Y >= nTextHeight))
			{
				if (mp_RCon->LoadAlternativeConsole(lam_FullBuffer) && (mp_RCon->mp_ABuf != this))
				{
					bRc = mp_RCon->mp_ABuf->DoSelectionCopyInt(bCopyAll, lbStreamMode, con.m_sel.srSelection.Left, con.m_sel.srSelection.Top, con.m_sel.srSelection.Right, con.m_sel.srSelection.Bottom, nFormat);
					lbProcessed = true;
					bufType = rbt_Selection;
				}
				else
				{
					_ASSERTE(FALSE && "Failed to load full console data?");
				}
			}
		}
		
		if (!lbProcessed)
		{
			bRc = DoSelectionCopyInt(bCopyAll, lbStreamMode, con.m_sel.srSelection.Left, con.m_sel.srSelection.Top, con.m_sel.srSelection.Right, con.m_sel.srSelection.Bottom, nFormat);
		}
	}

	// Fin, Сбрасываем
	if (bRc)
	{
		DoSelectionStop(); // con.m_sel.dwFlags = 0;
		
		if (bufType == rbt_Selection)
		{
			mp_RCon->SetActiveBuffer(rbt_Primary);
			// Сразу на выход!
		}
		
		if (m_Type == rbt_Primary)
		{
			UpdateSelection(); // обновить на экране
		}
	}

	return bRc;
}

int CRealBuffer::GetSelectionCharCount(bool bStreamMode, int srSelection_X1, int srSelection_Y1, int srSelection_X2, int srSelection_Y2, int* pnSelWidth, int* pnSelHeight, int nNewLineLen)
{
	int   nCharCount = 0;
	int   nSelWidth = srSelection_X2 - srSelection_X1 + 1;
	int   nSelHeight = srSelection_Y2 - srSelection_Y1 + 1;

	_ASSERTE(nNewLineLen==0 || nNewLineLen==1 || nNewLineLen==2);

	if (!bStreamMode)
	{
		nCharCount = ((nSelWidth+nNewLineLen/* "\r\n" */) * nSelHeight) - nNewLineLen; // после последней строки "\r\n" не ставится
	}
	else
	{
		if (nSelHeight == 1)
		{
			nCharCount = nSelWidth;
		}
		else if (nSelHeight == 2)
		{
			// На первой строке - до конца строки, вторая строка - до окончания блока, + "\r\n"
			nCharCount = (con.nTextWidth - srSelection_X1) + (srSelection_X2 + 1) + nNewLineLen;
		}
		else
		{
			Assert(nSelHeight>2);
			// На первой строке - до конца строки, последняя строка - до окончания блока, + "\r\n"
			nCharCount = (con.nTextWidth - srSelection_X1) + (srSelection_X2 + 1) + nNewLineLen
			             + ((nSelHeight - 2) * (con.nTextWidth + nNewLineLen)); // + серединка * (длину консоли + "\r\n")
		}
	}

	if (pnSelWidth)
		*pnSelWidth = nSelWidth;
	if (pnSelHeight)
		*pnSelHeight = nSelHeight;

	return nCharCount;
}

bool CRealBuffer::DoSelectionCopyInt(bool bCopyAll, bool bStreamMode, int srSelection_X1, int srSelection_Y1, int srSelection_X2, int srSelection_Y2, BYTE nFormat /*= 0xFF*/ /* use gpSet->isCTSHtmlFormat */)
{
	// Warning!!! Здесь уже нельзя ориентироваться на con.m_sel !!!

	LPCWSTR pszDataStart = NULL;
	WORD* pAttrStart = NULL;
	CharAttr* pAttrStartEx = NULL;
	int nTextWidth = 0, nTextHeight = 0;

	if (nFormat == 0xFF)
		nFormat = gpSet->isCTSHtmlFormat;

	if (m_Type == rbt_Primary)
	{
		pszDataStart = con.pConChar;
		pAttrStart = con.pConAttr;
		nTextWidth = this->GetTextWidth();
		nTextHeight = this->GetTextHeight();
		_ASSERTE(pszDataStart[nTextWidth*nTextHeight] == 0); // Должно быть ASCIIZ

		// Выделение "с прокруткой" обрабатываем только в альтернативных буферах,
		// а здесь нам нужны только "экранные" координаты

		COORD cr = BufferToScreen(MakeCoord(srSelection_X1, srSelection_Y1));
		srSelection_X1 = cr.X; srSelection_Y1 = cr.Y;
		cr = BufferToScreen(MakeCoord(srSelection_X2, srSelection_Y2));
		srSelection_X2 = cr.X; srSelection_Y2 = cr.Y;
	}
	else if (dump.pszBlock1)
	{
		WARNING("Доработать для режима с прокруткой");
		nTextWidth = dump.crSize.X;
		nTextHeight = dump.crSize.Y;
		//nTextWidth = this->GetTextWidth();
		//nTextHeight = this->GetTextHeight();
		_ASSERTE(dump.pszBlock1[nTextWidth*nTextHeight] == 0); // Должно быть ASCIIZ

		pszDataStart = dump.pszBlock1; // + con.m_sbi.srWindow.Top * nTextWidth; -- работаем с полным буфером
		pAttrStartEx = dump.pcaBlock1;
		//nTextHeight = min((dump.crSize.Y-con.m_sbi.srWindow.Top),(con.m_sbi.srWindow.Bottom - con.m_sbi.srWindow.Top + 1));
	}

	if (!pszDataStart || !nTextWidth || !nTextHeight)
	{
		Assert(pszDataStart != NULL);
		return false;
	}

	const Settings::AppSettings* pApp = gpSet->GetAppSettings(mp_RCon->GetActiveAppSettingsId());
	if (pApp == NULL)
	{
		Assert(pApp!=NULL);
		return false;
	}
	BYTE nEOL = pApp->CTSEOL();
	BYTE nTrimTailing = pApp->CTSTrimTrailing();
	bool bDetectLines = pApp->CTSDetectLineEnd();
	bool bBash = pApp->CTSBashMargin();

	wchar_t sPreLineBreak[] =
		{
			ucBox25,ucBox50,ucBox75,ucBox100,ucUpScroll,ucDnScroll,ucLeftScroll,ucRightScroll,ucArrowUp,ucArrowDown,
			//ucNoBreakSpace, -- this is space, why it was blocked?
			ucBoxDblVert,ucBoxSinglVert,ucBoxDblDownRight,ucBoxDblDownLeft,ucBoxDblUpRight,ucBoxDblUpLeft,ucBoxSinglDownRight,
			ucBoxSinglDownLeft,ucBoxSinglUpRight,ucBoxSinglUpLeft,ucBoxSinglDownDblHorz,ucBoxSinglUpDblHorz,ucBoxDblDownDblHorz,
			ucBoxDblUpDblHorz,ucBoxSinglDownHorz,ucBoxSinglUpHorz,ucBoxDblDownSinglHorz,ucBoxDblUpSinglHorz,ucBoxDblVertRight,
			ucBoxDblVertLeft,ucBoxDblVertSinglRight,ucBoxDblVertSinglLeft,ucBoxSinglVertRight,ucBoxSinglVertLeft,
			ucBoxDblHorz,ucBoxSinglHorz,ucBoxDblVertHorz,
			// End
			0 /*ASCIIZ!!!*/
		};

	// Pre validations
	if (srSelection_X1 > (srSelection_X2+(srSelection_Y2-srSelection_Y1)*nTextWidth))
	{
		Assert(srSelection_X1 <= (srSelection_X2+(srSelection_Y2-srSelection_Y1)*nTextWidth));
		return false;
	}

	DWORD dwErr = 0;
	BOOL  lbRc = FALSE;
	bool  Result = false;
	int   nSelWidth = 0, nSelHeight = 0;
	int   nNewLineLen = 2; // max "\r\n"

	int   nCharCount = GetSelectionCharCount(bStreamMode, srSelection_X1, srSelection_Y1, srSelection_X2, srSelection_Y2, &nSelWidth, &nSelHeight, nNewLineLen);
	_ASSERTE((nSelWidth>-(srSelection_X1-srSelection_X2)) && (nSelHeight>0) && (nCharCount>0));

	HGLOBAL hUnicode = NULL;
	hUnicode = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, (nCharCount+1)*sizeof(wchar_t));

	if (hUnicode == NULL)
	{
		Assert(hUnicode != NULL);
		return false;
	} 

	wchar_t *pch = (wchar_t*)GlobalLock(hUnicode);

	if (!pch)
	{
		Assert(pch != NULL);
		GlobalFree(hUnicode);
	}

	// Заполнить данными
	if ((srSelection_X1 + nSelWidth) > nTextWidth)
	{
		Assert((srSelection_X1 + nSelWidth) <= nTextWidth);
		nSelWidth = nTextWidth - srSelection_X1;
	}

	if ((srSelection_Y1 + nSelHeight) > nTextHeight)
	{
		Assert((srSelection_Y1 + nSelHeight) <= nTextHeight);
		nSelHeight = nTextHeight - srSelection_Y1;
	}


	nSelHeight--;

	COLORREF *pPal = mp_RCon->VCon()->GetColors();

	bool bUseHtml = (nFormat != 0);
	
	CHtmlCopy html;
	
	if (bUseHtml)
	{
		COLORREF crFore = 0xC0C0C0, crBack = 0;
		if ((m_Type == rbt_Primary) && pAttrStart)
		{
			WORD *pAttr = pAttrStart + con.nTextWidth*srSelection_Y1 + srSelection_X1;
			crFore = pPal[(*pAttr)&0xF]; crBack = pPal[((*pAttr)&0xF0)>>4];
		}
		else if (pAttrStartEx)
		{
			CharAttr* pAttr = pAttrStartEx + con.nTextWidth*srSelection_Y1 + srSelection_X1;
			crFore = pAttr->crForeColor; crBack = pAttr->crBackColor;
		}

		//wchar_t szClass[64]; _wsprintf(szClass, SKIPLEN(countof(szClass)) L"ConEmu%s%s", gpConEmu->ms_ConEmuBuild, WIN3264TEST(L"x32",L"x64"));
		html.Init((nFormat == 2), gpConEmu->ms_ConEmuBuild, gpSetCls->FontFaceName(), gpSetCls->FontHeightPx(), crFore, crBack);
	}


	TODO("Переделать на GetConsoleLine! Иначе могут не будут работать расширенные атрибуты");
	if (!bStreamMode)
	{
		// Блоковое выделение
		for (int Y = 0; Y <= nSelHeight; Y++)
		{
			LPCWSTR pszCon = NULL;

			if (m_Type == rbt_Primary)
			{
				pszCon = pszDataStart + con.nTextWidth*(Y+srSelection_Y1) + srSelection_X1;
			}
			else if (pszDataStart && (Y < nTextHeight))
			{
				WARNING("Проверить для режима с прокруткой!");
				pszCon = pszDataStart + dump.crSize.X*(Y+srSelection_Y1) + srSelection_X1;
			}

			//LPCWSTR pszDstStart = pch;
			LPCWSTR pszSrcStart = pszCon;

			int nMaxX = nSelWidth - 1;

			if (pszCon)
			{
				wchar_t* pchStart = pch;

				for (int X = 0; X <= nMaxX; X++)
				{
					*(pch++) = *(pszCon++);
				}

				if (nTrimTailing == 1)
				{
					while ((pch > pchStart) && (*(pch-1) == L' '))
					{
						*(--pch) = 0;
					}
				}

				if (bUseHtml)
				{
					INT_PTR nLineStart = pszSrcStart - pszDataStart;
					if ((m_Type == rbt_Primary) && pAttrStart)
						html.LineAdd(pszSrcStart, pAttrStart+nLineStart, pPal, nSelWidth);
					else if (pAttrStartEx)
						html.LineAdd(pszSrcStart, pAttrStartEx+nLineStart, nSelWidth);
				}

			}
			else if (nTrimTailing != 1)
			{
				wmemset(pch, L' ', nSelWidth);
				pch += nSelWidth;
			}

			// Добавить перевод строки
			if (Y < nSelHeight)
			{
				switch (nEOL)
				{
				case 1:
					*(pch++) = L'\n'; break;
				case 2:
					*(pch++) = L'\r'; break;
				default:
					*(pch++) = L'\r'; *(pch++) = L'\n';
				}
			}
		}
	}
	else
	{
		// Потоковое (текстовое) выделение
		int nX1, nX2;

		for (int Y = 0; Y <= nSelHeight; Y++)
		{
			nX1 = (Y == 0) ? srSelection_X1 : 0;
			nX2 = (Y == nSelHeight) ? srSelection_X2 : (con.nTextWidth-1);
			LPCWSTR pszCon = NULL;
			LPCWSTR pszNextLine = NULL;

			if (m_Type == rbt_Primary)
			{
				pszCon = pszDataStart + con.nTextWidth*(Y+srSelection_Y1) + nX1;
				pszNextLine = ((Y + 1) <= nSelHeight) ? (pszDataStart + con.nTextWidth*(Y+1+srSelection_Y1)) : NULL;
			}
			else if (pszDataStart && (Y < nTextHeight))
			{
				WARNING("Проверить для режима с прокруткой!");
				pszCon = pszDataStart + dump.crSize.X*(Y+srSelection_Y1) + nX1;
				pszNextLine = ((Y + 1) <= nSelHeight) ? (pszDataStart + dump.crSize.X*(Y+1+srSelection_Y1)) : NULL;
			}

			LPCWSTR pszDstStart = pch;
			LPCWSTR pszSrcStart = pszCon;

			wchar_t* pchStart = pch;

			if (pszCon)
			{
				for (int X = nX1; X <= nX2; X++)
				{
					*(pch++) = *(pszCon++);
				}

				if (bUseHtml)
				{
					INT_PTR nLineStart = pszSrcStart - pszDataStart;
					INT_PTR nLineLen = pch - pszDstStart;
					if ((m_Type == rbt_Primary) && pAttrStart)
						html.LineAdd(pszSrcStart, pAttrStart+nLineStart, pPal, nLineLen);
					else if (pAttrStartEx)
						html.LineAdd(pszSrcStart, pAttrStartEx+nLineStart, nLineLen);
				}

			}
			else
			{
				wmemset(pch, L' ', nSelWidth);
				pch += nSelWidth;
			}

			// Добавить перевод строки
			if (Y < nSelHeight)
			{
				bool bContinue = false;

				if (bDetectLines && pszNextLine
					// Allow maximum one space on the next line
					&& ((pszNextLine[0] != L' ') || (pszNextLine[0] == L' ' && pszNextLine[1] != L' '))
					// If right or left edge of screen is "Frame" - force to line break!
					&& !wcschr(sPreLineBreak, *(pch - 1))
					&& !wcschr(sPreLineBreak, *pszNextLine))
				{
					// Пытаемся опредлить, новая это строка или просто перенос в Prompt?
					if ((*(pch - 1) != L' ')
						|| (((pch - 1) > pchStart) && (*(pch - 2) != L' '))
						// sh.exe - one cell space pad on right edge
						|| (bBash && ((pch - 2) > pchStart) && (*(pch - 2) == L' ') && (*(pch - 3) != L' ')))
					{
						bContinue = true;
					}

					if (bBash && (*(pch - 1) == L' '))
					{
						*(--pch) = 0;
					}
				}

				if (!bContinue)
				{
					if (nTrimTailing)
					{
						while ((pch > pchStart) && (*(pch-1) == L' '))
						{
							*(--pch) = 0;
						}
					}

					switch (nEOL)
					{
					case 1:
						*(pch++) = L'\n'; break;
					case 2:
						*(pch++) = L'\r'; break;
					default:
						*(pch++) = L'\r'; *(pch++) = L'\n';
					}
				}
			}
			else
			{
				if (nTrimTailing)
				{
					while ((pch > pchStart) && (*(pch-1) == L' '))
					{
						*(--pch) = 0;
					}
				}
			}
		}
	}

	// Ready
	GlobalUnlock(hUnicode);
	// HTML?
	HGLOBAL hHtml = NULL;
	if (bUseHtml)
	{
		hHtml = html.CreateResult();
		if (!hHtml)
		{
			dwErr = GetLastError();
			DisplayLastError(L"Creating HTML format failed!", dwErr, MB_ICONSTOP);
			GlobalFree(hUnicode);
			return false;
		}
	}

	// User asked to copy HTML instead of HTML formatted (put HTML in CF_UNICODE)
	if ((nFormat == 2) && hHtml)
	{
		WARNING("hUnicode Overhead...");
		GlobalFree(hUnicode);
		hUnicode = hHtml;
		hHtml = NULL;
		bUseHtml = false;
	}

	// Открыть буфер обмена
	while (!(lbRc = OpenClipboard(ghWnd)))
	{
		dwErr = GetLastError();

		if (IDRETRY != DisplayLastError(L"OpenClipboard failed!", dwErr, MB_RETRYCANCEL|MB_ICONSTOP))
		{
			GlobalFree(hUnicode);
			if (hHtml) GlobalFree(hHtml);
			return false;
		}
	}

	UINT i_CF_HTML = bUseHtml ? RegisterClipboardFormat(L"HTML Format") : 0;

	lbRc = EmptyClipboard();
	// Установить данные
	Result = SetClipboardData(CF_UNICODETEXT, hUnicode)
		&& (!i_CF_HTML || SetClipboardData(i_CF_HTML, hHtml));

	while (!Result)
	{
		dwErr = GetLastError();

		if (IDRETRY != DisplayLastError(L"SetClipboardData(CF_UNICODETEXT, ...) failed!", dwErr, MB_RETRYCANCEL|MB_ICONSTOP))
		{
			GlobalFree(hUnicode); hUnicode = NULL;
			if (hHtml) GlobalFree(hHtml); hHtml = NULL;
			break;
		}

		Result = SetClipboardData(CF_UNICODETEXT, hUnicode)
			&& (!i_CF_HTML || SetClipboardData(i_CF_HTML, hHtml));
	}

	lbRc = CloseClipboard();

	return Result;
}

// обновить на экране
void CRealBuffer::UpdateSelection()
{
	// Show current selection state in the Status bar
	wchar_t szSelInfo[128] = L"";
	if (con.m_sel.dwFlags)
	{
		bool bStreamMode = isStreamSelection();
		int  nCharCount = GetSelectionCharCount(bStreamMode, con.m_sel.srSelection.Left, con.m_sel.srSelection.Top,
			con.m_sel.srSelection.Right, con.m_sel.srSelection.Bottom, NULL, NULL, 0);

		_wsprintf(szSelInfo, SKIPLEN(countof(szSelInfo)) L"%s selection {%i,%i}-{%i,%i} total %i chars",
			bStreamMode ? L"Stream" : L"Block",
			con.m_sel.srSelection.Left, con.m_sel.srSelection.Top,
			con.m_sel.srSelection.Right, con.m_sel.srSelection.Bottom,
			nCharCount);
	}
	mp_RCon->SetConStatus(szSelInfo);

	TODO("Это корректно? Нужно обновить VCon");
	con.bConsoleDataChanged = TRUE; // А эта - при вызовах из CVirtualConsole
	mp_RCon->mp_VCon->Update(true);
	mp_RCon->mp_VCon->Redraw();
}

bool CRealBuffer::isConSelectMode()
{
	if (!this) return false;

	if (con.m_sel.dwFlags != 0)
		return true;

	if ((this == mp_RCon->mp_RBuf) && mp_RCon->isFar())
	{
		// В Фаре может быть активен граббер (AltIns)
		if (con.m_ci.dwSize >= 40)  // Попробуем его определять по высоте курсора.
			return true;
	}

	return false;
}

bool CRealBuffer::isSelfSelectMode()
{
	if (!this) return false;
	
	return (con.m_sel.dwFlags != 0);
}

bool CRealBuffer::isStreamSelection()
{
	if (!this) return false;
	
	return ((con.m_sel.dwFlags & CONSOLE_TEXT_SELECTION) == CONSOLE_TEXT_SELECTION);
}

// true - if clipboard was set successfully
bool CRealBuffer::DoSelectionFinalize(bool abCopy, WPARAM wParam)
{
	bool bCopied = false;

	if (abCopy)
	{
		bCopied = DoSelectionCopy();
	}

	mp_RCon->mn_SelectModeSkipVk = wParam;
	DoSelectionStop(); // con.m_sel.dwFlags = 0;

	if (m_Type == rbt_Selection)
	{
		mp_RCon->SetActiveBuffer(rbt_Primary);
		// Сразу на выход!
		return bCopied;
	}
	else
	{
		//mb_ConsoleSelectMode = false;
		UpdateSelection(); // обновить на экране
	}

	return bCopied;
}

// pszChars may be NULL
const ConEmuHotKey* CRealBuffer::ProcessSelectionHotKey(DWORD VkState, bool bKeyDown, const wchar_t *pszChars)
{
	if (!this || !con.m_sel.dwFlags)
		return NULL;

	// If these was not processed by user HotKeys, lets do it...
	if (VkState == ConEmuHotKey::MakeHotKey('C', VK_CONTROL)
		|| VkState == ConEmuHotKey::MakeHotKey(VK_INSERT, VK_CONTROL))
	{
		if (bKeyDown)
		{
			DoSelectionFinalize(true, ConEmuHotKey::GetHotkey(VkState));
		}
		return ConEmuSkipHotKey;
	}

	return NULL;
}

// pszChars may be NULL
bool CRealBuffer::OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars)
{
	// Обработка Left/Right/Up/Down при выделении

	if (con.m_sel.dwFlags && messg == WM_KEYDOWN
	        && ((wParam == VK_ESCAPE) || (wParam == VK_RETURN)
				/*|| ((wParam == 'C' || wParam == VK_INSERT) && isPressed(VK_CONTROL)) -- moved to ProcessSelectionHotKey */
	            || (wParam == VK_LEFT) || (wParam == VK_RIGHT) || (wParam == VK_UP) || (wParam == VK_DOWN)
				|| (wParam == VK_HOME) || (wParam == VK_END))
	  )
	{
		if ((wParam == VK_ESCAPE) || (wParam == VK_RETURN) /*|| (wParam == 'C' || wParam == VK_INSERT)*/)
		{
			DoSelectionFinalize(wParam != VK_ESCAPE, wParam);
			return true;
		}
		else
		{
			COORD cr; ConsoleCursorPos(&cr);

			// -- координаты нужны абсолютные
			//// Поправить
			//cr.Y -= con.nTopVisibleLine;

			if (wParam == VK_LEFT)  { if (cr.X>0) cr.X--; }
			else if (wParam == VK_RIGHT) { if (cr.X<(GetBufferWidth()-1)) cr.X++; }
			else if (wParam == VK_UP)    { if (cr.Y>0) cr.Y--; }
			else if (wParam == VK_DOWN)  { if (cr.Y<(GetBufferHeight()-1)) cr.Y++; }
			else if (wParam == VK_HOME)  { cr.X = 0; }
			else if (wParam == VK_END)   { cr.X = (GetBufferWidth()-1); }

			// Теперь - двигаем
			BOOL bShift = isPressed(VK_SHIFT);

			if (!bShift)
			{
				BOOL lbStreamSelection = (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION)) == CONSOLE_TEXT_SELECTION;
				// Выделение уже было начато. Просто выделяем с нового начала.
				StartSelection(lbStreamSelection, cr.X,cr.Y);
			}
			else
			{
				ExpandSelection(cr.X,cr.Y);
			}
		}

		return true;
	}

	if (messg == WM_KEYUP)
	{
		if (wParam == mp_RCon->mn_SelectModeSkipVk)
		{
			mp_RCon->mn_SelectModeSkipVk = 0; // игнорируем отпускание, поскольку нажатие было на копирование/отмену
			return true;
		}
		else if (gpSet->isFarGotoEditor && isKey(wParam, gpSet->GetHotkeyById(vkFarGotoEditorVk)))
		{
			if (GetLastTextRangeType() != etr_None)
			{
				StoreLastTextRange(etr_None);
				UpdateSelection();
			}
		}
	}

	if (messg == WM_KEYDOWN)
	{
		if (gpSet->isFarGotoEditor && isKey(wParam, gpSet->GetHotkeyById(vkFarGotoEditorVk)))
		{
			if (ProcessFarHyperlink(true))
				UpdateSelection();
		}

		if (mp_RCon->mn_SelectModeSkipVk)
			mp_RCon->mn_SelectModeSkipVk = 0; // при _нажатии_ любой другой клавиши - сбросить флажок (во избежание)
	}

	switch (m_Type)
	{
	case rbt_DumpScreen:
	case rbt_Alternative:
	case rbt_Selection:
	case rbt_Find:
		if (messg == WM_KEYUP)
		{
			if (wParam == VK_ESCAPE)
			{
				if ((m_Type == rbt_Find) && gpConEmu->mp_Find->mh_FindDlg && IsWindow(gpConEmu->mp_Find->mh_FindDlg))
				{
					break; // Пока висит диалог поиска - буфер не закрывать!
				}
				mp_RCon->SetActiveBuffer(rbt_Primary);
				return true;
			}
		}
		else if (messg == WM_KEYDOWN)
		{
			if ((wParam == VK_NEXT) || (wParam == VK_PRIOR))
			{
				gpConEmu->key_BufferScroll(false, wParam, mp_RCon);
				return true;
			}
			else if (((m_Type != rbt_Selection) && (m_Type != rbt_Find)) // в режиме выделения стрелки трогать не будем
					&& ((wParam == VK_UP) || (wParam == VK_DOWN)))
			{
				gpConEmu->key_BufferScroll(false, wParam, mp_RCon);
				return true;
			}
		}
		break;
	default:
		;
	}

	// Разрешить дальнейшую обработки (хоткеи и прочее)
	return false;
}

COORD CRealBuffer::GetDefaultNtvdmHeight()
{
	// 100627 - con.m_sbi.dwSize.Y более использовать некорректно ввиду "far/w"
	COORD cr16bit = {80,(SHORT)(con.m_sbi.srWindow.Bottom-con.m_sbi.srWindow.Top+1)};

	if (gpSet->ntvdmHeight && cr16bit.Y >= (int)gpSet->ntvdmHeight) cr16bit.Y = gpSet->ntvdmHeight;
	else if (cr16bit.Y>=50) cr16bit.Y = 50;
	else if (cr16bit.Y>=43) cr16bit.Y = 43;
	else if (cr16bit.Y>=28) cr16bit.Y = 28;
	else if (cr16bit.Y>=25) cr16bit.Y = 25;
	
	return cr16bit;
}

const CONSOLE_SCREEN_BUFFER_INFO* CRealBuffer::GetSBI()
{
	return &con.m_sbi;
}

//BOOL CRealBuffer::IsConsoleDataChanged()
//{
//	if (!this) return FALSE;
//
//	return con.bConsoleDataChanged;
//}

BOOL CRealBuffer::GetConsoleLine(int nLine, wchar_t** pChar, /*CharAttr** pAttr,*/ int* pLen, MSectionLock* pcsData)
{
	// Может быть уже заблокировано
	MSectionLock csData;
	if (pcsData)
	{
		if (!pcsData->isLocked())
			pcsData->Lock(&csCON);
	}
	else
	{
		csData.Lock(&csCON);
	}
	
	_ASSERTE(nLine>=0 && nLine<GetWindowHeight());
	if (nLine < 0 || nLine >= con.nTextHeight)
	{
		return FALSE;
	}

	if ((m_Type == rbt_DumpScreen) || (m_Type == rbt_Alternative) || (m_Type == rbt_Selection) || (m_Type == rbt_Find))
	{
		if (!dump.pszBlock1)
			return FALSE;

		if (pChar)
			*pChar = dump.pszBlock1 + ((con.m_sbi.srWindow.Top + nLine) * dump.crSize.X) + con.m_sbi.srWindow.Left;
		if (pLen)
			*pLen = dump.crSize.X;
	}
	else
	{
		// Вернуть данные
		if (!con.pConChar || !con.pConAttr)
			return FALSE;
		
		if (pChar)
			*pChar = con.pConChar + (nLine * con.nTextWidth);
		//if (pAttr)
		//	*pAttr = con.pConAttr + (nLine * con.nTextWidth);
		if (pLen)
			*pLen = con.nTextWidth;
	}

	return TRUE;
}

void CRealBuffer::PrepareColorTable(bool bExtendFonts, CharAttr (&lcaTableExt)[0x100], CharAttr (&lcaTableOrg)[0x100], const Settings::AppSettings* pApp /*= NULL*/)
{
	CharAttr lca; // crForeColor, crBackColor, nFontIndex, nForeIdx, nBackIdx, crOrigForeColor, crOrigBackColor
	//COLORREF lcrForegroundColors[0x100], lcrBackgroundColors[0x100];
	//BYTE lnForegroundColors[0x100], lnBackgroundColors[0x100], lnFontByIndex[0x100];
	int  nColorIndex = 0;
	if (!pApp)
		pApp = gpSet->GetAppSettings(mp_RCon->GetActiveAppSettingsId());
	BYTE nFontNormalColor = pApp->FontNormalColor();
	BYTE nFontBoldColor = pApp->FontBoldColor();
	BYTE nFontItalicColor = pApp->FontItalicColor();

	for (int nBack = 0; nBack <= 0xF; nBack++)
	{
		for (int nFore = 0; nFore <= 0xF; nFore++, nColorIndex++)
		{
			memset(&lca, 0, sizeof(lca));
			lca.nForeIdx = nFore;
			lca.nBackIdx = nBack;
			lca.crForeColor = lca.crOrigForeColor = mp_RCon->mp_VCon->mp_Colors[lca.nForeIdx];
			lca.crBackColor = lca.crOrigBackColor = mp_RCon->mp_VCon->mp_Colors[lca.nBackIdx];
			lcaTableOrg[nColorIndex] = lca;

			if (bExtendFonts)
			{
				if (nBack == nFontBoldColor)  // nFontBoldColor may be -1, тогда мы сюда не попадаем
				{
					if (nFontNormalColor != 0xFF)
						lca.nBackIdx = nFontNormalColor;

					lca.nFontIndex = 1; //  Bold
					lca.crBackColor = lca.crOrigBackColor = mp_RCon->mp_VCon->mp_Colors[lca.nBackIdx];
				}
				else if (nBack == nFontItalicColor)  // nFontItalicColor may be -1, тогда мы сюда не попадаем
				{
					if (nFontNormalColor != 0xFF)
						lca.nBackIdx = nFontNormalColor;

					lca.nFontIndex = 2; // Italic
					lca.crBackColor = lca.crOrigBackColor = mp_RCon->mp_VCon->mp_Colors[lca.nBackIdx];
				}
			}

			lcaTableExt[nColorIndex] = lca;
		}
	}
}

// nWidth и nHeight это размеры, которые хочет получить VCon (оно могло еще не среагировать на изменения?
void CRealBuffer::GetConsoleData(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, ConEmuTextRange& etr)
{
	if (!this) return;

	//DWORD cbDstBufSize = nWidth * nHeight * 2;
	DWORD cwDstBufSize = nWidth * nHeight;
	_ASSERTE(nWidth != 0 && nHeight != 0);
	bool bDataValid = false;

	if (nWidth == 0 || nHeight == 0)
		return;

	#ifdef _DEBUG
	if (mp_RCon->mb_DebugLocked)
	{
		return;
	}
	#endif
	
	con.bConsoleDataChanged = FALSE;

	// формирование умолчательных цветов, по атрибутам консоли
	//TODO("В принципе, это можно делать не всегда, а только при изменениях");
	bool lbIsFar = (mp_RCon->GetFarPID() != 0);
	// Don't highlight while selection is present
	bool lbAllowHilightFileLine = (con.m_sel.dwFlags == 0) && mp_RCon->IsFarHyperlinkAllowed(false);
	if (!lbAllowHilightFileLine && (con.etr.etrLast != etr_None))
		StoreLastTextRange(etr_None);
	WARNING("lbIsFar - хорошо бы заменить на привязку к конкретным приложениям?");
	const Settings::AppSettings* pApp = gpSet->GetAppSettings(mp_RCon->GetActiveAppSettingsId());
	_ASSERTE(pApp!=NULL);
	// 120331 - зачем ограничивать настройку доп.цветов?
	bool bExtendColors = /*lbIsFar &&*/ pApp->ExtendColors();
	BYTE nExtendColorIdx = pApp->ExtendColorIdx();
	bool bExtendFonts = pApp->ExtendFonts();
	bool lbFade = mp_RCon->mp_VCon->isFade;
	//BOOL bUseColorKey = gpSet->isColorKey  // Должен быть включен в настройке
	//	&& mp_RCon->isFar(TRUE/*abPluginRequired*/) // в фаре загружен плагин (чтобы с цветами не проколоться)
	//	&& (mp_tabs && mn_tabsCount>0 && mp_tabs->Current) // Текущее окно - панели
	//	&& !(mb_LeftPanel && mb_RightPanel) // и хотя бы одна панель погашена
	//	&& (!con.m_ci.bVisible || con.m_ci.dwSize<30) // и сейчас НЕ включен режим граббера
	//	;
	CharAttr lca, lcaTableExt[0x100], lcaTableOrg[0x100], *lcaTable; // crForeColor, crBackColor, nFontIndex, nForeIdx, nBackIdx, crOrigForeColor, crOrigBackColor
	//COLORREF lcrForegroundColors[0x100], lcrBackgroundColors[0x100];
	//BYTE lnForegroundColors[0x100], lnBackgroundColors[0x100], lnFontByIndex[0x100];

	TODO("OPTIMIZE: В принципе, это можно делать не всегда, а только при изменениях");
	PrepareColorTable(bExtendFonts, lcaTableExt, lcaTableOrg, pApp);
	
	lcaTable = lcaTableOrg;

	// NonExclusive lock (need to ensure that buffers will not be recreated during processing)
	MSectionLock csData; csData.Lock(&csCON);
	con.bInGetConsoleData = TRUE;

	con.LastStartReadBufferTick = GetTickCount();

	HEAPVAL
	wchar_t wSetChar = L' ';
	CharAttr lcaDef;
	BYTE nDefTextAttr = (mp_RCon->GetDefaultBackColorIdx()<<4)|(mp_RCon->GetDefaultTextColorIdx());
	_ASSERTE(nDefTextAttr<countof(lcaTableOrg));
	lcaDef = lcaTable[nDefTextAttr]; // LtGray on Black
	//WORD    wSetAttr = 7;
	#ifdef _DEBUG
	wSetChar = (wchar_t)8776; //wSetAttr = 12;
	lcaDef = lcaTable[12]; // Red on Black
	#endif

	int nXMax = 0, nYMax = 0;
	int nX = 0, nY = 0;
	wchar_t  *pszDst = pChar;
	CharAttr *pcaDst = pAttr;

	if ((m_Type == rbt_DumpScreen) || (m_Type == rbt_Alternative) || (m_Type == rbt_Selection) || (m_Type == rbt_Find))
	{
		bDataValid = true;
		nXMax = min(nWidth, dump.crSize.X);
		int nFirstLine = con.m_sbi.srWindow.Top;
		nYMax = min(nHeight, (dump.crSize.Y - nFirstLine));
		wchar_t* pszSrc = dump.pszBlock1 + (nFirstLine * dump.crSize.X) + con.m_sbi.srWindow.Left;
		CharAttr* pcaSrc = dump.pcaBlock1 + (nFirstLine * dump.crSize.X) + con.m_sbi.srWindow.Left;
		for (int Y = 0; Y < nYMax; Y++)
		{
			wmemmove(pszDst, pszSrc, nXMax);
			memmove(pcaDst, pcaSrc, nXMax*sizeof(*pcaDst));

			// Иначе детектор глючит
			TODO("Избавиться от поля Flags. Вообще.");
			for (int i = 0; i < nXMax; i++)
				pcaDst[i].Flags = 0;

			if (nWidth > nXMax)
			{
				wmemset(pszDst+nXMax, wSetChar, nWidth-nXMax);
				for (int i = nXMax; i < nWidth; i++)
					pcaDst[i] = lcaDef;
			}

			pszDst += nWidth;
			pcaDst += nWidth;
			pszSrc += dump.crSize.X;
			pcaSrc += dump.crSize.X;
		}
		#ifdef _DEBUG
		*pszDst = 0; // для отладчика
		#endif
		// Очистка мусора за пределами идет ниже
	}
	else
	{
		if (!con.pConChar || !con.pConAttr)
		{
			nYMax = nHeight; // Мусор чистить не нужно, уже полный "reset"

			wmemset(pChar, wSetChar, cwDstBufSize);

			for (DWORD i = 0; i < cwDstBufSize; i++)
				pAttr[i] = lcaDef;

			//wmemset((wchar_t*)pAttr, wSetAttr, cbDstBufSize);
			//} else if (nWidth == con.nTextWidth && nHeight == con.nTextHeight) {
			//    TODO("Во время ресайза консоль может подглючивать - отдает не то что нужно...");
			//    //_ASSERTE(*con.pConChar!=ucBoxDblVert);
			//    memmove(pChar, con.pConChar, cbDstBufSize);
			//    WARNING("Это заменить на for");
			//    memmove(pAttr, con.pConAttr, cbDstBufSize);
		}
		else
		{
			bDataValid = true;

			TODO("Во время ресайза консоль может подглючивать - отдает не то что нужно...");
			//_ASSERTE(*con.pConChar!=ucBoxDblVert);
			nYMax = min(nHeight,con.nTextHeight);

			wchar_t  *pszSrc = con.pConChar;
			WORD     *pnSrc = con.pConAttr;

			// Т.к. есть блокировка (csData), то con.pConChar и con.pConAttr
			// не должны меняться во время выполнения этой функции
			const wchar_t* const pszSrcStart = con.pConChar;
			WORD* const pnSrcStart = con.pConAttr;
			size_t nSrcCells = (con.nTextWidth * con.nTextHeight);

			Assert(pszSrcStart==pszSrc && pnSrcStart==pnSrc);

			const AnnotationInfo *pcolSrc = NULL;
			const AnnotationInfo *pcolEnd = NULL;
			BOOL bUseColorData = FALSE, bStartUseColorData = FALSE;

			if (lbAllowHilightFileLine)
			{
				// Если мышь сместиласть - нужно посчитать
				// Даже если мышь не двигалась - текст мог измениться.
				/*if ((con.etr.mcr_FileLineStart.X == con.etr.mcr_FileLineEnd.X)
					|| (con.etr.mcr_FileLineStart.Y != mcr_LastMousePos.Y)
					|| (con.etr.mcr_FileLineStart.X > mcr_LastMousePos.X || con.etr.mcr_FileLineEnd.X < mcr_LastMousePos.X))*/
				if ((mp_RCon->mp_ABuf == this) && gpConEmu->isMeForeground())
				{
					ProcessFarHyperlink(false);
				}
			}

			if (gpSet->isTrueColorer
				&& mp_RCon->m_TrueColorerMap.IsValid()
				&& mp_RCon->mp_TrueColorerData
				/*&& mp_RCon->isFar()*/)
			{
				pcolSrc = mp_RCon->mp_TrueColorerData;
				pcolEnd = mp_RCon->mp_TrueColorerData + mp_RCon->m_TrueColorerMap.Ptr()->bufferSize;
				bUseColorData = TRUE;
				WARNING("Если far/w - pcolSrc нужно поднять вверх, bStartUseColorData=TRUE, bUseColorData=FALSE");
				if (con.bBufferHeight)
				{
					if (mp_RCon->isFar() && !mp_RCon->isFarBufferSupported())
					{
						bUseColorData = FALSE;
					}
					else
					{
						int nShiftRows = (con.m_sbi.dwSize.Y - nHeight) - con.m_sbi.srWindow.Top;
						_ASSERTE(nShiftRows>=0);
						if (nShiftRows > 0)
						{
							_ASSERTE(con.nTextWidth == (con.m_sbi.srWindow.Right - con.m_sbi.srWindow.Left + 1));
							pcolSrc -= nShiftRows*con.nTextWidth;
							bUseColorData = FALSE;
							bStartUseColorData = TRUE;
						}
					}
				}
			}

			DWORD cbDstLineSize = nWidth * 2;
			DWORD cnSrcLineLen = con.nTextWidth;
			DWORD cbSrcLineSize = cnSrcLineLen * 2;

			#ifdef _DEBUG
			if (con.nTextWidth != con.m_sbi.dwSize.X)
			{
				_ASSERTE(con.nTextWidth == con.m_sbi.dwSize.X);
			}
			#endif

			DWORD cbLineSize = min(cbDstLineSize,cbSrcLineSize);
			int nCharsLeft = max(0, (nWidth-con.nTextWidth));
			//int nY, nX;
			//120331 - Нехорошо заменять на "черный" с самого начала
			BYTE attrBackLast = 0;
			int nExtendStartsY = 0;
			//int nPrevDlgBorder = -1;
			
			bool lbStoreBackLast = false;
			if (bExtendColors)
			{
				BYTE FirstBackAttr = lcaTable[(*pnSrc) & 0xFF].nBackIdx;
				if (FirstBackAttr != nExtendColorIdx)
					attrBackLast = FirstBackAttr;

				const CEFAR_INFO_MAPPING* pFarInfo = lbIsFar ? mp_RCon->GetFarInfo() : NULL;
				if (pFarInfo)
				{
					// Если в качестве цвета "расширения" выбран цвет панелей - значит
					// пользователь просто настроил "другую" палитру для панелей фара.
					// К сожалению, таким образом нельзя заменить только цвета для элемента под курсором.
					if (((pFarInfo->nFarColors[col_PanelText] & 0xF0) >> 4) != nExtendColorIdx)
						lbStoreBackLast = true;
					else
						attrBackLast = FirstBackAttr;

					if (pFarInfo->FarInterfaceSettings.AlwaysShowMenuBar || mp_RCon->isEditor() || mp_RCon->isViewer())
						nExtendStartsY = 1; // пропустить обработку строки меню 
				}
				else
				{
					lbStoreBackLast = true;
				}
			}

			// Собственно данные
			for (nY = 0; nY < nYMax; nY++)
			{
				if ((pszSrcStart != con.pConChar) || (pnSrcStart != con.pConAttr))
				{
					Assert(pszSrcStart==con.pConChar && pnSrcStart==con.pConAttr);
					break;
				}

				if (nY == nExtendStartsY) lcaTable = lcaTableExt;

				// Текст
				memmove(pszDst, pszSrc, cbLineSize);

				if (nCharsLeft > 0)
					wmemset(pszDst+cnSrcLineLen, wSetChar, nCharsLeft);

				// Атрибуты
				DWORD atr = 0;

				if (mp_RCon->mn_InRecreate)
				{
					lca = lcaTable[7];

					for (nX = 0; nX < (int)cnSrcLineLen; nX++, pnSrc++, pcolSrc++)
					{
						pcaDst[nX] = lca;
					}
				}
				else
				{
					#if 0
					bool lbHilightFileLine = lbAllowHilightFileLine 
							&& (con.m_sel.dwFlags == 0)
							&& (nY == con.etr.mcr_FileLineStart.Y)
							&& (con.etr.mcr_FileLineStart.X < con.etr.mcr_FileLineEnd.X);
					#endif
					for (nX = 0; nX < (int)cnSrcLineLen; nX++, pnSrc++, pcolSrc++)
					{
						atr = (*pnSrc) & 0xFF; // интересут только нижний байт - там индексы цветов
						TODO("OPTIMIZE: lca = lcaTable[atr];");
						lca = lcaTable[atr];
						TODO("OPTIMIZE: вынести проверку bExtendColors за циклы");

						if (bExtendColors && (nY >= nExtendStartsY))
						{
							if (lca.nBackIdx == nExtendColorIdx)
							{
								lca.nBackIdx = attrBackLast; // фон нужно заменить на обычный цвет из соседней ячейки
								lca.nForeIdx += 0x10;
								lca.crForeColor = lca.crOrigForeColor = mp_RCon->mp_VCon->mp_Colors[lca.nForeIdx];
								lca.crBackColor = lca.crOrigBackColor = mp_RCon->mp_VCon->mp_Colors[lca.nBackIdx];
							}
							else if (lbStoreBackLast)
							{
								attrBackLast = lca.nBackIdx; // запомним обычный цвет предыдущей ячейки
							}
						}

						// Colorer & Far - TrueMod
						TODO("OPTIMIZE: вынести проверку bUseColorData за циклы");

						if (bStartUseColorData)
						{
							// В случае "far /w" буфер цвета может начаться НИЖЕ верхней видимой границы,
							// если буфер немного прокручен вверх
							if (pcolSrc >= mp_RCon->mp_TrueColorerData)
								bUseColorData = TRUE;
						}
						
						if (bUseColorData)
						{
							if (pcolSrc >= pcolEnd)
							{
								bUseColorData = FALSE;
							}
							else
							{
								TODO("OPTIMIZE: доступ к битовым полям тяжело идет...");

								if (pcolSrc->fg_valid)
								{
									lca.nFontIndex = 0; //bold/italic/underline, выставляется ниже
									lca.crForeColor = lbFade ? gpSet->GetFadeColor(pcolSrc->fg_color) : pcolSrc->fg_color;

									if (pcolSrc->bk_valid)
										lca.crBackColor = lbFade ? gpSet->GetFadeColor(pcolSrc->bk_color) : pcolSrc->bk_color;
								}
								else if (pcolSrc->bk_valid)
								{
									lca.nFontIndex = 0; //bold/italic/underline, выставляется ниже
									lca.crBackColor = lbFade ? gpSet->GetFadeColor(pcolSrc->bk_color) : pcolSrc->bk_color;
								}

								// nFontIndex: 0 - normal, 1 - bold, 2 - italic, 3 - bold&italic,..., 4 - underline, ...
								if (pcolSrc->style)
									lca.nFontIndex = pcolSrc->style & 7;
							}
						}

						//if (lbHilightFileLine && (nX >= con.etr.mcr_FileLineStart.X) && (nX <= con.etr.mcr_FileLineEnd.X))
						//	lca.nFontIndex |= 4; // Отрисовать его как Underline

						TODO("OPTIMIZE: lca = lcaTable[atr];");
						pcaDst[nX] = lca;
						//memmove(pcaDst, pnSrc, cbLineSize);
					}

					#if 0
					if (lbHilightFileLine)
					{
						int nFrom = con.etr.mcr_FileLineStart.X;
						int nTo = min(con.etr.mcr_FileLineEnd.X,(int)cnSrcLineLen);
						for (nX = nFrom; nX <= nTo; nX++)
						{
							pcaDst[nX].nFontIndex |= 4; // Отрисовать его как Underline
						}
					}
					#endif
				}

				// Залить остаток (если запрошен больший участок, чем есть консоль
				for (nX = cnSrcLineLen; nX < nWidth; nX++)
				{
					pcaDst[nX] = lcaDef;
				}

				// Far2 показывате красный 'A' в правом нижнем углу консоли
				// Этот ярко красный цвет фона может попасть в Extend Font Colors
				if (bExtendFonts && ((nY+1) == nYMax) && mp_RCon->isFar()
						&& (pszDst[nWidth-1] == L'A') && (atr == 0xCF))
				{
					// Вернуть "родной" цвет и шрифт
					pcaDst[nWidth-1] = lcaTable[atr];
				}

				// Next line
				pszDst += nWidth; pszSrc += cnSrcLineLen;
				pcaDst += nWidth; //pnSrc += con.nTextWidth;
			}

			#ifndef __GNUC__
			UNREFERENCED_PARAMETER(pszSrcStart);
			UNREFERENCED_PARAMETER(pnSrcStart);
			#endif
			UNREFERENCED_PARAMETER(nSrcCells);
		}
	} // rbt_Primary

	// Если вдруг запросили большую высоту, чем текущая в консоли - почистить низ
	for (nY = nYMax; nY < nHeight; nY++)
	{
		wmemset(pszDst, wSetChar, nWidth);
		pszDst += nWidth;

		//wmemset((wchar_t*)pcaDst, wSetAttr, nWidth);
		for (nX = 0; nX < nWidth; nX++)
		{
			pcaDst[nX] = lcaDef;
		}

		pcaDst += nWidth;
	}

	// Чтобы безопасно использовать строковые функции - гарантированно делаем ASCIIZ. Хотя pChars может и \0 содержать?
	*pszDst = 0;

	// Update hyperlinks and other underlines
	if (lbAllowHilightFileLine && (con.etr.etrLast != etr_None))
	{
		etr.mcr_FileLineStart = BufferToScreen(con.etr.mcr_FileLineStart);
		etr.mcr_FileLineEnd = BufferToScreen(con.etr.mcr_FileLineEnd);
		etr.etrLast = con.etr.etrLast;
	}
	else
	{
		etr.etrLast = etr_None;
	}

	if (bDataValid)
	{
		// Подготовить данные для Transparent
		// обнаружение диалогов нужно только при включенной прозрачности,
		// или при пропорциональном шрифте
		// Даже если НЕ (gpSet->NeedDialogDetect()) - нужно сбросить количество прямоугольников.
		PrepareTransparent(pChar, pAttr, nWidth, nHeight);

		if (mn_LastRgnFlags != m_Rgn.GetFlags())
		{
			// Попытаться найти панели и обновить флаги
			FindPanels();

			WARNING("Не думаю, что это хорошее место для обновления мышиного курсора");
			// Обновить мышиный курсор
			if (mp_RCon->isActive() && this == mp_RCon->mp_ABuf)
			{
				PostMessage(mp_RCon->GetView(), WM_SETCURSOR, -1, -1);
			}

			mn_LastRgnFlags = m_Rgn.GetFlags();
		}

		if (con.m_sel.dwFlags)
		{
			bool lbStreamMode = (con.m_sel.dwFlags & CONSOLE_TEXT_SELECTION) == CONSOLE_TEXT_SELECTION;
			// srSelection in Absolute coordinates now!
			// Поскольку здесь нас интересует только отображение - можно поступить просто
			COORD crStart = BufferToScreen(MakeCoord(con.m_sel.srSelection.Left, con.m_sel.srSelection.Top));
			COORD crEnd = BufferToScreen(MakeCoord(con.m_sel.srSelection.Right, con.m_sel.srSelection.Bottom));

			bool bAboveScreen = (con.m_sel.srSelection.Top < con.m_sbi.srWindow.Top);
			bool bBelowScreen = (con.m_sel.srSelection.Bottom > con.m_sbi.srWindow.Bottom);

			SMALL_RECT rc = {crStart.X, crStart.Y, crEnd.X, crEnd.Y};
			// Коррекция по видимой области
			MinMax(rc.Left, 0, nWidth-1); MinMax(rc.Right, 0, nWidth-1);
			MinMax(rc.Top, 0, nHeight-1); MinMax(rc.Bottom, 0, nHeight-1);

			// для прямоугольника выделения сбрасываем прозрачность и ставим стандартный цвет выделения (lcaSel)
			//CharAttr lcaSel = lcaTable[gpSet->isCTSColorIndex]; // Black on LtGray
			BYTE nForeIdx = (gpSet->isCTSColorIndex & 0xF);
			COLORREF crForeColor = mp_RCon->mp_VCon->mp_Colors[nForeIdx];
			BYTE nBackIdx = (gpSet->isCTSColorIndex & 0xF0) >> 4;
			COLORREF crBackColor = mp_RCon->mp_VCon->mp_Colors[nBackIdx];
			int nX1, nX2;

			
			for (nY = rc.Top; nY <= rc.Bottom; nY++)
			{
				if (!lbStreamMode)
				{
					// Block mode
					nX1 = rc.Left; nX2 = rc.Right;
				}
				else
				{
					nX1 = (nY == rc.Top && !bAboveScreen) ? rc.Left : 0;
					nX2 = (nY == rc.Bottom && !bBelowScreen) ? rc.Right : (nWidth-1);
				}

				pcaDst = pAttr + nWidth*nY + nX1;

				for (nX = nX1; nX <= nX2; nX++, pcaDst++)
				{
					//pcaDst[nX] = lcaSel; -- чтобы не сбрасывать флаги рамок и диалогов - ставим по полям
					pcaDst->crForeColor = pcaDst->crOrigForeColor = crForeColor;
					pcaDst->crBackColor = pcaDst->crOrigBackColor = crBackColor;
					pcaDst->nForeIdx = nForeIdx;
					pcaDst->nBackIdx = nBackIdx;
					#if 0
					pcaDst->bTransparent = FALSE;
					#else
					pcaDst->Flags &= ~CharAttr_Transparent;
					#endif
					pcaDst->ForeFont = 0;
				}
			}

			//} else {
			//	int nX1, nX2;
			//	for (nY = rc.Top; nY <= rc.Bottom; nY++) {
			//		pnDst = pAttr + nWidth*nY;
			//		nX1 = (nY == rc.Top) ? rc.Left : 0;
			//		nX2 = (nY == rc.Bottom) ? rc.Right : (nWidth-1);
			//		for (nX = nX1; nX <= nX2; nX++) {
			//			pnDst[nX] = lcaSel;
			//		}
			//	}
			//}
		}
	}

	// Если требуется показать "статус" - принудительно перебиваем первую видимую строку возвращаемого буфера
	if (!gpSet->isStatusBarShow && mp_RCon->ms_ConStatus[0])
	{
		int nLen = _tcslen(mp_RCon->ms_ConStatus);
		wmemcpy(pChar, mp_RCon->ms_ConStatus, nLen);

		if (nWidth>nLen)
			wmemset(pChar+nLen, L' ', nWidth-nLen);

		//wmemset((wchar_t*)pAttr, 0x47, nWidth);
		lca = lcaTableExt[7];

		for(int i = 0; i < nWidth; i++)
			pAttr[i] = lca;
	}

	con.LastEndReadBufferTick = GetTickCount();

	//FIN
	HEAPVAL
	con.bInGetConsoleData = FALSE;
	csData.Unlock();
	return;
}

DWORD_PTR CRealBuffer::GetKeybLayout()
{
	return con.dwKeybLayout;
}

void CRealBuffer::SetKeybLayout(DWORD_PTR anNewKeyboardLayout)
{
	con.dwKeybLayout = anNewKeyboardLayout;
}

DWORD CRealBuffer::GetConMode()
{
	return con.m_dwConsoleMode;
}

int CRealBuffer::GetStatusLineCount(int nLeftPanelEdge)
{
	if (!this)
		return 0;

	if (!mp_RCon->isFar() || !con.pConChar || !con.nTextWidth)
		return 0;
	
	int nBottom, nLeft;
	if (nLeftPanelEdge > mr_LeftPanelFull.left)
	{
		nBottom = mr_RightPanelFull.bottom;
		nLeft = mr_RightPanelFull.left;
	}
	else
	{
		nBottom = mr_LeftPanelFull.bottom;
		nLeft = mr_LeftPanelFull.left;
	}
	if (nBottom < 5)
		return 0; // минимальная высота панели

	for (int i = 2; i <= 11 && i < nBottom; i++)
	{
		if (con.pConChar[con.nTextWidth*(nBottom-i)+nLeft] == ucBoxDblVertSinglRight)
		{
			return (i - 1);
		}
	}
	
	return 0;
}

void CRealBuffer::PrepareTransparent(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight)
{
	//if (!mp_ConsoleInfo)
	if (!pChar || !pAttr)
		return;

	TODO("Хорошо бы m_FarInfo тоже в дамп скидывать.");
	CEFAR_INFO_MAPPING FI;
	
	if (m_Type == rbt_Primary)
	{
		FI = mp_RCon->m_FarInfo;

		// На (mp_RCon->isViewer() || mp_RCon->isEditor()) ориентироваться
		// нельзя, т.к. CheckFarStates еще не был вызван
		BOOL bViewerOrEditor = FALSE;
		if (mp_RCon->GetFarPID(TRUE))
		{
			int nTabType = mp_RCon->GetActiveTabType();
			bViewerOrEditor = ((nTabType & 0xFF) == 2 || (nTabType & 0xFF) == 3);
		}

		if (!bViewerOrEditor)
		{
			WARNING("Неправильно это, ведь FindPanels еще не был вызван");

			if (mb_LeftPanel)
			{
				FI.bFarLeftPanel = true;
				FI.FarLeftPanel.PanelRect = mr_LeftPanelFull;
			}
			else
			{
				FI.bFarLeftPanel = false;
			}

			if (mb_RightPanel)
			{
				FI.bFarRightPanel = true;
				FI.FarRightPanel.PanelRect = mr_RightPanelFull;
			}
			else
			{
				FI.bFarRightPanel = false;
			}

			FI.bViewerOrEditor = FALSE;
		}
		else
		{
			FI.bViewerOrEditor = TRUE;
			FI.bFarLeftPanel = false;
			FI.bFarRightPanel = false;
		}

		//if (!FI.bFarLeftPanel && !FI.bFarRightPanel)
		//{
		//	// Если нет панелей - это может быть вьювер/редактор
		//	FI.bViewerOrEditor = (mp_RCon->isViewer() || mp_RCon->isEditor());
		//}
	}
	else
	{
		ZeroStruct(FI);
		TODO("Загружать CEFAR_INFO_MAPPING из дампа");
	}

	m_Rgn.SetNeedTransparency(gpSet->isUserScreenTransparent);
	TODO("При загрузке дампа хорошо бы из него и палитру фара доставать/отдавать");
	m_Rgn.PrepareTransparent(&FI, mp_RCon->mp_VCon->mp_Colors, GetSBI(), pChar, pAttr, nWidth, nHeight);
	
	#ifdef _DEBUG
	int nCount = m_Rgn.GetDetectedDialogs(0,NULL,NULL);

	if (nCount == 1)
	{
		nCount = 1;
	}
	#endif
}

// Найти панели, обновить mp_RCon->mn_ConsoleProgress
void CRealBuffer::FindPanels()
{
	TODO("Положение панелей можно бы узнавать из плагина");
	WARNING("!!! Нужно еще сохранять флажок 'Меню сейчас показано'");
	
	#ifdef _DEBUG
	if (mp_RCon->mb_DebugLocked)
		return;
	WARNING("this==mp_RCon->mp_RBuf ?");
	#endif

	RECT rLeftPanel = MakeRect(-1,-1);
	RECT rLeftPanelFull = rLeftPanel;
	RECT rRightPanel = rLeftPanel;
	RECT rRightPanelFull = rLeftPanel;
	BOOL bLeftPanel = FALSE;
	BOOL bRightPanel = FALSE;
	BOOL bMayBePanels = FALSE;
	BOOL lbNeedUpdateSizes = FALSE;
	BOOL lbPanelsChanged = FALSE;
	short nLastProgress = mp_RCon->m_Progress.ConsoleProgress;
	short nNewProgress = -1;
	/*
	Имеем облом. При ресайзе панелей CtrlLeft/CtrlRight иногда сервер считывает
	содержимое консоли ДО окончания вывода в нее новой информации. В итоге верхняя
	часть считанного не соответствует нижней, что влечет ошибку
	определения панелей в CRealConsole::FindPanels - ошибочно считает, что
	Левая панель - полноэкранная :(
	*/
	WARNING("Добавить проверки по всем граням панелей, чтобы на них не было некорректных символов");
	// То есть на грани панели не было других диалогов (вертикальных/угловых бордюров поверх горизонтальной части панели)

	// функция проверяет (mn_ProgramStatus & CES_FARACTIVE) и т.п.
	if (mp_RCon->isFar())
	{
		if (mp_RCon->isFar(TRUE) && mp_RCon->m_FarInfo.cbSize)
		{
			if (mp_RCon->m__FarInfo.Ptr() && mp_RCon->m__FarInfo.Ptr()->nFarInfoIdx != mp_RCon->m_FarInfo.nFarInfoIdx)
			{
				mp_RCon->m__FarInfo.GetTo(&mp_RCon->m_FarInfo, sizeof(mp_RCon->m_FarInfo));
			}
		}

		// Если активен редактор или вьювер (или диалоги, копирование, и т.п.) - искать бессмысленно
		if ((mp_RCon->mn_FarStatus & CES_NOTPANELFLAGS) == 0)
			bMayBePanels = TRUE; // только если нет
	}

	if (bMayBePanels && con.nTextHeight >= MIN_CON_HEIGHT && con.nTextWidth >= MIN_CON_WIDTH)
	{
		uint nY = 0;
		BOOL lbIsMenu = FALSE;

		if (con.pConChar[0] == L' ')
		{
			lbIsMenu = TRUE;

			for(int i=0; i<con.nTextWidth; i++)
			{
				if (con.pConChar[i]==ucBoxDblHorz || con.pConChar[i]==ucBoxDblDownRight || con.pConChar[i]==ucBoxDblDownLeft)
				{
					lbIsMenu = FALSE; break;
				}
			}

			if (lbIsMenu)
				nY ++; // скорее всего, первая строка - меню
		}
		else if ((((con.pConChar[0] == L'R') && ((con.pConAttr[0] & 0xFF) == 0x4F))
					|| ((con.pConChar[0] == L'P') && ((con.pConAttr[0] & 0xFF) == 0x2F)))
			&& con.pConChar[1] == L' ')
		{
			for(int i=1; i<con.nTextWidth; i++)
			{
				if (con.pConChar[i]==ucBoxDblHorz || con.pConChar[i]==ucBoxDblDownRight || con.pConChar[i]==ucBoxDblDownLeft)
				{
					lbIsMenu = FALSE; break;
				}
			}

			if (lbIsMenu)
				nY ++; // скорее всего, первая строка - меню, при включенной записи макроса
		}

		uint nIdx = nY*con.nTextWidth;
		// Левая панель
		BOOL bFirstCharOk = (nY == 0)
		                    && (
		                        (con.pConChar[0] == L'R' && (con.pConAttr[0] & 0xFF) == 0x4F) // символ записи макроса
		                        || (con.pConChar[0] == L'P' && (con.pConAttr[0] & 0xFF) == 0x2F) // символ воспроизведения макроса
		                    );

		BOOL bFarShowColNames = TRUE;
		BOOL bFarShowStatus = TRUE;
		const CEFAR_INFO_MAPPING *pFar = NULL;
		if (mp_RCon->m_FarInfo.cbSize)
		{
			pFar = &mp_RCon->m_FarInfo;
			if (pFar)
			{
				if ((pFar->FarPanelSettings.ShowColumnTitles) == 0) //-V112
					bFarShowColNames = FALSE;
				if ((pFar->FarPanelSettings.ShowStatusLine) == 0)
					bFarShowStatus = FALSE;
			}
		}

		// из-за глюков индикации FAR2 пока вместо '[' - любой символ
		//if (( ((bFirstCharOk || con.pConChar[nIdx] == L'[') && (con.pConChar[nIdx+1]>=L'0' && con.pConChar[nIdx+1]<=L'9')) // открыто несколько редакторов/вьюверов
		if ((
		            ((bFirstCharOk || con.pConChar[nIdx] != ucBoxDblDownRight)
		             && (con.pConChar[nIdx+1]>=L'0' && con.pConChar[nIdx+1]<=L'9')) // открыто несколько редакторов/вьюверов
		            ||
		            ((bFirstCharOk || con.pConChar[nIdx] == ucBoxDblDownRight)
		             && (((con.pConChar[nIdx+1] == ucBoxDblHorz || con.pConChar[nIdx+1] == L' ') && bFarShowColNames)
		                 || con.pConChar[nIdx+1] == ucBoxSinglDownDblHorz // доп.окон нет, только рамка
						 || con.pConChar[nIdx+1] == ucBoxDblDownDblHorz
		                 || (con.pConChar[nIdx+1] == L'[' && con.pConChar[nIdx+2] == ucLeftScroll) // ScreenGadgets, default
						 || (!bFarShowColNames && !(con.pConChar[nIdx+1] == ucBoxDblHorz || con.pConChar[nIdx+1] == L' ')
							&& con.pConChar[nIdx+1] != ucBoxSinglDownDblHorz && con.pConChar[nIdx+1] != ucBoxDblDownDblHorz)
		                ))
		        )
		        && con.pConChar[nIdx+con.nTextWidth] == ucBoxDblVert) // двойная рамка продолжается вниз
		{
			for(int i=2; !bLeftPanel && i<con.nTextWidth; i++)
			{
				// Найти правый край левой панели
				if (con.pConChar[nIdx+i] == ucBoxDblDownLeft
				        && ((con.pConChar[nIdx+i-1] == ucBoxDblHorz || con.pConChar[nIdx+i-1] == L' ')
				            || con.pConChar[nIdx+i-1] == ucBoxSinglDownDblHorz // правый угол панели
							|| con.pConChar[nIdx+i-1] == ucBoxDblDownDblHorz
				            || (con.pConChar[nIdx+i-1] == L']' && con.pConChar[nIdx+i-2] == L'\\') // ScreenGadgets, default
							)
				        // МОЖЕТ быть закрыто AltHistory
				        /*&& con.pConChar[nIdx+i+con.nTextWidth] == ucBoxDblVert*/)
				{
					uint nBottom = con.nTextHeight - 1;

					while(nBottom > 4) //-V112
					{
						if (con.pConChar[con.nTextWidth*nBottom] == ucBoxDblUpRight
						        /*&& con.pConChar[con.nTextWidth*nBottom+i] == ucBoxDblUpLeft*/)
						{
							rLeftPanel.left = 1;
							rLeftPanel.top = nY + (bFarShowColNames ? 2 : 1);
							rLeftPanel.right = i-1;
							if (bFarShowStatus)
							{
								rLeftPanel.bottom = nBottom - 3;
								for (int j = (nBottom - 3); j > (int)(nBottom - 13) && j > rLeftPanel.top; j--)
								{
									if (con.pConChar[con.nTextWidth*j] == ucBoxDblVertSinglRight)
									{
										rLeftPanel.bottom = j - 1; break;
									}
								}
							}
							else
							{
								rLeftPanel.bottom = nBottom - 1;
							}
							rLeftPanelFull.left = 0;
							rLeftPanelFull.top = nY;
							rLeftPanelFull.right = i;
							rLeftPanelFull.bottom = nBottom;
							bLeftPanel = TRUE;
							break;
						}

						nBottom --;
					}
				}
			}
		}

		// (Если есть левая панель и она не FullScreen) или левой панели нет вообще
		if ((bLeftPanel && rLeftPanelFull.right < con.nTextWidth) || !bLeftPanel)
		{
			if (bLeftPanel)
			{
				// Положение известно, нужно только проверить наличие
				if (con.pConChar[nIdx+rLeftPanelFull.right+1] == ucBoxDblDownRight
				        /*&& con.pConChar[nIdx+rLeftPanelFull.right+1+con.nTextWidth] == ucBoxDblVert*/
				        /*&& con.pConChar[nIdx+con.nTextWidth*2] == ucBoxDblVert*/
				        /*&& con.pConChar[(rLeftPanelFull.bottom+3)*con.nTextWidth+rLeftPanelFull.right+1] == ucBoxDblUpRight*/
				        && con.pConChar[(rLeftPanelFull.bottom+1)*con.nTextWidth-1] == ucBoxDblUpLeft
				  )
				{
					rRightPanel = rLeftPanel; // bottom & top берем из rLeftPanel
					rRightPanel.left = rLeftPanelFull.right+2;
					rRightPanel.right = con.nTextWidth-2;
					rRightPanelFull = rLeftPanelFull;
					rRightPanelFull.left = rLeftPanelFull.right+1;
					rRightPanelFull.right = con.nTextWidth-1;
					bRightPanel = TRUE;
				}
			}

			// Начиная с FAR2 build 1295 панели могут быть разной высоты
			// или левой панели нет
			// или активная панель в FullScreen режиме
			if (!bRightPanel)
			{
				// нужно определить положение панели
				if (((con.pConChar[nIdx+con.nTextWidth-1]>=L'0' && con.pConChar[nIdx+con.nTextWidth-1]<=L'9')  // справа часы
				        || con.pConChar[nIdx+con.nTextWidth-1] == ucBoxDblDownLeft) // или рамка
				        && (con.pConChar[nIdx+con.nTextWidth*2-1] == ucBoxDblVert // ну и правая граница панели
							|| con.pConChar[nIdx+con.nTextWidth*2-1] == ucUpScroll) // или стрелка скроллбара
						)
				{
					int iMinFindX = bLeftPanel ? (rLeftPanelFull.right+1) : 0;
					for(int i=con.nTextWidth-3; !bRightPanel && i>=iMinFindX; i--)
					{
						// ищем левую границу правой панели
						if (con.pConChar[nIdx+i] == ucBoxDblDownRight
						        && (((con.pConChar[nIdx+i+1] == ucBoxDblHorz || con.pConChar[nIdx+i+1] == L' ') && bFarShowColNames)
						            || con.pConChar[nIdx+i+1] == ucBoxSinglDownDblHorz // правый угол панели
									|| con.pConChar[nIdx+i+1] == ucBoxDblDownDblHorz
						            || (con.pConChar[nIdx+i-1] == L']' && con.pConChar[nIdx+i-2] == L'\\') // ScreenGadgets, default
									|| (!bFarShowColNames && !(con.pConChar[nIdx+i+1] == ucBoxDblHorz || con.pConChar[nIdx+i+1] == L' ')
										&& con.pConChar[nIdx+i+1] != ucBoxSinglDownDblHorz && con.pConChar[nIdx+i+1] != ucBoxDblDownDblHorz)
									)
						        // МОЖЕТ быть закрыто AltHistory
						        /*&& con.pConChar[nIdx+i+con.nTextWidth] == ucBoxDblVert*/)
						{
							uint nBottom = con.nTextHeight - 1;

							while(nBottom > 4) //-V112
							{
								if (/*con.pConChar[con.nTextWidth*nBottom+i] == ucBoxDblUpRight
									&&*/ con.pConChar[con.nTextWidth*(nBottom+1)-1] == ucBoxDblUpLeft)
								{
									rRightPanel.left = i+1;
									rRightPanel.top = nY + (bFarShowColNames ? 2 : 1);
									rRightPanel.right = con.nTextWidth-2;
									//rRightPanel.bottom = nBottom - 3;
									if (bFarShowStatus)
									{
										rRightPanel.bottom = nBottom - 3;
										for (int j = (nBottom - 3); j > (int)(nBottom - 13) && j > rRightPanel.top; j--)
										{
											if (con.pConChar[con.nTextWidth*j+i] == ucBoxDblVertSinglRight)
											{
												rRightPanel.bottom = j - 1; break;
											}
										}
									}
									else
									{
										rRightPanel.bottom = nBottom - 1;
									}
									rRightPanelFull.left = i;
									rRightPanelFull.top = nY;
									rRightPanelFull.right = con.nTextWidth-1;
									rRightPanelFull.bottom = nBottom;
									bRightPanel = TRUE;
									break;
								}

								nBottom --;
							}
						}
					}
				}
			}
		}
	}

#ifdef _DEBUG

	if (bLeftPanel && !bRightPanel && rLeftPanelFull.right > 120)
	{
		bLeftPanel = bLeftPanel;
	}

#endif

	if (mp_RCon->isActive())
		lbNeedUpdateSizes = (memcmp(&mr_LeftPanel,&rLeftPanel,sizeof(mr_LeftPanel)) || memcmp(&mr_RightPanel,&rRightPanel,sizeof(mr_RightPanel)));

	lbPanelsChanged = lbNeedUpdateSizes || (mb_LeftPanel != bLeftPanel) || (mb_RightPanel != bRightPanel)
	                  || ((bLeftPanel || bRightPanel) && ((mp_RCon->mn_FarStatus & CES_FILEPANEL) == 0));
	mr_LeftPanel = rLeftPanel; mr_LeftPanelFull = rLeftPanelFull; mb_LeftPanel = bLeftPanel;
	mr_RightPanel = rRightPanel; mr_RightPanelFull = rRightPanelFull; mb_RightPanel = bRightPanel;

	if (bRightPanel || bLeftPanel)
		bMayBePanels = TRUE;
	else
		bMayBePanels = FALSE;

	nNewProgress = -1;

	// mn_ProgramStatus не катит. Хочется подхватывать прогресс из плагина Update, а там как раз CES_FARACTIVE
	//if (!abResetOnly && (mn_ProgramStatus & CES_FARACTIVE) == 0) {
	if (!bMayBePanels && (mp_RCon->mn_FarStatus & CES_NOTPANELFLAGS) == 0)
	{
		// Обработка прогресса NeroCMD и пр. консольных программ
		// Если курсор находится в видимой области
		int nY = con.m_sbi.dwCursorPosition.Y - con.m_sbi.srWindow.Top;

		if (/*con.m_ci.bVisible && con.m_ci.dwSize -- Update прячет курсор?
			&&*/ con.m_sbi.dwCursorPosition.X >= 0 && con.m_sbi.dwCursorPosition.X < con.nTextWidth
		    && nY >= 0 && nY < con.nTextHeight)
		{
			int nIdx = nY * con.nTextWidth;
			// Обработка прогресса NeroCMD и пр. консольных программ (если курсор находится в видимой области)
			nNewProgress = CheckProgressInConsole(con.pConChar+nIdx);
		}
	}

	if (mp_RCon->m_Progress.ConsoleProgress != nNewProgress || nLastProgress != nNewProgress)
	{
		// Запомнить, что получили из консоли
		mp_RCon->setConsoleProgress(nNewProgress);
		// Показать прогресс в заголовке
		mp_RCon->mb_ForceTitleChanged = TRUE;
	}

	if (lbPanelsChanged)
	{
		// Нужно вызвать (возможно повторно), чтобы обновить флаги состояний
		mp_RCon->CheckFarStates();
	}

	if (lbNeedUpdateSizes)
		gpConEmu->UpdateSizes();
}

bool CRealBuffer::isSelectionAllowed()
{
	if (!this)
		return false;

	#ifdef _DEBUG
	// Зачем звать эту функц из фоновых потоков?
	if (!gpConEmu->isMainThread())
	{
		int nDbg = -1;
	}
	#endif

	if (!con.pConChar || !con.pConAttr)
		return false; // Если данных консоли еще нет

	if (con.m_sel.dwFlags != 0)
		return true; // Если выделение было запущено через меню

	if (!gpSet->isCTSIntelligent)
		return false; // Mouse selection was disabled
	LPCWSTR pszPrcName = mp_RCon->GetActiveProcessName();
	if (!pszPrcName)
		return false; // No process - no selection
	LPCWSTR pszExcl = gpSet->GetIntelligentExceptionsMSZ();
	// Check exclusions
	if (pszExcl)
	{
		while (*pszExcl)
		{
			if (lstrcmpi(pszExcl, L"far") == 0)
			{
				// Tricky a little
				// Editor and panels - send mouse to console
				// Userscreen and viewer - use mouse for selection
				// If user want to send mouse to console always - set "far.exe" instead of "far"
				if (mp_RCon->isFar())
				{
					if (mp_RCon->isViewer())
						break; // Allow in viewer
					else if (mp_RCon->isEditor() || mp_RCon->isFilePanel(true, true))
						return false;
					else
					{
						DWORD nDlgFlags = m_Rgn.GetFlags();
						int nDialogs = m_Rgn.GetDetectedDialogs(3,NULL,NULL);
						if (nDialogs > 0)
							return false; // Any dialog on screen? Don't select
					}
				}
			}
			else if (lstrcmpi(pszExcl, pszPrcName) == 0)
			{
				// This app in the restricted list
				// Seems like it uses mouse for internal selection, dragging and so on...
				return false;
			}
			pszExcl += _tcslen(pszExcl)+1;
		}
	}

	return true;
}

bool CRealBuffer::isSelectionPresent()
{
	if (!this)
		return false;

	return (con.m_sel.dwFlags != 0);
}

bool CRealBuffer::isMouseSelectionPresent()
{
	if (!this)
		return false;

	return ((con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION) != 0);
}

bool CRealBuffer::GetConsoleSelectionInfo(CONSOLE_SELECTION_INFO *sel)
{
	if (!this)
		return false;

	#if 0
	// If it was already started - let it be... Don't look at settings
	if (!isSelectionAllowed())
		return false;
	#endif

	if (sel)
	{
		*sel = con.m_sel;
	}

	return isSelectionPresent();
}

void CRealBuffer::ConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci)
{
	if (!this) return;

	*ci = con.m_ci;

	// Если сейчас выделяется текст мышкой (ConEmu internal)
	// то курсор нужно переключить в половину знакоместа!
	if (isSelectionPresent())
	{
		TODO("А может ну его, эту половину знакоместа? Или по настройке?");
		if (ci->dwSize < 50)
			ci->dwSize = 50;
	}
	else
	{
		const Settings::AppSettings* pApp = gpSet->GetAppSettings(mp_RCon->GetActiveAppSettingsId());
		bool bActive = mp_RCon->isInFocus();
		if (pApp->CursorIgnoreSize(bActive))
			ci->dwSize = pApp->CursorFixedSize(bActive);
	}
}

void CRealBuffer::GetCursorInfo(COORD* pcr, CONSOLE_CURSOR_INFO* pci)
{
	if (pcr)
	{
		*pcr = con.m_sbi.dwCursorPosition;
	}

	if (pci)
	{
		//*pci = con.m_ci;
		ConsoleCursorInfo(pci);
	}
}

void CRealBuffer::ConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO* sbi)
{
	if (!this) return;

	*sbi = con.m_sbi;

	if (con.m_sel.dwFlags)
	{
		// В режиме выделения - положение курсора ставим сами
		ConsoleCursorPos(&(sbi->dwCursorPosition));
		//if (con.m_sel.dwSelectionAnchor.X == con.m_sel.srSelection.Left)
		//	sbi->dwCursorPosition.X = con.m_sel.srSelection.Right;
		//else
		//	sbi->dwCursorPosition.X = con.m_sel.srSelection.Left;
		//
		//if (con.m_sel.dwSelectionAnchor.Y == con.m_sel.srSelection.Top)
		//	sbi->dwCursorPosition.Y = con.m_sel.srSelection.Bottom;
		//else
		//	sbi->dwCursorPosition.Y = con.m_sel.srSelection.Top;
	}
}

void CRealBuffer::ConsoleCursorPos(COORD *pcr)
{
	if (con.m_sel.dwFlags)
	{
		// В режиме выделения - положение курсора ставим сами
		// con.m_sel.srSelection уже содержит "абсолютные" координаты

		if (con.m_sel.dwSelectionAnchor.X == con.m_sel.srSelection.Left)
			pcr->X = con.m_sel.srSelection.Right;
		else
			pcr->X = con.m_sel.srSelection.Left;

		if (con.m_sel.dwSelectionAnchor.Y == con.m_sel.srSelection.Top)
			pcr->Y = con.m_sel.srSelection.Bottom; // + con.nTopVisibleLine;
		else
			pcr->Y = con.m_sel.srSelection.Top; // + con.nTopVisibleLine;
	}
	else
	{
		*pcr = con.m_sbi.dwCursorPosition;
	}
}

void CRealBuffer::ResetBuffer()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return;
	}
	con.m_ci.bVisible = TRUE;
	con.m_ci.dwSize = 15;
	con.m_sbi.dwCursorPosition = MakeCoord(0,con.m_sbi.srWindow.Top);

	if (con.pConChar && con.pConAttr)
	{
		MSectionLock sc; sc.Lock(&csCON);
		DWORD OneBufferSize = con.nTextWidth*con.nTextHeight*sizeof(wchar_t);
		memset(con.pConChar,0,OneBufferSize);
		memset(con.pConAttr,0,OneBufferSize);
	}
}

// Преобразовать абсолютные координаты консоли в координаты нашего буфера
// (вычесть номер верхней видимой строки и скорректировать видимую высоту)
bool CRealBuffer::ConsoleRect2ScreenRect(const RECT &rcCon, RECT *prcScr)
{
	if (!this) return false;

	*prcScr = rcCon;

	if (con.bBufferHeight && con.nTopVisibleLine)
	{
		prcScr->top -= con.nTopVisibleLine;
		prcScr->bottom -= con.nTopVisibleLine;
	}

	bool lbRectOK = true;

	if (prcScr->left == 0 && prcScr->right >= con.nTextWidth)
		prcScr->right = con.nTextWidth - 1;

	if (prcScr->left)
	{
		if (prcScr->left >= con.nTextWidth)
			return false;

		if (prcScr->right >= con.nTextWidth)
			prcScr->right = con.nTextWidth - 1;
	}

	if (prcScr->bottom < 0)
	{
		lbRectOK = false; // полностью уехал за границу вверх
	}
	else if (prcScr->top >= con.nTextHeight)
	{
		lbRectOK = false; // полностью уехал за границу вниз
	}
	else
	{
		// Скорректировать по видимому прямоугольнику
		if (prcScr->top < 0)
			prcScr->top = 0;

		if (prcScr->bottom >= con.nTextHeight)
			prcScr->bottom = con.nTextHeight - 1;

		lbRectOK = (prcScr->bottom > prcScr->top);
	}

	return lbRectOK;
}

DWORD CRealBuffer::GetConsoleCP()
{
	return con.m_dwConsoleCP;
}

DWORD CRealBuffer::GetConsoleOutputCP()
{
	return con.m_dwConsoleOutputCP;
}

DWORD CRealBuffer::GetConsoleMode()
{
	return con.m_dwConsoleMode;
}

bool CRealBuffer::FindRangeStart(COORD& crFrom/*[In/Out]*/, COORD& crTo/*[In/Out]*/, bool& bUrlMode, LPCWSTR pszBreak, LPCWSTR pszUrlDelim, LPCWSTR pszSpacing, LPCWSTR pszUrl, LPCWSTR pszProtocol, LPCWSTR pChar, int nLen)
{
	bool lbRc = false;

	WARNING("Тут пока работаем в экранных координатах");
	_ASSERTE(crFrom.Y>=0 && crFrom.Y<GetTextHeight());
	_ASSERTE(crTo.Y>=0 && crTo.Y<GetTextHeight());

	// Курсор над комментарием?
	// Попробуем найти начало имени файла
	// 131026 Allows '?', otherwise links like http://go.com/fwlink/?LinkID=1 may fails
	while ((crFrom.X) > 0 && (pChar[crFrom.X-1]==L'?' || !wcschr(bUrlMode ? pszUrlDelim : pszBreak, pChar[crFrom.X-1])))
	{
		if (!bUrlMode && pChar[crFrom.X] == L'/')
		{
			if ((crFrom.X >= 2) && ((crFrom.X + 1) < nLen)
				&& ((pChar[crFrom.X+1] == L'/') && (pChar[crFrom.X-1] == L':')
					&& wcschr(pszUrl, pChar[crFrom.X-2]))) // как минимум одна буква на протокол
			{
				crFrom.X++;
			}

			if ((crFrom.X >= 3)
				&& ((pChar[crFrom.X-1] == L'/') // как минимум одна буква на протокол
					&& (((pChar[crFrom.X-2] == L':') && wcschr(pszUrl, pChar[crFrom.X-3])) // http://www.ya.ru
						|| ((crFrom.X >= 4) && (pChar[crFrom.X-2] == L'/') && (pChar[crFrom.X-3] == L':') && wcschr(pszUrl, pChar[crFrom.X-4])) // file:///c:\file.html
					))
				)
			{
				bUrlMode = true;
				crTo.X = crFrom.X-2;
				crFrom.X -= 3;
				while ((crFrom.X > 0) && wcschr(pszProtocol, pChar[crFrom.X-1]))
					crFrom.X--;
				break;
			}
			else if ((pChar[crFrom.X] == L'/') && (crFrom.X >= 1) && (pChar[crFrom.X-1] == L'/'))
			{	
				crFrom.X++;
				break; // Комментарий в строке?
			}
		}
		crFrom.X--;
		if (pChar[crFrom.X] == L':')
		{
			if (pChar[crFrom.X+1] == L' ')
			{
				// ASM - подсвечивать нужно "test.asasm(1,1)"
				// object.Exception@assembler.d(1239): test.asasm(1,1):
				crFrom.X += 2;
				break;
			}
			else if (pChar[crFrom.X+1] != L'\\' && pChar[crFrom.X+1] != L'/')
			{
				goto wrap; // Не оно
			}
		}
	}
	while (((crFrom.X+1) < nLen) && wcschr(pszSpacing, pChar[crFrom.X]))
		crFrom.X++;
	if (crFrom.X > crTo.X)
	{
		goto wrap; // Fail?
	}

	lbRc = true;
wrap:
	return lbRc;
}

bool CRealBuffer::CheckValidUrl(COORD& crFrom/*[In/Out]*/, COORD& crTo/*[In/Out]*/, bool& bUrlMode, LPCWSTR pszUrlDelim, LPCWSTR pszUrl, LPCWSTR pszProtocol, LPCWSTR pChar, int nLen)
{
	bool lbRc = false;

	WARNING("Тут пока работаем в экранных координатах");
	_ASSERTE(crFrom.Y>=0 && crFrom.Y<GetTextHeight());
	_ASSERTE(crTo.Y>=0 && crTo.Y<GetTextHeight());

	// URL? (Курсор мог стоять над протоколом)
	while ((crTo.X < nLen) && wcschr(pszProtocol, pChar[crTo.X]))
		crTo.X++;
	if (((crTo.X+1) < nLen) && (pChar[crTo.X] == L':'))
	{
		if (((crTo.X+4) < nLen) && (pChar[crTo.X+1] == L'/') && (pChar[crTo.X+2] == L'/'))
		{
			bUrlMode = true;
			if (wcschr(pszUrl+2 /*пропустить ":/"*/, pChar[crTo.X+3])
				|| ((((crTo.X+5) < nLen) && (pChar[crTo.X+3] == L'/'))
					&& wcschr(pszUrl+2 /*пропустить ":/"*/, pChar[crTo.X+4]))
				)
			{
				crFrom = crTo;
				while ((crFrom.X > 0) && wcschr(pszProtocol, pChar[crFrom.X-1]))
					crFrom.X--;
			}
			else
			{
				goto wrap; // Fail
			}
		}
	}

	lbRc = true;
wrap:
	return lbRc;
}

ExpandTextRangeType CRealBuffer::ExpandTextRange(COORD& crFrom/*[In/Out]*/, COORD& crTo/*[Out]*/, ExpandTextRangeType etr, wchar_t* pszText /*= NULL*/, size_t cchTextMax /*= 0*/)
{
	ExpandTextRangeType result = etr_None;

	WARNING("Тут пока работаем в экранных координатах, а приходят - абсолютные буферные");
	_ASSERTE(crFrom.Y>=con.m_sbi.srWindow.Top && crFrom.Y<GetBufferHeight());

	COORD lcrFrom = BufferToScreen(crFrom);
	COORD lcrTo = lcrFrom;

	//COORD crMouse = lcrFrom; // Save

	// Нужно получить строку
	MSectionLock csData; csData.Lock(&csCON);
	wchar_t* pChar = NULL;
	int nLen = 0;

	if (mp_RCon->mp_VCon && GetConsoleLine(lcrFrom.Y, &pChar, /*NULL,*/ &nLen, &csData) && pChar)
	{
		TODO("Проверить на ошибки после добавления горизонтальной прокрутки");
		if (etr == etr_Word)
		{
			while ((lcrFrom.X) > 0 && !(mp_RCon->mp_VCon->isCharSpace(pChar[lcrFrom.X-1]) || mp_RCon->mp_VCon->isCharNonSpacing(pChar[lcrFrom.X-1])))
				lcrFrom.X--;
			while (((lcrTo.X+1) < nLen) && !(mp_RCon->mp_VCon->isCharSpace(pChar[lcrTo.X]) || mp_RCon->mp_VCon->isCharNonSpacing(pChar[lcrTo.X])))
				lcrTo.X++;
			result = etr; // OK
		}
		else if (etr == etr_FileAndLine)
		{
			// В именах файлов недопустимы: "/\:|*?<>~t~r~n
			const wchar_t  pszBreak[] = {
								/*недопустимые в FS*/
								L'\"', '|', '*', '?', '<', '>', '\t', '\r', '\n', 
								/*для простоты - учитываем и рамки*/
								ucArrowUp, ucArrowDown, ucDnScroll, ucUpScroll,
								ucBox100, ucBox75, ucBox50, ucBox25,
								ucBoxDblVert, ucBoxSinglVert, ucBoxDblVertSinglRight, ucBoxDblVertSinglLeft,
								ucBoxDblDownRight, ucBoxDblDownLeft, ucBoxDblUpRight,
								ucBoxDblUpLeft, ucBoxSinglDownRight, ucBoxSinglDownLeft, ucBoxSinglUpRight,
								ucBoxSinglUpLeft, ucBoxSinglDownDblHorz, ucBoxSinglUpDblHorz, ucBoxDblDownDblHorz,
								ucBoxDblUpDblHorz, ucBoxSinglDownHorz, ucBoxSinglUpHorz, ucBoxDblDownSinglHorz,
								ucBoxDblUpSinglHorz, ucBoxDblVertRight, ucBoxDblVertLeft, 
								ucBoxSinglVertRight, ucBoxSinglVertLeft, ucBoxDblVertHorz,
								0};
			const wchar_t* pszSpacing = L" \t\xB7\x2192"; //B7 - режим "Show white spaces", 2192 - символ табуляции там же
			const wchar_t* pszSeparat = L" \t:(";
			const wchar_t* pszTermint = L":)],";
			const wchar_t* pszDigits  = L"0123456789";
			const wchar_t* pszSlashes = L"/\\";
			const wchar_t* pszUrl = L":/\\:%#ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz;?@&=+$,-_.!~*'()0123456789";
			const wchar_t* pszUrlTrimRight = L".,;";
			const wchar_t* pszProtocol = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.";
			const wchar_t* pszEMail = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-.";
			const wchar_t* pszUrlDelim = L"\\\"<>{}[]^`' \t\r\n";
			const wchar_t* pszUrlFileDelim = L"\"<>^ \t\r\n";
			int nColons = 0;
			bool bUrlMode = false, bMaybeMail = false;
			SHORT MailX = -1;

			// Курсор над комментарием?
			// Попробуем найти начало имени файла
			if (!FindRangeStart(lcrFrom, lcrTo, bUrlMode, pszBreak, pszUrlDelim, pszSpacing, pszUrl, pszProtocol, pChar, nLen))
				goto wrap;

			// URL? (Курсор мог стоять над протоколом)
			if (!CheckValidUrl(lcrFrom, lcrTo, bUrlMode, pszUrlDelim, pszUrl, pszProtocol, pChar, nLen))
				goto wrap;


			// Чтобы корректно флаги обработались (типа наличие расширения и т.п.)
			lcrTo.X = lcrFrom.X;

			// Теперь - найти конец.
			// Считаем, что для файлов конец это двоеточие, после которого идет описание ошибки
			// Для протоколов (http/...) - первый недопустимый символ

			TODO("Можно бы и просто открытие файлов прикрутить, без требования 'строки с ошибкой'");

			// -- VC
			// 1>t:\vcproject\conemu\realconsole.cpp(8104) : error C2065: 'qqq' : undeclared identifier
			// DefResolve.cpp(18) : error C2065: 'sdgagasdhsahd' : undeclared identifier
			// DefResolve.cpp(18): warning: note xxx
			// -- GCC
			// ConEmuC.cpp:49: error: 'qqq' does not name a type
			// 1.c:3: some message
			// file.cpp:29:29: error
			// CPP Check
			// [common\PipeServer.h:1145]: (style) C-style pointer casting
			// Delphi
			// T:\VCProject\FarPlugin\$FarPlugins\MaxRusov\far-plugins-read-only\FarLib\FarCtrl.pas(1002) Error: Undeclared identifier: 'PCTL_GETPLUGININFO'
			// FPC
			// FarCtrl.pas(1002,49) Error: Identifier not found "PCTL_GETPLUGININFO"
			// -- Possible?
			// abc.py (3): some message
			// ASM - подсвечивать нужно "test.asasm(1,1)"
			// object.Exception@assembler.d(1239): test.asasm(1,1):
			// -- URL's
			// file://c:\temp\qqq.html
			// http://www.farmanager.com
			// $ http://www.KKK.ru - левее слеша - не срабатывает
			// C:\ConEmu>http://www.KKK.ru - ...
			// -- False detects
			// 29.11.2011 18:31:47
			// C:\VC\unicode_far\macro.cpp  1251 Ln 5951/8291 Col 51 Ch 39 0043h 13:54
			// InfoW1900->SettingsControl(sc.Handle, SCTL_FREE, 0, 0);

			bool bDigits = false, bLineNumberFound = false, bWasSeparator = false;
			// Нас на интересуют строки типа "11.05.2010 10:20:35"
			// В имени файла должна быть хотя бы одна буква (расширение), причем английская
			int iExtFound = 0, iBracket = 0;
			// Поехали
			if (bUrlMode)
			{
				LPCWSTR pszDelim = (wcsncmp(pChar+lcrFrom.X, L"file://", 7) == 0) ? pszUrlFileDelim : pszUrlDelim;
				while (((lcrTo.X+1) < nLen) && !wcschr(pszDelim, pChar[lcrTo.X+1]))
					lcrTo.X++;
			}
			else while ((lcrTo.X+1) < nLen)
			{
				if ((pChar[lcrTo.X] == L'/') && ((lcrTo.X+1) < nLen) && (pChar[lcrTo.X+1] == L'/')
					&& !((lcrTo.X > 1) && (pChar[lcrTo.X] == L':'))) // и НЕ URL адрес
				{
					goto wrap; // Не оно (комментарий в строке)
				}

				if (bWasSeparator 
					&& ((pChar[lcrTo.X] >= L'0' && pChar[lcrTo.X] <= L'9')
						|| (bDigits && (pChar[lcrTo.X] == L',')))) // FarCtrl.pas(1002,49) Error: 
				{
					if (bLineNumberFound)
					{
						// gcc такие строки тоже может выкинуть
						// file.cpp:29:29: error
						lcrTo.X--;
						break;
					}
					if (!bDigits && (lcrFrom.X < lcrTo.X) /*&& (pChar[lcrTo.X-1] == L':')*/)
					{
						bDigits = true;
					}
				}
				else
				{
					if (iExtFound != 2)
					{
						if (!iExtFound)
						{
							if (pChar[lcrTo.X] == L'.')
								iExtFound = 1;
						}
						else
						{
							// Не особо заморачиваясь с точками и прочим. Просто небольшая страховка от ложных срабатываний...
							if ((pChar[lcrTo.X] >= L'a' && pChar[lcrTo.X] <= L'z') || (pChar[lcrTo.X] >= L'A' && pChar[lcrTo.X] <= L'Z'))
							{
								iExtFound = 2;
								iBracket = 0;
							}
						}
					}

					if (iExtFound == 2)
					{
						if (pChar[lcrTo.X] == L'.')
						{
							iExtFound = 1;
							iBracket = 0;
						}
						else if (wcschr(pszSlashes, pChar[lcrTo.X]) != NULL)
						{
							// Был слеш, значит расширения - еще нет
							iExtFound = 0;
							iBracket = 0;
							bWasSeparator = false;
						}
						else if (wcschr(pszSpacing, pChar[lcrTo.X]) && wcschr(pszSpacing, pChar[lcrTo.X+1]))
						{
							// Слишком много пробелов
							iExtFound = 0;
							iBracket = 0;
							bWasSeparator = false;
						}
						else
							bWasSeparator = (wcschr(pszSeparat, pChar[lcrTo.X]) != NULL);
					}

					// Расчитано на закрывающие : или ) или ] или ,
					_ASSERTE(pszTermint[0]==L':' && pszTermint[1]==L')' && pszTermint[2]==L']' && pszTermint[3]==L',' && pszTermint[5]==0);
					if (bDigits && wcschr(pszTermint, pChar[lcrTo.X]) /*pChar[lcrTo.X] == L':'*/)
					{
						// Validation
						if (((pChar[lcrTo.X] == L':')
								&& (wcschr(pszSpacing, pChar[lcrTo.X+1])
									|| wcschr(pszDigits, pChar[lcrTo.X+1])))
						// Если номер строки обрамлен скобками - скобки должны быть сбалансированы
						|| ((pChar[lcrTo.X] == L')') && (iBracket == 1)
								&& ((pChar[lcrTo.X+1] == L':')
									|| wcschr(pszSpacing, pChar[lcrTo.X+1])
									|| wcschr(pszDigits, pChar[lcrTo.X+1])))
						// [file.cpp:1234]: (cppcheck)
						|| ((pChar[lcrTo.X] == L']') && (pChar[lcrTo.X+1] == L':'))
							)
						{
							_ASSERTE(bLineNumberFound==false);
							bLineNumberFound = true;
							break; // found?
						}
					}
					bDigits = false;

					switch (pChar[lcrTo.X])
					{
					// Пока регулярок нет...
					case L'(': iBracket++; break;
					case L')': iBracket--; break;
					case L'/': case L'\\': iBracket = 0; break;
					case L'@':
						if (MailX != -1)
						{
							bMaybeMail = false;
						}
						else if (((lcrTo.X > 0) && wcschr(pszEMail, pChar[lcrTo.X-1]))
							&& (((lcrTo.X+1) < nLen) && wcschr(pszEMail, pChar[lcrTo.X+1])))
						{
							bMaybeMail = true;
							MailX = lcrTo.X;
						}
						break;
					}

					if (pChar[lcrTo.X] == L':')
						nColons++;
					else if (pChar[lcrTo.X] == L'\\' || pChar[lcrTo.X] == L'/')
						nColons = 0;
				}

				if (nColons >= 2)
					break;

				lcrTo.X++;
				if (wcschr(bUrlMode ? pszUrlDelim : pszBreak, pChar[lcrTo.X]))
				{
					if (bMaybeMail)
						break;
					goto wrap; // Не оно
				}
			} // end of 'while ((lcrTo.X+1) < nLen)'

			if (bUrlMode)
			{
				// Считаем, что OK
				bMaybeMail = false;
			}
			else
			{
				if (!bLineNumberFound && bDigits)
					bLineNumberFound = true;

				if (bLineNumberFound)
					bMaybeMail = false;

				if ((pChar[lcrTo.X] != L':'
						&& pChar[lcrTo.X] != L' '
						&& !((pChar[lcrTo.X] == L')') && iBracket == 1)
						&& !((pChar[lcrTo.X] == L']') && (pChar[lcrTo.X+1] == L':'))
					)
					|| !bLineNumberFound || (nColons > 2))
				{
					if (!bMaybeMail)
						goto wrap;
				}
				if (bMaybeMail || (!bMaybeMail && pChar[lcrTo.X] != L')'))
					lcrTo.X--;
				// Откатить ненужные пробелы
				while ((lcrFrom.X < lcrTo.X) && wcschr(pszSpacing, pChar[lcrFrom.X]))
					lcrFrom.X++;
				while ((lcrTo.X > lcrFrom.X) && wcschr(pszSpacing, pChar[lcrTo.X]))
					lcrTo.X--;
				if ((lcrFrom.X + 4) > lcrTo.X) // 1.c:1: //-V112
				{
					// Слишком коротко, считаем что не оно
					goto wrap;
				}
				if (!bMaybeMail)
				{
					// Проверить, чтобы был в наличии номер строки
					if (!(pChar[lcrTo.X] >= L'0' && pChar[lcrTo.X] <= L'9') // ConEmuC.cpp:49:
						&& !(pChar[lcrTo.X] == L')' && (pChar[lcrTo.X-1] >= L'0' && pChar[lcrTo.X-1] <= L'9'))) // ConEmuC.cpp(49) :
					{
						goto wrap; // Номера строки нет
					}
					// [file.cpp:1234]: (cppcheck) ?
					if ((pChar[lcrTo.X+1] == L']') && (pChar[lcrFrom.X] == L'['))
						lcrFrom.X++;
					// Чтобы даты ошибочно не подсвечивать:
					// 29.11.2011 18:31:47
					{
						bool bNoDigits = false;
						for (int i = lcrFrom.X; i <= lcrTo.X; i++)
						{
							if (pChar[i] < L'0' || pChar[i] > L'9')
							{
								bNoDigits = true;
							}
						}
						if (!bNoDigits)
							goto wrap;
					}
					// -- уже включены // Для красивости в VC включить скобки
					//if ((pChar[lcrTo.X] == L')') && (pChar[lcrTo.X+1] == L':'))
					//	lcrTo.X++;
				}
				else // bMaybeMail
				{
					// Для мейлов - проверяем допустимые символы (чтобы пробелов не было и прочего мусора)
					int x = MailX - 1; _ASSERTE(x>=0);
					while ((x > 0) && wcschr(pszEMail, pChar[x-1]))
						x--;
					lcrFrom.X = x;
					x = MailX + 1; _ASSERTE(x<nLen);
					while (((x+1) < nLen) && wcschr(pszEMail, pChar[x+1]))
						x++;
					lcrTo.X = x;
				}
			} // end "else / if (bUrlMode)"

			// Check mouse pos, it must be inside region
			if ((crFrom.X < lcrFrom.X) || (crFrom.X > lcrTo.X))
			{
				goto wrap;
			}

			// Ok
			if (pszText && cchTextMax)
			{
				_ASSERTE(!bMaybeMail || !bUrlMode); // Одновременно - флаги не могут быть выставлены!
				int iMailTo = (bMaybeMail && !bUrlMode) ? lstrlen(L"mailto:") : 0;
				if ((lcrTo.X - lcrFrom.X + 1 + iMailTo) >= (INT_PTR)cchTextMax)
					goto wrap; // Недостаточно места под текст
				if (iMailTo)
				{
					// Добавить префикс протокола
					lstrcpyn(pszText, L"mailto:", iMailTo+1);
					pszText += iMailTo;
					cchTextMax -= iMailTo;
					bUrlMode = true;
				}
				if (bUrlMode)
				{
					while ((lcrTo.X > lcrFrom.X) && wcschr(pszUrlTrimRight, pChar[lcrTo.X]))
						lcrTo.X--;
				}
				// Return hyperlink target
				memmove(pszText, pChar+lcrFrom.X, (lcrTo.X - lcrFrom.X + 1)*sizeof(*pszText));
				pszText[lcrTo.X - lcrFrom.X + 1] = 0;

				#ifdef _DEBUG
				if (!bUrlMode && wcsstr(pszText, L"//")!=NULL)
				{
					_ASSERTE(FALSE);
				}
				#endif
			}
			result = bUrlMode ? etr_Url : etr;
		}
	}
wrap:
	// Fail?
	if (result == etr_None)
	{
		// Reset coordinates
		crFrom = MakeCoord(0,0);
		crTo = MakeCoord(0,0);
	}
	else
	{
		crFrom = ScreenToBuffer(lcrFrom);
		crTo = ScreenToBuffer(lcrTo);
	}
	StoreLastTextRange(result);
	return result;
}

void CRealBuffer::StoreLastTextRange(ExpandTextRangeType etr)
{
	if (con.etr.etrLast != etr)
	{
		con.etr.etrLast = etr;
		//if (etr == etr_None)
		//{
		//	con.etr.mcr_FileLineStart = con.etr.mcr_FileLineEnd = MakeCoord(0,0);
		//}

		if ((mp_RCon->mp_ABuf == this) && mp_RCon->isVisible())
			gpConEmu->OnSetCursor();
	}
}

BOOL CRealBuffer::GetPanelRect(BOOL abRight, RECT* prc, BOOL abFull /*= FALSE*/, BOOL abIncludeEdges /*= FALSE*/)
{
	if (!this)
	{
		if (prc)
			*prc = MakeRect(-1,-1);

		return FALSE;
	}

	if (abRight)
	{
		if (prc)
			*prc = abFull ? mr_RightPanelFull : mr_RightPanel;

		if (mr_RightPanel.right <= mr_RightPanel.left)
			return FALSE;
	}
	else
	{
		if (prc)
			*prc = abFull ? mr_LeftPanelFull : mr_LeftPanel;

		if (mr_LeftPanel.right <= mr_LeftPanel.left)
			return FALSE;
	}
	
	if (prc && !abFull && abIncludeEdges)
	{
		prc->left  = abRight ? mr_RightPanelFull.left  : mr_LeftPanelFull.left;
		prc->right = abRight ? mr_RightPanelFull.right : mr_LeftPanelFull.right;
	}

	return TRUE;
}

BOOL CRealBuffer::isLeftPanel()
{
	return mb_LeftPanel;
}

BOOL CRealBuffer::isRightPanel()
{
	return mb_RightPanel;
}

short CRealBuffer::CheckProgressInConsole(const wchar_t* pszCurLine)
{
	// Обработка прогресса NeroCMD и пр. консольных программ (если курсор находится в видимой области)
	//Плагин Update
	//"Downloading Far                                               99%"
	//NeroCMD
	//"012% ########.................................................................."
	//ChkDsk
	//"Completed: 25%"
	//Rar
	// ...       Vista x86\Vista x86.7z         6%
	//aria2c
	//[#1 SIZE:0B/9.1MiB(0%) CN:1 SPD:1.2KiBs ETA:2h1m11s]
	int nIdx = 0;
	bool bAllowDot = false;
	short nProgress = -1;

	const wchar_t *szPercentEng = L" percent";
	const wchar_t *szComplEng  = L"Completed:";
	static wchar_t szPercentRus[16] = {}, szComplRus[16] = {};
	static int nPercentEngLen = lstrlen(szPercentEng), nComplEngLen = lstrlen(szComplEng);
	static int nPercentRusLen, nComplRusLen;
	
	if (szPercentRus[0] == 0)
	{
		szPercentRus[0] = L' ';
		TODO("Хорошо бы и другие национальные названия обрабатывать, брать из настройки");
		lstrcpy(szPercentRus,L"процент");
		lstrcpy(szComplRus,L"Завершено:");
		
		nPercentRusLen = lstrlen(szPercentRus);
		nComplRusLen = lstrlen(szComplEng);
	}

	// Сначала проверим, может цифры идут в начале строки (лидирующие пробелы)?
	if (pszCurLine[nIdx] == L' ' && isDigit(pszCurLine[nIdx+1]))
		nIdx++; // один лидирующий пробел перед цифрой
	else if (pszCurLine[nIdx] == L' ' && pszCurLine[nIdx+1] == L' ' && isDigit(pszCurLine[nIdx+2]))
		nIdx += 2; // два лидирующих пробела перед цифрой
	else if (!isDigit(pszCurLine[nIdx]))
	{
		// Строка начинается НЕ с цифры. Может начинается одним из известных префиксов (ChkDsk)?

		if (!wcsncmp(pszCurLine, szComplRus, nComplRusLen))
		{
			nIdx += nComplRusLen;

			if (pszCurLine[nIdx] == L' ') nIdx++;

			if (pszCurLine[nIdx] == L' ') nIdx++;

			bAllowDot = true;
		}
		else if (!wcsncmp(pszCurLine, szComplEng, nComplEngLen))
		{
			nIdx += nComplEngLen;

			if (pszCurLine[nIdx] == L' ') nIdx++;

			if (pszCurLine[nIdx] == L' ') nIdx++;

			bAllowDot = true;
		}
		else if (!wcsncmp(pszCurLine, L"[#", 2))
		{
			const wchar_t* p = wcsstr(pszCurLine, L"%) ");
			while ((p > pszCurLine) && (p[-1] != L'('))
				p--;
			if (p > pszCurLine)
				nIdx = p - pszCurLine;
		}

		// Известных префиксов не найдено, проверяем, может процент есть в конце строки?
		if (!nIdx)
		{
			//TODO("Не работает с одной цифрой");
			// Creating archive T:\From_Work\VMWare\VMWare.part006.rar
			// ...       Vista x86\Vista x86.7z         6%
			int i = GetTextWidth() - 1;

			// Откусить trailing spaces
			while(i>3 && pszCurLine[i] == L' ')
				i--;

			// Теперь, если дошли до '%' и перед ним - цифра
			if (i >= 3 && pszCurLine[i] == L'%' && isDigit(pszCurLine[i-1]))
			{
				//i -= 2;
				i--;

				int j = i, k = -1;
				while (j > 0 && isDigit(pszCurLine[j-1]))
					j--;

				// Может быть что-то типа "Progress 25.15%"
				if (((i - j) <= 2) && (j >= 2) && ((i - j) <= 2) && (pszCurLine[j-1] == L'.'))
				{
					k = j - 1;
					while (k > 0 && isDigit(pszCurLine[k-1]))
						k--;
				}

				if (k >= 0)
				{
					if (((j - k) <= 3) // 2 цифры + точка
						|| (((j - k) <= 4) && (pszCurLine[k] == L'1'))) // "100.0%"
					{
						nIdx = i = k;
						bAllowDot = true;
					}
				}
				else
				{
					if (((j - i) <= 2) // 2 цифры + точка
						|| (((j - i) <= 3) && (pszCurLine[j] == L'1'))) // "100%"
					{
						nIdx = i = j;
					}
				}

				#if 0
				// Две цифры перед '%'?
				if (isDigit(pszCurLine[i-1]))
					i--;

				// Три цифры допускается только для '100%'
				if (pszCurLine[i-1] == L'1' && !isDigit(pszCurLine[i-2]))
				{
					nIdx = i - 1;
				}
				// final check
				else if (!isDigit(pszCurLine[i-1]))
				{
					nIdx = i;
				}
				#endif

				// Может ошибочно детектировать прогресс, если его ввести в prompt
				// Допустим, что если в строке есть символ '>' - то это не прогресс
				while (i>=0)
				{
					if (pszCurLine[i] == L'>')
					{
						nIdx = 0;
						break;
					}

					i--;
				}
			}
		}
	}

	// Менять nProgress только если нашли проценты в строке с курсором
	if (isDigit(pszCurLine[nIdx]))
	{
		if (isDigit(pszCurLine[nIdx+1]) && isDigit(pszCurLine[nIdx+2])
			&& (pszCurLine[nIdx+3]==L'%' || (bAllowDot && pszCurLine[nIdx+3]==L'.')
			|| !wcsncmp(pszCurLine+nIdx+3,szPercentEng,nPercentEngLen)
			|| !wcsncmp(pszCurLine+nIdx+3,szPercentRus,nPercentRusLen)))
		{
			nProgress = 100*(pszCurLine[nIdx] - L'0') + 10*(pszCurLine[nIdx+1] - L'0') + (pszCurLine[nIdx+2] - L'0');
		}
		else if (isDigit(pszCurLine[nIdx+1])
			&& (pszCurLine[nIdx+2]==L'%' || (bAllowDot && pszCurLine[nIdx+2]==L'.')
			|| !wcsncmp(pszCurLine+nIdx+2,szPercentEng,nPercentEngLen)
			|| !wcsncmp(pszCurLine+nIdx+2,szPercentRus,nPercentRusLen)))
		{
			nProgress = 10*(pszCurLine[nIdx] - L'0') + (pszCurLine[nIdx+1] - L'0');
		}
		else if (pszCurLine[nIdx+1]==L'%' || (bAllowDot && pszCurLine[nIdx+1]==L'.')
			|| !wcsncmp(pszCurLine+nIdx+1,szPercentEng,nPercentEngLen)
			|| !wcsncmp(pszCurLine+nIdx+1,szPercentRus,nPercentRusLen))
		{
			nProgress = (pszCurLine[nIdx] - L'0');
		}
	}

	if (nProgress != -1)
	{
		mp_RCon->setLastConsoleProgress(nProgress, true); // его обновляем всегда
	}
	else
	{
		DWORD nDelta = GetTickCount() - mp_RCon->m_Progress.LastConProgrTick;
		if (nDelta < CONSOLEPROGRESSTIMEOUT) // Если таймаут предыдущего значения еще не наступил
			nProgress = mp_RCon->m_Progress.ConsoleProgress; // возъмем предыдущее значение
		mp_RCon->setLastConsoleProgress(-1, false); // его обновляем всегда
	}

	return nProgress;
}

LRESULT CRealBuffer::OnScroll(int nDirection, short nTrackPos /*= -1*/, UINT nCount /*= 1*/)
{
	if (!this) return 0;

	int nVisible = GetTextHeight();

	if (!nCount)
	{
		_ASSERTE(nCount >= 1);
		nCount = 1;
	}
	// returns "-1" when user choose "one screen per time"
	else if ((UINT)nCount >= (UINT)nVisible)
	{
		nCount = max(1,(nVisible-1));
	}

	// SB_LINEDOWN / SB_LINEUP / SB_PAGEDOWN / SB_PAGEUP
	if (m_Type == rbt_Primary)
	{
		if ((nDirection != SB_THUMBTRACK) && (nDirection != SB_THUMBPOSITION))
			nTrackPos = 0;
		if (nTrackPos < 0)
			nTrackPos = con.m_sbi.srWindow.Top;

		WPARAM wParm = MAKELONG(nDirection,nTrackPos);
		while (true)
		{
			WARNING("Переделать в команду пайпа");
			mp_RCon->PostConsoleMessage(mp_RCon->hConWnd, WM_VSCROLL, wParm, 0);

			if ((nCount <= 1) || (nDirection != SB_LINEUP && nDirection != SB_LINEDOWN) /*|| mp_RCon->isFar()*/)
				break;
			nCount--;
		}

		if ((nDirection == SB_THUMBTRACK) || (nDirection == SB_THUMBPOSITION))
		{
			_ASSERTE(nTrackPos>=0);
			int nVisible = GetTextHeight();

			if (nTrackPos < 0)
				nTrackPos = 0;
			else if ((nTrackPos + nVisible) >= con.m_sbi.dwSize.Y)
				nTrackPos = con.m_sbi.dwSize.Y - nVisible;

			// Обновим Top, иначе курсор отрисовывается в неправильном месте
			con.m_sbi.srWindow.Top = nTrackPos;
			con.m_sbi.srWindow.Bottom = nTrackPos + nVisible - 1;
			con.nTopVisibleLine = nTrackPos;

			////mp_RCon->mp_VCon->Invalidate();
			//mp_RCon->mb_DataChanged = TRUE; // Переменная используется внутри класса
			//con.bConsoleDataChanged = TRUE; // А эта - при вызовах из CVirtualConsole
		}
	}
	else
	{
		SHORT nNewTop = con.m_sbi.srWindow.Top;

		switch (nDirection)
		{
		case SB_LINEDOWN:
			if ((nNewTop + nVisible) < con.m_sbi.dwSize.Y)
				nNewTop = min((nNewTop+(SHORT)nCount),(con.m_sbi.dwSize.Y-nVisible));
			break;
		case SB_LINEUP:
			if (nNewTop > 0)
				nNewTop = max(0,(nNewTop-(SHORT)nCount));
			break;
		case SB_PAGEDOWN:
			nNewTop = min((con.m_sbi.dwSize.Y - nVisible),(con.m_sbi.srWindow.Top + nVisible - 1));
			break;
		case SB_PAGEUP:
			nNewTop = max(0, (con.m_sbi.srWindow.Top - nVisible + 1));
			break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			{
				_ASSERTE(nTrackPos>=0);

				if (nTrackPos < 0)
					nTrackPos = 0;
				else if ((nTrackPos + nVisible) >= con.m_sbi.dwSize.Y)
					nTrackPos = con.m_sbi.dwSize.Y - nVisible;

				nNewTop = nTrackPos;
			}
			break;
		case SB_ENDSCROLL:
			break;
		default:
			// Недопустимый код
			_ASSERTE(nDirection==SB_LINEUP);
		}


		if (nNewTop != con.m_sbi.srWindow.Top)
		{
			con.m_sbi.srWindow.Top = nNewTop;
			con.m_sbi.srWindow.Bottom = nNewTop + nVisible - 1;

			con.nTopVisibleLine = con.m_sbi.srWindow.Top;

			//mp_RCon->mp_VCon->Invalidate();
			mp_RCon->mb_DataChanged = TRUE; // Переменная используется внутри класса
			con.bConsoleDataChanged = TRUE; // А эта - при вызовах из CVirtualConsole

			//if (mp_RCon->isActive()) -- mp_RCon->isActive() проверит сама UpdateScrollInfo, а скроллбар может быть и в видимой но НЕ активной консоли
			//mp_RCon->UpdateScrollInfo();
		}
	}
	return 0;
}

LRESULT CRealBuffer::OnSetScrollPos(WPARAM wParam)
{
	if (!this) return 0;

	// SB_LINEDOWN / SB_LINEUP / SB_PAGEDOWN / SB_PAGEUP
	OnScroll(LOWORD(wParam),HIWORD(wParam));
	return 0;
}

const CRgnDetect* CRealBuffer::GetDetector()
{
	if (this)
		return &m_Rgn;
	return NULL;
}
