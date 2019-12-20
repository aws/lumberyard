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

#include "ComponentModeTestFixture.h"

namespace UnitTest
{
    void ComponentModeTestFixture::SetUp()
    {
        using namespace AzToolsFramework;
        using namespace AzToolsFramework::ComponentModeFramework;

        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_app.Start(AzFramework::Application::Descriptor());

        m_app.RegisterComponentDescriptor(AzToolsFramework::ComponentModeFramework::PlaceholderEditorComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(AzToolsFramework::ComponentModeFramework::AnotherPlaceholderEditorComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(AzToolsFramework::ComponentModeFramework::DependentPlaceholderEditorComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(AzToolsFramework::ComponentModeFramework::TestComponentModeComponent<OverrideMouseInteractionComponentMode>::CreateDescriptor());

        m_editorActions.Connect();
    }

    void ComponentModeTestFixture::TearDown()
    {
        m_editorActions.Disconnect();

        m_app.Stop();
        m_serializeContext.reset();
    }

    void SelectEntities(const AzToolsFramework::EntityIdList& entityList)
    {
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityList);
    }
}
