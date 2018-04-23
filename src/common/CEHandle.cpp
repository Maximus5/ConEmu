
/*
Copyright (c) 2018-present Maximus5
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

#define HIDE_USE_EXCEPTION_INFO

#include "CEHandle.h"

CEHandle::CEHandle()
{
}

CEHandle::~CEHandle()
{
	Close();
}

CEHandle::operator const HANDLE() const
{
	return mh_Handle;
}

CEHandle::operator bool() const
{
	return (mh_Handle && mh_Handle != INVALID_HANDLE_VALUE);
}

bool CEHandle::operator==(const HANDLE& cmp) const
{
	return (mh_Handle == cmp);
}

void CEHandle::Close()
{
	if (operator bool())
	{
		HANDLE h = NULL;
		std::swap(h, mh_Handle);
		CloseHandle(h);
	}
}

CEHandle::CEHandle(CEHandle&& src)
{
	std::swap(mh_Handle, src.mh_Handle);
}

CEHandle::CEHandle(const HANDLE& src)
	: mh_Handle(src)
{
}

const CEHandle& CEHandle::operator=(const HANDLE& src)
{
	if (mh_Handle != src)
	{
		Close();
		mh_Handle = src;
	}
	return *this;
}

const CEHandle& CEHandle::operator=(HANDLE&& src)
{
	if (mh_Handle != src)
	{
		Close();
		mh_Handle = src;
	}
	src = NULL;
	return *this;
}

const CEHandle& CEHandle::operator=(CEHandle&& src)
{
	if (mh_Handle != src.mh_Handle)
	{
		Close();
		std::swap(mh_Handle, src.mh_Handle);
	}
	else
	{
		src.mh_Handle = NULL;
	}
	return *this;
}
