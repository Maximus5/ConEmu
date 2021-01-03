
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

#include "defines.h"

template <class T>
class MFileMapping final
{
protected:
	HANDLE mapHandle_ = nullptr;
	T* dataPtr_ = nullptr; // WARNING!!! the access could be "read-only"
	bool isWriteAllowed_ = false;
	bool isNewlyCreated_ = false;
	const uint32_t typeSize_;
	uint32_t mappedSize_ = 0;
	wchar_t mapName_[128] = L"";
	MEMORY_BASIC_INFORMATION mapInfo_{};
	uint32_t lastError_ = 0;
	uint32_t exceptionCode_ = 0;
	wchar_t lastErrorText_[MAX_PATH * 2] = L"";

public:
	MFileMapping() : typeSize_(sizeof(T))
	{
	}

	~MFileMapping()
	{
		if (mapHandle_)
			CloseMap();
	}

	MFileMapping(const MFileMapping&) = delete;
	MFileMapping(MFileMapping&&) = delete;
	MFileMapping& operator=(const MFileMapping&) = delete;
	MFileMapping& operator=(MFileMapping&&) = delete;

public:
	T* Ptr()
	{
		return dataPtr_;
	}

	// ReSharper disable once CppNonExplicitConversionOperator
	operator T* ()
	{
		return dataPtr_;
	}

	bool IsValid()
	{
		return (dataPtr_ != nullptr);
	}

	LPCWSTR GetErrorText() const
	{
		return lastErrorText_;
	}

	uint32_t GetLastErrorCode() const
	{
		return lastError_;
	}

	uint32_t GetLastExceptionCode() const
	{
		return exceptionCode_;
	}

	MEMORY_BASIC_INFORMATION GetMapInfo() const
	{
		return mapInfo_;
	}

	uint32_t GetMaxSize() const
	{
		return mappedSize_;
	}

#ifndef CONEMU_MINIMAL
	bool SetFrom(const T* pSrc, const int nSize = -1)
	{
		lastError_ = 0;
		exceptionCode_ = 0;

		if (!IsValid())
		{
			wcscpy_c(lastErrorText_, L"Internal error. SetFrom failed: mapping is not valid.");
			return false;
		}
		if (!pSrc || !nSize)
		{
			wcscpy_c(lastErrorText_, L"Internal error. SetFrom failed: invalid arguments.");
			return false;
		}

		const uint32_t sizeToCopy = (nSize < 0) ? typeSize_ : nSize;
		if (sizeToCopy > mappedSize_)
		{
			msprintf(lastErrorText_, countof(lastErrorText_), L"Internal error. SetFrom failed: size %u is too large.", sizeToCopy);
			return false;
		}

		__try  // NOLINT(clang-diagnostic-language-extension-token)
		{
			if (memcmp(dataPtr_, pSrc, sizeToCopy) == 0) //-V106
			{
				return false; // nothing was changed
			}
			memmove_s(dataPtr_, mappedSize_, pSrc, sizeToCopy); //-V106
			return true;
		}
		__except((GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR || GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
			? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
		{
			lastError_ = GetLastError();
			exceptionCode_ = GetExceptionCode();
			msprintf(lastErrorText_, countof(lastErrorText_), L"Internal error. SetFrom failed: exception 0x%08X.", exceptionCode_);
			return false;
		}
	}
#endif

	bool GetTo(typename std::remove_const<T>::type* pDst, const int nSize = -1)
	{
		lastError_ = 0;
		exceptionCode_ = 0;

		if (!IsValid() || !nSize)
			return false;

		const uint32_t dstSize = (nSize < 0) ? typeSize_ : nSize;
		const uint32_t sizeToCopy = (std::min<uint32_t>)(dstSize, mappedSize_);

		__try  // NOLINT(clang-diagnostic-language-extension-token)
		{
			memmove_s(pDst, dstSize, dataPtr_, sizeToCopy); //-V106
			return true;
		}
		__except((GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR || GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
			? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
		{
			lastError_ = GetLastError();
			exceptionCode_ = GetExceptionCode();
			msprintf(lastErrorText_, countof(lastErrorText_), L"Internal error. GetTo failed: exception 0x%08X.", exceptionCode_);
			return false;
		}
	}

public:
	LPCWSTR InitName(const wchar_t* aszTemplate, DWORD Parm1 = 0, DWORD Parm2 = 0)
	{
		if (mapHandle_)
			CloseMap();

		msprintf(mapName_, countof(mapName_), aszTemplate, Parm1, Parm2);
		return mapName_;
	}

	void ClosePtr()
	{
		if (dataPtr_)
		{
			UnmapViewOfFile(static_cast<const void*>(dataPtr_));
			dataPtr_ = nullptr;
		}
	}

	void CloseMap()
	{
		if (dataPtr_)
			ClosePtr();

		if (mapHandle_)
		{
			CloseHandle(mapHandle_);
			mapHandle_ = nullptr;
		}

		mapHandle_ = nullptr;
		isWriteAllowed_ = false;
		dataPtr_ = nullptr;
		mappedSize_ = 0;
		lastError_ = 0;
		mapInfo_ = {};
	}

protected:
	// mappedSize_ and nSize are used only during creation or copy operations
	T* InternalOpenCreate(const bool abCreate, const bool abReadWrite, const int nSize)
	{
		if (mapHandle_)
			CloseMap();

		lastError_ = 0;
		lastErrorText_[0] = 0;
		_ASSERTE(mapHandle_ == nullptr && dataPtr_ == nullptr);
		_ASSERTE(nSize == -1 || nSize >= static_cast<INT_PTR>(sizeof(T)));

		if (mapName_[0] == 0)
		{
			_ASSERTE(mapName_[0] != 0);  // -V547
			wcscpy_c(lastErrorText_, L"Internal error. Mapping file name was not specified.");
			return nullptr;
		}

		mappedSize_ = (nSize <= 0) ? typeSize_ : nSize; //-V105 //-V103
		isWriteAllowed_ = abCreate || abReadWrite;

		const DWORD nFlags = isWriteAllowed_ ? (FILE_MAP_READ | FILE_MAP_WRITE) : FILE_MAP_READ;

		if (abCreate)
		{
			mapHandle_ = CreateFileMapping(INVALID_HANDLE_VALUE,
				LocalSecurity(), PAGE_READWRITE, 0, mappedSize_, mapName_);
		}
		else
		{
			mapHandle_ = OpenFileMapping(nFlags, false, mapName_);
		}
		lastError_ = GetLastError();

		if (!mapHandle_ || mapHandle_ == INVALID_HANDLE_VALUE)
		{
			isNewlyCreated_ = false;
			msprintf(lastErrorText_, countof(lastErrorText_), L"Can't %s console data file mapping. ErrCode=0x%08X\n%s",
				abCreate ? L"create" : L"open", lastError_, mapName_);
			mapHandle_ = nullptr;
			return nullptr;
		}

		isNewlyCreated_ = abCreate && lastError_ != ERROR_ALREADY_EXISTS;

		dataPtr_ = static_cast<T*>(MapViewOfFile(mapHandle_, nFlags, 0, 0, 0));

		if (!dataPtr_)
		{
			lastError_ = GetLastError();
			msprintf(lastErrorText_, countof(lastErrorText_), L"Can't map console info (%s). ErrCode=0x%08X\n%s",
				isWriteAllowed_ ? L"ReadWrite" : L"Read", lastError_, mapName_);
			return nullptr;
		}

		if (!VirtualQuery(dataPtr_, &mapInfo_, sizeof(mapInfo_)))
		{
			lastError_ = GetLastError();
			msprintf(lastErrorText_, countof(lastErrorText_),
				L"Can't map console info (%s). VirtualQuery failed, ErrCode=0x%08X\n%s",
				isWriteAllowed_ ? L"ReadWrite" : L"Read", lastError_, mapName_);
			CloseMap();
			return nullptr;
		}

		if (mapInfo_.RegionSize < mappedSize_)
		{
			msprintf(lastErrorText_, countof(lastErrorText_),
				L"Can't map console info (%s). Allocated size %u is smaller than expected %u\n%s",
				isWriteAllowed_ ? L"ReadWrite" : L"Read", static_cast<uint32_t>(mapInfo_.RegionSize), mappedSize_, mapName_);
			CloseMap();
			return nullptr;
		}

		return dataPtr_;
	}

public:
#ifndef CONEMU_MINIMAL
	T* Create(const int nSize = -1)
	{
		_ASSERTE(nSize == -1 || nSize >= sizeof(T));
		return InternalOpenCreate(true/*abCreate*/, true/*abReadWrite*/, nSize);
	}
#endif

	T* Open(const bool abReadWrite = false/*false - only Read*/, const int nSize = -1)
	{
		_ASSERTE(nSize == -1 || nSize >= static_cast<INT_PTR>(sizeof(T)));
		return InternalOpenCreate(false/*abCreate*/, abReadWrite, nSize);
	}

	const T* ReopenForRead()
	{
		if (mapHandle_)
		{
			if (dataPtr_)
				ClosePtr();

			isWriteAllowed_ = false;

			const DWORD nFlags = FILE_MAP_READ;
			dataPtr_ = static_cast<T*>(MapViewOfFile(mapHandle_, nFlags, 0, 0, 0));

			if (!dataPtr_)
			{
				lastError_ = GetLastError();
				msprintf(lastErrorText_, countof(lastErrorText_), L"Can't map console info (%s). ErrCode=0x%08X\n%s",
					isWriteAllowed_ ? L"ReadWrite" : L"Read", lastError_, mapName_);
			}
		}
		return dataPtr_;
	}
};
