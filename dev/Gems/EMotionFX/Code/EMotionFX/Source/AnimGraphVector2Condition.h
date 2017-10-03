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
#include "AnimGraphTransitionCondition.h"


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;

    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphVector2Condition
        : public AnimGraphTransitionCondition
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphVector2Condition, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS);

    public:
        AZ_RTTI(AnimGraphVector2Condition, "{605DF8B0-C39A-4BB4-B1A9-ABAF528E0739}", AnimGraphTransitionCondition);

        enum
        {
            TYPE_ID = 0x00002123
        };

        enum
        {
            ATTRIB_PARAMETER            = 0,
            ATTRIB_OPERATION            = 1,
            ATTRIB_FUNCTION             = 2,
            ATTRIB_TESTVALUE            = 3,
            ATTRIB_RANGEVALUE           = 4,
        };

        enum EOperation
        {
            OPERATION_LENGTH            = 0,
            OPERATION_GETX              = 1,
            OPERATION_GETY              = 2,
            OPERATION_NUMOPERATIONS
        };

        enum EFunction
        {
            FUNCTION_GREATER        = 0,
            FUNCTION_GREATEREQUAL   = 1,
            FUNCTION_LESS           = 2,
            FUNCTION_LESSEQUAL      = 3,
            FUNCTION_NOTEQUAL       = 4,
            FUNCTION_EQUAL          = 5,
            FUNCTION_INRANGE        = 6,
            FUNCTION_NOTINRANGE     = 7,
            FUNCTION_NUMFUNCTIONS
        };

        static AnimGraphVector2Condition* Create(AnimGraph* animGraph);

        void RegisterAttributes() override;

        const char* GetTypeString() const override;
        void GetSummary(MCore::String* outResult) const override;
        void GetTooltip(MCore::String* outResult) const override;
        const char* GetPaletteName() const override;

        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

        void SetFunction(EFunction func);
        const char* GetTestFunctionString() const;

        void SetOperation(EOperation operation);
        const char* GetOperationString() const;

        uint32 GetParameterType() const;

    private:
        // test function types
        typedef bool (MCORE_CDECL * BlendConditionParamValueFunction)(float paramValue, float testValue, float rangeValue);
        typedef float (MCORE_CDECL * BlendConditionOperationFunction)(const AZ::Vector2& vec);

        uint32                                      mParameterIndex;

        EFunction                                   mFunction;
        BlendConditionParamValueFunction            mTestFunction;

        EOperation                                  mOperation;
        BlendConditionOperationFunction             mOperationFunction;

        // float test functions
        static bool MCORE_CDECL TestGreater(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestGreaterEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestLess(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestLessEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestNotEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestInRange(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestNotInRange(float paramValue, float testValue, float rangeValue);

        // operation functions
        static float MCORE_CDECL OperationLength(const AZ::Vector2& vec);
        static float MCORE_CDECL OperationGetX(const AZ::Vector2& vec);
        static float MCORE_CDECL OperationGetY(const AZ::Vector2& vec);

        AnimGraphVector2Condition(AnimGraph* animGraph);
        ~AnimGraphVector2Condition();

        void OnUpdateAttributes() override;
    };
}   // namespace EMotionFX

