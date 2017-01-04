
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

#include "SetPgTransparent.h"

CSetPgTransparent::CSetPgTransparent()
{
}

CSetPgTransparent::~CSetPgTransparent()
{
}

LRESULT CSetPgTransparent::OnInitDialog(HWND hDlg, bool abInitial)
{
	// +Color key
	ColorSetEdit(hDlg, c38);

	checkDlgButton(hDlg, cbTransparent, (gpSet->nTransparent!=MAX_ALPHA_VALUE) ? BST_CHECKED : BST_UNCHECKED);
	SendDlgItemMessage(hDlg, slTransparent, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(MIN_ALPHA_VALUE, MAX_ALPHA_VALUE));
	SendDlgItemMessage(hDlg, slTransparent, TBM_SETPOS  , (WPARAM) true, (LPARAM) gpSet->nTransparent);
	checkDlgButton(hDlg, cbTransparentSeparate, gpSet->isTransparentSeparate ? BST_CHECKED : BST_UNCHECKED);
	//EnableWindow(GetDlgItem(hDlg, cbTransparentInactive), gpSet->isTransparentSeparate);
	//checkDlgButton(hDlg, cbTransparentInactive, (gpSet->nTransparentInactive!=MAX_ALPHA_VALUE) ? BST_CHECKED : BST_UNCHECKED);
	EnableWindow(GetDlgItem(hDlg, slTransparentInactive), gpSet->isTransparentSeparate);
	EnableWindow(GetDlgItem(hDlg, stTransparentInactive), gpSet->isTransparentSeparate);
	EnableWindow(GetDlgItem(hDlg, stOpaqueInactive), gpSet->isTransparentSeparate);
	SendDlgItemMessage(hDlg, slTransparentInactive, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(MIN_INACTIVE_ALPHA_VALUE, MAX_ALPHA_VALUE));
	SendDlgItemMessage(hDlg, slTransparentInactive, TBM_SETPOS  , (WPARAM) true, (LPARAM) gpSet->isTransparentSeparate ? gpSet->nTransparentInactive : gpSet->nTransparent);
	checkDlgButton(hDlg, cbUserScreenTransparent, gpSet->isUserScreenTransparent ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbColorKeyTransparent, gpSet->isColorKeyTransparent);

	return 0;
}
