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
#include "EMotionFXConfig.h"
#include "AnimGraphSyncTrack.h"
#include "EventManager.h"
#include "MotionEventTrack.h"
#include "Motion.h"
//#include <MCore/Source/LogManager.h>


namespace EMotionFX
{
    // constructor
    AnimGraphSyncTrack::AnimGraphSyncTrack()
    {
        mDuration   = 0.0f;
        mEvents.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_SYNCTRACK);
    }


    // destructor
    AnimGraphSyncTrack::~AnimGraphSyncTrack()
    {
    }


    // copy constructor
    AnimGraphSyncTrack::AnimGraphSyncTrack(const AnimGraphSyncTrack& track)
    {
        mEvents     = track.mEvents;
        mDuration   = track.mDuration;
    }


    // clear all events
    void AnimGraphSyncTrack::Clear()
    {
        mDuration   = 0.0f;
        mEvents.Clear(false); // false to keep memory for later reuse
    }


    // init the synctrack from a given event track
    void AnimGraphSyncTrack::InitFromEventTrack(const MotionEventTrack* track)
    {
        // just iterate over all events in the track and store them as sync events
        const uint32 numEvents = track->GetNumEvents();
        mEvents.Resize(numEvents);
        for (uint32 i = 0; i < numEvents; ++i)
        {
            const MotionEvent& motionEvent = track->GetEvent(i);
            mEvents[i].mID          = motionEvent.GetEventTypeID();
            mEvents[i].mMirrorID    = motionEvent.GetMirrorEventID();
            mEvents[i].mTime        = motionEvent.GetStartTime();
        }

        // store the duration of the motion in the synctrack as well
        mDuration = track->GetMotion()->GetMaxTime();
    }


    // init the synctrack from a given event track, in a mirrored way
    void AnimGraphSyncTrack::InitFromEventTrackMirrored(const MotionEventTrack* track)
    {
        const uint32 numEvents = track->GetNumEvents();
        mEvents.Resize(numEvents);
        for (uint32 i = 0; i < numEvents; ++i)
        {
            const MotionEvent& motionEvent = track->GetEvent(i);

            mEvents[i].mID          = motionEvent.GetMirrorEventID();
            mEvents[i].mMirrorID    = motionEvent.GetEventTypeID();
            mEvents[i].mTime        = motionEvent.GetStartTime();
        }

        // store the duration of the motion in the synctrack as well
        mDuration = track->GetMotion()->GetMaxTime();
    }


    // init the synctrack from another sync track, but in a mirrored way
    void AnimGraphSyncTrack::InitFromSyncTrackMirrored(const AnimGraphSyncTrack& track)
    {
        MCORE_ASSERT(this != &track);
        *this = track;
        const uint32 numEvents = track.GetNumEvents();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            mEvents[i].mID          = track.mEvents[i].mMirrorID;
            mEvents[i].mMirrorID    = track.mEvents[i].mID;
        }
    }


    // find the event indices
    bool AnimGraphSyncTrack::FindEventIndices(float timeInSeconds, uint32* outIndexA, uint32* outIndexB) const
    {
        // if we have no events at all, or if we provide some incorrect time value
        const uint32 numEvents = mEvents.GetLength();
        if (numEvents == 0 || timeInSeconds > mDuration || timeInSeconds < 0.0f)
        {
            *outIndexA      = MCORE_INVALIDINDEX32;
            *outIndexB      = MCORE_INVALIDINDEX32;
            return false;
        }

        // we have only one event, return the same one twice
        // "..t..[x]..."
        if (numEvents == 1)
        {
            *outIndexA      = 0;
            *outIndexB      = 0;
            return true;
        }

        // special case
        // handle the first event: ".t..[x].....[y].t."
        // where t is the timeInSeconds
        // in this case we have to return [y] and [x] as found events
        if (mEvents[0].mTime > timeInSeconds || mEvents.GetLast().mTime <= timeInSeconds)
        {
            *outIndexA      = mEvents.GetLength() - 1;
            *outIndexB      = 0;
            return true;
        }

        // we have more than one event
        // iterate over all events to find the right one
        // in case of "...[x]....t...[y]....[z]...." we return [x] and [y]
        for (uint32 i = 0; i < numEvents - 1; ++i)
        {
            // if our timeInSeconds is between the previous and current event
            if (mEvents[i].mTime <= timeInSeconds && mEvents[i + 1].mTime > timeInSeconds)
            {
                *outIndexA      = i;
                *outIndexB      = i + 1;
                return true;
            }
        }

        // this should never happen
        *outIndexA      = MCORE_INVALIDINDEX32;
        *outIndexB      = MCORE_INVALIDINDEX32;
        return false;
    }


    // calculate the occurrence of a given index combination
    // the occurrence is basically the n-th time the combination shows up
    uint32 AnimGraphSyncTrack::CalcOccurrence(uint32 indexA, uint32 indexB) const
    {
        // if we wrapped around when looping or this is the only event (then both indexA and indexB are equal)
        if (indexA > indexB || indexA == indexB)
        {
            return 0;
        }

        // get the event IDs
        const uint32 eventA = mEvents[indexA].mID;
        const uint32 eventB = mEvents[indexB].mID;

        // check the looping section, which is occurence 0
        uint32 occurrence = 0;
        if (mEvents.GetLast().mID == eventA && mEvents[0].mID == eventB)
        {
            if ((mEvents.GetLength() - 1) == indexA && eventB == 0)
            {
                return occurrence;
            }
            else
            {
                occurrence++;
            }
        }

        // iterate over all events
        const uint32 numEvents = mEvents.GetLength();
        for (uint32 i = 0; i < numEvents - 1; ++i)
        {
            // if we have an event type ID match
            if (mEvents[i].mID == eventA && mEvents[i + 1].mID == eventB)
            {
                // if this is the event we search for, return the current occurrence
                if (i == indexA && (i + 1) == indexB)
                {
                    return occurrence;
                }
                else
                {
                    occurrence++; // it was the same combination of events, but not this one
                }
            }
        }

        // actually we didn't find this combination
        return MCORE_INVALIDINDEX32;
    }


    // extract the n-th occurring given event combination
    bool AnimGraphSyncTrack::ExtractOccurrence(uint32 occurrence, uint32 firstEventID, uint32 secondEventID, uint32* outIndexA, uint32* outIndexB) const
    {
        // if there are no events
        const uint32 numEvents = mEvents.GetLength();
        if (numEvents == 0)
        {
            *outIndexA = MCORE_INVALIDINDEX32;
            *outIndexB = MCORE_INVALIDINDEX32;
            return false;
        }

        // if we have just one event
        if (numEvents == 1)
        {
            // if the given event is a match
            if (firstEventID == mEvents[0].mID && secondEventID == mEvents[0].mID)
            {
                *outIndexA = 0;
                *outIndexB = 0;
                return true;
            }
            else
            {
                *outIndexA = MCORE_INVALIDINDEX32;
                *outIndexB = MCORE_INVALIDINDEX32;
                return false;
            }
        }

        // we have multiple events
        uint32 currentOccurrence = 0;
        bool found = false;

        // continue while we're not done yet
        while (currentOccurrence <= occurrence)
        {
            // first check the looping special case
            if (mEvents.GetLast().mID == firstEventID && mEvents[0].mID == secondEventID)
            {
                // if it's the given occurrence we need
                if (currentOccurrence == occurrence)
                {
                    *outIndexA = mEvents.GetLength() - 1;
                    *outIndexB = 0;
                    return true;
                }
                else
                {
                    currentOccurrence++; // look further
                    found = true;
                }
            }

            // iterate over all event segments
            for (uint32 i = 0; i < numEvents - 1; ++i)
            {
                // if this event segment/combo is the one we're looking for
                if (mEvents[i].mID == firstEventID && mEvents[i + 1].mID == secondEventID)
                {
                    // if it's the given occurrence we need
                    if (currentOccurrence == occurrence)
                    {
                        *outIndexA = i;
                        *outIndexB = i + 1;
                        return true;
                    }
                    else
                    {
                        currentOccurrence++; // look further
                        found = true;
                    }
                }
            } // for all events

            // if we didn't find a single hit we won't find any other
            if (found == false)
            {
                *outIndexA = MCORE_INVALIDINDEX32;
                *outIndexB = MCORE_INVALIDINDEX32;
                return false;
            }
        }

        // we didn't find it
        *outIndexA = MCORE_INVALIDINDEX32;
        *outIndexB = MCORE_INVALIDINDEX32;
        return false;
    }


    // try to find a matching event combination
    bool AnimGraphSyncTrack::FindMatchingEvents(uint32 syncIndex, uint32 firstEventID, uint32 secondEventID, uint32* outIndexA, uint32* outIndexB, bool forward) const
    {
        // if the number of events is zero, we certainly don't have a match
        const uint32 numEvents = mEvents.GetLength();
        if (numEvents == 0)
        {
            *outIndexA = MCORE_INVALIDINDEX32;
            *outIndexB = MCORE_INVALIDINDEX32;
            return false;
        }

        // if there is just one event
        if (numEvents == 1)
        {
            if (firstEventID == mEvents[0].mID && secondEventID == mEvents[0].mID)
            {
                *outIndexA = 0;
                *outIndexB = 0;
                return true;
            }
            else
            {
                *outIndexA = MCORE_INVALIDINDEX32;
                *outIndexB = MCORE_INVALIDINDEX32;
                return false;
            }
        }

        // if the sync index is not set, start at the first pair (which starts from the last sync key)
        if (syncIndex == MCORE_INVALIDINDEX32)
        {
            if (forward)
            {
                syncIndex = numEvents - 1;
            }
            else
            {
                syncIndex = 0;
            }
        }

        if (forward)
        {
            // check all segments starting from the syncIndex, till the end
            for (uint32 i = syncIndex; i < numEvents; ++i)
            {
                // if this isn't the last looping segment
                if (i + 1 < numEvents)
                {
                    if (mEvents[i].mID == firstEventID && mEvents[i + 1].mID == secondEventID)
                    {
                        *outIndexA = i;
                        *outIndexB = i + 1;
                        return true;
                    }
                }
                else // the looping case
                {
                    if (mEvents.GetLast().mID == firstEventID && mEvents[0].mID == secondEventID)
                    {
                        *outIndexA = mEvents.GetLength() - 1;
                        *outIndexB = 0;
                        return true;
                    }
                }
            }

            // check everything before
            for (uint32 i = 0; i < syncIndex; ++i)
            {
                if (mEvents[i].mID == firstEventID && mEvents[i + 1].mID == secondEventID)
                {
                    *outIndexA = i;
                    *outIndexB = i + 1;
                    return true;
                }
            }
        }
        else // backward playback
        {
            // check all segments starting from the syncIndex, till the start
            for (int32 i = syncIndex; i >= 0; --i)
            {
                // if this isn't the last looping segment
                if (i - 1 >= 0)
                {
                    if (mEvents[i - 1].mID == firstEventID && mEvents[i].mID == secondEventID)
                    {
                        *outIndexA = i - 1;
                        *outIndexB = i;
                        return true;
                    }
                }
                else // the looping case
                {
                    if (mEvents[numEvents - 1].mID == firstEventID && mEvents[0].mID == secondEventID)
                    {
                        *outIndexA = numEvents - 1;
                        *outIndexB = 0;
                        return true;
                    }
                }
            }

            // check everything after
            for (uint32 i = numEvents - 1; i > syncIndex; --i)
            {
                if (mEvents[i - 1].mID == firstEventID && mEvents[i].mID == secondEventID)
                {
                    *outIndexA = i - 1;
                    *outIndexB = i;
                    return true;
                }
            }
        }

        // shouldn't be reached
        *outIndexA = MCORE_INVALIDINDEX32;
        *outIndexB = MCORE_INVALIDINDEX32;
        return false;
    }


    // calculate the segment length in seconds
    float AnimGraphSyncTrack::CalcSegmentLength(uint32 indexA, uint32 indexB) const
    {
        if (indexA < indexB) // this is normal, the first event comes before the second
        {
            return mEvents[indexB].mTime - mEvents[indexA].mTime;
        }
        else // looping case
        {
            return mDuration - mEvents[indexA].mTime + mEvents[indexB].mTime;
        }
    }


    void AnimGraphSyncTrack::SetDuration(float seconds)
    {
        mDuration = seconds;
    }


    /*
    // blend the two tracks
    void AnimGraphSyncTrack::Blend(const AnimGraphSyncTrack* dest, float weight)
    {
        MCORE_UNUSED(dest);
        MCORE_UNUSED(weight);
        // TODO: remove?

        // find the event track which has most events
        const uint32 numSourceEvents = mSyncPoints.GetLength();
        const uint32 numDestEvents   = dest->mSyncPoints.GetLength();

        if (numSourceEvents > 0 && numDestEvents == 0)  // keep the current track
            return;

        // handle cases where there is only one track with events
        if (numSourceEvents == 0 && numDestEvents > 0)
        {
            mSyncPoints = dest->mSyncPoints;
            mDuration   = dest->mDuration;
            return;
        }

        // get the source and dest duration
        const float sourceDuration  = mDuration;
        const float destDuration    = dest->mDuration;

        // if the source has more (or equal) amount of events, then we have to repeat/tile/wrap/loop the dest
        if (numSourceEvents >= numDestEvents)
        {
            // for all source events
            float destEventTime = 0.0f;
            for (uint32 i=0; i<numSourceEvents; ++i)
            {
                // get the source event time
                const float sourceEventTime = mSyncPoints[i];

                // find the dest event time
                if (i >= numDestEvents)
                {
                    const uint32 eventIndex = i % numDestEvents;
                    destEventTime += CalcTimeDeltaToPreviousEvent( dest, eventIndex );
                }
                else
                    destEventTime = dest->mSyncPoints[i];

                // store the interpolated time
                mSyncPoints[i]  = MCore::LinearInterpolate<float>(sourceEventTime, destEventTime, weight);
            }
        }
        else    // the dest track has more keys
        {
            mSyncPoints.Resize( numDestEvents );

            // for all dest events
            float sourceEventTime = 0.0f;
            for (uint32 i=0; i<numDestEvents; ++i)
            {
                // get the dest event time
                const float destEventTime = dest->mSyncPoints[i];

                // find the source event time
                if (i >= numSourceEvents)
                {
                    const uint32 eventIndex = i % numSourceEvents;
                    sourceEventTime += CalcTimeDeltaToPreviousEvent( this, eventIndex );
                }
                else
                    sourceEventTime = mSyncPoints[i];

                // store the interpolated time
                mSyncPoints[i]  = MCore::LinearInterpolate<float>(sourceEventTime, destEventTime, weight);
            }
        }

        // interpolate the duration
        mDuration = MCore::LinearInterpolate<float>(sourceDuration, destDuration, weight);

        //AZStd::string finalString;
        //for (uint32 i=0; i<mSyncPoints.GetLength(); ++i)
    //      finalString += AZStd::string::format("%.3f ", mSyncPoints[i]);
    //  finalString += AZStd::string::format(" [duration=%f, weight=%f]", mDuration, weight);
    //  MCore::LogInfo( finalString );

    }
    */
    /*
    void AnimGraphSyncTrack::Match(AnimGraphSyncTrack* dest)
    {
        // find the event track which has most events
        const uint32 numSourceEvents = mSyncPoints.GetLength();
        const uint32 numDestEvents   = dest->mSyncPoints.GetLength();

        if (numSourceEvents > 0 && numDestEvents == 0)  // keep the current track
            return;

        // handle cases where there is only one track with events
        if (numSourceEvents == 0 && numDestEvents > 0)
        {
            //mSyncPoints   = dest->mSyncPoints;
            //mDuration = dest->mDuration;
            return;
        }

        // get the source and dest duration
        //const float sourceDuration  = mDuration;
        //const float destDuration  = dest->mDuration;

        // if the source has more (or equal) amount of events, then we have to repeat/tile/wrap/loop the dest
        if (numSourceEvents >= numDestEvents)
        {
            // make sure we have enough points
            dest->mSyncPoints.Resize( numSourceEvents );

            // for all source events
            float destEventTime = 0.0f;
            for (uint32 i=0; i<numSourceEvents; ++i)
            {
                // get the source event time
                //const float sourceEventTime = mSyncPoints[i];

                // find the dest event time
                if (i >= numDestEvents)
                {
                    const uint32 eventIndex = i % numDestEvents;
                    destEventTime += CalcTimeDeltaToPreviousEvent( dest, eventIndex );
                }
                else
                    destEventTime = dest->mSyncPoints[i];

                // store the matched event time in dest
                dest->mSyncPoints[i] = destEventTime;
            }
        }
        else    // the dest track has more keys
        {
            mSyncPoints.Resize( numDestEvents );

            // for all dest events
            float sourceEventTime = 0.0f;
            for (uint32 i=0; i<numDestEvents; ++i)
            {
                // get the dest event time
                //const float destEventTime = dest->mSyncPoints[i];

                // find the source event time
                if (i >= numSourceEvents)
                {
                    const uint32 eventIndex = i % numSourceEvents;
                    sourceEventTime += CalcTimeDeltaToPreviousEvent( this, eventIndex );
                }
                else
                    sourceEventTime = mSyncPoints[i];

                // store the interpolated time
                mSyncPoints[i]  = sourceEventTime;
            }
        }

        // interpolate the duration
        //mDuration = MCore::LinearInterpolate<float>(sourceDuration, destDuration, weight);
    }
    */
}   // namespace EMotionFX

