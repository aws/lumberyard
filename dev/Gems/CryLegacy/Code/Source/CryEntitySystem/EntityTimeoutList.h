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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_ENTITYTIMEOUTLIST_H
#define CRYINCLUDE_CRYENTITYSYSTEM_ENTITYTIMEOUTLIST_H
#pragma once


#include <vector>
#include "SaltHandle.h"

typedef unsigned int EntityId; // Copied from IEntity.h

class CEntityTimeoutList
{
public:
    CEntityTimeoutList(ITimer* pTimer);

    void ResetTimeout(EntityId id);
    EntityId PopTimeoutEntity(float timeout);
    void Clear();

private:
    static ILINE CSaltHandle<> IdToHandle(const EntityId id) {return CSaltHandle<>(id >> 16, id & 0xffff); }

    class CEntry
    {
    public:
        CEntry()
            : m_id(0)
            , m_time(0.0f)
            , m_next(-1)
            , m_prev(-1) {}

        EntityId m_id;
        float m_time;
        int m_next;
        int m_prev;
    };

    typedef DynArray<CEntry> EntryContainer;
    EntryContainer m_entries;
    ITimer* m_pTimer;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_ENTITYTIMEOUTLIST_H
