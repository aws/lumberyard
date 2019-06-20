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

#include <Common/IAudioSystemImplementation.h>

#pragma warning(push)
#pragma warning(disable:4373)

namespace Audio
{
    struct AudioSystemImplementationMock
        : public AudioSystemImplementationComponent
    {
    public:
        MOCK_METHOD1(Update, void(const float));

        MOCK_METHOD0(Initialize, EAudioRequestStatus());

        MOCK_METHOD0(ShutDown, EAudioRequestStatus());

        MOCK_METHOD0(Release, EAudioRequestStatus());

        MOCK_METHOD0(OnAudioSystemRefresh, void());

        MOCK_METHOD0(OnLoseFocus, EAudioRequestStatus());

        MOCK_METHOD0(OnGetFocus, EAudioRequestStatus());

        MOCK_METHOD0(MuteAll, EAudioRequestStatus());

        MOCK_METHOD0(UnmuteAll, EAudioRequestStatus());

        MOCK_METHOD0(StopAllSounds, EAudioRequestStatus());

        MOCK_METHOD2(RegisterAudioObject, EAudioRequestStatus(IATLAudioObjectData* const, const char* const));

        MOCK_METHOD1(RegisterAudioObject, EAudioRequestStatus(IATLAudioObjectData* const));

        MOCK_METHOD1(UnregisterAudioObject, EAudioRequestStatus(IATLAudioObjectData* const));

        MOCK_METHOD1(ResetAudioObject, EAudioRequestStatus(IATLAudioObjectData* const));

        MOCK_METHOD1(UpdateAudioObject, EAudioRequestStatus(IATLAudioObjectData* const));

        MOCK_METHOD2(PrepareTriggerSync, EAudioRequestStatus(IATLAudioObjectData* const, const IATLTriggerImplData* const));

        MOCK_METHOD2(UnprepareTriggerSync, EAudioRequestStatus(IATLAudioObjectData* const, const IATLTriggerImplData* const));

        MOCK_METHOD3(PrepareTriggerAsync, EAudioRequestStatus(IATLAudioObjectData* const, const IATLTriggerImplData* const, IATLEventData* const));

        MOCK_METHOD3(UnprepareTriggerAsync, EAudioRequestStatus(IATLAudioObjectData* const, const IATLTriggerImplData* const, IATLEventData* const));

        MOCK_METHOD4(ActivateTrigger, EAudioRequestStatus(IATLAudioObjectData* const, const IATLTriggerImplData* const, IATLEventData* const, const TAudioSourceId));

        MOCK_METHOD2(StopEvent, EAudioRequestStatus(IATLAudioObjectData* const, const IATLEventData* const));

        MOCK_METHOD1(StopAllEvents, EAudioRequestStatus(IATLAudioObjectData* const));

        MOCK_METHOD2(SetPosition, EAudioRequestStatus(IATLAudioObjectData* const, const SATLWorldPosition&));

        MOCK_METHOD2(SetMultiplePositions, EAudioRequestStatus(IATLAudioObjectData* const, const MultiPositionParams&));

        MOCK_METHOD3(SetRtpc, EAudioRequestStatus(IATLAudioObjectData* const, const IATLRtpcImplData* const, const float));

        MOCK_METHOD2(SetSwitchState, EAudioRequestStatus(IATLAudioObjectData* const, const IATLSwitchStateImplData* const));

        MOCK_METHOD3(SetObstructionOcclusion, EAudioRequestStatus(IATLAudioObjectData* const, const float, const float));

        MOCK_METHOD3(SetEnvironment, EAudioRequestStatus(IATLAudioObjectData* const, const IATLEnvironmentImplData* const, const float));

        MOCK_METHOD2(SetListenerPosition, EAudioRequestStatus(IATLListenerData* const, const SATLWorldPosition&));

        MOCK_METHOD1(RegisterInMemoryFile, EAudioRequestStatus(SATLAudioFileEntryInfo* const));

        MOCK_METHOD1(UnregisterInMemoryFile, EAudioRequestStatus(SATLAudioFileEntryInfo* const));

        MOCK_METHOD2(ParseAudioFileEntry, EAudioRequestStatus(const XmlNodeRef, SATLAudioFileEntryInfo* const));

        MOCK_METHOD1(DeleteAudioFileEntryData, void(IATLAudioFileEntryData* const));

        MOCK_METHOD1(GetAudioFileLocation, const char* const(SATLAudioFileEntryInfo* const));

        MOCK_METHOD1(NewAudioTriggerImplData, const IATLTriggerImplData*(const XmlNodeRef));

        MOCK_METHOD1(DeleteAudioTriggerImplData, void(const IATLTriggerImplData* const));

        MOCK_METHOD1(NewAudioRtpcImplData, const IATLRtpcImplData*(const XmlNodeRef));

        MOCK_METHOD1(DeleteAudioRtpcImplData, void(const IATLRtpcImplData* const));

        MOCK_METHOD1(NewAudioSwitchStateImplData, const IATLSwitchStateImplData*(const XmlNodeRef));

        MOCK_METHOD1(DeleteAudioSwitchStateImplData, void(const IATLSwitchStateImplData* const));

        MOCK_METHOD1(NewAudioEnvironmentImplData, const IATLEnvironmentImplData*(const XmlNodeRef));

        MOCK_METHOD1(DeleteAudioEnvironmentImplData, void(const IATLEnvironmentImplData* const));

        MOCK_METHOD1(NewGlobalAudioObjectData, IATLAudioObjectData*(const TAudioObjectID));

        MOCK_METHOD1(NewAudioObjectData, IATLAudioObjectData*(const TAudioObjectID));

        MOCK_METHOD1(DeleteAudioObjectData, void(IATLAudioObjectData* const));

        MOCK_METHOD1(NewDefaultAudioListenerObjectData, IATLListenerData*(const TATLIDType));

        MOCK_METHOD1(NewAudioListenerObjectData, IATLListenerData*(const TATLIDType));

        MOCK_METHOD1(DeleteAudioListenerObjectData, void(IATLListenerData* const));

        MOCK_METHOD1(NewAudioEventData, IATLEventData*(const TAudioEventID));

        MOCK_METHOD1(DeleteAudioEventData, void(IATLEventData* const));

        MOCK_METHOD1(ResetAudioEventData, void(IATLEventData* const));

        MOCK_METHOD1(SetLanguage, void(const char* const));

        MOCK_CONST_METHOD0(GetImplSubPath, const char* const());

        MOCK_CONST_METHOD0(GetImplementationNameString, const char* const());

        MOCK_CONST_METHOD1(GetMemoryInfo, void(SAudioImplMemoryInfo&));

        MOCK_METHOD1(CreateAudioSource, bool(const SAudioInputConfig&));

        MOCK_METHOD1(DestroyAudioSource, void(TAudioSourceId));
    };

} // namespace Audio

#pragma warning(pop)

