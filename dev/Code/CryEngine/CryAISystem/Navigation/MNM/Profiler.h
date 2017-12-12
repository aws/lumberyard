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

#ifndef CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_PROFILER_H
#define CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_PROFILER_H
#pragma once


namespace MNM
{
    template<typename MemoryUsers, typename TimerNames = int, typename StatNames = int>
    struct Profiler
    {
        enum
        {
            MaxTimers = 32,
        };
        enum
        {
            MaxUsers = 32,
        };
        enum
        {
            MaxStats = 32,
        };

        struct TimerInfo
        {
            TimerInfo()
                : elapsed(0ll)
            {
            }

            CTimeValue elapsed;
        };

        struct MemoryInfo
        {
            MemoryInfo()
                : used(0)
                , peak(0)
            {
            }

            size_t used;
            size_t peak;
        };


        Profiler()
            : elapsed(0ll)
            , memoryUsed(0)
            , memoryPeak(0)
        {
            memset(&runningTimer[0], 0, sizeof(CTimeValue) * MaxTimers);
            memset(&stats[0], 0, sizeof(int) * MaxStats);
        }

        void AddMemory(MemoryUsers user, size_t amount)
        {
            memoryUsed += amount;
            if (memoryUsed > memoryPeak)
            {
                memoryPeak = memoryUsed;
            }

            memoryUsage[user].used += amount;
            if (memoryUsage[user].used > memoryUsage[user].peak)
            {
                memoryUsage[user].peak = memoryUsage[user].used;
            }
        }

        void FreeMemory(MemoryUsers user, size_t amount)
        {
            memoryUsed -= amount;
            memoryUsage[user].used -= amount;
        }

        inline void StartTimer(TimerNames timer)
        {
            runningTimer[timer] = gEnv->pTimer->GetAsyncTime();
        }

        inline void StopTimer(TimerNames timer)
        {
            CTimeValue end = gEnv->pTimer->GetAsyncTime();
            assert(runningTimer[timer].GetValue() != 0);

            CTimeValue timerElapsed = end - runningTimer[timer];
            timers[timer].elapsed += timerElapsed;

            elapsed += timerElapsed;
        }

        inline void AddTime(TimerNames timer, CTimeValue amount)
        {
            elapsed += amount;
            timers[timer].elapsed += amount;
        }

        inline void AddStat(StatNames stat, int amount)
        {
            stats[stat] += amount;
        }

        inline const MemoryInfo& operator[](MemoryUsers user) const
        {
            return memoryUsage[user];
        }

        inline const TimerInfo& operator[](TimerNames timer) const
        {
            return timers[timer];
        }

        inline int operator[](StatNames stat) const
        {
            return stats[stat];
        }

        inline const CTimeValue& GetTotalElapsed() const
        {
            return elapsed;
        }

        inline size_t GetMemoryUsage() const
        {
            return memoryUsed;
        }

        inline size_t GetMemoryPeak() const
        {
            return memoryPeak;
        }

    private:
        CTimeValue elapsed;
        size_t memoryUsed;
        size_t memoryPeak;

        TimerInfo timers[MaxTimers];
        MemoryInfo memoryUsage[MaxUsers];
        int stats[MaxStats];

        CTimeValue runningTimer[MaxTimers];
    };
}

#endif // CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_PROFILER_H
