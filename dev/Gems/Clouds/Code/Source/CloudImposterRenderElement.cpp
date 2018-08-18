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

#include "RendElement.h"
#include "CloudImposterRenderElement.h"
#include "I3DEngine.h"

namespace CloudsGem
{
    int CloudImposterRenderElement::s_MemUpdated = 0;
    int CloudImposterRenderElement::s_MemPostponed = 0;
    int CloudImposterRenderElement::s_PrevMemUpdated = 0;
    int CloudImposterRenderElement::s_PrevMemPostponed = 0;

    IDynTexture* CloudImposterRenderElement::s_pScreenTexture = nullptr;

    // Helper method to Enumerates and index edges of a box to find minimal enclosing rectangle
    static void GetEdgeNo(const uint32 edgeNumber, uint32& vertexA, uint32& vertexB)
    {
        switch (edgeNumber)
        {
        case  0: vertexA = 0; vertexB = 1; break;
        case  1: vertexA = 2; vertexB = 3; break;
        case  2: vertexA = 4; vertexB = 5; break;
        case  3: vertexA = 6; vertexB = 7; break;
        case  4: vertexA = 0; vertexB = 2; break;
        case  5: vertexA = 4; vertexB = 6; break;
        case  6: vertexA = 5; vertexB = 7; break;
        case  7: vertexA = 1; vertexB = 3; break;
        case  8: vertexA = 0; vertexB = 4; break;
        case  9: vertexA = 2; vertexB = 6; break;
        case 10: vertexA = 3; vertexB = 7; break;
        case 11: vertexA = 1; vertexB = 5; break;
        default:
            assert(0);
        }
    }

    CloudImposterRenderElement::CloudImposterRenderElement()
    {
        m_gemRE = gEnv->pRenderer->EF_CreateRE(eDATA_Gem);
        m_gemRE->mfSetFlags(FCEF_TRANSFORM);
        m_gemRE->mfSetDelegate(this);
    }

    CloudImposterRenderElement::~CloudImposterRenderElement()
    {
        ReleaseResources();
    }

    bool CloudImposterRenderElement::IsImposterValid(
        const CameraViewParameters& cam,
        float fRadiusX, float fRadiusY, float fCamRadiusX, float fCamRadiusY,
        const int iRequiredLogResX, const int iRequiredLogResY, const uint32 dwBestEdge)
    {
#if !defined(DEDICATED_SERVER)
        if (dwBestEdge != m_nLastBestEdge)
        {
            return false;
        }

        // Update transparency
        IRenderer* renderer = gEnv->pRenderer;
        SRenderPipeline* renderPipeline = renderer->GetRenderPipeline();
        float fTransparency = renderPipeline->m_pCurObject->m_II.m_AmbColor.a;
        IRenderShaderResources* shaderResources = renderPipeline->m_pShaderResources;

        if (shaderResources)
        {
            fTransparency *= shaderResources->GetStrengthValue(EFTT_OPACITY);
        }
        if (m_fCurTransparency != fTransparency)
        {
            m_fCurTransparency = fTransparency;
            return false;
        }

        // screen impostors should always be updated
        if (m_bScreenImposter)
        {
            m_vFarPoint.Set(0.0f, 0.0f, 0.0f);
            m_vNearPoint.Set(0.0f, 0.0f, 0.0f);
            return false;
        }

        // Split indicated non valid imposter
        if (m_bSplit)
        {
            return false;
        }

        // Get distance to camera
        Vec3 vEye = m_vPos - cam.vOrigin;
        float fDistance = vEye.GetLength();
        if (fDistance < std::numeric_limits<float>::epsilon())
        {
            return false;
        }
        vEye /= fDistance;

        // Old camera distance
        Vec3 vOldEye = m_vFarPoint - m_LastViewParameters.vOrigin;
        float fOldEyeDist = vOldEye.GetLength();
        if (fOldEyeDist < std::numeric_limits<float>::epsilon())
        {
            return false;                   // to avoid float exceptions
        }
        vOldEye /= fOldEyeDist;

        // Change in camera angle
        float fCosAlpha = vEye * vOldEye; // dot product of normalized vectors = cosine
        if (fCosAlpha < m_fErrorToleranceCosAngle)
        {
            return false;
        }

        // Change in sun direction
        Vec3 curSunDir(gEnv->p3DEngine->GetSunDir().GetNormalized());
        if (m_vLastSunDir.Dot(curSunDir) < 0.995)
        {
            return false;
        }

        // equal pow-of-2 size comparison for consistent look
        if (iRequiredLogResX != m_nLogResolutionX || iRequiredLogResY != m_nLogResolutionY)
        {
            return false;
        }

        if (renderer->GetFrameReset() != m_nFrameReset)
        {
            return false;
        }
#endif
        return true;
    }

    void CloudImposterRenderElement::ReleaseResources()
    {
        SAFE_DELETE(m_pTexture);
        SAFE_DELETE(s_pScreenTexture);
        SAFE_DELETE(m_pFrontTexture);
        SAFE_DELETE(m_pTextureDepth);
    }

    int IntersectRayAABB(Vec3 p, Vec3 d, SMinMaxBox a, Vec3& q)
    {
        float tmin = 0;
        float tmax = FLT_MAX;
        int i;
        const Vec3& min = a.GetMin();
        const Vec3& max = a.GetMax();
        for (i = 0; i < 3; i++)
        {
            if (fabs(d[i]) < 0.001f)
            {
                if (p[i] < min[i] || p[i] > max[i])
                {
                    return 0;
                }
            }
            else
            {
                float ood = 1.0f / d[i];
                float t1 = (min[i] - p[i]) * ood;
                float t2 = (max[i] - p[i]) * ood;
                if (t1 > t2) { Exchange(t1, t2); }
                if (t1 > tmin) { tmin = t1; }
                if (t2 > tmax) { tmax = t2; }
            }
        }
        q = p + d * tmin;

        return 1;
    }

    bool CloudImposterRenderElement::PrepareForUpdate()
    {
#if !defined(DEDICATED_SERVER)
        IRenderer* renderer = gEnv->pRenderer;
        if (renderer->GetRecursionLevel() > 0)
        {
            return false;
        }

        // Compute eye direction, distance from camera to cloud position, normalized distance
        CameraViewParameters cam = renderer->GetViewParameters();
        Vec3 vCenter = GetPosition();
        Vec3 vEye = vCenter - cam.vOrigin;
        float fDistance = vEye.GetLength();
        vEye /= fDistance;

        // Get viewport dimensions
        int32 D3DVP[4];
        renderer->GetViewport(&D3DVP[0], &D3DVP[1], &D3DVP[2], &D3DVP[3]);

        // Unproject world space bounding volume
        Vec3 vUnProjPos[9];
        int i = 0;
        for (i = 0; i < 8; i++)
        {
            vUnProjPos[i].x = (i & 1) ? m_WorldSpaceBV.GetMax().x : m_WorldSpaceBV.GetMin().x; 
            vUnProjPos[i].y = (i & 2) ? m_WorldSpaceBV.GetMax().y : m_WorldSpaceBV.GetMin().y; 
            vUnProjPos[i].z = (i & 4) ? m_WorldSpaceBV.GetMax().z : m_WorldSpaceBV.GetMin().z; 
        }
        vUnProjPos[8] = vCenter;
        
        CameraViewParameters tempCam;
        tempCam.fNear = cam.fNear;
        tempCam.fFar = cam.fFar;

        Matrix44A viewMat, projMat;
        mathMatrixPerspectiveOffCenter(&projMat, -1, 1, 1, -1, tempCam.fNear, tempCam.fFar);

        float fOldEdgeArea = -FLT_MAX;

        Vec3 vProjPos[9];
        uint32 dwBestEdge = 0xffffffff;     // favor the last edge we found
        float fBestArea = FLT_MAX;
        float fMinX, fMaxX, fMinY, fMaxY;

        // try to find minimal enclosing rectangle assuming the best projection frustum must be aligned to a AABB edge
        for (uint32 dwEdge = 0; dwEdge < 13; ++dwEdge)     // 12 edges and iteration no 13 processes the best again
        {
            uint32 dwEdgeA, dwEdgeB;

            if (dwEdge == 12 && fBestArea > fOldEdgeArea * 0.98f)      // not a lot better than old axis then keep old axis (to avoid jittering)
            {
                dwBestEdge = m_nLastBestEdge;
            }

            if (dwEdge == 12)
            {
                GetEdgeNo(dwBestEdge, dwEdgeA, dwEdgeB);      // the best again
            }
            else
            {
                GetEdgeNo(dwEdge, dwEdgeA, dwEdgeB);              // edge no dwEdge
            }

            // Convert edge indices to positions from the bounding volume and compute basis
            Vec3 vEdge[2] = { vUnProjPos[dwEdgeA], vUnProjPos[dwEdgeB] };
            Vec3 vRight = vEdge[0] - vEdge[1];
            Vec3 vUp = (vEdge[0] - cam.vOrigin) ^ vRight;

            // Compute model view matrix 
            tempCam.LookAt(cam.vOrigin, vCenter, vUp);
            tempCam.GetModelviewMatrix(viewMat.GetData());

            // Project the unprojected points into the new space
            Matrix44A identityMatrix = renderer->GetIdentityMatrix();
            mathVec3ProjectArray((Vec3*)&vProjPos[0].x, sizeof(Vec3), (Vec3*)&vUnProjPos[0].x, sizeof(Vec3), D3DVP, &projMat, &viewMat, &identityMatrix, 9, 0);

            // Calculate 2D extents of projected points
            fMinX = fMinY = FLT_MAX;
            fMaxX = fMaxY = -FLT_MAX;
            for (i = 0; i < 8; i++)
            {
                if (fMinX > vProjPos[i].x) { fMinX = vProjPos[i].x; }
                if (fMaxX < vProjPos[i].x) { fMaxX = vProjPos[i].x; }
                if (fMinY > vProjPos[i].y) { fMinY = vProjPos[i].y; }
                if (fMaxY < vProjPos[i].y) { fMaxY = vProjPos[i].y; }
            }

            // Compute area from extents
            float fArea = (fMaxX - fMinX) * (fMaxY - fMinY);

            if (dwEdge == m_nLastBestEdge)
            {
                fOldEdgeArea = fArea;
            }

            // Update the best edge and best area if smaller than best area
            if (fArea < fBestArea)
            {
                dwBestEdge = dwEdge;
                fBestArea = fArea;
            }
        }

        // high precision - no jitter
        float fCamZ = (tempCam.vOrigin - vCenter).Dot(tempCam.ViewDir());
        float f = -fCamZ / tempCam.fNear;
        vUnProjPos[0] = tempCam.CamToWorld(Vec3((fMinX / D3DVP[2] * 2.0f - 1.0f) * f, (fMinY / D3DVP[3] * 2.0f - 1.0f) * f, fCamZ));
        vUnProjPos[1] = tempCam.CamToWorld(Vec3((fMaxX / D3DVP[2] * 2.0f - 1.0f) * f, (fMinY / D3DVP[3] * 2.0f - 1.0f) * f, fCamZ));
        vUnProjPos[2] = tempCam.CamToWorld(Vec3((fMaxX / D3DVP[2] * 2.0f - 1.0f) * f, (fMaxY / D3DVP[3] * 2.0f - 1.0f) * f, fCamZ));
        vUnProjPos[3] = tempCam.CamToWorld(Vec3((fMinX / D3DVP[2] * 2.0f - 1.0f) * f, (fMaxY / D3DVP[3] * 2.0f - 1.0f) * f, fCamZ));

        m_vPos = vCenter;
        Vec3 vProjCenter = (vUnProjPos[0] + vUnProjPos[1] + vUnProjPos[2] + vUnProjPos[3]) / 4.0f;
        Vec3 vDif = vProjCenter - vCenter;
        float fDerivX = vDif * tempCam.vX;
        float fDerivY = vDif * tempCam.vY;

        float fRadius = m_WorldSpaceBV.GetRadius();
        Vec3 vRight = vUnProjPos[0] - vUnProjPos[1];
        Vec3 vUp = vUnProjPos[0] - vUnProjPos[2];
        float fRadiusX = vRight.len() / 2.0f + fabsf(fDerivX);
        float fRadiusY = vUp.len() / 2.0f + fabsf(fDerivY);

        Vec3 vNearest;
        int nCollide = IntersectRayAABB(cam.vOrigin, vEye, m_WorldSpaceBV, vNearest);
        Vec4 v4Nearest = Vec4(vNearest, 1);
        Vec4 v4Far = Vec4(vNearest + vEye * fRadius * 2.0f, 1.0f);
        Vec4 v4ZRange = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        Vec4 v4Column2 = renderer->GetViewProjectionMatrix().GetColumn4(2);
        Vec4 v4Column3 = renderer->GetViewProjectionMatrix().GetColumn4(3);

        bool bScreen = false;

        float fZ = v4Nearest.Dot(v4Column2);
        float fW = v4Nearest.Dot(v4Column3);

        float fNewNear = m_fNear;
        float fNewFar = m_fFar;
        if (fabs(fW) < 0.001f)              // to avoid division by 0 (near the object Screen is used and the value doesn't matter anyway)
        {
            fNewNear = 0.0f;
            bScreen = true;
        }
        else
        {
            fNewNear = 0.999f * fZ / fW;
        }

        fZ = v4Far.Dot(v4Column2);
        fW = v4Far.Dot(v4Column3);

        if (fabs(fW) < 0.001f)              // to avoid division by 0 (near the object Screen is used and the value doesn't matter anyway)
        {
            fNewFar = 1.0f;
            bScreen = true;
        }
        else
        {
            fNewFar = fZ / fW;
        }

        float fCamRadiusX = sqrtf(cam.fWR * cam.fWR + cam.fNear * cam.fNear);
        float fCamRadiusY = sqrtf(cam.fWT * cam.fWT + cam.fNear * cam.fNear);

        float fWidth = cam.fWR - cam.fWL;
        float fHeight = cam.fWT - cam.fWB;

        if (!bScreen)
        {
            bScreen = (fRadiusX * cam.fNear / fDistance >= fWidth || fRadiusY * cam.fNear / fDistance >= fHeight || (fDistance - fRadiusX <= fCamRadiusX) || (fDistance - fRadiusY <= fCamRadiusY));
        }

        IDynTexture* pDT = bScreen ? s_pScreenTexture : m_pTexture;
        SDynTexture2* pDT2 = (SDynTexture2*)pDT;


        float fRequiredResX = 1024.0f;
        float fRequiredResY = 512.0f;

        float imposterRatio = renderer->GetFloatConfigurationValue("r_ImposterRatio", 1.0f);
        float fTexScale = imposterRatio > 0.1f ? 1.0f / imposterRatio : 1.0f / 0.1f;

        if (!bScreen)  // outside cloud
        {
            assert(D3DVP[0] == 0 && D3DVP[1] == 0);         // otherwise the following lines don't make sense

            float fRadPixelX = (fMaxX - fMinX) * 2.0f;     // for some reason *2 is needed, most likely /near (*4) is the correct
            float fRadPixelY = (fMaxY - fMinY) * 2.0f;

            fRequiredResX = min(fRequiredResX, max(16.0f, fRadPixelX));
            fRequiredResY = min(fRequiredResY, max(16.0f, fRadPixelY));
        }

        int nRequiredLogXRes = LogBaseTwo((int)(fRequiredResX * fTexScale));
        int nRequiredLogYRes = LogBaseTwo((int)(fRequiredResY * fTexScale));

        bool isAlwaysUpdated = renderer->GetBooleanConfigurationValue("r_CloudsUpdateAlways", true);
        if (IsImposterValid(cam, fRadiusX, fRadiusY, fCamRadiusX, fCamRadiusY, nRequiredLogXRes, nRequiredLogYRes, dwBestEdge))
        {
            if (!pDT2 || !pDT2->_IsValid()) { return true; }
            if (!isAlwaysUpdated) { return false; }
        }
        if (pDT2)
        {
            pDT2->ResetUpdateMask();
        }

        bool bPostpone = false;
        int nCurFrame = renderer->GetFrameID(false);
        if (renderer->GetActiveGPUCount() == 1)
        {
            if (!isAlwaysUpdated && !bScreen && !m_bScreenImposter && pDT && pDT->GetTexture() && m_fRadiusX && m_fRadiusY)
            {
                int updatesPerFrame = renderer->GetIntegerConfigurationValue("r_ImpostersUpdatePerFrame", 6000);
                if (s_MemUpdated > updatesPerFrame)
                {
                    bPostpone = true;
                }
                if (s_PrevMemPostponed)
                {
                    int nDeltaFrames = s_PrevMemPostponed / updatesPerFrame;
                    if (nCurFrame - m_FrameUpdate > nDeltaFrames)
                    {
                        bPostpone = false;
                    }
                }
                if (bPostpone)
                {
                    s_MemPostponed += (1 << nRequiredLogXRes) * (1 << nRequiredLogYRes) * 4 / 1024;
                    return false;
                }
            }
        }
        m_FrameUpdate = nCurFrame;
        m_fNear = fNewNear;
        m_fFar = fNewFar;

        m_LastViewParameters = cam;
        m_vLastSunDir = gEnv->p3DEngine->GetSunDir().GetNormalized();

        m_nLogResolutionX = nRequiredLogXRes;
        m_nLogResolutionY = nRequiredLogYRes;

        if (!bScreen)
        { 
            m_LastViewParameters = tempCam;
            
            // otherwise the following lines don't make sense
            assert(D3DVP[0] == 0 && D3DVP[1] == 0);

            m_LastViewParameters.fWL = (fMinX / D3DVP[2] * 2 - 1);
            m_LastViewParameters.fWR = (fMaxX / D3DVP[2] * 2 - 1);
            m_LastViewParameters.fWT = (fMaxY / D3DVP[3] * 2 - 1);
            m_LastViewParameters.fWB = (fMinY / D3DVP[3] * 2 - 1);

            m_fRadiusX = 0.5f * (m_LastViewParameters.fWR - m_LastViewParameters.fWL) * fDistance / m_LastViewParameters.fNear;
            m_fRadiusY = 0.5f * (m_LastViewParameters.fWT - m_LastViewParameters.fWB) * fDistance / m_LastViewParameters.fNear;

            m_vQuadCorners[0] = vUnProjPos[0] - m_vPos;
            m_vQuadCorners[1] = vUnProjPos[1] - m_vPos;
            m_vQuadCorners[2] = vUnProjPos[2] - m_vPos;
            m_vQuadCorners[3] = vUnProjPos[3] - m_vPos;
            m_nLastBestEdge = dwBestEdge;

            m_bScreenImposter = false;
            // store points used in later error estimation
            m_vNearPoint = -m_LastViewParameters.vZ * m_LastViewParameters.fNear + m_LastViewParameters.vOrigin;
            m_vFarPoint = -m_LastViewParameters.vZ * m_LastViewParameters.fFar + m_LastViewParameters.vOrigin;
        }
        else
        { 
            m_bScreenImposter = true;
        }
#endif
        return true;
    }

    bool CloudImposterRenderElement::UpdateImposter()
    {
#if !defined(DEDICATED_SERVER)
        IRenderer* renderer = gEnv->pRenderer;
        SRenderPipeline* renderPipeline = renderer->GetRenderPipeline();

        if (!PrepareForUpdate())
        {
            return true;
        }

        // Save viewport for later restore
        int oldViewport[4];
        renderer->GetViewport(&oldViewport[0], &oldViewport[1], &oldViewport[2], &oldViewport[3]);

        // Find actual resolution
        int iResX = 1 << m_nLogResolutionX;
        int iResY = 1 << m_nLogResolutionY;

        renderer->FX_SetState(GS_DEPTHWRITE);

        IDynTexture** pDT;
        if (!m_bSplit)
        {
            pDT = !m_bScreenImposter ? &m_pTexture : &s_pScreenTexture;
            if (!*pDT)
            {
                *pDT = renderer->CreateDynTexture2(iResX, iResY, FT_STATE_CLAMP, "Imposter", eTP_Clouds);
            }

            if (*pDT)
            {
                (*pDT)->Update(iResX, iResY);
                CTexture* pT = (CTexture*)(*pDT)->GetTexture();
                int nSize = pT->GetDataSize();
                s_MemUpdated += nSize / 1024;
                renderPipeline->m_PS[renderPipeline->m_nProcessThreadID].m_ImpostersSizeUpdate += nSize;

                SDepthTexture* pDepth = renderer->FX_GetDepthSurface(iResX, iResY, false);
                (*pDT)->ClearRT();
                (*pDT)->SetRT(0, true, pDepth);

                renderer->FX_ClearTarget(pDepth);
                float fYFov, fXFov, fAspect, fNearest, fFar;
                m_LastViewParameters.GetPerspectiveParams(&fYFov, &fXFov, &fAspect, &fNearest, &fFar);
                CCamera EngCam;
                CCamera OldCam = renderer->GetCamera();
                int nW = iResX;
                int nH = iResY;

                fYFov = DEG2RAD(fYFov);
                fXFov = DEG2RAD(fXFov);
                if (m_bScreenImposter)
                {
                    nW = renderer->GetWidth();
                    nH = renderer->GetHeight();
                    fXFov = EngCam.GetFov();
                }
                Matrix34 matr;
                matr = Matrix34::CreateFromVectors(m_LastViewParameters.vX, -m_LastViewParameters.vZ, m_LastViewParameters.vY, m_LastViewParameters.vOrigin);
                EngCam.SetMatrix(matr);
                EngCam.SetFrustum(nW, nH, fXFov, fNearest, fFar);

                Matrix44A transposed = renderer->GetViewProjectionMatrix().GetTransposed();
                renderer->SetTranspOrigCameraProjMatrix(transposed);
                renderer->SetViewParameters(m_LastViewParameters);

                int nFL = renderPipeline->m_PersFlags2;
                renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_PersFlags |= RBPF_IMPOSTERGEN;
                renderPipeline->m_PersFlags2 |= RBPF2_NOALPHABLEND | RBPF2_NOALPHATEST;
                renderPipeline->m_StateAnd &= ~(GS_BLEND_MASK | GS_ALPHATEST_MASK);

                assert(!"GetI3DEngine()->RenderImposterContent() does not exist");
                renderPipeline->m_PersFlags2 = nFL;

                (*pDT)->RestoreRT(0, true);

                renderer->SetCamera(OldCam);
            }
        }
        renderer->RT_SetViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
#endif
        return true;
    }

    bool CloudImposterRenderElement::Display(bool /* bDisplayFrontOfSplit */)
    {
        return true;
    }
}