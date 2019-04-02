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

#include <physinterface.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Quaternion.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <AzFramework/Physics/ConstraintComponentBus.h>
#include <AzCore/Component/TickBus.h>

namespace LmbrCentral
{
    enum ActivationState
    {
        Inactive,
        WaitingForGeometry,
        Active
    };

    /*!
    * Stores configuration settings for physics constraints
    */
    class ConstraintConfiguration
    {
    public:
        AZ_TYPE_INFO(ConstraintConfiguration, "{057478C4-F979-42B8-8207-005479AA673C}");

        virtual ~ConstraintConfiguration() = default;
        /**
        * The intended behavior type of the constraint
        */
        enum class ConstraintType
        {
            Hinge,
            Ball,
            Slider,
            Plane,
            Magnet,
            Fixed,
            Free
        };

        /**
        * The primary axis of the constraint
        */
        enum class Axis
        {
            Z,
            NegativeZ,
            Y,
            NegativeY,
            X,
            NegativeX
        };

        /**
        * Is the owner of the constraint the entity that contains this component?
        */
        enum class OwnerType
        {
            Self,
            OtherEntity
        };

        /**
        * Constrain to world space or another entity
        */
        enum class TargetType
        {
            Entity,
            World
        };

        //////////////////////////////////////////////////////////////////////////
        AZ::EntityId m_owningEntity;
        AZ::EntityId m_targetEntity;
        AZ::s32 m_ownerPartId = 0;
        AZ::s32 m_targetPartId = 0;

        ConstraintType m_constraintType = ConstraintType::Hinge;
        Axis m_axis = Axis::Z;

        float m_maxPullForce = 0.0f;
        float m_maxBendTorque = 0.0f;

        float m_xmin = 0.0f;
        float m_xmax = 0.0f;

        float m_yzmax = 0.0f;

        float m_damping = 0.0f;
        float m_searchRadius = 0.0f;

        OwnerType m_ownerType = OwnerType::Self;
        TargetType m_targetType = TargetType::Entity;

        bool m_enableOnActivate = true;

        bool m_enableConstrainToPartId = false;
        bool m_enableForceLimits = false;
        bool m_enableRotationLimits = false;
        bool m_enableDamping = false;
        bool m_enableSearchRadius = false;

        // Members that map directly to CryEngine flag values
        bool m_enableTargetCollisions = true;
        bool m_enableRelativeRotation = true;
        bool m_enforceOnFastObjects = true;
        bool m_breakable = true;

        //////////////////////////////////////////////////////////////////////////
        static void Reflect(AZ::ReflectContext* context);

        static AZ::Quaternion GetWorldFrame(Axis axis, const AZ::Transform& pivotTransform);

        virtual bool GetOwningEntityVisibility() const { return false; }
        virtual bool GetTargetEntityVisibility() const { return false; }

        virtual bool GetAxisVisibility() const { return false; }
        virtual bool GetEnableRotationVisibility() const { return false; }
        virtual bool GetPartIdVisibility() const { return false; }
        virtual bool GetForceLimitVisibility() const { return false; }

        virtual bool GetRotationLimitGroupVisibility() const { return false; }
        virtual bool GetRotationLimitVisibilityX() const { return false; }
        virtual bool GetRotationLimitVisibilityYZ() const { return false; }

        virtual bool GetSearchRadiusGroupVisibility() const { return false; }
        virtual bool GetSearchRadiusVisibility() const { return false; }

        virtual bool GetDampingVisibility() const { return false; }

        virtual AZ::Crc32 OnConstraintTypeChanged() { return 0; }
        virtual AZ::Crc32 OnOwnerTypeChanged() { return 0; }
        virtual AZ::Crc32 OnTargetTypeChanged() { return 0; }
        virtual AZ::Crc32 OnPropertyChanged() const { return 0; }

        virtual const char * GetXLimitUnits() const { return ""; }
    };

    /**
    * ConstraintComponent
    *
    * The Constraint Components facilitates the creation of a physics constraint between two entities or an entity and a point in the world.
    * Both constrained entities must have a component that provides the physics service.
    */
    class ConstraintComponent
        : public AZ::Component
        , private AzFramework::PhysicsComponentNotificationBus::MultiHandler
        , private AzFramework::ConstraintComponentRequestBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(ConstraintComponent, "{0D5A3F64-68E9-45EE-A73F-892A4CB5BFAF}");

        ConstraintComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // Public methods
        void SetConfiguration(const ConstraintConfiguration& config);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Component descriptor
        static void Reflect(AZ::ReflectContext* context);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        //////////////////////////////////////////////////////////////////////////

    private:
        //////////////////////////////////////////////////////////////////////////
        // LmbrCentral::PhysicsComponentNotifications
        void OnPhysicsEnabled() override;
        void OnPhysicsDisabled() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ConstraintComponentRequestBus::Handler
        void SetConstraintEntities(const AZ::EntityId& owningEntity, const AZ::EntityId& targetEntity) override;
        void SetConstraintEntitiesWithPartIds(const AZ::EntityId& owningEntity, int ownerPartId, const AZ::EntityId& targetEntity, int targetPartId) override;
        void EnableConstraint() override;
        void DisableConstraint() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Private helpers
        void EnableInternal();
        void DisableInternal();
        void OnPhysicsEnabledChanged(bool enabled, AZ::EntityId entityId);
        void SetupPivotsAndFrame(pe_action_add_constraint &aac) const;
        void AddCryConstraint();
        bool IsEnabled() const;
        //////////////////////////////////////////////////////////////////////////

        // Non-serialized runtime state
        AZ::s32 m_constraintId = 0; // The CryPhysics constraint id (0 if disabled)
        bool m_shouldBeEnabled = false; // Did we attempt to enable the constraint?
        bool m_ownerPhysicsEnabled;
        bool m_targetPhysicsEnabled;

        // Serialized members
        ConstraintConfiguration m_config;

        pe_action_add_constraint m_action_add_constraint;
        IPhysicalEntity* m_physEntity = nullptr;
        IPhysicalEntity* m_physBuddy = nullptr;
        ActivationState m_activationState = ActivationState::Inactive;
    };

} // namespace LmbrCentral
