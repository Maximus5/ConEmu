#include "header.h"

TrayIcon Icon;

TrayIcon::TrayIcon()
{
    memset(&IconData, 0, sizeof(IconData));
    isWindowInTray = NULL;
}

TrayIcon::~TrayIcon()
{
}

void TrayIcon::HideWindowToTray()
{
    GetWindowText(ghWnd, IconData.szTip, 127);
    Shell_NotifyIcon(NIM_ADD, &IconData);
    ShowWindow(ghWnd, SW_HIDE);
    isWindowInTray = true;
}

void TrayIcon::RestoreWindowFromTray()
{
    ShowWindow(ghWnd, SW_SHOW); 
    SetForegroundWindow(ghWnd);
    Shell_NotifyIcon(NIM_DELETE, &IconData);
    EnableMenuItem(GetSystemMenu(ghWnd, false), ID_TOTRAY, MF_BYCOMMAND | MF_ENABLED);
    isWindowInTray = false;
}

void TrayIcon::LoadIcon(HWND inWnd, int inIconResource)
{
    IconData.cbSize = sizeof(NOTIFYICONDATA);
    IconData.uID = 1;
    IconData.hWnd = inWnd;
    IconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    IconData.uCallbackMessage = WM_TRAYNOTIFY;
    //!!!ICON
    IconData.hIcon = hClassIconSm;
    //(HICON)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(inIconResource), IMAGE_ICON, 
    //    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
}

void TrayIcon::Delete()
{
    Shell_NotifyIcon(NIM_DELETE, &IconData);
}
