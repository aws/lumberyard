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
#include "BlendTreeFinalNode.h"


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class Pose;
    class AnimGraphNode;


    /**
     *
     *
     *
     */
    class EMFX_API BlendTree
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTree, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREES);

    public:
        AZ_RTTI(BlendTree, "{A8B5BB1E-5BA9-4B0A-88E9-21BB7A199ED2}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000006
        };

        enum
        {
            OUTPUTPORT_POSE = 0
        };

        enum
        {
            PORTID_OUTPUT_POSE = 0
        };

        static BlendTree* Create(AnimGraph* animGraph, const char* name = nullptr);

        // overloaded
        const char* GetTypeString() const override                      { return "BlendTree"; }
        const char* GetPaletteName() const override                     { return "Blend Tree"; }
        AnimGraphObject::ECategory GetPaletteCategory() const override { return AnimGraphObject::CATEGORY_SOURCES; }
        bool GetCanActAsState() const override                          { return true; }
        bool GetHasVisualGraph() const override                         { return true; }
        bool GetCanHaveChildren() const override                        { return true; }
        bool GetSupportsDisable() const override                        { return true; }
        bool GetSupportsVisualization() const override                  { return true; }
        uint32 GetVisualColor() const override                          { return MCore::RGBA(53, 170, 53); }
        uint32 GetHasChildIndicatorColor() const override               { return MCore::RGBA(1, 193, 68); }
        AnimGraphObjectData* CreateObjectData() override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;

        void RegisterPorts() override;
        void RegisterAttributes() override;

        void Rewind(AnimGraphInstance* animGraphInstance) override;
        bool GetHasOutputPose() const override   { return true; }

        void SetVirtualFinalNode(AnimGraphNode* node);
        void SetFinalNode(BlendTreeFinalNode* finalNode);
        MCORE_INLINE AnimGraphNode* GetVirtualFinalNode() const        { return mVirtualFinalNode; }
        MCORE_INLINE BlendTreeFinalNode* GetFinalNode()                 { return mFinalNode; }

        // remove the node and auto delete connections to this node
        void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove) override;

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override             { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        void RecursiveSetUniqueDataFlag(AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled) override;
        AnimGraphNode* GetRealFinalNode() const;

        void RecursiveClonePostProcess(AnimGraphNode* resultNode) override;

    private:
        BlendTreeFinalNode*     mFinalNode;         /**< The final node, who's input will be what the output of this tree will be. */
        AnimGraphNode*         mVirtualFinalNode;  /**< The virtual final node, which is the node who's output is used as final output. A value of nullptr means it will use the real mFinalNode. */

        BlendTree(AnimGraph* animGraph, const char* name = nullptr);
        ~BlendTree();

        void RecursiveSetUniqueDataFlag(AnimGraphNode* startNode, AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled);
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
