
/*
Copyright (c) 2009-2012 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifdef _DEBUG
//#define PNGDUMP
#endif

#include <windows.h>
#ifdef PNGDUMP
#include <Gdiplus.h>
#endif
#include "Header.h"
#include "ScreenDump.h"

WARNING("GdiPlus is XP Only");

BOOL LoadScreen(HDC hScreen, int anWidth, int anHeight, LPBYTE* pScreen, DWORD *dwSize)
{
	*pScreen = NULL;
	*dwSize = 0;
	int iBitPerPixel = 16;
	int iHdrSize = sizeof(BITMAPINFOHEADER);
	int iScrWidth = anWidth;
	int iScrHeight= anHeight;
	int iWidth = iScrWidth;
	int iHeight= iScrHeight;
	int iMemSize = iWidth*iHeight*iBitPerPixel/8;
	BITMAPINFO *bi = (BITMAPINFO *) calloc(iHdrSize,1);
	BITMAPINFOHEADER *bih = &(bi->bmiHeader);
	bih->biSize = sizeof(BITMAPINFOHEADER);
	bih->biWidth  = iWidth;
	bih->biHeight = iHeight;
	bih->biPlanes = 1;
	bih->biBitCount = iBitPerPixel;
	bih->biCompression = BI_RGB;
	bih->biXPelsPerMeter = 96;
	bih->biYPelsPerMeter = 96;
	bih->biClrImportant = 0;
	char * pData = (char*)calloc(iMemSize,1);
	//char * pDataEnd = pData + iMemSize;

	if (pData==NULL || bi==NULL)
	{
		if (bi) free(bi);

		if (pData) free(pData);

		return FALSE;
	}

	//char *pDataCur = pData;
	HDC hMem = CreateCompatibleDC(hScreen);
	char* ppvBits=NULL;
	HBITMAP hScreenBmp = CreateDIBSection(hMem, (const BITMAPINFO*) bi, DIB_RGB_COLORS, (LPVOID*)&ppvBits, NULL, 0);
	HBITMAP hOldBmp = (HBITMAP) SelectObject(hMem, hScreenBmp);
	BOOL bRc = FALSE;
	bRc = BitBlt(hMem, 0,0,iWidth,iHeight, hScreen, 0,0, SRCCOPY);
	BITMAPFILEHEADER bfh;
	bfh.bfType = 0x4D42; // 'MB';
	bfh.bfSize = sizeof(BITMAPFILEHEADER)+iHdrSize+iMemSize;
	bfh.bfReserved1=0;
	bfh.bfReserved2=0;
	bfh.bfOffBits=sizeof(BITMAPFILEHEADER)+iHdrSize;
	*dwSize = sizeof(bfh)+iHdrSize+iMemSize;
	*pScreen = (LPBYTE)LocalAlloc(LPTR, *dwSize);
	BOOL lbRc = FALSE;

	if (*pScreen)
	{
		LPBYTE pPtr = *pScreen;
		memcpy(pPtr,&bfh,sizeof(bfh));
		pPtr += sizeof(bfh);
		memcpy(pPtr,bi,iHdrSize);
		pPtr += iHdrSize;
		memcpy(pPtr,ppvBits,iMemSize);
		pPtr += iMemSize;
		lbRc = TRUE;
	}
	else
	{
		*dwSize = 0;
	}

	SelectObject(hMem, hOldBmp);
	DeleteDC(hMem);
	DeleteObject(hScreenBmp);
	free(pData);
	free(bi);
	return lbRc;
}

#ifdef PNGDUMP
int GetCodecClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes
	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;
	Gdiplus::GetImageEncodersSize(&num, &size);

	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));

	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

	for(int j = 0; j < (int)num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			return j;  // Success
		}
	} // for

	return -1;  // Failure
} // GetCodecClsid


BOOL gbMStreamCoInitialized=FALSE;
class MStream : public IStream
{
	private:
		LONG mn_RefCount; BOOL mb_SelfAlloc; char* mp_Data; DWORD mn_DataSize; DWORD mn_DataLen; DWORD mn_DataPos;
	public:
		MStream(LPCVOID apData, DWORD anLen)
		{
			mn_RefCount = 1; mb_SelfAlloc=FALSE; mp_Data = (char*)apData; mn_DataSize = anLen;
			mn_DataPos = 0; mn_DataLen = anLen;

			if (!gbMStreamCoInitialized)
			{
				HRESULT hr = CoInitialize(NULL);
				gbMStreamCoInitialized = TRUE;
			}
		};
		MStream()
		{
			mn_RefCount = 1;
			mn_DataSize = 4096*1024; mn_DataPos = 0; mn_DataLen = 0;
			mp_Data = (char*)calloc(mn_DataSize,1);

			if (mp_Data==NULL)
			{
				mn_DataSize = 0;
			}

			mb_SelfAlloc = TRUE;

			if (!gbMStreamCoInitialized)
			{
				HRESULT hr = CoInitialize(NULL);
				gbMStreamCoInitialized = TRUE;
			}
		};
		HRESULT SaveAsData(
		    void** rpData,
		    long*  rnDataSize
		)
		{
			if (mp_Data==NULL)
			{
				return E_POINTER;
			}

			if (mn_DataLen>0)
			{
				*rpData = LocalAlloc(LPTR, mn_DataLen);
				memcpy(*rpData, mp_Data, mn_DataLen);
			}

			*rnDataSize = mn_DataLen;
			return S_OK;
		};
	private:
		~MStream()
		{
			if (mp_Data!=NULL)
			{
				if (mb_SelfAlloc) free(mp_Data);

				mp_Data=NULL;
				mn_DataSize = 0; mn_DataPos = 0; mn_DataLen = 0;
			}
		};
	public:
		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject)
		{
			if (riid==IID_IStream || riid==IID_ISequentialStream || riid==IID_IUnknown)
			{
				*ppvObject = this;
				return S_OK;
			}

			return E_NOINTERFACE;
		}

		virtual ULONG STDMETHODCALLTYPE AddRef(void)
		{
			if (this==NULL)
				return 0;

			return (++mn_RefCount);
		}

		virtual ULONG STDMETHODCALLTYPE Release(void)
		{
			if (this==NULL)
				return 0;

			mn_RefCount--;
			int nCount = mn_RefCount;

			if (nCount<=0)
			{
				delete this;
			}

			return nCount;
		}
	public:
		/* ISequentialStream */
		virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read(
		    /* [length_is][size_is][out] */ void *pv,
		    /* [in] */ ULONG cb,
		    /* [out] */ ULONG *pcbRead)
		{
			DWORD dwRead=0;

			if (mp_Data)
			{
				dwRead = min(cb, max(0,(mn_DataLen-mn_DataPos)));

				if (dwRead>0)
				{
					memmove(pv, mp_Data+mn_DataPos, dwRead);
					mn_DataPos += dwRead;
				}
			}
			else
			{
				return S_FALSE;
			}

			if (pcbRead) *pcbRead=dwRead;

			return S_OK;
		};

		virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write(
		    /* [size_is][in] */ const void *pv,
		    /* [in] */ ULONG cb,
		    /* [out] */ ULONG *pcbWritten)
		{
			DWORD dwWritten=0;

			if (!mp_Data)
			{
				ULARGE_INTEGER lNewSize; lNewSize.QuadPart = cb;
				HRESULT hr;

				if (FAILED(hr = SetSize(lNewSize)))
					return hr;
			}

			if (mp_Data)
			{
				if ((mn_DataPos+cb)>mn_DataSize)
				{
					// Нужно увеличить буфер, но сохранить текущий размер
					DWORD lLastLen = mn_DataLen;
					ULARGE_INTEGER lNewSize; lNewSize.QuadPart = mn_DataSize + max((cb+1024), 256*1024);

					if (lNewSize.HighPart!=0)
						return S_FALSE;

					if (FAILED(SetSize(lNewSize)))
						return S_FALSE;

					mn_DataLen = lLastLen; // Вернули текущий размер
				}

				// Теперь можно писать в буфер
				memmove(mp_Data+mn_DataPos, pv, cb);
				dwWritten = cb;
				mn_DataPos += cb;

				if (mn_DataPos>mn_DataLen) mn_DataLen=mn_DataPos;  //2008-03-18
			}
			else
			{
				return S_FALSE;
			}

			if (pcbWritten) *pcbWritten=dwWritten;

			return S_OK;
		};
	public:
		/* IStream */
		virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek(
		    /* [in] */ LARGE_INTEGER dlibMove,
		    /* [in] */ DWORD dwOrigin,
		    /* [out] */ ULARGE_INTEGER *plibNewPosition)
		{
			if (dwOrigin!=STREAM_SEEK_SET && dwOrigin!=STREAM_SEEK_CUR && dwOrigin!=STREAM_SEEK_END)
				return STG_E_INVALIDFUNCTION;

			if (mp_Data)
			{
				_ASSERTE(mn_DataSize);
				LARGE_INTEGER lNew;

				if (dwOrigin==STREAM_SEEK_SET)
				{
					lNew.QuadPart = dlibMove.QuadPart;
				}
				else if (dwOrigin==STREAM_SEEK_CUR)
				{
					lNew.QuadPart = mn_DataPos + dlibMove.QuadPart;
				}
				else if (dwOrigin==STREAM_SEEK_END)
				{
					lNew.QuadPart = mn_DataLen + dlibMove.QuadPart;
				}

				if (lNew.QuadPart<0)
					return S_FALSE;

				if (lNew.QuadPart>mn_DataSize)
					return S_FALSE;

				mn_DataPos = lNew.LowPart;

				if (plibNewPosition)
					plibNewPosition->QuadPart = mn_DataPos;
			}
			else
			{
				return S_FALSE;
			}

			return S_OK;
		};

		virtual HRESULT STDMETHODCALLTYPE SetSize(
		    /* [in] */ ULARGE_INTEGER libNewSize)
		{
			HRESULT hr = STG_E_INVALIDFUNCTION;
			ULARGE_INTEGER llTest; llTest.QuadPart = 0;
			LARGE_INTEGER llShift; llShift.QuadPart = 0;

			if (libNewSize.HighPart)
				return E_OUTOFMEMORY;

			if (mp_Data)
			{
				if (libNewSize.LowPart>mn_DataSize)
				{
					char* pNew = (char*)realloc(mp_Data, libNewSize.LowPart);

					if (pNew==NULL)
						return E_OUTOFMEMORY;

					mp_Data = pNew;
				}

				mn_DataLen = libNewSize.LowPart;

				if (mn_DataPos>mn_DataLen)  // Если размер уменьшили - проверить позицию
					mn_DataPos = mn_DataLen;

				hr = S_OK;
			}
			else
			{
				mp_Data = (char*)calloc(libNewSize.LowPart,1);

				if (mp_Data==NULL)
					return E_OUTOFMEMORY;

				mn_DataSize = libNewSize.LowPart;
				mn_DataLen = libNewSize.LowPart;
				mn_DataPos = 0;
				hr = S_OK;
			}

			return hr;
		};

		virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo(
		    /* [unique][in] */ IStream *pstm,
		    /* [in] */ ULARGE_INTEGER cb,
		    /* [out] */ ULARGE_INTEGER *pcbRead,
		    /* [out] */ ULARGE_INTEGER *pcbWritten)
		{
			return STG_E_INVALIDFUNCTION;
		};

		virtual HRESULT STDMETHODCALLTYPE Commit(
		    /* [in] */ DWORD grfCommitFlags)
		{
			if (mp_Data)
			{
				//
			}
			else
			{
				return S_FALSE;
			}

			return S_OK;
		};

		virtual HRESULT STDMETHODCALLTYPE Revert(void)
		{
			return STG_E_INVALIDFUNCTION;
		};

		virtual HRESULT STDMETHODCALLTYPE LockRegion(
		    /* [in] */ ULARGE_INTEGER libOffset,
		    /* [in] */ ULARGE_INTEGER cb,
		    /* [in] */ DWORD dwLockType)
		{
			return STG_E_INVALIDFUNCTION;
		};

		virtual HRESULT STDMETHODCALLTYPE UnlockRegion(
		    /* [in] */ ULARGE_INTEGER libOffset,
		    /* [in] */ ULARGE_INTEGER cb,
		    /* [in] */ DWORD dwLockType)
		{
			return STG_E_INVALIDFUNCTION;
		};

		virtual HRESULT STDMETHODCALLTYPE Stat(
		    /* [out] */ STATSTG *pstatstg,
		    /* [in] */ DWORD grfStatFlag)
		{
			return STG_E_INVALIDFUNCTION;
		};

		virtual HRESULT STDMETHODCALLTYPE Clone(
		    /* [out] */ IStream **ppstm)
		{
			return STG_E_INVALIDFUNCTION;
		};
};
#endif



BOOL DumpImage(HDC hScreen, int anWidth, int anHeight, LPCWSTR pszFile)
{
	BOOL lbRc = FALSE;
	LPBYTE pScreen = NULL;
	DWORD cbOut = 0;

	if (!LoadScreen(hScreen, anWidth, anHeight, &pScreen, &cbOut))
		return FALSE;

#ifdef PNGDUMP
	Gdiplus::GdiplusStartupInput gdiplusStartupInput; long quality;  Gdiplus::Status stat;
	ULONG_PTR gdiplusToken = 0; CLSID codecClsid; Gdiplus::EncoderParameters encoderParameters;
	// Initialize GDI+
	stat = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	if (stat==Gdiplus::Ok)
	{
		MStream *pStream = new MStream(pScreen, cbOut);

		if (pStream)
		{
			Gdiplus::Image image((IStream*)pStream, FALSE);
			pStream->Release(); pStream=NULL; GetCodecClsid(L"image/png", &codecClsid);
			encoderParameters.Count = 1; encoderParameters.Parameter[0].Guid = Gdiplus::EncoderQuality;
			encoderParameters.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
			encoderParameters.Parameter[0].NumberOfValues = 1; quality = 75;
			encoderParameters.Parameter[0].Value = &quality;
			pStream = new MStream();
			stat = image.Save((IStream*)pStream, &codecClsid, &encoderParameters);
			LocalFree(pScreen); pScreen = NULL; cbOut = 0;
			pStream->SaveAsData((void**)&pScreen, (long*)&cbOut);
			pStream->Release(); pStream=NULL;
		}

		Gdiplus::GdiplusShutdown(gdiplusToken);
	}

#endif

	if (pScreen && cbOut)
	{
		wchar_t szFile[MAX_PATH+1];
		lstrcpynW(szFile, pszFile, MAX_PATH);
		wchar_t* pszDot = wcsrchr(szFile, L'.');

		if (pszDot)
		{
#ifdef PNGDUMP
			lstrcpyW(pszDot, L".png");
#else
			lstrcpyW(pszDot, L".bmp");
#endif
			HANDLE hFile = CreateFile(szFile, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

			if (hFile != INVALID_HANDLE_VALUE)
			{
				DWORD dwWritten = 0;
				lbRc = WriteFile(hFile, pScreen, cbOut, &dwWritten, 0);
				CloseHandle(hFile);
			}
		}

		LocalFree(pScreen);
	}

	return S_OK;
}
