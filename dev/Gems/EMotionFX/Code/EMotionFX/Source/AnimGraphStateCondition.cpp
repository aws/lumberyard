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
#include "AnimGraphStateCondition.h"
#include "AnimGraph.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraphExitNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphManager.h"
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    AnimGraphStateCondition::AnimGraphStateCondition(AnimGraph* animGraph)
        : AnimGraphTransitionCondition(animGraph, TYPE_ID)
    {
        mState = nullptr;

        CreateAttributeValues();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    AnimGraphStateCondition::~AnimGraphStateCondition()
    {
    }


    // create
    AnimGraphStateCondition* AnimGraphStateCondition::Create(AnimGraph* animGraph)
    {
        return new AnimGraphStateCondition(animGraph);
    }


    // create unique data
    AnimGraphObjectData* AnimGraphStateCondition::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }


    // register the attributes
    void AnimGraphStateCondition::RegisterAttributes()
    {
        // register the state name
        MCore::AttributeSettings* attributeInfo = RegisterAttribute("State", "stateID", "The state to watch.", ATTRIBUTE_INTERFACETYPE_STATESELECTION);
        attributeInfo->SetDefaultValue(MCore::AttributeString::Create());

        // create the function combobox
        attributeInfo = RegisterAttribute("Test Function", "testFunction", "The type of test function or condition.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        attributeInfo->ResizeComboValues(6);
        attributeInfo->SetComboValue(FUNCTION_EXITSTATES,   "Trigger When Exit State Reached");
        attributeInfo->SetComboValue(FUNCTION_ENTERING,     "Started Transitioning Into State");
        attributeInfo->SetComboValue(FUNCTION_ENTER,        "State Fully Blended In");
        attributeInfo->SetComboValue(FUNCTION_EXIT,         "Leaving State, Transitioning Out");
        attributeInfo->SetComboValue(FUNCTION_END,          "State Fully Blended Out");
        attributeInfo->SetComboValue(FUNCTION_PLAYTIME,     "Has Reached Specified Playtime");
        attributeInfo->SetDefaultValue(MCore::AttributeFloat::Create(FUNCTION_EXITSTATES));

        // create the max playtime attribute
        attributeInfo = RegisterAttribute("Play Time", "playtime", "The float value in seconds to test against the current playtime of the state.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        attributeInfo->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        attributeInfo->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        attributeInfo->SetMaxValue(MCore::AttributeFloat::Create(FLT_MAX));
    }


    // get the palette name
    const char* AnimGraphStateCondition::GetPaletteName() const
    {
        return "State Condition";
    }


    // get the type string
    const char* AnimGraphStateCondition::GetTypeString() const
    {
        return "AnimGraphStateCondition";
    }


    // get the category
    AnimGraphObject::ECategory AnimGraphStateCondition::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_TRANSITIONCONDITIONS;
    }


    // test the condition
    bool AnimGraphStateCondition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        // add the unique data for the condition to the anim graph
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));

        // in case a event got triggered constantly fire true until the condition gets reset
        if (uniqueData->mTriggered)
        {
            return true;
        }

        // update the unique data
        if (uniqueData->mAnimGraphInstance != animGraphInstance ||
            uniqueData->mAnimGraphInstance->FindEventHandlerIndex(uniqueData->mEventHandler) == MCORE_INVALIDINDEX32)
        {
            // create a new event handler for this motion condition and add it to the motion instance
            uniqueData->mAnimGraphInstance = animGraphInstance;
            uniqueData->mEventHandler = new AnimGraphStateCondition::EventHandler(const_cast<AnimGraphStateCondition*>(this), uniqueData);
            uniqueData->mAnimGraphInstance->AddEventHandler(uniqueData->mEventHandler);
        }

        // get the condition test function type
        const int32 functionIndex = GetAttributeFloatAsInt32(ATTRIB_FUNCTION);

        // check what we want to do
        switch (functionIndex)
        {
        // reached an exit state
        case FUNCTION_EXITSTATES:
        {
            // check if the state is a valid state machine
            if (mState && mState->GetType() == AnimGraphStateMachine::TYPE_ID)
            {
                // type-cast the state to a state machine
                AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(mState);

                // check if we have reached an exit state
                if (stateMachine->GetExitStateReached(animGraphInstance))
                {
                    return true;
                }
            }

            break;
        }

        // reached the specified play time
        case FUNCTION_PLAYTIME:
        {
            // get the play time to check against
            // the has reached play time condition is not part of the event handler, so we have to manually handle it here
            const float playTime = GetAttributeFloat(ATTRIB_PLAYTIME)->GetValue();

            // reached the specified play time
            if (mState)
            {
                const float currentLocalTime = mState->GetCurrentPlayTime(uniqueData->mAnimGraphInstance);
                if (currentLocalTime > playTime)
                {
                    return true;
                }
            }
        }
        break;

        default:
        {
            // check if any event got triggered since we tested the condition the last time
            if (functionIndex == uniqueData->mLastTriggeredType)
            {
                uniqueData->mLastTriggeredType  = FUNCTION_NONE;
                uniqueData->mTriggered          = true;
                return true;
            }
        }
        }
        ;

        // no event got triggered, continue playing the state and don't autostart the transition
        return false;
    }


    // clonse the condition
    AnimGraphObject* AnimGraphStateCondition::Clone(AnimGraph* animGraph)
    {
        // create the clone
        AnimGraphStateCondition* clone = new AnimGraphStateCondition(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // update the data
    void AnimGraphStateCondition::OnUpdateAttributes()
    {
        // get a pointer to the selected motion node
        AnimGraphNode* animGraphNode = mAnimGraph->RecursiveFindNode(GetAttributeString(ATTRIB_STATE)->AsChar());
        if (animGraphNode && animGraphNode->GetCanActAsState())
        {
            mState = animGraphNode;
        }
        else
        {
            mState = nullptr;
        }

        // disable GUI items that have no influence
    #ifdef EMFX_EMSTUDIOBUILD
        // enable all attributes
        EnableAllAttributes(true);

        // disable the playtime value
        const int32 function = GetAttributeFloatAsInt32(ATTRIB_FUNCTION);
        if (function != FUNCTION_PLAYTIME)
        {
            SetAttributeDisabled(ATTRIB_PLAYTIME);
        }
    #endif
    }


    // reset the motion condition
    void AnimGraphStateCondition::Reset(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data and reset it
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        uniqueData->mTriggered          = false;
        uniqueData->mLastTriggeredType  = FUNCTION_NONE;
    }


    // when attribute values change
    void AnimGraphStateCondition::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // add the unique data for the condition to the anim graph
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            //uniqueData = new UniqueData(this, nullptr);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }
    }


    // get the name of the currently used test function
    const char* AnimGraphStateCondition::GetTestFunctionString() const
    {
        // get access to the combo values and the currently selected function
        MCore::AttributeSettings*   comboAttributeInfo  = GetAnimGraphManager().GetAttributeInfo(this, ATTRIB_FUNCTION);
        const uint32                functionIndex       = GetAttributeFloatAsUint32(ATTRIB_FUNCTION);

        return comboAttributeInfo->GetComboValue(functionIndex);
    }


    // construct and output the information summary string for this object
    void AnimGraphStateCondition::GetSummary(MCore::String* outResult) const
    {
        outResult->Format("%s: State='%s', Test Function='%s'", GetTypeString(), GetAttributeString(ATTRIB_STATE)->AsChar(), GetTestFunctionString());
    }


    // construct and output the tooltip for this object
    void AnimGraphStateCondition::GetTooltip(MCore::String* outResult) const
    {
        MCore::String columnName, columnValue;

        // add the condition type
        columnName = "Condition Type: ";
        columnValue = GetTypeString();
        outResult->Format("<table border=\"0\"><tr><td width=\"100\"><b>%s</b></td><td>%s</td>", columnName.AsChar(), columnValue.AsChar());

        // add the state name
        columnName = "State Name: ";
        columnValue = GetAttributeString(ATTRIB_STATE)->AsChar();
        outResult->FormatAdd("</tr><tr><td><b>%s</b></td><td>%s</td>", columnName.AsChar(), columnValue.AsChar());

        // add the test function
        columnName = "Test Function: ";
        columnValue = GetTestFunctionString();
        outResult->FormatAdd("</tr><tr><td><b>%s</b></td><td width=\"180\">%s</td></tr></table>", columnName.AsChar(), columnValue.AsChar());
    }

    //--------------------------------------------------------------------------------
    // class AnimGraphStateCondition::UniqueData
    //--------------------------------------------------------------------------------

    // constructor
    AnimGraphStateCondition::UniqueData::UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance)
        : AnimGraphObjectData(object, animGraphInstance)
    {
        mLastTriggeredType  = FUNCTION_NONE;
        mAnimGraphInstance = animGraphInstance;
        mEventHandler       = nullptr;
        mTriggered          = false;
    }


    // destructor
    AnimGraphStateCondition::UniqueData::~UniqueData()
    {
        // get rid of the event handler
        if (mEventHandler)
        {
            if (mAnimGraphInstance)
            {
                mAnimGraphInstance->RemoveEventHandler(mEventHandler, false);
            }

            mEventHandler->Destroy();
            mAnimGraphInstance = nullptr;
        }
    }


    // callback for when we renamed a node
    void AnimGraphStateCondition::OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const MCore::String& oldName)
    {
        MCORE_UNUSED(animGraph);
        if (GetAttributeString(ATTRIB_STATE)->GetValue().CheckIfIsEqual(oldName))
        {
            GetAttributeString(ATTRIB_STATE)->SetValue(node->GetName());
        }
    }


    // callback that gets called before a node gets removed
    void AnimGraphStateCondition::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        MCORE_UNUSED(animGraph);
        if (GetAttributeString(ATTRIB_STATE)->GetValue().CheckIfIsEqual(nodeToRemove->GetName()))
        {
            GetAttributeString(ATTRIB_STATE)->SetValue("");
        }
    }

    //--------------------------------------------------------------------------------
    // class AnimGraphStateCondition::EventHandler
    //--------------------------------------------------------------------------------

    // constructor
    AnimGraphStateCondition::EventHandler::EventHandler(AnimGraphStateCondition* condition, UniqueData* uniqueData)
        : EMotionFX::AnimGraphInstanceEventHandler()
    {
        mCondition      = condition;
        mUniqueData     = uniqueData;
    }


    // destructor
    AnimGraphStateCondition::EventHandler::~EventHandler()
    {
    }


    void AnimGraphStateCondition::EventHandler::OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        // check if the state and the anim graph instance are valid and return directly in case one of them is not
        if (state == nullptr || animGraphInstance == nullptr)
        {
            return;
        }

        const MCore::String& stateName = mCondition->GetAttributeString(ATTRIB_STATE)->GetValue();
        if (stateName.GetIsEmpty() || stateName.CheckIfIsEqual(state->GetName()))
        {
            mUniqueData->mLastTriggeredType = FUNCTION_ENTER;
        }
    }


    void AnimGraphStateCondition::EventHandler::OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        // check if the state and the anim graph instance are valid and return directly in case one of them is not
        if (state == nullptr || animGraphInstance == nullptr)
        {
            return;
        }

        // get the condition test function type
        const int32 functionIndex = mCondition->GetAttributeFloatAsInt32(ATTRIB_FUNCTION);
        if (functionIndex == FUNCTION_ENTERING)
        {
            const MCore::String& stateName = mCondition->GetAttributeString(ATTRIB_STATE)->GetValue();
            if (stateName.GetIsEmpty() || stateName.CheckIfIsEqual(state->GetName()))
            {
                mUniqueData->mLastTriggeredType = FUNCTION_ENTERING;
            }
        }
    }


    void AnimGraphStateCondition::EventHandler::OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        // check if the state and the anim graph instance are valid and return directly in case one of them is not
        if (state == nullptr || animGraphInstance == nullptr)
        {
            return;
        }

        const MCore::String& stateName = mCondition->GetAttributeString(ATTRIB_STATE)->GetValue();
        if (stateName.GetIsEmpty() || stateName.CheckIfIsEqual(state->GetName()))
        {
            mUniqueData->mLastTriggeredType = FUNCTION_EXIT;
        }
    }


    void AnimGraphStateCondition::EventHandler::OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        // check if the state and the anim graph instance are valid and return directly in case one of them is not
        if (state == nullptr || animGraphInstance == nullptr)
        {
            return;
        }

        const MCore::String& stateName = mCondition->GetAttributeString(ATTRIB_STATE)->GetValue();
        if (stateName.GetIsEmpty() || stateName.CheckIfIsEqual(state->GetName()))
        {
            mUniqueData->mLastTriggeredType = FUNCTION_END;
        }
    }
}   // namespace EMotionFX

