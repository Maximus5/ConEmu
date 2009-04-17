
#include <stdio.h>
#include <windows.h>
#include <ole2.h>
#include <crtdbg.h>

#include "ImgCache.h"

#pragma comment( lib, "ole32.lib" )


#define SafeRelease(p) if ((p)!=NULL) { (p)->Release(); (p) = NULL; }

CImgCache::CImgCache()
{
	mb_Quit = FALSE;
	ms_CachePath[0] = ms_LastStoragePath[0] = 0;

	// Prepare root storage file pathname
	SetCacheLocation(NULL); // По умолчанию - в %TEMP%

	// Initialize interfaces
	mp_RootStorage = mp_CurrentStorage = NULL;

	// Initialize Com
	CoInitialize(NULL);
}

CImgCache::~CImgCache(void)
{
	HRESULT hr;
	if (mp_CurrentStorage) {
		hr = mp_CurrentStorage->Commit(STGC_DEFAULT);
		_ASSERTE(SUCCEEDED(hr));
		SafeRelease(mp_CurrentStorage);
	}
	if (mp_RootStorage) {
		hr = mp_RootStorage->Commit(STGC_DEFAULT);
		_ASSERTE(SUCCEEDED(hr));
		SafeRelease(mp_RootStorage);
	}
	// Done Com
	CoUninitialize();
}

void CImgCache::SetCacheLocation(LPCWSTR asCachePath)
{
	// Prepare root storage file pathname
	if (asCachePath) {
		lstrcpyn(ms_CachePath, asCachePath, MAX_PATH-32);
	} else {
		GetTempPath(MAX_PATH-32, ms_CachePath);
	}
	// add end slash
	int nLen = lstrlen(ms_CachePath);
	if (nLen && ms_CachePath[nLen-1] != L'\\') {
		ms_CachePath[nLen++] = L'\\'; ms_CachePath[nLen] = 0;
	}
	// add our storage file name
	lstrcpy(ms_CachePath+nLen, ImgCacheFileName);
}

DWORD CImgCache::ShellExtractionThread(LPVOID apArg)
{
	CImgCache *pImgCache = (CImgCache*)apArg;
	// Initialize Com
	CoInitialize(NULL);

	// Loop

	// Done Com
	CoUninitialize();
	return 0;
}

BOOL CImgCache::LoadPreview()
{
	return TRUE;
}
