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
#include "ShapeComponentConverters.h"

namespace LmbrCentral
{
    const AZStd::array<AZ::Vector3, 6> s_boxAxes =
    { {
        AZ::Vector3::CreateAxisX(), -AZ::Vector3::CreateAxisX(),
        AZ::Vector3::CreateAxisY(), -AZ::Vector3::CreateAxisY(),
        AZ::Vector3::CreateAxisZ(), -AZ::Vector3::CreateAxisZ()
    } };

    /**
     * Pass a single axis, and return not of elements
     * Example: In -> (1, 0, 0) Out -> (0, 1, 1)
     */
    static AZ::Vector3 NotAxis(const AZ::Vector3& offset)
    {
        return AZ::Vector3::CreateOne() - AZ::Vector3(
            fabsf(AzToolsFramework::Sign(offset.GetX())),
            fabsf(AzToolsFramework::Sign(offset.GetY())),
            fabsf(AzToolsFramework::Sign(offset.GetZ())));
    }

    void EditorBoxShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();
        BoxShape::Activate(GetEntityId());
        EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
    }

    void EditorBoxShapeComponent::Deactivate()
    {
        UnregisterManipulators();

        EntitySelectionEvents::Bus::Handler::BusDisconnect();
        BoxShape::Deactivate();
        EditorBaseShapeComponent::Deactivate();
    }

    void EditorBoxShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate: EditorBoxColliderComponent -> EditorBoxShapeComponent
            serializeContext->ClassDeprecate(
                "EditorBoxColliderComponent",
                "{E1707478-4F5F-4C28-A31A-EF42B7BD2A68}",
                &ClassConverters::DeprecateEditorBoxColliderComponent);

            serializeContext->Class<EditorBoxShapeComponent, EditorBaseShapeComponent>()
                ->Version(2, &ClassConverters::UpgradeEditorBoxShapeComponent)
                ->Field("Configuration", &EditorBoxShapeComponent::m_configuration);

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
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorBoxShapeComponent::m_configuration, "Configuration", "Box Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorBoxShapeComponent::ConfigurationChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void EditorBoxShapeComponent::DrawShape(AzFramework::EntityDebugDisplayRequests* displayContext) const
    {
        const AZ::Vector3 boxMin = m_configuration.GetDimensions() * -0.5f;
        const AZ::Vector3 boxMax = m_configuration.GetDimensions() * 0.5f;
        displayContext->SetColor(m_shapeColor);
        displayContext->DrawSolidBox(boxMin, boxMax);
        displayContext->SetColor(m_shapeWireColor);
        displayContext->DrawWireBox(boxMin, boxMax);
    }

    void EditorBoxShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<BoxShapeComponent>()->SetConfiguration(m_configuration);
    }

    void EditorBoxShapeComponent::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
    {
        EditorBaseShapeComponent::OnTransformChanged(local, world);
        BoxShape::OnTransformChanged(local, world);
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
        const AzToolsFramework::ManipulatorManagerId manipulatorManagerId = AzToolsFramework::ManipulatorManagerId(1);
        for (size_t i = 0; i < m_linearManipulators.size(); ++i)
        {
            AZStd::unique_ptr<AzToolsFramework::LinearManipulator>& linearManipulator = m_linearManipulators[i];

            if (linearManipulator == nullptr)
            {
                linearManipulator = AZStd::make_unique<AzToolsFramework::LinearManipulator>(GetEntityId());
                linearManipulator->SetAxis(s_boxAxes[i]);

                AzToolsFramework::ManipulatorViews views;
                views.emplace_back(AzToolsFramework::CreateManipulatorViewQuadBillboard(
                    AZ::Color(0.06275f, 0.1647f, 0.1647f, 1.0f), 0.05f));
                linearManipulator->SetViews(AZStd::move(views));

                const AZ::Vector3 axis = linearManipulator->GetAxis();
                linearManipulator->InstallMouseMoveCallback([this, axis](
                    const AzToolsFramework::LinearManipulator::Action& action)
                {
                    OnMouseMoveManipulator(action, axis);
                });
            }

            linearManipulator->Register(manipulatorManagerId);
        }

        UpdateManipulators();
    }

    void EditorBoxShapeComponent::UnregisterManipulators()
    {
        for (AZStd::unique_ptr<AzToolsFramework::LinearManipulator>& linearManipulator : m_linearManipulators)
        {
            if (linearManipulator != nullptr)
            {
                linearManipulator->Unregister();
            }
        }
    }

    void EditorBoxShapeComponent::UpdateManipulators()
    {
        const AZ::Vector3 boxDimensions = m_configuration.GetDimensions();
        for (size_t i = 0; i < m_linearManipulators.size(); ++i)
        {
            if (AZStd::unique_ptr<AzToolsFramework::LinearManipulator>& linearManipulator = m_linearManipulators[i])
            {
                linearManipulator->SetPosition(s_boxAxes[i] * AZ::VectorFloat(0.5f) * boxDimensions);
                linearManipulator->SetBoundsDirty();
            }
        }
    }

    void EditorBoxShapeComponent::OnMouseMoveManipulator(
        const AzToolsFramework::LinearManipulator::Action& action, const AZ::Vector3& axis)
    {
        // calculate the amount of displacement along an axis this manipulator has moved
        // clamp movement so it cannot go negative based on axis direction
        const AZ::Vector3 axisDisplacement =
            action.LocalPosition().GetAbs() * 2.0f
                * action.LocalPosition().GetNormalized().Dot(axis).GetMax(AZ::VectorFloat::CreateZero());

        // update dimensions - preserve dimensions not effected by this
        // axis, and update current axis displacement
        SetBoxDimensions((NotAxis(axis)
            * m_configuration.GetDimensions()).GetMax(axisDisplacement));

        UpdateManipulators();

        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
            &AzToolsFramework::ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
    }
} // namespace LmbrCentral