﻿
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
#include "SetPgBase.h"

#define BALLOON_MSG_TIMERID 101

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
	, public CSetDlgFonts
{
	public:
		CSettings();
		virtual ~CSettings();
	private:
		Settings m_Settings;
		void ReleaseHandles();
		void InitVars_Pages();
		void InitPageNames();
	private:
		friend class CSetDlgButtons;
		friend class CSetDlgColors;
		friend class CSetDlgFonts;
		friend class CFontMgr;
		friend class CSetPgFonts;
		friend class CSetPgBase;
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
	public:
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

		HWND GetActivePage();
		CSetPgBase* GetActivePageObj();
		HWND GetPage(TabHwndIndex nPage);
		CSetPgBase* GetPageObj(TabHwndIndex nPage);
		template <class T>
		bool GetPageObj(T*& pObject)
		{
			pObject = dynamic_cast<T*>(GetPageObj(T::PageType()));
			return (pObject != NULL);
		};
		TabHwndIndex GetPageId(HWND hPage);

	private:
		TabHwndIndex m_LastActivePageId;

	public:
		//static void CenterDialog(HWND hWnd2);
		void OnSettingsClosed();
		DWORD BalloonStyle();
		// IDD_SETTINGS
		static INT_PTR CALLBACK wndOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
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
		void UpdatePos(int x, int y, bool bGetRect = false);
		void UpdateSize(const CESize w, const CESize h);
		void UpdateFontInfo();
		void Performance(UINT nID, BOOL bEnd);
		void PostUpdateCounters(bool bPosted);
		void SetArgBufferHeight(int anBufferHeight);
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
		// IDD_SETTINGS
		LRESULT OnInitDialog();
		//
		LRESULT OnListBoxDblClk(HWND hWnd2, WPARAM wParam, LPARAM lParam);
		LRESULT OnPage(LPNMHDR phdr);
		INT_PTR OnMeasureFontItem(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnDrawFontItem(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		void OnSaveActivityLogFile(HWND hListView);
		LRESULT OnActivityLogNotify(HWND hWnd2, WPARAM wParam, LPARAM lParam);
		wchar_t szSelectionModError[512];
		UINT mn_ActivateTabMsg;
		bool mb_IgnoreSelPage;
	public:
		void UpdateTextColorSettings(BOOL ChangeTextAttr = TRUE, BOOL ChangePopupAttr = TRUE, const AppSettings* apDistinct = NULL);
		void CheckSelectionModifiers(HWND hWnd2);
		void ChangeCurrentPalette(const ColorPalette* pPal, bool bChangeDropDown);
		void RegisterTipsFor(HWND hChildDlg);
	private:
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
	private:
		BOOL mb_TabHotKeyRegistered;
		void RegisterTabs();
		void UnregisterTabs();

		WORD mn_LastChangingFontCtrlId;

		bool mb_ThemingEnabled;

	private:
		static void CenterMoreDlg(HWND hWnd2);
	private:
		friend struct Settings;
	public:
		void UpdateWinHookSettings(HMODULE hLLKeyHookDll);
	public:
		bool isDialogMessage(MSG &Msg);
	private:
		CEHelpPopup* mp_HelpPopup;
	public:
		INT_PTR ProcessTipHelp(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
	private:

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

		CDynDialog *mp_Dialog;
		ConEmuSetupPages *m_Pages;
		int mn_PagesCount;
		static void SelectTreeItem(HWND hTree, HTREEITEM hItem, bool bPost = false);
		void ClearPages();
		TabHwndIndex GetPageId(HWND hPage, ConEmuSetupPages** pp);
		const ConEmuSetupPages* GetPageData(TabHwndIndex nPage);
		TabHwndIndex GetPageIdByDialogId(UINT DialogID);
		int GetDialogDpi();
};
