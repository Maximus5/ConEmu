
/*
Copyright (c) 2009-2012 Maximus5
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

#ifdef _DEBUG
	#define HOOK_ERROR_PROC
	//#undef HOOK_ERROR_PROC
	#define HOOK_ERROR_NO ERROR_INVALID_DATA
#else
	#undef HOOK_ERROR_PROC
#endif

#define USECHECKPROCESSMODULES
//#define ASSERT_ON_PROCNOTFOUND

#include <windows.h>
#include <Tlhelp32.h>
#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"
#include "../common/WinObjects.h"
//#include "../common/MArray.h"
#include "ShellProcessor.h"
#include "SetHook.h"
#include "UserImp.h"

//#include <WinInet.h>
//#pragma comment(lib, "wininet.lib")

#ifdef _DEBUG
#define DebugString(x) //OutputDebugString(x)
#define DebugStringA(x) //OutputDebugStringA(x)
#else
#define DebugString(x) //OutputDebugString(x)
#define DebugStringA(x) //OutputDebugStringA(x)
#endif

HMODULE ghOurModule = NULL; // Хэндл нашей dll'ки (здесь хуки не ставятся)
DWORD   gnHookMainThreadId = 0;

extern HWND    ghConWnd;      // RealConsole

extern BOOL gbDllStopCalled;

#ifdef _DEBUG
bool gbSuppressShowCall = false;
bool gbSkipSuppressShowCall = false;
bool gbSkipCheckProcessModules = false;
#endif


//!!!WARNING!!! Добавляя в этот список - не забыть добавить и в GetPreloadModules() !!!
const wchar_t *kernelbase = L"kernelbase.dll",	*kernelbase_noext = L"kernelbase";
const wchar_t *kernel32 = L"kernel32.dll",	*kernel32_noext = L"kernel32";
const wchar_t *user32   = L"user32.dll",	*user32_noext   = L"user32";
const wchar_t *gdi32    = L"gdi32.dll",		*gdi32_noext    = L"gdi32";
const wchar_t *shell32  = L"shell32.dll",	*shell32_noext  = L"shell32";
const wchar_t *advapi32 = L"advapi32.dll",	*advapi32_noext = L"advapi32";
const wchar_t *comdlg32 = L"comdlg32.dll",	*comdlg32_noext = L"comdlg32";
//!!!WARNING!!! Добавляя в этот список - не забыть добавить и в GetPreloadModules() !!!
HMODULE ghKernelBase = NULL, ghKernel32 = NULL, ghUser32 = NULL, ghGdi32 = NULL, ghShell32 = NULL, ghAdvapi32 = NULL, ghComdlg32 = NULL;
HMODULE* ghSysDll[] = {&ghKernelBase, &ghKernel32, &ghUser32, &ghGdi32, &ghShell32, &ghAdvapi32, &ghComdlg32};
//!!!WARNING!!! Добавляя в этот список - не забыть добавить и в GetPreloadModules() !!!


//typedef LONG (WINAPI* RegCloseKey_t)(HKEY hKey);
RegCloseKey_t RegCloseKey_f = NULL;
//typedef LONG (WINAPI* RegOpenKeyEx_t)(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
RegOpenKeyEx_t RegOpenKeyEx_f = NULL;
//typedef LONG (WINAPI* RegCreateKeyEx_t(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
RegCreateKeyEx_t RegCreateKeyEx_f = NULL;

//typedef BOOL (WINAPI* OnChooseColorA_t)(LPCHOOSECOLORA lpcc);
OnChooseColorA_t ChooseColorA_f = NULL;
//typedef BOOL (WINAPI* OnChooseColorW_t)(LPCHOOSECOLORW lpcc);
OnChooseColorW_t ChooseColorW_f = NULL;

struct PreloadFuncs {
	LPCSTR	sFuncName;
	void**	pFuncPtr;
};
struct PreloadModules {
	LPCWSTR      sModule, sModuleNoExt;
	HMODULE     *pModulePtr;
	PreloadFuncs Funcs[5];
};

size_t GetPreloadModules(PreloadModules** ppModules)
{
	static PreloadModules Checks[] =
	{
		{user32,	user32_noext,	&ghUser32,
			{{"IsWindow", (void**)&Is_Window}}
		},
		{gdi32,		gdi32_noext,	&ghGdi32},
		{shell32,	shell32_noext,	&ghShell32},
		{advapi32,	advapi32_noext,	&ghAdvapi32,
			{{"RegOpenKeyExW", (void**)&RegOpenKeyEx_f},
			 {"RegCreateKeyExW", (void**)&RegCreateKeyEx_f},
			 {"RegCloseKey", (void**)&RegCloseKey_f}}
		},
		{comdlg32,	comdlg32_noext,	&ghComdlg32,
			{{"ChooseColorA", (void**)&ChooseColorA_f},
			 {"ChooseColorW", (void**)&ChooseColorW_f}}
		},
	};
	*ppModules = Checks;
	return countof(Checks);
}

void CheckLoadedModule(LPCWSTR asModule)
{
	if (!asModule || !*asModule)
		return;

	PreloadModules* Checks = NULL;
	size_t nChecks = GetPreloadModules(&Checks);

	for (size_t m = 0; m < nChecks; m++)
	{
		if ((*Checks[m].pModulePtr) != NULL)
			continue;

		if (!lstrcmpiW(asModule, Checks[m].sModule) || !lstrcmpiW(asModule, Checks[m].sModuleNoExt))
		{
			*Checks[m].pModulePtr = LoadLibraryW(Checks[m].sModule); // LoadLibrary, т.к. и нам он нужен - накрутить счетчик
			if ((*Checks[m].pModulePtr) != NULL)
			{
				_ASSERTEX(Checks[m].Funcs[countof(Checks[m].Funcs)-1].sFuncName == NULL);
				
				for (size_t f = 0; f < countof(Checks[m].Funcs) && Checks[m].Funcs[f].sFuncName; f++)
				{
					*Checks[m].Funcs[f].pFuncPtr = (void*)GetProcAddress(*Checks[m].pModulePtr, Checks[m].Funcs[f].sFuncName);
				}
			}
		}
	}
}

void FreeLoadedModule(HMODULE hModule)
{
	if (!hModule)
		return;
	
	PreloadModules* Checks = NULL;
	size_t nChecks = GetPreloadModules(&Checks);

	for (size_t m = 0; m < nChecks; m++)
	{
		if ((*Checks[m].pModulePtr) != hModule)
			continue;
		
		if (GetModuleHandle(Checks[m].sModule) == NULL)
		{
			// По идее, такого быть не должно, т.к. счетчик мы накрутили, библиотека не должна была выгрузиться
			_ASSERTEX(*Checks[m].pModulePtr == NULL);
			
			*Checks[m].pModulePtr = NULL;
			
			_ASSERTEX(Checks[m].Funcs[countof(Checks[m].Funcs)-1].sFuncName == NULL);
			
			for (size_t f = 0; f < countof(Checks[m].Funcs) && Checks[m].Funcs[f].sFuncName; f++)
			{
				*Checks[m].Funcs[f].pFuncPtr = NULL;
			}
		}
	}
}

#define MAX_HOOKED_PROCS 255

// Использовать GetModuleFileName или CreateToolhelp32Snapshot во время загрузки библиотек нельзя
// Однако, хранить список модулей нужно
// 1. для того, чтобы знать, в каких модулях хуки уже ставились
// 2. для информации, чтобы передать в ConEmu если пользователь включил "Shell and processes log"
struct HkModuleInfo
{
	BOOL    bUsed;   // ячейка занята
	int     Hooked;  // 1-модуль обрабатывался (хуки установлены), 2-хуки сняты
	HMODULE hModule; // хэндл
	wchar_t sModuleName[128]; // Только информационно, в обработке не участвует
	HkModuleInfo* pNext;
	HkModuleInfo* pPrev;
	size_t nAdrUsed;
	struct StrAddresses
	{
		DWORD_PTR* ppAdr;
		#ifdef _DEBUG
		DWORD_PTR ppAdrCopy1, ppAdrCopy2;
		DWORD_PTR pModulePtr, nModuleSize;
		#endif
		DWORD_PTR  pOld;
		DWORD_PTR  pOur;
		union {
			BOOL bHooked;
			LPVOID Dummy;
		};
		#ifdef _DEBUG
		char sName[32];
		#endif
	} Addresses[MAX_HOOKED_PROCS];
};
WARNING("Хорошо бы выделять память под gpHookedModules через VirtualProtect, чтобы защитить ее от изменений дурными программами");
HkModuleInfo *gpHookedModules = NULL, *gpHookedModulesLast = NULL;
size_t gnHookedModules = 0;
CRITICAL_SECTION *gpHookedModulesSection = NULL;
void InitializeHookedModules()
{
	_ASSERTE(gpHookedModules==NULL && gpHookedModulesSection==NULL);
	if (!gpHookedModulesSection)
	{
		//MessageBox(NULL, L"InitializeHookedModules", L"Hooks", MB_SYSTEMMODAL);

		gpHookedModulesSection = (CRITICAL_SECTION*)calloc(1,sizeof(*gpHookedModulesSection));
		InitializeCriticalSection(gpHookedModulesSection);

		//WARNING: "new" вызывать из DllStart нельзя! DllStart вызывается НЕ из главной нити,
		//WARNING: причем, когда главная нить еще не была запущена. В итоге, если это 
		//WARNING: попытаться сделать мы получим:
		//WARNING: runtime error R6030  - CRT not initialized
		// -- gpHookedModules = new MArray<HkModuleInfo>;
		// -- поэтому тупо через массив
		//#ifdef _DEBUG
		//gnHookedModules = 16;
		//#else
		//gnHookedModules = 256;
		//#endif
		gpHookedModules = (HkModuleInfo*)calloc(sizeof(HkModuleInfo),1);
		if (!gpHookedModules)
		{
			_ASSERTE(gpHookedModules!=NULL);
		}
		gpHookedModulesLast = gpHookedModules;
	}
}
void FinalizeHookedModules()
{
	if (gpHookedModules)
	{
		if (gpHookedModulesSection)
			EnterCriticalSection(gpHookedModulesSection);

		HkModuleInfo *p = gpHookedModules;
		gpHookedModules = NULL;
		while (p)
		{
			HkModuleInfo *pNext = p->pNext;
			free(p);
			p = pNext;
		}

		if (gpHookedModulesSection)
			LeaveCriticalSection(gpHookedModulesSection);
	}
	if (gpHookedModulesSection)
	{
		DeleteCriticalSection(gpHookedModulesSection);
		free(gpHookedModulesSection);
		gpHookedModulesSection = NULL;
	}
}
HkModuleInfo* IsHookedModule(HMODULE hModule, LPWSTR pszName = NULL, size_t cchNameMax = 0)
{
	if (!gpHookedModulesSection)
		InitializeHookedModules();

	if (!gpHookedModules)
	{
		_ASSERTE(gpHookedModules!=NULL);
		return false;
	}

	//bool lbHooked = false;

	//_ASSERTE(gpHookedModules && gpHookedModulesSection);
	//if (bSection)
	//	EnterCriticalSection(gpHookedModulesSection);

	HkModuleInfo* p = gpHookedModules;
	while (p)
	{
		if (p->bUsed && (p->hModule == hModule))
		{
			_ASSERTE(p->Hooked == 1 || p->Hooked == 2);
			//lbHooked = true;
			
			// Если хотят узнать имя модуля (по hModule)
			if (pszName && (cchNameMax > 0))
				lstrcpyn(pszName, p->sModuleName, (int)cchNameMax);
			break;
		}
		p = p->pNext;
	}

	//if (bSection)
	//	LeaveCriticalSection(gpHookedModulesSection);

	return p;
}
HkModuleInfo* AddHookedModule(HMODULE hModule, LPCWSTR sModuleName)
{
	if (!gpHookedModulesSection)
		InitializeHookedModules();

	_ASSERTE(gpHookedModules && gpHookedModulesSection);
	if (!gpHookedModules)
	{
		_ASSERTE(gpHookedModules!=NULL);
		return NULL;
	}

	HkModuleInfo* p = IsHookedModule(hModule);

	if (!p)
	{
		EnterCriticalSection(gpHookedModulesSection);
		
		p = gpHookedModules;
		while (p)
		{
			if (!p->bUsed)
			{
				p->bUsed = TRUE; // сразу зарезервируем
				gnHookedModules++;
				memset(p->Addresses, 0, sizeof(p->Addresses));
				p->nAdrUsed = 0;
				p->Hooked = 1;
				lstrcpyn(p->sModuleName, sModuleName?sModuleName:L"", countof(p->sModuleName));
				// hModule - последним, чтобы не было проблем с другими потоками
				p->hModule = hModule;
				goto done;
			}
			p = p->pNext;
		}
		
		p = (HkModuleInfo*)calloc(sizeof(HkModuleInfo),1);
		if (!p)
		{
			_ASSERTE(p!=NULL);
		}
		else
		{
			gnHookedModules++;
			p->bUsed = TRUE;   // ячейка занята. тут можно первой, т.к. в цепочку еще не добавили
			p->Hooked = 1; // модуль обрабатывался (хуки установлены)
			p->hModule = hModule; // хэндл
			lstrcpyn(p->sModuleName, sModuleName?sModuleName:L"", countof(p->sModuleName));
			//_ASSERTEX(lstrcmpi(p->sModuleName,L"dsound.dll"));
			p->pNext = NULL;
			p->pPrev = gpHookedModulesLast;
			gpHookedModulesLast->pNext = p;
			gpHookedModulesLast = p;
		}
		
	done:
		LeaveCriticalSection(gpHookedModulesSection);
	}

	return p;
}
void RemoveHookedModule(HMODULE hModule)
{
	if (!gpHookedModulesSection)
		InitializeHookedModules();

	_ASSERTE(gpHookedModules && gpHookedModulesSection);
	if (!gpHookedModules)
	{
		_ASSERTE(gpHookedModules!=NULL);
		return;
	}

	HkModuleInfo* p = gpHookedModules;
	while (p)
	{
		if (p->bUsed && (p->hModule == hModule))
		{
			gnHookedModules--;
			// Именно в такой последовательности, чтобы с другими потоками не драться
			p->Hooked = 0;
			p->bUsed = FALSE;
			break;
		}
		p = p->pNext;
	}
}



BOOL gbHooksTemporaryDisabled = FALSE;
//BOOL gbInShellExecuteEx = FALSE;

//typedef VOID (WINAPI* OnLibraryLoaded_t)(HMODULE ahModule);
HMODULE ghOnLoadLibModule = NULL;
OnLibraryLoaded_t gfOnLibraryLoaded = NULL;
OnLibraryLoaded_t gfOnLibraryUnLoaded = NULL;

// Forward declarations of the hooks
FARPROC WINAPI OnGetProcAddress(HMODULE hModule, LPCSTR lpProcName);
FARPROC WINAPI OnGetProcAddressExp(HMODULE hModule, LPCSTR lpProcName);

HMODULE WINAPI OnLoadLibraryA(const char* lpFileName);
HMODULE WINAPI OnLoadLibraryW(const WCHAR* lpFileName);
HMODULE WINAPI OnLoadLibraryExA(const char* lpFileName, HANDLE hFile, DWORD dwFlags);
HMODULE WINAPI OnLoadLibraryExW(const WCHAR* lpFileName, HANDLE hFile, DWORD dwFlags);
BOOL WINAPI OnFreeLibrary(HMODULE hModule);

#ifndef HOOKEXPADDRESSONLY
HMODULE WINAPI OnLoadLibraryAExp(const char* lpFileName);
HMODULE WINAPI OnLoadLibraryWExp(const WCHAR* lpFileName);
HMODULE WINAPI OnLoadLibraryExAExp(const char* lpFileName, HANDLE hFile, DWORD dwFlags);
HMODULE WINAPI OnLoadLibraryExWExp(const WCHAR* lpFileName, HANDLE hFile, DWORD dwFlags);
BOOL WINAPI OnFreeLibraryExp(HMODULE hModule);
#endif

//#ifdef HOOK_ANSI_SEQUENCES
BOOL WINAPI OnWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
//#endif


#ifdef HOOK_ERROR_PROC
DWORD WINAPI OnGetLastError();
VOID WINAPI OnSetLastError(DWORD dwErrCode);
#endif

HookItem *gpHooks = NULL;
size_t gnHookedFuncs = 0;

const char *szGetProcAddress = "GetProcAddress";
const char *szLoadLibraryA = "LoadLibraryA";
const char *szLoadLibraryW = "LoadLibraryW";
const char *szLoadLibraryExA = "LoadLibraryExA";
const char *szLoadLibraryExW = "LoadLibraryExW";
const char *szFreeLibrary = "FreeLibrary";
const char *szWriteConsoleW = "WriteConsoleW";
#ifdef HOOK_ERROR_PROC
const char *szGetLastError = "GetLastError";
const char *szSetLastError = "SetLastError";
#endif

#define HOOKEXPADDRESSONLY
enum HookLibFuncs
{
	hlfGetProcAddress = 0,

	#ifndef HOOKEXPADDRESSONLY
	// попробуем обойтись только GetProcAddress (для UPX)
	hlfLoadLibraryA,
	hlfLoadLibraryW,
	hlfLoadLibraryExA,
	hlfLoadLibraryExW,
	hlfFreeLibrary,
	//#ifdef HOOK_ANSI_SEQUENCES
	hlfWriteConsoleW,
	//#endif
	#endif

	//#ifdef HOOK_ERROR_PROC
	//eGetLastError,
	//eSetLastError,
	//#endif
	hlfKernelLast,
};

struct HookItemWork {
	HMODULE hLib;
	FARPROC OldAddress;
	FARPROC NewAddress;
	const char* Name;
} gKernelFuncs[hlfKernelLast] = {};/* = {
	{NULL, OnGetProcAddressExp, szGetProcAddress},
	{NULL, OnLoadLibraryAExp, szLoadLibraryA},
	{NULL, OnLoadLibraryWExp, szLoadLibraryW},
	{NULL, OnLoadLibraryExAExp, szLoadLibraryExA},
	{NULL, OnLoadLibraryExWExp, szLoadLibraryExW},
	{NULL, OnFreeLibraryExp, szFreeLibrary},
};*/

void InitKernelFuncs()
{
	#undef SETFUNC
	#define SETFUNC(m,i,f,n) \
		gKernelFuncs[i].hLib = m; \
		gKernelFuncs[i].OldAddress = NULL; \
		gKernelFuncs[i].NewAddress = (FARPROC)f; \
		gKernelFuncs[i].Name = n;

	WARNING("Захукать бы LdrGetProcAddressEx в ntdll.dll, но там нужно не просто экспорты менять, а ставить jmp на входе в функцию");
	SETFUNC(ghKernel32/*(ghKernelBase?ghKernelBase:ghKernel32)*/, hlfGetProcAddress, OnGetProcAddressExp, szGetProcAddress);
	#ifndef HOOKEXPADDRESSONLY
	SETFUNC(ghKernel32, hlfLoadLibraryA, OnLoadLibraryAExp, szLoadLibraryA);
	SETFUNC(ghKernel32, hlfLoadLibraryW, OnLoadLibraryWExp, szLoadLibraryW);
	SETFUNC(ghKernel32, hlfLoadLibraryExA, OnLoadLibraryExAExp, szLoadLibraryExA);
	SETFUNC(ghKernel32, hlfLoadLibraryExW, OnLoadLibraryExWExp, szLoadLibraryExW);
	SETFUNC(ghKernel32, hlfFreeLibrary, OnFreeLibraryExp, szFreeLibrary);
	//#ifdef HOOK_ANSI_SEQUENCES
	SETFUNC(ghKernel32, hlfWriteConsoleW, OnWriteConsoleW, szWriteConsoleW);
	//#endif
	#endif
	//#ifdef HOOK_ERROR_PROC
	//SETFUNC(eGetLastError,
	//SETFUNC(eSetLastError,
	//#endif

	// Индексы первых 6-и функций должны совпадать, т.к. там инфа по callback-ам
#ifdef _DEBUG
	if (!gpHooks)
	{
		_ASSERTEX(gpHooks!=NULL);
	}
	else
	{
		for (int f = 0; f < hlfKernelLast; f++)
		{
			_ASSERTEX(gpHooks[f].Name==gKernelFuncs[f].Name);
		}
	}
#endif

	#undef SETFUNC
}

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

	gnHookedFuncs = 0;
	#define ADDFUNC(pProc,szName,szDll) \
		gpHooks[gnHookedFuncs].NewAddress = pProc; \
		gpHooks[gnHookedFuncs].Name = szName; \
		gpHooks[gnHookedFuncs].DllName = szDll; \
		if (pProc) gnHookedFuncs++;
	/* ************************ */
	ADDFUNC((void*)OnGetProcAddress,		szGetProcAddress,		kernel32); // eGetProcAddress, ...
	ADDFUNC((void*)OnLoadLibraryA,			szLoadLibraryA,			kernel32); // ...
	ADDFUNC((void*)OnLoadLibraryW,			szLoadLibraryW,			kernel32);
	ADDFUNC((void*)OnLoadLibraryExA,		szLoadLibraryExA,		kernel32);
	ADDFUNC((void*)OnLoadLibraryExW,		szLoadLibraryExW,		kernel32);
	ADDFUNC((void*)OnFreeLibrary,			szFreeLibrary,			kernel32); // OnFreeLibrary тоже нужен!

	#ifdef HOOK_ERROR_PROC
	// Для отладки появления системных ошибок
	ADDFUNC((void*)OnGetLastError,			szGetLastError,			kernel32);
	ADDFUNC((void*)OnSetLastError,			szSetLastError,			kernel32); // eSetLastError
	#endif

	ADDFUNC(NULL,NULL,NULL);
	#undef ADDFUNC
	/* ************************ */

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
	L"advapi32.dll",
//	L"shell32.dll", -- shell нужно обрабатывать обязательно. по крайней мере в WinXP/Win2k3 (ShellExecute должен звать наш CreateProcess)
	L"wininet.dll", // какой-то криминал с этой библиотекой?
//#ifndef _DEBUG
	L"mssign32.dll",
	L"crypt32.dll",
	L"uxtheme.dll", // подозрение на exception на некоторых Win7 & Far3 (Bugs\2012\120124\Info.txt, пункт 3)
	WIN3264TEST(L"ConEmuCD.dll",L"ConEmuCD64.dll"),
	/*
	// test
	L"twext.dll",
	L"propsys.dll",
	L"ntmarta.dll",
	L"Wldap32.dll",
	L"userenv.dll",
	L"zipfldr.dll",
	L"shdocvw.dll",
	L"linkinfo.dll",
	L"ntshrui.dll",
	L"cscapi.dll",
	*/
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
		for (int i = 0; gpHooks[i].NewAddress; i++)
		{
			if (gpHooks[i].NewAddress == OurFunction)
			{
				*ph = &(gpHooks[i]);
				// По идее, ExeOldAddress должен совпадать с OldAddress, если включен "Inject ConEmuHk"
				return (abAllowModified && gpHooks[i].ExeOldAddress) ? gpHooks[i].ExeOldAddress : gpHooks[i].OldAddress;
			}
		}
	}

	_ASSERT(FALSE); // сюда мы попадать не должны
	return DefaultFunction;
}

FARPROC WINAPI GetLoadLibraryW()
{
	#ifndef HOOKEXPADDRESSONLY
	if (gKernelFuncs[hlfLoadLibraryW].OldAddress)
	{
		return (FARPROC)gKernelFuncs[hlfLoadLibraryW].OldAddress;
	}
	_ASSERTEX(gKernelFuncs[hlfLoadLibraryW].OldAddress!=NULL);
	#endif

	HookItem* ph;
	return (FARPROC)GetOriginalAddress(OnLoadLibraryW, LoadLibraryW, FALSE, &ph);
}

FARPROC WINAPI GetWriteConsoleW()
{
	#ifndef HOOKEXPADDRESSONLY
	if (gKernelFuncs[hlfWriteConsoleW].OldAddress)
	{
		return (FARPROC)gKernelFuncs[hlfWriteConsoleW].OldAddress;
	}
	_ASSERTEX(gKernelFuncs[hlfWriteConsoleW].OldAddress!=NULL);
	#endif

	//#ifdef HOOK_ANSI_SEQUENCES
	HookItem* ph;
	return (FARPROC)GetOriginalAddress(OnWriteConsoleW, WriteConsoleW, FALSE, &ph);
	//#else
	//return (FARPROC)WriteConsoleW;
	//#endif
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


//extern HANDLE ghHookMutex;
MSection* gpHookCS = NULL;

bool SetExports(HMODULE Module);

// Заполнить поле HookItem.OldAddress (реальные процедуры из внешних библиотек)
// apHooks->Name && apHooks->DllName MUST be for a lifetime
bool __stdcall InitHooks(HookItem* apHooks)
{
	size_t i, j;
	bool skip;

	if (!gpHookCS)
	{
		gpHookCS = new MSection;
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
		for (i = 0; apHooks[i].NewAddress; i++)
		{
			if (apHooks[i].Name==NULL || apHooks[i].DllName==NULL)
			{
				_ASSERTE(apHooks[i].Name!=NULL && apHooks[i].DllName!=NULL);
				break;
			}

			skip = false;

			for (j = 0; gpHooks[j].NewAddress; j++)
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
					// Не должно быть такого - функции должны только добавляться
					_ASSERTEX(lstrcmpiA(gpHooks[j].Name, apHooks[i].Name) && lstrcmpiW(gpHooks[j].DllName, apHooks[i].DllName));
					gpHooks[j].NewAddress = apHooks[i].NewAddress;
					if (j >= gnHookedFuncs)
						gnHookedFuncs = j+1;
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
				_ASSERTEX(j >= gnHookedFuncs);
				gnHookedFuncs = j+1;
				gpHooks[j+1].Name = NULL; // на всякий
				gpHooks[j+1].NewAddress = NULL; // на всякий
			}
		}
	}

	// Для добавленных в gpHooks функций определить "оригинальный" адрес экспорта
	for (i = 0; gpHooks[i].NewAddress; i++)
	{
		if (gpHooks[i].DllNameA[0] == 0)
			WideCharToMultiByte(CP_ACP, 0, gpHooks[i].DllName, -1, gpHooks[i].DllNameA, (int)countof(gpHooks[i].DllNameA), 0,0);

		if (!gpHooks[i].OldAddress)
		{
			// Сейчас - не загружаем
			HMODULE mod = GetModuleHandle(gpHooks[i].DllName);

			if (mod == NULL)
			{
				_ASSERTE(mod != NULL 
					// Библиотеки, которые могут быть НЕ подлинкованы на старте
					|| (gpHooks[i].DllName == shell32 
						|| gpHooks[i].DllName == user32 
						|| gpHooks[i].DllName == gdi32 
						|| gpHooks[i].DllName == advapi32
						|| gpHooks[i].DllName == comdlg32 
						));
			}
			else
			{
				WARNING("Тут часто возвращается XXXStub вместо самой функции!");
				gpHooks[i].OldAddress = (void*)GetProcAddress(mod, gpHooks[i].Name);

				if (gpHooks[i].OldAddress == NULL)
				{
					_ASSERTE(gpHooks[i].OldAddress != NULL);
				}
				gpHooks[i].hDll = mod;
			}
		}
	}

	// Обработать экспорты в Kernel32.dll
	static bool KernelHooked = false;
	if (!KernelHooked)
	{
		KernelHooked = true;

		_ASSERTEX(ghKernel32!=NULL);
		OSVERSIONINFO osv = {sizeof(osv)};
		GetVersionEx(&osv);
		if (osv.dwMajorVersion > 6 || (osv.dwMajorVersion == 6 && osv.dwMinorVersion > 0))
		{
			ghKernelBase = LoadLibrary(kernelbase);
		}

		InitKernelFuncs();

		WARNING("Без перехвата экспорта в kernel не работает поддержка UPX-нутых модулей");
		// Но при такой обработке валится EMenu на Win8
		TODO("Нужно вставлять jmp в начало функции LdrGetProcAddressEx в ntdll.dll");
#if 0
		// Необходимо для обработки UPX-нутых модулей
		SetExports(ghKernel32);
#endif

		/*
		if (ghKernelBase)
		{
			WARNING("will fail on Win7 x64");
			SetExports(ghKernelBase);
		}
		*/
	}
	
	return true;
}

void ShutdownHooks()
{
	UnsetAllHooks();
	
	//// Завершить работу с реестром
	//DoneHooksReg();

	// Уменьшение счетчиков загрузок (а надо ли?)
	for (size_t s = 0; s < countof(ghSysDll); s++)
	{
		if (ghSysDll[s] && *ghSysDll[s])
		{
			FreeLibrary(*ghSysDll[s]);
			*ghSysDll[s] = NULL;
		}
	}
	
	if (gpHookCS)
	{
		MSection *p = gpHookCS;
		gpHookCS = NULL;
		delete p;
	}

	FinalizeHookedModules();
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
	if (!ProcName|| !DllName)
	{
		_ASSERTE(ProcName!=NULL && DllName!=NULL);
		return false;
	}
	_ASSERTE(ProcName[0]!=0 && DllName[0]!=0);

	bool bFound = false;

	if (gpHooks)
	{
		for (int i = 0; i<MAX_HOOKED_PROCS && gpHooks[i].NewAddress; i++)
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

bool FindModuleFileName(HMODULE ahModule, LPWSTR pszName, size_t cchNameMax)
{
	bool lbFound = false;
	if (pszName && cchNameMax)
	{
		//*pszName = 0;
#ifdef _WIN64
		msprintf(pszName, cchNameMax, L"<HMODULE=0x%08X%08X> ",
			(DWORD)((((u64)ahModule) & 0xFFFFFFFF00000000) >> 32), //-V112
			(DWORD)(((u64)ahModule) & 0xFFFFFFFF)); //-V112
#else
		msprintf(pszName, cchNameMax, L"<HMODULE=0x%08X> ", (DWORD)ahModule);
#endif

		INT_PTR nLen = lstrlen(pszName);
		pszName += nLen;
		cchNameMax -= nLen;
		_ASSERTE(cchNameMax>0);
	}

	//TH32CS_SNAPMODULE - может зависать при вызовах из LoadLibrary/FreeLibrary.
	lbFound = (IsHookedModule(ahModule, pszName, cchNameMax) != NULL);

/*
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);

	if (snapshot != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 module = {sizeof(MODULEENTRY32)};

		for (BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
		{
			if (module.hModule == ahModule)
			{
				if (pszName)
					_wcscpyn_c(pszName, cchNameMax, module.szModule, cchNameMax);
				lbFound = true;
				break;
			}
		}

		CloseHandle(snapshot);
	}
*/

	return lbFound;
}

bool IsModuleExcluded(HMODULE module, LPCSTR asModuleA, LPCWSTR asModuleW)
{
	if (module == ghOurModule)
		return true;

	BOOL lbResource = LDR_IS_RESOURCE(module);
	if (lbResource)
		return true;

	// игнорировать системные библиотеки вида
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

#if 1
	for (int i = 0; ExcludedModules[i]; i++)
		if (module == GetModuleHandle(ExcludedModules[i]))
			return true;
#else
	wchar_t szModule[MAX_PATH*2]; szModule[0] = 0;
	DWORD nLen = GetModuleFileNameW(module, szModule, countof(szModule));
	if ((nLen == 0) || (nLen >= countof(szModule)))
	{
		//_ASSERTE(nLen>0 && nLen<countof(szModule));
		return true; // Что-то с модулем не то...
	}
	LPCWSTR pszName = wcsrchr(szModule, L'\\');
	if (pszName) pszName++; else pszName = szModule;
	for (int i = 0; ExcludedModules[i]; i++)
	{
		if (lstrcmpi(ExcludedModules[i], pszName) == 0)
			return true; // указан в исключениях
	}
#endif

	return false;
}


#define GetPtrFromRVA(rva,pNTHeader,imageBase) (PVOID)((imageBase)+(rva))

extern BOOL gbInCommonShutdown;

bool LockHooks(HMODULE Module, LPCWSTR asAction, MSectionLock* apCS)
{
	DWORD nCurTID = GetCurrentThreadId();

	//while (nHookMutexWait != WAIT_OBJECT_0)
	BOOL lbLockHooksSection = FALSE;
	while (!(lbLockHooksSection = apCS->Lock(gpHookCS, TRUE, 10000)))
	{
		#ifdef _DEBUG
		if (!IsDebuggerPresent())
		{
			_ASSERTE(lbLockHooksSection);
		}
		#endif

		if (gbInCommonShutdown)
			return false;

		wchar_t* szTrapMsg = (wchar_t*)calloc(1024,2);
		wchar_t* szName = (wchar_t*)calloc((MAX_PATH+1),2);

		if (!FindModuleFileName(Module, szName, MAX_PATH+1)) szName[0] = 0;

		DWORD nTID = GetCurrentThreadId(); DWORD nPID = GetCurrentProcessId();
		msprintf(szTrapMsg, 1024, 
			L"Can't %s hooks in module '%s'\nCurrent PID=%u, TID=%i\nCan't lock hook section\nPress 'Retry' to repeat locking",
			asAction, szName, nPID, nTID);

		int nBtn = 
			#ifdef CONEMU_MINIMAL
			GuiMessageBox
			#else
			MessageBoxW
			#endif
			(GetConEmuHWND(TRUE), szTrapMsg, L"ConEmu", MB_RETRYCANCEL|MB_ICONSTOP|MB_SYSTEMMODAL);

		free(szTrapMsg);
		free(szName);

		if (nBtn != IDRETRY)
			return false;

		//nHookMutexWait = WaitForSingleObject(ghHookMutex, 10000);
		//continue;
	}

	#ifdef _DEBUG
	wchar_t szDbg[80];
	msprintf(szDbg, countof(szDbg), L"ConEmuHk: LockHooks, TID=%u\n", nCurTID);
	if (nCurTID != gnHookMainThreadId)
	{
		int nDbg = 0;
	}
	DebugString(szDbg);
	#endif

	return true;
}

bool SetExportsSEH(HMODULE Module)
{
	bool lbRc = false;

	DWORD ExportDir = 0;
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)Module;
	IMAGE_NT_HEADERS* nt_header = 0;
	
	if (dos_header->e_magic == IMAGE_DOS_SIGNATURE /*'ZM'*/)
	{
		nt_header = (IMAGE_NT_HEADERS*)((char*)Module + dos_header->e_lfanew);
		if (nt_header->Signature == 0x004550)
		{
			ExportDir = (DWORD)(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		}
	}
	
	if (ExportDir != 0)
	{
		IMAGE_SECTION_HEADER* section = (IMAGE_SECTION_HEADER*)IMAGE_FIRST_SECTION(nt_header);

		for (WORD s = 0; s < nt_header->FileHeader.NumberOfSections; s++)
		{
			if (!((section[s].VirtualAddress == ExportDir) ||
			    ((section[s].VirtualAddress < ExportDir) &&
			     ((section[s].Misc.VirtualSize + section[s].VirtualAddress) > ExportDir))))
			{
				// Эта секция не содержит ExportDir
				continue;
			}

			//int nDiff = 0;//section[s].VirtualAddress - section[s].PointerToRawData;
			IMAGE_EXPORT_DIRECTORY* Export = (IMAGE_EXPORT_DIRECTORY*)((char*)Module + (ExportDir/*-nDiff*/));

			if (!Export->AddressOfNames || !Export->AddressOfNameOrdinals || !Export->AddressOfFunctions)
			{
				_ASSERTEX(Export->AddressOfNames && Export->AddressOfNameOrdinals && Export->AddressOfFunctions);
				continue;
			}

			DWORD* Name  = (DWORD*)(((BYTE*)Module) + Export->AddressOfNames);
			WORD*  Ordn  = (WORD*)(((BYTE*)Module) + Export->AddressOfNameOrdinals);
			DWORD* Shift = (DWORD*)(((BYTE*)Module) + Export->AddressOfFunctions);

			DWORD nCount = Export->NumberOfNames; // Export->NumberOfFunctions;

			DWORD old_protect = 0xCDCDCDCD;
			if (VirtualProtect(Shift, nCount * sizeof( DWORD ), PAGE_READWRITE, &old_protect ))
			{
				for (DWORD i = 0; i < nCount; i++)
				{
					char* pszExpName = ((char*)Module) + Name[i];
					DWORD nFnOrdn = Ordn[i];
					if (nFnOrdn > Export->NumberOfFunctions)
					{
						_ASSERTEX(nFnOrdn <= Export->NumberOfFunctions);
						continue;
					}
					void* ptrOldAddr = ((BYTE*)Module) + Shift[nFnOrdn];

					for (DWORD j = 0; j <= countof(gKernelFuncs); j++)
					{
						if ((Module == gKernelFuncs[j].hLib)
							&& gKernelFuncs[j].NewAddress
							&& !lstrcmpA(gKernelFuncs[j].Name, pszExpName))
						{
							gKernelFuncs[j].OldAddress = (FARPROC)ptrOldAddr;
							INT_PTR NewShift = ((BYTE*)gKernelFuncs[j].NewAddress) - ((BYTE*)Module);
							
							#ifdef _WIN64
							if (NewShift <= 0 || NewShift > (DWORD)-1)
							{
								_ASSERTEX((NewShift > 0) && (NewShift < (DWORD)-1));
								break;
							}
							#endif
							
							Shift[nFnOrdn] = (DWORD)NewShift;
							lbRc = true;

							break;
						}
					}
				}

				VirtualProtect( Shift, nCount * sizeof( DWORD ), old_protect, &old_protect );
			}
		}
	}

	return lbRc;
}

bool SetExports(HMODULE Module)
{
	_ASSERTEX((Module == ghKernel32 || Module == ghKernelBase) && Module);

	#ifdef _DEBUG
	if (Module == ghKernel32)
	{
		static bool KernelHooked = false; if (KernelHooked) { _ASSERTEX(KernelHooked==false); return false; } KernelHooked = true;
	}
	else if (Module == ghKernelBase)
	{
		static bool KernelBaseHooked = false; if (KernelBaseHooked) { _ASSERTEX(KernelBaseHooked==false); return false; } KernelBaseHooked = true;
	}
	#endif

	bool lbValid = IsModuleValid(Module);
	if ((Module == ghOurModule) || !lbValid)
	{
		_ASSERTEX(Module != ghOurModule);
		_ASSERTEX(IsModuleValid(Module));
		return false;
	}
	
	//InitKernelFuncs(); -- уже должно быть выполнено!
	_ASSERTEX(gKernelFuncs[0].NewAddress!=NULL);

	// переопределяем только первые 6 экспортов, и через gKernelFuncs
	//_ASSERTEX(gpHooks[0].Name == szGetProcAddress && gpHooks[5].Name == szFreeLibrary);
	//_ASSERTEX(gpHooks[1].Name == szLoadLibraryA && gpHooks[2].Name == szLoadLibraryW);
	//_ASSERTEX(gpHooks[3].Name == szLoadLibraryExA && gpHooks[4].Name == szLoadLibraryExW);
	
	#ifdef _WIN64
	if (((DWORD_PTR)Module) >= ((DWORD_PTR)ghOurModule))
	{
		//_ASSERTEX(((DWORD_PTR)Module) < ((DWORD_PTR)ghOurModule))
		wchar_t* pszMsg = (wchar_t*)malloc(MAX_PATH*3*sizeof(wchar_t));
		if (pszMsg)
		{
			wchar_t  szTitle[64];
			OSVERSIONINFO osv = {sizeof(osv)};
			GetVersionEx(&osv);
			msprintf(szTitle, countof(szTitle), L"ConEmuHk64, PID=%u, TID=%u", GetCurrentProcessId(), GetCurrentThreadId());
			msprintf(pszMsg, 250,
				L"ConEmuHk64.dll was loaded below Kernel32.dll\n"
				L"Some important features may be not available!\n"
				L"Please, report to developer!\n\n"
				L"OS version: %u.%u.%u (%s)\n"
				L"<ConEmuHk64.dll=0x%08X%08X>\n"
				L"<%s=0x%08X%08X>\n",
				osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber, osv.szCSDVersion,
				WIN3264WSPRINT(ghOurModule),
				(Module==ghKernelBase) ? kernelbase : kernel32,
				WIN3264WSPRINT(Module)
				);
			GetModuleFileName(ghOurModule, pszMsg+lstrlen(pszMsg), MAX_PATH);
			lstrcat(pszMsg, L"\n");
			GetModuleFileName(Module, pszMsg+lstrlen(pszMsg), MAX_PATH);
			
			GuiMessageBox(NULL, pszMsg, szTitle, MB_OK|MB_ICONSTOP|MB_SYSTEMMODAL);
			
			free(pszMsg);
		}
		return false;
	}
	#endif
	
	bool lbRc = false;
	
	SAFETRY
	{
		// В отдельной функции, а то компилятор глюкавит (под отладчиком во всяком случае куда-то не туда прыгает)
		lbRc = SetExportsSEH(Module);
	} SAFECATCH {
		lbRc = false;
	}
	
	return lbRc;
}

bool SetHookPrep(LPCWSTR asModule, HMODULE Module, BOOL abForceHooks, bool bExecutable, IMAGE_IMPORT_DESCRIPTOR* Import, size_t ImportCount, bool (&bFnNeedHook)[MAX_HOOKED_PROCS], HkModuleInfo* p);
bool SetHookChange(LPCWSTR asModule, HMODULE Module, BOOL abForceHooks, bool (&bFnNeedHook)[MAX_HOOKED_PROCS], HkModuleInfo* p);
// Подменить Импортируемые функции в модуле (Module)
//	если (abForceHooks == FALSE) то хуки не ставятся, если
//  будет обнаружен импорт, не совпадающий с оригиналом
//  Это для того, чтобы не выполнять множественные хуки при множественных LoadLibrary
bool SetHook(LPCWSTR asModule, HMODULE Module, BOOL abForceHooks)
{
	IMAGE_IMPORT_DESCRIPTOR* Import = NULL;
	DWORD Size = 0;
	HMODULE hExecutable = GetModuleHandle(0);

	if (!gpHooks)
		return false;

	if (!Module)
		Module = hExecutable;
		
	// Если он уже хукнут - не проверять больше ничего
	HkModuleInfo* p = IsHookedModule(Module);
	if (p)
		return true;
	
	if (!IsModuleValid(Module))
		return false;
		
	bool bExecutable = (Module == hExecutable);
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)Module;
	IMAGE_NT_HEADERS* nt_header = NULL;

	// Валидность адреса размером sizeof(IMAGE_DOS_HEADER) проверяется в IsModuleValid.
	if (dos_header->e_magic == IMAGE_DOS_SIGNATURE /*'ZM'*/)
	{
		nt_header = (IMAGE_NT_HEADERS*)((char*)Module + dos_header->e_lfanew);
		if (IsBadReadPtr(nt_header, sizeof(IMAGE_NT_HEADERS)))
			return false;

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
	if (!Import)
		return false;

#ifdef _DEBUG
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt_header); //-V220
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
	//DWORD nHookMutexWait = WaitForSingleObject(ghHookMutex, 10000);

	MSectionLock CS;
	if (!gpHookCS->isLockedExclusive() && !LockHooks(Module, L"install", &CS))
		return false;

	if (!p)
	{
		p = AddHookedModule(Module, asModule);
		if (!p)
			return false;
	}
		

	TODO("!!! Сохранять ORDINAL процедур !!!");
	bool res = false, bHooked = false;
	//INT_PTR i;
	INT_PTR nCount = Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
	bool bFnNeedHook[MAX_HOOKED_PROCS] = {};

	// в отдельной функции, т.к. __try
	res = SetHookPrep(asModule, Module, abForceHooks, bExecutable, Import, nCount, bFnNeedHook, p);

	// в отдельной функции, т.к. __try
	bHooked = SetHookChange(asModule, Module, abForceHooks, bFnNeedHook, p);
	
	#ifdef _DEBUG
	if (bHooked)
	{
		wchar_t* szDbg = (wchar_t*)calloc(MAX_PATH*3, 2);
		wchar_t* szModPath = (wchar_t*)calloc(MAX_PATH*2, 2);
		FindModuleFileName(Module, szModPath, MAX_PATH*2);
		_wcscpy_c(szDbg, MAX_PATH*3, L"  ## Hooks was set by conemu: ");
		_wcscat_c(szDbg, MAX_PATH*3, szModPath);
		_wcscat_c(szDbg, MAX_PATH*3, L"\n");
		DebugString(szDbg);
		free(szDbg);
		free(szModPath);
	}
	#endif

	//ReleaseMutex(ghHookMutex);
	CS.Unlock();

	// Плагин ConEmu может выполнить дополнительные действия
	if (gfOnLibraryLoaded)
	{
		gfOnLibraryLoaded(Module);
	}

	return res;
}

bool SetHookPrep(LPCWSTR asModule, HMODULE Module, BOOL abForceHooks, bool bExecutable, IMAGE_IMPORT_DESCRIPTOR* Import, size_t ImportCount, bool (&bFnNeedHook)[MAX_HOOKED_PROCS], HkModuleInfo* p)
{
	bool res = false;
	size_t i;

	//api-ms-win-core-libraryloader-l1-1-1.dll
	//api-ms-win-core-console-l1-1-0.dll
	//...
	char szCore[18];
	const char szCorePrefix[] = "api-ms-win-core-";
	const int nCorePrefLen = lstrlenA(szCorePrefix);
	_ASSERTE((nCorePrefLen+1)<countof(szCore));
	bool lbIsCodeModule = false;

	//_ASSERTEX(lstrcmpi(asModule, L"dsound.dll"));
	
	SAFETRY
	{
		//_ASSERTE(Size == (nCount * sizeof(IMAGE_IMPORT_DESCRIPTOR))); -- ровно быть не обязано
		for (i = 0; i < ImportCount; i++)
		{
			if (Import[i].Name == 0)
				break;

			//DebugString( ToTchar( (char*)Module + Import[i].Name ) );
			char* mod_name = (char*)Module + Import[i].Name;
			DWORD_PTR rvaINT = Import[i].OriginalFirstThunk;
			DWORD_PTR rvaIAT = Import[i].FirstThunk; //-V101

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

			size_t f, s, j;
			for (s = 0; s <= 1; s++)
			{
				if (s)
				{
					thunk = (IMAGE_THUNK_DATA*)GetPtrFromRVA(rvaIAT, nt_header, (PBYTE)Module);
					thunkO = (IMAGE_THUNK_DATA*)GetPtrFromRVA(rvaINT, nt_header, (PBYTE)Module);
					lstrcpynA(szCore, mod_name, nCorePrefLen+1);
					lbIsCodeModule = (lstrcmpiA(szCore, szCorePrefix) == 0);
				}

				for (f = 0;; thunk++, thunkO++, f++)
				{
					//111127 - ..\GIT\lib\perl5\site_perl\5.8.8\msys\auto\SVN\_Core\_Core.dll
					//         похоже, в этой длл кривая таблица импортов
					#ifndef USE_SEH
					BOOL lbBadThunk = IsBadReadPtr(thunk, sizeof(*thunk));
					if (lbBadThunk)
					{
						_ASSERTEX(!lbBadThunk);
						break;
					}
					#endif
					
					if (!thunk->u1.Function)
						break;
					
					#ifndef USE_SEH
					BOOL lbBadThunkO = IsBadReadPtr(thunkO, sizeof(*thunkO));
					if (lbBadThunkO)
					{
						_ASSERTEX(!lbBadThunkO);
						break;
					}
					#endif

					const char* pszFuncName = NULL;
					//ULONGLONG ordinalO = -1;

					if (thunk->u1.Function != thunkO->u1.Function)
					{
						//// Ordinal у нас пока не используется
						////if (IMAGE_SNAP_BY_ORDINAL(thunkO->u1.Ordinal))
						////{
						////	WARNING("Это НЕ ORDINAL, это Hint!!!");
						////	ordinalO = IMAGE_ORDINAL(thunkO->u1.Ordinal);
						////	pOrdinalNameO = NULL;
						////}

						
						// искать имя функции
						if (s)
						{
							if (!IMAGE_SNAP_BY_ORDINAL(thunkO->u1.Ordinal))
							{
								pOrdinalNameO = (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA(thunkO->u1.AddressOfData, nt_header, (PBYTE)Module);
								
								#ifdef USE_SEH
									pszFuncName = (LPCSTR)pOrdinalNameO->Name;
								#else
									//WARNING!!! Множественные вызовы IsBad???Ptr могут глючить и тормозить
									BOOL lbValidPtr = !IsBadReadPtr(pOrdinalNameO, sizeof(IMAGE_IMPORT_BY_NAME));
									_ASSERTE(lbValidPtr);

									if (lbValidPtr)
									{
										lbValidPtr = !IsBadStringPtrA((LPCSTR)pOrdinalNameO->Name, 10);
										_ASSERTE(lbValidPtr);

										if (lbValidPtr)
											pszFuncName = (LPCSTR)pOrdinalNameO->Name;
									}
								#endif
							}
						}
					}

					// Получили адрес функции, и (на втором шаге) имя функции
					// Теперь нужно подобрать (если есть) адрес перехвата
					for (j = 0; gpHooks[j].Name; j++)
					{
						_ASSERTEX(j<gnHookedFuncs && gnHookedFuncs<=MAX_HOOKED_PROCS);
					
						// Если не удалось определить оригинальный адрес процедуры (kernel32/WriteConsoleOutputW, и т.п.)
						if (gpHooks[j].OldAddress == NULL)
						{
							continue;
						}
						
						// Если адрес импорта в модуле уже совпадает с адресом одной из наших функций
						if (gpHooks[j].NewAddress == (void*)thunk->u1.Function)
						{
							res = true; // это уже захучено
							break;
						}
						
						#ifdef _DEBUG
						const void* ptrNewAddress = gpHooks[j].NewAddress;
						const void* ptrOldAddress = (void*)thunk->u1.Function;
						#endif

						// Проверяем адрес перехватываемой функции
						if ((void*)thunk->u1.Function != gpHooks[j].OldAddress)
						{
							// pszFuncName заполняется на втором шаге
							if (!pszFuncName /*|| !bExecutable*/)
							{
								continue; // Если имя импорта определить не удалось - пропускаем
							}
							else
							{
								// Имя модуля
								if ((lstrcmpiA(mod_name, gpHooks[j].DllNameA) != 0)
									&& !(lbIsCodeModule && (gpHooks[j].DllName == kernel32)))
									// Имя модуля не соответствует
									continue;

								// Проверяем, имя функции совпадает с перехватываемыми?
								if (lstrcmpA(pszFuncName, gpHooks[j].Name))
									continue;
							}

							if (!abForceHooks)
							{
								//_ASSERTEX(abForceHooks);
								continue; // запрещен перехват, если текущий адрес в модуле НЕ совпадает с оригинальным экспортом!
							}
							else if (bExecutable && !gpHooks[j].ExeOldAddress)
							{
								// OldAddress уже может отличаться от оригинального экспорта библиотеки
								//// Это происходит например с PeekConsoleIntputW при наличии плагина Anamorphosis
								// Про Anamorphosis несколько устарело. При включенном "Inject ConEmuHk"
								// хуки ставятся сразу при запуске процесса.
								// Но, теоретически, кто-то может успеть раньше, или флажок "Inject" выключен.

								// Также это может быть в новой архитектуре Win7 ("api-ms-win-core-..." и др.)

								gpHooks[j].ExeOldAddress = (void*)thunk->u1.Function;
							}
						}

						if (p->Addresses[j].ppAdr != NULL)
							break; // уже обработали, следующий импорт
						
						//#ifdef _DEBUG
						//// Это НЕ ORDINAL, это Hint!!!
						//if (gpHooks[j].nOrdinal == 0 && ordinalO != (ULONGLONG)-1)
						//	gpHooks[j].nOrdinal = (DWORD)ordinalO;
						//#endif


						_ASSERTE(sizeof(thunk->u1.Function)==sizeof(DWORD_PTR));
						
						if (thunk->u1.Function == (DWORD_PTR)gpHooks[j].NewAddress)
						{
							// оказалось захучено в другой нити? такого быть не должно, блокируется секцией
							// Но может быть захучено в прошлый раз, если не все модули были загружены при старте
							_ASSERTE(thunk->u1.Function != (DWORD_PTR)gpHooks[j].NewAddress);
						}
						else
						{
							bFnNeedHook[j] = true;
							p->Addresses[j].ppAdr = &thunk->u1.Function;
							#ifdef _DEBUG
							p->Addresses[j].ppAdrCopy1 = (DWORD_PTR)p->Addresses[j].ppAdr;
							p->Addresses[j].ppAdrCopy2 = (DWORD_PTR)*p->Addresses[j].ppAdr;
							p->Addresses[j].pModulePtr = (DWORD_PTR)p->hModule;
							IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((char*)p->hModule + ((IMAGE_DOS_HEADER*)p->hModule)->e_lfanew);
							p->Addresses[j].nModuleSize = nt_header->OptionalHeader.SizeOfImage;
							#endif
							//Для проверки, а то при UnsetHook("cscapi.dll") почему-то возникла ошибка ERROR_INVALID_PARAMETER в VirtualProtect
							_ASSERTEX(p->hModule==Module);
							_ASSERTEX(CheckCallbackPtr(p->hModule, 1, (FARPROC*)&p->Addresses[j].ppAdr, TRUE));
							p->Addresses[j].pOld = thunk->u1.Function;
							p->Addresses[j].pOur = (DWORD_PTR)gpHooks[j].NewAddress;
							#ifdef _DEBUG
							lstrcpynA(p->Addresses[j].sName, gpHooks[j].Name, countof(p->Addresses[j].sName));
							#endif
							
							_ASSERTEX(p->nAdrUsed < countof(p->Addresses));
							p->nAdrUsed++; //информационно
						}

						#ifdef _DEBUG
						if (bExecutable)
							gpHooks[j].ReplacedInExe = TRUE;
						#endif
							
						//DebugString( ToTchar( gpHooks[j].Name ) );
						res = true;

						break;
					} // for (j = 0; gpHooks[j].Name; j++)
				} // for (f = 0;; thunk++, thunkO++, f++)
			} // for (s = 0; s <= 1; s++)
		} // for (i = 0; i < nCount; i++)
	} SAFECATCH {
	}
	
	return res;
}

bool SetHookChange(LPCWSTR asModule, HMODULE Module, BOOL abForceHooks, bool (&bFnNeedHook)[MAX_HOOKED_PROCS], HkModuleInfo* p)
{
	bool bHooked = false;
	size_t j = 0;
	DWORD dwErr = (DWORD)-1;
	_ASSERTEX(j<gnHookedFuncs && gnHookedFuncs<=MAX_HOOKED_PROCS);
	
	SAFETRY
	{
		while (j < gnHookedFuncs)
		{
			// Может быть NULL, если импортируются не все функции
			if (p->Addresses[j].ppAdr && bFnNeedHook[j])
			{
				if (*p->Addresses[j].ppAdr == p->Addresses[j].pOur)
				{
					// оказалось захучено в другой нити или раньше
					_ASSERTEX(*p->Addresses[j].ppAdr != p->Addresses[j].pOur);
				}
				else
				{
					DWORD old_protect = 0xCDCDCDCD;
					if (!VirtualProtect(p->Addresses[j].ppAdr, sizeof(*p->Addresses[j].ppAdr),
					                   PAGE_READWRITE, &old_protect))
					{
						dwErr = GetLastError();
						_ASSERTEX(FALSE);
					}
					else
					{
						bHooked = true;
						
						*p->Addresses[j].ppAdr = p->Addresses[j].pOur;
						p->Addresses[j].bHooked = TRUE;
						
						VirtualProtect(p->Addresses[j].ppAdr, sizeof(*p->Addresses[j].ppAdr), old_protect, &old_protect);
					}

				}
			}
			
			j++;
		}
	} SAFECATCH {
		// Ошибка назначения
		p->Addresses[j].pOur = 0;
	}
	
	return bHooked;
}

DWORD GetMainThreadId()
{
	// Найти ID основной нити
	if (!gnHookMainThreadId)
	{
		DWORD dwPID = GetCurrentProcessId();
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwPID);

		if (snapshot != INVALID_HANDLE_VALUE)
		{
			THREADENTRY32 module = {sizeof(THREADENTRY32)};

			if (Thread32First(snapshot, &module))
			{
				while (!gnHookMainThreadId)
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
	return gnHookMainThreadId;
}

// Подменить Импортируемые функции во всех модулях процесса, загруженных ДО conemu.dll
// *aszExcludedModules - должны указывать на константные значения (program lifetime)
bool __stdcall SetAllHooks(HMODULE ahOurDll, const wchar_t** aszExcludedModules /*= NULL*/, BOOL abForceHooks)
{
	// т.к. SetAllHooks может быть вызван из разных dll - запоминаем однократно
	if (!ghOurModule) ghOurModule = ahOurDll;

	InitHooks(NULL);
	if (!gpHooks)
		return false;

	#ifdef _DEBUG
	wchar_t szHookProc[128];
	for (int i = 0; gpHooks[i].NewAddress; i++)
	{
		msprintf(szHookProc, countof(szHookProc), L"## %S -> 0x%08X (exe: 0x%X)\n", gpHooks[i].Name, (DWORD)gpHooks[i].NewAddress, (DWORD)gpHooks[i].ExeOldAddress);
		DebugString(szHookProc);
	}
	#endif

	// Запомнить aszExcludedModules
	if (aszExcludedModules)
	{
		INT_PTR j;
		bool skip;

		for (INT_PTR i = 0; aszExcludedModules[i]; i++)
		{
			j = 0; skip = false;

			while (ExcludedModules[j])
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
	GetMainThreadId();
	_ASSERTE(gnHookMainThreadId!=0);
	//if (!gnHookMainThreadId)
	//{
	//	DWORD dwPID = GetCurrentProcessId();
	//	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwPID);
	//	if (snapshot != INVALID_HANDLE_VALUE)
	//	{
	//		THREADENTRY32 module = {sizeof(THREADENTRY32)};
	//		if (Thread32First(snapshot, &module))
	//		{
	//			while (!gnHookMainThreadId)
	//			{
	//				if (module.th32OwnerProcessID == dwPID)
	//				{
	//					gnHookMainThreadId = module.th32ThreadID;
	//					break;
	//				}
	//				if (!Thread32Next(snapshot, &module))
	//					break;
	//			}
	//		}
	//		CloseHandle(snapshot);
	//	}
	//}

	// Начались замены во всех загруженных (linked) модулях
	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);

	if (snapshot != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 module = {sizeof(MODULEENTRY32)};

		for (BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
		{
			if (module.hModule && !IsModuleExcluded(module.hModule, NULL, module.szModule))
			{
				DebugString(module.szModule);
				SetHook(module.szModule, module.hModule/*, (module.hModule == hExecutable)*/, abForceHooks);
			}
		}

		CloseHandle(snapshot);
	}

	return true;
}


bool UnsetHookInt(HMODULE Module)
{
	bool bUnhooked = false, res = false;
	IMAGE_IMPORT_DESCRIPTOR* Import = 0;
	size_t Size = 0;
	_ASSERTE(Module!=NULL);
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)Module;

	SAFETRY
	{
		if (dos_header->e_magic == IMAGE_DOS_SIGNATURE /*'ZM'*/)
		{
			IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((char*)Module + dos_header->e_lfanew);
		
			if (nt_header->Signature != 0x004550)
				goto wrap;
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
			goto wrap;
		
		// if wrong module or no import table
		if (Module == INVALID_HANDLE_VALUE || !Import)
			goto wrap;
		
		size_t i, s;
		size_t nCount = Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
		
		//_ASSERTE(Size == (nCount * sizeof(IMAGE_IMPORT_DESCRIPTOR))); -- ровно быть не обязано
		for (s = 0; s <= 1; s++)
		{
			for (i = 0; i < nCount; i++)
			{
				if (Import[i].Name == 0)
					break;
			
				#ifdef _DEBUG
				char* mod_name = (char*)Module + Import[i].Name;
				#endif
				DWORD_PTR rvaINT = Import[i].OriginalFirstThunk;
				DWORD_PTR rvaIAT = Import[i].FirstThunk; //-V101
			
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
				for (f = 0 ;; thunk++, thunkO++, f++)
				{
					//110220 - something strange. валимся при выходе из некоторых программ (AddFont.exe)
					//         смысл в том, что thunk указывает на НЕ валидную область памяти.
					//         Разбор полетов показал, что программа сама порушила таблицу импортов.
					//Issue 466: We must check every thunk, not first (perl.exe fails?)
					//111127 - ..\GIT\lib\perl5\site_perl\5.8.8\msys\auto\SVN\_Core\_Core.dll
					//         похоже, в этой длл кривая таблица импортов
					#ifndef USE_SEH
					if (IsBadReadPtr(thunk, sizeof(IMAGE_THUNK_DATA)))
					{
						_ASSERTE(thunk && FALSE);
						break;
					}
					#endif

					if (!thunk->u1.Function)
					{
						break;
					}

					#ifndef USE_SEH
					if (IsBadReadPtr(thunkO, sizeof(IMAGE_THUNK_DATA)))
					{
						_ASSERTE(thunkO && FALSE);
						break;
					}
					#endif
			
					const char* pszFuncName = NULL;

					// Имя функции проверяем на втором шаге
					if (s && thunk->u1.Function != thunkO->u1.Function && !IMAGE_SNAP_BY_ORDINAL(thunkO->u1.Ordinal))
					{
						pOrdinalNameO = (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA(thunkO->u1.AddressOfData, nt_header, (PBYTE)Module);

						#ifdef USE_SEH
							pszFuncName = (LPCSTR)pOrdinalNameO->Name;
						#else
							//WARNING!!! Множественные вызовы IsBad???Ptr могут глючить и тормозить
							BOOL lbValidPtr = !IsBadReadPtr(pOrdinalNameO, sizeof(IMAGE_IMPORT_BY_NAME));
							#ifdef _DEBUG
							static bool bFirstAssert = false;
							if (!lbValidPtr && !bFirstAssert)
							{
								bFirstAssert = true;
								_ASSERTE(lbValidPtr);
							}
							#endif
				
							if (lbValidPtr)
							{
								//WARNING!!! Множественные вызовы IsBad???Ptr могут глючить и тормозить
								lbValidPtr = !IsBadStringPtrA((LPCSTR)pOrdinalNameO->Name, 10);
								_ASSERTE(lbValidPtr);
				
								if (lbValidPtr)
									pszFuncName = (LPCSTR)pOrdinalNameO->Name;
							}
						#endif
					}
			
					int j;
			
					for (j = 0; gpHooks[j].Name; j++)
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
						DWORD old_protect = 0xCDCDCDCD;
						if (VirtualProtect(&thunk->u1.Function, sizeof(thunk->u1.Function),
									   PAGE_READWRITE, &old_protect))
						{
							// BugBug: ExeOldAddress может отличаться от оригинального, если функция была перехвачена ДО нас
							//if (abExecutable && gpHooks[j].ExeOldAddress)
							//	thunk->u1.Function = (DWORD_PTR)gpHooks[j].ExeOldAddress;
							//else
							thunk->u1.Function = (DWORD_PTR)gpHooks[j].OldAddress;
							VirtualProtect(&thunk->u1.Function, sizeof(thunk->u1.Function), old_protect, &old_protect);
							bUnhooked = true;
						}
						//DebugString( ToTchar( gpHooks[j].Name ) );
						break; // перейти к следующему thunk-у
					}
				}
			}
		}
	wrap:
		res = bUnhooked;
	} SAFECATCH {
	}

	return res;
}

// Подменить Импортируемые функции в модуле
bool UnsetHook(HMODULE Module)
{
	if (!gpHooks)
		return false;

	if (!IsModuleValid(Module))
		return false;

	MSectionLock CS;
	if (!gpHookCS->isLockedExclusive() && !LockHooks(Module, L"uninstall", &CS))
		return false;

	HkModuleInfo* p = IsHookedModule(Module);
	bool bUnhooked = false;
	DWORD dwErr = (DWORD)-1;
	
	if (!p)
	{
		// Хотя модуль и не обрабатывался нами, но может получиться, что у него переопределенные импорты
		// Зовем в отдельной функции, т.к. __try
		bUnhooked = UnsetHookInt(Module);
	}
	else
	{
		if (p->Hooked == 1)
		{
			for (size_t i = 0; i < MAX_HOOKED_PROCS; i++)
			{
				if (p->Addresses[i].pOur == 0)
					continue; // Этот адрес поменять не смогли
			
				#ifdef _DEBUG
				//Для проверки, а то при UnsetHook("cscapi.dll") почему-то возникла ошибка ERROR_INVALID_PARAMETER в VirtualProtect
				CheckCallbackPtr(p->hModule, 1, (FARPROC*)&p->Addresses[i].ppAdr, TRUE);
				#endif

				DWORD old_protect = 0xCDCDCDCD;
				if (!VirtualProtect(p->Addresses[i].ppAdr, sizeof(*p->Addresses[i].ppAdr),
								   PAGE_READWRITE, &old_protect))
				{
					dwErr = GetLastError();
					//Один раз выскочило ERROR_INVALID_PARAMETER
					// При этом, (p->Addresses[i].ppAdr==0x04cde0e0), (p->Addresses[i].ppAdr==0x912edebf)
					// что было полной пургой. Ни одного модуля в этих адресах не было
					_ASSERTEX(dwErr==ERROR_INVALID_ADDRESS);
				}
				else
				{
					bUnhooked = true;
					// BugBug: ExeOldAddress может отличаться от оригинального, если функция была перехвачена без нас
					//if (abExecutable && gpHooks[j].ExeOldAddress)
					//	thunk->u1.Function = (DWORD_PTR)gpHooks[j].ExeOldAddress;
					//else
					*p->Addresses[i].ppAdr = p->Addresses[i].pOld;
					p->Addresses[i].bHooked = FALSE;
					VirtualProtect(p->Addresses[i].ppAdr, sizeof(*p->Addresses[i].ppAdr), old_protect, &old_protect);
				}
			}
			// Хуки сняты
			p->Hooked = 2;
		}
	}


	#ifdef _DEBUG
	if (bUnhooked && p)
	{
		wchar_t* szDbg = (wchar_t*)calloc(MAX_PATH*3, 2);
		lstrcpy(szDbg, L"  ## Hooks was UNset by conemu: ");
		lstrcat(szDbg, p->sModuleName);
		lstrcat(szDbg, L"\n");
		DebugString(szDbg);
		free(szDbg);
	}
	#endif
	
	return bUnhooked;
}

void __stdcall UnsetAllHooks()
{
	#ifdef _DEBUG
	HMODULE hExecutable = GetModuleHandle(0);
	#endif
	//Warning: TH32CS_SNAPMODULE - может зависать при вызовах из LoadLibrary/FreeLibrary.
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);

	WARNING("Убрать перехыват экспортов из Kernel32.dll");

	if (snapshot != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 module = {sizeof(module)};

		for (BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
		{
			if (module.hModule && !IsModuleExcluded(module.hModule, NULL, module.szModule))
			{
				//WARNING!!! OutputDebugString must NOT be used from ConEmuHk::DllMain(DLL_PROCESS_DETACH). See Issue 465
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


	CESERVER_REQ* pIn = NULL;
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
	pIn = ExecuteNewCmdOnCreate(NULL, ghConWnd, eLoadLibrary, L"Fail", asModuleW, szErrCode, NULL, NULL, NULL, NULL,
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

#ifdef USECHECKPROCESSMODULES
// В процессе загрузки модуля (module) могли подгрузиться
// (статически или динамически) и другие библиотеки!
void CheckProcessModules(HMODULE hFromModule);
#endif

// Заменить в модуле Module ЭКСпортируемые функции на подменяемые плагином нихрена
// НЕ получится, т.к. в Win32 библиотека shell32 может быть загружена ПОСЛЕ conemu.dll
//   что вызовет некорректные смещения функций,
// а в Win64 смещения вообще должны быть 64битными, а структура модуля хранит только 32битные смещения

bool PrepareNewModule(HMODULE module, LPCSTR asModuleA, LPCWSTR asModuleW, BOOL abNoSnapshoot = FALSE)
{
	bool lbAllSysLoaded = true;
	for (size_t s = 0; s < countof(ghSysDll); s++)
	{
		if (ghSysDll[s] && (*ghSysDll[s] == NULL))
		{
			lbAllSysLoaded = false;
			break;
		}
	}

	if (!lbAllSysLoaded)
	{
		// Некоторые перехватываемые библиотеки могли быть
		// не загружены во время первичной инициализации
		// Соответственно для них (если они появились) нужно
		// получить "оригинальные" адреса процедур
		InitHooks(NULL);
	}


	if (!module)
	{
		LoadModuleFailed(asModuleA, asModuleW);

		#ifdef USECHECKPROCESSMODULES
		// В процессе загрузки модуля (module) могли подгрузиться
		// (статически или динамически) и другие библиотеки!
		CheckProcessModules(module);
		#endif
		return false;
	}

	// Проверить по gpHookedModules а не был ли модуль уже обработан?
	if (IsHookedModule(module))
	{
		// Этот модуль уже обработан!
		return false;
	}

	bool lbModuleOk = false;

	BOOL lbResource = LDR_IS_RESOURCE(module);

	CShellProc* sp = new CShellProc();
	if (sp != NULL)
	{
		if (!gnLastLogSetChange || ((GetTickCount() - gnLastLogSetChange) > 2000))
		{
			gnLastLogSetChange = GetTickCount();
			gbLogLibraries = sp->LoadGuiMapping();
		}

		if (gbLogLibraries)
		{
			CESERVER_REQ* pIn = NULL;
			LPCWSTR pszModuleW = asModuleW;
			wchar_t szModule[MAX_PATH+1]; szModule[0] = 0;
			if (!asModuleA && !asModuleW)
			{
				wcscpy_c(szModule, L"<NULL>");
				pszModuleW = szModule;
			}
			else if (asModuleA)
			{
				MultiByteToWideChar(AreFileApisANSI() ? CP_ACP : CP_OEMCP, 0, asModuleA, -1, szModule, countof(szModule));
				szModule[countof(szModule)-1] = 0;
				pszModuleW = szModule;
			}
			wchar_t szInfo[64]; szInfo[0] = 0;
			#ifdef _WIN64
			if ((DWORD)((DWORD_PTR)module >> 32))
				msprintf(szInfo, countof(szInfo), L"Module=0x%08X%08X",
					(DWORD)((DWORD_PTR)module >> 32), (DWORD)((DWORD_PTR)module & 0xFFFFFFFF)); //-V112
			else
				msprintf(szInfo, countof(szInfo), L"Module=0x%08X",
					(DWORD)((DWORD_PTR)module & 0xFFFFFFFF)); //-V112
			#else
			msprintf(szInfo, countof(szInfo), L"Module=0x%08X", (DWORD)module);
			#endif
			pIn = sp->NewCmdOnCreate(eLoadLibrary, NULL, pszModuleW, szInfo, NULL, NULL, NULL, NULL,
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
		sp = NULL;
	}


	#ifdef USECHECKPROCESSMODULES
	if (!lbResource)
	{
		if (!abNoSnapshoot /*&& !lbResource*/)
		{
			#if 0
			// -- уже выполнено выше
			// Некоторые перехватываемые библиотеки могли быть
			// не загружены во время первичной инициализации
			// Соответственно для них (если они появились) нужно
			// получить "оригинальные" адреса процедур
			InitHooks(NULL);
			#endif

			// В процессе загрузки модуля (module) могли подгрузиться
			// (статически или динамически) и другие библиотеки!
			CheckProcessModules(module);
		}

		if (!IsModuleExcluded(module, asModuleA, asModuleW))
		{
			wchar_t szModule[128] = {};
			if (asModuleA)
			{
				LPCSTR pszNameA = strrchr(asModuleA, '\\');
				if (!pszNameA) pszNameA = asModuleA; else pszNameA++;
				MultiByteToWideChar(CP_ACP, 0, pszNameA, -1, szModule, countof(szModule)-1);				
			}
			else if (asModuleW)
			{
				LPCWSTR pszNameW = wcsrchr(asModuleW, L'\\');
				if (!pszNameW) pszNameW = asModuleW; else pszNameW++;
				lstrcpyn(szModule, pszNameW, countof(szModule));
			}
			
			lbModuleOk = true;
			// Подмена импортируемых функций в module
			SetHook(szModule, module, FALSE);
		}
	}
	#else
	lbModuleOk = true;
	#endif

	return lbModuleOk;
}

#ifdef USECHECKPROCESSMODULES
// В процессе загрузки модуля (module) могли подгрузиться
// (статически или динамически) и другие библиотеки!
void CheckProcessModules(HMODULE hFromModule)
{
#ifdef _DEBUG
	if (gbSkipCheckProcessModules)
	{
		return;
	}
#endif

	WARNING("TH32CS_SNAPMODULE - может зависать при вызовах из LoadLibrary/FreeLibrary!!!");
	// Может, имеет смысл запустить фоновую нить, в которой проверить все загруженные модули?

	//Warning: TH32CS_SNAPMODULE - может зависать при вызовах из LoadLibrary/FreeLibrary.
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	MODULEENTRY32 mi = {sizeof(mi)};
	if (h && h != INVALID_HANDLE_VALUE && Module32First(h, &mi))
	{
		BOOL lbAddMod = FALSE;
		do {
			//CheckLoadedModule(mi.szModule);
			//if (!ghUser32)
			//{
			//	// Если на старте exe-шника user32 НЕ подлинковался - нужно загрузить из него требуемые процедуры!
			//	if (*mi.szModule && (!lstrcmpiW(mi.szModule, L"user32.dll") || !lstrcmpiW(mi.szModule, L"user32")))
			//	{
			//		ghUser32 = LoadLibraryW(user32); // LoadLibrary, т.к. и нам он нужен - накрутить счетчик
			//		//InitHooks(NULL); -- ниже и так будет выполнено
			//	}
			//}
			//if (!ghShell32)
			//{
			//	// Если на старте exe-шника shell32 НЕ подлинковался - нужно загрузить из него требуемые процедуры!
			//	if (*mi.szModule && (!lstrcmpiW(mi.szModule, L"shell32.dll") || !lstrcmpiW(mi.szModule, L"shell32")))
			//	{
			//		ghShell32 = LoadLibraryW(shell32); // LoadLibrary, т.к. и нам он нужен - накрутить счетчик
			//		//InitHooks(NULL); -- ниже и так будет выполнено
			//	}
			//}
			//if (!ghAdvapi32)
			//{
			//	// Если на старте exe-шника advapi32 НЕ подлинковался - нужно загрузить из него требуемые процедуры!
			//	if (*mi.szModule && (!lstrcmpiW(mi.szModule, L"advapi32.dll") || !lstrcmpiW(mi.szModule, L"advapi32")))
			//	{
			//		ghAdvapi32 = LoadLibraryW(advapi32); // LoadLibrary, т.к. и нам он нужен - накрутить счетчик
			//		if (ghAdvapi32)
			//		{
			//			RegOpenKeyEx_f = (RegOpenKeyEx_t)GetProcAddress(ghAdvapi32, "RegOpenKeyExW");
			//			RegCreateKeyEx_f = (RegCreateKeyEx_t)GetProcAddress(ghAdvapi32, "RegCreateKeyExW");
			//			RegCloseKey_f = (RegCloseKey_t)GetProcAddress(ghAdvapi32, "RegCloseKey");
			//		}
			//		//InitHooks(NULL); -- ниже и так будет выполнено
			//	}
			//}

			if (lbAddMod)
			{
				if (PrepareNewModule(mi.hModule, NULL, mi.szModule, TRUE/*не звать CheckProcessModules*/))
					CheckLoadedModule(mi.szModule);
			}
			else if (mi.hModule == hFromModule)
			{
				lbAddMod = TRUE;
			}
		} while (Module32Next(h, &mi));
		CloseHandle(h);
	}
}
#endif

/* ************** */
HMODULE WINAPI OnLoadLibraryAWork(FARPROC lpfn, HookItem *ph, BOOL bMainThread, const char* lpFileName)
{
	typedef HMODULE(WINAPI* OnLoadLibraryA_t)(const char* lpFileName);
	HMODULE module = ((OnLoadLibraryA_t)lpfn)(lpFileName);
	DWORD dwLoadErrCode = GetLastError();

	if (gbHooksTemporaryDisabled)
		return module;

	if (PrepareNewModule(module, lpFileName, NULL))
	{
		if (ph && ph->PostCallBack)
		{
			SETARGS1(&module,lpFileName);
			ph->PostCallBack(&args);
		}
	}

	SetLastError(dwLoadErrCode);
	return module;
}

HMODULE WINAPI OnLoadLibraryA(const char* lpFileName)
{
	typedef HMODULE(WINAPI* OnLoadLibraryA_t)(const char* lpFileName);
	ORIGINAL(LoadLibraryA);
	return OnLoadLibraryAWork((FARPROC)F(LoadLibraryA), ph, bMainThread, lpFileName);
}

#ifndef HOOKEXPADDRESSONLY
HMODULE WINAPI OnLoadLibraryAExp(const char* lpFileName)
{
	BOOL bMainThread = (GetCurrentThreadId() == gnHookMainThreadId);
	return OnLoadLibraryAWork(gKernelFuncs[hlfLoadLibraryA].OldAddress, gpHooks+hlfLoadLibraryA, bMainThread, lpFileName);
}
#endif

/* ************** */
HMODULE WINAPI OnLoadLibraryWWork(FARPROC lpfn, HookItem *ph, BOOL bMainThread, const wchar_t* lpFileName)
{
	typedef HMODULE(WINAPI* OnLoadLibraryW_t)(const wchar_t* lpFileName);
	HMODULE module = NULL;

	// Спрятать ExtendedConsole.dll с глаз долой, в сервисную папку "ConEmu"
	if (lpFileName 
		&& ((lstrcmpiW(lpFileName, L"ExtendedConsole.dll") == 0)
			|| lstrcmpiW(lpFileName, L"ExtendedConsole64.dll") == 0))
	{
		CESERVER_CONSOLE_MAPPING_HDR *Info = (CESERVER_CONSOLE_MAPPING_HDR*)calloc(1,sizeof(*Info));
		if (Info && ::LoadSrvMapping(ghConWnd, *Info))
		{
			size_t cchMax = countof(Info->sConEmuBaseDir)+64;
			wchar_t* pszFullPath = (wchar_t*)calloc(cchMax,sizeof(*pszFullPath));
			if (pszFullPath)
			{
				_wcscpy_c(pszFullPath, cchMax, Info->sConEmuBaseDir);
				_wcscat_c(pszFullPath, cchMax, WIN3264TEST(L"\\ExtendedConsole.dll",L"\\ExtendedConsole64.dll"));

				module = ((OnLoadLibraryW_t)lpfn)(pszFullPath);

				SafeFree(pszFullPath);
			}
		}
		SafeFree(Info);
	}
	
	if (!module)
		module = ((OnLoadLibraryW_t)lpfn)(lpFileName);
	DWORD dwLoadErrCode = GetLastError();

	if (gbHooksTemporaryDisabled)
		return module;

	// Больше не требуется. Загрузка ExtendedConsole обработана выше
#if 0
	// Far 3 x64 все равно пытается загрузить L"ExtendedConsole.dll" вместо L"ExtendedConsole64.dll"
	#ifdef _WIN64
	if (!module)
	{
		DWORD dwErrCode = dwLoadErrCode;
		//0x7E - module not found
		//0xC1 - module не является приложением Win32.
		if ((dwErrCode == ERROR_MOD_NOT_FOUND || dwErrCode == ERROR_BAD_EXE_FORMAT)
			&& lpFileName && (lstrcmpiW(lpFileName, L"extendedconsole.dll") == 0))
		{
			module = ((OnLoadLibraryW_t)lpfn)(L"ExtendedConsole64.dll");
		}
	}
	#endif
#endif

	
	if (PrepareNewModule(module, NULL, lpFileName))
	{
		if (ph && ph->PostCallBack)
		{
			SETARGS1(&module,lpFileName);
			ph->PostCallBack(&args);
		}
	}

	SetLastError(dwLoadErrCode);
	return module;
}

HMODULE WINAPI OnLoadLibraryW(const wchar_t* lpFileName)
{
	typedef HMODULE(WINAPI* OnLoadLibraryW_t)(const wchar_t* lpFileName);
	ORIGINAL(LoadLibraryW);
	return OnLoadLibraryWWork((FARPROC)F(LoadLibraryW), ph, bMainThread, lpFileName);
}

#ifndef HOOKEXPADDRESSONLY
HMODULE WINAPI OnLoadLibraryWExp(const wchar_t* lpFileName)
{
	BOOL bMainThread = (GetCurrentThreadId() == gnHookMainThreadId);
	return OnLoadLibraryWWork(gKernelFuncs[hlfLoadLibraryW].OldAddress, gpHooks+hlfLoadLibraryW, bMainThread, lpFileName);
}
#endif

/* ************** */
HMODULE WINAPI OnLoadLibraryExAWork(FARPROC lpfn, HookItem *ph, BOOL bMainThread, const char* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	typedef HMODULE(WINAPI* OnLoadLibraryExA_t)(const char* lpFileName, HANDLE hFile, DWORD dwFlags);
	HMODULE module = ((OnLoadLibraryExA_t)lpfn)(lpFileName, hFile, dwFlags);
	DWORD dwLoadErrCode = GetLastError();

	if (gbHooksTemporaryDisabled)
		return module;

	if (PrepareNewModule(module, lpFileName, NULL))
	{
		if (ph && ph->PostCallBack)
		{
			SETARGS3(&module,lpFileName,hFile,dwFlags);
			ph->PostCallBack(&args);
		}
	}

	SetLastError(dwLoadErrCode);
	return module;
}

HMODULE WINAPI OnLoadLibraryExA(const char* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	typedef HMODULE(WINAPI* OnLoadLibraryExA_t)(const char* lpFileName, HANDLE hFile, DWORD dwFlags);
	ORIGINAL(LoadLibraryExA);
	return OnLoadLibraryExAWork((FARPROC)F(LoadLibraryExA), ph, bMainThread, lpFileName, hFile, dwFlags);
}

#ifndef HOOKEXPADDRESSONLY
HMODULE WINAPI OnLoadLibraryExAExp(const char* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	BOOL bMainThread = (GetCurrentThreadId() == gnHookMainThreadId);
	return OnLoadLibraryExAWork(gKernelFuncs[hlfLoadLibraryExA].OldAddress, gpHooks+hlfLoadLibraryExA, bMainThread, lpFileName, hFile, dwFlags);
}
#endif

/* ************** */
HMODULE WINAPI OnLoadLibraryExWWork(FARPROC lpfn, HookItem *ph, BOOL bMainThread, const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	typedef HMODULE(WINAPI* OnLoadLibraryExW_t)(const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags);
	HMODULE module = ((OnLoadLibraryExW_t)lpfn)(lpFileName, hFile, dwFlags);
	DWORD dwLoadErrCode = GetLastError();

	if (gbHooksTemporaryDisabled)
		return module;

	if (PrepareNewModule(module, NULL, lpFileName))
	{
		if (ph && ph->PostCallBack)
		{
			SETARGS3(&module,lpFileName,hFile,dwFlags);
			ph->PostCallBack(&args);
		}
	}

	SetLastError(dwLoadErrCode);
	return module;
}

HMODULE WINAPI OnLoadLibraryExW(const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	typedef HMODULE(WINAPI* OnLoadLibraryExW_t)(const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags);
	ORIGINAL(LoadLibraryExW);
	return OnLoadLibraryExWWork((FARPROC)F(LoadLibraryExW), ph, bMainThread, lpFileName, hFile, dwFlags);
}

#ifndef HOOKEXPADDRESSONLY
HMODULE WINAPI OnLoadLibraryExWExp(const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	BOOL bMainThread = (GetCurrentThreadId() == gnHookMainThreadId);
	return OnLoadLibraryExWWork(gKernelFuncs[hlfLoadLibraryExW].OldAddress, gpHooks+hlfLoadLibraryExW, bMainThread, lpFileName, hFile, dwFlags);
}
#endif

/* ************** */
FARPROC WINAPI OnGetProcAddressWork(FARPROC lpfn, HookItem *ph, BOOL bMainThread, HMODULE hModule, LPCSTR lpProcName)
{
	typedef FARPROC(WINAPI* OnGetProcAddress_t)(HMODULE hModule, LPCSTR lpProcName);
	FARPROC lpfnRc = NULL;

	#ifdef LOG_ORIGINAL_CALL
	char gszLastGetProcAddress[255], lsProcNameCut[64];

	if (((DWORD_PTR)lpProcName) <= 0xFFFF)
		msprintf(lsProcNameCut, countof(lsProcNameCut), "%u", LOWORD(lpProcName));
	else
		lstrcpynA(lsProcNameCut, lpProcName, countof(lsProcNameCut));
	#endif

	WARNING("Убрать gbHooksTemporaryDisabled?");
	if (gbHooksTemporaryDisabled)
	{
		TODO("!!!");
		#ifdef LOG_ORIGINAL_CALL
		msprintf(gszLastGetProcAddress, countof(gszLastGetProcAddress), "   OnGetProcAddress(x%08X,%s)",
			(DWORD)hModule, lsProcNameCut);
		#endif
	}
	else if (((DWORD_PTR)lpProcName) <= 0xFFFF)
	{
		TODO("!!! Обрабатывать и ORDINAL values !!!");
		#ifdef LOG_ORIGINAL_CALL
		msprintf(gszLastGetProcAddress, countof(gszLastGetProcAddress), "   OnGetProcAddress(x%08X,%s)",
			(DWORD)hModule, lsProcNameCut);
		#endif
	}
	else
	{
		#ifdef LOG_ORIGINAL_CALL
		msprintf(gszLastGetProcAddress, countof(gszLastGetProcAddress), "   OnGetProcAddress(x%08X,%s)",
			(DWORD)hModule, lsProcNameCut);
		#endif

		if (gpHooks)
		{
			if (gbDllStopCalled)
			{
				//-- assert нельзя, т.к. все уже деинициализировано!
				//_ASSERTE(ghHeap!=NULL);
				lpfnRc = NULL;
			}
			else
			{
				for (int i = 0; gpHooks[i].Name; i++)
				{
					// The spelling and case of a function name pointed to by lpProcName must be identical
					// to that in the EXPORTS statement of the source DLL's module-definition (.Def) file
					if (gpHooks[i].hDll == hModule
							&& lstrcmpA(gpHooks[i].Name, lpProcName) == 0)
					{
						lpfnRc = (FARPROC)gpHooks[i].NewAddress;
						break;
					}
				}
			}
		}
	}

	#ifdef LOG_ORIGINAL_CALL
	if (lpfnRc)
		lstrcatA(gszLastGetProcAddress, " - hooked");
	#endif

	if (!lpfnRc)
	{
		lpfnRc = ((OnGetProcAddress_t)lpfn)(hModule, lpProcName);

		#ifdef _DEBUG
		#ifdef ASSERT_ON_PROCNOTFOUND
		DWORD dwErr = GetLastError();
		_ASSERTEX(lpfnRc != NULL);
		SetLastError(dwErr);
		#endif
		#endif
	}

	#ifdef LOG_ORIGINAL_CALL
	int nLeft = lstrlenA(gszLastGetProcAddress);
	msprintf(gszLastGetProcAddress+nLeft, countof(gszLastGetProcAddress)-nLeft, 
		WIN3264TEST(" - 0x%08X\n"," - 0x%08X%08X\n"), WIN3264WSPRINT(lpfnRc));
	DebugStringA(gszLastGetProcAddress);
	#endif

	return lpfnRc;
}

FARPROC WINAPI OnGetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	typedef FARPROC(WINAPI* OnGetProcAddress_t)(HMODULE hModule, LPCSTR lpProcName);
	ORIGINALFAST(GetProcAddress);
	return OnGetProcAddressWork((FARPROC)F(GetProcAddress), ph, FALSE, hModule, lpProcName);
}
FARPROC WINAPI OnGetProcAddressExp(HMODULE hModule, LPCSTR lpProcName)
{
	return OnGetProcAddressWork(gKernelFuncs[hlfGetProcAddress].OldAddress, gpHooks+hlfGetProcAddress, FALSE, hModule, lpProcName);
}

/* ************** */
BOOL WINAPI OnFreeLibraryWork(FARPROC lpfn, HookItem *ph, BOOL bMainThread, HMODULE hModule)
{
	typedef BOOL (WINAPI* OnFreeLibrary_t)(HMODULE hModule);
	BOOL lbRc = FALSE;
	BOOL lbResource = LDR_IS_RESOURCE(hModule);
	// lbResource получается TRUE например при вызовах из version.dll
	//BOOL lbProcess = !lbResource;
	wchar_t szModule[MAX_PATH*2]; szModule[0] = 0;

	if (gbLogLibraries && !gbDllStopCalled)
	{
		CShellProc* sp = new CShellProc();
		if (sp->LoadGuiMapping())
		{
			CESERVER_REQ* pIn = NULL;
			szModule[0] = 0;
			wchar_t szHandle[32] = {};
			#ifdef _WIN64
				msprintf(szHandle, countof(szModule), L", <HMODULE=0x%08X%08X>",
					(DWORD)((((u64)hModule) & 0xFFFFFFFF00000000) >> 32), //-V112
					(DWORD)(((u64)hModule) & 0xFFFFFFFF)); //-V112
			#else
				msprintf(szHandle, countof(szModule), L", <HMODULE=0x%08X>", (DWORD)hModule);
			#endif
			
			// GetModuleFileName в некоторых случаях зависает O_O. Поэтому, запоминаем в локальном массиве имя загруженного ранее модуля
			if (FindModuleFileName(hModule, szModule, countof(szModule)-lstrlen(szModule)-1))
				wcscat_c(szModule, szHandle);
			else
				wcscpy_c(szModule, szHandle+2);

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

#ifdef _DEBUG
	BOOL lbModulePre = IsModuleValid(hModule); // GetModuleFileName(hModule, szModule, countof(szModule));
#endif

	DWORD dwFreeErrCode = 0;

	// for unlocking CS
	{
		//-- Locking is inadmissible. One FreeLibrary may cause another FreeLibrary in _different_ thread. 
		//MSectionLock CS;
		//if (gpHookCS->isLockedExclusive() || LockHooks(hModule, L"free", &CS))
		{
			lbRc = ((OnFreeLibrary_t)lpfn)(hModule);
			dwFreeErrCode = GetLastError();
		}
		//else
		//{
		//	lbRc = FALSE;
		//	dwFreeErrCode = E_UNEXPECTED;
		//}
	}

	// Далее только если !LDR_IS_RESOURCE
	if (lbRc && !lbResource && !gbDllStopCalled)
	{
		// Попробуем определить, действительно ли модуль выгружен, или только счетчик уменьшился
		BOOL lbModulePost = IsModuleValid(hModule); // GetModuleFileName(hModule, szModule, countof(szModule));
		#ifdef _DEBUG
		DWORD dwErr = lbModulePost ? 0 : GetLastError();
		#endif

		if (!lbModulePost)
		{
			RemoveHookedModule(hModule);

			if (ghOnLoadLibModule == hModule)
			{
				ghOnLoadLibModule = NULL;
				gfOnLibraryLoaded = NULL;
				gfOnLibraryUnLoaded = NULL;
			}

			if (gpHooks)
			{
				for (int i = 0; i<MAX_HOOKED_PROCS && gpHooks[i].NewAddress; i++)
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
			
			TODO("Тоже на цикл переделать, как в CheckLoadedModule");

			if (gfOnLibraryUnLoaded)
			{
				gfOnLibraryUnLoaded(hModule);
			}

			// Если выгружена библиотека ghUser32/ghAdvapi32/ghComdlg32...
			// проверить, может какие наши импорты стали невалидными
			FreeLoadedModule(hModule);
		}
	}

	SetLastError(dwFreeErrCode);
	return lbRc;
}

BOOL WINAPI OnFreeLibrary(HMODULE hModule)
{
	if (user->isUser32(hModule))
	{
		// нельзя, иначе пойдут ошибки доступа
		return FALSE;
	}
	typedef BOOL (WINAPI* OnFreeLibrary_t)(HMODULE hModule);
	ORIGINALFAST(FreeLibrary);
	return OnFreeLibraryWork((FARPROC)F(FreeLibrary), ph, FALSE, hModule);
}
#ifndef HOOKEXPADDRESSONLY
BOOL WINAPI OnFreeLibraryExp(HMODULE hModule)
{
	typedef BOOL (WINAPI* OnFreeLibrary_t)(HMODULE hModule);
	return OnFreeLibraryWork(gKernelFuncs[hlfFreeLibrary].OldAddress, gpHooks+hlfFreeLibrary, FALSE, hModule);
}
#endif

#ifdef HOOK_ERROR_PROC
DWORD WINAPI OnGetLastError()
{
	typedef DWORD (WINAPI* OnGetLastError_t)();
	SUPPRESSORIGINALSHOWCALL;
	ORIGINALFAST(GetLastError);
	DWORD nErr = 0;

	if (F(GetLastError))
		nErr = F(GetLastError)();

	if (nErr == HOOK_ERROR_NO)
	{
		nErr = HOOK_ERROR_NO;
	}
	return nErr;
}
VOID WINAPI OnSetLastError(DWORD dwErrCode)
{
	typedef DWORD (WINAPI* OnSetLastError_t)(DWORD dwErrCode);
	SUPPRESSORIGINALSHOWCALL;
	ORIGINALFAST(SetLastError);

	if (dwErrCode == HOOK_ERROR_NO)
	{
		dwErrCode = HOOK_ERROR_NO;
	}

	if (F(SetLastError))
		F(SetLastError)(dwErrCode);
}
#endif




/* ***************** Logging ****************** */
#ifdef LOG_ORIGINAL_CALL
void LogFunctionCall(LPCSTR asFunc, LPCSTR asFile, int anLine)
{
	if (!gbSuppressShowCall || gbSkipSuppressShowCall)
	{
		DWORD nErr = GetLastError();
		char sFunc[128]; _wsprintfA(sFunc, SKIPLEN(countof(sFunc)) "Hook: %s\n", asFunc);
		DebugStringA(sFunc);
		SetLastError(nErr);
	}
	else
	{
		gbSuppressShowCall = false;
	}
}
#endif