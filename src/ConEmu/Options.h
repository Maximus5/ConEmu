
/*
Copyright (c) 2009-2016 Maximus5
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

#define MAX_CONFIG_NAME 240

#define MIN_ALPHA_VALUE 40
#define MIN_INACTIVE_ALPHA_VALUE 0
#define MAX_ALPHA_VALUE 255

// That is positive value for LF.lfHeight
#define DEF_INSIDESIZEY_P 10
#define DEF_FONTSIZEY_P   16
#define DEF_TABFONTY_P    16
#define DEF_STATUSFONTY_P 14
// That is positive value for LF.lfHeight
#define DEF_INSIDESIZEY_U 9
#define DEF_FONTSIZEY_U   14
#define DEF_TABFONTY_U    13
#define DEF_STATUSFONTY_U 12

//TODO: #define MAX_FONT_GROUPS 20 // Main, Borders, Japan, Cyrillic, ...
// #define MAX_FONT_STYLES 8 //normal/(bold|italic|underline)
enum CEFontStyles
{
	fnt_Normal    = 0,
	fnt_Bold      = 1,
	fnt_Italic    = 2,
	fnt_Underline = 4,

	fnt_StdFontMask = (fnt_Bold|fnt_Italic|fnt_Underline), // 7
	MAX_FONT_STYLES = (fnt_StdFontMask+1), // 8

	fnt_UCharMap    = (MAX_FONT_STYLES),   // 8

	fnt_Alternative = (MAX_FONT_STYLES+1), // 9 (old fnt_FarBorders)

	fnt_NULL        = (MAX_FONT_STYLES+2), // 10 (used in VirtualConsole SelectFont)
};

#define CURSORSIZE_MIN 5
#define CURSORSIZE_MAX 100
#define CURSORSIZEPIX_MIN 1
#define CURSORSIZEPIX_STD 2
#define CURSORSIZEPIX_MAX 99

// Animation delay used not only in Quake mode,
// but also when hiding/restoring window to/from TSA
#define QUAKEANIMATION_DEF 300
#define QUAKEANIMATION_MAX 2000
#define QUAKEVISIBLELIMIT 80 // Если "Видимая область" окна стала менее (%) - считаем что окно стало "не видимым"
#define QUAKEVISIBLETRASH 10 // Не "выезжать" а просто "вынести наверх", если видимая область достаточно большая

#define HIDECAPTIONALWAYSFRAME_DEF 255
#define HIDECAPTIONALWAYSFRAME_MAX 0x7F

#define RUNQUEUE_CREATE_LAG_DEF 100
#define RUNQUEUE_CREATE_LAG_MIN 10 // 10 ms
#define RUNQUEUE_CREATE_LAG_MAX 10*1000 // 10 sec

enum FarMacroVersion
{
	fmv_Default = 0,
	fmv_Standard,
	fmv_Lua,
};

// -> Header.h
//enum BackgroundOp
//{
//	eUpLeft = 0,
//	eStretch = 1,
//	eTile = 2,
//	eUpRight = 3,
//	eDownLeft = 4,
//	eDownRight = 5,
//};

#define BgImageColorsDefaults (1|2)
#define DefaultSelectionConsoleColor 0xE0

#include "OptionTypes.h"
#include "Hotkeys.h"
#include "SetTypes.h"
#include "SetAppSettings.h"
#include "SetCmdTask.h"
#include "UpdateSet.h"
#include "../common/wcwidth.h"

class CSettings;
class CSetDlgButtons;
struct CommandHistory;
struct ColorPalette;


#define SCROLLBAR_DELAY_MIN 100
#define SCROLLBAR_DELAY_MAX 15000

#define CENTERCONSOLEPAD_MIN 0
#define CENTERCONSOLEPAD_MAX 64


#define DefaultAdminTitleSuffix L" (Admin)"

#define DEFAULT_TERMINAL_APPS L"explorer.exe"

#define TABBAR_DEFAULT_CLICK_ACTION 1
#define TABBTN_DEFAULT_CLICK_ACTION 0

// ‘%1’ - line number, ‘%2’ - column number, ‘%3’ - C:\\Path\\File, ‘%4’ - C:/Path/File, ‘%5’ - /C/Path/File
#define HI_GOTO_EDITOR_FAR     L"far.exe /e%1:%2 \"%3\""
#define HI_GOTO_EDITOR_VIMW    L"vim.exe +%1 \"%3\""
// Use '#' prefix to run GUI editor outside of ConEmu
#define HI_GOTO_EDITOR_SCITE   L"#scite.exe \"-open:%4\" -goto:%1,%2"
#define HI_GOTO_EDITOR_NPADP   L"#notepad++.exe -n%1 \"%3\""
#define HI_GOTO_EDITOR_SUBLM   L"#sublime_text.exe \"%3:%1:%2\""
// And just a starter for highlighted file
#define HI_GOTO_EDITOR_CMD     L"cmd.exe /c \"echo Starting \"%3\" & \"%3\"\""
#define HI_GOTO_EDITOR_CMD_NC  L"cmd.exe /c \"echo Starting \"%3\" & \"%3\"\" -new_console:n"


extern const wchar_t gsDefaultColorScheme[64]; // = L"<ConEmu>";


class Settings
{
	private:
		CEOptionList* mp_OptionList;
	public:
		Settings();
		~Settings();
	protected:
		friend class CSettings;
		friend class CSetDlgButtons;
		friend class CSetDlgColors;
		friend class CSetPgApps;
		friend class CSetPgMarkCopy;
		friend class CSetPgColors;
		friend class CSetPgKeys;

		void ResetSettings();
		void ReleasePointers();
	public:

		wchar_t Type[16]; // Информационно: L"[reg]" или L"[xml]"

		bool IsConfigNew; // true, если конфигурация новая
		bool IsConfigPartial; // true, if config has no task or start command

	public:
		enum TabsOnTaskbar { tot_ConEmuOnly = 0, tot_AllTabsAllOS = 1, tot_AllTabsWin7 = 2, tot_DontShow = 3};

	protected:
		/* *** helper methods for option initializations *** */
		// isStatusColumnHidden
		static void StatusColumnName(int index);
		static void StatusColumnDefaults(CEOptionArray<bool,csi_Last>::arr_type& columns);

	public:
		/* *** some short names *** */
		using CEOptionStr16 = CEOptionStrFix<16>;
		using CEOptionStr32 = CEOptionStrFix<32>;
		using CEOptionStr64 = CEOptionStrFix<64>;
		using CEOptionFName = CEOptionStrFix<LF_FACESIZE>;

	public:
		public:  CEOptionInt    DefaultBufferHeight = CEOptionInt(L"DefaultBufferHeight", 1000);
		public:  CEOptionBool   AutoBufferHeight = CEOptionBool(L"AutoBufferHeight", true); // Long console output
		public:  CEOptionBool   UseScrollLock = CEOptionBool(L"UseScrollLock", true);
		public:  CEOptionInt    nCmdOutputCP = CEOptionInt(L"CmdOutputCP", 0);
		public:  CEOptionComSpec ComSpec; // Defaults are set in CEOptionComSpec::Reset
		public:  CEOptionStringDelim DefaultTerminalApps = CEOptionStringDelim(L"DefaultTerminalApps", L"explorer.exe"); // Stored as "|"-delimited string
		// #OPT_TODO ColorTableIndexName - not implemented
		//private: CEOptionArray<COLORREF, 0x20, ColorTableIndexName/*"ColorTableNN" decimal*/, ColorTableDefaults/*<ConEmu>*/> Colors; // L"ColorTableNN", Colors[i]
		public:  CEOptionBool   isFontAutoSize = CEOptionBool(L"FontAutoSize", false);
		public:  CEOptionBool   isAutoRegisterFonts = CEOptionBool(L"AutoRegisterFonts", true);
		public:  CEOptionBool   isTrueColorer = CEOptionBool(L"TrueColorerSupport", true);
		public:  CEOptionBool   isVividColors = CEOptionBool(L"VividColors", true);
		public:  CEOptionByte   isShowBgImage = CEOptionByte(L"BackGround Image show", 0);
		public:  CEOptionStrFix sBgImage = CEOptionStrFix(L"BackGround Image", MAX_PATH, L"%USERPROFILE%\\back.jpg");
		public:  CEOptionByte   bgImageDarker = CEOptionByte(L"bgImageDarker", 255);
		public:  CEOptionDWORD  nBgImageColors = CEOptionDWORD(L"bgImageColors", (DWORD)-1);
		public:  CEOptionEnum<BackgroundOp> bgOperation = CEOptionEnum<BackgroundOp>(L"bgOperation", eUpLeft); // BackgroundOp {eUpLeft = 0, eStretch = 1, eTile = 2, ...}
		public:  CEOptionByte   isBgPluginAllowed = CEOptionByte(L"bgPluginAllowed", 1);
		public:  CEOptionByte   nTransparent = CEOptionByte(L"AlphaValue", 255);
		public:  CEOptionBool   isTransparentSeparate = CEOptionBool(L"AlphaValueSeparate", false);
		public:  CEOptionByte   nTransparentInactive = CEOptionByte(L"AlphaValueInactive", 255);
		public:  CEOptionBool   isUserScreenTransparent = CEOptionBool(L"UserScreenTransparent", false);
		public:  CEOptionBool   isColorKeyTransparent = CEOptionBool(L"ColorKeyTransparent", false);
		public:  CEOptionDWORD  nColorKeyValue = CEOptionDWORD(L"ColorKeyValue", RGB(1,1,1));
		public:  CEOptionBool   isSaveCmdHistory = CEOptionBool(L"SaveCmdHistory", true);
		// #OPT_TODO CEOPTIONV(public, CEOptionString, psHistoryLocation, L"CmdHistoryLocation");
		public:  CEOptionEnum<StartupType> nStartType = CEOptionEnum<StartupType>(L"StartType", start_Command); // !!! POST VALIDATION IS REQUIRED !!!
		public:  CEOptionString psStartSingleApp = CEOptionString(L"CmdLine");
		public:  CEOptionString psStartTasksFile = CEOptionString(L"StartTasksFile");
		public:  CEOptionString psStartTasksName = CEOptionString(L"StartTasksName");
		public:  CEOptionBool   isStartFarFolders = CEOptionBool(L"StartFarFolders");
		public:  CEOptionBool   isStartFarEditors = CEOptionBool(L"StartFarEditors");
		public:  CEOptionUInt   nStartCreateDelay = CEOptionUInt(L"StartCreateDelay", RUNQUEUE_CREATE_LAG_DEF); // RUNQUEUE_CREATE_LAG
		public:  CEOptionBool   isStoreTaskbarkTasks = CEOptionBool(L"StoreTaskbarkTasks");
		public:  CEOptionBool   isStoreTaskbarCommands = CEOptionBool(L"StoreTaskbarCommands");
		public:  CEOptionFName  inFont = CEOptionFName(L"FontName");
		public:  CEOptionBool   isBold = CEOptionBool(L"FontBold", false);
		public:  CEOptionBool   isItalic = CEOptionBool(L"FontItalic", false);
		public:  CEOptionUInt   mn_AntiAlias = CEOptionUInt(L"Anti-aliasing", CLEARTYPE_NATURAL_QUALITY, get_antialias_default, antialias_validate);
		public:  CEOptionByte   mn_LoadFontCharSet = CEOptionByte(L"FontCharSet", DEFAULT_CHARSET); // Loaded or Saved to settings // !!! mb_CharSetWasSet = FALSE;
		public:  CEOptionUInt   FontSizeY = CEOptionUInt(L"FontSize", DEF_FONTSIZEY_P);  // высота основного шрифта (загруженная из настроек!)
		public:  CEOptionUInt   FontSizeX = CEOptionUInt(L"FontSizeX");  // ширина основного шрифта
		public:  CEOptionUInt   FontSizeX3 = CEOptionUInt(L"FontSizeX3", 0); // ширина знакоместа при моноширинном режиме (не путать с FontSizeX2)
		public:  CEOptionBool   FontUseDpi = CEOptionBool(L"FontUseDpi", true);
		public:  CEOptionBool   FontUseUnits = CEOptionBool(L"FontUseUnits", false);
		public:  CEOptionBool   isAntiAlias2 = CEOptionBool(L"Anti-aliasing2", false); // disabled by default to avoid dashed framed
		public:  CEOptionBool   isHideCaption = CEOptionBool(L"HideCaption", false); // Hide caption when maximized
		public:  CEOptionBool   isHideChildCaption = CEOptionBool(L"HideChildCaption", true); // Hide caption of child GUI applications, started in ConEmu tabs (PuTTY, Notepad, etc.)
		public:  CEOptionBool   isFocusInChildWindows = CEOptionBool(L"FocusInChildWindows", true);
		public:  CEOptionBool   mb_IntegralSize = CEOptionBool(L"IntegralSize", false);
		public:  CEOptionEnum<ConEmuQuakeMode> isQuakeStyle = CEOptionEnum<ConEmuQuakeMode>(L"QuakeStyle", quake_Disabled);
		public:  CEOptionBool   isRestore2ActiveMon = CEOptionBool(L"Restore2ActiveMon", false);
		private: CEOptionBool   mb_HideCaptionAlways = CEOptionBool(L"HideCaptionAlways", false);
		public:  CEOptionByte   nHideCaptionAlwaysFrame = CEOptionByte(L"HideCaptionAlwaysFrame", HIDECAPTIONALWAYSFRAME_DEF);
		public:  CEOptionUInt   nHideCaptionAlwaysDelay = CEOptionUInt(L"HideCaptionAlwaysDelay", 2000);
		public:  CEOptionUInt   nHideCaptionAlwaysDisappear = CEOptionUInt(L"HideCaptionAlwaysDisappear", 2000);
		public:  CEOptionBool   isDownShowHiddenMessage = CEOptionBool(L"DownShowHiddenMessage", false);
		public:  CEOptionBool   isDownShowExOnTopMessage = CEOptionBool(L"DownShowExOnTopMessage", false);
		public:  CEOptionBool   isAlwaysOnTop = CEOptionBool(L"AlwaysOnTop", false);
		public:  CEOptionBool   isSnapToDesktopEdges = CEOptionBool(L"SnapToDesktopEdges", false);
		public:  CEOptionBool   isExtendUCharMap = CEOptionBool(L"ExtendUCharMap", true); // !!! FAR
		public:  CEOptionBool   isDisableMouse = CEOptionBool(L"DisableMouse", false);
		public:  CEOptionBool   isMouseSkipActivation = CEOptionBool(L"MouseSkipActivation", true);
		public:  CEOptionBool   isMouseSkipMoving = CEOptionBool(L"MouseSkipMoving", true);
		public:  CEOptionBool   isMouseDragWindow = CEOptionBool(L"MouseDragWindow", true);
		public:  CEOptionBool   isFarHourglass = CEOptionBool(L"FarHourglass", true);
		public:  CEOptionUInt   nFarHourglassDelay = CEOptionUInt(L"FarHourglassDelay", 500);
		public:  CEOptionByte   isDisableFarFlashing = CEOptionByte(L"DisableFarFlashing", false); // if (isDisableFarFlashing>2) isDisableFarFlashing = 2;
		public:  CEOptionUInt   isDisableAllFlashing = CEOptionUInt(L"DisableAllFlashing", false); // if (isDisableAllFlashing>2) isDisableAllFlashing = 2;
		public:  CEOptionBool   isCTSIntelligent = CEOptionBool(L"CTSIntelligent", true);
		private: CEOptionStringDelim _pszCTSIntelligentExceptions = CEOptionStringDelim(L"CTSIntelligentExceptions", L"far|vim"); // !!! "|" delimited! // Don't use IntelliSel in these app-processes
		public:  CEOptionBool   isCTSAutoCopy = CEOptionBool(L"CTS.AutoCopy", true);
		public:  CEOptionBool   isCTSResetOnRelease = CEOptionBool(L"CTS.ResetOnRelease", false);
		public:  CEOptionBool   isCTSIBeam = CEOptionBool(L"CTS.IBeam", true);
		public:  CEOptionEnum<CTSEndOnTyping> isCTSEndOnTyping = CEOptionEnum<CTSEndOnTyping>(L"CTS.EndOnTyping", ceot_Off);
		public:  CEOptionBool   isCTSEndOnKeyPress = CEOptionBool(L"CTS.EndOnKeyPress", false); // +isCTSEndOnTyping. +все, что не генерит WM_CHAR (стрелки и пр.)
		public:  CEOptionBool   isCTSEraseBeforeReset = CEOptionBool(L"CTS.EraseBeforeReset", true);
		public:  CEOptionBool   isCTSFreezeBeforeSelect = CEOptionBool(L"CTS.Freeze", false);
		public:  CEOptionBool   isCTSSelectBlock = CEOptionBool(L"CTS.SelectBlock", true);
		public:  CEOptionBool   isCTSSelectText = CEOptionBool(L"CTS.SelectText", true);
		public:  CEOptionEnum<CTSCopyFormat> isCTSHtmlFormat = CEOptionEnum<CTSCopyFormat>(L"CTS.HtmlFormat", CTSFormatText); // MinMax(CTSFormatANSI)
		public:  CEOptionDWORD  isCTSForceLocale = CEOptionDWORD(L"CTS.ForceLocale", RELEASEDEBUGTEST(0,0x0419/*russian in debug*/)); // Try to bypass clipboard locale problems (pasting to old non-unicode apps)
		public:  CEOptionByte   isCTSActMode = CEOptionByte(L"CTS.ActMode"); // режим и модификатор разрешения действий правой и средней кнопки мышки
		public:  CEOptionByte   isCTSRBtnAction = CEOptionByte(L"CTS.RBtnAction"); // enum: 0-off, 1-copy, 2-paste, 3-auto
		public:  CEOptionByte   isCTSMBtnAction = CEOptionByte(L"CTS.MBtnAction"); // enum: 0-off, 1-copy, 2-paste, 3-auto
		public:  CEOptionByte   isCTSColorIndex = CEOptionByte(L"CTS.ColorIndex");
		public:  CEOptionBool   isPasteConfirmEnter = CEOptionBool(L"ClipboardConfirmEnter");
		public:  CEOptionUInt   nPasteConfirmLonger = CEOptionUInt(L"ClipboardConfirmLonger");
		public:  CEOptionBool   isFarGotoEditor = CEOptionBool(L"FarGotoEditorOpt"); // Подсвечивать и переходить на файл/строку (ошибки компилятора)
		public:  CEOptionString sFarGotoEditor = CEOptionString(L"FarGotoEditorPath"); // Команда запуска редактора
		public:  CEOptionBool   isHighlightMouseRow = CEOptionBool(L"HighlightMouseRow");
		public:  CEOptionBool   isHighlightMouseCol = CEOptionBool(L"HighlightMouseCol");
		public:  CEOptionByte   m_isKeyboardHooks = CEOptionByte(L"KeyboardHooks"); // if (m_isKeyboardHooks>2) m_isKeyboardHooks = 0;
		public:  CEOptionByte   isPartBrush75 = CEOptionByte(L"PartBrush75"); // if (isPartBrush75<5) isPartBrush75=5; else if (isPartBrush75>250) isPartBrush75=250;
		public:  CEOptionByte   isPartBrush50 = CEOptionByte(L"PartBrush50"); // if (isPartBrush50<5) isPartBrush50=5; else if (isPartBrush50>250) isPartBrush50=250;
		public:  CEOptionByte   isPartBrush25 = CEOptionByte(L"PartBrush25"); // if (isPartBrush25<5) isPartBrush25=5; else if (isPartBrush25>250) isPartBrush25=250;
		public:  CEOptionByte   isPartBrushBlack = CEOptionByte(L"PartBrushBlack");
		public:  CEOptionByte   isRClickSendKey = CEOptionByte(L"RightClick opens context menu"); // 0 - не звать EMenu, 1 - звать всегда, 2 - звать по длинному клику
		public:  CEOptionString sRClickMacro = CEOptionString(L"RightClickMacro2");
		public:  CEOptionBool   isSafeFarClose = CEOptionBool(L"SafeFarClose");
		public:  CEOptionString sSafeFarCloseMacro = CEOptionString(L"SafeFarCloseMacro");
		public:  CEOptionBool   isSendAltTab = CEOptionBool(L"SendAltTab");
		public:  CEOptionBool   isSendAltEsc = CEOptionBool(L"SendAltEsc");
		public:  CEOptionBool   isSendAltPrintScrn = CEOptionBool(L"SendAltPrintScrn");
		public:  CEOptionBool   isSendPrintScrn = CEOptionBool(L"SendPrintScrn");
		public:  CEOptionBool   isSendCtrlEsc = CEOptionBool(L"SendCtrlEsc");
		public:  CEOptionBool   mb_MinToTray = CEOptionBool(L"Min2Tray");
		public:  CEOptionBool   mb_AlwaysShowTrayIcon = CEOptionBool(L"AlwaysShowTrayIcon");
		public:  CEOptionByte   isMonospace = CEOptionByte(L"Monospace"); // 0 - proportional, 1 - monospace, 2 - forcemonospace
		public:  CEOptionBool   isRSelFix = CEOptionBool(L"RSelectionFix");
		public:  CEOptionByte   isDragEnabled = CEOptionByte(L"Dnd");
		public:  CEOptionByte   isDropEnabled = CEOptionByte(L"DndDrop");
		public:  CEOptionBool   isDefCopy = CEOptionBool(L"DefCopy");
		public:  CEOptionByte   isDropUseMenu = CEOptionByte(L"DropUseMenu");
		public:  CEOptionBool   isDragOverlay = CEOptionBool(L"DragOverlay");
		public:  CEOptionBool   isDragShowIcons = CEOptionBool(L"DragShowIcons");
		public:  CEOptionByte   isDragPanel = CEOptionByte(L"DragPanel"); // if (isDragPanel > 2) isDragPanel = 1; // изменение размера панелей мышкой
		public:  CEOptionBool   isDragPanelBothEdges = CEOptionBool(L"DragPanelBothEdges"); // таскать за обе рамки (правую-левой панели и левую-правой панели)
		public:  CEOptionBool   isKeyBarRClick = CEOptionBool(L"KeyBarRClick"); // Правый клик по кейбару - показать PopupMenu
		public:  CEOptionBool   isDebugSteps = CEOptionBool(L"DebugSteps");
		public:  CEOptionByte   isDebugLog = CEOptionByte(L"DebugLog");
		public:  CEOptionBool   isEnhanceGraphics = CEOptionBool(L"EnhanceGraphics"); // Progressbars and scrollbars (pseudographics)
		public:  CEOptionBool   isEnhanceButtons = CEOptionBool(L"EnhanceButtons"); // Buttons, CheckBoxes and RadioButtons (pseudographics)
		public:  CEOptionBool   isFadeInactive = CEOptionBool(L"FadeInactive");
		private: CEOptionByte   mn_FadeLow = CEOptionByte(L"FadeInactiveLow");
		private: CEOptionByte   mn_FadeHigh = CEOptionByte(L"FadeInactiveHigh");
		public:  CEOptionBool   isStatusBarShow = CEOptionBool(L"StatusBar.Show");
		public:  CEOptionDWORD  isStatusBarFlags = CEOptionDWORD(L"StatusBar.Flags"); // set of CEStatusFlags
		public:  CEOptionFName  sStatusFontFace = CEOptionFName(L"StatusFontFace");
		public:  CEOptionUInt   nStatusFontCharSet = CEOptionUInt(L"StatusFontCharSet");
		public:  CEOptionInt    nStatusFontHeight = CEOptionInt(L"StatusFontHeight");
		public:  CEOptionDWORD  nStatusBarBack = CEOptionDWORD(L"StatusBar.Color.Back");
		public:  CEOptionDWORD  nStatusBarLight = CEOptionDWORD(L"StatusBar.Color.Light");
		public:  CEOptionDWORD  nStatusBarDark = CEOptionDWORD(L"StatusBar.Color.Dark");
		public:  CEOptionArray<bool,csi_Last> isStatusColumnHidden = CEOptionArray<bool,csi_Last>(StatusColumnName, StatusColumnDefaults);
		public:  CEOptionByte   isTabs = CEOptionByte(L"Tabs"); // 0 - don't show, 1 - always show, 2 - auto show
		public:  CEOptionByte   nTabsLocation = CEOptionByte(L"TabsLocation"); // 0 - top, 1 - bottom
		public:  CEOptionBool   isTabIcons = CEOptionBool(L"TabIcons");
		public:  CEOptionBool   isOneTabPerGroup = CEOptionBool(L"OneTabPerGroup");
		public:  CEOptionByte   bActivateSplitMouseOver = CEOptionByte(L"ActivateSplitMouseOver");
		public:  CEOptionBool   isTabSelf = CEOptionBool(L"TabSelf");
		public:  CEOptionBool   isTabRecent = CEOptionBool(L"TabRecent");
		public:  CEOptionBool   isTabLazy = CEOptionBool(L"TabLazy");
		public:  CEOptionInt    nTabFlashChanged = CEOptionInt(L"TabFlashChanged");
		public:  CEOptionUInt   nTabBarDblClickAction = CEOptionUInt(L"TabDblClick"); // 0-None, 1-Auto, 2-Maximize/Restore, 3-NewTab (SettingsNS::tabBarDefaultClickActions)
		public:  CEOptionUInt   nTabBtnDblClickAction = CEOptionUInt(L"TabBtnDblClick"); // 0-None, 1-Maximize/Restore, 2-Close, 3-Restart, 4-Duplicate (SettingsNS::tabBtnDefaultClickActions)
		private: CEOptionEnum<TabsOnTaskbar> m_isTabsOnTaskBar = CEOptionEnum<TabsOnTaskbar>(L"TabsOnTaskBar"); // 0 - ConEmu only, 1 - all tabs & all OS, 2 - all tabs & Win 7, 3 - DON'T SHOW
		public:  CEOptionBool   isTaskbarOverlay = CEOptionBool(L"TaskBarOverlay");
		public:  CEOptionBool   isTaskbarProgress = CEOptionBool(L"TaskbarProgress");
		public:  CEOptionFName  sTabFontFace = CEOptionFName(L"TabFontFace");
		public:  CEOptionUInt   nTabFontCharSet = CEOptionUInt(L"TabFontCharSet");
		public:  CEOptionInt    nTabFontHeight = CEOptionInt(L"TabFontHeight");
		public:  CEOptionString sTabCloseMacro = CEOptionString(L"TabCloseMacro"); //if (!reg->Load(L"TabCloseMacro", &sTabCloseMacro) || (sTabCloseMacro && !*sTabCloseMacro)) { if (sTabCloseMacro) { free(sTabCloseMacro); sTabCloseMacro = NULL; } }
		public:  CEOptionString sSaveAllMacro = CEOptionString(L"SaveAllEditors"); //if (!reg->Load(L"SaveAllEditors", &sSaveAllMacro)) { sSaveAllMacro = lstrdup(L"...
		public:  CEOptionInt    nToolbarAddSpace = CEOptionInt(L"ToolbarAddSpace"); // UInt?
		public:  CEOptionSize   wndWidth = CEOptionSize(L"ConWnd Width");
		public:  CEOptionSize   wndHeight = CEOptionSize(L"ConWnd Height");
		public:  CEOptionUInt   ntvdmHeight = CEOptionUInt(L"16bit Height"); // в символах
		public:  CEOptionInt    _wndX = CEOptionInt(L"ConWnd X"); // в пикселях
		public:  CEOptionInt    _wndY = CEOptionInt(L"ConWnd Y"); // в пикселях
		public:  CEOptionDWORD  _WindowMode = CEOptionDWORD(L"WindowMode"); // if (WindowMode!=wmFullScreen && WindowMode!=wmMaximized && WindowMode!=wmNormal) WindowMode = wmNormal;
		public:  CEOptionBool   wndCascade = CEOptionBool(L"Cascaded");
		public:  CEOptionBool   isAutoSaveSizePos = CEOptionBool(L"AutoSaveSizePos");
		public:  CEOptionBool   isUseCurrentSizePos = CEOptionBool(L"UseCurrentSizePos"); // Show in settings dialog and save current window size/pos
		public:  CEOptionUInt   nSlideShowElapse = CEOptionUInt(L"SlideShowElapse");
		public:  CEOptionUInt   nIconID = CEOptionUInt(L"IconID");
		public:  CEOptionBool   isTryToCenter = CEOptionBool(L"TryToCenter");
		public:  CEOptionUInt   nCenterConsolePad = CEOptionUInt(L"CenterConsolePad");
		public:  CEOptionByte   isAlwaysShowScrollbar = CEOptionByte(L"ShowScrollbar"); // if (isAlwaysShowScrollbar > 2) isAlwaysShowScrollbar = 2; // 0-не показывать, 1-всегда, 2-автоматически (на откусывает место от консоли)
		public:  CEOptionUInt   nScrollBarAppearDelay = CEOptionUInt(L"ScrollBarAppearDelay");
		public:  CEOptionUInt   nScrollBarDisappearDelay = CEOptionUInt(L"ScrollBarDisappearDelay");
		public:  CEOptionBool   isSingleInstance = CEOptionBool(L"SingleInstance");
		public:  CEOptionBool   isShowHelpTooltips = CEOptionBool(L"ShowHelpTooltips");
		public:  CEOptionBool   mb_isMulti = CEOptionBool(L"Multi");
		public:  CEOptionBool   isMultiShowButtons = CEOptionBool(L"Multi.ShowButtons");
		public:  CEOptionBool   isMultiShowSearch = CEOptionBool(L"Multi.ShowSearch");
		public:  CEOptionBool   isNumberInCaption = CEOptionBool(L"NumberInCaption");
		private: CEOptionDWORD  nHostkeyNumberModifier = CEOptionDWORD(L"Multi.Modifier"); // TestHostkeyModifiers(); // Используется для 0..9, WinSize
		private: CEOptionUInt   nHostkeyArrowModifier = CEOptionUInt(L"Multi.ArrowsModifier"); // TestHostkeyModifiers(); // Используется для WinSize
		public:  CEOptionByte   nCloseConfirmFlags = CEOptionByte(L"Multi.CloseConfirm"); // CloseConfirmOptions
		public:  CEOptionBool   isMultiAutoCreate = CEOptionBool(L"Multi.AutoCreate");
		public:  CEOptionByte   isMultiLeaveOnClose = CEOptionByte(L"Multi.LeaveOnClose"); // 0 - закрываться, 1 - оставаться, 2 - НЕ оставаться при закрытии "крестиком"
		public:  CEOptionByte   isMultiHideOnClose = CEOptionByte(L"Multi.HideOnClose"); // 0 - не скрываться, 1 - в трей, 2 - просто минимизация
		public:  CEOptionByte   isMultiMinByEsc = CEOptionByte(L"Multi.MinByEsc"); // 0 - Never, 1 - Always, 2 - NoConsoles
		public:  CEOptionBool   isMapShiftEscToEsc = CEOptionBool(L"MapShiftEscToEsc"); // used only when isMultiMinByEsc==1 and only for console apps
		public:  CEOptionBool   isMultiIterate = CEOptionBool(L"Multi.Iterate");
		public:  CEOptionBool   isMultiNewConfirm = CEOptionBool(L"Multi.NewConfirm");
		public:  CEOptionBool   isMultiDupConfirm = CEOptionBool(L"Multi.DupConfirm");
		public:  CEOptionBool   isMultiDetachConfirm = CEOptionBool(L"Multi.DetachConfirm");
		public:  CEOptionBool   isUseWinNumber = CEOptionBool(L"Multi.UseNumbers");
		public:  CEOptionBool   isUseWinTab = CEOptionBool(L"Multi.UseWinTab");
		public:  CEOptionBool   isUseWinArrows = CEOptionBool(L"Multi.UseArrows");
		public:  CEOptionByte   nSplitWidth = CEOptionByte(L"Multi.SplitWidth");
		public:  CEOptionByte   nSplitHeight = CEOptionByte(L"Multi.SplitHeight");
		public:  CEOptionBool   isFARuseASCIIsort = CEOptionBool(L"FARuseASCIIsort");
		public:  CEOptionBool   isFixAltOnAltTab = CEOptionBool(L"FixAltOnAltTab");
		public:  CEOptionBool   isUseAltGrayPlus = CEOptionBool(L"UseAltGrayPlus");
		public:  CEOptionBool   isShellNoZoneCheck = CEOptionBool(L"ShellNoZoneCheck");
		public:  CEOptionStr32  szTabConsole = CEOptionStr32(L"TabConsole");
		public:  CEOptionStr16  szTabModifiedSuffix = CEOptionStr16(L"TabModifiedSuffix");
		public:  CEOptionString pszTabSkipWords = CEOptionString(L"TabSkipWords");
		public:  CEOptionStr32  szTabPanels = CEOptionStr32(L"TabPanels");
		public:  CEOptionStr32  szTabEditor = CEOptionStr32(L"TabEditor");
		public:  CEOptionStr32  szTabEditorModified = CEOptionStr32(L"TabEditorModified");
		public:  CEOptionStr32  szTabViewer = CEOptionStr32(L"TabViewer");
		public:  CEOptionUInt   nTabLenMax = CEOptionUInt(L"TabLenMax", 20); // if (nTabLenMax < 10 || nTabLenMax >= CONEMUTABMAX) nTabLenMax = 20;
		public:  CEOptionStr64  szAdminTitleSuffix = CEOptionStr64(L"AdminTitleSuffix"); // DefaultAdminTitleSuffix /* " (Admin)" */
		public:  CEOptionByte   bAdminShield = CEOptionByte(L"AdminShowShield"); // enum AdminTabStyle
		public:  CEOptionBool   bHideInactiveConsoleTabs = CEOptionBool(L"HideInactiveConsoleTabs");
		public:  CEOptionBool   bShowFarWindows = CEOptionBool(L"ShowFarWindows");
		public:  CEOptionUInt   nMainTimerElapse = CEOptionUInt(L"MainTimerElapse", 10); // if (nMainTimerElapse>1000) nMainTimerElapse = 1000; // периодичность, с которой из консоли считывается текст
		public:  CEOptionUInt   nMainTimerInactiveElapse = CEOptionUInt(L"MainTimerInactiveElapse", 1000); // if (nMainTimerInactiveElapse>10000) nMainTimerInactiveElapse = 10000; // периодичность при неактивности
		public:  CEOptionBool   isSkipFocusEvents = CEOptionBool(L"SkipFocusEvents");
		public:  CEOptionByte   isMonitorConsoleLang = CEOptionByte(L"MonitorConsoleLang"); // bitmask. 1 - follow up console HKL (e.g. after XLat in Far Manager), 2 - use one HKL for all tabs
		public:  CEOptionBool   isSleepInBackground = CEOptionBool(L"SleepInBackground");
		public:  CEOptionBool   isRetardInactivePanes = CEOptionBool(L"RetardInactivePanes");
		public:  CEOptionBool   mb_MinimizeOnLoseFocus = CEOptionBool(L"MinimizeOnLoseFocus");
		public:  CEOptionDWORD  nAffinity = CEOptionDWORD(L"AffinityMask");
		public:  CEOptionBool   isUseInjects = CEOptionBool(L"UseInjects"); // NB. Root process is infiltrated always.
		public:  CEOptionBool   isProcessAnsi = CEOptionBool(L"ProcessAnsi"); // ANSI X3.64 & XTerm-256-colors Support
		public:  CEOptionBool   isAnsiLog = CEOptionBool(L"AnsiLog"); // Limited logging of console contents (same output as processed by CECF_ProcessAnsi)
		public:  CEOptionString pszAnsiLog = CEOptionString(L"AnsiLogPath");
		public:  CEOptionBool   isProcessNewConArg = CEOptionBool(L"ProcessNewConArg"); // Enable processing of '-new_console' and '-cur_console' switches in your shell prompt, scripts etc. started in ConEmu tabs
		public:  CEOptionBool   isProcessCmdStart = CEOptionBool(L"ProcessCmdStart"); // Use "start xxx.exe" to start new tab
		public:  CEOptionBool   isProcessCtrlZ = CEOptionBool(L"ProcessCtrlZ"); // Treat Ctrl-Z as ‘EndOfStream’. On new line press Ctrl-Z and Enter. Refer to the gh#465 for details (Go input streams).
		public:  CEOptionBool   mb_UseClink = CEOptionBool(L"UseClink"); // использовать расширение командной строки (ReadConsole)
		public:  CEOptionBool   isSuppressBells = CEOptionBool(L"SuppressBells");
		public:  CEOptionBool   isConsoleExceptionHandler = CEOptionBool(L"ConsoleExceptionHandler");
		public:  CEOptionBool   isConVisible = CEOptionBool(L"ConVisible"); /* *** Debugging *** */

	public:

		//reg->LoadMSZ(L"EnvironmentSet", psEnvironmentSet);
		wchar_t* psEnvironmentSet; // commands: multiline, "\r\n" separated

		// Service functions
		wchar_t* LineDelimited2MSZ(const wchar_t* apszApps, bool bLowerCase = true); // "|"-delimited string -> MSZ
		wchar_t* MSZ2LineDelimited(const wchar_t* apszLines, LPCWSTR asDelim = L"|", bool bFinalToo = false); // MSZ -> "<asDelim>"-delimited string
		wchar_t* MultiLine2MSZ(const wchar_t* apszLines, DWORD* pcbSize/*in bytes*/); // "\r\n"-delimited string -> MSZ

		bool LoadMSZ(SettingsBase* reg, LPCWSTR asName, wchar_t*& rsLines, LPCWSTR asDelim /*= L"|"*/, bool bFinalToo /*= false*/);
		void SaveMSZ(SettingsBase* reg, LPCWSTR asName, LPCWSTR rsLines, LPCWSTR asDelim /*= L"|"*/, bool bLowerCase /*= true*/);

		// Replace default terminal
		bool isSetDefaultTerminal;
		bool isRegisterOnOsStartup;
		bool isRegisterOnOsStartupTSA;
		bool isRegisterAgressive;
		bool isDefaultTerminalNoInjects;
		bool isDefaultTerminalNewWindow;
		bool isDefaultTerminalDebugLog;
		BYTE nDefaultTerminalConfirmClose; // "Press Enter to close console". 0 - Auto, 1 - Always, 2 - Never
		wchar_t* GetDefaultTerminalApps(); // "|" delimited
		const wchar_t* GetDefaultTerminalAppsMSZ(); // "\0" delimited
		void SetDefaultTerminalApps(const wchar_t* apszApps); // "|" delimited


	public:
		int GetAppSettingsId(LPCWSTR asExeAppName, bool abElevated);
		const AppSettings* GetAppSettings(int anAppId=-1);
		const COLORREF* GetDefColors(LPCWSTR asDefName = NULL);
		COLORREF* GetColors(int anAppId=-1, BOOL abFade = FALSE);
		COLORREF* GetColorsPrepare(COLORREF *pColors, COLORREF *pColorsFade, bool* pbFadeInitialized, BOOL abFade);
		void PrepareFadeColors(COLORREF *pColors, COLORREF *pColorsFade, bool* pbFadeInitialized);
		COLORREF* GetPaletteColors(LPCWSTR asPalette, BOOL abFade = FALSE);
		COLORREF GetFadeColor(COLORREF cr);
		void ResetFadeColors();

		const CommandTasks* CmdTaskGet(int anIndex); // 0-based, index of CmdTasks. "-1" == autosaved task
		const CommandTasks* CmdTaskGetByName(LPCWSTR asTaskName);
		void CmdTaskSetVkMod(int anIndex, DWORD VkMod); // 0-based, index of CmdTasks
		int  CmdTaskSet(int anIndex, LPCWSTR asName, LPCWSTR asGuiArgs, LPCWSTR asCommands, CETASKFLAGS aFlags = CETF_DONT_CHANGE); // 0-based, index of CmdTasks
		bool CmdTaskXch(int anIndex1, int anIndex2); // 0-based, index of CmdTasks

		const ColorPalette* PaletteGet(int anIndex); // 0-based, index of Palettes, or -1 for "<Current color scheme>"
		const ColorPalette* PaletteGetByName(LPCWSTR asName);
		const ColorPalette* PaletteFindCurrent(bool bMatchAttributes);
		const ColorPalette* PaletteFindByColors(bool bMatchAttributes, const ColorPalette* pCur);
		int PaletteGetIndex(LPCWSTR asName);
		void PaletteSaveAs(LPCWSTR asName); // Save active colors to named palette
		void PaletteSaveAs(LPCWSTR asName, bool abExtendColors, BYTE anExtendColorIdx, BYTE anTextColorIdx, BYTE anBackColorIdx, BYTE anPopTextColorIdx, BYTE anPopBackColorIdx, const COLORREF (&aColors)[0x20], bool abSaveSettings);
		void PaletteDelete(LPCWSTR asName); // Delete named palette
		void PaletteSetStdIndexes();
		int PaletteSetActive(LPCWSTR asName);


	public:
		AppSettings AppStd;
	protected:
		friend struct AppSettings;
		int AppCount;
		AppSettings** Apps;
		// Для CSettings
		AppSettings* GetAppSettingsPtr(int anAppId, BOOL abCreateNew = FALSE);
		void AppSettingsDelete(int anAppId);
		bool AppSettingsXch(int anIndex1, int anIndex2); // 0-based, index of Apps

		int CmdTaskCount;
		CommandTasks** CmdTasks;
		CommandTasks* StartupTask;
		void FreeCmdTasks();

		int PaletteCount, PaletteAllocated;
		ColorPalette** Palettes;
		ColorPalette* PaletteGetPtr(int anIndex); // 0-based, index of Palettes
		void SavePalettes(SettingsBase* reg);
		void SortPalettes();
		void FreePalettes();

		void SaveStatusSettings(SettingsBase* reg);

		void LoadSizeSettings(SettingsBase* reg);
		void SaveSizeSettings(SettingsBase* reg);
		void PatchSizeSettings();

	private:
		COLORREF ColorsFade[0x20];
		bool mb_FadeInitialized;

		//struct CEAppColors
		//{
		//	COLORREF Colors[0x20];
		//	COLORREF ColorsFade[0x20];
		//	bool FadeInitialized;
		//} **AppColors; // [AppCount]

		void LoadCursorSettings(SettingsBase* reg, CECursorType* pActive, CECursorType* pInactive);

		void LoadAppsSettings(bool bAppendMode, SettingsBase* reg, bool abFromOpDlg = false);
		void LoadAppSettings(SettingsBase* reg, AppSettings* pApp/*, COLORREF* pColors*/);
		void SaveAppSettings(SettingsBase* reg, AppSettings* pApp/*, COLORREF* pColors*/);

		void SaveStdColors(SettingsBase* reg);
		void SaveStartCommands(SettingsBase* reg);

		void FreeApps(int NewAppCount = 0, AppSettings** NewApps = NULL/*, Settings::CEAppColors** NewAppColors = NULL*/);

		DWORD mn_FadeMul;
		inline BYTE GetFadeColorItem(BYTE c);

	public:

		//reg->Load(L"ConsoleFontName", ConsoleFont.lfFaceName, countof(ConsoleFont.lfFaceName));
		//reg->Load(L"ConsoleFontWidth", ConsoleFont.lfWidth);
		//reg->Load(L"ConsoleFontHeight", ConsoleFont.lfHeight);
		LOGFONT ConsoleFont;

		bool NeedDialogDetect();



		/* *** Transparency *** */
		bool isTransparentAllowed();

		/* *** Command Line History (from start dialog) *** */
		//reg->Load(L"CmdLineHistory", &psCmdHistory);
		CommandHistory* pHistory;
		// Helpers
		void HistoryAdd(LPCWSTR asCmd);
		void HistoryReset();
		void HistoryLoad(SettingsBase* reg);
		void HistorySave(SettingsBase* reg);
		LPCWSTR HistoryGet(int index);

	public:
		BOOL mb_CharSetWasSet;

		// Previously, the option was used to define different font
		// generally for Far Manager frames (pseudographics)
		// Now user may use it for any range of characters (CJK, etc.)
		// Used with settings: inFont2, FontSizeX2, mpc_CharAltFontRanges
		// "FixFarBorders"
		BYTE isFixFarBorders;
		// "FontName2"
		wchar_t inFont2[LF_FACESIZE];
		// "FontSizeX2" : width of *alternative* font (to avoid dashed frames this may be larger than main font width)
		UINT FontSizeX2;
		// "FixFarBordersRanges" ==> ParseCharRanges(...)
		// Default: "2013-25C4"; Example: "0410-044F;2013-25C4;"
		BYTE mpc_CharAltFontRanges[0x10000];
		int ParseCharRanges(LPCWSTR asRanges, BYTE (&Chars)[0x10000], BYTE abValue = TRUE);
		wchar_t* CreateCharRanges(BYTE (&Chars)[0x10000]); // caller must free(result)
		bool CheckCharAltFont(ucs32 inChar);


		UINT nQuakeAnimation;
		public:
		void SetHideCaptionAlways(bool bHideCaptionAlways);
		void SwitchHideCaptionAlways();
		bool isHideCaptionAlways(); //<<mb_HideCaptionAlways
		bool isMinimizeOnLoseFocus();
		bool isForcedHideCaptionAlways(); // true, если mb_HideCaptionAlways отключать нельзя
		bool isCaptionHidden(ConEmuWindowMode wmNewMode = wmCurrent);
		bool isFrameHidden();
		int HideCaptionAlwaysFrame();
		/* *** Text selection *** */
		public:
		// Service functions
		wchar_t* GetIntelligentExceptions(); // "|" delimited
		const wchar_t* GetIntelligentExceptionsMSZ(); // "\0" delimited
		void SetIntelligentExceptions(const wchar_t* apszApps); // "|" delimited


		bool IsModifierPressed(int nDescrID, bool bAllowEmpty);
		void IsModifierPressed(int nDescrID, bool* pbNoEmpty, bool* pbAllowEmpty);

	public:
		bool isKeyboardHooks(bool abNoDisable = false, bool abNoDbgCheck = false);



		//Для тачскринов - удобнее по длинному тапу показывать меню,
		// а по двойному (Press and tap) выполнять выделение файлов
		// Поэтому, если isRClickTouch, то "длинный"/"короткий" клик инвертируется
		// --> isRClickSendKey==1 - звать всегда (isRClickTouchInvert не влияет)
		// --> isRClickSendKey==2 - звать по длинному тапу (аналог простого RClick)
		// При этом, PressAndTap всегда посылает RClick в консоль (для выделения файлов).
		bool isRClickTouchInvert();
		LPCWSTR RClickMacro(FarMacroVersion fmv);
		LPCWSTR RClickMacroDefault(FarMacroVersion fmv);

		LPCWSTR SafeFarCloseMacro(FarMacroVersion fmv);
		LPCWSTR SafeFarCloseMacroDefault(FarMacroVersion fmv);


		bool isMinToTray(bool bRawOnly = false);
		void SetMinToTray(bool bMinToTray);
		bool isAlwaysShowTrayIcon();

		/* *** Drag *** */




		// Helpers
		bool mb_DisableLogging;
		uint isLogging(uint level = 1);
		void EnableLogging();
		void DisableLogging();
		LPCWSTR GetLogFileName();


		//mn_LastFadeSrc = mn_LastFadeDst = -1;
		COLORREF mn_LastFadeSrc;
		//mn_LastFadeSrc = mn_LastFadeDst = -1;
		COLORREF mn_LastFadeDst;
		public:

		int StatusBarFontHeight(); // { return max(4,nStatusFontHeight); };
		int StatusBarHeight(); // { return StatusBarFontHeight() + ((isStatusBarFlags & csf_NoVerticalPad) ? ((isStatusBarFlags & csf_HorzDelim) ? 1 : 0) : 2); };
		//для информации, чтобы сохранить изменения при выходе
		bool mb_StatusSettingsWasChanged;

		bool isActivateSplitMouseOver();


		//TODO:
		bool isTabsInCaption;

		// Tab theme properties
		//int ilDragHeight; = 10

		public:
		TabsOnTaskbar GetRawTabsOnTaskBar() { return (TabsOnTaskbar)m_isTabsOnTaskBar; };
		bool isTabsOnTaskBar();
		bool isWindowOnTaskBar(bool bStrictOnly = false);
		//void SetTabsOnTaskBar(BYTE nTabsOnTaskBar);


		LPCWSTR TabCloseMacro(FarMacroVersion fmv);
		LPCWSTR TabCloseMacroDefault(FarMacroVersion fmv);

		LPCWSTR SaveAllMacro(FarMacroVersion fmv);
		LPCWSTR SaveAllMacroDefault(FarMacroVersion fmv);

		// Monitor information
		RECT LastMonRect;

		bool isIntegralSize();

	private:
		// При закрытии окна крестиком - сохранять только один раз,
		// а то размер может в процессе закрытия консолей измениться
		bool mb_ExitSettingsAutoSaved;
	public:

		public:
		// Max - 3 keys, so lower 3 bytes only
		DWORD HostkeyNumberModifier() { return (nHostkeyNumberModifier & 0xFFFFFF); };
		DWORD HostkeyArrowModifier()  { return (nHostkeyArrowModifier  & 0xFFFFFF); };
		//
		public:
		// L"Multi.CloseConfirm", isCloseConsoleConfirm);
		enum CloseConfirmOptions {
			cc_None      = 0, // Don't confirm any close actions
			cc_Window    = 1, // Window close (cross clicking)
			cc_Console   = 2, // Tab close (from tab menu for example)
			cc_Running   = 4, // When running process was detected
			cc_FarEV     = 8, // was isCloseEditViewConfirm
		};
		// helpers
		bool isCloseOnLastTabClose();
		bool isCloseOnCrossClick();
		bool isMinOnLastTabClose();
		bool isHideOnLastTabClose();
		// L"Multi.MinByEsc" isMultiMinByEsc
		enum MultiMinByEsc { mbe_Never = 0, mbe_Always = 1, mbe_NoConsoles = 2 };

		// FindText: bMatchCase, bMatchWholeWords, bFreezeConsole, bHighlightAll;
		// FindOptions.pszText may be used to pre-fill search dialog field if search-bar is hidden
		FindTextOptions FindOptions;


	public:
		/* ************************ */
		/* ************************ */
		/* *** Hotkeys/Hostkeys *** */
		/* ************************ */
		/* ************************ */

		// VkMod = LOBYTE - VK, старшие три байта - модификаторы (тоже VK)

		// Вернуть заданный VkMod, или 0 если не задан. nDescrID = vkXXX (e.g. vkMinimizeRestore)
		DWORD GetHotkeyById(int nDescrID, const ConEmuHotKey** ppHK = NULL);
		// Return hotkeyname by ID
		LPCWSTR GetHotkeyNameById(int nDescrID, wchar_t (&szFull)[128], bool bShowNone = true);
		// Проверить, задан ли этот hotkey. nDescrID = vkXXX (e.g. vkMinimizeRestore)
		bool IsHotkey(int nDescrID);
		// Установить новый hotkey. nDescrID = vkXXX (e.g. vkMinimizeRestore).
		void SetHotkeyById(int nDescrID, DWORD VkMod);
		// Проверить, есть ли хоткеи с назначенным одиночным модификатором
		bool isModifierExist(BYTE Mod/*VK*/, bool abStrictSingle = false);
		// Есть ли такой хоткей или модификатор (актуально для VK_APPS)
		bool isKeyOrModifierExist(BYTE Mod/*VK*/);
		// Проверить на дубли
		void CheckHotkeyUnique();
	private:
		void LoadHotkeys(bool bAppendMode, SettingsBase* reg, const bool& bSendAltEnter, const bool& bSendAltSpace, const bool& bSendAltF9);
		void SaveHotkeys(SettingsBase* reg, int SaveDescrLangID = 0);
	public:

		/* *** Tab Templates *** */

		//TabStyle nTabStyle; // enum

		// Old style:
		// * Disabled: bAdminShield = false, szAdminTitleSuffix = ""
		// * Shield:   bAdminShield = true,  szAdminTitleSuffix ignored (may be filled!)
		// * Suffix:   bAdminShield = false, szAdminTitleSuffix = " (Admin)"
		// New style:
		// * Disabled: bAdminShield = 0,  szAdminTitleSuffix = ""
		// * Shield:   bAdminShield = 1,  szAdminTitleSuffix ignored (may be filled!)
		// * Suffix:   bAdminShield = 0,  szAdminTitleSuffix = " (Admin)"
		// * Shld+Suf: bAdminShield = 3,  szAdminTitleSuffix = " (Admin)"
		bool isAdminShield();
		bool isAdminSuffix();

		// L"HideDisabledTabs" -- is not saved
		bool isHideDisabledTabs() { return false; };


		bool NeedCreateAppWindow();


		//bool isAdvLangChange; // в Висте без ConIme в самой консоли не меняется язык, пока не послать WM_SETFOCUS. Но при этом исчезает диалог быстрого поиска


		//bool isLangChangeWsPlugin;





		DWORD isUseClink(bool abCheckVersion = false);


		/* *** Thumbnails and Tiles *** */
		//reg->Load(L"PanView.BackColor", ThSet.crBackground.RawColor);
		//reg->Load(L"PanView.PFrame", ThSet.nPreviewFrame); if (ThSet.nPreviewFrame!=0 && ThSet.nPreviewFrame!=1) ThSet.nPreviewFrame = 1;
		//и т.п...
		PanelViewSetMapping ThSet;

		/* *** AutoUpdate *** */
		ConEmuUpdateSettings UpdSet;

		/* *** Notification *** */
		wchar_t StopBuzzingDate[16];


		/* *** HotKeys & GuiMacros *** */
		//reg->Load(L"GuiMacro<N>.Key", &Macros.vk);
		//reg->Load(L"GuiMacro<N>.Macro", &Macros.szGuiMacro);
		//struct HotGuiMacro
		//{
		//	union {
		//		BYTE vk;
		//		LPVOID dummy;
		//	};
		//	wchar_t* szGuiMacro;
		//};
		//HotGuiMacro Macros[24];

	public:
		void LoadSettings(bool& rbNeedCreateVanilla, const SettingsStorage* apStorage = NULL);
		void LoadSettings(bool bAppendMode, SettingsBase* reg);
		void InitSettings();
		void InitVanilla();
		void InitVanillaFontSettings();
		bool SaveVanilla(SettingsBase* reg);
		void LoadCmdTasks(bool bAppendMode, SettingsBase* reg, bool abFromOpDlg = false);
		void LoadPalettes(bool bAppendMode, SettingsBase* reg);
		void CreatePredefinedPalettes(int iAddUserCount);
		void LoadProgresses(SettingsBase* reg);
		BOOL SaveSettings(BOOL abSilent = FALSE, const SettingsStorage* apStorage = NULL);
		void SaveAppsSettings(SettingsBase* reg);
		bool SaveCmdTasks(SettingsBase* reg);
		bool SaveProgresses(SettingsBase* reg);
		void SaveConsoleFont();
		void SaveFindOptions(SettingsBase* reg = NULL);
		void SaveSettingsOnExit();
		void SaveStopBuzzingDate();
		//void UpdateMargins(RECT arcMargins);
	public:
		void CheckConsoleSettings();
		void ResetSavedOnExit();

		SettingsBase* CreateSettings(const SettingsStorage* apStorage);
		wchar_t* GetStoragePlaceDescr(const SettingsStorage* apStorage, LPCWSTR asPrefix);

		void GetSettingsType(SettingsStorage& Storage, bool& ReadOnly);

	protected:
		// defaults
		static u32 antialias_default();
		static bool antialias_validate(u32& val);
};
