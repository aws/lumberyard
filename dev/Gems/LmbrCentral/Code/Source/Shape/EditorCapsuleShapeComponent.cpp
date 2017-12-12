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
#include "StdAfx.h"
#include "EditorCapsuleShapeComponent.h"

#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Serialization/EditContext.h>
#include "CapsuleShapeComponent.h"

namespace LmbrCentral
{
    namespace ClassConverters
    {
        static bool DeprecateEditorCapsuleColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

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

            serializeContext->Class<EditorCapsuleShapeComponent, EditorComponentBase>()
                ->Version(1)
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


    namespace ClassConverters
    {
        static bool DeprecateEditorCapsuleColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="EditorCapsuleColliderComponent" field="element" version="1" type="{63247EE1-B081-40D9-8AE2-98E5C738EBD8}">
             <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
              <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
               <Class name="AZ::u64" field="Id" value="5440985258719867508" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
              </Class>
             </Class>
             <Class name="CapsuleColliderConfiguration" field="Configuration" version="1" type="{902BCDA9-C9E5-429C-991B-74C241ED2889}">
              <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
              <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>

            New:
            <Class name="EditorCapsuleShapeComponent" field="element" version="1" type="{06B6C9BE-3648-4DA2-9892-755636EF6E19}">
             <Class name="CapsuleShapeConfig" field="Configuration" version="1" type="{00931AEB-2AD8-42CE-B1DC-FA4332F51501}">
              <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
              <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>
            */

            // Cache the Configuration
            CapsuleShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<CapsuleShapeConfig>(configuration);
            }

            // Convert to EditorCapsuleShapeComponent
            bool result = classElement.Convert<EditorCapsuleShapeComponent>(context);
            if (result)
            {
                configIndex = classElement.AddElement<CapsuleShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<CapsuleShapeConfig>(context, configuration);
                }
                return true;
            }
            return false;
        }

    } // namespace ClassConverters

} // namespace LmbrCentral
