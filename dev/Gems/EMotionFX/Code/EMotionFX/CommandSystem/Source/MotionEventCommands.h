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

#ifndef __EMFX_MOTIONEVENTCOMMANDS_H
#define __EMFX_MOTIONEVENTCOMMANDS_H

// include the required headers
#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/Motion.h>


namespace CommandSystem
{
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // EMotionFX::Motion* Event Track
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    MCORE_DEFINECOMMAND_START(CommandCreateMotionEventTrack, "Create motion event track", true)
    MCORE_DEFINECOMMAND_END

        MCORE_DEFINECOMMAND_START(CommandRemoveMotionEventTrack, "Remove motion event track", true)
    int32           mOldTrackIndex;
    bool            mOldEnabled;
    MCORE_DEFINECOMMAND_END

        MCORE_DEFINECOMMAND_START(CommandAdjustMotionEventTrack, "Adjust motion event track", true)
    AZStd::string   mOldName;
    bool            mOldEnabled;
    MCORE_DEFINECOMMAND_END

        MCORE_DEFINECOMMAND(CommandClearMotionEvents, "ClearMotionEvents", "Clear all motion events", false)

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // EMotionFX::Motion* Events
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    MCORE_DEFINECOMMAND_START(CommandCreateMotionEvent, "Create motion event", true)
    uint32  mMotionEventNr;
    MCORE_DEFINECOMMAND_END

        MCORE_DEFINECOMMAND_START(CommandRemoveMotionEvent, "Remove motion event", true)
    float           mOldStartTime;
    float           mOldEndTime;
    AZStd::string   mOldEventType;
    AZStd::string   mOldParameters;
    MCORE_DEFINECOMMAND_END

        MCORE_DEFINECOMMAND_START(CommandAdjustMotionEvent, "Adjust motion event", true)
    float           mOldStartTime;
    float           mOldEndTime;
    uint32          mOldEventTypeID;
    uint32          mOldMirrorTypeID;
    AZStd::string   mOldEventType;
    AZStd::string   mOldMirrorType;
    uint32          mOldParamIndex;
    MCORE_DEFINECOMMAND_END

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Command helpers
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void COMMANDSYSTEM_API CommandAddEventTrack();
    void COMMANDSYSTEM_API CommandRemoveEventTrack(uint32 trackIndex);
    void COMMANDSYSTEM_API CommandRenameEventTrack(uint32 trackIndex, const char* newName);
    void COMMANDSYSTEM_API CommandEnableEventTrack(uint32 trackIndex, bool isEnabled);
    void COMMANDSYSTEM_API CommandHelperAddMotionEvent(const char* trackName, float startTime, float endTime, const char* eventType = "", const char* eventParameters = "", MCore::CommandGroup* commandGroup = nullptr);
    void COMMANDSYSTEM_API CommandHelperRemoveMotionEvent(const char* trackName, uint32 eventNr, MCore::CommandGroup* commandGroup = nullptr);
    void COMMANDSYSTEM_API CommandHelperRemoveMotionEvent(uint32 motionID, const char* trackName, uint32 eventNr, MCore::CommandGroup* commandGroup = nullptr);
    void COMMANDSYSTEM_API CommandHelperRemoveMotionEvents(const char* trackName, const MCore::Array<uint32>& eventNumbers, MCore::CommandGroup* commandGroup = nullptr);
    void COMMANDSYSTEM_API CommandHelperMotionEventTrackChanged(uint32 eventNr, float startTime, float endTime, const char* oldTrackName, const char* newTrackName);
} // namespace CommandSystem


#endif
