
/*
Copyright (c) 2015-2016 Maximus5
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

#include "Options.h"
#include "PushInfo.h"
#include "TrayIcon.h"
#include "../common/CEStr.h"

static CPushInfo::PushInfo gPushInfo[] = {
	{
		/* Valid for */
		{ 2015, 8, 22 }, { 2015, 8, 27 },
		/* Message for TSA icon */
		L"The article needs your help,\n"
		L"please consider to read issue#289"
		/* Full text for message box, used in About dialog, requires \r\n */
		, L"The article needs your help,\r\n"
		L"please consider to read GitHub issue#289\r\n"
		L"https://github.com/Maximus5/ConEmu/issues/289"
		/* URL to be opened if user press <Yes> in the confirmation dialog */
		, L"https://github.com/Maximus5/ConEmu/issues/289"
	},
	{
		/* Valid for */
		{ 2015, 3, 13 }, { 2015, 4, 8 },
		/* Message for TSA icon */
		L"ConEmu was selected to be on the ballot\n"
		L"of SourceForge.net project of the month!\n"
		L"Project needs your vote!"
		/* Full text for message box, used in About dialog, requires \r\n */
		, L"Please vote for ConEmu! Project needs your help!\r\n"
		L"\r\n"
		L"SourceForge project of the month community choice ballot will end on 2015 April 15.\r\n"
		L"\r\n"
		L"User must be registered at SourceForge.net to vote.\r\n"
		L"Just post in the voting thread: ‘VOTE: conemu’\r\n"
		L"\r\n"
		L"SourceForge.net voting thread URL:\r\n"
		L"https://sourceforge.net/p/potm/discussion/vote/thread/ca38c9ae/\r\n"
		/* URL to be opened if user press <Yes> in the confirmation dialog */
		, L"https://sourceforge.net/p/potm/discussion/vote/thread/ca38c9ae/"
	},
};

#define TSA_MESSAGE_FOOTER L"\n-------------------\nClick here for options..."
#define INFO_MESSAGE_FOOTER L"<Yes> - open browser, <No> - stop buzzing."

CPushInfo::CPushInfo()
	: mp_Active(NULL)
{
	SYSTEMTIME st = {};
	FILETIME ft = {}, ft1 = {}, ft2 = {};

	GetLocalTime(&st);
	SystemTimeToFileTime(&st, &ft);

	int iCmp;
	for (INT_PTR i = 0; i < countof(gPushInfo); i++)
	{
		SYSTEMTIME st1 = {(WORD)gPushInfo[i].dtBegin.wYear, (WORD)gPushInfo[i].dtBegin.wMonth, 0, (WORD)gPushInfo[i].dtBegin.wDay};
		SystemTimeToFileTime(&st1, &ft1);
		SYSTEMTIME st2 = {(WORD)gPushInfo[i].dtEnd.wYear, (WORD)gPushInfo[i].dtEnd.wMonth, 0, (WORD)gPushInfo[i].dtEnd.wDay};
		SystemTimeToFileTime(&st2, &ft2);

		iCmp = CompareFileTime(&ft1, &ft);
		if (iCmp > 0) continue;
		iCmp = CompareFileTime(&ft, &ft2);
		if (iCmp > 0) continue;

		if (gpSet->StopBuzzingDate[0])
		{
			SYSTEMTIME stStop = {}; FILETIME ftStop = {};
			wchar_t* pszEnd, *pszDate = gpSet->StopBuzzingDate;
			if (isDigit(pszDate[0]))
				stStop.wYear = wcstoul(pszDate, &pszEnd, 10);
			if (stStop.wYear && pszEnd && *pszEnd == L'-')
				stStop.wMonth = wcstoul((pszDate = (pszEnd+1)), &pszEnd, 10);
			if (stStop.wMonth && pszEnd && *pszEnd == L'-')
				stStop.wDay = wcstoul((pszDate = (pszEnd+1)), &pszEnd, 10);
			if (stStop.wDay)
			{
				SystemTimeToFileTime(&stStop, &ftStop);
				int iCmp1 = CompareFileTime(&ft1, &ftStop);
				int iCmp2 = CompareFileTime(&ftStop, &ft2);
				if (iCmp1 <= 0 && iCmp2 <= 0)
				{
					// Don't show this notification in TSA
					gPushInfo[i].pszTsaNotify = NULL;
				}
			}
		}

		mp_Active = (gPushInfo + i);
		break;
	}
}

CPushInfo::~CPushInfo()
{
}

bool CPushInfo::ShowNotification()
{
	if (!mp_Active)
		return false;

	if (mp_Active->pszTsaNotify)
	{
		CEStr lsMsg(mp_Active->pszTsaNotify, TSA_MESSAGE_FOOTER);
		Icon.ShowTrayIcon(lsMsg, tsa_Push_Notify);
	}
	return true;
}

void CPushInfo::OnNotificationClick()
{
	if (!mp_Active)
		return;

	bool bOpenUrl = false;

	if (mp_Active->pszFullMessage)
	{
		CEStr lsMsg(mp_Active->pszFullMessage, L"\r\n\r\n", INFO_MESSAGE_FOOTER);
		int iBtn = MsgBox(lsMsg, MB_ICONINFORMATION|MB_YESNOCANCEL);

		switch (iBtn)
		{
		case IDYES:
			bOpenUrl = true;
			break;
		case IDNO:
			gpSet->SaveStopBuzzingDate();
			break;
		}
	}
	else
	{
		bOpenUrl = true;
	}

	if (bOpenUrl)
	{
		OpenNotificationUrl();
	}
}

void CPushInfo::OpenNotificationUrl()
{
	if (!mp_Active || !mp_Active->pszUrl)
		return;
	DWORD shellRc = (DWORD)(INT_PTR)ShellExecute(ghWnd, L"open", mp_Active->pszUrl, NULL, NULL, SW_SHOWNORMAL);
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}
