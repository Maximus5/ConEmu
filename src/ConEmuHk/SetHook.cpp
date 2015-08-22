
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

//#define USECHECKPROCESSMODULES
#define ASSERT_ON_PROCNOTFOUND

#include <windows.h>
//#include <Tlhelp32.h>
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
#include "../modules/minhook/include/MinHook.h"
#include "../common/HkFunc.h"


#ifdef _DEBUG
	//WARNING!!! OutputDebugString must NOT be used from ConEmuHk::DllMain(DLL_PROCESS_DETACH). See Issue 465
	#define DebugString(x) //if ((gnDllState != ds_DllProcessDetach) || gbIsSshProcess) OutputDebugString(x)
	#define DebugStringA(x) //if ((gnDllState != ds_DllProcessDetach) || gbIsSshProcess) OutputDebugStringA(x)
#else
	#define DebugString(x)
	#define DebugStringA(x)
#endif


HMODULE ghOurModule = NULL; // Our dll library
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



//typedef VOID (WINAPI* OnLibraryLoaded_t)(HMODULE ahModule);
HMODULE ghOnLoadLibModule = NULL;
OnLibraryLoaded_t gfOnLibraryLoaded = NULL;
OnLibraryLoaded_t gfOnLibraryUnLoaded = NULL;

// Forward declarations of the hooks

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


BOOL gbLogLibraries = FALSE;
DWORD gnLastLogSetChange = 0;


// Используется в том случае, если требуется выполнить оригинальную функцию, без нашей обертки
// пример в OnPeekConsoleInputW
void* __cdecl GetOriginalAddress(void* OurFunction, HookItem** ph, bool abAllowNulls /*= false*/)
{
	if (gpHooks)
	{
		for (int i = 0; gpHooks[i].NewAddress; i++)
		{
			if (gpHooks[i].NewAddress == OurFunction)
			{
				if (ph) *ph = &(gpHooks[i]);
				// Return address where we may call "original" function
				return gpHooks[i].CallAddress;
			}
		}
	}

	_ASSERT(!gbHooksWasSet || abAllowNulls);
	return NULL;
}

// Used in GetLoadLibraryAddress, however it may be obsolete with minhook
FARPROC WINAPI GetLoadLibraryW()
{
	LPVOID fnLoadLibraryW = (LPVOID)&LoadLibraryW;
	return (FARPROC)fnLoadLibraryW;
}

FARPROC WINAPI GetWriteConsoleW()
{
	return (FARPROC)GetOriginalAddress((LPVOID)CEAnsi::OnWriteConsoleW, NULL);
}

FARPROC WINAPI GetVirtualAlloc()
{
	LPVOID fnVirtualAlloc = NULL;
	#ifdef _DEBUG
	extern LPVOID WINAPI OnVirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
	fnVirtualAlloc = GetOriginalAddress((LPVOID)OnVirtualAlloc, NULL);
	#endif
	if (!fnVirtualAlloc)
		fnVirtualAlloc = (LPVOID)&VirtualAlloc;
	return (FARPROC)fnVirtualAlloc;
}

FARPROC WINAPI GetTrampoline(LPCSTR pszName)
{
	if (gpHooks)
	{
		for (int i = 0; gpHooks[i].NewAddress; i++)
		{
			if (strcmp(gpHooks[i].Name, pszName) == 0)
			{
				// Return address where we may call "original" function
				return (FARPROC)gpHooks[i].CallAddress;
			}
		}
	}
	//_ASSERT(!gbHooksWasSet); -- DON'T CALL ANY VISUAL FUNCTIONS HERE !!!
	return NULL;
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
int InitHooks(HookItem* apHooks)
{
	int iFunc = 0;
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

	if (!gpHookCS)
	{
		gpHookCS = new MSection;
	}

	if (gpHooks == NULL)
	{
		MH_STATUS status = MH_Initialize();
		if (status != MH_OK)
		{
			_ASSERTE(status == MH_OK);
			return -1;
		}

		gpHooks = (HookItem*)calloc(sizeof(HookItem),MAX_HOOKED_PROCS);
		if (!gpHooks)
		{
			return -2;
		}

		// Load kernelbase
		static bool KernelHooked = false;
		if (!KernelHooked)
		{
			KernelHooked = true;

			_ASSERTEX(ghKernel32 != NULL);
			if (IsWin7())
			{
				ghKernelBase = LoadLibrary(kernelbase);
			}
		}

		if (!InitHooksLibrary())
		{
			SafeFree(gpHooks);
			return -3;
		}
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

		if (!gpHooks[i].HookedAddress)
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
				if (mod == ghKernel32)
				{
					if (!(gpHooks[i].HookedAddress = (void*)GetProcAddress(ghKernelBase, ExportName)))
					{
						// Strange, most kernel functions are expected to be in KernelBase now
						gpHooks[i].HookedAddress = (void*)GetProcAddress(mod, ExportName);
					}
					else
					{
						mod = ghKernelBase;
					}
				}
				else
				{
					gpHooks[i].HookedAddress = (void*)GetProcAddress(mod, ExportName);
				}

				if (gpHooks[i].HookedAddress != NULL)
				{
					iFunc++;
				}
				// WinXP does not have many hooked functions, will not show dozens of asserts
				#ifdef _DEBUG
				else
				{
					static int isWin7 = 0;
					if (isWin7 == 0)
					{
						OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7)};
						DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
						BOOL isGrEq = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
						isWin7 = isGrEq ? 1 : -1;
					}

					_ASSERTE((isWin7 == -1) || (gpHooks[i].HookedAddress != NULL));
				}
				#endif

				gpHooks[i].hDll = mod;
			}
		}
	}

	return iFunc;
}

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
			HookItem* ph = NULL;
			GetOriginalAddress((LPVOID)OnLoadLibraryW, &ph, true);
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
bool SetAllHooks()
{
	if (!gpHooks)
	{
		HLOG1("SetAllHooks.InitHooks",0);
		InitHooks(NULL);
		if (!gpHooks)
			return false;
		HLOGEND1();
	}

	#ifdef _DEBUG
	wchar_t szHookProc[128];
	for (int i = 0; gpHooks[i].NewAddress; i++)
	{
		if (!gpHooks[i].HookedAddress || gpHooks[i].CallAddress)
			continue; // Already processed
		msprintf(szHookProc, countof(szHookProc),
			L"## %S :: " WIN3264TEST(L"x%08X", L"x%X%08X") L" ==>> " WIN3264TEST(L"x%08X", L"x%X%08X") L"\n",
			gpHooks[i].Name,
			WIN3264WSPRINT(gpHooks[i].HookedAddress),
			WIN3264WSPRINT(gpHooks[i].NewAddress)
			);
		DebugString(szHookProc);
	}
	#endif

	// Для исполняемого файла могут быть заданы дополнительные inject-ы (сравнение в FAR)
	HMODULE hExecutable = GetModuleHandle(0);

	HLOG0("SetAllHooks.GetMainThreadId",0);
	// Найти ID основной нити
	GetMainThreadId(false);
	_ASSERTE(gnHookMainThreadId!=0);
	HLOGEND();

	MH_STATUS status;
	for (int i = 0; gpHooks[i].NewAddress; i++)
	{
		if (gpHooks[i].HookedAddress && !gpHooks[i].CallAddress)
		{
			status = MH_CreateHook((LPVOID)gpHooks[i].HookedAddress, (LPVOID)gpHooks[i].NewAddress, &gpHooks[i].CallAddress);
			_ASSERTE(status == MH_OK);
		}
	}

	status = MH_EnableHook(MH_ALL_HOOKS);
	_ASSERTE(status == MH_OK);

	DebugString(L"SetAllHooks finished\n");

	return true;
}

void UnsetAllHooks()
{
	HMODULE hExecutable = GetModuleHandle(0);

	wchar_t szInfo[MAX_PATH+2] = {};

	if (gbLdrDllNotificationUsed)
	{
		_ASSERTEX(LdrUnregisterDllNotification!=NULL);
		LdrUnregisterDllNotification(gpLdrDllNotificationCookie);
	}

	// Set all "original" function calls to NULL
	{
	extern FARPROC CallWriteConsoleW;
	CallWriteConsoleW = NULL;
	extern GetConsoleWindow_T gfGetRealConsoleWindow; // from ConEmuCheck.cpp
	gfGetRealConsoleWindow = NULL;

	hkFunc.OnHooksUnloaded();
	}

	MH_STATUS status = MH_Uninitialize();

	#ifdef _DEBUG
	DebugStringA("UnsetAllHooks finished\n");
	#endif

	UNREFERENCED_PARAMETER(status);
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
		HWND hConWnd = GetRealConsoleWindow();
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

	int iFunc = 0;
	if (!lbAllSysLoaded)
	{
		// Некоторые перехватываемые библиотеки могли быть
		// не загружены во время первичной инициализации
		// Соответственно для них (если они появились) нужно
		// получить "оригинальные" адреса процедур
		iFunc = InitHooks(NULL);
	}


	if (!module)
	{
		LoadModuleFailed(asModuleA, asModuleW);
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
				HWND hConWnd = GetRealConsoleWindow();
				CESERVER_REQ* pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
				ExecuteFreeResult(pIn);
				if (pOut) ExecuteFreeResult(pOut);
			}
		}

		delete sp;
		sp = NULL;
	}

	// Refresh hooked exports in the loaded library
	if (iFunc > 0)
	{
		SetAllHooks();
	}

	lbModuleOk = true;

	return lbModuleOk;
}

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

	if (gbLdrDllNotificationUsed)
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
				HWND hConWnd = GetRealConsoleWindow();
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