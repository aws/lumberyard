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

#ifndef __EMFX_ACTORCOMMANDS_H
#define __EMFX_ACTORCOMMANDS_H

// include the required headers
#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/ActorInstance.h>
#include "CommandManager.h"


namespace CommandSystem
{
    // adjust the given actor
        MCORE_DEFINECOMMAND_START(CommandAdjustActor, "Adjust actor", true)
    uint32                                          mOldMotionExtractionNodeIndex;
    uint32                                          mOldTrajectoryNodeIndex;
    MCore::String                                   mOldAttachmentNodes;
    MCore::String                                   mOldExcludedFromBoundsNodes;
    MCore::String                                   mOldName;
    MCore::Array<EMotionFX::Actor::NodeMirrorInfo>  mOldMirrorSetup;
    bool                                            mOldDirtyFlag;

    void                                            SetIsAttachmentNode(EMotionFX::Actor* actor, bool isAttachmentNode);
    void                                            SetIsExcludedFromBoundsNode(EMotionFX::Actor* actor, bool excludedFromBounds);
    MCORE_DEFINECOMMAND_END


    // set the collision meshes of the given actor
        MCORE_DEFINECOMMAND_START(CommandActorSetCollisionMeshes, "Actor set collison meshes", true)
    MCore::String   mOldNodeList;
    bool            mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // remove actor
        MCORE_DEFINECOMMAND_START(CommandRemoveActor, "Remove actor", true)
public:
    MCore::String   mData;
    uint32          mPreviouslyUsedID;
    MCore::String   mOldFileName;
    bool            mOldDirtyFlag;
    bool            mOldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // scale actor data
        MCORE_DEFINECOMMAND_START(CommandScaleActorData, "Scale actor data", true)
public:
    MCore::String   mOldUnitType;
    uint32          mActorID;
    float           mScaleFactor;
    bool            mOldActorDirtyFlag;
    bool            mUseUnitType;
    MCORE_DEFINECOMMAND_END


    // reset actor instance to bind pose
        MCORE_DEFINECOMMAND(CommandResetToBindPose, "ResetToBindPose", "Reset actor instance to bind pose", false)


    // this will be called in case all render actors need to get removed and reconstructed completely
    MCORE_DEFINECOMMAND(CommandReInitRenderActors, "ReInitRenderActors", "Reinit render actors", false)


    // will be called in case an actor got removed and we have to remove a render actor or in case there is a new actor we need to create a render actor for, all current render actors won't get touched
    MCORE_DEFINECOMMAND(CommandUpdateRenderActors, "UpdateRenderActors", "Update render actors", false)

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void COMMANDSYSTEM_API ClearScene(bool deleteActors = true, bool deleteActorInstances = true, MCore::CommandGroup* commandGroup = nullptr);
    void COMMANDSYSTEM_API PrepareCollisionMeshesNodesString(EMotionFX::Actor* actor, uint32 lod, MCore::String* outNodeNames);
    void COMMANDSYSTEM_API PrepareExcludedNodesString(EMotionFX::Actor* actor, MCore::String* outNodeNames);
} // namespace CommandSystem


#endif
