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
#include "CommandSystemConfig.h"
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/Command.h>
#include "CommandManager.h"

#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/BlendTreeConnection.h>


namespace CommandSystem
{
    // create a blend node
    MCORE_DEFINECOMMAND_START(CommandAnimGraphCreateNode, "Create a anim graph node", true)
public:
    uint32  mNodeID;
    uint32  mAnimGraphID;
    bool    mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // adjust a node
        MCORE_DEFINECOMMAND_START(CommandAnimGraphAdjustNode, "Adjust a anim graph node", true)
    uint32              mNodeID;
    int32               mOldPosX;
    int32               mOldPosY;
    AZStd::string       mOldName;
    bool                mOldDirtyFlag;
    bool                mOldEnabled;
    bool                mOldVisualized;
    AZStd::string       mNodeGroupName;
    AZStd::string       mOldParameterMask;

public:
    uint32 GetNodeID() const                        { return mNodeID; }
    const AZStd::string& GetOldName() const         { return mOldName; }
    uint32              mAnimGraphID;
    MCORE_DEFINECOMMAND_END


    // remove a node
    MCORE_DEFINECOMMAND_START(CommandAnimGraphRemoveNode, "Remove a anim graph node", true)
    uint32          mNodeID;
    uint32          mAnimGraphID;
    uint32          mParentID;
    AZStd::string   mType;
    AZStd::string   mParentName;
    AZStd::string   mName;
    AZStd::string   mNodeGroupName;
    int32           mPosX;
    int32           mPosY;
    AZStd::string   mOldAttributesString;
    bool            mCollapsed;
    bool            mOldDirtyFlag;
    bool            mIsEntryNode;

public:
    uint32 GetNodeID() const        { return mNodeID; }
    uint32 GetParentID() const      { return mParentID; }
    MCORE_DEFINECOMMAND_END


    // set the entry state of a state machine
        MCORE_DEFINECOMMAND_START(CommandAnimGraphSetEntryState, "Set entry state", true)
public:
    uint32          mAnimGraphID;
    uint32          mOldEntryStateNodeID;
    uint32          mOldStateMachineNodeID;
    bool            mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    COMMANDSYSTEM_API void CreateAnimGraphNode(EMotionFX::AnimGraph* animGraph, const AZStd::string& typeString, const AZStd::string& namePrefix, EMotionFX::AnimGraphNode* parentNode, int32 offsetX, int32 offsetY, const AZStd::string& attributesString);

    COMMANDSYSTEM_API AZStd::string FindNodeNameInReservationList(EMotionFX::AnimGraphNode* node, const MCore::Array<EMotionFX::AnimGraphNode*>& copiedNodes, const MCore::Array<AZStd::string>& nameReserveList);

    COMMANDSYSTEM_API void DeleteNodes(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& nodeNames);
    COMMANDSYSTEM_API void DeleteNodes(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& nodeNames, AZStd::vector<EMotionFX::AnimGraphNode*>& nodeList, AZStd::vector<EMotionFX::BlendTreeConnection*>& connectionList, AZStd::vector<EMotionFX::AnimGraphStateTransition*>& transitionList, bool autoChangeEntryStates = true);

    COMMANDSYSTEM_API void ConstructCopyAnimGraphNodesCommandGroup(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, AZStd::vector<EMotionFX::AnimGraphNode*>& inOutNodesToCopy, bool cutMode, bool ignoreTopLevelConnections, AZStd::vector<AZStd::string>& outResult);
    COMMANDSYSTEM_API void AdjustCopyAnimGraphNodesCommandGroup(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& nodesToCopy, int32 posX, int32 posY, const AZStd::string& parentName, bool cutMode);
} // namespace CommandSystem
