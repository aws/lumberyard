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
    class EMFX_API AnimGraphParameterCondition
        : public AnimGraphTransitionCondition
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphParameterCondition, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS);

    public:
        AZ_RTTI(AnimGraphParameterCondition, "{458D0D08-3F1E-4116-89FC-50F447EDC84E}", AnimGraphTransitionCondition);

        enum
        {
            TYPE_ID = 0x00002000
        };

        enum
        {
            ATTRIB_PARAMETER            = 0,
            ATTRIB_TESTVALUE            = 1,
            ATTRIB_RANGEVALUE           = 2,
            ATTRIB_FUNCTION             = 3,
            ATTRIB_TESTSTRING           = 4,
            ATTRIB_STRINGTESTFUNCTION   = 5
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

        enum EStringFunction
        {
            STRINGFUNCTION_EQUAL_CASESENSITIVE      = 0,
            //STRINGFUNCTION_EQUAL_CASEINSENSITIVE  = 1,
            STRINGFUNCTION_NOTEQUAL_CASESENSITIVE   = 1,
            //STRINGFUNCTION_NOTEQUAL_CASEINSENSITIVE= 3,
            STRINGFUNCTION_NUMFUNCTIONS             = 2
        };

        static AnimGraphParameterCondition* Create(AnimGraph* animGraph);

        void RegisterAttributes() override;
        void OnUpdateAttributes() override;

        const char* GetTypeString() const override;
        void GetSummary(MCore::String* outResult) const override;
        void GetTooltip(MCore::String* outResult) const override;
        const char* GetPaletteName() const override;

        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

        // float
        void SetFunction(EFunction func);
        const char* GetTestFunctionString() const;

        // string
        void SetStringFunction(EStringFunction func);
        const char* GetStringTestFunctionString() const;

        uint32 GetParameterType() const;

    private:
        // test function types
        typedef bool (MCORE_CDECL * BlendConditionParamValueFunction)(float paramValue, float testValue, float rangeValue);
        //typedef bool (MCORE_CDECL *BlendConditionStringParamValueFunction)(const MCore::String& paramValue, const char* testValue);

        uint32                                      mParameterIndex;

        // float
        EFunction                                   mFunction;
        BlendConditionParamValueFunction            mTestFunction;

        // string
        EStringFunction                             mStringFunction;
        //BlendConditionStringParamValueFunction    mStringTestFunction;

        // float test functions
        static bool MCORE_CDECL TestGreater(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestGreaterEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestLess(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestLessEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestNotEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestInRange(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestNotInRange(float paramValue, float testValue, float rangeValue);

        // string test functions
        //static bool MCORE_CDECL StringTestEqualCaseSensitive(const MCore::String& paramValue, const char* testValue);
        //static bool MCORE_CDECL StringTestEqualCaseInsensitive(const MCore::String& paramValue, const char* testValue);
        //static bool MCORE_CDECL StringTestNotEqualCaseSensitive(const MCore::String& paramValue, const char* testValue);
        //static bool MCORE_CDECL StringTestNotEqualCaseInsensitive(const MCore::String& paramValue, const char* testValue);

        AnimGraphParameterCondition(AnimGraph* animGraph);
        ~AnimGraphParameterCondition();
    };
}   // namespace EMotionFX
