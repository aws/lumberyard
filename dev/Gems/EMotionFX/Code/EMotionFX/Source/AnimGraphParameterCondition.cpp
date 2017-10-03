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

// include the required headers
#include "EMotionFXConfig.h"
#include <MCore/Source/Compare.h>
#include <MCore/Source/UnicodeString.h>
#include <MCore/Source/AttributeSettings.h>
#include "AnimGraphParameterCondition.h"
#include "AnimGraph.h"
#include "AnimGraphInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "EMotionFXManager.h"
#include "AnimGraphManager.h"


namespace EMotionFX
{
    // constructor
    AnimGraphParameterCondition::AnimGraphParameterCondition(AnimGraph* animGraph)
        : AnimGraphTransitionCondition(animGraph, TYPE_ID)
    {
        mParameterIndex = MCORE_INVALIDINDEX32;

        // float
        mTestFunction       = TestGreater;
        mFunction           = FUNCTION_GREATER;

        // string
        //mStringTestFunction   = StringTestEqualCaseInsensitive;
        mStringFunction     = STRINGFUNCTION_EQUAL_CASESENSITIVE;

        CreateAttributeValues();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    AnimGraphParameterCondition::~AnimGraphParameterCondition()
    {
    }


    // create
    AnimGraphParameterCondition* AnimGraphParameterCondition::Create(AnimGraph* animGraph)
    {
        return new AnimGraphParameterCondition(animGraph);
    }


    // create unique data
    AnimGraphObjectData* AnimGraphParameterCondition::CreateObjectData()
    {
        return AnimGraphObjectData::Create(this, nullptr);
    }


    // register the attributes
    void AnimGraphParameterCondition::RegisterAttributes()
    {
        MCore::AttributeSettings* attribInfo;

        // register the source motion file name
        attribInfo = RegisterAttribute("Parameter", "parameter", "The parameter name to apply the condition on.", ATTRIBUTE_INTERFACETYPE_PARAMETERPICKER);
        attribInfo->SetDefaultValue(MCore::AttributeString::Create());

        // create the test value float spinner
        attribInfo = RegisterAttribute("Test Value", "testValue", "The float value to test against the parameter value.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        attribInfo->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        attribInfo->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        attribInfo->SetMaxValue(MCore::AttributeFloat::Create(FLT_MAX));

        // create the low range value
        attribInfo = RegisterAttribute("Range Value", "rangeValue", "The range high or low bound value, only used when the function is set to 'In Range' or 'Not in Range'.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        attribInfo->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        attribInfo->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        attribInfo->SetMaxValue(MCore::AttributeFloat::Create(FLT_MAX));

        // create the function combobox
        attribInfo = RegisterAttribute("Test Function", "testFunction", "The type of test function or condition.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        attribInfo->ResizeComboValues(FUNCTION_NUMFUNCTIONS);
        attribInfo->SetComboValue(FUNCTION_GREATER,     "param > testValue");
        attribInfo->SetComboValue(FUNCTION_GREATEREQUAL, "param >= testValue");
        attribInfo->SetComboValue(FUNCTION_LESS,        "param < testValue");
        attribInfo->SetComboValue(FUNCTION_LESSEQUAL,   "param <= testValue");
        attribInfo->SetComboValue(FUNCTION_NOTEQUAL,    "param != testValue");
        attribInfo->SetComboValue(FUNCTION_EQUAL,       "param == testValue");
        attribInfo->SetComboValue(FUNCTION_INRANGE,     "param INRANGE [testValue..rangeValue]");
        attribInfo->SetComboValue(FUNCTION_NOTINRANGE,  "param NOT INRANGE [testValue..rangeValue]");
        attribInfo->SetDefaultValue(MCore::AttributeFloat::Create(static_cast<float>(mFunction)));

        // register the test string
        attribInfo = RegisterAttribute("Test String", "testString", "The string to test against the parameter value.", MCore::ATTRIBUTE_INTERFACETYPE_STRING);
        attribInfo->SetDefaultValue(MCore::AttributeString::Create());

        // create the string function combobox
        attribInfo = RegisterAttribute("String Test Function", "stringTestFunction", "The type of the string comparison operation.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        attribInfo->ResizeComboValues(STRINGFUNCTION_NUMFUNCTIONS);
        attribInfo->SetComboValue(STRINGFUNCTION_EQUAL_CASESENSITIVE, "Is Equal (case-sensitive)");
        attribInfo->SetComboValue(STRINGFUNCTION_NOTEQUAL_CASESENSITIVE, "Is Not Equal (case-sensitive)");
        attribInfo->SetDefaultValue(MCore::AttributeFloat::Create(static_cast<float>(mStringFunction)));
    }


    // get the palette name
    const char* AnimGraphParameterCondition::GetPaletteName() const
    {
        return "Parameter Condition";
    }


    // get the type string
    const char* AnimGraphParameterCondition::GetTypeString() const
    {
        return "AnimGraphParameterCondition";
    }

    
    // get the type of the selected paramter
    uint32 AnimGraphParameterCondition::GetParameterType() const
    {
        // if there is no parameter selected yet, return invalid type
        if (mParameterIndex == MCORE_INVALIDINDEX32)
        {
            return MCORE_INVALIDINDEX32;
        }

        // get access to the parameter info and return the type of its default value
        MCore::AttributeSettings* parameterInfo = mAnimGraph->GetParameter(mParameterIndex);
        return parameterInfo->GetDefaultValue()->GetType();
    }


    // test the condition
    bool AnimGraphParameterCondition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        // allow the transition in case we don't have a valid parameter to test against
        if (mParameterIndex == MCORE_INVALIDINDEX32)
        {
            return false; // act like this condition failed
        }
        // string parameter handling
        const uint32 parameterType = GetParameterType();
        if (parameterType == MCore::AttributeString::TYPE_ID)
        {
            const MCore::String&    paramValue  = static_cast<MCore::AttributeString*>(animGraphInstance->GetParameterValue(mParameterIndex))->GetValue();
            const MCore::String&    testValue   = GetAttributeString(ATTRIB_TESTSTRING)->GetValue();

            // now apply the function
            //return mStringTestFunction( paramValue->GetValue(), testValue.AsChar() );

            if (mStringFunction == STRINGFUNCTION_EQUAL_CASESENSITIVE)
            {
                return paramValue.CheckIfIsEqual(testValue.AsChar());
            }
            else
            {
                return !(paramValue.CheckIfIsEqual(testValue.AsChar()));
            }
        }

        if (parameterType != MCore::AttributeFloat::TYPE_ID)
        {
            return false;
        }

        // try to convert the parameter value into a float
        float paramValue = 0.0f;
        MCore::Attribute* attribute = animGraphInstance->GetParameterValue(mParameterIndex);
        MCORE_ASSERT(attribute->GetType() == MCore::AttributeFloat::TYPE_ID);
        paramValue = static_cast<MCore::AttributeFloat*>(attribute)->GetValue();
        const float testValue   = GetAttributeFloat(ATTRIB_TESTVALUE)->GetValue();
        const float rangeValue  = GetAttributeFloat(ATTRIB_RANGEVALUE)->GetValue();

        // now apply the function
        return mTestFunction(paramValue, testValue, rangeValue);
    }


    // clonse the condition
    AnimGraphObject* AnimGraphParameterCondition::Clone(AnimGraph* animGraph)
    {
        // create the clone
        AnimGraphParameterCondition* clone = new AnimGraphParameterCondition(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // set the math function to use
    void AnimGraphParameterCondition::SetFunction(EFunction func)
    {
        // if it didn't change, don't update anything
        if (func == mFunction)
        {
            return;
        }

        mFunction = func;
        switch (mFunction)
        {
        case FUNCTION_GREATER:
            mTestFunction = TestGreater;
            return;
        case FUNCTION_GREATEREQUAL:
            mTestFunction = TestGreaterEqual;
            return;
        case FUNCTION_LESS:
            mTestFunction = TestLess;
            return;
        case FUNCTION_LESSEQUAL:
            mTestFunction = TestLessEqual;
            return;
        case FUNCTION_NOTEQUAL:
            mTestFunction = TestNotEqual;
            return;
        case FUNCTION_EQUAL:
            mTestFunction = TestEqual;
            return;
        case FUNCTION_INRANGE:
            mTestFunction = TestInRange;
            return;
        case FUNCTION_NOTINRANGE:
            mTestFunction = TestNotInRange;
            return;
        default:
            MCORE_ASSERT(false);        // function unknown
        }
        ;
    }


    // set the string function to use
    void AnimGraphParameterCondition::SetStringFunction(EStringFunction func)
    {
        // if it didn't change, don't update anything
        if (func == mStringFunction)
        {
            return;
        }

        mStringFunction = func;
        /*switch (mStringFunction)
        {
            case STRINGFUNCTION_EQUAL_CASESENSITIVE:        mStringTestFunction = StringTestEqualCaseSensitive;     return;
            case STRINGFUNCTION_EQUAL_CASEINSENSITIVE:      mStringTestFunction = StringTestEqualCaseInsensitive;   return;
            case STRINGFUNCTION_NOTEQUAL_CASESENSITIVE:     mStringTestFunction = StringTestNotEqualCaseSensitive;  return;
            case STRINGFUNCTION_NOTEQUAL_CASEINSENSITIVE:   mStringTestFunction = StringTestNotEqualCaseInsensitive;return;
            default: MCORE_ASSERT(false);   // function unknown
        };*/
    }


    // update the data
    void AnimGraphParameterCondition::OnUpdateAttributes()
    {
        MCORE_ASSERT(mAnimGraph);

        // update the function pointers
        SetFunction((EFunction)((uint32)GetAttributeFloat(ATTRIB_FUNCTION)->GetValue()));
        SetStringFunction((EStringFunction)((uint32)GetAttributeFloat(ATTRIB_STRINGTESTFUNCTION)->GetValue()));

        // get the name of the parameter we want to compare against
        const char* name = GetAttributeString(ATTRIB_PARAMETER)->AsChar();

        // convert this name into an index value, to prevent string based lookups every frame
        mParameterIndex = MCORE_INVALIDINDEX32;
    #ifdef EMFX_EMSTUDIOBUILD
        MCore::AttributeSettings* parameterInfo = nullptr;
    #endif
        if (name)
        {
            const uint32 numParams = mAnimGraph->GetNumParameters();
            for (uint32 i = 0; i < numParams; ++i)
            {
                if (mAnimGraph->GetParameter(i)->GetNameString().CheckIfIsEqual(name))
                {
                #ifdef EMFX_EMSTUDIOBUILD
                    parameterInfo = mAnimGraph->GetParameter(i);
                #endif
                    mParameterIndex = i;
                    break;
                }
            }
        }

        // disable GUI items that have no influence
    #ifdef EMFX_EMSTUDIOBUILD
        // disable all attributes
        EnableAllAttributes(false);

        // always enable the parameter selection attribute
        SetAttributeEnabled(ATTRIB_PARAMETER);

        if (parameterInfo)
        {
            // enable string interface
            if (parameterInfo->GetDefaultValue()->GetType() == MCore::AttributeString::TYPE_ID)
            {
                SetAttributeEnabled(ATTRIB_TESTSTRING);
                SetAttributeEnabled(ATTRIB_STRINGTESTFUNCTION);
            }
            else // enable non-string interface
            if (parameterInfo->GetDefaultValue()->GetType() == MCore::AttributeFloat::TYPE_ID)
            {
                SetAttributeEnabled(ATTRIB_TESTVALUE);
                //SetAttributeEnabled( ATTRIB_RANGEVALUE );
                SetAttributeEnabled(ATTRIB_FUNCTION);

                // disable the range value if we're not using a function that needs the range
                //const int32 function = GetAttributeFloatAsInt32(ATTRIB_FUNCTION);
                if (mFunction == FUNCTION_INRANGE || mFunction == FUNCTION_NOTINRANGE)
                {
                    SetAttributeEnabled(ATTRIB_RANGEVALUE);
                }
            }
        }
    #endif
    }


    // get the name of the currently used test function
    const char* AnimGraphParameterCondition::GetTestFunctionString() const
    {
        // get access to the combo values and the currently selected function
        MCore::AttributeSettings*   comboAttributeInfo  = GetAnimGraphManager().GetAttributeInfo(this, ATTRIB_FUNCTION);
        const int32                 functionIndex       = GetAttributeFloatAsInt32(ATTRIB_FUNCTION);
        return comboAttributeInfo->GetComboValue(functionIndex);
    }


    // get the name of the currently used string test function
    const char* AnimGraphParameterCondition::GetStringTestFunctionString() const
    {
        // get access to the combo values and the currently selected function
        MCore::AttributeSettings*   comboAttributeInfo  = GetAnimGraphManager().GetAttributeInfo(this, ATTRIB_STRINGTESTFUNCTION);
        const int32                 functionIndex       = GetAttributeFloatAsInt32(ATTRIB_STRINGTESTFUNCTION);

        return comboAttributeInfo->GetComboValue(functionIndex);
    }


    // construct and output the information summary string for this object
    void AnimGraphParameterCondition::GetSummary(MCore::String* outResult) const
    {
        outResult->Format("%s: Parameter Name='%s', Test Function='%s', Test Value=%.2f, String Test Function='%s', String Test Value='%s'", GetTypeString(), GetAttributeString(ATTRIB_PARAMETER)->AsChar(), GetTestFunctionString(), GetAttributeFloat(ATTRIB_TESTVALUE)->GetValue(), GetStringTestFunctionString(), GetAttributeString(ATTRIB_TESTSTRING)->AsChar());
    }


    // construct and output the tooltip for this object
    void AnimGraphParameterCondition::GetTooltip(MCore::String* outResult) const
    {
        MCore::String columnName, columnValue;

        // add the condition type
        columnName = "Condition Type: ";
        columnValue = GetTypeString();
        outResult->Format("<table border=\"0\"><tr><td width=\"120\"><b>%s</b></td><td><nobr>%s</nobr></td>", columnName.AsChar(), columnValue.AsChar());

        // add the parameter
        columnName = "Parameter Name: ";
        //columnName.ConvertToNonBreakingHTMLSpaces();
        columnValue = GetAttributeString(ATTRIB_PARAMETER)->AsChar();
        //columnValue.ConvertToNonBreakingHTMLSpaces();
        outResult->FormatAdd("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.AsChar(), columnValue.AsChar());

        if (GetParameterType() == MCore::AttributeString::TYPE_ID)
        {
            // add the test string
            columnName = "Test String: ";
            //columnName.ConvertToNonBreakingHTMLSpaces();
            columnValue = GetAttributeString(ATTRIB_TESTSTRING)->AsChar();
            //columnValue.ConvertToNonBreakingHTMLSpaces();
            outResult->FormatAdd("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.AsChar(), columnValue.AsChar());

            // add the string test function
            columnName = "String Test Function: ";
            //columnName.ConvertToNonBreakingHTMLSpaces();
            columnValue = GetStringTestFunctionString();
            //columnValue.ConvertToNonBreakingHTMLSpaces();
            outResult->FormatAdd("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td></tr></table>", columnName.AsChar(), columnValue.AsChar());
        }
        else
        {
            // add the test value
            columnName = "Test Value: ";
            //columnName.ConvertToNonBreakingHTMLSpaces();
            columnValue.Format("%.3f", GetAttributeFloat(ATTRIB_TESTVALUE)->GetValue());
            //columnValue.ConvertToNonBreakingHTMLSpaces();
            outResult->FormatAdd("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.AsChar(), columnValue.AsChar());

            // add the range value
            columnName = "Range Value: ";
            //columnName.ConvertToNonBreakingHTMLSpaces();
            columnValue.Format("%.3f", GetAttributeFloat(ATTRIB_RANGEVALUE)->GetValue());
            //columnValue.ConvertToNonBreakingHTMLSpaces();
            outResult->FormatAdd("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.AsChar(), columnValue.AsChar());

            // add the test function
            columnName = "Test Function: ";
            //columnName.ConvertToNonBreakingHTMLSpaces();
            columnValue = GetTestFunctionString();
            //columnValue.ConvertToNonBreakingHTMLSpaces();
            outResult->FormatAdd("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td></tr>", columnName.AsChar(), columnValue.AsChar());
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



    //------------------------------------------------------------------------------------------
    // String Test Functions
    //------------------------------------------------------------------------------------------
    /*bool AnimGraphParameterCondition::StringTestEqualCaseSensitive(const MCore::String& paramValue, const char* testValue)           { return paramValue.CheckIfIsEqual(testValue); }
    bool AnimGraphParameterCondition::StringTestEqualCaseInsensitive(const MCore::String& paramValue, const char* testValue)       { return paramValue.CheckIfIsEqualNoCase(testValue); }
    bool AnimGraphParameterCondition::StringTestNotEqualCaseSensitive(const MCore::String& paramValue, const char* testValue)      { return !(paramValue.CheckIfIsEqual(testValue)); }
    bool AnimGraphParameterCondition::StringTestNotEqualCaseInsensitive(const MCore::String& paramValue, const char* testValue)        { return !(paramValue.CheckIfIsEqualNoCase(testValue)); }*/
}   // namespace EMotionFX
