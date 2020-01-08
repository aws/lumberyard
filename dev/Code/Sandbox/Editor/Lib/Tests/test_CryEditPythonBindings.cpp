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
#include <MainWindow.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace CryEditPythonBindingsUnitTests
{

    class CryEditPythonBindingsFixture
        : public testing::Test
    {
    public:
        AzToolsFramework::ToolsApplication m_app;

        void SetUp() override
        {
            AzFramework::Application::Descriptor appDesc;
            appDesc.m_enableDrilling = false;

            m_app.Start(appDesc);
            m_app.RegisterComponentDescriptor(AzToolsFramework::CryEditPythonHandler::CreateDescriptor());
        }

        void TearDown() override
        {
            m_app.Stop();
        }
    };

    TEST_F(CryEditPythonBindingsFixture, CryEditCommands_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = m_app.GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        EXPECT_TRUE(behaviorContext->m_methods.find("open_level") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("open_level_no_prompt") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("create_level") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("create_level_no_prompt") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_game_folder") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_current_level_name") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_current_level_path") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("load_all_plugins") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_current_view_position") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_current_view_rotation") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_current_view_position") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_current_view_rotation") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("export_to_engine") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_config_spec") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_config_platform") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_config_spec") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_result_to_success") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_result_to_failure") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("idle_enable") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("is_idle_enabled") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("idle_is_enabled") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("idle_wait") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("start_process_detached") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("launch_lua_editor") != behaviorContext->m_methods.end());
    }

    TEST_F(CryEditPythonBindingsFixture, CryEditCheckoutDialogCommands_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = m_app.GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        EXPECT_TRUE(behaviorContext->m_methods.find("enable_for_all") != behaviorContext->m_methods.end());
    }
}