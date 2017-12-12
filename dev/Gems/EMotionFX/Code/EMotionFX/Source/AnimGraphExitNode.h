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
#include "MemoryCategories.h"
#include "AnimGraphNode.h"


namespace EMotionFX
{
    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphExitNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphExitNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_STATEMACHINES);
    public:
        AZ_RTTI(AnimGraphExitNode, "{B589D37C-2ECD-4033-8FA9-9483BB098C60}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x32521069
        };

        //
        enum
        {
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_OUTPUT_POSE  = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
                : AnimGraphNodeData(node, animGraphInstance)             { mPreviousNode = nullptr; }
            ~UniqueData() {}

            uint32 GetClassSize() const override                                                                                                { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override                    { return new (destMem) UniqueData(static_cast<AnimGraphNode*>(object), animGraphInstance); }

            void SetPreviousNode(AnimGraphNode* mPreviousNode);
            void Reset() override   { mPreviousNode = nullptr; }

        public:
            AnimGraphNode* mPreviousNode;
        };

        static AnimGraphExitNode* Create(AnimGraph* animGraph);

        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        void RegisterPorts() override;
        void RegisterAttributes() override;

        uint32 GetVisualColor() const override                      { return MCore::RGBA(255, 0, 0); }
        bool GetCanActAsState() const override                      { return true; }
        bool GetSupportsVisualization() const override              { return true; }

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        bool GetHasOutputPose() const override                      { return true; }
        bool GetIsDeletable() const override                        { return false; }
        bool GetIsLastInstanceDeletable() const override            { return false; }
        bool GetCanBeInsideSubStateMachineOnly() const override     { return true; }
        bool GetHasVisualOutputPorts() const override               { return false; }
        bool GetCanHaveOnlyOneInsideParent() const override         { return true; }
        AnimGraphObjectData* CreateObjectData() override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;

        void RecursiveResetFlags(AnimGraphInstance* animGraphInstance, uint32 flagsToDisable) override;
        void OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* previousState, AnimGraphStateTransition* usedTransition) override;
        void Rewind(AnimGraphInstance* animGraphInstance) override;

    private:
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        AnimGraphExitNode(AnimGraph* animGraph);
        ~AnimGraphExitNode();
    };
}   // namespace EMotionFX
