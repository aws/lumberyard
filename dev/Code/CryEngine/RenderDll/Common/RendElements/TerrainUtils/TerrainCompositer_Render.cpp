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

#include "DriverD3D.h"
#include "I3DEngine.h"

#include "TerrainCompositer.h"
#include "VirtualTexture.h"
#include <Terrain/Bus/TerrainProviderBus.h>

#include <Textures/TextureManager.h>

namespace Terrain
{
    namespace
    {
        void BindTexture(int slot, EHWShaderClass shaderStage, CTexture* pTex, CTexture* pBackupTex)
        {
            STexState bilinearSamplerState;
            bilinearSamplerState.SetFilterMode(FILTER_LINEAR);
            bilinearSamplerState.SetClampMode(TADDR_CLAMP, TADDR_CLAMP, TADDR_CLAMP);
            int bilinearSamplerStateID = CTexture::GetTexState(bilinearSamplerState);

            if (pTex)
            {
                pTex->Apply(slot, bilinearSamplerStateID, EFTT_UNKNOWN, -1, SResourceView::DefaultView, shaderStage);
            }
            else
            {
                if (pBackupTex)
                {
                    pBackupTex->Apply(slot, bilinearSamplerStateID, EFTT_UNKNOWN, -1, SResourceView::DefaultView, shaderStage);
                }
            }
        }

        void ClearTile()
        {
            static CCryNameTSCRC clearPassTechniqueName("ClearTile");
            static CCryNameR terrainTileClearColorName("g_terrainTileClearColor");

            const Vec4 tileDebugClearColor(0.5f, 0.0f, 0.5f, 1.0f);

            CShader* terrainTileCompositingShader = TerrainCompositer::GetTerrainTileCompositingShader();

            if (terrainTileCompositingShader)
            {
                uint32 numPasses;

                terrainTileCompositingShader->FXSetTechnique(clearPassTechniqueName);
                terrainTileCompositingShader->FXBegin(&numPasses, 0);
                terrainTileCompositingShader->FXBeginPass(0);

                terrainTileCompositingShader->FXSetPSFloat(terrainTileClearColorName, &tileDebugClearColor, 1);

                gcpRendD3D->SetCullMode(R_CULL_NONE);
                gcpRendD3D->DrawQuad(0, 0, 1, 1, ColorF(1, 1, 1, 1));

                terrainTileCompositingShader->FXEndPass();
                terrainTileCompositingShader->FXEnd();
            }
        }

        void BindMacroMaterialShaderConstants(const MacroMaterial& regionMacroMaterial)
        {
            static CCryNameR macroMaterialParamsName("g_macroMaterialParams");
            static CCryNameR macroMaterialDiffuseColorName("g_macroMaterialDiffuseColor");

            const float epsilon = 0.001f;

            // Material Params
            Vec4 macroMaterialParams;
            // #define TERRAIN_SYSTEM_MACRO_MAT_GLOSS_SCALE (g_macroMaterialParams.x)
            // #define TERRAIN_SYSTEM_MACRO_MAT_NORMAL_SCALE (g_macroMaterialParams.y)
            // #define TERRAIN_SYSTEM_MACRO_MAT_SPEC_REFL (g_macroMaterialParams.z)
            // w unused
            macroMaterialParams.x = regionMacroMaterial.m_macroGlossMapScale;
            macroMaterialParams.y = AZ::GetMax(regionMacroMaterial.m_macroNormalMapScale, epsilon);
            macroMaterialParams.z = regionMacroMaterial.m_macroSpecReflectance;

            Vec4 macroMaterialDiffuseColor;
            macroMaterialDiffuseColor.x = regionMacroMaterial.m_macroColorMapColor.GetR();
            macroMaterialDiffuseColor.y = regionMacroMaterial.m_macroColorMapColor.GetG();
            macroMaterialDiffuseColor.z = regionMacroMaterial.m_macroColorMapColor.GetB();
            macroMaterialDiffuseColor.w = regionMacroMaterial.m_macroColorMapColor.GetA();

            gcpRendD3D->m_RP.m_pShader->FXSetPSFloat(macroMaterialParamsName, &macroMaterialParams, 1);
            gcpRendD3D->m_RP.m_pShader->FXSetPSFloat(macroMaterialDiffuseColorName, &macroMaterialDiffuseColor, 1);
        }
    } // end util namespace

    bool TerrainCompositer::RenderTile(const Terrain::VirtualTileRequestInfo& requestInfo)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        {
            // generate temp textures for ping pong blending
            int tmpBufferWidth = requestInfo.m_physicalViewportDest.m_width;
            int tmpBufferHeight = requestInfo.m_physicalViewportDest.m_height;
            if (m_compositeTempTextures[0] == nullptr
                || m_compositeTempTextures[0]->GetWidth() != tmpBufferWidth
                || m_compositeTempTextures[0]->GetHeight() != tmpBufferHeight)
            {
                m_compositeTempTextures[0] = CTexture::CreateRenderTarget("TerrainCompositer::HeightTmp[0]", tmpBufferWidth, tmpBufferHeight, Col_Black, eTT_2D, FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_NOMIPS, eTF_R8G8B8A8, -1);
                m_compositeTempTextures[1] = CTexture::CreateRenderTarget("TerrainCompositer::HeightTmp[1]", tmpBufferWidth, tmpBufferHeight, Col_Black, eTT_2D, FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_NOMIPS, eTF_R8G8B8A8, -1);
                m_compositeTempTextures[2] = CTexture::CreateRenderTarget("TerrainCompositer::HeightTmp[2]", tmpBufferWidth, tmpBufferHeight, Col_Black, eTT_2D, FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_NOMIPS, eTF_R8G8B8A8, -1);
                m_compositeTempTextures[3] = CTexture::CreateRenderTarget("TerrainCompositer::HeightTmp[3]", tmpBufferWidth, tmpBufferHeight, Col_Black, eTT_2D, FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_NOMIPS, eTF_R8G8B8A8, -1);
            }
        }

        AZ::Vector4 tileWorldMinMax = m_VirtualTexture->CalculateWorldBoundsForVirtualTile(requestInfo.m_virtualTileAddress, true);  // returns (World Minimum X, World Minimum Y, World Maximum X, World Maximum Y)
        const float tileWorldSizeX = tileWorldMinMax.GetZ() - tileWorldMinMax.GetX();
        const float tileWorldSizeY = tileWorldMinMax.GetW() - tileWorldMinMax.GetY();

        AZ::Vector3 worldOrigin(0.0f);
        AZ::Vector3 worldSize(0.0f);
        AZ::Vector3 regionSize(0.0f);

        TerrainProviderRequestBus::BroadcastResult(worldOrigin, &TerrainProviderRequestBus::Events::GetWorldOrigin);
        TerrainProviderRequestBus::BroadcastResult(worldSize, &TerrainProviderRequestBus::Events::GetWorldSize);
        TerrainProviderRequestBus::BroadcastResult(regionSize, &TerrainProviderRequestBus::Events::GetRegionSize);

        // Determine regions to iterate over
        // region X,Y in the world grid
        const int startRegionX = static_cast<int>(floor((tileWorldMinMax.GetX() - worldOrigin.GetX()) / regionSize.GetX()));
        const int startRegionY = static_cast<int>(floor((tileWorldMinMax.GetY() - worldOrigin.GetY()) / regionSize.GetY()));
        const int endRegionX = static_cast<int>(floor((tileWorldMinMax.GetZ() - worldOrigin.GetX()) / regionSize.GetX()));
        const int endRegionY = static_cast<int>(floor((tileWorldMinMax.GetW() - worldOrigin.GetY()) / regionSize.GetY()));

        // Gather regions so we can query region material asset status
        int regionCount = (endRegionY - startRegionY + 1) * (endRegionX - startRegionX + 1);
        RegionIndexVector regionsToRequest;
        for (int regionY = startRegionY; regionY <= endRegionY; ++regionY)
        {
            for (int regionX = startRegionX; regionX <= endRegionX; ++regionX)
            {
                regionsToRequest.push_back(AZStd::make_pair(regionX, regionY));
            }
        }

        m_currentRegionMaterials.clear();
        RequestResult result = RequestResult::NoAssetsForRegion;
        Terrain::WorldMaterialRequestBus::BroadcastResult(result, &Terrain::WorldMaterialRequestBus::Events::RequestRegionMaterials, regionsToRequest, m_currentRegionMaterials);

        if (gEnv->IsEditor())
        {
            // In Editor only, for debugging asset issues while authoring
            // Clear the entire quad first
            bool pushRenderTargetResult = false;
            pushRenderTargetResult = gcpRendD3D->FX_PushRenderTarget(0, m_VirtualTexture->GetPhysicalTexture(0), nullptr);
            AZ_Assert(pushRenderTargetResult, "Failed to push render target 0");
            pushRenderTargetResult = gcpRendD3D->FX_PushRenderTarget(1, m_VirtualTexture->GetPhysicalTexture(1), nullptr);
            AZ_Assert(pushRenderTargetResult, "Failed to push render target 1");
            gcpRendD3D->SetViewport(requestInfo.m_physicalViewportDest.m_topLeftX, requestInfo.m_physicalViewportDest.m_topLeftY, requestInfo.m_physicalViewportDest.m_width, requestInfo.m_physicalViewportDest.m_height);
            gcpRendD3D->FX_SetActiveRenderTargets();

            ClearTile();

            gcpRendD3D->FX_PopRenderTarget(0);
            gcpRendD3D->FX_PopRenderTarget(1);
        }

        if (result == RequestResult::NoAssetsForRegion)
        {
            // No valid material assets, so just end it here
            return true;
        }
        else if (result == RequestResult::Loading)
        {
            // So we can come back and reprocess this request
            return false;
        }
        //else result == RequestResult::Success

        // Cache prev state
        CShader* pPrevShader = gcpRendD3D->m_RP.m_pShader;
        CShaderResources* pPrevShaderResources = gcpRendD3D->m_RP.m_pShaderResources;

        // We rely on Terrain::WorldMaterialRequestBus::Handler::RequestRegionMaterials returning regions in the same order we request them
        size_t regionIdx = 0;
        for (int regionY = startRegionY; regionY <= endRegionY; ++regionY)
        {
            for (int regionX = startRegionX; regionX <= endRegionX; ++regionX)
            {
                // m_currentRegionMaterials will always have `regionCount` number of elements, whether or not there are valid terrain material assets.
                // Invalid (or missing) terrain material assets will have a null default material, which we can filter on.
                RegionMaterials& currentRegionMaterial = m_currentRegionMaterials[regionIdx++];

                // Sanity check for default material
                if (!currentRegionMaterial.m_defaultMaterial)
                {
                    continue;
                }

                // Calculate region bounds
                // regionMin = regionIndex * regionSize + worldOrigin
                // regionMax = (regionIndex + 1) * regionSize + worldOrigin
                AZ::Vector3 regionWorldMin = AZ::Vector3(static_cast<float>(regionX), static_cast<float>(regionY), 0.0f) * regionSize + worldOrigin;
                AZ::Vector3 regionWorldMax = AZ::Vector3(static_cast<float>(regionX + 1), static_cast<float>(regionY + 1), 0.0f) * regionSize + worldOrigin;

                // Calculate overlapping world unit viewport
                float viewportWorldMinX = AZ::GetMax(regionWorldMin.GetX(), tileWorldMinMax.GetX());
                float viewportWorldMinY = AZ::GetMax(regionWorldMin.GetY(), tileWorldMinMax.GetY());
                float viewportWorldMaxX = AZ::GetMin(regionWorldMax.GetX(), tileWorldMinMax.GetZ());
                float viewportWorldMaxY = AZ::GetMin(regionWorldMax.GetY(), tileWorldMinMax.GetW());

                float viewportWorldSizeX = viewportWorldMaxX - viewportWorldMinX;
                float viewportWorldSizeY = viewportWorldMaxY - viewportWorldMinY;

                Vec4 tileUVoffsetScale;
                tileUVoffsetScale.x = viewportWorldMinX; // tile origin
                tileUVoffsetScale.y = viewportWorldMinY; // tile origin
                tileUVoffsetScale.z = viewportWorldSizeX; // tile size
                tileUVoffsetScale.w = viewportWorldSizeY; // tile size

                Vec4 regionUVoffsetScale;
                regionUVoffsetScale.x = (viewportWorldMinX - regionWorldMin.GetX()) / regionSize.GetX();
                regionUVoffsetScale.y = (viewportWorldMinY - regionWorldMin.GetY()) / regionSize.GetY();
                regionUVoffsetScale.z = viewportWorldSizeX / regionSize.GetX(); // worldTileSize / Size
                regionUVoffsetScale.w = viewportWorldSizeY / regionSize.GetY(); // worldTileSize / Size

                Vec4 quadVertices;
                quadVertices.x = (viewportWorldMinX - tileWorldMinMax.GetX()) / tileWorldSizeX;
                quadVertices.y = (viewportWorldMinY - tileWorldMinMax.GetY()) / tileWorldSizeY;
                quadVertices.z = (viewportWorldMaxX - tileWorldMinMax.GetX()) / tileWorldSizeX;
                quadVertices.w = (viewportWorldMaxY - tileWorldMinMax.GetY()) / tileWorldSizeY;

                // Set viewport for ping pong buffers
                gcpRendD3D->SetViewport(0, 0, requestInfo.m_physicalViewportDest.m_width, requestInfo.m_physicalViewportDest.m_height);

                int pingPongSource = 1;
                int pingPongTarget = 0;

                // including default material
                const int totalLayerCount = currentRegionMaterial.m_materialLayers.size() + 1;

                // Bind macro textures
                _smart_ptr<ITexture>& macroColorTex = currentRegionMaterial.m_macroMaterial.m_macroColorMap;
                _smart_ptr<ITexture>& macroNormalTex = currentRegionMaterial.m_macroMaterial.m_macroNormalMap;
                _smart_ptr<ITexture>& macroGlossTex = currentRegionMaterial.m_macroMaterial.m_macroGlossMap;

                // Bind macro material textures
                BindTexture(12, EHWShaderClass::eHWSC_Pixel, static_cast<CTexture*>(macroColorTex.get()), CTextureManager::Instance()->GetBlackTexture());
                BindTexture(13, EHWShaderClass::eHWSC_Pixel, static_cast<CTexture*>(macroNormalTex.get()), CTextureManager::Instance()->GetBlackTexture());
                BindTexture(14, EHWShaderClass::eHWSC_Pixel, static_cast<CTexture*>(macroGlossTex.get()), CTextureManager::Instance()->GetBlackTexture());

                for (int layerIndex = 0; layerIndex < totalLayerCount; ++layerIndex)
                {
                    IMaterial* currentLayerMaterial = nullptr;
                    ITexture* currentLayerSplatTexture = nullptr;

                    if (layerIndex == 0)
                    {
                        // Process the default material as the first layer
                        // Using a white texture for the splat map so it overwrites any existing texture data

                        currentLayerMaterial = currentRegionMaterial.m_defaultMaterial;
                        currentLayerSplatTexture = gcpRendD3D->GetWhiteTexture();
                    }
                    else
                    {
                        TerrainMaterialLayer& layer = currentRegionMaterial.m_materialLayers[layerIndex - 1];

                        currentLayerMaterial = layer.m_material;
                        currentLayerSplatTexture = layer.m_splatTexture;
                    }

                    if (currentLayerMaterial == nullptr
                        || currentLayerSplatTexture == nullptr)
                    {
                        AZ_Error("TerrainCompositer", currentLayerMaterial != nullptr, "TerrainCompositer::RenderTile | null material layer during compositing! region=[%d,%d]",
                            regionX, regionY);
                        AZ_Error("TerrainCompositer", currentLayerSplatTexture != nullptr, "TerrainCompositer::RenderTile | null splat texture during compositing! region=[%d,%d]",
                            regionX, regionY);
                        continue;
                    }

                    // No need for a backup texture for the splat map, we skip missing splat maps above
                    BindTexture(15, EHWShaderClass::eHWSC_Pixel, static_cast<CTexture*>(currentLayerSplatTexture), nullptr);

                    // Flip source and index
                    pingPongSource = pingPongTarget;
                    pingPongTarget = (pingPongTarget + 1) % 2;

                    m_compositeTempTextures[pingPongSource * 2]->ApplyTexture(10, eHWSC_Pixel);
                    m_compositeTempTextures[pingPongSource * 2 + 1]->ApplyTexture(11, eHWSC_Pixel);

                    if (layerIndex == (totalLayerCount - 1))
                    {
                        // Last splat, we do directly to the virtual texture
                        bool pushRenderTargetResult = false;
                        pushRenderTargetResult = gcpRendD3D->FX_PushRenderTarget(0, m_VirtualTexture->GetPhysicalTexture(0), nullptr);
                        AZ_Assert(pushRenderTargetResult, "Failed to push render target 0");
                        pushRenderTargetResult = gcpRendD3D->FX_PushRenderTarget(1, m_VirtualTexture->GetPhysicalTexture(1), nullptr);
                        AZ_Assert(pushRenderTargetResult, "Failed to push render target 1");
                        gcpRendD3D->SetViewport(requestInfo.m_physicalViewportDest.m_topLeftX, requestInfo.m_physicalViewportDest.m_topLeftY, requestInfo.m_physicalViewportDest.m_width, requestInfo.m_physicalViewportDest.m_height);
                        gcpRendD3D->FX_SetActiveRenderTargets();
                    }
                    else
                    {
                        // Continue ping pong
                        gcpRendD3D->FX_PushRenderTarget(0, m_compositeTempTextures[pingPongTarget * 2], nullptr);
                        gcpRendD3D->FX_PushRenderTarget(1, m_compositeTempTextures[pingPongTarget * 2 + 1], nullptr);
                        gcpRendD3D->FX_SetActiveRenderTargets();
                    }

                    RenderMaterial(currentLayerMaterial, currentRegionMaterial.m_macroMaterial, quadVertices, regionUVoffsetScale, tileUVoffsetScale);

                    gcpRendD3D->FX_PopRenderTarget(0);
                    gcpRendD3D->FX_PopRenderTarget(1);
                }

                gcpRendD3D->FX_SetActiveRenderTargets();
            }
        }

        // Cleanup, release handles to materials/textures
        for (RegionMaterials& currRegionMaterial : m_currentRegionMaterials)
        {
            currRegionMaterial.Clear();
        }

        // Restore prev state
        gcpRendD3D->m_RP.m_pShader = pPrevShader;
        gcpRendD3D->m_RP.m_pShaderResources = pPrevShaderResources;

        // Success!
        return true;
    }

    void TerrainCompositer::RenderMaterial(IMaterial* material, const MacroMaterial& regionMacroMaterial, Vec4 quadVertices, Vec4 regionUVoffsetScale, Vec4 tileUVoffsetScale)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
        AZ_Assert(material, "TerrainCompositer::RenderMaterial | material == nullptr");

        static CCryNameR varTileUVoffsetScale("tileUVoffsetScale");
        static CCryNameR varRegionUVoffsetScale("regionUVoffsetScale");

        SShaderItem& shaderItem = material->GetShaderItem();
        IShader* shader = shaderItem.m_pShader;

        uint32 numPasses = 0;
        gcpRendD3D->m_RP.m_pShader = static_cast<CShader*>(shaderItem.m_pShader);
        gcpRendD3D->m_RP.m_pShaderResources = static_cast<CShaderResources*>(shaderItem.m_pShaderResources);
        shader->FXSetTechnique("General");
        shader->FXBegin(&numPasses, 0);
        for (uint32_t p = 0; p < numPasses; ++p)
        {
            shader->FXSetVSFloat(varRegionUVoffsetScale, &regionUVoffsetScale, 1);
            shader->FXSetVSFloat(varTileUVoffsetScale, &tileUVoffsetScale, 1);

            BindMacroMaterialShaderConstants(regionMacroMaterial);

            shader->FXBeginPass(p);

            gcpRendD3D->SetCullMode(R_CULL_NONE);

            // Always use the same state
            // We don't have a free alpha channel on output, so we're blending in shader
            const int renderStateFlags = GS_BLSRC_ONE | GS_BLDST_ZERO | GS_NODEPTHTEST;
            gcpRendD3D->FX_SetState(renderStateFlags, -1, renderStateFlags);

            // Flag materials for commit
            gcpRendD3D->m_RP.m_nCommitFlags |= FC_MATERIAL_PARAMS;

            gcpRendD3D->FX_Commit();
            gcpRendD3D->DrawQuad(quadVertices.x, quadVertices.y, quadVertices.z, quadVertices.w, ColorF(1, 1, 1, 1), 1.0f, 0, 0, 1, 1);
            shader->FXEndPass();
        }
        shader->FXEnd();
    }

    void TerrainCompositer::SetupMacroMaterial(const AZ::Aabb& nodeAABB)
    {
        // Set macro textures
        AZ::Vector2 tileWorldMin(nodeAABB.GetMin().GetX(), nodeAABB.GetMin().GetY());
        AZ::Vector2 tileWorldMax(nodeAABB.GetMax().GetX(), nodeAABB.GetMax().GetY());

        int regionX, regionY;
        TerrainProviderRequestBus::Broadcast(&TerrainProviderRequestBus::Events::GetRegionIndex, tileWorldMin, tileWorldMax, regionX, regionY);

        RequestResult result = RequestResult::NoAssetsForRegion;
        MacroMaterial macroMaterial;
        Terrain::WorldMaterialRequestBus::BroadcastResult(result, &Terrain::WorldMaterialRequestBus::Events::GetMacroMaterial, regionX, regionY, macroMaterial);

        // Bind macro material textures
        BindTexture(8, EHWShaderClass::eHWSC_Pixel, static_cast<CTexture*>(macroMaterial.m_macroColorMap.get()), CTextureManager::Instance()->GetWhiteTexture());

        // Our shaders assume normals maps are sign normalized, so a black texture as the default works well here.
        // The zero-length degenerate normals from sampling the black texture end up having no net effect on the
        // material layers' normal map, which is the result we want.
        BindTexture(9, EHWShaderClass::eHWSC_Pixel, static_cast<CTexture*>(macroMaterial.m_macroNormalMap.get()), CTextureManager::Instance()->GetBlackTexture());

        // Defaults to no gloss
        BindTexture(10, EHWShaderClass::eHWSC_Pixel, static_cast<CTexture*>(macroMaterial.m_macroGlossMap.get()), CTextureManager::Instance()->GetBlackTexture());

        BindMacroMaterialShaderConstants(macroMaterial);
    }

    bool TerrainCompositer::BeginRendering(const AZ::Aabb& nodeAABB)
    {
        const TerrainRenderingParameters* pTerrainParams = TerrainRenderingParameters::GetInstance();
        const int terrainTextureSamplerStateId = pTerrainParams->m_cachedTerrainTextureSamplerStateId;

        return m_VirtualTexture->BeginRendering(eHWSC_Pixel, terrainTextureSamplerStateId, nodeAABB);
    }
}

#endif
