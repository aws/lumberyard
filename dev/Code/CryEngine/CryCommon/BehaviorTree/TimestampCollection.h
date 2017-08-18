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

#ifndef TimestampCollection_h
#define TimestampCollection_h

#pragma once

#if defined(USING_BEHAVIOR_TREE_TIMESTAMP_DEBUGGING) || defined(USING_BEHAVIOR_TREE_EDITOR)
#define STORE_EXTRA_TIMESTAMP_DATA
#endif

#if defined(USING_BEHAVIOR_TREE_SERIALIZATION)
#include <Serialization/StringList.h>
#endif


namespace BehaviorTree
{
    // Unique identifier for a timestamp. (The CRC of the timestamp name.)
    struct TimestampID
    {
        TimestampID()
            : id(0)
        {
        }

        TimestampID(const char* name)
        {
#ifdef STORE_EXTRA_TIMESTAMP_DATA
            timestampName = name;
#endif

            id = CCrc32::Compute(name);
        }

        bool operator == (const TimestampID& rhs) const
        {
            return id == rhs.id;
        }

        operator bool () const
        {
            return id != 0;
        }

#ifdef STORE_EXTRA_TIMESTAMP_DATA
        string timestampName;
#endif

        unsigned int id;
    };

    struct Timestamp;
    typedef std::vector<Timestamp> Timestamps;

    // A timestamp is a small piece of information on when a certain event
    // occurred. For example, this is useful if you want to query when
    // a character was last shot at or hit, or maybe when he last could
    // see the player. Or when he last moved to a new cover location.
    //
    // Optional:
    // Timestamps can be declared as exclusive to each other. For example,
    // <EnemySeenTime> could be said to be exclusive to <EnemyLostTime>.
    // You can't see the player and at the same time consider him lost.
    struct Timestamp
    {
        Timestamp()
            : hasBeenSetAtLeastOnce(false)
        {
            Invalidate();
        }

        TimestampID id;
        TimestampID exclusiveTo;
        CTimeValue time;
        uint32 setOnEventNameCRC32;
        bool hasBeenSetAtLeastOnce;

#if defined(STORE_EXTRA_TIMESTAMP_DATA)
        string setOnEventName;
#endif

        bool IsValid() const
        {
            return time.GetMilliSecondsAsInt64() >= 0;
        }

        void Invalidate()
        {
            time = CTimeValue(-1.0f);
        }

#if defined (USING_BEHAVIOR_TREE_SERIALIZATION)
        void Serialize(Serialization::IArchive& archive)
        {
            archive(id.timestampName, "name", "^<Name");
            archive(setOnEventName, "setOnEventName", "^<Set on event");

            if (id.timestampName.empty())
            {
                archive.Error(id.timestampName, "Name must be specified");
            }

            if (setOnEventName.empty())
            {
                archive.Error(setOnEventName, "Event must be specified");
            }

            bool refenreceFound = false;

            Serialization::StringList exclusiveToTimestampList;
            exclusiveToTimestampList.push_back("");

            Timestamps* timestamps = archive.FindContext<Timestamps>();
            for (Timestamps::const_iterator it = timestamps->begin(), end = timestamps->end(); it != end; ++it)
            {
                const Timestamp& timestamp = *it;
                if (timestamp.id.timestampName == id.timestampName)
                {
                    if (refenreceFound)
                    {
                        archive.Error(id.timestampName, "Duplicated timestamp name");
                    }

                    refenreceFound = true;
                    continue;
                }

                exclusiveToTimestampList.push_back(timestamp.id.timestampName);
            }

            Serialization::StringListValue exclusiveToStringListValue(exclusiveToTimestampList, exclusiveTo.timestampName);
            archive(exclusiveToStringListValue, "exclusiveTo", "^<Exclusive To");
            exclusiveTo.timestampName = exclusiveToStringListValue.c_str();
        }

        bool operator < (const Timestamp& rhs) const
        {
            return (id.timestampName < rhs.id.timestampName);
        }
#endif
    };

    // Contains information about when certain events occurred.
    // Allows us to get answers to the following questions:
    // Q: How long was it since X happened?
    // Q: How long has Y been going on?
    class TimestampCollection
    {
        struct MatchesID
        {
            const TimestampID& id;
            MatchesID(const TimestampID& _id)
                : id(_id) {}
            bool operator () (const Timestamp& timestamp) const { return timestamp.id == id; }
        };

    public:
        bool HasBeenSetAtLeastOnce(const TimestampID& id) const
        {
            const Timestamps::const_iterator it = std::find_if(m_timestamps.begin(), m_timestamps.end(), MatchesID(id));

            if (it != m_timestamps.end())
            {
                return it->hasBeenSetAtLeastOnce;
            }

            return false;
        }

        void GetElapsedTimeSince(const TimestampID& id, CTimeValue& elapsedTime, bool& valid) const
        {
            const Timestamps::const_iterator it = std::find_if(m_timestamps.begin(), m_timestamps.end(), MatchesID(id));

            if (it != m_timestamps.end())
            {
                if (it->IsValid())
                {
                    valid = true;
                    elapsedTime = gEnv->pTimer->GetFrameStartTime() - it->time;
                    return;
                }
            }

            valid = false;
            elapsedTime = CTimeValue(0.0f);
        }

        void HandleEvent(uint32 eventNameCRC32)
        {
            TimestampIDs timestampsToInvalidate;

            {
                // Go through timestamps, refresh the time value and see if there
                // are any exclusive timestamps that should be removed.

                Timestamps::iterator it = m_timestamps.begin();
                Timestamps::iterator end = m_timestamps.end();

                for (; it != end; ++it)
                {
                    Timestamp& timestamp = *it;

                    if (timestamp.setOnEventNameCRC32 == eventNameCRC32)
                    {
                        timestamp.time = gEnv->pTimer->GetFrameStartTime();
                        timestamp.hasBeenSetAtLeastOnce = true;

                        if (timestamp.exclusiveTo)
                        {
                            timestampsToInvalidate.push_back(timestamp.exclusiveTo);
                        }
                    }
                }
            }

            {
                // Invalidate the timestamps that were pushed out via exclusivity

                TimestampIDs::iterator it = timestampsToInvalidate.begin();
                TimestampIDs::iterator end = timestampsToInvalidate.end();

                for (; it != end; ++it)
                {
                    Timestamps::iterator timestamp = std::find_if(m_timestamps.begin(), m_timestamps.end(), MatchesID(*it));

                    if (timestamp != m_timestamps.end())
                    {
                        timestamp->Invalidate();
                    }
                }
            }
        }

        void LoadFromXml(const XmlNodeRef& xml)
        {
            for (int i = 0; i < xml->getChildCount(); ++i)
            {
                XmlNodeRef child = xml->getChild(i);

                const char* name = child->getAttr("name");
                const char* setOnEvent = child->getAttr("setOnEvent");
                const char* exclusiveTo = child->getAttr("exclusiveTo");

                if (!name)
                {
                    gEnv->pLog->LogError("Missing 'name' attribute at line %d.", xml->getLine());
                    continue;
                }

                Timestamp timestamp;
                timestamp.id = TimestampID(name);
                timestamp.setOnEventNameCRC32 = CCrc32::Compute(setOnEvent);
                timestamp.exclusiveTo = TimestampID(exclusiveTo);
#if defined (STORE_EXTRA_TIMESTAMP_DATA)
                timestamp.setOnEventName = setOnEvent;
#endif
                m_timestamps.push_back(timestamp);
            }
        }

        const Timestamps& GetTimestamps() const { return m_timestamps; }

#if defined (USING_BEHAVIOR_TREE_SERIALIZATION)
        void Serialize(Serialization::IArchive& archive)
        {
            if (archive.IsOutput())
            {
                std::sort(m_timestamps.begin(), m_timestamps.end());
            }

            Serialization::SContext<Timestamps> variableDeclarations(archive, &m_timestamps);
            archive(m_timestamps, "timestamps", "^[<>]");
        }
#endif

#if defined (USING_BEHAVIOR_TREE_XML_DESCRIPTION_CREATION)
        XmlNodeRef CreateXmlDescription()
        {
            if (m_timestamps.size() == 0)
            {
                return XmlNodeRef();
            }

            XmlNodeRef timestampsXml = GetISystem()->CreateXmlNode("Timestamps");
            for (Timestamps::const_iterator it = m_timestamps.begin(), end = m_timestamps.end(); it != end; ++it)
            {
                const Timestamp& timestamp = *it;

                XmlNodeRef timestampXml = GetISystem()->CreateXmlNode("Timestamp");
                timestampXml->setAttr("name", timestamp.id.timestampName);
                timestampXml->setAttr("setOnEvent", timestamp.setOnEventName);
                if (!timestamp.exclusiveTo.timestampName.empty())
                {
                    timestampXml->setAttr("exclusiveTo", timestamp.exclusiveTo.timestampName);
                }

                timestampsXml->addChild(timestampXml);
            }

            return timestampsXml;
        }
#endif

    private:
        typedef std::vector<TimestampID> TimestampIDs;
        Timestamps m_timestamps;
    };
}

#endif // TimestampCollection_h