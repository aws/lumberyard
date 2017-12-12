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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/any.h>

#include <GameplayEventBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace StarterGameGem
{
    struct AudioEvent
    {
        AZ_RTTI(AudioEvent, "{1E982258-BB3D-400A-A5BA-CE1A62663160}");

        static void Reflect(AZ::ReflectContext* context);

        bool IsValid() const;

        // TODO: Convert these to Crc32s in Activate()?
        AZStd::string m_eventName;
        AZStd::string m_audioToPlay;
        bool m_stopPlaying;

        AZ::GameplayNotificationId m_notificationId;
    };


    class AudioMultiListenerComponent
        : public AZ::Component
        , public AZ::GameplayNotificationBus::MultiHandler
    {
    public:
        AZ_COMPONENT(AudioMultiListenerComponent, "{062C4525-BB4E-49B3-8406-B16BFCCF1513}");

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        // Required Reflect function.
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("AudioMultiListenerService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("AudioTriggerService"));
        }

        //////////////////////////////////////////////////////////////////////////
        // AZ::GameplayNotificationBus
        void OnEventBegin(const AZStd::any& value) override;
        void OnEventUpdating(const AZStd::any& value) override;
        void OnEventEnd(const AZStd::any& value) override;

    protected:
        void ReactToEvent(const AZStd::any& value);

        typedef AZStd::vector<AudioEvent> VecOfAudioEvents;
        VecOfAudioEvents m_events;

    };

} // namespace StarterGameGem
