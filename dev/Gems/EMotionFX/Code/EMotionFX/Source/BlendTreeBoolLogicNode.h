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
    class EMFX_API BlendTreeBoolLogicNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeBoolLogicNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeBoolLogicNode, "{1C7C02C1-D55A-4F2B-8947-BC5163916BBA}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000011
        };

        //
        enum
        {
            INPUTPORT_X         = 0,
            INPUTPORT_Y         = 1,
            OUTPUTPORT_VALUE    = 0,
            OUTPUTPORT_BOOL     = 1
        };

        enum
        {
            PORTID_INPUT_X      = 0,
            PORTID_INPUT_Y      = 1,
            PORTID_OUTPUT_VALUE = 0,
            PORTID_OUTPUT_BOOL  = 1
        };

        enum EFunction
        {
            FUNCTION_AND        = 0,
            FUNCTION_OR         = 1,
            FUNCTION_XOR        = 2,
            NUM_FUNCTIONS
        };

        enum
        {
            ATTRIB_FUNCTION         = 0,
            ATTRIB_STATICVALUE      = 1,
            ATTRIB_TRUEVALUE        = 2,
            ATTRIB_FALSEVALUE       = 3
        };

        static BlendTreeBoolLogicNode* Create(AnimGraph* animGraph);

        void RegisterPorts() override;
        void RegisterAttributes() override;

        void SetFunction(EFunction func);

        uint32 GetVisualColor() const override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

    private:
        typedef bool (MCORE_CDECL * BlendTreeBoolLogicFunction)(bool x, bool y);

        EFunction                   mFunctionEnum;
        BlendTreeBoolLogicFunction  mFunction;

        BlendTreeBoolLogicNode(AnimGraph* animGraph);
        ~BlendTreeBoolLogicNode();

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void OnUpdateAttributes() override;

        static bool MCORE_CDECL BoolLogicAND(bool x, bool y);
        static bool MCORE_CDECL BoolLogicOR(bool x, bool y);
        static bool MCORE_CDECL BoolLogicXOR(bool x, bool y);
    };
}   // namespace EMotionFX
