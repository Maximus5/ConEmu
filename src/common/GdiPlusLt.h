
#pragma once

#define OUT
#define GDIPCONST const

#define    PixelFormatIndexed      0x00010000 // Indexes into a palette
#define    PixelFormatGDI          0x00020000 // Is a GDI-supported format
#define    PixelFormatAlpha        0x00040000 // Has an alpha component
#define    PixelFormatPAlpha       0x00080000 // Pre-multiplied alpha
#define    PixelFormatExtended     0x00100000 // Extended color 16 bits/channel
#define    PixelFormatCanonical    0x00200000 

#define PixelFormat1bppIndexed     (1 | ( 1 << 8) | PixelFormatIndexed | PixelFormatGDI)

namespace Gdiplus
{
enum Status { Ok = 0, Win32Error = 7, };
typedef DWORD ARGB;
typedef DWORD PixelFormat;
struct GdiplusStartupInput
{
    UINT32 GdiplusVersion;             
    void* DebugEventCallback;
    BOOL SuppressBackgroundThread;
    BOOL SuppressExternalCodecs; 
    
    GdiplusStartupInput()
    {
        GdiplusVersion = 1;
        DebugEventCallback = NULL;
        SuppressBackgroundThread = FALSE;
        SuppressExternalCodecs = FALSE;
    }
};

typedef struct _GpBitmap
{
	int Dummy;
} GpBitmap, GpImage;

typedef struct _GpGraphics
{
	int Dummy;
} GpGraphics;

enum DitherType
{
    DitherTypeSolid         = 1,
};

enum PaletteType
{
    PaletteTypeFixedBW          = 2,
};

struct ColorPalette
{

public:

    UINT Flags;             // Palette flags
    UINT Count;             // Number of color entries
    ARGB Entries[1];        // Palette color entries
};

enum PaletteFlags
{
    PaletteFlagsHasAlpha    = 0x0001,
    PaletteFlagsGrayScale   = 0x0002,
    PaletteFlagsHalftone    = 0x0004
};

enum EncoderParameterValueType
{
    EncoderParameterValueTypeLong           = 4,    // 32-bit unsigned int
};

//---------------------------------------------------------------------------
// Encoder Parameter structure
//---------------------------------------------------------------------------
class EncoderParameter
{
public:
    GUID    Guid;               // GUID of the parameter
    ULONG   NumberOfValues;     // Number of the parameter values
    ULONG   Type;               // Value type, like ValueTypeLONG  etc.
    VOID*   Value;              // A pointer to the parameter values
};

//---------------------------------------------------------------------------
// Encoder Parameters structure
//---------------------------------------------------------------------------
class EncoderParameters
{
public:
    UINT Count;                      // Number of parameters in this structure
    EncoderParameter Parameter[1];   // Parameter values
};

//--------------------------------------------------------------------------
// ImageCodecInfo structure
//--------------------------------------------------------------------------

class ImageCodecInfo
{
public:          
    CLSID Clsid;
    GUID  FormatID;
    const WCHAR* CodecName;
    const WCHAR* DllName;
    const WCHAR* FormatDescription;
    const WCHAR* FilenameExtension;
    const WCHAR* MimeType;
    DWORD Flags;
    DWORD Version;
    DWORD SigCount;
    DWORD SigSize;
    const BYTE* SigPattern;
    const BYTE* SigMask;
};

};
