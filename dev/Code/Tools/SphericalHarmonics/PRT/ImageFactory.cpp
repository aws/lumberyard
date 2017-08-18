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

#include "ImageFactory.h"
#include "HDRImageReader.h"
#include "CommonImageReader.h"
#include "TIFImageReader.h"

NImage::CImageFactory* NImage::CImageFactory::Instance()
{
    static CImageFactory inst;
    return &inst;
}

const NSH::CSmartPtr<const NImage::IImageReader, CSHAllocator<> > NImage::CImageFactory::GetImageReader(const NImage::CImageFactory::EImageType cImageType)
{
    switch (cImageType)
    {
    case IMAGE_TYPE_HDR24:
        return NSH::CSmartPtr<const CHDRImageReader, CSHAllocator<> >(new CHDRImageReader);
        break;
#if defined(USE_D3DX)
    case IMAGE_TYPE_COMMON:
        return NSH::CSmartPtr<const CCommonImageReader, CSHAllocator<> >(new CCommonImageReader);
        break;
#endif
    case IMAGE_TYPE_TIFF:
        return NSH::CSmartPtr<const CTIFImageReader, CSHAllocator<> >(new CTIFImageReader);
        break;
    default:
        assert(false);
    }
    return NSH::CSmartPtr<const NImage::IImageReader, CSHAllocator<> >(NULL);
}

#endif