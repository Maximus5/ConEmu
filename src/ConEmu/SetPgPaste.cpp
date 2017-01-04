
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

#include "SetPgPaste.h"

CSetPgPaste::CSetPgPaste()
{
}

CSetPgPaste::~CSetPgPaste()
{
}

LRESULT CSetPgPaste::OnInitDialog(HWND hDlg, bool abInitial)
{
	PasteLinesMode mode;

	mode = gpSet->AppStd.isPasteAllLines;
	checkRadioButton(hDlg, rPasteM1MultiLine, rPasteM1Nothing,
		(mode == plm_FirstLine) ? rPasteM1FirstLine
		: (mode == plm_SingleLine) ? rPasteM1SingleLine
		: (mode == plm_Nothing) ? rPasteM1Nothing
		: rPasteM1MultiLine);

	mode = gpSet->AppStd.isPasteFirstLine;
	checkRadioButton(hDlg, rPasteM2MultiLine, rPasteM2Nothing,
		(mode == plm_MultiLine) ? rPasteM2MultiLine
		: (mode == plm_FirstLine) ? rPasteM2FirstLine
		: (mode == plm_Nothing) ? rPasteM2Nothing
		: rPasteM2SingleLine);

	checkDlgButton(hDlg, cbClipConfirmEnter, gpSet->isPasteConfirmEnter);

	checkDlgButton(hDlg, cbClipConfirmLimit, (gpSet->nPasteConfirmLonger!=0));
	SetDlgItemInt(hDlg, tClipConfirmLimit, gpSet->nPasteConfirmLonger, FALSE);

	return 0;
}

void CSetPgPaste::OnPostLocalize(HWND hDlg)
{
	setCtrlTitleByHotkey(hDlg, gbPasteM1, vkPasteText, L"(", L")");
	setCtrlTitleByHotkey(hDlg, gbPasteM2, vkPasteFirstLine, L"(", L")");
}

// cbClipShiftIns, rPasteM1MultiLine, rPasteM1FirstLine, rPasteM1SingleLine, rPasteM1Nothing
void CSetPgPaste::OnBtn_ClipShiftIns(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rPasteM1MultiLine || CB==rPasteM1FirstLine || CB==rPasteM1SingleLine || CB==rPasteM1Nothing);
	_ASSERTE(uCheck);

	switch (CB)
	{
	case rPasteM1MultiLine:
		gpSet->AppStd.isPasteAllLines = plm_Default; break;
	case rPasteM1FirstLine:
		gpSet->AppStd.isPasteAllLines = plm_FirstLine; break;
	case rPasteM1SingleLine:
		gpSet->AppStd.isPasteAllLines = plm_SingleLine; break;
	case rPasteM1Nothing:
		gpSet->AppStd.isPasteAllLines = plm_Nothing; break;

	#if defined(_DEBUG)
	default:
		_ASSERTE(FALSE && "Unsupported option");
	#endif
	}

} // cbClipShiftIns, rPasteM1MultiLine, rPasteM1FirstLine, rPasteM1SingleLine, rPasteM1Nothing


// cbClipCtrlV, rPasteM2MultiLine, rPasteM2FirstLine, rPasteM2SingleLine, rPasteM2Nothing
void CSetPgPaste::OnBtn_ClipCtrlV(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==rPasteM2MultiLine || CB==rPasteM2FirstLine || CB==rPasteM2SingleLine || CB==rPasteM2Nothing);
	_ASSERTE(uCheck);

	switch (CB)
	{
	case rPasteM2MultiLine:
		gpSet->AppStd.isPasteFirstLine = plm_MultiLine; break;
	case rPasteM2FirstLine:
		gpSet->AppStd.isPasteFirstLine = plm_FirstLine; break;
	case rPasteM2SingleLine:
		gpSet->AppStd.isPasteFirstLine = plm_Default; break;
	case rPasteM2Nothing:
		gpSet->AppStd.isPasteFirstLine = plm_Nothing; break;

	#if defined(_DEBUG)
	default:
		_ASSERTE(FALSE && "Unsupported option");
	#endif
	}

} // cbClipCtrlV, rPasteM2MultiLine, rPasteM2FirstLine, rPasteM2SingleLine, rPasteM2Nothing

// cbClipConfirmEnter
void CSetPgPaste::OnBtn_ClipConfirmEnter(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbClipConfirmEnter);

	gpSet->isPasteConfirmEnter = (uCheck != BST_UNCHECKED);

} // cbClipConfirmEnter

// cbClipConfirmLimit
void CSetPgPaste::OnBtn_ClipConfirmLimit(HWND hDlg, WORD CB, BYTE uCheck)
{
	_ASSERTE(CB==cbClipConfirmLimit);

	if (uCheck)
	{
		gpSet->nPasteConfirmLonger = gpSet->nPasteConfirmLonger ? gpSet->nPasteConfirmLonger : 200;
	}
	else
	{
		gpSet->nPasteConfirmLonger = 0;
	}
	SetDlgItemInt(hDlg, tClipConfirmLimit, gpSet->nPasteConfirmLonger, FALSE);
	enableDlgItem(hDlg, tClipConfirmLimit, (gpSet->nPasteConfirmLonger != 0));

} // cbClipConfirmLimit

LRESULT CSetPgPaste::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case tClipConfirmLimit:
	{
		BOOL lbValOk = FALSE;
		gpSet->nPasteConfirmLonger = GetDlgItemInt(hDlg, tClipConfirmLimit, &lbValOk, FALSE);
		if (isChecked(hDlg, cbClipConfirmLimit) != (gpSet->nPasteConfirmLonger!=0))
			checkDlgButton(hDlg, cbClipConfirmLimit, (gpSet->nPasteConfirmLonger!=0));
		if (lbValOk && (gpSet->nPasteConfirmLonger == 0))
		{
			SetFocus(GetDlgItem(hDlg, cbClipConfirmLimit));
			enableDlgItem(hDlg, tClipConfirmLimit, false);
		}
		break;
	} // case tClipConfirmLimit:

	default:
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}
