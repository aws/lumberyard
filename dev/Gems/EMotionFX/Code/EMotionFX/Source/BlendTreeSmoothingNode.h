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


namespace EMotionFX
{
    /**
     *
     */
    class EMFX_API BlendTreeSmoothingNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeSmoothingNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeSmoothingNode, "{80D8C793-3CD4-4216-B804-CC00EAD20FAA}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000456
        };

        enum
        {
            ATTRIB_INTERPOLATIONSPEED   = 0,
            ATTRIB_USESTARTVALUE        = 1,
            ATTRIB_STARTVALUE           = 2
        };

        enum
        {
            INPUTPORT_DEST              = 0,
            OUTPUTPORT_RESULT           = 0
        };

        enum
        {
            PORTID_INPUT_DEST           = 0,
            PORTID_OUTPUT_RESULT        = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
                : AnimGraphNodeData(node, animGraphInstance)     { mCurrentValue = 0.0f; mFrameDeltaTime = 0.0f; }

            uint32 GetClassSize() const override                                                                                        { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override            { return new (destMem) UniqueData(static_cast<AnimGraphNode*>(object), animGraphInstance); }

        public:
            float   mFrameDeltaTime;
            float   mCurrentValue;
        };

        static BlendTreeSmoothingNode* Create(AnimGraph* animGraph);

        void Rewind(AnimGraphInstance* animGraphInstance) override;
        void RegisterPorts() override;
        void RegisterAttributes() override;
        uint32 GetVisualColor() const override;
        bool GetSupportsDisable() const override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        void OnUpdateAttributes() override;
        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        AnimGraphObjectData* CreateObjectData() override;

    private:
        BlendTreeSmoothingNode(AnimGraph* animGraph);
        ~BlendTreeSmoothingNode();

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
} // namespace EMotionFX
