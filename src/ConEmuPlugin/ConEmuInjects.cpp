
#include <windows.h>
#include <TCHAR.h>
#include <Tlhelp32.h>
#include "..\common\common.hpp"
#include "..\common\ConEmuCheck.h"

#ifdef _DEBUG
	#include <crtdbg.h>
#else
	#define _ASSERTE(x)
#endif

#define DebugString(s) OutputDebugString(s)

extern HMODULE ghPluginModule;
#define InfisInstance ghPluginModule

extern HWND ConEmuHwnd; // Содержит хэндл окна отрисовки. Это ДОЧЕРНЕЕ окно.

static bool gbConEmuInput = false; // TRUE если консоль спрятана и должна работать через очередь ConEmu

//WARNING("Все SendMessage нужно переделать на PipeExecute, т.к. из 'Run as' в Win7 нифига не пошлется");

static void* GetOriginalAddress( HINSTANCE module, const char* name );
static HMODULE WINAPI OnLoadLibraryW( const WCHAR* lpFileName );
static HMODULE WINAPI OnLoadLibraryA( const char* lpFileName );
static HMODULE WINAPI OnLoadLibraryExW( const WCHAR* lpFileName, HANDLE hFile, DWORD dwFlags );
static HMODULE WINAPI OnLoadLibraryExA( const char* lpFileName, HANDLE hFile, DWORD dwFlags );

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

typedef struct HookItem
{
    void*  NewAddress;
    char*  Name;
    TCHAR* DllName;
    void*  OldAddress;
} HookItem;

static TCHAR kernel32[] = _T("kernel32.dll");
static TCHAR user32[]   = _T("user32.dll");
static TCHAR shell32[]  = _T("shell32.dll");

static HookItem Hooks[] = {
	// My
    {OnLoadLibraryA,        "LoadLibraryA",        kernel32, 0},
	{OnLoadLibraryW,        "LoadLibraryW",        kernel32, 0},
    {OnLoadLibraryExA,      "LoadLibraryExA",      kernel32, 0},
	{OnLoadLibraryExW,      "LoadLibraryExW",      kernel32, 0},
	{OnTrackPopupMenu,      "TrackPopupMenu",      user32,   0},
	{OnTrackPopupMenuEx,    "TrackPopupMenuEx",    user32,   0},
	{OnFlashWindow,         "FlashWindow",         user32,   0},
	{OnFlashWindowEx,       "FlashWindowEx",       user32,   0},
	{OnSetForegroundWindow, "SetForegroundWindow", user32,   0},
	{OnShellExecuteExA,     "ShellExecuteExA",     shell32,  0},
	{OnShellExecuteExW,     "ShellExecuteExW",     shell32,  0},
	{OnShellExecuteA,       "ShellExecuteA",       shell32,  0},
	{OnShellExecuteW,       "ShellExecuteW",       shell32,  0},
/*
    {OnCreateProcessW,    "CreateProcessW",   kernel32, 0},
    {OnCreateProcessA,    "CreateProcessA",   kernel32, 0},
    {OnGetConsoleTitleA,  "GetConsoleTitleA", kernel32, 0},
    {OnGetConsoleTitleW,  "GetConsoleTitleW", kernel32, 0},
    {OnSetConsoleTitleA,  "SetConsoleTitleA", kernel32, 0},
    {OnSetConsoleTitleW,  "SetConsoleTitleW", kernel32, 0},
    {OnSetConsoleCtrlHandler, "SetConsoleCtrlHandler", kernel32, 0},
    {OnGetConsoleMode,    "GetConsoleMode",   kernel32, 0},
    {OnSetConsoleMode,    "SetConsoleMode",   kernel32, 0},
    {OnReadConsoleW,      "ReadConsoleW",     kernel32, 0},
    {OnReadConsoleA,      "ReadConsoleA",     kernel32, 0},
    {OnReadFile,          "ReadFile",         kernel32, 0},
    {OnWriteFile,         "WriteFile",        kernel32, 0},
    {OnWriteConsoleA,     "WriteConsoleA",    kernel32, 0},
    {OnWriteConsoleW,     "WriteConsoleW",    kernel32, 0},
    {OnReadConsoleInputW, "ReadConsoleInputW",kernel32, 0},
    {OnReadConsoleInputA, "ReadConsoleInputA",kernel32, 0},
    {OnPeekConsoleInputW, "PeekConsoleInputW",kernel32, 0},
    {OnPeekConsoleInputA, "PeekConsoleInputA",kernel32, 0},
    {OnGetNumberOfConsoleInputEvents, "GetNumberOfConsoleInputEvents",kernel32, 0},
    {OnWaitForSingleObjectEx, "WaitForSingleObjectEx",   kernel32, 0},
    {OnWaitForSingleObject, "WaitForSingleObject",   kernel32, 0},
    {OnWaitForMultipleObjectsEx, "WaitForMultipleObjectsEx",   kernel32, 0},
    {OnWaitForMultipleObjects, "WaitForMultipleObjects",   kernel32, 0},
    {OnSetConsoleCP,      "SetConsoleCP",     kernel32, 0},
    {OnGetConsoleCP,      "GetConsoleCP",     kernel32, 0},
    {OnGetConsoleOutputCP,"GetConsoleOutputCP",kernel32, 0},
    {OnSetConsoleOutputCP,"SetConsoleOutputCP",kernel32, 0},
    {OnSetConsoleActiveScreenBuffer,"SetConsoleActiveScreenBuffer",kernel32, 0},
    {OnCreateFileA,       "CreateFileA",      kernel32, 0},
    {OnCreateFileW,       "CreateFileW",      kernel32, 0},
    {OnFreeConsole,       "FreeConsole",      kernel32, 0},
    {OnAllocConsole,      "AllocConsole",     kernel32, 0},
    {OnSetConsoleCursorInfo, "SetConsoleCursorInfo",    kernel32, 0},
    {OnGetConsoleCursorInfo, "GetConsoleCursorInfo",    kernel32, 0},
    {OnFindWindowA,       "FindWindowA",      user32, 0},
    {OnFindWindowW,       "FindWindowW",      user32, 0},
    {OnFindWindowExA,     "FindWindowExA",    user32, 0},
    {OnFindWindowExW,     "FindWindowExW",    user32, 0},
    {OnGetWindowTextA,    "GetWindowTextA",   user32, 0},
    {OnGetWindowTextW,    "GetWindowTextW",   user32, 0},
*/
    {0, 0, 0}
};


// хуки на LoadLibraryXXX требовались conman'у в тех случаях,
// когда подменяемые библиотеки (user32.dll) НЕ были загружены
// до загрузки самой infis.dll
//static HookItem LoadLibraryHooks[] = {
//    {0, 0, 0}
//};


static TCHAR* ExcludedModules[] = {
    _T("conemu.dll"), // по идее не нужно - должно по ghPluginModule пропускаться. Но на всякий случай!
    _T("kernel32.dll"),
    // а user32.dll не нужно?
    0
};



//static BOOL WINAPI OnReadConsoleInputW( HANDLE input, INPUT_RECORD* buffer, DWORD size, DWORD* read )
//{
//    if( Detached )
//        return ReadConsoleInputW( input, buffer, size, read );
//
//    if( !buffer || !read || !size )
//        return FALSE;
//    if( !IsHandleConsole( input, false ) )
//        return FALSE;
//
//    HANDLE ar[] = { input, isActive };
//    WaitForMultipleObjects( 2, ar, TRUE, INFINITE );
//    return ReadConsoleInputW( input, buffer, size, read );
//}
//
//static BOOL WINAPI OnReadConsoleInputA( HANDLE input, INPUT_RECORD* buffer, DWORD size, DWORD* read )
//{
//    if( Detached )
//        return ReadConsoleInputA( input, buffer, size, read );
//
//    if( !buffer || !read || !size )
//        return FALSE;
//    if( !IsHandleConsole( input, false ) )
//        return FALSE;
//
//    HANDLE ar[] = { input, isActive };
//    WaitForMultipleObjects( 2, ar, TRUE, INFINITE );
//    return ReadConsoleInputA( input, buffer, size, read );
//}
//
//
//static HWND WINAPI OnFindWindowA( char* class_name, char* title )
//{
//    typedef HWND WINAPI FindWindowA_t( char* class_name, char* title );
//    static bool Init = false;
//    static FindWindowA_t* FindWindowA_f = 0;
//    if( !Init )
//    {
//        FindWindowA_f = (FindWindowA_t*)GetOriginalAddress( GetModuleHandle( user32 ),
//                                                            "FindWindowA" );
//        Init = true;
//    }
//
//    assert( FindWindowA_f );
//
//    if( (!title || Detached) && FindWindowA_f )
//        return FindWindowA_f( class_name, title );
//
//    char buffer[1000];
//    OnGetConsoleTitleA( buffer, 1000 );
//
//    if( !lstrcmpiA( buffer, title ) )
//        return MainWindow;
//    else
//        return FindWindowA_f( class_name, title );
//}











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
static bool SetHook( const HookItem* item, HMODULE Module = 0 )
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
                        if( item[j].OldAddress && (void*)thunk->u1.Function == item[j].OldAddress )
                        {
                            DWORD old_protect;
                            VirtualProtect( &thunk->u1.Function, sizeof( thunk->u1.Function ),
                                            PAGE_READWRITE, &old_protect );
                            thunk->u1.Function = (DWORD_PTR)item[j].NewAddress;
                            VirtualProtect( &thunk->u1.Function, sizeof( DWORD ), old_protect, &old_protect );
                            //DebugString( ToTchar( item[j].Name ) );
                            res = true;
                            break;
                        }
                }
            }
        }
    }

    return res;
}

// Подменить Импортируемые функции во всех модулях процесса, загруженных ДО conemu.dll
static bool SetHookEx( const HookItem* item, HMODULE inst )
{
    if( !item )
        return false;
    //MEMORY_BASIC_INFORMATION minfo;
    //char* pb = (char*)GetModuleHandle( 0 );

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
    if(snapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 module = {sizeof(module)};

        for(BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
        {
            if(module.hModule != inst && !IsModuleExcluded(module.hModule))
            {
                DebugString( module.szModule );
                SetHook( item, module.hModule );
            }
        }

        CloseHandle(snapshot);
    }

    return true;
}



// Используется в том случае, если требуется выполнить оригинальную функцию, без нашей обертки
// пример в OnFindWindowA
static void* GetOriginalAddress( HINSTANCE module, const char* name )
{
    if( !name )
        return 0;

    for( int i = 0; Hooks[i].Name; i++ )
        if( !lstrcmpiA( name, Hooks[i].Name ) )
            return Hooks[i].OldAddress;
    return GetProcAddress( module, name );
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

	// Заполнить поле HookItem.OldAddress (реальные процедуры из внешних библиотек)
	InitHooks( Hooks );
	
	// Подменить Импортируемые функции во всех модулях процесса, загруженных ДО conemu.dll
	SetHookEx( Hooks, InfisInstance );

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
