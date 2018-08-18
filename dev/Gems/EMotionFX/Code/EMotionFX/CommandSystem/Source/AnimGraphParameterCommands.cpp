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

#include "AnimGraphParameterCommands.h"
#include "AnimGraphConnectionCommands.h"
#include "CommandManager.h"

#include <AzCore/std/sort.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphParameterCondition.h>
#include <EMotionFX/Source/AnimGraphTagCondition.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/Parameter/Parameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <MCore/Source/AttributeFactory.h>
#include <MCore/Source/AttributePool.h>
#include <MCore/Source/ReflectionSerializer.h>

namespace CommandSystem
{
    //-------------------------------------------------------------------------------------
    // Create a anim graph parameter
    //-------------------------------------------------------------------------------------
    CommandAnimGraphCreateParameter::CommandAnimGraphCreateParameter(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphCreateParameter", orgCommand)
    {
    }


    CommandAnimGraphCreateParameter::~CommandAnimGraphCreateParameter()
    {
    }


    bool CommandAnimGraphCreateParameter::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Get the parameter name.
        AZStd::string name;
        parameters.GetValue("name", this, name);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot add parameter '%s' to anim graph. Anim graph id '%i' is not valid.", name.c_str(), animGraphID);
            return false;
        }

        // Check if the parameter name is unique and not used by any other parameter yet.
        if (animGraph->FindParameterByName(name))
        {
            outResult = AZStd::string::format("There is already a parameter with the name '%s', please use a another, unique name.", name.c_str());
            return false;
        }

        // Get the data type and check if it is a valid one.
        AZ::Outcome<AZStd::string> valueString = parameters.GetValueIfExists("type", this);
        if (!valueString.IsSuccess())
        {
            outResult = AZStd::string::format("The type was not specified. Please use -help or use the command browser to see a list of valid options.");
            return false;
        }
        const AZ::TypeId parameterType(valueString.GetValue().c_str(), valueString.GetValue().size());

        // Create the new parameter based on the dialog settings.
        AZStd::unique_ptr<EMotionFX::Parameter> newParam(EMotionFX::ParameterFactory::Create(parameterType));

        if (!newParam)
        {
            outResult = AZStd::string::format("Could not construct parameter '%s'", name.c_str());
            return false;
        }

        newParam->SetName(name);

        // Description.
        AZStd::string description;
        parameters.GetValue("description", this, description);
        newParam->SetDescription(description);

        valueString = parameters.GetValueIfExists("minValue", this);
        if (valueString.IsSuccess())
        {
            if (!MCore::ReflectionSerializer::DeserializeIntoMember(newParam.get(), "minValue", valueString.GetValue()))
            {
                outResult = AZStd::string::format("Failed to initialize minimum value from string '%s'", valueString.GetValue().c_str());
                return false;
            }
        }
        valueString = parameters.GetValueIfExists("maxValue", this);
        if (valueString.IsSuccess())
        {
            if (!MCore::ReflectionSerializer::DeserializeIntoMember(newParam.get(), "maxValue", valueString.GetValue()))
            {
                outResult = AZStd::string::format("Failed to initialize maximum value from string '%s'", valueString.GetValue().c_str());
                return false;
            }
        }
        valueString = parameters.GetValueIfExists("defaultValue", this);
        if (valueString.IsSuccess())
        {
            if (!MCore::ReflectionSerializer::DeserializeIntoMember(newParam.get(), "defaultValue", valueString.GetValue()))
            {
                outResult = AZStd::string::format("Failed to initialize default value from string '%s'", valueString.GetValue().c_str());
                return false;
            }
        }
        valueString = parameters.GetValueIfExists("contents", this);
        if (valueString.IsSuccess())
        {
            MCore::ReflectionSerializer::Deserialize(newParam.get(), valueString.GetValue());
        }

        // Check if the group parameter got specified.
        const EMotionFX::GroupParameter* parentGroupParameter = nullptr;
        valueString = parameters.GetValueIfExists("parent", this);
        if (valueString.IsSuccess())
        {
            // Find the group parameter index and get a pointer to the group parameter.
            parentGroupParameter = animGraph->FindGroupParameterByName(valueString.GetValue());
            if (!parentGroupParameter)
            {
                MCore::LogWarning("The group parameter named '%s' could not be found. The parameter cannot be added to the group.", valueString.GetValue().c_str());
            }
        }

        // The position inside the parameter array where the parameter should get added to.
        const int parameterIndex = parameters.GetValueAsInt("index", this);
        const size_t parentGroupSize = parentGroupParameter ? parentGroupParameter->GetNumParameters() : animGraph->GetNumParameters();
        if (parameterIndex != -1 && (parameterIndex < 0 || parameterIndex > static_cast<int>(parentGroupSize)))
        {
            outResult = AZStd::string::format("Index '%d' out of range.", parameterIndex);
            return false;
        }

        const bool paramResult = parameterIndex == -1
            ? animGraph->AddParameter(newParam.get(), parentGroupParameter)
            : animGraph->InsertParameter(parameterIndex, newParam.get(), parentGroupParameter);

        if (!paramResult)
        {
            outResult = AZStd::string::format("Could not add parameter: '%s.'", newParam->GetName().c_str());
            return false;
        }

        if (azrtti_typeid(newParam.get()) != azrtti_typeid<EMotionFX::GroupParameter>())
        {
            const AZ::Outcome<size_t> valueParameterIndex = animGraph->FindValueParameterIndex(static_cast<const EMotionFX::ValueParameter*>(newParam.get()));
            AZ_Assert(valueParameterIndex.IsSuccess(), "Expected a valid value parameter index");

            // Update all anim graph instances.
            const size_t numInstances = EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances();
            for (size_t i = 0; i < numInstances; ++i)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i);

                // Only update anim graph instances using the given anim graph.
                if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
                {
                    animGraphInstance->InsertParameterValue(static_cast<uint32>(valueParameterIndex.GetValue()));
                }
            }

            // Get an array of all parameter nodes.
            MCore::Array<EMotionFX::AnimGraphNode*> parameterNodes;
            parameterNodes.Reserve(8);
            animGraph->RecursiveCollectNodesOfType(azrtti_typeid<EMotionFX::BlendTreeParameterNode>(), &parameterNodes);

            // Reinit all parameter nodes.
            const uint32 numParamNodes = parameterNodes.GetLength();
            for (uint32 i = 0; i < numParamNodes; ++i)
            {
                EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(parameterNodes[i]);
                parameterNode->Reinit();
            }

            // Set the parameter name as command result.
            outResult = name.c_str();

            // Save the current dirty flag and tell the anim graph that something got changed.
            mOldDirtyFlag = animGraph->GetDirtyFlag();
            animGraph->SetDirtyFlag(true);

            animGraph->Reinit();
            animGraph->UpdateUniqueData();
        }
        newParam.release(); // adding the parameter succeeded, release the memory because it is owned by the AnimGraph

        return true;
    }


    bool CommandAnimGraphCreateParameter::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Get the parameter name.
        AZStd::string name;
        parameters.GetValue("name", this, name);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot undo add parameter '%s' to anim graph. Anim graph id '%i' is not valid.", name.c_str(), animGraphID);
            return false;
        }

        const AZStd::string commandString = AZStd::string::format("AnimGraphRemoveParameter -animGraphID %i -name \"%s\"", animGraph->GetID(), name.c_str());

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // Set the dirty flag back to the old value.
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphCreateParameter::InitSyntax()
    {
        GetSyntax().ReserveParameters(9);
        GetSyntax().AddRequiredParameter("animGraphID",     "The id of the anim graph.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("type",            "The type of this parameter (UUID).", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("name",            "The name of the parameter, which has to be unique inside the currently selected anim graph.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("description",             "The description of the parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("index",                   "The position where the parameter should be added. If the parameter is not specified it will get added to the end. This index is relative to the \"parent\" parameter", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("contents",                "The serialized contents of the parameter (in reflected XML).", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("parent",                  "The parent group name into which the parameter should be added. If not specified it will get added to the root group.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    const char* CommandAnimGraphCreateParameter::GetDescription() const
    {
        return "This command creates a anim graph parameter with given settings. The name of the parameter is returned on success.";
    }

    //-------------------------------------------------------------------------------------
    // Remove a anim graph parameter
    //-------------------------------------------------------------------------------------
    CommandAnimGraphRemoveParameter::CommandAnimGraphRemoveParameter(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphRemoveParameter", orgCommand)
    {
    }


    CommandAnimGraphRemoveParameter::~CommandAnimGraphRemoveParameter()
    {
    }


    bool CommandAnimGraphRemoveParameter::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Get the parameter name.
        parameters.GetValue("name", this, mName);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot remove parameter '%s' from anim graph. Anim graph id '%i' is not valid.", mName.c_str(), animGraphID);
            return false;
        }

        // Check if there is a parameter with the given name.
        const EMotionFX::Parameter* parameter = animGraph->FindParameterByName(mName);
        if (!parameter)
        {
            outResult = AZStd::string::format("Cannot remove parameter '%s' from anim graph. There is no parameter with the given name.", mName.c_str(), animGraphID);
            return false;
        }
        AZ_Assert(azrtti_typeid(parameter) != azrtti_typeid<EMotionFX::GroupParameter>(), "CommmandAnimGraphRemoveParameter called for a group parameter");

        const EMotionFX::GroupParameter* parentGroup = animGraph->FindParentGroupParameter(parameter);

        const AZ::Outcome<size_t> parameterIndex = parentGroup ? parentGroup->FindRelativeParameterIndex(parameter) : animGraph->FindRelativeParameterIndex(parameter);
        AZ_Assert(parameterIndex.IsSuccess(), "Expected valid parameter index");

        // Store undo info before we remove it, so that we can recreate it later.
        mType = azrtti_typeid(parameter);
        mIndex = parameterIndex.GetValue();
        mParent = parentGroup ? parentGroup->GetName() : "";
        mContents = MCore::ReflectionSerializer::Serialize(parameter).GetValue();

        AZ::Outcome<size_t> valueParameterIndex = AZ::Failure();
        if (mType != azrtti_typeid<EMotionFX::GroupParameter>())
        {
            valueParameterIndex = animGraph->FindValueParameterIndex(static_cast<const EMotionFX::ValueParameter*>(parameter));
        }

        // Remove the parameter from the anim graph.
        if (animGraph->RemoveParameter(const_cast<EMotionFX::Parameter*>(parameter)))
        {
            // Remove the parameter from all corresponding anim graph instances if it is a value parameter
            if (mType != azrtti_typeid<EMotionFX::GroupParameter>())
            {
                const size_t numInstances = EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances();
                for (size_t i = 0; i < numInstances; ++i)
                {
                    EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i);
                    if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
                    {
                        // Remove the parameter.
                        animGraphInstance->RemoveParameterValue(static_cast<uint32>(valueParameterIndex.GetValue()));
                    }
                }

                // Get an array of all parameter nodes.
                MCore::Array<EMotionFX::AnimGraphNode*> parameterNodes;
                animGraph->RecursiveCollectNodesOfType(azrtti_typeid<EMotionFX::BlendTreeParameterNode>(), &parameterNodes);

                // Reinit all parameter nodes.
                const uint32 numParamNodes = parameterNodes.GetLength();
                for (uint32 i = 0; i < numParamNodes; ++i)
                {
                    EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(parameterNodes[i]);
                    parameterNode->Reinit();
                }

                // Save the current dirty flag and tell the anim graph that something got changed.
                mOldDirtyFlag = animGraph->GetDirtyFlag();
                animGraph->SetDirtyFlag(true);

                animGraph->Reinit();
                animGraph->UpdateUniqueData();
            }
        }

        return true;
    }


    bool CommandAnimGraphRemoveParameter::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot undo remove parameter '%s' from anim graph. Anim graph id '%i' is not valid.", mName.c_str(), animGraphID);
            return false;
        }

        // Execute the command to create the parameter again.
        AZStd::string commandString;

        commandString = AZStd::string::format("AnimGraphCreateParameter -animGraphID %i -name \"%s\" -index %i -type \"%s\" -contents {%s} -parent \"%s\"",
                animGraph->GetID(),
                mName.c_str(),
                mIndex,
                mType.ToString<AZStd::string>().c_str(),
                mContents.c_str(),
                mParent.c_str());

        // The parameter will be restored to the right parent group because the index is absolute

        // Execute the command.
        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
            return false;
        }

        // Set the dirty flag back to the old value.
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphRemoveParameter::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name", "The name of the parameter inside the currently selected anim graph.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    const char* CommandAnimGraphRemoveParameter::GetDescription() const
    {
        return "This command removes a anim graph parameter with the specified name.";
    }


    //-------------------------------------------------------------------------------------
    // Adjust a anim graph parameter
    //-------------------------------------------------------------------------------------
    CommandAnimGraphAdjustParameter::CommandAnimGraphAdjustParameter(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphAdjustParameter", orgCommand)
    {
    }


    CommandAnimGraphAdjustParameter::~CommandAnimGraphAdjustParameter()
    {
    }


    void CommandAnimGraphAdjustParameter::UpdateTransitionConditions(EMotionFX::AnimGraph* animGraph, const AZStd::string& oldParameterName, const AZStd::string& newParameterName)
    {
        if (animGraph->GetIsOwnedByRuntime())
        {
            return;
        }

        // Collect all parameter transition conditions.
        MCore::Array<EMotionFX::AnimGraphTransitionCondition*> parameterConditions;
        animGraph->RecursiveCollectTransitionConditionsOfType(azrtti_typeid<EMotionFX::AnimGraphParameterCondition>(), &parameterConditions);

        const uint32 numParameterConditions = parameterConditions.GetLength();
        for (uint32 j = 0; j < numParameterConditions; ++j)
        {
            EMotionFX::AnimGraphParameterCondition* parameterCondition = static_cast<EMotionFX::AnimGraphParameterCondition*>(parameterConditions[j]);

            // Adjust all parameter conditions along with the parameter name change.
            if (parameterCondition->GetParameterName() == oldParameterName)
            {
                parameterCondition->SetParameterName(newParameterName);
            }
        }


        // Collect all tag transition conditions.
        MCore::Array<EMotionFX::AnimGraphTransitionCondition*> tagConditions;
        animGraph->RecursiveCollectTransitionConditionsOfType(azrtti_typeid<EMotionFX::AnimGraphTagCondition>(), &tagConditions);

        const uint32 numTagConditions = tagConditions.GetLength();
        for (uint32 j = 0; j < numTagConditions; ++j)
        {
            EMotionFX::AnimGraphTagCondition* tagCondition = static_cast<EMotionFX::AnimGraphTagCondition*>(tagConditions[j]);
            tagCondition->RenameTag(oldParameterName, newParameterName);
        }

        animGraph->Reinit();
    }


    bool CommandAnimGraphAdjustParameter::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Get the parameter name.
        parameters.GetValue("name", this, mOldName);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot adjust parameter '%s'. Anim graph with id '%d' not found.", mOldName.c_str(), animGraphID);
            return false;
        }

        const EMotionFX::Parameter* parameter = animGraph->FindParameterByName(mOldName);
        if (!parameter)
        {
            outResult = AZStd::string::format("There is no parameter with the name '%s'.", mOldName.c_str());
            return false;
        }
        const AZ::Outcome<size_t> parameterIndex = animGraph->FindParameterIndex(parameter);

        const EMotionFX::GroupParameter* currentParent = animGraph->FindParentGroupParameter(parameter);

        // Store the undo info.
        mOldType = azrtti_typeid(parameter);
        mOldContents = MCore::ReflectionSerializer::Serialize(parameter).GetValue();

        // Get the new name and check if it is valid.
        AZStd::string newName;
        parameters.GetValue("newName", this, newName);
        if (!newName.empty())
        {
            if (newName == mOldName)
            {
                newName.clear();
            }
            else if (animGraph->FindParameterByName(newName))
            {
                outResult = AZStd::string::format("There is already a parameter with the name '%s', please use a unique name.", newName.c_str());
                return false;
            }
        }

        // Get the data type and check if it is a valid one.
        bool changedType = false;
        AZ::Outcome<AZStd::string> valueString = parameters.GetValueIfExists("type", this);
        if (valueString.IsSuccess())
        {
            const AZ::TypeId type = AZ::TypeId::CreateString(valueString.GetValue().c_str(), valueString.GetValue().size());
            if (type.IsNull())
            {
                outResult = AZStd::string::format("The type is not a valid UUID type. Please use -help or use the command browser to see a list of valid options.");
                return false;
            }
            if (type != mOldType)
            {
                AZStd::unique_ptr<EMotionFX::Parameter> newParameter(EMotionFX::ParameterFactory::Create(type));
                newParameter->SetName(newName.empty() ? mOldName : newName);
                newParameter->SetDescription(parameter->GetDescription());

                const AZ::Outcome<size_t> paramIndexRelativeToParent = currentParent ? currentParent->FindRelativeParameterIndex(parameter) : animGraph->FindRelativeParameterIndex(parameter);
                AZ_Assert(paramIndexRelativeToParent.IsSuccess(), "Expected parameter to be in the parent");

                if (!animGraph->RemoveParameter(const_cast<EMotionFX::Parameter*>(parameter)))
                {
                    outResult = AZStd::string::format("Could not remove current parameter '%s' to change its type.", mOldName.c_str());
                    return false;
                }
                if (!animGraph->InsertParameter(paramIndexRelativeToParent.GetValue(), newParameter.get(), currentParent))
                {
                    outResult = AZStd::string::format("Could not insert new parameter '%s' to change its type.", newName.c_str());
                    return false;
                }
                parameter = newParameter.release();
                changedType = true;
            }
        }

        EMotionFX::Parameter* mutableParam = const_cast<EMotionFX::Parameter*>(parameter);
        // Get the value strings.
        valueString = parameters.GetValueIfExists("minValue", this);
        if (valueString.IsSuccess())
        {
            MCore::ReflectionSerializer::DeserializeIntoMember(mutableParam, "minValue", valueString.GetValue());
        }
        valueString = parameters.GetValueIfExists("maxValue", this);
        if (valueString.IsSuccess())
        {
            MCore::ReflectionSerializer::DeserializeIntoMember(mutableParam, "maxValue", valueString.GetValue());
        }
        valueString = parameters.GetValueIfExists("defaultValue", this);
        if (valueString.IsSuccess())
        {
            MCore::ReflectionSerializer::DeserializeIntoMember(mutableParam, "defaultValue", valueString.GetValue());
        }
        valueString = parameters.GetValueIfExists("description", this);
        if (valueString.IsSuccess())
        {
            MCore::ReflectionSerializer::DeserializeIntoMember(mutableParam, "description", valueString.GetValue());
        }
        valueString = parameters.GetValueIfExists("contents", this);
        if (valueString.IsSuccess())
        {
            MCore::ReflectionSerializer::Deserialize(mutableParam, valueString.GetValue());
        }

        if (azrtti_istypeof<EMotionFX::ValueParameter>(parameter))
        {
            const EMotionFX::ValueParameter* valueParameter = static_cast<const EMotionFX::ValueParameter*>(parameter);
            const AZ::Outcome<size_t> valueParameterIndex = animGraph->FindValueParameterIndex(valueParameter);
            AZ_Assert(valueParameterIndex.IsSuccess(), "Expect a valid value parameter index");

            // Update all corresponding anim graph instances.
            const size_t numInstances = animGraph->GetNumAnimGraphInstances();
            for (uint32 i = 0; i < numInstances; ++i)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = animGraph->GetAnimGraphInstance(i);
                // reinit the modified parameters
                if (mOldType != azrtti_typeid<EMotionFX::GroupParameter>())
                {
                    animGraphInstance->ReInitParameterValue(static_cast<uint32>(valueParameterIndex.GetValue()));
                }
                else
                {
                    animGraphInstance->AddMissingParameterValues();
                }
            }

            if (!newName.empty())
            {
                // Get an array of all parameter nodes.
                MCore::Array<EMotionFX::AnimGraphNode*> parameterNodes;
                parameterNodes.Reserve(16);
                animGraph->RecursiveCollectNodesOfType(azrtti_typeid<EMotionFX::BlendTreeParameterNode>(), &parameterNodes);

                const uint32 numParamNodes = parameterNodes.GetLength();
                for (uint32 i = 0; i < numParamNodes; ++i)
                {
                    // Type-cast the anim graph node to a parameter node and reinitialize the ports.
                    EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(parameterNodes[i]);

                    // In case we changed the name of a parameter also update the parameter masks.
                    parameterNode->RenameParameterName(mOldName, newName);
                }
            }

            // Apply the name change., only required to do it if the type didn't change
            if (!changedType && !newName.empty())
            {
                RenameParameterNodePorts(animGraph, nullptr, mOldName, newName);

                // Update all transition conditions along with the parameter name change.
                UpdateTransitionConditions(animGraph, mOldName, newName);

                EMotionFX::Parameter* editableParameter = const_cast<EMotionFX::Parameter*>(parameter);
                animGraph->RenameParameter(editableParameter, newName);
            }

            // Save the current dirty flag and tell the anim graph that something got changed.
            mOldDirtyFlag = animGraph->GetDirtyFlag();
            animGraph->SetDirtyFlag(true);

            animGraph->Reinit();
            animGraph->UpdateUniqueData();
        }
        else if (mOldType != azrtti_typeid<EMotionFX::GroupParameter>())
        {
            // Update all corresponding anim graph instances.
            const size_t numInstances = animGraph->GetNumAnimGraphInstances();
            for (uint32 i = 0; i < numInstances; ++i)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = animGraph->GetAnimGraphInstance(i);
                animGraphInstance->RemoveParameterValue(static_cast<uint32>(parameterIndex.GetValue()));
            }
           
            // Get an array of all parameter nodes.
            MCore::Array<EMotionFX::AnimGraphNode*> parameterNodes;
            parameterNodes.Reserve(16);
            animGraph->RecursiveCollectNodesOfType(azrtti_typeid<EMotionFX::BlendTreeParameterNode>(), &parameterNodes);

            const uint32 numParamNodes = parameterNodes.GetLength();
            for (uint32 i = 0; i < numParamNodes; ++i)
            {
                // Type-cast the anim graph node to a parameter node and reinitialize the ports.
                EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(parameterNodes[i]);
                
                // In case we changed the name of a parameter also update the parameter masks.
                parameterNode->RemoveParameterByName(newName);
            }

            // Save the current dirty flag and tell the anim graph that something got changed.
            mOldDirtyFlag = animGraph->GetDirtyFlag();
            animGraph->SetDirtyFlag(true);

            animGraph->Reinit();
            animGraph->UpdateUniqueData();
        }

        return true;
    }


    void CommandAnimGraphAdjustParameter::RenameParameterNodePorts(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* startNode, const AZStd::string& oldName, const AZStd::string& newName)
    {
        // Root state machines.
        if (!startNode)
        {
            EMotionFX::AnimGraphStateMachine* stateMachine = animGraph->GetRootStateMachine();
            const uint32 numChildNodes = stateMachine->GetNumChildNodes();
            for (uint32 i = 0; i < numChildNodes; ++i)
            {
                RenameParameterNodePorts(animGraph, stateMachine->GetChildNode(i), oldName, newName);
            }
        }
        else
        {
            // Update output port name when start node is a blend tree.
            if (azrtti_typeid(startNode) == azrtti_typeid<EMotionFX::BlendTreeParameterNode>())
            {
                EMotionFX::BlendTreeParameterNode* paramNode = static_cast<EMotionFX::BlendTreeParameterNode*>(startNode);
                const uint32 index = paramNode->FindOutputPortIndex(oldName.c_str());
                if (index != MCORE_INVALIDINDEX32)
                {
                    paramNode->SetOutputPortName(index, newName.c_str());
                }
            }

            // recurse
            const uint32 numChildNodes = startNode->GetNumChildNodes();
            for (uint32 i = 0; i < numChildNodes; ++i)
            {
                RenameParameterNodePorts(animGraph, startNode->GetChildNode(i), oldName, newName);
            }
        }
    }


    bool CommandAnimGraphAdjustParameter::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot adjust parameter to anim graph. Anim graph id '%d' is not valid.", animGraphID);
            return false;
        }

        // get the name and check if it is unique
        AZStd::string name;
        AZStd::string newName;
        parameters.GetValue("name", this, name);
        parameters.GetValue("newName", this, newName);

        // Get the parameter index.
        AZ::Outcome<size_t> paramIndex = animGraph->FindParameterIndexByName(name.c_str());
        if (!paramIndex.IsSuccess())
        {
            paramIndex = animGraph->FindParameterIndexByName(newName.c_str());
        }

        // If the neither the former nor the new parameter exist, return.
        if (!paramIndex.IsSuccess())
        {
            if (newName.empty())
            {
                outResult = AZStd::string::format("There is no parameter with the name '%s'.", name.c_str());
            }
            else
            {
                outResult = AZStd::string::format("There is no parameter with the name '%s'.", newName.c_str());
            }

            return false;
        }

        // Construct and execute the command.
        const AZStd::string commandString = AZStd::string::format("AnimGraphAdjustParameter -animGraphID %i -name \"%s\" -newName \"%s\" -type \"%s\" -contents {%s}",
                animGraph->GetID(),
                newName.c_str(),
                name.c_str(),
                mOldType.ToString<AZStd::string>().c_str(),
                mOldContents.c_str());

        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
        }

        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphAdjustParameter::InitSyntax()
    {
        GetSyntax().ReserveParameters(9);
        GetSyntax().AddRequiredParameter("animGraphID",     "The id of the anim graph.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name",            "The name of the parameter inside the currently selected anim graph to modify.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("type",                    "The new type (UUID).", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("newName",                 "The new name of the parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("defaultValue",            "The new default value of the parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("minValue",                "The new minimum value of the parameter. In case of checkboxes or strings this parameter value will be ignored.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("maxValue",                "The new maximum value of the parameter. In case of checkboxes or strings this parameter value will be ignored.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("description",             "The new description of the parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("contents",                "The contents of the parameter (serialized reflected XML)", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    const char* CommandAnimGraphAdjustParameter::GetDescription() const
    {
        return "This command adjusts a anim graph parameter with given settings.";
    }


    //-------------------------------------------------------------------------------------
    // Move a parameter to another position
    //-------------------------------------------------------------------------------------
    CommandAnimGraphMoveParameter::CommandAnimGraphMoveParameter(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphMoveParameter", orgCommand)
    {
    }


    CommandAnimGraphMoveParameter::~CommandAnimGraphMoveParameter()
    {
    }

    void CommandAnimGraphMoveParameter::RelinkConnections(EMotionFX::AnimGraph* animGraph, const AZ::Outcome<size_t>& valueIndexBeforeMove, const AZ::Outcome<size_t>& valueIndexAfterMove)
    {
        // Get an array of all parameter nodes.
        MCore::Array<EMotionFX::AnimGraphNode*> parameterNodes;
        animGraph->RecursiveCollectNodesOfType(azrtti_typeid<EMotionFX::BlendTreeParameterNode>(), &parameterNodes);

        // Relink the affected connections that are connected to parameter nodes.
        const uint32 numParamNodes = parameterNodes.GetLength();
        for (uint32 i = 0; i < numParamNodes; ++i)
        {
            EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(parameterNodes[i]);

            if (valueIndexBeforeMove.IsSuccess() && valueIndexAfterMove.IsSuccess())
            {
                // Make sure the parameter mask is sorted and represents the moves.
                parameterNode->Reinit();

                // Calculate the new port indices based on the current value parameter order. When using a parameter mask, they will be local to the mask entries, elsewise the port indices will be equal to the anim graph value parameter indices.
                const uint32 portBeforeMove = static_cast<uint32>(parameterNode->FindPortForParameterIndex(valueIndexBeforeMove.GetValue()));
                const uint32 portAfterMove = static_cast<uint32>(parameterNode->FindPortForParameterIndex(valueIndexAfterMove.GetValue()));

                // Only swap connections if they actually moved. When a parameter mask is selected, a parameter move does not mean the ports move as well.
                EMotionFX::AnimGraphNode* parentNode = parameterNode->GetParentNode();
                if (portBeforeMove != MCORE_INVALIDINDEX32 &&
                    portAfterMove != MCORE_INVALIDINDEX32 &&
                    parentNode)
                {
                    parameterNode->GetOutputPort(portBeforeMove).mConnection = nullptr;
                    parameterNode->GetOutputPort(portAfterMove).mConnection = nullptr;

                    const uint32 numChildNodes = parentNode->GetNumChildNodes();
                    for (uint32 childIndex = 0; childIndex < numChildNodes; ++childIndex)
                    {
                        EMotionFX::AnimGraphNode* childNode = parentNode->GetChildNode(childIndex);

                        // Skip the current parameter node.
                        if (childNode == parameterNode)
                        {
                            continue;
                        }

                        const uint32 numConnections = childNode->GetNumConnections();
                        for (uint32 c = 0; c < numConnections; ++c)
                        {
                            EMotionFX::BlendTreeConnection* connection = childNode->GetConnection(c);

                            // Is this connection starting at the parameter node?
                            if (connection->GetSourceNode() == parameterNode)
                            {
                                // Swap the source ports for the connection.
                                if (connection->GetSourcePort() == portAfterMove)
                                {
                                    connection->SetSourcePort(static_cast<uint16>(portBeforeMove));
                                    parameterNode->GetOutputPort(portAfterMove).mConnection = connection;
                                }
                                else if (connection->GetSourcePort() == portBeforeMove)
                                {
                                    connection->SetSourcePort(static_cast<uint16>(portAfterMove));
                                    parameterNode->GetOutputPort(portAfterMove).mConnection = connection;
                                }
                            }
                        }
                    }
                }
            }
        }    
    }


    bool CommandAnimGraphMoveParameter::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZStd::string name;
        parameters.GetValue("name", this, name);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot swap parameters. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        AZ::Outcome<AZStd::string> parent = parameters.GetValueIfExists("parent", this);

        // if parent is empty, the parameter is being moved (or already in) the root group
        const EMotionFX::GroupParameter* destinationParent = animGraph->FindGroupParameterByName(parent.IsSuccess() ? parent.GetValue() : "");
        if (!destinationParent)
        {
            outResult = AZStd::string::format("Could not find destination parent \"%s\"", parent.GetValue().c_str());
            return false;
        }

        int32 destinationIndex = parameters.GetValueAsInt("index", this);
        const EMotionFX::ParameterVector& siblingDestinationParamenters = destinationParent->GetChildParameters();
        if (destinationIndex < 0 || destinationIndex > siblingDestinationParamenters.size())
        {
            outResult = AZStd::string::format("Index %d is out of bounds for parent \"%s\"", destinationIndex, parent.GetValue().c_str());
            return false;
        }

        const EMotionFX::Parameter* parameter = animGraph->FindParameterByName(name);
        if (!parameter)
        {
            outResult = AZStd::string::format("There is no parameter with the name '%s'.", name.c_str());
            return false;
        }

        // Get the current data for undo
        const EMotionFX::GroupParameter* currentParent = animGraph->FindParentGroupParameter(parameter);
        if (currentParent)
        {
            mOldParent = currentParent->GetName();
            mOldIndex = currentParent->FindRelativeParameterIndex(parameter).GetValue();
        }
        else
        {
            mOldParent.clear(); // means the root
            mOldIndex = animGraph->FindRelativeParameterIndex(parameter).GetValue();
        }

        // If the parameter being moved is a value parameter (not a group), we need to update the anim graph instances and
        // nodes. To do so, we need to get the absolute index of the parameter before and after the move.
        AZ::Outcome<size_t> valueIndexBeforeMove = AZ::Failure();
        AZ::Outcome<size_t> valueIndexAfterMove = AZ::Failure();
        const bool isValueParameter = azrtti_typeid(parameter) != azrtti_typeid<EMotionFX::GroupParameter>();
        if (isValueParameter)
        {
            valueIndexBeforeMove = animGraph->FindValueParameterIndex(static_cast<const EMotionFX::ValueParameter*>(parameter));
        }

        if (!animGraph->TakeParameterFromParent(parameter))
        {
            outResult = AZStd::string::format("Could not remove the parameter \"%s\" from its parent", name.c_str());
            return false;
        }
        animGraph->InsertParameter(destinationIndex, const_cast<EMotionFX::Parameter*>(parameter), destinationParent);

        if (isValueParameter)
        {
            valueIndexAfterMove = animGraph->FindValueParameterIndex(static_cast<const EMotionFX::ValueParameter*>(parameter));
        }

        if (!isValueParameter || valueIndexAfterMove.GetValue() == valueIndexBeforeMove.GetValue())
        {
            // Nothing else to do, the anim graph instances and nodes dont require an update
            return true;
        }

        // Remove the parameter from all corresponding anim graph instances if it is a value parameter
        const size_t numInstances = EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances();
        for (size_t i = 0; i < numInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i);
            if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
            {
                // Remove the parameter and add it to the new position
                animGraphInstance->RemoveParameterValue(static_cast<uint32>(valueIndexBeforeMove.GetValue()));
                animGraphInstance->InsertParameterValue(static_cast<uint32>(valueIndexAfterMove.GetValue()));
            }
        }

        // Automatically relink connected connections to the new ports.
        RelinkConnections(animGraph, valueIndexBeforeMove, valueIndexAfterMove);

        // Save the current dirty flag and tell the anim graph that something got changed.
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        animGraph->Reinit();
        animGraph->UpdateUniqueData();

        return true;
    }


    bool CommandAnimGraphMoveParameter::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZStd::string name;
        parameters.GetValue("name", this, name);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot undo move parameters. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        AZStd::string commandString = AZStd::string::format("AnimGraphMoveParameter -animGraphID %i -name \"%s\" -index %d",
                animGraphID,
                name.c_str(),
                mOldIndex);
        if (!mOldParent.empty())
        {
            commandString += AZStd::string::format(" -parent \"%s\"", mOldParent.c_str());
        }

        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
            return false;
        }

        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphMoveParameter::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name", "The name of the parameter to move.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("index", "The new index of the parameter, relative to the new parent", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("parent", "The new parent of the parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    const char* CommandAnimGraphMoveParameter::GetDescription() const
    {
        return "This command moves a parameter to another place in the parameter hierarchy.";
    }

    //--------------------------------------------------------------------------------
    // Helper functions
    //--------------------------------------------------------------------------------

    //--------------------------------------------------------------------------------
    // Construct create parameter command strings
    //--------------------------------------------------------------------------------
    void ConstructCreateParameterCommand(AZStd::string& outResult, EMotionFX::AnimGraph* animGraph, const EMotionFX::Parameter* parameter, uint32 insertAtIndex)
    {
        // Build the command string.
        AZStd::string parameterContents;
        parameterContents = MCore::ReflectionSerializer::Serialize(parameter).GetValue();

        outResult = AZStd::string::format("AnimGraphCreateParameter -animGraphID %i -type \"%s\" -name \"%s\" -contents {%s}",
                animGraph->GetID(),
                azrtti_typeid(parameter).ToString<AZStd::string>().c_str(),
                parameter->GetName().c_str(),
                parameterContents.c_str());

        if (insertAtIndex != MCORE_INVALIDINDEX32)
        {
            outResult += AZStd::string::format(" -index \"%i\"", insertAtIndex);
        }
    }


    //--------------------------------------------------------------------------------
    // Remove and or clear parameter helper functions
    //--------------------------------------------------------------------------------
    void ClearParametersCommand(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup)
    {
        const EMotionFX::ValueParameterVector parameters = animGraph->RecursivelyGetValueParameters();
        if (parameters.empty())
        {
            return;
        }

        // Iterate through all parameters and fill the parameter name array.
        AZStd::vector<AZStd::string> parameterNames;
        parameterNames.reserve(static_cast<size_t>(parameters.size()));
        for (const EMotionFX::ValueParameter* parameter : parameters)
        {
            parameterNames.push_back(parameter->GetName());
        }

        // Is the command group parameter set?
        if (!commandGroup)
        {
            // Create and fill the command group.
            AZStd::string outResult;
            MCore::CommandGroup internalCommandGroup("Clear parameters");
            BuildRemoveParametersCommandGroup(animGraph, parameterNames, &internalCommandGroup);

            // Execute the command group.
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, outResult))
            {
                AZ_Error("EMotionFX", false, outResult.c_str());
            }
        }
        else
        {
            // Use the given parameter command group.
            BuildRemoveParametersCommandGroup(animGraph, parameterNames, commandGroup);
        }
    }


    bool BuildRemoveParametersCommandGroup(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& parameterNamesToRemove, MCore::CommandGroup* commandGroup)
    {
        // Make sure the anim graph is valid and that the parameter names array at least contains a single one.
        if (!animGraph || parameterNamesToRemove.empty())
        {
            return false;
        }

        // Create the command group.
        AZStd::string outResult;
        AZStd::string commandString;

        MCore::CommandGroup internalCommandGroup(commandString.c_str());
        MCore::CommandGroup* usedCommandGroup = commandGroup;
        if (!commandGroup)
        {
            if (parameterNamesToRemove.size() == 1)
            {
                commandString = AZStd::string::format("Remove parameter '%s'", parameterNamesToRemove[0].c_str());
            }
            else
            {
                commandString = AZStd::string::format("Remove %d parameters", parameterNamesToRemove.size());
            }

            usedCommandGroup = &internalCommandGroup;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 1: Get all parameter nodes in the anim graph
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        MCore::Array<EMotionFX::AnimGraphNode*> parameterNodes;
        animGraph->RecursiveCollectNodesOfType(azrtti_typeid<EMotionFX::BlendTreeParameterNode>(), &parameterNodes);
        const uint32 numParameterNodes = parameterNodes.GetLength();

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 2: Remember the old connections
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        AZStd::vector<ParameterConnectionItem> oldParameterConnections;
        for (uint32 i = 0; i < numParameterNodes; ++i)
        {
            const EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(parameterNodes[i]);
            const EMotionFX::AnimGraphNode* parentNode = parameterNode->GetParentNode();
            if (!parentNode)
            {
                continue;
            }

            // Iterate through all nodes and check if any of these is connected to our parameter node.
            const uint32 numNodes = parentNode->GetNumChildNodes();
            for (uint32 j = 0; j < numNodes; ++j)
            {
                const EMotionFX::AnimGraphNode* childNode = parentNode->GetChildNode(j);

                // Skip the node in case it is the parameter node itself.
                if (childNode == parameterNode)
                {
                    continue;
                }

                const uint32 numConnections = childNode->GetNumConnections();
                for (uint32 c = 0; c < numConnections; ++c)
                {
                    const EMotionFX::BlendTreeConnection* connection = childNode->GetConnection(c);

                    // Is the connection plugged to the parameter node?
                    if (connection->GetSourceNode() == parameterNode)
                    {
                        // Only add if this connection is not plugged to one of the parameter node ports that is going to be removed.
                        const uint16 sourcePort = connection->GetSourcePort();
                        const uint32 parameterIndex = parameterNode->GetParameterIndex(sourcePort);
                        const EMotionFX::ValueParameter* valueParameter = animGraph->FindValueParameter(parameterIndex);
                        const AZStd::string& parameterName = valueParameter->GetName();

                        if (AZStd::find(parameterNamesToRemove.begin(), parameterNamesToRemove.end(), parameterName) == parameterNamesToRemove.end())
                        {
                            ParameterConnectionItem connectionItem;
                            connectionItem.SetParameterNodeName(parameterNode->GetName());
                            connectionItem.SetTargetNodeName(childNode->GetName());
                            connectionItem.SetParameterName(parameterName.c_str());
                            connectionItem.mTargetNodePort = connection->GetTargetPort();

                            oldParameterConnections.push_back(connectionItem);
                        }
                    }
                }
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 3: Delete all connections for all parameter nodes.
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        AZStd::vector<EMotionFX::BlendTreeConnection*> connectionList;

        // Iterate through all parameter nodes.
        for (uint32 i = 0; i < numParameterNodes; ++i)
        {
            EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(parameterNodes[i]);
            EMotionFX::AnimGraphNode* parentNode = parameterNode->GetParentNode();
            if (!parentNode)
            {
                continue;
            }

            DeleteNodeConnections(usedCommandGroup, parameterNode, parentNode, connectionList, false);
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 4: Adjust parameter masks for all parameter nodes
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        AZStd::string parameterMaskString;
        for (uint32 i = 0; i < numParameterNodes; ++i)
        {
            // Type-cast the anim graph node to a parameter node and check if there is a parameter mask applied.
            const EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(parameterNodes[i]);
            if (!parameterNode->GetParameters().empty())
            {
                // Create the new parameter mask excluding the parameter we are removing.
                parameterMaskString = EMotionFX::BlendTreeParameterNode::ConstructParameterNamesString(parameterNode->GetParameters(), parameterNamesToRemove);
                commandString = AZStd::string::format("AnimGraphAdjustNode -animGraphID %i -name \"%s\" -parameterMask \"%s\"", animGraph->GetID(), parameterNode->GetName(), parameterMaskString.c_str());
                usedCommandGroup->AddCommandString(commandString);
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 5: Remove the parameters from the anim graph
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        for (const AZStd::string& parameterName : parameterNamesToRemove)
        {
            commandString = AZStd::string::format("AnimGraphRemoveParameter -animGraphID %i -name \"%s\"", animGraph->GetID(), parameterName.c_str());
            usedCommandGroup->AddCommandString(commandString);
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 6: Recreate the connections at the mapped/new ports
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        RecreateOldConnections(animGraph, oldParameterConnections, commandGroup, parameterNamesToRemove);

        // Is the command group parameter set?
        if (!commandGroup)
        {
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, outResult))
            {
                AZ_Error("EMotionFX", false, outResult.c_str());
                return false;
            }
        }

        return true;
    }


    void RecreateOldParameterMaskConnections(EMotionFX::AnimGraph* animGraph, const AZStd::vector<ParameterConnectionItem>& oldParameterConnections, MCore::CommandGroup* commandGroup, const AZStd::vector<AZStd::string>& newParameterMask)
    {
        if (!animGraph)
        {
            return;
        }

        AZStd::string commandString;

        // Get the number of old connections and terate through them.
        for (const ParameterConnectionItem& connectionItem : oldParameterConnections)
        {
            // Find the parameter node by name.
            EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNodeByName(connectionItem.GetParameterNodeName());
            if (!animGraphNode || azrtti_typeid(animGraphNode) != azrtti_typeid<EMotionFX::BlendTreeParameterNode>())
            {
                continue;
            }

            const EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(animGraphNode);

            uint32 parameterNodePortNr = MCORE_INVALIDINDEX32;
            if (newParameterMask.empty())
            {
                // In case the parameter mask is empty, all parameters are shown in the parameter node. We can just use the global value parameter index.
                const AZ::Outcome<size_t> parameterIndex = animGraph->FindValueParameterIndexByName(connectionItem.GetParameterName());
                MCORE_ASSERT(parameterIndex.IsSuccess());
                parameterNodePortNr = static_cast<uint32>(parameterIndex.GetValue());
            }
            else
            {
                // Search the index in the local parameter name mask array.
                const auto iterator = AZStd::find(newParameterMask.begin(), newParameterMask.end(), connectionItem.GetParameterName());
                parameterNodePortNr = static_cast<uint32>(iterator - newParameterMask.begin());
            }

            if (parameterNodePortNr != MCORE_INVALIDINDEX32)
            {

                commandString = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d",
                    animGraph->GetID(),
                    parameterNode->GetName(),
                    connectionItem.GetTargetNodeName(),
                    parameterNodePortNr,
                    connectionItem.mTargetNodePort);

                commandGroup->AddCommandString(commandString);
            }
        }
    }


    void RecreateOldConnections(EMotionFX::AnimGraph* animGraph, const AZStd::vector<ParameterConnectionItem>& oldParameterConnections, MCore::CommandGroup* commandGroup, const AZStd::vector<AZStd::string>& parametersToBeRemoved)
    {
        if (!animGraph)
        {
            return;
        }

        AZStd::string commandString;

        // Get the number of old connections and terate through them.
        for (const ParameterConnectionItem& connectionItem : oldParameterConnections)
        {
            // Find the parameter node by name.
            EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNodeByName(connectionItem.GetParameterNodeName());
            if (!animGraphNode || azrtti_typeid(animGraphNode) != azrtti_typeid<EMotionFX::BlendTreeParameterNode>())
            {
                continue;
            }

            const EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(animGraphNode);

            const size_t sourcePortNr = parameterNode->CalcNewPortIndexForParameter(connectionItem.GetParameterName(), parametersToBeRemoved);
            if (sourcePortNr != MCORE_INVALIDINDEX32)
            {
                commandString = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d",
                        animGraph->GetID(),
                        parameterNode->GetName(),
                        connectionItem.GetTargetNodeName(),
                        sourcePortNr,
                        connectionItem.mTargetNodePort);

                commandGroup->AddCommandString(commandString);
            }
        }
    }

    void RewireConnectionsForParameterNodesAfterParameterIndexChange(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameterVector& valueParametersBeforeChange, const EMotionFX::ValueParameterVector& valueParametersAfterChange)
    {
        // Get an array of all parameter nodes.
        MCore::Array<EMotionFX::AnimGraphNode*> parameterNodes;
        animGraph->RecursiveCollectNodesOfType(azrtti_typeid<EMotionFX::BlendTreeParameterNode>(), &parameterNodes);

        // Relink the affected connections that are connected to parameter nodes.
        const uint32 numParamNodes = parameterNodes.GetLength();
        for (uint32 i = 0; i < numParamNodes; ++i)
        {
            EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(parameterNodes[i]);

            AZStd::vector<EMotionFX::BlendTreeConnection*> connections;
            AZStd::vector<EMotionFX::AnimGraphNode*> targetNodes;
            parameterNode->CollectOutgoingConnections(connections, targetNodes);

            EMotionFX::ValueParameterVector parameterPerConnection;
            const AZStd::vector<AZStd::string>& parameterMask = parameterNode->GetParameters();

            for (EMotionFX::BlendTreeConnection* connection : connections)
            {
                // Get the parameter index before the change.
                const uint32 parameterIndex = parameterNode->GetParameterIndex(connection->GetSourcePort());
                parameterPerConnection.emplace_back(valueParametersBeforeChange[parameterIndex]);
            }

            // Update the internally stored parameter indices for the parameter mask in order find the correct port index for the connections
            parameterNode->Reinit();

            const size_t blendTreeConnectionSize = connections.size();
            for (size_t j = 0; j < blendTreeConnectionSize; ++j)
            {
                EMotionFX::BlendTreeConnection* connection = connections[j];
                const EMotionFX::ValueParameter* valueParameter = parameterPerConnection[j];

                // Find the value parameter index in the value parameter array after the change.
                const auto iterator = AZStd::find(valueParametersAfterChange.begin(), valueParametersAfterChange.end(), valueParameter);
                if (iterator == valueParametersAfterChange.end())
                {
                    continue;
                }

                const size_t parameterIndexAfterChange = AZStd::distance(valueParametersAfterChange.begin(), iterator);

                // Depending on if we use a parameter mask or not the parameter index after the change can differentiate from the actual port index.
                const uint32 portIndexAfterChange = static_cast<uint32>(parameterNode->FindPortForParameterIndex(parameterIndexAfterChange));

                // Adjust the actual source port of the connection and update the port's connection.
                connection->SetSourcePort(static_cast<uint16>(portIndexAfterChange));
                parameterNode->GetOutputPort(portIndexAfterChange).mConnection = connection;
            }
        }
    }

} // namesapce EMotionFX
