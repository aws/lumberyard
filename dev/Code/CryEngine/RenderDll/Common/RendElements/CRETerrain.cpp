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

#include "CRETerrain.h"
#include <Terrain/Bus/TerrainProviderBus.h>
#include "I3DEngine.h"

#include "TerrainUtils/CDLOD/CDLODQuadTree.h"
#include "TerrainUtils/TerrainTextureCache.h"
#include "TerrainUtils/TerrainRenderingParameters.h"

#include <AzCore/std/smart_ptr/make_shared.h>
#include <MathConversion.h>

namespace Terrain
{
    //////////////////////////////////////////////////////////////////////
    // Utility Functions
    void GeneratePlaneVertices(int meshTriangleResolution, std::vector<SVF_P3F_C4B_T2F>& vertices)
    {
        UCol white;
        white.dcolor = 0xffffffff;

        const int sampleCount = meshTriangleResolution + 1;
        const float halfPixelOffset = 0.5f / sampleCount;

        for (int yi = 0; yi < sampleCount; ++yi)
        {
            for (int xi = 0; xi < sampleCount; ++xi)
            {
                // Grid indices with zero in the lower left, because these will get normalized into texture coordinates
                float texcoordX = static_cast<float>(xi);
                float texcoordY = static_cast<float>(yi);

                // Grid indices with zero in the middle because the terrain patch will get offset using the node center
                float z = 0;
                float x = static_cast<float>(xi - sampleCount / 2);
                float y = static_cast<float>(yi - sampleCount / 2);
                vertices.push_back(
                    {
                        Vec3(x, y, z),
                        white,
                        Vec2(texcoordX, texcoordY)
                    }
                    );
            }
        }

        assert(vertices.size() == sampleCount * sampleCount);
    }

    //! Generate indices for rendering a tessellated plane. Use xOrigin and yOrigin and meshVertexResolution to create an index buffer for a subset of those vertices
    void GeneratePlaneIndices(int xOrigin, int yOrigin, int subsetTriangleResolution, int meshTriangleResolution, std::vector<vtx_idx>& indices)
    {
        const int sampleCount = meshTriangleResolution + 1;
        const int subsetSampleCount = subsetTriangleResolution;

        for (int yi = yOrigin; yi < (yOrigin + subsetSampleCount); ++yi)
        {
            for (int xi = xOrigin; xi < (xOrigin + subsetSampleCount); ++xi)
            {
                vtx_idx i0 = yi * sampleCount + xi;
                vtx_idx i1 = i0 + 1;
                vtx_idx i2 = i0 + sampleCount;
                vtx_idx i3 = i2 + 1;

                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i2);

                indices.push_back(i1);
                indices.push_back(i3);
                indices.push_back(i2);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////

    CRETerrain::CRETerrain()
        : CRendElementBase()
    {
        mfSetType(eDATA_TerrainSystem);
        mfUpdateFlags(FCEF_TRANSFORM);

        m_quadTree = AZStd::make_unique<CDLODQuadTree>();
        m_terrainTextureQuadTree = AZStd::make_unique<CDLODQuadTree>();

        // Connect to RenderNotification bus
        AZ::RenderThreadEventsBus::Handler::BusConnect();
    }

    CRETerrain::~CRETerrain()
    {
        // Disconnect from RenderNotification bus
        AZ::RenderThreadEventsBus::Handler::BusDisconnect();
    }

    void CRETerrain::Release(bool)
    {
        // Make sure the terrain texture cache immediately disconnects
        if (m_terrainTextureCache)
        {
            m_terrainTextureCache->Shutdown();
        }
    }

    Vec3 CRETerrain::GetPositionAt(float x, float y) const
    {
        return Vec3(0, 0, 0);
    }

    template<typename T>
    void Clamp(T& v, const T& min, const T& max)
    {
        if (v < min)
        {
            v = min;
        }
        if (v > max)
        {
            v = max;
        }
    }

    void CRETerrain::mfPrepare(bool bCheckOverflow)
    {
        if (!TerrainProviderRequestBus::HasHandlers())
        {
            AZ_Warning("CRETerrain", false, "CRETerrain::mfPrepare | The terrain system hasn't been activated");
            m_dataPrepared = false;
            return;
        }

        if (bCheckOverflow)
        {
            gRenDev->FX_CheckOverflow(0, 0, this);
        }

        gRenDev->m_RP.m_pRE = this;
        gRenDev->m_RP.m_RendNumIndices = 0;
        gRenDev->m_RP.m_RendNumVerts = 0;

        int nThreadID = gRenDev->m_RP.m_nProcessThreadID;

        static unsigned int lastUpdateFrame = 0xffff;

        TerrainRenderingParameters* pTerrainParams = TerrainRenderingParameters::GetInstance();

        // no, no, seriously I ONLY want to do this once a frame
        if (gRenDev->m_nCurFSStackLevel == 0 && lastUpdateFrame != gRenDev->m_nFrameSwapID)
        {
            lastUpdateFrame = gRenDev->m_nFrameSwapID;

            // Clamp settings
            float minVertexSpacing = 0.0f;
            TerrainDataRequestBus::BroadcastResult(minVertexSpacing, &TerrainDataRequestBus::Events::GetHeightmapCellSize);
            float maxVertexSpacing = static_cast<float>(pTerrainParams->m_terrainLeafNodeSize);
            pTerrainParams->m_terrainLeafNodeVertexSpacing = AZ::GetClamp(pTerrainParams->m_terrainLeafNodeVertexSpacing, minVertexSpacing, maxVertexSpacing);

            AZ_Assert(AZ::IsClose(fmod(pTerrainParams->m_terrainLeafNodeSize, pTerrainParams->m_terrainLeafNodeVertexSpacing), 0, 0.01),
                "CRETerrain::mfPrepare | terrainLeafNodeSize must be divisible by terrainVertexSpacing");

            {
                // Update terrain patch mesh settings
                int terrainMeshResolution = static_cast<int>(0.5f + pTerrainParams->m_terrainLeafNodeSize / pTerrainParams->m_terrainLeafNodeVertexSpacing);

                if (m_terrainRenderMesh.GetMeshResolution() != terrainMeshResolution)
                {
                    m_terrainRenderMesh = TerrainPatchMesh(terrainMeshResolution);
                }
            }

            {
                // Update terrain cache settings
                float leafNodeVertexSpacing = pTerrainParams->m_terrainLeafNodeVertexSpacing;

                TerrainTextureCache::TerrainTextureCacheDesc currentTextureCacheDesc;
                if (m_terrainTextureCache != nullptr)
                {
                    currentTextureCacheDesc = m_terrainTextureCache->GetTerrainTextureCacheDesc();
                }

                if (m_terrainTextureCache == nullptr
                    || m_terrainTextureCache->GetLodLevels() != pTerrainParams->m_terrainLodLevelCount
                    || m_terrainTextureCache->GetTerrainDataTileSize() != pTerrainParams->m_terrainDataTileSize
                    || m_terrainTextureCache->GetTerrainDataPhysicalCacheSize() != pTerrainParams->m_terrainDataPhysicalCacheSize
                    || !AZ::IsClose(m_terrainTextureCache->GetLeafNodeVertexSpacing(), leafNodeVertexSpacing, 0.001f)
                    || currentTextureCacheDesc.m_terrainTextureParams.m_texelsPerMeter != pTerrainParams->m_terrainTextureTexelsPerMeter
                    || currentTextureCacheDesc.m_terrainTextureParams.m_mipLevels != pTerrainParams->m_terrainTextureMipLevels
                    || currentTextureCacheDesc.m_terrainTextureParams.m_tileSize != pTerrainParams->m_terrainTextureTileSize
                    || currentTextureCacheDesc.m_terrainTextureParams.m_physicalTextureCacheSize != pTerrainParams->m_terrainTexturePhysicalCacheSize)
                {
                    // recreate cache
                    TerrainTextureCache::TerrainTextureCacheDesc textureCacheDesc;

                    // Height map
                    textureCacheDesc.m_heightmapRenderParams.m_lodLevelCount = pTerrainParams->m_terrainLodLevelCount;
                    textureCacheDesc.m_heightmapRenderParams.m_tileSize = pTerrainParams->m_terrainDataTileSize;
                    textureCacheDesc.m_heightmapRenderParams.m_physicalTextureCacheSize = pTerrainParams->m_terrainDataPhysicalCacheSize;
                    textureCacheDesc.m_heightmapRenderParams.m_vertexSpacing = leafNodeVertexSpacing;

                    // Terrain texture compositing
                    textureCacheDesc.m_terrainTextureParams.m_texelsPerMeter = pTerrainParams->m_terrainTextureTexelsPerMeter;
                    textureCacheDesc.m_terrainTextureParams.m_mipLevels = pTerrainParams->m_terrainTextureMipLevels;
                    textureCacheDesc.m_terrainTextureParams.m_tileSize = pTerrainParams->m_terrainTextureTileSize;
                    textureCacheDesc.m_terrainTextureParams.m_physicalTextureCacheSize = pTerrainParams->m_terrainTexturePhysicalCacheSize;

                    if (m_terrainTextureCache)
                    {
                        // Shutdown/Reset must be called first because direct assignment to the sharedptr means the new instance
                        // of TerrainTextureCache is created before the previous instance is destroyed - this would end up
                        // asserting because we need the previous instance to disconnect ebuses first.
                        m_terrainTextureCache->Shutdown();
                        m_terrainTextureCache.reset();
                    }
                    m_terrainTextureCache = AZStd::make_shared<TerrainTextureCache>(textureCacheDesc);
                    m_terrainTextureCache->Init();
                }
            }

            if (m_terrainTextureCache != nullptr
                && pTerrainParams->m_terrainTextureCacheClear != 0)
            {
                pTerrainParams->m_terrainTextureCacheClear = 0;
                m_terrainTextureCache->ClearHeightmapCache();
                m_terrainTextureCache->ClearCompositedTextureCache();
            }

            // Update quad tree
            const CCamera& camera = gRenDev->m_RP.m_TI[nThreadID].m_cam;
            Vec3 cameraPosition = camera.GetPosition();

            UpdateQuadTree();

            // We've initialized / updated our data, so we're good to start rendering in mfDraw.
            m_dataPrepared = true;
        }
    }

    bool CRETerrain::mfDraw(CShader* shader, SShaderPass* shaderPass)
    {
        // we can't do anything without the extension
        static bool displayWarning = true;
        if (!TerrainProviderRequestBus::HasHandlers())
        {
            if (displayWarning)
            {
                CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "CRETerrain doesn't have an extension installed. Terrain rendering will be skipped until an extension arrives.");
                displayWarning = false;
            }
            return true;
        }

        // Make sure we only try to render data that's been prepared this frame.
        // (This guards against corruption that can be caused by spawning / despawning terrain mid-frame)
        if (!m_dataPrepared)
        {
            return true;
        }

        if (!displayWarning)
        {
            CryLog("CRETerrain extension installed, resuming rendering.");
        }

        displayWarning = true;

        const TerrainRenderingParameters* pTerrainParams = TerrainRenderingParameters::GetInstance();

        SRenderPipeline& refRenderPipeline = gRenDev->m_RP;
        if (!pTerrainParams->m_terrainEnabled)
        {
            return true;
        }

        const bool isReflectionPass = SRendItem::m_RecurseLevel[refRenderPipeline.m_nProcessThreadID] != 0;
        const bool isShadowPass = (refRenderPipeline.m_TI[refRenderPipeline.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN) != 0;

        LODSelection* lodSelection = nullptr;
        {
            PROFILE_LABEL_SCOPE("TERRAIN NODE GATHER");
            // Gathering terrain nodes

            const CCamera& camera = gRenDev->GetCamera();

            // if its a shadow pass, we want the frustum planes from the shadow camera
            const CCamera& frustumPlanesCamera = isShadowPass ? refRenderPipeline.m_ShadowInfo.m_pCurShadowFrustum->FrustumPlanes[refRenderPipeline.m_ShadowInfo.m_nOmniLightSide] : camera;

            // gather frustum planes
            AZ::Plane frustumPlanes[FRUSTUM_PLANES];
            for (int i = 0; i < FRUSTUM_PLANES; ++i)
            {
                const Plane* frPlane = frustumPlanesCamera.GetFrustumPlane(i);
                frustumPlanes[i].Set(frPlane->n.x, frPlane->n.y, frPlane->n.z, frPlane->d);
            }

            // We still want the observer position and view range to be based on the main scene camera
            AZ::Vector3 cameraPos;
            Vec3 pos = camera.GetPosition();
            cameraPos.Set(pos.x, pos.y, pos.z);

            const bool debugFreezeNodes = pTerrainParams->m_terrainDebugFreezeNodes > 0;
            static bool isFreezeNodeDataSaved = false;
            if (!isShadowPass && debugFreezeNodes && !isFreezeNodeDataSaved)
            {
                for (int i = 0; i < FRUSTUM_PLANES; ++i)
                {
                    m_savedFrustumPlanes[i] = frustumPlanes[i];
                }

                m_savedCameraPosition = cameraPos;
            }
            else if (!isShadowPass && debugFreezeNodes)
            {
                // apply the saved info
                for (int i = 0; i < FRUSTUM_PLANES; ++i)
                {
                    frustumPlanes[i] = m_savedFrustumPlanes[i];
                }
                cameraPos = m_savedCameraPosition;
            }
            if (!isShadowPass)
            {
                isFreezeNodeDataSaved = debugFreezeNodes;
            }

            // When rendering reflections, limit to lowest detail LOD
            int stopAtLodLevel = isReflectionPass ? (m_quadTree->GetLODLevelCount() - 1) : 0;

            // Terrain texture compositing requests
            if (!isReflectionPass
                && !isShadowPass)
            {
                const float terrainTextureViewDistance = pTerrainParams->m_terrainTextureCompositeViewDistance;

                // Texture streaming quad tree selection setup
                LODSelection::LODSelectionDesc textureStreamingSelectionDesc;
                std::copy_n(frustumPlanes, LODSelection::c_FrustumPlaneCount, textureStreamingSelectionDesc.m_frustumPlanes);
                textureStreamingSelectionDesc.m_cameraPosition = cameraPos;
                textureStreamingSelectionDesc.m_lodRatio = 0.0f;                                        // irrelevant for texture streaming
                textureStreamingSelectionDesc.m_initialLODDistance = 0.0f;                              // irrelevant for texture streaming
                textureStreamingSelectionDesc.m_maxVisibilityDistance = terrainTextureViewDistance;
                textureStreamingSelectionDesc.m_stopAtLODLevel = stopAtLodLevel;
                textureStreamingSelectionDesc.m_morphStartRatio = CDLODQUADTREE_DEFAULT_MORPH_START_RATIO;
                textureStreamingSelectionDesc.m_flags = (LODSelection::Flags)(LODSelection::TextureStreaming | LODSelection::IncludeAllNodesInRange | LODSelection::SortByDistance);
                LODSelectionOnStack<4096> cdlodSelection(textureStreamingSelectionDesc);

                int vpX(0), vpY(0), vpWidth(0), vpHeight(0);
                gRenDev->GetViewport(&vpX, &vpY, &vpWidth, &vpHeight);

                float clampedScreenHeight = AZ::GetMin(static_cast<float>(vpHeight), static_cast<float>(pTerrainParams->m_terrainMaxScreenHeight));

                cdlodSelection.m_textureStreamingParams.m_textureTileSize = pTerrainParams->m_terrainTextureTileSize;
                cdlodSelection.m_textureStreamingParams.m_screenPixelHeight = clampedScreenHeight;
                cdlodSelection.m_textureStreamingParams.m_texelsPerMeter = static_cast<float>(pTerrainParams->m_terrainTextureTexelsPerMeter);
                cdlodSelection.m_textureStreamingParams.m_fov = camera.GetFov();

                m_terrainTextureQuadTree->LODSelect(&cdlodSelection);

                if (pTerrainParams->m_terrainTextureRequestDebug == 1
                    && gRenDev->GetFrameID() % 60 == 0)
                {
                    AZ_Printf("CRETerrain", "Count=[%d] MaxMipExceededCount=[%d]", cdlodSelection.GetSelectionCount(), cdlodSelection.m_debugMaxMipLevelExceededCount);
                }

                if (pTerrainParams->m_terrainDrawAABBs == 2)
                {
                    // Draw the far plane
                    const Vec3 offset = pos + gRenDev->GetViewParameters().ViewDir() * terrainTextureViewDistance;
                    const Vec3 upVec = gRenDev->GetViewParameters().vY * 10000.0f;
                    const Vec3 rightVec = gRenDev->GetViewParameters().vX * 10000.0f;

                    Vec3 farPlaneVertices[6];
                    farPlaneVertices[0] = offset + (rightVec) + (upVec);
                    farPlaneVertices[1] = offset + (-rightVec) + (upVec);
                    farPlaneVertices[2] = offset + (-rightVec) + (-upVec);
                    farPlaneVertices[3] = offset + (-rightVec) + (-upVec);
                    farPlaneVertices[4] = offset + (rightVec) + (-upVec);
                    farPlaneVertices[5] = offset + (rightVec) + (upVec);

                    gRenDev->GetIRenderAuxGeom()->DrawTriangles(&farPlaneVertices[0], 6, ColorB(0xFF, 0x00, 0x00, 0x80));

                    RenderNodeAABBs(&cdlodSelection);
                }

                // Terrain texture requests based on culling results
                m_terrainTextureCache->RequestTerrainTextureTiles(cdlodSelection);
            }

            LODSelection::LODSelectionDesc terrainMeshSelectionDesc;
            std::copy_n(frustumPlanes, LODSelection::c_FrustumPlaneCount, terrainMeshSelectionDesc.m_frustumPlanes);
            terrainMeshSelectionDesc.m_cameraPosition = cameraPos;
            terrainMeshSelectionDesc.m_lodRatio = pTerrainParams->m_terrainLodLevelDistanceRatio;
            terrainMeshSelectionDesc.m_initialLODDistance = pTerrainParams->m_terrainLod0Distance;
            terrainMeshSelectionDesc.m_maxVisibilityDistance = pTerrainParams->m_terrainViewDistance;
            terrainMeshSelectionDesc.m_stopAtLODLevel = stopAtLodLevel;
            terrainMeshSelectionDesc.m_morphStartRatio = CDLODQUADTREE_DEFAULT_MORPH_START_RATIO;
            terrainMeshSelectionDesc.m_flags = (LODSelection::Flags)(LODSelection::SortByDistance);
            LODSelectionOnStack<4096> cdlodSelection(terrainMeshSelectionDesc);

            m_quadTree->LODSelect(&cdlodSelection);

            // Debug render by rendering AABBs
            if (!isShadowPass
                && !isReflectionPass
                && pTerrainParams->m_terrainDrawAABBs == 1)
            {
                RenderNodeAABBs(&cdlodSelection);
            }

            lodSelection = &cdlodSelection;

            if (m_terrainTextureCache)
            {
                // Terrain heightmap data requests based on lod selection results
                m_terrainTextureCache->RequestTerrainDataTiles(*lodSelection);
            }
        }

        {
            PROFILE_LABEL_SCOPE("TERRAIN DRAW");

            gRenDev->SetCullMode(R_CULL_BACK);

            uint64 prevShaderRTMask = gRenDev->m_RP.m_FlagsShader_RT;
            uint32 prevStateAnd = gRenDev->m_RP.m_StateAnd;

            int prevStencilState = gRenDev->m_RP.m_CurStencilState;
            uint32 prevStencilRef = gRenDev->m_RP.m_CurStencRef;
            uint32 prevStencilMask = gRenDev->m_RP.m_CurStencMask;
            uint32 prevStencilWriteMask = gRenDev->m_RP.m_CurStencWriteMask;

            uint32 prevState = gRenDev->m_RP.m_CurState;

            if (!isShadowPass)
            {
                // We are utilizing EFSLIST_TERRAINLAYERS so our custom terrain renders after all other
                // opaque objects. To use this render list, we have to manually set blend, depth, and stencil states:
                // * Disable alpha blend
                // * Enable depth write
                // * Enable stencil write

                gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_ALPHABLEND];
                gRenDev->m_RP.m_StateAnd &= ~GS_BLEND_MASK;

                // Manually forcing stencil state to write BIT_STENCIL_RESERVED for terrain
                // BIT_STENCIL_RESERVED designates objects that may receive deferred decals
                gRenDev->FX_SetStencilState(
                    STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
                    STENCOP_FAIL(FSS_STENCOP_KEEP) |
                    STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
                    STENCOP_PASS(FSS_STENCOP_REPLACE),
                    BIT_STENCIL_RESERVED, 0xFFFFFFFF, 0xFFFFFFFF);

                uint32 nState = GS_DEPTHWRITE | GS_STENCIL;
                gRenDev->FX_SetState(nState);
            }

            {
                uint32 nPasses(0);
                shader->FXBegin(&nPasses, FEF_DONTSETSTATES);

                if (nPasses > 0)
                {
                    if (pTerrainParams->m_terrainWireframe != 2)
                    {
                        for (uint32 passIdx = 0; passIdx < nPasses; ++passIdx)
                        {
                            shader->FXBeginPass(passIdx);

                            gRenDev->FX_Commit();

                            if (lodSelection)
                            {
                                RenderNodes(lodSelection);
                            }

                            shader->FXEndPass();
                        }
                    }

                    shader->FXEnd();

                    if (pTerrainParams->m_terrainWireframe != 0
                        && !isShadowPass
                        && !isReflectionPass)
                    {
                        static CCryNameTSCRC terrainWireframeTechnique("TerrainDebugColor");
                        static CCryNameR terrainDebugColorConstant("g_terrainDebugColor");
                        Vec4 terrainWireframeColor(0, 0, 0, 1);

                        bool techniqueExists = shader->FXSetTechnique(terrainWireframeTechnique);
                        if (techniqueExists)
                        {
                            shader->FXBeginPass(0);
                            shader->FXSetPSFloat(terrainDebugColorConstant, &terrainWireframeColor, 1);
                            gRenDev->FX_Commit();

                            RenderNodesWireframe(lodSelection);

                            shader->FXEndPass();
                            shader->FXEnd();
                        }
                    }
                }
            }

            if (!isShadowPass)
            {
                // Restore renderer state
                gRenDev->m_RP.m_FlagsShader_RT = prevShaderRTMask;
                gRenDev->m_RP.m_StateAnd = prevStateAnd;
                gRenDev->FX_SetStencilState(prevStencilState, prevStencilRef, prevStencilMask, prevStencilWriteMask);
                gRenDev->FX_SetState(prevState);
            }
        }
        return true;
    }

    void CRETerrain::OnRenderThreadRenderSceneBegin()
    {
        UpdateTextureCaches();
    }

    void CRETerrain::UpdateQuadTree()
    {
        const float epsilon = 0.01f;

        TerrainRenderingParameters* pTerrainParams = TerrainRenderingParameters::GetInstance();
        AZ::Vector3 worldOrigin(0.0f);
        AZ::Vector3 worldSize(0.0f);
        TerrainProviderRequestBus::BroadcastResult(worldOrigin, &TerrainProviderRequestBus::Events::GetWorldOrigin);
        TerrainProviderRequestBus::BroadcastResult(worldSize, &TerrainProviderRequestBus::Events::GetWorldSize);

        {
            // Update quadtree used for terrain mesh patch rendering

            const int leafNodeSize = m_quadTree->GetLeafNodeSize();
            const int lodLevelCount = m_quadTree->GetLODLevelCount();
            CDLODQuadTree::MapDimensions quadTreeDimensions = m_quadTree->GetWorldMapDims();

            pTerrainParams->m_terrainLodLevelCount = AZ::GetClamp(pTerrainParams->m_terrainLodLevelCount, 1, TerrainRenderingParameters::c_TerrainMaxLODLevels);

            if (leafNodeSize != pTerrainParams->m_terrainLeafNodeSize
                || lodLevelCount != pTerrainParams->m_terrainLodLevelCount
                || !quadTreeDimensions.m_worldMin.IsClose(worldOrigin)
                || !quadTreeDimensions.m_worldSize.IsClose(worldSize))
            {
                CDLODQuadTree::CreateDesc createDesc;
                createDesc.m_leafRenderNodeSize = pTerrainParams->m_terrainLeafNodeSize;
                createDesc.m_lodLevelCount = pTerrainParams->m_terrainLodLevelCount;
                createDesc.m_mapDims.m_worldMin = worldOrigin;
                createDesc.m_mapDims.m_worldSize = worldSize;

                m_quadTree->Create(createDesc);
            }
        }

        {
            // Update quadtree used for terrain texture tile compositing
            const int leafNodeSize = m_terrainTextureQuadTree->GetLeafNodeSize();
            const int mipLevels = m_terrainTextureQuadTree->GetLODLevelCount();
            CDLODQuadTree::MapDimensions quadTreeDimensions = m_terrainTextureQuadTree->GetWorldMapDims();

            // Warning when these aren't divisible, will lead to visual artifacts
            const int requiredLeafNodeSize = pTerrainParams->m_terrainTextureTileSize / pTerrainParams->m_terrainTextureTexelsPerMeter;

            pTerrainParams->m_terrainTextureMipLevels = AZ::GetClamp(pTerrainParams->m_terrainTextureMipLevels, 1, TerrainRenderingParameters::c_TerrainMaxLODLevels);

            if (leafNodeSize != requiredLeafNodeSize
                || mipLevels != pTerrainParams->m_terrainTextureMipLevels
                || !quadTreeDimensions.m_worldMin.IsClose(worldOrigin)
                || !quadTreeDimensions.m_worldSize.IsClose(worldSize))
            {
                CDLODQuadTree::CreateDesc createDesc;
                createDesc.m_leafRenderNodeSize = requiredLeafNodeSize;
                createDesc.m_lodLevelCount = pTerrainParams->m_terrainTextureMipLevels;
                createDesc.m_mapDims.m_worldMin = worldOrigin;
                createDesc.m_mapDims.m_worldSize = worldSize;

                m_terrainTextureQuadTree->Create(createDesc);

                AZ_Warning("CRETerrain", pTerrainParams->m_terrainTextureTileSize % pTerrainParams->m_terrainTextureTexelsPerMeter == 0,
                    "terrainTextureTileSize not divisible by terrainTextureTexelsPerMeter! This will result in visual artifacts!");
            }
        }
    }

    void CRETerrain::RenderNodes(const LODSelection* selection) const
    {
        // Collect morph constants for each LOD before we start rendering
        // (probably can be calculated and stored in LODSelection during the LODSelect call, as that feels like a better place for that data)
        Vec4 morphConstants[TerrainRenderingParameters::c_TerrainMaxLODLevels];
        int lodLevelCount = m_quadTree->GetLODLevelCount();
        for (int iLodLevel = 0; iLodLevel < lodLevelCount; ++iLodLevel)
        {
            float lodLevelMorphConstants[4];
            selection->GetMorphConsts(iLodLevel, lodLevelMorphConstants);
            morphConstants[iLodLevel].x = lodLevelMorphConstants[0];
            morphConstants[iLodLevel].y = lodLevelMorphConstants[1];
            morphConstants[iLodLevel].z = lodLevelMorphConstants[2];
            morphConstants[iLodLevel].w = lodLevelMorphConstants[3];
        }

        const TerrainRenderingParameters* pTerrainParams = TerrainRenderingParameters::GetInstance();
        const float textureCompositeDistanceSqr = pTerrainParams->GetTerrainTextureCompositeViewDistanceSqr();

        float pomHeightBias = 0.0f;
        float pomDisplacement = 0.0f;
        float selfShadowStrength = 0.0f;
        Terrain::WorldMaterialRequestBus::Broadcast(&Terrain::WorldMaterialRequestBus::Events::GetTerrainPOMParameters, pomHeightBias, pomDisplacement, selfShadowStrength);

        if (m_terrainTextureCache
            && selection->GetSelectionCount() > 0)
        {
            m_terrainTextureCache->SetPOMHeightBias(pomHeightBias);
            m_terrainTextureCache->SetPOMDisplacement(pomDisplacement);
            m_terrainTextureCache->SetSelfShadowStrength(selfShadowStrength);

            for (int i = 0; i < selection->GetSelectionCount(); i++)
            {
                const SelectedNode& nodeSel = selection->GetSelection()[i];

                // Max compositing distance check
                bool useCompositedTerrainTextures = nodeSel.m_minDistSqrToCamera < textureCompositeDistanceSqr;

                // Binds virtual textures for rendering
                // Sets constants for utilizing VirtualTextures in shader
                if (!m_terrainTextureCache->BeginRendering(nodeSel, useCompositedTerrainTextures))
                {
                    continue;
                }

                m_terrainRenderMesh.RenderNode(nodeSel, morphConstants[nodeSel.m_lodLevel]);
            }

            m_terrainTextureCache->EndRendering();
        }
    }

    void CRETerrain::RenderNodesWireframe(const LODSelection* selection) const
    {
        CScopedWireFrameMode scopedWireFrame(gRenDev, R_WIREFRAME_MODE);
        RenderNodes(selection);
    }

    void CRETerrain::RenderNodeAABBs(const LODSelection* selection) const
    {
        float dbgCl = 0.2f;
        float dbgLODLevelColors[4][4] = {
            { 1.0f, 1.0f, 1.0f, 1.0f },
            { 1.0f, dbgCl, dbgCl, 1.0f },
            { dbgCl, 1.0f, dbgCl, 1.0f },
            { dbgCl, dbgCl, 1.0f, 1.0f }
        };

        for (int i = 0; i < selection->GetSelectionCount(); i++)
        {
            const SelectedNode& nodeSel = selection->GetSelection()[i];
            int lodLevel = nodeSel.m_lodLevel;

            bool drawFull = nodeSel.TL && nodeSel.TR && nodeSel.BL && nodeSel.BR;

            ColorB col(
                (((int)(dbgLODLevelColors[lodLevel % 4][0] * 255.0f) & 0xFF)),
                (((int)(dbgLODLevelColors[lodLevel % 4][1] * 255.0f) & 0xFF)),
                (((int)(dbgLODLevelColors[lodLevel % 4][2] * 255.0f) & 0xFF)),
                (((int)(dbgLODLevelColors[lodLevel % 4][3] * 255.0f) & 0xFF)));

            AABB boundingBox = AZAabbToLyAABB(nodeSel.m_aabb);

            if (drawFull)
            {
                gRenDev->GetIRenderAuxGeom()->DrawAABB(boundingBox, false, col, eBBD_Faceted);
            }
            else
            {
                Vec3 center = boundingBox.GetCenter();
                float midX = center.x;
                float midY = center.y;

                if (nodeSel.TL)
                {
                    AABB bbSub = boundingBox;
                    bbSub.max.x = midX;
                    bbSub.max.y = midY;
                    gRenDev->GetIRenderAuxGeom()->DrawAABB(bbSub, false, col, eBBD_Faceted);
                }
                if (nodeSel.TR)
                {
                    AABB bbSub = boundingBox;
                    bbSub.min.x = midX;
                    bbSub.max.y = midY;
                    gRenDev->GetIRenderAuxGeom()->DrawAABB(bbSub, false, col, eBBD_Faceted);
                }
                if (nodeSel.BL)
                {
                    AABB bbSub = boundingBox;
                    bbSub.max.x = midX;
                    bbSub.min.y = midY;
                    gRenDev->GetIRenderAuxGeom()->DrawAABB(bbSub, false, col, eBBD_Faceted);
                }
                if (nodeSel.BR)
                {
                    AABB bbSub = boundingBox;
                    bbSub.min.x = midX;
                    bbSub.min.y = midY;
                    gRenDev->GetIRenderAuxGeom()->DrawAABB(bbSub, false, col, eBBD_Faceted);
                }
            }
        }
    }

    void CRETerrain::UpdateTextureCaches()
    {
        if (!m_terrainTextureCache)
        {
            return;
        }

        PROFILE_LABEL_SCOPE("TERRAIN CACHE UPDATE");

        m_terrainTextureCache->RenderUpdate();
    }



    //////////////////////////////////////////////////////////////////////
    // TerrainPatchMesh Impl

    TerrainPatchMesh::TerrainPatchMesh()
        : m_uMeshResolution(0)
    {
    }

    TerrainPatchMesh::TerrainPatchMesh(int meshResolution)
        : m_uMeshResolution(meshResolution)
    {
        UCol white;
        white.dcolor = 0xffffffff;

        std::vector<SVF_P3F_C4B_T2F> vertices;
        std::vector<vtx_idx> indices;

        const unsigned int meshRes = m_uMeshResolution;
        const unsigned int halfMeshRes = meshRes / 2;

        GeneratePlaneVertices(meshRes, vertices);

        m_renderPatchStartIndices[0] = 0;
        GeneratePlaneIndices(0, 0, halfMeshRes, meshRes, indices);  // 0,0 (top left)

        m_renderPatchStartIndices[1] = indices.size();
        GeneratePlaneIndices(halfMeshRes, 0, halfMeshRes, meshRes, indices);  // 1,0 (top right)

        m_renderPatchStartIndices[2] = indices.size();
        GeneratePlaneIndices(0, halfMeshRes, halfMeshRes, meshRes, indices);  // 0,1 (bottom left)

        m_renderPatchStartIndices[3] = indices.size();
        GeneratePlaneIndices(halfMeshRes, halfMeshRes, halfMeshRes, meshRes, indices);  // 1,1 (bottom right)

        m_renderMesh = gRenDev->CreateRenderMeshInitialized(
            vertices.data(), vertices.size(), eVF_P3F_C4B_T2F,
            indices.data(), indices.size(), prtTriangleList,
            "CRETerrain:TerrainPatchMesh", "procedural", eRMT_Dynamic);

        m_uRenderPatchIndexCount = m_renderPatchStartIndices[1];
    }

    void TerrainPatchMesh::RenderFullPatch() const
    {
        HRESULT hr = S_OK;

        CRenderMesh* renderMesh = static_cast<CRenderMesh*>(m_renderMesh.get());
        renderMesh->CheckUpdate(0);
        hr = gRenDev->FX_SetVertexDeclaration(0, renderMesh->_GetVertexFormat());

        renderMesh->BindStreamsToRenderPipeline();

        unsigned int vertCount = renderMesh->GetNumVerts();
        gRenDev->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, vertCount, 0, renderMesh->GetNumInds());
    }

    void TerrainPatchMesh::RenderNode(const SelectedNode& nodeSel, const Vec4& morphConstants) const
    {
        static CCryNameR terrainPatchInfoName("g_terrainPatchInformation");
        static CCryNameR morphConstantsName("g_morphConsts");

        // Prepare node specific data
        Vec4 terrainPatchInfo;
        AZ::Vector3 nodeCenter = nodeSel.m_aabb.GetCenter();
        terrainPatchInfo.x = nodeCenter.GetX();
        terrainPatchInfo.y = nodeCenter.GetY();
        terrainPatchInfo.z = nodeSel.GetSize().GetX() / static_cast<float>(m_uMeshResolution);
        terrainPatchInfo.w = static_cast<float>(nodeSel.m_lodLevel);

        HRESULT hr = S_OK;

        gRenDev->m_RP.m_pShader->FXSetVSFloat(morphConstantsName, &morphConstants, 1);
        gRenDev->m_RP.m_pShader->FXSetVSFloat(terrainPatchInfoName, &terrainPatchInfo, 1);

        gRenDev->m_RP.m_pShader->FXSetPSFloat(terrainPatchInfoName, &terrainPatchInfo, 1);

        gRenDev->FX_Commit();

        CRenderMesh* renderMesh = static_cast<CRenderMesh*>(m_renderMesh.get());
        renderMesh->CheckUpdate(VSM_GENERAL);
        hr = gRenDev->FX_SetVertexDeclaration(VSM_GENERAL, renderMesh->_GetVertexFormat());

        renderMesh->BindStreamsToRenderPipeline();

        unsigned int vertCount = renderMesh->GetNumVerts();

        if (nodeSel.TL && nodeSel.TR && nodeSel.BL && nodeSel.BR)
        {
            gRenDev->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, vertCount, 0, renderMesh->GetNumInds());
            return;
        }

        if (nodeSel.TL)
        {
            gRenDev->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, vertCount, m_renderPatchStartIndices[0], m_uRenderPatchIndexCount);
        }

        if (nodeSel.TR)
        {
            gRenDev->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, vertCount, m_renderPatchStartIndices[1], m_uRenderPatchIndexCount);
        }

        if (nodeSel.BL)
        {
            gRenDev->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, vertCount, m_renderPatchStartIndices[2], m_uRenderPatchIndexCount);
        }

        if (nodeSel.BR)
        {
            gRenDev->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, vertCount, m_renderPatchStartIndices[3], m_uRenderPatchIndexCount);
        }
    }
}

#endif
