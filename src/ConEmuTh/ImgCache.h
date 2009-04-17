
#pragma once

#include <Objbase.h>
#include <Objidl.h>

#define ImgCacheFileName L"ConEmuTh.cache"
#define ImgCacheListName L"#LFN"

class CImgCache
{
public:
	wchar_t ms_CachePath[MAX_PATH];
	wchar_t ms_LastStoragePath[32768];
public:
	CImgCache();
	~CImgCache(void);
	void SetCacheLocation(LPCWSTR asCachePath);

protected:
	BOOL mb_Quit;
	static DWORD WINAPI ShellExtractionThread(LPVOID apArg);
	IStorage *mp_RootStorage, *mp_CurrentStorage;
	BOOL LoadPreview();
};
