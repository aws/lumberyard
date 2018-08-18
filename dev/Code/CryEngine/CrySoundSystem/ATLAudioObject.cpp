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
#include "ATLAudioObject.h"
#include "SoundCVars.h"
#include "ATLUtils.h"
#include "Random.h"
#include <IEntitySystem.h>
#include <I3DEngine.h>
#include <IRenderer.h>
#include <IRenderAuxGeom.h>
#include <ISurfaceType.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ReportStartingTriggerInstance(const TAudioTriggerInstanceID audioTriggerInstanceID, const TAudioControlID audioControlID)
    {
        SATLTriggerInstanceState& rTriggerInstState = m_cTriggers.emplace(audioTriggerInstanceID, SATLTriggerInstanceState()).first->second;
        rTriggerInstState.nTriggerID = audioControlID;
        rTriggerInstState.nFlags |= eATS_STARTING;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ReportStartedTriggerInstance(
        const TAudioTriggerInstanceID audioTriggerInstanceID,
        void* const pOwnerOverride,
        void* const pUserData,
        void* const pUserDataOwner,
        const TATLEnumFlagsType nFlags)
    {
        TObjectTriggerStates::iterator Iter(m_cTriggers.find(audioTriggerInstanceID));

        if (Iter != m_cTriggers.end())
        {
            SATLTriggerInstanceState& rTriggerInstState = Iter->second;
            if (rTriggerInstState.numPlayingEvents > 0 || rTriggerInstState.numLoadingEvents > 0)
            {
                rTriggerInstState.pOwnerOverride = pOwnerOverride;
                rTriggerInstState.pUserData = pUserData;
                rTriggerInstState.pUserDataOwner = pUserDataOwner;
                rTriggerInstState.nFlags &= ~eATS_STARTING;

                if ((nFlags & eARF_SYNC_FINISHED_CALLBACK) == 0)
                {
                    rTriggerInstState.nFlags |= eATS_CALLBACK_ON_AUDIO_THREAD;
                }
            }
            else
            {
                // All of the events have either finished before we got here or never started.
                // So we report this trigger as finished immediately.
                ReportFinishedTriggerInstance(Iter);
            }
        }
        else
        {
            g_audioLogger.Log(eALT_WARNING, "Reported a started instance %u that couldn't be found on an object %u",
                audioTriggerInstanceID, GetID());
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ReportStartedEvent(const CATLEvent* const pEvent)
    {
        m_cActiveEvents.insert(pEvent->GetID());
        m_cTriggerImpls.emplace(pEvent->m_nTriggerImplID, SATLTriggerImplState());

        const TObjectTriggerStates::iterator Iter(m_cTriggers.find(pEvent->m_nTriggerInstanceID));

        if (Iter != m_cTriggers.end())
        {
            SATLTriggerInstanceState& rTriggerInstState = Iter->second;

            switch (pEvent->m_audioEventState)
            {
                case eAES_PLAYING:
                {
                    ++(rTriggerInstState.numPlayingEvents);
                    break;
                }
                case eAES_PLAYING_DELAYED:
                {
                    AZ_Assert(rTriggerInstState.numLoadingEvents > 0, "Event state is PLAYING_DELAYED but there are no loading events!");
                    --(rTriggerInstState.numLoadingEvents);
                    ++(rTriggerInstState.numPlayingEvents);
                    break;
                }
                case eAES_LOADING:
                {
                    ++(rTriggerInstState.numLoadingEvents);
                    break;
                }
                case eAES_UNLOADING:
                {
                    // not handled currently
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Unknown event state in ReportStartedEvent (%d)", pEvent->m_audioEventState);
                    break;
                }
            }
        }
        else
        {
            AZ_Assert(false, "ATL Event must exist and was not found in ReportStartedEvent!");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ReportFinishedEvent(const CATLEvent* const pEvent, const bool bSuccess)
    {
        m_cActiveEvents.erase(pEvent->GetID());

        TObjectTriggerStates::iterator Iter(m_cTriggers.begin());

        if (FindPlace(m_cTriggers, pEvent->m_nTriggerInstanceID, Iter))
        {
            switch (pEvent->m_audioEventState)
            {
                case eAES_PLAYING:
                case eAES_PLAYING_DELAYED: // intentional fall-through
                {
                    SATLTriggerInstanceState& rTriggerInstState = Iter->second;
                    AZ_Assert(rTriggerInstState.numPlayingEvents > 0, "ReportFinishedEvent - Trigger instances being decremented too many times!");

                    if (--(rTriggerInstState.numPlayingEvents) == 0 &&
                        rTriggerInstState.numLoadingEvents == 0 &&
                        (rTriggerInstState.nFlags & eATS_STARTING) == 0)
                    {
                        ReportFinishedTriggerInstance(Iter);
                    }

                    DecrementRefCount();
                    break;
                }
                case eAES_LOADING:
                {
                    if (bSuccess)
                    {
                        ReportPrepUnprepTriggerImpl(pEvent->m_nTriggerImplID, true);
                    }

                    DecrementRefCount();
                    break;
                }
                case eAES_UNLOADING:
                {
                    if (bSuccess)
                    {
                        ReportPrepUnprepTriggerImpl(pEvent->m_nTriggerImplID, false);
                    }

                    DecrementRefCount();
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Unknown event state in ReportFinishedEvent (%d)", pEvent->m_audioEventState);
                    break;
                }
            }
        }
        else
        {
            g_audioLogger.Log(eALT_WARNING, "Reported finished event %u on an inactive trigger %u", pEvent->GetID(), pEvent->m_nTriggerID);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ReportPrepUnprepTriggerImpl(const TAudioTriggerImplID nTriggerImplID, const bool bPrepared)
    {
        if (bPrepared)
        {
            m_cTriggerImpls[nTriggerImplID].nFlags |= eATS_PREPARED;
        }
        else
        {
            m_cTriggerImpls[nTriggerImplID].nFlags &= ~eATS_PREPARED;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::SetSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID)
    {
        m_cSwitchStates[nSwitchID] = nStateID;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::SetRtpc(const TAudioControlID nRtpcID, const float fValue)
    {
        m_cRtpcs[nRtpcID] = fValue;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::SetEnvironmentAmount(const TAudioEnvironmentID nEnvironmentID, const float fAmount)
    {
        if (fAmount > 0.0f)
        {
            m_cEnvironments[nEnvironmentID] = fAmount;
        }
        else
        {
            m_cEnvironments.erase(nEnvironmentID);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const TObjectTriggerImplStates& CATLAudioObjectBase::GetTriggerImpls() const
    {
        return m_cTriggerImpls;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const TObjectRtpcMap& CATLAudioObjectBase::GetRtpcs() const
    {
        return m_cRtpcs;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const TObjectEnvironmentMap& CATLAudioObjectBase::GetEnvironments() const
    {
        return m_cEnvironments;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ClearRtpcs()
    {
        m_cRtpcs.clear();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ClearEnvironments()
    {
        m_cEnvironments.clear();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TObjectTriggerInstanceSet CATLAudioObjectBase::GetTriggerInstancesByOwner(void* const owner) const
    {
        AZ_Assert(owner, "Retrieving a filtered list of trigger instances requires a non-null Owner pointer!");
        TObjectTriggerInstanceSet filteredTriggers;

        for (auto& triggerInstanceState : m_cTriggers)
        {
            if (triggerInstanceState.second.pOwnerOverride == owner)
            {
                filteredTriggers.insert(triggerInstanceState.first);
            }
        }

        return filteredTriggers;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::Update(const float fUpdateIntervalMS, const SATLWorldPosition& rListenerPosition)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::Clear()
    {
        m_cActiveEvents.clear();

        m_cTriggers.clear();
        m_cTriggerImpls.clear();
        m_cSwitchStates.clear();
        m_cRtpcs.clear();
        m_cEnvironments.clear();

        m_nRefCounter = 0;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ReportFinishedTriggerInstance(TObjectTriggerStates::iterator& iTriggerEntry)
    {
        SATLTriggerInstanceState& rTriggerInstState = iTriggerEntry->second;
        SAudioRequest oRequest;
        SAudioCallbackManagerRequestData<eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE> oRequestData(rTriggerInstState.nTriggerID);
        oRequest.nFlags = (eARF_PRIORITY_HIGH | eARF_THREAD_SAFE_PUSH | eARF_SYNC_CALLBACK);
        oRequest.nAudioObjectID = GetID();
        oRequest.pData = &oRequestData;
        oRequest.pOwner = rTriggerInstState.pOwnerOverride;
        oRequest.pUserData = rTriggerInstState.pUserData;
        oRequest.pUserDataOwner = rTriggerInstState.pUserDataOwner;

        if ((rTriggerInstState.nFlags & eATS_CALLBACK_ON_AUDIO_THREAD) != 0)
        {
            oRequest.nFlags &= ~eARF_SYNC_CALLBACK;
        }

        AudioSystemThreadSafeRequestBus::Broadcast(&AudioSystemThreadSafeRequestBus::Events::PushRequestThreadSafe, oRequest);

        if ((rTriggerInstState.nFlags & eATS_PREPARED) != 0)
        {
            // if the trigger instance was manually prepared -- keep it
            rTriggerInstState.nFlags &= ~eATS_PLAYING;
        }
        else
        {
            //if the trigger instance wasn't prepared -- kill it
            m_cTriggers.erase(iTriggerEntry);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const float CATLAudioObject::CPropagationProcessor::SRayInfo::s_fSmoothingAlpha = 0.05f;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::CPropagationProcessor::SRayInfo::Reset()
    {
        fTotalSoundOcclusion = 0.0f;
        nNumHits = 0;
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        fAvgHits = 0.0f;
        fDistToFirstObstacle = FLT_MAX;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLAudioObject::CPropagationProcessor::s_canIssueRWIs = false;
    const float CATLAudioObject::CPropagationProcessor::s_minObstructionDistance = 0.3f;
    const size_t CATLAudioObject::CPropagationProcessor::s_maxObstructionRayHits;
    const size_t CATLAudioObject::CPropagationProcessor::s_maxObstructionRays;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    int CATLAudioObject::CPropagationProcessor::OnObstructionTest(const EventPhys* pEvent)
    {
        EventPhysRWIResult* const pRWIResult = (EventPhysRWIResult*)pEvent;

        if (pRWIResult->iForeignData == PHYS_FOREIGN_ID_SOUND_OBSTRUCTION)
        {
            auto const pRayInfo = static_cast<CPropagationProcessor::SRayInfo*>(pRWIResult->pForeignData);

            if (pRayInfo)
            {
                ProcessObstructionRay(pRWIResult->nHits, pRayInfo);

                SAudioRequest oRequest;
                SAudioCallbackManagerRequestData<eACMRT_REPORT_PROCESSED_OBSTRUCTION_RAY> oRequestData(pRayInfo->nAudioObjectID, pRayInfo->nRayID);
                oRequest.nFlags = eARF_THREAD_SAFE_PUSH;
                oRequest.pData = &oRequestData;

                AudioSystemThreadSafeRequestBus::Broadcast(&AudioSystemThreadSafeRequestBus::Events::PushRequestThreadSafe, oRequest);
            }
            else
            {
                g_audioLogger.Log(eALT_WARNING, "Could not calculate obstruction test");
            }
        }
        else
        {
            g_audioLogger.Log(eALT_WARNING, "Unknown object obstruction test (%d)", pRWIResult->iForeignData);
        }

        return 1;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::CPropagationProcessor::ProcessObstructionRay(const int nNumHits, SRayInfo* const pRayInfo, const bool bReset)
    {
        float fTotalOcclusion = 0.0f;
        int nNumRealHits = 0;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        float fMinDistance = FLT_MAX;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

        if (nNumHits > 0)
        {
            ISurfaceTypeManager* const pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
            const size_t nCount = min(static_cast<size_t>(nNumHits) + 1, s_maxObstructionRayHits);

            for (size_t i = 0; i < nCount; ++i)
            {
                const float fDist = pRayInfo->aHits[i].dist;

                if (fDist > 0.0f)
                {
                    ISurfaceType* const pMat = pSurfaceTypeManager->GetSurfaceType(pRayInfo->aHits[i].surface_idx);
                    if (pMat)
                    {
                        IPhysicalEntity* pCollider = pRayInfo->aHits[i].pCollider;
                        IEntity* pEntity = (pCollider ? gEnv->pEntitySystem->GetEntityFromPhysics(pCollider) : nullptr);

                        const ISurfaceType::SPhysicalParams& physParams = pMat->GetPhyscalParams();
                        fTotalOcclusion += physParams.sound_obstruction * (pEntity ? pEntity->GetObstructionMultiplier() : 1.f);// not clamping b/w 0 and 1 for performance reasons
                        ++nNumRealHits;

                #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                        fMinDistance = min(fMinDistance, fDist);
                #endif // INCLUDE_AUDIO_PRODUCTION_CODE
                    }
                }
            }
        }

        fTotalOcclusion = clamp_tpl(fTotalOcclusion, 0.0f, 1.0f);

        // If the num hits differs too much from the last ray, reduce the change in TotalOcclusion in inverse proportion.
        // This reduces thrashing at the boundaries of occluding entities.
        const int nAbsHitDiff = abs(nNumRealHits - pRayInfo->nNumHits);
        const float fNumHitCorrection = nAbsHitDiff > 1 ? 1.0f / nAbsHitDiff : 1.0f;

        pRayInfo->nNumHits = nNumRealHits;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        pRayInfo->fDistToFirstObstacle = fMinDistance;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

        if (bReset)
        {
            pRayInfo->fTotalSoundOcclusion = fTotalOcclusion;

    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
            pRayInfo->fAvgHits = static_cast<float>(pRayInfo->nNumHits);
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
        }
        else
        {
            pRayInfo->fTotalSoundOcclusion += fNumHitCorrection * (fTotalOcclusion - pRayInfo->fTotalSoundOcclusion) * SRayInfo::s_fSmoothingAlpha;

    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
            pRayInfo->fAvgHits += (pRayInfo->nNumHits - pRayInfo->fAvgHits) * SRayInfo::s_fSmoothingAlpha;
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    size_t CATLAudioObject::CPropagationProcessor::NumRaysFromCalcType(const EAudioObjectObstructionCalcType eCalcType)
    {
        size_t nNumRays = 0;
        switch (eCalcType)
        {
            case eAOOCT_IGNORE:
            {
                nNumRays = 0;
                break;
            }
            case eAOOCT_SINGLE_RAY:
            {
                nNumRays = 1;
                break;
            }
            case eAOOCT_MULTI_RAY:
            {
                nNumRays = 5;
                break;
            }
            case eAOOCT_NONE: // falls through
            default:
            {
                g_audioLogger.Log(eALT_WARNING, "Unknown object obstruction calculation type (%d)", eCalcType);
                break;
            }
        }

        return nNumRays;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLAudioObject::CPropagationProcessor::CPropagationProcessor(
        const TAudioObjectID nObjectID,
        const SATLWorldPosition& rPosition,
        size_t& rRefCounter)
        : m_nRemainingRays(0)
        , m_nTotalRays(0)
        , m_oObstructionValue(0.2f, 0.0001f)
        , m_oOcclusionValue(0.2f, 0.0001f)
        , m_rPosition(rPosition)
        , m_rRefCounter(rRefCounter)
        , m_fCurrListenerDist(0.0f)
        , m_eObstOcclCalcType(eAOOCT_NONE)

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        , m_vRayDebugInfos(s_maxObstructionRays)
        , m_fTimeSinceLastUpdateMS(0.0f)
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
    {
        m_vRayInfos.reserve(s_maxObstructionRays);

        for (size_t i = 0; i < s_maxObstructionRays; ++i)
        {
            m_vRayInfos.push_back(SRayInfo(i, nObjectID));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLAudioObject::CPropagationProcessor::~CPropagationProcessor()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::CPropagationProcessor::Update(const float fUpdateMS)
    {
        m_oObstructionValue.Update(fUpdateMS);
        m_oOcclusionValue.Update(fUpdateMS);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        m_fTimeSinceLastUpdateMS += fUpdateMS;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::CPropagationProcessor::SetObstructionOcclusionCalcType(const EAudioObjectObstructionCalcType eObstOcclCalcType)
    {
        m_eObstOcclCalcType = eObstOcclCalcType;

        if (eObstOcclCalcType == eAOOCT_IGNORE)
        {
            // Reset the sound obstruction/occlusion when disabled.
            m_oObstructionValue.Reset();
            m_oOcclusionValue.Reset();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::CPropagationProcessor::GetPropagationData(SATLSoundPropagationData& rPropagationData) const
    {
        rPropagationData.fObstruction = m_oObstructionValue.GetCurrent();
        rPropagationData.fOcclusion = m_oOcclusionValue.GetCurrent();
    }

    static inline float frand_symm()
    {
        return cry_random(-1.0f, 1.0f);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::CPropagationProcessor::RunObstructionQuery(
        const SATLWorldPosition& rListenerPosition,
        const bool bSyncCall,
        const bool bReset)
    {
        if (m_nRemainingRays == 0)
        {
            static const Vec3 vUp(0.0f, 0.0f, 1.0f);
            static const float fMinPeripheralRayOffset = 0.3f;
            static const float fMaxPeripheralRayOffset = 1.0f;
            static const float fMinRandomOffset = 0.05f;
            static const float fMaxRandomOffset = 0.5f;
            static const float fMinOffsetDistance = 1.0f;
            static const float fMaxOffsetDistance = 20.0f;

            // the previous query has already been processed
            const Vec3& vListener = rListenerPosition.GetPositionVec();
            const Vec3& vObject = m_rPosition.GetPositionVec();
            const Vec3 vDiff = vObject - vListener;

            const float fDistance = vDiff.GetLength();

            m_fCurrListenerDist = fDistance;

            m_nTotalRays = NumRaysFromCalcType(m_eObstOcclCalcType);

            const float fOffsetParam = clamp_tpl((fDistance - fMinOffsetDistance) / (fMaxOffsetDistance - fMinOffsetDistance), 0.0f, 1.0f);
            const float fOffsetScale = fMaxPeripheralRayOffset * fOffsetParam + fMinPeripheralRayOffset * (1.0f - fOffsetParam);
            const float fRndOffsetScale = fMaxRandomOffset * fOffsetParam + fMinRandomOffset * (1.0f - fOffsetParam);

            const Vec3 vSide = vDiff.GetNormalized() % vUp;

            // the 0th ray is always shot from the listener to the center of the source
            // the 0th ray only gets 1/2 of the random variation
            CastObstructionRay(
                vListener,
                (vUp * frand_symm() + vSide * frand_symm()) * fRndOffsetScale * 0.5f,
                vDiff,
                0,
                bSyncCall,
                bReset);

            if (m_nTotalRays > 1)
            {
                // rays 1 and 2 start below and above the listener and go towards the source
                CastObstructionRay(
                    vListener - (vUp * fOffsetScale),
                    (vUp * frand_symm() + vSide * frand_symm()) * fRndOffsetScale,
                    vDiff,
                    1,
                    bSyncCall,
                    bReset);

                CastObstructionRay(
                    vListener + (vUp * fOffsetScale),
                    (vUp * frand_symm() + vSide * frand_symm()) * fRndOffsetScale,
                    vDiff,
                    2,
                    bSyncCall,
                    bReset);

                // rays 3 and 4 start left and right of the listener and go towards the source
                CastObstructionRay(
                    vListener - (vSide * fOffsetScale),
                    (vUp * frand_symm() + vSide * frand_symm()) * fRndOffsetScale,
                    vDiff,
                    3,
                    bSyncCall,
                    bReset);

                CastObstructionRay(
                    vListener + (vSide * fOffsetScale),
                    (vUp * frand_symm() + vSide * frand_symm()) * fRndOffsetScale,
                    vDiff,
                    4,
                    bSyncCall,
                    bReset);
            }

            if (bSyncCall)
            {
                // If the ObstructionQuery was synchronous, calculate the new obstruction/occlusion values right away.
                // Reset the resulting values without smoothing if bReset is true.
                ProcessObstructionOcclusion(bReset);
            }
            else
            {
                // for each async ray BATCH increase the refcount by 1
                ++m_rRefCounter;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::CPropagationProcessor::ReportRayProcessed(const size_t nRayID)
    {
        AZ_Assert(m_nRemainingRays > 0, "ReportRayProcessed - There are no more remaining rays!"); // make sure there are rays remaining
        AZ_Assert((0 <= nRayID) && (nRayID <= m_nTotalRays), "ReportRayProcessed - The passed RayID is invalid!"); // check RayID validity

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        if (m_nRemainingRays == 0 || m_rRefCounter == 0)
        {
            g_audioLogger.Log(eALT_FATAL, "Negative ref or ray count on audio object %u", m_vRayInfos[nRayID].nAudioObjectID);
        }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

        if (--m_nRemainingRays == 0)
        {
            ProcessObstructionOcclusion();
            --m_rRefCounter;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::CPropagationProcessor::ReleasePendingRays()
    {
        if (m_nRemainingRays > 0)
        {
            m_nRemainingRays = 0;
            --m_rRefCounter;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::CPropagationProcessor::ProcessObstructionOcclusion(const bool bReset)
    {
        if (m_nTotalRays > 0)
        {
            // the obstruction value always comes from the 0th ray, going directly from the listener to the source
            float fObstruction = m_vRayInfos[0].fTotalSoundOcclusion;
            float fOcclusion = 0.0f;

            if (m_fCurrListenerDist > ATL_FLOAT_EPSILON)
            {
                if (m_nTotalRays > 1)
                {
                    // the total occlusion value is the average of the occlusions of all of the peripheral rays
                    for (size_t i = 1; i < m_nTotalRays; ++i)
                    {
                        fOcclusion += m_vRayInfos[i].fTotalSoundOcclusion;
                    }

                    fOcclusion /= (m_nTotalRays - 1);
                }
                else
                {
                    fOcclusion = fObstruction;
                }

                //the obstruction effect gets less pronounced when the distance between the object and the listener increases
                fObstruction *= min(g_audioCVars.m_fFullObstructionMaxDistance / m_fCurrListenerDist, 1.0f);

                // since the Obstruction filter is applied on top of Occlusion, make sure we only apply what's exceeding the occlusion value
                fObstruction = max(fObstruction - fOcclusion, 0.0f);
            }
            else
            {
                // sound is tracking the listener, there is no obstruction
                fObstruction = 0.0f;
                fOcclusion = 0.0f;
            }

            m_oObstructionValue.SetNewTarget(fObstruction, bReset);
            m_oOcclusionValue.SetNewTarget(fOcclusion, bReset);

    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
            if (m_fTimeSinceLastUpdateMS > 100.0f) // only re-sample the rays about 10 times per second for a smoother debug drawing
            {
                m_fTimeSinceLastUpdateMS = 0.0f;

                for (size_t i = 0; i < m_nTotalRays; ++i)
                {
                    const SRayInfo& rRayInfo = m_vRayInfos[i];
                    SRayDebugInfo& rRayDebugInfo = m_vRayDebugInfos[i];

                    rRayDebugInfo.vBegin = rRayInfo.vStartPosition + rRayInfo.vRndOffset;
                    rRayDebugInfo.vEnd = rRayInfo.vStartPosition + rRayInfo.vRndOffset + rRayInfo.vDirection;

                    if (rRayDebugInfo.vStableEnd.IsZeroFast())
                    {
                        // to be moved to the PropagationProcessor Reset method
                        rRayDebugInfo.vStableEnd = rRayDebugInfo.vEnd;
                    }
                    else
                    {
                        rRayDebugInfo.vStableEnd += (rRayDebugInfo.vEnd - rRayDebugInfo.vStableEnd) * 0.1f;
                    }

                    rRayDebugInfo.fDistToNearestObstacle = rRayInfo.fDistToFirstObstacle;
                    rRayDebugInfo.nNumHits = rRayInfo.nNumHits;
                    rRayDebugInfo.fAvgHits = rRayInfo.fAvgHits;
                    rRayDebugInfo.fOcclusionValue = rRayInfo.fTotalSoundOcclusion;
                }
            }
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::CPropagationProcessor::CastObstructionRay(
        const Vec3& rOrigin,
        const Vec3& rRndOffset,
        const Vec3& rDirection,
        const size_t nRayIdx,
        const bool bSyncCall,
        const bool bReset)
    {
        static const int nPhysicsFlags = (ent_water | ent_static | ent_sleeping_rigid | ent_rigid | ent_terrain);
        SRayInfo& rRayInfo = m_vRayInfos[nRayIdx];

        const int nNumHits = gEnv->pPhysicalWorld->RayWorldIntersection(
                rOrigin + rRndOffset,
                rDirection,
                nPhysicsFlags,
                bSyncCall ? rwi_pierceability0 : (rwi_pierceability0 | rwi_queue),
                rRayInfo.aHits,
                static_cast<int>(s_maxObstructionRayHits),
                nullptr,
                0,
                &rRayInfo,
                PHYS_FOREIGN_ID_SOUND_OBSTRUCTION);

        if (bSyncCall)
        {
            ProcessObstructionRay(nNumHits, &rRayInfo, bReset);
        }
        else
        {
            ++m_nRemainingRays;
        }

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        rRayInfo.vStartPosition = rOrigin;
        rRayInfo.vDirection = rDirection;
        rRayInfo.vRndOffset = rRndOffset;

        if (bSyncCall)
        {
            ++s_nTotalSyncPhysRays;
        }
        else
        {
            ++s_nTotalAsyncPhysRays;
        }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::Update(const float fUpdateIntervalMS, const SATLWorldPosition& rListenerPosition)
    {
        CATLAudioObjectBase::Update(fUpdateIntervalMS, rListenerPosition);
        m_oPropagationProcessor.Update(fUpdateIntervalMS);

        if (CanRunObstructionOcclusion())
        {
            const float fDistance = (m_oPosition.GetPositionVec() - rListenerPosition.GetPositionVec()).GetLength();

            if ((CPropagationProcessor::s_minObstructionDistance < fDistance) && (fDistance < g_audioCVars.m_fOcclusionMaxDistance))
            {
                // make the physics ray cast call sync or async depending on the distance to the listener
                // LY-64528 - disable synchronous raycasts from audio thread, they can cause crashes.
                const bool bSyncCall = false;   // = (fDistance <= g_audioCVars.m_fOcclusionMaxSyncDistance);
                m_oPropagationProcessor.RunObstructionQuery(rListenerPosition, bSyncCall);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::UpdateVelocity(float const fUpdateIntervalMS)
    {
        Vec3 const cPositionDelta = m_oPosition.GetPositionVec() - m_oPreviousPosition.GetPositionVec();
        float const fCurrentVelocity = (1000.0f * cPositionDelta.GetLength()) / fUpdateIntervalMS; // fCurrentVelocity is given in units per second

        if (fabs(fCurrentVelocity - m_fPreviousVelocity) > g_audioCVars.m_fVelocityTrackingThreshold)
        {
            m_fPreviousVelocity = fCurrentVelocity;
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_SET_RTPC_VALUE> oRequestData(SATLInternalControlIDs::nObjectSpeedRtpcID, fCurrentVelocity);

            oRequest.nAudioObjectID = GetID();
            oRequest.nFlags = eARF_THREAD_SAFE_PUSH;
            oRequest.pData = &oRequestData;
            AudioSystemThreadSafeRequestBus::Broadcast(&AudioSystemThreadSafeRequestBus::Events::PushRequestThreadSafe, oRequest);
        }

        m_oPreviousPosition = m_oPosition;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::ReportPhysicsRayProcessed(const size_t nRayID)
    {
        m_oPropagationProcessor.ReportRayProcessed(nRayID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::SetPosition(const SATLWorldPosition& oNewPosition)
    {
        m_oPosition = oNewPosition;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::Clear()
    {
        CATLAudioObjectBase::Clear();
        m_oPosition = SATLWorldPosition();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::SetObstructionOcclusionCalc(const EAudioObjectObstructionCalcType ePassedOOCalcType)
    {
        AZ_Assert(ePassedOOCalcType != eAOOCT_NONE, "Invalid Obstruction/Occlusion type in SetObstructionOcclusionCalc");
        m_oPropagationProcessor.SetObstructionOcclusionCalcType(ePassedOOCalcType);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::ResetObstructionOcclusion(const SATLWorldPosition& rListenerPosition)
    {
        // cast the obstruction rays synchronously and reset the obstruction/occlusion values
        // (instead of merely setting them as targets for the SmoothFloats)
        // LY-64528 - disable synchronous raycasts from audio thread, they can cause crashes.
        const bool bSyncCall = false;
        m_oPropagationProcessor.RunObstructionQuery(rListenerPosition, bSyncCall, true);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::GetPropagationData(SATLSoundPropagationData& rPropagationData)
    {
        m_oPropagationProcessor.GetPropagationData(rPropagationData);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::ReleasePendingRays()
    {
        m_oPropagationProcessor.ReleasePendingRays();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::SetVelocityTracking(const bool bTrackingOn)
    {
        if (bTrackingOn)
        {
            m_oPreviousPosition = m_oPosition;
            m_nFlags |= eAOF_TRACK_VELOCITY;
        }
        else
        {
            m_nFlags &= ~eAOF_TRACK_VELOCITY;
        }
    }

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::CheckBeforeRemoval(const CATLDebugNameStore* const pDebugNameStore)
    {
        // warn if there is activity on an object being cleared
        if (!m_cActiveEvents.empty())
        {
            const char* const sEventString = GetEventIDs("; ").c_str();
            g_audioLogger.Log(eALT_WARNING, "Active events on an object (ID: %u) being released!  #Events: %u   EventIDs: %s", GetID(), m_cActiveEvents.size(), sEventString);
        }

        if (!m_cTriggers.empty())
        {
            const char* const sTriggerString = GetTriggerNames("; ", pDebugNameStore).c_str();
            g_audioLogger.Log(eALT_WARNING, "Active triggers on an object (ID: %u) being released!  #Triggers: %u   TriggerNames: %s", GetID(), m_cTriggers.size(), sTriggerString);
        }
    }


    using TTriggerCountMap = ATLMapLookupType<TAudioControlID, size_t>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> CATLAudioObjectBase::GetTriggerNames(const char* const sSeparator, const CATLDebugNameStore* const pDebugNameStore)
    {
        CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> sTriggerString;

        TTriggerCountMap cTriggerCounts;

        for (auto& trigger : m_cTriggers)
        {
            ++cTriggerCounts[trigger.second.nTriggerID];
        }

        for (auto& triggerCount : cTriggerCounts)
        {
            const char* const pName = pDebugNameStore->LookupAudioTriggerName(triggerCount.first);

            if (pName)
            {
                const size_t nInstances = triggerCount.second;

                if (nInstances == 1)
                {
                    sTriggerString.Format("%s%s%s", sTriggerString.c_str(), pName, sSeparator);
                }
                else
                {
                    sTriggerString.Format("%s%s(%d inst.)%s", sTriggerString.c_str(), pName, nInstances, sSeparator);
                }
            }
        }

        return sTriggerString;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> CATLAudioObjectBase::GetEventIDs(const char* const sSeparator)
    {
        CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> sEventString;
        for (auto& activeEvent : m_cActiveEvents)
        {
            sEventString.Format("%s%u%s", sEventString.c_str(), activeEvent, sSeparator);
        }

        return sEventString;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const float CATLAudioObjectBase::CStateDebugDrawData::fMinAlpha = 0.5f;
    const float CATLAudioObjectBase::CStateDebugDrawData::fMaxAlpha = 1.0f;
    const int CATLAudioObjectBase::CStateDebugDrawData::nMaxToMinUpdates = 100;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLAudioObjectBase::CStateDebugDrawData::CStateDebugDrawData(const TAudioSwitchStateID nState)
        : nCurrentState(nState)
        , fCurrentAlpha(fMaxAlpha)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLAudioObjectBase::CStateDebugDrawData::~CStateDebugDrawData()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::CStateDebugDrawData::Update(const TAudioSwitchStateID nNewState)
    {
        if ((nNewState == nCurrentState) && (fCurrentAlpha > fMinAlpha))
        {
            fCurrentAlpha -= (fMaxAlpha - fMinAlpha) / nMaxToMinUpdates;
        }
        else if (nNewState != nCurrentState)
        {
            nCurrentState = nNewState;
            fCurrentAlpha = fMaxAlpha;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::DrawDebugInfo(IRenderAuxGeom& auxGeom, const Vec3& vListenerPos, const CATLDebugNameStore* const pDebugNameStore) const
    {
        m_oPropagationProcessor.DrawObstructionRays(auxGeom);

        if (!m_cTriggers.empty())
        {
            const Vec3 vPos(m_oPosition.GetPositionVec());
            Vec3 vScreenPos(ZERO);

            if (IRenderer* pRenderer = gEnv->pRenderer)
            {
                pRenderer->ProjectToScreen(vPos.x, vPos.y, vPos.z, &vScreenPos.x, &vScreenPos.y, &vScreenPos.z);

                vScreenPos.x = vScreenPos.x * 0.01f * pRenderer->GetWidth();
                vScreenPos.y = vScreenPos.y * 0.01f * pRenderer->GetHeight();
            }
            else
            {
                vScreenPos.z = -1.0f;
            }

            if ((0.0f <= vScreenPos.z) && (vScreenPos.z <= 1.0f))
            {
                const float fDist = vPos.GetDistance(vListenerPos);

                if ((g_audioCVars.m_nDrawAudioDebug & eADDF_DRAW_SPHERES) != 0)
                {
                    const SAuxGeomRenderFlags nPreviousRenderFlags = auxGeom.GetRenderFlags();
                    SAuxGeomRenderFlags nNewRenderFlags(e_Def3DPublicRenderflags | e_AlphaBlended);
                    nNewRenderFlags.SetCullMode(e_CullModeNone);
                    auxGeom.SetRenderFlags(nNewRenderFlags);
                    const float fRadius = 0.15f;
                    auxGeom.DrawSphere(vPos, fRadius, ColorB(255, 1, 1, 255));
                    auxGeom.SetRenderFlags(nPreviousRenderFlags);
                }

                const float fFontSize = 1.3f;
                const float fLineHeight = 12.0f;

                if ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_OBJECT_STATES) != 0)
                {
                    Vec3 vSwitchPos(vScreenPos);

                    for (auto& switchState : m_cSwitchStates)
                    {
                        const TAudioControlID nSwitchID = switchState.first;
                        const TAudioSwitchStateID nStateID = switchState.second;

                        const char* const pSwitchName = pDebugNameStore->LookupAudioSwitchName(nSwitchID);
                        const char* const pStateName = pDebugNameStore->LookupAudioSwitchStateName(nSwitchID, nStateID);

                        if (pSwitchName && pStateName)
                        {
                            CStateDebugDrawData& oDrawData = m_cStateDrawInfoMap[nSwitchID];
                            oDrawData.Update(nStateID);
                            const float fSwitchTextColor[4] = { 0.8f, 0.8f, 0.8f, oDrawData.fCurrentAlpha };

                            vSwitchPos.y -= fLineHeight;
                            auxGeom.Draw2dLabel(
                                vSwitchPos.x,
                                vSwitchPos.y,
                                fFontSize,
                                fSwitchTextColor,
                                false,
                                "%s: %s\n",
                                pSwitchName,
                                pStateName);
                        }
                    }
                }

                if ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_OBJECT_LABEL) != 0)
                {
                    static const float fObjectTextColor[4] = { 0.90f, 0.90f, 0.90f, 1.0f };
                    static const float fObjectGrayTextColor[4] = { 0.50f, 0.50f, 0.50f, 1.0f };

                    const size_t nNumRays = m_oPropagationProcessor.GetNumRays();
                    SATLSoundPropagationData oPropagationData;
                    m_oPropagationProcessor.GetPropagationData(oPropagationData);

                    const TAudioObjectID nObjectID = GetID();
                    auxGeom.Draw2dLabel(
                        vScreenPos.x,
                        vScreenPos.y,
                        fFontSize,
                        fObjectTextColor,
                        false,
                        "%s  ID: %llu  RefCnt: %2" PRISIZE_T "  Dist: %4.1fm",
                        pDebugNameStore->LookupAudioObjectName(nObjectID),
                        nObjectID,
                        GetRefCount(),
                        fDist);

                    auxGeom.Draw2dLabel(
                        vScreenPos.x,
                        vScreenPos.y + fLineHeight,
                        fFontSize,
                        nNumRays > 0 ? fObjectTextColor : fObjectGrayTextColor,
                        false,
                        "Obst: %3.2f  Occl: %3.2f  #Rays: %1" PRISIZE_T,
                        oPropagationData.fObstruction,
                        oPropagationData.fOcclusion,
                        nNumRays);
                }

                if ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_OBJECT_TRIGGERS) != 0)
                {
                    const float fTriggerTextColor[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
                    CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH> sTriggerString;

                    TTriggerCountMap cTriggerCounts;

                    for (auto& trigger : m_cTriggers)
                    {
                        ++cTriggerCounts[trigger.second.nTriggerID];
                    }

                    for (auto& triggerCount : cTriggerCounts)
                    {
                        const char* const pName = pDebugNameStore->LookupAudioTriggerName(triggerCount.first);
                        if (pName)
                        {
                            const size_t nInstances = triggerCount.second;
                            if (nInstances == 1)
                            {
                                sTriggerString.Format("%s%s\n", sTriggerString.c_str(), pName);
                            }
                            else
                            {
                                sTriggerString.Format("%s%s: %" PRISIZE_T "\n", sTriggerString.c_str(), pName, nInstances);
                            }
                        }
                    }

                    auxGeom.Draw2dLabel(
                        vScreenPos.x,
                        vScreenPos.y + 2.0f * fLineHeight,
                        fFontSize,
                        fTriggerTextColor,
                        false,
                        "%s",
                        sTriggerString.c_str());
                }

                if ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_OBJECT_RTPCS) != 0)
                {
                    const float fRtpcTextColor[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
                    Vec3 vRtpcPos(vScreenPos);

                    for (auto& rtpc : m_cRtpcs)
                    {
                        const TAudioControlID nRtpcID = rtpc.first;
                        const float fRtpcValue = rtpc.second;

                        const char* const pRtpcName = pDebugNameStore->LookupAudioRtpcName(nRtpcID);

                        if (pRtpcName)
                        {
                            const float xOffset = 5.f;

                            vRtpcPos.y -= fLineHeight;	// list grows up
                            auxGeom.Draw2dLabelCustom(
                            vRtpcPos.x - xOffset,
                                vRtpcPos.y,
                                fFontSize,
                                fRtpcTextColor,
                                eDrawText_Right,		// right-justified
                                "%s: %2.2f\n",
                                pRtpcName,
                                fRtpcValue);
                        }
                    }
                }

                if ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_OBJECT_ENVIRONMENTS) != 0)
                {
                    const float fEnvTextColor[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
                    Vec3 vEnvPos(vScreenPos);

                    for (auto& environment : m_cEnvironments)
                    {
                        const TAudioControlID nEnvID = environment.first;
                        const float fEnvValue = environment.second;

                        const char* const pEnvName = pDebugNameStore->LookupAudioEnvironmentName(nEnvID);

                        if (pEnvName)
                        {
                            const float xOffset = 5.f;

                            vEnvPos.y += fLineHeight;	// list grows down
                            auxGeom.Draw2dLabelCustom(
                                vEnvPos.x - xOffset,
                                vEnvPos.y,
                                fFontSize,
                                fEnvTextColor,
                                eDrawText_Right,		// right-justified
                                "%s: %.2f\n",
                                pEnvName,
                                fEnvValue);
                        }
                    }
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    size_t CATLAudioObject::CPropagationProcessor::s_nTotalSyncPhysRays = 0;
    size_t CATLAudioObject::CPropagationProcessor::s_nTotalAsyncPhysRays = 0;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::CPropagationProcessor::DrawObstructionRays(IRenderAuxGeom& auxGeom) const
    {
        static const ColorB cObstructedRayColor(200, 20, 1, 255);
        static const ColorB cFreeRayColor(20, 200, 1, 255);
        static const ColorB cIntersectionSphereColor(250, 200, 1, 240);
        static const float aObstructedRayLabelColor[4] = { 1.0f, 0.0f, 0.02f, 0.9f };
        static const float aFreeRayLabelColor[4] = { 0.0f, 1.0f, 0.02f, 0.9f };
        static const float fCollisionPtSphereRadius = 0.01f;

        if (CanRunObstructionOcclusion())
        {
            const SAuxGeomRenderFlags nPreviousRenderFlags = auxGeom.GetRenderFlags();
            SAuxGeomRenderFlags nNewRenderFlags(e_Def3DPublicRenderflags | e_AlphaBlended);
            nNewRenderFlags.SetCullMode(e_CullModeNone);

            for (size_t i = 0; i < m_nTotalRays; ++i)
            {
                const bool bRayObstructed = (m_vRayDebugInfos[i].nNumHits > 0);
                const Vec3 vRayEnd = bRayObstructed
                    ? m_vRayDebugInfos[i].vBegin + (m_vRayDebugInfos[i].vEnd - m_vRayDebugInfos[i].vBegin).GetNormalized() * m_vRayDebugInfos[i].fDistToNearestObstacle
                    : m_vRayDebugInfos[i].vEnd;// only draw the ray to the first collision point

                if ((g_audioCVars.m_nDrawAudioDebug & eADDF_DRAW_OBSTRUCTION_RAYS) != 0)
                {
                    const ColorB& rRayColor = bRayObstructed ? cObstructedRayColor : cFreeRayColor;

                    auxGeom.SetRenderFlags(nNewRenderFlags);

                    if (bRayObstructed)
                    {
                        // mark the nearest collision with a small sphere
                        auxGeom.DrawSphere(vRayEnd, fCollisionPtSphereRadius, cIntersectionSphereColor);
                    }

                    auxGeom.DrawLine(m_vRayDebugInfos[i].vBegin, rRayColor, vRayEnd, rRayColor, 1.0f);
                    auxGeom.SetRenderFlags(nPreviousRenderFlags);
                }

                if (IRenderer* pRenderer = (g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_OBSTRUCTION_RAY_LABELS) != 0 ? gEnv->pRenderer : nullptr)
                {
                    Vec3 vScreenPos(ZERO);

                    pRenderer->ProjectToScreen(m_vRayDebugInfos[i].vStableEnd.x, m_vRayDebugInfos[i].vStableEnd.y, m_vRayDebugInfos[i].vStableEnd.z, &vScreenPos.x, &vScreenPos.y, &vScreenPos.z);

                    vScreenPos.x = vScreenPos.x * 0.01f * pRenderer->GetWidth();
                    vScreenPos.y = vScreenPos.y * 0.01f * pRenderer->GetHeight();

                    if ((0.0f <= vScreenPos.z) && (vScreenPos.z <= 1.0f))
                    {
                        const float fColorLerp = clamp_tpl(m_vRayInfos[i].fAvgHits, 0.0f, 1.0f);
                        const float aLabelColor[4] =
                        {
                            aObstructedRayLabelColor[0] * fColorLerp + aFreeRayLabelColor[0] * (1.0f - fColorLerp),
                            aObstructedRayLabelColor[1] * fColorLerp + aFreeRayLabelColor[1] * (1.0f - fColorLerp),
                            aObstructedRayLabelColor[2] * fColorLerp + aFreeRayLabelColor[2] * (1.0f - fColorLerp),
                            aObstructedRayLabelColor[3] * fColorLerp + aFreeRayLabelColor[3] * (1.0f - fColorLerp)
                        };

                        auxGeom.Draw2dLabel(
                            vScreenPos.x,
                            vScreenPos.y - 12.0f,
                            1.2f,
                            aLabelColor,
                            true,
                            "ObjID: %llu\n#Hits: %2.1f\nOccl: %3.2f",
                            m_vRayInfos[i].nAudioObjectID, // a const member, will not be overwritten by a thread filling the obstruction data in
                            m_vRayDebugInfos[i].fAvgHits,
                            m_vRayDebugInfos[i].fOcclusionValue);
                    }
                }
            }
        }
    }

#endif // INCLUDE_AUDIO_PRODUCTION_CODE
} // namespace Audio
