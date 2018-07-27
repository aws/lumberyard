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

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <EMotionFX/Source/AnimGraphBus.h>
#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"
#include "AnimGraphInstance.h"
#include "ActorInstance.h"
#include "EventManager.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraphNodeGroup.h"
#include "AnimGraphTransitionCondition.h"
#include "AnimGraphStateTransition.h"
#include "AnimGraphObjectData.h"
#include "AnimGraphEventBuffer.h"
#include "AnimGraphManager.h"
#include "AnimGraph.h"
#include "Recorder.h"

#include "AnimGraphMotionNode.h"
#include "ActorManager.h"
#include "EMotionFXManager.h"

#include <MCore/Source/StringIdPool.h>
#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/Attribute.h>
#include <MCore/Source/AttributeFactory.h>
#include <MCore/Source/Stream.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/Random.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNode, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNode::Port, AnimGraphAllocator, 0)

    AnimGraphNode::AnimGraphNode()
        : AnimGraphObject(nullptr)
        , m_id(AnimGraphNodeId::Create())
        , mNodeIndex(MCORE_INVALIDINDEX32)
        , mParentNode(nullptr)
        , mCustomData(nullptr)
        , mPosX(0)
        , mPosY(0)
        , mVisEnabled(false)
        , mIsCollapsed(false)
        , mDisabled(false)
    {
        mVisualizeColor = MCore::GenerateColor();

        mInputPortChangeFunction    = &AnimGraphNodeDefaultInputPortChangeFunction;

        mInputPorts.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);
        mOutputPorts.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);
    }


    AnimGraphNode::AnimGraphNode(AnimGraph* animGraph, const char* name)
        : AnimGraphNode()
    {
        SetName(name);
        InitAfterLoading(animGraph);
    }


    AnimGraphNode::~AnimGraphNode()
    {
        RemoveAllConnections();
        RemoveAllChildNodes();

        if (mAnimGraph)
        {
            mAnimGraph->RemoveObject(this);
        }
    }


    void AnimGraphNode::Reinit()
    {
        for (BlendTreeConnection* connection : mConnections)
        {
            connection->Reinit();
        }

        for (AnimGraphNode* childNode : mChildNodes)
        {
            childNode->Reinit();
        }
    }


    bool AnimGraphNode::InitAfterLoading(AnimGraph* animGraph)
    {
        bool result = true;
        SetAnimGraph(animGraph);

        if (animGraph)
        {
            animGraph->AddObject(this);
        }

        // Initialize connections.
        for (BlendTreeConnection* connection : mConnections)
        {
            connection->InitAfterLoading(animGraph);
        }

        // Initialize child nodes.
        for (AnimGraphNode* childNode : mChildNodes)
        {
            // Sync the child node's parent.
            childNode->SetParentNode(this);

            if (!childNode->InitAfterLoading(animGraph))
            {
                result = false;
            }
        }

        return result;
    }


    // copy base settings to the other node
    void AnimGraphNode::CopyBaseNodeTo(AnimGraphNode* node) const
    {
        //CopyBaseObjectTo( node );

        // now copy the node related things
        // the parent
        //if (mParentNode)
        //node->mParentNode = node->GetAnimGraph()->RecursiveFindNodeByID( mParentNode->GetID() );

        // copy the easy values
        node->m_name            = m_name;
        node->m_id              = m_id;
        node->mNodeInfo         = mNodeInfo;
        node->mCustomData       = mCustomData;
        node->mDisabled         = mDisabled;
        node->mPosX             = mPosX;
        node->mPosY             = mPosY;
        node->mVisualizeColor   = mVisualizeColor;
        node->mVisEnabled       = mVisEnabled;
        node->mIsCollapsed      = mIsCollapsed;
    }


    void AnimGraphNode::RemoveAllConnections()
    {
        for (BlendTreeConnection* connection : mConnections)
        {
            delete connection;
        }

        mConnections.clear();
    }


    // add a connection
    BlendTreeConnection* AnimGraphNode::AddConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort)
    {
        // make sure the source and target ports are in range
        if (mInputPorts.GetIsValidIndex(targetPort) == false || sourceNode->mOutputPorts.GetIsValidIndex(sourcePort) == false)
        {
            return nullptr;
        }

        BlendTreeConnection* connection = aznew BlendTreeConnection(sourceNode, sourcePort, targetPort);
        mConnections.push_back(connection);
        mInputPorts[targetPort].mConnection = connection;
        sourceNode->mOutputPorts[sourcePort].mConnection = connection;
        return connection;
    }


    // validate the connections
    bool AnimGraphNode::ValidateConnections() const
    {
        for (const BlendTreeConnection* connection : mConnections)
        {
            if (!connection->GetIsValid())
            {
                return false;
            }
        }

        return true;
    }


    // find a given connection to a given node
    uint32 AnimGraphNode::FindConnectionFromNode(AnimGraphNode* sourceNode, uint16 sourcePort) const
    {
        const size_t numConnections = mConnections.size();
        for (size_t i = 0; i < numConnections; ++i)
        {
            if (mConnections[i]->GetSourceNode() == sourceNode && mConnections[i]->GetSourcePort() == sourcePort)
            {
                return static_cast<uint32>(i);
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // check if the given input port is connected
    bool AnimGraphNode::CheckIfIsInputPortConnected(uint16 inputPort) const
    {
        for (const BlendTreeConnection* connection : mConnections)
        {
            if (connection->GetTargetPort() == inputPort)
            {
                return true;
            }
        }

        // failure, no connected plugged into the given port
        return false;
    }


    // remove all child nodes
    void AnimGraphNode::RemoveAllChildNodes(bool delFromMem)
    {
        if (delFromMem)
        {
            for (AnimGraphNode* childNode : mChildNodes)
            {
                delete childNode;
            }
        }

        mChildNodes.clear();

        // trigger that we removed nodes
        GetEventManager().OnRemovedChildNode(mAnimGraph, this);

        // TODO: remove the nodes from the node groups of the anim graph as well here
    }


    // remove a given node
    void AnimGraphNode::RemoveChildNode(uint32 index, bool delFromMem)
    {
        // remove the node from its node group
        AnimGraphNodeGroup* nodeGroup = mAnimGraph->FindNodeGroupForNode(mChildNodes[index]);
        if (nodeGroup)
        {
            nodeGroup->RemoveNodeById(mChildNodes[index]->GetId());
        }

        // delete the node from memory
        if (delFromMem)
        {
            delete mChildNodes[index];
        }

        // delete the node from the child array
        mChildNodes.erase(mChildNodes.begin() + index);

        // trigger callbacks
        GetEventManager().OnRemovedChildNode(mAnimGraph, this);
    }


    // remove a node by pointer
    void AnimGraphNode::RemoveChildNodeByPointer(AnimGraphNode* node, bool delFromMem)
    {
        // find the index of the given node in the child node array and remove it in case the index is valid
        const auto iterator = AZStd::find(mChildNodes.begin(), mChildNodes.end(), node);

        if (iterator != mChildNodes.end())
        {
            const uint32 index = static_cast<uint32>(iterator - mChildNodes.begin());
            RemoveChildNode(index, delFromMem);
        }
    }


    AnimGraphNode* AnimGraphNode::RecursiveFindNodeByName(const char* nodeName) const
    {
        if (AzFramework::StringFunc::Equal(m_name.c_str(), nodeName, true /* case sensitive */))
        {
            return const_cast<AnimGraphNode*>(this);
        }

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            AnimGraphNode* result = childNode->RecursiveFindNodeByName(nodeName);
            if (result)
            {
                return result;
            }
        }

        return nullptr;
    }


    bool AnimGraphNode::RecursiveIsNodeNameUnique(const AZStd::string& newNameCandidate, const AnimGraphNode* forNode) const
    {
        if (forNode != this && m_name == newNameCandidate)
        {
            return false;
        }

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            if (!childNode->RecursiveIsNodeNameUnique(newNameCandidate, forNode))
            {
                return false;
            }
        }

        return true;
    }


    AnimGraphNode* AnimGraphNode::RecursiveFindNodeById(AnimGraphNodeId nodeId) const
    {
        if (m_id == nodeId)
        {
            return const_cast<AnimGraphNode*>(this);
        }

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            AnimGraphNode* result = childNode->RecursiveFindNodeById(nodeId);
            if (result)
            {
                return result;
            }
        }

        return nullptr;
    }


    // find a child node by name
    AnimGraphNode* AnimGraphNode::FindChildNode(const char* name) const
    {
        for (AnimGraphNode* childNode : mChildNodes)
        {
            // compare the node name with the parameter and return a pointer to the node in case they are equal
            if (AzFramework::StringFunc::Equal(childNode->GetName(), name, true /* case sensitive */))
            {
                return childNode;
            }
        }

        // failure, return nullptr pointer
        return nullptr;
    }


    AnimGraphNode* AnimGraphNode::FindChildNodeById(AnimGraphNodeId childId) const
    {
        for (AnimGraphNode* childNode : mChildNodes)
        {
            if (childNode->GetId() == childId)
            {
                return childNode;
            }
        }

        return nullptr;
    }


    // find a child node index by name
    uint32 AnimGraphNode::FindChildNodeIndex(const char* name) const
    {
        const size_t numChildNodes = mChildNodes.size();
        for (size_t i = 0; i < numChildNodes; ++i)
        {
            // compare the node name with the parameter and return the relative child node index in case they are equal
            if (AzFramework::StringFunc::Equal(mChildNodes[i]->GetNameString().c_str(), name, true /* case sensitive */))
            {
                return static_cast<uint32>(i);
            }
        }

        // failure, return invalid index
        return MCORE_INVALIDINDEX32;
    }


    // find a child node index
    uint32 AnimGraphNode::FindChildNodeIndex(AnimGraphNode* node) const
    {
        const auto iterator = AZStd::find(mChildNodes.begin(), mChildNodes.end(), node);
        if (iterator == mChildNodes.end())
        {
            return MCORE_INVALIDINDEX32;
        }

        const size_t index = iterator - mChildNodes.begin();
        return static_cast<uint32>(index);
    }


    AnimGraphNode* AnimGraphNode::FindFirstChildNodeOfType(const AZ::TypeId& nodeType) const
    {
        for (AnimGraphNode* childNode : mChildNodes)
        {
            if (azrtti_typeid(childNode) == nodeType)
            {
                return childNode;
            }
        }

        return nullptr;
    }


    bool AnimGraphNode::HasChildNodeOfType(const AZ::TypeId& nodeType) const
    {
        for (const AnimGraphNode* childNode : mChildNodes)
        {
            if (azrtti_typeid(childNode) == nodeType)
            {
                return true;
            }
        }

        return false;
    }


    // does this node has a specific incoming connection?
    bool AnimGraphNode::GetHasConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort) const
    {
        for (const BlendTreeConnection* connection : mConnections)
        {
            if (connection->GetSourceNode() == sourceNode && connection->GetSourcePort() == sourcePort && connection->GetTargetPort() == targetPort)
            {
                return true;
            }
        }

        return false;
    }


    // remove a given connection
    void AnimGraphNode::RemoveConnection(BlendTreeConnection* connection, bool delFromMem)
    {
        mInputPorts[connection->GetTargetPort()].mConnection = nullptr;

        AnimGraphNode* sourceNode = connection->GetSourceNode();
        if (sourceNode)
        {
            sourceNode->mOutputPorts[connection->GetSourcePort()].mConnection = nullptr;
        }

        // Remove object by value.
        mConnections.erase(AZStd::remove(mConnections.begin(), mConnections.end(), connection), mConnections.end());
        if (delFromMem)
        {
            delete connection;
        }
    }


    // remove a given connection
    void AnimGraphNode::RemoveConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort)
    {
        // for all input connections
        for (BlendTreeConnection* connection : mConnections)
        {
            if (connection->GetSourceNode() == sourceNode && connection->GetSourcePort() == sourcePort && connection->GetTargetPort() == targetPort)
            {
                RemoveConnection(connection, true);
                return;
            }
        }
    }


    // remove a connection with a given ID
    bool AnimGraphNode::RemoveConnectionByID(uint32 id, bool delFromMem)
    {
        const size_t numConnections = mConnections.size();
        for (size_t i = 0; i < numConnections; ++i)
        {
            if (mConnections[i]->GetId() == id)
            {
                mInputPorts[mConnections[i]->GetTargetPort()].mConnection = nullptr;

                AnimGraphNode* sourceNode = mConnections[i]->GetSourceNode();
                if (sourceNode)
                {
                    sourceNode->mOutputPorts[mConnections[i]->GetSourcePort()].mConnection = nullptr;
                }

                if (delFromMem)
                {
                    delete mConnections[i];
                }

                mConnections.erase(mConnections.begin() + i);
            }
        }

        return false;
    }


    // find a given connection
    BlendTreeConnection* AnimGraphNode::FindConnection(const AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort) const
    {
        for (BlendTreeConnection* connection : mConnections)
        {
            if (connection->GetSourceNode() == sourceNode && connection->GetSourcePort() == sourcePort && connection->GetTargetPort() == targetPort)
            {
                return connection;
            }
        }

        return nullptr;
    }


    // get the number of input ports
    uint32 AnimGraphNode::GetNumInputs() const
    {
        return mInputPorts.GetLength();
    }


    // get the number of output ports
    uint32 AnimGraphNode::GetNumOutputs() const
    {
        return mOutputPorts.GetLength();
    }


    // get the input ports
    const MCore::Array<AnimGraphNode::Port>& AnimGraphNode::GetInputPorts() const
    {
        return mInputPorts;
    }


    // get the output ports
    const MCore::Array<AnimGraphNode::Port>& AnimGraphNode::GetOutputPorts() const
    {
        return mOutputPorts;
    }


    // initialize the input ports
    void AnimGraphNode::InitInputPorts(uint32 numPorts)
    {
        mInputPorts.Resize(numPorts);
    }


    // initialize the output ports
    void AnimGraphNode::InitOutputPorts(uint32 numPorts)
    {
        mOutputPorts.Resize(numPorts);
    }


    // find a given output port number
    uint32 AnimGraphNode::FindOutputPortIndex(const char* name) const
    {
        const uint32 numPorts = mOutputPorts.GetLength();
        for (uint32 i = 0; i < numPorts; ++i)
        {
            // if the port name is equal to the name we are searching for, return the index
            if (AzFramework::StringFunc::Equal(MCore::GetStringIdPool().GetName(mOutputPorts[i].mNameID).c_str(), name, true /* case sensitive */))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find a given input port number
    uint32 AnimGraphNode::FindInputPortIndex(const char* name) const
    {
        const uint32 numPorts = mInputPorts.GetLength();
        for (uint32 i = 0; i < numPorts; ++i)
        {
            // if the port name is equal to the name we are searching for, return the index
            if (AzFramework::StringFunc::Equal(MCore::GetStringIdPool().GetName(mInputPorts[i].mNameID).c_str(), name, true /* case sensitive */))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // add an output port and return its index
    uint32 AnimGraphNode::AddOutputPort()
    {
        mOutputPorts.AddEmpty();
        return mOutputPorts.GetLength() - 1;
    }


    // add an input port, and return its index
    uint32 AnimGraphNode::AddInputPort()
    {
        mInputPorts.AddEmpty();
        return mInputPorts.GetLength() - 1;
    }


    // setup a port name
    void AnimGraphNode::SetInputPortName(uint32 portIndex, const char* name)
    {
        MCORE_ASSERT(portIndex < mInputPorts.GetLength());
        mInputPorts[portIndex].mNameID = MCore::GetStringIdPool().GenerateIdForString(name);
    }


    // setup a port name
    void AnimGraphNode::SetOutputPortName(uint32 portIndex, const char* name)
    {
        MCORE_ASSERT(portIndex < mOutputPorts.GetLength());
        mOutputPorts[portIndex].mNameID = MCore::GetStringIdPool().GenerateIdForString(name);
    }


    // get the total number of children
    uint32 AnimGraphNode::RecursiveCalcNumNodes() const
    {
        uint32 result = 0;
        for (const AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveCountChildNodes(result);
        }

        return result;
    }


    // recursively count the number of nodes down the hierarchy
    void AnimGraphNode::RecursiveCountChildNodes(uint32& numNodes) const
    {
        // increase the counter
        numNodes++;

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveCountChildNodes(numNodes);
        }
    }


    // recursively calculate the number of node connections
    uint32 AnimGraphNode::RecursiveCalcNumNodeConnections() const
    {
        uint32 result = 0;
        RecursiveCountNodeConnections(result);
        return result;
    }


    // recursively calculate the number of node connections
    void AnimGraphNode::RecursiveCountNodeConnections(uint32& numConnections) const
    {
        // add the connections to our counter
        numConnections += GetNumConnections();

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveCountNodeConnections(numConnections);
        }
    }


    // setup an output port to output a given local pose
    void AnimGraphNode::SetupOutputPortAsPose(const char* name, uint32 outputPortNr, uint32 portID)
    {
        // check if we already registered this port ID
        const uint32 duplicatePort = FindOutputPortByID(portID);
        if (duplicatePort != MCORE_INVALIDINDEX32)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetOutputPortAsPose() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, MCore::GetStringIdPool().GetName(mOutputPorts[duplicatePort].mNameID).c_str(), name, RTTI_GetTypeName());
        }

        SetOutputPortName(outputPortNr, name);
        mOutputPorts[outputPortNr].Clear();
        mOutputPorts[outputPortNr].mCompatibleTypes[0] = AttributePose::TYPE_ID;    // setup the compatible types of this port
        mOutputPorts[outputPortNr].mPortID = portID;
    }


    // setup an output port to output a given motion instance
    void AnimGraphNode::SetupOutputPortAsMotionInstance(const char* name, uint32 outputPortNr, uint32 portID)
    {
        // check if we already registered this port ID
        const uint32 duplicatePort = FindOutputPortByID(portID);
        if (duplicatePort != MCORE_INVALIDINDEX32)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetOutputPortAsMotionInstance() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, MCore::GetStringIdPool().GetName(mOutputPorts[duplicatePort].mNameID).c_str(), name, RTTI_GetTypeName());
        }

        SetOutputPortName(outputPortNr, name);
        mOutputPorts[outputPortNr].Clear();
        mOutputPorts[outputPortNr].mCompatibleTypes[0] = AttributeMotionInstance::TYPE_ID;  // setup the compatible types of this port
        mOutputPorts[outputPortNr].mPortID = portID;
    }


    // setup an output port
    void AnimGraphNode::SetupOutputPort(const char* name, uint32 outputPortNr, uint32 attributeTypeID, uint32 portID)
    {
        // check if we already registered this port ID
        const uint32 duplicatePort = FindOutputPortByID(portID);
        if (duplicatePort != MCORE_INVALIDINDEX32)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetOutputPort() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' name='%s')", portID, MCore::GetStringIdPool().GetName(mOutputPorts[duplicatePort].mNameID).c_str(), name, RTTI_GetTypeName());
        }

        SetOutputPortName(outputPortNr, name);
        mOutputPorts[outputPortNr].Clear();
        //mOutputPorts[outputPortNr].mValue = MCore::GetAttributePool().RequestNew( attributeTypeID );
        mOutputPorts[outputPortNr].mCompatibleTypes[0] = attributeTypeID;
        mOutputPorts[outputPortNr].mPortID = portID;
    }


    // setup an input port as a number (float/int/bool)
    void AnimGraphNode::SetupInputPortAsNumber(const char* name, uint32 inputPortNr, uint32 portID)
    {
        // check if we already registered this port ID
        const uint32 duplicatePort = FindInputPortByID(portID);
        if (duplicatePort != MCORE_INVALIDINDEX32)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetInputPortAsNumber() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, MCore::GetStringIdPool().GetName(mInputPorts[duplicatePort].mNameID).c_str(), name, RTTI_GetTypeName());
        }

        SetInputPortName(inputPortNr, name);
        mInputPorts[inputPortNr].Clear();
        mInputPorts[inputPortNr].mCompatibleTypes[0] = MCore::AttributeFloat::TYPE_ID;
        //mInputPorts[inputPortNr].mCompatibleTypes[1] = MCore::AttributeInt32::TYPE_ID;
        //mInputPorts[inputPortNr].mCompatibleTypes[2] = MCore::AttributeBool::TYPE_ID;;
        mInputPorts[inputPortNr].mPortID = portID;
    }


    // setup a given input port in a generic way
    void AnimGraphNode::SetupInputPort(const char* name, uint32 inputPortNr, uint32 attributeTypeID, uint32 portID)
    {
        // check if we already registered this port ID
        const uint32 duplicatePort = FindInputPortByID(portID);
        if (duplicatePort != MCORE_INVALIDINDEX32)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetInputPort() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, MCore::GetStringIdPool().GetName(mInputPorts[duplicatePort].mNameID).c_str(), name, RTTI_GetTypeName());
        }

        SetInputPortName(inputPortNr, name);
        mInputPorts[inputPortNr].Clear();
        mInputPorts[inputPortNr].mCompatibleTypes[0] = attributeTypeID;
        mInputPorts[inputPortNr].mPortID = portID;

        // make sure we were able to create the attribute
        //MCORE_ASSERT( mInputPorts[inputPortNr].mValue );
    }


    void AnimGraphNode::RecursiveResetUniqueData(AnimGraphInstance* animGraphInstance)
    {
        ResetUniqueData(animGraphInstance);

        for (AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveResetUniqueData(animGraphInstance);
        }
    }


    void AnimGraphNode::RecursiveOnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // some attributes have changed
        OnUpdateUniqueData(animGraphInstance);

        for (AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveOnUpdateUniqueData(animGraphInstance);
        }
    }


    // get the input value for a given port
    const MCore::Attribute* AnimGraphNode::GetInputValue(AnimGraphInstance* animGraphInstance, uint32 inputPort) const
    {
        MCORE_UNUSED(animGraphInstance);

        // get the connection that is plugged into the port
        BlendTreeConnection* connection = mInputPorts[inputPort].mConnection;
        MCORE_ASSERT(connection); // make sure there is a connection plugged in, otherwise we can't read the value

        // get the value from the output port of the source node
        return connection->GetSourceNode()->GetOutputValue(animGraphInstance, connection->GetSourcePort());
    }


    // recursively reset the output is ready flag
    void AnimGraphNode::RecursiveResetFlags(AnimGraphInstance* animGraphInstance, uint32 flagsToReset)
    {
        // reset the flag in this node
        animGraphInstance->DisableObjectFlags(mObjectIndex, flagsToReset);

    #ifdef EMFX_EMSTUDIOBUILD
        // reset all connections
        for (BlendTreeConnection* connection : mConnections)
        {
            connection->SetIsVisited(false);
        }
    #endif

        for (AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveResetFlags(animGraphInstance, flagsToReset);
        }
    }


    // sync the current time with another node
    void AnimGraphNode::SyncPlayTime(AnimGraphInstance* animGraphInstance, AnimGraphNode* masterNode)
    {
        const float masterDuration = masterNode->GetDuration(animGraphInstance);
        const float normalizedTime = (masterDuration > MCore::Math::epsilon) ? masterNode->GetCurrentPlayTime(animGraphInstance) / masterDuration : 0.0f;
        SetCurrentPlayTimeNormalized(animGraphInstance, normalizedTime);
    }


    // automatically sync
    void AnimGraphNode::AutoSync(AnimGraphInstance* animGraphInstance, AnimGraphNode* masterNode, float weight, ESyncMode syncMode, bool resync, bool modifyMasterSpeed)
    {
        // exit if we don't want to sync or we have no master node to sync to
        if (syncMode == SYNCMODE_DISABLED || masterNode == nullptr)
        {
            return;
        }

        // if one of the tracks is empty, sync the full clip
        if (syncMode == SYNCMODE_TRACKBASED)
        {
            // get the sync tracks
            AnimGraphSyncTrack& syncTrackA = masterNode->FindUniqueNodeData(animGraphInstance)->GetSyncTrack();
            AnimGraphSyncTrack& syncTrackB = FindUniqueNodeData(animGraphInstance)->GetSyncTrack();

            // if we have sync keys in both nodes, do the track based sync
            if (syncTrackA.GetNumEvents() > 0 && syncTrackB.GetNumEvents() > 0)
            {
                SyncUsingSyncTracks(animGraphInstance, masterNode, &syncTrackA, &syncTrackB, weight, resync, modifyMasterSpeed);
                return;
            }
        }

        // we either have no evens inside the sync tracks in both nodes, or we just want to sync based on full clips
        SyncFullNode(animGraphInstance, masterNode, weight, modifyMasterSpeed);
    }


    void AnimGraphNode::SyncFullNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* masterNode, float weight, bool modifyMasterSpeed)
    {
        SyncPlaySpeeds(animGraphInstance, masterNode, weight, modifyMasterSpeed);
        SyncPlayTime(animGraphInstance, masterNode);
    }


    // set the normalized time
    void AnimGraphNode::SetCurrentPlayTimeNormalized(AnimGraphInstance* animGraphInstance, float normalizedTime)
    {
        const float duration = GetDuration(animGraphInstance);
        SetCurrentPlayTime(animGraphInstance, normalizedTime * duration);
    }


    // sync blend the play speed of two nodes
    void AnimGraphNode::SyncPlaySpeeds(AnimGraphInstance* animGraphInstance, AnimGraphNode* masterNode, float weight, bool modifyMasterSpeed)
    {
        AnimGraphNodeData* uniqueDataA = masterNode->FindUniqueNodeData(animGraphInstance);
        AnimGraphNodeData* uniqueDataB = FindUniqueNodeData(animGraphInstance);

        const float durationA   = uniqueDataA->GetDuration();
        const float durationB   = uniqueDataB->GetDuration();
        const float timeRatio   = (durationB > MCore::Math::epsilon) ? durationA / durationB : 0.0f;
        const float timeRatio2  = (durationA > MCore::Math::epsilon) ? durationB / durationA : 0.0f;
        const float factorA     = MCore::LinearInterpolate<float>(1.0f, timeRatio, weight);
        const float factorB     = MCore::LinearInterpolate<float>(timeRatio2, 1.0f, weight);
        const float interpolatedSpeed   = MCore::LinearInterpolate<float>(uniqueDataA->GetPlaySpeed(), uniqueDataB->GetPlaySpeed(), weight);

        if (modifyMasterSpeed)
        {
            uniqueDataA->SetPlaySpeed(interpolatedSpeed * factorA);
        }

        uniqueDataB->SetPlaySpeed(interpolatedSpeed * factorB);
    }


    // calculate the sync factors
    void AnimGraphNode::CalcSyncFactors(AnimGraphInstance* animGraphInstance, AnimGraphNode* masterNode, AnimGraphNode* slaveNode, ESyncMode syncMode, float weight, float* outMasterFactor, float* outSlaveFactor, float* outPlaySpeed)
    {
        // get the unique datas
        AnimGraphNodeData* uniqueDataA = masterNode->FindUniqueNodeData(animGraphInstance);
        AnimGraphNodeData* uniqueDataB = slaveNode->FindUniqueNodeData(animGraphInstance);

        // already output the play speed
        *outPlaySpeed = MCore::LinearInterpolate<float>(uniqueDataA->GetPlaySpeed(), uniqueDataB->GetPlaySpeed(), weight);

        // exit if we don't want to sync or we have no master node to sync to
        if (syncMode == SYNCMODE_DISABLED)
        {
            *outMasterFactor = 1.0f;
            *outSlaveFactor  = 1.0f;
            return;
        }

        // if one of the tracks is empty, sync the full clip
        if (syncMode == SYNCMODE_TRACKBASED)
        {
            // get the sync tracks
            AnimGraphSyncTrack& syncTrackA = uniqueDataA->GetSyncTrack();
            AnimGraphSyncTrack& syncTrackB = uniqueDataB->GetSyncTrack();

            // if we have sync keys in both nodes, do the track based sync
            if (syncTrackA.GetNumEvents() > 0 && syncTrackB.GetNumEvents() > 0)
            {
                const uint32 syncIndexA = uniqueDataA->GetSyncIndex();
                const uint32 syncIndexB = uniqueDataB->GetSyncIndex();

                // if the sync indices are invalid, act like no syncing
                if (syncIndexA == MCORE_INVALIDINDEX32 || syncIndexB == MCORE_INVALIDINDEX32)
                {
                    *outMasterFactor = 1.0f;
                    *outSlaveFactor  = 1.0f;
                    return;
                }

                // get the segment lengths
                // TODO: handle motion clip start and end
                uint32 syncIndexA2 = syncIndexA + 1;
                if (syncIndexA2 >= syncTrackA.GetNumEvents())
                {
                    syncIndexA2 = 0;
                }

                uint32 syncIndexB2 = syncIndexB + 1;
                if (syncIndexB2 >= syncTrackB.GetNumEvents())
                {
                    syncIndexB2 = 0;
                }

                const float durationA = syncTrackA.CalcSegmentLength(syncIndexA, syncIndexA2);
                const float durationB = syncTrackB.CalcSegmentLength(syncIndexB, syncIndexB2);
                const float timeRatio   = (durationB > MCore::Math::epsilon) ? durationA / durationB : 0.0f;
                const float timeRatio2  = (durationA > MCore::Math::epsilon) ? durationB / durationA : 0.0f;
                *outMasterFactor = MCore::LinearInterpolate<float>(1.0f, timeRatio, weight);
                *outSlaveFactor = MCore::LinearInterpolate<float>(timeRatio2, 1.0f, weight);
                return;
            }
        }

        // calculate the factor based on full clip sync
        const float durationA   = uniqueDataA->GetDuration();
        const float durationB   = uniqueDataB->GetDuration();
        const float timeRatio   = (durationB > MCore::Math::epsilon) ? durationA / durationB : 0.0f;
        const float timeRatio2  = (durationA > MCore::Math::epsilon) ? durationB / durationA : 0.0f;
        *outMasterFactor = MCore::LinearInterpolate<float>(1.0f, timeRatio, weight);
        *outSlaveFactor = MCore::LinearInterpolate<float>(timeRatio2, 1.0f, weight);
    }


    // recursively call the on change motion set callback function
    void AnimGraphNode::RecursiveOnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet)
    {
        // callback call
        OnChangeMotionSet(animGraphInstance, newMotionSet);

        // get the number of child nodes, iterate through them and recursively call this function
        const uint32 numChildNodes = GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            mChildNodes[i]->RecursiveOnChangeMotionSet(animGraphInstance, newMotionSet);
        }
    }


    // perform syncing using the sync tracks
    void AnimGraphNode::SyncUsingSyncTracks(AnimGraphInstance* animGraphInstance, AnimGraphNode* syncWithNode, AnimGraphSyncTrack* syncTrackA, AnimGraphSyncTrack* syncTrackB, float weight, bool resync, bool modifyMasterSpeed)
    {
        AnimGraphNode* nodeA = syncWithNode;
        AnimGraphNode* nodeB = this;

        // first sync the speeds
        //  SyncPlaySpeeds( animGraphInstance, nodeA, weight, modifyMasterSpeed);

        AnimGraphNodeData* uniqueDataA = nodeA->FindUniqueNodeData(animGraphInstance);
        AnimGraphNodeData* uniqueDataB = nodeB->FindUniqueNodeData(animGraphInstance);

        // get the time of motion A
        const float currentTime = uniqueDataA->GetCurrentPlayTime();
        const bool forward = !uniqueDataA->GetIsBackwardPlaying();

        // get the event indices
        uint32 firstIndexA;
        uint32 firstIndexB;
        if (syncTrackA->FindEventIndices(currentTime, &firstIndexA, &firstIndexB) == false)
        {
            //MCORE_ASSERT(false);
            //MCore::LogInfo("FAILED FindEventIndices at time %f", currentTime);
            return;
        }

        // if the main motion changed event, we must make sure we also change it
        if (uniqueDataA->GetSyncIndex() != firstIndexA)
        {
            animGraphInstance->EnableObjectFlags(nodeA->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCINDEX_CHANGED);
        }

        uint32 startEventIndex = uniqueDataB->GetSyncIndex();
        if (animGraphInstance->GetIsObjectFlagEnabled(nodeA->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCINDEX_CHANGED))
        {
            if (forward)
            {
                startEventIndex++;
            }
            else
            {
                startEventIndex--;
            }

            if (startEventIndex >= syncTrackB->GetNumEvents())
            {
                startEventIndex = 0;
            }

            if (startEventIndex == MCORE_INVALIDINDEX32)
            {
                startEventIndex = syncTrackB->GetNumEvents() - 1;
            }

            animGraphInstance->EnableObjectFlags(nodeB->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCINDEX_CHANGED);
        }

        // find the matching indices in the second track
        uint32 secondIndexA;
        uint32 secondIndexB;
        if (resync == false)
        {
            if (syncTrackB->FindMatchingEvents(startEventIndex, syncTrackA->GetEvent(firstIndexA).mID, syncTrackA->GetEvent(firstIndexB).mID, &secondIndexA, &secondIndexB, forward) == false)
            {
                //MCORE_ASSERT(false);
                //MCore::LogInfo("FAILED FindMatchingEvents");
                return;
            }
        }
        else // resyncing is required
        {
            const uint32 occurrence = syncTrackA->CalcOccurrence(firstIndexA, firstIndexB);
            if (syncTrackB->ExtractOccurrence(occurrence, syncTrackA->GetEvent(firstIndexA).mID, syncTrackA->GetEvent(firstIndexB).mID, &secondIndexA, &secondIndexB) == false)
            {
                //MCORE_ASSERT(false);
                //MCore::LogInfo("FAILED ExtractOccurrence");
                return;
            }
        }

        // update the sync indices
        uniqueDataA->SetSyncIndex(firstIndexA);
        uniqueDataB->SetSyncIndex(secondIndexA);

        // calculate the segment lengths
        const float firstSegmentLength  = syncTrackA->CalcSegmentLength(firstIndexA, firstIndexB);
        const float secondSegmentLength = syncTrackB->CalcSegmentLength(secondIndexA, secondIndexB);

        //MCore::LogInfo("t=%f firstA=%d firstB=%d occ=%d secA=%d secB=%d", currentTime, firstIndexA, firstIndexB, occurrence, secondIndexA, secondIndexB);

        // calculate the normalized offset inside the segment
        float normalizedOffset;
        if (firstIndexA < firstIndexB) // normal case
        {
            normalizedOffset = (firstSegmentLength > MCore::Math::epsilon) ? (currentTime - syncTrackA->GetEvent(firstIndexA).mTime) / firstSegmentLength : 0.0f;
        }
        else // looping case
        {
            float timeOffset;
            if (currentTime > syncTrackA->GetEvent(0).mTime)
            {
                timeOffset = currentTime - syncTrackA->GetEvent(firstIndexA).mTime;
            }
            else
            {
                timeOffset = (uniqueDataA->GetDuration() - syncTrackA->GetEvent(firstIndexA).mTime) + currentTime;
            }

            normalizedOffset = (firstSegmentLength > MCore::Math::epsilon) ? timeOffset / firstSegmentLength : 0.0f;
        }

        // get the durations of both nodes for later on
        const float durationA   = firstSegmentLength;
        const float durationB   = secondSegmentLength;

        // calculate the new time in the motion
        //  bool looped = false;
        float newTimeB;
        if (secondIndexA < secondIndexB) // if the second segment is a non-wrapping one, so a regular non-looping case
        {
            newTimeB = syncTrackB->GetEvent(secondIndexA).mTime + secondSegmentLength * normalizedOffset;
        }
        else // looping case
        {
            // calculate the new play time
            const float unwrappedTime = syncTrackB->GetEvent(secondIndexA).mTime + secondSegmentLength * normalizedOffset;

            // if it is past the motion duration, we need to wrap around
            if (unwrappedTime > uniqueDataB->GetDuration())
            {
                // the new wrapped time
                newTimeB = MCore::Math::SafeFMod(unwrappedTime, uniqueDataB->GetDuration()); // TODO: doesn't take into account the motion start and end clip times

                // check if we looped
                //if (uniqueDataB->GetPreSyncTime() > syncTrackB->GetEvent(syncTrackB->GetNumEvents()-1).mTime)
                //looped = true;
            }
            else
            {
                newTimeB = unwrappedTime;
            }
        }

        // adjust the play speeds
        nodeB->SetCurrentPlayTime(animGraphInstance, newTimeB);
        const float timeRatio       = (durationB > MCore::Math::epsilon) ? durationA / durationB : 0.0f;
        const float timeRatio2      = (durationA > MCore::Math::epsilon) ? durationB / durationA : 0.0f;
        const float factorA         = MCore::LinearInterpolate<float>(1.0f, timeRatio, weight);
        const float factorB         = MCore::LinearInterpolate<float>(timeRatio2, 1.0f, weight);
        const float interpolatedSpeed   = MCore::LinearInterpolate<float>(uniqueDataA->GetPlaySpeed(), uniqueDataB->GetPlaySpeed(), weight);

        if (modifyMasterSpeed)
        {
            uniqueDataA->SetPlaySpeed(interpolatedSpeed * factorA);
        }

        uniqueDataB->SetPlaySpeed(interpolatedSpeed * factorB);
    }


    // check if the given node is the parent or the parent of the parent etc. of the node
    bool AnimGraphNode::RecursiveIsParentNode(AnimGraphNode* node) const
    {
        // if we're dealing with a root node we can directly return failure
        if (!mParentNode)
        {
            return false;
        }

        // check if the parent is the node and return success in that case
        if (mParentNode == node)
        {
            return true;
        }

        // check if the parent's parent is the node we're searching for
        return mParentNode->RecursiveIsParentNode(node);
    }


    // check if the given node is a child or a child of a chid etc. of the node
    bool AnimGraphNode::RecursiveIsChildNode(AnimGraphNode* node) const
    {
        // check if the given node is a child node of the current node
        if (FindChildNodeIndex(node) != MCORE_INVALIDINDEX32)
        {
            return true;
        }

        // get the number of child nodes, iterate through them and compare if the node is a child of the child nodes of this node
        for (const AnimGraphNode* childNode : mChildNodes)
        {
            if (childNode->RecursiveIsChildNode(node))
            {
                return true;
            }
        }

        // failure, the node isn't a child or a child of a child node
        return false;
    }


#ifdef EMFX_EMSTUDIOBUILD
    void AnimGraphNode::SetHasError(AnimGraphInstance* animGraphInstance, bool hasError)
    {
        // check if the anim graph instance is valid
        //if (animGraphInstance == nullptr)
        //return;

        // nothing to change, return directly, only update when something changed
        if (GetHasErrorFlag(animGraphInstance) == hasError)
        {
            return;
        }

        // update the flag
        SetHasErrorFlag(animGraphInstance, hasError);

        // sync the current node
        GetEventManager().Sync(animGraphInstance, this);

        // in case the parent node is valid check the error status of the parent by checking all children recursively and set that value
        if (mParentNode)
        {
            mParentNode->SetHasError(animGraphInstance, mParentNode->HierarchicalHasError(animGraphInstance, true));
        }
    }


    bool AnimGraphNode::HierarchicalHasError(AnimGraphInstance* animGraphInstance, bool onlyCheckChildNodes) const
    {
        // check if the anim graph instance is valid
        //if (animGraphInstance == nullptr)
        //return true;

        if (onlyCheckChildNodes == false && GetHasErrorFlag(animGraphInstance))
        {
            return true;
        }

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            if (childNode->GetHasErrorFlag(animGraphInstance))
            {
                return true;
            }
        }

        // no erroneous node found
        return false;
    }
#endif


    // collect child nodes of the given type
    void AnimGraphNode::CollectChildNodesOfType(const AZ::TypeId& nodeType, MCore::Array<AnimGraphNode*>* outNodes) const
    {
        for (AnimGraphNode* childNode : mChildNodes)
        {
            // check the current node type and add it to the output array in case they are the same
            if (azrtti_typeid(childNode) == nodeType)
            {
                outNodes->Add(childNode);
            }
        }
    }


    void AnimGraphNode::CollectChildNodesOfType(const AZ::TypeId& nodeType, AZStd::vector<AnimGraphNode*>& outNodes) const
    {
        for (AnimGraphNode* childNode : mChildNodes)
        {
            if (azrtti_typeid(childNode) == nodeType)
            {
                outNodes.push_back(childNode);
            }
        }
    }


    // recursively collect nodes of the given type
    void AnimGraphNode::RecursiveCollectNodesOfType(const AZ::TypeId& nodeType, MCore::Array<AnimGraphNode*>* outNodes) const
    {
        // check the current node type
        if (nodeType == azrtti_typeid(this))
        {
            outNodes->Add(const_cast<AnimGraphNode*>(this));
        }

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveCollectNodesOfType(nodeType, outNodes);
        }
    }


    // get the transition conditions of a given type, recursively
    void AnimGraphNode::RecursiveCollectTransitionConditionsOfType(const AZ::TypeId& conditionType, MCore::Array<AnimGraphTransitionCondition*>* outConditions) const
    {
        // check if the current node is a state machine
        if (azrtti_typeid(this) == azrtti_typeid<AnimGraphStateMachine>())
        {
            // type cast the node to a state machine
            const AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(const_cast<AnimGraphNode*>(this));

            // get the number of transitions and iterate through them
            const uint32 numTransitions = stateMachine->GetNumTransitions();
            for (uint32 i = 0; i < numTransitions; ++i)
            {
                const AnimGraphStateTransition* transition = stateMachine->GetTransition(i);

                // get the number of conditions and iterate through them
                const size_t numConditions = transition->GetNumConditions();
                for (size_t j = 0; j < numConditions; ++j)
                {
                    // check if the given condition is of the given type and add it to the output array in this case
                    AnimGraphTransitionCondition* condition = transition->GetCondition(j);
                    if (azrtti_typeid(condition) == conditionType)
                    {
                        outConditions->Add(condition);
                    }
                }
            }
        }

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveCollectTransitionConditionsOfType(conditionType, outConditions);
        }
    }


    // find the input port, based on the port name
    AnimGraphNode::Port* AnimGraphNode::FindInputPortByName(const char* portName)
    {
        // for all ports
        const uint32 numPorts = mInputPorts.GetLength();
        for (uint32 i = 0; i < numPorts; ++i)
        {
            if (mInputPorts[i].GetNameString() == portName)
            {
                return &(mInputPorts[i]);
            }
        }

        return nullptr;
    }


    // find the output port, based on the port name
    AnimGraphNode::Port* AnimGraphNode::FindOutputPortByName(const char* portName)
    {
        // for all ports
        const uint32 numPorts = mOutputPorts.GetLength();
        for (uint32 i = 0; i < numPorts; ++i)
        {
            if (mOutputPorts[i].GetNameString() == portName)
            {
                return &(mOutputPorts[i]);
            }
        }

        return nullptr;
    }


    // find the input port index, based on the port id
    uint32 AnimGraphNode::FindInputPortByID(uint32 portID) const
    {
        // for all ports
        const uint32 numPorts = mInputPorts.GetLength();
        for (uint32 i = 0; i < numPorts; ++i)
        {
            if (mInputPorts[i].mPortID == portID)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find the output port index, based on the port id
    uint32 AnimGraphNode::FindOutputPortByID(uint32 portID) const
    {
        // for all ports
        const uint32 numPorts = mOutputPorts.GetLength();
        for (uint32 i = 0; i < numPorts; ++i)
        {
            if (mOutputPorts[i].mPortID == portID)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // this function will get called to rewind motion nodes as well as states etc. to reset several settings when a state gets exited
    void AnimGraphNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        // on default only reset the current time back to the start
        SetCurrentPlayTimeNormalized(animGraphInstance, 0.0f);
    }


    // collect all outgoing connections
    void AnimGraphNode::CollectOutgoingConnections(AZStd::vector<BlendTreeConnection*>& outConnections, AZStd::vector<AnimGraphNode*>& outTargetNodes) const
    {
        outConnections.clear();
        outTargetNodes.clear();

        // if we don't have a parent node we cannot proceed
        if (!mParentNode)
        {
            return;
        }

        // get the number of child nodes and iterate through them
        const uint32 numNodes = mParentNode->GetNumChildNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // get the current child node and skip it if it is the same as this node
            AnimGraphNode* childNode = mParentNode->GetChildNode(i);
            if (childNode == this)
            {
                continue;
            }

            // get the number of connections and iterate through them
            const uint32 numConnections = childNode->GetNumConnections();
            for (uint32 j = 0; j < numConnections; ++j)
            {
                // check if the connection comes from this node, if so add it to our output array
                BlendTreeConnection* connection = childNode->GetConnection(j);
                if (connection->GetSourceNode() == this)
                {
                    outConnections.emplace_back(connection);
                    outTargetNodes.emplace_back(childNode);
                }
            }
        }
    }


    // find and return the connection connected to the given port
    BlendTreeConnection* AnimGraphNode::FindConnection(uint16 port) const
    {
        // get the number of connections and iterate through them
        const uint32 numConnections = GetNumConnections();
        for (uint32 i = 0; i < numConnections; ++i)
        {
            // get the current connection and check if the connection is connected to the given port
            BlendTreeConnection* connection = GetConnection(i);
            if (connection->GetTargetPort() == port)
            {
                return connection;
            }
        }

        // failure, there is no connection connected to the given port
        return nullptr;
    }


    // callback that gets called before a node gets removed
    void AnimGraphNode::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        for (AnimGraphNode* childNode : mChildNodes)
        {
            childNode->OnRemoveNode(animGraph, nodeToRemove);
        }
    }


    // collect internal objects
    void AnimGraphNode::RecursiveCollectObjects(MCore::Array<AnimGraphObject*>& outObjects) const
    {
        outObjects.Add(const_cast<AnimGraphNode*>(this));

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveCollectObjects(outObjects);
        }
    }


    // topdown update
    void AnimGraphNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        //if (mDisabled)
        //return;

        // get the unique data
        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
        HierarchicalSyncAllInputNodes(animGraphInstance, uniqueData);

        // top down update all incoming connections
        for (BlendTreeConnection* connection : mConnections)
        {
            connection->GetSourceNode()->TopDownUpdate(animGraphInstance, timePassedInSeconds);
        }
    }

    // default update implementation
    void AnimGraphNode::Output(AnimGraphInstance* animGraphInstance)
    {
        OutputAllIncomingNodes(animGraphInstance);
    }


    // default update implementation
    void AnimGraphNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // get the unique data
        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);

        // iterate over all incoming connections
        bool syncTrackFound = false;
        size_t connectionIndex = MCORE_INVALIDINDEX32;
        const size_t numConnections = mConnections.size();
        for (size_t i = 0; i < numConnections; ++i)
        {
            const BlendTreeConnection* connection = mConnections[i];
            AnimGraphNode* sourceNode = connection->GetSourceNode();

            // update the node
            UpdateIncomingNode(animGraphInstance, sourceNode, timePassedInSeconds);

            // use the sync track of the first input port of this node
            if (connection->GetTargetPort() == 0 && sourceNode->GetHasOutputPose())
            {
                syncTrackFound = true;
                connectionIndex = i;
            }
        }

        if (connectionIndex != MCORE_INVALIDINDEX32)
        {
            uniqueData->Init(animGraphInstance, mConnections[connectionIndex]->GetSourceNode());
        }

        // set the current sync track to the first input connection
        if (!syncTrackFound && numConnections > 0 && mConnections[0]->GetSourceNode()->GetHasOutputPose()) // just pick the first connection's sync track
        {
            uniqueData->Init(animGraphInstance, mConnections[0]->GetSourceNode());
        }
    }


    // output all incoming nodes
    void AnimGraphNode::OutputAllIncomingNodes(AnimGraphInstance* animGraphInstance)
    {
        for (const BlendTreeConnection* connection : mConnections)
        {
            OutputIncomingNode(animGraphInstance, connection->GetSourceNode());
        }
    }


    // update a specific node
    void AnimGraphNode::UpdateIncomingNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* node, float timePassedInSeconds)
    {
        node->PerformUpdate(animGraphInstance, timePassedInSeconds);
    }


    // update all incoming nodes
    void AnimGraphNode::UpdateAllIncomingNodes(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        for (const BlendTreeConnection* connection : mConnections)
        {
            AnimGraphNode* sourceNode = connection->GetSourceNode();
            sourceNode->PerformUpdate(animGraphInstance, timePassedInSeconds);
        }
    }


    // mark the connections going to a given node as visited
    void AnimGraphNode::MarkConnectionVisited(AnimGraphNode* sourceNode)
    {
    #ifdef EMFX_EMSTUDIOBUILD
        for (BlendTreeConnection* connection : mConnections)
        {
            if (connection->GetSourceNode() == sourceNode)
            {
                connection->SetIsVisited(true);
            }
        }
    #else
        MCORE_UNUSED(sourceNode);
    #endif
    }


    // output a node
    void AnimGraphNode::OutputIncomingNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeToOutput)
    {
        if (nodeToOutput == nullptr)
        {
            return;
        }

        // output the node
        nodeToOutput->PerformOutput(animGraphInstance);

        // mark any connection originating from this node as visited
    #ifdef EMFX_EMSTUDIOBUILD
        for (BlendTreeConnection* connection : mConnections)
        {
            if (connection->GetSourceNode() == nodeToOutput)
            {
                connection->SetIsVisited(true);
            }
        }
    #endif
    }



    // Process events and motion extraction delta.
    void AnimGraphNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // Post process all incoming nodes.
        bool poseFound = false;
        size_t connectionIndex = MCORE_INVALIDINDEX32;
        AZ::u16 minTargetPortIndex = MCORE_INVALIDINDEX16;
        const size_t numConnections = mConnections.size();
        for (size_t i = 0; i < numConnections; ++i)
        {
            const BlendTreeConnection* connection = mConnections[i];
            AnimGraphNode* sourceNode = connection->GetSourceNode();

            // update the node
            sourceNode->PerformPostUpdate(animGraphInstance, timePassedInSeconds);

            // If the input node has no pose, we can skip to the next connection.
            if (!sourceNode->GetHasOutputPose())
            {
                continue;
            }

            // Find the first connection that plugs into a port that can take a pose.
            const AZ::u16 targetPortIndex = connection->GetTargetPort();
            if (mInputPorts[targetPortIndex].mCompatibleTypes[0] == AttributePose::TYPE_ID)
            {
                poseFound = true;
                if (targetPortIndex < minTargetPortIndex)
                {
                    connectionIndex = i;
                    minTargetPortIndex = targetPortIndex;
                }
            }
        }

        // request the anim graph reference counted data objects
        RequestRefDatas(animGraphInstance);

        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
        if (poseFound && connectionIndex != MCORE_INVALIDINDEX32)
        {
            const BlendTreeConnection* connection = mConnections[connectionIndex];
            AnimGraphNode* sourceNode = connection->GetSourceNode();

            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            AnimGraphRefCountedData* sourceData = sourceNode->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();

            data->SetEventBuffer(sourceData->GetEventBuffer());
            data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
            data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
        }
        else
        if (poseFound == false && numConnections > 0 && mConnections[0]->GetSourceNode()->GetHasOutputPose())
        {
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            AnimGraphNode* sourceNode = mConnections[0]->GetSourceNode();
            AnimGraphRefCountedData* sourceData = sourceNode->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
            data->SetEventBuffer(sourceData->GetEventBuffer());
            data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
            data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
        }
        else
        {
            if (poseFound == false)
            {
                AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
                data->ClearEventBuffer();
                data->ZeroTrajectoryDelta();
            }
        }
    }



    // recursively set object data flag
    void AnimGraphNode::RecursiveSetUniqueDataFlag(AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled)
    {
        // set the flag
        animGraphInstance->SetObjectFlags(mObjectIndex, flag, enabled);

        // recurse downwards
        for (BlendTreeConnection* connection : mConnections)
        {
            connection->GetSourceNode()->RecursiveSetUniqueDataFlag(animGraphInstance, flag, enabled);
        }
    }


    // filter the event based on a given event mode
    void AnimGraphNode::FilterEvents(AnimGraphInstance* animGraphInstance, EEventMode eventMode, AnimGraphNode* nodeA, AnimGraphNode* nodeB, float localWeight, AnimGraphRefCountedData* refData)
    {
        switch (eventMode)
        {
        // only the master
        case EVENTMODE_MASTERONLY:
        {
            if (nodeA)
            {
                AnimGraphRefCountedData* refDataA = nodeA->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
                if (refDataA)
                {
                    refData->SetEventBuffer(refDataA->GetEventBuffer());
                }
            }
        }
        break;


        // only the slave
        case EVENTMODE_SLAVEONLY:
        {
            if (nodeB)
            {
                AnimGraphRefCountedData* refDataB = nodeB->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
                if (refDataB)
                {
                    refData->SetEventBuffer(refDataB->GetEventBuffer());
                }
            }
            else if (nodeA)
            {
                AnimGraphRefCountedData* refDataA = nodeA->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
                if (refDataA)
                {
                    refData->SetEventBuffer(refDataA->GetEventBuffer());   // master is also slave
                }
            }
        }
        break;


        // both nodes
        case EVENTMODE_BOTHNODES:
        {
            AnimGraphRefCountedData* refDataA = nodeA ? nodeA->FindUniqueNodeData(animGraphInstance)->GetRefCountedData() : nullptr;
            AnimGraphRefCountedData* refDataB = nodeB ? nodeB->FindUniqueNodeData(animGraphInstance)->GetRefCountedData() : nullptr;

            const uint32 numEventsA = refDataA ? refDataA->GetEventBuffer().GetNumEvents() : 0;
            const uint32 numEventsB = refDataB ? refDataB->GetEventBuffer().GetNumEvents() : 0;

            // resize to the right number of events already
            AnimGraphEventBuffer& eventBuffer = refData->GetEventBuffer();
            eventBuffer.Resize(numEventsA + numEventsB);

            // add the first node's events
            if (refDataA)
            {
                const AnimGraphEventBuffer& eventBufferA = refDataA->GetEventBuffer();
                for (uint32 i = 0; i < numEventsA; ++i)
                {
                    eventBuffer.SetEvent(i, eventBufferA.GetEvent(i));
                }
            }

            if (refDataB)
            {
                const AnimGraphEventBuffer& eventBufferB = refDataB->GetEventBuffer();
                for (uint32 i = 0; i < numEventsB; ++i)
                {
                    eventBuffer.SetEvent(numEventsA + i, eventBufferB.GetEvent(i));
                }
            }
        }
        break;


        // most active node's events
        case EVENTMODE_MOSTACTIVE:
        {
            // if the weight is lower than half, use the first node's events
            if (localWeight <= 0.5f)
            {
                if (nodeA)
                {
                    AnimGraphRefCountedData* refDataA = nodeA->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
                    if (refDataA)
                    {
                        refData->SetEventBuffer(refDataA->GetEventBuffer());
                    }
                }
            }
            else // try to use the second node's events
            {
                if (nodeB)
                {
                    AnimGraphRefCountedData* refDataB = nodeB->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
                    if (refDataB)
                    {
                        refData->SetEventBuffer(refDataB->GetEventBuffer());
                    }
                }
                else if (nodeA)
                {
                    AnimGraphRefCountedData* refDataA = nodeA->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
                    if (refDataA)
                    {
                        refData->SetEventBuffer(refDataA->GetEventBuffer());   // master is also slave
                    }
                }
            }
        }
        break;

        default:
            AZ_Assert(false, "Unknown event filter mode used!");
        };
    }


    // hierarchically sync input a given input node
    void AnimGraphNode::HierarchicalSyncInputNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* inputNode, AnimGraphNodeData* uniqueDataOfThisNode)
    {
        AnimGraphNodeData* inputUniqueData = inputNode->FindUniqueNodeData(animGraphInstance);

        if (animGraphInstance->GetIsSynced(inputNode->GetObjectIndex()))
        {
            inputNode->AutoSync(animGraphInstance, this, 0.0f, SYNCMODE_TRACKBASED, false, false);
        }
        else
        {
            inputUniqueData->SetPlaySpeed(uniqueDataOfThisNode->GetPlaySpeed()); // default child node speed propagation in case it is not synced
        }
        // pass the global weight along to the child nodes
        inputUniqueData->SetGlobalWeight(uniqueDataOfThisNode->GetGlobalWeight());
        inputUniqueData->SetLocalWeight(1.0f);
    }


    // hierarchically sync all input nodes
    void AnimGraphNode::HierarchicalSyncAllInputNodes(AnimGraphInstance* animGraphInstance, AnimGraphNodeData* uniqueDataOfThisNode)
    {
        // for all connections
        for (const BlendTreeConnection* connection : mConnections)
        {
            AnimGraphNode* inputNode = connection->GetSourceNode();
            HierarchicalSyncInputNode(animGraphInstance, inputNode, uniqueDataOfThisNode);
        }
    }


    // on default create a base class object
    void AnimGraphNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // try to find existing data
        AnimGraphNodeData* data = animGraphInstance->FindUniqueNodeData(this);
        if (data == nullptr) // doesn't exist
        {
            AnimGraphNodeData* newData = aznew AnimGraphNodeData(this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(newData);
        }
    }


    // recursively collect active animgraph nodes
    void AnimGraphNode::RecursiveCollectActiveNodes(AnimGraphInstance* animGraphInstance, MCore::Array<AnimGraphNode*>* outNodes, const AZ::TypeId& nodeType) const
    {
        // check and add this node
        if (azrtti_typeid(this) == nodeType || nodeType.IsNull())
        {
            if (animGraphInstance->GetIsOutputReady(mObjectIndex)) // if we processed this node
            {
                outNodes->Add(const_cast<AnimGraphNode*>(this));
            }
        }

        // process all child nodes (but only active ones)
        for (const AnimGraphNode* childNode : mChildNodes)
        {
            if (animGraphInstance->GetIsOutputReady(childNode->GetObjectIndex()))
            {
                childNode->RecursiveCollectActiveNodes(animGraphInstance, outNodes, nodeType);
            }
        }
    }


    // decrease the reference count
    void AnimGraphNode::DecreaseRef(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
        if (uniqueData->GetPoseRefCount() == 0)
        {
            //MCore::LogInfo("Node %s of type %s has 0 refcount (parent=%s)", GetName(), RTTI_GetTypeName(), GetParentNode()->GetName());
            return;
        }

        //MCORE_ASSERT( GetPoseRefCount() > 0 );
        uniqueData->DecreasePoseRefCount();
        if (uniqueData->GetPoseRefCount() > 0)
        {
            return;
        }

        //AnimGraphNodeData* uniqueData = animGraphInstance->FindUniqueNodeData(this);
        //MCore::LogInfo("AnimGraphNode::DecreaseRef() - Releasing poses for node %s", GetName());
        const uint32 threadIndex = animGraphInstance->GetActorInstance()->GetThreadIndex();

        AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(threadIndex)->GetPosePool();

        const uint32 numOutputs = GetNumOutputs();
        for (uint32 i = 0; i < numOutputs; ++i)
        {
            if (GetOutputPort(i).mCompatibleTypes[0] != AttributePose::TYPE_ID)
            {
                continue;
            }

            MCore::Attribute* attribute = GetOutputAttribute(animGraphInstance, i);
            MCORE_ASSERT(attribute->GetType() == AttributePose::TYPE_ID);

            AttributePose* poseAttribute = static_cast<AttributePose*>(attribute);
            AnimGraphPose* pose = poseAttribute->GetValue();
            if (pose)
            {
                posePool.FreePose(pose);
            }
            poseAttribute->SetValue(nullptr);
        }
    }


    // request poses from the pose cache for all output poses
    void AnimGraphNode::RequestPoses(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        const uint32 threadIndex = actorInstance->GetThreadIndex();

        AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(threadIndex)->GetPosePool();

        const uint32 numOutputs = GetNumOutputs();
        for (uint32 i = 0; i < numOutputs; ++i)
        {
            if (GetOutputPort(i).mCompatibleTypes[0] != AttributePose::TYPE_ID)
            {
                continue;
            }

            MCore::Attribute* attribute = GetOutputAttribute(animGraphInstance, i);
            MCORE_ASSERT(attribute->GetType() == AttributePose::TYPE_ID);

            AnimGraphPose* pose = posePool.RequestPose(actorInstance);
            AttributePose* poseAttribute = static_cast<AttributePose*>(attribute);
            //      MCORE_ASSERT(poseAttribute->GetValue() == nullptr);
            poseAttribute->SetValue(pose);
        }
    }


    // free all poses from all incoming nodes
    void AnimGraphNode::FreeIncomingPoses(AnimGraphInstance* animGraphInstance)
    {
        //MCore::LogInfo("Free incoming poses for node %s with parent %s", GetName(), (GetParentNode()) ? GetParentNode()->GetName() : "ROOT");
        const uint32 numInputPorts = mInputPorts.GetLength();
        for (uint32 i = 0; i < numInputPorts; ++i)
        {
            BlendTreeConnection* connection = mInputPorts[i].mConnection;
            if (connection == nullptr)
            {
                continue;
            }

            AnimGraphNode* sourceNode = connection->GetSourceNode();
            //if (sourceNode->GetOutputPort(connection->GetSourcePort()).mCompatibleTypes[0] == AttributePose::TYPE_ID)
            sourceNode->DecreaseRef(animGraphInstance);
        }
    }


    // free all poses from all incoming nodes
    void AnimGraphNode::FreeIncomingRefDatas(AnimGraphInstance* animGraphInstance)
    {
        const uint32 numInputPorts = mInputPorts.GetLength();
        for (uint32 i = 0; i < numInputPorts; ++i)
        {
            BlendTreeConnection* connection = mInputPorts[i].mConnection;
            if (connection == nullptr)
            {
                continue;
            }

            AnimGraphNode* sourceNode = connection->GetSourceNode();
            //MCore::LogInfo("*********** Free ref data of incoming %s", sourceNode->GetName());
            sourceNode->DecreaseRefDataRef(animGraphInstance);
        }
    }


    // request poses from the pose cache for all output poses
    void AnimGraphNode::RequestRefDatas(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        const uint32 threadIndex = actorInstance->GetThreadIndex();

        AnimGraphRefCountedDataPool& pool = GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool();
        AnimGraphRefCountedData* newData = pool.RequestNew();
        //MCORE_ASSERT( FindUniqueNodeData(animGraphInstance)->GetRefCountedData() == nullptr);
        //if (FindUniqueNodeData(animGraphInstance)->GetRefCountedData())
        //DebugBreak();

        FindUniqueNodeData(animGraphInstance)->SetRefCountedData(newData);
    }


    // decrease the reference count
    void AnimGraphNode::DecreaseRefDataRef(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
        //MCore::LogInfo("******************** %s has refcount %d", GetName(), uniqueData->GetRefDataRefCount());
        if (uniqueData->GetRefDataRefCount() == 0)
        {
            return;
        }

        uniqueData->DecreaseRefDataRefCount();
        if (uniqueData->GetRefDataRefCount() > 0)
        {
            return;
        }

        //MCore::LogInfo("******************** freeing %s", GetName());

        // free it
        const uint32 threadIndex = animGraphInstance->GetActorInstance()->GetThreadIndex();
        AnimGraphRefCountedDataPool& pool = GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool();
        if (uniqueData->GetRefCountedData())
        {
            pool.Free(uniqueData->GetRefCountedData());
            uniqueData->SetRefCountedData(nullptr);
        }
        /*
            // force free of the child nodes
            const uint32 numChildNodes = mChildNodes.GetLength();
            for (uint32 i=0; i<numChildNodes; ++i)
            {
                AnimGraphNodeData* childUniqueData = mChildNodes[i]->FindUniqueNodeData(animGraphInstance);
                if (childUniqueData->GetRefDataRefCount() > 0)
                {
                    if (childUniqueData->GetRefCountedData())
                    {
                        pool.Free( childUniqueData->GetRefCountedData() );
                        childUniqueData->SetRefCountedData( nullptr );
                    }

                    childUniqueData->SetRefDataRefCount(0);
                }
            }*/
    }



    // perform top down update
    void AnimGraphNode::PerformTopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // check if we already did update
        if (animGraphInstance->GetIsTopDownUpdateReady(mObjectIndex))
        {
            return;
        }

        TopDownUpdate(animGraphInstance, timePassedInSeconds);

        // mark as done
        animGraphInstance->EnableObjectFlags(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_TOPDOWNUPDATE_READY);
    }


    // perform post update
    void AnimGraphNode::PerformPostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // check if we already did update
        if (animGraphInstance->GetIsPostUpdateReady(mObjectIndex))
        {
            FreeIncomingRefDatas(animGraphInstance);
            return;
        }

        // perform the actual post update
        //MCore::LogInfo("PostUpdate on '%s' - refCount = %d", GetName(), FindUniqueNodeData(animGraphInstance)->GetRefDataRefCount());
        PostUpdate(animGraphInstance, timePassedInSeconds);

        // free the incoming refs
        FreeIncomingRefDatas(animGraphInstance);

        // mark as done
        animGraphInstance->EnableObjectFlags(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_POSTUPDATE_READY);
    }


    // perform an update
    void AnimGraphNode::PerformUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // check if we already did update
        if (animGraphInstance->GetIsUpdateReady(mObjectIndex))
        {
            return;
        }

        // increase ref count for incoming nodes
        IncreaseInputRefCounts(animGraphInstance);
        IncreaseInputRefDataRefCounts(animGraphInstance);

        // perform the actual node update
        //MCore::LogInfo("Update for node %s (refCount=%d)", GetName(), FindUniqueNodeData(animGraphInstance)->GetRefDataRefCount());
        Update(animGraphInstance, timePassedInSeconds);

        // mark as output
        animGraphInstance->EnableObjectFlags(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_UPDATE_READY);
    }


    //
    void AnimGraphNode::PerformOutput(AnimGraphInstance* animGraphInstance)
    {
        // check if we already did output
        if (animGraphInstance->GetIsOutputReady(mObjectIndex))
        {
            // now decrease ref counts of all input nodes as we do not need the poses of this input node anymore for this node
            // once the pose ref count of a node reaches zero it will automatically release the poses back to the pool so they can be reused again by others
            FreeIncomingPoses(animGraphInstance);

            return;
        }

        // perform the output
        Output(animGraphInstance);

        //if (HasOutputPose())
        //MCore::LogInfo("%s     used=%d/%d  ref=%d", GetName(), GetActorManager().GetScheduler()->GetPosePool(threadIndex).GetNumUsedPoses(), GetActorManager().GetScheduler()->GetPosePool(threadIndex).GetNumPoses(), FindUniqueNodeData(animGraphInstance)->GetPoseRefCount());

        // now decrease ref counts of all input nodes as we do not need the poses of this input node anymore for this node
        // once the pose ref count of a node reaches zero it will automatically release the poses back to the pool so they can be reused again by others
        FreeIncomingPoses(animGraphInstance);

        // mark as output
        animGraphInstance->EnableObjectFlags(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_OUTPUT_READY);
    }


    // increase input ref counts
    void AnimGraphNode::IncreaseInputRefDataRefCounts(AnimGraphInstance* animGraphInstance)
    {
        const uint32 numInputPorts = mInputPorts.GetLength();
        for (uint32 i = 0; i < numInputPorts; ++i)
        {
            BlendTreeConnection* connection = mInputPorts[i].mConnection;
            if (connection == nullptr)
            {
                continue;
            }

            AnimGraphNode* sourceNode = connection->GetSourceNode();
            sourceNode->IncreaseRefDataRefCount(animGraphInstance);
        }
    }


    // increase input ref counts
    void AnimGraphNode::IncreaseInputRefCounts(AnimGraphInstance* animGraphInstance)
    {
        const uint32 numInputPorts = mInputPorts.GetLength();
        for (uint32 i = 0; i < numInputPorts; ++i)
        {
            BlendTreeConnection* connection = mInputPorts[i].mConnection;
            if (connection == nullptr)
            {
                continue;
            }

            AnimGraphNode* sourceNode = connection->GetSourceNode();
            //if (sourceNode->GetOutputPort( connection->GetSourcePort() ).mCompatibleTypes[0] == AttributePose::TYPE_ID)
            {
                //AnimGraphNodeData* uniqueData = sourceNode->FindUniqueNodeData(animGraphInstance);
                sourceNode->IncreasePoseRefCount(animGraphInstance);
            }
        }
    }


    void AnimGraphNode::RelinkPortConnections()
    {
        // After deserializing, nodes hold an array of incoming connections. Each node port caches a pointer to its connection object which we need to link.
        for (BlendTreeConnection* connection : mConnections)
        {
            AnimGraphNode* sourceNode = connection->GetSourceNode();
            const AZ::u16 targetPortNr = connection->GetTargetPort();
            const AZ::u16 sourcePortNr = connection->GetSourcePort();

            if (sourceNode)
            {
                if (sourcePortNr < sourceNode->GetNumOutputs())
                {
                    sourceNode->GetOutputPort(sourcePortNr).mConnection = connection;
                }
                else
                {
                    AZ_Error("EMotionFX", false, "Can't link output port %i of '%s' with the connection going to %s at port %i.", sourcePortNr, sourceNode->GetName(), GetName(), targetPortNr);
                }
            }

            if (targetPortNr < GetNumInputs())
            {
                mInputPorts[targetPortNr].mConnection = connection;
            }
            else
            {
                AZ_Error("EMotionFX", false, "Can't link input port %i of '%s' with the connection coming from %s at port %i.", targetPortNr, GetName(), sourceNode->GetName(), sourcePortNr);
            }
        }
    }
    

    // do we have a child of a given type?
    bool AnimGraphNode::CheckIfHasChildOfType(const AZ::TypeId& nodeType) const
    {
        for (const AnimGraphNode* childNode : mChildNodes)
        {
            if (azrtti_typeid(childNode) == nodeType)
            {
                return true;
            }
        }

        return false;
    }


    // check if we can visualize
    bool AnimGraphNode::GetCanVisualize(AnimGraphInstance* animGraphInstance) const
    {
        return (mVisEnabled && animGraphInstance->GetVisualizationEnabled() && EMotionFX::GetRecorder().GetIsInPlayMode() == false);
    }


    // remove internal attributes in all anim graph instances
    void AnimGraphNode::RemoveInternalAttributesForAllInstances()
    {
        // for all output ports
        const uint32 numOutputPorts = GetNumOutputs();
        for (uint32 p = 0; p < numOutputPorts; ++p)
        {
            Port& port = GetOutputPort(p);
            const uint32 internalAttributeIndex = port.mAttributeIndex;
            if (internalAttributeIndex == MCORE_INVALIDINDEX32)
            {
                continue;
            }

            const size_t numInstances = mAnimGraph->GetNumAnimGraphInstances();
            for (size_t i = 0; i < numInstances; ++i)
            {
                AnimGraphInstance* animGraphInstance = mAnimGraph->GetAnimGraphInstance(i);
                animGraphInstance->RemoveInternalAttribute(internalAttributeIndex);
            }

            mAnimGraph->DecreaseInternalAttributeIndices(internalAttributeIndex);
            port.mAttributeIndex = MCORE_INVALIDINDEX32;
        }
    }


    // decrease values higher than a given param value
    void AnimGraphNode::DecreaseInternalAttributeIndices(uint32 decreaseEverythingHigherThan)
    {
        const uint32 numOutputPorts = GetNumOutputs();
        for (uint32 p = 0; p < numOutputPorts; ++p)
        {
            Port& port = GetOutputPort(p);
            if (port.mAttributeIndex > decreaseEverythingHigherThan && port.mAttributeIndex != MCORE_INVALIDINDEX32)
            {
                port.mAttributeIndex--;
            }
        }
    }


    // init the internal attributes
    void AnimGraphNode::InitInternalAttributes(AnimGraphInstance* animGraphInstance)
    {
        // for all output ports of this node
        const uint32 numOutputPorts = GetNumOutputs();
        for (uint32 p = 0; p < numOutputPorts; ++p)
        {
            AnimGraphNode::Port& port = GetOutputPort(p);
            MCore::Attribute* newAttribute = MCore::GetAttributePool().RequestNew(port.mCompatibleTypes[0]); // assume compatibility type 0 to be the attribute type ID
            port.mAttributeIndex = animGraphInstance->AddInternalAttribute(newAttribute);
        }
    }


    void* AnimGraphNode::GetCustomData() const
    {
        return mCustomData;
    }


    void AnimGraphNode::SetCustomData(void* dataPointer)
    {
        mCustomData = dataPointer;
    }


    void AnimGraphNode::SetNodeInfo(const char* info)
    {
        mNodeInfo = info;

        SyncVisualObject();
    }


    const char* AnimGraphNode::GetNodeInfo() const
    {
        return mNodeInfo.c_str();
    }


    const AZStd::string& AnimGraphNode::GetNodeInfoString() const
    {
        return mNodeInfo;
    }


    bool AnimGraphNode::GetIsEnabled() const
    {
        return (mDisabled == false);
    }


    void AnimGraphNode::SetIsEnabled(bool enabled)
    {
        mDisabled = !enabled;
    }


    bool AnimGraphNode::GetIsCollapsed() const
    {
        return mIsCollapsed;
    }


    void AnimGraphNode::SetIsCollapsed(bool collapsed)
    {
        mIsCollapsed = collapsed;
    }


    void AnimGraphNode::SetVisualizeColor(uint32 color)
    {
        mVisualizeColor = color;
    }


    uint32 AnimGraphNode::GetVisualizeColor() const
    {
        return mVisualizeColor;
    }


    void AnimGraphNode::SetVisualPos(int32 x, int32 y)
    {
        mPosX = x;
        mPosY = y;
    }


    int32 AnimGraphNode::GetVisualPosX() const
    {
        return mPosX;
    }


    int32 AnimGraphNode::GetVisualPosY() const
    {
        return mPosY;
    }


    bool AnimGraphNode::GetIsVisualizationEnabled() const
    {
        return mVisEnabled;
    }


    void AnimGraphNode::SetVisualization(bool enabled)
    {
        mVisEnabled = enabled;
    }


    void AnimGraphNode::AddChildNode(AnimGraphNode* node)
    {
        mChildNodes.push_back(node);
        node->SetParentNode(this);
    }


    void AnimGraphNode::ReserveChildNodes(uint32 numChildNodes)
    {
        mChildNodes.reserve(numChildNodes);
    }


    const char* AnimGraphNode::GetName() const
    {
        return m_name.c_str();
    }


    const AZStd::string& AnimGraphNode::GetNameString() const
    {
        return m_name;
    }


    void AnimGraphNode::SetName(const char* name)
    {
        m_name = name;
    }


    // change the ports of a node, with automatic connection updates/deletion
    void AnimGraphNode::GatherRequiredConnectionChangesInputPorts(const MCore::Array<Port>& newPorts, MCore::Array<BlendTreeConnection*>* outToRemoveConnections, MCore::Array<BlendTreeConnection*>* outChangedConnections)
    {
        // for all current ports, check if we still have one in the new ports list
        const uint32 numCurrentPorts = mInputPorts.GetLength();
        for (uint32 c = 0; c < numCurrentPorts; ++c)
        {
            const Port& curPort = mInputPorts[c];
            if (curPort.mConnection == nullptr)
            {
                continue;
            }

            bool foundCompatiblePort = false;
            uint32 compatibleIndex = MCORE_INVALIDINDEX32;

            // iterate over all new ports
            const uint32 numNewPorts = newPorts.GetLength();
            for (uint32 n = 0; n < numNewPorts && foundCompatiblePort == false; ++n)
            {
                const Port& newPort = newPorts[n];
                if (curPort.mNameID == newPort.mNameID && curPort.mCompatibleTypes[0] == newPort.mCompatibleTypes[0])
                {
                    foundCompatiblePort = true;
                    compatibleIndex     = n;
                }
            }

            // add the connection to the remove list
            if (foundCompatiblePort == false)
            {
                outToRemoveConnections->Add(curPort.mConnection);
            }
            else // the connection might need to be updated
            {
                // the port index changed, remap it
                if (curPort.mConnection->GetTargetPort() != compatibleIndex)
                {
                    outChangedConnections->Add(curPort.mConnection);
                }
            }
        }
    }


    // set the input port change function
    void AnimGraphNode::SetInputPortChangeFunction(const AZStd::function<void(AnimGraphNode* node, const MCore::Array<Port>& newPorts)>& func)
    {
        mInputPortChangeFunction = func;
    }


    // execute the input port change function
    void AnimGraphNode::ExecuteInputPortChangeFunction(const MCore::Array<Port>& newPorts)
    {
        if (mInputPortChangeFunction)
        {
            mInputPortChangeFunction(this, newPorts);
        }
    }


    void AnimGraphNode::SetInputPorts(const MCore::Array<AnimGraphNode::Port>& inputPorts)
    {
        mInputPorts = inputPorts;
    }


    // change the ports of a node, with automatic connection updates/deletion
    void AnimGraphNodeDefaultInputPortChangeFunction(AnimGraphNode* node, const MCore::Array<AnimGraphNode::Port>& newPorts)
    {
        const MCore::Array<EMotionFX::AnimGraphNode::Port> oldPorts = node->GetInputPorts();

        // figure out which connection changes are required
        MCore::Array<BlendTreeConnection*> toRemoveConnections;
        MCore::Array<BlendTreeConnection*> toUpdateConnections;
        node->GatherRequiredConnectionChangesInputPorts(newPorts, &toRemoveConnections, &toUpdateConnections);

        // if there is nothing to do, return
        if (toRemoveConnections.GetLength() == 0 && toUpdateConnections.GetLength() == 0)
        {
            return;
        }

        // remove connections
        for (uint32 i = 0; i < toRemoveConnections.GetLength(); ++i)
        {
            node->RemoveConnection(toRemoveConnections[i]);
        }

        // update connections by remapping them to the same port names in the new ports list
        for (uint32 i = 0; i < toUpdateConnections.GetLength(); ++i)
        {
            BlendTreeConnection* connection = toUpdateConnections[i];
            const uint32 orgPortNameID = node->GetInputPort(connection->GetTargetPort()).mNameID;

            bool found = false;
            for (uint32 p = 0; p < newPorts.GetLength() && found == false; ++p)
            {
                if (newPorts[p].mNameID == orgPortNameID)
                {
                    connection->SetTargetPort(static_cast<uint16>(p));
                    found = true;
                }
            }

            MCORE_ASSERT(found);
        }

        // update the input ports
        node->SetInputPorts(newPorts);

        // copy unchanged connection pointers
        for (uint32 i = 0; i < oldPorts.GetLength(); ++i)
        {
            if (oldPorts[i].mConnection)
            {
                if (toRemoveConnections.Contains(oldPorts[i].mConnection) == false && toUpdateConnections.Contains(oldPorts[i].mConnection) == false)
                {
                    node->GetInputPort(i).mConnection = oldPorts[i].mConnection;
                }
            }
        }

        // update pointers in the modified ports
        for (uint32 i = 0; i < oldPorts.GetLength(); ++i)
        {
            if (oldPorts[i].mConnection)
            {
                if (toUpdateConnections.Contains(oldPorts[i].mConnection))
                {
                    node->GetInputPort(oldPorts[i].mConnection->GetTargetPort()).mConnection = oldPorts[i].mConnection;
                }
            }
        }
    }


    void AnimGraphNode::GetAttributeStringForAffectedNodeIds(const AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::string& attributesString) const
    {
        // Default implementation is that the transition is not affected, therefore we don't do anything
        AZ_UNUSED(convertedIds);
        AZ_UNUSED(attributesString);
    }


    void AnimGraphNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphNode, AnimGraphObject>()
            ->Version(1)
            ->PersistentId([](const void* instance) -> AZ::u64 { return static_cast<const AnimGraphNode*>(instance)->GetId(); })
            ->Field("id", &AnimGraphNode::m_id)
            ->Field("name", &AnimGraphNode::m_name)
            ->Field("posX", &AnimGraphNode::mPosX)
            ->Field("posY", &AnimGraphNode::mPosY)
            ->Field("visualizeColor", &AnimGraphNode::mVisualizeColor)
            ->Field("isDisabled", &AnimGraphNode::mDisabled)
            ->Field("isCollapsed", &AnimGraphNode::mIsCollapsed)
            ->Field("isVisEnabled", &AnimGraphNode::mVisEnabled)
            ->Field("childNodes", &AnimGraphNode::mChildNodes)
            ->Field("connections", &AnimGraphNode::mConnections);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphNode>("AnimGraphNode", "Base anim graph node")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC("AnimGraphNodeName", 0x15120d7d), &AnimGraphNode::m_name, "Name", "Name of the node")
                ->Attribute(AZ_CRC("AnimGraph", 0x0d53d4b3), &AnimGraphNode::GetAnimGraph)
                ;
    }
} // namespace EMotionFX