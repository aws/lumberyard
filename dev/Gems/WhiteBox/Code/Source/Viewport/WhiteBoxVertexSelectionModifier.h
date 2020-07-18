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

#include "Viewport/WhiteBoxViewportConstants.h"

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Memory/Memory.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace AzToolsFramework
{
    class ManipulatorViewSphere;
    class SelectionManipulator;

    namespace ViewportInteraction
    {
        struct MouseInteraction;
    }
} // namespace AzToolsFramework

namespace WhiteBox
{
    //! VertexSelectionModifier provides the ability to select a vertex in the viewport.
    class VertexSelectionModifier
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        VertexSelectionModifier(
            const AZ::EntityComponentIdPair& entityComponentIdPair, Api::VertexHandle vertexHandle,
            const AZ::Vector3& intersectionPoint);
        ~VertexSelectionModifier();

        bool MouseOver() const;
        void ForwardMouseOverEvent(const AzToolsFramework::ViewportInteraction::MouseInteraction& interaction);

        Api::VertexHandle GetHandle() const; // Generic context version
        Api::VertexHandle GetVertexHandle() const;
        void SetVertexHandle(Api::VertexHandle vertexHandle);
        void SetColor(const AZ::Color& color);

        void Refresh();
        void UpdateIntersectionPoint(const AZ::Vector3& intersectionPoint);
        bool PerformingAction() const;

    private:
        void CreateManipulator();
        void DestroyManipulator();

        void CreateView();

        Api::VertexHandle m_vertexHandle; //!< The vertex handle this modifier is currently associated with.
        AZ::EntityComponentIdPair
            m_entityComponentIdPair; //!< The entity and component id this modifier is associated with.
        AZStd::shared_ptr<AzToolsFramework::SelectionManipulator>
            m_selectionManipulator; //!< Manipulator for performing vertex selection.
        AZStd::shared_ptr<AzToolsFramework::ManipulatorViewSphere>
            m_vertexView; //!< Manipulator view used to represent a mesh vertex for selection.
        AZ::Color m_color = cl_whiteBoxVertexHoveredColor; //! The current color of the vertex.
    };

    inline Api::VertexHandle VertexSelectionModifier::GetHandle() const
    {
        return GetVertexHandle();
    }

    inline Api::VertexHandle VertexSelectionModifier::GetVertexHandle() const
    {
        return m_vertexHandle;
    }

    inline void VertexSelectionModifier::SetVertexHandle(const Api::VertexHandle vertexHandle)
    {
        m_vertexHandle = vertexHandle;
    }

    inline void VertexSelectionModifier::SetColor(const AZ::Color& color)
    {
        m_color = color;
    }
} // namespace WhiteBox
