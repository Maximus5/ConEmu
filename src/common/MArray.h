
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

#ifndef _MARRAY_HEADER_
#define _MARRAY_HEADER_

//#pragma warning( disable : 4101 )

#ifndef MCHKHEAP
	#define MCHKHEAP
#endif

#ifdef _DEBUG
#define _ARRAY_ASSERTE(x) if (!(x)) { throw "MArray assertion"; }
#else
#define _ARRAY_ASSERTE(x)
#endif

#include <type_traits>
#include <iterator>

template<typename _Ty>
class MArray
{
	protected:
		const ssize_t mn_TySize = sizeof(_Ty);
		ssize_t mn_Elements = 0;
		ssize_t mn_MaxSize = 0;
		#ifdef _DEBUG
		ssize_t mn_Reserved;
		#endif
		_Ty*    mp_Elements = nullptr;

		void _DestroyElements(ssize_t from, ssize_t to)
		{
			_ARRAY_ASSERTE(from >= 0 && from < mn_Elements && to >= from && to <= mn_Elements);
			if (!std::is_trivially_destructible<_Ty>::value)
			{
				if (mp_Elements)
				{
					for (ssize_t i = from; i < to; ++i)
					{
						mp_Elements[i].~_Ty();
					}
				}
			}
		}

	public:
		template <typename T>
		struct iterator_impl : public std::iterator<std::random_access_iterator_tag, T>
		{
		private:
			T* ptr = nullptr;
		public:
			typedef iterator_impl<T> _Unchecked_type;

			iterator_impl(T* _ptr) : ptr(_ptr)
			{
			};

			T& operator*() const
			{
				_ARRAY_ASSERTE(ptr);
				return *ptr;
			};
			T* operator->() const
			{
				_ARRAY_ASSERTE(ptr);
				return ptr;
			};

			iterator_impl<T>& operator++()
			{
				++ptr;
				return *this;
			};
			iterator_impl<T>& operator--()
			{
				--ptr;
				return *this;
			};
			iterator_impl<T>& operator+=(ssize_t index)
			{
				ptr += index;
				return *this;
			};
			iterator_impl<T>& operator-=(ssize_t index)
			{
				ptr -= index;
				return *this;
			};
			iterator_impl<T> operator+(ssize_t index) const
			{
				return iterator_impl<T>(ptr+index);
			};
			iterator_impl<T> operator-(ssize_t index) const
			{
				return iterator_impl<T>(ptr-index);
			};
			ptrdiff_t operator-(const iterator_impl<T>& iter) const
			{
				return ptr - iter.ptr;
			};

			bool operator<(const iterator_impl<T>& iter) const
			{
				return (ptr < iter.ptr);
			};
			bool operator!=(const iterator_impl<T>& iter) const
			{
				return (ptr != iter.ptr);
			};
			bool operator==(const iterator_impl<T>& iter) const
			{
				return (ptr == iter.ptr);
			};
		};
		using iterator = iterator_impl<_Ty>;
		using const_iterator = iterator_impl<const _Ty>;

	public:
		MArray()
		{
			#ifdef _DEBUG
			{
				// Debug purposes
				#ifdef _WIN64
				mn_Reserved = 0xCCCCCCCCCCCCCCCCLL;
				#else
				mn_Reserved = 0xCCCCCCCC;
				#endif
			}
			#endif
			MCHKHEAP;
			if (mn_TySize==0)
			{
				_ARRAY_ASSERTE(!(mn_TySize==0));
				return;
			}
			MCHKHEAP;
		};
		~MArray()
		{
			clear();
		};

		MArray(const MArray<_Ty>&) = delete;
		MArray(MArray<_Ty>&&) = delete;

		void swap(MArray<_Ty>& obj)
		{
			_ARRAY_ASSERTE(mn_TySize == obj.mn_TySize);

			std::swap(mn_Elements, obj.mn_Elements);
			std::swap(mn_MaxSize, obj.mn_MaxSize);
			std::swap(mp_Elements, obj.mp_Elements);

			#ifdef _DEBUG
			std::swap(mn_Reserved, obj.mn_Reserved);
			#endif
		}

		iterator begin()
		{
			return mp_Elements;
		};
		iterator end()
		{
			return mp_Elements + mn_Elements;
		};
		const_iterator cbegin() const
		{
			return mp_Elements;
		};
		const_iterator cend() const
		{
			return mp_Elements + mn_Elements;
		};

		// UB if _Index is invalid
		const _Ty & operator[](ssize_t _Index) const
		{
			#ifdef _DEBUG
			if (_Index<0 || _Index>=mn_Elements)
			{
				_ARRAY_ASSERTE(!(_Index<0 || _Index>=mn_Elements));
			}
			const _Ty& _Item = mp_Elements[_Index];
			#endif
			return mp_Elements[_Index];
		};
		// UB if _Index is invalid
		_Ty & operator[](ssize_t _Index)
		{
			#ifdef _DEBUG
			if (_Index<0 || _Index>=mn_Elements)
			{
				_ARRAY_ASSERTE(!(_Index<0 || _Index>=mn_Elements));
			}
			_Ty& _Item = mp_Elements[_Index];
			#endif
			return mp_Elements[_Index];
		};

		ssize_t push_back(const _Ty& _Item)
		{
			if (mn_TySize==0)
			{
				_ARRAY_ASSERTE(!(mn_TySize==0));
				return -1;
			}
			MCHKHEAP;
			if (mn_MaxSize <= mn_Elements)
			{
				_ARRAY_ASSERTE(mn_MaxSize==mn_Elements);
				addsize(std::max<ssize_t>(256, mn_MaxSize));
			}
			// op begin
			ssize_t nPos = mn_Elements;
			{
				new (mp_Elements + nPos) _Ty(_Item);
				++mn_Elements;
			}
			// op end
			MCHKHEAP;
			return nPos;
		};
		ssize_t push_back(_Ty&& _Item)
		{
			if (mn_TySize==0)
			{
				_ARRAY_ASSERTE(!(mn_TySize==0));
				return -1;
			}
			MCHKHEAP;
			if (mn_MaxSize <= mn_Elements)
			{
				_ARRAY_ASSERTE(mn_MaxSize==mn_Elements);
				addsize(std::max<ssize_t>(256, mn_MaxSize));
			}
			// op begin
			ssize_t nPos = mn_Elements;
			{
				new (mp_Elements + nPos) _Ty(std::move(_Item));
				++mn_Elements;
			}
			// op end
			MCHKHEAP;
			return nPos;
		};
		void insert(ssize_t nPosBefore, const _Ty& _Item)
		{
			if (mn_TySize==0)
			{
				_ARRAY_ASSERTE(!(mn_TySize==0));
				return;
			}
			if ((nPosBefore < 0) || (nPosBefore >= mn_Elements))
			{
				push_back(_Item);
				return;
			}
			MCHKHEAP;
			if (mn_MaxSize<=mn_Elements)
			{
				_ARRAY_ASSERTE(mn_MaxSize==mn_Elements);
				addsize(std::max<ssize_t>(256, mn_MaxSize));
			}
			_ARRAY_ASSERTE(mn_Elements > nPosBefore && nPosBefore >= 0);
			_ARRAY_ASSERTE(mn_MaxSize > mn_Elements);
			// op begin
			{
				auto s_begin = this->begin()+nPosBefore, s_end = this->end();
				new (mp_Elements + mn_Elements) _Ty();
				++mn_Elements;
				std::move_backward(s_begin, s_end, this->end());
				mp_Elements[nPosBefore] = _Item;
			}
			// op end
			MCHKHEAP;
		};
		void insert(ssize_t nPosBefore, _Ty&& _Item)
		{
			if (mn_TySize==0)
			{
				_ARRAY_ASSERTE(!(mn_TySize==0));
				return;
			}
			if ((nPosBefore < 0) || (nPosBefore >= mn_Elements))
			{
				push_back(std::move(_Item));
				return;
			}
			MCHKHEAP;
			if (mn_MaxSize<=mn_Elements)
			{
				_ARRAY_ASSERTE(mn_MaxSize==mn_Elements);
				addsize(std::max<ssize_t>(256, mn_MaxSize));
			}
			_ARRAY_ASSERTE(mn_Elements > nPosBefore && nPosBefore >= 0);
			_ARRAY_ASSERTE(mn_MaxSize > mn_Elements);
			// op begin
			{
				auto s_begin = this->begin()+nPosBefore, s_end = this->end();
				new (mp_Elements + mn_Elements) _Ty();
				++mn_Elements;
				std::move_backward(s_begin, s_end, this->end());
				mp_Elements[nPosBefore] = std::move(_Item);
			}
			// op end
			MCHKHEAP;
		};
		bool pop_back(_Ty& _Item)
		{
			if ((mn_TySize==0) || (mn_Elements<=0))
			{
				_ARRAY_ASSERTE((!(mn_TySize==0))&&(mn_Elements>=0));
				return false;
			}
			MCHKHEAP;
			_Item = std::move(mp_Elements[mn_Elements-1]);
			this->_DestroyElements(mn_Elements-1, mn_Elements);
			MCHKHEAP;
			--mn_Elements;
			return true;
		};
		_Ty* detach()
		{
			_Ty* p = nullptr;
			std::swap(p, mp_Elements);
			clear();
			return p;
		};
		void clear()
		{
			if (mp_Elements)
			{
				if (mn_Elements > 0)
					this->_DestroyElements(0, mn_Elements);
				free(mp_Elements);
				mp_Elements = NULL;
			}
			mn_MaxSize = 0; mn_Elements = 0;
			MCHKHEAP;
		};
		void erase(ssize_t _Index)
		{
			if (_Index < 0 || _Index >= mn_Elements)
			{
				_ARRAY_ASSERTE(!(_Index<0 || _Index>=mn_Elements));
				return;
			}

			// op begin
			if ((_Index+1) < mn_Elements)
			{
				MCHKHEAP;
				std::move(this->begin()+_Index+1, this->end(), this->begin()+_Index);
				MCHKHEAP;
			}
			this->_DestroyElements(mn_Elements - 1, mn_Elements);
			mn_Elements--;
			// op end
		};
		void eraseall()
		{
			if (mn_Elements > 0)
				this->_DestroyElements(0, mn_Elements);
			mn_Elements = 0;
		};

		ssize_t size() const
		{
			return mn_Elements;
		};

		ssize_t capacity() const
		{
			return mn_MaxSize;
		};

		bool empty() const
		{
			return (mn_Elements == 0);
		};

		bool addsize(ssize_t nElements)
		{
			if (mn_TySize==0)
			{
				_ARRAY_ASSERTE(!(mn_TySize==0));
				return false;
			}
			if (nElements < 0)
			{
				_ARRAY_ASSERTE(nElements > 0);
				return false;
			}
			MCHKHEAP;
			ssize_t nNewMaxSize = mn_MaxSize + nElements;
			if (mp_Elements)
			{
				_Ty* ptrNew = (_Ty*)calloc(nNewMaxSize, mn_TySize);
				MCHKHEAP;
				if (ptrNew == nullptr)
				{
					_ARRAY_ASSERTE(ptrNew!=NULL);
					return false;
				}
				std::move(this->begin(), this->end(), iterator(ptrNew));
				MCHKHEAP;
				this->_DestroyElements(0, mn_Elements);
				std::swap(mp_Elements, ptrNew);
				free(ptrNew);
			}
			else
			{
				_ARRAY_ASSERTE(mn_Elements == 0);
				mp_Elements = (_Ty*)calloc(nNewMaxSize, mn_TySize);
				if (mp_Elements==NULL)
				{
					_ARRAY_ASSERTE(mp_Elements!=NULL);
					return false;
				}
			}
			mn_MaxSize = nNewMaxSize;
			MCHKHEAP;
			return true;
		};

		// Increase or decrease count of elements
		bool resize(ssize_t nNewCount)
		{
			if (nNewCount < 0)
			{
				_ARRAY_ASSERTE(nNewCount >= 0);
				nNewCount = 0;
			}

			if (mn_MaxSize < nNewCount)
			{
				if (!addsize(nNewCount - mn_MaxSize))
					return false;
			}

			_ARRAY_ASSERTE(mn_MaxSize >= nNewCount);

			while (mn_Elements < nNewCount)
			{
				new (mp_Elements + mn_Elements) _Ty();
				++mn_Elements;
			}

			if (mn_Elements > nNewCount)
			{
				this->_DestroyElements(nNewCount, mn_Elements);
				mn_Elements = nNewCount;
			}

			_ARRAY_ASSERTE(mn_Elements == nNewCount);

			return true;
		}

		#ifndef NOMARRAYSORT
		void sort(int(*compareFunction)(const _Ty* e1, const _Ty* e2))
		{
			MCHKHEAP;
			if (mp_Elements && mn_Elements > 1)
			{
				using qsortFunction = int (__cdecl *)(const void *,const void *);
				qsort(mp_Elements, mn_Elements, mn_TySize, (qsortFunction)compareFunction);
			}
			MCHKHEAP;
		};
		#endif

		bool reserve(ssize_t _nCount)
		{
			if (mn_TySize==0)
			{
				_ARRAY_ASSERTE(!(mn_TySize==0));
				return false;
			}
			if (_nCount>mn_Elements)
			{
				if (_nCount>mn_MaxSize)
				{
					addsize(_nCount-mn_MaxSize);
				}
				// 01.03.2005 !!! Only allocate memory, Not initialize values!
				//mn_Elements = std::min(_nCount,mn_MaxSize);
			}
			return true;
		};

		void set_at(ssize_t _Index, _Ty & _Item)
		{
			if (mn_TySize==0)
			{
				_ARRAY_ASSERTE(!(mn_TySize==0));
				return;
			}
			#ifdef _DEBUG
			if (_Index<0/* || _Index>=mn_Elements*/)
			{
				_ARRAY_ASSERTE(!(_Index<0/* || _Index>=mn_Elements*/));
			}
			#endif

			if (_Index>=mn_Elements)
			{
				if (_Index>=mn_MaxSize)
				{
					addsize(std::max<ssize_t>(256, (_Index-mn_MaxSize+1)));
				}
				mn_Elements = _Index+1;
			}

			#ifdef _DEBUG
			_Ty _ItemDbg = ((_Ty*)mp_Elements)[_Index];
			#endif

			MCHKHEAP;
			memmove(
				((_Ty*)mp_Elements)+(_Index),
				&_Item, mn_TySize);
			MCHKHEAP;
		};
};

//#pragma warning( default : 4101 )

#endif // #ifndef _MARRAY_HEADER_
