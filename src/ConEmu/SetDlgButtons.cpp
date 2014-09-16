
/*
Copyright (c) 2014 Maximus5
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
#include "SetDlgButtons.h"
#include "SetDlgLists.h"
#include "Status.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

#ifdef _DEBUG
void CSetDlgButtons::UnitTests()
{
	_ASSERTE(lstrcmp(CSettings::RASTER_FONTS_NAME, L"Raster Fonts")==0);
}
#endif

bool CSetDlgButtons::checkDlgButton(HWND hParent, WORD nCtrlId, UINT uCheck)
{
	#ifdef _DEBUG
	HWND hCheckBox = NULL;
	if (!hParent)
	{
		_ASSERTE(hParent!=NULL);
	}
	else
	{
		hCheckBox = GetDlgItem(hParent, nCtrlId);
		if (!hCheckBox)
		{
			//_ASSERTE(hCheckBox!=NULL && "Checkbox not found in hParent dlg");
			wchar_t szErr[128]; _wsprintf(szErr, SKIPLEN(countof(szErr)) L"checkDlgButton failed\nControlID %u not found in hParent dlg", nCtrlId);
			MsgBox(szErr, MB_SYSTEMMODAL|MB_ICONSTOP, L"ConEmu settings", ghOpWnd);
		}
		else
		{
			_ASSERTE(uCheck==BST_UNCHECKED || uCheck==BST_CHECKED || (uCheck==BST_INDETERMINATE && (BS_3STATE&GetWindowLong(hCheckBox,GWL_STYLE))));
		}
	}
	#endif

	// Аналог CheckDlgButton
	BOOL bRc = CheckDlgButton(hParent, nCtrlId, uCheck);
	return (bRc != FALSE);
}

bool CSetDlgButtons::checkRadioButton(HWND hParent, int nIDFirstButton, int nIDLastButton, int nIDCheckButton)
{
	#ifdef _DEBUG
	if (!hParent)
	{
		_ASSERTE(hParent!=NULL);
	}
	else if (!GetDlgItem(hParent, nIDFirstButton) || !GetDlgItem(hParent, nIDLastButton) || !GetDlgItem(hParent, nIDCheckButton))
	{
		//_ASSERTE(GetDlgItem(hParent, nIDFirstButton) && "Checkbox not found in hParent dlg");
		//_ASSERTE(GetDlgItem(hParent, nIDLastButton) && "Checkbox not found in hParent dlg");
		//_ASSERTE(GetDlgItem(hParent, nIDCheckButton) && "Checkbox not found in hParent dlg");
		wchar_t szErr[128]; _wsprintf(szErr, SKIPLEN(countof(szErr)) L"checkRadioButton failed\nControlIDs %u,%u,%u not found in hParent dlg", nIDFirstButton, nIDLastButton, nIDCheckButton);
		MsgBox(szErr, MB_SYSTEMMODAL|MB_ICONSTOP, L"ConEmu settings", ghOpWnd);
	}
	#endif

	// Аналог CheckRadioButton
	BOOL bRc = CheckRadioButton(hParent, nIDFirstButton, nIDLastButton, nIDCheckButton);
	return (bRc != FALSE);
}

// FALSE - выключена
// TRUE (BST_CHECKED) - включена
// BST_INDETERMINATE (2) - 3-d state
BYTE CSetDlgButtons::IsChecked(HWND hParent, WORD nCtrlId)
{
	#ifdef _DEBUG
	if (!hParent)
	{
		_ASSERTE(hParent!=NULL);
	}
	else if (!GetDlgItem(hParent, nCtrlId))
	{
		//_ASSERTE(hCheckBox!=NULL && "Checkbox not found in hParent dlg");
		wchar_t szErr[128]; _wsprintf(szErr, SKIPLEN(countof(szErr)) L"IsChecked failed\nControlID %u not found in hParent dlg", nCtrlId);
		MsgBox(szErr, MB_SYSTEMMODAL|MB_ICONSTOP, L"ConEmu settings", ghOpWnd);
	}
	#endif

	// Аналог IsDlgButtonChecked
	int nChecked = SendDlgItemMessage(hParent, nCtrlId, BM_GETCHECK, 0, 0);
	_ASSERTE(nChecked==0 || nChecked==1 || nChecked==2);

	if (nChecked!=0 && nChecked!=1 && nChecked!=2)
		nChecked = 0;

	return LOBYTE(nChecked);
}

LRESULT CSetDlgButtons::OnButtonClicked(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	_ASSERTE(hDlg!=NULL);
	WORD CB = LOWORD(wParam);

	switch (CB)
	{
		case IDOK:
		case IDCANCEL:
		case IDCLOSE:
			// -- обрабатываются в wndOpProc
			break;
		case rNoneAA:
		case rStandardAA:
		case rCTAA:
			PostMessage(hDlg, gpSetCls->mn_MsgRecreateFont, wParam, 0);
			break;
		case rNormal:
		case rFullScreen:
		case rMaximized:
			if (gpSet->isQuakeStyle)
			{
				gpSet->_WindowMode = CB;
				RECT rcWnd = gpConEmu->GetDefaultRect();
				//gpConEmu->SetWindowMode((ConEmuWindowMode)CB);
				SetWindowPos(ghWnd, NULL, rcWnd.left, rcWnd.top, rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top, SWP_NOZORDER);
				apiSetForegroundWindow(ghOpWnd);
			}
			else
			{
				EnableWindow(GetDlgItem(hDlg, cbApplyPos), TRUE);
				CSetDlgLists::EnableDlgItems(hDlg, CSetDlgLists::eSizeCtrlId, CB == rNormal);
			}
			break;
		case cbApplyPos:
			if (!gpConEmu->mp_Inside)
			{
				if (gpSet->isQuakeStyle
					|| (IsChecked(hDlg, rNormal) == BST_CHECKED))
				{
					int newX, newY;
					wchar_t* psSize;
					CESize newW, newH;
					//wchar_t temp[MAX_PATH];
					//GetDlgItemText(hDlg, tWndWidth, temp, countof(temp));  newW = klatoi(temp);
					//GetDlgItemText(hDlg, tWndHeight, temp, countof(temp)); newH = klatoi(temp);
					BOOL lbOk;

					psSize = GetDlgItemTextPtr(hDlg, tWndWidth);
					if (!psSize || !newW.SetFromString(true, psSize))
						newW.Raw = gpConEmu->WndWidth.Raw;
					SafeFree(psSize);
					psSize = GetDlgItemTextPtr(hDlg, tWndHeight);
					if (!psSize || !newH.SetFromString(false, psSize))
						newH.Raw = gpConEmu->WndHeight.Raw;
					SafeFree(psSize);

					newX = (int)GetDlgItemInt(hDlg, tWndX, &lbOk, TRUE);
					if (!lbOk) newX = gpConEmu->wndX;
					newY = (int)GetDlgItemInt(hDlg, tWndY, &lbOk, TRUE);
					if (!lbOk) newY = gpConEmu->wndY;

					if (gpSet->isQuakeStyle)
					{
						SetFocus(GetDlgItem(hDlg, tWndWidth));
						// Чтобы GetDefaultRect сработал правильно - сразу обновим значения
						if (!gpSet->wndCascade)
							gpConEmu->wndX = newX;
						gpConEmu->WndWidth.Set(true, newW.Style, newW.Value);
						gpConEmu->WndHeight.Set(false, newH.Style, newH.Value);
						RECT rcQuake = gpConEmu->GetDefaultRect();
						// And size/move!
						SetWindowPos(ghWnd, NULL, rcQuake.left, rcQuake.top, rcQuake.right-rcQuake.left, rcQuake.bottom-rcQuake.top, SWP_NOZORDER);
					}
					else
					{
						SetFocus(GetDlgItem(hDlg, rNormal));

						if (gpConEmu->isZoomed() || gpConEmu->isIconic() || gpConEmu->isFullScreen())
							gpConEmu->SetWindowMode(wmNormal);

						SetWindowPos(ghWnd, NULL, newX, newY, 0,0, SWP_NOSIZE|SWP_NOZORDER);

						// Установить размер
						gpConEmu->SizeWindow(newW, newH);

						SetWindowPos(ghWnd, NULL, newX, newY, 0,0, SWP_NOSIZE|SWP_NOZORDER);
					}
				}
				else if (IsChecked(hDlg, rMaximized) == BST_CHECKED)
				{
					SetFocus(GetDlgItem(hDlg, rMaximized));

					if (!gpConEmu->isZoomed())
						gpConEmu->SetWindowMode(wmMaximized);
				}
				else if (IsChecked(hDlg, rFullScreen) == BST_CHECKED)
				{
					SetFocus(GetDlgItem(hDlg, rFullScreen));

					if (!gpConEmu->isFullScreen())
						gpConEmu->SetWindowMode(wmFullScreen);
				}

				// Запомнить "идеальный" размер окна, выбранный пользователем
				gpConEmu->StoreIdealRect();
				//gpConEmu->UpdateIdealRect(TRUE);

				EnableWindow(GetDlgItem(hDlg, cbApplyPos), FALSE);
				apiSetForegroundWindow(ghOpWnd);
			} // cbApplyPos
			break;
		case rCascade:
		case rFixed:
			gpSet->wndCascade = (CB == rCascade);
			if (gpSet->isQuakeStyle)
			{
				gpSetCls->UpdatePosSizeEnabled(hDlg);
				EnableWindow(GetDlgItem(hDlg, cbApplyPos), TRUE);
			}
			break;
		case cbUseCurrentSizePos:
			gpSet->isUseCurrentSizePos = IsChecked(hDlg, cbUseCurrentSizePos);
			if (gpSet->isUseCurrentSizePos)
			{
				gpSetCls->UpdateWindowMode(gpConEmu->WindowMode);
				gpSetCls->UpdatePos(gpConEmu->wndX, gpConEmu->wndY, true);
				gpSetCls->UpdateSize(gpConEmu->WndWidth, gpConEmu->WndHeight);
			}
			break;
		case cbAutoSaveSizePos:
			gpSet->isAutoSaveSizePos = IsChecked(hDlg, cbAutoSaveSizePos);
			break;
		case cbFontAuto:
			gpSet->isFontAutoSize = IsChecked(hDlg, cbFontAuto);

			if (gpSet->isFontAutoSize && gpSetCls->LogFont.lfFaceName[0] == L'['
			        && !wcsncmp(gpSetCls->LogFont.lfFaceName+1, CSetDlgFonts::RASTER_FONTS_NAME, _tcslen(CSetDlgFonts::RASTER_FONTS_NAME)))
			{
				gpSet->isFontAutoSize = false;
				checkDlgButton(hDlg, cbFontAuto, BST_UNCHECKED);
				gpSetCls->ShowFontErrorTip(gpSetCls->szRasterAutoError);
			}

			break;
		case cbFixFarBorders:

			//gpSet->isFixFarBorders = !gpSet->isFixFarBorders;
			switch (IsChecked(hDlg, cbFixFarBorders))
			{
				case BST_UNCHECKED:
					gpSet->isFixFarBorders = 0; break;
				case BST_CHECKED:
					gpSet->isFixFarBorders = 1; break;
				case BST_INDETERMINATE:
					gpSet->isFixFarBorders = 2; break;
			}

			gpConEmu->Update(true);
			break;
		//case cbCursorColor:
		//	gpSet->AppStd.isCursorColor = IsChecked(hDlg,cbCursorColor);
		//	gpConEmu->Update(true);
		//	break;
		//case cbCursorBlink:
		//	gpSet->AppStd.isCursorBlink = IsChecked(hDlg,cbCursorBlink);
		//	if (!gpSet->AppStd.isCursorBlink) // если мигание отключается - то курсор может "замереть" в погашенном состоянии.
		//		CVConGroup::InvalidateAll();
		//	break;
		case cbSingleInstance:
			gpSet->isSingleInstance = IsChecked(hDlg, cbSingleInstance);
			break;
		case cbShowHelpTooltips:
			gpSet->isShowHelpTooltips = IsChecked(hDlg, cbShowHelpTooltips);
			break;
		case cbMultiCon:
			gpSet->mb_isMulti = IsChecked(hDlg, cbMultiCon);
			gpConEmu->UpdateWinHookSettings();
			break;
		case cbMultiShowButtons:
			gpSet->isMultiShowButtons = IsChecked(hDlg, cbMultiShowButtons);
			gpConEmu->mp_TabBar->OnShowButtonsChanged();
			break;
		case cbMultiIterate:
			gpSet->isMultiIterate = IsChecked(hDlg, cbMultiIterate);
			break;
		case cbNewConfirm:
			gpSet->isMultiNewConfirm = IsChecked(hDlg, cbNewConfirm);
			break;
		case cbDupConfirm:
			gpSet->isMultiDupConfirm = IsChecked(hDlg, cbDupConfirm);
			break;
		case cbConfirmDetach:
			gpSet->isMultiDetachConfirm = IsChecked(hDlg, cbConfirmDetach);
			break;
		case cbLongOutput:
			gpSet->AutoBufferHeight = IsChecked(hDlg, cbLongOutput);
			gpConEmu->UpdateFarSettings();
			EnableWindow(GetDlgItem(hDlg, tLongOutputHeight), gpSet->AutoBufferHeight);
			break;
		case rbComspecAuto:
		case rbComspecEnvVar:
		case rbComspecCmd:
		case rbComspecExplicit:
			if (IsChecked(hDlg, rbComspecExplicit))
				gpSet->ComSpec.csType = cst_Explicit;
			else if (IsChecked(hDlg, rbComspecCmd))
				gpSet->ComSpec.csType = cst_Cmd;
			else if (IsChecked(hDlg, rbComspecEnvVar))
				gpSet->ComSpec.csType = cst_EnvVar;
			else
				gpSet->ComSpec.csType = cst_AutoTccCmd;
			gpSetCls->EnableDlgItem(hDlg, cbComspecUpdateEnv, (gpSet->ComSpec.csType!=cst_EnvVar));
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbComspecExplicit:
			{
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
			} //cbComspecExplicit
			break;
		case cbComspecTest:
			{
				wchar_t* psz = GetComspec(&gpSet->ComSpec);
				MsgBox(psz ? psz : L"<NULL>", MB_ICONINFORMATION, gpConEmu->GetDefaultTitle(), ghOpWnd);
				SafeFree(psz);
			} // cbComspecTest
			break;
		case rbComspec_OSbit:
		case rbComspec_AppBit:
		case rbComspec_x32:
			if (IsChecked(hDlg, rbComspec_x32))
				gpSet->ComSpec.csBits = csb_x32;
			else if (IsChecked(hDlg, rbComspec_AppBit))
				gpSet->ComSpec.csBits = csb_SameApp;
			else
				gpSet->ComSpec.csBits = csb_SameOS;
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbComspecUpdateEnv:
			gpSet->ComSpec.isUpdateEnv = IsChecked(hDlg, cbComspecUpdateEnv);
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbAddConEmu2Path:
			SetConEmuFlags(gpSet->ComSpec.AddConEmu2Path, CEAP_AddConEmuExeDir, IsChecked(hDlg, cbAddConEmu2Path) ? CEAP_AddConEmuExeDir : CEAP_None);
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbAddConEmuBase2Path:
			SetConEmuFlags(gpSet->ComSpec.AddConEmu2Path, CEAP_AddConEmuBaseDir, IsChecked(hDlg, cbAddConEmuBase2Path) ? CEAP_AddConEmuBaseDir : CEAP_None);
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbComspecUncPaths:
			gpSet->ComSpec.isAllowUncPaths = IsChecked(hDlg, cbComspecUncPaths);
			break;
		case cbCmdAutorunNewWnd:
			// does not insterested in ATM, this is used in ShellIntegration function only
			break;
		case bCmdAutoRegister:
		case bCmdAutoUnregister:
		case bCmdAutoClear:
			gpSetCls->ShellIntegration(hDlg, CSettings::ShellIntgr_CmdAuto, CB==bCmdAutoRegister, CB==bCmdAutoClear);
			gpSetCls->pageOpProc_Integr(hDlg, UM_RELOAD_AUTORUN, UM_RELOAD_AUTORUN, 0);
			break;
		case cbBold:
		case cbItalic:
		case cbFontAsDeviceUnits:
		case cbFontMonitorDpi:
			PostMessage(hDlg, gpSetCls->mn_MsgRecreateFont, wParam, 0);
			break;
		case cbBgImage:
			{
				gpSet->isShowBgImage = IsChecked(hDlg, cbBgImage);
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
			break;
		case rbBgReplaceIndexes:
		case rbBgReplaceTransparent:
			//TODO: ...
			break;
		case cbBgAllowPlugin:
			gpSet->isBgPluginAllowed = IsChecked(hDlg, cbBgAllowPlugin);
			gpSetCls->NeedBackgroundUpdate();
			gpConEmu->Update(true);
			break;
		case cbRClick:
			gpSet->isRClickSendKey = IsChecked(hDlg, cbRClick); //0-1-2
			break;
		case cbSafeFarClose:
			gpSet->isSafeFarClose = IsChecked(hDlg, cbSafeFarClose);
			break;
		case cbMinToTray:
			gpSet->mb_MinToTray = IsChecked(hDlg, cbMinToTray);
			break;
		case cbCloseConsoleConfirm:
			gpSet->isCloseConsoleConfirm = IsChecked(hDlg, cbCloseConsoleConfirm);
			break;
		case cbCloseEditViewConfirm:
			gpSet->isCloseEditViewConfirm = IsChecked(hDlg, cbCloseEditViewConfirm);
			break;
		case cbAlwaysShowTrayIcon:
			gpSet->mb_AlwaysShowTrayIcon = IsChecked(hDlg, cbAlwaysShowTrayIcon);
			Icon.SettingsChanged();
			break;
		case cbQuakeStyle:
		case cbQuakeAutoHide:
			{
				BYTE NewQuakeMode = IsChecked(hDlg, cbQuakeStyle)
					? IsChecked(hDlg, cbQuakeAutoHide) ? 2 : 1 : 0;

				//ConEmuWindowMode NewWindowMode =
				//	IsChecked(hDlg, rMaximized) ? wmMaximized :
				//	IsChecked(hDlg, rFullScreen) ? wmFullScreen :
				//	wmNormal;

				// здесь меняются gpSet->isQuakeStyle, gpSet->isTryToCenter, gpSet->SetMinToTray
				gpConEmu->SetQuakeMode(NewQuakeMode, (ConEmuWindowMode)gpSet->_WindowMode, true);
			}
			break;
		case cbHideCaption:
			gpSet->isHideCaption = IsChecked(hDlg, cbHideCaption);
			if (!gpSet->isQuakeStyle && gpConEmu->isZoomed())
			{
				gpConEmu->OnHideCaption();
				apiSetForegroundWindow(ghOpWnd);
			}
			break;
		case cbHideCaptionAlways:
			gpSet->SetHideCaptionAlways(0!=IsChecked(hDlg, cbHideCaptionAlways));

			if (gpSet->isHideCaptionAlways())
			{
				checkDlgButton(hDlg, cbHideCaptionAlways, BST_CHECKED);
				//TODO("показать тултип, что скрытие обязательно при прозрачности");
			}
			EnableWindow(GetDlgItem(hDlg, cbHideCaptionAlways), !gpSet->isForcedHideCaptionAlways());

			gpConEmu->OnHideCaption();
			apiSetForegroundWindow(ghOpWnd);
			break;
		case cbHideChildCaption:
			gpSet->isHideChildCaption = IsChecked(hDlg, CB);
			gpConEmu->OnSize(true);
			break;
		case cbFARuseASCIIsort:
			gpSet->isFARuseASCIIsort = IsChecked(hDlg, cbFARuseASCIIsort);
			gpConEmu->UpdateFarSettings();
			break;
		case cbShellNoZoneCheck:
			gpSet->isShellNoZoneCheck = IsChecked(hDlg, cbShellNoZoneCheck);
			gpConEmu->UpdateFarSettings();
			break;
		case cbKeyBarRClick:
			gpSet->isKeyBarRClick = IsChecked(hDlg, cbKeyBarRClick);
			break;
		case cbDragPanel:
			gpSet->isDragPanel = IsChecked(hDlg, cbDragPanel);
			gpConEmu->OnSetCursor();
			break;
		case cbDragPanelBothEdges:
			gpSet->isDragPanelBothEdges = IsChecked(hDlg, cbDragPanelBothEdges);
			gpConEmu->OnSetCursor();
			break;
		case cbTryToCenter:
			gpSet->isTryToCenter = IsChecked(hDlg, cbTryToCenter);
			// ресайзим консоль, иначе после включения/отключения PAD-size
			// размер консоли не изменится и она отрисуется с некорректным размером
			gpConEmu->OnSize(true);
			gpConEmu->InvalidateAll();
			break;
		case cbIntegralSize:
			gpSet->mb_IntegralSize = (IsChecked(hDlg, cbIntegralSize) == BST_UNCHECKED);
			break;
		case rbScrollbarHide:
		case rbScrollbarShow:
		case rbScrollbarAuto:
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
			break;
		case cbFarHourglass:
			gpSet->isFarHourglass = IsChecked(hDlg, cbFarHourglass);
			gpConEmu->OnSetCursor();
			break;
		case cbExtendUCharMap:
			gpSet->isExtendUCharMap = IsChecked(hDlg, cbExtendUCharMap);
			gpConEmu->Update(true);
			break;
		case cbFixAltOnAltTab:
			gpSet->isFixAltOnAltTab = IsChecked(hDlg, cbFixAltOnAltTab);
			break;
		case cbAutoRegFonts:
			gpSet->isAutoRegisterFonts = IsChecked(hDlg, cbAutoRegFonts);
			break;
		case cbDebugSteps:
			gpSet->isDebugSteps = IsChecked(hDlg, cbDebugSteps);
			break;
		case cbDragL:
		case cbDragR:
			gpSet->isDragEnabled =
			    (IsChecked(hDlg, cbDragL) ? DRAG_L_ALLOWED : 0) |
			    (IsChecked(hDlg, cbDragR) ? DRAG_R_ALLOWED : 0);
			break;
		case cbDropEnabled:
			gpSet->isDropEnabled = IsChecked(hDlg, cbDropEnabled);
			break;
		case cbDnDCopy:
			gpSet->isDefCopy = IsChecked(hDlg, cbDnDCopy) == BST_CHECKED;
			break;
		case cbDropUseMenu:
			gpSet->isDropUseMenu = IsChecked(hDlg, cbDropUseMenu);
			break;
		case cbDragImage:
			gpSet->isDragOverlay = IsChecked(hDlg, cbDragImage);
			break;
		case cbDragIcons:
			gpSet->isDragShowIcons = IsChecked(hDlg, cbDragIcons) == BST_CHECKED;
			break;
		case cbEnhanceGraphics: // Progressbars and scrollbars
			gpSet->isEnhanceGraphics = IsChecked(hDlg, cbEnhanceGraphics);
			gpConEmu->Update(true);
			break;
		case cbEnhanceButtons: // Buttons, CheckBoxes and RadioButtons
			gpSet->isEnhanceButtons = IsChecked(hDlg, cbEnhanceButtons);
			gpConEmu->Update(true);
			break;
		//case cbTabs:
		case rbTabsNone:
		case rbTabsAlways:
		case rbTabsAuto:

			if (IsChecked(hDlg, rbTabsAuto))
			{
				gpSet->isTabs = 2;
			}
			else if (IsChecked(hDlg, rbTabsAlways))
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

			//switch (IsChecked(hDlg, cbTabs))
			//{
			//	case BST_UNCHECKED:
			//		gpSet->isTabs = 0; break;
			//	case BST_CHECKED:
			//		gpSet->isTabs = 1; break;
			//	case BST_INDETERMINATE:
			//		gpSet->isTabs = 2; break;
			//}

			//TODO("Хорошо бы сразу видимость табов менять");
			////gpConEmu->mp_TabBar->Update(TRUE); -- это как-то неправильно работает.
			break;
		case cbTabsLocationBottom:
			gpSet->nTabsLocation = IsChecked(hDlg, cbTabsLocationBottom);
			gpConEmu->OnSize();
			break;
		case cbOneTabPerGroup:
			gpSet->isOneTabPerGroup = IsChecked(hDlg, cbOneTabPerGroup);
			gpConEmu->mp_TabBar->Update(TRUE);
			break;
		case cbActivateSplitMouseOver:
			GetCursorPos(&gpConEmu->mouse.ptLastSplitOverCheck);
			gpSet->bActivateSplitMouseOver = IsChecked(hDlg, cbActivateSplitMouseOver);
			gpConEmu->OnActivateSplitChanged();
			break;
		case cbTabSelf:
			gpSet->isTabSelf = IsChecked(hDlg, cbTabSelf);
			break;
		case cbTabRecent:
			gpSet->isTabRecent = IsChecked(hDlg, cbTabRecent);
			break;
		case cbTabLazy:
			gpSet->isTabLazy = IsChecked(hDlg, cbTabLazy);
			break;
		//case cbTabsOnTaskBar:
		case cbTaskbarShield:
			gpSet->isTaskbarShield = IsChecked(hDlg, CB);
			gpConEmu->Taskbar_UpdateOverlay();
			break;
		case cbTaskbarProgress:
			gpSet->isTaskbarProgress = IsChecked(hDlg, CB);
			gpConEmu->UpdateProgress();
			break;
		case rbTaskbarBtnActive:
		case rbTaskbarBtnAll:
		case rbTaskbarBtnWin7:
		case rbTaskbarBtnHidden:
			// 3state: BST_UNCHECKED/BST_CHECKED/BST_INDETERMINATE
			gpSet->m_isTabsOnTaskBar = IsChecked(hDlg, rbTaskbarBtnAll) ? 1
				: IsChecked(hDlg, rbTaskbarBtnWin7) ? 2
				: IsChecked(hDlg, rbTaskbarBtnHidden) ? 3 : 0;
			if ((gpSet->m_isTabsOnTaskBar == 3) && !gpSet->mb_MinToTray)
			{
				gpSet->SetMinToTray(true);
			}
			gpConEmu->OnTaskbarSettingsChanged();
			break;
		case cbRSelectionFix:
			gpSet->isRSelFix = IsChecked(hDlg, cbRSelectionFix);
			break;
		case cbEnableMouse:
			gpSet->isDisableMouse = IsChecked(hDlg, cbEnableMouse) ? false : true;
			break;
		case cbSkipActivation:
			gpSet->isMouseSkipActivation = IsChecked(hDlg, cbSkipActivation);
			break;
		case cbSkipMove:
			gpSet->isMouseSkipMoving = IsChecked(hDlg, cbSkipMove);
			break;
		case cbMonitorConsoleLang:
			// "|2" reserved for "One layout for all consoles", always on
			gpSet->isMonitorConsoleLang = IsChecked(hDlg, cbMonitorConsoleLang) ? 3 : 0;
			break;
		case cbSkipFocusEvents:
			gpSet->isSkipFocusEvents = IsChecked(hDlg, cbSkipFocusEvents);
			break;
		case cbMonospace:
			{
				#ifdef _DEBUG
				BYTE cMonospaceNow = gpSet->isMonospace;
				#endif
				gpSet->isMonospace = IsChecked(hDlg, cbMonospace);

				if (gpSet->isMonospace) gpSetCls->isMonospaceSelected = gpSet->isMonospace;

				gpSetCls->mb_IgnoreEditChanged = TRUE;
				gpSetCls->ResetFontWidth();
				gpConEmu->Update(true);
				gpSetCls->mb_IgnoreEditChanged = FALSE;
			} // cbMonospace
			break;
		case cbExtendFonts:
			gpSet->AppStd.isExtendFonts = IsChecked(hDlg, cbExtendFonts);
			gpConEmu->Update(true);
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
			OnButtonClicked_Cursor(hDlg, wParam, lParam, &gpSet->AppStd);
			gpConEmu->Update(true);
			CVConGroup::InvalidateAll();
			break;
		case bBgImage:
			{
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
			break;
		case cbVisible:
			gpSet->isConVisible = IsChecked(hDlg, cbVisible);

			if (gpSet->isConVisible)
			{
				// Если показывать - то только текущую (иначе на экране мешанина консолей будет
				CVConGuard VCon;
				if (CVConGroup::GetActiveVCon(&VCon) >= 0)
					VCon->RCon()->ShowConsole(gpSet->isConVisible);
			}
			else
			{
				// А если скрывать - то все сразу
				for (int i=0; i<MAX_CONSOLE_COUNT; i++)
				{
					CVirtualConsole *pCon = gpConEmu->GetVCon(i);

					if (pCon) pCon->RCon()->ShowConsole(FALSE);
				}
			}

			apiSetForegroundWindow(ghOpWnd);
			break;
			//case cbLockRealConsolePos:
			//	isLockRealConsolePos = IsChecked(hDlg, cbLockRealConsolePos);
			//	break;
		case cbUseInjects:
			gpSet->isUseInjects = IsChecked(hDlg, cbUseInjects);
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbProcessAnsi:
			gpSet->isProcessAnsi = IsChecked(hDlg, cbProcessAnsi);
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbAnsiLog:
			gpSet->isAnsiLog = IsChecked(hDlg, cbAnsiLog);
			break;
		case cbProcessNewConArg:
			gpSet->isProcessNewConArg = IsChecked(hDlg, cbProcessNewConArg);
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbSuppressBells:
			gpSet->isSuppressBells = IsChecked(hDlg, cbSuppressBells);
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbConsoleExceptionHandler:
			gpSet->isConsoleExceptionHandler = IsChecked(hDlg, cbConsoleExceptionHandler);
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbUseClink:
			gpSet->mb_UseClink = IsChecked(hDlg, cbUseClink);
			if (gpSet->mb_UseClink && !gpSet->isUseClink())
			{
				checkDlgButton(hDlg, cbUseClink, BST_UNCHECKED);
				wchar_t szErrInfo[MAX_PATH+200];
				_wsprintf(szErrInfo, SKIPLEN(countof(szErrInfo))
					L"Clink was not found in '%s\\clink'. Download and unpack clink files\nhttp://mridgers.github.io/clink/\n\n"
					L"Note that you don't need to check 'Use clink'\nif you already have set up clink globally.",
					gpConEmu->ms_ConEmuBaseDir);
				MsgBox(szErrInfo, MB_ICONSTOP|MB_SYSTEMMODAL, NULL, ghOpWnd);
			}
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbClinkWebPage:
			ShellExecute(NULL, L"open", L"http://mridgers.github.io/clink/", NULL, NULL, SW_SHOWNORMAL);
			break;
		case cbPortableRegistry:
			#ifdef USEPORTABLEREGISTRY
			gpSet->isPortableReg = IsChecked(hDlg, cbPortableRegistry);
			// Проверить, готов ли к использованию
			if (!gpConEmu->PreparePortableReg())
			{
				gpSet->isPortableReg = false;
				checkDlgButton(hDlg, cbPortableRegistry, BST_UNCHECKED);
			}
			else
			{
				gpConEmu->OnGlobalSettingsChanged();
			}
			#endif
			break;
		case bRealConsoleSettings:
			gpSetCls->EditConsoleFont(ghOpWnd);
			break;
		case cbDesktopMode:
			gpSet->isDesktopMode = IsChecked(hDlg, cbDesktopMode);
			gpConEmu->OnDesktopMode();
			break;
		case cbSnapToDesktopEdges:
			gpSet->isSnapToDesktopEdges = IsChecked(hDlg, cbSnapToDesktopEdges);
			if (gpSet->isSnapToDesktopEdges)
				gpConEmu->OnMoving();
			break;
		case cbAlwaysOnTop:
			gpSet->isAlwaysOnTop = IsChecked(hDlg, cbAlwaysOnTop);
			gpConEmu->OnAlwaysOnTop();
			break;
		case cbSleepInBackground:
			gpSet->isSleepInBackground = IsChecked(hDlg, cbSleepInBackground);
			CVConGroup::OnGuiFocused(TRUE);
			break;
		case cbRetardInactivePanes:
			gpSet->isRetardInactivePanes = IsChecked(hDlg, cbRetardInactivePanes);
			CVConGroup::OnGuiFocused(TRUE);
			break;
		case cbMinimizeOnLoseFocus:
			gpSet->mb_MinimizeOnLoseFocus = IsChecked(hDlg, cbMinimizeOnLoseFocus);
			break;
		case cbFocusInChildWindows:
			gpSet->isFocusInChildWindows = IsChecked(hDlg, cbFocusInChildWindows);
			break;
		case cbDisableFarFlashing:
			gpSet->isDisableFarFlashing = IsChecked(hDlg, cbDisableFarFlashing);
			break;
		case cbDisableAllFlashing:
			gpSet->isDisableAllFlashing = IsChecked(hDlg, cbDisableAllFlashing);
			break;
		case cbShowWasHiddenMsg:
			gpSet->isDownShowHiddenMessage = IsChecked(hDlg, cbShowWasHiddenMsg) ? false : true;
			break;
		case cbTabsInCaption:
			gpSet->isTabsInCaption = IsChecked(hDlg, cbTabsInCaption);
			////RedrawWindow(ghWnd, NULL, NULL, RDW_UPDATENOW|RDW_FRAME);
			////gpConEmu->OnNcMessage(ghWnd, WM_NCPAINT, 0,0);
			//SendMessage(ghWnd, WM_NCACTIVATE, 0, 0);
			//SendMessage(ghWnd, WM_NCPAINT, 0, 0);
			gpConEmu->RedrawFrame();
			break;
		case cbNumberInCaption:
			gpSet->isNumberInCaption = IsChecked(hDlg, cbNumberInCaption);
			gpConEmu->UpdateTitle();
			break;
		case cbAdminShield:
		case cbAdminSuffix:
		{
			BOOL bShield = IsChecked(hDlg, cbAdminShield);
			BOOL bSuffix = IsChecked(hDlg, cbAdminSuffix);
			gpSet->bAdminShield = (bShield && bSuffix) ? ats_ShieldSuffix : bShield ? ats_Shield : bSuffix ? ats_Empty : ats_Disabled;
			if (bSuffix && !*gpSet->szAdminTitleSuffix)
			{
				wcscpy_c(gpSet->szAdminTitleSuffix, DefaultAdminTitleSuffix);
				SetDlgItemText(hDlg, tAdminSuffix, gpSet->szAdminTitleSuffix);
			}
			gpConEmu->mp_TabBar->Update(TRUE);
			break;
		}
		case cbHideInactiveConTabs:
			gpSet->bHideInactiveConsoleTabs = IsChecked(hDlg, cbHideInactiveConTabs);
			gpConEmu->mp_TabBar->Update(TRUE);
			break;
		case cbHideDisabledTabs:
			gpSet->bHideDisabledTabs = IsChecked(hDlg, cbHideDisabledTabs);
			gpConEmu->mp_TabBar->Update(TRUE);
			break;
		case cbShowFarWindows:
			gpSet->bShowFarWindows = IsChecked(hDlg, cbShowFarWindows);
			gpConEmu->mp_TabBar->Update(TRUE);
			break;

		case cbCloseConEmuWithLastTab:
			if (IsChecked(hDlg, CB))
			{
				gpSet->isMultiLeaveOnClose = 0;
			}
			else
			{
				//_ASSERTE(FALSE && "Set up {isMultiLeaveOnClose=1/2}");
				gpSet->isMultiLeaveOnClose = IsChecked(hDlg, cbCloseConEmuOnCrossClicking) ? 2 : 1;
			}
			gpConEmu->LogString(L"isMultiLeaveOnClose changed from dialog (cbCloseConEmuWithLastTab)");

			checkDlgButton(hDlg, cbMinimizeOnLastTabClose, (gpSet->isMultiLeaveOnClose && gpSet->isMultiHideOnClose != 0) ? BST_CHECKED : BST_UNCHECKED);
			checkDlgButton(hDlg, cbHideOnLastTabClose, (gpSet->isMultiLeaveOnClose && gpSet->isMultiHideOnClose == 1) ? BST_CHECKED : BST_UNCHECKED);
			CSettings::EnableDlgItem(hDlg, cbCloseConEmuOnCrossClicking, (gpSet->isMultiLeaveOnClose != 0));
			CSettings::EnableDlgItem(hDlg, cbMinimizeOnLastTabClose, (gpSet->isMultiLeaveOnClose != 0));
			CSettings::EnableDlgItem(hDlg, cbHideOnLastTabClose, (gpSet->isMultiLeaveOnClose != 0) && (gpSet->isMultiHideOnClose != 0));
			break;

		case cbCloseConEmuOnCrossClicking:
			if (!IsChecked(hDlg, cbCloseConEmuWithLastTab))
			{
				//_ASSERTE(FALSE && "Set up {isMultiLeaveOnClose=1/2}");
				gpSet->isMultiLeaveOnClose = IsChecked(hDlg, cbCloseConEmuOnCrossClicking) ? 2 : 1;
				gpConEmu->LogString(L"isMultiLeaveOnClose changed from dialog (cbCloseConEmuOnCrossClicking)");
			}
			break;

		case cbMinimizeOnLastTabClose:
		case cbHideOnLastTabClose:
			if (!IsChecked(hDlg, cbCloseConEmuWithLastTab))
			{
				if (!IsChecked(hDlg, cbMinimizeOnLastTabClose))
				{
					gpSet->isMultiHideOnClose = 0;
				}
				else
				{
					gpSet->isMultiHideOnClose = IsChecked(hDlg, cbHideOnLastTabClose) ? 1 : 2;
				}
				checkDlgButton(hDlg, cbHideOnLastTabClose, (gpSet->isMultiLeaveOnClose && gpSet->isMultiHideOnClose == 1) ? BST_CHECKED : BST_UNCHECKED);
				CSettings::EnableDlgItem(hDlg, cbHideOnLastTabClose, (gpSet->isMultiLeaveOnClose != 0) && (gpSet->isMultiHideOnClose != 0));
			}
			break;

		case rbMinByEscNever:
		case rbMinByEscEmpty:
		case rbMinByEscAlways:
			gpSet->isMultiMinByEsc = (CB == rbMinByEscAlways) ? 1 : (CB == rbMinByEscEmpty) ? 2 : 0;
			EnableWindow(GetDlgItem(hDlg, cbMapShiftEscToEsc), (gpSet->isMultiMinByEsc == 1 /*Always*/));
			break;
		case cbMapShiftEscToEsc:
			gpSet->isMapShiftEscToEsc = IsChecked(hDlg, CB);
			break;

		case cbGuiMacroHelp:
			ConEmuAbout::OnInfo_About(L"Macro");
			break;

		case cbUseWinArrows:
		case cbUseWinNumber:
		case cbUseWinTab:
			switch (CB)
			{
				case cbUseWinArrows:
					gpSet->isUseWinArrows = IsChecked(hDlg, CB);
					break;
				case cbUseWinNumber:
					gpSet->isUseWinNumber = IsChecked(hDlg, CB);
					break;
				case cbUseWinTab:
					gpSet->isUseWinTab = IsChecked(hDlg, CB);
					break;
			}
			gpConEmu->UpdateWinHookSettings();
			break;

		//case cbUseWinNumber:
		//	gpSet->isUseWinNumber = IsChecked(hDlg, cbUseWinNumber);
		//	if (hDlg) checkDlgButton(hDlg, cbUseWinNumberK, gpSet->isUseWinNumber ? BST_CHECKED : BST_UNCHECKED);
		//	gpConEmu->UpdateWinHookSettings();
		//	break;

		case cbSendAltTab:
		case cbSendAltEsc:
		case cbSendAltPrintScrn:
		case cbSendPrintScrn:
		case cbSendCtrlEsc:
			switch (CB)
			{
				case cbSendAltTab:
					gpSet->isSendAltTab = IsChecked(hDlg, cbSendAltTab); break;
				case cbSendAltEsc:
					gpSet->isSendAltEsc = IsChecked(hDlg, cbSendAltEsc); break;
				case cbSendAltPrintScrn:
					gpSet->isSendAltPrintScrn = IsChecked(hDlg, cbSendAltPrintScrn); break;
				case cbSendPrintScrn:
					gpSet->isSendPrintScrn = IsChecked(hDlg, cbSendPrintScrn); break;
				case cbSendCtrlEsc:
					gpSet->isSendCtrlEsc = IsChecked(hDlg, cbSendCtrlEsc); break;
			}
			gpConEmu->UpdateWinHookSettings();
			break;

		case rbHotkeysAll:
		case rbHotkeysUser:
		case rbHotkeysSystem:
		case rbHotkeysMacros:
		case cbHotkeysAssignedOnly:
			gpSetCls->FillHotKeysList(hDlg, TRUE);
			gpSetCls->OnHotkeysNotify(hDlg, MAKELONG(lbConEmuHotKeys,0xFFFF), 0);
			break;


		case cbInstallKeybHooks:
			switch (IsChecked(hDlg,cbInstallKeybHooks))
			{
					// Разрешено
				case BST_CHECKED: gpSet->m_isKeyboardHooks = 1; gpConEmu->RegisterHooks(); break;
					// Запрещено
				case BST_UNCHECKED: gpSet->m_isKeyboardHooks = 2; gpConEmu->UnRegisterHooks(); break;
					// Запрос при старте
				case BST_INDETERMINATE: gpSet->m_isKeyboardHooks = 0; break;
			}

			break;
		case cbDosBox:
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
			break;

		case bApplyViewSettings:
			gpConEmu->OnPanelViewSettingsChanged();
			//gpConEmu->UpdateGuiInfoMapping();
			break;
		case cbThumbLoadFiles:

			switch (IsChecked(hDlg,CB))
			{
				case BST_CHECKED:       gpSet->ThSet.bLoadPreviews = 3; break;
				case BST_INDETERMINATE: gpSet->ThSet.bLoadPreviews = 1; break;
				default: gpSet->ThSet.bLoadPreviews = 0;
			}

			break;
		case cbThumbLoadFolders:
			gpSet->ThSet.bLoadFolders = IsChecked(hDlg, CB);
			break;
		case cbThumbUsePicView2:
			gpSet->ThSet.bUsePicView2 = IsChecked(hDlg, CB);
			break;
		case cbThumbRestoreOnStartup:
			gpSet->ThSet.bRestoreOnStartup = IsChecked(hDlg, CB);
			break;
		case cbThumbPreviewBox:
			gpSet->ThSet.nPreviewFrame = IsChecked(hDlg, CB);
			break;
		case cbThumbSelectionBox:
			gpSet->ThSet.nSelectFrame = IsChecked(hDlg, CB);
			break;
		case rbThumbBackColorIdx: case rbThumbBackColorRGB:
			gpSet->ThSet.crBackground.UseIndex = IsChecked(hDlg, rbThumbBackColorIdx);
			CSettings::InvalidateCtrl(GetDlgItem(hDlg, c32), TRUE);
			break;
		case rbThumbPreviewBoxColorIdx: case rbThumbPreviewBoxColorRGB:
			gpSet->ThSet.crPreviewFrame.UseIndex = IsChecked(hDlg, rbThumbPreviewBoxColorIdx);
			CSettings::InvalidateCtrl(GetDlgItem(hDlg, c33), TRUE);
			break;
		case rbThumbSelectionBoxColorIdx: case rbThumbSelectionBoxColorRGB:
			gpSet->ThSet.crSelectFrame.UseIndex = IsChecked(hDlg, rbThumbSelectionBoxColorIdx);
			CSettings::InvalidateCtrl(GetDlgItem(hDlg, c34), TRUE);
			break;

		case cbActivityReset:
			{
				ListView_DeleteAllItems(GetDlgItem(hDlg, lbActivityLog));
				//wchar_t szText[2]; szText[0] = 0;
				//HWND hDetails = GetDlgItem(hDlg, lbActivityDetails);
				//ListView_SetItemText(hDetails, 0, 1, szText);
				//ListView_SetItemText(hDetails, 1, 1, szText);
				SetDlgItemText(hDlg, ebActivityApp, L"");
				SetDlgItemText(hDlg, ebActivityParm, L"");
			} // cbActivityReset
			break;
		case cbActivitySaveAs:
			{
				gpSetCls->OnSaveActivityLogFile(GetDlgItem(hDlg, lbActivityLog));
			} // cbActivitySaveAs
			break;
		case rbActivityDisabled:
		case rbActivityShell:
		case rbActivityInput:
		case rbActivityCmd:
		case rbActivityAnsi:
			{
				HWND hList = GetDlgItem(hDlg, lbActivityLog);
				//HWND hDetails = GetDlgItem(hDlg, lbActivityDetails);
				switch (LOWORD(wParam))
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
				for (int c = 0; (c <= 40) && ListView_DeleteColumn(hList, 0); c++);
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
			}; // rbActivityShell
			break;

		case cbExtendColors:
			gpSet->AppStd.isExtendColors = IsChecked(hDlg, cbExtendColors) == BST_CHECKED ? true : false;

			for(int i=16; i<32; i++) //-V112
				EnableWindow(GetDlgItem(hDlg, tc0+i), gpSet->AppStd.isExtendColors);

			EnableWindow(GetDlgItem(hDlg, lbExtendIdx), gpSet->AppStd.isExtendColors);

			if (lParam)
			{
				gpConEmu->Update(true);
			}

			break;
		case cbColorSchemeSave:
		case cbColorSchemeDelete:
			{
				HWND hList = GetDlgItem(hDlg, lbDefaultColors);
				int nLen = GetWindowTextLength(hList);
				if (nLen < 1)
					break;
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
				// Перетряхнуть
				gpSetCls->OnInitDialog_Color(hDlg);
			} // cbColorSchemeSave, cbColorSchemeDelete
			break;
		case cbTrueColorer:
			gpSet->isTrueColorer = IsChecked(hDlg, cbTrueColorer);
			gpConEmu->UpdateFarSettings();
			gpConEmu->Update(true);
			break;
		case cbFadeInactive:
			gpSet->isFadeInactive = IsChecked(hDlg, cbFadeInactive);
			CVConGroup::InvalidateAll();
			break;
		case cbTransparent:
			{
				int newV = gpSet->nTransparent;

				if (IsChecked(hDlg, cbTransparent))
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
					SendDlgItemMessage(hDlg, slTransparent, TBM_SETPOS, (WPARAM) true, (LPARAM)gpSet->nTransparent);
					if (!gpSet->isTransparentSeparate)
						SendDlgItemMessage(hDlg, slTransparentInactive, TBM_SETPOS, (WPARAM) true, (LPARAM) gpSet->nTransparent);
					gpConEmu->OnTransparent();
				}
			} break;
		case cbTransparentSeparate:
			{
				gpSet->isTransparentSeparate = IsChecked(hDlg, cbTransparentSeparate);
				//EnableWindow(GetDlgItem(hDlg, cbTransparentInactive), gpSet->isTransparentSeparate);
				EnableWindow(GetDlgItem(hDlg, slTransparentInactive), gpSet->isTransparentSeparate);
				EnableWindow(GetDlgItem(hDlg, stTransparentInactive1), gpSet->isTransparentSeparate);
				EnableWindow(GetDlgItem(hDlg, stTransparentInactive2), gpSet->isTransparentSeparate);
				//checkDlgButton(hDlg, cbTransparentInactive, (gpSet->nTransparentInactive!=MAX_ALPHA_VALUE) ? BST_CHECKED : BST_UNCHECKED);
				SendDlgItemMessage(hDlg, slTransparentInactive, TBM_SETPOS, (WPARAM) true, (LPARAM) gpSet->isTransparentSeparate ? gpSet->nTransparentInactive : gpSet->nTransparent);
				gpConEmu->OnTransparent();
			} break;
		//case cbTransparentInactive:
		//	{
		//		int newV = gpSet->nTransparentInactive;
		//		if (IsChecked(hDlg, cbTransparentInactive))
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
			{
				gpSet->isUserScreenTransparent = IsChecked(hDlg, cbUserScreenTransparent);

				if (hDlg) checkDlgButton(hDlg, cbHideCaptionAlways, gpSet->isHideCaptionAlways() ? BST_CHECKED : BST_UNCHECKED);
				EnableWindow(GetDlgItem(hDlg, cbHideCaptionAlways), !gpSet->isForcedHideCaptionAlways());

				gpConEmu->OnHideCaption(); // при прозрачности - обязательно скрытие заголовка + кнопки
				gpConEmu->UpdateWindowRgn();
			} break;
		case cbColorKeyTransparent:
			{
				gpSet->isColorKeyTransparent = IsChecked(hDlg, cbColorKeyTransparent);
				gpConEmu->OnTransparent();
			} break;


		/* *** Text selections options *** */
		case cbCTSIntelligent:
			gpSet->isCTSIntelligent = IsChecked(hDlg, cbCTSIntelligent);
			break;
		case rbCTSActAlways: case rbCTSActBufferOnly:
			gpSet->isCTSActMode = (CB==rbCTSActAlways) ? 1 : 2;
			break;
		case rbCopyFmtHtml0: case rbCopyFmtHtml1: case rbCopyFmtHtml2:
			gpSet->isCTSHtmlFormat = (CB - rbCopyFmtHtml0);
			break;
		case cbCTSFreezeBeforeSelect:
			gpSet->isCTSFreezeBeforeSelect = IsChecked(hDlg,CB);
			break;
		case cbCTSAutoCopy:
			gpSet->isCTSAutoCopy = IsChecked(hDlg,CB);
			break;
		case cbCTSIBeam:
			gpSet->isCTSIBeam = IsChecked(hDlg,CB);
			gpConEmu->OnSetCursor();
			break;
		case cbCTSEndOnTyping:
		case cbCTSEndCopyBefore:
			gpSet->isCTSEndOnTyping = IsChecked(hDlg,cbCTSEndOnTyping) ? IsChecked(hDlg,cbCTSEndCopyBefore) ? 1 : 2 : 0;
			EnableWindow(GetDlgItem(hDlg, cbCTSEndOnKeyPress), gpSet->isCTSEndOnTyping!=0);
			EnableWindow(GetDlgItem(hDlg, cbCTSEndCopyBefore), gpSet->isCTSEndOnTyping!=0);
			checkDlgButton(hDlg, cbCTSEndOnKeyPress, gpSet->isCTSEndOnKeyPress);
			break;
		case cbCTSEndOnKeyPress:
			gpSet->isCTSEndOnKeyPress = IsChecked(hDlg,CB);
			break;
		case cbCTSBlockSelection:
			gpSet->isCTSSelectBlock = IsChecked(hDlg,CB);
			gpSetCls->CheckSelectionModifiers(hDlg);
			break;
		case cbCTSTextSelection:
			gpSet->isCTSSelectText = IsChecked(hDlg,CB);
			gpSetCls->CheckSelectionModifiers(hDlg);
			break;
		case cbCTSDetectLineEnd:
			gpSet->AppStd.isCTSDetectLineEnd = IsChecked(hDlg, CB);
			break;
		case cbCTSBashMargin:
			gpSet->AppStd.isCTSBashMargin = IsChecked(hDlg, CB);
			break;
		case cbCTSTrimTrailing:
			gpSet->AppStd.isCTSTrimTrailing = IsChecked(hDlg, CB);
			break;
		case cbCTSClickPromptPosition:
			gpSet->AppStd.isCTSClickPromptPosition = IsChecked(hDlg,CB);
			gpSetCls->CheckSelectionModifiers(hDlg);
			break;
		case cbCTSDeleteLeftWord:
			gpSet->AppStd.isCTSDeleteLeftWord = IsChecked(hDlg,CB);
			break;
		case cbClipShiftIns:
			gpSet->AppStd.isPasteAllLines = IsChecked(hDlg,CB);
			break;
		case cbCTSShiftArrowStartSel:
			gpSet->AppStd.isCTSShiftArrowStart = IsChecked(hDlg,CB);
			break;
		case cbClipCtrlV:
			gpSet->AppStd.isPasteFirstLine = IsChecked(hDlg,CB);
			break;
		case cbClipConfirmEnter:
			gpSet->isPasteConfirmEnter = IsChecked(hDlg,CB);
			break;
		case cbClipConfirmLimit:
			if (IsChecked(hDlg,CB))
			{
				gpSet->nPasteConfirmLonger = gpSet->nPasteConfirmLonger ? gpSet->nPasteConfirmLonger : 200;
			}
			else
			{
				gpSet->nPasteConfirmLonger = 0;
			}
			SetDlgItemInt(hDlg, tClipConfirmLimit, gpSet->nPasteConfirmLonger, FALSE);
			EnableWindow(GetDlgItem(hDlg, tClipConfirmLimit), (gpSet->nPasteConfirmLonger != 0));
			break;
		case cbFarGotoEditor:
			gpSet->isFarGotoEditor = IsChecked(hDlg,CB);
			break;
		case cbHighlightMouseRow:
			gpSet->isHighlightMouseRow = IsChecked(hDlg,CB);
			gpConEmu->Update(true);
			break;
		case cbHighlightMouseCol:
			gpSet->isHighlightMouseCol = IsChecked(hDlg,CB);
			gpConEmu->Update(true);
			break;
		/* *** Text selections options *** */



		/* *** Update settings *** */
		case cbUpdateCheckOnStartup:
			gpSet->UpdSet.isUpdateCheckOnStartup = IsChecked(hDlg, CB);
			break;
		case cbUpdateCheckHourly:
			gpSet->UpdSet.isUpdateCheckHourly = IsChecked(hDlg, CB);
			break;
		case cbUpdateConfirmDownload:
			gpSet->UpdSet.isUpdateConfirmDownload = (IsChecked(hDlg, CB) == BST_UNCHECKED);
			break;
		case rbUpdateStableOnly:
		case rbUpdatePreview:
		case rbUpdateLatestAvailable:
			gpSet->UpdSet.isUpdateUseBuilds = IsChecked(hDlg, rbUpdateStableOnly) ? 1 : IsChecked(hDlg, rbUpdateLatestAvailable) ? 2 : 3;
			break;
		case cbUpdateUseProxy:
			gpSet->UpdSet.isUpdateUseProxy = IsChecked(hDlg, CB);
			{
				UINT nItems[] = {stUpdateProxy, tUpdateProxy, stUpdateProxyUser, tUpdateProxyUser, stUpdateProxyPassword, tUpdateProxyPassword};
				for (size_t i = 0; i < countof(nItems); i++)
				{
					HWND hItem = GetDlgItem(hDlg, nItems[i]);
					if (!hItem)
					{
						_ASSERTE(GetDlgItem(hDlg, nItems[i])!=NULL);
						continue;
					}
					EnableWindow(hItem, gpSet->UpdSet.isUpdateUseProxy);
				}
			}
			break;
		case cbUpdateLeavePackages:
			gpSet->UpdSet.isUpdateLeavePackages = IsChecked(hDlg, CB);
			break;
		case cbUpdateArcCmdLine:
			{
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
			}
			break;
		case cbUpdateDownloadPath:
			{
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
			}
			break;
		/* *** Update settings *** */

		/* *** Command groups *** */
		case cbCmdTasksAdd:
		case cbCmdTasksDel:
		case cbCmdTasksUp:
		case cbCmdTasksDown:
		case cbCmdGroupKey:
		case cbCmdGroupApp:
		case cbCmdTasksParm:
		case cbCmdTasksDir:
		case cbCmdTasksActive:
		case cbCmdTasksReload:
		case cbAddDefaults:
		case cbCmdTaskbarTasks:
		case cbCmdTaskbarCommands:
		case cbCmdTaskbarUpdate:
			return OnButtonClicked_Tasks(hDlg, wParam, lParam);
		/* *** Command groups *** */


		/* *** Default terminal *** */
		case cbDefaultTerminal:
		case cbDefaultTerminalStartup:
		case cbDefaultTerminalTSA:
		case cbDefaultTerminalNoInjects:
		case cbDefaultTerminalUseExisting:
		case rbDefaultTerminalConfAuto:
		case rbDefaultTerminalConfAlways:
		case rbDefaultTerminalConfNever:
			{
				bool bSetupDefaultTerminal = false;

				switch (CB)
				{
				case cbDefaultTerminal:
					gpSet->isSetDefaultTerminal = IsChecked(hDlg, cbDefaultTerminal);
					bSetupDefaultTerminal = gpSet->isSetDefaultTerminal;
					break;
				case cbDefaultTerminalStartup:
				case cbDefaultTerminalTSA:
				case cbDefTermAgressive:
					if ((CB == cbDefaultTerminalStartup || CB == cbDefTermAgressive) && IsChecked(hDlg, CB))
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
					gpSet->isRegisterOnOsStartup = (IsChecked(hDlg, cbDefaultTerminalStartup) != BST_UNCHECKED);
					gpSet->isRegisterOnOsStartupTSA = (IsChecked(hDlg, cbDefaultTerminalTSA) != BST_UNCHECKED);
					gpSet->isRegisterAgressive = (IsChecked(hDlg, cbDefTermAgressive) != BST_UNCHECKED);
					EnableWindow(GetDlgItem(hDlg, cbDefaultTerminalTSA), gpSet->isRegisterOnOsStartup);
					// And update registry
					gpConEmu->mp_DefTrm->CheckRegisterOsStartup();
					break;
				case cbDefaultTerminalNoInjects:
					gpSet->isDefaultTerminalNoInjects = IsChecked(hDlg, cbDefaultTerminalNoInjects);
					break;
				case cbDefaultTerminalUseExisting:
					gpSet->isDefaultTerminalNewWindow = !IsChecked(hDlg, cbDefaultTerminalUseExisting);
					break;
				case rbDefaultTerminalConfAuto:
				case rbDefaultTerminalConfAlways:
				case rbDefaultTerminalConfNever:
					gpSet->nDefaultTerminalConfirmClose =
						IsChecked(hDlg, rbDefaultTerminalConfAuto) ? 0 :
						IsChecked(hDlg, rbDefaultTerminalConfAlways) ? 1 : 2;
					break;
				}

				gpConEmu->mp_DefTrm->ApplyAndSave(true, true);

				if (gpSet->isSetDefaultTerminal && bSetupDefaultTerminal)
				{
					// Change mouse cursor due to long operation
					SetCursor(LoadCursor(NULL,IDC_WAIT));
					// Redraw checkboxes to avoid lags in painting while installing hooks
					RedrawWindow(hDlg, NULL, NULL, RDW_UPDATENOW|RDW_ALLCHILDREN);
					// Инициировать эксплорер, если он еще не был обработан
					gpConEmu->mp_DefTrm->StartGuiDefTerm(true);
					// Вернуть фокус в окно настроек
					SetForegroundWindow(ghOpWnd);
				}
			}
			break;
		/* *** Default terminal *** */


		case bGotoEditorCmd:
			{
				wchar_t szPath[MAX_PATH+1] = {};
				wchar_t szInitialDir[MAX_PATH+1]; GetCurrentDirectory(countof(szInitialDir), szInitialDir);

				LPCWSTR pszTemp = gpSet->sFarGotoEditor;
				CmdArg szExe;
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
			}
			break;

		default:
		{
			if (CB >= c32 && CB <= c34)
			{
				if (gpSetCls->ColorEditDialog(hDlg, CB))
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
				if (gpSetCls->ColorEditDialog(hDlg, CB))
				{
					gpConEmu->mp_Status->UpdateStatusBar(true);
				}
			} // if (CB >= c35 && CB <= c37)
			else if (CB == c38)
			{
				if (gpSetCls->ColorEditDialog(hDlg, CB))
				{
					gpConEmu->OnTransparent();
				}
			} // if (CB == c38)
			else if (CB >= c0 && CB <= CSetDlgColors::MAX_COLOR_EDT_ID)
			{
				if (gpSetCls->ColorEditDialog(hDlg, CB))
				{
					//gpConEmu->m_Back->Refresh();
					gpConEmu->Update(true);
				}
			} // else if (CB >= c0 && CB <= MAX_COLOR_EDT_ID)
			else if (gpSetCls->GetPageId(hDlg) == CSettings::thi_Status)
			{
				/* *** Status bar options *** */
				switch (CB)
				{
				case cbShowStatusBar:
					gpConEmu->StatusCommand(csc_ShowHide, IsChecked(hDlg,CB) ? 1 : 2);
					break;

				case cbStatusVertSep:
					if (IsChecked(hDlg,CB))
						gpSet->isStatusBarFlags |= csf_VertDelim;
					else
						gpSet->isStatusBarFlags &= ~csf_VertDelim;
					gpConEmu->mp_Status->UpdateStatusBar(true);
					break;

				case cbStatusHorzSep:
					if (IsChecked(hDlg,CB))
						gpSet->isStatusBarFlags |= csf_HorzDelim;
					else
						gpSet->isStatusBarFlags &= ~csf_HorzDelim;
					gpConEmu->RecreateControls(false, true, true);
					break;

				case cbStatusVertPad:
					if (!IsChecked(hDlg,CB))
						gpSet->isStatusBarFlags |= csf_NoVerticalPad;
					else
						gpSet->isStatusBarFlags &= ~csf_NoVerticalPad;
					gpConEmu->RecreateControls(false, true, true);
					break;

				case cbStatusSystemColors:
					if (IsChecked(hDlg,CB))
						gpSet->isStatusBarFlags |= csf_SystemColors;
					else
						gpSet->isStatusBarFlags &= ~csf_SystemColors;
					CSetDlgLists::EnableDlgItems(hDlg, CSetDlgLists::eStatusColorIds, !(gpSet->isStatusBarFlags & csf_SystemColors));
					gpConEmu->mp_Status->UpdateStatusBar(true);
				break;

				case cbStatusAddAll:
				case cbStatusAddSelected:
				case cbStatusDelSelected:
				case cbStatusDelAll:
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
					}

					if (bChanged)
					{
						gpSetCls->OnInitDialog_StatusItems(hDlg);
						gpConEmu->mp_Status->UpdateStatusBar(true);
					}
					break;
				}
				//else
				//{
				//	for (size_t i = 0; i < countof(SettingsNS::StatusItems); i++)
				//	{
				//		if (CB == SettingsNS::StatusItems[i].nDlgID)
				//		{
				//			gpSet->isStatusColumnHidden[SettingsNS::StatusItems[i].stItem] = !IsChecked(hDlg,CB);
				//			gpConEmu->mp_Status->UpdateStatusBar(true);
				//			break;
				//		}
				//	}
				//}
				} // switch (CB)
				/* *** Status bar options *** */
			} // else if (GetPageId(hDlg) == CSettings::thi_Status)

			else
			{
				_ASSERTE(FALSE && "Button click was not processed");
			}

		} // default:
	}

	return 0;
}

LRESULT CSetDlgButtons::OnButtonClicked_Cursor(HWND hDlg, WPARAM wParam, LPARAM lParam, Settings::AppSettings* pApp)
{
	WORD CB = LOWORD(wParam);

	switch (CB)
	{
		case rCursorH:
		case rCursorV:
		case rCursorB:
		case rCursorR:
			pApp->CursorActive.CursorType = (CECursorStyle)(CB - rCursorH);
			break;
		//case cbBlockInactiveCursor:
		//	pApp->isCursorBlockInactive = IsChecked(hDlg, cbBlockInactiveCursor);
		//	CVConGroup::InvalidateAll();
		//	break;
		case cbCursorColor:
			pApp->CursorActive.isColor = IsChecked(hDlg,cbCursorColor);
			break;
		case cbCursorBlink:
			pApp->CursorActive.isBlinking = IsChecked(hDlg,cbCursorBlink);
			break;
		case cbCursorIgnoreSize:
			pApp->CursorActive.isFixedSize = IsChecked(hDlg,cbCursorIgnoreSize);
			break;

		case cbInactiveCursor:
			pApp->CursorInactive.Used = IsChecked(hDlg,cbInactiveCursor);
			break;

		case rInactiveCursorH:
		case rInactiveCursorV:
		case rInactiveCursorB:
		case rInactiveCursorR:
			pApp->CursorInactive.CursorType = (CECursorStyle)(CB - rInactiveCursorH);
			break;
		case cbInactiveCursorColor:
			pApp->CursorInactive.isColor = IsChecked(hDlg,cbInactiveCursorColor);
			break;
		case cbInactiveCursorBlink:
			pApp->CursorInactive.isBlinking = IsChecked(hDlg,cbInactiveCursorBlink);
			break;
		case cbInactiveCursorIgnoreSize:
			pApp->CursorInactive.isFixedSize = IsChecked(hDlg,cbInactiveCursorIgnoreSize);
			break;

		default:
			_ASSERTE(FALSE && "Unprocessed ID");
	}

	return 0;
}

LRESULT CSetDlgButtons::OnButtonClicked_Tasks(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	WORD CB = LOWORD(wParam);

	switch (CB)
	{
	case cbCmdTasksAdd:
		{
			int iCount = (int)SendDlgItemMessage(hDlg, lbCmdTasks, LB_GETCOUNT, 0,0);
			if (!gpSet->CmdTaskGet(iCount))
				gpSet->CmdTaskSet(iCount, L"", L"", L"");

			gpSetCls->OnInitDialog_Tasks(hDlg, false);

			SendDlgItemMessage(hDlg, lbCmdTasks, LB_SETCURSEL, iCount, 0);
			gpSetCls->OnComboBox(hDlg, MAKELONG(lbCmdTasks,LBN_SELCHANGE), 0);
		} // cbCmdTasksAdd
		break;

	case cbCmdTasksDel:
		{
			int iCur = (int)SendDlgItemMessage(hDlg, lbCmdTasks, LB_GETCURSEL, 0,0);
			if (iCur < 0)
				break;

			const CommandTasks* p = gpSet->CmdTaskGet(iCur);
			if (!p)
				break;

			bool bIsStartup = false;
			if (gpSet->psStartTasksName && p->pszName && (lstrcmpi(gpSet->psStartTasksName, p->pszName) == 0))
				bIsStartup = true;

			size_t cchMax = (p->pszName ? _tcslen(p->pszName) : 0) + 200;
			wchar_t* pszMsg = (wchar_t*)malloc(cchMax*sizeof(*pszMsg));
			if (!pszMsg)
				break;

			_wsprintf(pszMsg, SKIPLEN(cchMax) L"%sDelete command group %s?",
				bIsStartup ? L"Warning! You about to delete startup task!\n\n" : L"",
				p->pszName ? p->pszName : L"{???}");

			int nBtn = MsgBox(pszMsg, MB_YESNO|(bIsStartup ? MB_ICONEXCLAMATION : MB_ICONQUESTION), NULL, ghOpWnd);
			SafeFree(pszMsg);

			if (nBtn != IDYES)
				break;

			gpSet->CmdTaskSet(iCur, NULL, NULL, NULL);

			if (bIsStartup && gpSet->psStartTasksName)
				*gpSet->psStartTasksName = 0;

			gpSetCls->OnInitDialog_Tasks(hDlg, false);

			int iCount = (int)SendDlgItemMessage(hDlg, lbCmdTasks, LB_GETCOUNT, 0,0);
			SendDlgItemMessage(hDlg, lbCmdTasks, LB_SETCURSEL, min(iCur,(iCount-1)), 0);
			gpSetCls->OnComboBox(hDlg, MAKELONG(lbCmdTasks,LBN_SELCHANGE), 0);

		} // cbCmdTasksDel
		break;

	case cbCmdTasksUp:
	case cbCmdTasksDown:
		{
			int iCur, iChg;
			iCur = (int)SendDlgItemMessage(hDlg, lbCmdTasks, LB_GETCURSEL, 0,0);
			if (iCur < 0)
				break;
			if (CB == cbCmdTasksUp)
			{
				if (!iCur)
					break;
				iChg = iCur - 1;
			}
			else
			{
				iChg = iCur + 1;
				if (iChg >= (int)SendDlgItemMessage(hDlg, lbCmdTasks, LB_GETCOUNT, 0,0))
					break;
			}

			if (!gpSet->CmdTaskXch(iCur, iChg))
				break;

			gpSetCls->OnInitDialog_Tasks(hDlg, false);

			SendDlgItemMessage(hDlg, lbCmdTasks, LB_SETCURSEL, iChg, 0);
		} // cbCmdTasksUp, cbCmdTasksDown
		break;

	case cbCmdGroupKey:
		{
			int iCur = (int)SendDlgItemMessage(hDlg, lbCmdTasks, LB_GETCURSEL, 0,0);
			if (iCur < 0)
				break;
			const CommandTasks* pCmd = gpSet->CmdTaskGet(iCur);
			if (!pCmd)
				break;

			DWORD VkMod = pCmd->HotKey.GetVkMod();
			if (CHotKeyDialog::EditHotKey(ghOpWnd, VkMod))
			{
				gpSet->CmdTaskSetVkMod(iCur, VkMod);
				wchar_t szKey[128] = L"";
				SetDlgItemText(hDlg, tCmdGroupKey, ConEmuHotKey::GetHotkeyName(pCmd->HotKey.GetVkMod(), szKey));
			}
		} // cbCmdGroupKey
		break;

	case cbCmdGroupApp:
		{
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
						gpSetCls->OnEditChanged(hDlg, MAKELPARAM(tCmdGroupCommands,EN_CHANGE), 0);
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
		}
		break;

	case cbCmdTasksParm:
		{
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
		break;

	case cbCmdTasksDir:
		{
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
		}
		break;

	case cbCmdTasksActive:
		{
			wchar_t* pszTasks = CVConGroup::GetTasks(NULL); // вернуть все открытые таски
			if (pszTasks)
			{
				SendDlgItemMessage(hDlg, tCmdGroupCommands, EM_REPLACESEL, TRUE, (LPARAM)pszTasks);
				SafeFree(pszTasks);
			}
		}
		break;

	case cbAddDefaults:
		{
			if (MsgBox(L"Do you want to ADD default tasks in your task list?",
					MB_YESNO|MB_ICONEXCLAMATION, gpConEmu->GetDefaultTitle(), ghOpWnd) != IDYES)
				break;

			// Добавить таски В КОНЕЦ
			CreateDefaultTasks(true);

			// Обновить список на экране
			gpSetCls->OnInitDialog_Tasks(hDlg, true);
		} // case cbAddDefaults:
		break;

	case cbCmdTasksReload:
		{
			if (MsgBox(L"Warning! All unsaved changes will be lost!\n\nReload command groups from settings?",
					MB_YESNO|MB_ICONEXCLAMATION, gpConEmu->GetDefaultTitle(), ghOpWnd) != IDYES)
				break;

			// Обновить группы команд
			gpSet->LoadCmdTasks(NULL, true);

			// Обновить список на экране
			gpSetCls->OnInitDialog_Tasks(hDlg, true);
		} // cbCmdTasksReload
		break;

	case cbCmdTaskbarTasks: // Находится в IDD_SPG_TASKBAR!
		gpSet->isStoreTaskbarkTasks = IsChecked(hDlg, CB);
		break;
	case cbCmdTaskbarCommands: // Находится в IDD_SPG_TASKBAR!
		gpSet->isStoreTaskbarCommands = IsChecked(hDlg, CB);
		break;
	case cbCmdTaskbarUpdate: // Находится в IDD_SPG_TASKBAR!
		if (!gpSet->SaveCmdTasks(NULL))
		{
			LPCWSTR pszMsg = L"Can't save task list to settings!\r\nJump list may be not working!\r\nUpdate Windows 7 task list now?";
			if (MsgBox(pszMsg, MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2, gpConEmu->GetDefaultTitle(), ghOpWnd) != IDYES)
				break; // Обновлять таскбар не будем
		}
		UpdateWin7TaskList(true);
		break;
	}

	return 0;
}
