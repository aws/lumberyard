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

// Description : Light sources manager


#include "StdAfx.h"

#include "3dEngine.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "AABBSV.h"
#include "terrain.h"
#include "LightEntity.h"
#include "ObjectsTree.h"
#include "Brush.h"
#include "ClipVolumeManager.h"

ILightSource* C3DEngine::CreateLightSource()
{
    // construct new object
    CLightEntity* pLightEntity = new CLightEntity();

    m_lstStaticLights.Add(pLightEntity);

    return pLightEntity;
}

void C3DEngine::DeleteLightSource(ILightSource* pLightSource)
{
    if (m_lstStaticLights.Delete((CLightEntity*)pLightSource) || pLightSource == m_pSun)
    {
        if (pLightSource == m_pSun)
        {
            m_pSun = NULL;
        }

        delete pLightSource;
    }
    else
    {
        assert(!"Light object not found");
    }
}

void CLightEntity::Release(bool)
{
    Get3DEngine()->UnRegisterEntityDirect(this);
    Get3DEngine()->DeleteLightSource(this);
}

void CLightEntity::SetLightProperties(const CDLight& light)
{
    C3DEngine* engine = Get3DEngine();

    m_light = light;

    m_bShadowCaster = (m_light.m_Flags & DLF_CASTSHADOW_MAPS) != 0;

    m_light.m_fBaseRadius = m_light.m_fRadius;
    m_light.m_fLightFrustumAngle = CLAMP(m_light.m_fLightFrustumAngle, 0.f, (LIGHT_PROJECTOR_MAX_FOV / 2.f));

    if (!(m_light.m_Flags & (DLF_PROJECT | DLF_AREA_LIGHT)))
    {
        m_light.m_fLightFrustumAngle = 90.f / 2.f;
    }

    m_light.m_pOwner = this;

    if (m_light.m_Flags & DLF_ATTACH_TO_SUN)
    {
        m_dwRndFlags |= ERF_RENDER_ALWAYS | ERF_HUD;
    }

    engine->GetLightEntities()->Delete((ILightSource*)this);

    PodArray<ILightSource*>& lightEntities = *engine->GetLightEntities();

    //on consoles we force all lights (except sun) to be deferred
    if (GetCVars()->e_DynamicLightsForceDeferred && !(m_light.m_Flags & (DLF_SUN | DLF_POST_3D_RENDERER)))
    {
        m_light.m_Flags |= DLF_DEFERRED_LIGHT;
    }

    if (light.m_Flags & DLF_DEFERRED_LIGHT)
    {
        lightEntities.Add((ILightSource*)this);
    }
    else
    {
        lightEntities.InsertBefore((ILightSource*)this, 0);
    }
}

void C3DEngine::ResetCasterCombinationsCache()
{
    for (int nSunInUse = 0; nSunInUse < 2; nSunInUse++)
    {
        // clear user counters
        for (ShadowFrustumListsCacheUsers::iterator it = m_FrustumsCacheUsers[nSunInUse].begin(); it != m_FrustumsCacheUsers[nSunInUse].end(); ++it)
        {
            it->second = 0;
        }
    }
}

void C3DEngine::DeleteAllStaticLightSources()
{
    for (int i = 0; i < m_lstStaticLights.Count(); i++)
    {
        delete m_lstStaticLights[i];
    }
    m_lstStaticLights.Reset();

    m_pSun = NULL;
}

void C3DEngine::InitShadowFrustums(const SRenderingPassInfo& passInfo)
{
    assert(passInfo.IsGeneralPass());
    FUNCTION_PROFILER_3DENGINE_LEGACYONLY;
    AZ_TRACE_METHOD();

    if (m_pSun)
    {
        CDLight* pLight = &m_pSun->GetLightProperties();
        CLightEntity* pLightEntity = (CLightEntity*)pLight->m_pOwner;

        if (passInfo.RenderShadows() && (pLight->m_Flags & DLF_CASTSHADOW_MAPS) && pLight->m_Id >= 0)
        {
            pLightEntity->UpdateGSMLightSourceShadowFrustum(passInfo);

            if (pLightEntity->m_pShadowMapInfo)
            {
                pLight->m_pShadowMapFrustums = pLightEntity->m_pShadowMapInfo->pGSM;
            }
        }

        _smart_ptr<IMaterial> pMat = pLightEntity->GetMaterial();
        if (pMat)
        {
            pLight->m_Shader = pMat->GetShaderItem();
        }

        // update copy of light ion the renderer
        if (pLight->m_Id >= 0)
        {
            CDLight* pRndLight = NULL;
            GetRenderer()->EF_Query(EFQ_LightSource, pLight->m_Id, pRndLight);
            assert(pLight->m_Id == pRndLight->m_Id);
            pRndLight->m_pShadowMapFrustums = pLight->m_pShadowMapFrustums;
            pRndLight->m_Shader = pLight->m_Shader;
            pRndLight->m_Flags = pLight->m_Flags;
        }

        // add per object shadow frustums
        m_nCustomShadowFrustumCount = 0;
        if (passInfo.RenderShadows() &&  GetCVars()->e_ShadowsPerObject > 0)
        {
            const uint nFrustumCount = m_lstPerObjectShadows.size();
            if (nFrustumCount > m_lstCustomShadowFrustums.size())
            {
                m_lstCustomShadowFrustums.resize(nFrustumCount);
            }

            for (uint i = 0; i < nFrustumCount; ++i)
            {
                if (m_lstPerObjectShadows[i].pCaster)
                {
                    ShadowMapFrustum* pFr = &m_lstCustomShadowFrustums[i];
                    pFr->m_eFrustumType = ShadowMapFrustum::e_PerObject;

                    CLightEntity::ProcessPerObjectFrustum(pFr, &m_lstPerObjectShadows[i], m_pSun, passInfo);
                    ++m_nCustomShadowFrustumCount;
                }
            }
        }
    }

    if (passInfo.RenderShadows())
    {
        ResetCasterCombinationsCache();
    }
}

void C3DEngine::AddPerObjectShadow(IShadowCaster* pCaster, float fConstBias, float fSlopeBias, float fJitter, const Vec3& vBBoxScale, uint nTexSize)
{
    SPerObjectShadow* pOS = GetPerObjectShadow(pCaster);
    if (!pOS)
    {
        pOS = &m_lstPerObjectShadows.AddNew();
    }

    const bool bRequiresObjTreeUpdate = pOS->pCaster != pCaster;

    pOS->pCaster = pCaster;
    pOS->fConstBias = fConstBias;
    pOS->fSlopeBias = fSlopeBias;
    pOS->fJitter = fJitter;
    pOS->vBBoxScale = vBBoxScale;
    pOS->nTexSize = nTexSize;

    if (bRequiresObjTreeUpdate)
    {
        FRAME_PROFILER("C3DEngine::AddPerObjectShadow", GetSystem(), PROFILE_3DENGINE);
        ObjectsTreeMarkAsUncompiled(static_cast<IRenderNode*>(pCaster));
    }
}

void C3DEngine::RemovePerObjectShadow(IShadowCaster* pCaster)
{
    SPerObjectShadow* pOS = GetPerObjectShadow(pCaster);
    if (pOS)
    {
        FRAME_PROFILER("C3DEngine::RemovePerObjectShadow", GetSystem(), PROFILE_3DENGINE);

        size_t nIndex = (size_t)(pOS - m_lstPerObjectShadows.begin());
        m_lstPerObjectShadows.Delete(nIndex);

        ObjectsTreeMarkAsUncompiled(static_cast<IRenderNode*>(pCaster));
    }
}

struct SPerObjectShadow* C3DEngine::GetPerObjectShadow(IShadowCaster* pCaster)
{
    for (int i = 0; i < m_lstPerObjectShadows.Count(); ++i)
    {
        if (m_lstPerObjectShadows[i].pCaster == pCaster)
        {
            return &m_lstPerObjectShadows[i];
        }
    }

    return NULL;
}

void C3DEngine::GetCustomShadowMapFrustums(ShadowMapFrustum*& arrFrustums, int& nFrustumCount)
{
    arrFrustums = m_lstCustomShadowFrustums.begin();
    nFrustumCount = m_nCustomShadowFrustumCount;
}

//  delete pLight->m_pProjCamera;
//pLight->m_pProjCamera=0;
//if(pLight->m_pShader)
//      SAFE_RELEASE(pLight->m_pShader);
namespace
{
    static inline bool CmpCastShadowFlag(const CDLight* p1, const CDLight* p2)
    {
        // move sun first
        if ((p1->m_Flags & DLF_SUN) > (p2->m_Flags & DLF_SUN))
        {
            return true;
        }
        else if ((p1->m_Flags & DLF_SUN) < (p2->m_Flags & DLF_SUN))
        {
            return false;
        }

        // move shadow casters first
        if ((p1->m_Flags & DLF_CASTSHADOW_MAPS) > (p2->m_Flags & DLF_CASTSHADOW_MAPS))
        {
            return true;
        }
        else if ((p1->m_Flags & DLF_CASTSHADOW_MAPS) < (p2->m_Flags & DLF_CASTSHADOW_MAPS))
        {
            return false;
        }

        // get some sorting consistency for shadow casters
        if (p1->m_pOwner > p2->m_pOwner)
        {
            return true;
        }
        else if (p1->m_pOwner < p2->m_pOwner)
        {
            return false;
        }

        return false;
    }
}

//////////////////////////////////////////////////////////////////////////

void C3DEngine::SubmitSun(const SRenderingPassInfo& passInfo)
{
    assert(passInfo.IsGeneralPass());
    FUNCTION_PROFILER_3DENGINE_LEGACYONLY;
    AZ_TRACE_METHOD();

    if (m_pSun)
    {
        CDLight* light = &m_pSun->GetLightProperties();

        GetRenderer()->EF_ADDDlight(light, passInfo);

        SetupLightScissors(light, passInfo);
    }
}

void C3DEngine::SetupLightScissors(CDLight* pLight, const SRenderingPassInfo& passInfo)
{
    Vec3 vViewVec = pLight->m_Origin - passInfo.GetCamera().GetPosition();
    float fDistToLS =  vViewVec.GetLength();

    // Use max of width/height for area lights.
    float fMaxRadius = pLight->m_fRadius;

    //TODO Stereo:: for stereo/vr, this code isn't using the per eye projection matrices, so its wrong. This causes the light to get cut off on one side of the square for each eye.
    //can just multiply fMaxRadius by a sufficently large number as a hack if needed to mitigate the problem until a real solution is implemented.
    //see https://issues.labcollab.net/browse/LMBR-5381

    if (pLight->m_Flags & DLF_AREA_LIGHT) // Use max for area lights.
    {
        fMaxRadius += max(pLight->m_fAreaWidth, pLight->m_fAreaHeight);
    }
    else if (pLight->m_Flags & DLF_DEFERRED_CUBEMAPS)
    {
        fMaxRadius = pLight->m_ProbeExtents.len();  // This is not optimal for a box
    }
    ITexture* pLightTexture = pLight->m_pLightImage ? pLight->m_pLightImage : NULL;
    bool bProjectiveLight = (pLight->m_Flags & DLF_PROJECT) && pLightTexture && !(pLightTexture->GetFlags() & FT_REPLICATE_TO_ALL_SIDES);
    bool bInsideLightVolume = fDistToLS <= fMaxRadius;
    if (bInsideLightVolume && !bProjectiveLight)
    {
        //optimization when we are inside light frustum
        pLight->m_sX = 0;
        pLight->m_sY = 0;
        pLight->m_sWidth  = GetRenderer()->GetWidth();
        pLight->m_sHeight = GetRenderer()->GetHeight();

        return;
    }

    Matrix44 mProj, mView;
    GetRenderer()->GetProjectionMatrix(mProj.GetData());
    GetRenderer()->GetModelViewMatrix(mView.GetData());

    Vec3 vCenter = pLight->m_Origin;
    float fRadius = fMaxRadius;

    const int nMaxVertsToProject = 10;
    int nVertsToProject = 4;
    Vec3 pBRectVertices[nMaxVertsToProject];

    Vec4 vCenterVS = Vec4(vCenter, 1) * mView;

    if (!bInsideLightVolume)
    {
        // Compute tangent planes
        float r = fRadius;
        float sq_r = r * r;

        Vec3 vLPosVS = Vec3(vCenterVS.x, vCenterVS.y, vCenterVS.z);
        float lx = vLPosVS.x;
        float ly = vLPosVS.y;
        float lz = vLPosVS.z;
        float sq_lx = lx * lx;
        float sq_ly = ly * ly;
        float sq_lz = lz * lz;

        // Compute left and right tangent planes to light sphere
        float sqrt_d = sqrt_tpl(max(sq_r * sq_lx  - (sq_lx + sq_lz) * (sq_r - sq_lz), 0.0f));
        float nx = iszero(sq_lx + sq_lz) ? 1.0f : (r * lx + sqrt_d) / (sq_lx + sq_lz);
        float nz = iszero(lz) ? 1.0f : (r - nx * lx) / lz;

        Vec3 vTanLeft = Vec3(nx, 0, nz).normalized();

        nx = iszero(sq_lx + sq_lz) ? 1.0f : (r * lx - sqrt_d) / (sq_lx + sq_lz);
        nz = iszero(lz) ? 1.0f : (r - nx * lx) / lz;
        Vec3 vTanRight = Vec3(nx, 0, nz).normalized();

        pBRectVertices[0] = vLPosVS - r * vTanLeft;
        pBRectVertices[1] = vLPosVS - r * vTanRight;

        // Compute top and bottom tangent planes to light sphere
        sqrt_d = sqrt_tpl(max(sq_r * sq_ly  - (sq_ly + sq_lz) * (sq_r - sq_lz), 0.0f));
        float ny = iszero(sq_ly + sq_lz) ? 1.0f : (r * ly - sqrt_d) / (sq_ly + sq_lz);
        nz = iszero(lz) ? 1.0f : (r - ny * ly) / lz;
        Vec3 vTanBottom = Vec3(0, ny, nz).normalized();

        ny = iszero(sq_ly + sq_lz) ? 1.0f :  (r * ly + sqrt_d) / (sq_ly + sq_lz);
        nz = iszero(lz) ? 1.0f : (r - ny * ly) / lz;
        Vec3 vTanTop = Vec3(0, ny, nz).normalized();

        pBRectVertices[2] = vLPosVS - r * vTanTop;
        pBRectVertices[3] = vLPosVS - r * vTanBottom;
    }

    if (bProjectiveLight)
    {
        // todo: improve/simplify projective case

        Vec3 vRight  = pLight->m_ObjMatrix.GetColumn2();
        Vec3 vUp      = -pLight->m_ObjMatrix.GetColumn1();
        Vec3 pDirFront = pLight->m_ObjMatrix.GetColumn0();
        pDirFront.NormalizeFast();

        // Cone radius
        float fConeAngleThreshold = 0.0f;
        float fConeRadiusScale = /*min(1.0f,*/ tan_tpl((pLight->m_fLightFrustumAngle + fConeAngleThreshold) * (gf_PI / 180.0f));  //);
        float fConeRadius = fRadius * fConeRadiusScale;

        Vec3 pDiagA = (vUp + vRight);
        float fDiagLen = 1.0f / pDiagA.GetLengthFast();
        pDiagA *= fDiagLen;

        Vec3 pDiagB = (vUp - vRight);
        pDiagB *= fDiagLen;

        float fPyramidBase =  sqrt_tpl(fConeRadius * fConeRadius * 2.0f);
        pDirFront *= fRadius;

        Vec3 pEdgeA  = (pDirFront + pDiagA * fPyramidBase);
        Vec3 pEdgeA2 = (pDirFront - pDiagA * fPyramidBase);
        Vec3 pEdgeB  = (pDirFront + pDiagB * fPyramidBase);
        Vec3 pEdgeB2 = (pDirFront - pDiagB * fPyramidBase);

        uint32 nOffset = 4;

        // Check whether the camera is inside the extended bounding sphere that contains pyramid

        // we are inside light frustum
        // Put all pyramid vertices in view space
        Vec4 pPosVS = Vec4(vCenter, 1) * mView;
        pBRectVertices[nOffset++] = Vec3(pPosVS.x, pPosVS.y, pPosVS.z);
        pPosVS = Vec4(vCenter + pEdgeA, 1) * mView;
        pBRectVertices[nOffset++] = Vec3(pPosVS.x, pPosVS.y, pPosVS.z);
        pPosVS = Vec4(vCenter + pEdgeB, 1) * mView;
        pBRectVertices[nOffset++] = Vec3(pPosVS.x, pPosVS.y, pPosVS.z);
        pPosVS = Vec4(vCenter + pEdgeA2, 1) * mView;
        pBRectVertices[nOffset++] = Vec3(pPosVS.x, pPosVS.y, pPosVS.z);
        pPosVS = Vec4(vCenter + pEdgeB2, 1) * mView;
        pBRectVertices[nOffset++] = Vec3(pPosVS.x, pPosVS.y, pPosVS.z);

        nVertsToProject = nOffset;
    }

    Vec3 vPMin = Vec3(1, 1, 999999.0f);
    Vec2 vPMax = Vec2(0, 0);
    Vec2 vMin = Vec2(1, 1);
    Vec2 vMax = Vec2(0, 0);

    int nStart = 0;

    if (bInsideLightVolume)
    {
        nStart = 4;
        vMin = Vec2(0, 0);
        vMax = Vec2(1, 1);
    }

    if (GetCVars()->e_ScissorDebug)
    {
        mView.Invert();
    }

    // Project all vertices
    for (int i = nStart; i < nVertsToProject; i++)
    {
        if (GetCVars()->e_ScissorDebug)
        {
            if (GetRenderer()->GetIRenderAuxGeom() != NULL)
            {
                Vec4 pVertWS = Vec4(pBRectVertices[i], 1) * mView;
                Vec3 v = Vec3(pVertWS.x, pVertWS.y, pVertWS.z);
                GetRenderer()->GetIRenderAuxGeom()->DrawPoint(v, RGBA8(0xff, 0xff, 0xff, 0xff), 10);

                int32 nPrevVert = (i - 1) < 0 ? nVertsToProject - 1 : (i - 1);
                pVertWS = Vec4(pBRectVertices[nPrevVert], 1) * mView;
                Vec3 v2 = Vec3(pVertWS.x, pVertWS.y, pVertWS.z);
                GetRenderer()->GetIRenderAuxGeom()->DrawLine(v, RGBA8(0xff, 0xff, 0x0, 0xff), v2, RGBA8(0xff, 0xff, 0x0, 0xff), 3.0f);
            }
        }

        Vec4 vScreenPoint = Vec4(pBRectVertices[i], 1.0) * mProj;

        //projection space clamping
        vScreenPoint.w = max(vScreenPoint.w, 0.00000000000001f);
        vScreenPoint.x = max(vScreenPoint.x, -(vScreenPoint.w));
        vScreenPoint.x = min(vScreenPoint.x, vScreenPoint.w);
        vScreenPoint.y = max(vScreenPoint.y, -(vScreenPoint.w));
        vScreenPoint.y = min(vScreenPoint.y, vScreenPoint.w);

        //NDC
        vScreenPoint /= vScreenPoint.w;

        //output coords
        //generate viewport (x=0,y=0,height=1,width=1)
        Vec2 vWin;
        vWin.x = (1.0f + vScreenPoint.x) *  0.5f;
        vWin.y = (1.0f + vScreenPoint.y) *  0.5f;  //flip coords for y axis

        assert(vWin.x >= 0.0f && vWin.x <= 1.0f);
        assert(vWin.y >= 0.0f && vWin.y <= 1.0f);

        if (bProjectiveLight && i >= 4)
        {
            // Get light pyramid screen bounds
            vPMin.x = min(vPMin.x, vWin.x);
            vPMin.y = min(vPMin.y, vWin.y);
            vPMax.x = max(vPMax.x, vWin.x);
            vPMax.y = max(vPMax.y, vWin.y);

            vPMin.z = min(vPMin.z, vScreenPoint.z); // if pyramid intersects the nearplane, the test is unreliable. (requires proper clipping)
        }
        else
        {
            // Get light sphere screen bounds
            vMin.x = min(vMin.x, vWin.x);
            vMin.y = min(vMin.y, vWin.y);
            vMax.x = max(vMax.x, vWin.x);
            vMax.y = max(vMax.y, vWin.y);
        }
    }

    int iWidth = GetRenderer()->GetWidth();
    int iHeight = GetRenderer()->GetHeight();
    float fWidth = (float)iWidth;
    float fHeight = (float)iHeight;

    if (bProjectiveLight)
    {
        vPMin.x = (float)fsel(vPMin.z, vPMin.x, vMin.x); // Use sphere bounds if pyramid bounds are unreliable
        vPMin.y = (float)fsel(vPMin.z, vPMin.y, vMin.y);
        vPMax.x = (float)fsel(vPMin.z, vPMax.x, vMax.x);
        vPMax.y = (float)fsel(vPMin.z, vPMax.y, vMax.y);

        // Clamp light pyramid bounds to light sphere screen bounds
        vMin.x = clamp_tpl<float>(vPMin.x, vMin.x, vMax.x);
        vMin.y = clamp_tpl<float>(vPMin.y, vMin.y, vMax.y);
        vMax.x = clamp_tpl<float>(vPMax.x, vMin.x, vMax.x);
        vMax.y = clamp_tpl<float>(vPMax.y, vMin.y, vMax.y);
    }

    pLight->m_sX = (short)(vMin.x * fWidth);
    pLight->m_sY = (short)((1.0f - vMax.y) * fHeight);
    pLight->m_sWidth = (short)ceilf((vMax.x - vMin.x) * fWidth);
    pLight->m_sHeight = (short)ceilf((vMax.y - vMin.y) * fHeight);

    // make sure we don't create a scissor rect out of bound (D3DError)
    pLight->m_sWidth = (pLight->m_sX + pLight->m_sWidth) > iWidth ? iWidth - pLight->m_sX : pLight->m_sWidth;
    pLight->m_sHeight = (pLight->m_sY + pLight->m_sHeight) > iHeight ? iHeight - pLight->m_sY : pLight->m_sHeight;

#if !defined(RELEASE)
    if (GetCVars()->e_ScissorDebug == 2)
    {
        // Render 2d areas additively on screen
        IRenderAuxGeom* pAuxRenderer = gEnv->pRenderer->GetIRenderAuxGeom();
        if (pAuxRenderer)
        {
            SAuxGeomRenderFlags oldRenderFlags = pAuxRenderer->GetRenderFlags();

            SAuxGeomRenderFlags newRenderFlags;
            newRenderFlags.SetDepthTestFlag(e_DepthTestOff);
            newRenderFlags.SetAlphaBlendMode(e_AlphaAdditive);
            newRenderFlags.SetMode2D3DFlag(e_Mode2D);
            pAuxRenderer->SetRenderFlags(newRenderFlags);

            const float screenWidth = (float)GetRenderer()->GetWidth();
            const float screenHeight = (float)GetRenderer()->GetHeight();

            // Calc resolve area
            const float left = pLight->m_sX / screenWidth;
            const float top = pLight->m_sY / screenHeight;
            const float right = (pLight->m_sX + pLight->m_sWidth) / screenWidth;
            const float bottom = (pLight->m_sY + pLight->m_sHeight) / screenHeight;

            // Render resolve area
            ColorB areaColor(50, 0, 50, 255);

            if (vPMin.z < 0.0f)
            {
                areaColor = ColorB(0, 100, 0, 255);
            }

            const uint vertexCount = 6;
            const Vec3 vert[vertexCount] = {
                Vec3(left, top, 0.0f),
                Vec3(left, bottom, 0.0f),
                Vec3(right, top, 0.0f),
                Vec3(left, bottom, 0.0f),
                Vec3(right, bottom, 0.0f),
                Vec3(right, top, 0.0f)
            };
            pAuxRenderer->DrawTriangles(vert, vertexCount, areaColor);

            // Set previous Aux render flags back again
            pAuxRenderer->SetRenderFlags(oldRenderFlags);
        }
    }
#endif
}

void C3DEngine::RemoveEntityLightSources(IRenderNode* pEntity)
{
    for (int i = 0; i < m_lstStaticLights.Count(); i++)
    {
        if (m_lstStaticLights[i] == pEntity)
        {
            m_lstStaticLights.Delete(i);
            if (pEntity == m_pSun)
            {
                m_pSun = NULL;
            }
            i--;
        }
    }
}

ILightSource* C3DEngine::GetSunEntity()
{
    return m_pSun;
}

void C3DEngine::OnCasterDeleted(IShadowCaster* pCaster)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_3DENGINE);
    { // make sure pointer to object will not be used somewhere in the renderer
        if (m_pSun)
        {
            m_pSun->OnCasterDeleted(pCaster);
        }

        if (GetRenderer()->GetActiveGPUCount() > 1)
        {
            if (ShadowFrustumMGPUCache* pFrustumCache = GetRenderer()->GetShadowFrustumMGPUCache())
            {
                pFrustumCache->DeleteFromCache(pCaster);
            }
        }

        // remove from per object shadows list
        RemovePerObjectShadow(pCaster);
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CLightVolumesMgr::Init()
{
    m_bUpdateLightVolumes = false;
    for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        m_pLightVolumes[i].reserve(LV_MAX_COUNT);
        m_pLightVolsInfo[i].reserve(LV_MAX_COUNT);
    }
    memset(m_nWorldCells, 0, sizeof(m_nWorldCells));
    memset(m_pWorldLightCells, 0, sizeof(m_pWorldLightCells));
}

void CLightVolumesMgr::Reset()
{
    for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        stl::free_container(m_pLightVolumes[i]);
    }

    m_bUpdateLightVolumes = false;
    memset(m_nWorldCells, 0, sizeof(m_nWorldCells));
    memset(m_pWorldLightCells, 0, sizeof(m_pWorldLightCells));
}

//////////////////////////////////////////////////////////////////////////

uint16 CLightVolumesMgr::RegisterVolume(const Vec3& vPos, f32 fRadius, uint8 nClipVolumeRef, const SRenderingPassInfo& passInfo)
{
    DynArray<SLightVolInfo*>& lightVolsInfo = m_pLightVolsInfo[passInfo.ThreadID()];

    IF ((m_bUpdateLightVolumes & (lightVolsInfo.size() < LV_MAX_COUNT)) && fRadius < 256.0f, 1)
    {
        FUNCTION_PROFILER_3DENGINE;

        int32 nPosx = (int32)(floorf(vPos.x * LV_CELL_RSIZEX));
        int32 nPosy = (int32)(floorf(vPos.y * LV_CELL_RSIZEY));
        int32 nPosz = (int32)(floorf(vPos.z * LV_CELL_RSIZEZ));

        // Check if world cell has any light volume, else add new one
        uint16 nHashIndex = GetWorldHashBucketKey(nPosx, nPosy, nPosz);
        uint16* pCurrentVolumeID = &m_nWorldCells[nHashIndex];

        while (*pCurrentVolumeID != 0)
        {
            SLightVolInfo& sVolInfo = *lightVolsInfo[*pCurrentVolumeID - 1];

            int32 nVolumePosx = (int32)(floorf(sVolInfo.vVolume.x * LV_CELL_RSIZEX));
            int32 nVolumePosy = (int32)(floorf(sVolInfo.vVolume.y * LV_CELL_RSIZEY));
            int32 nVolumePosz = (int32)(floorf(sVolInfo.vVolume.z * LV_CELL_RSIZEZ));

            if (nPosx == nVolumePosx &&
                nPosy == nVolumePosy &&
                nPosz == nVolumePosz &&
                nClipVolumeRef  == sVolInfo.nClipVolumeID)
            {
                return (uint16) * pCurrentVolumeID;
            }

            pCurrentVolumeID = &sVolInfo.nNextVolume;
        }

        // create new volume
        SLightVolInfo* pLightVolInfo = new SLightVolInfo(vPos, fRadius, nClipVolumeRef);
        lightVolsInfo.push_back(pLightVolInfo);
        *pCurrentVolumeID = lightVolsInfo.size();

        return *pCurrentVolumeID;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

void CLightVolumesMgr::RegisterLight(const CDLight& pDL, uint32 nLightID, const SRenderingPassInfo& passInfo)
{
    IF ((m_bUpdateLightVolumes & !(pDL.m_Flags & LV_DLF_LIGHTVOLUMES_MASK)), 1)
    {
        FUNCTION_PROFILER_3DENGINE;

        const f32 fColCheck = (f32) fsel(pDL.m_Color.r + pDL.m_Color.g + pDL.m_Color.b - 0.333f, 1.0f, 0.0f);  //light color > threshold
        const f32 fRadCheck = (f32) fsel(pDL.m_fRadius - 0.5f, 1.0f, 0.0f);  //light radius > threshold
        if (fColCheck * fRadCheck)
        {
            //if the radius is large than certain value, all the the world light cells will be lighted anyway. So we just add the light to all the cells
            //the input radius restriction will be added too
            if(floorf(pDL.m_fRadius*LV_LIGHT_CELL_R_SIZE) > LV_LIGHTS_WORLD_BUCKET_SIZE)
            {
                for (int32 idx = 0; idx < LV_LIGHTS_WORLD_BUCKET_SIZE; idx++)
                {
                    SLightCell& lightCell = m_pWorldLightCells[idx];
                    CryPrefetch(&lightCell);
                    if (lightCell.nLightCount < LV_LIGHTS_MAX_COUNT)
                    {
                        lightCell.nLightID[lightCell.nLightCount] = nLightID;
                        lightCell.nLightCount += 1;
                    }
                }
            }
            else
            {
                int32 nMiny = (int32)(floorf((pDL.m_Origin.y - pDL.m_fRadius) * LV_LIGHT_CELL_R_SIZE));
                int32 nMaxy = (int32)(floorf((pDL.m_Origin.y + pDL.m_fRadius) * LV_LIGHT_CELL_R_SIZE));
                int32 nMinx = (int32)(floorf((pDL.m_Origin.x - pDL.m_fRadius) * LV_LIGHT_CELL_R_SIZE));
                int32 nMaxx = (int32)(floorf((pDL.m_Origin.x + pDL.m_fRadius) * LV_LIGHT_CELL_R_SIZE));

                // Register light into all cells touched by light radius
                for (int32 y = nMiny, ymax = nMaxy; y <= ymax; ++y)
                {
                    for (int32 x = nMinx, xmax = nMaxx; x <= xmax; ++x)
                    {
                        SLightCell& lightCell = m_pWorldLightCells[GetWorldHashBucketKey(x, y, 1, LV_LIGHTS_WORLD_BUCKET_SIZE)];
                        CryPrefetch(&lightCell);
                        if (lightCell.nLightCount < LV_LIGHTS_MAX_COUNT)
                        {
                            //only if the las light added to the cell wasn't the same light
                            if (!(lightCell.nLightCount > 0 && lightCell.nLightID[lightCell.nLightCount - 1] == nLightID))
                            {
                                lightCell.nLightID[lightCell.nLightCount] = nLightID;
                                lightCell.nLightCount += 1;
                            }
                        }
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void CLightVolumesMgr::AddLight(const SRenderLight& pLight, const SLightVolInfo* __restrict pVolInfo, SLightVolume& pVolume)
{
    // Check for clip volume
    if (pLight.m_nStencilRef[0] == pVolInfo->nClipVolumeID || pLight.m_nStencilRef[1] == pVolInfo->nClipVolumeID ||
        pLight.m_nStencilRef[0] == CClipVolumeManager::AffectsEverythingStencilRef)
    {
        const Vec4* __restrict vLight = (Vec4*) &pLight.m_Origin.x;
        const Vec4& vVolume = pVolInfo->vVolume;
        const f32 fDimCheck = (f32) fsel(vLight->w - vVolume.w * 0.1f, 1.0f, 0.0f);  //light radius not more than 10x smaller than volume radius
        const f32 fOverlapCheck = (f32) fsel(sqr(vVolume.x - vLight->x) + sqr(vVolume.y - vLight->y) + sqr(vVolume.z - vLight->z) - sqr(vVolume.w + vLight->w), 0.0f, 1.0f);// touches volumes
        if (fDimCheck * fOverlapCheck)
        {
            float fAttenuationBulbSize = pLight.m_fAttenuationBulbSize;
            Vec3 lightColor =  *((Vec3*)&pLight.m_Color);

            // Adjust light intensity so that the intended brightness is reached 1 meter from the light's surface
            IF (!(pLight.m_Flags & (DLF_AREA_LIGHT | DLF_AMBIENT)), 1)
            {
                fAttenuationBulbSize = max(fAttenuationBulbSize, 0.001f);

                // Solve I * 1 / (1 + d/lightsize)^2 = 1
                float intensityMul = 1.0f + 1.0f / fAttenuationBulbSize;
                intensityMul *= intensityMul;
                lightColor *= intensityMul;
            }

            pVolume.pData.push_back();
            SLightVolume::SLightData& lightData = pVolume.pData[pVolume.pData.size() - 1];
            lightData.vPos = *vLight;
            lightData.vColor = Vec4(lightColor, fAttenuationBulbSize);
            lightData.vParams = Vec4(0.f, 0.f, 0.f, 0.f);

            IF (pLight.m_Flags & DLF_PROJECT, 1)
            {
                lightData.vParams = Vec4(pLight.m_ObjMatrix.GetColumn0(), cos_tpl(DEG2RAD(pLight.m_fLightFrustumAngle)));
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void CLightVolumesMgr::Update(const SRenderingPassInfo& passInfo)
{
    uint32 nThreadID = passInfo.ThreadID();
    DynArray<SLightVolInfo*>& lightVolsInfo = m_pLightVolsInfo[nThreadID];

    if (!m_bUpdateLightVolumes || lightVolsInfo.empty())
    {
        return;
    }

    FUNCTION_PROFILER_3DENGINE;
    TArray<SRenderLight>* pLights = GetRenderer()->EF_GetDeferredLights(passInfo);
    const uint32 nLightCount = pLights->size();

    uint32 nLightVols = lightVolsInfo.size();
    LightVolumeVector& lightVols = m_pLightVolumes[nThreadID];
    uint32 existingLightVolsCount = 0;  //This is 0, that just means we will be overwriting all existing light volumes
    
    //If this is a recursive pass (not the first time that this is called this frame), we're just going to be adding on new light volumes to the existing collection
    if (passInfo.IsRecursivePass())
    {
        existingLightVolsCount = lightVols.size();

        //If no new light volumes have been added, don't bother updating
        if (nLightVols == existingLightVolsCount)
        {
            return;
        }
    }

    lightVols.resize(nLightVols);

    if (!nLightCount)
    {
        //Start out existingLightVolsCount to avoid clearing out existing light volumes when we don't need to
        for (uint32 v = existingLightVolsCount; v < nLightVols; ++v)
        {
            lightVols[v].pData.resize(0);
        }

        return;
    }

    const int MAX_NUM_LIGHTS_FOR_LIGHT_VOLUME_UPDATE = 1024;

    if (nLightCount > MAX_NUM_LIGHTS_FOR_LIGHT_VOLUME_UPDATE)
    {
        CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "More lights in the scene (%d) than supported by the Light Volume Update function (%d). Extra lights will be ignored.",
            nLightCount, MAX_NUM_LIGHTS_FOR_LIGHT_VOLUME_UPDATE);
    }

    //This can be a uint8 array because nLightVols should never be greater than 256(LV_MAX_COUNT)
    assert(LV_MAX_COUNT <= 256);
    uint8 lightProcessedStateArray[MAX_NUM_LIGHTS_FOR_LIGHT_VOLUME_UPDATE];

    //Start at the number of light volumes that already exist so that we don't end up re-updating light volumes unnecessarily. 
    for (uint32 v = existingLightVolsCount; v < nLightVols; ++v)
    {
        const Vec4* __restrict vBVol = &lightVolsInfo[v]->vVolume;
        int32 nMiny = (int32)(floorf((vBVol->y - vBVol->w) * LV_LIGHT_CELL_R_SIZE));
        int32 nMaxy = (int32)(floorf((vBVol->y + vBVol->w) * LV_LIGHT_CELL_R_SIZE));
        int32 nMinx = (int32)(floorf((vBVol->x - vBVol->w) * LV_LIGHT_CELL_R_SIZE));
        int32 nMaxx = (int32)(floorf((vBVol->x + vBVol->w) * LV_LIGHT_CELL_R_SIZE));

        lightVols[v].pData.resize(0);

        // Loop through active light cells touching bounding volume (~avg 2 cells)
        for (int32 y = nMiny, ymax = nMaxy; y <= ymax; ++y)
        {
            for (int32 x = nMinx, xmax = nMaxx; x <= xmax; ++x)
            {
                const SLightCell& lightCell = m_pWorldLightCells[GetWorldHashBucketKey(x, y, 1, LV_LIGHTS_WORLD_BUCKET_SIZE)];
                CryPrefetch(&lightCell);

                const SRenderLight& pFirstDL = (*pLights)[lightCell.nLightID[0]];
                CryPrefetch(&pFirstDL);
                CryPrefetch(&pFirstDL.m_ObjMatrix);
                for (uint32 l = 0; (l < lightCell.nLightCount) & (lightVols[v].pData.size() < LIGHTVOLUME_MAXLIGHTS); ++l)
                {
                    const int32 nLightId = lightCell.nLightID[l];

                    //Only allow IDs < MAX_NUM_LIGHTS_FOR_LIGHT_VOLUME_UPDATE to continue or else we'll overflow access to
                    //lightProcessedStateArray[MAX_NUM_LIGHTS_FOR_LIGHT_VOLUME_UPDATE].  Skipping the extra lights shouldn't really matter
                    //since A) folks won't be using that many lights, and B) for the case of light emitting particles, they tend to be grouped
                    //so that the individual contributions tend to bleed together anyway.
                    if (nLightId >= MAX_NUM_LIGHTS_FOR_LIGHT_VOLUME_UPDATE)
                    {
                        continue;
                    }

                    const SRenderLight& pDL = (*pLights)[nLightId];
                    const int32 nNextLightId = lightCell.nLightID[(l + 1) & (LIGHTVOLUME_MAXLIGHTS - 1)];
                    const SRenderLight& pNextDL = (*pLights)[nNextLightId];
                    CryPrefetch(&pNextDL);
                    CryPrefetch(&pNextDL.m_ObjMatrix);

                    IF (lightProcessedStateArray[nLightId] != v + 1, 1)
                    {
                        lightProcessedStateArray[nLightId] = v + 1;
                        AddLight(pDL, &*lightVolsInfo[v], lightVols[v]);
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void CLightVolumesMgr::Clear(const SRenderingPassInfo& passInfo)
{
    DynArray<SLightVolInfo*>& lightVolsInfo = m_pLightVolsInfo[passInfo.ThreadID()];

    m_bUpdateLightVolumes = false;
    if (GetCVars()->e_LightVolumes && passInfo.IsGeneralPass() && GetCVars()->e_DynamicLights)
    {
        memset(m_nWorldCells, 0, sizeof(m_nWorldCells));
        memset(m_pWorldLightCells, 0, sizeof(m_pWorldLightCells));

        //Clean up volume info data
        for (size_t i = 0; i < lightVolsInfo.size(); ++i)
        {
            delete lightVolsInfo[i];
        }

        m_pLightVolsInfo[passInfo.ThreadID()].clear();
        m_bUpdateLightVolumes = (GetCVars()->e_LightVolumes == 1) ? true : false;
    }
}

//////////////////////////////////////////////////////////////////////////

void CLightVolumesMgr::GetLightVolumes(threadID nThreadID, SLightVolume*& pLightVols, uint32& nNumVols)
{
    pLightVols = 0;
    nNumVols = 0;
    if (GetCVars()->e_LightVolumes == 1 && GetCVars()->e_DynamicLights && !m_pLightVolumes[nThreadID].empty())
    {
        pLightVols = &m_pLightVolumes[nThreadID][0];
        nNumVols = m_pLightVolumes[nThreadID].size();
    }
}


void C3DEngine::GetLightVolumes(threadID nThreadID, SLightVolume*& pLightVols, uint32& nNumVols)
{
    m_LightVolumesMgr.GetLightVolumes(nThreadID, pLightVols, nNumVols);
}

//////////////////////////////////////////////////////////////////////////
#ifndef _RELEASE
void CLightVolumesMgr::DrawDebug(const SRenderingPassInfo& passInfo)
{
    DynArray<SLightVolInfo*>& lightVolsInfo = m_pLightVolsInfo[passInfo.ThreadID()];

    IRenderer* pRenderer = GetRenderer();
    IRenderAuxGeom* pAuxGeom = GetRenderer()->GetIRenderAuxGeom();
    if (!pAuxGeom || !passInfo.IsGeneralPass())
    {
        return;
    }

    ColorF cWhite = ColorF(1, 1, 1, 1);
    ColorF cBad = ColorF(1.0f, 0.0, 0.0f, 1.0f);
    ColorF cWarning = ColorF(1.0f, 1.0, 0.0f, 1.0f);
    ColorF cGood = ColorF(0.0f, 0.5, 1.0f, 1.0f);
    ColorF cSingleCell = ColorF(0.0f, 1.0, 0.0f, 1.0f);

    const uint32 nLightVols = lightVolsInfo.size();
    LightVolumeVector& lightVols = m_pLightVolumes[passInfo.ThreadID()];
    const Vec3 vCamPos = passInfo.GetCamera().GetPosition();

    float fYLine = 8.0f, fYStep = 20.0f;
    GetRenderer()->Draw2dLabel(8.0f, fYLine += fYStep, 2.0f, (float*)&cWhite.r, false, "Light Volumes Info (count %d)", nLightVols);

    for (uint32 v = 0; v < nLightVols; ++v)  // draw each light volume
    {
        SLightVolume& lv = lightVols[v];
        SLightVolInfo& lvInfo = *lightVolsInfo[v];

        ColorF& cCol = (lv.pData.size() >= 10) ? cBad : ((lv.pData.size() >= 5) ? cWarning : cGood);
        const Vec3 vPos = Vec3(lvInfo.vVolume.x, lvInfo.vVolume.y, lvInfo.vVolume.z);
        const float fCamDistSq = (vPos - vCamPos).len2();
        cCol.a = max(0.25f, min(1.0f, 1024.0f / (fCamDistSq + 1e-6f)));

        pRenderer->DrawLabelEx(vPos, 1.3f, (float*)&cCol.r, true, true, "Id: %d\nPos: %.2f %.2f %.2f\nRadius: %.2f\nLights: %d\nOutLights: %d",
            v, vPos.x, vPos.y, vPos.z, lvInfo.vVolume.w, lv.pData.size(), (*(int32*)&lvInfo.vVolume.w) & (1 << 31) ? 1 : 0);

        if (GetCVars()->e_LightVolumesDebug == 2)
        {
            const float fSideSize = 0.707f * sqrtf(lvInfo.vVolume.w * lvInfo.vVolume.w * 2);
            pAuxGeom->DrawAABB(AABB(vPos - Vec3(fSideSize), vPos + Vec3(fSideSize)), false, cCol, eBBD_Faceted);
        }

        if (GetCVars()->e_LightVolumesDebug == 3)
        {
            cBad.a = 1.0f;
            const Vec3 vCellPos = Vec3(floorf((lvInfo.vVolume.x) * LV_CELL_RSIZEX) * LV_CELL_SIZEX,
                    floorf((lvInfo.vVolume.y) * LV_CELL_RSIZEY) * LV_CELL_SIZEY,
                    floorf((lvInfo.vVolume.z) * LV_CELL_RSIZEZ) * LV_CELL_SIZEZ);

            const Vec3 vMin = vCellPos;
            const Vec3 vMax = vMin + Vec3(LV_CELL_SIZEX, LV_CELL_SIZEY, LV_CELL_SIZEZ);
            pAuxGeom->DrawAABB(AABB(vMin, vMax), false, cBad, eBBD_Faceted);
        }
    }
}
#endif
