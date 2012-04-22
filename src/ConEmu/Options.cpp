
/*
Copyright (c) 2009-2012 Maximus5
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

#define SHOWDEBUGSTR

#include "Header.h"
//#include "../common/ConEmuCheck.h"
#include "Options.h"
#include "ConEmu.h"
#include "VConChild.h"
#include "VirtualConsole.h"
#include "RealConsole.h"
#include "TabBar.h"
#include "Background.h"
#include "TrayIcon.h"
#include "LoadImg.h"
#include "../ConEmuPlugin/FarDefaultMacros.h"
#include "OptionsFast.h"
#include "version.h"

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


struct CONEMUDEFCOLORS
{
	const wchar_t* pszTitle;
	DWORD dwDefColors[0x10];
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
		L"<Murena scheme>", {
			0x00000000, 0x00644100, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
			0x00808080, 0x00ff0000, 0x0076c587, 0x00ffff00, 0x00004bff, 0x00d78ce6, 0x0000ffff, 0x00ffffff
		}
	},
	{
		L"<tc-maxx>", {
			0x00000000, RGB(11,27,59), RGB(0,128,0), RGB(0,90,135), RGB(106,7,28), RGB(128,0,128), RGB(128,128,0), RGB(40,150,177),
			RGB(128,128,128), RGB(0,0,255), RGB(0,255,0), RGB(0,215,243), RGB(190,7,23), RGB(255,0,255), RGB(255,255,0), RGB(255,255,255)
		}
	},
	{
		L"<Solarized (John Doe)>", {
			0x00362b00, 0x00423607, 0x00756e58, 0x00837b65, 0x002f32dc, 0x00c4716c, 0x00164bcb, 0x00d5e8ee,
			0x00a1a193, 0x00d28b26, 0x00009985, 0x0098a12a, 0x00969483, 0x008236d3, 0x000089b5, 0x00e3f6fd		
		}
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
	gpSet = this; // сразу!
	
	// Сброс переменных (struct, допустимо)
	memset(this, 0, sizeof(*this));

	// -- уже memset
	//// Некоторые вещи нужно делать вне InitSettings, т.к. она может быть
	//// вызвана потом из интерфейса диалога настроек
	//Type[0] = 0;
	//psCmd = psCurCmd = NULL;
	////wcscpy_c(szDefCmd, L"far");
	//psCmdHistory = NULL; nCmdHistorySize = 0;

	// Умолчания устанавливаются в CSettings::CSettings
	//-- // Теперь установим умолчания настроек	
	//-- InitSettings();
}

void Settings::ReleasePointers()
{
	if (sTabCloseMacro) {free(sTabCloseMacro); sTabCloseMacro = NULL;}
	
	if (sSafeFarCloseMacro) {free(sSafeFarCloseMacro); sSafeFarCloseMacro = NULL;}

	if (sSaveAllMacro) {free(sSaveAllMacro); sSaveAllMacro = NULL;}

	if (sRClickMacro) {free(sRClickMacro); sRClickMacro = NULL;}
	
	if (psCmd) {free(psCmd); psCmd = NULL;}
	if (psCurCmd) {free(psCurCmd); psCurCmd = NULL;}
	if (psCmdHistory) {free(psCmdHistory); psCmdHistory = NULL;}
	
	UpdSet.FreePointers();
}

Settings::~Settings()
{
	ReleasePointers();
}

void Settings::InitSettings()
{
	MCHKHEAP

	// Освободить память, т.к. функция может быть вызвана из окна интерфейса настроек	
	ReleasePointers();
	
	// Сброс переменных (struct, допустимо)
	memset(this, 0, sizeof(*this));
	
//------------------------------------------------------------------------
///| Moved from CVirtualConsole |/////////////////////////////////////////
//------------------------------------------------------------------------
	isAutoRegisterFonts = true;
	isMulti = true; icMultiNew = 'W'; icMultiNext = 'Q'; icMultiRecreate = 192/*VK_тильда*/; icMultiBuffer = 'A';
	icMinimizeRestore = 'C';
	icMultiClose = 0/*VK_DELETE*/; icMultiCmd = 'X'; isMultiAutoCreate = false; isMultiLeaveOnClose = false; isMultiIterate = true;
	isMultiNewConfirm = true; isUseWinNumber = true; isUseWinArrows = false; isUseWinTab = false; nMultiHotkeyModifier = VK_LWIN; TestHostkeyModifiers();
	m_isKeyboardHooks = 0;
	isFARuseASCIIsort = false; isFixAltOnAltTab = false; isShellNoZoneCheck = false;
	isFadeInactive = true; mn_FadeLow = DEFAULT_FADE_LOW; mn_FadeHigh = DEFAULT_FADE_HIGH; mb_FadeInitialized = false;
	mn_LastFadeSrc = mn_LastFadeDst = -1;
	nMainTimerElapse = 10;
	nMainTimerInactiveElapse = 1000;
	nAffinity = 0; // 0 - don't change default affinity
	isSkipFocusEvents = false;
	isSendAltEnter = isSendAltSpace = isSendAltTab = isSendAltEsc = isSendAltPrintScrn = isSendPrintScrn = isSendCtrlEsc = isSendAltF9 = false;
	isMonitorConsoleLang = 3;
	DefaultBufferHeight = 1000; AutoBufferHeight = true;
	nCmdOutputCP = 0;

	//WARNING("InitSettings() может вызываться из интерфейса настройки, не промахнуться с хэндлами");
	//// Шрифты
	//memset(m_Fonts, 0, sizeof(m_Fonts));
	////TODO: OLD - на переделку
	//memset(&LogFont, 0, sizeof(LogFont));
	//memset(&LogFont2, 0, sizeof(LogFont2));
	/*LogFont.lfHeight = mn_FontHeight =*/ FontSizeY = 16;
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
	//wcscpy_c(inFont, L"Lucida Console");
	//wcscpy_c(inFont2, L"Lucida Console");
	//mb_Name1Ok = FALSE; mb_Name2Ok = FALSE;
	isTryToCenter = false;
	isAlwaysShowScrollbar = 2;
	isTabFrame = true;
	//isForceMonospace = false; isProportional = false;
	isMonospace = 1;
	isMinToTray = false; isAlwaysShowTrayIcon = false;
	memset(&rcTabMargins, 0, sizeof(rcTabMargins));
	//isFontAutoSize = false; mn_AutoFontWidth = mn_AutoFontHeight = -1;
	
	ConsoleFont.lfHeight = 5;
	ConsoleFont.lfWidth = 3;
	wcscpy_c(ConsoleFont.lfFaceName, L"Lucida Console");
	
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
	AppStd.isExtendFonts = false;
	AppStd.nFontNormalColor = 1; AppStd.nFontBoldColor = 12; AppStd.nFontItalicColor = 13;
	AppStd.isCursorV = true;
	AppStd.isCursorBlink = true;
	AppStd.isCursorColor = false;
	AppStd.isCursorBlockInactive = true;

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
	nTransparent = 255;
	//isColorKey = false;
	//ColorKey = RGB(1,1,1);
	isUserScreenTransparent = false;
	isFixFarBorders = 1; isEnhanceGraphics = true; isEnhanceButtons = false;
	isPartBrush75 = 0xC8; isPartBrush50 = 0x96; isPartBrush25 = 0x5A;
	isPartBrushBlack = 32; //-V112
	isExtendUCharMap = true;
	isDownShowHiddenMessage = false;
	ParseCharRanges(L"2013-25C4", mpc_FixFarBorderValues);
	wndHeight = 25;
	ntvdmHeight = 0; // Подбирать автоматически
	wndWidth = 80;
	WindowMode = rNormal;
	//isFullScreen = false;
	isHideCaption = mb_HideCaptionAlways = false;
	nHideCaptionAlwaysFrame = 1; nHideCaptionAlwaysDelay = 2000; nHideCaptionAlwaysDisappear = 2000;
	isDesktopMode = false;
	isAlwaysOnTop = false;
	isSleepInBackground = false; // по умолчанию - не включать "засыпание в фоне".
	wndX = 0; wndY = 0; wndCascade = true;
	isAutoSaveSizePos = false; mb_SizePosAutoSaved = false;
	isConVisible = false; //isLockRealConsolePos = false;
	isUseInjects = true;
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
	wcscpy_c(szTabConsole, L"<%c> %s `%p`" /*L "%s [%c]" */);
	wcscpy_c(szTabEditor, L"<%c.%i>{%s} `%p`" /* L"[%s]" */);
	wcscpy_c(szTabEditorModified, L"<%c.%i>[%s] `%p` *" /* L"[%s] *" */);
	wcscpy_c(szTabViewer, L"<%c.%i>[%s] `%p`" /* L"{%s}" */);
	nTabLenMax = 20;
	isSafeFarClose = true;
	sSafeFarCloseMacro = NULL; // если NULL - то используется макрос по умолчанию
	isConsoleTextSelection = 1; // Always
	isCTSSelectBlock = true; isCTSVkBlock = VK_LMENU; // по умолчанию - блок выделяется c LAlt
	isCTSSelectText = true; isCTSVkText = VK_LSHIFT; // а текст - при нажатом LShift
	isCTSVkBlockStart = 0; isCTSVkTextStart = 0; // при желании, пользователь может назначить hotkey запуска выделения
	isCTSActMode = 2; // BufferOnly
	isCTSVkAct = 0; // т.к. по умолчанию - только BufferOnly, то вообще без модификаторов
	isCTSRBtnAction = 3; // Auto (Выделения нет - Paste, Есть - Copy)
	isCTSMBtnAction = 0; // <None>
	isCTSColorIndex = 0xE0;
	isFarGotoEditor = true; isFarGotoEditorVk = VK_LCONTROL;
	isTabs = 1; isTabSelf = true; isTabRecent = true; isTabLazy = true;
	ilDragHeight = 10;
	m_isTabsOnTaskBar = 2;
	isTabsInCaption = false; //cbTabsInCaption
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
	isDragEnabled = DRAG_L_ALLOWED; isDropEnabled = (BYTE)1; isDefCopy = true;
	nLDragKey = 0; nRDragKey = VK_LCONTROL;
	isDragOverlay = 1; isDragShowIcons = true;
	// изменение размера панелей мышкой
	isDragPanel = 2; // по умолчанию сделаем чтобы драгалось макросами (вдруг у юзера на Ctrl-Left/Right/Up/Down макросы висят... как бы конфуза не получилось)
	isDragPanelBothEdges = false; // таскать за обе рамки (правую-левой панели и левую-правой панели)
	isKeyBarRClick = true;
	isDebugSteps = true;
	MCHKHEAP
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

void Settings::LoadAppSettings(SettingsBase* reg)
{
	BOOL lbOpened = FALSE;
	wchar_t szAppKey[MAX_PATH+64];
	wcscpy_c(szAppKey, gpSetCls->GetConfigPath());
	wcscat_c(szAppKey, L"\\Apps");
	wchar_t* pszAppKey = szAppKey+lstrlen(szAppKey);

	int NewAppCount = 0;
	AppSettings** NewApps = NULL;
	CEAppColors** NewAppColors = NULL;
	
	lbOpened = reg->OpenKey(szAppKey, KEY_READ);
	if (lbOpened)
	{
		reg->Load(L"Count", NewAppCount);
		reg->CloseKey();
	}

	if (lbOpened && NewAppCount > 0)
	{
		NewApps = (AppSettings**)calloc(NewAppCount, sizeof(*NewApps));
		NewAppColors = (CEAppColors**)calloc(NewAppCount, sizeof(*NewAppColors));

		int nSucceeded = 0;
		for (int i = 0; i < NewAppCount; i++)
		{
			_wsprintf(pszAppKey, SKIPLEN(32) L"\\App%i", i+1);

			lbOpened = reg->OpenKey(szAppKey, KEY_READ);
			if (lbOpened)
			{
				_ASSERTE(AppStd.AppNames == NULL && AppStd.AppNamesLwr == NULL);

				NewApps[nSucceeded] = (AppSettings*)malloc(sizeof(AppSettings));
				NewAppColors[nSucceeded] = (CEAppColors*)calloc(1,sizeof(CEAppColors));

				// Умолчания берем из основной ветки!
				*NewApps[nSucceeded] = AppStd;
				memmove(NewAppColors[nSucceeded]->Colors, Colors, sizeof(Colors));
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
					LoadAppSettings(reg, NewApps[nSucceeded], NewAppColors[nSucceeded]->Colors);
					nSucceeded++;
				}
				reg->CloseKey();
			}
		}
		NewAppCount = nSucceeded;
	}

	int OldAppCount = this->AppCount;
	this->AppCount = NewAppCount;
	AppSettings** OldApps = Apps; Apps = NewApps;
	CEAppColors** OldAppColors = AppColors; AppColors = NewAppColors;
	for (int i = 0; i < OldAppCount && OldApps; i++)
	{
		if (OldApps[i])
		{
			OldApps[i]->FreeApps();
			SafeFree(OldApps[i]);
		}
		SafeFree(OldAppColors[i]);
	}
	SafeFree(OldApps);
	SafeFree(OldAppColors);
}

void Settings::LoadAppSettings(SettingsBase* reg, Settings::AppSettings* pApp, COLORREF* pColors)
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
			reg->Load(ColorName, pColors[i]);
		}
		reg->Load(L"ExtendColors", pApp->isExtendColors);
		reg->Load(L"ExtendColorIdx", pApp->nExtendColorIdx);
		if (pApp->nExtendColorIdx<0 || pApp->nExtendColorIdx>15) pApp->nExtendColorIdx=14;
	}
	else
	{
		reg->Load(L"OverridePalette", pApp->OverridePalette);
		if (!reg->Load(L"PaletteName", pApp->szPaletteName, countof(pApp->szPaletteName)))
			pApp->szPaletteName[0] = 0;
		const Settings::ColorPalette* pPal = PaletteGet(PaletteGetIndex(pApp->szPaletteName));
		_ASSERTE(pPal!=NULL); // NULL не может быть. Всегда как минимум - стандартная палитра
		pApp->isExtendColors = pPal->isExtendColors;
		pApp->nExtendColorIdx = pPal->nExtendColorIdx;
		memmove(pColors, pPal->Colors, sizeof(pPal->Colors));
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
	reg->Load(L"CursorType", pApp->isCursorV);
	reg->Load(L"CursorColor", pApp->isCursorColor);
	reg->Load(L"CursorBlink", pApp->isCursorBlink);
	reg->Load(L"CursorBlockInactive", pApp->isCursorBlockInactive);
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

		reg = CreateSettings();
		if (!reg)
		{
			_ASSERTE(reg!=NULL);
			return;
		}
		lbDelete = true;
	}

	if (CmdTasks)
	{
		for (int i = 0; i < CmdTaskCount; i++)
		{
			if (CmdTasks[i])
				CmdTasks[i]->FreePtr();
		}
		SafeFree(CmdTasks);
	}

	BOOL lbOpened = FALSE;
	wchar_t szCmdKey[MAX_PATH+64], szVal[32];
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
				int iCmdMax = 0, iCmdCount = 0;
				reg->Load(L"Count", iCmdMax);
				if (iCmdMax > 0)
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
						CmdTasks[i] = (CommandTasks*)calloc(NewTasksCount, sizeof(CommandTasks));

						CmdTasks[i]->cchCmdMax = nTotalLen+1;
						CmdTasks[i]->pszCommands = (wchar_t*)malloc(CmdTasks[i]->cchCmdMax*sizeof(wchar_t));
						if (CmdTasks[i]->pszCommands)
						{
							//CmdTasks[i]->nCommands = iCmdCount;

							int nActive = 0;
							reg->Load(L"Active", nActive); // 1-based

							wchar_t* psz = CmdTasks[i]->pszCommands; // dest script
							for (int k = 0; k < iCmdCount; k++)
							{
								if ((k+1) == nActive)
									*(psz++) = L'>';

								lstrcpy(psz, pszCommands[k]);
								SafeFree(pszCommands[k]);

								if ((k+1) < iCmdCount)
									lstrcat(psz, L"\r\n\r\n"); // для визуальности редактирования

								psz += lstrlen(psz);	
							}

							wchar_t* pszNameSet = NULL;
							if (!reg->Load(L"Name", &pszNameSet) || !*pszNameSet)
								SafeFree(pszNameSet);

							CmdTasks[i]->SetName(pszNameSet, i);

							nSucceeded++;
						}
					}

					SafeFree(pszCommands);
				}

				reg->CloseKey();
			}
		}
		CmdTaskCount = nSucceeded;
	}

	if (lbDelete)
		delete reg;
}

void Settings::SaveCmdTasks(SettingsBase* reg)
{
	bool lbDelete = false;
	if (!reg)
	{
		reg = CreateSettings();
		if (!reg)
		{
			_ASSERTE(reg!=NULL);
			return;
		}
		lbDelete = true;
	}

	BOOL lbOpened = FALSE;
	wchar_t szCmdKey[MAX_PATH+64], szVal[32];
	wcscpy_c(szCmdKey, gpSetCls->GetConfigPath());
	wcscat_c(szCmdKey, L"\\Tasks");
	wchar_t* pszCmdKey = szCmdKey+lstrlen(szCmdKey);

	lbOpened = reg->OpenKey(szCmdKey, KEY_WRITE);
	if (lbOpened)
	{
		reg->Save(L"Count", CmdTaskCount);
		reg->CloseKey();
	}

	if (lbOpened && CmdTasks && CmdTaskCount > 0)
	{
		int nSucceeded = 0;
		for (int i = 0; i < CmdTaskCount; i++)
		{
			_wsprintf(pszCmdKey, SKIPLEN(32) L"\\Task%i", i+1); // 1-based

			lbOpened = reg->OpenKey(szCmdKey, KEY_WRITE);
			if (lbOpened)
			{
				reg->Save(L"Name", CmdTasks[i]->pszName);

				int iCmdCount = 0;
				int nActive = 0; // 1-based
				if (CmdTasks[i]->pszCommands)
				{
					wchar_t* pszCmd = CmdTasks[i]->pszCommands;
					while (*pszCmd == L'\r' || *pszCmd == L'\n' || *pszCmd == L'\t' || *pszCmd == L' ')
						pszCmd++;

					while (pszCmd && *pszCmd)
					{
						iCmdCount++;

						wchar_t* pszEnd = wcschr(pszCmd, L'\n');
						if (pszEnd && (*(pszEnd-1) == L'\r'))
							pszEnd--;
						wchar_t chSave;
						if (pszEnd)
						{
							chSave = *pszEnd;
							*pszEnd = 0;
						}

						_wsprintf(szVal, SKIPLEN(countof(szVal)) L"Cmd%i", iCmdCount); // 1-based

						if (*pszCmd == L'>')
						{
							pszCmd++;
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

				reg->CloseKey();
			}
		}
	}

	if (lbDelete)
		delete reg;
}

void Settings::SortPalettes()
{
	for (int i = 0; (i+1) < PaletteCount; i++)
	{
		int iMin = i;
		for (int j = (i+1); j < PaletteCount; j++)
		{
			if (Palettes[iMin]->bPredefined && !Palettes[j]->bPredefined)
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

void Settings::LoadPalettes(SettingsBase* reg)
{
	TCHAR ColorName[] = L"ColorTable00";

	bool lbDelete = false;
	if (!reg)
	{
		reg = CreateSettings();
		if (!reg)
		{
			_ASSERTE(reg!=NULL);
			return;
		}
		lbDelete = true;
	}

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
    	_ASSERTE(countof(Palettes[n]->Colors)==0x20 && countof(DefColors[n].dwDefColors)==0x10);
    	memmove(Palettes[n]->Colors, DefColors[n].dwDefColors, 0x10*sizeof(Palettes[n]->Colors[0]));
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
		reg = CreateSettings();
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
				memmove(AppColors[k]->Colors, Palettes[i]->Colors, sizeof(Palettes[i]->Colors));
				AppColors[k]->FadeInitialized = false;
				Apps[k]->isExtendColors = Palettes[i]->isExtendColors;
				Apps[k]->nExtendColorIdx = Palettes[i]->nExtendColorIdx;
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
	if (anIndex == -1)
	{
		static ColorPalette StdPal = {};
		StdPal.bPredefined = false;
		static wchar_t szCurrentScheme[64] = L"<Current color scheme>";
		StdPal.pszName = szCurrentScheme;
		StdPal.isExtendColors = AppStd.isExtendColors;
		StdPal.nExtendColorIdx = AppStd.nExtendColorIdx;
		_ASSERTE(sizeof(StdPal.Colors) == sizeof(this->Colors));
		memmove(StdPal.Colors, this->Colors, sizeof(StdPal.Colors));
		return &StdPal;
	}

	if (anIndex < -1 || anIndex >= PaletteCount || !Palettes)
		return NULL;

	return Palettes[anIndex];
}

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

	if (nIndex == -1)
	{
		// Добавляем новую палитру
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
	}


	// Сохранять допускается только пользовательские палитры
	Palettes[nIndex]->bPredefined = false;
	Palettes[nIndex]->pszName = lstrdup(asName);
	Palettes[nIndex]->isExtendColors = AppStd.isExtendColors;
	Palettes[nIndex]->nExtendColorIdx = AppStd.nExtendColorIdx;
	_ASSERTE(sizeof(Palettes[nIndex]->Colors) == sizeof(this->Colors));
	memmove(Palettes[nIndex]->Colors, this->Colors, sizeof(Palettes[nIndex]->Colors));
	_ASSERTE(nIndex < PaletteCount);

	// Теперь, собственно, пишем настройки
	SavePalettes(NULL);
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

void Settings::LoadSettings()
{
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

//------------------------------------------------------------------------
///| Loading from registry |//////////////////////////////////////////////
//------------------------------------------------------------------------
	SettingsBase* reg = CreateSettings();
	wcscpy_c(Type, reg->Type);

	BOOL lbOpened = FALSE, lbNeedCreateVanilla = FALSE;

	lbOpened = reg->OpenKey(gpSetCls->GetConfigPath(), KEY_READ);
	// Поддержка старого стиля хранения (настройки БЕЗ имени конфига - лежали в корне ключа Software\ConEmu)
	if (!lbOpened && (*gpSetCls->GetConfigName() == 0))
	{
		lbOpened = reg->OpenKey(CONEMU_ROOT_KEY, KEY_READ);
		lbNeedCreateVanilla = lbOpened;
	}

	if (lbOpened)
	{
		LoadAppSettings(reg, &AppStd, Colors);

		reg->Load(L"TrueColorerSupport", isTrueColorer);
		reg->Load(L"FadeInactive", isFadeInactive);
		reg->Load(L"FadeInactiveLow", mn_FadeLow);
		reg->Load(L"FadeInactiveHigh", mn_FadeHigh);

		if (mn_FadeHigh <= mn_FadeLow) { mn_FadeLow = DEFAULT_FADE_LOW; mn_FadeHigh = DEFAULT_FADE_HIGH; }
		mn_LastFadeSrc = mn_LastFadeDst = -1;

		// Debugging
		reg->Load(L"ConVisible", isConVisible);
		reg->Load(L"ConInMode", nConInMode);
		
		
		reg->Load(L"UseInjects", isUseInjects);
		//if (isUseInjects > BST_INDETERMINATE)
		//	isUseInjects = BST_CHECKED;

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

		if (!reg->Load(L"Monospace", isMonospace))
		{
			// Compatibility. Пережитки прошлого, загрузить по "старым" именам параметров
			bool bForceMonospace = false, bProportional = false;
			reg->Load(L"ForceMonospace", bForceMonospace);
			reg->Load(L"Proportional", bProportional);
			isMonospace = bForceMonospace ? 2 : bProportional ? 0 : 1;
		}
		if (isMonospace > 2) isMonospace = 2;

		//isMonospaceSelected = isMonospace ? isMonospace : 1; // запомнить, чтобы выбирать то что нужно при смене шрифта
			
			
			
		reg->Load(L"CmdLine", &psCmd);
		reg->Load(L"CmdLineHistory", &psCmdHistory); nCmdHistorySize = 0; HistoryCheck();
		reg->Load(L"Multi", isMulti);
		reg->Load(L"Multi.Modifier", nMultiHotkeyModifier); TestHostkeyModifiers();
		reg->Load(L"Multi.NewConsole", icMultiNew);
		reg->Load(L"Multi.Next", icMultiNext);
		reg->Load(L"Multi.Recreate", icMultiRecreate);
		reg->Load(L"Multi.Close", icMultiClose);
		reg->Load(L"Multi.CmdKey", icMultiCmd);
		reg->Load(L"Multi.NewConfirm", isMultiNewConfirm);
		reg->Load(L"Multi.Buffer", icMultiBuffer);
		reg->Load(L"Multi.UseArrows", isUseWinArrows);
		reg->Load(L"Multi.UseNumbers", isUseWinNumber);
		reg->Load(L"Multi.UseWinTab", isUseWinTab);
		reg->Load(L"Multi.AutoCreate", isMultiAutoCreate);
		reg->Load(L"Multi.LeaveOnClose", isMultiLeaveOnClose);
		reg->Load(L"Multi.Iterate", isMultiIterate);
		reg->Load(L"MinimizeRestore", icMinimizeRestore);

		reg->Load(L"KeyboardHooks", m_isKeyboardHooks); if (m_isKeyboardHooks>2) m_isKeyboardHooks = 0;

		reg->Load(L"FontAutoSize", isFontAutoSize);
		reg->Load(L"FontSize", FontSizeY);
		reg->Load(L"FontSizeX", FontSizeX);
		reg->Load(L"FontSizeX3", FontSizeX3);
		reg->Load(L"FontSizeX2", FontSizeX2);
		reg->Load(L"FontCharSet", mn_LoadFontCharSet); mb_CharSetWasSet = FALSE;
		reg->Load(L"Anti-aliasing", mn_AntiAlias);
		
		reg->Load(L"ConsoleFontName", ConsoleFont.lfFaceName, countof(ConsoleFont.lfFaceName));
		reg->Load(L"ConsoleFontWidth", ConsoleFont.lfWidth);
		reg->Load(L"ConsoleFontHeight", ConsoleFont.lfHeight);
		
		reg->Load(L"WindowMode", WindowMode); if (WindowMode!=rFullScreen && WindowMode!=rMaximized && WindowMode!=rNormal) WindowMode = rNormal;
		reg->Load(L"HideCaption", isHideCaption);
		// грузим именно в mb_HideCaptionAlways, т.к. прозрачность сбивает темы в заголовке, поэтому возврат идет через isHideCaptionAlways()
		reg->Load(L"HideCaptionAlways", mb_HideCaptionAlways);
		reg->Load(L"HideCaptionAlwaysFrame", nHideCaptionAlwaysFrame);

		if (nHideCaptionAlwaysFrame > 10) nHideCaptionAlwaysFrame = 10;

		reg->Load(L"HideCaptionAlwaysDelay", nHideCaptionAlwaysDelay);

		if (nHideCaptionAlwaysDelay > 30000) nHideCaptionAlwaysDelay = 30000;

		reg->Load(L"HideCaptionAlwaysDisappear", nHideCaptionAlwaysDisappear);

		if (nHideCaptionAlwaysDisappear > 30000) nHideCaptionAlwaysDisappear = 30000;

		reg->Load(L"DownShowHiddenMessage", isDownShowHiddenMessage);

		reg->Load(L"ConWnd X", wndX); /*if (wndX<-10) wndX = 0;*/
		reg->Load(L"ConWnd Y", wndY); /*if (wndY<-10) wndY = 0;*/
		reg->Load(L"AutoSaveSizePos", isAutoSaveSizePos);
		// ЭТО не влияет на szDefCmd. Только прямое указание флажка "/BufferHeight N"
		// может сменить (умолчательную) команду запуска на "cmd" или "far"
		reg->Load(L"Cascaded", wndCascade);
		reg->Load(L"ConWnd Width", wndWidth); if (!wndWidth) wndWidth = 80; else if (wndWidth>1000) wndWidth = 1000;
		reg->Load(L"ConWnd Height", wndHeight); if (!wndHeight) wndHeight = 25; else if (wndHeight>500) wndHeight = 500;

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
		nVal = ComSpec.csBits;
		reg->Load(L"ComSpec.Bits", nVal);
		if (nVal <= csb_Last) ComSpec.csBits = (ComSpecBits)nVal;
		nVal = ComSpec.isUpdateEnv;
		reg->Load(L"ComSpec.UpdateEnv", nVal);
		ComSpec.isUpdateEnv = (nVal != 0);
		reg->Load(L"ComSpec.Path", ComSpec.ComspecExplicit, countof(ComSpec.ComspecExplicit));
		//-- wcscpy_c(ComSpec.ComspecInitial, gpConEmu->ms_ComSpecInitial);
		// Обработать 32/64 (найти tcc.exe и т.п.)
		FindComspec(&ComSpec);
		UpdateComspec(&ComSpec);

		reg->Load(L"ConsoleTextSelection", isConsoleTextSelection); if (isConsoleTextSelection>2) isConsoleTextSelection = 2;

		reg->Load(L"CTS.SelectBlock", isCTSSelectBlock);
		reg->Load(L"CTS.VkBlock", isCTSVkBlock);
		reg->Load(L"CTS.VkBlockStart", isCTSVkBlockStart);
		reg->Load(L"CTS.SelectText", isCTSSelectText);
		reg->Load(L"CTS.VkText", isCTSVkText);
		reg->Load(L"CTS.VkTextStart", isCTSVkTextStart);

		reg->Load(L"CTS.ActMode", isCTSActMode); if (!isCTSActMode || isCTSActMode>2) isCTSActMode = 2;

		reg->Load(L"CTS.VkAct", isCTSVkAct);

		reg->Load(L"CTS.RBtnAction", isCTSRBtnAction); if (isCTSRBtnAction>3) isCTSRBtnAction = 0;

		reg->Load(L"CTS.MBtnAction", isCTSMBtnAction); if (isCTSMBtnAction>3) isCTSMBtnAction = 0;

		reg->Load(L"CTS.ColorIndex", isCTSColorIndex); if ((isCTSColorIndex & 0xF) == ((isCTSColorIndex & 0xF0)>>4)) isCTSColorIndex = 0xE0;
		
		reg->Load(L"FarGotoEditor", isFarGotoEditor);
		reg->Load(L"FarGotoEditorVk", isFarGotoEditorVk);

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
		
		reg->Load(L"AltEnter", isSendAltEnter);
		reg->Load(L"AltSpace", isSendAltSpace); //if (isSendAltSpace > 2) isSendAltSpace = 2; // когда-то был 3state, теперь - bool
		reg->Load(L"SendAltTab", isSendAltTab);
		reg->Load(L"SendAltEsc", isSendAltEsc);
		reg->Load(L"SendAltPrintScrn", isSendAltPrintScrn);
		reg->Load(L"SendPrintScrn", isSendPrintScrn);
		reg->Load(L"SendCtrlEsc", isSendCtrlEsc);
		reg->Load(L"SendAltF9", isSendAltF9);
		
		reg->Load(L"Min2Tray", isMinToTray);
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
		if (bgOperation!=eUpLeft && bgOperation!=eStretch && bgOperation!=eTile) bgOperation = 0;
		reg->Load(L"bgPluginAllowed", isBgPluginAllowed);
		if (isBgPluginAllowed!=0 && isBgPluginAllowed!=1 && isBgPluginAllowed!=2) isBgPluginAllowed = 1;

		reg->Load(L"AlphaValue", nTransparent);
		if (nTransparent < MIN_ALPHA_VALUE) nTransparent = MIN_ALPHA_VALUE;

		//reg->Load(L"UseColorKey", isColorKey);
		//reg->Load(L"ColorKey", ColorKey);
		reg->Load(L"UserScreenTransparent", isUserScreenTransparent);
		
		//reg->Load(L"Update Console handle", isUpdConHandle);
		reg->Load(L"DisableMouse", isDisableMouse);
		reg->Load(L"RSelectionFix", isRSelFix);
		reg->Load(L"MouseSkipActivation", isMouseSkipActivation);
		reg->Load(L"MouseSkipMoving", isMouseSkipMoving);
		reg->Load(L"FarHourglass", isFarHourglass);
		reg->Load(L"FarHourglassDelay", nFarHourglassDelay);
		
		reg->Load(L"Dnd", isDragEnabled);
		isDropEnabled = (BYTE)(isDragEnabled ? 1 : 0); // ранее "DndDrop" не было, поэтому ставим default
		reg->Load(L"DndLKey", nLDragKey);
		reg->Load(L"DndRKey", nRDragKey);
		reg->Load(L"DndDrop", isDropEnabled);
		reg->Load(L"DefCopy", isDefCopy);
		reg->Load(L"DragOverlay", isDragOverlay);
		reg->Load(L"DragShowIcons", isDragShowIcons);
		
		reg->Load(L"DragPanel", isDragPanel); if (isDragPanel > 2) isDragPanel = 1;
		reg->Load(L"DragPanelBothEdges", isDragPanelBothEdges);

		reg->Load(L"KeyBarRClick", isKeyBarRClick);
		
		reg->Load(L"DebugSteps", isDebugSteps);				

		//reg->Load(L"GUIpb", isGUIpb);
		reg->Load(L"Tabs", isTabs);
		reg->Load(L"TabSelf", isTabSelf);
		reg->Load(L"TabLazy", isTabLazy);
		reg->Load(L"TabRecent", isTabRecent);
		reg->Load(L"TabsOnTaskBar", m_isTabsOnTaskBar);

		if (!reg->Load(L"TabCloseMacro", &sTabCloseMacro) || (sTabCloseMacro && !*sTabCloseMacro)) { SafeFree(sTabCloseMacro); }

		reg->Load(L"TabFontFace", sTabFontFace, countof(sTabFontFace));
		reg->Load(L"TabFontCharSet", nTabFontCharSet);
		reg->Load(L"TabFontHeight", nTabFontHeight);

		if (!reg->Load(L"SaveAllEditors", &sSaveAllMacro) || (sSaveAllMacro && !*sSaveAllMacro)) { SafeFree(sSaveAllMacro); }

		reg->Load(L"TabFrame", isTabFrame);
		reg->Load(L"TabMargins", rcTabMargins);
		
		reg->Load(L"ToolbarAddSpace", nToolbarAddSpace);
		if (nToolbarAddSpace<0 || nToolbarAddSpace>100) nToolbarAddSpace = 0;

		reg->Load(L"SlideShowElapse", nSlideShowElapse);
		
		reg->Load(L"IconID", nIconID);
		
		
		reg->Load(L"TabConsole", szTabConsole, countof(szTabConsole));
		//WCHAR* pszVert = wcschr(szTabPanels, L'|');
		//if (!pszVert) {
		//    if (_tcslen(szTabPanels)>54) szTabPanels[54] = 0;
		//    pszVert = szTabPanels + _tcslen(szTabPanels);
		//    wcscpy_c(pszVert+1, L"Console");
		//}
		//*pszVert = 0; pszTabConsole = pszVert+1;
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

		reg->Load(L"ShowScrollbar", isAlwaysShowScrollbar); if (isAlwaysShowScrollbar > 2) isAlwaysShowScrollbar = 2;

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
		reg->Load(L"AlwaysOnTop", isAlwaysOnTop);
		reg->Load(L"SleepInBackground", isSleepInBackground);

		reg->Load(L"DisableFarFlashing", isDisableFarFlashing); if (isDisableFarFlashing>2) isDisableFarFlashing = 2;

		reg->Load(L"DisableAllFlashing", isDisableAllFlashing); if (isDisableAllFlashing>2) isDisableAllFlashing = 2;

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
		reg->Load(L"Update.UseBuilds", UpdSet.isUpdateUseBuilds); if (UpdSet.isUpdateUseBuilds>2) UpdSet.isUpdateUseBuilds = 2; // 1-stable only, 2-latest
		reg->Load(L"Update.UseProxy", UpdSet.isUpdateUseProxy);
		reg->Load(L"Update.Proxy", &UpdSet.szUpdateProxy);
		reg->Load(L"Update.ProxyUser", &UpdSet.szUpdateProxyUser);
		reg->Load(L"Update.ProxyPassword", &UpdSet.szUpdateProxyPassword);
		reg->Load(L"Update.DownloadSetup", UpdSet.isUpdateDownloadSetup); // 1-Installer (ConEmuSetup.exe), 2-7z archieve (ConEmu.7z), WinRar or 7z required
		reg->Load(L"Update.ExeCmdLine", &UpdSet.szUpdateExeCmdLine);
		reg->Load(L"Update.ArcCmdLine", &UpdSet.szUpdateArcCmdLine);
		reg->Load(L"Update.DownloadPath", &UpdSet.szUpdateDownloadPath);
		reg->Load(L"Update.LeavePackages", UpdSet.isUpdateLeavePackages);
		reg->Load(L"Update.PostUpdateCmd", &UpdSet.szUpdatePostUpdateCmd);

		/* Done */
		reg->CloseKey();
	}

	LoadPalettes(reg);

	LoadAppSettings(reg);

	LoadCmdTasks(reg);

	// сервис больше не нужен
	delete reg;
	reg = NULL;

	
	// Зовем "FastConfiguration" (перед созданием новой/чистой конфигурации)
	{
		LPCWSTR pszDef = gpConEmu->GetDefaultTitle();
		wchar_t szType[8]; bool ReadOnly;
		GetSettingsType(szType, ReadOnly);
		LPCWSTR pszConfig = gpSetCls->GetConfigName();
		wchar_t szTitle[1024];
		if (pszConfig && *pszConfig)
			_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s fast configuration (%s) %s", pszDef, pszConfig, szType);
		else
			_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s fast configuration %s", pszDef, szType);
		
		CheckOptionsFast(szTitle, lbNeedCreateVanilla);
	}
	

	if (lbNeedCreateVanilla)
	{
		SaveSettings(TRUE/*abSilent*/);
	}

	// Передернуть палитру затенения
	ResetFadeColors();
	GetColors(-1, TRUE);
	
	

	// Проверить необходимость установки хуков
	//-- isKeyboardHooks();
	// При первом запуске - проверить, хотят ли включить автообновление?
	//-- CheckUpdatesWanted();



	// Стили окна
	if (!WindowMode)
	{
		// Иначе окно вообще не отображается
		_ASSERTE(WindowMode!=0);
		WindowMode = rNormal;
	}

	if (wndCascade && (ghWnd == NULL))
	{
		// Сдвиг при каскаде
		int nShift = (GetSystemMetrics(SM_CYSIZEFRAME)+GetSystemMetrics(SM_CYCAPTION))*1.5;
		// Координаты и размер виртуальной рабочей области
		RECT rcScreen = MakeRect(800,600);
		int nMonitors = GetSystemMetrics(SM_CMONITORS);

		if (nMonitors > 1)
		{
			// Размер виртуального экрана по всем мониторам
			rcScreen.left = GetSystemMetrics(SM_XVIRTUALSCREEN); // may be <0
			rcScreen.top  = GetSystemMetrics(SM_YVIRTUALSCREEN);
			rcScreen.right = rcScreen.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
			rcScreen.bottom = rcScreen.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
			TODO("Хорошо бы исключить из рассмотрения Taskbar...");
		}
		else
		{
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, 0);
		}

		HWND hPrev = FindWindow(VirtualConsoleClassMain, NULL);

		while(hPrev)
		{
			/*if (Is Iconic(hPrev) || Is Zoomed(hPrev)) {
				hPrev = FindWindowEx(NULL, hPrev, VirtualConsoleClassMain, NULL);
				continue;
			}*/
			WINDOWPLACEMENT wpl = {sizeof(WINDOWPLACEMENT)}; // Workspace coordinates!!!

			if (!GetWindowPlacement(hPrev, &wpl))
			{
				break;
			}

			// Screen coordinates!
			RECT rcWnd; GetWindowRect(hPrev, &rcWnd);

			if (wpl.showCmd == SW_HIDE || !IsWindowVisible(hPrev)
			        || wpl.showCmd == SW_SHOWMINIMIZED || wpl.showCmd == SW_SHOWMAXIMIZED
			        /* Max в режиме скрытия заголовка */
			        || (wpl.rcNormalPosition.left<rcScreen.left || wpl.rcNormalPosition.top<rcScreen.top))
			{
				hPrev = FindWindowEx(NULL, hPrev, VirtualConsoleClassMain, NULL);
				continue;
			}

			wndX = rcWnd.left + nShift;
			wndY = rcWnd.top + nShift;
			break; // нашли, сдвинулись, выходим
		}
	}

	if (rcTabMargins.top > 100) rcTabMargins.top = 100;

	_ASSERTE(!rcTabMargins.bottom && !rcTabMargins.left && !rcTabMargins.right);
	rcTabMargins.bottom = rcTabMargins.left = rcTabMargins.right = 0;

	if (!psCmdHistory)
	{
		psCmdHistory = (wchar_t*)calloc(2,2);
	}

	MCHKHEAP
	
	// Переменные загружены, выполнить дополнительные действия в классе настроек
	gpSetCls->SettingsLoaded();
}

/*
[HKEY_CURRENT_USER\Software\ConEmu\.Vanilla\Fonts\Font3]
"Title"="CJK"
"Used"=hex:01 ; Для быстрого включения/отключения группы
"CharRange"="2E80-2EFF;3000-303F;3200-4DB5;4E00-9FC3;F900-FAFF;FE30-FE4F;"
"FontFace"="Arial Unicode MS"
"Width"=dword:00000000
"Height"=dword:00000009
"Charset"=hex:00
"Bold"=hex:00
"Italic"=hex:00
"Anti-aliasing"=hex:04
*/
BOOL Settings::FontRangeLoad(SettingsBase* reg, int Idx)
{
	_ASSERTE(FALSE); // Не реализовано

	BOOL lbOpened = FALSE; //, lbNeedCreateVanilla = FALSE;
	wchar_t szFontKey[MAX_PATH+20];
	_wsprintf(szFontKey, SKIPLEN(countof(szFontKey)) L"%s\\Font%i", gpSetCls->GetConfigPath(), (Idx+1));
	
	
	
	lbOpened = reg->OpenKey(szFontKey, KEY_READ);
	if (lbOpened)
	{
		TODO("Загрузить поля m_Fonts[Idx], очистив hFonts, не забыть про nLoadHeight/nLoadWidth");

		reg->CloseKey();
	}
	
	return lbOpened;
}
BOOL Settings::FontRangeSave(SettingsBase* reg, int Idx)
{
	_ASSERTE(FALSE); // Не реализовано
	return FALSE;
}

void Settings::SaveSizePosOnExit()
{
	if (!this || !isAutoSaveSizePos)
		return;

	// При закрытии окна крестиком - сохранять только один раз,
	// а то размер может в процессе закрытия консолей измениться
	if (mb_SizePosAutoSaved)
		return;
	mb_SizePosAutoSaved = true;
		
	SettingsBase* reg = CreateSettings();

	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
	{
		reg->Save(L"ConWnd Width", wndWidth);
		reg->Save(L"ConWnd Height", wndHeight);
		reg->Save(L"16bit Height", ntvdmHeight);
		reg->Save(L"ConWnd X", wndX);
		reg->Save(L"ConWnd Y", wndY);
		reg->Save(L"Cascaded", wndCascade);
		reg->Save(L"AutoSaveSizePos", isAutoSaveSizePos);
		reg->CloseKey();
	}

	delete reg;
}

void Settings::SaveConsoleFont()
{
	if (!this)
		return;

	SettingsBase* reg = CreateSettings();

	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
	{
		reg->Save(L"ConsoleFontName", ConsoleFont.lfFaceName);
		reg->Save(L"ConsoleFontWidth", ConsoleFont.lfWidth);
		reg->Save(L"ConsoleFontHeight", ConsoleFont.lfHeight);
		reg->CloseKey();
	}

	delete reg;
}

void Settings::UpdateMargins(RECT arcMargins)
{
	if (memcmp(&arcMargins, &rcTabMargins, sizeof(rcTabMargins))==0)
		return;

	rcTabMargins = arcMargins;
	SettingsBase* reg = CreateSettings();

	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
	{
		reg->Save(L"TabMargins", rcTabMargins);
		reg->CloseKey();
	}

	delete reg;
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
			SaveAppSettings(reg, Apps[i], NULL/*Цвета сохраняются как Имя палитры*/);
			reg->CloseKey();
		}
	}
}

void Settings::SaveAppSettings(SettingsBase* reg, Settings::AppSettings* pApp, COLORREF* pColors)
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
			reg->Save(ColorName, (DWORD)pColors[i]);
		}

		reg->Save(L"ExtendColors", pApp->isExtendColors);
		reg->Save(L"ExtendColorIdx", pApp->nExtendColorIdx);
	}
	else
	{
		reg->Save(L"OverridePalette", pApp->OverridePalette);
		reg->Save(L"PaletteName", pApp->szPaletteName);
	}

	reg->Save(L"OverrideExtendFonts", pApp->OverrideExtendFonts);
	reg->Save(L"ExtendFonts", pApp->isExtendFonts);
	reg->Save(L"ExtendFontNormalIdx", pApp->nFontNormalColor);
	reg->Save(L"ExtendFontBoldIdx", pApp->nFontBoldColor);
	reg->Save(L"ExtendFontItalicIdx", pApp->nFontItalicColor);

	reg->Save(L"OverrideCursor", pApp->OverrideCursor);
	reg->Save(L"CursorType", pApp->isCursorV);
	reg->Save(L"CursorColor", pApp->isCursorColor);
	reg->Save(L"CursorBlink", pApp->isCursorBlink);
	reg->Save(L"CursorBlockInactive", pApp->isCursorBlockInactive);
}

BOOL Settings::SaveSettings(BOOL abSilent /*= FALSE*/)
{
	BOOL lbRc = FALSE;

	gpSetCls->SettingsPreSave();

	SettingsBase* reg = CreateSettings();

	// Если в реестре настройка есть, или изменилось значение
	bool lbCurAutoRegisterFonts = isAutoRegisterFonts, lbCurAutoRegisterFontsRc = false;
	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_READ, abSilent))
	{
		lbCurAutoRegisterFontsRc = reg->Load(L"AutoRegisterFonts", lbCurAutoRegisterFonts);
		reg->CloseKey();
	}


	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE, abSilent))
	{
		wcscpy_c(Type, reg->Type);

		SaveAppSettings(reg, &AppStd, Colors);

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

		#ifdef USEPORTABLEREGISTRY
		reg->Save(L"PortableReg", isPortableReg);
		#endif
		
		//reg->Save(L"LockRealConsolePos", isLockRealConsolePos);
		reg->Save(L"CmdLine", psCmd);

		if (psCmdHistory)
			reg->SaveMSZ(L"CmdLineHistory", psCmdHistory, nCmdHistorySize);

		reg->Save(L"Multi", isMulti);
		reg->Save(L"Multi.Modifier", nMultiHotkeyModifier);
		reg->Save(L"Multi.NewConsole", icMultiNew);
		reg->Save(L"Multi.Next", icMultiNext);
		reg->Save(L"Multi.Recreate", icMultiRecreate);
		reg->Save(L"Multi.Close", icMultiClose);
		reg->Save(L"Multi.CmdKey", icMultiCmd);
		reg->Save(L"Multi.NewConfirm", isMultiNewConfirm);
		reg->Save(L"Multi.Buffer", icMultiBuffer);
		reg->Save(L"Multi.UseArrows", isUseWinArrows);
		reg->Save(L"Multi.UseNumbers", isUseWinNumber);
		reg->Save(L"Multi.UseWinTab", isUseWinTab);
		reg->Save(L"Multi.AutoCreate", isMultiAutoCreate);
		reg->Save(L"Multi.LeaveOnClose", isMultiLeaveOnClose);
		reg->Save(L"Multi.Iterate", isMultiIterate);
		reg->Save(L"MinimizeRestore", icMinimizeRestore);
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
		//reg->Save(L"UseColorKey", isColorKey);		
		//reg->Save(L"ColorKey", ColorKey);		
		reg->Save(L"UserScreenTransparent", isUserScreenTransparent);		
		DWORD saveMode = (ghWnd == NULL) 
					? gpConEmu->WindowMode
					: (gpConEmu->isFullScreen() ? rFullScreen : gpConEmu->isZoomed() ? rMaximized : rNormal);
		reg->Save(L"WindowMode", saveMode);
		reg->Save(L"HideCaption", isHideCaption);
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
		reg->Save(L"ComSpec.Path", ComSpec.ComspecExplicit);
		reg->Save(L"ConsoleTextSelection", isConsoleTextSelection);
		reg->Save(L"CTS.SelectBlock", isCTSSelectBlock);
		reg->Save(L"CTS.VkBlock", isCTSVkBlock);
		reg->Save(L"CTS.VkBlockStart", isCTSVkBlockStart);
		reg->Save(L"CTS.SelectText", isCTSSelectText);
		reg->Save(L"CTS.VkText", isCTSVkText);
		reg->Save(L"CTS.VkTextStart", isCTSVkTextStart);
		reg->Save(L"CTS.ActMode", isCTSActMode);
		reg->Save(L"CTS.VkAct", isCTSVkAct);
		reg->Save(L"CTS.RBtnAction", isCTSRBtnAction);
		reg->Save(L"CTS.MBtnAction", isCTSMBtnAction);
		reg->Save(L"CTS.ColorIndex", isCTSColorIndex);
		
		reg->Save(L"FarGotoEditor", isFarGotoEditor);
		reg->Save(L"FarGotoEditorVk", isFarGotoEditorVk);
		
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
		reg->Save(L"AltEnter", isSendAltEnter);
		reg->Save(L"AltSpace", isSendAltSpace);
		reg->Save(L"SendAltTab", isSendAltTab);
		reg->Save(L"SendAltEsc", isSendAltEsc);
		reg->Save(L"SendAltPrintScrn", isSendAltPrintScrn);
		reg->Save(L"SendPrintScrn", isSendPrintScrn);
		reg->Save(L"SendCtrlEsc", isSendCtrlEsc);
		reg->Save(L"SendAltF9", isSendAltF9);
		reg->Save(L"Min2Tray", isMinToTray);
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
		reg->Save(L"DndLKey", nLDragKey);
		reg->Save(L"DndRKey", nRDragKey);
		reg->Save(L"DndDrop", isDropEnabled);
		reg->Save(L"DefCopy", isDefCopy);
		reg->Save(L"DragOverlay", isDragOverlay);
		reg->Save(L"DragShowIcons", isDragShowIcons);
		reg->Save(L"DebugSteps", isDebugSteps);
		reg->Save(L"DragPanel", isDragPanel);
		reg->Save(L"DragPanelBothEdges", isDragPanelBothEdges);
		reg->Save(L"KeyBarRClick", isKeyBarRClick);
		//reg->Save(L"GUIpb", isGUIpb);
		reg->Save(L"Tabs", isTabs);
		reg->Save(L"TabSelf", isTabSelf);
		reg->Save(L"TabLazy", isTabLazy);
		reg->Save(L"TabRecent", isTabRecent);
		reg->Save(L"TabsOnTaskBar", m_isTabsOnTaskBar);
		reg->Save(L"TabCloseMacro", sTabCloseMacro);
		reg->Save(L"TabFontFace", sTabFontFace);
		reg->Save(L"TabFontCharSet", nTabFontCharSet);
		reg->Save(L"TabFontHeight", nTabFontHeight);
		reg->Save(L"SaveAllEditors", sSaveAllMacro);
		reg->Save(L"TabFrame", isTabFrame);
		reg->Save(L"TabMargins", rcTabMargins);
		reg->Save(L"ToolbarAddSpace", nToolbarAddSpace);
		reg->Save(L"TabConsole", szTabConsole);
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
		reg->Save(L"ShowScrollbar", isAlwaysShowScrollbar);
		reg->Save(L"IconID", nIconID);
		//reg->Save(L"Update Console handle", isUpdConHandle);
		reg->Save(L"ConWnd Width", wndWidth);
		reg->Save(L"ConWnd Height", wndHeight);
		reg->Save(L"16bit Height", ntvdmHeight);
		reg->Save(L"ConWnd X", wndX);
		reg->Save(L"ConWnd Y", wndY);
		reg->Save(L"Cascaded", wndCascade);
		reg->Save(L"AutoSaveSizePos", isAutoSaveSizePos);
		mb_SizePosAutoSaved = false; // Раз было инициированное пользователей сохранение настроек - сбросим флажок
		/*reg->Save(L"ScrollTitle", isScrollTitle);
		reg->Save(L"ScrollTitleLen", ScrollTitleLen);*/
		reg->Save(L"MainTimerElapse", nMainTimerElapse);
		reg->Save(L"MainTimerInactiveElapse", nMainTimerInactiveElapse);
		reg->Save(L"AffinityMask", nAffinity);
		reg->Save(L"SkipFocusEvents", isSkipFocusEvents);
		reg->Save(L"MonitorConsoleLang", isMonitorConsoleLang);
		reg->Save(L"DesktopMode", isDesktopMode);
		reg->Save(L"AlwaysOnTop", isAlwaysOnTop);
		reg->Save(L"SleepInBackground", isSleepInBackground);
		reg->Save(L"DisableFarFlashing", isDisableFarFlashing);
		reg->Save(L"DisableAllFlashing", isDisableAllFlashing);
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
		reg->Save(L"Update.DownloadSetup", UpdSet.isUpdateDownloadSetup); // 1-Installer (ConEmuSetup.exe), 2-7z archieve (ConEmu.7z), WinRar or 7z required
		reg->Save(L"Update.ExeCmdLine", UpdSet.szUpdateExeCmdLine);
		reg->Save(L"Update.ArcCmdLine", UpdSet.szUpdateArcCmdLine);
		reg->Save(L"Update.DownloadPath", UpdSet.szUpdateDownloadPath);
		reg->Save(L"Update.LeavePackages", UpdSet.isUpdateLeavePackages);
		reg->Save(L"Update.PostUpdateCmd", UpdSet.szUpdatePostUpdateCmd);

		/* Done */
		reg->CloseKey();

		/* Subsections */
		SaveCmdTasks(reg);
		SaveAppSettings(reg);

		/* Done */
		lbRc = TRUE;
	}

	//}
	delete reg;

	// Вроде и показывать не нужно. Объект уже сам ругнулся
	//MessageBoxA(ghOpWnd, "Failed", "Information", MB_ICONERROR);
	return lbRc;
}

bool Settings::isKeyboardHooks()
{
	//// Нужно и для WinXP, но только в "локальном" режиме
	//if (gOSVer.dwMajorVersion < 6)
	//{
	//	return true;
	//}


	//if (m_isKeyboardHooks == 0)
	//{
	//	// Вопрос пользователю еще не задавали (это на старте, окно еще и не создано)
	//	int nBtn = MessageBox(NULL,
	//	                      L"Do You want to use Win-Number combination for \n"
	//	                      L"switching between consoles (Multi Console feature)? \n\n"
	//	                      L"If You choose 'Yes' - ConEmu will install keyboard hook. \n"
	//	                      L"So, You must allow that in antiviral software (such as AVP). \n\n"
	//	                      L"You can change behavior later via Settings->Features->\n"
	//	                      L"'Install keyboard hooks (Vista & Win7)' check box, or\n"
	//	                      L"'KeyboardHooks' value in ConEmu settings (registry or xml)."
	//	                      , gpConEmu->GetDefaultTitle(), MB_YESNOCANCEL|MB_ICONQUESTION);

	//	if (nBtn == IDCANCEL)
	//	{
	//		m_isKeyboardHooks = 2; // NO
	//	}
	//	else
	//	{
	//		m_isKeyboardHooks = (nBtn == IDYES) ? 1 : 2;
	//		SettingsBase* reg = CreateSettings();

	//		if (!reg)
	//		{
	//			_ASSERTE(reg!=NULL);
	//		}
	//		else
	//		{
	//			if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
	//			{
	//				reg->Save(L"KeyboardHooks", m_isKeyboardHooks);
	//				reg->CloseKey();
	//			}

	//			delete reg;
	//		}
	//	}
	//}

	return (m_isKeyboardHooks == 1);
}

//bool Settings::CheckUpdatesWanted()
//{
//	if (UpdSet.isUpdateUseBuilds == 0)
//	{
//		int nBtn = MessageBox(NULL,
//			L"Do you want to enable automatic updates? \n\n"
//			L"You may change settings on 'Update' page of Settings dialog later"
//			, gpConEmu->GetDefaultTitle(), MB_YESNOCANCEL|MB_ICONQUESTION);
//
//		if (nBtn == IDCANCEL)
//		{
//			return false;
//		}
//
//		UpdSet.isUpdateCheckOnStartup = (nBtn == IDYES);
//		UpdSet.isUpdateCheckHourly = false;
//		UpdSet.isUpdateConfirmDownload = true;
//		UpdSet.isUpdateUseBuilds = 2;
//
//		SettingsBase* reg = CreateSettings();
//		if (!reg)
//		{
//			_ASSERTE(reg!=NULL);
//		}
//		else
//		{
//			if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
//			{
//				reg->Save(L"Update.CheckOnStartup", UpdSet.isUpdateCheckOnStartup);
//				reg->Save(L"Update.CheckHourly", UpdSet.isUpdateCheckHourly);
//				reg->Save(L"Update.ConfirmDownload", UpdSet.isUpdateConfirmDownload);
//				reg->Save(L"Update.UseBuilds", UpdSet.isUpdateUseBuilds);
//				reg->CloseKey();
//			}
//
//			delete reg;
//		}
//	}
//
//	return (UpdSet.isUpdateUseBuilds != 0);
//}

bool Settings::IsHostkey(WORD vk)
{
	for(int i=0; i < 15 && mn_HostModOk[i]; i++)
		if (mn_HostModOk[i] == vk)
			return true;

	return false;
}

// Если есть vk - заменить на vkNew
void Settings::ReplaceHostkey(BYTE vk, BYTE vkNew)
{
	for(int i = 0; i < 15; i++)
	{
		if (gpSet->mn_HostModOk[i] == vk)
		{
			gpSet->mn_HostModOk[i] = vkNew;
			return;
		}
	}
}

void Settings::AddHostkey(BYTE vk)
{
	for(int i = 0; i < 15; i++)
	{
		if (gpSet->mn_HostModOk[i] == vk)
			break; // уже есть

		if (!gpSet->mn_HostModOk[i])
		{
			gpSet->mn_HostModOk[i] = vk; // добавить
			break;
		}
	}
}

BYTE Settings::CheckHostkeyModifier(BYTE vk)
{
	// Если передан VK_NULL
	if (!vk)
		return 0;

	switch(vk)
	{
		case VK_LWIN: case VK_RWIN:

			if (gpSet->IsHostkey(VK_RWIN))
				ReplaceHostkey(VK_RWIN, VK_LWIN);

			vk = VK_LWIN; // Сохраняем только Левый-Win
			break;
		case VK_APPS:
			break; // Это - ок
		case VK_LSHIFT:

			if (gpSet->IsHostkey(VK_RSHIFT) || gpSet->IsHostkey(VK_SHIFT))
			{
				vk = VK_SHIFT;
				ReplaceHostkey(VK_RSHIFT, VK_SHIFT);
			}

			break;
		case VK_RSHIFT:

			if (gpSet->IsHostkey(VK_LSHIFT) || gpSet->IsHostkey(VK_SHIFT))
			{
				vk = VK_SHIFT;
				ReplaceHostkey(VK_LSHIFT, VK_SHIFT);
			}

			break;
		case VK_SHIFT:

			if (gpSet->IsHostkey(VK_LSHIFT))
				ReplaceHostkey(VK_LSHIFT, VK_SHIFT);
			else if (gpSet->IsHostkey(VK_RSHIFT))
				ReplaceHostkey(VK_RSHIFT, VK_SHIFT);

			break;
		case VK_LMENU:

			if (gpSet->IsHostkey(VK_RMENU) || gpSet->IsHostkey(VK_MENU))
			{
				vk = VK_MENU;
				ReplaceHostkey(VK_RMENU, VK_MENU);
			}

			break;
		case VK_RMENU:

			if (gpSet->IsHostkey(VK_LMENU) || gpSet->IsHostkey(VK_MENU))
			{
				vk = VK_MENU;
				ReplaceHostkey(VK_LMENU, VK_MENU);
			}

			break;
		case VK_MENU:

			if (gpSet->IsHostkey(VK_LMENU))
				ReplaceHostkey(VK_LMENU, VK_MENU);
			else if (gpSet->IsHostkey(VK_RMENU))
				ReplaceHostkey(VK_RMENU, VK_MENU);

			break;
		case VK_LCONTROL:

			if (gpSet->IsHostkey(VK_RCONTROL) || gpSet->IsHostkey(VK_CONTROL))
			{
				vk = VK_CONTROL;
				ReplaceHostkey(VK_RCONTROL, VK_CONTROL);
			}

			break;
		case VK_RCONTROL:

			if (gpSet->IsHostkey(VK_LCONTROL) || gpSet->IsHostkey(VK_CONTROL))
			{
				vk = VK_CONTROL;
				ReplaceHostkey(VK_LCONTROL, VK_CONTROL);
			}

			break;
		case VK_CONTROL:

			if (gpSet->IsHostkey(VK_LCONTROL))
				ReplaceHostkey(VK_LCONTROL, VK_CONTROL);
			else if (gpSet->IsHostkey(VK_RCONTROL))
				ReplaceHostkey(VK_RCONTROL, VK_CONTROL);

			break;
	}

	// Добавить в список входящих в Host
	AddHostkey(vk);
	// Вернуть (возможно измененный) VK
	return vk;
}

bool Settings::TestHostkeyModifiers()
{
	memset(mn_HostModOk, 0, sizeof(mn_HostModOk));
	memset(mn_HostModSkip, 0, sizeof(mn_HostModSkip));

	if (!nMultiHotkeyModifier)
		nMultiHotkeyModifier = VK_LWIN;

	BYTE vk;
	vk = (nMultiHotkeyModifier & 0xFF);
	CheckHostkeyModifier(vk);
	vk = (nMultiHotkeyModifier & 0xFF00) >> 8;
	CheckHostkeyModifier(vk);
	vk = (nMultiHotkeyModifier & 0xFF0000) >> 16;
	CheckHostkeyModifier(vk);
	vk = (nMultiHotkeyModifier & 0xFF000000) >> 24;
	CheckHostkeyModifier(vk);
	// Однако, допустимо не более 3-х клавиш (больше смысла не имеет)
	TrimHostkeys();
	// Сформировать (возможно скорректированную) маску HostKey
	bool lbChanged = MakeHostkeyModifier();
	return lbChanged;
}

bool Settings::MakeHostkeyModifier()
{
	bool lbChanged = false;
	// Сформировать (возможно скорректированную) маску HostKey
	DWORD nNew = 0;

	if (gpSet->mn_HostModOk[0])
		nNew |= gpSet->mn_HostModOk[0];

	if (gpSet->mn_HostModOk[1])
		nNew |= ((DWORD)(gpSet->mn_HostModOk[1])) << 8;

	if (gpSet->mn_HostModOk[2])
		nNew |= ((DWORD)(gpSet->mn_HostModOk[2])) << 16;

	if (gpSet->mn_HostModOk[3])
		nNew |= ((DWORD)(gpSet->mn_HostModOk[3])) << 24;

	TODO("!!! Добавить в mn_HostModSkip те VK, которые отсутствуют в mn_HostModOk");

	if (gpSet->nMultiHotkeyModifier != nNew)
	{
		gpSet->nMultiHotkeyModifier = nNew;
		lbChanged = true;
	}

	return lbChanged;
}

// Оставить в mn_HostModOk только 3 VK
void Settings::TrimHostkeys()
{
	if (gpSet->mn_HostModOk[0] == 0)
		return;

	int i = 0;

	while (++i < 15 && gpSet->mn_HostModOk[i])
		;

	// Если вдруг задали более 3-х модификаторов - обрезать, оставив 3 последних
	if (i > 3)
	{
		if (i >= (int)countof(gpSet->mn_HostModOk))
		{
			_ASSERTE(i < countof(gpSet->mn_HostModOk));
			i = countof(gpSet->mn_HostModOk) - 1;
		}
		memmove(gpSet->mn_HostModOk, gpSet->mn_HostModOk+i-3, 3); //-V112
	}

	memset(gpSet->mn_HostModOk+3, 0, sizeof(gpSet->mn_HostModOk)-3);
}

bool Settings::isHostkeySingleLR(WORD vk, WORD vkC, WORD vkL, WORD vkR)
{
	if (vk == vkC)
	{
		bool bLeft  = isPressed(vkL);
		bool bRight = isPressed(vkR);

		if (bLeft && !bRight)
			return (nMultiHotkeyModifier == vkL);

		if (bRight && !bLeft)
			return (nMultiHotkeyModifier == vkR);

		// нажатие обоих шифтов - игнорируем
		return false;
	}

	if (vk == vkL)
		return (nMultiHotkeyModifier == vkL);

	if (vk == vkR)
		return (nMultiHotkeyModifier == vkR);

	return false;
}

bool Settings::IsHostkeySingle(WORD vk)
{
	if (nMultiHotkeyModifier > 0xFF)
		return false; // в Host-комбинации больше одной клавиши

	if (vk == VK_LWIN || vk == VK_RWIN)
		return (nMultiHotkeyModifier == VK_LWIN);

	if (vk == VK_APPS)
		return (nMultiHotkeyModifier == VK_APPS);

	if (vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT)
		return isHostkeySingleLR(vk, VK_SHIFT, VK_LSHIFT, VK_RSHIFT);

	if (vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL)
		return isHostkeySingleLR(vk, VK_CONTROL, VK_LCONTROL, VK_RCONTROL);

	if (vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU)
		return isHostkeySingleLR(vk, VK_MENU, VK_LMENU, VK_RMENU);

	return false;
}

// набор флагов MOD_xxx для RegisterHotKey
UINT Settings::GetHostKeyMod()
{
	UINT nMOD = 0;

	for(int i=0; i < 15 && mn_HostModOk[i]; i++)
	{
		switch(mn_HostModOk[i])
		{
			case VK_LWIN: case VK_RWIN:
				nMOD |= MOD_WIN;
				break;
			case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
				nMOD |= MOD_CONTROL;
				break;
			case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
				nMOD |= MOD_SHIFT;
				break;
			case VK_MENU: case VK_LMENU: case VK_RMENU:
				nMOD |= MOD_ALT;
				break;
		}
	}

	if (!nMOD)
		nMOD = MOD_WIN;

	return nMOD;
}

WORD Settings::GetPressedHostkey()
{
	_ASSERTE(mn_HostModOk[0]!=0);

	if (mn_HostModOk[0] == VK_LWIN)
	{
		if (isPressed(VK_LWIN))
			return VK_LWIN;

		if (isPressed(VK_RWIN))
			return VK_RWIN;
	}

	if (!isPressed(mn_HostModOk[0]))
	{
		_ASSERT(FALSE);
		return 0;
	}

	// Для правых-левых - возвращаем общий, т.к. именно он приходит в WM_KEYUP
	if (mn_HostModOk[0] == VK_LSHIFT || mn_HostModOk[0] == VK_RSHIFT)
		return VK_SHIFT;

	if (mn_HostModOk[0] == VK_LMENU || mn_HostModOk[0] == VK_RMENU)
		return VK_MENU;

	if (mn_HostModOk[0] == VK_LCONTROL || mn_HostModOk[0] == VK_RCONTROL)
		return VK_CONTROL;

	return mn_HostModOk[0];
}

bool Settings::IsHostkeyPressed()
{
	if (mn_HostModOk[0] == 0)
	{
		_ASSERTE(mn_HostModOk[0]!=0);
		mn_HostModOk[0] = VK_LWIN;
		return isPressed(VK_LWIN) || isPressed(VK_RWIN);
	}

	// Не более 3-х модификаторов + кнопка
	_ASSERTE(mn_HostModOk[4] == 0);
	for(int i = 0; i < 4 && mn_HostModOk[i]; i++) //-V112
	{
		if (mn_HostModOk[i] == VK_LWIN)
		{
			if (!(isPressed(VK_LWIN) || isPressed(VK_RWIN)))
				return false;
		}
		else if (!isPressed(mn_HostModOk[i]))
		{
			return false;
		}
	}

	// Не более 3-х модификаторов + кнопка
	for(int j = 0; j < 4 && mn_HostModSkip[j]; j++) //-V112
	{
		if (isPressed(mn_HostModSkip[j]))
			return false;
	}

	return true;
}

LPCTSTR Settings::GetCmd()
{
	if (psCurCmd && *psCurCmd)
		return psCurCmd;

	if (psCmd && *psCmd)
		return psCmd;

	SafeFree(psCurCmd); // впринципе, эта строка скорее всего не нужна, но на всякий случай...
	// Хорошо бы более корректно определить версию фара, но это не всегда просто
	// Например x64 файл сложно обработать в x86 ConEmu.
	DWORD nFarSize = 0;

	if (lstrcmpi(gpSetCls->GetDefaultCmd(), L"far") == 0)
	{
		// Ищем фар. (1) В папке ConEmu, (2) в текущей директории, (2) на уровень вверх от папки ConEmu
		wchar_t szFar[MAX_PATH*2], *pszSlash;
		szFar[0] = L'"';
		wcscpy_add(1, szFar, gpConEmu->ms_ConEmuExeDir); // Теперь szFar содержит путь запуска программы
		pszSlash = szFar + _tcslen(szFar);
		_ASSERTE(pszSlash > szFar);
		BOOL lbFound = FALSE;

		// (1) В папке ConEmu
		if (!lbFound)
		{
			wcscpy_add(pszSlash, szFar, L"\\Far.exe");

			if (FileExists(szFar+1, &nFarSize))
				lbFound = TRUE;
		}

		// (2) в текущей директории
		if (!lbFound && lstrcmpi(gpConEmu->ms_ConEmuCurDir, gpConEmu->ms_ConEmuExeDir))
		{
			szFar[0] = L'"';
			wcscpy_add(1, szFar, gpConEmu->ms_ConEmuCurDir);
			wcscat_add(1, szFar, L"\\Far.exe");

			if (FileExists(szFar+1, &nFarSize))
				lbFound = TRUE;
		}

		// (3) на уровень вверх
		if (!lbFound)
		{
			szFar[0] = L'"';
			wcscpy_add(1, szFar, gpConEmu->ms_ConEmuExeDir);
			pszSlash = szFar + _tcslen(szFar);
			*pszSlash = 0;
			pszSlash = wcsrchr(szFar, L'\\');

			if (pszSlash)
			{
				wcscpy_add(pszSlash+1, szFar, L"Far.exe");

				if (FileExists(szFar+1, &nFarSize))
					lbFound = TRUE;
			}
		}

		if (lbFound)
		{
			// 110124 - нафиг, если пользователю надо - сам или параметр настроит, или реестр
			//// far чаще всего будет установлен в "Program Files", поэтому для избежания проблем - окавычиваем
			//// Пока тупо - если far.exe > 1200K - считаем, что это Far2
			//wcscat_c(szFar, (nFarSize>1228800) ? L"\" /w" : L"\"");
			wcscat_c(szFar, L"\"");

			// Finally - Result
			psCurCmd = lstrdup(szFar);
		}
		else
		{
			// Если Far.exe не найден рядом с ConEmu - запустить cmd.exe
			psCurCmd = GetComspec(&ComSpec);
			//wcscpy_c(szFar, L"cmd");
		}

	}
	else
	{
		// Simple Copy
		psCurCmd = lstrdup(gpSetCls->GetDefaultCmd());
	}

	return psCurCmd;
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

void Settings::HistoryAdd(LPCWSTR asCmd)
{
	// Группы и так отображаются в диалоге/меню. В историю их не пишем
	if (!asCmd || !*asCmd || (*asCmd == TaskBracketLeft))
		return;

	if (psCmd && lstrcmp(psCmd, asCmd)==0)
		return;

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
	free(psCmdHistory);
	psCmdHistory = pszNewHistory;
	nCmdHistorySize = (psz - pszNewHistory + 1)*sizeof(wchar_t);
	HEAPVAL;
	// И сразу сохранить в настройках
	SettingsBase* reg = CreateSettings();

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

//BOOL Settings::CheckConIme()
//{
//	if (!(gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 0))
//		return FALSE; // Проверять только в Vista
//
//	long  lbStopWarning = FALSE;
//	DWORD dwValue=1;
//	SettingsBase* reg = CreateSettings();
//
//	// БЕЗ имени конфигурации!
//	if (reg->OpenKey(CONEMU_ROOT_KEY, KEY_READ))
//	{
//		if (!reg->Load(_T("StopWarningConIme"), lbStopWarning))
//			lbStopWarning = FALSE;
//
//		reg->CloseKey();
//	}
//
//	if (!lbStopWarning)
//	{
//		HKEY hk = NULL;
//
//		if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Console", 0, KEY_READ, &hk))
//		{
//			DWORD dwType = REG_DWORD, nSize = sizeof(DWORD);
//
//			if (0 != RegQueryValueEx(hk, L"LoadConIme", 0, &dwType, (LPBYTE)&dwValue, &nSize))
//				dwValue = 1;
//
//			RegCloseKey(hk);
//
//			if (dwValue!=0)
//			{
//				if (IDCANCEL==MessageBox(0,
//				                        L"Unwanted value of 'LoadConIme' registry parameter!\r\n"
//				                        L"Press 'Cancel' to stop this message.\r\n"
//				                        L"Take a look at 'FAQ-ConEmu.txt'.\r\n"
//				                        L"You may simply import file 'Disable_ConIme.reg'\r\n"
//				                        L"located in 'ConEmu.Addons' folder.",
//				                        gpConEmu->GetDefaultTitle(),MB_OKCANCEL|MB_ICONEXCLAMATION))
//					lbStopWarning = TRUE;
//			}
//		}
//		else
//		{
//			if (IDCANCEL==MessageBox(0,
//			                        L"Can't determine a value of 'LoadConIme' registry parameter!\r\n"
//			                        L"Press 'Cancel' to stop this message.\r\n"
//			                        L"Take a look at 'FAQ-ConEmu.txt'",
//			                        gpConEmu->GetDefaultTitle(),MB_OKCANCEL|MB_ICONEXCLAMATION))
//				lbStopWarning = TRUE;
//		}
//
//		if (lbStopWarning)
//		{
//			// БЕЗ имени конфигурации!
//			if (reg->OpenKey(CONEMU_ROOT_KEY, KEY_WRITE))
//			{
//				reg->Save(_T("StopWarningConIme"), lbStopWarning);
//				reg->CloseKey();
//			}
//		}
//	}
//
//	delete reg;
//	return TRUE;
//}

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

SettingsBase* Settings::CreateSettings()
{
#ifndef __GNUC__
	SettingsBase* pReg = NULL;
	BOOL lbXml = FALSE;
	DWORD dwAttr = -1;
	LPWSTR pszXmlFile = gpConEmu->ConEmuXml();

	if (pszXmlFile && *pszXmlFile)
	{
		dwAttr = GetFileAttributes(pszXmlFile);

		if (dwAttr != (DWORD)-1 && !(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
			lbXml = TRUE;
	}

	if (lbXml)
	{
		pReg = new SettingsXML();

		if (!((SettingsXML*)pReg)->IsXmlAllowed())
		{
			// Если MSXml.DomDocument не зарегистрирован
			pszXmlFile[0] = 0;
			lbXml = FALSE;
		}
	}

	if (!lbXml)
		pReg = new SettingsRegistry();

	return pReg;
#else
	return new SettingsRegistry();
#endif
}

void Settings::GetSettingsType(wchar_t (&szType)[8], bool& ReadOnly)
{
	const wchar_t* pszType = L"[reg]";
	ReadOnly = false;

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
			pszType = L"[xml]";
			// Проверим, сможем ли мы в него записать
			hFile = CreateFile(pszXmlFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
			                   NULL, OPEN_EXISTING, 0, 0);

			if (hFile != INVALID_HANDLE_VALUE)
			{
				CloseHandle(hFile); hFile = NULL; // OK
			}
			else
			{
				//EnableWindow(GetDlgItem(ghOpWnd, bSaveSettings), FALSE); // Сохранение запрещено
				ReadOnly = true;
			}
		}
	}
#endif

	wcscpy_c(szType, pszType);
}

bool Settings::isHideCaptionAlways()
{
	return mb_HideCaptionAlways || (!mb_HideCaptionAlways && isUserScreenTransparent);
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
			_ASSERTE(pch && *pch);
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
		CEAppColors** NewAppColors = (CEAppColors**)calloc(NewAppCount, sizeof(*NewAppColors));
		if (!NewApps || !NewAppColors)
		{
			_ASSERTE(NewApps && NewAppColors);
			return NULL;
		}
		if (Apps && (AppCount > 0))
		{
			memmove(NewApps, Apps, AppCount*sizeof(*NewApps));
			memmove(NewAppColors, AppColors, AppCount*sizeof(*NewAppColors));
		}
		AppSettings** pOld = Apps;
		CEAppColors** pOldColors = AppColors;
		Apps = NewApps;
		AppColors = NewAppColors;
		SafeFree(pOld);
		SafeFree(pOldColors);
		
		Apps[anAppId] = (AppSettings*)calloc(1,sizeof(AppSettings));
		AppColors[anAppId] = (CEAppColors*)calloc(1,sizeof(CEAppColors));

		if (!Apps[anAppId] || !AppColors[anAppId])
		{
			_ASSERTE(Apps[anAppId]!=NULL && AppColors[anAppId]!=NULL);
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
	if (!Apps || !AppColors || (anAppId < 0) || (anAppId >= AppCount))
	{
		_ASSERTE(Apps && AppColors && (anAppId >= 0) && (anAppId < AppCount));
		return;
	}

	AppSettings* pOld = Apps[anAppId];
	CEAppColors* pOldClr = AppColors[anAppId];

	for (int i = anAppId+1; i < AppCount; i++)
	{
		Apps[i-1] = Apps[i];
		AppColors[i-1] = AppColors[i];
	}

	_ASSERTE(AppCount>0);
	AppCount--;

	if (pOld)
	{
		pOld->FreeApps();
		free(pOld);
	}
	SafeFree(pOldClr);
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

	CEAppColors* pClr = AppColors[anIndex1];
	AppColors[anIndex1] = AppColors[anIndex2];
	AppColors[anIndex2] = pClr;

	return true;
}

void Settings::ResetFadeColors()
{
	mn_LastFadeSrc = mn_LastFadeDst = -1;
	mb_FadeInitialized = false;

	for (int i = 0; i < AppCount; i++)
	{
		AppColors[i]->FadeInitialized = false;
	}
}

COLORREF* Settings::GetColors(int anAppId/*=-1*/, BOOL abFade)
{
	COLORREF *pColors = Colors, *pColorsFade = ColorsFade;
	bool* pbFadeInitialized = &mb_FadeInitialized;
	if ((anAppId >= 0) && (anAppId < AppCount) && Apps[anAppId]->OverridePalette)
	{
		_ASSERTE(countof(Colors)==countof(AppColors[anAppId]->Colors) && countof(ColorsFade)==countof(AppColors[anAppId]->ColorsFade));
		pColors = AppColors[anAppId]->Colors; pColorsFade = AppColors[anAppId]->ColorsFade;
		pbFadeInitialized = &AppColors[anAppId]->FadeInitialized;
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

bool Settings::isModifierPressed(DWORD vk)
{
	// если НЕ 0 - должен быть нажат
	if (vk)
	{
		if (!isPressed(vk))
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
	// Пока что, окно для Application нужно создавать только для XP и ниже
	// в том случае, если на таскбаре отображаются кнопки запущенных консолей
	// Это для того, чтобы при Alt-Tab не светилась "лишняя" иконка главного окна
	if (!IsWindows7 && isTabsOnTaskBar())
		return TRUE;
	return FALSE;
}

bool Settings::isTabsOnTaskBar()
{
	if (isDesktopMode)
		return false;
	if ((m_isTabsOnTaskBar == 1) || (((BYTE)m_isTabsOnTaskBar > 1) && IsWindows7))
		return true;
	return false;
}

bool Settings::isRClickTouchInvert()
{
	return gpConEmu->IsGesturesEnabled() && (isRClickSendKey == 2);
}

LPCWSTR Settings::RClickMacro()
{
	if (sRClickMacro && *sRClickMacro)
		return sRClickMacro;
	return RClickMacroDefault();
}

LPCWSTR Settings::RClickMacroDefault()
{
	// L"@$If (!CmdLine.Empty) %Flg_Cmd=1; %CmdCurPos=CmdLine.ItemCount-CmdLine.CurPos+1; %CmdVal=CmdLine.Value; Esc $Else %Flg_Cmd=0; $End $Text \"rclk_gui:\" Enter $If (%Flg_Cmd==1) $Text %CmdVal %Flg_Cmd=0; %Num=%CmdCurPos; $While (%Num!=0) %Num=%Num-1; CtrlS $End $End"
	static LPCWSTR pszDefaultMacro = FarRClickMacroDefault;
	return pszDefaultMacro;
}

LPCWSTR Settings::SafeFarCloseMacro()
{
	if (sSafeFarCloseMacro && *sSafeFarCloseMacro)
		return sSafeFarCloseMacro;
	return SafeFarCloseMacroDefault();
}

LPCWSTR Settings::SafeFarCloseMacroDefault()
{
	// L"@$while (Dialog||Editor||Viewer||Menu||Disks||MainMenu||UserMenu||Other||Help) $if (Editor) ShiftF10 $else Esc $end $end  Esc  $if (Shell) F10 $if (Dialog) Enter $end $Exit $end  F10"
	static LPCWSTR pszDefaultMacro = FarSafeCloseMacroDefault;
	return pszDefaultMacro;
}

LPCWSTR Settings::TabCloseMacro()
{
	if (sTabCloseMacro && *sTabCloseMacro)
		return sTabCloseMacro;
	return TabCloseMacroDefault();
}

LPCWSTR Settings::TabCloseMacroDefault()
{
	// L"@$if (Shell) F10 $if (Dialog) Enter $end $else F10 $end";
	static LPCWSTR pszDefaultMacro = FarTabCloseMacroDefault;
	return pszDefaultMacro;
}

LPCWSTR Settings::SaveAllMacro()
{
	if (sSaveAllMacro && *sSaveAllMacro)
		return sSaveAllMacro;
	return SaveAllMacroDefault();
}

LPCWSTR Settings::SaveAllMacroDefault()
{
	// L"@F2 $If (!Editor) $Exit $End %i0=-1; F12 %cur = CurPos; Home Down %s = Menu.Select(\" * \",3,2); $While (%s > 0) $If (%s == %i0) MsgBox(\"FAR SaveAll\",\"Asterisk in menuitem for already processed window\",0x10001) $Exit $End Enter $If (Editor) F2 $If (!Editor) $Exit $End $Else $If (!Viewer) $Exit $End $End %i0 = %s; F12 %s = Menu.Select(\" * \",3,2); $End $If (Menu && Title==\"Screens\") Home $Rep (%cur-1) Down $End Enter $End $Exit"
	static LPCWSTR pszDefaultMacro = FarSaveAllMacroDefault;
	return pszDefaultMacro;
}

const Settings::CommandTasks* Settings::CmdTaskGet(int anIndex)
{
	if (!CmdTasks || (anIndex < 0) || (anIndex >= CmdTaskCount))
		return NULL;
	return (CmdTasks[anIndex]);
}

// anIndex - 0-based, index of CmdTasks
// asName - имя, или NULL, если эту группу нужно удалить (хвост сдвигается вверх)
// asCommands - список команд (скрипт)
void Settings::CmdTaskSet(int anIndex, LPCWSTR asName, LPCWSTR asCommands)
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
