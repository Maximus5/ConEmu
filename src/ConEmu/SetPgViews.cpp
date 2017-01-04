
/*
Copyright (c) 2016-2017 Maximus5
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

#include "OptionsClass.h"
#include "VConGroup.h"
#include "VirtualConsole.h"
#include "SetDlgLists.h"
#include "SetPgFonts.h"
#include "SetPgViews.h"

CSetPgViews::CSetPgViews()
{
}

CSetPgViews::~CSetPgViews()
{
}

LRESULT CSetPgViews::OnInitDialog(HWND hDlg, bool abInitial)
{
	CVConGuard VCon;
	CVConGroup::GetActiveVCon(&VCon);
	// пока выключим
	EnableWindow(GetDlgItem(hDlg, bApplyViewSettings), VCon.VCon() ? VCon->IsPanelViews() : FALSE);

	SetDlgItemText(hDlg, tThumbsFontName, gpSet->ThSet.Thumbs.sFontName);
	SetDlgItemText(hDlg, tTilesFontName, gpSet->ThSet.Tiles.sFontName);

	if (CSetDlgFonts::EnumFontsFinished())  // Если шрифты уже считаны
	{
		CSetPgFonts* pFonts = NULL;
		if (gpSetCls->GetPageObj(pFonts))
		{
			pFonts->CopyFontsTo(hDlg, tThumbsFontName, tTilesFontName, 0); // можно скопировать список с вкладки [thi_Fonts]
		}
	}

	UINT nVal;

	nVal = gpSet->ThSet.Thumbs.nFontHeight;
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, tThumbsFontSize), CSetDlgLists::eFSizesSmall, nVal, false);

	nVal = gpSet->ThSet.Tiles.nFontHeight;
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, tTilesFontSize), CSetDlgLists::eFSizesSmall, nVal, false);

	SetDlgItemInt(hDlg, tThumbsImgSize, gpSet->ThSet.Thumbs.nImgSize, FALSE);
	SetDlgItemInt(hDlg, tThumbsX1, gpSet->ThSet.Thumbs.nSpaceX1, FALSE);
	SetDlgItemInt(hDlg, tThumbsY1, gpSet->ThSet.Thumbs.nSpaceY1, FALSE);
	SetDlgItemInt(hDlg, tThumbsX2, gpSet->ThSet.Thumbs.nSpaceX2, FALSE);
	SetDlgItemInt(hDlg, tThumbsY2, gpSet->ThSet.Thumbs.nSpaceY2, FALSE);
	SetDlgItemInt(hDlg, tThumbsSpacing, gpSet->ThSet.Thumbs.nLabelSpacing, FALSE);
	SetDlgItemInt(hDlg, tThumbsPadding, gpSet->ThSet.Thumbs.nLabelPadding, FALSE);
	SetDlgItemInt(hDlg, tTilesImgSize, gpSet->ThSet.Tiles.nImgSize, FALSE);
	SetDlgItemInt(hDlg, tTilesX1, gpSet->ThSet.Tiles.nSpaceX1, FALSE);
	SetDlgItemInt(hDlg, tTilesY1, gpSet->ThSet.Tiles.nSpaceY1, FALSE);
	SetDlgItemInt(hDlg, tTilesX2, gpSet->ThSet.Tiles.nSpaceX2, FALSE);
	SetDlgItemInt(hDlg, tTilesY2, gpSet->ThSet.Tiles.nSpaceY2, FALSE);
	SetDlgItemInt(hDlg, tTilesSpacing, gpSet->ThSet.Tiles.nLabelSpacing, FALSE);
	SetDlgItemInt(hDlg, tTilesPadding, gpSet->ThSet.Tiles.nLabelPadding, FALSE);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, tThumbMaxZoom), CSetDlgLists::eThumbMaxZoom, gpSet->ThSet.nMaxZoom, false);

	// Colors
	for(uint c = c32; c <= c34; c++)
		ColorSetEdit(hDlg, c);

	nVal = gpSet->ThSet.crBackground.ColorIdx;
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbThumbBackColorIdx), CSetDlgLists::eColorIdxTh, nVal, false);
	checkRadioButton(hDlg, rbThumbBackColorIdx, rbThumbBackColorRGB,
	                 gpSet->ThSet.crBackground.UseIndex ? rbThumbBackColorIdx : rbThumbBackColorRGB);
	checkDlgButton(hDlg, cbThumbPreviewBox, gpSet->ThSet.nPreviewFrame ? 1 : 0);
	nVal = gpSet->ThSet.crPreviewFrame.ColorIdx;
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbThumbPreviewBoxColorIdx), CSetDlgLists::eColorIdxTh, nVal, false);
	checkRadioButton(hDlg, rbThumbPreviewBoxColorIdx, rbThumbPreviewBoxColorRGB,
	                 gpSet->ThSet.crPreviewFrame.UseIndex ? rbThumbPreviewBoxColorIdx : rbThumbPreviewBoxColorRGB);
	checkDlgButton(hDlg, cbThumbSelectionBox, gpSet->ThSet.nSelectFrame ? 1 : 0);
	nVal = gpSet->ThSet.crSelectFrame.ColorIdx;
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbThumbSelectionBoxColorIdx), CSetDlgLists::eColorIdxTh, nVal, false);
	checkRadioButton(hDlg, rbThumbSelectionBoxColorIdx, rbThumbSelectionBoxColorRGB,
	                 gpSet->ThSet.crSelectFrame.UseIndex ? rbThumbSelectionBoxColorIdx : rbThumbSelectionBoxColorRGB);

	if ((gpSet->ThSet.bLoadPreviews & 3) == 3)
		checkDlgButton(hDlg, cbThumbLoadFiles, BST_CHECKED);
	else if ((gpSet->ThSet.bLoadPreviews & 3) == 1)
		checkDlgButton(hDlg, cbThumbLoadFiles, BST_INDETERMINATE);
	else
		checkDlgButton(hDlg, cbThumbLoadFiles, BST_UNCHECKED);

	checkDlgButton(hDlg, cbThumbLoadFolders, gpSet->ThSet.bLoadFolders);
	SetDlgItemInt(hDlg, tThumbLoadingTimeout, gpSet->ThSet.nLoadTimeout, FALSE);
	checkDlgButton(hDlg, cbThumbUsePicView2, gpSet->ThSet.bUsePicView2);
	checkDlgButton(hDlg, cbThumbRestoreOnStartup, gpSet->ThSet.bRestoreOnStartup);

	return 0;
}

INT_PTR CSetPgViews::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	switch (code)
	{
	case CBN_EDITCHANGE:
		{
			switch (nCtrlId)
			{
				case tThumbsFontName:
					GetDlgItemText(hDlg, nCtrlId, gpSet->ThSet.Thumbs.sFontName, countof(gpSet->ThSet.Thumbs.sFontName)); break;
				case tThumbsFontSize:
					gpSet->ThSet.Thumbs.nFontHeight = GetNumber(hDlg, nCtrlId); break;
				case tTilesFontName:
					GetDlgItemText(hDlg, nCtrlId, gpSet->ThSet.Tiles.sFontName, countof(gpSet->ThSet.Tiles.sFontName)); break;
				case tTilesFontSize:
					gpSet->ThSet.Tiles.nFontHeight = GetNumber(hDlg, nCtrlId); break;
				default:
					_ASSERTE(FALSE && "EditBox was not processed");
			}
			break;
		} // CBN_EDITCHANGE

	case CBN_SELCHANGE:
		{
			UINT val;
			INT_PTR nSel = SendDlgItemMessage(hDlg, nCtrlId, CB_GETCURSEL, 0, 0);

			switch (nCtrlId)
			{
				case lbThumbBackColorIdx:
					gpSet->ThSet.crBackground.ColorIdx = nSel;
					InvalidateCtrl(GetDlgItem(hDlg, c32), TRUE);
					break;
				case lbThumbPreviewBoxColorIdx:
					gpSet->ThSet.crPreviewFrame.ColorIdx = nSel;
					InvalidateCtrl(GetDlgItem(hDlg, c33), TRUE);
					break;
				case lbThumbSelectionBoxColorIdx:
					gpSet->ThSet.crSelectFrame.ColorIdx = nSel;
					InvalidateCtrl(GetDlgItem(hDlg, c34), TRUE);
					break;
				case tThumbsFontName:
					SendDlgItemMessage(hDlg, nCtrlId, CB_GETLBTEXT, nSel, (LPARAM)gpSet->ThSet.Thumbs.sFontName);
					break;
				case tThumbsFontSize:
					if (CSetDlgLists::GetListBoxItem(hDlg, nCtrlId, CSetDlgLists::eFSizesSmall, val))
						gpSet->ThSet.Thumbs.nFontHeight = val;
					break;
				case tTilesFontName:
					SendDlgItemMessage(hDlg, nCtrlId, CB_GETLBTEXT, nSel, (LPARAM)gpSet->ThSet.Tiles.sFontName);
					break;
				case tTilesFontSize:
					if (CSetDlgLists::GetListBoxItem(hDlg, nCtrlId, CSetDlgLists::eFSizesSmall, val))
						gpSet->ThSet.Tiles.nFontHeight = val;
					break;
				case tThumbMaxZoom:
					gpSet->ThSet.nMaxZoom = max(100,((nSel+1)*100));
				default:
					_ASSERTE(FALSE && "ListBox was not processed");
			}

			break;
		} // CBN_SELCHANGE

	} // switch (code)

	return 0;
}

LRESULT CSetPgViews::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	BOOL bValOk = FALSE;
	UINT nVal = GetDlgItemInt(hDlg, nCtrlId, &bValOk, FALSE);

	if (bValOk)
	{
		switch (nCtrlId)
		{
		case tThumbLoadingTimeout:
			gpSet->ThSet.nLoadTimeout = nVal; break;
			//
		case tThumbsImgSize:
			gpSet->ThSet.Thumbs.nImgSize = nVal; break;
			//
		case tThumbsX1:
			gpSet->ThSet.Thumbs.nSpaceX1 = nVal; break;
		case tThumbsY1:
			gpSet->ThSet.Thumbs.nSpaceY1 = nVal; break;
		case tThumbsX2:
			gpSet->ThSet.Thumbs.nSpaceX2 = nVal; break;
		case tThumbsY2:
			gpSet->ThSet.Thumbs.nSpaceY2 = nVal; break;
			//
		case tThumbsSpacing:
			gpSet->ThSet.Thumbs.nLabelSpacing = nVal; break;
		case tThumbsPadding:
			gpSet->ThSet.Thumbs.nLabelPadding = nVal; break;
			// ****************
		case tTilesImgSize:
			gpSet->ThSet.Tiles.nImgSize = nVal; break;
			//
		case tTilesX1:
			gpSet->ThSet.Tiles.nSpaceX1 = nVal; break;
		case tTilesY1:
			gpSet->ThSet.Tiles.nSpaceY1 = nVal; break;
		case tTilesX2:
			gpSet->ThSet.Tiles.nSpaceX2 = nVal; break;
		case tTilesY2:
			gpSet->ThSet.Tiles.nSpaceY2 = nVal; break;
			//
		case tTilesSpacing:
			gpSet->ThSet.Tiles.nLabelSpacing = nVal; break;
		case tTilesPadding:
			gpSet->ThSet.Tiles.nLabelPadding = nVal; break;
		default:
			_ASSERTE(FALSE && "EditBox was not processed");
		} // switch (TB)
	} // if (bValOk)

	if (nCtrlId >= tc32 && nCtrlId <= tc38)
	{
		COLORREF color = 0;

		if (CSetDlgColors::GetColorById(nCtrlId - (tc0-c0), &color))
		{
			if (CSetDlgColors::GetColorRef(hDlg, nCtrlId, &color))
			{
				if (CSetDlgColors::SetColorById(nCtrlId - (tc0-c0), color))
				{
					InvalidateCtrl(GetDlgItem(hDlg, nCtrlId - (tc0-c0)), TRUE);
					// done
				}
			}
		}
		else
		{
			_ASSERTE(FALSE && "EditBox was not processed");
		}
	}

	return 0;
}
