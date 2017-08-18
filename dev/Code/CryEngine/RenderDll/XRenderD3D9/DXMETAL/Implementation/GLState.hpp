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

// Description: Declares the state types and the functions to modify them
//              and apply to a device

#ifndef __GLSTATE__
#define __GLSTATE__

#include "GLCommon.hpp"

//  Confetti BEGIN: Igor Lobanchikov
#import <Metal/MTLRenderPipeline.h>
#import <Metal/MTLRenderCommandEncoder.h>
//  Confetti END: Igor Lobanchikov

namespace NCryMetal
{
    class CDevice;
    class CContext;

    //  Confetti BEGIN: Igor Lobanchikov
    struct SMetalBlendState
    {
        MTLColorWriteMask   writeMask;
        bool                blendingEnabled;
        MTLBlendOperation   alphaBlendOperation;
        MTLBlendOperation   rgbBlendOperation;
        MTLBlendFactor      destinationAlphaBlendFactor;
        MTLBlendFactor      destinationRGBBlendFactor;
        MTLBlendFactor      sourceAlphaBlendFactor;
        MTLBlendFactor      sourceRGBBlendFactor;

        void ResetToDefault()
        {
            writeMask = MTLColorWriteMaskAll;
            blendingEnabled = false;
            alphaBlendOperation = rgbBlendOperation = MTLBlendOperationAdd;
            destinationAlphaBlendFactor = destinationRGBBlendFactor = MTLBlendFactorZero;
            sourceAlphaBlendFactor = sourceRGBBlendFactor = MTLBlendFactorOne;
        }
    };
    //  Confetti End: Igor Lobanchikov

    struct SBlendState
    {
        //  Confetti BEGIN: Igor Lobanchikov
        SMetalBlendState colorAttachements[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
        //  Confetti End: Igor Lobanchikov
    };

    //  Confetti BEGIN: Igor Lobanchikov
    struct SRasterizerState
    {
        MTLCullMode         cullMode;
        float               depthBias;
        float               depthSlopeScale;
        float               depthBiasClamp;
        MTLWinding          frontFaceWinding;
        MTLTriangleFillMode triangleFillMode;
        MTLDepthClipMode    depthClipMode;

        //  Igor: this setting is not supported via Metal resterizer state
        //  but rather via scissor rect property.
        bool                scissorEnable;
    };
    //  Confetti End: Igor Lobanchikov

    //  Confetti BEGIN: Igor Lobanchikov
    bool InitializeBlendState(const D3D11_BLEND_DESC& kDesc, SBlendState& kState, CDevice* pDevice);
    bool InitializeDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& kDesc, id<MTLDepthStencilState>& kState, CDevice* pDevice);
    bool InitializeRasterizerState(const D3D11_RASTERIZER_DESC& kDesc, SRasterizerState& kState, CDevice* pDevice);
    bool InitializeSamplerState(const D3D11_SAMPLER_DESC& kDesc, id<MTLSamplerState>& kState, CDevice* pDevice);
    //  Confetti End: Igor Lobanchikov
}


#endif //__GLSTATE__

