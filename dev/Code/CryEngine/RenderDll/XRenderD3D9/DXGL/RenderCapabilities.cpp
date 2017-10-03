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
#include "../../Common/RenderCapabilities.h"
#include "Implementation/GLCommon.hpp"
#include "Interfaces/CCryDXGLDevice.hpp"
#include "Implementation/GLDevice.hpp"

namespace RenderCapabilities
{
    NCryOpenGL::CDevice* GetGLDevice()
    {
        AZ_Assert(gcpRendD3D, "gcpRendD3D is NULL");
        CCryDXGLDevice* pDXGLDevice(CCryDXGLDevice::FromInterface(static_cast<ID3D11Device*>(&gcpRendD3D->GetDevice())));
        AZ_Assert(pDXGLDevice, "CCryDXGLDevice is NULL");
        return pDXGLDevice->GetGLDevice();
    }

    bool SupportsTextureViews()
    {
        return GetGLDevice()->IsFeatureSupported(NCryOpenGL::eF_TextureViews);
    }

    bool SupportsStencilTextures()
    {
        return GetGLDevice()->IsFeatureSupported(NCryOpenGL::eF_StencilTextures);
    }

    bool SupportsDepthClipping()
    {
        return GetGLDevice()->IsFeatureSupported(NCryOpenGL::eF_DepthClipping);
    }

    bool SupportsDualSourceBlending()
    {
        return GetGLDevice()->IsFeatureSupported(NCryOpenGL::eF_DualSourceBlending);
    }

#if defined(OPENGL_ES)
    int GetAvailableMRTbpp()
    {
        if (GLAD_GL_EXT_shader_pixel_local_storage)
        {
            GLint availableTiledMem;
            glGetIntegerv(GL_MAX_SHADER_PIXEL_LOCAL_STORAGE_FAST_SIZE_EXT, &availableTiledMem);
            return availableTiledMem * 8;
        }

        return 128;
    }

    bool Supports128bppGmemPath()
    {
        return (SupportsFrameBufferFetches() || SupportsPLSExtension()) && GetAvailableMRTbpp() >= 128;
    }

    bool Supports256bppGmemPath()
    {
        return (SupportsFrameBufferFetches() || SupportsPLSExtension()) && GetAvailableMRTbpp() >= 256;
    }

    bool SupportsHalfFloatRendering()
    {
        return GLAD_GL_EXT_color_buffer_half_float;
    }

    bool SupportsPLSExtension()
    {
        return GLAD_GL_EXT_shader_pixel_local_storage;
    }

    bool SupportsFrameBufferFetches()
    {
        return GLAD_GL_EXT_shader_framebuffer_fetch;
    }
#else
    bool SupportsPLSExtension()
    {
        return false;
    }

    bool SupportsFrameBufferFetches()
    {
        return false;
    }
#endif

    uint32 GetDeviceGLVersion()
    {
        return GetGLDevice()->GetFeatureSpec().m_kVersion.ToUint();
    }
}