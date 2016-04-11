﻿
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"
#pragma warning(disable: 4091)
#include <shlobj.h>
#pragma warning(default: 4091)
#ifdef __GNUC__
#include "ShObjIdl_Part.h"
#endif // __GNUC__

#include "../common/MSetter.h"
#include "../common/WUser.h"

#include "AboutDlg.h"
#include "Background.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "DefaultTerm.h"
#include "HotkeyDlg.h"
#include "Options.h"
#include "OptionsClass.h"
#include "OptionsFast.h"
#include "Recreate.h"
#include "SetCmdTask.h"
#include "SetDlgButtons.h"
#include "SetDlgLists.h"
#include "SetPgColors.h"
#include "SetPgComspec.h"
#include "SetPgFeatures.h"
#include "SetPgIntegr.h"
#include "SetPgKeys.h"
#include "SetPgPaste.h"
#include "SetPgSizePos.h"
#include "SetPgStatus.h"
#include "SetPgTasks.h"
#include "Status.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

#ifdef _DEBUG
void CSetDlgButtons::UnitTests()
{
	_ASSERTE(lstrcmp(CFontMgr::RASTER_FONTS_NAME, L"Raster Fonts")==0);
}
#endif

BYTE CSetDlgButtons::isOptChecked(WORD nCtrlId, WORD CB, BYTE uCheck)
{
	if (nCtrlId == CB)
		return uCheck;
	return 0;
}

bool CSetDlgButtons::ProcessButtonClick(HWND hDlg, WORD CB, BYTE uCheck)
{
	bool bProcessed = true;

	switch (CB)
	{
		case IDOK:
		case IDCLOSE:
			_ASSERTE(FALSE && "IDOK/IDCLOSE must be processed in wndOpProc");
			break;
		case IDCANCEL:
			// That may be triggered in Tasks' commands field (multiline edit)
			PostMessage(ghOpWnd, WM_CLOSE, 0, 0);
			break;
		case rNoneAA:
		case rStandardAA:
		case rCTAA:
			OnBtn_NoneStandardCleartype(hDlg, CB, uCheck);
			break;
		case rNormal:
		case rFullScreen:
		case rMaximized:
			OnBtn_NormalFullscreenMaximized(hDlg, CB, uCheck);
			break;
		case cbApplyPos:
			OnBtn_ApplyPos(hDlg, CB, uCheck);
			break;
		case rCascade:
		case rFixed:
			OnBtn_CascadeFixed(hDlg, CB, uCheck);
			break;
		case cbUseCurrentSizePos:
			OnBtn_UseCurrentSizePos(hDlg, CB, uCheck);
			break;
		case cbAutoSaveSizePos:
			OnBtn_AutoSaveSizePos(hDlg, CB, uCheck);
			break;
		case cbFontAuto:
			OnBtn_FontAuto(hDlg, CB, uCheck);
			break;
		case cbFixFarBorders:
			OnBtn_FixFarBorders(hDlg, CB, uCheck);
			break;
		case cbFont2AA:
			OnBtn_AntiAliasing2(hDlg, CB, uCheck);
			break;
		case cbUnicodeRangesApply:
			OnBtn_UnicodeRangesApply(hDlg, CB, uCheck);
			break;
		//case cbCursorColor:
		//	gpSet->AppStd.isCursorColor = isChecked(hDlg,cbCursorColor);
		//	gpConEmu->Update(true);
		//	break;
		//case cbCursorBlink:
		//	gpSet->AppStd.isCursorBlink = isChecked(hDlg,cbCursorBlink);
		//	if (!gpSet->AppStd.isCursorBlink) // если мигание отключается - то курсор может "замереть" в погашенном состоянии.
		//		CVConGroup::InvalidateAll();
		//	break;
		case cbSingleInstance:
			OnBtn_SingleInstance(hDlg, CB, uCheck);
			break;
		case cbShowHelpTooltips:
			OnBtn_ShowHelpTooltips(hDlg, CB, uCheck);
			break;
		case cbMultiCon:
			OnBtn_MultiCon(hDlg, CB, uCheck);
			break;
		case cbMultiShowButtons:
		case cbMultiShowSearch:
			OnBtn_MultiShowPanes(hDlg, CB, uCheck);
			break;
		case cbMultiIterate:
			OnBtn_MultiIterate(hDlg, CB, uCheck);
			break;
		case cbNewConfirm:
			OnBtn_NewConfirm(hDlg, CB, uCheck);
			break;
		case cbDupConfirm:
			OnBtn_DupConfirm(hDlg, CB, uCheck);
			break;
		case cbConfirmDetach:
			OnBtn_ConfirmDetach(hDlg, CB, uCheck);
			break;
		case cbLongOutput:
			OnBtn_LongOutput(hDlg, CB, uCheck);
			break;
		case rbComspecAuto:
		case rbComspecEnvVar:
		case rbComspecCmd:
		case rbComspecExplicit:
			OnBtn_ComspecRadio(hDlg, CB, uCheck);
			break;
		case cbComspecExplicit:
			OnBtn_ComspecExplicit(hDlg, CB, uCheck);
			break;
		case cbComspecTest:
			OnBtn_ComspecTest(hDlg, CB, uCheck);
			break;
		case rbComspec_OSbit:
		case rbComspec_AppBit:
		case rbComspec_x32:
			OnBtn_ComspecBitsRadio(hDlg, CB, uCheck);
			break;
		case cbComspecUpdateEnv:
			OnBtn_ComspecUpdateEnv(hDlg, CB, uCheck);
			break;
		case cbAddConEmu2Path:
			OnBtn_AddConEmu2Path(hDlg, CB, uCheck);
			break;
		case cbAddConEmuBase2Path:
			OnBtn_AddConEmuBase2Path(hDlg, CB, uCheck);
			break;
		case cbComspecUncPaths:
			OnBtn_ComspecUncPaths(hDlg, CB, uCheck);
			break;
		case cbCmdAutorunNewWnd:
			OnBtn_CmdAutorunNewWnd(hDlg, CB, uCheck);
			break;
		case bCmdAutoRegister:
		case bCmdAutoUnregister:
		case bCmdAutoClear:
			OnBtn_CmdAutoActions(hDlg, CB, uCheck);
			break;
		case cbBold:
		case cbItalic:
		case cbFontAsDeviceUnits:
		case cbFontMonitorDpi:
			OnBtn_FontStyles(hDlg, CB, uCheck);
			break;
		case cbBgImage:
			OnBtn_BgImageEnable(hDlg, CB, uCheck);
			break;
		case bBgImage:
			OnBtn_BgImageChoose(hDlg, CB, uCheck);
			break;
		case cbBgAllowPlugin:
			OnBtn_BgAllowPlugin(hDlg, CB, uCheck);
			break;
		case cbRClick:
			OnBtn_RClick(hDlg, CB, uCheck);
			break;
		case cbSafeFarClose:
			OnBtn_SafeFarClose(hDlg, CB, uCheck);
			break;
		case cbMinToTray:
			OnBtn_MinToTray(hDlg, CB, uCheck);
			break;
		case cbCloseWindowConfirm:
		case cbCloseConsoleConfirm:
		case cbConfirmCloseRunning:
		case cbCloseEditViewConfirm:
			OnBtn_CloseConfirmFlags(hDlg, CB, uCheck);
			break;
		case cbAlwaysShowTrayIcon:
			OnBtn_AlwaysShowTrayIcon(hDlg, CB, uCheck);
			break;
		case cbQuakeStyle:
		case cbQuakeAutoHide:
			OnBtn_QuakeStyles(hDlg, CB, uCheck);
			break;
		case cbHideCaption:
			OnBtn_HideCaption(hDlg, CB, uCheck);
			break;
		case cbHideCaptionAlways:
			OnBtn_HideCaptionAlways(hDlg, CB, uCheck);
			break;
		case cbHideChildCaption:
			OnBtn_HideChildCaption(hDlg, CB, uCheck);
			break;
		case cbFARuseASCIIsort:
			OnBtn_FARuseASCIIsort(hDlg, CB, uCheck);
			break;
		case cbShellNoZoneCheck:
			OnBtn_ShellNoZoneCheck(hDlg, CB, uCheck);
			break;
		case cbKeyBarRClick:
			OnBtn_KeyBarRClick(hDlg, CB, uCheck);
			break;
		case cbDragPanel:
			OnBtn_DragPanel(hDlg, CB, uCheck);
			break;
		case cbDragPanelBothEdges:
			OnBtn_DragPanelBothEdges(hDlg, CB, uCheck);
			break;
		case cbTryToCenter:
			OnBtn_TryToCenter(hDlg, CB, uCheck);
			break;
		case cbIntegralSize:
			OnBtn_IntegralSize(hDlg, CB, uCheck);
			break;
		case cbRestore2ActiveMonitor:
			OnBtn_Restore2ActiveMonitor(hDlg, CB, uCheck);
			break;
		case rbScrollbarHide:
		case rbScrollbarShow:
		case rbScrollbarAuto:
			OnBtn_ScrollbarStyle(hDlg, CB, uCheck);
			break;
		case cbFarHourglass:
			OnBtn_FarHourglass(hDlg, CB, uCheck);
			break;
		case cbExtendUCharMap:
			OnBtn_ExtendUCharMap(hDlg, CB, uCheck);
			break;
		case cbFixAltOnAltTab:
			OnBtn_FixAltOnAltTab(hDlg, CB, uCheck);
			break;
		case cbAutoRegFonts:
			OnBtn_AutoRegFonts(hDlg, CB, uCheck);
			break;
		case cbDebugSteps:
			OnBtn_DebugSteps(hDlg, CB, uCheck);
			break;
		case cbDebugLog:
			OnBtn_DebugLog(hDlg, CB, uCheck);
			break;
		case cbDragL:
		case cbDragR:
			OnBtn_DragLR(hDlg, CB, uCheck);
			break;
		case cbDropEnabled:
			OnBtn_DropEnabled(hDlg, CB, uCheck);
			break;
		case cbDnDCopy:
			OnBtn_DnDCopy(hDlg, CB, uCheck);
			break;
		case cbDropUseMenu:
			OnBtn_DropUseMenu(hDlg, CB, uCheck);
			break;
		case cbDragImage:
			OnBtn_DragImage(hDlg, CB, uCheck);
			break;
		case cbDragIcons:
			OnBtn_DragIcons(hDlg, CB, uCheck);
			break;
		case cbEnhanceGraphics: // Progressbars and scrollbars
			OnBtn_EnhanceGraphics(hDlg, CB, uCheck);
			break;
		case cbEnhanceButtons: // Buttons, CheckBoxes and RadioButtons
			OnBtn_EnhanceButtons(hDlg, CB, uCheck);
			break;
		case rbTabsNone:
		case rbTabsAlways:
		case rbTabsAuto:
			OnBtn_TabsRadioAuto(hDlg, CB, uCheck);
			break;
		case cbTabsLocationBottom:
			OnBtn_TabsLocationBottom(hDlg, CB, uCheck);
			break;
		case cbOneTabPerGroup:
			OnBtn_OneTabPerGroup(hDlg, CB, uCheck);
			break;
		case cbActivateSplitMouseOver:
			OnBtn_ActivateSplitMouseOver(hDlg, CB, uCheck);
			break;
		case cbTabSelf:
			OnBtn_TabSelf(hDlg, CB, uCheck);
			break;
		case cbTabRecent:
			OnBtn_TabRecent(hDlg, CB, uCheck);
			break;
		case cbTabLazy:
			OnBtn_TabLazy(hDlg, CB, uCheck);
			break;
		case cbTaskbarOverlay:
			OnBtn_TaskbarOverlay(hDlg, CB, uCheck);
			break;
		case cbTaskbarProgress:
			OnBtn_TaskbarProgress(hDlg, CB, uCheck);
			break;
		case rbTaskbarBtnActive:
		case rbTaskbarBtnAll:
		case rbTaskbarBtnWin7:
		case rbTaskbarBtnHidden:
			OnBtn_TaskbarBtnRadio(hDlg, CB, uCheck);
			break;
		case cbRSelectionFix:
			OnBtn_RSelectionFix(hDlg, CB, uCheck);
			break;
		case cbEnableMouse:
			OnBtn_EnableMouse(hDlg, CB, uCheck);
			break;
		case cbSkipActivation:
			OnBtn_SkipActivation(hDlg, CB, uCheck);
			break;
		case cbSkipMove:
			OnBtn_SkipMove(hDlg, CB, uCheck);
			break;
		case cbMonitorConsoleLang:
			OnBtn_MonitorConsoleLang(hDlg, CB, uCheck);
			break;
		case cbSkipFocusEvents:
			OnBtn_SkipFocusEvents(hDlg, CB, uCheck);
			break;
		case cbMonospace:
			OnBtn_Monospace(hDlg, CB, uCheck);
			break;
		case cbExtendFonts:
			OnBtn_ExtendFonts(hDlg, CB, uCheck);
			break;
		//case cbAutoConHandle:
		//	isUpdConHandle = !isUpdConHandle;
		//	gpConEmu->Update(true);
		//	break;
		case rCursorH:
		case rCursorV:
		case rCursorB:
		case rCursorR:
		case cbCursorColor:
		case cbCursorBlink:
		case cbCursorIgnoreSize:
		case cbInactiveCursor:
		case rInactiveCursorH:
		case rInactiveCursorV:
		case rInactiveCursorB:
		case rInactiveCursorR:
		case cbInactiveCursorColor:
		case cbInactiveCursorBlink:
		case cbInactiveCursorIgnoreSize:
			OnBtn_CursorOptions(hDlg, CB, uCheck);
			break;
		case cbVisible:
			OnBtn_RConVisible(hDlg, CB, uCheck);
			break;
		//case cbLockRealConsolePos:
		//	isLockRealConsolePos = isChecked(hDlg, cbLockRealConsolePos);
		//	break;
		case cbUseInjects:
			OnBtn_UseInjects(hDlg, CB, uCheck);
			break;
		case cbProcessAnsi:
			OnBtn_ProcessAnsi(hDlg, CB, uCheck);
			break;
		case cbAnsiLog:
			OnBtn_AnsiLog(hDlg, CB, uCheck);
			break;
		case cbProcessNewConArg:
			OnBtn_ProcessNewConArg(hDlg, CB, uCheck);
			break;
		case cbProcessCmdStart:
			OnBtn_ProcessCmdStart(hDlg, CB, uCheck);
			break;
		case cbProcessCtrlZ:
			OnBtn_ProcessCtrlZ(hDlg, CB, uCheck);
			break;
		case cbSuppressBells:
			OnBtn_SuppressBells(hDlg, CB, uCheck);
			break;
		case cbConsoleExceptionHandler:
			OnBtn_ConsoleExceptionHandler(hDlg, CB, uCheck);
			break;
		case cbUseClink:
			OnBtn_UseClink(hDlg, CB, uCheck);
			break;
		case cbClinkWebPage:
			OnBtn_ClinkWebPage(hDlg, CB, uCheck);
			break;
		case bRealConsoleSettings:
			OnBtn_RealConsoleSettings(hDlg, CB, uCheck);
			break;
		case cbSnapToDesktopEdges:
			OnBtn_SnapToDesktopEdges(hDlg, CB, uCheck);
			break;
		case cbAlwaysOnTop:
			OnBtn_AlwaysOnTop(hDlg, CB, uCheck);
			break;
		case cbSleepInBackground:
			OnBtn_SleepInBackground(hDlg, CB, uCheck);
			break;
		case cbRetardInactivePanes:
			OnBtn_RetardInactivePanes(hDlg, CB, uCheck);
			break;
		case cbMinimizeOnLoseFocus:
			OnBtn_MinimizeOnLoseFocus(hDlg, CB, uCheck);
			break;
		case cbFocusInChildWindows:
			OnBtn_FocusInChildWindows(hDlg, CB, uCheck);
			break;
		case cbDisableFarFlashing:
			OnBtn_DisableFarFlashing(hDlg, CB, uCheck);
			break;
		case cbDisableAllFlashing:
			OnBtn_DisableAllFlashing(hDlg, CB, uCheck);
			break;
		case cbShowWasHiddenMsg:
			OnBtn_ShowWasHiddenMsg(hDlg, CB, uCheck);
			break;
		case cbShowWasSetOnTopMsg:
			OnBtn_ShowWasSetOnTopMsg(hDlg, CB, uCheck);
			break;
		case cbTabsInCaption:
			OnBtn_TabsInCaption(hDlg, CB, uCheck);
			break;
		case cbNumberInCaption:
			OnBtn_NumberInCaption(hDlg, CB, uCheck);
			break;
		case cbAdminShield:
		case cbAdminSuffix:
			OnBtn_AdminSuffixOrShield(hDlg, CB, uCheck);
			break;
		case cbHideInactiveConTabs:
			OnBtn_HideInactiveConTabs(hDlg, CB, uCheck);
			break;
		case cbHideDisabledTabs:
			OnBtn_HideDisabledTabs(hDlg, CB, uCheck);
			break;
		case cbShowFarWindows:
			OnBtn_ShowFarWindows(hDlg, CB, uCheck);
			break;
		case cbCloseConEmuWithLastTab:
		case cbCloseConEmuOnCrossClicking:
			OnBtn_CloseConEmuOptions(hDlg, CB, uCheck);
			break;
		case cbMinimizeOnLastTabClose:
		case cbHideOnLastTabClose:
			OnBtn_HideOrMinOnLastTabClose(hDlg, CB, uCheck);
			break;
		case rbMinByEscNever:
		case rbMinByEscEmpty:
		case rbMinByEscAlways:
			OnBtn_MinByEscRadio(hDlg, CB, uCheck);
			break;
		case cbMapShiftEscToEsc:
			OnBtn_MapShiftEscToEsc(hDlg, CB, uCheck);
			break;
		case cbGuiMacroHelp:
			OnBtn_GuiMacroHelp(hDlg, CB, uCheck);
			break;
		case cbUseWinArrows:
		case cbUseWinNumber:
		case cbUseWinTab:
			OnBtn_UseWinArrowNumTab(hDlg, CB, uCheck);
			break;

		//case cbUseWinNumber:
		//	gpSet->isUseWinNumber = isChecked(hDlg, cbUseWinNumber);
		//	if (hDlg) checkDlgButton(hDlg, cbUseWinNumberK, gpSet->isUseWinNumber ? BST_CHECKED : BST_UNCHECKED);
		//	gpConEmu->UpdateWinHookSettings();
		//	break;

		case cbSendAltTab:
		case cbSendAltEsc:
		case cbSendAltPrintScrn:
		case cbSendPrintScrn:
		case cbSendCtrlEsc:
			OnBtn_SendConsoleSpecials(hDlg, CB, uCheck);
			break;

		case cbInstallKeybHooks:
			OnBtn_InstallKeybHooks(hDlg, CB, uCheck);
			break;

		case cbDosBox:
			OnBtn_DosBox(hDlg, CB, uCheck);
			break;

		case bApplyViewSettings:
			OnBtn_ApplyViewSettings(hDlg, CB, uCheck);
			break;
		case cbThumbLoadFiles:
			OnBtn_ThumbLoadFiles(hDlg, CB, uCheck);
			break;
		case cbThumbLoadFolders:
			OnBtn_ThumbLoadFolders(hDlg, CB, uCheck);
			break;
		case cbThumbUsePicView2:
			OnBtn_ThumbUsePicView2(hDlg, CB, uCheck);
			break;
		case cbThumbRestoreOnStartup:
			OnBtn_ThumbRestoreOnStartup(hDlg, CB, uCheck);
			break;
		case cbThumbPreviewBox:
			OnBtn_ThumbPreviewBox(hDlg, CB, uCheck);
			break;
		case cbThumbSelectionBox:
			OnBtn_ThumbSelectionBox(hDlg, CB, uCheck);
			break;
		case rbThumbBackColorIdx: case rbThumbBackColorRGB:
			OnBtn_ThumbBackColors(hDlg, CB, uCheck);
			break;
		case rbThumbPreviewBoxColorIdx: case rbThumbPreviewBoxColorRGB:
			OnBtn_ThumbPreviewBoxColors(hDlg, CB, uCheck);
			break;
		case rbThumbSelectionBoxColorIdx: case rbThumbSelectionBoxColorRGB:
			OnBtn_ThumbSelectionBoxColors(hDlg, CB, uCheck);
			break;

		case cbActivityReset:
			OnBtn_ActivityReset(hDlg, CB, uCheck);
			break;
		case cbActivitySaveAs:
			OnBtn_ActivitySaveAs(hDlg, CB, uCheck);
			break;
		case rbActivityDisabled:
		case rbActivityShell:
		case rbActivityInput:
		case rbActivityCmd:
		case rbActivityAnsi:
			OnBtn_DebugActivityRadio(hDlg, CB, uCheck);
			break;

		case rbColorRgbDec:
		case rbColorRgbHex:
		case rbColorBgrHex:
			OnBtn_ColorFormat(hDlg, CB, uCheck);
			break;
		case cbExtendColors:
			OnBtn_ExtendColors(hDlg, CB, uCheck);
			break;
		case cbColorResetExt:
			OnBtn_ColorResetExt(hDlg, CB, uCheck);
			break;
		case cbColorSchemeSave:
		case cbColorSchemeDelete:
			OnBtn_ColorSchemeSaveDelete(hDlg, CB, uCheck);
			break;
		case cbTrueColorer:
			OnBtn_TrueColorer(hDlg, CB, uCheck);
			break;
		case cbFadeInactive:
			OnBtn_FadeInactive(hDlg, CB, uCheck);
			break;
		case cbTransparent:
			OnBtn_Transparent(hDlg, CB, uCheck);
			break;
		case cbTransparentSeparate:
			OnBtn_TransparentSeparate(hDlg, CB, uCheck);
			break;
		//case cbTransparentInactive:
		//	{
		//		int newV = gpSet->nTransparentInactive;
		//		if (isChecked(hDlg, cbTransparentInactive))
		//		{
		//			if (newV == MAX_ALPHA_VALUE) newV = 200;
		//		}
		//		else
		//		{
		//			newV = MAX_ALPHA_VALUE;
		//		}
		//		if (newV != gpSet->nTransparentInactive)
		//		{
		//			gpSet->nTransparentInactive = newV;
		//			SendDlgItemMessage(hDlg, slTransparentInactive, TBM_SETPOS, (WPARAM) true, (LPARAM)gpSet->nTransparentInactive);
		//			//gpConEmu->OnTransparent(); -- смысла нет, ConEmu сейчас "активен"
		//		}
		//	} break;
		case cbUserScreenTransparent:
			OnBtn_UserScreenTransparent(hDlg, CB, uCheck);
			break;
		case cbColorKeyTransparent:
			OnBtn_ColorKeyTransparent(hDlg, CB, uCheck);
			break;


		/* *** Text selections options *** */
		case cbCTSIntelligent:
			OnBtn_CTSIntelligent(hDlg, CB, uCheck);
			break;
		case rbCTSActAlways: case rbCTSActBufferOnly:
			OnBtn_CTSActConditionRadio(hDlg, CB, uCheck);
			break;
		case cbCTSFreezeBeforeSelect:
			OnBtn_CTSFreezeBeforeSelect(hDlg, CB, uCheck);
			break;
		case cbCTSAutoCopy:
			OnBtn_CTSAutoCopy(hDlg, CB, uCheck);
			break;
		case cbCTSResetOnRelease:
			OnBtn_CTSResetOnRelease(hDlg, CB, uCheck);
			break;
		case cbCTSIBeam:
			OnBtn_CTSIBeam(hDlg, CB, uCheck);
			break;
		case cbCTSEndOnTyping:
		case cbCTSEndCopyBefore:
			OnBtn_CTSEndCopyAuto(hDlg, CB, uCheck);
			break;
		case cbCTSEndOnKeyPress:
			OnBtn_CTSEndOnKeyPress(hDlg, CB, uCheck);
			break;
		case cbCTSEraseBeforeReset:
			OnBtn_CTSEraseBeforeReset(hDlg, CB, uCheck);
			break;
		case cbCTSBlockSelection:
			OnBtn_CTSBlockSelection(hDlg, CB, uCheck);
			break;
		case cbCTSTextSelection:
			OnBtn_CTSTextSelection(hDlg, CB, uCheck);
			break;
		case cbCTSDetectLineEnd:
			OnBtn_CTSDetectLineEnd(hDlg, CB, uCheck);
			break;
		case cbCTSBashMargin:
			OnBtn_CTSBashMargin(hDlg, CB, uCheck);
			break;
		case cbCTSTrimTrailing:
			OnBtn_CTSTrimTrailing(hDlg, CB, uCheck);
			break;
		case cbCTSClickPromptPosition:
			OnBtn_CTSClickPromptPosition(hDlg, CB, uCheck);
			break;
		case cbCTSDeleteLeftWord:
			OnBtn_CTSDeleteLeftWord(hDlg, CB, uCheck);
			break;
		case cbCTSShiftArrowStartSel:
			OnBtn_CTSShiftArrowStartSel(hDlg, CB, uCheck);
			break;
		case rPasteM1MultiLine:
		case rPasteM1FirstLine:
		case rPasteM1SingleLine:
		case rPasteM1Nothing:
			CSetPgPaste::OnBtn_ClipShiftIns(hDlg, CB, uCheck);
			break;
		case rPasteM2MultiLine:
		case rPasteM2FirstLine:
		case rPasteM2SingleLine:
		case rPasteM2Nothing:
			CSetPgPaste::OnBtn_ClipCtrlV(hDlg, CB, uCheck);
			break;
		case cbClipConfirmEnter:
			CSetPgPaste::OnBtn_ClipConfirmEnter(hDlg, CB, uCheck);
			break;
		case cbClipConfirmLimit:
			CSetPgPaste::OnBtn_ClipConfirmLimit(hDlg, CB, uCheck);
			break;
		case cbFarGotoEditor:
			OnBtn_FarGotoEditor(hDlg, CB, uCheck);
			break;
		case cbHighlightMouseRow:
			OnBtn_HighlightMouseRow(hDlg, CB, uCheck);
			break;
		case cbHighlightMouseCol:
			OnBtn_HighlightMouseCol(hDlg, CB, uCheck);
			break;
		/* *** Text selections options *** */



		/* *** Update settings *** */
		case cbUpdateCheckOnStartup:
			OnBtn_UpdateCheckOnStartup(hDlg, CB, uCheck);
			break;
		case cbUpdateCheckHourly:
			OnBtn_UpdateCheckHourly(hDlg, CB, uCheck);
			break;
		case cbUpdateConfirmDownload:
			OnBtn_UpdateConfirmDownload(hDlg, CB, uCheck);
			break;
		case rbUpdateStableOnly:
		case rbUpdatePreview:
		case rbUpdateLatestAvailable:
			OnBtn_UpdateTypeRadio(hDlg, CB, uCheck);
			break;
		case cbUpdateInetTool:
			OnBtn_UpdateInetTool(hDlg, CB, uCheck);
			break;
		case cbUpdateInetToolPath:
			OnBtn_UpdateInetToolCmd(hDlg, CB, uCheck);
			break;
		case cbUpdateUseProxy:
			OnBtn_UpdateUseProxy(hDlg, CB, uCheck);
			break;
		case cbUpdateLeavePackages:
			OnBtn_UpdateLeavePackages(hDlg, CB, uCheck);
			break;
		case cbUpdateArcCmdLine:
			OnBtn_UpdateArcCmdLine(hDlg, CB, uCheck);
			break;
		case cbUpdateDownloadPath:
			OnBtn_UpdateDownloadPath(hDlg, CB, uCheck);
			break;
		case cbUpdateCheck:
			OnBtn_UpdateApplyAndCheck(hDlg, CB, uCheck);
			break;
		/* *** Update settings *** */

		/* *** Command groups *** */
		case cbCmdGrpDefaultNew:
		case cbCmdGrpDefaultCmd:
		case cbCmdGrpTaskbar:
		case cbCmdGrpToolbar:
			OnBtn_CmdTasksFlags(hDlg, CB, uCheck);
			break;
		case cbCmdTasksAdd:
			OnBtn_CmdTasksAdd(hDlg, CB, uCheck);
			break;
		case cbCmdTasksDel:
			OnBtn_CmdTasksDel(hDlg, CB, uCheck);
			break;
		case cbCmdTasksUp:
		case cbCmdTasksDown:
			OnBtn_CmdTasksUpDown(hDlg, CB, uCheck);
			break;
		case cbCmdGroupKey:
			OnBtn_CmdGroupKey(hDlg, CB, uCheck);
			break;
		case cbCmdGroupApp:
			OnBtn_CmdGroupApp(hDlg, CB, uCheck);
			break;
		case cbCmdTasksParm:
			OnBtn_CmdTasksParm(hDlg, CB, uCheck);
			break;
		case cbCmdTasksDir:
			OnBtn_CmdTasksDir(hDlg, CB, uCheck);
			break;
		case cbCmdTasksActive:
			OnBtn_CmdTasksActive(hDlg, CB, uCheck);
			break;
		case cbCmdTasksReload:
			OnBtn_CmdTasksReload(hDlg, CB, uCheck);
			break;
		case cbAddDefaults:
			OnBtn_AddDefaults(hDlg, CB, uCheck);
			break;
		case cbCmdTaskbarTasks:
			OnBtn_CmdTaskbarTasks(hDlg, CB, uCheck);
			break;
		case cbCmdTaskbarCommands:
			OnBtn_CmdTaskbarCommands(hDlg, CB, uCheck);
			break;
		case cbCmdTaskbarUpdate:
			OnBtn_CmdTaskbarUpdate(hDlg, CB, uCheck);
			break;
		case stCmdGroupCommands:
			break; // если нужен тултип для StaticText - нужен стиль SS_NOTIFY, а тогда нужно этот ID просто пропустить, чтобы ассерта не было
		/* *** Command groups *** */


		/* *** Default terminal *** */
		case cbDefaultTerminal:
		case cbDefaultTerminalStartup:
		case cbDefaultTerminalTSA:
		case cbDefTermAgressive:
		case cbDefaultTerminalNoInjects:
		case cbDefaultTerminalUseExisting:
		case cbDefaultTerminalDebugLog:
		case cbApplyDefTerm:
		case rbDefaultTerminalConfAuto:
		case rbDefaultTerminalConfAlways:
		case rbDefaultTerminalConfNever:
			OnBtn_DefTerm(hDlg, CB, uCheck);
			break;
		/* *** Default terminal *** */


		case bGotoEditorCmd:
			OnBtn_GotoEditorCmd(hDlg, CB, uCheck);
			break;

		case c0:  case c1:  case c2:  case c3:  case c4:  case c5:  case c6:  case c7:
		case c8:  case c9:  case c10: case c11: case c12: case c13: case c14: case c15:
		case c16: case c17: case c18: case c19: case c20: case c21: case c22: case c23:
		case c24: case c25: case c26: case c27: case c28: case c29: case c30: case c31:
		case c32: case c33: case c34: case c35: case c36: case c37: case c38:
			//OnBtn_ColorField(hDlg, CB, uCheck);
			break;

		/* *** Status bar options *** */
		case cbShowStatusBar:
			OnBtn_ShowStatusBar(hDlg, CB, uCheck);
			break;
		case cbStatusVertSep:
			OnBtn_StatusVertSep(hDlg, CB, uCheck);
			break;
		case cbStatusHorzSep:
			OnBtn_StatusHorzSep(hDlg, CB, uCheck);
			break;
		case cbStatusVertPad:
			OnBtn_StatusVertPad(hDlg, CB, uCheck);
			break;
		case cbStatusSystemColors:
			OnBtn_StatusSystemColors(hDlg, CB, uCheck);
			break;
		case cbStatusAddAll:
		case cbStatusAddSelected:
		case cbStatusDelSelected:
		case cbStatusDelAll:
			OnBtn_StatusAddDel(hDlg, CB, uCheck);
			break;

		case cbHotkeysAssignedOnly:
			//TODO: Move to CSetPgKeys::PageDlgProc
			if (gpSetCls->GetActivePageObj() && (gpSetCls->GetActivePageObj()->GetPageType() == thi_Keys))
			{
				dynamic_cast<CSetPgKeys*>(gpSetCls->GetActivePageObj())->RefilterHotkeys();
			}
			break;

		default:
			if (CDlgItemHelper::isHyperlinkCtrl(CB))
			{
				CDlgItemHelper::ProcessHyperlinkCtrl(hDlg, CB);
				break;
			}
			Assert(FALSE && "Button click was not processed");
			bProcessed = false;
	}

	return bProcessed;
}

// rCursorH ... cbInactiveCursorIgnoreSize
void CSetDlgButtons::OnButtonClicked_Cursor(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp)
{
	switch (CB)
	{
	case rCursorH:
	case rCursorV:
	case rCursorB:
	case rCursorR:
		OnBtn_CursorX(hDlg, CB, uCheck, pApp);
		break;
	case cbCursorColor:
		OnBtn_CursorColor(hDlg, CB, uCheck, pApp);
		break;
	case cbCursorBlink:
		OnBtn_CursorBlink(hDlg, CB, uCheck, pApp);
		break;
	case cbCursorIgnoreSize:
		OnBtn_CursorIgnoreSize(hDlg, CB, uCheck, pApp);
		break;
	case cbInactiveCursor:
		OnBtn_InactiveCursor(hDlg, CB, uCheck, pApp);
		break;
	case rInactiveCursorH:
	case rInactiveCursorV:
	case rInactiveCursorB:
	case rInactiveCursorR:
		OnBtn_InactiveCursorX(hDlg, CB, uCheck, pApp);
		break;
	case cbInactiveCursorColor:
		OnBtn_InactiveCursorColor(hDlg, CB, uCheck, pApp);
		break;
	case cbInactiveCursorBlink:
		OnBtn_InactiveCursorBlink(hDlg, CB, uCheck, pApp);
		break;
	case cbInactiveCursorIgnoreSize:
		OnBtn_InactiveCursorIgnoreSize(hDlg, CB, uCheck, pApp);
		break;

	default:
		_ASSERT(FALSE && "Not handled");
		return;
	}
} // rCursorH ... cbInactiveCursorIgnoreSize

// Service function for GuiMacro::SetOption
wchar_t* CSetDlgButtons::CheckButtonMacro(WORD CB, BYTE uCheck)
{
	if (uCheck > BST_INDETERMINATE)
	{
		_ASSERTE(uCheck==BST_UNCHECKED || uCheck==BST_CHECKED || uCheck==BST_INDETERMINATE);
		return lstrdup(L"InvalidValue");
	}

	HWND hDlg, hBtn = NULL;
	wchar_t szClass[32] = L"";

	// If settings dialog is active - check if current page has CB
	if ((hDlg = gpSetCls->GetActivePage()) != NULL)
	{
		if ((hBtn = GetDlgItem(hDlg, CB)) != NULL)
		{
			GetClassName(hBtn, szClass, countof(szClass));
			if (lstrcmpi(szClass, L"Button") != 0)
			{
				hDlg = hBtn = NULL;
			}
			else
			{
				checkDlgButton(hBtn, 0, uCheck);
			}
		}
		else
		{
			hDlg = NULL;
		}
	}

	if (!ProcessButtonClick(hDlg, CB, uCheck))
	{
		return lstrdup(L"InvalidID");
	}

	return lstrdup(L"OK");
}

// Called from CSettings dialog
LRESULT CSetDlgButtons::OnButtonClicked(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	_ASSERTE(hDlg!=NULL);
	WORD CB = LOWORD(wParam);
	BYTE uCheck = isChecked(hDlg, CB);

	ProcessButtonClick(hDlg, CB, uCheck);

	return 0;
}



/* *********************************************************** */
/*                                                             */
/*                                                             */
/*                                                             */
/* *********************************************************** */

// rCursorH || rCursorV || rCursorB || rCursorR
void CSetDlgButtons::OnBtn_CursorX(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp)
{
	_ASSERTE(CB==rCursorH || CB==rCursorV || CB==rCursorB || CB==rCursorR);

	pApp->CursorActive.CursorType = (CECursorStyle)(CB - rCursorH);

} // rCursorR


// cbCursorColor
void CSetDlgButtons::OnBtn_CursorColor(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp)
{
	_ASSERTE(CB==cbCursorColor);

	pApp->CursorActive.isColor = uCheck;

} // cbCursorColor


// cbCursorBlink
void CSetDlgButtons::OnBtn_CursorBlink(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp)
{
	_ASSERTE(CB==cbCursorBlink);

	pApp->CursorActive.isBlinking = uCheck;

} // cbCursorBlink


// cbCursorIgnoreSize
void CSetDlgButtons::OnBtn_CursorIgnoreSize(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp)
{
	_ASSERTE(CB==cbCursorIgnoreSize);

	pApp->CursorActive.isFixedSize = uCheck;

} // cbCursorIgnoreSize


// cbInactiveCursor
void CSetDlgButtons::OnBtn_InactiveCursor(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp)
{
	_ASSERTE(CB==cbInactiveCursor);

	pApp->CursorInactive.Used = uCheck;
} // cbInactiveCursor


// rInactiveCursorH || rInactiveCursorV || rInactiveCursorB || rInactiveCursorR
void CSetDlgButtons::OnBtn_InactiveCursorX(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp)
{
	_ASSERTE(CB==rInactiveCursorH || CB==rInactiveCursorV || CB==rInactiveCursorB || CB==rInactiveCursorR);

	pApp->CursorInactive.CursorType = (CECursorStyle)(CB - rInactiveCursorH);

} // rInactiveCursorR


// cbInactiveCursorColor
void CSetDlgButtons::OnBtn_InactiveCursorColor(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp)
{
	_ASSERTE(CB==cbInactiveCursorColor);

	pApp->CursorInactive.isColor = uCheck;

} // cbInactiveCursorColor


// cbInactiveCursorBlink
void CSetDlgButtons::OnBtn_InactiveCursorBlink(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp)
{
	_ASSERTE(CB==cbInactiveCursorBlink);

	pApp->CursorInactive.isBlinking = uCheck;

} // cbInactiveCursorBlink


// cbInactiveCursorIgnoreSize
void CSetDlgButtons::OnBtn_InactiveCursorIgnoreSize(HWND hDlg, WORD CB, BYTE uCheck, AppSettings* pApp)
{
	_ASSERTE(CB==cbInactiveCursorIgnoreSize);

	pApp->CursorInactive.isFixedSize = uCheck;

} // cbInactiveCursorIgnoreSize


// cbCmdGrpDefaultNew, cbCmdGrpDefaultCmd, cbCmdGrpTaskbar, cbCmdGrpToolbar
void CSetDlgButtons::OnBtn_CmdTasksFlags(HWND hDlg, WORD CB, BYTE uCheck)
{
	// Only visual mode is supported
	if (!hDlg)
		return;

	int iCur = CSetDlgLists::GetListboxCurSel(hDlg, lbCmdTasks, true);
	if (iCur < 0)
		return;

	CommandTasks* p = (CommandTasks*)gpSet->CmdTaskGet(iCur);
	if (!p)
		return;

	bool bDistinct = false;
	CETASKFLAGS flag = CETF_NONE;

	switch (CB)
	{
	case cbCmdGrpDefaultNew:
		flag = CETF_NEW_DEFAULT;
		if (isChecked(hDlg, CB))
		{
			bDistinct = true;
			p->Flags |= flag;
		}
		else
		{
			p->Flags &= ~flag;
		}
		break;
	case cbCmdGrpDefaultCmd:
		flag = CETF_CMD_DEFAULT;
		if (isChecked(hDlg, CB))
		{
			bDistinct = true;
			p->Flags |= flag;
		}
		else
		{
			p->Flags &= ~flag;
		}
		break;
	case cbCmdGrpTaskbar:
		if (isChecked(hDlg, CB))
			p->Flags &= ~CETF_NO_TASKBAR;
		else
			p->Flags |= CETF_NO_TASKBAR;
		break;
	case cbCmdGrpToolbar:
		if (isChecked(hDlg, CB))
			p->Flags |= CETF_ADD_TOOLBAR;
		else
			p->Flags &= ~CETF_ADD_TOOLBAR;
		break;
	}

	if (bDistinct)
	{
		int nGroup = 0;
		CommandTasks* pGrp = NULL;
		while ((pGrp = (CommandTasks*)gpSet->CmdTaskGet(nGroup++)))
		{
			if (pGrp != p)
				pGrp->Flags &= ~flag;
		}
	}

} // cbCmdGrpDefaultNew, cbCmdGrpDefaultCmd, cbCmdGrpTaskbar, cbCmdGrpToolbar


// cbCmdTasksAdd
void CSetDlgButtons::OnBtn_CmdTasksAdd(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCmdTasksAdd);

	int iCount = (int)SendDlgItemMessage(hDlg, lbCmdTasks, LB_GETCOUNT, 0,0);
	if (iCount < 0)
		return;
	if (!gpSet->CmdTaskGet(iCount))
		gpSet->CmdTaskSet(iCount, L"", L"", L"");

	CSetPgTasks* pTasksPg;
	if (gpSetCls->GetPageObj(pTasksPg))
		pTasksPg->OnInitDialog(hDlg, false);

	CSetDlgLists::ListBoxMultiSel(hDlg, lbCmdTasks, iCount);

	if (pTasksPg)
		pTasksPg->OnComboBox(hDlg, lbCmdTasks, LBN_SELCHANGE);

} // cbCmdTasksAdd


// cbCmdTasksDel
void CSetDlgButtons::OnBtn_CmdTasksDel(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCmdTasksDel);

	int *Selected = NULL, iCount = CSetDlgLists::GetListboxSelection(hDlg, lbCmdTasks, Selected);
	if (iCount <= 0)
		return;

	const CommandTasks* p = NULL;
	bool bIsStartup = false;

	for (INT_PTR i = iCount-1; i >= 0; i--)
	{
		p = gpSet->CmdTaskGet(Selected[iCount-1]);
		if (!p)
		{
			_ASSERTE(FALSE && "Failed to get selected task");
			delete[] Selected;
			return;
		}

		if (gpSet->psStartTasksName && p->pszName && (lstrcmpi(gpSet->psStartTasksName, p->pszName) == 0))
			bIsStartup = true;
	}

	size_t cchMax = (p->pszName ? _tcslen(p->pszName) : 0) + 200;
	wchar_t* pszMsg = (wchar_t*)malloc(cchMax*sizeof(*pszMsg));
	if (!pszMsg)
	{
		delete[] Selected;
		return;
	}

	wchar_t szOthers[64] = L"";
	if (iCount > 1)
		_wsprintf(szOthers, SKIPCOUNT(szOthers) L"\n" L"and %i other task(s)", (iCount-1));

	_wsprintf(pszMsg, SKIPLEN(cchMax) L"%sDelete command group\n%s%s?",
		bIsStartup ? L"Warning! You about to delete startup task!\n\n" : L"",
		p->pszName ? p->pszName : L"{???}",
		szOthers);

	int nBtn = MsgBox(pszMsg, MB_YESNO|(bIsStartup ? MB_ICONEXCLAMATION : MB_ICONQUESTION), NULL, ghOpWnd);
	SafeFree(pszMsg);

	if (nBtn != IDYES)
	{
		delete[] Selected;
		return;
	}

	for (INT_PTR i = iCount-1; i >= 0; i--)
	{
		gpSet->CmdTaskSet(Selected[i], NULL, NULL, NULL);

		if (bIsStartup && gpSet->psStartTasksName)
			*gpSet->psStartTasksName = 0;
	}

	CSetPgTasks* pTasksPg;
	if (gpSetCls->GetPageObj(pTasksPg))
		pTasksPg->OnInitDialog(hDlg, false);

	iCount = (int)SendDlgItemMessage(hDlg, lbCmdTasks, LB_GETCOUNT, 0,0);
	CSetDlgLists::ListBoxMultiSel(hDlg, lbCmdTasks, min(Selected[0],(iCount-1)));

	if (pTasksPg)
		pTasksPg->OnComboBox(hDlg, lbCmdTasks, LBN_SELCHANGE);

	delete[] Selected;

} // cbCmdTasksDel


// cbCmdTasksUp || cbCmdTasksDown
void CSetDlgButtons::OnBtn_CmdTasksUpDown(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCmdTasksUp || CB==cbCmdTasksDown);

	int iCur, iChg;
	iCur = CSetDlgLists::GetListboxCurSel(hDlg, lbCmdTasks, true);
	if (iCur < 0)
		return;
	if (CB == cbCmdTasksUp)
	{
		if (!iCur)
			return;
		iChg = iCur - 1;
	}
	else
	{
		iChg = iCur + 1;
		if (iChg >= (int)SendDlgItemMessage(hDlg, lbCmdTasks, LB_GETCOUNT, 0,0))
			return;
	}

	if (!gpSet->CmdTaskXch(iCur, iChg))
		return;

	CSetPgTasks* pTasksPg;
	if (gpSetCls->GetPageObj(pTasksPg))
		pTasksPg->OnInitDialog(hDlg, false);

	CSetDlgLists::ListBoxMultiSel(hDlg, lbCmdTasks, iChg);

	if (pTasksPg)
		pTasksPg->OnComboBox(hDlg, lbCmdTasks, LBN_SELCHANGE);

} // cbCmdTasksUp || cbCmdTasksDown


// cbCmdGroupKey
void CSetDlgButtons::OnBtn_CmdGroupKey(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCmdGroupKey);

	int iCur = CSetDlgLists::GetListboxCurSel(hDlg, lbCmdTasks, true);
	if (iCur < 0)
		return;
	const CommandTasks* pCmd = gpSet->CmdTaskGet(iCur);
	if (!pCmd)
		return;

	DWORD VkMod = pCmd->HotKey.GetVkMod();
	if (CHotKeyDialog::EditHotKey(ghOpWnd, VkMod))
	{
		gpSet->CmdTaskSetVkMod(iCur, VkMod);
		wchar_t szKey[128] = L"";
		SetDlgItemText(hDlg, tCmdGroupKey, ConEmuHotKey::GetHotkeyName(pCmd->HotKey.GetVkMod(), szKey));
	}
} // cbCmdGroupKey


// cbCmdGroupApp
void CSetDlgButtons::OnBtn_CmdGroupApp(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCmdGroupApp);

	// Добавить команду в группу
	RConStartArgs args;
	args.aRecreate = cra_EditTab;
	int nDlgRc = gpConEmu->RecreateDlg(&args);

	if (nDlgRc == IDC_START)
	{
		wchar_t* pszCmd = args.CreateCommandLine();
		if (!pszCmd || !*pszCmd)
		{
			DisplayLastError(L"Can't compile command line for new tab\nAll fields are empty?", -1);
		}
		else
		{
			//SendDlgItemMessage(hDlg, tCmdGroupCommands, EM_REPLACESEL, TRUE, (LPARAM)pszName);
			wchar_t* pszFull = GetDlgItemTextPtr(hDlg, tCmdGroupCommands);
			if (!pszFull || !*pszFull)
			{
				SafeFree(pszFull);
				pszFull = pszCmd;
			}
			else
			{
				size_t cchLen = _tcslen(pszFull);
				size_t cchMax = cchLen + 7 + _tcslen(pszCmd);
				pszFull = (wchar_t*)realloc(pszFull, cchMax*sizeof(*pszFull));

				int nRN = 0;
				if (cchLen >= 2)
				{
					if (pszFull[cchLen-2] == L'\r' && pszFull[cchLen-1] == L'\n')
					{
						nRN++;
						if (cchLen >= 4)
						{
							if (pszFull[cchLen-4] == L'\r' && pszFull[cchLen-3] == L'\n')
							{
								nRN++;
							}
						}
					}
				}

				if (nRN < 2)
					_wcscat_c(pszFull, cchMax, nRN ? L"\r\n" : L"\r\n\r\n");

				_wcscat_c(pszFull, cchMax, pszCmd);
			}

			if (pszFull)
			{
				SetDlgItemText(hDlg, tCmdGroupCommands, pszFull);
				CSetPgTasks* pTasksPg;
				if (gpSetCls->GetPageObj(pTasksPg))
					pTasksPg->OnEditChanged(hDlg, tCmdGroupCommands);
			}
			else
			{
				_ASSERTE(pszFull);
			}
			if (pszCmd == pszFull)
				pszCmd = NULL;
			SafeFree(pszFull);
		}
		SafeFree(pszCmd);
	}
} // cbCmdGroupApp


// cbCmdTasksParm
void CSetDlgButtons::OnBtn_CmdTasksParm(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCmdTasksParm);

	// Добавить файл
	wchar_t temp[MAX_PATH+10] = {};
	OPENFILENAME ofn = {sizeof(ofn)};
	ofn.hwndOwner = ghOpWnd;
	ofn.lpstrFilter = L"All files (*.*)\0*.*\0Text files (*.txt,*.ini,*.log)\0*.txt;*.ini;*.log\0Executables (*.exe,*.com,*.bat,*.cmd)\0*.exe;*.com;*.bat;*.cmd\0Scripts (*.vbs,*.vbe,*.js,*.jse)\0*.vbs;*.vbe;*.js;*.jse\0\0";
	//ofn.lpstrFilter = L"All files (*.*)\0*.*\0\0";
	ofn.lpstrFile = temp+1;
	ofn.nMaxFile = countof(temp)-10;
	ofn.lpstrTitle = L"Choose file";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
	            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn))
	{
		LPCWSTR pszName = temp+1;
		if (wcschr(pszName, L' '))
		{
			temp[0] = L'"';
			wcscat_c(temp, L"\"");
			pszName = temp;
		}
		else
		{
			temp[0] = L' ';
		}
		//wcscat_c(temp, L"\r\n\r\n");

		SendDlgItemMessage(hDlg, tCmdGroupCommands, EM_REPLACESEL, TRUE, (LPARAM)pszName);
	}
} // cbCmdTasksParm


// cbCmdTasksDir
void CSetDlgButtons::OnBtn_CmdTasksDir(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCmdTasksDir);

	TODO("Извлечь текущий каталог запуска");

	BROWSEINFO bi = {ghOpWnd};
	wchar_t szFolder[MAX_PATH+1] = {0};
	TODO("Извлечь текущий каталог запуска");
	bi.pszDisplayName = szFolder;
	wchar_t szTitle[100];
	bi.lpszTitle = wcscpy(szTitle, L"Choose tab startup directory");
	bi.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS | BIF_VALIDATE;
	bi.lpfn = CRecreateDlg::BrowseCallbackProc;
	bi.lParam = (LPARAM)szFolder;
	LPITEMIDLIST pRc = SHBrowseForFolder(&bi);

	if (pRc)
	{
		if (SHGetPathFromIDList(pRc, szFolder))
		{
			wchar_t szFull[MAX_PATH+32];
			bool bQuot = wcschr(szFolder, L' ') != NULL;
			wcscpy_c(szFull, bQuot ? L" \"-new_console:d:" : L" -new_console:d:");
			wcscat_c(szFull, szFolder);
			if (bQuot)
				wcscat_c(szFull, L"\"");

			SendDlgItemMessage(hDlg, tCmdGroupCommands, EM_REPLACESEL, TRUE, (LPARAM)szFull);
		}

		CoTaskMemFree(pRc);
	}
} // cbCmdTasksDir

// cbCmdTasksActive
void CSetDlgButtons::OnBtn_CmdTasksActive(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCmdTasksActive);

	wchar_t* pszTasks = CVConGroup::GetTasks(NULL); // вернуть все открытые таски
	if (pszTasks)
	{
		SendDlgItemMessage(hDlg, tCmdGroupCommands, EM_REPLACESEL, TRUE, (LPARAM)pszTasks);
		SafeFree(pszTasks);
	}
} // cbCmdTasksActive


// cbAddDefaults
void CSetDlgButtons::OnBtn_AddDefaults(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbAddDefaults);

	int iBtn = MsgBox(
		L"Do you want to ADD NEW default tasks in your task list?\n\n"
		L"Choose <No> to REWRITE EXISTING tasks with defaults too."
		, MB_YESNOCANCEL|MB_ICONEXCLAMATION, gpConEmu->GetDefaultTitle(), ghOpWnd);
	if (iBtn == IDCANCEL)
		return;

	// Append or rewrite default tasks
	CreateDefaultTasks(slf_AppendTasks|((iBtn == IDNO) ? slf_RewriteExisting : slf_None));

	// Обновить список на экране
	CSetPgTasks* pTasksPg;
	if (gpSetCls->GetPageObj(pTasksPg))
		pTasksPg->OnInitDialog(hDlg, true);

} // cbAddDefaults

// cbCmdTasksReload
void CSetDlgButtons::OnBtn_CmdTasksReload(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCmdTasksReload);

	if (MsgBox(L"Warning! All unsaved changes will be lost!\n\nReload command groups from settings?",
			MB_YESNO|MB_ICONEXCLAMATION, gpConEmu->GetDefaultTitle(), ghOpWnd) != IDYES)
		return;

	// Обновить группы команд
	gpSet->LoadCmdTasks(NULL, true);

	// Обновить список на экране
	CSetPgTasks* pTasksPg;
	if (gpSetCls->GetPageObj(pTasksPg))
		pTasksPg->OnInitDialog(hDlg, true);

} // cbCmdTasksReload

// cbCmdTaskbarTasks
void CSetDlgButtons::OnBtn_CmdTaskbarTasks(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCmdTaskbarTasks);

	gpSet->isStoreTaskbarkTasks = uCheck;

} // cbCmdTaskbarTasks

// cbCmdTaskbarCommands
void CSetDlgButtons::OnBtn_CmdTaskbarCommands(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCmdTaskbarCommands);

	gpSet->isStoreTaskbarCommands = uCheck;

} // cbCmdTaskbarCommands


// cbCmdTaskbarUpdate - Находится в IDD_SPG_TASKBAR!
void CSetDlgButtons::OnBtn_CmdTaskbarUpdate(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCmdTaskbarUpdate);

	if (!gpSet->SaveCmdTasks(NULL))
	{
		LPCWSTR pszMsg = L"Can't save task list to settings!\r\nJump list may be not working!\r\nUpdate Windows 7 task list now?";
		if (MsgBox(pszMsg, MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2, gpConEmu->GetDefaultTitle(), ghOpWnd) != IDYES)
			return; // Обновлять таскбар не будем
	}

	UpdateWin7TaskList(true);

} // cbCmdTaskbarUpdate


// rNoneAA || rStandardAA || rCTAA
void CSetDlgButtons::OnBtn_NoneStandardCleartype(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rNoneAA || CB==rStandardAA || CB==rCTAA);

	PostMessage(hDlg, gpSetCls->mn_MsgRecreateFont, CB, 0);

} // rNoneAA || rStandardAA || rCTAA


// rNormal || rFullScreen || rMaximized
void CSetDlgButtons::OnBtn_NormalFullscreenMaximized(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rNormal || CB==rFullScreen || CB==rMaximized);

	EnableWindow(GetDlgItem(hDlg, cbApplyPos), TRUE);
	CSetDlgLists::EnableDlgItems(hDlg, CSetDlgLists::eSizeCtrlId, CB == rNormal);

} // rNormal || rFullScreen || rMaximized


// cbApplyPos
void CSetDlgButtons::OnBtn_ApplyPos(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbApplyPos);

	if (!gpConEmu->mp_Inside)
	{
		bool bStored = false;
		ConEmuWindowMode Mode = (ConEmuWindowMode)gpSet->_WindowMode;
		if (hDlg)
		{
			if (isChecked(hDlg, rNormal))
				Mode = wmNormal;
			else if (isChecked(hDlg, rMaximized))
				Mode = wmMaximized;
			else if (isChecked(hDlg, rFullScreen))
				Mode = wmFullScreen;
		}

		#if 0
		if (gpSet->isQuakeStyle)
		{
			RECT rcWnd = gpConEmu->GetDefaultRect();
			//gpConEmu->SetWindowMode((ConEmuWindowMode)CB);
			SetWindowPos(ghWnd, NULL, rcWnd.left, rcWnd.top, rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top, SWP_NOZORDER);
			apiSetForegroundWindow(ghOpWnd);
		}
		#endif

		if (Mode == wmNormal)
		{
			CEStr psX(GetDlgItemTextPtr(hDlg, tWndX));
			CEStr psY(GetDlgItemTextPtr(hDlg, tWndY));
			CEStr psW(GetDlgItemTextPtr(hDlg, tWndWidth));
			CEStr psH(GetDlgItemTextPtr(hDlg, tWndHeight));

			if (!gpConEmu->isWindowNormal())
				gpConEmu->SetWindowMode(wmNormal);

			bStored = gpConEmu->SetWindowPosSize(psX, psY, psW, psH);
		}
		else if (Mode == wmMaximized)
		{
			SetFocus(GetDlgItem(hDlg, rMaximized));

			if (!gpConEmu->isZoomed())
				gpConEmu->SetWindowMode(wmMaximized);
		}
		else if (Mode == wmFullScreen)
		{
			SetFocus(GetDlgItem(hDlg, rFullScreen));

			if (!gpConEmu->isFullScreen())
				gpConEmu->SetWindowMode(wmFullScreen);
		}

		// Запомнить "идеальный" размер окна, выбранный пользователем
		if (!bStored)
			gpConEmu->StoreIdealRect();
		//gpConEmu->UpdateIdealRect(TRUE);

		EnableWindow(GetDlgItem(hDlg, cbApplyPos), FALSE);
		apiSetForegroundWindow(ghOpWnd);
	}
} // cbApplyPos


// rCascade || rFixed
void CSetDlgButtons::OnBtn_CascadeFixed(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rCascade || CB==rFixed);

	gpSet->wndCascade = (CB == rCascade);
	if (hDlg && gpSet->isQuakeStyle)
	{
		CSetPgSizePos* pObj;
		if (gpSetCls->GetPageObj(pObj))
		{
			pObj->EnablePosSizeControls(hDlg);
			enableDlgItem(hDlg, cbApplyPos, true);
		}
	}
} // rCascade || rFixed


// cbUseCurrentSizePos
void CSetDlgButtons::OnBtn_UseCurrentSizePos(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUseCurrentSizePos);

	gpSet->isUseCurrentSizePos = uCheck;
	if (gpSet->isUseCurrentSizePos)
	{
		gpSetCls->UpdateWindowMode(gpConEmu->WindowMode);
		gpSetCls->UpdatePos(gpConEmu->wndX, gpConEmu->wndY, true);
		gpSetCls->UpdateSize(gpConEmu->WndWidth, gpConEmu->WndHeight);
	}
} // cbUseCurrentSizePos


// cbAutoSaveSizePos
void CSetDlgButtons::OnBtn_AutoSaveSizePos(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbAutoSaveSizePos);

	gpSet->isAutoSaveSizePos = uCheck;

} // cbAutoSaveSizePos


// cbFontAuto
void CSetDlgButtons::OnBtn_FontAuto(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbFontAuto);

	gpSet->isFontAutoSize = uCheck;

	if (gpSet->isFontAutoSize && gpFontMgr->LogFont.lfFaceName[0] == L'['
	        && !wcsncmp(gpFontMgr->LogFont.lfFaceName+1, CFontMgr::RASTER_FONTS_NAME, _tcslen(CFontMgr::RASTER_FONTS_NAME)))
	{
		gpSet->isFontAutoSize = false;
		checkDlgButton(hDlg, cbFontAuto, BST_UNCHECKED);
		gpSetCls->ShowFontErrorTip(gpFontMgr->szRasterAutoError);
	}
} // cbFontAuto


// cbFixFarBorders
void CSetDlgButtons::OnBtn_FixFarBorders(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbFixFarBorders);

	gpSet->isFixFarBorders = (uCheck == BST_UNCHECKED) ? 0 : 1;
	gpFontMgr->ResetFontWidth();
	gpConEmu->Update(true);

} // cbFixFarBorders


// cbFont2AA
void CSetDlgButtons::OnBtn_AntiAliasing2(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbFont2AA);

	gpSet->isAntiAlias2 = (uCheck != BST_UNCHECKED);

	PostMessage(hDlg, gpSetCls->mn_MsgRecreateFont, CB, 0);

} // cbFont2AA


// cbUnicodeRangesApply
void CSetDlgButtons::OnBtn_UnicodeRangesApply(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUnicodeRangesApply);

	wchar_t* pszNameAndRange = GetDlgItemTextPtr(hDlg, tUnicodeRanges);
	LPCWSTR pszRanges = pszNameAndRange ? wcsstr(pszNameAndRange, L": ") : NULL;
	if (pszRanges) pszRanges += 2; else pszRanges = L"";
	gpSet->ParseCharRanges(pszRanges, gpSet->mpc_CharAltFontRanges);
	SafeFree(pszNameAndRange);

	if (gpSet->isFixFarBorders)
	{
		gpFontMgr->ResetFontWidth();
		gpConEmu->Update(true);
	}
} // cbUnicodeRangesApply


// cbSingleInstance
void CSetDlgButtons::OnBtn_SingleInstance(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbSingleInstance);

	gpSet->isSingleInstance = uCheck;

} // cbSingleInstance


// cbShowHelpTooltips
void CSetDlgButtons::OnBtn_ShowHelpTooltips(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbShowHelpTooltips);

	gpSet->isShowHelpTooltips = uCheck;


} // cbShowHelpTooltips



// cbMultiCon
void CSetDlgButtons::OnBtn_MultiCon(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbMultiCon);

	gpSet->mb_isMulti = uCheck;
	gpConEmu->UpdateWinHookSettings();

} // cbMultiCon


// cbMultiShowButtons, cbMultiShowSearch
void CSetDlgButtons::OnBtn_MultiShowPanes(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbMultiShowButtons || CB==cbMultiShowSearch);

	switch (CB)
	{
	case cbMultiShowButtons:
		gpSet->isMultiShowButtons = uCheck; break;
	case cbMultiShowSearch:
		gpSet->isMultiShowSearch = uCheck; break;
	}
	gpConEmu->mp_TabBar->OnShowButtonsChanged();

} // cbMultiShowButtons, cbMultiShowSearch


// cbMultiIterate
void CSetDlgButtons::OnBtn_MultiIterate(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbMultiIterate);

	gpSet->isMultiIterate = uCheck;

} // cbMultiIterate


// cbNewConfirm
void CSetDlgButtons::OnBtn_NewConfirm(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbNewConfirm);

	gpSet->isMultiNewConfirm = uCheck;

} // cbNewConfirm


// cbDupConfirm
void CSetDlgButtons::OnBtn_DupConfirm(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbDupConfirm);

	gpSet->isMultiDupConfirm = uCheck;

} // cbDupConfirm


// cbConfirmDetach
void CSetDlgButtons::OnBtn_ConfirmDetach(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbConfirmDetach);

	gpSet->isMultiDetachConfirm = uCheck;

} // cbConfirmDetach


// cbLongOutput
void CSetDlgButtons::OnBtn_LongOutput(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbLongOutput);

	gpSet->AutoBufferHeight = uCheck;
	CVConGroup::OnUpdateFarSettings();
	EnableWindow(GetDlgItem(hDlg, tLongOutputHeight), gpSet->AutoBufferHeight);

} // cbLongOutput


// rbComspecAuto || rbComspecEnvVar || rbComspecCmd || rbComspecExplicit
void CSetDlgButtons::OnBtn_ComspecRadio(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rbComspecAuto || CB==rbComspecEnvVar || CB==rbComspecCmd || CB==rbComspecExplicit);

	if (isOptChecked(rbComspecExplicit, CB, uCheck))
		gpSet->ComSpec.csType = cst_Explicit;
	else if (isOptChecked(rbComspecCmd, CB, uCheck))
		gpSet->ComSpec.csType = cst_Cmd;
	else if (isOptChecked(rbComspecEnvVar, CB, uCheck))
		gpSet->ComSpec.csType = cst_EnvVar;
	else
		gpSet->ComSpec.csType = cst_AutoTccCmd;
	enableDlgItem(hDlg, cbComspecUpdateEnv, (gpSet->ComSpec.csType!=cst_EnvVar));
	gpConEmu->OnGlobalSettingsChanged();

} // rbComspecAuto || rbComspecEnvVar || rbComspecCmd || rbComspecExplicit


// cbComspecExplicit
void CSetDlgButtons::OnBtn_ComspecExplicit(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbComspecExplicit);

	wchar_t temp[MAX_PATH], edt[MAX_PATH];
	if (!GetDlgItemText(hDlg, tComspecExplicit, edt, countof(edt)))
		edt[0] = 0;
	ExpandEnvironmentStrings(edt, temp, countof(temp));
	OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = ghOpWnd;
	ofn.lpstrFilter = L"Processors (cmd.exe,tcc.exe)\0cmd.exe;tcc.exe\0Executables (*.exe)\0*.exe\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = temp;
	ofn.nMaxFile = countof(temp);
	ofn.lpstrTitle = L"Choose command processor";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
				| OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn))
	{
		bool bChanged = (lstrcmp(gpSet->ComSpec.ComspecExplicit, temp)!=0);
		SetDlgItemText(hDlg, tComspecExplicit, temp);
		if (bChanged)
		{
			wcscpy_c(gpSet->ComSpec.ComspecExplicit, temp);
			gpSet->ComSpec.csType = cst_Explicit;
			checkRadioButton(hDlg, rbComspecAuto, rbComspecExplicit, rbComspecExplicit);
			gpConEmu->OnGlobalSettingsChanged();
		}
	}
} // cbComspecExplicit


//cbComspecTest
void CSetDlgButtons::OnBtn_ComspecTest(HWND hDlg, WORD CB, BYTE uCheck)
{
	wchar_t* psz = GetComspec(&gpSet->ComSpec);

	MsgBox(psz ? psz : L"<NULL>", MB_ICONINFORMATION, gpConEmu->GetDefaultTitle(), ghOpWnd);
	SafeFree(psz);

} // cbComspecTest


// rbComspec_OSbit || rbComspec_AppBit || rbComspec_x32
void CSetDlgButtons::OnBtn_ComspecBitsRadio(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rbComspec_x32 || CB==rbComspec_OSbit || CB==rbComspec_AppBit);

	if (isOptChecked(rbComspec_x32, CB, uCheck))
		gpSet->ComSpec.csBits = csb_x32;
	else if (isOptChecked(rbComspec_AppBit, CB, uCheck))
		gpSet->ComSpec.csBits = csb_SameApp;
	else
		gpSet->ComSpec.csBits = csb_SameOS;

	gpConEmu->OnGlobalSettingsChanged();
} // rbComspec_x32 || rbComspec_OSbit || rbComspec_AppBit


// cbComspecUpdateEnv
void CSetDlgButtons::OnBtn_ComspecUpdateEnv(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbComspecUpdateEnv);

	gpSet->ComSpec.isUpdateEnv = uCheck;
	gpConEmu->OnGlobalSettingsChanged();

} // cbComspecUpdateEnv


// cbAddConEmu2Path
void CSetDlgButtons::OnBtn_AddConEmu2Path(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbAddConEmu2Path);

	SetConEmuFlags(gpSet->ComSpec.AddConEmu2Path, CEAP_AddConEmuExeDir, uCheck ? CEAP_AddConEmuExeDir : CEAP_None);
	gpConEmu->OnGlobalSettingsChanged();

} // cbAddConEmu2Path


// cbAddConEmuBase2Path
void CSetDlgButtons::OnBtn_AddConEmuBase2Path(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbAddConEmuBase2Path);

	SetConEmuFlags(gpSet->ComSpec.AddConEmu2Path, CEAP_AddConEmuBaseDir, uCheck ? CEAP_AddConEmuBaseDir : CEAP_None);
	gpConEmu->OnGlobalSettingsChanged();

} // cbAddConEmuBase2Path


// cbComspecUncPaths
void CSetDlgButtons::OnBtn_ComspecUncPaths(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbComspecUncPaths);

	gpSet->ComSpec.isAllowUncPaths = uCheck;

} // cbComspecUncPaths


// cbCmdAutorunNewWnd
void CSetDlgButtons::OnBtn_CmdAutorunNewWnd(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCmdAutorunNewWnd);
	// does not insterested in ATM, this is used in ShellIntegration function only
} // cbCmdAutorunNewWnd


// bCmdAutoClear || bCmdAutoRegister || bCmdAutoUnregister
void CSetDlgButtons::OnBtn_CmdAutoActions(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==bCmdAutoClear || CB==bCmdAutoRegister || CB==bCmdAutoUnregister);

	CSetPgComspec* pComspecPg;
	if (gpSetCls->GetPageObj(pComspecPg))
	{
		pComspecPg->RegisterCmdAutorun(CB==bCmdAutoRegister, CB==bCmdAutoClear);
		pComspecPg->ReloadAutorun();
	}
	else
	{
		_ASSERTE(pComspecPg!=NULL && "CSetPgComspec was not created!");
	}

} // bCmdAutoClear || bCmdAutoRegister || bCmdAutoUnregister


// cbFontMonitorDpi || cbBold || cbItalic || cbFontAsDeviceUnits
void CSetDlgButtons::OnBtn_FontStyles(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbFontMonitorDpi || CB==cbBold || CB==cbItalic || CB==cbFontAsDeviceUnits);

	PostMessage(hDlg, gpSetCls->mn_MsgRecreateFont, CB, 0);

} // cbFontMonitorDpi || cbBold || cbItalic || cbFontAsDeviceUnits


// cbBgImage
void CSetDlgButtons::OnBtn_BgImageEnable(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbBgImage);

	gpSet->isShowBgImage = uCheck;

	EnableWindow(GetDlgItem(hDlg, tBgImage), gpSet->isShowBgImage);
	//EnableWindow(GetDlgItem(hDlg, tDarker), gpSet->isShowBgImage);
	//EnableWindow(GetDlgItem(hDlg, slDarker), gpSet->isShowBgImage);
	EnableWindow(GetDlgItem(hDlg, bBgImage), gpSet->isShowBgImage);
	//EnableWindow(GetDlgItem(hDlg, rBgUpLeft), gpSet->isShowBgImage);
	//EnableWindow(GetDlgItem(hDlg, rBgStretch), gpSet->isShowBgImage);
	//EnableWindow(GetDlgItem(hDlg, rBgTile), gpSet->isShowBgImage);

	BOOL lbNeedLoad;
	#ifndef APPDISTINCTBACKGROUND
	lbNeedLoad = (mp_Bg == NULL);
	#else
	lbNeedLoad = (gpSetCls->mp_BgInfo == NULL) || (lstrcmpi(gpSetCls->mp_BgInfo->BgImage(), gpSet->sBgImage) != 0);
	#endif

	if (gpSet->isShowBgImage && gpSet->bgImageDarker == 0)
	{
		if (MsgBox( L"Background image will NOT be visible\n"
					L"while 'Darkening' is 0. Increase it?",
					MB_YESNO|MB_ICONEXCLAMATION,
					gpConEmu->GetDefaultTitle(),
					ghOpWnd )!=IDNO)
		{
			gpSetCls->SetBgImageDarker(0x46, false);
			//gpSet->bgImageDarker = 0x46;
			//SendDlgItemMessage(hDlg, slDarker, TBM_SETPOS, (WPARAM) true, (LPARAM) gpSet->bgImageDarker);
			//TCHAR tmp[10];
			//_wsprintf(tmp, SKIPLEN(countof(tmp)) L"%i", gpSet->bgImageDarker);
			//SetDlgItemText(hDlg, tDarker, tmp);
			lbNeedLoad = TRUE;
		}
	}

	if (lbNeedLoad)
	{
		gpSetCls->LoadBackgroundFile(gpSet->sBgImage, true);
	}

	gpSetCls->NeedBackgroundUpdate();

	gpConEmu->Update(true);

} // cbBgImage


// bBgImage
void CSetDlgButtons::OnBtn_BgImageChoose(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==bBgImage);

	wchar_t temp[MAX_PATH], edt[MAX_PATH];
	if (!GetDlgItemText(hDlg, tBgImage, edt, countof(edt)))
		edt[0] = 0;
	ExpandEnvironmentStrings(edt, temp, countof(temp));
	OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = ghOpWnd;
	ofn.lpstrFilter = L"All images (*.bmp,*.jpg,*.png)\0*.bmp;*.jpg;*.jpe;*.jpeg;*.png\0Bitmap images (*.bmp)\0*.bmp\0JPEG images (*.jpg)\0*.jpg;*.jpe;*.jpeg\0PNG images (*.png)\0*.png\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = temp;
	ofn.nMaxFile = countof(temp);
	ofn.lpstrTitle = L"Choose background image";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
				| OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn))
	{
		if (gpSetCls->LoadBackgroundFile(temp, true))
		{
			bool bUseEnvVar = false;
			size_t nEnvLen = _tcslen(gpConEmu->ms_ConEmuExeDir);
			if (_tcslen(temp) > nEnvLen && temp[nEnvLen] == L'\\')
			{
				temp[nEnvLen] = 0;
				if (lstrcmpi(temp, gpConEmu->ms_ConEmuExeDir) == 0)
					bUseEnvVar = true;
				temp[nEnvLen] = L'\\';
			}
			if (bUseEnvVar)
			{
				wcscpy_c(gpSet->sBgImage, L"%ConEmuDir%");
				wcscat_c(gpSet->sBgImage, temp + _tcslen(gpConEmu->ms_ConEmuExeDir));
			}
			else
			{
				wcscpy_c(gpSet->sBgImage, temp);
			}
			SetDlgItemText(hDlg, tBgImage, gpSet->sBgImage);
			gpConEmu->Update(true);
		}
	}
} // bBgImage


// cbBgAllowPlugin
void CSetDlgButtons::OnBtn_BgAllowPlugin(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbBgAllowPlugin);

	gpSet->isBgPluginAllowed = uCheck;
	gpSetCls->NeedBackgroundUpdate();
	gpConEmu->Update(true);

} // cbBgAllowPlugin


// cbRClick
void CSetDlgButtons::OnBtn_RClick(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbRClick);

	gpSet->isRClickSendKey = uCheck; //0-1-2

} // cbRClick


// cbSafeFarClose
void CSetDlgButtons::OnBtn_SafeFarClose(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbSafeFarClose);

	gpSet->isSafeFarClose = uCheck;

} // cbSafeFarClose


// cbMinToTray
void CSetDlgButtons::OnBtn_MinToTray(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbMinToTray);

	gpSet->mb_MinToTray = uCheck;

} // cbMinToTray


// cbCloseWindowConfirm, cbCloseConsoleConfirm, cbConfirmCloseRunning, cbCloseEditViewConfirm
void CSetDlgButtons::OnBtn_CloseConfirmFlags(HWND hDlg, WORD CB, BYTE uCheck)
{
	BYTE Flag = Settings::cc_None;

	switch (CB)
	{
	case cbCloseWindowConfirm:
		Flag = Settings::cc_Window;
		break;
	case cbCloseConsoleConfirm:
		Flag = Settings::cc_Console;
		break;
	case cbConfirmCloseRunning:
		Flag = Settings::cc_Running;
		break;
	case cbCloseEditViewConfirm:
		Flag = Settings::cc_FarEV;
		break;
	default:
		_ASSERTE(CB==cbCloseWindowConfirm || CB==cbCloseConsoleConfirm || CB==cbConfirmCloseRunning || CB==cbCloseEditViewConfirm);
		return;
	}

	if (uCheck)
		gpSet->nCloseConfirmFlags |= Flag;
	else
		gpSet->nCloseConfirmFlags &= ~Flag;

} // cbCloseWindowConfirm, cbCloseConsoleConfirm, cbConfirmCloseRunning, cbCloseEditViewConfirm


// cbAlwaysShowTrayIcon
void CSetDlgButtons::OnBtn_AlwaysShowTrayIcon(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbAlwaysShowTrayIcon);

	gpSet->mb_AlwaysShowTrayIcon = uCheck;
	Icon.SettingsChanged();

} // cbAlwaysShowTrayIcon


// cbQuakeAutoHide || cbQuakeStyle
void CSetDlgButtons::OnBtn_QuakeStyles(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbQuakeAutoHide || CB==cbQuakeStyle);

	bool bQuake = (CB == cbQuakeStyle) ? (uCheck != 0) : (gpSet->isQuakeStyle != 0);
	bool bAuto = (CB == cbQuakeAutoHide) ? (uCheck != 0) : (gpSet->isQuakeStyle == 2);
	BYTE NewQuakeMode = bQuake ? bAuto ? 2 : 1 : 0;

	//ConEmuWindowMode NewWindowMode =
	//	isOptChecked(rMaximized, CB, uCheck) ? wmMaximized :
	//	isOptChecked(rFullScreen, CB, uCheck) ? wmFullScreen :
	//	wmNormal;

	// здесь меняются gpSet->isQuakeStyle, gpSet->isTryToCenter, gpSet->SetMinToTray
	gpConEmu->SetQuakeMode(NewQuakeMode, (ConEmuWindowMode)gpSet->_WindowMode, true);

} // cbQuakeAutoHide || cbQuakeStyle


// cbHideCaption
void CSetDlgButtons::OnBtn_HideCaption(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbHideCaption);

	gpSet->isHideCaption = uCheck;
	if (!gpSet->isQuakeStyle && gpConEmu->isZoomed())
	{
		gpConEmu->OnHideCaption();
		apiSetForegroundWindow(ghOpWnd);
	}
} // cbHideCaption


// cbHideCaptionAlways
void CSetDlgButtons::OnBtn_HideCaptionAlways(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbHideCaptionAlways);

	gpSet->SetHideCaptionAlways(0!=uCheck);

	if (gpSet->isHideCaptionAlways())
	{
		checkDlgButton(hDlg, cbHideCaptionAlways, BST_CHECKED);
		//TODO("показать тултип, что скрытие обязательно при прозрачности");
	}
	EnableWindow(GetDlgItem(hDlg, cbHideCaptionAlways), !gpSet->isForcedHideCaptionAlways());

	gpConEmu->OnHideCaption();
	apiSetForegroundWindow(ghOpWnd);

} // cbHideCaptionAlways


// cbHideChildCaption
void CSetDlgButtons::OnBtn_HideChildCaption(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbHideChildCaption);

	gpSet->isHideChildCaption = uCheck;
	gpConEmu->OnSize(true);

} // cbHideChildCaption


// cbFARuseASCIIsort
void CSetDlgButtons::OnBtn_FARuseASCIIsort(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbFARuseASCIIsort);

	gpSet->isFARuseASCIIsort = uCheck;
	CVConGroup::OnUpdateFarSettings();

} // cbFARuseASCIIsort


// cbShellNoZoneCheck
void CSetDlgButtons::OnBtn_ShellNoZoneCheck(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbShellNoZoneCheck);

	gpSet->isShellNoZoneCheck = uCheck;
	CVConGroup::OnUpdateFarSettings();

} // cbShellNoZoneCheck


// cbKeyBarRClick
void CSetDlgButtons::OnBtn_KeyBarRClick(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbKeyBarRClick);

	gpSet->isKeyBarRClick = uCheck;

} // cbKeyBarRClick


// cbDragPanel
void CSetDlgButtons::OnBtn_DragPanel(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbDragPanel);

	gpSet->isDragPanel = uCheck;
	gpConEmu->OnSetCursor();

} // cbDragPanel


// cbDragPanelBothEdges
void CSetDlgButtons::OnBtn_DragPanelBothEdges(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbDragPanelBothEdges);

	gpSet->isDragPanelBothEdges = uCheck;
	gpConEmu->OnSetCursor();

} // cbDragPanelBothEdges


// cbTryToCenter
void CSetDlgButtons::OnBtn_TryToCenter(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbTryToCenter);

	gpSet->isTryToCenter = uCheck;
	// ресайзим консоль, иначе после включения/отключения PAD-size
	// размер консоли не изменится и она отрисуется с некорректным размером
	gpConEmu->OnSize(true);
	gpConEmu->InvalidateAll();

} // cbTryToCenter


// cbIntegralSize
void CSetDlgButtons::OnBtn_IntegralSize(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbIntegralSize);

	gpSet->mb_IntegralSize = (uCheck == BST_UNCHECKED);

} // cbIntegralSize


// cbRestore2ActiveMonitor
void CSetDlgButtons::OnBtn_Restore2ActiveMonitor(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbRestore2ActiveMonitor);

	gpSet->isRestore2ActiveMon = (uCheck != BST_UNCHECKED);

} // cbRestore2ActiveMonitor


// rbScrollbarAuto || rbScrollbarHide || rbScrollbarShow
void CSetDlgButtons::OnBtn_ScrollbarStyle(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rbScrollbarAuto || CB==rbScrollbarHide || CB==rbScrollbarShow);

	gpSet->isAlwaysShowScrollbar = CB - rbScrollbarHide;
	if (!gpSet->isAlwaysShowScrollbar) gpConEmu->OnAlwaysShowScrollbar(false);
	if (gpConEmu->isZoomed() || gpConEmu->isFullScreen())
		CVConGroup::SyncConsoleToWindow();
	else
		gpConEmu->SizeWindow(gpConEmu->WndWidth, gpConEmu->WndHeight);
	if (gpSet->isAlwaysShowScrollbar) gpConEmu->OnAlwaysShowScrollbar(false);
	gpConEmu->ReSize();
	//gpConEmu->OnSize(true);
	gpConEmu->InvalidateAll();

} // rbScrollbarAuto || rbScrollbarHide || rbScrollbarShow


// cbFarHourglass
void CSetDlgButtons::OnBtn_FarHourglass(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbFarHourglass);

	gpSet->isFarHourglass = uCheck;
	gpConEmu->OnSetCursor();

} // cbFarHourglass


// cbExtendUCharMap
void CSetDlgButtons::OnBtn_ExtendUCharMap(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbExtendUCharMap);

	gpSet->isExtendUCharMap = uCheck;
	gpConEmu->Update(true);

} // cbExtendUCharMap


// cbFixAltOnAltTab
void CSetDlgButtons::OnBtn_FixAltOnAltTab(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbFixAltOnAltTab);

	gpSet->isFixAltOnAltTab = uCheck;

} // cbFixAltOnAltTab


// cbAutoRegFonts
void CSetDlgButtons::OnBtn_AutoRegFonts(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbAutoRegFonts);

	gpSet->isAutoRegisterFonts = uCheck;

} // cbAutoRegFonts


// cbDebugSteps
void CSetDlgButtons::OnBtn_DebugSteps(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbDebugSteps);

	gpSet->isDebugSteps = uCheck;

} // cbDebugSteps


  // cbDebugLog
void CSetDlgButtons::OnBtn_DebugLog(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbDebugLog);

	// Reset value loaded from registry only if there
	// was no `-log` switch in ConEmu.exe arguments
	if (uCheck)
	{
		if (!gpSet->isDebugLog && !gpConEmu->opt.AdvLogging.Exists)
			gpSet->isDebugLog = 1;
		_ASSERTE(gpSet->isLogging());
		gpSet->EnableLogging();
		CSetPgFeatures* pFeat = NULL;
		if (gpSetCls->GetPageObj(pFeat))
			pFeat->UpdateLogLocation();
	}
	else
	{
		if (gpSet->isDebugLog && !gpConEmu->opt.AdvLogging.Exists)
			gpSet->isDebugLog = 0;
		gpSet->DisableLogging();
	}

} // cbDebugLog


// cbDragL || cbDragR
void CSetDlgButtons::OnBtn_DragLR(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbDragL || CB==cbDragR);

	gpSet->isDragEnabled =
	    (isOptChecked(cbDragL, CB, uCheck) ? DRAG_L_ALLOWED : 0) |
	    (isOptChecked(cbDragR, CB, uCheck) ? DRAG_R_ALLOWED : 0);

} // cbDragL || cbDragR


// cbDropEnabled
void CSetDlgButtons::OnBtn_DropEnabled(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbDropEnabled);

	gpSet->isDropEnabled = uCheck;

} // cbDropEnabled


// cbDnDCopy
void CSetDlgButtons::OnBtn_DnDCopy(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbDnDCopy);

	gpSet->isDefCopy = (uCheck == BST_CHECKED);

} // cbDnDCopy


// cbDropUseMenu
void CSetDlgButtons::OnBtn_DropUseMenu(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbDropUseMenu);

	gpSet->isDropUseMenu = uCheck;

} // cbDropUseMenu


// cbDragImage
void CSetDlgButtons::OnBtn_DragImage(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbDragImage);

	gpSet->isDragOverlay = uCheck;

} // cbDragImage


// cbDragIcons
void CSetDlgButtons::OnBtn_DragIcons(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbDragIcons);

	gpSet->isDragShowIcons = (uCheck == BST_CHECKED);

} // cbDragIcons


// cbEnhanceGraphics
void CSetDlgButtons::OnBtn_EnhanceGraphics(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbEnhanceGraphics);

	gpSet->isEnhanceGraphics = uCheck;
	gpConEmu->Update(true);

} // cbEnhanceGraphics


// cbEnhanceButtons
void CSetDlgButtons::OnBtn_EnhanceButtons(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbEnhanceButtons);

	gpSet->isEnhanceButtons = uCheck;
	gpConEmu->Update(true);

} // cbEnhanceButtons

// rbTabsNone || rbTabsAlways || rbTabsAuto
void CSetDlgButtons::OnBtn_TabsRadioAuto(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rbTabsNone || CB==rbTabsAlways || CB==rbTabsAuto);

	if (isOptChecked(rbTabsAuto, CB, uCheck))
	{
		gpSet->isTabs = 2;
	}
	else if (isOptChecked(rbTabsAlways, CB, uCheck))
	{
		gpSet->isTabs = 1;
		gpConEmu->ForceShowTabs(TRUE);
	}
	else
	{
		gpSet->isTabs = 0;
		gpConEmu->ForceShowTabs(FALSE);
	}

	gpConEmu->mp_TabBar->Update();
	gpConEmu->UpdateWindowRgn();

} // rbTabsNone || rbTabsAlways || rbTabsAuto


// cbTabsLocationBottom
void CSetDlgButtons::OnBtn_TabsLocationBottom(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbTabsLocationBottom);

	gpSet->nTabsLocation = uCheck;
	gpConEmu->OnSize();

} // cbTabsLocationBottom


// cbOneTabPerGroup
void CSetDlgButtons::OnBtn_OneTabPerGroup(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbOneTabPerGroup);

	gpSet->isOneTabPerGroup = uCheck;
	gpConEmu->mp_TabBar->Update(TRUE);

} // cbOneTabPerGroup


// cbActivateSplitMouseOver
void CSetDlgButtons::OnBtn_ActivateSplitMouseOver(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbActivateSplitMouseOver);

	GetCursorPos(&gpConEmu->mouse.ptLastSplitOverCheck);
	gpSet->bActivateSplitMouseOver = uCheck;
	gpConEmu->OnActivateSplitChanged();

} // cbActivateSplitMouseOver


// cbTabSelf
void CSetDlgButtons::OnBtn_TabSelf(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbTabSelf);

	gpSet->isTabSelf = uCheck;

} // cbTabSelf


// cbTabRecent
void CSetDlgButtons::OnBtn_TabRecent(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbTabRecent);

	gpSet->isTabRecent = uCheck;

} // cbTabRecent


// cbTabLazy
void CSetDlgButtons::OnBtn_TabLazy(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbTabLazy);

	gpSet->isTabLazy = uCheck;

} // cbTabLazy


// cbTaskbarOverlay
void CSetDlgButtons::OnBtn_TaskbarOverlay(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbTaskbarOverlay);

	gpSet->isTaskbarOverlay = uCheck;
	gpConEmu->Taskbar_UpdateOverlay();

} // cbTaskbarOverlay


// cbTaskbarProgress
void CSetDlgButtons::OnBtn_TaskbarProgress(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbTaskbarProgress);

	gpSet->isTaskbarProgress = uCheck;
	gpConEmu->UpdateProgress();

} // cbTaskbarProgress


// rbTaskbarBtnActive || rbTaskbarBtnAll || rbTaskbarBtnWin7 || rbTaskbarBtnHidden
void CSetDlgButtons::OnBtn_TaskbarBtnRadio(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rbTaskbarBtnActive || CB==rbTaskbarBtnAll || CB==rbTaskbarBtnWin7 || CB==rbTaskbarBtnHidden);

	// 3state: BST_UNCHECKED/BST_CHECKED/BST_INDETERMINATE
	gpSet->m_isTabsOnTaskBar = isOptChecked(rbTaskbarBtnAll, CB, uCheck) ? 1
		: isOptChecked(rbTaskbarBtnWin7, CB, uCheck) ? 2
		: isOptChecked(rbTaskbarBtnHidden, CB, uCheck) ? 3 : 0;
	if ((gpSet->m_isTabsOnTaskBar == 3) && !gpSet->mb_MinToTray)
	{
		gpSet->SetMinToTray(true);
	}
	gpConEmu->OnTaskbarSettingsChanged();

} // rbTaskbarBtnActive || rbTaskbarBtnAll || rbTaskbarBtnWin7 || rbTaskbarBtnHidden


// cbRSelectionFix
void CSetDlgButtons::OnBtn_RSelectionFix(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbRSelectionFix);

	gpSet->isRSelFix = uCheck;

} // cbRSelectionFix


// cbEnableMouse
void CSetDlgButtons::OnBtn_EnableMouse(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbEnableMouse);

	gpSet->isDisableMouse = uCheck ? false : true;

} // cbEnableMouse


// cbSkipActivation
void CSetDlgButtons::OnBtn_SkipActivation(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbSkipActivation);

	gpSet->isMouseSkipActivation = uCheck;

} // cbSkipActivation


// cbSkipMove
void CSetDlgButtons::OnBtn_SkipMove(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbSkipMove);

	gpSet->isMouseSkipMoving = uCheck;

} // cbSkipMove


// cbMonitorConsoleLang
void CSetDlgButtons::OnBtn_MonitorConsoleLang(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbMonitorConsoleLang);

	// "|2" reserved for "One layout for all consoles", always on
	gpSet->isMonitorConsoleLang = uCheck ? 3 : 0;

} // cbMonitorConsoleLang


// cbSkipFocusEvents
void CSetDlgButtons::OnBtn_SkipFocusEvents(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbSkipFocusEvents);

	gpSet->isSkipFocusEvents = uCheck;

} // cbSkipFocusEvents


// cbMonospace
void CSetDlgButtons::OnBtn_Monospace(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbMonospace);

	DEBUGTEST(BYTE cMonospaceNow = gpSet->isMonospace);

	gpSet->isMonospace = uCheck;

	MSetter lIgnoreEdit(&CSetPgBase::mb_IgnoreEditChanged);

	gpFontMgr->ResetFontWidth();
	gpConEmu->Update(true);

} // cbMonospace


// cbExtendFonts
void CSetDlgButtons::OnBtn_ExtendFonts(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbExtendFonts);

	gpSet->AppStd.isExtendFonts = uCheck;
	CSetDlgLists::EnableDlgItems(hDlg, CSetDlgLists::eExtendFonts, gpSet->AppStd.isExtendFonts);
	gpConEmu->Update(true);

} // cbExtendFonts


// rCursorH ... cbInactiveCursorIgnoreSize
void CSetDlgButtons::OnBtn_CursorOptions(HWND hDlg, WORD CB, BYTE uCheck)
{
	OnButtonClicked_Cursor(hDlg, CB, uCheck, &gpSet->AppStd);


	gpConEmu->Update(true);
	CVConGroup::InvalidateAll();

} // rCursorH ... cbInactiveCursorIgnoreSize


static bool ShowHideRealCon(CVirtualConsole* pVCon, LPARAM lParam)
{
	pVCon->RCon()->ShowConsole((int)lParam);
	return true; // continue
}

// cbVisible
void CSetDlgButtons::OnBtn_RConVisible(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbVisible);

	gpSet->isConVisible = uCheck;

	if (gpSet->isConVisible)
	{
		// Если показывать - то только текущую (иначе на экране мешанина консолей будет
		CVConGroup::EnumVCon(evf_Active, ShowHideRealCon, 1);
	}
	else
	{
		// А если скрывать - то все сразу
		CVConGroup::EnumVCon(evf_All, ShowHideRealCon, 0);
	}

	apiSetForegroundWindow(ghOpWnd);

} // cbVisible


// cbUseInjects
void CSetDlgButtons::OnBtn_UseInjects(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUseInjects);

	gpSet->isUseInjects = uCheck;
	gpConEmu->OnGlobalSettingsChanged();

} // cbUseInjects


// cbProcessAnsi
void CSetDlgButtons::OnBtn_ProcessAnsi(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbProcessAnsi);

	gpSet->isProcessAnsi = uCheck;
	gpConEmu->OnGlobalSettingsChanged();

} // cbProcessAnsi


// cbAnsiLog
void CSetDlgButtons::OnBtn_AnsiLog(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbAnsiLog);

	gpSet->isAnsiLog = uCheck;

} // cbAnsiLog


// cbProcessNewConArg
void CSetDlgButtons::OnBtn_ProcessNewConArg(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbProcessNewConArg);

	gpSet->isProcessNewConArg = uCheck;
	gpConEmu->OnGlobalSettingsChanged();

} // cbProcessNewConArg


// cbProcessCmdStart
void CSetDlgButtons::OnBtn_ProcessCmdStart(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbProcessCmdStart);

	gpSet->isProcessCmdStart = uCheck;
	gpConEmu->OnGlobalSettingsChanged();

} // cbProcessCmdStart


// cbProcessCtrlZ
void CSetDlgButtons::OnBtn_ProcessCtrlZ(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbProcessCtrlZ);

	gpSet->isProcessCtrlZ = (uCheck != BST_UNCHECKED);
	gpConEmu->OnGlobalSettingsChanged();

} // cbProcessCtrlZ


// cbSuppressBells
void CSetDlgButtons::OnBtn_SuppressBells(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbSuppressBells);

	gpSet->isSuppressBells = uCheck;
	gpConEmu->OnGlobalSettingsChanged();

} // cbSuppressBells


// cbConsoleExceptionHandler
void CSetDlgButtons::OnBtn_ConsoleExceptionHandler(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbConsoleExceptionHandler);

	gpSet->isConsoleExceptionHandler = uCheck;
	gpConEmu->OnGlobalSettingsChanged();

} // cbConsoleExceptionHandler


// cbUseClink
void CSetDlgButtons::OnBtn_UseClink(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUseClink);

	gpSet->mb_UseClink = uCheck;

	if (gpSet->mb_UseClink && !gpSet->isUseClink())
	{
		checkDlgButton(hDlg, cbUseClink, BST_UNCHECKED);
		wchar_t szErrInfo[MAX_PATH+200];
		_wsprintf(szErrInfo, SKIPLEN(countof(szErrInfo))
			L"Clink was not found in '%s\\clink'. Download and unpack clink files\nhttps://mridgers.github.io/clink/\n\n"
			L"Note that you don't need to check 'Use clink'\nif you already have set up clink globally.",
			gpConEmu->ms_ConEmuBaseDir);
		MsgBox(szErrInfo, MB_ICONSTOP|MB_SYSTEMMODAL, NULL, ghOpWnd);
	}
	gpConEmu->OnGlobalSettingsChanged();

} // cbUseClink


// cbClinkWebPage
void CSetDlgButtons::OnBtn_ClinkWebPage(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbClinkWebPage);

	ShellExecute(NULL, L"open", L"https://mridgers.github.io/clink/", NULL, NULL, SW_SHOWNORMAL);

} // cbClinkWebPage


// bRealConsoleSettings
void CSetDlgButtons::OnBtn_RealConsoleSettings(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==bRealConsoleSettings);

	gpSetCls->EditConsoleFont(ghOpWnd);

} // bRealConsoleSettings


// cbSnapToDesktopEdges
void CSetDlgButtons::OnBtn_SnapToDesktopEdges(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbSnapToDesktopEdges);

	gpSet->isSnapToDesktopEdges = uCheck;
	if (gpSet->isSnapToDesktopEdges)
		gpConEmu->OnMoving();

} // cbSnapToDesktopEdges


// cbAlwaysOnTop
void CSetDlgButtons::OnBtn_AlwaysOnTop(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbAlwaysOnTop);

	gpSet->isAlwaysOnTop = uCheck;
	gpConEmu->DoAlwaysOnTopSwitch();

} // cbAlwaysOnTop


// cbSleepInBackground
void CSetDlgButtons::OnBtn_SleepInBackground(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbSleepInBackground);

	gpSet->isSleepInBackground = uCheck;
	CVConGroup::OnGuiFocused(TRUE);

} // cbSleepInBackground


// cbRetardInactivePanes
void CSetDlgButtons::OnBtn_RetardInactivePanes(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbRetardInactivePanes);

	gpSet->isRetardInactivePanes = uCheck;
	CVConGroup::OnGuiFocused(TRUE);

} // cbRetardInactivePanes


// cbMinimizeOnLoseFocus
void CSetDlgButtons::OnBtn_MinimizeOnLoseFocus(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbMinimizeOnLoseFocus);

	gpSet->mb_MinimizeOnLoseFocus = uCheck;

} // cbMinimizeOnLoseFocus


// cbFocusInChildWindows
void CSetDlgButtons::OnBtn_FocusInChildWindows(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbFocusInChildWindows);

	gpSet->isFocusInChildWindows = uCheck;

} // cbFocusInChildWindows


// cbDisableFarFlashing
void CSetDlgButtons::OnBtn_DisableFarFlashing(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbDisableFarFlashing);

	gpSet->isDisableFarFlashing = uCheck;

} // cbDisableFarFlashing


// cbDisableAllFlashing
void CSetDlgButtons::OnBtn_DisableAllFlashing(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbDisableAllFlashing);

	gpSet->isDisableAllFlashing = uCheck;

} // cbDisableAllFlashing


// cbShowWasHiddenMsg
void CSetDlgButtons::OnBtn_ShowWasHiddenMsg(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbShowWasHiddenMsg);

	gpSet->isDownShowHiddenMessage = uCheck ? false : true;

} // cbShowWasHiddenMsg


// cbShowWasSetOnTopMsg
void CSetDlgButtons::OnBtn_ShowWasSetOnTopMsg(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbShowWasSetOnTopMsg);

	gpSet->isDownShowExOnTopMessage = uCheck ? false : true;

} // cbShowWasSetOnTopMsg


// cbTabsInCaption
void CSetDlgButtons::OnBtn_TabsInCaption(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbTabsInCaption);

	gpSet->isTabsInCaption = uCheck;
	////RedrawWindow(ghWnd, NULL, NULL, RDW_UPDATENOW|RDW_FRAME);
	////gpConEmu->OnNcMessage(ghWnd, WM_NCPAINT, 0,0);
	//SendMessage(ghWnd, WM_NCACTIVATE, 0, 0);
	//SendMessage(ghWnd, WM_NCPAINT, 0, 0);
	gpConEmu->RedrawFrame();

} // cbTabsInCaption


// cbNumberInCaption
void CSetDlgButtons::OnBtn_NumberInCaption(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbNumberInCaption);

	gpSet->isNumberInCaption = uCheck;
	gpConEmu->UpdateTitle();

} // cbNumberInCaption


// cbAdminShield || cbAdminSuffix
void CSetDlgButtons::OnBtn_AdminSuffixOrShield(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbAdminShield || CB==cbAdminSuffix);

	// This may be called from GuiMacro, so hDlg may be NULL
	BOOL bShield = (gpSet->bAdminShield == ats_ShieldSuffix || gpSet->bAdminShield == ats_Shield);
	BOOL bSuffix = (gpSet->bAdminShield == ats_ShieldSuffix || gpSet->bAdminShield == ats_Empty);

	if (hDlg)
	{
		bShield = isChecked(hDlg, cbAdminShield);
		bSuffix = isChecked(hDlg, cbAdminSuffix);
	}
	else if (CB == cbAdminShield)
	{
		bShield = uCheck;
	}
	else if (CB == cbAdminSuffix)
	{
		bSuffix = uCheck;
	}

	gpSet->bAdminShield = (bShield && bSuffix) ? ats_ShieldSuffix : bShield ? ats_Shield : bSuffix ? ats_Empty : ats_Disabled;

	if (bSuffix && !*gpSet->szAdminTitleSuffix)
	{
		wcscpy_c(gpSet->szAdminTitleSuffix, DefaultAdminTitleSuffix);
		SetDlgItemText(hDlg, tAdminSuffix, gpSet->szAdminTitleSuffix);
	}
	gpConEmu->mp_TabBar->Update(TRUE);

} // cbAdminShield || cbAdminSuffix


// cbHideInactiveConTabs
void CSetDlgButtons::OnBtn_HideInactiveConTabs(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbHideInactiveConTabs);

	gpSet->bHideInactiveConsoleTabs = uCheck;
	gpConEmu->mp_TabBar->Update(TRUE);

} // cbHideInactiveConTabs


// cbHideDisabledTabs
void CSetDlgButtons::OnBtn_HideDisabledTabs(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbHideDisabledTabs);

	gpSet->bHideDisabledTabs = uCheck;
	gpConEmu->mp_TabBar->Update(TRUE);

} // cbHideDisabledTabs


// cbShowFarWindows
void CSetDlgButtons::OnBtn_ShowFarWindows(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbShowFarWindows);

	gpSet->bShowFarWindows = uCheck;
	gpConEmu->mp_TabBar->Update(TRUE);

} // cbShowFarWindows


// cbCloseConEmuWithLastTab || cbCloseConEmuOnCrossClicking
void CSetDlgButtons::OnBtn_CloseConEmuOptions(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCloseConEmuWithLastTab || CB==cbCloseConEmuOnCrossClicking);

	// isMultiLeaveOnClose: 0 - закрываться, 1 - оставаться, 2 - НЕ оставаться при закрытии "крестиком"

	BYTE CurVal = gpSet->isMultiLeaveOnClose;
	BOOL bClose = gpSet->isCloseOnLastTabClose(); // закрываться с последним табом
	BOOL bQuit = gpSet->isCloseOnCrossClick();  // закрываться по крестику
	_ASSERTE((bClose&&bQuit) || (!bClose&&bQuit) || (!bClose&&!bQuit));

	// hDlg may be NULL if called from GuiMacro
	if (hDlg)
	{
		bClose = isChecked(hDlg, cbCloseConEmuWithLastTab);
		bQuit = isChecked(hDlg, cbCloseConEmuOnCrossClicking);
	}
	else if (CB == cbCloseConEmuWithLastTab)
	{
		bClose = uCheck;
	}
	else if (CB == cbCloseConEmuOnCrossClicking)
	{
		bQuit = uCheck;
	}

	// Apply new value
	gpSet->isMultiLeaveOnClose = bClose ? 0 : bQuit ? 2 : 1;

	if (CurVal != gpSet->isMultiLeaveOnClose)
	{
		gpConEmu->LogString(L"isMultiLeaveOnClose changed from dialog or macro (cbCloseConEmuWithLastTab)");
	}

	if (hDlg)
	{
		checkDlgButton(hDlg, cbMinimizeOnLastTabClose, gpSet->isMinOnLastTabClose() ? BST_CHECKED : BST_UNCHECKED);
		checkDlgButton(hDlg, cbHideOnLastTabClose, gpSet->isHideOnLastTabClose() ? BST_CHECKED : BST_UNCHECKED);
		enableDlgItem(hDlg, cbCloseConEmuOnCrossClicking, !gpSet->isCloseOnLastTabClose());
		enableDlgItem(hDlg, cbMinimizeOnLastTabClose, !gpSet->isCloseOnLastTabClose());
		enableDlgItem(hDlg, cbHideOnLastTabClose, !gpSet->isCloseOnLastTabClose() && gpSet->isMinOnLastTabClose());
	}
} // cbCloseConEmuWithLastTab || cbCloseConEmuOnCrossClicking


// cbMinimizeOnLastTabClose || cbHideOnLastTabClose
void CSetDlgButtons::OnBtn_HideOrMinOnLastTabClose(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbMinimizeOnLastTabClose || CB==cbHideOnLastTabClose);

	if ((hDlg && !isChecked(hDlg, cbCloseConEmuWithLastTab))
		|| (!hDlg && !gpSet->isCloseOnLastTabClose()))
	{
		// isMultiHideOnClose: 0 - не скрываться, 1 - в трей, 2 - просто минимизация

		BOOL chkMin = gpSet->isMinOnLastTabClose();
		BOOL chkTSA = gpSet->isHideOnLastTabClose();

		// hDlg may be NULL if called from GuiMacro
		if (hDlg)
		{
			chkMin = isChecked(hDlg, cbMinimizeOnLastTabClose);
			chkTSA = isChecked(hDlg, cbHideOnLastTabClose);
		}
		else if (CB == cbMinimizeOnLastTabClose)
		{
			chkMin = uCheck;
		}
		else if (CB == cbHideOnLastTabClose)
		{
			chkTSA = uCheck;
		}

		if (!chkMin)
		{
			gpSet->isMultiHideOnClose = 0;
		}
		else
		{
			gpSet->isMultiHideOnClose = chkTSA ? 1 : 2;
		}

		if (hDlg)
		{
			checkDlgButton(hDlg, cbHideOnLastTabClose, gpSet->isHideOnLastTabClose() ? BST_CHECKED : BST_UNCHECKED);
			enableDlgItem(hDlg, cbHideOnLastTabClose, !gpSet->isCloseOnLastTabClose() && gpSet->isMinOnLastTabClose());
		}
	}
} // cbMinimizeOnLastTabClose || cbHideOnLastTabClose


// rbMinByEscNever || rbMinByEscEmpty || rbMinByEscAlways
void CSetDlgButtons::OnBtn_MinByEscRadio(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rbMinByEscNever || CB==rbMinByEscEmpty ||CB==rbMinByEscAlways);

	gpSet->isMultiMinByEsc = (CB == rbMinByEscAlways) ? 1 : (CB == rbMinByEscEmpty) ? 2 : 0;
	EnableWindow(GetDlgItem(hDlg, cbMapShiftEscToEsc), (gpSet->isMultiMinByEsc == 1 /*Always*/));

} // rbMinByEscNever || rbMinByEscEmpty || rbMinByEscAlways


// cbMapShiftEscToEsc
void CSetDlgButtons::OnBtn_MapShiftEscToEsc(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbMapShiftEscToEsc);

	gpSet->isMapShiftEscToEsc = uCheck;

} // cbMapShiftEscToEsc


// cbGuiMacroHelp
void CSetDlgButtons::OnBtn_GuiMacroHelp(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbGuiMacroHelp);

	ConEmuAbout::OnInfo_About(L"Macro");

} // cbGuiMacroHelp


// cbUseWinArrows || cbUseWinNumber || cbUseWinTab
void CSetDlgButtons::OnBtn_UseWinArrowNumTab(HWND hDlg, WORD CB, BYTE uCheck)
{
	switch (CB)
	{
	case cbUseWinArrows:
		gpSet->isUseWinArrows = uCheck;
		break;
	case cbUseWinNumber:
		gpSet->isUseWinNumber = uCheck;
		break;
	case cbUseWinTab:
		gpSet->isUseWinTab = uCheck;
		break;
	#ifdef _DEBUG
	default:
		_ASSERTE(FALSE && "Not handled");
	#endif
	}

	gpConEmu->UpdateWinHookSettings();

} // cbUseWinArrows || cbUseWinNumber || cbUseWinTab


// cbSendAltTab || cbSendAltEsc || cbSendAltPrintScrn || cbSendPrintScrn || cbSendCtrlEsc
void CSetDlgButtons::OnBtn_SendConsoleSpecials(HWND hDlg, WORD CB, BYTE uCheck)
{
	switch (CB)
	{
	case cbSendAltTab:
		gpSet->isSendAltTab = uCheck; break;
	case cbSendAltEsc:
		gpSet->isSendAltEsc = uCheck; break;
	case cbSendAltPrintScrn:
		gpSet->isSendAltPrintScrn = uCheck; break;
	case cbSendPrintScrn:
		gpSet->isSendPrintScrn = uCheck; break;
	case cbSendCtrlEsc:
		gpSet->isSendCtrlEsc = uCheck; break;
	#ifdef _DEBUG
	default:
		_ASSERTE(FALSE && "Not handled");
	#endif
	}

	gpConEmu->UpdateWinHookSettings();

} // cbSendAltTab || cbSendAltEsc || cbSendAltPrintScrn || cbSendPrintScrn || cbSendCtrlEsc


// cbInstallKeybHooks
void CSetDlgButtons::OnBtn_InstallKeybHooks(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbInstallKeybHooks);

	switch (uCheck)
	{
		// Разрешено
	case BST_CHECKED: gpSet->m_isKeyboardHooks = 1; gpConEmu->RegisterHooks(); break;
		// Запрещено
	case BST_UNCHECKED: gpSet->m_isKeyboardHooks = 2; gpConEmu->UnRegisterHooks(); break;
		// Запрос при старте
	case BST_INDETERMINATE: gpSet->m_isKeyboardHooks = 0; break;
	}
} // cbInstallKeybHooks


// cbDosBox
void CSetDlgButtons::OnBtn_DosBox(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbDosBox);

	if (gpConEmu->mb_DosBoxExists)
	{
		checkDlgButton(hDlg, cbDosBox, BST_CHECKED);
		EnableWindow(GetDlgItem(hDlg, cbDosBox), FALSE); // изменение пока запрещено
	}
	else
	{
		checkDlgButton(hDlg, cbDosBox, BST_UNCHECKED);
		size_t nMaxCCH = MAX_PATH*3;
		wchar_t* pszErrInfo = (wchar_t*)malloc(nMaxCCH*sizeof(wchar_t));
		_wsprintf(pszErrInfo, SKIPLEN(nMaxCCH) L"DosBox is not installed!\n"
				L"\n"
				L"DosBox files must be located here:"
				L"%s\\DosBox\\"
				L"\n"
				L"1. Copy files DOSBox.exe, SDL.dll, SDL_net.dll\n"
				L"2. Create of modify configuration file DOSBox.conf",
				gpConEmu->ms_ConEmuBaseDir);
	}
} // cbDosBox


// bApplyViewSettings
void CSetDlgButtons::OnBtn_ApplyViewSettings(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==bApplyViewSettings);

	gpConEmu->OnPanelViewSettingsChanged();
	//gpConEmu->UpdateGuiInfoMapping();

} // bApplyViewSettings


// cbThumbLoadFiles
void CSetDlgButtons::OnBtn_ThumbLoadFiles(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbThumbLoadFiles);

	switch (uCheck)
	{
		case BST_CHECKED:       gpSet->ThSet.bLoadPreviews = 3; break;
		case BST_INDETERMINATE: gpSet->ThSet.bLoadPreviews = 1; break;
		default: gpSet->ThSet.bLoadPreviews = 0;
	}
} // cbThumbLoadFiles


// cbThumbLoadFolders
void CSetDlgButtons::OnBtn_ThumbLoadFolders(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbThumbLoadFolders);

	gpSet->ThSet.bLoadFolders = uCheck;

} // cbThumbLoadFolders


// cbThumbUsePicView2
void CSetDlgButtons::OnBtn_ThumbUsePicView2(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbThumbUsePicView2);

	gpSet->ThSet.bUsePicView2 = uCheck;

} // cbThumbUsePicView2


// cbThumbRestoreOnStartup
void CSetDlgButtons::OnBtn_ThumbRestoreOnStartup(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbThumbRestoreOnStartup);

	gpSet->ThSet.bRestoreOnStartup = uCheck;

} // cbThumbRestoreOnStartup


// cbThumbPreviewBox
void CSetDlgButtons::OnBtn_ThumbPreviewBox(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbThumbPreviewBox);

	gpSet->ThSet.nPreviewFrame = uCheck;

} // cbThumbPreviewBox


// cbThumbSelectionBox
void CSetDlgButtons::OnBtn_ThumbSelectionBox(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbThumbSelectionBox);

	gpSet->ThSet.nSelectFrame = uCheck;

} // cbThumbSelectionBox


// rbThumbBackColorIdx || rbThumbBackColorRGB
void CSetDlgButtons::OnBtn_ThumbBackColors(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rbThumbBackColorIdx || CB==rbThumbBackColorRGB);

	gpSet->ThSet.crBackground.UseIndex = isOptChecked(rbThumbBackColorIdx, CB, uCheck);
	CSettings::InvalidateCtrl(GetDlgItem(hDlg, c32), TRUE);

} // rbThumbBackColorIdx || rbThumbBackColorRGB


// rbThumbPreviewBoxColorIdx || rbThumbPreviewBoxColorRGB
void CSetDlgButtons::OnBtn_ThumbPreviewBoxColors(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rbThumbPreviewBoxColorIdx || CB==rbThumbPreviewBoxColorRGB);

	gpSet->ThSet.crPreviewFrame.UseIndex = isOptChecked(rbThumbPreviewBoxColorIdx, CB, uCheck);
	CSettings::InvalidateCtrl(GetDlgItem(hDlg, c33), TRUE);

} // rbThumbPreviewBoxColorIdx || rbThumbPreviewBoxColorRGB


// rbThumbSelectionBoxColorIdx || rbThumbSelectionBoxColorRGB
void CSetDlgButtons::OnBtn_ThumbSelectionBoxColors(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rbThumbSelectionBoxColorIdx || CB==rbThumbSelectionBoxColorRGB);

	gpSet->ThSet.crSelectFrame.UseIndex = isOptChecked(rbThumbSelectionBoxColorIdx, CB, uCheck);
	CSettings::InvalidateCtrl(GetDlgItem(hDlg, c34), TRUE);

} // rbThumbSelectionBoxColorIdx || rbThumbSelectionBoxColorRGB


// cbActivityReset
void CSetDlgButtons::OnBtn_ActivityReset(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbActivityReset);

	ListView_DeleteAllItems(GetDlgItem(hDlg, lbActivityLog));
	//wchar_t szText[2]; szText[0] = 0;
	//HWND hDetails = GetDlgItem(hDlg, lbActivityDetails);
	//ListView_SetItemText(hDetails, 0, 1, szText);
	//ListView_SetItemText(hDetails, 1, 1, szText);
	SetDlgItemText(hDlg, ebActivityApp, L"");
	SetDlgItemText(hDlg, ebActivityParm, L"");

} // cbActivityReset


// cbActivitySaveAs
void CSetDlgButtons::OnBtn_ActivitySaveAs(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbActivitySaveAs);

	gpSetCls->OnSaveActivityLogFile(GetDlgItem(hDlg, lbActivityLog));

} // cbActivitySaveAs


// rbActivityDisabled || rbActivityShell || rbActivityInput || rbActivityCmd || rbActivityAnsi
void CSetDlgButtons::OnBtn_DebugActivityRadio(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rbActivityDisabled || CB==rbActivityShell || CB==rbActivityInput || CB==rbActivityCmd || CB==rbActivityAnsi);

	HWND hList = GetDlgItem(hDlg, lbActivityLog);
	//HWND hDetails = GetDlgItem(hDlg, lbActivityDetails);
	switch (CB)
	{
	case rbActivityShell:
		gpSetCls->m_ActivityLoggingType = glt_Processes; break;
	case rbActivityInput:
		gpSetCls->m_ActivityLoggingType = glt_Input; break;
	case rbActivityCmd:
		gpSetCls->m_ActivityLoggingType = glt_Commands; break;
	case rbActivityAnsi:
		gpSetCls->m_ActivityLoggingType = glt_Ansi; break;
	default:
		gpSetCls->m_ActivityLoggingType = glt_None;
	}

	ListView_DeleteAllItems(hList);

	for (int c = 0; (c <= 40) && ListView_DeleteColumn(hList, 0); c++)
		;

	//ListView_DeleteAllItems(hDetails);
	//for (int c = 0; (c <= 40) && ListView_DeleteColumn(hDetails, 0); c++);

	SetDlgItemText(hDlg, ebActivityApp, L"");
	SetDlgItemText(hDlg, ebActivityParm, L"");

	if (gpSetCls->m_ActivityLoggingType == glt_Processes)
	{
		LVCOLUMN col = {
			LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT,
			gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi)};
		wchar_t szTitle[64]; col.pszText = szTitle;

		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);

		wcscpy_c(szTitle, L"Time");		ListView_InsertColumn(hList, CSettings::lpc_Time, &col);
		col.cx = gpSetCls->EvalSize(55, esf_Horizontal|esf_CanUseDpi); col.fmt = LVCFMT_RIGHT;
		wcscpy_c(szTitle, L"PPID");		ListView_InsertColumn(hList, CSettings::lpc_PPID, &col);
		col.cx = gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi); col.fmt = LVCFMT_LEFT;
		wcscpy_c(szTitle, L"Func");		ListView_InsertColumn(hList, CSettings::lpc_Func, &col);
		col.cx = gpSetCls->EvalSize(50, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Oper");		ListView_InsertColumn(hList, CSettings::lpc_Oper, &col);
		col.cx = gpSetCls->EvalSize(40, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Bits");		ListView_InsertColumn(hList, CSettings::lpc_Bits, &col);
		wcscpy_c(szTitle, L"Syst");		ListView_InsertColumn(hList, CSettings::lpc_System, &col);
		col.cx = gpSetCls->EvalSize(120, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"App");		ListView_InsertColumn(hList, CSettings::lpc_App, &col);
		wcscpy_c(szTitle, L"Params");	ListView_InsertColumn(hList, CSettings::lpc_Params, &col);
		//wcscpy_c(szTitle, L"CurDir");	ListView_InsertColumn(hList, 7, &col);
		col.cx = gpSetCls->EvalSize(120, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Flags");	ListView_InsertColumn(hList, CSettings::lpc_Flags, &col);
		col.cx = gpSetCls->EvalSize(80, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"StdIn");	ListView_InsertColumn(hList, CSettings::lpc_StdIn, &col);
		wcscpy_c(szTitle, L"StdOut");	ListView_InsertColumn(hList, CSettings::lpc_StdOut, &col);
		wcscpy_c(szTitle, L"StdErr");	ListView_InsertColumn(hList, CSettings::lpc_StdErr, &col);

	}
	else if (gpSetCls->m_ActivityLoggingType == glt_Input)
	{
		LVCOLUMN col = {
			LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT,
			gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi)};
		wchar_t szTitle[64]; col.pszText = szTitle;

		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);

		wcscpy_c(szTitle, L"Time");		ListView_InsertColumn(hList, CSettings::lic_Time, &col);
		col.cx = gpSetCls->EvalSize(50, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Type");		ListView_InsertColumn(hList, CSettings::lic_Type, &col);
		col.cx = gpSetCls->EvalSize(50, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"##");		ListView_InsertColumn(hList, CSettings::lic_Dup, &col);
		col.cx = gpSetCls->EvalSize(300, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Event");	ListView_InsertColumn(hList, CSettings::lic_Event, &col);

	}
	else if (gpSetCls->m_ActivityLoggingType == glt_Commands)
	{
		gpSetCls->mn_ActivityCmdStartTick = timeGetTime();

		LVCOLUMN col = {
			LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT,
			gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi)};
		wchar_t szTitle[64]; col.pszText = szTitle;

		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);

		col.cx = gpSetCls->EvalSize(50, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"In/Out");	ListView_InsertColumn(hList, CSettings::lcc_InOut, &col);
		col.cx = gpSetCls->EvalSize(70, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Time");		ListView_InsertColumn(hList, CSettings::lcc_Time, &col);
		col.cx = gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Duration");	ListView_InsertColumn(hList, CSettings::lcc_Duration, &col);
		col.cx = gpSetCls->EvalSize(50, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Cmd");		ListView_InsertColumn(hList, CSettings::lcc_Command, &col);
		wcscpy_c(szTitle, L"Size");		ListView_InsertColumn(hList, CSettings::lcc_Size, &col);
		wcscpy_c(szTitle, L"PID");		ListView_InsertColumn(hList, CSettings::lcc_PID, &col);
		col.cx = gpSetCls->EvalSize(300, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Pipe");		ListView_InsertColumn(hList, CSettings::lcc_Pipe, &col);
		wcscpy_c(szTitle, L"Extra");	ListView_InsertColumn(hList, CSettings::lcc_Extra, &col);

	}
	else if (gpSetCls->m_ActivityLoggingType == glt_Ansi)
	{
		LVCOLUMN col = {
			LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT,
			gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi)};
		wchar_t szTitle[64]; col.pszText = szTitle;

		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);

		wcscpy_c(szTitle, L"Time");		ListView_InsertColumn(hList, CSettings::lac_Time, &col);
		col.cx = gpSetCls->EvalSize(500, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Event");	ListView_InsertColumn(hList, CSettings::lac_Sequence, &col);
	}
	else
	{
		LVCOLUMN col = {
			LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT,
			gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi)};
		wchar_t szTitle[4]; col.pszText = szTitle;
		wcscpy_c(szTitle, L" ");		ListView_InsertColumn(hList, 0, &col);
		//ListView_InsertColumn(hDetails, 0, &col);
	}
	ListView_DeleteAllItems(GetDlgItem(hDlg, lbActivityLog));

	gpConEmu->OnGlobalSettingsChanged();

} // rbActivityDisabled || rbActivityShell || rbActivityInput || rbActivityCmd || rbActivityAnsi


// rbColorRgbDec || rbColorRgbHex || rbColorBgrHex
void CSetDlgButtons::OnBtn_ColorFormat(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rbColorRgbDec || CB==rbColorRgbHex || CB==rbColorBgrHex);

	gpSetCls->m_ColorFormat = (CSettings::ColorShowFormat)(CB - rbColorRgbDec);

	if (hDlg)
	{
		CSetPgColors* pColorsPg;
		if (gpSetCls->GetPageObj(pColorsPg))
			pColorsPg->OnInitDialog(hDlg, false);
	}

} // rbColorRgbDec || rbColorRgbHex || rbColorBgrHex

// cbExtendColors
void CSetDlgButtons::OnBtn_ExtendColors(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbExtendColors);

	gpSet->AppStd.isExtendColors = (uCheck == BST_CHECKED);

	if (hDlg)
	{
		for (int i=16; i<32; i++) //-V112
			EnableWindow(GetDlgItem(hDlg, tc0+i), gpSet->AppStd.isExtendColors);

		EnableWindow(GetDlgItem(hDlg, lbExtendIdx), gpSet->AppStd.isExtendColors);
	}

	if (hDlg)
	{
		gpConEmu->Update(true);
	}

} // cbExtendColors


// cbColorResetExt
void CSetDlgButtons::OnBtn_ColorResetExt(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbColorResetExt);

	if (MsgBox(L"Reset colors 16..31 to default Windows palette?", MB_ICONQUESTION|MB_YESNO, NULL, ghOpWnd, true) != IDYES)
		return;

	const COLORREF* pcrColors = gpSet->GetDefColors();
	if (!pcrColors)
	{
		Assert(pcrColors!=NULL);
		return;
	}

	for (int i = 0; i < 16; i++)
	{
		CSetDlgColors::SetColorById(c16+i, pcrColors[i]);
		if (hDlg)
		{
			CSetDlgColors::ColorSetEdit(hDlg, c16+i);
			gpSetCls->InvalidateCtrl(GetDlgItem(hDlg, c16+i), TRUE);
		}
	}

	gpConEmu->InvalidateAll();
	gpConEmu->Update(true);

} // cbColorResetExt


// cbColorSchemeSave || cbColorSchemeDelete
void CSetDlgButtons::OnBtn_ColorSchemeSaveDelete(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbColorSchemeSave || CB==cbColorSchemeDelete);

	if (!hDlg)
	{
		_ASSERTE(hDlg!=NULL);
		return;
	}

	HWND hList = GetDlgItem(hDlg, lbDefaultColors);
	int nLen = GetWindowTextLength(hList);
	if (nLen < 1)
		return;

	wchar_t* pszName = (wchar_t*)malloc((nLen+1)*sizeof(wchar_t));
	GetWindowText(hList, pszName, nLen+1);
	if (*pszName != L'<')
	{
		if (CB == cbColorSchemeSave)
			gpSet->PaletteSaveAs(pszName);
		else
			gpSet->PaletteDelete(pszName);
	}
	// Поставить фокус в список, а то кнопки могут "задизэблиться"
	SetFocus(hList);
	HWND hCB = GetDlgItem(hDlg, CB);
	SetWindowLongPtr(hCB, GWL_STYLE, GetWindowLongPtr(hCB, GWL_STYLE) & ~BS_DEFPUSHBUTTON);
	// Refresh
	CSetPgColors* pColorsPg;
	if (gpSetCls->GetPageObj(pColorsPg))
		pColorsPg->OnInitDialog(hDlg, false);
	SafeFree(pszName);

} // cbColorSchemeSave || cbColorSchemeDelete


// cbTrueColorer
void CSetDlgButtons::OnBtn_TrueColorer(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbTrueColorer);

	gpSet->isTrueColorer = uCheck;
	CVConGroup::OnUpdateFarSettings();
	gpConEmu->Update(true);

} // cbTrueColorer


// cbFadeInactive
void CSetDlgButtons::OnBtn_FadeInactive(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbFadeInactive);

	gpSet->isFadeInactive = uCheck;
	CVConGroup::InvalidateAll();

} // cbFadeInactive


// cbTransparent
void CSetDlgButtons::OnBtn_Transparent(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbTransparent);

	int newV = gpSet->nTransparent;

	if (uCheck)
	{
		if (newV == MAX_ALPHA_VALUE) newV = 200;
	}
	else
	{
		newV = MAX_ALPHA_VALUE;
	}

	if (newV != gpSet->nTransparent)
	{
		gpSet->nTransparent = newV;

		if (hDlg)
		{
			SendDlgItemMessage(hDlg, slTransparent, TBM_SETPOS, (WPARAM) true, (LPARAM)gpSet->nTransparent);
			if (!gpSet->isTransparentSeparate)
				SendDlgItemMessage(hDlg, slTransparentInactive, TBM_SETPOS, (WPARAM) true, (LPARAM) gpSet->nTransparent);
		}

		gpConEmu->OnTransparent();
	}
} // cbTransparent


// cbTransparentSeparate
void CSetDlgButtons::OnBtn_TransparentSeparate(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbTransparentSeparate);

	gpSet->isTransparentSeparate = uCheck;

	if (hDlg)
	{
		//EnableWindow(GetDlgItem(hDlg, cbTransparentInactive), gpSet->isTransparentSeparate);
		EnableWindow(GetDlgItem(hDlg, slTransparentInactive), gpSet->isTransparentSeparate);
		EnableWindow(GetDlgItem(hDlg, stTransparentInactive), gpSet->isTransparentSeparate);
		EnableWindow(GetDlgItem(hDlg, stOpaqueInactive), gpSet->isTransparentSeparate);
		//checkDlgButton(hDlg, cbTransparentInactive, (gpSet->nTransparentInactive!=MAX_ALPHA_VALUE) ? BST_CHECKED : BST_UNCHECKED);
		SendDlgItemMessage(hDlg, slTransparentInactive, TBM_SETPOS, (WPARAM) true, (LPARAM) gpSet->isTransparentSeparate ? gpSet->nTransparentInactive : gpSet->nTransparent);
	}

	gpConEmu->OnTransparent();

} // cbTransparentSeparate


// cbUserScreenTransparent
void CSetDlgButtons::OnBtn_UserScreenTransparent(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUserScreenTransparent);

	gpSet->isUserScreenTransparent = uCheck;

	gpConEmu->OnHideCaption(); // при прозрачности - обязательно скрытие заголовка + кнопки
	gpConEmu->UpdateWindowRgn();

} // cbUserScreenTransparent


// cbColorKeyTransparent
void CSetDlgButtons::OnBtn_ColorKeyTransparent(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbColorKeyTransparent);

	gpSet->isColorKeyTransparent = uCheck;
	gpConEmu->OnTransparent();

} // cbColorKeyTransparent



/* *** Text selections options *** */

// cbCTSIntelligent
void CSetDlgButtons::OnBtn_CTSIntelligent(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSIntelligent);

	gpSet->isCTSIntelligent = uCheck;

} // cbCTSIntelligent


// rbCTSActAlways || rbCTSActBufferOnly
void CSetDlgButtons::OnBtn_CTSActConditionRadio(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB== rbCTSActAlways || CB==rbCTSActBufferOnly);

	gpSet->isCTSActMode = (CB==rbCTSActAlways) ? 1 : 2;

} // rbCTSActAlways || rbCTSActBufferOnly


// cbCTSFreezeBeforeSelect
void CSetDlgButtons::OnBtn_CTSFreezeBeforeSelect(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSFreezeBeforeSelect);

	gpSet->isCTSFreezeBeforeSelect = uCheck;

} // cbCTSFreezeBeforeSelect


// cbCTSAutoCopy
void CSetDlgButtons::OnBtn_CTSAutoCopy(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSAutoCopy);

	gpSet->isCTSAutoCopy = (uCheck != BST_UNCHECKED);

	if (hDlg)
	{
		CSettings::checkDlgButton(hDlg, cbCTSResetOnRelease, (gpSet->isCTSResetOnRelease && gpSet->isCTSAutoCopy));
		enableDlgItem(hDlg, cbCTSResetOnRelease, gpSet->isCTSAutoCopy);
	}

} // cbCTSAutoCopy


// cbCTSResetOnRelease
void CSetDlgButtons::OnBtn_CTSResetOnRelease(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSResetOnRelease);

	gpSet->isCTSResetOnRelease = (uCheck != BST_UNCHECKED);

} // cbCTSResetOnRelease


// cbCTSIBeam
void CSetDlgButtons::OnBtn_CTSIBeam(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSIBeam);

	gpSet->isCTSIBeam = uCheck;
	gpConEmu->OnSetCursor();

} // cbCTSIBeam


// cbCTSEndOnTyping || cbCTSEndCopyBefore
void CSetDlgButtons::OnBtn_CTSEndCopyAuto(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSEndOnTyping || CB==cbCTSEndCopyBefore);

	// isCTSEndOnTyping: 0 - off, 1 - copy & reset, 2 - reset only

	BOOL bTyping = (gpSet->isCTSEndOnTyping != 0);
	BOOL bCopy = (gpSet->isCTSEndOnTyping == 1);

	if (hDlg)
	{
		bTyping = isChecked(hDlg, cbCTSEndOnTyping);
		bCopy = isChecked(hDlg, cbCTSEndCopyBefore);
	}
	else if (CB == cbCTSEndOnTyping)
	{
		bTyping = uCheck;
	}
	else if (CB == cbCTSEndCopyBefore)
	{
		bCopy = uCheck;
	}

	gpSet->isCTSEndOnTyping = bTyping ? bCopy ? 1 : 2 : 0;

	if (hDlg)
	{
		enableDlgItem(hDlg, cbCTSEndOnKeyPress, gpSet->isCTSEndOnTyping!=0);
		enableDlgItem(hDlg, cbCTSEndCopyBefore, gpSet->isCTSEndOnTyping!=0);
		enableDlgItem(hDlg, cbCTSEraseBeforeReset, gpSet->isCTSEndOnTyping!=0);
		//checkDlgButton(hDlg, cbCTSEndOnKeyPress, gpSet->isCTSEndOnKeyPress); -- здесь не меняется -- "End on any key"
	}
} // cbCTSEndOnTyping || cbCTSEndCopyBefore


// cbCTSEndOnKeyPress
void CSetDlgButtons::OnBtn_CTSEndOnKeyPress(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSEndOnKeyPress);

	gpSet->isCTSEndOnKeyPress = (uCheck != BST_UNCHECKED);

} // cbCTSEndOnKeyPress


// cbCTSEraseBeforeReset
void CSetDlgButtons::OnBtn_CTSEraseBeforeReset(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSEraseBeforeReset);

	gpSet->isCTSEraseBeforeReset = (uCheck != BST_UNCHECKED);

} // cbCTSEraseBeforeReset


// cbCTSBlockSelection
void CSetDlgButtons::OnBtn_CTSBlockSelection(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSBlockSelection);

	gpSet->isCTSSelectBlock = uCheck;
	gpSetCls->CheckSelectionModifiers(hDlg);

} // cbCTSBlockSelection


// cbCTSTextSelection
void CSetDlgButtons::OnBtn_CTSTextSelection(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSTextSelection);

	gpSet->isCTSSelectText = uCheck;
	gpSetCls->CheckSelectionModifiers(hDlg);

} // cbCTSTextSelection


// cbCTSDetectLineEnd
void CSetDlgButtons::OnBtn_CTSDetectLineEnd(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSDetectLineEnd);

	gpSet->AppStd.isCTSDetectLineEnd = uCheck;

} // cbCTSDetectLineEnd


// cbCTSBashMargin
void CSetDlgButtons::OnBtn_CTSBashMargin(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSBashMargin);

	gpSet->AppStd.isCTSBashMargin = uCheck;

} // cbCTSBashMargin


// cbCTSTrimTrailing
void CSetDlgButtons::OnBtn_CTSTrimTrailing(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSTrimTrailing);

	gpSet->AppStd.isCTSTrimTrailing = uCheck;

} // cbCTSTrimTrailing


// cbCTSClickPromptPosition
void CSetDlgButtons::OnBtn_CTSClickPromptPosition(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSClickPromptPosition);

	gpSet->AppStd.isCTSClickPromptPosition = uCheck;
	gpSetCls->CheckSelectionModifiers(hDlg);

} // cbCTSClickPromptPosition


// cbCTSDeleteLeftWord
void CSetDlgButtons::OnBtn_CTSDeleteLeftWord(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSDeleteLeftWord);

	gpSet->AppStd.isCTSDeleteLeftWord = uCheck;

} // cbCTSDeleteLeftWord


// cbCTSShiftArrowStartSel
void CSetDlgButtons::OnBtn_CTSShiftArrowStartSel(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbCTSShiftArrowStartSel);

	gpSet->AppStd.isCTSShiftArrowStart = uCheck;

} // cbCTSShiftArrowStartSel


// cbFarGotoEditor
void CSetDlgButtons::OnBtn_FarGotoEditor(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbFarGotoEditor);

	gpSet->isFarGotoEditor = uCheck;

} // cbFarGotoEditor


// cbHighlightMouseRow
void CSetDlgButtons::OnBtn_HighlightMouseRow(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbHighlightMouseRow);

	gpSet->isHighlightMouseRow = uCheck;
	gpConEmu->Update(true);

} // cbHighlightMouseRow


// cbHighlightMouseCol
void CSetDlgButtons::OnBtn_HighlightMouseCol(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbHighlightMouseCol);

	gpSet->isHighlightMouseCol = uCheck;
	gpConEmu->Update(true);

} // cbHighlightMouseCol
/* *** Text selections options *** */

/* *** Update settings *** */

// cbUpdateCheckOnStartup
void CSetDlgButtons::OnBtn_UpdateCheckOnStartup(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUpdateCheckOnStartup);

	gpSet->UpdSet.isUpdateCheckOnStartup = uCheck;

} // cbUpdateCheckOnStartup


// cbUpdateCheckHourly
void CSetDlgButtons::OnBtn_UpdateCheckHourly(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUpdateCheckHourly);

	gpSet->UpdSet.isUpdateCheckHourly = uCheck;

} // cbUpdateCheckHourly


// cbUpdateConfirmDownload
void CSetDlgButtons::OnBtn_UpdateConfirmDownload(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUpdateConfirmDownload);

	gpSet->UpdSet.isUpdateConfirmDownload = (uCheck == BST_UNCHECKED);

} // cbUpdateConfirmDownload


// rbUpdateStableOnly || rbUpdatePreview || rbUpdateLatestAvailable
void CSetDlgButtons::OnBtn_UpdateTypeRadio(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rbUpdateStableOnly || CB==rbUpdatePreview || CB==rbUpdateLatestAvailable);

	gpSet->UpdSet.isUpdateUseBuilds = isOptChecked(rbUpdateStableOnly, CB, uCheck) ? 1 : isOptChecked(rbUpdateLatestAvailable, CB, uCheck) ? 2 : 3;

} // rbUpdateStableOnly || rbUpdatePreview || rbUpdateLatestAvailable


// cbUpdateInetTool
void CSetDlgButtons::OnBtn_UpdateInetTool(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUpdateInetTool);

	gpSet->UpdSet.isUpdateInetTool = (uCheck != BST_UNCHECKED);
	bool bEnabled = (gpSet->UpdSet.isUpdateInetTool);
	UINT nItems[] = {tUpdateInetTool, cbUpdateInetToolPath};
	enableDlgItems(hDlg, nItems, countof(nItems), bEnabled);
	SetDlgItemText(hDlg, tUpdateInetTool, gpSet->UpdSet.GetUpdateInetToolCmd());

	// Enable/Disable ‘Proxy’ fields too
	OnBtn_UpdateUseProxy(hDlg, cbUpdateUseProxy, gpSet->UpdSet.isUpdateUseProxy?BST_CHECKED:BST_UNCHECKED);

} // cbUpdateInetTool


// cbUpdateInetToolPath
void CSetDlgButtons::OnBtn_UpdateInetToolCmd(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUpdateInetToolPath);

	wchar_t szInetExe[MAX_PATH] = {};
	OPENFILENAME ofn = {sizeof(ofn)};
	ofn.hwndOwner = ghOpWnd;
	ofn.lpstrFilter = L"Exe files (*.exe)\0*.exe\0\0";
	ofn.nFilterIndex = 1;

	ofn.lpstrFile = szInetExe;
	ofn.nMaxFile = countof(szInetExe);
	ofn.lpstrTitle = L"Choose downloader tool";
	ofn.lpstrDefExt = L"exe";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
		| OFN_FILEMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

	if (GetOpenFileName(&ofn))
	{
		CEStr lsCmd;
		LPCWSTR pszName = PointToName(szInetExe);
		if (lstrcmpi(pszName, L"wget.exe") == 0)
			lsCmd = lstrmerge(L"\"", szInetExe, L"\" %1 -O %2");
		else if (lstrcmpi(pszName, L"curl.exe") == 0)
			lsCmd = lstrmerge(L"\"", szInetExe, L"\" -L %1 -o %2");
		else
			lsCmd = lstrmerge(L"\"", szInetExe, L"\" %1 %2");
		SetDlgItemText(hDlg, tUpdateInetTool, lsCmd.ms_Val);
	}
} // cbUpdateInetToolPath


// cbUpdateUseProxy
void CSetDlgButtons::OnBtn_UpdateUseProxy(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUpdateUseProxy);

	gpSet->UpdSet.isUpdateUseProxy = (uCheck != BST_UNCHECKED);
	bool bEnabled = (gpSet->UpdSet.isUpdateUseProxy && !gpSet->UpdSet.isUpdateInetTool);
	UINT nItems[] = {stUpdateProxy, tUpdateProxy, stUpdateProxyUser, tUpdateProxyUser, stUpdateProxyPassword, tUpdateProxyPassword};
	enableDlgItems(hDlg, nItems, countof(nItems), bEnabled);

} // cbUpdateUseProxy


// cbUpdateLeavePackages
void CSetDlgButtons::OnBtn_UpdateLeavePackages(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUpdateLeavePackages);

	gpSet->UpdSet.isUpdateLeavePackages = uCheck;

} // cbUpdateLeavePackages


// cbUpdateArcCmdLine
void CSetDlgButtons::OnBtn_UpdateArcCmdLine(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUpdateArcCmdLine);

	wchar_t szArcExe[MAX_PATH] = {};
	OPENFILENAME ofn = {sizeof(ofn)};
	ofn.hwndOwner = ghOpWnd;
	ofn.lpstrFilter = L"7-Zip or WinRar\0WinRAR.exe;UnRAR.exe;Rar.exe;7zg.exe;7z.exe\0Exe files (*.exe)\0*.exe\0\0";
	ofn.nFilterIndex = 1;

	ofn.lpstrFile = szArcExe;
	ofn.nMaxFile = countof(szArcExe);
	ofn.lpstrTitle = L"Choose 7-Zip or WinRar location";
	ofn.lpstrDefExt = L"exe";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
		| OFN_FILEMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

	if (GetOpenFileName(&ofn))
	{
		size_t nMax = _tcslen(szArcExe)+128;
		wchar_t *pszNew = (wchar_t*)calloc(nMax,sizeof(*pszNew));
		_wsprintf(pszNew, SKIPLEN(nMax) L"\"%s\"  x -y \"%%1\"", szArcExe);
		SetDlgItemText(hDlg, tUpdateArcCmdLine, pszNew);
		//if (gpSet->UpdSet.szUpdateArcCmdLine && lstrcmp(gpSet->UpdSet.szUpdateArcCmdLine, gpSet->UpdSet.szUpdateArcCmdLineDef) == 0)
		//	SafeFree(gpSet->UpdSet.szUpdateArcCmdLine);
		SafeFree(pszNew);
	}
} // cbUpdateArcCmdLine


// cbUpdateDownloadPath
void CSetDlgButtons::OnBtn_UpdateDownloadPath(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUpdateDownloadPath);

	wchar_t szStorePath[MAX_PATH] = {};
	wchar_t szInitial[MAX_PATH+1];
	ExpandEnvironmentStrings(gpSet->UpdSet.szUpdateDownloadPath, szInitial, countof(szInitial));
	OPENFILENAME ofn = {sizeof(ofn)};
	ofn.hwndOwner = ghOpWnd;
	ofn.lpstrFilter = L"Packages\0ConEmuSetup.*.exe;ConEmu.*.7z\0\0";
	ofn.nFilterIndex = 1;
	wcscpy_c(szStorePath, L"ConEmuSetup.exe");
	ofn.lpstrFile = szStorePath;
	ofn.nMaxFile = countof(szStorePath);
	ofn.lpstrInitialDir = szInitial;
	ofn.lpstrTitle = L"Choose download path";
	ofn.lpstrDefExt = L"";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
		| OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY;

	if (GetSaveFileName(&ofn))
	{
		wchar_t *pszSlash = wcsrchr(szStorePath, L'\\');
		if (pszSlash)
		{
			*pszSlash = 0;
			SetDlgItemText(hDlg, tUpdateDownloadPath, szStorePath);
		}
	}
} // cbUpdateDownloadPath

// cbUpdateCheck
void CSetDlgButtons::OnBtn_UpdateApplyAndCheck(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUpdateCheck);

	if (hDlg && gpSetCls)
	{
		wchar_t* pszVerIniLocation = NULL;
		gpSetCls->GetString(hDlg, tUpdateVerLocation, &pszVerIniLocation);
		if (pszVerIniLocation && (0 == lstrcmp(pszVerIniLocation, gpSet->UpdSet.UpdateVerLocationDefault())))
			SafeFree(pszVerIniLocation);
		gpSet->UpdSet.SetUpdateVerLocation(pszVerIniLocation);
		SafeFree(pszVerIniLocation);
	}

	gpConEmu->CheckUpdates(TRUE);

} // cbUpdateCheck

/* *** Update settings *** */


/* *** Default terminal *** */
void CSetDlgButtons::OnBtn_DefTerm(HWND hDlg, WORD CB, BYTE uCheck)
{
	bool bSetupDefaultTerminal = false;

	switch (CB)
	{
	case cbDefaultTerminal:
		gpSet->isSetDefaultTerminal = uCheck;
		bSetupDefaultTerminal = gpSet->isSetDefaultTerminal;
		break;

	case cbApplyDefTerm:
		bSetupDefaultTerminal = gpSet->isSetDefaultTerminal;
		break;

	case cbDefaultTerminalStartup:
	case cbDefaultTerminalTSA:
	case cbDefTermAgressive:
		if ((CB == cbDefaultTerminalStartup || CB == cbDefTermAgressive) && uCheck)
		{
			if (!gpSet->isSetDefaultTerminal)
			{
				if (MsgBox(L"Default Terminal feature was not enabled. Enable it now?", MB_YESNO|MB_ICONEXCLAMATION,
						NULL, ghOpWnd) != IDYES)
				{
					break;
				}
				gpSet->isSetDefaultTerminal = true;
				checkDlgButton(hDlg, cbDefaultTerminal, BST_CHECKED);
				bSetupDefaultTerminal = true;
			}
		}

		switch (CB)
		{
		case cbDefaultTerminalStartup:
			gpSet->isRegisterOnOsStartup = (uCheck != BST_UNCHECKED); break;
		case cbDefaultTerminalTSA:
			gpSet->isRegisterOnOsStartupTSA = (uCheck != BST_UNCHECKED); break;
		case cbDefTermAgressive:
			gpSet->isRegisterAgressive = (uCheck != BST_UNCHECKED); break;
		}

		if (hDlg)
		{
			EnableWindow(GetDlgItem(hDlg, cbDefaultTerminalTSA), gpSet->isRegisterOnOsStartup);
		}

		// And update registry
		gpConEmu->mp_DefTrm->CheckRegisterOsStartup();
		break;

	case cbDefaultTerminalNoInjects:
		gpSet->isDefaultTerminalNoInjects = uCheck;
		break;

	case cbDefaultTerminalUseExisting:
		gpSet->isDefaultTerminalNewWindow = !uCheck;
		break;

	case cbDefaultTerminalDebugLog:
		gpSet->isDefaultTerminalDebugLog = (uCheck != BST_UNCHECKED);
		break;

	case rbDefaultTerminalConfAuto:
	case rbDefaultTerminalConfAlways:
	case rbDefaultTerminalConfNever:
		gpSet->nDefaultTerminalConfirmClose =
			isOptChecked(rbDefaultTerminalConfAuto, CB, uCheck) ? 0 :
			isOptChecked(rbDefaultTerminalConfAlways, CB, uCheck) ? 1 : 2;
		break;

	#ifdef _DEBUG
	default:
		_ASSERTE(FALSE && "Not handled");
	#endif
	}

	gpConEmu->mp_DefTrm->ApplyAndSave(true, true);

	if (gpSet->isSetDefaultTerminal && bSetupDefaultTerminal)
	{
		// Change mouse cursor due to long operation
		SetCursor(LoadCursor(NULL,IDC_WAIT));

		// Redraw checkboxes to avoid lags in painting while installing hooks
		if (hDlg)
			RedrawWindow(hDlg, NULL, NULL, RDW_UPDATENOW|RDW_ALLCHILDREN);

		// Инициировать эксплорер, если он еще не был обработан
		gpConEmu->mp_DefTrm->StartGuiDefTerm(true);

		// Вернуть фокус в окно настроек
		SetForegroundWindow(ghOpWnd);
	}
}
/* *** Default terminal *** */


// bGotoEditorCmd
void CSetDlgButtons::OnBtn_GotoEditorCmd(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbUpdateDownloadPath);

	wchar_t szPath[MAX_PATH+1] = {};
	wchar_t szInitialDir[MAX_PATH+1]; GetCurrentDirectory(countof(szInitialDir), szInitialDir);

	LPCWSTR pszTemp = gpSet->sFarGotoEditor;
	CEStr szExe;
	if (NextArg(&pszTemp, szExe) == 0)
	{
		lstrcpyn(szPath, szExe, countof(szPath));
	}

	OPENFILENAME ofn = {sizeof(ofn)};
	ofn.hwndOwner = ghOpWnd;
	ofn.lpstrFilter = L"Executables (*.exe)\0*.exe\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szPath;
	ofn.nMaxFile = countof(szPath);
	ofn.lpstrInitialDir = szInitialDir;
	ofn.lpstrTitle = L"Choose file editor";
	ofn.lpstrDefExt = L"exe";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
		| OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY;

	if (GetSaveFileName(&ofn))
	{
		wchar_t *pszBuf = MergeCmdLine(szPath, pszTemp);
		if (pszBuf)
		{
			SetDlgItemText(hDlg, lbGotoEditorCmd, pszBuf);

			SafeFree(gpSet->sFarGotoEditor);
			gpSet->sFarGotoEditor = pszBuf;
		}
	}
} // bGotoEditorCmd


// c0, c1, .. c32, .. c38
void CSetDlgButtons::OnBtn_ColorField(HWND hDlg, WORD CB, BYTE uCheck)
{
	if (CB >= c32 && CB <= c34)
	{
		if (CSetDlgColors::ColorEditDialog(hDlg, CB))
		{
			if (CB == c32)
			{
				gpSet->ThSet.crBackground.UseIndex = 0;
				checkRadioButton(hDlg, rbThumbBackColorIdx, rbThumbBackColorRGB, rbThumbBackColorRGB);
			}
			else if (CB == c33)
			{
				gpSet->ThSet.crPreviewFrame.UseIndex = 0;
				checkRadioButton(hDlg, rbThumbPreviewBoxColorIdx, rbThumbPreviewBoxColorRGB, rbThumbPreviewBoxColorRGB);
			}
			else if (CB == c34)
			{
				gpSet->ThSet.crSelectFrame.UseIndex = 0;
				checkRadioButton(hDlg, rbThumbSelectionBoxColorIdx, rbThumbSelectionBoxColorRGB, rbThumbSelectionBoxColorRGB);
			}

			CSettings::InvalidateCtrl(GetDlgItem(hDlg, CB), TRUE);
			// done
		}
	} // else if (CB >= c32 && CB <= c34)
	else if (CB >= c35 && CB <= c37)
	{
		if (CSetDlgColors::ColorEditDialog(hDlg, CB))
		{
			gpConEmu->mp_Status->UpdateStatusBar(true);
		}
	} // if (CB >= c35 && CB <= c37)
	else if (CB == c38)
	{
		if (CSetDlgColors::ColorEditDialog(hDlg, CB))
		{
			gpConEmu->OnTransparent();
		}
	} // if (CB == c38)
	else if (CB >= c0 && CB <= CSetDlgColors::MAX_COLOR_EDT_ID)
	{
		if (CSetDlgColors::ColorEditDialog(hDlg, CB))
		{
			gpConEmu->InvalidateAll();
			gpConEmu->Update(true);
		}
	} // else if (CB >= c0 && CB <= MAX_COLOR_EDT_ID)
	else
	{
		_ASSERTE(FALSE && "ColorBtn was not handled");
	}
} // c0, c1, .. c32, .. c38


/* *** Status bar options *** */
// cbShowStatusBar
void CSetDlgButtons::OnBtn_ShowStatusBar(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbShowStatusBar);

	gpConEmu->StatusCommand(csc_ShowHide, uCheck ? 1 : 2);

} // cbShowStatusBar


// cbStatusVertSep
void CSetDlgButtons::OnBtn_StatusVertSep(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbStatusVertSep);

	if (uCheck)
		gpSet->isStatusBarFlags |= csf_VertDelim;
	else
		gpSet->isStatusBarFlags &= ~csf_VertDelim;
	gpConEmu->mp_Status->UpdateStatusBar(true);

} // cbStatusVertSep


// cbStatusHorzSep
void CSetDlgButtons::OnBtn_StatusHorzSep(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbStatusHorzSep);

	if (uCheck)
		gpSet->isStatusBarFlags |= csf_HorzDelim;
	else
		gpSet->isStatusBarFlags &= ~csf_HorzDelim;
	gpConEmu->RecreateControls(false, true, true);

} // cbStatusHorzSep


// cbStatusVertPad
void CSetDlgButtons::OnBtn_StatusVertPad(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbStatusVertPad);

	if (!uCheck)
		gpSet->isStatusBarFlags |= csf_NoVerticalPad;
	else
		gpSet->isStatusBarFlags &= ~csf_NoVerticalPad;
	gpConEmu->RecreateControls(false, true, true);

} // cbStatusVertPad


// cbStatusSystemColors
void CSetDlgButtons::OnBtn_StatusSystemColors(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbStatusSystemColors);

	if (uCheck)
		gpSet->isStatusBarFlags |= csf_SystemColors;
	else
		gpSet->isStatusBarFlags &= ~csf_SystemColors;
	CSetDlgLists::EnableDlgItems(hDlg, CSetDlgLists::eStatusColorIds, !(gpSet->isStatusBarFlags & csf_SystemColors));
	gpConEmu->mp_Status->UpdateStatusBar(true);

} // cbStatusSystemColors


// cbStatusAddAll || cbStatusAddSelected || cbStatusDelSelected || cbStatusDelAll
void CSetDlgButtons::OnBtn_StatusAddDel(HWND hDlg, WORD CB, BYTE uCheck)
{
	HWND hList = GetDlgItem(hDlg, (CB == cbStatusAddAll || CB == cbStatusAddSelected) ? lbStatusAvailable : lbStatusSelected);

	_ASSERTE(hList!=NULL);
	INT_PTR iCurAvail = SendMessage(hList, LB_GETCURSEL, 0, 0);
	INT_PTR iData = (iCurAvail >= 0) ? SendMessage(hList, LB_GETITEMDATA, iCurAvail, 0) : -1;

	bool bChanged = false;

	// gpSet->isStatusColumnHidden[SettingsNS::StatusItems[i].stItem] = ...
	StatusColInfo* pColumns = NULL;
	size_t nCount = CStatus::GetAllStatusCols(&pColumns);
	_ASSERTE(pColumns!=NULL);

	switch (CB)
	{
	case cbStatusAddSelected:
		if (iData >= 0 && iData < (INT_PTR)countof(gpSet->isStatusColumnHidden) && gpSet->isStatusColumnHidden[iData])
		{
			gpSet->isStatusColumnHidden[iData] = false;
			bChanged = true;
		}
		break;

	case cbStatusDelSelected:
		if (iData >= 0 && iData < (INT_PTR)countof(gpSet->isStatusColumnHidden) && !gpSet->isStatusColumnHidden[iData])
		{
			gpSet->isStatusColumnHidden[iData] = true;
			bChanged = true;
		}
		break;

	case cbStatusAddAll:
	case cbStatusDelAll:
		{
			bool bHide = (CB == cbStatusDelAll);
			for (size_t i = 0; i < nCount; i++)
			{
				CEStatusItems nID = pColumns[i].nID;
				if ((nID == csi_Info) || (pColumns[i].sSettingName == NULL))
					continue;
				if (gpSet->isStatusColumnHidden[nID] != bHide)
				{
					gpSet->isStatusColumnHidden[nID] = bHide;
					bChanged = true;
				}
			}
		}
		break;

	#ifdef _DEBUG
	default:
		_ASSERTE(FALSE && "Not handled");
	#endif
	}

	if (bChanged)
	{
		CSetPgStatus* pDlg = dynamic_cast<CSetPgStatus*>(gpSetCls->GetPageObj(thi_Status));
		if (pDlg)
			pDlg->UpdateStatusItems(hDlg);
		gpConEmu->mp_Status->UpdateStatusBar(true);
	}
} // cbStatusAddAll || cbStatusAddSelected || cbStatusDelSelected || cbStatusDelAll
