
/*
Copyright (c) 2015-2017 Maximus5
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

#define DEBUGSTRWIN(s) //DEBUGSTR(s)

#include <windows.h>
#include "header.h"
#include "WinObjBase.h"
#include "../common/MMap.h"

// struct TabPanelWinMap { CTabPanelWin* object; HWND hWnd; WNDPROC defaultProc; };
static MMap<HWND,TabPanelWinMap>* gp_TabPanelWinMap = NULL;

CWinObjBase::CWinObjBase()
{
	InitObjMap();
}

CWinObjBase::~CWinObjBase()
{
	// gp_TabPanelWinMap is life-time object
	// DON'T destroy it here!
}

bool CWinObjBase::InitObjMap()
{
	if (!gp_TabPanelWinMap)
	{
		gp_TabPanelWinMap = (MMap<HWND,TabPanelWinMap>*)calloc(1,sizeof(*gp_TabPanelWinMap));
		gp_TabPanelWinMap->Init(8); // Rebar+Tabbar+Searchbar+Toolbar, only 4 windows at the moment max
	}

	return (gp_TabPanelWinMap!=NULL);
}

CWinObjBase* CWinObjBase::GetObj(HWND hWnd, TabPanelWinMap* val, bool Remove /*= false*/)
{
	TabPanelWinMap objMap = {};

	if (!gp_TabPanelWinMap || !gp_TabPanelWinMap->Get(hWnd, &objMap, Remove))
		return NULL;
	if (val)
		*val = objMap;

	return objMap.object;
}

void CWinObjBase::SetObj(HWND hWnd, CWinObjBase* object, WNDPROC defaultProc)
{
	if (!InitObjMap())
	{
		_ASSERTE(FALSE && "Memory allocation?");
		return;
	}

	TabPanelWinMap objMap = {};
	objMap.object = object;
	objMap.hWnd = hWnd;
	objMap.defaultProc = defaultProc;

	gp_TabPanelWinMap->Set(hWnd, objMap);
}
