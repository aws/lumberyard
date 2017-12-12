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
    class EMFX_API BlendTreeFloatMath2Node
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeFloatMath2Node, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeFloatMath2Node, "{9F5FA0EE-B6EE-420C-9015-26E5F17AAA3E}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000009
        };

        //
        enum
        {
            INPUTPORT_X         = 0,
            INPUTPORT_Y         = 1,
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_INPUT_X          = 0,
            PORTID_INPUT_Y          = 1,
            PORTID_OUTPUT_RESULT    = 0
        };

        enum
        {
            ATTRIB_MATHFUNCTION = 0,
            ATTRIB_STATICVALUE  = 1
        };

        // available math functions
        enum EMathFunction
        {
            MATHFUNCTION_ADD            = 0,
            MATHFUNCTION_SUBTRACT       = 1,
            MATHFUNCTION_MULTIPLY       = 2,
            MATHFUNCTION_DIVIDE         = 3,
            MATHFUNCTION_AVERAGE        = 4,
            MATHFUNCTION_RANDOMFLOAT    = 5,
            MATHFUNCTION_MOD            = 6,
            MATHFUNCTION_MIN            = 7,
            MATHFUNCTION_MAX            = 8,
            MATHFUNCTION_POW            = 9,
            MATHFUNCTION_NUMFUNCTIONS
        };

        static BlendTreeFloatMath2Node* Create(AnimGraph* animGraph);

        void RegisterPorts() override;
        void RegisterAttributes() override;

        void SetMathFunction(EMathFunction func);
        void OnUpdateAttributes() override;

        uint32 GetVisualColor() const override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

    private:
        typedef float (MCORE_CDECL * BlendTreeMath2Function)(float x, float y);

        EMathFunction           mMathFunction;
        BlendTreeMath2Function  mCalculateFunc;

        BlendTreeFloatMath2Node(AnimGraph* animGraph);
        ~BlendTreeFloatMath2Node();

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        static float MCORE_CDECL CalculateAdd(float x, float y);
        static float MCORE_CDECL CalculateSubtract(float x, float y);
        static float MCORE_CDECL CalculateMultiply(float x, float y);
        static float MCORE_CDECL CalculateDivide(float x, float y);
        static float MCORE_CDECL CalculateAverage(float x, float y);
        static float MCORE_CDECL CalculateRandomFloat(float x, float y);
        static float MCORE_CDECL CalculateMod(float x, float y);
        static float MCORE_CDECL CalculateMin(float x, float y);
        static float MCORE_CDECL CalculateMax(float x, float y);
        static float MCORE_CDECL CalculatePow(float x, float y);
    };
}   // namespace EMotionFX
