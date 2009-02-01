#define ArraySize(a)  (sizeof(a)/sizeof(a[0]))

void UpdateConEmuTabs(int event, bool losingFocus, bool editorSave);

typedef BOOL (WINAPI *TReadConsoleInput)(HANDLE hConsole,INPUT_RECORD *ir,DWORD nNumber,LPDWORD nNumberOfRead);
typedef BOOL (WINAPI *TPeekConsoleInput)(HANDLE hConsole,INPUT_RECORD *ir,DWORD nNumber,LPDWORD nNumberOfRead);
TReadConsoleInput p_fnReadConsoleInputOrgA;
TReadConsoleInput p_fnReadConsoleInputOrgW;
TPeekConsoleInput p_fnPeekConsoleInputOrgA;
TPeekConsoleInput p_fnPeekConsoleInputOrgW;


// non standard Far API parts
struct InitDialogItem
{
	int Type;
	int X1;
	int Y1;
	int X2;
	int Y2;
	int Focus;
	DWORD_PTR Selected;
	unsigned int Flags;
	int DefaultButton;
	char *Data;
};
void InitDialogItems(const struct InitDialogItem *Init, struct FarDialogItem *Item, int ItemsNumber);
