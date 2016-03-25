
/*
Copyright (c) 2015 Maximus5
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

#ifdef _DEBUG
	#define DebugString(x) //OutputDebugString(x)
	#define DebugStringConSize(x) //OutputDebugString(x)
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
	#define BsDelWordMsg(s) //MessageBox(NULL, s, L"OnPromptBsDeleteWord called", MB_SYSTEMMODAL);
#else
	#define DebugString(x) //OutputDebugString(x)
	#define DebugStringConSize(x) //OutputDebugString(x)
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
	#define BsDelWordMsg(s) //MessageBox(NULL, s, L"OnPromptBsDeleteWord called", MB_SYSTEMMODAL);
#endif

#include "../common/Common.h"
#include "../common/ConsoleRead.h"
#include "../common/MRect.h"
#include "../common/UnicodeChars.h"
#include "../common/WConsole.h"

#include "hlpConsole.h"
#include "hlpProcess.h"

/* **************** */

BOOL MyGetConsoleFontSize(COORD& crFontSize)
{
	BOOL lbRc = FALSE;

	if (ghConEmuWnd)
	{
		_ASSERTE(gnGuiPID!=0);
		ConEmuGuiMapping* inf = (ConEmuGuiMapping*)malloc(sizeof(ConEmuGuiMapping));
		if (inf)
		{
			if (LoadGuiMapping(gnGuiPID, *inf))
			{
				crFontSize.Y = (SHORT)inf->MainFont.nFontHeight;
				crFontSize.X = (SHORT)inf->MainFont.nFontCellWidth;
				lbRc = TRUE;
			}
			free(inf);
		}

		_ASSERTEX(lbRc && (crFontSize.X > 3) && (crFontSize.Y > 4));
	}

	return lbRc;
}

BOOL IsVisibleRectLocked(COORD& crLocked)
{
	CESERVER_CONSOLE_MAPPING_HDR SrvMap;
	if (LoadSrvMapping(ghConWnd, SrvMap))
	{
		if (SrvMap.bLockVisibleArea && (SrvMap.crLockedVisible.X > 0) && (SrvMap.crLockedVisible.Y > 0))
		{
			crLocked = SrvMap.crLockedVisible;
			return TRUE;
		}
	}
	return FALSE;
}


// Due to Microsoft bug we need to lock Server reading thread to avoid crash of kernel
// http://conemu.github.io/en/MicrosoftBugs.html#Exception_in_ReadConsoleOutput
void LockServerReadingThread(bool bLock, COORD dwSize, CESERVER_REQ*& pIn, CESERVER_REQ*& pOut)
{
	DWORD nServerPID = gnServerPID;
	if (!nServerPID)
		return;

	DWORD nErrSave = GetLastError();

	if (bLock)
	{
		HANDLE hOurThreadHandle = NULL;

		// We need to give our thread handle (to server process) to avoid
		// locking of server reading thread (in case of our thread fails)
		DWORD dwErr = -1;
		HANDLE hServer = OpenProcess(PROCESS_DUP_HANDLE, FALSE, nServerPID);
		if (!hServer)
		{
			dwErr = GetLastError();
			_ASSERTEX(hServer!=NULL && "Open server handle fails, Can't dup handle!");
		}
		else
		{
			if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), hServer, &hOurThreadHandle,
					SYNCHRONIZE|THREAD_QUERY_INFORMATION, FALSE, 0))
			{
				dwErr = GetLastError();
				_ASSERTEX(hServer!=NULL && "DuplicateHandle fails, Can't dup handle!");
				hOurThreadHandle = NULL;
			}
			CloseHandle(hServer);
		}

		pIn = ExecuteNewCmd(CECMD_SETCONSCRBUF, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETCONSCRBUF));
		if (pIn)
		{
			pIn->SetConScrBuf.bLock = TRUE;
			pIn->SetConScrBuf.dwSize = dwSize; // Informational
			pIn->SetConScrBuf.hRequestor = hOurThreadHandle;
		}
	}
	else
	{
		if (pIn)
		{
			pIn->SetConScrBuf.bLock = FALSE;
		}
	}

	if (pIn)
	{
		ExecuteFreeResult(pOut);
		pOut = ExecuteSrvCmd(nServerPID, pIn, ghConWnd);
		if (pOut && pOut->DataSize() >= sizeof(CESERVER_REQ_SETCONSCRBUF))
		{
			pIn->SetConScrBuf.hTemp = pOut->SetConScrBuf.hTemp;
		}
	}

	if (!bLock)
	{
		ExecuteFreeResult(pIn);
		ExecuteFreeResult(pOut);
	}

	// Transparently to calling function...
	SetLastError(nErrSave);
}

BOOL GetConsoleScreenBufferInfoCached(HANDLE hConsoleOutput, PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo, BOOL bForced /*= FALSE*/)
{
	BOOL lbRc = FALSE;

	static DWORD s_LastCheckTick = 0;
	static CONSOLE_SCREEN_BUFFER_INFO s_csbi = {};
	static HANDLE s_hConOut = NULL;
	//DWORD nTickDelta = 0;
	//const DWORD TickDeltaMax = 250;

	if (hConsoleOutput == NULL)
	{
		// Сброс
		s_hConOut = NULL;
		GetConsoleModeCached(NULL, NULL);
		return FALSE;
	}

	if (!lpConsoleScreenBufferInfo)
	{
		_ASSERTEX(lpConsoleScreenBufferInfo!=NULL);
		return FALSE;
	}

#if 0
	if (s_hConOut && (s_hConOut == hConsoleOutput))
	{
		nTickDelta = GetTickCount() - s_LastCheckTick;
		if (nTickDelta <= TickDeltaMax)
		{
			if (bForced)
			{
				#ifdef _DEBUG
				lbRc = FALSE;
				#endif
			}
			else
			{
				*lpConsoleScreenBufferInfo = s_csbi;
				lbRc = TRUE;
			}
		}
	}
#endif

	if (!lbRc)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = {};
		lbRc = GetConsoleScreenBufferInfo(hConsoleOutput, &csbi);
		DEBUGTEST(DWORD nBufErr = GetLastError());
		*lpConsoleScreenBufferInfo = csbi;
		if (lbRc)
		{
			s_csbi = csbi;
			s_LastCheckTick = GetTickCount();
			s_hConOut = hConsoleOutput;
		}
	}

	return lbRc;
}

BOOL GetConsoleModeCached(HANDLE hConsoleHandle, LPDWORD lpMode, BOOL bForced /*= FALSE*/)
{
	BOOL lbRc = FALSE;

	static DWORD s_LastCheckTick = 0;
	static DWORD s_dwMode = 0;
	static HANDLE s_hConHandle = NULL;
	DWORD nTickDelta = 0;
	const DWORD TickDeltaMax = 250;

	if (hConsoleHandle == NULL)
	{
		// Сброс
		s_hConHandle = NULL;
		return FALSE;
	}

	if (!lpMode)
	{
		_ASSERTEX(lpMode!=NULL);
		return FALSE;
	}

	if (s_hConHandle && (s_hConHandle == hConsoleHandle))
	{
		nTickDelta = GetTickCount() - s_LastCheckTick;
		if (nTickDelta <= TickDeltaMax)
		{
			if (bForced)
			{
				#ifdef _DEBUG
				lbRc = FALSE;
				#endif
			}
			else
			{
				*lpMode = s_dwMode;
				lbRc = TRUE;
			}
		}
	}

	if (!lbRc)
	{
		DWORD dwMode = 0;
		lbRc = GetConsoleMode(hConsoleHandle, &dwMode);
		*lpMode = dwMode;
		if (lbRc)
		{
			s_dwMode = dwMode;
			s_LastCheckTick = GetTickCount();
			s_hConHandle = hConsoleHandle;
		}
	}

	return lbRc;
}

bool IsPromptActionAllowed(bool bForce, bool bBashMargin, HANDLE* phConIn)
{
	if (!gReadConsoleInfo.InReadConsoleTID && !gReadConsoleInfo.LastReadConsoleInputTID)
		return false;

	DWORD nConInMode = 0;

	HANDLE hConIn = gReadConsoleInfo.InReadConsoleTID ? gReadConsoleInfo.hConsoleInput : gReadConsoleInfo.hConsoleInput2;
	if (!hConIn)
		return false;

	if (!gReadConsoleInfo.InReadConsoleTID && gReadConsoleInfo.LastReadConsoleInputTID)
	{
		// Проверить, может программа мышь сама обрабатывает?
		if (!GetConsoleMode(hConIn, &nConInMode))
			return false;
		if (!bForce && ((nConInMode & ENABLE_MOUSE_INPUT) == ENABLE_MOUSE_INPUT))
			return false; // Разрешить обрабатывать самой программе
	}

	if (phConIn)
		*phConIn = hConIn;

	return true;
}

BOOL OnPromptBsDeleteWord(bool bForce, bool bBashMargin)
{
	HANDLE hConIn = NULL;
	if (!IsPromptActionAllowed(bForce, bBashMargin, &hConIn))
	{
		BsDelWordMsg(L"Skipped due to !IsPromptActionAllowed!");
		return FALSE;
	}

	int iBSCount = 0;
	BOOL lbWrite = FALSE;
	DWORD dwLastError = 0;

	HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (GetConsoleScreenBufferInfo(hConOut, &csbi) && csbi.dwSize.X && csbi.dwSize.Y)
	{
		if (csbi.dwCursorPosition.X == 0)
		{
			iBSCount = 1;
		}
		else
		{
			bool bDBCS = false;
			DWORD nRead = 0;
			BOOL bReadOk = FALSE;

			// Если в консоли выбрана DBCS кодировка - там все не просто
			DWORD nCP = GetConsoleOutputCP();
			if (nCP && nCP != CP_UTF7 && nCP != CP_UTF8 && nCP != 1200 && nCP != 1201)
			{
				CPINFO cp = {};
				if (GetCPInfo(nCP, &cp) && (cp.MaxCharSize > 1))
				{
					bDBCS = true;
				}
			}

			#ifdef _DEBUG
			wchar_t szDbg[120];
			_wsprintf(szDbg, SKIPCOUNT(szDbg) L"CP=%u bDBCS=%u IsDbcs=%u X=%i", nCP, bDBCS, IsDbcs(), csbi.dwCursorPosition.X);
			BsDelWordMsg(szDbg);
			#endif

			int xPos = csbi.dwCursorPosition.X;
			COORD cr = {0, csbi.dwCursorPosition.Y};
			if ((xPos == 0) && (csbi.dwCursorPosition.Y > 0))
			{
				cr.Y--;
				xPos = csbi.dwSize.X;
			}
			COORD cursorFix = MakeCoord(xPos, cr.Y);

			wchar_t* pwszLine = (wchar_t*)malloc((csbi.dwSize.X+1)*sizeof(*pwszLine));


			if (pwszLine)
			{
				pwszLine[csbi.dwSize.X] = 0;

				// Считать строку
				if (bDBCS)
				{
					CHAR_INFO *pData = (CHAR_INFO*)calloc(csbi.dwSize.X, sizeof(CHAR_INFO));
					COORD bufSize = {csbi.dwSize.X, 1};
					SMALL_RECT rgn = {0, cr.Y, csbi.dwSize.X-1, cr.Y};

					bReadOk = ReadConsoleOutputEx(hConOut, pData, bufSize, rgn, &cursorFix);
					dwLastError = GetLastError();
					_ASSERTE(bReadOk);

					if (bReadOk)
					{
						for (int i = 0; i < csbi.dwSize.X; i++)
							pwszLine[i] = pData[i].Char.UnicodeChar;
						nRead = csbi.dwSize.X;
						xPos = cursorFix.X;
					}

					SafeFree(pData);
				}
				else
				{
					bReadOk = ReadConsoleOutputCharacterW(hConOut, pwszLine, csbi.dwSize.X, cr, &nRead);
					if (bReadOk && !nRead)
						bReadOk = FALSE;
				}

				if (bReadOk)
				{
					// Count chars
					{
						if ((int)nRead >= xPos)
						{
							int i = xPos - 1;
							_ASSERTEX(i >= 0);

							iBSCount = 0;

							// Only RIGHT brackets here to be sure that `(x86)` will be deleted including left bracket
							wchar_t cBreaks[] = L"\x20\xA0>])}$.,/\\\"";

							// Delete all `spaces` first
							while ((i >= 0) && ((pwszLine[i] == ucSpace) || (pwszLine[i] == ucNoBreakSpace)))
								iBSCount++, i--;
							_ASSERTE(cBreaks[0]==ucSpace && cBreaks[1]==ucNoBreakSpace);
							// delimiters
							while ((i >= 0) && wcschr(cBreaks+2, pwszLine[i]))
								iBSCount++, i--;
							// and all `NON-spaces`
							while ((i >= 0) && !wcschr(cBreaks, pwszLine[i]))
								iBSCount++, i--;
						}
					}
				}
			}

			// Done, string was processed
			SafeFree(pwszLine);
		}
	}
	else
	{
		BsDelWordMsg(L"GetConsoleScreenBufferInfo failed");
	}

	if (iBSCount > 0)
	{
		INPUT_RECORD* pr = (INPUT_RECORD*)calloc((size_t)iBSCount,sizeof(*pr));
		if (pr != NULL)
		{
			WORD vk = VK_BACK;
			HKL hkl = GetKeyboardLayout(gReadConsoleInfo.InReadConsoleTID ? gReadConsoleInfo.InReadConsoleTID : gReadConsoleInfo.LastReadConsoleInputTID);
			WORD sc = MapVirtualKeyEx(vk, 0/*MAPVK_VK_TO_VSC*/, hkl);
			if (!sc)
			{
				_ASSERTEX(sc!=NULL && "Can't determine SC?");
				sc = 0x0E;
			}

			for (int i = 0; i < iBSCount; i++)
			{
				pr[i].EventType = KEY_EVENT;
				pr[i].Event.KeyEvent.bKeyDown = TRUE;
				pr[i].Event.KeyEvent.wRepeatCount = 1;
				pr[i].Event.KeyEvent.wVirtualKeyCode = vk;
				pr[i].Event.KeyEvent.wVirtualScanCode = sc;
				pr[i].Event.KeyEvent.uChar.UnicodeChar = vk; // BS
				pr[i].Event.KeyEvent.dwControlKeyState = 0;
			}

			DWORD nWritten = 0;

			while (iBSCount > 0)
			{
				lbWrite = WriteConsoleInputW(hConIn, pr, min(iBSCount,256), &nWritten);
				if (!lbWrite || !nWritten)
					break;
				iBSCount -= nWritten;
			}

			free(pr);
		}
	}
	else
	{
		BsDelWordMsg(L"Nothing to delete");
	}

	UNREFERENCED_PARAMETER(dwLastError);
	return FALSE;
}

// bBashMargin - sh.exe has pad in one space cell on right edge of window
BOOL OnReadConsoleClick(SHORT xPos, SHORT yPos, bool bForce, bool bBashMargin)
{
	TODO("Тут бы нужно еще учитывать, что консоль могла прокрутиться вверх на несколько строк, если был ENABLE_WRAP_AT_EOL_OUTPUT");
	TODO("Еще интересно, что будет, если координата начала вдруг окажется за пределами буфера (типа сузили окно, и курсор уехал)");

	HANDLE hConIn = NULL;
	if (!IsPromptActionAllowed(bForce, bBashMargin, &hConIn))
		return FALSE;

	BOOL lbRc = FALSE, lbWrite = FALSE;
    int nChars = 0;
    DWORD nWritten = 0, dwLastError = 0;

	HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (GetConsoleScreenBufferInfo(hConOut, &csbi) && csbi.dwSize.X && csbi.dwSize.Y)
	{
		bool bHomeEnd = false;
		lbRc = TRUE;

		// When we are outside of standard ReadConsole[A|W]
		// it's almost impossible to detect where readline was started.
		// So, it's more safe do not try to go upper lines at all.
		if (!gReadConsoleInfo.InReadConsoleTID && (yPos < csbi.dwCursorPosition.Y))
		{
			yPos = csbi.dwCursorPosition.Y;
		}

		nChars = (csbi.dwSize.X * (yPos - csbi.dwCursorPosition.Y))
			+ (xPos - csbi.dwCursorPosition.X);

		if (nChars != 0)
		{
			char* pszLine = (char*)malloc(csbi.dwSize.X+1);
			wchar_t* pwszLine = (wchar_t*)malloc((csbi.dwSize.X+1)*sizeof(*pwszLine));

			if (pszLine && pwszLine)
			{
				int nChecked = 0;
				int iCount;
				DWORD nRead;
				COORD cr;
				SHORT nPrevSpaces = 0, nPrevChars = 0;
				SHORT nWhole = 0, nPrint = 0;
				bool bDBCS = false;
				// Если в консоли выбрана DBCS кодировка - там все не просто
				DWORD nCP = GetConsoleOutputCP();
				if (nCP && nCP != CP_UTF7 && nCP != CP_UTF8 && nCP != 1200 && nCP != 1201)
				{
					CPINFO cp = {};
					if (GetCPInfo(nCP, &cp) && (cp.MaxCharSize > 1))
					{
						bDBCS = true;
					}
				}

				TODO("DBCS!!! Must to convert cursor pos ('DBCS') to char pos!");
				// Ok, теперь нужно проверить, не был ли клик сделан "за пределами строки ввода"

				SHORT y = csbi.dwCursorPosition.Y;
				while (true)
				{
					cr.Y = y;
					if (nChars > 0)
					{
						cr.X = (y == csbi.dwCursorPosition.Y) ? csbi.dwCursorPosition.X : 0;
						iCount = (y == yPos) ? (xPos - cr.X) : (csbi.dwSize.X - cr.X);
						if (iCount < 0)
							break;
					}
					else
					{
						cr.X = 0;
						iCount = ((y == csbi.dwCursorPosition.Y) ? csbi.dwCursorPosition.X : csbi.dwSize.X)
							- ((y == yPos) ? xPos : 0);
						if (iCount < 0)
							break;
					}

					// Считать строку
					if (bDBCS)
					{
						// На DBCS кодировках "ReadConsoleOutputCharacterW" фигню возвращает
						BOOL bReadOk = ReadConsoleOutputCharacterA(hConOut, pszLine, iCount, cr, &nRead);
						dwLastError = GetLastError();

						if (!bReadOk || !nRead)
						{
							// Однако и ReadConsoleOutputCharacterA может глючить, пробуем "W"
							bReadOk = ReadConsoleOutputCharacterW(hConOut, pwszLine, iCount, cr, &nRead);
							dwLastError = GetLastError();

							if (!bReadOk || !nRead)
								break;

							bDBCS = false; // Thread string as simple Unicode.
						}
						else
						{
							nRead = MultiByteToWideChar(nCP, 0, pszLine, nRead, pwszLine, csbi.dwSize.X);
						}

						// Check chars count
						if (((int)nRead) <= 0)
							break;
					}
					else
					{
						if (!ReadConsoleOutputCharacterW(hConOut, pwszLine, iCount, cr, &nRead) || !nRead)
							break;
					}

					if (nRead > (DWORD)csbi.dwSize.X)
					{
						_ASSERTEX(nRead <= (DWORD)csbi.dwSize.X);
						break;
					}
					pwszLine[nRead] = 0;

					nWhole = nPrint = (SHORT)nRead;
					// Сначала посмотреть сколько в конце строки пробелов
					while ((nPrint > 0) && (pwszLine[nPrint-1] == L' '))
					{
						nPrint--;
					}

					// В каком направлении идем
					if (nChars > 0) // Вниз
					{
						// Если знаков (не пробелов) больше 0 - учитываем и концевые пробелы предыдущей строки
						if (nPrint > 0)
						{
							nChecked += nPrevSpaces + nPrint;
						}
						else
						{
							// Если на предыдущей строке значащих символов не было - завершаем
							if (nPrevChars <= 0)
								break;
						}
					}
					else // Вверх
					{
						if (nPrint <= 0)
							break; // На первой же пустой строке мы останавливаемся
						nChecked += nWhole;
					}
					nPrevChars = nPrint;
					nPrevSpaces = nWhole - nPrint;
					_ASSERTEX(nPrevSpaces>=0);


					// Цикл + условие
					if (nChars > 0)
					{
						if ((++y) > yPos)
							break;
					}
					else
					{
						if ((--y) < yPos)
							break;
					}
				}
				SafeFree(pszLine);
				SafeFree(pwszLine);

				// Changed?
				nChars = (nChars > 0) ? nChecked : -nChecked;
				//nChars = (csbi.dwSize.X * (yPos - csbi.dwCursorPosition.Y))
				//	+ (xPos - csbi.dwCursorPosition.X);
			}
		}

		if (nChars != 0)
		{
			int nCount = bHomeEnd ? 1 : (nChars < 0) ? (-nChars) : nChars;
			if (!bHomeEnd && (nCount > (csbi.dwSize.X * (csbi.srWindow.Bottom - csbi.srWindow.Top))))
			{
				bHomeEnd = true;
				nCount = 1;
			}

			INPUT_RECORD* pr = (INPUT_RECORD*)calloc((size_t)nCount,sizeof(*pr));
			if (pr != NULL)
			{
				WORD vk = bHomeEnd ? ((nChars < 0) ? VK_HOME : VK_END) :
					((nChars < 0) ? VK_LEFT : VK_RIGHT);
				HKL hkl = GetKeyboardLayout(gReadConsoleInfo.InReadConsoleTID ? gReadConsoleInfo.InReadConsoleTID : gReadConsoleInfo.LastReadConsoleInputTID);
				WORD sc = MapVirtualKeyEx(vk, 0/*MAPVK_VK_TO_VSC*/, hkl);
				if (!sc)
				{
					_ASSERTEX(sc!=NULL && "Can't determine SC?");
					sc = (vk == VK_LEFT)  ? 0x4B :
						 (vk == VK_RIGHT) ? 0x4D :
						 (vk == VK_HOME)  ? 0x47 :
						 (vk == VK_RIGHT) ? 0x4F : 0;
				}

				for (int i = 0; i < nCount; i++)
				{
					pr[i].EventType = KEY_EVENT;
					pr[i].Event.KeyEvent.bKeyDown = TRUE;
					pr[i].Event.KeyEvent.wRepeatCount = 1;
					pr[i].Event.KeyEvent.wVirtualKeyCode = vk;
					pr[i].Event.KeyEvent.wVirtualScanCode = sc;
					pr[i].Event.KeyEvent.dwControlKeyState = ENHANCED_KEY;
				}

				while (nCount > 0)
				{
					lbWrite = WriteConsoleInputW(hConIn, pr, min(nCount,256), &nWritten);
					if (!lbWrite || !nWritten)
						break;
					nCount -= nWritten;
				}

				free(pr);
			}
		}
	}

	UNREFERENCED_PARAMETER(dwLastError);
	return lbRc;
}

BOOL OnExecutePromptCmd(LPCWSTR asCmd)
{
	if (!gReadConsoleInfo.InReadConsoleTID && !gReadConsoleInfo.LastReadConsoleInputTID)
		return FALSE;

	HANDLE hConIn = gReadConsoleInfo.InReadConsoleTID ? gReadConsoleInfo.hConsoleInput : gReadConsoleInfo.hConsoleInput2;
	if (!hConIn)
		return FALSE;

	BOOL lbRc = FALSE;
	INPUT_RECORD r[256];
	INPUT_RECORD* pr = r;
	INPUT_RECORD* prEnd = r + countof(r);
	LPCWSTR pch = asCmd;
	DWORD nWrite, nWritten;
	BOOL lbWrite;

	while (*pch)
	{
		// Если (\r\n)|(\n) - слать \r
		if ((*pch == L'\r') || (*pch == L'\n'))
		{
			TranslateKeyPress(0, 0, L'\r', -1, pr, pr+1);

			if (*pch == L'\r' && *(pch+1) == L'\n')
				pch += 2;
			else
				pch ++;
		}
		else
		{
			TranslateKeyPress(0, 0, *pch, -1, pr, pr+1);
			pch ++;
		}

		pr += 2;
		if (pr >= prEnd)
		{
			_ASSERTE(pr == prEnd);

			if (pr && (pr > r))
			{
				nWrite = (DWORD)(pr - r);
				lbWrite = WriteConsoleInputW(hConIn, r, nWrite, &nWritten);
				if (!lbWrite)
				{
					pr = NULL;
					lbRc = FALSE;
					break;
				}
				if (*pch) // Чтобы не было переполнения буфера
					Sleep(10);
				lbRc = TRUE;
			}

			pr = r;
		}
	}

	if (pr && (pr > r))
	{
		nWrite = (DWORD)(pr - r);
		lbRc = WriteConsoleInputW(hConIn, r, nWrite, &nWritten);
	}

	return lbRc;
}

AttachConsole_t GetAttachConsoleProc()
{
	static HMODULE hKernel = NULL;
	static AttachConsole_t _AttachConsole = NULL;
	if (!hKernel)
	{
		hKernel = GetModuleHandle(L"kernel32.dll");
		if (hKernel)
		{
			_AttachConsole = (AttachConsole_t)GetProcAddress(hKernel, "AttachConsole");
		}
	}
	return _AttachConsole;
}

bool AttachServerConsole()
{
	bool lbAttachRc = false;
	DWORD nErrCode;
	HWND hCurCon = GetRealConsoleWindow();
	if (hCurCon == NULL && gnServerPID != 0)
	{
		// функция есть только в WinXP и выше
		AttachConsole_t _AttachConsole = GetAttachConsoleProc();
		if (_AttachConsole)
		{
			lbAttachRc = (_AttachConsole(gnServerPID) != 0);
			if (!lbAttachRc)
			{
				nErrCode = GetLastError();
				_ASSERTE(nErrCode==0 && lbAttachRc);
			}
		}
	}
	return lbAttachRc;
}

// Drop the progress flag, because the prompt may appear
void CheckPowershellProgressPresence()
{
	if (!gbPowerShellMonitorProgress || (gnPowerShellProgressValue == -1))
		return;

	// При возврате в Prompt - сброс прогресса
	gnPowerShellProgressValue = -1;
	GuiSetProgress(0,0);
}

// PowerShell AI для определения прогресса в консоли
void CheckPowerShellProgress(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
{
	_ASSERTE(dwBufferSize.Y >= 5);
	int nProgress = -1;
	WORD nNeedAttr = lpBuffer->Attributes;

	for (SHORT Y = dwBufferSize.Y - 2; Y > 0; Y--)
	{
		const CHAR_INFO* pLine = lpBuffer + dwBufferSize.X * Y;

		// 120720 - PS игнорирует PopupColors в консоли. Вывод прогресса всегда идет 0x3E
		if (nNeedAttr/*gnConsolePopupColors*/ != 0)
		{
			if ((pLine[4].Attributes != nNeedAttr/*gnConsolePopupColors*/)
				|| (pLine[dwBufferSize.X - 7].Attributes != nNeedAttr/*gnConsolePopupColors*/))
				break; // не оно
		}

		if ((pLine[4].Char.UnicodeChar == L'[') && (pLine[dwBufferSize.X - 7].Char.UnicodeChar == L']'))
		{
			// Считаем проценты
			SHORT nLen = dwBufferSize.X - 7 - 5;
			if (nLen > 0)
			{
				nProgress = 100;
				for (SHORT X = 5; X < (dwBufferSize.X - 8); X++)
				{
					if (pLine[X].Char.UnicodeChar == L' ')
					{
						nProgress = (X - 5) * 100 / nLen;
						break;
					}
				}
			}
			break;
		}
	}

	if (nProgress != gnPowerShellProgressValue)
	{
		gnPowerShellProgressValue = nProgress;
		GuiSetProgress((nProgress != -1) ? 1 : 0, nProgress);
	}
}

void FixHwnd4ConText(HWND& hWnd)
{
	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
	{
		hWnd = ghConWnd;
	}
}
