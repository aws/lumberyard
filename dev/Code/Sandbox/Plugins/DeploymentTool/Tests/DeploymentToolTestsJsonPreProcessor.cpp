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
#include "DeploymentTool_precompiled.h"

#ifdef AZ_TESTS_ENABLED

#include <AzTest/AzTest.h>

#include "../JsonPreProcessor.h"
#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace DeployTool
{
    class DeploymentToolTestsJsonPreProcessor
        : public ::testing::Test
    {
    public:
        AZ_TEST_CLASS_ALLOCATOR(DeploymentToolTestsJsonPreProcessor);

        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
        }

        void TearDown() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }
    };

    TEST_F(DeploymentToolTestsJsonPreProcessor, DeploymentToolTestsJsonPreProcessor_CommentMarkerInString)
    {
        AZStd::string testInput =
            "{\n"
            "\"foo\": \"b#ar\" # hello\n"
            "}\n";
        AZStd::string testOutput =
            "{\n"
            "\"foo\": \"b#ar\" \n"
            "}\n";
        JsonPreProcessor jsonPreProcessor(testInput.c_str());
        ASSERT_TRUE(AzFramework::StringFunc::Equal(testOutput.c_str(), jsonPreProcessor.GetResult().c_str()));
    }

    TEST_F(DeploymentToolTestsJsonPreProcessor, DeploymentToolTestsJsonPreProcessor_AdvancedComments)
    {
        AZStd::string testInput =
            "\"astronauts\": [\n"
            "   \"Neil Armstrong\", # Neil Armstrong was the first person to walk on the Moon.\n"
            "   \"Buzz Aldrin\", # Buzz Aldrin was the second person to walk on the Moon.\n"
            "   \"Sally Ride\" # Sally Ride was the first American woman in space.\n"
            "]\n";
        AZStd::string testOutput =
            "\"astronauts\": [\n"
            "   \"Neil Armstrong\", \n"
            "   \"Buzz Aldrin\", \n"
            "   \"Sally Ride\" \n"
            "]\n";
        JsonPreProcessor jsonPreProcessor(testInput.c_str());
        ASSERT_TRUE(AzFramework::StringFunc::Equal(testOutput.c_str(), jsonPreProcessor.GetResult().c_str()));
    }

    TEST_F(DeploymentToolTestsJsonPreProcessor, DeploymentToolTestsJsonPreProcessor_Empty)
    {
        AZStd::string testInput = "";
        AZStd::string testOutput = "";
        JsonPreProcessor jsonPreProcessor(testInput.c_str());
        ASSERT_TRUE(AzFramework::StringFunc::Equal(testOutput.c_str(), jsonPreProcessor.GetResult().c_str()));
    }

    TEST_F(DeploymentToolTestsJsonPreProcessor, DeploymentToolTestsJsonPreProcessor_CommentAllOneLine)
    {
        AZStd::string testInput = "{} # empty one no line feed";
        AZStd::string testOutput = "{} ";
            JsonPreProcessor jsonPreProcessor(testInput.c_str());
        ASSERT_TRUE(AzFramework::StringFunc::Equal(testOutput.c_str(), jsonPreProcessor.GetResult().c_str()));
    }

    TEST_F(DeploymentToolTestsJsonPreProcessor, DeploymentToolTestsJsonPreProcessor_EscapedQuoteWithCommentMarker)
    {
        AZStd::string testInput = "{\"\\\"#foo\":\"bar\"}";
        AZStd::string testOutput = "{\"\\\"#foo\":\"bar\"}";
        JsonPreProcessor jsonPreProcessor(testInput.c_str());
        ASSERT_TRUE(AzFramework::StringFunc::Equal(testOutput.c_str(), jsonPreProcessor.GetResult().c_str()));
    }
} // namespace DeployTool
#endif // AZ_TESTS_ENABLED