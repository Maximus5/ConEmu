
/*
Copyright (c) 2014-present Maximus5
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
#include <commctrl.h>
#include "DpiAware.h"

#include <tuple>

#include "DynDialog.h"
#include "SearchCtrl.h"
#include "../common/Monitors.h"
#include "../common/StartupEnvDef.h"

#ifdef _DEBUG
static bool gbSkipSetDialogDPI = false;
#endif

MModule CDpiAware::shCore_{};
MModule CDpiAware::user32_{};
CDpiAware::GetDpiForMonitor_t CDpiAware::getDpiForMonitor_ = nullptr;


//#define DPI_DEBUG_CUSTOM 144 // 96, 120, 144, 192 - these are standard dpi-s
#undef DPI_DEBUG_CUSTOM

/*
struct DpiValue
	int Ydpi;
	int Xdpi;
*/

DpiValue::~DpiValue()
{
}

DpiValue::DpiValue()
{
	#if defined(_DEBUG) && defined(DPI_DEBUG_CUSTOM)
	SetDpi(DPI_DEBUG_CUSTOM,DPI_DEBUG_CUSTOM);
	#else
	SetDpi(96,96, DpiSource::Default);
	#endif
}

DpiValue::DpiValue(const int xDpi, const int yDpi, const DpiSource source /*= DpiSource::Explicit*/)
{
	SetDpi(xDpi, yDpi, source);
}

DpiValue DpiValue::FromWParam(WPARAM wParam)
{
	DpiValue dpi;
	#if defined(_DEBUG) && defined(DPI_DEBUG_CUSTOM)
	dpi.SetDpi(DPI_DEBUG_CUSTOM, DPI_DEBUG_CUSTOM, DpiSource::Default);
	#else
	dpi.SetDpi(96,96, DpiSource::Default);
	#endif
	dpi.OnDpiChanged(wParam);
	return dpi;
}

bool DpiValue::Equals(const DpiValue& dpi) const
{
	return ((Xdpi == dpi.Xdpi) && (Ydpi == dpi.Ydpi));
}

bool DpiValue::operator==(const DpiValue& dpi) const
{
	return Equals(dpi);
}

bool DpiValue::operator!=(const DpiValue& dpi) const
{
	return !Equals(dpi);
}

int DpiValue::GetDpi() const
{
	return Ydpi;
}

void DpiValue::SetDpi(const int xDpi, const int yDpi, const DpiSource source)
{
	Xdpi = xDpi; Ydpi = yDpi;
	source_ = source;
}

void DpiValue::SetDpi(const DpiValue& dpi)
{
	SetDpi(dpi.Xdpi, dpi.Ydpi, dpi.source_);
}

void DpiValue::OnDpiChanged(const WPARAM wParam)
{
	// That comes from WM_DPICHANGED notification message
	if (static_cast<int>(LOWORD(wParam)) > 0 && static_cast<int>(HIWORD(wParam)) > 0)
	{
		SetDpi(static_cast<int>(LOWORD(wParam)), static_cast<int>(HIWORD(wParam)), DpiSource::WParam);
		_ASSERTE(Ydpi >= 96 && Xdpi >= 96);
	}
	else
	{
		_ASSERTE(static_cast<int>(LOWORD(wParam)) > 0 && static_cast<int>(HIWORD(wParam)) > 0);
	}
}


/*
class CDpiAware - static helper methods
*/
const MModule& CDpiAware::GetShCore()
{
	static bool loaded = false;
	if (!loaded)
	{
		// ReSharper disable once StringLiteralTypo
		shCore_.Load(L"Shcore.dll");
		loaded = true;
	}
	return shCore_;
}

const MModule& CDpiAware::GetUser32()
{
	static bool loaded = false;
	if (!loaded)
	{
		// ReSharper disable once StringLiteralTypo
		user32_.Load(L"user32.dll");
		loaded = true;
	}
	return user32_;
}

HRESULT CDpiAware::SetProcessDpiAwareness()
{
	HRESULT hr = E_FAIL;
	// Actual export name - "SetProcessDpiAwareness"
	typedef HRESULT (WINAPI* SetProcessDpiAwareness_t)(ProcessDpiAwareness value);
	SetProcessDpiAwareness_t fWin81 = nullptr;
	// "SetProcessDPIAware
	typedef BOOL (WINAPI* SetProcessDpiAware_t)();
	SetProcessDpiAware_t fVista = nullptr;

	if (IsWin8_1() && GetShCore().GetProcAddress("SetProcessDpiAwareness", fWin81))
	{
		// E_ACCESSDENIED could be thrown because we already provide the manifest
		hr = fWin81(Process_Per_Monitor_DPI_Aware);
	}
	else if (IsWin6() && GetUser32().GetProcAddress("SetProcessDPIAware", fVista))
	{
		hr = fVista() ? S_OK : E_ACCESSDENIED;
	}

	return hr;
}

void CDpiAware::UpdateStartupInfo(CEStartupEnv* pStartEnv)
{
	if (!pStartEnv)
		return;

	pStartEnv->bIsPerMonitorDpi = IsPerMonitorDpi();
	for (INT_PTR i = static_cast<int>(pStartEnv->nMonitorsCount) - 1; i >= 0; i--)
	{
		// ReSharper disable once CppLocalVariableMayBeConst
		HMONITOR hMon = MonitorFromRect(&pStartEnv->Monitors[i].rcMonitor, MONITOR_DEFAULTTONEAREST);
		if (!hMon)
			continue;

		for (int j = MDT_Effective_DPI; j <= MDT_Raw_DPI; j++)
		{
			const DpiValue dpi = QueryDpiForMonitor(hMon, static_cast<MonitorDpiType>(j));
			pStartEnv->Monitors[i].dpis[j + 1].x = dpi.Xdpi;
			pStartEnv->Monitors[i].dpis[j + 1].y = dpi.Xdpi;
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
			if (0 == RegQueryValueEx(hk, L"Win8DpiScaling", nullptr, nullptr, (LPBYTE)&nValue, &nSize) && nSize == sizeof(nValue))
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

int CDpiAware::QueryDpi(HWND hWnd /*= nullptr*/, DpiValue* pDpi /*= nullptr*/)
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
			const auto dpi = QueryDpiForMonitor(hMon);
			if (pDpi)
				*pDpi = dpi;
			return dpi.GetDpi();
		}
	}

	return QueryDpiForWindow(hWnd, pDpi);
}

// if hWnd is nullptr - returns DC's dpi
int CDpiAware::QueryDpiForWindow(HWND hWnd /*= nullptr*/, DpiValue* pDpi /*= nullptr*/)
{
	#if defined(_DEBUG) && defined(DPI_DEBUG_CUSTOM)
	if (pDpi) pDpi->SetDpi(DPI_DEBUG_CUSTOM,DPI_DEBUG_CUSTOM);
	return DPI_DEBUG_CUSTOM;
	#endif

	int dpi = 96;
	HDC desktopDc = GetDC(hWnd);
	if (desktopDc != nullptr)
	{
		// Get native resolution
		int x = GetDeviceCaps(desktopDc, LOGPIXELSX);
		int y = GetDeviceCaps(desktopDc, LOGPIXELSY);
		ReleaseDC(nullptr, desktopDc);
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

DpiValue CDpiAware::QueryDpiForRect(const RECT& rcWnd, const MonitorDpiType dpiType /*= MDT_Default*/)
{
	HMONITOR hMon = MonitorFromRect(&rcWnd, MONITOR_DEFAULTTONEAREST);
	return QueryDpiForMonitor(hMon, dpiType);
}

// ReSharper disable once CppParameterMayBeConst
DpiValue CDpiAware::QueryDpiForMonitor(HMONITOR hMon, const MonitorDpiType dpiType /*= MDT_Default*/)
{
	DpiValue dpi{};

	#if defined(_DEBUG) && defined(DPI_DEBUG_CUSTOM)
	dpi.SetDpi(DPI_DEBUG_CUSTOM, DPI_DEBUG_CUSTOM);
	return DPI_DEBUG_CUSTOM;
	#endif


	static bool loaded = false;
	if (!loaded)
	{
		GetShCore().GetProcAddress("GetDpiForMonitor", getDpiForMonitor_);
		loaded = true;
	}

	HRESULT hr = E_FAIL;
	if (hMon && (getDpiForMonitor_ != nullptr))
	{
		int x = 0, y = 0;
		hr = getDpiForMonitor_(hMon, dpiType, reinterpret_cast<UINT*>(&x), reinterpret_cast<UINT*>(&y));
		if (SUCCEEDED(hr) && (x > 0) && (y > 0))
		{
			dpi.SetDpi(x, y, DpiValue::DpiSource::PerMonitor);
		}
	}
	else
	{
		int overallX = 0, overallY = 0;
		if (overallX <= 0 || overallY <= 0)
		{
			// ReSharper disable once CppLocalVariableMayBeConst
			HDC hdc = GetDC(nullptr);
			if (hdc)
			{
				overallX = GetDeviceCaps(hdc, LOGPIXELSX);
				overallY = GetDeviceCaps(hdc, LOGPIXELSY);
				ReleaseDC(nullptr, hdc);
			}
		}
		if (overallX > 0 && overallY > 0)
		{
			dpi.SetDpi(overallX, overallY, DpiValue::DpiSource::StaticOverall);
		}
	}

	return dpi;
}

void CDpiAware::GetCenteredRect(HWND hWnd, RECT& rcCentered, HMONITOR hDefault /*= nullptr*/)
{
#ifdef _DEBUG
	HMONITOR hMon;
	if (hWnd)
		hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	else
		hMon = MonitorFromRect(&rcCentered, MONITOR_DEFAULTTONEAREST);
#endif

	MONITORINFO mi = {};
	GetNearestMonitorInfo(&mi, hDefault, hWnd ? nullptr : &rcCentered, hWnd);

	const int iWidth = rcCentered.right - rcCentered.left;
	const int iHeight = rcCentered.bottom - rcCentered.top;

	RECT rcNew{};
	rcNew.left = (mi.rcWork.left + mi.rcWork.right - iWidth) / 2;
	rcNew.top = (mi.rcWork.top + mi.rcWork.bottom - iHeight) / 2;
	rcNew.right = rcNew.left + iWidth;
	rcNew.bottom = rcNew.top + iHeight;

	rcCentered = rcNew;

#ifdef _DEBUG
	std::ignore = hMon;
#endif
}

void CDpiAware::CenterDialog(HWND hDialog)
{
	RECT rect = {}, rectAfter = {};
	if (!hDialog || !GetWindowRect(hDialog, &rect))
		return;

	// If possible, open our startup dialogs on the monitor,
	// where user have clicked our icon (shortcut on the desktop or TaskBar)
	HMONITOR hDefault = ghWnd ? nullptr : gpStartEnv->hStartMon;

	// Position dialog in the workarea center
	CDpiAware::GetCenteredRect(nullptr, rect, hDefault);
	MoveWindowRect(hDialog, rect);

	if (IsPerMonitorDpi() && GetWindowRect(hDialog, &rectAfter))
	{
		if ((RectWidth(rect) != RectWidth(rectAfter)) || (RectHeight(rect) != RectHeight(rectAfter)))
		{
			CDpiAware::GetCenteredRect(nullptr, rectAfter, nullptr);
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

bool CDpiForDialog::Create(CDpiForDialog*& pHelper)
{
	if (pHelper)
		return true;

	// In Win10 and newly implemented PerMonitor DPI support we don't need dialog handlers anymore
	if (IsWin10())
		return false;

	// Only if required
	if (!CDpiAware::IsPerMonitorDpi())
		return false;

	pHelper = new CDpiForDialog();
	return (pHelper != nullptr);
}

CDpiForDialog::CDpiForDialog()
{
	mh_Dlg = nullptr;
	//mn_InitFontHeight = 8;
	ZeroStruct(mlf_InitFont);
	mn_TemplateFontSize = 8;
	mn_CurFontHeight = 0;
	ZeroStruct(mlf_CurFont);
	mh_OldFont = mh_CurFont = nullptr;
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
	if ((mh_OldFont != nullptr)
		&& (GetObject(mh_OldFont, sizeof(mlf_InitFont), &mlf_InitFont) > 0))
	{
		swprintf_c(szLog, L"CDpiForDialog(x%08X) Font='%s' lfHeight=%i Points=%u", (DWORD)(DWORD_PTR)hWnd, mlf_InitFont.lfFaceName, mlf_InitFont.lfHeight, mn_TemplateFontSize);
	}
	else
	{
		ZeroStruct(mlf_InitFont);
		mlf_InitFont.lfHeight = GetFontSizeForDpi(nullptr, 96);
		lstrcpyn(mlf_InitFont.lfFaceName, L"MS Shell Dlg", countof(mlf_InitFont.lfFaceName));
		mlf_InitFont.lfWeight = 400;
		mlf_InitFont.lfCharSet = DEFAULT_CHARSET;
		swprintf_c(szLog, L"CDpiForDialog(x%08X) DefaultFont='%s' lfHeight=%i Points=%u", (DWORD)(DWORD_PTR)hWnd, mlf_InitFont.lfFaceName, mlf_InitFont.lfHeight, mn_TemplateFontSize);
	}

	LogString(szLog);

	// Up to Windows 8 - OS will care of dialog scaling
	// And what will happen in Windows 8.1?
	// If `Per-monitor` dpi was choosed in the OS settings,
	// we need to re-scale our dialog manually!
	// But if one dpi was chosen for all monitors?

	m_InitDpi = CDpiAware::QueryDpiForMonitor(nullptr); // Whole desktop DPI (in most cases that will be Primary monitor DPI)
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
		MArray<DlgItem>* p = nullptr;
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

	return true;
}

bool CDpiForDialog::SetDialogDPI(const DpiValue& newDpi, LPRECT lprcSuggested /*= nullptr*/)
{
	wchar_t szLog[160];
	RECT rcClient = {}, rcCurWnd = {};

	#ifdef _DEBUG
	if (gbSkipSetDialogDPI)
	{
		GetClientRect(mh_Dlg, &rcClient); GetWindowRect(mh_Dlg, &rcCurWnd);
		swprintf_c(szLog, L"SKIPPED CDpiForDialog::SetDialogDPI x%08X, OldDpi={%i,%i}, NewDpi={%i,%i}, CurSize={%i,%i}, CurClient={%i,%i}",
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
		swprintf_c(szLog, L"CDpiForDialog::SetDialogDPI x%08X forces larger dpi value from {%i,%i} to {%i,%i}",
			(DWORD)(DWORD_PTR)mh_Dlg, newDpi.Xdpi, newDpi.Ydpi, setDpi.Xdpi, setDpi.Ydpi);
		LogString(szLog);
	}

	if (m_CurDpi.Equals(setDpi))
		return false;

	bool bRc = false;
	MArray<DlgItem>* p = nullptr;
	DpiValue curDpi(m_CurDpi);
	HFONT hf = nullptr;

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
	mn_CurFontHeight = GetFontSizeForDpi(nullptr, m_CurDpi.Ydpi);
	//(m_CurDpi.Ydpi && m_InitDpi.Ydpi) ? (mn_InitFontHeight * m_CurDpi.Ydpi / m_InitDpi.Ydpi) : -11;
	mlf_CurFont = mlf_InitFont;
	mlf_CurFont.lfHeight = mn_CurFontHeight;
	mlf_CurFont.lfWidth = 0; // Font mapper fault

	if (mn_CurFontHeight == 0)
		goto wrap;


	if (!m_Items.Get(m_CurDpi.Ydpi, &p))
	{
		MArray<DlgItem>* pOrig = nullptr;
		int iOrigDpi = 0;
		if (!m_Items.GetNext(nullptr, &iOrigDpi, &pOrig) || !pOrig || (iOrigDpi <= 0))
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
		swprintf_c(szLog, L"CDpiForDialog::SetDialogDPI x%08X, OldDpi={%i,%i}, NewDpi={%i,%i}, OldSize={%i,%i}, NewSize={%i,%i}, NewFont=%i",
			(DWORD)(DWORD_PTR)mh_Dlg, curDpi.Xdpi, curDpi.Ydpi, newDpi.Xdpi, newDpi.Ydpi,
			(rcCurWnd.right - rcCurWnd.left), (rcCurWnd.bottom - rcCurWnd.top), di.r.right, di.r.bottom,
			mlf_CurFont.lfHeight);
		LogString(szLog);
	}

	hf = CreateFontIndirect(&mlf_CurFont);
	if (hf == nullptr)
	{
		goto wrap;
	}

	for (INT_PTR k = p->size() - 1; k >= 1; k--)
	{
		const DlgItem& di = (*p)[k];
		GetClassName(di.h, szClass, countof(szClass));
		DWORD nCtrlId = GetWindowLong(di.h, GWL_ID);
		DWORD nStyles = GetWindowLong(di.h, GWL_STYLE);
		bool bResizeCombo = (lstrcmpi(szClass, L"ComboBox") == 0);
		int iComboFieldHeight = 0, iComboWasHeight = 0;
		LONG_PTR lFieldHeight = 0, lNewHeight = 0;
		RECT rcCur = {};
		HWND hComboEdit = nullptr;
		RECT rcEdit = {}, rcItemClient = {};

		if (bResizeCombo && (nStyles & CBS_OWNERDRAWFIXED))
		{
			GetWindowRect(di.h, &rcCur);
			hComboEdit = FindWindowEx(di.h, nullptr, L"Edit", nullptr);
			GetClientRect(di.h, &rcItemClient);
			GetClientRect(hComboEdit, &rcEdit);
			iComboWasHeight = (rcCur.bottom - rcCur.top);
			lFieldHeight = SendMessage(di.h, CB_GETITEMHEIGHT, -1, 0);
			if (lFieldHeight < iComboWasHeight && lFieldHeight > 0)
			{
				iComboFieldHeight = static_cast<int>(lFieldHeight);
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
				swprintf_c(szLog, L"CDpiForDialog::Combo height changed - OldHeight=%i, ItemHeight=%i, NewHeight=%i, NewItemHeight=%i",
					(rcCur.bottom - rcCur.top), lFieldHeight, newH, lNewHeight);
				LogString(szLog);
				SendMessage(di.h, CB_SETITEMHEIGHT, -1, lNewHeight);
			}
			SendMessage(di.h, CB_SETEDITSEL, 0, MAKELPARAM(-1,0));
		}
		EditIconHint_ResChanged(di.h);
		InvalidateRect(di.h, nullptr, TRUE);
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
		SetWindowPos(mh_Dlg, nullptr,
			lprcSuggested ? lprcSuggested->left : 0, lprcSuggested ? lprcSuggested->top : 0,
			di.r.right, di.r.bottom,
			nWndFlags);
		RECT rc = {}; GetClientRect(mh_Dlg, &rc);
		InvalidateRect(mh_Dlg, nullptr, TRUE);
		RedrawWindow(mh_Dlg, &rc, nullptr, /*RDW_ERASE|*/RDW_ALLCHILDREN/*|RDW_INVALIDATE|RDW_UPDATENOW|RDW_INTERNALPAINT*/);
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
		MArray<DlgItem>* p = nullptr;
		int iDpi = 0;
		while (m_Items.GetNext(iDpi ? &iDpi : nullptr, &iDpi, &p))
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
			DpiValue dpi = DpiValue::FromWParam(wParam);
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
		return nullptr;
	}
	OffsetRect(&i.r, -i.r.left, -i.r.top);
	p->push_back(i);

	i.h = nullptr;
	while ((i.h = FindWindowEx(hDlg, i.h, nullptr, nullptr)) != nullptr)
	{
		if (GetWindowRect(i.h, &i.r) && MapWindowPoints(nullptr, hDlg, (LPPOINT)&i.r, 2))
		{
			#ifdef _DEBUG
			DWORD_PTR ID = GetWindowLong(i.h, GWL_ID);
			if (ID == cbExtendFonts)
			{
				RECT rcMargin = {};
				if (Button_GetTextMargin(i.h, &rcMargin))
				{
					wchar_t szLog[100];
					swprintf_c(szLog, L"CheckBox Rect={%i,%i}-{%i,%i} Margin={%i,%i}-{%i,%i}",
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
		//bool bSelfDC = (hdc == nullptr);
		//if (bSelfDC) hdc = GetDC(mh_Dlg);
		UINT nTemplSize = mn_TemplateFontSize ? mn_TemplateFontSize : 8;
		int logY = 99; //GetDeviceCaps(hdc, LOGPIXELSY);
		//if (logY <= 0) logY = 99;
		iFontHeight = -MulDiv(nTemplSize, Ydpi*logY, 72*96);
		//if (bSelfDC && hdc) ReleaseDC(mh_Dlg, hdc);
	}

	return iFontHeight;
}
