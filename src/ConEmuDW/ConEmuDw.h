
#pragma once

BOOL WINAPI GetTextAttributes(FarColor* Attributes);
BOOL WINAPI SetTextAttributes(const FarColor* Attributes);
BOOL WINAPI ReadOutput(FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT* ReadRegion);
BOOL WINAPI WriteOutput(const FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT* WriteRegion);
void WINAPI CommitOutput();
int  WINAPI GetColorDialog(FarColor* Color, BOOL Centered, BOOL AddTransparent);
