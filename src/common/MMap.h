
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
	bool Init(size_t nMaxCount = 255)
	{
		_ASSERTE(mp_Items == NULL);
		mp_Items = (MapItems*)calloc(nMaxCount,sizeof(MapItems));
		if (!mp_Items)
		{
			_ASSERTE(mp_Items!=NULL && "Failed to allocate MMap storage");
			return false;
		}
		mn_MaxCount = nMaxCount;
		return true;
	};

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

	bool Set(const KEY_T& key, const VAL_T& val)
	{
		if (!mp_Items || !mn_MaxCount)
		{
			_ASSERTE(mp_Items && mn_MaxCount && "MMap::Set - Storage must be allocated first");
			return false;
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
		_ASSERTE(FALSE && "Can't add item to MMap, not enough storage");
		return false;
	};

	void Del(const KEY_T& key)
	{
		if (!mp_Items || !mn_MaxCount)
		{
			_ASSERTE(mp_Items && mn_MaxCount && "MMap::Set - Storage must be allocated first");
			return;
		}
		for (size_t i = 0; i < mn_MaxCount; i++)
		{
			if (mp_Items[i].used && (key == mp_Items[i].key))
			{
				memset(&(mp_Items[i].key), 0, sizeof(mp_Items[i].key));
				memset(&(mp_Items[i].val), 0, sizeof(mp_Items[i].val));
				mp_Items[i].used = false;
				return;
			}
		}
	};
};
