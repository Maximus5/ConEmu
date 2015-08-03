
/*
Copyright (c) 2009-2014 Maximus5
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

#pragma once

#ifndef MEMORY_HEADER_DEFINED
#define MEMORY_HEADER_DEFINED

extern HANDLE ghHeap;

#ifdef _DEBUG
#define TRACK_MEMORY_ALLOCATIONS
#endif

// Heap allocation routines

bool HeapInitialize();
void HeapDeinitialize();

void * __cdecl operator new(size_t size);
void * __cdecl operator new[](size_t size);
void __cdecl operator delete(void *block);
void __cdecl operator delete[](void *ptr);

#ifdef TRACK_MEMORY_ALLOCATIONS
	typedef struct tag_xf_mem_block
	{
		union
		{
			BOOL   bBlockUsed;
			LPVOID Padding;
		};
		size_t nBlockSize;
		char   sCreatedFrom[32];
	} xf_mem_block;

	//#define DEBUG_NEW new(THIS_FILE, __LINE__)

	#if defined(__CYGWIN__)
	extern "C" {
	#endif
	void * __cdecl xf_malloc(size_t _Size, LPCSTR lpszFileName, int nLine);
	void * __cdecl xf_calloc(size_t _Count, size_t _Size, LPCSTR lpszFileName, int nLine);
	void * __cdecl xf_realloc(void * _Memory, size_t _Size, LPCSTR lpszFileName, int nLine);
	void __cdecl xf_free(void * _Memory, LPCSTR lpszFileName, int nLine);
	#if defined(__CYGWIN__)
	}
	#endif

	void __cdecl xf_dump();
	#ifdef FORCE_HEAP_CHECK
	void __cdecl xf_dump_chk();
	#else
	#define xf_dump_chk()
	#endif

	#define malloc(s) xf_malloc(s,__FILE__,__LINE__)
	#define calloc(c,s) xf_calloc(c,s,__FILE__,__LINE__)
	#define realloc(p,s) xf_realloc(p,s,__FILE__,__LINE__)
	#define free(p) xf_free(p,__FILE__,__LINE__)

#else

	#if defined(__CYGWIN__)
	extern "C" {
	#endif
	void * __cdecl xf_malloc(size_t _Size);
	void * __cdecl xf_calloc(size_t _Count, size_t _Size);
	void * __cdecl xf_realloc(void * _Memory, size_t _Size);
	void __cdecl xf_free(void * _Memory);
	#if defined(__CYGWIN__)
	}
	#endif

	#define malloc xf_malloc
	#define calloc xf_calloc
	#define realloc xf_realloc
	#define free xf_free

	#define xf_dump()
	#define xf_dump_chk()

#endif

bool __cdecl xf_validate(void * _Memory = NULL);

#ifdef MVALIDATE_POINTERS
#define SafeFree(p) if ((p)!=NULL) { if (!xf_validate((p))) free((p)); (p) = NULL; }
//#define SafeDelete(p) if ((p)!=NULL) { if (!xf_validate((p))) delete (p); (p) = NULL; }
template <class T>
void SafeDelete(T*& p)
{
	T* ps = p;
	p = NULL;
	if (!xf_validate((ps)))
	{
		delete ps;
	}
}
#else
#ifdef TRACK_MEMORY_ALLOCATIONS
#define SafeFree(p) if ((p)!=NULL) { xf_free((p),__FILE__,__LINE__); (p) = NULL; }
#else
#define SafeFree(p) if ((p)!=NULL) { xf_free((p)); (p) = NULL; }
#endif
//#define SafeDelete(p) if ((p)!=NULL) { delete (p); (p) = NULL; }
template <class T>
void SafeDelete(T*& p)
{
	if (!p) return;
	T* ps = p;
	p = NULL;
	delete ps;
}
#endif

char* lstrdup(const char* asText);
wchar_t* lstrdup(const wchar_t* asText, size_t cchExtraSizeAdd = 0);
wchar_t* lstrdupW(const char* asText, UINT cp = CP_ACP);
wchar_t* lstrmerge(const wchar_t* asStr1, const wchar_t* asStr2, const wchar_t* asStr3 = NULL, const wchar_t* asStr4 = NULL, const wchar_t* asStr5 = NULL);
bool lstrmerge(wchar_t** apsStr1, const wchar_t* asStr2, const wchar_t* asStr3 = NULL, const wchar_t* asStr4 = NULL, const wchar_t* asStr5 = NULL);

#endif // #ifndef MEMORY_HEADER_DEFINED
