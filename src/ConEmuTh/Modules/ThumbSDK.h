
#pragma once

#include <pshpack1.h>

#define CET_CAN_PREVIEW  1
#define CET_CAN_LOADINFO 2

#define CET_CM_BGR  1
//#define CET_CM_BGRA 2

struct CET_Init {
	// [In]
	DWORD cbSize; // size of this structure
	HMODULE hModule; // Module handle of initializing module
	// [Out]
	DWORD nErrNumber;
	DWORD nFlags; // Combination of CET_CAN_xxx flags
	LPARAM lParam; // Module may use this value
};

struct CET_LoadPreview {
	// [In]
	DWORD cbSize; // size of this structure
	LPARAM lParam; // Module may use this value. It comes from CET_Init
	LPCWSTR sFileName; // Full file path name
	FILETIME ftModified; // Date of last file modification
	__int64 nFileSize; // Size of the file on disk
	COORD crLoadSize; // Required preview dimensions (in pixels)
	BOOL bTilesMode; // TRUE, when pszInfo acquired.
	COLORREF crBackground; // Must be used to fill background.
	// [Out]
	DWORD nErrNumber;
	COORD crSize; // Предпочтительно, должен совпадать с crLoadSize
	DWORD cbStride; // Bytes per line
	DWORD nBits; // 32 bit required!
	// [Out] Next fields MUST be LocalAlloc(LPTR)
	DWORD ColorModel; // One of CET_CM_xxx
	LPDWORD Pixels; // Alpha channel (highest byte) allowed.
	// Module can place here information about original image (dimension, format, etc.)
	// This must be double zero terminated string
	wchar_t *pszInfo; 
};

struct CET_LoadInfo {
	// [In]
	DWORD cbSize; // size of this structure
	LPARAM lParam; // Module may use this value. It comes from CET_Init
	LPCWSTR sFileName; // Full file path name
	FILETIME ftModified; // Date of last file modification
	__int64 nFileSize; // Size of the file on disk
	// [Out] Next field MUST be LocalAlloc(LPTR)
	DWORD nErrNumber;
	// Module must place here information about original file
	// This must be double zero terminated string
	/* Example:
	far.exe
	File and archive manager
	v2.0 (build 1503) x86
	2.85Mb, 15.04.10 13:01
	*/
	wchar_t *pszInfo; 
};

// Export name: CET_Init.
typedef BOOL (WINAPI* CET_Init_t)(struct CET_Init*);
// Export name: CET_Done.
typedef VOID (WINAPI* CET_Done_t)(struct CET_Init*);
// Export name: CET_Load. Argument may be CET_LoadPreview* or CET_LoadInfo*
typedef BOOL (WINAPI* CET_Load_t)(LPVOID);


#include <poppack.h>
