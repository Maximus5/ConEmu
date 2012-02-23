
/*
Copyright (c) 2009-2012 Maximus5
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


#include "Header.h"
#include "../common/common.hpp"
#include "../common/WinObjects.h"
#include "ConEmu.h"
#include "VConGroup.h"
#include "VConChild.h"
#include "Options.h"
#include "TabBar.h"
#include "VirtualConsole.h"
#include "RealConsole.h"

#define DEBUGSTRDRAW(s) //DEBUGSTR(s)
#define DEBUGSTRTABS(s) //DEBUGSTR(s)
#define DEBUGSTRLANG(s) //DEBUGSTR(s)

CVConGroup::CVConGroup()
{
	mp_VActive = NULL;
	memset(mp_VCon, 0, sizeof(mp_VCon));
}

CVConGroup::~CVConGroup()
{
	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i])
		{
			CVirtualConsole* p = mp_VCon[i];
			mp_VCon[i] = NULL;
			p->Release();
		}
	}
}

CVConGuard& CVConGroup::ActiveVCon()
{
	CVConGuard VCon(mp_VActive);
	return VCon;
}

// ¬озвращает индекс (0-based) активной консоли
int CVConGroup::GetActiveVCon()
{
	if (mp_VActive)
	{
		for (size_t i = 0; i < countof(mp_VCon); i++)
		{
			if (mp_VCon[i] == mp_VActive)
				return i;
		}
	}
	return -1;
}

// nIdx - 0 based
CVConGuard& CVConGroup::GetVCon(int nIdx)
{
	CVConGuard VCon;
	
	if (nIdx < 0 || nIdx >= (int)countof(mp_VCon))
	{
		_ASSERTE(nIdx>=0 && nIdx<(int)countof(mp_VCon));
	}
	else
	{
		VCon = mp_VCon[nIdx];
	}

	return VCon;
}

CVConGuard& CVConGroup::GetVConFromPoint(POINT ptScreen)
{
	CVConGuard VCon;

	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if ((pVCon = mp_VCon[i]) != NULL && pVCon->isVisible())
		{
			
			HWND hView = pVCon->GetView();
			if (hView)
			{
				RECT rcView; GetWindowRect(hView, &rcView);

				if (PtInRect(&rcView, ptScreen))
				{
					VCon = pVCon;
					break;
				}
			}
			else
			{
				_ASSERTE(pVCon->GetView()!=NULL);
			}
		}
	}

	return VCon;
}
