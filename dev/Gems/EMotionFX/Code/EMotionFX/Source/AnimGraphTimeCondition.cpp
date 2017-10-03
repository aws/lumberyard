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
#include <MCore/Source/Random.h>
#include <MCore/Source/AttributeSettings.h>
#include "AnimGraphTimeCondition.h"
#include "AnimGraph.h"
#include "AnimGraphInstance.h"
#include "AnimGraphManager.h"
#include "EMotionFXManager.h"


namespace EMotionFX
{
    // constructor
    AnimGraphTimeCondition::AnimGraphTimeCondition(AnimGraph* animGraph)
        : AnimGraphTransitionCondition(animGraph, TYPE_ID)
    {
        CreateAttributeValues();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    AnimGraphTimeCondition::~AnimGraphTimeCondition()
    {
    }


    // create
    AnimGraphTimeCondition* AnimGraphTimeCondition::Create(AnimGraph* animGraph)
    {
        return new AnimGraphTimeCondition(animGraph);
    }


    // create unique data
    AnimGraphObjectData* AnimGraphTimeCondition::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }


    // register the attributes
    void AnimGraphTimeCondition::RegisterAttributes()
    {
        // create the count down time value float spinner
        MCore::AttributeSettings* attribInfo = RegisterAttribute("Count Down Time", "countDownTime", "The amount of seconds the condition will count down until the condition will trigger.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        attribInfo->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));
        attribInfo->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        attribInfo->SetMaxValue(MCore::AttributeFloat::Create(FLT_MAX));

        // create the randomization flag checkbox
        attribInfo = RegisterAttribute("Use Randomization", "randomization", "When randomization is enabled the count down time will be a random one between the min and max random time.", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        attribInfo->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // create the min random time value float spinner
        attribInfo = RegisterAttribute("Min Random Time", "minRandomTime", "The minimum randomized count down time. This will be only used then randomization is enabled.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        attribInfo->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        attribInfo->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        attribInfo->SetMaxValue(MCore::AttributeFloat::Create(FLT_MAX));

        // create the max random time value float spinner
        attribInfo = RegisterAttribute("Max Random Time", "maxRandomTime", "The maximum randomized count down time. This will be only used then randomization is enabled.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        attribInfo->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));
        attribInfo->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        attribInfo->SetMaxValue(MCore::AttributeFloat::Create(FLT_MAX));
    }


    // get the palette name
    const char* AnimGraphTimeCondition::GetPaletteName() const
    {
        return "Time Condition";
    }


    // get the type string
    const char* AnimGraphTimeCondition::GetTypeString() const
    {
        return "AnimGraphTimeCondition";
    }

    
    // update the passed time of the condition
    void AnimGraphTimeCondition::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // add the unique data for the condition to the anim graph
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));

        // increase the elapsed time of the condition
        uniqueData->mElapsedTime += timePassedInSeconds;
    }


    // reset the condition
    void AnimGraphTimeCondition::Reset(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data and reset it
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));

        // reset the elapsed time
        uniqueData->mElapsedTime = 0.0f;

        // use randomized count downs?
        if (GetAttributeFloatAsBool(ATTRIB_USERANDOMIZATION))
        {
            const float minValue = GetAttributeFloat(ATTRIB_MINRANDOMTIME)->GetValue();
            const float maxValue = GetAttributeFloat(ATTRIB_MAXRANDOMTIME)->GetValue();

            // create a randomized count down value
            uniqueData->mCountDownTime = MCore::Random::RandF(minValue, maxValue);
        }
        else
        {
            // get the fixed count down value from the attribute
            uniqueData->mCountDownTime = GetAttributeFloat(ATTRIB_COUNTDOWNTIME)->GetValue();
        }
    }


    // test the condition
    bool AnimGraphTimeCondition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        // add the unique data for the condition to the anim graph
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));

        // in case the elapsed time is bigger than the count down time, we can trigger the condition
        if (uniqueData->mElapsedTime >= uniqueData->mCountDownTime)
        {
            return true;
        }

        // we have not reached the count down time yet, don't trigger yet
        return false;
    }


    // clonse the condition
    AnimGraphObject* AnimGraphTimeCondition::Clone(AnimGraph* animGraph)
    {
        // create the clone
        AnimGraphTimeCondition* clone = new AnimGraphTimeCondition(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // pre-create unique data object
    void AnimGraphTimeCondition::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // add the unique data for the condition to the anim graph
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            //uniqueData = new UniqueData(this, animGraphInstance);
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }
    }


    // construct and output the information summary string for this object
    void AnimGraphTimeCondition::GetSummary(MCore::String* outResult) const
    {
        outResult->Format("%s: Count Down=%.2f secs, RandomizationUsed=%d, Random Count Down Range=(%.2f secs, %.2f secs)", GetTypeString(), GetAttributeFloat(ATTRIB_COUNTDOWNTIME)->GetValue(), GetAttributeFloat(ATTRIB_USERANDOMIZATION)->GetValue(), GetAttributeFloat(ATTRIB_MINRANDOMTIME)->GetValue(), GetAttributeFloat(ATTRIB_MAXRANDOMTIME)->GetValue());
    }


    // construct and output the tooltip for this object
    void AnimGraphTimeCondition::GetTooltip(MCore::String* outResult) const
    {
        MCore::String columnName, columnValue;

        // add the condition type
        columnName = "Condition Type: ";
        columnValue = GetTypeString();
        outResult->Format("<table border=\"0\"><tr><td width=\"165\"><b>%s</b></td><td>%s</td>", columnName.AsChar(), columnValue.AsChar());

        // add the count down
        columnName = "Count Down: ";
        outResult->FormatAdd("</tr><tr><td><b>%s</b></td><td>%.2f secs</td>", columnName.AsChar(), GetAttributeFloat(ATTRIB_COUNTDOWNTIME)->GetValue());

        // add the randomization used flag
        columnName = "Randomization Used: ";
        outResult->FormatAdd("</tr><tr><td><b>%s</b></td><td>%s</td>", columnName.AsChar(), GetAttributeFloat(ATTRIB_USERANDOMIZATION)->GetValue() ? "Yes" : "No");

        // add the random count down range
        columnName = "Random Count Down Range: ";
        outResult->FormatAdd("</tr><tr><td><b>%s</b></td><td>(%.2f secs, %.2f secs)</td></tr></table>", columnName.AsChar(), GetAttributeFloat(ATTRIB_MINRANDOMTIME)->GetValue(), GetAttributeFloat(ATTRIB_MAXRANDOMTIME)->GetValue());
    }


    // update the attributes
    void AnimGraphTimeCondition::OnUpdateAttributes()
    {
        // disable GUI items that have no influence
    #ifdef EMFX_EMSTUDIOBUILD
        // enable all attributes
        EnableAllAttributes(true);

        // disable randomize values if randomization is off
        if (GetAttributeFloatAsBool(ATTRIB_USERANDOMIZATION) == false)
        {
            SetAttributeDisabled(ATTRIB_MINRANDOMTIME);
            SetAttributeDisabled(ATTRIB_MAXRANDOMTIME);
        }
        else
        {
            SetAttributeDisabled(ATTRIB_COUNTDOWNTIME);
        }
    #endif
    }


    //--------------------------------------------------------------------------------
    // class AnimGraphTimeCondition::UniqueData
    //--------------------------------------------------------------------------------

    // constructor
    AnimGraphTimeCondition::UniqueData::UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance)
        : AnimGraphObjectData(object, animGraphInstance)
    {
        mElapsedTime    = 0.0f;
        mCountDownTime  = 0.0f;
    }


    // destructor
    AnimGraphTimeCondition::UniqueData::~UniqueData()
    {
    }
} // namespace EMotionFX
