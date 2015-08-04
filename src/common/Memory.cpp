
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

#define HIDE_USE_EXCEPTION_INFO
#include <windows.h>
#include "defines.h"
#include "Memory.h"
#include "MStrSafe.h"

#ifdef _DEBUG
#include "MAssert.h"
#endif

#ifndef _ASSERTE
#define _ASSERTE(x)
#endif

HANDLE ghHeap = NULL;

#ifdef TRACK_MEMORY_ALLOCATIONS
static const char* _PointToName(const char* asFileOrPath)
{
	if (!asFileOrPath)
	{
		_ASSERTE(asFileOrPath!=NULL);
		return NULL;
	}

	const char* pszSlash = strrchr(asFileOrPath, '\\');
	if (!pszSlash) // MinGW builds
		pszSlash = strrchr(asFileOrPath, '/');

	if (pszSlash)
		return pszSlash+1;

	return asFileOrPath;
}
#endif

bool HeapInitialize()
{
	if (ghHeap == NULL)
	{
		//#ifdef MVALIDATE_POINTERS
		//	ghHeap = HeapCreate(0, 200000, 0);
		//#else
		ghHeap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 200000, 0);
		//#endif
	}

	return (ghHeap != NULL);
}

void HeapDeinitialize()
{
	if (ghHeap)
	{
		HeapDestroy(ghHeap);
		ghHeap = NULL;
	}
}


void * __cdecl xf_malloc
(
    size_t _Size
#ifdef TRACK_MEMORY_ALLOCATIONS
    , LPCSTR lpszFileName, int nLine
#endif
)
{
	_ASSERTE(ghHeap);
	_ASSERTE(_Size>0);
#ifdef TRACK_MEMORY_ALLOCATIONS
	#ifdef FORCE_HEAP_CHECK
	xf_dump_chk();
	#endif

	size_t nTotalSize = _Size+sizeof(xf_mem_block)+8;
	xf_mem_block* p = (xf_mem_block*)HeapAlloc(ghHeap, 0, nTotalSize);
	if (p)
	{
		p->bBlockUsed = TRUE;
		p->nBlockSize = _Size;

		msprintf(p->sCreatedFrom, countof(p->sCreatedFrom), "%s:%i", _PointToName(lpszFileName), nLine);

		#ifdef _DEBUG
		if (_Size > 0) memset(p+1, 0xFD, _Size);
		#endif

		memset(((LPBYTE)(p+1))+_Size, 0xCC, 8);
	}
	else
	{
		_ASSERTE(p!=NULL);
	}
	return p?(p+1):p;
#else
	void* p = HeapAlloc(ghHeap, 0, _Size);
	return p;
#endif
}


void * __cdecl xf_calloc
(
    size_t _Count, size_t _Size
#ifdef TRACK_MEMORY_ALLOCATIONS
    , LPCSTR lpszFileName, int nLine
#endif
)
{
	_ASSERTE(ghHeap);
	_ASSERTE((_Count*_Size)>0);
#ifdef TRACK_MEMORY_ALLOCATIONS
	#ifdef FORCE_HEAP_CHECK
	xf_dump_chk();
	#endif

	size_t nTotalSize = _Count*_Size+sizeof(xf_mem_block)+8;
	xf_mem_block* p = (xf_mem_block*)HeapAlloc(ghHeap, HEAP_ZERO_MEMORY, nTotalSize);
	if (p)
	{
		p->bBlockUsed = TRUE;
		p->nBlockSize = _Count*_Size;

		msprintf(p->sCreatedFrom, countof(p->sCreatedFrom), "%s:%i", _PointToName(lpszFileName), nLine);

		memset(((LPBYTE)(p+1))+_Count*_Size, 0xCC, 8);
	}
	else
	{
		_ASSERTE(p!=NULL);
	}
	return p?(p+1):p;
#else
	void* p = HeapAlloc(ghHeap, HEAP_ZERO_MEMORY, _Count*_Size);
	return p;
#endif
}


void* __cdecl xf_realloc
(
    void * _Memory, size_t _Size
#ifdef TRACK_MEMORY_ALLOCATIONS
    , LPCSTR lpszFileName, int nLine
#endif
)
{
	_ASSERTE(ghHeap);
	_ASSERTE(_Size>0);
	if (!_Memory)
	{
		return xf_malloc(_Size
			#ifdef TRACK_MEMORY_ALLOCATIONS
				, lpszFileName, nLine
			#endif
			);
	}
#ifdef TRACK_MEMORY_ALLOCATIONS
	xf_mem_block* pOld = ((xf_mem_block*)_Memory)-1;

	size_t _Size1 = HeapSize(ghHeap, 0, pOld);
	_ASSERTE(_Size1 < (_Size+sizeof(xf_mem_block)+8));
	size_t _Size2 = 0;

	if (pOld->bBlockUsed == TRUE)
	{
		int nCCcmp = memcmp(((LPBYTE)_Memory)+pOld->nBlockSize, "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC", 8);
		_ASSERTE(nCCcmp == 0);
		_ASSERTE(_Size1 == (pOld->nBlockSize+sizeof(xf_mem_block)+8));
		_Size2 = pOld->nBlockSize;
	}
	else
	{
		_ASSERTE(pOld->bBlockUsed == TRUE);
		if (_Size1 > (sizeof(xf_mem_block)+8))
			_Size2 = _Size1 - (sizeof(xf_mem_block)+8);
	}

	xf_mem_block* p = (xf_mem_block*)HeapReAlloc(ghHeap, 0, pOld, _Size+sizeof(xf_mem_block)+8);
	if (p)
	{
		p->bBlockUsed = TRUE;
		p->nBlockSize = _Size;

		msprintf(p->sCreatedFrom, countof(p->sCreatedFrom), "%s:%i", _PointToName(lpszFileName), nLine);

		#ifdef _DEBUG
		if (_Size > _Size2) memset(((LPBYTE)(p+1))+_Size2, 0xFD, _Size - _Size2);
		#endif

		memset(((LPBYTE)(p+1))+_Size, 0xCC, 8);
	}
	else
	{
		_ASSERTE(p!=NULL);
	}
	return p?(p+1):p;
#else
	void* p = HeapReAlloc(ghHeap, HEAP_ZERO_MEMORY, _Memory, _Size);
	return p;
#endif
}


void __cdecl xf_free
(
    void * _Memory
#ifdef TRACK_MEMORY_ALLOCATIONS
    , LPCSTR lpszFileName, int nLine
#endif
)
{
	if (!_Memory)
	{
		return; // Nothing to do
	}
	if (!ghHeap)
	{
		//_ASSERTE(ghHeap && _Memory);
		#ifdef _DEBUG
		_CrtDbgBreak();
		#endif
		return;
	}
#ifdef TRACK_MEMORY_ALLOCATIONS
	xf_mem_block* p = ((xf_mem_block*)_Memory)-1;

	if (p->bBlockUsed == TRUE)
	{
		int nCCcmp = memcmp(((LPBYTE)_Memory)+p->nBlockSize, "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC", 8);
		_ASSERTE(nCCcmp == 0);
		memset(_Memory, 0xFD, p->nBlockSize);
	}
	else
	{
		_ASSERTE(p->bBlockUsed == TRUE);
	}

	p->bBlockUsed = FALSE;
	msprintf(p->sCreatedFrom, countof(p->sCreatedFrom), "-- %s:%i", _PointToName(lpszFileName), nLine);
	_Memory = (void*)p;
#endif
	#ifdef _DEBUG
	size_t _Size1 = HeapSize(ghHeap, 0, _Memory);
	_ASSERTE(_Size1 > 0);
	#endif

	HeapFree(ghHeap, 0, _Memory);

	#ifdef FORCE_HEAP_CHECK
	xf_dump_chk();
	#endif
	//#ifdef _DEBUG
	//SIZE_T _Size2 = HeapSize(ghHeap, 0, _Memory);
	//if (_Size1 == _Size2) {
	//	_ASSERTE(_Size1 != _Size2);
	//}
	//#endif
}


#ifdef TRACK_MEMORY_ALLOCATIONS
#ifdef FORCE_HEAP_CHECK
void __cdecl xf_dump_chk()
{
#ifndef CONEMU_MINIMAL
	PROCESS_HEAP_ENTRY ent = {NULL};
	HeapLock(ghHeap);
	//HeapCompact(ghHeap,0);
	char sBlockInfo[255];
	PVOID pLast = NULL;

	while(HeapWalk(ghHeap, &ent))
	{
		if (pLast == ent.lpData)
		{
			msprintf(sBlockInfo, countof(sBlockInfo), "!!! HeapWalk cycled at 0x%08X, size=0x%08X\n", (DWORD)ent.lpData, ent.cbData);
			OutputDebugStringA(sBlockInfo);
			_ASSERTE(pLast != ent.lpData);
			break;
		}

		if (((int)ent.cbData) < 0)
		{
			msprintf(sBlockInfo, countof(sBlockInfo), "!!! Invalid memory block size at 0x%08X, size=0x%08X\n", (DWORD)ent.lpData, ent.cbData);
			OutputDebugStringA(sBlockInfo);
			_ASSERTE(((int)ent.cbData) >= 0);
			break;
		}
	}

	HeapUnlock(ghHeap);
#endif
}
#endif


void __cdecl xf_dump()
{
#ifndef CONEMU_MINIMAL
	PROCESS_HEAP_ENTRY ent = {NULL};
	HeapLock(ghHeap);
	//HeapCompact(ghHeap,0);
	char sBlockInfo[255];
	PVOID pLast = NULL;
	size_t cbUsedSize = 0, cbBrokenSize = 0;
	DWORD cCount = 0;

	while (HeapWalk(ghHeap, &ent))
	{
		if (pLast == ent.lpData)
		{
			msprintf(sBlockInfo, countof(sBlockInfo), "!!! HeapWalk cycled at 0x%08X, size=0x%08X\n", LODWORD(ent.lpData), ent.cbData);
			OutputDebugStringA(sBlockInfo);
			_ASSERTE(pLast != ent.lpData);
			break;
		}

		if (((int)ent.cbData) < 0)
		{
			msprintf(sBlockInfo, countof(sBlockInfo), "!!! Invalid memory block size at 0x%08X, size=0x%08X\n", LODWORD(ent.lpData), ent.cbData);
			OutputDebugStringA(sBlockInfo);
			_ASSERTE(((int)ent.cbData) >= 0);
			break;
		}

		if (ent.wFlags & PROCESS_HEAP_ENTRY_BUSY)
		{
			xf_mem_block* p = (xf_mem_block*)ent.lpData;

			if (p->bBlockUsed==TRUE && (p->nBlockSize+sizeof(xf_mem_block)+8)==ent.cbData)
			{
				msprintf(sBlockInfo, countof(sBlockInfo), "Used memory block at 0x" WIN3264TEST("%08X","%08X%08X") ", size %u\n    Allocated from: %s\n", WIN3264WSPRINT(ent.lpData), ent.cbData,
				          p->sCreatedFrom);

				cbUsedSize += p->nBlockSize;
			}
			else
			{
				msprintf(sBlockInfo, countof(sBlockInfo), "Used memory block at 0x" WIN3264TEST("%08X","%08X%08X") ", size %u\n    Allocated from: %s\n", WIN3264WSPRINT(ent.lpData), ent.cbData,
				          "<Header information broken!>");

				cbBrokenSize += ent.cbData;
			}

			cCount++;

			pLast = ent.lpData;
			OutputDebugStringA(sBlockInfo);
		}
	}

	HeapUnlock(ghHeap);

	msprintf(sBlockInfo, countof(sBlockInfo), "Used size 0x" WIN3264TEST("%08X","%08X%08X") ", broken size 0x" WIN3264TEST("%08X","%08X%08X") ", total blocks %u\n",
		WIN3264WSPRINT(cbUsedSize), WIN3264WSPRINT(cbBrokenSize), cCount);
	OutputDebugStringA(sBlockInfo);
#endif
}
#endif


bool __cdecl xf_validate(void * _Memory /*= NULL*/)
{
	_ASSERTE(ghHeap);
#ifdef TRACK_MEMORY_ALLOCATIONS

	if (_Memory)
	{
		int nCCcmp;
		xf_mem_block* p = ((xf_mem_block*)_Memory)-1;
		if (p->bBlockUsed == TRUE)
		{
			nCCcmp = memcmp(((LPBYTE)_Memory)+p->nBlockSize, "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC", 8);
			_ASSERTE(nCCcmp == 0);
		}
		else
		{
			_ASSERTE(p->bBlockUsed == TRUE);
		}
		_Memory = (void*)p;
	}

#endif
	BOOL b = HeapValidate(ghHeap, 0, _Memory);
	return (b!=FALSE);
}


void * __cdecl operator new(size_t _Size)
{
	void * p = xf_malloc(
	               _Size
#ifdef TRACK_MEMORY_ALLOCATIONS
	               ,__FILE__,__LINE__
#endif
	           );
#ifdef MVALIDATE_POINTERS
	_ASSERTE(p != NULL);

	if (p == NULL) InvalidOp();

#endif
	return p;
}


void * __cdecl operator new[](size_t _Size)
{
	void * p = xf_malloc(
	               _Size
#ifdef TRACK_MEMORY_ALLOCATIONS
	               ,__FILE__,__LINE__
#endif
	           );
#ifdef MVALIDATE_POINTERS
	_ASSERTE(p != NULL);

	if (p == NULL) InvalidOp();

#endif
	return p;
}


void __cdecl operator delete(void *p)
{
	xf_free(
	           p
#ifdef TRACK_MEMORY_ALLOCATIONS
	           ,__FILE__,__LINE__
#endif
	       );
}


void __cdecl operator delete[](void *p)
{
	xf_free(
	           p
#ifdef TRACK_MEMORY_ALLOCATIONS
	           ,__FILE__,__LINE__
#endif
	       );
}


char* lstrdup(const char* asText)
{
	int nLen = asText ? lstrlenA(asText) : 0;
	char* psz = (char*)malloc(nLen+1);

	if (nLen)
		StringCchCopyA(psz, nLen+1, asText);
	else
		psz[0] = 0;

	return psz;
}

wchar_t* lstrdup(const wchar_t* asText, size_t cchExtraSizeAdd /* = 0 */)
{
	size_t nLen = asText ? lstrlenW(asText) : 0;
	wchar_t* psz = (wchar_t*)malloc((nLen+1+cchExtraSizeAdd) * sizeof(*psz));

	if (nLen)
		StringCchCopyW(psz, nLen+1, asText);

	// Ensure AsciiZ
	psz[nLen] = 0;

	return psz;
}

wchar_t* lstrdupW(const char* asText, UINT cp /*= CP_ACP*/)
{
	int nLen = asText ? lstrlenA(asText) : 0;
	wchar_t* psz = (wchar_t*)malloc((nLen+1) * sizeof(*psz));

	if (nLen)
		MultiByteToWideChar(cp, 0, asText, nLen+1, psz, nLen+1);
	else
		psz[0] = 0;

	return psz;
}

wchar_t* lstrmerge(const wchar_t* asStr1, const wchar_t* asStr2, const wchar_t* asStr3 /*= NULL*/, const wchar_t* asStr4 /*= NULL*/, const wchar_t* asStr5 /*= NULL*/)
{
	size_t cchMax = 1;
	const size_t Count = 5;
	size_t cch[Count] = {};
	const wchar_t* pszStr[Count] = {asStr1, asStr2, asStr3, asStr4, asStr5};

	for (size_t i = 0; i < Count; i++)
	{
		cch[i] = pszStr[i] ? lstrlen(pszStr[i]) : 0;
		cchMax += cch[i];
	}

	wchar_t* pszRet = (wchar_t*)malloc(cchMax*sizeof(*pszRet));
	if (!pszRet)
		return NULL;
	*pszRet = 0;
	wchar_t* psz = pszRet;

	for (size_t i = 0; i < Count; i++)
	{
		if (!cch[i])
			continue;

		_wcscpy_c(psz, cch[i]+1, pszStr[i]);
		psz += cch[i];
	}

	return pszRet;
}

bool lstrmerge(wchar_t** apsStr1, const wchar_t* asStr2, const wchar_t* asStr3 /*= NULL*/, const wchar_t* asStr4 /*= NULL*/, const wchar_t* asStr5 /*= NULL*/)
{
	_ASSERTE(apsStr1!=NULL);

	wchar_t* pszNew = lstrmerge(*apsStr1, asStr2, asStr3, asStr4, asStr5);
	if (!pszNew)
		return false;

	if (*apsStr1)
		free(*apsStr1);
	*apsStr1 = pszNew;

	return true;
}
