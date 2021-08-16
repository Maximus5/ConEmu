
/*
Copyright (c) 2013-present Maximus5
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

#include "Common.h"
#include "MAssert.h"
#include "MArray.h"
#include "Monitors.h"

// If possible, open our windows on the monitor where user
// have clicked our icon (shortcut on the desktop or TaskBar)
HMONITOR GetStartupMonitor()
{
	STARTUPINFO si = {};
	si.cb = sizeof(si);
	MONITORINFO mi = {};
	mi.cbSize = sizeof(mi);
	POINT ptCur = {};
	const POINT ptOut = {-32000,-32000};
	HMONITOR hStartupMonitor = nullptr;

	GetStartupInfo(&si);
	GetCursorPos(&ptCur);

	// Get primary monitor, it's expected to be started at 0x0, but we can't be sure
	// ReSharper disable once CppLocalVariableMayBeConst
	HMONITOR hPrimaryMonitor = MonitorFromPoint(ptOut, MONITOR_DEFAULTTOPRIMARY);

	// Get the monitor where mouse cursor is located
	// ReSharper disable once CppLocalVariableMayBeConst
	HMONITOR hMouseMonitor = MonitorFromPoint(ptCur, MONITOR_DEFAULTTONEAREST);

	// si.hStdOutput may have a handle of monitor with shortcut or used taskbar
	if (si.hStdOutput && GetMonitorInfo(static_cast<HMONITOR>(si.hStdOutput), &mi))
	{
		hStartupMonitor = static_cast<HMONITOR>(si.hStdOutput);
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
	return nullptr;
}

// Startup monitor ay be specified from command line
// Example: ConEmu.exe -Monitor <1 | x10001 | "\\.\DISPLAY1">
HMONITOR MonitorFromParam(LPCWSTR asMonitor)
{
	if (!asMonitor || !*asMonitor)
		return nullptr;

	wchar_t* pszEnd;
	HMONITOR hMon = nullptr;
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
	EnumDisplayMonitors(nullptr, nullptr, impl::FindMonitor, reinterpret_cast<LPARAM>(&Impl));

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

	if (!EnumDisplayMonitors(nullptr, nullptr, FindMonitorsWorkspace, reinterpret_cast<LPARAM>(&rcAllMonRect)) || IsRectEmpty(&rcAllMonRect))
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

BOOL CALLBACK FindPrimaryMonitor(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MONITORINFO mi = {sizeof(mi)};

	if (GetMonitorInfo(hMonitor, &mi) && (mi.dwFlags & MONITORINFOF_PRIMARY))
	{
		MonitorInfo* pMon = reinterpret_cast<MonitorInfo*>(dwData);
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

// ReSharper disable once CppParameterMayBeConst
bool GetMonitorInfoSafe(HMONITOR hMon, MonitorInfo& result)
{
	if (!hMon)
	{
		return false;
	}

	result = MonitorInfo{};
	result.hMon = hMon;
	result.mi.cbSize = sizeof(result.mi);
	const bool bMonitor = GetMonitorInfo(hMon, &result.mi);
	if (!bMonitor)
	{
		GetPrimaryMonitorInfo(&result.mi);
	}

	return bMonitor;
}


MonitorInfo GetPrimaryMonitorInfo()
{
	MonitorInfo m = {};

	EnumDisplayMonitors(nullptr, nullptr, FindPrimaryMonitor, reinterpret_cast<LPARAM>(&m));

	if (!m.hMon)
	{
		_ASSERTE(FALSE && "FindPrimaryMonitor fails");
		// If enumeration of monitors fails - use default data
		m.mi.cbSize = 0;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &m.mi.rcWork, 0);
		m.mi.rcMonitor.left = m.mi.rcMonitor.top = 0;
		m.mi.rcMonitor.right = GetSystemMetrics(SM_CXFULLSCREEN);
		m.mi.rcMonitor.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
		m.mi.dwFlags = 0;
	}

	return m;
}

HMONITOR GetPrimaryMonitorInfo(MONITORINFO* pmi)
{
	const MonitorInfo m = GetPrimaryMonitorInfo();
	if (pmi)
		*pmi = m.mi;
	return m.hMon;
}

HMONITOR GetNearestMonitorInfo(MONITORINFO* pmi, HMONITOR hDefault /*= nullptr*/, LPCRECT prcWnd /*= nullptr*/, HWND hWnd /*= nullptr*/)
{
	const MonitorInfo m = GetNearestMonitorInfo(hDefault, prcWnd, hWnd);
	if (pmi)
		*pmi = m.mi;
	return m.hMon;
}

MonitorInfo GetNearestMonitorInfo(HMONITOR hDefault /*= nullptr*/, LPCRECT prcWnd /*= nullptr*/, HWND hWnd /*= nullptr*/)
{
	MonitorInfo result{};

	if (hDefault)
	{
		result.mi.cbSize = sizeof(result.mi);
		if (GetMonitorInfo(hDefault, &result.mi))
		{
			result.hMon = hDefault;
		}
		else
		{
			_ASSERTE(FALSE && "GetMonitorInfo(hDefault) failed");
			result.mi.cbSize = 0;
		}
	}

	if (!result.hMon)
	{
		if (prcWnd)
		{
			result.hMon = MonitorFromRect(prcWnd, MONITOR_DEFAULTTONEAREST);
		}
		else if (hWnd)
		{
			result.hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		}

		if (result.hMon)
		{
			result.mi.cbSize = sizeof(result.mi);
			if (!GetMonitorInfo(result.hMon, &result.mi))
			{
				_ASSERTE(FALSE && "GetMonitorInfo(hDefault) failed");
				result.mi.cbSize = 0;
			}
		}
	}

	if (!result.hMon)
	{
		_ASSERTE(FALSE && "Nor RECT neither HWND was succeeded, defaulting to PRIMARY");
		result.hMon = GetPrimaryMonitorInfo(&result.mi);
	}

	return result;
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
	_FindAllMonitors* p = reinterpret_cast<_FindAllMonitors*>(dwData);
	_FindAllMonitorsItem Info = {hMonitor, *lprcMonitor};
	Info.ptCenter.x = (Info.rcMon.left + Info.rcMon.right) >> 1;
	Info.ptCenter.y = (Info.rcMon.top + Info.rcMon.bottom) >> 1;
	p->MonArray.push_back(Info);
	return TRUE;
}

static bool MonitorSortCallback(const _FindAllMonitorsItem& e1, const _FindAllMonitorsItem& e2)
{
	if (e1.ptCenter.x < e2.ptCenter.x)
		return true;
	else if (e1.ptCenter.x > e2.ptCenter.x)
		return false;
	else if (e1.ptCenter.y < e2.ptCenter.y)
		return true;
	else if (e1.ptCenter.y > e2.ptCenter.y)
		return false;
	return false;
}

HMONITOR GetNextMonitorInfo(MONITORINFO* pmi, LPCRECT prcWnd, bool next)
{
	const MonitorInfo m = GetNextMonitorInfo(prcWnd, next);
	if (pmi)
		*pmi = m.mi;
	return m.hMon;
}

MonitorInfo GetNextMonitorInfo(LPCRECT prcWnd, bool next)
{
	_FindAllMonitors Monitors;

	EnumDisplayMonitors(nullptr, nullptr, EnumAllMonitors, reinterpret_cast<LPARAM>(&Monitors));

	const auto iMonCount = Monitors.MonArray.size();
	if (iMonCount < 2)
		return {};

	HMONITOR hFound = nullptr;

	_ASSERTE(prcWnd!=nullptr); // Иначе будем искать от Primary

	HMONITOR hNearest = prcWnd ? MonitorFromRect(prcWnd, MONITOR_DEFAULTTONEAREST) : GetPrimaryMonitorInfo(nullptr);
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
			if (next)
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
		_ASSERTE((hFound!=nullptr) && "Can't find current monitor in monitors array");

		if (next)
		{
			hFound = Monitors.MonArray[0].hMon;
		}
		else
		{
			hFound = Monitors.MonArray[iMonCount-1].hMon;
		}
	}

	MonitorInfo result{};
	if (hFound != nullptr)
	{
		result.mi.cbSize = sizeof(result.mi);
		if (GetMonitorInfo(hFound, &result.mi))
		{
			result.hMon = hFound;
		}
		else
		{
			result.mi.cbSize = 0;
		}
	}

	return result;
}

HWND FindTaskbarWindow(LPRECT rcMon /*= nullptr*/)
{
	HWND hTaskbar = nullptr;
	RECT rcTaskbar, rcMatch;

	while ((hTaskbar = FindWindowEx(nullptr, hTaskbar, L"Shell_TrayWnd", nullptr)) != nullptr)
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

bool IsTaskbarAutoHidden(LPRECT rcMon /*= nullptr*/, PUINT pEdge /*= nullptr*/, HWND* pTaskbar /*= nullptr*/)
{
	HWND hTaskbar = FindTaskbarWindow(rcMon);
	if (pTaskbar)
		*pTaskbar = hTaskbar;
	if (!hTaskbar)
	{
		if (pEdge)
			*pEdge = (UINT)-1;
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
