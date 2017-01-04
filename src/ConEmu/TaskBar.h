
/*
Copyright (c) 2011-2017 Maximus5
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

#ifndef __ITaskbarList4_FWD_DEFINED__
#define __ITaskbarList4_FWD_DEFINED__
typedef interface ITaskbarList4 ITaskbarList4;
#endif 	/* __ITaskbarList4_FWD_DEFINED__ */

#ifndef __ITaskbarList3_FWD_DEFINED__
#define __ITaskbarList3_FWD_DEFINED__
typedef interface ITaskbarList3 ITaskbarList3;

// -- defined in <ShObjIdl.h> or "ShObjIdl_Part.h"
//enum TBPFLAG
//{
//	TBPF_NOPROGRESS	= 0,
//	TBPF_INDETERMINATE	= 0x1,
//	TBPF_NORMAL	= 0x2,
//	TBPF_ERROR	= 0x4,
//	TBPF_PAUSED	= 0x8
//} 	TBPFLAG;
#endif 	/* __ITaskbarList3_FWD_DEFINED__ */

#ifndef __ITaskbarList2_FWD_DEFINED__
#define __ITaskbarList2_FWD_DEFINED__
typedef interface ITaskbarList2 ITaskbarList2;
#endif 	/* __ITaskbarList2_FWD_DEFINED__ */

#ifndef __ITaskbarList_FWD_DEFINED__
#define __ITaskbarList_FWD_DEFINED__
typedef interface ITaskbarList ITaskbarList;
#endif 	/* __ITaskbarList_FWD_DEFINED__ */

class CConEmuMain;

class CTaskBar
{
protected:
	CConEmuMain* mp_ConEmu;
	ITaskbarList4 *mp_TaskBar4; // Win7
	ITaskbarList3 *mp_TaskBar3; // Win7
	ITaskbarList2 *mp_TaskBar2; // WinXP
	ITaskbarList  *mp_TaskBar1; // Win2k
	HICON mh_Shield;
	bool  mb_OleInitalized;
protected:
	// We need ordered array, so MMap is not suitable here
	MArray<LPVOID> m_Ghosts;
	void Taskbar_GhostAppend(LPVOID pVCon);
	void Taskbar_GhostRemove(LPVOID pVCon);
public:
	void Taskbar_GhostReorder();
	bool isTaskbarSmallIcons();
public:
	CTaskBar(CConEmuMain* apOwner);
	virtual ~CTaskBar();

	void Taskbar_Init();
	void Taskbar_Release();

	HRESULT Taskbar_SetActiveTab(HWND hBtn);
	HRESULT Taskbar_RegisterTab(HWND hBtn, LPVOID pVCon, BOOL abSetActive = FALSE);
	HRESULT Taskbar_UnregisterTab(HWND hBtn, LPVOID pVCon);
	HRESULT Taskbar_AddTabXP(HWND hBtn);
	HRESULT Taskbar_DeleteTabXP(HWND hBtn);
	HRESULT Taskbar_SetProgressValue(int nProgress);
	HRESULT Taskbar_SetProgressState(UINT/*TBPFLAG*/ nState);
	HRESULT Taskbar_MarkFullscreenWindow(HWND hwnd, BOOL fFullscreen);

	bool Taskbar_GhostSnapshotRequired();

	void Taskbar_SetShield(bool abShield);
	void Taskbar_SetOverlay(HICON ahIcon);
	void Taskbar_UpdateOverlay();
};
