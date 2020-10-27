
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

#pragma once

HMONITOR GetStartupMonitor();
HMONITOR MonitorFromParam(LPCWSTR asMonitor);

RECT GetAllMonitorsWorkspace();

struct MonitorInfo
{
	HMONITOR hMon{ nullptr };
	MONITORINFO mi{};
};

bool GetMonitorInfoSafe(HMONITOR hMon, MONITORINFO& mi);
HMONITOR GetPrimaryMonitorInfo(MONITORINFO* pmi);
HMONITOR GetNearestMonitorInfo(MONITORINFO* pmi = nullptr, HMONITOR hDefault = nullptr, LPCRECT prcWnd = nullptr, HWND hWnd = nullptr);
HMONITOR GetNextMonitorInfo(MONITORINFO* pmi, LPCRECT prcWnd, bool next);

bool GetMonitorInfoSafe(HMONITOR hMon, MonitorInfo& result);
MonitorInfo GetPrimaryMonitorInfo();
MonitorInfo GetNearestMonitorInfo(HMONITOR hDefault, LPCRECT prcWnd = nullptr, HWND hWnd = nullptr);
MonitorInfo GetNextMonitorInfo(LPCRECT prcWnd, bool next);

HWND FindTaskbarWindow(LPRECT rcMon = nullptr);
bool IsTaskbarAutoHidden(LPRECT rcMon = nullptr, PUINT pEdge = nullptr, HWND* pTaskbar = nullptr);
