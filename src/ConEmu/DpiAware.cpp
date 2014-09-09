
/*
Copyright (c) 2014 Maximus5
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

#include "Header.h"
#include "DpiAware.h"
#include "SearchCtrl.h"
#include "../common/Monitors.h"
#include "../common/StartupEnvDef.h"

//#define DPI_144

/*
struct DpiValue
	int Ydpi;
	int Xdpi;
*/

DpiValue::DpiValue()
{
	#if defined(_DEBUG) && defined(DPI_144)
	SetDpi(144,144);
	#else
	SetDpi(96,96);
	#endif
}

DpiValue::DpiValue(WPARAM wParam)
{
	#if defined(_DEBUG) && defined(DPI_144)
	SetDpi(144,144);
	#else
	SetDpi(96,96);
	#endif
	OnDpiChanged(wParam);
}

DpiValue::DpiValue(const DpiValue& dpi)
{
	Xdpi = dpi.Xdpi; Ydpi = dpi.Ydpi;
}

DpiValue::operator int() const
{
	return Ydpi;
}

struct DpiValue& DpiValue::operator=(WORD dpi)
{
	Xdpi = dpi;
	Ydpi = dpi;
	return *this;
}

bool DpiValue::Equals(const DpiValue& dpi) const
{
	return ((Xdpi == dpi.Xdpi) && (Ydpi == dpi.Ydpi));
}

void DpiValue::SetDpi(int xdpi, int ydpi)
{
	Xdpi = xdpi; Ydpi = ydpi;
}

void DpiValue::SetDpi(const DpiValue& dpi)
{
	SetDpi(dpi.Xdpi, dpi.Ydpi);
}

void DpiValue::OnDpiChanged(WPARAM wParam)
{
	// That comes from WM_DPICHANGED notification message
	if ((int)LOWORD(wParam) > 0 && (int)HIWORD(wParam) > 0)
	{
		SetDpi((int)LOWORD(wParam), (int)HIWORD(wParam));
		_ASSERTE(Ydpi >= 96 && Xdpi >= 96);
	}
}


/*
class CDpiAware - static helper methods
*/
HRESULT CDpiAware::setProcessDPIAwareness()
{
	HRESULT hr = E_FAIL;
	HMODULE hUser;
	// Actual export name - "SetProcessDpiAwarenessInternal"
	typedef HRESULT (WINAPI* SetProcessDPIAwareness_t)(ProcessDpiAwareness value);
	SetProcessDPIAwareness_t fWin81;
	// "SetProcessDPIAware
	typedef BOOL (WINAPI* SetProcessDPIAware_t)(void);
	SetProcessDPIAware_t fVista;


	if ((hUser = GetModuleHandle(L"user32.dll")) == NULL)
	{
		goto wrap;
	}

	if ((fWin81 = (SetProcessDPIAwareness_t)GetProcAddress(hUser, "SetProcessDpiAwarenessInternal")) != NULL)
	{
		hr = fWin81(Process_Per_Monitor_DPI_Aware);
		goto wrap;
	}

	if ((fVista = (SetProcessDPIAware_t)GetProcAddress(hUser, "SetProcessDPIAware")) != NULL)
	{
		hr = fVista() ? S_OK : E_ACCESSDENIED;
		goto wrap;
	}

wrap:
	return hr;
}

void CDpiAware::UpdateStartupInfo(CEStartupEnv* pStartEnv)
{
	if (!pStartEnv)
		return;

	pStartEnv->bIsPerMonitorDpi = IsPerMonitorDpi();
	for (INT_PTR i = ((int)pStartEnv->nMonitorsCount)-1; i >= 0; i--)
	{
		HMONITOR hMon = MonitorFromRect(&pStartEnv->Monitors[i].rcMonitor, MONITOR_DEFAULTTONEAREST);
		if (!hMon)
			continue;

		for (int j = MDT_Effective_DPI; j <= MDT_Raw_DPI; j++)
		{
			DpiValue dpi;
			QueryDpiForMonitor(hMon, &dpi, (MonitorDpiType)j);
			pStartEnv->Monitors[i].dpis[j+1].x = dpi.Xdpi;
			pStartEnv->Monitors[i].dpis[j+1].y = dpi.Xdpi;
		}
	}
}

bool CDpiAware::IsPerMonitorDpi()
{
	#if defined(_DEBUG) && defined(DPI_144)
	return true;
	#endif

	// Checkbox 'Let me choose one scaling level for all my displays'
	// on the 'Control Panel\ Appearance and Personalization\Display user interface (UI)'
	// Changes will be applied on the next logon only

	static int iPerMonitor = 0;

	if (iPerMonitor == 0)
	{
		HKEY hk;
		if (IsWindows8_1 && (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hk)))
		{
			DWORD nValue = 0, nSize = sizeof(nValue);
			if (0 == RegQueryValueEx(hk, L"Win8DpiScaling", NULL, NULL, (LPBYTE)&nValue, &nSize) && nSize == sizeof(nValue))
			{
				iPerMonitor = (nValue == 0) ? 1 : -1;
			}
			else
			{
				iPerMonitor = -1;
			}
			RegCloseKey(hk);
		}
		else
		{
			iPerMonitor = -1;
		}
	}

	return (iPerMonitor == 1);
}

int CDpiAware::QueryDpi(HWND hWnd /*= NULL*/, DpiValue* pDpi /*= NULL*/)
{
	#if defined(_DEBUG) && defined(DPI_144)
	if (pDpi) pDpi->SetDpi(144,144);
	return 144;
	#endif

	if (hWnd && IsPerMonitorDpi())
	{
		HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		if (hMon)
		{
			return QueryDpiForMonitor(hMon, pDpi);
		}
	}

	return QueryDpiForWindow(hWnd, pDpi);
}

// if hWnd is NULL - returns DC's dpi
int CDpiAware::QueryDpiForWindow(HWND hWnd /*= NULL*/, DpiValue* pDpi /*= NULL*/)
{
	#if defined(_DEBUG) && defined(DPI_144)
	if (pDpi) pDpi->SetDpi(144,144);
	return 144;
	#endif

	int dpi = 96;
	HDC desktopDc = GetDC(hWnd);
	if (desktopDc != NULL)
	{
		// Get native resolution
		int x = GetDeviceCaps(desktopDc, LOGPIXELSX);
		int y = GetDeviceCaps(desktopDc, LOGPIXELSY);
		ReleaseDC(NULL, desktopDc);
		if (x >= 96 && y >= 96)
		{
			if (pDpi)
			{
				pDpi->Xdpi = x; pDpi->Ydpi = y;
			}
			dpi = y;
		}
	}
	return dpi;
}

int CDpiAware::QueryDpiForMonitor(HMONITOR hmon, DpiValue* pDpi /*= NULL*/, MonitorDpiType dpiType /*= MDT_Default*/)
{
	#if defined(_DEBUG) && defined(DPI_144)
	if (pDpi) pDpi->SetDpi(144,144);
	return 144;
	#endif

	int dpiX = 96, dpiY = 96;

	static HMODULE Shcore = NULL;
	typedef HRESULT (WINAPI* GetDPIForMonitor_t)(HMONITOR hmonitor, MonitorDpiType dpiType, UINT *dpiX, UINT *dpiY);
	static GetDPIForMonitor_t getDPIForMonitor = NULL;

	if (Shcore == NULL)
	{
		Shcore = LoadLibrary(L"Shcore.dll");
		getDPIForMonitor = Shcore ? (GetDPIForMonitor_t)GetProcAddress(Shcore, "GetDpiForMonitor") : NULL;

		if ((Shcore == NULL) || (getDPIForMonitor == NULL))
		{
			Shcore = (HMODULE)INVALID_HANDLE_VALUE;
		}
	}

	UINT x = 0, y = 0;
	HRESULT hr = E_FAIL;
	bool bSet = false;
	if (hmon && (Shcore != (HMODULE)INVALID_HANDLE_VALUE))
	{
		hr = getDPIForMonitor(hmon, dpiType/*MDT_Effective_DPI*/, &x, &y);
		if (SUCCEEDED(hr) && (x > 0) && (y > 0))
		{
			if (pDpi)
			{
				pDpi->SetDpi((int)x, (int)y);
				bSet = true;
			}
			dpiX = (int)x;
			dpiY = (int)y;
		}
	}
	else
	{
		static int overallX = 0, overallY = 0;
		if (overallX <= 0 || overallY <= 0)
		{
			HDC hdc = GetDC(NULL);
			if (hdc)
			{
				overallX = GetDeviceCaps(hdc, LOGPIXELSX);
				overallY = GetDeviceCaps(hdc, LOGPIXELSY);
				ReleaseDC(NULL, hdc);
			}
		}
		if (overallX > 0 && overallY > 0)
		{
			dpiX = overallX; dpiY = overallY;
		}
	}

	if (pDpi && !bSet)
	{
		pDpi->SetDpi(dpiX, dpiY);
	}

	return dpiY;
}

void CDpiAware::GetCenteredRect(HWND hWnd, RECT& rcCentered)
{
	bool lbCentered = false;
	HMONITOR hMon;

	if (hWnd)
		hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	else
		hMon = MonitorFromRect(&rcCentered, MONITOR_DEFAULTTONEAREST);

	MONITORINFO mi = {};
	GetNearestMonitorInfo(&mi, NULL, hWnd ? NULL : &rcCentered, hWnd);

	int iWidth  = rcCentered.right - rcCentered.left;
	int iHeight = rcCentered.bottom - rcCentered.top;

	RECT rcNew = {
			(mi.rcWork.left + mi.rcWork.right - iWidth)/2,
			(mi.rcWork.top + mi.rcWork.bottom - iHeight)/2
	};
	rcNew.right = rcNew.left + iWidth;
	rcNew.bottom = rcNew.top + iHeight;

	rcCentered = rcNew;
}


/*
class CDpiForDialog - handle per-monitor dpi for our resource-based dialogs
	HWND mh_Dlg;

	DpiValue m_InitDpi;
	int mn_InitFontHeight;
	LOGFONT mlf_InitFont;
	DpiValue m_CurDpi;
	int mn_CurFontHeight;
	LOGFONT mlf_CurFont;

	HFONT mh_OldFont, mh_CurFont;

	struct DlgItem
	{
		HWND h;
		RECT r;
	};
	MMap<int, MArray<DlgItem>*> m_Items;
*/

CDpiForDialog::CDpiForDialog()
{
	mh_Dlg = NULL;
	mn_InitFontHeight = 8;
	ZeroStruct(mlf_InitFont);
	mn_CurFontHeight = 0;
	ZeroStruct(mlf_CurFont);
	mh_OldFont = mh_CurFont = NULL;
	ZeroStruct(m_Items);
	mn_InSet = 0;
}

CDpiForDialog::~CDpiForDialog()
{
	_ASSERTE(mn_InSet == 0);
	Detach();
}

bool CDpiForDialog::Attach(HWND hWnd, HWND hCenterParent, DpiValue* pCurDpi /*= NULL*/)
{
	mh_Dlg = hWnd;

	wchar_t szLog[100];

	mh_OldFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	if ((mh_OldFont != NULL)
		&& (GetObject(mh_OldFont, sizeof(mlf_InitFont), &mlf_InitFont) > 0))
	{
		mn_InitFontHeight = mlf_InitFont.lfHeight;
		_wsprintf(szLog, SKIPLEN(countof(szLog)) L"CDpiForDialog(x%08X) Font='%s' lfHeight=%i", (DWORD)(DWORD_PTR)hWnd, mlf_InitFont.lfFaceName, mlf_InitFont.lfHeight);
	}
	else
	{
		mn_InitFontHeight = 8;
		ZeroStruct(mlf_InitFont);
		mlf_InitFont.lfHeight = mn_InitFontHeight;
		lstrcpyn(mlf_InitFont.lfFaceName, L"MS Shell Dlg", countof(mlf_InitFont.lfFaceName));
		mlf_InitFont.lfWeight = 400;
		mlf_InitFont.lfCharSet = DEFAULT_CHARSET;
		_wsprintf(szLog, SKIPLEN(countof(szLog)) L"CDpiForDialog(x%08X) DefaultFont='%s' lfHeight=%i", (DWORD)(DWORD_PTR)hWnd, mlf_InitFont.lfFaceName, mlf_InitFont.lfHeight);
	}

	LogString(szLog);

	// Up to Windows 8 - OS will care of dialog scaling
	// And what will happens in Windows 8.1?
	// If `Per-monitor` dpi was choosed in the OS settings,
	// we need to re-scale our dialog manually!
	// But if one dpi was choosed for all monitors?

	CDpiAware::QueryDpi(hCenterParent ? hCenterParent : hWnd, &m_InitDpi);

	if (!m_Items.Initialized())
		m_Items.Init(8);

	if (CDpiAware::IsPerMonitorDpi())
	{
		// When Windows 8.1 is in per-monitor mode
		// and application is marked as per-monitor-dpi aware
		// Windows does not resize dialogs automatically.
		// Our resources are designed for standard 96 dpi.
		m_CurDpi.SetDpi(96, 96);
		MArray<DlgItem>* p = NULL;
		if (m_Items.Get(m_CurDpi.Ydpi, &p) && p)
			delete p;
		p = LoadDialogItems(hWnd);
		m_Items.Set(m_CurDpi.Ydpi, p);

		// Need to resize the dialog?
		if (m_InitDpi.Ydpi != m_CurDpi.Ydpi)
		{
			if (!SetDialogDPI(m_InitDpi))
				return false;

			if (pCurDpi)
				pCurDpi->SetDpi(m_InitDpi);
		}
	}
	else
	{
		m_CurDpi.SetDpi(m_InitDpi.Xdpi, m_InitDpi.Ydpi);
	}

	return true;
}

bool CDpiForDialog::SetDialogDPI(const DpiValue& newDpi, LPRECT lprcSuggested /*= NULL*/)
{
	if (mn_InSet > 0)
		return false;

	if (newDpi.Ydpi <= 0 || newDpi.Xdpi <= 0)
		return false;
	if (m_CurDpi.Ydpi <= 0 || m_CurDpi.Xdpi <= 0)
		return false;
	if (m_CurDpi.Equals(newDpi))
		return false;

	bool bRc = false;
	MArray<DlgItem>* p = NULL;
	DpiValue curDpi(m_CurDpi);
	HFONT hf = NULL;

	wchar_t szClass[100];

	#ifdef _DEBUG
	LOGFONT lftest1 = {}, lftest2 = {};
	HFONT hftest;
	int itest1, itest2;
	#endif

	// To avoid several nested passes
	InterlockedIncrement(&mn_InSet);
	m_CurDpi.SetDpi(newDpi);

	_wsprintf(szClass, SKIPLEN(countof(szClass)) L"CDpiForDialog::SetDialogDPI(x%08X, {%i,%i})", (DWORD)(DWORD_PTR)mh_Dlg, newDpi.Xdpi, newDpi.Ydpi);
	LogString(szClass);

	// Eval
	mn_CurFontHeight = (mn_InitFontHeight * newDpi.Ydpi / 96);
	mlf_CurFont = mlf_InitFont;
	mlf_CurFont.lfHeight = mn_CurFontHeight;
	mlf_CurFont.lfWidth = 0; // Font mapper fault

	if (mn_CurFontHeight == 0 || mn_InitFontHeight == 0)
		goto wrap;


	if (!m_Items.Get(newDpi.Ydpi, &p))
	{
		MArray<DlgItem>* pOrig = NULL;
		int iOrigDpi = 0;
		if (!m_Items.GetNext(NULL, &iOrigDpi, &pOrig) || !pOrig || (iOrigDpi <= 0))
			goto wrap;
		int iNewDpi = newDpi.Ydpi;

		p = new MArray<DlgItem>();

		DWORD dwStyle = GetWindowLong(mh_Dlg, GWL_STYLE);
		DWORD dwStyleEx = GetWindowLong(mh_Dlg, GWL_EXSTYLE);

		RECT rcClient = {}, rcCurWnd = {};
		if (!GetClientRect(mh_Dlg, &rcClient) || !GetWindowRect(mh_Dlg, &rcCurWnd))
		{
			delete p;
			goto wrap;
		}

		_ASSERTE(rcClient.left==0 && rcClient.top==0);
		int calcDlgWidth = rcClient.right * newDpi.Xdpi / curDpi.Xdpi;
		int calcDlgHeight = rcClient.bottom * newDpi.Ydpi / curDpi.Ydpi;

		DlgItem i = {mh_Dlg};

		// Windows DWM manager do not resize NonClient areas of per-monitor dpi aware applications
		// So, we can not use AdjustWindowRectEx to determine full window rectangle
		// Just use current NonClient dimensions
		i.r.right = calcDlgWidth + ((rcCurWnd.right - rcCurWnd.left) - rcClient.right);
		i.r.bottom = calcDlgHeight + ((rcCurWnd.bottom - rcCurWnd.top) - rcClient.bottom);

		// .right and .bottom are width and height of the dialog
		_ASSERTE(i.r.left==0 && i.r.top==0);
		p->push_back(i);

		for (INT_PTR k = 1; k < pOrig->size(); k++)
		{
			const DlgItem& iOrig = (*pOrig)[k];
			i.h = iOrig.h;
			i.r.left = iOrig.r.left * iNewDpi / iOrigDpi;
			i.r.top = iOrig.r.top * iNewDpi / iOrigDpi;
			i.r.right = iOrig.r.right * iNewDpi / iOrigDpi;
			i.r.bottom = iOrig.r.bottom * iNewDpi / iOrigDpi;
			p->push_back(i);
		}

		m_Items.Set(iNewDpi, p);
	}

	hf = CreateFontIndirect(&mlf_CurFont);
	if (hf == NULL)
		goto wrap;

	for (INT_PTR k = p->size() - 1; k >= 0; k--)
	{
		const DlgItem& di = (*p)[k];
		DEBUGTEST(GetClassName(di.h, szClass, countof(szClass)));

		int newW = di.r.right - di.r.left;
		int newH = di.r.bottom - di.r.top;
		MoveWindow(di.h, di.r.left, di.r.top, newW, newH, FALSE);
		SendMessage(di.h, WM_SETFONT, (WPARAM)hf, FALSE/*immediately*/);
		EditIconHint_ResChanged(di.h);
		InvalidateRect(di.h, NULL, TRUE);
		#ifdef _DEBUG
		itest1 = GetObject(hf, sizeof(lftest1), &lftest1);
		hftest = (HFONT)SendMessage(di.h, WM_GETFONT, 0, 0);
		itest2 = GetObject(hftest, sizeof(lftest2), &lftest2);
		#endif
	}

	if (p->size() > 0)
	{
		const DlgItem& di = (*p)[0];
		SendMessage(mh_Dlg, WM_SETFONT, (WPARAM)hf, FALSE);
		DWORD nWndFlags = SWP_NOZORDER | (lprcSuggested ? 0 : SWP_NOMOVE);
		SetWindowPos(mh_Dlg, NULL,
			lprcSuggested ? lprcSuggested->left : 0, lprcSuggested ? lprcSuggested->top : 0,
			di.r.right, di.r.bottom,
			nWndFlags);
		RECT rc = {}; GetClientRect(mh_Dlg, &rc);
		InvalidateRect(mh_Dlg, NULL, TRUE);
		RedrawWindow(mh_Dlg, &rc, NULL, /*RDW_ERASE|*/RDW_ALLCHILDREN/*|RDW_INVALIDATE|RDW_UPDATENOW|RDW_INTERNALPAINT*/);
	}

	if (mh_CurFont != hf)
		DeleteObject(mh_CurFont);
	mh_CurFont = hf;

	bRc = true;
wrap:
	InterlockedDecrement(&mn_InSet);
	return bRc;
}

void CDpiForDialog::Detach()
{
	if (m_Items.Initialized())
	{
		MArray<DlgItem>* p = NULL;
		int iDpi = 0;
		while (m_Items.GetNext(iDpi ? &iDpi : NULL, &iDpi, &p))
		{
			if (p)
				delete p;
		}
		m_Items.Release();
	}
}

bool CDpiForDialog::ProcessDpiMessages(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	switch (nMsg)
	{
	case WM_DPICHANGED:
		{
			DpiValue dpi(wParam);
			LPRECT lprcSuggested = (LPRECT)lParam;
			SetDialogDPI(dpi, lprcSuggested);
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
		}
		return true;
	}
	return false;
}

MArray<CDpiForDialog::DlgItem>* CDpiForDialog::LoadDialogItems(HWND hDlg)
{
	MArray<DlgItem>* p = new MArray<DlgItem>();
	DlgItem i = {hDlg};
	if (!GetWindowRect(hDlg, &i.r))
	{
		delete p;
		return NULL;
	}
	OffsetRect(&i.r, -i.r.left, -i.r.top);
	p->push_back(i);

	i.h = NULL;
	while ((i.h = FindWindowEx(hDlg, i.h, NULL, NULL)) != NULL)
	{
		if (GetWindowRect(i.h, &i.r) && MapWindowPoints(NULL, hDlg, (LPPOINT)&i.r, 2))
		{
			p->push_back(i);
		}
	}

	return p;
}
