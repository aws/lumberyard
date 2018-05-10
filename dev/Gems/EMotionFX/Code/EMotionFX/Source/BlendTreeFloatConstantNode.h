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

#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"


namespace EMotionFX
{
    /**
     *
     */
    class EMFX_API BlendTreeFloatConstantNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeFloatConstantNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeFloatConstantNode, "{033D3D2F-04D3-439F-BFC3-1BDE16BBE37A}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000148
        };

        enum
        {
            ATTRIB_VALUE = 0
        };

        enum
        {
            OUTPUTPORT_RESULT = 0
        };

        enum
        {
            PORTID_OUTPUT_RESULT = 0
        };

        static BlendTreeFloatConstantNode* Create(AnimGraph* animGraph);

        void RegisterPorts() override;
        void RegisterAttributes() override;

        uint32 GetVisualColor() const override;
        bool GetSupportsDisable() const override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

    private:
        BlendTreeFloatConstantNode(AnimGraph* animGraph);
        ~BlendTreeFloatConstantNode();

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void OnUpdateAttributes() override;
    };

}   // namespace EMotionFX
