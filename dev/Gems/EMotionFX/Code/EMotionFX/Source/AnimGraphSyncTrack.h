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
#include <MCore/Source/Array.h>


namespace EMotionFX
{
    // forward declarations
    class MotionEventTrack;

    /**
     *
     *
     */
    class EMFX_API AnimGraphSyncTrack
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphSyncTrack, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_SYNCTRACK);

    public:
        struct EMFX_API Event
        {
            uint32      mID;        /**< The event type ID. */
            uint32      mMirrorID;  /**< The mirror ID. */
            float       mTime;      /**< The time in seconds, at which this event happens. */
        };

        AnimGraphSyncTrack();
        AnimGraphSyncTrack(const AnimGraphSyncTrack& track);
        ~AnimGraphSyncTrack();

        void Clear();
        void InitFromEventTrack(const MotionEventTrack* track);
        void InitFromEventTrackMirrored(const MotionEventTrack* track);
        void InitFromSyncTrackMirrored(const AnimGraphSyncTrack& track);

        bool FindEventIndices(float timeInSeconds, uint32* outIndexA, uint32* outIndexB) const;
        bool FindMatchingEvents(uint32 syncIndex, uint32 firstEventID, uint32 secondEventID, uint32* outIndexA, uint32* outIndexB, bool forward) const;
        float CalcSegmentLength(uint32 indexA, uint32 indexB) const;
        bool ExtractOccurrence(uint32 occurrence, uint32 firstEventID, uint32 secondEventID, uint32* outIndexA, uint32* outIndexB) const;
        uint32 CalcOccurrence(uint32 indexA, uint32 indexB) const;

        void SetDuration(float seconds);
        MCORE_INLINE float GetDuration() const                          { return mDuration; }

        MCORE_INLINE uint32 GetNumEvents() const                        { return mEvents.GetLength(); }
        MCORE_INLINE const Event& GetEvent(uint32 index) const          { return mEvents[index]; }

    private:
        MCore::Array<Event> mEvents;        /**< The sync events, sorted on time. */
        float               mDuration;      /**< The duration of the track. */
    };
}   // namespace EMotionFX
