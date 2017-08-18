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
#ifndef CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTAUDIO_H
#define CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTAUDIO_H
#pragma once

#include <IAudioSystem.h>
#include <STLPoolAllocator.h>
#include "Components/IComponentAudio.h"

//! Implementation of the entity's audio component.
struct CComponentAudio
    : public IComponentAudio
{
public:
    CComponentAudio();
    virtual ~CComponentAudio();

    // IComponent
    void ProcessEvent(SEntityEvent& event) override;
    void Initialize(const SComponentInitializer& init) override;
    void Done() override;
    void UpdateComponent(SEntityUpdateContext& ctx) override {}
    bool InitComponent(IEntity* pEntity, SEntitySpawnParams& params) override { return true; }
    void Reload(IEntity* pEntity, SEntitySpawnParams& params) override;
    void GetMemoryUsage(ICrySizer* pSizer) const override { pSizer->AddObject(this, sizeof(*this)); }
    // ~IComponent

    virtual void SerializeXML(XmlNodeRef& entityNode, bool bLoading) {}
    virtual void Serialize(TSerialize ser);
    virtual bool NeedSerialize() { return false; }
    virtual bool GetSignature(TSerialize signature);

    // IComponentAudio
    void SetFadeDistance(const float fFadeDistance) override { m_fFadeDistance = fFadeDistance; }
    float GetFadeDistance() const override { return m_fFadeDistance; }
    void SetEnvironmentFadeDistance(const float fEnvironmentFadeDistance) override { m_fEnvironmentFadeDistance = fEnvironmentFadeDistance; }
    float GetEnvironmentFadeDistance() const override { return m_fEnvironmentFadeDistance; }
    void SetEnvironmentID(const Audio::TAudioEnvironmentID nEnvironmentID) override { m_nAudioEnvironmentID = nEnvironmentID; }
    Audio::TAudioEnvironmentID GetEnvironmentID() const override { return m_nAudioEnvironmentID; }
    Audio::TAudioProxyID CreateAuxAudioProxy() override;
    bool RemoveAuxAudioProxy(const Audio::TAudioProxyID nAudioProxyLocalID) override;
    void SetAuxAudioProxyOffset(const Audio::SATLWorldPosition& rOffset, const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) override;
    const Audio::SATLWorldPosition& GetAuxAudioProxyOffset(const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) override;
    bool ExecuteTrigger(const Audio::TAudioControlID nTriggerID, const ELipSyncMethod eLipSyncMethod, const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID, const Audio::SAudioCallBackInfos& rCallBackInfos = Audio::SAudioCallBackInfos::GetEmptyObject()) override;
    void StopTrigger(const Audio::TAudioControlID nID, const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) override;
    void StopAllTriggers() override;
    void SetSwitchState(const Audio::TAudioControlID nSwitchID, const Audio::TAudioSwitchStateID nStateID, const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) override;
    void SetRtpcValue(const Audio::TAudioControlID nRtpcID, const float fValue, const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) override;
    void ResetRtpcValues(const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) override;
    void SetObstructionCalcType(const Audio::EAudioObjectObstructionCalcType eObstructionType, const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) override;
    void SetEnvironmentAmount(const Audio::TAudioEnvironmentID nEnvironmentID, const float fAmount, const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) override;
    void SetCurrentEnvironments(const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) override;
    void AuxAudioProxiesMoveWithEntity(const bool bCanMoveWithEntity, const Audio::TAudioProxyID nAudioProxyLocalID = INVALID_AUDIO_PROXY_ID) override;
    void AddAsListenerToAuxAudioProxy(const Audio::TAudioProxyID nAudioProxyLocalID, Audio::AudioRequestCallbackType func, Audio::EAudioRequestType requestType = Audio::eART_AUDIO_ALL_REQUESTS, Audio::TATLEnumFlagsType specificRequestMask = ALL_AUDIO_REQUEST_SPECIFIC_TYPE_FLAGS) override;
    void RemoveAsListenerFromAuxAudioProxy(const Audio::TAudioProxyID nAudioProxyLocalID, Audio::AudioRequestCallbackType func) override;
    // ~IComponentAudio

private:

    enum EEntityAudioComponentFlags : Audio::TATLEnumFlagsType
    {
        eEACF_NONE                  = 0,
        eEACF_HIDDEN                = BIT(0),
        eEACF_CAN_MOVE_WITH_ENTITY  = BIT(1),
    };

    struct SAudioProxyWrapper
    {
        // this needs to be non-explicit
        SAudioProxyWrapper(Audio::IAudioProxy* const pPassedIAudioProxy)
            : pIAudioProxy(pPassedIAudioProxy)
            , mFlags(eEACF_CAN_MOVE_WITH_ENTITY)
        {}

        ~SAudioProxyWrapper() {}

        Audio::IAudioProxy* const pIAudioProxy;
        Audio::SATLWorldPosition oOffset;
        Audio::TATLEnumFlagsType mFlags;

    private:
        SAudioProxyWrapper()
            : pIAudioProxy(nullptr)
        {}
    };

    using TAudioProxyPair = std::pair<const Audio::TAudioProxyID, SAudioProxyWrapper>;
    using TAuxAudioProxies = std::map<const Audio::TAudioProxyID, SAudioProxyWrapper, std::less<Audio::TAudioProxyID>, stl::STLPoolAllocator<TAudioProxyPair> >;

    static TAudioProxyPair s_oNULLAudioProxyPair;
    static const Audio::SATLWorldPosition s_oNULLOffset;

    void OnListenerEnter(const IEntity* const pEntity);
    void OnListenerMoveNear(const IEntity* const __restrict pEntity, const IEntity* const __restrict pArea);
    void OnListenerMoveInside(const IEntity* const pEntity);
    void OnListenerExclusiveMoveInside(const IEntity* const __restrict pEntity, const IEntity* const __restrict pAreaHigh, const IEntity* const __restrict pAreaLow, const float fFade);
    void OnMove();
    void OnEnter(const IEntity* const pEntity);
    void OnLeaveNear(const IEntity* const pEntity);
    void OnMoveNear(const IEntity* const __restrict pEntity, const IEntity* const __restrict pArea);
    void OnMoveInside(const IEntity* const pEntity);
    void OnExclusiveMoveInside(const IEntity* const __restrict pEntity, const IEntity* const __restrict pEntityAreaHigh, const IEntity* const __restrict pEntityAreaLow, const float fFade);
    void OnAreaCrossing();

    TAudioProxyPair& GetAuxAudioProxyPair(const Audio::TAudioProxyID nAudioProxyLocalID);
    void SetEnvironmentAmountInternal(const IEntity* const pEntity, const float fAmount) const;
    void UpdateHideStatus();

    // Function objects
    struct SReleaseAudioProxy
    {
        SReleaseAudioProxy() {}

        inline void operator()(const TAudioProxyPair& rPair)
        {
            rPair.second.pIAudioProxy->Release();
        }
    };

    struct SResetAudioProxy
    {
        SResetAudioProxy() {}

        inline void operator()(const TAudioProxyPair& rPair)
        {
            rPair.second.pIAudioProxy->Reset();
        }
    };

    struct SInitializeAudioProxy
    {
        explicit SInitializeAudioProxy(const char* const sPassedObjectName)
            : sObjectName(sPassedObjectName)
        {}

        inline void operator()(const TAudioProxyPair& rPair)
        {
            rPair.second.pIAudioProxy->Initialize(sObjectName);
        }

    private:
        const char* const sObjectName;
    };

    struct SRepositionAudioProxy
    {
        explicit SRepositionAudioProxy(const Audio::SATLWorldPosition& rPassedPosition, const bool forceUpdate = false)
            : rPosition(rPassedPosition)
            , bForceUpdate(forceUpdate)
        {}

        inline void operator()(const TAudioProxyPair& rPair)
        {
            if (rPair.second.mFlags & eEACF_CAN_MOVE_WITH_ENTITY || bForceUpdate)
            {
                rPair.second.pIAudioProxy->SetPosition(Audio::SATLWorldPosition(rPosition.mPosition * rPair.second.oOffset.mPosition));
            }
        }

    private:
        const Audio::SATLWorldPosition& rPosition;
        const bool bForceUpdate;
    };

    struct SSetFlag
    {
        explicit SSetFlag(const Audio::TATLEnumFlagsType& nPassedFlag, const bool setFlag)
            : nFlag(nPassedFlag)
            , bSetFlag(setFlag)
        {}

        inline void operator()(TAudioProxyPair& rPair)
        {
            if (bSetFlag)
            {
                rPair.second.mFlags |= nFlag;
            }
            else
            {
                rPair.second.mFlags &= ~nFlag;
            }
        }

    private:
        const Audio::TATLEnumFlagsType& nFlag;
        const bool bSetFlag;
    };

    struct SStopTrigger
    {
        explicit SStopTrigger(const Audio::TAudioControlID nPassedID)
            : nTriggerID(nPassedID)
        {}

        inline void operator()(const TAudioProxyPair& rPair)
        {
            rPair.second.pIAudioProxy->StopTrigger(nTriggerID);
        }

    private:
        const Audio::TAudioControlID nTriggerID;
    };

    struct SStopAllTriggers
    {
        SStopAllTriggers()
        {}

        inline void operator()(TAudioProxyPair const& rPair)
        {
            rPair.second.pIAudioProxy->StopAllTriggers();
        }
    };

    struct SSetSwitchState
    {
        SSetSwitchState(const Audio::TAudioControlID nPassedSwitchID, const Audio::TAudioSwitchStateID nPassedStateID)
            : nSwitchID(nPassedSwitchID)
            , nStateID(nPassedStateID)
        {}

        inline void operator()(const TAudioProxyPair& rPair)
        {
            rPair.second.pIAudioProxy->SetSwitchState(nSwitchID, nStateID);
        }

    private:
        const Audio::TAudioControlID nSwitchID;
        const Audio::TAudioSwitchStateID nStateID;
    };

    struct SSetRtpcValue
    {
        SSetRtpcValue(const Audio::TAudioControlID nPassedRtpcID, const float fPassedValue)
            : nRtpcID(nPassedRtpcID)
            , fValue(fPassedValue)
        {}

        inline void operator()(const TAudioProxyPair& rPair)
        {
            rPair.second.pIAudioProxy->SetRtpcValue(nRtpcID, fValue);
        }

    private:
        const Audio::TAudioControlID nRtpcID;
        const float fValue;
    };

    struct SResetRtpcValues
    {
        SResetRtpcValues()
        {}

        inline void operator()(const TAudioProxyPair& rPair)
        {
            rPair.second.pIAudioProxy->ResetRtpcValues();
        }
    };

    struct SSetObstructionCalcType
    {
        explicit SSetObstructionCalcType(const Audio::EAudioObjectObstructionCalcType ePassedObstructionType)
            : eObstructionType(ePassedObstructionType)
        {}

        inline void operator()(const TAudioProxyPair& rPair)
        {
            rPair.second.pIAudioProxy->SetObstructionCalcType(eObstructionType);
        }

    private:
        const Audio::EAudioObjectObstructionCalcType eObstructionType;
    };

    struct SSetEnvironmentAmount
    {
        SSetEnvironmentAmount(const Audio::TAudioEnvironmentID nPassedEnvironmentID, const float fPassedAmount)
            : nEnvironmentID(nPassedEnvironmentID)
            , fAmount(fPassedAmount)
        {}

        inline void operator()(const TAudioProxyPair& rPair)
        {
            rPair.second.pIAudioProxy->SetEnvironmentAmount(nEnvironmentID, fAmount);
        }

    private:
        const Audio::TAudioEnvironmentID nEnvironmentID;
        const float fAmount;
    };

    struct SSetCurrentEnvironments
    {
        SSetCurrentEnvironments(const EntityId nEntityID)
            : m_nEntityID(nEntityID)
        {}

        inline void operator()(const TAudioProxyPair& rPair)
        {
            rPair.second.pIAudioProxy->SetCurrentEnvironments(m_nEntityID);
        }

    private:
        const EntityId m_nEntityID;
    };

    struct SSetAuxAudioProxyOffset
    {
        SSetAuxAudioProxyOffset(const Audio::SATLWorldPosition& rPassedOffset, const Matrix34& rPassedEntityPosition)
            : rOffset(rPassedOffset)
            , rEntityPosition(rPassedEntityPosition)
        {}

        inline void operator()(TAudioProxyPair& rPair)
        {
            rPair.second.oOffset = rOffset;
            rPair.second.oOffset.NormalizeForwardVec();
            const Audio::SATLWorldPosition oPosition(rEntityPosition);
            (SRepositionAudioProxy(oPosition, true))(rPair);
        }

    private:
        const Audio::SATLWorldPosition& rOffset;
        const Matrix34& rEntityPosition;
    };
    // ~Function objects

    TAuxAudioProxies m_mapAuxAudioProxies;
    Audio::TAudioProxyID m_nAudioProxyIDCounter;

    Audio::TAudioEnvironmentID m_nAudioEnvironmentID;
    IEntity* m_pEntity;
    Audio::TATLEnumFlagsType m_nFlags;

    float m_fFadeDistance;
    float m_fEnvironmentFadeDistance;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTAUDIO_H