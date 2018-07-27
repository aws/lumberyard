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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : TimeOfDayScheduler


#ifndef CRYINCLUDE_CRYACTION_TIMEOFDAYSCHEDULER_H
#define CRYINCLUDE_CRYACTION_TIMEOFDAYSCHEDULER_H
#pragma once

class CTimeOfDayScheduler
{
public:
    // timer id
    typedef int TimeOfDayTimerId;
    static const int InvalidTimerId = -1;

    // timer id, user data, current time of day (not scheduled time of day!)
    typedef void (* TimeOfDayTimerCallback)(TimeOfDayTimerId, void*, float);

    CTimeOfDayScheduler();
    ~CTimeOfDayScheduler();

    void Reset();  // clears all scheduled timers
    void Update(); // updates (should be called every frame, internally updates in intervalls)

    void GetMemoryStatistics(ICrySizer* s)
    {
        s->Add(*this);
        s->AddContainer(m_entries);
    }

    TimeOfDayTimerId AddTimer(float time, TimeOfDayTimerCallback callback, void* pUserData);
    void* RemoveTimer(TimeOfDayTimerId id); // returns user data

protected:
    struct SEntry
    {
        SEntry(TimeOfDayTimerId _id, float _time, TimeOfDayTimerCallback _callback, void* _pUserData)
        {
            this->id = _id;
            this->time = _time;
            this->callback = _callback;
            this->pUserData = _pUserData;
        }

        bool operator==(const TimeOfDayTimerId& otherId) const
        {
            return id == otherId;
        }

        bool operator<(const SEntry& other) const
        {
            return time < other.time;
        }

        bool operator<(const float& otherTime) const
        {
            return time < otherTime;
        }

        TimeOfDayTimerId id;                // 4 bytes
        float time;                         // 4 bytes
        TimeOfDayTimerCallback callback;    // 4/8 bytes
        void* pUserData;                    // 4/8 bytes
        //                                   = 32/48 bytes
    };

    typedef std::vector<SEntry> TEntries;
    TEntries m_entries;
    TimeOfDayTimerId m_nextId;
    float m_lastTime;
    bool m_bForceUpdate;
};

#endif // CRYINCLUDE_CRYACTION_TIMEOFDAYSCHEDULER_H
