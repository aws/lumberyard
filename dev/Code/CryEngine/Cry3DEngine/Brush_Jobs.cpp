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

#include "StdAfx.h"
#include <ILMSerializationManager.h>
#include "StatObj.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "terrain_sector.h"
#include "CullBuffer.h"
#include "3dEngine.h"
#include "IndexedMesh.h"
#include "Brush.h"
#include "terrain.h"
#include "Environment/OceanEnvironmentBus.h"

///////////////////////////////////////////////////////////////////////////////
void CBrush::Render(const CLodValue& lodValue, const SRenderingPassInfo& passInfo, const SSectorTextureSet* pTerrainTexInfo, AZ::LegacyJobExecutor* pJobExecutor, const SRendItemSorter& rendItemSorter)
{
    FUNCTION_PROFILER_3DENGINE;
    CVars* pCVars = GetCVars();

    // Collision proxy is visible in Editor while in editing mode.
    if (m_dwRndFlags & ERF_COLLISION_PROXY)
    {
        if (!gEnv->IsEditor() || !gEnv->IsEditing())
        {
            if (pCVars->e_DebugDraw == 0)
            {
                return;
            }
        }
    }

    if (m_dwRndFlags & ERF_HIDDEN)
    {
        return;
    }

    const Vec3 vCamPos = passInfo.GetCamera().GetPosition();
    const Vec3 vObjCenter = CBrush::GetBBox().GetCenter();
    const Vec3 vObjPos = CBrush::GetPos();
    CRNTmpData::SRNUserData& userData = m_pRNTmpData->userData;

    CRenderObject* pObj = gEnv->pRenderer->EF_GetObject_Temp(passInfo.ThreadID());
    pObj->m_fDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, CBrush::GetBBox())) * passInfo.GetZoomFactor();

    pObj->m_pRenderNode = this;
    pObj->m_II.m_Matrix = userData.objMat;
    pObj->m_fAlpha = 1.f;
    IF (!m_bDrawLast, 1)
    {
        pObj->m_nSort = fastround_positive(pObj->m_fDistance * 2.0f);
    }
    else
    {
        pObj->m_fSort = 10000.0f;
    }

    _smart_ptr<IMaterial> pMat = pObj->m_pCurrMaterial = CBrush::GetMaterial();

    if (m_dwRndFlags & ERF_NO_DECALNODE_DECALS)
    {
        pObj->m_ObjFlags |= FOB_DYNAMIC_OBJECT;
        pObj->m_NoDecalReceiver = true;
    }
    else
    {
        pObj->m_ObjFlags &= ~FOB_DYNAMIC_OBJECT;
    }

    if (uint8 nMaterialLayers = IRenderNode::GetMaterialLayers())
    {
        uint8 nFrozenLayer = (nMaterialLayers & MTL_LAYER_FROZEN) ? MTL_LAYER_FROZEN_MASK : 0;
        uint8 nWetLayer = (nMaterialLayers & MTL_LAYER_WET) ? MTL_LAYER_WET_MASK : 0;
        pObj->m_nMaterialLayers = (uint32)(nFrozenLayer << 24) | (nWetLayer << 16);
    }
    else
    {
        pObj->m_nMaterialLayers = 0;
    }

    if (!passInfo.IsShadowPass() && m_nInternalFlags & IRenderNode::REQUIRES_NEAREST_CUBEMAP)
    {
        if (!(pObj->m_nTextureID = GetObjManager()->CheckCachedNearestCubeProbe(this)) || !pCVars->e_CacheNearestCubePicking)
        {
            pObj->m_nTextureID = GetObjManager()->GetNearestCubeProbe(m_pOcNode->m_pVisArea, CBrush::GetBBox());
        }

        m_pRNTmpData->userData.nCubeMapId = pObj->m_nTextureID;
    }

    //////////////////////////////////////////////////////////////////////////
    // temp fix to update ambient color (Vlad please review!)
    pObj->m_nClipVolumeStencilRef = userData.m_pClipVolume ? userData.m_pClipVolume->GetStencilRef() : 0;
    pObj->m_II.m_AmbColor = Vec3(0, 0, 0);
    //////////////////////////////////////////////////////////////////////////

    if (pTerrainTexInfo)
    {
        bool bUseTerrainColor = (GetCVars()->e_BrushUseTerrainColor == 2);
        if (pMat && GetCVars()->e_BrushUseTerrainColor == 1)
        {
            if (pMat->GetSafeSubMtl(0)->GetFlags() & MTL_FLAG_BLEND_TERRAIN)
            {
                bUseTerrainColor = true;
            }
        }

        if (bUseTerrainColor)
        {
            pObj->m_ObjFlags |= FOB_BLEND_WITH_TERRAIN_COLOR;
            pObj->m_nTextureID = pTerrainTexInfo->nTex0;
            SRenderObjData* pOD = GetRenderer()->EF_GetObjData(pObj, true, passInfo.ThreadID());
            pOD->m_fTempVars[3] = pTerrainTexInfo->fTexOffsetX;
            pOD->m_fTempVars[4] = pTerrainTexInfo->fTexOffsetY;
            pOD->m_fTempVars[5] = pTerrainTexInfo->fTexScale;
            pOD->m_fTempVars[8] = m_fWSMaxViewDist * 0.3333f; // 0.333f => replicating magic ratio from fRenderQuality
        }
        else
        {
            pObj->m_ObjFlags &= ~FOB_BLEND_WITH_TERRAIN_COLOR;
        }
    }

    // check the object against the water level
    if (GetObjManager()->IsAfterWater(vObjCenter, passInfo))
    {
        pObj->m_ObjFlags |= FOB_AFTER_WATER;
    }
    else
    {
        pObj->m_ObjFlags &= ~FOB_AFTER_WATER;
    }

    if (CStatObj* pStatObj = (CStatObj*)CBrush::GetEntityStatObj())
    {
        // temporary fix for autoreload from max export, Vladimir needs to properly fix it!
        if (pObj->m_pCurrMaterial != GetMaterial())
        {
            pObj->m_pCurrMaterial = GetMaterial();
        }

        if (Get3DEngine()->IsTessellationAllowed(pObj, passInfo, true))
        {
            // Allow this RO to be tessellated, however actual tessellation will be applied if enabled in material
            pObj->m_ObjFlags |= FOB_ALLOW_TESSELLATION;
        }
        else
        {
            pObj->m_ObjFlags &= ~FOB_ALLOW_TESSELLATION;
        }

        if (pCVars->e_BBoxes)
        {
            GetObjManager()->RenderObjectDebugInfo((IRenderNode*)this, pObj->m_fDistance, passInfo);
        }

        if (lodValue.LodA() <= 0 && Cry3DEngineBase::GetCVars()->e_MergedMeshes != 0 && m_pDeform && m_pDeform->HasDeformableData())
        {
            if (GetCVars()->e_StatObjBufferRenderTasks == 1 && passInfo.IsGeneralPass())
            {
                GetObjManager()->PushIntoCullOutputQueue(SCheckOcclusionOutput::CreateDeformableBrushOutput(this, gEnv->pRenderer->EF_DuplicateRO(pObj, passInfo), lodValue.LodA(), rendItemSorter));
            }
            else
            {
                m_pDeform->RenderInternalDeform(gEnv->pRenderer->EF_DuplicateRO(pObj, passInfo), lodValue.LodA(), CBrush::GetBBox(), passInfo, rendItemSorter);
            }
        }

        if (GetCVars()->e_StatObjBufferRenderTasks == 1 && passInfo.IsGeneralPass() && GetCVars()->e_DebugDraw)
        {
            // execute on MainThread for debug drawing, as else we run into threading issues with the AuxRenderer
            GetObjManager()->PushIntoCullOutputQueue(SCheckOcclusionOutput::CreateBrushOutput(this, pObj, lodValue, rendItemSorter));
        }
        else
        {
            // it can happen that a brush has a shader which forces the objects into the preprocess/water list
            // these lists are handled earlier than GENERAL lists, thus they need to use another sync variable
            if (!passInfo.IsShadowPass() && !passInfo.IsRecursivePass() && m_bExecuteAsPreprocessJob)
            {
                pJobExecutor = gEnv->pRenderer->GetGenerateRendItemJobExecutorPreProcess();
            }

            if (pJobExecutor && GetCVars()->e_DebugDraw == 0)
            {
                // legacy job priority: "passInfo.IsGeneralPass() ? JobManager::eRegularPriority : JobManager::eLowPriority"
                pJobExecutor->StartJob(
                    [this, pObj, lodValue, passInfo, rendItemSorter]
                    {
                        this->Render_JobEntry(pObj, lodValue, passInfo, rendItemSorter);
                    }
                );
            }
            else
            {
                Render_JobEntry(pObj, lodValue, passInfo, rendItemSorter);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void CBrush::Render_JobEntry(CRenderObject* pObj, const CLodValue lodValue, const SRenderingPassInfo& passInfo, SRendItemSorter rendItemSorter)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ThreeDEngine);

    CStatObj* pStatObj = (CStatObj*)CBrush::GetEntityStatObj();
    pStatObj->RenderInternal(pObj, 0, lodValue, passInfo, rendItemSorter, false);
}

///////////////////////////////////////////////////////////////////////////////
IStatObj* CBrush::GetEntityStatObj(unsigned int nPartId, unsigned int nSubPartId, Matrix34A* pMatrix, bool bReturnOnlyVisible)
{
    if (nPartId != 0)
    {
        return 0;
    }

    if (pMatrix)
    {
        *pMatrix = m_Matrix;
    }

    return m_pStatObj;
}

///////////////////////////////////////////////////////////////////////////////
void CBrush::OnRenderNodeBecomeVisible(const SRenderingPassInfo& passInfo)
{
    assert(m_pRNTmpData);
    CRNTmpData::SRNUserData& userData = m_pRNTmpData->userData;

    userData.objMat = m_Matrix;
    const Vec3 vCamPos = passInfo.GetCamera().GetPosition();
    float fEntDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, CBrush::GetBBox())) * passInfo.GetZoomFactor();

    userData.nWantedLod = GetObjManager()->GetObjectLOD(this, fEntDistance);

    int nLod = userData.nWantedLod;

    if (CStatObj* pStatObj = (CStatObj*)CBrush::GetEntityStatObj())
    {
        nLod = CLAMP(nLod, pStatObj->GetMinUsableLod(), (int)pStatObj->m_nMaxUsableLod);
        nLod = pStatObj->FindNearesLoadedLOD(nLod);
    }

    userData.lodDistDissolveTransitionState.nNewLod = userData.lodDistDissolveTransitionState.nOldLod = userData.nWantedLod;
    userData.lodDistDissolveTransitionState.fStartDist = 0.0f;
    userData.lodDistDissolveTransitionState.bFarside = false;
}


///////////////////////////////////////////////////////////////////////////////
bool CBrush::CanExecuteRenderAsJob()
{
    return !HasDeformableData();
}
