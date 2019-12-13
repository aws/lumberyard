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

#include "FileIOHandler_wwise.h"
#include "ATLEntities_wwise.h"
#include <IAudioSystemImplementation.h>


namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioSystemImpl_wwise
        : public AudioSystemImplementationComponent
    {
    public:
        CAudioSystemImpl_wwise();
        ~CAudioSystemImpl_wwise() override;

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
            const MultiPositionParams& multiPositionParams) override;
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

        const SATLTriggerImplData_wwise* NewAudioTriggerImplData(const XmlNodeRef pAudioTriggerNode) override;
        void DeleteAudioTriggerImplData(const IATLTriggerImplData* const pOldTriggerImplData) override;

        const SATLRtpcImplData_wwise* NewAudioRtpcImplData(const XmlNodeRef pAudioRtpcNode) override;
        void DeleteAudioRtpcImplData(const IATLRtpcImplData* const pOldRtpcImplData) override;

        const SATLSwitchStateImplData_wwise* NewAudioSwitchStateImplData(const XmlNodeRef pAudioSwitchStateImplNode) override;
        void DeleteAudioSwitchStateImplData(const IATLSwitchStateImplData* const pOldAudioSwitchStateImplData) override;

        const SATLEnvironmentImplData_wwise* NewAudioEnvironmentImplData(const XmlNodeRef pAudioEnvironmentNode) override;
        void DeleteAudioEnvironmentImplData(const IATLEnvironmentImplData* const pOldEnvironmentImplData) override;

        SATLAudioObjectData_wwise* NewGlobalAudioObjectData(const TAudioObjectID nObjectID) override;
        SATLAudioObjectData_wwise* NewAudioObjectData(const TAudioObjectID nObjectID) override;
        void DeleteAudioObjectData(IATLAudioObjectData* const pOldObjectData) override;

        SATLListenerData_wwise* NewDefaultAudioListenerObjectData(const TATLIDType nObjectID) override;
        SATLListenerData_wwise* NewAudioListenerObjectData(const TATLIDType nObjectID) override;
        void DeleteAudioListenerObjectData(IATLListenerData* const pOldListenerData) override;

        SATLEventData_wwise* NewAudioEventData(const TAudioEventID nEventID) override;
        void DeleteAudioEventData(IATLEventData* const pOldEventData) override;
        void ResetAudioEventData(IATLEventData* const pEventData) override;

        const char* const GetImplSubPath() const override;
        void SetLanguage(const char* const sLanguage) override;

        // Below data is only used when INCLUDE_WWISE_IMPL_PRODUCTION_CODE is defined!
        const char* const GetImplementationNameString() const override;
        void GetMemoryInfo(SAudioImplMemoryInfo& oMemoryInfo) const override;
        AZStd::vector<AudioImplMemoryPoolInfo> GetMemoryPoolInfo() override;

        bool CreateAudioSource(const SAudioInputConfig& sourceConfig) override;
        void DestroyAudioSource(TAudioSourceId sourceId) override;

        void SetPanningMode(PanningMode mode) override;
        // ~AudioSystemImplementationRequestBus

    private:
        static const char* const sWwiseImplSubPath;
        static const char* const sWwiseEventTag;
        static const char* const sWwiseRtpcTag;
        static const char* const sWwiseSwitchTag;
        static const char* const sWwiseStateTag;
        static const char* const sWwiseRtpcSwitchTag;
        static const char* const sWwiseFileTag;
        static const char* const sWwiseAuxBusTag;
        static const char* const sWwiseValueTag;

        static const char* const sWwiseNameAttribute;
        static const char* const sWwiseValueAttribute;
        static const char* const sWwiseMutiplierAttribute;
        static const char* const sWwiseShiftAttribute;
        static const char* const sWwiseLocalisedAttribute;
        static const char* const sWwiseGlobalAudioObjectName;

        static const float sObstructionOcclusionMin;
        static const float sObstructionOcclusionMax;

        struct SEnvPairCompare
        {
            bool operator()(const AZStd::pair<const AkAuxBusID, float>& pair1, const AZStd::pair<const AkAuxBusID, float>& pair2) const;
        };

        const SATLSwitchStateImplData_wwise* ParseWwiseSwitchOrState(XmlNodeRef pNode, EWwiseSwitchType eType);
        const SATLSwitchStateImplData_wwise* ParseWwiseRtpcSwitch(XmlNodeRef pNode);
        void ParseRtpcImpl(XmlNodeRef pNode, AkRtpcID& rAkRtpcID, float& rMult, float& rShift);
        EAudioRequestStatus PrepUnprepTriggerSync(
            const IATLTriggerImplData* const pTriggerData,
            bool bPrepare);
        EAudioRequestStatus PrepUnprepTriggerAsync(
            const IATLTriggerImplData* const pTriggerData,
            IATLEventData* const pEventData,
            bool bPrepare);

        EAudioRequestStatus PostEnvironmentAmounts(IATLAudioObjectData* const pAudioObjectData);

        AkGameObjectID m_globalGameObjectID;
        AkGameObjectID m_defaultListenerGameObjectID;
        AkBankID m_nInitBankID;
        CFileIOHandler_wwise m_oFileIOHandler;

        CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> m_sRegularSoundBankFolder;
        CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> m_sLocalizedSoundBankFolder;

#if !defined(WWISE_FOR_RELEASE)
        bool m_bCommSystemInitialized;
#endif // !WWISE_FOR_RELEASE

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
        AZStd::vector<AudioImplMemoryPoolInfo> m_debugMemoryPoolInfo;
        AZStd::string m_fullImplString;
        AZStd::string m_speakerConfigString;
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
    };
} // namespace Audio
