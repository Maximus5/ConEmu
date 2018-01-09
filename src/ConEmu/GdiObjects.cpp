
/*
Copyright (c) 2018 Maximus5
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
#define SHOWDEBUGSTR

#include "Header.h"

#include "ConEmu.h"
#include "GdiObjects.h"

#define DEBUGSTRCOUNT(s) DEBUGSTR(s)

HandleMonitor::HandleMonitor()
{
}

HandleMonitor::~HandleMonitor()
{
}

void HandleMonitor::DoCheck()
{
	wchar_t szInfo[120];

	DWORD GdiObjCount = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
	DWORD UserObjCount = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);

	if (!m_GDIObjects)
		m_GDIObjects = GdiObjCount;
	else if (GdiObjCount > m_GDIObjects)
	{
		swprintf_c(szInfo, L"GDIObjects increase: %u >> %u", m_GDIObjects, GdiObjCount);
		if (!gpConEmu->LogString(szInfo))
			DEBUGSTRCOUNT(szInfo);
		m_GDIObjects = GdiObjCount;
	}

	if (!m_UserObjects)
		m_UserObjects = UserObjCount;
	else if (UserObjCount > m_UserObjects)
	{
		swprintf_c(szInfo, L"UserObjects increase: %u >> %u", m_UserObjects, UserObjCount);
		if (!gpConEmu->LogString(szInfo))
			DEBUGSTRCOUNT(szInfo);
		m_UserObjects = UserObjCount;
	}
}
