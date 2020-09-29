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

#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkFixture.h>
#include <AzManipulatorTestFramework/DirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace UnitTest
{
    class GridSnappingFixture
        : public ToolsApplicationFixture
    {
    public:
        GridSnappingFixture()
            : m_viewportManipulatorInteraction(AZStd::make_unique<AzManipulatorTestFramework::DirectCallManipulatorViewportInteraction>())
            , m_actionDispatcher(AZStd::make_unique<AzManipulatorTestFramework::ImmediateModeActionDispatcher>(*m_viewportManipulatorInteraction))
            , m_linearManipulator(CreateLinearManipulator(m_viewportManipulatorInteraction->GetManipulatorManager().GetId()))
        {}

    protected:
        void SetUpEditorFixtureImpl() override
        {
            m_cameraState =
                AzFramework::CreateIdentityDefaultCamera(AZ::Vector3(-7.3f, -1.0f, 0.0f), AZ::Vector2(800.0f, 600.0f));
        }

    public:

        AZStd::unique_ptr<AzManipulatorTestFramework::ManipulatorViewportInteractionInterface> m_viewportManipulatorInteraction;
        AZStd::unique_ptr<AzManipulatorTestFramework::ImmediateModeActionDispatcher> m_actionDispatcher;
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_linearManipulator;
        AzFramework::CameraState m_cameraState;
    };
} // namespace UnitTest
