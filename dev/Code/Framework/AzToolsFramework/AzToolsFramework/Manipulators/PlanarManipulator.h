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

#include "BaseManipulator.h"

#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AzToolsFramework
{
    class ManipulatorView;

    /**
     * PlanarManipulator serves as a visual tool for users to modify values
     * in two dimension in a plane defined two non-collinear axes in 3D space.
     */
    class PlanarManipulator
        : public BaseManipulator
    {
    public:
        AZ_RTTI(PlanarManipulator, "{2B1C2140-F3B1-4DB2-B066-156B67B57B97}", BaseManipulator)
        AZ_CLASS_ALLOCATOR(PlanarManipulator, AZ::SystemAllocator, 0)

        explicit PlanarManipulator(AZ::EntityId entityId);
        ~PlanarManipulator();

        /**
         * The state of the manipulator at the start of an interaction.
         */
        struct Start
        {
            AZ::Vector3 m_localPosition; ///< The current position of the manipulator in local space.
            AZ::Vector3 m_snapOffset; ///< The snap offset amount to ensure manipulator is aligned to the grid.
        };

        /**
         * The state of the manipulator during an interaction.
         */
        struct Current
        {
            AZ::Vector3 m_localOffset; ///< The current position of the manipulator in local space.
        };

        /**
         * Mouse action data used by MouseActionCallback (wraps Start and Current manipulator state).
         */
        struct Action
        {
            Start m_start;
            Current m_current;
            AZ::Vector3 LocalPosition() const { return m_start.m_localPosition + m_current.m_localOffset; }
        };

        /**
         * This is the function signature of callbacks that will be invoked whenever a manipulator
         * is being clicked on or dragged.
         */
        using MouseActionCallback = AZStd::function<void(const Action&)>;

        void InstallLeftMouseDownCallback(MouseActionCallback onMouseDownCallback);
        void InstallLeftMouseUpCallback(MouseActionCallback onMouseUpCallback);
        void InstallMouseMoveCallback(MouseActionCallback onMouseMoveCallback);

        void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::EntityDebugDisplayRequests& display,
            const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        /**
         * Make sure @param axis1 and @param axis2 are not collinear.
         */
        void SetAxes(const AZ::Vector3& axis1, const AZ::Vector3& axis2);
        void SetPosition(const AZ::Vector3& position) { m_position = position; }

        const AZ::Vector3& GetAxis1() const { return m_fixed.m_axis1; }
        const AZ::Vector3& GetAxis2() const { return m_fixed.m_axis2; }
        const AZ::Vector3& GetPosition() const { return m_position; }

        void SetView(AZStd::unique_ptr<ManipulatorView>&& view);

    private:
        void OnLeftMouseDownImpl(
            const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance) override;
        void OnLeftMouseUpImpl(
            const ViewportInteraction::MouseInteraction& interaction) override;
        void OnMouseMoveImpl(
            const ViewportInteraction::MouseInteraction& interaction) override;

        void SetBoundsDirtyImpl() override;
        void InvalidateImpl() override;

        /**
         * Unchanging data set once for the planar manipulator.
         */
        struct Fixed
        {
            AZ::Vector3 m_axis1 = AZ::Vector3::CreateAxisX(); ///< m_axis1 and m_axis2 have to be orthogonal, they together define a plane in 3d space.
            AZ::Vector3 m_axis2 = AZ::Vector3::CreateAxisY();
            AZ::Vector3 m_normal = AZ::Vector3::CreateAxisZ(); ///< m_normal is calculated automatically when setting the axes.
        };

        /**
         * Initial data recorded when a press first happens with a planar manipulator.
         */
        struct StartInternal
        {
            AZ::Vector3 m_localPosition; ///< The starting position of the manipulator in local space.
            AZ::Vector3 m_localHitPosition; ///< The intersection point in world space between the ray and the manipulator when the mouse down event happens.
            AZ::Vector3 m_snapOffset; ///< The snap offset amount to ensure manipulator is aligned to the grid.
        };

        AZ::Vector3 m_position = AZ::Vector3::CreateZero(); ///< Position in local space.

        Fixed m_fixed;
        StartInternal m_startInternal;

        MouseActionCallback m_onLeftMouseDownCallback = nullptr;
        MouseActionCallback m_onLeftMouseUpCallback = nullptr;
        MouseActionCallback m_onMouseMoveCallback = nullptr;

        AZStd::unique_ptr<ManipulatorView> m_manipulatorView = nullptr; ///< Look of manipulator.

        static StartInternal CalculateManipulationDataStart(
            const Fixed& fixed, const AZ::Transform& worldFromLocal, bool snapping, float gridSize,
            const AZ::Vector3 localStartPosition, const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
            ManipulatorSpace manipulatorSpace);

        static Action CalculateManipulationDataAction(
            const Fixed& fixed, const StartInternal& startInternal, const AZ::Transform& worldFromLocal, bool snapping,
            float gridSize, const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
            ManipulatorSpace manipulatorSpace);
    };
} // namespace AzToolsFramework