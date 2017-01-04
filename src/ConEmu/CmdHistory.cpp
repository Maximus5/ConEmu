
/*
Copyright (c) 2014-2017 Maximus5
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
#include "CmdHistory.h"

CommandHistory::CommandHistory(int anMaxItemCount)
{
	MaxItemCount = anMaxItemCount;
}

CommandHistory::~CommandHistory()
{
	FreeItems();
}

void CommandHistory::FreeItems()
{
	if (!this)
	{
		_ASSERTE(FALSE && "Was not initialized");
		return;
	}

	INT_PTR iCount = Items.size();

	for (INT_PTR i = 0; i < iCount; i++)
	{
		wchar_t* p = Items[i];
		SafeFree(p);
	}

	Items.eraseall();
}

// Return size in bytes
UINT CommandHistory::CreateMSZ(wchar_t*& rsMSZ)
{
	if (!this)
	{
		_ASSERTE(FALSE && "Was not initialized");
		return 0;
	}

	_ASSERTE(rsMSZ==NULL);
	if (Items.size() <= 0)
	{
		SafeFree(rsMSZ);
		return 0;
	}

	INT_PTR iCount = Items.size();
	UINT cchMax = 0;
	MArray<int> len;
	len.alloc(iCount);

	for (INT_PTR i = 0; i < iCount; i++)
	{
		int iSize = lstrlen(Items[i])+1;
		cchMax += iSize;
		len.push_back(iSize);
	}

	if (!cchMax)
	{
		_ASSERTE(FALSE && "Length failed for non-empty history");
		SafeFree(rsMSZ);
		return 0;
	}

	cchMax = (cchMax + 1) * sizeof(*rsMSZ);

	rsMSZ = (wchar_t*)malloc(cchMax);
	if (!rsMSZ)
	{
		_ASSERTE(rsMSZ!=NULL);
		return 0;
	}

	wchar_t* p = rsMSZ;
	for (INT_PTR i = 0; i < iCount; i++)
	{
		int iSize = len[i];
		_wcscpy_c(p, iSize, Items[i]);
		p += iSize;
	}

	*p = 0;
	_ASSERTE((p > rsMSZ) && (*(p) == 0 && *(p-1) == 0));

	return cchMax;
}

void CommandHistory::ParseMSZ(LPCWSTR pszzMSZ)
{
	if (!this)
	{
		_ASSERTE(FALSE && "Was not initialized");
		return;
	}

	FreeItems();

	// Nothing to add?
	if (!pszzMSZ || !*pszzMSZ)
	{
		return;
	}

	int iCount = 0;

	while ((*pszzMSZ) && (iCount < MaxItemCount))
	{
		int iLen = lstrlen(pszzMSZ);
		Add(pszzMSZ, false);
		pszzMSZ += (iLen + 1);
		iCount++;
	}
}

void CommandHistory::Add(LPCWSTR asCommand, bool bFront /*= true*/)
{
	if (!this)
	{
		_ASSERTE(FALSE && "Was not initialized");
		return;
	}

	if (!asCommand || !*asCommand)
	{
		_ASSERTE(asCommand && *asCommand);
		return;
	}

	wchar_t* p;
	INT_PTR iCount;

	// bFront==false is used during History loading,
	// no need to remove duplicates from settings
	if (bFront)
	{
		// Already on first place?
		if (!Items.empty())
		{
			p = Items[0];
			if (p && lstrcmp(p, asCommand) == 0)
			{
				return;
			}
		}

		// First, remove asCommand from list, if it was there already
		iCount = Items.size();
		while (iCount > 0)
		{
			p = Items[--iCount];
			if (!p || lstrcmp(p, asCommand) == 0)
			{
				SafeFree(p);
				Items.erase(iCount);
			}
		}

		// Second, trim history tail to MaxItemCount
		iCount = Items.size();
		while ((iCount > 0) && (iCount >= MaxItemCount))
		{
			p = Items[--iCount];
			SafeFree(p);
			Items.erase(iCount);
		}
	}

	p = lstrdup(asCommand);
	if (!p)
		return;

	if (bFront)
		Items.insert(0, p);
	else
		Items.push_back(p);
}

LPCWSTR CommandHistory::Get(int index)
{
	if (!this)
	{
		_ASSERTE(FALSE && "Was not initialized");
		return NULL;
	}

	if (index < 0 || index >= Items.size())
		return NULL;

	LPCWSTR pszItem = Items[index];
	if (!pszItem || !*pszItem)
	{
		_ASSERTE(pszItem && *pszItem);
		return NULL;
	}

	return pszItem;
}

int CommandHistory::Compare(int index, LPCWSTR asCommand)
{
	if (!this)
	{
		_ASSERTE(FALSE && "Was not initialized");
		return -1;
	}

	int iCmp;

	LPCWSTR pszItem = Get(index);

	if (!pszItem || !*pszItem)
		iCmp = -1;
	else if (!asCommand || !*asCommand)
		iCmp = 1;
	else
		iCmp = lstrcmp(pszItem, asCommand);

	return iCmp;
}
