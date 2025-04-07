#include "Dark.h"
#include "IatHook.h"

static fnOpenNcThemeData _OpenNcThemeData = nullptr;

void DarkModeInit(HWND hWnd)
{
	// Dark window titlebar
	INT value = 1;
	DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));

	// Dark menus and dark scrollbars
	HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hUxtheme)
	{
		_OpenNcThemeData = reinterpret_cast<fnOpenNcThemeData>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(49)));
		DarkFixScrollBar();
		fnSetPreferredAppMode SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
		SetPreferredAppMode(ForceDark);
		FreeLibrary(hUxtheme);
	}
}

void DarkFixScrollBar()
{
	HMODULE hComctl = LoadLibraryExW(L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hComctl)
	{
		auto addr = FindDelayLoadThunkInModule(hComctl, "uxtheme.dll", 49); // OpenNcThemeData
		if (addr)
		{
			DWORD oldProtect;
			if (VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect))
			{
				auto MyOpenThemeData = [](HWND hWnd, LPCWSTR classList) -> HTHEME
				{
					if (wcscmp(classList, L"ScrollBar") == 0)
					{
						hWnd = nullptr;
						classList = L"Explorer::ScrollBar";
					}
					return _OpenNcThemeData(hWnd, classList);
				};
				addr->u1.Function = reinterpret_cast<ULONG_PTR>(static_cast<fnOpenNcThemeData>(MyOpenThemeData));
				VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
			}
		}
		FreeLibrary(hComctl);
	}
}

// Draws white arrow for dark toolbar dropdown button
void DarkToolbarDrawDropDownArrow(HDC hdc, int x, int y)
{
	RECT rc = { x, y,  x + 7, y + 1 };
	FillRect(hdc, &rc, WHITE_BRUSH);
	rc = { x + 1, y + 1,  x + 6, y + 2 };
	FillRect(hdc, &rc, WHITE_BRUSH);
	rc = { x + 2, y + 2,  x + 5, y + 3 };
	FillRect(hdc, &rc, WHITE_BRUSH);
	rc = { x + 3, y + 3,  x + 4, y + 4 };
	FillRect(hdc, &rc, WHITE_BRUSH);
}

void DarkToolbarTooltips(HWND hWnd_Toolbar)
{
	HWND hWnd_Tooltips = (HWND)SendMessage(hWnd_Toolbar, TB_GETTOOLTIPS, 0, 0);
	if (hWnd_Tooltips)
		SetWindowTheme(hWnd_Tooltips, L"DarkMode_Explorer", NULL);
}

LRESULT DarkToolbarCustomDraw(LPNMTBCUSTOMDRAW nmtb)
{
	if (nmtb->nmcd.dwDrawStage == CDDS_PREPAINT)
	{
		FillRect(nmtb->nmcd.hdc, &nmtb->nmcd.rc, BG_BRUSH_DARK);
		return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTERASE | TBCDRF_USECDCOLORS;
	}

	else if (nmtb->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
	{
		// make button rect 1px smaller
//	                if nmcd.lItemlParam not in self.__dropdown_button_ids:
//	                    nmcd.rc.left += 1
//	                    nmcd.rc.right -= 1
//	                nmcd.rc.top += 2
//	                nmcd.rc.bottom -= 2

		if (nmtb->nmcd.uItemState & CDIS_CHECKED)
		{
			// checked button state

			nmtb->nmcd.rc.bottom -= 1;

			// border
			FillRect(nmtb->nmcd.hdc, &nmtb->nmcd.rc, DARK_TOOLBAR_BUTTON_BORDER_BRUSH);

			// make 1px smaller
			nmtb->nmcd.rc.left += 1;
			nmtb->nmcd.rc.right -= 1;
			nmtb->nmcd.rc.top += 1;
			nmtb->nmcd.rc.bottom -= 1;

			FillRect(nmtb->nmcd.hdc, &nmtb->nmcd.rc, DARK_TOOLBAR_BUTTON_BG_BRUSH);

			//	                    if nmtb->nmcd.lItemlParam in self.__dropdown_button_ids and not nmcd.lItemlParam in self.__wholedropdown_button_ids:
			//	                        return CDRF_NOTIFYPOSTPAINT | TBCDRF_NOBACKGROUND | TBCDRF_NOOFFSET | TBCDRF_NOETCHEDEFFECT | TBCDRF_NOEDGES

			return TBCDRF_NOBACKGROUND | TBCDRF_NOOFFSET | TBCDRF_NOETCHEDEFFECT | TBCDRF_NOEDGES;
		}
		else if (nmtb->nmcd.uItemState & CDIS_HOT)
		{
			// hot (rollover) button state

			//nmtb->nmcd.rc.top -= 1;
			nmtb->nmcd.rc.bottom -= 1;

			// border
			FillRect(nmtb->nmcd.hdc, &nmtb->nmcd.rc, DARK_TOOLBAR_BUTTON_BORDER_BRUSH);

			nmtb->nmcd.rc.left += 1;
			nmtb->nmcd.rc.right -= 1;
			nmtb->nmcd.rc.top += 1;
			nmtb->nmcd.rc.bottom -= 1;

			nmtb->clrHighlightHotTrack = DARK_TOOLBAR_BUTTON_ROLLOVER_BG_COLOR;

			//	                    if nmcd.lItemlParam in self.__dropdown_button_ids:
						//if (nmtb->nmcd.dwItemSpec < 3)
			if (nmtb->nmcd.dwItemSpec == TID_CREATE_CON || nmtb->nmcd.dwItemSpec == TID_ACTIVE_NUMBER)
				return CDRF_NOTIFYPOSTPAINT | TBCDRF_NOOFFSET | TBCDRF_NOETCHEDEFFECT | TBCDRF_NOEDGES | TBCDRF_HILITEHOTTRACK; // #| TBCDRF_USECDCOLORS

			return TBCDRF_HILITEHOTTRACK | TBCDRF_NOOFFSET | TBCDRF_NOETCHEDEFFECT | TBCDRF_NOEDGES;
		}
		else if (nmtb->nmcd.uItemState & CDIS_DISABLED)
		{
			// disabled button state
			return TBCDRF_BLENDICON;
		}
		else
		{

			nmtb->nmcd.rc.top -= 2;
			nmtb->nmcd.rc.bottom -= 2;

			// default button state
//	                    if nmcd.lItemlParam in self.__dropdown_button_ids:
			if (nmtb->nmcd.dwItemSpec == TID_CREATE_CON || nmtb->nmcd.dwItemSpec == TID_ACTIVE_NUMBER)
				return CDRF_NOTIFYPOSTPAINT;
			return CDRF_DODEFAULT;
		}
	}

	else if (nmtb->nmcd.dwDrawStage == CDDS_ITEMPOSTPAINT)
	{
		//	                if (nmtb->nmcd.uItemState & CDIS_HOT || nmtb->nmcd.uItemState & CDIS_CHECKED)
		//	                {
		////	                    if nmcd.lItemlParam in self.__wholedropdown_button_ids:
		//						//if (nmtb->nmcd.dwItemSpec < 3)
		//	                        DrawDropDownArrow(nmtb->nmcd.hdc, nmtb->nmcd.rc.left + 20, nmtb->nmcd.rc.top + 3);
		//
		////	                    else:
		//
		//						//RECT rc = { nmtb->nmcd.rc.left + 21, nmtb->nmcd.rc.top - 4, nmtb->nmcd.rc.left + 22, nmtb->nmcd.rc.bottom + 4 };
		//	     //               FillRect(nmtb->nmcd.hdc, &rc, DARK_TOOLBAR_BUTTON_BORDER_BRUSH);
		//	     //               DrawDropDownArrow(nmtb->nmcd.hdc, nmtb->nmcd.rc.left + 25, nmtb->nmcd.rc.top + 4);
		//	                }
		//	                else
		//	                {
		////	                    if nmcd.lItemlParam in self.__wholedropdown_button_ids:
		//							//if (nmtb->nmcd.dwItemSpec < 3)
		//	                        DrawDropDownArrow(nmtb->nmcd.hdc, nmtb->nmcd.rc.left + 23 - 3, nmtb->nmcd.rc.top + 9 - 5 - 1);
		//
		////	                    else:
		//	                        //DrawDropDownArrow(nmtb->nmcd.hdc, nmtb->nmcd.rc.left + 26, nmtb->nmcd.rc.top + 5);
		//	                }
		DarkToolbarDrawDropDownArrow(nmtb->nmcd.hdc, nmtb->nmcd.rc.left + 20, nmtb->nmcd.rc.top + 3 + (nmtb->nmcd.uItemState & CDIS_HOT ? 0 : 1));
		return CDRF_SKIPDEFAULT;
	}
	return 0;
}

LRESULT DarkTabCtrlPaint(HWND hwnd, HFONT hfont, bool isTabIcons)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, TEXT_COLOR_DARK);
	SelectObject(hdc, hfont);

	// Tabbar background
	FillRect(hdc, &ps.rcPaint, BG_BRUSH_DARK);

	HIMAGELIST himl = isTabIcons ? (HIMAGELIST)SendMessage(hwnd, TCM_GETIMAGELIST, 0, 0) : NULL;

	INT_PTR cnt = SendMessage(hwnd, TCM_GETITEMCOUNT, 0, 0);
	RECT rc;
	WCHAR pszText[64];

	TCITEM tie;
	tie.mask = TCIF_TEXT | TCIF_IMAGE;
	tie.pszText = pszText;
	tie.cchTextMax = 64;

	INT_PTR cursel = SendMessage(hwnd, TCM_GETCURSEL, 0, 0);

	for (INT i = 0; i < cnt; i++)
	{
		SendMessage(hwnd, TCM_GETITEMRECT, i, (LPARAM)&rc);

		rc.top = 0;
		rc.bottom -= 3;

		// Tab background
		FillRect(hdc, &rc, i == cursel ? TAB_SELECTED_BG_BRUSH_DARK : BG_BRUSH_DARK);

		SendMessage(hwnd, TCM_GETITEM, (WPARAM)i, (LPARAM)&tie);

		if (isTabIcons)
		{
			// Tab icon
			ImageList_Draw(himl, tie.iImage, hdc, rc.left + 3, rc.top + 1, ILD_NORMAL);
			rc.left += 24;
		}

		// Tab text
		DrawText(hdc, tie.pszText, -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
	}

	EndPaint(hwnd, &ps);
	return FALSE;
}

LRESULT DarkFindPanelColorEdit(HDC hdc)
{
	SetTextColor(hdc, TEXT_COLOR_DARK);
	SetBkColor(hdc, CONTROL_BG_COLOR_DARK);
	SetDCBrushColor(hdc, CONTROL_BG_COLOR_DARK);
	return (LRESULT)GetStockObject(DC_BRUSH);
}

void DarkRemoveAmpersands(wchar_t* szText)
{
	wchar_t szWinTextClean[MAX_WIN_TEXT_LEN] = L"";
	wchar_t* buffer;
	wchar_t* token = wcstok_s(szText, L"&", &buffer);
	while (token)
	{
		// This should be safe, right?
		if (token > szText && *(token - 1) == L'&')
			wcscat_s(szWinTextClean, L"&");
		wcscat_s(szWinTextClean, token);
		token = wcstok_s(nullptr, L"&", &buffer);
	}
	wcscpy_s(szText, MAX_WIN_TEXT_LEN, szWinTextClean);
}

void DarkDialogInit(HWND hDlg)
{
	// Dark window titlebar
	INT value = 1;
	DwmSetWindowAttribute(hDlg, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));

	HWND hwnd = ::GetTopWindow(hDlg);
	wchar_t lpClassName[64];

	wchar_t szWinText[MAX_WIN_TEXT_LEN];
	LONG style, ex_style, nButtonType;

	while (hwnd)
	{
		style = GetWindowLong(hwnd, GWL_STYLE);
		if (style & WS_VISIBLE)
		{
			GetClassName(hwnd, lpClassName, 64);

			if (wcscmp(lpClassName, WC_BUTTON) == 0)
			{
				SetWindowTheme(hwnd, L"DarkMode_Explorer", NULL);

				nButtonType = style & BS_TYPEMASK;

				if (nButtonType == BS_GROUPBOX || nButtonType == BS_AUTOCHECKBOX || nButtonType == BS_AUTORADIOBUTTON || nButtonType == BS_AUTO3STATE)
				{
					RECT rc;
					GetWindowRect(hwnd, &rc);
					MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);

					//wchar_t szWinText[MAX_WIN_TEXT_LEN];
					GetWindowText(hwnd, szWinText, MAX_WIN_TEXT_LEN);

					// Remove single "&", replace "&&" with "&"
					wchar_t szWinTextClean[MAX_WIN_TEXT_LEN] = L"";
					wchar_t* buffer;
					wchar_t* token = wcstok_s(szWinText, L"&", &buffer);
					while (token)
					{
						// This should be safe, right?
						if (token > szWinText + 1 && *(token - 1) == L'&')
							wcscat_s(szWinTextClean, L"&");
						wcscat_s(szWinTextClean, token);
						token = wcstok_s(nullptr, L"&", &buffer);
					}

					// Add static with original button text and original font - but this one can have a custom color (white) which is set in WM_CTLCOLORSTATIC
					HWND h_Static = CreateWindowEx(
						0, //WS_EX_TRANSPARENT,
						WC_STATIC,
						szWinTextClean,
						WS_CHILD | SS_SIMPLE | WS_VISIBLE,
						nButtonType == BS_GROUPBOX ? rc.left + 9 : rc.left + 16,
						rc.top,
						nButtonType == BS_GROUPBOX ? rc.right - rc.left - 16 : rc.right - rc.left,
						nButtonType == BS_GROUPBOX ? 16 : rc.bottom - rc.top,
						hDlg,
						nullptr,
						g_hInstance,
						nullptr
					);

					SendMessage(h_Static, WM_SETFONT,
						SendMessage(hwnd, WM_GETFONT, 0, 0),
						MAKELPARAM(1, 0));

					// Hide original (black) checkbox/radiobutton text by resizing the control
					if (nButtonType != BS_GROUPBOX)
						SetWindowPos(hwnd, 0, 0, 0, 16, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOMOVE);
				}
			}

			else if (wcscmp(lpClassName, WC_COMBOBOX) == 0)
			{
				SetWindowTheme(hwnd, L"DarkMode_CFD", NULL);

				//SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			}

			else if (wcscmp(lpClassName, WC_LISTVIEW) == 0)
			{
				SetWindowTheme(hwnd, L"DarkMode_Explorer", NULL);

				SendMessage(hwnd, LVM_SETTEXTCOLOR, 0, TEXT_COLOR_DARK);
				SendMessage(hwnd, LVM_SETTEXTBKCOLOR, 0, CONTROL_BG_COLOR_DARK);
				SendMessage(hwnd, LVM_SETBKCOLOR, 0, CONTROL_BG_COLOR_DARK);

				// Unfortunately I couldn't find a way to invert the colors of a SysHeader32,
				// it's always black on white. But without theming removed it looks slightly
				// better inside a dark mode ListView.
				HWND h_header = (HWND)SendMessage(hwnd, LVM_GETHEADER, 0, 0);
				if (h_header)
				{
					SetWindowTheme(h_header, L"", L"");
				}
			}

			else if (wcscmp(lpClassName, WC_EDIT) == 0)
			{
				SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE & ~WS_EX_STATICEDGE);
				SetWindowLong(hwnd, GWL_STYLE, style | WS_BORDER);

				SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			}

			else if (wcscmp(lpClassName, WC_STATIC) == 0)
			{
				ex_style = GetWindowLong(hwnd, GWL_EXSTYLE);
				if (ex_style & WS_EX_STATICEDGE || ex_style & WS_EX_CLIENTEDGE)
				{
					SetWindowLong(hwnd, GWL_EXSTYLE, ex_style & ~WS_EX_STATICEDGE & ~WS_EX_CLIENTEDGE);
					SetWindowLong(hwnd, GWL_STYLE, style | WS_BORDER);

					//SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
				}
			}

			else if (wcscmp(lpClassName, WC_LISTBOX) == 0)
			{
				ex_style = GetWindowLong(hwnd, GWL_EXSTYLE);
				if (ex_style & WS_EX_CLIENTEDGE)
				{
					SetWindowLong(hwnd, GWL_EXSTYLE, ex_style & ~WS_EX_CLIENTEDGE);
					SetWindowLong(hwnd, GWL_STYLE, style | WS_BORDER);

					//SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
				}
			}

			else if (wcscmp(lpClassName, WC_TREEVIEW) == 0)
			{
				SetWindowTheme(hwnd, L"DarkMode_Explorer", NULL);

				//SetWindowLongPtrA(hwnd, GWL_EXSTYLE, GetWindowLongPtrA(hwnd, GWL_EXSTYLE) & ~WS_EX_NOPARENTNOTIFY);
				SendMessage(hwnd, TVM_SETBKCOLOR, 0, (LPARAM)CONTROL_BG_COLOR_DARK);
				SendMessage(hwnd, TVM_SETTEXTCOLOR, 0, (LPARAM)TEXT_COLOR_DARK);
				//TreeView_SetTextColor(hwnd, TEXT_COLOR_DARK);

				//SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			}

			//else if (wcscmp(lpClassName, HOTKEY_CLASS) == 0)
			//{
			//	SetWindowTheme(hwnd, L"DarkMode_Explorer", NULL);
			//	//SetWindowTheme(hwnd, L"", L"");
			//}

//			else if (wcscmp(lpClassName, WC_TABCONTROL) == 0)
//			{
//				//SetWindowTheme(hwnd, L"DarkMode_Explorer", NULL);
//				SetWindowTheme(hwnd, L"", L"");
//			}

//			else if (wcscmp(lpClassName, TRACKBAR_CLASS) == 0)
//			{
//				SetWindowTheme(hwnd, L"DarkMode_Explorer", NULL);
//				//SetWindowTheme(hwnd, L"", L"");
//			}

			//else OutputDebugString(lpClassName);

			// Still missing control classes:
			// - HOTKEY_CLASS: unfortunately there seems to be no way to style it for dark mode, it's always black on white.
			// - TRACKBAR_CLASS: styled via WM_CTLCOLORSTATIC and NM_CUSTOMDRAW
		}

		hwnd = ::GetNextWindow(hwnd, GW_HWNDNEXT);
	}

	//	SetWindowPos(hDlg, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

LRESULT DarkOnCtlColorDlg(HDC hdc)
{
	SetBkColor(hdc, BG_COLOR_DARK);
	return (INT_PTR)BG_BRUSH_DARK;
}

LRESULT DarkOnCtlColorStatic(HWND hWnd, HDC hdc)
{
	wchar_t lpClassName[64];
	GetClassName(hWnd, lpClassName, 64);
	if (wcscmp(lpClassName, WC_STATIC) == 0)
	{
		LONG style = GetWindowLong(hWnd, GWL_STYLE);
		if (style & WS_DISABLED)
		{
			// Couldn't find a better way to make disabled static controls look decent in dark mode
			// than enabling them, but changing the text color to grey.
			SetWindowLong(hWnd, GWL_STYLE, style & ~WS_DISABLED);
			SetTextColor(hdc, TEXT_COLOR_DARK_DISABLED);
		}
		else
			SetTextColor(hdc, style & SS_NOTIFY ? TEXT_COLOR_DARK_LINK : TEXT_COLOR_DARK);
	}
	else
		SetTextColor(hdc, TEXT_COLOR_DARK);
	SetBkColor(hdc, BG_COLOR_DARK);
	return (INT_PTR)BG_BRUSH_DARK;
}

LRESULT DarkOnCtlColorBtn(HDC hdc)
{
	SetDCBrushColor(hdc, BG_COLOR_DARK);
	return (INT_PTR)GetStockObject(DC_BRUSH);
}

LRESULT DarkOnCtlColorEditColorListBox(HDC hdc)
{
	SetTextColor(hdc, TEXT_COLOR_DARK);
	SetBkColor(hdc, CONTROL_BG_COLOR_DARK);
	SetDCBrushColor(hdc, CONTROL_BG_COLOR_DARK);
	return (INT_PTR)GetStockObject(DC_BRUSH);
}

void DarkAboutOnDrawItem(DRAWITEMSTRUCT* dis)
{
	SelectClipRgn(dis->hDC, NULL);

	dis->rcItem.left -= 2;
	dis->rcItem.top -= 2;

	dis->rcItem.right += 4;
	dis->rcItem.bottom += 2;

	SetBkMode(dis->hDC, TRANSPARENT);

	SetTextColor(dis->hDC, TEXT_COLOR_DARK);

	HFONT hfont = (HFONT)SendMessage(dis->hwndItem, WM_GETFONT, 0, 0);
	SelectObject(dis->hDC, hfont);

	wchar_t pszText[64] = L"";
	TCITEM tie;
	tie.mask = TCIF_TEXT;
	tie.pszText = pszText;
	tie.cchTextMax = 64;
	TabCtrl_GetItem(dis->hwndItem, dis->itemID, &tie);

	FillRect(dis->hDC, &dis->rcItem, dis->itemState & ODS_SELECTED ? TAB_SELECTED_BG_BRUSH_DARK : CONTROL_BG_BRUSH_DARK);

	DrawText(dis->hDC, tie.pszText, -1, &dis->rcItem, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	ExcludeClipRect(dis->hDC,
		dis->rcItem.left,
		dis->rcItem.top - 2,
		dis->rcItem.right,
		dis->rcItem.bottom + 2
	);
}
