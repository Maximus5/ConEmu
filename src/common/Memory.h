
/*
Copyright (c) 2009-2016 Maximus5
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

	#define XF_PLACE_ARGS_DEF , LPCSTR lpszFileName, int nLine
	#define XF_PLACE_ARGS_VAL , __FILE__, __LINE__
#else
	#define XF_PLACE_ARGS_DEF
	#define XF_PLACE_ARGS_VAL
#endif

#if defined(__CYGWIN__)
extern "C" {
#endif
void * __cdecl xf_malloc(size_t _Size XF_PLACE_ARGS_DEF);
void * __cdecl xf_calloc(size_t _Count, size_t _Size XF_PLACE_ARGS_DEF);
void * __cdecl xf_realloc(void * _Memory, size_t _Size XF_PLACE_ARGS_DEF);
void __cdecl xf_free(void * _Memory XF_PLACE_ARGS_DEF);
#if defined(__CYGWIN__)
}
#endif

#define malloc(s) xf_malloc(s XF_PLACE_ARGS_VAL)
#define calloc(c,s) xf_calloc(c,s XF_PLACE_ARGS_VAL)
#define realloc(p,s) xf_realloc(p,s XF_PLACE_ARGS_VAL)
#define free(p) xf_free(p XF_PLACE_ARGS_VAL)



#ifdef TRACK_MEMORY_ALLOCATIONS

	void __cdecl xf_dump();
	#ifdef FORCE_HEAP_CHECK
	void __cdecl xf_dump_chk();
	#else
	#define xf_dump_chk()
	#endif

	void xf_set_tag(void* _Memory, LPCSTR lpszFileName, int nLine);

#else

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

#endif // #ifndef MEMORY_HEADER_DEFINED
