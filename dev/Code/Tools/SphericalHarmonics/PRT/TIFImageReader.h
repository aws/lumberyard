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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_TIFIMAGEREADER_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_TIFIMAGEREADER_H
#pragma once

#if defined(OFFLINE_COMPUTATION)



#include "IImageReader.h"


namespace NImage
{
    class CTIFImageReader
        : public IImageReader
    {
    public:
        INSTALL_CLASS_NEW(CTIFImageReader)

        virtual const void* ReadImage(const char* cpImageName, uint32 & rImageWidth, uint32 & rImageheight, EImageFileFormat & rFormat, const uint32) const;
        virtual ~CTIFImageReader(){}
    private:
    };
}

#endif
#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_TIFIMAGEREADER_H
