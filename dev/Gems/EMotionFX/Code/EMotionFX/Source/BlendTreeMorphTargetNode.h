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
    class EMFX_API BlendTreeMorphTargetNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeMorphTargetNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeMorphTargetNode, "{E9C9DFD0-565A-4B2D-9D0C-BB9F056D48D7}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00002445
        };

        enum
        {
            ATTRIB_MORPHTARGET  = 0
        };

        enum
        {
            INPUTPORT_POSE      = 0,
            INPUTPORT_WEIGHT    = 1,
            OUTPUTPORT_POSE     = 0
        };

        enum
        {
            PORTID_INPUT_POSE   = 0,
            PORTID_INPUT_WEIGHT = 1,
            PORTID_OUTPUT_POSE  = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
                : AnimGraphNodeData(node, animGraphInstance)
                , m_lastLodLevel(MCORE_INVALIDINDEX32)
                , m_morphTargetIndex(MCORE_INVALIDINDEX32)
            {}
            ~UniqueData() override = default;

            uint32 GetClassSize() const override    { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override   { return new (destMem) UniqueData(static_cast<AnimGraphNode*>(object), animGraphInstance); }

        public:
            uint32 m_lastLodLevel;
            uint32 m_morphTargetIndex;
        };

        static BlendTreeMorphTargetNode* Create(AnimGraph* animGraph);

        void Init(AnimGraphInstance* animGraphInstance) override;
        bool GetHasOutputPose() const override              { return true; }
        bool GetSupportsVisualization() const override      { return true; }
        bool GetSupportsDisable() const override            { return true; }
        uint32 GetVisualColor() const override              { return MCore::RGBA(159, 81, 255); }
        const char* GetTypeString() const override;
        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        void RegisterPorts() override;
        void RegisterAttributes() override;

        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

    private:
        BlendTreeMorphTargetNode(AnimGraph* animGraph);
        ~BlendTreeMorphTargetNode() override = default;

        void Output(AnimGraphInstance* animGraphInstance) override;
        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        void UpdateMorphIndices(ActorInstance* actorInstance, UniqueData* uniqueData, bool forceUpdate);
    };
}   // namespace EMotionFX
