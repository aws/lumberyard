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
#include "EditorCapsuleShapeComponent.h"

#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Serialization/EditContext.h>
#include "CapsuleShapeComponent.h"
#include "ShapeComponentConverters.h"

namespace LmbrCentral
{
    void EditorCapsuleShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            // Deprecate: EditorCapsuleColliderComponent -> EditorCapsuleShapeComponent
            serializeContext->ClassDeprecate(
                "EditorCapsuleColliderComponent",
                "{63247EE1-B081-40D9-8AE2-98E5C738EBD8}",
                &ClassConverters::DeprecateEditorCapsuleColliderComponent
                );

            serializeContext->Class<EditorCapsuleShapeComponent, EditorBaseShapeComponent>()
                ->Version(2, &ClassConverters::UpgradeEditorCapsuleShapeComponent)
                ->Field("Configuration", &EditorCapsuleShapeComponent::m_configuration)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorCapsuleShapeComponent>("Capsule Shape", "The Capsule Shape component creates a capsule around the associated entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Capsule_Shape.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Capsule_Shape.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-shapes.html")
                    ->DataElement(0, &EditorCapsuleShapeComponent::m_configuration, "Configuration", "Capsule Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCapsuleShapeComponent::ConfigurationChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
            }
        }
    }

    void EditorCapsuleShapeComponent::DrawShape(AzFramework::EntityDebugDisplayRequests* displayContext) const
    {
        float straightSideLength = AZStd::max(0.f, m_configuration.GetHeight() - (2.f * m_configuration.GetRadius()));
        if (displayContext)
        {
            displayContext->SetColor(s_shapeWireColor);
            displayContext->DrawWireCapsule(AZ::Vector3::CreateZero(), AZ::Vector3::CreateAxisZ(1), m_configuration.GetRadius(), straightSideLength);
        }
    }

    void EditorCapsuleShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();
        CapsuleShape::Activate(GetEntityId());        
    }

    void EditorCapsuleShapeComponent::Deactivate()
    {        
        CapsuleShape::Deactivate();
        EditorBaseShapeComponent::Deactivate();
    }

    void EditorCapsuleShapeComponent::ConfigurationChanged()
    {        
        InvalidateCache(CapsuleShapeComponent::CapsuleIntersectionDataCache::Obsolete_ShapeChange);
    }

    AZ::Crc32 EditorCapsuleShapeComponent::OnConfigurationChanged()
    {
        // if radius is large enough that shape is essentially a sphere, ensure height remains accurate
        if ((m_configuration.m_height - 2 * m_configuration.m_radius) < std::numeric_limits<float>::epsilon())
        {
            m_configuration.m_height = 2 * m_configuration.m_radius;
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        return AZ::Edit::PropertyRefreshLevels::None;
    }

    void EditorCapsuleShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto component = gameEntity->CreateComponent<CapsuleShapeComponent>();
        if (component)
        {
            component->SetConfiguration(m_configuration);
        }
    }
} // namespace LmbrCentral
