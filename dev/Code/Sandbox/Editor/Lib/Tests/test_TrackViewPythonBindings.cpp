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
#include <TrackView/TrackViewPythonFuncs.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace TrackViewPythonBindingsUnitTests
{

    class TrackViewPythonBindingsFixture
        : public testing::Test
    {
    public:
        AzToolsFramework::ToolsApplication m_app;

        void SetUp() override
        {
            AzFramework::Application::Descriptor appDesc;
            appDesc.m_enableDrilling = false;

            m_app.Start(appDesc);
            m_app.RegisterComponentDescriptor(AzToolsFramework::TrackViewFuncsHandler::CreateDescriptor());
        }

        void TearDown() override
        {
            m_app.Stop();
        }
    };

    TEST_F(TrackViewPythonBindingsFixture, TrackViewEditorCommands_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = m_app.GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        EXPECT_TRUE(behaviorContext->m_methods.find("set_recording") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("new_sequence") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("delete_sequence") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_current_sequence") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_num_sequences") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_sequence_name") != behaviorContext->m_methods.end());

        // Blocked by LY-99827
        //EXPECT_TRUE(behaviorContext->m_methods.find("get_sequence_time_range") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("set_sequence_time_range") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("play_sequence") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("stop_sequence") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_time") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("add_node") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("add_selected_entities") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("add_layer_node") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("delete_node") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("add_track") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("delete_track") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_num_nodes") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_node_name") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("get_num_track_keys") != behaviorContext->m_methods.end());

        // LY-101771
        //EXPECT_TRUE(behaviorContext->m_methods.find("get_key_value") != behaviorContext->m_methods.end());
        //EXPECT_TRUE(behaviorContext->m_methods.find("get_interpolated_value") != behaviorContext->m_methods.end());
    }
}