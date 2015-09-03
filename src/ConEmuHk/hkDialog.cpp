
/*
Copyright (c) 2015 Maximus5
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

#ifdef _DEBUG
	#define DebugString(x) //OutputDebugString(x)
#else
	#define DebugString(x) //OutputDebugString(x)
#endif

#include "../common/common.hpp"

#define DEFINE_HOOK_MACROS
#include "ConEmuHooks.h"
#include "hkDialog.h"
#include "GuiAttach.h"
#include "MainThread.h"
#include "SetHook.h"

/* **************** */

OnChooseColorA_t ChooseColorA_f = NULL; // For Del
OnChooseColorW_t ChooseColorW_f = NULL; // For Del

/* **************** */

extern HMODULE ghOurModule;
extern BOOL GuiSetForeground(HWND hWnd);

/* **************** */

void PatchDialogParentWnd(HWND& hWndParent)
{
	WARNING("Проверить все перехваты диалогов (A/W). По идее, надо менять hWndParent, а то диалоги прячутся");
	// Re: conemu + emenu/{Run Sandboxed} замораживает фар

	if (ghConEmuWndDC)
	{
		if (!hWndParent || !IsWindowVisible(hWndParent))
			hWndParent = ghConEmuWnd;
	}
}

/* **************** */

HWND WINAPI OnCreateDialogIndirectParamA(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit)
{
	//typedef HWND (WINAPI* OnCreateDialogIndirectParamA_t)(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit);
	ORIGINALFASTEX(CreateDialogIndirectParamA,NULL);
	HWND hWnd = NULL;
	BOOL bAttachGui = (hWndParent == NULL), bStyleHidden = FALSE;
	// Со стилями - полная фигня сюда попадает
	DWORD lStyle = lpTemplate ? lpTemplate->style : 0;
	DWORD lStyleEx = lpTemplate ? lpTemplate->dwExtendedStyle : 0;

	if (/*CheckCanCreateWindow((LPCSTR)32770, NULL, lStyle, lStyleEx, hWndParent, bAttachGui, bStyleHidden)
		&&*/ F(CreateDialogIndirectParamA) != NULL)
	{
		//if (bAttachGui)
		//{
		//	x = grcConEmuClient.left; y = grcConEmuClient.top;
		//	nWidth = grcConEmuClient.right - grcConEmuClient.left; nHeight = grcConEmuClient.bottom - grcConEmuClient.top;
		//}

		hWnd = F(CreateDialogIndirectParamA)(hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit);
		DWORD dwErr = GetLastError();

		if (hWnd && bAttachGui)
		{
			lStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
			lStyleEx = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE);
			if (CheckCanCreateWindow((LPCSTR)32770, NULL, lStyle, lStyleEx, hWndParent, bAttachGui, bStyleHidden) && bAttachGui)
			{
				OnGuiWindowAttached(hWnd, NULL, (LPCSTR)32770, NULL, lStyle, lStyleEx, bStyleHidden);
			}

			SetLastError(dwErr);
		}
	}

	return hWnd;
}


HWND WINAPI OnCreateDialogIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit)
{
	//typedef HWND (WINAPI* OnCreateDialogIndirectParamW_t)(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit);
	ORIGINALFASTEX(CreateDialogIndirectParamW,NULL);
	HWND hWnd = NULL;
	BOOL bAttachGui = (hWndParent == NULL), bStyleHidden = FALSE;
	// Со стилями - полная фигня сюда попадает
	DWORD lStyle = lpTemplate ? lpTemplate->style : 0;
	DWORD lStyleEx = lpTemplate ? lpTemplate->dwExtendedStyle : 0;

	if (/*CheckCanCreateWindow(NULL, (LPCWSTR)32770, lStyle, lStyleEx, hWndParent, bAttachGui, bStyleHidden)
		&&*/ F(CreateDialogIndirectParamW) != NULL)
	{
		//if (bAttachGui)
		//{
		//	x = grcConEmuClient.left; y = grcConEmuClient.top;
		//	nWidth = grcConEmuClient.right - grcConEmuClient.left; nHeight = grcConEmuClient.bottom - grcConEmuClient.top;
		//}

		hWnd = F(CreateDialogIndirectParamW)(hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit);
		DWORD dwErr = GetLastError();

		if (hWnd && bAttachGui)
		{
			lStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
			lStyleEx = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE);
			if (CheckCanCreateWindow((LPCSTR)32770, NULL, lStyle, lStyleEx, hWndParent, bAttachGui, bStyleHidden) && bAttachGui)
			{
				OnGuiWindowAttached(hWnd, NULL, NULL, (LPCWSTR)32770, lStyle, lStyleEx, bStyleHidden);
			}

			SetLastError(dwErr);
		}
	}

	return hWnd;
}


HWND WINAPI OnCreateDialogParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	//typedef HWND (WINAPI* OnCreateDialogParamA_t)(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	ORIGINALFASTEX(CreateDialogParamA,NULL);
	HWND hWnd = NULL;
	BOOL bAttachGui = FALSE, bStyleHidden = FALSE;
	LPCDLGTEMPLATE lpTemplate = NULL;
	DWORD lStyle = 0; //lpTemplate ? lpTemplate->style : 0;
	DWORD lStyleEx = 0; //lpTemplate ? lpTemplate->dwExtendedStyle : 0;

	// Загрузить ресурс диалога, и глянуть его параметры lStyle/lStyleEx
	HRSRC hDlgSrc = FindResourceA(hInstance, lpTemplateName, /*RT_DIALOG*/MAKEINTRESOURCEA(5));
	if (hDlgSrc)
	{
		HGLOBAL hDlgHnd = LoadResource(hInstance, hDlgSrc);
		if (hDlgHnd)
		{
			lpTemplate = (LPCDLGTEMPLATE)LockResource(hDlgHnd);
			if (lpTemplate)
			{
				lStyle = lpTemplate ? lpTemplate->style : 0;
				lStyleEx = lpTemplate ? lpTemplate->dwExtendedStyle : 0;
			}
		}
	}

	if ((!lpTemplate || CheckCanCreateWindow((LPSTR)32770, NULL, lStyle, lStyleEx, hWndParent, bAttachGui, bStyleHidden))
		&& F(CreateDialogParamA) != NULL)
	{
		//if (bAttachGui)
		//{
		//	x = grcConEmuClient.left; y = grcConEmuClient.top;
		//	nWidth = grcConEmuClient.right - grcConEmuClient.left; nHeight = grcConEmuClient.bottom - grcConEmuClient.top;
		//}

		hWnd = F(CreateDialogParamA)(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
		DWORD dwErr = GetLastError();

		if (hWnd && bAttachGui)
		{
			OnGuiWindowAttached(hWnd, NULL, (LPCSTR)32770, NULL, lStyle, lStyleEx, bStyleHidden);

			SetLastError(dwErr);
		}
	}

	return hWnd;
}


HWND WINAPI OnCreateDialogParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	//typedef HWND (WINAPI* OnCreateDialogParamW_t)(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	ORIGINALFASTEX(CreateDialogParamW,NULL);
	HWND hWnd = NULL;
	BOOL bAttachGui = FALSE, bStyleHidden = FALSE;
	LPCDLGTEMPLATE lpTemplate = NULL;
	DWORD lStyle = 0; //lpTemplate ? lpTemplate->style : 0;
	DWORD lStyleEx = 0; //lpTemplate ? lpTemplate->dwExtendedStyle : 0;

	// Загрузить ресурс диалога, и глянуть его параметры lStyle/lStyleEx
	HRSRC hDlgSrc = FindResourceW(hInstance, lpTemplateName, RT_DIALOG);
	if (hDlgSrc)
	{
		HGLOBAL hDlgHnd = LoadResource(hInstance, hDlgSrc);
		if (hDlgHnd)
		{
			lpTemplate = (LPCDLGTEMPLATE)LockResource(hDlgHnd);
			if (lpTemplate)
			{
				lStyle = lpTemplate ? lpTemplate->style : 0;
				lStyleEx = lpTemplate ? lpTemplate->dwExtendedStyle : 0;
			}
		}
	}

	if ((!lpTemplate || CheckCanCreateWindow(NULL, (LPWSTR)32770, lStyle, lStyleEx, hWndParent, bAttachGui, bStyleHidden))
		&& F(CreateDialogParamW) != NULL)
	{
		//if (bAttachGui)
		//{
		//	x = grcConEmuClient.left; y = grcConEmuClient.top;
		//	nWidth = grcConEmuClient.right - grcConEmuClient.left; nHeight = grcConEmuClient.bottom - grcConEmuClient.top;
		//}

		hWnd = F(CreateDialogParamW)(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
		DWORD dwErr = GetLastError();

		if (hWnd && bAttachGui)
		{
			OnGuiWindowAttached(hWnd, NULL, NULL, (LPCWSTR)32770, lStyle, lStyleEx, bStyleHidden);

			SetLastError(dwErr);
		}
	}

	return hWnd;
}


// Нужна для "поднятия" консольного окна при вызове Shell операций
INT_PTR WINAPI OnDialogBoxParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	//typedef INT_PTR (WINAPI* OnDialogBoxParamW_t)(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	ORIGINALFASTEX(DialogBoxParamW,NULL);
	INT_PTR iRc = 0;

	if (ghConEmuWndDC)
	{
		// Необходимо "поднять" наверх консольное окно, иначе Shell-овский диалог окажется ПОД ConEmu
		GuiSetForeground(hWndParent ? hWndParent : ghConWnd);
		// bugreport from Andrey Budko: conemu + emenu/{Run Sandboxed} замораживает фар
		PatchDialogParentWnd(hWndParent);
	}

	if (F(DialogBoxParamW))
		iRc = F(DialogBoxParamW)(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);

	return iRc;
}





/* *************************** */
/*       FOR DELETION          */
/* *************************** */

typedef BOOL (WINAPI* SimpleApiFunction_t)(void*);
struct SimpleApiFunctionArg
{
	enum SimpleApiFunctionType
	{
		sft_ChooseColorA = 1,
		sft_ChooseColorW = 2,
	} funcType;
	SimpleApiFunction_t funcPtr;
	void  *pArg;
	BOOL   bResult;
	DWORD  nLastError;
};

INT_PTR CALLBACK SimpleApiDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SimpleApiFunctionArg* P = NULL;

	if (uMsg == WM_INITDIALOG)
	{
		P = (SimpleApiFunctionArg*)lParam;
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);

		int x = 0, y = 0;
		RECT rcDC = {};
		if (!GetWindowRect(ghConEmuWndDC, &rcDC))
			SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcDC, 0);
		if (rcDC.right > rcDC.left && rcDC.bottom > rcDC.top)
		{
			x = max(rcDC.left, ((rcDC.right+rcDC.left-300)>>1));
			y = max(rcDC.top, ((rcDC.bottom+rcDC.top-200)>>1));
		}

		//typedef BOOL (WINAPI* SetWindowText_t)(HWND,LPCWSTR);
		//SetWindowText_t SetWindowText_f = (SetWindowText_t)GetProcAddress(ghUser32, "SetWindowTextW");


		switch (P->funcType)
		{
			case SimpleApiFunctionArg::sft_ChooseColorA:
				((LPCHOOSECOLORA)P->pArg)->hwndOwner = hwndDlg;
				SetWindowTextW(hwndDlg, L"ConEmu ColorPicker");
				break;
			case SimpleApiFunctionArg::sft_ChooseColorW:
				((LPCHOOSECOLORW)P->pArg)->hwndOwner = hwndDlg;
				SetWindowTextW(hwndDlg, L"ConEmu ColorPicker");
				break;
		}

		SetWindowPos(hwndDlg, HWND_TOP, x, y, 0, 0, SWP_SHOWWINDOW);

		P->bResult = P->funcPtr(P->pArg);
		P->nLastError = GetLastError();

		//typedef BOOL (WINAPI* EndDialog_t)(HWND,INT_PTR);
		//EndDialog_t EndDialog_f = (EndDialog_t)GetProcAddress(ghUser32, "EndDialog");
		EndDialog(hwndDlg, 1);

		return FALSE;
	}

	switch (uMsg)
	{
	case WM_GETMINMAXINFO:
		{
			MINMAXINFO* p = (MINMAXINFO*)lParam;
			p->ptMinTrackSize.x = p->ptMinTrackSize.y = 0;
			SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0);
		}
		return TRUE;
	}

	return FALSE;
}

BOOL MyChooseColor(SimpleApiFunction_t funcPtr, void* lpcc, BOOL abUnicode)
{
	BOOL lbRc = FALSE;

	SimpleApiFunctionArg Arg =
	{
		abUnicode?SimpleApiFunctionArg::sft_ChooseColorW:SimpleApiFunctionArg::sft_ChooseColorA,
		funcPtr, lpcc
	};

	DLGTEMPLATE* pTempl = (DLGTEMPLATE*)GlobalAlloc(GPTR, sizeof(DLGTEMPLATE)+3*sizeof(WORD)); //-V119
	if (!pTempl)
	{
		_ASSERTE(pTempl!=NULL);
		Arg.bResult = Arg.funcPtr(Arg.pArg);
		Arg.nLastError = GetLastError();
		return Arg.bResult;
	}
	pTempl->style = WS_POPUP|/*DS_MODALFRAME|DS_CENTER|*/DS_SETFOREGROUND;

	INT_PTR iRc = DialogBoxIndirectParam(ghOurModule, pTempl, NULL, SimpleApiDialogProc, (LPARAM)&Arg);
	if (iRc == 1)
	{
		lbRc = Arg.bResult;
		SetLastError(Arg.nLastError);
	}

	return lbRc;
}

/*
Replace them with hooking of 

INT_PTR WINAPI DialogBoxIndirectParamAorW(
  _In_opt_  HINSTANCE hInstance,
  _In_      LPCDLGTEMPLATE hDialogTemplate,
  _In_opt_  HWND hWndParent,
  _In_opt_  DLGPROC lpDialogFunc,
  _In_      LPARAM dwInitParam,
  _In_      DWORD Flags
);

These hooks were created for maintain Far's "Color picker" plugin
*/

BOOL WINAPI OnChooseColorA(LPCHOOSECOLORA lpcc)
{
	ORIGINALFASTEX(ChooseColorA,ChooseColorA_f);
	BOOL lbRc;
	if (lpcc->hwndOwner == NULL || lpcc->hwndOwner == GetRealConsoleWindow())
		lbRc = MyChooseColor((SimpleApiFunction_t)F(ChooseColorA), lpcc, FALSE);
	else
		lbRc = F(ChooseColorA)(lpcc);
	return lbRc;
}

BOOL WINAPI OnChooseColorW(LPCHOOSECOLORW lpcc)
{
	ORIGINALFASTEX(ChooseColorW,ChooseColorW_f);
	BOOL lbRc;
	if (lpcc->hwndOwner == NULL || lpcc->hwndOwner == GetRealConsoleWindow())
		lbRc = MyChooseColor((SimpleApiFunction_t)F(ChooseColorW), lpcc, TRUE);
	else
		lbRc = F(ChooseColorW)(lpcc);
	return lbRc;
}

/* *************************** */
/*       FOR DELETION          */
/* *************************** */
