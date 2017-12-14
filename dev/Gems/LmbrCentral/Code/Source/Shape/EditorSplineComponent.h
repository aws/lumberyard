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

#include "SplineComponent.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/Manipulators/EditorVertexSelection.h>
#include <AzToolsFramework/Manipulators/HoverSelection.h>
#include <AzToolsFramework/Manipulators/SplineSelectionManipulator.h>
#include <LmbrCentral/Shape/SplineComponentBus.h>

namespace AzToolsFramework
{
    class SplineSelectionManipulator;
}

namespace LmbrCentral
{
    /**
     * SplineHoverSelection is a concrete implementation of HoverSelection wrapping a Spline and
     * SplineManipulator. The underlying manipulators are used to control selection.
     */
    class SplineHoverSelection
        : public AzToolsFramework::HoverSelection
    {
    public:
        SplineHoverSelection();
        ~SplineHoverSelection();

        void Create(AZ::EntityId entityId, AzToolsFramework::ManipulatorManagerId managerId) override;
        void Destroy() override;
        void Register(AzToolsFramework::ManipulatorManagerId managerId) override;
        void Unregister() override;
        void SetBoundsDirty() override;
        void Refresh() override;

        AZStd::weak_ptr<AZ::Spline> m_spline;

    private:
        AZ_DISABLE_COPY_MOVE(SplineHoverSelection)

        AZStd::unique_ptr<AzToolsFramework::SplineSelectionManipulator> m_splineSelectionManipulator = nullptr; ///< Manipulator for adding points to spline.
    };

    /**
     * Editor representation of SplineComponent.
     */
    class EditorSplineComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private SplineComponentRequestBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private AzToolsFramework::ToolsApplicationEvents::Bus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorSplineComponent, "{5B29D788-4885-4D56-BD9B-C0C45BE08EC1}", EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorSplineComponent() = default;

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase implementation
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        AZ_DISABLE_COPY_MOVE(EditorSplineComponent)

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("SplineService", 0x2b674d3c));
            provided.push_back(AZ_CRC("VertexContainerService", 0x22cf8e10));
        }

        /// AzToolsFramework::EntitySelectionEvents::Bus::Handler
        void OnSelected() override;
        void OnDeselected() override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // SplineComponentRequestBus handler
        AZ::ConstSplinePtr GetSpline() override;
        void ChangeSplineType(AZ::u64 splineType) override;
        void SetClosed(bool closed) override;
        bool GetVertex(size_t index, AZ::Vector3& vertex) const override;
        void AddVertex(const AZ::Vector3& vertex) override;
        bool UpdateVertex(size_t index, const AZ::Vector3& vertex) override;
        bool InsertVertex(size_t index, const AZ::Vector3& vertex) override;
        bool RemoveVertex(size_t index) override;
        void SetVertices(const AZStd::vector<AZ::Vector3>& vertices) override;
        void ClearVertices() override;
        size_t Size() const override;
        bool Empty() const override;

        // Manipulator handling
        void CreateManipulators();

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntity(bool& handled) override;

        // AzToolsFramework::ToolsApplicationEvents
        void AfterUndoRedo() override;

        void OnChangeSplineType();

        AzToolsFramework::EditorVertexSelectionVariable<AZ::Vector3> m_vertexSelection; ///< Handles all manipulator interactions with vertices (inserting and translating).

        SplineCommon m_splineCommon; ///< Stores common spline functionality and properties
    };
} // namespace LmbrCentral
