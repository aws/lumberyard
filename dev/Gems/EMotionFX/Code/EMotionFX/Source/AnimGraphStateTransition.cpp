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
#include "AnimGraphStateTransition.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraphEntryNode.h"
#include "AnimGraphExitNode.h"
#include "AnimGraphTransitionCondition.h"
#include "AnimGraphManager.h"
#include "AnimGraphRefCountedData.h"
#include "AnimGraph.h"
#include "EMotionFXManager.h"
#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    AnimGraphStateTransition::AnimGraphStateTransition(AnimGraph* animGraph)
        : AnimGraphObject(animGraph, TYPE_ID)
    {
        if (animGraph)
        {
            animGraph->AddObject(this);
        }

        mConditions.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_TRANSITIONS);
        mIsWildcardTransition   = false;
        mID                     = MCore::GetIDGenerator().GenerateID();
        mSourceNode             = nullptr;
        mTargetNode             = nullptr;

        CreateAttributeValues();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    AnimGraphStateTransition::~AnimGraphStateTransition()
    {
        RemoveAllConditions(true);
        if (mAnimGraph)
        {
            mAnimGraph->RemoveObject(this);
        }
    }


    // create
    AnimGraphStateTransition* AnimGraphStateTransition::Create(AnimGraph* animGraph)
    {
        return new AnimGraphStateTransition(animGraph);
    }


    // create unique data
    AnimGraphObjectData* AnimGraphStateTransition::CreateObjectData()
    {
        return new UniqueData(this, nullptr, nullptr);
    }


    // register parameters
    void AnimGraphStateTransition::RegisterAttributes()
    {
        // is the state disabled?
        MCore::AttributeSettings* param = RegisterAttribute("Disabled", "isDisabled", "Is disabled? If yes the transition will not be used by the state machine.", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        param->SetFlag(MCore::AttributeSettings::FLAGINDEX_REINITGUI_ONVALUECHANGE, true);
        param->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // the transition priority value
        param = RegisterAttribute("Priority", "priority", "The priority level of the transition.", MCore::ATTRIBUTE_INTERFACETYPE_INTSPINNER);
        param->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        param->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        param->SetMaxValue(MCore::AttributeFloat::Create(1000000.0f));

        // can the transition be interrupted while already transitioning?
        param = RegisterAttribute("Can Be Interrupted By Others", "canBeInterrupted", "Can be interrupted? If enabled the transition can be interrupted by other transitions, while it is already transitioning.", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        param->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // can this transition interrupt other, already transitioning transitions?
        param = RegisterAttribute("Can Interrupt Other Transitions", "canInterruptOthers", "Can interrupt other transitions? If enabled the transition can be activated while another one is already transitioning.", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        param->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // can this transition interrupt itself?
        param = RegisterAttribute("Allow Self Interruption", "allowSelfInterruption", "Can interrupt itself? If enabled the transition can interrupt and restart itself.", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        param->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // used for wildcard transitions only and specifies to which node groups the transition will actually trigger
        param = RegisterAttribute("Allow Transitions From", "allowTransitionsFrom", "States and groups of states from which the wildcard transition can get activated.", ATTRIBUTE_INTERFACETYPE_STATEFILTERLOCAL);
        param->SetDefaultValue(EMotionFX::AttributeStateFilterLocal::Create());

        // the transition blend time
        param = RegisterAttribute("Transition Time", "blendTime", "The transition time, in seconds.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        param->SetDefaultValue(MCore::AttributeFloat::Create(0.3f));
        param->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        param->SetMaxValue(MCore::AttributeFloat::Create(1000000.0f));

        // sync setting
        param = RegisterSyncAttribute();
        static_cast<MCore::AttributeFloat*>(param->GetDefaultValue())->SetValue(SYNCMODE_DISABLED);

        // the event filtering mode
        param = RegisterEventFilterAttribute();
        static_cast<MCore::AttributeFloat*>(param->GetDefaultValue())->SetValue(EVENTMODE_BOTHNODES);

        // the interpolation type
        MCore::AttributeSettings* interpolationParam = RegisterAttribute("Interpolation", "interpolation", "The interpolation type to use.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        interpolationParam->ResizeComboValues(2);
        interpolationParam->SetComboValue(INTERPOLATIONFUNCTION_LINEAR,     "Linear");
        interpolationParam->SetComboValue(INTERPOLATIONFUNCTION_EASECURVE,  "Ease Curve");
        interpolationParam->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // the sectioned ease curve ease-in point
        param = RegisterAttribute("Ease-In Smoothness", "easeInSmooth", "The smoothness of the ease-in, where 0 means linear and 1 means fully smooth.\nInterpolation type has to be Ease Curve.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        param->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        param->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        param->SetMaxValue(MCore::AttributeFloat::Create(1.0f));

        // the sectioned ease curve ease-out point
        param = RegisterAttribute("Ease-Out Smoothness", "easeOutSmooth", "The smoothness of the ease-out, where 0 means linear and 1 means fully smooth.\nInterpolation type has to be Ease Curve.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        param->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));
        param->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        param->SetMaxValue(MCore::AttributeFloat::Create(1.0f));
    }


    // calculate the transition output, this is the main function
    void AnimGraphStateTransition::CalcTransitionOutput(AnimGraphInstance* animGraphInstance, AnimGraphPose& from, AnimGraphPose& to, AnimGraphPose* outputPose) const
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));

        // calculate the blend weight, based on the type of smoothing
        const float weight = uniqueData->mBlendWeight;

        // blend the two poses
        *outputPose = from;
        outputPose->Blend(&to, weight);
    }


    // update the transition
    void AnimGraphStateTransition::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // get the unique data
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));

        // get the blend time for the transition
        float blendTime = GetBlendTime(animGraphInstance);

        // update timers
        uniqueData->mTotalSeconds += timePassedInSeconds;
        if (uniqueData->mTotalSeconds > blendTime)
        {
            uniqueData->mTotalSeconds = blendTime;
            uniqueData->mIsDone = true;
        }
        else
        {
            uniqueData->mIsDone = false;
        }

        // calculate the blend weight
        if (blendTime > MCore::Math::epsilon)
        {
            uniqueData->mBlendProgress = uniqueData->mTotalSeconds / blendTime;
        }
        else
        {
            uniqueData->mBlendProgress = 1.0f;
        }

        uniqueData->mBlendWeight = CalculateWeight(uniqueData->mBlendProgress);
    }


    // motion extraction
    void AnimGraphStateTransition::ExtractMotion(AnimGraphInstance* animGraphInstance, Transform* outTransform, Transform* outTransformMirrored) const
    {
        // get the unique data
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));

        // get the source node, for wildcard transitions use the overwritten source node from the unique data
        AnimGraphNode* sourceNode = GetSourceNode(animGraphInstance);

        // calculate the blend weight, based on the type of smoothing
        const float weight = uniqueData->mBlendWeight;

        // blend between them
        AnimGraphRefCountedData* sourceData = sourceNode->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
        AnimGraphRefCountedData* targetData = mTargetNode->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
        Transform trajectoryDeltaA = sourceData->GetTrajectoryDelta();
        trajectoryDeltaA.Blend(targetData->GetTrajectoryDelta(), weight);
        *outTransform = trajectoryDeltaA;

        Transform trajectoryDeltaAMirrored = sourceData->GetTrajectoryDeltaMirrored();
        trajectoryDeltaAMirrored.Blend(targetData->GetTrajectoryDeltaMirrored(), weight);
        *outTransformMirrored = trajectoryDeltaAMirrored;
    }


    // the transition started
    void AnimGraphStateTransition::OnStartTransition(AnimGraphInstance* animGraphInstance)
    {
        // get the unique data
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));

        uniqueData->mBlendWeight    = 0.0f;
        uniqueData->mIsDone         = false;
        uniqueData->mTotalSeconds   = 0.0f;
        uniqueData->mBlendProgress  = 0.0f;

        mTargetNode->SetSyncIndex(animGraphInstance, MCORE_INVALIDINDEX32);
    }


    // check and return if the transition is transitioning or already done
    bool AnimGraphStateTransition::GetIsDone(AnimGraphInstance* animGraphInstance) const
    {
        // get the unique data and return the is done flag
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        return uniqueData->mIsDone;
    }


    // return the blend weight value
    float AnimGraphStateTransition::GetBlendWeight(AnimGraphInstance* animGraphInstance) const
    {
        // get the unique data and return the is done flag
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        return uniqueData->mBlendWeight;
    }


    // end the transition
    void AnimGraphStateTransition::OnEndTransition(AnimGraphInstance* animGraphInstance)
    {
        // get the unique data
        UniqueData* uniqueData      = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        uniqueData->mBlendWeight    = 1.0f;
        uniqueData->mBlendProgress  = 1.0f;
        uniqueData->mIsDone         = true;
    }


    // convert attributes for backward compatibility
    // this handles attributes that got renamed or who's types have changed during the development progress
    bool AnimGraphStateTransition::ConvertAttribute(uint32 attributeIndex, const MCore::Attribute* attributeToConvert, const MCore::String& attributeName)
    {
        // convert things by the base class
        const bool result = AnimGraphObject::ConvertAttribute(attributeIndex, attributeToConvert, attributeName);

        // if we try to convert the old syncMotions setting
        // we renamed the 'syncMotions' into 'sync' and also changed the type from a bool to integer
        if (attributeName == "sync" && attributeIndex != MCORE_INVALIDINDEX32)
        {
            // if its a boolean
            if (attributeToConvert->GetType() == MCore::AttributeBool::TYPE_ID)
            {
                const ESyncMode syncMode = (static_cast<const MCore::AttributeBool*>(attributeToConvert)->GetValue()) ? SYNCMODE_TRACKBASED : SYNCMODE_DISABLED;
                GetAttributeFloat(attributeIndex)->SetValue(static_cast<float>(syncMode));
                return true;
            }
        }

        return result;
    }


    // clone the node
    AnimGraphObject* AnimGraphStateTransition::Clone(AnimGraph* animGraph)
    {
        // create the clone
        AnimGraphStateTransition* clone = new AnimGraphStateTransition(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // add a new condition
    void AnimGraphStateTransition::AddCondition(AnimGraphTransitionCondition* condition)
    {
        mConditions.Add(condition);
    }


    // pre-create unique data object
    void AnimGraphStateTransition::Init(AnimGraphInstance* animGraphInstance)
    {
        // pre-alloc all conditions
        const uint32 numConditions = mConditions.GetLength();
        for (uint32 i = 0; i < numConditions; ++i)
        {
            mConditions[i]->Init(animGraphInstance);
        }
    }


    // insert a new condition at the given index
    void AnimGraphStateTransition::InsertCondition(AnimGraphTransitionCondition* condition, uint32 index)
    {
        mConditions.Insert(index, condition);
    }


    // reserve space for a given amount of conditions
    void AnimGraphStateTransition::ReserveConditions(uint32 numConditions)
    {
        mConditions.Reserve(numConditions);
    }


    // remove a given condition
    void AnimGraphStateTransition::RemoveCondition(uint32 index, bool delFromMem)
    {
        if (delFromMem)
        {
            mConditions[index]->Destroy();
        }

        mConditions.Remove(index);
    }


    // remove all conditions
    void AnimGraphStateTransition::RemoveAllConditions(bool delFromMem)
    {
        // delete them all from memory
        if (delFromMem)
        {
            const uint32 numConditions = mConditions.GetLength();
            for (uint32 i = 0; i < numConditions; ++i)
            {
                mConditions[i]->Destroy();
            }
        }

        // clear the conditions array
        mConditions.Clear();
    }


    // check if all conditions are tested positive
    bool AnimGraphStateTransition::CheckIfIsReady(AnimGraphInstance* animGraphInstance) const
    {
        // get the number of conditions and return false in case there aren't any
        const uint32 numConditions = mConditions.GetLength();
        if (numConditions == 0)
        {
            return false;
        }

        // if one of the conditions is false, we evaluate to false
    #ifdef EMFX_EMSTUDIOBUILD
        bool isReady = true;
    #endif
        for (uint32 i = 0; i < numConditions; ++i)
        {
            // get the condition, test if this condition is fulfilled and update the previous test result
            AnimGraphTransitionCondition* condition = mConditions[i];
            const bool testResult = condition->TestCondition(animGraphInstance);
            condition->UpdatePreviousTestResult(animGraphInstance, testResult);

            // return directly in case one condition is not ready yet
            if (testResult == false)
        #ifndef EMFX_EMSTUDIOBUILD
            {
                return false;
            }
        #else
            {
                isReady = false;
            }
        #endif
        }

        // all conditions where true
    #ifndef EMFX_EMSTUDIOBUILD
        return true;
    #else
        return isReady;
    #endif
    }


    // set the wildcard flag for the transition
    void AnimGraphStateTransition::SetIsWildcardTransition(bool isWildcardTransition)
    {
        mIsWildcardTransition = isWildcardTransition;
    }


    void AnimGraphStateTransition::SetSourceNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* sourceNode)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        uniqueData->mSourceNode = sourceNode;
    }


    // get the source node of the transition
    AnimGraphNode* AnimGraphStateTransition::GetSourceNode(AnimGraphInstance* animGraphInstance) const
    {
        // return the normal source node in case we are not dealing with a wildcard transition
        if (mIsWildcardTransition == false)
        {
            return mSourceNode;
        }

        // wildcard transition special case handling
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        return uniqueData->mSourceNode;
    }


    // get the blend time
    float AnimGraphStateTransition::GetBlendTime(AnimGraphInstance* animGraphInstance) const
    {
        MCORE_UNUSED(animGraphInstance);

        // just return the blend time from the attribute
        return GetAttributeFloat(ATTRIB_BLENDTIME)->GetValue();
    }


    // callback for when we renamed a node
    void AnimGraphStateTransition::OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const MCore::String& oldName)
    {
        // get the number of conditions, iterate through them and call the callback
        const uint32 numConditions = mConditions.GetLength();
        for (uint32 i = 0; i < numConditions; ++i)
        {
            mConditions[i]->OnRenamedNode(animGraph, node, oldName);
        }
    }


    // callback that gets called after a new node got created
    void AnimGraphStateTransition::OnCreatedNode(AnimGraph* animGraph, AnimGraphNode* node)
    {
        // get the number of conditions, iterate through them and call the callback
        const uint32 numConditions = mConditions.GetLength();
        for (uint32 i = 0; i < numConditions; ++i)
        {
            mConditions[i]->OnCreatedNode(animGraph, node);
        }
    }


    // callback that gets called before a node gets removed
    void AnimGraphStateTransition::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        // get the number of conditions, iterate through them and call the callback
        const uint32 numConditions = mConditions.GetLength();
        for (uint32 i = 0; i < numConditions; ++i)
        {
            mConditions[i]->OnRemoveNode(animGraph, nodeToRemove);
        }
    }


    // update data
    void AnimGraphStateTransition::OnUpdateAttributes()
    {
        // get the number of conditions
        const uint32 numConditions = mConditions.GetLength();
        for (uint32 i = 0; i < numConditions; ++i)
        {
            mConditions[i]->OnUpdateAttributes();
        }

        // disable GUI items that have no influence
    #ifdef EMFX_EMSTUDIOBUILD
        // enable all attributes
        EnableAllAttributes(true);

        // if the interpolation type is linear, disable the ease-in and ease-out values
        if (GetAttributeFloat(ATTRIB_INTERPOLATIONTYPE)->GetValue() == INTERPOLATIONFUNCTION_LINEAR)
        {
            SetAttributeDisabled(ATTRIB_EASEIN_SMOOTH);
            SetAttributeDisabled(ATTRIB_EASEOUT_SMOOTH);
        }

        // enable the allow transitions from link only for wildcard transitions
        if (GetIsWildcardTransition() == false)
        {
            SetAttributeDisabled(ATTRIB_ALLOWEDSTATES);
        }

        // in case the source or the target node is an exit node
        if ((mSourceNode && mSourceNode->GetType() == AnimGraphExitNode::TYPE_ID) ||
            (mTargetNode && mTargetNode->GetType() == AnimGraphExitNode::TYPE_ID))
        {
            SetAttributeDisabled(ATTRIB_CANBEINTERRUPTED);
            SetAttributeDisabled(ATTRIB_CANINTERRUPTOTHERTRANSITIONS);
            SetAttributeDisabled(ATTRIB_ALLOWSELFINTERRUPTION);
            SetAttributeDisabled(ATTRIB_BLENDTIME);
            SetAttributeDisabled(ATTRIB_INTERPOLATIONTYPE);
            SetAttributeDisabled(ATTRIB_EASEIN_SMOOTH);
            SetAttributeDisabled(ATTRIB_EASEOUT_SMOOTH);
            SetAttributeDisabled(ATTRIB_SYNC);
            SetAttributeDisabled(ATTRIB_EVENTMODE);

            // automatically set the blend time to zero in case this transition is connected to an exit state
            float oldBlendTime = GetAttributeFloat(ATTRIB_BLENDTIME)->GetValue();
            GetAttributeFloat(ATTRIB_BLENDTIME)->SetValue(0.0f);
            if (oldBlendTime != 0.0f)
            {
                mAnimGraph->SetDirtyFlag(true);
            }
        }

        // in case the source or the target node is a pass-through node
        if ((mSourceNode && mSourceNode->GetType() == AnimGraphEntryNode::TYPE_ID) ||
            (mTargetNode && mTargetNode->GetType() == AnimGraphEntryNode::TYPE_ID))
        {
            SetAttributeDisabled(ATTRIB_CANBEINTERRUPTED);
            SetAttributeDisabled(ATTRIB_CANINTERRUPTOTHERTRANSITIONS);
            SetAttributeDisabled(ATTRIB_ALLOWSELFINTERRUPTION);
            SetAttributeDisabled(ATTRIB_BLENDTIME);
            SetAttributeDisabled(ATTRIB_INTERPOLATIONTYPE);
            SetAttributeDisabled(ATTRIB_EASEIN_SMOOTH);
            SetAttributeDisabled(ATTRIB_EASEOUT_SMOOTH);
            SetAttributeDisabled(ATTRIB_SYNC);
            SetAttributeDisabled(ATTRIB_EVENTMODE);

            // automatically set the blend time to zero in case this transition is connected to an exit state
            float oldBlendTime = GetAttributeFloat(ATTRIB_BLENDTIME)->GetValue();
            GetAttributeFloat(ATTRIB_BLENDTIME)->SetValue(0.0f);
            if (oldBlendTime != 0.0f)
            {
                mAnimGraph->SetDirtyFlag(true);
            }
        }
    #endif
    }


    // reset all transition conditions
    void AnimGraphStateTransition::ResetConditions(AnimGraphInstance* animGraphInstance)
    {
        // iterate through all conditions and reset them
        const uint32 numConditions = mConditions.GetLength();
        for (uint32 i = 0; i < numConditions; ++i)
        {
            mConditions[i]->Reset(animGraphInstance);
        }
    }


    // get the priority value of the transition
    int32 AnimGraphStateTransition::GetPriority() const
    {
        return GetAttributeFloatAsInt32(ATTRIB_PRIORITY);
    }


    // true in case the transition is disabled, false if not
    bool AnimGraphStateTransition::GetIsDisabled() const
    {
        return GetAttributeFloatAsBool(ATTRIB_DISABLED);
    }


    // set if the transition shall be disabled
    void AnimGraphStateTransition::SetIsDisabled(bool isDisabled)
    {
        GetAttributeFloat(ATTRIB_DISABLED)->SetValue(isDisabled);
    }


    // can be interrupted?
    bool AnimGraphStateTransition::GetCanBeInterrupted() const
    {
        return GetAttributeFloatAsBool(ATTRIB_CANBEINTERRUPTED);
    }


    // can interrupt other transitions?
    bool AnimGraphStateTransition::GetCanInterruptOtherTransitions() const
    {
        return GetAttributeFloatAsBool(ATTRIB_CANINTERRUPTOTHERTRANSITIONS);
    }


    // can this transition interrupt itself?
    bool AnimGraphStateTransition::GetCanInterruptItself() const
    {
        return GetAttributeFloatAsBool(ATTRIB_ALLOWSELFINTERRUPTION);
    }


    // add all sub objects
    void AnimGraphStateTransition::RecursiveCollectObjects(MCore::Array<AnimGraphObject*>& outObjects) const
    {
        // iterate through all conditions
        const uint32 numConditions = mConditions.GetLength();
        for (uint32 i = 0; i < numConditions; ++i)
        {
            mConditions[i]->RecursiveCollectObjects(outObjects);
        }

        outObjects.Add(const_cast<AnimGraphStateTransition*>(this));
    }


    // calculate the blend weight, based on the type of smoothing
    float AnimGraphStateTransition::CalculateWeight(float linearWeight) const
    {
        // pick the right interpolation type
        const uint32 comboIndex = GetAttributeFloatAsUint32(ATTRIB_INTERPOLATIONTYPE);
        switch (comboIndex)
        {
        case INTERPOLATIONFUNCTION_LINEAR:
            return linearWeight;
        case INTERPOLATIONFUNCTION_EASECURVE:
        {
            const float easeInSmooth    = GetAttributeFloat(ATTRIB_EASEIN_SMOOTH)->GetValue();
            const float easeOutSmooth   = GetAttributeFloat(ATTRIB_EASEOUT_SMOOTH)->GetValue();
            return MCore::SampleEaseInOutCurveWithSmoothness(linearWeight, easeInSmooth, easeOutSmooth);
        }

        default:
            MCORE_ASSERT(false);    // should be allowed
        }

        return linearWeight;
    }


    // constructor
    AnimGraphStateTransition::UniqueData::UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance, AnimGraphNode* sourceNode)
        : AnimGraphObjectData(object, animGraphInstance)
    {
        mIsDone             = false;
        mBlendWeight        = 0.0f;
        mTotalSeconds       = 0.0f;
        mBlendProgress      = 0.0f;
        mSourceNode         = sourceNode;
    }


    // when attributes have changed their value
    void AnimGraphStateTransition::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data in case it has already been created
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            //uniqueData = new UniqueData(this, animGraphInstance, nullptr);
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        // pre-alloc all conditions
        const uint32 numConditions = mConditions.GetLength();
        for (uint32 i = 0; i < numConditions; ++i)
        {
            mConditions[i]->OnUpdateUniqueData(animGraphInstance);
        }
    }


    // recursive clone
    AnimGraphObject* AnimGraphStateTransition::RecursiveClone(AnimGraph* animGraph, AnimGraphObject* parentObject)
    {
        MCORE_UNUSED(parentObject);

        AnimGraphObject* transitionClone = Clone(animGraph);
        MCORE_ASSERT(transitionClone->GetBaseType() == AnimGraphStateTransition::BASETYPE_ID);
        AnimGraphStateTransition* result = static_cast<AnimGraphStateTransition*>(transitionClone);
        //  animGraph->AddObject( result );

        // copy easy settings
        result->mStartOffsetX           = mStartOffsetX;
        result->mStartOffsetY           = mStartOffsetY;
        result->mEndOffsetX             = mEndOffsetX;
        result->mEndOffsetY             = mEndOffsetY;
        result->mIsWildcardTransition   = mIsWildcardTransition;

        // copy the nodes, remap them into the new animgraph
        if (mSourceNode)
        {
            result->mSourceNode = animGraph->RecursiveFindNodeByID(mSourceNode->GetID());
            MCORE_ASSERT(result->mSourceNode);
        }

        if (mTargetNode)
        {
            result->mTargetNode = animGraph->RecursiveFindNodeByID(mTargetNode->GetID());
            MCORE_ASSERT(result->mTargetNode);
        }

        // copy attribute values
        const uint32 numAttributes = mAttributeValues.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            result->GetAttribute(i)->InitFrom(mAttributeValues[i]);
        }

        // clone the conditions
        const uint32 numConditions = mConditions.GetLength();
        result->ReserveConditions(numConditions);
        for (uint32 i = 0; i < numConditions; ++i)
        {
            // clone it
            AnimGraphObject* conditionClone = mConditions[i]->RecursiveClone(animGraph, result);
            MCORE_ASSERT(conditionClone->GetBaseType() == AnimGraphTransitionCondition::BASETYPE_ID);
            AnimGraphTransitionCondition* resultCondition = static_cast<AnimGraphTransitionCondition*>(conditionClone);

            // add it to the transition
            result->AddCondition(resultCondition);
        }

        return result;
    }

    void AnimGraphStateTransition::SetID(uint32 id)
    {
        mID = id;
    }


    uint32 AnimGraphStateTransition::GetBaseType() const
    {
        return BASETYPE_ID;
    }


    uint32 AnimGraphStateTransition::GetVisualColor() const
    {
        return MCore::RGBA(150, 150, 150);
    }


    bool AnimGraphStateTransition::GetIsStateTransitionNode() const
    {
        return true;
    }


    const char* AnimGraphStateTransition::GetTypeString() const
    {
        return "AnimGraphDefaultTransition";
    }


    const char* AnimGraphStateTransition::GetPaletteName() const
    {
        return "Transition";
    }


    AnimGraphObject::ECategory AnimGraphStateTransition::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_TRANSITIONS;
    }


    AnimGraphStateTransition::ESyncMode AnimGraphStateTransition::GetSyncMode() const
    {
        return (ESyncMode)((uint32)GetAttributeFloat(ATTRIB_SYNC)->GetValue());
    }


    AnimGraphStateTransition::EEventMode AnimGraphStateTransition::GetEventFilterMode() const
    {
        return (EEventMode)((uint32)GetAttributeFloat(ATTRIB_EVENTMODE)->GetValue());
    }


    void AnimGraphStateTransition::SetSourceNode(AnimGraphNode* node)
    {
        mSourceNode = node;
    }


    void AnimGraphStateTransition::SetTargetNode(AnimGraphNode* node)
    {
        mTargetNode = node;
    }


    void AnimGraphStateTransition::SetVisualOffsets(int32 startX, int32 startY, int32 endX, int32 endY)
    {
        mStartOffsetX   = startX;
        mStartOffsetY   = startY;
        mEndOffsetX     = endX;
        mEndOffsetY     = endY;
    }


    int32 AnimGraphStateTransition::GetVisualStartOffsetX() const
    {
        return mStartOffsetX;
    }


    int32 AnimGraphStateTransition::GetVisualStartOffsetY() const
    {
        return mStartOffsetY;
    }


    int32 AnimGraphStateTransition::GetVisualEndOffsetX() const
    {
        return mEndOffsetX;
    }


    int32 AnimGraphStateTransition::GetVisualEndOffsetY() const
    {
        return mEndOffsetY;
    }


    uint32 AnimGraphStateTransition::FindConditionIndex(AnimGraphTransitionCondition* condition) const
    {
        return mConditions.Find(condition);
    }
}   // namespace EMotionFX
