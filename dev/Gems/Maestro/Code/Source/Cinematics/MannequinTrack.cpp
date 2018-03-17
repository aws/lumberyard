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

#include "Maestro_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>
#include "MannequinTrack.h"

//////////////////////////////////////////////////////////////////////////
/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
bool CMannequinTrack::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    return TAnimTrack<IMannequinKey>::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
}

void CMannequinTrack::SerializeKey(IMannequinKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        const char* str;

        str = keyNode->getAttr("fragName");
        key.m_fragmentName = str;

        str = keyNode->getAttr("tags");
        key.m_tags = str;

        key.m_duration = 0;
        keyNode->getAttr("duration", key.m_duration);

        key.m_priority = 0;
        keyNode->getAttr("priority", key.m_priority);
    }
    else
    {
        if (!key.m_fragmentName.empty())
        {
            keyNode->setAttr("fragName", key.m_fragmentName.c_str());
        }
        if (!key.m_tags.empty())
        {
            keyNode->setAttr("tags", key.m_tags.c_str());
        }
        if (key.m_duration > 0)
        {
            keyNode->setAttr("duration", key.m_duration);
        }
        if (key.m_priority > 0)
        {
            keyNode->setAttr("priority", key.m_priority);
        }
    }
}

void CMannequinTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    assert(key >= 0 && key < (int)m_keys.size());
    CheckValid();
    description = 0;
    duration = 0;
    if (!m_keys[key].m_fragmentName.empty())
    {
        description = m_keys[key].m_fragmentName.c_str();
        duration = m_keys[key].m_duration;
    }
}

float CMannequinTrack::GetKeyDuration(int key) const
{
    assert(key >= 0 && key < (int)m_keys.size());

    return m_keys[key].m_duration;
}

//////////////////////////////////////////////////////////////////////////
template<>
inline void TAnimTrack<IMannequinKey>::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<TAnimTrack<IMannequinKey> >()
        ->Version(1)
        ->Field("Flags", &TAnimTrack<IMannequinKey>::m_flags)
        ->Field("Range", &TAnimTrack<IMannequinKey>::m_timeRange)
        ->Field("ParamType", &TAnimTrack<IMannequinKey>::m_nParamType)
        ->Field("Keys", &TAnimTrack<IMannequinKey>::m_keys);
}

//////////////////////////////////////////////////////////////////////////
void CMannequinTrack::Reflect(AZ::SerializeContext* serializeContext)
{
    TAnimTrack<IMannequinKey>::Reflect(serializeContext);

    serializeContext->Class<CMannequinTrack, TAnimTrack<IMannequinKey> >()
        ->Version(1);
}