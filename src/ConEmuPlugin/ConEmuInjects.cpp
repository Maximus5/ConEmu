
#define DROP_SETCP_ON_WIN2K3R2

// Иначе не опередяется GetConsoleAliases (хотя он должен быть доступен в Win2k)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <TCHAR.h>
#include <Tlhelp32.h>
#include <shlwapi.h>
#include "..\common\common.hpp"
#include "..\common\ConEmuCheck.h"

#ifdef _DEBUG
	#include <crtdbg.h>
#else
	#ifndef _ASSERTE
		#define _ASSERTE(x)
	#endif
#endif

#define DebugString(s) OutputDebugString(s)

extern HMODULE ghPluginModule;

extern HWND ConEmuHwnd; // Содержит хэндл окна отрисовки. Это ДОЧЕРНЕЕ окно.
extern DWORD gnMainThreadId;
extern BOOL gbFARuseASCIIsort;
extern DWORD gdwServerPID;

static bool gbConEmuInput = false; // TRUE если консоль спрятана и должна работать через очередь ConEmu

extern CESERVER_REQ_CONINFO_HDR *gpConsoleInfo;

//WARNING("Все SendMessage нужно переделать на PipeExecute, т.к. из 'Run as' в Win7 нифига не пошлется");

static void* GetOriginalAddress( void* OurFunction, void* DefaultFunction, BOOL abAllowModified );
static HMODULE WINAPI OnLoadLibraryW( const WCHAR* lpFileName );
static HMODULE WINAPI OnLoadLibraryA( const char* lpFileName );
static HMODULE WINAPI OnLoadLibraryExW( const WCHAR* lpFileName, HANDLE hFile, DWORD dwFlags );
static HMODULE WINAPI OnLoadLibraryExA( const char* lpFileName, HANDLE hFile, DWORD dwFlags );

#define ORIGINAL(n) \
	BOOL bMainThread = (GetCurrentThreadId() == gnMainThreadId); \
	void* f##n = GetOriginalAddress(On##n, ##n, bMainThread);

static void GuiSetForeground(HWND hWnd)
{
	if (ConEmuHwnd) {
		CESERVER_REQ In, *pOut;
		ExecutePrepareCmd(&In, CECMD_SETFOREGROUND, sizeof(CESERVER_REQ_HDR)+sizeof(u64));
		In.qwData[0] = (u64)hWnd;
		HWND hConWnd = GetConsoleWindow();
		pOut = ExecuteGuiCmd(hConWnd, &In, hConWnd);
		if (pOut) ExecuteFreeResult(pOut);
	}
}

static void GuiFlashWindow(BOOL bSimple, HWND hWnd, BOOL bInvert, DWORD dwFlags, UINT uCount, DWORD dwTimeout)
{
	if (ConEmuHwnd) {
		CESERVER_REQ In, *pOut;
		ExecutePrepareCmd(&In, CECMD_FLASHWINDOW, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_FLASHWINFO));
		In.Flash.bSimple = bSimple;
		In.Flash.hWnd = hWnd;
		In.Flash.bInvert = bInvert;
		In.Flash.dwFlags = dwFlags;
		In.Flash.uCount = uCount;
		In.Flash.dwTimeout = dwTimeout;
		HWND hConWnd = GetConsoleWindow();
		pOut = ExecuteGuiCmd(hConWnd, &In, hConWnd);
		if (pOut) ExecuteFreeResult(pOut);
	}
}

static BOOL SrvSetConsoleCP(BOOL bSetOutputCP, DWORD nCP)
{
	_ASSERTE(ConEmuHwnd);

	BOOL lbRc = FALSE;

	if (gdwServerPID) {
		// Проверить живость процесса
		HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, gdwServerPID);
		if (hProcess) {
			if (WaitForSingleObject(hProcess,0) == WAIT_OBJECT_0)
				gdwServerPID = 0; // Процесс сервера завершился
			CloseHandle(hProcess);
		} else {
			gdwServerPID = 0;
		}
	}

	if (gdwServerPID) {
#ifndef DROP_SETCP_ON_WIN2K3R2
		CESERVER_REQ In, *pOut;
		ExecutePrepareCmd(&In, CECMD_SETCONSOLECP, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETCONCP));
		In.SetConCP.bSetOutputCP = bSetOutputCP;
		In.SetConCP.nCP = nCP;
		HWND hConWnd = GetConsoleWindow();
		pOut = ExecuteSrvCmd(gdwServerPID, &In, hConWnd);
		if (pOut) {
			if (pOut->hdr.nSize >= In.hdr.nSize) {
				lbRc = pOut->SetConCP.bSetOutputCP;
				if (!lbRc)
					SetLastError(pOut->SetConCP.nCP);
			}
			ExecuteFreeResult(pOut);
		}
#else
		lbRc = TRUE;
#endif
	} else {
		if (bSetOutputCP)
			lbRc = SetConsoleOutputCP(nCP);
		else
			lbRc = SetConsoleCP(nCP);
	}

	return lbRc;
}



static BOOL WINAPI OnTrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect)
{
	_ASSERTE(TrackPopupMenu!=OnTrackPopupMenu);
	
	#ifdef _DEBUG
		WCHAR szMsg[128]; wsprintf(szMsg, L"TrackPopupMenu(hwnd=0x%08X)\n", (DWORD)hWnd);
		DebugString(szMsg);
	#endif
	
	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		//UINT nSetFore = RegisterWindowMessage(CONEMUMSG_SETFOREGROUND);
		//DWORD_PTR dwRc = 0;
		//SendMessageTimeout(GetParent(ConEmuHwnd), nSetFore, 0, (LPARAM)hWnd, SMTO_NORMAL, 1000, &dwRc);
		GuiSetForeground(hWnd);
	}
	return TrackPopupMenu(hMenu, uFlags, x, y, nReserved, hWnd, prcRect);
}

static BOOL WINAPI OnTrackPopupMenuEx(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm)
{
	_ASSERTE(OnTrackPopupMenuEx != TrackPopupMenuEx);
	
	#ifdef _DEBUG
		WCHAR szMsg[128]; wsprintf(szMsg, L"TrackPopupMenuEx(hwnd=0x%08X)\n", (DWORD)hWnd);
		DebugString(szMsg);
	#endif
	
	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		//UINT nSetFore = RegisterWindowMessage(CONEMUMSG_SETFOREGROUND);
		//DWORD_PTR dwRc = 0;
		//SendMessageTimeout(GetParent(ConEmuHwnd), nSetFore, 0, (LPARAM)hWnd, SMTO_NORMAL, 1000, &dwRc);
		GuiSetForeground(hWnd);
	}
	return TrackPopupMenuEx(hmenu, fuFlags, x, y, hWnd, lptpm);
}

static BOOL WINAPI OnShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo)
{
	_ASSERTE(OnShellExecuteExA != ShellExecuteExA);
	
	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		if (!lpExecInfo->hwnd || lpExecInfo->hwnd == GetConsoleWindow())
			lpExecInfo->hwnd = GetParent(ConEmuHwnd);
	}
	return ShellExecuteExA(lpExecInfo);
}
static BOOL WINAPI OnShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo)
{
	_ASSERTE(OnShellExecuteExW != ShellExecuteExW);
	
	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		if (!lpExecInfo->hwnd || lpExecInfo->hwnd == GetConsoleWindow())
			lpExecInfo->hwnd = GetParent(ConEmuHwnd);
	}
	return ShellExecuteExW(lpExecInfo);
}

static HINSTANCE WINAPI OnShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd)
{
	_ASSERTE(OnShellExecuteA != ShellExecuteA);
	
	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		if (!hwnd || hwnd == GetConsoleWindow())
			hwnd = GetParent(ConEmuHwnd);
	}
	return ShellExecuteA(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
}
static HINSTANCE WINAPI OnShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	_ASSERTE(OnShellExecuteW != ShellExecuteW);
	
	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		if (!hwnd || hwnd == GetConsoleWindow())
			hwnd = GetParent(ConEmuHwnd);
	}
	return ShellExecuteW(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
}

static BOOL WINAPI OnFlashWindow(HWND hWnd, BOOL bInvert)
{
	_ASSERTE(FlashWindow != OnFlashWindow);

	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		GuiFlashWindow(TRUE, hWnd, bInvert, 0,0,0);
		return TRUE;
		//UINT nFlash = RegisterWindowMessage(CONEMUMSG_FLASHWINDOW);
		//DWORD_PTR dwRc = 0;
		//WPARAM wParam = (bInvert ? 2 : 1) << 25;
		//LRESULT lRc = SendMessageTimeout(GetParent(ConEmuHwnd), nFlash, wParam, (LPARAM)hWnd, SMTO_NORMAL, 1000, &dwRc);
		//return dwRc!=0;
	}
	return FlashWindow(hWnd, bInvert);
}
static BOOL WINAPI OnFlashWindowEx(PFLASHWINFO pfwi)
{
	_ASSERTE(FlashWindowEx != OnFlashWindowEx);

	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		GuiFlashWindow(FALSE, pfwi->hwnd, 0, pfwi->dwFlags, pfwi->uCount, pfwi->dwTimeout);
		return TRUE;
		//UINT nFlash = RegisterWindowMessage(CONEMUMSG_FLASHWINDOW);
		//DWORD_PTR dwRc = 0;
		//WPARAM wParam = ((pfwi->dwFlags & 0xF) << 24) | (pfwi->uCount & 0xFFFFFF);
		//LRESULT lRc = SendMessageTimeout(GetParent(ConEmuHwnd), nFlash, wParam, (LPARAM)pfwi->hwnd, SMTO_NORMAL, 1000, &dwRc);
		//return dwRc!=0;
	}
	return FlashWindowEx(pfwi);
}

static BOOL WINAPI OnSetForegroundWindow(HWND hWnd)
{
	_ASSERTE(SetForegroundWindow != OnSetForegroundWindow);
	
	if (ConEmuHwnd /*&& IsWindow(ConEmuHwnd)*/) {
		GuiSetForeground(hWnd);
		//UINT nSetFore = RegisterWindowMessage(CONEMUMSG_SETFOREGROUND);
		//DWORD_PTR dwRc = 0;
		//LRESULT lRc = SendMessageTimeout(GetParent(ConEmuHwnd), nSetFore, 0, (LPARAM)hWnd, SMTO_NORMAL, 1000, &dwRc);
		//return lRc!=0;
	}
	
	return SetForegroundWindow(hWnd);
}

#ifndef NORM_STOP_ON_NULL
#define NORM_STOP_ON_NULL 0x10000000
#endif

//static int WINAPI OnCompareStringA(LCID Locale, DWORD dwCmpFlags, LPCSTR lpString1, int cchCount1, LPCSTR lpString2, int cchCount2)
//static int WINAPI OnlstrcmpiA(LPCSTR lpString1, LPCSTR lpString2)
//{
//	_ASSERTE(OnlstrcmpiA!=lstrcmpiA);
//	int nCmp = 0;
//	
//	if (gbFARuseASCIIsort)
//	{
//		char ch1 = *lpString1++, ch10 = 0, ch2 = *lpString2++, ch20 = 0;
//		while (!nCmp && ch1)
//		{
//			if (ch1==ch2) {
//				if (!ch1) break;
//			} else if (ch1<0x80 && ch2<0x80) {
//				if (ch1>='A' && ch1<='Z') ch1 |= 0x20;
//				if (ch2>='A' && ch2<='Z') ch2 |= 0x20;
//				nCmp = (ch1==ch2) ? 0 : (ch1<ch2) ? -1 : 1;
//			} else {
//				nCmp = CompareStringA(LOCALE_USER_DEFAULT, NORM_IGNORECASE|NORM_STOP_ON_NULL|SORT_STRINGSORT, &ch1, 1, &ch2, 1)-2;
//			}
//			if (!ch1 || !ch2 || !nCmp) break;
//			ch1 = *lpString1++;
//			ch2 = *lpString2++;
//		}
//	} else {
//		nCmp = lstrcmpiA(lpString1, lpString2);
//	}
//	return nCmp;
//}

static int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2)
{
	_ASSERTE(OnCompareStringW!=CompareStringW);
	int nCmp = -1;
	
	if (gbFARuseASCIIsort)
	{
		//if (dwCmpFlags == (NORM_IGNORECASE|SORT_STRINGSORT) && cchCount1 == -1 && cchCount2 == -1) {
		//	//int n = lstrcmpiW(lpString1, lpString2);
		//	//nCmp = (n<0) ? 1 : (n>0) ? 3 : 2;
		//	
		//} else
		
		if (dwCmpFlags == (NORM_IGNORECASE|NORM_STOP_ON_NULL|SORT_STRINGSORT) /*&& GetCurrentThreadId()==gnMainThreadId*/) {
			//size_t nLen1 = lstrlen(lpString1), nLen2 = lstrlen(lpString2);
			
			//bool bUseWcs = true; // выбор типа сравнивания
			
			//if (bUseWcs) {
				//nCmp = CompareStringW(Locale, dwCmpFlags, lpString1, cchCount1, lpString2, cchCount2);
				int n = 0;
				//WCHAR ch1[2], ch2[2]; ch1[1] = ch2[1] = 0;
				WCHAR ch1 = *lpString1++, ch10 = 0, ch2 = *lpString2++, ch20 = 0;
				int n1 = (cchCount1==cchCount2) ? cchCount1
					: (cchCount1!=-1 && cchCount2!=-1) ? -1 
						: (cchCount1!=-1) ? cchCount2 
							: (cchCount2!=-1) ? cchCount1 : min(cchCount1,cchCount2);
				while (!n && /*(ch1=*lpString1++) && (ch2=*lpString2++) &&*/ n1--)
				{
					if (ch1==ch2) {
						if (!ch1) break;
						// n = 0;
					} else if (ch1<0x80 && ch2<0x80) {
						if (ch1>=L'A' && ch1<=L'Z') ch1 |= 0x20;
						if (ch2>=L'A' && ch2<=L'Z') ch2 |= 0x20;
						n = (ch1==ch2) ? 0 : (ch1<ch2) ? -1 : 1;
					} else {
						n = CompareStringW(Locale, dwCmpFlags, &ch1, 1, &ch2, 1)-2;
					}
					if (!ch1 || !ch2 || !n1) break;
					ch1 = *lpString1++;
					ch2 = *lpString2++;
				}
				//if (!n && ((ch1!=0) ^ (ch2!=0)))
					//n = _wcsnicmp(ch1, ch2, 1);
					//nCmp = lstrcmpiW(ch1, ch2);
					//nCmp = StrCmpNI(lpString1++, lpString2++, 1);
				nCmp = (n<0) ? 1 : (n>0) ? 3 : 2;
			//}
			//nCmp += 2;
			//static WCHAR temp[MAX_PATH+1];
			//size_t nLen1 = lstrlen(lpString1), nLen2 = lstrlen(lpString2);
			//if (nLen1 && nLen2 && nLen1 <= MAX_PATH && nLen2 <= MAX_PATH) {
			//	if (nLen1 == nLen2) {
			//		nCmp = lstrcmpiW(lpString1, lpString2)+2;
			//	} else if (nLen1 < nLen2) {
			//		lstrcpyn(temp, lpString2, nLen1+1);
			//		nCmp = lstrcmpiW(lpString1, temp)+2;
			//	} else if (nLen1 > nLen2) {
			//		lstrcpyn(temp, lpString1, nLen2+1);
			//		nCmp = lstrcmpiW(temp, lpString2)+2;
			//	}
			//}
		}
	}
	
	if (nCmp == -1)
		nCmp = CompareStringW(Locale, dwCmpFlags, lpString1, cchCount1, lpString2, cchCount2);
	return nCmp;
}

static DWORD WINAPI OnGetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName)
{
	_ASSERTE(OnGetConsoleAliasesW!=GetConsoleAliasesW);
	DWORD nError = 0;
	DWORD nRc = GetConsoleAliasesW(AliasBuffer,AliasBufferLength,ExeName);
	if (!nRc) {
		nError = GetLastError();
		// финт ушами
		if (nError == ERROR_NOT_ENOUGH_MEMORY && gdwServerPID) {
			CESERVER_REQ_HDR In;
			ExecutePrepareCmd((CESERVER_REQ*)&In, CECMD_GETALIASES,sizeof(CESERVER_REQ_HDR));
			CESERVER_REQ* pOut = ExecuteSrvCmd(gdwServerPID, (CESERVER_REQ*)&In, GetConsoleWindow());
			if (pOut) {
				DWORD nData = min(AliasBufferLength,(pOut->hdr.nSize-sizeof(pOut->hdr)));
				if (nData) {
					memmove(AliasBuffer, pOut->Data, nData);
					nRc = TRUE;
				}
				ExecuteFreeResult(pOut);
			}
			if (!nRc)
				SetLastError(nError); // вернуть, вдруг какая функция его поменяла
		}
	}
	return nRc;
}

static BOOL WINAPI OnSetConsoleCP(UINT wCodePageID)
{
	_ASSERTE(OnSetConsoleCP!=SetConsoleCP);
	TODO("Виснет в 2k3R2 при 'chcp 866 <enter> chcp 20866 <enter>");
	BOOL lbRc = FALSE;
	if (gdwServerPID) {
		lbRc = SrvSetConsoleCP(FALSE/*bSetOutputCP*/, wCodePageID);
	} else {
		lbRc = SetConsoleCP(wCodePageID);
	}
	return lbRc;
}

static BOOL WINAPI OnSetConsoleOutputCP(UINT wCodePageID)
{
	_ASSERTE(OnSetConsoleOutputCP!=SetConsoleOutputCP);
	BOOL lbRc = FALSE;
	if (gdwServerPID) {
		lbRc = SrvSetConsoleCP(TRUE/*bSetOutputCP*/, wCodePageID);
	} else {
		lbRc = SetConsoleOutputCP(wCodePageID);
	}
	return lbRc;
}

static BOOL WINAPI OnGetNumberOfConsoleInputEvents(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents)
{
	_ASSERTE(OnGetNumberOfConsoleInputEvents!=GetNumberOfConsoleInputEvents);
	if (gpConsoleInfo) {
		if (GetCurrentThreadId() == gnMainThreadId)
			gpConsoleInfo->nFarReadIdx++;
	}
	BOOL lbRc = GetNumberOfConsoleInputEvents(hConsoleInput, lpcNumberOfEvents);
	return lbRc;
}

typedef BOOL (WINAPI* PeekConsoleInput_t)(HANDLE,PINPUT_RECORD,DWORD,LPDWORD);

static BOOL WINAPI OnPeekConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	ORIGINAL(PeekConsoleInputA);
	_ASSERTE(OnPeekConsoleInputA!=fPeekConsoleInputA);
	if (gpConsoleInfo && bMainThread) {
		gpConsoleInfo->nFarReadIdx++;
	}
	BOOL lbRc = ((PeekConsoleInput_t)fPeekConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
	return lbRc;
}

static BOOL WINAPI OnPeekConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	ORIGINAL(PeekConsoleInputW);
	_ASSERTE(OnPeekConsoleInputW!=fPeekConsoleInputW);
	if (gpConsoleInfo && bMainThread) {
		gpConsoleInfo->nFarReadIdx++;
	}
	BOOL lbRc = ((PeekConsoleInput_t)fPeekConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
	return lbRc;
}

static BOOL WINAPI OnReadConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	_ASSERTE(OnReadConsoleInputA!=ReadConsoleInputA);
	if (gpConsoleInfo) {
		if (GetCurrentThreadId() == gnMainThreadId)
			gpConsoleInfo->nFarReadIdx++;
	}
	BOOL lbRc = ReadConsoleInputA(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
	return lbRc;
}

static BOOL WINAPI OnReadConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	_ASSERTE(OnReadConsoleInputW!=ReadConsoleInputW);
	if (gpConsoleInfo) {
		if (GetCurrentThreadId() == gnMainThreadId)
			gpConsoleInfo->nFarReadIdx++;
	}
	BOOL lbRc = ReadConsoleInputW(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
	return lbRc;
}

typedef struct HookItem
{
    void*  NewAddress;
    char*  Name;
    TCHAR* DllName;
    void*  OldAddress;
	BOOL   ReplacedInExe;
	void*  ExeOldAddress;
} HookItem;

static TCHAR kernel32[] = _T("kernel32.dll");
static TCHAR user32[]   = _T("user32.dll");
static TCHAR shell32[]  = _T("shell32.dll");

//static BOOL bHooksWin2k3R2Only = FALSE;
//static HookItem HooksWin2k3R2Only[] = {
//	{OnSetConsoleCP, "SetConsoleCP", kernel32, 0},
//	{OnSetConsoleOutputCP, "SetConsoleOutputCP", kernel32, 0},
//	/* ************************ */
//	{0, 0, 0}
//};

static HookItem HooksFarOnly[] = {
//	{OnlstrcmpiA,      "lstrcmpiA",      kernel32, 0},
	{OnCompareStringW, "CompareStringW", kernel32, 0},
	/* ************************ */
	{0, 0, 0}
};

static HookItem Hooks[] = {
	// My
	{OnPeekConsoleInputW,	"PeekConsoleInputW",	kernel32, 0},
	{OnPeekConsoleInputA,	"PeekConsoleInputA",	kernel32, 0},
	{OnReadConsoleInputW,	"ReadConsoleInputW",	kernel32, 0},
	{OnReadConsoleInputA,	"ReadConsoleInputA",	kernel32, 0},
    {OnLoadLibraryA,        "LoadLibraryA",			kernel32, 0},
	{OnLoadLibraryW,        "LoadLibraryW",			kernel32, 0},
    {OnLoadLibraryExA,      "LoadLibraryExA",		kernel32, 0},
	{OnLoadLibraryExW,      "LoadLibraryExW",		kernel32, 0},
	{OnGetConsoleAliasesW,  "GetConsoleAliasesW",	kernel32, 0},
	{OnGetNumberOfConsoleInputEvents,
							"GetNumberOfConsoleInputEvents",
													kernel32, 0},
	{OnTrackPopupMenu,      "TrackPopupMenu",		user32,   0},
	{OnTrackPopupMenuEx,    "TrackPopupMenuEx",		user32,   0},
	{OnFlashWindow,         "FlashWindow",			user32,   0},
	{OnFlashWindowEx,       "FlashWindowEx",		user32,   0},
	{OnSetForegroundWindow, "SetForegroundWindow",	user32,   0},
	{OnShellExecuteExA,     "ShellExecuteExA",		shell32,  0},
	{OnShellExecuteExW,     "ShellExecuteExW",		shell32,  0},
	{OnShellExecuteA,       "ShellExecuteA",		shell32,  0},
	{OnShellExecuteW,       "ShellExecuteW",		shell32,  0},
	/* ************************ */
	{0, 0, 0}
};



static TCHAR* ExcludedModules[] = {
    _T("conemu.dll"), _T("conemu.x64.dll"), // по идее не нужно - должно по ghPluginModule пропускаться. Но на всякий случай!
    _T("kernel32.dll"),
    // а user32.dll не нужно?
    0
};









static bool IsModuleExcluded( HMODULE module )
{
    for( int i = 0; ExcludedModules[i]; i++ )
        if( module == GetModuleHandle( ExcludedModules[i] ) )
            return true;
    return false;
}


// Заполнить поле HookItem.OldAddress (реальные процедуры из внешних библиотек)
static bool InitHooks( HookItem* item )
{
    if( !item )
        return false;

    for( int i = 0; item[i].Name; i++ )
    {
        if( !item[i].OldAddress )
        {
            HMODULE mod = GetModuleHandle( item[i].DllName );
            if( mod )
                item[i].OldAddress = GetProcAddress( mod, item[i].Name );
        }
    }
    return true;
}


// Подменить Импортируемые функции в модуле
static bool SetHook( HookItem* item, HMODULE Module = 0, BOOL abExecutable = FALSE )
{
    if( !item )
        return false;
//    __try{
    IMAGE_IMPORT_DESCRIPTOR* Import = 0;
    DWORD Size = 0;
    if( !Module )
        Module = GetModuleHandle( 0 );
    IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)Module;
    if( dos_header->e_magic == 'ZM' )
    {
        IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((char*)Module + dos_header->e_lfanew);
        if( nt_header->Signature != 0x004550 )
            return false;
        else
        {
            Import = (IMAGE_IMPORT_DESCRIPTOR*)((char*)Module +
                                         (DWORD)(nt_header->OptionalHeader.
                                         DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].
                                         VirtualAddress));
            Size = nt_header->OptionalHeader.
                                         DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
        }
    }
    else
        return false;

    // if wrong module or no import table
    if( Module == INVALID_HANDLE_VALUE || !Import )
        return false;

	#ifdef _WIN64
	_ASSERTE(sizeof(DWORD_PTR)==8);
	#else
	_ASSERTE(sizeof(DWORD_PTR)==4);
	#endif

	#ifdef _WIN64
		#define TOP_SHIFT 60
	#else
		#define TOP_SHIFT 28
	#endif

    bool res = false;
	int i;
	int nCount = Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
	//_ASSERTE(Size == (nCount * sizeof(IMAGE_IMPORT_DESCRIPTOR))); -- ровно быть не обязано
    for( i = 0; i < nCount; i++ )
    {
		if (Import[i].Name == 0)
			break;
        //DebugString( ToTchar( (char*)Module + Import[i].Name ) );
		#ifdef _DEBUG
		char* mod_name = (char*)Module + Import[i].Name;
		#endif
        IMAGE_IMPORT_BY_NAME** byname = (IMAGE_IMPORT_BY_NAME**)(Import[i].Characteristics + (DWORD_PTR)Module);
        IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)((char*)Module + Import[i].FirstThunk);
		IMAGE_THUNK_DATA* thunkO = (IMAGE_THUNK_DATA*)((char*)Module + Import[i].OriginalFirstThunk);
        for( ; thunk->u1.Function; thunk++, thunkO++, byname++)
        {
			DWORD dwIsOrdinal = (DWORD)(((DWORD_PTR)(*byname)) >> TOP_SHIFT);
			const char* pszFuncName = NULL;
			// Только для испольняемого файла (первый модуль) и если известно имя функции
			if (dwIsOrdinal != 8 && abExecutable) {
				BOOL lbValidPtr = !IsBadReadPtr(*byname, sizeof(DWORD_PTR));
				_ASSERTE(lbValidPtr);
				if (lbValidPtr) {
					lbValidPtr = !IsBadReadPtr((*byname)->Name, sizeof(DWORD_PTR));
					_ASSERTE(lbValidPtr);
					if (lbValidPtr) {
						pszFuncName = ((char*)Module)+(DWORD_PTR)((*byname)->Name);
						lbValidPtr = !IsBadStringPtrA(pszFuncName, 10);
						_ASSERTE(lbValidPtr);
						if (!lbValidPtr)
							pszFuncName = NULL;
					}
				}
			}

			int j = 0;
            for( j = 0; item[j].Name; j++ )
			{
				// OldAddress уже может отличаться от оригинального экспорта библиотеки
				// Это происходит например с PeekConsoleIntputW при наличии плагина Anamorphosis
                if( !item[j].OldAddress || (void*)thunk->u1.Function != item[j].OldAddress )
                {
					if (!pszFuncName || !abExecutable) {
						continue;
					} else {
						if (strcmp(pszFuncName, item[j].Name))
							continue;
					}
					item[j].ExeOldAddress = (void*)thunk->u1.Function;
				}

				DWORD old_protect = 0;
				VirtualProtect( &thunk->u1.Function, sizeof( thunk->u1.Function ),
					PAGE_READWRITE, &old_protect );
				thunk->u1.Function = (DWORD_PTR)item[j].NewAddress;
				VirtualProtect( &thunk->u1.Function, sizeof( DWORD ), old_protect, &old_protect );
				if (abExecutable)
					item[j].ReplacedInExe = TRUE;
				//DebugString( ToTchar( item[j].Name ) );
				res = true;
				break;
			}
        }
    }

    return res;
}

// Подменить Импортируемые функции во всех модулях процесса, загруженных ДО conemu.dll
static bool SetHookEx( HookItem* item, HMODULE inst )
{
    if( !item )
        return false;
    //MEMORY_BASIC_INFORMATION minfo;
    //char* pb = (char*)GetModuleHandle( 0 );

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
    if(snapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 module = {sizeof(module)};
		BOOL bFirst = TRUE;

        for(BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
        {
            if(module.hModule != inst && !IsModuleExcluded(module.hModule))
            {
                DebugString( module.szModule );
                SetHook( item, module.hModule, bFirst );
				if (bFirst) bFirst = FALSE;
            }
        }

        CloseHandle(snapshot);
    }

    return true;
}




// Подменить Импортируемые функции в модуле
static bool UnsetHook( const HookItem* item, HMODULE Module = 0, BOOL abExecutable = FALSE )
{
    if( !item )
        return false;
//    __try{
    IMAGE_IMPORT_DESCRIPTOR* Import = 0;
    DWORD Size = 0;
    if( !Module )
        Module = GetModuleHandle( 0 );
    IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)Module;
    if( dos_header->e_magic == 'ZM' )
    {
        IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((char*)Module + dos_header->e_lfanew);
        if( nt_header->Signature != 0x004550 )
            return false;
        else
        {
            Import = (IMAGE_IMPORT_DESCRIPTOR*)((char*)Module +
                                         (DWORD)(nt_header->OptionalHeader.
                                         DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].
                                         VirtualAddress));
            Size = nt_header->OptionalHeader.
                                         DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
        }
    }
    else
        return false;

    // if wrong module or no import table
    if( Module == INVALID_HANDLE_VALUE || !Import )
        return false;

    bool res = false;
    for( int i = 0; Import[i].Name; i++ )
    {
        //DebugString( ToTchar( (char*)Module + Import[i].Name ) );
        //char* mod_name = (char*)Module + Import[i].Name;
        //-- не используется -- IMAGE_IMPORT_BY_NAME** byname = (IMAGE_IMPORT_BY_NAME**)(Import[i].Characteristics + (DWORD_PTR)Module);
        {
            {
                IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)((char*)Module + Import[i].FirstThunk);
                for( ; thunk->u1.Function; thunk++/*, byname++*/) // byname не используется
                {
                    for( int j = 0; item[j].Name; j++ )
                    {
                    	// BugBug: в принципе, эту функцию мог захукать и другой модуль (уже после нас),
						// но лучше вернуть оригинальную, чем потом свалиться
                        if( item[j].OldAddress && (void*)thunk->u1.Function == item[j].NewAddress )
                        {
                            DWORD old_protect;
                            VirtualProtect( &thunk->u1.Function, sizeof( thunk->u1.Function ),
                                            PAGE_READWRITE, &old_protect );
							// BugBug: ExeOldAddress может отличаться от оригинального, если функция была перехвачена ДО нас
							//if (abExecutable && item[j].ExeOldAddress)
							//	thunk->u1.Function = (DWORD_PTR)item[j].ExeOldAddress;
							//else
								thunk->u1.Function = (DWORD_PTR)item[j].OldAddress;
                            VirtualProtect( &thunk->u1.Function, sizeof( DWORD ), old_protect, &old_protect );
                            //DebugString( ToTchar( item[j].Name ) );
                            res = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    return res;
}

void UnsetAllHooks()
{
	UnsetHook( HooksFarOnly, NULL, TRUE );
	
	/*if (bHooksWin2k3R2Only) {
		bHooksWin2k3R2Only = FALSE;
		UnsetHook( HooksWin2k3R2Only, NULL, TRUE );
	}*/
	
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
    if(snapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 module = {sizeof(module)};
		BOOL lbExecutable = TRUE;

        for(BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
        {
            if(module.hModule != ghPluginModule && !IsModuleExcluded(module.hModule))
            {
                DebugString( module.szModule );
                UnsetHook( Hooks, module.hModule, lbExecutable );
				if (lbExecutable) lbExecutable = FALSE;
            }
        }

        CloseHandle(snapshot);
    }
}



// Используется в том случае, если требуется выполнить оригинальную функцию, без нашей обертки
// пример в OnPeekConsoleInputW
static void* GetOriginalAddress( void* OurFunction, void* DefaultFunction, BOOL abAllowModified )
{
	for( int i = 0; Hooks[i].Name; i++ ) {
		if ( Hooks[i].NewAddress == OurFunction) {
			return (abAllowModified && Hooks[i].ExeOldAddress) ? Hooks[i].ExeOldAddress : Hooks[i].OldAddress;
		}
	}
    _ASSERT(FALSE); // сюда мы попадать не должны
    return DefaultFunction;
}



// Эту функцию нужно позвать из DllMain плагина
BOOL StartupHooks()
{
	HMODULE hKernel = GetModuleHandle( kernel32 );
	HMODULE hUser   = GetModuleHandle( user32 );
	HMODULE hShell  = GetModuleHandle( shell32 );
	if (!hShell) hShell = LoadLibrary ( shell32 );
	_ASSERTE(hKernel && hUser && hShell);
	if (!hKernel || !hUser || !hShell)
		return FALSE; // модули должны быть загружены ДО conemu.dll

	OSVERSIONINFOEX osv = {sizeof(OSVERSIONINFOEX)};
	GetVersionEx((LPOSVERSIONINFO)&osv);

	// Заполнить поле HookItem.OldAddress (реальные процедуры из внешних библиотек)
	InitHooks( Hooks );
	InitHooks( HooksFarOnly );
	//InitHooks( HooksWin2k3R2Only );

	//  Подменить Импортируемые функции в FAR.exe (пока это только сравнивание строк)
	SetHook( HooksFarOnly, NULL, TRUE );
	
	// Windows Server 2003 R2
	
	/*if (osv.dwMajorVersion==5 && osv.dwMinorVersion==2 && osv.wServicePackMajor>=2)
	{
		//DWORD dwBuild = GetSystemMetrics(SM_SERVERR2); // нихрена оно не возвращает. 0 тут :(
		bHooksWin2k3R2Only = TRUE;
		SetHook( HooksWin2k3R2Only, NULL );
	}*/
	
	// Подменить Импортируемые функции во всех модулях процесса, загруженных ДО conemu.dll
	SetHookEx( Hooks, ghPluginModule );

	// Заменить в модуле Module ЭКСпортируемые функции на подменяемые плагином нихрена
	// НЕ получится, т.к. в Win32 библиотека shell32 может быть загружена ПОСЛЕ conemu.dll
	//   что вызовет некорректные смещения функций,
	// а в Win64 смещения вообще должны быть 64битными, а структура модуля хранит только 32битные смещения
	
	return TRUE;
}




// Заменить в модуле Module ЭКСпортируемые функции на подменяемые плагином нихрена
// НЕ получится, т.к. в Win32 библиотека shell32 может быть загружена ПОСЛЕ conemu.dll
//   что вызовет некорректные смещения функций,
// а в Win64 смещения вообще должны быть 64битными, а структура модуля хранит только 32битные смещения

static HMODULE WINAPI OnLoadLibraryA( const char* lpFileName )
{
	_ASSERTE(LoadLibraryA!=OnLoadLibraryA);

    HMODULE module = LoadLibraryA( lpFileName );
    if( !module || module == ghPluginModule )
        return module;

    SetHook( Hooks, module );

	return module;
}

static HMODULE WINAPI OnLoadLibraryW( const WCHAR* lpFileName )
{
	_ASSERTE(LoadLibraryW!=OnLoadLibraryW);

	HMODULE module = LoadLibraryW( lpFileName );
	if( !module || module == ghPluginModule )
		return module;

	SetHook( Hooks, module );

	return module;
}

static HMODULE WINAPI OnLoadLibraryExA( const char* lpFileName, HANDLE hFile, DWORD dwFlags )
{
	_ASSERTE(LoadLibraryExW!=OnLoadLibraryExW);

	HMODULE module = LoadLibraryExA( lpFileName, hFile, dwFlags );
	if( !module || module == ghPluginModule )
		return module;

	SetHook( Hooks, module );

	return module;
}

static HMODULE WINAPI OnLoadLibraryExW( const WCHAR* lpFileName, HANDLE hFile, DWORD dwFlags )
{
    _ASSERTE(LoadLibraryExW!=OnLoadLibraryExW);

    HMODULE module = LoadLibraryExW( lpFileName, hFile, dwFlags );
	if( !module || module == ghPluginModule )
		return module;

	SetHook( Hooks, module );

	return module;
}
