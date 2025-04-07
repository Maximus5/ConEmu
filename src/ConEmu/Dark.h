#ifndef DARK_H
#define DARK_H

#include <windows.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include "Header.h"

#define DBGI(x) {char dbg[32];sprintf(dbg,"%ld",x);OutputDebugStringA(dbg);}

#define MAX_WIN_TEXT_LEN 256

#define DWMWA_USE_IMMERSIVE_DARK_MODE 20

static COLORREF TEXT_COLOR_DARK = 0xe0e0e0;
static COLORREF TEXT_COLOR_DARK_DISABLED = 0x646464;
static COLORREF TEXT_COLOR_DARK_LINK = 0xE9BD5B;

static HBRUSH BRUSH_DARK_DISABLED = CreateSolidBrush(TEXT_COLOR_DARK_DISABLED);

static COLORREF BG_COLOR_DARK = 0x202020;
static HBRUSH BG_BRUSH_DARK = CreateSolidBrush(BG_COLOR_DARK);

static COLORREF CONTROL_BG_COLOR_DARK = 0x333333;
static HBRUSH CONTROL_BG_BRUSH_DARK = CreateSolidBrush(CONTROL_BG_COLOR_DARK);

static HBRUSH TAB_SELECTED_BG_BRUSH_DARK = CreateSolidBrush(0x424242);

static HBRUSH DARK_TOOLBAR_BUTTON_BORDER_BRUSH = CreateSolidBrush(0x636363);
static HBRUSH DARK_TOOLBAR_BUTTON_BG_BRUSH = CreateSolidBrush(0x424242);
//
static COLORREF DARK_TOOLBAR_BUTTON_ROLLOVER_BG_COLOR = 0x434343;
static HBRUSH DARK_TOOLBAR_BUTTON_ROLLOVER_BG_BRUSH = CreateSolidBrush(DARK_TOOLBAR_BUTTON_ROLLOVER_BG_COLOR);

enum PreferredAppMode
{
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
};

using fnOpenNcThemeData = HTHEME(WINAPI*)(HWND hWnd, LPCWSTR pszClassList);
using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode);
//using fnFlushMenuThemes = void (WINAPI*)();

void DarkModeInit(HWND hWnd);
void DarkFixScrollBar();
void DarkToolbarDrawDropDownArrow(HDC hdc, int x, int y);
void DarkToolbarTooltips(HWND hWnd_Toolbar);
LRESULT DarkToolbarCustomDraw(LPNMTBCUSTOMDRAW nmtb);
LRESULT DarkFindPanelColorEdit(HDC hdc);
LRESULT DarkTabCtrlPaint(HWND hwnd, HFONT hfont, bool isTabIcons);
void DarkDialogInit(HWND hDlg);
LRESULT DarkOnCtlColorDlg(HDC hdc);
LRESULT DarkOnCtlColorStatic(HWND hWnd, HDC hdc);
LRESULT DarkOnCtlColorBtn(HDC hdc);
LRESULT DarkOnCtlColorEditColorListBox(HDC hdc);
void DarkAboutOnDrawItem(DRAWITEMSTRUCT* dis);

#endif
