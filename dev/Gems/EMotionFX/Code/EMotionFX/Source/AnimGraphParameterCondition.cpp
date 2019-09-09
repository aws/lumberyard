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

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphParameterCondition.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/IntParameter.h>
#include <EMotionFX/Source/Parameter/FloatParameter.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/FloatSpinnerParameter.h>
#include <EMotionFX/Source/Parameter/StringParameter.h>
#include <MCore/Source/Compare.h>


namespace EMotionFX
{
    const char* AnimGraphParameterCondition::s_stringFunctionEqual = "Is Equal (case-sensitive)";
    const char* AnimGraphParameterCondition::s_stringFunctionNotEqual = "Is Not Equal (case-sensitive)";

    const char* AnimGraphParameterCondition::s_functionGreater = "param > testValue";
    const char* AnimGraphParameterCondition::s_functionGreaterEqual = "param >= testValue";
    const char* AnimGraphParameterCondition::s_functionLess = "param < testValue";
    const char* AnimGraphParameterCondition::s_functionLessEqual = "param <= testValue";
    const char* AnimGraphParameterCondition::s_functionNotEqual = "param != testValue";
    const char* AnimGraphParameterCondition::s_functionEqual = "param == testValue";
    const char* AnimGraphParameterCondition::s_functionInRange = "param INRANGE [testValue..rangeValue]";
    const char* AnimGraphParameterCondition::s_functionNotInRange = "param NOT INRANGE [testValue..rangeValue]";

    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphParameterCondition, AnimGraphAllocator, 0)

    AnimGraphParameterCondition::AnimGraphParameterCondition()
        : AnimGraphTransitionCondition()
        , m_parameterIndex(AZ::Failure())
        , m_testFunction(TestGreater)
        , m_stringFunction(STRINGFUNCTION_EQUAL_CASESENSITIVE)
        , m_function(FUNCTION_GREATER)
        , m_testValue(0.0f)
        , m_rangeValue(0.0f)
    {
    }


    AnimGraphParameterCondition::AnimGraphParameterCondition(AnimGraph* animGraph)
        : AnimGraphParameterCondition()
    {
        InitAfterLoading(animGraph);
    }


    AnimGraphParameterCondition::~AnimGraphParameterCondition()
    {
    }


    void AnimGraphParameterCondition::Reinit()
    {
        // Find the parameter index for the given parameter name, to prevent string based lookups every frame
        m_parameterIndex = mAnimGraph->FindValueParameterIndexByName(m_parameterName);
        SetFunction(m_function);
    }


    bool AnimGraphParameterCondition::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphTransitionCondition::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    // get the palette name
    const char* AnimGraphParameterCondition::GetPaletteName() const
    {
        return "Parameter Condition";
    }

    
    void AnimGraphParameterCondition::SetTestString(const AZStd::string& testString)
    {
        m_testString = testString;
    }

    const AZStd::string& AnimGraphParameterCondition::GetTestString() const
    {
        return m_testString;
    }


    void AnimGraphParameterCondition::SetParameterName(const AZStd::string& parameterName)
    {
        m_parameterName = parameterName;
        if (mAnimGraph)
        {
            Reinit();
        }
    }


    const AZStd::string& AnimGraphParameterCondition::GetParameterName() const
    {
        return m_parameterName;
    }


    AZ::TypeId AnimGraphParameterCondition::GetParameterType() const
    {
        // if there is no parameter selected yet, return invalid type
        if (m_parameterIndex.IsSuccess())
        {
            // get access to the parameter info and return the type of its default value
            const ValueParameter* valueParameter = mAnimGraph->FindValueParameter(m_parameterIndex.GetValue());
            return azrtti_typeid(valueParameter);
        }
        else
        {
            return AZ::TypeId();
        }
    }


    bool AnimGraphParameterCondition::IsFloatParameter() const
    {
        if (!m_parameterIndex.IsSuccess())
        {
            return false;
        }

        const ValueParameter* valueParameter = mAnimGraph->FindValueParameter(m_parameterIndex.GetValue());
        if (!valueParameter)
        {
            return false;
        }

        if (azrtti_istypeof<const BoolParameter*>(valueParameter) ||
            azrtti_istypeof<const IntParameter*>(valueParameter) ||
            azrtti_istypeof<const FloatParameter*>(valueParameter))
        {
            return true;
        }

        return false;
    }


    // test the condition
    bool AnimGraphParameterCondition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        // allow the transition in case we don't have a valid parameter to test against
        if (!m_parameterIndex.IsSuccess())
        {
            return false; // act like this condition failed
        }
        // string parameter handling
        const AZ::TypeId parameterType = GetParameterType();
        if (parameterType == azrtti_typeid<StringParameter>())
        {
            const AZStd::string& paramValue = static_cast<MCore::AttributeString*>(animGraphInstance->GetParameterValue(static_cast<uint32>(m_parameterIndex.GetValue())))->GetValue();

            if (m_stringFunction == STRINGFUNCTION_EQUAL_CASESENSITIVE)
            {
                return paramValue == m_testString;
            }
            else
            {
                return paramValue != m_testString;
            }
        }

        // try to convert the parameter value into a float
        float parameterValue;
        if (!animGraphInstance->GetParameterValueAsFloat(static_cast<uint32>(m_parameterIndex.GetValue()), &parameterValue))
        {
            return false;
        }

        // now apply the function
        return m_testFunction(parameterValue, m_testValue, m_rangeValue);
    }


    void AnimGraphParameterCondition::SetFunction(EFunction func)
    {
        m_function = func;

        switch (m_function)
        {
            case FUNCTION_GREATER:
                m_testFunction = TestGreater;
                return;
            case FUNCTION_GREATEREQUAL:
                m_testFunction = TestGreaterEqual;
                return;
            case FUNCTION_LESS:
                m_testFunction = TestLess;
                return;
            case FUNCTION_LESSEQUAL:
                m_testFunction = TestLessEqual;
                return;
            case FUNCTION_NOTEQUAL:
                m_testFunction = TestNotEqual;
                return;
            case FUNCTION_EQUAL:
                m_testFunction = TestEqual;
                return;
            case FUNCTION_INRANGE:
                m_testFunction = TestInRange;
                return;
            case FUNCTION_NOTINRANGE:
                m_testFunction = TestNotInRange;
                return;
            default:
                MCORE_ASSERT(false);        // function unknown
        }
    }


    AnimGraphParameterCondition::EFunction AnimGraphParameterCondition::GetFunction() const
    {
        return m_function;
    }


    void AnimGraphParameterCondition::SetTestValue(float testValue)
    {
        m_testValue = testValue;
    }


    float AnimGraphParameterCondition::GetTestValue() const
    {
        return m_testValue;
    }


    void AnimGraphParameterCondition::SetRangeValue(float rangeValue)
    {
        m_rangeValue = rangeValue;
    }


    float AnimGraphParameterCondition::GetRangeValue() const
    {
        return m_rangeValue;
    }


    void AnimGraphParameterCondition::SetStringFunction(EStringFunction func)
    {
        m_stringFunction = func;
    }


    AnimGraphParameterCondition::EStringFunction AnimGraphParameterCondition::GetStringFunction() const
    {
        return m_stringFunction;
    }


    const char* AnimGraphParameterCondition::GetTestFunctionString(EFunction function)
    {
        switch (function)
        {
            case FUNCTION_GREATER:
            {
                return s_functionGreater;
            }
            case FUNCTION_GREATEREQUAL:
            {
                return s_functionGreaterEqual;
            }
            case FUNCTION_LESS:
            {
                return s_functionLess;
            }
            case FUNCTION_LESSEQUAL:
            {
                return s_functionLessEqual;
            }
            case FUNCTION_NOTEQUAL:
            {
                return s_functionNotEqual;
            }
            case FUNCTION_EQUAL:
            {
                return s_functionEqual;
            }
            case FUNCTION_INRANGE:
            {
                return s_functionInRange;
            }
            case FUNCTION_NOTINRANGE:
            {
                return s_functionNotInRange;
            }
            default:
            {
                return "Unknown operation";
            }
        }
    }


    const char* AnimGraphParameterCondition::GetTestFunctionString() const
    {
        return GetTestFunctionString(m_function);
    }


    const char* AnimGraphParameterCondition::GetStringTestFunctionString() const
    {
        if (m_stringFunction == STRINGFUNCTION_EQUAL_CASESENSITIVE)
        {
            return "Is Equal (case-sensitive)";
        }

        return "Is Not Equal (case-sensitive)";
    }


    // construct and output the information summary string for this object
    void AnimGraphParameterCondition::GetSummary(AZStd::string* outResult) const
    {
        *outResult = AZStd::string::format("%s: Parameter Name='%s', Test Function='%s', Test Value=%.2f, String Test Function='%s', String Test Value='%s'", RTTI_GetTypeName(), m_parameterName.c_str(), GetTestFunctionString(), m_testValue, GetStringTestFunctionString(), m_testString.c_str());
    }


    // construct and output the tooltip for this object
    void AnimGraphParameterCondition::GetTooltip(AZStd::string* outResult) const
    {
        AZStd::string columnName, columnValue;

        // add the condition type
        columnName = "Condition Type: ";
        columnValue = RTTI_GetTypeName();
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"120\"><b>%s</b></td><td><nobr>%s</nobr></td>", columnName.c_str(), columnValue.c_str());

        // add the parameter
        columnName = "Parameter Name: ";
        *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), m_parameterName.c_str());

        if (GetParameterType() == azrtti_typeid<StringParameter>())
        {
            // add the test string
            columnName = "Test String: ";
            *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), m_testString.c_str());

            // add the string test function
            columnName = "String Test Function: ";
            columnValue = GetStringTestFunctionString();
            *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td></tr></table>", columnName.c_str(), columnValue.c_str());
        }
        else
        {
            // add the test value
            columnName = "Test Value: ";
            columnValue = AZStd::string::format("%.3f", m_testValue);
            *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), columnValue.c_str());

            // add the range value
            columnName = "Range Value: ";
            columnValue = AZStd::string::format("%.3f", m_rangeValue);
            *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), columnValue.c_str());

            // add the test function
            columnName = "Test Function: ";
            columnValue = GetTestFunctionString();
            *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td></tr>", columnName.c_str(), columnValue.c_str());
        }
    }


    //------------------------------------------------------------------------------------------
    // Test Functions
    //------------------------------------------------------------------------------------------
    bool AnimGraphParameterCondition::TestGreater(float paramValue, float testValue, float rangeValue)             { MCORE_UNUSED(rangeValue); return (paramValue > testValue);    }
    bool AnimGraphParameterCondition::TestGreaterEqual(float paramValue, float testValue, float rangeValue)        { MCORE_UNUSED(rangeValue); return (paramValue >= testValue);   }
    bool AnimGraphParameterCondition::TestLess(float paramValue, float testValue, float rangeValue)                { MCORE_UNUSED(rangeValue); return (paramValue < testValue);    }
    bool AnimGraphParameterCondition::TestLessEqual(float paramValue, float testValue, float rangeValue)           { MCORE_UNUSED(rangeValue); return (paramValue <= testValue); }
    bool AnimGraphParameterCondition::TestEqual(float paramValue, float testValue, float rangeValue)               { MCORE_UNUSED(rangeValue); return MCore::Compare<float>::CheckIfIsClose(paramValue, testValue, MCore::Math::epsilon); }
    bool AnimGraphParameterCondition::TestNotEqual(float paramValue, float testValue, float rangeValue)            { MCORE_UNUSED(rangeValue); return (MCore::Compare<float>::CheckIfIsClose(paramValue, testValue, MCore::Math::epsilon) == false); }
    bool AnimGraphParameterCondition::TestInRange(float paramValue, float testValue, float rangeValue)
    {
        if (testValue <= rangeValue)
        {
            return (MCore::InRange<float>(paramValue, testValue, rangeValue));
        }
        else
        {
            return (MCore::InRange<float>(paramValue, rangeValue, testValue));
        }
    }


    AZ::Crc32 AnimGraphParameterCondition::GetStringParameterOptionsVisibility() const
    {
        if (GetParameterType() == azrtti_typeid<StringParameter>())
        {
            return AZ::Edit::PropertyVisibility::Show;
        }

        return AZ::Edit::PropertyVisibility::Hide;
    }


    AZ::Crc32 AnimGraphParameterCondition::GetFloatParameterOptionsVisibility() const
    {
        if (IsFloatParameter())
        {
            return AZ::Edit::PropertyVisibility::Show;
        }

        return AZ::Edit::PropertyVisibility::Hide;
    }


    AZ::Crc32 AnimGraphParameterCondition::GetRangeValueVisibility() const
    {
        if (m_function == AnimGraphParameterCondition::FUNCTION_INRANGE || m_function == AnimGraphParameterCondition::FUNCTION_NOTINRANGE)
        {
            return GetFloatParameterOptionsVisibility();
        }

        return AZ::Edit::PropertyVisibility::Hide;
    }


    bool AnimGraphParameterCondition::TestNotInRange(float paramValue, float testValue, float rangeValue)
    {
        if (testValue <= rangeValue)
        {
            return (MCore::InRange<float>(paramValue, testValue, rangeValue) == false);
        }
        else
        {
            return (MCore::InRange<float>(paramValue, rangeValue, testValue) == false);
        }
    }

    AZStd::vector<AZStd::string> AnimGraphParameterCondition::GetParameters() const
    {
        AZStd::vector<AZStd::string> parameters;
        if (!m_parameterName.empty())
        {
            parameters.emplace_back(m_parameterName);
        }
        return parameters;
    }

    AnimGraph* AnimGraphParameterCondition::GetParameterAnimGraph() const
    {
        return GetAnimGraph();
    }

    void AnimGraphParameterCondition::ParameterMaskChanged(const AZStd::vector<AZStd::string>& newParameterMask)
    {
        if (!newParameterMask.empty())
        {
            m_parameterName = *newParameterMask.begin();
            m_parameterIndex = mAnimGraph->FindValueParameterIndexByName(m_parameterName);
        }
    }

    void AnimGraphParameterCondition::AddRequiredParameters(AZStd::vector<AZStd::string>& parameterNames) const
    {
        AZ_UNUSED(parameterNames);
        // The parameter is replaceable
    }

    void AnimGraphParameterCondition::ParameterAdded(size_t newParameterIndex)
    {
        // Just recompute the index in the case the new parameter was inserted before ours
        m_parameterIndex = mAnimGraph->FindValueParameterIndexByName(m_parameterName);
    }

    void AnimGraphParameterCondition::ParameterRenamed(const AZStd::string& oldParameterName, const AZStd::string& newParameterName)
    {
        if (GetParameterName() == oldParameterName)
        {
            SetParameterName(newParameterName);
        }
    }

    void AnimGraphParameterCondition::ParameterOrderChanged(const ValueParameterVector& beforeChange, const ValueParameterVector& afterChange)
    {
        // Just recompute the index
        m_parameterIndex = mAnimGraph->FindValueParameterIndexByName(m_parameterName);
    }

    void AnimGraphParameterCondition::ParameterRemoved(const AZStd::string& oldParameterName)
    {
        if (oldParameterName == m_parameterName)
        {
            m_parameterName.clear();
            m_parameterIndex = AZ::Failure();
        }
    }

    void AnimGraphParameterCondition::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphParameterCondition, AnimGraphTransitionCondition>()
            ->Version(1)
            ->Field("parameterName", &AnimGraphParameterCondition::m_parameterName)
            ->Field("function", &AnimGraphParameterCondition::m_function)
            ->Field("testValue", &AnimGraphParameterCondition::m_testValue)
            ->Field("rangeValue", &AnimGraphParameterCondition::m_rangeValue)
            ->Field("stringFunction", &AnimGraphParameterCondition::m_stringFunction)
            ->Field("testString", &AnimGraphParameterCondition::m_testString)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Enum<EFunction>("Test Function", "The type of test function or condition.")
            ->Value(s_functionGreater, FUNCTION_GREATER)
            ->Value(s_functionGreaterEqual, FUNCTION_GREATEREQUAL)
            ->Value(s_functionLess, FUNCTION_LESS)
            ->Value(s_functionLessEqual, FUNCTION_LESSEQUAL)
            ->Value(s_functionNotEqual, FUNCTION_NOTEQUAL)
            ->Value(s_functionEqual, FUNCTION_EQUAL)
            ->Value(s_functionInRange, FUNCTION_INRANGE)
            ->Value(s_functionNotInRange, FUNCTION_NOTINRANGE)
            ;

        editContext->Class<AnimGraphParameterCondition>("Parameter Condition", "Parameter condition attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC("AnimGraphParameter", 0x778af55a), &AnimGraphParameterCondition::m_parameterName, "Parameter", "The parameter name to apply the condition on.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphParameterCondition::Reinit)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->Attribute(AZ_CRC("AnimGraph", 0x0d53d4b3), &AnimGraphParameterCondition::GetAnimGraph)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphParameterCondition::m_function)
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphParameterCondition::GetFloatParameterOptionsVisibility)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphParameterCondition::Reinit)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphParameterCondition::m_testValue, "Test Value", "The float value to test against the parameter value.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphParameterCondition::GetFloatParameterOptionsVisibility)
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphParameterCondition::m_rangeValue, "Range Value", "The range high or low bound value, only used when the function is set to 'In Range' or 'Not in Range'.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphParameterCondition::GetRangeValueVisibility)
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphParameterCondition::m_stringFunction, "String Test Function", "The type of the string comparison operation.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphParameterCondition::GetStringParameterOptionsVisibility)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphParameterCondition::Reinit)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->EnumAttribute(STRINGFUNCTION_EQUAL_CASESENSITIVE,s_stringFunctionEqual)
                ->EnumAttribute(STRINGFUNCTION_NOTEQUAL_CASESENSITIVE,  s_stringFunctionNotEqual)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphParameterCondition::m_testString, "Test String", "The string to test against the parameter value.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphParameterCondition::GetStringParameterOptionsVisibility)
            ;
    }
} // namespace EMotionFX