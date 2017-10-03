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
#include "CommandManager.h"
#include "ActorCommands.h"
#include "MotionCommands.h"
#include "MotionEventCommands.h"
#include "MotionSetCommands.h"
#include "MotionCompressionCommands.h"
#include "SelectionCommands.h"
#include "ImporterCommands.h"
#include "AnimGraphCommands.h"
#include "AnimGraphNodeCommands.h"
#include "AnimGraphNodeGroupCommands.h"
#include "AnimGraphConnectionCommands.h"
#include "AnimGraphParameterCommands.h"
#include "AnimGraphConditionCommands.h"
#include "MorphTargetCommands.h"
#include "AttachmentCommands.h"
#include "LODCommands.h"
#include "NodeGroupCommands.h"
#include "AnimGraphParameterGroupCommands.h"
#include "ActorInstanceCommands.h"
#include "MiscCommands.h"


namespace CommandSystem
{
    // the global command manager object
    CommandManager* gCommandManager = nullptr;

    // get the command manager
    CommandManager* GetCommandManager()
    {
        return gCommandManager;
    }


    // constructor
    CommandManager::CommandManager()
        : MCore::CommandManager()
    {
        // reserve some memory for the command objects
        mCommands.Reserve(128);

        // register actor commands
        RegisterCommand(new CommandImportActor());
        RegisterCommand(new CommandRemoveActor());
        RegisterCommand(new CommandScaleActorData());
        RegisterCommand(new CommandCreateActorInstance());
        RegisterCommand(new CommandRemoveActorInstance());
        RegisterCommand(new CommandAdjustMorphTarget());
        RegisterCommand(new CommandAdjustActorInstance());
        RegisterCommand(new CommandResetToBindPose());
        RegisterCommand(new CommandAddAttachment());
        RegisterCommand(new CommandRemoveAttachment());
        RegisterCommand(new CommandClearAttachments());
        RegisterCommand(new CommandAddDeformableAttachment());
        RegisterCommand(new CommandAdjustActor());
        RegisterCommand(new CommandActorSetCollisionMeshes());
        RegisterCommand(new CommandReInitRenderActors());
        RegisterCommand(new CommandUpdateRenderActors());
        RegisterCommand(new CommandAddLOD());
        RegisterCommand(new CommandRemoveLOD());

        // register motion commands
        RegisterCommand(new CommandImportMotion());
        RegisterCommand(new CommandRemoveMotion());
        RegisterCommand(new CommandScaleMotionData());
        RegisterCommand(new CommandPlayMotion());
        RegisterCommand(new CommandAdjustMotionInstance());
        RegisterCommand(new CommandAdjustDefaultPlayBackInfo());
        RegisterCommand(new CommandStopMotionInstances());
        RegisterCommand(new CommandStopAllMotionInstances());
        RegisterCommand(new CommandAdjustMotion());

        // register motion event commands
        RegisterCommand(new CommandCreateMotionEvent());
        RegisterCommand(new CommandRemoveMotionEvent());
        RegisterCommand(new CommandAdjustMotionEvent());
        RegisterCommand(new CommandClearMotionEvents());
        RegisterCommand(new CommandCreateMotionEventTrack());
        RegisterCommand(new CommandRemoveMotionEventTrack());
        RegisterCommand(new CommandAdjustMotionEventTrack());

        // register motion set commands
        RegisterCommand(new CommandCreateMotionSet());
        RegisterCommand(new CommandRemoveMotionSet());
        RegisterCommand(new CommandAdjustMotionSet());
        RegisterCommand(new CommandMotionSetAddMotion());
        RegisterCommand(new CommandMotionSetRemoveMotion());
        RegisterCommand(new CommandMotionSetAdjustMotion());
        RegisterCommand(new CommandLoadMotionSet());

        // register motion compression commands
        RegisterCommand(new CommandWaveletCompressMotion());
        RegisterCommand(new CommandKeyframeCompressMotion());

        // register node group commands
        RegisterCommand(new CommandAdjustNodeGroup());
        RegisterCommand(new CommandAddNodeGroup());
        RegisterCommand(new CommandRemoveNodeGroup());

        // register selection commands
        RegisterCommand(new CommandSelect());
        RegisterCommand(new CommandUnselect());
        RegisterCommand(new CommandClearSelection());
        RegisterCommand(new CommandToggleLockSelection());

        // anim graph commands
        RegisterCommand(new CommandAnimGraphCreateNode());
        RegisterCommand(new CommandAnimGraphAdjustNode());
        RegisterCommand(new CommandScaleAnimGraphData());
        RegisterCommand(new CommandAnimGraphCreateConnection());
        RegisterCommand(new CommandAnimGraphRemoveConnection());
        RegisterCommand(new CommandAnimGraphAdjustConnection());
        RegisterCommand(new CommandAnimGraphRemoveNode());
        RegisterCommand(new CommandAnimGraphCreateParameter());
        RegisterCommand(new CommandAnimGraphRemoveParameter());
        RegisterCommand(new CommandAnimGraphAdjustParameter());
        RegisterCommand(new CommandAnimGraphSwapParameters());
        RegisterCommand(new CommandLoadAnimGraph());
        RegisterCommand(new CommandAdjustAnimGraph());
        RegisterCommand(new CommandCreateAnimGraph());
        RegisterCommand(new CommandRemoveAnimGraph());
        RegisterCommand(new CommandCloneAnimGraph());
        RegisterCommand(new CommandActivateAnimGraph());
        RegisterCommand(new CommandAnimGraphSetEntryState());
        RegisterCommand(new CommandAnimGraphAddCondition());
        RegisterCommand(new CommandAnimGraphRemoveCondition());
        RegisterCommand(new CommandAnimGraphAddNodeGroup());
        RegisterCommand(new CommandAnimGraphRemoveNodeGroup());
        RegisterCommand(new CommandAnimGraphAdjustNodeGroup());
        RegisterCommand(new CommandAnimGraphAddParameterGroup());
        RegisterCommand(new CommandAnimGraphRemoveParameterGroup());
        RegisterCommand(new CommandAnimGraphAdjustParameterGroup());

        // register misc commands
        RegisterCommand(new CommandRecorderClear());

        gCommandManager     = this;
        mLockSelection      = false;
        mWorkspaceDirtyFlag = false;
    }


    // destructor
    CommandManager::~CommandManager()
    {
    }
} // namespace CommandSystem
