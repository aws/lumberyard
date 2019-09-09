/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
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
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <LmbrCentral/Animation/CharacterAnimationBus.h>

namespace Audio
{
    struct IAudioProxy;
}

namespace AudioAnimationEvents
{
    class JointAudioProxy;

    /*!
    * In-game audio animation events proxy component.
    * An entity with this component can execute audio_trigger animation events.
    * If the animation event has no associated bone, the entity position will be used.
    */
    class AudioAnimationEventsProxyComponent
        : public AZ::Component
        , protected AZ::TransformNotificationBus::Handler
        , protected LmbrCentral::CharacterAnimationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(AudioAnimationEventsProxyComponent, "{F34C4439-A98F-4023-A297-6AFF3F8DBDDA}");

    protected:
        ////////////////////////////////////////////////////////////////////////
        // TransformNotificationBus interface implementation
        void OnTransformChanged(const AZ::Transform&, const AZ::Transform&) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CharacterAnimationNotificationBus interface implementation
        void OnAnimationEvent(const LmbrCentral::AnimationEvent& event) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("AudioAnimationEventProxyService", 0x7d3dd7d5));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("AudioAnimationEventProxyService", 0x7d3dd7d5));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        bool m_tracksEntityPosition = true;
        AZ::Transform m_transform = AZ::Transform::Identity();
        AZStd::string m_audioEventName = "audio_trigger";
        std::unordered_map<int32 /*jointId*/, JointAudioProxy* /*audioProxy*/> m_jointAudioProxies;
    };
}
