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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphEntryNode.h>
#include <EMotionFX/Source/AnimGraphExitNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <Tests/AnimGraphFixture.h>


namespace EMotionFX
{
    class AnimGraphRefCountFixture : public AnimGraphFixture
    {
    public:
        void Run()
        {
            Simulate(/*simulationTime*/10.0f, /*expectedFps*/60.0f, /*fpsVariance*/0.0f,
                /*preCallback*/[this](AnimGraphInstance* animGraphInstance)
                {
                    this->m_animGraphInstance->SetAutoReleaseRefDatas(false);
                    this->m_animGraphInstance->SetAutoReleasePoses(false);
                },
                /*postCallback*/[this](AnimGraphInstance* animGraphInstance){},
                /*preUpdateCallback*/[this](AnimGraphInstance*, float, float, int frame){},
                /*postUpdateCallback*/[this](AnimGraphInstance*, float, float, int frame)
                {
                    const uint32 threadIndex = this->m_actorInstance->GetThreadIndex();

                    // Check if data and pose ref counts are back to 0 for all nodes.
                    AnimGraphRefCountedDataPool& refDataPool = GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool();
                    const uint32 numNodes = this->m_animGraph->GetNumNodes();
                    for (uint32 i = 0; i < numNodes; ++i)
                    {
                        const AnimGraphNode* node = this->m_animGraph->GetNode(i);
                        AnimGraphNodeData* nodeData = static_cast<AnimGraphNodeData*>(this->m_animGraphInstance->GetUniqueObjectData(node->GetObjectIndex()));
                        EXPECT_EQ(0, nodeData->GetRefDataRefCount()) << "Expected the data ref count to be 0 post update.";
                        EXPECT_EQ(0, nodeData->GetPoseRefCount()) << "Expected the pose ref count to be 0 post update.";
                    }

                    this->m_animGraphInstance->ReleaseRefDatas();
                    this->m_animGraphInstance->ReleasePoses();
                });
        }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct AnimGraphRefCountData_SimpleChain
    {
        int m_numStates;
        float m_blendTime;
        float m_countDownTime;
    };

    class AnimGraphRefCountTest_SimpleChain
        : public AnimGraphRefCountFixture
        , public ::testing::WithParamInterface<AnimGraphRefCountData_SimpleChain>
    {
    public:
        void ConstructGraph() override
        {
            const AnimGraphRefCountData_SimpleChain& param = GetParam();
            AnimGraphFixture::ConstructGraph();

            /*
                +-------+    +---+    +---+             +---+
                | Start |--->| A |--->| B |---> ... --->| N |
                +-------+    +---+    +---+             +---+
            */
            AnimGraphBindPoseNode* stateStart = aznew AnimGraphBindPoseNode();
            m_rootStateMachine->AddChildNode(stateStart);
            m_rootStateMachine->SetEntryState(stateStart);

            const char startChar = 'A';
            AnimGraphNode* prevState = stateStart;
            for (int i = 0; i < param.m_numStates; ++i)
            {
                AnimGraphBindPoseNode* state = aznew AnimGraphBindPoseNode();
                state->SetName(AZStd::string(1, startChar + i).c_str());
                m_rootStateMachine->AddChildNode(state);
                AddTransitionWithTimeCondition(prevState, state, /*blendTime*/param.m_blendTime, /*countDownTime*/param.m_countDownTime);
                prevState = state;
            }
        }
    };

    TEST_P(AnimGraphRefCountTest_SimpleChain, AnimGraphRefCountTest_SimpleChain)
    {
        Run();
    }

    std::vector<AnimGraphRefCountData_SimpleChain> animGraphRefCountTest_SimpleChainTestData
    {
        {
            /*m_numStates*/3,
            /*m_blendTime*/1.0,
            /*m_countDownTime*/1.0
        },
        {
            3,
            0.0,
            1.0
        },
        {
            3,
            0.0,
            0.0
        },
        {
            8,
            0.5,
            0.5
        },
        {
            16,
            0.2,
            0.2
        }
    };

    INSTANTIATE_TEST_CASE_P(AnimGraphRefCountTest_SimpleChain,
         AnimGraphRefCountTest_SimpleChain,
         ::testing::ValuesIn(animGraphRefCountTest_SimpleChainTestData)
    );

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /*
    This test fails at frame 362 where the root state machine leaves the transition from the sub state machine and fully blends into the end state.
    Why is this failing? The transition in the sub-state machine is active at frame 362 while it sets to is-done on the beginning of the update. This means that the current state
    has not been adjusted yet, even though the transition has ended as this happens further down in the state machine update. This also means that we update the source node (sub-sm)
    while we don't call output or post update to decrease the ref count. We can't first update and end transitions and then update the states as this will introduce one frame delay
    and make other tests fail again. This only happens with sub-state machines and only at the last frame when transitioning out of sub-state machines.
    */
    class AnimGraphRefCountTest_SimpleEntryExit
        : public AnimGraphRefCountFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();

            AnimGraphBindPoseNode* stateStart = aznew AnimGraphBindPoseNode();
            stateStart->SetName("Start");
            m_rootStateMachine->AddChildNode(stateStart);
            m_rootStateMachine->SetEntryState(stateStart);

            AnimGraphStateMachine* stateMachine = aznew AnimGraphStateMachine();
            stateMachine->SetName("Sub SM");
            m_rootStateMachine->AddChildNode(stateMachine);
            AddTransitionWithTimeCondition(stateStart, stateMachine, 1.0, 1.0);
            {
                AnimGraphEntryNode* subEntryNode = aznew AnimGraphEntryNode();
                subEntryNode->SetName("Entry");
                stateMachine->AddChildNode(subEntryNode);
                stateMachine->SetEntryState(subEntryNode);

                AnimGraphBindPoseNode* subBetweenNode = aznew AnimGraphBindPoseNode();
                subBetweenNode->SetName("Sub In-between");
                stateMachine->AddChildNode(subBetweenNode);
                AddTransitionWithTimeCondition(subEntryNode, subBetweenNode, 0.0, 0.3);

                AnimGraphExitNode* exitNode = aznew AnimGraphExitNode();
                exitNode->SetName("Exit");
                stateMachine->AddChildNode(exitNode);
                AddTransitionWithTimeCondition(subBetweenNode, exitNode, 1.0, 1.0);
            }

            AnimGraphBindPoseNode* stateEnd = aznew AnimGraphBindPoseNode();
            stateEnd->SetName("End");
            m_rootStateMachine->AddChildNode(stateEnd);
            AddTransitionWithTimeCondition(stateMachine, stateEnd, 1.0, 3.0);
        }
    };

    TEST_F(AnimGraphRefCountTest_SimpleEntryExit, DISABLED_AnimGraphRefCountTest_SimpleEntryExit)
    {
        Run();
    }
} // EMotionFX