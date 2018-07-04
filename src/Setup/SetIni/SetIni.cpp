
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "resource.h"
#include "../Setupper/VersionI.h"
#include "../Setupper/NextArg.h"

#define countof(a) (sizeof((a))/(sizeof(*(a))))
#define _ASSERTE(x)

HINSTANCE hInst;
//OSVERSIONINFO gOSVer = {sizeof(OSVERSIONINFO)};

static bool CalcCRC(const BYTE *pData, size_t cchSize, DWORD& crc)
{
	if (!pData)
	{
		_ASSERTE(pData==NULL || cchSize==0);
		return false;
	}

	static DWORD CRCtable[] =
	{
		0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 
		0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 
		0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 
		0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 
		0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 
		0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 
		0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 
		0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 
		0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 
		0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 
		0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106, 
		0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 
		0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 
		0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 
		0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 
		0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 
		0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 
		0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 
		0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 
		0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 
		0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 
		0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 
		0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 
		0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 
		0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 
		0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 
		0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E, 
		0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, 
		0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 
		0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 
		0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 
		0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 
		0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 
		0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 
		0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242, 
		0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 
		0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 
		0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 
		0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 
		0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9, 
		0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 
		0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 
		0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
	};
	
	//crc = 0xFFFFFFFF;

	for (const BYTE* p = pData; cchSize; cchSize--)
	{
		crc = ( crc >> 8 ) ^ CRCtable[(unsigned char) ((unsigned char) crc ^ *p++ )];
	}

	//crc ^= 0xFFFFFFFF;

	return true;
}

/*
 * Simple MD5 implementation
 *
 * Compile with: gcc -o md5 md5.c
 */
 
// Constants are the integer part of the sines of integers (in radians) * 2^32.
const uint32_t k[64] = {
0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee ,
0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501 ,
0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be ,
0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821 ,
0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa ,
0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8 ,
0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed ,
0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a ,
0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c ,
0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70 ,
0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05 ,
0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665 ,
0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039 ,
0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1 ,
0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1 ,
0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };
 
// r specifies the per-round shift amounts
const uint32_t r[] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                      5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
                      4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                      6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};
 
// leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))
 
void to_bytes(uint32_t val, uint8_t *bytes)
{
    bytes[0] = (uint8_t) val;
    bytes[1] = (uint8_t) (val >> 8);
    bytes[2] = (uint8_t) (val >> 16);
    bytes[3] = (uint8_t) (val >> 24);
}
 
uint32_t to_int32(const uint8_t *bytes)
{
    return (uint32_t) bytes[0]
        | ((uint32_t) bytes[1] << 8)
        | ((uint32_t) bytes[2] << 16)
        | ((uint32_t) bytes[3] << 24);
}
 
void md5(const uint8_t *initial_msg, size_t initial_len, uint8_t *digest)
{
 
    // These vars will contain the hash
    uint32_t h0, h1, h2, h3;
 
    // Message (to prepare)
    uint8_t *msg = NULL;
 
    size_t new_len, offset;
    uint32_t w[16];
    uint32_t a, b, c, d, i, f, g, temp;
 
    // Initialize variables - simple count in nibbles:
    h0 = 0x67452301;
    h1 = 0xefcdab89;
    h2 = 0x98badcfe;
    h3 = 0x10325476;
 
    //Pre-processing:
    //append "1" bit to message    
    //append "0" bits until message length in bits ? 448 (mod 512)
    //append length mod (2^64) to message
 
    for (new_len = initial_len + 1; new_len % (512/8) != 448/8; new_len++)
        ;
 
    msg = (uint8_t*)malloc(new_len + 8);
    memcpy(msg, initial_msg, initial_len);
    msg[initial_len] = 0x80; // append the "1" bit; most significant bit is "first"
    for (offset = initial_len + 1; offset < new_len; offset++)
        msg[offset] = 0; // append "0" bits
 
    // append the len in bits at the end of the buffer.
    to_bytes(initial_len*8, msg + new_len);
    // initial_len>>29 == initial_len*8>>32, but avoids overflow.
    to_bytes(initial_len>>29, msg + new_len + 4);
 
    // Process the message in successive 512-bit chunks:
    //for each 512-bit chunk of message:
    for(offset=0; offset<new_len; offset += (512/8)) {
 
        // break chunk into sixteen 32-bit words w[j], 0 ? j ? 15
        for (i = 0; i < 16; i++)
            w[i] = to_int32(msg + offset + i*4);
 
        // Initialize hash value for this chunk:
        a = h0;
        b = h1;
        c = h2;
        d = h3;
 
        // Main loop:
        for(i = 0; i<64; i++) {
 
            if (i < 16) {
                f = (b & c) | ((~b) & d);
                g = i;
            } else if (i < 32) {
                f = (d & b) | ((~d) & c);
                g = (5*i + 1) % 16;
            } else if (i < 48) {
                f = b ^ c ^ d;
                g = (3*i + 5) % 16;          
            } else {
                f = c ^ (b | (~d));
                g = (7*i) % 16;
            }
 
            temp = d;
            d = c;
            c = b;
            b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
            a = temp;
 
        }
 
        // Add this chunk's hash to result so far:
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
 
    }
 
    // cleanup
    free(msg);
 
    //var char digest[16] := h0 append h1 append h2 append h3 //(Output is in little-endian)
    to_bytes(h0, digest);
    to_bytes(h1, digest + 4);
    to_bytes(h2, digest + 8);
    to_bytes(h3, digest + 12);
}


void Write(LPCWSTR asText)
{
	DWORD dwWritten, dwLen = lstrlenW(asText);
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), asText, dwLen, &dwWritten, NULL);
}

int ReportError(int nErr, LPCWSTR asLabel, const void* pArg)
{
	wchar_t szMessage[2048];
	DWORD nLastErr = GetLastError();
	wsprintf(szMessage, asLabel, pArg);
	if (nLastErr != 0)
	{
		wsprintfW(szMessage+lstrlen(szMessage), HIWORD(nLastErr) ? L"  ErrorCode=0x%08X\n" : L"  ErrorCode=%u\n", nLastErr);
	}
	Write(szMessage);
	
	return nErr;
}

#ifdef __GNUC__
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#endif
{
	hInst = hInstance;
	
	Write(L"SetIni (c) Maximus5\n");
	
	//DWORD dwErr;
	bool bRc = false;
	wchar_t szArg[MAX_PATH+1], szIni[MAX_PATH+1], szSection[MAX_PATH+1], szKey[MAX_PATH+1], szValue[MAX_PATH+1], szUrl[MAX_PATH+1], szFull[MAX_PATH*2], szInfo[MAX_PATH*4];
	LPCWSTR pszCmdToken = GetCommandLine();
	//LPCWSTR pszCmdLineW = pszCmdToken;
	
	while ((pszCmdToken = NextArg(pszCmdToken, szArg)))
	{
		if (lstrcmpi(szArg, L"set") == 0)
		{
			if (
				NextArg(&pszCmdToken, szIni) || !*szIni
				|| NextArg(&pszCmdToken, szSection) || !*szSection
				|| NextArg(&pszCmdToken, szKey) || !*szKey
				|| NextArg(&pszCmdToken, szValue) || !*szValue
				)
			{
				return ReportError(31, L"Invalid command line\nUsage: SetIni set <ini-file> <section> <key-name> <value>\n", NULL);
			}
			wsprintfW(szInfo, L"[%s]\n%s=%s\n", szSection, szKey, szValue[0]==L'.' ? (szValue+1) : szValue);
			Write(szInfo);
			if (!WritePrivateProfileStringW(szSection, szKey, szValue[0]==L'.' ? (szValue+1) : szValue, szIni))
			{
				return ReportError(21, L"WritePrivateProfileString failed, Key=%s\n", szKey);
			}
			bRc = true;
		}
		else if (lstrcmpi(szArg, L"crc") == 0 || lstrcmpi(szArg, L"md5") == 0)
		{
			bool bMD5 = (lstrcmpi(szArg, L"md5") == 0);

			if (
				NextArg(&pszCmdToken, szIni) || !*szIni
				|| NextArg(&pszCmdToken, szSection) || !*szSection
				|| NextArg(&pszCmdToken, szKey) || !*szKey
				|| NextArg(&pszCmdToken, szValue) || !*szValue
				|| (!bMD5 && (NextArg(&pszCmdToken, szUrl) || !*szUrl))
				)
			{
				return ReportError(31, L"Invalid command line\nUsage: SetIni set <ini-file> <section> <key-name> <value>\n", NULL);
			}
			HANDLE hFile = CreateFileW(szValue, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			if (!hFile || hFile == INVALID_HANDLE_VALUE)
			{
				return ReportError(22, L"Can't open source file:\n  %s\n", szValue);
			}
			LARGE_INTEGER liSize;
			if (!GetFileSizeEx(hFile, &liSize))
			{
				CloseHandle(hFile);
				return ReportError(22, L"GetFileSizeEx failed\n  %s\n", szValue);
			}
			if (!liSize.LowPart)
			{
				CloseHandle(hFile);
				return ReportError(22, L"Source file is empty\n  %s\n", szValue);
			}
			LPBYTE ptrData = (LPBYTE)malloc(liSize.LowPart);
			if (!ReadFile(hFile, ptrData, liSize.LowPart, (DWORD*)&liSize.HighPart, NULL) || liSize.LowPart!=(DWORD)liSize.HighPart)
			{
				CloseHandle(hFile); free(ptrData);
				return ReportError(22, L"Reading source file failed\n  %s\n", szValue);
			}

			if (!bMD5)
			{
				DWORD crc = 0xFFFFFFFF;
				CalcCRC(ptrData, liSize.LowPart, crc);
				crc ^= 0xFFFFFFFF;
				free(ptrData);
				CloseHandle(hFile);
				wsprintfW(szFull, L"%u,x%08X,%s", liSize.LowPart, crc, szUrl);
			}
			else
			{
				uint8_t result[16] = {0};
				md5((const uint8_t*)ptrData, liSize.LowPart, result);
				for (int i = 0; i < 16; i++)
					wsprintfW(szFull+i*2, L"%02x", (UINT)result[i]);
			}

			wsprintfW(szInfo, L"[%s]\n%s=%s\n", szSection, szKey, szFull);
			Write(szInfo);
			if (!WritePrivateProfileStringW(szSection, szKey, szFull, szIni))
			{
				return ReportError(21, L"WritePrivateProfileString failed, Key=%s\n", szKey);
			}
			bRc = true;
		}
	}
	
	if (!bRc)
	{
		return ReportError(21, 
			L"Invalid command line\n"
			L"Usage: SetIni set <ini-file> <section> <key-name> <value>\n"
			L"   or: SetIni crc <ini-file> <section> <key-name> <src-file> <url>\n"
			L"   or: SetIni md5 <ini-file> <section> <key-name> <src-file> <url>\n"
			, NULL);
	}
	
	WritePrivateProfileStringW(NULL, NULL, NULL, NULL);
	
	return 0;
}
