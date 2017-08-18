/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Image utilities implementation.


#include "stdafx.h"
#include "ImageUtil.h"
#include "ImageGif.h"
#include "ImageTIF.h"
#include "ImageHDR.h"
#include "Image_DXTC.h"
#include "ImageASC.h"
#include "ImageBT.h"
#include "CryFile.h"
#include "GdiUtil.h"

#if !defined(AZ_PLATFORM_WINDOWS)
typedef struct tagBITMAPFILEHEADER {
    WORD  bfType;
    DWORD bfSize;
    WORD  bfReserved1;
    WORD  bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;
typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG  biWidth;
    LONG  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG  biXPelsPerMeter;
    LONG  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;
#endif

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::SaveBitmap(const QString& szFileName, CImageEx& inImage, bool inverseY)
{
    ////////////////////////////////////////////////////////////////////////
    // Simple DIB save code
    ////////////////////////////////////////////////////////////////////////

    unsigned int i;
    DWORD* pLine1 = NULL;
    DWORD* pLine2 = NULL;
    DWORD* pTemp = NULL;
    BITMAPFILEHEADER bitmapfileheader;
    BITMAPINFOHEADER bitmapinfoheader;

    CLogFile::FormatLine("Saving data to bitmap... %s", szFileName.toLatin1().data());

    int dwWidth = inImage.GetWidth();
    int dwHeight = inImage.GetHeight();
    DWORD* pData = (DWORD*)inImage.GetData();

    int iAddToPitch = (dwWidth * 3) % 4;
    if (iAddToPitch)
    {
        iAddToPitch = 4 - iAddToPitch;
    }

    uint8* pImage = new uint8[(dwWidth * 3 + iAddToPitch) * dwHeight];

    i = 0;
    for (int y = 0; y < dwHeight; y++)
    {
        int src_y = y;

        if (inverseY)
        {
            src_y = (dwHeight - 1) - y;
        }

        for (int x = 0; x < dwWidth; x++)
        {
            DWORD c = pData[x + src_y * dwWidth];
            pImage[i] = GetBValue(c);
            pImage[i + 1] = GetGValue(c);
            pImage[i + 2] = GetRValue(c);
            i += 3;
        }

        for (int j = 0; j < iAddToPitch; j++)
        {
            pImage[i++] = 0;
        }
    }

    // Fill in bitmap structures
    bitmapfileheader.bfType = 0x4D42;
    bitmapfileheader.bfSize = (dwWidth * dwHeight * 3) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bitmapfileheader.bfReserved1 = 0;
    bitmapfileheader.bfReserved2 = 0;
    bitmapfileheader.bfOffBits = sizeof(BITMAPFILEHEADER) +
        sizeof(BITMAPINFOHEADER) + (0/* * sizeof(RGBQUAD)*/);
    bitmapinfoheader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapinfoheader.biWidth = dwWidth;
    bitmapinfoheader.biHeight = dwHeight;
    bitmapinfoheader.biPlanes = 1;
    bitmapinfoheader.biBitCount = (WORD) 24;
    bitmapinfoheader.biCompression = 0;
    bitmapinfoheader.biSizeImage = 0;
    bitmapinfoheader.biXPelsPerMeter = 0;
    bitmapinfoheader.biYPelsPerMeter = 0;
    bitmapinfoheader.biClrUsed = 0;
    bitmapinfoheader.biClrImportant = 0;

    // Write bitmap to disk
    QFile file(szFileName);
    if (!file.open(QFile::WriteOnly))
    {
        delete []pImage;
        return false;
    }

    // Write the headers to the file
    file.write((char*)&bitmapfileheader, sizeof(BITMAPFILEHEADER));
    file.write((char*)&bitmapinfoheader, sizeof(BITMAPINFOHEADER));

    // Write the data
    file.write((char*)pImage, ((dwWidth * 3 + iAddToPitch) * dwHeight));

    delete []pImage;

    // Success
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::Save(const QString& strFileName, CImageEx& inImage)
{
    QImage imgBitmap;

    CreateBitmapFromImage(inImage, imgBitmap);

    return imgBitmap.save(strFileName);
}

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::SaveJPEG(const QString& strFileName, CImageEx& inImage)
{
    return Save(strFileName, inImage);
}

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::LoadImageWithGDIPlus(const QString& fileName, CImageEx& image)
{
    QImage imgBitmap(fileName);

    if (imgBitmap.isNull())
    {
        return false;
    }

    return CImageUtil::FillFromBITMAPObj(imgBitmap, image);
}

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::LoadJPEG(const QString& strFileName, CImageEx& outImage)
{
    return CImageUtil::LoadImageWithGDIPlus(strFileName, outImage);
}

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::SavePGM(const QString& fileName, const CImageEx& image)
{
    // There are two types of PGM ("Portable Grey Map") files - "raw" (binary) and "plain" (ASCII).  This function supports the "plain PGM"  format.
    // See http://netpbm.sourceforge.net/doc/pgm.html or https://en.wikipedia.org/wiki/Netpbm_format for the definition.

    uint32 width = image.GetWidth();
    uint32 height = image.GetHeight();
    uint32* pixels = image.GetData();

    // Create the file header.
    string fileHeader;
    fileHeader.Format(
        // P2 = PGM header for ASCII output.  (P5 is PGM header for binary output)
        "P2\n"
        // width and height of the image
        "%d %d\n"
        // The maximum grey value in the file.  (i.e. the max value for any given pixel below)
        "65535\n"
        , width, height);

    FILE* file = fopen(fileName.toUtf8().data(), "wt");
    if (!file)
    {
        return false;
    }

    // First print the file header
    fprintf(file, fileHeader.c_str());

    // Then print all the pixels.
    for (int32 y = 0; y < height; y++)
    {
        for (int32 x = 0; x < width; x++)
        {
            fprintf(file, "%d ", pixels[x + (y * width)]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::LoadPGM(const QString& fileName, CImageEx& image)
{
    FILE* file = fopen(fileName.toLatin1().data(), "rt");
    if (!file)
    {
        return false;
    }

    const char seps[] = " \n\t";
    char* token;


    int32 width = 0;
    int32 height = 0;
    int32 numColors = 1;


    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* str = new char[fileSize];
    fread(str, fileSize, 1, file);

    token = strtok(str, seps);

    while (token != NULL && token[0] == '#')
    {
        if (token != NULL && token[0] == '#')
        {
            strtok(NULL, "\n");
        }
        token = strtok(NULL, seps);
    }
    if (_stricmp(token, "P2") != 0)
    {
        // Bad file. not supported pgm.
        delete[]str;
        fclose(file);
        return false;
    }

    do
    {
        token = strtok(NULL, seps);
        if (token != NULL && token[0] == '#')
        {
            strtok(NULL, "\n");
        }
    } while (token != NULL && token[0] == '#');
    width = atoi(token);

    do
    {
        token = strtok(NULL, seps);
        if (token != NULL && token[0] == '#')
        {
            strtok(NULL, "\n");
        }
    } while (token != NULL && token[0] == '#');
    height = atoi(token);

    do
    {
        token = strtok(NULL, seps);
        if (token != NULL && token[0] == '#')
        {
            strtok(NULL, "\n");
        }
    } while (token != NULL && token[0] == '#');
    numColors = atoi(token);

    image.Allocate(width, height);

    uint32* p = image.GetData();
    int size = width * height;
    int i = 0;
    while (token != NULL && i < size)
    {
        do
        {
            token = strtok(NULL, seps);
        } while (token != NULL && token[0] == '#');
        *p++ = atoi(token);
        i++;
    }

    delete[]str;

    fclose(file);

    // If we have 16-bit greyscale values that we're storing into 32-bit pixels, denote it with an appropriate texture type.
    if (numColors > 255)
    {
        image.SetFormat(eTF_R16G16);
    }

    return true;
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
static inline uint16 us_endian (const byte* ptr)
{
    short n;
    memcpy(&n, ptr, sizeof(n));
    return n;
}

static inline unsigned long ul_endian (const byte* ptr)
{
    uint32 n;
    memcpy(&n, ptr, sizeof(n));
    return n;
}

static inline long l_endian (const byte* ptr)
{
    int32 n;
    memcpy(&n, ptr, sizeof(n));
    return n;
}

#define BFTYPE(x)    us_endian((x) +  0)
#define BFSIZE(x)    ul_endian((x) +  2)
#define BFOFFBITS(x) ul_endian((x) + 10)
#define BISIZE(x)    ul_endian((x) + 14)
#define BIWIDTH(x)   l_endian ((x) + 18)
#define BIHEIGHT(x)  l_endian ((x) + 22)
#define BITCOUNT(x)  us_endian((x) + 28)
#define BICOMP(x)    ul_endian((x) + 30)
#define IMAGESIZE(x) ul_endian((x) + 34)
#define BICLRUSED(x) ul_endian((x) + 46)
#define BICLRIMP(x)  ul_endian((x) + 50)
#define BIPALETTE(x) ((x) + 54)

// Type ID
#define BM "BM" // Windows 3.1x, 95, NT, ...
#define BA "BA" // OS/2 Bitmap Array
#define CI "CI" // OS/2 Color Icon
#define CP "CP" // OS/2 Color Pointer
#define IC "IC" // OS/2 Icon
#define PT "PT" // OS/2 Pointer

// Possible values for the header size
#define WinHSize   0x28
#define OS21xHSize 0x0C
#define OS22xHSize 0xF0

// Possible values for the BPP setting
#define Mono          1  // Monochrome bitmap
#define _16Color      4  // 16 color bitmap
#define _256Color     8  // 256 color bitmap
#define HIGHCOLOR    16  // 16bit (high color) bitmap
#define TRUECOLOR24  24  // 24bit (true color) bitmap
#define TRUECOLOR32  32  // 32bit (true color) bitmap

// Compression Types
#ifndef BI_RGB
#define BI_RGB        0  // none
#define BI_RLE8       1  // RLE 8-bit / pixel
#define BI_RLE4       2  // RLE 4-bit / pixel
#define BI_BITFIELDS  3  // Bitfields
#endif

#pragma pack(push,1)
struct SRGBcolor
{
    uint8 red, green, blue;
};
struct SRGBPixel
{
    uint8 red, green, blue, alpha;
};
#pragma pack(pop)

//===========================================================================
bool CImageUtil::LoadBmp(const QString& fileName, CImageEx& image)
{
    std::vector<uint8> data;

    CCryFile file;
    if (!file.Open(fileName.toLatin1().data(), "rb"))
    {
        CLogFile::FormatLine("File not found %s", fileName.toLatin1().data());
        return false;
    }

    long iSize = file.GetLength();

    data.resize(iSize);
    uint8* iBuffer = &data[0];
    file.ReadRaw(iBuffer, iSize);

    if (!((memcmp(iBuffer, BM, 2) == 0) && BISIZE(iBuffer) == WinHSize))
    {
        // Not bmp file.
        CLogFile::FormatLine("Invalid BMP file format %s", fileName.toLatin1().data());
        return false;
    }

    int mWidth = BIWIDTH(iBuffer);
    int mHeight = BIHEIGHT(iBuffer);
    image.Allocate(mWidth, mHeight);
    const int bmp_size = mWidth * mHeight;

    byte* iPtr = iBuffer + BFOFFBITS(iBuffer);

    // The last scanline in BMP corresponds to the top line in the image
    int  buffer_y = mWidth * (mHeight - 1);
    bool blip     = false;

    if (BITCOUNT(iBuffer) == _256Color)
    {
        //mpIndexImage = mfGet_IndexImage();
        byte* buffer = new byte[mWidth * mHeight];
        SRGBcolor mspPal[256];
        SRGBcolor* pwork = mspPal;
        byte* inpal   = BIPALETTE(iBuffer);

        for (int color = 0; color < 256; color++, pwork++)
        {
            // Whacky BMP palette is in BGR order.
            pwork->blue  = *inpal++;
            pwork->green = *inpal++;
            pwork->red   = *inpal++;
            inpal++; // Skip unused byte.
        }

        if (BICOMP(iBuffer) == BI_RGB)
        {
            // Read the pixels from "top" to "bottom"
            while (iPtr < iBuffer + iSize && buffer_y >= 0)
            {
                memcpy (buffer + buffer_y, iPtr, mWidth);
                iPtr += mWidth;
                buffer_y -= mWidth;
            } /* endwhile */
        }
        else
        if (BICOMP(iBuffer) == BI_RLE8)
        {
            // Decompress pixel data
            byte rl, rl1, i;    // runlength
            byte clridx, clridx1; // colorindex
            int buffer_x = 0;
            while (iPtr < iBuffer + iSize && buffer_y >= 0)
            {
                rl = rl1 = *iPtr++;
                clridx = clridx1 = *iPtr++;
                if (rl == 0)
                {
                    if (clridx == 0)
                    {
                        // new scanline
                        if (!blip)
                        {
                            // if we didnt already jumped to the new line, do it now
                            buffer_x  = 0;
                            buffer_y -= mWidth;
                        }
                        continue;
                    }
                    else
                    if (clridx == 1)
                    {
                        // end of bitmap
                        break;
                    }
                    else
                    if (clridx == 2)
                    {
                        // next 2 bytes mean column- and scanline- offset
                        buffer_x += *iPtr++;
                        buffer_y -= (mWidth * (*iPtr++));
                        continue;
                    }
                    else
                    if (clridx > 2)
                    {
                        rl1 = clridx;
                    }
                }

                for (i = 0; i < rl1; i++)
                {
                    if (!rl)
                    {
                        clridx1 = *iPtr++;
                    }
                    buffer [buffer_y + buffer_x] = clridx1;

                    if (++buffer_x >= mWidth)
                    {
                        buffer_x  = 0;
                        buffer_y -= mWidth;
                        blip = true;
                    }
                    else
                    {
                        blip = false;
                    }
                }
                // pad in case rl == 0 and clridx in [3..255]
                if (rl == 0 && (clridx & 0x01))
                {
                    iPtr++;
                }
            }
        }

        // Convert indexed to RGBA
        for (int y = 0; y < mHeight; y++)
        {
            for (int x = 0; x < mWidth; x++)
            {
                SRGBcolor& entry = mspPal[buffer[x + y * mWidth]];
                image.ValueAt(x, y) = 0xFF000000 | RGB(entry.red, entry.green, entry.blue);
            }
        }

        delete []buffer;
        return true;
    }
    else
    if (!BICLRUSED(iBuffer) && BITCOUNT(iBuffer) == TRUECOLOR24)
    {
        int iAddToPitch = (mWidth * 3) % 4;
        if (iAddToPitch)
        {
            iAddToPitch = 4 - iAddToPitch;
        }

        SRGBPixel* buffer = (SRGBPixel*)image.GetData();

        while (iPtr < iBuffer + iSize && buffer_y >= 0)
        {
            SRGBPixel* d = buffer + buffer_y;
            for (int x = mWidth; x; x--)
            {
                d->blue    = *iPtr++;
                d->green   = *iPtr++;
                d->red     = *iPtr++;
                d->alpha = 255;
                d++;
            } /* endfor */

            iPtr += iAddToPitch;

            buffer_y -= mWidth;
        }
        return true;
    }
    else
    if (!BICLRUSED(iBuffer) && BITCOUNT(iBuffer) == TRUECOLOR32)
    {
        SRGBPixel* buffer = (SRGBPixel*)image.GetData();

        while (iPtr < iBuffer + iSize && buffer_y >= 0)
        {
            SRGBPixel* d = buffer + buffer_y;
            for (int x = mWidth; x; x--)
            {
                d->blue    = *iPtr++;
                d->green   = *iPtr++;
                d->red     = *iPtr++;
                d->alpha   = *iPtr++;
                d++;
            } /* endfor */

            buffer_y -= mWidth;
        }
        return true;
    }

    CLogFile::FormatLine("Unknown BMP image format %s", fileName.toLatin1().data());

    return false;
}

//===========================================================================
bool CImageUtil::LoadBmp(const QString& fileName, CImageEx& image, const RECT& rc)
{
#pragma pack(push,1)
    struct SRGBcolor
    {
        uint8 red, green, blue;
    };
    struct SRGBPixel
    {
        uint8 red, green, blue, alpha;
    };
#pragma pack(pop)

    std::vector<uint8> header;

    CCryFile file;
    if (!file.Open(fileName.toLatin1().data(), "rb"))
    {
        CLogFile::FormatLine("File not found %s", fileName.toLatin1().data());
        return false;
    }

    long iSizeHeader = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    header.resize(iSizeHeader);


    //uint8* iBuffer = &data[0];
    uint8* iBuffer = &header[0];

    file.ReadRaw(iBuffer, iSizeHeader);

    if (!((memcmp(iBuffer, BM, 2) == 0) && BISIZE(iBuffer) == WinHSize))
    {
        // Not bmp file.
        CLogFile::FormatLine("Invalid BMP file format %s", fileName.toLatin1().data());
        return false;
    }

    int mWidth = BIWIDTH(iBuffer);
    int mHeight = BIHEIGHT(iBuffer);
    const int bmp_size = mWidth * mHeight;

    long offset = BFOFFBITS(iBuffer) - iSizeHeader;
    if (offset > 0)
    {
        std::vector<uint8> data;
        data.resize(offset);
        file.ReadRaw(&data[0], offset);
    }

    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;


    std::vector<uint8> data;

    int st = 4;
    if (BITCOUNT(iBuffer) == TRUECOLOR24)
    {
        st = 3;
    }

    int iAddToPitch = (mWidth * st) % 4;
    if (iAddToPitch)
    {
        iAddToPitch = 4 - iAddToPitch;
    }

    long iSize = mWidth * st + iAddToPitch;

    data.resize(iSize);

    //for (int y = mHeight-1; y > rc.bottom; y--)
    //file.ReadRaw( &data[0], iSize );

    file.Seek(file.GetPosition() + (mHeight - rc.bottom) * iSize, SEEK_SET);

    if (!BICLRUSED(iBuffer) && (BITCOUNT(iBuffer) == TRUECOLOR24 || BITCOUNT(iBuffer) == TRUECOLOR32))
    {
        image.Allocate(w, h);
        SRGBPixel* buffer = (SRGBPixel*)image.GetData();

        for (int y = h - 1; y >= 0; y--)
        {
            file.ReadRaw(&data[0], iSize);

            for (int x = 0; x < w; x++)
            {
                buffer[x + y * w].blue  = data[(x + rc.left) * st];
                buffer[x + y * w].green = data[(x + rc.left) * st + 1];
                buffer[x + y * w].red   = data[(x + rc.left) * st + 2];
                if (BITCOUNT(iBuffer) == TRUECOLOR24)
                {
                    buffer[x + y * w].alpha = 255;
                }
                else
                {
                    buffer[x + y * w].alpha = data[(x + rc.left) * st + 3];
                }
            }
        }
        return true;
    }

    CLogFile::FormatLine("Unknown BMP image format %s", fileName.toLatin1().data());
    return false;
}


//////////////////////////////////////////////////////////////////////////
bool CImageUtil::LoadImage(const QString& fileName, CImageEx& image, bool* pQualityLoss)
{
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

    if (pQualityLoss)
    {
        *pQualityLoss = false;
    }

    _splitpath(fileName.toLatin1().data(), drive, dir, fname, ext);

    // Only DDS has explicit sRGB flag - we'll assume by default all formats are stored in gamma space
    image.SetSRGB(true);

    if (_stricmp(ext, ".bmp") == 0)
    {
        return LoadBmp(fileName, image);
    }
    else if (_stricmp(ext, ".tif") == 0)
    {
        return CImageTIF().Load(fileName, image);
    }
    else if (_stricmp(ext, ".jpg") == 0)
    {
        if (pQualityLoss)
        {
            *pQualityLoss = true;     // we assume JPG has quality loss
        }
        return LoadJPEG(fileName, image);
    }
    else if (_stricmp(ext, ".gif") == 0)
    {
        return CImageGif().Load(fileName, image);
    }
    else if (_stricmp(ext, ".pgm") == 0)
    {
        return LoadPGM(fileName, image);
    }
    else if (_stricmp(ext, ".dds") == 0)
    {
        return CImage_DXTC().Load(fileName.toLatin1().data(), image, pQualityLoss);
    }
    else if (_stricmp(ext, ".png") == 0)
    {
        return CImageUtil::LoadImageWithGDIPlus(fileName, image);
    }
    else if (stricmp(ext, ".hdr") == 0)
    {
        return CImageHDR().Load(fileName.toLatin1().data(), image);
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::SaveImage(const QString& fileName, CImageEx& image)
{
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

    // Remove the read-only attribute so the file can be overwritten.
    QFile(fileName).setPermissions(QFile::ReadOther | QFile::WriteOther);

    _splitpath(fileName.toLatin1().data(), drive, dir, fname, ext);
    if (_stricmp(ext, ".bmp") == 0)
    {
        return SaveBitmap(fileName, image);
    }
    else if (_stricmp(ext, ".jpg") == 0)
    {
        return SaveJPEG(fileName, image);
    }
    else if (_stricmp(ext, ".pgm") == 0)
    {
        return SavePGM(fileName, image);
    }
    else if (_stricmp(ext, ".png") == 0)
    {
        return Save(fileName, image);
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CImageUtil::ScaleToFit(const CByteImage& srcImage, CByteImage& trgImage)
{
    trgImage.ScaleToFit(srcImage);
}

//////////////////////////////////////////////////////////////////////////
void CImageUtil::DownScaleSquareTextureTwice(const CImageEx& srcImage, CImageEx& trgImage, IImageUtil::_EAddrMode eAddressingMode)
{
    uint32* pSrcData = srcImage.GetData();
    int nSrcWidth = srcImage.GetWidth();
    int nSrcHeight = srcImage.GetHeight();
    int nTrgWidth = srcImage.GetWidth() >> 1;
    int nTrgHeight = srcImage.GetHeight() >> 1;

    // reallocate target
    trgImage.Release();
    trgImage.Allocate(nTrgWidth, nTrgHeight);
    uint32* pDstData = trgImage.GetData();

    // values in this filter are the log2 of the actual multiplicative values .. see DXCFILTER_BLUR3X3 for the used 3x3 filter
    static int filter[3][3] =
    {
        {0, 1, 0},
        {1, 2, 1},
        {0, 1, 0}
    };

    for (int i = 0; i < nTrgHeight; i++)
    {
        for (int j = 0; j < nTrgWidth; j++)
        {
            // filter3x3
            int x = j << 1;
            int y = i << 1;

            int r, g, b, a;
            r = b = g = a = 0;
            uint32 col;

            if (eAddressingMode == IImageUtil::WRAP) // TODO: this condition could be compile-time static by making it a template arg
            {
                for (int i = 0; i < 3; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        col = pSrcData[((y + nSrcHeight + i - 1) % nSrcHeight) * nSrcWidth + ((x + nSrcWidth + j - 1) % nSrcWidth)];

                        r += (col        & 0xff) << filter[i][j];
                        g += ((col >> 8)  & 0xff) << filter[i][j];
                        b += ((col >> 16) & 0xff) << filter[i][j];
                        a += ((col >> 24) & 0xff) << filter[i][j];
                    }
                }
            }
            else
            {
                assert(eAddressingMode == IImageUtil::CLAMP);
                for (int i = 0; i < 3; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        int x1 = clamp_tpl<int>((x + j), 0, nSrcWidth - 1);
                        int y1 = clamp_tpl<int>((y + i), 0, nSrcHeight - 1);
                        col = pSrcData[ y1 * nSrcWidth + x1];

                        r += (col        & 0xff) << filter[i][j];
                        g += ((col >> 8)  & 0xff) << filter[i][j];
                        b += ((col >> 16) & 0xff) << filter[i][j];
                        a += ((col >> 24) & 0xff) << filter[i][j];
                    }
                }
            }

            // the sum of the multiplicative values here is 16 so we shift by 4 bits
            r >>= 4;
            g >>= 4;
            b >>= 4;
            a >>= 4;

            uint32 res = r + (g << 8) + (b << 16) + (a << 24);

            *pDstData++ = res;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CImageUtil::ScaleToFit(const CImageEx& srcImage, CImageEx& trgImage)
{
    trgImage.ScaleToFit(srcImage);
}

//////////////////////////////////////////////////////////////////////////
void CImageUtil::ScaleToDoubleFit(const CImageEx& srcImage, CImageEx& trgImage)
{
    uint32 x, y, u, v;
    unsigned int* destRow, * dest, * src, * sourceRow;

    uint32 srcW = srcImage.GetWidth();
    uint32 srcH = srcImage.GetHeight();

    uint32 trgHalfW = trgImage.GetWidth() / 2;
    uint32 trgH = trgImage.GetHeight();

    uint32 xratio = trgHalfW > 0 ? (srcW << 16) / trgHalfW : 1;
    uint32 yratio = trgH > 0 ? (srcH << 16) / trgH : 1;

    src = srcImage.GetData();
    destRow = trgImage.GetData();

    v = 0;
    for (y = 0; y < trgH; y++)
    {
        u = 0;
        sourceRow = src + (v >> 16) * srcW;
        dest = destRow;
        for (x = 0; x < trgHalfW; x++)
        {
            *(dest + trgHalfW) = sourceRow[u >> 16];
            *dest++ = sourceRow[u >> 16];
            u += xratio;
        }
        v += yratio;
        destRow += trgHalfW * 2;
    }
}

//////////////////////////////////////////////////////////////////////////
void CImageUtil::SmoothImage(CByteImage& image, int numSteps)
{
    assert(numSteps > 0);
    uint8* buf = image.GetData();
    int w = image.GetWidth();
    int h = image.GetHeight();

    for (int steps = 0; steps < numSteps; steps++)
    {
        // Smooth the image.
        for (int y = 1; y < h - 1; y++)
        {
            // Precalculate for better speed
            uint8* ptr = &buf[y * w + 1];

            for (int x = 1; x < w - 1; x++)
            {
                // Smooth it out
                *ptr =
                    (
                        (uint32)ptr[1] +
                        ptr[w] +
                        ptr[-1] +
                        ptr[-w] +
                        ptr[w + 1] +
                        ptr[w - 1] +
                        ptr[-w + 1] +
                        ptr[-w - 1]
                    ) >> 3;

                // Next pixel
                ptr++;
            }
        }
    }
}

unsigned char CImageUtil::GetBilinearFilteredAt(const int iniX256, const int iniY256, const CByteImage& image)
{
    //  assert(image.IsValid());        if(!image.IsValid())return(0);      // this shouldn't be

    DWORD x = (DWORD)(iniX256) >> 8;
    DWORD y = (DWORD)(iniY256) >> 8;

    if (x >= image.GetWidth() - 1 || y >= image.GetHeight() - 1)
    {
        return image.ValueAt(x, y);                                                          // border is not filtered, 255 to get in range 0..1
    }
    DWORD rx = (DWORD)(iniX256) & 0xff;       // fractional aprt
    DWORD ry = (DWORD)(iniY256) & 0xff;       // fractional aprt

    DWORD top = (DWORD)image.ValueAt((int)x, (int)y) * (256 - rx)                      // left top
        + (DWORD)image.ValueAt((int)x + 1, (int)y) * rx;                                            // right top

    DWORD bottom = (DWORD)image.ValueAt((int)x, (int)y + 1) * (256 - rx)              // left bottom
        + (DWORD)image.ValueAt((int)x + 1, (int)y + 1) * rx;                                        // right bottom

    return (unsigned char)((top * (256 - ry) + bottom * ry) >> 16);
}

bool CImageUtil::FillFromBITMAPObj(const QImage& bitmap, CImageEx& image)
{
    if (bitmap.depth() != 32)
    {
        return false;
    }

    image.Allocate(bitmap.width(), bitmap.height());

    auto src_p = bitmap.bits();

    for (int i = 0; i < bitmap.height(); i++)
    {
        for (int k = 0; k < bitmap.width(); k++)
        {
            SRGBPixel*      dest_p  =   (SRGBPixel*)(&(image.ValueAt(k, i)));

            dest_p->blue            = *src_p++;
            dest_p->green   = *src_p++;
            dest_p->red         = *src_p++;
            dest_p->alpha       = *src_p++;
        }
    }

    return true;
}

bool CImageUtil::CreateBitmapFromImage(const CImageEx& image, QImage& bitmapObj)
{
    bitmapObj = QImage(image.GetWidth(), image.GetHeight(), QImage::Format_RGBA8888);
    std::copy(image.GetData(), image.GetData() + image.GetWidth() * image.GetHeight(), reinterpret_cast<uint32*>(bitmapObj.bits()));

    return true;
}
