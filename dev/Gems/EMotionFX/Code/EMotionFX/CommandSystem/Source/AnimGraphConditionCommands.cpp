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
#include <MCore/Source/ReflectionSerializer.h>


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
    bool CommandAnimGraphAddCondition::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
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
        AZStd::string name;
        parameters.GetValue("stateMachineName", this, name);

        // find the node in the anim graph
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeByName(name.c_str());
        if (!node)
        {
            outResult = AZStd::string::format("Cannot find node with name '%s' in anim graph '%s'.", name.c_str(), animGraph->GetFileName());
            return false;
        }

        // check if we are dealing with a state machine node
        if (azrtti_typeid(node) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            outResult = AZStd::string::format("Anim graph node with name '%s' is no state machine.", name.c_str());
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
            outResult = AZStd::string::format("Cannot find transition with id %i.", transitionID);
            return false;
        }

        // get the condition object
        const AZ::Outcome<AZStd::string> conditionTypeString = parameters.GetValueIfExists("conditionType", this);
        const AZ::TypeId conditionType = conditionTypeString.IsSuccess() ? AZ::TypeId::CreateString(conditionTypeString.GetValue().c_str()) : AZ::TypeId::CreateNull();
        EMotionFX::AnimGraphObject* conditionObject = nullptr;
        if (!conditionType.IsNull())
        {
            conditionObject = EMotionFX::AnimGraphObjectFactory::Create(conditionType);
        }
        if (!conditionObject)
        {
            outResult = AZStd::string::format("Condition object invalid. The given transition type is either invalid or no object has been registered with type %s.", conditionType.ToString<AZStd::string>().c_str());
            return false;
        }

        MCORE_ASSERT(azrtti_istypeof<EMotionFX::AnimGraphTransitionCondition>(conditionObject));

        // clone the condition
        EMotionFX::AnimGraphObject*                newConditionObject  = EMotionFX::AnimGraphObjectFactory::Create(azrtti_typeid(conditionObject), animGraph);
        EMotionFX::AnimGraphTransitionCondition*   newCondition        = static_cast<EMotionFX::AnimGraphTransitionCondition*>(newConditionObject);

        // Deserialize the contents directly, else we might be overwriting things in the end.
        if (parameters.CheckIfHasParameter("contents"))
        {
            AZStd::string contents;
            parameters.GetValue("contents", this, &contents);
            MCore::ReflectionSerializer::Deserialize(newCondition, contents);
        }

        // Redo mode
        if (!mOldContents.empty())
        {
            MCore::ReflectionSerializer::Deserialize(newCondition, mOldContents);
        }

        // get the location where to add the new condition
        size_t insertAt = MCORE_INVALIDINDEX32;
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
        outResult = AZStd::to_string(transitionID);
        mOldContents.clear();

        animGraph->Reinit();
        animGraph->UpdateUniqueData();

        return true;
    }


    // undo the command
    bool CommandAnimGraphAddCondition::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", mAnimGraphID);
            return false;
        }

        AZStd::string stateMachineName;
        parameters.GetValue("stateMachineName", this, stateMachineName);

        // find the node in the anim graph
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeByName(stateMachineName.c_str());
        if (node == nullptr)
        {
            outResult = AZStd::string::format("Cannot find node with name '%s' in anim graph '%s'.", stateMachineName.c_str(), animGraph->GetFileName());
            return false;
        }

        // check if we are dealing with a state machine node
        if (azrtti_typeid(node) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            outResult = AZStd::string::format("Anim graph node with name '%s' is no state machine.", stateMachineName.c_str());
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
            outResult = AZStd::string::format("Cannot find transition with id %i.", transitionID);
            return false;
        }

        // get the transition condition
        EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(mOldConditionIndex);

        // store the attributes string for redo
        mOldContents = MCore::ReflectionSerializer::Serialize(condition).GetValue();

        const AZStd::string commandString = AZStd::string::format("AnimGraphRemoveCondition -animGraphID %i -stateMachineName \"%s\" -transitionID %i -conditionIndex %i", animGraph->GetID(), stateMachineName.c_str(), transitionID, mOldConditionIndex);
        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            return false;
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);

        animGraph->Reinit();
        animGraph->UpdateUniqueData();

        return true;
    }


    // init the syntax of the command
    void CommandAnimGraphAddCondition::InitSyntax()
    {
        GetSyntax().ReserveParameters(6);
        GetSyntax().AddRequiredParameter("stateMachineName",    "The name of the state machine.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("conditionType",       "The type id of the transition condition to add.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("transitionID",        "The ID of the transition.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("animGraphID",                 "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("insertAt",                    "The index at which the transition condition will be added.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("contents",                    "The serialized contents of the parameter (in reflected XML).", MCore::CommandSyntax::PARAMTYPE_STRING, "");
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
        mOldConditionType   = AZ::TypeId::CreateNull();
        mOldConditionIndex  = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandAnimGraphRemoveCondition::~CommandAnimGraphRemoveCondition()
    {
    }


    // execute
    bool CommandAnimGraphRemoveCondition::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
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
        AZStd::string name;
        parameters.GetValue("stateMachineName", this, name);

        // find the node in the anim graph
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeByName(name.c_str());
        if (node == nullptr)
        {
            outResult = AZStd::string::format("Cannot find node with name '%s' in anim graph '%s'.", name.c_str(), animGraph->GetFileName());
            return false;
        }

        // check if we are dealing with a state machine node
        if (azrtti_typeid(node) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            outResult = AZStd::string::format("Anim graph node with name '%s' is no state machine.", name.c_str());
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
            outResult = AZStd::string::format("Cannot find transition with id %i.", transitionID);
            return false;
        }

        // get the transition condition
        const uint32 conditionIndex = parameters.GetValueAsInt("conditionIndex", this);
        MCORE_ASSERT(conditionIndex < transition->GetNumConditions());
        EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(conditionIndex);

        // remove all unique datas for the condition
        animGraph->RemoveAllObjectData(condition, true);

        // store information for undo
        mOldConditionType   = azrtti_typeid(condition);
        mOldConditionIndex  = conditionIndex;
        mOldContents        = MCore::ReflectionSerializer::Serialize(condition).GetValue();

        // remove the transition condition
        transition->RemoveCondition(conditionIndex);

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        animGraph->Reinit();
        animGraph->UpdateUniqueData();

        return true;
    }


    // undo the command
    bool CommandAnimGraphRemoveCondition::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", mAnimGraphID);
            return false;
        }

        AZStd::string stateMachineName;
        parameters.GetValue("stateMachineName", this, &stateMachineName);

        const uint32 transitionID = parameters.GetValueAsInt("transitionID", this);

        AZStd::string commandString;
        commandString = AZStd::string::format("AnimGraphAddCondition -animGraphID %i -stateMachineName \"%s\" -transitionID %i -conditionType \"%s\" -insertAt %i -contents {%s}", 
            mAnimGraphID, 
            stateMachineName.c_str(), 
            transitionID, 
            mOldConditionType.ToString<AZStd::string>().c_str(), 
            mOldConditionIndex, 
            mOldContents.c_str());
        if (GetCommandManager()->ExecuteCommandInsideCommand(commandString.c_str(), outResult) == false)
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

    //-------------------------------------------------------------------------------------
    // Adjust a transition condition
    //-------------------------------------------------------------------------------------
    // constructor
    CommandAnimGraphAdjustCondition::CommandAnimGraphAdjustCondition(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphAdjustCondition", orgCommand)
    {
    }


    // destructor
    CommandAnimGraphAdjustCondition::~CommandAnimGraphAdjustCondition()
    {
    }


    // execute
    bool CommandAnimGraphAdjustCondition::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        mAnimGraphID = animGraph->GetID();

        const AZStd::string stateMachineName = parameters.GetValue("stateMachineName", this);

        // find the node in the anim graph
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeByName(stateMachineName.c_str());
        if (!node)
        {
            outResult = AZStd::string::format("Cannot find node with name '%s' in anim graph '%s'.", stateMachineName.c_str(), animGraph->GetFileName());
            return false;
        }

        // check if we are dealing with a state machine node
        if (azrtti_typeid(node) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            outResult = AZStd::string::format("Anim graph node with name '%s' is no state machine.", stateMachineName.c_str());
            return false;
        }

        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(node);
        mStateMachineId = stateMachine->GetId();

        mTransitionId = parameters.GetValueAsInt("transitionID", this);

        EMotionFX::AnimGraphStateTransition* transition = stateMachine->FindTransitionByID(mTransitionId);
        if (!transition)
        {
            outResult = AZStd::string::format("Cannot find transition with id %i.", mTransitionId);
            return false;
        }

        mConditionIndex = parameters.GetValueAsInt("conditionIndex", this);
        AZ_Assert(mConditionIndex < transition->GetNumConditions(), "Expected condition index within the range");
        EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(mConditionIndex);

        AZ::Outcome<AZStd::string> serializedCondition = MCore::ReflectionSerializer::Serialize(condition);
        if (serializedCondition)
        {
            mOldContents.swap(serializedCondition.GetValue());
        }

        // set the attributes from a string
        if (parameters.CheckIfHasParameter("attributesString"))
        {
            AZStd::string attributesString;
            parameters.GetValue("attributesString", this, &attributesString);
            MCore::ReflectionSerializer::Deserialize(condition, MCore::CommandLine(attributesString));
        }

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        animGraph->Reinit();
        animGraph->UpdateUniqueData();

        return true;
    }


    // undo the command
    bool CommandAnimGraphAdjustCondition::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", mAnimGraphID);
            return false;
        }
        
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeById(mStateMachineId);
        AZ_Assert(azrtti_typeid(node) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>(), "Expected a state machine");

        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(node);
        EMotionFX::AnimGraphStateTransition* transition = stateMachine->FindTransitionByID(mTransitionId);
        AZ_Assert(transition, "Expected a valid transition");

        EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(mConditionIndex);
        AZ_Assert(condition, "Expected a valid condition");

        MCore::ReflectionSerializer::Deserialize(condition, mOldContents);

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandAnimGraphAdjustCondition::InitSyntax()
    {
        GetSyntax().ReserveParameters(5);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("stateMachineName", "The name of the state machine.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("transitionID", "The ID of the transition.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("conditionIndex", "The index of the transition condition to remove.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("attributesString", "The condition attributes as string.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    // get the description
    const char* CommandAnimGraphAdjustCondition::GetDescription() const
    {
        return "Remove a transition condition from a state machine transition.";
    }

} // namesapce EMotionFX
