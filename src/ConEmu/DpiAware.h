
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

#pragma once

#include "../common/MArray.h"
#include "../common/MMap.h"

enum ProcessDpiAwareness
{
	Process_DPI_Unaware            = 0,
	Process_System_DPI_Aware       = 1,
	Process_Per_Monitor_DPI_Aware  = 2
};

enum MonitorDpiType
{
	MDT_Effective_DPI  = 0,
	MDT_Angular_DPI    = 1,
	MDT_Raw_DPI        = 2,
	MDT_Default        = MDT_Effective_DPI
};

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

struct DpiValue
{
public:
	int Ydpi;
	int Xdpi;

public:
	DpiValue();

	DpiValue(WPARAM wParam);

	DpiValue(const DpiValue& dpi);

public:
	operator int() const;
	struct DpiValue& operator=(WORD dpi);
	bool Equals(const DpiValue& dpi) const;

	void SetDpi(int xdpi, int ydpi);
	void SetDpi(const DpiValue& dpi);
	void OnDpiChanged(WPARAM wParam);
};

struct CEStartupEnv;

class CDpiAware
{
public:
	static HRESULT setProcessDPIAwareness();

	static void UpdateStartupInfo(CEStartupEnv* pStartEnv);

	static bool IsPerMonitorDpi();

	static int QueryDpi(HWND hWnd = NULL, DpiValue* pDpi = NULL);

	// if hWnd is NULL - returns DC's dpi
	static int QueryDpiForWindow(HWND hWnd = NULL, DpiValue* pDpi = NULL);

	static int QueryDpiForRect(const RECT& rcWnd, DpiValue* pDpi = NULL, MonitorDpiType dpiType = MDT_Default);
	static int QueryDpiForMonitor(HMONITOR hmon, DpiValue* pDpi = NULL, MonitorDpiType dpiType = MDT_Default);

	// Dialog helper
	static void GetCenteredRect(HWND hWnd, RECT& rcCentered, HMONITOR hDefault = NULL);
	static void CenterDialog(HWND hDialog);
};

class CDynDialog;

class CDpiForDialog
{
protected:
	HWND mh_Dlg;

	LONG mn_InSet;

	DpiValue m_InitDpi;
	LOGFONT mlf_InitFont;
	DpiValue m_CurDpi;
	UINT mn_TemplateFontSize;
	int mn_CurFontHeight;
	LOGFONT mlf_CurFont;

	HFONT mh_OldFont, mh_CurFont;

	struct DlgItem
	{
		HWND h;
		RECT r;
	};
	MMap<int, MArray<DlgItem>*> m_Items;

public:
	CDpiForDialog();

	~CDpiForDialog();

	bool Attach(HWND hWnd, HWND hCenterParent, CDynDialog* apDlgTemplate);

	bool SetDialogDPI(const DpiValue& newDpi, LPRECT lprcSuggested = NULL);

	void Detach();

public:
	const DpiValue& GetCurDpi() const;
	bool ProcessDpiMessages(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam);

protected:
	static MArray<DlgItem>* LoadDialogItems(HWND hDlg);
	int GetFontSizeForDpi(HDC hdc, int Ydpi);
};
