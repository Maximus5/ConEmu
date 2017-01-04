
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

#include "ConEmu.h"
#include "OptionsClass.h"
#include "SetPgQuake.h"

CSetPgQuake::CSetPgQuake()
{
}

CSetPgQuake::~CSetPgQuake()
{
}

LRESULT CSetPgQuake::OnInitDialog(HWND hDlg, bool abInitial)
{
	// Quake style
	checkDlgButton(hDlg, cbQuakeStyle, gpSet->isQuakeStyle ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbQuakeAutoHide, (gpSet->isQuakeStyle == 2) ? BST_CHECKED : BST_UNCHECKED);
	// Show/Hide/Slide timeout
	SetDlgItemInt(hDlg, tQuakeAnimation, gpSet->nQuakeAnimation, FALSE);

	EnableWindow(GetDlgItem(hDlg, cbQuakeAutoHide), gpSet->isQuakeStyle);

	// Frame hide options
	SetDlgItemInt(hDlg, tHideCaptionAlwaysFrame, gpSet->HideCaptionAlwaysFrame(), TRUE);
	SetDlgItemInt(hDlg, tHideCaptionAlwaysDelay, gpSet->nHideCaptionAlwaysDelay, FALSE);
	SetDlgItemInt(hDlg, tHideCaptionAlwaysDissapear, gpSet->nHideCaptionAlwaysDisappear, FALSE);

	checkDlgButton(hDlg, cbAlwaysOnTop, gpSet->isAlwaysOnTop);

	return 0;
}

LRESULT CSetPgQuake::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case tQuakeAnimation:
		{
			BOOL lbOk = FALSE;
			UINT nNewVal = GetDlgItemInt(hDlg, nCtrlId, &lbOk, FALSE);

			gpSet->nQuakeAnimation = (nNewVal <= QUAKEANIMATION_MAX) ? nNewVal : QUAKEANIMATION_MAX;
		}
		break;

	case tHideCaptionAlwaysFrame:
		{
			BOOL lbOk = FALSE;
			int nNewVal = GetDlgItemInt(hDlg, nCtrlId, &lbOk, TRUE);

			if (lbOk && (gpSet->nHideCaptionAlwaysFrame != ((nNewVal < 0) ? 255 : (BYTE)nNewVal)))
			{
				gpSet->nHideCaptionAlwaysFrame = (nNewVal < 0) ? 255 : (BYTE)nNewVal;
				gpConEmu->OnHideCaption();
				gpConEmu->UpdateWindowRgn();
			}
		}
		break;

	case tHideCaptionAlwaysDelay:
	case tHideCaptionAlwaysDissapear:
		{
			BOOL lbOk = FALSE;
			UINT nNewVal = GetDlgItemInt(hDlg, nCtrlId, &lbOk, FALSE);

			if (lbOk)
			{
				switch (nCtrlId)
				{
				case tHideCaptionAlwaysDelay:
					gpSet->nHideCaptionAlwaysDelay = nNewVal;
					break;
				case tHideCaptionAlwaysDissapear:
					gpSet->nHideCaptionAlwaysDisappear = nNewVal;
					break;
				}
			}
		}
		break;

	default:
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}
