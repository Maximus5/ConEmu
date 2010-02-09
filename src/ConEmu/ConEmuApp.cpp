
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


#include "Header.h"
#include <commctrl.h>
#include "../common/ConEmuCheck.h"

#define DEBUGSTRMOVE(s) //DEBUGSTR(s)


#ifdef MSGLOGGER
BOOL bBlockDebugLog=false, bSendToDebugger=true, bSendToFile=false;
WCHAR *LogFilePath=NULL;
#endif
#ifndef _DEBUG
BOOL gbNoDblBuffer = false;
#else
BOOL gbNoDblBuffer = false;
#endif
BOOL gbMessagingStarted = FALSE;

#ifdef _DEBUG
wchar_t gszDbgModLabel[6] = {0};
#endif


//externs
HINSTANCE g_hInstance=NULL;
HWND ghWnd=NULL, ghWndDC=NULL, ghConWnd=NULL, ghWndApp=NULL;
CConEmuMain gConEmu;
//CVirtualConsole *pVCon=NULL;
CSettings gSet;
//TCHAR temp[MAX_PATH]; -- низзя, очень велик шанс нарваться при многопоточности
HICON hClassIcon = NULL, hClassIconSm = NULL;
BOOL gbDontEnable = FALSE;
BOOL gbDebugLogStarted = FALSE;


const TCHAR *const szClassName = VirtualConsoleClass;
const TCHAR *const szClassNameParent = VirtualConsoleClassMain;
const TCHAR *const szClassNameApp = VirtualConsoleClassApp;
const TCHAR *const szClassNameBack = VirtualConsoleClassBack;
const TCHAR *const szClassNameScroll = VirtualConsoleClassScroll;


OSVERSIONINFO gOSVer;


#ifdef MSGLOGGER
void DebugLogMessage(HWND h, UINT m, WPARAM w, LPARAM l, BOOL posted, BOOL extra)
{
    if (bBlockDebugLog || (!bSendToFile && !IsDebuggerPresent()))
        return;

    static char szMess[32], szWP[32], szLP[48], szWhole[255];
    //static SYSTEMTIME st;
    szMess[0]=0; szWP[0]=0; szLP[0]=0; szWhole[0]=0;
    
    #define MSG1(s) case s: lstrcpyA(szMess, #s); break;
    #define MSG2(s) case s: lstrcpyA(szMess, #s);
    #define WP1(s) case s: lstrcpyA(szWP, #s); break;
    #define WP2(s) case s: lstrcpyA(szWP, #s);
    
    switch (m)
    {
    MSG1(WM_NOTIFY);
    MSG1(WM_PAINT);
    MSG1(WM_TIMER);
    MSG2(WM_SIZING);
    {
        switch(w)
        {
        WP1(WMSZ_RIGHT);
        WP1(WMSZ_BOTTOM);
        WP1(WMSZ_BOTTOMRIGHT);
        WP1(WMSZ_LEFT);
        WP1(WMSZ_TOP);
        WP1(WMSZ_TOPLEFT);
        WP1(WMSZ_TOPRIGHT);
        WP1(WMSZ_BOTTOMLEFT);
        }
        break;
    }
    MSG1(WM_SIZE);
    MSG1(WM_KEYDOWN);
    MSG1(WM_KEYUP);
    MSG1(WM_SYSKEYDOWN);
    MSG1(WM_SYSKEYUP);
    MSG2(WM_MOUSEWHEEL);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_ACTIVATE);
		switch(w) {
			WP1(WA_ACTIVE);
			WP1(WA_CLICKACTIVE);
			WP1(WA_INACTIVE);
		}
        break;
    MSG2(WM_ACTIVATEAPP);
        if (w==0)
            break;
        break;
    MSG2(WM_KILLFOCUS);
        break;
    MSG1(WM_SETFOCUS);
    MSG2(WM_MOUSEMOVE);
        //wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        //break;
		return;
    MSG2(WM_RBUTTONDOWN);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_RBUTTONUP);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_MBUTTONDOWN);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_MBUTTONUP);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_LBUTTONDOWN);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_LBUTTONUP);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_LBUTTONDBLCLK);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_MBUTTONDBLCLK);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG2(WM_RBUTTONDBLCLK);
        wsprintfA(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
        break;
    MSG1(WM_CLOSE);
    MSG1(WM_CREATE);
    MSG2(WM_SYSCOMMAND);
    {   
        switch(w)
        {
        WP1(SC_MAXIMIZE_SECRET);
        WP2(SC_RESTORE_SECRET);
            break;
        WP1(SC_CLOSE);
        WP1(SC_MAXIMIZE);
        WP2(SC_RESTORE);
            break;
        WP1(SC_PROPERTIES_SECRET);
        WP1(ID_SETTINGS);
        WP1(ID_HELP);
        WP1(ID_ABOUT);
        WP1(ID_TOTRAY);
        WP1(ID_CONPROP);
        }
        break;
    }
    MSG1(WM_NCRBUTTONUP);
    MSG1(WM_TRAYNOTIFY);
    MSG1(WM_DESTROY);
    MSG1(WM_INPUTLANGCHANGE);
    MSG1(WM_INPUTLANGCHANGEREQUEST);
    MSG1(WM_IME_NOTIFY);
    MSG1(WM_VSCROLL);
    MSG1(WM_NULL);
    //default:
    //  return;
    }

    if (bSendToDebugger || bSendToFile) {
        if (!szMess[0]) wsprintfA(szMess, "%i", m);
        if (!szWP[0]) wsprintfA(szWP, "%i", w);
        if (!szLP[0]) wsprintfA(szLP, "%i(0x%08X)", l, l);
        

        if (bSendToDebugger) {
            static SYSTEMTIME st;
            GetLocalTime(&st);
            wsprintfA(szWhole, "%02i:%02i.%03i %s%s(%s, %s, %s)\n", st.wMinute, st.wSecond, st.wMilliseconds,
                (posted ? "Post" : "Send"), (extra ? "+" : ""), szMess, szWP, szLP);

            OutputDebugStringA(szWhole);
        } else if (bSendToFile) {
            wsprintfA(szWhole, "%s%s(%s, %s, %s)\n",
                (posted ? "Post" : "Send"), (extra ? "+" : ""), szMess, szWP, szLP);

            DebugLogFile(szWhole);
        }
    }
}
void DebugLogPos(HWND hw, int x, int y, int w, int h, LPCSTR asFunc)
{
	#ifdef MSGLOGGER
	if (!x && !y && hw == ghConWnd) {
		if (!IsDebuggerPresent() && !isPressed(VK_LBUTTON))
			x = x;
	}
	#endif
    if (bBlockDebugLog || (!bSendToFile && !IsDebuggerPresent()))
        return;

    

    if (bSendToDebugger) {
		wchar_t szPos[255];
        static SYSTEMTIME st;
        GetLocalTime(&st);
        wsprintfW(szPos, L"%02i:%02i.%03i %s(%s, %i,%i,%i,%i)\n",
            st.wMinute, st.wSecond, st.wMilliseconds, asFunc,
            (hw==ghConWnd) ? L"Con" : L"Emu", x,y,w,h);

        DEBUGSTRMOVE(szPos);
    } else if (bSendToFile) {
		char szPos[255];
        wsprintfA(szPos, "%s(%s, %i,%i,%i,%i)\n",
			asFunc,
            (hw==ghConWnd) ? "Con" : "Emu", x,y,w,h);

        DebugLogFile(szPos);
    }
}
void DebugLogBufSize(HANDLE h, COORD sz)
{
    if (bBlockDebugLog || (!bSendToFile && !IsDebuggerPresent()))
        return;

    static char szSize[255];

    if (bSendToDebugger) {
        static SYSTEMTIME st;
        GetLocalTime(&st);
        wsprintfA(szSize, "%02i:%02i.%03i ChangeBufferSize(%i,%i)\n",
            st.wMinute, st.wSecond, st.wMilliseconds,
            sz.X, sz.Y);

        OutputDebugStringA(szSize);
    } else if (bSendToFile) {
        wsprintfA(szSize, "ChangeBufferSize(%i,%i)\n",
            sz.X, sz.Y);

        DebugLogFile(szSize);
    }
}
void DebugLogFile(LPCSTR asMessage)
{
	if (!bSendToFile)
		return;
    HANDLE hLogFile = INVALID_HANDLE_VALUE;

    if (LogFilePath==NULL) {
        //WCHAR LogFilePath[MAX_PATH+1];
        LogFilePath = (WCHAR*)calloc(MAX_PATH+1,sizeof(WCHAR));
        GetModuleFileNameW(0,LogFilePath,MAX_PATH);
        WCHAR* pszDot = wcsrchr(LogFilePath, L'.');
        lstrcpyW(pszDot, L".log");
        hLogFile = CreateFileW(LogFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    } else {
        hLogFile = CreateFileW(LogFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    if (hLogFile!=INVALID_HANDLE_VALUE) {
        DWORD dwSize=0;
        SetFilePointer(hLogFile, 0, 0, FILE_END);

        SYSTEMTIME st;
        GetLocalTime(&st);
        char szWhole[32];
        wsprintfA(szWhole, "%02i:%02i:%02i.%03i ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        WriteFile(hLogFile, szWhole, lstrlenA(szWhole), &dwSize, NULL);

        WriteFile(hLogFile, asMessage, lstrlenA(asMessage), &dwSize, NULL);

        CloseHandle(hLogFile);
    }
}
#endif
#ifdef _DEBUG
char gsz_MDEBUG_TRAP_MSG[3000];
char gsz_MDEBUG_TRAP_MSG_APPEND[2000];
HWND gh_MDEBUG_TRAP_PARENT_WND = NULL;
int __stdcall _MDEBUG_TRAP(LPCSTR asFile, int anLine)
{
	//__debugbreak();
	_ASSERT(FALSE);
    wsprintfA(gsz_MDEBUG_TRAP_MSG, "MDEBUG_TRAP\r\n%s(%i)\r\n", asFile, anLine);
    if (gsz_MDEBUG_TRAP_MSG_APPEND[0])
        lstrcatA(gsz_MDEBUG_TRAP_MSG,gsz_MDEBUG_TRAP_MSG_APPEND);
    MessageBoxA(ghWnd,gsz_MDEBUG_TRAP_MSG,"MDEBUG_TRAP",MB_OK|MB_ICONSTOP);
    return 0;
}
int MDEBUG_CHK = TRUE;
#endif

/* Используются как extern в ConEmuCheck.cpp */
LPVOID _calloc(size_t nCount,size_t nSize) {
	return calloc(nCount,nSize);
}
LPVOID _malloc(size_t nCount) {
	return malloc(nCount);
}
void   _free(LPVOID ptr) {
	free(ptr);
}


LRESULT CALLBACK AppWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    
	if (messg == WM_CREATE) {
		if (ghWndApp == NULL)
			ghWndApp = hWnd;
	} else if (messg == WM_ACTIVATEAPP) {
		if (wParam && ghWnd)
			SetFocus(ghWnd);
	}
	
	result = DefWindowProc(hWnd, messg, wParam, lParam);

    return result;
}

BOOL CreateAppWindow()
{
	//2009-03-05 - нельзя этого делать. а то дочерние процессы с тем же Affinity запускаются...
	// На тормоза - не влияет. Но вроде бы на многопроцессорных из-за глюков в железе могут быть ошибки подсчета производительности, если этого не сделать
	if (gSet.nAffinity)
		SetProcessAffinityMask(GetCurrentProcess(), gSet.nAffinity);
	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	//SetThreadAffinityMask(GetCurrentThread(), 1);

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC   = ICC_COOL_CLASSES|ICC_BAR_CLASSES|ICC_TAB_CLASSES|ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&icex);

	/*DWORD dwErr = 0;
	HMODULE hInf = LoadLibrary(L"infis.dll");
	if (!hInf)
		dwErr = GetLastError();*/

    //!!!ICON
    gConEmu.LoadIcons();
    
    if (!gSet.isCreateAppWindow)
	    return TRUE;

	WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_DBLCLKS|CS_OWNDC, AppWndProc, 0, 0, 
		    g_hInstance, hClassIcon, LoadCursor(NULL, IDC_ARROW), 
		    NULL /*(HBRUSH)COLOR_BACKGROUND*/, 
		    NULL, szClassNameApp, hClassIconSm};// | CS_DROPSHADOW
    if (!RegisterClassEx(&wc))
        return -1;
    //ghWnd = CreateWindow(szClassName, 0, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, gSet.wndX, gSet.wndY, cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4, NULL, NULL, (HINSTANCE)g_hInstance, NULL);
    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
    int nWidth=100, nHeight=100, nX = -32000, nY = -32000;
    DWORD exStyle = WS_EX_APPWINDOW|WS_EX_ACCEPTFILES;
    
    
    // cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4; -- все равно это было не правильно
	ghWndApp = CreateWindowEx(exStyle, szClassNameApp, L"ConEmu", style, nX, nY, nWidth, nHeight, NULL, NULL, (HINSTANCE)g_hInstance, NULL);
	if (!ghWndApp) {
		MBoxA(_T("Can't create application window!"));
        return FALSE;
	}
	return TRUE;
}



void DisplayLastError(LPCTSTR asLabel, DWORD dwError /* =0 */)
{
	DWORD dw = dwError ? dwError : GetLastError();
	wchar_t* lpMsgBuf = NULL;
	MCHKHEAP
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL );
	int nLen = _tcslen(asLabel)+64+(lpMsgBuf ? wcslen(lpMsgBuf) : 0);
	wchar_t *out = new wchar_t[nLen];
	wsprintf(out, _T("%s\nLastError=0x%08X\n%s"), asLabel, dw, lpMsgBuf);
	if (gbMessagingStarted) SetForegroundWindow(ghWnd);
	MessageBox(gbMessagingStarted ? ghWnd : NULL, out, gConEmu.GetTitle(), MB_SYSTEMMODAL | MB_ICONERROR);
	MCHKHEAP
		LocalFree(lpMsgBuf);
	delete [] out;
	MCHKHEAP
}



//BOOL GetFontNameFromFile(LPCTSTR lpszFilePath, LPTSTR rsFontName)
//{
//	typedef struct _tagTT_OFFSET_TABLE{
//		USHORT	uMajorVersion;
//		USHORT	uMinorVersion;
//		USHORT	uNumOfTables;
//		USHORT	uSearchRange;
//		USHORT	uEntrySelector;
//		USHORT	uRangeShift;
//	}TT_OFFSET_TABLE;
//
//	typedef struct _tagTT_TABLE_DIRECTORY{
//		char	szTag[4];			//table name
//		ULONG	uCheckSum;			//Check sum
//		ULONG	uOffset;			//Offset from beginning of file
//		ULONG	uLength;			//length of the table in bytes
//	}TT_TABLE_DIRECTORY;
//
//	typedef struct _tagTT_NAME_TABLE_HEADER{
//		USHORT	uFSelector;			//format selector. Always 0
//		USHORT	uNRCount;			//Name Records count
//		USHORT	uStorageOffset;		//Offset for strings storage, from start of the table
//	}TT_NAME_TABLE_HEADER;
//
//	typedef struct _tagTT_NAME_RECORD{
//		USHORT	uPlatformID;
//		USHORT	uEncodingID;
//		USHORT	uLanguageID;
//		USHORT	uNameID;
//		USHORT	uStringLength;
//		USHORT	uStringOffset;	//from start of storage area
//	}TT_NAME_RECORD;
//
//	#define SWAPWORD(x)		MAKEWORD(HIBYTE(x), LOBYTE(x))
//	#define SWAPLONG(x)		MAKELONG(SWAPWORD(HIWORD(x)), SWAPWORD(LOWORD(x)))
//
//	BOOL lbRc = FALSE;
//	HANDLE f = NULL;
//	wchar_t szRetVal[MAX_PATH];
//	DWORD dwRead;
//
//	//if(f.Open(lpszFilePath, CFile::modeRead|CFile::shareDenyWrite)){
//	if ((f = CreateFile(lpszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL))!=INVALID_HANDLE_VALUE)
//	{
//		TT_OFFSET_TABLE ttOffsetTable;
//		//f.Read(&ttOffsetTable, sizeof(TT_OFFSET_TABLE));
//		if (ReadFile(f, &ttOffsetTable, sizeof(TT_OFFSET_TABLE), &(dwRead=0), NULL) && dwRead)
//		{
//			ttOffsetTable.uNumOfTables = SWAPWORD(ttOffsetTable.uNumOfTables);
//			ttOffsetTable.uMajorVersion = SWAPWORD(ttOffsetTable.uMajorVersion);
//			ttOffsetTable.uMinorVersion = SWAPWORD(ttOffsetTable.uMinorVersion);
//
//			//check is this is a true type font and the version is 1.0
//			if(ttOffsetTable.uMajorVersion != 1 || ttOffsetTable.uMinorVersion != 0)
//				return FALSE;
//			
//			TT_TABLE_DIRECTORY tblDir;
//			BOOL bFound = FALSE;
//			
//			for(int i=0; i< ttOffsetTable.uNumOfTables; i++){
//				//f.Read(&tblDir, sizeof(TT_TABLE_DIRECTORY));
//				if (ReadFile(f, &tblDir, sizeof(TT_TABLE_DIRECTORY), &(dwRead=0), NULL) && dwRead)
//				{
//					//strncpy(szRetVal, tblDir.szTag, 4); szRetVal[4] = 0;
//					//if(lstrcmpi(szRetVal, L"name") == 0)
//					//if (memcmp(tblDir.szTag, "name", 4) == 0)
//					if (strnicmp(tblDir.szTag, "name", 4) == 0)
//					{
//						bFound = TRUE;
//						tblDir.uLength = SWAPLONG(tblDir.uLength);
//						tblDir.uOffset = SWAPLONG(tblDir.uOffset);
//						break;
//					}
//				}
//			}
//			
//			if(bFound){
//				if (SetFilePointer(f, tblDir.uOffset, NULL, FILE_BEGIN)!=INVALID_SET_FILE_POINTER)
//				{
//					TT_NAME_TABLE_HEADER ttNTHeader;
//					//f.Read(&ttNTHeader, sizeof(TT_NAME_TABLE_HEADER));
//					if (ReadFile(f, &ttNTHeader, sizeof(TT_NAME_TABLE_HEADER), &(dwRead=0), NULL) && dwRead)
//					{
//						ttNTHeader.uNRCount = SWAPWORD(ttNTHeader.uNRCount);
//						ttNTHeader.uStorageOffset = SWAPWORD(ttNTHeader.uStorageOffset);
//						TT_NAME_RECORD ttRecord;
//						bFound = FALSE;
//						
//						for(int i=0; i<ttNTHeader.uNRCount; i++){
//							//f.Read(&ttRecord, sizeof(TT_NAME_RECORD));
//							if (ReadFile(f, &ttRecord, sizeof(TT_NAME_RECORD), &(dwRead=0), NULL) && dwRead)
//							{
//								ttRecord.uNameID = SWAPWORD(ttRecord.uNameID);
//								if(ttRecord.uNameID == 1){
//									ttRecord.uStringLength = SWAPWORD(ttRecord.uStringLength);
//									ttRecord.uStringOffset = SWAPWORD(ttRecord.uStringOffset);
//									//int nPos = f.GetPosition();
//									DWORD nPos = SetFilePointer(f, 0, 0, FILE_CURRENT);
//									//f.Seek(tblDir.uOffset + ttRecord.uStringOffset + ttNTHeader.uStorageOffset, CFile::begin);
//									if (SetFilePointer(f, tblDir.uOffset + ttRecord.uStringOffset + ttNTHeader.uStorageOffset, 0, FILE_BEGIN)!=INVALID_SET_FILE_POINTER)
//									{
//										if ((ttRecord.uStringLength + 1) < 33)
//										{
//											//f.Read(csTemp.GetBuffer(ttRecord.uStringLength + 1), ttRecord.uStringLength);
//											//csTemp.ReleaseBuffer();
//											char szName[MAX_PATH]; szName[ttRecord.uStringLength + 1] = 0;
//											if (ReadFile(f, szName, ttRecord.uStringLength + 1, &(dwRead=0), NULL) && dwRead)
//											{
//												//if(csTemp.GetLength() > 0){
//												if (szName[0]) {
//													szName[ttRecord.uStringLength + 1] = 0;
//													for (int j = ttRecord.uStringLength; j >= 0 && szName[j] == ' '; j--)
//														szName[j] = 0;
//													if (szName[0]) {
//														MultiByteToWideChar(CP_ACP, 0, szName, -1, szRetVal, 32);
//														szRetVal[31] = 0;
//														lbRc = TRUE;
//													}
//													break;
//												}
//											}
//										}
//									}
//									//f.Seek(nPos, CFile::begin);
//									SetFilePointer(f, nPos, 0, FILE_BEGIN);
//								}
//							}
//						}
//					}
//				}			
//			}
//		}
//		CloseHandle(f);
//	}
//	return lbRc;
//}

//BOOL FindFontInFolder(wchar_t* szTempFontFam)
//{
//	BOOL lbRc = FALSE;
//
//	typedef BOOL (WINAPI* FGetFontResourceInfo)(LPCTSTR lpszFilename,LPDWORD cbBuffer,LPVOID lpBuffer,DWORD dwQueryType);
//	FGetFontResourceInfo GetFontResourceInfo = NULL;
//	HMODULE hGdi = LoadLibrary(L"gdi32.dll");
//	if (!hGdi) return FALSE;
//	GetFontResourceInfo = (FGetFontResourceInfo)GetProcAddress(hGdi, "GetFontResourceInfoW");
//	if (!GetFontResourceInfo) return FALSE;
//
//	WIN32_FIND_DATA fnd;
//	wchar_t szFind[MAX_PATH]; wcscpy(szFind, gConEmu.ms_ConEmuExe);
//	wchar_t *pszSlash = wcsrchr(szFind, L'\\');
//
//	if (pszSlash) {
//		wcscpy(pszSlash, L"\\*.ttf");
//		HANDLE hFind = FindFirstFile(szFind, &fnd);
//		if (hFind != INVALID_HANDLE_VALUE) {
//			do {
//				if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
//					lbRc = TRUE; break;
//				}
//			} while (FindNextFile(hFind, &fnd));
//			FindClose(hFind);
//		}
//
//		if (lbRc) {
//			lbRc = FALSE;
//			pszSlash[1] = 0;
//			if ((wcslen(fnd.cFileName)+wcslen(szFind)) >= MAX_PATH) {
//				TCHAR* psz=(TCHAR*)calloc(wcslen(fnd.cFileName)+100,sizeof(TCHAR));
//				lstrcpyW(psz, L"Too long full pathname for font:\n");
//				lstrcatW(psz, fnd.cFileName);
//				MessageBox(NULL, psz, L"ConEmu", MB_OK|MB_ICONSTOP);
//				free(psz);
//			} else {
//				wcscat(szFind, fnd.cFileName);
//				// Теперь нужно определить имя шрифта
//				DWORD dwSize = MAX_PATH;
//				//if (!AddFontResourceEx(szFind, FR_PRIVATE, NULL)) //ADD fontname; by Mors
//				//{
//				//	TCHAR* psz=(TCHAR*)calloc(wcslen(szFind)+100,sizeof(TCHAR));
//				//	lstrcpyW(psz, L"Can't register font:\n");
//				//	lstrcatW(psz, szFind);
//				//	MessageBox(NULL, psz, L"ConEmu", MB_OK|MB_ICONSTOP);
//				//	free(psz);
//				//} else
//				//if (!GetFontResourceInfo(szFind, &dwSize, szTempFontFam, 1)) {
//				if (!gSet.GetFontNameFromFile(szFind, szTempFontFam)) {
//					DWORD dwErr = GetLastError();
//					TCHAR* psz=(TCHAR*)calloc(wcslen(szFind)+100,sizeof(TCHAR));
//					lstrcpyW(psz, L"Can't query font family for file:\n");
//					lstrcatW(psz, szFind);
//					wsprintf(psz+wcslen(psz), L"\nErrCode=0x%08X", dwErr);
//					MessageBox(NULL, psz, L"ConEmu", MB_OK|MB_ICONSTOP);
//					free(psz);
//				} else {
//					lstrcpynW(gSet.FontFile, szFind, countof(gSet.FontFile));
//					lbRc = TRUE;
//				}
//			}
//		}
//	}
//
//	return lbRc;
//}

//extern void SetConsoleFontSizeTo(HWND inConWnd, int inSizeX, int inSizeY);

// Disables the IME for all threads in a current process.
//void DisableIME()
//{
//	typedef BOOL (WINAPI* ImmDisableIMEt)(DWORD idThread);
//	BOOL lbDisabled = FALSE;
//	HMODULE hImm32 = LoadLibrary(_T("imm32.dll"));
//	if (hImm32) {
//		ImmDisableIMEt ImmDisableIMEf = (ImmDisableIMEt)GetProcAddress(hImm32, "ImmDisableIME");
//		if (ImmDisableIMEf) {
//			lbDisabled = ImmDisableIMEf(-1);
//		}
//	}
//	return;
//}

void MessageLoop()
{
	MSG Msg = {NULL};
	gbMessagingStarted = TRUE;
	while (GetMessage(&Msg, NULL, 0, 0))
	{
		BOOL lbDlgMsg = FALSE;
		if (ghOpWnd) {
			if (IsWindow(ghOpWnd))
				lbDlgMsg = IsDialogMessage(ghOpWnd, &Msg);
		}
		if (!lbDlgMsg)
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	gbMessagingStarted = FALSE;
}

/* С командной строкой (GetCommandLineW) у нас засада */
/*

ShellExecute("open", "ShowArg.exe", "\"test1\" test2");
GetCommandLineW(): "T:\XChange\VCProject\TestRunArg\ShowArg.exe" "test1" test2

CreateProcess("ShowArg.exe", "\"test1\" test2");
GetCommandLineW(): "test1" test2

CreateProcess(NULL, "\"ShowArg.exe\" \"test1\" test2");
GetCommandLineW(): "ShowArg.exe" "test1" test2

*/

void SplitCommandLine(wchar_t *str, uint *n)
{
	*n = 0;
	wchar_t *dst = str, ts;
	while (*str == ' ')
		str++;
	ts = ' ';
	while (*str)
	{
		if (*str == '"')
		{
			ts ^= 2; // ' ' <-> '"'
			str++;
		}
		while (*str && *str != '"' && *str != ts)
			*dst++ = *str++;
		if (*str == '"')
			continue;
		while (*str == ' ')
			str++;
		*dst++ = 0;
		(*n)++;
	}
	return;
}

//Result:
//  cmdLine - указатель на буфер с аргументами (!) он будет освобожден через free(cmdLine)
//  cmdNew  - то что запускается (после аргумента /cmd)
//  params  - количество аргументов 
//            0 - ком.строка пустая
//            ((uint)-1) - весь cmdNew должен быть ПРИКЛЕЕН к строке запуска по умолчанию
BOOL PrepareCommandLine(TCHAR*& cmdLine, TCHAR*& cmdNew, uint& params)
{
	params = 0;
    cmdNew = NULL;
    LPCWSTR pszCmdLine = GetCommandLine();
    int nInitLen = lstrlen(pszCmdLine);
    cmdLine = _tcsdup(pszCmdLine);
    
	
	// Имя исполняемого файла (conemu.exe)
	const wchar_t* pszExeName = wcsrchr(gConEmu.ms_ConEmuExe, L'\\');
	if (pszExeName) pszExeName++; else pszExeName = gConEmu.ms_ConEmuExe;
	
	wchar_t *pszNext = NULL, *pszStart = NULL, chSave = 0;
	
	if (*cmdLine == L' ') {
		// Исполняемого файла нет - сразу начинаются аргументы
		pszNext = NULL;
	} else if (*cmdLine == L'"') {
		// Имя между кавычками
		pszStart = cmdLine+1;
		pszNext = wcschr ( pszStart, L'"' );
		if (!pszNext) {
			MBoxA(L"Invalid command line: quates are not balanced");
			return FALSE;
		}
		chSave = *pszNext;
		*pszNext = 0;
	} else {
		pszStart = cmdLine;
		pszNext = wcschr ( pszStart, L' ' );
		if (!pszNext) pszNext = pszStart + lstrlen(pszStart);
		chSave = *pszNext;
		*pszNext = 0;
	}
	
	if (pszNext) {
		wchar_t* pszFN = wcsrchr(pszStart, L'\\');
		if (pszFN) pszFN++; else pszFN = pszStart;
		// Если первый параметр - наш conemu.exe или его путь - нужно его выбросить
		if (!lstrcmpi(pszFN, pszExeName)) {
			// Нужно отрезать
			int nCopy = (nInitLen - (pszNext - cmdLine)) * sizeof(wchar_t);
			TODO("Проверить, чтобы длину корректно посчитать");
			if (nCopy > 0)
				memmove(cmdLine, pszNext+1, nCopy);
			else
				*cmdLine = 0;
		} else {
			*pszNext = chSave;
		}
	}
	
	// AI. Если первый аргумент начинается НЕ с '/' - считаем что эта строка должна полностью передаваться в 
	// запускаемую программу (прилепляться к концу ком.строки по умолчанию)
	pszStart = cmdLine;
	while (*pszStart == L' ' || *pszStart == L'"') pszStart++; // пропустить пробелы и кавычки
	
	if (*pszStart == 0) {
		params = 0;
		*cmdLine = 0;
		cmdNew = NULL;
		
		// Эта переменная нужна для того, чтобы conemu можно было перезапустить
		// из cmd файла с теми же аргументами (selfupdate)
		SetEnvironmentVariableW(L"ConEmuArgs", L"");
		
	} else {
		// Эта переменная нужна для того, чтобы conemu можно было перезапустить
		// из cmd файла с теми же аргументами (selfupdate)
		SetEnvironmentVariableW(L"ConEmuArgs", cmdLine);
		
		// Теперь проверяем наличие слеша
		if (*pszStart != L'/') {
			params = (uint)-1;
			cmdNew = cmdLine;
			
		} else {
			// Если ком.строка содержит "/cmd" - все что после него используется для создания нового процесса
		    cmdNew = wcsstr(cmdLine, L"/cmd");
		    if (cmdNew)
		    {
		        *cmdNew = 0;
		        cmdNew += 5;
		    }
			// cmdLine готов к разбору
			SplitCommandLine(cmdLine, &params);
		}
	}
	
	return TRUE;
}

void ResetConman()
{
	HKEY hk = 0;
	DWORD dw = 0;
    // сбрость CreateInNewEnvironment для ConMan
    if (0 == RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\HoopoePG_2x"),
	        NULL, NULL, NULL, KEY_ALL_ACCESS, NULL, &hk, &dw))
	{
		RegSetValueEx(hk, _T("CreateInNewEnvironment"), NULL, REG_DWORD,
			(LPBYTE)&(dw=0), 4);
		RegCloseKey(hk);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	/*int nCmp;
	nCmp = StrCmpI(L" ", L"A"); // -1
	nCmp = StrCmpI(L" ", L"+");
	nCmp = StrCmpI(L" ", L"-");
	nCmp = wcscmp(L" ", L"-");
	nCmp = wcsicmp(L" ", L"-");
	nCmp = lstrcmp(L" ", L"-");
	nCmp = lstrcmpi(L" ", L"-");
	nCmp = StrCmpI(L" ", L"\\");*/

    g_hInstance = hInstance;
    gpNullSecurity = NullSecurity();

    #ifdef _DEBUG
	if (_tcsstr(GetCommandLine(), L"/debugi")) {
		if (!IsDebuggerPresent()) _ASSERT(FALSE);
	} else
	#endif
	if (_tcsstr(GetCommandLine(), L"/debug")) {
		if (!IsDebuggerPresent()) MBoxA(L"Conemu started");
	}
	

    //pVCon = NULL;

    //bool setParentDisabled=false;
    bool ClearTypePrm = false;
    bool FontPrm = false; TCHAR* FontVal = NULL; //wchar_t szTempFontFam[MAX_PATH];
    bool SizePrm = false; LONG SizeVal = 0;
    bool BufferHeightPrm = false; int BufferHeightVal = 0;
    bool ConfigPrm = false; TCHAR* ConfigVal = NULL;
	//bool FontFilePrm = false; TCHAR* FontFile = NULL; //ADD fontname; by Mors
	bool WindowPrm = false; int WindowModeVal = 0;
	bool AttachPrm = false; LONG AttachVal=0;
	bool MultiConPrm = false, MultiConValue = false;
	bool VisPrm = false, VisValue = false;
	bool SingleInstance = false;

    //gConEmu.cBlinkShift = GetCaretBlinkTime()/15;

    memset(&gOSVer, 0, sizeof(gOSVer));
    gOSVer.dwOSVersionInfoSize = sizeof(gOSVer);
    GetVersionEx(&gOSVer);
    
    //DisableIME();

    //Windows7 - SetParent для консоли валится
    //gConEmu.setParent = false; // PictureView теперь идет через Wrapper
    //if ((gOSVer.dwMajorVersion>6) || (gOSVer.dwMajorVersion==6 && gOSVer.dwMinorVersion>=1))
    //{
    //    setParentDisabled = true;
    //}
    //if (gOSVer.dwMajorVersion>=6)
    //{
	//    CheckConIme();
    //}
    
    //gSet.InitSettings();


//------------------------------------------------------------------------
///| Parsing the command line |///////////////////////////////////////////
//------------------------------------------------------------------------

    TCHAR* cmdNew = NULL;
    TCHAR *cmdLine = NULL;
    TCHAR *psUnknown = NULL;
    uint  params = 0;
    
    
    
    if (!PrepareCommandLine(/*OUT*/cmdLine, /*OUT*/cmdNew, /*OUT*/params))
		return 100;

	
    
	if (params && params != (uint)-1)
    {
    	TCHAR *curCommand = cmdLine;
		TODO("Если первый (после запускаемого файла) аргумент начинается НЕ с '/' - завершить разбор параметров и не заменять '""' на пробелы");
        //uint params; SplitCommandLine(curCommand, &params);

		//if(params < 1) {
        //    curCommand = NULL;
		//}
        // Parse parameters.
        // Duplicated parameters are permitted, the first value is used.
		uint i = 0; // ммать... curCommand увеличивался, а i НЕТ
        while (i < params && curCommand && *curCommand)
        {
	        if ( !klstricmp(curCommand, _T("/autosetup")) ) {
		        BOOL lbTurnOn = TRUE;
		        if ((i + 1) >= params)
			        return 101;
	        
		        curCommand += _tcslen(curCommand) + 1; i++;
		        if (*curCommand == _T('0'))
			        lbTurnOn = FALSE;
			    else {
			        if ((i + 1) >= params)
				        return 101;
				    curCommand += _tcslen(curCommand) + 1; i++;
				    DWORD dwAttr = GetFileAttributes(curCommand);
				    if (dwAttr == (DWORD)-1 || (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
					    return 102;
			    }

		        HKEY hk = NULL; DWORD dw;
		        int nSetupRc = 100;
		        if (0 != RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Command Processor"),
			        NULL, NULL, NULL, KEY_ALL_ACCESS, NULL, &hk, &dw))
			        return 103;
			    if (lbTurnOn) {
				    BOOL bNeedFree = FALSE;
				    if (*curCommand!=_T('"') && _tcschr(curCommand, _T(' '))) {
					    TCHAR* psz = (TCHAR*)calloc(_tcslen(curCommand)+3, sizeof(TCHAR));
					    *psz = _T('"');
					    _tcscpy(psz+1, curCommand);
					    _tcscat(psz, _T("\""));
					    curCommand = psz;
				    }
				    if (0==RegSetValueEx(hk, _T("AutoRun"), NULL, REG_SZ, (LPBYTE)curCommand,
						    sizeof(TCHAR)*(_tcslen(curCommand)+1)))
					    nSetupRc = 1;
					if (bNeedFree) 
						free(curCommand);
			    } else {
				    if (0==RegDeleteValue(hk, _T("AutoRun")))
					    nSetupRc = 1;
			    }
		        RegCloseKey(hk);
		        // сбрость CreateInNewEnvironment для ConMan
		        ResetConman();
		        return nSetupRc;
	        }
			else if ( !klstricmp(curCommand, _T("/multi")) ) {
				MultiConValue = true; MultiConPrm = true;
			}
			else if ( !klstricmp(curCommand, _T("/nomulti")) ) {
				MultiConValue = false; MultiConPrm = true;
			}
			else if ( !klstricmp(curCommand, _T("/visible")) ) {
				VisValue = true; VisPrm = true;
			}
			else if ( !klstricmp(curCommand, _T("/detached")) ) {
				gConEmu.mb_StartDetached = TRUE;
			}
            else if ( !klstricmp(curCommand, _T("/ct")) || !klstricmp(curCommand, _T("/cleartype")) )
            {
                ClearTypePrm = true;
            }

			// имя шрифта
            else if ( !klstricmp(curCommand, _T("/font")) && i + 1 < params)
            {
                curCommand += _tcslen(curCommand) + 1; i++;
                if (!FontPrm)
                {
                    FontPrm = true;
                    FontVal = curCommand;
                }
            }

			// Высота шрифта
            else if ( !klstricmp(curCommand, _T("/size")) && i + 1 < params)
            {
                curCommand += _tcslen(curCommand) + 1; i++;
                if (!SizePrm)
                {
                    SizePrm = true;
                    SizeVal = klatoi(curCommand);
                }
            }
            else if ( !klstricmp(curCommand, _T("/attach")) /*&& i + 1 < params*/)
            {
                //curCommand += _tcslen(curCommand) + 1; i++;
                if (!AttachPrm)
                {
                    AttachPrm = true; AttachVal = -1;
                    if ((i + 1) < params)
                    {
	                    TCHAR *nextCommand = curCommand + _tcslen(curCommand) + 1;
	                    if (*nextCommand != _T('/')) {
		                    curCommand = nextCommand; i++;
		                    AttachVal = klatoi(curCommand);
		                }
	                }
	                // интеллектуальный аттач - если к текущей консоли уже подцеплена другая копия
	                if (AttachVal == -1) {
		                HWND hCon = GetForegroundWindow();
		                if (!hCon) {
							// консоли нет
							return 100;
						} else {
			                TCHAR sClass[128];
			                if (GetClassName(hCon, sClass, 128)) {
				                if (_tcscmp(sClass, VirtualConsoleClassMain)==0) {
					                // Сверху УЖЕ другая копия ConEmu
					                return 1;
				                }
								// Если на самом верху НЕ консоль - это может быть панель проводника, 
								// или другое плавающее окошко... Поищем ВЕРХНЮЮ консоль
								if (_tcscmp(sClass, _T("ConsoleWindowClass"))!=0) {
									_tcscpy(sClass, _T("ConsoleWindowClass"));
									hCon = FindWindow(_T("ConsoleWindowClass"), NULL);
									if (!hCon)
										return 100;
								}
				                if (_tcscmp(sClass, _T("ConsoleWindowClass"))==0) {
					                // перебрать все ConEmu, может кто-то уже подцеплен?
					                HWND hEmu = NULL;
					                while ((hEmu = FindWindowEx(NULL, hEmu, VirtualConsoleClassMain, NULL)) != NULL)
					                {
						                if (hCon == (HWND)GetWindowLongPtr(hEmu, GWLP_USERDATA)) {
							                // к этой консоли уже подцеплен ConEmu
							                return 1;
						                }
					                }
								} else {
									// верхнее окно - НЕ консоль
									return 100;
								}
			                }
		                }
						gSet.hAttachConWnd = hCon;
	                }
                }
            }
            //Start ADD fontname; by Mors
            else if ( !klstricmp(curCommand, _T("/fontfile")) && i + 1 < params)
            {
                curCommand += _tcslen(curCommand) + 1; i++;
                //if (!FontFilePrm)
                {
                    //FontFilePrm = true;
					int nLen = _tcslen(curCommand);
					if (nLen >= MAX_PATH)
					{
						TCHAR* psz=(TCHAR*)calloc(nLen+100,sizeof(TCHAR));
						wsprintf(psz, _T("Too long /FontFile name (%i chars).\r\n"), nLen);
						_tcscat(psz, curCommand);
						MBoxA(psz);
						free(psz); free(cmdLine);
						return 100;
					}
                    //FontFile = curCommand;
                    gSet.RegisterFont(curCommand, TRUE);
                }
            }
            //End ADD fontname; by Mors
            else if ( !klstricmp(curCommand, _T("/fs")))
            {
                WindowModeVal = rFullScreen; WindowPrm = true;
            }
            else if ( !klstricmp(curCommand, _T("/max")))
            {
                WindowModeVal = rMaximized; WindowPrm = true;
            }
            else if ( !klstricmp(curCommand, L"/log") || !klstricmp(curCommand, L"/log0")  || !klstricmp(curCommand, L"/log1") )
            {
	            gSet.isAdvLogging = 1;
	        }
            else if ( !klstricmp(curCommand, _T("/log2")))
            {
	            gSet.isAdvLogging = 2;
	        }
            else if ( !klstricmp(curCommand, _T("/log3")))
            {
	            gSet.isAdvLogging = 3;
	        }
	        else if ( !klstricmp(curCommand, _T("/single")) )
	        {
	        	SingleInstance = true;
	        }
            //else if ( !klstricmp(curCommand, _T("/DontSetParent")) || !klstricmp(curCommand, _T("/Windows7")) )
            //{
            //    setParentDisabled = true;
            //}
            //else if ( !klstricmp(curCommand, _T("/SetParent")) )
            //{
            //    gConEmu.setParent = true;
            //}
            //else if ( !klstricmp(curCommand, _T("/SetParent2")) )
            //{
            //    gConEmu.setParent = true; gConEmu.setParent2 = true;
            //}
            else if (!klstricmp(curCommand, _T("/BufferHeight")) && i + 1 < params)
            {
                curCommand += _tcslen(curCommand) + 1; i++;
                if (!BufferHeightPrm)
                {
                    BufferHeightPrm = true;
                    BufferHeightVal = klatoi(curCommand);
                    if (BufferHeightVal < 0)
                    {
                        //setParent = true; -- Maximus5 - нефиг, все ручками
                        BufferHeightVal = -BufferHeightVal;
                    }
                }
            }
            else if (!klstricmp(curCommand, _T("/Config")) && i + 1 < params)
            {
                curCommand += _tcslen(curCommand) + 1; i++;
                if (!ConfigPrm)
                {
                    ConfigPrm = true;
                    const int maxConfigNameLen = 127;
                    int nLen = _tcslen(curCommand);
                    if (nLen > maxConfigNameLen)
                    {
	                    TCHAR* psz=(TCHAR*)calloc(nLen+100,sizeof(TCHAR));
	                    wsprintf(psz, _T("Too long /Config name (%i chars).\r\n"), nLen);
	                    _tcscat(psz, curCommand);
                        MBoxA(psz);
                        free(psz); free(cmdLine);
                        return 100;
                    }
                    ConfigVal = curCommand;
                }
            }
            else if ( !klstricmp(curCommand, _T("/?")))
            {
                MessageBox(NULL, pHelp, L"About ConEmu...", MB_ICONQUESTION);
                free(cmdLine);
                return -1; // NightRoman
            } else if (i>0 && !psUnknown && *curCommand) {
	            // ругнуться на неизвестную команду
	            psUnknown = curCommand;
            }

			curCommand += _tcslen(curCommand) + 1; i++;
        }
    }
    //if (setParentDisabled && (gConEmu.setParent || gConEmu.setParent2)) {
    //    gConEmu.setParent=false; gConEmu.setParent2=false;
    //}

    if (psUnknown) {
	    TCHAR* psMsg = (TCHAR*)calloc(_tcslen(psUnknown)+100,sizeof(TCHAR));
	    _tcscpy(psMsg, _T("Unknown command specified:\r\n"));
	    _tcscat(psMsg, psUnknown);
	    MBoxA(psMsg);
	    free(psMsg);
	    return 100;
    }

        
//------------------------------------------------------------------------
///| load settings and apply parameters |/////////////////////////////////
//------------------------------------------------------------------------
        
    // set config name before settings (i.e. where to load from)
    if (ConfigPrm)
    {
        _tcscat(gSet.Config, _T("\\"));
        _tcscat(gSet.Config, ConfigVal);
    }

    // load settings from registry
    gSet.LoadSettings();

	//#pragma message("Win2k: CLEARTYPE_NATURAL_QUALITY")
    //if (ClearTypePrm)
    //    gSet.LogFont.lfQuality = CLEARTYPE_NATURAL_QUALITY;
    //if (FontPrm)
    //    _tcscpy(gSet.LogFont.lfFaceName, FontVal);
    //if (SizePrm)
    //    gSet.LogFont.lfHeight = SizeVal;
    if (BufferHeightPrm) {
        gSet.SetArgBufferHeight ( BufferHeightVal );
    }
	if (!WindowPrm) {
		if (nCmdShow == SW_SHOWMAXIMIZED)
			gConEmu.WindowMode = rMaximized;
	} else {
		gConEmu.WindowMode = WindowModeVal;
	}
	if (MultiConPrm)
		gSet.isMulti = MultiConValue;
	if (VisValue)
		gSet.isConVisible = VisPrm;
	// Если запускается conman (нафига?) - принудительно включить флажок "Обновлять handle"
	TODO("Deprecated: isUpdConHandle использоваться не должен");
	if (gSet.isMulti || StrStrI(gSet.GetCmd(), L"conman.exe")) {
		gSet.isUpdConHandle = TRUE;
        // сбрость CreateInNewEnvironment для ConMan
        ResetConman();
	}


    // Установка параметров из командной строки
	if (cmdNew) {
		MCHKHEAP
		const wchar_t* pszDefCmd = NULL;
		wchar_t* pszReady = NULL;
		
		int nLen = lstrlen(cmdNew)+1;
		if (params == (uint)-1) {
			pszDefCmd = gSet.GetCmd();
			_ASSERTE(pszDefCmd && *pszDefCmd);
			nLen += 3 + lstrlen(pszDefCmd);
		}
		
		pszReady = (TCHAR*)malloc(nLen*sizeof(TCHAR));
		_ASSERTE(pszReady);
		
		if (pszDefCmd) {
			lstrcpy(pszReady, pszDefCmd);
			lstrcat(pszReady, L" ");
			lstrcat(pszReady, cmdNew);
		} else {
			lstrcpy(pszReady, cmdNew);
		}
		MCHKHEAP
		
		SafeFree(gSet.psCurCmd); // могло быть создано в gSet.GetCmd()
		gSet.psCurCmd = pszReady; pszReady = NULL;
	}
		
	//if (FontFilePrm) {
	//	if (!AddFontResourceEx(FontFile, FR_PRIVATE, NULL)) //ADD fontname; by Mors
	//	{
	//		TCHAR* psz=(TCHAR*)calloc(wcslen(FontFile)+100,sizeof(TCHAR));
	//		lstrcpyW(psz, L"Can't register font:\n");
	//		lstrcatW(psz, FontFile);
	//		MessageBox(NULL, psz, L"ConEmu", MB_OK|MB_ICONSTOP);
	//		free(psz);
	//		return 100;
	//	}
	//	lstrcpynW(gSet.FontFile, FontFile, countof(gSet.FontFile));
	//}
	// else if (gSet.isSearchForFont && gConEmu.ms_ConEmuExe[0]) {
	//	if (FindFontInFolder(szTempFontFam)) {
	//		// Шрифт уже зарегистрирован
	//		FontFilePrm = true;
	//		FontPrm = true;
	//		FontVal = szTempFontFam;
	//		FontFile = gSet.FontFile;
	//	}
	//}
	
	gSet.RegisterFonts();


	gSet.InitFont(
		FontPrm ? FontVal : NULL,
		SizePrm ? SizeVal : -1,
		ClearTypePrm ? CLEARTYPE_NATURAL_QUALITY : -1
		);

		
///////////////////////////////////

	if (SingleInstance) {
		// При запуске серии закладок из cmd файла второму экземпляру лучше чуть-чуть подождать
		Sleep(1000);
		// Поехали
		DWORD dwStart = GetTickCount();
		while (!gConEmu.isFirstInstance()) {
			if (gConEmu.RunSingleInstance())
				return 0; // командная строка успешно запущена в существующем экземпляре

			// Если ожидание длится более 10 секунд - запускаемся самостоятельно
			if ((GetTickCount() - dwStart) > 10*1000)
				break;
		}
	} else {
		// Иницилизировать событие все-равно нужно
		gConEmu.isFirstInstance();
	}
 
    
//------------------------------------------------------------------------
///| Allocating console |/////////////////////////////////////////////////
//------------------------------------------------------------------------

	BOOL lbConsoleAllocated = FALSE;
    if (AttachPrm) {
	    if (!AttachVal) {
		    MBoxA(_T("Invalid <process id> specified in the /Attach argument"));
		    //delete pVCon;
		    return 100;
	    }
	    gSet.nAttachPID = AttachVal;
    }


//------------------------------------------------------------------------
///| Create taskbar window |//////////////////////////////////////////////
//------------------------------------------------------------------------

    // Тут загружаются иконки, и создается кнопка на таскбаре (если надо)
    if (!CreateAppWindow()) {
	    //delete pVCon;
	    return 100;
    }

//------------------------------------------------------------------------
///| Creating window |////////////////////////////////////////////////////
//------------------------------------------------------------------------

    if (!gConEmu.CreateMainWindow()) {
	    free(cmdLine);
	    //delete pVCon;
	    return 100;
	}
    
    


//------------------------------------------------------------------------
///| Misc |///////////////////////////////////////////////////////////////
//------------------------------------------------------------------------

	gConEmu.PostCreate();

   
//------------------------------------------------------------------------
///| Main message loop |//////////////////////////////////////////////////
//------------------------------------------------------------------------

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	MessageLoop();
    
//------------------------------------------------------------------------
///| Deinitialization |///////////////////////////////////////////////////
//------------------------------------------------------------------------
    
    //KillTimer(ghWnd, 0);
    //delete pVCon;
    //CloseHandle(hChildProcess); -- он более не требуется

    //if (FontFilePrm) RemoveFontResourceEx(FontFile, FR_PRIVATE, NULL); //ADD fontname; by Mors
    
    gSet.UnregisterFonts();

	if (lbConsoleAllocated)
		FreeConsole();

    free(cmdLine);

	//CoUninitialize();
	OleUninitialize();

    return 0;
}
