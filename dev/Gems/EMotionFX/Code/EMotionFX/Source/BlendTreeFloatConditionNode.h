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
    class EMFX_API BlendTreeFloatConditionNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeFloatConditionNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeFloatConditionNode, "{1FA8AD35-8730-49AB-97FD-A602728DBF22}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000010
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
            FUNCTION_EQUAL          = 0,
            FUNCTION_GREATER        = 1,
            FUNCTION_LESS           = 2,
            FUNCTION_GREATEROREQUAL = 3,
            FUNCTION_LESSOREQUAL    = 4,
            FUNCTION_NOTEQUAL       = 5,
            NUM_FUNCTIONS
        };

        enum
        {
            ATTRIB_FUNCTION         = 0,
            ATTRIB_STATICVALUE      = 1,
            ATTRIB_TRUEVALUE        = 2,
            ATTRIB_FALSEVALUE       = 3,
            ATTRIB_TRUEMODE         = 4,
            ATTRIB_FALSEMODE        = 5
        };

        enum
        {
            MODE_VALUE  = 0,
            MODE_X      = 1,
            MODE_Y      = 2
        };

        static BlendTreeFloatConditionNode* Create(AnimGraph* animGraph);

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
        typedef bool (MCORE_CDECL * BlendTreeFloatConditionFunction)(float x, float y);

        EFunction                       mFunctionEnum;
        BlendTreeFloatConditionFunction mFunction;

        BlendTreeFloatConditionNode(AnimGraph* animGraph);
        ~BlendTreeFloatConditionNode();

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void OnUpdateAttributes() override;

        static bool MCORE_CDECL FloatConditionEqual(float x, float y);
        static bool MCORE_CDECL FloatConditionNotEqual(float x, float y);
        static bool MCORE_CDECL FloatConditionGreater(float x, float y);
        static bool MCORE_CDECL FloatConditionLess(float x, float y);
        static bool MCORE_CDECL FloatConditionGreaterOrEqual(float x, float y);
        static bool MCORE_CDECL FloatConditionLessOrEqual(float x, float y);
    };
}   // namespace EMotionFX
