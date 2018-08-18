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

#ifndef CRYINCLUDE_CRYACTION_CALLBACKTIMER_H
#define CRYINCLUDE_CRYACTION_CALLBACKTIMER_H
#pragma once


#include <IDMap.h>


class CTimeValue;
class CallbackTimer
{
public:
    CallbackTimer();

    typedef uint32 TimerID;
    typedef Functor2<void*, TimerID> Callback;

    void Clear();
    void Update();

    TimerID AddTimer(CTimeValue interval, bool repeating, const Callback& callback, void* userdata = 0);
    void* RemoveTimer(TimerID timerID);

    void GetMemoryStatistics(ICrySizer* s);
private:
    struct TimerInfo
    {
        TimerInfo(CTimeValue _interval, bool _repeating, const Callback& _callback, void* _userdata)
            : interval(_interval)
            , repeating(_repeating)
            , callback(_callback)
            , userdata(_userdata)
        {
        }

        CTimeValue interval;
        Callback callback;
        void* userdata;
        bool repeating;
    };

    typedef id_map<size_t, TimerInfo> Timers;

    struct TimeoutInfo
    {
        TimeoutInfo(CTimeValue _timeout, TimerID _timerID)
            : timeout(_timeout)
            , timerID(_timerID)
        {
        }

        CTimeValue timeout;
        TimerID timerID;

        ILINE bool operator<(const TimeoutInfo& other) const
        {
            return timeout < other.timeout;
        }
    };
    typedef std::deque<TimeoutInfo> Timeouts;

    Timers m_timers;
    Timeouts m_timeouts;
    bool m_resort;
};


#endif // CRYINCLUDE_CRYACTION_CALLBACKTIMER_H
