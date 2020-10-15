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

#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>

namespace UnitTest
{
    // create a linear manipulator with a unit sphere bounds
    AZStd::shared_ptr<AzToolsFramework::LinearManipulator> CreateLinearManipulator(
        const AzToolsFramework::ManipulatorManagerId manipulatorManagerId);

    // create a world space mouse pick ray
    AzToolsFramework::ViewportInteraction::MousePick CreateWorldSpaceMousePickRay(
        const AZ::Vector3& origin, const AZ::Vector3& direction);

    // create a mouse interaction in world space
    AzToolsFramework::ViewportInteraction::MouseInteraction CreateMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MousePick& worldSpaceRay,
        AzToolsFramework::ViewportInteraction::MouseButton button);

    // create a mouse interaction event in world space
    AzToolsFramework::ViewportInteraction::MouseInteractionEvent CreateMouseInteractionEvent(
        const AzToolsFramework::ViewportInteraction::MousePick& worldSpaceRay,
        AzToolsFramework::ViewportInteraction::MouseButton button,
        AzToolsFramework::ViewportInteraction::MouseEvent event);

    AzFramework::CameraState SetCameraStatePosition(const AZ::Vector3& position, AzFramework::CameraState& cameraState);

    // dispatch a mouse event to the main manipulator manager via a bus call
    void DispatchMouseInteractionEvent(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& event);

    class AzManipulatorTestFrameworkTest
        : public ToolsApplicationFixture
    {
    protected:
        AzManipulatorTestFrameworkTest(const AzToolsFramework::ManipulatorManagerId& manipulatorManagerId)
            : m_manipulatorManagerId(manipulatorManagerId) {}

        void SetUpEditorFixtureImpl() override
        {        
            m_linearManipulator = CreateLinearManipulator(m_manipulatorManagerId);

            // default sanity check call backs
            m_linearManipulator->InstallLeftMouseDownCallback(
                [this](const AzToolsFramework::LinearManipulator::Action& /*action*/)
            {
                m_receivedLeftMouseDown = true;
            });

            m_linearManipulator->InstallMouseMoveCallback(
                [this](const AzToolsFramework::LinearManipulator::Action& /*action*/)
            {
                m_receivedMouseMove = true;
            });

            m_linearManipulator->InstallLeftMouseUpCallback(
                [this](const AzToolsFramework::LinearManipulator::Action& /*action*/)
            {
                m_receivedLeftMouseUp = true;
            });
        }

        void TearDownEditorFixtureImpl() override
        {
            m_linearManipulator->Unregister();
            m_linearManipulator.reset();
        }

        const AzToolsFramework::ManipulatorManagerId m_manipulatorManagerId;
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_linearManipulator;

        // sanity flags for manipulator mouse callbacks
        bool m_receivedLeftMouseDown = false;
        bool m_receivedMouseMove = false;
        bool m_receivedLeftMouseUp = false;

        // initial world space starting position for mouse interaction
        const AzToolsFramework::ViewportInteraction::MousePick m_mouseStartingPositionRay =
            CreateWorldSpaceMousePickRay(AZ::Vector3(0.0f, -2.0f, 0.0f), AZ::Vector3(0.0f, 1.0f, 0.0f));
    };
} // namespace UnitTest

