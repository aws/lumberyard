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

// Description : BMPHelper


#include "CryLegacy_precompiled.h"
#include <platform.h>
#include "BMPHelper.h"
#include "RichSaveGameTypes.h"

#if defined(NEED_ENDIAN_SWAP)
// big endian
static const unsigned short BF_TYPE = 0x424D;            /* 0x42 == 'B', 0x4D == 'M' */
#else
// little endian
static const unsigned short BF_TYPE = 0x4D42;            /* 0x42 == 'B', 0x4D == 'M' */
#endif

#define COMPRESSION_BI_RGB 0

namespace BMPHelper
{
    size_t CalcBMPSize(int width, int height, int depth)
    {
        int bytesPerLine = width * depth;
        if (bytesPerLine & 0x0003)
        {
            bytesPerLine |= 0x0003;
            ++bytesPerLine;
        }
        size_t totalSize = bytesPerLine * height + sizeof(CryBitmapFileHeader) + sizeof(CryBitmapInfoHeader);
        return totalSize;
    }

    // pByteData: B G R A
    // if pFile != 0 writes into file. otherwise opens a new file with filename 'filename'
    // returns size of written bytes in outFileSize
    // can be optimized quite a bit
    bool InternalSaveBMP(const char* filename, AZ::IO::HandleType fileHandle, uint8* pByteData, int width, int height, int depth, bool inverseY)
    {
        CryBitmapFileHeader bitmapfileheader;
        CryBitmapInfoHeader bitmapinfoheader;

        ICryPak* pCryPak = gEnv->pCryPak;
        const bool bFileGiven = fileHandle != AZ::IO::InvalidHandle;
        if (!bFileGiven)
        {
            fileHandle = pCryPak->FOpen(filename, "wb");
        }
        if (fileHandle == AZ::IO::InvalidHandle)
        {
            return false;
        }

        int origBytesPerLine = width * depth;
        int bytesPerLine = origBytesPerLine;
        if (bytesPerLine & 0x0003)
        {
            bytesPerLine |= 0x0003;
            ++bytesPerLine;
        }
        int padBytes = bytesPerLine - origBytesPerLine; // how many pad-bytes

        uint8* pTempImage = new uint8[bytesPerLine * height]; // optimize and write one line only

        for (int y = 0; y < height; y++)
        {
            int src_y = y;
            if (inverseY)
            {
                src_y = (height - 1) - y;
            }

            uint8* pDstLine = pTempImage + y * bytesPerLine;
            uint8* pSrcLine = pByteData + src_y * origBytesPerLine;
            if (depth == 3)
            {
                uint8 r, g, b;
                for (int x = 0; x < width; x++)
                {
                    b = *pSrcLine++;
                    g = *pSrcLine++;
                    r = *pSrcLine++;
                    *pDstLine++ = b;
                    *pDstLine++ = g;
                    *pDstLine++ = r;
                }
            }
            else
            {
                uint8 r, g, b, a;
                for (int x = 0; x < width; x++)
                {
                    b = *pSrcLine++;
                    g = *pSrcLine++;
                    r = *pSrcLine++;
                    a = *pSrcLine++;
                    *pDstLine++ = b;
                    *pDstLine++ = g;
                    *pDstLine++ = r;
                    *pDstLine++ = a;
                }
            }
            for (int n = 0; n < padBytes; ++n)
            {
                *pDstLine++ = 0;
            }
        }

        // Fill in bitmap structures
        bitmapfileheader.bfType = BF_TYPE;
        bitmapfileheader.bfSize = bytesPerLine * height + sizeof(CryBitmapFileHeader) + sizeof(CryBitmapInfoHeader);
        bitmapfileheader.bfReserved1 = 0;
        bitmapfileheader.bfReserved2 = 0;
        bitmapfileheader.bfOffBits = sizeof(CryBitmapFileHeader) + sizeof(CryBitmapInfoHeader);
        bitmapinfoheader.biSize = sizeof(CryBitmapInfoHeader);
        bitmapinfoheader.biWidth = width;
        bitmapinfoheader.biHeight = height;
        bitmapinfoheader.biPlanes = 1;
        bitmapinfoheader.biBitCount = depth * 8;
        bitmapinfoheader.biCompression = COMPRESSION_BI_RGB;
        bitmapinfoheader.biSizeImage = 0;
        bitmapinfoheader.biXPelsPerMeter = 0;
        bitmapinfoheader.biYPelsPerMeter = 0;
        bitmapinfoheader.biClrUsed = 0;
        bitmapinfoheader.biClrImportant = 0;

        pCryPak->FWrite(&bitmapfileheader, 1, fileHandle);
        pCryPak->FWrite(&bitmapinfoheader, 1, fileHandle);
        pCryPak->FWrite(pTempImage, bytesPerLine * height, 1, fileHandle);

        if (!bFileGiven)
        {
            pCryPak->FClose(fileHandle);
        }

        delete[] pTempImage;

        // Success
        return true;
    }

    // fills data with BGR or BGRA
    // always top->bottom
    bool InternalLoadBMP(const char* filename, AZ::IO::HandleType fileHandle, uint8* pByteData, int& width, int& height, int& depth, bool bForceInverseY)
    {
        // todo: cleanup for pFile
        struct Cleanup
        {
            Cleanup()
                : m_fileHandle(AZ::IO::InvalidHandle)
                , m_bCloseFile(false)
                , m_bRestoreCur(false)
                , m_offset(0) {}
            ~Cleanup()
            {
                if (m_fileHandle != AZ::IO::InvalidHandle)
                {
                    if (m_bRestoreCur)
                    {
                        gEnv->pCryPak->FSeek(m_fileHandle, m_offset, SEEK_SET);
                    }
                    if (m_bCloseFile)
                    {
                        gEnv->pCryPak->FClose(m_fileHandle);
                    }
                }
            }
            void SetFile(AZ::IO::HandleType handle, bool bClose, bool bRestoreCur)
            {
                m_fileHandle = handle;
                m_bCloseFile = bClose;
                m_bRestoreCur = bRestoreCur;
                if (m_fileHandle != AZ::IO::InvalidHandle && m_bRestoreCur)
                {
                    m_offset = gEnv->pCryPak->FTell(m_fileHandle);
                }
            }
        protected:
            AZ::IO::HandleType m_fileHandle;
            bool  m_bCloseFile;
            bool  m_bRestoreCur;
            long  m_offset;
        };

        Cleanup cleanup; // potentially close the file on return (if pFile was given, do nothing)
        CryBitmapFileHeader bitmapfileheader;
        CryBitmapInfoHeader bitmapinfoheader;

        memset(&bitmapfileheader, 0, sizeof(CryBitmapFileHeader));
        memset(&bitmapinfoheader, 0, sizeof(CryBitmapInfoHeader));

        ICryPak* pCryPak = gEnv->pCryPak;
        const bool bFileGiven = fileHandle != AZ::IO::InvalidHandle;
        if (bFileGiven == false)
        {
            fileHandle = pCryPak->FOpen(filename, "rb");
        }
        if (fileHandle == AZ::IO::InvalidHandle)
        {
            return false;
        }

        cleanup.SetFile(fileHandle, !bFileGiven, bFileGiven);

        size_t els = pCryPak->FRead(&bitmapfileheader, 1, fileHandle);
        if (els != 1)
        {
            return false;
        }

        els = pCryPak->FRead(&bitmapinfoheader, 1, fileHandle);
        if (els != 1)
        {
            return false;
        }

        if (bitmapfileheader.bfType != BF_TYPE)
        {
            return false;
        }

        if (bitmapinfoheader.biCompression != COMPRESSION_BI_RGB)
        {
            return false;
        }

        if (bitmapinfoheader.biBitCount != 24 && bitmapinfoheader.biBitCount != 32)
        {
            return false;
        }

        if (bitmapinfoheader.biPlanes != 1)
        {
            return false;
        }

        int imgWidth = bitmapinfoheader.biWidth;
        int imgHeight = bitmapinfoheader.biHeight;
        int imgDepth = bitmapinfoheader.biBitCount == 24 ? 3 : 4;

        int imgBytesPerLine = imgWidth * imgDepth;
        int fileBytesPerLine = imgBytesPerLine;
        if (fileBytesPerLine & 0x0003)
        {
            fileBytesPerLine |= 0x0003;
            ++fileBytesPerLine;
        }
        size_t prologSize = sizeof(CryBitmapFileHeader) + sizeof(CryBitmapInfoHeader);
        size_t bufSize = bitmapfileheader.bfSize - prologSize;

        // some BMPs from Adobe PhotoShop seem 2 bytes longer
        if (bufSize != fileBytesPerLine * imgHeight && bufSize != (fileBytesPerLine * imgHeight) + 2)
        {
            return false;
        }

        bool bFlipY = true;
        width = imgWidth;
        height = imgHeight;
        if (height < 0)   // height > 0: bottom->top, height < 0: top->bottom, no flip necessary
        {
            height = -height;
            bFlipY = false;
        }
        depth = imgDepth;

        if (pByteData == 0) // only requested dimensions
        {
            return true;
        }

        uint8* pTempImage = new uint8[bufSize];
        if (pTempImage == 0)
        {
            return false;
        }

        els = pCryPak->FReadRaw(pTempImage, 1, bufSize, fileHandle);
        if (els != bufSize)
        {
            delete[] pTempImage;
            return false;
        }

        if (bForceInverseY)
        {
            bFlipY = !bFlipY;
        }

        // read all rows
        uint8* pSrcLine = pTempImage;
        for (int y = 0; y < imgHeight; ++y)
        {
            int dstY = y;
            if (bFlipY)
            {
                dstY = imgHeight - y - 1;
            }

            uint8* pDstLine = pByteData + dstY * imgBytesPerLine;
            memcpy(pDstLine, pSrcLine, imgBytesPerLine);
            pSrcLine += fileBytesPerLine;
        }

        delete[] pTempImage;

        // Success
        return true;
    }

    bool LoadBMP(const char* filename, uint8* pByteData, int& width, int& height, int& depth, bool bForceInverseY)
    {
        return InternalLoadBMP(filename, 0, pByteData, width, height, depth, bForceInverseY);
    }

    bool LoadBMP(AZ::IO::HandleType fileHandle, uint8* pByteData, int& width, int& height, int& depth, bool bForceInverseY)
    {
        return InternalLoadBMP("", fileHandle, pByteData, width, height, depth, bForceInverseY);
    }


    bool SaveBMP(const char* filename, uint8* pByteData, int width, int height, int depth, bool flipY)
    {
        return InternalSaveBMP(filename, 0, pByteData, width, height, depth, flipY);
    }

    bool SaveBMP(AZ::IO::HandleType fileHandle, uint8* pByteData, int width, int height, int depth, bool flipY)
    {
        return InternalSaveBMP("", fileHandle, pByteData, width, height, depth, flipY);
    }
};
