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
#include <MCore/Source/Command.h>
#include "CommandManager.h"
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>


namespace CommandSystem
{
    // add a transition condition
    MCORE_DEFINECOMMAND_START(CommandAnimGraphAddCondition, "Add a transition condition", true)
    uint32          mAnimGraphID;
    size_t          mOldConditionIndex;
    bool            mOldDirtyFlag;
    AZStd::string   mOldContents;
    MCORE_DEFINECOMMAND_END

    // remove a transition condition
    MCORE_DEFINECOMMAND_START(CommandAnimGraphRemoveCondition, "Remove a transition condition", true)
    uint32          mAnimGraphID;
    AZ::TypeId      mOldConditionType;
    size_t          mOldConditionIndex;
    AZStd::string   mOldContents;
    bool            mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END

    // modify a transition condition
    MCORE_DEFINECOMMAND_START(CommandAnimGraphAdjustCondition, "Adjust a transition condition", true)
    uint32          mAnimGraphID;
    EMotionFX::AnimGraphNodeId mStateMachineId;
    uint32          mTransitionId;
    size_t          mConditionIndex;
    bool            mOldDirtyFlag;
    AZStd::string   mOldContents;
    MCORE_DEFINECOMMAND_END

} // namespace CommandSystem
