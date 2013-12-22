
#pragma once

BOOL DumpImage(HDC hScreen, HBITMAP hBitmap, int anWidth, int anHeight, LPCWSTR pszFile);
BOOL DumpImage(HDC hScreen, HBITMAP hBitmap, int anX, int anY, int anWidth, int anHeight, LPCWSTR pszFile, bool PreserveTransparency=true);
BOOL DumpImage(BITMAPINFOHEADER* pHdr, LPVOID pBits, LPCWSTR pszFile);
