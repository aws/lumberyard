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
#include "EMotionFXConfig.h"
#include "BaseObject.h"

#include <MCore/Source/Array.h>


namespace EMotionFX
{
    // forward declarations
    class MotionInstance;
    class ActorInstance;
    class MotionEventTrack;
    class Motion;
    class AnimGraphEventBuffer;

    /**
     * The motion event table, which stores all events and their data on a memory efficient way.
     * Events have three generic properties: a time value, an event type string and a parameter string.
     * Unique strings are only stored once in memory, so if you have for example ten events of the type "SOUND"
     * only 1 string will be stored in memory, and the events will index into the table to retrieve the string.
     * The event table can also figure out what events to process within a given time range.
     * The handling of those events is done by the MotionEventHandler class that you specify to the MotionEventManager singleton.
     */
    class EMFX_API MotionEventTable
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL
        friend class MotionEvent;

    public:
        static MotionEventTable* Create();

        /**
         * Process all events within a given time range.
         * @param startTime The start time of the range, in seconds.
         * @param endTime The end time of the range, in seconds.
         * @param actorInstance The actor instance on which to trigger the event.
         * @param motionInstance The motion instance which triggers the event.
         * @note The end time is also allowed to be smaller than the start time.
         */
        void ProcessEvents(float startTime, float endTime, ActorInstance* actorInstance, MotionInstance* motionInstance) const;

        /**
         * Extract all events within a given time range, and output them to an event buffer.
         * @param startTime The start time of the range, in seconds.
         * @param endTime The end time of the range, in seconds.
         * @param motionInstance The motion instance which triggers the event.
         * @param outEventBuffer The output event buffer.
         * @note The end time is also allowed to be smaller than the start time.
         */
        void ExtractEvents(float startTime, float endTime, MotionInstance* motionInstance, AnimGraphEventBuffer* outEventBuffer) const;

        /**
         * Remove all motion event tracks.
         */
        void ReserveNumTracks(uint32 numTracks);
        void RemoveAllTracks(bool delFromMem = true);
        void RemoveTrack(uint32 index, bool delFromMem = true);
        void AddTrack(MotionEventTrack* track);
        void InsertTrack(uint32 index, MotionEventTrack* track);

        uint32 FindTrackIndexByName(const char* trackName) const;
        MotionEventTrack* FindTrackByName(const char* trackName) const;

        MCORE_INLINE uint32 GetNumTracks() const                    { return mTracks.GetLength(); }
        MCORE_INLINE MotionEventTrack* GetTrack(uint32 index)       { return mTracks[index]; }
        MCORE_INLINE MotionEventTrack* GetSyncTrack() const         { return mSyncTrack; }

        void AutoCreateSyncTrack(Motion* motion);

        void CopyTo(MotionEventTable* targetTable, Motion* targetTableMotion);

    private:
        MCore::Array<MotionEventTrack*>     mTracks;    /**< The motion event tracks. */
        MotionEventTrack*                   mSyncTrack; /**< A shortcut to the track containing sync events. */

        /**
         * The constructor.
         */
        MotionEventTable();

        /**
         * The destructor.
         */
        ~MotionEventTable();
    };
} // namespace EMotionFX
