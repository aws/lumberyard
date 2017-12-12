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
    class EMFX_API BlendTreeRetargetPoseNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeRetargetPoseNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeRetargetPoseNode, "{06658112-849B-41DC-B896-3C11DA29C03F}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000215
        };

        //
        enum
        {
            INPUTPORT_POSE      = 0,
            INPUTPORT_ENABLED   = 1,
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_INPUT_POSE   = 0,
            PORTID_INPUT_ENABLED = 1,
            PORTID_OUTPUT_POSE  = 0
        };

        static BlendTreeRetargetPoseNode* Create(AnimGraph* animGraph);

        void Init(AnimGraphInstance* animGraphInstance) override;
        void RegisterPorts() override;
        void RegisterAttributes() override;

        uint32 GetVisualColor() const override                  { return MCore::RGBA(50, 200, 50); }
        bool GetCanActAsState() const override                  { return false; }
        bool GetSupportsVisualization() const override          { return true; }
        bool GetHasOutputPose() const override                  { return true; }
        bool GetSupportsDisable() const                         { return true; }
        AnimGraphObjectData* CreateObjectData() override;

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;

        bool GetIsRetargetEnabled(AnimGraphInstance* animGraphInstance) const;

    private:
        BlendTreeRetargetPoseNode(AnimGraph* animGraph);
        ~BlendTreeRetargetPoseNode();

        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
