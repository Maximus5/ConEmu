
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
#include "SetPgCursor.h"

CSetPgCursor::CSetPgCursor()
{
}

CSetPgCursor::~CSetPgCursor()
{
}

void CSetPgCursor::InitCursorCtrls(HWND hDlg, const AppSettings* pApp)
{
	checkRadioButton(hDlg, rCursorH, rCursorR, (rCursorH + pApp->CursorActive.CursorType));
	checkDlgButton(hDlg, cbCursorColor, pApp->CursorActive.isColor);
	checkDlgButton(hDlg, cbCursorBlink, pApp->CursorActive.isBlinking);
	checkDlgButton(hDlg, cbCursorIgnoreSize, pApp->CursorActive.isFixedSize);
	SetDlgItemInt(hDlg, tCursorFixedSize, pApp->CursorActive.FixedSize, FALSE);
	SetDlgItemInt(hDlg, tCursorMinSize, pApp->CursorActive.MinSize, FALSE);

	checkDlgButton(hDlg, cbInactiveCursor, pApp->CursorInactive.Used);
	checkRadioButton(hDlg, rInactiveCursorH, rInactiveCursorR, (rInactiveCursorH + pApp->CursorInactive.CursorType));
	checkDlgButton(hDlg, cbInactiveCursorColor, pApp->CursorInactive.isColor);
	checkDlgButton(hDlg, cbInactiveCursorBlink, pApp->CursorInactive.isBlinking);
	checkDlgButton(hDlg, cbInactiveCursorIgnoreSize, pApp->CursorInactive.isFixedSize);
	SetDlgItemInt(hDlg, tInactiveCursorFixedSize, pApp->CursorInactive.FixedSize, FALSE);
	SetDlgItemInt(hDlg, tInactiveCursorMinSize, pApp->CursorInactive.MinSize, FALSE);
}

bool CSetPgCursor::OnEditChangedCursor(HWND hDlg, WORD nCtrlId, AppSettings* pApp)
{
	bool bChanged = false;

	switch (nCtrlId)
	{
		case tCursorFixedSize:
		case tInactiveCursorFixedSize:
		{
			BOOL lbOk = FALSE;
			UINT nNewVal = GetDlgItemInt(hDlg, nCtrlId, &lbOk, FALSE);
			if (lbOk)
			{
				UINT nMinSize = (nCtrlId == tCursorFixedSize) ? CURSORSIZE_MIN : 0;
				UINT nMaxSize = CURSORSIZE_MAX;
				if ((nNewVal >= nMinSize) && (nNewVal <= nMaxSize))
				{
					CECursorType* pCur = (nCtrlId == tCursorFixedSize) ? &pApp->CursorActive : &pApp->CursorInactive;

					if (pCur->FixedSize != nNewVal)
					{
						pCur->FixedSize = nNewVal;
						bChanged = true;
					}
				}
			}

			break;
		} //case tCursorFixedSize, tInactiveCursorFixedSize:

		case tCursorMinSize:
		case tInactiveCursorMinSize:
		{
			BOOL lbOk = FALSE;
			UINT nNewVal = GetDlgItemInt(hDlg, nCtrlId, &lbOk, FALSE);
			if (lbOk)
			{
				UINT nMinSize = (nCtrlId == tCursorMinSize) ? CURSORSIZEPIX_MIN : 0;
				UINT nMaxSize = CURSORSIZEPIX_MAX;
				if ((nNewVal >= nMinSize) && (nNewVal <= nMaxSize))
				{
					CECursorType* pCur = (nCtrlId == tCursorMinSize) ? &pApp->CursorActive : &pApp->CursorInactive;

					if (pCur->MinSize != nNewVal)
					{
						pCur->MinSize = nNewVal;
						bChanged = true;
					}
				}
			}

			break;
		} //case tCursorMinSize, tInactiveCursorMinSize:

		default:
			_ASSERTE(FALSE && "Unprocessed edit");
	}

	return bChanged;
}

LRESULT CSetPgCursor::OnInitDialog(HWND hDlg, bool abInitial)
{
	InitCursorCtrls(hDlg, &gpSet->AppStd);

	return 0;
}

LRESULT CSetPgCursor::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case tCursorFixedSize:
	case tInactiveCursorFixedSize:
	case tCursorMinSize:
	case tInactiveCursorMinSize:
	{
		if (CSetPgCursor::OnEditChangedCursor(hDlg, nCtrlId, &gpSet->AppStd))
		{
			gpConEmu->Update(true);
		}
		break;
	} //case tCursorFixedSize, tInactiveCursorFixedSize, tCursorMinSize, tInactiveCursorMinSize

	default:
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}
