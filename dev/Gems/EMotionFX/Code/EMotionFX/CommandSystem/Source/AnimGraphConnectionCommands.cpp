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

#include "AnimGraphConnectionCommands.h"
#include "AnimGraphNodeCommands.h"
#include "CommandManager.h"
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphMotionCondition.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/ActorManager.h>
#include <MCore/Source/ReflectionSerializer.h>


namespace CommandSystem
{
    EMotionFX::AnimGraph* CommandsGetAnimGraph(const MCore::CommandLine& parameters, MCore::Command* command, AZStd::string& outResult)
    {
        if (parameters.CheckIfHasParameter("animGraphID"))
        {
            const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", command);
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
            if (animGraph == nullptr)
            {
                outResult = AZStd::string::format("Anim graph with id '%i' cannot be found.", animGraphID);
                return nullptr;
            }
            return animGraph;
        }
        else
        {
            EMotionFX::AnimGraph* animGraph = GetCommandManager()->GetCurrentSelection().GetFirstAnimGraph();
            if (animGraph == nullptr)
            {
                outResult = AZStd::string::format("Anim graph id is not specified and no one anim graph is selected.");
                return nullptr;
            }
            return animGraph;
        }
    }

    //-------------------------------------------------------------------------------------
    // AnimGraphCreateConnection - create a connection between two nodes
    //-------------------------------------------------------------------------------------

    // constructor
    CommandAnimGraphCreateConnection::CommandAnimGraphCreateConnection(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphCreateConnection", orgCommand)
    {
        mConnectionId   = MCORE_INVALIDINDEX32;
        mTransitionType = AZ::TypeId::CreateNull();
    }

    // destructor
    CommandAnimGraphCreateConnection::~CommandAnimGraphCreateConnection()
    {
    }


    // execute
    bool CommandAnimGraphCreateConnection::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        mAnimGraphID = animGraph->GetID();

        // get the transition type
        AZ::Outcome<AZStd::string> transitionTypeString = parameters.GetValueIfExists("transitionType", this);
        if (transitionTypeString.IsSuccess())
        {
            mTransitionType = AZ::TypeId::CreateString(transitionTypeString.GetValue().c_str());
        }
        
        // get the node names
        AZStd::string sourceNodeName;
        AZStd::string targetNodeName;
        parameters.GetValue("sourceNode", "", sourceNodeName);
        parameters.GetValue("targetNode", "", targetNodeName);

        // find the source node in the anim graph
        EMotionFX::AnimGraphNode* sourceNode = animGraph->RecursiveFindNodeByName(sourceNodeName.c_str());

        // find the target node in the anim graph
        EMotionFX::AnimGraphNode* targetNode = animGraph->RecursiveFindNodeByName(targetNodeName.c_str());
        if (!targetNode)
        {
            outResult = AZStd::string::format("Cannot find target node with name '%s' in anim graph '%s'", targetNodeName.c_str(), animGraph->GetFileName());
            return false;
        }

        if (targetNode == sourceNode)
        {
            outResult = AZStd::string::format("Unable to create connection: source node and target node are the same. Node name = %s", targetNode->GetName());
            return false;
        }

        // get the ports
        mSourcePort = parameters.GetValueAsInt("sourcePort", 0);
        mTargetPort = parameters.GetValueAsInt("targetPort", 0);
        parameters.GetValue("sourcePortName", this, mSourcePortName);
        parameters.GetValue("targetPortName", this, mTargetPortName);

        // in case the source port got specified by name, overwrite the source port number
        if (!mSourcePortName.empty())
        {
            mSourcePort = sourceNode->FindOutputPortIndex(mSourcePortName.c_str());

            // in case we want to add this connection to a parameter node while the parameter name doesn't exist, still return true so that copy paste doesn't fail
            if (azrtti_typeid(sourceNode) == azrtti_typeid<EMotionFX::BlendTreeParameterNode>() && mSourcePort == -1)
            {
                mConnectionId = MCORE_INVALIDINDEX32;
                return true;
            }
        }

        // in case the target port got specified by name, overwrite the target port number
        if (!mTargetPortName.empty())
        {
            mTargetPort = targetNode->FindInputPortIndex(mTargetPortName.c_str());
        }

        // get the parent of the source node
        if (targetNode->GetParentNode() == nullptr)
        {
            outResult = AZStd::string::format("Cannot create connections between root state machines.");
            return false;
        }

        // if the parent is state machine, we don't need to check the port ranges
        if (azrtti_typeid(targetNode->GetParentNode()) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            if (sourceNode == nullptr)
            {
                outResult = AZStd::string::format("Cannot create blend tree connection in anim graph '%s' as the source node is not valid. ", animGraph->GetFileName());
                return false;
            }

            // verify port ranges
            if (mSourcePort >= (int32)sourceNode->GetNumOutputs() || mSourcePort < 0)
            {
                outResult = AZStd::string::format("The output port number is not valid for the given node. Node '%s' only has %d output ports.", sourceNode->GetName(), sourceNode->GetNumOutputs());
                return false;
            }

            if (mTargetPort >= (int32)targetNode->GetNumInputs() || mTargetPort < 0)
            {
                outResult = AZStd::string::format("The input port number is not valid for the given node. Node '%s' only has %d input ports.", targetNode->GetName(), targetNode->GetNumInputs());
                return false;
            }

            // check if connection already exists
            if (targetNode->GetHasConnection(sourceNode, static_cast<uint16>(mSourcePort), static_cast<uint16>(mTargetPort)))
            {
                outResult = AZStd::string::format("The connection you are trying to create already exists!");
                return false;
            }

            // create the connection and auto assign an id first of all
            EMotionFX::BlendTreeConnection* connection = targetNode->AddConnection(sourceNode, static_cast<uint16>(mSourcePort), static_cast<uint16>(mTargetPort));

            // get the connection ID
            if (parameters.CheckIfHasParameter("id"))
            {
                // manually assign the ID
                mConnectionId = parameters.GetValueAsInt("id", this);
                connection->SetId(mConnectionId);
            }

            // get the connection id
            mConnectionId = connection->GetId();
        }
        else // create a state transition
        {
            // get the state machine
            EMotionFX::AnimGraphStateMachine* machine = (EMotionFX::AnimGraphStateMachine*)targetNode->GetParentNode();

            // try to create the anim graph node
            EMotionFX::AnimGraphObject* object = EMotionFX::AnimGraphObjectFactory::Create(mTransitionType, animGraph);
            if (!object)
            {
                outResult = AZStd::string::format("Cannot create transition of type %d", mTransitionType.ToString<AZStd::string>().c_str());
                return false;
            }

            // check if this is really a transition
            if (!azrtti_istypeof<EMotionFX::AnimGraphStateTransition>(object))
            {
                outResult = AZStd::string::format("Cannot create state transition of type %d, because this object type is not inherited from AnimGraphStateTransition.", mTransitionType.ToString<AZStd::string>().c_str());
                return false;
            }

            // typecast the anim graph node into a state transition node
            EMotionFX::AnimGraphStateTransition* transition = static_cast<EMotionFX::AnimGraphStateTransition*>(object);

            // Deserialize first, manually specified parameters have higher priority and can overwrite the contents.
            if (parameters.CheckIfHasParameter("contents"))
            {
                AZStd::string contents;
                parameters.GetValue("contents", this, &contents);
                MCore::ReflectionSerializer::Deserialize(transition, contents);

                transition->RemoveAllConditions(true);
            }

            // check if we are dealing with a wildcard transition
            bool isWildcardTransition = false;
            if (sourceNode == nullptr)
            {
                isWildcardTransition = true;
            }

            // setup the transition properties
            transition->SetSourceNode(sourceNode);
            transition->SetTargetNode(targetNode);

            // get the offsets
            mStartOffsetX = parameters.GetValueAsInt("startOffsetX", 0);
            mStartOffsetY = parameters.GetValueAsInt("startOffsetY", 0);
            mEndOffsetX = parameters.GetValueAsInt("endOffsetX", 0);
            mEndOffsetY = parameters.GetValueAsInt("endOffsetY", 0);
            if (parameters.CheckIfHasValue("startOffsetX") || parameters.CheckIfHasValue("startOffsetY") || parameters.CheckIfHasValue("endOffsetX") || parameters.CheckIfHasValue("endOffsetY"))
            {
                transition->SetVisualOffsets(mStartOffsetX, mStartOffsetY, mEndOffsetX, mEndOffsetY);
            }
            transition->SetIsWildcardTransition(isWildcardTransition);

            // get the transition ID
            uint32 transitionID = MCORE_INVALIDINDEX32;
            if (parameters.CheckIfHasParameter("id"))
            {
                transitionID = parameters.GetValueAsInt("id", this);
            }

            // set the actor instance id in case we have specified it as parameter
            if (transitionID != MCORE_INVALIDINDEX32)
            {
                transition->SetID(transitionID);
            }

            // in case of redoing the command set the previously used id
            if (mConnectionId != MCORE_INVALIDINDEX32)
            {
                transition->SetID(mConnectionId);
            }

            // get the transition id and save it for undo
            mConnectionId = transition->GetID();

            // add the transition to the state machine
            machine->AddTransition(transition);
        }

        mTargetNodeId.SetInvalid();
        mSourceNodeId.SetInvalid();
        if (targetNode)
        {
            mTargetNodeId = targetNode->GetId();
        }
        if (sourceNode)
        {
            mSourceNodeId = sourceNode->GetId();
        }

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        // set the command result to the connection id
        outResult = AZStd::string::format("%i", mConnectionId);

        animGraph->Reinit();
        animGraph->UpdateUniqueData();

        return true;
    }


    // undo the command
    bool CommandAnimGraphCreateConnection::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", mAnimGraphID);
            return false;
        }

        // in case of a wildcard transition the source node is the invalid index, so that's all fine
        EMotionFX::AnimGraphNode* sourceNode = animGraph->RecursiveFindNodeById(mSourceNodeId);
        EMotionFX::AnimGraphNode* targetNode = animGraph->RecursiveFindNodeById(mTargetNodeId);

        if (sourceNode == nullptr || targetNode == nullptr)
        {
            outResult = AZStd::string::format("Source or target node does not exist!");
            return false;
        }

        // get the source node name, special path here as wildcard transitions have a nullptr source node
        AZStd::string sourceNodeName;
        if (sourceNode)
        {
            sourceNodeName = sourceNode->GetNameString();
        }

        // delete the connection
        const AZStd::string commandString = AZStd::string::format("AnimGraphRemoveConnection -animGraphID %i -targetNode \"%s\" -targetPort %d -sourceNode \"%s\" -sourcePort %d -id %d",
            animGraph->GetID(),
            targetNode->GetName(),
            mTargetPort,
            sourceNodeName.c_str(),
            mSourcePort,
            mConnectionId);

        // execute the command without putting it in the history
        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            if (outResult.size() > 0)
            {
                MCore::LogError(outResult.c_str());
            }

            return false;
        }

        // reset the data used for undo and redo
        mSourceNodeId.SetInvalid();
        mTargetNodeId.SetInvalid();

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);

        animGraph->Reinit();
        animGraph->UpdateUniqueData();

        return true;
    }


    // init the syntax of the command
    void CommandAnimGraphCreateConnection::InitSyntax()
    {
        GetSyntax().ReserveParameters(12);
        GetSyntax().AddRequiredParameter("sourceNode", "The name of the source node, where the connection starts (output port).", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("targetNode", "The name of the target node to connect to (input port).", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("animGraphID", "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("sourcePort", "The source port number where the connection starts inside the source node.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("targetPort", "The target port number where the connection connects into, in the target node.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("sourcePortName", "The source port name where the connection starts inside the source node.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("targetPortName", "The target port name where the connection connects into, in the target node.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("startOffsetX", "The start offset x position, which is the offset to from the upper left corner of the node where the connection starts from.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("startOffsetY", "The start offset y position, which is the offset to from the upper left corner of the node where the connection starts from.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("endOffsetX", "The end offset x position, which is the offset to from the upper left corner of the node where the connection ends.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("endOffsetY", "The end offset y position, which is the offset to from the upper left corner of the node where the connection ends.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("id", "The id of the connection.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("transitionType", "The transition type ID. This is the type ID (UUID) of the AnimGraphStateTransition inherited node types.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("contents", "The serialized contents of the parameter (in reflected XML).", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    // get the description
    const char* CommandAnimGraphCreateConnection::GetDescription() const
    {
        return "This command creates a connection between two anim graph nodes.";
    }




    //-------------------------------------------------------------------------------------
    // AnimGraphRemoveConnection - create a connection between two nodes
    //-------------------------------------------------------------------------------------

    // constructor
    CommandAnimGraphRemoveConnection::CommandAnimGraphRemoveConnection(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphRemoveConnection", orgCommand)
    {
        mSourcePort     = MCORE_INVALIDINDEX32;
        mTargetPort     = MCORE_INVALIDINDEX32;
        mTransitionType = AZ::TypeId::CreateNull();
        mStartOffsetX   = 0;
        mStartOffsetY   = 0;
        mEndOffsetX     = 0;
        mEndOffsetY     = 0;
        mConnectionID   = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandAnimGraphRemoveConnection::~CommandAnimGraphRemoveConnection()
    {
    }


    // execute
    bool CommandAnimGraphRemoveConnection::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        mAnimGraphID = animGraph->GetID();

        // get the node names
        AZStd::string sourceNodeName;
        AZStd::string targetNodeName;
        parameters.GetValue("sourceNode", "", sourceNodeName);
        parameters.GetValue("targetNode", "", targetNodeName);

        // find the source node in the anim graph
        EMotionFX::AnimGraphNode* sourceNode = animGraph->RecursiveFindNodeByName(sourceNodeName.c_str());
        //if (sourceNode == nullptr)
        //{
        //  outResult = AZStd::string::format("Cannot find source node with name '%s' in anim graph '%s'", sourceName.AsChar(), animGraph->GetName());
        //  return false;
        //}

        // find the target node in the anim graph
        EMotionFX::AnimGraphNode* targetNode = animGraph->RecursiveFindNodeByName(targetNodeName.c_str());
        if (targetNode == nullptr)
        {
            outResult = AZStd::string::format("Cannot find target node with name '%s' in anim graph '%s'", targetNodeName.c_str(), animGraph->GetFileName());
            return false;
        }

        // get the ids from the source and destination nodes
        mSourceNodeId.SetInvalid();
        mSourceNodeName.clear();
        if (sourceNode)
        {
            mSourceNodeId = sourceNode->GetId();
            mSourceNodeName = sourceNode->GetName();
        }
        mTargetNodeId = targetNode->GetId();
        mTargetNodeName = targetNode->GetName();

        // get the ports
        mSourcePort = parameters.GetValueAsInt("sourcePort", 0);
        mTargetPort = parameters.GetValueAsInt("targetPort", 0);

        // get the parent of the source node
        if (targetNode->GetParentNode() == nullptr)
        {
            outResult = AZStd::string::format("Cannot remove connections between root state machines.");
            return false;
        }

        // if the parent is state machine, we don't need to check the port ranges
        if (azrtti_typeid(targetNode->GetParentNode()) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            if (sourceNode == nullptr)
            {
                outResult = AZStd::string::format("Cannot remove blend tree connection in anim graph '%s' as the source node is not valid.", animGraph->GetFileName());
                return false;
            }

            // verify port ranges
            if (mSourcePort >= (int32)sourceNode->GetNumOutputs() || mSourcePort < 0)
            {
                outResult = AZStd::string::format("The output port number is not valid for the given node. Node '%s' only has %d output ports.", sourceNode->GetName(), sourceNode->GetNumOutputs());
                return false;
            }

            if (mTargetPort >= (int32)targetNode->GetNumInputs() || mTargetPort < 0)
            {
                outResult = AZStd::string::format("The input port number is not valid for the given node. Node '%s' only has %d input ports.", targetNode->GetName(), targetNode->GetNumInputs());
                return false;
            }

            // check if connection already exists
            if (!targetNode->GetHasConnection(sourceNode, static_cast<uint16>(mSourcePort), static_cast<uint16>(mTargetPort)))
            {
                outResult = AZStd::string::format("The connection you are trying to remove doesn't exist!");
                return false;
            }

            // get the connection ID and store it
            mConnectionID = targetNode->FindConnection(sourceNode, static_cast<uint16>(mSourcePort), static_cast<uint16>(mTargetPort))->GetId();

            // create the connection
            targetNode->RemoveConnection(sourceNode, static_cast<uint16>(mSourcePort), static_cast<uint16>(mTargetPort));
        }
        else // remove a state transition
        {
            // get the transition id
            const uint32 transitionID = parameters.GetValueAsInt("id", this);
            if (transitionID == MCORE_INVALIDINDEX32)
            {
                outResult = AZStd::string::format("You cannot remove a state transition with an invalid id. (Did you specify the id parameter?)");
                return false;
            }

            // get the state machine
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(targetNode->GetParentNode());

            // find the transition index based on the id
            const uint32 transitionIndex = stateMachine->FindTransitionIndexByID(transitionID);
            if (transitionIndex == MCORE_INVALIDINDEX32)
            {
                outResult = AZStd::string::format("The state transition you are trying to remove cannot be found.");
                return false;
            }

            // save the transition information for undo
            EMotionFX::AnimGraphStateTransition* transition = stateMachine->GetTransition(transitionIndex);
            mStartOffsetX       = transition->GetVisualStartOffsetX();
            mStartOffsetY       = transition->GetVisualStartOffsetY();
            mEndOffsetX         = transition->GetVisualEndOffsetX();
            mEndOffsetY         = transition->GetVisualEndOffsetY();
            mTransitionType     = azrtti_typeid(transition);
            mConnectionID       = transition->GetID();
            mOldContents        = MCore::ReflectionSerializer::Serialize(transition).GetValue();

            // remove all unique datas for the transition itself
            animGraph->RemoveAllObjectData(transition, true);

            // remove the transition
            stateMachine->RemoveTransition(transitionIndex);

            // get the number of actor instances and iterate through them
            const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
            for (uint32 i = 0; i < numActorInstances; ++i)
            {
                // get the anim graph instance of the current actor instance and reset its unique data
                EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetActorManager().GetActorInstance(i)->GetAnimGraphInstance();
                if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
                {
                    stateMachine->OnUpdateUniqueData(animGraphInstance);
                }
            }
        }

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        animGraph->Reinit();
        animGraph->UpdateUniqueData();

        return true;
    }


    // undo the command
    bool CommandAnimGraphRemoveConnection::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", mAnimGraphID);
            return false;
        }

        if (!mTargetNodeId.IsValid())
        {
            return false;
        }

        AZStd::string commandString = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d -startOffsetX %d -startOffsetY %d -endOffsetX %d -endOffsetY %d -id %d -transitionType \"%s\"",
                animGraph->GetID(),
                mSourceNodeName.c_str(),
                mTargetNodeName.c_str(),
                mSourcePort,
                mTargetPort,
                mStartOffsetX, mStartOffsetY,
                mEndOffsetX, mEndOffsetY,
                mConnectionID,
                mTransitionType.ToString<AZStd::string>().c_str());

        // add the old attributes
        if (mOldContents.empty() == false)
        {
            commandString += AZStd::string::format(" -contents {%s}", mOldContents.c_str());
        }

        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            if (outResult.size() > 0)
            {
                MCore::LogError(outResult.c_str());
            }

            return false;
        }

        mTargetNodeId.SetInvalid();
        mSourceNodeId.SetInvalid();
        mConnectionID   = MCORE_INVALIDINDEX32;
        mSourcePort     = MCORE_INVALIDINDEX32;
        mTargetPort     = MCORE_INVALIDINDEX32;
        mStartOffsetX   = 0;
        mStartOffsetY   = 0;
        mEndOffsetX     = 0;
        mEndOffsetY     = 0;

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);

        animGraph->Reinit();
        animGraph->UpdateUniqueData();

        return true;
    }


    // init the syntax of the command
    void CommandAnimGraphRemoveConnection::InitSyntax()
    {
        // required
        GetSyntax().ReserveParameters(6);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("sourceNode", "The name of the source node, where the connection starts (output port).", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("targetNode", "The name of the target node where it connects to (input port).", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("sourcePort", "The source port number where the connection starts inside the source node.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("targetPort", "The target port number where the connection connects into, in the target node.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("id", "The id of the connection.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
    }


    // get the description
    const char* CommandAnimGraphRemoveConnection::GetDescription() const
    {
        return "This command removes a connection between two anim graph nodes.";
    }



    //-------------------------------------------------------------------------------------
    // AnimGraphRemoveConnection - create a connection between two nodes
    //-------------------------------------------------------------------------------------

    // constructor
    CommandAnimGraphAdjustConnection::CommandAnimGraphAdjustConnection(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphAdjustConnection", orgCommand)
    {
        mStartOffsetX       = 0;
        mStartOffsetY       = 0;
        mEndOffsetX         = 0;
        mEndOffsetY         = 0;
        mOldDirtyFlag       = false;
    }


    // destructor
    CommandAnimGraphAdjustConnection::~CommandAnimGraphAdjustConnection()
    {
    }


    void CommandAnimGraphAdjustConnection::RewindTransitionIfActive(EMotionFX::AnimGraphStateTransition* transition)
    {
        const EMotionFX::AnimGraph* animGraph = transition->GetAnimGraph();
        EMotionFX::AnimGraphStateMachine* stateMachine = transition->GetStateMachine();

        const size_t numAnimGraphInstances = animGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numAnimGraphInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = animGraph->GetAnimGraphInstance(i);
            EMotionFX::AnimGraphStateTransition* activeTransition = stateMachine->GetActiveTransition(animGraphInstance);

            if (transition == activeTransition)
            {
                stateMachine->Rewind(animGraphInstance);
            }
        }
    }


    // execute
    bool CommandAnimGraphAdjustConnection::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        mAnimGraphID = animGraph->GetID();

        m_stateMachineName = parameters.GetValue("stateMachine", this);
        EMotionFX::AnimGraphNode* stateMachineNode = animGraph->RecursiveFindNodeByName(m_stateMachineName.c_str());
        if (!stateMachineNode)
        {
            outResult = AZStd::string::format("Cannot find state machine node with name '%s' in anim graph '%s'", m_stateMachineName.c_str(), animGraph->GetFileName());
            return false;
        }

        EMotionFX::AnimGraphStateMachine* stateMachine = azdynamic_cast<EMotionFX::AnimGraphStateMachine*>(stateMachineNode);
        if (!stateMachine)
        {
            outResult = AZStd::string::format("Anim graph node named '%s' is not a state machine.", m_stateMachineName.c_str());
            return false;
        }

        // get the transition id and find the transition index based on the id
        mTransitionID = parameters.GetValueAsInt("transitionID", this);
        const uint32 transitionIndex = stateMachine->FindTransitionIndexByID(mTransitionID);
        if (transitionIndex == MCORE_INVALIDINDEX32)
        {
            outResult = AZStd::string::format("The state transition you are trying to adjust cannot be found.");
            return false;
        }

        // save the transition information for undo
        EMotionFX::AnimGraphStateTransition* transition = stateMachine->GetTransition(transitionIndex);
        mStartOffsetX       = transition->GetVisualStartOffsetX();
        mStartOffsetY       = transition->GetVisualStartOffsetY();
        mEndOffsetX         = transition->GetVisualEndOffsetX();
        mEndOffsetY         = transition->GetVisualEndOffsetY();

        EMotionFX::AnimGraphNode* oldSourceNode = transition->GetSourceNode();
        EMotionFX::AnimGraphNode* oldTargetNode = transition->GetTargetNode();
        if (oldSourceNode)
        {
            mOldSourceNodeName = oldSourceNode->GetName();
        }
        if (oldTargetNode)
        {
            mOldTargetNodeName = oldTargetNode->GetName();
        }

        // set the new source node
        if (parameters.CheckIfHasParameter("sourceNode"))
        {
            const AZStd::string newSourceName = parameters.GetValue("sourceNode", this);
            EMotionFX::AnimGraphNode* newSourceNode = animGraph->RecursiveFindNodeByName(newSourceName.c_str());
            if (!newSourceNode)
            {
                outResult = AZStd::string::format("Cannot find new source node with name '%s' in anim graph '%s'", newSourceName.c_str(), animGraph->GetFileName());
                return false;
            }

            RewindTransitionIfActive(transition);
            transition->SetSourceNode(newSourceNode);
        }

        // set the new target node
        if (parameters.CheckIfHasParameter("targetNode"))
        {
            const AZStd::string newTargetName = parameters.GetValue("targetNode", this);
            EMotionFX::AnimGraphNode* newTargetNode = animGraph->RecursiveFindNodeByName(newTargetName.c_str());
            if (!newTargetNode)
            {
                outResult = AZStd::string::format("Cannot find new target node with name '%s' in anim graph '%s'", newTargetName.c_str(), animGraph->GetFileName());
                return false;
            }

            RewindTransitionIfActive(transition);
            transition->SetTargetNode(newTargetNode);
        }

        // set the new visual offsets
        if (parameters.CheckIfHasParameter("startOffsetX") && parameters.CheckIfHasParameter("startOffsetY") &&
            parameters.CheckIfHasParameter("endOffsetX") && parameters.CheckIfHasParameter("endOffsetY"))
        {
            const int32 newStartOffsetX = parameters.GetValueAsInt("startOffsetX", this);
            const int32 newStartOffsetY = parameters.GetValueAsInt("startOffsetY", this);
            const int32 newEndOffsetX = parameters.GetValueAsInt("endOffsetX", this);
            const int32 newEndOffsetY = parameters.GetValueAsInt("endOffsetY", this);

            transition->SetVisualOffsets(newStartOffsetX, newStartOffsetY, newEndOffsetX, newEndOffsetY);
        }

        // set the disabled flag
        if (parameters.CheckIfHasParameter("isDisabled"))
        {
            // get the new value from the command parameter
            bool isDisabled = parameters.GetValueAsBool("isDisabled", this);

            // get the current value from the transition and store it for undo
            mOldDisabledFlag = transition->GetIsDisabled();

            // enable or disable the transition
            transition->SetIsDisabled(isDisabled);
        }

        if (parameters.CheckIfHasParameter("attributesString"))
        {
            const AZStd::string attributesString = parameters.GetValue("attributesString", this);
            MCore::ReflectionSerializer::Deserialize(transition, MCore::CommandLine(attributesString));
        }

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        animGraph->Reinit();
        animGraph->UpdateUniqueData();

        return true;
    }


    // undo the command
    bool CommandAnimGraphAdjustConnection::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", mAnimGraphID);
            return false;
        }

        const AZStd::string stateMachineName = parameters.GetValue("stateMachine", this);
        const uint32 transitionID = parameters.GetValueAsInt("transitionID", this);

        AZStd::string commandString = AZStd::string::format("AnimGraphAdjustConnection -animGraphID %i -stateMachine \"%s\" -transitionID %i -startOffsetX %i -startOffsetY %i -endOffsetX %i -endOffsetY %i ", animGraph->GetID(), stateMachineName.c_str(), transitionID, mStartOffsetX, mStartOffsetY, mEndOffsetX, mEndOffsetY);
        if (!mOldSourceNodeName.empty())
        {
            commandString += AZStd::string::format("-sourceNode \"%s\" ", mOldSourceNodeName.c_str());
        }
        if (!mOldTargetNodeName.empty())
        {
            commandString += AZStd::string::format("-targetNode \"%s\" ", mOldTargetNodeName.c_str());
        }
        if (parameters.CheckIfHasParameter("isDisabled"))
        {
            commandString += AZStd::string::format("-isDisabled %s ", AZStd::to_string(mOldDisabledFlag).c_str());
        }

        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            if (!outResult.empty())
            {
                MCore::LogError(outResult.c_str());
            }

            return false;
        }

        mOldTargetNodeName.clear();
        mOldSourceNodeName.clear();
        mStartOffsetX   = 0;
        mStartOffsetY   = 0;
        mEndOffsetX     = 0;
        mEndOffsetY     = 0;

        animGraph->SetDirtyFlag(mOldDirtyFlag);

        animGraph->Reinit();
        animGraph->UpdateUniqueData();
        return true;
    }


    // init the syntax of the command
    void CommandAnimGraphAdjustConnection::InitSyntax()
    {
        GetSyntax().ReserveParameters(10);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("stateMachine", "The name of the state machine where the transition is in.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("transitionID", "The id of the transition to adjust.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("sourceNode", "The new source node of the transition.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("targetNode", "The new target node of the transition.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("startOffsetX", ".", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("startOffsetY", ".", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("endOffsetX", ".", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("endOffsetY", ".", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("isDisabled", "False in case the transition shall be active and working, true in case it should be disabled and act like it does not exist.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("attributesString", "The connection attributes as string.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    // get the description
    const char* CommandAnimGraphAdjustConnection::GetDescription() const
    {
        return "";
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    // Delete a given connection.
    void DeleteConnection(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* node, EMotionFX::BlendTreeConnection* connection, AZStd::vector<EMotionFX::BlendTreeConnection*>& connectionList)
    {
        // Skip directly if the connection is already in the list.
        if (AZStd::find(connectionList.begin(), connectionList.end(), connection) != connectionList.end())
        {
            return;
        }

        // In case the source node is specified, get the node name from the connection.
        AZStd::string sourceNodeName;
        if (connection->GetSourceNode())
        {
            sourceNodeName = connection->GetSourceNode()->GetName();
        }

        const AZStd::string commandString = AZStd::string::format("AnimGraphRemoveConnection -animGraphID %i -targetNode \"%s\" -targetPort %d -sourceNode \"%s\" -sourcePort %d",
                node->GetAnimGraph()->GetID(),
                node->GetName(),
                connection->GetTargetPort(),
                sourceNodeName.c_str(),
                connection->GetSourcePort());

        connectionList.push_back(connection);
        commandGroup->AddCommandString(commandString);
    }


    // Delete all incoming and outgoing connections for the given node.
    void DeleteNodeConnections(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* node, EMotionFX::AnimGraphNode* parentNode, AZStd::vector<EMotionFX::BlendTreeConnection*>& connectionList, bool recursive)
    {
        // Delete the connections that start from the given node.
        if (parentNode)
        {
            const uint32 numChildNodes = parentNode->GetNumChildNodes();
            for (uint32 i = 0; i < numChildNodes; ++i)
            {
                EMotionFX::AnimGraphNode* childNode = parentNode->GetChildNode(i);
                if (childNode == node)
                {
                    continue;
                }

                const uint32 numChildConnections = childNode->GetNumConnections();
                for (uint32 j = 0; j < numChildConnections; ++j)
                {
                    EMotionFX::BlendTreeConnection* childConnection = childNode->GetConnection(j);

                    // If the connection starts at the given node, delete it.
                    if (childConnection->GetSourceNode() == node)
                    {
                        DeleteConnection(commandGroup, childNode, childConnection, connectionList);
                    }
                }
            }
        }

        // Delete the connections that end in the given node.
        const uint32 numConnections = node->GetNumConnections();
        for (uint32 i = 0; i < numConnections; ++i)
        {
            EMotionFX::BlendTreeConnection* connection = node->GetConnection(i);
            DeleteConnection(commandGroup, node, connection, connectionList);
        }

        // Recursively delete all connections.
        if (recursive)
        {
            const uint32 numChildNodes = node->GetNumChildNodes();
            for (uint32 i = 0; i < numChildNodes; ++i)
            {
                EMotionFX::AnimGraphNode* childNode = node->GetChildNode(i);
                DeleteNodeConnections(commandGroup, childNode, node, connectionList, recursive);
            }
        }
    }


    // Relink the given connection from the given node to a new target node.
    void RelinkConnectionTarget(MCore::CommandGroup* commandGroup, const uint32 animGraphID, const char* sourceNodeName, uint32 sourcePortNr, const char* oldTargetNodeName, uint32 oldTargetPortNr, const char* newTargetNodeName, uint32 newTargetPortNr)
    {
        AZStd::string commandString;

        // Delete the old connection first.
        commandString = AZStd::string::format("AnimGraphRemoveConnection -animGraphID %i -targetNode \"%s\" -targetPort %d -sourceNode \"%s\" -sourcePort %d",
                animGraphID,
                oldTargetNodeName,
                oldTargetPortNr,
                sourceNodeName,
                sourcePortNr);

        commandGroup->AddCommandString(commandString);

        // Create the new connection.
        commandString = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d",
                animGraphID,
                sourceNodeName,
                newTargetNodeName,
                sourcePortNr,
                newTargetPortNr);

        commandGroup->AddCommandString(commandString);
    }


    // Relink connection to a new source node and or port.
    void RelinkConnectionSource(MCore::CommandGroup* commandGroup, const uint32 animGraphID, const char* oldSourceNodeName, uint32 oldSourcePortNr, const char* newSourceNodeName, uint32 newSourcePortNr, const char* targetNodeName, uint32 targetPortNr)
    {
        AZStd::string commandString;

        // Delete the old connection first.
        commandString = AZStd::string::format("AnimGraphRemoveConnection -animGraphID %i -targetNode \"%s\" -targetPort %d -sourceNode \"%s\" -sourcePort %d",
                animGraphID,
                targetNodeName,
                targetPortNr,
                oldSourceNodeName,
                oldSourcePortNr);

        commandGroup->AddCommandString(commandString);

        // Create the new connection.
        commandString = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d",
                animGraphID,
                newSourceNodeName,
                targetNodeName,
                newSourcePortNr,
                targetPortNr);

        commandGroup->AddCommandString(commandString);
    }


    void DeleteStateTransition(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphStateTransition* transition, AZStd::vector<EMotionFX::AnimGraphStateTransition*>& transitionList)
    {
        // Skip directly if the transition is already in the list.
        if (AZStd::find(transitionList.begin(), transitionList.end(), transition) != transitionList.end())
        {
            return;
        }

        EMotionFX::AnimGraphNode* sourceNode = transition->GetSourceNode();
        EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();
        EMotionFX::AnimGraphNode* parentNode = targetNode->GetParentNode();

        // Safety check, we need to be working with states, not blend tree nodes.
        if (azrtti_typeid(parentNode) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            MCore::LogError("Cannot delete state machine transition. The anim graph node named '%s' is not a state.", targetNode->GetName());
            return;
        }

        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

        // Remove transition conditions back to the front.
        AZStd::string commandString;
        const int32 numConditions = static_cast<int32>(transition->GetNumConditions());
        for (int32 i = numConditions - 1; i >= 0; --i)
        {
            commandString = AZStd::string::format("AnimGraphRemoveCondition -animGraphID %i -stateMachineName \"%s\" -transitionID %i -conditionIndex %i", stateMachine->GetAnimGraph()->GetID(), stateMachine->GetName(), transition->GetID(), i);
            commandGroup->AddCommandString(commandString);
        }

        // If we are dealing with a wildcard transition, reset the source node so that we use the empty name for that.
        if (transition->GetIsWildcardTransition())
        {
            sourceNode = nullptr;
        }

        // If the source node is specified, get the node name.
        AZStd::string sourceNodeName;
        if (sourceNode)
        {
            sourceNodeName = sourceNode->GetName();
        }

        commandString = AZStd::string::format("AnimGraphRemoveConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -targetPort 0 -sourcePort 0 -id %d", targetNode->GetAnimGraph()->GetID(), sourceNodeName.c_str(), targetNode->GetName(), transition->GetID());

        transitionList.push_back(transition);
        commandGroup->AddCommandString(commandString);
    }


    // Delete all incoming and outgoing transitions for the given node.
    void DeleteStateTransitions(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* state, EMotionFX::AnimGraphNode* parentNode, AZStd::vector<EMotionFX::AnimGraphStateTransition*>& transitionList, bool recursive)
    {
        // Only do for state machines.
        if (parentNode && azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

            const uint32 numTransitions = stateMachine->GetNumTransitions();
            for (uint32 j = 0; j < numTransitions; ++j)
            {
                EMotionFX::AnimGraphStateTransition* transition = stateMachine->GetTransition(j);
                const EMotionFX::AnimGraphNode* sourceNode = transition->GetSourceNode();
                const EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();

                // If the connection starts at the given node, delete it.
                if (targetNode == state || (!transition->GetIsWildcardTransition() && sourceNode == state))
                {
                    DeleteStateTransition(commandGroup, transition, transitionList);
                }
            }
        }

        // Recursively delete all transitions.
        if (recursive)
        {
            const uint32 numChildNodes = state->GetNumChildNodes();
            for (uint32 i = 0; i < numChildNodes; ++i)
            {
                EMotionFX::AnimGraphNode* childNode = state->GetChildNode(i);
                DeleteStateTransitions(commandGroup, childNode, state, transitionList, recursive);
            }
        }
    }


    void CopyStateTransition(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphStateMachine* stateMachine, EMotionFX::AnimGraphStateTransition* transition, 
        bool cutMode, AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::unordered_map<EMotionFX::AnimGraphNode*, AZStd::string>& newNamesByCopiedNodes)
    {
        EMotionFX::AnimGraphNode* sourceNode = transition->GetSourceNode();
        EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();

        // We only copy transitions that are between nodes that are copied. Otherwise, the transition doesn't have a valid origin/target. If the transition is a wildcard
        // we only need the target.
        if (newNamesByCopiedNodes.find(targetNode) == newNamesByCopiedNodes.end() ||
            (!transition->GetIsWildcardTransition() && newNamesByCopiedNodes.find(sourceNode) == newNamesByCopiedNodes.end()))
        {
            return;
        }

        AZStd::string sourceName;
        if (!transition->GetIsWildcardTransition() && sourceNode)
        {
            // In case the source node is going to get copied too get the new name, if not just use name of the source node of the connection.
            sourceName = sourceNode->GetNameString();

            // Check if it requries renaming
            if (!cutMode)
            {
                auto itSourceRenamed = newNamesByCopiedNodes.find(sourceNode);
                if (itSourceRenamed != newNamesByCopiedNodes.end())
                {
                    sourceName = itSourceRenamed->second;
                }
            }
        }

        AZStd::string targetName = targetNode->GetNameString();
        if (!cutMode)
        {
            auto itTargetRenamed = newNamesByCopiedNodes.find(targetNode);
            if (itTargetRenamed != newNamesByCopiedNodes.end())
            {
                targetName = itTargetRenamed->second;
            }
        }

        AZStd::string commandString;
        commandString = AZStd::string::format("AnimGraphCreateConnection -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d -transitionType \"%s\" -contents {%s}",
                sourceName.c_str(),
                targetName.c_str(),
                0, // sourcePort
                0, // targetPort
                azrtti_typeid(transition).ToString<AZStd::string>().c_str(),
                MCore::ReflectionSerializer::Serialize(transition).GetValue().c_str());
        commandGroup->AddCommandString(commandString);
        int commandsSinceTransitionCreation = 1;

        // Find the name of the state machine
        AZStd::string stateMachineName = stateMachine->GetNameString();
        if (!cutMode)
        {
            auto itStateMachineRenamed = newNamesByCopiedNodes.find(stateMachine);
            if (itStateMachineRenamed != newNamesByCopiedNodes.end())
            {
                stateMachineName = itStateMachineRenamed->second;
            }

            AZStd::string attributesString;
            transition->GetAttributeStringForAffectedNodeIds(convertedIds, attributesString);
            if (!attributesString.empty())
            {
                // need to convert
                commandString = AZStd::string::format("AnimGraphAdjustConnection -animGraphID %d -stateMachine \"%s\" -transitionID \"%%LASTRESULT%i%%\" -attributesString {%s}",
                    stateMachine->GetAnimGraph()->GetID(),
                    stateMachineName.c_str(),
                    commandsSinceTransitionCreation,
                    attributesString.c_str());
                ++commandsSinceTransitionCreation;
                commandGroup->AddCommandString(commandString);
            }
        }

        const size_t numConditions = transition->GetNumConditions();
        for (size_t i = 0; i < numConditions; ++i)
        {
            EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(i);
            const AZ::TypeId conditionType = azrtti_typeid(condition);

            commandString = AZStd::string::format("AnimGraphAddCondition -animGraphID %i -stateMachineName \"%s\" -transitionID \"%%LASTRESULT%i%%\" -conditionType \"%s\" -contents {%s}",
                stateMachine->GetAnimGraph()->GetID(), 
                stateMachineName.c_str(),
                commandsSinceTransitionCreation,
                conditionType.ToString<AZStd::string>().c_str(),
                MCore::ReflectionSerializer::Serialize(condition).GetValue().c_str());
            ++commandsSinceTransitionCreation;
            commandGroup->AddCommandString(commandString);

            if (!cutMode)
            {
                AZStd::string attributesString;
                condition->GetAttributeStringForAffectedNodeIds(convertedIds, attributesString);
                if (!attributesString.empty())
                {
                    // need to convert
                    commandString = AZStd::string::format("AnimGraphAdjustCondition -animGraphID %i -stateMachineName \"%s\" -transitionID \"%%LASTRESULT%i%%\" -conditionIndex %i -attributesString {%s}",
                        stateMachine->GetAnimGraph()->GetID(),
                        stateMachineName.c_str(),
                        commandsSinceTransitionCreation,
                        i,
                        attributesString.c_str());
                    ++commandsSinceTransitionCreation;
                    commandGroup->AddCommandString(commandString);
                }
            }
        }
    }


    void CopyBlendTreeConnection(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* targetNode, EMotionFX::BlendTreeConnection* connection, 
        bool cutMode, AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::unordered_map<EMotionFX::AnimGraphNode*, AZStd::string>& newNamesByCopiedNodes)
    {
        EMotionFX::AnimGraphNode* sourceNode = connection->GetSourceNode();
        if (!sourceNode)
        {
            return;
        }

        // Only copy connections that are between nodes that are copied.
        if (newNamesByCopiedNodes.find(sourceNode) == newNamesByCopiedNodes.end() ||
            newNamesByCopiedNodes.find(targetNode) == newNamesByCopiedNodes.end())
        {
            return;
        }

        AZStd::string sourceNodeName = sourceNode->GetNameString();
        if (!cutMode)
        {
            auto itSourceRenamed = newNamesByCopiedNodes.find(sourceNode);
            if (itSourceRenamed != newNamesByCopiedNodes.end())
            {
                sourceNodeName = itSourceRenamed->second;
            }
        }

        AZStd::string targetNodeName = targetNode->GetNameString();
        if (!cutMode)
        {
            auto itTargetRenamed = newNamesByCopiedNodes.find(targetNode);
            if (itTargetRenamed != newNamesByCopiedNodes.end())
            {
                targetNodeName = itTargetRenamed->second;
            }
        }

        const uint16 sourcePort  = connection->GetSourcePort();
        const uint16 targetPort  = connection->GetTargetPort();
        
        const AZStd::string commandString = AZStd::string::format("AnimGraphCreateConnection -sourceNode \"%s\" -targetNode \"%s\" -sourcePortName \"%s\" -targetPortName \"%s\"",
                    sourceNodeName.c_str(),
                    targetNodeName.c_str(),
                    sourceNode->GetOutputPort(sourcePort).GetName(),
                    targetNode->GetInputPort(targetPort).GetName(),
                    targetPort);
        commandGroup->AddCommandString(commandString);
    }
} // namesapce EMotionFX