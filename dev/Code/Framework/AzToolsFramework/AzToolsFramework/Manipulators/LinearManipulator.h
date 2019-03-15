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
#include <AzToolsFramework/Manipulators/ManipulatorView.h>

namespace AzToolsFramework
{
    class ManipulatorView;

    /**
     * LinearManipulator serves as a visual tool for users to modify values
     * in one dimension on an axis defined in 3D space.
     */
    class LinearManipulator
        : public BaseManipulator
    {
    public:
        AZ_RTTI(LinearManipulator, "{4AA805DA-7D3C-4AFA-8110-EECF32B8F530}", BaseManipulator)
        AZ_CLASS_ALLOCATOR(LinearManipulator, AZ::SystemAllocator, 0)

        explicit LinearManipulator(AZ::EntityId entityId, const AZ::Transform& worldFromLocal);
        ~LinearManipulator() = default;

        /**
         * Unchanging data set once for the linear manipulator.
         */
        struct Fixed
        {
            AZ::Vector3 m_axis = AZ::Vector3::CreateAxisX(); ///< The axis the manipulator will move along.
        };

        /**
         * The state of the manipulator at the start of an interaction.
         */
        struct Start
        {
            AZ::Vector3 m_localPosition; ///< The current position of the manipulator in local space.
            AZ::Vector3 m_localScale; ///< The current scale of the manipulator in local space.
            AZ::Vector3 m_localHitPosition; ///< The intersection point in local space between the ray and the manipulator when the mouse down event happens.
            AZ::Vector3 m_positionSnapOffset; ///< The snap offset amount to ensure manipulator is aligned to the grid.
            AZ::Vector3 m_scaleSnapOffset; ///< The snap offset amount to ensure manipulator is aligned to round scale increments.
            AZ::VectorFloat m_sign; ///< Used to determine which side of the axis we clicked on incase it's flipped to face the camera.
        };
        
        /**
         * The state of the manipulator during an interaction.
         */
        struct Current
        {
            AZ::Vector3 m_localPositionOffset; ///< The current offset of the manipulator from its starting position in local space.
            AZ::Vector3 m_localScaleOffset; ///< The current offset of the manipulator from its starting scale in local space.
        };

        /**
         * Mouse action data used by MouseActionCallback (wraps Fixed, Start and Current manipulator state).
         */
        struct Action
        {
            Fixed m_fixed;
            Start m_start;
            Current m_current;
            AZ::Vector3 LocalPosition() const { return m_start.m_localPosition + m_current.m_localPositionOffset; }
            AZ::Vector3 LocalScale() const { return m_start.m_localScale + m_current.m_localScaleOffset; }
        };

        /**
         * This is the function signature of callbacks that will be invoked whenever a manipulator
         * is clicked on or dragged.
         */
        using MouseActionCallback = AZStd::function<void(const Action&)>;

        void InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback);
        void InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback);
        void InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback);

        void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::EntityDebugDisplayRequests& display,
            const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void SetAxis(const AZ::Vector3& axis) { m_fixed.m_axis = axis; }
        void SetLocalTransform(const AZ::Transform& localTransform) { m_localTransform = localTransform; }
        void SetSpace(const AZ::Transform& worldFromLocal) { m_worldFromLocal = worldFromLocal; }

        AZ::Vector3 GetPosition() const { return m_localTransform.GetTranslation(); }
        const AZ::Vector3& GetAxis() const { return m_fixed.m_axis; }

        void SetViews(ManipulatorViews&& views);

        // Provide the ability to override manipulator space both for interactions and visuals.
        void OverrideInteractiveManipulatorSpace(
            const AZStd::function<ManipulatorSpace()>& interactiveManipulatorFunction);
        void OverrideVisualManipulatorSpace(
            const AZStd::function<ManipulatorSpace()>& visualManipulatorFunction);

    private:
        AZ_DISABLE_COPY_MOVE(LinearManipulator)

        void OnLeftMouseDownImpl(
            const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance) override;
        void OnLeftMouseUpImpl(
            const ViewportInteraction::MouseInteraction& interaction) override;
        void OnMouseMoveImpl(
            const ViewportInteraction::MouseInteraction& interaction) override;

        void InvalidateImpl() override;
        void SetBoundsDirtyImpl() override;

        /**
         * Initial data recorded when a press first happens with a linear manipulator.
         */
        struct StartInternal
        {
            AZ::Vector3 m_localPosition; ///< The position in local space of the manipulator when the mouse down event happens.
            AZ::Vector3 m_localScale; ///< The scale in local space of the manipulator when the mouse down event happens.
            AZ::Vector3 m_localHitPosition; ///< The intersection point in local space between the ray and the manipulator when the mouse down event happens.
            AZ::Vector3 m_localNormal; ///< The normal in local space of the manipulator when the mouse down event happens.
            AZ::Vector3 m_positionSnapOffset; ///< The snap offset amount to ensure manipulator position is aligned to the grid.
            AZ::Vector3 m_scaleSnapOffset; ///< The snap offset amount to ensure manipulator scale is aligned to the grid.
        };

        AZ::Transform m_localTransform = AZ::Transform::CreateIdentity(); ///< Local transform of the manipulator.
        AZ::Transform m_worldFromLocal = AZ::Transform::CreateIdentity(); ///< Space the manipulator is in (identity is world space).

        Fixed m_fixed;
        StartInternal m_startInternal;

        MouseActionCallback m_onLeftMouseDownCallback = nullptr;
        MouseActionCallback m_onLeftMouseUpCallback = nullptr;
        MouseActionCallback m_onMouseMoveCallback = nullptr;

        ManipulatorViews m_manipulatorViews; ///< Look of manipulator.

        AZStd::function<ManipulatorSpace()> m_interactiveManipulatorSpace; ///< Function to decide interactive manipulator space.
        AZStd::function<ManipulatorSpace()> m_visualManipulatorSpace; ///< Function to decide visual manipulator space.

        static StartInternal CalculateManipulationDataStart(
            const Fixed& fixed, const AZ::Transform& worldFromLocal, const AZ::Transform& localTransform,
            bool snapping, float gridSize, const AZ::Vector3& rayOrigin,const AZ::Vector3& rayDirection,
            ManipulatorSpace manipulatorSpace);

        static Action CalculateManipulationDataAction(
            const Fixed& fixed, const StartInternal& startInternal, const AZ::Transform& worldFromLocal,
            const AZ::Transform& localTransform, bool snapping, float gridSize, const AZ::Vector3& rayOrigin,
            const AZ::Vector3& rayDirection, ManipulatorSpace manipulatorSpace);
    };
} // namespace AzToolsFramework