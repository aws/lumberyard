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
#include "CREParticle.h"
#include "FrameProfiler.h"
#include "HeapContainer.h"

#include <iterator>
#include <I3DEngine.h>
#include <IParticles.h>
// Confetti BEGIN:
#include "CREParticleGPU.h"
#include <ParticleParams.h>
// Confetti END:
#include "../RenderView.h"
#include "../Textures/TextureManager.h"

//////////////////////////////////////////////////////////////////////////
// CFillRateManager implementation

void CFillRateManager::AddPixelCount(float fPixels)
{
    if (fPixels > 0.f)
    {
        Lock();
        m_afPixels.push_back(fPixels);
        m_fTotalPixels += fPixels;
        Unlock();
    }
}

void CFillRateManager::ComputeMaxPixels()
{
    FUNCTION_PROFILER_SYS(PARTICLE);

    // Find per-container maximum which will not exceed total.
    // don't use static here, this function can be called before particle cvars are registered
    ICVar* pVar = gEnv->pConsole->GetCVar("e_ParticlesMaxScreenFill");
    if (!pVar)
    {
        return;
    }
    float fMaxTotalPixels = pVar->GetFVal() * gRenDev->GetWidth() * gRenDev->GetHeight();
    float fNewMax = fMaxTotalPixels;

    Lock();

    if (m_fTotalPixels > fMaxTotalPixels)
    {
        // Compute max pixels we can have per emitter before total exceeded,
        // from previous frame's data.
        std::sort(m_afPixels.begin(), m_afPixels.end());
        float fUnclampedTotal = 0.f;
        for_array (i, m_afPixels)
        {
            float fTotal = fUnclampedTotal + (m_afPixels.size() - i) * m_afPixels[i];
            if (fTotal > fMaxTotalPixels)
            {
                fNewMax = (fMaxTotalPixels - fUnclampedTotal) / (m_afPixels.size() - i);
                break;
            }
            fUnclampedTotal += m_afPixels[i];
        }
    }

    // Update current value gradually.
    float fLastMax = m_fMaxPixels;
    float fMaxChange = max(fLastMax, fNewMax) * 0.25f;
    m_fMaxPixels = clamp_tpl(fNewMax, fLastMax - fMaxChange, fLastMax + fMaxChange);

    Reset();

    Unlock();
}

//////////////////////////////////////////////////////////////////////////
//
// CREParticle implementation.
//

CREParticle::CREParticle()
    : m_pVertexCreator(0)
    , m_nFirstVertex(0)
    , m_nFirstIndex(0)
{
    mfSetType(eDATA_Particle);
}

void CREParticle::Reset(IParticleVertexCreator* pVC, int nThreadId)
{
    m_pVertexCreator = pVC;
    m_nThreadId = nThreadId;
    Construct(m_RenderVerts);
    m_nFirstVertex = 0;
    m_nFirstIndex = 0;
}

SRenderVertices* CREParticle::AllocVertices(int nAllocVerts, int nAllocInds)
{
#if !defined(NULL_RENDERER)
    SRenderPipeline& rp = gRenDev->m_RP;

    // Reserve vertex memory, thread-safe
    uint32 nBufferIdx                                   = gEnv->pRenderer->EF_GetSkinningPoolID() % SRenderPipeline::nNumParticleVertexIndexBuffer;
    byte* pVertices                                     = rp.m_pParticleVertexVideoMemoryBase[nBufferIdx];

    // Limit vertex count for 16-bit indices.
    if (nAllocInds > 0)
    {
        nAllocVerts = min(nAllocVerts, (1 << 16));
    }

    LONG nParticleVerticesOffset;
    do
    {
        nParticleVerticesOffset = *(volatile LONG*)&rp.m_nParticleVertexOffset[nBufferIdx];
        if (nAllocVerts * sizeof(SVF_Particle) > rp.m_nParticleVertexBufferAvailableMemory - nParticleVerticesOffset)
        {
            nAllocVerts = (rp.m_nParticleVertexBufferAvailableMemory - nParticleVerticesOffset) / sizeof(SVF_Particle);
        }
    } while (CryInterlockedCompareExchange((volatile LONG*)&rp.m_nParticleVertexOffset[nBufferIdx], nParticleVerticesOffset + nAllocVerts * sizeof(SVF_Particle), nParticleVerticesOffset) != nParticleVerticesOffset);

    m_RenderVerts.aVertices.set(ArrayT(alias_cast<SVF_Particle*>(pVertices + nParticleVerticesOffset), nAllocVerts));
    m_nFirstVertex = check_cast<uint16>(nParticleVerticesOffset / sizeof(SVF_Particle));

    // Reserve index memory, thread-safe
    byte* pIndices                                      = rp.m_pParticleindexVideoMemoryBase[nBufferIdx];

    LONG nParticleIndicesOffset;
    do
    {
        nParticleIndicesOffset = *(volatile LONG*)&rp.m_nParticleIndexOffset[nBufferIdx];
        if (nAllocInds * sizeof(uint16) > rp.m_nParticleIndexBufferAvailableMemory - nParticleIndicesOffset)
        {
            nAllocInds = (rp.m_nParticleIndexBufferAvailableMemory - nParticleIndicesOffset) / sizeof(uint16);
        }
    } while (CryInterlockedCompareExchange((volatile LONG*)&rp.m_nParticleIndexOffset[nBufferIdx], nParticleIndicesOffset + nAllocInds * sizeof(uint16), nParticleIndicesOffset) != nParticleIndicesOffset);

    m_RenderVerts.aIndices.set(ArrayT(alias_cast<uint16*>(pIndices + nParticleIndicesOffset), nAllocInds));
    m_nFirstIndex = nParticleIndicesOffset / sizeof(uint16);

    m_RenderVerts.fPixels = 0.f;

#endif

    return &m_RenderVerts;
}

void CREParticle::mfPrepare(bool bCheckOverflow)
{
    FUNCTION_PROFILER_SYS(PARTICLE);

    CRenderer* rd = gRenDev;
    SRenderPipeline& rRP = rd->m_RP;
    rRP.m_CurVFormat = eVF_P3F_C4B_T4B_N3F2;

    rd->FX_StartMerging();

#if !defined(NULL_RENDERER)

    // the update jobs are synced in the render thread, before working on the particle-batch
    // tell the render pipeline how many vertices/indices we computed and that we are
    // using our own extern video memory buffer
    gRenDev->m_RP.m_RendNumVerts += m_RenderVerts.aVertices.size();
    gRenDev->m_RP.m_RendNumIndices += m_RenderVerts.aIndices.size();

    if (m_RenderVerts.aVertices.size()) // shouldn't be needed, but seems required(likely due a bug somewhere else)
    {
        if (!(gRenDev->m_RP.m_FlagsPerFlush & RBSI_EXTERN_VMEM_BUFFERS))
        {
            gRenDev->m_RP.m_nExternalVertexBufferFirstVertex = m_nFirstVertex;
            gRenDev->m_RP.m_nExternalVertexBufferFirstIndex = m_nFirstIndex;
#if !defined(STRIP_RENDER_THREAD)
            uint32 nPoolIdx = gRenDev->m_nPoolIndexRT % SRenderPipeline::nNumParticleVertexIndexBuffer;
            gRenDev->m_RP.m_pExternalVertexBuffer = gRenDev->m_RP.m_pParticleVertexBuffer[nPoolIdx];
            gRenDev->m_RP.m_pExternalIndexBuffer = gRenDev->m_RP.m_pParticleIndexBuffer[nPoolIdx];
#else
            gRenDev->m_RP.m_pExternalVertexBuffer = gRenDev->m_RP.m_pParticleVertexBuffer[0];
            gRenDev->m_RP.m_pExternalIndexBuffer = gRenDev->m_RP.m_pParticleIndexBuffer[0];
#endif
        }
        gRenDev->m_RP.m_FlagsPerFlush |= RBSI_EXTERN_VMEM_BUFFERS;

        gRenDev->m_FillRateManager.AddPixelCount(m_RenderVerts.fPixels);
    }

#endif

    if (rRP.m_RendNumVerts)
    {
        rRP.m_pRE = this;
    }

#if !defined(_RELEASE)
    if (CRenderer::CV_r_ParticlesDebug)
    {
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG0];
        if (rRP.m_pCurObject && CRenderer::CV_r_ParticlesDebug == 2)
        {
            rRP.m_pCurObject->m_RState &= ~OS_TRANSPARENT;
            rRP.m_pCurObject->m_RState |= OS_ADD_BLEND;
        }
    }
#endif

    if (CRenderer::CV_r_wireframe && rRP.m_pCurObject)
    {
        rRP.m_pCurObject->m_RState &= ~OS_TRANSPARENT;
    }

    assert(rRP.m_pCurObject);
    PREFAST_ASSUME(rRP.m_pCurObject);

    ////////////////////////////////////////////////////////////////////////////////////
    IF ((rd->m_nNumVols > 0) & ((rRP.m_pCurObject->m_ObjFlags & FOB_LIGHTVOLUME) > 0), 1)
    {
        SRenderObjData* pOD = rRP.m_pCurObject->GetObjData();
#ifndef _RELEASE
        if (((int32)pOD->m_LightVolumeId - 1) < 0 || ((int32)pOD->m_LightVolumeId - 1) >= (int32)rd->m_nNumVols)
        {
            __debugbreak();
        }
#endif
        rd->RT_SetLightVolumeShaderFlags(rd->m_pLightVols[ pOD->m_LightVolumeId - 1 ].pData.size());
    }
}

void CREParticle::ComputeVertices(SCameraInfo camInfo, uint64 uRenderFlags)
{
    FUNCTION_PROFILER_SYS(PARTICLE)

    m_pVertexCreator->ComputeVertices(camInfo, this, uRenderFlags, gRenDev->m_FillRateManager.GetMaxPixels());
}


//////////////////////////////////////////////////////////////////////////
//
// CRenderer particle functions implementation.
//


void CRenderer::EF_AddParticle(CREParticle* pParticle, SShaderItem& shaderItem, CRenderObject* pRO, const SRenderingPassInfo& passInfo)
{
#if !defined(NULL_RENDERER)
    if (pRO)
    {
        uint32 nBatchFlags;
        int nList;
        int nThreadID = m_RP.m_nFillThreadID;
        if (!EF_GetParticleListAndBatchFlags(nBatchFlags, nList, shaderItem, pRO, passInfo))
        {
            return;
        }

        SRendItemSorter rendItemSorter = SRendItemSorter::CreateRendItemSorter(passInfo);
        passInfo.GetRenderView()->AddRenderItem(pParticle, pRO, shaderItem, nList, !!(pRO->m_ObjFlags & FOB_AFTER_WATER), nBatchFlags, passInfo, rendItemSorter);
    }
#endif
}

void
CRenderer::EF_RemoveParticlesFromScene()
{
    m_FillRateManager.ComputeMaxPixels();
}

void
CRenderer::EF_CleanupParticles()
{
    for (int t = 0; t < RT_COMMAND_BUF_COUNT; ++t)
    {
        m_ComputeVerticesJobExecutors[t].WaitForCompletion();

        for (int i = m_arrCREParticle[t].size() - 1; i >= 0; i--)
        {
            delete m_arrCREParticle[t][i];
        }
        m_arrCREParticle[t].clear();
    }
}

void
CRenderer::SyncComputeVerticesJobs()
{
    for (int t = 0; t < RT_COMMAND_BUF_COUNT; ++t)
    {
        m_ComputeVerticesJobExecutors[t].WaitForCompletion();
    }
    m_nCREParticleCount[m_pRT->m_nCurThreadFill] = 0;
}

void
CRenderer::SafeReleaseParticleREs()
{
    EF_CleanupParticles();
}

void
CRenderer::GetMemoryUsageParticleREs(ICrySizer* pSizer)
{
    for (int t = 0; t < RT_COMMAND_BUF_COUNT; ++t)
    {
        size_t nSize = m_arrCREParticle[t].size() * sizeof(CREParticle) + m_arrCREParticle[t].capacity() * sizeof(CREParticle*);
        pSizer->AddObject(&m_arrCREParticle[t], nSize);
    }
}

void CRenderer::EF_AddMultipleParticlesToScene(const SAddParticlesToSceneJob* jobs, size_t numJobs, const SRenderingPassInfo& passInfo) PREFAST_SUPPRESS_WARNING(6262)
{
    FUNCTION_PROFILER_SYS(PARTICLE)
    ASSERT_IS_MAIN_THREAD(m_pRT)

    // update fill thread id for particle jobs
    const CCamera& camera = passInfo.GetCamera();
    int threadList = passInfo.ThreadID();

#if !defined(NULL_RENDERER)
    // skip particle rendering in rare cases (like after a resolution change)
    uint32 nPoolIdx = m_nPoolIndex % SRenderPipeline::nNumParticleVertexIndexBuffer;
    if (m_RP.m_pParticleVertexVideoMemoryBase[nPoolIdx] == NULL || m_RP.m_pParticleindexVideoMemoryBase[nPoolIdx] == NULL)
    {
        return;
    }
#endif

    // cap num Jobs to prevent a possible crash
    if (numJobs > nMaxParticleContainer)
    {
        gEnv->p3DEngine->GetParticleManager()->MarkAsOutOfMemory();
        numJobs = nMaxParticleContainer;
    }

    // if we have jobs, set our sync variables to running before starting the jobs
    if (numJobs)
    {
        // preallocate enough objects for particle rendering
        int nREStart = m_nCREParticleCount[threadList];
        m_nCREParticleCount[threadList] += numJobs;

        m_arrCREParticle[threadList].reserve(m_nCREParticleCount[threadList]);
        for (int i = m_arrCREParticle[threadList].size(); i < m_nCREParticleCount[threadList]; i++)
        {
            m_arrCREParticle[threadList].push_back(new CREParticle);
        }

        PrepareParticleRenderObjects(Array<const SAddParticlesToSceneJob>(jobs, numJobs), nREStart, passInfo);
    }
}

///////////////////////////////////////////////////////////////////////////////
void CRenderer::PrepareParticleRenderObjects(Array<const SAddParticlesToSceneJob> aJobs, int nREStart, SRenderingPassInfo passInfo) PREFAST_SUPPRESS_WARNING(6262)
{
#if !defined(NULL_RENDERER)
    FUNCTION_PROFILER_SYS(PARTICLE)

    // == create list of non empty container to submit to the renderer == //
    threadID threadList = passInfo.ThreadID();

    // make sure the GPU doesn't use the VB/IB Buffer we are going to fill anymore
#if !defined(NULL_RENDERER)
    WaitForParticleBuffer(m_nPoolIndex % SRenderPipeline::nNumParticleVertexIndexBuffer);
#endif

    // == now create the render elements and start processing those == //
    if (passInfo.IsGeneralPass())
    {
        m_ComputeVerticesJobExecutors[threadList].PushCompletionFence();
    }

    SRendItemSorter rendItemSorter = SRendItemSorter::CreateParticleRendItemSorter(passInfo);

    SCameraInfo camInfo(passInfo);

    for_array (i, aJobs)
    {
        // Generate the RenderItem for this particle container
        const SAddParticlesToSceneJob& job = aJobs[i];

        int nList;
        uint32 nBatchFlags;
        SShaderItem shaderItem = *job.pShaderItem;
        CRenderObject* pRenderObject = job.pRenderObject;

        if (!EF_GetParticleListAndBatchFlags(nBatchFlags, nList, shaderItem, pRenderObject, passInfo))
        {
            continue;
        }

        CREParticle* pRE = m_arrCREParticle[threadList][ nREStart + i ];
        assert(pRE);
        pRE->Reset(job.pPVC, threadList);
        if (job.nCustomTexId > 0)
        {
            pRE->m_CustomTexBind[0] = job.nCustomTexId;
        }
        else
        {
            if (passInfo.IsAuxWindow())
            {
                CTexture*   pTex = CTextureManager::Instance()->GetDefaultTexture("DefaultProbeCM");
                pRE->m_CustomTexBind[0] = (pTex ? pTex->GetID() : -1);
            }
            else
            {
                CTexture*   pTex = CTextureManager::Instance()->GetBlackTextureCM();
                pRE->m_CustomTexBind[0] = (pTex ? pTex->GetID() : -1);
            }
        }

        if (passInfo.IsGeneralPass())
        {
            // Start new job to compute the vertices
            const uint64 renderFlags = pRenderObject->m_ObjFlags;
            m_ComputeVerticesJobExecutors[threadList].StartJob([pRE, camInfo, renderFlags]()
            {
                pRE->ComputeVertices(camInfo, renderFlags);
            }); // Legacy JobManager priority: eLowPriority
        }
        else
        {
            // Perform it in same thread.
            pRE->ComputeVertices(camInfo, pRenderObject->m_ObjFlags);
        }

        // generate the RenderItem entries for this Particle Element
        passInfo.GetRenderView()->AddRenderItem(pRE, pRenderObject, shaderItem, nList, !!(pRenderObject->m_ObjFlags & FOB_AFTER_WATER), nBatchFlags, passInfo, rendItemSorter);
        rendItemSorter.IncreaseParticleCounter();
    }

    if (passInfo.IsGeneralPass())
    {
        m_ComputeVerticesJobExecutors[threadList].PopCompletionFence();
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
bool CRenderer::EF_GetParticleListAndBatchFlags(uint32& nBatchFlags, int& nList, SShaderItem& shaderItem, CRenderObject* pRO, const SRenderingPassInfo& passInfo)
{
    nBatchFlags = FB_GENERAL;

    const uint8 uHalfResBlend = CV_r_ParticlesHalfResBlendMode ? OS_ADD_BLEND : OS_ALPHA_BLEND;
    bool bHalfRes = (CV_r_ParticlesHalfRes + (pRO->m_ParticleObjFlags & CREParticle::ePOF_HALF_RES)) >= 2       // cvar allows or forces half-res
        && pRO->m_RState & uHalfResBlend
        && pRO->m_ObjFlags & FOB_AFTER_WATER;

    bool bVolumeFog = (pRO->m_ParticleObjFlags & CREParticle::ePOF_VOLUME_FOG) ? true : false;
    bool bUseTessShader = !bVolumeFog && (pRO->m_ObjFlags & FOB_ALLOW_TESSELLATION);

    // Needs to match the technique numbers in the particle shader
    const int TECHNIQUE_TESS = 0;
    const int TECHNIQUE_NO_TESS = 1;
    const int TECHNIQUE_VOL_FOG = 2;

    // Adjust shader and flags.
    if (bUseTessShader)
    {
        shaderItem.m_nTechnique = TECHNIQUE_TESS;
        pRO->m_ObjFlags &= ~FOB_OCTAGONAL;
    }
    else
    {
        // CONFETTI BEGIN: DAVID SROUR
        // HACK. There has to be a better way to handle the following.
#if defined (CRY_USE_METAL)
        // Don't want hull shader
        shaderItem.m_nTechnique = max(shaderItem.m_nTechnique, 0) + 1;
#else
        // Don't want hull shader
        shaderItem.m_nTechnique = bVolumeFog ? TECHNIQUE_VOL_FOG : TECHNIQUE_NO_TESS;
#endif
        // CONFETTI ENDs
    }

    SShaderTechnique* pTech = shaderItem.GetTechnique();
    assert(!pTech || (bUseTessShader && pTech->m_Flags & FHF_USE_HULL_SHADER) || !bUseTessShader);


    // Disable vertex instancing on unsupported hardware, or from cvar.
    /* Confetti iOS Note: David Srour
     * Instanced particles require to draw instances from a certain start position.
     * This feature will only be available with A9 GPUs.
     * Instanced drawing from a base instance index is already implemented at the Metal layer
     * of the renderer if the device supports the feature.
     * For iOS, using r_ParticlesInstanceVertices=1 will assert on the draw calls if the particle
     * effects make use of the draw indexed instanced draw calls with an instance
     * offset > 0 when running on iOS GPU families 1 & 2.
     */
    if (!(pRO->m_ObjFlags & FOB_ALLOW_TESSELLATION))
    {
#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(CREParticle_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        if (!CV_r_ParticlesInstanceVertices)
#endif
        {
            pRO->m_ObjFlags &= ~FOB_POINT_SPRITE;
        }
    }

    if (shaderItem.m_pShader && (((CShader*)shaderItem.m_pShader)->m_Flags & EF_REFRACTIVE))
    {
        if (CV_r_Refraction && CV_r_ParticlesRefraction)
        {
            nBatchFlags |= FB_TRANSPARENT;
            if (CRenderer::CV_r_RefractionPartialResolves)
            {
                pRO->m_ObjFlags |= FOB_REQUIRES_RESOLVE;
            }
        }
        else
        {
            return false;  // skip adding refractive particle
        }
        bHalfRes = false;
    }

    if (pTech)
    {
        int nThreadID = passInfo.ThreadID();
        SRenderObjData* pOD = pRO->GetObjData();

        nBatchFlags |= (pTech->m_nTechnique[TTYPE_PARTICLESTHICKNESSPASS] > 0) ? FB_PARTICLES_THICKNESS : 0;
        pRO->m_ObjFlags |= (pOD && pOD->m_LightVolumeId) ? FOB_LIGHTVOLUME : 0;
    }

    // Add batch flag so we can filter out these particles which need to be rendered after dof. Also disable half res for particles 
    // since we don't want to introduce one extra half res pass after dof. This will be mentioned in user document. 
    if (pRO->m_ObjFlags & FOB_RENDER_TRANS_AFTER_DOF)
    {
        nBatchFlags |= FB_TRANSPARENT_AFTER_DOF;
        bHalfRes = false;
    }

    if (bHalfRes)
    {
        nList = EFSLIST_HALFRES_PARTICLES;
    }
    else
    {
        nList = EFSLIST_TRANSP;
    }

    nList = bVolumeFog ? EFSLIST_FOG_VOLUME : nList;

    return true;
}
