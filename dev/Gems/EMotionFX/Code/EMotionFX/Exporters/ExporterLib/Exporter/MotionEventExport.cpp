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
#include <EMotionFX/Source/MotionEvent.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/Motion.h>


namespace ExporterLib
{
    uint32 GetMotionEventTrackChunkSize(EMotionFX::MotionEventTrack* motionEventTrack, EMotionFX::MotionEventTable* motionEventTable)
    {
        MCORE_UNUSED(motionEventTable);

        uint32 i;

        // get the number of motion events and parameters in the motion event track
        const uint32 numParameters      = motionEventTrack->GetNumParameters();
        const uint32 numMotionEvents    = motionEventTrack->GetNumEvents();

        // iterate through the motion events and construct a motion event type array
        MCore::Array<AZStd::string> motionEventTypes;
        MCore::Array<AZStd::string> mirrorEventTypes;
        motionEventTypes.Reserve(numMotionEvents);
        mirrorEventTypes.Reserve(numMotionEvents);
        for (i = 0; i < numMotionEvents; ++i)
        {
            // get the current motion event
            EMotionFX::MotionEvent& motionEvent = motionEventTrack->GetEvent(i);

            // check if the motion event type has already been added to the array, if not, add it
            if (motionEventTypes.Find(motionEvent.GetEventTypeString()) == MCORE_INVALIDINDEX32)
            {
                motionEventTypes.Add(motionEvent.GetEventTypeString());
            }

            if (mirrorEventTypes.Find(motionEvent.GetMirrorEventTypeString()) == MCORE_INVALIDINDEX32)
            {
                mirrorEventTypes.Add(motionEvent.GetMirrorEventTypeString());
            }
        }

        // get the number of unique motion event types in this motion event table
        const uint32 numMotionEventTypes = motionEventTypes.GetLength();
        const uint32 numMirrorEventTypes = mirrorEventTypes.GetLength();

        // calculate the chunk size
        uint32 result = sizeof(EMotionFX::FileFormat::FileMotionEventTrack) + numMotionEvents * sizeof(EMotionFX::FileFormat::FileMotionEvent) + GetStringChunkSize(motionEventTrack->GetName());
        for (i = 0; i < numMotionEventTypes; ++i)
        {
            result += GetStringChunkSize(motionEventTypes[i].c_str());
        }
        for (i = 0; i < numParameters; ++i)
        {
            result += GetStringChunkSize(motionEventTrack->GetParameter(i));
        }
        for (i = 0; i < numMirrorEventTypes; ++i)
        {
            result += GetStringChunkSize(mirrorEventTypes[i].c_str());
        }

        return result;
    }


    void SaveMotionEventTrack(MCore::Stream* file, EMotionFX::MotionEventTrack* motionEventTrack, EMotionFX::MotionEventTable* motionEventTable, MCore::Endian::EEndianType targetEndianType)
    {
        MCORE_UNUSED(motionEventTable);

        uint32 i;

        // get the number of motion events and parameters in the motion event track
        const uint32 numParameters      = motionEventTrack->GetNumParameters();
        const uint32 numMotionEvents    = motionEventTrack->GetNumEvents();

        // iterate through the motion events and construct a motion event type array
        MCore::Array<AZStd::string> motionEventTypes;
        MCore::Array<AZStd::string> mirrorEventTypes;
        motionEventTypes.Reserve(numMotionEvents);
        mirrorEventTypes.Reserve(numMotionEvents);
        for (i = 0; i < numMotionEvents; ++i)
        {
            // get the current motion event
            EMotionFX::MotionEvent& motionEvent = motionEventTrack->GetEvent(i);

            // check if the motion event type has already been added to the array, if not, add it
            if (motionEventTypes.Find(motionEvent.GetEventTypeString()) == MCORE_INVALIDINDEX32)
            {
                motionEventTypes.Add(motionEvent.GetEventTypeString());
            }

            if (mirrorEventTypes.Find(motionEvent.GetMirrorEventTypeString()) == MCORE_INVALIDINDEX32)
            {
                mirrorEventTypes.Add(motionEvent.GetMirrorEventTypeString());
            }
        }

        // get the number of unique motion event types in this motion event table
        const uint32 numMotionEventTypes = motionEventTypes.GetLength();
        const uint32 numMirrorEventTypes = mirrorEventTypes.GetLength();

        // the motion event track chunk
        EMotionFX::FileFormat::FileMotionEventTrack motionEventTrackChunk;
        memset(&motionEventTrackChunk, 0, sizeof(EMotionFX::FileFormat::FileMotionEventTrack));
        motionEventTrackChunk.mNumEvents            = numMotionEvents;
        motionEventTrackChunk.mNumTypeStrings       = numMotionEventTypes;
        motionEventTrackChunk.mNumParamStrings      = numParameters;
        motionEventTrackChunk.mNumMirrorTypeStrings = numMirrorEventTypes;
        motionEventTrackChunk.mIsEnabled            = motionEventTrack->GetIsEnabled();

        // endian conversion
        ConvertUnsignedInt(&motionEventTrackChunk.mNumEvents,              targetEndianType);
        ConvertUnsignedInt(&motionEventTrackChunk.mNumParamStrings,        targetEndianType);
        ConvertUnsignedInt(&motionEventTrackChunk.mNumTypeStrings,         targetEndianType);
        ConvertUnsignedInt(&motionEventTrackChunk.mNumMirrorTypeStrings,   targetEndianType);

        // save the chunk motion event track chunk
        file->Write(&motionEventTrackChunk, sizeof(EMotionFX::FileFormat::FileMotionEventTrack));

        // followed by: the motion event track name
        SaveString(motionEventTrack->GetName(), file, targetEndianType);

        // followed by: the motion event type strings
        for (i = 0; i < numMotionEventTypes; ++i)
        {
            SaveString(motionEventTypes[i].c_str(), file, targetEndianType);
        }

        // followed by: the motion event parameters
        for (i = 0; i < numParameters; ++i)
        {
            SaveString(motionEventTrack->GetParameter(i), file, targetEndianType);
        }

        // followed by: the motion event parameters
        for (i = 0; i < numMirrorEventTypes; ++i)
        {
            SaveString(mirrorEventTypes[i].c_str(), file, targetEndianType);
        }

        // log info
        MCore::LogInfo(" - Motion Event Track: '%s'", motionEventTrack->GetName());
        MCore::LogDetailedInfo("    - Parameters (%i)", numParameters);
        for (i = 0; i < numParameters; ++i)
        {
            MCore::LogDetailedInfo("       + %s", motionEventTrack->GetParameter(i));
        }

        MCore::LogDetailedInfo("    - Event Types (%i)", numMotionEventTypes);
        for (i = 0; i < numMotionEventTypes; ++i)
        {
            MCore::LogDetailedInfo("       + %s", motionEventTypes[i].c_str());
        }

        MCore::LogDetailedInfo("    - Mirror Types (%i)", numMirrorEventTypes);
        for (i = 0; i < numMirrorEventTypes; ++i)
        {
            MCore::LogDetailedInfo("       + %s", mirrorEventTypes[i].c_str());
        }

        MCore::LogDetailedInfo("    - Motion Events (%i)", numMotionEvents);

        // save the motion events
        for (i = 0; i < numMotionEvents; ++i)
        {
            // get the current motion event
            EMotionFX::MotionEvent& motionEvent = motionEventTrack->GetEvent(i);

            // the motion event chunk
            EMotionFX::FileFormat::FileMotionEvent      motionEventChunk;
            memset(&motionEventChunk, 0, sizeof(EMotionFX::FileFormat::FileMotionEvent));
            motionEventChunk.mStartTime         = motionEvent.GetStartTime();
            motionEventChunk.mEndTime           = motionEvent.GetEndTime();
            motionEventChunk.mEventTypeIndex    = motionEventTypes.Find(motionEvent.GetEventTypeString());
            motionEventChunk.mMirrorTypeIndex   = mirrorEventTypes.Find(motionEvent.GetMirrorEventTypeString());
            motionEventChunk.mParamIndex        = motionEvent.GetParameterIndex();

            MCORE_ASSERT(motionEventChunk.mEventTypeIndex != MCORE_INVALIDINDEX32);
            MCORE_ASSERT(motionEventChunk.mParamIndex != MCORE_INVALIDINDEX16);

            // log info
            MCore::LogDetailedInfo("       - Motion Event #%i", i);
            MCore::LogDetailedInfo("          + StartTime  = %f", motionEventChunk.mStartTime);
            MCore::LogDetailedInfo("          + EndTime    = %f", motionEventChunk.mEndTime);
            MCore::LogDetailedInfo("          + Type       = '%s'", motionEvent.GetEventTypeString());
            MCore::LogDetailedInfo("          + Mirror Type= '%s'", motionEvent.GetMirrorEventTypeString());
            MCore::LogDetailedInfo("          + Parameters = '%s'", motionEventTrack->GetParameter(motionEvent.GetParameterIndex()));

            // endian conversion
            ConvertFloat(&motionEventChunk.mStartTime,             targetEndianType);
            ConvertFloat(&motionEventChunk.mEndTime,               targetEndianType);
            ConvertUnsignedInt(&motionEventChunk.mEventTypeIndex,  targetEndianType);
            ConvertUnsignedInt(&motionEventChunk.mMirrorTypeIndex, targetEndianType);
            ConvertUnsignedShort(&motionEventChunk.mParamIndex,    targetEndianType);

            // save the motion event chunk
            file->Write(&motionEventChunk, sizeof(EMotionFX::FileFormat::FileMotionEvent));
        }
    }


    void SaveMotionEvents(MCore::Stream* file, EMotionFX::MotionEventTable* motionEventTable, MCore::Endian::EEndianType targetEndianType)
    {
        uint32 i;

        // get the number of motion event tracks and check if we have to save any at all
        const uint32 numEventTracks = motionEventTable->GetNumTracks();
        if (numEventTracks <= 0)
        {
            return;
        }

        // the motion event table chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID = EMotionFX::FileFormat::SHARED_CHUNK_MOTIONEVENTTABLE;
        chunkHeader.mVersion = 1;

        // calculate size of the chunk
        chunkHeader.mSizeInBytes = sizeof(EMotionFX::FileFormat::FileMotionEventTable);
        for (i = 0; i < numEventTracks; ++i)
        {
            chunkHeader.mSizeInBytes += GetMotionEventTrackChunkSize(motionEventTable->GetTrack(i), motionEventTable);
        }

        // the motion event table chunk
        EMotionFX::FileFormat::FileMotionEventTable motionEventTableChunk;
        motionEventTableChunk.mNumTracks = numEventTracks;

        // endian conversion
        ConvertFileChunk(&chunkHeader,                         targetEndianType);
        ConvertUnsignedInt(&motionEventTableChunk.mNumTracks,  targetEndianType);

        // save the chunk header and the chunk
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));
        file->Write(&motionEventTableChunk, sizeof(EMotionFX::FileFormat::FileMotionEventTable));

        // log info
        MCore::LogDetailedInfo("============================================================");
        MCore::LogInfo(" - Motion Event Table (NumTracks=%i)", numEventTracks);
        MCore::LogDetailedInfo("============================================================");

        // followed by: the motion event tracks
        for (i = 0; i < numEventTracks; ++i)
        {
            SaveMotionEventTrack(file, motionEventTable->GetTrack(i), motionEventTable, targetEndianType);
        }
    }
} // namespace ExporterLib
