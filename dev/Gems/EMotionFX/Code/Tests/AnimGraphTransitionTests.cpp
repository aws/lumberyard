
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

#include "AnimGraphTransitionFixture.h"
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>

namespace EMotionFX
{

    class AnimGraphTransitionFixtureParams : public AnimGraphTransitionFixture,
                                             public ::testing::WithParamInterface<decltype(&AnimGraphNodeData::GetLocalWeight)>
    {
    };

    TEST_P(AnimGraphTransitionFixtureParams, TestAnimGraphTransitionWeight)
    {
        // TODO use constexpr when we ditch msvc2013
        static const float fps = 60.0f;
        static const float updateInterval = 1.0f / fps;
        static const int numIterations = static_cast<int>(3.0f * fps);

        // Run the EMotionFX update loop for 3 seconds at 60 fps
        for (int i = 0; i < numIterations; ++i)
        {
            GetEMotionFX().Update(updateInterval);

            const AZStd::vector<AnimGraphNode*>& activeStates = mStateMachine->GetActiveStates(mAnimGraphInstance);

            float weight = 0.0f;
            for (const AnimGraphNode* activeState : activeStates)
            {
                for (int j = 0; j < 2; ++j)
                {
                    AnimGraphMotionNode* motionNode = j == 0 ? mMotionNode0 : mMotionNode1;
                    // Check if the node is active
                    if (motionNode == activeState)
                    {
                        AnimGraphNodeData* uniqueData = motionNode->FindUniqueNodeData(mAnimGraphInstance);
                        weight += (uniqueData->*(GetParam()))();
                    }
                }
            }
            // The combined weights for the active nodes should be close to 1
            EXPECT_FLOAT_EQ(weight, 1.0f) << "frame " << i;
        }
    }

    INSTANTIATE_TEST_CASE_P(TestAnimGraphTransitionWeights, AnimGraphTransitionFixtureParams,
        ::testing::Values(
            &AnimGraphNodeData::GetLocalWeight,
            &AnimGraphNodeData::GetGlobalWeight
        )
    );
} // end namespace EMotionFX
