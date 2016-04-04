#pragma once

#if !defined(__GNUC__)
#pragma warning(push)
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
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;
typedef INT_PTR ssize_t;

#if defined(__GNUC__) && !defined(__MINGW64_VERSION_MAJOR)
#define __forceinline __inline__
#endif
template <class T>__forceinline const T& klMin(const T& a, const T& b) {return a < b ? a : b;}
template <class T>__forceinline const T& klMax(const T& a, const T& b) {return a > b ? a : b;}
template <class T>__forceinline void klSwap(T& a, T& b) {T k = a; a = b; b = k;}
template <class T>__forceinline const T klSet(T& a, const T& b) { const T k = a; a = b; return k; }

#define klstricmp lstrcmpi
#define klstricmpA lstrcmpiA
#define klstricmpW lstrcmpiW


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

#if !defined(__GNUC__)
#pragma warning(pop)
#endif
