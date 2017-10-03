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
    class EMFX_API BlendTreeVector3Math2Node
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeVector3Math2Node, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeVector3Math2Node, "{30568371-DEDE-47CC-95C2-7EB7A353264B}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000021
        };

        //
        enum
        {
            INPUTPORT_X                 = 0,
            INPUTPORT_Y                 = 1,
            OUTPUTPORT_RESULT_VECTOR3   = 0,
            OUTPUTPORT_RESULT_FLOAT     = 1
        };

        enum
        {
            PORTID_INPUT_X              = 0,
            PORTID_INPUT_Y              = 1,
            PORTID_OUTPUT_VECTOR3       = 0,
            PORTID_OUTPUT_FLOAT         = 1
        };

        // available math functions
        enum EMathFunction
        {
            MATHFUNCTION_DOT            = 0,
            MATHFUNCTION_CROSS          = 1,
            MATHFUNCTION_ADD            = 2,
            MATHFUNCTION_SUBTRACT       = 3,
            MATHFUNCTION_MULTIPLY       = 4,
            MATHFUNCTION_DIVIDE         = 5,
            MATHFUNCTION_ANGLEDEGREES   = 6,
            MATHFUNCTION_NUMFUNCTIONS
        };

        enum
        {
            ATTRIB_MATHFUNCTION         = 0,
            ATTRIB_STATICVECTOR         = 1
        };

        static BlendTreeVector3Math2Node* Create(AnimGraph* animGraph);

        void RegisterPorts() override;
        void RegisterAttributes() override;
        void SetMathFunction(EMathFunction func);

        uint32 GetVisualColor() const override      { return MCore::RGBA(128, 255, 255); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

    private:
        typedef void (MCORE_CDECL * BlendTreeVec3Math1Function)(const MCore::Vector3& inputX, const MCore::Vector3& inputY, MCore::Vector3* vectorOutput, float* floatOutput);

        EMathFunction               mMathFunction;
        BlendTreeVec3Math1Function  mCalculateFunc;

        BlendTreeVector3Math2Node(AnimGraph* animGraph);
        ~BlendTreeVector3Math2Node();

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void OnUpdateAttributes() override;

        static void MCORE_CDECL CalculateDot(const MCore::Vector3& inputX, const MCore::Vector3& inputY, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateCross(const MCore::Vector3& inputX, const MCore::Vector3& inputY, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateAdd(const MCore::Vector3& inputX, const MCore::Vector3& inputY, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateSubtract(const MCore::Vector3& inputX, const MCore::Vector3& inputY, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateMultiply(const MCore::Vector3& inputX, const MCore::Vector3& inputY, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateDivide(const MCore::Vector3& inputX, const MCore::Vector3& inputY, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateAngleDegrees(const MCore::Vector3& inputX, const MCore::Vector3& inputY, MCore::Vector3* vectorOutput, float* floatOutput);
    };
}   // namespace EMotionFX
