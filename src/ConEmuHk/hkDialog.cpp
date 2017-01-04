
/*
Copyright (c) 2015-2017 Maximus5
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

#ifdef _DEBUG
	#define DebugString(x) //OutputDebugString(x)
#else
	#define DebugString(x) //OutputDebugString(x)
#endif

#include "../common/Common.h"

#include "hkDialog.h"
#include "GuiAttach.h"
#include "MainThread.h"

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
	ORIGINAL_EX(CreateDialogIndirectParamA);
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
	ORIGINAL_EX(CreateDialogIndirectParamW);
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
	ORIGINAL_EX(CreateDialogParamA);
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
	ORIGINAL_EX(CreateDialogParamW);
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


// !!! UNDOCUMENTED !!!
// Old note: Required for bring create dialog on top (Z-order) during shell operations
//           Does this mention "Run as" (pre-UAC)
//           or Explorer's choose application (start using) - can't remember.
// New note: It's required for proper function of ChooseColor functions
INT_PTR WINAPI OnDialogBoxIndirectParamAorW(HINSTANCE hInstance, LPCDLGTEMPLATE hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam, DWORD Flags)
{
	//typedef INT_PTR (WINAPI* OnDialogBoxIndirectParamAorW_t)(HINSTANCE hInstance, LPCDLGTEMPLATE hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam, DWORD Flags);
	ORIGINAL_EX(DialogBoxIndirectParamAorW);
	INT_PTR iRc = 0;

	if (ghConEmuWndDC)
	{
		// Необходимо "поднять" наверх консольное окно, иначе Shell-овский диалог окажется ПОД ConEmu
		GuiSetForeground(hWndParent ? hWndParent : ghConWnd);
		// bugreport from Andrey Budko: conemu + emenu/{Run Sandboxed} замораживает фар
		PatchDialogParentWnd(hWndParent);
	}

	if (F(DialogBoxIndirectParamAorW))
		iRc = F(DialogBoxIndirectParamAorW)(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam, Flags);

	return iRc;
}

BOOL WINAPI OnBeep(DWORD dwFreq, DWORD dwDuration)
{
	//typedef BOOL (WINAPI* OnBeep_t)(DWORD dwFreq, DWORD dwDuration);
	ORIGINAL_EX(Beep);
	BOOL lbRc = FALSE;

	if (isSuppressBells())
	{
		wchar_t szBeep[64]; msprintf(szBeep, countof(szBeep), L"--- Skipped Beep(Freq=%u,Dur=%u)\n", dwFreq, dwDuration);
		LogBeepSkip(szBeep);
		goto wrap;
	}

	if (F(Beep))
	{
		lbRc = F(Beep)(dwFreq, dwDuration);
	}

wrap:
	return lbRc;
}

BOOL WINAPI OnMessageBeep(UINT uType)
{
	//typedef BOOL (WINAPI* OnMessageBeep_t)(UINT uType);
	ORIGINAL_EX(MessageBeep);
	BOOL lbRc = FALSE;

	if (isSuppressBells())
	{
		wchar_t szBeep[48]; msprintf(szBeep, countof(szBeep), L"--- Skipped MessageBeep(ID=0x%X)\n", uType);
		LogBeepSkip(szBeep);
		goto wrap;
	}

	if (F(MessageBeep))
	{
		lbRc = F(MessageBeep)(uType);
	}

wrap:
	return lbRc;
}
