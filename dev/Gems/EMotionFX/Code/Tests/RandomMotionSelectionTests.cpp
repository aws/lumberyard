
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

#include "AnimGraphFixture.h"
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/Motion.h>
#include <AzCore/std/containers/unordered_map.h>
#include <cmath>

namespace EMotionFX
{

    class RandomMotionSelectionTests : public AnimGraphFixture
    {
    public:
        void TearDown() override
        {
            if (m_selectedMotionCount)
            {
                delete m_selectedMotionCount;
            }
            AnimGraphFixture::TearDown();
        }

        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_motionNode = aznew AnimGraphMotionNode();
            m_animGraph->GetRootStateMachine()->AddChildNode(m_motionNode);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_motionNode);
            m_motionNode->SetIndexMode(EMotionFX::AnimGraphMotionNode::EIndexMode::INDEXMODE_RANDOMIZE);
        }

        void SetUp() override
        {
            m_selectedMotionCount = new AZStd::unordered_map<uint32, uint32>();
            AnimGraphFixture::SetUp();

            const int motionCount = 3;
            for (int i = 0; i < motionCount; ++i)
            {
                // The motion set keeps track of motions by their name. Each motion
                // within the motion set must have a unique name.
                AZStd::string motionId = AZStd::string::format("testSkeletalMotion%i", i);
                SkeletalMotion* motion = SkeletalMotion::Create(motionId.c_str());
                motion->SetMaxTime(1.0f);
                MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(motion->GetName(), motion->GetName(), motion);
                m_motionSet->AddMotionEntry(motionEntry);

                m_motionNode->AddMotionId(motionId.c_str());
            }
        }
        AZStd::unordered_map<uint32, uint32>* m_selectedMotionCount = nullptr;
        AnimGraphMotionNode* m_motionNode = nullptr;
    };

    TEST_F(RandomMotionSelectionTests, RandomizeMotion)
    {
        const int iterationCount = 1000;
        AnimGraphMotionNode::UniqueData* nodeUniqueData = static_cast<AnimGraphMotionNode::UniqueData*>(m_motionNode->FindOrCreateUniqueNodeData(m_animGraphInstance));
        for (int i = 0; i < iterationCount; ++i)
        {
            m_motionNode->PickNewActiveMotion(m_animGraphInstance, nodeUniqueData);
            auto mapIterator = m_selectedMotionCount->find(nodeUniqueData->mActiveMotionIndex);
            if (mapIterator != m_selectedMotionCount->end())
            {
                mapIterator->second++;
            }
            else
            {
                m_selectedMotionCount->emplace(nodeUniqueData->mActiveMotionIndex, 1);
            }
        }

        const double expectedUniformFrequency = 1.0 / m_selectedMotionCount->size();
        const double tolerance = 0.1;
        bool success = true;
        for(auto& motionCountEntry : (*m_selectedMotionCount))
        {
            double frequency = motionCountEntry.second / static_cast<double>(iterationCount);
            bool isInTolerance = std::fabs((frequency - expectedUniformFrequency) / expectedUniformFrequency) < tolerance;
            success = success && isInTolerance;
        }
        ASSERT_TRUE(success);
    }

} // end namespace EMotionFX
