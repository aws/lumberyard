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

#include "EMotionFXConfig.h"
#include "AnimGraphTransitionCondition.h"
#include "EventManager.h"
#include "AnimGraph.h"


namespace EMotionFX
{
    AnimGraphTransitionCondition::AnimGraphTransitionCondition(AnimGraph* animGraph, uint32 typeID)
        : AnimGraphObject(animGraph, typeID)
    {
        mPreviousTestResult = false;

        if (animGraph)
        {
            animGraph->AddObject(this);
        }
    }


    AnimGraphTransitionCondition::~AnimGraphTransitionCondition()
    {
        if (mAnimGraph)
        {
            mAnimGraph->RemoveObject(this);
        }
    }


    uint32 AnimGraphTransitionCondition::GetBaseType() const
    {
        return BASETYPE_ID;
    }


    AnimGraphObject::ECategory AnimGraphTransitionCondition::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_TRANSITIONCONDITIONS;
    }


    // Update the test result from the last test condition call and call the corresponding event.
    void AnimGraphTransitionCondition::UpdatePreviousTestResult(AnimGraphInstance* animGraphInstance, bool newTestResult)
    {
        // Call the event in case the condition got triggered.
        if (newTestResult != mPreviousTestResult)
        {
            GetEventManager().OnConditionTriggered(animGraphInstance, this);
        }

        // Update the previous test result.
        mPreviousTestResult = newTestResult;
    }


    AnimGraphObject* AnimGraphTransitionCondition::RecursiveClone(AnimGraph* animGraph, AnimGraphObject* parentObject)
    {
        MCORE_UNUSED(parentObject);

        // Clone the condition (constructor already adds the object to the animgraph).
        AnimGraphObject* result = Clone(animGraph);
        MCORE_ASSERT(GetType() == result->GetType());

        // Copy attribute values.
        const uint32 numAttributes = mAttributeValues.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            result->GetAttribute(i)->InitFrom(mAttributeValues[i]);
        }

        return result;
    }
} // namespace EMotionFX