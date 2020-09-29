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

#include "AzManipulatorTestFrameworkWorldSpaceBuilderTest.h"

namespace UnitTest
{
    void AzManipulatorTestFrameworkWorldSpaceBuilderTest::ConsumeViewportLeftMouseClick(State& state)
    {
        // given a left mouse down ray in world space
        // consume the mouse down and up events
        state.m_actionDispatcher
            ->LogActions(true)
            ->CameraState(m_cameraState)
            ->MouseLButtonDown()
            ->Trace("Expecting left mouse button down")
            ->ExpectTrue(state.m_receivedLeftMouseDown)
            ->Trace("Not expecting left mouse button up")
            ->ExpectFalse(state.m_receivedLeftMouseUp)
            ->Trace("Not expecting mouse move")
            ->ExpectFalse(state.m_receivedMouseMove)
            ->ExpectManipulatorBeingInteracted()
            ->MouseLButtonUp()
            ->Trace("Expecting left mouse button up")
            ->ExpectTrue(state.m_receivedLeftMouseDown)
            ->ExpectTrue(state.m_receivedLeftMouseUp)
            ->ExpectFalse(state.m_receivedMouseMove)
            ->ExpectFalse(state.m_linearManipulator->PerformingAction())
            ->ExpectNotManipulatorBeingInteracted()
            ;
    }

    void AzManipulatorTestFrameworkWorldSpaceBuilderTest::ConsumeViewportMouseMoveHover(State& state)
    {
        // given a left mouse down ray in world space
        // consume the mouse move event
        state.m_actionDispatcher
            ->CameraState(m_cameraState)
            ->MouseRayMove(AZ::Vector3::CreateZero())
            ->ExpectFalse(state.m_linearManipulator->PerformingAction())
            ->ExpectNotManipulatorBeingInteracted()
            ->ExpectFalse(state.m_receivedLeftMouseDown)
            ->ExpectFalse(state.m_receivedMouseMove)
            ->ExpectFalse(state.m_receivedLeftMouseUp)
            ;
    }

    void AzManipulatorTestFrameworkWorldSpaceBuilderTest::ConsumeViewportMouseMoveActive(State& state)
    {
        // given a left mouse down ray in world space
        // consume the mouse move event
        state.m_actionDispatcher
            ->LogActions(true)
            ->CameraState(m_cameraState)
            ->MouseLButtonDown()
            ->MouseRayMove(AZ::Vector3::CreateZero())
            ->ExpectTrue(state.m_linearManipulator->PerformingAction())
            ->ExpectManipulatorBeingInteracted()
            ->MouseLButtonUp()
            ->ExpectTrue(state.m_receivedLeftMouseDown)
            ->ExpectTrue(state.m_receivedMouseMove)
            ->ExpectTrue(state.m_receivedLeftMouseUp)
            ;
    }

    void AzManipulatorTestFrameworkWorldSpaceBuilderTest::MoveManipulatorAlongAxis(State& state)
    {
        AZ::Vector3 movementAlongAxis = AZ::Vector3::CreateZero();
        const AZ::Vector3 expectedMovementAlongAxis = AZ::Vector3(-5.0f, 0.0f, 0.0f);

        state.m_linearManipulator->InstallMouseMoveCallback(
            [&movementAlongAxis](const AzToolsFramework::LinearManipulator::Action& action)
        {
            movementAlongAxis = action.m_current.m_localPositionOffset;
        });

        state.m_actionDispatcher
            ->CameraState(m_cameraState)
            ->MouseLButtonDown()                                                    
            ->MouseRayMove(expectedMovementAlongAxis)
            ->ExpectTrue(state.m_linearManipulator->PerformingAction())             
            ->ExpectManipulatorBeingInteracted()
            ->MouseLButtonUp()                                                      
            ->ExpectTrue(state.m_receivedLeftMouseDown)                             
            ->ExpectTrue(state.m_receivedLeftMouseUp)
            ->ExpectEq(movementAlongAxis, expectedMovementAlongAxis)                
            ;
    }

    TEST_F(AzManipulatorTestFrameworkWorldSpaceBuilderTest, ConsumeViewportLeftMouseClick)
    {
        ConsumeViewportLeftMouseClick(*m_directState);
        ConsumeViewportLeftMouseClick(*m_busState);
    }

    TEST_F(AzManipulatorTestFrameworkWorldSpaceBuilderTest, ConsumeViewportMouseMoveHover)
    {
        ConsumeViewportMouseMoveHover(*m_directState);
        ConsumeViewportMouseMoveHover(*m_busState);
    }

    TEST_F(AzManipulatorTestFrameworkWorldSpaceBuilderTest, ConsumeViewportMouseMoveActive)
    {
        ConsumeViewportMouseMoveActive(*m_directState);
        ConsumeViewportMouseMoveActive(*m_busState);
    }

    TEST_F(AzManipulatorTestFrameworkWorldSpaceBuilderTest, MoveManipulatorAlongAxis)
    {
        MoveManipulatorAlongAxis(*m_directState);
        MoveManipulatorAlongAxis(*m_busState);
    }
} // namespace UnitTest
