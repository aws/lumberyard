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

#include "StdAfx.h"
#include "AudioTriggerComponent.h"

#include <ISystem.h>

#include <LmbrCentral/Audio/AudioProxyComponentBus.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>


namespace LmbrCentral
{
    //=========================================================================
    namespace ClassConverters
    {
        extern bool ConvertOldEditorAudioComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        extern bool ConvertOldAudioComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        extern bool ConvertOldAudioTriggerComponent(AZ::SerializeContext&, AZ::SerializeContext::DataElementNode&);

    } // namespace ClassConverters

    //=========================================================================
    // Behavior Context AudioTriggerComponentNotificationBus forwarder
    class BehaviorAudioTriggerComponentNotificationBusHandler
        : public AudioTriggerComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorAudioTriggerComponentNotificationBusHandler, "{ACCB0C42-3752-496B-9B1F-19276925EBB0}", AZ::SystemAllocator,
            OnTriggerFinished);

        void OnTriggerFinished(const Audio::TAudioControlID triggerID) override
        {
            Call(FN_OnTriggerFinished, triggerID);
        }
    };

    //=========================================================================
    void AudioTriggerComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            // Any old audio editor components need to be converted to GenericComponentWrapper-wrapped Audio Trigger components.
            serializeContext->ClassDeprecate("EditorAudioComponent", "{B238217F-1D76-4DEE-AA2B-09B7DFC6BF29}", &ClassConverters::ConvertOldEditorAudioComponent);

            // Any old audio runtime components need to be converted to Audio Trigger components.
            serializeContext->ClassDeprecate("AudioComponent", "{53033C2C-EE40-4D19-A7F4-861D6AA820EB}", &ClassConverters::ConvertOldAudioComponent);

            // Deprecate the old AudioTriggerComponent in favor of a new Editor component.
            serializeContext->ClassDeprecate("AudioTriggerComponent", "{80089838-4444-4D67-9A89-66B0276BB916}", &ClassConverters::ConvertOldAudioTriggerComponent);

            serializeContext->Class<AudioTriggerComponent, AZ::Component>()
                ->Version(1)
                ->Field("Play Trigger", &AudioTriggerComponent::m_defaultPlayTriggerName)
                ->Field("Stop Trigger", &AudioTriggerComponent::m_defaultStopTriggerName)
                ->Field("Plays Immediately", &AudioTriggerComponent::m_playsImmediately)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AudioTriggerComponentRequestBus>("AudioTriggerComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Event("Play", &AudioTriggerComponentRequestBus::Events::Play)
                ->Event("Stop", &AudioTriggerComponentRequestBus::Events::Stop)
                ->Event("ExecuteTrigger", &AudioTriggerComponentRequestBus::Events::ExecuteTrigger)
                ->Event("KillTrigger", &AudioTriggerComponentRequestBus::Events::KillTrigger)
                ->Event("KillAllTriggers", &AudioTriggerComponentRequestBus::Events::KillAllTriggers)
                ->Event("SetMovesWithEntity", &AudioTriggerComponentRequestBus::Events::SetMovesWithEntity)
                ;

            behaviorContext->EBus<AudioTriggerComponentNotificationBus>("AudioTriggerComponentNotificationBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Handler<BehaviorAudioTriggerComponentNotificationBusHandler>()
                ;
        }

    }

    //=========================================================================
    void AudioTriggerComponent::Activate()
    {
        m_callbackInfo.reset(new Audio::SAudioCallBackInfos(
            this,
            static_cast<AZ::u64>(GetEntityId()),
            nullptr,
            (Audio::eARF_PRIORITY_NORMAL | Audio::eARF_SYNC_FINISHED_CALLBACK)
        ));

        OnPlayTriggerChanged();
        OnStopTriggerChanged();

        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::AddRequestListener,
            &AudioTriggerComponent::OnAudioEvent,
            this,
            Audio::eART_AUDIO_CALLBACK_MANAGER_REQUEST,
            Audio::eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE);

        AudioTriggerComponentRequestBus::Handler::BusConnect(GetEntityId());

        if (m_playsImmediately)
        {
            // if requested, play the set trigger at Activation time...
            Play();
        }
    }

    //=========================================================================
    void AudioTriggerComponent::Deactivate()
    {
        AudioTriggerComponentRequestBus::Handler::BusDisconnect(GetEntityId());

        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::RemoveRequestListener, AudioTriggerComponent::OnAudioEvent, this);

        KillAllTriggers();

        m_callbackInfo.reset();
    }

    //=========================================================================
    AudioTriggerComponent::AudioTriggerComponent(const AZStd::string& playTriggerName, const AZStd::string& stopTriggerName, bool playsImmediately)
        : m_defaultPlayTriggerName(playTriggerName)
        , m_defaultStopTriggerName(stopTriggerName)
        , m_playsImmediately(playsImmediately)
    {
    }

    //=========================================================================
    void AudioTriggerComponent::Play()
    {
        if (m_defaultPlayTriggerID != INVALID_AUDIO_CONTROL_ID)
        {
            AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::ExecuteTrigger, m_defaultPlayTriggerID, *m_callbackInfo);
        }
    }

    //=========================================================================
    void AudioTriggerComponent::Stop()
    {
        if (m_defaultStopTriggerID == INVALID_AUDIO_CONTROL_ID)
        {
            AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::KillTrigger, m_defaultPlayTriggerID);
        }
        else
        {
            AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::ExecuteTrigger, m_defaultStopTriggerID, *m_callbackInfo);
        }
    }

    //=========================================================================
    void AudioTriggerComponent::ExecuteTrigger(const char* triggerName)
    {
        if (triggerName && triggerName[0] != '\0')
        {
            Audio::TAudioControlID triggerID = INVALID_AUDIO_CONTROL_ID;
            Audio::AudioSystemRequestBus::BroadcastResult(triggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName);
            if (triggerID != INVALID_AUDIO_CONTROL_ID)
            {
                AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::ExecuteTrigger, triggerID, *m_callbackInfo);
            }
        }
    }

    //=========================================================================
    void AudioTriggerComponent::KillTrigger(const char* triggerName)
    {
        if (triggerName && triggerName[0] != '\0')
        {
            Audio::TAudioControlID triggerID = INVALID_AUDIO_CONTROL_ID;
            Audio::AudioSystemRequestBus::BroadcastResult(triggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName);
            if (triggerID != INVALID_AUDIO_CONTROL_ID)
            {
                AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::KillTrigger, triggerID);
            }
        }
    }

    //=========================================================================
    void AudioTriggerComponent::KillAllTriggers()
    {
        AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::KillAllTriggers);
    }

    //=========================================================================
    void AudioTriggerComponent::SetMovesWithEntity(bool shouldTrackEntity)
    {
        AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::SetMovesWithEntity, shouldTrackEntity);
    }

    //=========================================================================
    // static
    void AudioTriggerComponent::OnAudioEvent(const Audio::SAudioRequestInfo* const requestInfo)
    {
        // look for the 'finished trigger instance' event
        if (requestInfo->eAudioRequestType == Audio::eART_AUDIO_CALLBACK_MANAGER_REQUEST)
        {
            const auto notificationType = static_cast<Audio::EAudioCallbackManagerRequestType>(requestInfo->nSpecificAudioRequest);
            if (notificationType == Audio::eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE)
            {
                if (requestInfo->eResult == Audio::eARR_SUCCESS)
                {
                    AZ::EntityId entityId(reinterpret_cast<AZ::u64>(requestInfo->pUserData));
                    AudioTriggerComponentNotificationBus::Event(entityId, &AudioTriggerComponentNotificationBus::Events::OnTriggerFinished, requestInfo->nAudioControlID);
                }
            }
        }
    }

    //=========================================================================
    void AudioTriggerComponent::OnPlayTriggerChanged()
    {
        m_defaultPlayTriggerID = INVALID_AUDIO_CONTROL_ID;
        if (!m_defaultPlayTriggerName.empty())
        {
            Audio::AudioSystemRequestBus::BroadcastResult(m_defaultPlayTriggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, m_defaultPlayTriggerName.c_str());
        }

        // "ChangeNotify" sends callbacks on every key press for a text field!
        // This results in a lot of failed lookups.
    }

    //=========================================================================
    void AudioTriggerComponent::OnStopTriggerChanged()
    {
        m_defaultStopTriggerID = INVALID_AUDIO_CONTROL_ID;
        if (!m_defaultStopTriggerName.empty())
        {
            Audio::AudioSystemRequestBus::BroadcastResult(m_defaultStopTriggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, m_defaultStopTriggerName.c_str());
        }
    }

} // namespace LmbrCentral
