#pragma once

#ifdef MSGLOGGER

void DebugLogFile(LPCSTR asMessage);

void DebugLogMessage(HWND h, UINT m, WPARAM w, LPARAM l, int posted, BOOL extra)
{
	if (bBlockDebugLog || (!bSendToFile && !IsDebuggerPresent()))
		return;

	char szMess[32], szWP[32], szLP[48], szWhole[255];
	//static SYSTEMTIME st;
	szMess[0]=0; szWP[0]=0; szLP[0]=0; szWhole[0]=0;
#define MSG1(s) case s: lstrcpyA(szMess, #s); break;
#define MSG2(s) case s: lstrcpyA(szMess, #s);
#define WP1(s) case s: lstrcpyA(szWP, #s); break;
#define WP2(s) case s: lstrcpyA(szWP, #s);

	switch (m)
	{
			MSG1(WM_NOTIFY);
			MSG1(WM_PAINT);
			MSG1(WM_TIMER);
			MSG2(WM_SIZING);
			{
				switch(w)
				{
						WP1(WMSZ_RIGHT);
						WP1(WMSZ_BOTTOM);
						WP1(WMSZ_BOTTOMRIGHT);
						WP1(WMSZ_LEFT);
						WP1(WMSZ_TOP);
						WP1(WMSZ_TOPLEFT);
						WP1(WMSZ_TOPRIGHT);
						WP1(WMSZ_BOTTOMLEFT);
				}

				break;
			}

			MSG2(WM_SIZE);
			{
				sprintf_c(szLP, "{%ix%i}", GET_X_LPARAM(l), GET_Y_LPARAM(l));
				break;
			}
			MSG2(WM_MOVE);
			{
				sprintf_c(szLP, "{%i,%i}", GET_X_LPARAM(l), GET_Y_LPARAM(l));
				break;
			}
			MSG1(WM_GETMINMAXINFO);
			MSG2(WM_NCCALCSIZE);
			{
				if (l)
				{
					RECT r = w ? ((LPNCCALCSIZE_PARAMS)l)->rgrc[0] : *(LPRECT)l;
					sprintf_c(szLP, "{%i,%i} {%ix%i}", r.left, r.top, LOGRECTSIZE(r));
				}
				break;
			}

			MSG2(WM_WINDOWPOSCHANGING);
			{
				sprintf_c(szLP, "{%i,%i} {%ix%i}", ((LPWINDOWPOS)l)->x, ((LPWINDOWPOS)l)->y, ((LPWINDOWPOS)l)->cx, ((LPWINDOWPOS)l)->cy);
				break;
			}
			MSG2(WM_WINDOWPOSCHANGED);
			{
				sprintf_c(szLP, "{%i,%i} {%ix%i}", ((LPWINDOWPOS)l)->x, ((LPWINDOWPOS)l)->y, ((LPWINDOWPOS)l)->cx, ((LPWINDOWPOS)l)->cy);
				break;
			}

			MSG1(WM_KEYDOWN);
			MSG1(WM_KEYUP);
			MSG1(WM_SYSKEYDOWN);
			MSG1(WM_SYSKEYUP);
			MSG2(WM_MOUSEWHEEL);
			sprintf_c(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
			break;
			MSG2(WM_ACTIVATE);

			switch(w)
			{
					WP1(WA_ACTIVE);
					WP1(WA_CLICKACTIVE);
					WP1(WA_INACTIVE);
			}

			break;
			MSG2(WM_ACTIVATEAPP);

			if (w==0)
				break;

			break;
			MSG2(WM_KILLFOCUS);
			break;
			MSG1(WM_SETFOCUS);
			MSG1(WM_MOUSEACTIVATE);
			MSG2(WM_MOUSEMOVE);
			//sprintf_c(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
			#ifdef MSGLOGGER_MOUSE
			break;
			#else
			return;
			#endif
			MSG2(WM_RBUTTONDOWN);
			sprintf_c(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
			break;
			MSG2(WM_RBUTTONUP);
			sprintf_c(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
			break;
			MSG2(WM_MBUTTONDOWN);
			sprintf_c(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
			break;
			MSG2(WM_MBUTTONUP);
			sprintf_c(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
			break;
			MSG2(WM_LBUTTONDOWN);
			sprintf_c(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
			break;
			MSG2(WM_LBUTTONUP);
			sprintf_c(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
			break;
			MSG2(WM_LBUTTONDBLCLK);
			sprintf_c(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
			break;
			MSG2(WM_MBUTTONDBLCLK);
			sprintf_c(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
			break;
			MSG2(WM_RBUTTONDBLCLK);
			sprintf_c(szLP, "%i:%i", GET_X_LPARAM(l), GET_Y_LPARAM(l));
			break;
			MSG1(WM_CLOSE);
			MSG1(WM_CREATE);
			MSG2(WM_SYSCOMMAND);
			{
				switch (w)
				{
						WP1(SC_MAXIMIZE_SECRET);
						WP2(SC_RESTORE_SECRET);
						break;
						WP1(SC_CLOSE);
						WP1(SC_MAXIMIZE);
						WP2(SC_RESTORE);
						break;
						WP1(SC_PROPERTIES_SECRET);
					#ifdef ID_SETTINGS
						WP1(ID_SETTINGS);
						WP1(ID_HELP);
						WP1(ID_ABOUT);
						WP1(IDM_DONATE_LINK);
						WP1(ID_TOTRAY);
						WP1(ID_TOMONITOR);
						WP1(ID_CONPROP);
					#endif
				}

				break;
			}
			MSG1(WM_NCRBUTTONUP);
			#ifdef WM_TRAYNOTIFY
			MSG1(WM_TRAYNOTIFY);
			#endif
			MSG1(WM_DESTROY);
			MSG1(WM_INPUTLANGCHANGE);
			MSG1(WM_INPUTLANGCHANGEREQUEST);
			MSG1(WM_IME_NOTIFY);
			MSG1(WM_VSCROLL);
			MSG1(WM_NULL);
			//default:
			//  return;
	}

	if (bSendToDebugger || bSendToFile)
	{
		if (!szMess[0]) sprintf_c(szMess, "%i(x%02X)", m, m);

		if (!szWP[0]) sprintf_c(szWP, "%i", (DWORD)w);

		if (!szLP[0]) sprintf_c(szLP, "%i(0x%08X)", (int)l, (DWORD)l);

		LPCSTR pszSrc = (posted < 0) ? "Recv" : (posted ? "Post" : "Send");

		if (bSendToDebugger)
		{
			static SYSTEMTIME st;
			GetLocalTime(&st);
			sprintf_c(szWhole, "%02i:%02i:%02i.%03i %s%s: <%i> %s, %s, %s\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
				pszSrc, (extra ? "+" : ""), gnMessageNestingLevel, szMess, szWP, szLP);
			OutputDebugStringA(szWhole);
		}
		else if (bSendToFile)
		{
			sprintf_c(szWhole, "%s%s: <%i> %s, %s, %s\n",
				pszSrc, (extra ? "+" : ""), gnMessageNestingLevel, szMess, szWP, szLP);
			DebugLogFile(szWhole);
		}
	}
}

void DebugLogPos(HWND hw, int x, int y, int w, int h, LPCSTR asFunc)
{
#ifdef MSGLOGGER

	if (!x && !y && hw == ghConWnd)
	{
		if (!IsDebuggerPresent() && !isPressed(VK_LBUTTON))
			x = x;
	}

#endif

	if (bBlockDebugLog || (!bSendToFile && !IsDebuggerPresent()))
		return;

	if (bSendToDebugger)
	{
		wchar_t szPos[255];
		static SYSTEMTIME st;
		GetLocalTime(&st);
		swprintf_c(szPos, L"%02i:%02i.%03i %s(%s, %i,%i,%i,%i)\n",
		          st.wMinute, st.wSecond, st.wMilliseconds, asFunc,
		          (hw==ghConWnd) ? L"Con" : L"Emu", x,y,w,h);
		DEBUGSTRMOVE(szPos);
	}
	else if (bSendToFile)
	{
		char szPos[255];
		wsprintfA(szPos, "%s(%s, %i,%i,%i,%i)\n",
		          asFunc,
		          (hw==ghConWnd) ? "Con" : "Emu", x,y,w,h);
		DebugLogFile(szPos);
	}
}

void DebugLogBufSize(HANDLE h, COORD sz)
{
	if (bBlockDebugLog || (!bSendToFile && !IsDebuggerPresent()))
		return;

	static char szSize[255];

	if (bSendToDebugger)
	{
		static SYSTEMTIME st;
		GetLocalTime(&st);
		wsprintfA(szSize, "%02i:%02i.%03i ChangeBufferSize(%i,%i)\n",
		          st.wMinute, st.wSecond, st.wMilliseconds,
		          sz.X, sz.Y);
		OutputDebugStringA(szSize);
	}
	else if (bSendToFile)
	{
		wsprintfA(szSize, "ChangeBufferSize(%i,%i)\n",
		          sz.X, sz.Y);
		DebugLogFile(szSize);
	}
}

void DebugLogFile(LPCSTR asMessage)
{
	if (!bSendToFile)
		return;

	HANDLE hLogFile = INVALID_HANDLE_VALUE;

	if (LogFilePath==NULL)
	{
		//WCHAR LogFilePath[MAX_PATH+1];
		LogFilePath = (WCHAR*)calloc(MAX_PATH+1,sizeof(WCHAR));
		GetModuleFileNameW(0,LogFilePath,MAX_PATH);
		WCHAR* pszDot = wcsrchr(LogFilePath, L'.');
		lstrcpyW(pszDot, L".log");
		hLogFile = CreateFileW(LogFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	else
	{
		hLogFile = CreateFileW(LogFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}

	if (hLogFile!=INVALID_HANDLE_VALUE)
	{
		DWORD dwSize=0;
		SetFilePointer(hLogFile, 0, 0, FILE_END);
		SYSTEMTIME st;
		GetLocalTime(&st);
		char szWhole[32];
		wsprintfA(szWhole, "%02i:%02i:%02i.%03i ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		WriteFile(hLogFile, szWhole, lstrlenA(szWhole), &dwSize, NULL);
		WriteFile(hLogFile, asMessage, lstrlenA(asMessage), &dwSize, NULL);
		CloseHandle(hLogFile);
	}
}

#endif
