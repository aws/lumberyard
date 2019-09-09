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

#include <IAudioSystem.h>
#include <TimeValue.h>

struct ICVar;

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CNULLAudioProxy
        : public IAudioProxy
    {
    public:
        CNULLAudioProxy() {}
        ~CNULLAudioProxy() override {}

        void Initialize(const char* const, const bool = true) override {}
        void Release() override {}
        void Reset() override {}
        void ExecuteSourceTrigger(const TAudioControlID nTriggerID, const Audio::TAudioControlID& sourceId, const SAudioCallBackInfos & rCallbackInfos = SAudioCallBackInfos::GetEmptyObject()) override {}
        void ExecuteTrigger(const TAudioControlID, const ELipSyncMethod, const SAudioCallBackInfos& rCallbackInfos = SAudioCallBackInfos::GetEmptyObject()) override {}
        void StopAllTriggers() override {}
        void StopTrigger(const TAudioControlID) override {}
        void SetSwitchState(const TAudioControlID, const TAudioSwitchStateID) override {}
        void SetRtpcValue(const TAudioControlID, const float) override {}
        void SetObstructionCalcType(const EAudioObjectObstructionCalcType) override {}
        void SetPosition(const SATLWorldPosition&) override {}
        void SetPosition(const Vec3&) override {}
        void SetMultiplePositions(const MultiPositionParams& positions) override {}
        void SetEnvironmentAmount(const TAudioEnvironmentID, const float) override {}
        void SetCurrentEnvironments(const EntityId nEntityToIgnore = 0) override {}
        void SetLipSyncProvider(ILipSyncProvider* const) override {}
        void ResetRtpcValues() override {}
        TAudioObjectID GetAudioObjectID() const override { return INVALID_AUDIO_OBJECT_ID; }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CNULLAudioSystem
        : public IAudioSystem
    {
    public:
        CNULLAudioSystem()
        {
            AudioSystemRequestBus::Handler::BusConnect();
            AudioSystemThreadSafeRequestBus::Handler::BusConnect();
        }
        ~CNULLAudioSystem() override
        {
            AudioSystemRequestBus::Handler::BusDisconnect();
            AudioSystemThreadSafeRequestBus::Handler::BusDisconnect();
        }

        bool Initialize() override { return true; }
        void Release() override { delete this; }

        void Activate() override {}
        void Deactivate() override {}

        void ExternalUpdate() override {}

        void PushRequest(const SAudioRequest&) override {}
        void PushRequestBlocking(const SAudioRequest&) override {}
        void PushRequestThreadSafe(const SAudioRequest&) override {}
        void AddRequestListener(AudioRequestCallbackType, void* const, const EAudioRequestType, const TATLEnumFlagsType) override {}
        void RemoveRequestListener(AudioRequestCallbackType, void* const) override {}

        TAudioControlID GetAudioTriggerID(const char* const) const override { return INVALID_AUDIO_CONTROL_ID; }
        TAudioControlID GetAudioRtpcID(const char* const) const override { return INVALID_AUDIO_CONTROL_ID; }
        TAudioControlID GetAudioSwitchID(const char* const) const override { return INVALID_AUDIO_CONTROL_ID; }
        TAudioSwitchStateID GetAudioSwitchStateID(const TAudioControlID, const char* const) const override { return INVALID_AUDIO_SWITCH_STATE_ID; }
        TAudioPreloadRequestID GetAudioPreloadRequestID(const char* const) const override { return INVALID_AUDIO_PRELOAD_REQUEST_ID; }
        TAudioEnvironmentID GetAudioEnvironmentID(const char* const) const override { return INVALID_AUDIO_ENVIRONMENT_ID; }

        bool ReserveAudioListenerID(TAudioObjectID& rAudioObjectID) override { rAudioObjectID = INVALID_AUDIO_OBJECT_ID; return true; }
        bool ReleaseAudioListenerID(const TAudioObjectID) override { return true; }
        bool SetAudioListenerOverrideID(const TAudioObjectID) override { return true; }

        void OnCVarChanged(ICVar* const) override {}
        void GetInfo(SAudioSystemInfo&) override {}
        const char* GetControlsPath() const override { return ""; }
        void UpdateControlsPath() override {}
        void RefreshAudioSystem(const char* const) override {}

        IAudioProxy* GetFreeAudioProxy() override { return static_cast<IAudioProxy*>(&m_oNULLAudioProxy); }
        void FreeAudioProxy(IAudioProxy* const) override {}

        TAudioSourceId CreateAudioSource(const SAudioInputConfig& sourceConfig) override { return INVALID_AUDIO_SOURCE_ID; }
        void DestroyAudioSource(TAudioSourceId sourceId) override {}

        const char* GetAudioControlName(const EAudioControlType controlType, const TATLIDType atlID) const override { return nullptr; }
        const char* GetAudioSwitchStateName(const TAudioControlID switchID, const TAudioSwitchStateID stateID) const override { return nullptr; }

    private:
        CNULLAudioProxy m_oNULLAudioProxy;
    };

} // namespace Audio
