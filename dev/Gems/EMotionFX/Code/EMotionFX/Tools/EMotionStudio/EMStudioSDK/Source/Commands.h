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

#include "EMStudioConfig.h"
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/MotionSet.h>


namespace EMStudio
{
    MCORE_DEFINECOMMAND_START(CommandSaveActorAssetInfo, "Save actor assetinfo", false)
    MCORE_DEFINECOMMAND_END

        MCORE_DEFINECOMMAND_START(CommandSaveMotionAssetInfo, "Save motion assetinfo", false)
    MCORE_DEFINECOMMAND_END

        MCORE_DEFINECOMMAND_START(CommandSaveMotionSet, "Save motion set", false)
    void RecursiveSetDirtyFlag(EMotionFX::MotionSet* motionSet, bool dirtyFlag);
    MCORE_DEFINECOMMAND_END

        MCORE_DEFINECOMMAND_START(CommandSaveAnimGraph, "Save a anim graph", false)
    MCORE_DEFINECOMMAND_END

        MCORE_DEFINECOMMAND_START(CommandSaveWorkspace, "Save Workspace", false)
    MCORE_DEFINECOMMAND_END
} // namespace EMStudio