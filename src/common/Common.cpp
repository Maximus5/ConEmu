
#include <windows.h>
#include "common.hpp"

BOOL GetShortFileName(LPCWSTR asFullPath, wchar_t* rsShortName/*name only, MAX_PATH required*/)
{
	WIN32_FIND_DATAW fnd; memset(&fnd, 0, sizeof(fnd));
	HANDLE hFind = FindFirstFile(asFullPath, &fnd);
	if (hFind == INVALID_HANDLE_VALUE)
		return FALSE;
	FindClose(hFind);
	if (fnd.cAlternateFileName[0]) {
		if (wcslen(fnd.cAlternateFileName) < wcslen(fnd.cFileName)) {
			lstrcpyW(rsShortName, fnd.cAlternateFileName);
			return TRUE;
		}
	}

	return FALSE;
}

wchar_t* GetShortFileNameEx(LPCWSTR asLong)
{
	TODO("хорошо бы и сетевые диски обрабатывать");

	wchar_t* pszShort = wcsdup(asLong);
	wchar_t* pszCur = wcschr(pszShort+3, L'\\');
	wchar_t* pszSlash;
	wchar_t  szName[MAX_PATH+1];
	int nLen = 0;

	while (pszCur) {
		*pszCur = 0;
		{
			if (GetShortFileName(pszShort, szName)) {
				if ((pszSlash = wcsrchr(pszShort, L'\\'))==0)
					goto wrap;
				wcscpy(pszSlash+1, szName);
				pszSlash += 1+wcslen(szName);
				wcscpy(pszSlash+1, pszCur+1);
				pszCur = pszSlash;
			}
		}
		*pszCur = L'\\';
		pszCur = wcschr(pszCur+1, L'\\');
	}
	nLen = wcslen(pszShort);
	if (nLen>0 && pszShort[nLen-1]==L'\\')
		pszShort[--nLen] = 0;
	if (nLen <= MAX_PATH)
		return pszShort;

wrap:
	free(pszShort);
	return NULL;
}

int NextArg(const wchar_t** asCmdLine, wchar_t* rsArg/*[MAX_PATH+1]*/)
{
    LPCWSTR psCmdLine = *asCmdLine, pch = NULL;
    wchar_t ch = *psCmdLine;
    int nArgLen = 0;
    
    while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);
    if (ch == 0) return CERR_CMDLINEEMPTY;

    // аргумент начинаетс€ с "
    if (ch == L'"') {
        psCmdLine++;
        pch = wcschr(psCmdLine, L'"');
        if (!pch) return CERR_CMDLINE;
        while (pch[1] == L'"') {
            pch += 2;
            pch = wcschr(pch, L'"');
            if (!pch) return CERR_CMDLINE;
        }
        // “еперь в pch ссылка на последнюю "
    } else {
        // ƒо конца строки или до первого пробела
        //pch = wcschr(psCmdLine, L' ');
        // 09.06.2009 Maks - обломалс€ на: cmd /c" echo Y "
        pch = psCmdLine;
        while (*pch && *pch!=L' ' && *pch!=L'"') pch++;
        //if (!pch) pch = psCmdLine + wcslen(psCmdLine); // до конца строки
    }
    
    nArgLen = pch - psCmdLine;
    if (nArgLen > MAX_PATH) return CERR_CMDLINE;

    // ¬ернуть аргумент
    memcpy(rsArg, psCmdLine, nArgLen*sizeof(wchar_t));
    rsArg[nArgLen] = 0;

    psCmdLine = pch;
    
    // Finalize
    ch = *psCmdLine; // может указывать на закрывающую кавычку
    if (ch == L'"') ch = *(++psCmdLine);
    while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);
    *asCmdLine = psCmdLine;
    
    return 0;
}


BOOL PackInputRecord(const INPUT_RECORD* piRec, MSG* pMsg)
{
	_ASSERTE(pMsg!=NULL && piRec!=NULL);
	memset(pMsg, 0, sizeof(MSG));
	
    UINT nMsg = 0; WPARAM wParam = 0; LPARAM lParam = 0;
    if (piRec->EventType == KEY_EVENT) {
    	nMsg = piRec->Event.KeyEvent.bKeyDown ? WM_KEYDOWN : WM_KEYUP;
    	
		lParam |= (WORD)piRec->Event.KeyEvent.uChar.UnicodeChar;
		lParam |= ((BYTE)piRec->Event.KeyEvent.wVirtualKeyCode) << 16;
		lParam |= ((BYTE)piRec->Event.KeyEvent.wVirtualScanCode) << 24;
		
        wParam |= (WORD)piRec->Event.KeyEvent.dwControlKeyState;
        wParam |= ((DWORD)piRec->Event.KeyEvent.wRepeatCount & 0xFF) << 16;
    
    } else if (piRec->EventType == MOUSE_EVENT) {
		switch (piRec->Event.MouseEvent.dwEventFlags) {
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
		
		if (nMsg == MOUSE_EVENT_WHEELED || nMsg == MOUSE_EVENT_HWHEELED) {
    		// HIWORD() - short (direction[1/-1])*count*120
    		short nWheel = (short)((((DWORD)piRec->Event.MouseEvent.dwButtonState) & 0xFFFF0000) >> 16);
    		char  nCount = nWheel / 120;
    		wParam |= ((DWORD)(BYTE)nCount) << 24;
		}
		
    
    } else if (piRec->EventType == FOCUS_EVENT) {
    	nMsg = piRec->Event.FocusEvent.bSetFocus ? WM_SETFOCUS : WM_KILLFOCUS;
    	
    } else {
    	_ASSERT(FALSE);
    	return FALSE;
    }
    _ASSERTE(nMsg!=0);
    
    
    pMsg->message = nMsg;
    pMsg->wParam = wParam;
    pMsg->lParam = lParam;
    
    return TRUE;
}

BOOL UnpackInputRecord(const MSG* piMsg, INPUT_RECORD* pRec)
{
	_ASSERTE(piMsg!=NULL && pRec!=NULL);
	memset(pRec, 0, sizeof(INPUT_RECORD));
	
	if (piMsg->message == WM_KEYDOWN || piMsg->message == WM_KEYUP) {
		pRec->EventType = KEY_EVENT;
		
		// lParam
        pRec->Event.KeyEvent.bKeyDown = (piMsg->message == WM_KEYDOWN);
        pRec->Event.KeyEvent.uChar.UnicodeChar = (WCHAR)(piMsg->lParam & 0xFFFF);
        pRec->Event.KeyEvent.wVirtualKeyCode   = (((DWORD)piMsg->lParam) & 0xFF0000) >> 16;
        pRec->Event.KeyEvent.wVirtualScanCode  = (((DWORD)piMsg->lParam) & 0xFF000000) >> 24;
        
        // wParam. ѕока что тут может быть max(ENHANCED_KEY==0x0100)
        pRec->Event.KeyEvent.dwControlKeyState = ((DWORD)piMsg->wParam & 0xFFFF);
        
        pRec->Event.KeyEvent.wRepeatCount = ((DWORD)piMsg->wParam & 0xFF0000) >> 16;
        
	} else if (piMsg->message >= MOUSE_EVENT_FIRST && piMsg->message <= MOUSE_EVENT_LAST) {
		pRec->EventType = MOUSE_EVENT;
		
		switch (piMsg->message) {
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
		
		if (piMsg->message == MOUSE_EVENT_WHEELED || piMsg->message == MOUSE_EVENT_HWHEELED) {
    		// HIWORD() - short (direction[1/-1])*count*120
    		signed char nDir = (signed char)((((DWORD)piMsg->wParam) & 0xFF000000) >> 24);
    		WORD wDir = nDir*120;
    		pRec->Event.MouseEvent.dwButtonState |= wDir << 16;
		}
		
	} else if (piMsg->message == WM_SETFOCUS || piMsg->message == WM_KILLFOCUS) {
        pRec->EventType = FOCUS_EVENT;
        
        pRec->Event.FocusEvent.bSetFocus = (piMsg->message == WM_SETFOCUS);
        
	} else {
		return FALSE;
	}
	
	return TRUE;
}

class MNullDesc {
protected:
	PSECURITY_DESCRIPTOR mp_NullDesc;
	SECURITY_ATTRIBUTES  m_NullSecurity;
public:
	DWORD mn_LastError;
public:
	MNullDesc() {
		memset(&m_NullSecurity, 0, sizeof(m_NullSecurity));
		mp_NullDesc = NULL;
		mn_LastError = 0;
	};
	~MNullDesc() {
		memset(&m_NullSecurity, 0, sizeof(m_NullSecurity));
		LocalFree(mp_NullDesc); mp_NullDesc = NULL;
	};
public:
	SECURITY_ATTRIBUTES* NullSecurity() {
		mn_LastError = 0;
		
		if (mp_NullDesc) {
			_ASSERTE(m_NullSecurity.lpSecurityDescriptor==mp_NullDesc);
			return (&m_NullSecurity);
		}
		mp_NullDesc = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
		      SECURITY_DESCRIPTOR_MIN_LENGTH); 

		if (mp_NullDesc == NULL) {
			mn_LastError = GetLastError();
			return NULL;
		}

		if (!InitializeSecurityDescriptor(mp_NullDesc, SECURITY_DESCRIPTOR_REVISION)) {
			mn_LastError = GetLastError();
			LocalFree(mp_NullDesc); mp_NullDesc = NULL;
			return NULL;
		}

		// Add a null DACL to the security descriptor. 
		if (!SetSecurityDescriptorDacl(mp_NullDesc, TRUE, (PACL) NULL, FALSE)) {
			mn_LastError = GetLastError();
			LocalFree(mp_NullDesc); mp_NullDesc = NULL;
			return NULL;
		}

		m_NullSecurity.nLength = sizeof(m_NullSecurity);
		m_NullSecurity.lpSecurityDescriptor = mp_NullDesc;
		m_NullSecurity.bInheritHandle = TRUE; 
		
		return (&m_NullSecurity);
	};
};
MNullDesc gNullDesc;

SECURITY_ATTRIBUTES* NullSecurity()
{
	return gNullDesc.NullSecurity();
}
