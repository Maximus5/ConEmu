
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

	// With specified default value
	#define CEOPTIONV(access,opttype,optvar,optname,defval) \
		access:  opttype<defval> optvar = opttype<defval>(optname)
	// With template default value
	#define CEOPTIOND(access,opttype,optvar,optname) \
		access:  opttype<> optvar = opttype<>(optname)
	public:
		CEOPTIONV(public, CEOptionInt, DefaultBufferHeight, L"DefaultBufferHeight", 1000);
		CEOPTIONV(public, CEOptionBool, AutoBufferHeight, L"AutoBufferHeight", true); // Long console output
		CEOPTIONV(public, CEOptionBool, UseScrollLock, L"UseScrollLock", true);
		CEOPTIONV(public, CEOptionInt, nCmdOutputCP, L"CmdOutputCP", 0);
		CEOptionComSpec ComSpec; // Defaults are set in CEOptionComSpec::Reset
		CEOPTIONV(public, CEOptionStringDelim, DefaultTerminalApps, L"DefaultTerminalApps", L"explorer.exe"); // Stored as "|"-delimited string
		// #OPT_TODO ColorTableIndexName - not implemented
		//private: CEOptionArray<COLORREF, 0x20, ColorTableIndexName/*"ColorTableNN" decimal*/, ColorTableDefaults/*<ConEmu>*/> Colors; // L"ColorTableNN", Colors[i]
		CEOPTIONV(public, CEOptionBool, isFontAutoSize, L"FontAutoSize", false);
		CEOPTIONV(public, CEOptionBool, isAutoRegisterFonts, L"AutoRegisterFonts", true);
		CEOPTIONV(public, CEOptionBool, isTrueColorer, L"TrueColorerSupport", true);
		CEOPTIONV(public, CEOptionBool, isVividColors, L"VividColors", true);
		CEOPTIONV(public, CEOptionByte, isShowBgImage, L"BackGround Image show", 0);
		CEOPTIONV(public, CEOptionStringFixed, sBgImage, L"BackGround Image", MAX_PATH, L"%USERPROFILE%\\back.jpg");
		CEOPTIONV(public, CEOptionByte, bgImageDarker, L"bgImageDarker", 255);
		CEOPTIONV(public, CEOptionDWORD, nBgImageColors, L"bgImageColors", (DWORD)-1);
		CEOPTIONV(public, CEOptionByteEnum, bgOperation, L"bgOperation", BackgroundOp, eUpLeft); // BackgroundOp {eUpLeft = 0, eStretch = 1, eTile = 2, ...}
		CEOPTIONV(public, CEOptionByte, isBgPluginAllowed, L"bgPluginAllowed", 1);
		CEOPTIONV(public, CEOptionByte, nTransparent, L"AlphaValue", 255);
		CEOPTIONV(public, CEOptionBool, isTransparentSeparate, L"AlphaValueSeparate", false);
		CEOPTIONV(public, CEOptionByte, nTransparentInactive, L"AlphaValueInactive", 255);
		CEOPTIONV(public, CEOptionBool, isUserScreenTransparent, L"UserScreenTransparent", false);
		CEOPTIONV(public, CEOptionBool, isColorKeyTransparent, L"ColorKeyTransparent", false);
		CEOPTIONV(public, CEOptionDWORD, nColorKeyValue, L"ColorKeyValue", RGB(1,1,1));
		CEOPTIONV(public, CEOptionBool, isSaveCmdHistory, L"SaveCmdHistory", true);
		// #OPT_TODO CEOPTIONV(public, CEOptionString, psHistoryLocation, L"CmdHistoryLocation");
		CEOPTIONV(public, CEOptionByteEnum, nStartType, L"StartType", StartupType, start_Command); // !!! POST VALIDATION IS REQUIRED !!!
		CEOPTIOND(public, CEOptionString, psStartSingleApp, L"CmdLine");
		CEOPTIOND(public, CEOptionString, psStartTasksFile, L"StartTasksFile");
		CEOPTIOND(public, CEOptionString, psStartTasksName, L"StartTasksName");
		CEOPTIOND(public, CEOptionBool, isStartFarFolders, L"StartFarFolders");
		CEOPTIOND(public, CEOptionBool, isStartFarEditors, L"StartFarEditors");
		CEOPTIONV(public, CEOptionUInt, nStartCreateDelay, L"StartCreateDelay", RUNQUEUE_CREATE_LAG_DEF); // RUNQUEUE_CREATE_LAG
		CEOPTIOND(public, CEOptionBool, isStoreTaskbarkTasks, L"StoreTaskbarkTasks");
		CEOPTIOND(public, CEOptionBool, isStoreTaskbarCommands, L"StoreTaskbarCommands");
		CEOPTIONV(public, CEOptionStringFixed, inFont, L"FontName", LF_FACESIZE);
		CEOPTIONV(public, CEOptionBool, isBold, L"FontBold", false);
		CEOPTIONV(public, CEOptionBool, isItalic, L"FontItalic", false);
		CEOPTIONV(public, CEOptionUInt, mn_AntiAlias, L"Anti-aliasing", CLEARTYPE_NATURAL_QUALITY, get_antialias_default, antialias_validate);
		CEOPTIONV(public, CEOptionByte, mn_LoadFontCharSet, L"FontCharSet", DEFAULT_CHARSET); // Loaded or Saved to settings // !!! mb_CharSetWasSet = FALSE;
		CEOPTIONV(public, CEOptionUInt, FontSizeY, L"FontSize", DEF_FONTSIZEY_P);  // высота основного шрифта (загруженная из настроек!)
		CEOPTIOND(public, CEOptionUInt, FontSizeX, L"FontSizeX");  // ширина основного шрифта
		CEOPTIONV(public, CEOptionUInt, FontSizeX3, L"FontSizeX3", 0); // ширина знакоместа при моноширинном режиме (не путать с FontSizeX2)
		CEOPTIONV(public, CEOptionBool, FontUseDpi, L"FontUseDpi", true);
		CEOPTIONV(public, CEOptionBool, FontUseUnits, L"FontUseUnits", false);
		CEOPTIONV(public, CEOptionBool, isAntiAlias2, L"Anti-aliasing2", false); // disabled by default to avoid dashed framed
		CEOPTIONV(public, CEOptionBool, isHideCaption, L"HideCaption", false); // Hide caption when maximized
		CEOPTIONV(public, CEOptionBool, isHideChildCaption, L"HideChildCaption", true); // Hide caption of child GUI applications, started in ConEmu tabs (PuTTY, Notepad, etc.)
		CEOPTIONV(public, CEOptionBool, isFocusInChildWindows, L"FocusInChildWindows", true);
		CEOPTIONV(public, CEOptionBool, mb_IntegralSize, L"IntegralSize", false);
		CEOPTIONV(public, CEOptionByteEnum, isQuakeStyle, L"QuakeStyle", ConEmuQuakeMode, quake_Disabled);
		CEOPTIONV(public, CEOptionBool, isRestore2ActiveMon, L"Restore2ActiveMon", false);
		protected: CEOPTIONV(public, CEOptionBool, mb_HideCaptionAlways, L"HideCaptionAlways", false);
		public: CEOPTIONV(public, CEOptionByte, nHideCaptionAlwaysFrame, L"HideCaptionAlwaysFrame", HIDECAPTIONALWAYSFRAME_DEF);
		CEOPTIONV(public, CEOptionUInt, nHideCaptionAlwaysDelay, L"HideCaptionAlwaysDelay", 2000);
		CEOPTIONV(public, CEOptionUInt, nHideCaptionAlwaysDisappear, L"HideCaptionAlwaysDisappear", 2000);
		CEOPTIONV(public, CEOptionBool, isDownShowHiddenMessage, L"DownShowHiddenMessage", false);
		CEOPTIONV(public, CEOptionBool, isDownShowExOnTopMessage, L"DownShowExOnTopMessage", false);
		CEOPTIONV(public, CEOptionBool, isAlwaysOnTop, L"AlwaysOnTop", false);
		CEOPTIONV(public, CEOptionBool, isSnapToDesktopEdges, L"SnapToDesktopEdges", false);
		CEOPTIONV(public, CEOptionBool, isExtendUCharMap, L"ExtendUCharMap", true); // !!! FAR
		CEOPTIONV(public, CEOptionBool, isDisableMouse, L"DisableMouse", false);
		CEOPTIONV(public, CEOptionBool, isMouseSkipActivation, L"MouseSkipActivation", true);
		CEOPTIONV(public, CEOptionBool, isMouseSkipMoving, L"MouseSkipMoving", true);
		CEOPTIONV(public, CEOptionBool, isMouseDragWindow, L"MouseDragWindow", true);
		CEOPTIONV(public, CEOptionBool, isFarHourglass, L"FarHourglass", true);
		CEOPTIONV(public, CEOptionUInt, nFarHourglassDelay, L"FarHourglassDelay", 500);
		CEOPTIONV(public, CEOptionByte, isDisableFarFlashing, L"DisableFarFlashing", false); // if (isDisableFarFlashing>2) isDisableFarFlashing = 2;
		CEOPTIONV(public, CEOptionUInt, isDisableAllFlashing, L"DisableAllFlashing", false); // if (isDisableAllFlashing>2) isDisableAllFlashing = 2;
		CEOPTIONV(public, CEOptionBool, isCTSIntelligent, L"CTSIntelligent", true);
		private: CEOPTIONV(public, CEOptionStringDelim, _pszCTSIntelligentExceptions, L"CTSIntelligentExceptions", L"far|vim"); // !!! "|" delimited! // Don't use IntelliSel in these app-processes
		public: CEOPTIONV(public, CEOptionBool, isCTSAutoCopy, L"CTS.AutoCopy", true);
		CEOPTIONV(public, CEOptionBool, isCTSResetOnRelease, L"CTS.ResetOnRelease", false);
		CEOPTIONV(public, CEOptionBool, isCTSIBeam, L"CTS.IBeam", true);
		CEOPTIONV(public, CEOptionByteEnum, isCTSEndOnTyping, L"CTS.EndOnTyping", CTSEndOnTyping, ceot_Off);
		CEOPTIONV(public, CEOptionBool, isCTSEndOnKeyPress, L"CTS.EndOnKeyPress", false); // +isCTSEndOnTyping. +все, что не генерит WM_CHAR (стрелки и пр.)
		CEOPTIONV(public, CEOptionBool, isCTSEraseBeforeReset, L"CTS.EraseBeforeReset", true);
		CEOPTIONV(public, CEOptionBool, isCTSFreezeBeforeSelect, L"CTS.Freeze", false);
		CEOPTIONV(public, CEOptionBool, isCTSSelectBlock, L"CTS.SelectBlock", true);
		CEOPTIONV(public, CEOptionBool, isCTSSelectText, L"CTS.SelectText", true);
		CEOPTIONV(public, CEOptionByteEnum, isCTSHtmlFormat, L"CTS.HtmlFormat", CTSCopyFormat, CTSFormatText); // MinMax(CTSFormatANSI)
		CEOPTIONV(public, CEOptionDWORD, isCTSForceLocale, L"CTS.ForceLocale", RELEASEDEBUGTEST(0,0x0419/*russian in debug*/)); // Try to bypass clipboard locale problems (pasting to old non-unicode apps)
		CEOPTIOND(public, CEOptionByte, isCTSActMode, L"CTS.ActMode"); // режим и модификатор разрешения действий правой и средней кнопки мышки
		CEOPTIOND(public, CEOptionByte, isCTSRBtnAction, L"CTS.RBtnAction"); // enum: 0-off, 1-copy, 2-paste, 3-auto
		CEOPTIOND(public, CEOptionByte, isCTSMBtnAction, L"CTS.MBtnAction"); // enum: 0-off, 1-copy, 2-paste, 3-auto
		CEOPTIOND(public, CEOptionByte, isCTSColorIndex, L"CTS.ColorIndex");
		CEOPTIOND(public, CEOptionBool, isPasteConfirmEnter, L"ClipboardConfirmEnter");
		CEOPTIOND(public, CEOptionUInt, nPasteConfirmLonger, L"ClipboardConfirmLonger");
		CEOPTIOND(public, CEOptionBool, isFarGotoEditor, L"FarGotoEditorOpt"); // Подсвечивать и переходить на файл/строку (ошибки компилятора)
		CEOPTIOND(public, CEOptionString, sFarGotoEditor, L"FarGotoEditorPath"); // Команда запуска редактора
		CEOPTIOND(public, CEOptionBool, isHighlightMouseRow, L"HighlightMouseRow");
		CEOPTIOND(public, CEOptionBool, isHighlightMouseCol, L"HighlightMouseCol");
		CEOPTIOND(public, CEOptionByte, m_isKeyboardHooks, L"KeyboardHooks"); // if (m_isKeyboardHooks>2) m_isKeyboardHooks = 0;
		CEOPTIOND(public, CEOptionByte, isPartBrush75, L"PartBrush75"); // if (isPartBrush75<5) isPartBrush75=5; else if (isPartBrush75>250) isPartBrush75=250;
		CEOPTIOND(public, CEOptionByte, isPartBrush50, L"PartBrush50"); // if (isPartBrush50<5) isPartBrush50=5; else if (isPartBrush50>250) isPartBrush50=250;
		CEOPTIOND(public, CEOptionByte, isPartBrush25, L"PartBrush25"); if (isPartBrush25<5) isPartBrush25=5; else if (isPartBrush25>250) isPartBrush25=250;
		CEOPTIOND(public, CEOptionByte, isPartBrushBlack, L"PartBrushBlack");
		CEOPTIOND(public, CEOptionByte, isRClickSendKey, L"RightClick opens context menu"); // 0 - не звать EMenu, 1 - звать всегда, 2 - звать по длинному клику
		CEOPTIOND(public, CEOptionString, sRClickMacro, L"RightClickMacro2");
		CEOPTIOND(public, CEOptionBool, isSafeFarClose, L"SafeFarClose");
		CEOPTIOND(public, CEOptionString, sSafeFarCloseMacro, L"SafeFarCloseMacro");
		CEOPTIOND(public, CEOptionBool, isSendAltTab, L"SendAltTab");
		CEOPTIOND(public, CEOptionBool, isSendAltEsc, L"SendAltEsc");
		CEOPTIOND(public, CEOptionBool, isSendAltPrintScrn, L"SendAltPrintScrn");
		CEOPTIOND(public, CEOptionBool, isSendPrintScrn, L"SendPrintScrn");
		CEOPTIOND(public, CEOptionBool, isSendCtrlEsc, L"SendCtrlEsc");
		CEOPTIOND(public, CEOptionBool, mb_MinToTray, L"Min2Tray");
		CEOPTIOND(public, CEOptionBool, mb_AlwaysShowTrayIcon, L"AlwaysShowTrayIcon");
		CEOPTIOND(public, CEOptionByte, isMonospace, L"Monospace"); // 0 - proportional, 1 - monospace, 2 - forcemonospace
		CEOPTIOND(public, CEOptionBool, isRSelFix, L"RSelectionFix");
		CEOPTIOND(public, CEOptionByte, isDragEnabled, L"Dnd");
		CEOPTIOND(public, CEOptionByte, isDropEnabled, L"DndDrop");
		CEOPTIOND(public, CEOptionBool, isDefCopy, L"DefCopy");
		CEOPTIOND(public, CEOptionByte, isDropUseMenu, L"DropUseMenu");
		CEOPTIOND(public, CEOptionBool, isDragOverlay, L"DragOverlay");
		CEOPTIOND(public, CEOptionBool, isDragShowIcons, L"DragShowIcons");
		CEOPTIOND(public, CEOptionByte, isDragPanel, L"DragPanel"); // if (isDragPanel > 2) isDragPanel = 1; // изменение размера панелей мышкой
		CEOPTIOND(public, CEOptionBool, isDragPanelBothEdges, L"DragPanelBothEdges"); // таскать за обе рамки (правую-левой панели и левую-правой панели)
		CEOPTIOND(public, CEOptionBool, isKeyBarRClick, L"KeyBarRClick"); // Правый клик по кейбару - показать PopupMenu
		CEOPTIOND(public, CEOptionBool, isDebugSteps, L"DebugSteps");
		CEOPTIOND(public, CEOptionByte, isDebugLog, L"DebugLog");
		CEOPTIOND(public, CEOptionBool, isEnhanceGraphics, L"EnhanceGraphics"); // Progressbars and scrollbars (pseudographics)
		CEOPTIOND(public, CEOptionBool, isEnhanceButtons, L"EnhanceButtons"); // Buttons, CheckBoxes and RadioButtons (pseudographics)
		CEOPTIOND(public, CEOptionBool, isFadeInactive, L"FadeInactive");
		protected: CEOPTIOND(public, CEOptionByte, mn_FadeLow, L"FadeInactiveLow");
		protected: CEOPTIOND(public, CEOptionByte, mn_FadeHigh, L"FadeInactiveHigh");
		public: CEOPTIOND(public, CEOptionBool, isStatusBarShow, L"StatusBar.Show");
		CEOPTIOND(public, CEOptionDWORD, isStatusBarFlags, L"StatusBar.Flags"); // set of CEStatusFlags
		CEOPTIONV(public, CEOptionStringFixed, sStatusFontFace, L"StatusFontFace", LF_FACESIZE);
		CEOPTIOND(public, CEOptionUInt, nStatusFontCharSet, L"StatusFontCharSet");
		CEOPTIOND(public, CEOptionInt, nStatusFontHeight, L"StatusFontHeight");
		CEOPTIOND(public, CEOptionDWORD, nStatusBarBack, L"StatusBar.Color.Back");
		CEOPTIOND(public, CEOptionDWORD, nStatusBarLight, L"StatusBar.Color.Light");
		CEOPTIOND(public, CEOptionDWORD, nStatusBarDark, L"StatusBar.Color.Dark");
		CEOptionArray<bool,csi_Last,StatusColumnName,StatusColumnDefaults> isStatusColumnHidden;
		CEOPTIOND(public, CEOptionByte, isTabs, L"Tabs"); // 0 - don't show, 1 - always show, 2 - auto show
		CEOPTIOND(public, CEOptionByte, nTabsLocation, L"TabsLocation"); // 0 - top, 1 - bottom
		CEOPTIOND(public, CEOptionBool, isTabIcons, L"TabIcons");
		CEOPTIOND(public, CEOptionBool, isOneTabPerGroup, L"OneTabPerGroup");
		CEOPTIOND(public, CEOptionByte, bActivateSplitMouseOver, L"ActivateSplitMouseOver");
		CEOPTIOND(public, CEOptionBool, isTabSelf, L"TabSelf");
		CEOPTIOND(public, CEOptionBool, isTabRecent, L"TabRecent");
		CEOPTIOND(public, CEOptionBool, isTabLazy, L"TabLazy");
		CEOPTIOND(public, CEOptionInt, nTabFlashChanged, L"TabFlashChanged");
		CEOPTIOND(public, CEOptionUInt, nTabBarDblClickAction, L"TabDblClick"); // 0-None, 1-Auto, 2-Maximize/Restore, 3-NewTab (SettingsNS::tabBarDefaultClickActions)
		CEOPTIOND(public, CEOptionUInt, nTabBtnDblClickAction, L"TabBtnDblClick"); // 0-None, 1-Maximize/Restore, 2-Close, 3-Restart, 4-Duplicate (SettingsNS::tabBtnDefaultClickActions)
		protected: CEOPTIOND(public, CEOptionByte, m_isTabsOnTaskBar, L"TabsOnTaskBar"); // 0 - ConEmu only, 1 - all tabs & all OS, 2 - all tabs & Win 7, 3 - DON'T SHOW
		public: CEOPTIOND(public, CEOptionBool, isTaskbarOverlay, L"TaskBarOverlay");
		CEOPTIOND(public, CEOptionBool, isTaskbarProgress, L"TaskbarProgress");
		CEOPTIONV(public, CEOptionStringFixed, sTabFontFace, L"TabFontFace", LF_FACESIZE);
		CEOPTIOND(public, CEOptionUInt, nTabFontCharSet, L"TabFontCharSet");
		CEOPTIOND(public, CEOptionInt, nTabFontHeight, L"TabFontHeight");
		CEOPTIOND(public, CEOptionString, sTabCloseMacro, L"TabCloseMacro"); //if (!reg->Load(L"TabCloseMacro", &sTabCloseMacro) || (sTabCloseMacro && !*sTabCloseMacro)) { if (sTabCloseMacro) { free(sTabCloseMacro); sTabCloseMacro = NULL; } }
		CEOPTIOND(public, CEOptionString, sSaveAllMacro, L"SaveAllEditors"); //if (!reg->Load(L"SaveAllEditors", &sSaveAllMacro)) { sSaveAllMacro = lstrdup(L"...
		CEOPTIOND(public, CEOptionInt, nToolbarAddSpace, L"ToolbarAddSpace"); // UInt?
		CEOPTIOND(public, CEOptionSize, wndWidth, L"ConWnd Width");
		CEOPTIOND(public, CEOptionSize, wndHeight, L"ConWnd Height");
		CEOPTIOND(public, CEOptionUInt, ntvdmHeight, L"16bit Height"); // в символах
		CEOPTIOND(public, CEOptionInt, _wndX, L"ConWnd X"); // в пикселях
		CEOPTIOND(public, CEOptionInt, _wndY, L"ConWnd Y"); // в пикселях
		CEOPTIOND(public, CEOptionDWORD, _WindowMode, L"WindowMode"); // if (WindowMode!=wmFullScreen && WindowMode!=wmMaximized && WindowMode!=wmNormal) WindowMode = wmNormal;
		CEOPTIOND(public, CEOptionBool, wndCascade, L"Cascaded");
		CEOPTIOND(public, CEOptionBool, isAutoSaveSizePos, L"AutoSaveSizePos");
		CEOPTIOND(public, CEOptionBool, isUseCurrentSizePos, L"UseCurrentSizePos"); // Show in settings dialog and save current window size/pos
		CEOPTIOND(public, CEOptionUInt, nSlideShowElapse, L"SlideShowElapse");
		CEOPTIOND(public, CEOptionUInt, nIconID, L"IconID");
		CEOPTIOND(public, CEOptionBool, isTryToCenter, L"TryToCenter");
		CEOPTIOND(public, CEOptionUInt, nCenterConsolePad, L"CenterConsolePad");
		CEOPTIOND(public, CEOptionByte, isAlwaysShowScrollbar, L"ShowScrollbar"); // if (isAlwaysShowScrollbar > 2) isAlwaysShowScrollbar = 2; // 0-не показывать, 1-всегда, 2-автоматически (на откусывает место от консоли)
		CEOPTIOND(public, CEOptionUInt, nScrollBarAppearDelay, L"ScrollBarAppearDelay");
		CEOPTIOND(public, CEOptionUInt, nScrollBarDisappearDelay, L"ScrollBarDisappearDelay");
		CEOPTIOND(public, CEOptionBool, isSingleInstance, L"SingleInstance");
		CEOPTIOND(public, CEOptionBool, isShowHelpTooltips, L"ShowHelpTooltips");
		CEOPTIOND(public, CEOptionBool, mb_isMulti, L"Multi");
		CEOPTIOND(public, CEOptionBool, isMultiShowButtons, L"Multi.ShowButtons");
		CEOPTIOND(public, CEOptionBool, isMultiShowSearch, L"Multi.ShowSearch");
		CEOPTIOND(public, CEOptionBool, isNumberInCaption, L"NumberInCaption");
		private: CEOPTIOND(public, CEOptionDWORD, nHostkeyNumberModifier, L"Multi.Modifier"); // TestHostkeyModifiers(); // Используется для 0..9, WinSize
		private: CEOPTIOND(public, CEOptionUInt, nHostkeyArrowModifier, L"Multi.ArrowsModifier"); // TestHostkeyModifiers(); // Используется для WinSize
		public: CEOPTIOND(public, CEOptionByte, nCloseConfirmFlags, L"Multi.CloseConfirm"); // CloseConfirmOptions
		CEOPTIOND(public, CEOptionBool, isMultiAutoCreate, L"Multi.AutoCreate");
		CEOPTIOND(public, CEOptionByte, isMultiLeaveOnClose, L"Multi.LeaveOnClose"); // 0 - закрываться, 1 - оставаться, 2 - НЕ оставаться при закрытии "крестиком"
		CEOPTIOND(public, CEOptionByte, isMultiHideOnClose, L"Multi.HideOnClose"); // 0 - не скрываться, 1 - в трей, 2 - просто минимизация
		CEOPTIOND(public, CEOptionByte, isMultiMinByEsc, L"Multi.MinByEsc"); // 0 - Never, 1 - Always, 2 - NoConsoles
		CEOPTIOND(public, CEOptionBool, isMapShiftEscToEsc, L"MapShiftEscToEsc"); // used only when isMultiMinByEsc==1 and only for console apps
		CEOPTIOND(public, CEOptionBool, isMultiIterate, L"Multi.Iterate");
		CEOPTIOND(public, CEOptionBool, isMultiNewConfirm, L"Multi.NewConfirm");
		CEOPTIOND(public, CEOptionBool, isMultiDupConfirm, L"Multi.DupConfirm");
		CEOPTIOND(public, CEOptionBool, isMultiDetachConfirm, L"Multi.DetachConfirm");
		CEOPTIOND(public, CEOptionBool, isUseWinNumber, L"Multi.UseNumbers");
		CEOPTIOND(public, CEOptionBool, isUseWinTab, L"Multi.UseWinTab");
		CEOPTIOND(public, CEOptionBool, isUseWinArrows, L"Multi.UseArrows");
		CEOPTIOND(public, CEOptionByte, nSplitWidth, L"Multi.SplitWidth");
		CEOPTIOND(public, CEOptionByte, nSplitHeight, L"Multi.SplitHeight");
		CEOPTIOND(public, CEOptionBool, isFARuseASCIIsort, L"FARuseASCIIsort");
		CEOPTIOND(public, CEOptionBool, isFixAltOnAltTab, L"FixAltOnAltTab");
		CEOPTIOND(public, CEOptionBool, isUseAltGrayPlus, L"UseAltGrayPlus");
		CEOPTIOND(public, CEOptionBool, isShellNoZoneCheck, L"ShellNoZoneCheck");
		CEOPTIONV(public, CEOptionStringFixed, szTabConsole, L"TabConsole", 32);
		CEOPTIONV(public, CEOptionStringFixed, szTabModifiedSuffix, L"TabModifiedSuffix", 16);
		CEOPTIOND(public, CEOptionString, pszTabSkipWords, L"TabSkipWords");
		CEOPTIONV(public, CEOptionStringFixed, szTabPanels, L"TabPanels", 32);
		CEOPTIONV(public, CEOptionStringFixed, szTabEditor, L"TabEditor", 32);
		CEOPTIONV(public, CEOptionStringFixed, szTabEditorModified, L"TabEditorModified", 32);
		CEOPTIONV(public, CEOptionStringFixed, szTabViewer, L"TabViewer", 32);
		CEOPTIOND(public, CEOptionUInt, nTabLenMax, L"TabLenMax"); // if (nTabLenMax < 10 || nTabLenMax >= CONEMUTABMAX) nTabLenMax = 20;
		CEOPTIONV(public, CEOptionStringFixed, szAdminTitleSuffix, L"AdminTitleSuffix", 64); // DefaultAdminTitleSuffix /* " (Admin)" */
		CEOPTIOND(public, CEOptionByte, bAdminShield, L"AdminShowShield"); // enum AdminTabStyle
		CEOPTIOND(public, CEOptionBool, bHideInactiveConsoleTabs, L"HideInactiveConsoleTabs");
		CEOPTIOND(public, CEOptionBool, bShowFarWindows, L"ShowFarWindows");
		CEOPTIONV(public, CEOptionUInt, nMainTimerElapse, L"MainTimerElapse", 10); // if (nMainTimerElapse>1000) nMainTimerElapse = 1000; // периодичность, с которой из консоли считывается текст
		CEOPTIONV(public, CEOptionUInt, nMainTimerInactiveElapse, L"MainTimerInactiveElapse", 1000); // if (nMainTimerInactiveElapse>10000) nMainTimerInactiveElapse = 10000; // периодичность при неактивности
		CEOPTIOND(public, CEOptionBool, isSkipFocusEvents, L"SkipFocusEvents");
		CEOPTIOND(public, CEOptionByte, isMonitorConsoleLang, L"MonitorConsoleLang"); // bitmask. 1 - follow up console HKL (e.g. after XLat in Far Manager), 2 - use one HKL for all tabs
		CEOPTIOND(public, CEOptionBool, isSleepInBackground, L"SleepInBackground");
		CEOPTIOND(public, CEOptionBool, isRetardInactivePanes, L"RetardInactivePanes");
		CEOPTIOND(public, CEOptionBool, mb_MinimizeOnLoseFocus, L"MinimizeOnLoseFocus");
		CEOPTIOND(public, CEOptionDWORD, nAffinity, L"AffinityMask");
		CEOPTIOND(public, CEOptionBool, isUseInjects, L"UseInjects"); // NB. Root process is infiltrated always.
		CEOPTIOND(public, CEOptionBool, isProcessAnsi, L"ProcessAnsi"); // ANSI X3.64 & XTerm-256-colors Support
		CEOPTIOND(public, CEOptionBool, isAnsiLog, L"AnsiLog"); // Limited logging of console contents (same output as processed by CECF_ProcessAnsi)
		CEOPTIOND(public, CEOptionString, pszAnsiLog, L"AnsiLogPath");
		CEOPTIOND(public, CEOptionBool, isProcessNewConArg, L"ProcessNewConArg"); // Enable processing of '-new_console' and '-cur_console' switches in your shell prompt, scripts etc. started in ConEmu tabs
		CEOPTIOND(public, CEOptionBool, isProcessCmdStart, L"ProcessCmdStart"); // Use "start xxx.exe" to start new tab
		CEOPTIOND(public, CEOptionBool, isProcessCtrlZ, L"ProcessCtrlZ"); // Treat Ctrl-Z as ‘EndOfStream’. On new line press Ctrl-Z and Enter. Refer to the gh#465 for details (Go input streams).
		CEOPTIOND(public, CEOptionBool, mb_UseClink, L"UseClink"); // использовать расширение командной строки (ReadConsole)
		CEOPTIOND(public, CEOptionBool, isSuppressBells, L"SuppressBells");
		CEOPTIOND(public, CEOptionBool, isConsoleExceptionHandler, L"ConsoleExceptionHandler");
		CEOPTIOND(public, CEOptionBool, isConVisible, L"ConVisible"); /* *** Debugging *** */

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
		enum TabsOnTaskbar { tot_ConEmuOnly = 0, tot_AllTabsAllOS = 1, tot_AllTabsWin7 = 2, tot_DontShow = 3};
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
