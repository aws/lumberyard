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
#include "stdafx.h"
#include <AzTest/AzTest.h>
#include <Util/EditorUtils.h>
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <DisplaySettingsPythonFuncs.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace DisplaySettingsPythonBindingsUnitTests
{

    class DisplaySettingsPythonBindingsFixture
        : public testing::Test
    {
    public:
        AzToolsFramework::ToolsApplication m_app;

        void SetUp() override
        {
            AzFramework::Application::Descriptor appDesc;
            appDesc.m_enableDrilling = false;

            m_app.Start(appDesc);
            m_app.RegisterComponentDescriptor(AzToolsFramework::DisplaySettingsPythonFuncsHandler::CreateDescriptor());
        }

        void TearDown() override
        {
            m_app.Stop();
        }
    };

    TEST_F(DisplaySettingsPythonBindingsFixture, DisplaySettingsEditorCommands_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = m_app.GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        auto itDisplaySettingsCommands = behaviorContext->m_classes.find("DisplaySettings");
        EXPECT_TRUE(behaviorContext->m_classes.end() != itDisplaySettingsCommands);
        if (behaviorContext->m_classes.end() != itDisplaySettingsCommands)
        {
            AZ::BehaviorClass* behaviorClass = itDisplaySettingsCommands->second;
            EXPECT_TRUE(behaviorClass->m_methods.find("get_misc_editor_settings") != behaviorClass->m_methods.end());
            EXPECT_TRUE(behaviorClass->m_methods.find("set_misc_editor_settings") != behaviorClass->m_methods.end());
        }
    }
}