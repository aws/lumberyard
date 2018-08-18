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
#include "MeshColliderComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>

#include <IPhysics.h>
#include <IStatObj.h>
#include <MathConversion.h>

namespace LmbrCentral
{
    using AzFramework::ColliderComponentRequestBus;
    using AzFramework::ColliderComponentEventBus;

    void MeshColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<MeshColliderComponent, AZ::Component>()
                ->Version(1)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<MeshColliderComponent>(
                    "Mesh Collider", "The Mesh Collider component specifies that the collider geometry is provided by a mesh component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Physics")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/ColliderMesh.png")
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-physics-mesh-collider.html")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                ;
            }
        }
    }

    void MeshColliderComponent::Activate()
    {
        m_meshReady = false;

        ColliderComponentRequestBus::Handler::BusConnect(GetEntityId());
        MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void MeshColliderComponent::Deactivate()
    {
        ColliderComponentRequestBus::Handler::BusDisconnect();
        MeshComponentNotificationBus::Handler::BusDisconnect();
    }

    void MeshColliderComponent::OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>&)
    {
        m_meshReady = true;
        EBUS_EVENT_ID(GetEntityId(), ColliderComponentEventBus, OnColliderChanged);
    }

    void MeshColliderComponent::OnMeshDestroyed()
    {
        m_meshReady = false;
        EBUS_EVENT_ID(GetEntityId(), ColliderComponentEventBus, OnColliderChanged);
    }

    AZ::Transform GetTransformForColliderGeometry(const IPhysicalEntity& physicalEntity, const AZ::EntityId& colliderEntityId)
    {
        // get collider's world transform
        AZ::Transform colliderTransform = AZ::Transform::Identity();
        EBUS_EVENT_ID_RESULT(colliderTransform, colliderEntityId, AZ::TransformBus, GetWorldTM);

        // get physical entity's world transform
        AZ_Assert(physicalEntity.GetiForeignData() == PHYS_FOREIGN_ID_COMPONENT_ENTITY, "Could not retrieve AZ::EntityId from IPhysicalEntity");
        AZ::EntityId physicalEntityId = static_cast<AZ::EntityId>(physicalEntity.GetForeignData(PHYS_FOREIGN_ID_COMPONENT_ENTITY));

        AZ::Transform physicalEntityTransform = AZ::Transform::Identity();
        EBUS_EVENT_ID_RESULT(physicalEntityTransform, physicalEntityId, AZ::TransformBus, GetWorldTM);

        // return transform with relative position and rotation, but absolute scale
        physicalEntityTransform.ExtractScale();
        return physicalEntityTransform.GetInverseFast() * colliderTransform;
    }

    int MeshColliderComponent::AddColliderToPhysicalEntity(IPhysicalEntity& physicalEntity, int nextPartId)
    {
        if (!m_meshReady)
        {
            return NoPartsAdded;
        }

        IStatObj* entityStatObj = nullptr;
        EBUS_EVENT_ID_RESULT(entityStatObj, GetEntityId(), LegacyMeshComponentRequestBus, GetStatObj);
        if (!entityStatObj)
        {
            return NoPartsAdded;
        }

        // configure pe_geomparams
        pe_geomparams geometryParameters;
        geometryParameters.flags = geom_collides | geom_floats | geom_colltype_ray;

        // set full local transform matrix
        // some (but not all!) meshes support non-uniform scaling
        AZ::Transform geometryAzTransform = GetTransformForColliderGeometry(physicalEntity, GetEntityId());
        Matrix34 geometryTransform = AZTransformToLYTransform(geometryAzTransform);
        geometryParameters.pMtx3x4 = &geometryTransform;

        int finalPartId = entityStatObj->Physicalize(&physicalEntity, &geometryParameters, nextPartId);
        return finalPartId;
    }
} // namespace LmbrCentral
