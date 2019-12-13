
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
#include <AzCore/RTTI/TypeInfo.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphParameterAction.h>
#include <EMotionFX/Source/AnimGraphTimeCondition.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>


namespace EMotionFX
{
    class AnimGraphParameterActionTests : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();

            // 1. Add two state machines.
            m_node1 = aznew AnimGraphStateMachine();
            m_animGraph->GetRootStateMachine()->AddChildNode(m_node1);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_node1);

            m_node2 = aznew AnimGraphStateMachine();
            m_animGraph->GetRootStateMachine()->AddChildNode(m_node2);

            // 2. Add a transition and add a time condition.
            AnimGraphStateTransition* transition = AddTransition(m_node1, m_node2, 1.0f);
            AnimGraphTimeCondition* condition = aznew AnimGraphTimeCondition();
            condition->SetCountDownTime(0.1f);
            transition->AddCondition(condition);

            // 3. Add a parameter action.
            m_parameterAction = aznew AnimGraphParameterAction();
            TriggerActionSetup& actionSetup = transition->GetTriggerActionSetup();
            actionSetup.AddAction(m_parameterAction);
        }

        AnimGraphNode* m_node1 = nullptr;
        AnimGraphNode* m_node2 = nullptr;
        AnimGraphParameterAction* m_parameterAction = nullptr;
    };

    TEST_F(AnimGraphParameterActionTests, AnimGraphParameterActionTests_FloatParameter)
    {
        FloatParameter* parameter = aznew FloatSliderParameter();
        parameter->SetName("testFloat");
        parameter->SetDefaultValue(0.0f);
        m_animGraph->AddParameter(parameter);
        m_animGraphInstance->AddMissingParameterValues();

        const float triggerValue = 100.0f;
        m_parameterAction->SetParameterName("testFloat");
        m_parameterAction->SetTriggerValue(triggerValue);

        float outValue;
        EXPECT_TRUE(m_animGraphInstance->GetParameterValueAsFloat("testFloat", &outValue));
        EXPECT_NE(outValue, triggerValue);

        GetEMotionFX().Update(0.5f);

        EXPECT_TRUE(m_animGraphInstance->GetParameterValueAsFloat("testFloat", &outValue));
        EXPECT_EQ(outValue, triggerValue);

        GetEMotionFX().Update(1.0f);

        EXPECT_TRUE(m_animGraphInstance->GetParameterValueAsFloat("testFloat", &outValue));
        EXPECT_EQ(outValue, triggerValue)
            << "Expect the value get changed after the parameter action triggered.";
    }
}