
/*
Copyright (c) 2009-present Maximus5
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


#define DROP_SETCP_ON_WIN2K3R2

// for ConEmuDw.h
#define DEFINE_CONSOLE_EXPORTS

// Иначе не опередяется GetConsoleAliases (хотя он должен быть доступен в Win2k)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#include "../common/defines.h"
#include <TCHAR.h>
#include <Tlhelp32.h>
#include <shlwapi.h>
#include "../common/Common.h"
#include "../common/ConEmuCheck.h"
#include "../ConEmuHk/ConEmuHooks.h"
#include "../ConEmuDW/ConEmuDw.h"
#include "PluginHeader.h"
#include "ConEmuPluginBase.h"

//#ifdef _DEBUG
//	#include <crtdbg.h>
//#else
//	#ifndef _ASSERTE
//		#define _ASSERTE(x)
//	#endif
//#endif


extern HMODULE ghPluginModule;

extern struct HookModeFar gFarMode;


#define kernel32 L"kernel32.dll"
#define user32   L"user32.dll"
#define shell32  L"shell32.dll"



HMODULE ghHooksModule = NULL;
BOOL gbHooksModuleLoaded = FALSE; // TRUE, если был вызов LoadLibrary("ConEmuHk.dll"), тогда его нужно FreeLibrary при выходе
SetHookCallbacks_t SetHookCallbacks = NULL;
SetLoadLibraryCallback_t SetLoadLibraryCallback = NULL;
SetFarHookMode_t SetFarHookMode = NULL;
HMODULE ghExtConModule = NULL;
SetConsoleCallbacks_t SetConsoleCallbacks = NULL;


// Эту функцию нужно позвать из DllMain плагина
bool StartupHooks(HMODULE ahOurDll)
{
	WARNING("Добавить в аргументы строковый параметр - инфа об ошибке");

	if (ghHooksModule == NULL)
	{
		wchar_t szHkModule[64];
		#ifdef WIN64
			wcscpy_c(szHkModule, L"ConEmuHk64.dll");
		#else
			wcscpy_c(szHkModule, L"ConEmuHk.dll");
		#endif
		ghHooksModule = GetModuleHandle(szHkModule);

		if ((ghHooksModule == NULL) && (ghConEmuWndDC != NULL))
		{
			// Попробовать выполнить LoadLibrary? в некоторых случаях GetModuleHandle может обламываться
			ghHooksModule = LoadLibrary(szHkModule);
			if (ghHooksModule)
				gbHooksModuleLoaded = TRUE;
		}

		if (ghHooksModule == NULL)
		{
			if (ghConEmuWndDC != NULL)
			{
				_ASSERTE(ghHooksModule!=NULL);
				wchar_t szErrMsg[128];
				DWORD nErrCode = GetLastError();
				swprintf_c(szErrMsg,
					L"ConEmuHk was not loaded, but ConEmu found!\nFar PID=%u, ErrCode=0x%08X",
					GetCurrentProcessId(), nErrCode);
				MessageBox(NULL, szErrMsg, L"ConEmu plugin", MB_ICONSTOP|MB_SYSTEMMODAL);
			}

			return false;
		}
	}

	_ASSERTE(ghConEmuWndDC!=NULL);

	if (!SetHookCallbacks || !SetLoadLibraryCallback)
	{
		SetHookCallbacks = (SetHookCallbacks_t)GetProcAddress(ghHooksModule, "SetHookCallbacks");
		SetLoadLibraryCallback = (SetLoadLibraryCallback_t)GetProcAddress(ghHooksModule, "SetLoadLibraryCallback");
		SetFarHookMode = (SetFarHookMode_t)GetProcAddress(ghHooksModule, "SetFarHookMode");

		if (!SetHookCallbacks || !SetLoadLibraryCallback || !SetFarHookMode)
		{
			wchar_t szTitle[64], szText[255];
			swprintf_c(szTitle, L"ConEmu plugin, PID=%u", GetCurrentProcessId());
			swprintf_c(szText, L"ConEmuHk is broken, export (%s) not found!",
			          (!SetHookCallbacks) ? L"SetHookCallbacks"
			          : (!SetLoadLibraryCallback) ? L"SetLoadLibraryCallback"
			          : L"SetFarHookMode");
			MessageBox(NULL, szText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
			return FALSE;
		}
	}

	StartupConsoleHooks();

	SetLoadLibraryCallback(ghPluginModule, CPluginBase::OnLibraryLoaded, NULL/*OnLibraryUnLoaded*/);
	SetHookCallbacks("FreeConsole",  kernel32, ghPluginModule, CPluginBase::OnConsoleDetaching, NULL, NULL);
	SetHookCallbacks("AllocConsole", kernel32, ghPluginModule, NULL, CPluginBase::OnConsoleWasAttached, NULL);
	SetHookCallbacks("PeekConsoleInputA", kernel32, ghPluginModule, CPluginBase::OnConsolePeekInput, CPluginBase::OnConsolePeekInputPost, NULL);
	SetHookCallbacks("PeekConsoleInputW", kernel32, ghPluginModule, CPluginBase::OnConsolePeekInput, CPluginBase::OnConsolePeekInputPost, NULL);
	SetHookCallbacks("ReadConsoleInputA", kernel32, ghPluginModule, CPluginBase::OnConsoleReadInput, CPluginBase::OnConsoleReadInputPost, NULL);
	SetHookCallbacks("ReadConsoleInputW", kernel32, ghPluginModule, CPluginBase::OnConsoleReadInput, CPluginBase::OnConsoleReadInputPost, NULL);
	SetHookCallbacks("WriteConsoleOutputA", kernel32, ghPluginModule, CPluginBase::OnWriteConsoleOutput, NULL, NULL);
	SetHookCallbacks("WriteConsoleOutputW", kernel32, ghPluginModule, CPluginBase::OnWriteConsoleOutput, NULL, NULL);
	SetHookCallbacks("GetNumberOfConsoleInputEvents", kernel32, ghPluginModule, NULL, CPluginBase::OnGetNumberOfConsoleInputEventsPost, NULL);
	SetHookCallbacks("ShellExecuteExW", shell32, ghPluginModule, NULL, NULL, CPluginBase::OnShellExecuteExW_Except);
	gFarMode.OnCurDirChanged = CPluginBase::OnCurDirChanged;
	gFarMode.FarVer = gFarVersion;

	SetFarHookMode(&gFarMode);

	return true;
}

void StartupConsoleHooks()
{
	if (!ghExtConModule)
	{
		ghExtConModule = GetModuleHandle(WIN3264TEST(L"ExtendedConsole.dll",L"ExtendedConsole64.dll"));
	}
	if (ghExtConModule && !SetConsoleCallbacks)
	{
		SetConsoleCallbacks = (SetConsoleCallbacks_t)GetProcAddress(ghExtConModule, "SetConsoleCallbacks");
	}

	if (SetConsoleCallbacks)
	{
		HookItemCallback_t callbacks[] = {
			CPluginBase::OnWriteConsoleOutput
			,CPluginBase::OnPostWriteConsoleOutput
		};
		SetConsoleCallbacks(ghPluginModule, countof(callbacks), callbacks);
	}
}


void ShutdownHooks()
{
	if (SetLoadLibraryCallback)
	{
		SetLoadLibraryCallback(ghPluginModule, NULL, NULL);
		SetLoadLibraryCallback = NULL;
	}

	if (SetHookCallbacks)
	{
		SetHookCallbacks("FreeConsole",  kernel32, ghPluginModule, NULL, NULL, NULL);
		SetHookCallbacks("AllocConsole", kernel32, ghPluginModule, NULL, NULL, NULL);
		SetHookCallbacks("PeekConsoleInputA", kernel32, ghPluginModule, NULL, NULL, NULL);
		SetHookCallbacks("PeekConsoleInputW", kernel32, ghPluginModule, NULL, NULL, NULL);
		SetHookCallbacks("ReadConsoleInputA", kernel32, ghPluginModule, NULL, NULL, NULL);
		SetHookCallbacks("ReadConsoleInputW", kernel32, ghPluginModule, NULL, NULL, NULL);
		SetHookCallbacks("WriteConsoleOutputA", kernel32, ghPluginModule, NULL, NULL, NULL);
		SetHookCallbacks("WriteConsoleOutputW", kernel32, ghPluginModule, NULL, NULL, NULL);
		if (SetConsoleCallbacks)
			SetConsoleCallbacks(ghPluginModule, 0, nullptr);
		SetHookCallbacks("GetNumberOfConsoleInputEvents", kernel32, ghPluginModule, NULL, NULL, NULL);
		SetHookCallbacks("ShellExecuteExW", shell32, ghPluginModule, NULL, NULL, NULL);
		SetHookCallbacks = NULL;
	}

	// Если gbHooksModuleLoaded - нужно выполнить FreeLibrary
	if (gbHooksModuleLoaded)
	{
		gbHooksModuleLoaded = FALSE;
		if (ghHooksModule)
		{
			//DWORD dwErr = 0;
			//if (!FreeLibrary(ghHooksModule))
			//{
			//	dwErr = GetLastError();
			//	// Т.к. FreeLibrary перехватывается в ghHooksModule, то первый проход фиктивный
			//	if (dwErr)
			//		FreeLibrary(ghHooksModule);
			//}
			ghHooksModule = NULL;
		}
	}
}
