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
#include "ShadowMap.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"

#include "Common/Include_HLSL_CPP_Shared.h"
#include "Common/GraphicsPipelineStateSet.h"
#include "Common/TypedConstantBuffer.h"

//CShadowMapPass::PSOCache CShadowMapPass::m_cachePSO;

CShadowMapPass* g_pShadowMapPassPtr = 0; // Just a HACK. remove later.

void CShadowMapPass::Init()
{
}

void CShadowMapPass::Shutdown()
{
}

void CShadowMapPass::DrawBatch(CShader* pShader, SShaderPass* pPass, bool bObjSkinned, CDeviceGraphicsPSOPtr pPSO)
{
}


ILINE uint64 CShadowMapPass::GetPSOCacheKey(uint32 objFlags, int shaderID, int resID, bool bZPrePass)
{
    uint64 keyHigh = objFlags;
    uint32 keyLow = (resID << 18) | (shaderID << 6) | (bZPrePass ? 0x4 : 0);
    uint64 key = (keyHigh << 32) | keyLow;

    return key;
}

CDeviceGraphicsPSOPtr CShadowMapPass::CreatePipelineState(const SGraphicsPipelineStateDescription& description, int psoId)
{
    CShader* pShader = static_cast<CShader*>(description.shaderItem.m_pShader);
    CShaderResources* pRes = static_cast<CShaderResources*>(description.shaderItem.m_pShaderResources);
    uint64 objectFlags = description.objectFlags;
    uint32 mdMask = 0;
    uint32 mdvMask = description.objectFlags_MDV;


    SShaderTechnique* pTechnique = pShader->GetTechnique(description.shaderItem.m_nTechnique, description.technique);
    if (!pTechnique)
    {
        return nullptr;
    }
    SShaderPass* pShaderPass = &pTechnique->m_Passes[0];

    uint32 renderState = pShaderPass->m_RenderState;

    uint64 rtMask = description.objectRuntimeMask;

    rtMask |= g_HWSR_MaskBit[HWSR_QUALITY] | g_HWSR_MaskBit[HWSR_QUALITY1];
    SThreadInfo* const pShaderThreadInfo = &(gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID]);

    bool bTwoSided = false;
    {
        if (pRes->m_ResFlags & MTL_FLAG_2SIDED)
        {
            bTwoSided = true;
        }

        if (pRes->IsAlphaTested())
        {
            rtMask |= g_HWSR_MaskBit[HWSR_ALPHATEST];
        }

        if (psoId == ESR_RSM)
        {
            if (pRes->m_Textures[EFTT_DIFFUSE] && pRes->m_Textures[EFTT_DIFFUSE]->m_Ext.m_pTexModifier)
            {
                mdMask |= pRes->m_Textures[EFTT_DIFFUSE]->m_Ext.m_nUpdateFlags;
            }
        }

        if (pRes->m_pDeformInfo)
        {
            mdvMask |= pRes->m_pDeformInfo->m_eType;
        }
    }

    bool bTessellation = false;
    rtMask |= g_HWSR_MaskBit[HWSR_NO_TESSELLATION];

#ifdef TESSELLATION_RENDERER
    const bool bHasTesselationShaders = pShaderPass && pShaderPass->m_HShader && pShaderPass->m_DShader;
    if (bHasTesselationShaders && (!(objectFlags & FOB_NEAREST) && (objectFlags & FOB_ALLOW_TESSELLATION)))
    {
        rtMask &= ~g_HWSR_MaskBit[HWSR_NO_TESSELLATION];
        bTessellation = true;
    }
#endif


    ECull cullMode = bTwoSided ? eCULL_None : (pShaderPass->m_eCull != -1 ? (ECull)pShaderPass->m_eCull : eCULL_Back);
    if (pShader->m_eSHDType == eSHDT_Terrain)
    {
        //Flipped matrix for point light sources
        if (psoId == ESR_DIRECTIONAL)
        {
            cullMode = eCULL_None;
        }
        else
        {
            cullMode = eCULL_Front; //front faces culling by default for terrain
        }
    }

    if (psoId == ESR_RSM)
    {
        rtMask |= g_HWSR_MaskBit[ HWSR_SAMPLE4 ];

        rtMask |= g_HWSR_MaskBit[HWSR_CUBEMAP0] | g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ]; //valid for sun only

        cullMode = eCULL_Back;

        if (objectFlags & FOB_DECAL_TEXGEN_2D)
        {
            rtMask |= g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_2D];
        }

        //
        if ((objectFlags & FOB_BLEND_WITH_TERRAIN_COLOR)) // && rRP.m_pCurObject->m_nTextureID > 0
        {
            rtMask |= g_HWSR_MaskBit[HWSR_BLEND_WITH_TERRAIN_COLOR];
        }
    }
    else if (psoId = ESR_PointLight)
    {
        rtMask |= g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ];
        rtMask |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
    }

    mdvMask |= pShader->m_nMDV;
    if (objectFlags & FOB_OWNER_GEOMETRY)
    {
        mdvMask &= ~MDV_DEPTH_OFFSET;
    }
    if ((objectFlags & FOB_BENDED) && psoId != ESR_RSM)
    {
        mdvMask |= MDV_BENDING;
    }

    if (objectFlags & FOB_NEAREST)
    {
        rtMask |= g_HWSR_MaskBit[HWSR_NEAREST];
    }
    if (objectFlags & FOB_DISSOLVE)
    {
        rtMask |= g_HWSR_MaskBit[HWSR_DISSOLVE];
    }
    if (renderState & GS_ALPHATEST_MASK)
    {
        rtMask |= g_HWSR_MaskBit[HWSR_ALPHATEST];
    }


    eRenderPrimitiveType primitiveType = eRenderPrimitiveType(description.primitiveType);
    uint32 objectStreamMask = description.streamMask;

    if (bTessellation)
    {
        primitiveType = ept3ControlPointPatchList;
        objectStreamMask |= VSM_NORMALS;
    }

    // Create PSO
    CDeviceGraphicsPSODesc psoDesc(m_pResourceLayout.get(), pShader, pTechnique->m_NameCRC, rtMask, mdMask, mdvMask, bTessellation);

    psoDesc.m_RenderState = renderState;
    psoDesc.m_CullMode = cullMode;
    //psoDesc.m_RenderTargetFormats[i] =
    //psoDesc.m_DepthStencilFormat =

    psoDesc.m_VertexFormat = description.vertexFormat;
    psoDesc.m_ObjectStreamMask = objectStreamMask;
    psoDesc.m_PrimitiveType = primitiveType;

    psoDesc.Build();

    return CDeviceObjectFactory::GetInstance().CreateGraphicsPSO(psoDesc);
}

int CShadowMapPass::CreatePipelineStates(DevicePipelineStatesArray& out, const SGraphicsPipelineStateDescription& stateDesc, CGraphicsPipelineStateLocalCache* pStateCache)
{
    auto cachedStates = pStateCache->Find(stateDesc);
    if (cachedStates)
    {
        out = *cachedStates;
        return ESR_Count;
    }

    for (int psoId = ESR_First; psoId < ESR_Count; psoId++)
    {
        assert(psoId < 4); //temp check
        out[psoId] = CreatePipelineState(stateDesc, psoId);
    }

    pStateCache->Put(stateDesc, out);
    return ESR_Count;
}

void CShadowMapPass::FlushShader()
{
}

void CShadowMapPass::Render()
{
    return;
}

void CShadowMapPass::PreparePerPassConstantBuffer()
{
    ////per-object bias for Shadow Generation
    //rd->m_cEF.m_TempVecs[1][0] = 0.0f;

    //if (rd->m_RP.m_pShaderResources)
    //{
    //  if (rd->m_RP.m_pShaderResources->m_ResFlags & MTL_FLAG_2SIDED)
    //  {
    //      //handle terrain self-shadowing and two-sided geom
    //      rd->m_cEF.m_TempVecs[1][0] = rTI.m_vFrustumInfo.w;
    //  }

    //}

    return;
}

void CShadowMapPass::Prepare()
{
}

void CShadowMapPass::Execute()
{
    return;
}

void CShadowMapPass::Reset() 
{

}
