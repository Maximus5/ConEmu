
/*
Copyright (c) 2009-present Maximus5
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

#include "Memory.h"

#ifndef _MARRAY_HEADER_
#define _MARRAY_HEADER_

#ifndef MCHKHEAP
	#define MCHKHEAP
#endif

#ifdef _DEBUG
#define _ARRAY_ASSERTE(x) if (!(x)) { throw "MArray assertion"; }
#else
#define _ARRAY_ASSERTE(x)
#endif

#include <vector>

#ifndef NOMARRAYSORT
#include <algorithm>
#endif

template <typename T>
class MArrayAllocator
{
public:
	using value_type = T;

	~MArrayAllocator() { };
	MArrayAllocator()
	{
		HeapInitialize();
	};
	template<class U> MArrayAllocator(const MArrayAllocator<U>& other)
	{
		HeapInitialize();
	};
	template<class U> MArrayAllocator(MArrayAllocator<U>&& other) = delete;
	template<class U> MArrayAllocator<T>& operator=(const MArrayAllocator<U>& other) = delete;
	template<class U> bool operator==(const MArrayAllocator<U>& other) const { return true; };
	template<class U> bool operator!=(const MArrayAllocator<U>& other) const { return false; };

	T* allocate(std::size_t n)
	{
		T* ptr = (T*)malloc(n * sizeof(T));
		if (!ptr)
			throw std::bad_alloc();
		return ptr;
	};

	void deallocate( T* p, std::size_t n )
	{
		if (p) free(p);
	};
};

template <typename _Ty>
class MArray
{
	public:
		using vector_type = std::vector<_Ty, MArrayAllocator<_Ty>>;
		using iterator = typename vector_type::iterator;
		using const_iterator = typename vector_type::const_iterator;

	private:
		vector_type data;

	public:

		void swap(MArray<_Ty>& obj)
		{
			data.swap(obj.data);
		}

		iterator begin()
		{
			return data.begin();
		};
		iterator end()
		{
			return data.end();
		};
		const_iterator begin() const
		{
			return data.cbegin();
		};
		const_iterator end() const
		{
			return data.cend();
		};
		const_iterator cbegin() const
		{
			return data.cbegin();
		};
		const_iterator cend() const
		{
			return data.cend();
		};

		ssize_t size() const
		{
			ssize_t count = data.size();
			_ARRAY_ASSERTE(count >= 0);
			return count;
		};
		ssize_t capacity() const
		{
			ssize_t count = data.capacity();
			_ARRAY_ASSERTE(count >= 0);
			return count;
		};
		bool empty() const
		{
			return data.empty();
		};

		// UB if _Index is invalid
		const _Ty & operator[](ssize_t _Index) const
		{
			_ARRAY_ASSERTE(!(_Index<0 || _Index>=size()));
			return data[_Index];
		};
		// UB if _Index is invalid
		_Ty & operator[](ssize_t _Index)
		{
			_ARRAY_ASSERTE(!(_Index<0 || _Index>=size()));
			return data[_Index];
		};

		ssize_t push_back(const _Ty& _Item)
		{
			ssize_t nPos = size();
			data.push_back(_Item);
			return nPos;
		};
		ssize_t push_back(_Ty&& _Item)
		{
			ssize_t nPos = size();
			data.push_back(std::move(_Item));
			return nPos;
		};
		/// Insert at the end if nPosBefore==-1 or greater than current size
		ssize_t insert(ssize_t nPosBefore, const _Ty& _Item, ssize_t _Count = 1)
		{
			if (_Count <= 0)
				return -1;
			ssize_t inserted;
			if ((nPosBefore < 0) || (nPosBefore >= size()))
			{
				inserted = data.size();
				data.insert(data.end(), _Count, _Item);
			}
			else
			{
				inserted = nPosBefore;
				data.insert(data.begin()+nPosBefore, _Count, _Item);
			}
			return inserted;
		};
		/// Insert at the end if nPosBefore==-1 or greater than current size
		ssize_t insert(ssize_t nPosBefore, _Ty&& _Item)
		{
			ssize_t inserted;
			if ((nPosBefore < 0) || (nPosBefore >= size()))
			{
				#ifdef _DEBUG
				inserted = data.size();
				#endif
				inserted = push_back(std::move(_Item));  // -V519
			}
			else
			{
				inserted = nPosBefore;
				data.insert(data.begin()+nPosBefore, std::move(_Item));
			}
			return inserted;
		};
		/// Resized if _Index is >= size()
		ssize_t set_at(ssize_t _Index, const _Ty& _Item)
		{
			if (_Index < 0)
			{
				_ARRAY_ASSERTE(_Index>=0);  // -V547
				return -1;
			}

			if (_Index >= size())
			{
				if (!resize(_Index+1))
					return -1;
			}

			data[_Index] = _Item;
			return _Index;
		};

		bool pop_back(_Ty& _Item)
		{
			if (data.empty())
			{
				// this implementation of pop_back is used in cycles a lot
				// _ARRAY_ASSERTE(!data.empty());
				return false;
			}
			_Item = std::move(data[data.size()-1]);
			data.pop_back();
			return true;
		};

		void clear()
		{
			data.clear();
		};
		void erase(ssize_t _Index, ssize_t _Count = 1)
		{
			if (_Index < 0 || _Index + _Count > size() || _Count <= 0)
			{
				_ARRAY_ASSERTE((_Index>=0 && _Index+_Count<=size()) || _Count==0);
				return;
			}
			data.erase(data.begin() + _Index, data.begin() + _Index + _Count);
		};

		/// Increase or decrease count of elements
		bool resize(ssize_t nNewCount)
		{
			if (nNewCount < 0)
			{
				_ARRAY_ASSERTE(nNewCount>0);  // -V547
				return false;
			}
			data.resize(nNewCount);
			return true;
		}

		bool reserve(ssize_t nNewCount)
		{
			if (nNewCount < 0)
			{
				_ARRAY_ASSERTE(nNewCount>0);  // -V547
				return false;
			}
			data.reserve(nNewCount);
			return true;
		}


		#ifndef NOMARRAYSORT
		void sort(bool(*lessFunction)(const _Ty& e1, const _Ty& e2))
		{
			std::sort(data.begin(), data.end(), lessFunction);
		};
		#endif
};

#endif // #ifndef _MARRAY_HEADER_
