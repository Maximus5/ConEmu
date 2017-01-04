
/*
Copyright (c) 2013-2017 Maximus5
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
#include <windows.h>
#include "Common.h"
#include "MAssert.h"
#include "MArray.h"
#include "Monitors.h"

// If possible, open our windows on the monitor where user
// have clicked our icon (shortcut on the desktop or TaskBar)
HMONITOR GetStartupMonitor()
{
	STARTUPINFO si = {sizeof(si)};
	MONITORINFO mi = {sizeof(mi)};
	POINT ptCur = {}, ptOut = {-32000,-32000};
	HMONITOR hStartupMonitor = NULL, hMouseMonitor, hPrimaryMonitor;

	GetStartupInfo(&si);
	GetCursorPos(&ptCur);

	// Get primary monitor, it's expected to be started at 0x0, but we can't be sure
	hPrimaryMonitor = MonitorFromPoint(ptOut, MONITOR_DEFAULTTOPRIMARY);

	// Get the monitor where mouse cursor is located
	hMouseMonitor = MonitorFromPoint(ptCur, MONITOR_DEFAULTTONEAREST);

	// si.hStdOutput may have a handle of monitor with shortcut or used taskbar
	if (si.hStdOutput && GetMonitorInfo((HMONITOR)si.hStdOutput, &mi))
	{
		hStartupMonitor = (HMONITOR)si.hStdOutput;
	}

	// Now, due to MS Windows bugs or just an inconsistence,
	// hStartupMonitor has hPrimaryMonitor, if program was started
	// from shortcut, even if it is located on secondary monitor,
	// if it was started by keyboard.
	// So we can trust hStartupMonitor value only when it contains
	// non-primary monitor value (when user clicks shortcut with mouse)
	if (hStartupMonitor && ((hStartupMonitor != hPrimaryMonitor) || (hStartupMonitor == hMouseMonitor)))
		return hStartupMonitor;

	// gh#412: Don't force our window to primary monitor at all,
	//         otherwise we fail to restore ConEmu on secondary
	#if 0
	// Otherwise - return monitor where mouse cursor is located
	return hMouseMonitor ? hMouseMonitor : hPrimaryMonitor;
	#endif
	return NULL;
}

// Startup monitor ay be specified from command line
// Example: ConEmu.exe -Monitor <1 | x10001 | "\\.\DISPLAY1">
HMONITOR MonitorFromParam(LPCWSTR asMonitor)
{
	if (!asMonitor || !*asMonitor)
		return NULL;

	wchar_t* pszEnd;
	HMONITOR hMon = NULL;
	MONITORINFO mi = {sizeof(mi)};

	if (asMonitor[0] == L'x' || asMonitor[0] == L'X')
		hMon = (HMONITOR)(DWORD_PTR)wcstoul(asMonitor+1, &pszEnd, 16);
	else if (asMonitor[0] == L'0' && (asMonitor[1] == L'x' || asMonitor[1] == L'X'))
		hMon = (HMONITOR)(DWORD_PTR)wcstoul(asMonitor+2, &pszEnd, 16);
	// HMONITOR was already specified?
	if (GetMonitorInfo(hMon, &mi))
		return hMon;

	// Monitor name or index?
	struct impl {
		HMONITOR hMon;
		LPCWSTR pszName;
		LONG nIndex;

		static BOOL CALLBACK FindMonitor(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
		{
			impl* p = (impl*)dwData;
			MONITORINFOEX mi; ZeroStruct(mi); mi.cbSize = sizeof(mi);
			if (GetMonitorInfo(hMonitor, &mi))
			{
				// Get monitor by name?
				if (p->pszName && (0 == lstrcmpi(p->pszName, mi.szDevice)))
				{
					p->hMon = hMonitor;
					return FALSE; // Stop
				}
				// Get monitor by index
				if ((--p->nIndex) <= 0)
				{
					p->hMon = hMonitor;
					return FALSE; // Stop
				}
			}
			return TRUE; // Continue enum
		};
	} Impl = {};

	if (isDigit(asMonitor[0]))
		Impl.nIndex = wcstoul(asMonitor, &pszEnd, 10);
	else
		Impl.pszName = asMonitor;
	EnumDisplayMonitors(NULL, NULL, impl::FindMonitor, (LPARAM)&Impl);

	// Return what found or not
	return Impl.hMon;
}

BOOL CALLBACK FindMonitorsWorkspace(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	LPRECT lprc = (LPRECT)dwData;

	if (lprcMonitor->left < lprc->left)
		lprc->left = lprcMonitor->left;

	if (lprcMonitor->top < lprc->top)
		lprc->top = lprcMonitor->top;

	if (lprcMonitor->right > lprc->right)
		lprc->right = lprcMonitor->right;

	if (lprcMonitor->bottom > lprc->bottom)
		lprc->bottom = lprcMonitor->bottom;

	return TRUE;
}


RECT GetAllMonitorsWorkspace()
{
	RECT rcAllMonRect = {};

	if (!EnumDisplayMonitors(NULL, NULL, FindMonitorsWorkspace, (LPARAM)&rcAllMonRect) || IsRectEmpty(&rcAllMonRect))
	{
		#ifdef _DEBUG
		DWORD nErr = GetLastError();
        _ASSERTE(FALSE && "EnumDisplayMonitors fails");
		#endif

		rcAllMonRect.left = rcAllMonRect.right = 0;
		rcAllMonRect.right = GetSystemMetrics(SM_CXFULLSCREEN);
		rcAllMonRect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
	}

	return rcAllMonRect;
}

struct _FindPrimaryMonitor
{
	HMONITOR hMon;
	MONITORINFO mi;
};

BOOL CALLBACK FindPrimaryMonitor(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MONITORINFO mi = {sizeof(mi)};

	if (GetMonitorInfo(hMonitor, &mi) && (mi.dwFlags & MONITORINFOF_PRIMARY))
	{
		_FindPrimaryMonitor* pMon = (_FindPrimaryMonitor*)dwData;
		pMon->hMon = hMonitor;
		pMon->mi = mi;
		// And stop enumeration
		return FALSE;
	}

	return TRUE;
}

bool GetMonitorInfoSafe(HMONITOR hMon, MONITORINFO& mi)
{
	if (!hMon)
	{
		return false;
	}

	mi.cbSize = sizeof(mi);
	BOOL bMonitor = GetMonitorInfo(hMon, &mi);
	if (!bMonitor)
	{
		GetPrimaryMonitorInfo(&mi);
	}
	
	return (bMonitor!=FALSE);
}

HMONITOR GetPrimaryMonitorInfo(MONITORINFO* pmi /*= NULL*/)
{
	_FindPrimaryMonitor m = {NULL};

	EnumDisplayMonitors(NULL, NULL, FindPrimaryMonitor, (LPARAM)&m);

	if (!m.hMon)
	{
		_ASSERTE(FALSE && "FindPrimaryMonitor fails");
		// Если облом с мониторами - берем данные по умолчанию
		m.mi.cbSize = 0;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &m.mi.rcWork, 0);
		m.mi.rcMonitor.left = m.mi.rcMonitor.top = 0;
		m.mi.rcMonitor.right = GetSystemMetrics(SM_CXFULLSCREEN);
		m.mi.rcMonitor.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
		m.mi.dwFlags = 0;
	}

	if (pmi) *pmi = m.mi;
	return m.hMon;
}

HMONITOR GetNearestMonitorInfo(MONITORINFO* pmi /*= NULL*/, HMONITOR hDefault /*= NULL*/, LPCRECT prcWnd /*= NULL*/, HWND hWnd /*= NULL*/)
{
	HMONITOR hMon = NULL;
	MONITORINFO mi = {0};

	if (hDefault)
	{
		mi.cbSize = sizeof(mi);
		if (GetMonitorInfo(hDefault, &mi))
		{
			hMon = hDefault;
		}
		else
		{
			_ASSERTE(FALSE && "GetMonitorInfo(hDefault) failed");
			mi.cbSize = 0;
		}
	}

	if (!hMon)
	{
		if (prcWnd)
		{
			hMon = MonitorFromRect(prcWnd, MONITOR_DEFAULTTONEAREST);
		}
		else if (hWnd)
		{
			hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		}

		if (hMon)
		{
			mi.cbSize = sizeof(mi);
			if (!GetMonitorInfo(hMon, &mi))
			{
				_ASSERTE(FALSE && "GetMonitorInfo(hDefault) failed");
				mi.cbSize = 0;
			}
		}
	}
	
	if (!hMon)
	{
		_ASSERTE(FALSE && "Nor RECT neither HWND was succeeded, defaulting to PRIMARY");
		hMon = GetPrimaryMonitorInfo(&mi);
	}

	if (pmi) *pmi = mi;
	return hMon;
}

struct _FindAllMonitorsItem
{
	HMONITOR hMon;
	RECT rcMon;
	POINT ptCenter;
};

struct _FindAllMonitors
{
	MArray<_FindAllMonitorsItem> MonArray;
};

BOOL CALLBACK EnumAllMonitors(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	_FindAllMonitors* p = (_FindAllMonitors*)dwData;
	_FindAllMonitorsItem Info = {hMonitor, *lprcMonitor};
	Info.ptCenter.x = (Info.rcMon.left + Info.rcMon.right) >> 1;
	Info.ptCenter.y = (Info.rcMon.top + Info.rcMon.bottom) >> 1;
	p->MonArray.push_back(Info);
	return TRUE;
}

int MonitorSortCallback(_FindAllMonitorsItem &e1, _FindAllMonitorsItem &e2)
{
	if (e1.ptCenter.x < e2.ptCenter.x)
		return -1;
	else if (e1.ptCenter.x > e2.ptCenter.x)
		return 1;
	else if (e1.ptCenter.y < e2.ptCenter.y)
		return -1;
	else if (e1.ptCenter.y > e2.ptCenter.y)
		return 1;
	return 0;
}

HMONITOR GetNextMonitorInfo(MONITORINFO* pmi, LPCRECT prcWnd, bool Next)
{
	_FindAllMonitors Monitors;
	
	EnumDisplayMonitors(NULL, NULL, EnumAllMonitors, (LPARAM)&Monitors);

	INT_PTR iMonCount = Monitors.MonArray.size();
	if (iMonCount < 2)
		return NULL;

	HMONITOR hFound = NULL;

	_ASSERTE(prcWnd!=NULL); // Иначе будем искать от Primary

	HMONITOR hNearest = prcWnd ? MonitorFromRect(prcWnd, MONITOR_DEFAULTTONEAREST) : GetPrimaryMonitorInfo(NULL);
	#ifdef _DEBUG
	MONITORINFO miNrst = {}; GetMonitorInfoSafe(hNearest, miNrst);
	#endif

	TODO("Доработать упорядочивание мониторов?");
	Monitors.MonArray.sort(MonitorSortCallback);

	for (INT_PTR i = iMonCount; (i--) > 0;)
	{
		_FindAllMonitorsItem& Item = Monitors.MonArray[i];
		if (Item.hMon == hNearest)
		{
			//POINT ptNext = {
			//	Next ? (Item.rcMon.right + 100) : (Item.rcMon.left - 100),
			//	Item.ptCenter.y};
			//hFound = MonitorFromPoint(ptNext, MONITOR_DEFAULTTONEAREST);
			//if (hFound != hNearest)
			//	break; // нашли что-то подходящее


			INT_PTR j;
			if (Next)
			{
				j = ((i + 1) < iMonCount) ? (i + 1) : 0;
			}
			else
			{
				j = (i > 0) ? (i - 1) : (iMonCount - 1);
			}
			hFound = Monitors.MonArray[j].hMon;
			break;
		}
	}

	if (!hFound)
	{
		_ASSERTE((hFound!=NULL) && "Can't find current monitor in monitors array");

		if (Next)
		{
			hFound = Monitors.MonArray[0].hMon;
		}
		else
		{
			hFound = Monitors.MonArray[iMonCount-1].hMon;
		}
	}

	if (hFound != NULL)
	{
		MONITORINFO mi = {sizeof(mi)};
		if (GetMonitorInfo(hFound, &mi))
		{
			if (pmi) *pmi = mi;
		}
		else
		{
			hFound = NULL;
		}
	}

	return hFound;
}

HWND FindTaskbarWindow(LPRECT rcMon /*= NULL*/)
{
	HWND hTaskbar = NULL;
	RECT rcTaskbar, rcMatch;

	while ((hTaskbar = FindWindowEx(NULL, hTaskbar, L"Shell_TrayWnd", NULL)) != NULL)
	{
		if (!rcMon)
		{
			break; // OK, return first found
		}
		if (GetWindowRect(hTaskbar, &rcTaskbar)
			&& IntersectRect(&rcMatch, &rcTaskbar, rcMon))
		{
			break; // OK, taskbar match monitor
		}
	}

	return hTaskbar;
}

bool IsTaskbarAutoHidden(LPRECT rcMon /*= NULL*/, PUINT pEdge /*= NULL*/)
{
	HWND hTaskbar = FindTaskbarWindow(rcMon);
	if (!hTaskbar)
	{
		return false;
	}

	APPBARDATA state = {sizeof(state), hTaskbar};
	APPBARDATA pos = {sizeof(pos), hTaskbar};

	LRESULT lState = SHAppBarMessage(ABM_GETSTATE, &state);
	bool bAutoHidden = (lState & ABS_AUTOHIDE);

	if (SHAppBarMessage(ABM_GETTASKBARPOS, &pos))
	{
		if (pEdge)
			*pEdge = pos.uEdge;
	}
	else
	{
		_ASSERTE(FALSE && "Failed to get taskbar pos");
		if (pEdge)
			*pEdge = ABE_BOTTOM;
	}

	return bAutoHidden;
}
