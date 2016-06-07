
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

	public:
		CEOptionInt<L"DefaultBufferHeight", 1000> DefaultBufferHeight;
		CEOptionBool<L"AutoBufferHeight", true> AutoBufferHeight; // Long console output
		CEOptionBool<L"UseScrollLock", true> UseScrollLock;
		CEOptionInt<L"CmdOutputCP", 0> nCmdOutputCP;
		CEOptionComSpec ComSpec; // Defaults are set in CEOptionComSpec::Reset
		CEOptionStringDelim<L"DefaultTerminalApps", L"explorer.exe"> DefaultTerminalApps; // Stored as "|"-delimited string
		private: CEOptionArray<COLORREF, 0x20, ColorTableIndexName/*"ColorTableNN" decimal*/, ColorTableDefaults/*<ConEmu>*/> Colors; // L"ColorTableNN", Colors[i]
		public: CEOptionBool<L"FontAutoSize", false> isFontAutoSize;
		CEOptionBool<L"AutoRegisterFonts", true> isAutoRegisterFonts;
		CEOptionBool<L"TrueColorerSupport", true> isTrueColorer;
		CEOptionBool<L"VividColors", true> isVividColors;
		CEOptionByte<L"BackGround Image show", 0> isShowBgImage;
		CEOptionStringFixed<L"BackGround Image",MAX_PATH, L"%USERPROFILE%\\back.jpg"> sBgImage;
		CEOptionByte<L"bgImageDarker", 255> bgImageDarker;
		CEOptionDWORD<L"bgImageColors", (DWORD)-1> nBgImageColors;
		CEOptionByteEnum<L"bgOperation",BackgroundOp, eUpLeft> bgOperation; // BackgroundOp {eUpLeft = 0, eStretch = 1, eTile = 2, ...}
		CEOptionByte<L"bgPluginAllowed", 1> isBgPluginAllowed;
		CEOptionByte<L"AlphaValue", 255> nTransparent;
		CEOptionBool<L"AlphaValueSeparate", false> isTransparentSeparate;
		CEOptionByte<L"AlphaValueInactive", 255> nTransparentInactive;
		CEOptionBool<L"UserScreenTransparent", false> isUserScreenTransparent;
		CEOptionBool<L"ColorKeyTransparent", false> isColorKeyTransparent;
		CEOptionDWORD<L"ColorKeyValue", RGB(1,1,1)> nColorKeyValue;
		CEOptionBool<L"SaveCmdHistory", true> isSaveCmdHistory;
		//TODO: CEOptionString<L"CmdHistoryLocation"> psHistoryLocation;
		CEOptionByteEnum<L"StartType",StartupType, start_Command> nStartType; // !!! POST VALIDATION IS REQUIRED !!!
		CEOptionString<L"CmdLine"> psStartSingleApp;
		CEOptionString<L"StartTasksFile"> psStartTasksFile;
		CEOptionString<L"StartTasksName"> psStartTasksName;
		CEOptionBool<L"StartFarFolders"> isStartFarFolders;
		CEOptionBool<L"StartFarEditors"> isStartFarEditors;
		CEOptionUInt<L"StartCreateDelay", RUNQUEUE_CREATE_LAG_DEF> nStartCreateDelay; // RUNQUEUE_CREATE_LAG
		CEOptionBool<L"StoreTaskbarkTasks"> isStoreTaskbarkTasks;
		CEOptionBool<L"StoreTaskbarCommands"> isStoreTaskbarCommands;
		CEOptionStringFixed<L"FontName",LF_FACESIZE> inFont;
		CEOptionBool<L"FontBold", false> isBold;
		CEOptionBool<L"FontItalic", false> isItalic;
		CEOptionUInt<L"Anti-aliasing", CLEARTYPE_NATURAL_QUALITY, get_antialias_default, antialias_validate> mn_AntiAlias;
		CEOptionByte<L"FontCharSet", DEFAULT_CHARSET> mn_LoadFontCharSet; // Loaded or Saved to settings // !!! mb_CharSetWasSet = FALSE;
		CEOptionUInt<L"FontSize",DEF_FONTSIZEY_P> FontSizeY;  // высота основного шрифта (загруженная из настроек!)
		CEOptionUInt<L"FontSizeX"> FontSizeX;  // ширина основного шрифта
		CEOptionUInt<L"FontSizeX3",0> FontSizeX3; // ширина знакоместа при моноширинном режиме (не путать с FontSizeX2)
		CEOptionBool<L"FontUseDpi",true> FontUseDpi;
		CEOptionBool<L"FontUseUnits",false> FontUseUnits;
		CEOptionBool<L"Anti-aliasing2",false> isAntiAlias2; // disabled by default to avoid dashed framed
		CEOptionBool<L"HideCaption",false> isHideCaption; // Hide caption when maximized
		CEOptionBool<L"HideChildCaption",true> isHideChildCaption; // Hide caption of child GUI applications, started in ConEmu tabs (PuTTY, Notepad, etc.)
		CEOptionBool<L"FocusInChildWindows",true> isFocusInChildWindows;
		CEOptionBool<L"IntegralSize",false> mb_IntegralSize;
		CEOptionByteEnum<L"QuakeStyle",ConEmuQuakeMode, quake_Disabled> isQuakeStyle;
		CEOptionBool<L"Restore2ActiveMon",false> isRestore2ActiveMon;
		protected: CEOptionBool<L"HideCaptionAlways",false> mb_HideCaptionAlways;
		public: CEOptionByte<L"HideCaptionAlwaysFrame",HIDECAPTIONALWAYSFRAME_DEF> nHideCaptionAlwaysFrame;
		CEOptionUInt<L"HideCaptionAlwaysDelay",2000> nHideCaptionAlwaysDelay;
		CEOptionUInt<L"HideCaptionAlwaysDisappear",2000> nHideCaptionAlwaysDisappear;
		CEOptionBool<L"DownShowHiddenMessage",false> isDownShowHiddenMessage;
		CEOptionBool<L"DownShowExOnTopMessage",false> isDownShowExOnTopMessage;
		CEOptionBool<L"AlwaysOnTop",false> isAlwaysOnTop;
		CEOptionBool<L"SnapToDesktopEdges",false> isSnapToDesktopEdges;
		CEOptionBool<L"ExtendUCharMap",true> isExtendUCharMap; // !!! FAR
		CEOptionBool<L"DisableMouse",false> isDisableMouse;
		CEOptionBool<L"MouseSkipActivation",true> isMouseSkipActivation;
		CEOptionBool<L"MouseSkipMoving",true> isMouseSkipMoving;
		CEOptionBool<L"MouseDragWindow",true> isMouseDragWindow;
		CEOptionBool<L"FarHourglass",true> isFarHourglass;
		CEOptionUInt<L"FarHourglassDelay",500> nFarHourglassDelay;
		CEOptionByte<L"DisableFarFlashing",false> isDisableFarFlashing; // if (isDisableFarFlashing>2) isDisableFarFlashing = 2;
		CEOptionUInt<L"DisableAllFlashing",false> isDisableAllFlashing; // if (isDisableAllFlashing>2) isDisableAllFlashing = 2;
		CEOptionBool<L"CTSIntelligent",true> isCTSIntelligent;
		private: CEOptionStringDelim<L"CTSIntelligentExceptions", L"far|vim"> _pszCTSIntelligentExceptions; // !!! "|" delimited! // Don't use IntelliSel in these app-processes
		public: CEOptionBool<L"CTS.AutoCopy",true> isCTSAutoCopy;
		CEOptionBool<L"CTS.ResetOnRelease",false> isCTSResetOnRelease;
		CEOptionBool<L"CTS.IBeam",true> isCTSIBeam;
		CEOptionByteEnum<L"CTS.EndOnTyping",CTSEndOnTyping, ceot_Off> isCTSEndOnTyping;
		CEOptionBool<L"CTS.EndOnKeyPress",false> isCTSEndOnKeyPress; // +isCTSEndOnTyping. +все, что не генерит WM_CHAR (стрелки и пр.)
		CEOptionBool<L"CTS.EraseBeforeReset",true> isCTSEraseBeforeReset;
		CEOptionBool<L"CTS.Freeze",false> isCTSFreezeBeforeSelect;
		CEOptionBool<L"CTS.SelectBlock",true> isCTSSelectBlock;
		CEOptionBool<L"CTS.SelectText",true> isCTSSelectText;
		CEOptionByteEnum<L"CTS.HtmlFormat",CTSCopyFormat, CTSFormatText> isCTSHtmlFormat; // MinMax(CTSFormatANSI)
		CEOptionDWORD<L"CTS.ForceLocale", RELEASEDEBUGTEST(0,0x0419/*russian in debug*/)> isCTSForceLocale; // Try to bypass clipboard locale problems (pasting to old non-unicode apps)
		CEOptionByte<L"CTS.ActMode"> isCTSActMode; // режим и модификатор разрешения действий правой и средней кнопки мышки
		CEOptionByte<L"CTS.RBtnAction"> isCTSRBtnAction; // enum: 0-off, 1-copy, 2-paste, 3-auto
		CEOptionByte<L"CTS.MBtnAction"> isCTSMBtnAction; // enum: 0-off, 1-copy, 2-paste, 3-auto
		CEOptionByte<L"CTS.ColorIndex"> isCTSColorIndex;
		CEOptionBool<L"ClipboardConfirmEnter"> isPasteConfirmEnter;
		CEOptionUInt<L"ClipboardConfirmLonger"> nPasteConfirmLonger;
		CEOptionBool<L"FarGotoEditorOpt"> isFarGotoEditor; // Подсвечивать и переходить на файл/строку (ошибки компилятора)
		CEOptionString<L"FarGotoEditorPath"> sFarGotoEditor; // Команда запуска редактора
		CEOptionBool<L"HighlightMouseRow"> isHighlightMouseRow;
		CEOptionBool<L"HighlightMouseCol"> isHighlightMouseCol;
		CEOptionByte<L"KeyboardHooks"> m_isKeyboardHooks; // if (m_isKeyboardHooks>2) m_isKeyboardHooks = 0;
		CEOptionByte<L"PartBrush75"> isPartBrush75; // if (isPartBrush75<5) isPartBrush75=5; else if (isPartBrush75>250) isPartBrush75=250;
		CEOptionByte<L"PartBrush50"> isPartBrush50; // if (isPartBrush50<5) isPartBrush50=5; else if (isPartBrush50>250) isPartBrush50=250;
		CEOptionByte<L"PartBrush25"> isPartBrush25; if (isPartBrush25<5) isPartBrush25=5; else if (isPartBrush25>250) isPartBrush25=250;
		CEOptionByte<L"PartBrushBlack"> isPartBrushBlack;
		CEOptionByte<L"RightClick opens context menu"> isRClickSendKey; // 0 - не звать EMenu, 1 - звать всегда, 2 - звать по длинному клику
		CEOptionString<L"RightClickMacro2"> sRClickMacro;
		CEOptionBool<L"SafeFarClose"> isSafeFarClose;
		CEOptionString<L"SafeFarCloseMacro"> sSafeFarCloseMacro;
		CEOptionBool<L"SendAltTab"> isSendAltTab;
		CEOptionBool<L"SendAltEsc"> isSendAltEsc;
		CEOptionBool<L"SendAltPrintScrn"> isSendAltPrintScrn;
		CEOptionBool<L"SendPrintScrn"> isSendPrintScrn;
		CEOptionBool<L"SendCtrlEsc"> isSendCtrlEsc;
		CEOptionBool<L"Min2Tray"> mb_MinToTray;
		CEOptionBool<L"AlwaysShowTrayIcon"> mb_AlwaysShowTrayIcon;
		CEOptionByte<L"Monospace"> isMonospace; // 0 - proportional, 1 - monospace, 2 - forcemonospace
		CEOptionBool<L"RSelectionFix"> isRSelFix;
		CEOptionByte<L"Dnd"> isDragEnabled;
		CEOptionByte<L"DndDrop"> isDropEnabled;
		CEOptionBool<L"DefCopy"> isDefCopy;
		CEOptionByte<L"DropUseMenu"> isDropUseMenu;
		CEOptionBool<L"DragOverlay"> isDragOverlay;
		CEOptionBool<L"DragShowIcons"> isDragShowIcons;
		CEOptionByte<L"DragPanel"> isDragPanel; // if (isDragPanel > 2) isDragPanel = 1; // изменение размера панелей мышкой
		CEOptionBool<L"DragPanelBothEdges"> isDragPanelBothEdges; // таскать за обе рамки (правую-левой панели и левую-правой панели)
		CEOptionBool<L"KeyBarRClick"> isKeyBarRClick; // Правый клик по кейбару - показать PopupMenu
		CEOptionBool<L"DebugSteps"> isDebugSteps;
		CEOptionByte<L"DebugLog"> isDebugLog;
		CEOptionBool<L"EnhanceGraphics"> isEnhanceGraphics; // Progressbars and scrollbars (pseudographics)
		CEOptionBool<L"EnhanceButtons"> isEnhanceButtons; // Buttons, CheckBoxes and RadioButtons (pseudographics)
		CEOptionBool<L"FadeInactive"> isFadeInactive;
		protected: CEOptionByte<L"FadeInactiveLow"> mn_FadeLow;
		protected: CEOptionByte<L"FadeInactiveHigh"> mn_FadeHigh;
		public: CEOptionBool<L"StatusBar.Show"> isStatusBarShow;
		CEOptionDWORD<L"StatusBar.Flags"> isStatusBarFlags; // set of CEStatusFlags
		CEOptionStringFixed<L"StatusFontFace",LF_FACESIZE> sStatusFontFace;
		CEOptionUInt<L"StatusFontCharSet"> nStatusFontCharSet;
		CEOptionInt<L"StatusFontHeight"> nStatusFontHeight;
		CEOptionDWORD<L"StatusBar.Color.Back"> nStatusBarBack;
		CEOptionDWORD<L"StatusBar.Color.Light"> nStatusBarLight;
		CEOptionDWORD<L"StatusBar.Color.Dark"> nStatusBarDark;
		CEOptionArray<bool,csi_Last,StatusColumnName,StatusColumnDefaults> isStatusColumnHidden;
		CEOptionByte<L"Tabs"> isTabs; // 0 - don't show, 1 - always show, 2 - auto show
		CEOptionByte<L"TabsLocation"> nTabsLocation; // 0 - top, 1 - bottom
		CEOptionBool<L"TabIcons"> isTabIcons;
		CEOptionBool<L"OneTabPerGroup"> isOneTabPerGroup;
		CEOptionByte<L"ActivateSplitMouseOver"> bActivateSplitMouseOver;
		CEOptionBool<L"TabSelf"> isTabSelf;
		CEOptionBool<L"TabRecent"> isTabRecent;
		CEOptionBool<L"TabLazy"> isTabLazy;
		CEOptionInt<L"TabFlashChanged"> nTabFlashChanged;
		CEOptionUInt<L"TabDblClick"> nTabBarDblClickAction; // 0-None, 1-Auto, 2-Maximize/Restore, 3-NewTab (SettingsNS::tabBarDefaultClickActions)
		CEOptionUInt<L"TabBtnDblClick"> nTabBtnDblClickAction; // 0-None, 1-Maximize/Restore, 2-Close, 3-Restart, 4-Duplicate (SettingsNS::tabBtnDefaultClickActions)
		protected: CEOptionByte<L"TabsOnTaskBar"> m_isTabsOnTaskBar; // 0 - ConEmu only, 1 - all tabs & all OS, 2 - all tabs & Win 7, 3 - DON'T SHOW
		public: CEOptionBool<L"TaskBarOverlay"> isTaskbarOverlay;
		CEOptionBool<L"TaskbarProgress"> isTaskbarProgress;
		CEOptionStringFixed<L"TabFontFace",LF_FACESIZE> sTabFontFace;
		CEOptionUInt<L"TabFontCharSet"> nTabFontCharSet;
		CEOptionInt<L"TabFontHeight"> nTabFontHeight;
		CEOptionString<L"TabCloseMacro"> sTabCloseMacro; //if (!reg->Load(L"TabCloseMacro", &sTabCloseMacro) || (sTabCloseMacro && !*sTabCloseMacro)) { if (sTabCloseMacro) { free(sTabCloseMacro); sTabCloseMacro = NULL; } }
		CEOptionString<L"SaveAllEditors"> sSaveAllMacro; //if (!reg->Load(L"SaveAllEditors", &sSaveAllMacro)) { sSaveAllMacro = lstrdup(L"...
		CEOptionInt<L"ToolbarAddSpace"> nToolbarAddSpace; // UInt?
		CEOptionSize<L"ConWnd Width"> wndWidth;
		CEOptionSize<L"ConWnd Height"> wndHeight;
		CEOptionUInt<L"16bit Height"> ntvdmHeight; // в символах
		CEOptionInt<L"ConWnd X"> _wndX; // в пикселях
		CEOptionInt<L"ConWnd Y"> _wndY; // в пикселях
		CEOptionDWORD<L"WindowMode"> _WindowMode; // if (WindowMode!=wmFullScreen && WindowMode!=wmMaximized && WindowMode!=wmNormal) WindowMode = wmNormal;
		CEOptionBool<L"Cascaded"> wndCascade;
		CEOptionBool<L"AutoSaveSizePos"> isAutoSaveSizePos;
		CEOptionBool<L"UseCurrentSizePos"> isUseCurrentSizePos; // Show in settings dialog and save current window size/pos
		CEOptionUInt<L"SlideShowElapse"> nSlideShowElapse;
		CEOptionUInt<L"IconID"> nIconID;
		CEOptionBool<L"TryToCenter"> isTryToCenter;
		CEOptionUInt<L"CenterConsolePad"> nCenterConsolePad;
		CEOptionByte<L"ShowScrollbar"> isAlwaysShowScrollbar; // if (isAlwaysShowScrollbar > 2) isAlwaysShowScrollbar = 2; // 0-не показывать, 1-всегда, 2-автоматически (на откусывает место от консоли)
		CEOptionUInt<L"ScrollBarAppearDelay"> nScrollBarAppearDelay;
		CEOptionUInt<L"ScrollBarDisappearDelay"> nScrollBarDisappearDelay;
		CEOptionBool<L"SingleInstance"> isSingleInstance;
		CEOptionBool<L"ShowHelpTooltips"> isShowHelpTooltips;
		CEOptionBool<L"Multi"> mb_isMulti;
		CEOptionBool<L"Multi.ShowButtons"> isMultiShowButtons;
		CEOptionBool<L"Multi.ShowSearch"> isMultiShowSearch;
		CEOptionBool<L"NumberInCaption"> isNumberInCaption;
		private: CEOptionDWORD<L"Multi.Modifier"> nHostkeyNumberModifier; // TestHostkeyModifiers(); // Используется для 0..9, WinSize
		private: CEOptionUInt<L"Multi.ArrowsModifier"> nHostkeyArrowModifier; // TestHostkeyModifiers(); // Используется для WinSize
		public: CEOptionByte<L"Multi.CloseConfirm"> nCloseConfirmFlags; // CloseConfirmOptions
		CEOptionBool<L"Multi.AutoCreate"> isMultiAutoCreate;
		CEOptionByte<L"Multi.LeaveOnClose"> isMultiLeaveOnClose; // 0 - закрываться, 1 - оставаться, 2 - НЕ оставаться при закрытии "крестиком"
		CEOptionByte<L"Multi.HideOnClose"> isMultiHideOnClose; // 0 - не скрываться, 1 - в трей, 2 - просто минимизация
		CEOptionByte<L"Multi.MinByEsc"> isMultiMinByEsc; // 0 - Never, 1 - Always, 2 - NoConsoles
		CEOptionBool<L"MapShiftEscToEsc"> isMapShiftEscToEsc; // used only when isMultiMinByEsc==1 and only for console apps
		CEOptionBool<L"Multi.Iterate"> isMultiIterate;
		CEOptionBool<L"Multi.NewConfirm"> isMultiNewConfirm;
		CEOptionBool<L"Multi.DupConfirm"> isMultiDupConfirm;
		CEOptionBool<L"Multi.DetachConfirm"> isMultiDetachConfirm;
		CEOptionBool<L"Multi.UseNumbers"> isUseWinNumber;
		CEOptionBool<L"Multi.UseWinTab"> isUseWinTab;
		CEOptionBool<L"Multi.UseArrows"> isUseWinArrows;
		CEOptionByte<L"Multi.SplitWidth"> nSplitWidth;
		CEOptionByte<L"Multi.SplitHeight"> nSplitHeight;
		CEOptionBool<L"FARuseASCIIsort"> isFARuseASCIIsort;
		CEOptionBool<L"FixAltOnAltTab"> isFixAltOnAltTab;
		CEOptionBool<L"UseAltGrayPlus"> isUseAltGrayPlus;
		CEOptionBool<L"ShellNoZoneCheck"> isShellNoZoneCheck;
		CEOptionStringFixed<L"TabConsole",32> szTabConsole;
		CEOptionStringFixed<L"TabModifiedSuffix",16> szTabModifiedSuffix;
		CEOptionString<L"TabSkipWords"> pszTabSkipWords;
		CEOptionStringFixed<L"TabPanels",32> szTabPanels;
		CEOptionStringFixed<L"TabEditor",32> szTabEditor;
		CEOptionStringFixed<L"TabEditorModified",32> szTabEditorModified;
		CEOptionStringFixed<L"TabViewer",32> szTabViewer;
		CEOptionUInt<L"TabLenMax"> nTabLenMax; // if (nTabLenMax < 10 || nTabLenMax >= CONEMUTABMAX) nTabLenMax = 20;
		CEOptionStringFixed<L"AdminTitleSuffix",64> szAdminTitleSuffix; // DefaultAdminTitleSuffix /* " (Admin)" */
		CEOptionByte<L"AdminShowShield"> bAdminShield; // enum AdminTabStyle
		CEOptionBool<L"HideInactiveConsoleTabs"> bHideInactiveConsoleTabs;
		CEOptionBool<L"ShowFarWindows"> bShowFarWindows;
		CEOptionUInt<L"MainTimerElapse" nMainTimerElapse; // if (nMainTimerElapse>1000) nMainTimerElapse = 1000; // периодичность, с которой из консоли считывается текст
		CEOptionUInt<L"MainTimerInactiveElapse"> nMainTimerInactiveElapse; // if (nMainTimerInactiveElapse>10000) nMainTimerInactiveElapse = 10000; // периодичность при неактивности
		CEOptionBool<L"SkipFocusEvents"> isSkipFocusEvents;
		CEOptionByte<L"MonitorConsoleLang"> isMonitorConsoleLang; // bitmask. 1 - follow up console HKL (e.g. after XLat in Far Manager), 2 - use one HKL for all tabs
		CEOptionBool<L"SleepInBackground"> isSleepInBackground;
		CEOptionBool<L"RetardInactivePanes"> isRetardInactivePanes;
		CEOptionBool<L"MinimizeOnLoseFocus"> mb_MinimizeOnLoseFocus;
		CEOptionDWORD<L"AffinityMask"> nAffinity;
		CEOptionBool<L"UseInjects"> isUseInjects; // NB. Root process is infiltrated always.
		CEOptionBool<L"ProcessAnsi"> isProcessAnsi; // ANSI X3.64 & XTerm-256-colors Support
		CEOptionBool<L"AnsiLog"> isAnsiLog; // Limited logging of console contents (same output as processed by CECF_ProcessAnsi)
		CEOptionString<L"AnsiLogPath"> pszAnsiLog;
		CEOptionBool<L"ProcessNewConArg"> isProcessNewConArg; // Enable processing of '-new_console' and '-cur_console' switches in your shell prompt, scripts etc. started in ConEmu tabs
		CEOptionBool<L"ProcessCmdStart"> isProcessCmdStart; // Use "start xxx.exe" to start new tab
		CEOptionBool<L"ProcessCtrlZ"> isProcessCtrlZ; // Treat Ctrl-Z as ‘EndOfStream’. On new line press Ctrl-Z and Enter. Refer to the gh#465 for details (Go input streams).
		CEOptionBool<L"UseClink"> mb_UseClink; // использовать расширение командной строки (ReadConsole)
		CEOptionBool<L"SuppressBells"> isSuppressBells;
		CEOptionBool<L"ConsoleExceptionHandler"> isConsoleExceptionHandler;
		CEOptionBool<L"ConVisible"> isConVisible; /* *** Debugging *** */

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


		//
		struct ConEmuProgressStore
		{
			// This two are stored in settings
			wchar_t* pszName;
			DWORD    nDuration;
			// Following are used in runtime
			DWORD    nStartTick;

			void FreePtr()
			{
				SafeFree(pszName);
			};
		};

		DWORD ProgressesGetDuration(LPCWSTR asName);
		void ProgressesSetDuration(LPCWSTR asName, DWORD anDuration);

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

		int ProgressesCount;
		ConEmuProgressStore** Progresses;
		bool LoadProgress(SettingsBase* reg, ConEmuProgressStore* &pProgress);
		bool SaveProgress(SettingsBase* reg, ConEmuProgressStore* pProgress);
		void FreeProgresses();

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
