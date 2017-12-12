/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include "SceneGBuffer.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include "CryUtils.h"

#include "Common/Include_HLSL_CPP_Shared.h"
#include "Common/GraphicsPipelineStateSet.h"
#include "Common/TypedConstantBuffer.h"

#include "Common/RenderView.h"
#include "CompiledRenderObject.h"

CSceneGBufferPass* g_pGBufferPassPtr = 0; // Just a HACK. remove later.

void CSceneGBufferPass::Init()
{
    m_pPerPassResources = CDeviceObjectFactory::GetInstance().CreateResourceSet(CDeviceResourceSet::EFlags_ForceSetAllState);
    m_pResourceLayout = CDeviceObjectFactory::GetInstance().CreateResourceLayout();

    m_bValidResourceLayout = false;
}

void CSceneGBufferPass::Shutdown()
{
    m_pPerPassResources = nullptr;
    m_pResourceLayout = nullptr;
}

void CSceneGBufferPass::Reset()
{
    
}

void CSceneGBufferPass::OnResolutionChanged()
{
    CD3D9Renderer* rd = gcpRendD3D;

    int width = rd->m_MainViewport.nWidth;
    int height = rd->m_MainViewport.nHeight;

    if (!CTexture::s_ptexZTarget
        || CTexture::s_ptexZTarget->IsMSAAChanged()
        || CTexture::s_ptexZTarget->GetDstFormat() != CTexture::s_eTFZ
        || CTexture::s_ptexZTarget->GetWidth() != width
        || CTexture::s_ptexZTarget->GetHeight() != height)
    {
        rd->FX_Commit(); // Flush to unset the Z target before regenerating
        CTexture::GenerateZMaps();
    }
}

void CSceneGBufferPass::ProcessRenderList(ERenderListID list, SGraphicsPiplinePassContext& passContext)
{
    CD3D9Renderer* pRenderer = gcpRendD3D;
    SRenderPipeline& rp = pRenderer->m_RP;

    rp.m_nPassGroupID = list;
    rp.m_nPassGroupDIP = list;

    passContext.renderListId = list;

    passContext.passId = RS_TECH_GBUFPASS;

    if (CRenderer::CV_r_GraphicsPipeline >= 4)
    {
        // Duplicate pass for the individual command list per fill thread
        SGraphicsPiplinePassContext passContext0(passContext);
        SGraphicsPiplinePassContext passContext1(passContext);

        passContext0.sortGroupID = 0;
        passContext0.rendItems.start = rp.m_pRLD->m_nStartRI[passContext0.sortGroupID][passContext0.renderListId];
        passContext0.rendItems.end = rp.m_pRLD->m_nEndRI[passContext0.sortGroupID][passContext0.renderListId];

        passContext1.sortGroupID = 1;
        passContext1.rendItems.start = rp.m_pRLD->m_nStartRI[passContext1.sortGroupID][passContext1.renderListId];
        passContext1.rendItems.end = rp.m_pRLD->m_nEndRI[passContext1.sortGroupID][passContext1.renderListId];

        gcpRendD3D->FX_SetActiveRenderTargets(true);
        CHWShader_D3D::mfCommitParamsGlobal();

        // TODO: move out of draw-loop into once-per-frame prepare-loop
        pRenderer->PrepareRenderItems(passContext0);
        pRenderer->PrepareRenderItems(passContext1);

        // TODO: these two calls could be concurrently threaded
        pRenderer->DrawRenderItems(passContext0);
        pRenderer->DrawRenderItems(passContext1);
        return;
    }


    pRenderer->FX_SetActiveRenderTargets(true);
    CHWShader_D3D::mfCommitParamsGlobal();

    rp.m_nSortGroupID = 0;
    ProcessBatchesList(rp.m_pRLD->m_nStartRI[rp.m_nSortGroupID][list], rp.m_pRLD->m_nEndRI[rp.m_nSortGroupID][list], passContext.batchFilter, passContext);
    rp.m_nSortGroupID = 1;
    ProcessBatchesList(rp.m_pRLD->m_nStartRI[rp.m_nSortGroupID][list], rp.m_pRLD->m_nEndRI[rp.m_nSortGroupID][list], passContext.batchFilter, passContext);
}


void CSceneGBufferPass::ProcessBatchesList(int nums, int nume, uint32 nBatchFilter, SGraphicsPiplinePassContext& passContext)
{
    PROFILE_FRAME(GBuffer_ProcessBatchesList);

    if (nume - nums == 0)
    {
        return;
    }

    CD3D9Renderer* rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;

    CDeviceGraphicsCommandListPtr pCommandList = CDeviceObjectFactory::GetInstance().GetCoreGraphicsCommandList();
    PrepareCommandList(*pCommandList);

    auto& renderItems = CRenderView::CurrentRenderView()->GetRenderItems(rRP.m_nSortGroupID, rRP.m_nPassGroupID);

    rRP.m_nBatchFilter = nBatchFilter;
    rRP.m_nCommitFlags = FC_ALL;
    rd->FX_Commit();

    CShader* pShader = NULL;
    CShaderResources* pRes = NULL;
    CShaderResources* pPrevRes = NULL;
    CRenderObject* pPrevObject = NULL;
    int nTech;

    for (int i = nums; i < nume; i++)
    {
        SRendItem& ri = renderItems[i];
        if (!(ri.nBatchFlags & nBatchFilter))
        {
            continue;
        }

        CRenderObject* pObject = ri.pObj;
        CRendElementBase* pRE = ri.pElem;

        SRendItem::mfGet(ri.SortVal, nTech, pShader, pRes);

        if (pPrevObject != pObject)
        {
            if (pObject->m_ObjFlags & FOB_NEAREST && CRenderer::CV_r_nodrawnear > 0)
            {
                continue;
            }

            rRP.m_pCurObject = pObject;

            if (pObject != rRP.m_pIdendityRenderObject)
            {
                int flags = (pObject->m_ObjFlags & FOB_NEAREST) ? RBF_NEAREST : 0;
                if ((flags ^ rRP.m_Flags) & RBF_NEAREST)
                {
                    rd->UpdateNearestChange(flags);
                }
            }
            else
            {
                rd->HandleDefaultObject();
            }

            if (rRP.m_nCommitFlags & FC_GLOBAL_PARAMS)
            {
                CHWShader_D3D::mfCommitParamsGlobal();
                rRP.m_nCommitFlags &= ~FC_GLOBAL_PARAMS;
            }
            rd->FX_SetViewport();

            if ((pObject->m_ObjFlags & FOB_HAS_PREVMATRIX) && CRenderer::CV_r_MotionVectors && CRenderer::CV_r_MotionBlurGBufferVelocity)
            {
                if (!(rRP.m_PersFlags2 & RBPF2_MOTIONBLURPASS))
                {
                    rRP.m_PersFlags2 |= RBPF2_MOTIONBLURPASS;
                    rd->FX_PushRenderTarget(3, GetUtils().GetVelocityObjectRT(), NULL);
                    rd->FX_SetActiveRenderTargets();
                }
            }

            pPrevObject = pObject;
        }

#if 1
        // Update initialized or outdated resources
        if (pRes->m_pDeformInfo)
        {
            pRes->Rebuild(pShader);
        }
        if (pRes->m_pCompiledResourceSet->IsDirty())
        {
            pRes->m_pCompiledResourceSet->Build();
        }

        SShaderItem shaderItem;
        shaderItem.m_nTechnique = nTech;
        shaderItem.m_pShader = pShader;
        shaderItem.m_pShaderResources = pRes;

        static CCompiledRenderObject s_compiledObject;
        s_compiledObject.m_pRenderElement = pRE;
        s_compiledObject.m_shaderItem = shaderItem;

        pObject->m_bInstanceDataDirty = false;  // Enforce recompilation of entire object
        if (s_compiledObject.Compile(pObject, rRP.m_TI[rRP.m_nProcessThreadID].m_RealTime))
        {
            s_compiledObject.DrawAsync(*pCommandList, passContext);
        }
#else
        rRP.m_pShader = pShader;
        rRP.m_pShaderResources = pRes;
        rRP.m_CurVFormat = pShader->m_eVertexFormat;
        rRP.m_pRE = NULL;
        rRP.m_nShaderTechnique = nTech;

        rRP.m_nNumRendPasses = rRP.m_FirstIndex = rRP.m_FirstVertex = rRP.m_RendNumIndices = rRP.m_RendNumVerts = 0;
        rRP.m_RendNumGroup = -1;

        DrawObject(pCommandList, pRE, pPrevRes);
#endif
    }
}

void CSceneGBufferPass::DrawObject(CDeviceGraphicsCommandListPtr pCommandList, CRendElementBase* pRE, CShaderResources*& pPrevResources)
{
#ifdef CRY_INTEGRATE_DX12
    CD3D9Renderer* const __restrict pRenderer = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE renderPipeline = pRenderer->m_RP;

    CShader* pShader = renderPipeline.m_pShader;
    CRenderObject* pObj = renderPipeline.m_pCurObject;

    if (pRE->mfGetType() == eDATA_GeomCache)  // TODO: Geom caches currently can crash
    {
        return;
    }

    //pRE->mfPrepare(true);
    //
    //if (!renderPipeline.m_pRE && !renderPipeline.m_RendNumVerts || !pShader)
    //  return;

    renderPipeline.m_pRE = pRE;

    PROFILE_SHADER_SCOPE;

    SShaderTechnique* __restrict pTech = pShader->mfGetStartTechnique(renderPipeline.m_nShaderTechnique);
    EShaderTechniqueID nTechniqueID = TTYPE_Z;
    if (!pTech || pTech->m_nTechnique[nTechniqueID] < 0)
    {
        return;
    }

    renderPipeline.m_nShaderTechniqueType = nTechniqueID;
    renderPipeline.m_pRootTechnique = pTech;

    assert(pTech->m_nTechnique[nTechniqueID] < (int)pShader->m_HWTechniques.Num());
    pTech = pShader->m_HWTechniques[pTech->m_nTechnique[nTechniqueID]];

    renderPipeline.m_pCurTechnique = pTech;

    assert(pTech->m_Passes.Num() == 1);
    SShaderPass* pPass = &pTech->m_Passes[0];
    renderPipeline.m_pCurPass = pPass;

    // Get pipeline state for item and render
    CShaderResources* pResources = renderPipeline.m_pShaderResources;
    if (!pResources->m_pipelineStateCache)
    {
        pResources->m_pipelineStateCache = std::make_shared<CGraphicsPipelineStateLocalCache>();
    }

    {
        CRendElementBase::SGeometryInfo geomInfo;
        if (pRE->GetGeometryInfo(geomInfo))
        {
            SShaderItem shaderItem;
            shaderItem.m_nTechnique = renderPipeline.m_nShaderTechnique;
            shaderItem.m_pShader = pShader;
            shaderItem.m_pShaderResources = pResources;

            SGraphicsPipelineStateDescription psoDescription(
                pObj,
                shaderItem, nTechniqueID,
                geomInfo.vertexFormat, geomInfo.streamMask, geomInfo.primitiveType
                );

            psoDescription.objectRuntimeMask |= g_HWSR_MaskBit[HWSR_PER_INSTANCE_CB_TEMP];

            static DevicePipelineStatesArray psoArray[RS_TECH_NUM];
            psoArray[RS_TECH_GBUFPASS][0] = NULL;
            pRenderer->GetGraphicsPipeline().CreatePipelineStates(psoArray, psoDescription, pResources->m_pipelineStateCache.get());
            CDeviceGraphicsPSOPtr pso = psoArray[RS_TECH_GBUFPASS][0];

            if (pso)
            {
                pCommandList->SetPipelineState(pso);

                if (pResources != pPrevResources)
                {
                    // Update initialized or outdated resources
                    if (pResources->m_pDeformInfo)
                    {
                        pResources->Rebuild(pShader);
                    }
                    if (pResources->m_pCompiledResourceSet->IsDirty())
                    {
                        pResources->m_pCompiledResourceSet->Build();
                    }

                    pCommandList->SetResources(EResourceLayoutSlot_PerMaterialRS, pResources->m_pCompiledResourceSet.get());
                    pPrevResources = pResources;
                }

                uint8 stencilRef = 0;
                stencilRef = CRenderer::CV_r_VisAreaClipLightsPerPixel ? 0 : (stencilRef | BIT_STENCIL_INSIDE_CLIPVOLUME);
                stencilRef |= !(pObj->m_ObjFlags & FOB_DYNAMIC_OBJECT) ? BIT_STENCIL_RESERVED : 0;
                pCommandList->SetStencilRef(stencilRef);

                //CHWShader_D3D::s_pCurVS = ((CHWShader_D3D::SHWSInstance *)pso->m_pHwShaderInstances[eHWSC_Vertex])->m_Handle.m_pShader;
                //CHWShader_D3D::s_pCurInstVS = (CHWShader_D3D::SHWSInstance *)pso->m_pHwShaderInstances[eHWSC_Vertex];
                //CHWShader_D3D::s_pCurPS = ((CHWShader_D3D::SHWSInstance *)pso->m_pHwShaderInstances[eHWSC_Pixel])->m_Handle.m_pShader;
                //CHWShader_D3D::s_pCurInstPS = (CHWShader_D3D::SHWSInstance *)pso->m_pHwShaderInstances[eHWSC_Pixel];
                //
                //CHWShader_D3D* pCurVS = (CHWShader_D3D *)pPass->m_VShader;
                //CHWShader_D3D* pCurPS = (CHWShader_D3D *)pPass->m_PShader;
                //pCurVS->m_pCurInst = CHWShader_D3D::s_pCurInstVS;
                //pCurPS->m_pCurInst = CHWShader_D3D::s_pCurInstPS;
                //pCurVS->mfSetParametersPB();
                //pCurPS->mfSetParametersPB();
                //pCurVS->mfSetParametersPI();
                //pCurPS->mfSetParametersPI();
                //CHWShader_D3D::mfCommitParams();
                //
                //if (pso->m_pHwShaderInstances[eHWSC_Hull])
                //{
                //  CHWShader_D3D::s_pCurInstHS = (CHWShader_D3D::SHWSInstance *)pso->m_pHwShaderInstances[eHWSC_Hull];
                //}

                #if 0
                // Fill Per-Instance constant buffer
                {
                    static AzRHI::ConstantBufferPtr pPerInstConstantBuffer;
                    CTypedConstantBuffer<HLSL_PerInstanceConstantBuffer> cb(pPerInstConstantBuffer);
                    pPerInstConstantBuffer = cb.GetDeviceConstantBuffer();

                    cb->SPIObjWorldMat = pObj->m_II.m_Matrix;
                    cb->SPIAlphaTest[0] = pObj->m_DissolveRef * (1.0f / 255.0f);
                    cb->SPIAlphaTest[1] = (pObj->m_ObjFlags & FOB_DISSOLVE_OUT) ? 1.0f : -1.0f;
                    cb->SPIAlphaTest[2] = CRenderer::CV_r_ssdoAmountDirect;
                    cb->SPIAlphaTest[3] = pResources->m_AlphaRef;

                    float realTime = renderPipeline.m_TI[renderPipeline.m_nProcessThreadID].m_RealTime;
                    if (pObj->m_data.m_pBending)
                    {
                        pObj->m_data.m_pBending->GetShaderConstantsStatic(realTime, (Vec4*)cb->SPIBendInfo);
                    }
                    else
                    {
                        cb->SPIBendInfo[0] = cb->SPIBendInfo[1] = Vec4(ZERO);
                    }
                    /*
                    CRY DX12 TEMP DISABLE
                    if (pObj->m_data.m_pTerrainSectorTextureInfo)
                    {
                        // Fill terrain texture info if present
                        cb->Varying0[0] = pObj->m_data.m_pTerrainSectorTextureInfo->fTexOffsetX;
                        cb->Varying0[1] = pObj->m_data.m_pTerrainSectorTextureInfo->fTexOffsetY;
                        cb->Varying0[2] = pObj->m_data.m_pTerrainSectorTextureInfo->fTexScale;
                        cb->Varying0[3] = pObj->m_data.m_fMaxViewDistance; // Obj view max distance
                    }*/

                    cb.CopyToDevice();

                    pCommandList->SetInlineConstantBuffer(EResourceLayoutSlot_PerInstanceCB, cb.GetDeviceConstantBuffer(), eConstantBufferShaderSlot_PerInstance, EShaderStage_Vertex | EShaderStage_Pixel);
                }
                #endif

                //renderPipeline.m_FlagsStreams_Decl = CHWShader_D3D::s_pCurInstVS->m_VStreamMask_Decl;
                //renderPipeline.m_FlagsStreams_Stream = CHWShader_D3D::s_pCurInstVS->m_VStreamMask_Stream;

                //if (!pRE->mfCheckUpdate(rRP.m_CurVFormat, rRP.m_FlagsStreams_Stream | 0x80000000, nFrameID, bTessEnabled))
                //  continue;

                //pRenderer->FX_CommitStreams(pPass);

                uint32 bonesRemapGUID = 0;
                if ((pObj->m_ObjFlags & FOB_SKINNED) && CRenderer::CV_r_usehwskinning && !CRenderer::CV_r_character_nodeform)
                {
                    SSkinningData* pSkinningData = NULL;
                    if (SRenderObjData* pOD = pObj->GetObjData())
                    {
                        pSkinningData = pOD->m_pSkinningData;
                    }

                    if (pSkinningData)
                    {
                        AzRHI::ConstantBuffer* pBuffer[2] = { NULL };
                        //((CREMeshImpl *)pRE)->BindRemappedSkinningData(pSkinningData->remapGUID);
                        bonesRemapGUID = pSkinningData->remapGUID;

                        static CDeviceResourceSetPtr perInstanceExtraResources = CDeviceObjectFactory::GetInstance().CreateResourceSet();
                        perInstanceExtraResources->Clear();

                        // TODO: only bind to hs and ds stages when tessellation is enabled
                        const EShaderStage shaderStages = EShaderStage_Vertex | EShaderStage_Hull | EShaderStage_Domain;

                        CD3D9Renderer::SCharInstCB* pCurSkinningData  = alias_cast<CD3D9Renderer::SCharInstCB*>(pSkinningData->pCharInstCB);
                        CD3D9Renderer::SCharInstCB* pPrevSkinningData = nullptr;

                        if (pSkinningData->pPreviousSkinningRenderData)
                        {
                            pPrevSkinningData = alias_cast<CD3D9Renderer::SCharInstCB*>(pSkinningData->pPreviousSkinningRenderData->pCharInstCB);
                        }

                        perInstanceExtraResources->SetConstantBuffer(eConstantBufferShaderSlot_SkinQuat,     pCurSkinningData  ? pCurSkinningData->m_buffer  : nullptr, shaderStages);
                        perInstanceExtraResources->SetConstantBuffer(eConstantBufferShaderSlot_SkinQuatPrev, pPrevSkinningData ? pPrevSkinningData->m_buffer : nullptr, shaderStages);

                        WrappedDX11Buffer* pExtraWeights = reinterpret_cast<WrappedDX11Buffer*>(geomInfo.pSkinningExtraBonesBuffer);
                        perInstanceExtraResources->SetBuffer(EReservedTextureSlot_SkinExtraWeights, pExtraWeights ? *pExtraWeights : WrappedDX11Buffer(), shaderStages);

                        WrappedDX11Buffer* pAdjacencyBuffer = reinterpret_cast<WrappedDX11Buffer*>(geomInfo.pTessellationAdjacencyBuffer);
                        perInstanceExtraResources->SetBuffer(EReservedTextureSlot_AdjacencyInfo, pAdjacencyBuffer ? *pAdjacencyBuffer : WrappedDX11Buffer(), shaderStages);

                        perInstanceExtraResources->Build();

                        pCommandList->SetResources(EResourceLayoutSlot_PerInstanceExtraRS, perInstanceExtraResources.get());
                    }
                }

                //pRE->mfDraw(pShader, pPass);

                CRendElementBase::SGeometryInfo geomInfo;
                geomInfo.bonesRemapGUID = bonesRemapGUID;
                if (pRE->GetGeometryInfo(geomInfo))
                {
                    pCommandList->SetVertexBuffers(geomInfo.nMaxVertexStreams, (SStreamInfo*)geomInfo.vertexStream);

                    if (geomInfo.indexStream.pStream != nullptr)
                    {
                        pCommandList->SetIndexBuffer(*(SStreamInfo*)&geomInfo.indexStream);
                        pCommandList->DrawIndexed(geomInfo.nNumIndices, 1, geomInfo.nFirstIndex, 0, 0);
                    }
                    else
                    {
                        pCommandList->Draw(geomInfo.nNumVertices, 1, 0, 0);
                    }
                }

                //CHWShader_D3D::s_pCurInstHS = NULL;
            }
        }
    }
#endif
}


ILINE uint64 CSceneGBufferPass::GetPSOCacheKey(uint32 objFlags, int shaderID, int resID, bool bZPrePass)
{
    uint64 keyHigh = objFlags;
    uint32 keyLow = (resID << 18) | (shaderID << 6) | (bZPrePass ? 0x4 : 0);
    uint64 key = (keyHigh << 32) | keyLow;

    return key;
}

CDeviceGraphicsPSOPtr CSceneGBufferPass::CreatePipelineState(const SGraphicsPipelineStateDescription& desc, int psoId)
{
    // TODO: Handle quality and MSAA flags if required

    CDeviceGraphicsPSODesc psoDesc(m_pResourceLayout.get(), desc);

    CShaderResources* pRes = static_cast<CShaderResources*>(desc.shaderItem.m_pShaderResources);
    SShaderTechnique* pTechnique = psoDesc.m_pShader->GetTechnique(desc.shaderItem.m_nTechnique, desc.technique);
    const uint64 objectFlags = desc.objectFlags;

    if (!pTechnique)
    {
        return nullptr;
    }
    SShaderPass* pShaderPass = &pTechnique->m_Passes[0];

    psoDesc.m_RenderState  = pShaderPass->m_RenderState;

    psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_QUALITY] | g_HWSR_MaskBit[HWSR_QUALITY1];

    SThreadInfo* const pShaderThreadInfo = &(gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID]);
    if (pShaderThreadInfo->m_PersFlags & RBPF_REVERSE_DEPTH)
    {
        psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_REVERSE_DEPTH];
    }

    // Set resource states
    bool bTwoSided = false;
    {
        if (pRes->m_ResFlags & MTL_FLAG_2SIDED)
        {
            bTwoSided = true;
        }

        if (pRes->IsAlphaTested())
        {
            psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_ALPHATEST];
        }

        if (pRes->m_Textures[EFTT_DIFFUSE] && pRes->m_Textures[EFTT_DIFFUSE]->m_Ext.m_pTexModifier)
        {
            psoDesc.m_ShaderFlags_MD |= pRes->m_Textures[EFTT_DIFFUSE]->m_Ext.m_nUpdateFlags;
        }

        if (pRes->m_pDeformInfo)
        {
            psoDesc.m_ShaderFlags_MDV |= pRes->m_pDeformInfo->m_eType;
        }
    }

    psoDesc.m_ShaderFlags_MDV |= psoDesc.m_pShader->m_nMDV;

    if (objectFlags & FOB_OWNER_GEOMETRY)
    {
        psoDesc.m_ShaderFlags_MDV &= ~MDV_DEPTH_OFFSET;
    }

    if (objectFlags & FOB_BENDED)
    {
        psoDesc.m_ShaderFlags_MDV |= MDV_BENDING;
    }

    if ((objectFlags & FOB_BLEND_WITH_TERRAIN_COLOR) /*&& rRP.m_pCurObject->m_nTextureID > 0*/)
    {
        psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_BLEND_WITH_TERRAIN_COLOR];
    }

    psoDesc.m_bAllowTesselation = false;
    psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_NO_TESSELLATION];

#ifdef TESSELLATION_RENDERER
    const bool bHasTesselationShaders = pShaderPass && pShaderPass->m_HShader && pShaderPass->m_DShader;
    if (bHasTesselationShaders && (!(objectFlags & FOB_NEAREST) && (objectFlags & FOB_ALLOW_TESSELLATION)))
    {
        psoDesc.m_ShaderFlags_RT &= ~g_HWSR_MaskBit[HWSR_NO_TESSELLATION];
        psoDesc.m_bAllowTesselation = true;
    }
#endif

    if ((objectFlags & FOB_HAS_PREVMATRIX) && CRenderer::CV_r_MotionVectors && CRenderer::CV_r_MotionBlurGBufferVelocity)
    {
        psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_MOTION_BLUR];
        if ((objectFlags & FOB_MOTION_BLUR) && !(objectFlags & FOB_SKINNED))
        {
            psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_VERTEX_VELOCITY];
        }
    }

    // from EF_AddEf_HandleOldRTMask
    if (objectFlags & (FOB_NEAREST | FOB_DECAL_TEXGEN_2D | FOB_DISSOLVE | FOB_GLOBAL_ILLUMINATION | FOB_SOFT_PARTICLE))
    {
        if (objectFlags & FOB_DECAL_TEXGEN_2D)
        {
            psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_2D];
        }

        if (objectFlags & FOB_NEAREST)
        {
            psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_NEAREST];
        }

        if (objectFlags & FOB_DISSOLVE)
        {
            psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_DISSOLVE];
        }

        if (objectFlags & FOB_GLOBAL_ILLUMINATION)
        {
            psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_GLOBAL_ILLUMINATION];
        }

        if (CRenderer::CV_r_ParticlesSoftIsec && (objectFlags & FOB_SOFT_PARTICLE))
        {
            psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_SOFT_PARTICLE];
        }
    }

    // enable stencil in zpass for vis area and dynamic object markup
    if (desc.technique != TTYPE_ZPREPASS)
    {
        psoDesc.m_RenderState |= GS_STENCIL;
        psoDesc.m_StencilState =
            STENC_FUNC(FSS_STENCFUNC_ALWAYS)
            | STENCOP_FAIL(FSS_STENCOP_KEEP)
            | STENCOP_ZFAIL(FSS_STENCOP_KEEP)
            | STENCOP_PASS(FSS_STENCOP_REPLACE);
        psoDesc.m_StencilReadMask = 0xFF & (~BIT_STENCIL_RESERVED);
        psoDesc.m_StencilWriteMask = 0xFF;
    }

    if (psoDesc.m_RenderState & GS_ALPHATEST_MASK)
    {
        psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_ALPHATEST];
    }

    psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_PER_INSTANCE_CB_TEMP];

    // Disable alpha testing/depth writes if object renders using a z-prepass and requested technique is not z pre pass
    /*if ((objectFlags & FOB_ZPREPASS) && (desc.technique != TTYPE_ZPREPASS))
    {
        psoDesc.m_RenderState &= ~(GS_DEPTHWRITE | GS_DEPTHFUNC_MASK | GS_ALPHATEST_MASK);
        psoDesc.m_RenderState |= GS_DEPTHFUNC_EQUAL;
        psoDesc.m_ShaderFlags_RT &= ~g_HWSR_MaskBit[HWSR_ALPHATEST];
    }*/

    psoDesc.m_CullMode = bTwoSided ? eCULL_None : (pShaderPass->m_eCull != -1 ? (ECull)pShaderPass->m_eCull : eCULL_Back);
    psoDesc.m_PrimitiveType = eRenderPrimitiveType(desc.primitiveType);

    if (psoDesc.m_bAllowTesselation)
    {
        psoDesc.m_PrimitiveType = ept3ControlPointPatchList;
        psoDesc.m_ObjectStreamMask |= VSM_NORMALS;
    }

    RenderTargetList renderTargets;
    SDepthTexture depthTarget;
    GetRenderTargets(renderTargets, depthTarget);

    D3D11_TEXTURE2D_DESC depthTargetDesc;
    depthTarget.pTarget->GetDesc(&depthTargetDesc);
    psoDesc.m_DepthStencilFormat = CTexture::TexFormatFromDeviceFormat(depthTargetDesc.Format);

    for (int i = 0; i < renderTargets.size(); ++i)
    {
        psoDesc.m_RenderTargetFormats[i] = renderTargets[i]->GetTextureDstFormat();
    }

    psoDesc.Build();

    return CDeviceObjectFactory::GetInstance().CreateGraphicsPSO(psoDesc);
}

int CSceneGBufferPass::CreatePipelineStates(DevicePipelineStatesArray& out, const SGraphicsPipelineStateDescription& stateDesc, CGraphicsPipelineStateLocalCache* pStateCache)
{
    auto cachedStates = pStateCache->Find(stateDesc);
    if (cachedStates)
    {
        out = *cachedStates;
        return 1; //out[eSubPassZPrePass] ? 2 : 1;
    }

    // Z Pass.
    out[eSubPassZPass] = CreatePipelineState(stateDesc);
    if (!out[eSubPassZPass])
    {
        return 0;
    }

    /*// If ZPrepass needed.
    if (description.objectFlags & FOB_ZPREPASS)
    {
        // Enable z-prepass state for an object.
        SGraphicsPipelineStateDescription tempDescription(description);
        tempDescription.objectRuntimeMask |= g_HWSR_MaskBit[HWSR_PER_INSTANCE_CB_TEMP]; // Enable flag to use special per instance constant buffer
        out[eSubPassZPrePass] = CreatePipelineState(description);
        if (!out[eSubPassZPrePass])
        {
            return 0;
        }
    }*/

    pStateCache->Put(stateDesc, out);
    return 1;
}

void CSceneGBufferPass::RenderSceneOpaque()
{
    CD3D9Renderer* rd = gcpRendD3D;

    uint32 bfGeneral = SRendItem::BatchFlags(EFSLIST_GENERAL, rd->m_RP.m_pRLD);
    uint32 bfSkin = SRendItem::BatchFlags(EFSLIST_SKIN, rd->m_RP.m_pRLD);
    uint32 bfTransp = SRendItem::BatchFlags(EFSLIST_TRANSP, rd->m_RP.m_pRLD);

    g_pGBufferPassPtr = this; // Remove it

    uint32 filter = FB_Z;
    EShaderTechniqueID tech = (filter & FB_Z) ? TTYPE_Z : TTYPE_ZPREPASS;
    SGraphicsPiplinePassContext passContext(this, tech, FB_Z);
    passContext.nProcessThreadID = rd->m_RP.m_nProcessThreadID;
    passContext.nFrameID = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameID;
    passContext.passSubId = eSubPassZPass;
    passContext.pPipelineStats = &rd->m_RP.m_PS[passContext.nProcessThreadID];

    // Do Z-PrePass
    {
        //SGraphicsPiplinePassContext zPrePassContext(passContext);
        //zPrePassContext.batchFilter = FB_ZPREPASS;
        //zPrePassContext.passSubId = eSubPassZPrePass;
        //ProcessRenderList(EFSLIST_GENERAL, passContext);
    }

    if (bfGeneral & FB_Z)
    {
        ProcessRenderList(EFSLIST_GENERAL, passContext);
    }
    if (bfSkin & FB_Z)
    {
        ProcessRenderList(EFSLIST_SKIN, passContext);
    }
    if (bfTransp & FB_Z)
    {
        ProcessRenderList(EFSLIST_TRANSP, passContext);
    }

    // Restore state
    rd->GetGraphicsPipeline().ResetRenderState();
}

void CSceneGBufferPass::RenderSceneOverlays()
{
    if (CRenderer::CV_r_OldBackendSkip)
    {
        return;
    }

    CD3D9Renderer* rd = gcpRendD3D;

    uint32 bfDecal = SRendItem::BatchFlags(EFSLIST_DECAL, rd->m_RP.m_pRLD);
    uint32 bfTerrainLayer = SRendItem::BatchFlags(EFSLIST_TERRAINLAYER, rd->m_RP.m_pRLD);

    rd->m_RP.m_pRenderFunc = CD3D9Renderer::FX_FlushShader_ZPass;

    rd->m_RP.m_PersFlags2 &= ~RBPF2_NOALPHABLEND;
    rd->m_RP.m_StateAnd |= GS_BLEND_MASK;

    uint32 filter = FB_Z;
    EShaderTechniqueID tech = (filter & FB_Z) ? TTYPE_Z : TTYPE_ZPREPASS;
    SGraphicsPiplinePassContext passContext(this, tech, FB_Z);
    passContext.nProcessThreadID = rd->m_RP.m_nProcessThreadID;
    passContext.passSubId = eSubPassZPass;
    passContext.nFrameID = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameID;
    passContext.pPipelineStats = &rd->m_RP.m_PS[passContext.nProcessThreadID];

    if (bfTerrainLayer & FB_Z)
    {
        PROFILE_LABEL_SCOPE("TERRAIN_LAYERS");
        rd->m_RP.m_nPassGroupID = rd->m_RP.m_nPassGroupDIP = EFSLIST_TERRAINLAYER;
        rd->m_RP.m_nSortGroupID = 0;
        rd->FX_ProcessBatchesList(rd->m_RP.m_pRLD->m_nStartRI[rd->m_RP.m_nSortGroupID][EFSLIST_TERRAINLAYER], rd->m_RP.m_pRLD->m_nEndRI[rd->m_RP.m_nSortGroupID][EFSLIST_TERRAINLAYER], filter);
        rd->m_RP.m_nSortGroupID = 1;
        rd->FX_ProcessBatchesList(rd->m_RP.m_pRLD->m_nStartRI[rd->m_RP.m_nSortGroupID][EFSLIST_TERRAINLAYER], rd->m_RP.m_pRLD->m_nEndRI[rd->m_RP.m_nSortGroupID][EFSLIST_TERRAINLAYER], filter);
    }

    if (bfDecal & FB_Z)
    {
        PROFILE_LABEL_SCOPE("DECALS");
        rd->m_RP.m_nPassGroupID = rd->m_RP.m_nPassGroupDIP = EFSLIST_DECAL;
        rd->m_RP.m_nSortGroupID = 0;
        rd->FX_ProcessBatchesList(rd->m_RP.m_pRLD->m_nStartRI[rd->m_RP.m_nSortGroupID][EFSLIST_DECAL], rd->m_RP.m_pRLD->m_nEndRI[rd->m_RP.m_nSortGroupID][EFSLIST_DECAL], filter);
        rd->m_RP.m_nSortGroupID = 1;
        rd->FX_ProcessBatchesList(rd->m_RP.m_pRLD->m_nStartRI[rd->m_RP.m_nSortGroupID][EFSLIST_DECAL], rd->m_RP.m_pRLD->m_nEndRI[rd->m_RP.m_nSortGroupID][EFSLIST_DECAL], filter);
    }
}

void CSceneGBufferPass::PreparePerPassResources()
{
    CD3D9Renderer* rd = gcpRendD3D;

    m_pPerPassResources->Clear();

    // samplers
    {
        int materialSamplers[] =
        {
            gcpRendD3D->m_nMaterialAnisoHighSampler,                                                           // EFSS_ANISO_HIGH
            gcpRendD3D->m_nMaterialAnisoLowSampler,                                                            // EFSS_ANISO_LOW
            CTexture::GetTexState(STexState(FILTER_TRILINEAR, TADDR_WRAP,   TADDR_WRAP,   TADDR_WRAP,   0x0)), // EFSS_TRILINEAR
            CTexture::GetTexState(STexState(FILTER_BILINEAR,  TADDR_WRAP,   TADDR_WRAP,   TADDR_WRAP,   0x0)), // EFSS_BILINEAR
            CTexture::GetTexState(STexState(FILTER_TRILINEAR, TADDR_CLAMP,  TADDR_CLAMP,  TADDR_CLAMP,  0x0)), // EFSS_TRILINEAR_CLAMP
            CTexture::GetTexState(STexState(FILTER_BILINEAR,  TADDR_CLAMP,  TADDR_CLAMP,  TADDR_CLAMP,  0x0)), // EFSS_BILINEAR_CLAMP
            gcpRendD3D->m_nMaterialAnisoSamplerBorder,                                                         // EFSS_ANISO_HIGH_BORDER
            CTexture::GetTexState(STexState(FILTER_TRILINEAR, TADDR_BORDER, TADDR_BORDER, TADDR_BORDER, 0x0)), // EFSS_TRILINEAR_BORDER
        };

        static_assert(CRY_ARRAY_COUNT(materialSamplers) == EFSS_MAX, "Missing material sampler!");

        for (int i = 0; i < CRY_ARRAY_COUNT(materialSamplers); ++i)
        {
            m_pPerPassResources->SetSampler(EEfResSamplers(i), materialSamplers[i], EShaderStage_AllWithoutCompute);
        }
    }

    // textures

    {
        // TODO: CRY_INTEGRATE_DX12 TERRAIN ENABLE LATER
        /*
        int nTerrainTex0=0, nTerrainTex1=0;
        gEnv->p3DEngine->GetITerrain()->GetAtlasTexId(nTerrainTex0, nTerrainTex1);
		
        CTexture* textures[] =
        {
            CTexture::GetByID(nTerrainTex0),
            CTexture::s_ptexNormalsFitting,   // EFGT_NORMALS_FITTING
            CTexture::s_ptexDissolveNoiseMap, // EFGT_DISSOLVE_NOISE
        };
		*/
		CTexture* textures[] =
		{
			nullptr,
			CTexture::s_ptexNormalsFitting,   // EFGT_NORMALS_FITTING
			CTexture::s_ptexDissolveNoiseMap, // EFGT_DISSOLVE_NOISE
		};
        static_assert(CRY_ARRAY_COUNT(textures) == EPerPassTexture_Count - EPerPassTexture_First, "Missing per pass texture!");

        for (int i = 0; i < CRY_ARRAY_COUNT(textures); ++i)
        {
            m_pPerPassResources->SetTexture(EPerPassTexture(EPerPassTexture_First + i), textures[i], SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
        }
    }

    // constant buffers
    {
        rd->GetGraphicsPipeline().UpdatePerViewConstantBuffer();
        AzRHI::ConstantBufferPtr pPerViewCB = rd->GetGraphicsPipeline().GetPerViewConstantBuffer();
        m_pPerPassResources->SetConstantBuffer(eConstantBufferShaderSlot_PerView, pPerViewCB, EShaderStage_AllWithoutCompute);
    }

    m_pPerPassResources->Build();
}

bool CSceneGBufferPass::PrepareResourceLayout()
{
    m_pResourceLayout->SetConstantBuffer(EResourceLayoutSlot_PerInstanceCB,     eConstantBufferShaderSlot_PerInstance, EShaderStage_Vertex | EShaderStage_Pixel);
    m_pResourceLayout->SetResourceSet(EResourceLayoutSlot_PerMaterialRS,        gcpRendD3D->GetGraphicsPipeline().GetDefaultMaterialResources());
    m_pResourceLayout->SetResourceSet(EResourceLayoutSlot_PerInstanceExtraRS,   gcpRendD3D->GetGraphicsPipeline().GetDefaultInstanceExtraResources());
    m_pResourceLayout->SetResourceSet(EResourceLayoutSlot_PerPassRS,            m_pPerPassResources);

    // TODO: remove once all per batch constants are gone and all objects are using the new per instance constant buffer
    m_pResourceLayout->SetConstantBuffer(EResourceLayoutSlot_PerBatchCB,        eConstantBufferShaderSlot_PerBatch, EShaderStage_Vertex | EShaderStage_Pixel);
    m_pResourceLayout->SetConstantBuffer(EResourceLayoutSlot_PerInstanceLegacy, eConstantBufferShaderSlot_PerInstanceLegacy, EShaderStage_Vertex | EShaderStage_Pixel);

    return m_pResourceLayout->Build();
}

void CSceneGBufferPass::PrepareCommandList(CDeviceGraphicsCommandListRef RESTRICT_REFERENCE commandList) const
{
    RenderTargetList targets;
    SDepthTexture depthTarget;

    GetRenderTargets(targets, depthTarget);

    D3DViewPort viewPort = { 0.0f, 0.0f, static_cast<float>(targets[0]->GetWidth()), static_cast<float>(targets[0]->GetHeight()), 0.f, 1.f };

    D3D11_RECT viewPortRect = { (LONG)viewPort.TopLeftX, (LONG)viewPort.TopLeftY, LONG(viewPort.TopLeftX + viewPort.Width), LONG(viewPort.TopLeftY + viewPort.Height) };

    commandList.SetRenderTargets(targets.size(), &targets[0], &depthTarget);
    commandList.SetViewports(1, &viewPort);
    commandList.SetScissorRects(1, &viewPortRect);
    commandList.SetResourceLayout(m_pResourceLayout.get());
    commandList.SetResources(EResourceLayoutSlot_PerPassRS, m_pPerPassResources.get());
}

void CSceneGBufferPass::GetRenderTargets(RenderTargetList& targets, SDepthTexture& depthTarget) const
{
    targets[0] = CTexture::s_ptexSceneNormalsMap;
    targets[1] = CTexture::s_ptexSceneDiffuse;
    targets[2] = CTexture::s_ptexSceneSpecular;

    depthTarget = gcpRendD3D->m_DepthBufferOrigMSAA;
}

void CSceneGBufferPass::Prepare()
{
    OnResolutionChanged();
    PreparePerPassResources();

    if (!m_bValidResourceLayout)
    {
        m_bValidResourceLayout = PrepareResourceLayout();
    }
}

void CSceneGBufferPass::Execute()
{
    PROFILE_LABEL_SCOPE("GBUFFER");

    CD3D9Renderer* rd = gcpRendD3D;
    SThreadInfo* const pThreadInfo = &(rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID]);

    rd->RT_SetViewport(0, 0, rd->m_MainViewport.nWidth, rd->m_MainViewport.nHeight);

    const uint32 bfGeneral = SRendItem::BatchFlags(EFSLIST_GENERAL, rd->m_RP.m_pRLD);
    const bool bClearDepthBuffer = CRenderer::CV_r_usezpass != 2 || !(bfGeneral & FB_ZPREPASS);
    if (bClearDepthBuffer)
    {
        const bool bReverseDepth = !!(rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH);

        // Stencil initialized to 1 - 0 is reserved for MSAAed samples
        rd->FX_ClearTarget(&rd->m_DepthBufferOrigMSAA, CLEAR_ZBUFFER | CLEAR_STENCIL, Clr_FarPlane_R.r, 1);
        rd->m_nStencilMaskRef = 1;
    }

    if (CRenderer::CV_r_MotionVectors && CRenderer::CV_r_MotionBlurGBufferVelocity)
    {
        rd->FX_ClearTarget(GetUtils().GetVelocityObjectRT(), Clr_Transparent);
    }

    RenderTargetList targets;
    SDepthTexture depthTarget;
    GetRenderTargets(targets, depthTarget);

    const int nWidth  = rd->m_MainViewport.nWidth;
    const int nHeight = rd->m_MainViewport.nHeight;
    RECT rect = { 0, 0, nWidth, nHeight };

    // TODO investigate: These clears are probably not needed, they are just here for consistency with r_graphicspipeline=0
    rd->FX_ClearTarget(targets[0], Clr_Transparent, 1, &rect, true);
    rd->FX_ClearTarget(targets[1], Clr_Empty, 1, &rect, true);
    rd->FX_ClearTarget(targets[2], Clr_Empty, 1, &rect, true);

    rd->FX_PushRenderTarget(0, targets[0], &depthTarget, -1, true);
    rd->FX_PushRenderTarget(1, targets[1], NULL);
    rd->FX_PushRenderTarget(2, targets[2], NULL);
    rd->FX_SetActiveRenderTargets();

    RenderSceneOpaque();

    if (rd->m_RP.m_PersFlags2 & RBPF2_MOTIONBLURPASS)
    {
        rd->FX_PopRenderTarget(3);
        rd->FX_SetActiveRenderTargets();
        rd->m_RP.m_PersFlags2 &= ~RBPF2_MOTIONBLURPASS;
    }

    pThreadInfo->m_PersFlags |= RBPF_ZPASS;
    RenderSceneOverlays();

    pThreadInfo->m_PersFlags &= ~RBPF_ZPASS;
    rd->FX_PopRenderTarget(0);
    rd->FX_PopRenderTarget(1);
    rd->FX_PopRenderTarget(2);
    rd->FX_SetActiveRenderTargets();
}
