
#pragma once

#include <pshpack1.h>

#define CET_CAN_PREVIEW  1
#define CET_CAN_LOADINFO 2

#define CET_CM_BGR  1
#define CET_CM_BGRA 2
#define CET_CM_ICON 3
#define CET_CM_PNG  4
#define CET_CM_BMP  5

enum
{
	MODULE_GDIP = 0x6764692B, // gdi+
	MODULE_PNG  = 0x00706E67, // png
	MODULE_ICO  = 0x0069366F, // ico
	MODULE_PE   = 0x00700065, // pe
	MODULE_PIC2 = 0x70696332, // pic2
	//
	MODULE_CUSTOM = 0x75736572 // ALL OTHER MODULES (Non standard)
};

struct CET_Init
{
	// [In]
	size_t cbSize; // size of this structure
	HMODULE hModule; // Module handle of initializing module
	// [Out]
	DWORD nModuleID; // gdi+ / png / ico / pe / pic2
	DWORD nErrNumber;
	DWORD nFlags; // Combination of CET_CAN_xxx flags
	LPVOID pContext; // Module may use this value
};

struct CET_LoadInfo
{
	// [In]
	size_t cbSize; // size of this structure
	LPVOID pContext; // Module may use this value. It comes from CET_Init
	LPCWSTR sFileName; // Full file path name
	FILETIME ftModified; // Date of last file modification
	__int64 nFileSize; // Size of the file on disk
	BOOL bVirtualItem; // TRUE - when NO physical file
	const BYTE* pFileData; // may be NULL
	COORD crLoadSize; // Required preview dimensions (in pixels)
	//int nMaxZoom; // In percents, i.e. 500%
	BOOL bTilesMode; // TRUE, when pszInfo acquired.
	COLORREF crBackground; // Must be used to fill background.
	LPARAM iArgument; // May be used internally
	// [Out]
	DWORD nErrNumber;
	COORD crSize; // Предпочтительно, должен совпадать с crLoadSize
	DWORD cbStride; // Bytes per line
	DWORD nBits; // 32 bit required!
	// [Out]
	LPVOID pFileContext;
	DWORD ColorModel; // One of CET_CM_xxx
	const DWORD *Pixels; // Alpha channel (highest byte) allowed.
	DWORD cbPixelsSize; // size in BYTES
	const wchar_t *pszComments; // This may be "512 x 232 x 32bpp"
	//// Module can place here information about original image (dimension, format, etc.)
	//// This must be double zero terminated string
	//// When this (pszInfo) specified - ConEmu'll ignore pszComments and draw pszInfo
	//// instead of it's internal information in Tiles mode.
	//const wchar_t *pszInfo;
	//DWORD cbInfoSize; // size in BYTES
};

//struct CET_LoadInfo {
//	// [In]
//	DWORD cbSize; // size of this structure
//	LPARAM lParam; // Module may use this value. It comes from CET_Init
//	LPCWSTR sFileName; // Full file path name
//	FILETIME ftModified; // Date of last file modification
//	__int64 nFileSize; // Size of the file on disk
//	// [Out] Next field MUST be LocalAlloc(LPTR)
//	DWORD nErrNumber;
//	// Module must place here information about original file
//	// This must be double zero terminated string
//	/* Example:
//	Far.exe
//	File and archive manager
//	v2.0 (build 1503) x86
//	2.85Mb, 15.04.10 13:01
//	*/
//	wchar_t *pszInfo;
//};

// Export name: CET_Init.
typedef BOOL (WINAPI* CET_Init_t)(struct CET_Init*);
// Export name: CET_Done.
typedef VOID (WINAPI* CET_Done_t)(struct CET_Init*);
// Export name: CET_Load.
typedef BOOL (WINAPI* CET_Load_t)(struct CET_LoadInfo*);
// Export name: CET_Free.
typedef VOID (WINAPI* CET_Free_t)(struct CET_LoadInfo*);
// Export name: CET_Cancel.
typedef VOID (WINAPI* CET_Cancel_t)(LPVOID pContext);

#include <poppack.h>
