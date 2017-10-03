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
    // forward declarations
    class AnimGraphPose;
    class AnimGraphInstance;


    class EMFX_API BlendTreeBlend2Node
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeBlend2Node, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeBlend2Node, "{2079733F-10C1-4ECB-91F6-03DEDAD2B3FE}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000004
        };

        enum
        {
            ATTRIB_SYNC     = 0,    // sync mode (integer)
            ATTRIB_EVENTMODE = 1,    // event mode (integer)
            ATTRIB_MASK     = 2,
            ATTRIB_ADDITIVE = 3     // additive blending? (bool)
        };

        enum
        {
            INPUTPORT_POSE_A    = 0,
            INPUTPORT_POSE_B    = 1,
            INPUTPORT_WEIGHT    = 2,
            OUTPUTPORT_POSE     = 0
        };

        enum
        {
            PORTID_INPUT_POSE_A = 0,
            PORTID_INPUT_POSE_B = 1,
            PORTID_INPUT_WEIGHT = 2,

            PORTID_OUTPUT_POSE  = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
                : AnimGraphNodeData(node, animGraphInstance)     { mMustUpdate = true; mSyncTrackNode = nullptr; mMask.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTUNIQUEDATA); }

            uint32 GetClassSize() const override                                                                                         { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override            { return new (destMem) UniqueData(static_cast<AnimGraphNode*>(object), animGraphInstance); }

        public:
            MCore::Array<uint32>    mMask;
            AnimGraphNode*          mSyncTrackNode;
            bool                    mMustUpdate;
        };

        static BlendTreeBlend2Node* Create(AnimGraph* animGraph);

        bool GetHasOutputPose() const override              { return true; }
        bool GetSupportsDisable() const override            { return true; }
        bool GetSupportsVisualization() const override      { return true; }
        uint32 GetVisualColor() const override              { return MCore::RGBA(159, 81, 255); }

        void FindBlendNodes(AnimGraphInstance* animGraphInstance, AnimGraphNode** outBlendNodeA, AnimGraphNode** outBlendNodeB, float* outWeight, bool isAdditive);
        void Init(AnimGraphInstance* animGraphInstance) override;
        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;

        void RegisterPorts() override;
        void RegisterAttributes() override;
        bool ConvertAttribute(uint32 attributeIndex, const MCore::Attribute* attributeToConvert, const MCore::String& attributeName) override;

        void UpdateMotionExtractionDeltaNoFeathering(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeA, AnimGraphNode* nodeB, float weight, UniqueData* uniqueData);
        void UpdateMotionExtractionDeltaFeathering(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeA, AnimGraphNode* nodeB, float weight, UniqueData* uniqueData);

        const char* GetTypeString() const override;
        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

        void OnUpdateAttributes() override;

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

    private:
        BlendTreeBlend2Node(AnimGraph* animGraph);
        ~BlendTreeBlend2Node();

        void UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Output(AnimGraphInstance* animGraphInstance) override;
        void OutputNoFeathering(AnimGraphInstance* animGraphInstance);
        void OutputFeathering(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
    };
}   // namespace EMotionFX
