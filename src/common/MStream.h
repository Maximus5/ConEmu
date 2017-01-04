
/*
Copyright (c) 2010-2017 Maximus5
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

class MStream : public IStream
{
private:
	LONG mn_RefCount; BOOL mb_SelfAlloc; char* mp_Data; DWORD mn_DataSize; DWORD mn_DataLen; DWORD mn_DataPos;
public:
	MStream()
	{
		mn_RefCount = 1; mb_SelfAlloc=TRUE;
		mn_DataSize = 0; mn_DataPos = 0; mn_DataLen = 0;
		mp_Data = NULL;
	};
	virtual ~MStream()
	{
		if (mp_Data!=NULL)
		{
			if (mp_Data && mb_SelfAlloc) HeapFree(GetProcessHeap(), 0, mp_Data); 
			mp_Data=NULL;
			mn_DataSize = 0; mn_DataPos = 0; mn_DataLen = 0;
		}
	};
	void SetData(LPCVOID apData, DWORD anLen)
	{
		if (mp_Data && mb_SelfAlloc) HeapFree(GetProcessHeap(), 0, mp_Data); 
		mb_SelfAlloc=FALSE; mp_Data = (char*)apData; mn_DataSize = anLen;
		mn_DataPos = 0; mn_DataLen = anLen;
	};
public:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject)
	{
			if (riid==IID_IStream || riid==IID_ISequentialStream || riid==IID_IUnknown)
			{
				*ppvObject = this;
				AddRef(); // 2009-10-03 не было
				return S_OK;
			}
			return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef( void)
	{
		if (this==NULL)
			return 0;
		return (++mn_RefCount);
	}

	virtual ULONG STDMETHODCALLTYPE Release( void)
	{
		if (this==NULL)
			return 0;
		mn_RefCount--;
		int nCount = mn_RefCount;
		if (nCount<=0)
		{
			delete this;
		}
		return nCount;
	}
public:
	/* ISequentialStream */
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read( 
		/* [length_is][size_is][out] */ void *pv,
		/* [in] */ ULONG cb,
		/* [out] */ ULONG *pcbRead)
	{
		DWORD dwRead = 0;
		if (mp_Data)
		{
			dwRead = min(cb, (mn_DataLen>mn_DataPos)?(mn_DataLen-mn_DataPos):0);

			if (dwRead>0)
			{
				memmove(pv, mp_Data+mn_DataPos, dwRead);
				mn_DataPos += dwRead;
			}
		}
		else
		{
			return S_FALSE;
		}
		if (pcbRead) *pcbRead=dwRead;
		return S_OK;
	};

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write( 
		/* [size_is][in] */ const void *pv,
		/* [in] */ ULONG cb,
		/* [out] */ ULONG *pcbWritten)
	{
		DWORD dwWritten=0;
		if (!mp_Data)
		{
			ULARGE_INTEGER lNewSize; lNewSize.QuadPart = cb;
			HRESULT hr;
			if (FAILED(hr = SetSize(lNewSize)))
				return hr;
		}
		if (mp_Data)
		{
			if ((mn_DataPos+cb)>mn_DataSize)
			{
				// Нужно увеличить буфер, но сохранить текущий размер
				DWORD lLastLen = mn_DataLen;
				ULARGE_INTEGER lNewSize; lNewSize.QuadPart = mn_DataSize + max((cb+1024), 256*1024);
				if (lNewSize.HighPart!=0)
					return S_FALSE;
				if (FAILED(SetSize(lNewSize)))
					return S_FALSE;
				mn_DataLen = lLastLen; // Вернули текущий размер
			}
			// Теперь можно писать в буфер
			memmove(mp_Data+mn_DataPos, pv, cb);
			dwWritten = cb;
			mn_DataPos += cb;
			if (mn_DataPos>mn_DataLen) mn_DataLen=mn_DataPos; //2008-03-18
		}
		else
		{
			return S_FALSE;
		}
		if (pcbWritten) *pcbWritten=dwWritten;
		return S_OK;
	};
public:
	/* Wrapper */
	HRESULT STDMETHODCALLTYPE Seek( 
		/* [in] */ __int64 dlibMove,
		/* [in] */ DWORD dwOrigin,
		/* [out] */ __int64 *plibNewPosition)
	{
		HRESULT hr = S_OK;
		LARGE_INTEGER lMove; lMove.QuadPart = dlibMove;
		ULARGE_INTEGER lNew; lNew.QuadPart = 0;

		hr = Seek(lMove, dwOrigin, &lNew);

		if (hr == S_OK && plibNewPosition)
			*plibNewPosition = lNew.QuadPart;

		return hr;
	}
	/* IStream */
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek( 
		/* [in] */ LARGE_INTEGER dlibMove,
		/* [in] */ DWORD dwOrigin,
		/* [out] */ ULARGE_INTEGER *plibNewPosition)
	{
		if (dwOrigin!=STREAM_SEEK_SET && dwOrigin!=STREAM_SEEK_CUR && dwOrigin!=STREAM_SEEK_END)
			return STG_E_INVALIDFUNCTION;

		if (mp_Data)
		{
			_ASSERTE(mn_DataSize);
			LARGE_INTEGER lNew;
			if (dwOrigin==STREAM_SEEK_SET)
			{
				lNew.QuadPart = dlibMove.QuadPart;
			}
			else if (dwOrigin==STREAM_SEEK_CUR)
			{
				lNew.QuadPart = mn_DataPos + dlibMove.QuadPart;
			}
			else if (dwOrigin==STREAM_SEEK_END)
			{
				lNew.QuadPart = mn_DataLen + dlibMove.QuadPart;
			}
			if (lNew.QuadPart<0)
				return S_FALSE;
			if (lNew.QuadPart>mn_DataSize)
				return S_FALSE;
			mn_DataPos = lNew.LowPart;
			if (plibNewPosition)
				plibNewPosition->QuadPart = mn_DataPos;
		}
		else
		{
			return S_FALSE;
		}
		return S_OK;
	};

	virtual HRESULT STDMETHODCALLTYPE SetSize( 
		/* [in] */ ULARGE_INTEGER libNewSize)
	{
		HRESULT hr = STG_E_INVALIDFUNCTION;
		//ULARGE_INTEGER llTest; llTest.QuadPart = 0;
		//LARGE_INTEGER llShift; llShift.QuadPart = 0;

		if (libNewSize.HighPart)
			return E_OUTOFMEMORY;

		if (mp_Data)
		{
			if (libNewSize.LowPart>mn_DataSize)
			{
				char* pNew = (char*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mp_Data, libNewSize.LowPart);
				if (pNew==NULL)
					return E_OUTOFMEMORY;
				mp_Data = pNew;
			}
			mn_DataLen = libNewSize.LowPart;
			if (mn_DataPos>mn_DataLen) // Если размер уменьшили - проверить позицию
				mn_DataPos = mn_DataLen;
			hr = S_OK;

		}
		else
		{
			mp_Data = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, libNewSize.LowPart);
			if (mp_Data==NULL)
				return E_OUTOFMEMORY;
			mn_DataSize = libNewSize.LowPart;
			mn_DataLen = libNewSize.LowPart;
			mn_DataPos = 0;
			hr = S_OK;
		}
		return hr;
	};

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo( 
		/* [unique][in] */ IStream *pstm,
		/* [in] */ ULARGE_INTEGER cb,
		/* [out] */ ULARGE_INTEGER *pcbRead,
		/* [out] */ ULARGE_INTEGER *pcbWritten)
	{
		return STG_E_INVALIDFUNCTION;
	};

	virtual HRESULT STDMETHODCALLTYPE Commit( 
		/* [in] */ DWORD grfCommitFlags) 
	{
		if (mp_Data)
		{
			//
		}
		else
		{
			return S_FALSE;
		}
		return S_OK;
	};

	virtual HRESULT STDMETHODCALLTYPE Revert( void) 
	{
		return STG_E_INVALIDFUNCTION;
	};

	virtual HRESULT STDMETHODCALLTYPE LockRegion( 
		/* [in] */ ULARGE_INTEGER libOffset,
		/* [in] */ ULARGE_INTEGER cb,
		/* [in] */ DWORD dwLockType) 
	{
		return STG_E_INVALIDFUNCTION;
	};

	virtual HRESULT STDMETHODCALLTYPE UnlockRegion( 
		/* [in] */ ULARGE_INTEGER libOffset,
		/* [in] */ ULARGE_INTEGER cb,
		/* [in] */ DWORD dwLockType)
	{
		return STG_E_INVALIDFUNCTION;
	};

	virtual HRESULT STDMETHODCALLTYPE Stat( 
		/* [out] */ STATSTG *pstatstg,
		/* [in] */ DWORD grfStatFlag)
	{
		pstatstg->type = STGTY_STREAM;
		pstatstg->cbSize.QuadPart = mn_DataLen;
		SYSTEMTIME st; GetLocalTime(&st);
		FILETIME ft; SystemTimeToFileTime(&st,&ft);
		pstatstg->mtime = ft; pstatstg->ctime = ft; pstatstg->atime = ft;
		pstatstg->grfMode = STGM_READWRITE|STGM_SHARE_EXCLUSIVE;
		pstatstg->grfLocksSupported = LOCK_EXCLUSIVE;
		pstatstg->grfStateBits = 0;
		return S_OK;
	};

	virtual HRESULT STDMETHODCALLTYPE Clone( 
		/* [out] */ IStream **ppstm)
	{
		return STG_E_INVALIDFUNCTION;
	};
};
