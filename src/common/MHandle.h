
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
#include <functional>

class MHandle final
{
private:
	HANDLE handle_ = nullptr;
	std::function<void(HANDLE) noexcept> closeFunc_ = nullptr;

public:
	// ReSharper disable once CppNonExplicitConvertingConstructor
	// ReSharper disable once CppParameterMayBeConst
	MHandle(HANDLE handle) noexcept
		: handle_(handle)
	{
	}

	MHandle(HANDLE handle, std::function<void(HANDLE) noexcept> closeFunc) noexcept
		: handle_(handle), closeFunc_(std::move(closeFunc))
	{
	}

	MHandle() noexcept
	{
	}

	~MHandle() noexcept
	{
		Close();
	}

	// non-copyable
	MHandle(const MHandle&) = delete;
	MHandle& operator=(const MHandle&) = delete;
	// movable
	MHandle(MHandle&& src) noexcept
	{
		handle_ = src.handle_;
		closeFunc_ = std::move(src.closeFunc_);
		src.handle_ = nullptr;
	}
	MHandle& operator=(MHandle&& src) noexcept
	{
		if (handle_ != src.handle_)
			Close();
		handle_ = src.handle_;
		closeFunc_ = std::move(src.closeFunc_);
		src.handle_ = nullptr;
		return *this;
	}

	MHandle Duplicate(const DWORD desiredAccess, const bool inheritHandle = false) const
	{
		MHandle result{nullptr, CloseHandle};
		if (HasHandle())
		{
			auto* currentProcess = GetCurrentProcess();
			if (!::DuplicateHandle(currentProcess, GetHandle(), currentProcess, &result.handle_, desiredAccess, inheritHandle, 0))
			{
				result.Close();
			}
		}
		return result;
	}

public:
	bool HasHandle() const noexcept
	{
		return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE;
	}

	operator HANDLE() const noexcept
	{
		return handle_;
	}

	HANDLE GetHandle() const noexcept
	{
		return handle_;
	}

	bool SetHandle(HANDLE handle) noexcept
	{
		if (handle_ != handle)
			Close();
		handle_ = handle;
		closeFunc_ = nullptr;
		return HasHandle();
	}

	bool SetHandle(HANDLE handle, std::function<void(HANDLE)> closeFunc) noexcept
	{
		if (handle_ != handle)
			Close();
		handle_ = handle;
		closeFunc_ = std::move(closeFunc);
		return HasHandle();
	}

	void Close() noexcept
	{
		if (HasHandle() && closeFunc_ != nullptr)
		{
			HANDLE handle = nullptr;
			std::swap(handle, handle_);
			closeFunc_(handle);
		}
		closeFunc_ = nullptr;
		handle_ = nullptr;
	}
};
