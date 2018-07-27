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
#include "MotionEvent.h"

#include <MCore/Source/Array.h>


namespace EMotionFX
{
    // forward declarations
    class MotionInstance;
    class ActorInstance;
    class Motion;
    class AnimGraphEventBuffer;

    /**
     * The motion event track, which stores all events and their data on a memory efficient way.
     * Events have three generic properties: a time value, an event type string and a parameter string.
     * Unique strings are only stored once in memory, so if you have for example ten events of the type "SOUND"
     * only 1 string will be stored in memory, and the events will index into the table to retrieve the string.
     * The event table can also figure out what events to process within a given time range.
     * The handling of those events is done by the MotionEventHandler class that you specify to the MotionEventManager singleton.
     */
    class EMFX_API MotionEventTrack
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL
        friend class MotionEvent;

    public:
        /**
         * Creation method.
         * @param motion The motion object this track belongs to.
         */
        static MotionEventTrack* Create(Motion* motion);

        /**
         * Extended creation.
         * @param name The name of the track.
         * @param motion The motion object this track belongs to.
         */
        static MotionEventTrack* Create(const char* name, Motion* motion);

        /**
         * Set the name of the motion event track.
         * @param name The name of the motion event track.
         */
        void SetName(const char* name);

        /**
         * Get the index inside the parameter array for a given parameter string.
         * Imagine there are 100 events in a motion all having the same parameters, for example "Footstep.wav".
         * Using this method we only store the "Footstep" string once in memory and only store indices
         * into this string array inside each event to describe its parameters.
         * @param parameters The string that contains the parameters for the event.
         * @result Returns the index number inside the array of parameter strings, or MCORE_INVALIDINDEX32 when no parameter string exists
         *         which equals the specified parameter.
         */
        uint32 GetParameterIndex(const char* parameters) const;

        /**
         * Get the parameter string of a given parameter stored in the table.
         * @param nr The parameter number, which must be in range of [0..GetNumParameters() - 1].
         * @result The string containing the parameter.
         */
        const char* GetParameter(uint32 nr) const;

        /**
         * Get the parameter string of a given parameter stored in the table.
         * @param nr The parameter number, which must be in range of [0..GetNumParameters() - 1].
         * @result The string containing the parameter.
         */
        const AZStd::string& GetParameterString(uint32 nr) const;

        /**
         * Get the number of parameter strings stored inside the table.
         * @result The number of parameter strings stored inside the table.
         * @see GetParameter
         */
        uint32 GetNumParameters() const;

        /**
         * Remove all parameters.
         */
        void RemoveAllParameters();

        /**
         * Add a new parameter to the event track in case it is not in the list yet.
         * @param parameters The parameters of the event, which could be the filename of a sound file or anything else.
         * @result Returns the index number inside the array of parameter strings, or MCORE_INVALIDINDEX32 when no parameter string exists
         *         which equals the specified parameter.
         */
        uint32 AddParameter(const char* parameters);

        /**
         * Add an event to the event table.
         * The events can be ordered in any order on time. So you do not need to add them in a sorted order based on time.
         * This is already done automatically. The events will internally be stored sorted on time value.
         * @param timeValue The time, in seconds, at which the event should occur.
         * @param eventType The string describing the type of the event, this could for example be "SOUND" or whatever your game can process.
         * @param parameters The parameters of the event, which could be the filename of a sound file or anything else.
         * @param mirrorType The mirror type, which can be like "RightFoot" for an eventType of "LeftFoot".
         * @result Returns the index to the event inside the table.
         * @note Please beware that when you use this method, the event numbers/indices might change! This is because the events are stored
         *       in an ordered way, sorted on their time value. Adding events might insert events somewhere in the middle of the array, changing all event numbers.
         */
        uint32 AddEvent(float timeValue, const char* eventType, const char* parameters, const char* mirrorType);

        /**
         * Add an event to the event table.
         * The events can be ordered in any order on time. So you do not need to add them in a sorted order based on time.
         * This is already done automatically. The events will internally be stored sorted on time value.
         * @param timeValue The time, in seconds, at which the event should occur.
         * @param eventTypeID The unique event ID that you wish to add. Please keep in mind that you need to have the events registered with the EMotion FX event manager before you can add them!
         * @param parameters The parameters of the event, which could be the filename of a sound file or anything else.
         * @param mirrorTypeID The mirror type, which can be like "RightFoot" for an eventType of "LeftFoot".
         * @result Returns the index to the event inside the table.
         * @note Please beware that when you use this method, the event numbers/indices might change! This is because the events are stored
         *       in an ordered way, sorted on their time value. Adding events might insert events somewhere in the middle of the array, changing all event numbers.
         */
        uint32 AddEvent(float timeValue, uint32 eventTypeID, const char* parameters, uint32 mirrorTypeID);

        /**
         * Add an event to the event table.
         * The events can be ordered in any order on time. So you do not need to add them in a sorted order based on time.
         * This is already done automatically. The events will internally be stored sorted on time value.
         * @param startTimeValue The start time, in seconds, at which the event should occur.
         * @param endTimeValue The end time, in seconds, at which this event should stop.
         * @param eventType The string describing the type of the event, this could for example be "SOUND" or whatever your game can process.
         * @param parameters The parameters of the event, which could be the filename of a sound file or anything else.
         * @param mirrorType The mirror type, which can be like "RightFoot" for an eventType of "LeftFoot".
         * @result Returns the index to the event inside the table.
         * @note Please beware that when you use this method, the event numbers/indices might change! This is because the events are stored
         *       in an ordered way, sorted on their time value. Adding events might insert events somewhere in the middle of the array, changing all event numbers.
         */
        uint32 AddEvent(float startTimeValue, float endTimeValue, const char* eventType, const char* parameters, const char* mirrorType);

        /**
         * Add an event to the event table.
         * The events can be ordered in any order on time. So you do not need to add them in a sorted order based on time.
         * This is already done automatically. The events will internally be stored sorted on time value.
         * @param startTimeValue The start time, in seconds, at which the event should occur.
         * @param endTimeValue The end time, in seconds, at which this event should stop.
         * @param eventTypeID The unique event ID that you wish to add. Please keep in mind that you need to have the events registered with the EMotion FX event manager before you can add them!
         * @param parameters The parameters of the event, which could be the filename of a sound file or anything else.
         * @param mirrorTypeID The mirror type, which can be like "RightFoot" for an eventType of "LeftFoot".
         * @result Returns the index to the event inside the table.
         * @note Please beware that when you use this method, the event numbers/indices might change! This is because the events are stored
         *       in an ordered way, sorted on their time value. Adding events might insert events somewhere in the middle of the array, changing all event numbers.
         */
        uint32 AddEvent(float startTimeValue, float endTimeValue, uint32 eventTypeID, const char* parameters, uint32 mirrorTypeID);

        /**
         * Process all events within a given time range.
         * @param startTime The start time of the range, in seconds.
         * @param endTime The end time of the range, in seconds.
         * @param actorInstance The actor instance on which to trigger the event.
         * @param motionInstance The motion instance which triggers the event.
         * @note The end time is also allowed to be smaller than the start time.
         */
        void ProcessEvents(float startTime, float endTime, ActorInstance* actorInstance, MotionInstance* motionInstance);

        void ExtractEvents(float startTime, float endTime, MotionInstance* motionInstance, AnimGraphEventBuffer* outEventBuffer);

        void RemoveUnusedParameters();

        /**
         * Get the number of events stored inside the table.
         * @result The number of events stored inside this table.
         * @see GetEvent
         */
        uint32 GetNumEvents() const;

        /**
         * Get a given event from the table.
         * @param eventNr The event number you wish to retrieve. This must be in range of 0..GetNumEvents() - 1.
         * @result A const reference to the event.
         */
        MCORE_INLINE const MotionEvent& GetEvent(uint32 eventNr) const          { return mEvents[eventNr]; }

        /**
         * Get a given event from the table.
         * @param eventNr The event number you wish to retrieve. This must be in range of 0..GetNumEvents() - 1.
         * @result A reference to the event.
         */
        MCORE_INLINE MotionEvent& GetEvent(uint32 eventNr)                      { return mEvents[eventNr]; }

        /**
         * Remove a given event from the table.
         * @param eventNr The event number to remove. This must be in range of 0..GetNumEvents() - 1.
         */
        void RemoveEvent(uint32 eventNr);

        /**
         * Remove all motion events from the table. Does not remove the parameters.
         */
        void RemoveAllEvents();

        /**
         * Remove all motion events and parameters from the table.
         */
        void Clear();

        const char* GetName() const;
        const AZStd::string& GetNameString() const;

        uint32 GetNameID() const;
        void SetNameID(uint32 id);
        void SetIsEnabled(bool enabled);
        bool GetIsEnabled() const;

        bool GetIsDeletable() const;
        void SetIsDeletable(bool isDeletable);

        Motion* GetMotion() const;

        void CopyTo(MotionEventTrack* targetTrack);
        void ReserveNumEvents(uint32 numEvents);
        void ReserveNumParameters(uint32 numParamStrings);

    private:
        MCore::Array< MotionEvent >     mEvents;        /**< The collection of motion events. */
        MCore::Array< AZStd::string >   mParameters;    /**< The collection of different parameters for a specific motion. */
        Motion*                         mMotion;        /**< The motion where this track belongs to. */
        uint32                          mNameID;        /**< The name ID. */
        bool                            mEnabled;       /**< Is this track enabled? */
        bool                            mDeletable;

        /**
         * The constructor.
         * @param motion The motion object this track belongs to.
         */
        MotionEventTrack(Motion* motion);

        /**
         * Extended constructor.
         * @param name The name of the track.
         * @param motion The motion object this track belongs to.
         */
        MotionEventTrack(const char* name, Motion* motion);

        /**
         * The destructor.
         */
        ~MotionEventTrack();
    };
} // namespace EMotionFX

