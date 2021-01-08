
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

class MWnd final
{
private:
	HWND hwnd_ = nullptr;
public:
	// ReSharper disable once CppNonExplicitConvertingConstructor
	// ReSharper disable once CppParameterMayBeConst
	MWnd(HWND hwnd) noexcept
		: hwnd_(hwnd)
	{
	}

	MWnd() noexcept
	{
	}

	~MWnd() noexcept = default;

	// copyable
	MWnd(const MWnd&) noexcept = default;
	MWnd& operator=(const MWnd&) noexcept = default;
	// movable
	MWnd(MWnd&&) noexcept = default;
	MWnd& operator=(MWnd&&) noexcept = default;

public:
	bool HasHandle() const noexcept
	{
		return (hwnd_ != nullptr);
	}

	operator HWND() const noexcept
	{
		return hwnd_;
	}

	HWND GetHandle() const noexcept
	{
		return hwnd_;
	}

	// All windows handles in Windows (64-bit or 32-bit) are available to both 32-bit and 64-bit processes
	DWORD GetPortableHandle() const noexcept
	{
		const auto dword = LODWORD(hwnd_);
		if (reinterpret_cast<DWORD_PTR>(hwnd_) != dword)
		{
			_ASSERTE(reinterpret_cast<DWORD_PTR>(hwnd_) == dword);
			return 0;
		}
		return dword;
	}

	// ReSharper disable once CppParameterMayBeConst
	HWND SetHandle(HWND hwnd) noexcept
	{
		hwnd_ = hwnd;
		return hwnd_;
	}

	HWND SetPortableHandle(const DWORD portableHandle) noexcept
	{
		hwnd_ = reinterpret_cast<HWND>(static_cast<DWORD_PTR>(portableHandle));
		return hwnd_;
	}
};
