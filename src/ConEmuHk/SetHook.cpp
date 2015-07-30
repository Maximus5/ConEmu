
/*
Copyright (c) 2009-2015 Maximus5
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
//#define SKIPHOOKLOG

//#define USE_ONLY_INT_CHECK_PTR
#undef USE_ONLY_INT_CHECK_PTR

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
#if !defined(__GNUC__) || defined(__MINGW64_VERSION_MAJOR)
#include <intrin.h>
#else
#define _InterlockedIncrement InterlockedIncrement
#endif
#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"
#include "../common/MSection.h"
#include "../common/WObjects.h"
//#include "../common/MArray.h"
#include "ShellProcessor.h"
#include "SetHook.h"
#include "ConEmuHooks.h"
#include "Ansi.h"
#include "MainThread.h"


#ifdef _DEBUG
	//WARNING!!! OutputDebugString must NOT be used from ConEmuHk::DllMain(DLL_PROCESS_DETACH). See Issue 465
	#define DebugString(x) //if ((gnDllState != ds_DllProcessDetach) || gbIsSshProcess) OutputDebugString(x)
	#define DebugStringA(x) //if ((gnDllState != ds_DllProcessDetach) || gbIsSshProcess) OutputDebugStringA(x)
#else
	#define DebugString(x)
	#define DebugStringA(x)
#endif


HMODULE ghOurModule = NULL; // Хэндл нашей dll'ки (здесь хуки не ставятся)
MMap<DWORD,BOOL> gStartedThreads;

extern HWND    ghConWnd;      // RealConsole

extern BOOL gbDllStopCalled;
extern BOOL gbHooksWasSet;

extern bool gbPrepareDefaultTerminal;

#ifdef _DEBUG
bool gbSuppressShowCall = false;
bool gbSkipSuppressShowCall = false;
bool gbSkipCheckProcessModules = false;
#endif

bool gbHookExecutableOnly = false;


//!!!All dll names MUST BE LOWER CASE!!!
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

struct UNICODE_STRING
{
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
};
enum LDR_DLL_NOTIFICATION_REASON
{
	LDR_DLL_NOTIFICATION_REASON_LOADED = 1,
	LDR_DLL_NOTIFICATION_REASON_UNLOADED = 2,
};
struct LDR_DLL_LOADED_NOTIFICATION_DATA
{
    ULONG Flags;                    //Reserved.
    const UNICODE_STRING* FullDllName;   //The full path name of the DLL module.
    const UNICODE_STRING* BaseDllName;   //The base file name of the DLL module.
    PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
    ULONG SizeOfImage;              //The size of the DLL image, in bytes.
};
struct LDR_DLL_UNLOADED_NOTIFICATION_DATA
{
    ULONG Flags;                    //Reserved.
    const UNICODE_STRING* FullDllName;   //The full path name of the DLL module.
    const UNICODE_STRING* BaseDllName;   //The base file name of the DLL module.
    PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
    ULONG SizeOfImage;              //The size of the DLL image, in bytes.
};
union LDR_DLL_NOTIFICATION_DATA
{
    LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
    LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
};
typedef VOID (CALLBACK* PLDR_DLL_NOTIFICATION_FUNCTION)(ULONG NotificationReason, const LDR_DLL_NOTIFICATION_DATA* NotificationData, PVOID Context);
VOID CALLBACK LdrDllNotification(ULONG NotificationReason, const LDR_DLL_NOTIFICATION_DATA* NotificationData, PVOID Context);
typedef NTSTATUS (NTAPI* LdrRegisterDllNotification_t)(ULONG Flags, PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction, PVOID Context, PVOID *Cookie);
typedef NTSTATUS (NTAPI* LdrUnregisterDllNotification_t)(PVOID Cookie);
static LdrRegisterDllNotification_t LdrRegisterDllNotification = NULL;
static LdrUnregisterDllNotification_t LdrUnregisterDllNotification = NULL;
static PVOID gpLdrDllNotificationCookie = NULL;
static NTSTATUS gnLdrDllNotificationState = (NTSTATUS)-1;
static bool gbLdrDllNotificationUsed = false;

// Forwards
bool PrepareNewModule(HMODULE module, LPCSTR asModuleA, LPCWSTR asModuleW, BOOL abNoSnapshoot = FALSE, BOOL abForceHooks = FALSE);
void UnprepareModule(HMODULE hModule, LPCWSTR pszModule, int iStep);


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
MSectionSimple *gpHookedModulesSection = NULL;
void InitializeHookedModules()
{
	_ASSERTE(gpHookedModules==NULL && gpHookedModulesSection==NULL);
	if (!gpHookedModulesSection)
	{
		//MessageBox(NULL, L"InitializeHookedModules", L"Hooks", MB_SYSTEMMODAL);

		gpHookedModulesSection = new MSectionSimple(true);

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
	HLOG1("FinalizeHookedModules",0);
	if (gpHookedModules)
	{
		MSectionLockSimple CS;
		if (gpHookedModulesSection)
			CS.Lock(gpHookedModulesSection);

		HkModuleInfo *p = gpHookedModules;
		gpHookedModules = NULL;
		while (p)
		{
			HkModuleInfo *pNext = p->pNext;
			free(p);
			p = pNext;
		}
	}
	SafeDelete(gpHookedModulesSection);
	HLOGEND1();
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
	//	Enter Critical Section(gpHookedModulesSection);

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
	//	Leave Critical Section(gpHookedModulesSection);

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
		MSectionLockSimple CS;
		CS.Lock(gpHookedModulesSection);

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
				goto wrap;
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
	}

wrap:
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


#ifdef HOOK_ERROR_PROC
DWORD WINAPI OnGetLastError();
VOID WINAPI OnSetLastError(DWORD dwErrCode);
#endif

HookItem *gpHooks = NULL;
size_t gnHookedFuncs = 0;
//bool gbHooksSorted = false;

#if 0
struct HookItemNode
{
	const char* Name;
	HookItem *p;
	HookItemNode *pLeft;
	HookItemNode *pRight;
#ifdef _DEBUG
	size_t nLeftCount;
	size_t nRightCount;
#endif
};
HookItemNode *gpHooksTree = NULL; // [MAX_HOOKED_PROCS]
HookItemNode *gpHooksRoot = NULL; // Pointer to the "root" item in gpHooksTree
#endif

//struct HookItemNodePtr
//{
//	const void* Address;
//	HookItem *p;
//	HookItemNodePtr *pLeft;
//	HookItemNodePtr *pRight;
//#ifdef _DEBUG
//	size_t nLeftCount;
//	size_t nRightCount;
//#endif
//};
//HookItemNodePtr *gpHooksTreePtr = NULL; // [MAX_HOOKED_PROCS]
//HookItemNodePtr *gpHooksRootPtr = NULL; // Pointer to the "root" item in gpHooksTreePtr
//MSectionSimple* gpcsHooksRootPtr = NULL;
//HookItemNodePtr *gpHooksTreeNew = NULL; // [MAX_HOOKED_PROCS]
//HookItemNodePtr *gpHooksRootNew = NULL; // Pointer to the "root" item in gpHooksTreePtr

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
	hlfKernelLast,
};

struct HookItemWork {
	HMODULE hLib;
	FARPROC OldAddress;
	FARPROC NewAddress;
	const char* Name;
} gKernelFuncs[hlfKernelLast] = {};/* = {
	{NULL, OnGetProcAddressExp, szGetProcAddress},
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

	// Индексы первых функций должны совпадать, т.к. там инфа по callback-ам
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
		if (pProc/*need to be, ignore GCC warn*/) gnHookedFuncs++;
	/* ************************ */
	ADDFUNC((void*)OnGetProcAddress,		szGetProcAddress,		kernel32); // eGetProcAddress, ...

	// No need to hook these functions in Vista+
	if (!gbLdrDllNotificationUsed)
	{
		ADDFUNC((void*)OnLoadLibraryA,		szLoadLibraryA,			kernel32); // ...
		ADDFUNC((void*)OnLoadLibraryExA,	szLoadLibraryExA,		kernel32);
		ADDFUNC((void*)OnLoadLibraryExW,	szLoadLibraryExW,		kernel32);
		ADDFUNC((void*)OnFreeLibrary,		szFreeLibrary,			kernel32); // OnFreeLibrary тоже нужен!
	}
	// With only exception of LoadLibraryW - it handles "ExtendedConsole.dll" loading in Far 64
	if (gbIsFarProcess || !gbLdrDllNotificationUsed)
	{
		ADDFUNC((void*)OnLoadLibraryW,			szLoadLibraryW,			kernel32);
	}

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
	L"setupapi.dll", // "ConEmu\Bugs\2012\z120711\"
	L"uxtheme.dll", // подозрение на exception на некоторых Win7 & Far3 (Bugs\2012\120124\Info.txt, пункт 3)
	WIN3264TEST(L"ConEmuCD.dll",L"ConEmuCD64.dll"), // Loaded in-process when AlternativeServer is started
	WIN3264TEST(L"ExtendedConsole.dll",L"ExtendedConsole64.dll"), // Our API for Far Manager TrueColor support
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

	_ASSERT(!gbHooksWasSet || (gbLdrDllNotificationUsed && !gbIsFarProcess) || (!abAllowModified && !DefaultFunction));
	return DefaultFunction;
}

FARPROC WINAPI GetLoadLibraryW()
{
	HookItem* ph;
	return (FARPROC)GetOriginalAddress((void*)(FARPROC)OnLoadLibraryW, (void*)(FARPROC)LoadLibraryW, FALSE, &ph);
}

FARPROC WINAPI GetWriteConsoleW()
{
	HookItem* ph;
	return (FARPROC)GetOriginalAddress((void*)(FARPROC)CEAnsi::OnWriteConsoleW, (void*)(FARPROC)WriteConsoleW, FALSE, &ph);
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


MSection* gpHookCS = NULL;

bool SetExports(HMODULE Module);

DWORD CalculateNameCRC32(const char *apszName)
{
#if 1

	DWORD nCRC32 = 0xFFFFFFFF;

	static DWORD CRCtable[] = {
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 
	0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 
	0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 
	0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 
	0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 
	0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 
	0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106, 
	0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 
	0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 
	0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 
	0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 
	0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 
	0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 
	0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 
	0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E, 
	0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, 
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 
	0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 
	0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 
	0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242, 
	0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 
	0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 
	0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9, 
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 
	0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D };

	DWORD dwRead = lstrlenA(apszName);
	for (LPBYTE p = (LPBYTE)apszName; (dwRead--);)
	{
		nCRC32 = ( nCRC32 >> 8 ) ^ CRCtable[(unsigned char) ((unsigned char) nCRC32 ^ *p++ )];
	}

	// т.к. нас интересует только сравнение - последний XOR необязателен!
	//nCRC32 = ( nCRC32 ^ 0xFFFFFFFF );

#else
	// Этот "облегченный" алгоритм был расчитан на wchar_t
	DWORD nDwordCount = (anNameLen+1) >> 1;

	DWORD nCRC32 = 0x7A3B91F4;
	for (DWORD i = 0; i < nDwordCount; i++)
		nCRC32 ^= ((LPDWORD)apszName)[i];
#endif

	return nCRC32;
}


// Заполнить поле HookItem.OldAddress (реальные процедуры из внешних библиотек)
// apHooks->Name && apHooks->DllName MUST be for a lifetime
bool __stdcall InitHooks(HookItem* apHooks)
{
	size_t i, j;
	bool skip;

	static bool bLdrWasChecked = false;
	if (!bLdrWasChecked)
	{
		#ifndef _WIN32_WINNT_WIN8
		#define _WIN32_WINNT_WIN8 0x602
		#endif
		_ASSERTE(_WIN32_WINNT_WIN8==0x602);
		OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_WIN8), LOBYTE(_WIN32_WINNT_WIN8)};
		DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
		BOOL isAllowed = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);

		// LdrDllNotification работает так как нам надо начиная с Windows 8
		// В предыдущих версиях Windows нотификатор вызывается из LdrpFindOrMapDll
		// ДО того, как были обработаны импорты функцией LdrpProcessStaticImports (а точнее LdrpSnapThunk)

		if (isAllowed)
		{
			HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
			if (hNtDll)
			{
				LdrRegisterDllNotification = (LdrRegisterDllNotification_t)GetProcAddress(hNtDll, "LdrRegisterDllNotification");
				LdrUnregisterDllNotification = (LdrUnregisterDllNotification_t)GetProcAddress(hNtDll, "LdrUnregisterDllNotification");

				if (LdrRegisterDllNotification && LdrUnregisterDllNotification)
				{
					gnLdrDllNotificationState = LdrRegisterDllNotification(0, LdrDllNotification, NULL, &gpLdrDllNotificationCookie);
					gbLdrDllNotificationUsed = (gnLdrDllNotificationState == 0/*STATUS_SUCCESS*/);
				}
			}
		}

		bLdrWasChecked = true;
	}

#if 0
	if (gbHooksSorted && apHooks)
	{
		_ASSERTEX(FALSE && "Hooks are already initialized and blocked");
		return false;
	}
#endif

	if (!gpHookCS)
	{
		gpHookCS = new MSection;
	}

	//if (!gpcsHooksRootPtr)
	//{
	//	gpcsHooksRootPtr = (LPCRITICAL_SECTION)calloc(1,sizeof(*gpcsHooksRootPtr));
	//	Initialize Critical Section(gpcsHooksRootPtr);
	//}


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
			DWORD NameCRC = CalculateNameCRC32(apHooks[i].Name);

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
				if (gpHooks[j].NameCRC == NameCRC
					&& strcmp(gpHooks[j].Name, apHooks[i].Name) == 0
					&& wcscmp(gpHooks[j].DllName, apHooks[i].DllName) == 0)
				{
					// Не должно быть такого - функции должны только добавляться
					_ASSERTEX(lstrcmpiA(gpHooks[j].Name, apHooks[i].Name) && lstrcmpiW(gpHooks[j].DllName, apHooks[i].DllName));
					gpHooks[j].NewAddress = apHooks[i].NewAddress;
					if (j >= gnHookedFuncs)
						gnHookedFuncs = j+1;
					skip = true;
					break;
				}

				j++;
			}

			if (skip) continue;


			if ((j+1) >= MAX_HOOKED_PROCS)
			{
				// Превышено допустимое количество
				_ASSERTE((j+1) < MAX_HOOKED_PROCS);
				continue; // может какие другие хуки удастся обновить, а не добавить
			}

			gpHooks[j].Name = apHooks[i].Name;
			gpHooks[j].NameOrdinal = apHooks[i].NameOrdinal;
			gpHooks[j].DllName = apHooks[i].DllName;
			gpHooks[j].NewAddress = apHooks[i].NewAddress;
			gpHooks[j].NameCRC = NameCRC;
			_ASSERTEX(j >= gnHookedFuncs);
			gnHookedFuncs = j+1;
			gpHooks[j+1].Name = NULL; // на всякий
			gpHooks[j+1].NewAddress = NULL; // на всякий
		}
	}

	// Для добавленных в gpHooks функций определить "оригинальный" адрес экспорта
	for (i = 0; gpHooks[i].NewAddress; i++)
	{
		if (gpHooks[i].DllNameA[0] == 0)
		{
			int nLen = WideCharToMultiByte(CP_ACP, 0, gpHooks[i].DllName, -1, gpHooks[i].DllNameA, (int)countof(gpHooks[i].DllNameA), 0,0);
			if (nLen > 0) CharLowerBuffA(gpHooks[i].DllNameA, nLen);
		}

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
				const char* ExportName = gpHooks[i].NameOrdinal ? ((const char*)gpHooks[i].NameOrdinal) : gpHooks[i].Name;
				gpHooks[i].OldAddress = (void*)GetProcAddress(mod, ExportName);


				// WinXP does not have many hooked functions, will not show dozens of asserts
				#ifdef _DEBUG
				if (gpHooks[i].OldAddress == NULL)
				{
					static int isWin7 = 0;
					if (isWin7 == 0)
					{
						OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7)};
						DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
						BOOL isGrEq = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
						isWin7 = isGrEq ? 1 : -1;
					}

					_ASSERTE((isWin7 == -1) || (gpHooks[i].OldAddress != NULL));
				}
				#endif

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
		if (IsWin7())
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

#if 0
void AddHooksNode(HookItemNode* pRoot, HookItem* p, HookItemNode*& ppNext)
{
	int iCmp = strcmp(p->Name, pRoot->Name);
	_ASSERTEX(iCmp!=0); // All function names must be unique!

	if (iCmp < 0)
	{
		pRoot->nLeftCount++;
		if (!pRoot->pLeft)
		{
			ppNext->p = p;
			ppNext->Name = p->Name;
			pRoot->pLeft = ppNext++;
			return;
		}
		AddHooksNode(pRoot->pLeft, p, ppNext);
	}
	else
	{
		pRoot->nRightCount++;
		if (!pRoot->pRight)
		{
			ppNext->p = p;
			ppNext->Name = p->Name;
			pRoot->pRight = ppNext++;
			return;
		}
		AddHooksNode(pRoot->pRight, p, ppNext);
	}
}
#endif

#if 0
void BuildTree(HookItemNode*& pRoot, HookItem** pSorted, size_t nCount, HookItemNode*& ppNext)
{
	size_t n = nCount>>1;

	// Init root
	pRoot = ppNext++;
	pRoot->p = pSorted[n];
	pRoot->Name = pSorted[n]->Name;

	if (n > 0)
	{
		BuildTree(pRoot->pLeft, pSorted, n, ppNext);
		#ifdef _DEBUG
		_ASSERTEX(pRoot->pLeft!=NULL);
		pRoot->nLeftCount = 1 + pRoot->pLeft->nLeftCount + pRoot->pLeft->nRightCount;
		#endif
	}

	if ((n + 1) < nCount)
	{
		BuildTree(pRoot->pRight, pSorted+n+1, nCount-n-1, ppNext);
		#ifdef _DEBUG
		_ASSERTEX(pRoot->pRight!=NULL);
		pRoot->nRightCount = 1 + pRoot->pRight->nLeftCount + pRoot->pRight->nRightCount;
		#endif
	}
}
#endif

//void BuildTreePtr(HookItemNodePtr*& pRoot, HookItem** pSorted, size_t nCount, HookItemNodePtr*& ppNext)
//{
//	size_t n = nCount>>1;
//
//	// Init root
//	pRoot = ppNext++;
//	pRoot->p = pSorted[n];
//	pRoot->Address = pSorted[n]->OldAddress;
//
//	if (n > 0)
//	{
//		BuildTreePtr(pRoot->pLeft, pSorted, n, ppNext);
//		#ifdef _DEBUG
//		_ASSERTEX(pRoot->pLeft!=NULL);
//		pRoot->nLeftCount = 1 + pRoot->pLeft->nLeftCount + pRoot->pLeft->nRightCount;
//		#endif
//	}
//	else
//	{
//		pRoot->pLeft = NULL;
//		#ifdef _DEBUG
//		pRoot->nLeftCount = 0;
//		#endif
//	}
//
//	if ((n + 1) < nCount)
//	{
//		BuildTreePtr(pRoot->pRight, pSorted+n+1, nCount-n-1, ppNext);
//		#ifdef _DEBUG
//		_ASSERTEX(pRoot->pRight!=NULL);
//		pRoot->nRightCount = 1 + pRoot->pRight->nLeftCount + pRoot->pRight->nRightCount;
//		#endif
//	}
//	else
//	{
//		pRoot->pRight = NULL;
//		#ifdef _DEBUG
//		pRoot->nRightCount = 0;
//		#endif
//	}
//}

//void BuildTreeNew(HookItemNodePtr*& pRoot, HookItem** pSorted, size_t nCount, HookItemNodePtr*& ppNext)
//{
//	size_t n = nCount>>1;
//
//	// Init root
//	pRoot = ppNext++;
//	pRoot->p = pSorted[n];
//	pRoot->Address = pSorted[n]->NewAddress;
//
//	if (n > 0)
//	{
//		BuildTreeNew(pRoot->pLeft, pSorted, n, ppNext);
//		#ifdef _DEBUG
//		_ASSERTEX(pRoot->pLeft!=NULL);
//		pRoot->nLeftCount = 1 + pRoot->pLeft->nLeftCount + pRoot->pLeft->nRightCount;
//		#endif
//	}
//	else
//	{
//		pRoot->pLeft = NULL;
//		#ifdef _DEBUG
//		pRoot->nLeftCount = 0;
//		#endif
//	}
//
//	if ((n + 1) < nCount)
//	{
//		BuildTreeNew(pRoot->pRight, pSorted+n+1, nCount-n-1, ppNext);
//		#ifdef _DEBUG
//		_ASSERTEX(pRoot->pRight!=NULL);
//		pRoot->nRightCount = 1 + pRoot->pRight->nLeftCount + pRoot->pRight->nRightCount;
//		#endif
//	}
//	else
//	{
//		pRoot->pRight = NULL;
//		#ifdef _DEBUG
//		pRoot->nRightCount = 0;
//		#endif
//	}
//}

#if 0
HookItemNode* FindFunctionNode(HookItemNode* pRoot, const char* pszFuncName)
{
	if (!pRoot)
		return NULL;

	int nCmp = strcmp(pszFuncName, pRoot->Name);
	if (!nCmp)
		return pRoot;

	// BinTree is sorted

	HookItemNode* pc;
	if (nCmp < 0)
	{
		pc = FindFunctionNode(pRoot->pLeft, pszFuncName);
		//#ifdef _DEBUG
		//if (!pc)
		//	_ASSERTEX(FindFunctionNode(pRoot->pRight, pszFuncName)==NULL);
		//#endif
	}
	else
	{
		pc = FindFunctionNode(pRoot->pRight, pszFuncName);
		//#ifdef _DEBUG
		//if (!pc)
		//	_ASSERTEX(FindFunctionNode(pRoot->pLeft, pszFuncName)==NULL);
		//#endif
	}

	return pc;
}
#endif

//HookItemNodePtr* FindFunctionNodePtr(HookItemNodePtr* pRoot, const void* ptrFunc)
//{
//	if (!pRoot)
//		return NULL;
//
//	if (ptrFunc == pRoot->Address)
//		return pRoot;
//
//	// BinTree is sorted
//
//	HookItemNodePtr* pc;
//	if (ptrFunc < pRoot->Address)
//	{
//		pc = FindFunctionNodePtr(pRoot->pLeft, ptrFunc);
//		#ifdef _DEBUG
//		if (!pc)
//			_ASSERTEX(FindFunctionNodePtr(pRoot->pRight, ptrFunc)==NULL);
//		#endif
//	}
//	else
//	{
//		pc = FindFunctionNodePtr(pRoot->pRight, ptrFunc);
//		#ifdef _DEBUG
//		if (!pc)
//			_ASSERTEX(FindFunctionNodePtr(pRoot->pLeft, ptrFunc)==NULL);
//		#endif
//	}
//
//	return pc;
//}

HookItem* FindFunction(const char* pszFuncName)
{
	DWORD NameCRC = CalculateNameCRC32(pszFuncName);

	for (HookItem* p = gpHooks; p->NewAddress; ++p)
	{
		if (p->NameCRC == NameCRC)
		{
			if (strcmp(p->Name, pszFuncName) == 0)
				return p;
		}
	}

	//HookItemNode* pc = FindFunctionNode(gpHooksRoot, pszFuncName);
	//if (pc)
	//	return pc->p;

	return NULL;
}

//HookItem* FindFunctionPtr(HookItemNodePtr *pRoot, const void* ptrFunction)
//{
//	HookItemNodePtr* pc = FindFunctionNodePtr(pRoot, ptrFunction);
//	if (pc)
//		return pc->p;
//	return NULL;
//}

//// Unfortunately, tree must be rebuilded when new modules are loaded
//// (e.g. "shell32.dll", when it is not statically linked to exe)
//void InitHooksSortAddress()
//{
//	if (!gpHooks)
//	{
//		_ASSERTEX(gpHooks!=NULL);
//		return;
//	}
//	_ASSERTEX(gpHooks && gpHooks->Name);
//
//	HLOG0("InitHooksSortAddress",gnHookedFuncs);
//
//	// *** !!! ***
//	Enter Critical Section(gpcsHooksRootPtr);
//
//	// Sorted by address vector
//	HookItem** pSort = (HookItem**)malloc(gnHookedFuncs*sizeof(*pSort));
//	if (!pSort)
//	{
//		Leave Critical Section(gpcsHooksRootPtr);
//		_ASSERTEX(pSort!=NULL && "Memory allocation failed");
//		return;
//	}
//	size_t iMax = 0;
//	for (size_t i = 0; i < gnHookedFuncs; i++)
//	{
//		if (gpHooks[i].OldAddress)
//			pSort[iMax++] = (gpHooks+i);
//	}
//	if (iMax) iMax--;
//	// Go sorting
//	for (size_t i = 0; i < iMax; i++)
//	{
//		size_t m = i;
//		const void* ptrM = pSort[i]->OldAddress;
//		for (size_t j = i+1; j <= iMax; j++)
//		{
//			_ASSERTEX(pSort[j]->OldAddress != ptrM && pSort[j]->OldAddress);
//			if (pSort[j]->OldAddress < ptrM)
//			{
//				m = j; ptrM = pSort[j]->OldAddress;
//			}
//		}
//		if (m != i)
//		{
//			HookItem* p = pSort[i];
//			pSort[i] = pSort[m];
//			pSort[m] = p;
//		}
//	}
//
//	if (!gpHooksTreePtr)
//	{
//		gpHooksTreePtr = (HookItemNodePtr*)calloc(gnHookedFuncs,sizeof(HookItemNodePtr));
//		if (!gpHooksTreePtr)
//		{
//			Leave Critical Section(gpcsHooksRootPtr);
//			return;
//		}
//	}
//
//	// Go to building
//	HookItemNodePtr *ppNext = gpHooksTreePtr;
//	BuildTreePtr(gpHooksRootPtr, pSort, iMax, ppNext);
//
//	free(pSort);
//
//#ifdef _DEBUG
//	// Validate tree
//	_ASSERTEX(gpHooksRoot->nLeftCount>0 && gpHooksRoot->nRightCount>0);
//	_ASSERTEX((gpHooksRoot->nLeftCount<gpHooksRoot->nRightCount) ? ((gpHooksRoot->nRightCount-gpHooksRoot->nLeftCount)<=2) : ((gpHooksRoot->nLeftCount-gpHooksRoot->nRightCount)<=2));
//	_ASSERTEX(FindFunction("Not Existed")==NULL);
//	for (size_t i = 0; i < gnHookedFuncs; i++)
//	{
//		HookItem* pFind = FindFunction(gpHooks[i].Name);
//		_ASSERTEX(pFind == (gpHooks+i));
//	}
//#endif
//
//	HLOGEND();
//
//	Leave Critical Section(gpcsHooksRootPtr);
//}
//
//void InitHooksSortNewAddress()
//{
//	if (!gpHooks)
//	{
//		_ASSERTEX(gpHooks!=NULL);
//		return;
//	}
//	_ASSERTEX(gpHooks && gpHooks->Name);
//
//	HLOG0("InitHooksSortNewAddress",gnHookedFuncs);
//
//
//	// Sorted by address vector
//	HookItem** pSort = (HookItem**)malloc(gnHookedFuncs*sizeof(*pSort));
//	if (!pSort)
//	{
//		_ASSERTEX(pSort!=NULL && "Memory allocation failed");
//		return;
//	}
//	
//	for (size_t i = 0; i < gnHookedFuncs; i++)
//	{
//		pSort[i] = (gpHooks+i);
//	}
//	size_t iMax = gnHookedFuncs - 1;
//	// Go sorting
//	for (size_t i = 0; i < iMax; i++)
//	{
//		size_t m = i;
//		const void* ptrM = pSort[i]->NewAddress;
//		for (size_t j = i+1; j < gnHookedFuncs; j++)
//		{
//			_ASSERTEX(pSort[j]->NewAddress != ptrM && pSort[j]->NewAddress);
//			if (pSort[j]->NewAddress < ptrM)
//			{
//				m = j; ptrM = pSort[j]->NewAddress;
//			}
//		}
//		if (m != i)
//		{
//			HookItem* p = pSort[i];
//			pSort[i] = pSort[m];
//			pSort[m] = p;
//		}
//	}
//
//	if (!gpHooksTreeNew)
//	{
//		gpHooksTreeNew = (HookItemNodePtr*)calloc(gnHookedFuncs,sizeof(HookItemNodePtr));
//		if (!gpHooksTreeNew)
//		{
//			return;
//		}
//	}
//
//	// Go to building
//	HookItemNodePtr *ppNext = gpHooksTreeNew;
//	BuildTreeNew(gpHooksRootNew, pSort, iMax, ppNext);
//
//	free(pSort);
//
//#ifdef _DEBUG
//	// Validate tree
//	_ASSERTEX(gpHooksRoot->nLeftCount>0 && gpHooksRoot->nRightCount>0);
//	_ASSERTEX((gpHooksRoot->nLeftCount<gpHooksRoot->nRightCount) ? ((gpHooksRoot->nRightCount-gpHooksRoot->nLeftCount)<=2) : ((gpHooksRoot->nLeftCount-gpHooksRoot->nRightCount)<=2));
//	_ASSERTEX(FindFunction("Not Existed")==NULL);
//	for (size_t i = 0; i < gnHookedFuncs; i++)
//	{
//		HookItem* pFind = FindFunction(gpHooks[i].Name);
//		_ASSERTEX(pFind == (gpHooks+i));
//	}
//#endif
//
//	HLOGEND();
//}

#if 0
void __stdcall InitHooksSort()
{
	if (!gpHooks)
	{
		_ASSERTEX(gpHooks!=NULL);
		return;
	}
	if (gbHooksSorted)
	{
		_ASSERTEX(FALSE && "Hooks are already sorted");
		return;
	}
	gbHooksSorted = true;
	_ASSERTEX(gpHooks && gpHooks->Name);

	HLOG0("InitHooksSort",gnHookedFuncs);

#if 1
	// Sorted by name vector
	HookItem** pSort = (HookItem**)malloc(gnHookedFuncs*sizeof(*pSort));
	if (!pSort)
	{
		_ASSERTEX(pSort!=NULL && "Memory allocation failed");
		return;
	}
	for (size_t i = 0; i < gnHookedFuncs; i++)
	{
		pSort[i] = (gpHooks+i);
	}
	// Go sorting
	size_t iMax = (gnHookedFuncs-1);
	for (size_t i = 0; i < iMax; i++)
	{
		size_t m = i;
		LPCSTR pszM = pSort[i]->Name;
		for (size_t j = i+1; j < gnHookedFuncs; j++)
		{
			int iCmp = strcmp(pSort[j]->Name, pszM);
			_ASSERTEX(iCmp!=0);
			if (iCmp < 0)
			{
				m = j; pszM = pSort[j]->Name;
			}
		}
		if (m != i)
		{
			HookItem* p = pSort[i];
			pSort[i] = pSort[m];
			pSort[m] = p;
		}
	}

	gpHooksTree = (HookItemNode*)calloc(gnHookedFuncs,sizeof(HookItemNode));
	if (!gpHooksTree)
		return;

	// Go to building
	HookItemNode *ppNext = gpHooksTree;
	BuildTree(gpHooksRoot, pSort, gnHookedFuncs, ppNext);

	free(pSort);

#else

	gpHooksTree = (HookItemNode*)calloc(MAX_HOOKED_PROCS,sizeof(HookItemNode));

	// Init root
	gpHooksRoot = gpHooksTree;
	gpHooksRoot->p = gpHooks;
	gpHooksRoot->Name = gpHooks->Name;

	HookItemNode *ppNext = gpHooksTree+1;

	// Go to building
	for (size_t i = 1; i < MAX_HOOKED_PROCS; ++i)
	{
		if (!gpHooks[i].Name)
			break;
		AddHooksNode(gpHooksRoot, gpHooks+i, ppNext);
	}

#endif

#ifdef _DEBUG
	// Validate tree
	_ASSERTEX(gpHooksRoot->nLeftCount>0 && gpHooksRoot->nRightCount>0);
	_ASSERTEX((gpHooksRoot->nLeftCount<gpHooksRoot->nRightCount) ? ((gpHooksRoot->nRightCount-gpHooksRoot->nLeftCount)<=2) : ((gpHooksRoot->nLeftCount-gpHooksRoot->nRightCount)<=2));
	_ASSERTEX(FindFunction("Not Existed")==NULL);
	for (size_t i = 0; i < gnHookedFuncs; i++)
	{
		HookItem* pFind = FindFunction(gpHooks[i].Name);
		_ASSERTEX(pFind == (gpHooks+i));
	}
#endif

	HLOGEND();

	//// Tree with our NewAddress
	//InitHooksSortNewAddress();

	//// First call to address tree. But it may be rebuilded...
	//InitHooksSortAddress();
}
#endif

void ShutdownHooks()
{
	HLOG1("ShutdownHooks.UnsetAllHooks",0);
	UnsetAllHooks();
	HLOGEND1();

	//// Завершить работу с реестром
	//DoneHooksReg();

	// Уменьшение счетчиков загрузок (а надо ли?)
	HLOG1_("ShutdownHooks.FreeLibrary",1);
	for (size_t s = 0; s < countof(ghSysDll); s++)
	{
		if (ghSysDll[s] && *ghSysDll[s])
		{
			FreeLibrary(*ghSysDll[s]);
			*ghSysDll[s] = NULL;
		}
	}
	HLOGEND1();

	if (gpHookCS)
	{
		MSection *p = gpHookCS;
		gpHookCS = NULL;
		delete p;
	}

	//if (gpcsHooksRootPtr)
	//{
	//	Delete Critical Section(gpcsHooksRootPtr);
	//	SafeFree(gpcsHooksRootPtr);
	//}

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
			if (!strcmp(gpHooks[i].Name, ProcName) && !lstrcmpW(gpHooks[i].DllName,DllName))
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
	#ifdef _DEBUG
	DWORD nCurTID = GetCurrentThreadId();
	#endif

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
							&& !strcmp(gKernelFuncs[j].Name, pszExpName))
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
			GetOsVersionInformational(&osv);
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

bool SetHookPrep(LPCWSTR asModule, HMODULE Module, IMAGE_NT_HEADERS* nt_header, BOOL abForceHooks, bool bExecutable, IMAGE_IMPORT_DESCRIPTOR* Import, size_t ImportCount, bool (&bFnNeedHook)[MAX_HOOKED_PROCS], HkModuleInfo* p);
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

	HLOG0("SetHook.Init",(DWORD)Module);

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
		HLOGEND();
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

	HLOG("SetHook.Lock",(DWORD)Module);
	MSectionLock CS;
	if (!gpHookCS->isLockedExclusive() && !LockHooks(Module, L"install", &CS))
		return false;
	HLOGEND();

	if (!p)
	{
		HLOG("SetHook.Add",(DWORD)Module);
		p = AddHookedModule(Module, asModule);
		HLOGEND();
		if (!p)
			return false;
	}

	HLOG("SetHook.Prepare",(DWORD)Module);
	TODO("!!! Сохранять ORDINAL процедур !!!");
	bool res = false, bHooked = false;
	//INT_PTR i;
	INT_PTR nCount = Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
	bool bFnNeedHook[MAX_HOOKED_PROCS] = {};
	// в отдельной функции, т.к. __try
	res = SetHookPrep(asModule, Module, nt_header, abForceHooks, bExecutable, Import, nCount, bFnNeedHook, p);
	HLOGEND();

	HLOG("SetHook.Change",(DWORD)Module);
	// в отдельной функции, т.к. __try
	bHooked = SetHookChange(asModule, Module, abForceHooks, bFnNeedHook, p);
	HLOGEND();

	#ifdef _DEBUG
	if (bHooked)
	{
		HLOG("SetHook.FindModuleFileName",(DWORD)Module);
		wchar_t* szDbg = (wchar_t*)calloc(MAX_PATH*3, 2);
		wchar_t* szModPath = (wchar_t*)calloc(MAX_PATH*2, 2);
		FindModuleFileName(Module, szModPath, MAX_PATH*2);
		_wcscpy_c(szDbg, MAX_PATH*3, L"  ## Hooks was set by conemu: ");
		_wcscat_c(szDbg, MAX_PATH*3, szModPath);
		_wcscat_c(szDbg, MAX_PATH*3, L"\n");
		DebugString(szDbg);
		free(szDbg);
		free(szModPath);
		HLOGEND();
	}
	#endif

	HLOG("SetHook.Unlock",(DWORD)Module);
	//ReleaseMutex(ghHookMutex);
	CS.Unlock();
	HLOGEND();

	// Плагин ConEmu может выполнить дополнительные действия
	if (gfOnLibraryLoaded)
	{
		HLOG("SetHook.gfOnLibraryLoaded",(DWORD)Module);
		gfOnLibraryLoaded(Module);
		HLOGEND();
	}

	return res;
}

bool isBadModulePtr(const void *lp, UINT_PTR ucb, HMODULE Module, const IMAGE_NT_HEADERS* nt_header)
{
	bool bTestValid = (((LPBYTE)lp) >= ((LPBYTE)Module))
		&& ((((LPBYTE)lp) + ucb) <= (((LPBYTE)Module) + nt_header->OptionalHeader.SizeOfImage));

#ifdef USE_ONLY_INT_CHECK_PTR
	bool bApiValid = bTestValid;
#else
	bool bApiValid = !IsBadReadPtr(lp, ucb);

	#ifdef _DEBUG
	static bool bFirstAssert = false;
	if (bTestValid != bApiValid)
	{
		if (!bFirstAssert)
		{
			bFirstAssert = true;
			_ASSERTE(bTestValid != bApiValid);
		}
	}
	#endif
#endif

	return !bApiValid;
}

bool isBadModuleStringA(LPCSTR lpsz, UINT_PTR ucchMax, HMODULE Module, IMAGE_NT_HEADERS* nt_header)
{
	bool bTestStrValid = (((LPBYTE)lpsz) >= ((LPBYTE)Module))
		&& ((((LPBYTE)lpsz) + ucchMax) <= (((LPBYTE)Module) + nt_header->OptionalHeader.SizeOfImage));

#ifdef USE_ONLY_INT_CHECK_PTR
	bool bApiStrValid = bTestStrValid;
#else
	bool bApiStrValid = !IsBadStringPtrA(lpsz, ucchMax);

	#ifdef _DEBUG
	static bool bFirstAssert = false;
	if (bTestStrValid != bApiStrValid)
	{
		if (!bFirstAssert)
		{
			bFirstAssert = true;
			_ASSERTE(bTestStrValid != bApiStrValid);
		}
	}
	#endif
#endif

	return !bApiStrValid;
}

bool SetHookPrep(LPCWSTR asModule, HMODULE Module, IMAGE_NT_HEADERS* nt_header, BOOL abForceHooks, bool bExecutable, IMAGE_IMPORT_DESCRIPTOR* Import, size_t ImportCount, bool (&bFnNeedHook)[MAX_HOOKED_PROCS], HkModuleInfo* p)
{
	bool res = false;
	size_t i;

	//api-ms-win-core-libraryloader-l1-1-1.dll
	//api-ms-win-core-console-l1-1-0.dll
	//...
	char szCore[18];
	const char szCorePrefix[] = "api-ms-win-core-"; // MUST BE LOWER CASE!
	const int nCorePrefLen = lstrlenA(szCorePrefix);
	_ASSERTE((nCorePrefLen+1)<countof(szCore));
	bool lbIsCoreModule = false;
	char mod_name[MAX_PATH];

	//_ASSERTEX(lstrcmpi(asModule, L"dsound.dll"));

	SAFETRY
	{
		HLOG0("SetHookPrep.Begin",ImportCount);

		//if (!gpHooksRootPtr)
		//{
		//	InitHooksSortAddress();
		//}

		//_ASSERTE(Size == (nCount * sizeof(IMAGE_IMPORT_DESCRIPTOR))); -- ровно быть не обязано
		for (i = 0; i < ImportCount; i++)
		{
			if (Import[i].Name == 0)
				break;

			HLOG0("SetHookPrep.Import",i);

			HLOG1("SetHookPrep.CheckModuleName",i);
			//DebugString( ToTchar( (char*)Module + Import[i].Name ) );
			char* mod_name_ptr = (char*)Module + Import[i].Name;
			DWORD_PTR rvaINT = Import[i].OriginalFirstThunk;
			DWORD_PTR rvaIAT = Import[i].FirstThunk; //-V101
			lstrcpynA(mod_name, mod_name_ptr, countof(mod_name));
			CharLowerBuffA(mod_name, lstrlenA(mod_name)); // MUST BE LOWER CASE!
			lstrcpynA(szCore, mod_name, nCorePrefLen+1);
			lbIsCoreModule = (strcmp(szCore, szCorePrefix) == 0);

			bool bHookExists = false;
			for (size_t j = 0; gpHooks[j].Name; j++)
			{
				if ((strcmp(mod_name, gpHooks[j].DllNameA) != 0)
					&& !(lbIsCoreModule && (gpHooks[j].DllName == kernel32)))
					// Имя модуля не соответствует
					continue;
				bHookExists = true;
				break;
			}
			// Этот модуль вообще не хукается
			if (!bHookExists)
			{
				HLOGEND1();
				HLOGEND();
				continue;
			}

			if (rvaINT == 0)      // No Characteristics field?
			{
				// Yes! Gotta have a non-zero FirstThunk field then.
				rvaINT = rvaIAT;

				if (rvaINT == 0)       // No FirstThunk field?  Ooops!!!
				{
					_ASSERTE(rvaINT!=0);
					HLOGEND1();
					HLOGEND();
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
				HLOGEND1();
				HLOGEND();
				continue;
			}
			HLOGEND1();

			// ***** >>>>>> go

			HLOG1_("SetHookPrep.ImportThunksSteps",i);
			size_t f, s;
			for (s = 0; s <= 1; s++)
			{
				if (s)
				{
					thunk = (IMAGE_THUNK_DATA*)GetPtrFromRVA(rvaIAT, nt_header, (PBYTE)Module);
					thunkO = (IMAGE_THUNK_DATA*)GetPtrFromRVA(rvaINT, nt_header, (PBYTE)Module);
				}

				for (f = 0;; thunk++, thunkO++, f++)
				{
					//111127 - ..\GIT\lib\perl5\site_perl\5.8.8\msys\auto\SVN\_Core\_Core.dll
					//         похоже, в этой длл кривая таблица импортов
					#ifndef USE_SEH
					HLOG("SetHookPrep.lbBadThunk",f);
					bool lbBadThunk = isBadModulePtr(thunk, sizeof(*thunk), Module, nt_header);
					if (lbBadThunk)
					{
						_ASSERTEX(!lbBadThunk);
						break;
					}
					#endif

					if (!thunk->u1.Function)
						break;

					#ifndef USE_SEH
					HLOG("SetHookPrep.lbBadThunkO",f);
					bool lbBadThunkO = isBadModulePtr(thunkO, sizeof(*thunkO), Module, nt_header);
					if (lbBadThunkO)
					{
						_ASSERTEX(!lbBadThunkO);
						break;
					}
					#endif

					const char* pszFuncName = NULL;
					//ULONGLONG ordinalO = -1;


					// Получили адрес функции, и (на втором шаге) имя функции
					// Теперь нужно подобрать (если есть) адрес перехвата
					HookItem* ph = gpHooks;
					INT_PTR jj = -1;

					if (!s)
					{
						HLOG1("SetHookPrep.ImportThunks0",s);

						//HLOG2("SetHookPrep.FuncTreeNew",f);
						//ph = FindFunctionPtr(gpHooksRootNew, (void*)thunk->u1.Function);
						//HLOGEND2();
						//if (ph)
						//{
						//	// Already hooked, this is our function address
						//	HLOGEND1();
						//	continue;
						//}


						//HLOG2_("SetHookPrep.FuncTreeOld",f);
						//ph = FindFunctionPtr(gpHooksRootPtr, (void*)thunk->u1.Function);
						//HLOGEND2();
						//if (!ph)
						//{
						//	// This address (Old) does not exists in our tables
						//	HLOGEND1();
						//	continue;
						//}

						//jj = (ph - gpHooks);

						for (size_t j = 0; ph->Name; ++j, ++ph)
						{
							_ASSERTEX(j<gnHookedFuncs && gnHookedFuncs<=MAX_HOOKED_PROCS);

							// Если не удалось определить оригинальный адрес процедуры (kernel32/WriteConsoleOutputW, и т.п.)
							if (ph->OldAddress == NULL)
							{
								continue;
							}

							// Если адрес импорта в модуле уже совпадает с адресом одной из наших функций
							if (ph->NewAddress == (void*)thunk->u1.Function)
							{
								res = true; // это уже захучено
								break;
							}

							#ifdef _DEBUG
							//const void* ptrNewAddress = ph->NewAddress;
							//const void* ptrOldAddress = (void*)thunk->u1.Function;
							#endif

							// Проверяем адрес перехватываемой функции
							if ((void*)thunk->u1.Function == ph->OldAddress)
							{
								jj = j;
								break; // OK, Hook it!
							}
						} // for (size_t j = 0; ph->Name; ++j, ++ph), (s==0)

						HLOGEND1();
					}
					else
					{
						HLOG1("SetHookPrep.ImportThunks1",s);

						if (!abForceHooks)
						{
							//_ASSERTEX(abForceHooks);
							//HLOGEND1();
							HLOG2("!!!Function skipped of (!abForceHooks)",f);
							continue; // запрещен перехват, если текущий адрес в модуле НЕ совпадает с оригинальным экспортом!
						}

						// искать имя функции
						if ((thunk->u1.Function != thunkO->u1.Function)
							&& !IMAGE_SNAP_BY_ORDINAL(thunkO->u1.Ordinal))
						{
							pOrdinalNameO = (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA(thunkO->u1.AddressOfData, nt_header, (PBYTE)Module);

							#ifdef USE_SEH
								pszFuncName = (LPCSTR)pOrdinalNameO->Name;
							#else
								HLOG("SetHookPrep.pOrdinalNameO",f);
								//WARNING!!! Множественные вызовы IsBad???Ptr могут глючить и тормозить
								bool lbValidPtr = !isBadModulePtr(pOrdinalNameO, sizeof(IMAGE_IMPORT_BY_NAME), Module, nt_header);
								_ASSERTE(lbValidPtr);

								if (lbValidPtr)
								{
									lbValidPtr = !isBadModuleStringA((LPCSTR)pOrdinalNameO->Name, 10, Module, nt_header);
									_ASSERTE(lbValidPtr);

									if (lbValidPtr)
										pszFuncName = (LPCSTR)pOrdinalNameO->Name;
								}
							#endif
						}

						if (!pszFuncName || !*pszFuncName)
						{
							continue; // This import does not have "Function name"
						}

						HLOG2("SetHookPrep.FindFunction",f);
						ph = FindFunction(pszFuncName);
						HLOGEND2();
						if (!ph)
						{
							HLOGEND1();
							continue;
						}

						// Имя модуля
						HLOG2_("SetHookPrep.Module",f);
						if ((strcmp(mod_name, ph->DllNameA) != 0)
							&& !(lbIsCoreModule && (ph->DllName == kernel32)))
						{
							HLOGEND2();
							HLOGEND1();
							// Имя модуля не соответствует
							continue; // И дубли имен функций не допускаются. Пропускаем
						}
						HLOGEND2();

						jj = (ph - gpHooks);

						HLOGEND1();
					}


					if (jj >= 0)
					{
						HLOG1("SetHookPrep.WorkExport",f);

						if (bExecutable && !ph->ExeOldAddress)
						{
							// OldAddress уже может отличаться от оригинального экспорта библиотеки
							//// Это происходит например с PeekConsoleIntputW при наличии плагина Anamorphosis
							// Про Anamorphosis несколько устарело. При включенном "Inject ConEmuHk"
							// хуки ставятся сразу при запуске процесса.
							// Но, теоретически, кто-то может успеть раньше, или флажок "Inject" выключен.

							// Также это может быть в новой архитектуре Win7 ("api-ms-win-core-..." и др.)

							ph->ExeOldAddress = (void*)thunk->u1.Function;
						}

						// When we get here - jj matches pszFuncName or FuncPtr
						if (p->Addresses[jj].ppAdr != NULL)
						{
							HLOGEND1();
							continue; // уже обработали, следующий импорт
						}

						//#ifdef _DEBUG
						//// Это НЕ ORDINAL, это Hint!!!
						//if (ph->nOrdinal == 0 && ordinalO != (ULONGLONG)-1)
						//	ph->nOrdinal = (DWORD)ordinalO;
						//#endif


						_ASSERTE(sizeof(thunk->u1.Function)==sizeof(DWORD_PTR));

						if (thunk->u1.Function == (DWORD_PTR)ph->NewAddress)
						{
							// оказалось захучено в другой нити? такого быть не должно, блокируется секцией
							// Но может быть захучено в прошлый раз, если не все модули были загружены при старте
							_ASSERTE(thunk->u1.Function != (DWORD_PTR)ph->NewAddress);
						}
						else
						{
							bFnNeedHook[jj] = true;
							p->Addresses[jj].ppAdr = &thunk->u1.Function;
							#ifdef _DEBUG
							p->Addresses[jj].ppAdrCopy1 = (DWORD_PTR)p->Addresses[jj].ppAdr;
							p->Addresses[jj].ppAdrCopy2 = (DWORD_PTR)*p->Addresses[jj].ppAdr;
							p->Addresses[jj].pModulePtr = (DWORD_PTR)p->hModule;
							IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((char*)p->hModule + ((IMAGE_DOS_HEADER*)p->hModule)->e_lfanew);
							p->Addresses[jj].nModuleSize = nt_header->OptionalHeader.SizeOfImage;
							#endif
							//Для проверки, а то при UnsetHook("cscapi.dll") почему-то возникла ошибка ERROR_INVALID_PARAMETER в VirtualProtect
							_ASSERTEX(p->hModule==Module);
							HLOG2("SetHookPrep.CheckCallbackPtr.1",f);
							_ASSERTEX(CheckCallbackPtr(p->hModule, 1, (FARPROC*)&p->Addresses[jj].ppAdr, TRUE));
							HLOGEND2();
							p->Addresses[jj].pOld = thunk->u1.Function;
							p->Addresses[jj].pOur = (DWORD_PTR)ph->NewAddress;
							#ifdef _DEBUG
							lstrcpynA(p->Addresses[jj].sName, ph->Name, countof(p->Addresses[jj].sName));
							#endif

							_ASSERTEX(p->nAdrUsed < countof(p->Addresses));
							p->nAdrUsed++; //информационно
						}

						#ifdef _DEBUG
						if (bExecutable)
							ph->ReplacedInExe = TRUE;
						#endif

						//DebugString( ToTchar( ph->Name ) );
						res = true;
						HLOGEND1();
					} // if (jj >= 0)
					HLOGEND1();

				} // for (f = 0;; thunk++, thunkO++, f++)

			} // for (s = 0; s <= 1; s++)
			HLOGEND1();

			HLOGEND();
		} // for (i = 0; i < nCount; i++)
		HLOGEND();
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

VOID CALLBACK LdrDllNotification(ULONG NotificationReason, const LDR_DLL_NOTIFICATION_DATA* NotificationData, PVOID Context)
{
	DWORD   dwSaveErrCode = GetLastError();
	wchar_t szModule[MAX_PATH*2] = L"";
	HMODULE hModule;
	BOOL    bMainThread = (GetCurrentThreadId() == gnHookMainThreadId);

    const UNICODE_STRING* FullDllName;   //The full path name of the DLL module.
    const UNICODE_STRING* BaseDllName;   //The base file name of the DLL module.

	switch (NotificationReason)
	{
	case LDR_DLL_NOTIFICATION_REASON_LOADED:
		FullDllName = NotificationData->Loaded.FullDllName;
		BaseDllName = NotificationData->Loaded.BaseDllName;
		hModule = (HMODULE)NotificationData->Loaded.DllBase;
		break;
	case LDR_DLL_NOTIFICATION_REASON_UNLOADED:
		FullDllName = NotificationData->Unloaded.FullDllName;
		BaseDllName = NotificationData->Unloaded.BaseDllName;
		hModule = (HMODULE)NotificationData->Unloaded.DllBase;
		break;
	default:
		return;
	}

	if (FullDllName && FullDllName->Buffer)
		memmove(szModule, FullDllName->Buffer, min(sizeof(szModule)-2,FullDllName->Length));
	else if (BaseDllName && BaseDllName->Buffer)
		memmove(szModule, BaseDllName->Buffer, min(sizeof(szModule)-2,BaseDllName->Length));

	#ifdef _DEBUG
	wchar_t szDbgInfo[MAX_PATH*3];
	_wsprintf(szDbgInfo, SKIPLEN(countof(szDbgInfo)) L"ConEmuHk: Ldr(%s) " WIN3264TEST(L"0x%08X",L"0x%08X%08X") L" '%s'\n",
		(NotificationReason==LDR_DLL_NOTIFICATION_REASON_LOADED) ? L"Loaded" : L"Unload",
		WIN3264WSPRINT(hModule),
		szModule);
	DebugString(szDbgInfo);
	#endif

	switch (NotificationReason)
	{
	case LDR_DLL_NOTIFICATION_REASON_LOADED:
		if (PrepareNewModule(hModule, NULL, szModule, TRUE, TRUE))
		{
			HookItem* ph = NULL;;
			GetOriginalAddress((void*)(FARPROC)OnLoadLibraryW, (void*)(FARPROC)LoadLibraryW, FALSE, &ph);
			if (ph && ph->PostCallBack)
			{
				SETARGS1(&hModule,szModule);
				ph->PostCallBack(&args);
			}
		}
		break;
	case LDR_DLL_NOTIFICATION_REASON_UNLOADED:
		UnprepareModule(hModule, szModule, 0);
		UnprepareModule(hModule, szModule, 2);
		break;
	}

	SetLastError(dwSaveErrCode);
}

// Подменить Импортируемые функции во всех модулях процесса, загруженных ДО conemuhk.dll
// *aszExcludedModules - должны указывать на константные значения (program lifetime)
bool __stdcall SetAllHooks(HMODULE ahOurDll, const wchar_t** aszExcludedModules /*= NULL*/, BOOL abForceHooks)
{
	// т.к. SetAllHooks может быть вызван из разных dll - запоминаем однократно
	if (!ghOurModule) ghOurModule = ahOurDll;

	if (!gpHooks)
	{
		HLOG1("SetAllHooks.InitHooks",0);
		InitHooks(NULL);
		if (!gpHooks)
			return false;
		HLOGEND1();
	}

#if 0
	if (!gbHooksSorted)
	{
		HLOG1("InitHooksSort",0);
		InitHooksSort();
		HLOGEND1();
	}
#endif

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
	HMODULE hExecutable = GetModuleHandle(0);
	HANDLE snapshot;

	HLOG0("SetAllHooks.GetMainThreadId",0);
	// Найти ID основной нити
	GetMainThreadId(false);
	_ASSERTE(gnHookMainThreadId!=0);
	HLOGEND();

	wchar_t szInfo[MAX_PATH+2] = {};

	// Если просили хукать только exe-шник
	if (gbHookExecutableOnly)
	{
		GetModuleFileName(NULL, szInfo, countof(szInfo)-2);
		wcscat_c(szInfo, L"\n");
		DebugString(szInfo);
		// Go
		HLOG("SetAllHooks.SetHook(exe)",0);
		SetHook(szInfo, hExecutable, abForceHooks);
		HLOGEND();
	}
	else
	{
		#ifdef _DEBUG
		msprintf(szInfo, countof(szInfo), L"!!! TH32CS_SNAPMODULE, TID=%u, SetAllHooks, hOurModule=" WIN3264TEST(L"0x%08X\n",L"0x%08X%08X\n"), GetCurrentThreadId(), WIN3264WSPRINT(ahOurDll));
		DebugString(szInfo);
		#endif

		HLOG("SetAllHooks.CreateSnap",0);
		// Начались замены во всех загруженных (linked) модулях
		snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
		HLOGEND();

		if (snapshot != INVALID_HANDLE_VALUE)
		{
			MODULEENTRY32 module = {sizeof(MODULEENTRY32)};
			HLOG("SetAllHooks.EnumStart",0);

			HLOG("SetAllHooks.Module32First/Module32Next",0);
			for (BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
			{
				HLOGEND();
				if (module.hModule && !IsModuleExcluded(module.hModule, NULL, module.szModule))
				{
					lstrcpyn(szInfo, module.szModule, countof(szInfo)-2);
					wcscat_c(szInfo, L"\n");
					DebugString(szInfo);
					// Go
					HLOG1("SetAllHooks.SetHook",(DWORD)module.hModule);
					SetHook(module.szModule, module.hModule/*, (module.hModule == hExecutable)*/, abForceHooks);
					HLOGEND1();
				}
				HLOG("SetAllHooks.Module32First/Module32Next",0);
			}
			HLOGEND();

			HLOG("SetAllHooks.CloseSnap",0);
			CloseHandle(snapshot);
			HLOGEND();
		}
	}

	DebugString(L"SetAllHooks finished\n");

	return true;
}


bool UnsetHookInt(HMODULE Module)
{
	bool bUnhooked = false, res = false;
	IMAGE_IMPORT_DESCRIPTOR* Import = 0;
	size_t Size = 0;
	_ASSERTE(Module!=NULL);
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)Module;
	IMAGE_NT_HEADERS* nt_header;

	SAFETRY
	{
		if (dos_header->e_magic == IMAGE_DOS_SIGNATURE /*'ZM'*/)
		{
			nt_header = (IMAGE_NT_HEADERS*)((char*)Module + dos_header->e_lfanew);

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

		size_t i, s, nCount;
		nCount = Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);

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
					if (isBadModulePtr(thunk, sizeof(IMAGE_THUNK_DATA), Module, nt_header))
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
					if (isBadModulePtr(thunkO, sizeof(IMAGE_THUNK_DATA), Module, nt_header))
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
							#ifdef _DEBUG
							bool bTestValid = (((LPBYTE)pOrdinalNameO) >= ((LPBYTE)Module)) && (((LPBYTE)pOrdinalNameO) <= (((LPBYTE)Module) + nt_header->OptionalHeader.SizeOfImage));
							#endif
							//WARNING!!! Множественные вызовы IsBad???Ptr могут глючить и тормозить
							bool lbValidPtr = !isBadModulePtr(pOrdinalNameO, sizeof(IMAGE_IMPORT_BY_NAME), Module, nt_header);
							#ifdef _DEBUG
							static bool bFirstAssert = false;
							if (!lbValidPtr && !bFirstAssert)
							{
								bFirstAssert = true;
								//_ASSERTE(lbValidPtr);
							}
							#endif

							if (lbValidPtr)
							{
								//WARNING!!! Множественные вызовы IsBad???Ptr могут глючить и тормозить
								lbValidPtr = !isBadModuleStringA((LPCSTR)pOrdinalNameO->Name, 10, Module, nt_header);
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
								if (strcmp(pszFuncName, gpHooks[j].Name)!=0)
									continue;
							}

							// OldAddress уже может отличаться от оригинального экспорта библиотеки
							// Это если функцию захукали уже после нас
						}

						#ifdef _DEBUG
						// Может ли такое быть? Модуль был "захукан" без нашего ведома?
						// Наблюдается в Win2k8R2
						static bool bWarned = false;
						if (!bWarned)
						{
							bWarned = true;
							_ASSERTE(FALSE && "Unknown function replacement was found (external hook?)");
						}
						#endif

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
		HLOG1("UnsetHook.Int",0);
		bUnhooked = UnsetHookInt(Module);
		HLOGEND1();
	}
	else
	{
		if (p->Hooked == 1)
		{
			HLOG1("UnsetHook.Var",0);
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
			HLOGEND1();
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
	HMODULE hExecutable = GetModuleHandle(0);

	wchar_t szInfo[MAX_PATH+2] = {};

	if (gbLdrDllNotificationUsed)
	{
		_ASSERTEX(LdrUnregisterDllNotification!=NULL);
		LdrUnregisterDllNotification(gpLdrDllNotificationCookie);
	}

	// Если просили хукать только exe-шник
	if (gbHookExecutableOnly)
	{
		GetModuleFileName(NULL, szInfo, countof(szInfo)-2);
		#ifdef _DEBUG
		wcscat_c(szInfo, L"\n");
		//WARNING!!! OutputDebugString must NOT be used from ConEmuHk::DllMain(DLL_PROCESS_DETACH). See Issue 465
		DebugString(szInfo);
		#endif
		// Go
		UnsetHook(hExecutable);
	}
	else
	{
		#ifdef _DEBUG
		//WARNING!!! OutputDebugString must NOT be used from ConEmuHk::DllMain(DLL_PROCESS_DETACH). See Issue 465
		msprintf(szInfo, countof(szInfo), L"!!! TH32CS_SNAPMODULE, TID=%u, UnsetAllHooks\n", GetCurrentThreadId());
		DebugString(szInfo);
		#endif

		//Warning: TH32CS_SNAPMODULE - может зависать при вызовах из LoadLibrary/FreeLibrary.
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);

		WARNING("Убрать перехват экспортов из Kernel32.dll");

		if (snapshot != INVALID_HANDLE_VALUE)
		{
			MODULEENTRY32 module = {sizeof(module)};

			for (BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
			{
				if (module.hModule && !IsModuleExcluded(module.hModule, NULL, module.szModule))
				{
					lstrcpyn(szInfo, module.szModule, countof(szInfo)-2);
					//WARNING!!! OutputDebugString must NOT be used from ConEmuHk::DllMain(DLL_PROCESS_DETACH). See Issue 465
					#ifdef _DEBUG
					wcscat_c(szInfo, L"\n");
					DebugString(szInfo);
					#endif
					// Go
					UnsetHook(module.hModule/*, (module.hModule == hExecutable)*/);
				}
			}

			CloseHandle(snapshot);
		}
	}

	#ifdef _DEBUG
	DebugStringA("UnsetAllHooks finished\n");
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
			gbLogLibraries = sp->LoadSrvMapping() && sp->GetLogLibraries();
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

bool PrepareNewModule(HMODULE module, LPCSTR asModuleA, LPCWSTR asModuleW, BOOL abNoSnapshoot /*= FALSE*/, BOOL abForceHooks /*= FALSE*/)
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
			gbLogLibraries = sp->LoadSrvMapping(TRUE) && sp->GetLogLibraries();
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

		if (!gbHookExecutableOnly && !IsModuleExcluded(module, asModuleA, asModuleW))
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
	// Если просили хукать только exe-шник
	if (gbHookExecutableOnly)
	{
		return;
	}

#ifdef _DEBUG
	if (gbSkipCheckProcessModules)
	{
		return;
	}
#endif

#ifdef _DEBUG
	char szDbgInfo[100];
	msprintf(szDbgInfo, countof(szDbgInfo), "!!! TH32CS_SNAPMODULE, TID=%u, CheckProcessModules, hFromModule=" WIN3264TEST("0x%08X\n","0x%08X%08X\n"), GetCurrentThreadId(), WIN3264WSPRINT(hFromModule));
	DebugStringA(szDbgInfo);
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

#ifdef _DEBUG
void OnLoadLibraryLog(LPCSTR lpLibraryA, LPCWSTR lpLibraryW)
{
	#if 0
	if ((lpLibraryA && strncmp(lpLibraryA, "advapi32", 8)==0)
		|| (lpLibraryW && wcsncmp(lpLibraryW, L"advapi32", 8)==0))
	{
		extern HANDLE ghDebugSshLibs, ghDebugSshLibsRc;
		if (ghDebugSshLibs)
		{
			SetEvent(ghDebugSshLibs);
			WaitForSingleObject(ghDebugSshLibsRc, 1000);
		}
	}
	#endif
}
#else
#define OnLoadLibraryLog(lpLibraryA,lpLibraryW)
#endif

/* ************** */
HMODULE WINAPI OnLoadLibraryAWork(FARPROC lpfn, HookItem *ph, BOOL bMainThread, const char* lpFileName)
{
	typedef HMODULE(WINAPI* OnLoadLibraryA_t)(const char* lpFileName);
	OnLoadLibraryLog(lpFileName,NULL);
	HMODULE module = ((OnLoadLibraryA_t)lpfn)(lpFileName);
	DWORD dwLoadErrCode = GetLastError();

	if (gbHooksTemporaryDisabled)
		return module;

	// Issue 1079: Almost hangs with PHP
	if (lstrcmpiA(lpFileName, "kernel32.dll") == 0)
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

/* ************** */
HMODULE WINAPI OnLoadLibraryWWork(FARPROC lpfn, HookItem *ph, BOOL bMainThread, const wchar_t* lpFileName)
{
	typedef HMODULE(WINAPI* OnLoadLibraryW_t)(const wchar_t* lpFileName);
	HMODULE module = NULL;

	OnLoadLibraryLog(NULL,lpFileName);

	// Спрятать ExtendedConsole.dll с глаз долой, в сервисную папку "ConEmu"
	if (lpFileName 
		&& ((lstrcmpiW(lpFileName, L"ExtendedConsole.dll") == 0)
			|| lstrcmpiW(lpFileName, L"ExtendedConsole64.dll") == 0))
	{
		CESERVER_CONSOLE_MAPPING_HDR *Info = (CESERVER_CONSOLE_MAPPING_HDR*)calloc(1,sizeof(*Info));
		if (Info && ::LoadSrvMapping(ghConWnd, *Info))
		{
			size_t cchMax = countof(Info->ComSpec.ConEmuBaseDir)+64;
			wchar_t* pszFullPath = (wchar_t*)calloc(cchMax,sizeof(*pszFullPath));
			if (pszFullPath)
			{
				_wcscpy_c(pszFullPath, cchMax, Info->ComSpec.ConEmuBaseDir);
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

	if (gbHooksTemporaryDisabled || gbLdrDllNotificationUsed)
		return module;

	// Issue 1079: Almost hangs with PHP
	if (lstrcmpi(lpFileName, L"kernel32.dll") == 0)
		return module;

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

/* ************** */
HMODULE WINAPI OnLoadLibraryExAWork(FARPROC lpfn, HookItem *ph, BOOL bMainThread, const char* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	typedef HMODULE(WINAPI* OnLoadLibraryExA_t)(const char* lpFileName, HANDLE hFile, DWORD dwFlags);
	OnLoadLibraryLog(lpFileName,NULL);
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

/* ************** */
HMODULE WINAPI OnLoadLibraryExWWork(FARPROC lpfn, HookItem *ph, BOOL bMainThread, const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	typedef HMODULE(WINAPI* OnLoadLibraryExW_t)(const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags);
	OnLoadLibraryLog(NULL,lpFileName);
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
		msprintf(gszLastGetProcAddress, countof(gszLastGetProcAddress), "   OnGetProcAddress(x%08X,%s,%u)",
			(DWORD)hModule, (((DWORD_PTR)lpProcName) <= 0xFFFF) ? "" : lsProcNameCut,
			(((DWORD_PTR)lpProcName) <= 0xFFFF) ? (UINT)(DWORD_PTR)lsProcNameCut : 0);
		#endif
	}
	else if (gbDllStopCalled)
	{
		//-- assert нельзя, т.к. все уже деинициализировано!
		//_ASSERTE(ghHeap!=NULL);
		//-- lpfnRc = NULL; -- уже
	}
	else if (((DWORD_PTR)lpProcName) <= 0xFFFF)
	{
		TODO("!!! Обрабатывать и ORDINAL values !!!");
		#ifdef LOG_ORIGINAL_CALL
		msprintf(gszLastGetProcAddress, countof(gszLastGetProcAddress), "   OnGetProcAddress(x%08X,%u)",
			(DWORD)hModule, (UINT)(DWORD_PTR)lsProcNameCut);
		#endif

		// Ordinal - пока используется только для "ShellExecCmdLine"
		if (gpHooks && gbPrepareDefaultTerminal)
		{
			for (int i = 0; gpHooks[i].Name; i++)
			{
				// The spelling and case of a function name pointed to by lpProcName must be identical
				// to that in the EXPORTS statement of the source DLL's module-definition (.Def) file
				if (gpHooks[i].hDll == hModule
						&& gpHooks[i].NameOrdinal
						&& (gpHooks[i].NameOrdinal == (DWORD)(DWORD_PTR)lpProcName))
				{
					lpfnRc = (FARPROC)gpHooks[i].NewAddress;
					break;
				}
			}
		}
	}
	else
	{
		#ifdef LOG_ORIGINAL_CALL
		msprintf(gszLastGetProcAddress, countof(gszLastGetProcAddress), "   OnGetProcAddress(x%08X,%s)",
			(DWORD)hModule, lsProcNameCut);
		#endif

		if (gpHooks)
		{
			for (int i = 0; gpHooks[i].Name; i++)
			{
				// The spelling and case of a function name pointed to by lpProcName must be identical
				// to that in the EXPORTS statement of the source DLL's module-definition (.Def) file
				if (gpHooks[i].hDll == hModule
						&& strcmp(gpHooks[i].Name, lpProcName) == 0)
				{
					lpfnRc = (FARPROC)gpHooks[i].NewAddress;
					break;
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
void UnprepareModule(HMODULE hModule, LPCWSTR pszModule, int iStep)
{
	BOOL lbResource = LDR_IS_RESOURCE(hModule);
	// lbResource получается TRUE например при вызовах из version.dll
	wchar_t szModule[MAX_PATH*2]; szModule[0] = 0;

	if ((iStep == 0) && gbLogLibraries && !gbDllStopCalled)
	{
		CShellProc* sp = new CShellProc();
		if (sp->LoadSrvMapping())
		{
			CESERVER_REQ* pIn = NULL;
			if (pszModule && *pszModule)
			{
				lstrcpyn(szModule, pszModule, countof(szModule));
			}
			else
			{
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
			}

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



	// Далее только если !LDR_IS_RESOURCE
	if ((iStep > 0) && !lbResource && !gbDllStopCalled)
	{
		// Попробуем определить, действительно ли модуль выгружен, или только счетчик уменьшился
		// iStep == 2 comes from LdrDllNotification(Unload)
		// Похоже, что если библиотека была реально выгружена, то FreeLibrary выставляет SetLastError(ERROR_GEN_FAILURE)
		// Актуально только для Win2k/XP так что не будем на это полагаться
		BOOL lbModulePost = (iStep == 2) ? FALSE : IsModuleValid(hModule); // GetModuleFileName(hModule, szModule, countof(szModule));
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
}

BOOL WINAPI OnFreeLibraryWork(FARPROC lpfn, HookItem *ph, BOOL bMainThread, HMODULE hModule)
{
	typedef BOOL (WINAPI* OnFreeLibrary_t)(HMODULE hModule);
	BOOL lbRc = FALSE;
	BOOL lbResource = LDR_IS_RESOURCE(hModule);
	// lbResource получается TRUE например при вызовах из version.dll

	UnprepareModule(hModule, NULL, 0);

#ifdef _DEBUG
	BOOL lbModulePre = IsModuleValid(hModule); // GetModuleFileName(hModule, szModule, countof(szModule));
#endif

	// Section locking is inadmissible. One FreeLibrary may cause another FreeLibrary in _different_ thread.
	lbRc = ((OnFreeLibrary_t)lpfn)(hModule);
	DWORD dwFreeErrCode = GetLastError();

	// Далее только если !LDR_IS_RESOURCE
	if (lbRc && !lbResource)
		UnprepareModule(hModule, NULL, 1);

	SetLastError(dwFreeErrCode);
	return lbRc;
}

BOOL WINAPI OnFreeLibrary(HMODULE hModule)
{
	typedef BOOL (WINAPI* OnFreeLibrary_t)(HMODULE hModule);
	ORIGINALFAST(FreeLibrary);
	return OnFreeLibraryWork((FARPROC)F(FreeLibrary), ph, FALSE, hModule);
}

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
		char sFunc[128]; _wsprintfA(sFunc, SKIPLEN(countof(sFunc)) "Hook[%u]: %s\n", GetCurrentThreadId(), asFunc);
		DebugStringA(sFunc);
		SetLastError(nErr);
	}
	else
	{
		gbSuppressShowCall = false;
	}
}
#endif