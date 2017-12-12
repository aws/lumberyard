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

#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/AnimGraphParameterGroup.h>
#include <EMotionFX/Source/AnimGraph.h>


namespace CommandSystem
{
    // Adjust a parameter group.
    MCORE_DEFINECOMMAND_START(CommandAnimGraphAdjustParameterGroup, "Adjust anim graph parameter group", true)
    public:
        static AZStd::string GenerateParameterNameString(EMotionFX::AnimGraph* animGraph, const AZStd::vector<uint32>& parameterIndices);

        enum Action
        {
            ACTION_ADD,
            ACTION_REMOVE,
            ACTION_NONE
        };
        Action GetAction(const MCore::CommandLine& parameters);

        AZStd::string                   mOldName;                   //! Parameter group name before command execution.
        AZStd::vector<AZStd::string>    mOldParameterGroupNames;
        bool                            mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END

    // Add a parameter group.
    MCORE_DEFINECOMMAND_START(CommandAnimGraphAddParameterGroup, "Add anim graph parameter group", true)
        bool                mOldDirtyFlag;
        AZStd::string       mOldName;
    MCORE_DEFINECOMMAND_END

    // Remove a parameter group.
    MCORE_DEFINECOMMAND_START(CommandAnimGraphRemoveParameterGroup, "Remove anim graph parameter group", true)
        AZStd::string       mOldName;
        AZStd::string       mOldParameterNames;
        uint32              mOldIndex;
        bool                mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END

    // helper functions
    COMMANDSYSTEM_API void RemoveParameterGroup(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphParameterGroup* parameterGroup, bool removeParameters, MCore::CommandGroup* commandGroup = nullptr);
    COMMANDSYSTEM_API void ClearParameterGroups(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup = nullptr);
    COMMANDSYSTEM_API void MoveParameterGroupCommand(EMotionFX::AnimGraph* animGraph, uint32 moveFrom, uint32 moveTo);
} // namespace CommandSystem