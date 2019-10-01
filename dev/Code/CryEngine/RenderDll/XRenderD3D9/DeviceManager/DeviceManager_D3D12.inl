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

#ifndef DEVMAN_USE_STAGING_POOL
#error StagingPool is a requirement for DX12
#endif

//=============================================================================

#ifdef DEVMAN_USE_STAGING_POOL
D3DResource* CDeviceManager::AllocateStagingResource(D3DResource* pForTex, bool bUpload)
{
    D3DResource* pStagingResource = nullptr;

    if (S_OK == gcpRendD3D->GetDevice().CreateStagingResource(pForTex, &pStagingResource, bUpload))
    {
        pStagingResource;
    }

    return pStagingResource;
}

void CDeviceManager::ReleaseStagingResource(D3DResource* pStagingTex)
{
    if (pStagingTex && gcpRendD3D->GetDevice().ReleaseStagingResource(pStagingTex))
    {
        pStagingTex = nullptr;
    }
}
#endif

//=============================================================================

int32 CDeviceTexture::Release()
{
    int32 nRef = Cleanup();

    if (nRef <= 0 && !m_bNoDelete)
    {
        delete this;
    }

    return nRef;
}

void CDeviceTexture::Unbind()
{
    for (uint32 i = 0; i < MAX_TMU; i++)
    {
        if (CTexture::s_TexStages[i].m_DevTexture == this)
        {
            CTexture::s_TexStages[i].m_DevTexture = NULL;

            ID3D11ShaderResourceView* RV = NULL;
            gcpRendD3D->GetDeviceContext().PSSetShaderResources(i, 1, &RV);
        }
    }
}

void CDeviceTexture::DownloadToStagingResource(uint32 nSubRes, StagingHook cbTransfer)
{
    D3D11_RESOURCE_DIMENSION eResourceDimension;
    m_pD3DTexture->GetType(&eResourceDimension);

    D3DResource* pStagingResource;
    void* pStagingMemory = m_pStagingMemory[0];
    if (!(pStagingResource = m_pStagingResource[0]))
    {
        pStagingResource = gcpRendD3D->m_DevMan.AllocateStagingResource(m_pD3DTexture, FALSE);
    }

    assert(pStagingResource);

    if (gcpRendD3D->GetDeviceContext().CopyStagingResource(pStagingResource, m_pD3DTexture, nSubRes, FALSE) == S_OK)
    {
        // Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map.
        // Map and Unmap must be called between CPU and GPU accesses to the same memory
        // address on some system architectures, when the page caching behavior is write-back.
        // Map and Unmap invalidate and flush the last level CPU cache on some ARM systems,
        // to marshal data between the CPU and GPU through memory addresses with write-back behavior.
        gcpRendD3D->GetDeviceContext().WaitStagingResource(pStagingResource);
        gcpRendD3D->GetDeviceContext().MapStagingResource(pStagingResource, FALSE, &pStagingMemory);

        cbTransfer(pStagingMemory, 0, 0);

        gcpRendD3D->GetDeviceContext().UnmapStagingResource(pStagingResource, FALSE);
    }

    if (!(m_pStagingResource[0]))
    {
        gcpRendD3D->m_DevMan.ReleaseStagingResource(pStagingResource);
    }
}

void CDeviceTexture::DownloadToStagingResource(uint32 nSubRes)
{
    assert(m_pStagingResource[0]);

    gcpRendD3D->GetDeviceContext().CopyStagingResource(m_pStagingResource[0], m_pD3DTexture, nSubRes, FALSE);
}

void CDeviceTexture::UploadFromStagingResource(uint32 nSubRes, StagingHook cbTransfer)
{
    D3D11_RESOURCE_DIMENSION eResourceDimension;
    m_pD3DTexture->GetType(&eResourceDimension);

    D3DResource* pStagingResource;
    void* pStagingMemory = m_pStagingMemory[1];
    if (!(pStagingResource = m_pStagingResource[1]))
    {
        pStagingResource = gcpRendD3D->m_DevMan.AllocateStagingResource(m_pD3DTexture, TRUE);
    }

    assert(pStagingResource);

    // The first call to Map allocates a CPU virtual address range for the resource.
    // The last call to Unmap deallocates the CPU virtual address range.
    // Applications cannot rely on the address being consistent, unless Map is persistently nested.
    gcpRendD3D->GetDeviceContext().WaitStagingResource(pStagingResource);
    gcpRendD3D->GetDeviceContext().MapStagingResource(pStagingResource, TRUE, &pStagingMemory);

    if (cbTransfer(pStagingMemory, 0, 0))
    {
        gcpRendD3D->GetDeviceContext().CopyStagingResource(pStagingResource, m_pD3DTexture, nSubRes, TRUE);
    }

    // Unmap also flushes the CPU cache, when necessary, so that GPU reads to this
    // address reflect any modifications made by the CPU.
    gcpRendD3D->GetDeviceContext().UnmapStagingResource(pStagingResource, TRUE);

    if (!(m_pStagingResource[1]))
    {
        gcpRendD3D->m_DevMan.ReleaseStagingResource(pStagingResource);
    }
}

void CDeviceTexture::UploadFromStagingResource(uint32 nSubRes)
{
    assert(m_pStagingResource[1]);

    gcpRendD3D->GetDeviceContext().CopyStagingResource(m_pStagingResource[1], m_pD3DTexture, nSubRes, TRUE);
}

void CDeviceTexture::AccessCurrStagingResource(uint32 nSubRes, bool forUpload, StagingHook cbTransfer)
{
    // Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map.
    // Applications cannot rely on the address being consistent, unless Map is persistently nested.
    gcpRendD3D->GetDeviceContext().WaitStagingResource(m_pStagingResource[forUpload]);
    gcpRendD3D->GetDeviceContext().MapStagingResource(m_pStagingResource[forUpload], forUpload, &m_pStagingMemory[forUpload]);

    cbTransfer(m_pStagingMemory[forUpload], 0, 0);

    gcpRendD3D->GetDeviceContext().UnmapStagingResource(m_pStagingResource[forUpload], forUpload);
}

//=============================================================================

HRESULT CDeviceManager::Create2DTexture(const string& textureName, uint32 nWidth, uint32 nHeight, uint32 nMips, uint32 nArraySize, uint32 nUsage, const ColorF& cClearValue, D3DFormat Format, D3DPOOL Pool,
    LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI, bool bShouldBeCreated, int32 nESRAMOffset)
{
    HRESULT hr = S_OK;

    CDeviceTexture* pDeviceTexture = 0;//*/ new CDeviceTexture();
    D3DTexture* pD3DTex = NULL;

    uint32 nBindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (nUsage & USAGE_DEPTH_STENCIL)
    {
        nBindFlags |= D3D11_BIND_DEPTH_STENCIL;
    }
    else if (nUsage & USAGE_RENDER_TARGET)
    {
        nBindFlags |= D3D11_BIND_RENDER_TARGET;
    }
    if (nUsage & USAGE_UNORDERED_ACCESS)
    {
        nBindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    }
    uint32 nMiscFlags = 0;
    if (nUsage & USAGE_AUTOGENMIPS)
    {
        nMiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }

    D3D11_TEXTURE2D_DESC Desc;
    ZeroStruct(Desc);
    Desc.Width = nWidth;
    Desc.Height = nHeight;
    Desc.MipLevels = nMips;
    Desc.Format = Format;
    Desc.ArraySize = nArraySize;
    Desc.BindFlags = nBindFlags;
    Desc.CPUAccessFlags = (nUsage & USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;

    Desc.Usage = (nUsage & USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    Desc.Usage = (nUsage & USAGE_STAGING) ? D3D11_USAGE_STAGING : Desc.Usage;

    Desc.SampleDesc.Count = pTI ? pTI->m_nMSAASamples : 1;
    Desc.SampleDesc.Quality = pTI ? pTI->m_nMSAAQuality : 0;
    // AntonK: supported only by DX11 feature set
    // needs to be enabled for pure DX11 context
    //if(nUsage & USAGE_STREAMING)
    //nMiscFlags |= D3D11_RESOURCE_MISC_RESOURCE_CLAMP;
    //if (nUsage & USAGE_STREAMING)
    //    Desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
    Desc.MiscFlags = nMiscFlags;

    D3D11_SUBRESOURCE_DATA* pSRD = NULL;
    D3D11_SUBRESOURCE_DATA SRD[20];
    if (pTI && pTI->m_pData)
    {
        pSRD = &SRD[0];
        for (int i = 0; i < nMips; i++)
        {
            SRD[i].pSysMem = pTI->m_pData[i].pSysMem;
            SRD[i].SysMemPitch = pTI->m_pData[i].SysMemPitch;
            SRD[i].SysMemSlicePitch = pTI->m_pData[i].SysMemSlicePitch;
        }
    }

    if (nUsage & (USAGE_DEPTH_STENCIL | USAGE_RENDER_TARGET | USAGE_UNORDERED_ACCESS))
    {
        hr = gcpRendD3D->GetDevice().CreateTexture2D(&Desc, (const FLOAT*)&cClearValue, pSRD, &pD3DTex);
    }
    else
    {
        hr = gcpRendD3D->GetDevice().CreateTexture2D(&Desc, pSRD, &pD3DTex);
    }

    if (SUCCEEDED(hr) && pDeviceTexture == 0)
    {
        pDeviceTexture = new CDeviceTexture();
    }

    if (SUCCEEDED(hr) && pD3DTex)
    {
        pDeviceTexture->m_pD3DTexture = pD3DTex;
        if (!pDeviceTexture->m_nBaseAllocatedSize)
        {
            pDeviceTexture->m_nBaseAllocatedSize = CDeviceTexture::TextureDataSize(nWidth, nHeight, 1, nMips, 1, CTexture::TexFormatFromDeviceFormat(Format));

            // Register the VRAM allocation with the driller
            void* address = static_cast<void*>(pD3DTex);
            size_t byteSize = pDeviceTexture->m_nBaseAllocatedSize;
            EBUS_EVENT(Render::Debug::VRAMDrillerBus, RegisterAllocation, address, byteSize, textureName, Render::Debug::VRAM_CATEGORY_TEXTURE, Render::Debug::VRAM_SUBCATEGORY_TEXTURE_TEXTURE);
        }

        if (nUsage & USAGE_STAGE_ACCESS)
        {
            if (nUsage & USAGE_CPU_READ)
            {
                // Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map.
                pDeviceTexture->m_pStagingResource[0] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, FALSE);
            }
            if (nUsage & USAGE_CPU_WRITE)
            {
                // Applications cannot rely on the address being consistent, unless Map is persistently nested.
                pDeviceTexture->m_pStagingResource[1] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, TRUE);
                gcpRendD3D->GetDeviceContext().MapStagingResource(pDeviceTexture->m_pStagingResource[1], TRUE, &pDeviceTexture->m_pStagingMemory[1]);
            }
        }
    }
    else
    {
        SAFE_DELETE(pDeviceTexture);
    }

    *ppDevTexture = pDeviceTexture;

    return hr;
}

HRESULT CDeviceManager::CreateCubeTexture(const string& textureName, uint32 nSize, uint32 nMips, uint32 nArraySize, uint32 nUsage, const ColorF& cClearValue, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI, bool bShouldBeCreated)
{
    HRESULT hr = S_OK;

    CDeviceTexture* pDeviceTexture = 0;//new CDeviceTexture();
    D3DCubeTexture* pD3DTex = NULL;

    uint32 nBindFlags = D3D11_BIND_SHADER_RESOURCE;
    uint32 nMiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    if (nUsage & USAGE_DEPTH_STENCIL)
    {
        nBindFlags |= D3D11_BIND_DEPTH_STENCIL;
    }
    else if (nUsage & USAGE_RENDER_TARGET)
    {
        nBindFlags |= D3D11_BIND_RENDER_TARGET;
    }
    if (nUsage & USAGE_AUTOGENMIPS)
    {
        nMiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }
    D3D11_TEXTURE2D_DESC Desc;
    ZeroStruct(Desc);
    Desc.Width = nSize;
    Desc.Height = nSize;
    Desc.MipLevels = nMips;
    Desc.Format = Format;
    Desc.ArraySize = nArraySize * 6;
    Desc.BindFlags |= nBindFlags;
    Desc.CPUAccessFlags = (nUsage & USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;
    Desc.Usage = (nUsage & USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;
    Desc.MiscFlags = nMiscFlags;
    // AntonK: supported only by DX11 feature set
    // needs to be enabled for pure DX11 context
    //if(nUsage & USAGE_STREAMING)
    //Desc.MiscFlags |= D3D11_RESOURCE_MISC_RESOURCE_CLAMP;

    D3D11_SUBRESOURCE_DATA* pSRD = NULL;
    D3D11_SUBRESOURCE_DATA SRD[g_nD3D10MaxSupportedSubres];
    if (pTI && pTI->m_pData)
    {
        pSRD = &SRD[0];
        for (int j = 0; j < 6; j++)
        {
            for (int i = 0; i < nMips; i++)
            {
                int nSubresInd = j * nMips + i;
                SRD[nSubresInd].pSysMem = pTI->m_pData[nSubresInd].pSysMem;
                SRD[nSubresInd].SysMemPitch = pTI->m_pData[nSubresInd].SysMemPitch;
                SRD[nSubresInd].SysMemSlicePitch = pTI->m_pData[nSubresInd].SysMemSlicePitch;
            }
        }
    }

    if (nUsage & (USAGE_DEPTH_STENCIL | USAGE_RENDER_TARGET | USAGE_UNORDERED_ACCESS))
    {
        hr = gcpRendD3D->GetDevice().CreateTexture2D(&Desc, (const FLOAT*)&cClearValue, pSRD, &pD3DTex);
    }
    else
    {
        hr = gcpRendD3D->GetDevice().CreateTexture2D(&Desc, pSRD, &pD3DTex);
    }

    if (SUCCEEDED(hr) && pDeviceTexture == 0)
    {
        pDeviceTexture = new CDeviceTexture();
    }

    if (SUCCEEDED(hr) && pD3DTex)
    {
        pDeviceTexture->m_pD3DTexture = pD3DTex;
        pDeviceTexture->m_bCube = true;
        if (!pDeviceTexture->m_nBaseAllocatedSize)
        {
            pDeviceTexture->m_nBaseAllocatedSize = CDeviceTexture::TextureDataSize(nSize, nSize, 1, nMips, 1, CTexture::TexFormatFromDeviceFormat(Format)) * 6;

            // Register the VRAM allocation with the driller
            void* address = static_cast<void*>(pD3DTex);
            size_t byteSize = pDeviceTexture->m_nBaseAllocatedSize;
            EBUS_EVENT(Render::Debug::VRAMDrillerBus, RegisterAllocation, address, byteSize, textureName, Render::Debug::VRAM_CATEGORY_TEXTURE, Render::Debug::VRAM_SUBCATEGORY_TEXTURE_TEXTURE);
        }

        if (nUsage & USAGE_STAGE_ACCESS)
        {
            if (nUsage & USAGE_CPU_READ)
            {
                // Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map.
                pDeviceTexture->m_pStagingResource[0] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, FALSE);
            }
            if (nUsage & USAGE_CPU_WRITE)
            {
                // Applications cannot rely on the address being consistent, unless Map is persistently nested.
                pDeviceTexture->m_pStagingResource[1] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, TRUE);
                gcpRendD3D->GetDeviceContext().MapStagingResource(pDeviceTexture->m_pStagingResource[1], TRUE, &pDeviceTexture->m_pStagingMemory[1]);
            }
        }
    }
    else
    {
        SAFE_DELETE(pDeviceTexture);
    }

    *ppDevTexture = pDeviceTexture;
    return hr;
}

HRESULT CDeviceManager::CreateVolumeTexture(const string& textureName, uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nUsage, const ColorF& cClearValue, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI)
{
    HRESULT hr = S_OK;
    CDeviceTexture* pDeviceTexture = new CDeviceTexture();
    D3DVolumeTexture* pD3DTex = NULL;
    uint32 nBindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (nUsage & USAGE_DEPTH_STENCIL)
    {
        nBindFlags |= D3D11_BIND_DEPTH_STENCIL;
    }
    else if (nUsage & USAGE_RENDER_TARGET)
    {
        nBindFlags |= D3D11_BIND_RENDER_TARGET;
    }
    if (nUsage & USAGE_UNORDERED_ACCESS)
    {
        nBindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    }
    D3D11_TEXTURE3D_DESC Desc;
    ZeroStruct(Desc);
    Desc.Width = nWidth;
    Desc.Height = nHeight;
    Desc.Depth = nDepth;
    Desc.MipLevels = nMips;
    Desc.Format = (nUsage & USAGE_UAV_RWTEXTURE) ? CTexture::ConvertToTypelessFmt(Format) : Format;
    Desc.BindFlags = nBindFlags;
    Desc.CPUAccessFlags = 0;

    Desc.Usage = (nUsage & USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    Desc.Usage = (nUsage & USAGE_STAGING) ? D3D11_USAGE_STAGING : Desc.Usage;

    Desc.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA* pSRD = NULL;
    D3D11_SUBRESOURCE_DATA SRD[20] = { D3D11_SUBRESOURCE_DATA() };
    if (pTI && pTI->m_pData)
    {
        pSRD = &SRD[0];
        for (int i = 0; i < nMips; i++)
        {
            SRD[i].pSysMem = pTI->m_pData[i].pSysMem;
            SRD[i].SysMemPitch = pTI->m_pData[i].SysMemPitch;
            SRD[i].SysMemSlicePitch = pTI->m_pData[i].SysMemSlicePitch;
        }
    }

    if (nUsage & (USAGE_DEPTH_STENCIL | USAGE_RENDER_TARGET | USAGE_UNORDERED_ACCESS))
    {
        hr = gcpRendD3D->GetDevice().CreateTexture3D(&Desc, (const FLOAT*)&cClearValue, pSRD, &pD3DTex);
    }
    else
    {
        hr = gcpRendD3D->GetDevice().CreateTexture3D(&Desc, pSRD, &pD3DTex);
    }

    if (SUCCEEDED(hr) && pD3DTex)
    {
        pDeviceTexture->m_pD3DTexture = pD3DTex;

        if (!pDeviceTexture->m_nBaseAllocatedSize)
        {
            pDeviceTexture->m_nBaseAllocatedSize = CDeviceTexture::TextureDataSize(nWidth, nHeight, nDepth, nMips, 1, CTexture::TexFormatFromDeviceFormat(Format));

            // Register the VRAM allocation with the driller
            void* address = static_cast<void*>(pD3DTex);
            size_t byteSize = pDeviceTexture->m_nBaseAllocatedSize;
            EBUS_EVENT(Render::Debug::VRAMDrillerBus, RegisterAllocation, address, byteSize, textureName, Render::Debug::VRAM_CATEGORY_TEXTURE, Render::Debug::VRAM_SUBCATEGORY_TEXTURE_TEXTURE);
        }

        if (nUsage & USAGE_STAGE_ACCESS)
        {
            if (nUsage & USAGE_CPU_READ)
            {
                // Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map.
                pDeviceTexture->m_pStagingResource[0] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, FALSE);
            }
            if (nUsage & USAGE_CPU_WRITE)
            {
                // Applications cannot rely on the address being consistent, unless Map is persistently nested.
                pDeviceTexture->m_pStagingResource[1] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, TRUE);
                gcpRendD3D->GetDeviceContext().MapStagingResource(pDeviceTexture->m_pStagingResource[1], TRUE, &pDeviceTexture->m_pStagingMemory[1]);
            }
        }
    }
    else
    {
        SAFE_DELETE(pDeviceTexture);
    }

    *ppDevTexture = pDeviceTexture;
    return hr;
}

HRESULT CDeviceManager::CreateBuffer(
    uint32 nSize
    , uint32 elemSize
    , int32 nUsage
    , int32 nBindFlags
    , D3DBuffer** ppBuff)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
    HRESULT hr = S_OK;

# ifndef _RELEASE
    // ToDo verify that the usage and bindflags are correctly set (e.g certain
    // bit groups are mutually exclusive).
# endif

    D3D11_BUFFER_DESC BufDesc;
    ZeroStruct(BufDesc);

    BufDesc.ByteWidth = nSize * elemSize;
    int nD3DUsage = D3D11_USAGE_DEFAULT;
    nD3DUsage = (nUsage & USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : nD3DUsage;
    nD3DUsage = (nUsage & USAGE_IMMUTABLE) ? D3D11_USAGE_IMMUTABLE : nD3DUsage;
    nD3DUsage = (nUsage & USAGE_STAGING) ? D3D11_USAGE_STAGING : nD3DUsage;
    BufDesc.Usage = (D3D11_USAGE)nD3DUsage;

    if (BufDesc.Usage != D3D11_USAGE_STAGING)
    {
        switch (nBindFlags)
        {
        case BIND_VERTEX_BUFFER:
            BufDesc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
            break;
        case BIND_INDEX_BUFFER:
            BufDesc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
            break;
        case BIND_CONSTANT_BUFFER:
            BufDesc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
            break;

        case BIND_SHADER_RESOURCE:
        case BIND_UNORDERED_ACCESS:
            if (nBindFlags & BIND_SHADER_RESOURCE)
            {
                BufDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
            }
            if (nBindFlags & BIND_UNORDERED_ACCESS)
            {
                BufDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
            }
            break;

        case BIND_STREAM_OUTPUT:
        case BIND_RENDER_TARGET:
        case BIND_DEPTH_STENCIL:
            CryFatalError("trying to create (currently) unsupported buffer type");
            break;

        default:
            CryFatalError("trying to create unknown buffer type");
            break;
        }
    }
    else
    {
        BufDesc.BindFlags = 0;
    }

    BufDesc.MiscFlags = 0;
    BufDesc.CPUAccessFlags = 0;
    if (BufDesc.Usage != D3D11_USAGE_DEFAULT && BufDesc.Usage != D3D11_USAGE_IMMUTABLE)
    {
        BufDesc.CPUAccessFlags = (nUsage & USAGE_CPU_WRITE) ? D3D11_CPU_ACCESS_WRITE : 0;
        BufDesc.CPUAccessFlags |= (nUsage & USAGE_CPU_READ) ? D3D11_CPU_ACCESS_READ : 0;
    }

    if (nBindFlags & BIND_UNORDERED_ACCESS)
    {
        BufDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    }

    hr = gcpRendD3D->GetDevice().CreateBuffer(&BufDesc, NULL, (ID3D11Buffer**)ppBuff);
    static_cast<CCryDX12Buffer*>(*ppBuff)->GetD3D12Resource()->SetName(L"DevBuffer");
    CHECK_HRESULT(hr);
    return hr;
}

void CDeviceManager::ExtractBasePointer(D3DBuffer* buffer, uint8*& base_ptr)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = gcpRendD3D->GetDeviceContext().Map(buffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource);
    CHECK_HRESULT(hr);
    base_ptr = (uint8*)mappedResource.pData;
}
