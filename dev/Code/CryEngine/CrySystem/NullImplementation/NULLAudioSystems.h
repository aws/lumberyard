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
#include <IMusicSystem.h>
#include <TimeValue.h>

struct ICVar;

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CNULLMusicLogic
        : public IMusicLogic
    {
    public:
        CNULLMusicLogic() {}
        ~CNULLMusicLogic() override {}

        bool Init() override { return true; }
        void PostInit() {} // Not virtual!
        void Reset() override {}
        bool Start(const CCryNameCRC&) override { return true; }
        bool Stop() override { return true; }
        bool IsActive() const override { return true; }
        void Pause(const bool) override {}
        void Update(const CTimeValue&) override {}
        void SetEvent(EMusicLogicEvents, const float = 0.0f) override {}
        void GetMemoryStatistics(ICrySizer*) override {}
        void Serialize(TSerialize) override {}
        bool SetPreset(const CCryNameCRC&) override { return true; }
        void SendEvent(TMusicLogicEventId, float = 0.f) override {}
        TMusicLogicEventId GetEventId(const char*) override { return MUSICLOGIC_INVALID_EVENT_ID; }
        void Reload() override {}
        TMusicLogicInputId GetInputId(const char*) const { return MUSICLOGIC_INVALID_INPUT_ID; }
        float GetInputValue(TMusicLogicInputId) const { return 0.f; }

#if !defined(_RELEASE)
        void DrawInformation(IRenderer*, float, float, int) override {}
#endif // _RELEASE
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CNULLMusicSystem
        : public IMusicSystem
    {
    public:
        CNULLMusicSystem() {}
        ~CNULLMusicSystem() override {}

        void PostInit() override {}
        void Release() override {}
        void Update() override {}
        void ClearResources() override {}

        int GetBytesPerSample() override { return 4; }
        IMusicSystemSink* SetSink(IMusicSystemSink*) override { return nullptr; }
        bool SetData(SMusicInfo::Data*) override { return true; }
        void Unload() override {}
        void Pause(bool) override {}
        ILINE bool IsPaused() const override { return false; }
        void EnableEventProcessing(bool) override {}
        void OverrideNativeBackgroundMusicPlayer(const bool) override {}
        const bool IsOverrideNativeBackgroundMusicPlayer() const override { return false; }

        // Theme stuff
        bool SetTheme(const char*, bool = true, bool = true, int = 0, const char* const = nullptr, const IMusicSystemEventListener* const = nullptr) override { return true; }
        bool EndTheme(EThemeFadeType = EThemeFade_FadeOut, int = 10, bool = true) override { return true; }
        const char* GetTheme() const override { return ""; }
        CTimeValue GetThemeTime() const override { return m_TimeValue; }

        // Mood stuff
        bool SetMood(const char*, const bool, const bool, const IMusicSystemEventListener* const = nullptr) override { return true; }
        bool SetDefaultMood(const char*) override { return true; }
        const char* GetMood() const override { return ""; }
        CCryNameCRC GetMoodNameCRC() const override { return ""; }
        IStringItVec* GetThemes() const override { return nullptr; }
        IStringItVec* GetMoods(const char*) const override { return nullptr; }
        IStringItVec* const GetPatterns() const override { return nullptr; }
        bool AddMusicMoodEvent(const char*, float) override { return true; }
        CTimeValue GetMoodTime() const override { return m_TimeValue; }

        SMusicSystemStatus* GetStatus() override { return nullptr; }
        void GetMemoryUsage(class ICrySizer*) const override {}
        bool LoadMusicDataFromLUA(IScriptSystem*, const char*) { return true; }
        virtual void LogMsg(const int, const char*, ...) PRINTF_PARAMS(3, 4) {
        }                                                                   // override and PRINTF_PARAMS breaks on Android!
        bool IsNullImplementation() const override { return true; }
        bool IsPlaying() const override { return false; }
        bool IsPatternFading() const override { return false; }
        void SetAutoEndThemeBehavior(EAutoEndThemeBehavior) override {}

        float GetUpdateMilliseconds() override { return 0.f; }

        //////////////////////////////////////////////////////////////////////////
        // Editing support.
        //////////////////////////////////////////////////////////////////////////
        void RenamePattern(const char*, const char*) override {}
        void UpdatePattern(SMusicInfo::Pattern*) override {}
        void PlayPattern(const char* const, const bool, const bool, const bool, const IMusicSystemEventListener* const = nullptr) override {}
        bool StopPattern(const char* const, const IMusicSystemEventListener* const = nullptr) override { return false; }
        void DeletePattern(const char*) override {}
        const char* const GetPatternNameTrack1() const override { return nullptr; }
        int SetListenerOnPlayingPattern(const char* const, const IMusicSystemEventListener* const) { return 0; }
        void Silence() override {}
        bool LoadFromXML(const char* const, const bool) override { return true; }
        bool LoadGameType(const char* const) override { return true; }

        void PlayStinger() override {}

#ifndef _RELEASE
        // Production only stuff
        void DrawInformation(IRenderer* const, float, float) override {}
#endif // _RELEASE

        // For serialization where the called passed its own TSerialize object for serialization.
        // bPauseMusicSystem if true will pause the music system when reading.
        void Serialize(TSerialize, const bool = true) override {}

        // For serialization where the called doesn't provide a TSerialize object for serialization.
        void Serialize(const bool) override {}

        // Summary:
        //   Registers listener to the music system.
        void AddEventListener(IMusicSystemEventListener* const) override {}

        // Summary:
        //   Removes listener to the music system.
        void RemoveEventListener(IMusicSystemEventListener* const) override {}

        // Summary:
        // To get music logic related data.
        IMusicLogic* const GetMusicLogic() const override { return (IMusicLogic* const)&m_oNULLMusicLogic; }

        // Summary:
        // This music pattern instance will be preloaded so it plays immediately when called.
        bool PreloadPatternInstance(const char* const) override { return false; }

        // Summary:
        // To remove all preloaded music patterns.
        void ClearPreloadedPatterns() override {}

    protected:
        CTimeValue m_TimeValue;
        CNULLMusicLogic m_oNULLMusicLogic;
    };

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
        void ExecuteTrigger(TAudioControlID const, ELipSyncMethod const, SAudioCallBackInfos const& rCallbackInfos = SAudioCallBackInfos::GetEmptyObject()) override {}
        void StopAllTriggers() override {}
        void StopTrigger(const TAudioControlID) override {}
        void SetSwitchState(const TAudioControlID, const TAudioSwitchStateID) override {}
        void SetRtpcValue(const TAudioControlID, const float) override {}
        void SetObstructionCalcType(const EAudioObjectObstructionCalcType) override {}
        void SetPosition(const SATLWorldPosition&) override {}
        void SetPosition(const Vec3&) override {}
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
        #if defined(DEDICATED_SERVER)
            AudioSystemRequestBus::Handler::BusConnect();
            AudioSystemThreadSafeRequestBus::Handler::BusConnect();
        #endif // DEDICATED_SERVER
        }
        ~CNULLAudioSystem() override
        {
        #if defined(DEDICATED_SERVER)
            AudioSystemRequestBus::Handler::BusDisconnect();
            AudioSystemThreadSafeRequestBus::Handler::BusDisconnect();
        #endif // DEDICATED_SERVER
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

        IAudioProxy* GetFreeAudioProxy() override { return static_cast<IAudioProxy*>(&m_oNULLAudioProxy); }
        void FreeAudioProxy(IAudioProxy* const) override {}

        TAudioSourceId CreateAudioSource(const SAudioInputConfig& sourceConfig) override { return INVALID_AUDIO_SOURCE_ID; }
        void DestroyAudioSource(TAudioSourceId sourceId) override {}

        const char* GetAudioControlName(const EAudioControlType controlType, const TATLIDType atlID) const override { return nullptr; }
        const char* GetAudioSwitchStateName(const TAudioControlID switchID, const TAudioSwitchStateID stateID) const override { return nullptr; }

    private:
        CNULLAudioProxy m_oNULLAudioProxy;
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Note: The purpose of this class is to Warn and Forward whenever anyone uses the deprecated
    // gEnv->pAudioSystem-> to make calls to the Audio System.
    // 2016/12/15
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioSystemDeprecationShim
        : public IAudioSystem
    {
    public:
        AudioSystemDeprecationShim() = default;
        ~AudioSystemDeprecationShim() override = default;

        bool Initialize() override
        {
            // No forwarding
            return true;
        }

        void Release() override
        {
            // No forwarding
            delete this;
        }

        void Activate() override {}
        void Deactivate() override {}

        void ExternalUpdate() override
        {
            WarnOnDeprecatedCallUsed("ExternalUpdate");
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
        }

        void PushRequest(const SAudioRequest& audioRequestData) override
        {
            WarnOnDeprecatedCallUsed("PushRequest");
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, audioRequestData);
        }

        void PushRequestBlocking(const SAudioRequest& audioRequestData) override
        {
            WarnOnDeprecatedCallUsed("PushRequestBlocking");
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, audioRequestData);
        }

        void PushRequestThreadSafe(const SAudioRequest& audioRequestData) override
        {
            WarnOnDeprecatedCallUsed("PushRequestThreadSafe");
            AudioSystemThreadSafeRequestBus::Broadcast(&AudioSystemThreadSafeRequestBus::Events::PushRequestThreadSafe, audioRequestData);
        }

        void AddRequestListener(
            AudioRequestCallbackType callBack,
            void* const objectToListenTo,
            const EAudioRequestType requestType = eART_AUDIO_ALL_REQUESTS,
            const TATLEnumFlagsType specificRequestMask = ALL_AUDIO_REQUEST_SPECIFIC_TYPE_FLAGS) override
        {
            WarnOnDeprecatedCallUsed("AddRequestListener");
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::AddRequestListener, callBack, objectToListenTo, requestType, specificRequestMask);
        }

        void RemoveRequestListener(
            AudioRequestCallbackType callBack,
            void* const requestOwner) override
        {
            WarnOnDeprecatedCallUsed("RemoveRequestListener");
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::RemoveRequestListener, callBack, requestOwner);
        }

        TAudioControlID GetAudioTriggerID(const char* const audioTriggerName) const override
        {
            WarnOnDeprecatedCallUsed("GetAudioTriggerID");
            TAudioControlID triggerID = INVALID_AUDIO_CONTROL_ID;
            AudioSystemRequestBus::BroadcastResult(triggerID, &AudioSystemRequestBus::Events::GetAudioTriggerID, audioTriggerName);
            return triggerID;
        }

        TAudioControlID GetAudioRtpcID(const char* const audioRtpcName) const override
        {
            WarnOnDeprecatedCallUsed("GetAudioRtpcID");
            TAudioControlID rtpcID = INVALID_AUDIO_CONTROL_ID;
            AudioSystemRequestBus::BroadcastResult(rtpcID, &AudioSystemRequestBus::Events::GetAudioRtpcID, audioRtpcName);
            return rtpcID;
        }

        TAudioControlID GetAudioSwitchID(const char* const audioSwitchName) const override
        {
            WarnOnDeprecatedCallUsed("GetAudioSwitchID");
            TAudioControlID switchID = INVALID_AUDIO_CONTROL_ID;
            AudioSystemRequestBus::BroadcastResult(switchID, &AudioSystemRequestBus::Events::GetAudioSwitchID, audioSwitchName);
            return switchID;
        }

        TAudioSwitchStateID GetAudioSwitchStateID(const TAudioControlID switchID, const char* const audioSwitchStateName) const override
        {
            WarnOnDeprecatedCallUsed("GetAudioSwitchStateID");
            TAudioSwitchStateID stateID = INVALID_AUDIO_SWITCH_STATE_ID;
            AudioSystemRequestBus::BroadcastResult(stateID, &AudioSystemRequestBus::Events::GetAudioSwitchStateID, switchID, audioSwitchStateName);
            return stateID;
        }

        TAudioPreloadRequestID GetAudioPreloadRequestID(const char* const audioPreloadRequestName) const override
        {
            WarnOnDeprecatedCallUsed("GetAudioPreloadRequestID");
            TAudioPreloadRequestID preloadID = INVALID_AUDIO_PRELOAD_REQUEST_ID;
            AudioSystemRequestBus::BroadcastResult(preloadID, &AudioSystemRequestBus::Events::GetAudioPreloadRequestID, audioPreloadRequestName);
            return preloadID;
        }

        TAudioEnvironmentID GetAudioEnvironmentID(const char* const audioEnvironmentName) const override
        {
            WarnOnDeprecatedCallUsed("GetAudioEnvironmentID");
            TAudioEnvironmentID environmentID = INVALID_AUDIO_ENVIRONMENT_ID;
            AudioSystemRequestBus::BroadcastResult(environmentID, &AudioSystemRequestBus::Events::GetAudioEnvironmentID, audioEnvironmentName);
            return environmentID;
        }

        bool ReserveAudioListenerID(TAudioObjectID& audioListenerID) override
        {
            WarnOnDeprecatedCallUsed("ReserveAudioListenerID");
            bool result = false;
            AudioSystemRequestBus::BroadcastResult(result, &AudioSystemRequestBus::Events::ReserveAudioListenerID, audioListenerID);
            return result;
        }

        bool ReleaseAudioListenerID(const TAudioObjectID audioListenerID) override
        {
            WarnOnDeprecatedCallUsed("ReleaseAudioListenerID");
            bool result = false;
            AudioSystemRequestBus::BroadcastResult(result, &AudioSystemRequestBus::Events::ReleaseAudioListenerID, audioListenerID);
            return result;
        }

        bool SetAudioListenerOverrideID(const TAudioObjectID audioListenerID) override
        {
            WarnOnDeprecatedCallUsed("SetAudioListenerOverrideID");
            bool result = false;
            AudioSystemRequestBus::BroadcastResult(result, &AudioSystemRequestBus::Events::SetAudioListenerOverrideID, audioListenerID);
            return result;
        }

        void GetInfo(SAudioSystemInfo& audioSystemInfo) override
        {
            WarnOnDeprecatedCallUsed("GetInfo");
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::GetInfo, audioSystemInfo);
        }

        const char* GetControlsPath() const override
        {
            WarnOnDeprecatedCallUsed("GetControlsPath");
            const char* path = "";
            AudioSystemRequestBus::BroadcastResult(path, &AudioSystemRequestBus::Events::GetControlsPath);
            return path;
        }

        void UpdateControlsPath() override
        {
            WarnOnDeprecatedCallUsed("UpdateControlsPath");
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::UpdateControlsPath);
        }

        IAudioProxy* GetFreeAudioProxy() override
        {
            WarnOnDeprecatedCallUsed("GetFreeAudioProxy");
            IAudioProxy* audioProxy = nullptr;
            AudioSystemRequestBus::BroadcastResult(audioProxy, &AudioSystemRequestBus::Events::GetFreeAudioProxy);
            return audioProxy;
        }

        void FreeAudioProxy(IAudioProxy* const audioProxy) override
        {
            WarnOnDeprecatedCallUsed("FreeAudioProxy");
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::FreeAudioProxy, audioProxy);
        }

        TAudioSourceId CreateAudioSource(const SAudioInputConfig& sourceConfig) override
        {
            WarnOnDeprecatedCallUsed("CreateAudioSource");
            TAudioSourceId sourceId = INVALID_AUDIO_SOURCE_ID;
            AudioSystemRequestBus::BroadcastResult(sourceId, &AudioSystemRequestBus::Events::CreateAudioSource, sourceConfig);
            return sourceId;
        }

        void DestroyAudioSource(TAudioSourceId sourceId) override
        {
            WarnOnDeprecatedCallUsed("DestroyAudioSource");
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::DestroyAudioSource, sourceId);
        }

        const char* GetAudioControlName(const EAudioControlType controlType, const TATLIDType atlID) const override
        {
            WarnOnDeprecatedCallUsed("GetAudioControlName");
            const char* name = nullptr;
            AudioSystemRequestBus::BroadcastResult(name, &AudioSystemRequestBus::Events::GetAudioControlName, controlType, atlID);
            return name;
        }

        const char* GetAudioSwitchStateName(const TAudioControlID switchID, const TAudioSwitchStateID stateID) const override
        {
            WarnOnDeprecatedCallUsed("GetAudioSwitchStateName");
            const char* name = nullptr;
            AudioSystemRequestBus::BroadcastResult(name, &AudioSystemRequestBus::Events::GetAudioSwitchStateName, switchID, stateID);
            return name;
        }

        void OnCVarChanged(ICVar* const cvar) override
        {
            WarnOnDeprecatedCallUsed("OnCVarChanged");
            // No forwarding
        }

    private:
        static void WarnOnDeprecatedCallUsed(const char* functionName)
        {
            AZ_Warning("AudioSystem", false, "Usage of gEnv->pAudioSystem->%s(...) is deprecated and may be removed in the future!  Please update your client code to use the EBus calling convention.  (See class implementation of AudioSystemDeprecationShim for examples)", functionName);
        }
    };

} // namespace Audio
