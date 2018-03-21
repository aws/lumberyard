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

#include "HoverSelection.h"

#include <AzCore/Math/PolygonPrism.h>
#include <AzToolsFramework/Manipulators/EditorVertexSelection.h>
#include <AzToolsFramework/Manipulators/LineSegmentSelectionManipulator.h>
#include <AzToolsFramework/Manipulators/TranslationManipulator.h>

namespace AzToolsFramework
{
    void ConfigureTranslationManipulatorAppearance3d(
        TranslationManipulator* translationManipulator, const AZ::Vector3& localPosition)
    {
        // setup translation manipulator (visual appearance)
        translationManipulator->SetPosition(localPosition);
        translationManipulator->SetAxes(
            AZ::Vector3::CreateAxisX(),
            AZ::Vector3::CreateAxisY(),
            AZ::Vector3::CreateAxisZ());
        translationManipulator->ConfigurePlanarView(
            AZ::Color(1.0f, 0.0f, 0.0f, 1.0f),
            AZ::Color(0.0f, 1.0f, 0.0f, 1.0f),
            AZ::Color(0.0f, 0.0f, 1.0f, 1.0f));
        translationManipulator->ConfigureLinearView(
            2.0f,
            AZ::Color(1.0f, 0.0f, 0.0f, 1.0f),
            AZ::Color(0.0f, 1.0f, 0.0f, 1.0f),
            AZ::Color(0.0f, 0.0f, 1.0f, 1.0f));
        translationManipulator->ConfigureSurfaceView(
            0.1f, AZ::Color(1.0f, 1.0f, 0.0f, 0.5f));
    }

    void ConfigureTranslationManipulatorAppearance2d(
        TranslationManipulator* translationManipulator, const AZ::Vector2 &localPosition)
    {
        // setup translation manipulator (visual appearance)
        translationManipulator->SetPosition(Vector2ToVector3(localPosition));
        translationManipulator->SetAxes(
            AZ::Vector3::CreateAxisX(),
            AZ::Vector3::CreateAxisY());
        translationManipulator->ConfigurePlanarView(
            AZ::Color(1.0f, 0.0f, 0.0f, 1.0f));
        translationManipulator->ConfigureLinearView(
            2.0f,
            AZ::Color(1.0f, 0.0f, 0.0f, 1.0f),
            AZ::Color(0.0f, 1.0f, 0.0f, 1.0f));
    }

    HoverSelection::~HoverSelection() {}

    template<typename Vertex>
    static void UpdateLineSegmentPosition(
        size_t index, const AZ::VariableVertices<Vertex>& vertices,
        LineSegmentSelectionManipulator& lineSegment)
    {
        Vertex start;
        if (vertices.GetVertex(index, start))
        {
            lineSegment.SetStart(AdaptVertexOut(start));
        }

        Vertex end;
        if (vertices.GetVertex((index + 1) % vertices.Size(), end))
        {
            lineSegment.SetEnd(AdaptVertexOut(end));
        }

        // update the view
        const float lineWidth = 0.05f;
        lineSegment.SetView(
            CreateManipulatorViewLineSelect(lineSegment, AZ::Color(0.0f, 1.0f, 0.0f, 1.0f), lineWidth));
    }

    template<typename Vertex>
    LineSegmentHoverSelection<Vertex>::LineSegmentHoverSelection() {}

    template<typename Vertex>
    LineSegmentHoverSelection<Vertex>::~LineSegmentHoverSelection() {}

    template<typename Vertex>
    void LineSegmentHoverSelection<Vertex>::Create(AZ::EntityId entityId, ManipulatorManagerId managerId)
    {
        // create a line segment manipulator from vertex positions and setup its callback
        auto setupLineSegment = [this](AZ::EntityId entityId, ManipulatorManagerId managerId,
            size_t index, AZ::VariableVertices<Vertex>& vertices)
        {
            m_lineSegmentManipulators.push_back(AZStd::make_unique<LineSegmentSelectionManipulator>(entityId));
            AZStd::unique_ptr<LineSegmentSelectionManipulator>& lineSegmentManipulator = m_lineSegmentManipulators.back();
            lineSegmentManipulator->Register(managerId);

            UpdateLineSegmentPosition(index, vertices, *lineSegmentManipulator);

            lineSegmentManipulator->InstallLeftMouseUpCallback(
                [&vertices, index](
                    const LineSegmentSelectionManipulator::Action& action)
            {
                InsertVertex<Vertex>(vertices, index, AdaptVertexIn<Vertex>(action.m_localLineHitPosition));
            });
        };

        // create all line segment manipulators for the polygon prism (used for seleciton bounds)
        const size_t vertexCount = m_vertices->Size();
        if (vertexCount > 1)
        {
            // special case when there are only two vertices
            if (vertexCount == 2)
            {
                m_lineSegmentManipulators.reserve(vertexCount - 1);
                setupLineSegment(entityId, managerId, 0, *m_vertices);
            }
            else
            {
                m_lineSegmentManipulators.reserve(vertexCount);
                for (size_t i = 0; i < vertexCount; ++i)
                {
                    setupLineSegment(entityId, managerId, i, *m_vertices);
                }
            }
        }
    }

    template<typename Vertex>
    void LineSegmentHoverSelection<Vertex>::Destroy()
    {
        for (auto& manipulator : m_lineSegmentManipulators)
        {
            manipulator->Unregister();
        }

        m_lineSegmentManipulators.clear();
    }

    template<typename Vertex>
    void LineSegmentHoverSelection<Vertex>::Register(ManipulatorManagerId managerId)
    {
        for (auto& manipulator : m_lineSegmentManipulators)
        {
            manipulator->Register(managerId);
        }
    }

    template<typename Vertex>
    void LineSegmentHoverSelection<Vertex>::Unregister()
    {
        for (auto& manipulator : m_lineSegmentManipulators)
        {
            manipulator->Unregister();
        }
    }

    template<typename Vertex>
    void LineSegmentHoverSelection<Vertex>::SetBoundsDirty()
    {
        for (auto& manipulator : m_lineSegmentManipulators)
        {
            manipulator->SetBoundsDirty();
        }
    }

    template<typename Vertex>
    void LineSegmentHoverSelection<Vertex>::Refresh()
    {
        // update the start/end positions of all the line segment manipulators to ensure
        // they stay consistent with the polygon prism shape
        const size_t vertexCount = m_vertices->Size();
        if (vertexCount > 1)
        {
            for (size_t i = 0; i < m_lineSegmentManipulators.size(); ++i)
            {
                LineSegmentSelectionManipulator& lineSegmentManipulator = *m_lineSegmentManipulators[i];
                UpdateLineSegmentPosition(i, *m_vertices, lineSegmentManipulator);
                lineSegmentManipulator.SetBoundsDirty();
            }
        }
    }

    template<typename Vertex>
    void InsertVertex(AZ::VariableVertices<Vertex>& vertices, size_t index, const Vertex& localPosition)
    {
        vertices.InsertVertex((index + 1) % vertices.Size(), localPosition);

        // ensure property grid values are refreshed
        ToolsApplicationNotificationBus::Broadcast(
            &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay,
            Refresh_EntireTree);
    }

    template class LineSegmentHoverSelection<AZ::Vector2>;
    template class LineSegmentHoverSelection<AZ::Vector3>;
    template void InsertVertex(AZ::VariableVertices<AZ::Vector2>&, size_t, const AZ::Vector2&);
    template void InsertVertex(AZ::VariableVertices<AZ::Vector3>&, size_t, const AZ::Vector3&);
}