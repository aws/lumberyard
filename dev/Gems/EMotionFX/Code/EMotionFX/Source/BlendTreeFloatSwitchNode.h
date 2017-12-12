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
    class EMFX_API BlendTreeFloatSwitchNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeFloatSwitchNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeFloatSwitchNode, "{8DDB9197-74A4-4C75-A58F-5B68C924FCF1}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000012
        };

        //
        enum
        {
            INPUTPORT_0         = 0,
            INPUTPORT_1         = 1,
            INPUTPORT_2         = 2,
            INPUTPORT_3         = 3,
            INPUTPORT_4         = 4,
            INPUTPORT_DECISION  = 5,
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_INPUT_0          = 0,
            PORTID_INPUT_1          = 1,
            PORTID_INPUT_2          = 2,
            PORTID_INPUT_3          = 3,
            PORTID_INPUT_4          = 4,
            PORTID_INPUT_DECISION   = 5,
            PORTID_OUTPUT_RESULT    = 0
        };

        enum
        {
            ATTRIB_STATICVALUE_0    = 0,
            ATTRIB_STATICVALUE_1    = 1,
            ATTRIB_STATICVALUE_2    = 2,
            ATTRIB_STATICVALUE_3    = 3,
            ATTRIB_STATICVALUE_4    = 4
        };

        static BlendTreeFloatSwitchNode* Create(AnimGraph* animGraph);

        void RegisterPorts() override;
        void RegisterAttributes() override;

        void OnUpdateAttributes() override;

        uint32 GetVisualColor() const override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

    private:
        BlendTreeFloatSwitchNode(AnimGraph* animGraph);
        ~BlendTreeFloatSwitchNode();

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
