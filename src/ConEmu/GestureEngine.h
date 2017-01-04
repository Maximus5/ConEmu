
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

#if defined(__GNUC__) && (WINVER < 0x0601)

	#define HGESTUREINFO HANDLE

	typedef struct tagGESTURECONFIG {
	    DWORD dwID;                     // gesture ID
	    DWORD dwWant;                   // settings related to gesture ID that are to be turned on
	    DWORD dwBlock;                  // settings related to gesture ID that are to be turned off
	} GESTURECONFIG, *PGESTURECONFIG;
	typedef struct tagGESTUREINFO {
	    UINT cbSize;                    // size, in bytes, of this structure (including variable length Args field)
	    DWORD dwFlags;                  // see GF_* flags
	    DWORD dwID;                     // gesture ID, see GID_* defines
	    HWND hwndTarget;                // handle to window targeted by this gesture
	    POINTS ptsLocation;             // current location of this gesture
	    DWORD dwInstanceID;             // internally used
	    DWORD dwSequenceID;             // internally used
	    ULONGLONG ullArguments;         // arguments for gestures whose arguments fit in 8 BYTES
	    UINT cbExtraArgs;               // size, in bytes, of extra arguments, if any, that accompany this gesture
	} GESTUREINFO, *PGESTUREINFO;

	/*
	 * Gesture Messages
	 */
	#define WM_GESTURE                      0x0119
	#define WM_GESTURENOTIFY                0x011A

#endif

class CGestures
{
public:
	CGestures();
	virtual ~CGestures();

	void StartGestureLog();

public:
	bool IsGesturesEnabled();

	// Process WM_GESTURE messages
	virtual bool ProcessGestureMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);

	// This function is called when press and tap gesture is recognized
	virtual void ProcessPressAndTap(HWND hWnd, const LONG ldx, const LONG ldy, const short nDeltaX, const short nDeltaY);

	// This function is invoked when two finger tap gesture is recognized
	virtual void ProcessTwoFingerTap(HWND hWnd, const LONG ldx, const LONG ldy, const ULONG dist);

	// This function is called constantly through duration of zoom in/out gesture
	virtual void ProcessZoom(HWND hWnd, const double dZoomFactor, const LONG lZx, const LONG lZy);

	// This function is called throughout the duration of the panning/inertia gesture
	virtual bool ProcessMove(HWND hWnd, const LONG ldx, const LONG ldy);

	// This function is called throughout the duration of the rotation gesture
	virtual bool ProcessRotate(HWND hWnd, const LONG lAngle, const LONG lOx, const LONG lOy, bool bEnd);

private:
	void SendRClick(HWND hWnd, const LONG ldx, const LONG ldy);
	void DumpGesture(LPCWSTR tp, const GESTUREINFO& gi);
private:
	// При Pan и прочем это первая точка, в отличие от _ptFirst не меняется
	// в процессе перетаскивания и содержит ЭКРАННЫЕ координаты
	POINT _ptBegin;
	// first significant point of the gesture
	POINT _ptFirst;
	// second significant point of the gesture
	POINT _ptSecond;
	// 4 bytes long argument
	DWORD _dwArguments;
	// Rotation was started?
	bool  _inRotate;
private:
	bool _isTabletPC, _isGestures;
	typedef BOOL (WINAPI* GetGestureInfo_t)(HGESTUREINFO hGestureInfo, PGESTUREINFO pGestureInfo);
	GetGestureInfo_t _GetGestureInfo;
	typedef BOOL (WINAPI* SetGestureConfig_t)(HWND hwnd, DWORD dwReserved, UINT cIDs, PGESTURECONFIG pGestureConfig, UINT cbSize);
	SetGestureConfig_t _SetGestureConfig;
	typedef BOOL (WINAPI* CloseGestureInfoHandle_t)(HGESTUREINFO hGestureInfo);
	CloseGestureInfoHandle_t _CloseGestureInfoHandle;
};
