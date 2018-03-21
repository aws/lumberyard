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
#include "SphereShapeComponent.h"
#include "EditorSphereShapeComponent.h"

#include <AzCore/Serialization/EditContext.h>

#include "ShapeComponentConverters.h"

namespace LmbrCentral
{
    void EditorSphereShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            // Note: this must be called by the first EditorShapeComponent to have it's reflect function called. Which happens to be this one for now.
            EditorBaseShapeComponent::Reflect(*serializeContext);

            // Deprecate: EditorSphereColliderComponent -> EditorSphereShapeComponent
            serializeContext->ClassDeprecate(
                "EditorSphereColliderComponent",
                "{9A12FC39-60D2-4237-AC79-11FEDFEDB851}",
                &ClassConverters::DeprecateEditorSphereColliderComponent
                );

            serializeContext->Class<EditorSphereShapeComponent, EditorBaseShapeComponent>()
                ->Version(2, &ClassConverters::UpgradeEditorSphereShapeComponent)
                ->Field("Configuration", &EditorSphereShapeComponent::m_configuration)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorSphereShapeComponent>(
                    "Sphere Shape", "The Sphere Shape component creates a sphere around the associated entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Sphere_Shape.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Sphere_Shape.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-shapes.html")
                    ->DataElement(0, &EditorSphereShapeComponent::m_configuration, "Configuration", "Sphere Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSphereShapeComponent::ConfigurationChanged)    
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
            }
            
        }
    }

    void EditorSphereShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();
        SphereShape::Activate(GetEntityId());        
    }

    void EditorSphereShapeComponent::Deactivate()
    {
        SphereShape::Deactivate();        
        EditorBaseShapeComponent::Deactivate();
    }

    void EditorSphereShapeComponent::DrawShape(AzFramework::EntityDebugDisplayRequests* displayContext) const
    {
        displayContext->SetColor(EditorBaseShapeComponent::s_shapeColor);
        displayContext->DrawBall(AZ::Vector3::CreateZero(), m_configuration.GetRadius());
        displayContext->SetColor(EditorBaseShapeComponent::s_shapeWireColor);
        displayContext->DrawWireSphere(AZ::Vector3::CreateZero(), m_configuration.GetRadius());
    }

    void EditorSphereShapeComponent::ConfigurationChanged()
    {
        SphereShape::InvalidateCache(SphereIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        ShapeComponentNotificationsBus::Event(GetEntityId(), &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void EditorSphereShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto component = gameEntity->CreateComponent<SphereShapeComponent>();
        if (component)
        {
            component->SetConfiguration(m_configuration);
        }
    }
} // namespace LmbrCentral
