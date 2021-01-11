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

#include <LmbrCentral_precompiled.h>
#include <AzTest/AzTest.h>
#include <Builders/LuaBuilder/LuaBuilderWorker.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzCore/IO/FileIO.h>

namespace UnitTest
{
    using namespace LuaBuilder;
    using namespace AssetBuilderSDK;

    class LuaBuilderTests
        : public ::testing::Test
    {
        void SetUp() override
        {
            m_app.Start(m_descriptor);
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            const char* dir = m_app.GetExecutableFolder();

            AZ::IO::FileIOBase::GetInstance()->SetAlias("@root@", dir);

            auto* serializeContext = m_app.GetSerializeContext();
        }

        void TearDown() override
        {
            m_app.Stop();
        }

        AzToolsFramework::ToolsApplication m_app;
        AZ::ComponentApplication::Descriptor m_descriptor;
    };

    TEST_F(LuaBuilderTests, ParseLuaScript_UsingRequire_ShouldFindDependencies)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        char resolvedPath[AZ_MAX_PATH_LEN];
        AZ::IO::FileIOBase::GetInstance()->ResolvePath("@root@/../Gems/LmbrCentral/Code/Tests/Lua/test1.lua", resolvedPath, AZ_MAX_PATH_LEN);

        worker.ParseDependencies(AZStd::string(resolvedPath), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("scripts/test2.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("scripts/test3.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("scripts/test4.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_UsingReloadScript_ShouldFindDependencies)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        char resolvedPath[AZ_MAX_PATH_LEN];
        AZ::IO::FileIOBase::GetInstance()->ResolvePath("@root@/../Gems/LmbrCentral/Code/Tests/Lua/test2.lua", resolvedPath, AZ_MAX_PATH_LEN);

        worker.ParseDependencies(AZStd::string(resolvedPath), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("some/path/test3.lua", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_WithPathInAString_ShouldFindDependencies)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        char resolvedPath[AZ_MAX_PATH_LEN];
        AZ::IO::FileIOBase::GetInstance()->ResolvePath("@root@/../Gems/LmbrCentral/Code/Tests/Lua/test3_general_dependencies.lua", resolvedPath, AZ_MAX_PATH_LEN);

        worker.ParseDependencies(AZStd::string(resolvedPath), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("some/file/in/some/folder.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_WithConsoleCommand_ShouldFindDependencies)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        char resolvedPath[AZ_MAX_PATH_LEN];
        AZ::IO::FileIOBase::GetInstance()->ResolvePath("@root@/../Gems/LmbrCentral/Code/Tests/Lua/test4_console_command.lua", resolvedPath, AZ_MAX_PATH_LEN);

        worker.ParseDependencies(AZStd::string(resolvedPath), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("somefile.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("somefile/in/a/folder.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }

    void VerifyNoDependenciesGenerated(const AZStd::string& testFileUnresolvedPath)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        char resolvedPath[AZ_MAX_PATH_LEN];
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(testFileUnresolvedPath.c_str(), resolvedPath, AZ_MAX_PATH_LEN);

        worker.ParseDependencies(AZStd::string(resolvedPath), pathDependencies);

        EXPECT_TRUE(pathDependencies.empty());
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_CommentedOutDependency_EntireLine_ShouldFindNoDependencies)
    {
        VerifyNoDependenciesGenerated("@root@/../Gems/LmbrCentral/Code/Tests/Lua/test5_whole_line_comment.lua");
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_CommentedOutDependency_PartialLine_ShouldFindNoDependencies)
    {
        VerifyNoDependenciesGenerated("@root@/../Gems/LmbrCentral/Code/Tests/Lua/test6_partial_line_comment.lua");
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_CommentedOutDependency_BlockComment_ShouldFindNoDependencies)
    {
        VerifyNoDependenciesGenerated("@root@/../Gems/LmbrCentral/Code/Tests/Lua/test7_block_comment.lua");
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_CommentedOutDependency_NegatedBlockComment_ShouldFindDependencies)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        char resolvedPath[AZ_MAX_PATH_LEN];
        AZ::IO::FileIOBase::GetInstance()->ResolvePath("@root@/../Gems/LmbrCentral/Code/Tests/Lua/test8_negated_block_comment.lua", resolvedPath, AZ_MAX_PATH_LEN);

        worker.ParseDependencies(AZStd::string(resolvedPath), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("somefile.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("somefile/in/a/folder.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }
}
