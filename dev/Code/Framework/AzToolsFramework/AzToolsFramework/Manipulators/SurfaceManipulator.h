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

#include <AzCore/Memory/SystemAllocator.h>

namespace AzToolsFramework
{
    class ManipulatorView;

    /**
     * Surface manipulator will ensure the point(s) it controls snap precisely to the xy grid
     * while also staying aligned exactly to the height of the terrain.
     */
    class SurfaceManipulator
        : public BaseManipulator
    {
    public:
        AZ_RTTI(SurfaceManipulator, "{75B8EF42-A5F0-48EB-893E-84BED1BC8BAF}", BaseManipulator)
        AZ_CLASS_ALLOCATOR(SurfaceManipulator, AZ::SystemAllocator, 0)

        explicit SurfaceManipulator(AZ::EntityId entityId);
        ~SurfaceManipulator();

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
            AZ::Vector3 m_localOffset; ///< The current offset of the manipulator from its starting position in local space.
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

        using MouseActionCallback = AZStd::function<void(const Action&)>;

        void InstallLeftMouseDownCallback(MouseActionCallback onMouseDownCallback);
        void InstallLeftMouseUpCallback(MouseActionCallback onMouseUpCallback);
        void InstallMouseMoveCallback(MouseActionCallback onMouseMoveCallback);

        void OnLeftMouseDownImpl(
            const ViewportInteraction::MouseInteraction& interaction,
            float rayIntersectionDistance) override;
        void OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction) override;
        void OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction) override;

        void SetBoundsDirtyImpl() override;

        void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::EntityDebugDisplayRequests& display,
            const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void SetPosition(const AZ::Vector3& position) { m_position = position; }
        const AZ::Vector3& GetPosition() const { return m_position; }

        void SetView(AZStd::unique_ptr<ManipulatorView>&& view);

    protected:
        void InvalidateImpl() override;

    private:
        AZ::Vector3 m_position = AZ::Vector3::CreateZero(); ///< Position in local space.

        struct StartInternal
        {
            AZ::Vector3 m_localPosition; ///< The current position of the manipulator in local space.
            AZ::Vector3 m_localHitPosition; ///< The hit position with the terrain in local space.
            AZ::Vector3 m_snapOffset; ///< The snap offset amount to ensure manipulator is aligned to the grid.
        };

        StartInternal m_startInternal; ///< Internal intitial state recorded/created in OnMouseDown.

        AZStd::unique_ptr<ManipulatorView> m_manipulatorView = nullptr; ///< Look of manipulator.

        MouseActionCallback m_onLeftMouseDownCallback = nullptr;
        MouseActionCallback m_onLeftMouseUpCallback = nullptr;
        MouseActionCallback m_onMouseMoveCallback = nullptr;

        static StartInternal CalculateManipulationDataStart(
            const AZ::Transform& worldFromLocal, const AZ::Vector3& worldSurfacePosition,
            const AZ::Vector3& localPosition, bool snapping, float gridSize, int viewportId);

        static Action CalculateManipulationDataAction(
            const StartInternal& startInternal, const AZ::Transform& worldFromLocal, 
            const AZ::Vector3& worldSurfacePosition, bool snapping, float gridSize, int viewportId);
    };
}