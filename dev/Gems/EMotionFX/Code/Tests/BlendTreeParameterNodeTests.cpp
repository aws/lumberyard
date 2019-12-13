
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

#include <Tests/AnimGraphFixture.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>


namespace EMotionFX
{
    class BlendTreeParameterNodeTests : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();

            BlendTree* blendTree = aznew BlendTree();
            m_rootStateMachine->AddChildNode(blendTree);
            m_rootStateMachine->SetEntryState(blendTree);

            m_parameterNode = aznew BlendTreeParameterNode();
            m_parameterNode->SetName("Parameters0");
            blendTree->AddChildNode(m_parameterNode);
        }

    public:
        BlendTreeParameterNode* m_parameterNode;
    };

    TEST_F(BlendTreeParameterNodeTests, RenameParameter)
    {
        AZStd::string command;
        AZStd::string result;
        CommandSystem::CommandManager commandManager;

        const char* startParameterName = "Parameter0";
        const char* renamedParameterName = "RenamedParameter0";

        // Add new parameter to the anim graph and check if a output port got added for the parameter node.
        EXPECT_EQ(m_parameterNode->GetOutputPorts().size(), 0);
        command = AZStd::string::format("AnimGraphCreateParameter -animGraphID %d -type {2ED6BBAF-5C82-4EAA-8678-B220667254F2} -name %s",
            m_animGraph->GetID(),
            startParameterName);
        EXPECT_TRUE(commandManager.ExecuteCommand(command, result));
        m_parameterNode->Reinit(); // When the Animation Editor is present, EMStudio::AnimGraphEventHandler calls this.
        EXPECT_EQ(m_parameterNode->GetOutputPorts().size(), 1);
        EXPECT_STREQ(m_parameterNode->GetOutputPorts()[0].GetName(), startParameterName);

        // Rename anim graph parameter and check if the output port of the parameter node also got renamed.
        command = AZStd::string::format("AnimGraphAdjustParameter -animGraphID %d -name %s -newName %s",
            m_animGraph->GetID(),
            startParameterName,
            renamedParameterName);
        EXPECT_TRUE(commandManager.ExecuteCommand(command, result));
        EXPECT_EQ(m_parameterNode->GetOutputPorts().size(), 1);
        EXPECT_STREQ(m_parameterNode->GetOutputPorts()[0].GetName(), renamedParameterName);

        // Undo and redo
        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_STREQ(m_parameterNode->GetOutputPorts()[0].GetName(), startParameterName);

        EXPECT_TRUE(commandManager.Redo(result));
        EXPECT_STREQ(m_parameterNode->GetOutputPorts()[0].GetName(), renamedParameterName);
    }
} // namespace EMotionFX
