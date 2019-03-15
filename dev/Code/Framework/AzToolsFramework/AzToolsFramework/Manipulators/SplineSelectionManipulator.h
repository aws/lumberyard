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

#include <AzCore/Math/Spline.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    class ManipulatorView;

    /**
     * A manipulator to represent selection of a spline. Underlying spline data is
     * used to test mouse picking ray against to preview closest point on spline.
     */
    class SplineSelectionManipulator
        : public BaseManipulator
    {
    public:
        AZ_RTTI(SplineSelectionManipulator, "{3E6B2206-E910-48C9-BDB6-F45B539C00F4}", BaseManipulator);
        AZ_CLASS_ALLOCATOR(SplineSelectionManipulator, AZ::SystemAllocator, 0);

        explicit SplineSelectionManipulator(AZ::EntityId entityId);
        ~SplineSelectionManipulator();

        /**
         * Mouse action data used by MouseActionCallback.
         */
        struct Action
        {
            AZ::Vector3 m_localSplineHitPosition;
            AZ::SplineAddress m_splineAddress;
        };

        using MouseActionCallback = AZStd::function<void(const Action&)>;
        
        void InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback);
        void InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback);

        void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::EntityDebugDisplayRequests& display,
            const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void SetSpline(const AZStd::shared_ptr<const AZ::Spline>& spline) { m_spline = spline; }
        AZStd::weak_ptr<const AZ::Spline> GetSpline() const { return m_spline; }

        void SetView(AZStd::unique_ptr<ManipulatorView>&& view);

    private:
        void OnLeftMouseDownImpl(
            const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance) override;
        void OnLeftMouseUpImpl(
            const ViewportInteraction::MouseInteraction& interaction) override;
        
        void InvalidateImpl() override;
        void SetBoundsDirtyImpl() override;

        AZStd::weak_ptr<const AZ::Spline> m_spline;

        MouseActionCallback m_onLeftMouseDownCallback = nullptr;
        MouseActionCallback m_onLeftMouseUpCallback = nullptr;

        ViewportInteraction::KeyboardModifiers m_keyboardModifiers; ///< What modifier keys are pressed when interacting with this manipulator.

        AZStd::unique_ptr<ManipulatorView> m_manipulatorView = nullptr; ///< Look of manipulator and bounds for interaction.
    };

    SplineSelectionManipulator::Action CalculateManipulationDataAction(
        const AZ::Transform& worldFromLocal, const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection, const AZStd::weak_ptr<const AZ::Spline> spline);
}