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

#include <PhysX_precompiled.h>
#include <PxPhysicsAPI.h>
#include <Include/PhysX/PhysXSystemComponentBus.h>
#include <Include/PhysX/PhysXMeshShapeComponentBus.h>
#include <PhysXColliderComponent.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>
#include <LmbrCentral/Shape/CompoundShapeComponentBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>

namespace PhysX
{
    void PhysXColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PhysXColliderComponent, AZ::Component>()
                ->Version(1)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<PhysXColliderComponent>(
                    "PhysX Collider", "Collider component that can use a shape or a mesh asset to define its collision in PhysX")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/PrimitiveCollider.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/PrimitiveCollider.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }
    }

    Physics::Ptr<Physics::ShapeConfiguration> PhysXColliderComponent::GetShapeConfigFromEntity()
    {
        const AZ::EntityId& entityId = GetEntityId();
        AZ::Crc32 shapeType;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(shapeType, entityId,
            &LmbrCentral::ShapeComponentRequests::GetShapeType);

        if (shapeType == AZ_CRC("Sphere", 0x55f96687))
        {
            LmbrCentral::SphereShapeConfiguration config;
            LmbrCentral::SphereShapeComponentRequestsBus::EventResult(config, entityId,
                &LmbrCentral::SphereShapeComponentRequests::GetSphereConfiguration);
            return Physics::SphereShapeConfiguration::Create(config.GetRadius());
        }

        else if (shapeType == AZ_CRC("Box", 0x08a9483a))
        {
            LmbrCentral::BoxShapeConfiguration config;
            LmbrCentral::BoxShapeComponentRequestsBus::EventResult(config, entityId,
                &LmbrCentral::BoxShapeComponentRequests::GetBoxConfiguration);
            // config.m_dimensions[i] is total length of side, BoxShapeConfiguration uses half extents
            AZ::Vector3 halfExtents = config.GetDimensions() * 0.5f;
            return Physics::BoxShapeConfiguration::Create(halfExtents);
        }

        else if (shapeType == AZ_CRC("Capsule", 0xc268a183))
        {
            LmbrCentral::CapsuleShapeConfiguration config;
            LmbrCentral::CapsuleShapeComponentRequestsBus::EventResult(config, entityId,
                &LmbrCentral::CapsuleShapeComponentRequests::GetCapsuleConfiguration);
            float hh = AZStd::max(0.f, (0.5f * config.GetHeight()) - config.GetRadius());
            return Physics::CapsuleShapeConfiguration::Create(AZ::Vector3(0.0f, 0.0f, hh), AZ::Vector3(0.0f, 0.0f, -hh), config.GetRadius());
        }

        else if (shapeType == AZ_CRC("PhysXMesh", 0xe86bc8a6))
        {
            // It is a valid case to have PhysXMesh shape but we don't create any configurations for this
            return nullptr;
        }

        AZ_Warning("PhysX", false, "Shape not supported in PhysX.");
        return nullptr;
    }

    void PhysXColliderComponent::Activate()
    {
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        PhysXColliderComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void PhysXColliderComponent::Deactivate()
    {
        PhysXColliderComponentRequestBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
    }

    void PhysXColliderComponent::OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged)
        {
            AZ_Warning("[PhysX Primitive Collider Component]", false, "PhysX Primitive collider component does not currently support dynamic changes to collider shape...Entity %s [%llu]..."
                , m_entity->GetName().c_str(), m_entity->GetId());
        }
    }
} // namespace PhysX
