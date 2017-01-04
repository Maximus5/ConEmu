
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

class CVirtualConsole;

class CTaskBarGhost
{
protected:
	static ATOM mh_Class;
	HWND mh_Ghost;
	bool mb_TaskbarRegistered;
	struct {
		POINT VConSize;   // то, что было раньше в самом m_TabSize(cx/cy)
		POINT BitmapSize; // размер сформированной превьюшки
		RECT  UsedRect;   // Если VConSize < BitmapSize - то VCon "центрируется", UsedRect - где оно отрисовано
	} m_TabSize;
	HBITMAP mh_Snap;
	BITMAPINFO mbmi_Snap;
	PBYTE mpb_DS;
	BOOL mb_SimpleBlack;
	CVirtualConsole* mp_VCon;
	DWORD mn_LastUpdate;
	UINT mn_MsgUpdateThumbnail;
	HANDLE mh_SkipActivateEvent;
	bool mb_WasSkipActivate;
	wchar_t ms_LastTitle[1024];
	POINT mpt_Offset, mpt_ViewOffset, mpt_Size, mpt_ViewSize;
protected:
	static LRESULT CALLBACK GhostStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT GhostProc(UINT message, WPARAM wParam, LPARAM lParam);
	HBITMAP CreateThumbnail(int nWidth, int nHeight);
	bool CalcThumbnailSize(int nWidth, int nHeight, int &nShowWidth, int &nShowHeight);
	void UpdateGhostSize();
	void GetPreviewPosSize(POINT* pPtOffset, POINT* pPtViewOffset, POINT* pPtSize, POINT* pPtViewSize);

	// Обработка сообщений
	LRESULT OnClose();
	LRESULT OnDestroy();
	LRESULT OnDwmSendIconicLivePreviewBitmap();
	LRESULT OnDwmSendIconicThumbnail(short anWidth, short anHeight);
	HICON OnGetIcon(WPARAM anIconType);
	LRESULT OnSysCommand(WPARAM wParam, LPARAM lParam);
	LRESULT OnActivate(WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer(WPARAM TimerID);
	LRESULT OnCreate();

public:
	CTaskBarGhost(CVirtualConsole* apVCon);
	virtual ~CTaskBarGhost();
public:
	static CTaskBarGhost* Create(CVirtualConsole* apVCon);
	BOOL CreateTabSnapshot();
	BOOL UpdateTabSnapshot(BOOL abSimpleBlack = FALSE, BOOL abNoSnapshot = FALSE);
	LPCWSTR CheckTitle(BOOL abSkipValidation = FALSE);
	void ActivateTaskbar();
	bool NeedSnapshotCache();
	HWND GhostWnd();
};
