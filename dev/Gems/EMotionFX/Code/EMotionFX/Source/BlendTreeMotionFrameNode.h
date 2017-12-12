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
#include "AnimGraphNode.h"
#include "AnimGraphNodeData.h"


namespace EMotionFX
{
    /**
     *
     *
     *
     */
    class EMFX_API BlendTreeMotionFrameNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeMotionFrameNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeMotionFrameNode, "{37B59DF1-496E-453C-91F3-D51821CC3919}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000018
        };

        //
        enum
        {
            INPUTPORT_MOTION        = 0,
            INPUTPORT_TIME          = 1,
            OUTPUTPORT_RESULT       = 0
        };

        enum
        {
            PORTID_INPUT_MOTION     = 0,
            PORTID_INPUT_TIME       = 1,
            PORTID_OUTPUT_RESULT    = 0
        };

        enum
        {
            ATTRIB_TIME             = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
                : AnimGraphNodeData(node, animGraphInstance) { mOldTime = mNewTime = 0.0f; }
            ~UniqueData() {}

            uint32 GetClassSize() const override                                                                                    { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override        { return new (destMem) UniqueData(static_cast<AnimGraphNode*>(object), animGraphInstance); }

            void Reset() override
            {
                mOldTime = 0.0f;
                mNewTime = 0.0f;
            }

        public:
            float   mOldTime;
            float   mNewTime;
        };

        static BlendTreeMotionFrameNode* Create(AnimGraph* animGraph);

        void RegisterPorts() override;
        void RegisterAttributes() override;

        void Init(AnimGraphInstance* animGraphInstance) override;
        bool GetHasOutputPose() const override                                                  { return true; }
        bool GetSupportsVisualization() const override                                          { return true; }
        uint32 GetVisualColor() const override                                                  { return MCore::RGBA(50, 200, 50); }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }
        AnimGraphObjectData* CreateObjectData() override;
        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;

    private:
        BlendTreeMotionFrameNode(AnimGraph* animGraph);
        ~BlendTreeMotionFrameNode();

        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
