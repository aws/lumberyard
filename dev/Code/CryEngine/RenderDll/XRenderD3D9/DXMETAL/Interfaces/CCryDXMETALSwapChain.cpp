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

// Description : Definition of the DXGL wrapper for IDXGISwapChain

#include "StdAfx.h"
#include "CCryDXMETALDevice.hpp"
#include "CCryDXMETALGIOutput.hpp"
#include "CCryDXMETALSwapChain.hpp"
#include "CCryDXMETALTexture2D.hpp"
#include "../Implementation/METALDevice.hpp"
#include "../Implementation/METALContext.hpp"
#include "../Implementation/GLResource.hpp"


#if defined(AZ_PLATFORM_APPLE_OSX)
#include <AppKit/AppKit.h>
#else
#include <UIKit/UIKit.h>
#endif

#include <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
//  Confetti END: Igor Lobanchikov

CCryDXGLSwapChain::CCryDXGLSwapChain(CCryDXGLDevice* pDevice, const DXGI_SWAP_CHAIN_DESC& kDesc)
    : m_spDevice(pDevice)
    //  Confetti BEGIN: Igor Lobanchikov
    , m_pAutoreleasePool(0)
    //  Confetti End: Igor Lobanchikov
{
    DXGL_INITIALIZE_INTERFACE(DXGIDeviceSubObject)
    DXGL_INITIALIZE_INTERFACE(DXGISwapChain)

    m_kDesc = kDesc;
}

CCryDXGLSwapChain::~CCryDXGLSwapChain()
{
}

bool CCryDXGLSwapChain::Initialize()
{
    //  Confetti BEGIN: Igor Lobanchikov
    //  Igor: need to do this now since initializetion might not happen in the autorelease pool area and
    //  drawable and the texture won't be ever free!
    @autoreleasepool
    {
        
        NCryMetal::CDevice* pDevice(m_spDevice->GetGLDevice());
#if defined(AZ_PLATFORM_APPLE_OSX)
        CAMetalLayer* metalLayer = (static_cast<MetalView*>(pDevice->GetCurrentView())).metalLayer;
        m_Drawable = [metalLayer nextDrawable];
#else
        m_Drawable = [static_cast<CAMetalLayer*>(pDevice->GetCurrentView().layer)nextDrawable];
#endif
        
        [m_Drawable retain];
    }
    //  Confetti End: Igor Lobanchikov

    return UpdateTexture(true);
}

bool CCryDXGLSwapChain::UpdateTexture(bool bSetPixelFormat)
{
    //  Confetti BEGIN: Igor Lobanchikov
    //  Igor: Propagate actual texture resolution back from the RT to swap chan
    //  Check ho GL ES 3.0 does this
    if (m_Drawable)
    {
        m_kDesc.BufferDesc.Width = min(m_kDesc.BufferDesc.Width, (UINT)m_Drawable.texture.width);
        m_kDesc.BufferDesc.Height = min(m_kDesc.BufferDesc.Height, (UINT)m_Drawable.texture.height);
    }
    //  Confetti End: Igor Lobanchikov

    // Create a dummy texture that represents the default back buffer
    D3D11_TEXTURE2D_DESC kBackBufferDesc;
    kBackBufferDesc.Width = m_kDesc.BufferDesc.Width;
    kBackBufferDesc.Height = m_kDesc.BufferDesc.Height;
    kBackBufferDesc.MipLevels = 1;
    kBackBufferDesc.ArraySize = 1;
    kBackBufferDesc.Format = m_kDesc.BufferDesc.Format;
    kBackBufferDesc.SampleDesc = m_kDesc.SampleDesc;
    kBackBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    kBackBufferDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    kBackBufferDesc.CPUAccessFlags = 0;
    kBackBufferDesc.MiscFlags = 0;

    //  Confetti BEGIN: Igor Lobanchikov
    NCryMetal::SDefaultFrameBufferTexturePtr spBackBufferTex(NCryMetal::CreateBackBufferTexture(kBackBufferDesc));
    m_spBackBufferTexture = new CCryDXGLTexture2D(kBackBufferDesc, spBackBufferTex, m_spDevice);
    m_spBackBufferTexture->GetGLTexture()->m_bBackBuffer = true;

    if (m_Drawable)
    {
        m_spBackBufferTexture->GetGLTexture()->m_Texture = m_Drawable.texture;
    }
    else
    {
        m_spBackBufferTexture->GetGLTexture()->m_Texture = nil;
    }
    //  Confetti End: Igor Lobanchikov


    CRY_ASSERT(m_Drawable);

    //  Igor: this code is left here as a note about possible optimization.
    //  At the moment it is not clear if keeping this code is optimization
    //  or we will still need to copy data around at the different place at the
    //  same or higher cost.
    /*
    if ( m_Drawable &&
        ((m_kDesc.BufferDesc.Width == m_Drawable.texture.width) &&
        (m_kDesc.BufferDesc.Height == m_Drawable.texture.height)))
        m_spExposedBackBufferTexture = m_spBackBufferTexture;
    else
     */
    {
        NCryMetal::STexturePtr spGLTexture(NCryMetal::CreateTexture2D(kBackBufferDesc, NULL, m_spDevice->GetGLDevice()));
        m_spExposedBackBufferTexture = new CCryDXGLTexture2D(kBackBufferDesc, spGLTexture, m_spDevice);
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////
// IDXGISwapChain implementation
////////////////////////////////////////////////////////////////////////////////

HRESULT CCryDXGLSwapChain::Present(UINT SyncInterval, UINT Flags)
{
    //  Confetti BEGIN: Igor Lobanchikov
    NCryMetal::CDevice* pDevice(m_spDevice->GetGLDevice());

    if (!m_Drawable)
    {
        
#if defined(AZ_PLATFORM_APPLE_OSX)
        CAMetalLayer* metalLayer = (static_cast<MetalView*>(pDevice->GetCurrentView())).metalLayer;
        m_Drawable = [metalLayer nextDrawable];
#else
        m_Drawable = [static_cast<CAMetalLayer*>(pDevice->GetCurrentView().layer)nextDrawable];
#endif
        
        if (m_Drawable)
        {
            m_spBackBufferTexture->GetGLTexture()->m_Texture = m_Drawable.texture;
            [m_Drawable retain];
        }
        else
        {
            m_spBackBufferTexture->GetGLTexture()->m_Texture = nil;
        }
    }

    // Igor: this assert is kept here as a reminder that m_Drawable can be NULL
    //CRY_ASSERT(m_Drawable);
    {
        {
            ID3D11DeviceContext* pContext;
            m_spDevice->GetImmediateContext(&pContext);

            //  This forces clear if someone cleared RT but haven't not rendered anything before present.
            CCryDXGLDeviceContext::FromInterface(pContext)->GetGLContext()->FlushFrameBufferState();

            //  Igor: this essentially upscales virtual back buffer to the actual one.
            if (m_Drawable && m_spExposedBackBufferTexture != m_spBackBufferTexture)
            {
                NCryMetal::CContext::CopyFilterType filterType = NCryMetal::CContext::POINT;
                if (1 == CRenderer::CV_r_UpscalingQuality)
                {
                    filterType = NCryMetal::CContext::BILINEAR;
                }
                else if (2 == CRenderer::CV_r_UpscalingQuality)
                {
                    filterType = NCryMetal::CContext::BICUBIC;
                }
                else if (3 == CRenderer::CV_r_UpscalingQuality)
                {
                    filterType = NCryMetal::CContext::LANCZOS;
                }

                bool bRes = CCryDXGLDeviceContext::FromInterface(pContext)->GetGLContext()->
                        TrySlowCopySubresource(m_spBackBufferTexture->GetGLTexture(), 0, 0, 0, 0,
                        m_spExposedBackBufferTexture->GetGLTexture(), 0, 0, filterType);

                //  Make sure copy actually happens.
                CRY_ASSERT(bRes);
            }

            //  This commits command buffer.
            CCryDXGLDeviceContext::FromInterface(pContext)->GetGLContext()->Flush(true);

            pContext->Release();
        }

        if (m_Drawable)
        {
            [m_Drawable present];
            [m_Drawable release];
            m_Drawable = nil;
        }
    }

    //  Igor: leave a hint for the graphics debugger.
    id<MTLCommandQueue> mtlCommandQueue = pDevice->GetMetalCommandQueue();
    [mtlCommandQueue insertDebugCaptureBoundary];

    {
        ID3D11DeviceContext* pContext;
        m_spDevice->GetImmediateContext(&pContext);

        //  Create a new command buffer here. Can't do this on flush because need to do present, then insertDebugCaptureBoundary first.
        //  Although it is perfectly ok to flush then create a new command buffer, then do present and mark the end of frame,
        //  XCode frame capture won't work at all in this case.
        CCryDXGLDeviceContext::FromInterface(pContext)->GetGLContext()->InitMetalFrameResources();

        pContext->Release();
    }

    FlushAutoreleasePool();

    return pDevice->Present();
    //  Confetti End: Igor Lobanchikov
}

HRESULT CCryDXGLSwapChain::GetBuffer(UINT Buffer, REFIID riid, void** ppSurface)
{
    if (Buffer == 0 && riid == __uuidof(ID3D11Texture2D))
    {
        m_spExposedBackBufferTexture->AddRef();
        CCryDXGLTexture2D::ToInterface(reinterpret_cast<ID3D11Texture2D**>(ppSurface), m_spExposedBackBufferTexture.get());
        return S_OK;
    }
    DXGL_TODO("Support more than one swap chain buffer if required");
    return E_FAIL;
}

HRESULT CCryDXGLSwapChain::SetFullscreenState(BOOL Fullscreen, IDXGIOutput* pTarget)
{
    DXGL_NOT_IMPLEMENTED;
    return E_FAIL;
}

HRESULT CCryDXGLSwapChain::GetFullscreenState(BOOL* pFullscreen, IDXGIOutput** ppTarget)
{
    DXGL_NOT_IMPLEMENTED;
    return E_FAIL;
}

HRESULT CCryDXGLSwapChain::GetDesc(DXGI_SWAP_CHAIN_DESC* pDesc)
{
    (*pDesc) = m_kDesc;
    return S_OK;
}

HRESULT CCryDXGLSwapChain::ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags)
{
    if (
        Format         == m_kDesc.BufferDesc.Format &&
        Width          == m_kDesc.BufferDesc.Width  &&
        Height         == m_kDesc.BufferDesc.Height &&
        BufferCount    == m_kDesc.BufferCount       &&
        SwapChainFlags == m_kDesc.Flags)
    {
        return S_OK; // Nothing to do
    }
    if (BufferCount == m_kDesc.BufferCount)
    {
        m_kDesc.BufferDesc.Format = Format;
        m_kDesc.BufferDesc.Width  = Width;
        m_kDesc.BufferDesc.Height = Height;
        m_kDesc.Flags             = SwapChainFlags;

        if (UpdateTexture(false))
        {
            return S_OK;
        }
    }

    return E_FAIL;
}

HRESULT CCryDXGLSwapChain::ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters)
{
    DXGL_NOT_IMPLEMENTED;
    return E_FAIL;
}

HRESULT CCryDXGLSwapChain::GetContainingOutput(IDXGIOutput** ppOutput)
{
    DXGL_NOT_IMPLEMENTED;
    return E_FAIL;
}

HRESULT CCryDXGLSwapChain::GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats)
{
    DXGL_NOT_IMPLEMENTED;
    return E_FAIL;
}

HRESULT CCryDXGLSwapChain::GetLastPresentCount(UINT* pLastPresentCount)
{
    DXGL_NOT_IMPLEMENTED;
    return E_FAIL;
}

//  Confetti BEGIN: Igor Lobanchikov
void CCryDXGLSwapChain::FlushAutoreleasePool()
{
    CRY_ASSERT(m_pAutoreleasePool);

    [(NSAutoreleasePool*)m_pAutoreleasePool release];
    m_pAutoreleasePool = 0;

    TryCreateAutoreleasePool();
}

void CCryDXGLSwapChain::TryCreateAutoreleasePool()
{
    if (!m_pAutoreleasePool)
    {
        m_pAutoreleasePool = [[NSAutoreleasePool alloc] init];
    }
}
//  Confetti End: Igor Lobanchikov
