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
#include "EditorBoxShapeComponent.h"

#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>

#include "BoxShapeComponent.h"
#include "EditorShapeComponentConverters.h"
#include "ShapeDisplay.h"

namespace LmbrCentral
{
    void EditorBoxShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate: EditorBoxColliderComponent -> EditorBoxShapeComponent
            serializeContext->ClassDeprecate(
                "EditorBoxColliderComponent",
                "{E1707478-4F5F-4C28-A31A-EF42B7BD2A68}",
                &ClassConverters::DeprecateEditorBoxColliderComponent)
                ;

            serializeContext->Class<EditorBoxShapeComponent, EditorBaseShapeComponent>()
                ->Version(3, &ClassConverters::UpgradeEditorBoxShapeComponent)
                ->Field("BoxShape", &EditorBoxShapeComponent::m_boxShape)
                ;

            if (auto editContext = serializeContext->GetEditContext())
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
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorBoxShapeComponent::m_boxShape, "Box Shape", "Box Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorBoxShapeComponent::ConfigurationChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
            }
        }
    }

    void EditorBoxShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();

        m_boxManipulator.SetHandler(this);
        m_boxShape.Activate(GetEntityId());
        EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorBoxShapeComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        EntitySelectionEvents::Bus::Handler::BusDisconnect();
        m_boxShape.Deactivate();
        m_boxManipulator.SetHandler(nullptr);
        m_boxManipulator.OnDeselect();
        EditorBaseShapeComponent::Deactivate();
    }

    void EditorBoxShapeComponent::DisplayEntity(bool& handled)
    {
        DisplayShape(handled,
            [this]() { return CanDraw(); },
            [this](AzFramework::EntityDebugDisplayRequests* displayContext)
            {
                DrawBoxShape(
                    { m_shapeColor, m_shapeWireColor, m_displayFilled },
                    m_boxShape.GetBoxConfiguration(), *displayContext);
            },
            m_boxShape.GetCurrentTransform());
    }

    void EditorBoxShapeComponent::ConfigurationChanged()
    {
        m_boxShape.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(GetEntityId(),
            &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
        m_boxManipulator.RefreshManipulators();
    }

    void EditorBoxShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (BoxShapeComponent* boxShapeComponent = gameEntity->CreateComponent<BoxShapeComponent>())
        {
            boxShapeComponent->SetConfiguration(m_boxShape.GetBoxConfiguration());
        }

        if (m_visibleInGameView)
        {
            if (auto component = gameEntity->CreateComponent<BoxShapeDebugDisplayComponent>())
            {
                component->SetConfiguration(m_boxShape.GetBoxConfiguration());
            }
        }
    }

    void EditorBoxShapeComponent::OnSelected()
    {
        m_boxManipulator.OnSelect(GetEntityId());
    }

    void EditorBoxShapeComponent::OnDeselected()
    {
        m_boxManipulator.OnDeselect();
    }

    AZ::Vector3 EditorBoxShapeComponent::GetDimensions()
    {
        return m_boxShape.GetBoxDimensions();
    }

    void EditorBoxShapeComponent::SetDimensions(const AZ::Vector3& dimensions)
    {
        m_boxShape.SetBoxDimensions(dimensions);
    }

    AZ::Transform EditorBoxShapeComponent::GetCurrentTransform()
    {
        return m_boxShape.GetCurrentTransform();
    }

    void EditorBoxShapeComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        m_boxManipulator.RefreshManipulators();
    }
} // namespace LmbrCentral