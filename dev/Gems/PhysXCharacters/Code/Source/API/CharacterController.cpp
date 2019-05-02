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
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SystemBus.h>
#include <PhysX/ConfigurationBus.h>
#include <PhysX/SystemComponentBus.h>
#include <PhysX/Utils.h>
#include <PhysXCharacters/SystemBus.h>
#include <PhysXCharacters/NativeTypeIdentifiers.h>
#include <API/CharacterController.h>
#include <API/CharacterControllerCosmeticReplica.h>
#include <AzFramework/Physics/CollisionBus.h>

namespace PhysXCharacters
{
    void CharacterControllerConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CharacterControllerConfiguration, Physics::CharacterConfiguration>()
                ->Version(1)
                ->Field("SlopeBehaviour", &CharacterControllerConfiguration::m_slopeBehaviour)
                ->Field("ContactOffset", &CharacterControllerConfiguration::m_contactOffset)
                ->Field("ScaleCoeff", &CharacterControllerConfiguration::m_scaleCoefficient)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CharacterControllerConfiguration>(
                    "PhysX Character Controller Configuration", "PhysX Character Controller Configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &CharacterControllerConfiguration::m_slopeBehaviour,
                        "Slope Behaviour", "Behaviour of the controller on surfaces above the maximum slope")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->EnumAttribute(SlopeBehaviour::PreventClimbing, "Prevent Climbing")
                    ->EnumAttribute(SlopeBehaviour::ForceSliding, "Force Sliding")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterControllerConfiguration::m_contactOffset,
                        "Contact Offset", "Extra distance outside the controller used for smoother contact resolution")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterControllerConfiguration::m_scaleCoefficient,
                        "Scale", "Scalar coefficient used to scale the controller, usually slightly smaller than 1")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ;
            }
        }
    }

    void CharacterController::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CharacterController>()
                ->Version(1)
                ;
        }
    }

    CharacterController::CharacterController(physx::PxController* pxController)
        : m_pxController(pxController)
    {
    }

    void CharacterController::SetFilterDataAndShape(const Physics::CharacterConfiguration& characterConfig)
    {
        Physics::CollisionGroup collisionGroup;
        Physics::CollisionRequestBus::BroadcastResult(collisionGroup, &Physics::CollisionRequests::GetCollisionGroupById, characterConfig.m_collisionGroupId);

        PhysX::SystemRequestsBus::BroadcastResult(m_filterData, &PhysX::SystemRequests::CreateFilterData,
            characterConfig.m_collisionLayer, collisionGroup);
        m_pxControllerFilters = physx::PxControllerFilters(&m_filterData);
        m_pxControllerFilters.mFilterFlags = physx::PxQueryFlag::eSTATIC;

        physx::PxU32 numShapes = m_pxController->getActor()->getNbShapes();
        if (numShapes != 1)
        {
            AZ_Error("PhysX Character Controller", false, "Found %i shapes, expected exactly 1.", numShapes)
        }
        else
        {
            physx::PxShape* pxShape = nullptr;
            m_pxController->getActor()->getShapes(&pxShape, 1, 0);
            pxShape->setQueryFilterData(m_filterData);
            pxShape->setSimulationFilterData(m_filterData);

            // wrap the raw PhysX shape so that it is appropriately configured for raycasts etc.
            PhysX::SystemRequestsBus::BroadcastResult(m_shape, &PhysX::SystemRequests::CreateWrappedNativeShape, pxShape);
        }
    }

    void CharacterController::SetActorName(const AZStd::string& name)
    {
        m_name = name;
        if (m_pxController)
        {
            m_pxController->getActor()->setName(m_name.c_str());
        }
    }

    void CharacterController::SetUserData(const Physics::CharacterConfiguration& characterConfig)
    {
        m_actorUserData = PhysX::ActorData(m_pxController->getActor());
        m_actorUserData.SetCharacter(this);
        m_actorUserData.SetEntityId(characterConfig.m_entityId);
    }

    void CharacterController::SetMinimumMovementDistance(float distance)
    {
        m_minimumMovementDistance = distance;
    }

    void CharacterController::CreateShadowBody(const Physics::CharacterConfiguration& configuration, Physics::World& world)
    {
        Physics::RigidBodyConfiguration rigidBodyConfig;
        rigidBodyConfig.m_kinematic = true;
        rigidBodyConfig.m_debugName = configuration.m_debugName + " (Shadow)";
        rigidBodyConfig.m_entityId = configuration.m_entityId;
        Physics::SystemRequestBus::BroadcastResult(m_shadowBody, &Physics::SystemRequests::CreateRigidBody, rigidBodyConfig);
        world.AddBody(*m_shadowBody);
    }

    CharacterController::~CharacterController()
    {
        if (m_pxController)
        {
            m_pxController->release();
        }
        m_pxController = nullptr;
        m_shape = nullptr;
        m_material = nullptr;
    }

    void CharacterController::UpdateObservedVelocity(const AZ::Vector3& observedVelocity)
    {
        m_observedVelocity = observedVelocity;
    }

    // Physics::Character
    AZ::Vector3 CharacterController::GetBasePosition() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return AZ::Vector3::CreateZero();
        }

        return PxMathConvertExtended(m_pxController->getFootPosition());
    }

    void CharacterController::SetBasePosition(const AZ::Vector3& position)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return;
        }

        m_pxController->setFootPosition(PxMathConvertExtended(position));
        if (m_shadowBody)
        {
            m_shadowBody->SetTransform(AZ::Transform::CreateTranslation(GetBasePosition()));
        }
    }

    AZ::Vector3 CharacterController::GetCenterPosition() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return AZ::Vector3::CreateZero();
        }

        if (m_pxController->getType() == physx::PxControllerShapeType::eCAPSULE)
        {
            auto capsuleController = static_cast<physx::PxCapsuleController*>(m_pxController);
            const float halfHeight = 0.5f * capsuleController->getHeight() + capsuleController->getRadius();
            return GetBasePosition() + PxMathConvert(halfHeight * m_pxController->getUpDirection());
        }

        if (m_pxController->getType() == physx::PxControllerShapeType::eBOX)
        {
            auto boxController = static_cast<physx::PxBoxController*>(m_pxController);
            return GetBasePosition() + AZ::Vector3::CreateAxisZ(boxController->getHalfHeight());
        }

        AZ_Warning("PhysX Character Controller", false, "Unrecognized shape type.");
        return AZ::Vector3::CreateZero();
    }

    template<class T>
    static T CheckValidAndReturn(physx::PxController* controller, T result)
    {
        if (!controller)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
        }

        return result;
    }

    float CharacterController::GetStepHeight() const
    {
        return CheckValidAndReturn(m_pxController, m_pxController ? m_pxController->getStepOffset() : 0.0f);
    }

    void CharacterController::SetStepHeight(float stepHeight)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return;
        }

        if (stepHeight <= 0.0f)
        {
            AZ_Warning("PhysX Character Controller", false, "PhysX requires the step height to be positive.");
        }

        m_pxController->setStepOffset(stepHeight);
    }

    AZ::Vector3 CharacterController::GetUpDirection() const
    {
        return CheckValidAndReturn(m_pxController,
            m_pxController ? PxMathConvert(m_pxController->getUpDirection()) : AZ::Vector3::CreateZero());
    }

    void CharacterController::SetUpDirection(const AZ::Vector3& upDirection)
    {
        AZ_Warning("PhysX Character Controller", false, "Setting up direction is not currently supported.");
        return;
    }

    float CharacterController::GetSlopeLimitDegrees() const
    {
        return CheckValidAndReturn(m_pxController,
            m_pxController ? AZ::RadToDeg(acosf(m_pxController->getSlopeLimit())) : 0.0f);
    }

    void CharacterController::SetSlopeLimitDegrees(float slopeLimitDegrees)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return;
        }

        float slopeLimitClamped = AZ::GetClamp(slopeLimitDegrees, 0.0f, 90.0f);

        if (slopeLimitDegrees != slopeLimitClamped)
        {
            AZ_Warning("PhysX Character Controller", false, "Slope limit should be in the range 0-90 degrees.  "
                "Value %f was clamped to %f", slopeLimitDegrees, slopeLimitClamped);
        }

        m_pxController->setSlopeLimit(cosf(AZ::DegToRad(slopeLimitClamped)));
    }

    AZ::Vector3 CharacterController::GetVelocity() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return AZ::Vector3::CreateZero();
        }

        return m_observedVelocity;
    }

    AZ::Vector3 CharacterController::TryRelativeMove(const AZ::Vector3& deltaPosition, float deltaTime)
    {
        const AZ::Vector3& oldPosition = GetBasePosition();

        if (m_pxController)
        {
            m_pxController->move(PxMathConvert(deltaPosition), m_minimumMovementDistance, deltaTime, m_pxControllerFilters);
            if (m_shadowBody)
            {
                m_shadowBody->SetKinematicTarget(AZ::Transform::CreateTranslation(GetBasePosition()));
            }
            const AZ::Vector3& newPosition = GetBasePosition();
            m_observedVelocity = deltaTime > 0.0f ? (newPosition - oldPosition) / deltaTime : AZ::Vector3::CreateZero();
            return newPosition;
        }

        return oldPosition;
    }

    void CharacterController::SetRotation(const AZ::Quaternion& rotation)
    {
        if (m_shadowBody)
        {
            AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(rotation, GetBasePosition());
            m_shadowBody->SetKinematicTarget(transform);
        }
    }

    void CharacterController::CheckSupport(const AZ::Vector3& direction, float distance, const Physics::CharacterSupportInfo& supportInfo)
    {
        AZ_Warning("PhysX Character Controller", false, "Not yet supported.");
    }

    // Physics::WorldBody
    AZ::EntityId CharacterController::GetEntityId() const
    {
        return m_actorUserData.GetEntityId();
    }

    Physics::World* CharacterController::GetWorld() const
    {
        return m_pxController ? PhysX::Utils::GetUserData(m_pxController->getScene()) : nullptr;
    }

    AZ::Transform CharacterController::GetTransform() const
    {
        return AZ::Transform::CreateTranslation(GetPosition());
    }

    void CharacterController::SetTransform(const AZ::Transform& transform)
    {
        SetBasePosition(transform.GetTranslation());
    }

    AZ::Vector3 CharacterController::GetPosition() const
    {
        return GetBasePosition();
    }

    AZ::Quaternion CharacterController::GetOrientation() const
    {
        return AZ::Quaternion::CreateIdentity();
    }

    AZ::Aabb CharacterController::GetAabb() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return AZ::Aabb::CreateNull();
        }

        // use bounding box inflation factor of 1.0f so users can control inflation themselves
        const float inflationFactor = 1.0f;
        return PxMathConvert(m_pxController->getActor()->getWorldBounds(inflationFactor));
    }

    void CharacterController::RayCast(const Physics::RayCastRequest& request, Physics::RayCastResult& result) const
    {
        AZ_Warning("PhysX Character Controller", false, "Not yet supported.");
    }

    AZ::Crc32 CharacterController::GetNativeType() const
    {
        return NativeTypeIdentifiers::CharacterController;
    }

    void* CharacterController::GetNativePointer() const
    {
        return m_pxController;
    }
    void CharacterController::AttachShape(AZStd::shared_ptr<Physics::Shape> shape)
    {
        if (m_shadowBody)
        {
            m_shadowBody->AddShape(shape);
        }
    }
    // CharacterController specific
    void CharacterController::Resize(float height)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
        }

        if (height <= 0.0f)
        {
            AZ_Error("PhysX Character Controller", false, "PhysX requires controller height to be positive.");
            return;
        }

        if (m_pxController->getType() == physx::PxControllerShapeType::eCAPSULE)
        {
            auto capsuleController = static_cast<physx::PxCapsuleController*>(m_pxController);
            if (height <= 2.0f * capsuleController->getRadius())
            {
                AZ_Error("PhysX Character Controller", false, "Capsule height must exceed twice its radius.");
                return;
            }
        }

        m_pxController->resize(height);
    }

    float CharacterController::GetHeight() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return 0.0f;
        }

        if (m_pxController->getType() == physx::PxControllerShapeType::eBOX)
        {
            return static_cast<physx::PxBoxController*>(m_pxController)->getHalfHeight() * 2.0f;
        }

        else if (m_pxController->getType() == physx::PxControllerShapeType::eCAPSULE)
        {
            // PhysX capsule height refers to the length of the cylindrical section.
            // LY capsule height refers to the length including the hemispherical caps.
            auto capsuleController = static_cast<physx::PxCapsuleController*>(m_pxController);
            return capsuleController->getHeight() + 2.0f * capsuleController->getRadius();
        }

        AZ_Error("PhysX Character Controller", false, "Unrecognized controller shape type.");
        return 0.0f;
    }

    void CharacterController::SetHeight(float height)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return;
        }

        if (m_pxController->getType() == physx::PxControllerShapeType::eBOX)
        {
            if (height <= 0.0f)
            {
                AZ_Error("PhysX Character Controller", false, "PhysX requires controller height to be positive.");
                return;
            }

            auto boxController = static_cast<physx::PxBoxController*>(m_pxController);
            boxController->setHalfHeight(0.5f * height);
        }

        else if (m_pxController->getType() == physx::PxControllerShapeType::eCAPSULE)
        {
            auto capsuleController = static_cast<physx::PxCapsuleController*>(m_pxController);
            float radius = capsuleController->getRadius();
            if (height <= 2.0f * radius)
            {
                AZ_Error("PhysX Character Controller", false, "Capsule height must exceed twice its radius.");
                return;
            }

            // PhysX capsule height refers to the length of the cylindrical section.
            // LY capsule height refers to the length including the hemispherical caps.
            capsuleController->setHeight(height - 2.0f * radius);
        }

        else
        {
            AZ_Error("PhysX Character Controller", false, "Unrecognized controller shape type.");
        }
    }

    float CharacterController::GetRadius() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return 0.0f;
        }

        if (m_pxController->getType() == physx::PxControllerShapeType::eCAPSULE)
        {
            return static_cast<physx::PxCapsuleController*>(m_pxController)->getRadius();
        }

        AZ_Error("PhysX Character Controller", false, "Radius is only defined for capsule controllers.");
        return 0.0f;
    }

    void CharacterController::SetRadius(float radius)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return;
        }

        if (m_pxController->getType() == physx::PxControllerShapeType::eCAPSULE)
        {
            if (radius <= 0.0f)
            {
                AZ_Error("PhysX Character Controller", false, "PhysX requires radius to be positive.");
                return;
            }

            static_cast<physx::PxCapsuleController*>(m_pxController)->setRadius(radius);
        }

        else
        {
            AZ_Error("PhysX Character Controller", false, "Radius is only defined for capsule controllers.");
        }
    }

    float CharacterController::GetHalfSideExtent() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return 0.0f;
        }

        if (m_pxController->getType() == physx::PxControllerShapeType::eBOX)
        {
            return static_cast<physx::PxBoxController*>(m_pxController)->getHalfSideExtent();
        }

        AZ_Error("PhysX Character Controller", false, "Half side extent is only defined for box controllers.");
        return 0.0f;
    }

    void CharacterController::SetHalfSideExtent(float halfSideExtent)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return;
        }

        if (m_pxController->getType() == physx::PxControllerShapeType::eBOX)
        {
            if (halfSideExtent <= 0.0f)
            {
                AZ_Error("PhysX Character Controller", false, "PhysX requires half side extent to be positive.");
                return;
            }

            static_cast<physx::PxBoxController*>(m_pxController)->setHalfSideExtent(halfSideExtent);
        }

        else
        {
            AZ_Error("PhysX Character Controller", false, "Half side extent is only defined for box controllers.");
        }
    }

    float CharacterController::GetHalfForwardExtent() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return 0.0f;
        }

        if (m_pxController->getType() == physx::PxControllerShapeType::eBOX)
        {
            return static_cast<physx::PxBoxController*>(m_pxController)->getHalfForwardExtent();
        }

        AZ_Error("PhysX Character Controller", false, "Half forward extent is only defined for box controllers.");
        return 0.0f;
    }

    void CharacterController::SetHalfForwardExtent(float halfForwardExtent)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return;
        }

        if (m_pxController->getType() == physx::PxControllerShapeType::eBOX)
        {
            if (halfForwardExtent <= 0.0f)
            {
                AZ_Error("PhysX Character Controller", false, "PhysX requires half forward extent to be positive.");
                return;
            }

            static_cast<physx::PxBoxController*>(m_pxController)->setHalfForwardExtent(halfForwardExtent);
        }

        else
        {
            AZ_Error("PhysX Character Controller", false, "Half forward extent is only defined for box controllers.");
        }
    }
} // namespace PhysXCharacters
