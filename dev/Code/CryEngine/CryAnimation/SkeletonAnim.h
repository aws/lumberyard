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

#ifndef CRYINCLUDE_CRYANIMATION_SKELETONANIM_H
#define CRYINCLUDE_CRYANIMATION_SKELETONANIM_H
#pragma once

#include <AzCore/Jobs/LegacyJobExecutor.h>

#include "SkeletonPose.h"
#include "AnimationThreadTask.h"
#include "FacialAnimation/FacialModel.h"
#include "Memory/Memory.h"
#include "Memory/Pool.h"
#include "TransitionQueue.h"

#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
#include "PoseModifier/PoseModifier.h"
#endif

class CSkeletonAnim;
class CSkeletonPose;
class CCharInstance;
struct ModelAnimationHeader;

namespace Command
{
    class CBuffer;
}

class CLayer
{
public:
    CTransitionQueue m_transitionQueue;
    CPoseModifierQueue m_poseModifierQueue;
};

#define numVIRTUALLAYERS (ISkeletonAnim::LayerCount)

class CSkeletonAnimTask
    : public CAnimationThreadTask
{
public:
    static bool Initialize();

    static Memory::CPoolFrameLocal* GetMemoryPool() { return s_pMemoryPool; }

private:
    static Memory::CPoolFrameLocal* s_pMemoryPool;

public:
    CSkeletonAnimTask();
    ~CSkeletonAnimTask();

public:
    void Initialize(CSkeletonAnim& skeletonAnim);

    void Begin(const QuatTS& location, bool immediate);
    void Wait();

    void Prepare();
    void Synchronize();

    bool WasStarted() { return m_bStarted; }
    bool IsProcessed() { return m_bProcessed; }

    // CAnimationThreadTask
public:
    void Execute();

private:
    Command::CBuffer* m_pCommandBuffer;
    CSkeletonAnim* m_pSkeletonAnim;
    QuatTS m_location;

    bool m_bStarted;
    bool m_bProcessed;

private:
    AZ::LegacyJobExecutor m_jobExecutor;
};




//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------
class CSkeletonAnim
    : public ISkeletonAnim
{
private:
    friend class CSkeletonAnimTask;

public:
    CSkeletonAnim();
    ~CSkeletonAnim();

    CFacialDisplaceInfo m_facialDisplaceInfo;


    size_t SizeOfThis ();
    void GetMemoryUsage(ICrySizer* pSizer) const;
    void Serialize(TSerialize ser);



    void SetAnimationDrivenMotion(uint32 ts);
    void SetMirrorAnimation(uint32 ts);
    uint32 GetAnimationDrivenMotion() const { return m_AnimationDrivenMotion; };

    void SetTrackViewExclusive(uint32 i);
    void SetTrackViewMixingWeight(uint32 layer, f32 weight);

    uint32 GetTrackViewStatus() const { return m_TrackViewExclusive; };

    bool CalculateSegmentNormalizedTime(uint32& segment, f32& segmentNormalizedTime, const CAnimation* pAnimation, f32 animationNormalizedTime) const;

    virtual float CalculateCompleteBlendSpaceDuration(const CAnimation& rAnimation) const;

    virtual void SetAnimationNormalizedTime(CAnimation* pAnimation, f32 normalizedTime, bool entireClip = true);
    virtual f32 GetAnimationNormalizedTime(uint32 nLayer, uint32 num) const;
    virtual f32 GetAnimationNormalizedTime(const CAnimation* pAnimation) const;

    void SetLayerNormalizedTimeAndSegment (f32 normalizedTime, int nEOC,    int32 nAnimID0, int32 nAnimID1, uint8 nSegment0, uint8 nSegment1);
    virtual void SetLayerNormalizedTime(uint32 layer, f32 normalizedTime);
    virtual f32 GetLayerNormalizedTime(uint32 layer) const;

    // sets the animation speed scale for layers
    void SetLayerPlaybackScale(int32 nLayer, f32 fSpeed);
    f32 GetLayerPlaybackScale(uint32 nLayer) const
    {
        if (nLayer < numVIRTUALLAYERS)
        {
            return m_layers[nLayer].m_transitionQueue.m_fLayerPlaybackScale;
        }
        return 1.0f;
    }

    void SetLayerBlendWeight(int32 nLayer, f32 fMult);

    ILINE uint8 GetActiveLayer(uint8 layer) { return m_layers[layer].m_transitionQueue.m_bActive; }
    ILINE void SetActiveLayer(uint8 layer, uint8 val) { m_layers[layer].m_transitionQueue.m_bActive = val ? true : false; }

    virtual void SetDesiredMotionParam(EMotionParamID id, float value, float deltaTime);
    virtual bool GetDesiredMotionParam(EMotionParamID id, float& value) const;

    void ProcessAnimations(const QuatTS& rAnimCharLocationCurr, bool immediate);
    uint32 BlendManager (f32 deltatime, DynArray<CAnimation>& arrLayer, uint32 nLayer);
    void BlendManagerDebug ();
    uint32 BlendManagerDebug (DynArray<CAnimation>& arrLayer, uint32 nLayer);
    void LayerBlendManager(f32 fDeltaTime, uint32 nLayer);
    void DebugLogTransitionQueueState();


    CLayer m_layers[numVIRTUALLAYERS];
    CPoseModifierQueue m_poseModifierQueue;
#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
    CPoseModifierSetupPtr m_pPoseModifierSetup;
#endif

    int (* m_pEventCallback)(ICharacterInstance*, void*);
    void* m_pEventCallbackData;
    void SetEventCallback(CallBackFuncType func, void* pdata)
    {
        m_pEventCallback = func;
        m_pEventCallbackData = pdata;
    }
    AnimEventInstance GetLastAnimEvent() { return m_LastAnimEvent; };

    void AnimCallback(CAnimation* arrAFIFO, uint32 AnimNo, uint32 AnimQueueLen);
    void AnimCallbackInternal(bool sendAnimEventsForTimeOld, f32 normalizedTimeOld, f32 normalizedTimeNew, struct SAnimCallbackParams& params);



    AnimEventInstance m_LastAnimEvent;

    uint32 m_IsAnimPlaying;

    uint8 m_AnimationDrivenMotion;
    uint8 m_ShowDebugText;
    uint16 m_MirrorAnimation;

    void SetDebugging(uint32 debugFlags);

    bool m_TrackViewExclusive : 1;

    Vec3 GetCurrentVelocity() const;
    const QuatT& GetRelMovement() const;
    f32 GetUserData(int i) const { return m_fUserData[i]; }
    void InitSkeletonAnim(CCharInstance* pInstance, CSkeletonPose* pSkeletonPose);
    void ProcessAnimationUpdate(const QuatTS rAnimLocationCurr);
    void FinishAnimationComputations();

    f32 m_fUserData[NUM_ANIMATION_USER_DATA_SLOTS];

    CSkeletonPose* m_pSkeletonPose;
    CCharInstance* m_pInstance;

    bool m_bTimeUpdated;

    mutable bool m_bCachedRelativeMovementValid; // whether or not the m_cachedRelativeMovement is valid
    mutable QuatT m_cachedRelativeMovement;

    CSkeletonAnimTask m_threadTask;

    IAnimationPoseModifierPtr m_transformPinningPoseModifier;

#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
    IAnimationSerializablePtr GetPoseModifierSetup() { return m_pPoseModifierSetup; }
    IAnimationSerializableConstPtr GetPoseModifierSetup() const { return m_pPoseModifierSetup; }
#endif

    bool PushPoseModifier(uint32 layer, IAnimationPoseModifierPtr poseModifier, const char* name);

    void PoseModifiersPrepare(const QuatTS& location);
    void PoseModifiersExecutePost(Skeleton::CPoseData& poseData, const QuatTS& location);
    void PoseModifiersSynchronize();
    void PoseModifiersSwapBuffersAndClearActive();

    // Interface
public:
    bool StartAnimation(const char* szAnimName0, const CryCharAnimationParams& Params);
    bool StartAnimationById(int32 id, const CryCharAnimationParams& Params);
    bool StopAnimationInLayer(int32 nLayer, f32 BlendOutTime);
    bool StopAnimationsAllLayers();

    CAnimation* FindAnimInFIFO(uint32 nUserToken, int nLayer = 1);
    const CAnimation* FindAnimInFIFO(uint32 nUserToken, int nLayer = 1) const;

    // Ported to CTransitionQueue
    int GetNumAnimsInFIFO(uint32 nLayer) const { return m_layers[nLayer].m_transitionQueue.GetAnimationCount(); }
    CAnimation& GetAnimFromFIFO(uint32 nLayer, uint32 num) { return m_layers[nLayer].m_transitionQueue.GetAnimation(num); }
    const CAnimation& GetAnimFromFIFO(uint32 nLayer, uint32 num) const { return m_layers[nLayer].m_transitionQueue.GetAnimation(num); }
    bool RemoveAnimFromFIFO(uint32 nLayer, uint32 num, bool forceRemove = false) { return m_layers[nLayer].m_transitionQueue.RemoveAnimation(num, forceRemove); }
    void ClearFIFOLayer(uint32 nLayer) { m_layers[nLayer].m_transitionQueue.Clear(); }
    // makes sure there's no anim in this layer's queue that could cause a delay (useful when you want to play an
    // animation that you want to be 100% sure is going to be transitioned to immediately)
    virtual void RemoveTransitionDelayConditions(uint32 nLayer) { m_layers[nLayer].m_transitionQueue.RemoveDelayConditions(); }
    void ManualSeekAnimationInFIFO(uint32 nLayer, uint32 num, float time, bool triggerAnimEvents) { m_layers[nLayer].m_transitionQueue.ManualSeekAnimation(num, time, triggerAnimEvents, *m_pInstance); }

    virtual QuatT CalculateRelativeMovement(const float deltaTime, const bool CurrNext = 0) const;

private:
    uint32 AnimationToQueue(const ModelAnimationHeader* pAnim0, int a0, f32 btime, const CryCharAnimationParams& AnimParams);
    uint32 EvaluateTransitionFlags(CAnimation* arrAnimFiFo, uint32 numAnims);

    uint32 IsAnimationInMemory(CAnimationSet* pAnimationSet, CAnimation* pAnimation);
    void UpdateParameters(CAnimation* arrAnimFiFo, uint32 nMaxActiveInQueue, uint32 nLayer, f32 fDeltaTime);

    // Ported to CTransitionQueue
    void AppendAnimationToQueue(int32 nLayer, const CAnimation& rAnim) { m_layers[nLayer].m_transitionQueue.PushAnimation(rAnim); }
    void UpdateAnimationTime(CAnimation& rAnimation, uint32 nLayer, uint32 NumAnimsInQueue, uint32 AnimNo, uint32 idx);
    uint32 CheckIsCAFLoaded(CAnimationSet* pAnimationSet, int32 nAnimID);
    uint32 GetMaxSegments(const CAnimation& rAnimation) const;

public:
    SParametricSamplerInternal* AllocateRuntimeParametric();

#ifdef EDITOR_PCDEBUGCODE
    bool ExportHTRAndICAF(const char* szAnimationName, const char* savePath) const;
    bool ExportVGrid(const char* szAnimationName, const char* savePath = nullptr) const;
#endif

private:
    void Commands_Create(const QuatTS& location, Command::CBuffer& buffer);

    void CreateCommands_AnimationsInUpperLayer(uint32 layerIndex, CAnimationSet* pAnimationSet, const f32 upperLayersWeightFactor, Command::CBuffer& buffer);

    void Commands_BasePlayback(const CAnimation& rAnim, Command::CBuffer& buffer);
    void Commands_BaseEvaluationLMG(const CAnimation& rAnim, uint32 nTargetBuffer, Command::CBuffer& buffer);
    void Commands_LPlayback(const CAnimation& rAnim, uint32 nTargetBuffer, uint32 nSourceBuffer, uint32 nVLayer, f32 weightFactor, Command::CBuffer& buffer);

    void ParseLayer0(const CAnimation& rAnim, struct AnimInfo* ainfo, uint32& acounter, f32 fDeltaTime, const uint32 idx) const;
    void Extract_DeltaMovement(struct AnimInfo* pAInfo, uint32& acounter, struct SDeltaMotion*    pDeltaMotion) const;
    void GetOP_CubicInterpolation(IController* pRootController, uint32 IsCycle, f32 fStartKey, f32 fNumKeys, f32 fKeyTime, Quat& rot, Vec3& pos) const;
    IController* GetRootController(GlobalAnimationHeaderCAF& rCAF) const;

    typedef enum
    {
        FAC_ImmediateCall,
        FAC_ThreadedCall
    } FinishAnimationComputationCallMethod;
    void FinishAnimationComputations(FinishAnimationComputationCallMethod callMethod);
} _ALIGN(128);

#endif // CRYINCLUDE_CRYANIMATION_SKELETONANIM_H
