
/*
Copyright (c) 2009-2014 Maximus5
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


#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после загрузки плагина показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
#endif

#define SHOWDEBUGSTR
//#define MCHKHEAP
#define DEBUGSTRMENU(s) //DEBUGSTR(s)
#define DEBUGSTRINPUT(s) //DEBUGSTR(s)
#define DEBUGSTRDLGEVT(s) //DEBUGSTR(s)
#define DEBUGSTRCMD(s) //DEBUGSTR(s)
#define DEBUGSTRACTIVATE(s) //DEBUGSTR(s)


#include "ConEmuPluginBase.h"
#include "PluginHeader.h"
#include "ConEmuPluginA.h"
#include "ConEmuPlugin995.h"
#include "ConEmuPlugin1900.h"
#include "ConEmuPlugin2800.h"

extern MOUSE_EVENT_RECORD gLastMouseReadEvent;
extern LONG gnDummyMouseEventFromMacro;
extern BOOL gbUngetDummyMouseEvent;

CPluginBase* gpPlugin = NULL;

/* EXPORTS BEGIN */

#define MIN_FAR2_BUILD 1765 // svs 19.12.2010 22:52:53 +0300 - build 1765: Новая команда в FARMACROCOMMAND - MCMD_GETAREA
#define MAKEFARVERSION(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))

int WINAPI GetMinFarVersion()
{
	// Однако, FAR2 до сборки 748 не понимал две версии плагина в одном файле
	bool bFar2 = false;

	if (!LoadFarVersion())
		bFar2 = true;
	else
		bFar2 = gFarVersion.dwVerMajor>=2;

	if (bFar2)
	{
		return MAKEFARVERSION(2,0,MIN_FAR2_BUILD);
	}

	return MAKEFARVERSION(1,71,2470);
}

int GetMinFarVersionW()
{
	return MAKEFARVERSION(2,0,MIN_FAR2_BUILD);
}

/* EXPORTS END */

CPluginBase* Plugin()
{
	if (!gpPlugin)
	{
		if (gFarVersion.dwVerMajor==1)
			gpPlugin = new CPluginAnsi();
		else if (gFarVersion.dwBuild>=FAR_Y2_VER)
			gpPlugin = new CPluginW2800();
		else if (gFarVersion.dwBuild>=FAR_Y1_VER)
			gpPlugin = new CPluginW1900();
		else
			gpPlugin = new CPluginW995();
	}

	return gpPlugin;
}

CPluginBase::CPluginBase()
{
}

CPluginBase::~CPluginBase()
{
}

int CPluginBase::ShowMessageGui(int aiMsg, int aiButtons)
{
	wchar_t wszBuf[MAX_PATH];
	LPCWSTR pwszMsg = GetMsg(aiMsg, wszBuf, countof(wszBuf));

	wchar_t szTitle[128];
	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmu plugin (PID=%u)", GetCurrentProcessId());

	if (!pwszMsg || !*pwszMsg)
	{
		_wsprintf(wszBuf, SKIPLEN(countof(wszBuf)) L"<MsgID=%i>", aiMsg);
		pwszMsg = wszBuf;
	}

	int nRc = MessageBoxW(NULL, pwszMsg, szTitle, aiButtons);
	return nRc;
}

void CPluginBase::PostMacro(const wchar_t* asMacro, INPUT_RECORD* apRec)
{
	if (!asMacro || !*asMacro)
		return;

	_ASSERTE(GetCurrentThreadId()==gnMainThreadId);

	MOUSE_EVENT_RECORD mre;

	if (apRec && apRec->EventType == MOUSE_EVENT)
	{
		gLastMouseReadEvent = mre = apRec->Event.MouseEvent;
	}
	else
	{
		mre = gLastMouseReadEvent;
	}

	PostMacroApi(asMacro, apRec);

	//FAR BUGBUG: Макрос не запускается на исполнение, пока мышкой не дернем :(
	//  Это чаще всего проявляется при вызове меню по RClick
	//  Если курсор на другой панели, то RClick сразу по пассивной
	//  не вызывает отрисовку :(

#if 1
	//111002 - попробуем просто gbUngetDummyMouseEvent
	//InterlockedIncrement(&gnDummyMouseEventFromMacro);
	gnDummyMouseEventFromMacro = TRUE;
	gbUngetDummyMouseEvent = TRUE;
#endif
#if 0
	//if (!mcr.Param.PlainText.Flags) {
	INPUT_RECORD ir[2] = {{MOUSE_EVENT},{MOUSE_EVENT}};

	if (isPressed(VK_CAPITAL))
		ir[0].Event.MouseEvent.dwControlKeyState |= CAPSLOCK_ON;

	if (isPressed(VK_NUMLOCK))
		ir[0].Event.MouseEvent.dwControlKeyState |= NUMLOCK_ON;

	if (isPressed(VK_SCROLL))
		ir[0].Event.MouseEvent.dwControlKeyState |= SCROLLLOCK_ON;

	ir[0].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	ir[0].Event.MouseEvent.dwMousePosition = mre.dwMousePosition;

	// Вроде одного хватало, правда когда {0,0} посылался
	ir[1].Event.MouseEvent.dwControlKeyState = ir[0].Event.MouseEvent.dwControlKeyState;
	ir[1].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	//ir[1].Event.MouseEvent.dwMousePosition.X = 1;
	//ir[1].Event.MouseEvent.dwMousePosition.Y = 1;
	ir[0].Event.MouseEvent.dwMousePosition = mre.dwMousePosition;
	ir[0].Event.MouseEvent.dwMousePosition.X++;

	//2010-01-29 попробуем STD_OUTPUT
	//if (!ghConIn) {
	//	ghConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
	//		0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	//	if (ghConIn == INVALID_HANDLE_VALUE) {
	//		#ifdef _DEBUG
	//		DWORD dwErr = GetLastError();
	//		_ASSERTE(ghConIn!=INVALID_HANDLE_VALUE);
	//		#endif
	//		ghConIn = NULL;
	//		return;
	//	}
	//}
	TODO("Необязательно выполнять реальную запись в консольный буфер. Можно обойтись подстановкой в наших функциях перехвата чтения буфера.");
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	DWORD cbWritten = 0;

	// Вроде одного хватало, правда когда {0,0} посылался
	#ifdef _DEBUG
	BOOL fSuccess =
	#endif
	WriteConsoleInput(hIn/*ghConIn*/, ir, 1, &cbWritten);
	_ASSERTE(fSuccess && cbWritten==1);
	//}
	//InfoW995->AdvControl(InfoW995->ModuleNumber,ACTL_REDRAWALL,NULL);
#endif
}

bool CPluginBase::isMacroActive(int& iMacroActive)
{
	if (!FarHwnd)
	{
		return false;
	}

	if (!iMacroActive)
	{
		iMacroActive = IsMacroActive() ? 1 : 2;
	}

	return (iMacroActive == 1);
}

void CPluginBase::UpdatePanelDirs()
{
	bool bChanged = false;

	_ASSERTE(gPanelDirs.ActiveDir && gPanelDirs.PassiveDir);
	CmdArg* Pnls[] = {gPanelDirs.ActiveDir, gPanelDirs.PassiveDir};

	for (int i = 0; i <= 1; i++)
	{
		GetPanelDirFlags Flags = ((i == 0) ? gpdf_Active : gpdf_Passive) | gpdf_NoPlugin;

		wchar_t* pszDir = GetPanelDir(Flags);

		if (pszDir && (lstrcmp(pszDir, Pnls[i]->ms_Arg ? Pnls[i]->ms_Arg : L"") != 0))
		{
			Pnls[i]->Set(pszDir);
			bChanged = true;
		}
		SafeFree(pszDir);
	}

	if (bChanged)
	{
		// Send to GUI
		SendCurrentDirectory(FarHwnd, gPanelDirs.ActiveDir->ms_Arg, gPanelDirs.PassiveDir->ms_Arg);
	}
}
