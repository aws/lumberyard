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
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <IProximityTriggerSystem.h>
#include <physinterface.h>

#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <AzFramework/Physics/PhysicsSystemComponentBus.h>
#include <AzFramework/Physics/ColliderComponentBus.h>

#include <LmbrCentral/Physics/CryPhysicsComponentRequestBus.h>

#include "PhysicsSystemComponent.h"
#include <Physics/BusForwarding/BusForwarding.h>

namespace AZ
{
    class Transform;
}

namespace LmbrCentral
{
    /*!
     * Base class for in-game physics components.
     * An entity with a physics component can interact with the physics simulation.
     * The physics component gets its geometry from collider components placed
     * on its entity or any descendant entity.
     */
    class PhysicsComponent
        : public AZ::Component
        , public AzFramework::PhysicsComponentRequestBus::Handler
        , public CryPhysicsComponentRequestBus::Handler
        , protected EntityPhysicsEventBus::Handler
        , protected AzFramework::PhysicsSystemEventBus::Handler
        , protected AzFramework::ColliderComponentEventBus::MultiHandler
        , protected AZ::TransformNotificationBus::MultiHandler
        , protected AZ::EntityBus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysicsComponent, AZ::SystemAllocator, 0);
        AZ_RTTI(PhysicsComponent, "{6C2A2397-C33D-4ACA-8813-42B99E7B84DB}", AZ::Component);
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PhysicsService", 0xa7350d22));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("PhysicsService", 0xa7350d22));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("ColliderService", 0x902d4e93));
        }

        ////////////////////////////////////////////////////////////////////////
        // PhysicsComponentRequests
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() override;
        void AddImpulse(const AZ::Vector3& impulse) override;
        void AddImpulseAtPoint(const AZ::Vector3& impulse, const AZ::Vector3& worldSpacePoint) override;
        void AddAngularImpulse(const AZ::Vector3& impulse) override;
        AZ::Vector3 GetVelocity() override;
        void SetVelocity(const AZ::Vector3& velocity) override;
        AZ::Vector3 GetAcceleration() override;
        AZ::Vector3 GetAngularVelocity() override;
        void SetAngularVelocity(const AZ::Vector3& angularVelocity) override;
        AZ::Vector3 GetAngularAcceleration() override;
        float GetMass() override;
        void SetMass(float mass) override;
        float GetDensity() override;
        void SetDensity(float density) override;
        float GetLinearDamping() override;
        void SetLinearDamping(float damping) override;
        float GetSleepThreshold() override;
        void  SetSleepThreshold(float threshold) override;
        float GetWaterDamping() override;
        void SetWaterDamping(float waterDamping) override;
        float GetWaterDensity() override;
        void SetWaterDensity(float waterDensity) override;
        float GetWaterResistance() override;
        void SetWaterResistance(float waterResistance) override;
        AZ::Aabb GetAabb() override;
        bool IsAwake() const override;
        void ForceAwake() override;
        void ForceAsleep() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CryPhysicsComponentRequests
        IPhysicalEntity* GetPhysicalEntity() override;
        void GetPhysicsParameters(pe_params& outParameters) override;
        void SetPhysicsParameters(const pe_params& parameters) override;
        void GetPhysicsStatus(pe_status& outStatus) override;
        void ApplyPhysicsAction(const pe_action& action, bool threadSafe) override;
        bool IsPhysicsFullyEnabled() override;
        ////////////////////////////////////////////////////////////////////////

    protected:
        ////////////////////////////////////////////////////////////////////////
        // EntityPhysicsEvents

        //! When object moves in physics simulation,
        //! update its Transform in component-land.
        void OnPostStep(const PostStep& event) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ColliderComponentEvents
        void OnColliderChanged() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TransformNotifications (listening to self and descendants)

        //! Update physics to match new position
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        //! Add collider from new descendant
        void OnChildAdded(AZ::EntityId childId) override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EntityEvents
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // PhysicsSystemEvents
        void OnPrePhysicsUpdate() override;
        void OnPostPhysicsUpdate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // Functions for subclasses to implement

        //! Configure the freshly-created IPhysicsEntity
        virtual void ConfigurePhysicalEntity() = 0;

        //! Called each time the IPhysicalEntity's collision geometry is changed.
        virtual void ConfigureCollisionGeometry() = 0;

        virtual pe_type GetPhysicsType() const = 0;
        virtual bool CanInteractWithProximityTriggers() const = 0;
        virtual bool IsEnabledInitially() const = 0;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // Helper function to update the entity's AABB
        void UpdateProximityTriggerProxyAABB();
        ////////////////////////////////////////////////////////////////////////

        //! State of a change-request made to CryPhysics.
        //!
        //! Requests to CryPhysics may or may not be queued.
        //! We wait until an OnPrePhysicsUpdate and OnPostPhysicsUpdate
        //! have gone by before assuming that a request has taken effect.
        enum class SyncState
        {
            Queued, //!< Change-request has been submitted.
            Processing, //!< Change-request is being processed.
            Synced, //!< Change-request has taken effect.
        };

        void AddCollidersFromEntityAndDescendants(const AZ::EntityId& rootEntityId);
        void AddCollidersFromEntity(const AZ::EntityId& entityId);

        //! The actual IPhysicalEntity that participates with the physics simulation.
        AZStd::shared_ptr<IPhysicalEntity> m_physicalEntity;

        //! Whether the IPhysicalEntity is completely set up.
        //! We don't consider things "fully enabled" until the IPhysicalEntity
        //! has collision geometry.
        bool m_isPhysicsFullyEnabled = false;

        bool m_isApplyingPhysicsToEntityTransform = false;
        SProximityElement* m_proximityTriggerProxy = nullptr;

        //! IDs of colliders that have contributed geometry to the IPhysicalEntity.
        AZStd::unordered_set<AZ::EntityId> m_contributingColliders;

        //! The IPhysicalEntity's next unused part ID.
        int m_nextPartId = 0;

        //! Wait until CryPhysics has processed geometry changes
        //! before calling ConfigureCollisionGeometry()
        SyncState m_changedGeometrySyncState = SyncState::Synced;

        Internal::PhysicsComponentBusForwarder m_busForwarder;
    };
} // namespace LmbrCentral
