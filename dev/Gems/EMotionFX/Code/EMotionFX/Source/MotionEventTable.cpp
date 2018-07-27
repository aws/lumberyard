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
#include "MotionEventTable.h"
#include "MotionEvent.h"
#include "MotionEventTrack.h"
#include "EventManager.h"
#include "EventHandler.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionEventTable, MotionEventAllocator, 0)


    // constructor
    MotionEventTable::MotionEventTable()
        : BaseObject()
    {
        mTracks.SetMemoryCategory(EMFX_MEMCATEGORY_EVENTS);
        mSyncTrack = nullptr;
    }


    // destructor
    MotionEventTable::~MotionEventTable()
    {
        RemoveAllTracks();
    }


    // creation
    MotionEventTable* MotionEventTable::Create()
    {
        return aznew MotionEventTable();
    }


    // process all events within a given time range
    void MotionEventTable::ProcessEvents(float startTime, float endTime, ActorInstance* actorInstance, MotionInstance* motionInstance) const
    {
        // process all event tracks
        const uint32 numTracks = mTracks.GetLength();
        for (uint32 t = 0; t < numTracks; ++t)
        {
            MotionEventTrack* track = mTracks[t];

            // process the track's events
            if (track->GetIsEnabled())
            {
                track->ProcessEvents(startTime, endTime, actorInstance, motionInstance);
            }
        }
    }


    // extract all events from all tracks
    void MotionEventTable::ExtractEvents(float startTime, float endTime, MotionInstance* motionInstance, AnimGraphEventBuffer* outEventBuffer) const
    {
        // iterate over all tracks
        const uint32 numTracks = mTracks.GetLength();
        for (uint32 t = 0; t < numTracks; ++t)
        {
            MotionEventTrack* track = mTracks[t];

            // process the track's events
            if (track->GetIsEnabled())
            {
                track->ExtractEvents(startTime, endTime, motionInstance, outEventBuffer);
            }
        }
    }



    // remove all event tracks
    void MotionEventTable::RemoveAllTracks(bool delFromMem)
    {
        // remove the tracks from memory if we want to
        if (delFromMem)
        {
            const uint32 numTracks = mTracks.GetLength();
            for (uint32 i = 0; i < numTracks; ++i)
            {
                mTracks[i]->Destroy();
            }
        }

        mTracks.Clear();
    }


    // remove a specific track
    void MotionEventTable::RemoveTrack(uint32 index, bool delFromMem)
    {
        if (delFromMem)
        {
            mTracks[index]->Destroy();
        }

        mTracks.Remove(index);
    }


    // add a new track
    void MotionEventTable::AddTrack(MotionEventTrack* track)
    {
        mTracks.Add(track);
    }


    // insert a track
    void MotionEventTable::InsertTrack(uint32 index, MotionEventTrack* track)
    {
        mTracks.Insert(index, track);
    }


    // reserve space for a given amount of tracks to prevent re-allocs
    void MotionEventTable::ReserveNumTracks(uint32 numTracks)
    {
        mTracks.Reserve(numTracks);
    }


    // find a track by its name
    uint32 MotionEventTable::FindTrackIndexByName(const char* trackName) const
    {
        const uint32 numTracks = mTracks.GetLength();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            if (mTracks[i]->GetNameString() == trackName)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find a track name
    MotionEventTrack* MotionEventTable::FindTrackByName(const char* trackName) const
    {
        const uint32 trackIndex = FindTrackIndexByName(trackName);
        if (trackIndex == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        return mTracks[trackIndex];
    }


    // copy the table contents to another table
    void MotionEventTable::CopyTo(MotionEventTable* targetTable, Motion* targetTableMotion)
    {
        // remove all tracks in the target table
        targetTable->RemoveAllTracks();

        // copy over the tracks
        const uint32 numTracks = mTracks.GetLength();
        targetTable->ReserveNumTracks(numTracks); // pre-alloc space
        for (uint32 i = 0; i < numTracks; ++i)
        {
            // create the new track
            MotionEventTrack* newTrack = MotionEventTrack::Create(targetTableMotion);

            // copy its contents
            mTracks[i]->CopyTo(newTrack);

            // add the new track to the target
            targetTable->AddTrack(newTrack);
        }
    }


    // automatically create the sync track
    void MotionEventTable::AutoCreateSyncTrack(Motion* motion)
    {
        // check if the sync track is already there, if not create it
        MotionEventTrack* syncTrack = FindTrackByName("Sync");
        if (syncTrack == nullptr)
        {
            // create and add the sync track
            syncTrack = MotionEventTrack::Create("Sync", motion);
            AddTrack(syncTrack);
        }

        // make the sync track undeletable
        syncTrack->SetIsDeletable(false);

        // set the sync track shortcut
        mSyncTrack = syncTrack;
    }
} // namespace EMotionFX
