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

#include "LmbrCentral_precompiled.h"
#include "AudioPreloadComponent.h"

#include <ISystem.h>
#include <IAudioSystem.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>


namespace LmbrCentral
{
    //=========================================================================
    void AudioPreloadComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AudioPreloadComponent, AZ::Component>()
                ->Version(1)
                ->Field("Preload Name", &AudioPreloadComponent::m_defaultPreloadName)
                ->Field("Load Type", &AudioPreloadComponent::m_loadType)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Enum<static_cast<AZ::u32>(AudioPreloadComponent::LoadType::Auto)>("AudioPreloadComponentLoadType_Auto")
                ->Enum<static_cast<AZ::u32>(AudioPreloadComponent::LoadType::Manual)>("AudioPreloadComponentLoadType_Manual")
                ;

            behaviorContext->EBus<AudioPreloadComponentRequestBus>("AudioPreloadComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Event("Load", &AudioPreloadComponentRequestBus::Events::Load)
                ->Event("Unload", &AudioPreloadComponentRequestBus::Events::Unload)
                ->Event("LoadPreload", &AudioPreloadComponentRequestBus::Events::LoadPreload)
                ->Event("UnloadPreload", &AudioPreloadComponentRequestBus::Events::UnloadPreload)
                ->Event("IsLoaded", &AudioPreloadComponentRequestBus::Events::IsLoaded)
                ;
        }
    }

    //=========================================================================
    AudioPreloadComponent::AudioPreloadComponent(AudioPreloadComponent::LoadType loadType, const AZStd::string& preloadName)
        : m_loadType(loadType)
        , m_defaultPreloadName(preloadName)
    {
    }

    //=========================================================================
    void AudioPreloadComponent::Activate()
    {
        if (m_loadType == AudioPreloadComponent::LoadType::Auto)
        {
            Load();     // load the default preload (if any)
        }

        AudioPreloadComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    //=========================================================================
    void AudioPreloadComponent::Deactivate()
    {
        AudioPreloadComponentRequestBus::Handler::BusDisconnect(GetEntityId());

        if (m_loadType == AudioPreloadComponent::LoadType::Auto)
        {
            Unload();       // unload the default preload (if any)

            // Copy the preloads names first so we can iterate the copy while modifying the real list
            AZStd::unordered_set<AZStd::string> preloadsToUnload = m_loadedPreloads;

            for (auto& preload : preloadsToUnload)
            {
                UnloadPreload(preload.c_str());     // unload any remaining preloads
            }
        }
        else
        {
            AZStd::string preloadsRemaining;
            for (auto& preload : m_loadedPreloads)
            {
                preloadsRemaining += "\n\t";
                preloadsRemaining += preload;
            }

            AZ_Warning("AudioPreloadComponent", m_loadedPreloads.empty(),
                "A Manual-mode AudioPreloadComponent is Deactivating and has %d remaining loaded preloads:%s\nBe sure to match all manual 'LoadPreload's with an 'UnloadPreload'!",
                m_loadedPreloads.size(), preloadsRemaining.c_str());
        }
    }

    //=========================================================================
    void AudioPreloadComponent::Load()
    {
        if (!m_defaultPreloadName.empty())
        {
            LoadPreload(m_defaultPreloadName.c_str());
        }
    }

    //=========================================================================
    void AudioPreloadComponent::Unload()
    {
        if (!m_defaultPreloadName.empty())
        {
            UnloadPreload(m_defaultPreloadName.c_str());
        }
    }

    //=========================================================================
    void AudioPreloadComponent::LoadPreload(const char* preloadName)
    {
        if (IsLoaded(preloadName))
        {
            return;
        }

        Audio::SAudioRequest request;
        Audio::TAudioPreloadRequestID preloadRequestId = INVALID_AUDIO_PRELOAD_REQUEST_ID;
        Audio::AudioSystemRequestBus::BroadcastResult(preloadRequestId, &Audio::AudioSystemRequestBus::Events::GetAudioPreloadRequestID, preloadName);

        if (preloadRequestId != INVALID_AUDIO_PRELOAD_REQUEST_ID)
        {
            Audio::SAudioManagerRequestData<Audio::eAMRT_PRELOAD_SINGLE_REQUEST> requestData(preloadRequestId);
            request.nFlags = (Audio::eARF_PRIORITY_NORMAL);
            request.pData = &requestData;
            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, request);

            m_loadedPreloads.insert(AZStd::string(preloadName));
        }
    }

    //=========================================================================
    void AudioPreloadComponent::UnloadPreload(const char* preloadName)
    {
        if (!IsLoaded(preloadName))
        {
            return;
        }

        Audio::SAudioRequest request;
        Audio::TAudioPreloadRequestID preloadRequestId = INVALID_AUDIO_PRELOAD_REQUEST_ID;
        Audio::AudioSystemRequestBus::BroadcastResult(preloadRequestId, &Audio::AudioSystemRequestBus::Events::GetAudioPreloadRequestID, preloadName);

        if (preloadRequestId != INVALID_AUDIO_PRELOAD_REQUEST_ID)
        {
            Audio::SAudioManagerRequestData<Audio::eAMRT_UNLOAD_SINGLE_REQUEST> requestData(preloadRequestId);
            request.nFlags = (Audio::eARF_PRIORITY_NORMAL);
            request.pData = &requestData;
            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, request);

            m_loadedPreloads.erase(AZStd::string(preloadName));
        }
    }

    //=========================================================================
    bool AudioPreloadComponent::IsLoaded(const char* preloadName)
    {
        return (m_loadedPreloads.find(AZStd::string(preloadName)) != m_loadedPreloads.end());
    }

} // namespace LmbrCentral
