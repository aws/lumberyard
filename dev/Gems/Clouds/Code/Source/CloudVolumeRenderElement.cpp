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

#include <AzCore/std/containers/vector.h>
#include <IEntityRenderState.h> // <> required for Interfuscator

#include "CloudVolumeRenderElement.h"
#include "CloudVolumeTexture.h"

namespace CloudsGem
{
    CloudVolumeRenderElement::CloudVolumeRenderElement()
    {
        m_gemRE = gEnv->pRenderer->EF_CreateRE(eDATA_Gem);
        m_gemRE->mfSetFlags(FCEF_TRANSFORM);
        m_gemRE->mfSetDelegate(this);

        m_inverseWorldMatrix.SetIdentity();
        m_renderBoundsOS.Reset();
    }

    void CloudVolumeRenderElement::mfPrepare(bool bCheckOverflow)
    {
        IRenderer* renderer = gEnv->pRenderer;
        if (bCheckOverflow)
        {
            renderer->FX_CheckOverflow(0, 0, m_gemRE);
        }

        SRenderPipeline* renderPipeline = renderer->GetRenderPipeline();
        renderPipeline->SetRenderElement(m_gemRE);
        renderPipeline->m_RendNumIndices = 0;
        renderPipeline->m_RendNumVerts = 0;
        renderPipeline->m_CurVFormat = eVF_P3F;
    }

    bool CloudVolumeRenderElement::mfSetSampler(int customId, int nTUnit, int nTState, int nTexMaterialSlot, int nSUnit)
    {
        if (customId == TO_VOLOBJ_DENSITY || customId == TO_VOLOBJ_SHADOW)
        {
            IVolumeTexture* texture = customId == TO_VOLOBJ_DENSITY ? m_pDensVol : m_pShadVol;
            int texId = texture ? texture->GetTexID() : 0;
            gEnv->pRenderer->ApplyForID(texId, nTUnit, nTState, nTexMaterialSlot, nSUnit, true);
            return true;
        }
        return false;
    }

    bool CloudVolumeRenderElement::mfDraw(CShader* shader, SShaderPass* pass)
    {
#if !defined(DEDICATED_SERVER)

        IRenderer* renderer = gEnv->pRenderer;
        SRenderPipeline* renderPipeline = renderer->GetRenderPipeline();

        uint32 nFlagsPS2 = renderPipeline->m_PersFlags2;
        renderPipeline->m_PersFlags2 &= ~(RBPF2_COMMIT_PF | RBPF2_COMMIT_CM);

        // render
        uint32 nPasses = 0;
        shader->FXBegin(&nPasses, 0);
        if (!nPasses)
        {
            return false;
        }

        shader->FXBeginPass(0);

        if (m_nearPlaneIntersectsVolume)
        {
            renderer->SetCullMode(R_CULL_FRONT);
            renderer->FX_SetState(GS_COLMASK_RGB | GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
        }
        else
        {
            renderer->SetCullMode(R_CULL_BACK);
            renderer->FX_SetState(GS_COLMASK_RGB | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
        }

        // set vs constants
        shader->FXSetVSFloat("invObjSpaceMatrix", (const Vec4*)&m_inverseWorldMatrix.m00, 3);

        const Vec4 cEyePosVec(m_eyePosInWS, 0);
        shader->FXSetVSFloat("eyePosInWS", &cEyePosVec, 1);

        const Vec4 cViewerOutsideVec(!m_viewerInsideVolume ? 1.0f : 0.0f, m_nearPlaneIntersectsVolume ? 1.0f : 0.0f, 0.0f, 0.0f);
        shader->FXSetVSFloat("viewerIsOutside", &cViewerOutsideVec, 1);

        const Vec4 cEyePosInOSVec(m_eyePosInOS, 0);
        shader->FXSetVSFloat("eyePosInOS", &cEyePosInOSVec, 1);

        // set ps constants
        const Vec4 cEyePosInWSVec(m_eyePosInWS, 0);
        shader->FXSetPSFloat("eyePosInWS", &cEyePosInWSVec, 1);

        ColorF specColor(1, 1, 1, 1);
        ColorF diffColor(1, 1, 1, 1);

        IRenderShaderResources* pRes = renderPipeline->m_pShaderResources;
        if (pRes && pRes->HasLMConstants())
        {
            specColor = pRes->GetColorValue(EFTT_SPECULAR);
            diffColor = pRes->GetColorValue(EFTT_DIFFUSE);
        }

        Vec3 cloudShadingMultipliers;
        gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_CLOUDSHADING_MULTIPLIERS, cloudShadingMultipliers);

        Vec3 brightColor(gEnv->p3DEngine->GetSunColor() * cloudShadingMultipliers.x);
        brightColor = brightColor.CompMul(Vec3(specColor.r, specColor.g, specColor.b));

        const Vec4 darkColorData(0, 0, 0, m_alpha);
        shader->FXSetPSFloat("darkColor", &darkColorData, 1);
        
        const Vec4 brightColorData(brightColor, m_alpha);
        shader->FXSetPSFloat("brightColor", &brightColorData, 1);
        
        const Vec4 cVolumeTraceStartPlane(m_volumeTraceStartPlane.n, m_volumeTraceStartPlane.d);
        shader->FXSetPSFloat("volumeTraceStartPlane", &cVolumeTraceStartPlane, 1);

        const Vec4 cScaleConsts(0.4f, 0, 0, 0);
        shader->FXSetPSFloat("scaleConsts", &cScaleConsts, 1);

        // TODO: optimize shader and remove need to pass inv obj space matrix
        shader->FXSetPSFloat("invObjSpaceMatrix", (const Vec4*)&m_inverseWorldMatrix.m00, 3);


        // commit all render changes
        renderer->FX_Commit();

        // set vertex declaration and streams
        if (!FAILED(renderer->FX_SetVertexDeclaration(0, eVF_P3F)))
        {
            IRenderMesh* pHullMesh = m_pHullMesh.get();
            CDeviceBufferManager* devBufferMgr = renderer->GetDeviceBufferManager();

            // set vertex and index buffer
            pHullMesh->CheckUpdate(0);
            size_t vbOffset(0);
            size_t ibOffset(0);
            D3DBuffer* pVB = devBufferMgr->GetD3D(pHullMesh->GetVBStream(VSF_GENERAL), &vbOffset);
            D3DBuffer* pIB = devBufferMgr->GetD3D(pHullMesh->GetIBStream(), &ibOffset);
            assert(pVB && pIB);
            if (!pVB || !pIB)
            {
                return false;
            }

            HRESULT hr(S_OK);
            hr = renderer->FX_SetVStream(0, pVB, vbOffset, pHullMesh->GetStreamStride(VSF_GENERAL));
            hr = renderer->FX_SetIStream(pIB, ibOffset, (sizeof(vtx_idx) == 2 ? Index16 : Index32));

            renderer->GetPerInstanceConstantBufferPoolPointer()->SetConstantBuffer(renderPipeline->m_RIs[0][0]);

            renderer->FX_DrawIndexedPrimitive(pHullMesh->GetPrimitiveType(), 0, 0, pHullMesh->GetNumVerts(), 0, pHullMesh->GetNumInds());
        }

        shader->FXEndPass();
        shader->FXEnd();

        renderer->FX_ResetPipe();
        renderPipeline->m_PersFlags2 = nFlagsPS2;
#endif
        return true;
    }
}