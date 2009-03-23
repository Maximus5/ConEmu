#include <windows.h>
#include <Commctrl.h>
#include <uxtheme.h>
#include "header.h"


CProgressBars::CProgressBars(HWND hWnd, HINSTANCE hInstance)
{
	this->hWnd=hWnd;
	Progressbar1 = CreateWindow(PROGRESS_CLASS, NULL, PBS_SMOOTH | WS_CHILD | TCS_FOCUSNEVER, 0, 0, 100, 10, hWnd, NULL, hInstance, NULL);
	Progressbar2 = CreateWindow(PROGRESS_CLASS, NULL, PBS_SMOOTH | WS_CHILD | TCS_FOCUSNEVER, 0, 0, 100, 10, hWnd, NULL, hInstance, NULL);

	SendMessage(Progressbar1, PBM_SETRANGE, 0, (LPARAM) MAKELPARAM (0, 100));
	SendMessage(Progressbar2, PBM_SETRANGE, 0, (LPARAM) MAKELPARAM (0, 100));		

	mh_Uxtheme = LoadLibrary(_T("UxTheme.dll"));
	if (mh_Uxtheme) {
		SetWindowThemeF = (SetWindowThemeT)GetProcAddress(mh_Uxtheme, "SetWindowTheme");
		if (SetWindowThemeF) {
			SetWindowThemeF(Progressbar1, _T(" "), _T(" "));
			SetWindowThemeF(Progressbar2, _T(" "), _T(" "));
		}
	}

//	COL_DIALOGTEXT
}

CProgressBars::~CProgressBars()
{
	if (mh_Uxtheme) {
		FreeLibrary(mh_Uxtheme); mh_Uxtheme = NULL;
	}
}

void CProgressBars::OnTimer()
{
	if (gSet.isGUIpb && isCoping())
	{
		int delta=0;
		if (pVCon->ConChar[((pVCon->TextHeight-2)/2)*pVCon->TextWidth + (pVCon->TextWidth-45)/2 + 45] == _T('%'))
			delta=1; //нет общего индикатора

		if (pVCon->ConChar[((pVCon->TextHeight-4)/2+delta)*pVCon->TextWidth + (pVCon->TextWidth-45)/2 + 45] == _T('%'))
		{
			RECT rcShift; memset(&rcShift, 0, sizeof(RECT));
			MapWindowPoints(ghWndDC, ghWnd, (LPPOINT)&rcShift, 2);

			WCHAR tmp[4];
			tmp[0]=pVCon->ConChar[((pVCon->TextHeight-4)/2+delta)*pVCon->TextWidth + (pVCon->TextWidth-45)/2 + 42];
			tmp[1]=pVCon->ConChar[((pVCon->TextHeight-4)/2+delta)*pVCon->TextWidth + (pVCon->TextWidth-45)/2 + 43];
			tmp[2]=pVCon->ConChar[((pVCon->TextHeight-4)/2+delta)*pVCon->TextWidth + (pVCon->TextWidth-45)/2 + 44];
			tmp[3]=0;
			int perc;
			swscanf_s(tmp, _T("%i"), &perc);

			SendMessage(Progressbar1, PBM_SETPOS, perc, 0);
			SetWindowPos(Progressbar1, 0, 
				((pVCon->TextWidth-45)/2)*gSet.LogFont.lfWidth+rcShift.left, 
				(pVCon->TextHeight-4+delta*2)/2*gSet.LogFont.lfHeight+rcShift.top/*TabBar.Height()*/, 
				gSet.LogFont.lfWidth*41, gSet.LogFont.lfHeight, 0);
			ShowWindow(Progressbar1, SW_SHOW);

			if (pVCon->ConChar[((pVCon->TextHeight)/2)*pVCon->TextWidth + (pVCon->TextWidth-45)/2 + 45] == _T('%'))
			{
				WCHAR tmp[4];
				tmp[0]=pVCon->ConChar[((pVCon->TextHeight)/2)*pVCon->TextWidth + (pVCon->TextWidth-45)/2 + 42];
				tmp[1]=pVCon->ConChar[((pVCon->TextHeight)/2)*pVCon->TextWidth + (pVCon->TextWidth-45)/2 + 43];
				tmp[2]=pVCon->ConChar[((pVCon->TextHeight)/2)*pVCon->TextWidth + (pVCon->TextWidth-45)/2 + 44];
				tmp[3]=0;
				int perc;
				swscanf_s(tmp, _T("%i"), &perc);

				SendMessage(Progressbar2, PBM_SETPOS, perc, 0);
				SetWindowPos(Progressbar2, 0, 
					((pVCon->TextWidth-45)/2)*gSet.LogFont.lfWidth+rcShift.left, 
					(pVCon->TextHeight)/2*gSet.LogFont.lfHeight+rcShift.top/*TabBar.Height()*/, 
					gSet.LogFont.lfWidth*41, gSet.LogFont.lfHeight, 0);
				ShowWindow(Progressbar2, SW_SHOW);
			}
			else
				ShowWindow(Progressbar2, SW_HIDE);
		}
	//	else
	//		ShowWindow(Progressbar1, SW_HIDE);
	}
	else
	{
		ShowWindow(Progressbar1, SW_HIDE);
		ShowWindow(Progressbar2, SW_HIDE);
	}
}

bool CProgressBars::isCoping()
{
	//TODO: Локализация
	LPCTSTR pszTitle = gConEmu.GetTitle();
	if (wcsstr(pszTitle, _T("Копирование"))
		|| wcsstr(pszTitle, _T("Copying"))
		|| wcsstr(pszTitle, _T("Перенос"))
		|| wcsstr(pszTitle, _T("Moving"))
		)
		return true;
	return false;
}