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
#include <MathConversion.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/any.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    class ReflectContext;
}

namespace StarterGameGem
{
    enum WaypointEventType
    {
        WET_Wait,
        WET_TurnToFace,
        WET_PlayAnim,

        WET_Invalid,
    };


    /*!
    * WaypointSettingsComponentRequests
    * Messages serviced by the WaypointSettingsComponent
    */
    class WaypointSettingsComponentRequests
        : public AZ::ComponentBus
    {
    public:
        class WaypointEventSettings
        {
        public:
            AZ_TYPE_INFO(WaypointEventSettings, "{1DC02D98-C0E4-42BC-BCF8-6195885D9298}");

            WaypointEventSettings();
            virtual ~WaypointEventSettings() {}

            static void Reflect(AZ::ReflectContext* context);

            virtual AZ::u32 MajorPropertyChanged();
            //virtual AZ::u32 MinorPropertyChanged() { return AZ_CRC("RefreshNone", 0x98a5045b); }

            int m_type;
            bool IsValid() const;

            //////////////////////////////////////////////////////
            // WaypointEventType::Wait
            float m_duration;

            bool IsDurationVisible() const;
            //////////////////////////////////////////////////////

            //////////////////////////////////////////////////////
            // WaypointEventType::TurnToFace
            bool m_turnTowardsEntity;
            AZ::EntityId m_entityToFace;
            AZ::Vector3 m_directionToFace;

            bool IsTurnTowardsEntityVisible() const;
            bool IsEntityToFaceVisible() const;
            bool IsDirectionToFaceVisible() const;
            //////////////////////////////////////////////////////

            //////////////////////////////////////////////////////
            // WaypointEventType::PlayAnim
            AZStd::string m_animName;

            bool IsAnimNameVisible() const;
            //////////////////////////////////////////////////////
        };

        virtual ~WaypointSettingsComponentRequests() {}

        //! If true, the A.I. should perform actions upon arrival.
        virtual bool HasEvents() = 0;
        //! Get the next event.
        virtual WaypointSettingsComponentRequests::WaypointEventSettings GetNextEvent(const AZ::EntityId& entityId) = 0;
    };

    typedef AZStd::vector<WaypointSettingsComponentRequests::WaypointEventSettings> VecOfEventSettings;
    using WaypointSettingsComponentRequestsBus = AZ::EBus<WaypointSettingsComponentRequests>;


    class WaypointSettingsComponent
        : public AZ::Component
        , private WaypointSettingsComponentRequestsBus::Handler
    {
    public:
        AZ_COMPONENT(WaypointSettingsComponent, "{B47180B1-BB23-4676-9AF2-60DD99C6E901}");

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        //void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        // Required Reflect function.
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("WaypointSettingsComponent", 0x1616b453));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("WaypointSettingsComponent", 0x1616b453));
        }

        //////////////////////////////////////////////////////////////////////////
        // WaypointSettingsComponentRequestsBus::Handler
        bool HasEvents() override;
        WaypointSettingsComponentRequests::WaypointEventSettings GetNextEvent(const AZ::EntityId& entityId) override;
        //////////////////////////////////////////////////////////////////////////

    private:
        struct EntityProgress
        {
            EntityProgress(const AZ::EntityId& id)
                : m_entityId(id)
                , iter(nullptr)
            {}

            AZ::EntityId m_entityId;
            VecOfEventSettings::iterator iter;
        };
        typedef AZStd::vector<WaypointSettingsComponent::EntityProgress> VecOfProgress;

        WaypointSettingsComponent::VecOfProgress::iterator FindEntitysProgress(const AZ::EntityId& entityId);


        VecOfEventSettings m_eventsOnArrival;

        VecOfProgress m_progress;
    };
} // namespace StarterGameGem
