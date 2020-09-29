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

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/set.h>

#include <ATLEntities.h>
#include <ATLEntityData.h>

#include <climits>

#if AUDIO_ENABLE_CRY_PHYSICS
    #include <IPhysics.h>
#else
    #include <AzFramework/Physics/Casts.h>
#endif // AUDIO_ENABLE_CRY_PHYSICS

struct IRenderAuxGeom;

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EATLTriggerStatus : TATLEnumFlagsType
    {
        eATS_NONE                       = 0,
        eATS_PLAYING                    = AUDIO_BIT(0),
        eATS_PREPARED                   = AUDIO_BIT(1),
        eATS_LOADING                    = AUDIO_BIT(2),
        eATS_UNLOADING                  = AUDIO_BIT(3),
        eATS_STARTING                   = AUDIO_BIT(4),
        eATS_WAITING_FOR_REMOVAL        = AUDIO_BIT(5),
        eATS_CALLBACK_ON_AUDIO_THREAD   = AUDIO_BIT(6),
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLTriggerImplState
    {
        SATLTriggerImplState()
            : nFlags(eATS_NONE)
        {}

        TATLEnumFlagsType nFlags;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLTriggerInstanceState
    {
        SATLTriggerInstanceState()
            : nFlags(eATS_NONE)
            , nTriggerID(INVALID_AUDIO_CONTROL_ID)
            , numPlayingEvents(0)
            , numLoadingEvents(0)
            , pOwnerOverride(nullptr)
            , pUserData(nullptr)
            , pUserDataOwner(nullptr)
        {}

        TATLEnumFlagsType nFlags;
        TAudioControlID nTriggerID;
        size_t numPlayingEvents;
        size_t numLoadingEvents;
        void* pOwnerOverride;
        void* pUserData;
        void* pUserDataOwner;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // CATLAudioObjectBase-related typedefs

    using TObjectEventSet = ATLSetLookupType<TAudioEventID>;
    using TObjectTriggerInstanceSet = ATLSetLookupType<TAudioTriggerInstanceID>;
    using TObjectTriggerImplStates = ATLMapLookupType<TAudioTriggerImplID, SATLTriggerImplState>;
    using TObjectTriggerStates = ATLMapLookupType<TAudioTriggerInstanceID, SATLTriggerInstanceState>;
    using TObjectStateMap = ATLMapLookupType<TAudioControlID, TAudioSwitchStateID>;
    using TObjectRtpcMap = ATLMapLookupType<TAudioControlID, float>;
    using TObjectEnvironmentMap = ATLMapLookupType<TAudioEnvironmentID, float>;


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // this class wraps the common functionality shared by the AudioObject and the GlobalAudioObject
    class CATLAudioObjectBase
        : public CATLEntity<TAudioObjectID>
    {
    public:
        ~CATLAudioObjectBase() override {}

        void ReportStartingTriggerInstance(const TAudioTriggerInstanceID audioTriggerInstanceID, const TAudioControlID audioControlID);
        void ReportStartedTriggerInstance(const TAudioTriggerInstanceID audioTriggerInstanceID, void* const pOwnerOverride, void* const pUserData, void* const pUserDataOwner, const TATLEnumFlagsType nFlags);

        void ReportStartedEvent(const CATLEvent* const pEvent);
        void ReportFinishedEvent(const CATLEvent* const pEvent, const bool bSuccess);

        void ReportPrepUnprepTriggerImpl(const TAudioTriggerImplID nTriggerImplID, const bool bPrepared);

        void SetSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID);
        void SetRtpc(const TAudioControlID nRtpcID, const float fValue);
        void SetEnvironmentAmount(const TAudioEnvironmentID nEnvironmentID, const float fAmount);

        const TObjectTriggerImplStates& GetTriggerImpls() const;
        const TObjectRtpcMap& GetRtpcs() const;
        const TObjectEnvironmentMap& GetEnvironments() const;

        void ClearRtpcs();
        void ClearEnvironments();

        const TObjectEventSet& GetActiveEvents() const
        {
            return m_cActiveEvents;
        }

        bool HasActiveEvents() const;

        TObjectTriggerInstanceSet GetTriggerInstancesByOwner(void* const pOwner) const;

        void IncrementRefCount()
        {
            ++m_nRefCounter;
        }

        void DecrementRefCount()
        {
            AZ_Assert(m_nRefCounter > 0, "CATLAudioObjectBase - Too many refcount decrements!");
            --m_nRefCounter;
        }

        size_t GetRefCount() const
        {
            return m_nRefCounter;
        }

        void SetImplDataPtr(IATLAudioObjectData* const pImplData)
        {
            m_pImplData = pImplData;
        }

        IATLAudioObjectData* GetImplDataPtr() const
        {
            return m_pImplData;
        }

        virtual bool HasPosition() const = 0;

    protected:

        CATLAudioObjectBase(const TAudioObjectID nObjectID, const EATLDataScope eDataScope, IATLAudioObjectData* const pImplData = nullptr)
            : CATLEntity<TAudioObjectID>(nObjectID, eDataScope)
            , m_nRefCounter(0)
            , m_pImplData(pImplData)
        {}

        virtual void Clear();
        virtual void Update(const float fUpdateIntervalMS, const SATLWorldPosition& rListenerPosition);
        void ReportFinishedTriggerInstance(TObjectTriggerStates::iterator& iTriggerEntry);

        TObjectEventSet m_cActiveEvents;
        TObjectTriggerStates m_cTriggers;
        TObjectTriggerImplStates m_cTriggerImpls;
        TObjectRtpcMap m_cRtpcs;
        TObjectEnvironmentMap m_cEnvironments;
        TObjectStateMap m_cSwitchStates;
        size_t m_nRefCounter;
        IATLAudioObjectData* m_pImplData;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
    public:
        void CheckBeforeRemoval(const CATLDebugNameStore* const pDebugNameStore);

    protected:
        ///////////////////////////////////////////////////////////////////////////////////////////////
        class CStateDebugDrawData
        {
        public:
            CStateDebugDrawData(const TAudioSwitchStateID nState = INVALID_AUDIO_SWITCH_STATE_ID);
            ~CStateDebugDrawData();

            void Update(const TAudioSwitchStateID nNewState);

            TAudioSwitchStateID nCurrentState;
            float fCurrentAlpha;

        private:
            static const float fMaxAlpha;
            static const float fMinAlpha;
            static const int nMaxToMinUpdates;
        };

        using TStateDrawInfoMap = ATLMapLookupType<TAudioControlID, CStateDebugDrawData>;

        AZStd::string GetTriggerNames(const char* const sSeparator, const CATLDebugNameStore* const pDebugNameStore);
        AZStd::string GetEventIDs(const char* const sSeparator);

        mutable TStateDrawInfoMap m_cStateDrawInfoMap;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLGlobalAudioObject
        : public CATLAudioObjectBase
    {
    public:
        explicit CATLGlobalAudioObject(const TAudioObjectID nID, IATLAudioObjectData* const pImplData = nullptr)
            : CATLAudioObjectBase(nID, eADS_GLOBAL, pImplData)
        {}

        bool HasPosition() const override
        {
            return false;
        }
    };


    // Physics-related obstruction/occlusion raycasting...

#if AUDIO_ENABLE_CRY_PHYSICS
    static constexpr size_t s_maxObstructionRayHits = 5;
    static constexpr size_t s_maxObstructionRays = 5;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
    ///////////////////////////////////////////////////////////////////////////////////////////
    struct SRayDebugInfo
    {
        SRayDebugInfo()
            : vStartPosition(0.f)
            , vDirection(0.f)
            , vRndOffset(0.f)
            , vBegin(0.f)
            , vEnd(0.f)
            , vStableEnd(0.f)
            , fOcclusionValue(0.f)
            , fDistToNearestObstacle(std::numeric_limits<float>::max())
            , fAvgHits(0.f)
        {}

        ~SRayDebugInfo() = default;
        SRayDebugInfo(const SRayDebugInfo&) = default;
        SRayDebugInfo& operator=(const SRayDebugInfo&) = default;

        // Debug data...
        AZ::Vector3 vStartPosition;
        AZ::Vector3 vDirection;
        AZ::Vector3 vRndOffset;
        AZ::Vector3 vBegin;
        AZ::Vector3 vEnd;
        AZ::Vector3 vStableEnd;

        float fOcclusionValue;
        float fDistToNearestObstacle;
        float fAvgHits;
    };
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

    ///////////////////////////////////////////////////////////////////////////////////////////
    struct SRayInfo
    {
        static constexpr float s_fSmoothingAlpha = 0.05f;

        SRayInfo(const size_t nPassedRayID, const TAudioObjectID nPassedAudioObjectID)
            : nRayID(nPassedRayID)
            , nAudioObjectID(nPassedAudioObjectID)
            , fTotalSoundOcclusion(0.0f)
            , nNumHits(0)
        {}

        SRayInfo& operator=(const SRayInfo& rOther)
        {
            nRayID = rOther.nRayID;
            nAudioObjectID = rOther.nAudioObjectID;
            fTotalSoundOcclusion = rOther.fTotalSoundOcclusion;
            nNumHits = rOther.nNumHits;
            for (size_t i = 0; i < s_maxObstructionRayHits; ++i)
            {
                aHits[i] = rOther.aHits[i];
            }

        #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
            m_debugInfo = rOther.m_debugInfo;
        #endif // INCLUDE_AUDIO_PRODUCTION_CODE

            return *this;
        }

        ~SRayInfo() = default;

        // Runtime Data...
        size_t nRayID;
        TAudioObjectID nAudioObjectID;
        float fTotalSoundOcclusion;
        int nNumHits;

        ray_hit aHits[s_maxObstructionRayHits];

    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        SRayDebugInfo m_debugInfo;
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
    };



    ///////////////////////////////////////////////////////////////////////////////////////////////
    class CPropagationProcessor
    {
    public:
        static bool s_canIssueRWIs;
        static constexpr float s_minObstructionDistance = 0.3f;

        CPropagationProcessor(
            const TAudioObjectID nObjectID,
            const SATLWorldPosition& rPosition,
            size_t& rRefCounter);

        ~CPropagationProcessor();

        // CryPhysics system callback
        static int OnObstructionTest(const EventPhys* pEvent);

        static void ProcessObstructionRay(const int nNumHits, SRayInfo* const pRayInfo, const bool bReset = false);

        void Update(const float fUpdateMS);
        void SetObstructionOcclusionCalcType(const EAudioObjectObstructionCalcType eObstOcclCalcType);
        bool CanRunObstructionOcclusion() const
        {
            return s_canIssueRWIs && m_eObstOcclCalcType != eAOOCT_IGNORE;
        }
        void GetPropagationData(SATLSoundPropagationData& rPropagationData) const;
        void RunObstructionQuery(const SATLWorldPosition& rListenerPosition, const bool bSyncCall, const bool bReset = false);
        void ReportRayProcessed(const size_t nRayID);
        void ReleasePendingRays();

    private:
        static size_t NumRaysFromCalcType(const EAudioObjectObstructionCalcType eCalcType);

        void ProcessObstructionOcclusion(const bool bReset = false);
        void CastObstructionRay(const AZ::Vector3& rOrigin,
            const AZ::Vector3& rRndOffset,
            const AZ::Vector3& rDirection,
            const size_t nRayIdx,
            const bool bSyncCall,
            const bool bReset = false);

        size_t m_nRemainingRays;
        size_t m_nTotalRays;

        CSmoothFloat m_oObstructionValue;
        CSmoothFloat m_oOcclusionValue;
        const SATLWorldPosition& m_rPosition;

        size_t& m_rRefCounter;  // References the owning audio object's refcounter.

        float m_fCurrListenerDist;

        using TRayInfoVec = AZStd::vector<SRayInfo, Audio::AudioSystemStdAllocator>;
        TRayInfoVec m_vRayInfos;
        EAudioObjectObstructionCalcType m_eObstOcclCalcType;

        bool m_pendingRaysReleased;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
    public:
        void DrawObstructionRays(IRenderAuxGeom& auxGeom) const;
        size_t GetNumRays() const
        {
            return NumRaysFromCalcType(m_eObstOcclCalcType);
        }

        static size_t s_nTotalSyncPhysRays;
        static size_t s_nTotalAsyncPhysRays;

    private:
        mutable float m_fTimeSinceLastUpdateMS;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
    };

#else // !AUDIO_ENABLE_CRY_PHYSICS

    static constexpr AZ::u16 s_maxHitResultsPerRaycast = 5;
    static constexpr AZ::u16 s_maxRaysPerObject = 5;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct AudioRaycastRequest
    {
        Physics::RayCastRequest m_request{};
        TAudioObjectID m_audioObjectId = INVALID_AUDIO_OBJECT_ID;
        size_t m_rayIndex = 0;

        AudioRaycastRequest(const Physics::RayCastRequest& request, TAudioObjectID audioObjectId, size_t rayId)
            : m_request(request)
            , m_audioObjectId(audioObjectId)
            , m_rayIndex(rayId)
        {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct AudioRaycastResult
    {
        AZStd::vector<Physics::RayCastHit> m_result{};
        TAudioObjectID m_audioObjectId = INVALID_AUDIO_OBJECT_ID;
        size_t m_rayIndex = 0;

        AudioRaycastResult(AZStd::vector<Physics::RayCastHit>&& result, TAudioObjectID audioObjectId, size_t rayId)
            : m_result(AZStd::move(result))
            , m_audioObjectId(audioObjectId)
            , m_rayIndex(rayId)
        {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioRaycastRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioRaycastRequests() = default;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // No mutex!  All audio raycast requests are initiated and received on the Audio Thread.

        virtual void PushAudioRaycastRequest(const AudioRaycastRequest&) = 0;
    };

    using AudioRaycastRequestBus = AZ::EBus<AudioRaycastRequests>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioRaycastNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioRaycastNotifications() = default;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        // No mutex!  All audio raycast notifications are initiated and received on the Audio Thread.
        using BusIdType = TAudioObjectID;

        virtual void OnAudioRaycastResults(const AudioRaycastResult&) = 0;
    };

    using AudioRaycastNotificationBus = AZ::EBus<AudioRaycastNotifications>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct RaycastInfo
    {
        AZStd::fixed_vector<Physics::RayCastHit, s_maxHitResultsPerRaycast> m_hits;
        Physics::RayCastRequest m_raycastRequest;
        float m_contribution = 0.f;
        float m_cacheTimerMs = 0.f;
        AZ::u16 m_numHits = 0;
        bool m_pending = false; //!< Whether the ray has been requested and is still pending.
        bool m_cached = false;

        void UpdateContribution();
        void Reset()
        {
            m_hits.clear();
            m_contribution = 0.f;
            m_cacheTimerMs = 0.f;
            m_numHits = 0;
            m_pending = false;
            m_cached = false;
        }

        float GetDistanceScaledContribution() const;
        float GetNearestHitDistance() const;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class RaycastProcessor
    {
        friend class CATLAudioObject;

    public:
        RaycastProcessor(const TAudioObjectID objectId, const SATLWorldPosition& objectPosition);

        void Update(float deltaMs);
        void Reset();
        void SetType(EAudioObjectObstructionCalcType calcType);
        bool CanRun() const;
        void Run(const SATLWorldPosition& listenerPosition);
        void CastRay(const AZ::Vector3& origin, const AZ::Vector3& dest, const AZ::u16 rayIndex);

        float GetObstruction() const
        {
            return AZ::GetClamp(m_obstructionValue.GetCurrent(), 0.f, 1.f);
        }
        float GetOcclusion() const
        {
            return AZ::GetClamp(m_occlusionValue.GetCurrent(), 0.f, 1.f);
        }

#if defined(AZ_TESTS_ENABLED)
        void SetupTestRay(AZ::u16 rayIndex);
#endif // AZ_TESTS_ENABLED

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        void DrawObstructionRays(IRenderAuxGeom& auxGeom) const;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

        static constexpr float s_epsilon = 1e-3f;
        static bool s_raycastsEnabled;

    private:
        AZStd::fixed_vector<RaycastInfo, s_maxRaysPerObject> m_rayInfos;
        const SATLWorldPosition& m_position;
        CSmoothFloat m_obstructionValue;
        CSmoothFloat m_occlusionValue;
        TAudioObjectID m_audioObjectId;
        EAudioObjectObstructionCalcType m_obstOccType;
    };
#endif // AUDIO_ENABLE_CRY_PHYSICS


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLAudioObject
        : public CATLAudioObjectBase
#if !AUDIO_ENABLE_CRY_PHYSICS
        , public AudioRaycastNotificationBus::Handler
#endif // !AUDIO_ENABLE_CRY_PHYSICS
    {
    public:
        explicit CATLAudioObject(const TAudioObjectID nID, IATLAudioObjectData* const pImplData = nullptr)
            : CATLAudioObjectBase(nID, eADS_NONE, pImplData)
            , m_nFlags(eAOF_NONE)
            , m_fPreviousVelocity(0.0f)
#if AUDIO_ENABLE_CRY_PHYSICS
            , m_oPropagationProcessor(nID, m_oPosition, m_nRefCounter)
#else
            , m_raycastProcessor(nID, m_oPosition)
#endif // AUDIO_ENABLE_CRY_PHYSICS
        {
        }

        ~CATLAudioObject() override
        {
#if !AUDIO_ENABLE_CRY_PHYSICS
            AudioRaycastNotificationBus::Handler::BusDisconnect();
#endif // !AUDIO_ENABLE_CRY_PHYSICS
        }

        CATLAudioObject(const CATLAudioObject&) = delete;           // not defined; calls will fail at compile time
        CATLAudioObject& operator=(const CATLAudioObject&) = delete; // not defined; calls will fail at compile time

        // CATLAudioObjectBase
        bool HasPosition() const override
        {
            return true;
        }
        void Clear() override;
        void Update(const float fUpdateIntervalMS, const SATLWorldPosition& rListenerPosition) override;
        // ~CATLAudioObjectBase

        void SetPosition(const SATLWorldPosition& oNewPosition);

#if AUDIO_ENABLE_CRY_PHYSICS
        void ReportPhysicsRayProcessed(const size_t nRayID);
        void SetObstructionOcclusionCalc(const EAudioObjectObstructionCalcType ePassedOOCalcType);
        void ResetObstructionOcclusion(const SATLWorldPosition& rListenerPosition);
        bool CanRunObstructionOcclusion() const
        {
            return m_oPropagationProcessor.CanRunObstructionOcclusion();
        }
        void GetPropagationData(SATLSoundPropagationData& rPropagationData);
        void ReleasePendingRays();
#else
        void SetRaycastCalcType(const EAudioObjectObstructionCalcType type);
        void RunRaycasts(const SATLWorldPosition& listenerPos);
        bool CanRunRaycasts() const;
        void GetObstOccData(SATLSoundPropagationData& data) const;

#if defined(AZ_TESTS_ENABLED)
        RaycastProcessor& GetRaycastProcessor()
        {
            return m_raycastProcessor;
        }
        const RaycastProcessor& GetRaycastProcessor() const
        {
            return m_raycastProcessor;
        }
#endif // AZ_TESTS_ENABLED

        // AudioRaycastNotificationBus::Handler
        void OnAudioRaycastResults(const AudioRaycastResult& result) override;
#endif // AUDIO_ENABLE_CRY_PHYSICS

        void SetVelocityTracking(const bool bTrackingOn);
        bool GetVelocityTracking() const
        {
            return (m_nFlags & eAOF_TRACK_VELOCITY) != 0;
        }
        void UpdateVelocity(const float fUpdateIntervalMS);

    private:
        TATLEnumFlagsType m_nFlags;
        float m_fPreviousVelocity;
        SATLWorldPosition m_oPosition;
        SATLWorldPosition m_oPreviousPosition;

#if AUDIO_ENABLE_CRY_PHYSICS
        CPropagationProcessor m_oPropagationProcessor;
#else
        RaycastProcessor m_raycastProcessor;
#endif // AUDIO_ENABLE_CRY_PHYSICS

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
    public:
        void DrawDebugInfo(IRenderAuxGeom& auxGeom, const AZ::Vector3& vListenerPos, const CATLDebugNameStore* const pDebugNameStore) const;
        const SATLWorldPosition& GetPosition() const
        {
            return m_oPosition;
        }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
    };

} // namespace Audio
