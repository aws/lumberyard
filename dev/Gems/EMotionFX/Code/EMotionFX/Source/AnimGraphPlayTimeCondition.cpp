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
#include "AnimGraphPlayTimeCondition.h"
#include "AnimGraph.h"
#include "AnimGraphNode.h"
#include "AnimGraphInstance.h"
#include "EMotionFXManager.h"
#include "AnimGraphManager.h"


namespace EMotionFX
{
    // constructor
    AnimGraphPlayTimeCondition::AnimGraphPlayTimeCondition(AnimGraph* animGraph)
        : AnimGraphTransitionCondition(animGraph, TYPE_ID)
    {
        mNode = nullptr;
        CreateAttributeValues();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    AnimGraphPlayTimeCondition::~AnimGraphPlayTimeCondition()
    {
    }


    // create
    AnimGraphPlayTimeCondition* AnimGraphPlayTimeCondition::Create(AnimGraph* animGraph)
    {
        return new AnimGraphPlayTimeCondition(animGraph);
    }


    // create unique data
    AnimGraphObjectData* AnimGraphPlayTimeCondition::CreateObjectData()
    {
        return AnimGraphObjectData::Create(this, nullptr);
    }


    // register the attributes
    void AnimGraphPlayTimeCondition::RegisterAttributes()
    {
        // create the node selection picker attribute
        MCore::AttributeSettings* attribInfo = RegisterAttribute("Node", "nodeID", "The node to use.", ATTRIBUTE_INTERFACETYPE_ANIMGRAPHNODEPICKER);
        attribInfo->SetDefaultValue(MCore::AttributeString::Create());

        // create the play time value float spinner
        attribInfo = RegisterAttribute("Play Time", "countDownTime", "The play time in seconds.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        attribInfo->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));
        attribInfo->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        attribInfo->SetMaxValue(MCore::AttributeFloat::Create(FLT_MAX));

        // the mode
        MCore::AttributeSettings* modeParam = RegisterAttribute("Mode", "mode", "The way how to check the given play time set in this condition with the playtime from the node.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        modeParam->ResizeComboValues(MODE_NUM);
        modeParam->SetComboValue(MODE_REACHEDTIME,  "Reached Play Time X");
        modeParam->SetComboValue(MODE_REACHEDEND,   "Reached End");
        modeParam->SetComboValue(MODE_HASLESSTHAN,  "Less Than X Seconds Left");
        //modeParam->SetComboValue(MODE_HASMORETHAN, "More Than X Seconds Left");
        modeParam->SetDefaultValue(MCore::AttributeFloat::Create(0));
    }


    // get the palette name
    const char* AnimGraphPlayTimeCondition::GetPaletteName() const
    {
        return "Play Time Condition";
    }


    // get the type string
    const char* AnimGraphPlayTimeCondition::GetTypeString() const
    {
        return "AnimGraphPlayTimeCondition";
    }

    
    // test the condition
    bool AnimGraphPlayTimeCondition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        // if no node has been selected yet, always return false
        if (mNode == nullptr)
        {
            return false;
        }

        // get the playtime and the mode from the attributes
        const float x           = GetAttributeFloat(ATTRIB_PLAYTIME)->GetValue();
        const int32 mode        = static_cast<int32>(GetAttributeFloat(ATTRIB_MODE)->GetValue());
        const float playTime    = mNode->GetCurrentPlayTime(animGraphInstance);
        const float duration    = mNode->GetDuration(animGraphInstance);
        const float timeLeft    = duration - playTime;// TODO: what about backwards playing a node?

        switch (mode)
        {
        // has the selected node reached the given playtime?
        case MODE_REACHEDTIME:
        {
            if (playTime >= x)
            {
                return true;
            }

            break;
        }

        // has the selected node reached the end? does only work for non-looping motions
        case MODE_REACHEDEND:
        {
            if (playTime >= duration - MCore::Math::epsilon)
            {
                return true;
            }

            break;
        }

        // has the selected node less than X seconds remaining?
        case MODE_HASLESSTHAN:
        {
            if (timeLeft <= x + MCore::Math::epsilon)
            {
                return true;
            }

            break;
        }
        }

        return false;
    }


    // clonse the condition
    AnimGraphObject* AnimGraphPlayTimeCondition::Clone(AnimGraph* animGraph)
    {
        // create the clone
        AnimGraphPlayTimeCondition* clone = new AnimGraphPlayTimeCondition(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // construct and output the information summary string for this object
    void AnimGraphPlayTimeCondition::GetSummary(MCore::String* outResult) const
    {
        MCore::AttributeSettings* modeParam = GetAnimGraphManager().GetAttributeInfo(this, ATTRIB_MODE);
        const int32 mode = GetAttributeFloatAsInt32(ATTRIB_MODE);
        outResult->Format("%s: NodeName='%s', Play Time=%.2f secs, Mode='%s'", GetTypeString(), GetAttributeString(ATTRIB_NODE)->AsChar(), GetAttributeFloat(ATTRIB_PLAYTIME)->GetValue(), modeParam->GetComboValue(mode));
    }


    // construct and output the tooltip for this object
    void AnimGraphPlayTimeCondition::GetTooltip(MCore::String* outResult) const
    {
        MCore::AttributeSettings* modeParam = GetAnimGraphManager().GetAttributeInfo(this, ATTRIB_MODE);
        const int32 mode = GetAttributeFloatAsInt32(ATTRIB_MODE);

        MCore::String columnName, columnValue;

        // add the condition type
        columnName = "Condition Type: ";
        columnValue = GetTypeString();
        outResult->Format("<table border=\"0\"><tr><td width=\"105\"><b>%s</b></td><td>%s</td>", columnName.AsChar(), columnValue.AsChar());

        // add the node
        columnName = "Node: ";
        outResult->FormatAdd("</tr><tr><td><b>%s</b></td><td>%s</td>", columnName.AsChar(), GetAttributeString(ATTRIB_NODE)->AsChar());

        // add the time
        columnName = "Play Time: ";
        outResult->FormatAdd("</tr><tr><td><b>%s</b></td><td>%.2f secs</td>", columnName.AsChar(), GetAttributeFloat(ATTRIB_PLAYTIME)->GetValue());

        // add the mode
        columnName = "Mode: ";
        outResult->FormatAdd("</tr><tr><td><b>%s</b></td><td>%s</td>", columnName.AsChar(), modeParam->GetComboValue(mode));
    }


    // update the data
    void AnimGraphPlayTimeCondition::OnUpdateAttributes()
    {
        // get a pointer to the selected node
        mNode = mAnimGraph->RecursiveFindNode(GetAttributeString(ATTRIB_NODE)->AsChar());

        // disable GUI items that have no influence
    #ifdef EMFX_EMSTUDIOBUILD
        // enable all attributes
        EnableAllAttributes(true);

        // disable the interface elements in case no node has been selected yet
        if (mNode == nullptr)
        {
            SetAttributeDisabled(ATTRIB_PLAYTIME);
            SetAttributeDisabled(ATTRIB_MODE);
        }
    #endif
    }


    // callback for when we renamed a node
    void AnimGraphPlayTimeCondition::OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const MCore::String& oldName)
    {
        MCORE_UNUSED(animGraph);
        if (GetAttributeString(ATTRIB_NODE)->GetValue().CheckIfIsEqual(oldName))
        {
            GetAttributeString(ATTRIB_NODE)->SetValue(node->GetName());
        }
    }


    // callback that gets called before a node gets removed
    void AnimGraphPlayTimeCondition::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        MCORE_UNUSED(animGraph);
        if (GetAttributeString(ATTRIB_NODE)->GetValue().CheckIfIsEqual(nodeToRemove->GetName()))
        {
            GetAttributeString(ATTRIB_NODE)->SetValue("");
        }
    }
} // namespace EMotionFX
