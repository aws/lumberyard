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
#include "StarterGameGem_precompiled.h"
#include "AudioMultiListenerComponent.h"

#include "StarterGameUtility.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include <LmbrCentral/Audio/AudioTriggerComponentBus.h>

namespace StarterGameGem
{
    //-------------------------------------------
    // AudioEvent
    //-------------------------------------------
    void AudioEvent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AudioEvent>()
                ->Version(1)
                ->Field("EventName", &AudioEvent::m_eventName)
                ->Field("AudioToPlay", &AudioEvent::m_audioToPlay)
                ->Field("StopPlaying", &AudioEvent::m_stopPlaying)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<AudioEvent>("Audio Event", "Plays audio when event is heard")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(0, &AudioEvent::m_eventName, "Event Name", "The event name to listen for.")
                    ->DataElement(0, &AudioEvent::m_audioToPlay, "Audio", "The audio to play.")
                    ->DataElement(0, &AudioEvent::m_stopPlaying, "Stop?", "Tick to stop (unticked means play).")
                    ;
            }
        }
    }

    bool AudioEvent::IsValid() const
    {
        return m_eventName != "" && m_audioToPlay != "";
    }

    //-------------------------------------------
    // AudioMultiListenerComponent
    //-------------------------------------------
    void AudioMultiListenerComponent::Init()
    {
    }

    void AudioMultiListenerComponent::Activate()
    {
        // Listen for the events.
        AZ::EntityId myId = GetEntityId();
        for (VecOfAudioEvents::iterator it = m_events.begin(); it != m_events.end(); ++it)
        {
            AudioEvent& e = (*it);

            if (!e.IsValid())
            {
                continue;
            }

            e.m_notificationId = AZ::GameplayNotificationId(myId, AZ::Crc32(e.m_eventName.c_str()), StarterGameUtility::GetUuid("float"));
            AZ::GameplayNotificationBus::MultiHandler::BusConnect(e.m_notificationId);
        }
    }

    void AudioMultiListenerComponent::Deactivate()
    {
        // Stop listening for the events.
        for (VecOfAudioEvents::iterator it = m_events.begin(); it != m_events.end(); ++it)
        {
            AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(it->m_notificationId);
        }
    }

    void AudioMultiListenerComponent::Reflect(AZ::ReflectContext* context)
    {
        AudioEvent::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AudioMultiListenerComponent>()
                ->Version(1)
                ->Field("Events", &AudioMultiListenerComponent::m_events)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<AudioMultiListenerComponent>("Audio Multi-Listener", "Listens for GameplayEvents to trigger audio.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Game")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AudioMultiListenerComponent::m_events, "Events", "List of audio events.")
                    ;
            }
        }
    }

    void AudioMultiListenerComponent::OnEventBegin(const AZStd::any& value)
    {
        this->ReactToEvent(value);
    }

    void AudioMultiListenerComponent::OnEventUpdating(const AZStd::any& value)
    {
        this->ReactToEvent(value);
    }

    void AudioMultiListenerComponent::OnEventEnd(const AZStd::any& value)
    {
        this->ReactToEvent(value);
    }

    void AudioMultiListenerComponent::ReactToEvent(const AZStd::any& value)
    {
        for (VecOfAudioEvents::iterator it = m_events.begin(); it != m_events.end(); ++it)
        {
            AudioEvent& e = (*it);

            if (!e.IsValid())
            {
                continue;
            }

            if (*AZ::GameplayNotificationBus::GetCurrentBusId() == e.m_notificationId)
            {
                void (LmbrCentral::AudioTriggerComponentRequests::*funcPtr)(const char*);
                funcPtr = e.m_stopPlaying ? &LmbrCentral::AudioTriggerComponentRequests::KillTrigger : &LmbrCentral::AudioTriggerComponentRequests::ExecuteTrigger;
                LmbrCentral::AudioTriggerComponentRequestBus::Event(GetEntityId(), funcPtr, e.m_audioToPlay.c_str());
                // Don't break the for loop because we may have multiple sounds listening for the
                // same event.
            }
        }
    }

}
