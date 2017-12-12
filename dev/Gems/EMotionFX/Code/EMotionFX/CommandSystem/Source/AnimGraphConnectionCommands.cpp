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
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/ActorManager.h>


namespace CommandSystem
{
    EMotionFX::AnimGraph* CommandsGetAnimGraph(const MCore::CommandLine& parameters, MCore::Command* command, MCore::String& outResult)
    {
        if (parameters.CheckIfHasParameter("animGraphID"))
        {
            const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", command);
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
            if (animGraph == nullptr)
            {
                outResult.Format("Anim graph with id '%i' cannot be found.", animGraphID);
                return nullptr;
            }
            return animGraph;
        }
        else
        {
            EMotionFX::AnimGraph* animGraph = GetCommandManager()->GetCurrentSelection().GetFirstAnimGraph();
            if (animGraph == nullptr)
            {
                outResult.Format("Anim graph id is not specified and no one anim graph is selected.");
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
        mTargetNode     = MCORE_INVALIDINDEX32;
        mSourceNode     = MCORE_INVALIDINDEX32;
        mConnectionID   = MCORE_INVALIDINDEX32;
        mTransitionType = MCORE_INVALIDINDEX32;
    }

    // destructor
    CommandAnimGraphCreateConnection::~CommandAnimGraphCreateConnection()
    {
    }


    // execute
    bool CommandAnimGraphCreateConnection::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
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
        mTransitionType = parameters.GetValueAsInt("transitionType", MCORE_INVALIDINDEX32);

        // get the node names
        MCore::String sourceName;
        MCore::String targetName;
        parameters.GetValue("sourceNode", "", &sourceName);
        parameters.GetValue("targetNode", "", &targetName);

        // find the source node in the anim graph
        EMotionFX::AnimGraphNode* sourceNode = animGraph->RecursiveFindNode(sourceName.AsChar());
        // this is allowed for wildcard transitions
        //if (sourceNode == nullptr && mTransitionType != AnimGraphWildCardTransition::TYPE_ID)
        //{
        //  outResult.Format("Cannot find source node with name '%s' in anim graph '%s'", sourceName.AsChar(), animGraph->GetName());
        //  return false;
        //}

        // find the target node in the anim graph
        EMotionFX::AnimGraphNode* targetNode = animGraph->RecursiveFindNode(targetName.AsChar());
        if (targetNode == nullptr)
        {
            outResult.Format("Cannot find target node with name '%s' in anim graph '%s'", targetName.AsChar(), animGraph->GetName());
            return false;
        }

        // get the ports
        mSourcePort = parameters.GetValueAsInt("sourcePort", 0);
        mTargetPort = parameters.GetValueAsInt("targetPort", 0);
        parameters.GetValue("sourcePortName", this, &mSourcePortName);
        parameters.GetValue("targetPortName", this, &mTargetPortName);

        // in case the source port got specified by name, overwrite the source port number
        if (!mSourcePortName.GetIsEmpty())
        {
            mSourcePort = sourceNode->FindOutputPortIndex(mSourcePortName.AsChar());

            // in case we want to add this connection to a parameter node while the parameter name doesn't exist, still return true so that copy paste doesn't fail
            if (sourceNode->GetType() == EMotionFX::BlendTreeParameterNode::TYPE_ID && mSourcePort == -1)
            {
                mConnectionID = MCORE_INVALIDINDEX32;
                return true;
            }
        }

        // in case the target port got specified by name, overwrite the source port number
        if (!mTargetPortName.GetIsEmpty())
        {
            mTargetPort = sourceNode->FindOutputPortIndex(mTargetPortName.AsChar());
        }

        // get the offsets
        mStartOffsetX   = parameters.GetValueAsInt("startOffsetX", 0);
        mStartOffsetY   = parameters.GetValueAsInt("startOffsetY", 0);
        mEndOffsetX     = parameters.GetValueAsInt("endOffsetX", 0);
        mEndOffsetY     = parameters.GetValueAsInt("endOffsetY", 0);

        // get the parent of the source node
        if (targetNode->GetParentNode() == nullptr)
        {
            outResult.Format("Cannot create connections between root state machines.");
            return false;
        }

        /*
        uint32 transitionNodeIndex = EMotionFX::GetAnimGraphManager().GetNodeFactory()->FindRegisteredNodeByTypeID( mTransitionType );
        if (transitionNodeIndex != MCORE_INVALIDINDEX32)
            MCore::LogInfo("Transition type = '%s'", EMotionFX::GetAnimGraphManager().GetNodeFactory()->GetRegisteredNode( transitionNodeIndex )->GetTypeString() );
        */

        // if the parent is state machine, we don't need to check the port ranges
        if (targetNode->GetParentNode()->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            if (sourceNode == nullptr)
            {
                outResult.Format("Cannot create blend tree connection in anim graph '%s' as the source node is not valid. ", animGraph->GetName());
                return false;
            }

            // verify port ranges
            if (mSourcePort >= (int32)sourceNode->GetNumOutputs() || mSourcePort < 0)
            {
                outResult.Format("The output port number is not valid for the given node. Node '%s' only has %d output ports.", sourceNode->GetName(), sourceNode->GetNumOutputs());
                return false;
            }

            if (mTargetPort >= (int32)targetNode->GetNumInputs() || mTargetPort < 0)
            {
                outResult.Format("The input port number is not valid for the given node. Node '%s' only has %d input ports.", targetNode->GetName(), targetNode->GetNumInputs());
                return false;
            }

            // check if connection already exists
            if (targetNode->GetHasConnection(sourceNode, static_cast<uint16>(mSourcePort), static_cast<uint16>(mTargetPort)))
            {
                outResult.Format("The connection you are trying to create already exists!");
                return false;
            }

            // create the connection and auto assign an id first of all
            EMotionFX::BlendTreeConnection* connection = targetNode->AddConnection(sourceNode, static_cast<uint16>(mSourcePort), static_cast<uint16>(mTargetPort));

            // get the connection ID
            if (parameters.CheckIfHasParameter("id"))
            {
                // manually assign the ID
                mConnectionID = parameters.GetValueAsInt("id", this);
                connection->SetID(mConnectionID);
            }

            // get the connection id
            mConnectionID = connection->GetID();

            // set the attributes from a string
            /*if (parameters.CheckIfHasParameter("attributesString"))
            {
                String attributesString = parameters.GetValue("attributesString", this);
                connection->InitAttributesFromString( attributesString.AsChar() );
            }*/
        }
        else // create a state transition
        {
            // get the state machine
            EMotionFX::AnimGraphStateMachine* machine = (EMotionFX::AnimGraphStateMachine*)targetNode->GetParentNode();

            // try to create the anim graph node
            EMotionFX::AnimGraphObject* object = EMotionFX::GetAnimGraphManager().GetObjectFactory()->CreateObjectByTypeID(animGraph, mTransitionType);
            if (object == nullptr)
            {
                outResult.Format("Cannot create transition of type %d", mTransitionType);
                return false;
            }

            // check if this is really a transition
            if (object->GetBaseType() != EMotionFX::AnimGraphStateTransition::BASETYPE_ID)
            {
                outResult.Format("Cannot create state transition of type %d, because this object type is not inherited from AnimGraphStateTransition.", mTransitionType);
                return false;
            }

            // typecast the anim graph node into a state transition node
            EMotionFX::AnimGraphStateTransition* transition = static_cast<EMotionFX::AnimGraphStateTransition*>(object);

            // check if we are dealing with a wildcard transition
            bool isWildcardTransition = false;
            if (sourceNode == nullptr)
            {
                isWildcardTransition = true;
            }

            // setup the transition properties
            transition->SetSourceNode(sourceNode);
            transition->SetTargetNode(targetNode);
            transition->SetVisualOffsets(mStartOffsetX, mStartOffsetY, mEndOffsetX, mEndOffsetY);
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
            if (mConnectionID != MCORE_INVALIDINDEX32)
            {
                transition->SetID(mConnectionID);
            }

            // get the transition id and save it for undo
            mConnectionID = transition->GetID();

            // set the attributes from a string
            if (parameters.CheckIfHasParameter("attributesString"))
            {
                MCore::String attributesString;
                parameters.GetValue("attributesString", this, &attributesString);
                transition->InitAttributesFromString(attributesString.AsChar());
            }

            // add the transition to the state machine
            machine->AddTransition(transition);
        }

        mTargetNode = MCORE_INVALIDINDEX32;
        mSourceNode = MCORE_INVALIDINDEX32;
        if (targetNode)
        {
            mTargetNode = targetNode->GetID();
        }
        if (sourceNode)
        {
            mSourceNode = sourceNode->GetID();
        }

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        // set the command result to the connection id
        outResult.Format("%i", mConnectionID);

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
    bool CommandAnimGraphCreateConnection::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult.Format("The anim graph with id '%i' does not exist anymore.", mAnimGraphID);
            return false;
        }

        // in case of a wildcard transition the source node is the invalid index, so that's all fine
        EMotionFX::AnimGraphNode* sourceNode = animGraph->RecursiveFindNodeByID(mSourceNode);
        EMotionFX::AnimGraphNode* targetNode = animGraph->RecursiveFindNodeByID(mTargetNode);

        if (sourceNode == nullptr || targetNode == nullptr)
        {
            outResult.Format("Source or target node does not exist!");
            return false;
        }

        // get the source node name, special path here as wildcard transitions have a nullptr source node
        MCore::String sourceNodeName;
        if (sourceNode)
        {
            sourceNodeName = sourceNode->GetName();
        }

        // delete the connection
        MCore::String commandString;
        commandString.Format("AnimGraphRemoveConnection -animGraphID %i -targetNode \"%s\" -targetPort %d -sourceNode \"%s\" -sourcePort %d -id %d",
            animGraph->GetID(),
            targetNode->GetName(),
            mTargetPort,
            sourceNodeName.AsChar(),
            mSourcePort,
            mConnectionID);

        // execute the command without putting it in the history
        if (GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult) == false)
        {
            if (outResult.GetLength() > 0)
            {
                MCore::LogError(outResult.AsChar());
            }

            return false;
        }

        // reset the data used for undo and redo
        mSourceNode     = MCORE_INVALIDINDEX32;
        mTargetNode     = MCORE_INVALIDINDEX32;

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
        GetSyntax().AddParameter("transitionType", "The transition type ID. This is the class type ID of the AnimGraphStateTransition inherited node types.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("attributesString", "The connection attributes as string.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
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
        mTargetNodeID   = MCORE_INVALIDINDEX32;
        mSourceNodeID   = MCORE_INVALIDINDEX32;
        mSourcePort     = MCORE_INVALIDINDEX32;
        mTargetPort     = MCORE_INVALIDINDEX32;
        mTransitionType = MCORE_INVALIDINDEX32;
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
    bool CommandAnimGraphRemoveConnection::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
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
        MCore::String sourceName;
        MCore::String targetName;
        parameters.GetValue("sourceNode", "", &sourceName);
        parameters.GetValue("targetNode", "", &targetName);

        // find the source node in the anim graph
        EMotionFX::AnimGraphNode* sourceNode = animGraph->RecursiveFindNode(sourceName.AsChar());
        //if (sourceNode == nullptr)
        //{
        //  outResult.Format("Cannot find source node with name '%s' in anim graph '%s'", sourceName.AsChar(), animGraph->GetName());
        //  return false;
        //}

        // find the target node in the anim graph
        EMotionFX::AnimGraphNode* targetNode = animGraph->RecursiveFindNode(targetName.AsChar());
        if (targetNode == nullptr)
        {
            outResult.Format("Cannot find target node with name '%s' in anim graph '%s'", targetName.AsChar(), animGraph->GetName());
            return false;
        }

        // get the ids from the source and destination nodes
        mSourceNodeID = MCORE_INVALIDINDEX32;
        if (sourceNode)
        {
            mSourceNodeID = sourceNode->GetID();
        }
        mTargetNodeID = targetNode->GetID();

        // get the ports
        mSourcePort = parameters.GetValueAsInt("sourcePort", 0);
        mTargetPort = parameters.GetValueAsInt("targetPort", 0);

        // get the parent of the source node
        if (targetNode->GetParentNode() == nullptr)
        {
            outResult.Format("Cannot remove connections between root state machines.");
            return false;
        }

        // if the parent is state machine, we don't need to check the port ranges
        if (targetNode->GetParentNode()->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            if (sourceNode == nullptr)
            {
                outResult.Format("Cannot remove blend tree connection in anim graph '%s' as the source node is not valid.", animGraph->GetName());
                return false;
            }

            // verify port ranges
            if (mSourcePort >= (int32)sourceNode->GetNumOutputs() || mSourcePort < 0)
            {
                outResult.Format("The output port number is not valid for the given node. Node '%s' only has %d output ports.", sourceNode->GetName(), sourceNode->GetNumOutputs());
                return false;
            }

            if (mTargetPort >= (int32)targetNode->GetNumInputs() || mTargetPort < 0)
            {
                outResult.Format("The input port number is not valid for the given node. Node '%s' only has %d input ports.", targetNode->GetName(), targetNode->GetNumInputs());
                return false;
            }

            // check if connection already exists
            if (!targetNode->GetHasConnection(sourceNode, static_cast<uint16>(mSourcePort), static_cast<uint16>(mTargetPort)))
            {
                outResult.Format("The connection you are trying to remove doesn't exist!");
                return false;
            }

            // get the connection ID and store it
            mConnectionID = targetNode->FindConnection(sourceNode, static_cast<uint16>(mSourcePort), static_cast<uint16>(mTargetPort))->GetID();

            /*      // reset the synced flag
                    if (sourceNode)
                    {
                        const uint32 numAnimGraphInstances = EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances();
                        for (uint32 i=0; i<numAnimGraphInstances; ++i)
                            sourceNode->SetIsSynced(EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i), false);
                    }*/

            // create the connection
            targetNode->RemoveConnection(sourceNode, static_cast<uint16>(mSourcePort), static_cast<uint16>(mTargetPort));
        }
        else // remove a state transition
        {
            // get the transition id
            const uint32 transitionID = parameters.GetValueAsInt("id", this);
            if (transitionID == MCORE_INVALIDINDEX32)
            {
                outResult.Format("You cannot remove a state transition with an invalid id. (Did you specify the id parameter?)");
                return false;
            }

            // get the state machine
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(targetNode->GetParentNode());

            // find the transition index based on the id
            const uint32 transitionIndex = stateMachine->FindTransitionIndexByID(transitionID);
            if (transitionIndex == MCORE_INVALIDINDEX32)
            {
                outResult.Format("The state transition you are trying to remove cannot be found.");
                return false;
            }

            // save the transition information for undo
            EMotionFX::AnimGraphStateTransition* transition = stateMachine->GetTransition(transitionIndex);
            mStartOffsetX       = transition->GetVisualStartOffsetX();
            mStartOffsetY       = transition->GetVisualStartOffsetY();
            mEndOffsetX         = transition->GetVisualEndOffsetX();
            mEndOffsetY         = transition->GetVisualEndOffsetY();
            mTransitionType     = transition->GetType();
            mConnectionID       = transition->GetID();
            mOldAttributesString = transition->CreateAttributesString();

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
    bool CommandAnimGraphRemoveConnection::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult.Format("The anim graph with id '%i' does not exist anymore.", mAnimGraphID);
            return false;
        }

        if (mTargetNodeID == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        MCore::String sourceNodeName;
        if (mSourceNodeID != MCORE_INVALIDINDEX32)
        {
            sourceNodeName = MCore::GetStringIDGenerator().GetName(mSourceNodeID).AsChar();
        }

        MCore::String commandString;
        commandString.Format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d -startOffsetX %d -startOffsetY %d -endOffsetX %d -endOffsetY %d -id %d -transitionType %d",
            animGraph->GetID(),
            sourceNodeName.AsChar(),
            MCore::GetStringIDGenerator().GetName(mTargetNodeID).AsChar(),
            mSourcePort,
            mTargetPort,
            mStartOffsetX, mStartOffsetY,
            mEndOffsetX, mEndOffsetY,
            mConnectionID,
            mTransitionType);

        // add the old attributes
        if (mOldAttributesString.GetIsEmpty() == false)
        {
            MCore::String attributesString;
            attributesString.Format(" -attributesString \"%s\"", mOldAttributesString.AsChar());
            commandString += attributesString;
        }

        if (GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult) == false)
        {
            if (outResult.GetLength() > 0)
            {
                MCore::LogError(outResult.AsChar());
            }

            return false;
        }

        mTargetNodeID   = MCORE_INVALIDINDEX32;
        mSourceNodeID   = MCORE_INVALIDINDEX32;
        mConnectionID   = MCORE_INVALIDINDEX32;
        mSourcePort     = MCORE_INVALIDINDEX32;
        mTargetPort     = MCORE_INVALIDINDEX32;
        mStartOffsetX   = 0;
        mStartOffsetY   = 0;
        mEndOffsetX     = 0;
        mEndOffsetY     = 0;

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


    // execute
    bool CommandAnimGraphAdjustConnection::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        mAnimGraphID = animGraph->GetID();

        MCore::String stateMachineName;
        parameters.GetValue("stateMachine", "", &stateMachineName);

        // find the node in the anim graph
        EMotionFX::AnimGraphNode* stateMachineNode = animGraph->RecursiveFindNode(stateMachineName.AsChar());
        if (stateMachineNode == nullptr)
        {
            outResult.Format("Cannot find state machine node with name '%s' in anim graph '%s'", stateMachineName.AsChar(), animGraph->GetName());
            return false;
        }

        // type cast it to a state machine node
        if (stateMachineNode->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            outResult.Format("Anim graph node named '%s' is not a state machine.", stateMachineName.AsChar());
            return false;
        }
        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(stateMachineNode);

        // get the transition id and find the transition index based on the id
        const uint32 transitionID    = parameters.GetValueAsInt("transitionID", this);
        const uint32 transitionIndex = stateMachine->FindTransitionIndexByID(transitionID);
        if (transitionIndex == MCORE_INVALIDINDEX32)
        {
            outResult.Format("The state transition you are trying to adjust cannot be found.");
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
            // get the new source node name
            MCore::String newSourceName;
            parameters.GetValue("sourceNode", "", &newSourceName);

            // find the node in the anim graph
            EMotionFX::AnimGraphNode* newSourceNode = animGraph->RecursiveFindNode(newSourceName.AsChar());
            if (newSourceNode == nullptr)
            {
                outResult.Format("Cannot find new source node with name '%s' in anim graph '%s'", newSourceName.AsChar(), animGraph->GetName());
                return false;
            }

            // set the new source node
            transition->SetSourceNode(newSourceNode);
        }

        // set the new target node
        if (parameters.CheckIfHasParameter("targetNode"))
        {
            // get the new target node name
            MCore::String newTargetName;
            parameters.GetValue("targetNode", "", &newTargetName);

            // find the node in the anim graph
            EMotionFX::AnimGraphNode* newTargetNode = animGraph->RecursiveFindNode(newTargetName.AsChar());
            if (newTargetNode == nullptr)
            {
                outResult.Format("Cannot find new target node with name '%s' in anim graph '%s'", newTargetName.AsChar(), animGraph->GetName());
                return false;
            }

            // set the new target node
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
    bool CommandAnimGraphAdjustConnection::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult.Format("The anim graph with id '%i' does not exist anymore.", mAnimGraphID);
            return false;
        }

        MCore::String commandString;
        commandString.Reserve(2048);

        // get the state machine name and the transition id to work on
        MCore::String stateMachineName;
        parameters.GetValue("stateMachine", "", &stateMachineName);
        const uint32 transitionID = parameters.GetValueAsInt("transitionID", this);

        // construct the command including the required parameters
        commandString.Format("AnimGraphAdjustConnection -animGraphID %i -stateMachine \"%s\" -transitionID %i -startOffsetX %i -startOffsetY %i -endOffsetX %i -endOffsetY %i ", animGraph->GetID(), stateMachineName.AsChar(), transitionID, mStartOffsetX, mStartOffsetY, mEndOffsetX, mEndOffsetY);

        if (!mOldSourceNodeName.GetIsEmpty())
        {
            commandString.FormatAdd("-sourceNode \"%s\" ", mOldSourceNodeName.AsChar());
        }
        if (!mOldTargetNodeName.GetIsEmpty())
        {
            commandString.FormatAdd("-targetNode \"%s\" ", mOldTargetNodeName.AsChar());
        }

        if (parameters.CheckIfHasParameter("isDisabled"))
        {
            commandString.FormatAdd("-isDisabled %i ", mOldDisabledFlag);
        }

        if (GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult) == false)
        {
            if (outResult.GetIsEmpty() == false)
            {
                MCore::LogError(outResult.AsChar());
            }

            return false;
        }

        mOldTargetNodeName.Clear();
        mOldSourceNodeName.Clear();
        mStartOffsetX   = 0;
        mStartOffsetY   = 0;
        mEndOffsetX     = 0;
        mEndOffsetY     = 0;

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
        if(AZStd::find(connectionList.begin(), connectionList.end(), connection) != connectionList.end())
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
        if(AZStd::find(transitionList.begin(), transitionList.end(), transition) != transitionList.end())
        {
            return;
        }

        EMotionFX::AnimGraphNode* sourceNode = transition->GetSourceNode();
        EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();
        EMotionFX::AnimGraphNode* parentNode = targetNode->GetParentNode();

        // Safety check, we need to be working with states, not blend tree nodes.
        if (parentNode->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            MCore::LogError("Cannot delete state machine transition. The anim graph node named '%s' is not a state.", targetNode->GetName());
            return;
        }

        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

        // Remove transition conditions back to the front.
        AZStd::string commandString;
        const uint32 numConditions = transition->GetNumConditions();
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
        if (parentNode && parentNode->GetType() == EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

            const uint32 numTransitions = stateMachine->GetNumTransitions();
            for (uint32 j = 0; j < numTransitions; ++j)
            {
                EMotionFX::AnimGraphStateTransition* transition = stateMachine->GetTransition(j);

                // If the connection starts at the given node, delete it.
                if (transition->GetTargetNode() == state || (!transition->GetIsWildcardTransition() && transition->GetSourceNode() == state))
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


    void CopyStateTransition(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphStateMachine* stateMachine, const AZStd::string& parentName, EMotionFX::AnimGraphStateTransition* transition, const AZStd::vector<EMotionFX::AnimGraphNode*>& copiedNodes, const AZStd::vector<AZStd::string>& nameReserveList)
    {
        MCORE_UNUSED(nameReserveList);
        MCORE_UNUSED(stateMachine);
        MCORE_UNUSED(animGraph);

        EMotionFX::AnimGraphNode* sourceNode = transition->GetSourceNode();
        EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();

        AZStd::string sourceName;
        if (!transition->GetIsWildcardTransition() && sourceNode)
        {
            // In case the source node is going to get copied too get the new name, if not just use name of the source node of the connection.
            sourceName = sourceNode->GetName();
        }

        // Only copy transitions that are between nodes that are copied.
        if (AZStd::find(copiedNodes.begin(), copiedNodes.end(), targetNode) == copiedNodes.end() ||
            (!transition->GetIsWildcardTransition() && AZStd::find(copiedNodes.begin(), copiedNodes.end(), sourceNode) == copiedNodes.end()))
        {
            return;
        }

        AZStd::string commandString;
        commandString = AZStd::string::format("AnimGraphCreateConnection -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d -startOffsetX %d -startOffsetY %d -endOffsetX %d -endOffsetY %d -transitionType %d -attributesString \"%s\"",
            sourceName.c_str(),
            targetNode->GetName(),
            0, // sourcePort
            0, // targetPort
            transition->GetVisualStartOffsetX(),
            transition->GetVisualStartOffsetY(),
            transition->GetVisualEndOffsetX(),
            transition->GetVisualEndOffsetY(),
            transition->GetType(),
            transition->CreateAttributesString().AsChar());

        commandGroup->AddCommandString(commandString);


        const uint32 numConditions = transition->GetNumConditions();
        for (uint32 i = 0; i < numConditions; ++i)
        {
            EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(i);
            const uint32 conditionType = EMotionFX::GetAnimGraphManager().GetObjectFactory()->FindRegisteredObjectByTypeID(condition->GetType());

            commandString = AZStd::string::format("AnimGraphAddCondition -stateMachineName \"%s\" -transitionID \"%%LASTRESULT%%\" -conditionType %d -attributesString \"%s\"",
                parentName.c_str(),
                conditionType,
                condition->CreateAttributesString().AsChar());

            commandGroup->AddCommandString(commandString);
        }
    }


    void CopyBlendTreeConnection(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* targetNode, EMotionFX::BlendTreeConnection* connection, const AZStd::vector<EMotionFX::AnimGraphNode*>& copiedNodes, const AZStd::vector<AZStd::string>& nameReserveList)
    {
        MCORE_UNUSED(nameReserveList);
        MCORE_UNUSED(animGraph);

        const uint16                sourcePort  = connection->GetSourcePort();
        const uint16                targetPort  = connection->GetTargetPort();
        EMotionFX::AnimGraphNode*   sourceNode  = connection->GetSourceNode();

        if (!sourceNode)
        {
            return;
        }

        // Only copy connections that are between nodes that are copied.
        if (AZStd::find(copiedNodes.begin(), copiedNodes.end(), targetNode) == copiedNodes.end() ||
            AZStd::find(copiedNodes.begin(), copiedNodes.end(), sourceNode) == copiedNodes.end())
        {
            return;
        }

        AZStd::string commandString;
        if (sourceNode->GetType() == EMotionFX::BlendTreeParameterNode::TYPE_ID)
        {
            commandString = AZStd::string::format("AnimGraphCreateConnection -sourceNode \"%s\" -targetNode \"%s\" -sourcePortName \"%s\" -targetPort %d",
                sourceNode->GetName(),
                targetNode->GetName(),
                sourceNode->GetOutputPort(sourcePort).GetName(),
                targetPort);
        }
        else
        {
            commandString = AZStd::string::format("AnimGraphCreateConnection -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d",
                sourceNode->GetName(),
                targetNode->GetName(),
                sourcePort,
                targetPort);
        }

        commandGroup->AddCommandString(commandString);
    }
} // namesapce EMotionFX