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
#include <iostream>
#include <stdio.h>
#include <stdarg.h>

namespace UnitTest
{
    class AzManipulatorTestFrameworkWorldSpaceBuilderTest
        : public ToolsApplicationFixture
        , private AzToolsFramework::NewViewportInteractionModelEnabledRequestBus::Handler
    {
    protected:
        struct State
        {
            State(AZStd::unique_ptr<AzManipulatorTestFramework::ManipulatorViewportInteractionInterface> viewportManipulatorInteraction)
                : m_viewportManipulatorInteraction(viewportManipulatorInteraction.release())
                , m_actionDispatcher(AZStd::make_unique<AzManipulatorTestFramework::ImmediateModeActionDispatcher>(*m_viewportManipulatorInteraction))
                , m_linearManipulator(CreateLinearManipulator(m_viewportManipulatorInteraction->GetManipulatorManager().GetId()))
            {
                // default sanity check call backs
                m_linearManipulator->InstallLeftMouseDownCallback(
                    [this](const AzToolsFramework::LinearManipulator::Action& action)
                {
                    m_receivedLeftMouseDown = true;
                });

                m_linearManipulator->InstallMouseMoveCallback(
                    [this](const AzToolsFramework::LinearManipulator::Action& action)
                {
                    m_receivedMouseMove = true;
                });

                m_linearManipulator->InstallLeftMouseUpCallback(
                    [this](const AzToolsFramework::LinearManipulator::Action& action)
                {
                    m_receivedLeftMouseUp = true;
                });
            }

            ~State() = default;

            // sanity flags for manipulator mouse callbacks
            bool m_receivedLeftMouseDown = false;
            bool m_receivedMouseMove = false;
            bool m_receivedLeftMouseUp = false;

        private:
            AZStd::unique_ptr<AzManipulatorTestFramework::ManipulatorViewportInteractionInterface> m_viewportManipulatorInteraction;

        public:
            AZStd::unique_ptr<AzManipulatorTestFramework::ImmediateModeActionDispatcher> m_actionDispatcher;
            AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_linearManipulator;
        };

        void ConsumeViewportLeftMouseClick(State& state);
        void ConsumeViewportMouseMoveHover(State& state);
        void ConsumeViewportMouseMoveActive(State& state);
        void MoveManipulatorAlongAxis(State& state);

    protected:
        void SetUpEditorFixtureImpl() override
        {
            AzToolsFramework::NewViewportInteractionModelEnabledRequestBus::Handler::BusConnect();

            // set the default handler for HandleMouseInteraction to EditorDefaultSelections's implementation
            AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Event(
                AzToolsFramework::GetEntityContextId(),
                &AzToolsFramework::EditorInteractionSystemViewportSelection::SetDefaultHandler);

            m_directState = AZStd::make_unique<State>(
                AZStd::make_unique<AzManipulatorTestFramework::DirectCallManipulatorViewportInteraction>());
            m_busState = AZStd::make_unique<State>(
                AZStd::make_unique<AzManipulatorTestFramework::IndirectCallManipulatorViewportInteraction>());
            m_cameraState =
                AzFramework::CreateIdentityDefaultCamera(AZ::Vector3(0.0f, -1.0f, 0.0f), AZ::Vector2(800.0f, 600.0f));
        }

        void TearDownEditorFixtureImpl() override
        {
            AzToolsFramework::NewViewportInteractionModelEnabledRequestBus::Handler::BusDisconnect();

            m_directState.reset();
            m_busState.reset();
        }

        // NewViewportInteractionModelEnabledRequestBus ...
        bool IsNewViewportInteractionModelEnabled() override
        {
            return true;
        }

    public:
        AZStd::unique_ptr<State> m_directState;
        AZStd::unique_ptr<State> m_busState;
        AzFramework::CameraState m_cameraState;
    };
} // namespace UnitTest
