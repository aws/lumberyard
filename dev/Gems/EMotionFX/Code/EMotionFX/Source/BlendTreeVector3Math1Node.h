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
    class EMFX_API BlendTreeVector3Math1Node
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeVector3Math1Node, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeVector3Math1Node, "{79488BAA-7151-4B49-B4EB-0FCA268EF44F}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000020
        };

        //
        enum
        {
            INPUTPORT_X                 = 0,
            OUTPUTPORT_RESULT_VECTOR3   = 0,
            OUTPUTPORT_RESULT_FLOAT     = 1
        };

        enum
        {
            PORTID_INPUT_X              = 0,
            PORTID_OUTPUT_VECTOR3       = 0,
            PORTID_OUTPUT_FLOAT         = 1
        };

        // available math functions
        enum EMathFunction
        {
            MATHFUNCTION_LENGTH         = 0,
            MATHFUNCTION_SQUARELENGTH   = 1,
            MATHFUNCTION_NORMALIZE      = 2,
            MATHFUNCTION_ZERO           = 3,
            MATHFUNCTION_FLOOR          = 4,
            MATHFUNCTION_CEIL           = 5,
            MATHFUNCTION_ABS            = 6,
            MATHFUNCTION_RANDOM         = 7,
            MATHFUNCTION_RANDOMNEG      = 8,
            MATHFUNCTION_RANDOMDIRVEC   = 9,
            MATHFUNCTION_NEGATE         = 10,
            MATHFUNCTION_NUMFUNCTIONS
        };

        static BlendTreeVector3Math1Node* Create(AnimGraph* animGraph);

        void RegisterPorts() override;
        void RegisterAttributes() override;

        void SetMathFunction(EMathFunction func);

        uint32 GetVisualColor() const override  { return MCore::RGBA(128, 255, 255); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

    private:
        typedef void (MCORE_CDECL * BlendTreeVec3Math1Function)(const MCore::Vector3& input, MCore::Vector3* vectorOutput, float* floatOutput);

        EMathFunction               mMathFunction;
        BlendTreeVec3Math1Function  mCalculateFunc;

        BlendTreeVector3Math1Node(AnimGraph* animGraph);
        ~BlendTreeVector3Math1Node();

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void OnUpdateAttributes() override;

        static void MCORE_CDECL CalculateLength(const MCore::Vector3& input, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateSquareLength(const MCore::Vector3& input, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateNormalize(const MCore::Vector3& input, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateZero(const MCore::Vector3& input, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateAbs(const MCore::Vector3& input, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateFloor(const MCore::Vector3& input, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateCeil(const MCore::Vector3& input, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateRandomVector(const MCore::Vector3& input, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateRandomVectorNeg(const MCore::Vector3& input, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateRandomVectorDir(const MCore::Vector3& input, MCore::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateNegate(const MCore::Vector3& input, MCore::Vector3* vectorOutput, float* floatOutput);
    };
}   // namespace EMotionFX
