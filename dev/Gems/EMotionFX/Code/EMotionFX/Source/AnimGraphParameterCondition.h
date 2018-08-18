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

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>


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
    public:
        AZ_RTTI(AnimGraphParameterCondition, "{458D0D08-3F1E-4116-89FC-50F447EDC84E}", AnimGraphTransitionCondition)
        AZ_CLASS_ALLOCATOR_DECL

        enum EFunction : AZ::u8
        {
            FUNCTION_GREATER        = 0,
            FUNCTION_GREATEREQUAL   = 1,
            FUNCTION_LESS           = 2,
            FUNCTION_LESSEQUAL      = 3,
            FUNCTION_NOTEQUAL       = 4,
            FUNCTION_EQUAL          = 5,
            FUNCTION_INRANGE        = 6,
            FUNCTION_NOTINRANGE     = 7
        };

        enum EStringFunction : AZ::u8
        {
            STRINGFUNCTION_EQUAL_CASESENSITIVE      = 0,
            STRINGFUNCTION_NOTEQUAL_CASESENSITIVE   = 1
        };

        AnimGraphParameterCondition();
        AnimGraphParameterCondition(AnimGraph* animGraph);
        ~AnimGraphParameterCondition();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;

        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;

        // float
        void SetFunction(EFunction func);
        EFunction GetFunction() const;
        static const char* GetTestFunctionString(EFunction function);
        const char* GetTestFunctionString() const;
        void SetTestValue(float testValue);
        float GetTestValue() const;
        void SetRangeValue(float rangeValue);
        float GetRangeValue() const;

        // string
        void SetStringFunction(EStringFunction func);
        EStringFunction GetStringFunction() const;
        const char* GetStringTestFunctionString() const;
        void SetTestString(const AZStd::string& testString);
        const AZStd::string& GetTestString() const;

        void SetParameterName(const AZStd::string& parameterName);
        const AZStd::string& GetParameterName() const;
        AZ::TypeId GetParameterType() const;
        bool IsFloatParameter() const;

        static void Reflect(AZ::ReflectContext* context);

    private:
        // test function types
        typedef bool (MCORE_CDECL * BlendConditionParamValueFunction)(float paramValue, float testValue, float rangeValue);

        // float test functions
        static bool MCORE_CDECL TestGreater(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestGreaterEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestLess(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestLessEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestNotEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestInRange(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestNotInRange(float paramValue, float testValue, float rangeValue);

        AZ::Crc32 GetStringParameterOptionsVisibility() const;
        AZ::Crc32 GetFloatParameterOptionsVisibility() const;
        AZ::Crc32 GetRangeValueVisibility() const;

        static const char* s_stringFunctionEqual;
        static const char* s_stringFunctionNotEqual;

        static const char* s_functionGreater;
        static const char* s_functionGreaterEqual;
        static const char* s_functionLess;
        static const char* s_functionLessEqual;
        static const char* s_functionNotEqual;
        static const char* s_functionEqual;
        static const char* s_functionInRange;
        static const char* s_functionNotInRange;

        AZStd::string                       m_parameterName;
        AZStd::string                       m_testString;
        AZ::Outcome<size_t>                 m_parameterIndex;
        BlendConditionParamValueFunction    m_testFunction;
        EStringFunction                     m_stringFunction;
        EFunction                           m_function;
        float                               m_testValue;
        float                               m_rangeValue;
    };
} // namespace EMotionFX


namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::AnimGraphParameterCondition::EFunction, "{24886681-0CD8-49F4-BBC8-5EB22A18D9AE}");
} // namespace AZ