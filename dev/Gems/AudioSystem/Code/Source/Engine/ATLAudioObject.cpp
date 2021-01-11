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

#include <ATLAudioObject.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/chrono/clocks.h>

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
    #include <AzCore/std/string/conversions.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

#include <MathConversion.h>

#include <AudioInternalInterfaces.h>
#include <SoundCVars.h>
#include <ATLUtils.h>

#include <IEntitySystem.h>
#include <I3DEngine.h>
#include <IRenderer.h>
#include <IRenderAuxGeom.h>

#if !AUDIO_ENABLE_CRY_PHYSICS
    #include <CryPhysicsDeprecation.h>
    #include <AzFramework/Physics/World.h>
#else
    #include <ISurfaceType.h>
#endif

namespace Audio
{
    extern CAudioLogger g_audioLogger;

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
    bool CATLAudioObjectBase::HasActiveEvents() const
    {
        for (const auto& triggerInstance : m_cTriggers)
        {
            if (triggerInstance.second.numPlayingEvents != 0)
            {
                return true;
            }
        }

        return false;
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


#if AUDIO_ENABLE_CRY_PHYSICS
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CPropagationProcessor::s_canIssueRWIs = false;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    int CPropagationProcessor::OnObstructionTest(const EventPhys* pEvent)
    {
        EventPhysRWIResult* const pRWIResult = (EventPhysRWIResult*)pEvent;

        if (pRWIResult->iForeignData == PHYS_FOREIGN_ID_SOUND_OBSTRUCTION)
        {
            auto const pRayInfo = static_cast<SRayInfo*>(pRWIResult->pForeignData);

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
    void CPropagationProcessor::ProcessObstructionRay(const int nNumHits, SRayInfo* const pRayInfo, const bool bReset)
    {
        float fTotalOcclusion = 0.0f;
        int nNumRealHits = 0;

    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        float fMinDistance = std::numeric_limits<float>::max();
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE

        if (nNumHits > 0)
        {
            ISurfaceTypeManager* const pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
            const size_t nCount = AZStd::GetMin(aznumeric_cast<size_t>(nNumHits + 1), s_maxObstructionRayHits);

            for (size_t i = 0; i < nCount; ++i)
            {
                const float fDist = pRayInfo->aHits[i].dist;

                if (fDist > 0.0f)
                {
                    ISurfaceType* const pMat = pSurfaceTypeManager->GetSurfaceType(pRayInfo->aHits[i].surface_idx);
                    if (pMat)
                    {
                        IPhysicalEntity* pCollider = pRayInfo->aHits[i].pCollider;
                        IEntity* pEntity = (pCollider && gEnv->pEntitySystem ? gEnv->pEntitySystem->GetEntityFromPhysics(pCollider) : nullptr);

                        const ISurfaceType::SPhysicalParams& physParams = pMat->GetPhyscalParams();
                        fTotalOcclusion += physParams.sound_obstruction * (pEntity ? pEntity->GetObstructionMultiplier() : 1.f);
                        ++nNumRealHits;

                    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                        fMinDistance = AZStd::GetMin(fMinDistance, fDist);
                    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
                    }
                }
            }
        }

        fTotalOcclusion = AZ::GetClamp(fTotalOcclusion, 0.0f, 1.0f);

        // If the num hits differs too much from the last ray, reduce the change in TotalOcclusion in inverse proportion.
        // This reduces thrashing at the boundaries of occluding entities.
        const int nHitDiff = nNumRealHits - pRayInfo->nNumHits;
        const int nAbsHitDiff = AZ::GetMax(nHitDiff, -nHitDiff);
        const float fNumHitCorrection = nAbsHitDiff > 1 ? 1.0f / nAbsHitDiff : 1.0f;

        pRayInfo->nNumHits = nNumRealHits;

    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        pRayInfo->m_debugInfo.fDistToNearestObstacle = fMinDistance;
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE

        if (bReset)
        {
            pRayInfo->fTotalSoundOcclusion = fTotalOcclusion;

        #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
            pRayInfo->m_debugInfo.fAvgHits = static_cast<float>(pRayInfo->nNumHits);
        #endif // INCLUDE_AUDIO_PRODUCTION_CODE
        }
        else
        {
            pRayInfo->fTotalSoundOcclusion += fNumHitCorrection * (fTotalOcclusion - pRayInfo->fTotalSoundOcclusion) * SRayInfo::s_fSmoothingAlpha;

        #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
            pRayInfo->m_debugInfo.fAvgHits += (pRayInfo->nNumHits - pRayInfo->m_debugInfo.fAvgHits) * SRayInfo::s_fSmoothingAlpha;
        #endif // INCLUDE_AUDIO_PRODUCTION_CODE
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    size_t CPropagationProcessor::NumRaysFromCalcType(const EAudioObjectObstructionCalcType eCalcType)
    {
        switch (eCalcType)
        {
        case eAOOCT_IGNORE:
            return 0;
        case eAOOCT_SINGLE_RAY:
            return 1;
        case eAOOCT_MULTI_RAY:
            return 5;
        default:
            return 0;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CPropagationProcessor::CPropagationProcessor(
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
        , m_eObstOcclCalcType(eAOOCT_IGNORE)
        , m_pendingRaysReleased(false)

    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        , m_fTimeSinceLastUpdateMS(0.0f)
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
    {
        m_vRayInfos.reserve(s_maxObstructionRays);

        for (size_t i = 0; i < s_maxObstructionRays; ++i)
        {
            m_vRayInfos.emplace_back(i, nObjectID);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CPropagationProcessor::~CPropagationProcessor()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CPropagationProcessor::Update(const float fUpdateMS)
    {
        m_oObstructionValue.Update(fUpdateMS);
        m_oOcclusionValue.Update(fUpdateMS);

    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        m_fTimeSinceLastUpdateMS += fUpdateMS;
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CPropagationProcessor::SetObstructionOcclusionCalcType(const EAudioObjectObstructionCalcType eObstOcclCalcType)
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
    void CPropagationProcessor::GetPropagationData(SATLSoundPropagationData& rPropagationData) const
    {
        rPropagationData.fObstruction = m_oObstructionValue.GetCurrent();
        rPropagationData.fOcclusion = m_oOcclusionValue.GetCurrent();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CPropagationProcessor::RunObstructionQuery(
        const SATLWorldPosition& rListenerPosition,
        const bool bSyncCall,
        const bool bReset)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Audio);

        if (m_nRemainingRays == 0)
        {
            static const AZ::Vector3 vUp(0.0f, 0.0f, 1.0f);
            static const float fMinPeripheralRayOffset = 0.3f;
            static const float fMaxPeripheralRayOffset = 1.0f;
            static const float fMinRandomOffset = 0.05f;
            static const float fMaxRandomOffset = 0.5f;
            static const float fMinOffsetDistance = 1.0f;
            static const float fMaxOffsetDistance = 20.0f;

            // the previous query has already been processed
            const AZ::Vector3& vListener = rListenerPosition.GetPositionVec();
            const AZ::Vector3& vObject = m_rPosition.GetPositionVec();
            const AZ::Vector3 vDiff = vObject - vListener;

            const float fDistance = vDiff.GetLength();

            m_fCurrListenerDist = fDistance;

            m_nTotalRays = NumRaysFromCalcType(m_eObstOcclCalcType);

            const float fOffsetParam = AZ::GetClamp((fDistance - fMinOffsetDistance) / (fMaxOffsetDistance - fMinOffsetDistance), 0.0f, 1.0f);
            const float fOffsetScale = fMaxPeripheralRayOffset * fOffsetParam + fMinPeripheralRayOffset * (1.0f - fOffsetParam);
            const float fRandOffsetScale = fMaxRandomOffset * fOffsetParam + fMinRandomOffset * (1.0f - fOffsetParam);

            const AZ::Vector3 vSide = vDiff.GetNormalized().Cross(vUp);

            // the 0th ray is always shot from the listener to the center of the source
            // the 0th ray only gets 1/2 of the random variation
            CastObstructionRay(
                vListener,
                (vUp * frand_symm() + vSide * frand_symm()) * fRandOffsetScale * 0.5f,
                vDiff,
                0,
                bSyncCall,
                bReset);

            if (m_nTotalRays > 1)
            {
                // rays 1 and 2 start below and above the listener and go towards the source
                CastObstructionRay(
                    vListener - (vUp * fOffsetScale),
                    (vUp * frand_symm() + vSide * frand_symm()) * fRandOffsetScale,
                    vDiff,
                    1,
                    bSyncCall,
                    bReset);

                CastObstructionRay(
                    vListener + (vUp * fOffsetScale),
                    (vUp * frand_symm() + vSide * frand_symm()) * fRandOffsetScale,
                    vDiff,
                    2,
                    bSyncCall,
                    bReset);

                // rays 3 and 4 start left and right of the listener and go towards the source
                CastObstructionRay(
                    vListener - (vSide * fOffsetScale),
                    (vUp * frand_symm() + vSide * frand_symm()) * fRandOffsetScale,
                    vDiff,
                    3,
                    bSyncCall,
                    bReset);

                CastObstructionRay(
                    vListener + (vSide * fOffsetScale),
                    (vUp * frand_symm() + vSide * frand_symm()) * fRandOffsetScale,
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
    void CPropagationProcessor::ReportRayProcessed(const size_t nRayID)
    {
        // make the function to be more tolerant of results when we've released the pending rays.
        if (m_pendingRaysReleased)
        {
            return;
        }

        AZ_Assert(m_nRemainingRays > 0, "ReportRayProcessed - There are no more remaining rays!"); // make sure there are rays remaining
        AZ_Assert((0 <= nRayID) && (nRayID <= m_nTotalRays), "ReportRayProcessed - The passed RayID is invalid!"); // check RayID validity

    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        if (m_nRemainingRays == 0 || m_rRefCounter == 0)
        {
            g_audioLogger.Log(eALT_ASSERT, "Negative ref or ray count on audio object %u", m_vRayInfos[nRayID].nAudioObjectID);
        }
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE

        if (--m_nRemainingRays == 0)
        {
            ProcessObstructionOcclusion();
            --m_rRefCounter;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CPropagationProcessor::ReleasePendingRays()
    {
        if (m_nRemainingRays > 0)
        {
            m_nRemainingRays = 0;
            --m_rRefCounter;
            m_pendingRaysReleased = true;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CPropagationProcessor::ProcessObstructionOcclusion(const bool bReset)
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
                fObstruction *= AZ::GetMin(g_audioCVars.m_fFullObstructionMaxDistance / m_fCurrListenerDist, 1.0f);

                // since the Obstruction filter is applied on top of Occlusion, make sure we only apply what's exceeding the occlusion value
                fObstruction = AZ::GetMax(fObstruction - fOcclusion, 0.0f);
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
            // Update debug data...
            if (m_fTimeSinceLastUpdateMS > 100.0f) // only re-sample the rays about 10 times per second for a smoother debug drawing
            {
                m_fTimeSinceLastUpdateMS = 0.0f;

                for (size_t rayIndex = 0; rayIndex < m_nTotalRays; ++rayIndex)
                {
                    SRayDebugInfo& rayDebugInfo = m_vRayInfos[rayIndex].m_debugInfo;

                    rayDebugInfo.vBegin = rayDebugInfo.vStartPosition + rayDebugInfo.vRndOffset;
                    rayDebugInfo.vEnd = rayDebugInfo.vBegin + rayDebugInfo.vDirection;

                    if (rayDebugInfo.vStableEnd.IsClose(AZ::Vector3::CreateZero()))
                    {
                        rayDebugInfo.vStableEnd = rayDebugInfo.vEnd;
                    }
                    else
                    {
                        rayDebugInfo.vStableEnd += (rayDebugInfo.vEnd - rayDebugInfo.vStableEnd) * 0.1f;
                    }

                    rayDebugInfo.fOcclusionValue = m_vRayInfos[rayIndex].fTotalSoundOcclusion;
                }
            }
        #endif // INCLUDE_AUDIO_PRODUCTION_CODE
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CPropagationProcessor::CastObstructionRay(
        const AZ::Vector3& rOrigin,
        const AZ::Vector3& rRndOffset,
        const AZ::Vector3& rDirection,
        const size_t nRayIdx,
        const bool bSyncCall,
        const bool bReset)
    {
        AZ_Assert(nRayIdx < s_maxObstructionRays, "[Audio] CastObstructionRay - Ray Index (%zu) is out of bounds!\n", nRayIdx);
        SRayInfo& rRayInfo = m_vRayInfos[nRayIdx];

        static const int nPhysicsFlags = (ent_water | ent_static | ent_sleeping_rigid | ent_rigid | ent_terrain);
        const int nNumHits = gEnv->pPhysicalWorld->RayWorldIntersection(
            AZVec3ToLYVec3(rOrigin + rRndOffset),
            AZVec3ToLYVec3(rDirection),
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
        // Set the debug data about the raycast...
        rRayInfo.m_debugInfo.vStartPosition = rOrigin;
        rRayInfo.m_debugInfo.vDirection = rDirection;
        rRayInfo.m_debugInfo.vRndOffset = rRndOffset;

        rRayInfo.m_debugInfo.vBegin = rRayInfo.m_debugInfo.vStartPosition + rRayInfo.m_debugInfo.vRndOffset;
        rRayInfo.m_debugInfo.vEnd = rRayInfo.m_debugInfo.vBegin + rRayInfo.m_debugInfo.vDirection;
        rRayInfo.m_debugInfo.vStableEnd = rRayInfo.m_debugInfo.vEnd;

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

#endif // AUDIO_ENABLE_CRY_PHYSICS

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::Clear()
    {
        CATLAudioObjectBase::Clear();
        m_oPosition = SATLWorldPosition();
#if !AUDIO_ENABLE_CRY_PHYSICS
        m_raycastProcessor.Reset();
#endif // !AUDIO_ENABLE_CRY_PHYSICS
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::Update(const float fUpdateIntervalMS, const SATLWorldPosition& rListenerPosition)
    {
        CATLAudioObjectBase::Update(fUpdateIntervalMS, rListenerPosition);

#if AUDIO_ENABLE_CRY_PHYSICS
        m_oPropagationProcessor.Update(fUpdateIntervalMS);

        if (CanRunObstructionOcclusion())
        {
            const float fDistance = (m_oPosition.GetPositionVec() - rListenerPosition.GetPositionVec()).GetLength();

            AZ_WarningOnce("Audio Raycast", CPropagationProcessor::s_minObstructionDistance < g_audioCVars.m_fOcclusionMaxDistance,
                "The 's_OcclusionMaxDistance' CVar is set too low (%.2f).  Audio raycasts for occlusion are disabled from running!\n",
                g_audioCVars.m_fOcclusionMaxDistance);

            if ((CPropagationProcessor::s_minObstructionDistance < fDistance) && (fDistance < g_audioCVars.m_fOcclusionMaxDistance))
            {
                // make the physics ray cast call sync or async depending on the distance to the listener
                // LY-64528 - disable synchronous raycasts from audio thread, they can cause crashes.
                const bool bSyncCall = false;   // = (fDistance <= g_audioCVars.m_fOcclusionMaxSyncDistance);
                m_oPropagationProcessor.RunObstructionQuery(rListenerPosition, bSyncCall);
            }
        }
#else
        if (CanRunRaycasts())
        {
            m_raycastProcessor.Update(fUpdateIntervalMS);
            m_raycastProcessor.Run(rListenerPosition);
        }
#endif // AUDIO_ENABLE_CRY_PHYSICS
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::SetPosition(const SATLWorldPosition& oNewPosition)
    {
        m_oPosition = oNewPosition;
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

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::UpdateVelocity(float const fUpdateIntervalMS)
    {
        const AZ::Vector3 cPositionDelta = m_oPosition.GetPositionVec() - m_oPreviousPosition.GetPositionVec();
        const float fCurrentVelocity = (1000.0f * cPositionDelta.GetLength()) / fUpdateIntervalMS; // fCurrentVelocity is given in units per second

        if (AZ::GetAbs(fCurrentVelocity - m_fPreviousVelocity) > g_audioCVars.m_fVelocityTrackingThreshold)
        {
            m_fPreviousVelocity = fCurrentVelocity;
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_SET_RTPC_VALUE> oRequestData(ATLInternalControlIDs::ObjectSpeedRtpcID, fCurrentVelocity);

            oRequest.nAudioObjectID = GetID();
            oRequest.nFlags = eARF_THREAD_SAFE_PUSH;
            oRequest.pData = &oRequestData;
            AudioSystemThreadSafeRequestBus::Broadcast(&AudioSystemThreadSafeRequestBus::Events::PushRequestThreadSafe, oRequest);
        }

        m_oPreviousPosition = m_oPosition;
    }

#if AUDIO_ENABLE_CRY_PHYSICS
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::ReportPhysicsRayProcessed(const size_t nRayID)
    {
        m_oPropagationProcessor.ReportRayProcessed(nRayID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::SetObstructionOcclusionCalc(const EAudioObjectObstructionCalcType ePassedOOCalcType)
    {
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

#else // !AUDIO_ENABLE_CRY_PHYSICS

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::SetRaycastCalcType(const EAudioObjectObstructionCalcType calcType)
    {
        m_raycastProcessor.SetType(calcType);
        switch (calcType)
        {
        case eAOOCT_IGNORE:
            AudioRaycastNotificationBus::Handler::BusDisconnect();
            break;
        case eAOOCT_SINGLE_RAY:
            [[fallthrough]];
        case eAOOCT_MULTI_RAY:
            AudioRaycastNotificationBus::Handler::BusConnect(GetID());
            break;
        default:
            break;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::RunRaycasts(const SATLWorldPosition& listenerPos)
    {
        m_raycastProcessor.Run(listenerPos);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLAudioObject::CanRunRaycasts() const
    {
        return Audio::s_EnableRaycasts      // This is the CVar to enable/disable audio raycasts.
            && Audio::s_RaycastMinDistance < Audio::s_RaycastMaxDistance
            && m_raycastProcessor.CanRun();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::GetObstOccData(SATLSoundPropagationData& data) const
    {
        data.fObstruction = m_raycastProcessor.GetObstruction();
        data.fOcclusion = m_raycastProcessor.GetOcclusion();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::OnAudioRaycastResults(const AudioRaycastResult& result)
    {
        // Pull in the results to the raycast processor...
        AZ_Assert(result.m_audioObjectId != INVALID_AUDIO_OBJECT_ID, "Audio Raycast Results - audio object id is invalid!\n");
        AZ_Assert(result.m_rayIndex < s_maxRaysPerObject, "Audio Raycast Results - ray index is out of bounds (index: %zu)!\n", result.m_rayIndex);
        AZ_Assert(result.m_result.size() <= s_maxHitResultsPerRaycast, "Audio Raycast Results - too many hits returned (hits: %zu)!\n", result.m_result.size());

        RaycastInfo& info = m_raycastProcessor.m_rayInfos[result.m_rayIndex];
        if (!info.m_pending)
        {
            // This may mean that an audio object was recycled (reset) and then reused.
            // Need to investigate this further.
            return;
        }

        info.m_pending = false;
        info.m_hits.clear();
        info.m_numHits = 0;

        for (auto& hit : result.m_result)
        {
            if (hit.m_distance > 0.f)
            {
                info.m_hits.push_back(hit);
                info.m_numHits++;
            }
        }

        info.UpdateContribution();
        info.m_cached = true;
        info.m_cacheTimerMs = Audio::s_RaycastCacheTimeMs;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void RaycastInfo::UpdateContribution()
    {
        // This calculates the contribution of a single raycast.  This calculation can be updated
        // as needed to suit a user's needs.  This is provided as a first example.
        // Based on the number of hits reported, add values from the sequence: 1/2 + 1/4 + 1/8 + ...
        float newContribution = 0.f;

        for (AZ::u16 hit = 0; hit < m_numHits; ++hit)
        {
            newContribution += std::pow(2.f, -aznumeric_cast<float>(hit + 1));
        }

        m_contribution = newContribution;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    float RaycastInfo::GetDistanceScaledContribution() const
    {
        // Max extent is the s_RaycastMaxDistance, and use the distance embedded in the raycast request as a percent (inverse).
        // Objects closer to the listener will have greater contribution amounts.
        // Objects farther away will contribute less obstruction/occlusion, but distance attenuation will be the larger contributing factor.
        const float maxDistance = static_cast<float>(s_RaycastMaxDistance);
        float clampedDistance = AZ::GetClamp(m_raycastRequest.m_distance, 0.f, maxDistance);
        float distanceScale = 1.f - (clampedDistance / maxDistance);

        // Scale the contribution amount by (inverse) distance.
        return distanceScale * m_contribution;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    float RaycastInfo::GetNearestHitDistance() const
    {
        float minDistance = m_raycastRequest.m_distance;
        for (const auto& hit : m_hits)
        {
            minDistance = AZ::GetMin(minDistance, hit.m_distance);
        }

        return minDistance;
    }


    // static
    bool RaycastProcessor::s_raycastsEnabled = false;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    RaycastProcessor::RaycastProcessor(const TAudioObjectID objectId, const SATLWorldPosition& objectPosition)
        : m_rayInfos(s_maxRaysPerObject, RaycastInfo())
        , m_position(objectPosition)
        , m_obstructionValue(s_RaycastSmoothFactor, s_epsilon)
        , m_occlusionValue(s_RaycastSmoothFactor, s_epsilon)
        , m_audioObjectId(objectId)
        , m_obstOccType(eAOOCT_IGNORE)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void RaycastProcessor::Update(float elapsedMs)
    {
        if (m_obstOccType == eAOOCT_SINGLE_RAY || m_obstOccType == eAOOCT_MULTI_RAY)
        {
            // First ray is direct-path obstruction value...
            m_obstructionValue.SetNewTarget(m_rayInfos[0].GetDistanceScaledContribution());

            if (m_obstOccType == eAOOCT_MULTI_RAY)
            {
                float occlusion = 0.f;
                for (size_t i = 1; i < s_maxRaysPerObject; ++i)
                {
                    occlusion += m_rayInfos[i].GetDistanceScaledContribution();
                }

                // Average of the occlusion rays' contributions...
                occlusion /= aznumeric_cast<float>(s_maxRaysPerObject - 1);

                m_occlusionValue.SetNewTarget(occlusion);
            }

            // Tick down the cache timers, when expired mark them dirty...
            for (auto& rayInfo : m_rayInfos)
            {
                if (rayInfo.m_cached)
                {
                    rayInfo.m_cacheTimerMs -= elapsedMs;
                    rayInfo.m_cached = (rayInfo.m_cacheTimerMs > 0.f);
                }
            }
        }

        m_obstructionValue.Update(s_RaycastSmoothFactor);
        m_occlusionValue.Update(s_RaycastSmoothFactor);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void RaycastProcessor::Reset()
    {
        m_obstructionValue.Reset();
        m_occlusionValue.Reset();

        for (auto& rayInfo : m_rayInfos)
        {
            rayInfo.Reset();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void RaycastProcessor::SetType(EAudioObjectObstructionCalcType calcType)
    {
        if (calcType == m_obstOccType)
        {
            // No change to type, no need to reset any data.
            return;
        }

        if (calcType == eAOOCT_IGNORE)
        {
            // Reset the target values when turning off raycasts (set to IGNORE).
            m_obstructionValue.Reset();
            m_occlusionValue.Reset();
        }

        // Otherwise, switching to a new type we can allow the obst/occ values from before to smooth
        // to new targets as they become available.  Hence no reset.

        for (auto& rayInfo : m_rayInfos)
        {
            rayInfo.Reset();
        }

        m_obstOccType = calcType;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool RaycastProcessor::CanRun() const
    {
        return s_raycastsEnabled    // This enable/disable is set via ISystem events.
            && m_obstOccType != eAOOCT_IGNORE;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void RaycastProcessor::Run(const SATLWorldPosition& listenerPosition)
    {
        const AZ::Vector3 listener = listenerPosition.GetPositionVec();
        const AZ::Vector3 source = m_position.GetPositionVec();
        const AZ::Vector3 ray = source - listener;

        const float distance = ray.GetLength();

        // Prevent raycast when individual sources are not within the allowed distance range...
        if (Audio::s_RaycastMinDistance >= distance || distance >= Audio::s_RaycastMaxDistance)
        {
            Reset();
            return;
        }

        const AZ::Vector3 up = AZ::Vector3::CreateAxisZ();
        const AZ::Vector3 side = ray.GetNormalized().Cross(up);

        // Spread out the side rays based on the percentage the ray distance is of the maximum distance.
        // The begin of the rays spread by [0.f, 1.f] in the side direction.
        // The end of the rays spread by [1.f, 10.f] in the side direction.
        constexpr float spreadDistanceMinExtent = 1.f;
        constexpr float spreadDistanceMaxExtent = 10.f;
        constexpr float spreadDistanceDelta = spreadDistanceMaxExtent - spreadDistanceMinExtent;

        const float rayDistancePercent = (distance / Audio::s_RaycastMaxDistance);

        const float spreadDist = spreadDistanceMinExtent + rayDistancePercent * spreadDistanceDelta;

        // Cast ray 0, the direct obstruction ray.
        CastRay(listener, source, 0);

        if (m_obstOccType == eAOOCT_MULTI_RAY)
        {
            // Cast ray 1, an indirect occlusion ray.
            CastRay(listener, source + up, 1);

            // Cast ray 2, an indirect occlusion ray.
            CastRay(listener, source - up, 2);

            // Cast ray 3, an indirect occlusion ray.
            CastRay(listener + side * rayDistancePercent, source + side * spreadDist, 3);

            // Cast ray 4, an indirect occlusion ray.
            CastRay(listener - side * rayDistancePercent, source - side * spreadDist, 4);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void RaycastProcessor::CastRay(const AZ::Vector3& origin, const AZ::Vector3& dest, const AZ::u16 rayIndex)
    {
        AZ_Assert(rayIndex < s_maxRaysPerObject, "RaycastProcessor::CastRay - ray index is out of bounds!\n");

        RaycastInfo& rayInfo = m_rayInfos[rayIndex];
        if (rayInfo.m_pending || rayInfo.m_cached)
        {
            // A raycast is already in flight OR
            // A raycast result was received recently and is still considered valid.
            return;
        }

        rayInfo.m_raycastRequest.m_start = origin;
        rayInfo.m_raycastRequest.m_direction = dest - origin;
        rayInfo.m_raycastRequest.m_distance = rayInfo.m_raycastRequest.m_direction.NormalizeSafeWithLength();
        rayInfo.m_raycastRequest.m_maxResults = s_maxHitResultsPerRaycast;

        // Mark as pending
        rayInfo.m_pending = true;

        AudioRaycastRequest request(rayInfo.m_raycastRequest, m_audioObjectId, rayIndex);
        AudioRaycastRequestBus::Broadcast(&AudioRaycastRequestBus::Events::PushAudioRaycastRequest, request);
    }

#if defined(AZ_TESTS_ENABLED)
    void RaycastProcessor::SetupTestRay(AZ::u16 rayIndex)
    {
        if (rayIndex < s_maxRaysPerObject)
        {
            // Set the pending flag to true, so the results aren't discarded.
            m_rayInfos[rayIndex].m_pending = true;
            // Set the distance in the request structure so it doesn't have the default.
            m_rayInfos[rayIndex].m_raycastRequest.m_distance = (s_RaycastMaxDistance / 4.f);
        }
    }
#endif // AZ_TESTS_ENABLED

#endif // AUDIO_ENABLE_CRY_PHYSICS


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
    AZStd::string CATLAudioObjectBase::GetTriggerNames(const char* const sSeparator, const CATLDebugNameStore* const pDebugNameStore)
    {
        AZStd::string triggersString;

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
                    triggersString = AZStd::string::format("%s%s%s", triggersString.c_str(), pName, sSeparator);
                }
                else
                {
                    triggersString = AZStd::string::format("%s%s(%zu inst.)%s", triggersString.c_str(), pName, nInstances, sSeparator);
                }
            }
        }

        return triggersString;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string CATLAudioObjectBase::GetEventIDs(const char* const sSeparator)
    {
        AZStd::string eventsString;
        for (auto activeEvent : m_cActiveEvents)
        {
            eventsString = AZStd::string::format("%s%" PRIu64 "%s", eventsString.c_str(), activeEvent, sSeparator);
        }

        return eventsString;
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
    void CATLAudioObject::DrawDebugInfo(IRenderAuxGeom& auxGeom, const AZ::Vector3& vListenerPos, const CATLDebugNameStore* const pDebugNameStore) const
    {
#if AUDIO_ENABLE_CRY_PHYSICS
        m_oPropagationProcessor.DrawObstructionRays(auxGeom);
#else
        m_raycastProcessor.DrawObstructionRays(auxGeom);
#endif // AUDIO_ENABLE_CRY_PHYSICS

        if (!m_cTriggers.empty())
        {
            // Inspect triggers and apply filter (if set)...
            TTriggerCountMap cTriggerCounts;

            AZStd::string triggerFilter(g_audioCVars.m_pAudioTriggersDebugFilter->GetString());
            AZStd::to_lower(triggerFilter.begin(), triggerFilter.end());

            for (auto& trigger : m_cTriggers)
            {
                AZStd::string triggerName(pDebugNameStore->LookupAudioTriggerName(trigger.second.nTriggerID));
                AZStd::to_lower(triggerName.begin(), triggerName.end());

                if (AudioDebugDrawFilter(triggerName, triggerFilter))
                {
                    ++cTriggerCounts[trigger.second.nTriggerID];
                }
            }

            // Early out for this object if all trigger names were filtered out.
            if (cTriggerCounts.empty())
            {
                return;
            }

            const AZ::Vector3 vPos(m_oPosition.GetPositionVec());
            AZ::Vector3 vScreenPos(0.f);

            if (IRenderer* pRenderer = gEnv->pRenderer)
            {
                float screenProj[3];
                pRenderer->ProjectToScreen(vPos.GetX(), vPos.GetY(), vPos.GetZ(), &screenProj[0], &screenProj[1], &screenProj[2]);

                screenProj[0] *= 0.01f * static_cast<float>(pRenderer->GetWidth());
                screenProj[1] *= 0.01f * static_cast<float>(pRenderer->GetHeight());
                vScreenPos.Set(screenProj);
            }
            else
            {
                vScreenPos.SetZ(-1.0f);
            }

            if ((0.0f <= vScreenPos.GetZ()) && (vScreenPos.GetZ() <= 1.0f))
            {
                const float fDist = vPos.GetDistance(vListenerPos);

                if ((g_audioCVars.m_nDrawAudioDebug & eADDF_DRAW_SPHERES) != 0)
                {
                    const SAuxGeomRenderFlags nPreviousRenderFlags = auxGeom.GetRenderFlags();
                    SAuxGeomRenderFlags nNewRenderFlags(e_Def3DPublicRenderflags | e_AlphaBlended);
                    nNewRenderFlags.SetCullMode(e_CullModeNone);
                    auxGeom.SetRenderFlags(nNewRenderFlags);
                    const float fRadius = 0.15f;
                    const AZ::Color sphereColor(1.f, 0.1f, 0.1f, 1.f);
                    auxGeom.DrawSphere(AZVec3ToLYVec3(vPos), fRadius, AZColorToLYColorB(sphereColor));
                    auxGeom.SetRenderFlags(nPreviousRenderFlags);
                }

                const float fFontSize = 1.3f;
                const float fLineHeight = 12.0f;

                if ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_OBJECT_STATES) != 0)
                {
                    AZ::Vector3 vSwitchPos(vScreenPos);

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
                            const AZ::Color switchTextColor(0.8f, 0.8f, 0.8f, oDrawData.fCurrentAlpha);

                            vSwitchPos -= AZ::Vector3(0.f, fLineHeight, 0.f);
                            auxGeom.Draw2dLabel(
                                vSwitchPos.GetX(),
                                vSwitchPos.GetY(),
                                fFontSize,
                                AZColorToLYColorF(switchTextColor),
                                false,
                                "%s: %s\n",
                                pSwitchName,
                                pStateName);
                        }
                    }
                }

                const AZ::Color brightTextColor(0.9f, 0.9f, 0.9f, 1.f);
                const AZ::Color normalTextColor(0.75f, 0.75f, 0.75f, 1.f);
                const AZ::Color dimmedTextColor(0.5f, 0.5f, 0.5f, 1.f);

                if ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_OBJECT_LABEL) != 0)
                {
                    const TAudioObjectID nObjectID = GetID();
                    auxGeom.Draw2dLabel(
                        vScreenPos.GetX(),
                        vScreenPos.GetY(),
                        fFontSize,
                        AZColorToLYColorF(brightTextColor),
                        false,
                        "%s  ID: %llu  RefCnt: %2zu  Dist: %4.1fm",
                        pDebugNameStore->LookupAudioObjectName(nObjectID),
                        nObjectID,
                        GetRefCount(),
                        fDist);

#if AUDIO_ENABLE_CRY_PHYSICS
                    const size_t nNumRays = m_oPropagationProcessor.GetNumRays();
                    SATLSoundPropagationData oPropagationData;
                    m_oPropagationProcessor.GetPropagationData(oPropagationData);

                    auxGeom.Draw2dLabel(
                        vScreenPos.GetX(),
                        vScreenPos.GetY() + fLineHeight,
                        fFontSize,
                        AZColorToLYColorF(nNumRays > 0 ? brightTextColor : dimmedTextColor),
                        false,
                        "Obst: %3.2f  Occl: %3.2f  #Rays: %1zu",
                        oPropagationData.fObstruction,
                        oPropagationData.fOcclusion,
                        nNumRays);

#else
                    SATLSoundPropagationData obstOccData;
                    GetObstOccData(obstOccData);

                    auxGeom.Draw2dLabel(
                        vScreenPos.GetX(),
                        vScreenPos.GetY() + fLineHeight,
                        fFontSize,
                        AZColorToLYColorF(brightTextColor),
                        false,
                        "Obst: %.3f  Occl: %.3f",
                        obstOccData.fObstruction,
                        obstOccData.fOcclusion
                    );
#endif // AUDIO_ENABLE_CRY_PHYSICS
                }

                if ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_OBJECT_TRIGGERS) != 0)
                {
                    AZStd::string triggerStringFormatted;

                    for (auto& triggerCount : cTriggerCounts)
                    {
                        const char* const pName = pDebugNameStore->LookupAudioTriggerName(triggerCount.first);
                        if (pName)
                        {
                            const size_t nInstances = triggerCount.second;
                            if (nInstances == 1)
                            {
                                triggerStringFormatted += pName;
                                triggerStringFormatted += "\n";
                            }
                            else
                            {
                                triggerStringFormatted += AZStd::string::format("%s: %zu\n", pName, nInstances);
                            }
                        }
                    }

                    auxGeom.Draw2dLabel(
                        vScreenPos.GetX(),
                        vScreenPos.GetY() + (2.0f * fLineHeight),
                        fFontSize,
                        AZColorToLYColorF(normalTextColor),
                        false,
                        "%s",
                        triggerStringFormatted.c_str());
                }

                if ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_OBJECT_RTPCS) != 0)
                {
                    AZ::Vector3 vRtpcPos(vScreenPos);

                    for (auto& rtpc : m_cRtpcs)
                    {
                        const float fRtpcValue = rtpc.second;
                        const char* const pRtpcName = pDebugNameStore->LookupAudioRtpcName(rtpc.first);

                        if (pRtpcName)
                        {
                            const float xOffset = 5.f;

                            vRtpcPos -= AZ::Vector3(0.f, fLineHeight, 0.f);     // list grows up
                            auxGeom.Draw2dLabelCustom(
                                vRtpcPos.GetX() - xOffset,
                                vRtpcPos.GetY(),
                                fFontSize,
                                AZColorToLYColorF(normalTextColor),
                                eDrawText_Right,        // right-justified
                                "%s: %2.2f\n",
                                pRtpcName,
                                fRtpcValue);
                        }
                    }
                }

                if ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_OBJECT_ENVIRONMENTS) != 0)
                {
                    AZ::Vector3 vEnvPos(vScreenPos);

                    for (auto& environment : m_cEnvironments)
                    {
                        const float fEnvValue = environment.second;
                        const char* const pEnvName = pDebugNameStore->LookupAudioEnvironmentName(environment.first);

                        if (pEnvName)
                        {
                            const float xOffset = 5.f;

                            vEnvPos += AZ::Vector3(0.f, fLineHeight, 0.f);      // list grows down
                            auxGeom.Draw2dLabelCustom(
                                vEnvPos.GetX() - xOffset,
                                vEnvPos.GetY(),
                                fFontSize,
                                AZColorToLYColorF(normalTextColor),
                                eDrawText_Right,        // right-justified
                                "%s: %.2f\n",
                                pEnvName,
                                fEnvValue);
                        }
                    }
                }
            }
        }
    }

#if AUDIO_ENABLE_CRY_PHYSICS
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    size_t CPropagationProcessor::s_nTotalSyncPhysRays = 0;
    size_t CPropagationProcessor::s_nTotalAsyncPhysRays = 0;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CPropagationProcessor::DrawObstructionRays(IRenderAuxGeom& auxGeom) const
    {
        static const AZ::Color cObstructedRayColor(0.8f, 0.08f, 0.f, 1.f);
        static const AZ::Color cFreeRayColor(0.08f, 0.8f, 0.f, 1.f);
        static const AZ::Color cIntersectionSphereColor(0.9f, 0.8f, 0.f, 0.9f);
        static const AZ::Color cObstructedRayLabelColor(1.f, 0.f, 0.02f, 0.9f);
        static const AZ::Color cFreeRayLabelColor(0.f, 1.f, 0.02f, 0.9f);

        static const float fCollisionPtSphereRadius = 0.01f;

        if (CanRunObstructionOcclusion())
        {
            const SAuxGeomRenderFlags nPreviousRenderFlags = auxGeom.GetRenderFlags();
            SAuxGeomRenderFlags nNewRenderFlags(e_Def3DPublicRenderflags | e_AlphaBlended);
            nNewRenderFlags.SetCullMode(e_CullModeNone);

            for (size_t i = 0; i < m_nTotalRays; ++i)
            {
                const SRayInfo& rayInfo = m_vRayInfos[i];
                const SRayDebugInfo& rayDebugInfo = rayInfo.m_debugInfo;

                const bool bRayObstructed = (rayInfo.nNumHits > 0);
                const AZ::Vector3 vRayEnd = bRayObstructed
                    ? rayDebugInfo.vBegin + (rayDebugInfo.vEnd - rayDebugInfo.vBegin).GetNormalized() * rayDebugInfo.fDistToNearestObstacle
                    : rayDebugInfo.vEnd;    // only draw the ray to the first collision point if obstructed

                if ((g_audioCVars.m_nDrawAudioDebug & eADDF_DRAW_OBSTRUCTION_RAYS) != 0)
                {
                    const AZ::Color& rRayColor = bRayObstructed ? cObstructedRayColor : cFreeRayColor;

                    auxGeom.SetRenderFlags(nNewRenderFlags);

                    if (bRayObstructed)
                    {
                        // mark the nearest collision with a small sphere
                        auxGeom.DrawSphere(AZVec3ToLYVec3(vRayEnd), fCollisionPtSphereRadius, AZColorToLYColorB(cIntersectionSphereColor));
                    }

                    auxGeom.DrawLine(AZVec3ToLYVec3(rayDebugInfo.vBegin), AZColorToLYColorB(rRayColor), AZVec3ToLYVec3(vRayEnd), AZColorToLYColorB(rRayColor), 1.0f);
                    auxGeom.SetRenderFlags(nPreviousRenderFlags);
                }

                if (IRenderer* pRenderer = (g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_OBSTRUCTION_RAY_LABELS) != 0 ? gEnv->pRenderer : nullptr)
                {
                    float screenProj[3];
                    pRenderer->ProjectToScreen(rayDebugInfo.vStableEnd.GetX(), rayDebugInfo.vStableEnd.GetY(), rayDebugInfo.vStableEnd.GetZ(), &screenProj[0], &screenProj[1], &screenProj[2]);

                    screenProj[0] *= 0.01f * static_cast<float>(pRenderer->GetWidth());
                    screenProj[1] *= 0.01f * static_cast<float>(pRenderer->GetHeight());
                    AZ::Vector3 vScreenPos = AZ::Vector3::CreateFromFloat3(screenProj);

                    if ((0.0f <= vScreenPos.GetZ()) && (vScreenPos.GetZ() <= 1.0f))
                    {
                        const float colorLerpValue = AZ::GetClamp(rayDebugInfo.fAvgHits, 0.0f, 1.0f);
                        const AZ::Color cLabelColor = cFreeRayLabelColor.Lerp(cObstructedRayLabelColor, colorLerpValue);

                        auxGeom.Draw2dLabel(
                            vScreenPos.GetX(),
                            vScreenPos.GetY() - 12.0f,
                            1.2f,
                            AZColorToLYColorF(cLabelColor),
                            true,
                            "ObjID: %llu\n#Hits: %2.1f\nOccl: %3.2f",
                            rayInfo.nAudioObjectID,
                            rayDebugInfo.fAvgHits,
                            rayDebugInfo.fOcclusionValue);
                    }
                }
            }
        }
    }

#else // !AUDIO_ENABLE_CRY_PHYSICS

    void RaycastProcessor::DrawObstructionRays(IRenderAuxGeom& auxGeom) const
    {
        static const AZ::Color obstructedRayColor(0.8f, 0.08f, 0.f, 1.f);
        static const AZ::Color freeRayColor(0.08f, 0.8f, 0.f, 1.f);
        static const AZ::Color hitSphereColor(1.f, 0.27f, 0.f, 0.8f);
        static const AZ::Color obstructedRayLabelColor(1.f, 0.f, 0.02f, 0.9f);
        static const AZ::Color freeRayLabelColor(0.f, 1.f, 0.02f, 0.9f);

        static const float hitSphereRadius = 0.02f;

        if (!CanRun())
        {
            return;
        }

        const SAuxGeomRenderFlags previousRenderFlags = auxGeom.GetRenderFlags();
        SAuxGeomRenderFlags newRenderFlags(e_Def3DPublicRenderflags | e_AlphaBlended);
        newRenderFlags.SetCullMode(e_CullModeNone);
        auxGeom.SetRenderFlags(newRenderFlags);

        const bool drawRays = ((g_audioCVars.m_nDrawAudioDebug & eADDF_DRAW_OBSTRUCTION_RAYS) != 0);
        const bool drawLabels = ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_OBSTRUCTION_RAY_LABELS) != 0);

        size_t numRays = m_obstOccType == eAOOCT_SINGLE_RAY ? 1 : s_maxRaysPerObject;
        for (size_t rayIndex = 0; rayIndex < numRays; ++rayIndex)
        {
            const RaycastInfo& rayInfo = m_rayInfos[rayIndex];

            const AZ::Vector3 rayEnd = rayInfo.m_raycastRequest.m_start + rayInfo.m_raycastRequest.m_direction * rayInfo.GetNearestHitDistance();

            if (drawRays)
            {
                const bool rayObstructed = (rayInfo.m_numHits > 0);
                const AZ::Color& rayColor = (rayObstructed ? obstructedRayColor : freeRayColor);

                if (rayObstructed)
                {
                    auxGeom.DrawSphere(
                        AZVec3ToLYVec3(rayEnd),
                        hitSphereRadius,
                        AZColorToLYColorB(hitSphereColor)
                    );
                }

                auxGeom.DrawLine(
                    AZVec3ToLYVec3(rayInfo.m_raycastRequest.m_start),
                    AZColorToLYColorB(freeRayColor),
                    AZVec3ToLYVec3(rayEnd),
                    AZColorToLYColorB(rayColor),
                    1.f
                );
            }

            IRenderer* renderer = gEnv->pRenderer;
            if (drawLabels && renderer)
            {
                float screenProj[3];
                renderer->ProjectToScreen(rayEnd.GetX(), rayEnd.GetY(), rayEnd.GetZ(),
                    &screenProj[0], &screenProj[1], &screenProj[2]);

                screenProj[0] *= 0.01f * aznumeric_cast<float>(renderer->GetWidth());
                screenProj[1] *= 0.01f * aznumeric_cast<float>(renderer->GetHeight());

                AZ::Vector3 screenPos = AZ::Vector3::CreateFromFloat3(screenProj);

                if ((0.f <= screenPos.GetZ()) && (screenPos.GetZ() <= 1.f))
                {
                    float lerpValue = rayInfo.m_contribution;
                    AZ::Color labelColor = freeRayLabelColor.Lerp(obstructedRayLabelColor, lerpValue);

                    auxGeom.Draw2dLabel(
                        screenPos.GetX(),
                        screenPos.GetY() - 12.f,
                        1.6f,
                        AZColorToLYColorF(labelColor),
                        true,
                        (rayIndex == 0) ? "OBST: %.2f" : "OCCL: %.2f",
                        rayInfo.GetDistanceScaledContribution()
                    );
                }
            }
        }

        auxGeom.SetRenderFlags(previousRenderFlags);
    }

#endif // AUDIO_ENABLE_CRY_PHYSICS

#endif // INCLUDE_AUDIO_PRODUCTION_CODE

} // namespace Audio
