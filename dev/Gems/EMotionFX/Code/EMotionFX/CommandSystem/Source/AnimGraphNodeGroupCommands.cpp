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
#include "AnimGraphNodeGroupCommands.h"
#include "AnimGraphConnectionCommands.h"
#include "CommandManager.h"
#include <EMotionFX/Source/AnimGraph.h>
#include <MCore/Source/Random.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/ActorManager.h>


namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // CommandAnimGraphAdjustNodeGroup
    //--------------------------------------------------------------------------------

    // constructor
    CommandAnimGraphAdjustNodeGroup::CommandAnimGraphAdjustNodeGroup(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphAdjustNodeGroup", orgCommand)
    {
    }


    // destructor
    CommandAnimGraphAdjustNodeGroup::~CommandAnimGraphAdjustNodeGroup()
    {
    }


    MCore::String CommandAnimGraphAdjustNodeGroup::GenerateNodeNameString(EMotionFX::AnimGraph* animGraph, const MCore::Array<uint32>& nodeIDs)
    {
        if (nodeIDs.GetIsEmpty())
        {
            return "";
        }

        MCore::String result;
        const uint32 numNodes = nodeIDs.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNodeByID(nodeIDs[i]);
            if (animGraphNode == nullptr)
            {
                continue;
            }

            result += animGraphNode->GetName();
            if (i < numNodes - 1)
            {
                result += ';';
            }
        }

        return result;
    }


    // execute
    bool CommandAnimGraphAdjustNodeGroup::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // get the node group name
        MCore::String valueString;
        parameters.GetValue("name", this, &valueString);

        // find the node group index
        const uint32 groupIndex = animGraph->FindNodeGroupIndexByName(valueString.AsChar());
        if (groupIndex == MCORE_INVALIDINDEX32)
        {
            outResult.Format("Node group \"%s\" can not be found.", valueString.AsChar());
            return false;
        }

        // get a pointer to the node group and keep the old name
        EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(groupIndex);
        mOldName = nodeGroup->GetName();

        // is visible?
        if (parameters.CheckIfHasParameter("isVisible"))
        {
            const bool isVisible = parameters.GetValueAsBool("isVisible", this);
            mOldIsVisible = nodeGroup->GetIsVisible();
            nodeGroup->SetIsVisible(isVisible);
        }

        // background color
        if (parameters.CheckIfHasParameter("color"))
        {
            const AZ::Vector4 color = parameters.GetValueAsVector4("color", this);
            mOldColor = nodeGroup->GetColor();
            nodeGroup->SetColor(MCore::RGBAColor(color.GetX(), color.GetY(), color.GetZ(), color.GetW()));
        }

        // set the new name
        // if the new name is empty, the name is not changed
        parameters.GetValue("newName", this, &valueString);
        if (valueString.GetIsEmpty() == false)
        {
            nodeGroup->SetName(valueString.AsChar());
        }

        // check if parametes nodeNames is set
        if (parameters.CheckIfHasParameter("nodeNames"))
        {
            // keep the old nodes IDs
            mOldNodeIDs = nodeGroup->GetNodeArray();

            // get the node action
            parameters.GetValue("nodeAction", this, &valueString);

            // get the node names and split the string
            MCore::String nodeNameString;
            parameters.GetValue("nodeNames", this, &nodeNameString);
            MCore::Array<MCore::String> nodeNames = nodeNameString.Split();
            const uint32 numNodes = nodeNames.GetLength();

            // remove the selected nodes from the given node group
            if (valueString.CompareNoCase("remove") == 0)
            {
                // iterate through the nodes from the parameter node names array
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    // validate node
                    EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNode(nodeNames[i].AsChar());
                    if (animGraphNode == nullptr)
                    {
                        continue;
                    }

                    // remove the node from the given node group
                    nodeGroup->RemoveNodeByID(animGraphNode->GetID());
                }
            }
            else if (valueString.CompareNoCase("add") == 0) // add the selected nodes to the given node group
            {
                // iterate through the nodes from the parameter node names array
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    // validate node
                    EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNode(nodeNames[i].AsChar());
                    if (animGraphNode == nullptr)
                    {
                        continue;
                    }

                    // remove the node from all node groups
                    const uint32 numNodeGroups = animGraph->GetNumNodeGroups();
                    for (uint32 n = 0; n < numNodeGroups; ++n)
                    {
                        animGraph->GetNodeGroup(n)->RemoveNodeByID(animGraphNode->GetID());
                    }

                    // add the node to the given node group afterwards
                    nodeGroup->AddNode(animGraphNode->GetID());
                }
            }
            else if (valueString.CompareNoCase("replace") == 0) // clear the node group and then add the selected nodes to the given node group
            {
                // clear the node group upfront
                nodeGroup->RemoveAllNodes();

                // iterate through the nodes from the parameter node names array
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    // validate node
                    EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNode(nodeNames[i].AsChar());
                    if (animGraphNode == nullptr)
                    {
                        continue;
                    }

                    // remove the node from all node groups
                    const uint32 numNodeGroups = animGraph->GetNumNodeGroups();
                    for (uint32 n = 0; n < numNodeGroups; ++n)
                    {
                        animGraph->GetNodeGroup(n)->RemoveNodeByID(animGraphNode->GetID());
                    }

                    // add the node to the given node group afterwards
                    nodeGroup->AddNode(animGraphNode->GetID());
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


    // undo the command
    bool CommandAnimGraphAdjustNodeGroup::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph id and check if it is valid
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // construct the command
        MCore::String valueString;
        MCore::String commandString;
        commandString.Format("AnimGraphAdjustNodeGroup -animGraphID %i", animGraph->GetID());

        // set the old name or simply set the name if the name is not changed
        if (parameters.CheckIfHasParameter("newName"))
        {
            parameters.GetValue("newName", this, &valueString);
            commandString.FormatAdd(" -name \"%s\"", valueString.AsChar());
            commandString.FormatAdd(" -newName \"%s\"", mOldName.AsChar());
        }
        else
        {
            commandString.FormatAdd(" -name \"%s\"", mOldName.AsChar());
        }

        // set the old visible flag
        if (parameters.CheckIfHasParameter("isVisible"))
        {
            commandString.FormatAdd(" -isVisible %i", mOldIsVisible);
        }

        // set the old color
        if (parameters.CheckIfHasParameter("color"))
        {
            const AZ::Vector4 color(mOldColor.r, mOldColor.g, mOldColor.b, mOldColor.a);
            commandString.FormatAdd(" -color \"%s\"", MCore::String(color).AsChar());
        }

        // set the old nodes
        if (parameters.CheckIfHasParameter("nodeNames"))
        {
            MCore::String nodeNamesString = CommandAnimGraphAdjustNodeGroup::GenerateNodeNameString(animGraph, mOldNodeIDs);
            commandString.FormatAdd(" -nodeNames \"%s\" -nodeAction \"replace\"", nodeNamesString.AsChar());
        }

        // execute the command
        if (GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult) == false)
        {
            if (outResult.GetLength() > 0)
            {
                MCore::LogError(outResult.AsChar());
            }
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);

        return true;
    }


    // init the syntax of the command
    void CommandAnimGraphAdjustNodeGroup::InitSyntax()
    {
        GetSyntax().ReserveParameters(7);
        GetSyntax().AddRequiredParameter("name",    "The name of the node group to adjust.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("animGraphID",    "The id of the blend set the node group belongs to.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("isVisible",       "The visibility flag of the node group.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("newName",         "The new name of the node group.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("nodeNames",       "A list of node names that should be added/removed to/from the node group.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("nodeAction",      "The action to perform with the nodes passed to the command.", MCore::CommandSyntax::PARAMTYPE_STRING, "select");
        GetSyntax().AddParameter("color",           "The color to render the node group with.", MCore::CommandSyntax::PARAMTYPE_VECTOR4, "(1.0, 1.0, 1.0, 1.0)");
    }


    // get the description
    const char* CommandAnimGraphAdjustNodeGroup::GetDescription() const
    {
        return "This command can be used to adjust the node groups of the given anim graph.";
    }

    //--------------------------------------------------------------------------------
    // CommandAnimGraphAddNodeGroup
    //--------------------------------------------------------------------------------

    // constructor
    CommandAnimGraphAddNodeGroup::CommandAnimGraphAddNodeGroup(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphAddNodeGroup", orgCommand)
    {
    }


    // destructor
    CommandAnimGraphAddNodeGroup::~CommandAnimGraphAddNodeGroup()
    {
    }


    // execute
    bool CommandAnimGraphAddNodeGroup::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph id and check if it is valid
        const uint32            animGraphID    = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph*  animGraph      = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (animGraph == nullptr)
        {
            outResult.Format("Cannot add node group to anim graph. Anim graph id '%i' is not valid.", animGraphID);
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
            valueString.GenerateUniqueString("NodeGroup",   [&](const MCore::String& value)
                {
                    return (animGraph->FindNodeGroupByName(value.AsChar()) == nullptr);
                });
        }

        // add new node group to the anim graph
        EMotionFX::AnimGraphNodeGroup* nodeGroup = EMotionFX::AnimGraphNodeGroup::Create(valueString.AsChar());
        animGraph->AddNodeGroup(nodeGroup);

        // give the node group a random color
        const uint32 color32bit = MCore::GenerateColor();
        nodeGroup->SetColor(MCore::RGBAColor(color32bit));

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag   = animGraph->GetDirtyFlag();
        mOldName        = nodeGroup->GetName();
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
    bool CommandAnimGraphAddNodeGroup::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph id and check if it is valid
        const uint32            animGraphID    = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph*  animGraph      = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (animGraph == nullptr)
        {
            outResult.Format("Cannot add node group to anim graph. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        // construct the command
        MCore::String commandString;
        commandString.Format("AnimGraphRemoveNodeGroup -animGraphID %i -name \"%s\"", animGraphID, mOldName.AsChar());

        // execute the command
        MCore::String resultString;
        if (GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), resultString) == false)
        {
            if (resultString.GetLength() > 0)
            {
                MCore::LogError(resultString.AsChar());
            }
        }


        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandAnimGraphAddNodeGroup::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the blend set the node group belongs to.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("name", "The name of the node group.", MCore::CommandSyntax::PARAMTYPE_STRING, "Unnamed Node Group");
    }


    // get the description
    const char* CommandAnimGraphAddNodeGroup::GetDescription() const
    {
        return "This command can be used to add a new node group to the given anim graph.";
    }

    //--------------------------------------------------------------------------------
    // CommandAnimGraphRemoveNodeGroup
    //--------------------------------------------------------------------------------

    // constructor
    CommandAnimGraphRemoveNodeGroup::CommandAnimGraphRemoveNodeGroup(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphRemoveNodeGroup", orgCommand)
    {
    }


    // destructor
    CommandAnimGraphRemoveNodeGroup::~CommandAnimGraphRemoveNodeGroup()
    {
    }


    // execute
    bool CommandAnimGraphRemoveNodeGroup::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph id and check if it is valid
        const uint32            animGraphID    = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph*  animGraph      = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (animGraph == nullptr)
        {
            outResult.Format("Cannot add node group to anim graph. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        MCore::String valueString;
        parameters.GetValue("name", this, &valueString);

        // find the node group index and remove it
        const uint32 groupIndex = animGraph->FindNodeGroupIndexByName(valueString.AsChar());
        if (groupIndex == MCORE_INVALIDINDEX32)
        {
            outResult.Format("Cannot add node group to anim graph. Node group index is invalid.", groupIndex);
            return false;
        }

        // read out information for the command undo
        EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(groupIndex);
        mOldName        = nodeGroup->GetName();
        mOldColor       = nodeGroup->GetColor();
        mOldIsVisible   = nodeGroup->GetIsVisible();
        mOldNodeIDs     = nodeGroup->GetNodeArray();

        // remove the node group
        animGraph->RemoveNodeGroup(groupIndex);

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
    bool CommandAnimGraphRemoveNodeGroup::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph id and check if it is valid
        const uint32            animGraphID    = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph*  animGraph      = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (animGraph == nullptr)
        {
            outResult.Format("Cannot add node group to anim graph. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        // construct the command
        MCore::String valueString;
        MCore::CommandGroup commandGroup;
        valueString.Format("AnimGraphAddNodeGroup -animGraphID %i -name \"%s\"", animGraph->GetID(), mOldName.AsChar());
        commandGroup.AddCommandString(valueString.AsChar());

        MCore::String nodeNamesString = CommandAnimGraphAdjustNodeGroup::GenerateNodeNameString(animGraph, mOldNodeIDs);
        const AZ::Vector4 color(mOldColor.r, mOldColor.g, mOldColor.b, mOldColor.a);
        valueString.Format("AnimGraphAdjustNodeGroup -animGraphID %i -name \"%s\" -isVisible %i -color \"%s\" -nodeNames \"%s\" -nodeAction \"add\"", animGraph->GetID(), mOldName.AsChar(), mOldIsVisible, MCore::String(color).AsChar(), nodeNamesString.AsChar());
        commandGroup.AddCommandString(valueString.AsChar());

        // execute the command
        if (GetCommandManager()->ExecuteCommandGroupInsideCommand(commandGroup, valueString) == false)
        {
            if (valueString.GetLength() > 0)
            {
                MCore::LogError(valueString.AsChar());
            }
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandAnimGraphRemoveNodeGroup::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("animGraphID",    "The id of the blend set the node group belongs to.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name",            "The name of the node group to remove.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    // get the description
    const char* CommandAnimGraphRemoveNodeGroup::GetDescription() const
    {
        return "This command can be used to remove a node group from the given anim graph.";
    }


    //--------------------------------------------------------------------------------
    // Helper functions
    //--------------------------------------------------------------------------------

    void ClearNodeGroups(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup)
    {
        // get number of node groups
        const uint32 numNodeGroups = animGraph->GetNumNodeGroups();
        if (numNodeGroups == 0)
        {
            return;
        }

        // create the command group
        MCore::CommandGroup internalCommandGroup("Clear anim graph node groups");

        // get rid of all node groups
        MCore::String valueString;
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            // get pointer to the current actor instance
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(i);

            // construct the command
            valueString.Format("AnimGraphRemoveNodeGroup -animGraphID %i -name \"%s\"", animGraph->GetID(), nodeGroup->GetName());

            // add the command to the command group
            if (commandGroup == nullptr)
            {
                internalCommandGroup.AddCommandString(valueString.AsChar());
            }
            else
            {
                commandGroup->AddCommandString(valueString.AsChar());
            }
        }

        // execute the command or add it to the given command group
        if (commandGroup == nullptr)
        {
            if (GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, valueString) == false)
            {
                MCore::LogError(valueString.AsChar());
            }
        }
    }
} // namespace CommandSystem
