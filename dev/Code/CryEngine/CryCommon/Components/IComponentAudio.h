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
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTAUDIO_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTAUDIO_H
#pragma once

#include <IComponent.h>
#include <ComponentType.h>
#include <IAudioSystem.h>

//! Interface for the entity's audio component.
struct IComponentAudio
    : public IComponent
{
    DECLARE_COMPONENT_TYPE("ComponentAudio", 0x67D7F233F16047C5, 0xA3EAAD93F937753F)

    IComponentAudio() {}

    ComponentEventPriority GetEventPriority(const int eventID) const override { return EntityEventPriority::Audio; }

    virtual void SetFadeDistance(const float fFadeDistance) = 0;
    virtual float GetFadeDistance() const = 0;
    virtual void SetEnvironmentFadeDistance(const float fEnvironmentFadeDistance) = 0;
    virtual float GetEnvironmentFadeDistance() const = 0;
    virtual void SetEnvironmentID(const Audio::TAudioEnvironmentID nEnvironmentID) = 0;
    virtual Audio::TAudioEnvironmentID GetEnvironmentID() const = 0;
    virtual Audio::TAudioProxyID CreateAuxAudioProxy() = 0;
    virtual bool RemoveAuxAudioProxy(const Audio::TAudioProxyID nAudioProxyLocalID) = 0;
    virtual void SetAuxAudioProxyOffset(const Audio::SATLWorldPosition& rOffset, const Audio::TAudioProxyID = DEFAULT_AUDIO_PROXY_ID) = 0;
    virtual const Audio::SATLWorldPosition& GetAuxAudioProxyOffset(const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) = 0;
    virtual bool ExecuteTrigger(const Audio::TAudioControlID nTriggerID, const ELipSyncMethod eLipSyncMethod, const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID, const Audio::SAudioCallBackInfos& rCallBackInfos = Audio::SAudioCallBackInfos::GetEmptyObject()) = 0;
    virtual void StopTrigger(const Audio::TAudioControlID nTriggerID, const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) = 0;
    virtual void StopAllTriggers() = 0;
    virtual void SetSwitchState(const Audio::TAudioControlID nSwitchID, const Audio::TAudioSwitchStateID nStateID, const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) = 0;
    virtual void SetRtpcValue(const Audio::TAudioControlID nRtpcID, const float fValue, const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) = 0;
    virtual void ResetRtpcValues(const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) = 0;
    virtual void SetObstructionCalcType(const Audio::EAudioObjectObstructionCalcType eObstructionType, const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) = 0;
    virtual void SetEnvironmentAmount(const Audio::TAudioEnvironmentID nEnvironmentID, const float fAmount, const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) = 0;
    virtual void SetCurrentEnvironments(const Audio::TAudioProxyID nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID) = 0;
    virtual void AuxAudioProxiesMoveWithEntity(const bool bCanMoveWithEntity, const Audio::TAudioProxyID nAudioProxyLocalID = INVALID_AUDIO_PROXY_ID) = 0;
    virtual void AddAsListenerToAuxAudioProxy(const Audio::TAudioProxyID nAudioProxyLocalID, Audio::AudioRequestCallbackType func, Audio::EAudioRequestType requestType = Audio::eART_AUDIO_ALL_REQUESTS, Audio::TATLEnumFlagsType specificRequestMask = ALL_AUDIO_REQUEST_SPECIFIC_TYPE_FLAGS) = 0;
    virtual void RemoveAsListenerFromAuxAudioProxy(const Audio::TAudioProxyID nAudioProxyLocalID, Audio::AudioRequestCallbackType func) = 0;
};

DECLARE_SMART_POINTERS(IComponentAudio);

#endif // CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTAUDIO_H