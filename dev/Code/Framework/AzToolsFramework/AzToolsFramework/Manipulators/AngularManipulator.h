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

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AzToolsFramework
{
    class ManipulatorView;

    /**
     * AngularManipulator serves as a visual tool for users to change a component's property based on rotation
     * around an axis. The rotation angle increases if the rotation goes counter clock-wise when looking
     * in the opposite direction the rotation axis points to.
     */
    class AngularManipulator
        : public BaseManipulator
    {
    public:
        AZ_RTTI(AngularManipulator, "{01CB40F9-4537-4187-A8A6-1A12356D3FD1}", BaseManipulator);
        AZ_CLASS_ALLOCATOR(AngularManipulator, AZ::SystemAllocator, 0);

        AngularManipulator(AZ::EntityId entityId, const AZ::Transform& worldFromLocal);
        ~AngularManipulator() = default;

        /**
         * The state of the manipulator at the start of an interaction.
         */
        struct Start
        {
            AZ::Quaternion m_localOrientation; ///< Starting orientation of manipulator.
        };

        /**
         * The state of the manipulator during an interaction.
         */
        struct Current
        {
            AZ::Quaternion m_localRotation; ///< Amount of rotation to apply to manipulator during action.
        };

        /**
         * Mouse action data used by MouseActionCallback (wraps Start and Current manipulator state).
         */
        struct Action
        {
            Start m_start;
            Current m_current;
        };

        /**
         * This is the function signature of callbacks that will be invoked whenever a manipulator
         * is clicked on or dragged.
         */
        using MouseActionCallback = AZStd::function<void(const Action&)>;

        void InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback);
        void InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback);
        void InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback);

        void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::EntityDebugDisplayRequests& display,
            const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void SetAxis(const AZ::Vector3& axis) { m_fixed.m_axis = axis; }
        void SetSpace(const AZ::Transform& worldFromLocal) { m_worldFromLocal = worldFromLocal; }
        void SetLocalTransform(const AZ::Transform& localTransform) { m_localTransform = localTransform; }

        AZ::Vector3 GetPosition() const { return m_localTransform.GetTranslation(); }
        const AZ::Vector3& GetAxis() const { return m_fixed.m_axis; }

        void SetView(AZStd::unique_ptr<ManipulatorView>&& view);

    private:
        AZ_DISABLE_COPY_MOVE(AngularManipulator)

        void OnLeftMouseDownImpl(
            const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance) override;
        void OnLeftMouseUpImpl(
            const ViewportInteraction::MouseInteraction& interaction) override;
        void OnMouseMoveImpl(
            const ViewportInteraction::MouseInteraction& interaction) override;

        void SetBoundsDirtyImpl() override;
        void InvalidateImpl() override;

        /**
         * Unchanging data set once for the angular manipulator.
         */
        struct Fixed
        {
            AZ::Vector3 m_axis = AZ::Vector3::CreateAxisX(); ///< Axis for this angular manipulator to rotate around.
        };

        /**
         * Initial data recorded when a press first happens with an angular manipulator.
         */
        struct StartInternal
        {
            AZ::Transform m_worldFromLocal; ///< Initial transform when pressed.
            AZ::Transform m_localTransform; ///< Additional transform (offset) to apply to manipulator.
            AZ::Vector3 m_planePoint; ///< Position on plane to use for ray intersection.
            AZ::Vector3 m_planeNormal; ///< Normal of plane to use for ray intersection.
        };

        /**
         * Current data recorded each frame during an interaction with an angular manipulator.
         */
        struct CurrentInternal
        {
            float m_preSnapRadians = 0.0f; ///< Amount of rotation before a snap (snap increment accumulator).
            float m_radians = 0.0f; ///< Amount of rotation about the axis for this action.
            AZ::Vector3 m_worldHitPosition; ///< Initial world space hit position.
        };

        /**
         * Wrap start and current internal data during an interaction with an angular manipulator.
         */
        struct ActionInternal
        {
            StartInternal m_start;
            CurrentInternal m_current;
        };

        AZ::Transform m_localTransform = AZ::Transform::CreateIdentity(); ///< Local transform of the manipulator.
        AZ::Transform m_worldFromLocal = AZ::Transform::CreateIdentity(); ///< Space the manipulator is in (identity is world space).

        Fixed m_fixed;
        ActionInternal m_actionInternal;

        MouseActionCallback m_onLeftMouseDownCallback = nullptr;
        MouseActionCallback m_onLeftMouseUpCallback = nullptr;
        MouseActionCallback m_onMouseMoveCallback = nullptr;

        AZStd::unique_ptr<ManipulatorView> m_manipulatorView; ///< Look of manipulator.

        static ActionInternal CalculateManipulationDataStart(
            const Fixed& fixed, const AZ::Transform& worldFromLocal, const AZ::Transform& localTransform,
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float rayDistance,
            ManipulatorSpace manipulatorSpace);

        static Action CalculateManipulationDataAction(
            const Fixed& fixed, ActionInternal& actionInternal, const AZ::Transform& worldFromLocal,
            const AZ::Transform& localTransform, bool snapping, float angleStep,
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, ManipulatorSpace manipulatorSpace);
    };
} // namespace AzToolsFramework