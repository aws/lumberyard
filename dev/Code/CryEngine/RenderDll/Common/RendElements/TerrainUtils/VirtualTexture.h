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

#ifdef LY_TERRAIN_RUNTIME

#pragma once

#include <AzCore/Math/Aabb.h>

#include "VTWrapper.h"
#include "TerrainRenderingParameters.h"

//
// Terrain::VirtualTexture wraps management of a spatial VirtualTexture in XY world
// coordinates. The class translates 2D area requests to tile requests from VTWrapper.
//
// For using the VirtualTexture in shader, BeginRendering() binds the indirection map and physical
// cache textures to slots designated by the user, and sets up the required shader constants for
// use with the VirtualTexture.cfi shader header. Using this shader header, world coordinates are
// translated to physical UVs for directly sampling the physical texture cache. EndRendering()
// unbinds the textures.
//

namespace Terrain
{
    class VirtualTexture
    {
    public:
        struct VirtualTextureDesc
        {
            const char* m_name = nullptr;
            ETEX_Format* m_formats = nullptr;
            ColorF* m_clearColors = nullptr;
            int m_textureCount = 0;
            float m_texelsPerMeter = 1.0f;
            int m_mipLevels = 1;
            int m_tileSize = 64;
            int m_tilePadding = 1;
            int m_physicalTextureCacheSize = 2048;
            int m_worldSizeXMeters = 2048;
            int m_worldSizeYMeters = 2048;
            int m_worldOriginX = 0;
            int m_worldOriginY = 0;
            int m_initialIndirectionTileCacheSize = 1;
        };

        VirtualTexture(const VirtualTextureDesc& desc);

        ///////////////////////////////
        // Setters/Getters

        void SetTileRequestCallback(TileRequestCallback funcPtr)
        {
            m_vtWrapperPtr->SetTileRequestCallback(funcPtr);
        }

        void SetTextureSlots(int indirectionMapTextureSlot, int physicalTextureStartSlot)
        {
            m_indirectionMapTextureSlot = indirectionMapTextureSlot;
            m_physicalTextureStartSlot = physicalTextureStartSlot;
        }

        float GetTexelsPerMeter(int mipLevel) const
        {
            float texelsPerMeter = static_cast<float>(m_vtWrapperPtr->GetTileSize()) / m_tileSizeMetersPerMip[mipLevel];
            return texelsPerMeter;
        }

        float GetMetersPerTexel(int mipLevel) const
        {
            float metersPerTexel = m_tileSizeMetersPerMip[mipLevel] / static_cast<float>(m_vtWrapperPtr->GetTileSize());
            return metersPerTexel;
        }

        int GetMipLevels() const
        {
            return m_vtWrapperPtr->GetMipLevels();
        }

        int GetTileSize() const
        {
            return m_vtWrapperPtr->GetTileSize();
        }

        int GetTilePadding() const
        {
            return m_vtWrapperPtr->GetTilePadding();
        }

        int GetPhysicalTextureCacheSize() const
        {
            return m_vtWrapperPtr->GetPhysicalTextureCacheSize();
        }

        CTexture* GetPhysicalTexture(int index)
        {
            return m_vtWrapperPtr->GetPhysicalTexture(index);
        }

        AZStd::size_t GetActiveRequestCount() const
        {
            return m_vtWrapperPtr->GetActiveRequestCount();
        }

        ///////////////////////////////
        // Functionality

        void ClearCache();

        void RequestTile(float worldX, float worldY, int mipLevel);

        // Request all height tiles necessary for the specified world area
        void RequestTiles(float worldX, float worldY, float worldXWidth, float worldYWidth, int mipLevel);

        ///////////////////////////////
        // Update pump

        // Should only call update once a frame, as this is also pumping the indirection map update which gets
        // processed as needed on a separate worker thread. If we need to call update more than once a frame,
        // then we will need to separate the indirection map update from the VirtualTexture update.
        void Update(int maxRequestsToProcess);

        ///////////////////////////////
        // Rendering
        // These functions work in conjunction with VirtualTexture.cfi

        bool BeginRendering(EHWShaderClass shaderStage, int samplerStateId, const AZ::Aabb& nodeAABB);
        void EndRendering(EHWShaderClass shaderStage);

        ///////////////////////////////
        // Utility

        void ConvertWorldToVirtual(float worldX, float worldY, int mipLevel, int& virtualTileX, int& virtualTileY);

        // Returns the world bounds in an AZ::Vector4, taking into account padding
        // .x = World Minimum X
        // .y = World Minimum Y
        // .z = World Maximum X
        // .w = World Maximum Y
        AZ::Vector4 CalculateWorldBoundsForVirtualTile(int vTileX, int vTileY, int mipLevel, bool includePadding);

        AZ::Vector4 CalculateWorldBoundsForVirtualTile(const VirtualTile& virtualTileAddr, bool includePadding);

    protected:

        // Bind a texture to a specific stage and slot
        // We are utilizing slots outside the legacy renderers texture slot support (slots 16+), so we end up using
        // platform specific code to handle texture binding
        void BindTexture(EHWShaderClass shaderStage, CTexture* pTexture, AZ::u32 slot);

        // Unbind the specific stage and slot
        void UnbindTexture(EHWShaderClass shaderStage, AZ::u32 slot);

        int m_indirectionMapTextureSlot = 0;
        int m_physicalTextureStartSlot = 1;

        AZStd::unique_ptr<VTWrapper> m_vtWrapperPtr;

        int m_worldSizeXMeters;
        int m_worldSizeYMeters;

        // World origin - for calculating virtual addresses (-x, -y origin)
        int m_worldOriginX;
        int m_worldOriginY;

        // VirtualTexture chunk size, with respect to each IndirectionMap tile
        // The VirtualTexture has been broken up into separate chunks, for which each has its' own designated IndirectionMap tile
        // The IndirectionMap has been separated into tiles so we can keep indirection map CPU -> GPU updates separated and
        // to minimize wasted GPU memory, if we instead allocated a single large indirection map to cover the entire VirtualTexture
        int m_virtualTextureChunkSize;
        float m_virtualTextureChunkWorldSize;

        float m_tileSizeMetersPerMip[Terrain::TerrainRenderingParameters::c_VirtualTextureMaxMipLevels];
    };
}

#endif
