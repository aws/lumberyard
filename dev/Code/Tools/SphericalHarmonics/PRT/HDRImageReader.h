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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_HDRIMAGEREADER_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_HDRIMAGEREADER_H
#pragma once

#if defined(OFFLINE_COMPUTATION)


#include "IImageReader.h"

namespace NImage
{
    class CHDRImageReader
        : public IImageReader
    {
    public:
        INSTALL_CLASS_NEW(CHDRImageReader)
        //!< loads a hdr image with 3x32 bit floats
        virtual const void* ReadImage(const char* cpImageName, uint32 & rImageWidth, uint32 & rImageheight, EImageFileFormat & rFormat, const uint32) const;
        virtual ~CHDRImageReader(){}

    private:
        typedef unsigned char TRGBE[4];     //!< encoded colour
        //TRGBE accessors
        static const uint32 R = 0;
        static const uint32 G = 1;
        static const uint32 B = 2;
        static const uint32 E = 3;

        const float ConvertComponent(const int cExpo, const int cVal) const;                                //!< helper function, converts the stored component
        void ProcessRGBE(TRGBE* pScan, const int cLen, float* pCols, const uint32 cBPP) const;                  //!< helper function, processes one encoded colour
        const bool Decrunch(TRGBE* pScanline, const int cLen, FILE* pFile) const;       //!< helper function, reads a single scan line
        const bool OldDecrunch(TRGBE* pScanline, const int cLen, FILE* pFile) const;    //!< helper function for Decrunch
    };
}

#endif
#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_HDRIMAGEREADER_H
