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
#include "EditorCylinderShapeComponent.h"

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "CylinderShapeComponent.h"

namespace LmbrCentral
{
    namespace ClassConverters
    {
        static bool DeprecateEditorCylinderColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

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

            serializeContext->Class<EditorCylinderShapeComponent, EditorComponentBase>()
                ->Version(1)
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
    }

    namespace ClassConverters
    {
        static bool DeprecateEditorCylinderColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="EditorCylinderColliderComponent" field="element" version="1" type="{1C10CEE7-0A5C-4D4A-BBD9-5C3B6C6FE844}">
             <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
              <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
               <Class name="AZ::u64" field="Id" value="3854466472272486634" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
              </Class>
             </Class>
             <Class name="CylinderColliderConfiguration" field="Configuration" version="1" type="{E1DCB833-EFC4-43AC-97B0-4E07AA0DFAD9}">
              <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
              <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>

            New:
            <Class name="EditorCylinderShapeComponent" field="element" version="1" type="{D5FC4745-3C75-47D9-8C10-9F89502487DE}">
             <Class name="CylinderShapeConfig" field="Configuration" version="1" type="{53254779-82F1-441E-9116-81E1FACFECF4}">
              <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
              <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>
            */

            // Cache the Configuration
            CylinderShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<CylinderShapeConfig>(configuration);
            }

            // Convert to EditorCylinderShapeComponent
            bool result = classElement.Convert<EditorCylinderShapeComponent>(context);
            if (result)
            {
                configIndex = classElement.AddElement<CylinderShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<CylinderShapeConfig>(context, configuration);
                }
                return true;
            }
            return false;
        }

    } // namespace ClassConverters

} // namespace LmbrCentral
