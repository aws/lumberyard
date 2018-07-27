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

#include "stdafx.h"

#if defined(OFFLINE_COMPUTATION)

#include "HDRImageReader.h"
#include <memory.h>
#include <stdio.h>



const void* NImage::CHDRImageReader::ReadImage
(
    const char* cpImageName,
    uint32& rImageWidth,
    uint32& rImageHeight,
    NImage::EImageFileFormat& rFormat,
    const uint32
) const
{
    const uint32 cBPP = 3;
    //reads hdr files
    FILE* pFile = nullptr;
    azfopen(&pFile, cpImageName, "rb");
    if (!pFile)
    {
        char message[200];
        azsprintf(message, "Image file could not been opened for reading: %s\n", cpImageName);
        GetSHLog().LogError(message);
        return NULL;
    }
    //read header
    char str[200];

    fread(str, 10, 1, pFile);
    if (memcmp(str, "#?RADIANCE", 10))
    {
        char message[200];
        azsprintf(message, "Image file not in correct hdr format(#?RADIANCE tag not found): %s\n", cpImageName);
        fclose(pFile);
        GetSHLog().LogError(message);
        return NULL;
    }
    fseek(pFile, 1, SEEK_CUR);

    char cmd[2000];
    int i = 0;
    char c = 0, oldc;
    for (;; )
    {
        oldc = c;
        c = (char)fgetc(pFile);
        if (c == 0xa && oldc == 0xa)
        {
            break;
        }
        cmd[i++] = c;
    }

    char reso[200];
    for (int i = 0;; )
    {
        c = (char)fgetc(pFile);
        reso[i++] = c;
        if (c == 0xa)
        {
            break;
        }
    }

    long w, h;
    if (!azsscanf(reso, "-Y %ld +X %ld", &h, &w))
    {
        char message[200];
        azsprintf(message, "Image file not in correct hdr format(could not read width and height): %s\n", cpImageName);
        fclose(pFile);
        GetSHLog().LogError(message);
        return NULL;
    }

    rImageWidth = w;
    rImageHeight = h;

    static CSHAllocator<float> sAllocator;
    float* pOutput = (float*)(sAllocator.new_mem_array(sizeof(float) * rImageWidth * rImageWidth * cBPP));

    assert(pOutput);

    static CSHAllocator<TRGBE> sAllocatorTRGBE;
    TRGBE* pScanLine = (TRGBE*)(sAllocatorTRGBE.new_mem_array(sizeof(TRGBE) * rImageWidth));
    if (!pScanLine)
    {
        char message[200];
        azsprintf(message, "Image file not in correct hdr format(could not read scan line): %s\n", cpImageName);
        fclose(pFile);
        GetSHLog().LogError(message);
        return NULL;
    }

    float* pCurrentDest = pOutput;

    // convert image
    for (int y = rImageHeight - 1; y >= 0; y--)
    {
        if (Decrunch(pScanLine, rImageWidth, pFile) == false)
        {
            break;
        }
        ProcessRGBE(pScanLine, rImageWidth, pCurrentDest, cBPP);
        pCurrentDest += w * cBPP;
    }

    rFormat = (cBPP == 4) ? A32B32G32R32F : B32G32R32F;

    sAllocatorTRGBE.delete_mem_array(pScanLine, sizeof(TRGBE) * rImageWidth);

    fclose(pFile);
    return (void*)pOutput;
}

inline const float NImage::CHDRImageReader::ConvertComponent(const int cExpo, const int cVal) const
{
    return (cVal != 0) ? cVal / 256.0f * (float) pow(2.0, cExpo) : 0;
}

inline void NImage::CHDRImageReader::ProcessRGBE(TRGBE* pScan, const int cLen, float* pCols, const uint32 cBPP) const
{
    int len = cLen;
    while (len-- > 0)
    {
        const int cExpo = pScan[0][E] - 128;    //E
        pCols[0] = ConvertComponent(cExpo, pScan[0][R]);    //R
        pCols[1] = ConvertComponent(cExpo, pScan[0][G]);    //G
        pCols[2] = ConvertComponent(cExpo, pScan[0][B]);    //B
        pCols[3] = 1.0f;//A
        pCols += cBPP;
        pScan++;
    }
}

const bool NImage::CHDRImageReader::Decrunch(TRGBE* pScanLine, const int cLen, FILE* pFile) const
{
    static const uint32 s_cMinELength   = 8;                    // minimum scanline length for encoding
    static const uint32 s_cMaxELength   = 0x7FFF;           // maximum scanline length for encoding

    if (cLen < s_cMinELength || cLen > s_cMaxELength)
    {
        return OldDecrunch(pScanLine, cLen, pFile);
    }

    int i = fgetc(pFile);
    if (i != 2)
    {
        fseek(pFile, -1, SEEK_CUR);
        return OldDecrunch(pScanLine, cLen, pFile);
    }

    pScanLine[0][G] = (unsigned char)fgetc(pFile);
    pScanLine[0][B] = (unsigned char)fgetc(pFile);
    i = fgetc(pFile);

    if (pScanLine[0][G] != 2 || pScanLine[0][B] & 128)
    {
        pScanLine[0][R] = 2;
        pScanLine[0][E] = (unsigned char)i;
        return OldDecrunch(pScanLine + 1, cLen - 1, pFile);
    }

    // read each component
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < cLen; )
        {
            unsigned char code = (unsigned char)fgetc(pFile);
            if (code > 128)
            { // run
                code &= 127;
                const unsigned char val = (unsigned char)fgetc(pFile);
                while (code--)
                {
                    pScanLine[j++][i] = val;
                }
            }
            else
            {   // non-run
                while (code--)
                {
                    pScanLine[j++][i] = (unsigned char)fgetc(pFile);
                }
            }
        }
    }
    return feof(pFile) ? false : true;
}

const bool NImage::CHDRImageReader::OldDecrunch(TRGBE* pScanLine, const int cLen, FILE* pFile) const
{
    int len = cLen;
    int rshift = 0;
    while (len > 0)
    {
        pScanLine[0][R] = (unsigned char)fgetc(pFile);
        pScanLine[0][G] = (unsigned char)fgetc(pFile);
        pScanLine[0][B] = (unsigned char)fgetc(pFile);
        pScanLine[0][E] = (unsigned char)fgetc(pFile);
        if (feof(pFile))
        {
            return false;
        }

        if (pScanLine[0][R] == 1 && pScanLine[0][G] == 1 && pScanLine[0][B] == 1)
        {
            for (int i = pScanLine[0][E] << rshift; i > 0; i--)
            {
                memcpy(&pScanLine[0][0], &pScanLine[-1][0], 4);
                pScanLine++;
                len--;
            }
            rshift += 8;
        }
        else
        {
            pScanLine++;
            len--;
            rshift = 0;
        }
    }
    return true;
}

#endif