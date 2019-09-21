
/*
Copyright (c) 2012-present Maximus5
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

template<class KEY_T,class VAL_T,UINT HASH_SHIFT=0>
struct MMap
{
protected:
	struct MapItem
	{
		LONG  used;
		KEY_T key;
		VAL_T val;
	};
	struct MapItems
	{
		MapItem  *pItems;
		size_t    nCount;
		MapItems *pNextBlock;
	};
	MapItems *mp_FirstBlock = nullptr;
	MapItems *mp_LastBlock = nullptr;
	size_t mn_ItemsInBlock = 0; // max number of items in one block
	size_t mn_MaxCount = 0;     // informational

	MapItems* AllocateBlock()
	{
		_ASSERTE(mn_ItemsInBlock!=0);
		size_t cbBlockSize = sizeof(MapItems) + mn_ItemsInBlock*sizeof(MapItem);
		MapItems* pItems = (MapItems*)calloc(cbBlockSize,1);
		if (!pItems)
		{
			return NULL;
		}
		pItems->pItems = (MapItem*)(pItems+1);
		pItems->nCount = mn_ItemsInBlock;
		return pItems;
	};

	LONG UsedHash(const KEY_T& key)
	{
		_ASSERTE(sizeof(KEY_T) >= sizeof(LONG));
		LONG hash;
		if (!HASH_SHIFT)
			hash = *(LONG*)&key;
		else if (sizeof(KEY_T) >= sizeof(unsigned __int64))
			hash = (LONG)(((*(unsigned __int64*)&key) >> HASH_SHIFT) & 0xFFFFFFFF);
		else
			hash = (LONG)((*(DWORD*)&key) >> HASH_SHIFT);
		return hash ? hash : -1;
	}

	bool FindBlockItem(const KEY_T& key, MapItems*& pBlock, MapItem*& pItem)
	{
		if (!mp_FirstBlock)
		{
			_ASSERTE(mp_FirstBlock && "MMap::Get - Storage must be allocated first");
			return false;
		}

		LONG hash = UsedHash(key);

		pBlock = mp_FirstBlock;
		while (pBlock)
		{
			pItem = pBlock->pItems;
			size_t nMaxCount = pBlock->nCount;

			for (; nMaxCount--; pItem++)
			{
				// In case of KEY_T may contain zero value, we compare "key" too
				if ((pItem->used == hash) && (key == pItem->key))
				{
					return true;
				}
			}

			pBlock = pBlock->pNextBlock;
		}

		return false;
	};

public:
	MMap()
	{
	};
	~MMap()
	{
		// Don't use ‘Release()’ if heap is already deinitialized
		// That may happen because MMap is used for global static variables
		if (IsHeapInitialized())
		{
			Release();
		}
	};

public:
	bool Init(size_t nMaxCount = 256, bool bOnCreate = false)
	{
		// Required for proper UsedHash result
		_ASSERTE(sizeof(KEY_T) >= sizeof(LONG));

		if (bOnCreate)
			memset(this, 0, sizeof(*this));
		_ASSERTE(mp_FirstBlock == NULL);

		mn_ItemsInBlock = nMaxCount;

		mp_LastBlock = mp_FirstBlock = AllocateBlock();
		if (!mp_FirstBlock)
		{
			_ASSERTE(mp_FirstBlock!=NULL && "Failed to allocate MMap storage");
			return false;
		}
		// Informational
		mn_MaxCount = nMaxCount;

		return true;
	};

	bool Initialized()
	{
		return (mp_FirstBlock != NULL);
	};

	void Release()
	{
		mn_MaxCount = 0;
		if (mp_FirstBlock)
		{
			MapItems* pBlock = mp_FirstBlock;
			while (pBlock)
			{
				MapItems* p = pBlock;
				pBlock = pBlock->pNextBlock;
				free(p);
			}
			mp_FirstBlock = NULL;
		}
	};

	void Reset()
	{
		if (mp_FirstBlock)
		{
			MapItems* pBlock = mp_FirstBlock;
			while (pBlock)
			{
				memset(pBlock->pItems, 0, pBlock->nCount * sizeof(*pBlock->pItems));
				pBlock = pBlock->pNextBlock;
			}
		}
	};

	bool Get(const KEY_T& key, VAL_T* val, bool Remove = false)
	{
		if (!mp_FirstBlock)
		{
			_ASSERTE(mp_FirstBlock && "MMap::Get - Storage must be allocated first");
			return false;
		}

		MapItems* pBlock;
		MapItem* pItem;

		LONG hash = UsedHash(key);

		if (FindBlockItem(key, pBlock, pItem))
		{
			if (val)
			{
				*val = pItem->val;
			}

			if (Remove)
			{
				memset(&(pItem->key), 0, sizeof(pItem->key));  // -V568
				memset(&(pItem->val), 0, sizeof(pItem->val));  // -V568
				InterlockedCompareExchange(&pItem->used, 0, hash);
			}

			return true;
		}

		return false;
	};

	/// Returns copy of all keys/values in map
	/// @param  pKeys - [in] pointer to (KEY_T*), may be NULL
	///         [out] caller is responsible to release allocated
	///         memory via ReleasePointer call
	/// @param  pValues - [in] pointer to (VAL_T*), may be NULL
	///         [out] caller is responsible to release allocated
	///         memory via ReleasePointer call
	/// @result count of elements in pList or -1 on errors
	INT_PTR GetKeysValues(KEY_T** pKeys, VAL_T** pValues)
	{
		if (!mp_FirstBlock || !mn_MaxCount)
		{
			_ASSERTE(mp_FirstBlock && "MMap::Get - Storage must be allocated first");
			return -1;
		}

		INT_PTR nAllCount = 0;
		INT_PTR nMaxCount = mn_MaxCount;

		if (pKeys)
		{
			*pKeys = (KEY_T*)calloc(sizeof(KEY_T), nMaxCount);
			if (!*pKeys)
			{
				_ASSERTE(FALSE && "Allocation failure");
				return -1;
			}
		}
		if (pValues)
		{
			*pValues = (VAL_T*)calloc(sizeof(VAL_T), nMaxCount);
			if (!*pValues)
			{
				free(pKeys);
				_ASSERTE(FALSE && "Allocation failure");
				return -1;
			}
		}

		MapItems* pBlock = mp_FirstBlock;
		while (pBlock)
		{
			MapItem* pItem = pBlock->pItems;
			size_t nMaxCount = pBlock->nCount;

			for (; nMaxCount--; pItem++)
			{
				if (pItem->used)
				{
					if (pKeys)
						(*pKeys)[nAllCount] = pItem->key;
					if (pValues)
						(*pValues)[nAllCount] = pItem->val;
					nAllCount++;

					// It's possible that due to multithreading
					// new block appears during GetValues cycle
					if (nAllCount == nMaxCount)
					{
						break;
					}
				}
			}

			pBlock = pBlock->pNextBlock;
		}

		return nAllCount;
	};

	/// Call this function to release memory, allocated by GetValues or GetKeys
	void ReleasePointer(void* ptr)
	{
		free(ptr);
	};

	typedef bool(*MMapEnumCallback)(const KEY_T& next_key, const VAL_T& next_val, LPARAM lParam);

	/// Simple one-by-one enumerator
	/// Loop through elements while EnumCallback return true
	/// @result true if EnumCallback was returned true
	bool EnumKeysValues(MMapEnumCallback EnumCallback, LPARAM lParam)
	{
		if (!mp_FirstBlock)
		{
			_ASSERTE(mp_FirstBlock && "MMap::Get - Storage must be allocated first");
			return false;
		}
		if (!EnumCallback)
		{
			return false;
		}

		MapItems* pBlock = mp_FirstBlock;
		while (pBlock)
		{
			MapItem* pItem = pBlock->pItems;
			size_t nMaxCount = pBlock->nCount;

			for (; nMaxCount--; pItem++)
			{
				if (pItem->used)
				{
					if (EnumCallback(pItem->key, pItem->val, lParam))
						return true;
				}
			}

			pBlock = pBlock->pNextBlock;
		}

		return false;
	};

	/// Simple one-by-one enumerator
	/// @param  prev     - [IN]  pass NULL to get first element,
	///                          or pointer to variable with previous key
	/// @param  next_key - [IN]  pointer to KEY_T variable, or NULL
	///                    [OUT] set to found key on success
	/// @param  next_val - [IN]  pointer to VAL_T variable, or NULL
	///                    [OUT] set to found key on success
	/// @result Returns true if element found
	bool GetNext(const KEY_T* prev, KEY_T* next_key, VAL_T* next_val)
	{
		if (!mp_FirstBlock)
		{
			_ASSERTE(mp_FirstBlock && "MMap::Get - Storage must be allocated first");
			return false;
		}

		MapItems* pBlock;
		MapItem* pItem;

		if (prev != NULL)
		{
			// Find previous key
			if (!FindBlockItem(*prev, pBlock, pItem))
			{
				_ASSERTE(FALSE && "Multithreading? Item was removed during iteration");
				return false;
			}

			// Key found, goto next
			if ((++pItem) >= (pBlock->pItems + pBlock->nCount))
			{
				pBlock = pBlock->pNextBlock;
				pItem = pBlock ? pBlock->pItems : NULL;
			}
		}
		else
		{
			// First item was requested
			pBlock = mp_FirstBlock;
			pItem = mp_FirstBlock->pItems;
		}

		// Loop while non-empty element found
		while (pBlock)
		{
			MapItem* pEnd = pBlock->pItems + pBlock->nCount;

			for (; pItem && pItem < pEnd; pItem++)
			{
				if (pItem->used)
				{
					if (next_key)
						*next_key = pItem->key;

					if (next_val)
						*next_val = pItem->val;

					// Found
					return true;
				}
			}

			pBlock = pBlock->pNextBlock;
			pItem = pBlock ? pBlock->pItems : NULL;
		}

		// No more elements
		return false;
	};

	bool Set(const KEY_T& key, const VAL_T& val)
	{
		if (!mp_FirstBlock)
		{
			_ASSERTE(mp_FirstBlock && "MMap::Set - Storage must be allocated first");

			// Allow automatic initialization
			if (!Init())
			{
				return false;
			}
		}

		MapItems* pBlock;
		MapItem* pItem;

		if (FindBlockItem(key, pBlock, pItem))
		{
			pItem->val = val;

			// Multithreading issues?
			_ASSERTE(pItem->used && (pItem->key == key) && (0 == memcmp(&pItem->val, &val, sizeof(val))));

			return true;
		}

		LONG hash = UsedHash(key);

		// Add new
		pBlock = mp_FirstBlock;
		while (pBlock)
		{
			pItem = pBlock->pItems;
			size_t nMaxCount = pBlock->nCount;

			for (; nMaxCount--; pItem++)
			{
				if (!InterlockedCompareExchange(&pItem->used, hash, 0))
				{
					pItem->key = key;
					pItem->val = val;

					// Multithreading issues?
					_ASSERTE(pItem->used && (pItem->key == key) && !memcmp(&pItem->val, &val, sizeof(val)));

					return true;
				}
			}

			pBlock = pBlock->pNextBlock;
		}

		// Allocate and append new block in lock-free style

		pBlock = AllocateBlock();
		if (!pBlock)
		{
			_ASSERTE(pBlock!=NULL);  // -V547
			return false;
		}

		pItem = pBlock->pItems;
		pItem->used = hash;
		pItem->key = key;
		pItem->val = val;

		MapItems* pPrevLastBlock = (MapItems*)InterlockedExchangePointer((PVOID*)&mp_LastBlock, pBlock);
		_ASSERTE(pPrevLastBlock->pNextBlock == NULL);
		pPrevLastBlock->pNextBlock = pBlock;

		// Informational
		mn_MaxCount += pBlock->nCount;

		return true;
	};

	bool Del(const KEY_T& key)
	{
		// If empty or not initialized yed - do nothing
		if (!mp_FirstBlock)
		{
			return false;
		}

		return Get(key, NULL, true);
	};
};
