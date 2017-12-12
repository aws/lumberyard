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

// include the required headers
#include "EMotionFXConfig.h"
//#include "Pose.h"
#include "AnimGraphNode.h"


namespace EMotionFX
{
    class EMFX_API BlendTreePoseSwitchNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreePoseSwitchNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreePoseSwitchNode, "{1CB88289-B0B1-46D7-B218-DF3E5258B6B1}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000014
        };

        enum
        {
            INPUTPORT_POSE_0        = 0,
            INPUTPORT_POSE_1        = 1,
            INPUTPORT_POSE_2        = 2,
            INPUTPORT_POSE_3        = 3,
            INPUTPORT_POSE_4        = 4,
            INPUTPORT_POSE_5        = 5,
            INPUTPORT_POSE_6        = 6,
            INPUTPORT_POSE_7        = 7,
            INPUTPORT_POSE_8        = 8,
            INPUTPORT_POSE_9        = 9,
            INPUTPORT_DECISIONVALUE = 10,
            OUTPUTPORT_POSE         = 0
        };

        enum
        {
            PORTID_INPUT_POSE_0         = 0,
            PORTID_INPUT_POSE_1         = 1,
            PORTID_INPUT_POSE_2         = 2,
            PORTID_INPUT_POSE_3         = 3,
            PORTID_INPUT_POSE_4         = 4,
            PORTID_INPUT_POSE_5         = 5,
            PORTID_INPUT_POSE_6         = 6,
            PORTID_INPUT_POSE_7         = 7,
            PORTID_INPUT_POSE_8         = 8,
            PORTID_INPUT_POSE_9         = 9,
            PORTID_INPUT_DECISIONVALUE  = 10,
            PORTID_OUTPUT_POSE          = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance, int32 decisionIndex)
                : AnimGraphNodeData(node, animGraphInstance)        { mDecisionIndex = decisionIndex; }

            uint32 GetClassSize() const override                                                                                    { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override        { return new (destMem) UniqueData(static_cast<AnimGraphNode*>(object), animGraphInstance, MCORE_INVALIDINDEX32); }

        public:
            int32   mDecisionIndex;
        };

        static BlendTreePoseSwitchNode* Create(AnimGraph* animGraph);

        void Init(AnimGraphInstance* animGraphInstance) override;
        bool GetHasOutputPose() const override              { return true; }
        bool GetSupportsVisualization() const override      { return true; }
        uint32 GetVisualColor() const override              { return MCore::RGBA(159, 81, 255); }
        const char* GetTypeString() const override;
        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        void RegisterPorts() override;
        void RegisterAttributes() override;

        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;

        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

    private:
        BlendTreePoseSwitchNode(AnimGraph* animGraph);
        ~BlendTreePoseSwitchNode();

        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
