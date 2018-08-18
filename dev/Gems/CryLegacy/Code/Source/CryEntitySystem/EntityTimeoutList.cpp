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

#include "CryLegacy_precompiled.h"
#include "EntityTimeoutList.h"
#include "ITimer.h"
#include "EntityCVars.h"

// To be as efficient as possible this class uses funky linked list tricks. Instead of having
// head and tail members in the class, the head and tail indices are stored in the first element
// of the vector. The linked list superficially is therefore a circular list, in which the tail
// links back to the head and vice versa. Initially there is only one node which wraps around
// to itself. This setup allows insertion and deletion code to be faster, since there are no
// special cases when the node being inserted/removed is at the beginning/end. Note that this
// zero element is, logically speaking, not part of the list, but is only there as part of the
// linked list mechanics.

CEntityTimeoutList::CEntityTimeoutList(ITimer* pTimer)
    :   m_pTimer(pTimer)
{
    Clear();

    if (!m_pTimer)
    {
        CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "CEntityTimeoutList::CEntityTimeoutList() - pTimer = 0: Not seen timeout will not work, resulting in wasted memory.");
    }
}

void CEntityTimeoutList::ResetTimeout(EntityId id)
{
    if (id)
    {
        int index = IdToHandle(id).GetIndex() + 1;

        if (int(m_entries.size()) <= index)
        {
            bool wasEmpty = m_entries.empty();

            m_entries.resize(index + 1);

            if (wasEmpty)
            {
                m_entries[0].m_next = 0;
                m_entries[0].m_prev = 0;
            }

            if (CVar::pDebugNotSeenTimeout->GetIVal())
            {
                CryLogAlways("CEntityTimeoutList::ResetTimeout() - resizing entries (total memory used = %d)", int(m_entries.size() * sizeof(CEntry)));
            }
        }

        CEntry& entry = m_entries[index];
        int next = entry.m_next;
        int prev = entry.m_prev;

        if (next == -1 && CVar::pDebugNotSeenTimeout->GetIVal())
        {
            CryLogAlways("EntityId (0x%X) added to not seen timeout list", id);
        }

        if (next >= 0)
        {
            m_entries[next].m_prev = prev;
        }
        if (prev >= 0)
        {
            m_entries[prev].m_next = next;
        }

        CEntry& bookKeepingEntry = m_entries[0];
        int tail = bookKeepingEntry.m_prev;
        bookKeepingEntry.m_prev = index;

        entry.m_next = 0;
        entry.m_prev = tail;
        entry.m_id = id;
        entry.m_time = (m_pTimer ? m_pTimer->GetCurrTime() : 0.0f);

        m_entries[tail].m_next = index;
    }
}

EntityId CEntityTimeoutList::PopTimeoutEntity(float timeout)
{
    if (m_entries.empty() || m_entries[0].m_next == 0)
    {
        return 0;
    }

    float time = (m_pTimer ? m_pTimer->GetCurrTime() : 0.0f);
    int index = m_entries[0].m_next;
    if (m_entries[index].m_time + timeout > time)
    {
        return 0;
    }

    int next = m_entries[index].m_next;
    int prev = m_entries[index].m_prev;
    if (next >= 0)
    {
        m_entries[next].m_prev = prev;
    }
    if (prev >= 0)
    {
        m_entries[prev].m_next = next;
    }
    m_entries[index].m_next = -1;
    m_entries[index].m_prev = -1;

    return m_entries[index].m_id;
}

void CEntityTimeoutList::Clear()
{
    stl::free_container(m_entries);
}
