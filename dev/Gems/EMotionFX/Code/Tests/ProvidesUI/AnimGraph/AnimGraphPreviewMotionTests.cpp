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

#include <gtest/gtest.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigateWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewPlugin.h>
#include <QPushButton>
#include <QAction>
#include <QtTest>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/JackActor.h>
#include <Tests/TestAssetCode/TestMotionAssets.h>
#include <Tests/UI/AnimGraphUIFixture.h>

namespace EMotionFX
{   
    class PreviewMotionFixture:
        public AnimGraphUIFixture
    {
    public:
        void SetUp()
        {
            AnimGraphUIFixture::SetUp();
            
            //Create one motion set, and import one motion and add to the motion set.
            ExecuteCommands({
            R"str(CreateMotionSet -name MotionSet0)str"
                });

            EMStudio::MotionSetsWindowPlugin* motionSetsWindowPlugin = static_cast<EMStudio::MotionSetsWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MotionSetsWindowPlugin::CLASS_ID));
            ASSERT_TRUE(motionSetsWindowPlugin) << "Motion Window plugin not loaded";

            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(0);
            ASSERT_TRUE(motionSet) << "Motion set with id 0 does not exist";
            motionSetsWindowPlugin->SetSelectedSet(motionSet);

            ExecuteCommands({
            R"str(ImportMotion -filename Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_idle.motion)str",
            R"str(MotionSetAddMotion -motionSetID 0 -motionFilenamesAndIds Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_idle.motion;rin_idle)str"
            });
        }

        static void ExecuteCommands(std::vector<std::string> commands)
        {
            AZStd::string result;
            for (const auto& commandStr : commands)
            {
                if (commandStr == "UNDO")
                {
                    EXPECT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << "Undo: " << result.c_str();
                }
                else if (commandStr == "REDO")
                {
                    EXPECT_TRUE(CommandSystem::GetCommandManager()->Redo(result)) << "Redo: " << result.c_str();
                }
                else
                {
                    EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(commandStr.c_str(), result)) << commandStr.c_str() << ": " << result.c_str();
                }
            }
        }
    };
    

    TEST_F(PreviewMotionFixture, PreviewMotionTests)
    {
        AnimGraph* animGraph = CreateAnimGraph();
        EXPECT_NE(animGraph, nullptr) << "Cannot find newly created anim graph.";

        auto nodeGraph = GetActiveNodeGraph();
        EXPECT_TRUE(nodeGraph) << "Node graph not found.";

        // Serialize motion node members, the motion id for rin_idle.
        EMotionFX::AnimGraphMotionNode* tempMotionNode = aznew EMotionFX::AnimGraphMotionNode();
        tempMotionNode->SetMotionIds({ "rin_idle" });
        AZ::Outcome<AZStd::string> serializedMotionNode = MCore::ReflectionSerializer::SerializeMembersExcept(tempMotionNode, {});
        
        EMotionFX::AnimGraphNode* currentNode = nodeGraph->GetModelIndex().data(EMStudio::AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        ASSERT_TRUE(currentNode) << "No current AnimGraphNode found";
        CommandSystem::CreateAnimGraphNode(currentNode->GetAnimGraph(), azrtti_typeid<EMotionFX::AnimGraphMotionNode>(), "Motion", currentNode, 0, 0, serializedMotionNode.GetValue());

        // Check motion node has been created.
        AnimGraphMotionNode* motionNode = static_cast<AnimGraphMotionNode*>(animGraph->GetNode(1));
        EXPECT_NE(motionNode, nullptr) << "Cannot find newly created motion node.";

        nodeGraph->SelectAllNodes();
        const AZStd::vector<EMotionFX::AnimGraphNode*> selectedAnimGraphNodes = nodeGraph->GetSelectedAnimGraphNodes();
        m_blendGraphWidget->OnContextMenuEvent(m_blendGraphWidget, QPoint(0, 0), QPoint(0, 0), m_animGraphPlugin, selectedAnimGraphNodes, true, false, m_animGraphPlugin->GetActionFilter());

        // Check that Preview Motion action is available in the context menu.
        QMenu* selectedNodeContextMenu = m_blendGraphWidget->findChild<QMenu*>("BlendGraphWidget.SelectedNodeMenu");
        ASSERT_TRUE(selectedNodeContextMenu) << "Selected node context menu was not found.";
        QAction* previewMotionAction = GetNamedAction(selectedNodeContextMenu, QString("Preview rin_idle"));
        ASSERT_TRUE(previewMotionAction) << "Preview motion action not found.";
        delete tempMotionNode;
    }
} // end namespace EMotionFX
