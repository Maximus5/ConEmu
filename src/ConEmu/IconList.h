
/*
Copyright (c) 2013-2015 Maximus5
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

#include "../common/MArray.h"

struct MSectionSimple;

class CIconList
{
protected:
	struct TabIconCache {
		wchar_t* pszIconDescr;
		int nIconIdx;
		bool bAdmin;
		bool bWasNotLoaded;
	};
	MSectionSimple* mpcs;
	MArray<TabIconCache> m_Icons;
	int mn_SysCxIcon, mn_SysCyIcon; // Ssystem size
	void EvalDefaultTabIconSize(int& cx, int& cy);
	void EvalNearestIconSize(int nQueryY, int& cx, int& cy);
	int CreateTabIconInt(LPCWSTR asIconDescr, bool bAdmin, LPCWSTR asWorkDir);
	struct Dimensions {
		HIMAGELIST hTabIcons;
		int cxIcon, cyIcon;
		int nAdminIcon;
	};
	MArray<Dimensions> m_Dimensions;
	bool CreateDimension(int cx, int cy, Dimensions* pd);
public:

	int mn_AdminIcon;
public:
	CIconList();
	~CIconList();

	bool IsInitialized();
	bool Initialize();

	int GetTabIcon(bool bAdmin);
	int CreateTabIcon(LPCWSTR asIconDescr, bool bAdmin, LPCWSTR asWorkDir);

	HICON GetTabIconByIndex(int IconIndex);

	static LPCWSTR GetIconInfoStr(HICON h, wchar_t (&szInfo)[80]);

	HIMAGELIST GetImageList(int nMaxHeight = 0);
};
