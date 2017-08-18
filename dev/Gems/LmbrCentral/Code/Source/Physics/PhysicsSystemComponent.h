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

#include <LmbrCentral/Physics/PhysicsSystemComponentBus.h>
#include <AzCore/Component/Component.h>
#include <CrySystemBus.h>

struct IPhysicalWorld;

namespace LmbrCentral
{
    /*!
     * Internal bus for getting physics events to the PhysicsComponent
     */
    class EntityPhysicsEvents
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        ////////////////////////////////////////////////////////////////////////

        //! Data for OnPostStep event
        struct PostStep
        {
            AZ::EntityId m_entity; //!< ID of Entity involved in event
            AZ::Vector3 m_entityPosition; //!< Position of Entity
            AZ::Quaternion m_entityRotation; //!< Rotation of Entity
            float m_stepTimeDelta; //!< Time interval used in this physics step
            int m_stepId; //!< The world's internal step count
        };

        virtual ~EntityPhysicsEvents() {}

        //! The physics system has moved this Entity.
        virtual void OnPostStep(const EntityPhysicsEvents::PostStep& event) {}
    };
    using EntityPhysicsEventBus = AZ::EBus<EntityPhysicsEvents>;

    /*!
     * System component which listens for IPhysicalWorld events,
     * filters for events that involve AZ::Entities,
     * and broadcasts these events on the EntityPhysicsEventBus.
     */
    class PhysicsSystemComponent
        : public AZ::Component
        , public PhysicsSystemRequestBus::Handler
        , public CrySystemEventBus::Handler
    {
    public:
        AZ_COMPONENT(PhysicsSystemComponent, "{1586DBA1-F5F0-49AB-9F59-AE62C0E60AE0}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PhysicsSystemService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("PhysicsSystemService"));
        }

        ~PhysicsSystemComponent() override {}

    private:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // PhysicsSystemRequests
        RayCastResult RayCast(const RayCastConfiguration& rayCastConfiguration) override;
        AZStd::vector<AZ::EntityId> GatherPhysicalEntitiesInAABB(const AZ::Aabb& aabb, AZ::u32 query) override;
        AZStd::vector<AZ::EntityId> GatherPhysicalEntitiesAroundPoint(const AZ::Vector3& center, float radius, AZ::u32 query) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CrySystemEvents
        void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) override;
        void OnCrySystemShutdown(ISystem&) override;
        void OnCrySystemPrePhysicsUpdate() override;
        void OnCrySystemPostPhysicsUpdate() override;
        ////////////////////////////////////////////////////////////////////////

        void SetEnabled(bool enable);

        //! During system components' Activate()/Deactivate()
        //! gEnv->pPhysicalWorld is not available (in fact, gEnv is null).
        //! Therefore we "enable" ourselves after CrySystem has initialized.
        bool m_enabled = false;

        //! Since gEnv is unreliable for system components,
        //! maintain our own reference to gEnv->pPhysicalWorld.
        IPhysicalWorld* m_physicalWorld = nullptr;
    };
} // namespace LmbrCentral
