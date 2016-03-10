
/*
Copyright (c) 2014-2016 Maximus5
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

#define BST(v) (int)(v & 3) // BST_UNCHECKED/BST_CHECKED/BST_INDETERMINATE

class CSetDlgButtons
{
public:
	static bool checkDlgButton(HWND hParent, WORD nCtrlId, UINT uCheck);
	static bool checkRadioButton(HWND hParent, int nIDFirstButton, int nIDLastButton, int nIDCheckButton);
	static void enableDlgItems(HWND hParent, UINT* pnCtrlID, INT_PTR nCount, bool bEnabled);
	static BYTE IsChecked(HWND hParent, WORD nCtrlId);
	static BYTE IsChecked(WORD nCtrlId, WORD CB, BYTE uCheck);

public:
	static wchar_t* CheckButtonMacro(WORD CB, BYTE uCheck);

protected:
	static bool ProcessButtonClick(HWND hDlg, WORD CB, BYTE uCheck);
	static LRESULT OnButtonClicked(HWND hDlg, WPARAM wParam, LPARAM lParam);
	static void OnButtonClicked_Cursor(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp);

#ifdef _DEBUG
protected:
	static void UnitTests();
#endif

protected:
	// These function may be used for AppStd and AppDistinct
	static void OnBtn_CursorBlink(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp);
	static void OnBtn_CursorColor(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp);
	static void OnBtn_CursorIgnoreSize(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp);
	static void OnBtn_CursorX(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp);
	static void OnBtn_InactiveCursor(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp);
	static void OnBtn_InactiveCursorBlink(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp);
	static void OnBtn_InactiveCursorColor(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp);
	static void OnBtn_InactiveCursorIgnoreSize(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp);
	static void OnBtn_InactiveCursorX(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp);

protected:
	static void OnBtn_CmdTasksFlags(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CmdTasksAdd(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CmdTasksDel(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CmdTasksUpDown(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CmdGroupKey(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CmdGroupApp(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CmdTasksParm(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CmdTasksDir(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CmdTasksActive(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_AddDefaults(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CmdTasksReload(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CmdTaskbarTasks(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CmdTaskbarCommands(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CmdTaskbarUpdate(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_NoneStandardCleartype(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_NormalFullscreenMaximized(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ApplyPos(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CascadeFixed(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UseCurrentSizePos(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_AutoSaveSizePos(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_FontAuto(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_FixFarBorders(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_AntiAliasing2(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UnicodeRangesApply(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_SingleInstance(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ShowHelpTooltips(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_MultiCon(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_MultiShowPanes(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_MultiIterate(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_NewConfirm(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_DupConfirm(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ConfirmDetach(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_LongOutput(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ComspecRadio(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ComspecExplicit(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ComspecTest(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ComspecBitsRadio(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ComspecUpdateEnv(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_AddConEmu2Path(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_AddConEmuBase2Path(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ComspecUncPaths(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CmdAutorunNewWnd(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CmdAutoActions(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_FontStyles(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_BgImageEnable(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_BgImageChoose(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_BgReplaceTransparent(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_BgAllowPlugin(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_RClick(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_SafeFarClose(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_MinToTray(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CloseConfirmFlags(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_AlwaysShowTrayIcon(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_QuakeStyles(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_HideCaption(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_HideCaptionAlways(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_HideChildCaption(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_FARuseASCIIsort(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ShellNoZoneCheck(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_KeyBarRClick(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_DragPanel(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_DragPanelBothEdges(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_TryToCenter(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_IntegralSize(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_Restore2ActiveMonitor(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ScrollbarStyle(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_FarHourglass(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ExtendUCharMap(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_FixAltOnAltTab(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_AutoRegFonts(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_DebugSteps(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_DragLR(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_DropEnabled(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_DnDCopy(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_DropUseMenu(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_DragImage(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_DragIcons(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_EnhanceGraphics(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_EnhanceButtons(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_TabsRadioAuto(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_TabsLocationBottom(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_OneTabPerGroup(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ActivateSplitMouseOver(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_TabSelf(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_TabRecent(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_TabLazy(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_TaskbarOverlay(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_TaskbarProgress(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_TaskbarBtnRadio(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_RSelectionFix(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_EnableMouse(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_SkipActivation(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_SkipMove(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_MonitorConsoleLang(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_SkipFocusEvents(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_Monospace(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ExtendFonts(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CursorOptions(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_RConVisible(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UseInjects(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ProcessAnsi(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_AnsiLog(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ProcessNewConArg(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ProcessCmdStart(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ProcessCtrlZ(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_SuppressBells(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ConsoleExceptionHandler(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UseClink(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ClinkWebPage(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_RealConsoleSettings(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_SnapToDesktopEdges(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_AlwaysOnTop(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_SleepInBackground(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_RetardInactivePanes(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_MinimizeOnLoseFocus(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_FocusInChildWindows(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_DisableFarFlashing(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_DisableAllFlashing(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ShowWasHiddenMsg(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ShowWasSetOnTopMsg(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_TabsInCaption(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_NumberInCaption(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_AdminSuffixOrShield(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_HideInactiveConTabs(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_HideDisabledTabs(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ShowFarWindows(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CloseConEmuOptions(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_HideOrMinOnLastTabClose(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_MinByEscRadio(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_MapShiftEscToEsc(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_GuiMacroHelp(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UseWinArrowNumTab(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_SendConsoleSpecials(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_HotkeysListShowOptions(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_InstallKeybHooks(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_DosBox(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ApplyViewSettings(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ThumbLoadFiles(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ThumbLoadFolders(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ThumbUsePicView2(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ThumbRestoreOnStartup(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ThumbPreviewBox(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ThumbSelectionBox(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ThumbBackColors(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ThumbPreviewBoxColors(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ThumbSelectionBoxColors(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ActivityReset(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ActivitySaveAs(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_DebugActivityRadio(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ColorFormat(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ExtendColors(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ColorResetExt(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ColorSchemeSaveDelete(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_TrueColorer(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_FadeInactive(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_Transparent(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_TransparentSeparate(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UserScreenTransparent(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ColorKeyTransparent(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSIntelligent(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSActConditionRadio(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSFreezeBeforeSelect(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSAutoCopy(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSResetOnRelease(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSIBeam(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSEndCopyAuto(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSEndOnKeyPress(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSEraseBeforeReset(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSBlockSelection(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSTextSelection(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSDetectLineEnd(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSBashMargin(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSTrimTrailing(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSClickPromptPosition(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSDeleteLeftWord(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ClipShiftIns(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_CTSShiftArrowStartSel(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ClipCtrlV(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ClipConfirmEnter(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ClipConfirmLimit(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_FarGotoEditor(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_HighlightMouseRow(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_HighlightMouseCol(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UpdateCheckOnStartup(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UpdateCheckHourly(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UpdateConfirmDownload(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UpdateTypeRadio(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UpdateInetTool(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UpdateInetToolCmd(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UpdateUseProxy(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UpdateLeavePackages(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UpdateArcCmdLine(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UpdateDownloadPath(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_UpdateApplyAndCheck(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_DefTerm(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_GotoEditorCmd(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ColorField(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_ShowStatusBar(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_StatusVertSep(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_StatusHorzSep(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_StatusVertPad(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_StatusSystemColors(HWND hDlg, WORD CB, BYTE uCheck);
	static void OnBtn_StatusAddDel(HWND hDlg, WORD CB, BYTE uCheck);
};
