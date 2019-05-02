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

#include "Shape.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/ClassConverters.h>

namespace Physics
{
    void ColliderConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ColliderConfiguration>()
                ->Version(3, &Physics::ClassConverters::ColliderConfigurationConverter)
                ->Field("CollisionLayer", &ColliderConfiguration::m_collisionLayer)
                ->Field("CollisionGroupId", &ColliderConfiguration::m_collisionGroupId)
                ->Field("Visible", &ColliderConfiguration::m_visible)
                ->Field("Trigger", &ColliderConfiguration::m_isTrigger)
                ->Field("Exclusive", &ColliderConfiguration::m_isExclusive)
                ->Field("Position", &ColliderConfiguration::m_position)
                ->Field("Rotation", &ColliderConfiguration::m_rotation)
                ->Field("MaterialSelection", &ColliderConfiguration::m_materialSelection)
                ->Field("propertyVisibilityFlags", &ColliderConfiguration::m_propertyVisibilityFlags)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ColliderConfiguration>("ColliderConfiguration", "Configuration for a collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_visible, "Draw Collider", "Draw the collider.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_isTrigger, "Trigger", "If set, this collider will act as a trigger")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ColliderConfiguration::GetIsTriggerVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_collisionLayer, "Collision Layer", "The collision layer assigned to the collider")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ColliderConfiguration::GetCollisionLayerVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_collisionGroupId, "Collides With", "The collision group containing the layers this collider collides with")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ColliderConfiguration::GetCollisionLayerVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_position, "Offset", "Local offset from the rigid body")
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_rotation, "Rotation", "Local rotation relative to the rigid body")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_materialSelection, "Material", "Assign material library and select material")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ColliderConfiguration::GetMaterialSelectionVisibility)
                    ;
            }
        }
    }

    AZ::Crc32 ColliderConfiguration::GetPropertyVisibility(PropertyVisibility property) const
    {
        return (m_propertyVisibilityFlags & property) != 0 ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void ColliderConfiguration::SetPropertyVisibility(PropertyVisibility property, bool isVisible)
    {
        if (isVisible)
        {
            m_propertyVisibilityFlags |= property;
        }
        else
        {
            m_propertyVisibilityFlags &= ~property;
        }
    }

    AZ::Crc32 ColliderConfiguration::GetIsTriggerVisibility() const
    {
        return GetPropertyVisibility(IsTrigger);
    }

    AZ::Crc32 ColliderConfiguration::GetCollisionLayerVisibility() const
    {
        return GetPropertyVisibility(CollisionLayer);
    }

    AZ::Crc32 ColliderConfiguration::GetMaterialSelectionVisibility() const
    {
        return GetPropertyVisibility(MaterialSelection);
    }
}