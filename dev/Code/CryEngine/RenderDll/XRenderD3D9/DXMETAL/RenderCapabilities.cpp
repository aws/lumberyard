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
#include "StdAfx.h"
#include "Implementation/GLCommon.hpp"

namespace RenderCapabilities
{
    bool SupportsTextureViews()
    {
        return NCryMetal::SupportTextureViews();
    }

    bool SupportsStencilTextures()
    {
        return NCryMetal::SupportsStencilTextures();
    }

    int GetAvailableMRTbpp()
    {
        return NCryMetal::GetAvailableMRTbpp();
    }

    bool Supports128bppGmemPath()
    {
        return GetAvailableMRTbpp() >= 128;
    }

    bool Supports256bppGmemPath()
    {
        return GetAvailableMRTbpp() >= 256;
    }

    bool SupportsPLSExtension()
    {
        return false;
    }

    bool SupportsFrameBufferFetches()
    {
        return true;
    }
    
    bool SupportsDepthClipping()
    {
#if defined(AZ_PLATFORM_APPLE_OSX)
        return NCryMetal::s_isOsxMinVersion10_11;
#else
        return false;
#endif
    }
    
    void CacheMinOSVersionInfo()
    {
        NCryMetal::CacheMinOSVersionInfo();
    }

    bool SupportsDualSourceBlending()
    {
        // Metal supports dual source blending for devices running OXS >= 10.12 or iOS >= 11.0
        // but you need to declare the "index" of the render target in the shader (half4 Source1 [[ color(0), index(1) ]])
        // Unfortunately HLSLcc is not able to generate this type of declaration because the DX Shader bytecode doesn't
        // distinguish between a normal output or one for dual source blending.
        return false;
    }    
}
