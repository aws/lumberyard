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
#include "BoxShapeComponent.h"
#include "EditorBoxShapeComponent.h"

#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace LmbrCentral
{
    namespace ClassConverters
    {
        static bool DeprecateEditorBoxColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    void EditorBoxShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            // Deprecate: EditorBoxColliderComponent -> EditorBoxShapeComponent
            serializeContext->ClassDeprecate(
                "EditorBoxColliderComponent",
                "{E1707478-4F5F-4C28-A31A-EF42B7BD2A68}",
                &ClassConverters::DeprecateEditorBoxColliderComponent
                );

            serializeContext->Class<EditorBoxShapeComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Configuration", &EditorBoxShapeComponent::m_configuration)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorBoxShapeComponent>(
                    "Box Shape", "The Box Shape component creates a box around the associated entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Box_Shape.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Box_Shape.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorBoxShapeComponent::m_configuration, "Configuration", "Box Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                        ;
            }
        }
    }

    void EditorBoxShapeComponent::DrawShape(AzFramework::EntityDebugDisplayRequests* displayContext) const
    {
        AZ::Vector3 boxMin = m_configuration.GetDimensions() * -0.5;
        AZ::Vector3 boxMax = m_configuration.GetDimensions() * 0.5;
        displayContext->SetColor(s_shapeColor);
        displayContext->DrawSolidBox(boxMin, boxMax);
        displayContext->SetColor(s_shapeWireColor);
        displayContext->DrawWireBox(boxMin, boxMax);
    }

    void EditorBoxShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto component = gameEntity->CreateComponent<BoxShapeComponent>();
        if (component)
        {
            component->m_configuration = m_configuration;
        }
    }


    namespace ClassConverters
    {
        static bool DeprecateEditorBoxColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="EditorBoxColliderComponent" field="element" version="1" type="{E1707478-4F5F-4C28-A31A-EF42B7BD2A68}">
             <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
              <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
               <Class name="AZ::u64" field="Id" value="11026045358802895563" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
              </Class>
             </Class>
             <Class name="BoxColliderConfiguration" field="Configuration" version="1" type="{282E47CB-9F6D-47AE-A210-4CE879527EFD}">
              <Class name="Vector3" field="Size" value="1.0000000 1.0000000 1.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
             </Class>
            </Class>

            New:
            <Class name="EditorBoxShapeComponent" field="element" version="1" type="{2ADD9043-48E8-4263-859A-72E0024372BF}">
             <Class name="BoxShapeConfiguration" field="Configuration" version="1" type="{F034FBA2-AC2F-4E66-8152-14DFB90D6283}">
              <Class name="Vector3" field="Dimensions" value="1.0000000 1.0000000 1.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
             </Class>
            </Class>
            */

            // Cache the Configuration
            BoxShapeConfiguration configuration;
            int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<BoxShapeConfiguration>(configuration);
            }

            // Convert to EditorBoxShapeComponent
            bool result = classElement.Convert(context, "{2ADD9043-48E8-4263-859A-72E0024372BF}");
            if (result)
            {
                configIndex = classElement.AddElement<BoxShapeConfiguration>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<BoxShapeConfiguration>(context, configuration);
                }
                return true;
            }
            return false;
        }

    } // namespace ClassConverters

} // namespace LmbrCentral
