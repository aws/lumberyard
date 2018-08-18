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

#include "ATLEntities.h"
#include <ATLEntityData.h>
#include <IPhysics.h>
#include <TimeValue.h>

#include <AzCore/std/containers/set.h>

struct IRenderAuxGeom;

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EATLTriggerStatus : TATLEnumFlagsType
    {
        eATS_NONE                       = 0,
        eATS_PLAYING                    = BIT(0),
        eATS_PREPARED                   = BIT(1),
        eATS_LOADING                    = BIT(2),
        eATS_UNLOADING                  = BIT(3),
        eATS_STARTING                   = BIT(4),
        eATS_WAITING_FOR_REMOVAL        = BIT(5),
        eATS_CALLBACK_ON_AUDIO_THREAD   = BIT(6),
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
            , fExpirationTimeMS(0.0f)
            , fRemainingTimeMS(0.0f)
            , pOwnerOverride(nullptr)
            , pUserData(nullptr)
            , pUserDataOwner(nullptr)
        {}

        TATLEnumFlagsType nFlags;
        TAudioControlID nTriggerID;
        size_t numPlayingEvents;
        size_t numLoadingEvents;
        float fExpirationTimeMS;
        float fRemainingTimeMS;
        void* pOwnerOverride;
        void* pUserData;
        void* pUserDataOwner;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // CATLAudioObjectBase-related typedefs

    using TObjectEventSet = AZStd::set<TAudioEventID, AZStd::less<TAudioEventID>, Audio::AudioSystemStdAllocator>;

    using TObjectTriggerInstanceSet = AZStd::set<TAudioTriggerInstanceID, AZStd::less<TAudioTriggerInstanceID>, Audio::AudioSystemStdAllocator>;

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

        CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> GetTriggerNames(const char* const sSeparator, const CATLDebugNameStore* const pDebugNameStore);
        CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> GetEventIDs(const char* const sSeparator);

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

        ~CATLGlobalAudioObject() override {}

        bool HasPosition() const override
        {
            return false;
        }
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLAudioObject
        : public CATLAudioObjectBase
    {
    public:

        ///////////////////////////////////////////////////////////////////////////////////////////////
        class CPropagationProcessor
        {
        public:
            static bool s_canIssueRWIs;
            static const float s_minObstructionDistance;
            static const size_t s_maxObstructionRayHits = 5;
            static const size_t s_maxObstructionRays = 5;

            ///////////////////////////////////////////////////////////////////////////////////////////
            struct SRayInfo
            {
                static const float s_fSmoothingAlpha;

                SRayInfo(const size_t nPassedRayID, const TAudioObjectID nPassedAudioObjectID)
                    : nRayID(nPassedRayID)
                    , nAudioObjectID(nPassedAudioObjectID)
                    , fTotalSoundOcclusion(0.0f)
                    , nNumHits(0)
            #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                    , vStartPosition(ZERO)
                    , vDirection(ZERO)
                    , vRndOffset(ZERO)
                    , fAvgHits(0.0f)
                    , fDistToFirstObstacle(FLT_MAX)
            #endif // INCLUDE_AUDIO_PRODUCTION_CODE
                {}

                SRayInfo& operator=(const SRayInfo& rOther)
                {
                    const_cast<size_t&>(nRayID) = rOther.nRayID;
                    const_cast<TAudioObjectID&>(nAudioObjectID) = rOther.nAudioObjectID;
                    fTotalSoundOcclusion = rOther.fTotalSoundOcclusion;
                    nNumHits = rOther.nNumHits;
                    for (size_t i = 0; i < s_maxObstructionRayHits; ++i)
                    {
                        aHits[i] = rOther.aHits[i];
                    }

            #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                    vStartPosition = rOther.vStartPosition;
                    vDirection = rOther.vDirection;
                    vRndOffset = rOther.vRndOffset;
                    fAvgHits = rOther.fAvgHits;
                    fDistToFirstObstacle = rOther.fDistToFirstObstacle;
            #endif // INCLUDE_AUDIO_PRODUCTION_CODE

                    return *this;
                }

                ~SRayInfo() {}

                void Reset();

                const size_t nRayID;
                const TAudioObjectID nAudioObjectID;
                float fTotalSoundOcclusion;
                int nNumHits;
                ray_hit aHits[s_maxObstructionRayHits];

        #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                Vec3 vStartPosition;
                Vec3 vDirection;
                Vec3 vRndOffset;
                float fAvgHits;
                float fDistToFirstObstacle;
        #endif // INCLUDE_AUDIO_PRODUCTION_CODE
            }; // end struct SRayInfo


            CPropagationProcessor(
                const TAudioObjectID nObjectID,
                const SATLWorldPosition& rPosition,
                size_t& rRefCounter);

            ~CPropagationProcessor();

            // PhysicsSystem callback
            static int OnObstructionTest(const EventPhys* pEvent);
            static void ProcessObstructionRay(const int nNumHits, SRayInfo* const pRayInfo, const bool bReset = false);
            static size_t NumRaysFromCalcType(const EAudioObjectObstructionCalcType eCalcType);

            void Update(const float fUpdateMS);
            void SetObstructionOcclusionCalcType(const EAudioObjectObstructionCalcType eObstOcclCalcType);
            bool CanRunObstructionOcclusion() const
            {
                return s_canIssueRWIs && m_eObstOcclCalcType != eAOOCT_NONE && m_eObstOcclCalcType != eAOOCT_IGNORE;
            }
            void GetPropagationData(SATLSoundPropagationData& rPropagationData) const;
            void RunObstructionQuery(const SATLWorldPosition& rListenerPosition, const bool bSyncCall, const bool bReset = false);
            void ReportRayProcessed(const size_t nRayID);
            void ReleasePendingRays();

        private:
            void ProcessObstructionOcclusion(const bool bReset = false);
            void CastObstructionRay(const Vec3& rOrigin,
                const Vec3& rRndOffset,
                const Vec3& rDirection,
                const size_t nRayIdx,
                const bool bSyncCall,
                const bool bReset = false);

            size_t m_nRemainingRays;
            size_t m_nTotalRays;

            CSmoothFloat m_oObstructionValue;
            CSmoothFloat m_oOcclusionValue;
            const SATLWorldPosition& m_rPosition;

            size_t& m_rRefCounter;

            float m_fCurrListenerDist;

            using TRayInfoVec = AZStd::vector<SRayInfo, Audio::AudioSystemStdAllocator>;
            TRayInfoVec m_vRayInfos;
            EAudioObjectObstructionCalcType m_eObstOcclCalcType;

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
            ///////////////////////////////////////////////////////////////////////////////////////////
            struct SRayDebugInfo
            {
                SRayDebugInfo()
                    : vBegin(ZERO)
                    , vEnd(ZERO)
                    , vStableEnd(ZERO)
                    , fOcclusionValue(0.0f)
                    , fDistToNearestObstacle(FLT_MAX)
                    , nNumHits(0)
                    , fAvgHits(0.0f)
                {}

                ~SRayDebugInfo() {}

                Vec3 vBegin;
                Vec3 vEnd;
                Vec3 vStableEnd;
                float fOcclusionValue;
                float fDistToNearestObstacle;
                int nNumHits;
                float fAvgHits;
            }; // end struct SRayDebugInfo

            using TRayDebugInfoVec = AZStd::vector<SRayDebugInfo, Audio::AudioSystemStdAllocator>;
            TRayDebugInfoVec m_vRayDebugInfos;

            mutable float m_fTimeSinceLastUpdateMS;
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
        }; // end class CPropagationProcessor


        explicit CATLAudioObject(const TAudioObjectID nID, IATLAudioObjectData* const pImplData = nullptr)
            : CATLAudioObjectBase(nID, eADS_NONE, pImplData)
            , m_nFlags(eAOF_NONE)
            , m_fPreviousVelocity(0.0f)
            , m_oPropagationProcessor(nID, m_oPosition, m_nRefCounter)
        {}

        ~CATLAudioObject() override {}

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

        void ReportPhysicsRayProcessed(const size_t nRayID);
        void SetPosition(const SATLWorldPosition& oNewPosition);
        void SetObstructionOcclusionCalc(const EAudioObjectObstructionCalcType ePassedOOCalcType);
        void ResetObstructionOcclusion(const SATLWorldPosition& rListenerPosition);
        bool CanRunObstructionOcclusion() const
        {
            return m_oPropagationProcessor.CanRunObstructionOcclusion();
        }
        void GetPropagationData(SATLSoundPropagationData& rPropagationData);
        void ReleasePendingRays();
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
        CPropagationProcessor m_oPropagationProcessor;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
    public:
        void DrawDebugInfo(IRenderAuxGeom& auxGeom, const Vec3& vListenerPos, const CATLDebugNameStore* const pDebugNameStore) const;
        const SATLWorldPosition& GetPosition() const
        {
            return m_oPosition;
        }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
    };
} // namespace Audio
