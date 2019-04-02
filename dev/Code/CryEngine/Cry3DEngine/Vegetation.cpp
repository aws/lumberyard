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

// Description : Loading trees, buildings, register/unregister entities for rendering


#include "StdAfx.h"

#include "terrain.h"
#include "StatObj.h"
#include "ObjMan.h"
#include "CullBuffer.h"
#include "3dEngine.h"
#include "Vegetation.h"
#include "DeformableNode.h"

#define BYTE2RAD(x) ((x) * float(g_PI2) / 255.0f)


//volatile int g_lockVegetationPhysics = 0;

float CVegetation::g_scBoxDecomprTable[256] _ALIGN(128);

// g_scBoxDecomprTable heps on consoles (no difference on PC)
void CVegetation::InitVegDecomprTable()
{
    const float cDecompFactor = (VEGETATION_CONV_FACTOR / 255.f);
    for (int i = 0; i < 256; ++i)
    {
        g_scBoxDecomprTable[i] = (float)i * cDecompFactor;
    }
}

CVegetation::CVegetation()
    : m_pInstancingInfo{}
{
    Init();

    GetInstCount(eERType_Vegetation)++;
}

void CVegetation::Init()
{
    m_nObjectTypeIndex = 0;
    m_vPos.Set(0, 0, 0);
    m_ucScale = 0;
    m_pPhysEnt = 0;
    m_ucAngle = 0;
    m_ucAngleX = 0;
    m_ucAngleY = 0;
    m_ucSunDotTerrain = 255;
    m_pRNTmpData = NULL;
    m_pDeformable = NULL;
    m_bApplyPhys = false;
}

//////////////////////////////////////////////////////////////////////////
void CVegetation::CalcMatrix(Matrix34A& tm, int* pFObjFlags)
{
    Matrix34A matTmp;
    int orFlags = 0;

    matTmp.SetIdentity();

    if (float fAngle = GetZAngle())
    {
        matTmp.SetRotationZ(fAngle);
    }

    StatInstGroup& vegetGroup = GetStatObjGroup();

    if (vegetGroup.GetAlignToTerrainAmount() != 0.f)
    {
        Matrix33 m33;
        GetTerrain()->GetTerrainAlignmentMatrix(m_vPos, vegetGroup.GetAlignToTerrainAmount(), m33);
        matTmp = m33 * matTmp;
    }
    else
    {
        matTmp = Matrix34::CreateRotationXYZ(Ang3(BYTE2RAD(m_ucAngleX), BYTE2RAD(m_ucAngleY), GetZAngle()));
    }

    float fScale = GetScale();
    if (fScale != 1.0f)
    {
        Matrix33 m33;
        m33.SetIdentity();
        m33.SetScale(Vec3(fScale, fScale, fScale));
        matTmp = m33 * matTmp;
    }

    matTmp.SetTranslation(m_vPos);

    if (pFObjFlags)
    {
        *pFObjFlags |= (orFlags);
    }

    tm = matTmp;
}

//////////////////////////////////////////////////////////////////////////
AABB CVegetation::CalcBBox()
{
    AABB WSBBox = GetStatObj()->GetAABB();
    //WSBBox.min.z = 0;

    if (m_ucAngle || m_ucAngleX || m_ucAngleY)
    {
        Matrix34A tm;
        CalcMatrix(tm);
        WSBBox.SetTransformedAABB(tm, WSBBox);
    }
    else
    {
        float fScale = GetScale();
        WSBBox.min = m_vPos + WSBBox.min * fScale;
        WSBBox.max = m_vPos + WSBBox.max * fScale;
    }

    SetBBox(WSBBox);

    return WSBBox;
}

CLodValue CVegetation::ComputeLod(int wantedLod, const SRenderingPassInfo& passInfo)
{
    uint8 nDissolveRefA = 0;
    int nLodA = -1;
    int nLodB = -1;

    StatInstGroup& vegetGroup = GetStatObjGroup();
    if (IStatObj* pStatObj = vegetGroup.GetStatObj())
    {
        const Vec3 vCamPos = passInfo.GetCamera().GetPosition();

        int minUsableLod = pStatObj->GetMinUsableLod();
        int maxUsableLod = (int)pStatObj->GetMaxUsableLod();
        nLodA = CLAMP(wantedLod, minUsableLod, maxUsableLod);
        nLodA = pStatObj->FindNearesLoadedLOD(nLodA);

        if (passInfo.IsGeneralPass())
        {
            if (GetCVars()->e_Dissolve)
            {
                const float fEntDistance2D = sqrt_tpl(vCamPos.GetSquaredDistance2D(m_vPos)) * passInfo.GetZoomFactor();

                SLodDistDissolveTransitionState* pLodDistDissolveTransitionState = &m_pRNTmpData->userData.lodDistDissolveTransitionState;

                // when we first load before streaming we get a lod of -1. When a lod streams in
                // we kick off a transition to N, but without moving there's nothing to continue the transition.
                // Catch this case when we claim to be in lod -1 but have no sprite info, and snap.
                if (pLodDistDissolveTransitionState->nOldLod == -1)
                {
                    pLodDistDissolveTransitionState->nOldLod = pLodDistDissolveTransitionState->nNewLod;
                }

                float fDissolve = GetObjManager()->GetLodDistDissolveRef(pLodDistDissolveTransitionState, fEntDistance2D, nLodA, passInfo);

                nDissolveRefA = (uint8)(255.f * SATURATE(fDissolve));
                nLodA = pLodDistDissolveTransitionState->nOldLod;
                nLodB = pLodDistDissolveTransitionState->nNewLod;

                minUsableLod = min(minUsableLod, -1); // allow for sprites.

                nLodA = CLAMP(nLodA, minUsableLod, maxUsableLod);
                nLodB = CLAMP(nLodB, minUsableLod, maxUsableLod);
            }
        }

        if (GetCVars()->e_Dissolve && !passInfo.IsCachedShadowPass())
        {
            float fDissolveDist = CLAMP(0.1f * m_fWSMaxViewDist, GetFloatCVar(e_DissolveDistMin), GetFloatCVar(e_DissolveDistMax));

            const float fDissolveStartDist = sqr(m_fWSMaxViewDist - fDissolveDist);

            AABB bbox;
            FillBBox_NonVirtual(bbox);
            float fEntDistanceSq = Distance::Point_AABBSq(vCamPos, bbox) * sqr(passInfo.GetZoomFactor());

            if (fEntDistanceSq > fDissolveStartDist)
            {
                float fDissolve = (sqrt(fEntDistanceSq) - (m_fWSMaxViewDist - fDissolveDist))
                    / fDissolveDist;
                nDissolveRefA = (uint8)(255.f * SATURATE(fDissolve));
                nLodB = -1;
            }
        }
    }

    return CLodValue(nLodA, nDissolveRefA, nLodB);
}

//////////////////////////////////////////////////////////////////////////
bool CVegetation::CanExecuteRenderAsJob()
{
    if (m_pDeformable)
    {
        return false;
    }
    return true;
}

void CVegetation::Render(const SRenderingPassInfo& passInfo, const CLodValue& lodValue, SSectorTextureSet* pTerrainTexInfo, const SRendItemSorter& rendItemSorter) const
{
    FUNCTION_PROFILER_3DENGINE;
    StatInstGroup& vegetGroup = GetStatObjGroup();

    IStatObj* pStatObj = vegetGroup.GetStatObj();

    if (!pStatObj)
    {
        return;
    }

    const Vec3 vCamPos = passInfo.GetCamera().GetPosition();
    const Vec3 vObjCenter = GetBBox().GetCenter();
    const Vec3 vObjPos = GetPos();

    float fEntDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, GetBBox())) * passInfo.GetZoomFactor();
    float fEntDistance2D = sqrt_tpl(vCamPos.GetSquaredDistance2D(m_vPos)) * passInfo.GetZoomFactor();
    bool bUseTerrainColor((vegetGroup.bUseTerrainColor && GetCVars()->e_VegetationUseTerrainColor) || GetCVars()->e_VegetationUseTerrainColor == 2);
    CRNTmpData::SRNUserData& userData = m_pRNTmpData->userData;

    CRenderObject* pRenderObject = gEnv->pRenderer->EF_GetObject_Temp(passInfo.ThreadID());
    pRenderObject->m_pRenderNode = const_cast<IRenderNode*>(static_cast<const IRenderNode*>(this));
    pRenderObject->m_II.m_Matrix = m_pRNTmpData->userData.objMat;
    pRenderObject->m_fAlpha = 1.f;
    pRenderObject->m_ObjFlags |= FOB_DYNAMIC_OBJECT;

    if (bUseTerrainColor)
    {
        pRenderObject->m_ObjFlags |= FOB_BLEND_WITH_TERRAIN_COLOR;
    }
    else
    {
        pRenderObject->m_ObjFlags &= ~FOB_BLEND_WITH_TERRAIN_COLOR;
    }

    // set lights bit mask
    pRenderObject->m_fDistance = fEntDistance;
    pRenderObject->m_nSort = fastround_positive(fEntDistance * 2.0f);

    if (uint8 nMaterialLayers = GetMaterialLayers())
    {
        uint8 nFrozenLayer = (nMaterialLayers & MTL_LAYER_FROZEN) ? MTL_LAYER_FROZEN_MASK : 0;
        uint8 nWetLayer = (nMaterialLayers & MTL_LAYER_WET) ? MTL_LAYER_WET_MASK : 0;
        pRenderObject->m_nMaterialLayers = (uint32)(nFrozenLayer << 24) | (nWetLayer << 16);
    }
    else
    {
        pRenderObject->m_nMaterialLayers = 0;
    }

    if (vegetGroup.pMaterial)
    {
        pRenderObject->m_pCurrMaterial = vegetGroup.pMaterial;
    }
    else if (vegetGroup.GetStatObj())
    {
        pRenderObject->m_pCurrMaterial = vegetGroup.GetStatObj()->GetMaterial();
    }

    if (bUseTerrainColor)
    {
        pRenderObject->m_II.m_AmbColor.a = (1.0f / 255.f * m_ucSunDotTerrain);
    }
    else
    {
        pRenderObject->m_II.m_AmbColor.a = vegetGroup.fBrightness;
    }

    if (pRenderObject->m_II.m_AmbColor.a > 1.f)
    {
        pRenderObject->m_II.m_AmbColor.a = 1.f;
    }

    float fRenderQuality = min(1.f, max(1.f - fEntDistance2D / (m_fWSMaxViewDist * 0.3333f), 0.f));

    pRenderObject->m_nRenderQuality = (uint16)(fRenderQuality * 65535.0f);

    if (pTerrainTexInfo)
    {
        pRenderObject->m_nRenderQuality = (uint16)(fRenderQuality * 65535.0f);
    }

    if (pTerrainTexInfo)
    {
        if (pRenderObject->m_ObjFlags & (FOB_BLEND_WITH_TERRAIN_COLOR))
        {
            pRenderObject->m_nTextureID = pTerrainTexInfo->nTex0;
            SRenderObjData* pOD = GetRenderer()->EF_GetObjData(pRenderObject, true, passInfo.ThreadID());

            pOD->m_fTempVars[3] = pTerrainTexInfo->fTexOffsetX;
            pOD->m_fTempVars[4] = pTerrainTexInfo->fTexOffsetY;
            pOD->m_fTempVars[5] = pTerrainTexInfo->fTexScale;

            if (GetCVars()->e_VegetationUseTerrainColorDistance == 0)
            {
                pOD->m_fTempVars[8] = m_fWSMaxViewDist * 0.3333f; // 0.333f => replicating magic ratio from fRenderQuality
            }
            else if (GetCVars()->e_VegetationUseTerrainColorDistance > 0)
            {
                pOD->m_fTempVars[8] = GetCVars()->e_VegetationUseTerrainColorDistance;
            }
            else
            {
                pOD->m_fTempVars[8] = abs(GetCVars()->e_VegetationUseTerrainColorDistance) * vegetGroup.fVegRadius * GetCVars()->e_ViewDistRatioVegetation;
            }
        }
    }

    IFoliage* pFoliage = const_cast<CVegetation*>(this)->GetFoliage();
    if (pFoliage && ((CStatObjFoliage*)pFoliage)->m_pVegInst == this)
    {
        SRenderObjData* pOD = GetRenderer()->EF_GetObjData(pRenderObject, true, passInfo.ThreadID());
        if (pOD)
        {
            pOD->m_pSkinningData = pFoliage->GetSkinningData(pRenderObject->m_II.m_Matrix, passInfo);
            pRenderObject->m_ObjFlags |= FOB_SKINNED | FOB_DYNAMIC_OBJECT;
            pFoliage->SetFlags(pFoliage->GetFlags() & ~IFoliage::FLAG_FROZEN | -(int)(pRenderObject->m_nMaterialLayers & MTL_LAYER_FROZEN) & IFoliage::FLAG_FROZEN);
        }
    }

    // Physicalized foliage might have not found the correct stencil ref during onrendernodebecomevisible
    // because it can be called from the physics callback.
    // A query for the visareastencilref is therefore issued every time it is rendered.
    pRenderObject->m_nClipVolumeStencilRef = 0;
    if (m_pOcNode && m_pOcNode->m_pVisArea)
    {
        pRenderObject->m_nClipVolumeStencilRef = ((IVisArea*)m_pOcNode->m_pVisArea)->GetStencilRef();
    }
    else if (userData.m_pClipVolume)
    {
        pRenderObject->m_nClipVolumeStencilRef = userData.m_pClipVolume->GetStencilRef();
    }


    AABB bbox;
    FillBBox_NonVirtual(bbox);
    if (m_bEditor) // editor can modify material anytime
    {
        if (vegetGroup.pMaterial)
        {
            pRenderObject->m_pCurrMaterial = vegetGroup.pMaterial;
        }
        else
        {
            pRenderObject->m_pCurrMaterial = pStatObj->GetMaterial();
        }
    }

    // check the object against the water level
    if (GetObjManager()->IsAfterWater(vObjCenter, passInfo))
    {
        pRenderObject->m_ObjFlags |= FOB_AFTER_WATER;
    }
    else
    {
        pRenderObject->m_ObjFlags &= ~FOB_AFTER_WATER;
    }

    CRenderObject* pRenderObjectFin = pRenderObject;

    bool duplicated = false;
    if (m_pRNTmpData->userData.m_pFoliage) // fix for vegetation flicker when switching back from foliage mode
    {
        pRenderObjectFin = GetRenderer()->EF_DuplicateRO(pRenderObject, passInfo);
        if (SRenderObjData* objData = GetRenderer()->EF_GetObjData(pRenderObjectFin, false, passInfo.ThreadID()))
        {
            if (!objData->m_pSkinningData)
            {
                pRenderObjectFin->m_ObjFlags &= ~FOB_SKINNED;
            }
            else
            {
                pRenderObjectFin->m_ObjFlags |= FOB_SKINNED;
            }
        }
        duplicated = true;
    }
    Get3DEngine()->SetupBending(pRenderObject, this, pStatObj->GetRadiusVert(), passInfo, duplicated);

    if (Get3DEngine()->IsTessellationAllowed(pRenderObject, passInfo))
    {
        // Allow this RO to be tessellated, however actual tessellation will be applied if enabled in material
        pRenderObject->m_ObjFlags |= FOB_ALLOW_TESSELLATION;
    }
    else
    {
        pRenderObject->m_ObjFlags &= ~FOB_ALLOW_TESSELLATION;
    }
    pStatObj->RenderInternal(pRenderObject, 0, lodValue, passInfo, rendItemSorter, false);

    if (m_pDeformable)
    {
        m_pDeformable->RenderInternalDeform(pRenderObject, lodValue.LodA(), GetBBox(), passInfo, rendItemSorter);
    }

    if (GetCVars()->e_BBoxes)
    {
        GetObjManager()->RenderObjectDebugInfo((IRenderNode*)this, pRenderObject->m_fDistance, passInfo);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetation::Physicalize(bool bInstant)
{
    FUNCTION_PROFILER_3DENGINE;

    StatInstGroup& vegetGroup = GetStatObjGroup();

    IStatObj* pBody = vegetGroup.GetStatObj();
    if (!pBody)
    {
        return;
    }

    bool bHideability = vegetGroup.bHideability;
    bool bHideabilitySecondary = vegetGroup.bHideabilitySecondary;

    //////////////////////////////////////////////////////////////////////////
    // Not create instance if no physical geometry.
    if (!pBody->GetArrPhysGeomInfo()[PHYS_GEOM_TYPE_DEFAULT] && !(pBody->GetFlags() & STATIC_OBJECT_COMPOUND))
    {
        //no bHidability check for the E3 demo - make all the bushes with MAT_OBSTRUCT things soft cover
        //if(!(pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_OBSTRUCT] && (bHideability || pBody->m_nSpines)))
        if (!(pBody->GetArrPhysGeomInfo()[PHYS_GEOM_TYPE_OBSTRUCT]))
        {
            if (!(pBody->GetArrPhysGeomInfo()[PHYS_GEOM_TYPE_NO_COLLIDE]))
            {
                return;
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////
    m_bApplyPhys = true;

    AABB WSBBox = GetBBox();

    //  WriteLock lock(g_lockVegetationPhysics);
    int bNoOnDemand =
        !(GetCVars()->e_OnDemandPhysics & 0x1) || max(WSBBox.max.x - WSBBox.min.x, WSBBox.max.y - WSBBox.min.y) > Get3DEngine()->GetCVars()->e_OnDemandMaxSize;
    if (bNoOnDemand)
    {
        bInstant = true;
    }

    if (!bInstant)
    {
        gEnv->pPhysicalWorld->RegisterBBoxInPODGrid(&WSBBox.min);
        return;
    }

    // create new
    pe_params_pos pp;
    Matrix34A mtx;
    CalcMatrix(mtx);
    Matrix34 mtxNA = mtx;
    pp.pMtx3x4 = &mtxNA;
    float fScale = GetScale();
    //pp.pos = m_vPos;
    //pp.q.SetRotationXYZ( Ang3(0,0,GetZAngle()) );
    //pp.scale = fScale;
    if (m_pPhysEnt)
    {
        Dephysicalize();
    }

    m_pPhysEnt = GetSystem()->GetIPhysicalWorld()->CreatePhysicalEntity(PE_STATIC, (1 - bNoOnDemand) * 5.0f, &pp,
            (IRenderNode*)this, PHYS_FOREIGN_ID_STATIC);
    if (!m_pPhysEnt)
    {
        return;
    }

    pe_geomparams params;
    params.density = 800;
    params.flags |= geom_break_approximation;
    params.scale = fScale;

    if (pBody->GetFlags() & STATIC_OBJECT_COMPOUND)
    {
        Matrix34 scaleMat;
        scaleMat.Set(Vec3(fScale), Quat(IDENTITY), Vec3(ZERO));
        params.pMtx3x4 = &scaleMat;
        pBody->Physicalize(m_pPhysEnt, &params);
    }

    // collidable
    if (pBody->GetArrPhysGeomInfo()[PHYS_GEOM_TYPE_DEFAULT])
    {
        pe_params_ground_plane pgp;
        Vec3 bbox[2] = { pBody->GetBoxMin(), pBody->GetBoxMax() };
        pgp.iPlane = 0;
        pgp.ground.n.Set(0, 0, 1);
        (pgp.ground.origin = (bbox[0] + bbox[1]) * 0.5f).z -= (bbox[1].z - bbox[0].z) * 0.49f;
        pgp.ground.origin *= fScale;

        m_pPhysEnt->SetParams(&pgp, 1);

        if (pBody->GetFlags() & STATIC_OBJECT_NO_PLAYER_COLLIDE)
        {
            params.flags &= ~geom_colltype_player;
        }

        if (!(m_dwRndFlags & ERF_PROCEDURAL))
        {
            params.idmatBreakable = pBody->GetIDMatBreakable();
            if (pBody->GetBreakableByGame())
            {
                params.flags |= geom_manually_breakable;
            }
        }
        else
        {
            params.idmatBreakable = -1;
        }
        m_pPhysEnt->AddGeometry(pBody->GetArrPhysGeomInfo()[PHYS_GEOM_TYPE_DEFAULT], &params, -1, 1);
    }

    phys_geometry* pgeom;
    params.density = 2;
    params.idmatBreakable = -1;
    if (pBody->GetArrPhysGeomInfo()[PHYS_GEOM_TYPE_NO_COLLIDE] && pBody->GetArrPhysGeomInfo()[PHYS_GEOM_TYPE_OBSTRUCT])
    {
        params.minContactDist = GetFloatCVar(e_FoliageStiffness);
        params.flags = geom_squashy | geom_colltype_obstruct;
        m_pPhysEnt->AddGeometry(pBody->GetArrPhysGeomInfo()[PHYS_GEOM_TYPE_OBSTRUCT], &params, 1024, 1);

        if (!gEnv->IsDedicated() && GetCVars()->e_PhysFoliage >= 2)
        {
            params.density = 0;
            params.flags = geom_log_interactions;
            params.flagsCollider = 0;
            if (pBody->GetSpineCount())
            {
                params.flags |= geom_colltype_foliage_proxy;
            }
            m_pPhysEnt->AddGeometry(pBody->GetArrPhysGeomInfo()[PHYS_GEOM_TYPE_NO_COLLIDE], &params, 2048, 1);
        }
    }
    else if ((pgeom = pBody->GetArrPhysGeomInfo()[PHYS_GEOM_TYPE_NO_COLLIDE]) || (pgeom = pBody->GetArrPhysGeomInfo()[PHYS_GEOM_TYPE_OBSTRUCT]))
    {
        params.minContactDist = GetFloatCVar(e_FoliageStiffness);
        params.flags = geom_log_interactions | geom_squashy;
        if (pBody->GetArrPhysGeomInfo()[PHYS_GEOM_TYPE_OBSTRUCT])
        {
            params.flags |= geom_colltype_obstruct;
        }
        if (pBody->GetSpineCount())
        {
            params.flags |= geom_colltype_foliage_proxy;
        }
        m_pPhysEnt->AddGeometry(pgeom, &params, 1024, 1);
    }

    if (bHideability)
    {
        pe_params_foreign_data  foreignData;
        m_pPhysEnt->GetParams(&foreignData);
        foreignData.iForeignFlags |= PFF_HIDABLE;
        m_pPhysEnt->SetParams(&foreignData, 1);
    }

    if (bHideabilitySecondary)
    {
        pe_params_foreign_data  foreignData;
        m_pPhysEnt->GetParams(&foreignData);
        foreignData.iForeignFlags |= PFF_HIDABLE_SECONDARY;
        m_pPhysEnt->SetParams(&foreignData, 1);
    }

    //PhysicalizeFoliage();
}

bool CVegetation::PhysicalizeFoliage(bool bPhysicalize, int iSource, int nSlot)
{
    IStatObj* pBody = GetStatObj();
    if (!pBody || !pBody->GetSpines())
    {
        return false;
    }

    if (bPhysicalize)
    {
        // we need to create a temporary SRenderingPassInfo structuture here to pass into CheckCreateRNTmpData,
        // CheckCreateRNTmpData, can call OnBecomeVisible, which uses the camera
        Get3DEngine()->CheckCreateRNTmpData(&m_pRNTmpData, this, SRenderingPassInfo::CreateGeneralPassRenderingInfo(gEnv->p3DEngine->GetRenderingCamera()));
        gEnv->p3DEngine->Tick(); // clear temp stored Cameras to prevent wasting space due many temp cameras
        assert(m_pRNTmpData);

        Matrix34A mtx;
        CalcMatrix(mtx);
        if (pBody->PhysicalizeFoliage(m_pPhysEnt, mtx, m_pRNTmpData->userData.m_pFoliage, GetCVars()->e_FoliageBranchesTimeout, iSource)
            )//&& !pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_DEFAULT])
        {
            ((CStatObjFoliage*)m_pRNTmpData->userData.m_pFoliage)->m_pVegInst = this;
        }
    }
    else if (m_pRNTmpData && m_pRNTmpData->userData.m_pFoliage)
    {
        m_pRNTmpData->userData.m_pFoliage->Release();
        m_pRNTmpData->userData.m_pFoliage = NULL;
    }

    return m_pRNTmpData && m_pRNTmpData->userData.m_pFoliage;
}

IRenderNode* CVegetation::Clone() const
{
    CVegetation* pDestVeg = new CVegetation();

    //CVegetation member vars
    pDestVeg->m_vPos                    = m_vPos;
    pDestVeg->m_nObjectTypeIndex        = m_nObjectTypeIndex;
    pDestVeg->m_ucAngle                 = m_ucAngle;
    pDestVeg->m_ucAngleX                = m_ucAngleX;
    pDestVeg->m_ucAngleY                = m_ucAngleY;
    pDestVeg->m_ucSunDotTerrain         = m_ucSunDotTerrain;
    pDestVeg->m_ucScale                 = m_ucScale;
    pDestVeg->m_boxExtends[0]           = m_boxExtends[0];
    pDestVeg->m_boxExtends[1]           = m_boxExtends[1];
    pDestVeg->m_boxExtends[2]           = m_boxExtends[2];
    pDestVeg->m_boxExtends[3]           = m_boxExtends[3];
    pDestVeg->m_boxExtends[4]           = m_boxExtends[4];
    pDestVeg->m_boxExtends[5]           = m_boxExtends[5];
    pDestVeg->m_ucRadius                = m_ucRadius;

    //IRenderNode member vars
    //  We cannot just copy over due to issues with the linked list of IRenderNode objects
    pDestVeg->m_fWSMaxViewDist          = m_fWSMaxViewDist;
    pDestVeg->m_dwRndFlags                  = m_dwRndFlags;
    pDestVeg->m_pOcNode                         = m_pOcNode;
    pDestVeg->m_fViewDistanceMultiplier         = m_fViewDistanceMultiplier;
    pDestVeg->m_ucLodRatio                  = m_ucLodRatio;
    pDestVeg->m_cShadowLodBias          = m_cShadowLodBias;
    pDestVeg->m_nInternalFlags          = m_nInternalFlags;
    pDestVeg->m_nMaterialLayers         = m_nMaterialLayers;
    //pDestVeg->m_pRNTmpData                //If this is copied from the source render node, there are two
    //  pointers to the same data, and if either is deleted, there will
    //  be a crash when the dangling pointer is used on the other

    return pDestVeg;
}

void CVegetation::ShutDown()
{
    Dephysicalize();
    Get3DEngine()->FreeRenderNodeState(this); // Also does unregister entity.

    if (m_pRNTmpData)
    {
        Get3DEngine()->FreeRNTmpData(&m_pRNTmpData);
    }
    assert(!m_pRNTmpData);

    SAFE_DELETE(m_pDeformable);
}

CVegetation::~CVegetation()
{
    ShutDown();

    GetInstCount(eERType_Vegetation)--;

    Get3DEngine()->m_lstKilledVegetations.Delete(this);
}

void CVegetation::Dematerialize()
{
}

void CVegetation::Dephysicalize(bool bKeepIfReferenced)
{
    // delete old physics
    //  WriteLock lock(g_lockVegetationPhysics);
    if (m_pPhysEnt && GetSystem()->GetIPhysicalWorld()->DestroyPhysicalEntity(m_pPhysEnt, 4 * (int)bKeepIfReferenced))
    {
        m_pPhysEnt = 0;
        if (m_pRNTmpData && m_pRNTmpData->userData.m_pFoliage)
        {
            m_pRNTmpData->userData.m_pFoliage->Release();
            m_pRNTmpData->userData.m_pFoliage = NULL;
        }
    }
}

void CVegetation::GetMemoryUsage(ICrySizer* pSizer) const
{
    SIZER_COMPONENT_NAME(pSizer, "Vegetation");
    pSizer->AddObject(this, sizeof(*this));
}


const char* CVegetation::GetName() const
{
    const int groupSize = GetStatObjGroupSize();
    const IStatObj* statObj = GetObjManager()->GetListStaticTypes()[0][m_nObjectTypeIndex].GetStatObj();

    return groupSize && statObj ? statObj->GetFilePath() : "StatObjNotSet";
}

//////////////////////////////////////////////////////////////////////////
IRenderMesh* CVegetation::GetRenderMesh(int nLod)
{
    IStatObj*   pStatObj(GetStatObj());

    if (!pStatObj)
    {
        return NULL;
    }

    pStatObj = (CStatObj*)pStatObj->GetLodObject(nLod);

    if (!pStatObj)
    {
        return NULL;
    }

    return pStatObj->GetRenderMesh();
}
//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CVegetation::GetMaterialOverride()
{
    StatInstGroup& vegetGroup = GetStatObjGroup();

    if (vegetGroup.pMaterial)
    {
        return vegetGroup.pMaterial;
    }

    return NULL;
}

float CVegetation::GetZAngle() const
{
    return BYTE2RAD(m_ucAngle);
}

void CVegetation::OnRenderNodeBecomeVisible(const SRenderingPassInfo& passInfo)
{
    assert(m_pRNTmpData);
    CRNTmpData::SRNUserData& userData = m_pRNTmpData->userData;

    Matrix34A mtx;
    CalcMatrix(mtx);
    userData.objMat = mtx;

    UpdateBending();

    const Vec3 vCamPos = passInfo.GetCamera().GetPosition();
    StatInstGroup& vegetGroup = GetStatObjGroup();
    float fEntDistance2D = sqrt_tpl(vCamPos.GetSquaredDistance2D(m_vPos)) * passInfo.GetZoomFactor();
    float fEntDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, GetBBox())) * passInfo.GetZoomFactor();

    if (vegetGroup.bUseTerrainColor)
    {
        UpdateSunDotTerrain();
    }

    userData.nWantedLod = GetObjManager()->GetObjectLOD(this, fEntDistance);

    int nLod = userData.nWantedLod;

    //const float fSpriteSwitchDist = GetSpriteSwitchDist();
    //float fSwitchRange = min(fSpriteSwitchDist * GetCVars()->e_DissolveSpriteDistRatio, GetCVars()->e_DissolveSpriteMinDist);
    const float fSpriteSwitchDist = 8.0f; // From hard-coded defaults
    float fSwitchRange = min(fSpriteSwitchDist * 0.1f, 4.0f); // from hard-coded defaults

    if (fEntDistance2D > (fSpriteSwitchDist - fSwitchRange) && fSpriteSwitchDist + GetFloatCVar(e_DissolveDistband) < m_fWSMaxViewDist)
    {
        nLod = -1;
    }

    userData.lodDistDissolveTransitionState.nNewLod = userData.lodDistDissolveTransitionState.nOldLod = nLod;
    userData.lodDistDissolveTransitionState.fStartDist = 0.0f;
    userData.lodDistDissolveTransitionState.bFarside = false;
}


void CVegetation::UpdateBending()
{
    const StatInstGroup& vegetGroup = GetStatObjGroup();
    if (GetCVars()->e_VegetationBending)
    {
        // main bending scale (not affecting detail bending)
        // size relative scale causing some inconsistency problems in current levels
        // userData.m_Bending.m_fMainBendingScale = min(0.5f * vegetGroup.fBending / (vegetGroup.fVegRadiusVert * GetScale()), 1.f);
        CRNTmpData::SRNUserData& userData = m_pRNTmpData->userData;
        userData.m_Bending.m_fMainBendingScale = 0.1f * vegetGroup.fBending;
    }
}

void CVegetation::AddBending(Vec3 const& v)
{
    if (m_pRNTmpData)
    {
        m_pRNTmpData->userData.vCurrentWind += v;
    }
}


const float CVegetation::GetRadius() const
{
    const float* const __restrict cpDecompTable = g_scBoxDecomprTable;
    return cpDecompTable[m_ucRadius];
}

void CVegetation::SetBBox(const AABB& WSBBox)
{
    SetBoxExtends(m_boxExtends, &m_ucRadius, WSBBox, m_vPos);
}

IFoliage* CVegetation::GetFoliage(int nSlot)
{
    if (m_pRNTmpData)
    {
        return m_pRNTmpData->userData.m_pFoliage;
    }

    return 0;
}

IPhysicalEntity* CVegetation::GetBranchPhys(int idx, int nSlot)
{
    IFoliage* pFoliage = GetFoliage();
    return pFoliage && (unsigned int)idx < (unsigned int)((CStatObjFoliage*)pFoliage)->m_nRopes ? ((CStatObjFoliage*)pFoliage)->m_pRopes[idx] : 0;
}

void CVegetation::SetStatObjGroupIndex(int nVegetationanceGroupId)
{
    m_nObjectTypeIndex = nVegetationanceGroupId;
    CheckCreateDeformable();
}

void CVegetation::SetMatrix(const Matrix34& mat)
{
    m_vPos = mat.GetTranslation();
    if (m_pDeformable)
    {
        m_pDeformable->BakeDeform(mat);
    }
}

void CVegetation::CheckCreateDeformable()
{
    if (IStatObj* pStatObj = GetStatObj())
    {
        if (pStatObj->IsDeformable())
        {
            Matrix34A tm;
            CalcMatrix(tm);
            SAFE_DELETE(m_pDeformable);
            m_pDeformable = new CDeformableNode();
            m_pDeformable->SetStatObj(pStatObj);
            m_pDeformable->CreateDeformableSubObject(true, tm, NULL);
        }
        else
        {
            SAFE_DELETE(m_pDeformable);
        }
    }
}

void CVegetation::OffsetPosition(const Vec3& delta)
{
    if (m_pRNTmpData)
    {
        m_pRNTmpData->OffsetPosition(delta);
    }
    // GetBBox before moving position
    AABB aabb = GetBBox();
    if (m_bApplyPhys)
    {
        gEnv->pPhysicalWorld->UnregisterBBoxInPODGrid(&aabb.min);
    }

    m_vPos += delta;
    aabb.Move(delta);

    // SetBBox after new position is applied
    SetBBox(aabb);
    if (m_bApplyPhys)
    {
        gEnv->pPhysicalWorld->RegisterBBoxInPODGrid(&aabb.min);
    }

    if (m_pPhysEnt)
    {
        pe_params_pos par_pos;
        m_pPhysEnt->GetParams(&par_pos);
        par_pos.pos = m_vPos;
        m_pPhysEnt->SetParams(&par_pos);
    }
}

bool CVegetation::GetLodDistances(const SFrameLodInfo& frameLodInfo, float* distances) const
{
    StatInstGroup& vegetGroup = GetStatObjGroup();
    IStatObj* pStatObj = vegetGroup.GetStatObj();

    const float fEntityLodRatio = GetLodRatioNormalized();
    if (pStatObj && fEntityLodRatio >  0.0f)
    {
        const float fDistMultiplier = 1.0f / (fEntityLodRatio * frameLodInfo.fTargetSize);
        const float lodDistance = pStatObj->GetLodDistance();

        for (uint i = 0; i < SMeshLodInfo::s_nMaxLodCount; ++i)
        {
            distances[i] = lodDistance * (i + 1) * fDistMultiplier;
        }
    }
    else
    {
        for (uint i = 0; i < SMeshLodInfo::s_nMaxLodCount; ++i)
        {
            distances[i] = FLT_MAX;
        }
    }

    return true;
}

int CVegetation::GetStatObjGroupSize() const
{
    return GetObjManager()->GetListStaticTypes()[0].Count();
}

StatInstGroup& CVegetation::GetStatObjGroup() const
{
    return GetObjManager()->GetListStaticTypes()[0][m_nObjectTypeIndex];
}

IStatObj* CVegetation::GetStatObj()
{
    return GetObjManager()->GetListStaticTypes()[0][m_nObjectTypeIndex].GetStatObj();
}


uint8 CVegetation::GetMaterialLayers() const
{
    return GetObjManager()->GetListStaticTypes()[0][m_nObjectTypeIndex].nMaterialLayers;
}

float CVegetation::GetFirstLodDistance() const
{
    StatInstGroup& vegetGroup = GetStatObjGroup();
    CStatObj* pStatObj = static_cast<CStatObj*>(vegetGroup.GetStatObj());
    return pStatObj ? pStatObj->GetLodDistance() : FLT_MAX;
}
