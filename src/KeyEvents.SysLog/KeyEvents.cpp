// KeyEvents.cpp : Defines the entry point for the console application.
//

#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include "version.h"

#ifndef MOUSE_HWHEELED
#define MOUSE_HWHEELED 0x0008
#endif

#ifndef CONSOLE_FULLSCREEN_HARDWARE
#define CONSOLE_FULLSCREEN_HARDWARE 2   // console owns the hardware
#endif

#ifndef VK_ICO_00
#define VK_ICO_HELP       0xE3  //  Help key on ICO
#define VK_ICO_00         0xE4  //  00 key on ICO
#define VK_ICO_CLEAR      0xE6
#define VK_OEM_ATTN       0xF0
#define VK_OEM_AUTO       0xF3
#define VK_OEM_AX         0xE1  //  'AX' key on Japanese AX kbd
#define VK_OEM_BACKTAB    0xF5
#define VK_OEM_COPY       0xF2
#define VK_OEM_CUSEL      0xEF
#define VK_OEM_ENLW       0xF4
#define VK_OEM_FINISH     0xF1
#define VK_OEM_JUMP       0xEA
#define VK_OEM_PA1        0xEB
#define VK_OEM_PA2        0xEC
#define VK_OEM_PA3        0xED
#define VK_OEM_RESET      0xE9
#define VK_OEM_WSCTRL     0xEE
#endif

#define countof(a) (sizeof((a))/(sizeof(*(a))))

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);
BOOL MyGetKeyboardState(LPBYTE);
typedef BOOL (__stdcall *FGetConsoleKeyboardLayoutName)(wchar_t*);
DWORD CheckKeyboardLayout();
FGetConsoleKeyboardLayoutName pfnGetConsoleKeyboardLayoutName = NULL;

bool IsFullscreen()
{
	bool Result=false;
	DWORD ModeFlags=0;
	if(GetConsoleDisplayMode(&ModeFlags) && ModeFlags&CONSOLE_FULLSCREEN_HARDWARE)
	{
		Result=true;
	}
	return Result;
}

struct __XXX_Name
{
	int Val;
	const wchar_t *Name;
};

wchar_t* _XXX_ToName(int Val,const wchar_t *Pref,__XXX_Name *arrDef,size_t cntArr,wchar_t* pszName)
{
	for (size_t i=0; i<cntArr; i++)
	{
		if (arrDef[i].Val == Val)
		{
			wsprintfW(pszName, L"\"%s_%s\" [%d/0x%04X]",Pref,arrDef[i].Name,Val,Val);
			return pszName;
		}
	}

	wsprintfW(pszName, L"\"%s_????\" [%d/0x%04X]",Pref,Val,Val);
	return pszName;
}

wchar_t* __VK_KEY_ToName(int VkKey, wchar_t* pszName)
{
#define DEF_VK(k) { VK_##k , _T(#k) }
	__XXX_Name VK[]=
	{
		DEF_VK(ACCEPT),                           DEF_VK(ADD),
		DEF_VK(APPS),                             DEF_VK(ATTN),
		DEF_VK(BACK),                             DEF_VK(BROWSER_BACK),
		DEF_VK(BROWSER_FAVORITES),                DEF_VK(BROWSER_FORWARD),
		DEF_VK(BROWSER_HOME),                     DEF_VK(BROWSER_REFRESH),
		DEF_VK(BROWSER_SEARCH),                   DEF_VK(BROWSER_STOP),
		DEF_VK(CANCEL),                           DEF_VK(CAPITAL),
		DEF_VK(CLEAR),                            DEF_VK(CONTROL),
		DEF_VK(CONVERT),                          DEF_VK(CRSEL),
		DEF_VK(CRSEL),                            DEF_VK(DECIMAL),
		DEF_VK(DELETE),                           DEF_VK(DIVIDE),
		DEF_VK(DOWN),                             DEF_VK(END),
		DEF_VK(EREOF),                            DEF_VK(ESCAPE),
		DEF_VK(EXECUTE),                          DEF_VK(EXSEL),
		DEF_VK(F1),                               DEF_VK(F10),
		DEF_VK(F11),                              DEF_VK(F12),
		DEF_VK(F13),                              DEF_VK(F14),
		DEF_VK(F15),                              DEF_VK(F16),
		DEF_VK(F17),                              DEF_VK(F18),
		DEF_VK(F19),                              DEF_VK(F2),
		DEF_VK(F20),                              DEF_VK(F21),
		DEF_VK(F22),                              DEF_VK(F23),
		DEF_VK(F24),                              DEF_VK(F3),
		DEF_VK(F4),                               DEF_VK(F5),
		DEF_VK(F6),                               DEF_VK(F7),
		DEF_VK(F8),                               DEF_VK(F9),
		DEF_VK(HELP),                             DEF_VK(HOME),
		DEF_VK(ICO_00),                           DEF_VK(ICO_CLEAR),
		DEF_VK(ICO_HELP),                         DEF_VK(INSERT),
		DEF_VK(LAUNCH_APP1),                      DEF_VK(LAUNCH_APP2),
		DEF_VK(LAUNCH_MAIL),                      DEF_VK(LAUNCH_MEDIA_SELECT),
		DEF_VK(LBUTTON),                          DEF_VK(LCONTROL),
		DEF_VK(LEFT),                             DEF_VK(LMENU),
		DEF_VK(LSHIFT),                           DEF_VK(LWIN),
		DEF_VK(MBUTTON),                          DEF_VK(MEDIA_NEXT_TRACK),
		DEF_VK(MEDIA_PLAY_PAUSE),                 DEF_VK(MEDIA_PREV_TRACK),
		DEF_VK(MEDIA_STOP),                       DEF_VK(MENU),
		DEF_VK(MODECHANGE),                       DEF_VK(MULTIPLY),
		DEF_VK(NEXT),                             DEF_VK(NONAME),
		DEF_VK(NONCONVERT),                       DEF_VK(NUMLOCK),
		DEF_VK(NUMPAD0),                          DEF_VK(NUMPAD1),
		DEF_VK(NUMPAD2),                          DEF_VK(NUMPAD3),
		DEF_VK(NUMPAD4),                          DEF_VK(NUMPAD5),
		DEF_VK(NUMPAD6),                          DEF_VK(NUMPAD7),
		DEF_VK(NUMPAD8),                          DEF_VK(NUMPAD9),
		DEF_VK(OEM_1),                            DEF_VK(OEM_102),
		DEF_VK(OEM_2),                            DEF_VK(OEM_3),
		DEF_VK(OEM_4),                            DEF_VK(OEM_5),
		DEF_VK(OEM_6),                            DEF_VK(OEM_7),
		DEF_VK(OEM_8),                            DEF_VK(OEM_ATTN),
		DEF_VK(OEM_AUTO),                         DEF_VK(OEM_AX),
		DEF_VK(OEM_BACKTAB),                      DEF_VK(OEM_CLEAR),
		DEF_VK(OEM_COMMA),                        DEF_VK(OEM_COPY),
		DEF_VK(OEM_CUSEL),                        DEF_VK(OEM_ENLW),
		DEF_VK(OEM_FINISH),                       DEF_VK(OEM_JUMP),
		DEF_VK(OEM_MINUS),                        DEF_VK(OEM_PA1),
		DEF_VK(OEM_PA2),                          DEF_VK(OEM_PA3),
		DEF_VK(OEM_PERIOD),                       DEF_VK(OEM_PLUS),
		DEF_VK(OEM_RESET),                        DEF_VK(OEM_WSCTRL),
		DEF_VK(PA1),                              DEF_VK(PACKET),
		DEF_VK(PAUSE),                            DEF_VK(PLAY),
		DEF_VK(PRINT),                            DEF_VK(PRIOR),
		DEF_VK(PROCESSKEY),                       DEF_VK(RBUTTON),
		DEF_VK(RCONTROL),                         DEF_VK(RETURN),
		DEF_VK(RIGHT),                            DEF_VK(RMENU),
		DEF_VK(RSHIFT),                           DEF_VK(RWIN),
		DEF_VK(SCROLL),                           DEF_VK(SELECT),
		DEF_VK(SEPARATOR),                        DEF_VK(SHIFT),
		DEF_VK(SLEEP),                            DEF_VK(SNAPSHOT),
		DEF_VK(SPACE),                            DEF_VK(SUBTRACT),
		DEF_VK(TAB),                              DEF_VK(UP),
		DEF_VK(VOLUME_DOWN),                      DEF_VK(VOLUME_MUTE),
		DEF_VK(VOLUME_UP),                        DEF_VK(XBUTTON1),
		DEF_VK(XBUTTON2),                         DEF_VK(ZOOM),
	};

	if ((VkKey >= L'0' && VkKey <= L'9') || (VkKey >= L'A' && VkKey <= L'Z'))
	{
		wsprintfW(pszName, L"\"VK_%c\" [%d/0x%04X]",VkKey,VkKey,VkKey);
		return pszName;
	}
	else
		return _XXX_ToName(VkKey,L"VK",VK,countof(VK), pszName);
}

void __MOUSE_EVENT_RECORD_Dump(MOUSE_EVENT_RECORD *rec, wchar_t* pszRecord)
{
	wsprintfW(pszRecord,
	    L"MOUSE_EVENT_RECORD: [%d,%d], Btn=0x%08X (%c%c%c%c%c)\n         Ctrl=0x%08X (%c%c%c%c%c - %c%c%c%c), Flgs=0x%08X (%s)",
	    rec->dwMousePosition.X,
	    rec->dwMousePosition.Y,
	    rec->dwButtonState,
	    (rec->dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED?L'L':L'l'),
	    (rec->dwButtonState&RIGHTMOST_BUTTON_PRESSED?L'R':L'r'),
	    (rec->dwButtonState&FROM_LEFT_2ND_BUTTON_PRESSED?L'2':L' '),
	    (rec->dwButtonState&FROM_LEFT_3RD_BUTTON_PRESSED?L'3':L' '),
	    (rec->dwButtonState&FROM_LEFT_4TH_BUTTON_PRESSED?L'4':L' '),
	    rec->dwControlKeyState,
	    (rec->dwControlKeyState&LEFT_CTRL_PRESSED?L'C':L'c'),
	    (rec->dwControlKeyState&LEFT_ALT_PRESSED?L'A':L'a'),
	    (rec->dwControlKeyState&SHIFT_PRESSED?L'S':L's'),
	    (rec->dwControlKeyState&RIGHT_ALT_PRESSED?L'A':L'a'),
	    (rec->dwControlKeyState&RIGHT_CTRL_PRESSED?L'C':L'c'),
	    (rec->dwControlKeyState&ENHANCED_KEY?L'E':L'e'),
	    (rec->dwControlKeyState&CAPSLOCK_ON?L'C':L'c'),
	    (rec->dwControlKeyState&NUMLOCK_ON?L'N':L'n'),
	    (rec->dwControlKeyState&SCROLLLOCK_ON?L'S':L's'),
	    rec->dwEventFlags,
	    (rec->dwEventFlags==DOUBLE_CLICK?L"(DblClick)":
	     (rec->dwEventFlags==MOUSE_MOVED?L"(Moved)":
	      (rec->dwEventFlags==MOUSE_WHEELED?L"(Wheel)":
	       (rec->dwEventFlags==MOUSE_HWHEELED?L"(HWheel)":L""))))
	);

	if (rec->dwEventFlags==MOUSE_WHEELED  || rec->dwEventFlags==MOUSE_HWHEELED)
	{
		wsprintfW(pszRecord+wcslen(pszRecord),
			L" (Delta=%d)",HIWORD(rec->dwButtonState));
	}
}


void __INPUT_RECORD_Dump(INPUT_RECORD *rec, wchar_t* pszRecord)
{
	switch (rec->EventType)
	{
		case FOCUS_EVENT:
			wsprintfW(pszRecord, 
			    L"FOCUS_EVENT_RECORD: %s",
			    (rec->Event.FocusEvent.bSetFocus?L"TRUE":L"FALSE")
			);
			break;
		case WINDOW_BUFFER_SIZE_EVENT:
			wsprintfW(pszRecord, 
			    L"WINDOW_BUFFER_SIZE_RECORD: Size = [%d, %d]",
			    rec->Event.WindowBufferSizeEvent.dwSize.X,
			    rec->Event.WindowBufferSizeEvent.dwSize.Y
			);
			break;
		case MENU_EVENT:
			wsprintfW(pszRecord, 
			    L"MENU_EVENT_RECORD: CommandId = %d (0x%X) ",
			    rec->Event.MenuEvent.dwCommandId,
			    rec->Event.MenuEvent.dwCommandId
			);
			break;
		case KEY_EVENT:
		case 0:
		{
			WORD AsciiChar = (WORD)(BYTE)rec->Event.KeyEvent.uChar.AsciiChar;
			wchar_t szKeyName[255];
			wsprintfW(pszRecord, 
			    L"%s: %s, %d, Vk=%s, Scan=0x%04X uChar=[U='%c' (0x%04X): A='%C' (0x%02X)]\n         Ctrl=0x%08X (%c%c%c%c%c - %c%c%c%c)",
			    (rec->EventType==KEY_EVENT?L"KEY_EVENT_RECORD":L"NULL_EVENT_RECORD"),
			    (rec->Event.KeyEvent.bKeyDown?L"Dn":L"Up"),
			    rec->Event.KeyEvent.wRepeatCount,
			    __VK_KEY_ToName(rec->Event.KeyEvent.wVirtualKeyCode, szKeyName),
			    rec->Event.KeyEvent.wVirtualScanCode,
			    (rec->Event.KeyEvent.uChar.UnicodeChar && !(rec->Event.KeyEvent.uChar.UnicodeChar == L'\t' || rec->Event.KeyEvent.uChar.UnicodeChar == L'\r' || rec->Event.KeyEvent.uChar.UnicodeChar == L'\n')?rec->Event.KeyEvent.uChar.UnicodeChar:L' '),
			    rec->Event.KeyEvent.uChar.UnicodeChar,
			    (AsciiChar && AsciiChar != '\r' && AsciiChar != '\t' && AsciiChar !='\n' ?AsciiChar:' '),
			    AsciiChar,
			    rec->Event.KeyEvent.dwControlKeyState,
			    (rec->Event.KeyEvent.dwControlKeyState&LEFT_CTRL_PRESSED?L'C':L'c'),
			    (rec->Event.KeyEvent.dwControlKeyState&LEFT_ALT_PRESSED?L'A':L'a'),
			    (rec->Event.KeyEvent.dwControlKeyState&SHIFT_PRESSED?L'S':L's'),
			    (rec->Event.KeyEvent.dwControlKeyState&RIGHT_ALT_PRESSED?L'A':L'a'),
			    (rec->Event.KeyEvent.dwControlKeyState&RIGHT_CTRL_PRESSED?L'C':L'c'),
			    (rec->Event.KeyEvent.dwControlKeyState&ENHANCED_KEY?L'E':L'e'),
			    (rec->Event.KeyEvent.dwControlKeyState&CAPSLOCK_ON?L'C':L'c'),
			    (rec->Event.KeyEvent.dwControlKeyState&NUMLOCK_ON?L'N':L'n'),
			    (rec->Event.KeyEvent.dwControlKeyState&SCROLLLOCK_ON?L'S':L's')
			);
			break;
		}
		case MOUSE_EVENT:
			__MOUSE_EVENT_RECORD_Dump(&rec->Event.MouseEvent, pszRecord);
			break;
		default:
			wsprintfW(pszRecord, 
			    L"??????_EVENT_RECORD: EventType = %d",
			    rec->EventType
			);
			break;
	}

	wsprintfW(pszRecord+wcslen(pszRecord),
		L" (%s)",IsFullscreen()?L"Fullscreen":L"Windowed");
}


#ifdef __GNUC__
int main()
#else
int _tmain(int argc, _TCHAR* argv[])
#endif
{
	INPUT_RECORD r, rl = {};
	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	SetConsoleCtrlHandler(HandlerRoutine, TRUE);
	DWORD dw = 0;
	BOOL lbEscPressed = FALSE;
    HMODULE hKernel = GetModuleHandleW (L"kernel32.dll");
    wchar_t szFormat[1024];
    int WasMouseMoveDup = 0;


	printf("KeyEvents from Maximus5, ver %s. Far SysLog mode\n", KE_VER_STRING);
	    
    if (hKernel)
        pfnGetConsoleKeyboardLayoutName = (FGetConsoleKeyboardLayoutName)GetProcAddress (hKernel, "GetConsoleKeyboardLayoutNameW");

	GetConsoleMode(h, &dw);
	printf("Current console mode = 0x%08X\n\n", (unsigned int)dw);
	if (!(dw & ENABLE_MOUSE_INPUT))
	{
	    SetConsoleMode(h, dw|ENABLE_MOUSE_INPUT);
		GetConsoleMode(h, &dw);
		printf("No ENABLE_MOUSE_INPUT, changing to = 0x%08X\n\n", (unsigned int)dw);
	}

	printf("Press 'Esc' twice to exit program\nPress 'Enter' to insert empty line\n\n");
	
	while (TRUE)
	{
		memset(&r, 0, sizeof(r)); dw = 0;
		if (ReadConsoleInput(h, &r, 1, &dw))
		{
			if ((r.EventType == MOUSE_EVENT)
				&& (rl.EventType == MOUSE_EVENT)
				&& (r.Event.MouseEvent.dwEventFlags == MOUSE_MOVED)
				&& (rl.Event.MouseEvent.dwEventFlags == MOUSE_MOVED))
			{
				WasMouseMoveDup ++;
				wprintf(L"\rSkipping MOUSE_MOVED (%i)", WasMouseMoveDup);
				continue;
			}
			if (WasMouseMoveDup)
			{
				WasMouseMoveDup = 0;
				wprintf(L"\n");
			}
		
			SYSTEMTIME st; GetLocalTime(&st);
			__INPUT_RECORD_Dump(&r, szFormat);
			wprintf(L"%02i:%02i:%02i ", st.wHour,st.wMinute,st.wSecond);
			DWORD nLen = wcslen(szFormat);
			WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), szFormat, nLen, &nLen, NULL);
			wprintf(L"\n");

			if (r.EventType == MOUSE_EVENT)
				rl = r;
			
			if (r.EventType == KEY_EVENT) {
				if (!r.Event.KeyEvent.bKeyDown) {
					if (r.Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
						wprintf(L"\n");
						
					if (r.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
					{
						if (lbEscPressed) return 0;
						lbEscPressed = TRUE;
					} else if (lbEscPressed) {
						lbEscPressed = FALSE;
					}
				}
			}
		}
	}
	return 0;
}

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	SYSTEMTIME st; GetLocalTime(&st);
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
		printf("\n\n%i:%02i:%02i Ctrl-C event\n\n\n",st.wHour,st.wMinute,st.wSecond);
		break;
	case CTRL_BREAK_EVENT:
		printf("\n\n%i:%02i:%02i Ctrl-Break event\n\n\n",st.wHour,st.wMinute,st.wSecond);
		break;
	case CTRL_CLOSE_EVENT:
		printf("\n\n%i:%02i:%02i Close event\n\n\n",st.wHour,st.wMinute,st.wSecond);
		return FALSE;
	case CTRL_LOGOFF_EVENT:
		printf("\n\n%i:%02i:%02i Logoff event\n\n\n",st.wHour,st.wMinute,st.wSecond);
		break;
	case CTRL_SHUTDOWN_EVENT:
		printf("\n\n%i:%02i:%02i Shutdown event\n\n\n",st.wHour,st.wMinute,st.wSecond);
		break;
	}
	return TRUE;
}
