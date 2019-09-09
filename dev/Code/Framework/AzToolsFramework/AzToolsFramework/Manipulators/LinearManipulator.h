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
#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace AzToolsFramework
{
    /// LinearManipulator serves as a visual tool for users to modify values
    /// in one dimension on an axis defined in 3D space.
    class LinearManipulator
        : public BaseManipulator
    {
        /// Private constructor.
        explicit LinearManipulator(const AZ::Transform& worldFromLocal);

    public:
        AZ_RTTI(LinearManipulator, "{4AA805DA-7D3C-4AFA-8110-EECF32B8F530}", BaseManipulator)
        AZ_CLASS_ALLOCATOR(LinearManipulator, AZ::SystemAllocator, 0)

        LinearManipulator() = delete;
        LinearManipulator(const LinearManipulator&) = delete;
        LinearManipulator& operator=(const LinearManipulator&) = delete;

        ~LinearManipulator() = default;

        /// A Manipulator must only be created and managed through a shared_ptr.
        static AZStd::shared_ptr<LinearManipulator> MakeShared(const AZ::Transform& worldFromLocal)
        {
            return AZStd::shared_ptr<LinearManipulator>(aznew LinearManipulator(worldFromLocal));
        }

        /// Unchanging data set once for the linear manipulator.
        struct Fixed
        {
            AZ::Vector3 m_axis = AZ::Vector3::CreateAxisX(); ///< The axis the manipulator will move along.
        };

        /// The state of the manipulator at the start of an interaction.
        struct Start
        {
            AZ::Vector3 m_localPosition; ///< The current position of the manipulator in local space.
            AZ::Vector3 m_localScale; ///< The current scale of the manipulator in local space.
            AZ::Vector3 m_localHitPosition; ///< The intersection point in local space between the ray and the manipulator when the mouse down event happens.
            AZ::Vector3 m_positionSnapOffset; ///< The snap offset amount to ensure manipulator is aligned to the grid.
            AZ::Vector3 m_scaleSnapOffset; ///< The snap offset amount to ensure manipulator is aligned to round scale increments.
            AZ::VectorFloat m_sign; ///< Used to determine which side of the axis we clicked on in case it's flipped to face the camera.
        };

        /// The state of the manipulator during an interaction.
        struct Current
        {
            AZ::Vector3 m_localPositionOffset; ///< The current offset of the manipulator from its starting position in local space.
            AZ::Vector3 m_localScaleOffset; ///< The current offset of the manipulator from its starting scale in local space.
        };

        /// Mouse action data used by MouseActionCallback (wraps Fixed, Start and Current manipulator state).
        struct Action
        {
            Fixed m_fixed;
            Start m_start;
            Current m_current;
            ViewportInteraction::KeyboardModifiers m_modifiers;
            AZ::Vector3 LocalScale() const { return m_start.m_localScale + m_current.m_localScaleOffset; }
            AZ::Vector3 LocalScaleOffset() const { return m_start.m_scaleSnapOffset + m_current.m_localScaleOffset; }
            AZ::Vector3 LocalPosition() const { return m_start.m_localPosition + m_current.m_localPositionOffset; }
            AZ::Vector3 LocalPositionOffset() const { return m_current.m_localPositionOffset; }
        };

        /// This is the function signature of callbacks that will be invoked whenever a manipulator
        /// is clicked on or dragged.
        using MouseActionCallback = AZStd::function<void(const Action&)>;

        void InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback);
        void InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback);
        void InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback);

        void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void SetAxis(const AZ::Vector3& axis);
        void SetSpace(const AZ::Transform& worldFromLocal);
        void SetLocalTransform(const AZ::Transform& localTransform);
        void SetLocalPosition(const AZ::Vector3& localPosition);
        void SetLocalOrientation(const AZ::Quaternion& localOrientation);

        AZ::Vector3 GetPosition() const { return m_localTransform.GetTranslation(); }
        const AZ::Vector3& GetAxis() const { return m_fixed.m_axis; }

        void SetViews(ManipulatorViews&& views);

        void UseVisualOrientationOverride(bool use) { m_useVisualsOverride = use; }
        void SetVisualOrientationOverride(const AZ::Quaternion& visualOrientation)
        {
            m_visualOrientationOverride = visualOrientation;
        }

    private:
        void OnLeftMouseDownImpl(
            const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance) override;
        void OnLeftMouseUpImpl(
            const ViewportInteraction::MouseInteraction& interaction) override;
        void OnMouseMoveImpl(
            const ViewportInteraction::MouseInteraction& interaction) override;

        void InvalidateImpl() override;
        void SetBoundsDirtyImpl() override;

        /// Initial data recorded when a press first happens with a linear manipulator.
        struct StartInternal
        {
            AZ::Vector3 m_localPosition; ///< The position in local space of the manipulator when the mouse down event happens.
            AZ::Vector3 m_localScale; ///< The scale in local space of the manipulator when the mouse down event happens.
            AZ::Vector3 m_localHitPosition; ///< The intersection point in local space between the ray and the manipulator when the mouse down event happens.
            AZ::Vector3 m_localNormal; ///< The normal in local space of the manipulator when the mouse down event happens.
            AZ::Vector3 m_positionSnapOffset; ///< The snap offset amount to ensure manipulator position is aligned to the grid.
            AZ::Vector3 m_scaleSnapOffset; ///< The snap offset amount to ensure manipulator scale is aligned to the grid.
            AZ::VectorFloat m_screenToWorldScale; ///< Used to scale movement based on camera distance if we want screen space instead of world space displacement.
        };

        AZ::Transform m_localTransform = AZ::Transform::CreateIdentity(); ///< Local transform of the manipulator.
        AZ::Transform m_worldFromLocal = AZ::Transform::CreateIdentity(); ///< Space the manipulator is in (identity is world space).

        bool m_useVisualsOverride = false; // Set this to true to use the Visual Quaternion Override (decoupled from logical axis).
        AZ::Quaternion m_visualOrientationOverride = AZ::Quaternion::CreateIdentity(); // Quaternion to use only for visuals.

        Fixed m_fixed;
        StartInternal m_startInternal;

        MouseActionCallback m_onLeftMouseDownCallback = nullptr;
        MouseActionCallback m_onLeftMouseUpCallback = nullptr;
        MouseActionCallback m_onMouseMoveCallback = nullptr;

        ManipulatorViews m_manipulatorViews; ///< Look of manipulator.

        static StartInternal CalculateManipulationDataStart(
            const Fixed& fixed, const AZ::Transform& worldFromLocal, const AZ::Transform& localTransform,
            bool snapping, float gridSize, const AZ::Vector3& rayOrigin,const AZ::Vector3& rayDirection,
            const AzFramework::CameraState& cameraState);

        static Action CalculateManipulationDataAction(
            const Fixed& fixed, const StartInternal& startInternal, const AZ::Transform& worldFromLocal,
            const AZ::Transform& localTransform, bool snapping, float gridSize, const AZ::Vector3& rayOrigin,
            const AZ::Vector3& rayDirection, ViewportInteraction::KeyboardModifiers keyboardModifiers);
    };
} // namespace AzToolsFramework