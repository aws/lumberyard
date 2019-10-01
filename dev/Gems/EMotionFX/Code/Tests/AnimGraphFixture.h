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

#pragma once

#include "SystemComponentFixture.h"
#include <EMotionFX/Source/MotionSet.h>



namespace EMotionFX
{
    class Actor;
    class ActorInstance;
    class AnimGraph;
    class AnimGraphStateMachine;
    class AnimGraphStateTransition;
    class AnimGraphInstance;
    class AnimGraphTimeCondition;
    class Transform;

    class AnimGraphFixture : public SystemComponentFixture
    {
    public:
        void SetUp() override;
        void TearDown() override;

        // Derived classes should override and construct the rest of the graph at this point
        // They should call this base ConstructGrpah to get the anim graph and root state 
        // machine created
        virtual void ConstructGraph();

        AZStd::string SerializeAnimGraph() const;

        // Evaluates the graph
        void Evaluate();

        const Transform& GetOutputTransform(uint32 nodeIndex = 0);

        void AddValueParameter(const AZ::TypeId& typeId, const AZStd::string& name);

        // Helper functions for state machine construction (Works on m_rootStateMachine)
        AnimGraphStateTransition* AddTransition(AnimGraphNode* source, AnimGraphNode* target, float time);
        AnimGraphTimeCondition* AddTimeCondition(AnimGraphStateTransition* transition, float countDownTime);
        AnimGraphStateTransition* AddTransitionWithTimeCondition(AnimGraphNode* source, AnimGraphNode* target, float blendTime, float countDownTime);

        // Helper function for motion set construction (Works on m_motionSet)
        MotionSet::MotionEntry* AddMotionEntry(const AZStd::string& motionId, float motionMaxTime);


        using SimulateFrameCallback = std::function<void(AnimGraphInstance*, /*time*/float, /*timeDelta*/float, /*frame*/int)>;
        using SimulateCallback = std::function<void(AnimGraphInstance*)>;

        /**
         * Simulation helper with callbacks before and after starting the simulation as well as
         * callbakcs before and after the anim graph update.
         * Example: expectedFps = 60, fpsVariance = 10 -> actual framerate = [55, 65]
         * @param[in] simulationTime Simulation time in seconds.
         * @param[in] expectedFps is the targeted frame rate
         * @param[in] fpsVariance is the range in which the instabilities happen.
         */
        void Simulate(float simulationTime, float expectedFps, float fpsVariance,
            SimulateCallback preCallback,
            SimulateCallback postCallback,
            SimulateFrameCallback preUpdateCallback,
            SimulateFrameCallback postUpdateCallback);

    protected:
        Actor* m_actor = nullptr;
        ActorInstance* m_actorInstance = nullptr;
        AnimGraph* m_animGraph = nullptr;
        AnimGraphStateMachine* m_rootStateMachine = nullptr;
        AnimGraphInstance* m_animGraphInstance = nullptr;
        MotionSet* m_motionSet = nullptr;        
    };
}
