
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


// Visual Studio 2015 Universal CRT
// http://blogs.msdn.com/b/vcblog/archive/2015/03/03/introducing-the-universal-crt.aspx
#if defined(_MSC_VER) && (_MSC_VER >= 1900)
	#if defined(_DEBUG)
		#pragma comment(lib, "libvcruntimed.lib")
		#pragma comment(lib, "libucrtd.lib")
	#else
		#pragma comment(lib, "libvcruntime.lib")
		#pragma comment(lib, "libucrt.lib")
	#endif
#endif


#ifdef TRACK_MEMORY_ALLOCATIONS
static const char* _PointToName(const char* asFileOrPath)
{
	if (!asFileOrPath)
	{
		_ASSERTE(asFileOrPath!=NULL);
		return NULL;
	}

	// Utilize both type of slashes
	const char* pszBSlash = strrchr(asFileOrPath, '\\');
	const char* pszFSlash = strrchr(pszBSlash ? pszBSlash : asFileOrPath, '/');

	const char* pszSlash = pszFSlash ? pszFSlash : pszBSlash;;

	if (pszSlash)
		return pszSlash + 1;

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


#ifdef TRACK_MEMORY_ALLOCATIONS
void xf_set_tag(void* _Memory, LPCSTR lpszFileName, int nLine)
{
	xf_mem_block* p = ((xf_mem_block*)_Memory)-1;

	_ASSERTE(_Memory && p && p->bBlockUsed && p->nBlockSize);

	msprintf(p->sCreatedFrom, countof(p->sCreatedFrom), "%s:%i", _PointToName(lpszFileName), nLine);
}
#endif

void * __cdecl xf_malloc(size_t _Size XF_PLACE_ARGS_DEF)
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


void * __cdecl xf_calloc(size_t _Count, size_t _Size XF_PLACE_ARGS_DEF)
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


void* __cdecl xf_realloc(void * _Memory, size_t _Size XF_PLACE_ARGS_DEF)
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
	//_ASSERTE(_Size1 < (_Size+sizeof(xf_mem_block)+8));
	_ASSERTE(_Size1 > (sizeof(xf_mem_block)+8));
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


void __cdecl xf_free(void * _Memory XF_PLACE_ARGS_DEF)
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


#ifndef CONEMU_MINIMAL
#include <stdlib.h>
class CTrackBlocks
{
protected:
	HANDLE hHeap;
	size_t cchMaxCount, cchCount, cbUsedSize;
	xf_mem_block* pBlocks;
	bool ourBlocks;
protected:
	static int __cdecl CompareBlocks(const void* p1, const void* p2)
	{
		size_t s1 = ((xf_mem_block*)p1)->nBlockSize;
		size_t s2 = ((xf_mem_block*)p2)->nBlockSize;
		if (s1 < s2)
			return -1;
		else if (s1 > s2)
			return 1;
		else
			return 0;
	}
public:
	CTrackBlocks(bool abOurBlocks)
	{
		ourBlocks = abOurBlocks;
		hHeap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 20000, 0);
		cchCount = cbUsedSize = 0;
		cchMaxCount = 2048;
		pBlocks = (xf_mem_block*)HeapAlloc(hHeap, 0, cchMaxCount*sizeof(*pBlocks));
	};
	~CTrackBlocks()
	{
		// No need to free "pBlocks", we destroy heap completely
		HeapDestroy(hHeap);
	};
	void DumpBlocks()
	{
		if (pBlocks && cchCount)
		{
			char sBlockInfo[255];
			OutputDebugStringA(ourBlocks ? "\n============== OUR MEMORY BLOCKS BEGIN ==============\n" : "\n============== EXT MEMORY BLOCKS BEGIN ==============\n");
			qsort(pBlocks, cchCount, sizeof(*pBlocks), CompareBlocks);
			xf_mem_block* p = pBlocks;
			for (size_t i = 0; i < cchCount; i++, p++)
			{
				msprintf(sBlockInfo, countof(sBlockInfo), "Used memory block at 0x" WIN3264TEST("%08X","%08X%08X") ", size %u\n    Allocated from: %s\n",
					WIN3264WSPRINT(p->Padding), p->nBlockSize, p->sCreatedFrom);
				OutputDebugStringA(sBlockInfo);
			}
			OutputDebugStringA(ourBlocks ? "\n============== END OF OUR MEMORY BLOCKS =============\n" : "\n============== END OF EXT MEMORY BLOCKS =============\n");
		}
	};
	void AddBlock(const xf_mem_block* p)
	{
		xf_mem_block xm = *p;
		xm.Padding = (void*)p;
		PushBlock(xm);
	};
	void AddBlock(const PROCESS_HEAP_ENTRY& ent)
	{
		xf_mem_block xm = {};
		xm.Padding = ent.lpData;
		xm.nBlockSize = ent.cbData;
		lstrcpynA(xm.sCreatedFrom, "<EXTERNAL>", countof(xm.sCreatedFrom));
		PushBlock(xm);
	};
	size_t getUsedSize()
	{
		return cbUsedSize;
	};
protected:
	void PushBlock(const xf_mem_block& blk)
	{
		cbUsedSize += blk.nBlockSize;
		if (!pBlocks || (cchCount == cchMaxCount))
		{
			pBlocks = (xf_mem_block*)HeapReAlloc(hHeap, 0, pBlocks, cchMaxCount*2*sizeof(*pBlocks));
			if (!pBlocks)
			{
				OutputDebugStringA("\n\n******************************\n******* HeapReAlloc FAILED *******\n******************************\n\n");
				return;
			}
			cchMaxCount *= 2;
		}
		pBlocks[cchCount++] = blk;
	};
};
#endif

void __cdecl xf_dump()
{
#ifndef CONEMU_MINIMAL
	PROCESS_HEAP_ENTRY ent = {NULL};
	HeapLock(ghHeap);
	//HeapCompact(ghHeap,0);
	char sBlockInfo[255];
	PVOID pLast = NULL;
	DWORD cCount = 0;

	CTrackBlocks ourBlocks(true), extBlocks(false);

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
				ourBlocks.AddBlock(p);
			}
			else
			{
				extBlocks.AddBlock(ent);
			}

			cCount++;

			pLast = ent.lpData;
		}
	}

	HeapUnlock(ghHeap);

	extBlocks.DumpBlocks();
	ourBlocks.DumpBlocks();

	msprintf(sBlockInfo, countof(sBlockInfo),
		"Used size:   %u (0x" WIN3264TEST("%08X","%08X%08X") ")\n"
		"Broken size: %u (0x" WIN3264TEST("%08X","%08X%08X") ")\n"
		"Block count: %u\n"
		"=====================================================\n",
		LODWORD(ourBlocks.getUsedSize()), WIN3264WSPRINT(ourBlocks.getUsedSize()),
		LODWORD(extBlocks.getUsedSize()), WIN3264WSPRINT(extBlocks.getUsedSize()),
		cCount);
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
