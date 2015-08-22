
// Road map
// * RegQueryValue/RegSetValue/RegDeleteKeyValue/RegSetKeyValue/RegGetValue
// * RegDeleteKeyEx/RegDeleteKeyTransacted/RegDeleteKeyValue/RegDeleteTree
// * RegEnumKey/RegEnumKeyEx
// * RegCopyTree
// * wow64 support.
// * RegCreateKeyTransacted/RegOpenKeyTransacted/RegDeleteKeyTransacted
// * RegQueryInfoKey
// * ! Почти все функции Reg (по крайней мере в Win7) теперь лежат в kernel32.dll, а не в advapi32.dll
//
// !!! Next functions must fail for the virtual registry !!!
// * RegReplaceKey
// * RegRenameKey
// * RegUnLoadKey


/*
Copyright (c) 2011 Maximus5
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

#ifdef _DEBUG
	#define DEBUGSTR(x) OutputDebugString(x)
#else
	#define DEBUGSTR(x) //OutputDebugString(x)
#endif

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#define DEFINE_HOOK_MACROS

#define SETCONCP_READYTIMEOUT 5000
#define SETCONCP_TIMEOUT 1000

#include <windows.h>
#include <TCHAR.h>
#include <Tlhelp32.h>
#include <shlwapi.h>
#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"
#include "../common/execute.h"
#include "../common/WObjects.h"
#include "ConEmuHooks.h"
#include "SetHook.h"

// Is not used in release at least
#ifdef HOOKS_USE_VIRT_REGISTRY

#define MAX_HOOKED_PATH 64
//#define MAX_ADD_ENUMS 8 // пока хватает 3-х!
#ifdef _DEBUG
#define KEYS_IN_BLOCK 255
#else
#define KEYS_IN_BLOCK 255
#endif

#define IsOurRoot(k) ((k == HKEY_CURRENT_USER) || (k == HKEY_LOCAL_MACHINE))

#define HKEY__PREDEFINED_MASK         ( (ULONG_PTR)((LONG)0xF0000000) )
#define HKEY__PREDEFINED_TEST         ( (ULONG_PTR)((LONG)0x80000000) )
#define IsPredefined(k) (k && (((ULONG_PTR)k) & HKEY__PREDEFINED_MASK) == HKEY__PREDEFINED_TEST)

HKEY ghNewKeyRoot = NULL;
HKEY ghNewHKCU = NULL, ghNewHKLM32 = NULL, ghNewHKLM64 = NULL;
HKEY ghNewHKCUSoftware = NULL, ghNewHKLM32Software = NULL, ghNewHKLM64Software = NULL;
struct RegKeyHook;
RegKeyHook* gpRegKeyHooks = NULL;
struct RegKeyStore;
RegKeyStore* gpRegKeyStore = NULL;
BOOL gbRegHookedNow = FALSE;


struct RegKeyHook
{
	// Что перехватываем
	HKEY    hkRoot;
	wchar_t wsHooked[MAX_HOOKED_PATH];  // для W-функций
	char    sHooked[MAX_HOOKED_PATH];   // для A-функций
	int     nHookedLen;
	//wchar_t wsHookedU[64];  // для сравнения в W-функциях
	//char    sHookedU[64];   // для сравнения в A-функциях
	// Что показываем в RegEnumKey, вместо реальных ключей
	wchar_t wsGhost[MAX_HOOKED_PATH+40];  // для W-функций
	char    sGhost[MAX_HOOKED_PATH+40];   // для A-функций
	int     nGhostLen;
	//wchar_t wsGhostU[128]; // для сравнения в W-функциях
	//char    sGhostU[128];  // для сравнения в A-функциях
	// Для упрощения обработки - переопределенные пути
	//HKEY    hkNewKey;
	// "HKLM\..." или "HKCU\..."
	wchar_t wsNewPath[MAX_HOOKED_PATH+8]; // для W-функций
	char    sNewPath[MAX_HOOKED_PATH+8];  // для A-функций
};

typedef int RegKeyType;
const RegKeyType
	RKT_None = 0,
	RKT_HKCU = 1,
	RKT_HKLM32 = 2,
	RKT_HKLM64 = 3,
	RKT_HKCU_Software = 4, //-V112
	RKT_HKLM32_Software = 5,
	RKT_HKLM64_Software = 6
	;

struct RegKeyInfo
{
	RegKeyType rkt;
	HKEY hKey;
	//DWORD WowFlags; // KEY_WOW64_64KEY/KEY_WOW64_32KEY
	//HKEY hReplace;

	// Для отслеживания RegEnumKey(HK??\Software\...)
	DWORD nIndexInReal;
	// видимо еще один DWORD нужен, чтобы отслеживать, на чем остановился поиск в RealRegistry
	DWORD nCountInReal;
};

struct RegKeyBlock
{
	struct RegKeyInfo keys[KEYS_IN_BLOCK];
	struct RegKeyBlock *pNextBlock, *pPrevBlock;
};

// Класс для хранения списка открытых HKEY (RegKeyInfo) пока хранятся только "Software",
// для корректной обработки RegEnumKey[Ex]
// И собственно открытия дочерних ключей без полного пути
struct RegKeyStore
{
protected:
	int mn_StoreMaxCount, mn_StoreCurrent;
	// С такой структурой блокировка требуется ТОЛЬКО
	// при добавлении HKEY в список. Проверка наличия
	// блокировки не требует, т.к. блоки НЕ удаляются
	// вплоть до завершения программы.
	RegKeyBlock m_Store, *mp_Last;
	RegKeyInfo m_HKCU, m_HKLM32, m_HKLM64, m_HKLM; // predefined HKEY_...
	MSection mc_Lock;
	RegKeyInfo* mp_LastKey;
	BOOL mb_IsWindows64;
public:
	void Init()
	{
		mb_IsWindows64 = IsWindows64();
		memset(&m_Store, 0, sizeof(m_Store));
		mp_Last = &m_Store;
		mp_LastKey = NULL;
		memset(&m_HKCU, 0, sizeof(m_HKCU));
		memset(&m_HKLM, 0, sizeof(m_HKLM));
		memset(&m_HKLM32, 0, sizeof(m_HKLM32));
		memset(&m_HKLM64, 0, sizeof(m_HKLM64));
		// Common HKCU
		m_HKCU.hKey = HKEY_CURRENT_USER; m_HKCU.rkt = RKT_HKCU;
		// Native for process
		m_HKLM.hKey = HKEY_LOCAL_MACHINE; 
		#ifdef _WIN64
		m_HKLM.rkt = RKT_HKLM64;
		#else
		m_HKLM.rkt = RKT_HKLM32;
		#endif
		// When specified KEY_WOW64_64KEY/KEY_WOW64_32KEY
		m_HKLM32.hKey = HKEY_LOCAL_MACHINE; m_HKLM32.rkt = RKT_HKLM32;
		m_HKLM64.hKey = HKEY_LOCAL_MACHINE; m_HKLM64.rkt = mb_IsWindows64 ? RKT_HKLM64 : RKT_HKLM32;
	};
	void Free()
	{
		RegKeyBlock *pFrom = m_Store.pNextBlock;
		while (pFrom)
		{
			RegKeyBlock *p = pFrom;
			pFrom = pFrom->pNextBlock;
			free(p);
		}
	};
private:
	RegKeyInfo* FindFreeSlot()
	{
		RegKeyBlock *pFrom = &m_Store;
		while (pFrom)
		{
			RegKeyInfo *pKeys = pFrom->keys;
			for (UINT i = 0; i < KEYS_IN_BLOCK; i++, pKeys++)
			{
				if (!pKeys->rkt)
					return pKeys;
			}
			pFrom = pFrom->pNextBlock;
		}
		// Не достаточно ячеек. Добавим
		_ASSERTE(pFrom != NULL); // интересно, в реальной работе будет ли использовано более одного блока...
		// Новый блок
		pFrom = (RegKeyBlock*)calloc(sizeof(RegKeyBlock),1);
		mp_Last->pNextBlock = pFrom;
		return pFrom->keys; // новый блок, возвращаем первую ячейку
	}
	RegKeyInfo* FindKey(HKEY hKey, DWORD wowOptions=0 /*KEY_WOW64_64KEY/KEY_WOW64_32KEY*/)
	{
		if (!hKey)
			return NULL;

		// Speed up last call
		if (mp_LastKey && mp_LastKey->rkt && mp_LastKey->hKey == hKey)
			return mp_LastKey;

		if (hKey == HKEY_CURRENT_USER)
			return &m_HKCU;
		if (hKey == HKEY_LOCAL_MACHINE)
		{
			if (!wowOptions)
				return &m_HKLM;
			else if (wowOptions == KEY_WOW64_32KEY)
				return &m_HKLM32;
			else
				return &m_HKLM64;
		}
		
		RegKeyBlock *pFrom = &m_Store;
		while (pFrom)
		{
			RegKeyInfo *pKeys = pFrom->keys;
			for (UINT i = 0; i < KEYS_IN_BLOCK; i++, pKeys++)
			{
				if (pKeys->rkt && (pKeys->hKey == hKey))
				{
					// Speed up last call
					mp_LastKey = pKeys;
					return pKeys;
				}
			}
			pFrom = pFrom->pNextBlock;
		}
		return NULL;
	};
public:
	void Store(HKEY hKey, RegKeyType rkt)
	{
		if (!hKey || IsPredefined(hKey))
		{
			_ASSERTE(hKey!=NULL);
			_ASSERTE(!IsPredefined(hKey) || (rkt>=RKT_HKCU && rkt<=RKT_HKLM64));
			return;
		}
		//mhk_Last = hkSoftware;
		//WARNING("### Process RegKeyBlocks!");
		MSectionLock SC; SC.Lock(&mc_Lock, TRUE, 500);
		RegKeyInfo* p = FindFreeSlot();
		p->hKey = hKey;
		p->rkt = rkt;
		mp_LastKey = p;
	};
	// wowOptions нужен только для HKLM
	RegKeyType IsStored(HKEY hKey, DWORD wowOptions=0 /*KEY_WOW64_64KEY/KEY_WOW64_32KEY*/)
	{
		RegKeyInfo* p;
		if (hKey && (p = FindKey(hKey, wowOptions)))
			return p->rkt;
		return RKT_None;
	};
	void Remove(HKEY hKey)
	{
		if (!hKey || IsPredefined(hKey))
			return;
		RegKeyInfo* p = FindKey(hKey);
		if (p)
		{
			p->hKey = NULL;
			p->rkt = RKT_None;
		}
	};
	//void AppendEnum(HKEY hkSoftware, const RegKeyHook* apKey)
	//{
	//};
	//void Clear()
	//{
	//	if (mhk_Last)
	//		mhk_Last = NULL;
	//	WARNING("### Process RegKeyBlocks!");
	//	return;
	//};
};




LONG WINAPI OnRegCloseKey(HKEY hKey);
LONG WINAPI OnRegCreateKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
LONG WINAPI OnRegCreateKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);
LONG WINAPI OnRegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
LONG WINAPI OnRegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
LONG WINAPI OnRegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
LONG WINAPI OnRegOpenKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);
LONG WINAPI OnRegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
LONG WINAPI OnRegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
LONG WINAPI OnRegDeleteKeyA(HKEY hKey, LPCSTR lpSubKey);
LONG WINAPI OnRegDeleteKeyW(HKEY hKey, LPCWSTR lpSubKey);
LONG WINAPI OnRegConnectRegistryA(LPCSTR lpMachineName, HKEY hKey, PHKEY phkResult);
LONG WINAPI OnRegConnectRegistryW(LPCWSTR lpMachineName, HKEY hKey, PHKEY phkResult);
// Vista+
//LONG WINAPI OnRegOpenKeyTransacted(HKEY hKey, LPCTSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult, HANDLE hTransaction, PVOID pExtendedParameter);

void PrepareHookedKeyList();

#endif

bool InitHooksReg()
{
	bool lbRc = false;
#ifdef HOOKS_USE_VIRT_REGISTRY
	CESERVER_CONSOLE_MAPPING_HDR* pInfo = GetConMap();
	if (!pInfo || !(pInfo->isHookRegistry&1) || !*pInfo->sHiveFileName)
		return false;

	DEBUGSTR(L"ConEmuHk: Preparing for registry virtualization\n");

	HookItem HooksRegistry[] =
	{
		/* ************************ */
		{(void*)OnRegCloseKey,		"RegCloseKey",		advapi32},
		{(void*)OnRegCreateKeyA,	"RegCreateKeyA",	advapi32},
		{(void*)OnRegCreateKeyW,	"RegCreateKeyW",	advapi32},
		{(void*)OnRegCreateKeyExA,	"RegCreateKeyExA",	advapi32},
		{(void*)OnRegCreateKeyExW,	"RegCreateKeyExW",	advapi32},
		{(void*)OnRegOpenKeyA,		"RegOpenKeyA",		advapi32},
		{(void*)OnRegOpenKeyW,		"RegOpenKeyW",		advapi32},
		{(void*)OnRegOpenKeyExA,	"RegOpenKeyExA",	advapi32},
		{(void*)OnRegOpenKeyExW,	"RegOpenKeyExW",	advapi32},
		{(void*)OnRegDeleteKeyA,	"RegDeleteKeyA",	advapi32},
		{(void*)OnRegDeleteKeyW,	"RegDeleteKeyW",	advapi32},
		/* ************************ */
		{(void*)OnRegConnectRegistryA,	"RegConnectRegistryA",	advapi32},
		{(void*)OnRegConnectRegistryW,	"RegConnectRegistryW",	advapi32},
		/* ************************ */
		{0}
	};
	lbRc = (InitHooks(HooksRegistry) >= 0);

	if (lbRc)
	{
		PrepareHookedKeyList();
		DEBUGSTR(L"ConEmuHk: Registry virtualization prepared\n");
		// Если advapi32.dll уже загружена - можно сразу дернуть экспорты
		if (ghAdvapi32)
		{
			RegOpenKeyEx_f = (RegOpenKeyEx_t)GetProcAddress(ghAdvapi32, "RegOpenKeyExW");
			RegCreateKeyEx_f = (RegCreateKeyEx_t)GetProcAddress(ghAdvapi32, "RegCreateKeyExW");
			RegCloseKey_f = (RegCloseKey_t)GetProcAddress(ghAdvapi32, "RegCloseKey");
		}
	}
	else
	{
		DEBUGSTR(L"ConEmuHk: Registry virtualization failed!\n");
	}
	
#else
	lbRc = true;
#endif
	return lbRc;
}

#ifdef HOOKS_USE_VIRT_REGISTRY
void CloseRootKeys()
{
	PHKEY hk[] = {&ghNewKeyRoot,
		&ghNewHKCU, &ghNewHKLM32,
			(ghNewHKLM64==ghNewHKLM32) ? NULL : &ghNewHKLM64,
		&ghNewHKCUSoftware, &ghNewHKLM32Software,
			(ghNewHKLM64Software==ghNewHKLM32Software) ? NULL : &ghNewHKLM64Software};
	for (UINT i = 0; i < countof(hk); i++)
	{
		if (hk[i])
		{
			if (RegCloseKey_f && *(hk[i]) && (*(hk[i]) != (HKEY)-1))
				RegCloseKey_f(*(hk[i]));
			*(hk[i]) = NULL;
		}
	}
}
#endif

void DoneHooksReg()
{
#ifdef HOOKS_USE_VIRT_REGISTRY
#ifdef _DEBUG
	//WARNING!!! OutputDebugString must NOT be used from ConEmuHk::DllMain(DLL_PROCESS_DETACH). See Issue 465
	DEBUGSTR(L"ConEmuHk: Deinitializing registry virtualization!\n");
#endif
	if (gpRegKeyHooks)
	{
		free(gpRegKeyHooks);
		gpRegKeyHooks = NULL;
	}
	CloseRootKeys();
	if (gpRegKeyStore)
	{
		gpRegKeyStore->Free();
		free(gpRegKeyStore);
		gpRegKeyStore = NULL;
	}
#ifdef _DEBUG
	//WARNING!!! OutputDebugString must NOT be used from ConEmuHk::DllMain(DLL_PROCESS_DETACH). See Issue 465
	DEBUGSTR(L"ConEmuHk: Registry virtualization deinitialized!\n");
#endif
#endif
}

void InitHooksRegThread()
{
}

void DoneHooksRegThread()
{
}


#ifdef HOOKS_USE_VIRT_REGISTRY
// !!! WARNING !!! В ЭТОЙ функции обращений к реестру (или advapi32) НЕ ДЕЛАТЬ !!!
void PrepareHookedKeyList()
{
	CESERVER_CONSOLE_MAPPING_HDR* pInfo = GetConMap();
	if (!pInfo || !(pInfo->isHookRegistry&1) || !*pInfo->sHiveFileName)
		return;

	// Сейчас HookedNow НЕ проверять, т.к. функция вызывается при первоначальной инициализации
    // Здесь важен сам факт того, что виртуальный реестр может быть включен (в любой момент)!
    gbRegHookedNow = ((pInfo->isHookRegistry&3) == 3);
	//BOOL lbHookedNow = ((pInfo->isHookRegistry&3) == 3);
	//if (gbRegHookedNow != lbHookedNow)
	//{
	//	if (gbRegHookedNow && gpRegKeyStore)
	//		gpRegKeyStore->Clear();
	//	gbRegHookedNow = lbHookedNow;
	//	if (!lbHookedNow)
	//		return;
	//}

	if (!gpRegKeyStore)
	{
		WARNING("gpRegKeyStore");
		gpRegKeyStore = (RegKeyStore*)calloc(1,sizeof(*gpRegKeyStore));
		if (!gpRegKeyStore)
		{
			_ASSERTE(gpRegKeyStore!=NULL);
			return;
		}
		gpRegKeyStore->Init();
	}
	
	if (!gpRegKeyHooks)
	{
		wchar_t wsGhost[42] = L"." VIRTUAL_REGISTRY_GUID; // L".{16B56CA5-F8D2-4EEA-93DC-32403C7355E1}";
		//char sGhost[42] = ".{16B56CA5-F8D2-4EEA-93DC-32403C7355E1}";
		
		{
			struct { HKEY hKey; LPCWSTR pszKey; } RegKeyHooks[] =
			{
				// Поддерживаются только ПОДКЛЮЧИ в "Software\\"
				{HKEY_LOCAL_MACHINE, L"Far"},
				{HKEY_CURRENT_USER,  L"Far"},
				{HKEY_LOCAL_MACHINE, L"Far2"},
				{HKEY_CURRENT_USER,  L"Far2"},
				{HKEY_LOCAL_MACHINE, L"Far Manager"},
				{HKEY_CURRENT_USER,  L"Far Manager"},
				{NULL}
			};
			
			gpRegKeyHooks = (RegKeyHook*)calloc(countof(RegKeyHooks)+1, sizeof(RegKeyHook));
			if (!gpRegKeyHooks)
			{
				_ASSERTE(gpRegKeyHooks!=NULL);
				gpRegKeyStore->Free();
				free(gpRegKeyStore);
				gpRegKeyStore = NULL;
				return;
			}
			//memmove(gpRegKeyHooks, RegKeyHooks, sizeof(RegKeyHooks));
			for (size_t i = 0; i < countof(RegKeyHooks) && RegKeyHooks[i].hKey; i++)
			{
				gpRegKeyHooks[i].hkRoot = RegKeyHooks[i].hKey;
				lstrcpyn(gpRegKeyHooks[i].wsHooked, RegKeyHooks[i].pszKey, countof(gpRegKeyHooks[i].wsHooked));
			}
		}
		
						
		// Подготовить структуру gpRegKeyHooks к использованию
		for (UINT i = 0; gpRegKeyHooks[i].hkRoot; i++)
		{
			// Длина перехватываемого ключа
			gpRegKeyHooks[i].nHookedLen = lstrlen(gpRegKeyHooks[i].wsHooked);
			WideCharToMultiByte(CP_ACP, 0, gpRegKeyHooks[i].wsHooked, gpRegKeyHooks[i].nHookedLen+1,
				gpRegKeyHooks[i].sHooked, gpRegKeyHooks[i].nHookedLen+1, 0,0);

			// Что показываем в RegEnumKey, вместо реальных ключей
			wcscpy_c(gpRegKeyHooks[i].wsGhost, PointToName(gpRegKeyHooks[i].wsHooked));
			wcscat_c(gpRegKeyHooks[i].wsGhost, wsGhost);
			gpRegKeyHooks[i].nGhostLen = lstrlen(gpRegKeyHooks[i].wsGhost);
			WideCharToMultiByte(CP_ACP, 0, gpRegKeyHooks[i].wsGhost, -1, gpRegKeyHooks[i].sGhost, countof(gpRegKeyHooks[i].sGhost), 0,0);

			// Для упрощения обработки - переопределенные пути
			msprintf(gpRegKeyHooks[i].wsNewPath, countof(gpRegKeyHooks[i].wsNewPath), L"%s\\Software\\%s",
				(gpRegKeyHooks[i].hkRoot == HKEY_LOCAL_MACHINE) ? L"HKLM" : L"HKCU", gpRegKeyHooks[i].wsHooked);
			WideCharToMultiByte(CP_ACP, 0, gpRegKeyHooks[i].wsNewPath, -1, gpRegKeyHooks[i].sNewPath, countof(gpRegKeyHooks[i].sNewPath), 0, 0);
		}
	}
}


bool InitRegistryRoot(CESERVER_CONSOLE_MAPPING_HDR* pInfo)
{
	wchar_t szTitle[64];
	msprintf(szTitle, countof(szTitle), L"ConEmuHk, PID=%u", GetCurrentProcessId());

	if (!ghAdvapi32)
	{
		GuiMessageBox(ghConEmuWnd, L"ConEmuHk: InitRegistryRoot was called, but ghAdvapi32 is null!\n", szTitle, MB_OK|MB_ICONSTOP|MB_SYSTEMMODAL);
		return false;
	}

	if (!RegOpenKeyEx_f)
	{
		_ASSERTE(RegOpenKeyEx_f!=NULL);
		RegOpenKeyEx_f = (RegOpenKeyEx_t)GetProcAddress(ghAdvapi32, "RegOpenKeyExW");
		if (!RegOpenKeyEx_f)
		{
			GuiMessageBox(ghConEmuWnd, L"ConEmuHk: InitRegistryRoot was called, but GetProcAddress(RegOpenKeyExW) is null!\n", szTitle, MB_OK|MB_ICONSTOP|MB_SYSTEMMODAL);
			return false;
		}
	}
	if (!RegCreateKeyEx_f)
	{
		_ASSERTE(RegCreateKeyEx_f!=NULL);
		RegCreateKeyEx_f = (RegCreateKeyEx_t)GetProcAddress(ghAdvapi32, "RegCreateKeyExW");
		if (!RegCreateKeyEx_f)
		{
			GuiMessageBox(ghConEmuWnd, L"ConEmuHk: InitRegistryRoot was called, but GetProcAddress(RegCreateKeyExW) is null!\n", szTitle, MB_OK|MB_ICONSTOP|MB_SYSTEMMODAL);
			return false;
		}
	}
	if (!RegCloseKey_f)
	{
		_ASSERTE(RegCloseKey_f!=NULL);
		RegCloseKey_f = (RegCloseKey_t)GetProcAddress(ghAdvapi32, "RegCloseKey");
		if (!RegCloseKey_f)
		{
			GuiMessageBox(ghConEmuWnd, L"ConEmuHk: InitRegistryRoot was called, but GetProcAddress(RegCloseKey) is null!\n", szTitle, MB_OK|MB_ICONSTOP|MB_SYSTEMMODAL);
			return false;
		}
	}

	if (ghNewKeyRoot == NULL)
	{
		//OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)};
		//GetVersionEx(&osv);
		//if (osv.dwMajorVersion >= 6)
		if (!*pInfo->sMountKey)
		{
			// Vista+	
			typedef LONG (WINAPI* RegLoadAppKey_t)(LPCWSTR lpFile, PHKEY phkResult, REGSAM samDesired, DWORD dwOptions, DWORD Reserved);
			RegLoadAppKey_t RegLoadAppKey_f = (RegLoadAppKey_t)GetProcAddress(ghAdvapi32, "RegLoadAppKeyW");
			if (RegLoadAppKey_f)
			{
				LONG lRc = 0;
				if ((lRc = RegLoadAppKey_f(pInfo->sHiveFileName, &ghNewKeyRoot, KEY_ALL_ACCESS, 0, 0)) != 0)
				{
					if ((lRc = RegLoadAppKey_f(pInfo->sHiveFileName, &ghNewKeyRoot, KEY_READ, 0, 0)) != 0)
					{
						wchar_t szDbgMsg[128];
						msprintf(szDbgMsg, countof(szDbgMsg), L"ConEmuHk: RegLoadAppKey failed, code=0x%08X!\n", (DWORD)lRc);
						GuiMessageBox(ghConEmuWnd, szDbgMsg, szTitle, MB_OK|MB_ICONSTOP|MB_SYSTEMMODAL);
						ghNewKeyRoot = NULL;
					}
				}
			}
			else
			{
				GuiMessageBox(ghConEmuWnd, L"ConEmuHk: InitRegistryRoot was called, but GetProcAddress(RegLoadAppKeyW) is null!\n", szTitle, MB_OK|MB_ICONSTOP|MB_SYSTEMMODAL);
				ghNewKeyRoot = NULL;
			}				
		}
		else
		{
			// XP Only. т.к. для монтирования хайва требуются права админа
			HKEY hkRoot = pInfo->hMountRoot;
			if (hkRoot == HKEY_CURRENT_USER)
			{
				// Значит это "HKCU\Software\ConEmu Virtual Registry"
				if (RegOpenKeyEx_f(HKEY_CURRENT_USER, VIRTUAL_REGISTRY_ROOT, 0, KEY_ALL_ACCESS, &hkRoot))
				{
					GuiMessageBox(ghConEmuWnd, L"ConEmuHk: RegOpenKeyEx(" VIRTUAL_REGISTRY_ROOT L") failed!\n", szTitle, MB_OK|MB_ICONSTOP|MB_SYSTEMMODAL);
					ghNewKeyRoot = NULL;
					hkRoot = NULL;
				}
			}
			if (hkRoot && RegOpenKeyEx_f(hkRoot, pInfo->sMountKey, 0, KEY_ALL_ACCESS, &ghNewKeyRoot))
			{
				if (RegOpenKeyEx_f(hkRoot, pInfo->sMountKey, 0, KEY_READ, &ghNewKeyRoot))
				{
					wchar_t szErrMsg[512];
					msprintf(szErrMsg, countof(szErrMsg), 
						L"ConEmuHk: RegOpenKeyEx(0x%X,'%s') failed!\n",
						(DWORD)(LONG)(LONG_PTR)hkRoot, pInfo->sMountKey);
					GuiMessageBox(ghConEmuWnd, szErrMsg, szTitle, MB_OK|MB_ICONSTOP|MB_SYSTEMMODAL);
					ghNewKeyRoot = NULL;
				}
			}
		}

		if (!ghNewKeyRoot)
		{
			ghNewKeyRoot = (HKEY)-1;
		}
		else
		{
			LONG lRc = 0;
			LPCWSTR pszKeyName = NULL;
			if (!lRc)
				lRc = RegCreateKeyEx_f(ghNewKeyRoot, pszKeyName=L"HKCU", 0,0,0, KEY_ALL_ACCESS, 0, &ghNewHKCU, 0);
			if (!lRc)
				lRc = RegCreateKeyEx_f(ghNewKeyRoot, pszKeyName=L"HKCU\\Software", 0,0,0, KEY_ALL_ACCESS, 0, &ghNewHKCUSoftware, 0);
			if (!lRc)
				lRc = RegCreateKeyEx_f(ghNewKeyRoot, pszKeyName=L"HKLM", 0,0,0, KEY_ALL_ACCESS, 0, &ghNewHKLM32, 0);
			if (!lRc)
				lRc = RegCreateKeyEx_f(ghNewKeyRoot, pszKeyName=L"HKLM\\Software", 0,0,0, KEY_ALL_ACCESS, 0, &ghNewHKLM32Software, 0);
			if (!lRc && IsWindows64())
			{
				// эта ветка не должна выполняться в 32битных ОС
				if (!lRc)
					lRc = RegCreateKeyEx_f(ghNewKeyRoot, pszKeyName=L"HKLM64", 0,0,0, KEY_ALL_ACCESS, 0, &ghNewHKLM64, 0);
				if (!lRc)
					lRc = RegCreateKeyEx_f(ghNewKeyRoot, pszKeyName=L"HKLM64\\Software", 0,0,0, KEY_ALL_ACCESS, 0, &ghNewHKLM64Software, 0);
			}
			if (lRc != 0)
			{
				CloseRootKeys();
				ghNewKeyRoot = (HKEY)-1; // чтобы не пытаться открыть повторно
				wchar_t szErrInfo[MAX_PATH];
				msprintf(szErrInfo, countof(szErrInfo), L"ConEmuHk: Virtual subkey (%s) creation failed! ErrCode=0x%08X\n",
					pszKeyName ? pszKeyName : L"<NULL>", (DWORD)lRc);
				GuiMessageBox(ghConEmuWnd, szErrInfo, szTitle, MB_OK|MB_ICONSTOP|MB_SYSTEMMODAL);
			}
			// Для консистентности
			if (!ghNewHKLM64)
				ghNewHKLM64 = ghNewHKLM32;
			if (!ghNewHKLM64Software)
				ghNewHKLM64Software = ghNewHKLM32Software;
		}
	}

	if ((ghNewKeyRoot == NULL) || (ghNewKeyRoot == (HKEY)-1))
	{
		DEBUGSTR(L"ConEmuHk: Registry virtualization failed!\n");
		return false;
	}
	else
	{
		DEBUGSTR(L"ConEmuHk: Registry virtualization succeeded\n");
	}

	return true;
}

RegKeyHook* HooksRegistryPtr()
{
	if (!ghAdvapi32 || (ghNewKeyRoot == (HKEY)-1))
		return NULL;
		
	CESERVER_CONSOLE_MAPPING_HDR* pInfo = GetConMap();
	//if (!pInfo || !*pInfo->sHiveFileName)
	//	return NULL;
	gbRegHookedNow = pInfo && ((pInfo->isHookRegistry&3) == 3);
	if (!gbRegHookedNow)
		return NULL;
	
	// ghNewKeyRoot == ((HKEY)-1), - если был облом подключения виртуального реестра
	if (ghNewKeyRoot == NULL)
	{
		if (!InitRegistryRoot(pInfo))
			return NULL;
	}
	else if ((ghNewKeyRoot == NULL) || (ghNewKeyRoot == (HKEY)-1))
	{
		return NULL;
	}

	return gpRegKeyHooks;
}
#endif


/*
RegCloseKey
RegConnectRegistryA
RegConnectRegistryExA
RegConnectRegistryExW
RegConnectRegistryW
RegCopyTreeA
RegCopyTreeW
RegCreateKeyA
RegCreateKeyExA
RegCreateKeyExW
RegCreateKeyTransactedA
RegCreateKeyTransactedW
RegCreateKeyW
RegDeleteKeyA
RegDeleteKeyExA
RegDeleteKeyExW
RegDeleteKeyTransactedA
RegDeleteKeyTransactedW
RegDeleteKeyValueA
RegDeleteKeyValueW
RegDeleteKeyW
RegDeleteTreeA
RegDeleteTreeW
RegDeleteValueA
RegDeleteValueW
RegDisablePredefinedCache
RegDisablePredefinedCacheEx
RegDisableReflectionKey
RegEnableReflectionKey
RegEnumKeyA
RegEnumKeyExA
RegEnumKeyExW
RegEnumKeyW
RegEnumValueA
RegEnumValueW
RegFlushKey
RegGetKeySecurity
RegGetValueA
RegGetValueW
RegLoadAppKeyA
RegLoadAppKeyW
RegLoadKeyA
RegLoadKeyW
RegLoadMUIStringA
RegLoadMUIStringW
RegNotifyChangeKeyValue
RegOpenCurrentUser
RegOpenKeyA
RegOpenKeyExA
RegOpenKeyExW
RegOpenKeyTransactedA
RegOpenKeyTransactedW
RegOpenKeyW
RegOpenUserClassesRoot
RegOverridePredefKey
RegQueryInfoKeyA
RegQueryInfoKeyW
RegQueryMultipleValuesA
RegQueryMultipleValuesW
RegQueryReflectionKey
RegQueryValueA
RegQueryValueExA
RegQueryValueExW
RegQueryValueW
RegRenameKey
RegReplaceKeyA
RegReplaceKeyW
RegRestoreKeyA
RegRestoreKeyW
RegSaveKeyA
RegSaveKeyExA
RegSaveKeyExW
RegSaveKeyW
RegSetKeySecurity
RegSetKeyValueA
RegSetKeyValueW
RegSetValueA
RegSetValueExA
RegSetValueExW
RegSetValueW
RegUnLoadKeyA
RegUnLoadKeyW
*/


#ifdef HOOKS_USE_VIRT_REGISTRY
bool CheckKeyHookedA(HKEY& hKey, LPCSTR& lpSubKey, LPSTR& lpTemp, RegKeyType& rkt, DWORD wowOptions=0 /*KEY_WOW64_64KEY/KEY_WOW64_32KEY*/)
{
	_ASSERTE(lpTemp == NULL);
	lpTemp = NULL;
	rkt = RKT_None;
	if (!hKey || !gpRegKeyStore)
		return true;
	RegKeyHook* p = HooksRegistryPtr();
	if (!p)
		return true;

	if (hKey == ghNewKeyRoot || hKey == ghNewHKCU || hKey == ghNewHKLM32 || hKey == ghNewHKLM64
		|| hKey == ghNewHKCUSoftware || hKey == ghNewHKLM32Software || hKey == ghNewHKLM64Software)
	{
		// Это УЖЕ перехваченный ключ, проверять не нужно
		return true;
	}

		
	LPCSTR lpSubKeyTmp = lpSubKey;
	HKEY hKeyTmp = hKey;

	rkt = gpRegKeyStore->IsStored(hKey, wowOptions/*KEY_WOW64_64KEY/KEY_WOW64_32KEY*/);
	if (!rkt) // rkt == RKT_None
		return true;

	if (rkt >= RKT_HKCU && rkt <= RKT_HKLM64)
	{
		// копия HKCU или HKLM?
		if (!lpSubKeyTmp || !*lpSubKeyTmp)
		{
			return true; // нужно будет дернуть gpRegKeyStore->Store после успешного открытия
		}
		// Если lpSubKeyTmp НЕ начинается с "Software\\" - игнорируем
		if (   (lpSubKeyTmp[0] != 'S' && lpSubKeyTmp[0] != 's')
			|| (lpSubKeyTmp[1] != 'O' && lpSubKeyTmp[1] != 'o')
			|| (lpSubKeyTmp[2] != 'F' && lpSubKeyTmp[2] != 'f')
			|| (lpSubKeyTmp[3] != 'T' && lpSubKeyTmp[3] != 't')
			|| (lpSubKeyTmp[4] != 'W' && lpSubKeyTmp[4] != 'w')
			|| (lpSubKeyTmp[5] != 'A' && lpSubKeyTmp[5] != 'a')
			|| (lpSubKeyTmp[6] != 'R' && lpSubKeyTmp[6] != 'r')
			|| (lpSubKeyTmp[7] != 'E' && lpSubKeyTmp[7] != 'e')
			|| (lpSubKeyTmp[8] != '\\' && lpSubKeyTmp[8] != 0) )
		{
			rkt = RKT_None;
			return true; // продолжить с обычным реестром
		}

		//if (lpSubKeyTmp[8] == '\\' && lpSubKeyTmp[9])
		//{
		//	rkt = RKT_None; // подключи "Software\<Name>" сохранять не нужно
		//}
		switch (rkt)
		{
			// Значит открывается копия ключа "Software", нужно будет его сохранить
			case RKT_HKCU:
				hKeyTmp = ghNewHKCUSoftware;
				rkt = RKT_HKCU_Software;
				break;
			case RKT_HKLM32:
				hKeyTmp = ghNewHKLM32Software;
				rkt = RKT_HKLM32_Software;
				break;
			case RKT_HKLM64:
				hKeyTmp = ghNewHKLM64Software;
				rkt = RKT_HKLM64_Software;
				break;
			default:
				_ASSERTE(rkt==RKT_HKCU || rkt==RKT_HKLM32 || rkt==RKT_HKLM64);
				rkt = RKT_None;
		}

		// Если открывается копия ключа "Software"
		if ((lpSubKeyTmp[8] == 0) || (lpSubKeyTmp[8] == '\\' && lpSubKeyTmp[9] == 0))
		{
			// Сразу выйдем!
			_ASSERTE(rkt==RKT_HKCU_Software || rkt==RKT_HKLM32_Software || rkt==RKT_HKLM64_Software);
			return true;
		}

		lpSubKeyTmp += 9;
	}
	else 
	{
		if ((rkt < RKT_HKCU_Software) || (rkt > RKT_HKLM64_Software))
		{
			// Этого быть не должно - все ветки уже обработаны
			_ASSERTE(rkt>=RKT_HKCU_Software && rkt<=RKT_HKLM64_Software);
			rkt = RKT_None;
			return true;
		}

		// Раз попали сюда - значит hKey - это "Software"
		// Если просто открывается его копия - сразу выйдем
		if (!lpSubKeyTmp || !*lpSubKeyTmp)
		{
			// Сразу выйдем! rkt указан, будет сохранен
			return true;
		}
		switch (rkt)
		{
			case RKT_HKCU_Software:
				hKeyTmp = ghNewHKCUSoftware;
				break;
			case RKT_HKLM32_Software:
				hKeyTmp = ghNewHKLM32Software;
				break;
			case RKT_HKLM64_Software:
				hKeyTmp = ghNewHKLM64Software;
				break;
		}
	}

	// то, что не "Far..." нас не интересует
	if (   (lpSubKeyTmp[0] != 'F' && lpSubKeyTmp[0] != 'f')
		|| (lpSubKeyTmp[1] != 'A' && lpSubKeyTmp[1] != 'a')
		|| (lpSubKeyTmp[2] != 'R' && lpSubKeyTmp[2] != 'r') )
	{
		rkt = RKT_None;
		return true;
	}

	// Теперь - проверяем точно
	LPCSTR pszDot = NULL;
	if (   (lpSubKeyTmp[3] == 0 || lpSubKeyTmp[3] == '\\' || *(pszDot = lpSubKeyTmp+3) == '.') // Far1
		|| (lpSubKeyTmp[3] == '2' && (lpSubKeyTmp[4] == 0 || lpSubKeyTmp[4] == '\\' || *(pszDot = lpSubKeyTmp+4) == '.')) // Far2 //-V112
		|| (lpSubKeyTmp[3] == ' '
		&& (lpSubKeyTmp[4] == 'M' || lpSubKeyTmp[4] == 'm')
		&& (lpSubKeyTmp[5] == 'A' || lpSubKeyTmp[5] == 'a')
		&& (lpSubKeyTmp[6] == 'N' || lpSubKeyTmp[6] == 'n')
		&& (lpSubKeyTmp[7] == 'A' || lpSubKeyTmp[7] == 'a')
		&& (lpSubKeyTmp[8] == 'G' || lpSubKeyTmp[8] == 'g')
		&& (lpSubKeyTmp[9] == 'E' || lpSubKeyTmp[9] == 'e')
		&& (lpSubKeyTmp[10] == 'R' || lpSubKeyTmp[10] == 'r')
		&& (lpSubKeyTmp[11] == 0 || lpSubKeyTmp[11] == '\\' || *(pszDot = lpSubKeyTmp+11) == '.'))
		)
	{
		if (pszDot && *pszDot == '.')
		{
			//			// lpSubKeyTmp содержит дополнительный путь, нужно выделить память и дописать в конец
			//			lpTemp = (char*)calloc(MAX_HOOKED_PATH+8+nSrcLen,sizeof(char));
			//			if (!lpTemp)
			//			{
			//				_ASSERTE(lpTemp!=NULL);
			//				return false;
			//			}
			//			_strcpy_c(lpTemp, MAX_HOOKED_PATH+8, gpRegKeyHooks[i].sNewPath);
			//			_strcat_c(lpTemp, nSrcLen, lpSubKeyTmp+nLen);
			int nGuidLen = 38;
			// !!! ANSI !!!
			if (memcmp(pszDot+1, VIRTUAL_REGISTRY_GUID_A, nGuidLen)!=0)
				return true; // не он
			// Выбросить из lpSubKey GUID для обращения к реальному реестру
			int nLen = lstrlenA(lpSubKey);
			size_t nBeginLen = (pszDot - lpSubKey);
			lpTemp = (char*)malloc(nLen*sizeof(char));
			// !!! UNICODE !!!
			memmove(lpTemp, lpSubKey, nBeginLen*sizeof(char));
			memmove(lpTemp+nBeginLen, pszDot+nGuidLen+1, (nLen-nGuidLen)*sizeof(char));
			// hKey не меняем
			lpSubKey = lpTemp;
			return true;
		}
		// OK, заменяем на виртуальный ключ!
		hKey = hKeyTmp;
		lpSubKey = lpSubKeyTmp;
	}
			
	// Сохранять не нужно
	rkt = RKT_None;
	return true;
	
	//int nSrcLen = lstrlenA(lpSubKeyTmp);
	//char sTest[MAX_HOOKED_PATH+1];
	//for (UINT i = 0; gpRegKeyHooks[i].hkRoot; i++)
	//{
	//	if (gpRegKeyHooks[i].hkRoot != hKey)
	//	{
	//		WARNING("Проигнорируются ключи, открытые ранее как RegOpenKey(HKCU,'SOFTWARE') и т.п.");
	//		continue;
	//	}
	//	int nLen = gpRegKeyHooks[i].nHookedLen;
	//	if (nLen > nSrcLen)
	//		continue; // не перехыватывается
	//	if (lpSubKeyTmp[nLen] != 0 && lpSubKeyTmp[nLen] != '\\')
	//		continue; // точно не этот ключ
	//	lstrcpynA(sTest, lpSubKeyTmp, nLen+1);
	//	sTest[nLen] = 0;
	//	if (lstrcmpiA(sTest, gpRegKeyHooks[i].sHooked) == 0)
	//	{
	//		if (lpSubKeyTmp[nLen] == 0)
	//		{
	//			// все просто - заменяем
	//			hKey = ghNewKeyRoot;
	//			lpSubKey = gpRegKeyHooks[i].sNewPath;
	//		}
	//		else
	//		{
	//			// lpSubKeyTmp содержит дополнительный путь, нужно выделить память и дописать в конец
	//			lpTemp = (char*)calloc(MAX_HOOKED_PATH+8+nSrcLen,sizeof(char));
	//			if (!lpTemp)
	//			{
	//				_ASSERTE(lpTemp!=NULL);
	//				return false;
	//			}
	//			_strcpy_c(lpTemp, MAX_HOOKED_PATH+8, gpRegKeyHooks[i].sNewPath);
	//			_strcat_c(lpTemp, nSrcLen, lpSubKeyTmp+nLen);
	//			hKey = ghNewKeyRoot;
	//			lpSubKey = lpTemp;
	//		}
	//		return true; // закончили поиск
	//	}
	//}
	//
	//// Разрешаем открыть реестр
	//return true;
}

bool CheckKeyHookedW(HKEY& hKey, LPCWSTR& lpSubKey, LPWSTR& lpTemp, RegKeyType& rkt, DWORD wowOptions=0 /*KEY_WOW64_64KEY/KEY_WOW64_32KEY*/)
{
	_ASSERTE(lpTemp == NULL);
	lpTemp = NULL;
	rkt = RKT_None;
	if (!hKey || !gpRegKeyStore)
		return true;
	RegKeyHook* p = HooksRegistryPtr();
	if (!p)
		return true;

	if (hKey == ghNewKeyRoot || hKey == ghNewHKCU || hKey == ghNewHKLM32 || hKey == ghNewHKLM64
		|| hKey == ghNewHKCUSoftware || hKey == ghNewHKLM32Software || hKey == ghNewHKLM64Software)
	{
		// Это УЖЕ перехваченный ключ, проверять не нужно
		return true;
	}

#ifdef _DEBUG
	wchar_t szDbgMsg[1024];
	msprintf(szDbgMsg, countof(szDbgMsg), L"ConEmuHk: Checking key (0x%08X, '", (DWORD)hKey);
	wcscat_c(szDbgMsg, lpSubKey ? lpSubKey : L"");
	wcscat_c(szDbgMsg, L"')\n");
	LPCWSTR pszHighlight = lpSubKey ? wcsstr(lpSubKey, L"Highlight") : NULL;
	LPCWSTR pszGroup = lpSubKey ? wcsstr(lpSubKey, L"Group0") : NULL;
	LPCWSTR pszFar2 = lpSubKey ? wcsstr(lpSubKey, L"Far2") : NULL;
	DEBUGSTR(szDbgMsg);
#endif

	LPCWSTR lpSubKeyTmp = lpSubKey;
	HKEY hKeyTmp = hKey;

	rkt = gpRegKeyStore->IsStored(hKey, wowOptions/*KEY_WOW64_64KEY/KEY_WOW64_32KEY*/);
	if (!rkt) // rkt == RKT_None
		return true;

	if (rkt >= RKT_HKCU && rkt <= RKT_HKLM64)
	{
		// копия HKCU или HKLM?
		if (!lpSubKeyTmp || !*lpSubKeyTmp)
		{
			return true; // нужно будет дернуть gpRegKeyStore->Store после успешного открытия
		}
		// Если lpSubKeyTmp НЕ начинается с "Software\\" - игнорируем
		if (   (lpSubKeyTmp[0] != 'S' && lpSubKeyTmp[0] != 's')
			|| (lpSubKeyTmp[1] != 'O' && lpSubKeyTmp[1] != 'o')
			|| (lpSubKeyTmp[2] != 'F' && lpSubKeyTmp[2] != 'f')
			|| (lpSubKeyTmp[3] != 'T' && lpSubKeyTmp[3] != 't')
			|| (lpSubKeyTmp[4] != 'W' && lpSubKeyTmp[4] != 'w')
			|| (lpSubKeyTmp[5] != 'A' && lpSubKeyTmp[5] != 'a')
			|| (lpSubKeyTmp[6] != 'R' && lpSubKeyTmp[6] != 'r')
			|| (lpSubKeyTmp[7] != 'E' && lpSubKeyTmp[7] != 'e')
			|| (lpSubKeyTmp[8] != '\\' && lpSubKeyTmp[8] != 0) )
		{
			rkt = RKT_None;
			return true; // продолжить с обычным реестром
		}

		//if (lpSubKeyTmp[8] == '\\' && lpSubKeyTmp[9])
		//{
		//	rkt = RKT_None; // подключи "Software\<Name>" сохранять не нужно
		//}
		switch (rkt)
		{
			// Значит открывается копия ключа "Software", нужно будет его сохранить
			case RKT_HKCU:
				hKeyTmp = ghNewHKCUSoftware;
				rkt = RKT_HKCU_Software;
				break;
			case RKT_HKLM32:
				hKeyTmp = ghNewHKLM32Software;
				rkt = RKT_HKLM32_Software;
				break;
			case RKT_HKLM64:
				hKeyTmp = ghNewHKLM64Software;
				rkt = RKT_HKLM64_Software;
				break;
			default:
				_ASSERTE(rkt==RKT_HKCU || rkt==RKT_HKLM32 || rkt==RKT_HKLM64);
				rkt = RKT_None;
		}

		// Если открывается копия ключа "Software"
		if ((lpSubKeyTmp[8] == 0) || (lpSubKeyTmp[8] == '\\' && lpSubKeyTmp[9] == 0))
		{
			// Сразу выйдем!
			_ASSERTE(rkt==RKT_HKCU_Software || rkt==RKT_HKLM32_Software || rkt==RKT_HKLM64_Software);
			return true;
		}

		lpSubKeyTmp += 9;
	}
	else 
	{
		if ((rkt < RKT_HKCU_Software) || (rkt > RKT_HKLM64_Software))
		{
			// Этого быть не должно - все ветки уже обработаны
			_ASSERTE(rkt>=RKT_HKCU_Software && rkt<=RKT_HKLM64_Software);
			rkt = RKT_None;
			return true;
		}

		// Раз попали сюда - значит hKey - это "Software"
		// Если просто открывается его копия - сразу выйдем
		if (!lpSubKeyTmp || !*lpSubKeyTmp)
		{
			// Сразу выйдем! rkt указан, будет сохранен
			return true;
		}
		switch (rkt)
		{
			case RKT_HKCU_Software:
				hKeyTmp = ghNewHKCUSoftware;
				break;
			case RKT_HKLM32_Software:
				hKeyTmp = ghNewHKLM32Software;
				break;
			case RKT_HKLM64_Software:
				hKeyTmp = ghNewHKLM64Software;
				break;
		}
	}

	// то, что не "Far..." нас не интересует
	if (   (lpSubKeyTmp[0] != 'F' && lpSubKeyTmp[0] != 'f')
		|| (lpSubKeyTmp[1] != 'A' && lpSubKeyTmp[1] != 'a')
		|| (lpSubKeyTmp[2] != 'R' && lpSubKeyTmp[2] != 'r') )
	{
		rkt = RKT_None;
		return true;
	}

	// Теперь - проверяем точно
	LPCWSTR pszDot = NULL;
	if (   (lpSubKeyTmp[3] == 0 || lpSubKeyTmp[3] == '\\' || *(pszDot = lpSubKeyTmp+3) == '.') // Far1
		|| (lpSubKeyTmp[3] == '2' && (lpSubKeyTmp[4] == 0 || lpSubKeyTmp[4] == '\\' || *(pszDot = lpSubKeyTmp+4) == '.')) // Far2 //-V112
		|| (lpSubKeyTmp[3] == ' '
			&& (lpSubKeyTmp[4] == 'M' || lpSubKeyTmp[4] == 'm')
			&& (lpSubKeyTmp[5] == 'A' || lpSubKeyTmp[5] == 'a')
			&& (lpSubKeyTmp[6] == 'N' || lpSubKeyTmp[6] == 'n')
			&& (lpSubKeyTmp[7] == 'A' || lpSubKeyTmp[7] == 'a')
			&& (lpSubKeyTmp[8] == 'G' || lpSubKeyTmp[8] == 'g')
			&& (lpSubKeyTmp[9] == 'E' || lpSubKeyTmp[9] == 'e')
			&& (lpSubKeyTmp[10] == 'R' || lpSubKeyTmp[10] == 'r')
			&& (lpSubKeyTmp[11] == 0 || lpSubKeyTmp[11] == '\\' || *(pszDot = lpSubKeyTmp+11) == '.'))
		)
	{
		if (pszDot && *pszDot == '.')
		{
			//			// lpSubKeyTmp содержит дополнительный путь, нужно выделить память и дописать в конец
			//			lpTemp = (char*)calloc(MAX_HOOKED_PATH+8+nSrcLen,sizeof(char));
			//			if (!lpTemp)
			//			{
			//				_ASSERTE(lpTemp!=NULL);
			//				return false;
			//			}
			//			_strcpy_c(lpTemp, MAX_HOOKED_PATH+8, gpRegKeyHooks[i].sNewPath);
			//			_strcat_c(lpTemp, nSrcLen, lpSubKeyTmp+nLen);
			int nGuidLen = 38;
			// !!! UNICODE !!!
			if (memcmp(pszDot+1, VIRTUAL_REGISTRY_GUID, nGuidLen*2)!=0)
				return true; // не он
			// Выбросить из lpSubKey GUID для обращения к реальному реестру
			int nLen = lstrlen(lpSubKey);
			size_t nBeginLen = (pszDot - lpSubKey);
			lpTemp = (wchar_t*)malloc(nLen*sizeof(wchar_t));
			// !!! UNICODE !!!
			memmove(lpTemp, lpSubKey, nBeginLen*sizeof(wchar_t));
			memmove(lpTemp+nBeginLen, pszDot+nGuidLen+1, (nLen-nGuidLen)*sizeof(wchar_t));
			// hKey не меняем
			lpSubKey = lpTemp;
			return true;
		}
		// OK, заменяем на виртуальный ключ!
		hKey = hKeyTmp;
		lpSubKey = lpSubKeyTmp;
	}
			
	// Сохранять не нужно
	rkt = RKT_None;
	return true;
}

#define ORIGINALFASTREGA(n,k,s,wow) \
	ORIGINALFASTEX(n,NULL); \
	if (!F(n)) \
		return ERROR_INVALID_FUNCTION; \
	LPSTR lpTemp1 = NULL; RegKeyType rkt = RKT_None; \
	if (!CheckKeyHookedA(k, s, lpTemp1, rkt, wow)) \
	{ \
		_ASSERTE(FALSE); \
		return ERROR_NOT_ENOUGH_MEMORY; \
	} \

#define ORIGINALFASTREGOPENA(n,k,s,wow) \
	ORIGINALFASTREGA(n,k,s,wow);
	//bool isSoftware = (s && IsOurRoot(k) && (s[0] == 'S' || s[0] == 's') && (lstrcmpiA(s, "Software") == 0));
	
#define ORIGINALFASTREGW(n,k,s,wow) \
	ORIGINALFASTEX(n,NULL); \
	if (!F(n)) \
		return ERROR_INVALID_FUNCTION; \
	LPWSTR lpTemp1 = NULL; RegKeyType rkt = RKT_None; \
	if (!CheckKeyHookedW(k, s, lpTemp1, rkt, wow)) \
	{ \
		_ASSERTE(FALSE); \
		return ERROR_NOT_ENOUGH_MEMORY; \
	}
	
#define ORIGINALFASTREGOPENW(n,k,s,wow) \
	ORIGINALFASTREGW(n,k,s,wow);
	//RegKeyType rktSoftware = (rkt && (
	//	((rkt>=RKT_HKCU && rkt<=RKT_HKLM64) && (s[0] == L'S' || s[0] == L's') && (lstrcmpi(s, L"Software") == 0));

#define FINALIZEREG(l) \
	if (lpTemp1) \
		free(lpTemp1);

#ifdef _DEBUG
	#define PRINTREGRESULT(l,k) if (l==0) { wchar_t szDbg[1024]; msprintf(szDbg, countof(szDbg), L"ConEmuHk: KeyResult(%s)=0x%08X\n", lpSubKey ? lpSubKey : L"", (DWORD)k); DEBUGSTR(szDbg); }
	#define PRINTCLOSE(k) if (!IsPredefined(k)) { wchar_t szDbg[128]; msprintf(szDbg, countof(szDbg), L"ConEmuHk: RegClose(0x%08X)\n", (DWORD)k); DEBUGSTR(szDbg); }
#else
	#define PRINTREGRESULT(l,k)
	#define PRINTCLOSE(k)
#endif

#define FINALIZEREGOPENA(l,r,h) \
	FINALIZEREG(l); \
	if ((l == 0) && rkt && gpRegKeyStore) gpRegKeyStore->Store(h,rkt);

#define FINALIZEREGOPENW(l,r,h) \
	FINALIZEREG(l); PRINTREGRESULT(l,h); \
	if ((l == 0) && rkt && gpRegKeyStore) gpRegKeyStore->Store(h,rkt);

	
/* ******************** */
/* *** RegCreateKey *** */
/* ******************** */
typedef LONG (WINAPI* OnRegCloseKey_t)(HKEY hKey);
LONG WINAPI OnRegCloseKey(HKEY hKey)
{
	LONG lRc = -1;
	ORIGINALFASTEX(RegCloseKey,NULL);
	if (hKey && gpRegKeyStore)
	{
		PRINTCLOSE(hKey);
		gpRegKeyStore->Remove(hKey);
	}
	if (!F(RegCloseKey))
		return ERROR_INVALID_FUNCTION;
	lRc = F(RegCloseKey)(hKey);
	return lRc;
}
	
	
/* ******************** */
/* *** RegCreateKey *** */
/* ******************** */

typedef LONG(WINAPI* OnRegCreateKeyA_t)(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
LONG WINAPI OnRegCreateKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
{
	LONG lRc = -1;
	ORIGINALFASTREGOPENA(RegCreateKeyA,hKey,lpSubKey,0);
	lRc = F(RegCreateKeyA)(hKey, lpSubKey, phkResult);
	FINALIZEREGOPENA(lRc,hKey,*phkResult);
	return lRc;
}

typedef LONG(WINAPI* OnRegCreateKeyW_t)(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);
LONG WINAPI OnRegCreateKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult)
{
	LONG lRc = -1;
	ORIGINALFASTREGOPENW(RegCreateKeyW,hKey,lpSubKey,0);
	lRc = F(RegCreateKeyW)(hKey, lpSubKey, phkResult);
	FINALIZEREGOPENW(lRc,hKey,*phkResult);
	return lRc;
}


/* ********************** */
/* *** RegCreateKeyEx *** */
/* ********************** */

typedef LONG(WINAPI* OnRegCreateKeyExA_t)(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
LONG WINAPI OnRegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
	LONG lRc = -1;
	ORIGINALFASTREGOPENA(RegCreateKeyExA,hKey,lpSubKey,(samDesired&(KEY_WOW64_64KEY|KEY_WOW64_32KEY)));
	lRc = F(RegCreateKeyExA)(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
	FINALIZEREGOPENA(lRc,hKey,*phkResult);
	return lRc;
}

typedef LONG(WINAPI* OnRegCreateKeyExW_t)(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
LONG WINAPI OnRegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
	LONG lRc = -1;
	ORIGINALFASTREGOPENW(RegCreateKeyExW,hKey,lpSubKey,(samDesired&(KEY_WOW64_64KEY|KEY_WOW64_32KEY)));
	lRc = F(RegCreateKeyExW)(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
	FINALIZEREGOPENW(lRc,hKey,*phkResult);
	return lRc;
}


/* ****************** */
/* *** RegOpenKey *** */
/* ****************** */

typedef LONG(WINAPI* OnRegOpenKeyA_t)(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
LONG WINAPI OnRegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
{
	LONG lRc = -1;
	ORIGINALFASTREGOPENA(RegOpenKeyA,hKey,lpSubKey,0);
	lRc = F(RegOpenKeyA)(hKey, lpSubKey, phkResult);
	FINALIZEREGOPENA(lRc,hKey,*phkResult);
	return lRc;
}

typedef LONG(WINAPI* OnRegOpenKeyW_t)(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);
LONG WINAPI OnRegOpenKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult)
{
	LONG lRc = -1;
	ORIGINALFASTREGOPENW(RegOpenKeyW,hKey,lpSubKey,0);
	lRc = F(RegOpenKeyW)(hKey, lpSubKey, phkResult);
	FINALIZEREGOPENW(lRc,hKey,*phkResult);
	return lRc;
}


/* ******************** */
/* *** RegOpenKeyEx *** */
/* ******************** */

typedef LONG(WINAPI* OnRegOpenKeyExA_t)(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
LONG WINAPI OnRegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
	LONG lRc = -1;
	ORIGINALFASTREGOPENA(RegOpenKeyExA,hKey,lpSubKey,(samDesired&(KEY_WOW64_64KEY|KEY_WOW64_32KEY)));
	lRc = F(RegOpenKeyExA)(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	FINALIZEREGOPENA(lRc,hKey,*phkResult);
	return lRc;
}

typedef LONG(WINAPI* OnRegOpenKeyExW_t)(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
LONG WINAPI OnRegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
	LONG lRc = -1;
	ORIGINALFASTREGOPENW(RegOpenKeyExW,hKey,lpSubKey,(samDesired&(KEY_WOW64_64KEY|KEY_WOW64_32KEY)));
#ifdef _DEBUG
	HKEY hkDbg = NULL;
	if (hkDbg)
	{
		wchar_t szKey1[128], szKey2[128]; DWORD n;
		typedef LSTATUS (APIENTRY* RegEnumKeyExW_t)(HKEY hKey,DWORD dwIndex,LPWSTR lpName,LPDWORD lpcchName,LPDWORD lpReserved,LPWSTR lpClass,LPDWORD lpcchClass,PFILETIME lpftLastWriteTime);
		RegEnumKeyExW_t fEnum = (RegEnumKeyExW_t)GetProcAddress(ghAdvapi32, "RegEnumKeyExW");
		LONG l1, l2;
		if (fEnum)
		{
			l1 = fEnum(hkDbg, 0, szKey1, &(n=countof(szKey1)), 0,0,0,0);
			l2 = fEnum(hkDbg, 1, szKey2, &(n=countof(szKey2)), 0,0,0,0);
		}
		n = 0;
	}
#endif
	lRc = F(RegOpenKeyExW)(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	FINALIZEREGOPENW(lRc,hKey,*phkResult);
	return lRc;
}


/* ******************** */
/* *** RegDeleteKey *** */
/* ******************** */

typedef LONG(WINAPI* OnRegDeleteKeyA_t)(HKEY hKey, LPCSTR lpSubKey);
LONG WINAPI OnRegDeleteKeyA(HKEY hKey, LPCSTR lpSubKey)
{
	LONG lRc = -1;
	ORIGINALFASTREGA(RegDeleteKeyA,hKey,lpSubKey,0);
	lRc = F(RegDeleteKeyA)(hKey, lpSubKey);
	FINALIZEREG(lRc);
	return lRc;
}

typedef LONG(WINAPI* OnRegDeleteKeyW_t)(HKEY hKey, LPCWSTR lpSubKey);
LONG WINAPI OnRegDeleteKeyW(HKEY hKey, LPCWSTR lpSubKey)
{
	LONG lRc = -1;
	ORIGINALFASTREGW(RegDeleteKeyW,hKey,lpSubKey,0);
	lRc = F(RegDeleteKeyW)(hKey, lpSubKey);
	FINALIZEREG(lRc);
	return lRc;
}



/* ************************** */
/* *** RegConnectRegistry *** */
/* ************************** */

typedef LONG (WINAPI* OnRegConnectRegistryA_t)(LPCSTR lpMachineName, HKEY hKey, PHKEY phkResult);
LONG WINAPI OnRegConnectRegistryA(LPCSTR lpMachineName, HKEY hKey, PHKEY phkResult)
{
	LONG lRc = -1;
	ORIGINALFASTEX(RegConnectRegistryA,NULL);
	lRc = F(RegConnectRegistryA)(lpMachineName, hKey, phkResult);
	if ((lRc == 0)
		&& (!lpMachineName || !*lpMachineName)
		&& gpRegKeyStore && phkResult
		&& ((hKey == HKEY_CURRENT_USER) || (hKey == HKEY_LOCAL_MACHINE)))
	{
		RegKeyType rkt;
		if (hKey == HKEY_CURRENT_USER)
		{
			rkt = RKT_HKCU;
		}
		else
		{
			#ifdef _WIN64
			rkt = RKT_HKLM64;
			#else
			rkt = RKT_HKLM32;
			#endif
		}
		gpRegKeyStore->Store(*phkResult, rkt);
	}
	return lRc;
}

typedef LONG (WINAPI* OnRegConnectRegistryW_t)(LPCWSTR lpMachineName, HKEY hKey, PHKEY phkResult);
LONG WINAPI OnRegConnectRegistryW(LPCWSTR lpMachineName, HKEY hKey, PHKEY phkResult)
{
	LONG lRc = -1;
	ORIGINALFASTEX(RegConnectRegistryW,NULL);
	lRc = F(RegConnectRegistryW)(lpMachineName, hKey, phkResult);
	if ((lRc == 0)
		&& (!lpMachineName || !*lpMachineName)
		&& gpRegKeyStore && phkResult
		&& ((hKey == HKEY_CURRENT_USER) || (hKey == HKEY_LOCAL_MACHINE)))
	{
		RegKeyType rkt;
		if (hKey == HKEY_CURRENT_USER)
		{
			rkt = RKT_HKCU;
		}
		else
		{
			#ifdef _WIN64
			rkt = RKT_HKLM64;
			#else
			rkt = RKT_HKLM32;
			#endif
		}
		gpRegKeyStore->Store(*phkResult, rkt);
	}
	return lRc;
}
#endif
