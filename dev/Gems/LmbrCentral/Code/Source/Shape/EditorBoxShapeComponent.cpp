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
#include "EditorBoxShapeComponent.h"

#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>

#include "BoxShapeComponent.h"

namespace LmbrCentral
{
    namespace ClassConverters
    {
        static bool DeprecateEditorBoxColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    void EditorBoxShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();
        BoxShape::Activate(GetEntityId());       
        EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());        
    }

    void EditorBoxShapeComponent::Deactivate()
    {
        for (int i = 0; i < s_manipulatorCount; ++i)
        {
            if (m_linearManipulators[i] != nullptr)
            {
                delete m_linearManipulators[i];
                m_linearManipulators[i] = nullptr;
            }
        }        

        EntitySelectionEvents::Bus::Handler::BusDisconnect();
        BoxShape::Deactivate();
        EditorBaseShapeComponent::Deactivate();
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
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-shapes.html")
                    ->DataElement(0, &EditorBoxShapeComponent::m_configuration, "Configuration", "Box Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorBoxShapeComponent::ConfigurationChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
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
        auto component = gameEntity->CreateComponent<BoxShapeComponent>()->SetConfiguration(m_configuration);
    }

    void EditorBoxShapeComponent::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
    {
        EditorBaseShapeComponent::OnTransformChanged(local, world);
        BoxShape::OnTransformChanged(local, world);
               
        AZ::Transform tm = world;
        m_worldScale = tm.ExtractScaleExact();
        UpdateManipulators();   
    }

    void EditorBoxShapeComponent::ConfigurationChanged()
    {             
        BoxShape::InvalidateCache(BoxIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        
        ShapeComponentNotificationsBus::Event(GetEntityId(),
            &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

        UpdateManipulators();
    }

    void EditorBoxShapeComponent::OnSelected()
    {
        RegisterManipulators();
    }

    void EditorBoxShapeComponent::OnDeselected()
    {
        UnregisterManipulators();
    }

    void EditorBoxShapeComponent::RegisterManipulators()
    {
        AzToolsFramework::ManipulatorManagerId manipulatorManagerId = 1;
        for (int i = 0; i < s_manipulatorCount; ++i)
        {
            if (m_linearManipulators[i] == nullptr)
            {
                m_linearManipulators[i] = aznew AzToolsFramework::LinearManipulator(GetEntityId());
                m_linearManipulators[i]->SetDisplayType(AzToolsFramework::LinearManipulator::DisplayType::SquarePoint);
                m_linearManipulators[i]->SetColor(AZ::Color(0.06275f, 0.1647f, 0.1647f, 1.0f));

                m_linearManipulators[i]->InstallMouseDownCallback([this](const AzToolsFramework::LinearManipulationData& manipulationData)
                {
                    OnMouseDownManipulator(manipulationData);
                });

                m_linearManipulators[i]->InstallMouseMoveCallback([this, i](const AzToolsFramework::LinearManipulationData& manipulationData)
                {
                    OnMouseMoveManipulator(manipulationData, m_linearManipulators[i]);
                });
            }
            m_linearManipulators[i]->Register(manipulatorManagerId);
        }

        UpdateManipulators();
    }

    void EditorBoxShapeComponent::UnregisterManipulators()
    {
        for (int i = 0; i < s_manipulatorCount; ++i)
        {
            if (m_linearManipulators[i] != nullptr)
            {
                m_linearManipulators[i]->Unregister();
            }
        }
    }

    void EditorBoxShapeComponent::UpdateManipulators()
    {
        // We have to do this because all shape components are displayed with only regard for the 
        // maximum value of its entity's world scale. See the function EditorBaseShapeComponent::DisplayEntity().
        AZ::VectorFloat maxWorldScale = AZ::GetMax(m_worldScale.GetX(), AZ::GetMax(m_worldScale.GetY(), m_worldScale.GetX()));
        AZ::VectorFloat half(0.5f);
        AZ::VectorFloat scaleX = half * maxWorldScale / m_worldScale.GetX();
        AZ::VectorFloat scaleY = half * maxWorldScale / m_worldScale.GetY();
        AZ::VectorFloat scaleZ = half * maxWorldScale / m_worldScale.GetZ();

        AZ::Vector3 boxDimensions = m_configuration.GetDimensions();
        
        
        if (m_linearManipulators[0])
        {
            m_linearManipulators[0]->SetPosition(AZ::Vector3::CreateAxisX() * scaleX * boxDimensions);
            m_linearManipulators[0]->SetDirection(AZ::Vector3::CreateAxisX());
        }
        if (m_linearManipulators[1])
        {
            m_linearManipulators[1]->SetPosition(-AZ::Vector3::CreateAxisX() * scaleX * boxDimensions);
            m_linearManipulators[1]->SetDirection(-AZ::Vector3::CreateAxisX());
        }
        if (m_linearManipulators[2])
        {
            m_linearManipulators[2]->SetPosition(AZ::Vector3::CreateAxisY() * scaleY * boxDimensions);
            m_linearManipulators[2]->SetDirection(AZ::Vector3::CreateAxisY());
        }
        if (m_linearManipulators[3])
        {
            m_linearManipulators[3]->SetPosition(-AZ::Vector3::CreateAxisY() * scaleY * boxDimensions);
            m_linearManipulators[3]->SetDirection(-AZ::Vector3::CreateAxisY());
        }
        if (m_linearManipulators[4])
        {
            m_linearManipulators[4]->SetPosition(AZ::Vector3::CreateAxisZ() * scaleZ * boxDimensions);
            m_linearManipulators[4]->SetDirection(AZ::Vector3::CreateAxisZ());
        }
        if (m_linearManipulators[5])
        {
            m_linearManipulators[5]->SetPosition(-AZ::Vector3::CreateAxisZ() * scaleZ * boxDimensions);
            m_linearManipulators[5]->SetDirection(-AZ::Vector3::CreateAxisZ());
        }

        for (int i = 0; i < s_manipulatorCount; ++i)
        {
            if (m_linearManipulators[i] != nullptr)
            {
                m_linearManipulators[i]->SetBoundsDirty();
            }
        }
    }

    void EditorBoxShapeComponent::OnMouseDownManipulator(const AzToolsFramework::LinearManipulationData& /*manipulationData*/)
    {
        m_dimensionAtMouseDown = m_configuration.GetDimensions();
    }

    void EditorBoxShapeComponent::OnMouseMoveManipulator(const AzToolsFramework::LinearManipulationData& manipulationData, AzToolsFramework::LinearManipulator* manipulator)
    {
        AZ_Assert(manipulator != nullptr, "Manipulator is nullptr!");
        AZ_Assert(manipulator->GetManipulatorId() == manipulationData.m_manipulatorId, "Manipulator Ids mismatch!");

        AZ::VectorFloat maxWorldScaleInverse = AZ::VectorFloat::CreateOne() / AZ::GetMax(m_worldScale.GetX(), AZ::GetMax(m_worldScale.GetY(), m_worldScale.GetX()));
        AZ::VectorFloat delta = manipulationData.m_totalTranslationWorldDelta * 2.0f * maxWorldScaleInverse;

        AZ::VectorFloat zero = AZ::VectorFloat::CreateZero();
        AZ::Vector3 newDimensions;
        if (manipulator == m_linearManipulators[0] || manipulator == m_linearManipulators[1])
        {
            newDimensions = m_dimensionAtMouseDown + AZ::Vector3::CreateAxisX() * delta;
            if (newDimensions.GetX() < zero)
            {
                newDimensions.SetX(zero);
            }
        }
        else if (manipulator == m_linearManipulators[2] || manipulator == m_linearManipulators[3])
        {   
            newDimensions = m_dimensionAtMouseDown + AZ::Vector3::CreateAxisY() * delta;
            if (newDimensions.GetY() < zero)
            {
                newDimensions.SetY(zero);
            }
        }
        else // manipulator == m_linearManipulators[4] || manipulator == m_linearManipulators[5]
        {
            newDimensions = m_dimensionAtMouseDown + AZ::Vector3::CreateAxisZ() * delta;
            if (newDimensions.GetZ() < zero)
            {
                newDimensions.SetZ(zero);
            }
        }

        SetBoxDimensions(newDimensions);
        UpdateManipulators();

        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
            &AzToolsFramework::ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
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
             <Class name="BoxShapeConfig" field="Configuration" version="1" type="{F034FBA2-AC2F-4E66-8152-14DFB90D6283}">
              <Class name="Vector3" field="Dimensions" value="1.0000000 1.0000000 1.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
             </Class>
            </Class>
            */

            // Cache the Configuration
            BoxShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<BoxShapeConfig>(configuration);
            }

            // Convert to EditorBoxShapeComponent
            bool result = classElement.Convert(context, EditorBoxShapeComponentTypeId);
            if (result)
            {
                configIndex = classElement.AddElement<BoxShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<BoxShapeConfig>(context, configuration);
                }
                return true;
            }
            return false;
        }

    } // namespace ClassConverters

} // namespace LmbrCentral
