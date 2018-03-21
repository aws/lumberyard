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
#include "EditorCylinderShapeComponent.h"

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "CylinderShapeComponent.h"
#include "ShapeComponentConverters.h"

namespace LmbrCentral
{
    void EditorCylinderShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            // Deprecate: EditorCylinderColliderComponent -> EditorCylinderShapeComponent
            serializeContext->ClassDeprecate(
                "EditorCylinderColliderComponent",
                "{1C10CEE7-0A5C-4D4A-BBD9-5C3B6C6FE844}",
                &ClassConverters::DeprecateEditorCylinderColliderComponent
                );

            serializeContext->Class<EditorCylinderShapeComponent, EditorBaseShapeComponent>()
                ->Version(2, &ClassConverters::UpgradeEditorCylinderShapeComponent)
                ->Field("Configuration", &EditorCylinderShapeComponent::m_configuration)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorCylinderShapeComponent>(
                    "Cylinder Shape", "The Cylinder Shape component creates a cylinder around the associated entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Cylinder_Shape.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Cylinder_Shape.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-shapes.html")
                    ->DataElement(0, &EditorCylinderShapeComponent::m_configuration, "Configuration", "Cylinder Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCylinderShapeComponent::ConfigurationChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
            }
        }
    }

    void EditorCylinderShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();
        CylinderShape::Activate(GetEntityId());
    }

    void EditorCylinderShapeComponent::Deactivate()
    {
        CylinderShape::Deactivate();
        EditorBaseShapeComponent::Deactivate();
    }

    void EditorCylinderShapeComponent::DrawShape(AzFramework::EntityDebugDisplayRequests* displayContext) const
    {
        if (displayContext)
        {
            displayContext->SetColor(s_shapeColor);
            displayContext->DrawSolidCylinder(AZ::Vector3::CreateZero(), AZ::Vector3::CreateAxisZ(1.f), m_configuration.GetRadius(), m_configuration.GetHeight());

            displayContext->SetColor(s_shapeWireColor);
            displayContext->DrawWireCylinder(AZ::Vector3::CreateZero(), AZ::Vector3::CreateAxisZ(1.f), m_configuration.GetRadius(), m_configuration.GetHeight());
        }
    }

    void EditorCylinderShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto component = gameEntity->CreateComponent<CylinderShapeComponent>()->SetConfiguration(m_configuration);
    }

    void EditorCylinderShapeComponent::ConfigurationChanged() 
    {
        CylinderShape::InvalidateCache(CylinderIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        ShapeComponentNotificationsBus::Event(GetEntityId(), &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }
} // namespace LmbrCentral
