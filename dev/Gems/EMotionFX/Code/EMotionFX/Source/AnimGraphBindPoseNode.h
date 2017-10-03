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
    class EMFX_API AnimGraphBindPoseNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphBindPoseNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(AnimGraphBindPoseNode, "{72595B5C-045C-4DB1-88A4-40BC4560D7AF}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000017
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

        static AnimGraphBindPoseNode* Create(AnimGraph* animGraph);

        void Init(AnimGraphInstance* animGraphInstance) override;
        void RegisterPorts() override;
        void RegisterAttributes() override;

        uint32 GetVisualColor() const override                  { return MCore::RGBA(50, 200, 50); }
        bool GetCanActAsState() const override                  { return true; }
        bool GetSupportsVisualization() const override          { return true; }
        bool GetHasOutputPose() const override                  { return true; }
        AnimGraphObjectData* CreateObjectData() override;

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;

    private:
        AnimGraphBindPoseNode(AnimGraph* animGraph);
        ~AnimGraphBindPoseNode();

        void Output(AnimGraphInstance* animGraphInstance) override;
    };
}   // namespace EMotionFX
