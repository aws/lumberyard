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
    class EMFX_API AnimGraphEntryNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphEntryNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);
    public:
        AZ_RTTI(AnimGraphEntryNode, "{3F02348C-07CC-4303-B1C9-D4585CE04529}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x38020071
        };

        //
        enum
        {
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_OUTPUT_POSE = 0
        };

        static AnimGraphEntryNode* Create(AnimGraph* animGraph);

        void RegisterPorts() override;
        void RegisterAttributes() override;

        uint32 GetVisualColor() const override                      { return MCore::RGBA(50, 200, 50); }
        bool GetCanActAsState() const override                      { return true; }
        bool GetSupportsVisualization() const override              { return true; }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }
        bool GetHasOutputPose() const override                      { return true; }
        bool GetCanBeInsideSubStateMachineOnly() const override     { return true; }
        bool GetHasVisualOutputPorts() const override               { return false; }
        bool GetCanHaveOnlyOneInsideParent() const override         { return true; }
        AnimGraphObjectData* CreateObjectData() override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;

        AnimGraphNode* FindSourceNode(AnimGraphInstance* animGraphInstance);

    private:
        AnimGraphEntryNode(AnimGraph* animGraph);
        ~AnimGraphEntryNode();

        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
