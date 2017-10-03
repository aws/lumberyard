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
    class EMFX_API BlendTreeRangeRemapperNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeRangeRemapperNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);
    public:
        AZ_RTTI(BlendTreeRangeRemapperNode, "{D60E6686-ECBF-4B8F-A5A5-1164EE66C248}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x02094017
        };

        //
        enum
        {
            INPUTPORT_X         = 0,
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_INPUT_X          = 0,
            PORTID_OUTPUT_RESULT    = 1
        };

        enum
        {
            ATTRIB_INPUTMIN     = 0,
            ATTRIB_INPUTMAX     = 1,
            ATTRIB_OUTPUTMIN    = 2,
            ATTRIB_OUTPUTMAX    = 3
        };

        static BlendTreeRangeRemapperNode* Create(AnimGraph* animGraph);

        void RegisterPorts() override;
        void RegisterAttributes() override;

        uint32 GetVisualColor() const override                  { return MCore::RGBA(128, 255, 255); }
        bool GetSupportsDisable() const override                { return true; }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

    private:
        BlendTreeRangeRemapperNode(AnimGraph* animGraph);
        ~BlendTreeRangeRemapperNode();

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
