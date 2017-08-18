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

#ifndef CRYINCLUDE_CRYAISYSTEM_ACTORLOOKUP_H
#define CRYINCLUDE_CRYAISYSTEM_ACTORLOOKUP_H
#pragma once


#include "AIActor.h"


template<typename Ty>
struct ActorLookUpCastHelper
{
    static Ty* Cast(CAIActor* ptr)  {   assert(!"dangerous cast!"); return static_cast<Ty*>(ptr); }
};

template<>
struct ActorLookUpCastHelper<CAIObject>
{
    static CAIObject* Cast(CAIActor* ptr) { return static_cast<CAIObject*>(ptr); }
};

template<>
struct ActorLookUpCastHelper<CAIActor>
{
    static CAIActor* Cast(CAIActor* ptr) { return ptr; }
};

template<>
struct ActorLookUpCastHelper<CPipeUser>
{
    static CPipeUser* Cast(CAIActor* ptr) { return ptr->CastToCPipeUser(); }
};

template<>
struct ActorLookUpCastHelper<CPuppet>
{
    static CPuppet* Cast(CAIActor* ptr) { return ptr->CastToCPuppet(); }
};

class ActorLookUp
{
public:
    ILINE size_t GetActiveCount() const
    {
        return m_actors.size();
    }

    template<typename Ty>
    ILINE Ty* GetActor(uint32 index) const
    {
        if (index < m_actors.size())
        {
            if (CAIActor* actor = m_actors[index])
            {
                return ActorLookUpCastHelper<Ty>::Cast(actor);
            }
        }

        return 0;
    }

    ILINE IAIActorProxy* GetProxy(uint32 index) const
    {
        return m_proxies[index];
    }

    ILINE const Vec3& GetPosition(uint32 index) const
    {
        return m_positions[index];
    }

    ILINE EntityId GetEntityID(uint32 index) const
    {
        return m_entityIDs[index];
    }

    ILINE void UpdatePosition(CAIActor* actor, const Vec3& position)
    {
        size_t activeActorCount = GetActiveCount();

        for (size_t i = 0; i < activeActorCount; ++i)
        {
            if (m_actors[i] == actor)
            {
                m_positions[i] = position;
                return;
            }
        }
    }

    ILINE void UpdateProxy(CAIActor* actor)
    {
        size_t activeActorCount = GetActiveCount();

        for (size_t i = 0; i < activeActorCount; ++i)
        {
            if (m_actors[i] == actor)
            {
                m_proxies[i] = actor->GetProxy();
                return;
            }
        }
    }

    enum
    {
        Proxy           = BIT(0),
        Position    = BIT(1),
        EntityID    = BIT(2),
    };

    ILINE void Prepare(uint32 lookUpFields)
    {
        if (!m_actors.empty())
        {
            CryPrefetch(&m_actors[0]);

            if (lookUpFields & Proxy)
            {
                CryPrefetch(&m_proxies[0]);
            }

            if (lookUpFields & Position)
            {
                CryPrefetch(&m_positions[0]);
            }

            if (lookUpFields & EntityID)
            {
                CryPrefetch(&m_entityIDs[0]);
            }
        }
    }

    void AddActor(CAIActor* actor)
    {
        assert(actor);

        size_t activeActorCount = GetActiveCount();

        for (uint32 i = 0; i < activeActorCount; ++i)
        {
            if (m_actors[i] == actor)
            {
                m_proxies[i] = actor->GetProxy();
                m_positions[i] = actor->GetPos();
                m_entityIDs[i] = actor->GetEntityID();

                return;
            }
        }

        m_actors.push_back(actor);
        m_proxies.push_back(actor->GetProxy());
        m_positions.push_back(actor->GetPos());
        m_entityIDs.push_back(actor->GetEntityID());
    }

    void RemoveActor(CAIActor* actor)
    {
        assert(actor);

        size_t activeActorCount = GetActiveCount();

        for (size_t i = 0; i < activeActorCount; ++i)
        {
            if (m_actors[i] == actor)
            {
                std::swap(m_actors[i], m_actors.back());
                m_actors.pop_back();

                std::swap(m_proxies[i], m_proxies.back());
                m_proxies.pop_back();

                std::swap(m_positions[i], m_positions.back());
                m_positions.pop_back();

                std::swap(m_entityIDs[i], m_entityIDs.back());
                m_entityIDs.pop_back();

                return;
            }
        }
    }

private:
    typedef std::vector<CAIActor*> Actors;
    Actors m_actors;

    typedef std::vector<IAIActorProxy*> Proxies;
    Proxies m_proxies;

    typedef std::vector<Vec3> Positions;
    Positions m_positions;

    typedef std::vector<EntityId> EntityIDs;
    EntityIDs m_entityIDs;
};


#endif // CRYINCLUDE_CRYAISYSTEM_ACTORLOOKUP_H
