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

#include "CryLegacy_precompiled.h"
#include "SkeletonAnim.h"
#include "CharacterInstance.h"
#include "FacialAnimation/FacialInstance.h"
#include "Command_Buffer.h"
#include "ParametricSampler.h"
#include "CharacterManager.h"

//

Memory::CPoolFrameLocal* CSkeletonAnimTask::s_pMemoryPool = NULL;

//

bool CSkeletonAnimTask::Initialize()
{
    Memory::CContext& memoryContext = CAnimationContext::Instance().GetMemoryContext();
    s_pMemoryPool = Memory::CPoolFrameLocal::Create(memoryContext, 256000);
    if (!s_pMemoryPool)
    {
        return false;
    }

    return true;
}

//

CSkeletonAnimTask::CSkeletonAnimTask()
    : m_pCommandBuffer(NULL)
    , m_pSkeletonAnim(NULL)
    , m_bStarted(false)
    , m_bProcessed(false)
{
}

CSkeletonAnimTask::~CSkeletonAnimTask()
{
}

//

void CSkeletonAnimTask::Initialize(CSkeletonAnim& skeletonAnim)
{
    m_pSkeletonAnim = &skeletonAnim;
}

void CSkeletonAnimTask::Begin(const QuatTS& location, bool immediate)
{
    m_location = location;

    Prepare();

    if (Started())
    {
        CryFatalError("Animation Task started while already running!");
        return;
    }

    // wait for the skinning transformation from the last frame to finish
    // *note* the skinning pool id is increased in EF_Start, which is called during the frame
    // after the CommandBuffer Job, thus we need the current id to get the skinning data from the last frame
    int nFrameID = gEnv->pRenderer->EF_GetSkinningPoolID();
    int nList = nFrameID % 3;
    SSkinningData* pSkinningData = m_pSkeletonAnim->m_pInstance->arrSkinningRendererData[nList].pSkinningData;
    if (m_pSkeletonAnim->m_pInstance->arrSkinningRendererData[nList].nFrameID == nFrameID && pSkinningData && pSkinningData->pAsyncJobExecutor)
    {
        pSkinningData->pAsyncJobExecutor->WaitForCompletion();
    }

    if (immediate)
    {
        Execute();

        //--- TODO, risk of multiple Synchs from this the immediate mode should be implemented in CAnimationThreadTask instead
        Synchronize();

        return;
    }

	m_jobExecutor.Reset();
    m_jobExecutor.StartJob(
        [this]()
        {
            this->Execute();
        }
    );

    CAnimationThreadTask::Begin();
}

void CSkeletonAnimTask::Wait()
{
    DEFINE_PROFILER_FUNCTION();

    if (!Started())
    {
        return;
    }

    m_jobExecutor.WaitForCompletion();
    Synchronize();

    CAnimationThreadTask::Wait();
}

void CSkeletonAnimTask::Prepare()
{
    m_bProcessed = false;

    if (!s_pMemoryPool)
    {
        return;
    }

    CSkeletonPose* pSkeletonPose = m_pSkeletonAnim->m_pSkeletonPose;
    if (!pSkeletonPose->PreparePoseDataAndLocatorWriteables(*s_pMemoryPool))
    {
        return;
    }

    m_pCommandBuffer = (Command::CBuffer*)s_pMemoryPool->Allocate(sizeof(Command::CBuffer));
    if (!m_pCommandBuffer)
    {
        return;
    }

    m_pSkeletonAnim->PoseModifiersPrepare(m_location);

    m_pSkeletonAnim->Commands_Create(m_location, *m_pCommandBuffer);
    m_pSkeletonAnim->m_IsAnimPlaying = m_pCommandBuffer->GetCommandCount() != 0;

    pSkeletonPose->m_physics.SetLocation(m_location);
    pSkeletonPose->m_physics.Job_SynchronizeWithPhysicsPrepare(*s_pMemoryPool);

    m_bStarted = true;
}

void CSkeletonAnimTask::Synchronize()
{
    CSkeletonPose* pSkeletonPose = m_pSkeletonAnim->m_pSkeletonPose;
    pSkeletonPose->SynchronizePoseDataAndLocatorWriteables();

    m_bStarted = false;
}

// CAnimationThreadTask

void CSkeletonAnimTask::Execute()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Animation);

    if (!m_pSkeletonAnim->m_pSkeletonPose->m_bFullSkeletonUpdate)
    {
        return; // TODO: Should not even create a task!
    }
    //_controlfp( _EM_INEXACT|_EM_INVALID|_EM_UNDERFLOW|_EM_OVERFLOW,_MCW_EM );
    //_controlfp( _EM_INVALID|_EM_OVERFLOW,_MCW_EM );
    //_controlfp(0, _EM_INVALID|_EM_ZERODIVIDE|_EM_OVERFLOW );
    //_controlfp(0, _EM_INVALID|_EM_ZERODIVIDE);

    CSkeletonPose* pSkeletonPose = m_pSkeletonAnim->m_pSkeletonPose;
    if (!pSkeletonPose->GetPoseDataWriteable())
    {
        return;
    }

    if (!m_pCommandBuffer)
    {
        return;
    }

    m_pCommandBuffer->Execute();

    pSkeletonPose->GetPoseData().Validate(*pSkeletonPose->m_pInstance->m_pDefaultSkeleton);

    m_bProcessed = true;
}


//
/*
CSkeletonAnim
*/

CSkeletonAnim::CSkeletonAnim()
{
}

CSkeletonAnim::~CSkeletonAnim()
{
    StopAnimationsAllLayers();
}

//

void CSkeletonAnim::InitSkeletonAnim(CCharInstance* pInstance, CSkeletonPose* pSkeletonPose)
{
    m_pInstance = pInstance;
    m_pSkeletonPose = pSkeletonPose;
    m_ShowDebugText = 0;
    m_AnimationDrivenMotion = 0;
    m_MirrorAnimation = 0;
    m_IsAnimPlaying = 0;
    m_bTimeUpdated = false;

    m_layers[0].m_transitionQueue.SetFirstLayer();

    m_pEventCallback = 0;
    m_pEventCallbackData = 0;

    m_TrackViewExclusive = 0;

    for (int i = 0; i < NUM_ANIMATION_USER_DATA_SLOTS; i++)
    {
        m_fUserData[i] = 0.0f;
    }

    for (uint i = 0; i < numVIRTUALLAYERS; ++i)
    {
        m_layers[i].m_transitionQueue.SetAnimationSet(m_pInstance->m_pDefaultSkeleton->m_pAnimationSet);
        m_layers[i].m_transitionQueue.m_bActive = false;
    }

    //----------------------------------------------------
    m_cachedRelativeMovement.SetIdentity();
    m_bCachedRelativeMovementValid = false;

    //----------------------------------------------------

    m_threadTask.Initialize(*this);
}

void CSkeletonAnim::FinishAnimationComputations()
{
    FinishAnimationComputations(FAC_ThreadedCall);
}


void CSkeletonAnim::FinishAnimationComputations(FinishAnimationComputationCallMethod callMethod)
{
    DEFINE_PROFILER_FUNCTION();

    if (callMethod == FAC_ThreadedCall)
    {
        if (!m_threadTask.WasStarted())
        {
            return;
        }

        m_threadTask.Wait();
    }

    m_pSkeletonPose->GetPoseData().ValidateRelative(*m_pInstance->m_pDefaultSkeleton);

    //check if the oldest animation in the queue is still needed
    for (uint32 nVLayerNo = 0; nVLayerNo < numVIRTUALLAYERS; nVLayerNo++)
    {
        DynArray<CAnimation>& rAnimations = m_layers[nVLayerNo].m_transitionQueue.m_animations;

        if (rAnimations.size() > 0 && rAnimations[0].GetRemoveFromQueue())
        {
            RemoveAnimFromFIFO(nVLayerNo, 0, true);
        }
    }

    if (m_threadTask.IsProcessed())
    {
        m_pSkeletonPose->GetPoseData().ValidateRelative(*m_pInstance->m_pDefaultSkeleton);

        QuatTS rPhysLocation = m_pInstance->m_location;
        m_pInstance->m_SkeletonPose.SkeletonPostProcess(m_pInstance->m_SkeletonPose.GetPoseDataExplicitWriteable());
        m_pInstance->m_skeletonEffectManager.Update(&m_pInstance->m_SkeletonAnim, &m_pInstance->m_SkeletonPose, rPhysLocation);

        m_transformPinningPoseModifier.reset();
    }
    else
    {
        // [*DavidR | 24/Jan/2011] Update Fall and Play's standup timer even when the postprocess step is not invoked (usually when it is offscreen),
        // otherwise it would cause the character's pose to blend during 1 second after it reenters the frustrum even if the standup anim has finished
        // (see CSkeletonPose::ProcessPhysicsFallAndPlay)
        // [*DavidR | 24/Jan/2011] ToDo: We may want to update more timers (e.g., lying timer) the same way
        m_pSkeletonPose->m_physics.m_timeStandingUp += static_cast<float>(fsel(m_pSkeletonPose->m_physics.m_timeStandingUp, m_pInstance->m_fOriginalDeltaTime, 0.0f));
        m_pInstance->m_SkeletonPose.UpdateAttachments(m_pInstance->m_SkeletonPose.GetPoseDataExplicitWriteable());
        PoseModifiersSwapBuffersAndClearActive();
    }
}

CryGUID GUID_AnimationPoseModifier_TransformationPin = CryGUID::Construct(0xcc34ddea972e47daULL, 0x93f9cdcb98c28c8eULL);
bool CSkeletonAnim::PushPoseModifier(uint32 layer, IAnimationPoseModifierPtr poseModifier, const char* name)
{
    if (poseModifier)
    {
        // NOTE: Special case for the TransformationPin. This should really be
        // reworked to not require this.
        if (poseModifier.get()->GetFactory()->GetClassID() == GUID_AnimationPoseModifier_TransformationPin)
        {
            m_transformPinningPoseModifier = poseModifier;
            return true;
        }

        if (layer == uint32(-1))
        {
            return m_poseModifierQueue.Push(poseModifier, name, m_threadTask.WasStarted());
        }

        if (layer < numVIRTUALLAYERS)
        {
            return m_layers[layer].m_poseModifierQueue.Push(poseModifier, name, m_threadTask.WasStarted());
        }
    }

    return false;
}

void CSkeletonAnim::PoseModifiersPrepare(const QuatTS& location)
{
    Skeleton::CPoseData* pPoseData = m_pSkeletonPose->GetPoseDataWriteable();

    if (m_pSkeletonPose->m_bFullSkeletonUpdate && !m_pSkeletonPose->m_physics.m_bPhysicsRelinquished)
    {
        PushPoseModifier(15, m_pSkeletonPose->m_limbIk, "LimbIK");

        if (m_pSkeletonPose->m_bInstanceVisible && m_pSkeletonPose->m_recoil.get())
        {
            PushPoseModifier(15, m_pSkeletonPose->m_recoil, "Recoil");
        }
    }

#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
    if (m_pPoseModifierSetup)
    {
        PushPoseModifier(-1, m_pPoseModifierSetup->GetPoseModifierStack(), "Setup");
    }
#endif

    // Proper PoseModifiers initialization. Once everything is refactored away
    // this is the only code that should survive.
    SAnimationPoseModifierParams poseModifierParams;
    poseModifierParams.pCharacterInstance = m_pInstance;
    poseModifierParams.pPoseData = pPoseData; // TEMP: Should be NULL;
    float fInstanceDeltaTime = m_pInstance->m_fDeltaTime;
    if (fInstanceDeltaTime != 0.0f)
    {
        poseModifierParams.timeDelta = fInstanceDeltaTime;
    }
    else
    {
        poseModifierParams.timeDelta = m_pInstance->m_fOriginalDeltaTime;
    }
    poseModifierParams.location = location;

    for (uint i = 0; i < numVIRTUALLAYERS; ++i)
    {
        m_layers[i].m_poseModifierQueue.Prepare(poseModifierParams);
    }
    m_poseModifierQueue.Prepare(poseModifierParams);

    if (m_transformPinningPoseModifier)
    {
        m_transformPinningPoseModifier->Prepare(poseModifierParams);
    }

    if (m_pSkeletonPose->m_bFullSkeletonUpdate && !m_pSkeletonPose->m_physics.m_bPhysicsRelinquished)
    {
        if (CPoseBlenderAim* pPBAim = static_cast<CPoseBlenderAim*>(m_pSkeletonPose->m_PoseBlenderAim.get()))
        {
            if (pPBAim->m_blender.m_Set.nDirLayer < numVIRTUALLAYERS)
            {
                if (pPBAim->Prepare(poseModifierParams))
                {
                    PushPoseModifier(pPBAim->m_blender.m_Set.nDirLayer, m_pSkeletonPose->m_PoseBlenderAim, "BlenderAim");
                }
            }
        }

        if (CPoseBlenderLook* pPBLook = static_cast<CPoseBlenderLook*>(m_pSkeletonPose->m_PoseBlenderLook.get()))
        {
            if (pPBLook->m_blender.m_Set.nDirLayer < numVIRTUALLAYERS)
            {
                if (pPBLook->Prepare(poseModifierParams))
                {
                    PushPoseModifier(pPBLook->m_blender.m_Set.nDirLayer, m_pSkeletonPose->m_PoseBlenderLook, "BlenderLook");
                }
            }
        }
    }
}

void CSkeletonAnim::PoseModifiersExecutePost(Skeleton::CPoseData& poseData, const QuatTS& location)
{
    SAnimationPoseModifierParams params;
    params.pCharacterInstance = m_pInstance;
    params.pPoseData = &poseData;
    params.timeDelta = m_pInstance->m_fOriginalDeltaTime;
    params.location = location;

    if (m_transformPinningPoseModifier)
    {
        m_transformPinningPoseModifier->Execute(params);
    }
}


void CSkeletonAnim::PoseModifiersSynchronize()
{
    for (uint i = 0; i < numVIRTUALLAYERS; ++i)
    {
        m_layers[i].m_poseModifierQueue.Synchronize();
        m_layers[i].m_poseModifierQueue.SwapBuffersAndClearActive();
    }
    m_poseModifierQueue.Synchronize();
    m_poseModifierQueue.SwapBuffersAndClearActive();

    if (m_transformPinningPoseModifier)
    {
        m_transformPinningPoseModifier->Synchronize();
    }
}

void CSkeletonAnim::PoseModifiersSwapBuffersAndClearActive()
{
    for (uint i = 0; i < numVIRTUALLAYERS; ++i)
    {
        m_layers[i].m_poseModifierQueue.SwapBuffersAndClearActive();
    }
    m_poseModifierQueue.SwapBuffersAndClearActive();
}

void CSkeletonAnim::ProcessAnimations(const QuatTS& rAnimLocationCurr, bool immediate)
{
    if (!Console::GetInst().ca_thread)
    {
        immediate = true;
    }

    if (m_threadTask.WasStarted())
    {
        CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, "CSkeletonAnim::ProcessAnimations %s %p process called multiple times a frame", m_pInstance->GetFilePath(), m_pInstance);
        return;
    }

    m_bTimeUpdated = false;

    CCharInstance* pInstance = m_pSkeletonPose->m_pInstance;

    if (pInstance->FacialAnimationEnabled())
    {
        // Only update facial animation if we are close - this will add morph effectors to the list
        // that will last only one frame. Therefore if we don't update them, the morphs will be
        // removed automatically.
        if (m_pInstance->m_fPostProcessZoomAdjustedDistanceFromCamera < Console::GetInst().ca_FacialAnimationRadius)
        {
            CFacialInstance* pFaceInst = pInstance->m_pFacialInstance;
            if (pFaceInst)
            {
#if USE_FACIAL_ANIMATION_FRAMERATE_LIMITING
                const f32 fraction = (1.0f - m_pInstance->m_fPostProcessZoomAdjustedDistanceFromCamera / Console::GetInst().ca_FacialAnimationRadius);
                const uint32 fps = uint32(Console::GetInst().ca_FacialAnimationFramerate * fraction);
                pFaceInst->SetTargetFramerate(fps);
#endif
                /*
                    f32 fColor[4] = {1,1,0,1};
                    g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"%s Distance: %f Fraction: %f FPS: %i",m_pInstance->GetFilePath(), m_fPostProcessZoomAdjustedDistanceFromCamera, fraction, fps );
                    g_YLine+=0x20;
                */
                pFaceInst->Update(m_facialDisplaceInfo, pInstance->m_fDeltaTime, rAnimLocationCurr);
            }
        }
        else
        {
            m_facialDisplaceInfo.ClearFast();
        }
    }

    if (m_pSkeletonPose->m_bSetDefaultPoseExecute)
    {
        m_pSkeletonPose->SetDefaultPoseExecute(false);
        m_pSkeletonPose->m_bSetDefaultPoseExecute = false;
    }

    Skeleton::CPoseData& poseData = m_pSkeletonPose->GetPoseDataExplicitWriteable();

    if (!immediate)
    {
        gEnv->pCharacterManager->AddAnimationToSyncQueue(m_pInstance);
    }

    if (m_pInstance->GetCharEditMode() == 0)
    {
        int nCurrentFrameID = g_pCharacterManager->m_nUpdateCounter;
        if (m_pInstance->m_LastUpdateFrameID_Pre == nCurrentFrameID)
        {
            //multiple updates in the same frame can be a problem
            const char* name = m_pInstance->m_pDefaultSkeleton->GetModelFilePath();
            g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, name,    "several pre-updates: FrameID: %x", nCurrentFrameID);
            return;
        }
    }

    m_pInstance->m_LastUpdateFrameID_Pre = g_pCharacterManager->m_nUpdateCounter;

    //

    ProcessAnimationUpdate(rAnimLocationCurr);

    m_threadTask.Begin(rAnimLocationCurr, immediate);
    if (immediate)
    {
        FinishAnimationComputations(FAC_ImmediateCall);
    }
}


//------------------------------------------------------------------------
//---                        ANIMATION-UPDATE                          ---
//-----    we have to do this even if the character is not visible   -----
//------------------------------------------------------------------------
void CSkeletonAnim::ProcessAnimationUpdate(const QuatTS rAnimLocationCurr)
{
    DEFINE_PROFILER_FUNCTION();

    CSkeletonPose*  const __restrict pSkeletonPose = m_pSkeletonPose;

    CPoseBlenderAim* pPBAim = static_cast<CPoseBlenderAim*>(m_pSkeletonPose->m_PoseBlenderAim.get());
    if (pPBAim)
    {
        for (uint32 i = 0; i < MAX_EXEC_QUEUE * 2; i++)
        {
            pPBAim->m_blender.m_DirInfo[i].m_fWeight        = 0.0f;
            pPBAim->m_blender.m_DirInfo[i].m_nGlobalDirID0 = -1;
        }
    }

    if (Console::GetInst().ca_NoAnim)
    {
        return;
    }

    //compute number of animation in this layer
    DynArray<CAnimation>& rCurLayer0 = m_layers[0].m_transitionQueue.m_animations;
    uint32 numAnimsInLayer = rCurLayer0.size();
    CAnimationSet* pAnimationSet = m_pInstance->m_pDefaultSkeleton->m_pAnimationSet;
    for (uint32 a = 0; a < numAnimsInLayer; a++)
    {
        const ModelAnimationHeader* pAnimLMG = pAnimationSet->GetModelAnimationHeader(rCurLayer0[a].GetAnimationId());
        if (pAnimLMG && pAnimLMG->m_nAssetType == LMG_File)
        {
            GlobalAnimationHeaderLMG& rLMG = g_AnimationManager.m_arrGlobalLMG[pAnimLMG->m_nGlobalAnimId];
            //check if ParaGroup is invalid
            uint32 nError = rLMG.m_Status.size();
            if (nError)
            {
                //Usually we can't start invalid ParaGroups. So this must be the result of Hot-Loading
                gEnv->pLog->LogError("CryAnimation: %s: '%s'", rLMG.m_Status.c_str(), rLMG.GetFilePath());
                StopAnimationsAllLayers(); //it might be the best, to just stop all animations
                return;
            }
        }

        if (!rCurLayer0[a].IsActivated())
        {
            break;
        }
    }

    if (1)
    {
        //float fColor[4] = {1,1,0,1};
        //g_pIRenderer->Draw2dLabel( 1,g_YLine, 2.7f, fColor, false,"QueueUpdate: %s",m_pInstance->GetFilePath());
        //g_YLine+=26.0f;

        for (uint32 nVLayerNo = 0; nVLayerNo < numVIRTUALLAYERS; nVLayerNo++)
        {
            DynArray<CAnimation>& rCurLayer = m_layers[nVLayerNo].m_transitionQueue.m_animations;
            uint32 numAnimsPerLayer = rCurLayer.size();
            if (numAnimsPerLayer)
            {
                BlendManager(m_pInstance->m_fDeltaTime * m_layers[nVLayerNo].m_transitionQueue.m_fLayerPlaybackScale,  rCurLayer, nVLayerNo);
            }

            LayerBlendManager(m_pInstance->m_fDeltaTime, nVLayerNo);
        }

#ifndef _RELEASE
        if (Console::GetInst().ca_DebugText || m_ShowDebugText)
        {
            BlendManagerDebug();
        }
#endif

        m_pInstance->m_nForceUpdate = 0;

        m_bTimeUpdated = true;

        // Invalidate cache of relative movement
        m_bCachedRelativeMovementValid = false;
    }
}




//------------------------------------------------------------
//------------------------------------------------------------
//------------------------------------------------------------
void CSkeletonAnim::Serialize(TSerialize ser)
{
    //Disabled. It seems we assign these values to thew wrong instances.
    //This was the reason for Dev_Bug7649. QA confirmed that the game still works without serialization of the queue.
    //I think it is a bad idea to serialize such low-level components.
    //If it is really necessary to serialize the states, then we should do it at AG-level

    // Update: Added a cvar to turn it on again and have it tested by QA.
    if (!Console::GetInst().ca_SerializeSkeletonAnim)
    {
        return;
    }



    // make sure no parallel computations are running while serialization
    FinishAnimationComputations();

    CAnimationSet* pAnimationSet = m_pInstance->m_pDefaultSkeleton->m_pAnimationSet;
    if (ser.GetSerializationTarget() != eST_Network)
    {
        ser.BeginGroup("CSkeletonAnim");
        ser.Value("AnimationDrivenMotion", m_AnimationDrivenMotion);
        ser.Value("ForceSkeletonUpdate", m_pSkeletonPose->m_nForceSkeletonUpdate);

        for (uint32 nLayer = 0; nLayer < numVIRTUALLAYERS; nLayer++)
        {
            ser.BeginGroup("FIFO");
            uint32 nAnimsInFIFO = m_layers[nLayer].m_transitionQueue.m_animations.size();
            ser.Value("nAnimsInFIFO", nAnimsInFIFO);
            if (ser.IsReading())
            {
                m_layers[nLayer].m_transitionQueue.m_animations.resize(nAnimsInFIFO);
            }

            for (uint32 a = 0; a < nAnimsInFIFO; a++)
            {
                CAnimation& anim = m_layers[nLayer].m_transitionQueue.m_animations[a];
                anim.Serialize(ser);

                const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(anim.GetAnimationId());
                CRY_ASSERT(pAnim);
                if (ser.IsReading() && (pAnim->m_nAssetType == LMG_File))
                {
                    anim.m_pParametricSampler = AllocateRuntimeParametric();
                }

#ifdef USE_CRY_ASSERT
                if (anim.GetParametricSampler() != NULL)
                {
                    CRY_ASSERT(pAnim->m_nAssetType == LMG_File);
                }
#endif

                SParametricSamplerInternal* pParametric = (SParametricSamplerInternal*)anim.GetParametricSampler();
                if (pParametric)
                {
                    pParametric->Serialize(ser);
                }

                if (ser.IsReading())
                {
                    if (pParametric == NULL)
                    {
                        const ModelAnimationHeader& AnimHeader = pAnimationSet->GetModelAnimationHeaderRef(anim.GetAnimationId());
                        GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[AnimHeader.m_nGlobalAnimId];
                        if (rGlobalAnimHeader.IsAssetOnDemand())
                        {
                            if (rGlobalAnimHeader.IsAssetLoaded() == 0)
                            {
                                rGlobalAnimHeader.LoadCAF();
                            }

                            rGlobalAnimHeader.m_nRef_at_Runtime++;
                        }
                    }
                    else
                    {
                        int32 numAnims = pParametric->m_numExamples;
                        for (int32 i = 0; i < numAnims; i++)
                        {
                            int32 nAnimID   = pParametric->m_nAnimID[i];
                            if (nAnimID >= 0)
                            {
                                const ModelAnimationHeader& AnimHeader = pAnimationSet->GetModelAnimationHeaderRef(nAnimID);
                                GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[AnimHeader.m_nGlobalAnimId];
                                if (rGlobalAnimHeader.IsAssetOnDemand())
                                {
                                    if (rGlobalAnimHeader.IsAssetLoaded() == 0)
                                    {
                                        rGlobalAnimHeader.LoadCAF();
                                    }

                                    rGlobalAnimHeader.m_nRef_at_Runtime++;
                                }
                            }
                        }
                    }
                }
            }
            ser.EndGroup();
        }

        ser.EndGroup();
    }
}
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////

void CSkeletonAnim::SetDebugging(uint32 debugFlags)
{
    m_ShowDebugText = debugFlags > 0;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
size_t CSkeletonAnim::SizeOfThis()
{
    uint32 TotalSize  = 0;
    for (uint32 i = 0; i < numVIRTUALLAYERS; i++)
    {
        TotalSize += m_layers[i].m_transitionQueue.m_animations.capacity() * sizeof(CAnimation);
        TotalSize += m_layers[i].m_poseModifierQueue.GetSize();
    }
    return TotalSize;
}

void CSkeletonAnim::GetMemoryUsage(ICrySizer* pSizer) const
{
    for (uint32 i = 0; i < numVIRTUALLAYERS; i++)
    {
        pSizer->AddObject(m_layers[i].m_transitionQueue.m_animations);

        m_layers[i].m_poseModifierQueue.AddToSizer(pSizer);
    }

    pSizer->AddObject(m_transformPinningPoseModifier);
}

Vec3 CSkeletonAnim::GetCurrentVelocity() const
{
    float fColDebug[4] = {1, 1, 0, 1};
    f32 fDT = m_pInstance->m_fDeltaTime;
    if (fDT == 0)
    {
        //  g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fColDebug, false,"GetCurrentVelocity: %f", 0 );
        //  g_YLine+=16.0f;
        return Vec3(ZERO);
    }
    //  g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fColDebug, false,"GetCurrentVelocity: %f", (m_RelativeMovement.t/fDT).GetLength() );
    //  g_YLine+=16.0f;
    return GetRelMovement().t / fDT;
};

void CSkeletonAnim::SetTrackViewExclusive(uint32 i)
{
    m_TrackViewExclusive = i > 0;
}

void CSkeletonAnim::SetTrackViewMixingWeight(uint32 layer, f32 weight)
{
    if (!m_TrackViewExclusive)
    {
        CryLog("Trying to set manual mixing weight without being in TrackViewExclusive mode");
        return;
    }
    m_layers[layer].m_transitionQueue.SetManualMixingWeight(weight);
}

SParametricSamplerInternal* CSkeletonAnim::AllocateRuntimeParametric()
{
    for (int i = 0; i < g_totalParametrics; i++)
    {
        if (!g_usedParametrics[i])
        {
            SParametricSamplerInternal* ret = &g_parametricPool[i];
            g_usedParametrics[i] = 1;
            new (ret) SParametricSamplerInternal;
            return ret;
        }
    }

    CRY_ASSERT_TRACE(0, ("Run out of free parametric slots! If current usage is typical increase pool size via ca_ParametricPoolSize. Current Size: %d", g_totalParametrics));
    CryLogAlways("Run out of free parametric slots! If current usage is typical increase pool size via ca_ParametricPoolSize. Current Size: %d", g_totalParametrics);
    return nullptr;
}



