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

#ifdef LY_TERRAIN_RUNTIME

#include "VirtualTexture.h"

namespace Terrain
{
    /////////////////////////////////////////////////////////////////
    // VirtualTexture constructor/destructor

    VirtualTexture::VirtualTexture(const VirtualTextureDesc& desc)
        : m_worldOriginX(desc.m_worldOriginX)
        , m_worldOriginY(desc.m_worldOriginY)
    {
        VTWrapper::VTWrapperDesc vtWrapperDesc;
        vtWrapperDesc.m_name = desc.m_name;
        vtWrapperDesc.m_formats = desc.m_formats;
        vtWrapperDesc.m_clearColors = desc.m_clearColors;
        vtWrapperDesc.m_textureCount = desc.m_textureCount;
        vtWrapperDesc.m_texelsPerMeter = desc.m_texelsPerMeter;
        vtWrapperDesc.m_mipLevels = desc.m_mipLevels;
        vtWrapperDesc.m_tileSize = desc.m_tileSize;
        vtWrapperDesc.m_tilePadding = desc.m_tilePadding;
        vtWrapperDesc.m_physicalTextureCacheSize = desc.m_physicalTextureCacheSize;
        vtWrapperDesc.CalculateVirtualTextureSize(desc.m_worldSizeXMeters, desc.m_worldSizeYMeters);

        vtWrapperDesc.m_initialIndirectionTileCacheSize = desc.m_initialIndirectionTileCacheSize;

        // World size needs correction based on calculated virtual texture size
        m_worldSizeXMeters = static_cast<int>(vtWrapperDesc.m_virtualTextureSizeX / desc.m_texelsPerMeter);
        m_worldSizeYMeters = static_cast<int>(vtWrapperDesc.m_virtualTextureSizeY / desc.m_texelsPerMeter);

        m_vtWrapperPtr = AZStd::make_unique<VTWrapper>(vtWrapperDesc);

        // calculate the world extents of a lod 0 node
        float leafNodeSizeMeters = static_cast<float>(desc.m_tileSize) / desc.m_texelsPerMeter;
        m_tileSizeMetersPerMip[0] = leafNodeSizeMeters;

        for (int indexMip = 1; indexMip < desc.m_mipLevels; ++indexMip)
        {
            m_tileSizeMetersPerMip[indexMip] = 2.0f * m_tileSizeMetersPerMip[indexMip - 1];
        }

        m_virtualTextureChunkSize = m_vtWrapperPtr->GetVirtualTextureChunkSize();
        m_virtualTextureChunkWorldSize = m_virtualTextureChunkSize / desc.m_texelsPerMeter;
    }


    /////////////////////////////////////////////////////////////////
    // VirtualTexture Functionality

    void VirtualTexture::ClearCache()
    {
        m_vtWrapperPtr->ClearCache();
    }

    void VirtualTexture::RequestTile(float worldX, float worldY, int mipLevel)
    {
        int virtualTileX, virtualTileY;
        ConvertWorldToVirtual(worldX, worldY, mipLevel, virtualTileX, virtualTileY);

        m_vtWrapperPtr->RequestTile(virtualTileX, virtualTileY, mipLevel);
    }

    // Request all height tiles necessary for the specified world area
    void VirtualTexture::RequestTiles(float worldX, float worldY, float worldXWidth, float worldYWidth, int mipLevel)
    {
        AZ_Assert(mipLevel >= 0 && mipLevel < m_vtWrapperPtr->GetMipLevels(), "HeightMapVirtualTexture | Invalid mip level");

        float startRelativeX = worldX - m_worldOriginX;
        float startRelativeY = worldY - m_worldOriginY;

        int startVirtualTileX = static_cast<int>(floorf(startRelativeX / m_tileSizeMetersPerMip[mipLevel]));
        int startVirtualTileY = static_cast<int>(floorf(startRelativeY / m_tileSizeMetersPerMip[mipLevel]));

        float endRelativeX = startRelativeX + worldXWidth;
        float endRelativeY = startRelativeY + worldYWidth;

        int endVirtualTileX = static_cast<int>(ceilf(endRelativeX / m_tileSizeMetersPerMip[mipLevel]));
        int endVirtualTileY = static_cast<int>(ceilf(endRelativeY / m_tileSizeMetersPerMip[mipLevel]));

        for (int x = startVirtualTileX; x < endVirtualTileX; ++x)
        {
            for (int y = startVirtualTileY; y < endVirtualTileY; ++y)
            {
                m_vtWrapperPtr->RequestTile(x, y, mipLevel);
            }
        }
    }

    /////////////////////////////////////////////////////////////////
    // VirtualTexture Update pump

    void VirtualTexture::Update(int maxRequestsToProcess)
    {
        m_vtWrapperPtr->Update(maxRequestsToProcess);
    }

    /////////////////////////////////////////////////////////////////
    // VirtualTexture Rendering
    // These functions work in conjunction with VirtualTexture.cfi

    bool VirtualTexture::BeginRendering(EHWShaderClass shaderStage, int samplerStateId, const AZ::Aabb& nodeAABB)
    {
        static CCryNameR virtualTextureConsts0Name("g_virtualTextureConsts0");
        static CCryNameR virtualTextureConsts1Name("g_virtualTextureConsts1");

        if (shaderStage >= eHWSC_Geometry)
        {
            AZ_Error("VirtualTexture", false, "VirtualTexture support limited to Vertex and Pixel Shader!");
            return false;
        }

        CRenderer* d3dRenderer = gRenDev;

        // Resolve indirection map tile id
        const int mipLevelLookup = 0;   // miplevel is irrelevant, we just need a valid miplevel to retrieve the indirection tile
        int virtualTileX, virtualTileY;
        ConvertWorldToVirtual(nodeAABB.GetMin().GetX(), nodeAABB.GetMin().GetY(), mipLevelLookup, virtualTileX, virtualTileY);

        // Retrieve indirection tile
        IndirectionTileInfo indirectionTileInfo;
        m_vtWrapperPtr->GetIndirectionTile(virtualTileX, virtualTileY, mipLevelLookup, indirectionTileInfo);
        if (!indirectionTileInfo.m_indirectionTilePtr)
        {
            // Early out, indirection map is not ready
            return false;
        }

        // Bind indirection tile
        BindTexture(shaderStage, indirectionTileInfo.m_indirectionTilePtr, m_indirectionMapTextureSlot);

        // Bind physical textures
        for (int indexTexture = 0; indexTexture < m_vtWrapperPtr->GetPhysicalTextureCount(); ++indexTexture)
        {
            CTexture* pTex = m_vtWrapperPtr->GetPhysicalTexture(indexTexture);
            pTex->Apply(m_physicalTextureStartSlot + indexTexture, samplerStateId, EFTT_UNKNOWN, -1, SResourceView::DefaultView, shaderStage);
        }

        // Calculate constant data and bind
        float tileSize = static_cast<float>(m_vtWrapperPtr->GetTileSize());
        float tileSizeWithPadding = static_cast<float>(m_vtWrapperPtr->GetTileSize() + 2 * m_vtWrapperPtr->GetTilePadding());

        //float4 g_virtualTextureConsts0;
        //#define VT_INVERSE_WORLD_SIZE (g_virtualTextureConsts0.x)
        //#define VT_LEAF_NODE_UV_SIZE (g_virtualTextureConsts0.y)
        //#define VT_PTILE_UV_SIZE (g_virtualTextureConsts0.z)
        //#define VT_PTILE_WITH_PADDING_UV_SIZE (g_virtualTextureConsts0.w)
        Vec4 virtualTextureConsts0;
        virtualTextureConsts0.x = 1.0f / m_virtualTextureChunkWorldSize;
        virtualTextureConsts0.y = tileSize / m_virtualTextureChunkSize;
        virtualTextureConsts0.z = tileSize / m_vtWrapperPtr->GetPhysicalTextureCacheSize();
        virtualTextureConsts0.w = tileSizeWithPadding / m_vtWrapperPtr->GetPhysicalTextureCacheSize();

        //float4 g_virtualTextureConsts1; // .w: unused
        //#define VT_WORLD_ORIGIN (g_virtualTextureConsts1.xy)
        //#define VT_PTILE_PADDING_OFFSET (g_virtualTextureConsts1.z)
        Vec4 virtualTextureConsts1;
        // VirtualTexture chunk origin = VitualTexture origin + (chunk virtual tile offset) * (leaf node world size)
        virtualTextureConsts1.x = static_cast<float>(m_worldOriginX + indirectionTileInfo.m_vTileOffsetX * m_tileSizeMetersPerMip[0]);
        virtualTextureConsts1.y = static_cast<float>(m_worldOriginY + indirectionTileInfo.m_vTileOffsetY * m_tileSizeMetersPerMip[0]);
        virtualTextureConsts1.z = (m_vtWrapperPtr->GetTilePadding() + 0.5f) / m_vtWrapperPtr->GetPhysicalTextureCacheSize();
        virtualTextureConsts1.w = 0.0f;

        switch (shaderStage)
        {
        case eHWSC_Pixel:
            d3dRenderer->m_RP.m_pShader->FXSetPSFloat(virtualTextureConsts0Name, &virtualTextureConsts0, 1);
            d3dRenderer->m_RP.m_pShader->FXSetPSFloat(virtualTextureConsts1Name, &virtualTextureConsts1, 1);
            break;
        default:
            d3dRenderer->m_RP.m_pShader->FXSetVSFloat(virtualTextureConsts0Name, &virtualTextureConsts0, 1);
            d3dRenderer->m_RP.m_pShader->FXSetVSFloat(virtualTextureConsts1Name, &virtualTextureConsts1, 1);
            break;
        }

        return true;
    }

    void VirtualTexture::EndRendering(EHWShaderClass shaderStage)
    {
        if (shaderStage >= eHWSC_Geometry)
        {
            AZ_Error("VirtualTexture", false, "VirtualTexture support limited to Vertex and Pixel Shader!");
            return;
        }

        CRenderer* d3dRenderer = gRenDev;

        for (int indexTexture = 0; indexTexture < m_vtWrapperPtr->GetPhysicalTextureCount(); ++indexTexture)
        {
            CTexture* pTex = m_vtWrapperPtr->GetPhysicalTexture(indexTexture);
            pTex->Unbind();

            UnbindTexture(shaderStage, m_physicalTextureStartSlot + indexTexture);
        }

        // clear indirection map slot
        UnbindTexture(shaderStage, m_indirectionMapTextureSlot);
    }

    /////////////////////////////////////////////////////////////////
    // VirtualTexture Utility

    void VirtualTexture::ConvertWorldToVirtual(float worldX, float worldY, int mipLevel, int& virtualTileX, int& virtualTileY)
    {
        AZ_Assert(mipLevel >= 0 && mipLevel < m_vtWrapperPtr->GetMipLevels(), "Invalid mip level");

        float relativeX = worldX - m_worldOriginX;
        float relativeY = worldY - m_worldOriginY;

        // divide by size of a tile at this level
        virtualTileX = static_cast<int>(floorf(relativeX / m_tileSizeMetersPerMip[mipLevel]));
        virtualTileY = static_cast<int>(floorf(relativeY / m_tileSizeMetersPerMip[mipLevel]));
    }

    // Returns the world bounds in an AZ::Vector4, taking into account padding
    // .x = World Minimum X
    // .y = World Minimum Y
    // .z = World Maximum X
    // .w = World Maximum Y
    AZ::Vector4 VirtualTexture::CalculateWorldBoundsForVirtualTile(int vTileX, int vTileY, int mipLevel, bool includePadding)
    {
        int padding = includePadding ? m_vtWrapperPtr->GetTilePadding() : 0;

        float metersPerTexel = GetMetersPerTexel(mipLevel);
        float worldPadding = static_cast<float>(padding) * metersPerTexel;

        float worldXMin = vTileX * m_tileSizeMetersPerMip[mipLevel] + m_worldOriginX - worldPadding;
        float worldYMin = vTileY * m_tileSizeMetersPerMip[mipLevel] + m_worldOriginY - worldPadding;

        float worldXMax = (vTileX + 1) * m_tileSizeMetersPerMip[mipLevel] + m_worldOriginX + worldPadding;
        float worldYMax = (vTileY + 1) * m_tileSizeMetersPerMip[mipLevel] + m_worldOriginY + worldPadding;

        return AZ::Vector4(worldXMin, worldYMin, worldXMax, worldYMax);
    }

    AZ::Vector4 VirtualTexture::CalculateWorldBoundsForVirtualTile(const VirtualTile& virtualTileAddr, bool includePadding)
    {
        return CalculateWorldBoundsForVirtualTile(virtualTileAddr.m_x, virtualTileAddr.m_y, virtualTileAddr.m_mipLevel, includePadding);
    }

    void VirtualTexture::BindTexture(EHWShaderClass shaderStage, CTexture* pTexture, AZ::u32 slot)
    {
#if !defined(NULL_RENDERER)
        AZ_Assert(pTexture, "NULL virtual texture bind request.");
        gRenDev->m_DevMan.BindSRV(shaderStage, pTexture->GetShaderResourceView(), slot);
#endif
    }

    void VirtualTexture::UnbindTexture(EHWShaderClass shaderStage, AZ::u32 slot)
    {
#if !defined(NULL_RENDERER)
        gRenDev->m_DevMan.BindSRV(shaderStage, nullptr, slot);
#endif
    }
}

#endif
