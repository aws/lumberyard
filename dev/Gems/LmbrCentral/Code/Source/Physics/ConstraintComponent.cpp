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

#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/GameEntityContextBus.h>

#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <LmbrCentral/Physics/CryPhysicsComponentRequestBus.h>
#include <IPhysics.h>
#include <MathConversion.h>

#include "ConstraintComponent.h"

namespace LmbrCentral
{
    using AzFramework::ConstraintComponentNotificationBus;
    using AzFramework::ConstraintComponentRequestBus;
    using AzFramework::PhysicsComponentNotificationBus;

    //=========================================================================
    // ConstraintConfiguration::Reflect
    //=========================================================================
    void ConstraintConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ConstraintConfiguration>()
                ->Version(1)

                ->Field("Type", &ConstraintConfiguration::m_constraintType)

                ->Field("OwnerType", &ConstraintConfiguration::m_ownerType)
                ->Field("TargetType", &ConstraintConfiguration::m_targetType)

                ->Field("Owner", &ConstraintConfiguration::m_owningEntity)
                ->Field("Target", &ConstraintConfiguration::m_targetEntity)

                ->Field("Axis", &ConstraintConfiguration::m_axis)

                ->Field("EnableOnActivate", &ConstraintConfiguration::m_enableOnActivate)

                ->Field("EnablePartIds", &ConstraintConfiguration::m_enableConstrainToPartId)
                ->Field("EnableForceLimits", &ConstraintConfiguration::m_enableForceLimits)
                ->Field("EnableRotationLimits", &ConstraintConfiguration::m_enableRotationLimits)
                ->Field("EnableDamping", &ConstraintConfiguration::m_enableDamping)
                ->Field("EnableSearchRadius", &ConstraintConfiguration::m_enableSearchRadius)

                ->Field("OwnerPartId", &ConstraintConfiguration::m_ownerPartId)
                ->Field("TargetPartId", &ConstraintConfiguration::m_targetPartId)

                ->Field("MaxPullForce", &ConstraintConfiguration::m_maxPullForce)
                ->Field("MaxBendTorque", &ConstraintConfiguration::m_maxBendTorque)
                ->Field("XMin", &ConstraintConfiguration::m_xmin)
                ->Field("XMax", &ConstraintConfiguration::m_xmax)
                ->Field("YZMax", &ConstraintConfiguration::m_yzmax)
                ->Field("Damping", &ConstraintConfiguration::m_damping)
                ->Field("SearchRadius", &ConstraintConfiguration::m_searchRadius)

                ->Field("EnableCollision", &ConstraintConfiguration::m_enableTargetCollisions)
                ->Field("EnableRelativeRotation", &ConstraintConfiguration::m_enableRelativeRotation)
                ->Field("EnforceOnFastMovement", &ConstraintConfiguration::m_enforceOnFastObjects)
                ->Field("Breakable", &ConstraintConfiguration::m_breakable)
            ;
        }
    }

    //=========================================================================
    // ConstraintConfiguration::GetWorldFrame
    //=========================================================================
    AZ::Quaternion ConstraintConfiguration::GetWorldFrame(Axis axis, const AZ::Transform& pivotTransform)
    {
        AZ::Vector3 desiredAxis;
        switch (axis)
        {
            case Axis::Z :
            {
                desiredAxis = AZ::Vector3::CreateAxisZ(1.0f);
                break;
            }
            case Axis::NegativeZ :
            {
                desiredAxis = AZ::Vector3::CreateAxisZ(-1.0f);
                break;
            }
            case Axis::Y :
            {
                desiredAxis = AZ::Vector3::CreateAxisY(1.0f);
                break;
            }
            case Axis::NegativeY :
            {
                desiredAxis = AZ::Vector3::CreateAxisY(-1.0f);
                break;
            }
            case Axis::X :
            {
                desiredAxis = AZ::Vector3::CreateAxisX(1.0f);
                break;
            }
            case Axis::NegativeX :
            {
                desiredAxis = AZ::Vector3::CreateAxisX(-1.0f);
                break;
            }
            default :
            {
                AZ_Assert(false, "Unexpected case");
                desiredAxis = AZ::Vector3::CreateAxisZ(1.0f);
                break;
            }
        }

        // CryPhysics uses the X axis as its default axis for constraints
        const AZ::Vector3 defaultAxis = AZ::Vector3::CreateAxisX(1.0f);
        const AZ::Quaternion fromDefaultToDesiredAxis = AZ::Quaternion::CreateShortestArc(defaultAxis, desiredAxis);

        AZ::Transform pivotWorldNoScale = pivotTransform;
        pivotWorldNoScale.ExtractScale(); // To support correct orientation with non-uniform scale
        const AZ::Quaternion pivotWorldFrame = AZ::Quaternion::CreateFromTransform(pivotWorldNoScale);

        return (pivotWorldFrame * fromDefaultToDesiredAxis);
    }

    /// ConstraintComponentNotificationBus BehaviorContext forwarder
    class BehaviorConstraintComponentNotificationBusHandler : public ConstraintComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorConstraintComponentNotificationBusHandler, "{16BDFDDB-E70F-4B48-8C43-1E5018CD5722}", AZ::SystemAllocator,
            OnConstraintEntitiesChanged, OnConstraintEnabled, OnConstraintDisabled);

        void OnConstraintEntitiesChanged(const AZ::EntityId& oldOwnwer, const AZ::EntityId& oldTarget, const AZ::EntityId& newOwner, const AZ::EntityId& newTarget) override
        {
            Call(FN_OnConstraintEntitiesChanged, oldOwnwer, oldTarget, newOwner, newTarget);
        }

        void OnConstraintEnabled() override
        {
            Call(FN_OnConstraintEnabled);
        }

        void OnConstraintDisabled() override
        {
            Call(FN_OnConstraintDisabled);
        }
    };

    //=========================================================================
    // ConstraintComponent::Reflect
    //=========================================================================
    void ConstraintComponent::Reflect(AZ::ReflectContext* context)
    {
        ConstraintConfiguration::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ConstraintComponent, AZ::Component>()
                ->Version(1)
                ->Field("ConstraintConfiguration", &ConstraintComponent::m_config);
            ;
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {

            behaviorContext->EBus<ConstraintComponentRequestBus>("ConstraintComponentRequestBus")
                ->Event("SetConstraintEntities", &ConstraintComponentRequestBus::Events::SetConstraintEntities)
                ->Event("SetConstraintEntitiesWithPartIds", &ConstraintComponentRequestBus::Events::SetConstraintEntitiesWithPartIds)
                ->Event("EnableConstraint", &ConstraintComponentRequestBus::Events::EnableConstraint)
                ->Event("DisableConstraint", &ConstraintComponentRequestBus::Events::DisableConstraint)
                ;

            behaviorContext->EBus<ConstraintComponentNotificationBus>("ConstraintComponentNotificationBus")
                ->Handler<BehaviorConstraintComponentNotificationBusHandler>()
                ;
        }
    }

    //=========================================================================
    // ConstraintComponent::GetProvidedServices
    //=========================================================================
    void ConstraintComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ConstraintService", 0xa9a63349));
    }

    //=========================================================================
    // ConstraintComponent::GetRequiredServices
    //=========================================================================
    void ConstraintComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    //=========================================================================
    // ConstraintComponent::GetDependentServices
    //=========================================================================
    void ConstraintComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("PhysicsService", 0xa7350d22));
    }

    //=========================================================================
    // ConstraintComponent::Activate
    //=========================================================================
    void ConstraintComponent::Activate()
    {
        m_ownerPhysicsEnabled = false;
        m_targetPhysicsEnabled = false;

        if (m_config.m_enableOnActivate)
        {
            m_shouldBeEnabled = true;
            // CryPhysics is enabled after Activate and OnPhysicsEnabled will actually enable the constraint
        }

        ConstraintComponentRequestBus::Handler::BusConnect(GetEntityId());

        PhysicsComponentNotificationBus::MultiHandler::BusConnect(m_config.m_owningEntity);
        PhysicsComponentNotificationBus::MultiHandler::BusConnect(m_config.m_targetEntity);

        auto checkIfPhysicsAlreadyEnabled = [this](AZ::EntityId entityId)
        {
            bool physicsEnabled = false;
            CryPhysicsComponentRequestBus::EventResult(physicsEnabled, entityId, &CryPhysicsComponentRequestBus::Events::IsPhysicsFullyEnabled);
            if (physicsEnabled)
            {
                OnPhysicsEnabledChanged(true, entityId);
            }
        };

        // Owning and target entities could be activated before we connect the buses
        // In this case we need to check it and notify the constraint manually
        checkIfPhysicsAlreadyEnabled(m_config.m_owningEntity);
        checkIfPhysicsAlreadyEnabled(m_config.m_targetEntity);
    }

    //=========================================================================
    // ConstraintComponent::Deactivate
    //=========================================================================
    void ConstraintComponent::Deactivate()
    {
        ConstraintComponentRequestBus::Handler::BusDisconnect();

        PhysicsComponentNotificationBus::MultiHandler::BusDisconnect(m_config.m_targetEntity);
        PhysicsComponentNotificationBus::MultiHandler::BusDisconnect(m_config.m_owningEntity);

        DisableConstraint();
    }

    //=========================================================================
    // ConstraintComponent::OnPhysicsEnabled
    //=========================================================================
    void ConstraintComponent::OnPhysicsEnabled()
    {
        AZ::EntityId entityId = *PhysicsComponentNotificationBus::GetCurrentBusId();
        OnPhysicsEnabledChanged(true /* enabled */, entityId);
    }

    //=========================================================================
    // ConstraintComponent::OnPhysicsDisabled
    //=========================================================================
    void ConstraintComponent::OnPhysicsDisabled()
    {
        AZ::EntityId entityId = *PhysicsComponentNotificationBus::GetCurrentBusId();
        OnPhysicsEnabledChanged(false /* enabled */, entityId);
    }

    //=========================================================================
    // ConstraintComponent::OnPhysicsEnabledChanged
    //=========================================================================
    void ConstraintComponent::OnPhysicsEnabledChanged(bool enabled, AZ::EntityId entityId)
    {
        if (entityId == m_config.m_owningEntity)
        {
            m_ownerPhysicsEnabled = enabled;
        }
        else if (entityId == m_config.m_targetEntity)
        {
            m_targetPhysicsEnabled = enabled;
        }
        else
        {
            AZ_Assert(false, "OnPhysicsEnabled/Disabled received from non-owner, non-target entity %llu", entityId);
        }

        if (enabled)
        {
            EnableInternal();
        }
        else
        {
            DisableInternal();
        }
    }

    //=========================================================================
    // ConstraintComponent::SetConstraintEntities
    //=========================================================================
    void ConstraintComponent::SetConstraintEntities(const AZ::EntityId& owningEntity, const AZ::EntityId& targetEntity)
    {
        SetConstraintEntitiesWithPartIds(owningEntity, m_config.m_ownerPartId, targetEntity, m_config.m_targetPartId);
    }

    //=========================================================================
    // ConstraintComponent::SetConstraintEntitiesWithPartIds
    //=========================================================================
    void ConstraintComponent::SetConstraintEntitiesWithPartIds(const AZ::EntityId& owningEntity, int ownerPartId, const AZ::EntityId& targetEntity, int targetPartId)
    {
        if ((owningEntity == m_config.m_owningEntity) && (targetEntity == m_config.m_targetEntity) && 
            (ownerPartId == m_config.m_ownerPartId) && (targetPartId == m_config.m_targetPartId))
        {
            return;
        }

        DisableInternal();

        PhysicsComponentNotificationBus::MultiHandler::BusDisconnect(m_config.m_targetEntity);
        PhysicsComponentNotificationBus::MultiHandler::BusDisconnect(m_config.m_owningEntity);

        AZ::EntityId oldOwner = m_config.m_owningEntity;
        AZ::EntityId oldTarget = m_config.m_targetEntity;

        m_config.m_owningEntity = owningEntity;
        m_config.m_ownerPartId = ownerPartId;

        m_config.m_targetEntity = targetEntity;
        m_config.m_targetPartId = targetPartId;

        // Note: OnPhysicsEnabled will call EnableInternal
        PhysicsComponentNotificationBus::MultiHandler::BusConnect(m_config.m_owningEntity);
        PhysicsComponentNotificationBus::MultiHandler::BusConnect(m_config.m_targetEntity);

        EBUS_EVENT_ID(GetEntityId(), ConstraintComponentNotificationBus, OnConstraintEntitiesChanged, oldOwner, oldTarget, owningEntity, targetEntity);
    }

    //=========================================================================
    // ConstraintComponent::EnableConstraint
    //=========================================================================
    void ConstraintComponent::EnableConstraint()
    {
        m_shouldBeEnabled = true;
        EnableInternal();
    }

    //=========================================================================
    // ConstraintComponent::EnableConstraint
    //=========================================================================
    void ConstraintComponent::EnableInternal()
    {
        if (IsEnabled() || !m_ownerPhysicsEnabled || (!m_targetPhysicsEnabled && m_config.m_targetEntity.IsValid()))
        {
            return;
        }

        if (m_config.m_owningEntity == m_config.m_targetEntity)
        {
            AZ_Error("ConstraintComponent", false, "Constraint component on entity %llu incorrectly constrained to itself.", GetEntityId());
            return;
        }

        m_physEntity = nullptr;
        EBUS_EVENT_ID_RESULT(m_physEntity, m_config.m_owningEntity, CryPhysicsComponentRequestBus, GetPhysicalEntity);
        if (!m_physEntity)
        {
            AZ_Error("ConstraintComponent", false, "Failed to get physical entity for entity %llu", m_config.m_owningEntity);
            return;
        }

        if (m_physEntity->GetType() == PE_STATIC)
        {
            AZ_Error("ConstraintComponent", false, "Constraint owning entity (%s) is static.  "
                "Only movable bodies may be owning entities for constraints.", GetEntity()->GetName().c_str());
            return;
        }

        // The owner needs to be awake
        pe_action_awake actionAwake;
        actionAwake.minAwakeTime = 0.1f;
        m_physEntity->Action(&actionAwake);

        m_physBuddy = nullptr;
        if (m_config.m_targetEntity.IsValid())
        {
            EBUS_EVENT_ID_RESULT(m_physBuddy, m_config.m_targetEntity, CryPhysicsComponentRequestBus, GetPhysicalEntity);
            if (!m_physBuddy)
            {
                AZ_Error("ConstraintComponent", false, "Failed to get physical entity for target entity %llu on owning entity %llu", m_config.m_targetEntity, m_config.m_owningEntity);
                return;
            }

            // Ensure the buddy is awake as well
            pe_action_awake wakeBuddy;
            wakeBuddy.minAwakeTime = 0.1f;
            m_physBuddy->Action(&wakeBuddy);
        }
        else
        {
            m_physBuddy = WORLD_ENTITY;
        }

        m_action_add_constraint = pe_action_add_constraint();
        m_action_add_constraint.pBuddy = m_physBuddy;

        SetupPivotsAndFrame(m_action_add_constraint);

        switch (m_config.m_constraintType)
        {
            case ConstraintConfiguration::ConstraintType::Hinge :
            {
                m_action_add_constraint.yzlimits[0] = 0.0f;
                m_action_add_constraint.yzlimits[1] = 0.0f;
                if (m_config.m_enableRotationLimits)
                {
                    m_action_add_constraint.xlimits[0] = AZ::DegToRad(m_config.m_xmin);
                    m_action_add_constraint.xlimits[1] = AZ::DegToRad(m_config.m_xmax);
                }
                break;
            }
            case ConstraintConfiguration::ConstraintType::Ball :
            {
                if (m_config.m_enableRotationLimits)
                {
                    m_action_add_constraint.xlimits[0] = 0.0f;
                    m_action_add_constraint.xlimits[1] = 0.0f;
                    m_action_add_constraint.yzlimits[0] = 0.0f;
                    m_action_add_constraint.yzlimits[1] = AZ::DegToRad(m_config.m_yzmax);
                }
                break;
            }
            case ConstraintConfiguration::ConstraintType::Slider :
            {
                m_action_add_constraint.yzlimits[0] = 0.0f;
                m_action_add_constraint.yzlimits[1] = 0.0f;
                if (m_config.m_enableRotationLimits)
                {
                    m_action_add_constraint.xlimits[0] = m_config.m_xmin;
                    m_action_add_constraint.xlimits[1] = m_config.m_xmax;
                }
                m_action_add_constraint.flags |= constraint_line;
                break;
            }
            case ConstraintConfiguration::ConstraintType::Plane :
            {
                m_action_add_constraint.flags |= constraint_plane;
                break;
            }
            case ConstraintConfiguration::ConstraintType::Magnet :
            {
                break;
            }
            case ConstraintConfiguration::ConstraintType::Fixed :
            {
                m_action_add_constraint.flags |= constraint_no_rotation;
                break;
            }
            case ConstraintConfiguration::ConstraintType::Free :
            {
                m_action_add_constraint.flags |= constraint_free_position;
                m_action_add_constraint.flags |= constraint_no_rotation;
                break;
            }
            default:
            {
                AZ_Assert(false, "Unexpected constraint type?");
                break;
            }
        };

        if (m_config.m_enableConstrainToPartId)
        {
            m_action_add_constraint.partid[0] = m_config.m_ownerPartId;
            m_action_add_constraint.partid[1] = m_config.m_targetPartId;
        }

        if (m_config.m_enableForceLimits)
        {
            m_action_add_constraint.maxPullForce = m_config.m_maxPullForce;
        }

        if (m_config.m_enableRotationLimits)
        {
            m_action_add_constraint.maxBendTorque = m_config.m_maxBendTorque;
        }

        if (m_config.m_enableDamping)
        {
            m_action_add_constraint.damping = m_config.m_damping;
        }

        if (m_config.m_enableSearchRadius)
        {
            m_action_add_constraint.sensorRadius = m_config.m_searchRadius;
        }

        if (!m_config.m_enableTargetCollisions)
        {
            m_action_add_constraint.flags |= constraint_ignore_buddy;
        }
        if (!m_config.m_enableRelativeRotation)
        {
            m_action_add_constraint.flags |= constraint_no_rotation;
        }
        if (!m_config.m_enforceOnFastObjects)
        {
            m_action_add_constraint.flags |= constraint_no_enforcement;
        }
        if (!m_config.m_breakable)
        {
            m_action_add_constraint.flags |= constraint_no_tears;
        }

        // requests to add geometry in CryPhysics can get queued, so check if both entities have geometry (or the buddy is the world entity)
        // otherwise connect to the tick bus and wait for geometry to be available before submitting the action to create the constraint
        pe_status_nparts physEntityStatusParts, physBuddyStatusParts;
        int physEntityNumParts = m_physEntity->GetStatus(&physEntityStatusParts);
        int physBuddyNumParts = 0;
        if (m_physBuddy != WORLD_ENTITY)
        {
            physBuddyNumParts = m_physBuddy->GetStatus(&physBuddyStatusParts);
        }

        if (physEntityNumParts > 0 && (physBuddyNumParts > 0 || m_physBuddy == WORLD_ENTITY))
        {
            AddCryConstraint();
        }
        else
        {
            m_activationState = ActivationState::WaitingForGeometry;
            AZ::TickBus::Handler::BusConnect();
        }
    }

    //=========================================================================
    // ConstraintComponent::SetupPivotsAndFrame
    //=========================================================================
    void ConstraintComponent::SetupPivotsAndFrame(pe_action_add_constraint &aac) const
    {
        // Note: Coordinate space does not change constraint behavior, it only sets the coordinate space for initial positions and coordinate frames
        aac.flags &= ~local_frames;
        aac.flags |= world_frames;

        const AZ::EntityId selfId = GetEntityId();

        AZ::Transform selfTransform = AZ::Transform::Identity();
        EBUS_EVENT_ID_RESULT(selfTransform, selfId, AZ::TransformBus, GetWorldTM);
        const AZ::Vector3 selfWorldTranslation = selfTransform.GetTranslation();

        // World frame (qframe is ignored on ball socket constraints)
        if (m_config.m_constraintType != ConstraintConfiguration::ConstraintType::Ball)
        {
            const AZ::Quaternion worldFrame = ConstraintConfiguration::GetWorldFrame(m_config.m_axis, selfTransform);
            const quaternionf frame = AZQuaternionToLYQuaternion(worldFrame);
            aac.qframe[0] = frame;
            aac.qframe[1] = frame;
        }

        // Owner pivot
        AZ::Vector3 ownerPivot;
        const bool isMagnet = m_config.m_constraintType == ConstraintConfiguration::ConstraintType::Magnet;
        if (!isMagnet || (m_config.m_owningEntity == selfId))
        {
            // For all types except magnet, the owner pivot is always at self, whether it is owner, or a separate pivot
            ownerPivot = selfWorldTranslation;
        }
        else
        {
            // For magnet the pivot is always directly on the owner
            AZ::Transform ownerTransform = AZ::Transform::Identity();
            EBUS_EVENT_ID_RESULT(ownerTransform, m_config.m_owningEntity, AZ::TransformBus, GetWorldTM);
            ownerPivot = ownerTransform.GetTranslation();
        }
        aac.pt[0] = AZVec3ToLYVec3(ownerPivot);

        // Target pivot
        AZ::Vector3 targetPivot;
        if (!isMagnet || !m_config.m_targetEntity.IsValid())
        {
            // For all types except magnet, the target pivot is always at self, except for entity-to-entity magnet constraints
            targetPivot = selfWorldTranslation;
        }
        else
        {
            AZ::Transform targetTransform = AZ::Transform::Identity();
            EBUS_EVENT_ID_RESULT(targetTransform, m_config.m_targetEntity, AZ::TransformBus, GetWorldTM);
            targetPivot = targetTransform.GetTranslation();
        }
        aac.pt[1] = AZVec3ToLYVec3(targetPivot);
    }

    void ConstraintComponent::AddCryConstraint()
    {
        m_constraintId = m_physEntity->Action(&m_action_add_constraint) != 0;
        if (m_constraintId)
        {
            m_activationState = ActivationState::Active;
            EBUS_EVENT_ID(GetEntityId(), ConstraintComponentNotificationBus, OnConstraintEnabled);
        }
        else
        {
            m_activationState = ActivationState::Inactive;
            AZ_Error("ConstraintComponent", false, "Constraint failed to initialize on entity %llu.", GetEntityId());
        }
    }

    void ConstraintComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        pe_status_nparts physEntityStatusParts, physBuddyStatusParts;
        int physEntityNumParts = m_physEntity->GetStatus(&physEntityStatusParts);
        int physBuddyNumParts = 0;
        if (m_physBuddy != WORLD_ENTITY)
        {
            physBuddyNumParts = m_physBuddy->GetStatus(&physBuddyStatusParts);
        }

        if (physEntityNumParts > 0 && (physBuddyNumParts > 0 || m_physBuddy == WORLD_ENTITY))
        {
            AddCryConstraint();
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    //=========================================================================
    // ConstraintComponent::DisableConstraint
    //=========================================================================
    void ConstraintComponent::DisableConstraint()
    {
        m_shouldBeEnabled = false;
        DisableInternal();
    }

    //=========================================================================
    // ConstraintComponent::SetConfiguration
    //=========================================================================
    void ConstraintComponent::SetConfiguration(const ConstraintConfiguration& config)
    {
        m_config = config;
    }

    //=========================================================================
    // ConstraintComponent::Disable
    //=========================================================================
    void ConstraintComponent::DisableInternal()
    {
        if (!IsEnabled())
        {
            return;
        }

        if (m_physEntity)
        {
            pe_action_update_constraint auc;
            auc.idConstraint = m_constraintId;
            auc.bRemove = 1;
            m_physEntity->Action(&auc);
        }

        m_constraintId = 0;

        // reset the activation state and in case we were waiting for geometry check whether the tick bus is connected and disconnect
        m_activationState = ActivationState::Inactive;
        m_physEntity = nullptr;
        m_physBuddy = nullptr;
        if (AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }

        EBUS_EVENT_ID(GetEntityId(), ConstraintComponentNotificationBus, OnConstraintDisabled);
    }

    //=========================================================================
    // ConstraintComponent::Disable
    //=========================================================================
    bool ConstraintComponent::IsEnabled() const
    {
        return m_constraintId != 0;
    }
} // namespace LmbrCentral
