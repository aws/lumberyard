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

#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphTriggerActionCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphParameterAction.h>
#include <EMotionFX/Source/AnimGraphServantParameterAction.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphSymbolicServantParameterAction.h>
#include <MCore/Source/CommandGroup.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    class AnimGraphSimpleCopyPasteFixture
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<bool>
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();

            /*
                +---+            +---+
                | A |--Actions-->| B |
                +---+            +---+
            */
            m_stateA = aznew AnimGraphMotionNode();
            m_stateA->SetName("A");
            m_rootStateMachine->AddChildNode(m_stateA);
            m_rootStateMachine->SetEntryState(m_stateA);

            m_stateB = aznew AnimGraphMotionNode();
            m_stateB->SetName("B");
            m_rootStateMachine->AddChildNode(m_stateB);

            m_transition = AddTransition(m_stateA, m_stateB, 1.0f);
        }

        AnimGraphNode* m_stateA = nullptr;
        AnimGraphNode* m_stateB = nullptr;
        AnimGraphStateTransition* m_transition = nullptr;
    };

    TEST_P(AnimGraphSimpleCopyPasteFixture, AnimGraphCopyPasteTests_CopyAndPasteTransitionActions)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;
        MCore::CommandGroup commandGroup;
        const bool cutMode = GetParam();

        // 1. Add transition actions.
        CommandSystem::AddTransitionAction(m_transition, azrtti_typeid<AnimGraphParameterAction>());
        CommandSystem::AddTransitionAction(m_transition, azrtti_typeid<AnimGraphServantParameterAction>());
        CommandSystem::AddTransitionAction(m_transition, azrtti_typeid<AnimGraphSymbolicServantParameterAction>());
        EXPECT_EQ(3, m_transition->GetTriggerActionSetup().GetNumActions()) << "There should be exactly three transition actions.";

        // 2. Cut & paste both states
        AZStd::vector<EMotionFX::AnimGraphNode*> nodesToCopy;
        nodesToCopy.emplace_back(m_stateA);
        nodesToCopy.emplace_back(m_stateB);

        CommandSystem::AnimGraphCopyPasteData copyPasteData;
        CommandSystem::ConstructCopyAnimGraphNodesCommandGroup(&commandGroup,
            /*targetParentNode=*/m_rootStateMachine,
            nodesToCopy,
            /*posX=*/0,
            /*posY=*/0,
            /*cutMode=*/cutMode,
            copyPasteData,
            /*ignoreTopLevelConnections=*/false);
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result));

        if (cutMode)
        {
            EXPECT_EQ(1, m_rootStateMachine->GetNumTransitions())
                << "As we only had one transition before the cut & paste operation, there should be exactly one now, too.";

            AnimGraphStateTransition* newTransition = m_rootStateMachine->GetTransition(0);
            const TriggerActionSetup& actionSetup = newTransition->GetTriggerActionSetup();
            EXPECT_EQ(3, actionSetup.GetNumActions()) << "There should be three transition actions again.";
        }
        else
        {
            const size_t numTransitions = m_rootStateMachine->GetNumTransitions();
            EXPECT_EQ(2, numTransitions)
                << "After copy & paste, there should be two transitions.";

            for (size_t i = 0; i < numTransitions; ++i)
            {
                AnimGraphStateTransition* transition = m_rootStateMachine->GetTransition(i);
                const TriggerActionSetup& actionSetup = transition->GetTriggerActionSetup();
                EXPECT_EQ(3, actionSetup.GetNumActions()) << "There should be three transition actions for both transitions.";
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    TEST_P(AnimGraphSimpleCopyPasteFixture, AnimGraphCopyPasteTests_TransitionIds)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;
        MCore::CommandGroup commandGroup;
        const bool cutMode = GetParam();
        const AnimGraphConnectionId oldtransitionId = m_transition->GetId();

        AZStd::vector<EMotionFX::AnimGraphNode*> nodesToCopy;
        nodesToCopy.emplace_back(m_stateA);
        nodesToCopy.emplace_back(m_stateB);

        CommandSystem::AnimGraphCopyPasteData copyPasteData;
        CommandSystem::ConstructCopyAnimGraphNodesCommandGroup(&commandGroup,
            /*targetParentNode=*/m_rootStateMachine,
            nodesToCopy,
            /*posX=*/0,
            /*posY=*/0,
            /*cutMode=*/cutMode,
            copyPasteData,
            /*ignoreTopLevelConnections=*/false);
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result));

        if (cutMode)
        {
            EXPECT_EQ(1, m_rootStateMachine->GetNumTransitions())
                << "As we only had one transition before the cut & paste operation, there should be exactly one now, too.";

            AnimGraphStateTransition* newTransition = m_rootStateMachine->GetTransition(0);
            EXPECT_TRUE(newTransition->GetId() == oldtransitionId) << "There cut & pasted transition should have the same id.";
        }
        else
        {
            const size_t numTransitions = m_rootStateMachine->GetNumTransitions();
            EXPECT_EQ(2, numTransitions)
                << "After copy & paste, there should be two transitions.";

            for (size_t i = 0; i < numTransitions; ++i)
            {
                AnimGraphStateTransition* transition = m_rootStateMachine->GetTransition(i);

                if (transition == m_transition)
                {
                    continue;
                }

                EXPECT_FALSE(transition->GetId() == oldtransitionId) << "There copied transition should have another id. Transition ids need to be unique.";
            }
        }
    }

    INSTANTIATE_TEST_CASE_P(AnimGraphCopyPasteTests,
        AnimGraphSimpleCopyPasteFixture,
        ::testing::Bool());
} // namespace EMotionFX
