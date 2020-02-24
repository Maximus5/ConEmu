
/*
Copyright (c) 2014-present Maximus5
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
#include "../UnitTests/gtest.h"

#include "DynDialog.h"

TEST(CDynDialog, LoadTemplate)
{
	UINT nDlgID[] = {
		IDD_SETTINGS, IDD_SPG_FONTS,
		IDD_SPG_COLORS, IDD_SPG_INFO, IDD_SPG_FEATURES, IDD_SPG_DEBUG, IDD_SPG_FEATURE_FAR, IDD_SPG_TASKS,
		IDD_SPG_APPDISTINCT, IDD_SPG_COMSPEC, IDD_SPG_STARTUP, IDD_SPG_STATUSBAR, IDD_SPG_APPDISTINCT2, IDD_SPG_CURSOR,
		IDD_SPG_INTEGRATION, IDD_SPG_TRANSPARENT, IDD_SPG_SIZEPOS, IDD_SPG_MARKCOPY, IDD_SPG_TABS, IDD_SPG_VIEWS,
		IDD_SPG_KEYS, IDD_SPG_UPDATE, IDD_SPG_APPEAR, IDD_SPG_TASKBAR, IDD_SPG_DEFTERM, IDD_SPG_FARMACRO,
		IDD_SPG_KEYBOARD, IDD_SPG_MOUSE,
		IDD_SPG_HIGHLIGHT, IDD_SPG_PASTE, IDD_SPG_CONFIRM, IDD_SPG_HISTORY, IDD_SPG_BACKGR,
		IDD_MORE_CONFONT, IDD_MORE_DOSBOX, IDD_ATTACHDLG, IDD_FAST_CONFIG, IDD_FIND, IDD_ABOUT,
		IDD_RENAMETAB, IDD_CMDPROMPT, IDD_HELP, IDD_ACTION, IDD_HOTKEY,
		0
	};

	extern size_t gMsgBoxCallCount;

	for (int i = 0; nDlgID[i]; i++)
	{
		gMsgBoxCallCount = 0;
		CDynDialog dlg(nDlgID[i]);
		EXPECT_TRUE(dlg.LoadTemplate()) << "dlg_id=" << nDlgID[i];
		EXPECT_EQ(gMsgBoxCallCount, 0) << "dlg_id=" << nDlgID[i];
	}
}
