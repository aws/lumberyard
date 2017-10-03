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
#include <MCore/Source/AttributeSettings.h>
#include "AnimGraphMotionCondition.h"
#include "AnimGraph.h"
#include "AnimGraphManager.h"
#include "AnimGraphMotionNode.h"
#include "AnimGraphInstance.h"
#include <EMotionFX/Source/MotionSet.h>
#include "EMotionFXManager.h"


namespace EMotionFX
{
    AnimGraphMotionCondition::AnimGraphMotionCondition(AnimGraph* animGraph)
        : AnimGraphTransitionCondition(animGraph, TYPE_ID)
    {
        mMotionNode = nullptr;
        CreateAttributeValues();
        InitInternalAttributesForAllInstances();
    }


    AnimGraphMotionCondition::~AnimGraphMotionCondition()
    {
    }


    AnimGraphMotionCondition* AnimGraphMotionCondition::Create(AnimGraph* animGraph)
    {
        return new AnimGraphMotionCondition(animGraph);
    }


    AnimGraphObjectData* AnimGraphMotionCondition::CreateObjectData()
    {
        return new UniqueData(this, nullptr, nullptr);
    }


    void AnimGraphMotionCondition::RegisterAttributes()
    {
        // Register the source motion file name.
        MCore::AttributeSettings* attributeInfo = RegisterAttribute("Motion", "motionID", "The motion node to use.", ATTRIBUTE_INTERFACETYPE_BLENDTREEMOTIONNODEPICKER);
        attributeInfo->SetDefaultValue(MCore::AttributeString::Create());

        // Create the function combobox.
        attributeInfo = RegisterAttribute("Test Function", "testFunction", "The type of test function or condition.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        attributeInfo->ResizeComboValues(7);
        attributeInfo->SetComboValue(FUNCTION_EVENT,                    "Motion Event");
        attributeInfo->SetComboValue(FUNCTION_HASENDED,                 "Has Ended");
        attributeInfo->SetComboValue(FUNCTION_HASREACHEDMAXNUMLOOPS,    "Has Reached Max Num Loops");
        attributeInfo->SetComboValue(FUNCTION_PLAYTIME,                 "Has Reached Specified Play Time");
        attributeInfo->SetComboValue(FUNCTION_PLAYTIMELEFT,             "Has Less Play Time Left");
        attributeInfo->SetComboValue(FUNCTION_ISMOTIONASSIGNED,         "Is Motion Assigned?");
        attributeInfo->SetComboValue(FUNCTION_ISMOTIONNOTASSIGNED,      "Is Motion Not Assigned?");
        attributeInfo->SetDefaultValue(MCore::AttributeFloat::Create(FUNCTION_HASENDED));

        // Create the number of loops attribute.
        attributeInfo = RegisterAttribute("Num Loops", "numLoops", "The int value to test against the number of loops the motion already played.", MCore::ATTRIBUTE_INTERFACETYPE_INTSPINNER);
        attributeInfo->SetDefaultValue(MCore::AttributeFloat::Create(1));
        attributeInfo->SetMinValue(MCore::AttributeFloat::Create(1));
        attributeInfo->SetMaxValue(MCore::AttributeFloat::Create(100000000.0f));

        // Create the max playtime attribute.
        attributeInfo = RegisterAttribute("Time Value", "playtime", "The float value in seconds to test against.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        attributeInfo->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        attributeInfo->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        attributeInfo->SetMaxValue(MCore::AttributeFloat::Create(std::numeric_limits<float>::max()));

        // Register the event type.
        attributeInfo = RegisterAttribute("Event Type", "eventType", "The event type to catch.", MCore::ATTRIBUTE_INTERFACETYPE_STRING);
        attributeInfo->SetDefaultValue(MCore::AttributeString::Create());

        // Register the event parameter.
        attributeInfo = RegisterAttribute("Event Parameter", "eventParam", "The event parameter to catch.", MCore::ATTRIBUTE_INTERFACETYPE_STRING);
        attributeInfo->SetDefaultValue(MCore::AttributeString::Create());
    }


    const char* AnimGraphMotionCondition::GetPaletteName() const
    {
        return "Motion Condition";
    }


    const char* AnimGraphMotionCondition::GetTypeString() const
    {
        return "AnimGraphMotionCondition";
    }

    
    bool AnimGraphMotionCondition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        // Make sure the motion node to which the motion condition is linked to is valid.
        if (!mMotionNode)
        {
            return false;
        }

        // Early condition function check pass for the 'Is motion assigned?'. We can do this before retrieving the unique data.
        const int32 functionIndex = GetAttributeFloatAsInt32(ATTRIB_FUNCTION);
        if (functionIndex == FUNCTION_ISMOTIONASSIGNED || functionIndex == FUNCTION_ISMOTIONNOTASSIGNED)
        {
            const EMotionFX::MotionSet* motionSet = animGraphInstance->GetMotionSet();
            if (!motionSet)
            {
                return false;
            }

            // Iterate over motion entries from the motion node and check if there are motions assigned to them.
            const AZ::u32 numMotions = mMotionNode->GetNumMotions();
            for (AZ::u32 i = 0; i < numMotions; ++i)
            {
                const char* motionId = mMotionNode->GetMotionID(i);
                const EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntryByStringID(motionId);
                
                if (functionIndex == FUNCTION_ISMOTIONASSIGNED)
                {
                    // Any unassigned motion entry will make the condition to fail.
                    if (!motionEntry || (motionEntry && motionEntry->GetFilenameString().empty()))
                    {
                        return false;
                    }
                }
                else
                {
                    if (motionEntry && !motionEntry->GetFilenameString().empty())
                    {
                        // Any assigned motion entry will make the condition to fail.
                        return false;
                    }
                }
            }

            return true;
        }

        // Add the unique data for the condition to the anim graph.
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (!uniqueData)
        {
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(AnimGraphMotionCondition::TYPE_ID, const_cast<AnimGraphMotionCondition*>(this), animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        MotionInstance* motionInstance = mMotionNode->FindMotionInstance(animGraphInstance);
        if (!motionInstance)
        {
            return false;
        }

        // Update the unique data.
        if (uniqueData->mMotionInstance != motionInstance ||
            uniqueData->mMotionInstance->FindEventHandlerIndex(uniqueData->mEventHandler) == MCORE_INVALIDINDEX32)
        {
            // Create a new event handler for this motion condition and add it to the motion instance.
            uniqueData->mMotionInstance = motionInstance;
            uniqueData->mEventHandler = AnimGraphMotionCondition::EventHandler::Create(const_cast<AnimGraphMotionCondition*>(this), uniqueData);
            uniqueData->mMotionInstance->AddEventHandler(uniqueData->mEventHandler);
        }

        // In case a motion event got triggered or a motion has looped constantly fire true until the condition gets reset.
        if (uniqueData->mTriggered)
        {
            return true;
        }

        // Process the condition depending on the function used.
        switch (functionIndex)
        {
            // Has motion finished playing?
            case FUNCTION_HASENDED:
            {
                // Special case for non looping motions only.
                if (!motionInstance->GetIsPlayingForever())
                {
                    // Get the play time and the animation length.
                    const float currentTime = motionInstance->GetCurrentTime();
                    const float maxTime     = motionInstance->GetMaxTime();

                    // Differentiate between the play modes.
                    const EPlayMode playMode = motionInstance->GetPlayMode();
                    if (playMode == PLAYMODE_FORWARD)
                    {
                        // Return true in case the current playtime has reached the animation end.
                        if (currentTime >= maxTime - MCore::Math::epsilon)
                        {
                            return true;
                        }
                    }
                    else if (playMode == PLAYMODE_BACKWARD)
                    {
                        // Return true in case the current playtime has reached the animation start.
                        if (currentTime <= MCore::Math::epsilon)
                        {
                            return true;
                        }
                    }
                }
            }
            break;

            // Less than a given amount of play time left.
            case FUNCTION_PLAYTIMELEFT:
            {
                // Get the play time to check against.
                const float playTimeValue = GetAttributeFloat(ATTRIB_PLAYTIME)->GetValue();
                const float timeLeft = motionInstance->GetMaxTime() - motionInstance->GetCurrentTime();

                // Check if we are past that.
                if (timeLeft <= playTimeValue)
                {
                    return true;
                }
            }
            break;

            // Maximum number of loops.
            case FUNCTION_HASREACHEDMAXNUMLOOPS:
            {
                if ((int32)motionInstance->GetNumCurrentLoops() >= GetAttributeFloat(ATTRIB_NUMLOOPS)->GetValue())
                {
                    return true;
                }
            }
            break;

            // Reached the specified play time.
            // The has reached play time condition is not part of the event handler, so we have to manually handle it here.
            case FUNCTION_PLAYTIME:
            {
                // Get the play time to check against.
                const float playTime = GetAttributeFloat(ATTRIB_PLAYTIME)->GetValue();

                // Reached the specified play time.
                if (uniqueData->mMotionInstance->GetCurrentTime() > playTime)
                {
                    return true;
                }
            }
            break;
        }

        // No event got triggered, continue playing the state and don't autostart the transition.
        return false;
    }


    AnimGraphObject* AnimGraphMotionCondition::Clone(AnimGraph* animGraph)
    {
        AnimGraphMotionCondition* clone = new AnimGraphMotionCondition(animGraph);
        CopyBaseObjectTo(clone);
        return clone;
    }


    void AnimGraphMotionCondition::OnUpdateAttributes()
    {
        // Get a pointer to the selected motion node.
        AnimGraphNode* animGraphNode = mAnimGraph->RecursiveFindNode(GetAttributeString(ATTRIB_MOTIONNODE)->AsChar());
        if (animGraphNode && animGraphNode->GetType() == AnimGraphMotionNode::TYPE_ID)
        {
            mMotionNode = static_cast<AnimGraphMotionNode*>(animGraphNode);
        }
        else
        {
            mMotionNode = nullptr;
        }

        // Disable GUI items that have no influence.
    #ifdef EMFX_EMSTUDIOBUILD
        // Enable all attributes.
        EnableAllAttributes(true);

        // Disable the range value if we're not using a function that needs the range.
        const int32 function = GetAttributeFloatAsInt32(ATTRIB_FUNCTION);
        switch (function)
        {
            case FUNCTION_EVENT:
                SetAttributeDisabled(ATTRIB_NUMLOOPS);
                SetAttributeDisabled(ATTRIB_PLAYTIME);
                break;

            case FUNCTION_HASENDED:
                SetAttributeDisabled(ATTRIB_EVENTTYPE);
                SetAttributeDisabled(ATTRIB_EVENTPARAMETER);
                SetAttributeDisabled(ATTRIB_NUMLOOPS);
                SetAttributeDisabled(ATTRIB_PLAYTIME);
                break;

            case FUNCTION_HASREACHEDMAXNUMLOOPS:
                SetAttributeDisabled(ATTRIB_EVENTTYPE);
                SetAttributeDisabled(ATTRIB_EVENTPARAMETER);
                SetAttributeDisabled(ATTRIB_PLAYTIME);
                break;

            case FUNCTION_PLAYTIME:
                SetAttributeDisabled(ATTRIB_EVENTTYPE);
                SetAttributeDisabled(ATTRIB_EVENTPARAMETER);
                SetAttributeDisabled(ATTRIB_NUMLOOPS);
                break;

            case FUNCTION_PLAYTIMELEFT:
                SetAttributeDisabled(ATTRIB_EVENTTYPE);
                SetAttributeDisabled(ATTRIB_EVENTPARAMETER);
                SetAttributeDisabled(ATTRIB_NUMLOOPS);
                break;

            case FUNCTION_ISMOTIONASSIGNED:
            case FUNCTION_ISMOTIONNOTASSIGNED:
                SetAttributeDisabled(ATTRIB_EVENTTYPE);
                SetAttributeDisabled(ATTRIB_EVENTPARAMETER);
                SetAttributeDisabled(ATTRIB_NUMLOOPS);
                SetAttributeDisabled(ATTRIB_PLAYTIME);
                break;

            default:
                MCORE_ASSERT(false);
        }
    #endif
    }


    void AnimGraphMotionCondition::Reset(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData)
        {
            uniqueData->mTriggered = false;
        }
    }


    void AnimGraphMotionCondition::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // Add the unique data for the condition to the anim graph.
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (!uniqueData)
        {
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(AnimGraphMotionCondition::TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }
    }


    AnimGraphMotionCondition::EFunction AnimGraphMotionCondition::GetFunction() const
    {
        return (EFunction)((uint32)(GetAttributeFloat(ATTRIB_FUNCTION)->GetValue()));
    }


    const char* AnimGraphMotionCondition::GetTestFunctionString() const
    {
        // Get access to the combo values and the currently selected function.
        const MCore::AttributeSettings* comboAttributeInfo  = GetAnimGraphManager().GetAttributeInfo(this, ATTRIB_FUNCTION);
        const uint32                    functionIndex       = GetAttributeFloatAsUint32(ATTRIB_FUNCTION);

        return comboAttributeInfo->GetComboValue(functionIndex);
    }


    void AnimGraphMotionCondition::GetSummary(MCore::String* outResult) const
    {
        outResult->Format("%s: Motion Node Name='%s', Test Function='%s'", GetTypeString(), GetAttributeString(ATTRIB_MOTIONNODE)->AsChar(), GetTestFunctionString());
    }


    void AnimGraphMotionCondition::GetTooltip(MCore::String* outResult) const
    {
        AZStd::string columnName;
        AZStd::string columnValue;

        // Add the condition type.
        columnName = "Condition Type: ";
        columnValue = GetTypeString();
        outResult->Format("<table border=\"0\"><tr><td width=\"130\"><b>%s</b></td><td>%s</td>", columnName.c_str(), columnValue.c_str());

        // Add the motion node name.
        columnName = "Motion Node Name: ";
        columnValue = GetAttributeString(ATTRIB_MOTIONNODE)->AsChar();
        outResult->FormatAdd("</tr><tr><td><b>%s</b></td><td><nobr>%s</nobr></td>", columnName.c_str(), columnValue.c_str());

        // Add the test function.
        columnName = "Test Function: ";
        columnValue = GetTestFunctionString();
        outResult->FormatAdd("</tr><tr><td><b>%s</b></td><td><nobr>%s</nobr></td></tr></table>", columnName.c_str(), columnValue.c_str());
    }

    //--------------------------------------------------------------------------------
    // class AnimGraphMotionCondition::UniqueData
    //--------------------------------------------------------------------------------
    AnimGraphMotionCondition::UniqueData::UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance, MotionInstance* motionInstance)
        : AnimGraphObjectData(object, animGraphInstance)
    {
        mMotionInstance = motionInstance;
        mEventHandler   = nullptr;
        mTriggered      = false;
    }


    AnimGraphMotionCondition::UniqueData::~UniqueData()
    {
        if (mMotionInstance && mEventHandler)
        {
            mMotionInstance->RemoveEventHandler(mEventHandler);
        }
    }

    //--------------------------------------------------------------------------------
    // class AnimGraphMotionCondition::EventHandler
    //--------------------------------------------------------------------------------
    AnimGraphMotionCondition::EventHandler::EventHandler(AnimGraphMotionCondition* condition, UniqueData* uniqueData)
        : MotionInstanceEventHandler()
    {
        mCondition      = condition;
        mUniqueData     = uniqueData;
    }


    AnimGraphMotionCondition::EventHandler::~EventHandler()
    {
    }


    void AnimGraphMotionCondition::EventHandler::OnDeleteMotionInstance()
    {
        // Reset the motion instance and the flag in the unique data.
        mUniqueData->mMotionInstance    = nullptr;
        mUniqueData->mTriggered         = false;
        mUniqueData->mEventHandler      = nullptr;
    }


    void AnimGraphMotionCondition::EventHandler::OnEvent(const EventInfo& eventInfo)
    {
        // Make sure the condition function is the right one.
        if (mCondition->GetFunction() != FUNCTION_EVENT)
        {
            return;
        }

        // Check if the triggered motion event is of the given type and parameter from the motion condition.
        if ((mCondition->GetAttributeString(ATTRIB_EVENTPARAMETER)->GetValue().GetIsEmpty() || mCondition->GetAttributeString(ATTRIB_EVENTPARAMETER)->GetValue().CheckIfIsEqual(eventInfo.mParameters->AsChar())) &&
            (mCondition->GetAttributeString(ATTRIB_EVENTTYPE)->GetValue().GetIsEmpty() || mCondition->GetAttributeString(ATTRIB_EVENTTYPE)->GetValue().CheckIfIsEqual(GetEventManager().GetEventTypeString(eventInfo.mTypeID))))
        {
            if (eventInfo.mIsEventStart)
            {
                mUniqueData->mTriggered = true;
            }
            else
            {
                mUniqueData->mTriggered = false;
            }
        }
    }


    void AnimGraphMotionCondition::EventHandler::OnHasLooped()
    {
        if (mCondition->GetFunction() != FUNCTION_HASENDED)
        {
            return;
        }

        mUniqueData->mTriggered = true;
    }


    void AnimGraphMotionCondition::EventHandler::OnIsFrozenAtLastFrame()
    {
        if (mCondition->GetFunction() != FUNCTION_HASENDED)
        {
            return;
        }

        mUniqueData->mTriggered = true;
    }


    void AnimGraphMotionCondition::OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const MCore::String& oldName)
    {
        MCORE_UNUSED(animGraph);
        if (GetAttributeString(ATTRIB_MOTIONNODE)->GetValue().CheckIfIsEqual(oldName))
        {
            GetAttributeString(ATTRIB_MOTIONNODE)->SetValue(node->GetName());
        }
    }


    // Callback that gets called before a node gets removed.
    void AnimGraphMotionCondition::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        MCORE_UNUSED(animGraph);
        if (GetAttributeString(ATTRIB_MOTIONNODE)->GetValue().CheckIfIsEqual(nodeToRemove->GetName()))
        {
            GetAttributeString(ATTRIB_MOTIONNODE)->SetValue("");
        }
    }


    AnimGraphMotionCondition::EventHandler* AnimGraphMotionCondition::EventHandler::Create(AnimGraphMotionCondition* condition, UniqueData* uniqueData)
    {
        return new AnimGraphMotionCondition::EventHandler(condition, uniqueData);
    }
} // namespace EMotionFX