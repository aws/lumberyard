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

#include "AnimGraphParameterGroupCommands.h"
#include <AzFramework/StringFunc/StringFunc.h>
#include "AnimGraphParameterCommands.h"
#include "CommandManager.h"
#include <EMotionFX/Source/AnimGraph.h>
#include <MCore/Source/AttributeSettings.h>
#include <MCore/Source/Random.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/ActorManager.h>


namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // CommandAnimGraphAdjustParameterGroup
    //--------------------------------------------------------------------------------
    CommandAnimGraphAdjustParameterGroup::CommandAnimGraphAdjustParameterGroup(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphAdjustParameterGroup", orgCommand)
    {
    }


    CommandAnimGraphAdjustParameterGroup::~CommandAnimGraphAdjustParameterGroup()
    {
    }


    AZStd::string CommandAnimGraphAdjustParameterGroup::GenerateParameterNameString(EMotionFX::AnimGraph* animGraph, const AZStd::vector<uint32>& parameterIndices)
    {
        if (parameterIndices.empty())
        {
            return AZStd::string();
        }

        AZStd::string result;
        const size_t numParameters = parameterIndices.size();
        for (size_t i = 0; i < numParameters; ++i)
        {
            const MCore::AttributeSettings* parameter = animGraph->GetParameter(parameterIndices[i]);
            if (!parameter)
            {
                continue;
            }

            result += parameter->GetName();
            if (i < numParameters - 1)
            {
                result += ';';
            }
        }

        return result;
    }


    CommandAnimGraphAdjustParameterGroup::Action CommandAnimGraphAdjustParameterGroup::GetAction(const MCore::CommandLine& parameters)
    {
        // Do we want to add new parameters to the group, remove some from it or replace it entirely.
        AZStd::string actionString;
        parameters.GetValue("action", this, actionString);

        if (AzFramework::StringFunc::Equal(actionString.c_str(), "add"))
        {
            return ACTION_ADD;
        }
        else if (AzFramework::StringFunc::Equal(actionString.c_str(), "clear") || AzFramework::StringFunc::Equal(actionString.c_str(), "remove"))
        {
            return ACTION_REMOVE;
        }

        return ACTION_NONE;
    }


    bool CommandAnimGraphAdjustParameterGroup::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // Get the parameter name.
        AZStd::string name;
        parameters.GetValue("name", this, name);
        mOldName = name;

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult.Format("Cannot adjust parameter group. Anim graph id '%d' is not valid.", animGraphID);
            return false;
        }

        // Find the parameter group index and get a pointer to the parameter group.
        const uint32 groupIndex = animGraph->FindParameterGroupIndexByName(name.c_str());
        EMotionFX::AnimGraphParameterGroup* parameterGroup = nullptr;
        if (groupIndex != MCORE_INVALIDINDEX32)
        {
            parameterGroup = animGraph->GetParameterGroup(groupIndex);
        }

        // Set the new name.
        AZStd::string newName;
        parameters.GetValue("newName", this, newName);
        if (!newName.empty() && parameterGroup)
        {
            parameterGroup->SetName(newName.c_str());
        }

        if (parameters.CheckIfHasParameter("parameterNames"))
        {
            // Do we want to add new parameters to the group, remove some from it or replace it entirely.
            const Action action = GetAction(parameters);

            // Get the parameter string and tokenize it.
            AZStd::string parametersString;
            parameters.GetValue("parameterNames", this, parametersString);

            AZStd::vector<AZStd::string> parameterNames;
            AzFramework::StringFunc::Tokenize(parametersString.c_str(), parameterNames, ";", false, true);

            const size_t numParameters = parameterNames.size();
            mOldParameterGroupNames.resize(numParameters);
            
            // Iterate through all parameter names.
            for (size_t i = 0; i < numParameters; ++i)
            {
                const uint32 parameterIndex = animGraph->FindParameterIndex(parameterNames[i].c_str());
                if (parameterIndex == MCORE_INVALIDINDEX32)
                {
                    mOldParameterGroupNames[i].clear();
                    continue;
                }

                // Save the parameter group (for undo) to which the parameter belonged before command execution.
                EMotionFX::AnimGraphParameterGroup* oldParameterGroup = animGraph->FindParameterGroupForParameter(parameterIndex);
                if (oldParameterGroup)
                {
                    mOldParameterGroupNames[i] = oldParameterGroup->GetName();
                }
                else
                {
                    mOldParameterGroupNames[i].clear();
                }

                switch (action)
                {
                    // Add the parameter to the given group.
                    case ACTION_ADD:
                    {
                        // Make sure the parameter is not in any other group.
                        animGraph->RemoveParameterFromAllGroups(parameterIndex);

                        // Add the parameter to the desired group.
                        // Note: If the parameter group is nullptr, the parameter will move back to the Default group.
                        if (parameterGroup)
                        {
                            parameterGroup->AddParameter(parameterIndex);
                        }
                        break;
                    }

                    // Remove the parameter from the parameter group.
                    case ACTION_REMOVE:
                    {
                        if (!parameterGroup || parameterGroup == oldParameterGroup)
                        {
                            oldParameterGroup->RemoveParameter(parameterIndex);
                        }

                        break;
                    }
                }
            }
        }

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


    bool CommandAnimGraphAdjustParameterGroup::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult.Format("Cannot undo adjust parameter group. Anim graph id '%d' is not valid.", animGraphID);
            return false;
        }

        MCore::CommandGroup commandGroup;

        // Undo the group name as first step. All commands afterwards have to use mOldName as group name.
        if (parameters.CheckIfHasParameter("newName"))
        {
            AZStd::string newName;
            parameters.GetValue("newName", this, newName);
            
            const AZStd::string command = AZStd::string::format("AnimGraphAdjustParameterGroup -animGraphID %i  -name \"%s\" -newName \"%s\"",
                animGraph->GetID(), newName.c_str(), mOldName.c_str());

            commandGroup.AddCommandString(command);
        }

        if (parameters.CheckIfHasParameter("parameterNames"))
        {
            // Do we want to add new parameters to the group, remove some from it or replace it entirely.
            const Action action = GetAction(parameters);

            AZStd::string parametersString;
            parameters.GetValue("parameterNames", this, parametersString);

            AZStd::vector<AZStd::string> parameterNames;
            AzFramework::StringFunc::Tokenize(parametersString.c_str(), parameterNames, ";", false, true);

            const size_t parameterCount = parameterNames.size();
            AZ_Assert(parameterCount == mOldParameterGroupNames.size(), "The number of parameter names has to match the saved parameter group info for undo.");

            for (size_t i = 0; i < parameterCount; ++i)
            {
                const AZStd::string& parameterName = parameterNames[i];
                const AZStd::string& oldGroupName = mOldParameterGroupNames[i];

                switch (action)
                {
                    case ACTION_ADD:
                    {
                        if (oldGroupName.empty())
                        {
                            // An empty old group name means that the parameter was in the Default group before, so in this case just remove the parameter from the group.
                            const AZStd::string command = AZStd::string::format("AnimGraphAdjustParameterGroup -animGraphID %i -name \"%s\" -action \"remove\" -parameterNames \"%s\"",
                                animGraph->GetID(), mOldName.c_str(), parameterName.c_str());

                            commandGroup.AddCommandString(command);
                        }
                        else
                        {
                            // Add the parameter to the old group which auto removes it from all other groups.
                            const AZStd::string command = AZStd::string::format("AnimGraphAdjustParameterGroup -animGraphID %i -name \"%s\" -action \"add\" -parameterNames \"%s\"",
                                animGraph->GetID(), oldGroupName.c_str(), parameterName.c_str());

                            commandGroup.AddCommandString(command);
                        }

                        break;
                    }

                    case ACTION_REMOVE:
                    {

                            const AZStd::string command = AZStd::string::format("AnimGraphAdjustParameterGroup -animGraphID %i -name \"%s\" -action \"add\" -parameterNames \"%s\"",
                                animGraph->GetID(), oldGroupName.c_str(), parameterName.c_str());

                            commandGroup.AddCommandString(command);

                        break;
                    }
                }
            }
        }

        // Execute the command group.
        MCore::String result;
        if (!GetCommandManager()->ExecuteCommandGroupInsideCommand(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.AsChar());
        }

        // Set the dirty flag back to the old value.
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphAdjustParameterGroup::InitSyntax()
    {
        GetSyntax().ReserveParameters(5);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the blend set the parameter group belongs to.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("name",            "The name of the parameter group to adjust.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("newName",     "The new name of the parameter group.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("parameterNames",  "A list of parameter names that should be added/removed to/from the parameter group.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("action",          "The action to perform with the parameters passed to the command.", MCore::CommandSyntax::PARAMTYPE_STRING, "select");
    }


    const char* CommandAnimGraphAdjustParameterGroup::GetDescription() const
    {
        return "This command can be used to adjust the parameter groups of the given anim graph.";
    }

    //--------------------------------------------------------------------------------
    // CommandAnimGraphAddParameterGroup
    //--------------------------------------------------------------------------------
    CommandAnimGraphAddParameterGroup::CommandAnimGraphAddParameterGroup(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphAddParameterGroup", orgCommand)
    {
    }

    
    CommandAnimGraphAddParameterGroup::~CommandAnimGraphAddParameterGroup()
    {
    }


    bool CommandAnimGraphAddParameterGroup::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult.Format("Cannot add parameter group. Anim graph id '%d' is not valid.", animGraphID);
            return false;
        }

        MCore::String valueString;
        if (parameters.CheckIfHasParameter("name"))
        {
            parameters.GetValue("name", this, &valueString);
        }
        else
        {
            // generate a unique parameter name
            valueString.GenerateUniqueString("Parameter",   [&](const MCore::String& value)
                {
                    return (animGraph->FindParameterGroupByName(value.AsChar()) == nullptr);
                });
        }

        // add new parameter group to the anim graph
        EMotionFX::AnimGraphParameterGroup* parameterGroup = EMotionFX::AnimGraphParameterGroup::Create(valueString.AsChar());

        // if the index parameter is specified and valid insert the parameter group at the given position, else just add it to the end
        const uint32 index = parameters.GetValueAsInt("index", this);
        if (index != MCORE_INVALIDINDEX32)
        {
            const uint32 parameterGroupCount = animGraph->GetNumParameterGroups();
            if (index < parameterGroupCount)
            {
                animGraph->InsertParameterGroup(parameterGroup, index);
            }
            else
            {
                animGraph->AddParameterGroup(parameterGroup);
            }
        }
        else
        {
            animGraph->AddParameterGroup(parameterGroup);
        }

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag   = animGraph->GetDirtyFlag();
        mOldName        = parameterGroup->GetName();
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


    bool CommandAnimGraphAddParameterGroup::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult.Format("Cannot undo add parameter group. Anim graph id '%d' is not valid.", animGraphID);
            return false;
        }

        // Construct and execute the command.
        const AZStd::string command = AZStd::string::format("AnimGraphRemoveParameterGroup -animGraphID %i -name \"%s\"", animGraphID, mOldName.c_str());

        MCore::String result;
        if (!GetCommandManager()->ExecuteCommandInsideCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.AsChar());
        }

        // Set the dirty flag back to the old value.
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphAddParameterGroup::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the blend set the parameter group belongs to.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("name", "The name of the parameter group.", MCore::CommandSyntax::PARAMTYPE_STRING, "Unnamed Parameter Group");
        GetSyntax().AddParameter("index", "The index position where the new parameter group should be added to.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
    }


    const char* CommandAnimGraphAddParameterGroup::GetDescription() const
    {
        return "This command can be used to add a new parameter group to the given anim graph.";
    }

    //--------------------------------------------------------------------------------
    // CommandAnimGraphRemoveParameterGroup
    //--------------------------------------------------------------------------------
    CommandAnimGraphRemoveParameterGroup::CommandAnimGraphRemoveParameterGroup(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphRemoveParameterGroup", orgCommand)
    {
    }


    CommandAnimGraphRemoveParameterGroup::~CommandAnimGraphRemoveParameterGroup()
    {
    }


    bool CommandAnimGraphRemoveParameterGroup::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // Get the parameter name.
        AZStd::string name;
        parameters.GetValue("name", this, name);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult.Format("Cannot remove parameter group. Anim graph id '%d' is not valid.", animGraphID);
            return false;
        }

        // find the parameter group index and remove it
        const uint32 groupIndex = animGraph->FindParameterGroupIndexByName(name.c_str());
        if (groupIndex == MCORE_INVALIDINDEX32)
        {
            outResult.Format("Cannot add parameter group to anim graph. Parameter group index is invalid.", groupIndex);
            return false;
        }

        // read out information for the command undo
        EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->GetParameterGroup(groupIndex);
        if (parameterGroup)
        {
            mOldName = parameterGroup->GetName();
        }
        mOldIndex = groupIndex;
        mOldParameterNames.clear();
        if (parameterGroup)
        {
            mOldParameterNames = CommandAnimGraphAdjustParameterGroup::GenerateParameterNameString(animGraph, parameterGroup->GetParameterArray());
        }

        // remove the parameter group
        animGraph->RemoveParameterGroup(groupIndex);

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


    bool CommandAnimGraphRemoveParameterGroup::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult.Format("Cannot undo remove parameter group. Anim graph id '%d' is not valid.", animGraphID);
            return false;
        }

        // Construct the command group.
        AZStd::string command;
        MCore::CommandGroup commandGroup;

        command = AZStd::string::format("AnimGraphAddParameterGroup -animGraphID %i -name \"%s\" -index %d", animGraph->GetID(), mOldName.c_str(), mOldIndex);
        commandGroup.AddCommandString(command);

        command = AZStd::string::format("AnimGraphAdjustParameterGroup -animGraphID %i -name \"%s\" -parameterNames \"%s\" -action \"add\"", animGraph->GetID(), mOldName.c_str(), mOldParameterNames.c_str());
        commandGroup.AddCommandString(command);

        // Execute the command group.
        MCore::String result;
        if (!GetCommandManager()->ExecuteCommandGroupInsideCommand(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.AsChar());
        }

        // Set the dirty flag back to the old value.
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphRemoveParameterGroup::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the blend set the parameter group belongs to.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name", "The name of the parameter group to remove.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    const char* CommandAnimGraphRemoveParameterGroup::GetDescription() const
    {
        return "This command can be used to remove a parameter group from the given anim graph.";
    }


    //--------------------------------------------------------------------------------
    // Helper functions
    //--------------------------------------------------------------------------------
    void RemoveParameterGroup(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphParameterGroup* parameterGroup, bool removeParameters, MCore::CommandGroup* commandGroup)
    {
        // Create the command group and construct the remove parameter group command.
        MCore::CommandGroup internalCommandGroup("Remove parameter group");
        const AZStd::string command = AZStd::string::format("AnimGraphRemoveParameterGroup -animGraphID %i -name \"%s\"", animGraph->GetID(), parameterGroup->GetName());

        if (!commandGroup)
        {
            internalCommandGroup.AddCommandString(command.c_str());
        }
        else
        {
            commandGroup->AddCommandString(command.c_str());
        }

        if (removeParameters)
        {
            AZStd::vector<AZStd::string> parameterNamesToBeRemoved;

            // Add all parameters from the group to the array of parameters to be removed.
            const uint32 numParameters = parameterGroup->GetNumParameters();
            for (uint32 i = 0; i < numParameters; ++i)
            {
                MCore::AttributeSettings* parameter = animGraph->GetParameter(parameterGroup->GetParameter(i));
                parameterNamesToBeRemoved.push_back(parameter->GetName());
            }

            // Construct remove parameter commands for all elements in the array.
            if (!commandGroup)
            {
                RemoveParametersCommand(animGraph, parameterNamesToBeRemoved, &internalCommandGroup);
            }
            else
            {
                RemoveParametersCommand(animGraph, parameterNamesToBeRemoved, commandGroup);
            }
        }

        // Execute the command group.
        if (!commandGroup)
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }


    void ClearParameterGroups(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup)
    {
        const uint32 numParameterGroups = animGraph->GetNumParameterGroups();
        if (numParameterGroups == 0)
        {
            return;
        }

        MCore::CommandGroup internalCommandGroup("Clear parameter groups");

        // Construct remove parameter group commands for all groups and add them to the command group.
        for (uint32 i = 0; i < numParameterGroups; ++i)
        {
            EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->GetParameterGroup(i);

            if (commandGroup)
            {
                RemoveParameterGroup(animGraph, parameterGroup, false, &internalCommandGroup);
            }
            else
            {
                RemoveParameterGroup(animGraph, parameterGroup, false, commandGroup);
            }
        }

        // Execute the command group.
        if (!commandGroup)
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }


    void MoveParameterGroupCommand(EMotionFX::AnimGraph* animGraph, uint32 moveFrom, uint32 moveTo)
    {
        // Get the parameter group to move.
        EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->GetParameterGroup(moveFrom);

        MCore::CommandGroup commandGroup("Move command group");

        // 1: Remove the parameter group that we want to move.
        AZStd::string command;
        command = AZStd::string::format("AnimGraphRemoveParameterGroup -animGraphID %i -name \"%s\"", animGraph->GetID(), parameterGroup->GetName());
        commandGroup.AddCommandString(command);

        // 2: Add a new parameter group at the desired position.
        command = AZStd::string::format("AnimGraphAddParameterGroup -animGraphID %i -name \"%s\" -index %d", animGraph->GetID(), parameterGroup->GetName(), moveTo);
        commandGroup.AddCommandString(command);

        // 3: Move all parameters to the new group.
        AZStd::string parameterNamesString = CommandAnimGraphAdjustParameterGroup::GenerateParameterNameString(animGraph, parameterGroup->GetParameterArray());
        command = AZStd::string::format("AnimGraphAdjustParameterGroup -animGraphID %i -name \"%s\" -parameterNames \"%s\" -action \"add\"", animGraph->GetID(), parameterGroup->GetName(), parameterNamesString.c_str());
        commandGroup.AddCommandString(command);

        // Execute the command group.
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }
} // namespace CommandSystem