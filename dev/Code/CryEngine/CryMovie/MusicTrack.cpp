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
#include "MusicTrack.h"

//////////////////////////////////////////////////////////////////////////
void CMusicTrack::SerializeKey(IMusicKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        const char* pStr;
        int nType;
        if (!keyNode->getAttr("type", nType))
        {
            key.eType = eMusicKeyType_SetMood;
        }
        else
        {
            key.eType = (EMusicKeyType)nType;
        }
        pStr = keyNode->getAttr("mood");
        if (pStr)
        {
            cry_strcpy(key.szMood, pStr);
        }
        else
        {
            key.szMood[0] = 0;
        }
        if (!keyNode->getAttr("volramp_time", key.fTime))
        {
            key.fTime = 0.0f;
        }
    }
    else
    {
        keyNode->setAttr("type", key.eType);
        if (strlen(key.szMood) > 0)
        {
            keyNode->setAttr("mood", key.szMood);
        }
        keyNode->setAttr("volramp_time", key.fTime);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMusicTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    assert(key >= 0 && key < (int)m_keys.size());
    CheckValid();
    description = 0;
    duration = 0;
    switch (m_keys[key].eType)
    {
    case eMusicKeyType_SetMood:
        duration = 0.0f;
        description = m_keys[key].szMood;
        break;
    case eMusicKeyType_VolumeRamp:
        duration = m_keys[key].fTime;
        description = "RampDown";
        break;
    }
}
