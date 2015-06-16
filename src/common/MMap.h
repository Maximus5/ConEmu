
/*
Copyright (c) 2012 Maximus5
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

// This template is NOT THREAD SAFE

template<class KEY_T,class VAL_T>
struct MMap
{
protected:
	struct MapItems
	{
		bool  used;
		KEY_T key;
		VAL_T val;
	};
	MapItems *mp_Items;
	size_t mn_MaxCount;

public:
	MMap()
	{
		mp_Items = NULL; mn_MaxCount = 0;
	};
	~MMap()
	{
		// Don't use ‘Release()’ if heap is already deinitialized
		// That may happen because MMap is used for global static variables
		if (ghHeap)
		{
			Release();
		}
	};

public:
	bool Init(size_t nMaxCount = 256, bool bOnCreate = false)
	{
		if (bOnCreate)
			memset(this, 0, sizeof(*this));
		_ASSERTE(mp_Items == NULL);
		mp_Items = (MapItems*)calloc(nMaxCount,sizeof(*mp_Items));
		if (!mp_Items)
		{
			_ASSERTE(mp_Items!=NULL && "Failed to allocate MMap storage");
			return false;
		}
		mn_MaxCount = nMaxCount;
		return true;
	};

	bool Initialized()
	{
		return (mp_Items && mn_MaxCount);
	};

	void Release()
	{
		mn_MaxCount = 0;
		if (mp_Items)
		{
			free(mp_Items);
			mp_Items = NULL;
		}
	};

	void Reset()
	{
		if (mp_Items && mn_MaxCount)
		{
			memset(mp_Items, 0, mn_MaxCount*sizeof(*mp_Items));
		}
	}

	void CheckKey(const KEY_T& key)
	{
	#ifdef _DEBUG
		KEY_T NullKey;
		memset(&NullKey, 0, sizeof(NullKey));
		int iCmp = memcmp(&NullKey, &key, sizeof(KEY_T));
		_ASSERTE(iCmp != 0 && "NULL keys are not allowed");
	#endif
	}

	bool Get(const KEY_T& key, VAL_T* val, bool Remove = false)
	{
		if (!mp_Items || !mn_MaxCount)
		{
			_ASSERTE(mp_Items && mn_MaxCount && "MMap::Get - Storage must be allocated first");
			return false;
		}

		for (size_t i = 0; i < mn_MaxCount; i++)
		{
			if (mp_Items[i].used && (key == mp_Items[i].key))
			{
				if (val)
				{
					*val = mp_Items[i].val;
				}

				if (Remove)
				{
					memset(&(mp_Items[i].key), 0, sizeof(mp_Items[i].key));
					memset(&(mp_Items[i].val), 0, sizeof(mp_Items[i].val));
					mp_Items[i].used = false;
				}

				return true;
			}
		}

		return false;
	};

	bool GetNext(const KEY_T* prev/*pass NULL to get first key*/, KEY_T* next_key, VAL_T* next_val)
	{
		if (!mp_Items || !mn_MaxCount)
		{
			_ASSERTE(mp_Items && mn_MaxCount && "MMap::Get - Storage must be allocated first");
			return false;
		}

		INT_PTR iFrom = -1;

		if (prev != NULL)
		{
			while ((size_t)(++iFrom) < mn_MaxCount)
			{
				if (mp_Items[iFrom].used && (mp_Items[iFrom].key == *prev))
					break;
			}
		}

		for (size_t i = iFrom+1; i < mn_MaxCount; i++)
		{
			if (mp_Items[i].used)
			{
				if (next_key)
					*next_key = mp_Items[i].key;

				if (next_val)
					*next_val = mp_Items[i].val;

				return true;
			}
		}

		return false;
	}

	bool Set(const KEY_T& key, const VAL_T& val)
	{
		if (!mp_Items || !mn_MaxCount)
		{
			// Ругнемся, для четкости
			_ASSERTE(mp_Items && mn_MaxCount && "MMap::Set - Storage must be allocated first");

			// Но разрешим автоинит
			if (!Init())
			{
				return false;
			}
		}

		for (size_t i = 0; i < mn_MaxCount; i++)
		{
			if (mp_Items[i].used && (key == mp_Items[i].key))
			{
				mp_Items[i].val = val;
				// Multithread?
				_ASSERTE(mp_Items[i].used && (mp_Items[i].key == key) && !memcmp(&mp_Items[i].val, &val, sizeof(val)));
				return true;
			}
		}

		bool bFirst = true;
		while (true)
		{
			for (size_t i = 0; i < mn_MaxCount; i++)
			{
				if (!mp_Items[i].used)
				{
					mp_Items[i].used = true;
					mp_Items[i].key = key;
					mp_Items[i].val = val;
					// Multithread?
					_ASSERTE(mp_Items[i].used && (mp_Items[i].key == key) && !memcmp(&mp_Items[i].val, &val, sizeof(val)));
					return true;
				}
			}

			if (!bFirst)
			{
				_ASSERTE(FALSE && "Can't add item to MMap, not enough storage");
				break;
			}
			bFirst = false;

			size_t nNewMaxCount = mn_MaxCount + 256;
			MapItems* pNew = (MapItems*)realloc(mp_Items,nNewMaxCount*sizeof(*pNew));
			if (!pNew)
			{
				_ASSERTE(pNew!=NULL);
				return false;
			}
			memset(pNew+mn_MaxCount, 0, (nNewMaxCount - mn_MaxCount)*sizeof(*pNew));
			mp_Items = pNew;
		}

		_ASSERTE(FALSE && "Must not get here");
		return false;
	};

	bool Del(const KEY_T& key)
	{
		if (!mp_Items || !mn_MaxCount)
		{
			// -- если еще пустой - не важно
			//_ASSERTE(mp_Items && mn_MaxCount && "MMap::Set - Storage must be allocated first");
			return false;
		}
		for (size_t i = 0; i < mn_MaxCount; i++)
		{
			if (mp_Items[i].used && (key == mp_Items[i].key))
			{
				memset(&(mp_Items[i].key), 0, sizeof(mp_Items[i].key));
				memset(&(mp_Items[i].val), 0, sizeof(mp_Items[i].val));
				mp_Items[i].used = false;
				return true;
			}
		}
		return false;
	};
};
