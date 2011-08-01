
#include <windows.h>
#include "MAssert.h"
#include "MStrSafe.h"

LPCWSTR msprintf(LPWSTR lpOut, size_t cchOutMax, LPCWSTR lpFmt, ...)
{
	va_list argptr;
	va_start(argptr, lpFmt);
	
	const wchar_t* pszSrc = lpFmt;
	wchar_t* pszDst = lpOut;
	wchar_t  szValue[16];
	wchar_t* pszValue;

	while (*pszSrc)
	{
		if (*pszSrc == L'%')
		{
			pszSrc++;
			switch (*pszSrc)
			{
			case L'%':
				{
					*(pszDst++) = L'%';
					pszSrc++;
					continue;
				}
			case L'c':
				{
					// GCC: 'wchar_t' is promoted to 'int' when passed through '...'
					wchar_t c = va_arg( argptr, int );
					*(pszDst++) = c;
					pszSrc++;
					continue;
				}
			case L's':
				{
					pszValue = va_arg( argptr, wchar_t* );
					if (pszValue)
					{
						while (*pszValue)
						{
							*(pszDst++) = *(pszValue++);
						}
					}
					pszSrc++;
					continue;
				}
			case L'S':
				{
					char* pszValueA = va_arg( argptr, char* );
					if (pszValueA)
					{
						// по хорошему, тут бы MultiByteToWideChar звать, но
						// эта ветка должна по идее только для отладки использоваться
						while (*pszValueA)
						{
							*(pszDst++) = (wchar_t)*(pszValueA++);
						}
					}
					pszSrc++;
					continue;
				}
			case L'u':
			case L'i':
				{
					UINT nValue = 0;
					if (*pszSrc == L'i')
					{
						int n = va_arg( argptr, int );
						if (n < 0)
						{
							*(pszDst++) = L'-';
							nValue = -n;
						}
						else
						{
							nValue = n;
						}
					}
					else
					{
						nValue = va_arg( argptr, UINT );
					}
					pszSrc++;
					pszValue = szValue;
					while (nValue)
					{
						WORD n = (WORD)(nValue % 10);
						*(pszValue++) = (wchar_t)(L'0' + n);
						nValue = (nValue - n) / 10;
					}
					if (pszValue == szValue)
						*(pszValue++) = L'0';
					// Теперь перекинуть в szGuiPipeName
					while (pszValue > szValue)
					{
						*(pszDst++) = *(--pszValue);
					}
					continue;
				}
			case L'0':
			case L'X':
			case L'x':
				{
					int nLen = 0;
					DWORD nValue;
					wchar_t cBase = L'A';
					if (pszSrc[0] == L'0' && pszSrc[1] == L'8' && (pszSrc[2] == L'X' || pszSrc[2] == L'x'))
					{
						if (pszSrc[2] == L'x')
							cBase = L'a';
						memmove(szValue, L"00000000", 16);
						nLen = 8;
						pszSrc += 3;
					}
					else if (pszSrc[0] == L'X' || pszSrc[0] == L'x')
					{
						if (pszSrc[0] == L'x')
							cBase = L'a';
						pszSrc++;
					}
					else
					{
						_ASSERTE(*pszSrc == L'u' || *pszSrc == L's' || *pszSrc == L'c' || *pszSrc == L'i' || *pszSrc == L'X');
						goto wrap;
					}
					
					
					nValue = va_arg( argptr, UINT );
					pszValue = szValue;
					while (nValue)
					{
						WORD n = (WORD)(nValue & 0xF);
						if (n <= 9)
							*(pszValue++) = (wchar_t)(L'0' + n);
						else
							*(pszValue++) = (wchar_t)(cBase + n - 10);
						nValue = nValue >> 4;
					}
					if (!nLen)
					{
						nLen = (int)(pszValue - szValue);
						if (!nLen)
						{
							*pszValue = L'0';
							nLen = 1;
						}
					}
					else
					{
						pszValue = (szValue+nLen);
					}
					// Теперь перекинуть в szGuiPipeName
					while (pszValue > szValue)
					{
						*(pszDst++) = *(--pszValue);
					}
					continue;
				}
			default:
				_ASSERTE(*pszSrc == L'u' || *pszSrc == L's' || *pszSrc == L'c' || *pszSrc == L'i' || *pszSrc == L'X');
			}
		}
		else
		{
			*(pszDst++) = *(pszSrc++);
		}
	}
wrap:
	*pszDst = 0;
	_ASSERTE(lstrlen(lpOut) < (int)cchOutMax);
	return lpOut;
}
