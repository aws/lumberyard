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
#include "MotionEvent.h"
#include "MotionEventTrack.h"
#include "EventManager.h"
#include "EventHandler.h"

#include <MCore/Source/Compare.h>


namespace EMotionFX
{
    // default constructor
    MotionEvent::MotionEvent()
    {
        mStartTime      = 0.0f;
        mEndTime        = 0.0f;
        mParameterIndex = MCORE_INVALIDINDEX16;
        mEventTypeID    = MCORE_INVALIDINDEX32;
        mMirrorTypeID   = MCORE_INVALIDINDEX32;
    }


    // extended constructor
    MotionEvent::MotionEvent(float timeValue, uint32 eventTypeID, uint32 parameterIndex, uint32 mirrorTypeID)
    {
        mStartTime      = timeValue;
        mEndTime        = timeValue;
        mParameterIndex = static_cast<uint16>(parameterIndex);
        mEventTypeID    = eventTypeID;
        mMirrorTypeID   = mirrorTypeID;
    }


    // extended constructor
    MotionEvent::MotionEvent(float startTimeValue, float endTimeValue, uint32 eventTypeID, uint32 parameterIndex, uint32 mirrorTypeID)
    {
        mStartTime          = startTimeValue;
        mEndTime            = endTimeValue;
        mParameterIndex     = static_cast<uint16>(parameterIndex);
        mEventTypeID        = eventTypeID;
        mMirrorTypeID       = mirrorTypeID;
    }



    // destructor
    MotionEvent::~MotionEvent()
    {
    }


    // retrieve the event type string
    const char* MotionEvent::GetEventTypeString() const
    {
        // check if we use a registered event type
        if (mEventTypeID == MCORE_INVALIDINDEX32)
        {
            return "";
        }

        // try to locate the registered event type index
        uint32 eventIndex = GetEventManager().FindEventTypeIndex(mEventTypeID);
        if (eventIndex == MCORE_INVALIDINDEX32)
        {
            return "";
        }

        // return the string linked to this registered event type
        return GetEventManager().GetEventTypeString(eventIndex);
    }


    // retrieve the mirror event type string
    const char* MotionEvent::GetMirrorEventTypeString() const
    {
        // check if we use a registered event type
        if (mMirrorTypeID == MCORE_INVALIDINDEX32)
        {
            return "";
        }

        // try to locate the registered event type index
        uint32 eventIndex = GetEventManager().FindEventTypeIndex(mMirrorTypeID);
        if (eventIndex == MCORE_INVALIDINDEX32)
        {
            return "";
        }

        // return the string linked to this registered event type
        return GetEventManager().GetEventTypeString(eventIndex);
    }


    // get the mirror type ID
    uint32 MotionEvent::GetMirrorEventID() const
    {
        return mMirrorTypeID;
    }


    // get the event index
    uint32 MotionEvent::GetMirrorEventIndex() const
    {
        if (mMirrorTypeID != MCORE_INVALIDINDEX32)
        {
            return GetEventManager().FindEventTypeIndex(mMirrorTypeID);
        }

        return MCORE_INVALIDINDEX32;
    }


    // set the mirror event ID
    void MotionEvent::SetMirrorEventID(uint32 id)
    {
        mMirrorTypeID = id;
    }


    // retrieve the parameter string
    const AZStd::string& MotionEvent::GetParameterString(MotionEventTrack* eventTrack) const
    {
        return eventTrack->GetParameterString(mParameterIndex);
    }


    // get the event type index
    uint32 MotionEvent::GetEventTypeID() const
    {
        return mEventTypeID;
    }


    // get the parameter index
    uint16 MotionEvent::GetParameterIndex() const
    {
        return mParameterIndex;
    }


    // set the start time value
    void MotionEvent::SetStartTime(float timeValue)
    {
        mStartTime = timeValue;
    }


    // set the end time value
    void MotionEvent::SetEndTime(float timeValue)
    {
        mEndTime = timeValue;
    }


    // set the event type index
    void MotionEvent::SetEventTypeID(uint32 eventID)
    {
        mEventTypeID = eventID;
    }


    // set the parameter index
    void MotionEvent::SetParameterIndex(uint16 index)
    {
        mParameterIndex = index;
    }


    // update the mEventTypeID value
    void MotionEvent::UpdateEventTypeID(const char* eventTypeString)
    {
        mEventTypeID = GetEventManager().FindEventID(eventTypeString);
        MCORE_ASSERT(mEventTypeID != MCORE_INVALIDINDEX32);
    }


    // is this a tick event?
    bool MotionEvent::GetIsTickEvent() const
    {
        return MCore::Compare<float>::CheckIfIsClose(mStartTime, mEndTime, MCore::Math::epsilon);
    }


    // toggle tick event on or off
    void MotionEvent::ConvertToTickEvent()
    {
        mEndTime = mStartTime;
    }
}   // namespace EMotionFX
