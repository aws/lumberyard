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
#include <AzToolsFramework/Manipulators/TranslationManipulator.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <LmbrCentral/Shape/SplineComponentBus.h>

namespace LmbrCentral
{
    /**
     * Editor representation of SplineComponent.
     */
    class EditorSplineComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private SplineComponentRequestBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:

        AZ_EDITOR_COMPONENT(EditorSplineComponent, "{5B29D788-4885-4D56-BD9B-C0C45BE08EC1}", EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorSplineComponent() = default;
        ~EditorSplineComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EditorComponentBase implementation
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        ////////////////////////////////////////////////////////////////////////

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("SplineService", 0x2b674d3c));
            provided.push_back(AZ_CRC("VertexContainerService", 0x22cf8e10));
        }

    private:
        //////////////////////////////////////////////////////////////////////////
        /// AzToolsFramework::EntitySelectionEvents::Bus::Handler
        void OnSelected() override;
        void OnDeselected() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////
        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        //////////////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Manipulator handling
        void RegisterManipulators();
        void UnregisterManipulators();
        void RefreshManipulators();
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SplineComponentRequestBus handler
        AZ::ConstSplinePtr GetSpline() override;
        void ChangeSplineType(AZ::u64 splineType) override;
        void SetClosed(bool closed) override;

        void AddVertex(const AZ::Vector3& vertex) override;
        void UpdateVertex(size_t index, const AZ::Vector3& vertex) override;
        void InsertVertex(size_t index, const AZ::Vector3& vertex) override;
        void RemoveVertex(size_t index) override;
        void SetVertices(const AZStd::vector<AZ::Vector3>& vertices) override;
        void ClearVertices() override;
        //////////////////////////////////////////////////////////////////////////

        /**
         * Set the vertex and manipulator positions for a given index
         */
        void UpdateManipulatorAndVertexPositions(AzToolsFramework::TranslationManipulator* vertexManipulator, size_t vertexIndex, const AZ::Vector3& localOffset);

        AZStd::vector<AZStd::shared_ptr<AzToolsFramework::TranslationManipulator>> m_vertexManipulators; ///< Manipulators for each vertex when entity is selected.

        void DisplayEntity(bool& handled) override;
        void OnSplineChanged();

        SplineCommon m_splineCommon; ///< Stores common spline functionality and properties
        AZ::Vector3 m_initialManipulatorPositionLocal; ///< Position of vertex when first selected with a manipulator.
    };
} // namespace LmbrCentral
