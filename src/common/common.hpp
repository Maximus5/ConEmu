#include <windows.h>

//#define CONEMUPIPE      L"\\\\.\\pipe\\ConEmuPipe%u"
//#define CONEMUEVENTIN   L"ConEmuInEvent%u"
//#define CONEMUEVENTOUT  L"ConEmuOutEvent%u"
//#define CONEMUEVENTPIPE L"ConEmuPipeEvent%u"

#define CONEMUMAPPING    L"ConEmuPluginData%u"
#define CONEMUDRAGFROM   L"ConEmuDragFrom%u"
#define CONEMUDRAGTO     L"ConEmuDragTo%u"
#define CONEMUEXIT       L"ConEmuExit%u"
#define CONEMUALIVE      L"ConEmuAlive%u"
#define CONEMUREADY      L"ConEmuReady%u"
#define CMD_DRAGFROM     0
#define CMD_DRAGTO       1
#define CMD_EXIT         2
#define MAXCMDCOUNT      3

#define CONEMUTABMAX 0x400
struct ConEmuTab
{
	int  Pos;
	int  Current;
	int  Type;
	int  Modified;
	wchar_t Name[CONEMUTABMAX];
//	int  Modified;
//	int isEditor;
};

struct ForwardedPanelInfo
{
	RECT ActiveRect;
	RECT PassiveRect;
	int ActivePathShift; // сдвиг в этой структуре в байтах
	int PassivePathShift; // сдвиг в этой структуре в байтах
	WCHAR* pszActivePath/*[MAX_PATH+1]*/;
	WCHAR* pszPassivePath/*[MAX_PATH+1]*/;
};

struct FarVersion {
	union {
		DWORD dwVer;
		struct {
			WORD dwVerMinor;
			WORD dwVerMajor;
		};
	};
	DWORD dwBuild;
};

struct ForwardedFileInfo
{
	WCHAR Path[MAX_PATH+1];
};

/*enum PipeCmd
{
	SetTabs=0,
	DragFrom,
	DragTo
};*/

// ConEmu.dll экспортирует следующие функции
//HWND WINAPI GetFarHWND();
//void WINAPI _export GetFarVersion ( FarVersion* pfv );
