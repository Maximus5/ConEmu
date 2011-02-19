
#pragma once

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
	BOOL   bBlockUsed;
	size_t nBlockSize;
	char   sCreatedFrom[32];
} xf_mem_block;

//#define DEBUG_NEW new(THIS_FILE, __LINE__)
#define TRACK_MEMORY_ARGS LPCSTR lpszFileName, int nLine
// New Operators
//void* __cdecl operator new(size_t size, TRACK_MEMORY_ARGS);
//void * __cdecl operator new[] (size_t size, TRACK_MEMORY_ARGS);
//void __cdecl operator delete(void *block, TRACK_MEMORY_ARGS);
//void __cdecl operator delete[] (void *ptr, TRACK_MEMORY_ARGS);

void * __cdecl xf_calloc(size_t _Count, size_t _Size, LPCSTR lpszFileName, int nLine);
void __cdecl xf_free(void * _Memory, LPCSTR lpszFileName, int nLine);
void * __cdecl xf_malloc(size_t _Size, LPCSTR lpszFileName, int nLine);

void __cdecl xf_dump();
#ifdef FORCE_HEAP_CHECK
void __cdecl xf_dump_chk();
#else
#define xf_dump_chk()
#endif

#define malloc(s) xf_malloc(s,__FILE__,__LINE__)
#define calloc(c,s) xf_calloc(c,s,__FILE__,__LINE__)
#define free(p) xf_free(p,__FILE__,__LINE__)

#else

void * __cdecl xf_calloc(size_t _Count, size_t _Size);
void __cdecl xf_free(void * _Memory);
void * __cdecl xf_malloc(size_t _Size);

#define malloc xf_malloc
#define calloc xf_calloc
#define free xf_free

#define xf_dump()
#define xf_dump_chk()

#endif

bool __cdecl xf_validate(void * _Memory = NULL);

#ifdef MVALIDATE_POINTERS
#define SafeFree(p) if ((p)!=NULL) { if (!xf_validate((p))) free((p)); (p) = NULL; }
#define SafeDelete(p) if ((p)!=NULL) { if (!xf_validate((p))) delete (p); (p) = NULL; }
#else
#ifdef TRACK_MEMORY_ALLOCATIONS
#define SafeFree(p) if ((p)!=NULL) { xf_free((p),__FILE__,__LINE__); (p) = NULL; }
#else
#define SafeFree(p) if ((p)!=NULL) { xf_free((p)); (p) = NULL; }
#endif
#define SafeDelete(p) if ((p)!=NULL) { delete (p); (p) = NULL; }
#endif

char* lstrdup(const char* asText);
wchar_t* lstrdup(const wchar_t* asText);

const char* PointToName(const char* asFileOrPath);
