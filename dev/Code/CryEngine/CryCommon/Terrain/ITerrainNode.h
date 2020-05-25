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
#pragma once

#define MML_NOT_SET ((uint8) - 1)

struct ITerrainNode;

namespace LegacyTerrain
{
    namespace TerrainNodeSurface
    {
        static const int Hole = 128;
        static const int MaxSurfaceCount = 129;
    }
}

struct STerrainNodeLeafData
{
    STerrainNodeLeafData()
    {
        memset(this, 0, sizeof(*this));
    }

    ~STerrainNodeLeafData()
    {
        m_pRenderMesh = NULL;
    }

    struct TextureParams
    {
        void Set(const SSectorTextureSet& set)
        {
            offsetX = set.fTexOffsetX;
            offsetY = set.fTexOffsetY;
            scale = set.fTexScale;
            id = set.nTex0;
        }

        float offsetX;
        float offsetY;
        float scale;
        uint32 id;
    };

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    // Use m_TextureParams for double buffering to prevent flickering due to threading issues
    // Only level 0 was used before.
    TextureParams m_TextureParams[RT_COMMAND_BUF_COUNT];
#else
    TextureParams m_TextureParams[MAX_RECURSION_LEVELS];
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED


    int m_SurfaceAxisIndexCount[LegacyTerrain::TerrainNodeSurface::MaxSurfaceCount][4];
    PodArray<ITerrainNode*> m_Neighbors;
    PodArray<uint8> m_NeighborLods;
    _smart_ptr<IRenderMesh> m_pRenderMesh;
};

struct AABB;
struct SSectorTextureSet;
struct SRenderingPassInfo;
struct STerrainNodeLeafData;

struct ITerrainNode : public IShadowCaster
{
    ITerrainNode() : m_nGSMFrameId(0) {}
    virtual ~ITerrainNode() {}

    virtual ITerrainNode* FindMinNodeContainingBox(const AABB& aabbBox) = 0;
    virtual const AABB& GetLocalAABB() const = 0;
    virtual ITerrainNode* GetParent() const = 0;
    virtual SSectorTextureSet* GetTextureSet() = 0;
    virtual int GetAreaLOD(const SRenderingPassInfo& passInfo) const = 0;
    virtual STerrainNodeLeafData* GetLeafData() const = 0;

    int m_nGSMFrameId;
    uint8 m_QueuedLOD, m_CurrentLOD;
};
