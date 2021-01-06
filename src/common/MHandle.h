
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
	std::function<void(HANDLE)> closeFunc_ = nullptr;

public:
	// ReSharper disable once CppNonExplicitConvertingConstructor
	// ReSharper disable once CppParameterMayBeConst
	MHandle(HANDLE handle)
		: handle_(handle)
	{
	}

	MHandle(HANDLE handle, std::function<void(HANDLE)> closeFunc)
		: handle_(handle), closeFunc_(std::move(closeFunc))
	{
	}

	MHandle()
	{
	}

	~MHandle()
	{
		Close();
	}

	// non-copyable
	MHandle(const MHandle&) = delete;
	MHandle& operator=(const MHandle&) = delete;
	// movable
	MHandle(MHandle&&) = default;
	MHandle& operator=(MHandle&&) = default;

public:
	bool HasHandle() const
	{
		return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE;
	}

	operator HANDLE() const
	{
		return handle_;
	}

	HANDLE GetHandle() const
	{
		return handle_;
	}

	HANDLE SetHandle(HANDLE handle)
	{
		if (handle_ != handle)
			Close();
		handle_ = handle;
		closeFunc_ = nullptr;
		return handle_;
	}

	HANDLE SetHandle(HANDLE handle, std::function<void(HANDLE)> closeFunc)
	{
		if (handle_ != handle)
			Close();
		handle_ = handle;
		closeFunc_ = std::move(closeFunc);
		return handle_;
	}

	void Close()
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
