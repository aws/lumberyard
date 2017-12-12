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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_IIMAGEREADER_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_IIMAGEREADER_H
#pragma once

#if defined(OFFLINE_COMPUTATION)


#include <PRT/PRTTypes.h>
#include <exception>

namespace NImage
{
    //!< image float file format
    enum EImageFileFormat
    {
        A32B32G32R32F,
        B32G32R32F
    };

    //!< image reader interface
    struct IImageReader
    {
        //!< to have this method at least in common, a void pointer needs to be returned
        virtual const void* ReadImage(const char* cpImageName, uint32& rImageWidth, uint32& rImageHeight, EImageFileFormat& rFormat, const uint32 cBPP = 4) const = 0;
        //!< need virtual destructor here since inherited classes are using allocator
        virtual ~IImageReader(){}
    };
}

#endif
#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_IIMAGEREADER_H
