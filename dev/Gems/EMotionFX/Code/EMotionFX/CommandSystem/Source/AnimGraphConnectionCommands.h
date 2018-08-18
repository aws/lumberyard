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
#pragma once

// include the required headers
#include <EMotionFX/Source/AnimGraph.h>
#include "CommandSystemConfig.h"
#include <EMotionFX/Source/AnimGraph.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandLine.h>
#include "CommandManager.h"
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/BlendTreeConnection.h>

namespace CommandSystem
{
    // create a connection
    MCORE_DEFINECOMMAND_START(CommandAnimGraphCreateConnection, "Connect two anim graph nodes", true)
        uint32                      mAnimGraphID;
        EMotionFX::AnimGraphNodeId  mTargetNodeId;
        EMotionFX::AnimGraphNodeId  mSourceNodeId;
        uint32                      mConnectionId;
        AZ::TypeId                  mTransitionType;
        int32                       mStartOffsetX;
        int32                       mStartOffsetY;
        int32                       mEndOffsetX;
        int32                       mEndOffsetY;
        int32                       mSourcePort;
        int32                       mTargetPort;
        AZStd::string               mSourcePortName;
        AZStd::string               mTargetPortName;
        bool                        mOldDirtyFlag;

    public:
        uint32 GetConnectionId() const                          { return mConnectionId; }
        EMotionFX::AnimGraphNodeId GetTargetNodeId() const      { return mTargetNodeId; }
        EMotionFX::AnimGraphNodeId GetSourceNodeId() const      { return mSourceNodeId; }
        AZ::TypeId GetTransitionType() const                    { return mTransitionType; }
        int32 GetSourcePort() const                             { return mSourcePort; }
        int32 GetTargetPort() const                             { return mTargetPort; }
        int32 GetStartOffsetX() const                           { return mStartOffsetX; }
        int32 GetStartOffsetY() const                           { return mStartOffsetY; }
        int32 GetEndOffsetX() const                             { return mEndOffsetX; }
        int32 GetEndOffsetY() const                             { return mEndOffsetY; }
    MCORE_DEFINECOMMAND_END


    // remove a connection
    MCORE_DEFINECOMMAND_START(CommandAnimGraphRemoveConnection, "Remove a anim graph connection", true)
        uint32                      mAnimGraphID;
        EMotionFX::AnimGraphNodeId  mTargetNodeId;
        AZStd::string               mTargetNodeName;
        EMotionFX::AnimGraphNodeId  mSourceNodeId;
        AZStd::string               mSourceNodeName;
        uint32                      mConnectionID;
        AZ::TypeId                  mTransitionType;
        int32                       mStartOffsetX;
        int32                       mStartOffsetY;
        int32                       mEndOffsetX;
        int32                       mEndOffsetY;
        int32                       mSourcePort;
        int32                       mTargetPort;
        bool                        mOldDirtyFlag;
        AZStd::string               mOldContents;

    public:
        EMotionFX::AnimGraphNodeId GetTargetNodeID() const      { return mTargetNodeId; }
        EMotionFX::AnimGraphNodeId GetSourceNodeID() const      { return mSourceNodeId; }
        AZ::TypeId GetTransitionType() const                    { return mTransitionType; }
        int32 GetSourcePort() const                             { return mSourcePort; }
        int32 GetTargetPort() const                             { return mTargetPort; }
        int32 GetStartOffsetX() const                           { return mStartOffsetX; }
        int32 GetStartOffsetY() const                           { return mStartOffsetY; }
        int32 GetEndOffsetX() const                             { return mEndOffsetX; }
        int32 GetEndOffsetY() const                             { return mEndOffsetY; }
        uint32 GetConnectionID() const                          { return mConnectionID; }
    MCORE_DEFINECOMMAND_END


    // adjust a connection
    MCORE_DEFINECOMMAND_START(CommandAnimGraphAdjustConnection, "Adjust a anim graph connection", true)
    public:
        void RewindTransitionIfActive(EMotionFX::AnimGraphStateTransition* transition);

        uint32                  mAnimGraphID;
        AZStd::string           m_stateMachineName;
        uint32                  mTransitionID;
        AZStd::string           mOldTargetNodeName;
        AZStd::string           mOldSourceNodeName;
        int32                   mStartOffsetX;
        int32                   mStartOffsetY;
        int32                   mEndOffsetX;
        int32                   mEndOffsetY;
        bool                    mOldDisabledFlag;
        bool                    mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    COMMANDSYSTEM_API void DeleteConnection(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* node, EMotionFX::BlendTreeConnection* connection, AZStd::vector<EMotionFX::BlendTreeConnection*>& connectionList);
    COMMANDSYSTEM_API void DeleteNodeConnections(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* node, EMotionFX::AnimGraphNode* parentNode, AZStd::vector<EMotionFX::BlendTreeConnection*>& connectionList, bool recursive);
    COMMANDSYSTEM_API void RelinkConnectionTarget(MCore::CommandGroup* commandGroup, const uint32 animGraphID, const char* sourceNodeName, uint32 sourcePortNr, const char* oldTargetNodeName, uint32 oldTargetPortNr, const char* newTargetNodeName, uint32 newTargetPortNr);
    COMMANDSYSTEM_API void RelinkConnectionSource(MCore::CommandGroup* commandGroup, const uint32 animGraphID, const char* oldSourceNodeName, uint32 oldSourcePortNr, const char* newSourceNodeName, uint32 newSourcePortNr, const char* targetNodeName, uint32 targetPortNr);

    COMMANDSYSTEM_API void DeleteStateTransition(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphStateTransition* transition, AZStd::vector<EMotionFX::AnimGraphStateTransition*>& transitionList);
    COMMANDSYSTEM_API void DeleteStateTransitions(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* state, EMotionFX::AnimGraphNode* parentNode, AZStd::vector<EMotionFX::AnimGraphStateTransition*>& transitionList, bool recursive);

    COMMANDSYSTEM_API void CopyBlendTreeConnection(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* targetNode, EMotionFX::BlendTreeConnection* connection, 
        bool cutMode, AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::unordered_map<EMotionFX::AnimGraphNode*, AZStd::string>& newNamesByCopiedNodes);
    COMMANDSYSTEM_API void CopyStateTransition(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphStateMachine* stateMachine, EMotionFX::AnimGraphStateTransition* transition, 
        bool cutMode, AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::unordered_map<EMotionFX::AnimGraphNode*, AZStd::string>& newNamesByCopiedNodes);

    COMMANDSYSTEM_API EMotionFX::AnimGraph* CommandsGetAnimGraph(const MCore::CommandLine& parameters, MCore::Command* command, AZStd::string& outResult);
} // namespace CommandSystem