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
#include "CompiledRenderObject.h"

#include "DriverD3D.h"
#include "Common/Include_HLSL_CPP_Shared.h"
#include "CryUtils.h"
//////////////////////////////////////////////////////////////////////////
#include "GraphicsPipeline/ShadowMap.h"
#include "GraphicsPipeline/SceneGBuffer.h"
//////////////////////////////////////////////////////////////////////////

CRenderObjectsPools* CCompiledRenderObject::s_pPools = 0;

//////////////////////////////////////////////////////////////////////////
CRenderObjectsPools::CRenderObjectsPools()
{
}

//////////////////////////////////////////////////////////////////////////
CRenderObjectsPools::~CRenderObjectsPools()
{
}

//////////////////////////////////////////////////////////////////////////
AzRHI::ConstantBufferPtr CRenderObjectsPools::AllocatePerInstanceConstantBuffer()
{
    if (!m_freeConstantBuffers.empty())
    {
        AzRHI::ConstantBufferPtr ptr = m_freeConstantBuffers.back();
        m_freeConstantBuffers.pop_back();
        return ptr;
    }

    AzRHI::ConstantBuffer* buffer = gcpRendD3D->m_DevBufMan.CreateConstantBuffer(
        "PerInstanceCompiled",
        sizeof(HLSL_PerInstanceConstantBuffer),
        AzRHI::ConstantBufferUsage::Dynamic,
        AzRHI::ConstantBufferFlags::None);

    AzRHI::ConstantBufferPtr ptr;
    ptr.attach(buffer);
    return ptr;
}

//////////////////////////////////////////////////////////////////////////
void CRenderObjectsPools::FreePerInstanceConstantBuffer(const AzRHI::ConstantBufferPtr& buffer)
{
    if (buffer)
    {
        m_freeConstantBuffers.push_back(buffer);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCompiledRenderObject::Release()
{
    m_pRenderElement = 0;
    m_shaderItem.m_pShader = 0;
    m_shaderItem.m_pShaderResources = 0;

    m_StencilRef = 0;
    m_nInstances = 0;
    m_bCompiled = false;

    if (m_perInstanceCB && m_bOwnPerInstanceCB)
    {
        s_pPools->FreePerInstanceConstantBuffer(m_perInstanceCB);
    }
    m_perInstanceCB = nullptr;

    m_materialResourceSet = nullptr;

    m_InstancingCBs.clear();

    for (int i = 0; i < RS_TECH_NUM; i++)
    {
        m_pso[i] = DevicePipelineStatesArray();
    }

    if (m_pNext)
    {
        FreeToPool(m_pNext);
        m_pNext = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void CCompiledRenderObject::CompilePerInstanceConstantBuffer(CRenderObject* pRenderObject, float realTime)
{
    CCompiledRenderObject* pRootCompiled = pRenderObject->m_pCompiled;
    if (pRootCompiled && pRootCompiled->m_perInstanceCB)
    {
        // If root object have per instance constant buffer, share ours with root compiled object.
        m_bOwnPerInstanceCB = false;
        m_perInstanceCB = pRootCompiled->m_perInstanceCB;
        m_rootConstants = pRootCompiled->m_rootConstants;
        return;
    }

    // Create per instance constant buffer.
    HLSL_PerInstanceConstantBuffer cb;
    memset(&cb, 0, sizeof(HLSL_PerInstanceConstantBuffer));

    cb.SPIObjWorldMat = pRenderObject->m_II.m_Matrix;

    // Alpha Test
    float dissolve = 0;
    if (pRenderObject->m_ObjFlags & (FOB_DISSOLVE_OUT | FOB_DISSOLVE))
    {
        dissolve = float(pRenderObject->m_DissolveRef) * (1.0f / 255.0f);
    }
    float dissolveOut = (pRenderObject->m_ObjFlags & FOB_DISSOLVE_OUT) ? 1.0f : -1.0f;

    cb.SPIDissolveRef[0] = dissolve;
    cb.SPIDissolveRef[1] = dissolveOut;

    //if (pRenderObject->m_data.m_pBending)
    //{
    //    pRenderObject->m_data.m_pBending->GetShaderConstantsStatic(realTime, (Vec4*)cb.SPIBendInfo);
    //}

    m_rootConstants.dissolve = dissolve;
    m_rootConstants.dissolveOut = dissolveOut;

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //@TODO All this block should be removed!
    //@TODO. Not needed here! should be moved to the PerPass constant buffer.
    cb.SPIDissolveRef[2] = CRenderer::CV_r_ssdoAmountDirect; // Use free component for SSDO
    //@TODO. Not needed here! should be moved to the per material constants.
    cb.SPIDissolveRef[3] = ((CShaderResources*)m_shaderItem.m_pShaderResources)->m_AlphaRef;
    //cb.SPIRainLayerParams = 0; // Not used! //@TODO Remove it
    //cb.SPIVertexAO = 0; // Not used! //@TODO Remove it!
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // TODO: CRY_INTEGRATE_DX12 TERRAIN ENABLE LATER
    /*
    if (pRenderObject->m_data.m_pTerrainSectorTextureInfo)
    {
        // Fill terrain texture info if present
        cb.Varying0[0] = pRenderObject->m_data.m_pTerrainSectorTextureInfo->fTexOffsetX;
        cb.Varying0[1] = pRenderObject->m_data.m_pTerrainSectorTextureInfo->fTexOffsetY;
        cb.Varying0[2] = pRenderObject->m_data.m_pTerrainSectorTextureInfo->fTexScale;
        cb.Varying0[3] = pRenderObject->m_data.m_fMaxViewDistance; // Obj view max distance
    }
    else if (pRenderObject->m_ObjFlags & FOB_SKINNED)
    {
        if (SRenderObjData* pOD = pRenderObject->GetObjData())
        {
            // Skinning precision offset
            cb.Varying0[0] = pOD->m_fTempVars[0];
            cb.Varying0[1] = pOD->m_fTempVars[1];
            cb.Varying0[2] = pOD->m_fTempVars[2];
        }
        if (m_pRenderElement->mfGetType() == eDATA_Mesh)
        {
            // Skinning extra weights
            cb.Varying0[3] = ((CREMeshImpl*)m_pRenderElement)->m_pRenderMesh->m_extraBonesBuffer.m_numElements > 0 ? 1.0f : 0.0f;
        }
    }*/

    if (!m_perInstanceCB)
    {
        m_perInstanceCB = s_pPools->AllocatePerInstanceConstantBuffer();
        m_bOwnPerInstanceCB = true;
    }
    m_perInstanceCB->UpdateBuffer(&cb, sizeof(cb));
}

void CCompiledRenderObject::CompileInstancingData(CRenderObject* pRenderObject)
{
    int nSrcInsts = pRenderObject->m_Instances.size();
    if (m_nInstances == nSrcInsts)
    {
        return;
    }

    assert(nSrcInsts);
    m_nInstances = nSrcInsts;

    {
        //////////////////////////////////////////////////////////////////////////
        //@TODO!!! Remove it Later
        //////////////////////////////////////////////////////////////////////////
        // Workaround for AlphaTest working (must be filled in 3dengine)
        CShaderResources* pSR = (CShaderResources*)m_shaderItem.m_pShaderResources;
        for (int i = 0; i < nSrcInsts; i++)
        {
            pRenderObject->m_Instances[i].m_vDissolveInfo.w = pSR->m_AlphaRef;
        }
    }

    m_InstancingCBs.reserve(nSrcInsts);

    int nBeginOffs = 0;
    while (nSrcInsts)
    {
        int instanceCountPerConstantBuffer = MIN(nSrcInsts, 800); // 4096 Vec4 entries max in DX11

        int nSize = instanceCountPerConstantBuffer * sizeof(CRenderObject::SInstanceData);
        AzRHI::ConstantBuffer* constantBuffer = gcpRendD3D.m_DevBufMan.CreateConstantBuffer(
            "PerInstanceHWCompiled",
            nSize,
            AzRHI::ConstantBufferUsage::Dynamic,
            AzRHI::ConstantBufferFlags::None);

        if (constantBuffer)
        {
            constantBuffer->m_pDescriptorBlock = gcpRendD3D->m_DevBufMan.CreateDescriptorBlock(1);
            constantBuffer->m_nHeapOffset = constantBuffer->m_pDescriptorBlock ? constantBuffer->m_pDescriptorBlock->offset : -1;
            constantBuffer->UpdateBuffer(&pRenderObject->m_Instances[nBeginOffs], nSize);

            CCompiledRenderObject::SInstancingData ID;
            ID.m_nInstances = instanceCountPerConstantBuffer;
            ID.m_pConstBuffer.attach(constantBuffer);
            m_InstancingCBs.push_back(ID);
        }

        nBeginOffs += instanceCountPerConstantBuffer;
        nSrcInsts -= instanceCountPerConstantBuffer;
    }
    pRenderObject->m_Instances.clear();
}

//////////////////////////////////////////////////////////////////////////
void CCompiledRenderObject::CompilePerInstanceExtraResources(CRenderObject* pRenderObject)
{
    if (!pRenderObject->m_data.m_pSkinningData) // only needed for skinning at the moment
    {
        return;
    }

    CRendElementBase::SGeometryInfo geomInfo;
    if (!m_pRenderElement->GetGeometryInfo(geomInfo))
    {
        ZeroStruct(geomInfo);
    }

    m_perInstanceExtraResources = CDeviceObjectFactory::GetInstance().CreateResourceSet();

    if (SSkinningData* pSkinningData = pRenderObject->m_data.m_pSkinningData)
    {
        // TODO: only bind to hs and ds stages when tessellation is enabled
        const EShaderStage shaderStages = EShaderStage_Vertex | EShaderStage_Hull | EShaderStage_Domain;

        CD3D9Renderer::SCharInstCB* pCurSkinningData  = alias_cast<CD3D9Renderer::SCharInstCB*>(pSkinningData->pCharInstCB);
        CD3D9Renderer::SCharInstCB* pPrevSkinningData = nullptr;

        if (pSkinningData->pPreviousSkinningRenderData)
        {
            pPrevSkinningData = alias_cast<CD3D9Renderer::SCharInstCB*>(pSkinningData->pPreviousSkinningRenderData->pCharInstCB);
        }

        m_perInstanceExtraResources->SetConstantBuffer(eConstantBufferShaderSlot_SkinQuat,     pCurSkinningData  ? pCurSkinningData->m_buffer  : nullptr, shaderStages);
        m_perInstanceExtraResources->SetConstantBuffer(eConstantBufferShaderSlot_SkinQuatPrev, pPrevSkinningData ? pPrevSkinningData->m_buffer : nullptr, shaderStages);

        WrappedDX11Buffer* pExtraWeights = reinterpret_cast<WrappedDX11Buffer*>(geomInfo.pSkinningExtraBonesBuffer);
        m_perInstanceExtraResources->SetBuffer(EReservedTextureSlot_SkinExtraWeights, pExtraWeights ? *pExtraWeights : WrappedDX11Buffer(), shaderStages);

        WrappedDX11Buffer* pAdjacencyBuffer = reinterpret_cast<WrappedDX11Buffer*>(geomInfo.pTessellationAdjacencyBuffer);
        m_perInstanceExtraResources->SetBuffer(EReservedTextureSlot_AdjacencyInfo, pAdjacencyBuffer ? *pAdjacencyBuffer : WrappedDX11Buffer(), shaderStages);
    }

    m_perInstanceExtraResources->Build();
}

//////////////////////////////////////////////////////////////////////////
bool CCompiledRenderObject::Compile(CRenderObject* pRenderObject, float realTime)
{
    const bool bMuteWarnings = CRenderer::CV_r_GraphicsPipeline == 2;  // @TODO: Remove later

    m_bCompiled = false;

    bool bInstanceDataUpdateOnly = pRenderObject->m_bInstanceDataDirty;

    // Only objects with RenderElements can be compiled
    if (!m_pRenderElement)
    {
        if (!bMuteWarnings)
        {
            Warning("[CCompiledRenderObject] Compile failed, no render element");
        }
        return false;
    }

    if (pRenderObject->m_Instances.size())
    {
        CompileInstancingData(pRenderObject);
    }

    CompilePerInstanceConstantBuffer(pRenderObject, realTime);
    CompilePerInstanceExtraResources(pRenderObject);

    if (bInstanceDataUpdateOnly)
    {
        return true;
    }

    CRendElementBase* pRenderElement = m_pRenderElement;
    SShaderItem& shaderItem = m_shaderItem;

    CShaderResources* RESTRICT_POINTER pResources = static_cast<CShaderResources*>(shaderItem.m_pShaderResources);
    assert(pResources);

    m_materialResourceSet = pResources->m_pCompiledResourceSet;
    if (!m_materialResourceSet)
    {
        return false;
    }

    CRendElementBase::SGeometryInfo geomInfo;
    geomInfo.bonesRemapGUID = (pRenderObject->m_data.m_pSkinningData) ? pRenderObject->m_data.m_pSkinningData->remapGUID : 0;
    if (!pRenderElement->GetGeometryInfo(geomInfo))
    {
        if (!bMuteWarnings)
        {
            Warning("[CCompiledRenderObject] Compile failed, GetGeometryInfo fail");
        }
        return false;
    }

    // Fill stream pointers.
    m_indexStream = geomInfo.indexStream;
    for (int i = 0; i < CRY_ARRAY_COUNT(geomInfo.vertexStream); ++i)
    {
        m_vertexStream[i] = geomInfo.vertexStream[i];
    }

    m_nMaxVertexStream = geomInfo.nMaxVertexStreams;

    m_DIP.m_nNumIndices = geomInfo.nNumIndices;
    m_DIP.m_nStartIndex = geomInfo.nFirstIndex;
    m_DIP.m_nVerticesCount = geomInfo.nNumVertices;

    // Create Pipeline States
    SGraphicsPipelineStateDescription psoDescription(pRenderObject, shaderItem, TTYPE_GENERAL, geomInfo.vertexFormat, geomInfo.streamMask, geomInfo.primitiveType);
    psoDescription.objectRuntimeMask |= g_HWSR_MaskBit[HWSR_PER_INSTANCE_CB_TEMP];  // Enable flag to use special per instance constant buffer
    if (m_InstancingCBs.size() != 0)
    {
        psoDescription.objectRuntimeMask |= g_HWSR_MaskBit[HWSR_ENVIRONMENT_CUBEMAP];  // Enable flag to use static instancing
    }
    if (!gcpRendD3D->GetGraphicsPipeline().CreatePipelineStates(m_pso, psoDescription, pResources->m_pipelineStateCache.get()))
    {
        if (!bMuteWarnings)
        {
            Warning("[CCompiledRenderObject] Compile failed, CreatePipelineStates fail");
        }
        return false;
    }

    // stencil ref value
    uint8 stencilRef = 0; // @TODO: get from CRNTmpData::SRNUserData::m_pClipVolume::GetStencilRef
    m_StencilRef = CRenderer::CV_r_VisAreaClipLightsPerPixel ? 0 : (stencilRef | BIT_STENCIL_INSIDE_CLIPVOLUME);
    m_StencilRef |= (!(pRenderObject->m_ObjFlags & FOB_DYNAMIC_OBJECT) ? BIT_STENCIL_RESERVED : 0);

    m_bCompiled = true;


    {
        //////////////////////////////////////////////////////////////////////////
        //@TODO!!! Remove it Later, validation that PerBatch parameters not exist.
        //////////////////////////////////////////////////////////////////////////
        // Reflect shader parameters for each stage.
        for (int stage = 0; stage < eHWSC_Num; stage++)
        {
            uint32 nOutBufferSize = 0;
            CDeviceGraphicsPSOPtr pso = m_pso[RS_TECH_GBUFPASS][0];
            if (pso && pso->m_pHwShaderInstances[stage])
            {
                // Check that some per batch parameters are reflected
                assert(((CHWShader_D3D::SHWSInstance*)pso->m_pHwShaderInstances[stage])->m_nParams[0] < 0);
            }
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CCompiledRenderObject::DrawAsync(CDeviceGraphicsCommandListRef RESTRICT_REFERENCE commandList, const SGraphicsPiplinePassContext& passContext) const
{
    if (CRenderer::CV_r_NoDraw == 2) // Completely skip filling of the command list.
    {
        return;
    }

    CDeviceGraphicsPSOPtr pPso = m_pso[passContext.passId][passContext.passSubId];
    if (!pPso)
    {
        return;
    }

    // Set states
    commandList.SetPipelineState(pPso);
    commandList.SetStencilRef(m_StencilRef);
    commandList.SetResources(EResourceLayoutSlot_PerMaterialRS, m_materialResourceSet.get());
    commandList.SetInlineConstantBuffer(EResourceLayoutSlot_PerInstanceCB, m_perInstanceCB, eConstantBufferShaderSlot_PerInstance, EShaderStage_Vertex | EShaderStage_Pixel);

    if (m_perInstanceExtraResources)
    {
        commandList.SetResources(EResourceLayoutSlot_PerInstanceExtraRS, m_perInstanceExtraResources.get());
    }
    else
    {
        commandList.SetResources(EResourceLayoutSlot_PerInstanceExtraRS, gcpRendD3D->GetGraphicsPipeline().GetDefaultInstanceExtraResources().get());
    }

    commandList.SetVertexBuffers(m_nMaxVertexStream, m_vertexStream);

    if (CRenderer::CV_r_NoDraw != 3)
    {
#ifndef _RELEASE
        if (m_vertexStream[VSF_HWSKIN_INFO].pStream)
        {
            CryInterlockedIncrement(&passContext.pPipelineStats->m_NumRendSkinnedObjects);
        }
#endif

        if (!m_InstancingCBs.empty())
        {
            // Render instanced draw calls.
            assert(m_indexStream.pStream);
            commandList.SetIndexBuffer(m_indexStream);
            for (auto& ID : m_InstancingCBs)
            {
                commandList.SetInlineConstantBuffer(EResourceLayoutSlot_PerInstanceCB, ID.m_pConstBuffer, eConstantBufferShaderSlot_PerInstance, EShaderStage_Vertex | EShaderStage_Pixel);
                commandList.DrawIndexed(m_DIP.m_nNumIndices, ID.m_nInstances, m_DIP.m_nStartIndex, 0, 0);
            }
            return;
        }

        if (m_indexStream.pStream != nullptr)
        {
            commandList.SetIndexBuffer(m_indexStream);
            commandList.DrawIndexed(m_DIP.m_nNumIndices, 1, m_DIP.m_nStartIndex, 0, 0);
        }
        else
        {
            commandList.Draw(m_DIP.m_nVerticesCount, 1, 0, 0);
        }
    }
}
