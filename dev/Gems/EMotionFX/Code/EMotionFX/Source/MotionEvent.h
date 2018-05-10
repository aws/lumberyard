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
#include <AzCore/std/string/string.h>

namespace EMotionFX
{
    // forward declarations
    class MotionEventTrack;
    class MotionInstance;
    class ActorInstance;

    /**
     * A motion event, which describes an event that happens at a given time in a motion.
     * This could be a footstep sound that needs to be played, or a particle system that needs to be spawened
     * or a script that needs to be executed. Motion events are completely generic, which means EMotion FX does not
     * handle the events for you. It is up to you how you handle the events. Also we do not specify any set of available events.
     * Each event can be identified with an event type string, which could for example be "SOUND" and a parameter string, for example "Footstep.wav".
     * It is up to you to parse these strings and perform the required actions.
     * All motion events are stored in a motion event table. This table contains the string data for the event types and parameters, which can be shared
     * between events. This will mean if you have 100 events that contain the strings "SOUND" and "Footstep.wav", those strings will only be stored in memory once.
     * To manually add motion events to a motion, do something like this:
     *
     * <pre>
     * motion->GetEventTable().AddEvent(0.0f, "SOUND",   "Footstep.wav");
     * motion->GetEventTable().AddEvent(3.0f, "SCRIPT",  "OpenDoor.script");
     * motion->GetEventTable().AddEvent(7.0f, "SOUND",   "Footstep.wav");
     * </pre>
     *
     * However, please keep in mind that you first need to register your event types (such as "SOUND") before you are allowed to add them to the event tables!
     * You can do this on the following way:<br>
     *
     * <pre>
     * EMFX_EVENTMGR.RegisterEvent("SOUND",  100);  // 100 is here some integer value that represents your unique event type ID that is passed to the ProcessEvent method of your event handler.
     * EMFX_EVENTMGR.RegisterEvent("SCRIPT", 101);  // 101 is another unique number, this can again be anything you like, as long as no other registered event types use it.
     * </pre>
     *
     * It is best to register your events to the event manager during initialization of your game.
     *
     * To create your own event handler, inherit a new class from MotionEventHandler and overload the ProcessEvent method.
     * Then use EMFX_EVENTMGR.SetMotionEventHandler( yourHandler ) to use your handler instead of GetEMotionFX()'s default one which only logs the events.
     */
    class EMFX_API MotionEvent
    {
        MCORE_MEMORYOBJECTCATEGORY(MotionEvent, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_EVENTS);

    public:
        /**
         * The default constructor.
         */
        MotionEvent();

        /**
         * Extended constructor.
         * @param timeValue The time value, in seconds, when the motion event should occur.
         * @param eventTypeID The ID of the motion event.
         * @param parameterIndex The index into the event parameter array of the event table where this event will be added to.
         * @param mirrorTypeID The mirror event type ID.
         */
        MotionEvent(float timeValue, uint32 eventTypeID, uint32 parameterIndex, uint32 mirrorTypeID = MCORE_INVALIDINDEX32);

        /**
         * Extended constructor.
         * @param startTimeValue The start time value, in seconds, when the motion event should start.
         * @param endTimeValue The end time value, in seconds, when the motion event should end. When this is equal to the start time value we won't trigger an end event, but only a start event at the specified time.
         * @param eventTypeID The index into the event type array of the event table where this event will be added to.
         * @param parameterID The index into the event parameter array of the event table where this event will be added to.
         * @param mirrorTypeID The mirror event type ID.
         */
        MotionEvent(float startTimeValue, float endTimeValue, uint32 eventTypeID, uint32 parameterIndex, uint32 mirrorTypeID = MCORE_INVALIDINDEX32);

        /**
         * The destructor.
         */
        ~MotionEvent();

        /**
         * Get the event type ID, which is a unique integer ID number.
         * @result The event type ID, or MCORE_INVALIDINDEX32 when the event ID couldn't be identified when the event was added to the event table.
         */
        uint32 GetEventTypeID() const;

        /**
         * Get the parameter index, which is an index inside the array of stored parameter strings inside a motion event table.
         * @result The index into the parameter string array inside the motion event table.
         */
        uint16 GetParameterIndex() const;

        /**
         * Get the event type string for this event. This will return an empty string when the event type used
         * hasn't been registered at the EMotion FX event manager.
         * @result The string which describes the motion event type, for example "SOUND", or an empty string "" in case there is registered event using this event type ID.
         */
        const char* GetEventTypeString() const;

        const char* GetMirrorEventTypeString() const;
        uint32 GetMirrorEventID() const;
        uint32 GetMirrorEventIndex() const;
        void SetMirrorEventID(uint32 id);

        /**
         * Get the parameter string for this event.
         * @param eventTrack The motion event track where this event belongs to.
         * @result The string which contains the parameters for this event, for example "Footstep.wav".
         */
        const AZStd::string& GetParameterString(MotionEventTrack* eventTrack) const;

        /**
         * Set the start time value of the event, which is when the event should be processed.
         * @param timeValue The time on which the event shall be processed, in seconds.
         */
        void SetStartTime(float timeValue);

        /**
         * Set the end time value of the event, which is when the event should be processed.
         * @param timeValue The time on which the event shall be processed, in seconds.
         */
        void SetEndTime(float timeValue);

        /**
         * Set the event type ID.
         * @param eventID The unique event ID number.
         */
        void SetEventTypeID(uint32 eventID);

        /**
         * Set the parameter index, which points inside the array of parameter strings of the motion table.
         * @param index The index value which points inside the array of parameter strings.
         */
        void SetParameterIndex(uint16 index);

        /**
         * Update the event type ID of this motion event.
         * An event type ID is a unique integer value that represents the type of event.
         * Each event type ID is linked to a given string as well, for example "SOUND".
         * This method will try to find the ID that you specific when registering event types to the EMotion FX event manager.
         * It will try to locate the ID for the event type string (for example "SOUND") that you pass.
         *
         * An example: <br>
         * <pre>
         * // somewhere during initialization of your game or engine you registered the SOUND event and linked it to
         * // the event type integer value 100.
         * EMFX_EVENTMGR.RegisterEventType("SOUND", 100);
         *
         * // now later you wish to turn a given event into a sound event
         * event->UpdateEventTypeID("SOUND");
         *
         * // now the event type ID stored inside this event will contain a value of 100.
         *
         * </pre>
         *
         * @param eventTypeString The event type string.
         * @note A value of MCORE_INVALIDINDEX32 will be assigned to the event type when the eventTypeString hasn't been registered with the event manager.
         */
        void UpdateEventTypeID(const char* eventTypeString);

        /**
         * Get the start time value of this event, which is when it should be executed.
         * @result The start time, in seconds, on which the event should be executed/processed.
         */
        MCORE_INLINE float GetStartTime() const                         { return mStartTime; }

        /**
         * Get the end time value of this event, which is when it should stop.
         * @result The end time, in seconds, on which the event should be stop.
         */
        MCORE_INLINE float GetEndTime() const                           { return mEndTime; }

        /**
         * Check whether this is a tick event or not.
         * It is a tick event when both start and end time are equal.
         * @result Returns true when start and end time are equal, otherwise false is returned.
         */
        bool GetIsTickEvent() const;

        /**
         * Convert this event into a tick event.
         * This makes the end time equal to the start time.
         */
        void ConvertToTickEvent();

    private:
        float               mStartTime;         /**< The time value, in seconds, when the event start should be triggered. */
        float               mEndTime;           /**< The time value, in seconds, when the event end should be triggered. */
        uint32              mEventTypeID;       /**< The event type ID, which points inside the EMFX_EVENTMGR collection of registered event types. */
        uint32              mMirrorTypeID;      /**< The mirrored type ID. */
        uint16              mParameterIndex;    /**< An index inside the array of parameter strings inside the event table where this event is added to. */
    };
} // namespace EMotionFX
