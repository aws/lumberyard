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
    class EMFX_API BlendTreeMaskNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeMaskNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeMaskNode, "{24647B8B-05B4-4D5D-9161-F0AD0B456B09}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000016
        };

        //
        enum
        {
            INPUTPORT_POSE_0    = 0,
            INPUTPORT_POSE_1    = 1,
            INPUTPORT_POSE_2    = 2,
            INPUTPORT_POSE_3    = 3,
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_INPUT_POSE_0     = 0,
            PORTID_INPUT_POSE_1     = 1,
            PORTID_INPUT_POSE_2     = 2,
            PORTID_INPUT_POSE_3     = 3,
            PORTID_OUTPUT_RESULT    = 0
        };

        enum
        {
            ATTRIB_MASK_0   = 0,
            ATTRIB_MASK_1   = 1,
            ATTRIB_MASK_2   = 2,
            ATTRIB_MASK_3   = 3,
            ATTRIB_EVENTS_0 = 4,
            ATTRIB_EVENTS_1 = 5,
            ATTRIB_EVENTS_2 = 6,
            ATTRIB_EVENTS_3 = 7
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
                : AnimGraphNodeData(node, animGraphInstance)     { mMustUpdate = true; mMasks.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTUNIQUEDATA); }
            ~UniqueData()   {}

            uint32 GetClassSize() const override                                                                                        { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override            { return new (destMem) UniqueData(static_cast<AnimGraphNode*>(object), animGraphInstance); }

        public:
            MCore::Array< MCore::Array<uint32> >    mMasks;
            bool                                    mMustUpdate;
        };

        static BlendTreeMaskNode* Create(AnimGraph* animGraph);

        void Init(AnimGraphInstance* animGraphInstance) override;
        void RegisterPorts() override;
        void RegisterAttributes() override;
        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        bool GetHasOutputPose() const override                      { return true; }
        bool GetSupportsVisualization() const override              { return true; }
        uint32 GetVisualColor() const override                      { return MCore::RGBA(50, 200, 50); }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override         { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

        void OnUpdateAttributes() override;

    private:
        BlendTreeMaskNode(AnimGraph* animGraph);
        ~BlendTreeMaskNode();

        void UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
