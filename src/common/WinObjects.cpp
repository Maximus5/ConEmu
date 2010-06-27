
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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
#include "WinObjects.h"

#ifdef _DEBUG
void getWindowInfo(HWND ahWnd, wchar_t* rsInfo/*[1024]*/)
{
	if (!ahWnd) {
		lstrcpyW(rsInfo, L"<NULL>");
	} else if (!IsWindow(ahWnd)) {
		wsprintfW(rsInfo, L"0x%08X: Invalid window handle", (DWORD)ahWnd);
	} else {
		wchar_t szClass[256]; if (!GetClassName(ahWnd, szClass, 256)) lstrcpyW(szClass, L"<GetClassName failed>");
		wchar_t szTitle[512]; if (!GetWindowText(ahWnd, szTitle, 512)) szTitle[0] = 0;
		wsprintfW(rsInfo, L"0x%08X: %s - '%s'", (DWORD)ahWnd, szClass, szTitle);
	}
}
#endif

BOOL apiSetForegroundWindow(HWND ahWnd)
{
	#ifdef _DEBUG
	wchar_t szLastFore[1024]; getWindowInfo(GetForegroundWindow(), szLastFore);
	wchar_t szWnd[1024]; getWindowInfo(ahWnd, szWnd);
	#endif
	BOOL lbRc = ::SetForegroundWindow(ahWnd);
	return lbRc;
}

BOOL apiShowWindow(HWND ahWnd, int anCmdShow)
{
	#ifdef _DEBUG
	wchar_t szLastFore[1024]; getWindowInfo(GetForegroundWindow(), szLastFore);
	wchar_t szWnd[1024]; getWindowInfo(ahWnd, szWnd);
	#endif
	BOOL lbRc = ::ShowWindow(ahWnd, anCmdShow);
	return lbRc;
}

BOOL apiShowWindowAsync(HWND ahWnd, int anCmdShow)
{
	#ifdef _DEBUG
	wchar_t szLastFore[1024]; getWindowInfo(GetForegroundWindow(), szLastFore);
	wchar_t szWnd[1024]; getWindowInfo(ahWnd, szWnd);
	#endif
	BOOL lbRc = ::ShowWindowAsync(ahWnd, anCmdShow);
	return lbRc;
}



BOOL GetShortFileName(LPCWSTR asFullPath, wchar_t* rsShortName/*name only, MAX_PATH required*/)
{
	WIN32_FIND_DATAW fnd; memset(&fnd, 0, sizeof(fnd));
	HANDLE hFind = FindFirstFile(asFullPath, &fnd);
	if (hFind == INVALID_HANDLE_VALUE)
		return FALSE;
	FindClose(hFind);
	if (fnd.cAlternateFileName[0]) {
		if (lstrlenW(fnd.cAlternateFileName) < lstrlenW(fnd.cFileName)) {
			lstrcpyW(rsShortName, fnd.cAlternateFileName);
			return TRUE;
		}
	}

	return FALSE;
}

wchar_t* GetShortFileNameEx(LPCWSTR asLong)
{
	TODO("хорошо бы и сетевые диски обрабатывать");
	if (!asLong) return NULL;
	
	wchar_t* pszShort = /*_wcsdup(asLong);*/(wchar_t*)malloc((lstrlenW(asLong)+1)*2);
	lstrcpyW(pszShort, asLong);
	wchar_t* pszCur = wcschr(pszShort+3, L'\\');
	wchar_t* pszSlash;
	wchar_t  szName[MAX_PATH+1];
	size_t nLen = 0;

	while (pszCur) {
		*pszCur = 0;
		{
			if (GetShortFileName(pszShort, szName)) {
				if ((pszSlash = wcsrchr(pszShort, L'\\'))==0)
					goto wrap;
				lstrcpyW(pszSlash+1, szName);
				pszSlash += 1+lstrlenW(szName);
				lstrcpyW(pszSlash+1, pszCur+1);
				pszCur = pszSlash;
			}
		}
		*pszCur = L'\\';
		pszCur = wcschr(pszCur+1, L'\\');
	}
	nLen = lstrlenW(pszShort);
	if (nLen>0 && pszShort[nLen-1]==L'\\')
		pszShort[--nLen] = 0;
	if (nLen <= MAX_PATH)
		return pszShort;

wrap:
	free(pszShort);
	return NULL;
}


BOOL IsUserAdmin()
{
	OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)};
	GetVersionEx(&osv);
	// Проверять нужно только для висты, чтобы на XP лишний "Щит" не отображался
	if (osv.dwMajorVersion < 6)
		return FALSE;

	BOOL b;
	SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
	PSID AdministratorsGroup;

	b = AllocateAndInitializeSid(
		&NtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&AdministratorsGroup); 
	if(b) 
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) 
		{
			b = FALSE;
		}
		FreeSid(AdministratorsGroup);
	}
	return(b);
}






//// Undocumented console message
//#define WM_SETCONSOLEINFO			(WM_USER+201)
//
//#pragma pack(push, 1)
//
////
////	Structure to send console via WM_SETCONSOLEINFO
////
//typedef struct _CONSOLE_INFO
//{
//	ULONG		Length;
//	COORD		ScreenBufferSize;
//	COORD		WindowSize;
//	ULONG		WindowPosX;
//	ULONG		WindowPosY;
//
//	COORD		FontSize;
//	ULONG		FontFamily;
//	ULONG		FontWeight;
//	WCHAR		FaceName[32];
//
//	ULONG		CursorSize;
//	ULONG		FullScreen;
//	ULONG		QuickEdit;
//	ULONG		AutoPosition;
//	ULONG		InsertMode;
//
//	USHORT		ScreenColors;
//	USHORT		PopupColors;
//	ULONG		HistoryNoDup;
//	ULONG		HistoryBufferSize;
//	ULONG		NumberOfHistoryBuffers;
//
//	COLORREF	ColorTable[16];
//
//	ULONG		CodePage;
//	HWND		Hwnd;
//
//	WCHAR		ConsoleTitle[0x100];
//
//} CONSOLE_INFO;
//
//CONSOLE_INFO* gpConsoleInfoStr = NULL;
//HANDLE ghConsoleSection = NULL;
//const UINT gnConsoleSectionSize = sizeof(CONSOLE_INFO)+1024;
//
//#pragma pack(pop)
//
//
//
//
//
//
//
//
//
//
//BOOL GetShortFileName(LPCWSTR asFullPath, wchar_t* rsShortName/*name only, MAX_PATH required*/)
//{
//	WIN32_FIND_DATAW fnd; memset(&fnd, 0, sizeof(fnd));
//	HANDLE hFind = FindFirstFile(asFullPath, &fnd);
//	if (hFind == INVALID_HANDLE_VALUE)
//		return FALSE;
//	FindClose(hFind);
//	if (fnd.cAlternateFileName[0]) {
//		if (lstrlenW(fnd.cAlternateFileName) < lstrlenW(fnd.cFileName)) {
//			lstrcpyW(rsShortName, fnd.cAlternateFileName);
//			return TRUE;
//		}
//	}
//
//	return FALSE;
//}
//
//wchar_t* GetShortFileNameEx(LPCWSTR asLong)
//{
//	TODO("хорошо бы и сетевые диски обрабатывать");
//	if (!asLong) return NULL;
//	
//	wchar_t* pszShort = /*_wcsdup(asLong);*/(wchar_t*)malloc((lstrlenW(asLong)+1)*2);
//	lstrcpyW(pszShort, asLong);
//	wchar_t* pszCur = wcschr(pszShort+3, L'\\');
//	wchar_t* pszSlash;
//	wchar_t  szName[MAX_PATH+1];
//	size_t nLen = 0;
//
//	while (pszCur) {
//		*pszCur = 0;
//		{
//			if (GetShortFileName(pszShort, szName)) {
//				if ((pszSlash = wcsrchr(pszShort, L'\\'))==0)
//					goto wrap;
//				lstrcpyW(pszSlash+1, szName);
//				pszSlash += 1+lstrlenW(szName);
//				lstrcpyW(pszSlash+1, pszCur+1);
//				pszCur = pszSlash;
//			}
//		}
//		*pszCur = L'\\';
//		pszCur = wcschr(pszCur+1, L'\\');
//	}
//	nLen = lstrlenW(pszShort);
//	if (nLen>0 && pszShort[nLen-1]==L'\\')
//		pszShort[--nLen] = 0;
//	if (nLen <= MAX_PATH)
//		return pszShort;
//
//wrap:
//	free(pszShort);
//	return NULL;
//}
//
//int NextArg(const wchar_t** asCmdLine, wchar_t* rsArg/*[MAX_PATH+1]*/, const wchar_t** rsArgStart/*=NULL*/)
//{
//    LPCWSTR psCmdLine = *asCmdLine, pch = NULL;
//    wchar_t ch = *psCmdLine;
//    size_t nArgLen = 0;
//    
//    while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);
//    if (ch == 0) return CERR_CMDLINEEMPTY;
//
//    // аргумент начинается с "
//    if (ch == L'"') {
//        psCmdLine++;
//        pch = wcschr(psCmdLine, L'"');
//        if (!pch) return CERR_CMDLINE;
//        while (pch[1] == L'"') {
//            pch += 2;
//            pch = wcschr(pch, L'"');
//            if (!pch) return CERR_CMDLINE;
//        }
//        // Теперь в pch ссылка на последнюю "
//    } else {
//        // До конца строки или до первого пробела
//        //pch = wcschr(psCmdLine, L' ');
//        // 09.06.2009 Maks - обломался на: cmd /c" echo Y "
//        pch = psCmdLine;
//        while (*pch && *pch!=L' ' && *pch!=L'"') pch++;
//        //if (!pch) pch = psCmdLine + lstrlenW(psCmdLine); // до конца строки
//    }
//    
//    nArgLen = pch - psCmdLine;
//    if (nArgLen > MAX_PATH) return CERR_CMDLINE;
//
//    // Вернуть аргумент
//    memcpy(rsArg, psCmdLine, nArgLen*sizeof(wchar_t));
//    if (rsArgStart) *rsArgStart = psCmdLine;
//    rsArg[nArgLen] = 0;
//
//    psCmdLine = pch;
//    
//    // Finalize
//    ch = *psCmdLine; // может указывать на закрывающую кавычку
//    if (ch == L'"') ch = *(++psCmdLine);
//    while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);
//    *asCmdLine = psCmdLine;
//    
//    return 0;
//}
//
//
//BOOL PackInputRecord(const INPUT_RECORD* piRec, MSG* pMsg)
//{
//	_ASSERTE(pMsg!=NULL && piRec!=NULL);
//	memset(pMsg, 0, sizeof(MSG));
//	
//    UINT nMsg = 0; WPARAM wParam = 0; LPARAM lParam = 0;
//    if (piRec->EventType == KEY_EVENT) {
//    	nMsg = piRec->Event.KeyEvent.bKeyDown ? WM_KEYDOWN : WM_KEYUP;
//    	
//		lParam |= (WORD)piRec->Event.KeyEvent.uChar.UnicodeChar;
//		lParam |= ((BYTE)piRec->Event.KeyEvent.wVirtualKeyCode) << 16;
//		lParam |= ((BYTE)piRec->Event.KeyEvent.wVirtualScanCode) << 24;
//		
//        wParam |= (WORD)piRec->Event.KeyEvent.dwControlKeyState;
//        wParam |= ((DWORD)piRec->Event.KeyEvent.wRepeatCount & 0xFF) << 16;
//    
//    } else if (piRec->EventType == MOUSE_EVENT) {
//		switch (piRec->Event.MouseEvent.dwEventFlags) {
//			case MOUSE_MOVED:
//				nMsg = MOUSE_EVENT_MOVE;
//				break;
//			case 0:
//				nMsg = MOUSE_EVENT_CLICK;
//				break;
//			case DOUBLE_CLICK:
//				nMsg = MOUSE_EVENT_DBLCLICK;
//				break;
//			case MOUSE_WHEELED:
//				nMsg = MOUSE_EVENT_WHEELED;
//				break;
//			case /*MOUSE_HWHEELED*/ 0x0008:
//				nMsg = MOUSE_EVENT_HWHEELED;
//				break;
//			default:
//				_ASSERT(FALSE);
//		}
//		
//    	lParam = ((WORD)piRec->Event.MouseEvent.dwMousePosition.X)
//    	       | (((DWORD)(WORD)piRec->Event.MouseEvent.dwMousePosition.Y) << 16);
//		
//		// max 0x0010/*FROM_LEFT_4ND_BUTTON_PRESSED*/
//		wParam |= ((DWORD)piRec->Event.MouseEvent.dwButtonState) & 0xFF;
//		
//		// max - ENHANCED_KEY == 0x0100
//		wParam |= (((DWORD)piRec->Event.MouseEvent.dwControlKeyState) & 0xFFFF) << 8;
//		
//		if (nMsg == MOUSE_EVENT_WHEELED || nMsg == MOUSE_EVENT_HWHEELED) {
//    		// HIWORD() - short (direction[1/-1])*count*120
//    		short nWheel = (short)((((DWORD)piRec->Event.MouseEvent.dwButtonState) & 0xFFFF0000) >> 16);
//    		char  nCount = nWheel / 120;
//    		wParam |= ((DWORD)(BYTE)nCount) << 24;
//		}
//		
//    
//    } else if (piRec->EventType == FOCUS_EVENT) {
//    	nMsg = piRec->Event.FocusEvent.bSetFocus ? WM_SETFOCUS : WM_KILLFOCUS;
//    	
//    } else {
//    	_ASSERT(FALSE);
//    	return FALSE;
//    }
//    _ASSERTE(nMsg!=0);
//    
//    
//    pMsg->message = nMsg;
//    pMsg->wParam = wParam;
//    pMsg->lParam = lParam;
//    
//    return TRUE;
//}
//
//BOOL UnpackInputRecord(const MSG* piMsg, INPUT_RECORD* pRec)
//{
//	_ASSERTE(piMsg!=NULL && pRec!=NULL);
//	memset(pRec, 0, sizeof(INPUT_RECORD));
//
//	if (piMsg->message == 0)
//		return FALSE;
//	
//	if (piMsg->message == WM_KEYDOWN || piMsg->message == WM_KEYUP) {
//		pRec->EventType = KEY_EVENT;
//		
//		// lParam
//        pRec->Event.KeyEvent.bKeyDown = (piMsg->message == WM_KEYDOWN);
//        pRec->Event.KeyEvent.uChar.UnicodeChar = (WCHAR)(piMsg->lParam & 0xFFFF);
//        pRec->Event.KeyEvent.wVirtualKeyCode   = (((DWORD)piMsg->lParam) & 0xFF0000) >> 16;
//        pRec->Event.KeyEvent.wVirtualScanCode  = (((DWORD)piMsg->lParam) & 0xFF000000) >> 24;
//        
//        // wParam. Пока что тут может быть max(ENHANCED_KEY==0x0100)
//        pRec->Event.KeyEvent.dwControlKeyState = ((DWORD)piMsg->wParam & 0xFFFF);
//        
//        pRec->Event.KeyEvent.wRepeatCount = ((DWORD)piMsg->wParam & 0xFF0000) >> 16;
//        
//	} else if (piMsg->message >= MOUSE_EVENT_FIRST && piMsg->message <= MOUSE_EVENT_LAST) {
//		pRec->EventType = MOUSE_EVENT;
//		
//		switch (piMsg->message) {
//			case MOUSE_EVENT_MOVE:
//				pRec->Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
//				break;
//			case MOUSE_EVENT_CLICK:
//				pRec->Event.MouseEvent.dwEventFlags = 0;
//				break;
//			case MOUSE_EVENT_DBLCLICK:
//				pRec->Event.MouseEvent.dwEventFlags = DOUBLE_CLICK;
//				break;
//			case MOUSE_EVENT_WHEELED:
//				pRec->Event.MouseEvent.dwEventFlags = MOUSE_WHEELED;
//				break;
//			case MOUSE_EVENT_HWHEELED:
//				pRec->Event.MouseEvent.dwEventFlags = /*MOUSE_HWHEELED*/ 0x0008;
//				break;
//		}
//		
//		pRec->Event.MouseEvent.dwMousePosition.X = LOWORD(piMsg->lParam);
//		pRec->Event.MouseEvent.dwMousePosition.Y = HIWORD(piMsg->lParam);
//		
//		// max 0x0010/*FROM_LEFT_4ND_BUTTON_PRESSED*/
//		pRec->Event.MouseEvent.dwButtonState = ((DWORD)piMsg->wParam) & 0xFF;
//		
//		// max - ENHANCED_KEY == 0x0100
//		pRec->Event.MouseEvent.dwControlKeyState = (((DWORD)piMsg->wParam) & 0xFFFF00) >> 8;
//		
//		if (piMsg->message == MOUSE_EVENT_WHEELED || piMsg->message == MOUSE_EVENT_HWHEELED) {
//    		// HIWORD() - short (direction[1/-1])*count*120
//    		signed char nDir = (signed char)((((DWORD)piMsg->wParam) & 0xFF000000) >> 24);
//    		WORD wDir = nDir*120;
//    		pRec->Event.MouseEvent.dwButtonState |= wDir << 16;
//		}
//		
//	} else if (piMsg->message == WM_SETFOCUS || piMsg->message == WM_KILLFOCUS) {
//        pRec->EventType = FOCUS_EVENT;
//        
//        pRec->Event.FocusEvent.bSetFocus = (piMsg->message == WM_SETFOCUS);
//        
//	} else {
//		return FALSE;
//	}
//	
//	return TRUE;
//}
//
//class MNullDesc {
//protected:
//	PSECURITY_DESCRIPTOR mp_NullDesc;
//	SECURITY_ATTRIBUTES  m_NullSecurity;
//public:
//	DWORD mn_LastError;
//public:
//	MNullDesc() {
//		memset(&m_NullSecurity, 0, sizeof(m_NullSecurity));
//		mp_NullDesc = NULL;
//		mn_LastError = 0;
//	};
//	~MNullDesc() {
//		memset(&m_NullSecurity, 0, sizeof(m_NullSecurity));
//		LocalFree(mp_NullDesc); mp_NullDesc = NULL;
//	};
//public:
//	SECURITY_ATTRIBUTES* NullSecurity() {
//		mn_LastError = 0;
//		
//		if (mp_NullDesc) {
//			_ASSERTE(m_NullSecurity.lpSecurityDescriptor==mp_NullDesc);
//			return (&m_NullSecurity);
//		}
//		mp_NullDesc = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
//		      SECURITY_DESCRIPTOR_MIN_LENGTH); 
//
//		if (mp_NullDesc == NULL) {
//			mn_LastError = GetLastError();
//			return NULL;
//		}
//
//		if (!InitializeSecurityDescriptor(mp_NullDesc, SECURITY_DESCRIPTOR_REVISION)) {
//			mn_LastError = GetLastError();
//			LocalFree(mp_NullDesc); mp_NullDesc = NULL;
//			return NULL;
//		}
//
//		// Add a null DACL to the security descriptor. 
//		if (!SetSecurityDescriptorDacl(mp_NullDesc, TRUE, (PACL) NULL, FALSE)) {
//			mn_LastError = GetLastError();
//			LocalFree(mp_NullDesc); mp_NullDesc = NULL;
//			return NULL;
//		}
//
//		m_NullSecurity.nLength = sizeof(m_NullSecurity);
//		m_NullSecurity.lpSecurityDescriptor = mp_NullDesc;
//		m_NullSecurity.bInheritHandle = TRUE; 
//		
//		return (&m_NullSecurity);
//	};
//};
//MNullDesc *gNullDesc = NULL;
//
//SECURITY_ATTRIBUTES* NullSecurity()
//{
//	if (!gNullDesc) gNullDesc = new MNullDesc();
//	return gNullDesc->NullSecurity();
//}
//
//void CommonShutdown()
//{
//	if (gNullDesc) {
//		delete gNullDesc;
//		gNullDesc = NULL;
//	}
//
//	// Clean memory
//	if (ghConsoleSection) {
//		CloseHandle(ghConsoleSection);
//		ghConsoleSection = NULL;
//	}
//	if (gpConsoleInfoStr) {
//		free(gpConsoleInfoStr);
//		gpConsoleInfoStr = NULL;
//	}
//}
//
//BOOL IsUserAdmin()
//{
//	OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)};
//	GetVersionEx(&osv);
//	// Проверять нужно только для висты, чтобы на XP лишний "Щит" не отображался
//	if (osv.dwMajorVersion < 6)
//		return FALSE;
//
//	BOOL b;
//	SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
//	PSID AdministratorsGroup;
//
//	b = AllocateAndInitializeSid(
//		&NtAuthority,
//		2,
//		SECURITY_BUILTIN_DOMAIN_RID,
//		DOMAIN_ALIAS_RID_ADMINS,
//		0, 0, 0, 0, 0, 0,
//		&AdministratorsGroup); 
//	if(b) 
//	{
//		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) 
//		{
//			b = FALSE;
//		}
//		FreeSid(AdministratorsGroup);
//	}
//	return(b);
//}
//


MConHandle::MConHandle(LPCWSTR asName)
{
	mn_StdMode = 0;
	mb_OpenFailed = FALSE; mn_LastError = 0;
	mh_Handle = INVALID_HANDLE_VALUE;
	lstrcpynW(ms_Name, asName, 9);

	/*
FAR2 последний
Conemu последний

без плагов каких либо

пропиши одну ассоциацию

[HKEY_CURRENT_USER\Software\Far2\Associations\Type0]
"Mask"="*.ini"
"Description"=""
"Execute"="@"
"AltExec"=""
"View"=""
"AltView"=""
"Edit"=""
"AltEdit"=""
"State"=dword:0000003f

ФАР валится по двойному щелчку на INI файле. По Enter - не валится.


1:31:11.647 Mouse: {10x15} Btns:{L} KeyState: 0x00000000 |DOUBLE_CLICK
CECMD_CMDSTARTED(Cols=80, Rows=25, Buf=1000, Top=-1)
SetConsoleSizeSrv.Not waiting for ApplyFinished
SyncWindowToConsole(Cols=80, Rows=22)
Current size: Cols=80, Buf=1000
CECMD_CMDFINISHED(Cols=80, Rows=22, Buf=0, Top=-1)
SetConsoleSizeSrv.Not waiting for ApplyFinished
1:31:11.878 Mouse: {10x15} Btns:{} KeyState: 0x00000000 
Current size: Cols=80, Rows=22
1:31:11.906 Mouse: {10x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
1:31:11.949 Mouse: {10x15} Btns:{L} KeyState: 0x00000000 |DOUBLE_CLICK
CECMD_CMDSTARTED(Cols=80, Rows=22, Buf=1000, Top=-1)
SetConsoleSizeSrv.Not waiting for ApplyFinished
Current size: Cols=80, Buf=1000
CECMD_CMDFINISHED(Cols=80, Rows=22, Buf=0, Top=-1)
SetConsoleSizeSrv.Not waiting for ApplyFinished
1:31:12.163 Mouse: {10x15} Btns:{} KeyState: 0x00000000 
1:31:12.196 Mouse: {10x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
Current size: Cols=80, Rows=22
1:31:12.532 Mouse: {11x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
1:31:12.545 Mouse: {13x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
1:31:12.573 Mouse: {14x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
1:31:12.686 Mouse: {15x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
1:31:12.779 Mouse: {16x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
1:31:12.859 Mouse: {16x15} Btns:{L} KeyState: 0x00000000 
1:31:12.876 Mouse: {16x15} Btns:{L} KeyState: 0x00000000 |MOUSE_MOVED
1:31:12.944 Mouse: {16x15} Btns:{} KeyState: 0x00000000 
1:31:12.956 Mouse: {16x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
1:31:13.010 Mouse: {16x15} Btns:{L} KeyState: 0x00000000 |DOUBLE_CLICK
CECMD_CMDSTARTED(Cols=80, Rows=22, Buf=1000, Top=-1)
SetConsoleSizeSrv.Not waiting for ApplyFinished
SyncWindowToConsole(Cols=80, Rows=19)
Current size: Cols=80, Buf=1000
CECMD_CMDFINISHED(Cols=80, Rows=19, Buf=0, Top=-1)
1:31:13.150 Mouse: {16x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
1:31:13.175 Mouse: {16x15} Btns:{L} KeyState: 0x00000000 
1:31:13.206 Mouse: {16x15} Btns:{L} KeyState: 0x00000000 |MOUSE_MOVED
SetConsoleSizeSrv.Not waiting for ApplyFinished
1:31:13.240 Mouse: {16x15} Btns:{} KeyState: 0x00000000 
Current size: Cols=80, Rows=191:31:13.317 Mouse: {16x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
1:31:13.357 Mouse: {16x15} Btns:{L} KeyState: 0x00000000 |DOUBLE_CLICK


1:31:11 CurSize={80x25} ChangeTo={80x25} RefreshThread :CECMD_SETSIZESYNC
1:31:11 CurSize={80x1000} ChangeTo={80x25} RefreshThread :CECMD_SETSIZESYNC
1:31:11 CurSize={80x25} ChangeTo={80x25} RefreshThread :CECMD_SETSIZESYNC
1:31:11 CurSize={80x1000} ChangeTo={80x22} RefreshThread :CECMD_SETSIZESYNC
1:31:12 CurSize={80x22} ChangeTo={80x22} RefreshThread :CECMD_SETSIZESYNC
1:31:12 CurSize={80x1000} ChangeTo={80x22} RefreshThread :CECMD_SETSIZESYNC
1:31:13 CurSize={80x22} ChangeTo={80x22} RefreshThread :CECMD_SETSIZESYNC
1:31:13 CurSize={80x1000} ChangeTo={80x19} RefreshThread :CECMD_SETSIZESYNC
1:31:15 CurSize={80x19} ChangeTo={80x19} RefreshThread :CECMD_SETSIZESYNC
1:31:15 CurSize={80x1000} ChangeTo={80x19} RefreshThread :CECMD_SETSIZESYNC
1:31:16 CurSize={80x19} ChangeTo={80x19} RefreshThread :CECMD_SETSIZESYNC
1:31:16 CurSize={80x1000} ChangeTo={80x19} RefreshThread :CECMD_SETSIZESYNC
1:31:16 CurSize={80x19} ChangeTo={80x19} RefreshThread :CECMD_SETSIZESYNC
1:31:16 CurSize={80x19} ChangeTo={80x19} RefreshThread :CECMD_SETSIZESYNC
1:31:16 CurSize={80x1000} ChangeTo={80x16} RefreshThread :CECMD_SETSIZESYNC
1:31:25 CurSize={80x16} ChangeTo={80x16} RefreshThread :CECMD_SETSIZESYNC
1:31:25 CurSize={80x1000} ChangeTo={80x16} RefreshThread :CECMD_SETSIZESYNC




    */
	//OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)}; GetVersionEx(&osv);
	//if (osv.dwMajorVersion == 6 && osv.dwMinorVersion == 1) {
	//	if (!lstrcmpW(ms_Name, L"CONOUT$"))
	//		mn_StdMode = STD_OUTPUT_HANDLE;
	//	else if (!lstrcmpW(ms_Name, L"CONIN$"))
	//		mn_StdMode = STD_INPUT_HANDLE;
	//}
};

MConHandle::~MConHandle()
{
	Close();
};

MConHandle::operator const HANDLE()
{
	if (mh_Handle == INVALID_HANDLE_VALUE)
	{
		if (mn_StdMode) {
			mh_Handle = GetStdHandle(mn_StdMode);
			return mh_Handle;
		}

		// Чтобы случайно не открыть хэндл несколько раз в разных потоках
		MSectionLock CS; CS.Lock(&mcs_Handle, TRUE);
		// Во время ожидания хэндл мог быт открыт в другом потоке
		if (mh_Handle == INVALID_HANDLE_VALUE) {
			mh_Handle = CreateFileW(ms_Name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
				0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if (mh_Handle != INVALID_HANDLE_VALUE) {
				mb_OpenFailed = FALSE;
			} else {
				mn_LastError = GetLastError();
				if (!mb_OpenFailed) {
					mb_OpenFailed = TRUE; // чтобы ошибка вываливалась только один раз!
					char szErrMsg[512], szNameA[10], szSelfFull[MAX_PATH];
					const char *pszSelf;
					char *pszDot;
					if (!GetModuleFileNameA(0,szSelfFull,MAX_PATH)) {
						pszSelf = "???";
					} else {
						pszSelf = strrchr(szSelfFull, '\\');
						if (pszSelf) pszSelf++; else pszSelf = szSelfFull;
						pszDot = strrchr((char*)pszSelf, '.');
						if (pszDot) *pszDot = 0;
					}
					WideCharToMultiByte(CP_OEMCP, 0, ms_Name, -1, szNameA, sizeof(szNameA), 0,0);
					wsprintfA(szErrMsg, "%s: CreateFile(%s) failed, ErrCode=0x%08X\n", pszSelf, szNameA, mn_LastError); 
					HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
					if (h && h!=INVALID_HANDLE_VALUE) {
						DWORD dwWritten = 0;
						WriteFile(h, szErrMsg, lstrlenA(szErrMsg), &dwWritten, 0);
					}
				}
			}
		}
	}
	return mh_Handle;
};

void MConHandle::Close()
{
	if (mh_Handle != INVALID_HANDLE_VALUE) {
		if (mn_StdMode) {
			mh_Handle = INVALID_HANDLE_VALUE;
		} else {
			HANDLE h = mh_Handle;
			mh_Handle = INVALID_HANDLE_VALUE;
			mb_OpenFailed = FALSE;
			CloseHandle(h);
		}
	}
};





MEvent::MEvent()
{
	ms_EventName[0] = 0;
	mh_Event = NULL;
	mn_LastError = 0;
}

MEvent::~MEvent()
{
	if (mh_Event)
		Close();
}

void MEvent::Close()
{
	mn_LastError = 0;
	ms_EventName[0] = 0;

	if (mh_Event) {
		CloseHandle(mh_Event);
		mh_Event = NULL;
	}
}

void MEvent::InitName(const wchar_t *aszTemplate, DWORD Parm1)
{
	//va_list args;
	//va_start( args, aszTemplate );
	//vswprintf_s(ms_EventName,countof(ms_EventName),aszTemplate,args);
	wsprintfW(ms_EventName, aszTemplate, Parm1);
	if (mh_Event)
		Close();
	mn_LastError = 0;
}

HANDLE MEvent::Open()
{
	if (mh_Event) // Если уже открыто - сразу вернуть!
		return mh_Event;

	if (ms_EventName[0] == 0) {
		_ASSERTE(ms_EventName[0]!=0);
		return NULL;
	}

	mn_LastError = 0;
	mh_Event = OpenEvent(EVENT_MODIFY_STATE|SYNCHRONIZE, FALSE, ms_EventName);
	if (!mh_Event)
		mn_LastError = GetLastError();

	return mh_Event;
}

DWORD MEvent::Wait(DWORD dwMilliseconds, BOOL abAutoOpen/*=TRUE*/)
{
	if (!mh_Event && abAutoOpen)
		Open();
	if (!mh_Event)
		return WAIT_ABANDONED;

	DWORD dwWait = WaitForSingleObject(mh_Event, dwMilliseconds);
	#ifdef _DEBUG
	mn_LastError = GetLastError();
	#endif

	return dwWait;
}

HANDLE MEvent::GetHandle()
{
	return mh_Event;
}












////
////	SETCONSOLEINFO.C
////
////	Undocumented method to set console attributes at runtime
////
////	For Vista use the newly documented SetConsoleScreenBufferEx API
////
////	www.catch22.net
////
//
////
////	Wrapper around WM_SETCONSOLEINFO. We need to create the
////  necessary section (file-mapping) object in the context of the
////  process which owns the console, before posting the message
////
//BOOL SetConsoleInfo(HWND hwndConsole, CONSOLE_INFO *pci)
//{
//	DWORD   dwConsoleOwnerPid, dwCurProcId, dwConsoleThreadId;
//	PVOID   ptrView = 0;
//	DWORD   dwLastError=0;
//	WCHAR   ErrText[255];
//
//	//
//	//	Retrieve the process which "owns" the console
//	//	
//	dwCurProcId = GetCurrentProcessId();
//	dwConsoleThreadId = GetWindowThreadProcessId(hwndConsole, &dwConsoleOwnerPid);
//
//	// We'll fail, if console was created by other process
//	if (dwConsoleOwnerPid != dwCurProcId) {
//		_ASSERTE(dwConsoleOwnerPid == dwCurProcId);
//		return FALSE;
//	}
//
//
//	//
//	// Create a SECTION object backed by page-file, then map a view of
//	// this section into the owner process so we can write the contents 
//	// of the CONSOLE_INFO buffer into it
//	//
//	if (!ghConsoleSection) {
//		ghConsoleSection = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, gnConsoleSectionSize, 0);
//
//		if (!ghConsoleSection) {
//			dwLastError = GetLastError();
//			wsprintf(ErrText, L"Can't CreateFileMapping(ghConsoleSection). ErrCode=%i", dwLastError);
//			MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
//			return FALSE;
//		}
//	}
//
//
//	//
//	//	Copy our console structure into the section-object
//	//
//	ptrView = MapViewOfFile(ghConsoleSection, FILE_MAP_WRITE|FILE_MAP_READ, 0, 0, gnConsoleSectionSize);
//	if (!ptrView) {
//		dwLastError = GetLastError();
//		wsprintf(ErrText, L"Can't MapViewOfFile. ErrCode=%i", dwLastError);
//		MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
//
//	} else {
//		_ASSERTE(pci->Length==sizeof(CONSOLE_INFO));
//		memcpy(ptrView, pci, pci->Length);
//
//		UnmapViewOfFile(ptrView);
//
//		//  Send console window the "update" message
//		LRESULT dwConInfoRc = 0;
//		DWORD dwConInfoErr = 0;
//
//		dwConInfoRc = SendMessage(hwndConsole, WM_SETCONSOLEINFO, (WPARAM)ghConsoleSection, 0);
//		dwConInfoErr = GetLastError();
//	}
//
//	return TRUE;
//}
//
////
////	Fill the CONSOLE_INFO structure with information
////  about the current console window
////
//void GetConsoleSizeInfo(CONSOLE_INFO *pci)
//{
//	CONSOLE_SCREEN_BUFFER_INFO csbi;
//
//	HANDLE hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
//
//	BOOL lbRc = GetConsoleScreenBufferInfo(hConsoleOut, &csbi);
//	
//	if (lbRc) {
//		pci->ScreenBufferSize = csbi.dwSize;
//		pci->WindowSize.X	  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
//		pci->WindowSize.Y	  = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
//		// Было... а это координаты окна (хотя включается флажок "Autoposition"
//		//pci->WindowPosX	      = csbi.srWindow.Left;
//		//pci->WindowPosY		  = csbi.srWindow.Top;
//	} else {
//		_ASSERTE(lbRc);
//		pci->ScreenBufferSize.X = pci->WindowSize.X = 80;
//		pci->ScreenBufferSize.Y = pci->WindowSize.Y = 25;
//	}
//
//	// Поскольку включен флажок "AutoPosition" - то это игнорируется
//	pci->WindowPosX = pci->WindowPosY = 0;
//
//	/*
//	RECT rcWnd = {0}; GetWindowRect(GetConsoleWindow(), &rcWnd);
//	pci->WindowPosX	      = rcWnd.left;
//	pci->WindowPosY		  = rcWnd.top;
//	*/
//}
//
//
//#if defined(__GNUC__)
//#define __in
//#define __out
//#undef ENABLE_AUTO_POSITION
//#endif
//
////VISTA support:
//#ifndef ENABLE_AUTO_POSITION
//typedef struct _CONSOLE_FONT_INFOEX {
//	ULONG cbSize;
//	DWORD nFont;
//	COORD dwFontSize;
//	UINT FontFamily;
//	UINT FontWeight;
//	WCHAR FaceName[LF_FACESIZE];
//} CONSOLE_FONT_INFOEX, *PCONSOLE_FONT_INFOEX;
//#endif
//
//
//typedef BOOL (WINAPI *PGetCurrentConsoleFontEx)(__in HANDLE hConsoleOutput,__in BOOL bMaximumWindow,__out PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx);
//typedef BOOL (WINAPI *PSetCurrentConsoleFontEx)(__in HANDLE hConsoleOutput,__in BOOL bMaximumWindow,__out PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx);
//
//
//void SetConsoleFontSizeTo(HWND inConWnd, int inSizeY, int inSizeX, const wchar_t *asFontName)
//{
//
//
//	HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
//	if (!hKernel)
//		return;
//	PGetCurrentConsoleFontEx GetCurrentConsoleFontEx = (PGetCurrentConsoleFontEx)
//		GetProcAddress(hKernel, "GetCurrentConsoleFontEx");
//	PSetCurrentConsoleFontEx SetCurrentConsoleFontEx = (PSetCurrentConsoleFontEx)
//		GetProcAddress(hKernel, "SetCurrentConsoleFontEx");
//
//	if (GetCurrentConsoleFontEx && SetCurrentConsoleFontEx) // We have Vista
//	{
//		CONSOLE_FONT_INFOEX cfi = {sizeof(CONSOLE_FONT_INFOEX)};
//		//GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
//		cfi.dwFontSize.X = inSizeX;
//		cfi.dwFontSize.Y = inSizeY;
//		lstrcpynW(cfi.FaceName, asFontName ? asFontName : L"Lucida Console", LF_FACESIZE);
//		SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
//	}
//	else // We have other NT
//	{
//		const COLORREF DefaultColors[16] = 
//		{
//			0x00000000, 0x00800000, 0x00008000, 0x00808000,
//			0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0, 
//			0x00808080,	0x00ff0000, 0x0000ff00, 0x00ffff00,
//			0x000000ff, 0x00ff00ff,	0x0000ffff, 0x00ffffff
//		};
//
//		if (!gpConsoleInfoStr) {
//			gpConsoleInfoStr = (CONSOLE_INFO*)calloc(sizeof(CONSOLE_INFO),1);
//			if (!gpConsoleInfoStr) {
//				_ASSERTE(gpConsoleInfoStr!=NULL);
//				return; // memory allocation failed
//			}
//			gpConsoleInfoStr->Length = sizeof(CONSOLE_INFO);
//		}
//		int i;
//
//		// get current size/position settings rather than using defaults..
//		GetConsoleSizeInfo(gpConsoleInfoStr);
//
//		// set these to zero to keep current settings
//		gpConsoleInfoStr->FontSize.X				= inSizeX;
//		gpConsoleInfoStr->FontSize.Y				= inSizeY;
//		gpConsoleInfoStr->FontFamily				= 0;//0x30;//FF_MODERN|FIXED_PITCH;//0x30;
//		gpConsoleInfoStr->FontWeight				= 0;//0x400;
//		lstrcpynW(gpConsoleInfoStr->FaceName, asFontName ? asFontName : L"Lucida Console", 32);
//
//		gpConsoleInfoStr->CursorSize				= 25;
//		gpConsoleInfoStr->FullScreen				= FALSE;
//		gpConsoleInfoStr->QuickEdit					= FALSE;
//		gpConsoleInfoStr->AutoPosition				= 0x10000;
//		gpConsoleInfoStr->InsertMode				= TRUE;
//		gpConsoleInfoStr->ScreenColors				= MAKEWORD(0x7, 0x0);
//		gpConsoleInfoStr->PopupColors				= MAKEWORD(0x5, 0xf);
//
//		gpConsoleInfoStr->HistoryNoDup				= FALSE;
//		gpConsoleInfoStr->HistoryBufferSize			= 50;
//		gpConsoleInfoStr->NumberOfHistoryBuffers	= 4;
//
//		// color table
//		for(i = 0; i < 16; i++)
//			gpConsoleInfoStr->ColorTable[i] = DefaultColors[i];
//
//		gpConsoleInfoStr->CodePage					= GetConsoleOutputCP();//0;//0x352;
//		gpConsoleInfoStr->Hwnd						= inConWnd;
//
//		gpConsoleInfoStr->ConsoleTitle[0] = 0;
//		
//
//		SetConsoleInfo(inConWnd, gpConsoleInfoStr);
//	}
//}
///*
//-- пробовал в Win7 это не работает
//void SetConsoleBufferSize(HWND inConWnd, int anWidth, int anHeight, int anBufferHeight)
//{
//	if (!gpConsoleInfoStr) {
//		_ASSERTE(gpConsoleInfoStr!=NULL);
//		return; // memory allocation failed
//	}
//
//	TODO("Заполнить и другие текущие значения!");
//	gpConsoleInfoStr->CodePage					= GetConsoleOutputCP();//0;//0x352;
//
//	// Теперь собственно, что хотим поменять
//	gpConsoleInfoStr->ScreenBufferSize.X = gpConsoleInfoStr->WindowSize.X = anWidth;
//	gpConsoleInfoStr->WindowSize.Y = anHeight;
//	gpConsoleInfoStr->ScreenBufferSize.Y = anBufferHeight;
//
//	SetConsoleInfo(inConWnd, gpConsoleInfoStr);
//}
//*/





MSetter::MSetter(BOOL* st) :
	mp_BoolVal(NULL), mdw_DwordVal(NULL)
{
	type = st_BOOL;
	mp_BoolVal = st;
	if (mp_BoolVal) *mp_BoolVal = TRUE;
}
MSetter::MSetter(DWORD* st, DWORD setValue) :
	mp_BoolVal(NULL), mdw_DwordVal(NULL)
{
	type = st_DWORD; mdw_OldDwordValue = *st; *st = setValue;
	mdw_DwordVal = st;
}
MSetter::~MSetter() {
	Unlock();
}
void MSetter::Unlock() {
	if (type==st_BOOL) {
		if (mp_BoolVal) *mp_BoolVal = FALSE;
	} else
	if (type==st_DWORD) {
		if (mdw_DwordVal) *mdw_DwordVal = mdw_OldDwordValue;
	}
}
