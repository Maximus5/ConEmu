
/*
Copyright (c) 2009-2010 Maximus5
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


#include <windows.h>
#include "common.hpp"



// Undocumented console message
#define WM_SETCONSOLEINFO			(WM_USER+201)

#pragma pack(push, 1)

//
//	Structure to send console via WM_SETCONSOLEINFO
//
typedef struct _CONSOLE_INFO
{
	ULONG		Length;
	COORD		ScreenBufferSize;
	COORD		WindowSize;
	ULONG		WindowPosX;
	ULONG		WindowPosY;

	COORD		FontSize;
	ULONG		FontFamily;
	ULONG		FontWeight;
	WCHAR		FaceName[32];

	ULONG		CursorSize;
	ULONG		FullScreen;
	ULONG		QuickEdit;
	ULONG		AutoPosition;
	ULONG		InsertMode;

	USHORT		ScreenColors;
	USHORT		PopupColors;
	ULONG		HistoryNoDup;
	ULONG		HistoryBufferSize;
	ULONG		NumberOfHistoryBuffers;

	COLORREF	ColorTable[16];

	ULONG		CodePage;
	HWND		Hwnd;

	WCHAR		ConsoleTitle[0x100];

} CONSOLE_INFO;

CONSOLE_INFO* gpConsoleInfoStr = NULL;
HANDLE ghConsoleSection = NULL;
const UINT gnConsoleSectionSize = sizeof(CONSOLE_INFO)+1024;

#pragma pack(pop)










// Возвращает 0, если успешно, иначе - ошибка
int NextArg(const wchar_t** asCmdLine, wchar_t (&rsArg)[MAX_PATH+1], const wchar_t** rsArgStart/*=NULL*/)
{
	LPCWSTR psCmdLine = *asCmdLine, pch = NULL;
	wchar_t ch = *psCmdLine;
	size_t nArgLen = 0;
	bool lbQMode = false;

	while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);

	if (ch == 0) return CERR_CMDLINEEMPTY;

	// аргумент начинается с "
	if (ch == L'"')
	{
		lbQMode = true;
		psCmdLine++;
		pch = wcschr(psCmdLine, L'"');

		if (!pch) return CERR_CMDLINE;

		while (pch[1] == L'"')
		{
			pch += 2;
			pch = wcschr(pch, L'"');

			if (!pch) return CERR_CMDLINE;
		}

		// Теперь в pch ссылка на последнюю "
	}
	else
	{
		// До конца строки или до первого пробела
		//pch = wcschr(psCmdLine, L' ');
		// 09.06.2009 Maks - обломался на: cmd /c" echo Y "
		pch = psCmdLine;

		// Ищем обычным образом (до пробела/кавычки)
		while (*pch && *pch!=L' ' && *pch!=L'"') pch++;

		//if (!pch) pch = psCmdLine + lstrlenW(psCmdLine); // до конца строки
	}

	nArgLen = pch - psCmdLine;

	if (nArgLen > MAX_PATH) return CERR_CMDLINE;

	// Вернуть аргумент
	memcpy(rsArg, psCmdLine, nArgLen*sizeof(*rsArg));

	if (rsArgStart) *rsArgStart = psCmdLine;

	rsArg[nArgLen] = 0;
	psCmdLine = pch;
	// Finalize
	ch = *psCmdLine; // может указывать на закрывающую кавычку

	if (lbQMode && ch == L'"') ch = *(++psCmdLine);

	while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);

	*asCmdLine = psCmdLine;
	return 0;
}

int NextArg(const char** asCmdLine, char (&rsArg)[MAX_PATH+1], const char** rsArgStart/*=NULL*/)
{
	LPCSTR psCmdLine = *asCmdLine, pch = NULL;
	char ch = *psCmdLine;
	size_t nArgLen = 0;
	bool lbQMode = false;

	while (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') ch = *(++psCmdLine);

	if (ch == 0) return CERR_CMDLINEEMPTY;

	// аргумент начинается с "
	if (ch == '"')
	{
		lbQMode = true;
		psCmdLine++;
		pch = strchr(psCmdLine, '"');

		if (!pch) return CERR_CMDLINE;

		while (pch[1] == '"')
		{
			pch += 2;
			pch = strchr(pch, '"');

			if (!pch) return CERR_CMDLINE;
		}

		// Теперь в pch ссылка на последнюю "
	}
	else
	{
		// До конца строки или до первого пробела
		//pch = wcschr(psCmdLine, ' ');
		// 09.06.2009 Maks - обломался на: cmd /c" echo Y "
		pch = psCmdLine;

		// Ищем обычным образом (до пробела/кавычки)
		while (*pch && *pch!=' ' && *pch!='"') pch++;

		//if (!pch) pch = psCmdLine + lstrlenW(psCmdLine); // до конца строки
	}

	nArgLen = pch - psCmdLine;

	if (nArgLen > MAX_PATH) return CERR_CMDLINE;

	// Вернуть аргумент
	memcpy(rsArg, psCmdLine, nArgLen*sizeof(*rsArg));

	if (rsArgStart) *rsArgStart = psCmdLine;

	rsArg[nArgLen] = 0;
	psCmdLine = pch;
	// Finalize
	ch = *psCmdLine; // может указывать на закрывающую кавычку

	if (lbQMode && ch == '"') ch = *(++psCmdLine);

	while(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') ch = *(++psCmdLine);

	*asCmdLine = psCmdLine;
	return 0;
}

const wchar_t* SkipNonPrintable(const wchar_t* asParams)
{
	if (!asParams)
		return NULL;
	const wchar_t* psz = asParams;
	while (*psz == L' ' || *psz == L'\t' || *psz == L'\r' || *psz == L'\n') psz++;
	return psz;
}


BOOL PackInputRecord(const INPUT_RECORD* piRec, MSG64* pMsg)
{
	_ASSERTE(pMsg!=NULL && piRec!=NULL);
	memset(pMsg, 0, sizeof(MSG64));
	UINT nMsg = 0; WPARAM wParam = 0; LPARAM lParam = 0;

	if (piRec->EventType == KEY_EVENT)
	{
		nMsg = piRec->Event.KeyEvent.bKeyDown ? WM_KEYDOWN : WM_KEYUP;
		lParam |= (WORD)piRec->Event.KeyEvent.uChar.UnicodeChar;
		lParam |= ((BYTE)piRec->Event.KeyEvent.wVirtualKeyCode) << 16;
		lParam |= ((BYTE)piRec->Event.KeyEvent.wVirtualScanCode) << 24;
		wParam |= (WORD)piRec->Event.KeyEvent.dwControlKeyState;
		wParam |= ((DWORD)piRec->Event.KeyEvent.wRepeatCount & 0xFF) << 16;
	}
	else if (piRec->EventType == MOUSE_EVENT)
	{
		switch(piRec->Event.MouseEvent.dwEventFlags)
		{
			case MOUSE_MOVED:
				nMsg = MOUSE_EVENT_MOVE;
				break;
			case 0:
				nMsg = MOUSE_EVENT_CLICK;
				break;
			case DOUBLE_CLICK:
				nMsg = MOUSE_EVENT_DBLCLICK;
				break;
			case MOUSE_WHEELED:
				nMsg = MOUSE_EVENT_WHEELED;
				break;
			case /*MOUSE_HWHEELED*/ 0x0008:
				nMsg = MOUSE_EVENT_HWHEELED;
				break;
			default:
				_ASSERT(FALSE);
		}

		lParam = ((WORD)piRec->Event.MouseEvent.dwMousePosition.X)
		         | (((DWORD)(WORD)piRec->Event.MouseEvent.dwMousePosition.Y) << 16);
		// max 0x0010/*FROM_LEFT_4ND_BUTTON_PRESSED*/
		wParam |= ((DWORD)piRec->Event.MouseEvent.dwButtonState) & 0xFF;
		// max - ENHANCED_KEY == 0x0100
		wParam |= (((DWORD)piRec->Event.MouseEvent.dwControlKeyState) & 0xFFFF) << 8;

		if (nMsg == MOUSE_EVENT_WHEELED || nMsg == MOUSE_EVENT_HWHEELED)
		{
			// HIWORD() - short (direction[1/-1])*count*120
			short nWheel = (short)((((DWORD)piRec->Event.MouseEvent.dwButtonState) & 0xFFFF0000) >> 16);
			char  nCount = nWheel / 120;
			wParam |= ((DWORD)(BYTE)nCount) << 24;
		}
	}
	else if (piRec->EventType == FOCUS_EVENT)
	{
		nMsg = piRec->Event.FocusEvent.bSetFocus ? WM_SETFOCUS : WM_KILLFOCUS;
	}
	else
	{
		_ASSERT(FALSE);
		return FALSE;
	}

	_ASSERTE(nMsg!=0);
	pMsg->message = nMsg;
	pMsg->wParam = wParam;
	pMsg->lParam = lParam;
	return TRUE;
}

BOOL UnpackInputRecord(const MSG64* piMsg, INPUT_RECORD* pRec)
{
	_ASSERTE(piMsg!=NULL && pRec!=NULL);
	memset(pRec, 0, sizeof(INPUT_RECORD));

	if (piMsg->message == 0)
		return FALSE;

	if (piMsg->message == WM_KEYDOWN || piMsg->message == WM_KEYUP)
	{
		pRec->EventType = KEY_EVENT;
		// lParam
		pRec->Event.KeyEvent.bKeyDown = (piMsg->message == WM_KEYDOWN);
		pRec->Event.KeyEvent.uChar.UnicodeChar = (WCHAR)(piMsg->lParam & 0xFFFF);
		pRec->Event.KeyEvent.wVirtualKeyCode   = (((DWORD)piMsg->lParam) & 0xFF0000) >> 16;
		pRec->Event.KeyEvent.wVirtualScanCode  = (((DWORD)piMsg->lParam) & 0xFF000000) >> 24;
		// wParam. Пока что тут может быть max(ENHANCED_KEY==0x0100)
		pRec->Event.KeyEvent.dwControlKeyState = ((DWORD)piMsg->wParam & 0xFFFF);
		pRec->Event.KeyEvent.wRepeatCount = ((DWORD)piMsg->wParam & 0xFF0000) >> 16;
	}
	else if (piMsg->message >= MOUSE_EVENT_FIRST && piMsg->message <= MOUSE_EVENT_LAST)
	{
		pRec->EventType = MOUSE_EVENT;

		switch(piMsg->message)
		{
			case MOUSE_EVENT_MOVE:
				pRec->Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
				break;
			case MOUSE_EVENT_CLICK:
				pRec->Event.MouseEvent.dwEventFlags = 0;
				break;
			case MOUSE_EVENT_DBLCLICK:
				pRec->Event.MouseEvent.dwEventFlags = DOUBLE_CLICK;
				break;
			case MOUSE_EVENT_WHEELED:
				pRec->Event.MouseEvent.dwEventFlags = MOUSE_WHEELED;
				break;
			case MOUSE_EVENT_HWHEELED:
				pRec->Event.MouseEvent.dwEventFlags = /*MOUSE_HWHEELED*/ 0x0008;
				break;
		}

		pRec->Event.MouseEvent.dwMousePosition.X = LOWORD(piMsg->lParam);
		pRec->Event.MouseEvent.dwMousePosition.Y = HIWORD(piMsg->lParam);
		// max 0x0010/*FROM_LEFT_4ND_BUTTON_PRESSED*/
		pRec->Event.MouseEvent.dwButtonState = ((DWORD)piMsg->wParam) & 0xFF;
		// max - ENHANCED_KEY == 0x0100
		pRec->Event.MouseEvent.dwControlKeyState = (((DWORD)piMsg->wParam) & 0xFFFF00) >> 8;

		if (piMsg->message == MOUSE_EVENT_WHEELED || piMsg->message == MOUSE_EVENT_HWHEELED)
		{
			// HIWORD() - short (direction[1/-1])*count*120
			signed char nDir = (signed char)((((DWORD)piMsg->wParam) & 0xFF000000) >> 24);
			WORD wDir = nDir*120;
			pRec->Event.MouseEvent.dwButtonState |= wDir << 16;
		}
	}
	else if (piMsg->message == WM_SETFOCUS || piMsg->message == WM_KILLFOCUS)
	{
		pRec->EventType = FOCUS_EVENT;
		pRec->Event.FocusEvent.bSetFocus = (piMsg->message == WM_SETFOCUS);
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

class MNullDesc
{
	protected:
		PSECURITY_DESCRIPTOR mp_NullDesc;
		SECURITY_ATTRIBUTES  m_NullSecurity;
		PSECURITY_DESCRIPTOR mp_LocalDesc;
		SECURITY_ATTRIBUTES  m_LocalSecurity;
	public:
		DWORD mn_LastError;
	public:
		MNullDesc()
		{
			memset(&m_NullSecurity, 0, sizeof(m_NullSecurity));
			mp_NullDesc = NULL;
			memset(&m_LocalSecurity, 0, sizeof(m_LocalSecurity));
			mp_LocalDesc = NULL;
			mn_LastError = 0;
		};
		~MNullDesc()
		{
			memset(&m_NullSecurity, 0, sizeof(m_NullSecurity));
			LocalFree(mp_NullDesc); mp_NullDesc = NULL;
			memset(&m_LocalSecurity, 0, sizeof(m_LocalSecurity));
			LocalFree(mp_LocalDesc); mp_LocalDesc = NULL;
		};
	public:
		SECURITY_ATTRIBUTES* NullSecurity()
		{
			mn_LastError = 0;

			if (mp_NullDesc)
			{
				_ASSERTE(m_NullSecurity.lpSecurityDescriptor==mp_NullDesc);
				return (&m_NullSecurity);
			}

			mp_NullDesc = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
			              SECURITY_DESCRIPTOR_MIN_LENGTH);

			if (mp_NullDesc == NULL)
			{
				mn_LastError = GetLastError();
				return NULL;
			}

			if (!InitializeSecurityDescriptor(mp_NullDesc, SECURITY_DESCRIPTOR_REVISION))
			{
				mn_LastError = GetLastError();
				LocalFree(mp_NullDesc); mp_NullDesc = NULL;
				return NULL;
			}

			// Add a null DACL to the security descriptor.
			if (!SetSecurityDescriptorDacl(mp_NullDesc, TRUE, (PACL) NULL, FALSE))
			{
				mn_LastError = GetLastError();
				LocalFree(mp_NullDesc); mp_NullDesc = NULL;
				return NULL;
			}

			m_NullSecurity.nLength = sizeof(m_NullSecurity);
			m_NullSecurity.lpSecurityDescriptor = mp_NullDesc;
			m_NullSecurity.bInheritHandle = TRUE;
			return (&m_NullSecurity);
		};
		SECURITY_ATTRIBUTES* LocalSecurity()
		{
			static PACL pACL = NULL;
			//static PSID pNetworkSid = NULL, pSidWorld = NULL;
			BYTE NetworkSid[12] = {01,01,00,00,00,00,00,05,02,00,00,00};
			BYTE SidWorld[12]   = {01,01,00,00,00,00,00,01,00,00,00,00};
			PSID pNetworkSid = (PSID)NetworkSid;
			PSID pSidWorld = (PSID)SidWorld;
			//static DWORD LastError = 0;
			DWORD dwSize = 0;

			if (mp_LocalDesc)
			{
				_ASSERTE(m_LocalSecurity.lpSecurityDescriptor==mp_LocalDesc);
				return (&m_LocalSecurity);
			}

			mp_LocalDesc = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
			               SECURITY_DESCRIPTOR_MIN_LENGTH);

			if (mp_LocalDesc == NULL)
			{
				mn_LastError = GetLastError();
				return NULL;
			}

			if (!InitializeSecurityDescriptor(mp_LocalDesc, SECURITY_DESCRIPTOR_REVISION))
			{
				mn_LastError = GetLastError();
				LocalFree(mp_LocalDesc); mp_LocalDesc = NULL;
				return NULL;
			}

			//// Создать SID для Network & Everyone
			//dwSize = SECURITY_MAX_SID_SIZE;
			//pNetworkSid = (PSID)LocalAlloc(LMEM_FIXED, dwSize);
			//pSidWorld   = (PSID)LocalAlloc(LMEM_FIXED, dwSize);
			//if (!pNetworkSid || !pSidWorld)
			//{
			//	mn_LastError = GetLastError();
			//	return NULL;
			//}
			//dwSize = SECURITY_MAX_SID_SIZE;
			//// 01 01 00 00 00 00 00 05 02 00 00 00
			//if (!CreateWellKnownSid(WinNetworkSid, NULL, pNetworkSid, &dwSize))
			//{
			//	LastError = GetLastError();
			//	return NULL;
			//}
			//dwSize = SECURITY_MAX_SID_SIZE;
			//// 01 01 00 00 00 00 00 01 00 00 00 00
			//if (!CreateWellKnownSid(WinWorldSid, NULL, pSidWorld, &dwSize))
			//{
			//	mn_LastError = GetLastError();
			//	return NULL;
			//}
			// Создать DACL
			dwSize = sizeof(ACL)
			         + sizeof(ACCESS_DENIED_ACE) + GetLengthSid(pNetworkSid)
			         + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pSidWorld);
			pACL = (PACL)LocalAlloc(LPTR, dwSize);

			if (!InitializeAcl(pACL, dwSize, ACL_REVISION))
			{
				mn_LastError = GetLastError();
				return NULL;
			}

			// Теперь - собственно права
			if (!AddAccessDeniedAce(pACL, ACL_REVISION, GENERIC_ALL, pNetworkSid))
			{
				mn_LastError = GetLastError();
				return NULL;
			}

			if (!AddAccessAllowedAce(pACL, ACL_REVISION, GENERIC_ALL, pSidWorld))
			{
				mn_LastError = GetLastError();
				return NULL;
			}

			// Add a null DACL to the security descriptor.
			if (!SetSecurityDescriptorDacl(mp_LocalDesc, TRUE, pACL, FALSE))
			{
				mn_LastError = GetLastError();
				LocalFree(mp_LocalDesc); mp_LocalDesc = NULL;
				return NULL;
			}

			m_LocalSecurity.nLength = sizeof(m_LocalSecurity);
			m_LocalSecurity.lpSecurityDescriptor = mp_LocalDesc;
			m_LocalSecurity.bInheritHandle = TRUE;
			return (&m_LocalSecurity);
		};
};
MNullDesc *gNullDesc = NULL;

SECURITY_ATTRIBUTES* NullSecurity()
{
	if (!gNullDesc) gNullDesc = new MNullDesc();

	return gNullDesc->NullSecurity();
}
SECURITY_ATTRIBUTES* LocalSecurity()
{
	if (!gNullDesc) gNullDesc = new MNullDesc();

	return gNullDesc->LocalSecurity();
}

BOOL gbInCommonShutdown = FALSE;
HANDLE ghHookMutex = NULL;
#ifdef _DEBUG
extern HANDLE ghInMyAssertTrap;
extern DWORD gnInMyAssertThread;
#endif

void CommonShutdown()
{
	gbInCommonShutdown = TRUE;

	if (gNullDesc)
	{
		delete gNullDesc;
		gNullDesc = NULL;
	}

	// Clean memory
	if (ghConsoleSection)
	{
		CloseHandle(ghConsoleSection);
		ghConsoleSection = NULL;
	}

	if (gpConsoleInfoStr)
	{
		LocalFree(gpConsoleInfoStr);
		gpConsoleInfoStr = NULL;
	}

	if (ghHookMutex)
	{
		CloseHandle(ghHookMutex);
		ghHookMutex = NULL;
	}

#ifdef _DEBUG
	MyAssertShutdown();
#endif
}












static BOOL CALLBACK MyEnumMonitors(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	LPRECT lprc = (LPRECT)dwData;

	if (lprcMonitor->left < lprc->left)
		lprc->left = lprcMonitor->left;

	if (lprcMonitor->top < lprc->top)
		lprc->top = lprcMonitor->top;

	if (lprcMonitor->right > lprc->right)
		lprc->right = lprcMonitor->right;

	if (lprcMonitor->bottom > lprc->bottom)
		lprc->bottom = lprcMonitor->bottom;

	return TRUE;
}


//
//	SETCONSOLEINFO.C
//
//	Undocumented method to set console attributes at runtime
//
//	For Vista use the newly documented SetConsoleScreenBufferEx API
//
//	www.catch22.net
//

//
//	Wrapper around WM_SETCONSOLEINFO. We need to create the
//  necessary section (file-mapping) object in the context of the
//  process which owns the console, before posting the message
//
BOOL SetConsoleInfo(HWND hwndConsole, CONSOLE_INFO *pci)
{
	DWORD   dwConsoleOwnerPid, dwCurProcId, dwConsoleThreadId;
	PVOID   ptrView = 0;
	DWORD   dwLastError=0;
	WCHAR   ErrText[255];
	//
	//	Retrieve the process which "owns" the console
	//
	dwCurProcId = GetCurrentProcessId();
	dwConsoleThreadId = GetWindowThreadProcessId(hwndConsole, &dwConsoleOwnerPid);

	// We'll fail, if console was created by other process
	if (dwConsoleOwnerPid != dwCurProcId)
	{
		_ASSERTE(dwConsoleOwnerPid == dwCurProcId);
		return FALSE;
	}

	//
	// Create a SECTION object backed by page-file, then map a view of
	// this section into the owner process so we can write the contents
	// of the CONSOLE_INFO buffer into it
	//
	if (!ghConsoleSection)
	{
		ghConsoleSection = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, gnConsoleSectionSize, 0);

		if (!ghConsoleSection)
		{
			dwLastError = GetLastError();
			_wsprintf(ErrText, SKIPLEN(countof(ErrText)) L"Can't CreateFileMapping(ghConsoleSection). ErrCode=%i", dwLastError);
			MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
			return FALSE;
		}
	}

	//
	//	Copy our console structure into the section-object
	//
	ptrView = MapViewOfFile(ghConsoleSection, FILE_MAP_WRITE|FILE_MAP_READ, 0, 0, gnConsoleSectionSize);

	if (!ptrView)
	{
		dwLastError = GetLastError();
		_wsprintf(ErrText, SKIPLEN(countof(ErrText)) L"Can't MapViewOfFile. ErrCode=%i", dwLastError);
		MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
	}
	else
	{
		_ASSERTE(pci->Length==sizeof(CONSOLE_INFO));
		//2010-09-19 что-то на XP стало окошко мелькать.
		// при отсылке WM_SETCONSOLEINFO консоль отображается :(
		BOOL lbWasVisible = IsWindowVisible(hwndConsole);
		RECT rcOldPos = {0}, rcAllMonRect = {0};

		if (!lbWasVisible)
		{
			GetWindowRect(hwndConsole, &rcOldPos);
			// В много-мониторных конфигурациях координаты на некоторых могут быть отрицательными!
			EnumDisplayMonitors(NULL, NULL, MyEnumMonitors, (LPARAM)&rcAllMonRect);
			pci->AutoPosition = FALSE;
			pci->WindowPosX = rcAllMonRect.left - 1280;
			pci->WindowPosY = rcAllMonRect.top - 1024;
		}

		memcpy(ptrView, pci, pci->Length);
		UnmapViewOfFile(ptrView);
		//  Send console window the "update" message
		LRESULT dwConInfoRc = 0;
		DWORD dwConInfoErr = 0;
		dwConInfoRc = SendMessage(hwndConsole, WM_SETCONSOLEINFO, (WPARAM)ghConsoleSection, 0);
		dwConInfoErr = GetLastError();

		if (!lbWasVisible && IsWindowVisible(hwndConsole))
		{
#ifdef _DEBUG
			//Sleep(10);
#endif
			ShowWindow(hwndConsole, SW_HIDE);
			//SetWindowPos(hwndConsole, NULL, rcOldPos.left, rcOldPos.top, 0,0, SWP_NOSIZE|SWP_NOZORDER);
			// -- чтобы на некоторых системах не возникала проблема с позиционированием -> {0,0}
			// Issue 274: Окно реальной консоли позиционируется в неудобном месте
			SetWindowPos(hwndConsole, NULL, 0, 0, 0,0, SWP_NOSIZE|SWP_NOZORDER);
		}
	}

	return TRUE;
}

//
//	Fill the CONSOLE_INFO structure with information
//  about the current console window
//
void GetConsoleSizeInfo(CONSOLE_INFO *pci)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	HANDLE hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	BOOL lbRc = GetConsoleScreenBufferInfo(hConsoleOut, &csbi);

	if (lbRc)
	{
		pci->ScreenBufferSize = csbi.dwSize;
		pci->WindowSize.X	  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		pci->WindowSize.Y	  = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
		// Было... а это координаты окна (хотя включается флажок "Autoposition"
		//pci->WindowPosX	      = csbi.srWindow.Left;
		//pci->WindowPosY		  = csbi.srWindow.Top;
	}
	else
	{
		_ASSERTE(lbRc);
		pci->ScreenBufferSize.X = pci->WindowSize.X = 80;
		pci->ScreenBufferSize.Y = pci->WindowSize.Y = 25;
	}

	// Поскольку включен флажок "AutoPosition" - то это игнорируется
	pci->WindowPosX = pci->WindowPosY = 0;
	/*
	RECT rcWnd = {0}; GetWindowRect(GetConsoleWindow(), &rcWnd);
	pci->WindowPosX	      = rcWnd.left;
	pci->WindowPosY		  = rcWnd.top;
	*/
}


#if defined(__GNUC__)
#define __in
#define __out
#undef ENABLE_AUTO_POSITION
#endif

//VISTA support:
#ifndef ENABLE_AUTO_POSITION
typedef struct _CONSOLE_FONT_INFOEX
{
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


void SetConsoleFontSizeTo(HWND inConWnd, int inSizeY, int inSizeX, const wchar_t *asFontName)
{
	HMODULE hKernel = GetModuleHandle(L"kernel32.dll");

	if (!hKernel)
		return;

	PGetCurrentConsoleFontEx GetCurrentConsoleFontEx = (PGetCurrentConsoleFontEx)
	        GetProcAddress(hKernel, "GetCurrentConsoleFontEx");
	PSetCurrentConsoleFontEx SetCurrentConsoleFontEx = (PSetCurrentConsoleFontEx)
	        GetProcAddress(hKernel, "SetCurrentConsoleFontEx");

	if (GetCurrentConsoleFontEx && SetCurrentConsoleFontEx)  // We have Vista
	{
		CONSOLE_FONT_INFOEX cfi = {sizeof(CONSOLE_FONT_INFOEX)};
		//GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
		cfi.dwFontSize.X = inSizeX;
		cfi.dwFontSize.Y = inSizeY;
		lstrcpynW(cfi.FaceName, asFontName ? asFontName : L"Lucida Console", LF_FACESIZE);
		SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
	}
	else // We have other NT
	{
		const COLORREF DefaultColors[16] =
		{
			0x00000000, 0x00800000, 0x00008000, 0x00808000,
			0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
			0x00808080,	0x00ff0000, 0x0000ff00, 0x00ffff00,
			0x000000ff, 0x00ff00ff,	0x0000ffff, 0x00ffffff
		};

		if (!gpConsoleInfoStr)
		{
			gpConsoleInfoStr = (CONSOLE_INFO*)LocalAlloc(LPTR, sizeof(CONSOLE_INFO));

			if (!gpConsoleInfoStr)
			{
				_ASSERTE(gpConsoleInfoStr!=NULL);
				return; // memory allocation failed
			}

			gpConsoleInfoStr->Length = sizeof(CONSOLE_INFO);
		}

		int i;
		// get current size/position settings rather than using defaults..
		GetConsoleSizeInfo(gpConsoleInfoStr);
		// set these to zero to keep current settings
		gpConsoleInfoStr->FontSize.X				= inSizeX;
		gpConsoleInfoStr->FontSize.Y				= inSizeY;
		gpConsoleInfoStr->FontFamily				= 0;//0x30;//FF_MODERN|FIXED_PITCH;//0x30;
		gpConsoleInfoStr->FontWeight				= 0;//0x400;
		lstrcpynW(gpConsoleInfoStr->FaceName, asFontName ? asFontName : L"Lucida Console", 32);
		gpConsoleInfoStr->CursorSize				= 25;
		gpConsoleInfoStr->FullScreen				= FALSE;
		gpConsoleInfoStr->QuickEdit					= FALSE;
		//gpConsoleInfoStr->AutoPosition			= 0x10000;
		gpConsoleInfoStr->AutoPosition				= FALSE;
		RECT rcCon; GetWindowRect(inConWnd, &rcCon);
		gpConsoleInfoStr->WindowPosX = rcCon.left;
		gpConsoleInfoStr->WindowPosY = rcCon.top;
		gpConsoleInfoStr->InsertMode				= TRUE;
		gpConsoleInfoStr->ScreenColors				= MAKEWORD(0x7, 0x0);
		gpConsoleInfoStr->PopupColors				= MAKEWORD(0x5, 0xf);
		gpConsoleInfoStr->HistoryNoDup				= FALSE;
		gpConsoleInfoStr->HistoryBufferSize			= 50;
		gpConsoleInfoStr->NumberOfHistoryBuffers	= 4;

		// color table
		for(i = 0; i < 16; i++)
			gpConsoleInfoStr->ColorTable[i] = DefaultColors[i];

		gpConsoleInfoStr->CodePage					= GetConsoleOutputCP();//0;//0x352;
		gpConsoleInfoStr->Hwnd						= inConWnd;
		gpConsoleInfoStr->ConsoleTitle[0] = 0;
		SetConsoleInfo(inConWnd, gpConsoleInfoStr);
	}
}
/*
-- пробовал в Win7 это не работает
void SetConsoleBufferSize(HWND inConWnd, int anWidth, int anHeight, int anBufferHeight)
{
	if (!gpConsoleInfoStr) {
		_ASSERTE(gpConsoleInfoStr!=NULL);
		return; // memory allocation failed
	}

	TODO("Заполнить и другие текущие значения!");
	gpConsoleInfoStr->CodePage					= GetConsoleOutputCP();//0;//0x352;

	// Теперь собственно, что хотим поменять
	gpConsoleInfoStr->ScreenBufferSize.X = gpConsoleInfoStr->WindowSize.X = anWidth;
	gpConsoleInfoStr->WindowSize.Y = anHeight;
	gpConsoleInfoStr->ScreenBufferSize.Y = anBufferHeight;

	SetConsoleInfo(inConWnd, gpConsoleInfoStr);
}
*/

