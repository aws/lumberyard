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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Manipulators/PlanarManipulator.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/SurfaceManipulator.h>

namespace AzToolsFramework
{
    /**
     * TranslationManipulator is an aggregation of 3 linear manipulators and 3 planar manipulators who share the same origin.
     */
    class TranslationManipulator
    {
    public:
        /**
         * How many dimensions does this translation manipulator have
         */
        enum class Dimensions
        {
            Two,
            Three
        };

        AZ_RTTI(TranslationManipulator, "{D5E49EA2-30E0-42BC-A51D-6A7F87818260}");
        AZ_CLASS_ALLOCATOR(TranslationManipulator, AZ::SystemAllocator, 0);

        explicit TranslationManipulator(AZ::EntityId entityId, Dimensions dimensions);
        virtual ~TranslationManipulator();

        void Register(ManipulatorManagerId manipulatorManagerId);
        void Unregister();

        void InstallLinearManipulatorMouseDownCallback(LinearManipulator::MouseActionCallback onMouseDownCallback);
        void InstallLinearManipulatorMouseMoveCallback(LinearManipulator::MouseActionCallback onMouseMoveCallback);
        void InstallLinearManipulatorMouseUpCallback(LinearManipulator::MouseActionCallback onMouseUpCallback);

        void InstallPlanarManipulatorMouseDownCallback(PlanarManipulator::MouseActionCallback onMouseDownCallback);
        void InstallPlanarManipulatorMouseMoveCallback(PlanarManipulator::MouseActionCallback onMouseMoveCallback);
        void InstallPlanarManipulatorMouseUpCallback(PlanarManipulator::MouseActionCallback onMouseUpCallback);

        void InstallSurfaceManipulatorMouseDownCallback(SurfaceManipulator::MouseActionCallback onMouseDownCallback);
        void InstallSurfaceManipulatorMouseMoveCallback(SurfaceManipulator::MouseActionCallback onMouseMoveCallback);
        void InstallSurfaceManipulatorMouseUpCallback(SurfaceManipulator::MouseActionCallback onMouseUpCallback);

        void SetBoundsDirty();

        void SetPosition(const AZ::Vector3& position);
        const AZ::Vector3& GetPosition() const { return m_position; }

        void SetAxes(const AZ::Vector3& axis1, const AZ::Vector3& axis2, const AZ::Vector3& axis3 = AZ::Vector3::CreateAxisZ());

        void ConfigurePlanarView(
            const AZ::Color& plane1Color,
            const AZ::Color& plane2Color = AZ::Color(0.0f, 1.0f, 0.0f, 0.5f),
            const AZ::Color& plane3Color = AZ::Color(0.0f, 0.0f, 1.0f, 0.5f));

        void ConfigureLinearView(
            float axisLength,
            const AZ::Color& axis1Color, const AZ::Color& axis2Color,
            const AZ::Color& axis3Color = AZ::Color(0.0f, 0.0f, 1.0f, 0.5f));

        void ConfigureSurfaceView(
            float radius, const AZ::Color& color);

    private:
        AZ_DISABLE_COPY_MOVE(TranslationManipulator)

        AZ::Vector3 m_position; ///< Local space position of TranslationManipulator.
        const Dimensions m_dimensions; ///< How many dimensions of freedom does this manipulator have.

        AZStd::vector<AZStd::unique_ptr<LinearManipulator>> m_linearManipulators;
        AZStd::vector<AZStd::unique_ptr<PlanarManipulator>> m_planarManipulators;
        AZStd::unique_ptr<SurfaceManipulator> m_surfaceManipulator = nullptr;

        /**
         * Common processing for base manipulator type.
         */
        void ProcessManipulators(AZStd::function<void(BaseManipulator*)>);
    };

    /**
     * IndexedTranslationManipulator wraps a standard TranslationManipulator and allows it to be linked
     * to a particular index in a list of vertices/points.
     */
    template<typename Vertex>
    struct IndexedTranslationManipulator
    {
        explicit IndexedTranslationManipulator(
            AZ::EntityId entityId, TranslationManipulator::Dimensions dimensions, size_t index, const Vertex& position)
                : m_manipulator(entityId, dimensions)
        {
            m_vertices.push_back({ position, index });
        }

        /**
         * Store vertex start position as manipulator event occurs, index refers to location in container.
         */
        struct VertexLookup
        {
            Vertex m_start;
            size_t m_index;
        };

        /**
         * Helper to iterate over all vertices stored by the manipulator.
         */
        void Process(AZStd::function<void(VertexLookup&)> fn)
        {
            for (VertexLookup& vertex : m_vertices)
            {
                fn(vertex);
            }
        }

        AZStd::vector<VertexLookup> m_vertices; ///< List of vertices currently associated with this translation manipulator.
        TranslationManipulator m_manipulator;
    };
} // namespace AzToolsFramework