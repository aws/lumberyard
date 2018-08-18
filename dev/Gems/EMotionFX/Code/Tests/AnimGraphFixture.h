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

namespace EMotionFX
{
    class Actor;
    class ActorInstance;
    class AnimGraph;
    class AnimGraphInstance;
    class MotionSet;
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

        // Evaluates the graph
        void Evaluate();

        const Transform& GetOutputTransform(uint32 nodeIndex = 0);

        void AddValueParameter(const AZ::TypeId& typeId, const AZStd::string& name);

    protected:
        Actor* m_actor = nullptr;
        ActorInstance* m_actorInstance = nullptr;
        AnimGraph* m_animGraph = nullptr;
        AnimGraphInstance* m_animGraphInstance = nullptr;
        MotionSet* m_motionSet = nullptr;        
    };
}
