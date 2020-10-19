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

#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkFixture.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>

namespace UnitTest
{
    class AzManipulatorTestFrameworkBusCallTest
        : public AzManipulatorTestFrameworkTest
        , private AzToolsFramework::NewViewportInteractionModelEnabledRequestBus::Handler
    {
    protected:
        AzManipulatorTestFrameworkBusCallTest()
            : AzManipulatorTestFrameworkTest(AzToolsFramework::g_mainManipulatorManagerId) {}

        void SetUpEditorFixtureImpl() override
        {
            // connect the bus to allow us to manually specify we're using the new viewport interaction model
            AzToolsFramework::NewViewportInteractionModelEnabledRequestBus::Handler::BusConnect();

            // set the default handler for HandleMouseInteraction to EditorDefaultSelections's implementaiton
            AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Event(
                AzToolsFramework::GetEntityContextId(), &AzToolsFramework::EditorInteractionSystemViewportSelection::SetDefaultHandler);

            AzManipulatorTestFrameworkTest::SetUpEditorFixtureImpl();
        }

        void TearDownEditorFixtureImpl() override
        {
            AzManipulatorTestFrameworkTest::TearDownEditorFixtureImpl();
            AzToolsFramework::NewViewportInteractionModelEnabledRequestBus::Handler::BusDisconnect();
        }

        bool IsManipulatorInteractingBusCall() const
        {
            bool manipulatorInteracting = false;
            AzToolsFramework::ManipulatorManagerRequestBus::EventResult(
                manipulatorInteracting, AzToolsFramework::g_mainManipulatorManagerId,
                &AzToolsFramework::ManipulatorManagerRequestBus::Events::Interacting);

            return manipulatorInteracting;
        }

    private:
        // NewViewportInteractionModelEnabledRequestBus ...
        bool IsNewViewportInteractionModelEnabled() override
        {
            return true;
        }
    };
} // namespace UnitTest

