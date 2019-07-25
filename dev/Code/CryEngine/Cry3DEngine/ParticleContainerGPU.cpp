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

#include "ParticleContainerGPU.h"

#include "../CryCommon/IGPUParticleEngine.h"
#include "../CryCommon/IRenderer.h"
#include "../CryCommon/CREParticleGPU.h"
#include "ParticleSubEmitter.h"
#include "ParticleContainer.h"
#include "ParticleEmitter.h"

#include <AzCore/Math/MathUtils.h>

#include <new>
#include <math.h>

#include "../RenderDll/Common/Defs.h"

#define SHADOW_INSTRUCTION_COUNT 32

struct SImpl_GPUParticleContainer
{
    SImpl_GPUParticleContainer()
        : particleContainer(nullptr)
        , renderer(nullptr)
        , reParticleGPU(nullptr)
        , reReflectionPass(nullptr)
        , instance(nullptr)
        , lodBlendAlphaUpdateTime(0)
    {}

    ~SImpl_GPUParticleContainer()
    {
        ReleaseRenderElements();

        DestroyGPUEmitter();
    }

    void CreateGPUEmitter(IGPUParticleEngine::EmitterTypePtr parentGPUEmitter)
    {
        if (!particleContainer || !renderer || !renderer->GetGPUParticleEngine())
        {
            return;
        }

        if (instance)
        {
            DestroyGPUEmitter();
        }

        const SpawnParams& spawnParams = particleContainer->GetMain().GetSpawnParams();

        instance = renderer->GetGPUParticleEngine()->AddEmitter(&particleContainer->GetParams(), spawnParams, &(particleContainer->GetMain().GetEmitterFlags()), parentGPUEmitter, &target, &(particleContainer->GetMain().GetPhysEnviron()));
        AZ_Assert(instance, "GPU emitter was not created!");
    }

    void DestroyGPUEmitter()
    {
        if (!renderer || !renderer->GetGPUParticleEngine())
        {
            return;
        }

        if (instance)
        {
            renderer->GetGPUParticleEngine()->RemoveEmitter(instance);
            instance = nullptr;
        }
    }

    void ReleaseRenderElements()
    {
        if (!renderer || !renderer->GetGPUParticleEngine())
        {
            return;
        }

        IGPUParticleEngine* particleEngine = renderer->GetGPUParticleEngine();

        if (reParticleGPU)
        {
            particleEngine->QueueRenderElementToReleaseNextFrame(reParticleGPU);
            reParticleGPU = nullptr;
        }

        if (reReflectionPass)
        {
            particleEngine->QueueRenderElementToReleaseNextFrame(reReflectionPass);
            reReflectionPass = nullptr;
        }

        for (int i = 0; i < reShadow.size(); i++)
        {
            particleEngine->QueueRenderElementToReleaseNextFrame(reShadow[i]);
        }
        reShadow.clear();
    }

    CParticleContainer*                 particleContainer;
    IRenderer*                          renderer;
    CREParticleGPU*                     reParticleGPU;
    CREParticleGPU*                     reReflectionPass;
    std::vector<CREParticleGPU*>        reShadow;
    IGPUParticleEngine::EmitterTypePtr  instance;
    float                               lodBlendAlphaUpdateTime;
    ParticleTarget                      target;
};

CParticleContainerGPU::CParticleContainerGPU()
{
    CreateImplementation();
}

CParticleContainerGPU::~CParticleContainerGPU()
{
    DestroyImplementation();
}

void CParticleContainerGPU::CreateImplementation()
{
    CRY_ASSERT(sizeof(m_implBuffer) >= sizeof(SImpl_GPUParticleContainer));
    m_impl = new (m_implBuffer) SImpl_GPUParticleContainer();
}

void CParticleContainerGPU::DestroyImplementation()
{
    if (m_impl)
    {
        m_impl->~SImpl_GPUParticleContainer();
        m_impl = nullptr;
    }
}

void CParticleContainerGPU::Initialize(CParticleContainer* particleContainer)
{
    if (!particleContainer)
    {
        return;
    }

    // Recreate implementation if we are initializing again to relese memory correctly
    if (m_impl->particleContainer)
    {
        DestroyImplementation();
        CreateImplementation();
    }

    m_impl->particleContainer = particleContainer;
    m_impl->renderer = particleContainer->GetRenderer();

    if (!m_impl->renderer)
    {
        m_impl->particleContainer = nullptr;
        return;
    }

    IGPUParticleEngine* particleEngine = m_impl->renderer->GetGPUParticleEngine();

    if (!particleEngine)
    {
        m_impl->particleContainer = nullptr;
        m_impl->renderer = nullptr;
        return;
    }

    IGPUParticleEngine::EmitterTypePtr parentGPUEmitter = nullptr;
    if (m_impl->particleContainer->GetParent() && m_impl->particleContainer->GetParent()->GetGPUData())
    {
        parentGPUEmitter = m_impl->particleContainer->GetParent()->GetGPUData()->m_impl->instance;
    }

    m_impl->CreateGPUEmitter(parentGPUEmitter);

    m_impl->reParticleGPU = static_cast<CREParticleGPU*>(m_impl->renderer->EF_CreateRE(eDATA_GPUParticle));
    m_impl->reParticleGPU->SetInstance(m_impl->instance);

    // create render instruction containers for recursive passes (water reflections)
    m_impl->reReflectionPass = static_cast<CREParticleGPU*>(m_impl->renderer->EF_CreateRE(eDATA_GPUParticle));
    m_impl->reReflectionPass->SetInstance(m_impl->instance);
    m_impl->reReflectionPass->SetPass(EGPUParticlePass::Reflection);

    // create shadow render instructions
    for (int i = 0; i < SHADOW_INSTRUCTION_COUNT; i++)
    {
        m_impl->reShadow.push_back(nullptr);
        m_impl->reShadow[i] = static_cast<CREParticleGPU*>(m_impl->renderer->EF_CreateRE(eDATA_GPUParticle));
        m_impl->reShadow[i]->SetInstance(m_impl->instance);
        m_impl->reShadow[i]->SetPass(EGPUParticlePass::Shadow);
        m_impl->reShadow[i]->SetShadowMode(i);
    }
}

void CParticleContainerGPU::Render(SRendParams const& rParam, SPartRenderParams const& PRParams, const SRenderingPassInfo& passInfo)
{
    CRenderObject* pObj = m_impl->renderer->EF_GetObject_Temp(passInfo.ThreadID());
    if (!pObj)
    {
        // could not obtain temporary render object.
        return;
    }

    IGPUParticleEngine* particleEngine = m_impl->renderer->GetGPUParticleEngine();

    if (!particleEngine)
    {
        CRY_ASSERT(particleEngine);
        return;
    }

    SShaderItem* shaderItem = particleEngine->GetRenderShader();

    if (!shaderItem)
    {
        CRY_ASSERT(shaderItem);
        return;
    }

    //Set the lod blend value for gpu.
    SetLodBlendAlpha(m_impl->particleContainer->ComputeLodBlend(PRParams.m_fCamDistance, m_impl->particleContainer->GetAge() - m_impl->lodBlendAlphaUpdateTime));
    m_impl->lodBlendAlphaUpdateTime = m_impl->particleContainer->GetAge();

    const Matrix34 translation(m_impl->particleContainer->GetMain().GetLocation());
    const Matrix34 scale = Matrix34::CreateScale(Vec3(m_impl->particleContainer->GetMain().GetSpawnParams().fSizeScale));
    pObj->m_II.m_Matrix = translation * scale;

    pObj->m_fSort = 0;  //set to 0 to remove bias in sorting

    // aux windows (particle preview window) has no lighting
    if (!passInfo.IsAuxWindow())
    {
        pObj->m_ObjFlags |= FOB_LIGHTVOLUME;
    }
    else
    {
        // remove flags that are not allowed
        pObj->m_ObjFlags &= (~FOB_SOFT_PARTICLE);
        pObj->m_ObjFlags &= (~FOB_PARTICLE_SHADOWS);
    }

    // create lighting information
    auto* pParams = &m_impl->particleContainer->GetParams();

    CRenderObject* pRenderObject = pObj;
    SRenderObjData* pOD = gEnv->pRenderer->EF_GetObjData(pRenderObject, true, passInfo.ThreadID());
    float fEmissive = pParams->fEmissiveLighting;

    // Ambient color for shader incorporates actual ambient lighting, as well as the constant emissive value.
    pRenderObject->m_II.m_AmbColor = ColorF(fEmissive) + rParam.AmbientColor * pParams->fDiffuseLighting * Cry3DEngineBase::Get3DEngine()->m_fParticlesAmbientMultiplier;
    pRenderObject->m_II.m_AmbColor.a = pParams->fDiffuseLighting * Cry3DEngineBase::Get3DEngine()->m_fParticlesLightMultiplier;

    pRenderObject->m_nTextureID = pParams->nTexId;

    pOD->m_FogVolumeContribIdx[0] = pOD->m_FogVolumeContribIdx[1] = PRParams.m_nFogVolumeContribIdx;

    pOD->m_LightVolumeId = PRParams.m_nDeferredLightVolumeId;


    IF (!!pParams->fHeatScale, 0)
    {
        uint32 nHeatAmount = pParams->fHeatScale.GetStore();
    }

    // Set sort distance based on params and bounding box.
    if (pParams->fSortBoundsScale == PRParams.m_fMainBoundsScale)
    {
        pRenderObject->m_fDistance = PRParams.m_fCamDistance * passInfo.GetZoomFactor();
    }
    else
    {
        pRenderObject->m_fDistance = m_impl->particleContainer->GetMain().GetNearestDistance(passInfo.GetCamera().GetPosition(), pParams->fSortBoundsScale) * passInfo.GetZoomFactor();
    }

    pRenderObject->m_fDistance += pParams->fSortOffset;
    pRenderObject->m_ParticleObjFlags = (pParams->bHalfRes ? CREParticle::ePOF_HALF_RES : 0) | (pParams->bVolumeFog ? CREParticle::ePOF_VOLUME_FOG : 0);

    particleEngine->QueueEmitterNextFrame(m_impl->instance, passInfo.IsAuxWindow());

    particleEngine->SetEmitterTransform(m_impl->instance, pObj->m_II.m_Matrix);

    SCameraInfo camInfo(passInfo);

    if (passInfo.IsShadowPass())
    {
        SRendItemSorter rendItemSorter = SRendItemSorter::CreateShadowPassRendItemSorter(passInfo);
        const int shadowIndex = static_cast<int>(passInfo.GetShadowMapType());
        if (m_impl->reShadow[shadowIndex])
        {
            pRenderObject->m_ObjFlags |= FOB_DYNAMIC_OBJECT;
            m_impl->reShadow[shadowIndex]->SetCameraFOV(camInfo.pCamera->GetFov());
            m_impl->reShadow[shadowIndex]->SetAspectRatio(camInfo.pCamera->GetProjRatio());
            m_impl->reShadow[shadowIndex]->SetWireframeEnabled(rParam.bIsShowWireframe);
            m_impl->renderer->EF_AddEf((CRendElementBase*)m_impl->reShadow[shadowIndex], *shaderItem, pObj, passInfo, EFSLIST_SHADOW_GEN, 0, rendItemSorter);
        }
        else
        {
            CRY_ASSERT(m_impl->reShadow[shadowIndex]);
        }
    }
    else
    {
        CREParticleGPU* re = nullptr;

        if (passInfo.IsRecursivePass())
        {
            const int recursiveLevel = static_cast<int>(passInfo.GetRecursiveLevel());
            AZ_Assert(1 == recursiveLevel, "Going deeper than expected for reflection pass. Maybe we need a vector of REs like reShadow does.");
            re = m_impl->reReflectionPass;
        }
        else
        {
            re = m_impl->reParticleGPU;
        }

        SRendItemSorter rendItemSorter = SRendItemSorter::CreateRendItemSorter(passInfo);

        if (re)
        {
            if (!pParams->DepthOfFieldBlur && !passInfo.IsAuxWindow())
            {
                pObj->m_ObjFlags |= FOB_RENDER_TRANS_AFTER_DOF;
            }
            re->SetCameraFOV(camInfo.pCamera->GetFov());
            re->SetAspectRatio(camInfo.pCamera->GetProjRatio());
            re->SetWireframeEnabled(rParam.bIsShowWireframe);
            m_impl->renderer->EF_AddEf((CRendElementBase*)re, *shaderItem, pObj, passInfo, EFSLIST_TRANSP, 1, rendItemSorter);
        }
        else
        {
            CRY_ASSERT(re);
        }
    }

    //get list of objects for cubemap depth collision if necessary
    if (pParams->eDepthCollision == ParticleParams::EDepthCollision::Cubemap)
    {
        AABB cubemapAABB(m_impl->particleContainer->GetMain().GetPos(), pParams->fCubemapFarDistance);
        PodArray<IShadowCaster*> nodes;

        m_impl->particleContainer->m_pObjManager->MakeDepthCubemapRenderItemList(reinterpret_cast<CVisArea*>(m_impl->particleContainer->GetMain().GetEntityVisArea()),
            cubemapAABB, 0, &nodes, passInfo);

        //create RenderingPassInfo for GPU particle cubemap
        SRenderingPassInfo tempPassInfo = SRenderingPassInfo::CreateTempRenderingInfo(
                SRenderingPassInfo::STATIC_OBJECTS | SRenderingPassInfo::GPU_PARTICLE_COLLISION_CUBEMAP |
                SRenderingPassInfo::TERRAIN | SRenderingPassInfo::ENTITIES, passInfo);

        for (int i = 0; i < nodes.Count(); ++i)
        {
            nodes[i]->Render(rParam, tempPassInfo);
        }
    }
}

void CParticleContainerGPU::OnContainerUpdateLife()
{
    if (!m_impl->renderer)
    {
        CRY_ASSERT(m_impl->renderer);
        return;
    }

    IGPUParticleEngine* particleEngine = m_impl->renderer->GetGPUParticleEngine();

    if (!particleEngine)
    {
        CRY_ASSERT(particleEngine);
        return;
    }

    if (!m_impl->particleContainer)
    {
        CRY_ASSERT(m_impl->particleContainer);
        return;
    }

    if (AZ::IsClose(m_impl->particleContainer->GetAge(), 0.0f, EPSILON))
    {
        if (m_impl->instance)
        {
            particleEngine->ResetEmitter(m_impl->instance);
        }
        else
        {
            CRY_ASSERT(m_impl->instance);
        }
    }
    else if (m_impl->particleContainer->GetAge() >= m_impl->particleContainer->GetMain().GetStopAge())
    {
        // Without this, disabling the emitter was not actually stopping particle generation
        particleEngine->StopEmitter(m_impl->instance);
    }
    else if (m_impl->particleContainer->GetAge() > 0)
    {
        particleEngine->StartEmitter(m_impl->instance);
    }
}

void CParticleContainerGPU::SetLodBlendAlpha(float blendAlpha)
{
    IGPUParticleEngine* particleEngine = m_impl->renderer->GetGPUParticleEngine();
    if (particleEngine)
    {
        particleEngine->SetEmitterLodBlendAlpha(m_impl->instance, blendAlpha);
    }
    else
    {
        CRY_ASSERT(particleEngine);
    }
}

void CParticleContainerGPU::OnEffectChange()
{
    //tell GPUEngine to verify that internal emitter still is set up properly
    IGPUParticleEngine* particleEngine = m_impl->renderer->GetGPUParticleEngine();
    if (particleEngine)
    {
        particleEngine->OnEffectChanged(m_impl->instance);
    }
    else
    {
        CRY_ASSERT(particleEngine);
    }
}

void CParticleContainerGPU::Update()
{
    m_impl->target.bTarget = false;
    if (m_impl->particleContainer)
    {
        m_impl->particleContainer->GetTarget(m_impl->target, m_impl->particleContainer->GetDirectEmitter());
    }
    else
    {
        CRY_ASSERT(m_impl->particleContainer);
    }
}

void CParticleContainerGPU::Prime(float equilibriumAge)
{
    IGPUParticleEngine* particleEngine = m_impl->renderer->GetGPUParticleEngine();
    if (particleEngine)
    {
        particleEngine->PrimeEmitter(m_impl->instance, equilibriumAge);
    }
    else
    {
        CRY_ASSERT(particleEngine);
    }
}
