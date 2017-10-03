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

#include "EMotionFXConfig.h"
#include <MCore/Source/Compare.h>
#include <MCore/Source/UnicodeString.h>
#include <MCore/Source/Random.h>
#include <MCore/Source/AttributeSettings.h>
#include <EMotionFX/Source/AnimGraphAttributeTypes.h>
#include "AnimGraphTagCondition.h"
#include "AnimGraph.h"
#include "AnimGraphInstance.h"
#include "AnimGraphManager.h"
#include "EMotionFXManager.h"


namespace EMotionFX
{
    AnimGraphTagCondition::AnimGraphTagCondition(AnimGraph* animGraph)
        : AnimGraphTransitionCondition(animGraph, TYPE_ID)
    {
        CreateAttributeValues();
        InitInternalAttributesForAllInstances();
    }


    AnimGraphTagCondition::~AnimGraphTagCondition()
    {
    }


    AnimGraphTagCondition* AnimGraphTagCondition::Create(AnimGraph* animGraph)
    {
        return new AnimGraphTagCondition(animGraph);
    }


    void AnimGraphTagCondition::RegisterAttributes()
    {
        // Create the function combobox.
        MCore::AttributeSettings* attributeInfo = RegisterAttribute("Test Function", "testFunction", "The type of test function or condition.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        attributeInfo->ResizeComboValues(4);
        attributeInfo->SetComboValue(FUNCTION_ALL,        "All tags active");
        attributeInfo->SetComboValue(FUNCTION_NOTALL,     "One or more tags inactive");
        attributeInfo->SetComboValue(FUNCTION_ONEORMORE,  "One or more tags active");
        attributeInfo->SetComboValue(FUNCTION_NONE,       "No tag active");
        attributeInfo->SetDefaultValue(MCore::AttributeFloat::Create(FUNCTION_ALL));

        // Create the tags picker attribute.
        attributeInfo = RegisterAttribute("Tags", "tags", "The tags to watch.", ATTRIBUTE_INTERFACETYPE_TAGPICKER);
        attributeInfo->SetReinitGuiOnValueChange(true);
        attributeInfo->SetDefaultValue(MCore::AttributeArray::Create(MCore::AttributeString::TYPE_ID));
    }


    void AnimGraphTagCondition::OnUpdateAttributes()
    {
        const MCore::AttributeArray* arrayAttrib = GetAttributeArray(ATTRIB_TAGS);
        if (!arrayAttrib)
        {
            mTagParameterIndices.clear();
            return;
        }

        // Iterate through the chosen tags in the condition to cache the parameter indices.
        const size_t attributeCount = arrayAttrib->GetNumAttributes();
        mTagParameterIndices.resize(attributeCount);
        for (size_t i = 0; i < attributeCount; ++i)
        {
            const MCore::Attribute* attribute = arrayAttrib->GetAttribute(static_cast<uint32>(i));
            const MCore::String& tag = static_cast<const MCore::AttributeString*>(attribute)->GetValue();

            // Search for the parameter with the name of the tag and save the index.
            const AZ::u32 parameterIndex = mAnimGraph->FindParameterIndex(tag.AsChar());

            // Cache the parameter index to avoid string lookups at runtime.
            mTagParameterIndices[i] = parameterIndex;
        }
    }


    const char* AnimGraphTagCondition::GetPaletteName() const
    {
        return "Tag Condition";
    }


    const char* AnimGraphTagCondition::GetTypeString() const
    {
        return "AnimGraphTagCondition";
    }

    
    bool AnimGraphTagCondition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        const EFunction testFunction = static_cast<EFunction>(GetAttributeFloatAsInt32(ATTRIB_FUNCTION));

        // Iterate over the cached tag parameters.
        for (AZ::u32 parameterIndex : mTagParameterIndices)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                continue;
            }

            // Is the tag active?
            float value;
            animGraphInstance->GetFloatParameterValue(parameterIndex, &value);
            const bool tagActive = value > 0.5f;

            switch (testFunction)
            {
                case FUNCTION_ALL:
                {
                    // All tags have to be active, once we hit an inactive one, return failure.
                    if (!tagActive)
                    {
                        return false;
                    }
                    break;
                }

                case FUNCTION_ONEORMORE:
                {
                    // Return success with the first active tag.
                    if (tagActive)
                    {
                        return true;
                    }
                    break;
                }

                case FUNCTION_NONE:
                {
                    // No tags should be active. Return failure once with first active tag.
                    if (tagActive)
                    {
                        return false;
                    }
                    break;
                }

                case FUNCTION_NOTALL:
                {
                    // At least one tag has to be inactive. Once we hit the first inactive one, return success.
                    if (!tagActive)
                    {
                        return true;
                    }
                    break;
                }

                default:
                {
                    AZ_Assert(false, "Tag condition test function is undefined.");
                    return false;
                }
            }
        }

        if (testFunction == FUNCTION_ONEORMORE || testFunction == FUNCTION_NOTALL)
        {
            return false;
        }

        return true;
    }


    AnimGraphObject* AnimGraphTagCondition::Clone(AnimGraph* animGraph)
    {
        AnimGraphTagCondition* clone = new AnimGraphTagCondition(animGraph);
        CopyBaseObjectTo(clone);
        return clone;
    }


    AnimGraphObjectData* AnimGraphTagCondition::CreateObjectData()
    {
        return AnimGraphObjectData::Create(this, nullptr);
    }


    const char* AnimGraphTagCondition::GetTestFunctionString() const
    {
        MCore::AttributeSettings* comboAttributeInfo = GetAnimGraphManager().GetAttributeInfo(this, ATTRIB_FUNCTION);
        const int32 testFunction = GetAttributeFloatAsInt32(ATTRIB_FUNCTION);

        return comboAttributeInfo->GetComboValue(testFunction);
    }


    void AnimGraphTagCondition::CreateTagString(AZStd::string& outTagString) const
    {
        const MCore::AttributeArray* arrayAttrib = GetAttributeArray(ATTRIB_TAGS);
        if (!arrayAttrib)
        {
            outTagString += "[]";
            return;
        }

        outTagString = "[";

        const size_t attributeCount = arrayAttrib->GetNumAttributes();
        for (size_t i = 0; i < attributeCount; ++i)
        {
            const MCore::Attribute* attribute = arrayAttrib->GetAttribute(static_cast<uint32>(i));
            const MCore::String& tag = static_cast<const MCore::AttributeString*>(attribute)->GetValue();

            if (i > 0)
            {
                outTagString += ", ";
            }

            outTagString += tag;
        }

        outTagString += "]";
    }

    
    void AnimGraphTagCondition::GetSummary(MCore::String* outResult) const
    {
        const EFunction testFunction = static_cast<EFunction>(GetAttributeFloatAsInt32(ATTRIB_FUNCTION));
        outResult->Format("%s: Test Function='%s', Tags=", GetTypeString(), GetTestFunctionString());

        // Add the tags.
        AZStd::string tagString;
        CreateTagString(tagString);
        *outResult += tagString.c_str();
    }


    void AnimGraphTagCondition::GetTooltip(MCore::String* outResult) const
    {
        AZStd::string columnName, columnValue;

        // Add the condition type.
        columnName = "Condition Type: ";
        columnValue = GetTypeString();
        outResult->Format("<table border=\"0\"><tr><td width=\"165\"><b>%s</b></td><td>%s</td>", columnName.c_str(), columnValue.c_str());

        // Add the tag test function.
        columnName = "Test Function: ";
        columnValue = GetTestFunctionString();
        outResult->FormatAdd("</tr><tr><td><b>%s</b></td><td>%s</td>", columnName.c_str(), columnValue.c_str());

        // Add the tags.
        columnName = "Tags: ";
        CreateTagString(columnValue);
        outResult->FormatAdd("</tr><tr><td><b>%s</b></td><td>%s</td>", columnName.c_str(), columnValue.c_str());
    }
} // namespace EMotionFX