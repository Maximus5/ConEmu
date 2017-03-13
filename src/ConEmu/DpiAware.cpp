
/*
Copyright (c) 2014-2017 Maximus5
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

#include "Header.h"
#include <Commctrl.h>
#include "DpiAware.h"
#include "DynDialog.h"
#include "SearchCtrl.h"
#include "../common/Monitors.h"
#include "../common/StartupEnvDef.h"

#ifdef _DEBUG
static bool gbSkipSetDialogDPI = false;
#endif

//#define DPI_DEBUG_CUSTOM 144 // 96, 120, 144, 192 - these are standard dpi-s
#undef DPI_DEBUG_CUSTOM

/*
struct DpiValue
	int Ydpi;
	int Xdpi;
*/

DpiValue::DpiValue()
{
	#if defined(_DEBUG) && defined(DPI_DEBUG_CUSTOM)
	SetDpi(DPI_DEBUG_CUSTOM,DPI_DEBUG_CUSTOM);
	#else
	SetDpi(96,96);
	#endif
}

DpiValue::DpiValue(WPARAM wParam)
{
	#if defined(_DEBUG) && defined(DPI_DEBUG_CUSTOM)
	SetDpi(DPI_DEBUG_CUSTOM,DPI_DEBUG_CUSTOM);
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
	#if defined(_DEBUG) && defined(DPI_DEBUG_CUSTOM)
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
	#if defined(_DEBUG) && defined(DPI_DEBUG_CUSTOM)
	if (pDpi) pDpi->SetDpi(DPI_DEBUG_CUSTOM,DPI_DEBUG_CUSTOM);
	return DPI_DEBUG_CUSTOM;
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
	#if defined(_DEBUG) && defined(DPI_DEBUG_CUSTOM)
	if (pDpi) pDpi->SetDpi(DPI_DEBUG_CUSTOM,DPI_DEBUG_CUSTOM);
	return DPI_DEBUG_CUSTOM;
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

int CDpiAware::QueryDpiForRect(const RECT& rcWnd, DpiValue* pDpi /*= NULL*/, MonitorDpiType dpiType /*= MDT_Default*/)
{
	HMONITOR hMon = MonitorFromRect(&rcWnd, MONITOR_DEFAULTTONEAREST);
	return QueryDpiForMonitor(hMon, pDpi, dpiType);
}

int CDpiAware::QueryDpiForMonitor(HMONITOR hmon, DpiValue* pDpi /*= NULL*/, MonitorDpiType dpiType /*= MDT_Default*/)
{
	#if defined(_DEBUG) && defined(DPI_DEBUG_CUSTOM)
	if (pDpi) pDpi->SetDpi(DPI_DEBUG_CUSTOM,DPI_DEBUG_CUSTOM);
	return DPI_DEBUG_CUSTOM;
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
			if (Shcore)
				FreeLibrary(Shcore);
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

void CDpiAware::GetCenteredRect(HWND hWnd, RECT& rcCentered, HMONITOR hDefault /*= NULL*/)
{
	bool lbCentered = false;

	#ifdef _DEBUG
	HMONITOR hMon;
	if (hWnd)
		hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	else
		hMon = MonitorFromRect(&rcCentered, MONITOR_DEFAULTTONEAREST);
	#endif

	MONITORINFO mi = {};
	GetNearestMonitorInfo(&mi, hDefault, hWnd ? NULL : &rcCentered, hWnd);

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

void CDpiAware::CenterDialog(HWND hDialog)
{
	RECT rect = {}, rectAfter = {};
	if (!hDialog || !GetWindowRect(hDialog, &rect))
		return;

	// If possible, open our startup dialogs on the monitor,
	// where user have clicked our icon (shortcut on the desktop or TaskBar)
	HMONITOR hDefault = ghWnd ? NULL : gpStartEnv->hStartMon;

	// Position dialog in the workarea center
	CDpiAware::GetCenteredRect(NULL, rect, hDefault);
	MoveWindowRect(hDialog, rect);

	if (IsPerMonitorDpi() && GetWindowRect(hDialog, &rectAfter))
	{
		if ((RectWidth(rect) != RectWidth(rectAfter)) || (RectHeight(rect) != RectHeight(rectAfter)))
		{
			CDpiAware::GetCenteredRect(NULL, rectAfter, NULL);
			MoveWindowRect(hDialog, rectAfter);
		}
	}
}


/*
class CDpiForDialog - handle per-monitor dpi for our resource-based dialogs
	HWND mh_Dlg;

	LONG mn_InSet;

	DpiValue m_InitDpi;
	LOGFONT mlf_InitFont;
	DpiValue m_CurDpi;
	UINT mn_TemplateFontSize;
	int mn_CurFontHeight;
	LOGFONT mlf_CurFont;
	CDynDialog* mp_DlgTemplate;

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
	//mn_InitFontHeight = 8;
	ZeroStruct(mlf_InitFont);
	mn_TemplateFontSize = 8;
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

bool CDpiForDialog::Attach(HWND hWnd, HWND hCenterParent, CDynDialog* apDlgTemplate)
{
	mh_Dlg = hWnd;

	wchar_t szLog[100];

	mn_TemplateFontSize = apDlgTemplate ? apDlgTemplate->GetFontPointSize() : 8;

	mh_OldFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	if ((mh_OldFont != NULL)
		&& (GetObject(mh_OldFont, sizeof(mlf_InitFont), &mlf_InitFont) > 0))
	{
		_wsprintf(szLog, SKIPLEN(countof(szLog)) L"CDpiForDialog(x%08X) Font='%s' lfHeight=%i Points=%u", (DWORD)(DWORD_PTR)hWnd, mlf_InitFont.lfFaceName, mlf_InitFont.lfHeight, mn_TemplateFontSize);
	}
	else
	{
		ZeroStruct(mlf_InitFont);
		mlf_InitFont.lfHeight = GetFontSizeForDpi(NULL, 96);
		lstrcpyn(mlf_InitFont.lfFaceName, L"MS Shell Dlg", countof(mlf_InitFont.lfFaceName));
		mlf_InitFont.lfWeight = 400;
		mlf_InitFont.lfCharSet = DEFAULT_CHARSET;
		_wsprintf(szLog, SKIPLEN(countof(szLog)) L"CDpiForDialog(x%08X) DefaultFont='%s' lfHeight=%i Points=%u", (DWORD)(DWORD_PTR)hWnd, mlf_InitFont.lfFaceName, mlf_InitFont.lfHeight, mn_TemplateFontSize);
	}

	LogString(szLog);

	// Up to Windows 8 - OS will care of dialog scaling
	// And what will happen in Windows 8.1?
	// If `Per-monitor` dpi was choosed in the OS settings,
	// we need to re-scale our dialog manually!
	// But if one dpi was chosen for all monitors?

	CDpiAware::QueryDpiForMonitor(NULL, &m_InitDpi); // Whole desktop DPI (in most cases that will be Primary monitor DPI)
	m_CurDpi.SetDpi(m_InitDpi);

	if (!m_Items.Initialized())
		m_Items.Init(8);

	bool bPerMonitor = CDpiAware::IsPerMonitorDpi();
	DEBUGTEST(bPerMonitor = true);

	if (bPerMonitor)
	{
		// When Windows 8.1 is in per-monitor mode
		// and application is marked as per-monitor-dpi aware
		// Windows does not resize dialogs automatically.
		// Our resources are designed for standard 96 dpi.
		MArray<DlgItem>* p = NULL;
		if (m_Items.Get(m_CurDpi.Ydpi, &p) && p)
			delete p;
		p = LoadDialogItems(hWnd);
		m_Items.Set(m_CurDpi.Ydpi, p);

		DpiValue CurMonDpi;
		CDpiAware::QueryDpi(hCenterParent ? hCenterParent : hWnd, &CurMonDpi);

		// Need to resize the dialog?
		if (m_CurDpi.Ydpi != CurMonDpi.Ydpi)
		{
			if (!SetDialogDPI(CurMonDpi))
				return false;
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
	wchar_t szLog[160];
	RECT rcClient = {}, rcCurWnd = {};

	#ifdef _DEBUG
	if (gbSkipSetDialogDPI)
	{
		GetClientRect(mh_Dlg, &rcClient); GetWindowRect(mh_Dlg, &rcCurWnd);
		_wsprintf(szLog, SKIPCOUNT(szLog) L"SKIPPED CDpiForDialog::SetDialogDPI x%08X, OldDpi={%i,%i}, NewDpi={%i,%i}, CurSize={%i,%i}, CurClient={%i,%i}",
			(DWORD)(DWORD_PTR)mh_Dlg, m_CurDpi.Xdpi, m_CurDpi.Ydpi, newDpi.Xdpi, newDpi.Ydpi,
			(rcCurWnd.right - rcCurWnd.left), (rcCurWnd.bottom - rcCurWnd.top),
			(rcClient.right - rcClient.left), (rcClient.bottom - rcClient.top));
		LogString(szLog);
		return false;
	}
	#endif

	if (mn_InSet > 0)
		return false;

	if (newDpi.Ydpi <= 0 || newDpi.Xdpi <= 0)
		return false;
	if (m_CurDpi.Ydpi <= 0 || m_CurDpi.Xdpi <= 0)
		return false;

	// When overall DPI is very large but new dpi is small
	// (example: primary mon is 192 high-dpi, new mon is 96 dpi)
	// Windows goes crazy... HUGE caption, scrollbars, checkbox marks and so on...
	// So huge difference makes dialog unattractive, let's try to smooth that
	DpiValue setDpi(newDpi);
	if (m_InitDpi.Ydpi > MulDiv(setDpi.Ydpi, 144, 96))
	{
		// Increase DPI one step up
		setDpi.Ydpi = MulDiv(setDpi.Ydpi, 120, 96);
		setDpi.Xdpi = MulDiv(setDpi.Xdpi, 120, 96);
		// Log it
		_wsprintf(szLog, SKIPCOUNT(szLog) L"CDpiForDialog::SetDialogDPI x%08X forces larger dpi value from {%i,%i} to {%i,%i}",
			(DWORD)(DWORD_PTR)mh_Dlg, newDpi.Xdpi, newDpi.Ydpi, setDpi.Xdpi, setDpi.Ydpi);
		LogString(szLog);
	}

	if (m_CurDpi.Equals(setDpi))
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
	m_CurDpi.SetDpi(setDpi);

	// Eval
	mn_CurFontHeight = GetFontSizeForDpi(NULL, m_CurDpi.Ydpi);
	//(m_CurDpi.Ydpi && m_InitDpi.Ydpi) ? (mn_InitFontHeight * m_CurDpi.Ydpi / m_InitDpi.Ydpi) : -11;
	mlf_CurFont = mlf_InitFont;
	mlf_CurFont.lfHeight = mn_CurFontHeight;
	mlf_CurFont.lfWidth = 0; // Font mapper fault

	if (mn_CurFontHeight == 0)
		goto wrap;


	if (!m_Items.Get(m_CurDpi.Ydpi, &p))
	{
		MArray<DlgItem>* pOrig = NULL;
		int iOrigDpi = 0;
		if (!m_Items.GetNext(NULL, &iOrigDpi, &pOrig) || !pOrig || (iOrigDpi <= 0))
			goto wrap;
		int iNewDpi = m_CurDpi.Ydpi;

		p = new MArray<DlgItem>();

		DWORD dwStyle = GetWindowLong(mh_Dlg, GWL_STYLE);
		DWORD dwStyleEx = GetWindowLong(mh_Dlg, GWL_EXSTYLE);

		if (!GetClientRect(mh_Dlg, &rcClient) || !GetWindowRect(mh_Dlg, &rcCurWnd))
		{
			delete p;
			goto wrap;
		}

		_ASSERTE(rcClient.left==0 && rcClient.top==0);
		int calcDlgWidth = rcClient.right * m_CurDpi.Xdpi / curDpi.Xdpi;
		int calcDlgHeight = rcClient.bottom * m_CurDpi.Ydpi / curDpi.Ydpi;

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

	if (p->size() <= 0)
	{
		_ASSERTE(FALSE && "No elements");
		goto wrap;
	}
	else
	{
		const DlgItem& di = (*p)[0];
		_wsprintf(szLog, SKIPCOUNT(szLog) L"CDpiForDialog::SetDialogDPI x%08X, OldDpi={%i,%i}, NewDpi={%i,%i}, OldSize={%i,%i}, NewSize={%i,%i}, NewFont=%i",
			(DWORD)(DWORD_PTR)mh_Dlg, curDpi.Xdpi, curDpi.Ydpi, newDpi.Xdpi, newDpi.Ydpi,
			(rcCurWnd.right - rcCurWnd.left), (rcCurWnd.bottom - rcCurWnd.top), di.r.right, di.r.bottom,
			mlf_CurFont.lfHeight);
		LogString(szLog);
	}

	hf = CreateFontIndirect(&mlf_CurFont);
	if (hf == NULL)
	{
		goto wrap;
	}

	for (INT_PTR k = p->size() - 1; k >= 1; k--)
	{
		const DlgItem& di = (*p)[k];
		GetClassName(di.h, szClass, countof(szClass));
		DWORD nCtrlID = GetWindowLong(di.h, GWL_ID);
		DWORD nStyles = GetWindowLong(di.h, GWL_STYLE);
		bool bResizeCombo = (lstrcmpi(szClass, L"ComboBox") == 0);
		int iComboFieldHeight = 0, iComboWasHeight = 0;
		LONG_PTR lFieldHeight = 0, lNewHeight = 0;
		RECT rcCur = {};
		HWND hComboEdit = NULL;
		RECT rcEdit = {}, rcClient = {};

		if (bResizeCombo && (nStyles & CBS_OWNERDRAWFIXED))
		{
			GetWindowRect(di.h, &rcCur);
			hComboEdit = FindWindowEx(di.h, NULL, L"Edit", NULL);
			GetClientRect(di.h, &rcClient);
			GetClientRect(hComboEdit, &rcEdit);
			iComboWasHeight = (rcCur.bottom - rcCur.top);
			lFieldHeight = SendMessage(di.h, CB_GETITEMHEIGHT, -1, 0);
			if (lFieldHeight < iComboWasHeight)
			{
				iComboFieldHeight = lFieldHeight;
			}
		}

		int newW = di.r.right - di.r.left;
		int newH = di.r.bottom - di.r.top;

		MoveWindow(di.h, di.r.left, di.r.top, newW, newH, FALSE);
		SendMessage(di.h, WM_SETFONT, (WPARAM)hf, FALSE/*immediately*/);
		if (bResizeCombo)
		{
			if ((nStyles & CBS_OWNERDRAWFIXED) && (iComboWasHeight > 0) && (iComboFieldHeight > 0))
			{
				RECT rcEdit2 = {}, rcClient2 = {};
				GetClientRect(di.h, &rcClient2);
				GetClientRect(hComboEdit, &rcEdit2);
				lNewHeight = newH*iComboFieldHeight/iComboWasHeight;
				_wsprintf(szLog, SKIPCOUNT(szLog) L"CDpiForDialog::Combo height changed - OldHeight=%i, ItemHeight=%i, NewHeight=%i, NewItemHeight=%i",
					(rcCur.bottom - rcCur.top), lFieldHeight, newH, lNewHeight);
				LogString(szLog);
				SendMessage(di.h, CB_SETITEMHEIGHT, -1, lNewHeight);
			}
			SendMessage(di.h, CB_SETEDITSEL, 0, MAKELPARAM(-1,0));
		}
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

const DpiValue& CDpiForDialog::GetCurDpi() const
{
	return m_CurDpi;
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
			#ifdef _DEBUG
			DWORD_PTR ID = GetWindowLong(i.h, GWL_ID);
			if (ID == cbExtendFonts)
			{
				RECT rcMargin = {};
				if (Button_GetTextMargin(i.h, &rcMargin))
				{
					wchar_t szLog[100];
					_wsprintf(szLog, SKIPCOUNT(szLog) L"CheckBox Rect={%i,%i}-{%i,%i} Margin={%i,%i}-{%i,%i}",
						LOGRECTCOORDS(i.r), LOGRECTCOORDS(rcMargin));
					LogString(szLog);
				}
			}
			#endif

			p->push_back(i);
		}
	}

	return p;
}

int CDpiForDialog::GetFontSizeForDpi(HDC hdc, int Ydpi)
{
	int iFontHeight = -11;

	if (Ydpi > 0)
	{
		//bool bSelfDC = (hdc == NULL);
		//if (bSelfDC) hdc = GetDC(mh_Dlg);
		UINT nTemplSize = mn_TemplateFontSize ? mn_TemplateFontSize : 8;
		int logY = 99; //GetDeviceCaps(hdc, LOGPIXELSY);
		//if (logY <= 0) logY = 99;
		iFontHeight = -MulDiv(nTemplSize, Ydpi*logY, 72*96);
		//if (bSelfDC && hdc) ReleaseDC(mh_Dlg, hdc);
	}

	return iFontHeight;
}
