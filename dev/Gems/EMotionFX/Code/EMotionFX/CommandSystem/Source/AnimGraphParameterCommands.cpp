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
#include <AzFramework/StringFunc/StringFunc.h>
#include "AnimGraphConnectionCommands.h"
#include "CommandManager.h"

#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/AnimGraphParameterGroup.h>
#include <EMotionFX/Source/AnimGraphParameterCondition.h>
#include <EMotionFX/Source/ActorManager.h>
#include <MCore/Source/AttributeFactory.h>
#include <MCore/Source/AttributeSettings.h>
#include <MCore/Source/AttributePool.h>


namespace CommandSystem
{
    //-------------------------------------------------------------------------------------
    // Create a anim graph parameter
    //-------------------------------------------------------------------------------------
    CommandAnimGraphCreateParameter::CommandAnimGraphCreateParameter(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphCreateParameter", orgCommand)
    {
        mIndex = -1;
    }


    CommandAnimGraphCreateParameter::~CommandAnimGraphCreateParameter()
    {
    }

    
    bool CommandAnimGraphCreateParameter::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // Get the parameter name.
        AZStd::string name;
        parameters.GetValue("name", this, name);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult.Format("Cannot add parameter '%s' to anim graph. Anim graph id '%i' is not valid.", name.c_str(), animGraphID);
            return false;
        }

        // Check if the parameter name is unique and not used by any other parameter yet.
        if (animGraph->FindParameter(name.c_str()))
        {
            outResult.Format("There is already a parameter with the name '%s', please use a another, unique name.", name.c_str());
            return false;
        }

        // Get the data type and check if it is a valid one.
        const uint32 interfaceType = parameters.GetValueAsInt("interfaceType", this);
        if (interfaceType == MCORE_INVALIDINDEX32)
        {
            outResult.Format("The interface type is not a valid interface type. Please use -help or use the command browser to see a list of valid options.");
            return false;
        }

        // Create the new parameter based on the dialog settings.
        MCore::AttributeSettings* newParam = MCore::AttributeSettings::Create();
        newParam->SetName(name.c_str());
        newParam->SetInternalName(name.c_str());
        newParam->SetInterfaceType(interfaceType);

        const bool scalable = parameters.GetValueAsBool("isScalable", false);
        newParam->SetCanScale(scalable);

        // Get the data type.
        const uint32 dataType = EMotionFX::AnimGraphObject::InterfaceTypeToDataType(interfaceType);

        // init the values to this data type, so we later on know how to convert the strings to the values
        newParam->SetMinValue(MCore::GetAttributeFactory().CreateAttributeByType(dataType));
        newParam->SetMaxValue(MCore::GetAttributeFactory().CreateAttributeByType(dataType));
        newParam->SetDefaultValue(MCore::GetAttributeFactory().CreateAttributeByType(dataType));

        AZStd::string valueString;

        // Set the minimum value.
        if (dataType != MCore::AttributeBool::TYPE_ID && dataType != MCore::AttributeString::TYPE_ID)
        {
            parameters.GetValue("minValue", this, valueString);
            if (!newParam->GetMinValue()->InitFromString(valueString.c_str()))
            {
                outResult.Format("Failed to initialize minimum value from string '%s'", valueString.c_str());
                return false;
            }

            // Set the maximum value.
            parameters.GetValue("maxValue", this, valueString);
            if (!newParam->GetMaxValue()->InitFromString(valueString.c_str()))
            {
                outResult.Format("Failed to initialize maximum value from string '%s'", valueString.c_str());
                return false;
            }
        }

        // Set the default value.
        parameters.GetValue("defaultValue", this, valueString);
        if (!newParam->GetDefaultValue()->InitFromString(valueString.c_str()))
        {
            outResult.Format("Failed to initialize default value from string '%s'", valueString.c_str());
            return false;
        }

        // Description.
        parameters.GetValue("description", this, valueString);
        if (!valueString.empty())
        {
            newParam->SetDescription(valueString.c_str());
        }

        // The position inside the parameter array where the parameter should get added to.
        if (mIndex == -1)
        {
            mIndex = parameters.GetValueAsInt("index", this);
            if (mIndex < 0 || mIndex >= (int32)animGraph->GetNumParameters())
            {
                outResult.Format("Index '%d' out of range.", mIndex);
                mIndex = -1;
            }
        }

        // Add the new parameter.
        if (mIndex == -1)
        {
            animGraph->AddParameter(newParam);
        }
        else
        {
            // Relink all parameter indices in the parameter groups that have a bigger index than the one we're gonna insert.
            const uint32 numParameterGroups = animGraph->GetNumParameterGroups();
            for (uint32 i = 0; i < numParameterGroups; ++i)
            {
                EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->GetParameterGroup(i);

                const uint32 numParameters = parameterGroup->GetNumParameters();
                for (uint32 j = 0; j < numParameters; ++j)
                {
                    // Relink the index in case it is bigger.
                    const int32 groupParamIndex = parameterGroup->GetParameter(j);
                    if (groupParamIndex >= mIndex)
                    {
                        parameterGroup->SetParameter(j, groupParamIndex + 1);
                    }
                }
            }

            // Insert the parameter at the given index.
            animGraph->InsertParameter(newParam, mIndex);
        }

        // Check if the parameter group got specified.
        if (parameters.CheckIfHasParameter("parameterGroupName"))
        {
            parameters.GetValue("parameterGroupName", this, valueString);

            // Find the parameter group index and get a pointer to the parameter group.
            EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->FindParameterGroupByName(valueString.c_str());
            if (!parameterGroup)
            {
                MCore::LogWarning("The parameter group named '%s' could not be found. The parameter cannot be added to the group.", valueString.c_str());
            }
            else
            {
                // Add the parameter to the parameter group.
                if (mIndex == -1)
                {
                    parameterGroup->AddParameter(animGraph->GetNumParameters() - 1);
                }
                else
                {
                    parameterGroup->AddParameter(mIndex);
                }
            }
        }

        // Update all anim graph instances.
        const uint32 numInstances = EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i);
            if (!animGraphInstance)
            {
                continue;
            }

            // Only update actor instances using the given anim graph.
            if (animGraphInstance->GetAnimGraph() != animGraph)
            {
                continue;
            }

            // Add the missing parameters (the new ones we added to the anim graph).
            if (mIndex == -1)
            {
                animGraphInstance->AddParameterValue();
            }
            else
            {
                animGraphInstance->InsertParameterValue(mIndex);
            }
        }

        // Get an array of all parameter nodes.
        MCore::Array<EMotionFX::AnimGraphNode*> parameterNodes;
        parameterNodes.Reserve(8);
        animGraph->RecursiveCollectNodesOfType(EMotionFX::BlendTreeParameterNode::TYPE_ID, &parameterNodes);

        const uint32 numParamNodes = parameterNodes.GetLength();
        for (uint32 i = 0; i < numParamNodes; ++i)
        {
            // Type-cast the anim graph node to a parameter node and reinitialize the ports.
            EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(parameterNodes[i]);
            parameterNode->Reinit();
        }

        // Set the node name as command result.
        outResult = name.c_str();

        // Save the current dirty flag and tell the anim graph that something got changed.
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        animGraph->RecursiveUpdateAttributes();

        // Update unique datas for all corresponding anim graph instances.
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


    bool CommandAnimGraphCreateParameter::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // Get the parameter name.
        AZStd::string name;
        parameters.GetValue("name", this, name);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult.Format("Cannot undo add parameter '%s' to anim graph. Anim graph id '%i' is not valid.", name.c_str(), animGraphID);
            return false;
        }

        const AZStd::string commandString = AZStd::string::format("AnimGraphRemoveParameter -animGraphID %i -name \"%s\"", animGraph->GetID(), name.c_str());
        
        MCore::String result;
        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, result))
        {
            AZ_Error("EMotionFX", false, result.AsChar());
        }

        // Set the dirty flag back to the old value.
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphCreateParameter::InitSyntax()
    {
        GetSyntax().ReserveParameters(9);
        GetSyntax().AddRequiredParameter("animGraphID",    "The id of the anim graph.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("interfaceType",   "The interface type id.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name",            "The name of the parameter, which has to be unique inside the currently selected anim graph.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("defaultValue",    "The default value of the parameter.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("minValue",        "The minimum value of the parameter. In case of checkboxes or strings this parameter value will be ignored.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("maxValue",        "The maximum value of the parameter. In case of checkboxes or strings this parameter value will be ignored.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("description",             "The description of the parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("index",                   "The position where the parameter should be added. If the parameter is not specified it will get added to the end.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("parameterGroupName",      "The parameter group into which the parameter should be added. If the parameter is not specified it will get added to any parameter group.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("isScalable",              "Is this parameter scalable?", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
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


    bool CommandAnimGraphRemoveParameter::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // Get the parameter name.
        AZStd::string name;
        parameters.GetValue("name", this, name);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult.Format("Cannot remove parameter '%s' from anim graph. Anim graph id '%i' is not valid.", name.c_str(), animGraphID);
            return false;
        }

        // Check if there is a parameter with the given name.
        const uint32 paramIndex = animGraph->FindParameterIndex(name.c_str());
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            outResult.Format("Cannot remove parameter '%s' from anim graph. There is no parameter with the given name.", name.c_str(), animGraphID);
            return false;
        }

        // Store undo info before we remove it, so that we can recreate it later.
        MCore::AttributeSettings* parameter = animGraph->GetParameter(paramIndex);
        mName           = parameter->GetName();
        mDescription    = parameter->GetDescription();
        mInterfaceType  = parameter->GetInterfaceType();
        mIndex          = paramIndex;
        mIsScalable     = parameter->GetCanScale();

        MCore::String tempString;
        parameter->GetMinValue()->ConvertToString(tempString);
        mMinValue = tempString.AsChar();
        parameter->GetMaxValue()->ConvertToString(tempString);
        mMaxValue = tempString.AsChar();
        parameter->GetDefaultValue()->ConvertToString(tempString);
        mDefaultValue = tempString.AsChar();

        // Find and save the parameter group in which the node was before, for undo reasons.
        EMotionFX::AnimGraphParameterGroup* oldParameterGroup = animGraph->FindParameterGroupForParameter(paramIndex);
        mOldParameterGroupName.clear();
        if (oldParameterGroup)
        {
            mOldParameterGroupName = oldParameterGroup->GetName();
        }

        // Relink all parameter indices in the parameter groups that have a bigger index than the one we're gonna remove.
        const uint32 numParameterGroups = animGraph->GetNumParameterGroups();
        for (uint32 i = 0; i < numParameterGroups; ++i)
        {
            EMotionFX::AnimGraphParameterGroup* parameterGroup = animGraph->GetParameterGroup(i);

            const uint32 numParameters = parameterGroup->GetNumParameters();
            for (uint32 j = 0; j < numParameters; ++j)
            {
                // Relink the index in case it is bigger.
                const uint32 groupParamIndex = parameterGroup->GetParameter(j);
                if (groupParamIndex > paramIndex)
                {
                    parameterGroup->SetParameter(j, groupParamIndex - 1);
                }
            }

            if (parameterGroup == oldParameterGroup)
            {
                parameterGroup->RemoveParameter(paramIndex);
            }
        }

        // Remove the parameter from the anim graph.
        animGraph->RemoveParameter(paramIndex);

        // Remove the parameter from all corresponding anim graph instances.
        const uint32 numInstances = EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i);
            if (!animGraphInstance)
            {
                continue;
            }

            if (animGraphInstance->GetAnimGraph() != animGraph)
            {
                continue;
            }

            // Remove the parameter.
            animGraphInstance->RemoveParameterValue(paramIndex);
        }

        // Get an array of all parameter nodes.
        MCore::Array<EMotionFX::AnimGraphNode*> parameterNodes;
        animGraph->RecursiveCollectNodesOfType(EMotionFX::BlendTreeParameterNode::TYPE_ID, &parameterNodes);

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

        animGraph->RecursiveUpdateAttributes();

        // Update unique datas for all corresponding anim graph instances.
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


    bool CommandAnimGraphRemoveParameter::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult.Format("Cannot undo remove parameter '%s' from anim graph. Anim graph id '%i' is not valid.", mName.c_str(), animGraphID);
            return false;
        }

        // Execute the command to create the parameter again.
        AZStd::string commandString;
        if (mDescription.empty())
        {
            commandString = AZStd::string::format("AnimGraphCreateParameter -animGraphID %i -name \"%s\" -index %i -interfaceType %i -minValue \"%s\" -maxValue \"%s\" -defaultValue \"%s\" -isScalable %s", animGraph->GetID(), mName.c_str(), mIndex, mInterfaceType, mMinValue.c_str(), mMaxValue.c_str(), mDefaultValue.c_str(), mIsScalable ? "true" : "false");
        }
        else
        {
            commandString = AZStd::string::format("AnimGraphCreateParameter -animGraphID %i -name \"%s\" -index %i -description \"%s\" -interfaceType %i -minValue \"%s\" -maxValue \"%s\" -defaultValue \"%s\" -isScalable %s", animGraph->GetID(), mName.c_str(), mIndex, mDescription.c_str(), mInterfaceType, mMinValue.c_str(), mMaxValue.c_str(), mDefaultValue.c_str(), mIsScalable ? "true" : "false");
        }

        // In case the parameter was inside a parameter group before, restore it.
        if (!mOldParameterGroupName.empty())
        {
            commandString += AZStd::string::format(" -parameterGroupName \"%s\"", mOldParameterGroupName.c_str());
        }

        // Execute the command.
        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            AZ_Error("EMotionFX", false, outResult.AsChar());
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


    void CommandAnimGraphAdjustParameter::UpdateTransitionConditions(const char* oldParameterName, const char* newParameterName)
    {
        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (uint32 i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }
            
            // Collect all parameter transition conditions.
            MCore::Array<EMotionFX::AnimGraphTransitionCondition*> conditions;
            animGraph->RecursiveCollectTransitionConditionsOfType(EMotionFX::AnimGraphParameterCondition::TYPE_ID, &conditions);

            const uint32 numConditions = conditions.GetLength();
            for (uint32 j = 0; j < numConditions; ++j)
            {
                EMotionFX::AnimGraphParameterCondition* parameterCondition = static_cast<EMotionFX::AnimGraphParameterCondition*>(conditions[j]);

                // Adjust all parameter conditions along with the parameter name change.
                if (parameterCondition->GetAttributeString(EMotionFX::AnimGraphParameterCondition::ATTRIB_PARAMETER)->GetValue().CheckIfIsEqual(oldParameterName))
                {
                    parameterCondition->GetAttributeString(EMotionFX::AnimGraphParameterCondition::ATTRIB_PARAMETER)->SetValue(newParameterName);
                }
            }

            // Recursively update attributes of all nodes.
            animGraph->RecursiveUpdateAttributes();
        }
    }


    bool CommandAnimGraphAdjustParameter::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // Get the parameter name.
        AZStd::string name;
        parameters.GetValue("name", this, name);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult.Format("Cannot adjust parameter '%s'. Anim graph with id '%d' not found.", name.c_str(), animGraphID);
            return false;
        }

        const uint32 paramIndex = animGraph->FindParameterIndex(name.c_str());
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            outResult.Format("There is no parameter with the name '%s'.", name.c_str());
            return false;
        }

        MCore::AttributeSettings* paramSettings = animGraph->GetParameter(paramIndex);

        // Store the undo info.
        mOldName            = paramSettings->GetName();
        mOldDescription     = paramSettings->GetDescription();
        mOldInterfaceType   = paramSettings->GetInterfaceType();
        mOldIsScalable      = paramSettings->GetCanScale();

        MCore::String tempString;
        paramSettings->GetMinValue()->ConvertToString(tempString);
        mOldMinValue = tempString.AsChar();
        paramSettings->GetMaxValue()->ConvertToString(tempString);
        mOldMaxValue = tempString.AsChar();
        paramSettings->GetDefaultValue()->ConvertToString(tempString);
        mOldDefaultValue = tempString.AsChar();

        // Get the new name and check if it is valid.
        AZStd::string newName;
        parameters.GetValue("newName", this, newName);
        if (!newName.empty())
        {
            const uint32 newIndex = animGraph->FindParameterIndex(newName.c_str());
            if (newIndex != paramIndex && newIndex != MCORE_INVALIDINDEX32)
            {
                outResult.Format("There is already a parameter with the name '%s', please use a unique name.", newName.c_str());
                return false;
            }
        }

        // Get the data type and check if it is a valid one.
        uint32 interfaceType = MCORE_INVALIDINDEX32;
        if (parameters.CheckIfHasParameter("interfaceType"))
        {
            interfaceType = parameters.GetValueAsInt("interfaceType", this);
            if (interfaceType == MCORE_INVALIDINDEX32)
            {
                outResult.Format("The interface type is not a valid interface type. Please use -help or use the command browser to see a list of valid options.");
                return false;
            }
        }

        // Get the value strings.
        AZStd::string minValue;
        AZStd::string maxValue;
        AZStd::string defaultValue;
        AZStd::string description;
        parameters.GetValue("minValue", this, minValue);
        parameters.GetValue("maxValue", this, maxValue);
        parameters.GetValue("defaultValue", this, defaultValue);
        parameters.GetValue("description", this, description);

        // Apply the interface type.
        if (parameters.CheckIfHasParameter("interfaceType"))
        {
            paramSettings->SetInterfaceType(interfaceType);
        }
        else
        {
            interfaceType = paramSettings->GetInterfaceType();
        }

        // Init the values to the right data type, so we later on know how to convert the strings to the values.
        const uint32 dataType = EMotionFX::AnimGraphObject::InterfaceTypeToDataType(interfaceType);
        paramSettings->SetMinValue(MCore::GetAttributeFactory().CreateAttributeByType(dataType));
        paramSettings->SetMaxValue(MCore::GetAttributeFactory().CreateAttributeByType(dataType));
        paramSettings->SetDefaultValue(MCore::GetAttributeFactory().CreateAttributeByType(dataType));

        if (parameters.CheckIfHasParameter("isScalable"))
        {
            const bool isScalable = parameters.GetValueAsBool("isScalable", this);
            paramSettings->SetCanScale(isScalable);
        }

        // Set the minimum and maximum values.
        if (dataType != MCore::AttributeBool::TYPE_ID)
        {
            if (!minValue.empty())
            {
                if (!paramSettings->GetMinValue()->InitFromString(minValue.c_str()))
                {
                    outResult.Format("Failed to initialize minimum value from string '%s'", minValue.c_str());
                    return false;
                }
            }

            if (!maxValue.empty())
            {
                if (!paramSettings->GetMaxValue()->InitFromString(maxValue.c_str()))
                {
                    outResult.Format("Failed to initialize maximum value from string '%s'", maxValue.c_str());
                    return false;
                }
            }
        }

        // Set the default value.
        if (!defaultValue.empty())
        {
            if (!paramSettings->GetDefaultValue()->InitFromString(defaultValue.c_str()))
            {
                outResult.Format("Failed to initialize default value from string '%s'", defaultValue.c_str());
                return false;
            }
        }

        // Apply the name change.
        if (!newName.empty())
        {
            RenameParameterNodePorts(animGraph, nullptr, paramSettings->GetName(), newName.c_str());

            // Update all transition conditions along with the parameter name change.
            UpdateTransitionConditions(paramSettings->GetName(), newName.c_str());

            paramSettings->SetName(newName.c_str());
            paramSettings->SetInternalName(newName.c_str());
        }

        // Change description.
        if (!description.empty())
        {
            paramSettings->SetDescription(description.c_str());
        }


        // Update all corresponding anim graph instances.
        const uint32 numInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetActorManager().GetActorInstance(i)->GetAnimGraphInstance();
            if (!animGraphInstance)
            {
                continue;
            }

            if (animGraphInstance->GetAnimGraph() != animGraph)
            {
                continue;
            }

            // Add the missing parameters (the new ones we added to the anim graph).
            animGraphInstance->ReInitParameterValue(paramIndex);
        }


        // Get an array of all parameter nodes.
        MCore::Array<EMotionFX::AnimGraphNode*> parameterNodes;
        parameterNodes.Reserve(16);
        animGraph->RecursiveCollectNodesOfType(EMotionFX::BlendTreeParameterNode::TYPE_ID, &parameterNodes);

        const uint32 numParamNodes = parameterNodes.GetLength();
        for (uint32 i = 0; i < numParamNodes; ++i)
        {
            // Type-cast the anim graph node to a parameter node and reinitialize the ports.
            EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(parameterNodes[i]);

            // In case we changed the name of a parameter also update the parameter masks.
            if (!newName.empty())
            {
                EMotionFX::AttributeParameterMask* parameterMaskAttribute = static_cast<EMotionFX::AttributeParameterMask*>(parameterNode->GetAttribute(EMotionFX::BlendTreeParameterNode::ATTRIB_MASK));

                // Check if the parameter was in the parameter mask, if not just skip the process.
                const uint32 oldNameMaskIndex = parameterMaskAttribute->FindParameterNameIndex(mOldName.c_str());
                if (oldNameMaskIndex != MCORE_INVALIDINDEX32)
                {
                    // Remove the old name from the mask, add the new one and sort the mask afterwards.
                    parameterMaskAttribute->RemoveParameterName(oldNameMaskIndex);
                    parameterMaskAttribute->AddParameterName(newName.c_str());
                    parameterMaskAttribute->Sort(parameterNode->GetAnimGraph());
                }
            }

            parameterNode->Reinit();
        }

        // Save the current dirty flag and tell the anim graph that something got changed.
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        animGraph->RecursiveUpdateAttributes();

        // Update unique datas for all corresponding anim graph instances.
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


    void CommandAnimGraphAdjustParameter::RenameParameterNodePorts(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* startNode, const char* oldName, const char* newName)
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
            // Update outport name when start node is a blend tree.
            if (startNode->GetType() == EMotionFX::BlendTreeParameterNode::TYPE_ID)
            {
                EMotionFX::BlendTreeParameterNode* paramNode = static_cast<EMotionFX::BlendTreeParameterNode*>(startNode);
                const uint32 index = paramNode->FindOutputPortIndex(oldName);
                if (index != MCORE_INVALIDINDEX32)
                {
                    paramNode->SetOutputPortName(index, newName);
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


    bool CommandAnimGraphAdjustParameter::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult.Format("Cannot adjust parameter to anim graph. Anim graph id '%d' is not valid.", animGraphID);
            return false;
        }

        // get the name and check if it is unique
        AZStd::string name;
        AZStd::string newName;
        parameters.GetValue("name", this, name);
        parameters.GetValue("newName", this, newName);

        // Get the parameter index.
        uint32 paramIndex = animGraph->FindParameterIndex(name.c_str());
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            paramIndex = animGraph->FindParameterIndex(newName.c_str());
        }

        // If the neither the former nor the new parameter exist, return.
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            if (newName.empty())
            {
                outResult.Format("There is no parameter with the name '%s'.", name.c_str());
            }
            else
            {
                outResult.Format("There is no parameter with the name '%s'.", newName.c_str());
            }

            return false;
        }

        if (!newName.empty())
        {
            RenameParameterNodePorts(animGraph, nullptr, name.c_str(), newName.c_str());

            // Update all transition conditions along with the parameter name change.
            UpdateTransitionConditions(name.c_str(), newName.c_str());

            name = newName;
        }

        // Construct and execute the command.
        AZStd::string commandString;
        if (!mOldDescription.empty())
        {
            commandString = AZStd::string::format("AnimGraphAdjustParameter -animGraphID %i -name \"%s\" -newName \"%s\" -description \"%s\" -interfaceType %i -minValue \"%s\" -maxValue \"%s\" -defaultValue \"%s\" -isScalable %s", animGraph->GetID(), name.c_str(), mOldName.c_str(), mOldDescription.c_str(), mOldInterfaceType, mOldMinValue.c_str(), mOldMaxValue.c_str(), mOldDefaultValue.c_str(), mOldIsScalable ? "true" : "false");
        }
        else
        {
            commandString = AZStd::string::format("AnimGraphAdjustParameter -animGraphID %i -name \"%s\" -newName \"%s\" -interfaceType %i -minValue \"%s\" -maxValue \"%s\" -defaultValue \"%s\" -isScalable %s", animGraph->GetID(), name.c_str(), mOldName.c_str(), mOldInterfaceType, mOldMinValue.c_str(), mOldMaxValue.c_str(), mOldDefaultValue.c_str(), mOldIsScalable ? "true" : "false");
        }

        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            AZ_Error("EMotionFX", false, outResult.AsChar());
        }

        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphAdjustParameter::InitSyntax()
    {
        GetSyntax().ReserveParameters(8);
        GetSyntax().AddRequiredParameter("animGraphID",    "The id of the anim graph.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name",            "The name of the parameter inside the currently selected anim graph to modify.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("interfaceType",           "The new interface type id.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("newName",                 "The new name of the parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("defaultValue",            "The new default value of the parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("minValue",                "The new minimum value of the parameter. In case of checkboxes or strings this parameter value will be ignored.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("maxValue",                "The new maximum value of the parameter. In case of checkboxes or strings this parameter value will be ignored.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("description",             "The new description of the parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("isScalable",              "Is this parameter scalable? This is used when scaling anim graphs.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
    }


    const char* CommandAnimGraphAdjustParameter::GetDescription() const
    {
        return "This command adjusts a anim graph parameter with given settings.";
    }


    //-------------------------------------------------------------------------------------
    // Swap two parameters
    //-------------------------------------------------------------------------------------
    CommandAnimGraphSwapParameters::CommandAnimGraphSwapParameters(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphSwapParameters", orgCommand)
    {
    }


    CommandAnimGraphSwapParameters::~CommandAnimGraphSwapParameters()
    {
    }


    bool CommandAnimGraphSwapParameters::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        AZStd::string what;
        AZStd::string with;
        parameters.GetValue("what", this, what);
        parameters.GetValue("with", this, with);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult.Format("Cannot swap parameters. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        // Get the indices for the parameters to swap.
        const uint32 whatIndex = animGraph->FindParameterIndex(what.c_str());
        const uint32 withIndex = animGraph->FindParameterIndex(with.c_str());
        if (whatIndex == MCORE_INVALIDINDEX32 || withIndex == MCORE_INVALIDINDEX32)
        {
            outResult = "Cannot swap parameters. Cannot get indices for parameters.";
            return false;
        }

        // Swap the parameters in the anim graph.
        animGraph->SwapParameters(whatIndex, withIndex);

        // Move the parameter to the new correct group.
        EMotionFX::AnimGraphParameterGroup* withGroup = animGraph->FindParameterGroupForParameter(withIndex);
        animGraph->RemoveParameterFromAllGroups(whatIndex);
        if (withGroup)
        {
            withGroup->AddParameter(whatIndex);
        }

        // Get an array of all parameter nodes.
        MCore::Array<EMotionFX::AnimGraphNode*> parameterNodes;
        animGraph->RecursiveCollectNodesOfType(EMotionFX::BlendTreeParameterNode::TYPE_ID, &parameterNodes);

        const uint32 numParamNodes = parameterNodes.GetLength();
        for (uint32 i = 0; i < numParamNodes; ++i)
        {
            EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(parameterNodes[i]);
            EMotionFX::AttributeParameterMask* parameterMaskAttribute = static_cast<EMotionFX::AttributeParameterMask*>(parameterNode->GetAttribute(EMotionFX::BlendTreeParameterNode::ATTRIB_MASK));

            // Sort the parameter mask so that it is in the newly swapped order.
            parameterMaskAttribute->Sort(parameterNode->GetAnimGraph());

            // In case we have a parameter mask active we need to map these to the local indices of the given parameter node.
            const uint32 localWhatIndex = parameterMaskAttribute->FindLocalParameterIndex(animGraph, animGraph->GetParameter(whatIndex)->GetName());
            const uint32 localWithIndex = parameterMaskAttribute->FindLocalParameterIndex(animGraph, animGraph->GetParameter(withIndex)->GetName());

            if (localWhatIndex != MCORE_INVALIDINDEX32 && localWithIndex != MCORE_INVALIDINDEX32)
            {
                parameterNode->GetOutputPort(localWhatIndex).mConnection = nullptr;
                parameterNode->GetOutputPort(localWithIndex).mConnection = nullptr;

                EMotionFX::AnimGraphNode* parentNode = parameterNode->GetParentNode();
                if (parentNode)
                {
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
                                if (connection->GetSourcePort() == localWithIndex)
                                {
                                    connection->SetSourcePort(static_cast<uint16>(localWhatIndex));
                                    parameterNode->GetOutputPort(localWhatIndex).mConnection = connection;
                                }
                                else if (connection->GetSourcePort() == localWhatIndex)
                                {
                                    connection->SetSourcePort(static_cast<uint16>(localWithIndex));
                                    parameterNode->GetOutputPort(localWithIndex).mConnection = connection;
                                }
                            }
                        }
                    }
                }
            }

            parameterNode->Reinit();
        }

        // Update all corresponding anim graph instances.
        const uint32 numInstances = EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i);
            if (animGraphInstance->GetAnimGraph() != animGraph)
            {
                continue;
            }

            // Swap the parameters and update the attributes.
            animGraphInstance->SwapParameterValues(whatIndex, withIndex);
            animGraphInstance->OnUpdateUniqueData();
        }

        // Save the current dirty flag and tell the anim graph that something got changed.
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        animGraph->RecursiveUpdateAttributes();

        // Update all corresponding anim graph instances.
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


    bool CommandAnimGraphSwapParameters::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        AZStd::string what;
        AZStd::string with;
        parameters.GetValue("what", this, what);
        parameters.GetValue("with", this, with);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult.Format("Cannot undo swap parameters. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        const AZStd::string commandString = AZStd::string::format("AnimGraphSwapParameters -animGraphID %i -what \"%s\" -with \"%s\"", animGraphID, with.c_str(), what.c_str());
        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            AZ_Error("EMotionFX", false, outResult.AsChar());

            return false;
        }

        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphSwapParameters::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("what", "The name of the parameter to swap to.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("with", "The name of the parameter to swap with.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    const char* CommandAnimGraphSwapParameters::GetDescription() const
    {
        return "This command swaps the two given parameters.";
    }

    //--------------------------------------------------------------------------------
    // Helper functions
    //--------------------------------------------------------------------------------

    //--------------------------------------------------------------------------------
    // Construct create parameter command strings
    //--------------------------------------------------------------------------------
    void ConstructCreateParameterCommand(AZStd::string& outResult, EMotionFX::AnimGraph* animGraph, const char* parameterName, uint32 interfaceType, const MCore::Attribute* minAttribute, const MCore::Attribute* maxAttribute, const MCore::Attribute* defaultAttribute, const AZStd::string& description, uint32 insertAtIndex, bool isScalable)
    {
        MCore::String minValue;
        MCore::String maxValue;
        MCore::String defaultValue;

        // Convert the values to strings.
        minAttribute->ConvertToString(minValue);
        maxAttribute->ConvertToString(maxValue);
        defaultAttribute->ConvertToString(defaultValue);

        // Build the command string.
        outResult = AZStd::string::format("AnimGraphCreateParameter -animGraphID %i -name \"%s\" -interfaceType %i -minValue \"%s\" -maxValue \"%s\" -defaultValue \"%s\" -isScalable %s", animGraph->GetID(), parameterName, interfaceType, minValue.AsChar(), maxValue.AsChar(), defaultValue.AsChar(), isScalable ? "true" : "false");

        if (!description.empty())
        {
            outResult += AZStd::string::format(" -description \"%s\"", description.c_str());
        }

        if (insertAtIndex != MCORE_INVALIDINDEX32)
        {
            outResult += AZStd::string::format(" -index \"%i\"", insertAtIndex);
        }
    }


    void ConstructCreateParameterCommand(AZStd::string& outResult, EMotionFX::AnimGraph* animGraph, MCore::AttributeSettings* attributeSettings, uint32 insertAtIndex, bool isScalable)
    {
        ConstructCreateParameterCommand(outResult, animGraph, attributeSettings->GetName(), attributeSettings->GetInterfaceType(), attributeSettings->GetMinValue(), attributeSettings->GetMaxValue(), attributeSettings->GetDefaultValue(), attributeSettings->GetDescription(), insertAtIndex, isScalable);
    }


    //--------------------------------------------------------------------------------
    // Remove and or clear parameter helper functions
    //--------------------------------------------------------------------------------
    void ClearParametersCommand(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup)
    {
        const uint32 numParameters = animGraph->GetNumParameters();
        if (numParameters == 0)
        {
            return;
        }

        // Iterate through all parameters and fill the parameter name array.
        AZStd::vector<AZStd::string> parameterNames;
        parameterNames.resize(static_cast<size_t>(numParameters));
        for (int32 i = numParameters - 1; i >= 0; --i)
        {
            parameterNames[i] = animGraph->GetParameter(i)->GetName();
        }

        // Is the command group parameter set?
        if (!commandGroup)
        {
            // Create and fill the command group.
            MCore::String outResult;
            MCore::CommandGroup internalCommandGroup("Clear parameters");
            RemoveParametersCommand(animGraph, parameterNames, &internalCommandGroup);

            // Execute the command group.
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, outResult))
            {
                AZ_Error("EMotionFX", false, outResult.AsChar());
            }
        }
        else
        {
            // Use the given parameter command group.
            RemoveParametersCommand(animGraph, parameterNames, commandGroup);
        }
    }


    bool RemoveParametersCommand(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& parameterNames, MCore::CommandGroup* commandGroup)
    {
        // Make sure the anim graph is valid and that the parameter names array at least contains a single one.
        if (!animGraph || parameterNames.empty())
        {
            return false;
        }

        // Get the parameter indices and sort them.
        MCore::Array<uint32> parameterIndices;
        const size_t numParameterNames = parameterNames.size();
        parameterIndices.Reserve(static_cast<uint32>(numParameterNames));
        for (const AZStd::string& parameterName : parameterNames)
        {
            // Find the index of the parameter in the anim graph.
            const uint32 parameterIndex = animGraph->FindParameterIndex(parameterName.c_str());
            if (parameterIndex != MCORE_INVALIDINDEX32)
            {
                parameterIndices.Add(parameterIndex);
            }
        }
        parameterIndices.Sort();
        const uint32 numParameterIndices = parameterIndices.GetLength();

        // Create the command group.
        MCore::String outResult;
        AZStd::string commandString;

        MCore::CommandGroup internalCommandGroup(commandString.c_str());
        MCore::CommandGroup* usedCommandGroup = commandGroup;
        if (!commandGroup)
        {
            if (numParameterNames == 1)
            {
                commandString = AZStd::string::format("Remove parameter '%s'", parameterNames[0].c_str());
            }
            else
            {
                commandString = AZStd::string::format("Remove %d parameters", numParameterNames);
            }

            usedCommandGroup = &internalCommandGroup;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 1: Get all parameter nodes in the anim graph
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        MCore::Array<EMotionFX::AnimGraphNode*> parameterNodes;
        animGraph->RecursiveCollectNodesOfType(EMotionFX::BlendTreeParameterNode::TYPE_ID, &parameterNodes);
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
                        const uint16 sourcePort     = connection->GetSourcePort();
                        const uint32 parameterIndex = parameterNode->GetParameterIndex(sourcePort);
                        const char*  parameterName  = animGraph->GetParameter(parameterIndex)->GetName();

                        if (AZStd::find(parameterNames.begin(), parameterNames.end(), parameterName) == parameterNames.end())
                        {
                            ParameterConnectionItem connectionItem;
                            connectionItem.SetParameterNodeName(parameterNode->GetName());
                            connectionItem.SetTargetNodeName(childNode->GetName());
                            connectionItem.SetParameterName(parameterName);
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
            if (!parameterNode->GetParameterIndices().GetIsEmpty())
            {
                EMotionFX::AttributeParameterMask* parameterMaskAttribute = static_cast<EMotionFX::AttributeParameterMask*>(parameterNode->GetAttribute(EMotionFX::BlendTreeParameterNode::ATTRIB_MASK));

                // Create the new parameter mask excluding the parameter we are removing.
                ConstructParameterMaskString(parameterMaskString, animGraph, parameterMaskAttribute, parameterNames);
                commandString = AZStd::string::format("AnimGraphAdjustNode -animGraphID %i -name \"%s\" -parameterMask \"%s\"", animGraph->GetID(), parameterNode->GetName(), parameterMaskString.c_str());
                usedCommandGroup->AddCommandString(commandString);
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 5: Remove the parameters from the anim graph
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        for (int32 p = numParameterIndices - 1; p >= 0; --p)
        {
            commandString = AZStd::string::format("AnimGraphRemoveParameter -animGraphID %i -name \"%s\"", animGraph->GetID(), animGraph->GetParameter(parameterIndices[p])->GetName());
            usedCommandGroup->AddCommandString(commandString);
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 6: Recreate the connections at the mapped/new ports
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        RecreateOldConnections(animGraph, oldParameterConnections, commandGroup, parameterNames);

        // Is the command group parameter set?
        if (!commandGroup)
        {
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, outResult))
            {
                AZ_Error("EMotionFX", false, outResult.AsChar());
                return false;
            }
        }

        return true;
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
            EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNode(connectionItem.GetParameterNodeName());
            if (!animGraphNode || animGraphNode->GetType() != EMotionFX::BlendTreeParameterNode::TYPE_ID)
            {
                continue;
            }

            const EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(animGraphNode);

            const uint32 parameterIndex = animGraph->FindParameterIndex(connectionItem.GetParameterName());
            MCORE_ASSERT(parameterIndex != MCORE_INVALIDINDEX32);
            const uint32 sourcePortNr = parameterNode->CalcNewPortForParameterIndex(parameterIndex, parametersToBeRemoved);

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


    void ConstructParameterMaskString(AZStd::string& outResult, EMotionFX::AnimGraph* animGraph, EMotionFX::AttributeParameterMask* parameterMaskAttribute, AZStd::vector<AZStd::string> excludeParameterNames)
    {
        // Clear the result string.
        outResult.clear();

        // Build the parameters mask string by adding all the parameter names.
        const uint32 numParameters = animGraph->GetNumParameters();
        for (uint32 i = 0; i < numParameters; ++i)
        {
            const char* parameterName = animGraph->GetParameter(i)->GetName();

            if (AZStd::find(excludeParameterNames.begin(), excludeParameterNames.end(), parameterName) == excludeParameterNames.end() &&
                parameterMaskAttribute->FindParameterNameIndex(parameterName) != MCORE_INVALIDINDEX32)
            {
                if (!outResult.empty())
                {
                    outResult += ';';
                }

                outResult += parameterName;
            }
        }
    }
} // namesapce EMotionFX