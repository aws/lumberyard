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

#include <AzTest/AzTest.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/MockIGraphObject.h>
#include <SceneAPI/SceneData/Behaviors/SoftNameTypes.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            // AddNodeNameBasedVirtualType
            TEST(SoftNameTypesTest, AddNodeNameBasedVirtualType_AddValidEntry_NoCrash)
            {
                SoftNameTypes container;
                container.AddNodeNameBasedVirtualType("Test", 
                    SceneCore::PatternMatcher("Pattern", SceneCore::PatternMatcher::MatchApproach::PostFix), false);
            }

            // AddFileNameBasedVirtualType
            TEST(SoftNameTypesTest, AddFileNameBasedVirtualType_AddValidEntry_NoCrash)
            {
                SoftNameTypes container;
                container.AddFileNameBasedVirtualType("Test", 
                    SceneCore::PatternMatcher("Pattern", SceneCore::PatternMatcher::MatchApproach::PostFix),
                {
                    Uuid::CreateString("{01234567-89AB-CDEF-0123-456789ABCDEF}")
                }, false);
            }

            // IsVirtualType - Node name based
            TEST(SoftNameTypesTest, IsVirtualType_NodeBasedFindsValidName_ReturnsTrue)
            {
                SoftNameTypes container;
                container.AddNodeNameBasedVirtualType("Test", 
                    SceneCore::PatternMatcher("_postfix", SceneCore::PatternMatcher::MatchApproach::PostFix), false);

                Containers::Scene scene("Scene");
                Containers::SceneGraph::NodeIndex newIndex = scene.GetGraph().AddChild(scene.GetGraph().GetRoot(), "test_postfix");

                EXPECT_TRUE(container.IsVirtualType(scene, newIndex));
            }

            TEST(SoftNameTypesTest, IsVirtualType_NodeBasedIgnoresNonMatching_ReturnsFalse)
            {
                SoftNameTypes container;
                container.AddNodeNameBasedVirtualType("Test", 
                    SceneCore::PatternMatcher("_postfix", SceneCore::PatternMatcher::MatchApproach::PostFix), false);

                Containers::Scene scene("Scene");
                Containers::SceneGraph::NodeIndex newIndex = scene.GetGraph().AddChild(scene.GetGraph().GetRoot(), "test_long_name");

                EXPECT_FALSE(container.IsVirtualType(scene, newIndex));
            }

            TEST(SoftNameTypesTest, IsVirtualType_NodeBasedIncludeChildThatDoesNotMatchPattern_ChildTypeIsMarked)
            {
                SoftNameTypes container;
                container.AddNodeNameBasedVirtualType("Test", 
                    SceneCore::PatternMatcher("_postfix", SceneCore::PatternMatcher::MatchApproach::PostFix), true);

                Containers::Scene scene("Scene");
                Containers::SceneGraph::NodeIndex newIndex = scene.GetGraph().AddChild(scene.GetGraph().GetRoot(), "test_postfix");
                Containers::SceneGraph::NodeIndex childIndex = scene.GetGraph().AddChild(newIndex, "test_child");

                ASSERT_TRUE(container.IsVirtualType(scene, newIndex));
                EXPECT_TRUE(container.IsVirtualType(scene, childIndex));
            }

            // IsVirtualType - File name based
            TEST(SoftNameTypesTest, IsVirtualType_FileBasedFilterDoesnotMatchWithInclusiveFilter_IgnoresAll)
            {
                SoftNameTypes container;
                container.AddFileNameBasedVirtualType("Test", 
                    SceneCore::PatternMatcher("_test", SceneCore::PatternMatcher::MatchApproach::PostFix),
                    {
                        DataTypes::MockIGraphObject::TYPEINFO_Uuid()
                    }, true);

                Containers::Scene scene("Scene");
                Containers::SceneGraph& graph = scene.GetGraph();
                Containers::SceneGraph::NodeIndex firstIndex = graph.AddChild(graph.GetRoot(), "test1", AZStd::make_shared<DataTypes::MockIGraphObjectAlt>(1));
                Containers::SceneGraph::NodeIndex secondIndex = graph.AddSibling(firstIndex, "test2", AZStd::make_shared<DataTypes::MockIGraphObject>(2));
                Containers::SceneGraph::NodeIndex thirdIndex = graph.AddSibling(secondIndex, "test3", AZStd::make_shared<DataTypes::MockIGraphObjectAlt>(3));

                EXPECT_FALSE(container.IsVirtualType(scene, firstIndex));
                EXPECT_FALSE(container.IsVirtualType(scene, secondIndex));
                EXPECT_FALSE(container.IsVirtualType(scene, thirdIndex));
            }

            TEST(SoftNameTypesTest, IsVirtualType_FileBasedFilterDoesnotMatchWithExclusiveFilter_IgnoresAll)
            {
                SoftNameTypes container;
                container.AddFileNameBasedVirtualType("Test", 
                    SceneCore::PatternMatcher("_test", SceneCore::PatternMatcher::MatchApproach::PostFix),
                    {
                        DataTypes::MockIGraphObject::TYPEINFO_Uuid()
                    }, false);

                Containers::Scene scene("Scene");
                Containers::SceneGraph& graph = scene.GetGraph();
                Containers::SceneGraph::NodeIndex firstIndex = graph.AddChild(graph.GetRoot(), "test1", AZStd::make_shared<DataTypes::MockIGraphObjectAlt>(1));
                Containers::SceneGraph::NodeIndex secondIndex = graph.AddSibling(firstIndex, "test2", AZStd::make_shared<DataTypes::MockIGraphObject>(2));
                Containers::SceneGraph::NodeIndex thirdIndex = graph.AddSibling(secondIndex, "test3", AZStd::make_shared<DataTypes::MockIGraphObjectAlt>(3));

                EXPECT_FALSE(container.IsVirtualType(scene, firstIndex));
                EXPECT_FALSE(container.IsVirtualType(scene, secondIndex));
                EXPECT_FALSE(container.IsVirtualType(scene, thirdIndex));
            }

            TEST(SoftNameTypesTest, IsVirtualType_FileBasedInclusiveFilter_AcceptsMatchingTypeAndIgnoresOthers)
            {
                SoftNameTypes container;
                container.AddFileNameBasedVirtualType("Test", 
                    SceneCore::PatternMatcher("_test", SceneCore::PatternMatcher::MatchApproach::PostFix),
                    {
                        DataTypes::MockIGraphObject::TYPEINFO_Uuid()
                    }, true);

                Containers::Scene scene("Scene_test");
                Containers::SceneGraph& graph = scene.GetGraph();
                Containers::SceneGraph::NodeIndex firstIndex = graph.AddChild(graph.GetRoot(), "test1", AZStd::make_shared<DataTypes::MockIGraphObjectAlt>(1));
                Containers::SceneGraph::NodeIndex secondIndex = graph.AddSibling(firstIndex, "test2", AZStd::make_shared<DataTypes::MockIGraphObject>(2));
                Containers::SceneGraph::NodeIndex thirdIndex = graph.AddSibling(secondIndex, "test3", AZStd::make_shared<DataTypes::MockIGraphObjectAlt>(3));

                EXPECT_FALSE(container.IsVirtualType(scene, firstIndex));
                EXPECT_TRUE(container.IsVirtualType(scene, secondIndex));
                EXPECT_FALSE(container.IsVirtualType(scene, thirdIndex));
            }

            TEST(SoftNameTypesTest, IsVirtualType_FileBasedExclusiveFilter_IgnoreMatchingTypeAndAcceptsOthers)
            {
                SoftNameTypes container;
                container.AddFileNameBasedVirtualType("Test", 
                    SceneCore::PatternMatcher("_test", SceneCore::PatternMatcher::MatchApproach::PostFix),
                    {
                        DataTypes::MockIGraphObject::TYPEINFO_Uuid()
                    }, false);

                Containers::Scene scene("Scene_test");
                Containers::SceneGraph& graph = scene.GetGraph();
                Containers::SceneGraph::NodeIndex firstIndex = graph.AddChild(graph.GetRoot(), "test1", AZStd::make_shared<DataTypes::MockIGraphObjectAlt>(1));
                Containers::SceneGraph::NodeIndex secondIndex = graph.AddSibling(firstIndex, "test2", AZStd::make_shared<DataTypes::MockIGraphObject>(2));
                Containers::SceneGraph::NodeIndex thirdIndex = graph.AddSibling(secondIndex, "test3", AZStd::make_shared<DataTypes::MockIGraphObjectAlt>(3));

                EXPECT_TRUE(container.IsVirtualType(scene, firstIndex));
                EXPECT_FALSE(container.IsVirtualType(scene, secondIndex));
                EXPECT_TRUE(container.IsVirtualType(scene, thirdIndex));
            }

            // InitializeWithDefaults
            TEST(SoftNameTypesTest, InitializeWithDefaults_HasDefaultTypes_NoCrashOrAssert)
            {
                SoftNameTypes container;
                container.InitializeWithDefaults();
            }

            // GetVirtualTypeName
            TEST(SoftNameTypesTest, GetVirtualTypeName_NodeBasedGetNameForCrcString_ReturnsOriginalName)
            {
                Crc32 nameHash("Test");

                SoftNameTypes container;
                container.AddNodeNameBasedVirtualType("Test", 
                    SceneCore::PatternMatcher("Pattern", SceneCore::PatternMatcher::MatchApproach::PostFix), false);

                AZStd::string result;
                container.GetVirtualTypeName(result, nameHash);

                EXPECT_STREQ("Test", result.c_str());
            }

            TEST(SoftNameTypesTest, GetVirtualTypeName_FileBasedGetNameForCrcString_ReturnsOriginalName)
            {
                Crc32 nameHash("Test");

                SoftNameTypes container;
                container.AddFileNameBasedVirtualType("Test", 
                    SceneCore::PatternMatcher("Pattern", SceneCore::PatternMatcher::MatchApproach::PostFix),
                    {
                        DataTypes::MockIGraphObject::TYPEINFO_Uuid()
                    }, false);

                AZStd::string result;
                container.GetVirtualTypeName(result, nameHash);

                EXPECT_STREQ("Test", result.c_str());
            }

            TEST(SoftNameTypesTest, GetVirtualTypeName_UnregisteredType_LeavesResultEmpty)
            {
                Crc32 nameHash("Test");

                SoftNameTypes container;
                container.InitializeWithDefaults();

                AZStd::string result;
                container.GetVirtualTypeName(result, nameHash);

                EXPECT_TRUE(result.empty());
            }
        } // SceneData
    } // SceneAPI
} // AZ
