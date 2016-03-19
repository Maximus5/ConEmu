
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

#include "../common/MArray.h"
#include "../common/MFileMapping.h"
#include <commctrl.h>

#include "CustomFonts.h"
#include "DpiAware.h"
#include "FontMgr.h"
#include "HotkeyList.h"
#include "Options.h"
#include "SetDlgButtons.h"
#include "SetDlgColors.h"
#include "SetDlgFonts.h"

class CBackground;
class CBackgroundInfo;
class CDpiForDialog;
class CDynDialog;
class CImgButtons;
struct CEHelpPopup;
struct DebugLogShellActivity;
struct DpiValue;

enum SingleInstanceArgEnum
{
	sgl_Default  = 0,
	sgl_Enabled  = 1,
	sgl_Disabled = 2,
};

enum PerformanceCounterTypes
{
	tPerfFPS = 0,
	tPerfData,
	tPerfRender,
	tPerfBlt,
	tPerfInterval,
	tPerfKeyboard,
	tPerfLast
};

typedef DWORD EvalSizeFlags;
const EvalSizeFlags
	// May be used in combination
	esf_CanUseDpi    = 0x0010, // Take into account _dpi
	esf_CanUseZoom   = 0x0020, // Take into account mn_FontZoomValue
	esf_CanUseUnits  = 0x0040, // Make the result negative IF gpSet->FontUseUnits is ON
	// Must not be used together
	esf_Vertical     = 0x0001, // Used to get lfHeight
	esf_Horizontal   = 0x0000  // Used to get lfWidth
;

class CSettings
	: public CSetDlgButtons
	, public CSetDlgColors
	, public CSetDlgFonts
{
	public:
		CSettings();
		virtual ~CSettings();
	private:
		Settings m_Settings;
		void ReleaseHandles();
		void ReleaseHotkeys();
		void InitVars_Hotkeys();
		void InitVars_Pages();
	private:
		friend class CSetDlgButtons;
		friend class CSetDlgColors;
		friend class CSetDlgFonts;
		friend class CFontMgr;
	public:

		private:
			wchar_t ConfigPath[MAX_PATH];
			wchar_t ConfigName[MAX_CONFIG_NAME];
		public:
		LPCWSTR GetConfigPath();
		LPCWSTR GetConfigName();
		void SetConfigName(LPCWSTR asConfigName);

		// === Аргументы из командной строки ===
		// Для отладочных целей.
		bool isResetBasicSettings;
		bool isFastSetupDisabled;
		bool isDontCascade;
		// === Запрет сохранения опций при выходе ===
		bool ibDisableSaveSettingsOnExit;

	public:

		bool IsMulti();
		RecreateActionParm GetDefaultCreateAction();

		SingleInstanceArgEnum SingleInstanceArg; // по умолчанию = sgl_Default, но для Quake переключается на = sgl_Enabled
		bool IsSingleInstanceArg();
		SingleInstanceShowHideType SingleInstanceShowHide; // по умолчанию = sih_None
		bool ResetCmdHistory(HWND hParent = NULL);
		void SetSaveCmdHistory(bool bSaveHistory);

		//int DefaultBufferHeight;
		bool bForceBufferHeight; int nForceBufferHeight;

		#ifdef SHOW_AUTOSCROLL
		bool AutoScroll;
		#endif

		//bool AutoBufferHeight;
		//bool FarSyncSize;
		//int nCmdOutputCP;

		LONG EvalSize(LONG nSize, EvalSizeFlags Flags);

	public:
		char isAllowDetach;

		//
		enum GuiLoggingType m_ActivityLoggingType;
		DWORD mn_ActivityCmdStartTick;

		// Thumbnails and Tiles
		//PanelViewSetMapping ThSet;
		MFileMapping<PanelViewSetMapping> m_ThSetMap;

		// Working variables...
	private:
		#ifndef APPDISTINCTBACKGROUND
		CBackground* mp_Bg;
		BITMAPFILEHEADER* mp_BgImgData;
		BOOL mb_NeedBgUpdate; //, mb_WasVConBgImage;
		FILETIME ftBgModified;
		DWORD nBgModifiedTick;
		bool isBackgroundImageValid;
		bool mb_BgLastFade;
		#else
		CBackgroundInfo* mp_BgInfo;
		#endif
		CDpiForDialog* mp_DpiAware;
		CImgButtons* mp_ImgBtn;
	protected:
		enum ColorShowFormat { eRgbDec = 0, eRgbHex, eBgrHex } m_ColorFormat;
	public:
		void SetBgImageDarker(u8 newValue, bool bUpdate);
		#ifndef APPDISTINCTBACKGROUND
		bool PrepareBackground(CVirtualConsole* apVCon, HDC* phBgDc, COORD* pbgBmpSize);
		bool PollBackgroundFile(); // true, если файл изменен
		#else
		CBackgroundInfo* GetBackgroundObject();
		#endif
		bool LoadBackgroundFile(TCHAR *inPath, bool abShowErrors=false);
		bool IsBackgroundEnabled(CVirtualConsole* apVCon);
		void NeedBackgroundUpdate();
		//CBackground* CreateBackgroundImage(const BITMAPFILEHEADER* apBkImgData);
	public:

		//HWND hMain, hExt, hFar, hKeys, hTabs, hColors, hCmdTasks, hViews, hInfo, hDebug, hUpdate, hSelection;
		enum TabHwndIndex {
			thi_Main = 0,     // "Main"
			thi_SizePos,      //   "Size & Pos"
			thi_Show,         //   "Appearance"
			thi_Backgr,       //   "Background"
			thi_Confirm,      //   "Confirmations"
			thi_Taskbar,      //   "Task bar"
			thi_Update,       //   "Update"
			thi_Startup,      // "Startup"
			thi_Tasks,        //   "Tasks"
			thi_Comspec,      //   "ComSpec"
			thi_Environment,  //   "Environment"
			thi_Ext,          // "Features"
			thi_Cursor,       //   "Text cursor"
			thi_Colors,       //   "Colors"
			thi_Transparent,  //   "Transparency"
			thi_Tabs,         //   "Tabs"
			thi_Status,       //   "Status bar"
			thi_Apps,         //   "App distinct"
			thi_Integr,       // "Integration"
			thi_DefTerm,      //   "Default terminal"
			thi_Keys,         // "Keys & Macro"
			thi_KeybMouse,    //   "Controls"
			thi_MarkCopy,     //   "Mark & Copy"
			thi_Paste,        //   "Paste"
			thi_Hilight,      //   "Highlight"
			thi_Far,          // "Far Manager"
			thi_FarMacro,     //   "Far macros"
			thi_Views,        //   "Views"
			thi_Info,         // "Info"
			thi_Debug,        //   "Debug"
			//
			thi_Last
		};
		HWND GetActivePage();
		HWND GetPage(TabHwndIndex nPage);
		TabHwndIndex GetPageId(HWND hPage);

	private:
		// mh_Tabs moved to the m_Pages
		int  mn_LastActivedTab;

	public:
		//static void CenterDialog(HWND hWnd2);
		void OnSettingsClosed();
		DWORD BalloonStyle();
		// IDD_SETTINGS
		static INT_PTR CALLBACK wndOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		// Вкладки настроек: IDD_SPG_MAIN, IDD_SPG_FEATURE, и т.д.
		static INT_PTR CALLBACK pageOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		//
		void debugLogShell(HWND hWnd2, DebugLogShellActivity *pShl);
		void debugLogShellText(wchar_t* &pszParamEx, LPCWSTR asFile);
		void debugLogInfo(HWND hWnd2, CESERVER_REQ_PEEKREADINFO* pInfo);
		void debugLogCommand(CESERVER_REQ* pInfo, BOOL abInput, DWORD anTick, DWORD anDur, LPCWSTR asPipe, CESERVER_REQ* pResult = NULL);
		//
		bool SetOption(LPCWSTR asName, int anValue);
		bool SetOption(LPCWSTR asName, LPCWSTR asValue);
		void SettingsLoaded(SettingsLoadedFlags slfFlags, LPCWSTR pszCmdLine = NULL);
		void SettingsPreSave();
		static void Dialog(int IdShowPage = 0);
		void UpdateWindowMode(ConEmuWindowMode WndMode);
		void UpdatePosSizeEnabled(HWND hWnd2);
		void UpdatePos(int x, int y, bool bGetRect = false);
		void UpdateSize(const CESize w, const CESize h);
		void UpdateFontInfo();
		void UpdateLogLocation();
		void Performance(UINT nID, BOOL bEnd);
		void PostUpdateCounters(bool bPosted);
		void SetArgBufferHeight(int anBufferHeight);
	public:
		enum ShellIntegrType
		{
			ShellIntgr_Inside = 0,
			ShellIntgr_Here,
			ShellIntgr_CmdAuto,
		};
		void ShellIntegration(HWND hDlg, ShellIntegrType iMode, bool bEnabled, bool bForced = false);
	private:
		void RegisterShell(LPCWSTR asName, LPCWSTR asOpt, LPCWSTR asConfig, LPCWSTR asCmd, LPCWSTR asIcon);
		void UnregisterShell(LPCWSTR asName);
		void UnregisterShellInvalids();
		bool DeleteRegKeyRecursive(HKEY hRoot, LPCWSTR asParent, LPCWSTR asName);
	public:
		void UpdateConsoleMode(CRealConsole* pRCon);
		bool CheckTheming();
		void OnPanelViewAppeared(BOOL abAppear);
		bool EditConsoleFont(HWND hParent);
		static INT_PTR CALLBACK EditConsoleFontProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		static int CALLBACK EnumConFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount);
		bool CheckConsoleFontFast(LPCWSTR asCheckName = NULL);
		enum
		{
			ConFontErr_NonSystem   = 0x01,
			ConFontErr_NonRegistry = 0x02,
			ConFontErr_InvalidName = 0x04,
		};
	protected:
		BOOL bShowConFontError, bConsoleFontChecked;
		wchar_t sConFontError[512];
		wchar_t sDefaultConFontName[32]; // "последний шанс", если юзер отказался выбрать нормальный шрифт
		HWND hConFontDlg;
		DWORD nConFontError; // 0x01 - шрифт не зарегистрирован в системе, 0x02 - не указан в реестре для консоли
		HWND hwndConFontBalloon;
		static bool CheckConsoleFontRegistry(LPCWSTR asFaceName);
		static bool CheckConsoleFont(HWND ahDlg);
		static void ShowConFontErrorTip(LPCTSTR asInfo);
		LPCWSTR CreateConFontError(LPCWSTR asReqFont=NULL, LPCWSTR asGotFont=NULL);
		TOOLINFO tiConFontBalloon;
		DpiValue _dpi;
		DpiValue _dpi_all;
		int GetOverallDpi();
	public:
		int QueryDpi();
	private:
		static void ShowErrorTip(LPCTSTR asInfo, HWND hDlg, int nCtrlID, wchar_t* pszBuffer, int nBufferSize, HWND hBall, TOOLINFO *pti, HWND hTip, DWORD nTimeout, bool bLeftAligh = false);
	protected:
		void OnResetOrReload(bool abResetOnly, SettingsStorage* pXmlStorage = NULL);
		void ExportSettings();
		void ImportSettings();
		void SearchForControls(); // Find setting by typed name in the "Search" box
		static void InvalidateCtrl(HWND hCtrl, BOOL bErase);
		// IDD_SETTINGS
		LRESULT OnInitDialog();
		// OnInitDialogPage_t: IDD_SPG_MAIN, и т.д.
		LRESULT OnInitDialog_Main(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Background(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_WndSizePos(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Show(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Confirm(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Taskbar(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Cursor(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Startup(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Ext(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Comspec(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Environment(HWND hWnd2, bool abInitial);
		//LRESULT OnInitDialog_Output(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_MarkCopy(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Paste(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Hilight(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Far(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_FarMacro(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Keys(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Control(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Tabs(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Status(HWND hWnd2, bool abInitial);
		void OnInitDialog_StatusItems(HWND hWnd2);
		LRESULT OnInitDialog_Color(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Transparent(HWND hWnd2);
		LRESULT OnInitDialog_Tasks(HWND hWnd2, bool abForceReload);
		LRESULT OnInitDialog_Apps(HWND hWnd2, bool abForceReload);
		LRESULT OnInitDialog_Integr(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_DefTerm(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Views(HWND hWnd2);
		void OnInitDialog_CopyFonts(HWND hWnd2, int nList1, ...); // скопировать список шрифтов с вкладки hMain
		LRESULT OnInitDialog_Debug(HWND hWnd2);
		LRESULT OnInitDialog_Update(HWND hWnd2, bool abInitial);
		LRESULT OnInitDialog_Info(HWND hWnd2);
		//
		void InitCursorCtrls(HWND hWnd2, const AppSettings* pApp);
		bool mb_IgnoreCmdGroupEdit, mb_IgnoreCmdGroupList;
		//LRESULT OnButtonClicked_Apps(HWND hWnd2, WPARAM wParam, LPARAM lParam);
		//UINT mn_AppsEnableControlsMsg;
		WNDPROC mf_MarkCopyPreviewProc;
		static LRESULT CALLBACK MarkCopyPreviewProc(HWND hCtrl, UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR pageOpProc_Integr(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		static INT_PTR CALLBACK pageOpProc_AppsChild(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		INT_PTR pageOpProc_Apps(HWND hWnd2, HWND hChild, UINT messg, WPARAM wParam, LPARAM lParam);
		INT_PTR pageOpProc_Start(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		//LRESULT OnColorButtonClicked(HWND hWnd2, WPARAM wParam, LPARAM lParam);
		//LRESULT OnColorComboBox(HWND hWnd2, WPARAM wParam, LPARAM lParam);
		//LRESULT OnColorEditChanged(HWND hWnd2, WPARAM wParam, LPARAM lParam);
		LRESULT OnEditChanged(HWND hWnd2, WPARAM wParam, LPARAM lParam);
		bool OnEditChanged_Cursor(HWND hWnd2, WPARAM wParam, LPARAM lParam, AppSettings* pApp);
		LRESULT OnComboBox(HWND hWnd2, WPARAM wParam, LPARAM lParam);
		LRESULT OnListBoxDblClk(HWND hWnd2, WPARAM wParam, LPARAM lParam);
		LRESULT OnPage(LPNMHDR phdr);
		INT_PTR OnMeasureFontItem(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnDrawFontItem(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		void OnSaveActivityLogFile(HWND hListView);
		LRESULT OnActivityLogNotify(HWND hWnd2, WPARAM wParam, LPARAM lParam);
		void FillHotKeysList(HWND hWnd2, BOOL abInitial);
		void FillCBList(HWND hCombo, bool abInitial, LPCWSTR* ppszPredefined, LPCWSTR pszUser);
		LRESULT OnHotkeysNotify(HWND hWnd2, WPARAM wParam, LPARAM lParam);
		static int CALLBACK HotkeysCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
		wchar_t szSelectionModError[512];
		void CheckSelectionModifiers(HWND hWnd2);
		UINT mn_ActivateTabMsg;
		bool mb_IgnoreSelPage;
		void UpdateTextColorSettings(BOOL ChangeTextAttr = TRUE, BOOL ChangePopupAttr = TRUE, const AppSettings* apDistinct = NULL);
		void setHotkeyCheckbox(HWND hDlg, WORD nCtrlId, int iHotkeyId, LPCWSTR pszFrom, LPCWSTR pszTo, bool bChecked);
	public:
		void ChangeCurrentPalette(const ColorPalette* pPal, bool bChangeDropDown);
	private:
		BOOL mb_IgnoreEditChanged;
		//BOOL mb_CharSetWasSet;
		i64 mn_Freq;
		i64 mn_FPS[256]; int mn_FPS_CUR_FRAME;
		i64 mn_RFPS[128]; int mn_RFPS_CUR_FRAME;
		i64 mn_Counter[tPerfLast];
		i64 mn_CounterMax[tPerfLast];
		DWORD mn_CounterTick[tPerfLast];
		i64 mn_KbdDelayCounter, mn_KbdDelays[32/*must be power of 2*/]; LONG mn_KBD_CUR_FRAME;
		HWND hwndTip, hwndBalloon;
		static void ShowFontErrorTip(LPCTSTR asInfo);
		TOOLINFO tiBalloon;
		void RegisterTipsFor(HWND hChildDlg);
		static BOOL CALLBACK RegisterTipsForChild(HWND hChild, LPARAM lParam);
		void RecreateFont(WORD wFromID);
#if 0
		// Theming
		HMODULE mh_Uxtheme;
		typedef HRESULT(STDAPICALLTYPE *SetWindowThemeT)(HWND hwnd,LPCWSTR pszSubAppName,LPCWSTR pszSubIdList);
		SetWindowThemeT SetWindowThemeF;
		typedef HRESULT(STDAPICALLTYPE *EnableThemeDialogTextureT)(HWND hwnd,DWORD dwFlags);
		EnableThemeDialogTextureT EnableThemeDialogTextureF;
#endif
		UINT mn_MsgUpdateCounter; BOOL mb_MsgUpdateCounter;
		//wchar_t temp[MAX_PATH];
		UINT mn_MsgRecreateFont;
		UINT mn_MsgLoadFontFromMain;
	public:
		static int GetNumber(HWND hParent, WORD nCtrlId, int nMin = 0, int nMax = 0);
		static INT_PTR GetString(HWND hParent, WORD nCtrlId, wchar_t** ppszStr, LPCWSTR asNoDefault = NULL, bool abListBox = false);
		static void EnableDlgItem(HWND hParent, WORD nCtrlId, BOOL bEnabled);
	private:
		BOOL mb_TabHotKeyRegistered;
		void RegisterTabs();
		void UnregisterTabs();
		WORD mn_LastChangingFontCtrlId;

		bool mb_ThemingEnabled;

	public:
		static void SetHotkeyField(HWND hHk, BYTE vk);
	private:
		static void CenterMoreDlg(HWND hWnd2);
	private:
		DWORD MakeHotKey(BYTE Vk, BYTE vkMod1=0, BYTE vkMod2=0, BYTE vkMod3=0) { return ConEmuHotKey::MakeHotKey(Vk, vkMod1, vkMod2, vkMod3); };
		ConEmuHotKeyList m_HotKeys; // : public MArray<ConEmuHotKey>
		ConEmuHotKey *mp_ActiveHotKey;
		void SetHotkeyVkMod(ConEmuHotKey *pHK, DWORD VkMod);
		friend struct Settings;
	public:
		const ConEmuHotKey* GetHotKeyPtr(int idx);
		const ConEmuHotKey* GetHotKeyInfo(const ConEmuChord& VkState, bool bKeyDown, CRealConsole* pRCon);
		void UpdateWinHookSettings(HMODULE hLLKeyHookDll);
	public:
		bool isDialogMessage(MSG &Msg);
	private:
		CEHelpPopup* mp_HelpPopup;
		INT_PTR ProcessTipHelp(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
	private:

		enum KeyListColumns
		{
			klc_Type = 0,
			klc_Hotkey,
			klc_Desc
		};
		enum LogProcessColumns
		{
			lpc_Time = 0,
			lpc_PPID,
			lpc_Func,
			lpc_Oper,
			lpc_Bits,
			lpc_System,
			lpc_App,
			lpc_Params,
			lpc_Flags,
			lpc_StdIn,
			lpc_StdOut,
			lpc_StdErr,
		};
		enum LogInputColumns
		{
			lic_Time = 0,
			lic_Type,
			lic_Dup,
			lic_Event,
		};
		enum LogCommandsColumns
		{
			lcc_InOut = 0,
			lcc_Time,
			lcc_Duration,
			lcc_Command,
			lcc_Size,
			lcc_PID,
			lcc_Pipe,
			lcc_Extra,
		};
		enum LogAnsiColumns
		{
			lac_Time = 0,
			lac_Sequence,
		};
		struct LogCommandsData
		{
			BOOL  bInput, bMainThread;
			DWORD nTick, nDur, nCmd, nSize, nPID;
			wchar_t szPipe[64];
			wchar_t szExtra[128];
		};
		void debugLogCommand(HWND hWnd2, LogCommandsData* apData);

		enum ConEmuSetupItemType
		{
			sit_Bool      = 1,
			sit_Byte      = 2,
			sit_Char      = 3,
			sit_Long      = 4,
			sit_ULong     = 5,
			sit_Rect      = 6,
			sit_FixString = 7,
			sit_VarString = 8,
			sit_MSZString = 9,
			sit_FixData   = 10,
			sit_Fonts     = 11,
			sit_FontsAndRaster = 12,
		};
		struct ConEmuSetupItem
		{
			//DWORD nDlgID; // ID диалога
			DWORD nItemID; // ID контрола в диалоге, 0 - последний элемент в списке
			ConEmuSetupItemType nDataType; // Тип данных

			void* pData; // Ссылка на элемент в gpSet
			size_t nDataSize; // Размер или maxlen, если pData фиксированный

			ConEmuSetupItemType nListType; // Тип данных в pListData
			const void* pListData; // Для DDLB - можно задать список
			size_t nListItems; // количество элементов в списке

			#ifdef _DEBUG
			BOOL bFound; // для отладки корректности настройки
			#endif

			//wchar_t sItemName[32]; // Имя элемента в настройке (reg/xml)
		};
		struct ConEmuSetupPages
		{
			int              PageID;       // Dialog ID
			int              Level;        // 0, 1
			wchar_t          PageName[64]; // Label in treeview
			TabHwndIndex     PageIndex;    // mh_Tabs[...]
			bool             Collapsed;
			// Filled after creation
			bool             DpiChanged;
			HTREEITEM        hTI;
			ConEmuSetupItem* pItems;
			HWND             hPage;
			CDpiForDialog*   pDpiAware;
			CDynDialog*      pDialog;
		};
		CDynDialog *mp_Dialog;
		ConEmuSetupPages *m_Pages;
		int mn_PagesCount;
		static void SelectTreeItem(HWND hTree, HTREEITEM hItem, bool bPost = false);
		void ClearPages();
		HWND CreatePage(ConEmuSetupPages* p);
		CDynDialog*    mp_DlgDistinct2;
		CDpiForDialog* mp_DpiDistinct2;
		void ProcessDpiChange(ConEmuSetupPages* p);
		TabHwndIndex GetPageId(HWND hPage, ConEmuSetupPages** pp);
		int GetDialogDpi();
};
