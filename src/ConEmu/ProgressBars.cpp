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
	
	SetWindowTheme(Progressbar1, _T(" "), _T(" "));
	SetWindowTheme(Progressbar2, _T(" "), _T(" "));

//	COL_DIALOGTEXT
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
			WCHAR tmp[4];
			tmp[0]=pVCon->ConChar[((pVCon->TextHeight-4)/2+delta)*pVCon->TextWidth + (pVCon->TextWidth-45)/2 + 42];
			tmp[1]=pVCon->ConChar[((pVCon->TextHeight-4)/2+delta)*pVCon->TextWidth + (pVCon->TextWidth-45)/2 + 43];
			tmp[2]=pVCon->ConChar[((pVCon->TextHeight-4)/2+delta)*pVCon->TextWidth + (pVCon->TextWidth-45)/2 + 44];
			tmp[3]=0;
			int perc;
			swscanf_s(tmp, _T("%i"), &perc);

			SendMessage(Progressbar1, PBM_SETPOS, perc, 0);
			SetWindowPos(Progressbar1, 0, ((pVCon->TextWidth-45)/2)*gSet.LogFont.lfWidth, (pVCon->TextHeight-4+delta*2)/2*gSet.LogFont.lfHeight+TabBar.Height(), gSet.LogFont.lfWidth*41, gSet.LogFont.lfHeight, 0);
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
				SetWindowPos(Progressbar2, 0, ((pVCon->TextWidth-45)/2)*gSet.LogFont.lfWidth, (pVCon->TextHeight)/2*gSet.LogFont.lfHeight+TabBar.Height(), gSet.LogFont.lfWidth*41, gSet.LogFont.lfHeight, 0);
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
	wchar_t *a = wcsstr(gConEmu.Title, _T("Копирование"));
	wchar_t *b = wcsstr(gConEmu.Title, _T("Copying"));
	wchar_t *c = wcsstr(gConEmu.Title, _T("Перенос"));
	wchar_t *d = wcsstr(gConEmu.Title, _T("Moving"));
	return (a || b || c || d);// && (Title[0] == '{');
}