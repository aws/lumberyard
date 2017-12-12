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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/any.h>


namespace StarterGameGem
{
    enum SoundTypes
    {
        RifleHit,               // e.g. laser hitting the ground or a wall
        WhizzPast,              // when the laser whizzes past your head
        LauncherExplosion,      // e.g. the grenade exploding
        GunShot,                // the player's weapon firing
    };

    struct SoundProperties
    {
        AZ_TYPE_INFO(SoundProperties, "{8DE61F0A-D65F-45C1-BAC9-296FF8B61A08}");
        AZ_CLASS_ALLOCATOR(SoundProperties, AZ::SystemAllocator, 0);

        void Clone(const SoundProperties& other)
        {
            origin = other.origin;
            range = other.range;
            type = other.type;
            isLineSound = other.isLineSound;
            destination = other.destination;
            closestPoint = other.closestPoint;
        }

        AZ::Vector3 origin = AZ::Vector3::CreateZero();
        float range = 0.0f;
        int type = SoundTypes::RifleHit;

        // Only used for line sounds (such as 'WhizzPast').
        bool isLineSound = false;   // set in C++
        AZ::Vector3 destination = AZ::Vector3::CreateZero();
        AZ::Vector3 closestPoint = AZ::Vector3::CreateZero();
    };

    // For tracking a sound that's been broadcast and rendering its sphere of influence.
    struct SoundPropertiesDebug
        : public SoundProperties
    {
        AZ_TYPE_INFO(SoundPropertiesDebug, "{58A91D5C-1838-4A82-86FB-4E6307D8A890}", SoundProperties);
        AZ_CLASS_ALLOCATOR(SoundPropertiesDebug, AZ::SystemAllocator, 0);

        SoundPropertiesDebug(const SoundProperties& props)
            : SoundProperties(props)
            , duration(0.0f)
        {}

        float duration;
    };

    /*!
     * Requests for the A.I. sound manager system.
     */
    class AISoundManagerSystemRequests
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        // singleton pattern
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        ////////////////////////////////////////////////////////////////////////

        virtual ~AISoundManagerSystemRequests() = default;

        // Sends the sound to any nearby entities so they can react to it.
        virtual void BroadcastSound(SoundProperties props) = 0;

        // TODO: Move to StarterGameUtility
        virtual void RegisterEntity(const AZ::EntityId& entityId) = 0;
        virtual void UnregisterEntity(const AZ::EntityId& entityId) = 0;
    };

    using AISoundManagerSystemRequestBus = AZ::EBus<AISoundManagerSystemRequests>;

    /*!
     * System component which listens for audio events.
     */
    class AISoundManagerSystemComponent
        : public AZ::Component
        , public AISoundManagerSystemRequestBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(AISoundManagerSystemComponent, "{5992B53F-C023-41F1-8241-C0E38553E08B}");

        AISoundManagerSystemComponent();

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("AISoundManagerSystemService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("AISoundManagerSystemService"));
        }

        ~AISoundManagerSystemComponent() override {}

        static AISoundManagerSystemComponent* GetInstance();

        ////////////////////////////////////////////////////////////////////////
        // AudioManagerSystemRequests
        void BroadcastSound(SoundProperties props) override;

        void RegisterEntity(const AZ::EntityId& entityId) override;
        void UnregisterEntity(const AZ::EntityId& entityId) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// AZ::TickBus interface implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //////////////////////////////////////////////////////////////////////////

        bool IsLineSound(const SoundProperties& props) const;
        void SendGameplayEvent(const AZ::EntityId& entityId, const AZStd::string& eventName, const AZStd::any& value) const;

        // TODO: Move to utility.
        AZ::Vector3 GetClosestPointOnLineSegment(const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& p) const;

        AZStd::list<AZ::EntityId> m_registeredEntities;

        AZStd::string m_broadcastSoundEventName;

        AZStd::list<SoundPropertiesDebug> m_debugSounds;

    };
} // namespace StarterGameGem
