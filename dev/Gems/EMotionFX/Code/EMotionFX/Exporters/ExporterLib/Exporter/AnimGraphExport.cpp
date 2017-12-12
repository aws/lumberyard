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

#include "Exporter.h"
#include <MCore/Source/AttributeSettings.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphGameControllerSettings.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/AnimGraphParameterGroup.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/Importer/AnimGraphFileFormat.h>


namespace ExporterLib
{
    // function prototypes
    uint32 GetAnimGraphNodeSize(EMotionFX::AnimGraphNode* animGraphNode, bool includeName);
    uint32 GetAnimGraphObjectSize(EMotionFX::AnimGraphObject* animGraphNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Parameters
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // get the chunk size for a anim graph param info
    uint32 GetAnimGraphParamInfoSize(MCore::AttributeSettings* parameter)
    {
        uint32 result = sizeof(EMotionFX::FileFormat::AnimGraph_ParameterInfo);

        result += GetStringChunkSize(parameter->GetName());
        result += GetStringChunkSize(parameter->GetInternalName());
        result += GetStringChunkSize(parameter->GetDescription());

        if (parameter->GetMinValue() && parameter->GetMaxValue())
        {
            result += parameter->GetMinValue()->GetStreamSize();
            result += parameter->GetMaxValue()->GetStreamSize();
        }

        result += parameter->GetDefaultValue()->GetStreamSize();

        // get the number of combo values and iterate through them
        const uint32 numComboValues = parameter->GetNumComboValues();
        for (uint32 i = 0; i < numComboValues; ++i)
        {
            result += GetStringChunkSize(parameter->GetComboValue(i));
        }

        return result;
    }


    // save the given anim graph parameter info
    void SaveAnimGraphParamInfo(MCore::Stream* file, MCore::AttributeSettings* parameter, MCore::Endian::EEndianType targetEndianType)
    {
        EMotionFX::FileFormat::AnimGraph_ParameterInfo paramInfoChunk;
        const uint32 numComboValues     = parameter->GetNumComboValues();
        paramInfoChunk.mNumComboValues  = numComboValues;
        paramInfoChunk.mInterfaceType   = parameter->GetInterfaceType();
        paramInfoChunk.mFlags           = parameter->GetFlags();
        paramInfoChunk.mHasMinMax       = (parameter->GetMinValue() == nullptr || parameter->GetMaxValue() == nullptr) ? 0 : 1;
        paramInfoChunk.mAttributeType   = parameter->GetDefaultValue()->GetType();

        // add it to the log file
        //LogDetailedInfo("    - Parameter: name='%s', interal='%s'", parameter->mName.AsChar(), parameter->mInternalName.AsChar());
        //LogDetailedInfo("        + Description: '%s'", parameter->mDescription.AsChar());
        //LogDetailedInfo("        + InterfaceType: %i", paramInfoChunk.mInterfaceType);
        //LogDetailedInfo("        + ComboValues: %i", paramInfoChunk.mNumComboValues);

        // endian conversion
        ConvertUnsignedInt(&paramInfoChunk.mNumComboValues, targetEndianType);
        ConvertUnsignedInt(&paramInfoChunk.mInterfaceType, targetEndianType);
        ConvertUnsignedInt(&paramInfoChunk.mAttributeType, targetEndianType);
        ConvertUnsignedShort(&paramInfoChunk.mFlags, targetEndianType);

        // save the chunk
        file->Write(&paramInfoChunk, sizeof(EMotionFX::FileFormat::AnimGraph_ParameterInfo));

        // followed by:
        SaveString(parameter->GetName(), file, targetEndianType);
        SaveString(parameter->GetInternalName(), file, targetEndianType);
        SaveString(parameter->GetDescription(), file, targetEndianType);

        if (paramInfoChunk.mHasMinMax == 1)
        {
            parameter->GetMinValue()->Write(file, targetEndianType);
            parameter->GetMaxValue()->Write(file, targetEndianType);
        }

        parameter->GetDefaultValue()->Write(file, targetEndianType);

        // save combo values
        for (uint32 i = 0; i < numComboValues; ++i)
        {
            SaveString(parameter->GetComboValue(i), file, targetEndianType);
        }
    }


    // save the anim graph parameters
    void SaveAnimGraphParameters(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, MCore::Endian::EEndianType targetEndianType)
    {
        uint32 i;

        // get the number of anim graph parameters and check if there are any to save, if not skip directly
        const uint32 numParameters = animGraph->GetNumParameters();
        if (numParameters <= 0)
        {
            return;
        }

        // save the chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID        = EMotionFX::FileFormat::ANIMGRAPH_CHUNK_PARAMETERS;
        chunkHeader.mVersion        = 1;

        // calculate the chunk size
        chunkHeader.mSizeInBytes    = sizeof(uint32);
        for (i = 0; i < numParameters; ++i)
        {
            chunkHeader.mSizeInBytes += GetAnimGraphParamInfoSize(animGraph->GetParameter(i));
        }

        // convert endian and save the chunk header to the file
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // save the number of parameters to the file
        uint32 endianConvertedNumParameters = numParameters;
        ConvertUnsignedInt(&endianConvertedNumParameters, targetEndianType);
        file->Write(&endianConvertedNumParameters, sizeof(uint32));

        // add it to the log file
        //LogDetailedInfo("    - Parameters: (%i)", numParameters);

        // save the anim graph parameters
        for (i = 0; i < numParameters; ++i)
        {
            SaveAnimGraphParamInfo(file, animGraph->GetParameter(i), targetEndianType);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helpers
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // recursive get the node number of the given anim graph node inside the file
    bool RecursiveAnimGraphNodeNr(EMotionFX::AnimGraphNode* animGraphNode, EMotionFX::AnimGraphNode* searchFor, uint32& nodeNr)
    {
        if (animGraphNode == searchFor)
        {
            return true;
        }

        // increase the node counter
        nodeNr++;

        // get the number of children and iterate through them
        const uint32 numChildNodes = animGraphNode->GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            if (RecursiveAnimGraphNodeNr(animGraphNode->GetChildNode(i), searchFor, nodeNr))
            {
                return true;
            }
        }

        return false;
    }


    // get the node number of the given anim graph node inside the file
    uint32 GetAnimGraphNodeNr(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* animGraphNode)
    {
        if (animGraphNode == nullptr)
        {
            return MCORE_INVALIDINDEX32;
        }

        uint32 nodeNr = 0;

        EMotionFX::AnimGraphStateMachine* stateMachine = animGraph->GetRootStateMachine();
        if (RecursiveAnimGraphNodeNr(stateMachine, animGraphNode, nodeNr))
        {
            return nodeNr;
        }

        return MCORE_INVALIDINDEX32;
    }


    // for saving attributes in anim graph objects
    uint32 GetAttributeStreamSize(MCore::Attribute* attribute, MCore::AttributeSettings* attributeInfo)
    {
        uint32 result = sizeof(uint32) * 2; // the attribute stream size and the attribute type integer
        result += attribute->GetStreamSize();
        result += GetStringChunkSize(attributeInfo->GetInternalName());
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Node Connections
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // save the given anim graph node connection to the file
    void SaveNodeConnection(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* connectionParentNode, EMotionFX::BlendTreeConnection* connection, MCore::Endian::EEndianType targetEndianType)
    {
        EMotionFX::AnimGraphNode* sourceNode = connection->GetSourceNode();
        EMotionFX::AnimGraphNode* targetNode = connectionParentNode;

        EMotionFX::FileFormat::AnimGraph_NodeConnection connectionChunk;
        connectionChunk.mSourceNode     = GetAnimGraphNodeNr(animGraph, sourceNode);
        connectionChunk.mTargetNode     = GetAnimGraphNodeNr(animGraph, targetNode);
        connectionChunk.mSourceNodePort = static_cast<uint16>(sourceNode->GetOutputPort(connection->GetSourcePort()).mPortID);
        connectionChunk.mTargetNodePort = static_cast<uint16>(targetNode->GetInputPort(connection->GetTargetPort()).mPortID);

        // add it to the log file
        //LogDetailedInfo("    - Node Connection:");
        //LogDetailedInfo("        + SourceNode: name='%s', nr=%i", sourceNode->GetName(), connectionChunk.mSourceNode);
        //LogDetailedInfo("        + TargetNode: name='%s', nr=%i", targetNode->GetName(), connectionChunk.mTargetNode);
        //LogDetailedInfo("        + SourceNodePort: %i", connectionChunk.mSourceNodePort);
        //LogDetailedInfo("        + TargetNodePort: %i", connectionChunk.mTargetNodePort);

        // endian conversion
        ConvertUnsignedInt(&connectionChunk.mSourceNode, targetEndianType);
        ConvertUnsignedInt(&connectionChunk.mTargetNode, targetEndianType);
        ConvertUnsignedShort(&connectionChunk.mSourceNodePort, targetEndianType);
        ConvertUnsignedShort(&connectionChunk.mTargetNodePort, targetEndianType);

        // write the header to the stream
        file->Write(&connectionChunk, sizeof(EMotionFX::FileFormat::AnimGraph_NodeConnection));
    }


    // recursively save the node connections
    void RecursiveSaveNodeConnections(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* animGraphNode, MCore::Endian::EEndianType targetEndianType)
    {
        uint32 i;

        // get the number of connections, iterate through them and save all node connections for the given node
        const uint32 numConnections = animGraphNode->GetNumConnections();
        for (i = 0; i < numConnections; ++i)
        {
            SaveNodeConnection(file, animGraph, animGraphNode, animGraphNode->GetConnection(i), targetEndianType);
        }

        // get the number of children and iterate through them
        const uint32 numChildNodes = animGraphNode->GetNumChildNodes();
        for (i = 0; i < numChildNodes; ++i)
        {
            // get the child node and recurse down the hierarchy
            EMotionFX::AnimGraphNode* childNode = animGraphNode->GetChildNode(i);
            RecursiveSaveNodeConnections(file, animGraph, childNode, targetEndianType);
        }
    }


    // save all anim graph node connections to the file
    void SaveAnimGraphNodeConnections(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, MCore::Endian::EEndianType targetEndianType)
    {
        // get the number of connections for the given anim graph node and check if we have to save any connections, if not skip saving
        const uint32 numConnections = animGraph->RecursiveCalcNumNodeConnections();
        if (numConnections <= 0)
        {
            return;
        }

        // add it to the log file
        //LogDetailedInfo("- Node Connections: (%i)", numConnections);

        // save the chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID        = EMotionFX::FileFormat::ANIMGRAPH_CHUNK_NODECONNECTIONS;
        chunkHeader.mVersion        = 1;
        chunkHeader.mSizeInBytes    = sizeof(uint32) + numConnections * sizeof(EMotionFX::FileFormat::AnimGraph_NodeConnection);

        // convert endian and save the chunk header to the file
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // save the number of node connections to the file
        uint32 endianConvertedNumConnections = numConnections;
        ConvertUnsignedInt(&endianConvertedNumConnections, targetEndianType);
        file->Write(&endianConvertedNumConnections, sizeof(uint32));

        // save node connections
        RecursiveSaveNodeConnections(file, animGraph, animGraph->GetRootStateMachine(), targetEndianType);
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // State Machine Transitions
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // get the chunk size for a state transition
    uint32 GetStateTransitionSize(EMotionFX::AnimGraphStateTransition* transition)
    {
        // add the transition date to the result
        uint32 result = sizeof(EMotionFX::FileFormat::AnimGraph_StateTransition);
        result += GetAnimGraphObjectSize(transition);

        // add the condition date
        const uint32 numConditions = transition->GetNumConditions();
        for (uint32 i = 0; i < numConditions; ++i)
        {
            result += GetAnimGraphObjectSize(transition->GetCondition(i));
        }

        return result;
    }

    // save the given state machine transition to the file
    void SaveStateTransition(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphStateTransition* transition, MCore::Endian::EEndianType targetEndianType)
    {
        EMotionFX::AnimGraphNode* sourceNode = transition->GetSourceNode();
        EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();

        // check if we are dealing with a wildcard transition and reset the source node to nullptr in that case
        if (transition->GetIsWildcardTransition())
        {
            sourceNode = nullptr;
        }

        // create the state transition chunk and fill the data
        EMotionFX::FileFormat::AnimGraph_StateTransition transitionChunk;
        transitionChunk.mSourceNode     = GetAnimGraphNodeNr(animGraph, sourceNode);
        transitionChunk.mDestNode       = GetAnimGraphNodeNr(animGraph, targetNode);
        transitionChunk.mStartOffsetX   = transition->GetVisualStartOffsetX();
        transitionChunk.mStartOffsetY   = transition->GetVisualStartOffsetY();
        transitionChunk.mEndOffsetX     = transition->GetVisualEndOffsetX();
        transitionChunk.mEndOffsetY     = transition->GetVisualEndOffsetY();
        transitionChunk.mNumConditions  = transition->GetNumConditions();

        // add it to the log file
        /*AZ_Printf("EMotionFX", "    - State Transition:");
        if (transition->GetSourceNode())
        {
            AZ_Printf("EMotionFX", "        + SourceNode: name='%s', nr=%i", sourceNode->GetName(), transitionChunk.mSourceNode);
        }
        else
        {
            AZ_Printf("EMotionFX", "        + SourceNode: (WildCard)");
        }
        AZ_Printf("EMotionFX", "        + DestNode: name='%s', nr=%i", transition->GetTargetNode()->GetName(), transitionChunk.mDestNode);
        AZ_Printf("EMotionFX", "        + StartOffset: (%i, %i)", transitionChunk.mStartOffsetX, transitionChunk.mStartOffsetY);
        AZ_Printf("EMotionFX", "        + EndOffset: (%i, %i)", transitionChunk.mEndOffsetX, transitionChunk.mEndOffsetY);*/

        // endian conversion
        ConvertUnsignedInt(&transitionChunk.mSourceNode, targetEndianType);
        ConvertUnsignedInt(&transitionChunk.mDestNode, targetEndianType);
        ConvertInt(&transitionChunk.mStartOffsetX, targetEndianType);
        ConvertInt(&transitionChunk.mStartOffsetY, targetEndianType);
        ConvertInt(&transitionChunk.mEndOffsetX, targetEndianType);
        ConvertInt(&transitionChunk.mEndOffsetY, targetEndianType);
        ConvertUnsignedInt(&transitionChunk.mNumConditions, targetEndianType);

        // write the header to the stream
        file->Write(&transitionChunk, sizeof(EMotionFX::FileFormat::AnimGraph_StateTransition));

        //---------------------------------------
        // write the state transition data now
        //---------------------------------------
        EMotionFX::FileFormat::AnimGraph_NodeHeader nodeHeader;
        nodeHeader.mTypeID              = transition->GetType();
        nodeHeader.mVersion             = transition->GetAnimGraphSaveVersion();
        nodeHeader.mNumCustomDataBytes  = transition->GetCustomDataSize();
        nodeHeader.mParentIndex         = MCORE_INVALIDINDEX32;
        nodeHeader.mNumChildNodes       = 0;
        nodeHeader.mVisualPosX          = 0;
        nodeHeader.mVisualPosY          = 0;
        nodeHeader.mNumAttributes       = transition->GetNumAttributes();
        nodeHeader.mFlags               = 0;
        nodeHeader.mVisualizeColor      = transition->GetVisualColor();

        // endian conversion
        ConvertUnsignedInt(&nodeHeader.mTypeID, targetEndianType);
        ConvertUnsignedInt(&nodeHeader.mVersion, targetEndianType);
        ConvertUnsignedInt(&nodeHeader.mParentIndex, targetEndianType);
        ConvertUnsignedInt(&nodeHeader.mNumChildNodes, targetEndianType);
        ConvertUnsignedInt(&nodeHeader.mNumAttributes, targetEndianType);
        ConvertUnsignedInt(&nodeHeader.mVisualizeColor, targetEndianType);

        // save the chunk
        file->Write(&nodeHeader, sizeof(EMotionFX::FileFormat::AnimGraph_NodeHeader));

        // save the node attributes to the file
        transition->WriteAttributes(file, targetEndianType);

        // write the custom data
        transition->WriteCustomData(file, targetEndianType);

        //---------------------------------------
        // write the conditions
        //---------------------------------------

        // get the number of transition conditions and iterate through them
        const uint32 numConditions = transition->GetNumConditions();
        for (uint32 i = 0; i < numConditions; ++i)
        {
            // get the current transition condition
            EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(i);

            // create the condition chunk header and fill the data
            EMotionFX::FileFormat::AnimGraph_NodeHeader conditionHeader;
            conditionHeader.mTypeID             = condition->GetType();
            conditionHeader.mVersion            = condition->GetAnimGraphSaveVersion();
            conditionHeader.mNumCustomDataBytes = condition->GetCustomDataSize();
            conditionHeader.mParentIndex        = MCORE_INVALIDINDEX32;
            conditionHeader.mNumChildNodes      = 0;
            conditionHeader.mVisualPosX         = 0;
            conditionHeader.mVisualPosY         = 0;
            conditionHeader.mNumAttributes      = condition->GetNumAttributes();
            conditionHeader.mFlags              = 0;
            conditionHeader.mVisualizeColor     = MCore::RGBA(255, 255, 0);

            // endian conversion
            ConvertUnsignedInt(&conditionHeader.mTypeID, targetEndianType);
            ConvertUnsignedInt(&conditionHeader.mVersion, targetEndianType);
            ConvertUnsignedInt(&conditionHeader.mParentIndex, targetEndianType);
            ConvertUnsignedInt(&conditionHeader.mNumChildNodes, targetEndianType);
            ConvertUnsignedInt(&conditionHeader.mNumAttributes, targetEndianType);

            // save the chunk
            file->Write(&conditionHeader, sizeof(EMotionFX::FileFormat::AnimGraph_NodeHeader));

            // save the attributes to the file
            condition->WriteAttributes(file, targetEndianType);

            // write the custom data
            condition->WriteCustomData(file, targetEndianType);
        }
    }


    // save all state machine transitions for the given anim graph node to the file
    void SaveTransitionsForNode(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* animGraphNode, MCore::Endian::EEndianType targetEndianType)
    {
        // check if the node is a state machine, return directly if not
        if (animGraphNode->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            return;
        }

        // cast our node to a state machine
        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(animGraphNode);

        // get the number of state transitions and check if we have to save any transitions, if not skip saving
        const uint32 numStateTransitions = stateMachine->GetNumTransitions();
        if (numStateTransitions <= 0)
        {
            return;
        }

        // add it to the log file
        //LogDetailedInfo("- State Transitions: node='%s', numTransitions=%i", stateMachine->GetName(), numStateTransitions);

        // save the chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID        = EMotionFX::FileFormat::ANIMGRAPH_CHUNK_STATETRANSITIONS;
        chunkHeader.mVersion        = 1;
        chunkHeader.mSizeInBytes    = 2 * sizeof(uint32);
        for (uint32 a = 0; a < numStateTransitions; ++a)
        {
            chunkHeader.mSizeInBytes += GetStateTransitionSize(stateMachine->GetTransition(a));
        }

        // convert endian and save the chunk header to the file
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // save the number of state transitions to the file
        uint32 endianConvertedNumTransitions = numStateTransitions;
        ConvertUnsignedInt(&endianConvertedNumTransitions, targetEndianType);
        file->Write(&endianConvertedNumTransitions, sizeof(uint32));

        // save the blend node index
        uint32 endianConvertedNodeIndex = GetAnimGraphNodeNr(animGraph, animGraphNode);
        ConvertUnsignedInt(&endianConvertedNodeIndex, targetEndianType);
        file->Write(&endianConvertedNodeIndex, sizeof(uint32));

        // save the state transitions to the file
        for (uint32 i = 0; i < numStateTransitions; ++i)
        {
            SaveStateTransition(file, animGraph, stateMachine->GetTransition(i), targetEndianType);
        }
    }


    // recursively save the state transitions
    void RecursiveSaveStateTransitions(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* animGraphNode, MCore::Endian::EEndianType targetEndianType)
    {
        // save state transitions
        SaveTransitionsForNode(file, animGraph, animGraphNode, targetEndianType);

        // get the number of children and iterate through them
        const uint32 numChildNodes = animGraphNode->GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            // get the child node and recurse down the hierarchy
            EMotionFX::AnimGraphNode* childNode = animGraphNode->GetChildNode(i);
            RecursiveSaveStateTransitions(file, animGraph, childNode, targetEndianType);
        }
    }


    // save all anim graph node state machine transitions to the file
    void SaveAnimGraphStateTransitions(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, MCore::Endian::EEndianType targetEndianType)
    {
        RecursiveSaveStateTransitions(file, animGraph, animGraph->GetRootStateMachine(), targetEndianType);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Nodes
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // get the chunk size for a anim graph object
    uint32 GetAnimGraphObjectSize(EMotionFX::AnimGraphObject* animGraphNode)
    {
        uint32 result = sizeof(EMotionFX::FileFormat::AnimGraph_NodeHeader);

        // get the number of anim graph node attributes and iterate through them
        const uint32 numAttributes = animGraphNode->GetNumAttributes();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            result += GetAttributeStreamSize(animGraphNode->GetAttribute(i), EMotionFX::GetAnimGraphManager().GetAttributeInfo(animGraphNode, i));
        }

        // add custom data size
        result += animGraphNode->GetCustomDataSize();

        return result;
    }


    // get the chunk size for a anim graph node
    uint32 GetAnimGraphNodeSize(EMotionFX::AnimGraphNode* animGraphNode, bool includeName)
    {
        uint32 result = sizeof(EMotionFX::FileFormat::AnimGraph_NodeHeader);

        // get the number of anim graph node attributes and iterate through them
        const uint32 numAttributes = animGraphNode->GetNumAttributes();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            result += GetAttributeStreamSize(animGraphNode->GetAttribute(i), EMotionFX::GetAnimGraphManager().GetAttributeInfo(animGraphNode, i));
        }

        // add custom data size
        result += animGraphNode->GetCustomDataSize();

        if (includeName)
        {
            result += GetStringChunkSize(animGraphNode->GetName());
        }

        return result;
    }


    // save a anim graph node
    void SaveAnimGraphNode(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* animGraphNode, MCore::Endian::EEndianType targetEndianType)
    {
        // get the number of anim graph node attributes and the parent node
        EMotionFX::AnimGraphNode* parentNode       = animGraphNode->GetParentNode();

        EMotionFX::FileFormat::AnimGraph_NodeHeader nodeHeader;
        nodeHeader.mTypeID              = animGraphNode->GetType();
        nodeHeader.mVersion             = animGraphNode->GetAnimGraphSaveVersion();
        nodeHeader.mNumCustomDataBytes  = animGraphNode->GetCustomDataSize();
        nodeHeader.mParentIndex         = GetAnimGraphNodeNr(animGraph, parentNode);
        nodeHeader.mNumChildNodes       = animGraphNode->GetNumChildNodes();
        nodeHeader.mVisualPosX          = animGraphNode->GetVisualPosX();
        nodeHeader.mVisualPosY          = animGraphNode->GetVisualPosY();
        nodeHeader.mNumAttributes       = animGraphNode->GetNumAttributes();
        nodeHeader.mVisualizeColor      = animGraphNode->GetVisualizeColor();
        nodeHeader.mFlags = 0;

        if (animGraphNode->GetIsCollapsed())
        {
            nodeHeader.mFlags |= EMotionFX::FileFormat::ANIMGRAPH_NODEFLAG_COLLAPSED;
        }

        if (animGraphNode->GetIsVisualizationEnabled())
        {
            nodeHeader.mFlags |= EMotionFX::FileFormat::ANIMGRAPH_NODEFLAG_VISUALIZED;
        }

        if (animGraphNode->GetIsEnabled() == false)
        {
            nodeHeader.mFlags |= EMotionFX::FileFormat::ANIMGRAPH_NODEFLAG_DISABLED;
        }

        if (parentNode && parentNode->GetType() == EMotionFX::BlendTree::TYPE_ID)
        {
            if (static_cast<EMotionFX::BlendTree*>(parentNode)->GetVirtualFinalNode() == animGraphNode)
            {
                nodeHeader.mFlags |= EMotionFX::FileFormat::ANIMGRAPH_NODEFLAG_VIRTUALFINALOUTPUT;
            }
        }

        // add it to the log file
        //LogDetailedInfo("- Node: name='%s', nr=%i", animGraphNode->GetName(), GetAnimGraphNodeNr(animGraph, animGraphNode));
        //LogDetailedInfo("    + Type: name='%s', id=%i", animGraphNode->GetTypeString(), nodeHeader.mTypeID);
        //LogDetailedInfo("    + NumChilds: %i", nodeHeader.mNumChildNodes);
        //if (parentNode) LogDetailedInfo("    + Parent: name='%s', nr=%i", parentNode->GetName(), nodeHeader.mParentIndex);
        //LogDetailedInfo("    + VisualPos: x=%i, y=%i", nodeHeader.mVisualPosX, nodeHeader.mVisualPosY);
        //LogDetailedInfo("    + Version: %i", nodeHeader.mVersion);
        //LogDetailedInfo("    + Num attributes: %i", nodeHeader.mNumAttributes);

        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID        = EMotionFX::FileFormat::ANIMGRAPH_CHUNK_BLENDNODE;
        chunkHeader.mVersion        = 1;
        chunkHeader.mSizeInBytes    = GetAnimGraphNodeSize(animGraphNode, true);

        // convert endian and save the chunk header to the file
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // endian conversion
        ConvertUnsignedInt(&nodeHeader.mTypeID, targetEndianType);
        ConvertUnsignedInt(&nodeHeader.mVersion, targetEndianType);
        ConvertUnsignedInt(&nodeHeader.mParentIndex, targetEndianType);
        ConvertUnsignedInt(&nodeHeader.mNumChildNodes, targetEndianType);
        ConvertUnsignedInt(&nodeHeader.mNumAttributes, targetEndianType);
        ConvertUnsignedInt(&nodeHeader.mVisualizeColor, targetEndianType);
        ConvertInt(&nodeHeader.mVisualPosX, targetEndianType);
        ConvertInt(&nodeHeader.mVisualPosY, targetEndianType);

        // save the chunk
        file->Write(&nodeHeader, sizeof(EMotionFX::FileFormat::AnimGraph_NodeHeader));

        // followed by:
        SaveString(animGraphNode->GetName(), file, targetEndianType);

        // save the node attributes to the file
        animGraphNode->WriteAttributes(file, targetEndianType);

        // write the custom data
        animGraphNode->WriteCustomData(file, targetEndianType);
    }


    // recursively save the node and all its children and the children of the children
    void RecursiveSaveNodes(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* animGraphNode, MCore::Endian::EEndianType targetEndianType)
    {
        // save the node
        SaveAnimGraphNode(file, animGraph, animGraphNode, targetEndianType);

        // get the number of child nodes, iterate through and save them
        const uint32 numChildNodes = animGraphNode->GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            RecursiveSaveNodes(file, animGraph, animGraphNode->GetChildNode(i), targetEndianType);
        }
    }


    // save all anim graph nodes to the file in the correct order
    void SaveAnimGraphNodes(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, MCore::Endian::EEndianType targetEndianType)
    {
        RecursiveSaveNodes(file, animGraph, animGraph->GetRootStateMachine(), targetEndianType);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Anim Graph Node Groups
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // get the number of valid nodes in the node group
    uint32 GetNumNodesInNodeGroup(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNodeGroup* nodeGroup)
    {
        uint32 result = 0;

        // get the number of nodes in the node group and iterate through them
        const uint32 numNodes = nodeGroup->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // check if the anim graph node is valid and still part of the anim graph
            const uint32    nodeID          = nodeGroup->GetNode(i);
            EMotionFX::AnimGraphNode*  animGraphNode  = animGraph->RecursiveFindNodeByID(nodeID);
            if (animGraphNode)
            {
                result++;
            }
        }

        return result;
    }

    // calculate the size of the node group
    uint32 GetAnimGraphNodeGroupSize(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNodeGroup* nodeGroup)
    {
        uint32 result = sizeof(EMotionFX::FileFormat::AnimGraph_NodeGroup);
        result += GetNumNodesInNodeGroup(animGraph, nodeGroup) * sizeof(uint32);
        result += GetStringChunkSize(nodeGroup->GetName());
        return result;
    }


    // save the node group
    void SaveAnimGraphNodeGroup(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNodeGroup* nodeGroup, MCore::Endian::EEndianType targetEndianType)
    {
        // the node group chunk
        EMotionFX::FileFormat::AnimGraph_NodeGroup chunkNodeGroup;
        chunkNodeGroup.mIsVisible       = (uint8)nodeGroup->GetIsVisible();
        chunkNodeGroup.mNumNodes        = GetNumNodesInNodeGroup(animGraph, nodeGroup);
        CopyColor(nodeGroup->GetColor(), chunkNodeGroup.mColor);

        // convert endian
        ConvertUnsignedInt(&chunkNodeGroup.mNumNodes, targetEndianType);
        ConvertFileColor(&chunkNodeGroup.mColor, targetEndianType);

        // save the node group chunk and the name
        file->Write(&chunkNodeGroup, sizeof(EMotionFX::FileFormat::AnimGraph_NodeGroup));
        SaveString(nodeGroup->GetName(), file, targetEndianType);

        // get the number of nodes, iterate through, convert endian and write them
        const uint32 numNodes = nodeGroup->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // get the file global index of the node in the group
            const uint32    nodeID          = nodeGroup->GetNode(i);
            EMotionFX::AnimGraphNode*  animGraphNode  = animGraph->RecursiveFindNodeByID(nodeID);
            uint32          nodeIndex       = GetAnimGraphNodeNr(animGraph, animGraphNode);

            if (animGraphNode == nullptr)
            {
                MCore::LogWarning("The anim graph node group called '%s' contains a node (ID=%i) which is not valid.", nodeGroup->GetName(), nodeID);
            }
            else
            {
                // convert endian and write it to the file
                ConvertUnsignedInt(&nodeIndex, targetEndianType);
                file->Write(&nodeIndex, sizeof(uint32));
            }
        }
    }


    // save node groups
    void SaveAnimGraphNodeGroups(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, MCore::Endian::EEndianType targetEndianType)
    {
        uint32 i;

        // get the number of node groups and skip the chunk in case there are none
        const uint32 numNodeGroups = animGraph->GetNumNodeGroups();
        if (numNodeGroups == 0)
        {
            return;
        }

        // save the chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID        = EMotionFX::FileFormat::ANIMGRAPH_CHUNK_NODEGROUPS;
        chunkHeader.mVersion        = 1;

        // calculate the chunk size
        chunkHeader.mSizeInBytes    = sizeof(uint32);
        for (i = 0; i < numNodeGroups; ++i)
        {
            chunkHeader.mSizeInBytes += GetAnimGraphNodeGroupSize(animGraph, animGraph->GetNodeGroup(i));
        }

        // convert endian and save the chunk header to the file
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // save the node group chunk
        uint32 endianConvertedNumNodeGroups = numNodeGroups;
        ConvertUnsignedInt(&endianConvertedNumNodeGroups, targetEndianType);
        file->Write(&endianConvertedNumNodeGroups, sizeof(uint32));

        // iterate through the node groups and save them all
        for (i = 0; i < numNodeGroups; ++i)
        {
            SaveAnimGraphNodeGroup(file, animGraph, animGraph->GetNodeGroup(i), targetEndianType);
        }
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Anim Graph Parameter Groups
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // calculate the size of the group
    uint32 GetAnimGraphParameterGroupSize(EMotionFX::AnimGraphParameterGroup* parameterGroup)
    {
        uint32 result = sizeof(EMotionFX::FileFormat::AnimGraph_ParameterGroup);
        result += parameterGroup->GetNumParameters() * sizeof(uint32);
        result += GetStringChunkSize(parameterGroup->GetName());
        return result;
    }


    // save the parameter group
    void SaveAnimGraphParameterGroup(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphParameterGroup* parameterGroup, MCore::Endian::EEndianType targetEndianType)
    {
        MCORE_UNUSED(animGraph);

        // the group chunk
        EMotionFX::FileFormat::AnimGraph_ParameterGroup chunkParameterGroup;
        chunkParameterGroup.mNumParameters = parameterGroup->GetNumParameters();
        //#ifdef EMFX_EMSTUDIOBUILD
        chunkParameterGroup.mCollapsed = parameterGroup->GetIsCollapsed();
        //#endif

        // convert endian
        ConvertUnsignedInt(&chunkParameterGroup.mNumParameters, targetEndianType);

        // save the group chunk and the name
        file->Write(&chunkParameterGroup, sizeof(EMotionFX::FileFormat::AnimGraph_ParameterGroup));
        SaveString(parameterGroup->GetName(), file, targetEndianType);

        // get the number of parameters, iterate through, convert endian and write them
        const uint32 numParameters = parameterGroup->GetNumParameters();
        for (uint32 i = 0; i < numParameters; ++i)
        {
            // convert endian and write the parameter index to the file
            uint32 parameterIndex = parameterGroup->GetParameter(i);
            ConvertUnsignedInt(&parameterIndex, targetEndianType);
            file->Write(&parameterIndex, sizeof(uint32));
        }
    }


    // save parameter groups
    void SaveAnimGraphParameterGroups(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, MCore::Endian::EEndianType targetEndianType)
    {
        uint32 i;

        // get the number of parameter groups and skip the chunk in case there are none
        const uint32 numParameterGroups = animGraph->GetNumParameterGroups();
        if (numParameterGroups == 0)
        {
            return;
        }

        // save the chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID        = EMotionFX::FileFormat::ANIMGRAPH_CHUNK_PARAMETERGROUPS;
        chunkHeader.mVersion        = 1;

        // calculate the chunk size
        chunkHeader.mSizeInBytes    = sizeof(uint32);
        for (i = 0; i < numParameterGroups; ++i)
        {
            chunkHeader.mSizeInBytes += GetAnimGraphParameterGroupSize(animGraph->GetParameterGroup(i));
        }

        // convert endian and save the chunk header to the file
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // save the group chunk
        uint32 endianConvertedNumParameterGroups = numParameterGroups;
        ConvertUnsignedInt(&endianConvertedNumParameterGroups, targetEndianType);
        file->Write(&endianConvertedNumParameterGroups, sizeof(uint32));

        // iterate through the node groups and save them all
        for (i = 0; i < numParameterGroups; ++i)
        {
            SaveAnimGraphParameterGroup(file, animGraph, animGraph->GetParameterGroup(i), targetEndianType);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Game Controller Settings
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // get the size of a param info chunk
    uint32 GetAnimGraphGameControllerParamInfoSize(EMotionFX::AnimGraphGameControllerSettings::ParameterInfo* paramInfo)
    {
        uint32 result = sizeof(EMotionFX::FileFormat::AnimGraph_GameControllerParameterInfo);
        result += GetStringChunkSize(paramInfo->mParameterName.AsChar());
        return result;
    }


    // get the size of a button info chunk
    uint32 GetAnimGraphGameControllerButtonInfoSize(EMotionFX::AnimGraphGameControllerSettings::ButtonInfo* buttonInfo)
    {
        uint32 result = sizeof(EMotionFX::FileFormat::AnimGraph_GameControllerButtonInfo);
        result += GetStringChunkSize(buttonInfo->mString.AsChar());
        return result;
    }


    // get the size of the preset chunk
    uint32 GetAnimGraphGameControllerPresetSize(EMotionFX::AnimGraphGameControllerSettings::Preset* preset)
    {
        uint32 result = sizeof(EMotionFX::FileFormat::AnimGraph_GameControllerPreset);
        result += GetStringChunkSize(preset->GetName());

        uint32 i;
        const uint32 numParameterInfos = preset->GetNumParamInfos();
        for (i = 0; i < numParameterInfos; ++i)
        {
            result += GetAnimGraphGameControllerParamInfoSize(preset->GetParamInfo(i));
        }

        const uint32 numButtonInfos = preset->GetNumButtonInfos();
        for (i = 0; i < numButtonInfos; ++i)
        {
            result += GetAnimGraphGameControllerButtonInfoSize(preset->GetButtonInfo(i));
        }

        return result;
    }


    // save a game controller parameter info
    void SaveAnimGraphGameControllerParamInfo(MCore::Stream* file, EMotionFX::AnimGraphGameControllerSettings::ParameterInfo* parameterInfo, MCore::Endian::EEndianType targetEndianType)
    {
        // create and fill the button info chunk
        EMotionFX::FileFormat::AnimGraph_GameControllerParameterInfo paramInfoChunk;
        paramInfoChunk.mAxis    = parameterInfo->mAxis;
        paramInfoChunk.mMode    = static_cast<uint8>(parameterInfo->mMode);
        paramInfoChunk.mInvert  = parameterInfo->mInvert;

        // convert endian
        // nothing to convert, all uint8's

        // write the button info chunk
        file->Write(&paramInfoChunk, sizeof(EMotionFX::FileFormat::AnimGraph_GameControllerParameterInfo));

        // followed by:

        // 1. save the string of the parameter
        SaveString(parameterInfo->mParameterName.AsChar(), file, targetEndianType);
    }


    // save a game controller button info
    void SaveAnimGraphGameControllerButtonInfo(MCore::Stream* file, EMotionFX::AnimGraphGameControllerSettings::ButtonInfo* buttonInfo, MCore::Endian::EEndianType targetEndianType)
    {
        // create and fill the button info chunk
        EMotionFX::FileFormat::AnimGraph_GameControllerButtonInfo buttonInfoChunk;
        buttonInfoChunk.mButtonIndex    = static_cast<uint8>(buttonInfo->mButtonIndex);
        buttonInfoChunk.mMode           = static_cast<uint8>(buttonInfo->mMode);

        // convert endian
        // nothing to convert, all uint8's

        // write the button info chunk
        file->Write(&buttonInfoChunk, sizeof(EMotionFX::FileFormat::AnimGraph_GameControllerButtonInfo));

        // followed by:

        // 1. save the string of the button
        SaveString(buttonInfo->mString.AsChar(), file, targetEndianType);
    }


    // save a game controller preset
    void SaveAnimGraphGameControllerPreset(MCore::Stream* file, EMotionFX::AnimGraphGameControllerSettings::Preset* preset, MCore::Endian::EEndianType targetEndianType)
    {
        uint32 i;

        const uint32 numParameterInfos  = preset->GetNumParamInfos();
        const uint32 numButtonInfos     = preset->GetNumButtonInfos();

        // create and fill the preset chunk
        EMotionFX::FileFormat::AnimGraph_GameControllerPreset presetChunk;
        presetChunk.mNumParameterInfos  = numParameterInfos;
        presetChunk.mNumButtonInfos     = numButtonInfos;

        // convert endian
        ConvertUnsignedInt(&presetChunk.mNumParameterInfos, targetEndianType);
        ConvertUnsignedInt(&presetChunk.mNumButtonInfos, targetEndianType);

        // write the preset chunk
        file->Write(&presetChunk, sizeof(EMotionFX::FileFormat::AnimGraph_GameControllerPreset));

        // followed by:

        // 1. save the name of the preset
        SaveString(preset->GetName(), file, targetEndianType);

        // 2. iterate through the parameter infos and save them
        for (i = 0; i < numParameterInfos; ++i)
        {
            SaveAnimGraphGameControllerParamInfo(file, preset->GetParamInfo(i), targetEndianType);
        }

        // 3. iterate through the button infos and save them
        for (i = 0; i < numButtonInfos; ++i)
        {
            SaveAnimGraphGameControllerButtonInfo(file, preset->GetButtonInfo(i), targetEndianType);
        }
    }


    // save the game controller settings to disk
    void SaveAnimGraphGameControllerSettings(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, MCore::Endian::EEndianType targetEndianType)
    {
        uint32 i;

        // get the game controller settings
        EMotionFX::AnimGraphGameControllerSettings* gameControllerSettings = animGraph->GetGameControllerSettings();
        if (gameControllerSettings == nullptr)
        {
            return;
        }

        // get the number of presets and if there are none return directly
        const uint32 numPresets = gameControllerSettings->GetNumPresets();
        if (numPresets == 0)
        {
            return;
        }

        // save the chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID        = EMotionFX::FileFormat::ANIMGRAPH_CHUNK_GAMECONTROLLERSETTINGS;
        chunkHeader.mVersion        = 1;

        // calculate the chunk size
        chunkHeader.mSizeInBytes    = 2 * sizeof(uint32);
        for (i = 0; i < numPresets; ++i)
        {
            chunkHeader.mSizeInBytes += GetAnimGraphGameControllerPresetSize(gameControllerSettings->GetPreset(i));
        }

        // convert endian and save the chunk header to the file
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // save the active preset index
        uint32 endianConvertedActivePreset = gameControllerSettings->GetActivePresetIndex();
        ConvertUnsignedInt(&endianConvertedActivePreset, targetEndianType);
        file->Write(&endianConvertedActivePreset, sizeof(uint32));

        // save the number of presets
        uint32 endianConvertedNumPresets = numPresets;
        ConvertUnsignedInt(&endianConvertedNumPresets, targetEndianType);
        file->Write(&endianConvertedNumPresets, sizeof(uint32));

        // iterate through the node groups and save them all
        for (i = 0; i < numPresets; ++i)
        {
            SaveAnimGraphGameControllerPreset(file, gameControllerSettings->GetPreset(i), targetEndianType);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Anim Graph
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // save the header chunk of the anim graph file
    void SaveAnimGraphHeader(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, const char* companyName, MCore::Endian::EEndianType targetEndianType)
    {
        EMotionFX::AnimGraph::Statistics animGraphStats;
        animGraph->RecursiveCalcStatistics(animGraphStats);

        // the header information
        EMotionFX::FileFormat::AnimGraph_Header header;

        header.mFourCC[0]           = 'A';
        header.mFourCC[1]           = 'N';
        header.mFourCC[2]           = 'G';
        header.mFourCC[3]           = 'R';
        header.mFileVersion         = 1;
        header.mEndianType          = static_cast<uint8>(targetEndianType);
        header.mNumNodes            = animGraph->RecursiveCalcNumNodes();
        header.mNumStateTransitions = animGraphStats.m_numTransitions;
        header.mNumNodeConnections  = animGraph->RecursiveCalcNumNodeConnections();
        header.mNumParameters       = animGraph->GetNumParameters();

        // add it to the log file
        //LogDetailedInfo("- AnimGraph: company='%s', description='%s', copyright='%s'", companyName, description, copyright);
        //LogDetailedInfo("    + Endian: %s", header.mEndianType == Endian::ENDIAN_LITTLE ? "Little Endian" : "Big Endian");
        //LogDetailedInfo("    + NumNodes: %i", header.mNumNodes);
        //LogDetailedInfo("    + NumStateTransitions: %i", header.mNumStateTransitions);
        //LogDetailedInfo("    + NumNodeConnections: %i", header.mNumNodeConnections);
        //LogDetailedInfo("    + NumParameters: %i", header.mNumParameters);

        // endian conversion
        ConvertUnsignedInt(&header.mFileVersion, targetEndianType);
        ConvertUnsignedInt(&header.mNumNodes, targetEndianType);
        ConvertUnsignedInt(&header.mNumStateTransitions, targetEndianType);
        ConvertUnsignedInt(&header.mNumNodeConnections, targetEndianType);
        ConvertUnsignedInt(&header.mNumParameters, targetEndianType);

        // write the header to the stream
        file->Write(&header, sizeof(EMotionFX::FileFormat::AnimGraph_Header));

        // followed by:
        SaveString(animGraph->GetName(), file, targetEndianType);
        SaveString("", file, targetEndianType); // TODO: unsed, old copyright string
        SaveString(animGraph->GetDescription(), file, targetEndianType);
        SaveString(companyName, file, targetEndianType);
        SaveString(EMotionFX::GetEMotionFX().GetVersionString(), file, targetEndianType);
        SaveString(GetCompilationDate(), file, targetEndianType);
    }


    void SaveAnimGraphAdditionalInfo(MCore::Stream* file, EMotionFX::AnimGraph* animGraph, MCore::Endian::EEndianType targetEndianType)
    {
        // save the chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID        = EMotionFX::FileFormat::ANIMGRAPH_CHUNK_ADDITIONALINFO;
        chunkHeader.mVersion        = 1;
        chunkHeader.mSizeInBytes    = sizeof(EMotionFX::FileFormat::AnimGraph_AdditionalInfo);
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // write the additional info
        EMotionFX::FileFormat::AnimGraph_AdditionalInfo info;
        info.mUnitType = static_cast<uint8>(animGraph->GetUnitType());

        // convert endian
        // (no conversions needed)

        // write to the file
        file->Write(&info, sizeof(EMotionFX::FileFormat::AnimGraph_AdditionalInfo));
    }


    // save a anim graph
    void SaveAnimGraph(MCore::MemoryFile* file, EMotionFX::AnimGraph* animGraph, MCore::Endian::EEndianType targetEndianType, const char* companyName)
    {
        SaveAnimGraphHeader(file, animGraph, companyName, targetEndianType);
        SaveAnimGraphAdditionalInfo(file, animGraph, targetEndianType);
        SaveAnimGraphParameters(file, animGraph, targetEndianType);
        SaveAnimGraphNodes(file, animGraph, targetEndianType);
        SaveAnimGraphNodeConnections(file, animGraph, targetEndianType);
        SaveAnimGraphStateTransitions(file, animGraph, targetEndianType);
        SaveAnimGraphNodeGroups(file, animGraph, targetEndianType);
        SaveAnimGraphParameterGroups(file, animGraph, targetEndianType);
        SaveAnimGraphGameControllerSettings(file, animGraph, targetEndianType);
    }


    // return the anim graph file extension
    const char* GetAnimGraphFileExtension(bool includingDot)
    {
        if (includingDot)
        {
            return ".animgraph";
        }

        return "animgraph";
    }
} // namespace ExporterLib