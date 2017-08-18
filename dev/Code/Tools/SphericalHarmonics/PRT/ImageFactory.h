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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_IMAGEFACTORY_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_IMAGEFACTORY_H
#pragma once

#if defined(OFFLINE_COMPUTATION)


#include <exception>
#include "IImageReader.h"

namespace NImage
{
    class CHDRImageReader;
#if defined(USE_D3DX)
    class CCommonImageReader;
#endif
    //!< singleton to abstract image factory
    class CImageFactory
    {
    public:
        typedef enum EImageType
        {
            IMAGE_TYPE_HDR24,           //!< HDR map with 3xfloat 32
#if defined(USE_D3DX)
            IMAGE_TYPE_COMMON,      //!< normal format supported by DirectX
#endif
            IMAGE_TYPE_TIFF,            //!< tiff format(not supported by directx)
        }EImageType;

        //!< singleton stuff
        static CImageFactory* Instance();

        const NSH::CSmartPtr<const IImageReader, CSHAllocator<> >  GetImageReader(const EImageType cImageType);

    private:
        //!< singleton stuff
        CImageFactory(){}
        CImageFactory(const CImageFactory&);
        CImageFactory& operator= (const CImageFactory&);
    };
}

#endif

#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_IMAGEFACTORY_H
