
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

#define DROP_SETCP_ON_WIN2K3R2

// Иначе не опередяется GetConsoleAliases (хотя он должен быть доступен в Win2k)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#define DEFINE_HOOK_MACROS

#include <windows.h>
#include <Tlhelp32.h>
#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"
#include "../common/WinObjects.h"
#include "ShellProcessor.h"
#include "SetHook.h"

//#include <WinInet.h>
//#pragma comment(lib, "wininet.lib")

#define DebugString(x) // OutputDebugString(x)

HMODULE ghHookOurModule = NULL; // Хэндл нашей dll'ки (здесь хуки не ставятся)
DWORD   gnHookMainThreadId = 0;

//extern CEFAR_INFO_MAPPING *gpFarInfo, *gpFarInfoMapping; //gpConsoleInfo;
//extern HANDLE ghFarAliveEvent;
//extern HWND ConEmuHwnd; // Содержит хэндл окна отрисовки. Это ДОЧЕРНЕЕ окно.
//extern HWND FarHwnd;
//extern BOOL gbFARuseASCIIsort;
//extern DWORD gdwServerPID;
//extern BOOL gbShellNoZoneCheck;

//const wchar_t kernel32[] = L"kernel32.dll";
//const wchar_t user32[]   = L"user32.dll";
//const wchar_t shell32[]  = L"shell32.dll";
//HMODULE ghKernel32 = NULL, ghUser32 = NULL, ghShell32 = NULL;
extern const wchar_t *kernel32;// = L"kernel32.dll";
extern const wchar_t *user32  ;// = L"user32.dll";
extern const wchar_t *shell32 ;// = L"shell32.dll";
extern HMODULE ghKernel32, ghUser32, ghShell32;

BOOL gbHooksTemporaryDisabled = FALSE;
//BOOL gbInShellExecuteEx = FALSE;

//typedef VOID (WINAPI* OnLibraryLoaded_t)(HMODULE ahModule);
HMODULE ghOnLoadLibModule = NULL;
OnLibraryLoaded_t gfOnLibraryLoaded = NULL;
OnLibraryLoaded_t gfOnLibraryUnLoaded = NULL;

// Forward declarations of the hooks
HMODULE WINAPI OnLoadLibraryW(const WCHAR* lpFileName);
HMODULE WINAPI OnLoadLibraryA(const char* lpFileName);
HMODULE WINAPI OnLoadLibraryExW(const WCHAR* lpFileName, HANDLE hFile, DWORD dwFlags);
HMODULE WINAPI OnLoadLibraryExA(const char* lpFileName, HANDLE hFile, DWORD dwFlags);
BOOL WINAPI OnFreeLibrary(HMODULE hModule);
FARPROC WINAPI OnGetProcAddress(HMODULE hModule, LPCSTR lpProcName);

#define MAX_HOOKED_PROCS 255
HookItem *gpHooks = NULL;

bool InitHooksLibrary()
{
#ifndef HOOKS_SKIP_LIBRARY
	if (!gpHooks)
	{
		_ASSERTE(gpHooks!=NULL);
		return false;
	}
	if (gpHooks[0].NewAddress != NULL)
	{
		_ASSERTE(gpHooks[0].NewAddress==NULL);
		return false;
	}

	HookItem LibHooks[MAX_HOOKED_PROCS] =
	{
		/* ************************ */
		{(void*)OnLoadLibraryA,			"LoadLibraryA",			kernel32},
		{(void*)OnLoadLibraryW,			"LoadLibraryW",			kernel32},
		{(void*)OnLoadLibraryExA,		"LoadLibraryExA",		kernel32},
		{(void*)OnLoadLibraryExW,		"LoadLibraryExW",		kernel32},
		#ifndef HOOKS_SKIP_GETPROCADDRESS
		{(void*)OnGetProcAddress,		"GetProcAddress",		kernel32},
		#endif
		/* ************************ */
		{0}
	};
	
	memmove(gpHooks, LibHooks, sizeof(LibHooks));
#endif
	return true;
}



#define MAX_EXCLUDED_MODULES 40
// Skip/ignore/don't set hooks in modules...
const wchar_t* ExcludedModules[MAX_EXCLUDED_MODULES] =
{
	L"ntdll.dll",
	L"kernelbase.dll",
	L"kernel32.dll",
	L"user32.dll",
//	L"shell32.dll", -- shell нужно обрабатывать обязательно. по крайней мере в WinXP/Win2k3 (
	L"wininet.dll", // какой-то криминал с этой библиотекой?
//#ifndef _DEBUG
	L"mssign32.dll",
	L"crypt32.dll",
//#endif
	// А также исключаются все "API-MS-Win-..." в функции IsModuleExcluded
	0
};

BOOL gbLogLibraries = FALSE;
DWORD gnLastLogSetChange = 0;


// Используется в том случае, если требуется выполнить оригинальную функцию, без нашей обертки
// пример в OnPeekConsoleInputW
void* __cdecl GetOriginalAddress(void* OurFunction, void* DefaultFunction, BOOL abAllowModified, HookItem** ph)
{
	if (gpHooks)
	{
		for(int i = 0; gpHooks[i].NewAddress; i++)
		{
			if (gpHooks[i].NewAddress == OurFunction)
			{
				*ph = &(gpHooks[i]);
				return (abAllowModified && gpHooks[i].ExeOldAddress) ? gpHooks[i].ExeOldAddress : gpHooks[i].OldAddress;
			}
		}
	}

	_ASSERT(FALSE); // сюда мы попадать не должны
	return DefaultFunction;
}

CInFuncCall::CInFuncCall()
{
	mpn_Counter = NULL;
}
BOOL CInFuncCall::Inc(int* pnCounter)
{
	BOOL lbFirstCall = FALSE;
	mpn_Counter = pnCounter;

	if (mpn_Counter)
	{
		lbFirstCall = (*mpn_Counter) == 0;
		(*mpn_Counter)++;
	}

	return lbFirstCall;
}
CInFuncCall::~CInFuncCall()
{
	if (mpn_Counter && (*mpn_Counter)>0)(*mpn_Counter)--;
}


extern HANDLE ghHookMutex;

// Заполнить поле HookItem.OldAddress (реальные процедуры из внешних библиотек)
// apHooks->Name && apHooks->DllName MUST be for a lifetime
bool __stdcall InitHooks(HookItem* apHooks)
{
	int i, j;
	bool skip;

	if (!ghHookMutex)
	{
		wchar_t* szMutexName = (wchar_t*)malloc(MAX_PATH*2);
		msprintf(szMutexName, MAX_PATH, CEHOOKLOCKMUTEX, GetCurrentProcessId());
		ghHookMutex = CreateMutexW(NULL, FALSE, szMutexName);
		_ASSERTE(ghHookMutex != NULL);
		free(szMutexName);
	}

	if (gpHooks == NULL)
	{
		gpHooks = (HookItem*)calloc(sizeof(HookItem),MAX_HOOKED_PROCS);
		if (!gpHooks)
			return false;
			
		if (!InitHooksLibrary())
			return false;
	}

	if (apHooks && gpHooks)
	{
		for(i = 0; apHooks[i].NewAddress; i++)
		{
			if (apHooks[i].Name==NULL || apHooks[i].DllName==NULL)
			{
				_ASSERTE(apHooks[i].Name!=NULL && apHooks[i].DllName!=NULL);
				break;
			}

			skip = false;

			for(j = 0; gpHooks[j].NewAddress; j++)
			{
				if (gpHooks[j].NewAddress == apHooks[i].NewAddress)
				{
					skip = true; break;
				}
			}

			if (skip) continue;

			j = 0; // using while, because of j

			while (gpHooks[j].NewAddress)
			{
				if (!lstrcmpiA(gpHooks[j].Name, apHooks[i].Name)
				        && !lstrcmpiW(gpHooks[j].DllName, apHooks[i].DllName))
				{
					gpHooks[j].NewAddress = apHooks[i].NewAddress;
					skip = true; break;
				}

				j++;
			}

			if (skip) continue;

			if (j >= 0)
			{
				if ((j+1) >= MAX_HOOKED_PROCS)
				{
					// Превышено допустимое количество
					_ASSERTE((j+1) < MAX_HOOKED_PROCS);
					continue; // может какие другие хуки удастся обновить, а не добавить
				}

				gpHooks[j].Name = apHooks[i].Name;
				gpHooks[j].DllName = apHooks[i].DllName;
				gpHooks[j].NewAddress = apHooks[i].NewAddress;
				gpHooks[j+1].Name = NULL; // на всякий
				gpHooks[j+1].NewAddress = NULL; // на всякий
			}
		}
	}

	// Для добавленных в gpHooks функций определить "оригинальный" адрес экспорта
	for(i = 0; gpHooks[i].NewAddress; i++)
	{
		if (!gpHooks[i].OldAddress)
		{
			// Сейчас - не загружаем
			HMODULE mod = GetModuleHandle(gpHooks[i].DllName);

			if (mod == NULL)
			{
				_ASSERTE(mod != NULL || (gpHooks[i].DllName == shell32 || gpHooks[i].DllName == user32));
			}
			else
			{
				gpHooks[i].OldAddress = (void*)GetProcAddress(mod, gpHooks[i].Name);
				_ASSERTE(gpHooks[i].OldAddress != NULL);
				gpHooks[i].hDll = mod;
			}
		}
	}

	return true;
}

void __stdcall SetLoadLibraryCallback(HMODULE ahCallbackModule, OnLibraryLoaded_t afOnLibraryLoaded, OnLibraryLoaded_t afOnLibraryUnLoaded)
{
	ghOnLoadLibModule = ahCallbackModule;
	gfOnLibraryLoaded = afOnLibraryLoaded;
	gfOnLibraryUnLoaded = afOnLibraryUnLoaded;
}

bool __stdcall SetHookCallbacks(const char* ProcName, const wchar_t* DllName, HMODULE hCallbackModule,
                                HookItemPreCallback_t PreCallBack, HookItemPostCallback_t PostCallBack,
                                HookItemExceptCallback_t ExceptCallBack)
{
	_ASSERTE(ProcName!=NULL && DllName!=NULL);
	_ASSERTE(ProcName[0]!=0 && DllName[0]!=0);
	bool bFound = false;

	if (gpHooks)
	{
		for(int i = 0; i<MAX_HOOKED_PROCS && gpHooks[i].NewAddress; i++)
		{
			if (!lstrcmpA(gpHooks[i].Name, ProcName) && !lstrcmpW(gpHooks[i].DllName,DllName))
			{
				gpHooks[i].hCallbackModule = hCallbackModule;
				gpHooks[i].PreCallBack = PreCallBack;
				gpHooks[i].PostCallBack = PostCallBack;
				gpHooks[i].ExceptCallBack = ExceptCallBack;
				bFound = true;
				//break; // перехватов может быть более одного (деление хуков на exe/dll)
			}
		}
	}

	return bFound;
}

bool IsModuleExcluded(HMODULE module, LPCSTR asModuleA, LPCWSTR asModuleW)
{
	if (module == ghHookOurModule)
		return true;

	// Возможно, имеет смысл игнорировать системные библиотеки вида
    // API-MS-Win-Core-Util-L1-1-0.dll
	if (asModuleA)
	{
		char szTest[12]; lstrcpynA(szTest, asModuleA, 12);
		if (lstrcmpiA(szTest, "API-MS-Win-") == 0)
			return true;
	}
	else if (asModuleW)
	{
		wchar_t szTest[12]; lstrcpynW(szTest, asModuleW, 12);
		if (lstrcmpiW(szTest, L"API-MS-Win-") == 0)
			return true;
	}

	for(int i = 0; ExcludedModules[i]; i++)
		if (module == GetModuleHandle(ExcludedModules[i]))
			return true;

	return false;
}


#define GetPtrFromRVA(rva,pNTHeader,imageBase) (PVOID)((imageBase)+(rva))

extern BOOL gbInCommonShutdown;

// Подменить Импортируемые функции в модуле (Module)
//	если (abForceHooks == FALSE) то хуки не ставятся, если
//  будет обнаружен импорт, не совпадающий с оригиналом
//  Это для того, чтобы не выполнять множественные хуки при множественных LoadLibrary
bool SetHook(HMODULE Module, BOOL abForceHooks)
{
	IMAGE_IMPORT_DESCRIPTOR* Import = 0;
	DWORD Size = 0;
	HMODULE hExecutable = GetModuleHandle(0);

	if (!gpHooks)
		return false;

	if (!Module)
		Module = hExecutable;
		
	//#ifdef NDEBUG
	//	PRAGMA_ERROR("Один модуль обрабатывать ОДИН раз");
	//#endif

	BOOL bExecutable = (Module == hExecutable);
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)Module;
	IMAGE_NT_HEADERS* nt_header = NULL;

	if (dos_header->e_magic == IMAGE_DOS_SIGNATURE /*'ZM'*/)
	{
		nt_header = (IMAGE_NT_HEADERS*)((char*)Module + dos_header->e_lfanew);

		if (nt_header->Signature != 0x004550)
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
	if (Module == INVALID_HANDLE_VALUE || !Import)
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
	DWORD nHookMutexWait = WaitForSingleObject(ghHookMutex, 10000);

	while(nHookMutexWait != WAIT_OBJECT_0)
	{
#ifdef _DEBUG

		if (!IsDebuggerPresent())
		{
			_ASSERTE(nHookMutexWait == WAIT_OBJECT_0);
		}

#endif

		if (gbInCommonShutdown)
			return false;

		wchar_t* szTrapMsg = (wchar_t*)calloc(1024,2);
		wchar_t* szName = (wchar_t*)calloc((MAX_PATH+1),2);

		if (!GetModuleFileNameW(Module, szName, MAX_PATH+1)) szName[0] = 0;

		DWORD nTID = GetCurrentThreadId(); DWORD nPID = GetCurrentProcessId();
		msprintf(szTrapMsg, 1024, L"Can't install hooks in module '%s'\nCurrent PID=%u, TID=%i\nCan't lock hook mutex\nPress 'Retry' to repeat locking",
		          szName, nPID, nTID);

		int nBtn = 
			#ifdef CONEMU_MINIMAL
				GuiMessageBox
			#else
				MessageBoxW
			#endif
			(NULL, szTrapMsg, L"ConEmu", MB_RETRYCANCEL|MB_ICONSTOP|MB_SYSTEMMODAL);
		
		free(szTrapMsg);
		free(szName);
		
		if (nBtn != IDRETRY)
			return false;

		nHookMutexWait = WaitForSingleObject(ghHookMutex, 10000);
		continue;
	}

	TODO("!!! Сохранять ORDINAL процедур !!!");
	bool res = false, bHooked = false;
	int i;
	int nCount = Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);

	//_ASSERTE(Size == (nCount * sizeof(IMAGE_IMPORT_DESCRIPTOR))); -- ровно быть не обязано
	for(i = 0; i < nCount; i++)
	{
		if (Import[i].Name == 0)
			break;

		//DebugString( ToTchar( (char*)Module + Import[i].Name ) );
#ifdef _DEBUG
		char* mod_name = (char*)Module + Import[i].Name;
#endif
		DWORD_PTR rvaINT = Import[i].OriginalFirstThunk;
		DWORD_PTR rvaIAT = Import[i].FirstThunk;

		if (rvaINT == 0)      // No Characteristics field?
		{
			// Yes! Gotta have a non-zero FirstThunk field then.
			rvaINT = rvaIAT;

			if (rvaINT == 0)       // No FirstThunk field?  Ooops!!!
			{
				_ASSERTE(rvaINT!=0);
				break;
			}
		}

		//PIMAGE_IMPORT_BY_NAME pOrdinalName = NULL, pOrdinalNameO = NULL;
		PIMAGE_IMPORT_BY_NAME pOrdinalNameO = NULL;
		//IMAGE_IMPORT_BY_NAME** byname = (IMAGE_IMPORT_BY_NAME**)((char*)Module + rvaINT);
		//IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)((char*)Module + rvaIAT);
		IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)GetPtrFromRVA(rvaIAT, nt_header, (PBYTE)Module);
		IMAGE_THUNK_DATA* thunkO = (IMAGE_THUNK_DATA*)GetPtrFromRVA(rvaINT, nt_header, (PBYTE)Module);

		if (!thunk ||  !thunkO)
		{
			_ASSERTE(thunk && thunkO);
			continue;
		}

		int f = 0;

		for(f = 0 ; thunk->u1.Function; thunk++, thunkO++, f++)
		{
			const char* pszFuncName = NULL;
			ULONGLONG ordinalO = -1;

			if (thunk->u1.Function != thunkO->u1.Function)
			{
				// Ordinal у нас пока не используется
				if (IMAGE_SNAP_BY_ORDINAL(thunkO->u1.Ordinal))
				{
					WARNING("Это НЕ ORDINAL, это Hint!!!");
					ordinalO = IMAGE_ORDINAL(thunkO->u1.Ordinal);
					pOrdinalNameO = NULL;
				}

				TODO("Возможно стоит искать имя функции не только для EXE, но и для всех dll?");

				if (bExecutable)
				{
					if (!IMAGE_SNAP_BY_ORDINAL(thunkO->u1.Ordinal))
					{
						pOrdinalNameO = (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA(thunkO->u1.AddressOfData, nt_header, (PBYTE)Module);
						WARNING("Множественные вызовы IsBad???Ptr могут глючить");
						BOOL lbValidPtr = !IsBadReadPtr(pOrdinalNameO, sizeof(IMAGE_IMPORT_BY_NAME));
						_ASSERTE(lbValidPtr);

						if (lbValidPtr)
						{
							lbValidPtr = !IsBadStringPtrA((LPCSTR)pOrdinalNameO->Name, 10);
							_ASSERTE(lbValidPtr);

							if (lbValidPtr)
								pszFuncName = (LPCSTR)pOrdinalNameO->Name;
						}
					}
				}
			}

			int j;

			for(j = 0; gpHooks[j].Name; j++)
			{
#ifdef _DEBUG
				const void* ptrNewAddress = gpHooks[j].NewAddress;
				const void* ptrOldAddress = (void*)thunk->u1.Function;
#endif

				// Если адрес импорта в модуле уже совпадает с адресом одной из наших функций
				if (gpHooks[j].NewAddress == (void*)thunk->u1.Function)
				{
					res = true; // это уже захучено
					break;
				}

				// Если не удалось определить оригинальный адрес процедуры (kernel32/WriteConsoleOutputW, и т.п.)
				if (gpHooks[j].OldAddress == NULL)
				{
					continue;
				}

				// Проверяем, имя функции совпадает с перехватываемыми?
				if ((void*)thunk->u1.Function != gpHooks[j].OldAddress)
				{
					// bExecutable проверяется выше, если нет - то (pszFuncName == NULL)
					if (!pszFuncName /*|| !bExecutable*/)
					{
						continue; // Если имя импорта определить не удалось - пропускаем
					}
					else
					{
						if (lstrcmpA(pszFuncName, gpHooks[j].Name))
							continue;
					}

					if (!abForceHooks)
					{
						continue; // запрещен перехват, если текущий адрес в модуле НЕ совпадает с оригинальным экспортом!
					}
					else
					{
						// OldAddress уже может отличаться от оригинального экспорта библиотеки
						// Это происходит например с PeekConsoleIntputW при наличии плагина Anamorphosis
						gpHooks[j].ExeOldAddress = (void*)thunk->u1.Function;
					}
				}

				//#ifdef _DEBUG
				//// Это НЕ ORDINAL, это Hint!!!
				//if (gpHooks[j].nOrdinal == 0 && ordinalO != (ULONGLONG)-1)
				//	gpHooks[j].nOrdinal = (DWORD)ordinalO;
				//#endif

				bHooked = true;
				DWORD old_protect = 0; DWORD dwErr = 0;

				if (!VirtualProtect(&thunk->u1.Function, sizeof(thunk->u1.Function),
				                   PAGE_READWRITE, &old_protect))
				{
					dwErr = GetLastError();
					_ASSERTE(FALSE);
				}
				else
				{
					if (thunk->u1.Function == (DWORD_PTR)gpHooks[j].NewAddress)
					{
						// оказалось захучено в другой нити? такого быть не должно, блокируется мутексом
						_ASSERTE(thunk->u1.Function != (DWORD_PTR)gpHooks[j].NewAddress);
					}
					else
					{
						thunk->u1.Function = (DWORD_PTR)gpHooks[j].NewAddress;
					}

					VirtualProtect(&thunk->u1.Function, sizeof(DWORD), old_protect, &old_protect);
#ifdef _DEBUG

					if (bExecutable)
						gpHooks[j].ReplacedInExe = TRUE;

#endif
					//DebugString( ToTchar( gpHooks[j].Name ) );
					res = true;
				}

				break;
			}
		}
	}

#ifdef _DEBUG

	if (bHooked)
	{
		wchar_t* szDbg = (wchar_t*)calloc(MAX_PATH*3, 2);
		wchar_t* szModPath = (wchar_t*)calloc(MAX_PATH*2, 2);
		GetModuleFileName(Module, szModPath, MAX_PATH*2);
		_wcscpy_c(szDbg, MAX_PATH*3, L"  ## Hooks was set by conemu: ");
		_wcscat_c(szDbg, MAX_PATH*3, szModPath);
		_wcscat_c(szDbg, MAX_PATH*3, L"\n");
		OutputDebugStringW(szDbg);
		free(szDbg);
		free(szModPath);
	}

#endif
	ReleaseMutex(ghHookMutex);

	// Плагин ConEmu может выполнить дополнительные действия
	if (gfOnLibraryLoaded)
	{
		gfOnLibraryLoaded(Module);
	}

	return res;
}

// Подменить Импортируемые функции во всех модулях процесса, загруженных ДО conemu.dll
// *aszExcludedModules - должны указывать на константные значения (program lifetime)
bool __stdcall SetAllHooks(HMODULE ahOurDll, const wchar_t** aszExcludedModules /*= NULL*/, BOOL abForceHooks)
{
	// т.к. SetAllHooks может быть вызван из разных dll - запоминаем однократно
	if (!ghHookOurModule) ghHookOurModule = ahOurDll;

	InitHooks(NULL);
	if (!gpHooks)
		return false;

#ifdef _DEBUG
	wchar_t szHookProc[128];
	for(int i = 0; gpHooks[i].NewAddress; i++)
	{
		msprintf(szHookProc, countof(szHookProc), L"## %S -> 0x%08X (exe: 0x%X)\n", gpHooks[i].Name, (DWORD)gpHooks[i].NewAddress, (DWORD)gpHooks[i].ExeOldAddress);
		OutputDebugStringW(szHookProc);
	}

#endif

	// Запомнить aszExcludedModules
	if (aszExcludedModules)
	{
		int j;
		bool skip;

		for(int i = 0; aszExcludedModules[i]; i++)
		{
			j = 0; skip = false;

			while(ExcludedModules[j])
			{
				if (lstrcmpi(ExcludedModules[j], aszExcludedModules[i]) == 0)
				{
					skip = true; break;
				}

				j++;
			}

			if (skip) continue;

			if (j > 0)
			{
				if ((j+1) >= MAX_EXCLUDED_MODULES)
				{
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
	#ifdef _DEBUG
	HMODULE hExecutable = GetModuleHandle(0);
	#endif
	HANDLE snapshot;

	// Найти ID основной нити
	if (!gnHookMainThreadId)
	{
		DWORD dwPID = GetCurrentProcessId();
		snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwPID);

		if (snapshot != INVALID_HANDLE_VALUE)
		{
			THREADENTRY32 module = {sizeof(THREADENTRY32)};

			if (Thread32First(snapshot, &module))
			{
				while(!gnHookMainThreadId)
				{
					if (module.th32OwnerProcessID == dwPID)
					{
						gnHookMainThreadId = module.th32ThreadID;
						break;
					}

					if (!Thread32Next(snapshot, &module))
						break;
				}
			}

			CloseHandle(snapshot);
		}
	}

	_ASSERTE(gnHookMainThreadId!=0);
	// Начались замены во всех загруженных (linked) модулях
	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);

	if (snapshot != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 module = {sizeof(MODULEENTRY32)};

		for(BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
		{
			if (module.hModule && !IsModuleExcluded(module.hModule, NULL, module.szModule))
			{
				DebugString(module.szModule);
				SetHook(module.hModule/*, (module.hModule == hExecutable)*/, abForceHooks);
			}
		}

		CloseHandle(snapshot);
	}

	return true;
}




// Подменить Импортируемые функции в модуле
bool UnsetHook(HMODULE Module)
{
	if (!gpHooks)
		return false;

	IMAGE_IMPORT_DESCRIPTOR* Import = 0;
	DWORD Size = 0;
	_ASSERTE(Module!=NULL);
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)Module;

	if (dos_header->e_magic == IMAGE_DOS_SIGNATURE /*'ZM'*/)
	{
		IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((char*)Module + dos_header->e_lfanew);

		if (nt_header->Signature != 0x004550)
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
	if (Module == INVALID_HANDLE_VALUE || !Import)
		return false;

	bool res = false, bUnhooked = false;
	int i;
	int nCount = Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);

	//_ASSERTE(Size == (nCount * sizeof(IMAGE_IMPORT_DESCRIPTOR))); -- ровно быть не обязано
	for(i = 0; i < nCount; i++)
	{
		if (Import[i].Name == 0)
			break;

#ifdef _DEBUG
		char* mod_name = (char*)Module + Import[i].Name;
#endif
		DWORD_PTR rvaINT = Import[i].OriginalFirstThunk;
		DWORD_PTR rvaIAT = Import[i].FirstThunk;

		if (rvaINT == 0)      // No Characteristics field?
		{
			// Yes! Gotta have a non-zero FirstThunk field then.
			rvaINT = rvaIAT;

			if (rvaINT == 0)       // No FirstThunk field?  Ooops!!!
			{
				_ASSERTE(rvaINT!=0);
				break;
			}
		}

		//PIMAGE_IMPORT_BY_NAME pOrdinalName = NULL, pOrdinalNameO = NULL;
		PIMAGE_IMPORT_BY_NAME pOrdinalNameO = NULL;
		//IMAGE_IMPORT_BY_NAME** byname = (IMAGE_IMPORT_BY_NAME**)((char*)Module + rvaINT);
		//IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)((char*)Module + rvaIAT);
		IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)GetPtrFromRVA(rvaIAT, nt_header, (PBYTE)Module);
		IMAGE_THUNK_DATA* thunkO = (IMAGE_THUNK_DATA*)GetPtrFromRVA(rvaINT, nt_header, (PBYTE)Module);

		if (!thunk ||  !thunkO)
		{
			_ASSERTE(thunk && thunkO);
			continue;
		}

		//110220 - something strange. валимся при выходе из некоторых программ (AddFont.exe)
		//         смысл в том, что thunk указывает на НЕ валидную область памяти.
		//         Разбор полетов показал, что программа сама порушила таблицу импортов.
		if (IsBadReadPtr(thunk, sizeof(IMAGE_THUNK_DATA)) || IsBadReadPtr(thunkO, sizeof(IMAGE_THUNK_DATA)))
		{
			_ASSERTE(thunk && FALSE);
			break;
		}

		int f = 0;

		for(f = 0 ; thunk->u1.Function; thunk++, thunkO++, f++)
		{
			const char* pszFuncName = NULL;
			//ULONGLONG ordinalO = -1;

			//if ( IMAGE_SNAP_BY_ORDINAL(thunkO->u1.Ordinal) ) {
			//	ordinalO = IMAGE_ORDINAL(thunkO->u1.Ordinal);
			//	pOrdinalNameO = NULL;
			//}
			if (thunk->u1.Function!=thunkO->u1.Function && !IMAGE_SNAP_BY_ORDINAL(thunkO->u1.Ordinal))
			{
				pOrdinalNameO = (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA(thunkO->u1.AddressOfData, nt_header, (PBYTE)Module);
				BOOL lbValidPtr = !IsBadReadPtr(pOrdinalNameO, sizeof(IMAGE_IMPORT_BY_NAME));
				_ASSERTE(lbValidPtr);

				if (lbValidPtr)
				{
					lbValidPtr = !IsBadStringPtrA((LPCSTR)pOrdinalNameO->Name, 10);
					_ASSERTE(lbValidPtr);

					if (lbValidPtr)
						pszFuncName = (LPCSTR)pOrdinalNameO->Name;
				}
			}

			int j;

			for(j = 0; gpHooks[j].Name; j++)
			{
				if (!gpHooks[j].OldAddress)
					continue; // Эту функцию не обрабатывали (хотя должны были?)

				// Нужно найти функцию (thunk) в gpHooks через NewAddress или имя
				if ((void*)thunk->u1.Function != gpHooks[j].NewAddress)
				{
					if (!pszFuncName)
					{
						continue;
					}
					else
					{
						if (lstrcmpA(pszFuncName, gpHooks[j].Name))
							continue;
					}

					// OldAddress уже может отличаться от оригинального экспорта библиотеки
					// Это если функцию захукали уже после нас
				}

				// Если мы дошли сюда - значит функция найдена (или по адресу или по имени)
				// BugBug: в принципе, эту функцию мог захукать и другой модуль (уже после нас),
				// но лучше вернуть оригинальную, чем потом свалиться
				DWORD old_protect;
				bUnhooked = true;
				VirtualProtect(&thunk->u1.Function, sizeof(thunk->u1.Function),
				               PAGE_READWRITE, &old_protect);
				// BugBug: ExeOldAddress может отличаться от оригинального, если функция была перехвачена ДО нас
				//if (abExecutable && gpHooks[j].ExeOldAddress)
				//	thunk->u1.Function = (DWORD_PTR)gpHooks[j].ExeOldAddress;
				//else
				thunk->u1.Function = (DWORD_PTR)gpHooks[j].OldAddress;
				VirtualProtect(&thunk->u1.Function, sizeof(DWORD), old_protect, &old_protect);
				//DebugString( ToTchar( gpHooks[j].Name ) );
				res = true;
				break; // перейти к следующему thunk-у
			}
		}
	}

#ifdef _DEBUG

	if (bUnhooked)
	{
		wchar_t* szDbg = (wchar_t*)calloc(MAX_PATH*3, 2);
		wchar_t* szModPath = (wchar_t*)calloc(MAX_PATH*2, 2);
		GetModuleFileName(Module, szModPath, MAX_PATH*2);
		lstrcpy(szDbg, L"  ## Hooks was UNset by conemu: ");
		lstrcat(szDbg, szModPath);
		lstrcat(szDbg, L"\n");
		OutputDebugStringW(szDbg);
		free(szModPath);
		free(szDbg);
	}

#endif
	return res;
}

void __stdcall UnsetAllHooks()
{
	#ifdef _DEBUG
	HMODULE hExecutable = GetModuleHandle(0);
	#endif
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);

	if (snapshot != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 module = {sizeof(module)};

		for(BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
		{
			if (module.hModule && !IsModuleExcluded(module.hModule, NULL, module.szModule))
			{
				DebugString(module.szModule);
				UnsetHook(module.hModule/*, (module.hModule == hExecutable)*/);
			}
		}

		CloseHandle(snapshot);
	}

#ifdef _DEBUG
	hExecutable = hExecutable;
#endif
}






/* **************************** *
 *                              *
 *  Далее идут собственно хуки  *
 *                              *
 * **************************** */

void LoadModuleFailed(LPCSTR asModuleA, LPCWSTR asModuleW)
{
	DWORD dwErrCode = GetLastError();

	if (!gnLastLogSetChange)
	{
		CShellProc* sp = new CShellProc();
		if (sp)
		{
			gnLastLogSetChange = GetTickCount();
			gbLogLibraries = sp->LoadGuiMapping();
			delete sp;
		}
		SetLastError(dwErrCode);
	}

	if (!gbLogLibraries)
		return;


	CESERVER_REQ* pIn = NULL, *pOunt = NULL;
	wchar_t szModule[MAX_PATH+1]; szModule[0] = 0;
	wchar_t szErrCode[64]; szErrCode[0] = 0;
	msprintf(szErrCode, countof(szErrCode), L"ErrCode=0x%08X", dwErrCode);
	if (!asModuleA && !asModuleW)
	{
		wcscpy_c(szModule, L"<NULL>");
		asModuleW = szModule;
	}
	else if (asModuleA)
	{
		MultiByteToWideChar(AreFileApisANSI() ? CP_ACP : CP_OEMCP, 0, asModuleA, -1, szModule, countof(szModule));
		szModule[countof(szModule)-1] = 0;
		asModuleW = szModule;
	}
	pIn = ExecuteNewCmdOnCreate(eLoadLibrary, L"Fail", asModuleW, szErrCode, NULL, NULL, NULL, NULL,
		#ifdef _WIN64
		64
		#else
		32
		#endif
		, 0, NULL, NULL, NULL);
	if (pIn)
	{
		HWND hConWnd = GetConsoleWindow();
		CESERVER_REQ* pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
		ExecuteFreeResult(pIn);
		if (pOut) ExecuteFreeResult(pOut);
	}
	SetLastError(dwErrCode);
}

// Заменить в модуле Module ЭКСпортируемые функции на подменяемые плагином нихрена
// НЕ получится, т.к. в Win32 библиотека shell32 может быть загружена ПОСЛЕ conemu.dll
//   что вызовет некорректные смещения функций,
// а в Win64 смещения вообще должны быть 64битными, а структура модуля хранит только 32битные смещения

void PrepareNewModule(HMODULE module, LPCSTR asModuleA, LPCWSTR asModuleW)
{
	if (!module)
	{
		LoadModuleFailed(asModuleA, asModuleW);
		return;
	}

	if (!ghUser32)
	{
		// Если на старте exe-шника shell32 НЕ подлинковался - нужно загрузить из него требуемые процедуры!
		if ((asModuleA && (!lstrcmpiA(asModuleA, "user32.dll") || !lstrcmpiA(asModuleA, "user32"))) ||
			(asModuleW && (!lstrcmpiW(asModuleW, L"user32.dll") || !lstrcmpiW(asModuleW, L"user32"))))
		{
			ghUser32 = LoadLibraryW(user32);
			InitHooks(NULL);
		}
	}
	if (!ghShell32)
	{
		// Если на старте exe-шника shell32 НЕ подлинковался - нужно загрузить из него требуемые процедуры!
		if ((asModuleA && (!lstrcmpiA(asModuleA, "shell32.dll") || !lstrcmpiA(asModuleA, "shell32"))) ||
			(asModuleW && (!lstrcmpiW(asModuleW, L"shell32.dll") || !lstrcmpiW(asModuleW, L"shell32"))))
		{
			ghShell32 = LoadLibraryW(shell32);
			InitHooks(NULL);
		}
	}

	if (!module || IsModuleExcluded(module, asModuleA, asModuleW))
		return;

	CShellProc* sp = new CShellProc();
	if (!sp)
		return;
	if (!gnLastLogSetChange || ((GetTickCount() - gnLastLogSetChange) > 2000))
	{
		gnLastLogSetChange = GetTickCount();
		gbLogLibraries = sp->LoadGuiMapping();
	}

	if (gbLogLibraries)
	{
		CESERVER_REQ* pIn = NULL, *pOunt = NULL;
		wchar_t szModule[MAX_PATH+1]; szModule[0] = 0;
		if (!asModuleA && !asModuleW)
		{
			wcscpy_c(szModule, L"<NULL>");
			asModuleW = szModule;
		}
		else if (asModuleA)
		{
			MultiByteToWideChar(AreFileApisANSI() ? CP_ACP : CP_OEMCP, 0, asModuleA, -1, szModule, countof(szModule));
			szModule[countof(szModule)-1] = 0;
			asModuleW = szModule;
		}
		wchar_t szInfo[64]; szInfo[0] = 0;
		#ifdef _WIN64
		if ((DWORD)((DWORD_PTR)module >> 32))
			msprintf(szInfo, countof(szInfo), L"Module=0x%08X%08X",
				(DWORD)((DWORD_PTR)module >> 32), (DWORD)((DWORD_PTR)module & 0xFFFFFFFF));
		else
			msprintf(szInfo, countof(szInfo), L"Module=0x%08X",
				(DWORD)((DWORD_PTR)module & 0xFFFFFFFF));
		#else
		msprintf(szInfo, countof(szInfo), L"Module=0x%08X", (DWORD)module);
		#endif
		pIn = sp->NewCmdOnCreate(eLoadLibrary, NULL, asModuleW, szInfo, NULL, NULL, NULL, NULL,
			#ifdef _WIN64
			64
			#else
			32
			#endif
			, 0, NULL, NULL, NULL);
		if (pIn)
		{
			HWND hConWnd = GetConsoleWindow();
			CESERVER_REQ* pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
			ExecuteFreeResult(pIn);
			if (pOut) ExecuteFreeResult(pOut);
		}
	}

	delete sp;

	// Некоторые перехватываемые библиотеки могли быть
	// не загружены во время первичной инициализации
	InitHooks(NULL);
	// Подмена импортируемых функций в module
	SetHook(module/*, FALSE*/, FALSE);
}


typedef HMODULE(WINAPI* OnLoadLibraryA_t)(const char* lpFileName);
HMODULE WINAPI OnLoadLibraryA(const char* lpFileName)
{
	ORIGINAL(LoadLibraryA);
	HMODULE module = F(LoadLibraryA)(lpFileName);

	if (gbHooksTemporaryDisabled)
		return module;

	PrepareNewModule(module, lpFileName, NULL);

	if (ph && ph->PostCallBack)
	{
		SETARGS1(&module,lpFileName);
		ph->PostCallBack(&args);
	}

	return module;
}

typedef HMODULE(WINAPI* OnLoadLibraryW_t)(const wchar_t* lpFileName);
HMODULE WINAPI OnLoadLibraryW(const wchar_t* lpFileName)
{
	ORIGINAL(LoadLibraryW);
	HMODULE module = F(LoadLibraryW)(lpFileName);

	if (gbHooksTemporaryDisabled)
		return module;

	PrepareNewModule(module, NULL, lpFileName);

	if (ph && ph->PostCallBack)
	{
		SETARGS1(&module,lpFileName);
		ph->PostCallBack(&args);
	}

	return module;
}

typedef HMODULE(WINAPI* OnLoadLibraryExA_t)(const char* lpFileName, HANDLE hFile, DWORD dwFlags);
HMODULE WINAPI OnLoadLibraryExA(const char* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	ORIGINAL(LoadLibraryExA);
	HMODULE module = F(LoadLibraryExA)(lpFileName, hFile, dwFlags);

	if (gbHooksTemporaryDisabled)
		return module;

	PrepareNewModule(module, lpFileName, NULL);

	if (ph && ph->PostCallBack)
	{
		SETARGS3(&module,lpFileName,hFile,dwFlags);
		ph->PostCallBack(&args);
	}

	return module;
}

typedef HMODULE(WINAPI* OnLoadLibraryExW_t)(const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags);
HMODULE WINAPI OnLoadLibraryExW(const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	ORIGINAL(LoadLibraryExW);
	HMODULE module = F(LoadLibraryExW)(lpFileName, hFile, dwFlags);

	if (gbHooksTemporaryDisabled)
		return module;

	PrepareNewModule(module, NULL, lpFileName);

	if (ph && ph->PostCallBack)
	{
		SETARGS3(&module,lpFileName,hFile,dwFlags);
		ph->PostCallBack(&args);
	}

	return module;
}

typedef FARPROC(WINAPI* OnGetProcAddress_t)(HMODULE hModule, LPCSTR lpProcName);
FARPROC WINAPI OnGetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	ORIGINALFAST(GetProcAddress);
	FARPROC lpfn = NULL;

	if (gbHooksTemporaryDisabled)
	{
		TODO("!!!");
	}
	else if (((DWORD_PTR)lpProcName) <= 0xFFFF)
	{
		TODO("!!! Обрабатывать и ORDINAL values !!!");
	}
	else if (gpHooks)
	{
		for(int i = 0; gpHooks[i].Name; i++)
		{
			// The spelling and case of a function name pointed to by lpProcName must be identical
			// to that in the EXPORTS statement of the source DLL's module-definition (.Def) file
			if (gpHooks[i].hDll == hModule
			        && lstrcmpA(gpHooks[i].Name, lpProcName) == 0)
			{
				lpfn = (FARPROC)gpHooks[i].NewAddress;
				break;
			}
		}
	}

	if (!lpfn)
		lpfn = F(GetProcAddress)(hModule, lpProcName);

	return lpfn;
}

TODO("по хорошему бы еще и FreeDll хукать нужно, чтобы не позвать случайно неактуальную функцию...");
typedef BOOL (WINAPI* OnFreeLibrary_t)(HMODULE hModule);
BOOL WINAPI OnFreeLibrary(HMODULE hModule)
{
	ORIGINALFAST(FreeLibrary);

	if (gbLogLibraries)
	{
		CShellProc* sp = new CShellProc();
		if (sp->LoadGuiMapping())
		{
			CESERVER_REQ* pIn = NULL, *pOunt = NULL;
			wchar_t szModule[MAX_PATH+1]; szModule[0] = 0;
			if (!GetModuleFileName(hModule, szModule, countof(szModule)))
				msprintf(szModule, countof(szModule), L"<HMODULE=0x%08X>", (DWORD)hModule);
			pIn = sp->NewCmdOnCreate(eFreeLibrary, NULL, szModule, NULL, NULL, NULL, NULL, NULL,
				#ifdef _WIN64
				64
				#else
				32
				#endif
				, 0, NULL, NULL, NULL);
			if (pIn)
			{
				HWND hConWnd = GetConsoleWindow();
				CESERVER_REQ* pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
				ExecuteFreeResult(pIn);
				if (pOut) ExecuteFreeResult(pOut);
			}
		}
		delete sp;
	}

	if (ghUser32 && (hModule == ghUser32))
	{
		Is_Window = NULL;
	}

	if (ghOnLoadLibModule == hModule)
	{
		ghOnLoadLibModule = NULL;
		gfOnLibraryLoaded = NULL;
		gfOnLibraryUnLoaded = NULL;
	}

	if (gpHooks)
	{
		for(int i = 0; i<MAX_HOOKED_PROCS && gpHooks[i].NewAddress; i++)
		{
			if (gpHooks[i].hCallbackModule == hModule)
			{
				gpHooks[i].hCallbackModule = NULL;
				gpHooks[i].PreCallBack = NULL;
				gpHooks[i].PostCallBack = NULL;
				gpHooks[i].ExceptCallBack = NULL;
			}
		}
	}

	if (gfOnLibraryUnLoaded)
	{
		gfOnLibraryUnLoaded(hModule);
	}

	BOOL lbRc = FALSE;
	lbRc = F(FreeLibrary)(hModule);
	return lbRc;
}

