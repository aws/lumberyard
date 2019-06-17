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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/ComponentMode/ComponentModeCollection.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>
#include <Tests/TestTypes.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>

#include <QApplication>

namespace UnitTest
{
    using namespace AzToolsFramework;

    class ComponentModeTest
        : public AllocatorsTestFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;

    public:
        void SetUp() override
        {
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_app.Start(AzFramework::Application::Descriptor());
        }

        void TearDown() override
        {
            m_app.Stop();
            m_serializeContext.reset();
        }

        ToolsApplication m_app;
    };

    TEST_F(ComponentModeTest, BeginEndComponentMode)
    {
        using ::testing::Eq;

        EditorInteractionSystemViewportSelectionRequestBus::Event(
            GetEntityContextId(), &EditorInteractionSystemViewportSelection::SetDefaultHandler);

        QWidget rootWidget;
        ActionOverrideRequestBus::Event(
            GetEntityContextId(), &ActionOverrideRequests::SetupActionOverrideHandler, &rootWidget);

        ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &ComponentModeFramework::ComponentModeSystemRequests::BeginComponentMode,
            AZStd::vector<ComponentModeFramework::EntityAndComponentModeBuilders>{});

        bool inComponentMode = false;
        ComponentModeFramework::ComponentModeSystemRequestBus::BroadcastResult(
            inComponentMode, &ComponentModeFramework::ComponentModeSystemRequests::InComponentMode);

        EXPECT_TRUE(inComponentMode);

        ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &ComponentModeFramework::ComponentModeSystemRequests::EndComponentMode);

        ComponentModeFramework::ComponentModeSystemRequestBus::BroadcastResult(
            inComponentMode, &ComponentModeFramework::ComponentModeSystemRequests::InComponentMode);

        EXPECT_FALSE(inComponentMode);

        ActionOverrideRequestBus::Event(
            GetEntityContextId(), &ActionOverrideRequests::TeardownActionOverrideHandler);
    }

} // namespace UnitTest