
/*
Copyright (c) 2009-2013 Maximus5
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


// cbFARuseASCIIsort, cbFixAltOnAltTab

#undef USE_PALETTE_POPUP_COLORS

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"
#include "version.h"

#include "../ConEmuCD/GuiHooks.h"
#include "../ConEmuPlugin/FarDefaultMacros.h"
#include "Background.h"
#include "ConEmu.h"
#include "Inside.h"
#include "LoadImg.h"
#include "Macro.h"
#include "Options.h"
#include "OptionsFast.h"
#include "RealConsole.h"
#include "Status.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "VConChild.h"
#include "VirtualConsole.h"


//#define DEBUGSTRFONT(s) DEBUGSTR(s)

//#define COUNTER_REFRESH 5000
//#define BACKGROUND_FILE_POLL 5000

//#define RASTER_FONTS_NAME L"Raster Fonts"
//SIZE szRasterSizes[100] = {{0,0}}; // {{16,8},{6,9},{8,9},{5,12},{7,12},{8,12},{16,12},{12,16},{10,18}};
//const wchar_t szRasterAutoError[] = L"Font auto size is not allowed for a fixed raster font size. Select 'Terminal' instead of '[Raster Fonts ...]'";

//#define TEST_FONT_WIDTH_STRING_EN L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//#define TEST_FONT_WIDTH_STRING_RU L"АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"

//#define FAILED_FONT_TIMERID 101
//#define FAILED_FONT_TIMEOUT 3000
//#define FAILED_CONFONT_TIMEOUT 30000

//const u8 chSetsNums[] = {0, 178, 186, 136, 1, 238, 134, 161, 177, 129, 130, 77, 255, 204, 128, 2, 222, 162, 163};
//const char *ChSets[] = {"ANSI", "Arabic", "Baltic", "Chinese Big 5", "Default", "East Europe",
//                        "GB 2312", "Greek", "Hebrew", "Hangul", "Johab", "Mac", "OEM", "Russian", "Shiftjis",
//                        "Symbol", "Thai", "Turkish", "Vietnamese"
//                       };
//const WORD HostkeyCtrlIds[] = {cbHostWin, cbHostApps, cbHostLCtrl, cbHostRCtrl, cbHostLAlt, cbHostRAlt, cbHostLShift, cbHostRShift};
//int upToFontHeight=0;
//HWND ghOpWnd=NULL;

#ifdef _DEBUG
#define HEAPVAL //HeapValidate(GetProcessHeap(), 0, NULL);
#else
#define HEAPVAL
#endif

#define DEFAULT_FADE_LOW 0
#define DEFAULT_FADE_HIGH 0xD0

#define MAX_SPLITTER_SIZE 32


struct CONEMUDEFCOLORS
{
	const wchar_t* pszTitle;
	DWORD dwDefColors[0x10];
	BYTE  nIndexes[4]; // Text/Back/PopupText/PopupBack
	struct {
	bool  isBackground;
	DWORD clrBackground;
	} Bk;

	bool isIndexes() const { return (nIndexes[0] && nIndexes[1] && nIndexes[2] && nIndexes[3]); };
};

const CONEMUDEFCOLORS DefColors[] =
{
	{
		L"<Default color scheme (Windows standard)>", {
			0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
			0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
		}
	},
	{
		L"<Gamma 1 (for use with dark monitors)>", {
			0x00000000, 0x00960000, 0x0000aa00, 0x00aaaa00, 0x000000aa, 0x00800080, 0x0000aaaa, 0x00c0c0c0,
			0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
		}
	},
	{
		L"<PowerShell>", {
			0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00562401, 0x00F0EDEE, 0x00C0C0C0,
			0x00808080, 0x00ff0000, 0x0000FF00, 0x00FFFF00, 0x000000FF, 0x00FF00FF, 0x0000FFFF, 0x00FFFFFF
		}, {6,5,3,15}
	},
	{
		L"<Murena scheme>", {
			0x00000000, 0x00644100, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
			0x00808080, 0x00ff0000, 0x0076c587, 0x00ffff00, 0x00004bff, 0x00d78ce6, 0x0000ffff, 0x00ffffff
		}
	},
	{
		L"<Solarized>", {
			0x00423607, 0x002f32dc, 0x00009985, 0x000089b5, 0x00d28b26, 0x008236d3, 0x0098a12a, 0x00d5e8ee,
			0x00362b00, 0x00164bcb, 0x00756e58, 0x00837b65, 0x00969483, 0x00c4716c, 0x00a1a193, 0x00e3f6fd
		}
	},
	{
		L"<Solarized (Luke Maciak)>", {
			0x00423607, 0x00d28b26, 0x00009985, 0x000089b5, 0x002f32dc, 0x008236d3, 0x0098a12a, 0x00d5e8ee,
			0x00362b00, 0x00aaa897, 0x00756e58, 0x00837b65, 0x00004ff2, 0x00c4716c, 0x00a1a193, 0x00e3f6fd
		}
	},
	{
		L"<Solarized (John Doe)>", {
			0x00362b00, 0x00423607, 0x00756e58, 0x00837b65, 0x002f32dc, 0x00c4716c, 0x00164bcb, 0x00d5e8ee,
			0x00a1a193, 0x00d28b26, 0x00009985, 0x0098a12a, 0x00969483, 0x008236d3, 0x000089b5, 0x00e3f6fd		
		}
	},
	{
		L"<Solarized Light>", {
			0x00E3F6FD, 0x00D5E8EE, 0x00756e58, 0x00969483, 0x002f32dc, 0x00c4716c, 0x00164bcb, 0x00423607,
			0x00586E75, 0x00d28b26, 0x00009985, 0x0098a12a, 0x00837B65, 0x008236d3, 0x000089b5, 0x00362B00
		}
	},
	{
		L"<Solarized Me>", {
			0x00E3F6FD, 0x00D5E8EE, 0x00756E58, 0x008C8A77, 0x00164BCB, 0x00C4716C, 0x002F32DC, 0x00586E75,
			0x00423607, 0x00D28B26, 0x0000BB7E, 0x0098A12A, 0x00837B65, 0x008236D3, 0x000089B5, 0x00362B00
		}
	},
	{
		L"<Standard VGA>", {
			0x00000000, 0x00aa0000, 0x0000aa00, 0x00aaaa00, 0x000000aa, 0x00aa00aa, 0x000055aa, 0x00aaaaaa,
			0x00555555, 0x00ff5555, 0x0055ff55, 0x00ffff55, 0x005555ff, 0x00ff55ff, 0x0055ffff, 0x00ffffff
		}
	},
	{
		L"<tc-maxx>", {
			0x00000000, RGB(11,27,59), RGB(0,128,0), RGB(0,90,135), RGB(106,7,28), RGB(128,0,128), RGB(128,128,0), RGB(40,150,177),
			RGB(128,128,128), RGB(0,0,255), RGB(0,255,0), RGB(0,215,243), RGB(190,7,23), RGB(255,0,255), RGB(255,255,0), RGB(255,255,255)
		}
	},
	{
		L"<Terminal.app>", {
			0x00000000, 0x00e12e49, 0x0024bc25, 0x00c8bb33, 0x002136c2, 0x00d338d3, 0x0027adad, 0x00cdcccb,
			0x00838381, 0x00ff3358, 0x0022e731, 0x00f0f014, 0x001f39fc, 0x00f835f9, 0x0023ecea, 0x00ebebe9
		}
	},
	{
		L"<xterm>", {
			0x00000000, 0x00ee0000, 0x0000cd00, 0x00cdcd00, 0x000000cd, 0x00cd00cd, 0x0000cdcd, 0x00e5e5e5,
			0x007f7f7f, 0x00ff5c5c, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
		}
	},
	{
		L"<Twilight>", {
			0x00141414, 0x004c6acf, 0x00619083, 0x0069a8ce, 0x00a68775, 0x009d859b, 0x00b3a605, 0x00d7d7d7,
			0x00666666, 0x00a68775, 0x004da459, 0x00e6e60a, 0x00520ad8, 0x008b00e6, 0x003eeee1, 0x00e6e6e6
		}
	},
	{
		L"<Zenburn>", {
			0x003f3f3f, 0x00af6464, 0x00008000, 0x00808000, 0x00232333, 0x00aa50aa, 0x0000dcdc, 0x00ccdcdc,
			0x008080c0, 0x00ffafaf, 0x007f9f7f, 0x00d3d08c, 0x007071e3, 0x00c880c8, 0x00afdff0, 0x00ffffff
		},
	},
	{
		L"<Ubuntu>", { // Need to set up backround "picture" with colorfill #300A24
			0x0036342e, 0x00a46534, 0x00069a4e, 0x009a9806, 0x000000cc, 0x007b5075, 0x0000a0c4, 0x00cfd7d3,
			0x00535755, 0x00cf9f72, 0x0034e28a, 0x00e2e234, 0x002929ef, 0x00a87fad, 0x004fe9fc, 0x00eceeee			
		}, {15, 0, 0, 0}, {true, 0x00240A30}
	},
};
//const DWORD *dwDefColors = DefColors[0].dwDefColors;
//DWORD gdwLastColors[0x10] = {0};
//BOOL  gbLastColorsOk = FALSE;

//#define MAX_COLOR_EDT_ID c31




#define SetThumbColor(s,rgb,idx,us) { (s).RawColor = 0; (s).ColorRGB = rgb; (s).ColorIdx = idx; (s).UseIndex = us; }
#define SetThumbSize(s,sz,x1,y1,x2,y2,ls,lp,fn,fs) { \
		(s).nImgSize = sz; (s).nSpaceX1 = x1; (s).nSpaceY1 = y1; (s).nSpaceX2 = x2; (s).nSpaceY2 = y2; \
		(s).nLabelSpacing = ls; (s).nLabelPadding = lp; wcscpy_c((s).sFontName,fn); (s).nFontHeight=fs; }
#define ThumbLoadSet(s,n) { \
		reg->Load(L"PanView." s L".ImgSize", n.nImgSize); \
		reg->Load(L"PanView." s L".SpaceX1", n.nSpaceX1); \
		reg->Load(L"PanView." s L".SpaceY1", n.nSpaceY1); \
		reg->Load(L"PanView." s L".SpaceX2", n.nSpaceX2); \
		reg->Load(L"PanView." s L".SpaceY2", n.nSpaceY2); \
		reg->Load(L"PanView." s L".LabelSpacing", n.nLabelSpacing); \
		reg->Load(L"PanView." s L".LabelPadding", n.nLabelPadding); \
		reg->Load(L"PanView." s L".FontName", n.sFontName, countof(n.sFontName)); \
		reg->Load(L"PanView." s L".FontHeight", n.nFontHeight); }
#define ThumbSaveSet(s,n) { \
		reg->Save(L"PanView." s L".ImgSize", n.nImgSize); \
		reg->Save(L"PanView." s L".SpaceX1", n.nSpaceX1); \
		reg->Save(L"PanView." s L".SpaceY1", n.nSpaceY1); \
		reg->Save(L"PanView." s L".SpaceX2", n.nSpaceX2); \
		reg->Save(L"PanView." s L".SpaceY2", n.nSpaceY2); \
		reg->Save(L"PanView." s L".LabelSpacing", n.nLabelSpacing); \
		reg->Save(L"PanView." s L".LabelPadding", n.nLabelPadding); \
		reg->Save(L"PanView." s L".FontName", n.sFontName); \
		reg->Save(L"PanView." s L".FontHeight", n.nFontHeight); }




Settings::Settings()
{
	//gpSet = this; // -- нельзя. Settings может использоваться как копия настроек (!= gpSet)
	
	ResetSettings();

	// Умолчания устанавливаются в CSettings::CSettings
	//-- // Теперь установим умолчания настроек	
	//-- InitSettings();
}

void Settings::ResetSettings()
{
	#ifdef OPTION_TYPES_USED
	PRAGMA_ERROR("memset not allowed for classes!");

	#else

	// Сброс переменных (struct, допустимо)
	memset(this, 0, sizeof(*this));

	#endif
}

void Settings::ReleasePointers()
{
	SafeFree(sTabCloseMacro);
	
	SafeFree(sSafeFarCloseMacro);

	SafeFree(sSaveAllMacro);

	SafeFree(sRClickMacro);
	
	SafeFree(psStartSingleApp);
	SafeFree(psStartTasksFile);
	SafeFree(psStartTasksName);
	//SafeFree(psCurCmd);
	SafeFree(psCmdHistory); isSaveCmdHistory = true;
	SafeFree(psDefaultTerminalApps);

	FreeCmdTasks();
	CmdTaskCount = 0;
	
	FreePalettes();
	PaletteCount = 0;

	FreeApps();
	AppStd.FreeApps();

	FreeProgresses();
	ProgressesCount = 0;
	
	UpdSet.FreePointers();
}

Settings::~Settings()
{
	ReleasePointers();
}

void Settings::InitSettings()
{
	if (gpConEmu) gpConEmu->LogString(L"Settings::InitSettings()");
	MCHKHEAP

	// Освободить память, т.к. функция может быть вызвана из окна интерфейса настроек	
	ReleasePointers();
	
	// Сброс переменных
	ResetSettings();
	
//------------------------------------------------------------------------
///| Moved from CVirtualConsole |/////////////////////////////////////////
//------------------------------------------------------------------------
	isAutoRegisterFonts = true;
	nHostkeyNumberModifier = VK_LWIN; //TestHostkeyModifiers(nHostkeyNumberModifier);
	nHostkeyArrowModifier = VK_LWIN; //TestHostkeyModifiers(nHostkeyArrowModifier);
	isSingleInstance = false;
	isShowHelpTooltips = true;
	isMulti = true;
	isMultiShowButtons = true;
	isNumberInCaption = false;
	//vmMultiNew = 'W' | (nMultiHotkeyModifier << 8);
	//vmMultiNext = 'Q' | (nMultiHotkeyModifier << 8);
	//vmMultiRecreate = 192/*VK_тильда*/ | (nMultiHotkeyModifier << 8);
	//vmMultiBuffer = 'A' | (nMultiHotkeyModifier << 8);
	//vmMinimizeRestore = 'C' | (nMultiHotkeyModifier << 8);
	//vmMultiClose = VK_DELETE | (nMultiHotkeyModifier << 8);
	//vmMultiCmd = 'X' | (nMultiHotkeyModifier << 8);
	isMultiAutoCreate = false;
	isMultiLeaveOnClose = 0;
	isMultiHideOnClose = 0;
	isMultiIterate = true;
	isMultiMinByEsc = 2; isMapShiftEscToEsc = true; // isMapShiftEscToEsc used only when isMultiMinByEsc==1 and only for console apps
	isMultiNewConfirm = true;
	isUseWinNumber = true; isUseWinArrows = false; isUseWinTab = false;
	nSplitWidth = nSplitHeight = 4;
	//nSplitClr1 = nSplitClr2 = RGB(160,160,160);
	m_isKeyboardHooks = 0;
	isFARuseASCIIsort = false; isFixAltOnAltTab = false; isShellNoZoneCheck = false;
	isFadeInactive = true; mn_FadeLow = DEFAULT_FADE_LOW; mn_FadeHigh = DEFAULT_FADE_HIGH; mb_FadeInitialized = false;
	mn_LastFadeSrc = mn_LastFadeDst = -1;
	nMainTimerElapse = 10;
	nMainTimerInactiveElapse = 1000;
	nAffinity = 0; // 0 - don't change default affinity
	isSkipFocusEvents = false;
	//isSendAltEnter = isSendAltSpace = isSendAltF9 = false;
	isSendAltTab = isSendAltEsc = isSendAltPrintScrn = isSendPrintScrn = isSendCtrlEsc = false;
	isMonitorConsoleLang = 3;
	DefaultBufferHeight = 1000; AutoBufferHeight = true;
	isSaveCmdHistory = true;
	nCmdOutputCP = 0;
	ComSpec.AddConEmu2Path = CEAP_AddAll;
	{
		ComSpec.isAllowUncPaths = FALSE;
		// Load defaults from windows registry (Command Processor settings)
		SettingsRegistry UncChk;
		if (UncChk.OpenKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor", KEY_READ))
		{
			DWORD DisableUNCCheck = 0;
			if (UncChk.Load(L"DisableUNCCheck", (LPBYTE)&DisableUNCCheck, sizeof(DisableUNCCheck)))
			{
				ComSpec.isAllowUncPaths = (DisableUNCCheck == 1);
			}
		}
	}

	bool bIsDbcs = (GetSystemMetrics(SM_DBCSENABLED) != 0);

	//WARNING("InitSettings() может вызываться из интерфейса настройки, не промахнуться с хэндлами");
	//// Шрифты
	//memset(m_Fonts, 0, sizeof(m_Fonts));
	////TODO: OLD - на переделку
	//memset(&LogFont, 0, sizeof(LogFont));
	//memset(&LogFont2, 0, sizeof(LogFont2));
	/*LogFont.lfHeight = mn_FontHeight =*/
	FontSizeY = 16;
	//-- Issue 577: Для иероглифов - сделаем "пошире", а то глифы в консоль не влезут...
	//-- пошире не будем. DBCS консоль хитрая, на каждый иероглиф отводится 2 ячейки
	//-- "не влезть" может только если выполнить "chcp 65001", что врядли, а у если надо - руками пусть ставят
	FontSizeX3 = 0; // bIsDbcs ? 15 : 0;
	//LogFont.lfWidth = mn_FontWidth = FontSizeX = mn_BorderFontWidth = 0;
	//FontSizeX2 = 0; FontSizeX3 = 0;
	//LogFont.lfEscapement = LogFont.lfOrientation = 0;
	/*LogFont.lfWeight = FW_NORMAL;*/ isBold = false;
	/*LogFont.lfItalic = LogFont.lfUnderline = LogFont.lfStrikeOut = FALSE;*/ isItalic = false;
	/*LogFont.lfCharSet*/ mn_LoadFontCharSet = DEFAULT_CHARSET;
	//LogFont.lfOutPrecision = OUT_TT_PRECIS;
	//LogFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	/*LogFont.lfQuality =*/ mn_AntiAlias = ANTIALIASED_QUALITY;
	//LogFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	inFont[0] = inFont2[0] = 0;
	//wcscpy_c(inFont, gsLucidaConsole);
	//wcscpy_c(inFont2, gsLucidaConsole);
	//mb_Name1Ok = FALSE; mb_Name2Ok = FALSE;
	isTryToCenter = false;
	nCenterConsolePad = 0;
	isAlwaysShowScrollbar = 2;
	nScrollBarAppearDelay = 100;
	nScrollBarDisappearDelay = 1000;
	//isTabFrame = true;
	//isForceMonospace = false; isProportional = false;
	
	//Issue 577: Для иероглифов - по умолчанию отключим моноширность
	isMonospace = bIsDbcs ? 0 : 1;

	mb_MinToTray = false; isAlwaysShowTrayIcon = false;
	//memset(&rcTabMargins, 0, sizeof(rcTabMargins));
	//isFontAutoSize = false; mn_AutoFontWidth = mn_AutoFontHeight = -1;
	
	ConsoleFont.lfHeight = 5;
	ConsoleFont.lfWidth = 3;
	wcscpy_c(ConsoleFont.lfFaceName, gsDefConFont);
	
	{
		SettingsRegistry RegConColors, RegConDef;

		if (RegConColors.OpenKey(L"Console", KEY_READ))
		{
			RegConDef.OpenKey(HKEY_USERS, L".DEFAULT\\Console", KEY_READ);
			TCHAR ColorName[] = L"ColorTable00";
			bool  lbBlackFound = false;

			for (uint i = 0x10; i--;)
			{
				// L"ColorTableNN"
				ColorName[10] = i/10 + '0';
				ColorName[11] = i%10 + '0';

				if (!RegConColors.Load(ColorName, (LPBYTE)&(Colors[i]), sizeof(Colors[0])))
					if (!RegConDef.Load(ColorName, (LPBYTE)&(Colors[i]), sizeof(Colors[0])))
						Colors[i] = DefColors[0].dwDefColors[i]; //-V108

				if (Colors[i] == 0)
				{
					if (!lbBlackFound)
						lbBlackFound = true;
					else if (lbBlackFound)
						Colors[i] = DefColors[0].dwDefColors[i]; //-V108
				}

				Colors[i+0x10] = Colors[i]; // Умолчания
			}

			RegConDef.CloseKey();
			RegConColors.CloseKey();
		}
	}

	isTrueColorer = true; // включим по умочанию, ибо Far3

	AppStd.isExtendColors = false;
	AppStd.nExtendColorIdx = 14;
	AppStd.nTextColorIdx = AppStd.nBackColorIdx = 16; // Auto
	AppStd.nPopTextColorIdx = AppStd.nPopBackColorIdx = 16; // Auto
	AppStd.isExtendFonts = false;
	AppStd.nFontNormalColor = 1; AppStd.nFontBoldColor = 12; AppStd.nFontItalicColor = 13;
	{
		_ASSERTE(sizeof(AppStd.CursorActive) == sizeof(DWORD));
		AppStd.CursorActive.Raw = 0; // Сброс
		AppStd.CursorActive.CursorType = cur_Vert; // 0 - Horz, 1 - Vert, 2 - Hollow-block
		AppStd.CursorActive.isBlinking = true;
		AppStd.CursorActive.isColor = true;
		//AppStd.CursorActive.isCursorBlockInactive = true;
		AppStd.CursorActive.isFixedSize = false;
		AppStd.CursorActive.FixedSize = 25; // в процентах
		AppStd.CursorActive.MinSize = 2; // в пикселях

		AppStd.CursorInactive.Raw = AppStd.CursorActive.Raw; // копирование
		AppStd.CursorInactive.Used = true;
		AppStd.CursorInactive.CursorType = cur_Rect;
		AppStd.CursorInactive.isBlinking = false;
	}
	AppStd.isCTSDetectLineEnd = true;
	AppStd.isCTSBashMargin = false;
	AppStd.isCTSTrimTrailing = 2;
	AppStd.isCTSEOL = 0;
	AppStd.isCTSShiftArrowStart = true;
	AppStd.isPasteAllLines = true;
	AppStd.isPasteFirstLine = true;
	
	// 0 - off, 1 - force, 2 - try to detect "ReadConsole" (don't use 2 in bash)
	AppStd.isCTSClickPromptPosition = 2; // Кликом мышки позиционировать курсор в Cmd Prompt (cmd.exe, Powershell.exe, ...) + vkCTSVkPromptClk
	AppStd.isCTSDeleteLeftWord = 2; // Ctrl+BS - удалять слово слева от курсора

	// пока не учитывается
	AppStd.isShowBgImage = 0;
	AppStd.sBgImage[0] = 0;
	AppStd.nBgOperation = eUpLeft;

	//CheckTheming(); -- сейчас - нельзя. нужно дождаться, пока главное окно будет создано
	//mb_ThemingEnabled = (gOSVer.dwMajorVersion >= 6 || (gOSVer.dwMajorVersion == 5 && gOSVer.dwMinorVersion >= 1));
//------------------------------------------------------------------------
///| Default settings |///////////////////////////////////////////////////
//------------------------------------------------------------------------
	isShowBgImage = 0;
	wcscpy_c(sBgImage, L"c:\\back.bmp");
	bgImageDarker = 255; // 0x46;
	//nBgImageColors = 1|2; синий,зеленый
	//nBgImageColors = 2; // только синий
	nBgImageColors = (DWORD)-1; // Получить цвет панелей из фара - иначе "1|2" == BgImageColorsDefaults.
	bgOperation = eUpLeft;
	isBgPluginAllowed = 1;
	nTransparent = nTransparentInactive = 255;
	isTransparentSeparate = false;
	//isColorKey = false;
	//ColorKey = RGB(1,1,1);
	isUserScreenTransparent = false;
	isColorKeyTransparent = false;
	nColorKeyValue = RGB(1,1,1);
	isFixFarBorders = 1; isEnhanceGraphics = true; isEnhanceButtons = false;
	isPartBrush75 = 0xC8; isPartBrush50 = 0x96; isPartBrush25 = 0x5A;
	isPartBrushBlack = 32; //-V112
	isExtendUCharMap = true;
	isDownShowHiddenMessage = false;
	ParseCharRanges(L"2013-25C4", mpc_FixFarBorderValues);
	wndWidth.Set(true, ss_Standard, 80);
	wndHeight.Set(false, ss_Standard, 25);
	ntvdmHeight = 0; // Подбирать автоматически
	mb_IntegralSize = false;
	_WindowMode = rNormal;
	isUseCurrentSizePos = true; // Show in settings dialog and save current window size/pos
	//isFullScreen = false;
	isHideCaption = false; mb_HideCaptionAlways = false; isQuakeStyle = false; nQuakeAnimation = QUAKEANIMATION_DEF;
	isHideChildCaption = true;
	isFocusInChildWindows = true;
	nHideCaptionAlwaysFrame = 255; nHideCaptionAlwaysDelay = 2000; nHideCaptionAlwaysDisappear = 2000;
	isDesktopMode = false;
	isSnapToDesktopEdges = false;
	isAlwaysOnTop = false;
	isSleepInBackground = false; // по умолчанию - не включать "засыпание в фоне".
	mb_MinimizeOnLoseFocus = false; // не "прятаться" при потере фокуса
	RECT rcWork = {}; SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
	if (gbIsWine)
	{
		_wndX = max(90,rcWork.left); _wndY = max(90,rcWork.top);
	}
	else
	{
		_wndX = rcWork.left; _wndY = rcWork.top;
	}
	wndCascade = true;
	isAutoSaveSizePos = false; mb_SizePosAutoSaved = false;
	isConVisible = false; //isLockRealConsolePos = false;
	isUseInjects = true; // Fuck... Features or speed. User must choose!

	isSetDefaultTerminal = false;
	isRegisterOnOsStartup = false;
	isDefaultTerminalNoInjects = false;
	nDefaultTerminalConfirmClose = 1 /* Always */;
	SetDefaultTerminalApps(L"explorer.exe"/* to default value */); // "|"-delimited string -> MSZ

	isProcessAnsi = true;
	isSuppressBells = false; // пока не доделано - false
	isConsoleExceptionHandler = false; // по умолчанию - false
	mb_UseClink = true;
	#ifdef USEPORTABLEREGISTRY
	isPortableReg = true; // включено по умолчанию, DEBUG
	#else
	isPortableReg = false;
	#endif
	nConInMode = (DWORD)-1; // по умолчанию, включится (ENABLE_QUICK_EDIT_MODE|ENABLE_EXTENDED_FLAGS|ENABLE_INSERT_MODE)
	nSlideShowElapse = 2500;
	nIconID = IDI_ICON1;
	isRClickSendKey = 2;
	sRClickMacro = NULL;
	//wcscpy_c(szTabConsole, L"<%c> %s `%p`" /*L "%s [%c]" */);
	//wcscpy_c(szTabEditor, L"<%c.%i>{%s} `%p`" /* L"[%s]" */);
	//wcscpy_c(szTabEditorModified, L"<%c.%i>[%s] `%p` *" /* L"[%s] *" */);
	//wcscpy_c(szTabViewer, L"<%c.%i>[%s] `%p`" /* L"{%s}" */);
	wcscpy_c(szTabConsole, L"<%c> %s");
	//wcscpy_c(szTabSkipWords, L"Administrator:|Администратор:");
	MultiByteToWideChar(1251/*rus*/, 0, "Administrator:|Администратор:", -1, szTabSkipWords, countof(szTabSkipWords));
	wcscpy_c(szTabPanels, szTabConsole); // Раньше была только настройка "TabConsole". Унаследовать ее в "TabPanels"
	wcscpy_c(szTabEditor, L"<%c.%i>{%s}");
	wcscpy_c(szTabEditorModified, L"<%c.%i>[%s] *");
	wcscpy_c(szTabViewer, L"<%c.%i>[%s]");
	nTabLenMax = 20;
	nTabWidthMax = 200;
	nTabStyle = ts_Win8;
	isSafeFarClose = true;
	sSafeFarCloseMacro = NULL; // если NULL - то используется макрос по умолчанию
	isConsoleTextSelection = 1; // Always
	isCTSAutoCopy = true;
	isCTSIBeam = true;
	isCTSEndOnTyping = false;
	isCTSEndOnKeyPress = false;
	isCTSFreezeBeforeSelect = false;
	isCTSSelectBlock = true; //isCTSVkBlock = VK_LMENU; // по умолчанию - блок выделяется c LAlt
	isCTSSelectText = true; //isCTSVkText = VK_LSHIFT; // а текст - при нажатом LShift
	//vmCTSVkBlockStart = 0; // при желании, пользователь может назначить hotkey запуска выделения
	//vmCTSVkTextStart = 0;  // при желании, пользователь может назначить hotkey запуска выделения
	isCTSActMode = 2; // BufferOnly
	//isCTSVkAct = 0; // т.к. по умолчанию - только BufferOnly, то вообще без модификаторов
	isCTSRBtnAction = 3; // Auto (Выделения нет - Paste, Есть - Copy)
	isCTSMBtnAction = 0; // <None>
	isCTSColorIndex = 0xE0;
	isPasteConfirmEnter = true;
	nPasteConfirmLonger = 200;
	isFarGotoEditor = true; //isFarGotoEditorVk = VK_LCONTROL;
	sFarGotoEditor = lstrdup(L"far.exe /e%1:%2 \"%3\"");

	isStatusBarShow = true;
	isStatusBarFlags = csf_HorzDelim;
	nStatusBarBack = RGB(64,64,64);
	nStatusBarLight = RGB(255,255,255);
	nStatusBarDark = RGB(160,160,160);
	wcscpy_c(sStatusFontFace, L"Tahoma"); nStatusFontCharSet = ANSI_CHARSET; nStatusFontHeight = 14;
	//nHideStatusColumns = ces_CursorInfo;
	_ASSERTE(countof(isStatusColumnHidden)>csi_Last);
	memset(isStatusColumnHidden, 0, sizeof(isStatusColumnHidden));
	// выставим те колонки, которые не нужны "юзеру" по умолчанию
	isStatusColumnHidden[csi_ConsoleTitle] = true;
	isStatusColumnHidden[csi_InputLocale] = true;
	isStatusColumnHidden[csi_WindowPos] = true;
	isStatusColumnHidden[csi_WindowSize] = true;
	isStatusColumnHidden[csi_WindowClient] = true;
	isStatusColumnHidden[csi_WindowWork] = true;
	isStatusColumnHidden[csi_WindowStyle] = true;
	isStatusColumnHidden[csi_WindowStyleEx] = true;
	isStatusColumnHidden[csi_HwndFore] = true;
	isStatusColumnHidden[csi_HwndFocus] = true;
	isStatusColumnHidden[csi_ConEmuPID] = true;
	isStatusColumnHidden[csi_CursorInfo] = true;
	isStatusColumnHidden[csi_ConEmuHWND] = true;
	isStatusColumnHidden[csi_ConEmuView] = true;
	isStatusColumnHidden[csi_ServerHWND] = true;

	isTabs = 1; nTabsLocation = 0; isTabIcons = true; isOneTabPerGroup = false;
	isActivateSplitMouseOver = false;
	isTabSelf = true; isTabRecent = true; isTabLazy = true;
	nTabBarDblClickAction = TABBAR_DEFAULT_CLICK_ACTION; nTabBtnDblClickAction = TABBTN_DEFAULT_CLICK_ACTION;
	ilDragHeight = 10;
	m_isTabsOnTaskBar = 2;
	isTaskbarShield = true;

	isTabsInCaption = false; //cbTabsInCaption
	#if defined(CONEMU_TABBAR_EX)
	isTabsInCaption = true;
	#endif

	wcscpy_c(sTabFontFace, L"Tahoma"); nTabFontCharSet = ANSI_CHARSET; nTabFontHeight = 16;
	sTabCloseMacro = sSaveAllMacro = NULL;
	nToolbarAddSpace = 0;
	wcscpy_c(szAdminTitleSuffix, L" (Admin)");
	bAdminShield = true;
	bHideInactiveConsoleTabs = false;
	bHideDisabledTabs = false;
	bShowFarWindows = true;
	isDisableMouse = false;
	isRSelFix = true; isMouseSkipActivation = true; isMouseSkipMoving = true;
	isFarHourglass = true; nFarHourglassDelay = 500;
	isDisableFarFlashing = false; isDisableAllFlashing = false;
	isDragEnabled = DRAG_L_ALLOWED; isDropEnabled = (BYTE)1; isDefCopy = true; isDropUseMenu = 2;
	//nLDragKey = 0; nRDragKey = VK_LCONTROL;
	isDragOverlay = 1; isDragShowIcons = true;
	// изменение размера панелей мышкой
	isDragPanel = 2; // по умолчанию сделаем чтобы драгалось макросами (вдруг у юзера на Ctrl-Left/Right/Up/Down макросы висят... как бы конфуза не получилось)
	isDragPanelBothEdges = false; // таскать за обе рамки (правую-левой панели и левую-правой панели)
	isKeyBarRClick = true;
	isDebugSteps = true;
	MCHKHEAP
	FindOptions.bMatchCase = false;
	FindOptions.bMatchWholeWords = false;
	FindOptions.bFreezeConsole = true;
	FindOptions.bHighlightAll = true;
	FindOptions.bTransparent = true;
	// Thumbnails
	memset(&ThSet, 0, sizeof(ThSet));
	ThSet.cbSize = sizeof(ThSet);
	// фон превьюшки: RGB или Index
	SetThumbColor(ThSet.crBackground, RGB(255,255,255), 16, TRUE); // Поставим по умолчанию "Auto"
	// серая рамка вокруг превьюшки
	ThSet.nPreviewFrame = 1;
	SetThumbColor(ThSet.crPreviewFrame, RGB(128,128,128), 8, TRUE);
	// рамка вокруг текущего элемента
	ThSet.nSelectFrame = 1;
	SetThumbColor(ThSet.crSelectFrame, RGB(192,192,192), 7, FALSE);
	/* теперь разнообразные размеры */
	// отступы для preview
	SetThumbSize(ThSet.Thumbs,96,1,1,5,20,2,0,L"Tahoma",14);
	// отступы для tiles
	SetThumbSize(ThSet.Tiles,48,4,4,172,4,4,1,L"Tahoma",14); //-V112
	// Прочие параметры загрузки
	ThSet.bLoadPreviews = 3;   // bitmask of {1=Thumbs, 2=Tiles}
	ThSet.bLoadFolders = true; // true - load infolder previews (only for Thumbs)
	ThSet.nLoadTimeout = 15;   // 15 sec
	ThSet.nMaxZoom = 600; // %%
	ThSet.bUsePicView2 = true;
	ThSet.bRestoreOnStartup = false;
	//// Пока не используется
	//DWORD nCacheFolderType; // юзер/программа/temp/и т.п.
	//wchar_t sCacheFolder[MAX_PATH];

	/* *** AutoUpdate *** */
	_ASSERTE(UpdSet.szUpdateVerLocation==NULL); // Уже должен был быть вызван ReleasePointers
	UpdSet.ResetToDefaults();
}

// В Desktop, Inside (и еще может быть когда) Transparent включать нельзя
bool Settings::isTransparentAllowed()
{
	// Окно работает в "Child" режиме, прозрачность не допускается
	if (isDesktopMode || gpConEmu->mp_Inside)
		return false;

	// Чтобы не рушилось отображение картинки плагинами
	if (gpConEmu->isPictureView())
		return false;

	// Можно, по настройкам
	return true;
}

// true - не допускать Gaps в Normal режиме. Подгонять размер окна точно под консоль.
// false - размер окна - свободный (попиксельно)
bool Settings::isIntegralSize()
{
	if (mb_IntegralSize)
		return false;

	if (isQuakeStyle || gpConEmu->mp_Inside)
		return false;

	#if 0
	if ((1 & (WORD)GetKeyState(VK_NUMLOCK)) == 0)
		return false;
	#endif

	return true;
}

void Settings::FreeApps(int NewAppCount, AppSettings** NewApps/*, Settings::CEAppColors** NewAppColors*/)
{
	int OldAppCount = this->AppCount;
	this->AppCount = NewAppCount;
	AppSettings** OldApps = Apps; Apps = NewApps;
	//CEAppColors** OldAppColors = AppColors; AppColors = NewAppColors;
	for (int i = 0; i < OldAppCount && OldApps; i++)
	{
		if (OldApps[i])
		{
			OldApps[i]->FreeApps();
			SafeFree(OldApps[i]);
		}
		//SafeFree(OldAppColors[i]);
	}
	SafeFree(OldApps);
	//SafeFree(OldAppColors);
}

void Settings::LoadAppSettings(SettingsBase* reg, bool abFromOpDlg /*= false*/)
{
	bool lbDelete = false;
	if (!reg)
	{
		// Если открыто окно настроек - не передергивать
		if (!abFromOpDlg && ghOpWnd)
		{
			return;
		}

		reg = CreateSettings(NULL);
		if (!reg)
		{
			_ASSERTE(reg!=NULL);
			return;
		}
		lbDelete = true;
	}

	BOOL lbOpened = FALSE;
	wchar_t szAppKey[MAX_PATH+64];
	wcscpy_c(szAppKey, gpSetCls->GetConfigPath());
	wcscat_c(szAppKey, L"\\Apps");
	wchar_t* pszAppKey = szAppKey+lstrlen(szAppKey);

	int NewAppCount = 0;
	AppSettings** NewApps = NULL;
	//CEAppColors** NewAppColors = NULL;
	
	lbOpened = reg->OpenKey(szAppKey, KEY_READ);
	if (lbOpened)
	{
		reg->Load(L"Count", NewAppCount);
		reg->CloseKey();
	}

	if (lbOpened && NewAppCount > 0)
	{
		NewApps = (AppSettings**)calloc(NewAppCount, sizeof(*NewApps));
		//NewAppColors = (CEAppColors**)calloc(NewAppCount, sizeof(*NewAppColors));

		int nSucceeded = 0;
		for (int i = 0; i < NewAppCount; i++)
		{
			_wsprintf(pszAppKey, SKIPLEN(32) L"\\App%i", i+1);

			lbOpened = reg->OpenKey(szAppKey, KEY_READ);
			if (lbOpened)
			{
				_ASSERTE(AppStd.AppNames == NULL && AppStd.AppNamesLwr == NULL);

				NewApps[nSucceeded] = (AppSettings*)malloc(sizeof(AppSettings));
				//NewAppColors[nSucceeded] = (CEAppColors*)calloc(1,sizeof(CEAppColors));

				// Умолчания берем из основной ветки!
				*NewApps[nSucceeded] = AppStd;
				//memmove(NewAppColors[nSucceeded]->Colors, Colors, sizeof(Colors));
				NewApps[nSucceeded]->AppNames = NULL;
				NewApps[nSucceeded]->AppNamesLwr = NULL;
				NewApps[nSucceeded]->cchNameMax = 0;

				// Загрузка "AppNames" - снаружи, т.к. LoadAppSettings используется и для загрузки &AppStd
				reg->Load(L"AppNames", &NewApps[nSucceeded]->AppNames);
				if (NewApps[nSucceeded]->AppNames /*&& *NewApps[nSucceeded]->AppNames*/)
				{
					NewApps[nSucceeded]->cchNameMax = wcslen(NewApps[nSucceeded]->AppNames)+1;
					NewApps[nSucceeded]->AppNamesLwr = lstrdup(NewApps[nSucceeded]->AppNames);
					CharLowerBuff(NewApps[nSucceeded]->AppNamesLwr, lstrlen(NewApps[nSucceeded]->AppNamesLwr));
					reg->Load(L"Elevated", NewApps[nSucceeded]->Elevated);
					LoadAppSettings(reg, NewApps[nSucceeded]/*, NewAppColors[nSucceeded]->Colors*/);
					nSucceeded++;
				}
				reg->CloseKey();
			}
		}
		NewAppCount = nSucceeded;
	}

	FreeApps(NewAppCount, NewApps/*, NewAppColors*/);

	if (lbDelete)
		delete reg;
}

void Settings::LoadCursorSettings(SettingsBase* reg, CECursorType* pActive, CECursorType* pInactive)
{
	#define _MinMax(a,mn,mx) if (a < mn) a = mn; else if (a > mx) a = mx;

	// Is there new style was saved?
	bool bActive = reg->Load(L"CursorTypeActive", pActive->Raw);
	DEBUGTEST(bool bInactive = false);

	if (bActive)
	{
		_MinMax(pActive->CursorType, cur_First, cur_Last);
		_MinMax(pActive->FixedSize, CURSORSIZE_MIN, CURSORSIZE_MAX);
		_MinMax(pActive->MinSize, CURSORSIZEPIX_MIN, CURSORSIZEPIX_MAX);

		DEBUGTEST(bInactive = )
		reg->Load(L"CursorTypeInactive", pInactive->Raw);

		_MinMax(pInactive->CursorType, cur_First, cur_Last);
		_MinMax(pInactive->FixedSize, CURSORSIZE_MIN, CURSORSIZE_MAX);
		_MinMax(pInactive->MinSize, CURSORSIZEPIX_MIN, CURSORSIZEPIX_MAX);
	}
	else
	{
		// Need to convert old settings style

		BYTE isCursorType = cur_Vert; // 0 - Horz, 1 - Vert, 2 - Hollow-block
		bool isCursorBlink = true;
		bool isCursorColor = true;
		bool isCursorBlockInactive = true;
		bool isCursorIgnoreSize = false;
		BYTE nCursorFixedSize = 25; // в процентах
		BYTE nCursorMinSize = 2; // в пикселях

		reg->Load(L"CursorType", isCursorType);
			_MinMax(isCursorType, cur_First, cur_Last);
		reg->Load(L"CursorColor", isCursorColor);
		reg->Load(L"CursorBlink", isCursorBlink);
		reg->Load(L"CursorBlockInactive", isCursorBlockInactive);
		reg->Load(L"CursorIgnoreSize", isCursorIgnoreSize);
		reg->Load(L"CursorFixedSize", nCursorFixedSize);
			_MinMax(nCursorFixedSize, CURSORSIZE_MIN, CURSORSIZE_MAX);
		reg->Load(L"CursorMinSize", nCursorMinSize);
			_MinMax(nCursorMinSize, CURSORSIZEPIX_MIN, CURSORSIZEPIX_MAX);

		pActive->Raw = 0; // Сброс
		pActive->CursorType = (CECursorStyle)isCursorType; // 0 - Horz, 1 - Vert, 2 - Hollow-block
		pActive->isBlinking = isCursorBlink;
		pActive->isColor = isCursorColor;
		//AppStd.CursorActive.isCursorBlockInactive = true;
		pActive->isFixedSize = isCursorIgnoreSize;
		pActive->FixedSize = nCursorFixedSize; // в процентах
		pActive->MinSize = nCursorMinSize; // в пикселях

		pInactive->Raw = AppStd.CursorActive.Raw; // копирование
		pInactive->Used = isCursorBlockInactive;
		pInactive->CursorType = cur_Rect;
		pInactive->isBlinking = false;
	}
}

void Settings::LoadAppSettings(SettingsBase* reg, Settings::AppSettings* pApp/*, COLORREF* pColors*/)
{
	// Для AppStd данные загружаются из основной ветки! В том числе и цвета (RGB[32] а не имя палитры)
	bool bStd = (pApp == &AppStd);

	pApp->OverridePalette = bStd;
	if (bStd)
	{
		TCHAR ColorName[] = L"ColorTable00";
		for (size_t i = countof(Colors)/*0x20*/; i--;)
		{
			ColorName[10] = i/10 + '0';
			ColorName[11] = i%10 + '0';
			reg->Load(ColorName, Colors[i]);
		}

		reg->Load(L"ExtendColors", pApp->isExtendColors);
		reg->Load(L"ExtendColorIdx", pApp->nExtendColorIdx);
		if (pApp->nExtendColorIdx > 15) pApp->nExtendColorIdx=14;

		reg->Load(L"TextColorIdx", pApp->nTextColorIdx);
		if (pApp->nTextColorIdx > 16) pApp->nTextColorIdx=16;
		reg->Load(L"BackColorIdx", pApp->nBackColorIdx);
		if (pApp->nBackColorIdx > 16) pApp->nBackColorIdx=16;
		reg->Load(L"PopTextColorIdx", pApp->nPopTextColorIdx);
		if (pApp->nPopTextColorIdx > 16) pApp->nPopTextColorIdx=16;
		reg->Load(L"PopBackColorIdx", pApp->nPopBackColorIdx);
		if (pApp->nPopBackColorIdx > 16) pApp->nPopBackColorIdx=16;
	}
	else
	{
		reg->Load(L"OverridePalette", pApp->OverridePalette);
		if (!reg->Load(L"PaletteName", pApp->szPaletteName, countof(pApp->szPaletteName)))
			pApp->szPaletteName[0] = 0;
		pApp->ResetPaletteIndex();
		const Settings::ColorPalette* pPal = PaletteGet(pApp->GetPaletteIndex());

		_ASSERTE(pPal!=NULL); // NULL не может быть. Всегда как минимум - стандартная палитра
		pApp->isExtendColors = pPal->isExtendColors;
		pApp->nExtendColorIdx = pPal->nExtendColorIdx;

		pApp->nTextColorIdx = pPal->nTextColorIdx;
		pApp->nBackColorIdx = pPal->nBackColorIdx;
		pApp->nPopTextColorIdx = pPal->nPopTextColorIdx;
		pApp->nPopBackColorIdx = pPal->nPopBackColorIdx;

		//memmove(pColors, pPal->Colors, sizeof(pPal->Colors));
	}

	pApp->OverrideExtendFonts = bStd;
	if (!bStd)
		reg->Load(L"OverrideExtendFonts", pApp->OverrideExtendFonts);
	reg->Load(L"ExtendFonts", pApp->isExtendFonts);
	reg->Load(L"ExtendFontNormalIdx", pApp->nFontNormalColor);
	if (pApp->nFontNormalColor>15 && pApp->nFontNormalColor!=255) pApp->nFontNormalColor=1;
	reg->Load(L"ExtendFontBoldIdx", pApp->nFontBoldColor);
	if (pApp->nFontBoldColor>15 && pApp->nFontBoldColor!=255) pApp->nFontBoldColor=12;
	reg->Load(L"ExtendFontItalicIdx", pApp->nFontItalicColor);
	if (pApp->nFontItalicColor>15 && pApp->nFontItalicColor!=255) pApp->nFontItalicColor=13;

	pApp->OverrideCursor = bStd;
	if (!bStd)
		reg->Load(L"OverrideCursor", pApp->OverrideCursor);
	LoadCursorSettings(reg, &pApp->CursorActive, &pApp->CursorInactive);

	pApp->OverrideClipboard = bStd;
	if (!bStd)
		reg->Load(L"OverrideClipboard", pApp->OverrideClipboard);
	reg->Load(L"ClipboardDetectLineEnd", pApp->isCTSDetectLineEnd);
	reg->Load(L"ClipboardBashMargin", pApp->isCTSBashMargin);
	reg->Load(L"ClipboardTrimTrailing", pApp->isCTSTrimTrailing);
	reg->Load(L"ClipboardEOL", pApp->isCTSEOL); MinMax(pApp->isCTSEOL,2);
	reg->Load(L"ClipboardArrowStart", pApp->isCTSShiftArrowStart);
	reg->Load(L"ClipboardAllLines", pApp->isPasteAllLines);
	reg->Load(L"ClipboardFirstLine", pApp->isPasteFirstLine);
	reg->Load(L"ClipboardClickPromptPosition", pApp->isCTSClickPromptPosition); MinMax(pApp->isCTSClickPromptPosition,2);
	reg->Load(L"ClipboardDeleteLeftWord", pApp->isCTSDeleteLeftWord); MinMax(pApp->isCTSDeleteLeftWord,2);
}

void Settings::FreeCmdTasks()
{
	if (CmdTasks)
	{
		for (int i = 0; i < CmdTaskCount; i++)
		{
			if (CmdTasks[i])
				CmdTasks[i]->FreePtr();
		}
		SafeFree(CmdTasks);
	}

	if (StartupTask)
	{
		StartupTask->FreePtr();
		SafeFree(StartupTask);
	}
}

void Settings::LoadCmdTasks(SettingsBase* reg, bool abFromOpDlg /*= false*/)
{
	bool lbDelete = false;
	if (!reg)
	{
		// Если открыто окно настроек - не передергивать
		if (!abFromOpDlg && ghOpWnd)
		{
			return;
		}

		reg = CreateSettings(NULL);
		if (!reg)
		{
			_ASSERTE(reg!=NULL);
			return;
		}
		lbDelete = true;
	}

	FreeCmdTasks();

	BOOL lbOpened = FALSE;
	wchar_t szCmdKey[MAX_PATH+64];
	wcscpy_c(szCmdKey, gpSetCls->GetConfigPath());
	wcscat_c(szCmdKey, L"\\Tasks");
	wchar_t* pszCmdKey = szCmdKey+lstrlen(szCmdKey);

	int NewTasksCount = 0;
	
	lbOpened = reg->OpenKey(szCmdKey, KEY_READ);
	if (lbOpened)
	{
		reg->Load(L"Count", NewTasksCount);
		reg->CloseKey();
	}

	if (lbOpened && NewTasksCount > 0)
	{
		CmdTasks = (CommandTasks**)calloc(NewTasksCount, sizeof(CommandTasks*));

		int nSucceeded = 0;
		for (int i = 0; i < NewTasksCount; i++)
		{
			_wsprintf(pszCmdKey, SKIPLEN(32) L"\\Task%i", i+1); // 1-based

			lbOpened = reg->OpenKey(szCmdKey, KEY_READ);
			if (lbOpened)
			{
				if (LoadCmdTask(reg, CmdTasks[i], i))
					nSucceeded++;

				reg->CloseKey();
			}
		}
		CmdTaskCount = nSucceeded;
	}

	if (lbDelete)
		delete reg;
}

bool Settings::LoadCmdTask(SettingsBase* reg, CommandTasks* &pTask, int iIndex)
{
	bool lbRc = false;
	wchar_t szVal[32];
	int iCmdMax = 0, iCmdCount = 0;

	wchar_t* pszNameSet = NULL;
	if (!reg->Load(L"Name", &pszNameSet) || !*pszNameSet)
	{
		SafeFree(pszNameSet);
		goto wrap;
	}

	pTask = (CommandTasks*)calloc(1, sizeof(CommandTasks));
	if (!pTask)
	{
		SafeFree(pszNameSet);
		goto wrap;
	}

	pTask->SetName(pszNameSet, iIndex);

	if (!reg->Load(L"GuiArgs", &pTask->pszGuiArgs) || !*pTask->pszGuiArgs)
	{
		pTask->cchGuiArgMax = 0;
		SafeFree(pTask->pszGuiArgs);
	}
	else
	{
		pTask->cchGuiArgMax = _tcslen(pTask->pszGuiArgs)+1;
	}

	lbRc = true;

	if (reg->Load(L"Count", iCmdMax) && (iCmdMax > 0))
	{
		size_t nTotalLen = 1024; // с запасом, для редактирования через интерфейс
		wchar_t** pszCommands = (wchar_t**)calloc(iCmdMax, sizeof(wchar_t*));

		for (int j = 0; j < iCmdMax; j++)
		{
			_wsprintf(szVal, SKIPLEN(countof(szVal)) L"Cmd%i", j+1); // 1-based

			if (reg->Load(szVal, &(pszCommands[j])) && pszCommands[j] && *pszCommands[j])
			{
				iCmdCount++;
				nTotalLen += _tcslen(pszCommands[j])+8; // + ">\r\n\r\n"
			}
			else
				SafeFree(pszCommands[j]);
		}

		if ((iCmdCount > 0) && (nTotalLen))
		{
			pTask->cchCmdMax = nTotalLen+1;
			pTask->pszCommands = (wchar_t*)malloc(pTask->cchCmdMax*sizeof(wchar_t));
			if (pTask->pszCommands)
			{
				//pTask->nCommands = iCmdCount;

				int nActive = 0;
				reg->Load(L"Active", nActive); // 1-based

				wchar_t* psz = pTask->pszCommands; // dest script
				for (int k = 0; k < iCmdCount; k++)
				{
					bool bActive = false;
					gpConEmu->ParseScriptLineOptions(pszCommands[k], NULL, &bActive);

					if (((k+1) == nActive) && !bActive)
						*(psz++) = L'>';

					lstrcpy(psz, pszCommands[k]);
					SafeFree(pszCommands[k]);

					if ((k+1) < iCmdCount)
						lstrcat(psz, L"\r\n\r\n"); // для визуальности редактирования

					psz += lstrlen(psz);	
				}
			}
		}

		SafeFree(pszCommands);
	}

wrap:
	return lbRc;
}

bool Settings::SaveCmdTasks(SettingsBase* reg)
{
	bool lbDelete = false;
	if (!reg)
	{
		reg = CreateSettings(NULL);
		if (!reg)
		{
			_ASSERTE(reg!=NULL);
			return false;
		}
		lbDelete = true;
	}

	bool lbRc = false;
	BOOL lbOpened = FALSE;
	wchar_t szCmdKey[MAX_PATH+64];
	wcscpy_c(szCmdKey, gpSetCls->GetConfigPath());
	wcscat_c(szCmdKey, L"\\Tasks");
	wchar_t* pszCmdKey = szCmdKey+lstrlen(szCmdKey);

	lbOpened = reg->OpenKey(szCmdKey, KEY_WRITE);
	if (lbOpened)
	{
		lbRc = true;
		reg->Save(L"Count", CmdTaskCount);
		reg->CloseKey();
	}

	if (lbOpened && CmdTasks && CmdTaskCount > 0)
	{
		//int nSucceeded = 0;
		for (int i = 0; i < CmdTaskCount; i++)
		{
			_wsprintf(pszCmdKey, SKIPLEN(32) L"\\Task%i", i+1); // 1-based

			lbOpened = reg->OpenKey(szCmdKey, KEY_WRITE);
			if (!lbOpened)
			{
				lbRc = false;
			}
			else
			{
				SaveCmdTask(reg, CmdTasks[i]);

				reg->CloseKey();
			}
		}
	}

	if (lbDelete)
		delete reg;

	return lbRc;
}

bool Settings::SaveCmdTask(SettingsBase* reg, CommandTasks* pTask)
{
	bool lbRc = true;
	int iCmdCount = 0;
	int nActive = 0; // 1-based
	wchar_t szVal[32];

	reg->Save(L"Name", pTask->pszName);
	reg->Save(L"GuiArgs", pTask->pszGuiArgs);

	if (pTask->pszCommands)
	{
		wchar_t* pszCmd = pTask->pszCommands;
		while (*pszCmd == L'\r' || *pszCmd == L'\n' || *pszCmd == L'\t' || *pszCmd == L' ')
			pszCmd++;

		while (pszCmd && *pszCmd)
		{
			iCmdCount++;

			wchar_t* pszEnd = wcschr(pszCmd, L'\n');
			if (pszEnd && (*(pszEnd-1) == L'\r'))
				pszEnd--;
			wchar_t chSave = 0;
			if (pszEnd)
			{
				chSave = *pszEnd;
				*pszEnd = 0;
			}

			_wsprintf(szVal, SKIPLEN(countof(szVal)) L"Cmd%i", iCmdCount); // 1-based

			if (*pszCmd == L'>')
			{
				//pszCmd++;
				nActive = iCmdCount; // 1-based
			}

			reg->Save(szVal, pszCmd);

			if (pszEnd)
				*pszEnd = chSave;

			if (!pszEnd)
				break;
			pszCmd = pszEnd+1;
			while (*pszCmd == L'\r' || *pszCmd == L'\n' || *pszCmd == L'\t' || *pszCmd == L' ')
				pszCmd++;
		}

		reg->Save(L"Active", nActive); // 1-based
	}
	
	reg->Save(L"Count", iCmdCount);

	return lbRc;
}

void Settings::SortPalettes()
{
	for (int i = 0; (i+1) < PaletteCount; i++)
	{
		int iMin = i;
		for (int j = (i+1); j < PaletteCount; j++)
		{
			// Don't "sort" predefined palettes at all
			if (Palettes[iMin]->bPredefined /*&& !Palettes[j]->bPredefined*/)
			{
				continue;
			}
			else if (!Palettes[iMin]->bPredefined && Palettes[j]->bPredefined)
			{
				_ASSERTE(!(!Palettes[iMin]->bPredefined && Palettes[j]->bPredefined));
				iMin = j;
			}
			else if (CSTR_GREATER_THAN == CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, Palettes[iMin]->pszName, -1, Palettes[j]->pszName, -1))
			{
				iMin = j;
			}
		}

		if (iMin != i)
		{
			ColorPalette* p = Palettes[i];
			Palettes[i] = Palettes[iMin];
			Palettes[iMin] = p;
		}
	}
}

void Settings::FreePalettes()
{
	if (Palettes)
	{
		for (int i = 0; i < PaletteCount; i++)
		{
			if (Palettes[i])
				Palettes[i]->FreePtr();
		}
		SafeFree(Palettes);
	}
	PaletteCount = 0;
}

void Settings::LoadPalettes(SettingsBase* reg)
{
	TCHAR ColorName[] = L"ColorTable00";

	bool lbDelete = false;
	if (!reg)
	{
		reg = CreateSettings(NULL);
		if (!reg)
		{
			_ASSERTE(reg!=NULL);
			return;
		}
		lbDelete = true;
	}


	FreePalettes();


	BOOL lbOpened = FALSE;
	wchar_t szColorKey[MAX_PATH+64];
	wcscpy_c(szColorKey, gpSetCls->GetConfigPath());
	wcscat_c(szColorKey, L"\\Colors");
	wchar_t* pszColorKey = szColorKey + lstrlen(szColorKey);

	int UserCount = 0;
	lbOpened = reg->OpenKey(szColorKey, KEY_READ);
	if (lbOpened)
	{
		reg->Load(L"Count", UserCount);
		reg->CloseKey();
	}

	
	// Predefined
	Palettes = (ColorPalette**)calloc((UserCount + countof(DefColors)), sizeof(ColorPalette*));
	for (size_t n = 0; n < countof(DefColors); n++)
	{
    	Palettes[n] = (ColorPalette*)calloc(1, sizeof(ColorPalette));
    	_ASSERTE(DefColors[n].pszTitle && DefColors[n].pszTitle[0]==L'<' && DefColors[n].pszTitle[lstrlen(DefColors[n].pszTitle)-1]==L'>');
    	Palettes[n]->pszName = lstrdup(DefColors[n].pszTitle);
    	Palettes[n]->bPredefined = true;
		Palettes[n]->isExtendColors = false;
		Palettes[n]->nExtendColorIdx = 14;
		Palettes[n]->nTextColorIdx = Palettes[n]->nBackColorIdx = 16; // Auto
		Palettes[n]->nPopTextColorIdx = Palettes[n]->nPopBackColorIdx = 16; // Auto
    	_ASSERTE(countof(Palettes[n]->Colors)==0x20 && countof(DefColors[n].dwDefColors)==0x10);
    	memmove(Palettes[n]->Colors, DefColors[n].dwDefColors, 0x10*sizeof(Palettes[n]->Colors[0]));
		if (DefColors[n].isIndexes())
		{
			Palettes[n]->nTextColorIdx = DefColors[n].nIndexes[0];
			Palettes[n]->nBackColorIdx = DefColors[n].nIndexes[1];
			Palettes[n]->nPopTextColorIdx = DefColors[n].nIndexes[2];
			Palettes[n]->nPopBackColorIdx = DefColors[n].nIndexes[3];
		}
		else
		{
			Palettes[n]->nTextColorIdx = Palettes[n]->nBackColorIdx = 16; // Auto
			Palettes[n]->nPopTextColorIdx = Palettes[n]->nPopBackColorIdx = 16; // Auto
		}
    	// Расширения - инициализируем теми же цветами
    	memmove(Palettes[n]->Colors+0x10, DefColors[n].dwDefColors, 0x10*sizeof(Palettes[n]->Colors[0]));
	}
	// Инициализировали "Палитрами по умолчанию"
	PaletteCount = (int)countof(DefColors);

	// User
	for (int i = 0; i < UserCount; i++)
	{
		_wsprintf(pszColorKey, SKIPLEN(32) L"\\Palette%i", i+1); // 1-based

		lbOpened = reg->OpenKey(szColorKey, KEY_READ);
		if (lbOpened)
		{
			Palettes[PaletteCount] = (ColorPalette*)calloc(1, sizeof(ColorPalette));

			reg->Load(L"Name", &Palettes[PaletteCount]->pszName);
			reg->Load(L"ExtendColors", Palettes[PaletteCount]->isExtendColors);
			reg->Load(L"ExtendColorIdx", Palettes[PaletteCount]->nExtendColorIdx);

			reg->Load(L"TextColorIdx", Palettes[PaletteCount]->nTextColorIdx); MinMax(Palettes[PaletteCount]->nTextColorIdx,16);
			reg->Load(L"BackColorIdx", Palettes[PaletteCount]->nBackColorIdx); MinMax(Palettes[PaletteCount]->nBackColorIdx,16);
			reg->Load(L"PopTextColorIdx", Palettes[PaletteCount]->nPopTextColorIdx); MinMax(Palettes[PaletteCount]->nPopTextColorIdx,16);
			reg->Load(L"PopBackColorIdx", Palettes[PaletteCount]->nPopBackColorIdx); MinMax(Palettes[PaletteCount]->nPopBackColorIdx,16);

			_ASSERTE(countof(Colors) == countof(Palettes[PaletteCount]->Colors));
			for (size_t k = 0; k < countof(Palettes[PaletteCount]->Colors)/*0x20*/; k++)
			{
				ColorName[10] = k/10 + '0';
				ColorName[11] = k%10 + '0';
				reg->Load(ColorName, Palettes[PaletteCount]->Colors[k]);
			}

			PaletteCount++;

			reg->CloseKey();
		}
	}

	// sort
	SortPalettes();

	if (lbDelete)
		delete reg;
}

void Settings::SavePalettes(SettingsBase* reg)
{
	TCHAR ColorName[] = L"ColorTable00";

	bool lbDelete = false;
	if (!reg)
	{
		reg = CreateSettings(NULL);
		if (!reg)
		{
			_ASSERTE(reg!=NULL);
			return;
		}
		lbDelete = true;
	}

	// sort
	SortPalettes();

	BOOL lbOpened = FALSE;
	wchar_t szColorKey[MAX_PATH+64];
	wcscpy_c(szColorKey, gpSetCls->GetConfigPath());
	wcscat_c(szColorKey, L"\\Colors");
	wchar_t* pszColorKey = szColorKey + lstrlen(szColorKey);

	int UserCount = 0;

	for (int i = 0; i < PaletteCount; i++)
	{
		if (!Palettes[i] || Palettes[i]->bPredefined)
			continue; // Системные - не сохраняем

		_wsprintf(pszColorKey, SKIPLEN(32) L"\\Palette%i", UserCount+1); // 1-based

		lbOpened = reg->OpenKey(szColorKey, KEY_WRITE);
		if (lbOpened)
		{
			UserCount++;

			reg->Save(L"Name", Palettes[i]->pszName);
			reg->Save(L"ExtendColors", Palettes[i]->isExtendColors);
			reg->Save(L"ExtendColorIdx", Palettes[i]->nExtendColorIdx);

			reg->Save(L"TextColorIdx", Palettes[i]->nTextColorIdx);
			reg->Save(L"BackColorIdx", Palettes[i]->nBackColorIdx);
			reg->Save(L"PopTextColorIdx", Palettes[i]->nPopTextColorIdx);
			reg->Save(L"PopBackColorIdx", Palettes[i]->nPopBackColorIdx);

			_ASSERTE(countof(Colors) == countof(Palettes[i]->Colors));
			for (size_t k = 0; k < countof(Palettes[i]->Colors)/*0x20*/; k++)
			{
				ColorName[10] = k/10 + '0';
				ColorName[11] = k%10 + '0';
				reg->Save(ColorName, Palettes[i]->Colors[k]);
			}

			reg->CloseKey();
		}
	}
	
	*pszColorKey = 0;
	lbOpened = reg->OpenKey(szColorKey, KEY_WRITE);
	if (lbOpened)
	{
		reg->Save(L"Count", UserCount);
		reg->CloseKey();
	}

	for (int k = 0; k < AppCount; k++)
	{
		bool bFound = false;
		for (int i = 0; i < PaletteCount; i++)
		{
			if (Palettes[i]->pszName && (lstrcmpi(Apps[k]->szPaletteName, Palettes[i]->pszName) == 0))
			{
				//memmove(AppColors[k]->Colors, Palettes[i]->Colors, sizeof(Palettes[i]->Colors));
				//AppColors[k]->FadeInitialized = false;
				Apps[k]->isExtendColors = Palettes[i]->isExtendColors;
				Apps[k]->nExtendColorIdx = Palettes[i]->nExtendColorIdx;

				Apps[k]->nTextColorIdx = Palettes[i]->nTextColorIdx;
				Apps[k]->nBackColorIdx = Palettes[i]->nBackColorIdx;
				Apps[k]->nPopTextColorIdx = Palettes[i]->nPopTextColorIdx;
				Apps[k]->nPopBackColorIdx = Palettes[i]->nPopBackColorIdx;

				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			Apps[k]->OverridePalette = false; // нету такой палитры, будем рисовать "основной"
		}
	}

	if (lbDelete)
	{
		delete reg;
	
		gpConEmu->Update(true);
	}
}

// 0-based, index of Palettes
// -1 -- current palette
const Settings::ColorPalette* Settings::PaletteGet(int anIndex)
{
	return PaletteGetPtr(anIndex);
}

// 0-based, index of Palettes
// -1 -- current palette
Settings::ColorPalette* Settings::PaletteGetPtr(int anIndex)
{
	if ((anIndex >= 0) && (anIndex < PaletteCount) && Palettes && Palettes[anIndex])
	{
		return Palettes[anIndex];
	}

	_ASSERTE(anIndex==-1);

	static ColorPalette StdPal = {};
	StdPal.bPredefined = false;
	static wchar_t szCurrentScheme[64] = L"<Current color scheme>";
	StdPal.pszName = szCurrentScheme;
		
	StdPal.isExtendColors = AppStd.isExtendColors;
	StdPal.nExtendColorIdx = AppStd.nExtendColorIdx;
		
	StdPal.nTextColorIdx = AppStd.nTextColorIdx;
	StdPal.nBackColorIdx = AppStd.nBackColorIdx;
	StdPal.nPopTextColorIdx = AppStd.nPopTextColorIdx;
	StdPal.nPopBackColorIdx = AppStd.nPopBackColorIdx;

	_ASSERTE(sizeof(StdPal.Colors) == sizeof(this->Colors));
	memmove(StdPal.Colors, this->Colors, sizeof(StdPal.Colors));
	return &StdPal;
}

void Settings::PaletteSetStdIndexes()
{
	if (!Apps || AppCount < 1)
		return; // Нету никого

	for (int i = 0; i < AppCount; i++)
	{
		int nPalIdx = Apps[i]->GetPaletteIndex();
		if (nPalIdx == -1)
		{
			Apps[i]->nTextColorIdx = AppStd.nTextColorIdx;
			Apps[i]->nBackColorIdx = AppStd.nBackColorIdx;
			Apps[i]->nPopTextColorIdx = AppStd.nPopTextColorIdx;
			Apps[i]->nPopBackColorIdx = AppStd.nPopBackColorIdx;
		}
	}
}

int Settings::AppSettings::GetPaletteIndex()
{
	if (this == NULL) // *AppSettings
	{
		_ASSERTE(this!=NULL);
		return -1;
	}
	return gpSet->PaletteGetIndex(szPaletteName);
}

void Settings::AppSettings::SetPaletteName(LPCWSTR asNewPaletteName)
{
	if (this == NULL)
	{
		_ASSERTE(this!=NULL);
		return;
	}
	lstrcpyn(szPaletteName, asNewPaletteName, countof(szPaletteName));
	ResetPaletteIndex();
}

void Settings::AppSettings::ResetPaletteIndex()
{
	// TODO:
}

// Returns Zero-based palette index, or "-1" when not found
int Settings::PaletteGetIndex(LPCWSTR asName)
{
	if (!Palettes || (PaletteCount < 1) || !asName || !*asName)
		return -1;

	for (int i = 0; i < PaletteCount; i++)
	{
		if (!Palettes[i] || !Palettes[i]->pszName)
			continue;

		if (lstrcmpi(asName, Palettes[i]->pszName) == 0)
			return i;
	}

	return -1;
}

// Returns Zero-based palette index, or "-1" when not found
int Settings::PaletteSetActive(LPCWSTR asName)
{
	int nPalIdx = PaletteGetIndex(asName);

	const Settings::ColorPalette* pPal = (nPalIdx != -1) ? PaletteGet(nPalIdx) : NULL;

	if (pPal)
	{
		uint nCount = countof(pPal->Colors);

		for (uint i = 0; i < nCount; i++)
		{
			Colors[i] = pPal->Colors[i]; //-V108
		}

		AppStd.nTextColorIdx = pPal->nTextColorIdx;
		AppStd.nBackColorIdx = pPal->nBackColorIdx;
		AppStd.nPopTextColorIdx = pPal->nPopTextColorIdx;
		AppStd.nPopBackColorIdx = pPal->nPopBackColorIdx;

		AppStd.nExtendColorIdx = pPal->nExtendColorIdx;
		AppStd.isExtendColors = pPal->isExtendColors;
	}

	return nPalIdx;
}

// Save active colors to named palette
void Settings::PaletteSaveAs(LPCWSTR asName)
{
	// Пользовательские палитры не могут именоваться как "<...>"
	if (!asName || !*asName || wcspbrk(asName, L"<>"))
	{
		DisplayLastError(L"Invalid palette name!\nUser palette name must not contain '<' or '>' symbols.", -1, MB_ICONSTOP, asName ? asName : L"<Null>");
		return;
	}

	int nIndex = PaletteGetIndex(asName);

	// "Предопределенные" палитры перезаписывать нельзя
	if ((nIndex != -1) && Palettes[nIndex]->bPredefined)
		nIndex = -1;

	bool bNewPalette = false;

	if (nIndex == -1)
	{
		// Добавляем новую палитру
		bNewPalette = true;
		nIndex = PaletteCount;

		ColorPalette** ppNew = (ColorPalette**)calloc(nIndex+1,sizeof(ColorPalette*));
		if (!ppNew)
		{
			_ASSERTE(ppNew!=NULL);
			return;
		}
		if ((PaletteCount > 0) && Palettes)
		{
			memmove(ppNew, Palettes, PaletteCount*sizeof(ColorPalette*));
		}
		SafeFree(Palettes);
		Palettes = ppNew;

		PaletteCount = nIndex+1;
	}

	_ASSERTE(Palettes);

	if (!Palettes[nIndex])
	{
		Palettes[nIndex] = (ColorPalette*)calloc(1,sizeof(ColorPalette));
		if (!Palettes[nIndex])
		{
			_ASSERTE(Palettes[nIndex]);
			return;
		}
		_ASSERTE(bNewPalette && "Already must be set");
		bNewPalette = true;
	}


	// Сохранять допускается только пользовательские палитры
	Palettes[nIndex]->bPredefined = false;
	Palettes[nIndex]->pszName = lstrdup(asName);
	Palettes[nIndex]->isExtendColors = AppStd.isExtendColors;
	Palettes[nIndex]->nExtendColorIdx = AppStd.nExtendColorIdx;
	
	BOOL bTextChanged = !bNewPalette && ((Palettes[nIndex]->nTextColorIdx != AppStd.nTextColorIdx) || (Palettes[nIndex]->nBackColorIdx != AppStd.nBackColorIdx));
	BOOL bPopupChanged = !bNewPalette && ((Palettes[nIndex]->nPopTextColorIdx != AppStd.nPopTextColorIdx) || (Palettes[nIndex]->nPopBackColorIdx != AppStd.nPopBackColorIdx));
	Palettes[nIndex]->nTextColorIdx = AppStd.nTextColorIdx;
	Palettes[nIndex]->nBackColorIdx = AppStd.nBackColorIdx;
	Palettes[nIndex]->nPopTextColorIdx = AppStd.nPopTextColorIdx;
	Palettes[nIndex]->nPopBackColorIdx = AppStd.nPopBackColorIdx;

	_ASSERTE(sizeof(Palettes[nIndex]->Colors) == sizeof(this->Colors));
	memmove(Palettes[nIndex]->Colors, this->Colors, sizeof(Palettes[nIndex]->Colors));
	_ASSERTE(nIndex < PaletteCount);

	// Теперь, собственно, пишем настройки
	SavePalettes(NULL);

	// Обновить консоли
	if (bTextChanged || bPopupChanged)
	{
		gpSetCls->UpdateTextColorSettings(bTextChanged, bPopupChanged);
	}
}

// Delete named palette
void Settings::PaletteDelete(LPCWSTR asName)
{
	int nIndex = PaletteGetIndex(asName);

	// "Предопределенные" палитры удалять нельзя
	if ((nIndex == -1) || Palettes[nIndex]->bPredefined)
		return;

	// Грохнуть
	Palettes[nIndex]->FreePtr();
	// Сдвинуть хвост
	for (int i = (nIndex+1); i < PaletteCount; i++)
	{
		Palettes[i-1] = Palettes[i];
	}
	// Уменьшить количество
	if (PaletteCount > 0)
	{
		Palettes[PaletteCount-1] = NULL;
		PaletteCount--;
	}
	
	// Теперь, собственно, пишем настройки
	SavePalettes(NULL);
}

/* ************************************************************************ */
/* ************************************************************************ */
/* ************************************************************************ */
/* ************************************************************************ */

DWORD Settings::ProgressesGetDuration(LPCWSTR asName)
{
	if (!Progresses || !asName || !*asName)
		return 0;

	for (int i = 0; i < ProgressesCount; i++)
	{
		if (Progresses[i] && Progresses[i]->pszName)
		{
			if (lstrcmpi(Progresses[i]->pszName, asName) == 0)
				return Progresses[i]->nDuration;
		}
	}

	return 0;
}

void Settings::ProgressesSetDuration(LPCWSTR asName, DWORD anDuration)
{
	if (!asName || !*asName)
	{
		_ASSERTE(asName && *asName);
		return;
	}

	if (Progresses)
	{
		for (int i = 0; i < ProgressesCount; i++)
		{
			if (Progresses[i] && Progresses[i]->pszName)
			{
				if (lstrcmpi(Progresses[i]->pszName, asName) == 0)
				{
					Progresses[i]->nDuration = anDuration;
					goto done; // Update settings
				}
			}
		}
	}

	// Extend Array
	{
		ConEmuProgressStore** ppNew = (ConEmuProgressStore**)calloc(ProgressesCount+1,sizeof(ConEmuProgressStore*));
		if (!ppNew)
		{
			_ASSERTE(ppNew!=NULL);
			return;
		}
		if ((ProgressesCount > 0) && Progresses)
		{
			memmove(ppNew, Progresses, ProgressesCount*sizeof(ConEmuProgressStore*));
		}
		SafeFree(Progresses);
		Progresses = ppNew;
	}

	// Create new entry
	Progresses[ProgressesCount] = (ConEmuProgressStore*)calloc(1,sizeof(ConEmuProgressStore));
	if (!Progresses[ProgressesCount])
	{
		_ASSERTE(Progresses[ProgressesCount]);
		return;
	}

	Progresses[ProgressesCount]->pszName = lstrdup(asName);
	Progresses[ProgressesCount]->nDuration = anDuration;

	ProgressesCount++;

done:
	// Теперь, собственно, пишем настройки
	SaveProgresses(NULL);
}

void Settings::LoadProgresses(SettingsBase* reg)
{
	bool lbDelete = false;
	if (!reg)
	{
		reg = CreateSettings(NULL);
		if (!reg)
		{
			_ASSERTE(reg!=NULL);
			return;
		}
		lbDelete = true;
	}

	FreeProgresses();

	BOOL lbOpened = FALSE;
	wchar_t szCmdKey[MAX_PATH+64];
	wcscpy_c(szCmdKey, gpSetCls->GetConfigPath());
	wcscat_c(szCmdKey, L"\\Progress");
	wchar_t* pszCmdKey = szCmdKey+lstrlen(szCmdKey);

	int NewProgressesCount = 0;
	
	lbOpened = reg->OpenKey(szCmdKey, KEY_READ);
	if (lbOpened)
	{
		reg->Load(L"Count", NewProgressesCount);
		reg->CloseKey();
	}

	if (lbOpened && NewProgressesCount > 0)
	{
		Progresses = (ConEmuProgressStore**)calloc(NewProgressesCount, sizeof(ConEmuProgressStore*));

		int nSucceeded = 0;
		for (int i = 0; i < NewProgressesCount; i++)
		{
			_wsprintf(pszCmdKey, SKIPLEN(32) L"\\Item%i", i+1); // 1-based

			lbOpened = reg->OpenKey(szCmdKey, KEY_READ);
			if (lbOpened)
			{
				if (LoadProgress(reg, Progresses[i]))
					nSucceeded++;

				reg->CloseKey();
			}
		}
		ProgressesCount = nSucceeded;
	}

	if (lbDelete)
		delete reg;
}

bool Settings::SaveProgresses(SettingsBase* reg)
{
	bool lbDelete = false;
	if (!reg)
	{
		reg = CreateSettings(NULL);
		if (!reg)
		{
			_ASSERTE(reg!=NULL);
			return false;
		}
		lbDelete = true;
	}

	bool lbRc = false;
	BOOL lbOpened = FALSE;
	wchar_t szCmdKey[MAX_PATH+64];
	wcscpy_c(szCmdKey, gpSetCls->GetConfigPath());
	wcscat_c(szCmdKey, L"\\Progress");
	wchar_t* pszCmdKey = szCmdKey+lstrlen(szCmdKey);

	lbOpened = reg->OpenKey(szCmdKey, KEY_WRITE);
	if (lbOpened)
	{
		lbRc = true;
		reg->Save(L"Count", ProgressesCount);
		reg->CloseKey();
	}

	if (lbOpened && Progresses && ProgressesCount > 0)
	{
		//int nSucceeded = 0;
		for (int i = 0; i < ProgressesCount; i++)
		{
			_wsprintf(pszCmdKey, SKIPLEN(32) L"\\Item%i", i+1); // 1-based

			lbOpened = reg->OpenKey(szCmdKey, KEY_WRITE);
			if (!lbOpened)
			{
				lbRc = false;
			}
			else
			{
				SaveProgress(reg, Progresses[i]);

				reg->CloseKey();
			}
		}
	}

	if (lbDelete)
		delete reg;

	return lbRc;
}

bool Settings::LoadProgress(SettingsBase* reg, Settings::ConEmuProgressStore* &pProgress)
{
	bool lbRc = false;
	ConEmuProgressStore* p = NULL;

	wchar_t* pszName = NULL;
	if (!reg->Load(L"Name", &pszName) || !*pszName)
	{
		SafeFree(pszName);
		goto wrap;
	}

	p = (ConEmuProgressStore*)calloc(1, sizeof(ConEmuProgressStore));
	if (!p)
	{
		SafeFree(pszName);
		goto wrap;
	}

	p->pszName = pszName;
	reg->Load(L"Duration", p->nDuration);

wrap:
	return lbRc;
}

bool Settings::SaveProgress(SettingsBase* reg, Settings::ConEmuProgressStore* pProgress)
{
	reg->Save(L"Name", pProgress->pszName);
	reg->Save(L"Duration", pProgress->nDuration);
	return true;
}

void Settings::FreeProgresses()
{
	if (Progresses)
	{
		for (int i = 0; i < ProgressesCount; i++)
		{
			if (Progresses[i])
				Progresses[i]->FreePtr();
		}
		SafeFree(Progresses);
	}
}







/* ************************************************************************ */
/* ************************************************************************ */
/* ************************************************************************ */
/* ************************************************************************ */

void Settings::LoadSettings(bool *rbNeedCreateVanilla)
{
	if (gpConEmu) gpConEmu->LogString(L"Settings::LoadSettings");

	MCHKHEAP
	mb_CharSetWasSet = FALSE;

	// -- перенесено в CheckOptionsFast()
	//;; Q. В Windows Vista зависают другие консольные процессы.
	//	;; A. "Виноват" процесс ConIme.exe. Вроде бы он служит для ввода иероглифов
	//	;;    (китай и т.п.). Зачем он нужен, если ввод теперь идет в графическом окне?
	//	;;    Нужно запретить его автозапуск или вообще переименовать этот файл, например
	//	;;    в 'ConIme.ex1' (видимо это возможно только в безопасном режиме).
	//	;;    Запретить автозапуск: Внесите в реестр и перезагрузитесь
	//if (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 0)
	//{
	//	CheckConIme();
	//}

//-----------------------------------------------------------------------
///| Preload settings actions |//////////////////////////////////////////
//-----------------------------------------------------------------------
	if (gpConEmu->mp_Inside)
	{
		if (gpConEmu->mp_Inside->m_InsideIntegration == CConEmuInside::ii_Auto)
		{
			HWND hParent = gpConEmu->mp_Inside->InsideFindParent();

			if (hParent == INSIDE_PARENT_NOT_FOUND)
			{
				return;
			}
			else if (hParent)
			{
				// Типа, запуститься как панель в Explorer (не в таскбаре, а в проводнике)
				//isStatusBarShow = false;
				isTabs = 0;
				FontSizeY = 10;
				isTryToCenter = true;
				nCenterConsolePad = 3;
				// Скрыть колонки, чтобы много слишком не было...
				isStatusColumnHidden[csi_ConsolePos] = true;
				isStatusColumnHidden[csi_ConsoleSize] = true;
				isStatusColumnHidden[csi_CursorX] = true;
				isStatusColumnHidden[csi_CursorY] = true;
				isStatusColumnHidden[csi_CursorSize] = true;
				isStatusColumnHidden[csi_Server] = true;
			}
		}

		if (gpConEmu->mp_Inside
			&& ((gpConEmu->mp_Inside->mh_InsideParentWND == NULL)
				|| (gpConEmu->mp_Inside->mh_InsideParentWND
					&& (gpConEmu->mp_Inside->mh_InsideParentWND != INSIDE_PARENT_NOT_FOUND)
					&& !IsWindow(gpConEmu->mp_Inside->mh_InsideParentWND))))
		{
			SafeDelete(gpConEmu->mp_Inside);
		}
	}


//-----------------------------------------------------------------------
///| Loading from reg/xml |//////////////////////////////////////////////
//-----------------------------------------------------------------------
	SettingsBase* reg = CreateSettings(NULL);
	if (!reg)
	{
		_ASSERTE(reg!=NULL);
		return;
	}

	wcscpy_c(Type, reg->m_Storage.szType);

	BOOL lbOpened = FALSE;
	*rbNeedCreateVanilla = false;

	lbOpened = reg->OpenKey(gpSetCls->GetConfigPath(), KEY_READ);
	// Поддержка старого стиля хранения (настройки БЕЗ имени конфига - лежали в корне ключа Software\ConEmu)
	if (!lbOpened && (*gpSetCls->GetConfigName() == 0))
	{
		lbOpened = reg->OpenKey(CONEMU_ROOT_KEY, KEY_READ);
		*rbNeedCreateVanilla = (lbOpened != FALSE);
	}

	if (*rbNeedCreateVanilla)
	{
		IsConfigNew = true;
		TODO("Здесь можно включить настройки, которые должны включаться только для новых конфигураций!");
	}
	else
	{
		IsConfigNew = false;
	}


	// Для совместимости настроек
	bool bSendAltEnter = false, bSendAltSpace = false, bSendAltF9 = false;


	if (lbOpened)
	{
		LoadAppSettings(reg, &AppStd/*, Colors*/);

		reg->Load(L"TrueColorerSupport", isTrueColorer);
		reg->Load(L"FadeInactive", isFadeInactive);
		reg->Load(L"FadeInactiveLow", mn_FadeLow);
		reg->Load(L"FadeInactiveHigh", mn_FadeHigh);

		if (mn_FadeHigh <= mn_FadeLow) { mn_FadeLow = DEFAULT_FADE_LOW; mn_FadeHigh = DEFAULT_FADE_HIGH; }
		mn_LastFadeSrc = mn_LastFadeDst = -1;

		// Debugging
		reg->Load(L"ConVisible", isConVisible);
		reg->Load(L"ConInMode", nConInMode);
		
		
		reg->Load(L"UseInjects", isUseInjects); //MinMax(isUseInjects, BST_INDETERMINATE);

		reg->Load(L"SetDefaultTerminal", isSetDefaultTerminal);
		reg->Load(L"SetDefaultTerminalStartup", isRegisterOnOsStartup);
		reg->Load(L"DefaultTerminalNoInjects", isDefaultTerminalNoInjects);
		reg->Load(L"DefaultTerminalConfirm", nDefaultTerminalConfirmClose);
		{
		wchar_t* pszApps = NULL;
		reg->Load(L"DefaultTerminalApps", &pszApps);
		SetDefaultTerminalApps((pszApps && *pszApps) ? pszApps : DEFAULT_TERMINAL_APPS); // "|"-delimited string -> MSZ
		SafeFree(pszApps);
		}

		reg->Load(L"ProcessAnsi", isProcessAnsi);

		reg->Load(L"SuppressBells", isSuppressBells);

		reg->Load(L"ConsoleExceptionHandler", isConsoleExceptionHandler);

		reg->Load(L"UseClink", mb_UseClink);

		#ifdef USEPORTABLEREGISTRY
		reg->Load(L"PortableReg", isPortableReg);
		#endif
		
		// Don't move invisible real console. This affects GUI eMenu.
		//reg->Load(L"LockRealConsolePos", isLockRealConsolePos);
		//reg->Load(L"DumpPackets", szDumpPackets);
		reg->Load(L"AutoRegisterFonts", isAutoRegisterFonts);

		reg->Load(L"FontName", inFont, countof(inFont));

		reg->Load(L"FontName2", inFont2, countof(inFont2));

		if (!*inFont || !*inFont2)
			isAutoRegisterFonts = true;

		reg->Load(L"FontBold", isBold);
		reg->Load(L"FontItalic", isItalic);

		if (!reg->Load(L"Monospace", isMonospace) && !GetSystemMetrics(SM_DBCSENABLED))
		{
			// Compatibility. Пережитки прошлого, загрузить по "старым" именам параметров
			bool bForceMonospace = false, bProportional = false;
			reg->Load(L"ForceMonospace", bForceMonospace);
			reg->Load(L"Proportional", bProportional);
			isMonospace = bForceMonospace ? 2 : bProportional ? 0 : 1;
		}
		if (isMonospace > 2) isMonospace = 2;

		//isMonospaceSelected = isMonospace ? isMonospace : 1; // запомнить, чтобы выбирать то что нужно при смене шрифта
			
			
			
		bool bCmdLine = reg->Load(L"CmdLine", &psStartSingleApp);
		reg->Load(L"StartTasksFile", &psStartTasksFile);
		reg->Load(L"StartTasksName", &psStartTasksName);
		reg->Load(L"StartFarFolders", isStartFarFolders);
		reg->Load(L"StartFarEditors", isStartFarEditors);
		if (!reg->Load(L"StartType", nStartType) && bCmdLine)
		{
			if (*psStartSingleApp == CmdFilePrefix)
				nStartType = 1;
			else if (*psStartSingleApp == TaskBracketLeft)
				nStartType = 2;
		}
		// Check
		if (nStartType > (rbStartLastTabs - rbStartSingleApp))
		{
			_ASSERTE(nStartType <= (rbStartLastTabs - rbStartSingleApp));
			nStartType = 0;
		}
		reg->Load(L"StoreTaskbarkTasks", isStoreTaskbarkTasks);
		reg->Load(L"StoreTaskbarCommands", isStoreTaskbarCommands);

		reg->Load(L"SaveCmdHistory", isSaveCmdHistory);
		reg->Load(L"CmdLineHistory", &psCmdHistory); nCmdHistorySize = 0; HistoryCheck();
		reg->Load(L"SingleInstance", isSingleInstance);
		reg->Load(L"ShowHelpTooltips", isShowHelpTooltips);
		reg->Load(L"Multi", isMulti);
		reg->Load(L"Multi.ShowButtons", isMultiShowButtons);
		reg->Load(L"Multi.NumberInCaption", isNumberInCaption);
		//LoadVkMod(reg, L"Multi.NewConsole", vmMultiNew, vmMultiNew);
		//LoadVkMod(reg, L"Multi.NewConsoleShift", vmMultiNewShift, SetModifier(vmMultiNew,VK_SHIFT, true/*Xor*/));
		//LoadVkMod(reg, L"Multi.Next", vmMultiNext, vmMultiNext);
		//LoadVkMod(reg, L"Multi.NextShift", vmMultiNextShift, SetModifier(vmMultiNext,VK_SHIFT, true/*Xor*/));
		//LoadVkMod(reg, L"Multi.Recreate", vmMultiRecreate, vmMultiRecreate);
		//LoadVkMod(reg, L"Multi.Close", vmMultiClose, vmMultiClose);
		reg->Load(L"Multi.CloseConfirm", isCloseConsoleConfirm);
		reg->Load(L"Multi.CloseEditViewConfirm", isCloseEditViewConfirm);
		//LoadVkMod(reg, L"Multi.CmdKey", vmMultiCmd, vmMultiCmd);
		reg->Load(L"Multi.NewConfirm", isMultiNewConfirm);
		//LoadVkMod(reg, L"Multi.Buffer", vmMultiBuffer, vmMultiBuffer);
		reg->Load(L"Multi.UseArrows", isUseWinArrows);
		reg->Load(L"Multi.UseNumbers", isUseWinNumber);
		reg->Load(L"Multi.UseWinTab", isUseWinTab);
		reg->Load(L"Multi.AutoCreate", isMultiAutoCreate);
		reg->Load(L"Multi.LeaveOnClose", isMultiLeaveOnClose);
		reg->Load(L"Multi.HideOnClose", isMultiHideOnClose);
		reg->Load(L"Multi.MinByEsc", isMultiMinByEsc); MinMax(isMultiMinByEsc, 2);
		reg->Load(L"MapShiftEscToEsc", isMapShiftEscToEsc);
		reg->Load(L"Multi.Iterate", isMultiIterate);
		//LoadVkMod(reg, L"MinimizeRestore", vmMinimizeRestore, vmMinimizeRestore);
		reg->Load(L"Multi.SplitWidth", nSplitWidth); MinMax(nSplitWidth, MAX_SPLITTER_SIZE);
		reg->Load(L"Multi.SplitHeight", nSplitHeight); MinMax(nSplitHeight, MAX_SPLITTER_SIZE);
		//reg->Load(L"Multi.SplitClr1", nSplitClr1);
		//reg->Load(L"Multi.SplitClr2", nSplitClr2);

		reg->Load(L"KeyboardHooks", m_isKeyboardHooks); if (m_isKeyboardHooks>2) m_isKeyboardHooks = 0;

		reg->Load(L"FontAutoSize", isFontAutoSize);
		reg->Load(L"FontSize", FontSizeY);
		reg->Load(L"FontSizeX", FontSizeX);
		reg->Load(L"FontSizeX3", FontSizeX3);
		reg->Load(L"FontSizeX2", FontSizeX2);
		reg->Load(L"FontCharSet", mn_LoadFontCharSet); mb_CharSetWasSet = FALSE;

		reg->Load(L"Anti-aliasing", mn_AntiAlias);
		// Must be explicitly defined, 0 - not allowed
		if (mn_AntiAlias!=NONANTIALIASED_QUALITY && mn_AntiAlias!=ANTIALIASED_QUALITY && mn_AntiAlias!=CLEARTYPE_NATURAL_QUALITY)
			mn_AntiAlias = NONANTIALIASED_QUALITY;
		
		reg->Load(L"ConsoleFontName", ConsoleFont.lfFaceName, countof(ConsoleFont.lfFaceName));
		reg->Load(L"ConsoleFontWidth", ConsoleFont.lfWidth);
		reg->Load(L"ConsoleFontHeight", ConsoleFont.lfHeight);
		
		reg->Load(L"UseCurrentSizePos", isUseCurrentSizePos);
		reg->Load(L"WindowMode", _WindowMode); if (_WindowMode!=rFullScreen && _WindowMode!=rMaximized && _WindowMode!=rNormal) _WindowMode = rNormal;
		reg->Load(L"IntegralSize", mb_IntegralSize);
		reg->Load(L"QuakeStyle", isQuakeStyle);
		reg->Load(L"QuakeAnimation", nQuakeAnimation); MinMax(nQuakeAnimation, QUAKEANIMATION_MAX);
		reg->Load(L"HideCaption", isHideCaption);
		reg->Load(L"HideChildCaption", isHideChildCaption);
		reg->Load(L"FocusInChildWindows", isFocusInChildWindows);
		// грузим именно в mb_HideCaptionAlways, т.к. прозрачность сбивает темы в заголовке, поэтому возврат идет через isHideCaptionAlways()
		reg->Load(L"HideCaptionAlways", mb_HideCaptionAlways);
		reg->Load(L"HideCaptionAlwaysFrame", nHideCaptionAlwaysFrame); if (nHideCaptionAlwaysFrame > 0x7F) nHideCaptionAlwaysFrame = 255;

		reg->Load(L"HideCaptionAlwaysDelay", nHideCaptionAlwaysDelay);

		if (nHideCaptionAlwaysDelay > 30000) nHideCaptionAlwaysDelay = 30000;

		reg->Load(L"HideCaptionAlwaysDisappear", nHideCaptionAlwaysDisappear);

		if (nHideCaptionAlwaysDisappear > 30000) nHideCaptionAlwaysDisappear = 30000;

		reg->Load(L"DownShowHiddenMessage", isDownShowHiddenMessage);

		reg->Load(L"ConWnd X", _wndX); /*if (wndX<-10) wndX = 0;*/
		reg->Load(L"ConWnd Y", _wndY); /*if (wndY<-10) wndY = 0;*/
		reg->Load(L"AutoSaveSizePos", isAutoSaveSizePos);
		// ЭТО не влияет на szDefCmd. Только прямое указание флажка "/BufferHeight N"
		// может сменить (умолчательную) команду запуска на "cmd" или "far"
		reg->Load(L"Cascaded", wndCascade);
		DWORD sizeRaw = wndWidth.Raw;
		reg->Load(L"ConWnd Width", sizeRaw); wndWidth.SetFromRaw(true, sizeRaw);
		sizeRaw = wndHeight.Raw;
		reg->Load(L"ConWnd Height", sizeRaw); wndHeight.SetFromRaw(false, sizeRaw);

		if (gpSetCls->isAdvLogging)
		{
			char szInfo[120]; _wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "Loaded pos: {%i,%i}, size: {%s,%s}", _wndX, _wndY, wndWidth.AsString(), wndHeight.AsString());
			gpConEmu->LogString(szInfo);
		}

		//TODO: Эти два параметра не сохраняются
		reg->Load(L"16bit Height", ntvdmHeight);

		if (ntvdmHeight!=0 && ntvdmHeight!=25 && ntvdmHeight!=28 && ntvdmHeight!=43 && ntvdmHeight!=50) ntvdmHeight = 25;

		reg->Load(L"DefaultBufferHeight", DefaultBufferHeight);

		if (DefaultBufferHeight < LONGOUTPUTHEIGHT_MIN)
			DefaultBufferHeight = LONGOUTPUTHEIGHT_MIN;
		else if (DefaultBufferHeight > LONGOUTPUTHEIGHT_MAX)
			DefaultBufferHeight = LONGOUTPUTHEIGHT_MAX;

		reg->Load(L"AutoBufferHeight", AutoBufferHeight);
		//reg->Load(L"FarSyncSize", FarSyncSize);
		reg->Load(L"CmdOutputCP", nCmdOutputCP);

		BYTE nVal = ComSpec.csType;
		reg->Load(L"ComSpec.Type", nVal);
		if (nVal <= cst_Last) ComSpec.csType = (ComSpecType)nVal;
		//
		nVal = ComSpec.csBits;
		reg->Load(L"ComSpec.Bits", nVal);
		if (nVal <= csb_Last) ComSpec.csBits = (ComSpecBits)nVal;
		//
		nVal = ComSpec.isUpdateEnv;
		reg->Load(L"ComSpec.UpdateEnv", nVal);
		ComSpec.isUpdateEnv = (nVal != 0);
		//
		nVal = (BYTE)((ComSpec.AddConEmu2Path & CEAP_AddConEmuBaseDir) == CEAP_AddConEmuBaseDir);
		reg->Load(L"ComSpec.EnvAddPath", nVal);
		SetConEmuFlags(ComSpec.AddConEmu2Path, CEAP_AddConEmuBaseDir, nVal ? CEAP_AddConEmuBaseDir : CEAP_None);
		//
		nVal = (BYTE)((ComSpec.AddConEmu2Path & CEAP_AddConEmuExeDir) == CEAP_AddConEmuExeDir);
		reg->Load(L"ComSpec.EnvAddExePath", nVal);
		SetConEmuFlags(ComSpec.AddConEmu2Path, CEAP_AddConEmuExeDir, nVal ? CEAP_AddConEmuExeDir : CEAP_None);
		//
		nVal = ComSpec.isAllowUncPaths;
		reg->Load(L"ComSpec.UncPaths", nVal);
		ComSpec.isAllowUncPaths = (nVal != 0);
		//
		reg->Load(L"ComSpec.Path", ComSpec.ComspecExplicit, countof(ComSpec.ComspecExplicit));
		//-- wcscpy_c(ComSpec.ComspecInitial, gpConEmu->ms_ComSpecInitial);
		// Обработать 32/64 (найти tcc.exe и т.п.)
		FindComspec(&ComSpec);
		//Update Comspec(&ComSpec); --> CSettings::SettingsLoaded

		reg->Load(L"ConsoleTextSelection", isConsoleTextSelection); if (isConsoleTextSelection>2) isConsoleTextSelection = 2;

		reg->Load(L"CTS.AutoCopy", isCTSAutoCopy);
		reg->Load(L"CTS.IBeam", isCTSIBeam);
		reg->Load(L"CTS.EndOnTyping", isCTSEndOnTyping); MinMax(isCTSEndOnTyping, 2);
		reg->Load(L"CTS.EndOnKeyPress", isCTSEndOnKeyPress);
		reg->Load(L"CTS.Freeze", isCTSFreezeBeforeSelect);
		reg->Load(L"CTS.SelectBlock", isCTSSelectBlock);
		//reg->Load(L"CTS.VkBlock", isCTSVkBlock);
		//LoadVkMod(reg, L"CTS.VkBlockStart", vmCTSVkBlockStart, vmCTSVkBlockStart);
		reg->Load(L"CTS.SelectText", isCTSSelectText);
		//reg->Load(L"CTS.ClickPromptPosition", isCTSClickPromptPosition); if (isCTSClickPromptPosition > 2) isCTSClickPromptPosition = 2;
		//reg->Load(L"CTS.VkText", isCTSVkText);
		//LoadVkMod(reg, L"CTS.VkTextStart", vmCTSVkTextStart, vmCTSVkTextStart);

		reg->Load(L"CTS.ActMode", isCTSActMode); if (!isCTSActMode || isCTSActMode>2) isCTSActMode = 2;

		//reg->Load(L"CTS.VkAct", isCTSVkAct);

		reg->Load(L"CTS.RBtnAction", isCTSRBtnAction); if (isCTSRBtnAction>3) isCTSRBtnAction = 0;

		reg->Load(L"CTS.MBtnAction", isCTSMBtnAction); if (isCTSMBtnAction>3) isCTSMBtnAction = 0;

		reg->Load(L"CTS.ColorIndex", isCTSColorIndex); if ((isCTSColorIndex & 0xF) == ((isCTSColorIndex & 0xF0)>>4)) isCTSColorIndex = 0xE0;

		reg->Load(L"ClipboardConfirmEnter", isPasteConfirmEnter);
		reg->Load(L"ClipboardConfirmLonger", nPasteConfirmLonger);
		
		reg->Load(L"FarGotoEditorOpt", isFarGotoEditor);
		reg->Load(L"FarGotoEditorPath", &sFarGotoEditor);
		//reg->Load(L"FarGotoEditorVk", isFarGotoEditorVk);

		if (!reg->Load(L"FixFarBorders", isFixFarBorders))
			reg->Load(L"Experimental", isFixFarBorders); //очень старое имя настройки

		//mszCharRanges[0] = 0;

		TODO("Это вообще нужно будет расширить, определяя произвольное количество групп шрифтов");
		// max 100 ranges x 10 chars + a little ;)		
		wchar_t szCharRanges[1024] = {};		
		if (!reg->Load(L"FixFarBordersRanges", szCharRanges, countof(szCharRanges)))		
		{
			wcscpy_c(szCharRanges, L"2013-25C4"); // default
		}
		//{		
		//	int n = 0, nMax = countof(icFixFarBorderRanges);		
		//	wchar_t *pszRange = mszCharRanges, *pszNext = NULL;		
		//	wchar_t cBegin, cEnd;		
		//		
		//	while(*pszRange && n < nMax)		
		//	{		
		//		cBegin = (wchar_t)wcstol(pszRange, &pszNext, 16);		
		//		
		//		if (!cBegin || cBegin == 0xFFFF || *pszNext != L'-') break;		
		//		
		//		pszRange = pszNext + 1;		
		//		cEnd = (wchar_t)wcstol(pszRange, &pszNext, 16);		
		//		
		//		if (!cEnd || cEnd == 0xFFFF) break;		
		//		
		//		icFixFarBorderRanges[n].bUsed = true;		
		//		icFixFarBorderRanges[n].cBegin = cBegin;		
		//		icFixFarBorderRanges[n].cEnd = cEnd;		
		//		n ++;		
		//		
		//		if (*pszNext != L';') break;		
		//		
		//		pszRange = pszNext + 1;		
		//	}		
		//		
		//	for(; n < nMax; n++)		
		//		icFixFarBorderRanges[n].bUsed = false;		
		//}		
		ParseCharRanges(szCharRanges, mpc_FixFarBorderValues);

		reg->Load(L"ExtendUCharMap", isExtendUCharMap);
		reg->Load(L"PartBrush75", isPartBrush75); if (isPartBrush75<5) isPartBrush75=5; else if (isPartBrush75>250) isPartBrush75=250;
		reg->Load(L"PartBrush50", isPartBrush50); if (isPartBrush50<5) isPartBrush50=5; else if (isPartBrush50>250) isPartBrush50=250;
		reg->Load(L"PartBrush25", isPartBrush25); if (isPartBrush25<5) isPartBrush25=5; else if (isPartBrush25>250) isPartBrush25=250;
		reg->Load(L"PartBrushBlack", isPartBrushBlack);

		if (isPartBrush50>=isPartBrush75) isPartBrush50=isPartBrush75-10;

		if (isPartBrush25>=isPartBrush50) isPartBrush25=isPartBrush50-10;

		// Выделим в отдельную настройку
		reg->Load(L"EnhanceGraphics", isEnhanceGraphics);
		
		reg->Load(L"EnhanceButtons", isEnhanceButtons);

		if (isFixFarBorders == 2 && !isEnhanceGraphics)
		{
			isFixFarBorders = 1;
			isEnhanceGraphics = true;
		}

		reg->Load(L"RightClick opens context menu", isRClickSendKey);
		if (!reg->Load(L"RightClickMacro2", &sRClickMacro) || (sRClickMacro && !*sRClickMacro)) { SafeFree(sRClickMacro); }
		
		//reg->Load(L"AltEnter", isSendAltEnter);
		//reg->Load(L"AltSpace", isSendAltSpace); //if (isSendAltSpace > 2) isSendAltSpace = 2; // когда-то был 3state, теперь - bool
		reg->Load(L"SendAltTab", isSendAltTab);
		reg->Load(L"SendAltEsc", isSendAltEsc);
		reg->Load(L"SendAltPrintScrn", isSendAltPrintScrn);
		reg->Load(L"SendPrintScrn", isSendPrintScrn);
		reg->Load(L"SendCtrlEsc", isSendCtrlEsc);
		//reg->Load(L"SendAltF9", isSendAltF9);

		// Для совместимости настроек
		reg->Load(L"AltEnter", bSendAltEnter);
		reg->Load(L"AltSpace", bSendAltSpace);
		reg->Load(L"SendAltF9", bSendAltF9);

		
		reg->Load(L"Min2Tray", mb_MinToTray);
		reg->Load(L"AlwaysShowTrayIcon", isAlwaysShowTrayIcon);
		
		reg->Load(L"SafeFarClose", isSafeFarClose);
		if (!reg->Load(L"SafeFarCloseMacro", &sSafeFarCloseMacro) || (sSafeFarCloseMacro && !*sSafeFarCloseMacro)) { SafeFree(sSafeFarCloseMacro); }
		
		reg->Load(L"FARuseASCIIsort", isFARuseASCIIsort);
		reg->Load(L"ShellNoZoneCheck", isShellNoZoneCheck);
		reg->Load(L"FixAltOnAltTab", isFixAltOnAltTab);
		
		reg->Load(L"BackGround Image show", isShowBgImage);
		if (isShowBgImage!=0 && isShowBgImage!=1 && isShowBgImage!=2) isShowBgImage = 0;
		reg->Load(L"BackGround Image", sBgImage, countof(sBgImage));
		reg->Load(L"bgImageDarker", bgImageDarker);
		reg->Load(L"bgImageColors", nBgImageColors);
		if (!nBgImageColors) nBgImageColors = (DWORD)-1; //1|2 == BgImageColorsDefaults;
		reg->Load(L"bgOperation", bgOperation);
		if (bgOperation!=eUpLeft && bgOperation!=eUpRight
			&& bgOperation!=eDownLeft && bgOperation!=eDownRight
			&& bgOperation!=eStretch && bgOperation!=eTile)
		{
			bgOperation = eUpLeft;
		}
		reg->Load(L"bgPluginAllowed", isBgPluginAllowed);
		if (isBgPluginAllowed!=0 && isBgPluginAllowed!=1 && isBgPluginAllowed!=2) isBgPluginAllowed = 1;

		reg->Load(L"AlphaValue", nTransparent); MinMax(nTransparent, MIN_ALPHA_VALUE, 255);
		reg->Load(L"AlphaValueSeparate", isTransparentSeparate);
		if (!reg->Load(L"AlphaValueInactive", nTransparentInactive))
		{	
			nTransparentInactive = nTransparent;
		}
		else
		{
			MinMax(nTransparentInactive, MIN_INACTIVE_ALPHA_VALUE, 255);
		}

		//reg->Load(L"UseColorKey", isColorKey);
		//reg->Load(L"ColorKey", ColorKey);
		reg->Load(L"UserScreenTransparent", isUserScreenTransparent);

		reg->Load(L"ColorKeyTransparent", isColorKeyTransparent);
		reg->Load(L"ColorKeyValue", nColorKeyValue); nColorKeyValue = nColorKeyValue & 0xFFFFFF;

		
		//reg->Load(L"Update Console handle", isUpdConHandle);
		reg->Load(L"DisableMouse", isDisableMouse);
		reg->Load(L"RSelectionFix", isRSelFix);
		reg->Load(L"MouseSkipActivation", isMouseSkipActivation);
		reg->Load(L"MouseSkipMoving", isMouseSkipMoving);
		reg->Load(L"FarHourglass", isFarHourglass);
		reg->Load(L"FarHourglassDelay", nFarHourglassDelay);
		
		reg->Load(L"Dnd", isDragEnabled);
		isDropEnabled = (BYTE)(isDragEnabled ? 1 : 0); // ранее "DndDrop" не было, поэтому ставим default
		//reg->Load(L"DndLKey", nLDragKey);
		//reg->Load(L"DndRKey", nRDragKey);
		reg->Load(L"DndDrop", isDropEnabled);
		reg->Load(L"DefCopy", isDefCopy);
		reg->Load(L"DropUseMenu", isDropUseMenu);
		reg->Load(L"DragOverlay", isDragOverlay);
		reg->Load(L"DragShowIcons", isDragShowIcons);
		
		reg->Load(L"DragPanel", isDragPanel); if (isDragPanel > 2) isDragPanel = 1;
		reg->Load(L"DragPanelBothEdges", isDragPanelBothEdges);

		reg->Load(L"KeyBarRClick", isKeyBarRClick);
		
		reg->Load(L"DebugSteps", isDebugSteps);				

		mb_StatusSettingsWasChanged = false;
		reg->Load(L"StatusBar.Show", isStatusBarShow);
		reg->Load(L"StatusBar.Flags", isStatusBarFlags);
		reg->Load(L"StatusFontFace", sStatusFontFace, countof(sStatusFontFace));
		reg->Load(L"StatusFontCharSet", nStatusFontCharSet);
		reg->Load(L"StatusFontHeight", nStatusFontHeight); nStatusFontHeight = max(4,nStatusFontHeight);
		reg->Load(L"StatusBar.Color.Back", nStatusBarBack);
		reg->Load(L"StatusBar.Color.Light", nStatusBarLight);
		reg->Load(L"StatusBar.Color.Dark", nStatusBarDark);
		_ASSERTE(countof(isStatusColumnHidden)>csi_Last);
		_ASSERTE(csi_Info==0);
		//reg->Load(L"StatusBar.HideColumns", nHideStatusColumns);
		for (int i = csi_Info; i < csi_Last; i++)
		{
			LPCWSTR pszName = gpConEmu->mp_Status->GetSettingName((CEStatusItems)i);
			if (pszName)
				reg->Load(pszName, isStatusColumnHidden[i]);
			else
				isStatusColumnHidden[i] = false;
		}

		//reg->Load(L"GUIpb", isGUIpb);
		reg->Load(L"Tabs", isTabs);
		reg->Load(L"TabsLocation", nTabsLocation);
		reg->Load(L"TabIcons", isTabIcons);
		reg->Load(L"OneTabPerGroup", isOneTabPerGroup);
		reg->Load(L"ActivateSplitMouseOver", isActivateSplitMouseOver);
		reg->Load(L"TabSelf", isTabSelf);
		reg->Load(L"TabLazy", isTabLazy);
		reg->Load(L"TabRecent", isTabRecent);
		reg->Load(L"TabDblClick", nTabBarDblClickAction); MinMax(nTabBarDblClickAction, 3);
		reg->Load(L"TabBtnDblClick", nTabBtnDblClickAction); MinMax(nTabBtnDblClickAction, 4);
		reg->Load(L"TabsOnTaskBar", m_isTabsOnTaskBar);
		reg->Load(L"TaskBarOverlay", isTaskbarShield);

		if (!reg->Load(L"TabCloseMacro", &sTabCloseMacro) || (sTabCloseMacro && !*sTabCloseMacro)) { SafeFree(sTabCloseMacro); }

		reg->Load(L"TabFontFace", sTabFontFace, countof(sTabFontFace));
		reg->Load(L"TabFontCharSet", nTabFontCharSet);
		reg->Load(L"TabFontHeight", nTabFontHeight); nTabFontHeight = max(4,nTabFontHeight);

		if (!reg->Load(L"SaveAllEditors", &sSaveAllMacro) || (sSaveAllMacro && !*sSaveAllMacro)) { SafeFree(sSaveAllMacro); }

		//reg->Load(L"TabFrame", isTabFrame);
		//reg->Load(L"TabMargins", rcTabMargins);
		
		reg->Load(L"ToolbarAddSpace", nToolbarAddSpace);
		if (nToolbarAddSpace<0 || nToolbarAddSpace>100) nToolbarAddSpace = 0;

		reg->Load(L"SlideShowElapse", nSlideShowElapse);
		
		reg->Load(L"IconID", nIconID);
		
		
		reg->Load(L"TabConsole", szTabConsole, countof(szTabConsole));
		reg->Load(L"TabSkipWords", szTabSkipWords, countof(szTabSkipWords));
		wcscpy_c(szTabPanels, szTabConsole); // Раньше была только настройка "TabConsole". Унаследовать ее в "TabPanels"
		reg->Load(L"TabPanels", szTabPanels, countof(szTabPanels));
		reg->Load(L"TabEditor", szTabEditor, countof(szTabEditor));
		reg->Load(L"TabEditorModified", szTabEditorModified, countof(szTabEditorModified));
		reg->Load(L"TabViewer", szTabViewer, countof(szTabViewer));
		reg->Load(L"TabLenMax", nTabLenMax); if (nTabLenMax < 10 || nTabLenMax >= CONEMUTABMAX) nTabLenMax = 20;

		/*reg->Load(L"ScrollTitle", isScrollTitle);
		reg->Load(L"ScrollTitleLen", ScrollTitleLen);*/
		reg->Load(L"AdminTitleSuffix", szAdminTitleSuffix, countof(szAdminTitleSuffix)); szAdminTitleSuffix[countof(szAdminTitleSuffix)-1] = 0;
		reg->Load(L"AdminShowShield", bAdminShield);
		reg->Load(L"HideInactiveConsoleTabs", bHideInactiveConsoleTabs);
		//reg->Load(L"HideDisabledTabs", bHideDisabledTabs);
		reg->Load(L"ShowFarWindows", bShowFarWindows);
		
		reg->Load(L"TryToCenter", isTryToCenter);
		reg->Load(L"CenterConsolePad", nCenterConsolePad); MinMax(nCenterConsolePad,CENTERCONSOLEPAD_MIN,CENTERCONSOLEPAD_MAX);

		reg->Load(L"ShowScrollbar", isAlwaysShowScrollbar); MinMax(isAlwaysShowScrollbar,2);
		reg->Load(L"ScrollBarAppearDelay", nScrollBarAppearDelay); MinMax(nScrollBarAppearDelay,SCROLLBAR_DELAY_MIN,SCROLLBAR_DELAY_MAX);
		reg->Load(L"ScrollBarDisappearDelay", nScrollBarDisappearDelay); MinMax(nScrollBarDisappearDelay,SCROLLBAR_DELAY_MIN,SCROLLBAR_DELAY_MAX);

		//reg->Load(L"CreateAppWindow", isCreateAppWindow);
		//reg->Load(L"AllowDetach", isAllowDetach);

		reg->Load(L"MainTimerElapse", nMainTimerElapse); if (nMainTimerElapse>1000) nMainTimerElapse = 1000;
		reg->Load(L"MainTimerInactiveElapse", nMainTimerInactiveElapse); if (nMainTimerInactiveElapse>10000) nMainTimerInactiveElapse = 10000;

		reg->Load(L"AffinityMask", nAffinity);
		
		//reg->Load(L"AdvLangChange", isAdvLangChange);
		
		reg->Load(L"SkipFocusEvents", isSkipFocusEvents);
		
		//reg->Load(L"LangChangeWsPlugin", isLangChangeWsPlugin);
		reg->Load(L"MonitorConsoleLang", isMonitorConsoleLang);
		reg->Load(L"DesktopMode", isDesktopMode);
		reg->Load(L"SnapToDesktopEdges", isSnapToDesktopEdges);
		reg->Load(L"AlwaysOnTop", isAlwaysOnTop);
		reg->Load(L"SleepInBackground", isSleepInBackground);
		reg->Load(L"MinimizeOnLoseFocus", mb_MinimizeOnLoseFocus);

		reg->Load(L"DisableFarFlashing", isDisableFarFlashing); if (isDisableFarFlashing>2) isDisableFarFlashing = 2;

		reg->Load(L"DisableAllFlashing", isDisableAllFlashing); if (isDisableAllFlashing>2) isDisableAllFlashing = 2;

		/* FindText: bMatchCase, bMatchWholeWords, bFreezeConsole, bHighlightAll */
		reg->Load(L"FindText", &FindOptions.pszText);
		reg->Load(L"FindMatchCase", FindOptions.bMatchCase);
		reg->Load(L"FindMatchWholeWords", FindOptions.bMatchWholeWords);
		reg->Load(L"FindFreezeConsole", FindOptions.bFreezeConsole);
		reg->Load(L"FindHighlightAll", FindOptions.bHighlightAll);
		reg->Load(L"FindTransparent", FindOptions.bTransparent);


		/* *********** Thumbnails and Tiles ************* */
		reg->Load(L"PanView.BackColor", ThSet.crBackground.RawColor);

		reg->Load(L"PanView.PFrame", ThSet.nPreviewFrame); if (ThSet.nPreviewFrame!=0 && ThSet.nPreviewFrame!=1) ThSet.nPreviewFrame = 1;

		reg->Load(L"PanView.PFrameColor", ThSet.crPreviewFrame.RawColor);

		reg->Load(L"PanView.SFrame", ThSet.nSelectFrame); if (ThSet.nSelectFrame!=0 && ThSet.nSelectFrame!=1) ThSet.nSelectFrame = 1;

		reg->Load(L"PanView.SFrameColor", ThSet.crSelectFrame.RawColor);
		/* теперь разнообразные размеры */
		ThumbLoadSet(L"Thumbs", ThSet.Thumbs);
		ThumbLoadSet(L"Tiles", ThSet.Tiles);
		// Прочие параметры загрузки
		reg->Load(L"PanView.LoadPreviews", ThSet.bLoadPreviews);
		reg->Load(L"PanView.LoadFolders", ThSet.bLoadFolders);
		reg->Load(L"PanView.LoadTimeout", ThSet.nLoadTimeout);
		reg->Load(L"PanView.MaxZoom", ThSet.nMaxZoom);
		reg->Load(L"PanView.UsePicView2", ThSet.bUsePicView2);
		reg->Load(L"PanView.RestoreOnStartup", ThSet.bRestoreOnStartup);

		/* *** AutoUpdate *** */
		reg->Load(L"Update.VerLocation", &UpdSet.szUpdateVerLocation);
		reg->Load(L"Update.CheckOnStartup", UpdSet.isUpdateCheckOnStartup);
		reg->Load(L"Update.CheckHourly", UpdSet.isUpdateCheckHourly);
		reg->Load(L"Update.ConfirmDownload", UpdSet.isUpdateConfirmDownload);
		reg->Load(L"Update.UseBuilds", UpdSet.isUpdateUseBuilds); if (UpdSet.isUpdateUseBuilds>3) UpdSet.isUpdateUseBuilds = 3; // 1-stable only, 2-latest, 3-preview
		reg->Load(L"Update.UseProxy", UpdSet.isUpdateUseProxy);
		reg->Load(L"Update.Proxy", &UpdSet.szUpdateProxy);
		reg->Load(L"Update.ProxyUser", &UpdSet.szUpdateProxyUser);
		reg->Load(L"Update.ProxyPassword", &UpdSet.szUpdateProxyPassword);
		reg->Load(L"Update.DownloadMode", UpdSet.isUpdateDownloadSetup); // 1-Installer (ConEmuSetup.exe), 2-7z archieve (ConEmu.7z), WinRar or 7z required
		reg->Load(L"Update.ExeCmdLine", &UpdSet.szUpdateExeCmdLine);
		reg->Load(L"Update.ArcCmdLine", &UpdSet.szUpdateArcCmdLine);
		reg->Load(L"Update.DownloadPath", &UpdSet.szUpdateDownloadPath);
		reg->Load(L"Update.LeavePackages", UpdSet.isUpdateLeavePackages);
		reg->Load(L"Update.PostUpdateCmd", &UpdSet.szUpdatePostUpdateCmd);

		/* Hotkeys */
		LoadHotkeys(reg);
		// Для совместимости настроек
		if (bSendAltSpace || bSendAltEnter || bSendAltF9)
		{
			ConEmuHotKey* pHK;
			// Если раньше был включен флажок "Send Alt+Space to console"
			if (bSendAltSpace && GetHotkeyById(vkSystemMenu, (const ConEmuHotKey**)&pHK) && pHK->NotChanged && (pHK->VkMod == ConEmuHotKey::MakeHotKey(VK_SPACE,VK_MENU)))
			{
				pHK->VkMod = 0; // Сбросить VkMod для vkSystemMenu (раньше назывался vkAltSpace)
			}
			// Если раньше был включен флажок "Send Alt+Enter to console"
			if (bSendAltEnter && GetHotkeyById(vkAltEnter, (const ConEmuHotKey**)&pHK) && pHK->NotChanged && (pHK->VkMod == ConEmuHotKey::MakeHotKey(VK_RETURN,VK_MENU)))
			{
				pHK->VkMod = 0; // Сбросить VkMod для vkAltEnter
			}
			// Если раньше был включен флажок "Send Alt+F9 to console"
			if (bSendAltF9 && GetHotkeyById(vkAltF9, (const ConEmuHotKey**)&pHK) && pHK->NotChanged && (pHK->VkMod == ConEmuHotKey::MakeHotKey(VK_F9,VK_MENU)))
			{
				pHK->VkMod = 0; // Сбросить VkMod для vkAltF9
			}
		}

		/* Done */
		reg->CloseKey();
	}

	LoadPalettes(reg);

	LoadAppSettings(reg);

	LoadCmdTasks(reg);

	// сервис больше не нужен
	delete reg;
	reg = NULL;

	// -- перенесено в gpSetCls->SettingsLoaded();
	//// Зовем "FastConfiguration" (перед созданием новой/чистой конфигурации)
	//{
	//	LPCWSTR pszDef = gpConEmu->GetDefaultTitle();
	//	wchar_t szType[8]; bool ReadOnly;
	//	GetSettingsType(szType, ReadOnly);
	//	LPCWSTR pszConfig = gpSetCls->GetConfigName();
	//	wchar_t szTitle[1024];
	//	if (pszConfig && *pszConfig)
	//		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s fast configuration (%s) %s", pszDef, pszConfig, szType);
	//	else
	//		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s fast configuration %s", pszDef, szType);
	//	
	//	CheckOptionsFast(szTitle, lbNeedCreateVanilla);
	//}
	//

	//if (lbNeedCreateVanilla)
	//{
	//	SaveSettings(TRUE/*abSilent*/);
	//}

	//// Передернуть палитру затенения
	//ResetFadeColors();
	//GetColors(-1, TRUE);
	//
	//

	//// Проверить необходимость установки хуков
	////-- isKeyboardHooks();
	//// При первом запуске - проверить, хотят ли включить автообновление?
	////-- CheckUpdatesWanted();



	//// Стили окна
	//if ((_WindowMode!=rNormal) && (_WindowMode!=rMaximized) && (_WindowMode!=rFullScreen))
	//{
	//	// Иначе окно вообще не отображается
	//	_ASSERTE(_WindowMode!=0);
	//	_WindowMode = rNormal;
	//}

	////if (rcTabMargins.top > 100) rcTabMargins.top = 100;
	////_ASSERTE((rcTabMargins.bottom == 0 || rcTabMargins.top == 0) && !rcTabMargins.left && !rcTabMargins.right);
	////rcTabMargins.left = rcTabMargins.right = 0;
	////int nTabHeight = rcTabMargins.top ? rcTabMargins.top : rcTabMargins.bottom;
	////if (nTabsLocation == 1)
	////{
	////	rcTabMargins.top = 0; rcTabMargins.bottom = nTabHeight;
	////}
	////else
	////{
	////	rcTabMargins.top = nTabHeight; rcTabMargins.bottom = 0;
	////}

	//if (!psCmdHistory)
	//{
	//	psCmdHistory = (wchar_t*)calloc(2,2);
	//}

	//MCHKHEAP
	
	// -- перенесено в WinMain
	//// Переменные загружены, выполнить дополнительные действия в классе настроек
	//gpSetCls->SettingsLoaded();
}

void Settings::SaveSizePosOnExit()
{
	if (!this || !(isAutoSaveSizePos || mb_StatusSettingsWasChanged))
		return;

	// При закрытии окна крестиком - сохранять только один раз,
	// а то размер может в процессе закрытия консолей измениться
	if (mb_SizePosAutoSaved)
		return;
	mb_SizePosAutoSaved = true;

	gpConEmu->LogWindowPos(L"SaveSizePosOnExit");
		
	SettingsBase* reg = CreateSettings(NULL);
	if (!reg)
	{
		_ASSERTE(reg!=NULL);
		return;
	}

	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
	{
		if (isAutoSaveSizePos)
		{
			reg->Save(L"UseCurrentSizePos", isUseCurrentSizePos);
			DWORD saveMode = (isUseCurrentSizePos==false) ? _WindowMode // сохранять будем то, что задано пользователем явно
					: ( (ghWnd == NULL)  // иначе - текущее состояние
							? gpConEmu->WindowMode
							: (gpConEmu->isFullScreen() ? rFullScreen : gpConEmu->isZoomed() ? rMaximized : rNormal));
			reg->Save(L"WindowMode", saveMode);
			reg->Save(L"ConWnd Width", isUseCurrentSizePos ? gpConEmu->WndWidth.Raw : wndWidth.Raw);
			reg->Save(L"ConWnd Height", isUseCurrentSizePos ? gpConEmu->WndHeight.Raw : wndHeight.Raw);
			reg->Save(L"Cascaded", wndCascade);
			reg->Save(L"IntegralSize", mb_IntegralSize);
			reg->Save(L"ConWnd X", isUseCurrentSizePos ? gpConEmu->wndX : _wndX);
			reg->Save(L"ConWnd Y", isUseCurrentSizePos ? gpConEmu->wndY : _wndY);
			reg->Save(L"16bit Height", ntvdmHeight);
			reg->Save(L"AutoSaveSizePos", isAutoSaveSizePos);
		}

		if (mb_StatusSettingsWasChanged)
			SaveStatusSettings(reg);
		//{
		//	mb_StatusSettingsWasChanged = false;
		//	reg->Save(L"StatusBar.Show", isStatusBarShow);
		//	for (int i = csi_Info; i < csi_Last; i++)
		//	{
		//		LPCWSTR pszName = gpConEmu->mp_Status->GetSettingName((CEStatusItems)i);
		//		if (pszName)
		//			reg->Save(pszName, isStatusColumnHidden[i]);
		//	}
		//}

		reg->CloseKey();
	}

	delete reg;
}

void Settings::SaveConsoleFont()
{
	if (!this)
		return;

	SettingsBase* reg = CreateSettings(NULL);
	if (!reg)
	{
		_ASSERTE(reg!=NULL);
		return;
	}

	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
	{
		reg->Save(L"ConsoleFontName", ConsoleFont.lfFaceName);
		reg->Save(L"ConsoleFontWidth", ConsoleFont.lfWidth);
		reg->Save(L"ConsoleFontHeight", ConsoleFont.lfHeight);
		reg->CloseKey();
	}

	delete reg;
}

void Settings::SaveFindOptions(SettingsBase* reg/* = NULL*/)
{
	bool bDelete = (reg == NULL);
	if (!reg)
	{
		reg = CreateSettings(NULL);
		if (!reg)
		{
			_ASSERTE(reg!=NULL);
			return;
		}

		if (!reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE, TRUE))
		{
			delete reg;
			return;
		}
	}
	
	reg->Save(L"FindText", FindOptions.pszText);
	reg->Save(L"FindMatchCase", FindOptions.bMatchCase);
	reg->Save(L"FindMatchWholeWords", FindOptions.bMatchWholeWords);
#if 0
	// пока не работает - не сохраняем
	reg->Save(L"FindFreezeConsole", FindOptions.bFreezeConsole);
	reg->Save(L"FindHighlightAll", FindOptions.bHighlightAll);
#endif
	reg->Save(L"FindTransparent", FindOptions.bTransparent);

	if (bDelete)
	{
		reg->CloseKey();
		delete reg;
	}
}

void Settings::SaveAppSettings(SettingsBase* reg)
{
	BOOL lbOpened = FALSE;
	wchar_t szAppKey[MAX_PATH+64];
	wcscpy_c(szAppKey, gpSetCls->GetConfigPath());
	wcscat_c(szAppKey, L"\\Apps");
	wchar_t* pszAppKey = szAppKey+lstrlen(szAppKey);

	lbOpened = reg->OpenKey(szAppKey, KEY_WRITE);
	if (lbOpened)
	{
		reg->Save(L"Count", AppCount);
		reg->CloseKey();
	}

	for (int i = 0; i < AppCount; i++)
	{
		_wsprintf(pszAppKey, SKIPLEN(32) L"\\App%i", i+1);

		lbOpened = reg->OpenKey(szAppKey, KEY_WRITE);
		if (lbOpened)
		{
			// Загрузка "AppNames" - снаружи, т.к. LoadAppSettings используется и для загрузки &AppStd
			reg->Save(L"AppNames", Apps[i]->AppNames);
			reg->Save(L"Elevated", Apps[i]->Elevated);
			SaveAppSettings(reg, Apps[i]/*, Цвета сохраняются как Имя палитры*/);
			reg->CloseKey();
		}
	}
}

void Settings::SaveAppSettings(SettingsBase* reg, Settings::AppSettings* pApp/*, COLORREF* pColors*/)
{
	// Для AppStd данные загружаются из основной ветки! В том числе и цвета (RGB[32] а не имя палитры)
	bool bStd = (pApp == &AppStd);

	if (bStd)
	{
		TCHAR ColorName[] = L"ColorTable00";

		for(uint i = 0; i<countof(Colors)/*0x20*/; i++)
		{
			ColorName[10] = i/10 + '0';
			ColorName[11] = i%10 + '0';
			reg->Save(ColorName, (DWORD)Colors[i]);
		}

		reg->Save(L"ExtendColors", pApp->isExtendColors);
		reg->Save(L"ExtendColorIdx", pApp->nExtendColorIdx);

		reg->Save(L"TextColorIdx", pApp->nTextColorIdx);
		reg->Save(L"BackColorIdx", pApp->nBackColorIdx);
		reg->Save(L"PopTextColorIdx", pApp->nPopTextColorIdx);
		reg->Save(L"PopBackColorIdx", pApp->nPopBackColorIdx);
	}
	else
	{
		reg->Save(L"OverridePalette", pApp->OverridePalette);
		reg->Save(L"PaletteName", pApp->szPaletteName);
	}

	if (!bStd)
		reg->Save(L"OverrideExtendFonts", pApp->OverrideExtendFonts);
	reg->Save(L"ExtendFonts", pApp->isExtendFonts);
	reg->Save(L"ExtendFontNormalIdx", pApp->nFontNormalColor);
	reg->Save(L"ExtendFontBoldIdx", pApp->nFontBoldColor);
	reg->Save(L"ExtendFontItalicIdx", pApp->nFontItalicColor);

	if (!bStd)
		reg->Save(L"OverrideCursor", pApp->OverrideCursor);
	reg->Save(L"CursorTypeActive", pApp->CursorActive.Raw);
	reg->Save(L"CursorTypeInactive", pApp->CursorInactive.Raw);

	if (!bStd)
		reg->Save(L"OverrideClipboard", pApp->OverrideClipboard);
	reg->Save(L"ClipboardDetectLineEnd", pApp->isCTSDetectLineEnd);
	reg->Save(L"ClipboardBashMargin", pApp->isCTSBashMargin);
	reg->Save(L"ClipboardTrimTrailing", pApp->isCTSTrimTrailing);
	reg->Save(L"ClipboardEOL", pApp->isCTSEOL);
	reg->Save(L"ClipboardArrowStart", pApp->isCTSShiftArrowStart);
	reg->Save(L"ClipboardAllLines", pApp->isPasteAllLines);
	reg->Save(L"ClipboardFirstLine", pApp->isPasteFirstLine);
	reg->Save(L"ClipboardClickPromptPosition", pApp->isCTSClickPromptPosition);
	reg->Save(L"ClipboardDeleteLeftWord", pApp->isCTSDeleteLeftWord);
}

void Settings::SaveStatusSettings(SettingsBase* reg)
{
	mb_StatusSettingsWasChanged = false;
	reg->Save(L"StatusBar.Show", isStatusBarShow);
	reg->Save(L"StatusBar.Flags", isStatusBarFlags);
	reg->Save(L"StatusFontFace", sStatusFontFace);
	reg->Save(L"StatusFontCharSet", nStatusFontCharSet);
	reg->Save(L"StatusFontHeight", nStatusFontHeight);
	reg->Save(L"StatusBar.Color.Back", nStatusBarBack);
	reg->Save(L"StatusBar.Color.Light", nStatusBarLight);
	reg->Save(L"StatusBar.Color.Dark", nStatusBarDark);
	for (int i = csi_Info; i < csi_Last; i++)
	{
		LPCWSTR pszName = gpConEmu->mp_Status->GetSettingName((CEStatusItems)i);
		if (pszName)
			reg->Save(pszName, isStatusColumnHidden[i]);
	}
}

BOOL Settings::SaveSettings(BOOL abSilent /*= FALSE*/, const SettingsStorage* apStorage /*= NULL*/)
{
	if (gpConEmu) gpConEmu->LogString(L"Settings::SaveSettings");

	BOOL lbRc = FALSE;

	gpSetCls->SettingsPreSave();

	SettingsBase* reg = CreateSettings(apStorage);
	if (!reg)
	{
		_ASSERTE(reg!=NULL);
		return FALSE;
	}

	// Если в реестре настройка есть, или изменилось значение
	bool lbCurAutoRegisterFonts = isAutoRegisterFonts, lbCurAutoRegisterFontsRc = false;
	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_READ, abSilent))
	{
		lbCurAutoRegisterFontsRc = reg->Load(L"AutoRegisterFonts", lbCurAutoRegisterFonts);
		reg->CloseKey();
	}


	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE, abSilent))
	{
		// При сохранении меняем "сохраненный" тип (отображается в заголовке диалога)
		// только если сохранение было "умолчательное", а не "Экспорт в файл"
		if (apStorage == NULL)
		{
			wcscpy_c(Type, reg->m_Storage.szType);
		}

		SaveAppSettings(reg, &AppStd/*, Colors*/);

		reg->Save(L"TrueColorerSupport", isTrueColorer);
		reg->Save(L"FadeInactive", isFadeInactive);
		reg->Save(L"FadeInactiveLow", mn_FadeLow);
		reg->Save(L"FadeInactiveHigh", mn_FadeHigh);

		/*if (!isFullScreen && !gpConEmu->isZoomed() && !gpConEmu->isIconic())
		{
		    RECT rcPos; GetWindowRect(ghWnd, &rcPos);
		    wndX = rcPos.left;
		    wndY = rcPos.top;
		}*/
		reg->Save(L"ConVisible", isConVisible);
		reg->Save(L"ConInMode", nConInMode);
		
		reg->Save(L"UseInjects", isUseInjects);

		reg->Save(L"SetDefaultTerminal", isSetDefaultTerminal);
		reg->Save(L"SetDefaultTerminalStartup", isRegisterOnOsStartup);
		reg->Save(L"DefaultTerminalNoInjects", isDefaultTerminalNoInjects);
		reg->Save(L"DefaultTerminalConfirm", nDefaultTerminalConfirmClose);
		{
		wchar_t* pszApps = GetDefaultTerminalApps(); // MSZ -> "|"-delimited string
		reg->Save(L"DefaultTerminalApps", pszApps);
		SafeFree(pszApps);
		}

		reg->Save(L"ProcessAnsi", isProcessAnsi);

		_ASSERTE(isSuppressBells==false); // пока не доделано - не сохраняем
		//reg->Save(L"SuppressBells", isSuppressBells);

		reg->Save(L"ConsoleExceptionHandler", isConsoleExceptionHandler);

		reg->Save(L"UseClink", mb_UseClink);

		#ifdef USEPORTABLEREGISTRY
		reg->Save(L"PortableReg", isPortableReg);
		#endif
		
		//reg->Save(L"LockRealConsolePos", isLockRealConsolePos);

		reg->Save(L"StartType", nStartType);
		reg->Save(L"CmdLine", psStartSingleApp);
		reg->Save(L"StartTasksFile", psStartTasksFile);
		reg->Save(L"StartTasksName", psStartTasksName);
		reg->Save(L"StartFarFolders", isStartFarFolders);
		reg->Save(L"StartFarEditors", isStartFarEditors);

		reg->Save(L"StoreTaskbarkTasks", isStoreTaskbarkTasks);
		reg->Save(L"StoreTaskbarCommands", isStoreTaskbarCommands);

		reg->Save(L"SaveCmdHistory", isSaveCmdHistory);
		if (psCmdHistory)
		{
			// Пишем всегда, даже если (!isSaveCmdHistory), т.к. история могла быть "преднастроена"
			reg->SaveMSZ(L"CmdLineHistory", psCmdHistory, nCmdHistorySize);
		}

		reg->Save(L"SingleInstance", isSingleInstance);
		reg->Save(L"ShowHelpTooltips", isShowHelpTooltips);
		reg->Save(L"Multi", isMulti);
		reg->Save(L"Multi.ShowButtons", isMultiShowButtons);
		reg->Save(L"Multi.NumberInCaption", isNumberInCaption);
		//reg->Save(L"Multi.NewConsole", vmMultiNew);
		//reg->Save(L"Multi.NewConsoleShift", vmMultiNewShift);
		//reg->Save(L"Multi.Next", vmMultiNext);
		//reg->Save(L"Multi.NextShift", vmMultiNextShift);
		//reg->Save(L"Multi.Recreate", vmMultiRecreate);
		//reg->Save(L"Multi.Close", vmMultiClose);
		reg->Save(L"Multi.CloseConfirm", isCloseConsoleConfirm);
		reg->Save(L"Multi.CloseEditViewConfirm", isCloseEditViewConfirm);
		//reg->Save(L"Multi.CmdKey", vmMultiCmd);
		reg->Save(L"Multi.NewConfirm", isMultiNewConfirm);
		//reg->Save(L"Multi.Buffer", vmMultiBuffer);
		reg->Save(L"Multi.UseArrows", isUseWinArrows);
		reg->Save(L"Multi.UseNumbers", isUseWinNumber);
		reg->Save(L"Multi.UseWinTab", isUseWinTab);
		reg->Save(L"Multi.AutoCreate", isMultiAutoCreate);
		reg->Save(L"Multi.LeaveOnClose", isMultiLeaveOnClose);
		reg->Save(L"Multi.HideOnClose", isMultiHideOnClose);
		reg->Save(L"Multi.MinByEsc", isMultiMinByEsc);
		reg->Save(L"MapShiftEscToEsc", isMapShiftEscToEsc);
		reg->Save(L"Multi.Iterate", isMultiIterate);
		reg->Save(L"Multi.SplitWidth", nSplitWidth);
		reg->Save(L"Multi.SplitHeight", nSplitHeight);
		//reg->Save(L"Multi.SplitClr1", nSplitClr1);
		//reg->Save(L"Multi.SplitClr2", nSplitClr2);

		//reg->Save(L"MinimizeRestore", vmMinimizeRestore);
		_ASSERTE(m_isKeyboardHooks!=0);
		reg->Save(L"KeyboardHooks", m_isKeyboardHooks);
		reg->Save(L"FontName", inFont);
		reg->Save(L"FontName2", inFont2);

		// Если в реестре настройка есть, или изменилось значение
		if (lbCurAutoRegisterFontsRc || (isAutoRegisterFonts != lbCurAutoRegisterFonts))
			reg->Save(L"AutoRegisterFonts", isAutoRegisterFonts);
			
		reg->Save(L"FontAutoSize", isFontAutoSize);
		reg->Save(L"FontSize", FontSizeY);
		reg->Save(L"FontSizeX", FontSizeX);
		reg->Save(L"FontSizeX2", FontSizeX2);
		reg->Save(L"FontSizeX3", FontSizeX3);
		reg->Save(L"FontCharSet", mn_LoadFontCharSet);
		if (ghOpWnd != NULL)			
			mb_CharSetWasSet = FALSE;
		reg->Save(L"Anti-aliasing", mn_AntiAlias);
		reg->Save(L"FontBold", isBold);
		reg->Save(L"FontItalic", isItalic);
		
		reg->Save(L"Monospace", isMonospace);
		
		reg->Save(L"BackGround Image show", isShowBgImage);
		reg->Save(L"BackGround Image", sBgImage);
		reg->Save(L"bgImageDarker", bgImageDarker);
		reg->Save(L"bgImageColors", nBgImageColors);
		reg->Save(L"bgOperation", bgOperation);
		reg->Save(L"bgPluginAllowed", isBgPluginAllowed);
		reg->Save(L"AlphaValue", nTransparent);
		reg->Save(L"AlphaValueSeparate", isTransparentSeparate);
		reg->Save(L"AlphaValueInactive", nTransparentInactive);
		//reg->Save(L"UseColorKey", isColorKey);
		//reg->Save(L"ColorKey", ColorKey);
		reg->Save(L"UserScreenTransparent", isUserScreenTransparent);
		reg->Save(L"ColorKeyTransparent", isColorKeyTransparent);
		reg->Save(L"ColorKeyValue", nColorKeyValue);
		reg->Save(L"UseCurrentSizePos", isUseCurrentSizePos);
		DWORD saveMode = (isUseCurrentSizePos==false) ? _WindowMode // сохранять будем то, что задано пользователем явно
				: ( (ghWnd == NULL)  // иначе - текущее состояние
						? gpConEmu->WindowMode
						: (gpConEmu->isFullScreen() ? rFullScreen : gpConEmu->isZoomed() ? rMaximized : rNormal));
		reg->Save(L"WindowMode", saveMode);
		reg->Save(L"ConWnd Width", isUseCurrentSizePos ? gpConEmu->WndWidth.Raw : wndWidth.Raw);
		reg->Save(L"ConWnd Height", isUseCurrentSizePos ? gpConEmu->WndHeight.Raw : wndHeight.Raw);
		reg->Save(L"Cascaded", wndCascade);
		reg->Save(L"ConWnd X", isUseCurrentSizePos ? gpConEmu->wndX : _wndX);
		reg->Save(L"ConWnd Y", isUseCurrentSizePos ? gpConEmu->wndY : _wndY);
		reg->Save(L"16bit Height", ntvdmHeight);
		reg->Save(L"AutoSaveSizePos", isAutoSaveSizePos);
		mb_SizePosAutoSaved = false; // Раз было инициированное пользователей сохранение настроек - сбросим флажок
		reg->Save(L"IntegralSize", mb_IntegralSize);
		reg->Save(L"QuakeStyle", isQuakeStyle);
		reg->Save(L"QuakeAnimation", nQuakeAnimation);
		reg->Save(L"HideCaption", isHideCaption);
		reg->Save(L"HideChildCaption", isHideChildCaption);
		reg->Save(L"FocusInChildWindows", isFocusInChildWindows);
		reg->Save(L"HideCaptionAlways", mb_HideCaptionAlways);
		reg->Save(L"HideCaptionAlwaysFrame", nHideCaptionAlwaysFrame);
		reg->Save(L"HideCaptionAlwaysDelay", nHideCaptionAlwaysDelay);
		reg->Save(L"HideCaptionAlwaysDisappear", nHideCaptionAlwaysDisappear);
		reg->Save(L"DownShowHiddenMessage", isDownShowHiddenMessage);
		reg->Save(L"ConsoleFontName", ConsoleFont.lfFaceName);
		reg->Save(L"ConsoleFontWidth", ConsoleFont.lfWidth);
		reg->Save(L"ConsoleFontHeight", ConsoleFont.lfHeight);
		reg->Save(L"DefaultBufferHeight", DefaultBufferHeight);
		reg->Save(L"AutoBufferHeight", AutoBufferHeight);
		reg->Save(L"CmdOutputCP", nCmdOutputCP);
		reg->Save(L"ComSpec.Type", (BYTE)ComSpec.csType);
		reg->Save(L"ComSpec.Bits", (BYTE)ComSpec.csBits);
		reg->Save(L"ComSpec.UpdateEnv", (bool)ComSpec.isUpdateEnv);
		reg->Save(L"ComSpec.EnvAddPath", (bool)((ComSpec.AddConEmu2Path & CEAP_AddConEmuBaseDir) == CEAP_AddConEmuBaseDir));
		reg->Save(L"ComSpec.EnvAddExePath", (bool)((ComSpec.AddConEmu2Path & CEAP_AddConEmuExeDir) == CEAP_AddConEmuExeDir));
		reg->Save(L"ComSpec.UncPaths", (bool)ComSpec.isAllowUncPaths);
		reg->Save(L"ComSpec.Path", ComSpec.ComspecExplicit);
		reg->Save(L"ConsoleTextSelection", isConsoleTextSelection);
		reg->Save(L"CTS.AutoCopy", isCTSAutoCopy);
		reg->Save(L"CTS.IBeam", isCTSIBeam);
		reg->Save(L"CTS.EndOnTyping", isCTSEndOnTyping);
		reg->Save(L"CTS.EndOnKeyPress", isCTSEndOnKeyPress);
		reg->Save(L"CTS.Freeze", isCTSFreezeBeforeSelect);
		reg->Save(L"CTS.SelectBlock", isCTSSelectBlock);
		//reg->Save(L"CTS.VkBlock", isCTSVkBlock);
		//reg->Save(L"CTS.VkBlockStart", vmCTSVkBlockStart);
		reg->Save(L"CTS.SelectText", isCTSSelectText);
		//reg->Save(L"CTS.ClickPromptPosition", isCTSClickPromptPosition);
		//reg->Save(L"CTS.VkText", isCTSVkText);
		//reg->Save(L"CTS.VkTextStart", vmCTSVkTextStart);
		reg->Save(L"CTS.ActMode", isCTSActMode);
		//reg->Save(L"CTS.VkAct", isCTSVkAct);
		reg->Save(L"CTS.RBtnAction", isCTSRBtnAction);
		reg->Save(L"CTS.MBtnAction", isCTSMBtnAction);
		reg->Save(L"CTS.ColorIndex", isCTSColorIndex);
		
		reg->Save(L"ClipboardConfirmEnter", isPasteConfirmEnter);
		reg->Save(L"ClipboardConfirmLonger", nPasteConfirmLonger);

		reg->Save(L"FarGotoEditorOpt", isFarGotoEditor);
		reg->Save(L"FarGotoEditorPath", sFarGotoEditor);
		//reg->Save(L"FarGotoEditorVk", isFarGotoEditorVk);
		
		reg->Save(L"FixFarBorders", isFixFarBorders);
		{
		wchar_t* pszCharRanges = CreateCharRanges(mpc_FixFarBorderValues);
		reg->Save(L"FixFarBordersRanges", pszCharRanges ? pszCharRanges : L"2013-25C4");
		if (pszCharRanges) free(pszCharRanges);
		}
		reg->Save(L"ExtendUCharMap", isExtendUCharMap);
		reg->Save(L"EnhanceGraphics", isEnhanceGraphics);
		reg->Save(L"EnhanceButtons", isEnhanceButtons);
		reg->Save(L"PartBrush75", isPartBrush75);
		reg->Save(L"PartBrush50", isPartBrush50);
		reg->Save(L"PartBrush25", isPartBrush25);
		reg->Save(L"PartBrushBlack", isPartBrushBlack);
		reg->Save(L"RightClick opens context menu", isRClickSendKey);
		reg->Save(L"RightClickMacro2", sRClickMacro);
		//reg->Save(L"AltEnter", isSendAltEnter);
		//reg->Save(L"AltSpace", isSendAltSpace);
		reg->Save(L"SendAltTab", isSendAltTab);
		reg->Save(L"SendAltEsc", isSendAltEsc);
		reg->Save(L"SendAltPrintScrn", isSendAltPrintScrn);
		reg->Save(L"SendPrintScrn", isSendPrintScrn);
		reg->Save(L"SendCtrlEsc", isSendCtrlEsc);
		//reg->Save(L"SendAltF9", isSendAltF9);
		reg->Save(L"Min2Tray", mb_MinToTray);
		reg->Save(L"AlwaysShowTrayIcon", isAlwaysShowTrayIcon);
		reg->Save(L"SafeFarClose", isSafeFarClose);
		reg->Save(L"SafeFarCloseMacro", sSafeFarCloseMacro);
		reg->Save(L"FARuseASCIIsort", isFARuseASCIIsort);
		reg->Save(L"ShellNoZoneCheck", isShellNoZoneCheck);
		reg->Save(L"FixAltOnAltTab", isFixAltOnAltTab);
		reg->Save(L"DisableMouse", isDisableMouse);
		reg->Save(L"RSelectionFix", isRSelFix);
		reg->Save(L"MouseSkipActivation", isMouseSkipActivation);
		reg->Save(L"MouseSkipMoving", isMouseSkipMoving);
		reg->Save(L"FarHourglass", isFarHourglass);
		reg->Save(L"FarHourglassDelay", nFarHourglassDelay);
		reg->Save(L"Dnd", isDragEnabled);
		//reg->Save(L"DndLKey", nLDragKey);
		//reg->Save(L"DndRKey", nRDragKey);
		reg->Save(L"DndDrop", isDropEnabled);
		reg->Save(L"DefCopy", isDefCopy);
		reg->Save(L"DropUseMenu", isDropUseMenu);
		reg->Save(L"DragOverlay", isDragOverlay);
		reg->Save(L"DragShowIcons", isDragShowIcons);
		reg->Save(L"DebugSteps", isDebugSteps);
		reg->Save(L"DragPanel", isDragPanel);
		reg->Save(L"DragPanelBothEdges", isDragPanelBothEdges);
		reg->Save(L"KeyBarRClick", isKeyBarRClick);
		//reg->Save(L"GUIpb", isGUIpb);
		SaveStatusSettings(reg);
		reg->Save(L"Tabs", isTabs);
		reg->Save(L"TabsLocation", nTabsLocation);
		reg->Save(L"TabIcons", isTabIcons);
		reg->Save(L"OneTabPerGroup", isOneTabPerGroup);
		reg->Save(L"ActivateSplitMouseOver", isActivateSplitMouseOver);
		reg->Save(L"TabSelf", isTabSelf);
		reg->Save(L"TabLazy", isTabLazy);
		reg->Save(L"TabRecent", isTabRecent);
		reg->Save(L"TabDblClick", nTabBarDblClickAction);
		reg->Save(L"TabBtnDblClick", nTabBtnDblClickAction);
		reg->Save(L"TabsOnTaskBar", m_isTabsOnTaskBar);
		reg->Save(L"TaskBarOverlay", isTaskbarShield);
		reg->Save(L"TabCloseMacro", sTabCloseMacro);
		reg->Save(L"TabFontFace", sTabFontFace);
		reg->Save(L"TabFontCharSet", nTabFontCharSet);
		reg->Save(L"TabFontHeight", nTabFontHeight);
		reg->Save(L"SaveAllEditors", sSaveAllMacro);
		//reg->Save(L"TabFrame", isTabFrame);
		//reg->Save(L"TabMargins", rcTabMargins);
		reg->Save(L"ToolbarAddSpace", nToolbarAddSpace);
		reg->Save(L"TabConsole", szTabConsole);
		reg->Save(L"TabSkipWords", szTabSkipWords);
		reg->Save(L"TabPanels", szTabPanels);
		reg->Save(L"TabEditor", szTabEditor);
		reg->Save(L"TabEditorModified", szTabEditorModified);
		reg->Save(L"TabViewer", szTabViewer);
		reg->Save(L"TabLenMax", nTabLenMax);
		reg->Save(L"AdminTitleSuffix", szAdminTitleSuffix);
		reg->Save(L"AdminShowShield", bAdminShield);
		reg->Save(L"HideInactiveConsoleTabs", bHideInactiveConsoleTabs);
		//reg->Save(L"HideDisabledTabs", bHideDisabledTabs);
		reg->Save(L"ShowFarWindows", bShowFarWindows);
		reg->Save(L"TryToCenter", isTryToCenter);
		reg->Save(L"CenterConsolePad", nCenterConsolePad);
		reg->Save(L"ShowScrollbar", isAlwaysShowScrollbar);
		reg->Save(L"ScrollBarAppearDelay", nScrollBarAppearDelay);
		reg->Save(L"ScrollBarDisappearDelay", nScrollBarDisappearDelay);
		reg->Save(L"IconID", nIconID);
		//reg->Save(L"Update Console handle", isUpdConHandle);
		/*reg->Save(L"ScrollTitle", isScrollTitle);
		reg->Save(L"ScrollTitleLen", ScrollTitleLen);*/
		reg->Save(L"MainTimerElapse", nMainTimerElapse);
		reg->Save(L"MainTimerInactiveElapse", nMainTimerInactiveElapse);
		reg->Save(L"AffinityMask", nAffinity);
		reg->Save(L"SkipFocusEvents", isSkipFocusEvents);
		reg->Save(L"MonitorConsoleLang", isMonitorConsoleLang);
		reg->Save(L"DesktopMode", isDesktopMode);
		reg->Save(L"SnapToDesktopEdges", isSnapToDesktopEdges);
		reg->Save(L"AlwaysOnTop", isAlwaysOnTop);
		reg->Save(L"SleepInBackground", isSleepInBackground);
		reg->Save(L"MinimizeOnLoseFocus", mb_MinimizeOnLoseFocus);
		reg->Save(L"DisableFarFlashing", isDisableFarFlashing);
		reg->Save(L"DisableAllFlashing", isDisableAllFlashing);
		/* FindText: bMatchCase, bMatchWholeWords, bFreezeConsole, bHighlightAll */
		SaveFindOptions(reg);
		/* *********** Thumbnails and Tiles ************* */
		reg->Save(L"PanView.BackColor", ThSet.crBackground.RawColor);
		reg->Save(L"PanView.PFrame", ThSet.nPreviewFrame);
		reg->Save(L"PanView.PFrameColor", ThSet.crPreviewFrame.RawColor);
		reg->Save(L"PanView.SFrame", ThSet.nSelectFrame);
		reg->Save(L"PanView.SFrameColor", ThSet.crSelectFrame.RawColor);
		/* теперь разнообразные размеры */
		ThumbSaveSet(L"Thumbs", ThSet.Thumbs);
		ThumbSaveSet(L"Tiles", ThSet.Tiles);
		// Прочие параметры загрузки
		reg->Save(L"PanView.LoadPreviews", ThSet.bLoadPreviews);
		reg->Save(L"PanView.LoadFolders", ThSet.bLoadFolders);
		reg->Save(L"PanView.LoadTimeout", ThSet.nLoadTimeout);
		reg->Save(L"PanView.MaxZoom", ThSet.nMaxZoom);
		reg->Save(L"PanView.UsePicView2", ThSet.bUsePicView2);
		reg->Save(L"PanView.RestoreOnStartup", ThSet.bRestoreOnStartup);

		/* *** AutoUpdate *** */
		reg->Save(L"Update.VerLocation", UpdSet.szUpdateVerLocation);
		reg->Save(L"Update.CheckOnStartup", UpdSet.isUpdateCheckOnStartup);
		reg->Save(L"Update.CheckHourly", UpdSet.isUpdateCheckHourly);
		reg->Save(L"Update.ConfirmDownload", UpdSet.isUpdateConfirmDownload);
		reg->Save(L"Update.UseBuilds", UpdSet.isUpdateUseBuilds);
		reg->Save(L"Update.UseProxy", UpdSet.isUpdateUseProxy);
		reg->Save(L"Update.Proxy", UpdSet.szUpdateProxy);
		reg->Save(L"Update.ProxyUser", UpdSet.szUpdateProxyUser);
		reg->Save(L"Update.ProxyPassword", UpdSet.szUpdateProxyPassword);
		//reg->Save(L"Update.DownloadSetup", UpdSet.isUpdateDownloadSetup); // 1-Installer (ConEmuSetup.exe), 2-7z archieve (ConEmu.7z), WinRar or 7z required
		reg->Save(L"Update.ExeCmdLine", UpdSet.szUpdateExeCmdLine);
		reg->Save(L"Update.ArcCmdLine", UpdSet.szUpdateArcCmdLine);
		reg->Save(L"Update.DownloadPath", UpdSet.szUpdateDownloadPath);
		reg->Save(L"Update.LeavePackages", UpdSet.isUpdateLeavePackages);
		reg->Save(L"Update.PostUpdateCmd", UpdSet.szUpdatePostUpdateCmd);

		/* Hotkeys */
		SaveHotkeys(reg);

		/* Done */
		reg->CloseKey();

		/* Subsections */
		SaveCmdTasks(reg);
		SaveAppSettings(reg);
		SavePalettes(reg);

		/* Done */
		lbRc = TRUE;
	}

	//}
	delete reg;

	// Вроде и показывать не нужно. Объект уже сам ругнулся
	//MessageBoxA(ghOpWnd, "Failed", "Information", MB_ICONERROR);
	return lbRc;
}

int Settings::StatusBarFontHeight()
{
	return max(4,nStatusFontHeight);
}

int Settings::StatusBarHeight()
{
	int iHeight = StatusBarFontHeight();
	if (isStatusBarFlags & csf_NoVerticalPad)
		iHeight += (isStatusBarFlags & csf_HorzDelim) ? 1 : 0;
	else
		iHeight += (isStatusBarFlags & csf_HorzDelim) ? 2 : 1;
	return iHeight;
}


DWORD Settings::isUseClink(bool abCheckVersion /*= false*/)
{
	if (!mb_UseClink)
		return false;

	wchar_t szClink32[MAX_PATH+30], szClink64[MAX_PATH+30];
	
	wcscpy_c(szClink32, gpConEmu->ms_ConEmuBaseDir);
	wcscat_c(szClink32, L"\\clink\\clink_dll_x86.dll");
	if (!FileExists(szClink32))
		szClink32[0] = 0;

	#ifdef _WIN64
	wcscpy_c(szClink64, gpConEmu->ms_ConEmuBaseDir);
	wcscat_c(szClink64, L"\\clink\\clink_dll_x64.dll");
	if (!FileExists(szClink64))
		szClink64[0] = 0;
	#else
		szClink64[0] = 0;
	#endif

	if (!szClink32[0] && !szClink64[0])
	{
		return false;
	}

	static int nVersionChecked = 0;
	static VS_FIXEDFILEINFO vi = {};

	if (nVersionChecked == 0)
	{
		//DWORD nErrCode;
		//HMODULE hLib = LoadLibraryEx(WIN3264TEST(szClink32,szClink64), NULL, LOAD_LIBRARY_AS_DATAFILE|LOAD_LIBRARY_AS_IMAGE_RESOURCE);
		//if (!hLib)
		//{
		//	nVersionChecked = -1;
		//	nErrCode = GetLastError();
		//	DisplayLastError(L"Failed to load clink library as DataFile", nErrCode);
		//	return false;
		//}
		//FreeLibrary(hLib);

		LPCTSTR pszPath = WIN3264TEST(szClink32,szClink64[0] ? szClink64 : szClink32);

		DWORD dwRsrvd = 0;
		DWORD dwSize = GetFileVersionInfoSize(pszPath, &dwRsrvd);

		if (dwSize == 0)
		{
			nVersionChecked = 1; // clink ver 0.1.1
		}
		else
		{
			void *pVerData = calloc(dwSize, 1);

			if (pVerData)
			{
				VS_FIXEDFILEINFO *lvs = NULL;
				UINT nLen = sizeof(lvs);

				if (GetFileVersionInfo(pszPath, 0, dwSize, pVerData))
				{
					wchar_t szSlash[3]; lstrcpyW(szSlash, L"\\");

					if (VerQueryValue((void*)pVerData, szSlash, (void**)&lvs, &nLen))
					{
						vi = *lvs;
						nVersionChecked = 2;
					}
				}

				free(pVerData);
			}
		}
	}

	if (nVersionChecked != 1 && nVersionChecked != 2)
		return false;
	return (DWORD)nVersionChecked;
}

bool Settings::isKeyboardHooks(bool abNoDisable /*= false*/)
{
	// Если хуки запрещены ключом "/nokeyhooks"
	if (gpConEmu->DisableKeybHooks && !abNoDisable)
		return false;

	return (m_isKeyboardHooks == 0) || (m_isKeyboardHooks == 1);
}

//LPCTSTR Settings::GetCurCmd()
//{
//	return psCurCmd;
//}
//
//void Settings::SetCmdPtr(wchar_t*& psNewCmd)
//{
//	_ASSERTE(psNewCmd!=NULL);
//
//	if (psCurCmd && (psCurCmd != psNewCmd))
//	{
//		SafeFree(psCurCmd);
//	}
//
//	psCurCmd = psNewCmd;
//
//	// Release it
//	psNewCmd = NULL;
//}

RecreateActionParm Settings::GetDefaultCreateAction()
{
	return isMulti ? cra_CreateTab : cra_CreateWindow;
}

void Settings::HistoryCheck()
{
	if (!psCmdHistory || !*psCmdHistory)
	{
		nCmdHistorySize = 0;
	}
	else
	{
		const wchar_t* psz = psCmdHistory;

		while(*psz)
			psz += _tcslen(psz)+1;

		if (psz == psCmdHistory)
			nCmdHistorySize = 0;
		else
			nCmdHistorySize = (psz - psCmdHistory + 1)*sizeof(wchar_t);
	}
}

void Settings::HistoryReset()
{
	SafeFree(psCmdHistory);
	psCmdHistory = (wchar_t*)calloc(2,2);
	nCmdHistorySize = 0;

	// И сразу сохранить в настройках
	SettingsBase* reg = CreateSettings(NULL);
	if (!reg)
	{
		_ASSERTE(reg!=NULL);
		return;
	}

	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
	{
		HEAPVAL;
		reg->SaveMSZ(L"CmdLineHistory", psCmdHistory, nCmdHistorySize);
		HEAPVAL;
		reg->CloseKey();
	}

	delete reg;
}

void Settings::HistoryAdd(LPCWSTR asCmd)
{
	if (!isSaveCmdHistory)
		return;

	// Группы и так отображаются в диалоге/меню. В историю их не пишем
	if (!asCmd || !*asCmd || (*asCmd == TaskBracketLeft))
		return;

	if (psStartSingleApp && lstrcmp(psStartSingleApp, asCmd)==0)
		return;
	if (psStartTasksFile && lstrcmp(psStartTasksFile, asCmd)==0)
		return;
	if (psStartTasksName && lstrcmp(psStartTasksName, asCmd)==0)
		return;

	LPCWSTR psCurCmd = gpSetCls->GetCurCmd();
	if (psCurCmd && lstrcmp(psCurCmd, asCmd)==0)
		return;

	HEAPVAL
	wchar_t *pszNewHistory, *psz;
	int nCount = 0;
	DWORD nCchNewSize = (nCmdHistorySize>>1) + _tcslen(asCmd) + 2;
	DWORD nNewSize = nCchNewSize*2;
	pszNewHistory = (wchar_t*)malloc(nNewSize);

	//wchar_t* pszEnd = pszNewHistory + nNewSize/sizeof(wchar_t);
	if (!pszNewHistory) return;

	_wcscpy_c(pszNewHistory, nCchNewSize, asCmd);
	psz = pszNewHistory + _tcslen(pszNewHistory) + 1;
	nCount++;

	if (psCmdHistory)
	{
		wchar_t* pszOld = psCmdHistory;
		int nLen;
		HEAPVAL;

		while(nCount < MAX_CMD_HISTORY && *pszOld /*&& psz < pszEnd*/)
		{
			const wchar_t *pszCur = pszOld;
			pszOld += _tcslen(pszOld) + 1;

			if (lstrcmp(pszCur, asCmd) == 0)
				continue;

			_wcscpy_c(psz, nCchNewSize-(psz-pszNewHistory), pszCur);
			psz += (nLen = (_tcslen(psz)+1));
			nCount ++;
		}
	}

	*psz = 0;
	HEAPVAL;
	SafeFree(psCmdHistory);
	psCmdHistory = pszNewHistory;
	nCmdHistorySize = (psz - pszNewHistory + 1)*sizeof(wchar_t);
	HEAPVAL;
	// И сразу сохранить в настройках
	SettingsBase* reg = CreateSettings(NULL);
	if (!reg)
	{
		_ASSERTE(reg!=NULL);
		return;
	}

	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
	{
		HEAPVAL;
		reg->SaveMSZ(L"CmdLineHistory", psCmdHistory, nCmdHistorySize);
		HEAPVAL;
		reg->CloseKey();
	}

	delete reg;
}

LPCWSTR Settings::HistoryGet()
{
	if (psCmdHistory && *psCmdHistory)
		return psCmdHistory;

	return NULL;
}

// например, L"2013-25C3,25C4"
// Возвращает 0 - в случае успеха,
// при ошибке - индекс (1-based) ошибочного символа в asRanges
// -1 - ошибка выделения памяти
int Settings::ParseCharRanges(LPCWSTR asRanges, BYTE (&Chars)[0x10000], BYTE abValue /*= TRUE*/)
{
	if (!asRanges)
	{
		_ASSERTE(asRanges!=NULL);
		return -1;
	}
	
	int iRc = 0;
	int n = 0, nMax = _tcslen(asRanges);
	wchar_t *pszCopy = lstrdup(asRanges);
	if (!pszCopy)
	{
		_ASSERTE(pszCopy!=NULL);
		return -1;
	}
	wchar_t *pszRange = pszCopy;
	wchar_t *pszNext = NULL;
	UINT cBegin, cEnd;
	
	memset(Chars, 0, sizeof(Chars));

	while(*pszRange && n < nMax)
	{
		cBegin = (UINT)wcstol(pszRange, &pszNext, 16);
		if (!cBegin || (cBegin > 0xFFFF))
		{
			iRc = (int)(pszRange - asRanges);
			goto wrap;
		}
			
		switch (*pszNext)
		{
		case L';':
		case 0:
			cEnd = cBegin;
			break;
		case L'-':
		case L' ':
			pszRange = pszNext + 1;
			cEnd = (UINT)wcstol(pszRange, &pszNext, 16);
			if ((cEnd < cBegin) || (cEnd > 0xFFFF))
			{
				iRc = (int)(pszRange - asRanges);
				goto wrap;
			}
			break;
		default:
			iRc = (int)(pszNext - asRanges);
			goto wrap;
		}

		for (UINT i = cBegin; i <= cEnd; i++)
			Chars[i] = abValue;
		
		if (*pszNext != L';') break;
		pszRange = pszNext + 1;
	}
	
	iRc = 0; // ok
wrap:
	if (pszCopy)
		free(pszCopy);
	return iRc;
}

// caller must free(result)
wchar_t* Settings::CreateCharRanges(BYTE (&Chars)[0x10000])
{
	size_t nMax = 1024;
	wchar_t* pszRanges = (wchar_t*)calloc(nMax,sizeof(*pszRanges));
	if (!pszRanges)
	{
		_ASSERTE(pszRanges!=NULL);
		return NULL;
	}
	
	wchar_t* psz = pszRanges;
	wchar_t* pszEnd = pszRanges + nMax;
	UINT c = 0;
	_ASSERTE((countof(Chars)-1) == 0xFFFF);
	while (c < countof(Chars))
	{
		if (Chars[c])
		{
			if ((psz + 10) >= pszEnd)
			{
				// Слишком длинный блок
				_ASSERTE((psz + 10) < pszEnd);
				break;
			}
			
			UINT cBegin = (c++);
			UINT cEnd = cBegin;
			
			while (c < countof(Chars) && Chars[c])
			{
				cEnd = (c++);
			}
			
			if (cBegin == cEnd)
			{
				wsprintf(psz, L"%04X;", cBegin);
			}
			else
			{
				wsprintf(psz, L"%04X-%04X;", cBegin, cEnd);
			}
			psz += _tcslen(psz);
		}
		else
		{
			c++;
		}
	}
	
	return pszRanges;
}

bool Settings::isCharBorder(wchar_t inChar)
{
	return mpc_FixFarBorderValues[(WORD)inChar];
}

void Settings::CheckConsoleSettings()
{
	// Если в ключе [HKEY_CURRENT_USER\Console] будут левые значения - то в Win7 могут
	// начаться страшные глюки :-)
	// например, консольное окно будет "дырявое" - рамка есть, а содержимого - нет :-P
	DWORD nFullScr = 0, nFullScrEmu = 0, nSize;
	HKEY hkCon, hkEmu;
	if (!RegOpenKeyEx(HKEY_CURRENT_USER, L"Console", 0, KEY_ALL_ACCESS, &hkCon))
	{
		if (RegQueryValueEx(hkCon, L"FullScreen", NULL, NULL, (LPBYTE)&nFullScr, &(nSize=sizeof(nFullScr))))
			nFullScr = 0;
		if (!RegOpenKeyEx(hkCon, CEC_INITTITLE, 0, KEY_ALL_ACCESS, &hkEmu))
		{
			if (RegQueryValueEx(hkEmu, L"FullScreen", NULL, NULL, (LPBYTE)&nFullScrEmu, &(nSize=sizeof(nFullScrEmu))))
				nFullScrEmu = 0;
			RegCloseKey(hkEmu);
		}
		else
		{
			nFullScrEmu = nFullScr;
		}
		RegCloseKey(hkCon);
	}
	
	if (nFullScr || nFullScrEmu)
	{
		if (gOSVer.dwMajorVersion >= 6)
		{
			wchar_t szWarning[512];
			_wsprintf(szWarning, SKIPLEN(countof(szWarning)) 
				L"Warning!\n"
				L"Dangerous values detected in yours registry\n\n"
				L"Please check [HKEY_CURRENT_USER\\Console] and [HKEY_CURRENT_USER\\Console\\ConEmu] keys\n\n"
				L"\"FullScreen\" recommended value is dword:00000000\n\n"
				L"Current value is dword:%08X",
				nFullScr ? nFullScr : nFullScrEmu);
			MBoxA(szWarning);
		}
	}
}

SettingsBase* Settings::CreateSettings(const SettingsStorage* apStorage)
{
	SettingsBase* pReg = NULL;

	BOOL lbXml = FALSE;
	LPCWSTR pszFile = NULL;

	if (apStorage)
	{
		if (lstrcmp(apStorage->szType, CONEMU_CONFIGTYPE_XML) == 0)
		{
			pszFile = apStorage->pszFile;
			if (!pszFile || !*pszFile)
			{
				DisplayLastError(L"Invalid xml-path was specified!", -1);
				return NULL;
			}
			HANDLE hFile = CreateFile(pszFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE || !hFile)
			{
				DisplayLastError(L"Failed to create xml file!");
				return NULL;
			}
			CloseHandle(hFile);
			lbXml = TRUE;
		}
	}


#ifndef __GNUC__
	DWORD dwAttr = -1;

	if (!apStorage)
	{
		pszFile = gpConEmu->ConEmuXml();
	}

	// добавил lbXml - он мог быть принудительно включен
	if (pszFile && *pszFile && !lbXml)
	{
		dwAttr = GetFileAttributes(pszFile);

		if (dwAttr != (DWORD)-1 && !(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
			lbXml = TRUE;
	}

	if (lbXml)
	{
		SettingsStorage XmlStorage = {};
		XmlStorage.pszFile = pszFile;

		pReg = new SettingsXML(XmlStorage);

		if (!((SettingsXML*)pReg)->IsXmlAllowed())
		{
			// Если MSXml.DomDocument не зарегистрирован
			if (!apStorage)
			{
				// Чтобы не пытаться повторно открыть XML - интерфейс не доступен!
				gpConEmu->ConEmuXml()[0] = 0;
			}
			lbXml = FALSE;
			SafeDelete(pReg);
		}
	}
#endif

	if (lbXml && !pReg && apStorage)
	{
		DisplayLastError(L"XML storage is not available! Check IXMLDOMDocument interface!", -1);
		return NULL;
	}

	if (!pReg)
	{
		pReg = new SettingsRegistry();
	}

	return pReg;
}

void Settings::GetSettingsType(SettingsStorage& Storage, bool& ReadOnly)
{
	const wchar_t* pszType = CONEMU_CONFIGTYPE_REG /*L"[reg]"*/;
	ReadOnly = false;

	ZeroStruct(Storage);

#ifndef __GNUC__
	HANDLE hFile = NULL;
	LPWSTR pszXmlFile = gpConEmu->ConEmuXml();

	if (pszXmlFile && *pszXmlFile)
	{
		hFile = CreateFile(pszXmlFile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
		                   NULL, OPEN_EXISTING, 0, 0);

		// XML-файл есть
		if (hFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hFile); hFile = NULL;

			if (SettingsXML::IsXmlAllowed())
			{
				pszType = CONEMU_CONFIGTYPE_XML /*L"[xml]"*/;
				Storage.pszFile = pszXmlFile;

				// Проверим, сможем ли мы в него записать
				hFile = CreateFile(pszXmlFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
				                   NULL, OPEN_EXISTING, 0, 0);

				if (hFile != INVALID_HANDLE_VALUE)
				{
					CloseHandle(hFile); hFile = NULL; // OK
				}
				else
				{
					DWORD nErr = GetLastError();
					//EnableWindow(GetDlgItem(ghOpWnd, bSaveSettings), FALSE); // Сохранение запрещено
					ReadOnly = true;
					UNREFERENCED_PARAMETER(nErr);
				}
			}
		}
	}
#endif

	wcscpy_c(Storage.szType, pszType);
	Storage.pszConfig = gpSetCls->GetConfigName();
}

void Settings::SetHideCaptionAlways(bool bHideCaptionAlways)
{
	mb_HideCaptionAlways = bHideCaptionAlways;
}

void Settings::SwitchHideCaptionAlways()
{
	mb_HideCaptionAlways = !mb_HideCaptionAlways;
}

bool Settings::isHideCaptionAlways()
{
	return mb_HideCaptionAlways || (!mb_HideCaptionAlways && isUserScreenTransparent) || isQuakeStyle || gpConEmu->mp_Inside;
}

bool Settings::isForcedHideCaptionAlways()
{
	return (isUserScreenTransparent || isQuakeStyle);
}

bool Settings::isMinimizeOnLoseFocus()
{
	return (isQuakeStyle == 2) || (isQuakeStyle == 0 && mb_MinimizeOnLoseFocus);
}

bool Settings::isMinToTray(bool bRawOnly /*= false*/)
{
	// Сворачивать в TSA только если включен флажок. Юзер его мог принудительно отключить.
	return (mb_MinToTray || /*(m_isTabsOnTaskBar == 3) ||*/ gpConEmu->ForceMinimizeToTray);
}

void Settings::SetMinToTray(bool bMinToTray)
{
	mb_MinToTray = bMinToTray;

	if (ghOpWnd && gpSetCls->mh_Tabs[CSettings::thi_Taskbar])
	{
		gpSetCls->checkDlgButton(gpSetCls->mh_Tabs[CSettings::thi_Taskbar], cbMinToTray, mb_MinToTray);
	}
}

bool Settings::isCaptionHidden(ConEmuWindowMode wmNewMode /*= wmCurrent*/)
{
	bool bCaptionHidden = isHideCaptionAlways(); // <== Quake & UserScreen here.
	if (!bCaptionHidden)
	{
		if (wmNewMode == wmCurrent || wmNewMode == wmNotChanging)
			wmNewMode = gpConEmu->WindowMode;

		bCaptionHidden = (wmNewMode == wmFullScreen)
				|| ((wmNewMode == wmMaximized) && isHideCaption);
	}
	return bCaptionHidden;
}

// Функция НЕ учитывает isCaptionHidden.
// Возвращает true, если 'Frame width' меньше системной для ThickFame
// иначе - false, меняем рамку на "NonResizable"
bool Settings::isFrameHidden()
{
	if (!nHideCaptionAlwaysFrame || isQuakeStyle)
		return true;
	if (nHideCaptionAlwaysFrame > 0x7F)
		return false; // sure

	// otherwise - need to check system settings
	UINT nSysFrame = GetSystemMetrics(SM_CXSIZEFRAME);
	if (nSysFrame > nHideCaptionAlwaysFrame)
		return true;

	return false;
}

int Settings::GetAppSettingsId(LPCWSTR asExeAppName, bool abElevated)
{
	if (!asExeAppName || !*asExeAppName || !Apps || (AppCount < 1))
		return -1;

	wchar_t szLower[MAX_PATH]; lstrcpyn(szLower, asExeAppName, countof(szLower));
	CharLowerBuff(szLower, lstrlen(szLower));

	for (int i = 0; i < AppCount; i++)
	{
		if (Apps[i]->Elevated == 1)
		{
			if (!abElevated)
				continue;
		}
		else if (Apps[i]->Elevated == 2)
		{
			if (abElevated)
				continue;
		}

		wchar_t* pch = Apps[i]->AppNamesLwr;
		if (!pch || !*pch)
		{
			_ASSERTE((pch && *pch) || (ghOpWnd && IsWindowVisible(gpSetCls->mh_Tabs[CSettings::thi_Apps])));
			continue;
		}

		if (lstrcmp(pch, L"*") == 0)
		{
			return i;
		}
		
		while (*pch)
		{
			wchar_t* pName = szLower;
			while (*pName && *pch && (*pName == *pch))
			{
				pName++; pch++;
			}

			if (!*pName)
			{
				if (!*pch || (*pch == L'|'))
					return i;
			}

			pch = wcschr(pch, L'|');
			if (pch)
			{
				pch++;
				continue; // перечислено несколько допустимых имен
			}
			else
			{
				break; // не совпало
			}
		}
	}
	
	return -1;
}

const Settings::AppSettings* Settings::GetAppSettings(int anAppId/*=-1*/)
{
	if ((anAppId < 0) || (anAppId >= AppCount))
	{
		_ASSERTE(!((anAppId < -1) || (anAppId > AppCount))); // - иначе здесь должен быть валидный индекс для Apps/AppColors
		if (AppStd.AppNames != NULL)
		{
			_ASSERTE(AppStd.AppNames == NULL);
			AppStd.AppNames = NULL;
		}
		if (AppStd.AppNamesLwr != NULL)
		{
			_ASSERTE(AppStd.AppNamesLwr == NULL);
			AppStd.AppNamesLwr = NULL;
		}
		return &AppStd;
	}

	return Apps[anAppId];
}

Settings::AppSettings* Settings::GetAppSettingsPtr(int anAppId, BOOL abCreateNew /*= FALSE*/)
{
	if ((anAppId == AppCount) && abCreateNew)
	{
		_ASSERTE(gpConEmu->isMainThread());
		int NewAppCount = AppCount+1;
		AppSettings** NewApps = (AppSettings**)calloc(NewAppCount, sizeof(*NewApps));
		//CEAppColors** NewAppColors = (CEAppColors**)calloc(NewAppCount, sizeof(*NewAppColors));
		if (!NewApps /*|| !NewAppColors*/)
		{
			//_ASSERTE(NewApps && NewAppColors);
			_ASSERTE(NewApps);
			return NULL;
		}
		if (Apps && (AppCount > 0))
		{
			memmove(NewApps, Apps, AppCount*sizeof(*NewApps));
			//memmove(NewAppColors, AppColors, AppCount*sizeof(*NewAppColors));
		}
		AppSettings** pOld = Apps;
		//CEAppColors** pOldColors = AppColors;
		Apps = NewApps;
		//AppColors = NewAppColors;
		SafeFree(pOld);
		//SafeFree(pOldColors);
		
		Apps[anAppId] = (AppSettings*)calloc(1,sizeof(AppSettings));
		//AppColors[anAppId] = (CEAppColors*)calloc(1,sizeof(CEAppColors));

		if (!Apps[anAppId] /*|| !AppColors[anAppId]*/)
		{
			_ASSERTE(Apps[anAppId]!=NULL /*&& AppColors[anAppId]!=NULL*/);
			return NULL;
		}
		Apps[anAppId]->cchNameMax = MAX_PATH;
		Apps[anAppId]->AppNames = (wchar_t*)calloc(Apps[anAppId]->cchNameMax,sizeof(wchar_t));
		Apps[anAppId]->AppNamesLwr = (wchar_t*)calloc(Apps[anAppId]->cchNameMax,sizeof(wchar_t));

		AppCount = NewAppCount;
	}

	if ((anAppId < 0) || (anAppId >= AppCount))
	{
		_ASSERTE(!((anAppId < 0) || (anAppId > AppCount)));
		return NULL;
	}

	return Apps[anAppId];
}

void Settings::AppSettingsDelete(int anAppId)
{
	_ASSERTE(gpConEmu->isMainThread())
	if (!Apps /*|| !AppColors*/ || (anAppId < 0) || (anAppId >= AppCount))
	{
		_ASSERTE(Apps /*&& AppColors*/ && (anAppId >= 0) && (anAppId < AppCount));
		return;
	}

	AppSettings* pOld = Apps[anAppId];
	//CEAppColors* pOldClr = AppColors[anAppId];

	for (int i = anAppId+1; i < AppCount; i++)
	{
		Apps[i-1] = Apps[i];
		//AppColors[i-1] = AppColors[i];
	}

	_ASSERTE(AppCount>0);
	AppCount--;

	if (pOld)
	{
		pOld->FreeApps();
		free(pOld);
	}
	//SafeFree(pOldClr);
}

// 0-based, index of Apps
bool Settings::AppSettingsXch(int anIndex1, int anIndex2)
{
	if (((anIndex1 < 0) || (anIndex1 >= AppCount))
		|| ((anIndex2 < 0) || (anIndex2 >= AppCount))
		|| !CmdTasks)
	{
		return false;
	}

	AppSettings* p = Apps[anIndex1];
	Apps[anIndex1] = Apps[anIndex2];
	Apps[anIndex2] = p;

	//CEAppColors* pClr = AppColors[anIndex1];
	//AppColors[anIndex1] = AppColors[anIndex2];
	//AppColors[anIndex2] = pClr;

	return true;
}

void Settings::ResetFadeColors()
{
	mn_LastFadeSrc = mn_LastFadeDst = -1;
	mb_FadeInitialized = false;

	for (int i = 0; i < PaletteCount; i++)
	{
		if (Palettes[i])
		{
			Palettes[i]->FadeInitialized = false;
		}
	}
}

COLORREF* Settings::GetColors(int anAppId/*=-1*/, BOOL abFade)
{
	COLORREF *pColors = Colors;
	COLORREF *pColorsFade = ColorsFade;
	bool* pbFadeInitialized = &mb_FadeInitialized;
	if ((anAppId >= 0) && (anAppId < AppCount) && Apps[anAppId]->OverridePalette && Apps[anAppId]->szPaletteName[0])
	{
		ColorPalette* palPtr = PaletteGetPtr(Apps[anAppId]->GetPaletteIndex());
		_ASSERTE(palPtr && countof(Colors)==countof(palPtr->Colors) && countof(ColorsFade)==countof(palPtr->ColorsFade));

		pColors = palPtr->Colors;
		pColorsFade = palPtr->ColorsFade;
		pbFadeInitialized = &palPtr->FadeInitialized;
	}

	if (!abFade || !isFadeInactive)
		return pColors;

	if (!*pbFadeInitialized)
	{
		// Для ускорения, при повторных запросах, GetFadeColor кеширует результат
		mn_LastFadeSrc = mn_LastFadeDst = -1;

		// Валидация
		if (((int)mn_FadeHigh - (int)mn_FadeLow) < 64)
		{
			mn_FadeLow = DEFAULT_FADE_LOW; mn_FadeHigh = DEFAULT_FADE_HIGH;
		}

		// Prepare
		mn_FadeMul = mn_FadeHigh - mn_FadeLow;
		*pbFadeInitialized = true;

		// Посчитать "затемненные" цвета
		for (size_t i = 0; i < countof(ColorsFade); i++)
		{
			pColorsFade[i] = GetFadeColor(pColors[i]);
		}
	}

	return pColorsFade;
}

COLORREF Settings::GetFadeColor(COLORREF cr)
{
	if (!isFadeInactive)
		return cr;
		
	if (cr == mn_LastFadeSrc)
		return mn_LastFadeDst;

	MYCOLORREF mcr, mcrFade = {}; mcr.color = cr;

	//-нафиг
	//if (!mb_FadeInitialized)
	//{
	//	GetColors(TRUE);
	//}

	mcrFade.rgbRed = GetFadeColorItem(mcr.rgbRed);
	mcrFade.rgbGreen = GetFadeColorItem(mcr.rgbGreen);
	mcrFade.rgbBlue = GetFadeColorItem(mcr.rgbBlue);

	mn_LastFadeSrc = cr;
	mn_LastFadeDst = mcrFade.color;
	
	return mcrFade.color;
}

BYTE Settings::GetFadeColorItem(BYTE c)
{
	DWORD nRc;

	switch(c)
	{
		case 0:
			return mn_FadeLow;
		case 255:
			return mn_FadeHigh;
		default:
			nRc = ((((DWORD)c) + mn_FadeLow) * mn_FadeHigh) >> 8;

			if (nRc >= 255)
			{
				//_ASSERTE(nRc <= 255); -- такие (mn_FadeLow&mn_FadeHigh) пользователь в настройке мог задать
				return 255;
			}

			return (BYTE)nRc;
	}
}

bool Settings::NeedDialogDetect()
{
	// 100331 теперь оно нужно и для PanelView
	return true;
	//return (isUserScreenTransparent || !isMonospace);
}

bool Settings::IsModifierPressed(int nDescrID, bool bAllowEmpty)
{
	DWORD vk = ConEmuHotKey::GetHotkey(GetHotkeyById(nDescrID));

	// если НЕ 0 - должен быть нажат
	if (vk)
	{
		if (!isPressed(vk))
			return false;
	}
	else if (!bAllowEmpty)
	{
		return false;
	}

	// но другие модификаторы нажаты быть не должны!
	if (vk != VK_SHIFT && vk != VK_LSHIFT && vk != VK_RSHIFT)
	{
		if (isPressed(VK_SHIFT))
			return false;
	}

	if (vk != VK_MENU && vk != VK_LMENU && vk != VK_RMENU)
	{
		if (isPressed(VK_MENU))
			return false;
	}

	if (vk != VK_CONTROL && vk != VK_LCONTROL && vk != VK_RCONTROL)
	{
		if (isPressed(VK_CONTROL))
			return false;
	}

	// Можно
	return true;
}

bool Settings::NeedCreateAppWindow()
{
	// Если просили не показывать на таскбаре - создать скрытое родительское окно
	// m_isTabsOnTaskBar: 0 - ConEmu only, 1 - all tabs & all OS, 2 - all tabs & Win 7, 3 - DON'T SHOW
	if (m_isTabsOnTaskBar == 3)
		return true;

	// Пока что, окно для Application нужно создавать только для XP и ниже
	// в том случае, если на таскбаре отображаются кнопки запущенных консолей
	// Это для того, чтобы при Alt-Tab не светилась "лишняя" иконка главного окна
	if (!IsWindows7 && isTabsOnTaskBar())
		return true;
	return false;
}

// Показывать табы на таскбаре? (для каждой консоли - своя кнопка)
bool Settings::isTabsOnTaskBar()
{
	if (isDesktopMode || (m_isTabsOnTaskBar == 3) || gpConEmu->mp_Inside)
		return false;
	if ((m_isTabsOnTaskBar == 1) || ((m_isTabsOnTaskBar == 2) && IsWindows7))
		return true;
	return false;
}

// Показывать ConEmu (ghWnd) на таскбаре
bool Settings::isWindowOnTaskBar(bool bStrictOnly /*= false*/)
{
	// m_isTabsOnTaskBar: 0 - ConEmu only, 1 - all tabs & all OS, 2 - all tabs & Win 7, 3 - DON'T SHOW

	if (isDesktopMode || (m_isTabsOnTaskBar == 3) || gpConEmu->mp_Inside)
		return false;

	if (bStrictOnly)
		return true;

	if (IsWindows7)
	{
		// Реализовано через API таскбара
		if ((m_isTabsOnTaskBar == 1) || (m_isTabsOnTaskBar == 2))
			return true;
	}
	else
	{
		// в старых OS - нужно создавать фейковые окна, а ghWnd на таскбаре не показывать
		if (m_isTabsOnTaskBar == 1)
			return false;
	}
	return true;
}

bool Settings::isRClickTouchInvert()
{
	return gpConEmu->IsGesturesEnabled() && (isRClickSendKey == 2);
}

#define IsLuaMacroOk(psz,def) ((fmv != fmv_Lua) || (lstrcmpi(psz,def) != 0))

LPCWSTR Settings::RClickMacro(FarMacroVersion fmv)
{
	if (sRClickMacro && *sRClickMacro && IsLuaMacroOk(sRClickMacro,FarRClickMacroDefault2))
		return sRClickMacro;
	return RClickMacroDefault(fmv);
}

LPCWSTR Settings::RClickMacroDefault(FarMacroVersion fmv)
{
	// L"@$If (!CmdLine.Empty) %Flg_Cmd=1; %CmdCurPos=CmdLine.ItemCount-CmdLine.CurPos+1; %CmdVal=CmdLine.Value; Esc $Else %Flg_Cmd=0; $End $Text \"rclk_gui:\" Enter $If (%Flg_Cmd==1) $Text %CmdVal %Flg_Cmd=0; %Num=%CmdCurPos; $While (%Num!=0) %Num=%Num-1; CtrlS $End $End"
	static LPCWSTR pszDefaultMacro = FarRClickMacroDefault2;
	static LPCWSTR pszDefaultMacroL = FarRClickMacroDefault3;
	return (fmv == fmv_Lua) ? pszDefaultMacroL : pszDefaultMacro;
}

LPCWSTR Settings::SafeFarCloseMacro(FarMacroVersion fmv)
{
	if (sSafeFarCloseMacro && *sSafeFarCloseMacro && IsLuaMacroOk(sSafeFarCloseMacro,FarSafeCloseMacroDefault2))
		return sSafeFarCloseMacro;
	return SafeFarCloseMacroDefault(fmv);
}

LPCWSTR Settings::SafeFarCloseMacroDefault(FarMacroVersion fmv)
{
	// L"@$while (Dialog||Editor||Viewer||Menu||Disks||MainMenu||UserMenu||Other||Help) $if (Editor) ShiftF10 $else Esc $end $end  Esc  $if (Shell) F10 $if (Dialog) Enter $end $Exit $end  F10"
	static LPCWSTR pszDefaultMacro = FarSafeCloseMacroDefault2;
	static LPCWSTR pszDefaultMacroL = FarSafeCloseMacroDefault3;
	return (fmv == fmv_Lua) ? pszDefaultMacroL : pszDefaultMacro;
}

LPCWSTR Settings::TabCloseMacro(FarMacroVersion fmv)
{
	if (sTabCloseMacro && *sTabCloseMacro && IsLuaMacroOk(sTabCloseMacro,FarTabCloseMacroDefault2))
		return sTabCloseMacro;
	return TabCloseMacroDefault(fmv);
}

LPCWSTR Settings::TabCloseMacroDefault(FarMacroVersion fmv)
{
	// L"@$if (Shell) F10 $if (Dialog) Enter $end $else F10 $end";
	static LPCWSTR pszDefaultMacro = FarTabCloseMacroDefault2;
	static LPCWSTR pszDefaultMacroL = FarTabCloseMacroDefault3;
	return (fmv == fmv_Lua) ? pszDefaultMacroL : pszDefaultMacro;
}

LPCWSTR Settings::SaveAllMacro(FarMacroVersion fmv)
{
	if (sSaveAllMacro && *sSaveAllMacro && IsLuaMacroOk(sSaveAllMacro,FarSaveAllMacroDefault2))
		return sSaveAllMacro;
	return SaveAllMacroDefault(fmv);
}

LPCWSTR Settings::SaveAllMacroDefault(FarMacroVersion fmv)
{
	// L"@F2 $If (!Editor) $Exit $End %i0=-1; F12 %cur = CurPos; Home Down %s = Menu.Select(\" * \",3,2); $While (%s > 0) $If (%s == %i0) MsgBox(\"FAR SaveAll\",\"Asterisk in menuitem for already processed window\",0x10001) $Exit $End Enter $If (Editor) F2 $If (!Editor) $Exit $End $Else $If (!Viewer) $Exit $End $End %i0 = %s; F12 %s = Menu.Select(\" * \",3,2); $End $If (Menu && Title==\"Screens\") Home $Rep (%cur-1) Down $End Enter $End $Exit"
	static LPCWSTR pszDefaultMacro = FarSaveAllMacroDefault2;
	static LPCWSTR pszDefaultMacroL = FarSaveAllMacroDefault3;
	return (fmv == fmv_Lua) ? pszDefaultMacroL : pszDefaultMacro;
}

#undef IsLuaMacroOk

const Settings::CommandTasks* Settings::CmdTaskGet(int anIndex)
{
	if (anIndex == -1)
	{
		_ASSERTE(StartupTask!=NULL);
		// Создать!
		return StartupTask;
	}

	if (!CmdTasks || (anIndex < 0) || (anIndex >= CmdTaskCount))
		return NULL;

	return (CmdTasks[anIndex]);
}

// anIndex - 0-based, index of CmdTasks
// asName - имя, или NULL, если эту группу нужно удалить (хвост сдвигается вверх)
// asCommands - список команд (скрипт)
void Settings::CmdTaskSet(int anIndex, LPCWSTR asName, LPCWSTR asGuiArgs, LPCWSTR asCommands)
{
	if (anIndex < 0)
	{
		_ASSERTE(anIndex>=0);
		return;
	}

	if (CmdTasks && (asName == NULL))
	{
		// Грохнуть ту, что просили
		CmdTasks[anIndex]->FreePtr();
		// Сдвинуть хвост
		for (int i = (anIndex+1); i < CmdTaskCount; i++)
		{
			CmdTasks[i-1] = CmdTasks[i];
		}
		// Уменьшить количество
		if (CmdTaskCount > 0)
		{
			CmdTasks[CmdTaskCount-1] = NULL;
			CmdTaskCount--;
		}
		return;
	}

	if (anIndex >= CmdTaskCount)
	{
		CommandTasks** ppNew = (CommandTasks**)calloc(anIndex+1,sizeof(CommandTasks*));
		if (!ppNew)
		{
			_ASSERTE(ppNew!=NULL);
			return;
		}
		if ((CmdTaskCount > 0) && CmdTasks)
		{
			memmove(ppNew, CmdTasks, CmdTaskCount*sizeof(CommandTasks*));
		}
		SafeFree(CmdTasks);
		CmdTasks = ppNew;

		// CmdTaskCount накручивается в конце функции
	}

	if (CmdTasks[anIndex] == NULL)
	{
		CmdTasks[anIndex] = (CommandTasks*)calloc(1, sizeof(CommandTasks));
		if (!CmdTasks[anIndex])
		{
			_ASSERTE(CmdTasks[anIndex]);
			return;
		}
	}

	CmdTasks[anIndex]->SetName(asName, anIndex);
	CmdTasks[anIndex]->SetGuiArg(asGuiArgs);
	CmdTasks[anIndex]->SetCommands(asCommands);

	if (anIndex >= CmdTaskCount)
		CmdTaskCount = anIndex+1;
}

bool Settings::CmdTaskXch(int anIndex1, int anIndex2)
{
	if (((anIndex1 < 0) || (anIndex1 >= CmdTaskCount))
		|| ((anIndex2 < 0) || (anIndex2 >= CmdTaskCount))
		|| !CmdTasks)
	{
		return false;
	}

	Settings::CommandTasks* p = CmdTasks[anIndex1];
	CmdTasks[anIndex1] = CmdTasks[anIndex2];
	CmdTasks[anIndex2] = p;

	return true;
}

// "\0" delimited
const wchar_t* Settings::GetDefaultTerminalAppsMSZ()
{
	return psDefaultTerminalApps;
}

// "|" delimited
wchar_t* Settings::GetDefaultTerminalApps()
{
	if (!psDefaultTerminalApps || !*psDefaultTerminalApps)
	{
		return lstrdup(L"");
	}
	// Evaluate required len
	INT_PTR nTotalLen = 0, nLen;
	const wchar_t* psz = psDefaultTerminalApps;
	while (*psz)
	{
		nLen = _tcslen(psz)+1;
		psz += nLen;
		nTotalLen += nLen;
	}
	// Buffer
	wchar_t* pszRet = (wchar_t*)malloc((nTotalLen+1)*sizeof(*pszRet));
	if (!pszRet)
	{
		_ASSERTE(pszRet);
		return lstrdup(L"");
	}
	// Conversion
	wchar_t* pszDst = pszRet; psz = psDefaultTerminalApps;
	while (*psz)
	{
		nLen = _tcslen(psz);
		wmemmove(pszDst, psz, nLen);
		psz += nLen+1;
		pszDst += nLen;
		if (*psz)
		{
			*(pszDst++) = L'|';
		}
		else
		{
			*(pszDst++) = 0;
			break;
		}
	}
	*pszDst = 0;

	return pszRet;
}
// "|" delimited
void Settings::SetDefaultTerminalApps(const wchar_t* apszApps)
{
	SafeFree(psDefaultTerminalApps);
	if (!apszApps || !*apszApps)
	{
		_ASSERTE(apszApps && *apszApps);
		apszApps = DEFAULT_TERMINAL_APPS/*L"explorer.exe"*/;
	}

	// "|" delimited String -> MSZ
	INT_PTR nLen = _tcslen(apszApps);
	if (nLen > 0)
	{
		wchar_t* pszDst = (wchar_t*)malloc((nLen+3)*sizeof(*pszDst));

		if (pszDst)
		{
			wchar_t* psz = pszDst;
			while (*apszApps)
			{
				const wchar_t* pszNext = wcschr(apszApps, L'|');
				if (!pszNext) pszNext = apszApps + _tcslen(apszApps);
				
				if (pszNext > apszApps)
				{
					wchar_t* pszLwr = psz;
					wmemmove(psz, apszApps, pszNext-apszApps);
					psz += pszNext-apszApps;
					*(psz++) = 0;
					CharLowerBuff(pszLwr, pszNext-apszApps);
				}

				if (!*pszNext)
					break;
				apszApps = pszNext + 1;
			}
			*(psz++) = 0;
			*(psz++) = 0; // для гарантии

			psDefaultTerminalApps = pszDst;
		}
	}

	if (gpConEmu)
	{
		gpConEmu->OnDefaultTermChanged();
	}
}



// Вернуть заданный VkMod, или 0 если не задан
// nDescrID = vkXXX (e.g. vkMinimizeRestore)
DWORD Settings::GetHotkeyById(int nDescrID, const ConEmuHotKey** ppHK)
{
	if (!gpSetCls || !gpSetCls->m_HotKeys)
	{
		_ASSERTE(gpSetCls && gpSetCls->m_HotKeys);
		return 0;
	}

	static int iLastFound = -1;

	for (int j = -1;; j++)
	{
		ConEmuHotKey *pHK;

		if (j == -1)
		{
			if (iLastFound == -1)
				continue;
			else
				pHK = (gpSetCls->m_HotKeys + iLastFound);
		}
		else
		{
			pHK = (gpSetCls->m_HotKeys + j);
		}

		if (!pHK->DescrLangID)
			break;

		if (pHK->DescrLangID == nDescrID)
		{
			iLastFound = (int)(pHK - gpSetCls->m_HotKeys);
			DWORD VkMod = pHK->VkMod;
			if (pHK->HkType == chk_Modifier)
			{
				_ASSERTE(VkMod == ConEmuHotKey::GetHotkey(VkMod));
				VkMod = ConEmuHotKey::GetHotkey(VkMod); // младший байт
			}

			if (ppHK)
				*ppHK = pHK;
			return VkMod;
		}
	}

	return 0;
}

// Проверить, задан ли этот hotkey
// nDescrID = vkXXX (e.g. vkMinimizeRestore)
bool Settings::IsHotkey(int nDescrID)
{
	DWORD nVk = ConEmuHotKey::GetHotkey(GetHotkeyById(nDescrID));
	return (nVk != 0);
}

// Установить новый hotkey
// nDescrID = vkXXX (e.g. vkMinimizeRestore)
// VkMod = LOBYTE - VK, старшие три байта - модификаторы (тоже VK)
void Settings::SetHotkeyById(int nDescrID, DWORD VkMod)
{
	if (!gpSetCls || !gpSetCls->m_HotKeys)
	{
		_ASSERTE(gpSetCls && gpSetCls->m_HotKeys);
		return;
	}

	for (ConEmuHotKey *ppHK = gpSetCls->m_HotKeys; ppHK->DescrLangID; ++ppHK)
	{
		if (ppHK->DescrLangID == nDescrID)
		{
			ppHK->VkMod = VkMod;
			break;
		}
	}

	CheckHotkeyUnique();
}

bool Settings::isModifierExist(BYTE Mod/*VK*/, bool abStrictSingle /*= false*/)
{
	if (!gpSetCls || !gpSetCls->m_HotKeys)
	{
		_ASSERTE(gpSetCls && gpSetCls->m_HotKeys);
		return false;
	}

	for (ConEmuHotKey *ppHK = gpSetCls->m_HotKeys; ppHK->DescrLangID; ++ppHK)
	{
		if (ppHK->VkMod == 0)
			continue;

		if (ppHK->HkType == chk_Modifier)
		{
			if (ConEmuHotKey::GetHotkey(ppHK->VkMod) == Mod)
				return true;
		}
		else if (!abStrictSingle)
		{
			if (ConEmuHotKey::HasModifier(ppHK->VkMod, Mod))
				return true;
		}
		else
		{
			if ((ConEmuHotKey::GetModifier(ppHK->VkMod, 1) == Mod) && !ConEmuHotKey::GetModifier(ppHK->VkMod, 2))
				return true;
		}
	}

	return false;
}

// Есть ли такой хоткей или модификатор (актуально для VK_APPS)
bool Settings::isKeyOrModifierExist(BYTE Mod/*VK*/)
{
	if (!gpSetCls || !gpSetCls->m_HotKeys)
	{
		_ASSERTE(gpSetCls && gpSetCls->m_HotKeys);
		return false;
	}

	for (ConEmuHotKey *ppHK = gpSetCls->m_HotKeys; ppHK->DescrLangID; ++ppHK)
	{
		if (ppHK->VkMod == 0)
			continue;

		if (ppHK->HkType == chk_Modifier)
			continue; // эти не рассматриваем

		if ((ConEmuHotKey::GetHotkey(ppHK->VkMod) == Mod) || ConEmuHotKey::HasModifier(ppHK->VkMod, Mod))
			return true;
	}

	return false;
}

void Settings::LoadHotkeys(SettingsBase* reg)
{
	if (!gpSetCls || !gpSetCls->m_HotKeys)
	{
		_ASSERTE(gpSetCls && gpSetCls->m_HotKeys);
		return;
	}

	reg->Load(L"Multi.Modifier", nHostkeyNumberModifier); ConEmuHotKey::TestHostkeyModifiers(nHostkeyNumberModifier);
	nHostkeyArrowModifier = nHostkeyArrowModifier; // Умолчание - то же что и "Multi.Modifier"
	reg->Load(L"Multi.ArrowsModifier", nHostkeyArrowModifier); ConEmuHotKey::TestHostkeyModifiers(nHostkeyArrowModifier);

	wchar_t szMacroName[80];

	// Compatibility with old settings format
	BYTE MacroVersion = 0;
	reg->Load(L"KeyMacroVersion", MacroVersion);

	for (ConEmuHotKey *ppHK = gpSetCls->m_HotKeys; ppHK->DescrLangID; ++ppHK)
	{
		ppHK->NotChanged = true;

		if (!*ppHK->Name)
		{
			continue;
		}
		
		// Эти модификаторы раньше в настройке не сохранялись, для совместимости - добавляем VK_SHIFT к предыдущей
		switch (ppHK->DescrLangID)
		{
		case vkMultiNewShift:
			_ASSERTE((*(ppHK-1)).DescrLangID == vkMultiNew);
			ppHK->VkMod = ConEmuHotKey::SetModifier((*(ppHK-1)).VkMod,VK_SHIFT, true/*Xor*/);
			break;
		case vkMultiNextShift:
			_ASSERTE((*(ppHK-1)).DescrLangID == vkMultiNext);
			ppHK->VkMod = ConEmuHotKey::SetModifier((*(ppHK-1)).VkMod,VK_SHIFT, true/*Xor*/);
			break;
		}

		_ASSERTE(ppHK->HkType != chk_NumHost);
		_ASSERTE(ppHK->HkType != chk_ArrHost);
		_ASSERTE(ppHK->HkType != chk_System);

		if (reg->Load(ppHK->Name, ppHK->VkMod))
		{
			// Чтобы знать, что комбинация уже сохранялась в настройке ранее
			ppHK->NotChanged = false;
		}

		if (ppHK->HkType != chk_Modifier)
		{
			// Если это НЕ 0 (0 - значит HotKey не задан)
			// И это не DWORD (для HotKey без модификатора - пишется CEHOTKEY_NOMOD)
			// Т.к. раньше был Byte - нужно добавить nHostkeyModifier
			if (ppHK->VkMod && ((ppHK->VkMod & CEHOTKEY_MODMASK) == 0))
			{
				// Добавляем тот модификатор, который был раньше общий
				_ASSERTE(nHostkeyNumberModifier!=0);
				ppHK->VkMod |= (nHostkeyNumberModifier << 8);
			}
		}

		if (ppHK->HkType == chk_Macro)
		{
			wcscpy_c(szMacroName, ppHK->Name);
			wcscat_c(szMacroName, L".Text");
			_ASSERTE(ppHK->GuiMacro == NULL);
			wchar_t* pszMacro = NULL;
			reg->Load(szMacroName, &pszMacro);
			if (MacroVersion < GUI_MACRO_VERSION)
			{
				ppHK->GuiMacro = CConEmuMacro::ConvertMacro(pszMacro, MacroVersion, true);
				SafeFree(pszMacro);
			}
			else
			{
				ppHK->GuiMacro = pszMacro;
			}
		}
	}
}

void Settings::CheckHotkeyUnique()
{
	// Проверка уникальности
	wchar_t* pszFailMsg = NULL;

	// Некоторые хоткеи имеют "локальное" действие
	// А некоторые проверять не хочется
	int SkipCheckID[] = {vkCtrlTab_Left, vkCtrlTab_Up, vkCtrlTab_Right, vkCtrlTab_Down, vkEscNoConsoles};
	bool bSkip = false;

	// Go
	for (ConEmuHotKey *ppHK1 = gpSetCls->m_HotKeys; ppHK1[0].DescrLangID && ppHK1[1].DescrLangID; ++ppHK1)
	{
		if ((ppHK1->HkType == chk_Modifier) || (ConEmuHotKey::GetHotkey(ppHK1->VkMod) == 0))
			continue;

		// Некоторые хоткеи не проверять
		bSkip = false;
		for (size_t i = 0; i < countof(SkipCheckID); i++)
		{
			if (ppHK1->DescrLangID == SkipCheckID[i])
			{
				bSkip = true; break;
			}
		}
		if (bSkip)
			continue;
		
		for (ConEmuHotKey *ppHK2 = ppHK1+1; ppHK2[0].DescrLangID; ++ppHK2)
		{
			if ((ppHK2->HkType == chk_Modifier) || (ConEmuHotKey::GetHotkey(ppHK2->VkMod) == 0))
				continue;

			// Некоторые хоткеи не проверять
			bSkip = false;
			for (size_t i = 0; i < countof(SkipCheckID); i++)
			{
				if (ppHK2->DescrLangID == SkipCheckID[i])
				{
					bSkip = true; break;
				}
			}
			if (bSkip)
				continue;

			// Хоткеи различаются?
			if (ppHK1->VkMod != ppHK2->VkMod)
				continue;

			// Если совпадают - может быть это макрос, переехавший в системную область?
			if (((ppHK1->HkType == chk_Macro) || (ppHK2->HkType == chk_Macro))
				&& ppHK1->GuiMacro && *ppHK1->GuiMacro && ppHK2->GuiMacro && *ppHK2->GuiMacro)
			{
				if (lstrcmp(ppHK1->GuiMacro, ppHK2->GuiMacro) == 0)
				{
					if (ppHK1->HkType == chk_Macro)
						ppHK1->VkMod = 0;
					else
						ppHK2->VkMod = 0;
					continue;
				}
			}

			wchar_t szDescr1[512], szDescr2[512], szKey[128];

			ppHK1->GetDescription(szDescr1, countof(szDescr1), true);
			ppHK2->GetDescription(szDescr2, countof(szDescr2), true);

			//if (ppHK1->HkType == chk_Macro)
			//	_wsprintf(szDescr1, SKIPLEN(countof(szDescr1)) L"Macro %02i", ppHK1->DescrLangID-vkGuMacro01+1);
			//else if (!LoadString(g_hInstance, ppHK1->DescrLangID, szDescr1, countof(szDescr1)))
			//	_wsprintf(szDescr1, SKIPLEN(countof(szDescr1)) L"%i", ppHK1->DescrLangID);

			//if (ppHK2->HkType == chk_Macro)
			//	_wsprintf(szDescr2, SKIPLEN(countof(szDescr2)) L"Macro %02i", ppHK2->DescrLangID-vkGuMacro01+1);
			//else if (!LoadString(g_hInstance, ppHK2->DescrLangID, szDescr2, countof(szDescr2)))
			//	_wsprintf(szDescr2, SKIPLEN(countof(szDescr2)) L"%i", ppHK2->DescrLangID);

			ppHK1->GetHotkeyName(szKey);

			int nAllLen = lstrlen(szDescr1) + lstrlen(szDescr2) + lstrlen(szKey) + 256;
			pszFailMsg = (wchar_t*)malloc(nAllLen*sizeof(*pszFailMsg));
			_wsprintf(pszFailMsg, SKIPLEN(nAllLen)
				L"Hotkey <%s> is not unique\n%s\n%s",
				szKey, szDescr1, szDescr2);

			goto wrap;
		}
	}
wrap:
	if (pszFailMsg)
	{
		Icon.ShowTrayIcon(pszFailMsg, tsa_Config_Error);
		free(pszFailMsg);
	}
}

void Settings::SaveHotkeys(SettingsBase* reg)
{
	if (!gpSetCls || !gpSetCls->m_HotKeys)
	{
		_ASSERTE(gpSetCls && gpSetCls->m_HotKeys);
		return;
	}

	reg->Save(L"Multi.Modifier", nHostkeyNumberModifier);
	reg->Save(L"Multi.ArrowsModifier", nHostkeyArrowModifier);

	BYTE MacroVersion = GUI_MACRO_VERSION;
	reg->Save(L"KeyMacroVersion", MacroVersion);

	wchar_t szMacroName[80];

	for (ConEmuHotKey *ppHK = gpSetCls->m_HotKeys; ppHK->DescrLangID; ++ppHK)
	{
		if (!*ppHK->Name)
			continue;
		
		if ((ppHK->HkType == chk_Modifier)
			|| (ppHK->HkType == chk_NumHost)
			|| (ppHK->HkType == chk_ArrHost))
		{
			_ASSERTE(ppHK->VkMod == (ppHK->VkMod & 0xFF));
			BYTE Mod = (BYTE)(ppHK->VkMod & 0xFF);
			reg->Save(ppHK->Name, Mod);
		}
		else
		{
			reg->Save(ppHK->Name, ppHK->VkMod);

			if (ppHK->HkType == chk_Macro)
			{
				wcscpy_c(szMacroName, ppHK->Name);
				wcscat_c(szMacroName, L".Text");
				//wchar_t* pszEsc = EscapeString(true, ppHK->GuiMacro);
				//reg->Save(szMacroName, pszEsc);
				//SafeFree(pszEsc);
				reg->Save(szMacroName, ppHK->GuiMacro);
			}
		}
	}
}
