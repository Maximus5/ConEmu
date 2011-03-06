
// Road map
// * RegEnumKey/RegEnumKeyEx
// * wow64 support.
// * OnRegOpenKeyTransacted
// * RegCopyTree
// * RegDeleteKey/RegDeleteKeyEx/RegDeleteKeyTransacted/RegDeleteKeyValue/RegDeleteTree

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#define DEFINE_HOOK_MACROS

#define SETCONCP_READYTIMEOUT 5000
#define SETCONCP_TIMEOUT 1000

#include <windows.h>
#include <TCHAR.h>
#include <Tlhelp32.h>
#include <shlwapi.h>
#include "..\common\common.hpp"
#include "..\common\ConEmuCheck.h"
#include "SetHook.h"
#include "..\common\execute.h"
#include "ConEmuHooks.h"


static LONG WINAPI OnRegCreateKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
static LONG WINAPI OnRegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
static LONG WINAPI OnRegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
static LONG WINAPI OnRegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
static LONG WINAPI OnRegCreateKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);
static LONG WINAPI OnRegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
static LONG WINAPI OnRegOpenKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);
static LONG WINAPI OnRegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
// Vista+
//static LONG WINAPI OnRegOpenKeyTransacted(HKEY hKey, LPCTSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult, HANDLE hTransaction, PVOID pExtendedParameter);



static TCHAR advapi32[]  = _T("Advapi32.dll");

static HookItem HooksRegistry[] =
{
	/* ***** MOST CALLED ***** */
	{(void*)OnRegCreateKeyA,	"RegCreateKeyA",	advapi32},
	{(void*)OnRegCreateKeyExA,	"RegCreateKeyExA",	advapi32},
	{(void*)OnRegOpenKeyA,		"RegOpenKeyA",		advapi32},
	{(void*)OnRegOpenKeyExA,	"RegOpenKeyExA",	advapi32},
	{(void*)OnRegCreateKeyW,	"RegCreateKeyW",	advapi32},
	{(void*)OnRegCreateKeyExW,	"RegCreateKeyExW",	advapi32},
	{(void*)OnRegOpenKeyW,		"RegOpenKeyW",		advapi32},
	{(void*)OnRegOpenKeyExW,	"RegOpenKeyExW",	advapi32},
	/* ************************ */
	{0}
};



struct RegKeyHook
{
	// Что перехватываем
	HKEY    hkRoot;
	wchar_t wsHooked[64];  // для W-функций
	char    sHooked[64];   // для A-функций
	int     nHookedLen;
	wchar_t wsHookedU[64];  // для сравнения в W-функциях
	char    sHookedU[64];   // для сравнения в A-функциях
	// Что показываем в RegEnumKey, вместо реальных ключей
	wchar_t wsGhost[128];  // для W-функций
	char    sGhost[128];   // для A-функций
	int     nGhostLen;
	wchar_t wsGhostU[128]; // для сравнения в W-функциях
	char    sGhostU[128];  // для сравнения в A-функциях
	// Для упрощения обработки - переопределенные пути
	HKEY    hkNewKey;
	wchar_t wsNewPath[96]; // для W-функций
	char    sNewPath[96];  // для A-функций
};
static RegKeyHook RegKeyHooks[] =
{
	{HKEY_LOCAL_MACHINE, L"SOFTWARE\\Far"},
	{HKEY_LOCAL_MACHINE, L"SOFTWARE\\Far2"},
	{HKEY_LOCAL_MACHINE, L"SOFTWARE\\Far Manager"},
	{HKEY_CURRENT_USER,  L"SOFTWARE\\Far"},
	{HKEY_CURRENT_USER,  L"SOFTWARE\\Far2"},
	{HKEY_CURRENT_USER,  L"SOFTWARE\\Far Manager"},
	{NULL}
};

static HKEY ghNewKeyRoot = NULL;

HookItem* HooksRegistryPtr()
{
	if (ghNewKeyRoot == NULL)
	{
		HWND hConWnd = GetConsoleWindow();
		if (hConWnd)
		{
			MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> ConMap;
			ConMap.InitName(CECONMAPNAME, (DWORD)hConWnd);
			CESERVER_CONSOLE_MAPPING_HDR* pHdr = ConMap.Open();
			wchar_t szDbgErr[255];
			if (!pHdr)
			{
				_ASSERTE(pHdr!=NULL);
				_wsprintf(szDbgErr, SKIPLEN(countof(szDbgErr)) 
					L"!!! HooksRegistryPtr(PID=%u): Retrieving console settings failed!\n",
					GetCurrentProcessId());
				OutputDebugString(szDbgErr);
				OutputDebugString(ConMap.GetErrorText());
				OutputDebugString(L"\n");
			}
			else if (pHdr->nProtocolVersion != CESERVER_REQ_VER)
			{
				_ASSERTE(pHdr->nProtocolVersion == CESERVER_REQ_VER);
				_wsprintf(szDbgErr, SKIPLEN(countof(szDbgErr)) 
					L"!!! HooksRegistryPtr(PID=%u): Unsupported protocol version: %u, required: %u!\n",
					pHdr->nProtocolVersion, CESERVER_REQ_VER);
				OutputDebugString(szDbgErr);
			}
			else if (!pHdr->bHookRegistry)
			{
				OutputDebugString(L"HooksRegistryPtr: Registry hooks now allowed\n");
			}
			else
			{
				// Можно
			}
		}
	}
	
	if (ghNewKeyRoot == NULL)
	{
		ghNewKeyRoot = (HKEY)-1;
		//PRAGMA_ERROR("Mount hive!");
	}
	
	if (ghNewKeyRoot && (ghNewKeyRoot != (HKEY)-1))
	{
		UUID Ghost = {0};
		wchar_t wsGhost[42] = L".{0}";
		char sGhost[42] = ".{0}";
		typedef long (__stdcall* UuidCreate_t)(UUID *Uuid);
		UuidCreate_t UuidCreate = NULL;
		HMODULE hRpcrt4 = LoadLibrary(L"Rpcrt4.dll");
		if (hRpcrt4)
		{
			if ((UuidCreate = (UuidCreate_t)GetProcAddress(hRpcrt4, "UuidCreate")) != NULL)
				UuidCreate(&Ghost);
			FreeLibrary(hRpcrt4);
			_wsprintf(wsGhost, SKIPLEN(countof(wsGhost)) L".{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
				Ghost.Data1, (DWORD)(WORD)Ghost.Data2, (DWORD)(WORD)Ghost.Data3,
				(DWORD)(BYTE)Ghost.Data4[0], (DWORD)(BYTE)Ghost.Data4[1], (DWORD)(BYTE)Ghost.Data4[2], (DWORD)(BYTE)Ghost.Data4[3],
				(DWORD)(BYTE)Ghost.Data4[4], (DWORD)(BYTE)Ghost.Data4[5], (DWORD)(BYTE)Ghost.Data4[6], (DWORD)(BYTE)Ghost.Data4[7]);
			_wsprintfA(sGhost, SKIPLEN(countof(sGhost)) ".{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
				Ghost.Data1, (DWORD)(WORD)Ghost.Data2, (DWORD)(WORD)Ghost.Data3,
				(DWORD)(BYTE)Ghost.Data4[0], (DWORD)(BYTE)Ghost.Data4[1], (DWORD)(BYTE)Ghost.Data4[2], (DWORD)(BYTE)Ghost.Data4[3],
				(DWORD)(BYTE)Ghost.Data4[4], (DWORD)(BYTE)Ghost.Data4[5], (DWORD)(BYTE)Ghost.Data4[6], (DWORD)(BYTE)Ghost.Data4[7]);
		}
						
		// Подготовить структуру RegKeyHooks к использованию
		for (UINT i = 0; i < countof(RegKeyHooks); i++)
		{
			// Длина перехватываемого ключа
			RegKeyHooks[i].nHookedLen = lstrlen(RegKeyHooks[i].wsHooked);
			WideCharToMultiByte(CP_ACP, 0, RegKeyHooks[i].wsHooked, RegKeyHooks[i].nHookedLen+1,
				RegKeyHooks[i].sHooked, RegKeyHooks[i].nHookedLen+1, 0,0);

			// Для удобства сравнения в uppercase
			wcscpy_c(RegKeyHooks[i].wsHookedU, RegKeyHooks[i].wsHooked);
			CharUpperBuffW(RegKeyHooks[i].wsHookedU, RegKeyHooks[i].nHookedLen);
			_strcpy_c(RegKeyHooks[i].sHookedU, countof(RegKeyHooks[i].sHookedU), RegKeyHooks[i].sHooked);
			CharUpperBuffA(RegKeyHooks[i].sHookedU, RegKeyHooks[i].nHookedLen);

			// Что показываем в RegEnumKey, вместо реальных ключей
			wcscpy_c(RegKeyHooks[i].wsGhost, RegKeyHooks[i].wsHooked);
			wcscat_c(RegKeyHooks[i].wsGhost, wsGhost);
			_strcpy_c(RegKeyHooks[i].sGhost, countof(RegKeyHooks[i].sGhost), RegKeyHooks[i].sHooked);
			_strcat_c(RegKeyHooks[i].sGhost, countof(RegKeyHooks[i].sGhost), sGhost);
			RegKeyHooks[i].nGhostLen = lstrlen(RegKeyHooks[i].wsGhost);
			wcscpy_c(RegKeyHooks[i].wsGhostU, RegKeyHooks[i].wsGhost);
			CharUpperBuffW(RegKeyHooks[i].wsGhostU, RegKeyHooks[i].nGhostLen);
			_strcpy_c(RegKeyHooks[i].sGhostU, countof(RegKeyHooks[i].sGhostU), RegKeyHooks[i].sGhost);
			CharUpperBuffA(RegKeyHooks[i].sGhostU, RegKeyHooks[i].nGhostLen);
			
			// Для упрощения обработки - переопределенные пути
			RegKeyHooks[i].hkNewKey = ghNewKeyRoot;
			_wsprintf(RegKeyHooks[i].wsNewPath, SKIPLEN(countof(RegKeyHooks[i].wsNewPath)) L"%s\\%s",
				(RegKeyHooks[i].hkRoot == HKEY_LOCAL_MACHINE) ? L"HKLM" : L"HKCU", RegKeyHooks[i].wsHooked);
			_wsprintfA(RegKeyHooks[i].sNewPath, SKIPLEN(countof(RegKeyHooks[i].sNewPath)) "%s\\%s",
				(RegKeyHooks[i].hkRoot == HKEY_LOCAL_MACHINE) ? L"HKLM" : L"HKCU", RegKeyHooks[i].sHooked);
		}
	}
	else
	{
		return NULL;
	}
	
	return HooksRegistry;
}


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

static void CheckKeyHookedA(HKEY& hKey, LPCSTR& lpSubKey)
{
}

static void CheckKeyHookedW(HKEY& hKey, LPCWSTR& lpSubKey)
{
}

typedef LONG(WINAPI* OnRegCreateKeyA_t)(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
static LONG WINAPI OnRegCreateKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
{
	ORIGINALFAST(RegCreateKeyA);
	LONG lRc = -1;
	CheckKeyHookedA(hKey, lpSubKey);
	lRc = F(RegCreateKeyA)(hKey, lpSubKey, phkResult);
	return lRc;
}

typedef LONG(WINAPI* OnRegCreateKeyExA_t)(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
static LONG WINAPI OnRegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
	ORIGINALFAST(RegCreateKeyExA);
	LONG lRc = -1;
	CheckKeyHookedA(hKey, lpSubKey);
	lRc = F(RegCreateKeyExA)(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
	return lRc;
}

typedef LONG(WINAPI* OnRegOpenKeyA_t)(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
static LONG WINAPI OnRegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
{
	ORIGINALFAST(RegOpenKeyA);
	LONG lRc = -1;
	CheckKeyHookedA(hKey, lpSubKey);
	lRc = F(RegOpenKeyA)(hKey, lpSubKey, phkResult);
	return lRc;
}

typedef LONG(WINAPI* OnRegOpenKeyExA_t)(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
static LONG WINAPI OnRegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
	ORIGINALFAST(RegOpenKeyExA);
	LONG lRc = -1;
	CheckKeyHookedA(hKey, lpSubKey);
	lRc = F(RegOpenKeyExA)(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	return lRc;
}

typedef LONG(WINAPI* OnRegCreateKeyW_t)(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);
static LONG WINAPI OnRegCreateKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult)
{
	ORIGINALFAST(RegCreateKeyW);
	LONG lRc = -1;
	CheckKeyHookedW(hKey, lpSubKey);
	lRc = F(RegCreateKeyW)(hKey, lpSubKey, phkResult);
	return lRc;
}

typedef LONG(WINAPI* OnRegCreateKeyExW_t)(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
static LONG WINAPI OnRegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
	ORIGINALFAST(RegCreateKeyExW);
	LONG lRc = -1;
	CheckKeyHookedW(hKey, lpSubKey);
	lRc = F(RegCreateKeyExW)(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
	return lRc;
}

typedef LONG(WINAPI* OnRegOpenKeyW_t)(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);
static LONG WINAPI OnRegOpenKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult)
{
	ORIGINALFAST(RegOpenKeyW);
	LONG lRc = -1;
	CheckKeyHookedW(hKey, lpSubKey);
	lRc = F(RegOpenKeyW)(hKey, lpSubKey, phkResult);
	return lRc;
}

typedef LONG(WINAPI* OnRegOpenKeyExW_t)(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
static LONG WINAPI OnRegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
	ORIGINALFAST(RegOpenKeyExW);
	LONG lRc = -1;
	CheckKeyHookedW(hKey, lpSubKey);
	lRc = F(RegOpenKeyExW)(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	return lRc;
}
