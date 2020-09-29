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

#include <AzFramework/Application/Application.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <QApplication>

#include <SceneAPI/SceneCore/Mocks/Containers/MockScene.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/MockIGraphObject.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>
#include <SceneAPI/SceneUI/RowWidgets/NodeTreeSelectionWidget.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            using Containers::SceneGraph;

            class Test_NodeSelectionWidget
                : public UI::NodeTreeSelectionWidget
            {
            public:
                AZ_CLASS_ALLOCATOR(Test_NodeSelectionWidget, SystemAllocator, 0)

                Test_NodeSelectionWidget(QWidget* parent)
                    : UI::NodeTreeSelectionWidget(parent)
                {}

                virtual size_t GetSelectedCount()
                {
                    return CalculateSelectedCount();
                }

                virtual size_t GetTotalCount()
                {
                    return CalculateTotalCount();
                }
            };

            class NodeTreeSelectionWidgetTest
                : public ::testing::Test
            {
            public:
                void SetUp() override
                {
                    if (!AllocatorInstance<SystemAllocator>::IsReady())
                    {
                        AllocatorInstance<SystemAllocator>::Create();
                    }

                    m_qApp = AZStd::make_unique<QApplication>(m_qAppArgc, const_cast<char**>(m_qAppArgv));

                    m_context = AZStd::make_unique<SerializeContext>();
                    m_manifestWidget = AZStd::make_unique<UI::ManifestWidget>(m_context.get(), nullptr);
                    m_scene = AZStd::make_shared<Containers::Scene>("TestScene");

                    m_manifestWidget->BuildFromScene(m_scene);
                    SceneGraph& graph = m_scene->GetGraph();

                    SceneGraph::NodeIndex root = graph.GetRoot();

                    // Create graph for testing
                    SceneGraph::NodeIndex indexA = graph.AddChild(root, "A", AZStd::make_shared<DataTypes::MockIGraphObject>(1));
                    SceneGraph::NodeIndex indexA2 = graph.AddChild(indexA, "A2", AZStd::make_shared<DataTypes::MockIGraphObject>(4));
                    graph.AddChild(indexA2, "A22", AZStd::make_shared<DataTypes::MockIGraphObject>(5));
                    graph.AddChild(root, "B", AZStd::make_shared<DataTypes::MockIGraphObject>(2));
                    graph.AddChild(root, "C", AZStd::make_shared<DataTypes::MockIGraphObject>(3));

                    m_ntsWidget = AZStd::make_unique<Test_NodeSelectionWidget>(m_manifestWidget.get());
                }

                void TearDown() override
                {
                    if (AllocatorInstance<SystemAllocator>::IsReady())
                    {
                        AllocatorInstance<SystemAllocator>::Destroy();
                    }
                }

                const char* m_qAppArg0 = "test_app";
                const char* m_qAppArgv[2] { m_qAppArg0, nullptr };
                int m_qAppArgc = 2;

                AZStd::unique_ptr<QApplication> m_qApp;
                AZStd::unique_ptr<SerializeContext> m_context;
                AZStd::unique_ptr<UI::ManifestWidget> m_manifestWidget;
                AZStd::shared_ptr<Containers::Scene> m_scene;
                AZStd::unique_ptr<Test_NodeSelectionWidget> m_ntsWidget;
            };

            TEST_F(NodeTreeSelectionWidgetTest, GetSelection_NoneSelected_NoneAreSelected_FT)
            {
                SceneData::SceneNodeSelectionList testList = SceneData::SceneNodeSelectionList();

                m_ntsWidget->SetList(testList);

                size_t actualSelected = m_ntsWidget->GetSelectedCount();
                size_t actualTotal = m_ntsWidget->GetTotalCount();

                EXPECT_NE(actualTotal, 0);
                EXPECT_EQ(actualSelected, 0);
            }

            TEST_F(NodeTreeSelectionWidgetTest, GetSelection_ThreeSelected_ThreeAreSelected_FT)
            {
                SceneData::SceneNodeSelectionList testList = SceneData::SceneNodeSelectionList();
                testList.AddSelectedNode("A22");
                testList.RemoveSelectedNode("B");
                testList.RemoveSelectedNode("C");

                m_ntsWidget->SetList(testList);

                size_t actualSelected = m_ntsWidget->GetSelectedCount();

                EXPECT_EQ(actualSelected, 3);
            }

            TEST_F(NodeTreeSelectionWidgetTest, GetSelection_AllSelected_AllAreSelected_FT)
            {
                SceneData::SceneNodeSelectionList testList = SceneData::SceneNodeSelectionList();
                testList.AddSelectedNode("A22");
                testList.AddSelectedNode("B");
                testList.AddSelectedNode("C");

                m_ntsWidget->SetList(testList);

                size_t actualSelected = m_ntsWidget->GetSelectedCount();
                size_t actualTotal = m_ntsWidget->GetTotalCount();

                EXPECT_NE(actualTotal, 0);
                EXPECT_EQ(actualSelected, actualTotal);
            }
        } // namespace SceneUI
    } // namespace SceneAPI
} // namespace AZ
