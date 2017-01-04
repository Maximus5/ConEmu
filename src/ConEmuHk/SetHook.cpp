
/*
Copyright (c) 2009-2017 Maximus5
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

#define HIDE_USE_EXCEPTION_INFO

#define DROP_SETCP_ON_WIN2K3R2

//#define USE_ONLY_INT_CHECK_PTR
#undef USE_ONLY_INT_CHECK_PTR

// Иначе не опередяется GetConsoleAliases (хотя он должен быть доступен в Win2k)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501


//#define USECHECKPROCESSMODULES
#define ASSERT_ON_PROCNOTFOUND

#include <windows.h>
#include <intrin.h>
#include "../common/Common.h"
#include "../common/ConEmuCheck.h"
#include "../common/WErrGuard.h"
#include "../common/MSection.h"
#include "../common/WModuleCheck.h"
//#include "../common/MArray.h"
#include "Ansi.h"
#include "DefTermHk.h"
#include "GuiAttach.h"
#include "hkConsole.h"
#include "hkDialog.h"
#include "hkKernel.h"
#include "hkLibrary.h"
#include "hkStdIO.h"
#include "MainThread.h"
#include "SetHook.h"
#include "ShellProcessor.h"
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

extern HWND    ghConWnd;      // RealConsole

extern bool gbPrepareDefaultTerminal;

#ifdef _DEBUG
bool gbSuppressShowCall = false;
bool gbSkipSuppressShowCall = false;
bool gbSkipCheckProcessModules = false;
#endif

// Called function indexes logging
namespace HookLogger
{
	// #define HOOK_LOG_MAX 1024 // Must be a power of 2
	FnCall g_calls[HOOK_LOG_MAX];
	LONG   g_callsidx = -1;
};

MH_STATUS g_mhInit = MH_UNKNOWN;
MH_STATUS g_mhCreate = MH_UNKNOWN;
MH_STATUS g_mhEnable = MH_UNKNOWN;
MH_STATUS g_mhCritical = MH_UNKNOWN;
MH_STATUS g_mhDisableAll = MH_UNKNOWN;
MH_STATUS g_mhDeinit = MH_UNKNOWN;


//!!!All dll names MUST BE LOWER CASE!!!
//!!!WARNING!!! Добавляя в этот список - не забыть добавить и в GetPreloadModules() !!!
const wchar_t *kernelbase = L"kernelbase.dll",	*kernelbase_noext = L"kernelbase";
const wchar_t *kernel32 = L"kernel32.dll",	*kernel32_noext = L"kernel32";
const wchar_t *user32   = L"user32.dll",	*user32_noext   = L"user32";
const wchar_t *gdi32    = L"gdi32.dll",		*gdi32_noext    = L"gdi32";
const wchar_t *shell32  = L"shell32.dll",	*shell32_noext  = L"shell32";
const wchar_t *advapi32 = L"advapi32.dll",	*advapi32_noext = L"advapi32";
//!!!WARNING!!! Добавляя в этот список - не забыть добавить и в GetPreloadModules() !!!
HMODULE
	ghKernelBase = NULL,
	ghKernel32 = NULL,
	ghUser32 = NULL,
	ghGdi32 = NULL,
	ghShell32 = NULL,
	ghAdvapi32 = NULL;
HMODULE* ghSysDll[] = {
	&ghKernelBase,
	&ghKernel32,
	&ghUser32,
	&ghGdi32,
	&ghShell32,
	&ghAdvapi32};
//!!!WARNING!!! Добавляя в этот список - не забыть добавить и в GetPreloadModules() !!!


// Forwards

void LogModuleLoaded(LPCWSTR pwszModule, HMODULE hModule);
void LogModuleUnloaded(LPCWSTR pwszModule, HMODULE hModule);


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
	static size_t snModulesCount = 0;
	static PreloadModules* spModules = NULL;
	if (spModules)
	{
		if (ppModules)
			*ppModules = spModules;
		return snModulesCount;
	}

	PreloadModules Modules[] =
	{
		{gdi32,		gdi32_noext,	&ghGdi32},
		{shell32,	shell32_noext,	&ghShell32},
		{advapi32,	advapi32_noext,	&ghAdvapi32},
	};
	snModulesCount = countof(Modules);
	spModules = new PreloadModules[snModulesCount];
	if (!spModules)
		return 0;
	memmove(spModules, Modules, sizeof(Modules));
	if (ppModules)
		*ppModules = spModules;
	return snModulesCount;
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


///
/// Let's know all processed modules
///

HkModuleMap* gpHookedModules = NULL;

bool InitializeHookedModules()
{
	if (!gpHookedModules)
	{
		//MessageBox(NULL, L"InitializeHookedModules", L"Hooks", MB_SYSTEMMODAL);

		//WARNING: "new" вызывать из DllStart нельзя! DllStart вызывается НЕ из главной нити,
		//WARNING: причем, когда главная нить еще не была запущена. В итоге, если это 
		//WARNING: попытаться сделать мы получим:
		//WARNING: runtime error R6030  - CRT not initialized
		// -- gpHookedModules = new MArray<HkModuleInfo>;
		// -- поэтому тупо через массив
		HkModuleMap* pMap = (HkModuleMap*)calloc(sizeof(HkModuleMap),1);
		if (!pMap)
		{
			_ASSERTE(pMap!=NULL);
		}
		else if (!pMap->Init())
		{
			free(pMap);
		}
		else
		{
			gpHookedModules = pMap;
		}
	}

	return (gpHookedModules != NULL);
}

void FinalizeHookedModules()
{
	HLOG1("FinalizeHookedModules",0);
	if (gpHookedModules)
	{
		HkModuleInfo** pp = NULL;
		INT_PTR iCount = gpHookedModules->GetKeysValues(NULL, &pp);
		if (iCount > 0)
		{
			for (INT_PTR i = 0; i < iCount; i++)
				free(pp[i]);
			gpHookedModules->ReleasePointer(pp);
		}
		gpHookedModules->Release();
		SafeFree(gpHookedModules);
	}
	HLOGEND1();
}

HkModuleInfo* IsHookedModule(HMODULE hModule, LPWSTR pszName = NULL, size_t cchNameMax = 0)
{
	// Must be initialized already, but JIC
	if (!gpHookedModules)
		InitializeHookedModules();

	if (!gpHookedModules)
	{
		_ASSERTE(gpHookedModules!=NULL);
		return NULL;
	}

	//bool lbHooked = false;

	HkModuleInfo* p = NULL;
	if (gpHookedModules->Get(hModule, &p))
	{
		//_ASSERTE(p->Hooked == 1 || p->Hooked == 2);
		//lbHooked = true;

		// If we need to retrieve module name by its handle
		if (pszName && cchNameMax)
		{
			lstrcpyn(pszName, p->sModuleName, (int)cchNameMax);
		}
	}

	return p;
}

HkModuleInfo* AddHookedModule(HMODULE hModule, LPCWSTR sModuleName)
{
	// Must be initialized already, but JIC
	if (!gpHookedModules)
		InitializeHookedModules();

	_ASSERTE(gpHookedModules);
	if (!gpHookedModules)
	{
		_ASSERTE(gpHookedModules!=NULL);
		return NULL;
	}

	HkModuleInfo* p = IsHookedModule(hModule);

	if (!p)
	{
		p = (HkModuleInfo*)calloc(sizeof(HkModuleInfo),1);
		if (!p)
		{
			_ASSERTE(p!=NULL);
		}
		else
		{
			p->hModule = hModule;

			lstrcpyn(p->sModuleName, sModuleName?sModuleName:L"", countof(p->sModuleName));
			//_ASSERTEX(lstrcmpi(p->sModuleName,L"dsound.dll"));

			gpHookedModules->Set(hModule, p);
		}
	}

	return p;
}

void RemoveHookedModule(HMODULE hModule)
{
	// Must be initialized already, but JIC
	if (!gpHookedModules)
		InitializeHookedModules();

	_ASSERTE(gpHookedModules);
	if (!gpHookedModules)
	{
		_ASSERTE(gpHookedModules!=NULL);
		return;
	}

	HkModuleInfo* p = NULL;
	if (gpHookedModules->Get(hModule, &p, true/*Remove*/))
	{
		SafeFree(p);
	}
}



//typedef VOID (WINAPI* OnLibraryLoaded_t)(HMODULE ahModule);
HMODULE ghOnLoadLibModule = NULL;
OnLibraryLoaded_t gfOnLibraryLoaded = NULL;
OnLibraryLoaded_t gfOnLibraryUnLoaded = NULL;


HookItem *gpHooks = NULL; // [MAX_HOOKED_PROCS]
size_t gnHookedFuncs = 0;


BOOL gbLogLibraries = FALSE;
DWORD gnLastLogSetChange = 0;


// Используется в том случае, если требуется выполнить оригинальную функцию, без нашей обертки
// пример в OnPeekConsoleInputW
void* __cdecl GetOriginalAddress(void* OurFunction, DWORD nFnID /*= 0*/, void* ApiFunction/*= NULL*/, HookItem** ph/*= NULL*/, bool abAllowNulls /*= false*/)
{
	void* lpfn = NULL;

	// Initialized?
	if (!gpHooks || !HooksWereSetRaw)
	{
		goto wrap;
	}

	// We must know function index in the gpHooks
	if (nFnID && (nFnID <= gnHookedFuncs))
	{
		size_t nIdx = nFnID-1;
		if (gpHooks[nIdx].NewAddress == OurFunction)
		{
			if (ph) *ph = &(gpHooks[nIdx]);
			// Return address where we may call "original" function
			lpfn = gpHooks[nIdx].CallAddress;
			HookLogger::Log(nFnID);
			InterlockedIncrement(&gpHooks[nIdx].Counter);
			goto wrap;
		}
	}

	// That is strange. Try to find via pointer
	// gh-888: Due AllocConsole from DefTerm (GUI app) only limited list of functions may be hooked
	_ASSERTE((abAllowNulls || gbPrepareDefaultTerminal) && "Function index was not detected");
	for (int i = 0; gpHooks[i].NewAddress; i++)
	{
		if (gpHooks[i].NewAddress == OurFunction)
		{
			if (ph) *ph = &(gpHooks[i]);
			// Return address where we may call "original" function
			lpfn = gpHooks[i].CallAddress;
			HookLogger::Log(i+1); // Function ID
			InterlockedIncrement(&gpHooks[i].Counter);
			goto wrap;
		}
	}

wrap:
	// Not yet hooked?
	if (!lpfn && !abAllowNulls && ApiFunction)
	{
		#ifdef _DEBUG
		if (HooksWereSet)
		{
			static bool asserted = false;
			_ASSERT(asserted && "The function was not hooked before");
			if (gbPrepareDefaultTerminal)
				asserted = true;
		}
		#endif
		if (ph) *ph = NULL;
		lpfn = ApiFunction;
	}

	_ASSERT(lpfn || (!HooksWereSet || abAllowNulls));
	return lpfn;
}

// Used in GetLoadLibraryAddress, however it may be obsolete with minhook
FARPROC WINAPI GetLoadLibraryW()
{
	// KERNEL ADDRESS
	LPVOID fnLoadLibraryW = NULL; // Can't use (LPVOID)&LoadLibraryW anymore, our imports are changed
	HookItem* p = NULL;
	if (GetOriginalAddress((LPVOID)OnLoadLibraryW, HOOK_FN_ID(LoadLibraryW), NULL, &p, true))
	{
		fnLoadLibraryW = p->HookedAddress;
		if (!fnLoadLibraryW)
		{
			_ASSERTEX(p->HookedAddress && "LoadLibraryW was not hooked?");
			fnLoadLibraryW = (LPVOID)&LoadLibraryW;
		}
	}
	else
	{
		_ASSERTEX(fnLoadLibraryW && "Failed to find function address in Kernel32");
	}
	return (FARPROC)fnLoadLibraryW;
}

FARPROC WINAPI GetWriteConsoleW()
{
	return (FARPROC)GetOriginalAddress((LPVOID)OnWriteConsoleW, HOOK_FN_ID(WriteConsoleW));
}

// Our modules (ConEmuCD.dll, ConEmuDW.dll, ConEmu.dll and so on)
// need and ready to call unhooked functions (trampolines)
BOOL WINAPI RequestTrampolines(LPCWSTR asModule, HMODULE hModule)
{
	if (!gpHooks)
		return FALSE;
	bool bRc = SetImports(asModule, hModule, TRUE);
	return bRc;
}



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

	// Init gnLdrDllNotificationUsed. Supported only in Win8 and higher.
	if (!gnLdrDllNotificationUsed)
	{
		CheckLdrNotificationAvailable();
	}

	if (!InitializeHookedModules())
	{
		return -3;
	}

	if (gpHooks == NULL)
	{
		// gh#250: Fight with CreateToolhelp32Snapshot lags
		MH_INITIALIZE mhInit = {sizeof(mhInit)};
		mhInit.Flags = MH_FLAGS_SKIP_EXEC_CHECK;
		if (gbPrepareDefaultTerminal)
		{
			// Let use ‘standard’ enumerations in DefTerm mode
			mhInit.ThreadListCreate = CreateToolhelp32Snapshot;
			mhInit.ThreadListFirst = Thread32First;
			mhInit.ThreadListNext = Thread32Next;
			mhInit.ThreadListClose = CloseHandle;
		}
		else
		{
			// Use our internal enumerator
			mhInit.ThreadListCreate = HookThreadListCreate;
			mhInit.ThreadListFirst = HookThreadListNext;
			mhInit.ThreadListNext = HookThreadListNext;
			mhInit.ThreadListClose = HookThreadListClose;
		}

		g_mhInit = MH_InitializeEx(&mhInit);
		if (g_mhInit != MH_OK)
		{
			_ASSERTE(g_mhInit == MH_OK);
			return -1;
		}

		gpHooks = (HookItem*)calloc(sizeof(HookItem),MAX_HOOKED_PROCS);
		if (!gpHooks)
		{
			return -2;
		}

		// Load kernelbase
		// cmd.exe calls functions from KernelBase.dll but not from kernel32.dll
		// Actually, cmd.exe links to set of api-ms-win-core-*.dll libraries
		// But that is the behavior from Windows 8.0 and higher only!
		static bool KernelHooked = false;
		if (!KernelHooked)
		{
			KernelHooked = true;

			_ASSERTEX(ghKernel32 != NULL);
			// Windows 7 still uses kernel32.dll,
			// 64-bit version of MultiRun was printed unprocessed ANSI
			if (IsWin8())
			{
				ghKernelBase = LoadLibrary(kernelbase);
			}
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

			j = 0; // using while, because of j

			while (gpHooks[j].NewAddress)
			{
				if (gpHooks[j].NewAddress == apHooks[i].NewAddress)
				{
					_ASSERTEX((gnDllState & ds_HooksStarted) && "Function NewAddress is not unique! Skipped!");
					skip = true; break;
				}

				if (gpHooks[j].NameCRC == NameCRC
					&& strcmp(gpHooks[j].Name, apHooks[i].Name) == 0
					&& wcscmp(gpHooks[j].DllName, apHooks[i].DllName) == 0)
				{
					_ASSERTEX(FALSE && "Function name/library is not unique! Skipped!");
					skip = true;
					break;
				}

				j++;
			}

			if (skip) continue;


			if ((j+1) >= MAX_HOOKED_PROCS)
			{
				// Maximum possible hooked function count was reached
				_ASSERTE((j+1) < MAX_HOOKED_PROCS);
				break;
			}

			// Store new function
			gpHooks[j] = apHooks[i];
			// Set function ID and NameCRC
			if (apHooks[i].pnFnID)
				*apHooks[i].pnFnID = j+1; // 1-based ID
			gpHooks[j].NameCRC = NameCRC;
			_ASSERTEX(gpHooks[j].Counter == 0);

			// Increase total hooked count
			_ASSERTEX(j >= gnHookedFuncs);
			gnHookedFuncs = j+1;

			// Clear next item just in case
			gpHooks[j+1].Name = NULL;
			gpHooks[j+1].NewAddress = NULL;
		}
	}

	// Local variables to speed up GetModuleHandle calls
	HMODULE hLastModule = NULL;
	LPCWSTR pszLastModule = NULL;

	// Determine original function addresses
	for (i = 0; gpHooks[i].NewAddress; i++)
	{
		if (gpHooks[i].DllNameA[0] == 0)
		{
			int nLen = WideCharToMultiByte(CP_ACP, 0, gpHooks[i].DllName, -1, gpHooks[i].DllNameA, (int)countof(gpHooks[i].DllNameA), 0,0);
			if (nLen > 0) CharLowerBuffA(gpHooks[i].DllNameA, nLen);
		}

		if (!gpHooks[i].HookedAddress)
		{
			// If we need to hook exact library (kernel32) instead of KernelBase
			HMODULE hRequiredMod = gpHooks[i].hDll;

			// Don't load them now with LoadLibrary, process only already loaded modules
			HMODULE mod = hRequiredMod;
			if (!hRequiredMod)
			{
				// Speed up GetModuleHandle calls
				if (pszLastModule == gpHooks[i].DllName)
				{
					mod = hLastModule;
				}
				else
				{
					pszLastModule = gpHooks[i].DllName;
					hLastModule = mod = GetModuleHandle(pszLastModule);
				}
			}

			if (mod == NULL)
			{
				_ASSERTE(mod != NULL 
					// Библиотеки, которые могут быть НЕ подлинкованы на старте
					|| (gpHooks[i].DllName == shell32 
						|| gpHooks[i].DllName == user32 
						|| gpHooks[i].DllName == gdi32 
						|| gpHooks[i].DllName == advapi32
						));
			}
			else
			{
				// NB, we often get XXXStub instead of the function itself
				const char* ExportName = gpHooks[i].NameOrdinal ? ((const char*)gpHooks[i].NameOrdinal) : gpHooks[i].Name;

				//TODO: In fact, we need to hook BOTH kernel32.dll and Kernelbase.dll   *
				//TODO: But that is subject to change our code... otherwise we may get  *
				//TODO: problems with double hooked calls                               *

				if (ghKernelBase           // If Kernelbase.dll is loaded
					&& (mod == ghKernel32) // Than, for kernel32.dll we'll try to hook them in Kernelbase.dll
					&& !hRequiredMod       // But, some kernel function must be hooked in the kernel32.dll itself (ExitProcess)
					)
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
				#ifdef _DEBUG
				else
				{
					// WinXP does not have many hooked functions, don't show dozens of asserts
					_ASSERTE(!IsWin7() || (gpHooks[i].HookedAddress != NULL));
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


// Main initialization routine
bool StartupHooks()
{
	_ASSERTE(!IsConsoleServer(gsExeName));
	if (gbConEmuCProcess)
	{
		// ConEmuC.exe and ConEmuC64.exe must not be "hooked"
		return false;
	}

	//HLOG0("StartupHooks",0);
	gnDllState |= ds_HooksStarting;

	#ifdef _DEBUG
	// Консольное окно уже должно быть инициализировано в DllMain
	_ASSERTE(gbAttachGuiClient || gbDosBoxProcess || gbPrepareDefaultTerminal || (ghConWnd != NULL && ghConWnd == GetRealConsoleWindow()));
	wchar_t sClass[128];
	if (ghConWnd)
	{
		GetClassName(ghConWnd, sClass, countof(sClass));
		_ASSERTE(isConsoleClass(sClass));
	}

	prepare_timings;
	#endif

	// -- ghConEmuWnd уже должен быть установлен в DllMain!!!
	//gbInShellExecuteEx = FALSE;

	WARNING("Получить из мэппинга gdwServerPID");

	//TODO: Change GetModuleHandle to GetModuleHandleEx? Does it exist in Win2k?

	// Зовем LoadLibrary. Kernel-то должен был сразу загрузиться (static link) в любой
	// windows приложении, но вот shell32 - не обязательно, а нам нужно хуки проинициализировать
	ghKernel32 = LoadLibrary(kernel32);
	// user32/shell32/advapi32 тянут за собой много других библиотек, НЕ загружаем, если они еще не подлинкованы
	if (!ghUser32)
	{
		ghUser32 = GetModuleHandle(user32);
		if (ghUser32) ghUser32 = LoadLibrary(user32); // если подлинкован - увеличить счетчик
	}
	ghShell32 = GetModuleHandle(shell32);
	if (ghShell32) ghShell32 = LoadLibrary(shell32); // если подлинкован - увеличить счетчик
	ghAdvapi32 = GetModuleHandle(advapi32);
	if (ghAdvapi32) ghAdvapi32 = LoadLibrary(advapi32); // если подлинкован - увеличить счетчик

	if (ghKernel32)
		gfGetProcessId = (GetProcessId_t)GetProcAddress(ghKernel32, "GetProcessId");

	// Prepare array and check basic requirements (LdrNotification, LoadLibrary, etc.)
	InitHooks(NULL);


	// Populate hooked function list
	if (!gbPrepareDefaultTerminal)
	{
		// General (console & ChildGui)
		HLOG1("StartupHooks.InitHooks",0);
		InitHooksCommon();
		HLOGEND1();
	}
	else
	{
		// Default Terminal?
		HLOG1("StartupHooks.InitHooks",0);
		InitHooksDefTerm();
		HLOGEND1();
	}

	// List of functions is prepared

	// Now we call minhook engine to ‘detour’ the API
	print_timings(L"SetAllHooks");
	HLOG1("SetAllHooks",0);
	bool lbRc = SetAllHooks();
	if (!lbRc)
	{
		gnDllState &= ~ds_HooksStarted;
		gnDllState |= ds_HooksStartFailed;
	}
	HLOGEND1();

	print_timings(L"SetAllHooks - done");

	return lbRc;
}

void ShutdownHooks()
{
	gnDllState |= ds_HooksStopping;

	HLOG1("ShutdownHooks.UnsetImports", 0);
	UnsetImports();
	HLOGEND1();

	HLOG1_("ShutdownHooks.UnsetAllHooks",0);
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

	//if (gpcsHooksRootPtr)
	//{
	//	Delete Critical Section(gpcsHooksRootPtr);
	//	SafeFree(gpcsHooksRootPtr);
	//}

	FinalizeHookedModules();

	gnDllState |= ds_HooksStopped;
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

	//TH32CS_SNAPMODULE - may hang during LoadLibrary/FreeLibrary.
	lbFound = (IsHookedModule(ahModule, pszName, cchNameMax) != NULL);

	return lbFound;
}

LPCWSTR FormatModuleHandle(HMODULE ahModule, LPCWSTR asFmt32, LPCWSTR asFmt64, LPWSTR pszName, size_t cchNameMax)
{
	#ifdef _WIN64
	if (((DWORD_PTR)ahModule) > 0xFFFFFFFF)
	{
		msprintf(pszName, cchNameMax, asFmt64 ? asFmt64 : L"Module=0x%08X%08X", WIN3264WSPRINT(ahModule));
	}
	else
	#endif
	{
		msprintf(pszName, cchNameMax, asFmt32 ? asFmt32 : L"Module=0x%08X", LODWORD(ahModule));
	}
	return pszName;
}




// Let our modules use trampolined (original) versions without supefluous steps
bool SetImportsPrep(LPCWSTR asModule, HMODULE Module, IMAGE_NT_HEADERS* nt_header, BOOL abForceHooks, IMAGE_IMPORT_DESCRIPTOR* Import, size_t ImportCount, bool (&bFnNeedHook)[MAX_HOOKED_PROCS], HkModuleInfo* p);
bool SetImportsChange(LPCWSTR asModule, HMODULE Module, BOOL abForceHooks, bool (&bFnNeedHook)[MAX_HOOKED_PROCS], HkModuleInfo* p);
// Подменить Импортируемые функции в модуле (Module)
//	если (abForceHooks == FALSE) то хуки не ставятся, если
//  будет обнаружен импорт, не совпадающий с оригиналом
//  Это для того, чтобы не выполнять множественные хуки при множественных LoadLibrary
bool SetImports(LPCWSTR asModule, HMODULE Module, BOOL abForceHooks)
{
	IMAGE_IMPORT_DESCRIPTOR* Import = NULL;
	DWORD Size = 0;
	HMODULE hExecutable = GetModuleHandle(0);

	if (!gpHooks)
		return false;

	if (!Module)
		return false;

	// If our module is already processed - there's nothing to do
	HkModuleInfo* p = IsHookedModule(Module);
	/// TODO: On startup only imports from kernel32 and user32 modules are processed
	/// TODO: Other imports (gdi32?) may be left unprocessed and our modules will use hooked variants
	/// TODO: Not a big problem though
	if (p && p->Hooked)
		return true;

	/* No need to do superfluous checks on our modules
	if (!IsModuleValid(Module))
		return false;
	*/

	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)Module;
	IMAGE_NT_HEADERS* nt_header = NULL;

	HLOG0("SetImports.Init",(DWORD)Module);

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


	DEBUGTEST(PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt_header)); //-V220
	_ASSERTE(sizeof(DWORD_PTR)==WIN3264TEST(4,8));

	// Module is valid, but was not processed yet
	if (!p)
	{
		HLOG("SetImports.Add",(DWORD)Module);
		p = AddHookedModule(Module, asModule);
		HLOGEND();
		if (!p)
			return false;
	}
	// Set it now, to be sure
	p->Hooked = 1;

	HLOG("SetImports.Prepare",(DWORD)Module);
	bool res = false, bHooked = false;
	INT_PTR nCount = Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
	bool bFnNeedHook[MAX_HOOKED_PROCS] = {};
	// Separate function to allow exception handlers
	res = SetImportsPrep(asModule, Module, nt_header, abForceHooks, Import, nCount, bFnNeedHook, p);
	HLOGEND();

	HLOG("SetImports.Change",(DWORD)Module);
	// Separate function to allow exception handlers
	bHooked = SetImportsChange(asModule, Module, abForceHooks, bFnNeedHook, p);
	HLOGEND();

	#ifdef _DEBUG
	if (bHooked)
	{
		HLOG("SetImports.FindModuleFileName",(DWORD)Module);
		wchar_t* szDbg = (wchar_t*)calloc(MAX_PATH*3, 2);
		wchar_t* szModPath = (wchar_t*)calloc(MAX_PATH*2, 2);
		FindModuleFileName(Module, szModPath, MAX_PATH*2);
		_wcscpy_c(szDbg, MAX_PATH*3, L"  ## Imports were changed by conemu: ");
		_wcscat_c(szDbg, MAX_PATH*3, szModPath);
		_wcscat_c(szDbg, MAX_PATH*3, L"\n");
		DebugString(szDbg);
		free(szDbg);
		free(szModPath);
		HLOGEND();
	}
	#endif

	return (res && bHooked);
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

bool SetImportsPrep(LPCWSTR asModule, HMODULE Module, IMAGE_NT_HEADERS* nt_header, BOOL abForceHooks, IMAGE_IMPORT_DESCRIPTOR* Import, size_t ImportCount, bool (&bFnNeedHook)[MAX_HOOKED_PROCS], HkModuleInfo* p)
{
	bool res = false;
	size_t i;

	#define GetPtrFromRVA(rva,pNTHeader,imageBase) (PVOID)((imageBase)+(rva))

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
		HLOG0("SetImportsPrep.Begin",ImportCount);

		//_ASSERTE(Size == (nCount * sizeof(IMAGE_IMPORT_DESCRIPTOR))); -- may not be

		for (i = 0; i < ImportCount; i++)
		{
			if (Import[i].Name == 0)
				break;

			HLOG0("SetImportsPrep.Import",i);

			HLOG1("SetImportsPrep.CheckModuleName",i);
			//DebugString( ToTchar( (char*)Module + Import[i].Name ) );
			char* mod_name_ptr = (char*)Module + Import[i].Name;
			DWORD_PTR rvaINT = Import[i].OriginalFirstThunk;
			DWORD_PTR rvaIAT = Import[i].FirstThunk; //-V101
			lstrcpynA(mod_name, mod_name_ptr, countof(mod_name));
			CharLowerBuffA(mod_name, lstrlenA(mod_name)); // MUST BE LOWER CASE!
			lstrcpynA(szCore, mod_name, nCorePrefLen+1);
			lbIsCoreModule = (strcmp(szCore, szCorePrefix) == 0);

			// Process only imports from hooked modules
			bool bHookExists = false;
			for (size_t j = 0; gpHooks[j].Name; j++)
			{
				if ((strcmp(mod_name, gpHooks[j].DllNameA) != 0)
					&& !(lbIsCoreModule && (gpHooks[j].DllName == kernel32)))
					continue;
				bHookExists = true;
				break;
			}
			// That imported module is not hooked (strange)
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

			PIMAGE_IMPORT_BY_NAME pOrdinalNameO = NULL;
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

			HLOG1_("SetImportsPrep.ImportThunksSteps",i);
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
					//         some dlls have bad import tables
					#ifndef USE_SEH
					HLOG("SetImportsPrep.lbBadThunk",f);
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
					HLOG("SetImportsPrep.lbBadThunkO",f);
					bool lbBadThunkO = isBadModulePtr(thunkO, sizeof(*thunkO), Module, nt_header);
					if (lbBadThunkO)
					{
						_ASSERTEX(!lbBadThunkO);
						break;
					}
					#endif

					const char* pszFuncName = NULL;


					// We get function address and (if s==1) function name
					// Now we need to find trampoline address
					HookItem* ph = gpHooks;
					INT_PTR jj = -1;

					if (!s)
					{
						HLOG1("SetImportsPrep.ImportThunks0",s);

						for (size_t j = 0; ph->Name; ++j, ++ph)
						{
							_ASSERTEX(j<gnHookedFuncs && gnHookedFuncs<=MAX_HOOKED_PROCS);

							// If we failed to determine original address of the procedure (kernel32/WriteConsoleOutputW, etc.)
							if (ph->HookedAddress == NULL)
							{
								continue;
							}

							// Если адрес импорта в модуле уже совпадает с адресом одной из наших функций
							if (ph->NewAddress == (void*)thunk->u1.Function)
							{
								res = true; // это уже захучено
								break;
							}

							// Check the address of hooked function
							if ((void*)thunk->u1.Function == ph->HookedAddress)
							{
								jj = j;
								break; // OK, Hook it!
							}
						} // for (size_t j = 0; ph->Name; ++j, ++ph), (s==0)

						HLOGEND1();
					}
					else
					{
						HLOG1("SetImportsPrep.ImportThunks1",s);

						// If changes are restricted if addresses do not match
						if (!abForceHooks)
						{
							//_ASSERTEX(abForceHooks);
							HLOG2("!!!Function skipped of (!abForceHooks)",f);
							continue;
						}

						// Find function name
						if ((thunk->u1.Function != thunkO->u1.Function)
							&& !IMAGE_SNAP_BY_ORDINAL(thunkO->u1.Ordinal))
						{
							pOrdinalNameO = (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA(thunkO->u1.AddressOfData, nt_header, (PBYTE)Module);

							#ifdef USE_SEH
								pszFuncName = (LPCSTR)pOrdinalNameO->Name;
							#else
								HLOG("SetImportsPrep.pOrdinalNameO",f);
								// WARNING: Numerous IsBad???Ptr calls may introduce lags and bugs
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

						HLOG2("SetImportsPrep.FindFunction",f);
						ph = FindFunction(pszFuncName);
						HLOGEND2();
						if (!ph)
						{
							HLOGEND1();
							continue;
						}

						// Module name
						HLOG2_("SetImportsPrep.Module",f);
						if ((strcmp(mod_name, ph->DllNameA) != 0)
							&& !(lbIsCoreModule && (ph->DllName == kernel32)))
						{
							HLOGEND2();
							HLOGEND1();
							// Names are different and no duplicates are allowed
							continue;
						}
						HLOGEND2();

						jj = (ph - gpHooks);

						HLOGEND1();
					}


					if (jj >= 0)
					{
						HLOG1("SetImportsPrep.WorkExport",f);

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
							HLOG2("SetImportsPrep.CheckCallbackPtr.1",f);
							_ASSERTEX(CheckCallbackPtr(p->hModule, 1, (FARPROC*)&p->Addresses[jj].ppAdr, TRUE));
							HLOGEND2();
							p->Addresses[jj].pOld = thunk->u1.Function;
							p->Addresses[jj].pOur = (DWORD_PTR)ph->CallAddress;
							#ifdef _DEBUG
							lstrcpynA(p->Addresses[jj].sName, ph->Name, countof(p->Addresses[jj].sName));
							#endif

							_ASSERTEX(p->nAdrUsed < countof(p->Addresses));
							p->nAdrUsed++; //информационно
						}

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

bool SetImportsChange(LPCWSTR asModule, HMODULE Module, BOOL abForceHooks, bool (&bFnNeedHook)[MAX_HOOKED_PROCS], HkModuleInfo* p)
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

// Return original imports to our modules, changed by SetImports
bool UnsetImports(HkModuleInfo* p)
{
	if (!gpHooks)
		return false;

	bool bUnhooked = false;
	DWORD dwErr = (DWORD)-1;

	if (p && (p->Hooked == 1))
	{
		HLOG1("UnsetHook.Var",0);
		// Change state immediately
		p->Hooked = 2;
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

void UnsetImports()
{
	if (!gpHooks || !gpHookedModules)
		return;

	HkModuleInfo** pp = NULL;
	INT_PTR iCount = gpHookedModules->GetKeysValues(NULL, &pp);
	if (iCount > 0)
	{
		for (INT_PTR i = 0; i < iCount; i++)
		{
			UnsetImports(pp[i]);
		}
		gpHookedModules->ReleasePointer(pp);
	}
}


/// Set hooks using trampolines
bool SetAllHooks()
{
	if (!gpHooks)
	{
		// Functions were not defined?
		_ASSERTE(gpHooks!=NULL);

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

	#if 0
	// Для исполняемого файла могут быть заданы дополнительные inject-ы (сравнение в FAR)
	HMODULE hExecutable = GetModuleHandle(0);
	#endif

	HLOG0("SetAllHooks.GetMainThreadId",0);
	// Найти ID основной нити
	GetMainThreadId(false);
	_ASSERTE(gnHookMainThreadId!=0);
	HLOGEND();

	MH_STATUS status;
	HLOG("SetAllHooks.MH_CreateHook", LODWORD(gnHookedFuncs));
	for (int i = 0; gpHooks[i].NewAddress; i++)
	{
		if (gpHooks[i].HookedAddress && !gpHooks[i].CallAddress)
		{
			g_mhCreate = status = MH_CreateHook((LPVOID)gpHooks[i].HookedAddress, (LPVOID)gpHooks[i].NewAddress, &gpHooks[i].CallAddress);
			_ASSERTE(status == MH_OK);
		}
	}
	HLOGEND();

	// Let GetOriginalAddress return trampolines
	gnDllState |= ds_HooksStarted;

	HLOG("SetAllHooks.MH_EnableHook", 0);
	g_mhEnable = status = MH_EnableHook(MH_ALL_HOOKS);
	HLOGEND();
	_ASSERTE(status == MH_OK);

	DebugString(L"SetAllHooks finished\n");


	SetImports(WIN3264TEST(L"ConEmuHk",L"ConEmuHk64"), ghOurModule, TRUE);

	DebugString(L"SetImports finished\n");


	extern FARPROC CallWriteConsoleW;
	CallWriteConsoleW = (FARPROC)GetOriginalAddress((LPVOID)OnWriteConsoleW, HOOK_FN_ID(WriteConsoleW), NULL, NULL, gbPrepareDefaultTerminal);

	extern GetConsoleWindow_T gfGetRealConsoleWindow; // from ConEmuCheck.cpp
	gfGetRealConsoleWindow = (GetConsoleWindow_T)GetOriginalAddress((LPVOID)OnGetConsoleWindow, HOOK_FN_ID(GetConsoleWindow), NULL, NULL, gbPrepareDefaultTerminal);

	DebugString(L"Functions prepared\n");

	return true;
}

void UnsetAllHooks()
{
	HMODULE hExecutable = GetModuleHandle(0);

	wchar_t szInfo[MAX_PATH+2] = {};

	if (gnLdrDllNotificationUsed)
	{
		HLOG1("UnregisterLdrNotification", 0);
		UnregisterLdrNotification();
		HLOGEND1();
	}

	// Set all "original" function calls to NULL
	{
	extern FARPROC CallWriteConsoleW;
	CallWriteConsoleW = NULL;
	extern GetConsoleWindow_T gfGetRealConsoleWindow; // from ConEmuCheck.cpp
	gfGetRealConsoleWindow = NULL;

	HLOG1("hkFunc.OnHooksUnloaded", 0);
	OnHooksUnloaded();
	HLOGEND1();
	}

	// Some functions must be unhooked first
	MH_STATUS status;
	if (gpHooks)
	{
		HLOG1("MH_DisableHook(Specials)", 0);
		DWORD nSpecialID[] = {
			HOOK_FN_ID(CloseHandle),
			0};
		for (size_t i = 0; i < countof(nSpecialID); i++)
		{
			size_t nFuncID = nSpecialID[i];
			if (nFuncID && (nFuncID <= gnHookedFuncs))
			{
				nFuncID--;
				g_mhCritical = status = MH_DisableHook(gpHooks[nFuncID].HookedAddress);
				if (status == MH_OK)
				{
					gpHooks[nFuncID].CallAddress = NULL;
				}
			}
		}
		HLOGEND1();
	}

	// Don't release minhook buffers in DefTerm mode,
	// keep these instructions just in case...
	if (gbPrepareDefaultTerminal)
	{
		HLOG1("MH_DisableHook(MH_ALL_HOOKS)", 0);
		g_mhDisableAll = MH_DisableHook(MH_ALL_HOOKS);
		HLOGEND1();
	}
	else
	{
		HLOG1("MH_Uninitialize", 0);
		g_mhDeinit = MH_Uninitialize();
		HLOGEND1();
	}

	#ifdef _DEBUG
	DebugStringA("UnsetAllHooks finished\n");
	#endif
}



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
	pIn = ExecuteNewCmdOnCreate(NULL, ghConWnd, eLoadLibrary, L"Fail", asModuleW, szErrCode, L"", NULL, NULL, NULL, NULL,
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


void ProcessOnLibraryLoadedW(HMODULE module)
{
	// Far Manager ConEmu plugin may do some additional operations:
	// such as initialization of background plugins...
	if (gfOnLibraryLoaded && module)
	{
		ScopedObject(CLastErrorGuard);
		HLOG1_("PrepareNewModule.gfOnLibraryLoaded", LODWORD(module));
		gfOnLibraryLoaded(module);
		HLOGEND1();
	}
}


bool PrepareNewModule(HMODULE module, LPCSTR asModuleA, LPCWSTR asModuleW, BOOL abNoSnapshot /*= FALSE*/, BOOL abForceHooks /*= FALSE*/)
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
		HLOG1("PrepareNewModule.InitHooks",0);
		// Некоторые перехватываемые библиотеки могли быть
		// не загружены во время первичной инициализации
		// Соответственно для них (если они появились) нужно
		// получить "оригинальные" адреса процедур
		iFunc = InitHooks(NULL);
		HLOGEND1();
	}


	if (!module)
	{
		LoadModuleFailed(asModuleA, asModuleW);
		return false;
	}

	// Ensure we have unicode module name
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

	// Проверить по gpHookedModules а не был ли модуль уже обработан?
	HLOG1("PrepareNewModule.IsHookedModule",LODWORD(module));
	HkModuleInfo* p = IsHookedModule(module);
	HLOGEND1();
	if (p)
	{
		// Этот модуль уже обработан!
		return false;
	}

	bool lbModuleOk = false;

	BOOL lbResource = LDR_IS_RESOURCE(module);

	HLOG1_("PrepareNewModule.CShellProc",0);
	LogModuleLoaded(pszModuleW, module);
	HLOGEND1();

	// Remember, it is processed already
	HLOG1_("AddHookedModule",LODWORD(module));
	p = AddHookedModule(module, pszModuleW);
	HLOGEND1();
	if (!p)
	{
		// Exit on critical failures
		return false;
	}

	// Refresh hooked exports in the loaded library
	if (iFunc > 0)
	{
		HLOG1_("PrepareNewModule.SetAllHooks",iFunc);
		SetAllHooks();
		HLOGEND1();
	}

	// Far Manager ConEmu plugin may do some additional operations:
	// such as initialization of background plugins...
	ProcessOnLibraryLoadedW(module);

	lbModuleOk = true;
	UNREFERENCED_PARAMETER(p);
	return lbModuleOk;
}


/* ************** */
void UnprepareModule(HMODULE hModule, LPCWSTR pszModule, int iStep)
{
	BOOL lbResource = LDR_IS_RESOURCE(hModule);
	// lbResource получается TRUE например при вызовах из version.dll

	if ((iStep == 0) && gbLogLibraries && !(gnDllState & ds_DllStoping))
	{
		LogModuleUnloaded(pszModule, hModule);
	}


	// Than only real dlls (!LDR_IS_RESOURCE)
	if ((iStep > 0) && !lbResource && !(gnDllState & ds_DllStoping))
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

			// Если выгружена библиотека ghUser32/ghAdvapi32...
			// проверить, может какие наши импорты стали невалидными
			FreeLoadedModule(hModule);
		}
	}
}







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


void LogModuleLoaded(LPCWSTR pwszModule, HMODULE hModule)
{
	// Update gnExeFlags
	// For cmd.exe
	if (gbIsCmdProcess || gbIsPowerShellProcess)
	{
		if (lstrcmpi(PointToName(pwszModule), WIN3264TEST(L"clink_dll_x86.dll", L"clink_dll_x64.dll")) == 0)
			gnExeFlags |= caf_Clink;
	}

	CShellProc* sp = NULL;

	if (!gnLastLogSetChange || ((GetTickCount() - gnLastLogSetChange) > 2000))
	{
		sp = new CShellProc();
		if (sp)
		{
			gnLastLogSetChange = GetTickCount();
			gbLogLibraries = sp->LoadSrvMapping(TRUE) && sp->GetLogLibraries();
		}
	}
	else if (gbLogLibraries)
	{
		sp = new CShellProc();
		if (!sp)
		{
			gbLogLibraries = FALSE;
		}
	}

	if (gbLogLibraries)
	{
		CESERVER_REQ* pIn = NULL;

		wchar_t szInfo[64] = L"";
		FormatModuleHandle(hModule, L"Module=0x%08X", L"Module=0x%08X%08X", szInfo, countof(szInfo));

		pIn = sp->NewCmdOnCreate(eLoadLibrary, NULL, pwszModule, szInfo, NULL, NULL, NULL, NULL, NULL, WIN3264TEST(32,64), 0, NULL, NULL, NULL);
		if (pIn)
		{
			HWND hConWnd = GetRealConsoleWindow();
			CESERVER_REQ* pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
			ExecuteFreeResult(pIn);
			if (pOut) ExecuteFreeResult(pOut);
		}
	}

	SafeDelete(sp);
}

void LogModuleUnloaded(LPCWSTR pwszModule, HMODULE hModule)
{
	CShellProc* sp = new CShellProc();
	if (sp->LoadSrvMapping())
	{
		wchar_t szModule[MAX_PATH*2]; szModule[0] = 0;

		CESERVER_REQ* pIn = NULL;
		if (pwszModule && *pwszModule)
		{
			lstrcpyn(szModule, pwszModule, countof(szModule));
		}
		else
		{
			wchar_t szHandle[64] = L"";
			FormatModuleHandle(hModule, L", <HMODULE=0x%08X>",L", <HMODULE=0x%08X%08X>", szHandle, countof(szHandle));

			// GetModuleFileName may hang in some cases, so we use our internal modules array
			if (FindModuleFileName(hModule, szModule, countof(szModule)-lstrlen(szHandle)-1))
				wcscat_c(szModule, szHandle);
			else
				wcscpy_c(szModule, szHandle+2);
		}

		pIn = sp->NewCmdOnCreate(eFreeLibrary, NULL, szModule, NULL, NULL, NULL, NULL, NULL, NULL,
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

#ifdef _DEBUG
LONG COriginalCallCount::mn_LastTID = 0;
LONG COriginalCallCount::mn_LastFnID = 0;

COriginalCallCount::COriginalCallCount(LONG* pThreadId, LONG* pCount, LONG nFnID, LPCSTR asFnName)
	: mp_ThreadId(pThreadId)
	, mp_Count(pCount)
{
	LONG nTID = GetCurrentThreadId();
	LONG lOldTid = InterlockedExchange(mp_ThreadId, nTID);
	LONG nNewCount = 1;
	if (lOldTid != nTID)
		InterlockedExchange(mp_Count, nNewCount);
	else
		nNewCount = InterlockedIncrement(mp_Count);

	InterlockedExchange(&mn_LastTID, nTID);
	InterlockedExchange(&mn_LastFnID, nFnID);

	if (nNewCount == 2)
	{
		// Don't use ASSERT or any GUI function here to avoid infinite recursion!
		char szMsg[120];
		msprintf(szMsg, countof(szMsg), "!!! Hook !!! %s Count=%u\n", asFnName, nNewCount);
		OutputDebugStringA(szMsg);
		int iDbg = 0;
	}
};

COriginalCallCount::~COriginalCallCount()
{
	LONG nLeft, nTID = GetCurrentThreadId();
	if (*mp_ThreadId == nTID)
	{
		nLeft = InterlockedDecrement(mp_Count);
		if (nLeft <= 0)
			InterlockedExchange(mp_ThreadId, 0);
	}
}
#endif // #ifdef _DEBUG
