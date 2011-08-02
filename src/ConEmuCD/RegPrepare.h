
#pragma once

#ifdef REGPREPARE_EXTERNAL

	typedef int (WINAPI* MountVirtualHive_t)(LPCWSTR asHive, PHKEY phKey, LPCWSTR asXPMountName, wchar_t* pszErrInfo, int cchErrInfoMax, BOOL* pbKeyMounted);
	typedef int (WINAPI* UnMountVirtualHive_t)(LPCWSTR asXPMountName, wchar_t* pszErrInfo, int cchErrInfoMax);
	
#else

	#if defined(__GNUC__)
	extern "C" {
	#endif

	int WINAPI MountVirtualHive(LPCWSTR asHive, PHKEY phKey, LPCWSTR asXPMountName, wchar_t* pszErrInfo, int cchErrInfoMax, BOOL* pbKeyMounted);
	int WINAPI UnMountVirtualHive(LPCWSTR asXPMountName, wchar_t* pszErrInfo, int cchErrInfoMax);
		
	#if defined(__GNUC__)
	};
	#endif

#endif
