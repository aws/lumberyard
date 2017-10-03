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

#ifndef __EMFX_LODCOMMANDS_H
#define __EMFX_LODCOMMANDS_H

// include the required headers
#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/LODGenerator.h>
#include <EMotionFX/Source/MorphSetup.h>


namespace CommandSystem
{
    // add a LOD level to the actor
    MCORE_DEFINECOMMAND_START(CommandAddLOD, "Add LOD", false)
    bool                            mOldDirtyFlag;
    MCore::String                   mOldSkeletalLOD;
    MCORE_DEFINECOMMAND_END


    // remove a LOD level from the actor
        MCORE_DEFINECOMMAND_START(CommandRemoveLOD, "Remove LOD", false)
    bool                            mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // helper functions
    EMotionFX::LODGenerator::InitSettings COMMANDSYSTEM_API ConstructInitSettings(MCore::Command* command, const MCore::CommandLine& parameters);
    EMotionFX::LODGenerator::GenerateSettings COMMANDSYSTEM_API ConstructGenerateSettings(MCore::Command* command, const MCore::CommandLine& parameters, EMotionFX::MorphSetup* morphSetup);
    void COMMANDSYSTEM_API ConstructInitSettingsCommandParameters(const EMotionFX::LODGenerator::InitSettings& initSettings, MCore::String* outString);
    void COMMANDSYSTEM_API ConstructGenerateSettingsCommandParameters(const EMotionFX::LODGenerator::GenerateSettings& generateSettings, MCore::String* outString, EMotionFX::MorphSetup* morphSetup);
    void COMMANDSYSTEM_API ClearLODLevels(EMotionFX::Actor* actor, MCore::CommandGroup* commandGroup = nullptr);

    void COMMANDSYSTEM_API ConstructReplaceAutomaticLODCommand(EMotionFX::Actor* actor, uint32 lodLevel, const EMotionFX::LODGenerator::InitSettings& initSettings, const EMotionFX::LODGenerator::GenerateSettings& generateSettings, const MCore::Array<uint32>& enabledNodeIDs, MCore::String* outString, bool useForMetaData = false);
    void COMMANDSYSTEM_API ConstructReplaceManualLODCommand(EMotionFX::Actor* actor, uint32 lodLevel, const char* lodActorFileName, const MCore::Array<uint32>& enabledNodeIDs, MCore::String* outString, bool useForMetaData = false);
} // namespace CommandSystem


#endif
