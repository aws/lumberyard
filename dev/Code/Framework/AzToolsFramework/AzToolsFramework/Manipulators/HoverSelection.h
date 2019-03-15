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

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Math/VertexContainerInterface.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>

namespace AZ
{
    class Vector2;
    class Vector3;
    class Spline;
    class PolygonPrism;
}

namespace AzToolsFramework
{
    class TranslationManipulators;
    class LineSegmentSelectionManipulator;
}

namespace AzToolsFramework
{
    /**
     * HoverSelection provides an interface for manipulator/s offering selection when
     * the mouse is hovered over a particular bound. This interface is used to represent
     * a Spline manipulator bound, and a series of LineSegment manipulator bounds.
     * This generic interface allows EditorVertexSelection to use either Spline or LineSegment selection.
     */
    class HoverSelection
    {
    public:
        virtual ~HoverSelection();

        virtual void Create(AZ::EntityId entityId, ManipulatorManagerId managerId) = 0;
        virtual void Destroy() = 0;
        virtual void Register(ManipulatorManagerId managerId) = 0;
        virtual void Unregister() = 0;
        virtual void SetBoundsDirty() = 0;
        virtual void Refresh() = 0;
    };

    /**
     * LineSegmentHoverSelection is a concrete implementation of HoverSelection wrapping a collection/container
     * of vertices and a list of LineSegmentManipulators. The underlying manipulators are used to control selection
     * by highlighting where on the line a new vertex will be inserted.
     */
    template<typename Vertex>
    class LineSegmentHoverSelection
        : public HoverSelection
    {
    public:
        LineSegmentHoverSelection();
        ~LineSegmentHoverSelection();

        void Create(AZ::EntityId entityId, ManipulatorManagerId managerId) override;
        void Destroy() override;
        void Register(ManipulatorManagerId managerId) override;
        void Unregister() override;
        void SetBoundsDirty() override;
        void Refresh() override;

        AZStd::unique_ptr<AZ::VariableVertices<Vertex>> m_vertices;

    private:
        AZ_DISABLE_COPY_MOVE(LineSegmentHoverSelection)

        AZStd::vector<AZStd::shared_ptr<LineSegmentSelectionManipulator>> m_lineSegmentManipulators; ///< Manipulators for each line.
    };

    /**
     * NullHoverSelection is used when vertices cannot be inserted. This serves as a noop
     * and is used to prevent the need for additional null checks in EditorVertexSelection.
     */
    class NullHoverSelection
        : public HoverSelection
    {
    public:
        NullHoverSelection() = default;
        ~NullHoverSelection() = default;

        void Create(AZ::EntityId /*entityId*/, ManipulatorManagerId /*managerId*/) override {}
        void Destroy() override {}
        void Register(ManipulatorManagerId /*managerId*/) override {}
        void Unregister() override {}
        void SetBoundsDirty() override {}
        void Refresh() override {}

    private:
        AZ_DISABLE_COPY_MOVE(NullHoverSelection)
    };

    /**
     * Helper for inserting a vertex in a variable vertices container.
     */
    template<typename Vertex>
    void InsertVertex(AZ::VariableVertices<Vertex>& vertices, size_t index, const Vertex& localPosition);

    /**
     * Function pointer to configure how a translation manipulator should look and behave (dimensions/axes/views).
     */
    using TranslationManipulatorConfiguratorFn = void(*)(
        TranslationManipulators*, const AZ::Transform& localTransform);

    void ConfigureTranslationManipulatorAppearance3d(
        TranslationManipulators* translationManipulators,
        const AZ::Transform& localTransform);
    void ConfigureTranslationManipulatorAppearance2d(
        TranslationManipulators* translationManipulators,
        const AZ::Transform& localTransform);
}