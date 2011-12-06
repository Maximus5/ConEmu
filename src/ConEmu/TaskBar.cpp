
/*
Copyright (c) 2011 Maximus5
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

#include "header.h"
#include "TaskBar.h"
#include "ConEmu.h"

// COM TaskBarList interface support
#ifdef __GNUC__
#include "ShObjIdl_Part.h"
const CLSID CLSID_TaskbarList = {0x56FDF344, 0xFD6D, 0x11d0, {0x95, 0x8A, 0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90}};
const IID IID_ITaskbarList4 = {0xc43dc798, 0x95d1, 0x4bea, {0x90, 0x30, 0xbb, 0x99, 0xe2, 0x98, 0x3a, 0x1a}};
const IID IID_ITaskbarList3 = {0xea1afb91, 0x9e28, 0x4b86, {0x90, 0xe9, 0x9e, 0x9f, 0x8a, 0x5e, 0xef, 0xaf}};
const IID IID_ITaskbarList2 = {0x602D4995, 0xB13A, 0x429b, {0xA6, 0x6E, 0x19, 0x35, 0xE4, 0x4F, 0x43, 0x17}};
const IID IID_ITaskbarList  = {0x56FDF342, 0xFD6D, 0x11d0, {0x95, 0x8A, 0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90}};
#else
#include <ShObjIdl.h>
#ifndef __ITaskbarList3_INTERFACE_DEFINED__
#undef __shobjidl_h__
#include "ShObjIdl_Part.h"
const IID IID_ITaskbarList4 = {0xc43dc798, 0x95d1, 0x4bea, {0x90, 0x30, 0xbb, 0x99, 0xe2, 0x98, 0x3a, 0x1a}};
const IID IID_ITaskbarList3 = {0xea1afb91, 0x9e28, 0x4b86, {0x90, 0xe9, 0x9e, 0x9f, 0x8a, 0x5e, 0xef, 0xaf}};
#endif
#endif

CTaskBar::CTaskBar()
{
	mp_TaskBar1 = NULL;
	mp_TaskBar2 = NULL;
	mp_TaskBar3 = NULL;
	mp_TaskBar4 = NULL;
}

CTaskBar::~CTaskBar()
{
	Taskbar_Release();
}

void CTaskBar::Taskbar_Init()
{
	HRESULT hr = S_OK;

	if (!mp_TaskBar1)
	{
		// ¬ PostCreate это выполн€етс€ дольше всего. ѕо идее мешать не должно,
		// т.к. серверна€ нить уже запущена.
		hr = CoCreateInstance(CLSID_TaskbarList,NULL,CLSCTX_INPROC_SERVER,IID_ITaskbarList,(void**)&mp_TaskBar1);
	
		if (hr == S_OK && mp_TaskBar1)
		{
			hr = mp_TaskBar1->HrInit();
		}
	
		if (hr != S_OK && mp_TaskBar1)
		{
			if (mp_TaskBar1) mp_TaskBar1->Release();
	
			mp_TaskBar1 = NULL;
		}
	}
	
	if (!mp_TaskBar2 && mp_TaskBar1)
	{
		hr = mp_TaskBar1->QueryInterface(IID_ITaskbarList2, (void**)&mp_TaskBar2);
	}
	
	if (!mp_TaskBar3 && mp_TaskBar2)
	{
		hr = mp_TaskBar2->QueryInterface(IID_ITaskbarList3, (void**)&mp_TaskBar3);
	}
	
	if (!mp_TaskBar4 && mp_TaskBar2)
	{
		hr = mp_TaskBar2->QueryInterface(IID_ITaskbarList4, (void**)&mp_TaskBar4);
	}
}

void CTaskBar::Taskbar_Release()
{
	if (mp_TaskBar4)
	{
		mp_TaskBar4->Release();
		mp_TaskBar4 = NULL;
	}

	if (mp_TaskBar3)
	{
		mp_TaskBar3->Release();
		mp_TaskBar3 = NULL;
	}

	if (mp_TaskBar2)
	{
		mp_TaskBar2->Release();
		mp_TaskBar2 = NULL;
	}

	if (mp_TaskBar1)
	{
		mp_TaskBar1->Release();
		mp_TaskBar1 = NULL;
	}
}

HRESULT CTaskBar::Taskbar_SetActiveTab(HWND hBtn)
{
	HRESULT hr = E_NOINTERFACE;

	if (mp_TaskBar3)
	{
		// 3-й параметр в писании описан как Reserved
		hr = mp_TaskBar3->SetTabActive(hBtn, ghWnd, 0);
	}
	else if (mp_TaskBar2)
	{
		hr = mp_TaskBar2->ActivateTab(hBtn);
		hr = mp_TaskBar2->SetActiveAlt(hBtn);
	}

	return hr;
}

bool CTaskBar::Taskbar_GhostSnapshootRequired()
{
	_ASSERTE(mp_TaskBar1!=NULL);
	return gpConEmu->IsDwm();
		//(gOSVer.dwMajorVersion >= 6);
}

HRESULT CTaskBar::Taskbar_RegisterTab(HWND hBtn, BOOL abSetActive)
{
	HRESULT hr;

	_ASSERTE(mp_TaskBar1!=NULL);
	
    // Tell the taskbar about this tab window
	if (mp_TaskBar3)
	{
		hr = mp_TaskBar3->RegisterTab(hBtn, ghWnd);
		hr = mp_TaskBar3->SetTabOrder(hBtn, 0);
	}
	else if (mp_TaskBar1)
	{
		//ShowWindow(hBtn, SW_SHOWNA);
		hr = mp_TaskBar1->AddTab(hBtn);
	}
	else
	{
		hr = E_NOINTERFACE;
	}
	
	if (SUCCEEDED(hr) && abSetActive)
	{
		hr = Taskbar_SetActiveTab(hBtn);
	}

	if (mp_TaskBar4)
	{
		hr = mp_TaskBar4->SetTabProperties(hBtn, STPF_NONE/*STPF_USEAPPTHUMBNAILWHENACTIVE|STPF_USEAPPPEEKWHENACTIVE*/);
	}
	
	return hr;
}

HRESULT CTaskBar::Taskbar_UnregisterTab(HWND hBtn)
{
	HRESULT hr;
	
	if (mp_TaskBar3)
	{
		hr = mp_TaskBar3->UnregisterTab(hBtn);
	}
	else if (mp_TaskBar1)
	{
		hr = mp_TaskBar1->DeleteTab(hBtn);
	}
	else
	{
		hr = E_NOINTERFACE;
	}
	
	return hr;
}

HRESULT CTaskBar::Taskbar_AddTabXP(HWND hBtn)
{
	HRESULT hr;

	// 111127 на Vista тоже кнопку "убирать" нужно
	_ASSERTE(gpConEmu && (gOSVer.dwMajorVersion <= 5 || (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 0)));

	if (mp_TaskBar1)
	{
		hr = mp_TaskBar1->AddTab(hBtn);
	}
	else
	{
		hr = E_NOINTERFACE;
	}

	return hr;
}

HRESULT CTaskBar::Taskbar_DeleteTabXP(HWND hBtn)
{
	HRESULT hr;

	// 111127 на Vista тоже кнопку "убирать" нужно
	_ASSERTE(gpConEmu && (gOSVer.dwMajorVersion <= 5 || (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 0)));

	if (mp_TaskBar1)
	{
		hr = mp_TaskBar1->DeleteTab(hBtn);
	}
	else
	{
		hr = E_NOINTERFACE;
	}

	return hr;
}
