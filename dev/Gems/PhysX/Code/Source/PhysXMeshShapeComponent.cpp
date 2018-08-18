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

#include <PhysXMeshShapeComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace PhysX
{
    void PhysXMeshShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysXMeshShapeComponent, AZ::Component>()
                ->Version(1)
                ->Field("PxMesh", &PhysXMeshShapeComponent::m_meshColliderAsset)
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PhysXMeshShapeComponent>(
                    "PhysX Mesh Shape", "Provides PhysX convex or triangular mesh shape")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->DataElement(0, &PhysXMeshShapeComponent::m_meshColliderAsset, "PxMesh", "PhysX Mesh Collider asset")
                ;
            }
        }
    }

    void PhysXMeshShapeComponent::Activate()
    {
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        PhysXMeshShapeComponentRequestBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void PhysXMeshShapeComponent::Deactivate()
    {
        LmbrCentral::ShapeComponentRequestsBus::Handler::BusDisconnect();
        PhysXMeshShapeComponentRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void PhysXMeshShapeComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
        LmbrCentral::ShapeComponentNotificationsBus::Event(GetEntityId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged, LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    // Default implementation for the shape bus
    AZ::Aabb PhysXMeshShapeComponent::GetEncompassingAabb()
    {
        AZ_Error("PhysX", false, "GetEncompassingAabb: Shape interface not implemented in PhysXMeshShapeComponent.");
        return AZ::Aabb();
    }

    bool PhysXMeshShapeComponent::IsPointInside(const AZ::Vector3& point)
    {
        AZ_Error("PhysX", false, "IsPointInside: Shape interface not implemented in PhysXMeshShapeComponent.");
        return false;
    }

    float PhysXMeshShapeComponent::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        AZ_Error("PhysX", false, "DistanceSquaredFromPoint: Shape interface not implemented in PhysXMeshShapeComponent.");
        return (m_currentTransform.GetPosition().GetDistanceSq(point));
    }
} // namespace PhysX
