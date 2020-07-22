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

#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>
#include <QApplication>
#include <QtTest>
#include "qtestsystem.h"
#include <Tests/ProvidesUI/AnimGraph/SimpleAnimGraphUIFixture.h>

namespace EMotionFX
{
    TEST_F(SimpleAnimGraphUIFixture, ResetAnimGraph)
    {
        // This test checks that we can reset a graph without any problem.
        EMStudio::AnimGraphModel& animGraphModel = m_animGraphPlugin->GetAnimGraphModel();

        AZStd::string commandResult;
        MCore::CommandGroup group;
        CommandSystem::ClearAnimGraphsCommand(&group);
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(group, commandResult)) << commandResult.c_str();

        EXPECT_EQ(GetAnimGraphManager().GetNumAnimGraphs(), 0);
        EXPECT_EQ(animGraphModel.GetFocusedAnimGraph(), nullptr);
        m_animGraph = nullptr;
    }

    TEST_F(SimpleAnimGraphUIFixture, FocusRemainValidAfterDeleteFocus)
    {
        // This test checks that a focused item can be deleted, and afterward the focus will get set correctly.
        EMStudio::AnimGraphModel& animGraphModel = m_animGraphPlugin->GetAnimGraphModel();
        AnimGraphNode* motionNode = m_animGraph->RecursiveFindNodeByName("testMotion");
        EXPECT_TRUE(motionNode);
        AnimGraphNode* blendTreeNode = m_animGraph->RecursiveFindNodeByName("testBlendTree");
        EXPECT_TRUE(blendTreeNode);

        // Focus on the motion node.
        const QModelIndex motionNodeModelIndex = animGraphModel.FindFirstModelIndex(motionNode);
        animGraphModel.Focus(motionNodeModelIndex);
        EXPECT_EQ(motionNodeModelIndex, animGraphModel.GetFocus());

        // Delete the motion Node.
        AZStd::string removeCommand = AZStd::string::format("AnimGraphRemoveNode -animGraphID %d -name testMotion", m_animGraphId);
        AZStd::string commandResult;
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(removeCommand, commandResult)) << commandResult.c_str();

        // The focus should change.
        const QModelIndex focusIndex = animGraphModel.GetFocus();
        EXPECT_TRUE(focusIndex.isValid()) << "AnimGraphModel should have a valid index after removing the focus node.";
        EXPECT_EQ(focusIndex, animGraphModel.FindFirstModelIndex(m_animGraph->GetRootStateMachine())) << "the root statemachine node should become the new focus";
    }

    TEST_F(SimpleAnimGraphUIFixture, ParametersWindowFocusChange)
    {
        // This test checks that parameters window behave expected after model changes.
        EMStudio::AnimGraphModel& animGraphModel = m_animGraphPlugin->GetAnimGraphModel();
        AnimGraphNode* motionNode = m_animGraph->RecursiveFindNodeByName("testMotion");
        EXPECT_TRUE(motionNode);
        AnimGraphNode* blendTreeNode = m_animGraph->RecursiveFindNodeByName("testBlendTree");
        EXPECT_TRUE(blendTreeNode);

        // Focus on the motion node.
        const QModelIndex motionNodeModelIndex = animGraphModel.FindFirstModelIndex(motionNode);
        animGraphModel.Focus(motionNodeModelIndex);

        // Check the parameters window
        const EMStudio::ParameterWindow* parameterWindow = m_animGraphPlugin->GetParameterWindow();
        EXPECT_EQ(parameterWindow->GetTopLevelItemCount(), 3) << "Should be 3 parameters added in the parameters window.";

        // Force the model to look at an invalid index. This should reset the parameters window.
        animGraphModel.Focus(QModelIndex());
        EXPECT_EQ(parameterWindow->GetTopLevelItemCount(), 0) << "Should be 0 parameters in the parameters window after reset.";

        // Force the model to look back at the motion node.
        animGraphModel.Focus(motionNodeModelIndex);
        EXPECT_EQ(parameterWindow->GetTopLevelItemCount(), 3) << "Should be 3 parameters added in the parameters window.";

        // Delete the motion node.
        AZStd::string removeCommand = AZStd::string::format("AnimGraphRemoveNode -animGraphID %d -name testMotion", m_animGraphId);
        AZStd::string commandResult;
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(removeCommand, commandResult)) << commandResult.c_str();

        // The parameter windows shouldn't be affected
        EXPECT_EQ(parameterWindow->GetTopLevelItemCount(), 3) << "Should be 3 parameters added in the parameters window.";
    }
} // namespace EMotionFX
