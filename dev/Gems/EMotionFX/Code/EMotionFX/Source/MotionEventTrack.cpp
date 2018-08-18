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
#include "MotionEventTrack.h"
#include "MotionEvent.h"
#include "EventManager.h"
#include "EventHandler.h"
#include "AnimGraphEventBuffer.h"
#include <MCore/Source/StringIdPool.h>
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionEventTrack, MotionEventAllocator, 0)


    // constructor
    MotionEventTrack::MotionEventTrack(Motion* motion)
        : BaseObject()
    {
        mMotion     = motion;
        mEnabled    = true;
        mNameID     = MCORE_INVALIDINDEX32;
        mParameters.SetMemoryCategory(EMFX_MEMCATEGORY_EVENTS);
        mEvents.SetMemoryCategory(EMFX_MEMCATEGORY_EVENTS);
        mDeletable  = true;
    }


    // extended constructor
    MotionEventTrack::MotionEventTrack(const char* name, Motion* motion)
        : BaseObject()
    {
        mMotion     = motion;
        mEnabled    = true;
        mDeletable  = true;

        mParameters.SetMemoryCategory(EMFX_MEMCATEGORY_EVENTS);
        mEvents.SetMemoryCategory(EMFX_MEMCATEGORY_EVENTS);
        SetName(name);
    }


    // destructor
    MotionEventTrack::~MotionEventTrack()
    {
    }


    // creation
    MotionEventTrack* MotionEventTrack::Create(Motion* motion)
    {
        return aznew MotionEventTrack(motion);
    }


    // creation
    MotionEventTrack* MotionEventTrack::Create(const char* name, Motion* motion)
    {
        return aznew MotionEventTrack(name, motion);
    }


    // set the name of the motion event track
    void MotionEventTrack::SetName(const char* name)
    {
        mNameID = MCore::GetStringIdPool().GenerateIdForString(name);
    }


    // get the parameter index by string
    uint32 MotionEventTrack::GetParameterIndex(const char* parameters) const
    {
        // try to find locate the parameter string in the array
        const uint32 numParams = mParameters.GetLength();
        for (uint32 i = 0; i < numParams; ++i)
        {
            if (mParameters[i] == parameters)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // the main method to add an event, which automatically inserts it at the right sorted location
    uint32 MotionEventTrack::AddEvent(float startTimeValue, float endTimeValue, const char* eventType, const char* parameters, const char* mirrorType)
    {
        // get the event type index
        uint32 eventTypeID = GetEventManager().FindEventID(eventType);
        uint32 mirrorTypeID = GetEventManager().FindEventID(mirrorType);

        // if the event type hasn't been registered at the event manager yet, we have a problem
        if (eventTypeID == MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("MotionEventTrack::AddEvent - *** WARNING: Event type '%s' has not yet been registered at the EMotion FX event manager! Event will not be added! ***", eventType);
            return MCORE_INVALIDINDEX32;
        }

        // get the parameter index
        uint32 parameterIndex = AddParameter(parameters);

        // if there are no events yet
        if (mEvents.GetLength() == 0)
        {
            mEvents.Add(MotionEvent(startTimeValue, endTimeValue, eventTypeID, parameterIndex, mirrorTypeID));
            return 0;
        }
        else // if there are events already stored
        {
            // if we must add it at the end
            if (mEvents.GetLast().GetStartTime() <= startTimeValue)
            {
                mEvents.Add(MotionEvent(startTimeValue, endTimeValue, eventTypeID, parameterIndex, mirrorTypeID));
                return mEvents.GetLength() - 1;
            }
            else
            {
                // if we must add it at the start
                if (mEvents.GetFirst().GetStartTime() >= startTimeValue)
                {
                    mEvents.Insert(0, MotionEvent(startTimeValue, endTimeValue, eventTypeID, parameterIndex, mirrorTypeID));
                    return 0;
                }
            }

            // add the event (array sorted on time)
            uint32 insertPos = 0;
            const uint32 numEvents = mEvents.GetLength();
            for (uint32 i = 0; i < numEvents - 1; ++i)
            {
                // if this is the place we need to insert it
                if (mEvents[i].GetStartTime() <= startTimeValue && mEvents[i + 1].GetStartTime() >= startTimeValue)
                {
                    insertPos = i + 1;
                    break;
                }
            }

            // insert it
            mEvents.Insert(insertPos, MotionEvent(startTimeValue, endTimeValue, eventTypeID, parameterIndex, mirrorTypeID));
            return insertPos;
        }
    }


    // add a motion event
    uint32 MotionEventTrack::AddEvent(float startTimeValue, float endTimeValue, uint32 eventTypeID, const char* parameters, uint32 mirrorTypeID)
    {
        // find the event index in the array
        const uint32 eventIndex  = GetEventManager().FindEventTypeIndex(eventTypeID);
        if (eventIndex == MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("MotionEventTrack::AddEvent - *** WARNING: Event type with ID %d has not yet been registered at the EMotion FX event manager! Event will not be added! ***", eventTypeID);
            return MCORE_INVALIDINDEX32;
        }

        uint32 mirrorEventIndex = MCORE_INVALIDINDEX32;
        if (mirrorTypeID != MCORE_INVALIDINDEX32)
        {
            mirrorEventIndex = GetEventManager().FindEventTypeIndex(mirrorTypeID);
            if (mirrorEventIndex == MCORE_INVALIDINDEX32)
            {
                MCore::LogWarning("MotionEventTrack::AddEvent - *** WARNING: Event type with ID %d has not yet been registered at the EMotion FX event manager! Event will not be added! ***", mirrorTypeID);
            }
        }

        // get the name of the event
        const char* eventName = GetEventManager().GetEventTypeString(eventIndex);
        const char* mirrorTypeName = (mirrorEventIndex != MCORE_INVALIDINDEX32) ? GetEventManager().GetEventTypeString(mirrorEventIndex) : "";

        // add the event for real
        return AddEvent(startTimeValue, endTimeValue, eventName, parameters, mirrorTypeName);
    }


    // add the event by using a integer event ID
    uint32 MotionEventTrack::AddEvent(float timeValue, uint32 eventTypeID, const char* parameters, uint32 mirrorTypeID)
    {
        // find the event index in the array
        const uint32 eventIndex  = GetEventManager().FindEventTypeIndex(eventTypeID);
        if (eventIndex == MCORE_INVALIDINDEX32)
        {
            MCore::LogInfo("MotionEventTrack::AddEvent - *** WARNING: Event type with ID %d has not yet been registered at the EMotion FX event manager! Event will not be added! ***", eventTypeID);
            return MCORE_INVALIDINDEX32;
        }

        uint32 mirrorEventIndex = MCORE_INVALIDINDEX32;
        if (mirrorTypeID != MCORE_INVALIDINDEX32)
        {
            mirrorEventIndex = GetEventManager().FindEventTypeIndex(mirrorTypeID);
            if (mirrorEventIndex == MCORE_INVALIDINDEX32)
            {
                MCore::LogWarning("MotionEventTrack::AddEvent - *** WARNING: Event type with ID %d has not yet been registered at the EMotion FX event manager! Event will not be added! ***", mirrorTypeID);
            }
        }

        // get the name of the event
        const char* eventName = GetEventManager().GetEventTypeString(eventIndex);
        const char* mirrorTypeName = (mirrorEventIndex != MCORE_INVALIDINDEX32) ? GetEventManager().GetEventTypeString(mirrorEventIndex) : "";

        // add the event for real
        return AddEvent(timeValue, timeValue, eventName, parameters, mirrorTypeName);
    }


    // add an event (adds it sorted on time value)
    uint32 MotionEventTrack::AddEvent(float timeValue, const char* eventType, const char* parameters, const char* mirrorType)
    {
        return AddEvent(timeValue, timeValue, eventType, parameters, mirrorType);
    }


    // add a new parameter and return its index
    uint32 MotionEventTrack::AddParameter(const char* parameters)
    {
        // get the parameter index
        uint32 parameterIndex = GetParameterIndex(parameters);

        // if the parameter string isn't stored yet
        if (parameterIndex == MCORE_INVALIDINDEX32)
        {
            mParameters.Add(parameters);
            parameterIndex = mParameters.GetLength() - 1;
        }

        return parameterIndex;
    }


    // process all events within a given time range
    void MotionEventTrack::ProcessEvents(float startTime, float endTime, ActorInstance* actorInstance, MotionInstance* motionInstance)
    {
        EventInfo eventInfo;

        // TODO: optimize with hierarchical search like keyframe finder
        // if we are playing forward
        if (startTime <= endTime)
        {
            // for all events
            const uint32 numEvents = mEvents.GetLength();
            for (uint32 i = 0; i < numEvents; ++i)
            {
                // get the event start time
                const float eventStartTime = mEvents[i].GetStartTime();
                const float eventEndTime   = mEvents[i].GetEndTime();

                // we can quit processing events if its impossible that they execute
                if (eventStartTime > endTime)
                {
                    break;
                }

                // if we need to execute the event
                if (eventStartTime >= startTime && eventStartTime < endTime)
                {
                    eventInfo.mTimeValue        = eventStartTime;
                    eventInfo.mTypeID           = mEvents[i].GetEventTypeID();
                    eventInfo.mTypeString       = const_cast<AZStd::string*>(&GetEventManager().GetEventTypeStringAsString(mEvents[i].GetEventTypeID()));
                    eventInfo.mParameters       = &mParameters[mEvents[i].GetParameterIndex()];
                    eventInfo.mActorInstance    = actorInstance;
                    eventInfo.mMotionInstance   = motionInstance;
                    eventInfo.mIsEventStart     = true;
                    eventInfo.mEvent            = &mEvents[i];
                    GetEventManager().OnEvent(eventInfo);
                }

                // trigger the event end
                if (eventEndTime >= startTime && eventEndTime < endTime && mEvents[i].GetIsTickEvent() == false)
                {
                    eventInfo.mTimeValue        = eventStartTime;
                    eventInfo.mTypeID           = mEvents[i].GetEventTypeID();
                    eventInfo.mTypeString       = const_cast<AZStd::string*>(&GetEventManager().GetEventTypeStringAsString(mEvents[i].GetEventTypeID()));
                    eventInfo.mParameters       = &mParameters[mEvents[i].GetParameterIndex()];
                    eventInfo.mActorInstance    = actorInstance;
                    eventInfo.mMotionInstance   = motionInstance;
                    eventInfo.mIsEventStart     = false;
                    eventInfo.mEvent            = &mEvents[i];
                    GetEventManager().OnEvent(eventInfo);
                }
            } // for all events
        }
        else // playing backward
        {
            const uint32 numEvents = mEvents.GetLength();
            for (uint32 i = numEvents - 1; i != MCORE_INVALIDINDEX32; --i)
            {
                // get the event time
                const float eventStartTime = mEvents[i].GetStartTime();
                const float eventEndTime   = mEvents[i].GetEndTime();

                // we can quit processing events if its impossible that they execute
                if (eventEndTime < endTime)
                {
                    break;
                }

                // if we need to execute the event
                if (eventStartTime > endTime && eventStartTime <= startTime)
                {
                    eventInfo.mTimeValue        = eventStartTime;
                    eventInfo.mTypeID           = mEvents[i].GetEventTypeID();
                    eventInfo.mTypeString       = const_cast<AZStd::string*>(&GetEventManager().GetEventTypeStringAsString(mEvents[i].GetEventTypeID()));
                    eventInfo.mParameters       = &mParameters[mEvents[i].GetParameterIndex()];
                    eventInfo.mActorInstance    = motionInstance->GetActorInstance();
                    eventInfo.mMotionInstance   = motionInstance;
                    eventInfo.mIsEventStart     = false;
                    eventInfo.mEvent            = &mEvents[i];
                    GetEventManager().OnEvent(eventInfo);
                }

                // trigger the event end
                if (eventEndTime > endTime && eventEndTime <= startTime && mEvents[i].GetIsTickEvent() == false)
                {
                    eventInfo.mTimeValue        = eventStartTime;
                    eventInfo.mTypeID           = mEvents[i].GetEventTypeID();
                    eventInfo.mTypeString       = const_cast<AZStd::string*>(&GetEventManager().GetEventTypeStringAsString(mEvents[i].GetEventTypeID()));
                    eventInfo.mParameters       = &mParameters[mEvents[i].GetParameterIndex()];
                    eventInfo.mActorInstance    = motionInstance->GetActorInstance();
                    eventInfo.mMotionInstance   = motionInstance;
                    eventInfo.mIsEventStart     = true;
                    eventInfo.mEvent            = &mEvents[i];
                    GetEventManager().OnEvent(eventInfo);
                }
            } // for all events
        }
    }


    // extract events
    void MotionEventTrack::ExtractEvents(float startTime, float endTime, MotionInstance* motionInstance, AnimGraphEventBuffer* outEventBuffer)
    {
        EventInfo eventInfo;

        // TODO: optimize with hierarchical search like keyframe finder
        // if we are playing forward
        if (startTime <= endTime)
        {
            // for all events
            const uint32 numEvents = mEvents.GetLength();
            for (uint32 i = 0; i < numEvents; ++i)
            {
                // get the event start time
                const float eventStartTime = mEvents[i].GetStartTime();
                const float eventEndTime   = mEvents[i].GetEndTime();

                // we can quit processing events if its impossible that they execute
                if (eventStartTime > endTime)
                {
                    break;
                }

                // if we need to execute the event
                if (eventStartTime >= startTime && eventStartTime < endTime)
                {
                    eventInfo.mTimeValue        = eventStartTime;
                    eventInfo.mTypeID           = mEvents[i].GetEventTypeID();
                    eventInfo.mTypeString       = const_cast<AZStd::string*>(&GetEventManager().GetEventTypeStringAsString(mEvents[i].GetEventTypeID()));
                    eventInfo.mParameters       = &mParameters[mEvents[i].GetParameterIndex()];
                    eventInfo.mActorInstance    = motionInstance->GetActorInstance();
                    eventInfo.mMotionInstance   = motionInstance;
                    eventInfo.mIsEventStart     = true;
                    eventInfo.mEvent            = &mEvents[i];
                    outEventBuffer->AddEvent(eventInfo);
                }

                // trigger the event end
                if (eventEndTime >= startTime && eventEndTime < endTime && mEvents[i].GetIsTickEvent() == false)
                {
                    eventInfo.mTimeValue        = eventStartTime;
                    eventInfo.mTypeID           = mEvents[i].GetEventTypeID();
                    eventInfo.mTypeString       = const_cast<AZStd::string*>(&GetEventManager().GetEventTypeStringAsString(mEvents[i].GetEventTypeID()));
                    eventInfo.mParameters       = &mParameters[mEvents[i].GetParameterIndex()];
                    eventInfo.mActorInstance    = motionInstance->GetActorInstance();
                    eventInfo.mMotionInstance   = motionInstance;
                    eventInfo.mIsEventStart     = false;
                    eventInfo.mEvent            = &mEvents[i];
                    outEventBuffer->AddEvent(eventInfo);
                }
            } // for all events
        }
        else // playing backward
        {
            const uint32 numEvents = mEvents.GetLength();
            for (uint32 i = numEvents - 1; i != MCORE_INVALIDINDEX32; --i)
            {
                // get the event time
                const float eventStartTime = mEvents[i].GetStartTime();
                const float eventEndTime   = mEvents[i].GetEndTime();

                // we can quit processing events if its impossible that they execute
                if (eventEndTime < endTime)
                {
                    break;
                }

                // if we need to execute the event
                if (eventStartTime > endTime && eventStartTime <= startTime)
                {
                    eventInfo.mTimeValue        = eventStartTime;
                    eventInfo.mTypeID           = mEvents[i].GetEventTypeID();
                    eventInfo.mTypeString       = const_cast<AZStd::string*>(&GetEventManager().GetEventTypeStringAsString(mEvents[i].GetEventTypeID()));
                    eventInfo.mParameters       = &mParameters[mEvents[i].GetParameterIndex()];
                    eventInfo.mActorInstance    = motionInstance->GetActorInstance();
                    eventInfo.mMotionInstance   = motionInstance;
                    eventInfo.mIsEventStart     = false;
                    eventInfo.mEvent            = &mEvents[i];
                    outEventBuffer->AddEvent(eventInfo);
                }

                // trigger the event end
                if (eventEndTime > endTime && eventEndTime <= startTime && mEvents[i].GetIsTickEvent() == false)
                {
                    eventInfo.mTimeValue        = eventStartTime;
                    eventInfo.mTypeID           = mEvents[i].GetEventTypeID();
                    eventInfo.mTypeString       = const_cast<AZStd::string*>(&GetEventManager().GetEventTypeStringAsString(mEvents[i].GetEventTypeID()));
                    eventInfo.mParameters       = &mParameters[mEvents[i].GetParameterIndex()];
                    eventInfo.mActorInstance    = motionInstance->GetActorInstance();
                    eventInfo.mMotionInstance   = motionInstance;
                    eventInfo.mIsEventStart     = true;
                    eventInfo.mEvent            = &mEvents[i];
                    outEventBuffer->AddEvent(eventInfo);
                }
            } // for all events
        }
    }



    // get a given parameter string
    const char* MotionEventTrack::GetParameter(uint32 nr) const
    {
        return mParameters[nr].c_str();
    }


    // get a given parameter string
    const AZStd::string& MotionEventTrack::GetParameterString(uint32 nr) const
    {
        return mParameters[nr];
    }


    // get the number of parameter strings
    uint32 MotionEventTrack::GetNumParameters() const
    {
        return mParameters.GetLength();
    }


    // get the number of events
    uint32 MotionEventTrack::GetNumEvents() const
    {
        return mEvents.GetLength();
    }


    // remove a given event
    void MotionEventTrack::RemoveEvent(uint32 eventNr)
    {
        mEvents.Remove(eventNr);
    }


    // remove all events from the table
    void MotionEventTrack::RemoveAllEvents()
    {
        mEvents.Clear();
    }


    // remove all events and parameters from the table
    void MotionEventTrack::Clear()
    {
        RemoveAllEvents();
        mParameters.Clear();
    }


    // get the name
    const char* MotionEventTrack::GetName() const
    {
        if (mNameID == MCORE_INVALIDINDEX32)
        {
            return "";
        }

        return MCore::GetStringIdPool().GetName(mNameID).c_str();
    }


    // get the name as string object
    const AZStd::string& MotionEventTrack::GetNameString() const
    {
        if (mNameID == MCORE_INVALIDINDEX32)
        {
            return MCore::GetStringIdPool().GetName(0);
        }

        return MCore::GetStringIdPool().GetName(mNameID);
    }


    // remove all parameters
    void MotionEventTrack::RemoveAllParameters()
    {
        mParameters.Clear();
    }


    // copy the track contents to a target track
    // this overwrites all existing contents of the target track
    void MotionEventTrack::CopyTo(MotionEventTrack* targetTrack)
    {
        targetTrack->mNameID        = mNameID;
        targetTrack->mEvents        = mEvents;
        targetTrack->mParameters    = mParameters;
        targetTrack->mEnabled       = mEnabled;
    }


    // reserve memory for a given amount of events
    void MotionEventTrack::ReserveNumEvents(uint32 numEvents)
    {
        mEvents.Reserve(numEvents);
    }


    // reserve memory for a given amount of parameter strings
    void MotionEventTrack::ReserveNumParameters(uint32 numParamStrings)
    {
        mParameters.Reserve(numParamStrings);
    }


    // remove unused parameters
    void MotionEventTrack::RemoveUnusedParameters()
    {
        MCore::Array<AZStd::string> usedParameters;

        // build the new parameters table
        const uint32 numEvents = mEvents.GetLength();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            if (mEvents[i].GetParameterIndex() != MCORE_INVALIDINDEX16 && usedParameters.Contains(mEvents[i].GetParameterString(this)) == false)
            {
                usedParameters.Add(mEvents[i].GetParameterString(this));
            }
        }

        // remap the events into the new parameters table
        for (uint32 i = 0; i < numEvents; ++i)
        {
            if (mEvents[i].GetParameterIndex() != MCORE_INVALIDINDEX16)
            {
                const uint32 index = usedParameters.Find(mEvents[i].GetParameterString(this));
                MCORE_ASSERT(index != MCORE_INVALIDINDEX32);
                mEvents[i].SetParameterIndex(static_cast<uint16>(index));
            }
        }

        // update the old table with the new one
        mParameters = usedParameters;
    }


    uint32 MotionEventTrack::GetNameID() const
    {
        return mNameID;
    }


    void MotionEventTrack::SetNameID(uint32 id)
    {
        mNameID = id;
    }


    void MotionEventTrack::SetIsEnabled(bool enabled)
    {
        mEnabled = enabled;
    }


    bool MotionEventTrack::GetIsEnabled() const
    {
        return mEnabled;
    }


    bool MotionEventTrack::GetIsDeletable() const
    {
        return mDeletable;
    }


    void MotionEventTrack::SetIsDeletable(bool isDeletable)
    {
        mDeletable = isDeletable;
    }


    Motion* MotionEventTrack::GetMotion() const
    {
        return mMotion;
    }
} // namespace EMotionFX
