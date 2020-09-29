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

#include "GridSnappingTest.h"

namespace UnitTest
{
    TEST_F(GridSnappingFixture, MouseDownWithSnappingEnabledSnapsToClosestGridSize)
    {
        AZ::Vector3 initialPosition = AZ::Vector3(-7.3, 0.0, 0.0);
        AZ::Vector3 expectedPosition = AZ::Vector3(-15.0, 0.0, 0.0);

        // callback to update the manipulator's current position
        m_linearManipulator->InstallMouseMoveCallback(
            [this](const AzToolsFramework::LinearManipulator::Action& action)
        {
            auto pos = action.LocalPosition();
            m_linearManipulator->SetLocalPosition(pos);
        });

        m_linearManipulator->SetLocalPosition(initialPosition);

        m_actionDispatcher
            ->EnableSnapToGrid()
            ->GridSize(5.0f)
            ->CameraState(m_cameraState)
            ->MouseLButtonDown()
            ->ExpectManipulatorBeingInteracted()
            ->MouseRayMove(AZ::Vector3(-12, 0, 0))
            ->ExpectEq(m_linearManipulator->GetPosition(), expectedPosition)
            ->MouseLButtonUp()
            ->ExpectNotManipulatorBeingInteracted()
            ;
    }
} // namespace UnitTest
