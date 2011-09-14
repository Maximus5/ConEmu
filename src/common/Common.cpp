
/*
Copyright (c) 2009-2011 Maximus5
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

BOOL PackInputRecord(const INPUT_RECORD* piRec, MSG64* pMsg)
{
	if (!pMsg || !piRec)
	{
		_ASSERTE(pMsg!=NULL && piRec!=NULL);
		return FALSE;
	}

	memset(pMsg, 0, sizeof(MSG64));
	UINT nMsg = 0; WPARAM wParam = 0; LPARAM lParam = 0;

	if (piRec->EventType == KEY_EVENT)
	{
		nMsg = piRec->Event.KeyEvent.bKeyDown ? WM_KEYDOWN : WM_KEYUP;
		lParam |= (DWORD_PTR)(WORD)piRec->Event.KeyEvent.uChar.UnicodeChar;
		lParam |= ((DWORD_PTR)piRec->Event.KeyEvent.wVirtualKeyCode & 0xFF) << 16;
		lParam |= ((DWORD_PTR)piRec->Event.KeyEvent.wVirtualScanCode & 0xFF) << 24;
		wParam |= (DWORD_PTR)piRec->Event.KeyEvent.dwControlKeyState & 0xFFFF;
		wParam |= ((DWORD_PTR)piRec->Event.KeyEvent.wRepeatCount & 0xFF) << 16;
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

		lParam = ((DWORD_PTR)(WORD)piRec->Event.MouseEvent.dwMousePosition.X)
		         | (((DWORD_PTR)(WORD)piRec->Event.MouseEvent.dwMousePosition.Y) << 16);
		// max 0x0010/*FROM_LEFT_4ND_BUTTON_PRESSED*/
		wParam |= ((DWORD_PTR)piRec->Event.MouseEvent.dwButtonState) & 0xFF;
		// max - ENHANCED_KEY == 0x0100
		wParam |= (((DWORD_PTR)piRec->Event.MouseEvent.dwControlKeyState) & 0xFFFF) << 8;

		if (nMsg == MOUSE_EVENT_WHEELED || nMsg == MOUSE_EVENT_HWHEELED)
		{
			// HIWORD() - short (direction[1/-1])*count*120
			short nWheel = (short)((((DWORD)piRec->Event.MouseEvent.dwButtonState) & 0xFFFF0000) >> 16);
			char  nCount = nWheel / 120;
			wParam |= ((DWORD_PTR)(BYTE)nCount) << 24;
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
	if (!piMsg || !pRec)
	{
		_ASSERTE(piMsg!=NULL && pRec!=NULL);
		return FALSE;
	}

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

//#ifdef CONEMU_MINIMAL
//#include "base64.h"
//#endif

class MNullDesc
{
protected:
	PACL mp_ACL;
	#ifndef CONEMU_MINIMAL
	PSECURITY_DESCRIPTOR mp_NullDesc;
	SECURITY_ATTRIBUTES  m_NullSecurity;
	#endif
	PSECURITY_DESCRIPTOR mp_LocalDesc;
	SECURITY_ATTRIBUTES  m_LocalSecurity;
	HMODULE mh_AdvApi;

	typedef BOOL (WINAPI *AddAccessAllowedAce_t)(PACL pAcl, DWORD dwAceRevision, DWORD AccessMask, PSID pSid);
	AddAccessAllowedAce_t AddAccessAllowedAce;
	typedef BOOL (WINAPI *AddAccessDeniedAce_t)(PACL pAcl, DWORD dwAceRevision, DWORD AccessMask, PSID pSid);
	AddAccessDeniedAce_t AddAccessDeniedAce;
	typedef BOOL (WINAPI *AllocateAndInitializeSid_t)(PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority, BYTE nSubAuthorityCount, DWORD dwSubAuthority0, DWORD dwSubAuthority1, DWORD dwSubAuthority2, DWORD dwSubAuthority3, DWORD dwSubAuthority4, DWORD dwSubAuthority5, DWORD dwSubAuthority6, DWORD dwSubAuthority7, PSID *pSid);
	AllocateAndInitializeSid_t AllocateAndInitializeSid;
	typedef DWORD (WINAPI *GetLengthSid_t)(PSID pSid);
	GetLengthSid_t GetLengthSid;
	typedef BOOL (WINAPI *InitializeAcl_t)(PACL pAcl, DWORD nAclLength, DWORD dwAclRevision);
	InitializeAcl_t InitializeAcl;
	typedef BOOL (WINAPI *InitializeSecurityDescriptor_t)(PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD dwRevision);
	InitializeSecurityDescriptor_t InitializeSecurityDescriptor;
	typedef BOOL (WINAPI *SetSecurityDescriptorDacl_t)(PSECURITY_DESCRIPTOR pSecurityDescriptor, BOOL bDaclPresent, PACL pDacl, BOOL bDaclDefaulted);
	SetSecurityDescriptorDacl_t SetSecurityDescriptorDacl;

	BOOL LoadAdvApi()
	{
		if (mh_AdvApi)
		{
			return TRUE;
		}

		for (int i = 0; i <= 1; i++)
		{
			if (i == 0)
			{
				OSVERSIONINFOW osv = {sizeof(OSVERSIONINFOW)};
				GetVersionExW(&osv);
				if ((osv.dwMajorVersion < 6)
					|| ((osv.dwMajorVersion == 6) && (osv.dwMinorVersion == 0)))
					continue; // в Vista и ниже "KernelBase.dll" еще не было
				mh_AdvApi = LoadLibrary(L"KernelBase.dll");
			}
			else
			{
				mh_AdvApi = LoadLibrary(L"Advapi32.dll");
			}
			if (!mh_AdvApi)
			{
				#ifdef _DEBUG
				mn_LastError = GetLastError();
				#endif
				return FALSE;
			}

			AddAccessAllowedAce = (AddAccessAllowedAce_t)GetProcAddress(mh_AdvApi, "AddAccessAllowedAce");
			AddAccessDeniedAce = (AddAccessDeniedAce_t)GetProcAddress(mh_AdvApi, "AddAccessDeniedAce");
			AllocateAndInitializeSid = (AllocateAndInitializeSid_t)GetProcAddress(mh_AdvApi, "AllocateAndInitializeSid");
			GetLengthSid = (GetLengthSid_t)GetProcAddress(mh_AdvApi, "GetLengthSid");
			InitializeAcl = (InitializeAcl_t)GetProcAddress(mh_AdvApi, "InitializeAcl");
			InitializeSecurityDescriptor = (InitializeSecurityDescriptor_t)GetProcAddress(mh_AdvApi, "InitializeSecurityDescriptor");
			SetSecurityDescriptorDacl = (SetSecurityDescriptorDacl_t)GetProcAddress(mh_AdvApi, "SetSecurityDescriptorDacl");

			if (AddAccessAllowedAce && AddAccessDeniedAce && AllocateAndInitializeSid &&
				GetLengthSid && InitializeAcl && InitializeSecurityDescriptor && SetSecurityDescriptorDacl)
			{
				return TRUE;
			}

	    	UnloadAdvApi();
	    	return FALSE;
	    }
		return FALSE;
	}
	void UnloadAdvApi()
	{
		if (mh_AdvApi)
		{
			FreeLibrary(mh_AdvApi);
			mh_AdvApi = NULL;
		}
	}
	
public:
	#ifdef _DEBUG
	DWORD mn_LastError;
	#endif
public:
	MNullDesc()
	{
		#ifndef CONEMU_MINIMAL
		memset(&m_NullSecurity, 0, sizeof(m_NullSecurity));
		mp_NullDesc = NULL;
		#endif
		memset(&m_LocalSecurity, 0, sizeof(m_LocalSecurity));
		mp_LocalDesc = NULL;
		mp_ACL = NULL;
		#ifdef _DEBUG
		mn_LastError = 0;
		#endif
		mh_AdvApi = NULL;
	};
	~MNullDesc()
	{
		#ifndef CONEMU_MINIMAL
		memset(&m_NullSecurity, 0, sizeof(m_NullSecurity));
		LocalFree(mp_NullDesc); mp_NullDesc = NULL;
		#endif
		memset(&m_LocalSecurity, 0, sizeof(m_LocalSecurity));
		LocalFree(mp_LocalDesc); mp_LocalDesc = NULL;
	};
public:
	#ifndef CONEMU_MINIMAL
	SECURITY_ATTRIBUTES* NullSecurity()
	{
		if (mp_NullDesc)
		{
			_ASSERTE(m_NullSecurity.lpSecurityDescriptor==mp_NullDesc);
			return (&m_NullSecurity);
		}

		#ifdef _DEBUG
		mn_LastError = 0;
		#endif

		SECURITY_ATTRIBUTES* lpSec = NULL;
		
		if (!LoadAdvApi())
			goto wrap;

		mp_NullDesc = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
		              SECURITY_DESCRIPTOR_MIN_LENGTH);

		if (mp_NullDesc == NULL)
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			goto wrap;
		}

		if (!InitializeSecurityDescriptor(mp_NullDesc, SECURITY_DESCRIPTOR_REVISION))
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			LocalFree(mp_NullDesc); mp_NullDesc = NULL;
			goto wrap;
		}

		// Add a null DACL to the security descriptor.
		if (!SetSecurityDescriptorDacl(mp_NullDesc, TRUE, (PACL) NULL, FALSE))
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			LocalFree(mp_NullDesc); mp_NullDesc = NULL;
			goto wrap;
		}

		m_NullSecurity.nLength = sizeof(m_NullSecurity);
		m_NullSecurity.lpSecurityDescriptor = mp_NullDesc;
		m_NullSecurity.bInheritHandle = TRUE;
		lpSec = &m_NullSecurity;
	wrap:
		UnloadAdvApi();
		return lpSec;
	};
	#endif // CONEMU_MINIMAL
	SECURITY_ATTRIBUTES* LocalSecurity()
	{
		if (mp_LocalDesc)
		{
			_ASSERTE(m_LocalSecurity.lpSecurityDescriptor==mp_LocalDesc);
			return (&m_LocalSecurity);
		}

		#ifdef _DEBUG
		mn_LastError = 0;
		#endif

		//#ifdef CONEMU_MINIMAL
		//		// Возможно, есть переменная окружения CESECURITYNAME?
		//		int nLen = 1024;
		//		char* pszEncoded = (char*)calloc(1024,1);
		//		if (!pszEncoded)
		//			goto DoLoad;
		//		nLen = GetEnvironmentVariableA(CESECURITYNAME, pszEncoded, 1024);
		//		if (nLen > 0)
		//		{
		//			if (nLen > 1024)
		//			{
		//				nLen ++;
		//				free(pszEncoded);
		//				pszEncoded = (char*)calloc(nLen,1);
		//				nLen = GetEnvironmentVariableA(CESECURITYNAME, pszEncoded, 1024);
		//				if (!nLen)
		//				{
		//					free(pszEncoded);
		//					goto DoLoad;
		//				}
		//			}
		//		}
		//		free(pszEncoded);
		//		DoLoad:
		//#endif
		
		SECURITY_ATTRIBUTES* lpSec = NULL;
		BYTE NetworkSid[12] = {01,01,00,00,00,00,00,05,02,00,00,00};
		BYTE SidWorld[12]   = {01,01,00,00,00,00,00,01,00,00,00,00};
		PSID pNetworkSid = (PSID)NetworkSid;
		PSID pSidWorld = (PSID)SidWorld;
		DWORD dwSize = 0;

		if (!LoadAdvApi())
			goto wrap;
		
		mp_LocalDesc = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
		               SECURITY_DESCRIPTOR_MIN_LENGTH);

		if (mp_LocalDesc == NULL)
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			goto wrap;
		}

		if (!InitializeSecurityDescriptor(mp_LocalDesc, SECURITY_DESCRIPTOR_REVISION))
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			LocalFree(mp_LocalDesc); mp_LocalDesc = NULL;
			goto wrap;
		}

		dwSize = sizeof(ACL) //-V104 //-V119 //-V103
		         + sizeof(ACCESS_DENIED_ACE) + GetLengthSid(pNetworkSid)
		         + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pSidWorld);
		mp_ACL = (PACL)LocalAlloc(LPTR, dwSize); //-V106

		if (!InitializeAcl(mp_ACL, dwSize, ACL_REVISION))
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			goto wrap;
		}

		if (!AddAccessDeniedAce(mp_ACL, ACL_REVISION, GENERIC_ALL, pNetworkSid))
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			goto wrap;
		}

		if (!AddAccessAllowedAce(mp_ACL, ACL_REVISION, GENERIC_ALL, pSidWorld))
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			goto wrap;
		}

		if (!SetSecurityDescriptorDacl(mp_LocalDesc, TRUE, mp_ACL, FALSE))
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			LocalFree(mp_LocalDesc); mp_LocalDesc = NULL;
			goto wrap;
		}

	//SetLocal:
		m_LocalSecurity.nLength = sizeof(m_LocalSecurity);
		m_LocalSecurity.lpSecurityDescriptor = mp_LocalDesc;
		m_LocalSecurity.bInheritHandle = TRUE;
		lpSec = &m_LocalSecurity;
	wrap:
		UnloadAdvApi();
		return lpSec;
	};
};
MNullDesc *gNullDesc = NULL;

#ifndef CONEMU_MINIMAL
SECURITY_ATTRIBUTES* NullSecurity()
{
	if (!gNullDesc) gNullDesc = new MNullDesc();

	return gNullDesc->NullSecurity();
}
#endif
SECURITY_ATTRIBUTES* LocalSecurity()
{
	if (!gNullDesc) gNullDesc = new MNullDesc();

	return gNullDesc->LocalSecurity();
}

BOOL gbInCommonShutdown = FALSE;
//HANDLE ghHookMutex = NULL;
//MSection* gpHookCS = NULL;
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

	//if (ghHookMutex)
	//{
	//	CloseHandle(ghHookMutex);
	//	ghHookMutex = NULL;
	//}
	//if (gpHookCS)
	//{
	//	MSection *p = gpHookCS;
	//	gpHookCS = NULL;
	//	delete p;
	//}

#ifdef _DEBUG
	MyAssertShutdown();
#endif
}












#ifndef CONEMU_MINIMAL
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
#endif


//
//	SETCONSOLEINFO.C
//
//	Undocumented method to set console attributes at runtime
//
//	For Vista use the newly documented SetConsoleScreenBufferEx API
//
//	www.catch22.net
//

#ifndef CONEMU_MINIMAL
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

		memcpy(ptrView, pci, pci->Length); //-V106
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
#endif

#ifndef CONEMU_MINIMAL
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
#endif


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


#ifndef CONEMU_MINIMAL
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
		lstrcpynW(cfi.FaceName, asFontName ? asFontName : L"Lucida Console", LF_FACESIZE); //-V303
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
		lstrcpynW(gpConsoleInfoStr->FaceName, asFontName ? asFontName : L"Lucida Console", countof(gpConsoleInfoStr->FaceName)); //-V303
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
		gpConsoleInfoStr->NumberOfHistoryBuffers	= 4; //-V112

		// color table
		for(i = 0; i < 16; i++)
			gpConsoleInfoStr->ColorTable[i] = DefaultColors[i];

		gpConsoleInfoStr->CodePage					= GetConsoleOutputCP();//0;//0x352;
		gpConsoleInfoStr->Hwnd						= inConWnd;
		gpConsoleInfoStr->ConsoleTitle[0] = 0;
		SetConsoleInfo(inConWnd, gpConsoleInfoStr);
	}
}
#endif
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

