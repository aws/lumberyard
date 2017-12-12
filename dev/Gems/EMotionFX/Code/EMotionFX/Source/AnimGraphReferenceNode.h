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
     *
     *
     */
    class EMFX_API AnimGraphReferenceNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphReferenceNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);
    public:
        AZ_RTTI(AnimGraphReferenceNode, "{FA2BFE62-B8D6-44FD-AAD2-3A591C784EE2}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000a0a
        };

        enum
        {
            ATTRIB_REFERENCENODE    = 0
        };

        //
        enum
        {
            OUTPUTPORT_RESULT       = 0
        };

        enum
        {
            PORTID_OUTPUT_POSE      = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance, AnimGraphInstance* internalAnimGraphInstance)
                : AnimGraphNodeData(node, animGraphInstance)
            {
                mInternalAnimGraphInstance = internalAnimGraphInstance;
                mNeedsUpdate        = true;
                mDeletedNode        = nullptr;
            }

            ~UniqueData()
            {
                if (mInternalAnimGraphInstance)
                {
                    mInternalAnimGraphInstance->Destroy();
                }
            }

            uint32 GetClassSize() const override                                                                                    { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override        { return new (destMem) UniqueData(static_cast<AnimGraphNode*>(object), animGraphInstance, nullptr); }
            //void Reset();

        public:
            AnimGraphInstance* mInternalAnimGraphInstance;
            AnimGraphNode*     mDeletedNode;
            bool                mNeedsUpdate;
        };

        static AnimGraphReferenceNode* Create(AnimGraph* animGraph);

        void RegisterPorts() override;
        void RegisterAttributes() override;
        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        void OnUpdateAttributes() override;
        AnimGraphObjectData* CreateObjectData() override;

        void OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const MCore::String& oldName) override;
        void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove) override;
        void OnCreatedNode(AnimGraph* animGraph, AnimGraphNode* node) override;

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override             { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        void SetCurrentPlayTime(AnimGraphInstance* animGraphInstance, float timeInSeconds) override;
        //float GetDuration(AnimGraphInstance* animGraphInstance) const;
        //float GetCurrentPlayTime(AnimGraphInstance* animGraphInstance) const;
        //float GetPlaySpeed(AnimGraphInstance* animGraphInstance) const;
        void Rewind(AnimGraphInstance* animGraphInstance) override;
        void SetPlaySpeed(AnimGraphInstance* animGraphInstance, float speedFactor) override;

        uint32 GetVisualColor() const override                      { return MCore::RGBA(50, 200, 50); }
        bool GetCanActAsState() const override                      { return true; }
        bool GetSupportsVisualization() const override              { return true; }
        bool GetHasOutputPose() const override                      { return true; }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;

    private:
        AnimGraph*     mRefAnimGraph;

        AnimGraphReferenceNode(AnimGraph* animGraph);
        ~AnimGraphReferenceNode();

        void UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
