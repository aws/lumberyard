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

#include "AnimGraphConditionCommands.h" 
#include "AnimGraphConnectionCommands.h"

#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>


namespace CommandSystem
{
    void AddCondition(const EMotionFX::AnimGraphStateTransition* transition, const AZ::TypeId& conditionType,
        const AZStd::optional<AZStd::string>& contents, const AZStd::optional<size_t>& insertAt,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        AZStd::string command = AZStd::string::format("%s -%s %i -%s %s -conditionType \"%s\"",
            CommandAddTransitionCondition::s_commandName,
            EMotionFX::ParameterMixinAnimGraphId::s_parameterName, transition->GetAnimGraph()->GetID(),
            EMotionFX::ParameterMixinTransitionId::s_parameterName, transition->GetId().ToString().c_str(),
            conditionType.ToString<AZStd::string>().c_str());

        if (insertAt)
        {
            command += AZStd::string::format(" -insertAt %d", insertAt.value());
        }

        if (contents)
        {
            command += AZStd::string::format(" -contents {");
            command += contents.value();
            command += "}";
        }

        CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);
    }

    AZ_CLASS_ALLOCATOR_IMPL(CommandAddTransitionCondition, EMotionFX::CommandAllocator, 0)
    const char* CommandAddTransitionCondition::s_commandName = "AnimGraphAddCondition";

    CommandAddTransitionCondition::CommandAddTransitionCondition(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , mOldConditionIndex(MCORE_INVALIDINDEX32)
    {
    }

    bool CommandAddTransitionCondition::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(this, outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphStateTransition* transition = GetTransition(animGraph, this, outResult);
        if (!transition)
        {
            return false;
        }

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
        outResult = m_transitionId.ToString().c_str();
        mOldContents.clear();

        newCondition->Reinit();
        animGraph->UpdateUniqueData();

        return true;
    }

    bool CommandAddTransitionCondition::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(this, outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphStateTransition* transition = GetTransition(animGraph, this, outResult);
        if (!transition)
        {
            return false;
        }

        EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(mOldConditionIndex);

        // store the attributes string for redo
        mOldContents = MCore::ReflectionSerializer::Serialize(condition).GetValue();

        RemoveCondition(transition, mOldConditionIndex, /*commandGroup*/nullptr, /*executeInsideCommand*/true);

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }

    void CommandAddTransitionCondition::InitSyntax()
    {
        GetSyntax().ReserveParameters(5);

        MCore::CommandSyntax& syntax = GetSyntax();
        ParameterMixinAnimGraphId::InitSyntax(syntax);
        ParameterMixinTransitionId::InitSyntax(syntax);
        
        GetSyntax().AddRequiredParameter("conditionType",       "The type id of the transition condition to add.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("insertAt",                    "The index at which the transition condition will be added.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("contents",                    "The serialized contents of the parameter (in reflected XML).", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }

    bool CommandAddTransitionCondition::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinAnimGraphId::SetCommandParameters(parameters);
        ParameterMixinTransitionId::SetCommandParameters(parameters);
        return true;
    }

    const char* CommandAddTransitionCondition::GetDescription() const
    {
        return "Add a new a transition condition to a state machine transition.";
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void RemoveCondition(const EMotionFX::AnimGraphStateTransition* transition, const size_t conditionIndex,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        AZStd::string command = AZStd::string::format("%s -%s %i -%s %s -conditionIndex %d",
            CommandRemoveTransitionCondition::s_commandName,
            EMotionFX::ParameterMixinAnimGraphId::s_parameterName, transition->GetAnimGraph()->GetID(),
            EMotionFX::ParameterMixinTransitionId::s_parameterName, transition->GetId().ToString().c_str(),
            conditionIndex);

        CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);
    }

    AZ_CLASS_ALLOCATOR_IMPL(CommandRemoveTransitionCondition, EMotionFX::CommandAllocator, 0)
    const char* CommandRemoveTransitionCondition::s_commandName = "AnimGraphRemoveCondition";

    CommandRemoveTransitionCondition::CommandRemoveTransitionCondition(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
        mOldConditionType   = AZ::TypeId::CreateNull();
        mOldConditionIndex  = MCORE_INVALIDINDEX32;
    }

    bool CommandRemoveTransitionCondition::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(this, outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphStateTransition* transition = GetTransition(animGraph, this, outResult);
        if (!transition)
        {
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

        animGraph->UpdateUniqueData();

        return true;
    }

    bool CommandRemoveTransitionCondition::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(this, outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphStateTransition* transition = GetTransition(animGraph, this, outResult);
        if (!transition)
        {
            return false;
        }

        CommandSystem::AddCondition(transition, mOldConditionType,
            mOldContents, mOldConditionIndex,
            /*commandGroup*/nullptr, /*executeInsideCommand*/true);

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }

    void CommandRemoveTransitionCondition::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);

        MCore::CommandSyntax& syntax = GetSyntax();
        ParameterMixinAnimGraphId::InitSyntax(syntax);
        ParameterMixinTransitionId::InitSyntax(syntax);

        GetSyntax().AddRequiredParameter("conditionIndex", "The index of the transition condition to remove.", MCore::CommandSyntax::PARAMTYPE_INT);
    }

    bool CommandRemoveTransitionCondition::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinAnimGraphId::SetCommandParameters(parameters);
        ParameterMixinTransitionId::SetCommandParameters(parameters);
        return true;
    }

    const char* CommandRemoveTransitionCondition::GetDescription() const
    {
        return "Remove a transition condition from a state machine transition.";
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AZ_CLASS_ALLOCATOR_IMPL(CommandAdjustTransitionCondition, EMotionFX::CommandAllocator, 0)
    const char* CommandAdjustTransitionCondition::s_commandName = "AnimGraphAdjustCondition";

    CommandAdjustTransitionCondition::CommandAdjustTransitionCondition(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
    }

    bool CommandAdjustTransitionCondition::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(this, outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphStateTransition* transition = GetTransition(animGraph, this, outResult);
        if (!transition)
        {
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

        condition->Reinit();
        animGraph->UpdateUniqueData();

        return true;
    }

    bool CommandAdjustTransitionCondition::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(this, outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphStateTransition* transition = GetTransition(animGraph, this, outResult);
        if (!transition)
        {
            return false;
        }

        EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(mConditionIndex);
        AZ_Assert(condition, "Expected a valid condition");

        MCore::ReflectionSerializer::Deserialize(condition, mOldContents);

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }

    void CommandAdjustTransitionCondition::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);

        MCore::CommandSyntax& syntax = GetSyntax();
        ParameterMixinAnimGraphId::InitSyntax(syntax);
        ParameterMixinTransitionId::InitSyntax(syntax);

        GetSyntax().AddRequiredParameter("conditionIndex", "The index of the transition condition to remove.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("attributesString", "The condition attributes as string.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }

    bool CommandAdjustTransitionCondition::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinAnimGraphId::SetCommandParameters(parameters);
        ParameterMixinTransitionId::SetCommandParameters(parameters);
        return true;
    }

    const char* CommandAdjustTransitionCondition::GetDescription() const
    {
        return "Remove a transition condition from a state machine transition.";
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool GetTransitionFromParameter(const MCore::CommandLine& parameters,
        MCore::Command* command,
        EMotionFX::AnimGraph* animGraph,
        EMotionFX::AnimGraphConnectionId& outTransitionId,
        EMotionFX::AnimGraphStateTransition*& outTransition,
        AZStd::string& outResult)
    {
        outTransition = nullptr;
        outTransitionId = EMotionFX::AnimGraphConnectionId::CreateFromString(parameters.GetValue("transitionID", command));
        if (!outTransitionId.IsValid())
        {
            outResult = AZStd::string::format("Cannot get transition. Transition id is invalid.");
            return false;
        }

        outTransition = animGraph->RecursiveFindTransitionById(outTransitionId);
        if (!outTransition)
        {
            outResult = AZStd::string::format("Cannot find transition with id '%s'.", outTransitionId.ToString().c_str());
            return false;
        }

        return true;
    }
} // namesapce EMotionFX