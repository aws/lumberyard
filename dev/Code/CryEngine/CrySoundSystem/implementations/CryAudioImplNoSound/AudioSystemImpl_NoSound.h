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

#pragma once

#include <IAudioSystemImplementation.h>
#include <AudioAllocators.h>

#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/map.h>

namespace Audio
{
    struct STriggerCallbackInfo;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioSystemImpl_NoSound
        : public AudioSystemImplementationComponent
    {
    public:
        CAudioSystemImpl_NoSound();
        ~CAudioSystemImpl_NoSound() override;

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        // ~AZ::Component

        // AudioSystemImplementationNotificationBus
        void OnAudioSystemLoseFocus() override;
        void OnAudioSystemGetFocus() override;
        void OnAudioSystemMuteAll() override;
        void OnAudioSystemUnmuteAll() override;
        void OnAudioSystemRefresh() override;
        // ~AudioSystemImplementationNotificationBus

        // AudioSystemImplementationRequestBus
        void Update(const float fUpdateIntervalMS) override;

        EAudioRequestStatus Initialize() override;
        EAudioRequestStatus ShutDown() override;
        EAudioRequestStatus Release() override;
        EAudioRequestStatus StopAllSounds() override;

        EAudioRequestStatus RegisterAudioObject(
            IATLAudioObjectData* const pObjectData,
            const char* const sObjectName) override;
        EAudioRequestStatus UnregisterAudioObject(IATLAudioObjectData* const pObjectData) override;
        EAudioRequestStatus ResetAudioObject(IATLAudioObjectData* const pObjectData) override;
        EAudioRequestStatus UpdateAudioObject(IATLAudioObjectData* const pObjectData) override;

        EAudioRequestStatus PrepareTriggerSync(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLTriggerImplData* const pTriggerDataData) override;
        EAudioRequestStatus UnprepareTriggerSync(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLTriggerImplData* const pTriggerData) override;
        EAudioRequestStatus PrepareTriggerAsync(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLTriggerImplData* const pTriggerData,
            IATLEventData* const pEventData) override;
        EAudioRequestStatus UnprepareTriggerAsync(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLTriggerImplData* const pTriggerData,
            IATLEventData* const pEventData) override;
        EAudioRequestStatus ActivateTrigger(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLTriggerImplData* const pTriggerData,
            IATLEventData* const pEventData,
            const SATLSourceData* const pSourceData) override;
        EAudioRequestStatus StopEvent(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLEventData* const pEventData) override;
        EAudioRequestStatus StopAllEvents(
            IATLAudioObjectData* const pAudioObjectData) override;
        EAudioRequestStatus SetPosition(
            IATLAudioObjectData* const pAudioObjectData,
            const SATLWorldPosition& sWorldPosition) override;
        EAudioRequestStatus SetMultiplePositions(
            IATLAudioObjectData* const pAudioObjectData,
            const MultiPositionParams& multiPositions) override;
        EAudioRequestStatus SetEnvironment(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLEnvironmentImplData* const pEnvironmentImplData,
            const float fAmount) override;
        EAudioRequestStatus SetRtpc(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLRtpcImplData* const pRtpcData,
            const float fValue) override;
        EAudioRequestStatus SetSwitchState(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLSwitchStateImplData* const pSwitchStateData) override;
        EAudioRequestStatus SetObstructionOcclusion(
            IATLAudioObjectData* const pAudioObjectData,
            const float fObstruction,
            const float fOcclusion) override;
        EAudioRequestStatus SetListenerPosition(
            IATLListenerData* const pListenerData,
            const SATLWorldPosition& oNewPosition) override;
        EAudioRequestStatus ResetRtpc(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLRtpcImplData* const pRtpcData) override;

        EAudioRequestStatus RegisterInMemoryFile(SATLAudioFileEntryInfo* const pAudioFileEntry) override;
        EAudioRequestStatus UnregisterInMemoryFile(SATLAudioFileEntryInfo* const pAudioFileEntry) override;

        EAudioRequestStatus ParseAudioFileEntry(const XmlNodeRef pAudioFileEntryNode, SATLAudioFileEntryInfo* const pFileEntryInfo) override;
        void DeleteAudioFileEntryData(IATLAudioFileEntryData* const pOldAudioFileEntryData) override;
        const char* const GetAudioFileLocation(SATLAudioFileEntryInfo* const pFileEntryInfo) override;
        const IATLTriggerImplData* NewAudioTriggerImplData(const XmlNodeRef pAudioTriggerNode) override;
        void DeleteAudioTriggerImplData(const IATLTriggerImplData* const pOldTriggerImplData) override;

        const IATLRtpcImplData* NewAudioRtpcImplData(const XmlNodeRef pAudioRtpcNode) override;
        void DeleteAudioRtpcImplData(const IATLRtpcImplData* const pOldRtpcImplData) override;

        const IATLSwitchStateImplData* NewAudioSwitchStateImplData(const XmlNodeRef pAudioSwitchStateImplNode) override;
        void DeleteAudioSwitchStateImplData(const IATLSwitchStateImplData* const pOldAudioSwitchStateImplData) override;

        const IATLEnvironmentImplData* NewAudioEnvironmentImplData(const XmlNodeRef pAudioEnvironmentNode) override;
        void DeleteAudioEnvironmentImplData(const IATLEnvironmentImplData* const pOldEnvironmentImplData) override;

        IATLAudioObjectData* NewGlobalAudioObjectData(const TAudioObjectID nObjectID) override;
        IATLAudioObjectData* NewAudioObjectData(const TAudioObjectID nObjectID) override;
        void DeleteAudioObjectData(IATLAudioObjectData* const pOldObjectData) override;

        IATLListenerData* NewDefaultAudioListenerObjectData(const TATLIDType nListenerID) override;
        IATLListenerData* NewAudioListenerObjectData(const TATLIDType nListenerID) override;
        void DeleteAudioListenerObjectData(IATLListenerData* const pOldListenerData) override;

        IATLEventData* NewAudioEventData(const TAudioEventID nEventID) override;
        void DeleteAudioEventData(IATLEventData* const pOldEventData) override;
        void ResetAudioEventData(IATLEventData* const pEventData) override;

        const char* const GetImplSubPath() const override;
        void SetLanguage(const char* const sLanguage) override;

        // Below data is only used when INCLUDE_AUDIO_PRODUCTION_CODE is defined!
        const char* const GetImplementationNameString() const override { return s_nosoundLongName; }
        void GetMemoryInfo(SAudioImplMemoryInfo& oMemoryInfo) const override;
        AZStd::vector<AudioImplMemoryPoolInfo> GetMemoryPoolInfo() override { return m_debugMemoryPoolInfo; }

        bool CreateAudioSource(const SAudioInputConfig& sourceConfig) override { return false; }
        void DestroyAudioSource(TAudioSourceId sourceId) override {}
        // IAudioSystemImplementation interface END

    private:
        template <typename ValueType>
        using AudioSet = AZStd::set<ValueType, AZStd::less<ValueType>, Audio::AudioImplStdAllocator>;

        template <typename KeyType, typename ValueType>
        using AudioMap = AZStd::map<KeyType, ValueType, AZStd::less<KeyType>, Audio::AudioImplStdAllocator>;

        AudioSet<IATLAudioObjectData*> m_registeredAudioObjects;
        AudioMap<const IATLTriggerImplData*, STriggerCallbackInfo*> m_registeredTriggerCallbackInfo;
        AudioSet<const IATLRtpcImplData*> m_registeredRtpcData;
        AudioSet<const IATLSwitchStateImplData*> m_registeredSwitchStateData;
        AudioSet<const IATLEnvironmentImplData*> m_registeredEnvironmentData;

        EAudioRequestStatus PrepUnprepTriggerSync(
            IATLAudioObjectData* const pAudioObject,
            const IATLTriggerImplData* const pTrigger,
            bool bPrepare);
        EAudioRequestStatus PrepUnprepTriggerAsync(
            IATLAudioObjectData* const pAudioObject,
            const IATLTriggerImplData* const pTrigger,
            IATLEventData* const pEvent,
            bool bPrepare);

        static const char* const s_nosoundShortName;
        static const char* const s_nosoundLongName;
        static const char* const s_nosoundImplSubPath;
        static const char* const s_nosoundFileTag;
        static const char* const s_nosoundNameAttr;
        static const float s_obstructionOcclusionMin;
        static const float s_obstructionOcclusionMax;

        AudioSet<IATLAudioFileEntryData*> m_soundBanks;
        AZStd::vector<AudioImplMemoryPoolInfo> m_debugMemoryPoolInfo;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Data for tracking the progress of prepare/unprepare of a trigger
    struct STriggerCallbackInfo
    {
        using GenericCallbackType = void(*)(void* cookie);
        GenericCallbackType m_prepareEventCallback;
        float m_timeElasped; // simulate the process of prepare/unprepare by taking some time
        float m_timeTotal;
        IATLEventData* m_eventData;
        bool m_isPrepared;

        STriggerCallbackInfo()
            : m_prepareEventCallback(nullptr)
            , m_timeElasped(0.f)
            , m_timeTotal(0.f)
            , m_eventData(nullptr)
            , m_isPrepared(false)
        {}

        // setup callback for prepare/unprepare trigger asynchronously
        void SendEvent(GenericCallbackType callbackFunction, IATLEventData* eventData, float timeTotal = 1.0f)
        {
            m_prepareEventCallback = callbackFunction;
            m_eventData = eventData;
            m_timeTotal = timeTotal;
            m_timeElasped = 0.0f;
        }

        void PrepareEvent()
        {
            m_isPrepared = true;
        }

        void UnprepareEvent()
        {
            m_isPrepared = false;
        }
    };
} // namespace Audio
