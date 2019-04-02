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

#include "CloudRenderElement.h"
#include "CloudImposterRenderElement.h"

#include <AzCore/std/sort.h>

namespace CloudsGem
{
    static const uint32 s_iShadeResolution = 32;
    static const float s_fAlbedo = 0.9f;
    static const float s_fExtinction = 80.0f;
    static const float s_fTransparency = exp(-s_fExtinction);
    static const float s_fScatterFactor = s_fAlbedo * s_fExtinction * (1.0f / (4.0f * gf_PI));
    static const float s_fSortAngleErrorTolerance = 0.8f;
    static const float s_fSortSquareDistanceTolerance = 100.0f;
    static const float s_fEpsilon = 1e-4f;

    static void GenerateVertex(SVF_P3F_C4B_T2F& vertex, const Vec3& v, const Vec3& sunDir, const Vec3& origin, const Vec3& negCam, const Vec3& pos, const Vec2& uv, const Vec3 color, float alpha)
    {
        // Compute view direction used to compute sun contribution
        Vec3 position(pos + v);
        Vec3 viewDir(origin - position);
        viewDir = viewDir.GetLengthSquared() < s_fEpsilon ? negCam : viewDir.GetNormalized();

        // Compute sun color contribution
        float sunColorModulation = sunDir.Dot(v.GetNormalized()) * 0.5f + 0.5f;
        sunColorModulation *= sunDir.Dot(viewDir) * 0.25f + 0.75f;
        sunColorModulation = clamp_tpl(sunColorModulation, 0.0f, 1.0f);

        // Compute final vertex color
        ColorF vertexColor(color, alpha);
        vertexColor.ScaleCol(sunColorModulation);
        vertexColor.Clamp();

        // Construct vertex
        vertex.xyz = position;
        vertex.color.dcolor = vertexColor.pack_argb8888();
        vertex.st = uv;
    }

    CloudRenderElement::CloudRenderElement()
    {
        m_gemRE = gEnv->pRenderer->EF_CreateRE(eDATA_Gem);
        m_gemRE->mfSetFlags(FCEF_TRANSFORM);
        m_gemRE->mfSetDelegate(this);
    }

    void CloudRenderElement::DrawBillboards(const CameraViewParameters& camera)
    {
#if !defined(DEDICATED_SERVER)
        int nParts = m_particles.size();
        if (nParts == 0)
        {
            return;
        }

        IRenderer* renderer = gEnv->pRenderer;
        SRenderPipeline* renderPipeline = renderer->GetRenderPipeline();
        SRenderThread* thread = renderer->GetRenderThread();
        assert(thread->IsRenderThread());

        CameraViewParameters cam(camera);
        
        // Setup basis for screen facing billboards
        Vec3 vUp(0, 0, 1);
        Vec3 vParticlePlane = cam.vX % cam.vY;
        Vec3 vParticleX = (vUp % vParticlePlane).GetNormalized();
        Vec3 vParticleY = (vParticleX % vParticlePlane).GetNormalized();

        // Sort particles if the direction or position of camera has moved significantly
        float fCosAngleSinceLastSort = m_vLastSortViewDir * renderer->GetViewParameters().ViewDir();
        float fSquareDistanceSinceLastSort = (renderer->GetViewParameters().vOrigin - m_vLastSortCamPos).GetLengthSquared();
        if (fCosAngleSinceLastSort < s_fSortAngleErrorTolerance || fSquareDistanceSinceLastSort > s_fSortSquareDistanceTolerance)
        {
            Vec3 vSortPos = -cam.ViewDir();
            vSortPos *= (1.1f * m_boundingBox.GetRadius());
            SortParticles(cam.ViewDir(), vSortPos, ESortDirection::eSort_TOWARD);

            m_vLastSortViewDir = renderer->GetViewParameters().ViewDir();
            m_vLastSortCamPos = renderer->GetViewParameters().vOrigin;
        }

        CRenderObject* renderObject = renderPipeline->m_pCurObject;
        SRenderObjData* objectData = renderObject->GetObjData();
        IShader* shader = renderPipeline->m_pShader;
        SShaderTechnique* shaderTechnique = renderPipeline->m_pCurTechnique;
        SShaderPass* pPass = renderPipeline->m_pCurPass;
        CShaderResources* shaderResources = renderPipeline->m_pShaderResources;
        IRenderElementDelegate* renderElement = renderObject->GetRE()->mfGetDelegate();
        CloudImposterRenderElement* imposter = static_cast<CloudImposterRenderElement*>(renderElement);
        Vec3 vPos = imposter->GetPosition();

        uint32 nPasses;
        if (renderer->GetRecursionLevel() > 0)
        {
            static CCryNameTSCRC techName("Cloud_Recursive");
            shader->FXSetTechnique(techName);
        }
        else
        {
            static CCryNameTSCRC techName("Cloud");
            shader->FXSetTechnique(techName);
        }

        // Begin shading without modifying existing state. State was setup outside this method
        shader->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
        shader->FXBeginPass(0);

        // Get particle diffuse texture
        if (shaderResources)
        {
            SEfResTexture*  pTextureRes = shaderResources->GetTextureResource(EFTT_DIFFUSE);
            if (pTextureRes)
            {
                m_pTexParticle = pTextureRes->m_Sampler.m_pTex;
            }
        }
        if (m_pTexParticle)
        {
            m_pTexParticle->ApplyTexture(0);
        }
        else
        {
            AZ_Warning("ShadersSystem", false, "Error: missing diffuse texture for clouds in CloudRenderElement::DrawBillboards");
        }

        // Setup alpha blending
        renderer->FX_SetState(GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST | GS_ALPHATEST_GREATER, 0);
        renderer->SetCull(eCULL_None);

        CShaderMan* shaderManager = renderer->GetShaderManager();
        if (renderer->GetRecursionLevel() > 0)
        {
            shaderManager->m_RTRect = Vec4(0, 0, 1, 1);
        }

        // Reflection pass
        if (renderer->GetRecursionLevel() > 0)
        {
            Vec4 vCloudColorScale(m_fCloudColorScale, 0, 0, 0);
            shader->FXSetPSFloat("g_CloudColorScale", &vCloudColorScale, 1);
        }

        renderer->FX_Commit();

        if (!FAILED(renderer->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
        {
            // Extract rotation matrix from world matrix. This will be used 
            // to rotate the particle in world space
            Matrix33 rotation;
            renderObject->m_II.m_Matrix.GetRotation33(rotation);

            // Draw each billboard
            int nStartPart = 0;
            int nCurParts = 0;
            while (nStartPart < nParts)
            {
                nCurParts = min(32768, nParts - nStartPart);

                TempDynVB<SVF_P3F_C4B_T2F> vb(renderer);
                vb.Allocate(nCurParts * 4);
                SVF_P3F_C4B_T2F* pDst = vb.Lock();

                TempDynIB16 ib(renderer);
                ib.Allocate(nCurParts * 6);
                uint16* pDstInds = ib.Lock();

                // get various run-time parameters to determine cloud shading
                Vec3 sunDir(gEnv->p3DEngine->GetSunDir().GetNormalized());

                float minHeight(m_boundingBox.GetMin().z);
                float totalHeight(m_boundingBox.GetMax().z - minHeight);

                ColorF cloudSpec, cloudDiff;
                GetIllumParams(cloudSpec, cloudDiff);

                I3DEngine* p3DEngine(gEnv->p3DEngine);
                assert(0 != p3DEngine);

                Vec3 cloudShadingMultipliers;
                p3DEngine->GetGlobalParameter(E3DPARAM_CLOUDSHADING_MULTIPLIERS, cloudShadingMultipliers);

                Vec3 negCamFrontDir(-cam.ViewDir());

                m_fCloudColorScale = 1.0f;

                // Compute normalized m_fCloudColorScale for HDR rendering
                Vec3 brightColor(cloudShadingMultipliers.x* p3DEngine->GetSunColor().CompMul(Vec3(cloudSpec.r, cloudSpec.g, cloudSpec.b)));
                if (brightColor.x > m_fCloudColorScale)
                {
                    m_fCloudColorScale = brightColor.x;
                }
                if (brightColor.y > m_fCloudColorScale)
                {
                    m_fCloudColorScale = brightColor.y;
                }
                if (brightColor.z > m_fCloudColorScale)
                {
                    m_fCloudColorScale = brightColor.z;
                }
                brightColor /= m_fCloudColorScale;

                // Draw each particle as a textured billboard.
                for (int i = 0; i < nCurParts; i++)
                {
                    const AZStd::shared_ptr<CloudParticle>& p = m_particles[i + nStartPart];
                    const uint32 nInd = i * 4;
                    SVF_P3F_C4B_T2F* pQuad = &pDst[nInd];

                    Vec3 pos = rotation * p->GetPosition() * m_fScale + vPos;
                    Vec3 x = vParticleX * p->GetRadius() * m_fScale;
                    Vec3 y = vParticleY * p->GetRadius() * m_fScale;
                    
                    float alpha = renderObject->m_fAlpha;
                    GenerateVertex(pQuad[0], Vec3{ -y - x }, sunDir, cam.vOrigin, negCamFrontDir, pos, { p->GetUV(0).x, p->GetUV(0).y }, brightColor, alpha);
                    GenerateVertex(pQuad[1], Vec3{ -y + x }, sunDir, cam.vOrigin, negCamFrontDir, pos, { p->GetUV(1).x, p->GetUV(0).y }, brightColor, alpha);
                    GenerateVertex(pQuad[2], Vec3{  y + x }, sunDir, cam.vOrigin, negCamFrontDir, pos, { p->GetUV(1).x, p->GetUV(1).y }, brightColor, alpha);
                    GenerateVertex(pQuad[3], Vec3{  y - x }, sunDir, cam.vOrigin, negCamFrontDir, pos, { p->GetUV(0).x, p->GetUV(1).y }, brightColor, alpha);

                    uint16* pInds = &pDstInds[i * 6];
                    pInds[0] = nInd + 0;
                    pInds[1] = nInd + 1;
                    pInds[2] = nInd + 2;                                           // Triangle 0
                    pInds[3] = nInd + 0;
                    pInds[4] = nInd + 2;
                    pInds[5] = nInd + 3;                                           // Triangle 1
                }

                vb.Unlock();
                vb.Bind(0);
                vb.Release();

                ib.Unlock();
                ib.Bind();
                ib.Release();

                renderer->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, nCurParts * 4, 0, nCurParts * 6);

                nStartPart += nCurParts;
            }
        }

        renderPipeline->m_pCurObject = renderObject;
        renderPipeline->m_pShader = (CShader*)shader;
        renderPipeline->m_pCurTechnique = shaderTechnique;
        renderPipeline->m_pCurPass = pPass;
#endif
    }

    void CloudRenderElement::SetParticles(const AZStd::vector<CloudParticle>& particles, const SMinMaxBox& box)
    {
        m_bReshadeCloud = true;
        m_particles.clear();
        m_particles.reserve(particles.size());

        m_boundingBox = box;

        for (const auto& particle : particles)
        {
            auto newParticle = AZStd::make_shared<CloudParticle>(particle);
            m_particles.emplace_back(newParticle);
        }
    }

    void CloudRenderElement::SortParticles(const Vec3& vViewDir, const Vec3& vSortPoint, ESortDirection eDir)
    {
        Vec3 partPos;
        for (uint32 i = 0; i < m_particles.size(); ++i)
        {
            partPos = m_particles[i]->GetPosition();
            partPos -= vSortPoint;
            m_particles[i]->SetSquareSortDistance(partPos * vViewDir);
        }

        switch (eDir)
        {
        case ESortDirection::eSort_TOWARD:
            AZStd::sort(m_particles.begin(), m_particles.end(), [](const AZStd::shared_ptr<CloudParticle>& a, const AZStd::shared_ptr<CloudParticle>& b) { return b < a; });
            break;
        case ESortDirection::eSort_AWAY:
            AZStd::sort(m_particles.begin(), m_particles.end(), [](const AZStd::shared_ptr<CloudParticle>& a, const AZStd::shared_ptr<CloudParticle>& b) { return a < b; });
            break;
        default:
            break;
        }
    }

    void CloudRenderElement::GetIllumParams(ColorF& specColor, ColorF& diffColor)
    {
        // Fill out specular and diffuse colors if available
        // otherwise determine the diffuse and specular from sun color
        SRenderPipeline* pRenderPipeline = gEnv->pRenderer->GetRenderPipeline();
        IRenderShaderResources* shaderResources = pRenderPipeline->m_pShaderResources;
        if (shaderResources && shaderResources->HasLMConstants())
        {
            specColor = shaderResources->GetColorValue(EFTT_SPECULAR);
            diffColor = shaderResources->GetColorValue(EFTT_DIFFUSE);
        }
        else
        {
            ColorF col = pRenderPipeline->m_pSunLight->m_Color;
            float fLum = col.Luminance();
            col.NormalizeCol(diffColor);

            specColor.a = 0.1f;
            specColor = specColor * fLum / 1.5f;

            diffColor = pRenderPipeline->m_pCurObject->m_II.m_AmbColor / 5.0f;
        }
    }

    void CloudRenderElement::ShadeCloud(Vec3 vPos)
    {
        SRenderPipeline* pRenderPipeline = gEnv->pRenderer->GetRenderPipeline();
        ColorF specColor, diffColor;
        if (pRenderPipeline->m_pSunLight)
        {
            GetIllumParams(specColor, diffColor);
            m_CurSpecColor = specColor;
            m_CurDiffColor = diffColor;
            m_bReshadeCloud = false;

            if (pRenderPipeline->m_pCurObject && pRenderPipeline->m_pCurObject->GetRE())
            {
                IRenderElementDelegate* renderElement = pRenderPipeline->m_pCurObject->GetRE()->mfGetDelegate();
                CloudImposterRenderElement* imposter = static_cast<CloudImposterRenderElement*>(renderElement);
                imposter->SetScreenImposterState(true);
            }
        }
    }

    void CloudRenderElement::UpdateWorldSpaceBounds(CRenderObject* renderObject)
    {
        SRenderPipeline* pRenderPipeline = gEnv->pRenderer->GetRenderPipeline();
        IRenderElementDelegate* renderElement = pRenderPipeline->m_pCurObject->GetRE()->mfGetDelegate();
        CloudImposterRenderElement* imposter = static_cast<CloudImposterRenderElement*>(renderElement);
        AZ_Assert(imposter, "Render element was null.");
        if (imposter)
        {
            SMinMaxBox bounds = m_boundingBox;
            bounds.Transform(renderObject->m_II.m_Matrix);
            imposter->SetBBox(bounds.GetMin(), bounds.GetMax());
        }
    }

    void CloudRenderElement::mfPrepare(bool bCheckOverflow)
    {
        IRenderer* renderer = gEnv->pRenderer;

        if (bCheckOverflow)
        {
            renderer->FX_CheckOverflow(0, 0, m_gemRE);
        }

        SRenderPipeline* renderPipeline = renderer->GetRenderPipeline();
        CRenderObject* renderObject = renderPipeline->m_pCurObject;

        IRenderElement* renderElement = renderObject->GetRE();
        IRenderElementDelegate* renderElementDelegate = renderElement ? renderElement->mfGetDelegate() : nullptr;
        CloudImposterRenderElement* imposter = renderElementDelegate ? static_cast<CloudImposterRenderElement*>(renderElementDelegate) : nullptr;
        SRenderObjData* objectData = renderObject->GetObjData();
        assert(objectData);
        if (objectData)
        {
            if (!imposter)
            {
                imposter = new CloudImposterRenderElement();
                renderObject->SetRE(imposter->GetRE());

                // Setup alpha blending
                imposter->SetState(GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA | GS_ALPHATEST_GREATER);
                imposter->SetAlphaRef(0);
            }

            // Reshade of scale has changed
            if (objectData->m_fTempVars[0] != objectData->m_fTempVars[1])
            {
                objectData->m_fTempVars[1] = objectData->m_fTempVars[0];
                m_bReshadeCloud = true;
            }
            m_fScale = objectData->m_fTempVars[0];


            // Reshade if diffuse or specular has changed
            ColorF specColor, diffColor;
            GetIllumParams(specColor, diffColor);
            if (specColor != m_CurSpecColor || diffColor != m_CurDiffColor)
            {
                m_bReshadeCloud = true;
            }

            UpdateWorldSpaceBounds(renderObject);
            Vec3 vPos = imposter->GetPosition();

            if (m_bReshadeCloud)
            {
                ShadeCloud(vPos);
            }
            UpdateImposter(renderObject);

            renderPipeline->m_pCurObject = renderObject;
            renderPipeline->SetRenderElement(m_gemRE);
            renderPipeline->m_RendNumIndices = 0;
            renderPipeline->m_RendNumVerts = 4;
            renderPipeline->m_FirstVertex = 0;
        }
    }

    bool CloudRenderElement::UpdateImposter(CRenderObject* pObj)
    {
#if !defined(DEDICATED_SERVER)
        IRenderer* renderer = gEnv->pRenderer;
        SRenderPipeline* renderPipeline = renderer->GetRenderPipeline();
        CRenderObject* renderObject = renderPipeline->m_pCurObject;
        SRenderObjData* objectData = renderObject->GetObjData();
        IRenderElementDelegate* renderElement = renderObject->GetRE()->mfGetDelegate();
        CloudImposterRenderElement* imposter = static_cast<CloudImposterRenderElement*>(renderElement);

        if (!imposter->PrepareForUpdate())
        {
            bool isAlwaysUpdated = renderer->GetBooleanConfigurationValue("r_CloudsUpdateAlways", true);
            if (isAlwaysUpdated && imposter->GetFrameReset() == renderer->GetFrameReset())
            {
                return true;
            }
        }

        imposter->SetFrameResetValue(renderer->GetFrameReset());

        // Save view and projection matrices along with viewport dimensions
        int iOldVP[4];
        renderer->GetViewport(&iOldVP[0], &iOldVP[1], &iOldVP[2], &iOldVP[3]);
        Matrix44A origMatView = renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_matView;
        Matrix44A origMatProj = renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_matProj;

        // Determine and set new viewport
        int nLogX = imposter->GetLogResolutionX();
        int nLogY = imposter->GetLogResolutionY();
        int iResX = 1 << nLogX;
        int iResY = 1 << nLogY;
        while (iResX > renderer->GetCurrentTextureAtlasSize())
        {
            nLogX--;
            iResX = 1 << nLogX;
        }
        while (iResY > renderer->GetCurrentTextureAtlasSize())
        {
            nLogY--;
            iResY = 1 << nLogY;
        }
        renderer->RT_SetViewport(0, 0, iResX, iResY);

        // Write last view matrix into current view matrix
        Matrix44A* m = &renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_matView;
        auto& lastViewParameters = imposter->GetLastViewParameters();
        lastViewParameters.GetModelviewMatrix((float*)m);

        // Write last projection matrix
        m = &renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_matProj;
        mathMatrixPerspectiveOffCenter(m,
            lastViewParameters.fWL, lastViewParameters.fWR, lastViewParameters.fWB,
            lastViewParameters.fWT, lastViewParameters.fNear, lastViewParameters.fFar);

        // Update textures if the view
        IDynTexture** texturePtr = nullptr;
        if (!imposter->IsSplit())
        {
            texturePtr = imposter->IsScreenImposter() ? imposter->GetScreenTexture() : imposter->GetTexture();
            if (!*texturePtr)
            {
                *texturePtr = renderer->CreateDynTexture2(iResX, iResY, FT_STATE_CLAMP, "CloudImposter", eTP_Clouds);
            }
            IDynTexture* texture = *texturePtr;
            if (texture)
            {
                SDepthTexture* pDepth = renderer->GetDepthBufferOrig();
                uint32 nX1, nY1, nW1, nH1;
                texture->Update(iResX, iResY);
                texture->GetImageRect(nX1, nY1, nW1, nH1);
                if (nW1 > (int)renderer->GetBackBufferWidth() || nH1 > (int)renderer->GetBackBufferHeight())
                {
                    pDepth = renderer->FX_GetDepthSurface(nW1, nH1, false);
                }
                texture->ClearRT();
                texture->SetRT(0, true, pDepth);

                uint32 nX, nY, nW, nH;
                texture->GetSubImageRect(nX, nY, nW, nH);

                int nSize = iResX * iResY * 4;
                //                imposter->UpdateMemoryUsage(nSize / 1024);
                renderPipeline->m_PS[renderPipeline->m_nProcessThreadID].m_CloudImpostersSizeUpdate += nSize;

                DrawBillboards(lastViewParameters);

                texture->SetUpdateMask();
                texture->RestoreRT(0, true);
            }
        }
        renderer->RT_SetViewport(iOldVP[0], iOldVP[1], iOldVP[2], iOldVP[3]);

        renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_matView = origMatView;
        renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_matProj = origMatProj;
#endif
        return true;
    }

    bool CloudRenderElement::Display(bool bDisplayFrontOfSplit)
    {
#if !defined(DEDICATED_SERVER)
        IRenderer* renderer = gEnv->pRenderer;
        SRenderPipeline* renderPipeline = renderer->GetRenderPipeline();

        CRenderObject* renderObject = renderPipeline->m_pCurObject;
        IRenderElementDelegate* renderElement = renderObject->GetRE()->mfGetDelegate();
        CloudImposterRenderElement* imposter = static_cast<CloudImposterRenderElement*>(renderElement);

        Vec3 vPos = imposter->GetPosition();
        CShader* shader = renderPipeline->m_pShader;
        SShaderTechnique* shaderTechnique = renderPipeline->m_pCurTechnique;
        SShaderPass* pPass = renderPipeline->m_pCurPass;

        // Verify that debug cvar value is applicable (can't show screen imposters if screen imposter isn't set)
        int cloudsDebug = renderer->GetIntegerConfigurationValue("r_CloudsDebug", 0);
        if (cloudsDebug == 1 && !imposter->IsScreenImposter())
        {
            return true;
        }
        if (cloudsDebug == 2 && imposter->IsScreenImposter())
        {
            return true;
        }

        uint32 nPersFlags2 = renderPipeline->m_PersFlags2;
        renderPipeline->m_PersFlags2 &= ~(RBPF2_COMMIT_PF | RBPF2_COMMIT_CM);

        uint32 nPasses = 0;
        ColorF col(1, 1, 1, 1);

        // For reflection pass, draw billboards directly and exit
        if (renderer->GetRecursionLevel() > 0)
        {
            DrawBillboards(renderer->GetViewParameters());
            return true;
        }

        // Is the imposter a screen imposter or not
        IDynTexture* pDT = imposter->IsScreenImposter() ? *imposter->GetScreenTexture() : *imposter->GetTexture();

        float fOffsetU = 0, fOffsetV = 0;
        if (pDT && (!bDisplayFrontOfSplit || (bDisplayFrontOfSplit && imposter->GetFrontTexture())))
        {
            pDT->Apply(0);
            fOffsetU = 0.5f / (float)pDT->GetWidth();
            fOffsetV = 0.5f / (float)pDT->GetHeight();
        }

        // set depth texture for soft clipping of cloud against scene geometry
        renderer->ApplyDepthTextureState(1, FILTER_POINT, true);

        int State = GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA | GS_ALPHATEST_GREATER;
        if (imposter->IsSplit())
        {
            State |= (!bDisplayFrontOfSplit) ? GS_DEPTHWRITE : GS_NODEPTHTEST;
        }

        renderer->FX_SetState(State, 0);

        Vec4 vCloudColorScale(m_fCloudColorScale, 0, 0, 0);

        if (!imposter->IsScreenImposter())
        {
            Vec3 x, y, z;
            z = vPos - imposter->GetLastViewParameters().vOrigin;
            z.Normalize();

            x = (z ^ imposter->GetLastViewParameters().vY);
            x.Normalize();
            x *= imposter->GetRadiusX();

            y = (x ^ z);
            y.Normalize();
            y *= imposter->GetRadiusY();

            const CameraViewParameters& cam = renderer->GetViewParameters();
            Matrix44A* m = &renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_matView;
            cam.GetModelviewMatrix((float*)m);
            m = &renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_matProj;

            // Perspective projection
            mathMatrixPerspectiveOffCenter(m, cam.fWL, cam.fWR, cam.fWB, cam.fWT, cam.fNear, cam.fFar);

            if (renderer->GetRecursionLevel() <= 0)
            {
                const SRenderTileInfo* rti = renderer->GetRenderTileInfo();
                if (rti->nGridSizeX > 1.f || rti->nGridSizeY > 1.f)
                {
                    // shift and scale viewport
                    m->m00 *= rti->nGridSizeX;
                    m->m11 *= rti->nGridSizeY;
                    m->m20 = (rti->nGridSizeX - 1.f) - rti->nPosX * 2.0f;
                    m->m21 = -((rti->nGridSizeY - 1.f) - rti->nPosY * 2.0f);
                }
            }

            renderer->SetCull(eCULL_None);

            // Set technique
            static CCryNameTSCRC techName("Cloud_Imposter");
            shader->FXSetTechnique(techName);
            shader->FXBegin(&nPasses, FEF_DONTSETSTATES);
            shader->FXBeginPass(0);

            // Set color scale name
            shader->FXSetPSFloat("g_CloudColorScale", &vCloudColorScale, 1);

            // Set position
            Vec3 highlightPosition;
            gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_POS, highlightPosition);
            Vec4 lightningPosition(highlightPosition.x, highlightPosition.y, highlightPosition.z, 0.0f);
            shader->FXSetVSFloat("LightningPos", &lightningPosition, 1);

            // Set color and size
            Vec3 lightColor, lightSize;
            gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_COLOR, lightColor);
            gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_SIZE, lightSize);
            Vec4 lightningColorSize(lightColor.x, lightColor.y, lightColor.z, lightSize.x * 0.01f);
            shader->FXSetVSFloat("LightningColSize", &lightningColorSize, 1);

            renderPipeline->m_nCommitFlags |= FC_MATERIAL_PARAMS;
            renderer->FX_Commit();

            // Draw quad at position of imposter
            Vec3* quadCorners = imposter->GetQuadCorners();
            renderer->DrawQuad3D(
                quadCorners[0] + vPos,
                quadCorners[1] + vPos,
                quadCorners[2] + vPos,
                quadCorners[3] + vPos, col, 0 + fOffsetU, 1 - fOffsetV, 1 - fOffsetU, 0 + fOffsetV);

            int impostersDraw = renderer->GetIntegerConfigurationValue("r_ImpostersDraw", 0);

            // Draw bounding box
            if (impostersDraw & 4)
            {
                renderer->FX_SetState(GS_NODEPTHTEST);

                SAuxGeomRenderFlags auxFlags;
                auxFlags.SetDepthTestFlag(e_DepthTestOff);
                renderer->GetIRenderAuxGeom()->SetRenderFlags(auxFlags);

                const SMinMaxBox worldSpaceBounds = imposter->GetWorldSpaceBounds();
                renderer->GetIRenderAuxGeom()->DrawAABB(AABB(worldSpaceBounds.GetMin(), worldSpaceBounds.GetMax()), false, Col_White, eBBD_Faceted);
            }

            // Draw wireframe
            if (impostersDraw & 2)
            {
                Vec3 vertices[] = { vPos - y - x, vPos - y + x, vPos + y + x, vPos + y - x };
                vtx_idx indices[] = { 0, 1, 2, 0, 2, 3 };

                SAuxGeomRenderFlags auxFlags;
                auxFlags.SetFillMode(e_FillModeWireframe);
                auxFlags.SetDepthTestFlag(e_DepthTestOn);
                renderer->GetIRenderAuxGeom()->SetRenderFlags(auxFlags);
                renderer->GetIRenderAuxGeom()->DrawTriangles(vertices, 4, indices, 6, Col_Green);
            }
        }
        else
        {
            // Draw bounding box if requested
            int impostersDraw = renderer->GetIntegerConfigurationValue("r_ImpostersDraw", 0);
            if (impostersDraw & 4)
            {
                const SMinMaxBox worldSpaceBounds = imposter->GetWorldSpaceBounds();
                renderer->GetIRenderAuxGeom()->DrawAABB(AABB(worldSpaceBounds.GetMin(), worldSpaceBounds.GetMax()), false, Col_Red, eBBD_Faceted);
            }

            // Save current view and projection matrices
            Matrix44A origMatProj = renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_matProj;
            Matrix44A origMatView = renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_matView;

            // Change to an orthographic projection
            Matrix44A* m = &renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_matProj;
            mathMatrixOrthoOffCenterLH(m, -1, 1, -1, 1, -1, 1);

            // If using the main camera, Scale and offset viewport to align with current tile
            if (renderer->GetRecursionLevel() <= 0)
            {
                const SRenderTileInfo* rti = renderer->GetRenderTileInfo();
                if (rti->nGridSizeX > 1.f || rti->nGridSizeY > 1.f)
                {
                    m->m00 *= rti->nGridSizeX;
                    m->m11 *= rti->nGridSizeY;
                    m->m30 = -((rti->nGridSizeX - 1.f) - rti->nPosX * 2.0f);
                    m->m31 = ((rti->nGridSizeY - 1.f) - rti->nPosY * 2.0f);
                }
            }

            // Reset view matrix
            renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_matView.SetIdentity();

            // Set technique and begin shader pass
            shader->FXSetTechnique("Cloud_ScreenImposter");
            shader->FXBegin(&nPasses, FEF_DONTSETSTATES);
            shader->FXBeginPass(0);

            // Cloud world space position
            Vec4 pos(imposter->GetPosition(), 1);
            shader->FXSetVSFloat("vCloudWSPos", &pos, 1);

            // Cloud color scale
            shader->FXSetPSFloat("g_CloudColorScale", &vCloudColorScale, 1);

            // Light position
            Vec3 lPos;
            gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_POS, lPos);
            Vec4 lightningPosition(lPos.x, lPos.y, lPos.z, col.a);
            shader->FXSetVSFloat("LightningPos", &lightningPosition, 1);

            // Pack light color and size
            Vec3 lightColor, lightSize;
            gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_COLOR, lightColor);
            gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_SIZE, lightSize);
            Vec4 lightningColorSize(lightColor.x, lightColor.y, lightColor.z, lightSize.x * 0.01f);
            shader->FXSetVSFloat("LightningColSize", &lightningColorSize, 1);

            float fNear = imposter->GetNear();
            float fFar = imposter->GetFar();

            fNear = fNear < 0 || fNear > 1 ? 0.92f : fNear;
            fFar = fFar < 0 || fFar > 1 ? 0.999f : fFar;
            float fZ = (fNear + fFar) * 0.5f;

            TempDynVB<SVF_P3F_T2F_T3F> vb(renderer);
            vb.Allocate(4);
            SVF_P3F_T2F_T3F* vQuad = vb.Lock();

            Vec3 vCoords[8];
            renderer->GetViewParameters().CalcVerts(vCoords);
            vQuad[0] = { {-1, -1, fZ }, {0, 1},  vCoords[6] - vCoords[2] }; // LB
            vQuad[1] = { { 1, -1, fZ }, {1, 1},  vCoords[7] - vCoords[3] }; // RB
            vQuad[3] = { { 1,  1, fZ }, {1, 0},  vCoords[4] - vCoords[0] }; // RT
            vQuad[2] = { {-1,  1, fZ }, {0, 0},  vCoords[5] - vCoords[1] }; // LT

            vb.Unlock();
            vb.Bind(0);

            renderPipeline->m_nCommitFlags |= FC_MATERIAL_PARAMS;
            renderer->FX_Commit();
            if (!FAILED(renderer->FX_SetVertexDeclaration(0, eVF_P3F_T2F_T3F)))
            {
                renderer->FX_DrawPrimitive(eptTriangleStrip, 0, 4);
            }
            vb.Release();

            // Restore view / proj matrices
            renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_matView = origMatView;
            renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_matProj = origMatProj;
        }

        shader->FXEndPass();
        shader->FXEnd();

        renderPipeline->m_PersFlags2 = nPersFlags2;
        renderPipeline->m_pCurObject = renderObject;
        renderPipeline->m_pShader = shader;
        renderPipeline->m_pCurTechnique = shaderTechnique;
        renderPipeline->m_pCurPass = pPass;
#endif
        return true;
    }

    bool CloudRenderElement::mfDraw(CShader* ef, SShaderPass* pPass)
    {
#if !defined(DEDICATED_SERVER)
        int impostersDraw = gEnv->pRenderer->GetIntegerConfigurationValue("r_ImpostersDraw", 0);
        if (impostersDraw)
        {
            SRenderPipeline* pRenderPipeline = gEnv->pRenderer->GetRenderPipeline();
            IRenderElementDelegate* renderElement = pRenderPipeline->m_pCurObject->GetRE()->mfGetDelegate();
            CloudImposterRenderElement* imposter = static_cast<CloudImposterRenderElement*>(renderElement);

            Display(false);
            if (imposter->IsSplit())
            {
                Display(true);
            }
        }
#endif
        return true;
    }
}
