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

#pragma once

#include "BoxShape.h"
#include "EditorBaseShapeComponent.h"
#include <AzCore/std/containers/array.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>

namespace LmbrCentral
{
    class EditorBoxShapeComponent;

    class EditorBoxShapeComponent
        : public EditorBaseShapeComponent
        , public BoxShape
        , private AzToolsFramework::EntitySelectionEvents::Bus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorBoxShapeComponent, EditorBoxShapeComponentTypeId, EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        EditorBoxShapeComponent() = default;

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase implementation
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        void DrawShape(AzFramework::EntityDebugDisplayRequests* displayContext) const override;

        BoxShapeConfig& GetConfiguration() override { return m_configuration; }

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            EditorBaseShapeComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("BoxShapeService", 0x946a0032));
        }

    private:
        AZ_DISABLE_COPY_MOVE(EditorBoxShapeComponent);

        void ConfigurationChanged();

        /// AzToolsFramework::EntitySelectionEvents::Bus::Handler
        void OnSelected() override;
        void OnDeselected() override;

        BoxShapeConfig m_configuration; ///< Stores configuration of a Box for this component

        // Linear Manipulators
        void RegisterManipulators();
        void UnregisterManipulators();
        void UpdateManipulators();

        void OnMouseMoveManipulator(
            const AzToolsFramework::LinearManipulator::Action& action, const AZ::Vector3& axis);

        AZStd::array<AZStd::unique_ptr<AzToolsFramework::LinearManipulator>, 6> m_linearManipulators;
    };
} // namespace LmbrCentral
