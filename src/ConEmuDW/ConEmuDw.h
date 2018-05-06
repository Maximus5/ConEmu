
#pragma once

#ifdef DEFINE_CONSOLE_EXPORTS
	// for ConEmuDW.cpp

	struct FarColor;
	struct FAR_CHAR_INFO;

	typedef int (WINAPI* GetColorDialog_t)(FarColor* Color, BOOL Centered, BOOL AddTransparent);
	typedef BOOL (WINAPI* GetTextAttributes_t)(FarColor* Attributes);
	typedef BOOL (WINAPI* SetTextAttributes_t)(const FarColor* Attributes);
	typedef BOOL (WINAPI* ClearExtraRegions_t)(const FarColor* Attributes, int Mode);
	typedef BOOL (WINAPI* ReadOutput_t)(FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT* ReadRegion);
	typedef BOOL (WINAPI* WriteOutput_t)(const FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT* WriteRegion);
	typedef void (WINAPI* Commit_t)();

	typedef bool (WINAPI* SetConsoleCallbacks_t)(HMODULE hCallbackModule, DWORD nCallbackCount, HookItemCallback_t *ppfnCallbacks);

#else

#if defined(__GNUC__)
extern "C" {
#endif
	BOOL WINAPI GetTextAttributes(FarColor* Attributes);
	BOOL WINAPI SetTextAttributes(const FarColor* Attributes);
	BOOL WINAPI ClearExtraRegions(const FarColor* Color, int Mode);
	BOOL WINAPI ClearExtraRegionsOld(const FarColor* Color);
	BOOL WINAPI ReadOutput(FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT* ReadRegion);
	BOOL WINAPI WriteOutput(const FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT* WriteRegion);
	BOOL WINAPI Commit();
	int  WINAPI GetColorDialog(FarColor* Color, BOOL Centered, BOOL AddTransparent);
#if defined(__GNUC__)
};
#endif

#endif