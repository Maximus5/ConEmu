#pragma once

#if !defined(__GNUC__)
#pragma warning(disable: 4244) // convertion to lower size
#pragma warning(disable: 4267) // conversion from 'size_t' to 'DWORD'
#pragma warning(disable: 4146) // unary minus operator applied to unsigned type
#pragma warning(disable: 4800) // forcing value to bool 'true' or 'false'
#pragma warning(disable: 4733) // Inline asm assigning to 'FS:0' : handler not registered as safe handler
#pragma warning(disable: 4731) // frame pointer register 'ebp' modified by inline assembly code
#pragma warning(disable: 4996) // ... was declared deprecated
#endif

typedef unsigned int uint;
typedef unsigned __int8 byte;
typedef unsigned __int64 QWORD;
typedef __int8 i8;
typedef __int16 i16;
typedef __int32 i32;
typedef __int64 i64;
typedef unsigned __int8 u8;
typedef unsigned __int16 u16;
typedef unsigned __int64 u64;
typedef DWORD u32;

#if defined(__GNUC__) && !defined(__MINGW64_VERSION_MAJOR)
#define __forceinline __inline__
#endif
template <class T>__forceinline const T& klMin(const T &a, const T &b) {return a < b ? a : b;}
template <class T>__forceinline const T& klMax(const T &a, const T &b) {return a > b ? a : b;}

#define klstricmp lstrcmpi
#define klstricmpA lstrcmpiA
#define klstricmpW lstrcmpiW

#ifdef UNICODE
#define klatoi _wtoi
//#define klSplitCommandLine klSplitCommandLineW
#define klstrncpy(in1, in2, in3) _tcsncpy((wchar_t *) in1, (wchar_t *) in2, in3)
#endif

//#define sizeofarray(array) (sizeof(array)/sizeof(*array))
//#define klInit()


//struct klFile
//	// define KL_File_no_init to skip generation of constructors and destructors
//{
//	HANDLE hHandle;
//	u32 iCount;
//	//i64 lSize;
//	ULARGE_INTEGER lSize;
//	TCHAR Name[MAX_PATH];
//
//	void Close() {if (hHandle != INVALID_HANDLE_VALUE) {CloseHandle(hHandle); hHandle = INVALID_HANDLE_VALUE;}}
//	bool Open(const void *pName = NULL, u32 Access = GENERIC_READ, u32 CreationDisposition = OPEN_EXISTING, u32 ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE, u32 FlagsAndAttributes = FILE_ATTRIBUTE_NORMAL)
//	{
//		if (pName == NULL)
//			pName = Name;
//		hHandle = CreateFile((LPCTSTR)pName, Access, ShareMode, NULL, CreationDisposition, FlagsAndAttributes, NULL);
//		if (hHandle != INVALID_HANDLE_VALUE)
//		{
//			if (pName != Name)
//				klstrncpy(Name, pName, MAX_PATH);
//			lSize.LowPart = ::GetFileSize(hHandle, &lSize.HighPart);
//			if (lSize.LowPart == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
//				lSize.QuadPart = 0;
//			return true;
//		}
//		else
//			return false;
//	}
//	bool Read(void *buffer, u32 lSize) {return ReadFile(hHandle, buffer, lSize, &iCount, NULL) != 0 ? iCount == lSize : false;}
//	bool Write(const void *buffer, u32 lSize) {return WriteFile(hHandle, buffer, lSize, &iCount, NULL) != 0;}
//};



__forceinline u32 __cdecl klMulDivU32(u32 a, u32 b, u32 c)
{
#if !defined(__GNUC__) && !defined(WIN64)
	__asm
	{
		mov	eax, a
		mul	b
		div	c
	}
#else
	return (a*b/c);
#endif
}

