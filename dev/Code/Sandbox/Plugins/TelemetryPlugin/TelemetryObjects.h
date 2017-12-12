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

// Description : Structures for representing telemetry data

#ifndef CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYOBJECTS_H
#define CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYOBJECTS_H
#pragma once


//////////////////////////////////////////////////////////////////////////
struct STelemetryNode;
struct STelemetryEvent;
class CTelemetryTimeline;
class CTelemetryRepository;
//////////////////////////////////////////////////////////////////////////

enum EStatOwner
{
    eStatOwner_AI,
    eStatOwner_Player,
    eStatOwner_Player_AITeam,

    eStatOwner_Num,
};

//////////////////////////////////////////////////////////////////////////

struct STelemetryNode
{
    typedef std::map<string, string> TStates;

    STelemetryNode();
    STelemetryNode(const char* name);

    EStatOwner getType() const { return m_type; }
    const char* getName() const { return m_name.c_str(); }
    void setName(const char* name);
    const TStates& getStates() const { return m_states; }
    void addState(const char* key, const char* value) { m_states.insert(std::make_pair<string, string>(key, value)); }

    void swap(STelemetryNode& other)
    {
        std::swap(m_type, other.m_type);
        std::swap(m_name, other.m_name);
        std::swap(m_states, other.m_states);
    }

private:

    EStatOwner m_type;
    CryStackStringT<char, 64> m_name;
    TStates m_states;
};

typedef cryshared_ptr<STelemetryNode> TTelemNodePtr;

namespace std
{
    inline void swap(STelemetryNode& _Left, STelemetryNode& _Right)
    {
        _Left.swap(_Right);
    }
}

//////////////////////////////////////////////////////////////////////////

struct STelemetryEvent
{
    typedef std::map<string, string> TStates;

    uint64 timeMs;
    Vec3 position;
    CTelemetryTimeline* timeline;
    TStates states;

    STelemetryEvent()
        : timeMs(-1)
        , timeline(0)
    { }

    STelemetryEvent(int64 time, const Vec3& pos)
        : timeMs(time)
        , position(pos)
        , timeline(0)
    { }

    void swap(STelemetryEvent& other)
    {
        std::swap(timeMs, other.timeMs);
        std::swap(position, other.position);
        std::swap(timeline, other.timeline);
        std::swap(states, other.states);
    }
};

namespace std
{
    inline void swap(STelemetryEvent& _Left, STelemetryEvent& _Right)
    {
        _Left.swap(_Right);
    }
}

//////////////////////////////////////////////////////////////////////////

class CTelemetryTimeline
{
public:
    CTelemetryTimeline();
    CTelemetryTimeline(CTelemetryRepository* repo, const char* name, TTelemNodePtr owner);
    ~CTelemetryTimeline();

    void Init(CTelemetryRepository* repo, const char* name, TTelemNodePtr owner);

    void Swap(CTelemetryTimeline& other);

    void UpdateBBox();

    TTelemNodePtr getOwner() const
    {
        return m_owner;
    }

    void setOwner(TTelemNodePtr owner)
    {
        m_owner = owner;
    }

    const char* getName() const
    {
        return m_name.c_str();
    }

    void setName(const char* name)
    {
        m_name = name;
    }

    const AABB& getBBox() const
    {
        return m_bbox;
    }

    size_t getEventCount() const
    {
        return m_events.size();
    }

    void AppendEvent(STelemetryEvent& event);

    void Clear();

    const STelemetryEvent& getEvent(size_t index) const
    {
        return *m_events[index];
    }

    STelemetryEvent& getEvent(size_t index)
    {
        return *m_events[index];
    }

    STelemetryEvent** getEvents()
    {
        return &m_events[0];
    }

private:
    string m_name;
    AABB m_bbox;
    std::vector<STelemetryEvent*> m_events;
    CTelemetryRepository* m_repo;
    TTelemNodePtr m_owner;
};

//////////////////////////////////////////////////////////////////////////

namespace std
{
    inline void swap(CTelemetryTimeline& _Left, CTelemetryTimeline& _Right)
    {
        _Left.Swap(_Right);
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#endif // CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYOBJECTS_H
