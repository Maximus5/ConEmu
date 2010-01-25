
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
#include "..\common\common.hpp"
#include <Tlhelp32.h>
#include "SetHook.h"

#define DebugString(x) // OutputDebugString(x)

static HMODULE hOurModule = NULL; // Хэндл нашей dll'ки (здесь хуки не ставятся)
static DWORD   nMainThreadId = 0;


static TCHAR kernel32[] = _T("kernel32.dll");
static TCHAR user32[]   = _T("user32.dll");
static TCHAR shell32[]  = _T("shell32.dll");

static HMODULE WINAPI OnLoadLibraryW( const WCHAR* lpFileName );
static HMODULE WINAPI OnLoadLibraryA( const char* lpFileName );
static HMODULE WINAPI OnLoadLibraryExW( const WCHAR* lpFileName, HANDLE hFile, DWORD dwFlags );
static HMODULE WINAPI OnLoadLibraryExA( const char* lpFileName, HANDLE hFile, DWORD dwFlags );
static FARPROC WINAPI OnGetProcAddress( HMODULE hModule, LPCSTR lpProcName );


const HookItem LibraryHooks[6] = {
    {OnLoadLibraryA,        "LoadLibraryA",			kernel32},
	{OnLoadLibraryW,        "LoadLibraryW",			kernel32},
    {OnLoadLibraryExA,      "LoadLibraryExA",		kernel32},
	{OnLoadLibraryExW,      "LoadLibraryExW",		kernel32},
	{OnGetProcAddress,      "GetProcAddress",       kernel32},
	{0,0,0}
};


#define MAX_HOOKED_PROCS 50
static HookItem Hooks[MAX_HOOKED_PROCS] = {
	{0,0,0}
};

#define MAX_EXCLUDED_MODULES 20
static wchar_t* ExcludedModules[MAX_EXCLUDED_MODULES] = {
    L"kernel32.dll",
    // а user32.dll не нужно?
    0
};

#define ORIGINAL(n) \
	_ASSERTE(##n!=On##n); \
	BOOL bMainThread = (GetCurrentThreadId() == nMainThreadId); \
	void* f##n = GetOriginalAddress(On##n, ##n, bMainThread); \
	_ASSERTE(f##n!=NULL && f##n!=On##n);



// Используется в том случае, если требуется выполнить оригинальную функцию, без нашей обертки
// пример в OnPeekConsoleInputW
void* GetOriginalAddress( void* OurFunction, void* DefaultFunction, BOOL abAllowModified )
{
	for( int i = 0; Hooks[i].Name; i++ ) {
		if ( Hooks[i].NewAddress == OurFunction) {
			return (abAllowModified && Hooks[i].ExeOldAddress) ? Hooks[i].ExeOldAddress : Hooks[i].OldAddress;
		}
	}
    _ASSERT(FALSE); // сюда мы попадать не должны
    return DefaultFunction;
}

// Заполнить поле HookItem.OldAddress (реальные процедуры из внешних библиотек)
bool InitHooks( HookItem* apHooks )
{
	int i, j;
	bool skip;
	
    if( apHooks )
    {
    	for (i = 0; apHooks[i].Name; i++) {
    		k = -1; skip = false;

    		for (j = 0; Hooks[j].Name; j++) {
    			if (Hooks[j].NewAddress == apHooks[i].NewAddress) {
    				skip = true; break;
    			}
    		}
    		if (skip) continue;

    		j = 0; // using while, because of j
    		while (Hooks[j].Name) {
    			if (!lstrcmpiA(Hooks[j].Name, apHooks[i].Name)
    				&& !lstrcmpiW(Hooks[j].DllName, apHooks[i].DllName))
				{
					Hooks[j].NewAddress = apHooks[i].NewAddress;
					skip = true; break;
				}
				j++;
    		}
    		if (skip) continue;
    		
    		if (j > 0) {
    			if ((j+1) >= MAX_HOOKED_PROCS) {
    				// Превышено допустимое количество
    				_ASSERTE((j+1) < MAX_HOOKED_PROCS);
    				continue;
    			}
    			Hooks[j].Name = apHooks[i].Name;
    			Hooks[j].DllName = apHooks[i].DllName;
				Hooks[j].NewAddress = apHooks[i].NewAddress;
				Hooks[j+1].Name = NULL; // на всякий
    		}
    	}
    }

    for (i = 0; Hooks[i].Name; i++)
    {
        if( !Hooks[i].OldAddress )
        {
        	// Сейчас - не загружаем
            HMODULE mod = GetModuleHandle( Hooks[i].DllName );
            if( mod ) {
                Hooks[i].OldAddress = GetProcAddress( mod, Hooks[i].Name );
                Hooks[i].hDll = mod;
            }
        }
    }
    
    return true;
}

static bool IsModuleExcluded( HMODULE module, TCHAR* aszExcludedModules )
{
	if (module == hOurModule)
		return true;

    for( int i = 0; ExcludedModules[i]; i++ )
        if( module == GetModuleHandle( ExcludedModules[i] ) )
            return true;
	
    return false;
}



#ifdef _DEBUG
static PIMAGE_SECTION_HEADER GetEnclosingSectionHeader(DWORD_PTR rva, PIMAGE_NT_HEADERS pNTHeader)
{
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pNTHeader);
	unsigned i;

	for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++, section++ )
	{
		// This 3 line idiocy is because Watcom's linker actually sets the
		// Misc.VirtualSize field to 0.  (!!! - Retards....!!!)
		DWORD size = section->Misc.VirtualSize;
		if ( 0 == size )
			size = section->SizeOfRawData;

		// Is the RVA within this section?
		if ( (rva >= section->VirtualAddress) && 
			(rva < (section->VirtualAddress + size)))
			return section;
	}

	return 0;
}
#endif

static LPVOID GetPtrFromRVA( DWORD_PTR rva, PIMAGE_NT_HEADERS pNTHeader, PBYTE imageBase )
{
#ifdef _DEBUG
	PIMAGE_SECTION_HEADER pSectionHdr;
	INT delta;

	pSectionHdr = GetEnclosingSectionHeader( rva, pNTHeader );
	if ( !pSectionHdr )
		; //return 0;
	else
		delta = (INT)(pSectionHdr->VirtualAddress-pSectionHdr->PointerToRawData);
#endif

	return (PVOID) ( imageBase + rva /*- delta*/ );
}


// Подменить Импортируемые функции в модуле
static bool SetHook( HMODULE Module, BOOL abExecutable )
{
    IMAGE_IMPORT_DESCRIPTOR* Import = 0;
    DWORD Size = 0;
    HMODULE hExecutable = GetModuleHandle( 0 );
    if( !Module )
        Module = hExecutable;
    BOOL bExecutable = (Module == hExecutable);
    IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)Module;
	IMAGE_NT_HEADERS* nt_header = NULL;
    if( dos_header->e_magic == 'ZM' )
    {
        nt_header = (IMAGE_NT_HEADERS*)((char*)Module + dos_header->e_lfanew);
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

#ifdef _DEBUG
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt_header);
#endif

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

		DWORD_PTR rvaINT = Import[i].OriginalFirstThunk;
		DWORD_PTR rvaIAT = Import[i].FirstThunk;
		if ( rvaINT == 0 )   // No Characteristics field?
		{
			// Yes! Gotta have a non-zero FirstThunk field then.
			rvaINT = rvaIAT;
			if ( rvaINT == 0 )   // No FirstThunk field?  Ooops!!!
				break;
		}

		PIMAGE_IMPORT_BY_NAME pOrdinalName = NULL, pOrdinalNameO = NULL;
		IMAGE_IMPORT_BY_NAME** byname = (IMAGE_IMPORT_BY_NAME**)((char*)Module + rvaINT);
        //IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)((char*)Module + rvaIAT);
		IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)GetPtrFromRVA( rvaIAT, nt_header, (PBYTE)Module );
		IMAGE_THUNK_DATA* thunkO = (IMAGE_THUNK_DATA*)GetPtrFromRVA( rvaINT, nt_header, (PBYTE)Module );
		if (!thunk ||  !thunkO) {
			_ASSERTE(thunk && thunkO);
			continue;
		}

		int f = 0;
        for(f = 0 ; thunk->u1.Function; thunk++, thunkO++, f++)
        {
			const char* pszFuncName = NULL;
			ULONGLONG ordinalO = -1;
			if (bExecutable) {
				if ( IMAGE_SNAP_BY_ORDINAL(thunkO->u1.Ordinal) ) {
					ordinalO = IMAGE_ORDINAL(thunkO->u1.Ordinal);
					pOrdinalNameO = NULL;
				} else {
					pOrdinalNameO = (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA(thunkO->u1.AddressOfData, nt_header, (PBYTE)Module);
					BOOL lbValidPtr = !IsBadReadPtr(pOrdinalNameO, sizeof(IMAGE_IMPORT_BY_NAME));
					_ASSERTE(lbValidPtr);
					if (lbValidPtr) {
						lbValidPtr = !IsBadStringPtrA((LPCSTR)pOrdinalNameO->Name, 10);
						_ASSERTE(lbValidPtr);
						if (lbValidPtr)
							pszFuncName = (LPCSTR)pOrdinalNameO->Name;
					}
				}
			}

			int j = 0;
            for( j = 0; Hooks[j].Name; j++ )
			{
				// OldAddress уже может отличаться от оригинального экспорта библиотеки
				// Это происходит например с PeekConsoleIntputW при наличии плагина Anamorphosis
                if( !Hooks[j].OldAddress || (void*)thunk->u1.Function != Hooks[j].OldAddress )
                {
					if (!pszFuncName || !bExecutable) {
						continue;
					} else {
						if (strcmp(pszFuncName, Hooks[j].Name))
							continue;
					}
					Hooks[j].ExeOldAddress = (void*)thunk->u1.Function;
				}

				DWORD old_protect = 0;
				VirtualProtect( &thunk->u1.Function, sizeof( thunk->u1.Function ),
					PAGE_READWRITE, &old_protect );
				thunk->u1.Function = (DWORD_PTR)Hooks[j].NewAddress;
				VirtualProtect( &thunk->u1.Function, sizeof( DWORD ), old_protect, &old_protect );
				#ifdef _DEBUG
				if (bExecutable)
					Hooks[j].ReplacedInExe = TRUE;
				#endif
				//DebugString( ToTchar( Hooks[j].Name ) );
				res = true;
				break;
			}
        }
    }

    return res;
}

// Подменить Импортируемые функции во всех модулях процесса, загруженных ДО conemu.dll
// *aszExcludedModules - должны указывать на константные значения (program lifetime)
bool SetAllHooks( HMODULE ahOurDll, const wchar_t* aszExcludedModules /*= NULL*/ )
{
	// т.к. SetAllHooks может быть вызван из разных dll - запоминаем однократно
	if (!hOurModule) hOurModule = ahOurDll;
	
	InitHooks ( NULL );
	
	// Запомнить aszExcludedModules
	if (aszExcludedModules) {
		int j;
		bool skip;
		for ( int i = 0; aszExcludedModules[i]; i++ ) {
			j = 0; skip = false;
			while (ExcludedModules[j]) {
				if (lstrcmpi(ExcludedModules[j], aszExcludedModules[i]) == 0) {
					skip = true; break;
				}
				j++;
			}
    		if (skip) continue;
    		
    		if (j > 0) {
    			if ((j+1) >= MAX_EXCLUDED_MODULES) {
    				// Превышено допустимое количество
    				_ASSERTE((j+1) < MAX_EXCLUDED_MODULES);
    				continue;
    			}
    			ExcludedModules[j] = aszExcludedModules[i];
				ExcludedModules[j+1] = NULL; // на всякий
    		}
		}
	}
	
	// Для исполняемого файла могут быть заданы дополнительные inject-ы (сравнение в FAR)
    HMODULE hExecutable = GetModuleHandle( 0 );

    // Найти ID основной нити
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetCurrentProcessId());
    if(snapshot != INVALID_HANDLE_VALUE)
    {
    	THREADENTRY32 module = {sizeof(THREADENTRY32)};
    	if (Thread32First(snapshot, &module))
    		nMainThreadId = module.th32ThreadID;
    	CloseHandle(snapshot);
	}

	// Начались замены во всех загруженных (static linked) модулях
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
    if(snapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 module = {sizeof(MODULEENTRY32)};

        for(BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
        {
            if(module.hModule && !IsModuleExcluded(module.hModule))
            {
                DebugString( module.szModule );
                SetHook( module.hModule, (module.hModule == hExecutable) );
            }
        }

        CloseHandle(snapshot);
    }

    return true;
}




// Подменить Импортируемые функции в модуле
static bool UnsetHook( HMODULE Module, BOOL abExecutable )
{
    IMAGE_IMPORT_DESCRIPTOR* Import = 0;
    DWORD Size = 0;

    _ASSERTE(Module!=NULL); 
    
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
                    for( int j = 0; Hooks[j].Name; j++ )
                    {
                    	// BugBug: в принципе, эту функцию мог захукать и другой модуль (уже после нас),
						// но лучше вернуть оригинальную, чем потом свалиться
                        if( Hooks[j].OldAddress && (void*)thunk->u1.Function == Hooks[j].NewAddress )
                        {
                            DWORD old_protect;
                            VirtualProtect( &thunk->u1.Function, sizeof( thunk->u1.Function ),
                                            PAGE_READWRITE, &old_protect );
							// BugBug: ExeOldAddress может отличаться от оригинального, если функция была перехвачена ДО нас
							//if (abExecutable && Hooks[j].ExeOldAddress)
							//	thunk->u1.Function = (DWORD_PTR)Hooks[j].ExeOldAddress;
							//else
								thunk->u1.Function = (DWORD_PTR)Hooks[j].OldAddress;
                            VirtualProtect( &thunk->u1.Function, sizeof( DWORD ), old_protect, &old_protect );
                            //DebugString( ToTchar( Hooks[j].Name ) );
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

void UnsetAllHooks( )
{
    HMODULE hExecutable = GetModuleHandle( 0 );

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
    if(snapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 module = {sizeof(module)};

        for(BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
        {
            if(module.hModule && !IsModuleExcluded(module.hModule))
            {
                DebugString( module.szModule );
                UnsetHook( module.hModule, (module.hModule == hExecutable) );
            }
        }

        CloseHandle(snapshot);
    }
}


// Заменить в модуле Module ЭКСпортируемые функции на подменяемые плагином нихрена
// НЕ получится, т.к. в Win32 библиотека shell32 может быть загружена ПОСЛЕ conemu.dll
//   что вызовет некорректные смещения функций,
// а в Win64 смещения вообще должны быть 64битными, а структура модуля хранит только 32битные смещения

static void PrepareNewModule ( HMODULE module )
{
    if( !module || IsModuleExcluded(module) )
        return;

    // Некоторые перехватываемые библиотеки могли быть
    // не загружены во время первичной инициализации
    InitHooks( NULL );
    
    // Подмена импортируемых функций в module
    SetHook( module, FALSE );
}


typedef HMODULE (WINAPI* OnLoadLibraryA_t)( const char* lpFileName );
static HMODULE WINAPI OnLoadLibraryA( const char* lpFileName )
{
	ORIGINAL(LoadLibraryA);

    HMODULE module = (OnLoadLibraryA_t)fLoadLibraryA( lpFileName );
    
    PrepareNewModule ( module );
    
	return module;
}

typedef HMODULE (WINAPI* OnLoadLibraryW_t)( const wchar_t* lpFileName );
static HMODULE WINAPI OnLoadLibraryW( const wchar_t* lpFileName )
{
	ORIGINAL(LoadLibraryW);

	HMODULE module = (OnLoadLibraryW_t)fLoadLibraryW( lpFileName );
    
    PrepareNewModule ( module );
    
	return module;
}

typedef HMODULE (WINAPI* OnLoadLibraryExA_t)( const char* lpFileName, HANDLE hFile, DWORD dwFlags );
static HMODULE WINAPI OnLoadLibraryExA( const char* lpFileName, HANDLE hFile, DWORD dwFlags )
{
	ORIGINAL(LoadLibraryExA);

	HMODULE module = (OnLoadLibraryExA_t)fLoadLibraryExA( lpFileName, hFile, dwFlags );
    
    PrepareNewModule ( module );
    
	return module;
}

typedef HMODULE (WINAPI* OnLoadLibraryExW_t)( const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags );
static HMODULE WINAPI OnLoadLibraryExW( const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags )
{
    ORIGINAL(LoadLibraryExW);

    HMODULE module = (OnLoadLibraryExW_t)fLoadLibraryExW( lpFileName, hFile, dwFlags );
    
    PrepareNewModule ( module );
    
	return module;
}

typedef FARPROC (WINAPI* OnGetProcAddress_t)(HMODULE hModule, LPCSTR lpProcName);
static FARPROC WINAPI OnGetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	ORIGINAL(GetProcAddress);
	FARPROC lpfn = NULL;
	
    for (int i = 0; Hooks[i].Name; i++)
    {
    	// The spelling and case of a function name pointed to by lpProcName must be identical
    	// to that in the EXPORTS statement of the source DLL's module-definition (.Def) file
        if (Hooks[i].hDll == hModule
        	&& strcmp(Hooks[i].Name, lpProcName) == 0)
        {
        	lpfn = (FARPROC)Hooks[i].NewAddress;
        	break;
        }
    }
    
    if (!lpfn)
    	lpfn = (OnGetProcAddress_t)fGetProcAddress(hModule, lpProcName);
    
    return lpfn;
}

TODO("по хорошему бы еще и FreeDll хукать нужно, чтобы не позвать случайно неактуальную функцию...");
