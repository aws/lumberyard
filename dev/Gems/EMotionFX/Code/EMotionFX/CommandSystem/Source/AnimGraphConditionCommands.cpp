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
#include "AnimGraphConditionCommands.h"
#include "AnimGraphConnectionCommands.h"
#include "CommandManager.h"

#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/ActorManager.h>


namespace CommandSystem
{
    //-------------------------------------------------------------------------------------
    // add a transition condition
    //-------------------------------------------------------------------------------------
    // constructor
    CommandAnimGraphAddCondition::CommandAnimGraphAddCondition(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphAddCondition", orgCommand)
    {
        mOldConditionIndex = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandAnimGraphAddCondition::~CommandAnimGraphAddCondition()
    {
    }


    // execute
    bool CommandAnimGraphAddCondition::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        mAnimGraphID = animGraph->GetID();

        // get the name of the node
        MCore::String name;
        parameters.GetValue("stateMachineName", this, &name);

        // find the node in the anim graph
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNode(name.AsChar());
        if (node == nullptr)
        {
            outResult.Format("Cannot find node with name '%s' in anim graph '%s'.", name.AsChar(), animGraph->GetName());
            return false;
        }

        // check if we are dealing with a state machine node
        if (node->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            outResult.Format("Anim graph node with name '%s' is no state machine.", name.AsChar());
            return false;
        }

        // convert the node into a state machine
        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(node);

        // get the transition from the parameter list
        const uint32 transitionID = parameters.GetValueAsInt("transitionID", this);
        if (transitionID == MCORE_INVALIDINDEX32)
        {
            outResult = "Invalid transition id specified.";
            return false;
        }

        // find the transition by its id and check if the result is valid
        EMotionFX::AnimGraphStateTransition* transition = stateMachine->FindTransitionByID(transitionID);
        if (transition == nullptr)
        {
            outResult.Format("Cannot find transition with id %i.", transitionID);
            return false;
        }

        // get the condition object
        const uint32                    conditionType   = parameters.GetValueAsInt("conditionType", this);
        EMotionFX::AnimGraphObject*    conditionObject = EMotionFX::GetAnimGraphManager().GetObjectFactory()->GetRegisteredObject(conditionType);
        if (conditionObject == nullptr)
        {
            outResult.Format("Condition object invalid. The given transition type is either invalid or no object has been registered with type %i.", conditionType);
            return false;
        }

        MCORE_ASSERT(conditionObject->GetBaseType() == EMotionFX::AnimGraphTransitionCondition::BASETYPE_ID);

        // clone the condition
        EMotionFX::AnimGraphObject*                newConditionObject  = EMotionFX::GetAnimGraphManager().GetObjectFactory()->CreateObject(animGraph, conditionObject);
        EMotionFX::AnimGraphTransitionCondition*   newCondition        = static_cast<EMotionFX::AnimGraphTransitionCondition*>(newConditionObject);

        // set the attributes from a string
        if (parameters.CheckIfHasParameter("attributesString"))
        {
            MCore::String attributesString;
            parameters.GetValue("attributesString", this, &attributesString);
            newCondition->InitAttributesFromString(attributesString.AsChar());
        }

        // if we are in redo mode
        if (mOldAttributesString.GetIsEmpty() == false)
        {
            newCondition->InitAttributesFromString(mOldAttributesString.AsChar());
        }

        // get the location where to add the new condition
        uint32 insertAt = MCORE_INVALIDINDEX32;
        if (parameters.CheckIfHasParameter("insertAt"))
        {
            insertAt = parameters.GetValueAsInt("insertAt", this);
        }

        // add it to the transition
        if (insertAt == MCORE_INVALIDINDEX32)
        {
            transition->AddCondition(newCondition);
        }
        else
        {
            transition->InsertCondition(newCondition, insertAt);
        }

        // store information for undo
        mOldConditionIndex = transition->FindConditionIndex(newCondition);

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        // set the command result to the transition id and return success
        //outResult.Format("%i", transitionID);
        outResult = MCore::String(transitionID);
        mOldAttributesString.Clear();

        // recursively update attributes of all nodes
        animGraph->RecursiveUpdateAttributes();

        // update unique datas recursively for all anim graph instances
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetActorManager().GetActorInstance(i)->GetAnimGraphInstance();
            if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
            {
                animGraphInstance->OnUpdateUniqueData();
            }
        }

        return true;
    }


    // undo the command
    bool CommandAnimGraphAddCondition::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult.Format("The anim graph with id '%i' does not exist anymore.", mAnimGraphID);
            return false;
        }

        MCore::String stateMachineName;
        parameters.GetValue("stateMachineName", this, &stateMachineName);

        // find the node in the anim graph
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNode(stateMachineName.AsChar());
        if (node == nullptr)
        {
            outResult.Format("Cannot find node with name '%s' in anim graph '%s'.", stateMachineName.AsChar(), animGraph->GetName());
            return false;
        }

        // check if we are dealing with a state machine node
        if (node->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            outResult.Format("Anim graph node with name '%s' is no state machine.", stateMachineName.AsChar());
            return false;
        }

        // convert the node into a state machine
        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(node);

        // get the transition id
        const uint32 transitionID = parameters.GetValueAsInt("transitionID", this);

        // find the transition by its id and check if the result is valid
        EMotionFX::AnimGraphStateTransition* transition = stateMachine->FindTransitionByID(transitionID);
        if (transition == nullptr)
        {
            outResult.Format("Cannot find transition with id %i.", transitionID);
            return false;
        }

        // get the transition condition
        EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(mOldConditionIndex);

        // store the attributes string for redo
        mOldAttributesString = condition->CreateAttributesString();

        MCore::String commandString;
        commandString.Format("AnimGraphRemoveCondition -animGraphID %i -stateMachineName \"%s\" -transitionID %i -conditionIndex %i", animGraph->GetID(), stateMachineName.AsChar(), transitionID, mOldConditionIndex);
        if (GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult) == false)
        {
            return false;
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);

        // recursively update attributes of all nodes
        animGraph->RecursiveUpdateAttributes();

        // update unique datas recursively for all anim graph instances
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetActorManager().GetActorInstance(i)->GetAnimGraphInstance();
            if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
            {
                animGraphInstance->OnUpdateUniqueData();
            }
        }

        return true;
    }


    // init the syntax of the command
    void CommandAnimGraphAddCondition::InitSyntax()
    {
        GetSyntax().ReserveParameters(6);
        GetSyntax().AddRequiredParameter("stateMachineName",    "The name of the state machine.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("conditionType",       "The type id of the transition condition to add.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("transitionID",        "The ID of the transition.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("animGraphID",                "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("insertAt",                    "The index at which the transition condition will be added.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("attributesString",            "The node attributes as string.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    // get the description
    const char* CommandAnimGraphAddCondition::GetDescription() const
    {
        return "Add a new a transition condition to a state machine transition.";
    }


    //-------------------------------------------------------------------------------------
    // Remove a transition condition
    //-------------------------------------------------------------------------------------
    // constructor
    CommandAnimGraphRemoveCondition::CommandAnimGraphRemoveCondition(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphRemoveCondition", orgCommand)
    {
        mOldConditionType   = MCORE_INVALIDINDEX32;
        mOldConditionIndex  = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandAnimGraphRemoveCondition::~CommandAnimGraphRemoveCondition()
    {
    }


    // execute
    bool CommandAnimGraphRemoveCondition::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        mAnimGraphID = animGraph->GetID();

        // get the name of the node
        MCore::String name;
        parameters.GetValue("stateMachineName", this, &name);

        // find the node in the anim graph
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNode(name.AsChar());
        if (node == nullptr)
        {
            outResult.Format("Cannot find node with name '%s' in anim graph '%s'.", name.AsChar(), animGraph->GetName());
            return false;
        }

        // check if we are dealing with a state machine node
        if (node->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            outResult.Format("Anim graph node with name '%s' is no state machine.", name.AsChar());
            return false;
        }

        // convert the node into a state machine
        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(node);

        // get the transition id
        const uint32 transitionID = parameters.GetValueAsInt("transitionID", this);

        // find the transition by its id and check if the result is valid
        EMotionFX::AnimGraphStateTransition* transition = stateMachine->FindTransitionByID(transitionID);
        if (transition == nullptr)
        {
            outResult.Format("Cannot find transition with id %i.", transitionID);
            return false;
        }

        // get the transition condition
        const uint32 conditionIndex = parameters.GetValueAsInt("conditionIndex", this);
        MCORE_ASSERT(conditionIndex < transition->GetNumConditions());
        EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(conditionIndex);

        // remove all unique datas for the condition
        animGraph->RemoveAllObjectData(condition, true);

        // store information for undo
        mOldConditionType   = EMotionFX::GetAnimGraphManager().GetObjectFactory()->FindRegisteredObjectByTypeID(condition->GetType());
        mOldConditionIndex  = conditionIndex;
        mOldAttributesString = condition->CreateAttributesString();

        // remove the transition condition
        transition->RemoveCondition(conditionIndex);

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        // recursively update attributes of all nodes
        animGraph->RecursiveUpdateAttributes();

        // update unique datas recursively for all anim graph instances
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetActorManager().GetActorInstance(i)->GetAnimGraphInstance();
            if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
            {
                animGraphInstance->OnUpdateUniqueData();
            }
        }

        return true;
    }


    // undo the command
    bool CommandAnimGraphRemoveCondition::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult.Format("The anim graph with id '%i' does not exist anymore.", mAnimGraphID);
            return false;
        }

        MCore::String stateMachineName;
        parameters.GetValue("stateMachineName", this, &stateMachineName);

        const uint32 transitionID = parameters.GetValueAsInt("transitionID", this);

        MCore::String commandString;
        commandString.Format("AnimGraphAddCondition -animGraphID %i -stateMachineName \"%s\" -transitionID %i -conditionType %i -insertAt %i -attributesString \"%s\"", mAnimGraphID, stateMachineName.AsChar(), transitionID, mOldConditionType, mOldConditionIndex, mOldAttributesString.AsChar());
        if (GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult) == false)
        {
            return false;
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandAnimGraphRemoveCondition::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);
        GetSyntax().AddRequiredParameter("animGraphID",        "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("stateMachineName",    "The name of the state machine.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("transitionID",        "The ID of the transition.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("conditionIndex",      "The index of the transition condition to remove.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    // get the description
    const char* CommandAnimGraphRemoveCondition::GetDescription() const
    {
        return "Remove a transition condition from a state machine transition.";
    }
} // namesapce EMotionFX
