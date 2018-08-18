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


#include "CryLegacy_precompiled.h"
#include "TimeOfDayScheduler.h"
#include "ITimeOfDay.h"

CTimeOfDayScheduler::CTimeOfDayScheduler()
{
    m_nextId = 0;
    m_lastTime = 0.0f;
    m_bForceUpdate = true;
}

CTimeOfDayScheduler::~CTimeOfDayScheduler()
{
}

void CTimeOfDayScheduler::Reset()
{
    stl::free_container(m_entries);
    m_nextId = 0;
    m_lastTime = gEnv->p3DEngine->GetTimeOfDay()->GetTime();
    m_bForceUpdate = true;
}

CTimeOfDayScheduler::TimeOfDayTimerId CTimeOfDayScheduler::AddTimer(float time, CTimeOfDayScheduler::TimeOfDayTimerCallback callback, void* pUserData)
{
    assert (callback != 0);
    if (callback == 0)
    {
        return InvalidTimerId;
    }
    SEntry entryForTime(0, time, 0, 0);
    TEntries::iterator iter = std::lower_bound(m_entries.begin(), m_entries.end(), entryForTime);
    m_entries.insert(iter, SEntry(m_nextId, time, callback, pUserData));
    return m_nextId++;
}

void* CTimeOfDayScheduler::RemoveTimer(TimeOfDayTimerId id)
{
    TEntries::iterator iter = std::find(m_entries.begin(), m_entries.end(), id);
    if (iter == m_entries.end())
    {
        return 0;
    }
    void* pUserData = iter->pUserData;
    m_entries.erase(iter);
    return pUserData;
}

void CTimeOfDayScheduler::Update()
{
    static const float MIN_DT = 1.0f / 100.0f;

    float curTime = gEnv->p3DEngine->GetTimeOfDay()->GetTime();
    float lastTime = m_lastTime;
    const float dt = curTime - lastTime;

    // only evaluate if at least some time passed
    if (m_bForceUpdate == false && fabs_tpl(dt) < MIN_DT)
    {
        return;
    }
    m_bForceUpdate = false;

    // execute all entries between lastTime and curTime
    // if curTime < lastTime, we wrapped around and have to process two intervals
    // [lastTime, 24.0] and [0, curTime]
    TEntries processingEntries; // no need to make member var, as allocation/execution currently is NOT too often
    SEntry entryForTime(0, lastTime, 0, 0);
    TEntries::iterator iter = std::lower_bound(m_entries.begin(), m_entries.end(), entryForTime);
    TEntries::iterator iterEnd = m_entries.end();
    const bool bWrap = lastTime > curTime;
    const float maxTime = bWrap ? 24.0f : curTime;

    //CryLogAlways("CTOD: lastTime=%f curTime=%f", lastTime, curTime);

    // process interval [lastTime, curTime] or in case of wrap [lastTime, 24.0]
    while (iter != iterEnd)
    {
        const SEntry& entry = *iter;
        assert (entry.time >= lastTime);
        if (entry.time > maxTime)
        {
            break;
        }
        // CryLogAlways("Adding: %d time=%f", entry.id, entry.time);
        processingEntries.push_back(entry);
        ++iter;
    }

    if (bWrap) // process interval [0, curTime]
    {
        iter = m_entries.begin();
        while (iter != iterEnd)
        {
            const SEntry& entry = *iter;
            if (entry.time > curTime)
            {
                break;
            }
            // CryLogAlways("Adding[wrap]: %d time=%f", entry.id, entry.time);
            processingEntries.push_back(entry);
            ++iter;
        }
    }

    iter = processingEntries.begin();
    iterEnd = processingEntries.end();
    while (iter != iterEnd)
    {
        const SEntry& entry = *iter;
        entry.callback(entry.id, entry.pUserData, curTime);
        ++iter;
    }

    m_lastTime = curTime;
}
