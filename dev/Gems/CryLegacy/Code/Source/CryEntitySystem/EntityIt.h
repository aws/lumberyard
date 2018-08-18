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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_ENTITYIT_H
#define CRYINCLUDE_CRYENTITYSYSTEM_ENTITYIT_H

#pragma once

#include "EntitySystem.h"

struct IEntity;

class CEntityItMap
    : public IEntityIt
{
public:
    CEntityItMap(CEntitySystem* pEntitySystem)
        : m_pEntitySystem(pEntitySystem)
    {
        assert(pEntitySystem);
        m_nRefCount = 0;
        MoveFirst();
    }

    bool IsEnd()
    {
        uint32 dwMaxUsed = (uint32)m_pEntitySystem->m_EntitySaltBuffer.GetMaxUsed();

        // jump over gaps
        while (m_id <= (int)dwMaxUsed)
        {
            if (m_pEntitySystem->m_EntityArray[m_id] != 0)
            {
                return false;
            }

            ++m_id;
        }

        return m_id > (int)dwMaxUsed; // we passed the last element
    }

    IEntity* This()
    {
        if (IsEnd())     // might advance m_id
        {
            return 0;
        }
        else
        {
            return (IEntity*)m_pEntitySystem->m_EntityArray[m_id];
        }
    }

    IEntity* Next()
    {
        if (IsEnd())     // might advance m_id
        {
            return 0;
        }
        else
        {
            return (IEntity*)m_pEntitySystem->m_EntityArray[m_id++];
        }
    }

    void MoveFirst() { m_id = 0; };

    void AddRef() {m_nRefCount++; }

    void Release()
    {
        --m_nRefCount;
        if (m_nRefCount <= 0)
        {
            delete this;
        }
    }

protected: // ---------------------------------------------------

    CEntitySystem*             m_pEntitySystem;         //
    int                                     m_nRefCount;                //
    int                                     m_id;                               //
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_ENTITYIT_H
