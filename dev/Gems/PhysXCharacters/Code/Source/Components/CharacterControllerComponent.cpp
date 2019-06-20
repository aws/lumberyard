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

#include <PhysXCharacters_precompiled.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <API/CharacterController.h>
#include <API/CharacterControllerCosmeticReplica.h>
#include <Components/CharacterControllerComponent.h>
#include <PhysXCharacters/SystemBus.h>
#include <PhysX/ColliderComponentBus.h>
#include <AzFramework/Physics/Utils.h>

namespace PhysXCharacters
{
    void CharacterControllerComponent::Reflect(AZ::ReflectContext* context)
    {
        CharacterControllerConfiguration::Reflect(context);
        CharacterController::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CharacterControllerComponent, AZ::Component>()
                ->Version(1)
                ->Field("CharacterConfig", &CharacterControllerComponent::m_characterConfig)
                ->Field("ShapeConfig", &CharacterControllerComponent::m_shapeConfig)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<Physics::CharacterRequestBus>("Character Controller")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                ->Event("Get Base Position", &Physics::CharacterRequests::GetBasePosition)
                ->Event("Set Base Position", &Physics::CharacterRequests::SetBasePosition,
                    { { { "Position" , "The controller will directly move (teleport) to this base position" } } })
                ->Event("Get Center Position", &Physics::CharacterRequests::GetCenterPosition)
                ->Event("Get Step Height", &Physics::CharacterRequests::GetStepHeight)
                ->Event("Set Step Height", &Physics::CharacterRequests::SetStepHeight,
                    { { { "Step Height", "The new value for the step height" } } })
                ->Event("Get Up Direction", &Physics::CharacterRequests::GetUpDirection)
                ->Event("Get Slope Limit (Degrees)", &Physics::CharacterRequests::GetSlopeLimitDegrees)
                ->Event("Set Slope Limit (Degrees)", &Physics::CharacterRequests::SetSlopeLimitDegrees,
                    { { { "Slope Limit", "The new value for the slope limit in degrees" } } })
                ->Event("Get Velocity", &Physics::CharacterRequests::GetVelocity)
                ->Event("Try Relative Move", &Physics::CharacterRequests::TryRelativeMove,
                    { { { "Delta Position", "Desired movement relative to current position" },
                        { "Delta Time", "Duration of the movement" } } })
                ->Attribute(AZ::Script::Attributes::ToolTip,
                    "Tries to perform a relative movement, returning the new base position")
                ;

            behaviorContext->EBus<CharacterControllerRequestBus>("Character Controller (PhysX specific)")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                ->Event("Resize", &CharacterControllerRequests::Resize,
                    { { { "Height", "The new height for the character controller (maintains the base position)" } } })
                ->Event("Get Height", &CharacterControllerRequests::GetHeight)
                ->Event("Set Height", &CharacterControllerRequests::SetHeight,
                    { { { "Height", "The new height for the character controller (maintains the center position)" } } })
                ->Event("Get Radius", &CharacterControllerRequests::GetRadius)
                ->Event("Set Radius", &CharacterControllerRequests::SetRadius,
                    { { { "Radius", "The new radius (for capsule controllers only)" } } })
                ->Event("Get Half Side Extent", &CharacterControllerRequests::GetHalfSideExtent)
                ->Event("Set Half Side Extent", &CharacterControllerRequests::SetHalfSideExtent,
                    { { { "Half Side Extent", "The new half side extent (for box controllers only)" } } })
                ->Event("Get Half Forward Extent", &CharacterControllerRequests::GetHalfForwardExtent)
                ->Event("Set Half Forward Extent", &CharacterControllerRequests::SetHalfForwardExtent,
                    { { { "Half Forward Extent", "The new half forward extent (for box controllers only)" } } })
                ;
        }
    }

    CharacterControllerComponent::CharacterControllerComponent() = default;

    CharacterControllerComponent::CharacterControllerComponent(AZStd::unique_ptr<Physics::CharacterConfiguration> characterConfig,
        AZStd::unique_ptr<Physics::ShapeConfiguration> shapeConfig)
        : m_characterConfig(AZStd::move(characterConfig))
        , m_shapeConfig(AZStd::move(shapeConfig))
    {
    }

    CharacterControllerComponent::~CharacterControllerComponent() = default;

    // AZ::Component
    void CharacterControllerComponent::Init()
    {
    }

    void CharacterControllerComponent::Activate()
    {
        AZStd::shared_ptr<Physics::World> defaultWorld;
        Physics::DefaultWorldBus::BroadcastResult(defaultWorld, &Physics::DefaultWorldRequests::GetDefaultWorld);
        if (!defaultWorld)
        {
            AZ_Error("PhysX Character Controller Component", false, "Failed to retrieve default world.");
            return;
        }

        m_characterConfig->m_debugName = GetEntity()->GetName();
        m_characterConfig->m_entityId = GetEntityId();

        if (m_characterConfig->m_directControl)
        {
            Physics::CharacterSystemRequestBus::BroadcastResult(m_controller,
                &Physics::CharacterSystemRequests::CreateCharacter, *m_characterConfig, *m_shapeConfig, *defaultWorld);
            if (!m_controller)
            {
                AZ_Error("PhysX Character Controller Component", false, "Failed to create character controller.");
                return;
            }

            AZ::Vector3 entityTranslation = AZ::Vector3::CreateZero();
            AZ::TransformBus::EventResult(entityTranslation, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);
            // It's usually more convenient to control the foot position rather than the centre of the capsule, so
            // make the foot position coincide with the entity position.
            m_controller->SetBasePosition(entityTranslation);
            AttachColliders(*m_controller);
            CharacterControllerRequestBus::Handler::BusConnect(GetEntityId());
        }

        else
        {
            // make a replica
            m_controller = AZStd::make_unique<CharacterControllerCosmeticReplica>(*m_characterConfig,
                *m_shapeConfig, *defaultWorld);
            AttachColliders(*m_controller);
            AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        }

        Physics::CharacterRequestBus::Handler::BusConnect(GetEntityId());
    }

    void CharacterControllerComponent::Deactivate()
    {
        CharacterControllerRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        Physics::CharacterRequestBus::Handler::BusDisconnect();
        Physics::Utils::DeferDelete(AZStd::move(m_controller));
    }

    // helper functions
    bool CharacterControllerComponent::ValidateDirectlyControlled()
    {
        if (!m_characterConfig->m_directControl)
        {
            AZ_Warning("PhysX Character Controller Component", false, "Character controller is not directly controlled.");
            return false;
        }

        if (!m_controller)
        {
            AZ_Error("PhysX Character Controller Component", false, "Invalid character controller.");
            return false;
        }

        return true;
    }

    template<class T>
    static T CheckValidAndReturn(const AZStd::unique_ptr<Physics::Character>& controller, T result)
    {
        if (!controller)
        {
            AZ_Error("PhysX Character Controller Component", false, "Invalid character controller.");
        }

        return result;
    }

    // Physics::CharacterRequestBus
    AZ::Vector3 CharacterControllerComponent::GetBasePosition() const
    {
        return CheckValidAndReturn(m_controller, m_controller ? m_controller->GetBasePosition() : AZ::Vector3::CreateZero());
    }

    void CharacterControllerComponent::SetBasePosition(const AZ::Vector3& position)
    {
        if (!m_controller)
        {
            AZ_Error("PhysX Character Controller Component", false, "Invalid character controller.");
            return;
        }

        m_controller->SetBasePosition(position);
    }

    AZ::Vector3 CharacterControllerComponent::GetCenterPosition() const
    {
        return CheckValidAndReturn(m_controller, m_controller ? m_controller->GetCenterPosition() : AZ::Vector3::CreateZero());
    }

    float CharacterControllerComponent::GetStepHeight() const
    {
        return CheckValidAndReturn(m_controller, m_controller ? m_controller->GetStepHeight() : 0.0f);
    }

    void CharacterControllerComponent::SetStepHeight(float stepHeight)
    {
        if (!m_controller)
        {
            AZ_Error("PhysX Character Controller Component", false, "Invalid character controller.");
            return;
        }

        m_controller->SetStepHeight(stepHeight);
    }

    AZ::Vector3 CharacterControllerComponent::GetUpDirection() const
    {
        return CheckValidAndReturn(m_controller, m_controller ? m_controller->GetUpDirection() : AZ::Vector3::CreateZero());
    }

    void CharacterControllerComponent::SetUpDirection(const AZ::Vector3& upDirection)
    {
        AZ_Warning("PhysX Character Controller Component", false, "Setting up direction is not currently supported.");
    }

    float CharacterControllerComponent::GetSlopeLimitDegrees() const
    {
        return CheckValidAndReturn(m_controller, m_controller ? m_controller->GetSlopeLimitDegrees() : 0.0f);
    }

    void CharacterControllerComponent::SetSlopeLimitDegrees(float slopeLimitDegrees)
    {
        if (!m_controller)
        {
            AZ_Error("PhysX Character Controller Component", false, "Invalid character controller.");
            return;
        }

        m_controller->SetSlopeLimitDegrees(slopeLimitDegrees);
    }

    AZ::Vector3 CharacterControllerComponent::GetVelocity() const
    {
        return CheckValidAndReturn(m_controller, m_controller ? m_controller->GetVelocity() : AZ::Vector3::CreateZero());
    }

    AZ::Vector3 CharacterControllerComponent::TryRelativeMove(const AZ::Vector3& deltaPosition, float deltaTime)
    {
        if (!ValidateDirectlyControlled())
        {
            return AZ::Vector3::CreateZero();
        }

        const AZ::Vector3& newPosition = m_controller->TryRelativeMove(deltaPosition, deltaTime);
        AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTranslation, newPosition);
        return newPosition;
    }

    // CharacterControllerRequestBus
    void CharacterControllerComponent::Resize(float height)
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->Resize(height);
        }

        AZ_Error("PhysX Character Controller Component", false, "Invalid character controller.");
    }

    float CharacterControllerComponent::GetHeight()
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->GetHeight();
        }

        AZ_Error("PhysX Character Controller Component", false, "Invalid character controller.");
        return 0.0f;
    }

    void CharacterControllerComponent::SetHeight(float height)
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->SetHeight(height);
        }

        AZ_Error("PhysX Character Controller Component", false, "Invalid character controller.");
    }

    float CharacterControllerComponent::GetRadius()
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->GetRadius();
        }

        AZ_Error("PhysX Character Controller Component", false, "Invalid character controller.");
        return 0.0f;
    }

    void CharacterControllerComponent::SetRadius(float radius)
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->SetRadius(radius);
        }

        AZ_Error("PhysX Character Controller Component", false, "Invalid character controller.");
    }

    float CharacterControllerComponent::GetHalfSideExtent()
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->GetHalfSideExtent();
        }

        AZ_Error("PhysX Character Controller Component", false, "Invalid character controller.");
        return 0.0f;
    }

    void CharacterControllerComponent::SetHalfSideExtent(float halfSideExtent)
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->SetHalfSideExtent(halfSideExtent);
        }

        AZ_Error("PhysX Character Controller Component", false, "Invalid character controller.");
    }

    float CharacterControllerComponent::GetHalfForwardExtent()
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->GetHalfForwardExtent();
        }

        AZ_Error("PhysX Character Controller Component", false, "Invalid character controller.");
        return 0.0f;
    }

    void CharacterControllerComponent::SetHalfForwardExtent(float halfForwardExtent)
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->SetHalfForwardExtent(halfForwardExtent);
        }

        AZ_Error("PhysX Character Controller Component", false, "Invalid character controller.");
    }

    // TransformNotificationBus
    void CharacterControllerComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        if (auto controllerReplica = static_cast<CharacterControllerCosmeticReplica*>(m_controller.get()))
        {
            AZ::Quaternion rotation = AZ::Quaternion::CreateFromTransform(world);
            controllerReplica->Move(world.GetTranslation());
            controllerReplica->SetRotation(rotation);
        }
    }

    void CharacterControllerComponent::AttachColliders(Physics::Character& character)
    {
        PhysX::ColliderComponentRequestBus::EnumerateHandlersId(GetEntityId(), [&character](PhysX::ColliderComponentRequests* handler)
        {
            character.AttachShape(handler->GetShape());
            return true;
        });
    }
} // namespace PhysXCharacters
