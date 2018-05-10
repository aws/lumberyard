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

#include "AnimGraphNodeCommands.h"
#include "CommandManager.h"
#include "AnimGraphConnectionCommands.h"

#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/AnimGraphMotionCondition.h>
#include <EMotionFX/Source/ActorManager.h>
#include <MCore/Source/AttributeSettings.h>


namespace CommandSystem
{
    AZStd::string GenerateUniqueNodeName(EMotionFX::AnimGraph* animGraph, const AZStd::string& namePrefix, const AZStd::string& typeString, const MCore::Array<AZStd::string>& nameReserveList = MCore::Array<AZStd::string>())
    {
        AZStd::string nameResult;

        if (namePrefix.empty())
        {
            nameResult = animGraph->GenerateNodeName(nameReserveList, typeString.c_str());
        }
        else
        {
            nameResult = animGraph->GenerateNodeName(nameReserveList, namePrefix.c_str());
        }

        // remove the AnimGraph prefix from the node names
        AzFramework::StringFunc::Replace(nameResult, "AnimGraph", "", true /* case sensitive */);

        // also remove the BlendTree prefix from all other nodes
        if (AzFramework::StringFunc::Equal(typeString.c_str(), "BlendTree", false /* no case */) == false)
        {
            AzFramework::StringFunc::Replace(nameResult, "BlendTree", "", true /* case sensitive */);
        }

        return nameResult;
    }


    AZStd::string GenerateUniqueNodeName(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node, const MCore::Array<AZStd::string>& nameReserveList)
    {
        if (node == nullptr)
        {
            return AZStd::string();
        }

        AZStd::string namePrefix    = node->GetName();
        AZStd::string typeString    = node->GetTypeString();

        const char* nameReadPtr = namePrefix.c_str();
        size_t newLength = namePrefix.size();
        for (size_t i = newLength; i > 0; i--)
        {
            char currentChar = nameReadPtr[i - 1];
            if (isdigit(currentChar) != 0)
            {
                newLength--;
            }
            else
            {
                break;
            }
        }

        if (newLength > 0)
        {
            namePrefix.resize(newLength);
        }

        return GenerateUniqueNodeName(animGraph, namePrefix, typeString, nameReserveList);
    }


    AZStd::string FindNodeNameInReservationList(EMotionFX::AnimGraphNode* node, const MCore::Array<EMotionFX::AnimGraphNode*>& copiedNodes, const MCore::Array<AZStd::string>& nameReserveList)
    {
        // in case the node got copied get the new name, if not just use name of the old anim graph node
        const uint32 nodeIndex = copiedNodes.Find(node);
        if (nodeIndex != MCORE_INVALIDINDEX32)
        {
            return nameReserveList[nodeIndex];
        }

        // return the node's name in case that is valid
        if (node)
        {
            return node->GetName();
        }

        // even the node is invalid, return an empty string
        return AZStd::string();
    }

    //-------------------------------------------------------------------------------------
    // Create a anim graph node
    //-------------------------------------------------------------------------------------

    // constructor
    CommandAnimGraphCreateNode::CommandAnimGraphCreateNode(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphCreateNode", orgCommand)
    {
        mNodeID = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandAnimGraphCreateNode::~CommandAnimGraphCreateNode()
    {
        mNodeID = MCORE_INVALIDINDEX32;
    }


    // execute
    bool CommandAnimGraphCreateNode::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        mAnimGraphID = animGraph->GetID();

        // find the graph
        EMotionFX::AnimGraphNode* parentNode = nullptr;
        AZStd::string parentName;
        parameters.GetValue("parentName", "", &parentName);
        if (parentName.empty() == false)
        {
            parentNode = animGraph->RecursiveFindNode(parentName.c_str());
            if (parentNode == nullptr)
            {
                outResult = AZStd::string::format("There is no anim graph node with name '%s' inside the selected/active anim graph.", parentName.c_str());
                return false;
            }
        }

        // get the type of the node to be created
        AZStd::string typeString;
        parameters.GetValue("type", this, &typeString);
        if (parentName.empty() && AzFramework::StringFunc::Equal(typeString.c_str(), "AnimGraphStateMachine", false /* no case */) == false)
        {
            outResult = AZStd::string::format("Cannot create node. Root nodes can only be of type AnimGraphStateMachine.");
            return false;
        }

        // get the name check if the name is unique or not
        AZStd::string name;
        parameters.GetValue("name", "", &name);
        if (name.empty() == false)
        {
            // if there is already a node with the same name
            if (animGraph->RecursiveFindNode(name.c_str()))
            {
                outResult = AZStd::string::format("Failed to create node, as there is already a node with name '%s'", name.c_str());
                return false;
            }
        }

        // try to create the node of the given type
        EMotionFX::AnimGraphObject* object = EMotionFX::GetAnimGraphManager().GetObjectFactory()->CreateObjectByTypeString(animGraph, typeString.c_str());
        if (object == nullptr)
        {
            outResult = AZStd::string::format("Failed to create node of type '%s'", typeString.c_str());
            return false;
        }

        // check if it is really a node
        if (object->GetBaseType() != EMotionFX::AnimGraphNode::BASETYPE_ID)
        {
            outResult = AZStd::string::format("Failed to create node of type '%s' as it is no AnimGraphNode inherited object.", typeString.c_str());
            return false;
        }

        // convert the object into a node
        EMotionFX::AnimGraphNode* node = static_cast<EMotionFX::AnimGraphNode*>(object);

        // if its a root node it has to be a state machine
        if (parentNode == nullptr)
        {
            if (node->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
            {
                node->Destroy();
                outResult = AZStd::string::format("Nodes without parents are only allowed to be state machines, cancelling creation!");
                return false;
            }
        }
        else
        {
            if (azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                if (!node->GetCanActAsState())
                {
                    outResult = AZStd::string::format("Node with name '%s' cannot be added to state machine as the node with type '%s' can not act as a state.", name.c_str(), typeString.c_str());
                    node->Destroy();
                    return false;
                }

                // Handle node types that are only allowed once as a child.
                const AZ::Uuid& nodeType = azrtti_typeid(node);
                if (node->GetCanHaveOnlyOneInsideParent() && parentNode->HasChildNodeOfType(nodeType))
                {
                    outResult = AZStd::string::format("Node with name '%s' and type '%s' ('%s') cannot be added to state machine as a node with the given type already exists. Multiple nodes with of this type per state machine are not allowed.", name.c_str(), typeString.c_str(), nodeType.ToString<AZStd::string>().c_str());
                    node->Destroy();
                    return false;
                }
            }

            // Avoid creating states that are only used by sub state machine the lower in hierarchy levels.
            if (node->GetCanBeInsideSubStateMachineOnly() && EMotionFX::AnimGraphStateMachine::GetHierarchyLevel(parentNode) < 2)
            {
                outResult = AZStd::string::format("Node with name '%s' cannot get added to '%s' as the node with type '%s' is only used for sub state machines.", name.c_str(), parentNode->GetTypeString(), typeString.c_str());
                node->Destroy();
                return false;
            }
        }

        // if the name is not empty, set it
        if (name.empty() == false)
        {
            if (AzFramework::StringFunc::Equal(name.c_str(), "GENERATE", false /* no case */))
            {
                AZStd::string namePrefix;
                parameters.GetValue("namePrefix", this, &namePrefix);
                name = GenerateUniqueNodeName(animGraph, namePrefix, typeString);
            }

            node->SetName(name.c_str());
        }

        // now that the node is created, adjust its position
        const int32 xPos = parameters.GetValueAsInt("xPos", this);
        const int32 yPos = parameters.GetValueAsInt("yPos", this);
        node->SetVisualPos(xPos, yPos);

        // set the new value to the enabled flag
        if (parameters.CheckIfHasParameter("enabled"))
        {
            node->SetIsEnabled(parameters.GetValueAsBool("enabled", this));
        }

        // set the new value to the visualization flag
        if (parameters.CheckIfHasParameter("visualize"))
        {
            node->SetVisualization(parameters.GetValueAsBool("visualize", this));
        }

        // set the attributes from a string
        if (parameters.CheckIfHasParameter("attributesString"))
        {
            AZStd::string attributesString;
            parameters.GetValue("attributesString", this, &attributesString);
            node->InitAttributesFromString(attributesString.c_str());
        }

        // collapse the node if expected
        const bool collapsed = parameters.GetValueAsBool("collapsed", this);
        node->SetIsCollapsed(collapsed);

        // store the node id for the callbacks
        mNodeID = node->GetID();

        // store the anim graph id for undo
        mAnimGraphID = animGraph->GetID();

        // check if the parent is valid
        if (parentNode)
        {
            // add the node in the parent
            parentNode->AddChildNode(node);

            // in case the parent node is a state machine
            if (azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                // type cast the parent node to a state machine
                EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

                // in case this is the first state we add to the state machine, default it to the entry state
                if (stateMachine->GetNumChildNodes() == 1)
                {
                    stateMachine->SetEntryState(node);
                }
            }
        }
        else
        {
            //animGraph->AddStateMachine( (EMotionFX::AnimGraphStateMachine*)node );
            MCORE_ASSERT(false);
            MCore::LogError("Cannot add node at root level.");
        }

        // handle blend tree final node separately
        if (parentNode && parentNode->GetType() == EMotionFX::BlendTree::TYPE_ID)
        {
            if (node->GetType() == EMotionFX::BlendTreeFinalNode::TYPE_ID)
            {
                ((EMotionFX::BlendTree*)parentNode)->SetFinalNode((EMotionFX::BlendTreeFinalNode*)node);
            }
        }

        // init the node
        node->InitForAnimGraph(animGraph);

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        // return the node name
        outResult = node->GetNameString();

        // call the post create node event
        EMotionFX::GetEventManager().OnCreatedNode(animGraph, node);

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

        // init new node for all anim graph instances belonging to it
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetActorManager().GetActorInstance(i)->GetAnimGraphInstance();
            if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
            {
                node->Init(animGraphInstance);

                // activate the state automatically in all animgraph instances
                if (parentNode->GetType() == EMotionFX::AnimGraphStateMachine::TYPE_ID)
                {
                    if (parentNode->GetNumChildNodes() == 1)
                    {
                        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);
                        stateMachine->SwitchToState(animGraphInstance, node);
                    }
                }
            }
        }

        return true;
    }


    // undo the command
    bool CommandAnimGraphCreateNode::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", mAnimGraphID);
            return false;
        }

        // locate the node
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeByID(mNodeID);
        if (node == nullptr)
        {
            return false;
        }

        mNodeID = MCORE_INVALIDINDEX32;

        AZStd::string commandString;
        commandString = AZStd::string::format("AnimGraphRemoveNode -animGraphID %i -name \"%s\"", animGraph->GetID(), node->GetName());
        if (GetCommandManager()->ExecuteCommandInsideCommand(commandString.c_str(), outResult) == false)
        {
            if (outResult.size() > 0)
            {
                MCore::LogError(outResult.c_str());
            }
            return false;
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandAnimGraphCreateNode::InitSyntax()
    {
        // required
        GetSyntax().ReserveParameters(12);
        GetSyntax().AddRequiredParameter("type",            "The node type string.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("animGraphID",            "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("parentName",              "The name of the parent node to add it to.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("name",                    "The name of the node, set to GENERATE to automatically generate a unique name.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("xPos",                    "The x position of the upper left corner in the visual graph.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("yPos",                    "The y position of the upper left corner in the visual graph.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("collapsed",               "The node collapse flag. This is only for the visual representation and does not affect the functionality.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
        GetSyntax().AddParameter("center",                  "Center the created node around the mouse cursor or not.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("namePrefix",              "The prefix of the name, when the name is set to GENERATE.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("attributesString",        "The node attributes as string.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("enabled",                 "Is the node enabled?", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("visualize",               "Is the node visualized?", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
    }


    // get the description
    const char* CommandAnimGraphCreateNode::GetDescription() const
    {
        return "This command creates a anim graph node of a given type. It returns the node name if successful.";
    }



    //-------------------------------------------------------------------------------------
    // AnimGraphAdjustNode - adjust node settings
    //-------------------------------------------------------------------------------------

    // constructor
    CommandAnimGraphAdjustNode::CommandAnimGraphAdjustNode(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphAdjustNode", orgCommand)
    {
        mNodeID     = MCORE_INVALIDINDEX32;
        mOldPosX    = 0;
        mOldPosY    = 0;
    }


    // destructor
    CommandAnimGraphAdjustNode::~CommandAnimGraphAdjustNode()
    {
    }


    // execute
    bool CommandAnimGraphAdjustNode::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
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
        parameters.GetValue("name", "", &name);

        // find the node in the anim graph
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNode(name.c_str());
        if (node == nullptr)
        {
            outResult = AZStd::string::format("Cannot find node with name '%s' in anim graph '%s'", name.c_str(), animGraph->GetName());
            return false;
        }

        // get the x and y pos
        int32 xPos = node->GetVisualPosX();
        int32 yPos = node->GetVisualPosY();

        mOldPosX = node->GetVisualPosX();
        mOldPosY = node->GetVisualPosY();

        // get the new position values
        if (parameters.CheckIfHasParameter("xPos"))
        {
            xPos = parameters.GetValueAsInt("xPos", this);
        }

        if (parameters.CheckIfHasParameter("yPos"))
        {
            yPos = parameters.GetValueAsInt("yPos", this);
        }

        // set the new name
        AZStd::string newName;
        parameters.GetValue("newName", this, &newName);
        if (newName.size() > 0)
        {
            // find the node group the node was in before the name change
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->FindNodeGroupForNode(node);
            mNodeGroupName.clear();
            if (nodeGroup)
            {
                // remember the node group name for undo
                mNodeGroupName = nodeGroup->GetName();

                // remove the node from the node group as its id is going to change
                nodeGroup->RemoveNodeByID(node->GetID());
            }

            mOldName = node->GetName();
            node->SetName(newName.c_str());

            // as the id of the node changed after renaming it, we have to readd the node with the new id
            if (nodeGroup)
            {
                nodeGroup->AddNode(node->GetID());
            }

            // call the post rename node event
            EMotionFX::GetEventManager().OnRenamedNode(animGraph, node, mOldName);
        }

        // remember and set the new value to the enabled flag
        mOldEnabled = node->GetIsEnabled();
        if (parameters.CheckIfHasParameter("enabled"))
        {
            node->SetIsEnabled(parameters.GetValueAsBool("enabled", this));
        }

        // remember and set the new value to the visualization flag
        mOldVisualized = node->GetIsVisualizationEnabled();
        if (parameters.CheckIfHasParameter("visualize"))
        {
            node->SetVisualization(parameters.GetValueAsBool("visualize", this));
        }

        // adjust the position
        node->SetVisualPos(xPos, yPos);
        mNodeID = node->GetID();

        // do only for parameter nodes
        if (node->GetType() == EMotionFX::BlendTreeParameterNode::TYPE_ID && parameters.CheckIfHasParameter("parameterMask"))
        {
            // type cast to a parameter node
            EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(node);

            // remember the old attribute mask for undo
            mOldParameterMask.clear();
            const MCore::Array<uint32>& parameterIndices = parameterNode->GetParameterIndices();
            const uint32 numParameters = parameterIndices.GetLength();
            for (uint32 i = 0; i < numParameters; ++i)
            {
                if (i == numParameters - 1)
                {
                    mOldParameterMask += animGraph->GetParameter(parameterIndices[i])->GetName();
                }
                else
                {
                    mOldParameterMask += animGraph->GetParameter(parameterIndices[i])->GetName();
                    mOldParameterMask += "\n";
                }
            }

            // get the new parameter mask and set it
            AZStd::string newParameterMaskString;
            parameters.GetValue("parameterMask", this, &newParameterMaskString);
            AZStd::vector<AZStd::string> newParameterMask;
            AzFramework::StringFunc::Tokenize(newParameterMaskString.c_str(), newParameterMask, MCore::CharacterConstants::semiColon, true /* keep empty strings */, true /* keep space strings */);

            // get the parameter mask attribute and update the mask
            EMotionFX::AttributeParameterMask* parameterMaskAttribute = static_cast<EMotionFX::AttributeParameterMask*>(parameterNode->GetAttribute(EMotionFX::BlendTreeParameterNode::ATTRIB_MASK));
            parameterMaskAttribute->SetParameters(newParameterMask);
            parameterMaskAttribute->Sort(parameterNode->GetAnimGraph());
            parameterNode->Reinit();
        }

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        // only update attributes in case it is wanted
        if (parameters.GetValueAsBool("updateAttributes", this))
        {
            // recursively update attributes of all nodes and return success
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
        }

        return true;
    }


    // undo the command
    bool CommandAnimGraphAdjustNode::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", mAnimGraphID);
            return false;
        }

        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeByID(mNodeID);
        if (node == nullptr)
        {
            outResult = AZStd::string::format("Cannot find node with ID %d.", mNodeID);
            return false;
        }

        // restore the name
        if (mOldName.size() > 0)
        {
            AZStd::string currentName = node->GetName();

            // find the node group the node was in before the name change
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->FindNodeGroupForNode(node);

            // remove the node from the node group as its id is going to change
            if (nodeGroup)
            {
                nodeGroup->RemoveNodeByID(node->GetID());
            }

            node->SetName(mOldName.c_str());

            // as the id of the node changed after renaming it, we have to readd the node with the new id
            if (nodeGroup)
            {
                nodeGroup->AddNode(node->GetID());
            }

            // call the post rename node event
            EMotionFX::GetEventManager().OnRenamedNode(animGraph, node, currentName);
        }

        mNodeID = node->GetID();
        node->SetVisualPos(mOldPosX, mOldPosY);

        // set the old values to the enabled flag and the visualization flag
        node->SetIsEnabled(mOldEnabled);
        node->SetVisualization(mOldVisualized);

        // do only for parameter nodes
        if (node->GetType() == EMotionFX::BlendTreeParameterNode::TYPE_ID && parameters.CheckIfHasParameter("parameterMask"))
        {
            // type cast to a parameter node
            EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(node);

            // get the parameter mask attribute and update the mask
            EMotionFX::AttributeParameterMask* parameterMaskAttribute = static_cast<EMotionFX::AttributeParameterMask*>(parameterNode->GetAttribute(EMotionFX::BlendTreeParameterNode::ATTRIB_MASK));
            parameterMaskAttribute->InitFromString(mOldParameterMask);
            parameterMaskAttribute->Sort(parameterNode->GetAnimGraph());
            parameterNode->Reinit();
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);

        // recursively update attributes of all nodes and return success
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
    void CommandAnimGraphAdjustNode::InitSyntax()
    {
        // required
        GetSyntax().ReserveParameters(8);
        GetSyntax().AddRequiredParameter("animGraphID",    "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name",            "The name of the node to modify.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("newName",                 "The new name of the node.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("xPos",                    "The new x position of the upper left corner in the visual graph.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("yPos",                    "The new y position of the upper left corner in the visual graph.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("enabled",                 "Is the node enabled?", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("visualize",               "Is the node visualized?", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
        GetSyntax().AddParameter("updateAttributes",        "Update attributes afterwards?", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("parameterMask",           "The new parameter mask. Parameter names separated by semicolon.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    // get the description
    const char* CommandAnimGraphAdjustNode::GetDescription() const
    {
        return "This command adjust properties of a given anim graph node.";
    }


    //-------------------------------------------------------------------------------------
    // Remove a anim graph node
    //-------------------------------------------------------------------------------------
    // constructor
    CommandAnimGraphRemoveNode::CommandAnimGraphRemoveNode(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphRemoveNode", orgCommand)
    {
        mNodeID     = MCORE_INVALIDINDEX32;
        mParentID   = MCORE_INVALIDINDEX32;
        mIsEntryNode= false;
    }


    // destructor
    CommandAnimGraphRemoveNode::~CommandAnimGraphRemoveNode()
    {
    }


    // execute
    bool CommandAnimGraphRemoveNode::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        mAnimGraphID = animGraph->GetID();

        // find the emfx node
        AZStd::string name;
        parameters.GetValue("name", "", &name);
        EMotionFX::AnimGraphNode* emfxNode = animGraph->RecursiveFindNode(name.c_str());
        if (emfxNode == nullptr)
        {
            outResult = AZStd::string::format("There is no node with the name '%s'", name.c_str());
            return false;
        }

        mType               = emfxNode->GetTypeString();
        mName               = emfxNode->GetName();
        mPosX               = emfxNode->GetVisualPosX();
        mPosY               = emfxNode->GetVisualPosY();
        mCollapsed          = emfxNode->GetIsCollapsed();
        mOldAttributesString = emfxNode->CreateAttributesString();

        // store the node ID
        mNodeID             = emfxNode->GetID();

        // remember the node group for the node for undo
        mNodeGroupName.clear();
        EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->FindNodeGroupForNode(emfxNode);
        if (nodeGroup)
        {
            mNodeGroupName = nodeGroup->GetName();
        }

        // get the parent node
        EMotionFX::AnimGraphNode* parentNode = emfxNode->GetParentNode();
        if (parentNode)
        {
            if (azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);
                if (stateMachine->GetEntryState() == emfxNode)
                {
                    mIsEntryNode = true;

                    // Find a new entry node if we can
                    //--------------------------
                    // Find alternative entry state.
                    EMotionFX::AnimGraphNode* newEntryState = nullptr;
                    uint32 numStates = stateMachine->GetNumChildNodes();
                    for (uint32 s = 0; s < numStates; ++s)
                    {
                        EMotionFX::AnimGraphNode* childNode = stateMachine->GetChildNode(s);
                        if (childNode != emfxNode)
                        {
                            newEntryState = childNode;
                            break;
                        }
                    }

                    // Check if we've found a new possible entry state.
                    if (newEntryState)
                    {
                        AZStd::string commandString = AZStd::string::format("AnimGraphSetEntryState -animGraphID %i -entryNodeName \"%s\"", animGraph->GetID(), newEntryState->GetName());
                        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString.c_str(), outResult))
                        {
                            AZ_Error("EMotionFX", false, outResult.c_str());
                        }
                    }
                    //--------------------------

                }
            }

            mParentName = parentNode->GetName();
            mParentID = parentNode->GetID();

            // call the pre remove node event
            EMotionFX::GetEventManager().OnRemoveNode(animGraph, emfxNode);

            // remove all unique datas for the node
            animGraph->RemoveAllObjectData(emfxNode, true);

            // remove the actual node
            parentNode->RemoveChildNodeByPointer(emfxNode);
        }
        else
        {
            mParentName.clear();
            MCore::LogError("Cannot remove root state machine.");
            MCORE_ASSERT(false);
            return false;
        }

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        // recursively update attributes of all nodes
        animGraph->RecursiveUpdateAttributes();

        // update all attributes recursively for all anim graph instances
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
    bool CommandAnimGraphRemoveNode::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", mAnimGraphID);
            return false;
        }

        // create the node again
        MCore::CommandGroup group("Recreating node");
        AZStd::string commandString;
        commandString.reserve(16192);
        if (mParentName.size() > 0)
        {
            commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"%s\" -parentName \"%s\" -name \"%s\" -xPos %d -yPos %d -collapsed %s -center false -attributesString \"%s\"", 
                animGraph->GetID(), 
                mType.c_str(), 
                mParentName.c_str(), 
                mName.c_str(), 
                mPosX, 
                mPosY, 
                AZStd::to_string(mCollapsed).c_str(),
                mOldAttributesString.c_str());
            group.AddCommandString( commandString );

            if (mIsEntryNode)
            {
                commandString = AZStd::string::format("AnimGraphSetEntryState -animGraphID %i -entryNodeName \"%s\"", animGraph->GetID(), mName.c_str());
                group.AddCommandString( commandString );
            }
        }
        else
        {
            commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"%s\" -name \"%s\" -xPos %d -yPos %d -collapsed %s -center false -attributesString \"%s\"", 
                animGraph->GetID(), 
                mType.c_str(), 
                mName.c_str(), 
                mPosX, 
                mPosY, 
                AZStd::to_string(mCollapsed).c_str(),
                mOldAttributesString.c_str());
            group.AddCommandString(commandString);
        }

        if (!GetCommandManager()->ExecuteCommandGroupInsideCommand(group, outResult))
        {
            if (outResult.size() > 0)
            {
                MCore::LogError(outResult.c_str());
            }

            return false;
        }

        // add it to the old node group if it was assigned to one before
        if (mNodeGroupName.empty() == false)
        {
            commandString = AZStd::string::format("AnimGraphAdjustNodeGroup -animGraphID %i -name \"%s\" -nodeNames \"%s\" -nodeAction \"add\"", animGraph->GetID(), mNodeGroupName.c_str(), mName.c_str());
            if (GetCommandManager()->ExecuteCommandInsideCommand(commandString.c_str(), outResult) == false)
            {
                if (outResult.size() > 0)
                {
                    MCore::LogError(outResult.c_str());
                }

                return false;
            }
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandAnimGraphRemoveNode::InitSyntax()
    {
        // required
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name", "The name of the node to remove.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    // get the description
    const char* CommandAnimGraphRemoveNode::GetDescription() const
    {
        return "This command removes a anim graph nodewith a given name.";
    }

    //-------------------------------------------------------------------------------------
    // Set the entry state of a state machine
    //-------------------------------------------------------------------------------------

    // constructor
    CommandAnimGraphSetEntryState::CommandAnimGraphSetEntryState(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphSetEntryState", orgCommand)
    {
        mOldEntryStateNodeID    = MCORE_INVALIDINDEX32;
        mOldStateMachineNodeID  = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandAnimGraphSetEntryState::~CommandAnimGraphSetEntryState()
    {
    }


    // execute
    bool CommandAnimGraphSetEntryState::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        mAnimGraphID = animGraph->GetID();

        AZStd::string entryNodeName;
        parameters.GetValue("entryNodeName", this, &entryNodeName);

        // find the entry anim graph node
        EMotionFX::AnimGraphNode* entryNode = animGraph->RecursiveFindNode(entryNodeName.c_str());
        if (entryNode == nullptr)
        {
            outResult = AZStd::string::format("There is no entry node with the name '%s'", entryNodeName.c_str());
            return false;
        }

        // check if the parent node is a state machine
        EMotionFX::AnimGraphNode*  stateMachineNode = entryNode->GetParentNode();
        if (stateMachineNode == nullptr || stateMachineNode->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            outResult = AZStd::string::format("Cannot set entry node '%s'. Parent node is not a state machine or not valid at all.", entryNodeName.c_str());
            return false;
        }

        // get the parent state machine
        EMotionFX::AnimGraphStateMachine* stateMachine = (EMotionFX::AnimGraphStateMachine*)stateMachineNode;

        // store the id of the old entry node
        EMotionFX::AnimGraphNode* oldEntryNode = stateMachine->GetEntryState();
        if (oldEntryNode)
        {
            mOldEntryStateNodeID = oldEntryNode->GetID();
        }

        // store the id of the state machine
        mOldStateMachineNodeID = stateMachineNode->GetID();

        // set the new entry state for the state machine
        stateMachine->SetEntryState(entryNode);

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
    bool CommandAnimGraphSetEntryState::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", mAnimGraphID);
            return false;
        }

        // get the state machine
        EMotionFX::AnimGraphNode*  stateMachineNode = animGraph->RecursiveFindNodeByID(mOldStateMachineNodeID);
        if (stateMachineNode == nullptr || stateMachineNode->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            outResult = "Cannot undo set entry node. Parent node is not a state machine or not valid at all.";
            return false;
        }

        // get the parent state machine
        EMotionFX::AnimGraphStateMachine* stateMachine = (EMotionFX::AnimGraphStateMachine*)stateMachineNode;

        // find the entry anim graph node
        if (mOldEntryStateNodeID != MCORE_INVALIDINDEX32)
        {
            EMotionFX::AnimGraphNode* entryNode = animGraph->RecursiveFindNodeByID(mOldEntryStateNodeID);
            if (entryNode == nullptr)
            {
                outResult = "Cannot undo set entry node. Old entry node cannot be found.";
                return false;
            }

            // set the old entry state for the state machine
            stateMachine->SetEntryState(entryNode);
        }
        else
        {
            // set the old entry state for the state machine
            stateMachine->SetEntryState(nullptr);
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
    void CommandAnimGraphSetEntryState::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("entryNodeName", "The name of the new entry node.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("animGraphID", "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
    }


    // get the description
    const char* CommandAnimGraphSetEntryState::GetDescription() const
    {
        return "This command sets the entry state of a state machine.";
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void CreateAnimGraphNode(EMotionFX::AnimGraph* animGraph, const AZStd::string& typeString, const AZStd::string& namePrefix, EMotionFX::AnimGraphNode* parentNode, int32 offsetX, int32 offsetY, const AZStd::string& attributesString)
    {
        AZStd::string commandString, commandResult;

        // if we want to add a blendtree, we also should automatically add a final node
        if (AzFramework::StringFunc::Equal(typeString.c_str(), "BlendTree", false /* no case */))
        {
            MCORE_ASSERT(parentNode);
            MCore::CommandGroup group("Create blend tree");

            commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"%s\" -parentName \"%s\" -xPos %d -yPos %d -name GENERATE -namePrefix \"%s\"", animGraph->GetID(), typeString.c_str(), parentNode->GetName(), offsetX, offsetY, namePrefix.c_str());

            if (attributesString.empty() == false)
            {
                commandString += AZStd::string::format(" -attributesString \"%s\" ", attributesString.c_str());
            }

            group.AddCommandString(commandString.c_str());

            // auto create the final node
            commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"BlendTreeFinalNode\" -parentName \"%%LASTRESULT%%\" -xPos %d -yPos %d -name GENERATE -namePrefix \"FinalNode\"", animGraph->GetID(), 0, 0);
            group.AddCommandString(commandString.c_str());

            // execute the command
            if (GetCommandManager()->ExecuteCommandGroup(group, commandResult) == false)
            {
                if (commandResult.empty() == false)
                {
                    MCore::LogError(commandResult.c_str());
                }
            }
        }
        // if we want to add a sub state machine, we also should automatically add an exit node
        else if (AzFramework::StringFunc::Equal(typeString.c_str(), "AnimGraphStateMachine", false /* no case */) && EMotionFX::AnimGraphStateMachine::GetHierarchyLevel(parentNode) > 0)
        {
            MCORE_ASSERT(parentNode);
            MCore::CommandGroup group("Create child state machine");

            commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"%s\" -parentName \"%s\" -xPos %d -yPos %d -name GENERATE -namePrefix \"%s\"", animGraph->GetID(), typeString.c_str(), parentNode->GetName(), offsetX, offsetY, namePrefix.c_str());

            if (attributesString.empty() == false)
            {
                commandString += AZStd::string::format(" -attributesString \"%s\" ", attributesString.c_str());
            }

            group.AddCommandString(commandString.c_str());

            // auto create an exit node in case we're not creating a state machine inside a blend tree
            if (parentNode->GetType() != EMotionFX::BlendTree::TYPE_ID)
            {
                commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"AnimGraphEntryNode\" -parentName \"%%LASTRESULT%%\" -xPos %d -yPos %d -name GENERATE -namePrefix \"EntryNode\"", animGraph->GetID(), -200, 0);
                group.AddCommandString(commandString.c_str());

                commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"AnimGraphExitNode\" -parentName \"%%LASTRESULT2%%\" -xPos %d -yPos %d -name GENERATE -namePrefix \"ExitNode\"", animGraph->GetID(), 200, 0);
                group.AddCommandString(commandString.c_str());
            }

            // execute the command
            if (GetCommandManager()->ExecuteCommandGroup(group, commandResult) == false)
            {
                if (commandResult.empty() == false)
                {
                    MCore::LogError(commandResult.c_str());
                }
            }
        }
        else
        {
            if (parentNode)
            {
                commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"%s\" -parentName \"%s\" -xPos %d -yPos %d -name GENERATE -namePrefix \"%s\"", animGraph->GetID(), typeString.c_str(), parentNode->GetName(), offsetX, offsetY, namePrefix.c_str());
            }
            else
            {
                commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"%s\" -xPos %d -yPos %d -name GENERATE -namePrefix \"%s\"", animGraph->GetID(), typeString.c_str(), offsetX, offsetY, namePrefix.c_str());
            }

            if (attributesString.empty() == false)
            {
                commandString += AZStd::string::format(" -attributesString \"%s\" ", attributesString.c_str());
            }

            // execute the command
            if (GetCommandManager()->ExecuteCommand(commandString.c_str(), commandResult) == false)
            {
                if (commandResult.empty() == false)
                {
                    MCore::LogError(commandResult.c_str());
                }
            }
        }
    }


    void DeleteNode(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node, AZStd::vector<EMotionFX::AnimGraphNode*>& nodeList, AZStd::vector<EMotionFX::BlendTreeConnection*>& connectionList, AZStd::vector<EMotionFX::AnimGraphStateTransition*>& transitionList, bool recursive, bool firstRootIteration = true, bool autoChangeEntryStates = true)
    {
        MCORE_UNUSED(recursive);
        MCORE_UNUSED(autoChangeEntryStates);

        if (!node)
        {
            return;
        }

        // Skip directly if the node is already in the list.
        if(AZStd::find(nodeList.begin(), nodeList.end(), node) != nodeList.end())
        {
            return;
        }

        // Only delete nodes that are also deletable, final nodes e.g. can't be cut and deleted.
        if (firstRootIteration && !node->GetIsDeletable())
        {
            return;
        }

        // Check if the last instance is not deletable while all others are.
        if (firstRootIteration && !node->GetIsLastInstanceDeletable())
        {
            EMotionFX::AnimGraphNode* parentNode = node->GetParentNode();
            if (parentNode)
            {
                // Gather the number of nodes with the same type as the one we're trying to remove.
                MCore::Array<EMotionFX::AnimGraphNode*> outNodes;
                parentNode->CollectChildNodesOfType(node->GetType(), &outNodes);
                const uint32 numTypeNodes = outNodes.GetLength();

                // Gather the number of already removed nodes with the same type as the one we're trying to remove.
                const size_t numTotalDeletedNodes = nodeList.size();
                uint32 numTypeDeletedNodes = 0;
                for (size_t i = 0; i < numTotalDeletedNodes; ++i)
                {
                    // Check if the nodes have the same parent, meaning they are in the same graph plus check if they have the same type
                    // if that both is the same we can increase the number of deleted nodes for the graph where the current node is in.
                    if (nodeList[i]->GetParentNode() == parentNode && nodeList[i]->GetType() == node->GetType())
                    {
                        numTypeDeletedNodes++;
                    }
                }

                // In case there the number of nodes with the same type as the given node is bigger than the number of already removed nodes + 1 means there is
                // only one node left with the given type return directly without deleting the node as we're not allowed to remove the last instance of the node.
                if (numTypeDeletedNodes + 1 >= numTypeNodes)
                {
                    return;
                }
            }
        }


        /////////////////////////
        // 1. Delete all connections and transitions that are connected to the node.

        // Delete all incoming and outgoing connections from current node.
        DeleteNodeConnections(commandGroup, node, node->GetParentNode(), connectionList, true);
        DeleteStateTransitions(commandGroup, node, node->GetParentNode(), transitionList, true);

        /////////////////////////
        // 2. Delete all child nodes recursively before deleting the node.

        // Get the number of child nodes, iterate through them and recursively call the function.
        const uint32 numChildNodes = node->GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            EMotionFX::AnimGraphNode* childNode = node->GetChildNode(i);
            DeleteNode(commandGroup, animGraph, childNode, nodeList, connectionList, transitionList, true, false, false);
        }     

        /////////////////////////
        // 3. Delete the node.

        // Add the remove node to the command group.
        AZStd::string commandString = AZStd::string::format("AnimGraphRemoveNode -animGraphID %i -name \"%s\"", node->GetAnimGraph()->GetID(), node->GetName());

        nodeList.push_back(node);
        commandGroup->AddCommandString(commandString);
    }


    void DeleteNodes(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& nodeNames, AZStd::vector<EMotionFX::AnimGraphNode*>& nodeList, AZStd::vector<EMotionFX::BlendTreeConnection*>& connectionList, AZStd::vector<EMotionFX::AnimGraphStateTransition*>& transitionList, bool autoChangeEntryStates)
    {
        const size_t numNodeNames = nodeNames.size();
        for (size_t i = 0; i < numNodeNames; ++i)
        {
            EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNode(nodeNames[i].c_str());

            // Add the delete node commands to the command group.
            DeleteNode(commandGroup, animGraph, node, nodeList, connectionList, transitionList, true, true, autoChangeEntryStates);
        }
    }


    void DeleteNodes(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& nodeNames)
    {
        MCore::CommandGroup commandGroup("Delete anim graph nodes");

        AZStd::vector<EMotionFX::BlendTreeConnection*> connectionList;
        AZStd::vector<EMotionFX::AnimGraphStateTransition*> transitionList;
        AZStd::vector<EMotionFX::AnimGraphNode*> nodeList;
        DeleteNodes(&commandGroup, animGraph, nodeNames, nodeList, connectionList, transitionList);

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void DeleteNodes(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, const AZStd::vector<EMotionFX::AnimGraphNode*>& nodes, bool autoChangeEntryStates)
    {
        AZStd::vector<EMotionFX::BlendTreeConnection*> connectionList;
        AZStd::vector<EMotionFX::AnimGraphStateTransition*> transitionList;
        AZStd::vector<EMotionFX::AnimGraphNode*> nodeList;
        AZStd::vector<AZStd::string> nodeNames;
        
        const size_t numNodes = nodes.size();
        nodeNames.reserve(numNodes);
        for (size_t i = 0; i < numNodes; ++i)
        {
            nodeNames.push_back(nodes[i]->GetName());
        }

        DeleteNodes(commandGroup, animGraph, nodeNames, nodeList, connectionList, transitionList, autoChangeEntryStates);
    }


    void CopyAnimGraphNodeCommand(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node, AZStd::vector<EMotionFX::AnimGraphNode*>& copiedNodes, AZStd::vector<AZStd::string>& nameReserveList, bool firstRootIteration, bool connectionPass, bool ignoreRootConnections, const AZStd::string& parentName = "")
    {
        if (!node)
        {
            return;
        }

        AZStd::string commandString;

        // Check if we are in the node construction pass.
        if (!connectionPass)
        {
            int32 posX = node->GetVisualPosX();
            int32 posY = node->GetVisualPosY();

            // Construct the parent name.
            AZStd::string finalParentName = parentName;
            if (firstRootIteration || parentName.empty())
            {
                finalParentName = "$(PARENTNAME)";
            }

            commandString = AZStd::string::format("AnimGraphCreateNode -type \"%s\" -parentName \"%s\" -xPos %i -yPos %i -name \"%s\" -collapsed %s -enabled %s -visualize %s",
                node->GetTypeString(),
                finalParentName.c_str(),
                posX,
                posY,
                node->GetName(),
                AZStd::to_string(node->GetIsCollapsed()).c_str(),
                AZStd::to_string(node->GetIsEnabled()).c_str(),
                AZStd::to_string(node->GetIsVisualizationEnabled()).c_str());

            // Don't put that into the format as the attribute string can become pretty big strings.
            commandString +=  " -attributesString \"";
            commandString += node->CreateAttributesString().c_str();
            commandString += "\"";

            commandGroup->AddCommandString(commandString);

            // Check if the given node is part of a node group.
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->FindNodeGroupForNode(node);
            if (nodeGroup)
            {
                commandString = AZStd::string::format("AnimGraphAdjustNodeGroup -name \"%s\" -nodeNames \"%s\" -nodeAction \"add\"",
                    nodeGroup->GetName(),
                    node->GetName());

                commandGroup->AddCommandString(commandString);
            }

            // Add the node and its new name to the name reservation list.
            copiedNodes.push_back(node);
            nameReserveList.push_back(node->GetName());
        }

        // Check if we are in the connection construction pass.
        if (connectionPass && !ignoreRootConnections)
        {
            if (node->GetType() == EMotionFX::AnimGraphStateMachine::TYPE_ID)
            {
                EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(node);

                EMotionFX::AnimGraphNode* entryState = stateMachine->GetEntryState();
                if (entryState)
                {
                    commandString = AZStd::string::format("AnimGraphSetEntryState -entryNodeName \"%s\"", entryState->GetName());
                    commandGroup->AddCommandString(commandString);
                }

                const uint32 numTransitions = stateMachine->GetNumTransitions();
                for (uint32 i = 0; i < numTransitions; ++i)
                {
                    CopyStateTransition(commandGroup, animGraph, stateMachine, stateMachine->GetName(), stateMachine->GetTransition(i), copiedNodes, nameReserveList);
                }
            }
            else
            {
                const uint32 numConnections = node->GetNumConnections();
                for (uint32 i = 0; i < numConnections; ++i)
                {
                    EMotionFX::BlendTreeConnection* connection = node->GetConnection(i);
                    CopyBlendTreeConnection(commandGroup, animGraph, node, connection, copiedNodes, nameReserveList);
                }
            }
        }

        // Recurse through the child nodes.
        const uint32 numChildNodes = node->GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            EMotionFX::AnimGraphNode* childNode = node->GetChildNode(i);
            CopyAnimGraphNodeCommand(commandGroup, animGraph, childNode, copiedNodes, nameReserveList, false, connectionPass, false, node->GetName());
        }
    }


    void ConstructCopyAnimGraphNodesCommandGroup(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, AZStd::vector<EMotionFX::AnimGraphNode*>& nodesToCopy, bool cutMode, bool ignoreTopLevelConnections, AZStd::vector<AZStd::string>& outResult)
    {
        outResult.clear();

        if (nodesToCopy.empty())
        {
            return;
        }

        // Remove all nodes that are child nodes of other selected nodes.
        for (size_t i = 0; i < nodesToCopy.size(); )
        {
            EMotionFX::AnimGraphNode* node = nodesToCopy[i];

            bool removeNode = false;
            for (size_t j = 0; j < nodesToCopy.size(); ++j)
            {
                if (node != nodesToCopy[j] && node->RecursiveIsParentNode(nodesToCopy[j]))
                {
                    removeNode = true;
                    break;
                }
            }

            if (removeNode)
            {
                nodesToCopy.erase(nodesToCopy.begin() + i);
            }
            else
            {
                i++;
            }
        }

        AZStd::vector<EMotionFX::AnimGraphNode*> recursiveCopiedNodes;
        AZStd::vector<AZStd::string> nameReserveList;

        // In case we are in cut and paste mode and delete the cut nodes.
        if (cutMode)
        {
            DeleteNodes(commandGroup, animGraph, nodesToCopy, false);
        }

        // 1. Pass: Create the new nodes.
        AZStd::string newNodeName;
        const size_t numNodes = nodesToCopy.size();
        for (size_t i = 0; i < numNodes; ++i)
        {
            EMotionFX::AnimGraphNode* node = nodesToCopy[i];

            // Only copy nodes that are also deletable, final nodes e.g. can't be copied.
            if (!node->GetIsDeletable())
            {
                continue;
            }

            CopyAnimGraphNodeCommand(commandGroup, animGraph, node, recursiveCopiedNodes, nameReserveList, true, false, ignoreTopLevelConnections);

            outResult.push_back(node->GetName());
        }

        // 2. Pass: Create connections and transitions.
        AZStd::vector<EMotionFX::AnimGraphStateMachine*> parentStateMachines;
        for (size_t i = 0; i < numNodes; ++i)
        {
            EMotionFX::AnimGraphNode* node = nodesToCopy[i];

            // Only copy nodes that are also deletable, final nodes e.g. can't be copied.
            if (!node->GetIsDeletable())
            {
                continue;
            }

            // Check if the parent node of the current one is NOT copied along, if so add the parent state machines for the 3. pass.
            EMotionFX::AnimGraphNode* parentNode = node->GetParentNode();
            if (parentNode &&
                parentNode->GetType() == EMotionFX::AnimGraphStateMachine::TYPE_ID &&
                AZStd::find(nodesToCopy.begin(), nodesToCopy.end(), parentNode) == nodesToCopy.end())
            {
                EMotionFX::AnimGraphStateMachine* parentSM = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

                // Only add if it is not in yet.
                if (AZStd::find(parentStateMachines.begin(), parentStateMachines.end(), parentSM) == parentStateMachines.end())
                {
                    parentStateMachines.push_back(parentSM);
                }
            }

            CopyAnimGraphNodeCommand(commandGroup, animGraph, node, recursiveCopiedNodes, nameReserveList, true, true, ignoreTopLevelConnections, node->GetName());
        }

        if (ignoreTopLevelConnections == false)
        {
            // 3. Pass: Copy transitions.
            const size_t numParentSMs = parentStateMachines.size();
            for (size_t i = 0; i < numParentSMs; ++i)
            {
                EMotionFX::AnimGraphStateMachine* parentStateMachine = parentStateMachines[i];

                const uint32 numTransitions = parentStateMachine->GetNumTransitions();
                for (uint32 t = 0; t < numTransitions; ++t)
                {
                    CopyStateTransition(commandGroup, animGraph, parentStateMachine, "$(PARENTNAME)", parentStateMachine->GetTransition(t), recursiveCopiedNodes, nameReserveList);
                }
            }
        }
    }


    void UpdateParameterNodeName(MCore::CommandLine* commandLine, AZStd::string& inOutCommand, const AZStd::vector<AZStd::string>& oldNodeNames, const MCore::Array<AZStd::string>& newNodeNames, const char* parameterName, AZStd::string* nodeName)
    {
        // Get the node name from the parameter.
        commandLine->GetValue(parameterName, "", nodeName);
        if (nodeName->empty())
        {
            return;
        }

        // Find the node name in the old node names array, if it is not in there we are safe to just use the current name.
        // This probably means the old node got deleted after we copied the nodes.
        auto iterator = AZStd::find(oldNodeNames.begin(), oldNodeNames.end(), nodeName->c_str());
        if (iterator == oldNodeNames.end())
        {
            return;
        }

        const uint32 nodeNameIndex = static_cast<uint32>(iterator - oldNodeNames.begin());

        AzFramework::StringFunc::Replace(inOutCommand,
            AZStd::string::format("-%s \"%s\"", parameterName, nodeName->c_str()).c_str(),
            AZStd::string::format("-%s \"%s\"", parameterName, newNodeNames[nodeNameIndex].c_str()).c_str(),
            true);
    }


    void AdjustCopyAnimGraphNodesCommandGroup(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& topLevelPastedNodes, int32 posX, int32 posY, const AZStd::string& parentName, bool cutMode)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 1: Iterate over the top level copy&paste nodes and calculate the mid point of them.
        ///////////////////////////////////////////////////////////////////////////////////////////////////////

        // Variables to sum up the node positions to later calculate the middle point of the copied nodes.
        int32   middlePosX  = 0;
        int32   middlePosY  = 0;
        uint32  numNodes    = 0;

        // Get the number of commands, iterate through them and sum up the node positions of the selected nodes.
        AZStd::string nodeName;
        AZStd::string command;
        AZStd::vector<AZStd::string> pastedNodeNames;
        const uint32 numCommands = commandGroup->GetNumCommands();
        for (uint32 i = 0; i < numCommands; ++i)
        {
            command = commandGroup->GetCommandString(i);

            // Only process create node commands.
            if (command.find("AnimGraphCreateNode") == AZStd::string::npos)
            {
                continue;
            }

            MCore::CommandLine commandLine(command.c_str());

            // Get the node name from the command line and add it to the pasted nodes array.
            commandLine.GetValue("name", "", &nodeName);
            pastedNodeNames.push_back(nodeName.c_str());

            // Check if they are part of the user selection, so top level copy&pasted nodes.
            if(AZStd::find(topLevelPastedNodes.begin(), topLevelPastedNodes.end(), nodeName.c_str()) == topLevelPastedNodes.end())
            {
                continue;
            }

            // Accumulate the positions of all pasted root nodes.
            const int32 nodePosX = commandLine.GetValueAsInt("xPos", 0);
            const int32 nodePosY = commandLine.GetValueAsInt("yPos", 0);

            middlePosX += nodePosX;
            middlePosY += nodePosY;
            numNodes++;
        }

        AZ_Assert(topLevelPastedNodes.size() == numNodes, "numNodes should be the same as the user selected top level nodes.");

        // Average the accumulated node positions and use if as the mid point for the paste operation.
        middlePosX = static_cast<int32>(MCore::Math::SignOfFloat((float)middlePosX) * (MCore::Math::Abs((float)middlePosX) / (float)numNodes));
        middlePosY = static_cast<int32>(MCore::Math::SignOfFloat((float)middlePosY) * (MCore::Math::Abs((float)middlePosY) / (float)numNodes));


        ///////////////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 2: Create new unique node names for all the pasted nodes
        ///////////////////////////////////////////////////////////////////////////////////////////////////////
        MCore::Array<AZStd::string> newNodeNames;
        const size_t numPastedNodeNames = pastedNodeNames.size();
        newNodeNames.Reserve(static_cast<uint32>(numPastedNodeNames));
        for (size_t i = 0; i < numPastedNodeNames; ++i)
        {
            // Find the node with the given name in the anim graph.
            EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNode(pastedNodeNames[i].c_str());

            // Use the old name in case the copied node got deleted before pasting it, create a new unique node name in case it still exists.
            if (!node)
            {
                newNodeNames.Add(pastedNodeNames[i].c_str());
                //LogError("Using old name for node called '%s'.", pastedNodeNames[i].AsChar());
            }
            else
            {
                if (cutMode)
                {
                    // If we are cut&pasting the nodes that we cut don't need to be renamed.
                    newNodeNames.Add(pastedNodeNames[i].c_str());
                }
                else
                {
                    AZStd::string newNodeName = GenerateUniqueNodeName(animGraph, node, newNodeNames);
                    newNodeNames.Add(newNodeName.c_str());
                    //LogError("Node named '%s' got renamed to '%s'.", pastedNodeNames[i].AsChar(), newNodeName.AsChar());
                }
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 3: Adjust other attributes to new node names and adjust their positions etc.
        ///////////////////////////////////////////////////////////////////////////////////////////////////////

        // Iterate through the nodes once again and adjust their positions.
        AZStd::string tempString;
        for (uint32 i = 0; i < numCommands; ++i)
        {
            command = commandGroup->GetCommandString(i);
            MCore::CommandLine commandLine(command.c_str());

            // Only process create node commands.
            if (command.find("AnimGraphCreateNode") != AZStd::string::npos)
            {
                // Get the node name parameter and check if the given node is one we selected for copy and paste.
                commandLine.GetValue("name", "", &nodeName);

                UpdateParameterNodeName(&commandLine, command, pastedNodeNames, newNodeNames, "name", &tempString);

                if(AZStd::find(topLevelPastedNodes.begin(), topLevelPastedNodes.end(), nodeName.c_str()) != topLevelPastedNodes.end())
                {
                    const int32 nodePosX = commandLine.GetValueAsInt("xPos", 0);
                    const int32 nodePosY = commandLine.GetValueAsInt("yPos", 0);
                    //LOG("Node: %s, PosX: %d, PosY: %d", nodeName.AsChar(), nodePosX, nodePosY);

                    // Calculate the new position after pasting.
                    int32 newPosX = nodePosX + (posX - middlePosX);
                    int32 newPosY = nodePosY + (posY - middlePosY);

                    // In case the node comes from another hierarchy level, place it exactly at paste mouse position.
                    /*if (command.Find("$(PARENTNAME)") != MCORE_INVALIDINDEX32)
                    {
                        newPosX = posX;
                        newPosY = posY;
                    }*/

                    // Adjust x position.
                    AzFramework::StringFunc::Replace(command,
                        AZStd::string::format("-xPos %d", nodePosX).c_str(),
                        AZStd::string::format("-xPos %d", newPosX).c_str(),
                        true);

                    // Adjust y position.
                    AzFramework::StringFunc::Replace(command,
                        AZStd::string::format("-yPos %d", nodePosY).c_str(),
                        AZStd::string::format("-yPos %d", newPosY).c_str(),
                        true);

                    // Replace the parent name in case that node has the parent macro set.
                    AzFramework::StringFunc::Replace(command, "$(PARENTNAME)", parentName.c_str(), true);
                }
                else
                {
                    UpdateParameterNodeName(&commandLine, command, pastedNodeNames, newNodeNames, "parentName", &tempString);
                }
            }
            else if (command.find("AnimGraphCreateConnection") != AZStd::string::npos)
            {
                UpdateParameterNodeName(&commandLine, command, pastedNodeNames, newNodeNames, "sourceNode", &tempString);
                UpdateParameterNodeName(&commandLine, command, pastedNodeNames, newNodeNames, "targetNode", &tempString);
            }
            else if (command.find("AnimGraphAddCondition") != AZStd::string::npos)
            {
                // Replace the parent name in case that node has the parent macro set.
                AzFramework::StringFunc::Replace(command, "$(PARENTNAME)", parentName.c_str(), true);

                UpdateParameterNodeName(&commandLine, command, pastedNodeNames, newNodeNames, "stateMachineName", &tempString);

                if (commandLine.CheckIfHasParameter("attributesString"))
                {
                    commandLine.GetValue("attributesString", "", &tempString);
                    MCore::CommandLine attributesCommandLine(tempString.c_str());

                    // State condition: Watch state.
                    if (attributesCommandLine.CheckIfHasParameter("stateID"))
                    {
                        UpdateParameterNodeName(&attributesCommandLine, command, pastedNodeNames, newNodeNames, "stateID", &tempString);
                    }

                    // Motion condition: Watch state.
                    if (attributesCommandLine.CheckIfHasParameter("motionID"))
                    {
                        UpdateParameterNodeName(&attributesCommandLine, command, pastedNodeNames, newNodeNames, "motionID", &tempString);
                    }
                }
            }
            else if (command.find("AnimGraphAdjustNodeGroup") != AZStd::string::npos)
            {
                UpdateParameterNodeName(&commandLine, command, pastedNodeNames, newNodeNames, "nodeNames", &tempString);
            }
            else if (command.find("AnimGraphSetEntryState") != AZStd::string::npos)
            {
                UpdateParameterNodeName(&commandLine, command, pastedNodeNames, newNodeNames, "entryNodeName", &tempString);
            }

            // Adjust the command string.
            commandGroup->SetCommandString(i, command.c_str());
            //LogError(command.AsChar());
        }
    }
} // namesapce EMotionFX