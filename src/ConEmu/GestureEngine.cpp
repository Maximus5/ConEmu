
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR
#include <pshpack8.h> // 8bytes must be (Gesture API)
#include "Header.h"

#define USE_DUMPGEST
//#ifdef USE_DUMPGEST
#define DEBUGSTRPAN(s) DEBUGSTR(s)
//#else
//#define DEBUGSTRGEST(s)
//#endif

#include "GestureEngine.h"
#include "ConEmu.h"
#include "FontMgr.h"
#include "VirtualConsole.h"
#include "VConGroup.h"
#include "RealConsole.h"
#include "TabBar.h"

#if defined(__GNUC__) && (WINVER < 0x0601)
	/*
	 * Gesture flags - GESTUREINFO.dwFlags
	 */
	#define GF_BEGIN                        0x00000001
	#define GF_INERTIA                      0x00000002
	#define GF_END                          0x00000004

	/*
	 * Gesture IDs
	 */
	#define GID_BEGIN                       1
	#define GID_END                         2
	#define GID_ZOOM                        3
	#define GID_PAN                         4
	#define GID_ROTATE                      5
	#define GID_TWOFINGERTAP                6
	#define GID_PRESSANDTAP                 7
	#define GID_ROLLOVER                    GID_PRESSANDTAP

	/*
	 * Gesture configuration flags - GESTURECONFIG.dwWant or GESTURECONFIG.dwBlock
	 */

	/*
	 * Common gesture configuration flags - set GESTURECONFIG.dwID to zero
	 */
	#define GC_ALLGESTURES                              0x00000001

	/*
	 * Zoom gesture configuration flags - set GESTURECONFIG.dwID to GID_ZOOM
	 */
	#define GC_ZOOM                                     0x00000001

	/*
	 * Pan gesture configuration flags - set GESTURECONFIG.dwID to GID_PAN
	 */
	#define GC_PAN                                      0x00000001
	#define GC_PAN_WITH_SINGLE_FINGER_VERTICALLY        0x00000002
	#define GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY      0x00000004
	#define GC_PAN_WITH_GUTTER                          0x00000008
	#define GC_PAN_WITH_INERTIA                         0x00000010

	/*
	 * Rotate gesture configuration flags - set GESTURECONFIG.dwID to GID_ROTATE
	 */
	#define GC_ROTATE                                   0x00000001

	/*
	 * Two finger tap gesture configuration flags - set GESTURECONFIG.dwID to GID_TWOFINGERTAP
	 */
	#define GC_TWOFINGERTAP                             0x00000001

	/*
	 * PressAndTap gesture configuration flags - set GESTURECONFIG.dwID to GID_PRESSANDTAP
	 */
	#define GC_PRESSANDTAP                              0x00000001
	#define GC_ROLLOVER                                 GC_PRESSANDTAP

#endif


#ifndef GID_PRESSANDTAP
	#define GID_PRESSANDTAP GID_ROLLOVER
#endif


// Default constructor of the class.
CGestures::CGestures()
:   _dwArguments(0), _inRotate(false)
{
	DEBUGTEST(GESTUREINFO c = {sizeof(c)});
	_isTabletPC = (GetSystemMetrics(SM_TABLETPC) != FALSE);
	_isGestures = IsWindows7;
	if (_isGestures)
	{
		HMODULE hUser = GetModuleHandle(L"user32.dll");
		_GetGestureInfo = (GetGestureInfo_t)GetProcAddress(hUser, "GetGestureInfo");
		_SetGestureConfig = (SetGestureConfig_t)GetProcAddress(hUser, "SetGestureConfig");
		_CloseGestureInfoHandle = (CloseGestureInfoHandle_t)GetProcAddress(hUser, "CloseGestureInfoHandle");
		if (!_GetGestureInfo || !_SetGestureConfig || !_CloseGestureInfoHandle)
			_isGestures = false;
	}
}

// Destructor
CGestures::~CGestures()
{
}

void CGestures::StartGestureLog()
{
	if (!gpConEmu->mp_Log)
		return;

	wchar_t szInfo[100];
	_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Gestures: TabletPC=%u, Gestures=%u, Enabled=%u", (int)_isTabletPC, (int)_isGestures, (int)IsGesturesEnabled());
	gpConEmu->LogString(szInfo);
}

bool CGestures::IsGesturesEnabled()
{
	if (!_isTabletPC || !_isGestures)
		return false;
	// Финт ушами - считаем, что событие от мыши, если мышиный курсор
	// видим на экране. Если НЕ видим - то событие от тачскрина. Актуально
	// для того, чтобы различать правый клик от мышки и от тачскрина.
	CURSORINFO ci = {sizeof(ci)};
	if (!GetCursorInfo(&ci))
		return false;
	// 0 - cursor is hidden, 2 - touchscreen is used
	if (ci.flags == 0 || (ci.flags & 2/*CURSOR_SUPPRESSED*/))
		return true;
	_ASSERTE(ci.flags == CURSOR_SHOWING);
	return false;
}

void CGestures::DumpGesture(LPCWSTR tp, const GESTUREINFO& gi)
{
	wchar_t szDump[256];

	_wsprintf(szDump, SKIPLEN(countof(szDump))
		L"Gesture(x%08X {%i,%i} %s",
		LODWORD(gi.hwndTarget), gi.ptsLocation.x, gi.ptsLocation.y,
		tp); // tp - имя жеста

	switch (gi.dwID)
	{
	case GID_PRESSANDTAP:
		{
			DWORD h = LODWORD(gi.ullArguments); _wsprintf(szDump+_tcslen(szDump), SKIPLEN(32)
				L" Dist={%i,%i}", (int)(short)LOWORD(h), (int)(short)HIWORD(h));
			break;
		}

	case GID_ROTATE:
		{
			DWORD h = LODWORD(gi.ullArguments); _wsprintf(szDump+_tcslen(szDump), SKIPLEN(32)
				L" %i", (int)LOWORD(h));
			break;
		}
	}

	if (gi.dwFlags&GF_BEGIN)
		wcscat_c(szDump, L" GF_BEGIN");

	if (gi.dwFlags&GF_END)
		wcscat_c(szDump, L" GF_END");

	if (gi.dwFlags&GF_INERTIA)
	{
		wcscat_c(szDump, L" GF_INERTIA");
		DWORD h = HIDWORD(gi.ullArguments); _wsprintf(szDump+_tcslen(szDump), SKIPLEN(32)
			L" {%i,%i}", (int)(short)LOWORD(h), (int)(short)HIWORD(h));
	}



	if (gpSet->isLogging(2))
	{
		gpConEmu->LogString(szDump);
	}
	else
	{
		#ifdef USE_DUMPGEST
		wcscat_c(szDump, L")\n");
		DEBUGSTR(szDump);
		#endif
	}
}

// Main function of this class decodes gesture information
// in:
//      hWnd        window handle
//      wParam      message parameter (message-specific)
//      lParam      message parameter (message-specific)
bool CGestures::ProcessGestureMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	if ((uMsg != WM_GESTURENOTIFY) && (uMsg != WM_GESTURE))
		return false;

	if (!_isGestures)
	{
		_ASSERTE(_isGestures);
		gpConEmu->LogString(L"Gesture message received but not allowed, skipping");
		return false;
	}

	if (uMsg == WM_GESTURENOTIFY)
	{
		// This is the right place to define the list of gestures that this
		// application will support. By populating GESTURECONFIG structure
		// and calling SetGestureConfig function. We can choose gestures
		// that we want to handle in our application. In this app we
		// decide to handle all gestures.
		GESTURECONFIG gc[] = {
			{GID_ZOOM, GC_ZOOM},
			{GID_ROTATE, GC_ROTATE},
			{GID_PAN,
				GC_PAN|GC_PAN_WITH_GUTTER|GC_PAN_WITH_INERTIA,
				GC_PAN_WITH_SINGLE_FINGER_VERTICALLY|GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY
			},
			{GID_PRESSANDTAP, GC_PRESSANDTAP},
			{GID_TWOFINGERTAP, GC_TWOFINGERTAP},
		};

		BOOL bResult = _SetGestureConfig(hWnd, 0, countof(gc), gc, sizeof(GESTURECONFIG));
		DWORD dwErr = GetLastError();

		if (gpSet->isLogging())
		{
			wchar_t szNotify[60];
			_wsprintf(szNotify, SKIPLEN(countof(szNotify)) L"SetGestureConfig -> %u,%u", bResult, dwErr);
			gpConEmu->LogString(szNotify);
		}

		if (!bResult)
		{
			DisplayLastError(L"Error in execution of SetGestureConfig", dwErr);
		}

		lResult = ::DefWindowProc(hWnd, WM_GESTURENOTIFY, wParam, lParam);

		return true;
	}

	// Остался только WM_GESTURE
	Assert(uMsg==WM_GESTURE);

	// helper variables
	POINT ptZoomCenter;
	double k;

	GESTUREINFO gi = {sizeof(gi)};
	// Checking for compiler alignment errors
	if (gi.cbSize != WIN3264TEST(48,56))
	{
		// Struct member alignment must be 8bytes even on x86
		Assert(sizeof(GESTUREINFO)==WIN3264TEST(48,56));
		_isGestures = false;
		return false;
	}
	BOOL bResult = _GetGestureInfo((HGESTUREINFO)lParam, &gi);

	if (!bResult)
	{
		//_ASSERT(L"_GetGestureInfo failed!" && 0);
		DWORD dwErr = GetLastError();
		DisplayLastError(L"Error in execution of _GetGestureInfo", dwErr);
		return FALSE;
	}

	#ifdef USE_DUMPGEST
	bool bLog = (gpSet->isLogging(2) != 0);
	UNREFERENCED_PARAMETER(bLog);
	bLog = true;
	#endif

	#define DUMPGEST(tp) DumpGesture(tp, gi)

	//#ifdef USE_DUMPGEST
	//	wchar_t szDump[256];
	//	#define DUMPGEST(tp)
	//		_wsprintf(szDump, SKIPLEN(countof(szDump))
	//			L"Gesture(x%08X {%i,%i} %s",
	//			(DWORD)gi.hwndTarget, gi.ptsLocation.x, gi.ptsLocation.y,
	//			tp);
	//		if (gi.dwID==GID_PRESSANDTAP) {
	//			DWORD h = LODWORD(gi.ullArguments); _wsprintf(szDump+_tcslen(szDump), SKIPLEN(32)
	//				L" Dist={%i,%i}", (int)(short)LOWORD(h), (int)(short)HIWORD(h)); }
	//		if (gi.dwID==GID_ROTATE) {
	//			DWORD h = LODWORD(gi.ullArguments); _wsprintf(szDump+_tcslen(szDump), SKIPLEN(32)
	//				L" %i", (int)LOWORD(h)); }
	//		if (gi.dwFlags&GF_BEGIN) wcscat_c(szDump, L" GF_BEGIN");
	//		if (gi.dwFlags&GF_END) wcscat_c(szDump, L" GF_END");
	//		if (gi.dwFlags&GF_INERTIA) { wcscat_c(szDump, L" GF_INERTIA");
	//			DWORD h = HIDWORD(gi.ullArguments); _wsprintf(szDump+_tcslen(szDump), SKIPLEN(32)
	//				L" {%i,%i}", (int)(short)LOWORD(h), (int)(short)HIWORD(h)); }
	//		wcscat_c(szDump, L")\n");
	//		DEBUGSTR(szDump)
	//#else
	//#define DUMPGEST(s)
	//#endif

	switch (gi.dwID)
	{
	case GID_BEGIN:
		DUMPGEST(L"GID_BEGIN");
		break;

	case GID_END:
		DUMPGEST(L"GID_END");
		break;

	case GID_ZOOM:
		DUMPGEST(L"GID_ZOOM");
		if (gi.dwFlags & GF_BEGIN)
		{
			_dwArguments = LODWORD(gi.ullArguments);
			_ptFirst.x = gi.ptsLocation.x;
			_ptFirst.y = gi.ptsLocation.y;
			ScreenToClient(hWnd,&_ptFirst);
		}
		else
		{
			// We read here the second point of the gesture. This is middle point between
			// fingers in this new position.
			_ptSecond.x = gi.ptsLocation.x;
			_ptSecond.y = gi.ptsLocation.y;
			ScreenToClient(hWnd,&_ptSecond);

			// We have to calculate zoom center point
			ptZoomCenter.x = (_ptFirst.x + _ptSecond.x)/2;
			ptZoomCenter.y = (_ptFirst.y + _ptSecond.y)/2;

			// The zoom factor is the ratio between the new and the old distance.
			// The new distance between two fingers is stored in gi.ullArguments
			// (lower DWORD) and the old distance is stored in _dwArguments.
			k = (double)(LODWORD(gi.ullArguments))/(double)(_dwArguments);

			// Now we process zooming in/out of the object
			ProcessZoom(hWnd, k, ptZoomCenter.x, ptZoomCenter.y);

			// Now we have to store new information as a starting information
			// for the next step in this gesture.
			_ptFirst = _ptSecond;
			_dwArguments = LODWORD(gi.ullArguments);
		}
		break;

	case GID_PAN:
		DUMPGEST(L"GID_PAN");
		if (gi.dwFlags & GF_BEGIN)
		{
			_ptFirst.x = gi.ptsLocation.x;
			_ptFirst.y = gi.ptsLocation.y;
			_ptBegin.x = gi.ptsLocation.x;
			_ptBegin.y = gi.ptsLocation.y;
			ScreenToClient(hWnd, &_ptFirst);
		}
		else
		{
			// We read the second point of this gesture. It is a middle point
			// between fingers in this new position
			_ptSecond.x = gi.ptsLocation.x;
			_ptSecond.y = gi.ptsLocation.y;
			ScreenToClient(hWnd, &_ptSecond);

			if (!(gi.dwFlags & (GF_END/*|GF_INERTIA*/)))
			{
				// We apply move operation of the object
				if (ProcessMove(hWnd, _ptSecond.x-_ptFirst.x, _ptSecond.y-_ptFirst.y))
				{
					// We have to copy second point into first one to prepare
					// for the next step of this gesture.
					_ptFirst = _ptSecond;
				}
			}

		}
		break;

	case GID_ROTATE:
		DUMPGEST(L"GID_ROTATE");
		if (gi.dwFlags & GF_BEGIN)
		{
			_inRotate = false;
			_dwArguments = LODWORD(gi.ullArguments); // Запомним начальный угол
		}
		else
		{
			_ptFirst.x = gi.ptsLocation.x;
			_ptFirst.y = gi.ptsLocation.y;
			ScreenToClient(hWnd, &_ptFirst);
			// Пока угол не станет достаточным для смены таба - игнорируем
			if (ProcessRotate(hWnd,
					LODWORD(gi.ullArguments) - _dwArguments,
					_ptFirst.x,_ptFirst.y, ((gi.dwFlags & GF_END) == GF_END)))
			{
				_dwArguments = LODWORD(gi.ullArguments);
			}
		}
		break;

	case GID_TWOFINGERTAP:
		DUMPGEST(L"GID_TWOFINGERTAP");
		_ptFirst.x = gi.ptsLocation.x;
		_ptFirst.y = gi.ptsLocation.y;
		ScreenToClient(hWnd,&_ptFirst);
		ProcessTwoFingerTap(hWnd, _ptFirst.x, _ptFirst.y, LODWORD(gi.ullArguments));
		break;

	case GID_PRESSANDTAP:
		DUMPGEST(L"GID_PRESSANDTAP");
		if (gi.dwFlags & GF_BEGIN)
		{
			_ptFirst.x = gi.ptsLocation.x;
			_ptFirst.y = gi.ptsLocation.y;
			ScreenToClient(hWnd,&_ptFirst);
			DWORD nDelta = LODWORD(gi.ullArguments);
			short nDeltaX = (short)LOWORD(nDelta);
			short nDeltaY = (short)HIWORD(nDelta);
			ProcessPressAndTap(hWnd, _ptFirst.x, _ptFirst.y, nDeltaX, nDeltaY);
		}
		break;
	default:
		DUMPGEST(L"GID_<UNKNOWN>");
	}

	_CloseGestureInfoHandle((HGESTUREINFO)lParam);

	return TRUE;
}

void CGestures::SendRClick(HWND hWnd, const LONG ldx, const LONG ldy)
{
	CVConGuard VCon;
	CRealConsole* pRCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : NULL;
	if (pRCon)
	{
		POINT pt = {ldx, ldy};
		if (hWnd != VCon->GetView())
			MapWindowPoints(hWnd, VCon->GetView(), &pt, 1);
		pRCon->OnMouse(WM_RBUTTONDOWN, MK_RBUTTON, pt.x, pt.y);
		pRCon->OnMouse(WM_RBUTTONUP, 0, pt.x, pt.y);
	}
}

// This function is called when press and tap gesture is recognized
void CGestures::ProcessPressAndTap(HWND hWnd, const LONG ldx, const LONG ldy, const short nDeltaX, const short nDeltaY)
{
	SendRClick(hWnd, ldx, ldy);
	return;
}

// This function is invoked when two finger tap gesture is recognized
// ldx/ldy - Indicates the center of the two fingers.
void CGestures::ProcessTwoFingerTap(HWND hWnd, const LONG ldx, const LONG ldy, const ULONG dist)
{
	SendRClick(hWnd, ldx, ldy);
	return;
}

// This function is called constantly through duration of zoom in/out gesture
void CGestures::ProcessZoom(HWND hWnd, const double dZoomFactor, const LONG lZx, const LONG lZy)
{
	if (dZoomFactor > 0)
	{
		if (dZoomFactor > 1.01)
		{
			int nDelta = (1 * dZoomFactor);
			if (nDelta < 1)
				nDelta = 1;
			else if (nDelta > 8)
				nDelta = 8;

			gpConEmu->PostFontSetSize(1, nDelta);
		}
		else if (dZoomFactor < 0.99)
		{
			int nDelta = (1.0 / dZoomFactor);
			if (nDelta < 1)
				nDelta = 1;
			else if (nDelta > 8)
				nDelta = 8;

			gpConEmu->PostFontSetSize(1, -nDelta);
		}
	}
	return;
}

// This function is called throughout the duration of the panning/inertia gesture
bool CGestures::ProcessMove(HWND hWnd, const LONG ldx, const LONG ldy)
{
	bool lbSent = false;

	if (ldy)
	{
		CVConGuard VCon;
		CRealConsole* pRCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : NULL;
		if (pRCon)
		{
			TODO("Если можно будет задавать разный шрифт для разных консолей - заменить gpSet->FontHeight()");
			// Соотнести Pan с высотой шрифта
			int dy = ((ldy < 0) ? -ldy : ldy) / gpFontMgr->FontHeight();
			if (dy > 0)
			{
				short Delta = ((ldy < 0) ? -120 : 120) * dy;

				#ifdef _DEBUG
				wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"  ProcessMove(%i,%i), WheelDelta=%i\n", ldx, ldy, (int)Delta);
				DEBUGSTRPAN(szDbg);
				#endif

				POINT pt = _ptBegin;
				if (hWnd != VCon->GetView())
					MapWindowPoints(hWnd, VCon->GetView(), &pt, 1);

				pRCon->OnScroll(WM_MOUSEWHEEL, MAKELPARAM(0,Delta), pt.x, pt.y, true);

				lbSent = true; // Запомнить обработанную координату
			}
			TODO("Horizontal Scroll!");
		}
	}

	return lbSent;
}

// This function is called throughout the duration of the rotation gesture
bool CGestures::ProcessRotate(HWND hWnd, const LONG lAngle, const LONG lOx, const LONG lOy, bool bEnd)
{
	if (!gpConEmu->mp_TabBar)
	{
		_ASSERTE(gpConEmu->mp_TabBar!=NULL);
		return false;
	}

	bool bProcessed = false;

	if ((((lAngle<0)?-lAngle:lAngle) / 2048) >= 1)
	{
		if (!_inRotate)
		{
			//starting
			_inRotate = true;
		}

		if (lAngle > 0)
			gpConEmu->mp_TabBar->SwitchPrev();
		else
			gpConEmu->mp_TabBar->SwitchNext();

		bProcessed = true;
	}

	if (bEnd /*&& _inRotate*/)
	{
		_inRotate = false;
		gpConEmu->mp_TabBar->SwitchCommit();
	}

	return bProcessed;
}
