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

#include "StdAfx.h"
#include "MannequinTrack.h"

//////////////////////////////////////////////////////////////////////////
bool CMannequinTrack::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    if (bLoading)
    {
    }
    else
    {
    }

    return TAnimTrack<IMannequinKey>::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
}

void CMannequinTrack::SerializeKey(IMannequinKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        const char* str;

        str = keyNode->getAttr("fragName");
        cry_strcpy(key.m_fragmentName, str);

        str = keyNode->getAttr("tags");
        cry_strcpy(key.m_tags, str);

        key.m_duration = 0;
        keyNode->getAttr("duration", key.m_duration);

        key.m_priority = 0;
        keyNode->getAttr("priority", key.m_priority);
    }
    else
    {
        if (strlen(key.m_fragmentName) > 0)
        {
            keyNode->setAttr("fragName", key.m_fragmentName);
        }
        if (strlen(key.m_tags) > 0)
        {
            keyNode->setAttr("tags", key.m_tags);
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
    if (strlen(m_keys[key].m_fragmentName) > 0)
    {
        description = m_keys[key].m_fragmentName;
        duration = m_keys[key].m_duration;
    }
}

float CMannequinTrack::GetKeyDuration(int key) const
{
    assert(key >= 0 && key < (int)m_keys.size());

    return m_keys[key].m_duration;
}
