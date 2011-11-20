
/*
Copyright (c) 2009-2011 Maximus5
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

#define CONEMU_ROOT_KEY L"Software\\ConEmu"

#define MIN_ALPHA_VALUE 40
#define MAX_FONT_STYLES 8  //normal/(bold|italic|underline)
#define MAX_FONT_GROUPS 20 // Main, Borders, Japan, Cyrillic, ...

#include <pshpack1.h>
typedef struct tagMYRGB
{
	union
	{
		COLORREF color;
		struct
		{
			BYTE    rgbBlue;
			BYTE    rgbGreen;
			BYTE    rgbRed;
			BYTE    rgbReserved;
		};
	};
} MYRGB, MYCOLORREF;
#include <poppack.h>

enum BackgroundOp
{
	eUpLeft = 0,
	eStretch = 1,
	eTile = 2,
};

#define BgImageColorsDefaults (1|2)

class CSettings;

struct Settings
{
	public:
		Settings();
		~Settings();
	protected:
		friend class CSettings;
	
		void ReleasePointers();		
	public:

		wchar_t Type[16]; // Информационно: L"[reg]" или L"[xml]"

		//reg->Load(L"DefaultBufferHeight", DefaultBufferHeight);
		int DefaultBufferHeight;
		
		//bool AutoScroll;
		
		//reg->Load(L"AutoBufferHeight", AutoBufferHeight);
		bool AutoBufferHeight;
		//reg->Load(L"CmdOutputCP", nCmdOutputCP);
		int nCmdOutputCP;

	public:
		//reg->Load(L"FontAutoSize", isFontAutoSize);
		bool isFontAutoSize;
		//reg->Load(L"AutoRegisterFonts", isAutoRegisterFonts);
		bool isAutoRegisterFonts;
		
		//reg->Load(L"ConsoleFontName", ConsoleFont.lfFaceName, countof(ConsoleFont.lfFaceName));
		//reg->Load(L"ConsoleFontWidth", ConsoleFont.lfWidth);
		//reg->Load(L"ConsoleFontHeight", ConsoleFont.lfHeight);
		LOGFONT ConsoleFont;
		
		COLORREF* GetColors(BOOL abFade = FALSE);
		COLORREF GetFadeColor(COLORREF cr);
		bool NeedDialogDetect();
		
		//reg->Load(L"ExtendColors", isExtendColors);
		bool isExtendColors;
		//reg->Load(L"ExtendColorIdx", nExtendColor);
		char nExtendColor; // 0..15
		
		//reg->Load(L"ExtendFonts", isExtendFonts);
		bool isExtendFonts;
		//reg->Load(L"ExtendFontNormalIdx", nFontNormalColor);
		char nFontNormalColor; // 0..15
		//reg->Load(L"ExtendFontBoldIdx", nFontBoldColor);
		char nFontBoldColor;   // 0..15
		//reg->Load(L"ExtendFontItalicIdx", nFontItalicColor);
		char nFontItalicColor; // 0..15
		
		//reg->Load(L"TrueColorerSupport", isTrueColorer);
		bool isTrueColorer;
		

		/* *** Background image *** */
		//reg->Load(L"BackGround Image show", isShowBgImage);
		char isShowBgImage;
		//reg->Load(L"BackGround Image", sBgImage, countof(sBgImage));		
		WCHAR sBgImage[MAX_PATH];
		//reg->Load(L"bgImageDarker", bgImageDarker);
		u8 bgImageDarker;		
		//reg->Load(L"bgImageColors", nBgImageColors);
		DWORD nBgImageColors;
		//reg->Load(L"bgOperation", bgOperation);
		char bgOperation; // BackgroundOp {eUpLeft = 0, eStretch = 1, eTile = 2}
		//reg->Load(L"bgPluginAllowed", isBgPluginAllowed);
		char isBgPluginAllowed;
		
		
		//bool isBackgroundImageValid;
		
		

		/* *** Transparency *** */
		//reg->Load(L"AlphaValue", nTransparent);
		u8 nTransparent;
		//reg->Load(L"UserScreenTransparent", isUserScreenTransparent);
		bool isUserScreenTransparent;

		/* *** Command Line History (from start dialog) *** */
		//reg->Load(L"CmdLineHistory", &psCmdHistory);
		LPWSTR psCmdHistory;
		//nCmdHistorySize = 0; HistoryCheck();
		DWORD nCmdHistorySize;

		/* *** Command Line (Registry) *** */
		//reg->Load(L"CmdLine", &psCmd);
		LPTSTR psCmd;
		
		/* Command Line ("/cmd" arg) */
		LPTSTR psCurCmd;

		/* 'Default' command line (if nor Registry, nor /cmd specified) */
		//WCHAR  szDefCmd[16];
	public:
		/* "Active" command line */
		LPCTSTR GetCmd();
		/* "Default" command line "far/cmd", based on /BufferHeight switch */
		//LPCTSTR GetDefaultCmd();
		///* OUR(!) startup info */
		//STARTUPINFOW ourSI;
		/* If Attach to PID requested */
		//DWORD nAttachPID; HWND hAttachConWnd;

		//reg->Load(L"FontName", inFont, countof(inFont))
		wchar_t inFont[LF_FACESIZE];
		//reg->Load(L"FontName2", inFont2, countof(inFont2))
		wchar_t inFont2[LF_FACESIZE];
		//reg->Load(L"FontBold", isBold);
		bool isBold;
		//reg->Load(L"FontItalic", isItalic);
		bool isItalic;
		//reg->Load(L"Anti-aliasing", Quality);
		DWORD mn_AntiAlias; //загружался как Quality
		//reg->Load(L"FontCharSet", mn_LoadFontCharSet); mb_CharSetWasSet = FALSE;
		BYTE mn_LoadFontCharSet; // То что загружено изначально (или уже сохранено в реестр)
		//reg->Load(L"FontCharSet", mn_LoadFontCharSet); mb_CharSetWasSet = FALSE;
		BOOL mb_CharSetWasSet;
		//reg->Load(L"FontSize", FontSizeY);
		DWORD FontSizeY;  // высота основного шрифта (загруженная из настроек!)
		//reg->Load(L"FontSizeX", FontSizeX);
		DWORD FontSizeX;  // ширина основного шрифта
		//reg->Load(L"FontSizeX2", FontSizeX2);
		DWORD FontSizeX2; // ширина для FixFarBorders (ширина создаваемого шрифта для отрисовки рамок, не путать со знакоместом)
		//reg->Load(L"FontSizeX3", FontSizeX3);
		DWORD FontSizeX3; // ширина знакоместа при моноширном режиме (не путать с FontSizeX2)
		
		//bool isFullScreen;
		//reg->Load(L"HideCaption", isHideCaption);
		bool isHideCaption;
		protected:
		//reg->Load(L"HideCaptionAlways", mb_HideCaptionAlways);
		bool mb_HideCaptionAlways;
		public:
		bool isHideCaptionAlways(); //<<mb_HideCaptionAlways
		//reg->Load(L"HideCaptionAlwaysFrame", nHideCaptionAlwaysFrame);
		BYTE nHideCaptionAlwaysFrame;
		//reg->Load(L"HideCaptionAlwaysDelay", nHideCaptionAlwaysDelay);
		DWORD nHideCaptionAlwaysDelay;
		//reg->Load(L"HideCaptionAlwaysDisappear", nHideCaptionAlwaysDisappear);
		DWORD nHideCaptionAlwaysDisappear;
		//reg->Load(L"DownShowHiddenMessage", isDownShowHiddenMessage);
		bool isDownShowHiddenMessage;
		//reg->Load(L"AlwaysOnTop", isAlwaysOnTop);
		bool isAlwaysOnTop;
		//reg->Load(L"DesktopMode", isDesktopMode);
		bool isDesktopMode;
		//reg->Load(L"FixFarBorders", isFixFarBorders)
		BYTE isFixFarBorders;
		//reg->Load(L"ExtendUCharMap", isExtendUCharMap);
		bool isExtendUCharMap;
		//reg->Load(L"DisableMouse", isDisableMouse);
		bool isDisableMouse;
		//reg->Load(L"MouseSkipActivation", isMouseSkipActivation);
		bool isMouseSkipActivation;
		//reg->Load(L"MouseSkipMoving", isMouseSkipMoving);
		bool isMouseSkipMoving;
		//reg->Load(L"FarHourglass", isFarHourglass);
		bool isFarHourglass;
		//reg->Load(L"FarHourglassDelay", nFarHourglassDelay);
		DWORD nFarHourglassDelay;
		//reg->Load(L"DisableFarFlashing", isDisableFarFlashing); if (isDisableFarFlashing>2) isDisableFarFlashing = 2;
		BYTE isDisableFarFlashing;
		//reg->Load(L"DisableAllFlashing", isDisableAllFlashing); if (isDisableAllFlashing>2) isDisableAllFlashing = 2;
		BYTE isDisableAllFlashing;
		/* *** Text selection *** */
		//reg->Load(L"ConsoleTextSelection", isConsoleTextSelection);
		BYTE isConsoleTextSelection;
		//reg->Load(L"CTS.SelectBlock", isCTSSelectBlock);
		bool isCTSSelectBlock;
		//reg->Load(L"CTS.SelectText", isCTSSelectText);
		bool isCTSSelectText;
		//reg->Load(L"CTS.VkBlock", isCTSVkBlock);
		BYTE isCTSVkBlock; // модификатор запуска выделения мышкой
		//reg->Load(L"CTS.VkText", isCTSVkText);
		BYTE isCTSVkText; // модификатор запуска выделения мышкой
		//reg->Load(L"CTS.ActMode", isCTSActMode);
		BYTE isCTSActMode; // режим и модификатор разрешения действий правой и средней кнопки мышки
		//reg->Load(L"CTS.VkAct", isCTSVkAct);
		BYTE isCTSVkAct; // режим и модификатор разрешения действий правой и средней кнопки мышки
		//reg->Load(L"CTS.RBtnAction", isCTSRBtnAction);
		BYTE isCTSRBtnAction; // 0-off, 1-copy, 2-paste
		//reg->Load(L"CTS.MBtnAction", isCTSMBtnAction);
		BYTE isCTSMBtnAction; // 0-off, 1-copy, 2-paste
		//reg->Load(L"CTS.ColorIndex", isCTSColorIndex);
		BYTE isCTSColorIndex;
		//reg->Load(L"FarGotoEditor", isFarGotoEditor);
		bool isFarGotoEditor; // Подсвечивать и переходить на файл/строку (ошибки компилятора)
		//reg->Load(L"FarGotoEditorVk", isFarGotoEditorVk);
		BYTE isFarGotoEditorVk; // Клавиша-модификатор для isFarGotoEditor
		
		bool isModifierPressed(DWORD vk);
		//bool isSelectionModifierPressed();
		//typedef struct tag_CharRanges
		//{
		//	bool bUsed;
		//	wchar_t cBegin, cEnd;
		//} CharRanges;
		//wchar_t mszCharRanges[120];
		//CharRanges icFixFarBorderRanges[10];
		
		// !!! Зовется из настроек Init/Load... !!!
		int ParseCharRanges(LPCWSTR asRanges, BYTE (&Chars)[0x10000], BYTE abValue = TRUE); // например, L"2013-25C3,25C4"
		wchar_t* CreateCharRanges(BYTE (&Chars)[0x10000]); // caller must free(result)
		BYTE mpc_FixFarBorderValues[0x10000];
		
		//reg->Load(L"KeyboardHooks", m_isKeyboardHooks); if (m_isKeyboardHooks>2) m_isKeyboardHooks = 0;
		BYTE m_isKeyboardHooks;
	public:
		bool isKeyboardHooks();
		
		bool isCharBorder(wchar_t inChar);
		
		//reg->Load(L"PartBrush75", isPartBrush75); if (isPartBrush75<5) isPartBrush75=5; else if (isPartBrush75>250) isPartBrush75=250;
		BYTE isPartBrush75;
		//reg->Load(L"PartBrush50", isPartBrush50); if (isPartBrush50<5) isPartBrush50=5; else if (isPartBrush50>250) isPartBrush50=250;
		BYTE isPartBrush50;
		//reg->Load(L"PartBrush25", isPartBrush25); if (isPartBrush25<5) isPartBrush25=5; else if (isPartBrush25>250) isPartBrush25=250;
		BYTE isPartBrush25;
		//reg->Load(L"PartBrushBlack", isPartBrushBlack);
		BYTE isPartBrushBlack;
		
		//reg->Load(L"CursorType", isCursorV);
		bool isCursorV;
		//reg->Load(L"CursorBlink", isCursorBlink);
		bool isCursorBlink;
		//reg->Load(L"CursorColor", isCursorColor);
		bool isCursorColor;
		//reg->Load(L"CursorBlockInactive", isCursorBlockInactive);
		bool isCursorBlockInactive;
		
		//reg->Load(L"RightClick opens context menu", isRClickSendKey);
		char isRClickSendKey;
		//reg->Load(L"RightClickMacro2", &sRClickMacro);
		wchar_t *sRClickMacro;
		
		//reg->Load(L"SafeFarClose", isSafeFarClose);
		bool isSafeFarClose;
		//reg->Load(L"SafeFarCloseMacro", &sSafeFarCloseMacro);
		wchar_t *sSafeFarCloseMacro;
		
		//reg->Load(L"AltEnter", isSendAltEnter);
		bool isSendAltEnter;
		//reg->Load(L"AltSpace", isSendAltSpace);
		bool isSendAltSpace;
		//reg->Load(L"SendAltTab", isSendAltTab);
		bool isSendAltTab;
		//reg->Load(L"SendAltEsc", isSendAltEsc);
		bool isSendAltEsc;
		//reg->Load(L"SendAltPrintScrn", isSendAltPrintScrn);
		bool isSendAltPrintScrn;
		//reg->Load(L"SendPrintScrn", isSendPrintScrn);
		bool isSendPrintScrn;
		//reg->Load(L"SendCtrlEsc", isSendCtrlEsc);
		bool isSendCtrlEsc;
		
		//reg->Load(L"Min2Tray", isMinToTray);
		bool isMinToTray;
		//reg->Load(L"AlwaysShowTrayIcon", isAlwaysShowTrayIcon);
		bool isAlwaysShowTrayIcon;
		//bool isForceMonospace, isProportional;
		//reg->Load(L"Monospace", isMonospace)
		BYTE isMonospace; // 0 - proportional, 1 - monospace, 2 - forcemonospace
		//bool isUpdConHandle;
		//reg->Load(L"RSelectionFix", isRSelFix);
		bool isRSelFix;
		
		/* *** Drag *** */
		//reg->Load(L"Dnd", isDragEnabled);
		BYTE isDragEnabled;
		//reg->Load(L"DndDrop", isDropEnabled);
		BYTE isDropEnabled;
		//reg->Load(L"DndLKey", nLDragKey);
		BYTE nLDragKey;
		//reg->Load(L"DndRKey", nRDragKey);
		BYTE nRDragKey; // Был DWORD
		//reg->Load(L"DefCopy", isDefCopy);
		bool isDefCopy;
		//reg->Load(L"DragOverlay", isDragOverlay);
		BYTE isDragOverlay;
		//reg->Load(L"DragShowIcons", isDragShowIcons);
		bool isDragShowIcons;
		//reg->Load(L"DragPanel", isDragPanel); if (isDragPanel > 2) isDragPanel = 1;
		BYTE isDragPanel; // изменение размера панелей мышкой
		
		//reg->Load(L"DebugSteps", isDebugSteps);
		bool isDebugSteps;
		
		//reg->Load(L"EnhanceGraphics", isEnhanceGraphics);
		bool isEnhanceGraphics; // Progressbars and scrollbars (pseudographics)
		
		//reg->Load(L"FadeInactive", isFadeInactive);
		bool isFadeInactive;
		protected:
		//reg->Load(L"FadeInactiveLow", mn_FadeLow);
		BYTE mn_FadeLow;
		//reg->Load(L"FadeInactiveHigh", mn_FadeHigh);		
		BYTE mn_FadeHigh;
		//mn_LastFadeSrc = mn_LastFadeDst = -1;
		COLORREF mn_LastFadeSrc;
		//mn_LastFadeSrc = mn_LastFadeDst = -1;		
		COLORREF mn_LastFadeDst;
		public:
		
		//reg->Load(L"Tabs", isTabs);
		char isTabs;
		//reg->Load(L"TabSelf", isTabSelf);
		bool isTabSelf;
		//reg->Load(L"TabRecent", isTabRecent);
		bool isTabRecent;
		//reg->Load(L"TabLazy", isTabLazy);
		bool isTabLazy;
		
		//TODO:
		bool isTabsInCaption;

		// Tab theme properties
		int ilDragHeight;

		protected:
		//reg->Load(L"TabsOnTaskBar", m_isTabsOnTaskBar);
		char m_isTabsOnTaskBar;
		public:
		bool isTabsOnTaskBar();
		
		//reg->Load(L"TabFontFace", sTabFontFace, countof(sTabFontFace));
		wchar_t sTabFontFace[LF_FACESIZE];
		//reg->Load(L"TabFontCharSet", nTabFontCharSet);
		DWORD nTabFontCharSet;
		//reg->Load(L"TabFontHeight", nTabFontHeight);
		int nTabFontHeight;
		
		//if (!reg->Load(L"TabCloseMacro", &sTabCloseMacro) || (sTabCloseMacro && !*sTabCloseMacro)) { if (sTabCloseMacro) { free(sTabCloseMacro); sTabCloseMacro = NULL; } }
		wchar_t *sTabCloseMacro;
		//if (!reg->Load(L"SaveAllEditors", &sSaveAllMacro)) { sSaveAllMacro = lstrdup(L"...
		wchar_t *sSaveAllMacro;
		
		//reg->Load(L"ToolbarAddSpace", nToolbarAddSpace);
		int nToolbarAddSpace;
		//reg->Load(L"ConWnd Width", wndWidth);
		DWORD wndWidth;
		//reg->Load(L"ConWnd Height", wndHeight);
		DWORD wndHeight;
		//reg->Load(L"16bit Height", ntvdmHeight);
		DWORD ntvdmHeight; // в символах
		//reg->Load(L"ConWnd X", wndX);
		int wndX; // в пикселях
		//reg->Load(L"ConWnd Y", wndY);
		int wndY; // в пикселях
		//reg->Load(L"WindowMode", WindowMode); if (WindowMode!=rFullScreen && WindowMode!=rMaximized && WindowMode!=rNormal) WindowMode = rNormal;
		DWORD WindowMode;
		//reg->Load(L"Cascaded", wndCascade);
		bool wndCascade;
		//reg->Load(L"AutoSaveSizePos", isAutoSaveSizePos);
		bool isAutoSaveSizePos;
	private:
		// При закрытии окна крестиком - сохранять только один раз,
		// а то размер может в процессе закрытия консолей измениться
		bool mb_SizePosAutoSaved;
	public:
		//reg->Load(L"SlideShowElapse", nSlideShowElapse);
		DWORD nSlideShowElapse;
		//reg->Load(L"IconID", nIconID);
		DWORD nIconID;
		//reg->Load(L"TryToCenter", isTryToCenter);
		bool isTryToCenter;
		//reg->Load(L"ShowScrollbar", isAlwaysShowScrollbar); if (isAlwaysShowScrollbar > 2) isAlwaysShowScrollbar = 2;
		BYTE isAlwaysShowScrollbar;
		
		//reg->Load(L"TabMargins", rcTabMargins);
		RECT rcTabMargins;
		//reg->Load(L"TabFrame", isTabFrame);
		bool isTabFrame;
		
		//reg->Load(L"MinimizeRestore", icMinimizeRestore);
		BYTE icMinimizeRestore;
		//reg->Load(L"Multi", isMulti);
		bool isMulti;
		private:
		//reg->Load(L"Multi.Modifier", nMultiHotkeyModifier); TestHostkeyModifiers();
		DWORD nMultiHotkeyModifier;
		public:
		//reg->Load(L"Multi.NewConsole", icMultiNew);
		BYTE icMultiNew;
		//reg->Load(L"Multi.Next", icMultiNext);
		BYTE icMultiNext;
		//reg->Load(L"Multi.Recreate", icMultiRecreate);
		BYTE icMultiRecreate;
		//reg->Load(L"Multi.Buffer", icMultiBuffer);
		BYTE icMultiBuffer;
		//reg->Load(L"Multi.Close", icMultiClose);
		BYTE icMultiClose;
		//reg->Load(L"Multi.CmdKey", icMultiCmd);
		BYTE icMultiCmd;
		//reg->Load(L"Multi.AutoCreate", isMultiAutoCreate);
		bool isMultiAutoCreate;
		//reg->Load(L"Multi.LeaveOnClose", isMultiLeaveOnClose);
		bool isMultiLeaveOnClose;
		//reg->Load(L"Multi.Iterate", isMultiIterate);
		bool isMultiIterate;
		//reg->Load(L"Multi.NewConfirm", isMultiNewConfirm);
		bool isMultiNewConfirm;
		//reg->Load(L"Multi.UseNumbers", isUseWinNumber);
		bool isUseWinNumber;
		//reg->Load(L"Multi.UseWinTab", isUseWinTab);
		bool isUseWinTab;
		//reg->Load(L"Multi.UseArrows", isUseWinArrows);
		bool isUseWinArrows;
		//reg->Load(L"FARuseASCIIsort", isFARuseASCIIsort);
		bool isFARuseASCIIsort;
		//reg->Load(L"FixAltOnAltTab", isFixAltOnAltTab);
		bool isFixAltOnAltTab;
		//reg->Load(L"ShellNoZoneCheck", isShellNoZoneCheck);
		bool isShellNoZoneCheck;
		
		bool IsHostkey(WORD vk);
		bool IsHostkeySingle(WORD vk);
		bool IsHostkeyPressed();
		WORD GetPressedHostkey();
		UINT GetHostKeyMod(); // набор флагов MOD_xxx для RegisterHotKey

		/* *** Заголовки табов *** */
		//reg->Load(L"TabConsole", szTabConsole, countof(szTabConsole));
		WCHAR szTabConsole[32];
		//reg->Load(L"TabEditor", szTabEditor, countof(szTabEditor));
		WCHAR szTabEditor[32];
		//reg->Load(L"TabEditorModified", szTabEditorModified, countof(szTabEditorModified));
		WCHAR szTabEditorModified[32];
		//reg->Load(L"TabViewer", szTabViewer, countof(szTabViewer));
		WCHAR szTabViewer[32];
		//reg->Load(L"TabLenMax", nTabLenMax); if (nTabLenMax < 10 || nTabLenMax >= CONEMUTABMAX) nTabLenMax = 20;
		DWORD nTabLenMax;

		//reg->Load(L"AdminTitleSuffix", szAdminTitleSuffix, countof(szAdminTitleSuffix)); szAdminTitleSuffix[countof(szAdminTitleSuffix)-1] = 0;
		wchar_t szAdminTitleSuffix[64]; //" (Admin)"
		//reg->Load(L"AdminShowShield", bAdminShield);
		bool bAdminShield;
		//reg->Load(L"HideInactiveConsoleTabs", bHideInactiveConsoleTabs);
		bool bHideInactiveConsoleTabs;

		TODO("загрузка bHideDisabledTabs");
		bool bHideDisabledTabs;
		
		//bool isVisualizer;
		//char nVizNormal, nVizFore, nVizTab, nVizEOL, nVizEOF;
		//wchar_t cVizTab, cVizEOL, cVizEOF;

		//char isAllowDetach;
		//bool isCreateAppWindow;
		bool NeedCreateAppWindow();
		/*bool isScrollTitle;
		DWORD ScrollTitleLen;*/
		
		//reg->Load(L"MainTimerElapse", nMainTimerElapse); if (nMainTimerElapse>1000) nMainTimerElapse = 1000;
		DWORD nMainTimerElapse; // периодичность, с которой из консоли считывается текст
		//reg->Load(L"MainTimerInactiveElapse", nMainTimerInactiveElapse); if (nMainTimerInactiveElapse>10000) nMainTimerInactiveElapse = 10000;
		DWORD nMainTimerInactiveElapse; // периодичность при неактивности
		
		//bool isAdvLangChange; // в Висте без ConIme в самой консоли не меняется язык, пока не послать WM_SETFOCUS. Но при этом исчезает диалог быстрого поиска
		
		//reg->Load(L"SkipFocusEvents", isSkipFocusEvents);
		bool isSkipFocusEvents;
		
		//bool isLangChangeWsPlugin;
		
		//reg->Load(L"MonitorConsoleLang", isMonitorConsoleLang);
		char isMonitorConsoleLang;
		
		//reg->Load(L"SleepInBackground", isSleepInBackground);
		bool isSleepInBackground;

		//reg->Load(L"AffinityMask", nAffinity);
		DWORD nAffinity;

		//reg->Load(L"UseInjects", isUseInjects);
		bool isUseInjects;
		//reg->Load(L"PortableReg", isPortableReg);
		bool isPortableReg;

		/* *** Debugging *** */
		//reg->Load(L"ConVisible", isConVisible);
		bool isConVisible;
		//reg->Load(L"ConInMode", nConInMode);
		DWORD nConInMode;
		
		//
		//enum GuiLoggingType m_RealConLoggingType;

		/* *** Thumbnails and Tiles *** */
		//reg->Load(L"PanView.BackColor", ThSet.crBackground.RawColor);
		//reg->Load(L"PanView.PFrame", ThSet.nPreviewFrame); if (ThSet.nPreviewFrame!=0 && ThSet.nPreviewFrame!=1) ThSet.nPreviewFrame = 1;
		//и т.п...
		PanelViewSetMapping ThSet;
		
		//MFileMapping<PanelViewSetMapping> m_ThSetMap;

		// Working variables...
	//private:
	//	//HBITMAP  hBgBitmap;
	//	//COORD    bgBmp;
	//	//HDC      hBgDc;
	//	CBackground* mp_Bg;
	//	//MSection mcs_BgImgData;
	//	BITMAPFILEHEADER* mp_BgImgData;
	//	BOOL mb_NeedBgUpdate; //, mb_WasVConBgImage;
	//	bool mb_BgLastFade;
	//	FILETIME ftBgModified;
	//	DWORD nBgModifiedTick;
	//public:
	//	bool PrepareBackground(HDC* phBgDc, COORD* pbgBmpSize);
	//	bool PollBackgroundFile(); // true, если файл изменен
	//	bool /*LoadImageFrom*/LoadBackgroundFile(TCHAR *inPath, bool abShowErrors=false);
	//	bool IsBackgroundEnabled(CVirtualConsole* apVCon);
	//	void NeedBackgroundUpdate();
	//	//CBackground* CreateBackgroundImage(const BITMAPFILEHEADER* apBkImgData);
	public:
		//HFONT   mh_Font[MAX_FONT_STYLES], mh_Font2;
		//TODO("По хорошему, CharWidth & CharABC нужно разделять по шрифтам - у Bold ширина может быть больше");
		//WORD    CharWidth[0x10000]; //, Font2Width[0x10000];
		//ABC     CharABC[0x10000];
		//HWND hMain, hExt, hKeys, hTabs, hColors, hViews, hInfo, hDebug;
		////static void CenterDialog(HWND hWnd2);
		//void OnClose();
		//static INT_PTR CALLBACK wndOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		//static INT_PTR CALLBACK mainOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		//static INT_PTR CALLBACK extOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		//static INT_PTR CALLBACK keysOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		//static INT_PTR CALLBACK tabsOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		//static INT_PTR CALLBACK colorOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		//static INT_PTR CALLBACK viewsOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		//static INT_PTR CALLBACK infoOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		//static INT_PTR CALLBACK debugOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		//static INT_PTR CALLBACK hideOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		////static INT_PTR CALLBACK multiOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		//static INT_PTR CALLBACK selectionOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		void LoadSettings();
		void InitSettings();
		BOOL SaveSettings(BOOL abSilent = FALSE);
		void SaveSizePosOnExit();
		void SaveConsoleFont();
		//bool ShowColorDialog(HWND HWndOwner, COLORREF *inColor);
		//static int CALLBACK EnumFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount);
		//static int CALLBACK EnumFontCallBackEx(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam);
		void UpdateMargins(RECT arcMargins);
		//static void Dialog();
		//void UpdatePos(int x, int y);
		//void UpdateSize(UINT w, UINT h);
		//void UpdateTTF(BOOL bNewTTF);
		//void UpdateFontInfo();
		//void Performance(UINT nID, BOOL bEnd);
		//void InitFont(LPCWSTR asFontName=NULL, int anFontHeight=-1, int anQuality=-1);
		//BOOL RegisterFont(LPCWSTR asFontFile, BOOL abDefault);
		//void RegisterFonts();
	//private:
	//	void RegisterFontsInt(LPCWSTR asFromDir);
	public:
		//void UnregisterFonts();
		//BOOL GetFontNameFromFile(LPCTSTR lpszFilePath, wchar_t (&rsFontName)[LF_FACESIZE], wchar_t (&rsFullFontName)[LF_FACESIZE]);
		//BOOL GetFontNameFromFile_TTF(LPCTSTR lpszFilePath, wchar_t (&rsFontName)[LF_FACESIZE], wchar_t (&rsFullFontName)[LF_FACESIZE]);
		//BOOL GetFontNameFromFile_OTF(LPCTSTR lpszFilePath, wchar_t (&rsFontName)[LF_FACESIZE], wchar_t (&rsFullFontName)[LF_FACESIZE]);
		void HistoryCheck();
		void HistoryAdd(LPCWSTR asCmd);
		LPCWSTR HistoryGet();
		//void UpdateConsoleMode(DWORD nMode);
		BOOL CheckConIme();
		void CheckConsoleSettings();
		
		SettingsBase* CreateSettings();
		
		//bool AutoRecreateFont(int nFontW, int nFontH);
		//bool MacroFontSetSize(int nRelative/*0/1*/, int nValue/*1,2,...*/);
		//void MacroFontSetName(LPCWSTR pszFontName, WORD anHeight /*= 0*/, WORD anWidth /*= 0*/);
		//bool CheckTheming();
		//void OnPanelViewAppeared(BOOL abAppear);
		//bool EditConsoleFont(HWND hParent);
		//static INT_PTR CALLBACK EditConsoleFontProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		//static int CALLBACK EnumConFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount);
		//bool CheckConsoleFontFast();
		//enum
		//{
		//	ConFontErr_NonSystem   = 0x01,
		//	ConFontErr_NonRegistry = 0x02,
		//	ConFontErr_InvalidName = 0x04,
		//};
	//protected:
	//	BOOL bShowConFontError, bConsoleFontChecked;
	//	wchar_t sConFontError[512];
	//	wchar_t sDefaultConFontName[32]; // "последний шанс", если юзер отказался выбрать нормальный шрифт
	//	HWND hConFontDlg;
	//	DWORD nConFontError; // 0x01 - шрифт не зарегистрирован в системе, 0x02 - не указан в реестре для консоли
	//	HWND hwndConFontBalloon;
	//	static bool CheckConsoleFontRegistry(LPCWSTR asFaceName);
	//	static bool CheckConsoleFont(HWND ahDlg);
	//	static void ShowConFontErrorTip(LPCTSTR asInfo);
	//	LPCWSTR CreateConFontError(LPCWSTR asReqFont=NULL, LPCWSTR asGotFont=NULL);
	//	TOOLINFO tiConFontBalloon;
	//private:
	//	static void ShowErrorTip(LPCTSTR asInfo, HWND hDlg, int nCtrlID, wchar_t* pszBuffer, int nBufferSize, HWND hBall, TOOLINFO *pti, HWND hTip, DWORD nTimeout);
	//protected:
	//	void OnResetOrReload(BOOL abResetSettings);
	//	LRESULT OnInitDialog();
	//	LRESULT OnInitDialog_Main(HWND hWnd2);
	//	LRESULT OnInitDialog_Ext(HWND hWnd2);
	//	LRESULT OnInitDialog_Keys(HWND hWnd2);
	//	LRESULT OnInitDialog_Tabs(HWND hWnd2);
	//	LRESULT OnInitDialog_Color(HWND hWnd2);
	//	LRESULT OnInitDialog_Views(HWND hWnd2);
	//	LRESULT OnInitDialog_ViewsFonts(HWND hWnd2);
	//	LRESULT OnInitDialog_Info(HWND hWnd2);
	//	LRESULT OnInitDialog_Debug(HWND hWnd2);
	//	LRESULT OnButtonClicked(HWND hWnd2, WPARAM wParam, LPARAM lParam);
	//	LRESULT OnColorButtonClicked(HWND hWnd2, WPARAM wParam, LPARAM lParam);
	//	LRESULT OnColorComboBox(HWND hWnd2, WPARAM wParam, LPARAM lParam);		
	//	LRESULT OnColorEditChanged(HWND hWnd2, WPARAM wParam, LPARAM lParam);				
	//	LRESULT OnEditChanged(HWND hWnd2, WPARAM wParam, LPARAM lParam);				
	//	LRESULT OnComboBox(HWND hWnd2, WPARAM wParam, LPARAM lParam);		
	//	LRESULT OnTab(LPNMHDR phdr);
	//	INT_PTR OnMeasureFontItem(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
	//	INT_PTR OnDrawFontItem(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
	//	void OnSaveActivityLogFile(HWND hListView);
	//	void FillHotKeysList();
	//	UINT mn_ActivateTabMsg;
	private:
		//bool GetColorById(WORD nID, COLORREF* color);
		//bool SetColorById(WORD nID, COLORREF color);
		//void ColorSetEdit(HWND hWnd2, WORD c);
		//bool ColorEditDialog(HWND hWnd2, WORD c);
		//void FillBgImageColors();
		//HBRUSH mh_CtlColorBrush;
		//INT_PTR ColorCtlStatic(HWND hWnd2, WORD c, HWND hItem);
		//COLORREF acrCustClr[16]; // array of custom colors
		//BOOL mb_IgnoreEditChanged, mb_IgnoreTtfChange, mb_CharSetWasSet;
		//i64 mn_Freq;
		//i64 mn_FPS[20]; int mn_FPS_CUR_FRAME;
		//i64 mn_RFPS[20]; int mn_RFPS_CUR_FRAME;
		//i64 mn_Counter[tPerfInterval-gbPerformance];
		//i64 mn_CounterMax[tPerfInterval-gbPerformance];
		//DWORD mn_CounterTick[tPerfInterval-gbPerformance];
		//HWND hwndTip, hwndBalloon;
		//static void ShowFontErrorTip(LPCTSTR asInfo);
		//TOOLINFO tiBalloon;
		//void RegisterTipsFor(HWND hChildDlg);
		//HFONT CreateFontIndirectMy(LOGFONT *inFont);
		//void RecreateFont(WORD wFromID);
		//// Theming
		//HMODULE mh_Uxtheme;
		//typedef HRESULT(STDAPICALLTYPE *SetWindowThemeT)(HWND hwnd,LPCWSTR pszSubAppName,LPCWSTR pszSubIdList);
		//SetWindowThemeT SetWindowThemeF;
		//typedef HRESULT(STDAPICALLTYPE *EnableThemeDialogTextureT)(HWND hwnd,DWORD dwFlags);
		//EnableThemeDialogTextureT EnableThemeDialogTextureF;
		//UINT mn_MsgUpdateCounter;
		////wchar_t temp[MAX_PATH];
		//UINT mn_MsgRecreateFont;
		//UINT mn_MsgLoadFontFromMain;
		//static int IsChecked(HWND hParent, WORD nCtrlId);
		//static int GetNumber(HWND hParent, WORD nCtrlId);
		//static int SelectString(HWND hParent, WORD nCtrlId, LPCWSTR asText);
		//static int SelectStringExact(HWND hParent, WORD nCtrlId, LPCWSTR asText);
		//BOOL mb_TabHotKeyRegistered;
		//void RegisterTabs();
		//void UnregisterTabs();
		//static DWORD CALLBACK EnumFontsThread(LPVOID apArg);
		//HANDLE mh_EnumThread;
		//WORD mn_LastChangingFontCtrlId;
		//// Временно регистрируемые шрифты
		//typedef struct tag_RegFont
		//{
		//	BOOL    bDefault;             // Этот шрифт пользователь указал через /fontfile
		//	wchar_t szFontFile[MAX_PATH]; // полный путь
		//	wchar_t szFontName[32];       // Font Family
		//	BOOL    bUnicode;             // Юникодный?
		//	BOOL    bHasBorders;          // Имеет ли данный шрифт символы рамок
		//	BOOL    bAlreadyInSystem;     // Шрифт с таким именем уже был зарегистрирован в системе
		//} RegFont;
		//std::vector<RegFont> m_RegFonts;
		//BOOL mb_StopRegisterFonts;
		
		// reg->Load(L"ColorTableNN", Colors[i]);
		COLORREF Colors[0x20];
		
		bool mb_FadeInitialized;
		
		DWORD mn_FadeMul;
		COLORREF ColorsFade[0x20];
		//BOOL GetColorRef(HWND hDlg, WORD TB, COLORREF* pCR);
		inline BYTE GetFadeColorItem(BYTE c);
		
		//
		//bool mb_ThemingEnabled;
		//
		bool TestHostkeyModifiers();
		static BYTE CheckHostkeyModifier(BYTE vk);
		static void ReplaceHostkey(BYTE vk, BYTE vkNew);
		static void AddHostkey(BYTE vk);
		static void TrimHostkeys();
		//static void SetupHotkeyChecks(HWND hWnd2);
		static bool MakeHostkeyModifier();
		static BYTE HostkeyCtrlId2Vk(WORD nID);
		BYTE mn_HostModOk[15], mn_HostModSkip[15];
		bool isHostkeySingleLR(WORD vk, WORD vkC, WORD vkL, WORD vkR);
		//static void FillListBoxItems(HWND hList, uint nItems, const WCHAR** pszItems, const DWORD* pnValues, DWORD& nValue);
		//static void GetListBoxItem(HWND hList, uint nItems, const WCHAR** pszItems, const DWORD* pnValues, DWORD& nValue);
		//static void CenterMoreDlg(HWND hWnd2);
		//static bool IsAlmostMonospace(LPCWSTR asFaceName, int tmMaxCharWidth, int tmAveCharWidth, int tmHeight);
	private:
		//struct ConEmuHotKeys
		//{
		//	int  DescrLangID;
		//	
		//	int  Type; // 0 - hotkey, 1 - modifier (для драга, например)
		//	
		//	// User
		//	BYTE* VkPtr; // Если NULL - значит системный, не изменяемый
		//	
		//	// System or default
		//	BYTE Vk;
		//	DWORD Modifier; // System only, для "User" - используется "HostKey"
		//	
		//	TODO("Сюда можно бы еще добавить инфу на какой странице и как его настраивать");
		//};
		//#define MAKEMODIFIER2(vk1,vk2) ((DWORD)vk1&0xFF)|(((DWORD)vk2&0xFF)<<8)
		//#define MAKEMODIFIER3(vk1,vk2,vk3) ((DWORD)vk1&0xFF)|(((DWORD)vk2&0xFF)<<8)|(((DWORD)vk3&0xFF)<<16)
		//ConEmuHotKeys *m_HotKeys;
		//enum KeyListColumns
		//{
		//	klc_Type = 0,
		//	klc_Hotkey,
		//	klc_Desc
		//};
		//enum LogProcessColumns
		//{
		//	lpc_Time = 0,
		//	lpc_PPID,
		//	lpc_Func,
		//	lpc_Oper,
		//	lpc_Bits,
		//	lpc_System,
		//	lpc_App,
		//	lpc_Params,
		//	lpc_Flags,
		//	lpc_StdIn,
		//	lpc_StdOut,
		//	lpc_StdErr,
		//};
		//enum LogInputColumns
		//{
		//	lic_Time = 0,
		//	lic_Type,
		//	lic_Dup,
		//	lic_Event,
		//};
		//static void GetVkKeyName(BYTE vk, wchar_t (&szName)[128]);
	private:
		struct CEFontRange
		{
		public:
			bool    bUsed;              // Для быстрого включения/отключения
			wchar_t sTitle[64];         // "Title"="Borders and scrollbars"
			wchar_t sRange[1024];       // "CharRange"="2013-25C4;"
			wchar_t sFace[LF_FACESIZE]; // "FontFace"="Andale Mono"
			LONG    nHeight;            // "Height"=dword:00000012
			LONG    nWidth;             // "Width"=dword:00000000
			LONG    nLoadHeight;        // Для корректного вычисления относительных размеров шрифта при
			LONG    nLoadWidth;         // ресайзе GUI, или изменения из макроса - запомнить исходные
			BYTE    nCharset;           // "Charset"=hex:00
			bool    bBold;              // "Bold"=hex:00
			bool    bItalic;            // "Italic"=hex:00
			BYTE    nQuality;           // "Anti-aliasing"=hex:03
			/*    */
		private:
			LOGFONT LF;
		public:
			LOGFONT& LogFont()
			{
				LF.lfHeight = nHeight;
				LF.lfWidth = nWidth;
				LF.lfEscapement = 0;
				LF.lfOrientation = 0;
				LF.lfWeight = bBold?FW_BOLD:FW_NORMAL;
				LF.lfItalic = bItalic ? 1 : 0;
				LF.lfUnderline = 0;
				LF.lfStrikeOut = 0;
				LF.lfQuality = nQuality;
				LF.lfCharSet = nCharset;
				LF.lfOutPrecision = OUT_TT_PRECIS;
				LF.lfClipPrecision = CLIP_DEFAULT_PRECIS;
				LF.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
				lstrcpyn(LF.lfFaceName, sFace, countof(LF.lfFaceName));
				return LF;
			};
			void Scale(LONG anMainHeight, LONG anMainLoadHeight)
			{
				// Высота
				if (nLoadHeight == 0 || !anMainLoadHeight)
				{
					nHeight = anMainHeight; // Высота равна высоте в Main
				}
				else
				{
					nHeight = nLoadHeight * anMainHeight / anMainLoadHeight;
				}
				// Ширина
				if (nLoadWidth == 0 || !anMainLoadHeight)
				{
					nWidth = 0;
				}
				else
				{
					nWidth = nLoadWidth * anMainHeight / anMainLoadHeight;
				}
			};
			/*    */
			HFONT hFonts[MAX_FONT_STYLES]; //normal/(bold|italic|underline)
			/*    */
			BYTE RangeData[0x10000];
		};
		CEFontRange m_Fonts[MAX_FONT_GROUPS]; // 0-Main, 1-Borders, 2 и более - user defined
		BOOL FontRangeLoad(SettingsBase* reg, int Idx);
		BOOL FontRangeSave(SettingsBase* reg, int Idx);
};

#include "OptionsClass.h"
