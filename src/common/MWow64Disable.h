
/*
Copyright (c) 2009-2017 Maximus5
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

#include <windows.h>

// Класс отключения редиректора системных библиотек.
class MWow64Disable
{
	protected:
		typedef BOOL (WINAPI* Wow64DisableWow64FsRedirection_t)(PVOID* OldValue);
		typedef BOOL (WINAPI* Wow64RevertWow64FsRedirection_t)(PVOID OldValue);
		Wow64DisableWow64FsRedirection_t _Wow64DisableWow64FsRedirection;
		Wow64RevertWow64FsRedirection_t _Wow64RevertWow64FsRedirection;

		BOOL mb_Disabled;
		PVOID m_OldValue;
	public:
		void Disable()
		{
			if (!mb_Disabled && _Wow64DisableWow64FsRedirection)
			{
				mb_Disabled = _Wow64DisableWow64FsRedirection(&m_OldValue);
			}
		};
		void Restore()
		{
			if (mb_Disabled)
			{
				mb_Disabled = FALSE;

				if (_Wow64RevertWow64FsRedirection)
					_Wow64RevertWow64FsRedirection(m_OldValue);
			}
		};
	public:
		MWow64Disable()
		{
			mb_Disabled = FALSE; m_OldValue = NULL;
			HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");

			if (hKernel)
			{
				_Wow64DisableWow64FsRedirection = (Wow64DisableWow64FsRedirection_t)GetProcAddress(hKernel, "Wow64DisableWow64FsRedirection");
				_Wow64RevertWow64FsRedirection = (Wow64RevertWow64FsRedirection_t)GetProcAddress(hKernel, "Wow64RevertWow64FsRedirection");
			}
			else
			{
				_Wow64DisableWow64FsRedirection = NULL;
				_Wow64RevertWow64FsRedirection = NULL;
			}
		};
		~MWow64Disable()
		{
			Restore();
		};
};
