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
#include "SoundTrack.h"

//////////////////////////////////////////////////////////////////////////
void CSoundTrack::SerializeKey(ISoundKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        const char* sTemp;
        sTemp = keyNode->getAttr("StartTrigger");
        strncpy(key.sStartTrigger, sTemp, sizeof(key.sStartTrigger));
        key.sStartTrigger[sizeof(key.sStartTrigger) - 1] = '\0';

        sTemp = keyNode->getAttr("StopTrigger");
        strncpy(key.sStopTrigger, sTemp, sizeof(key.sStopTrigger));
        key.sStopTrigger[sizeof(key.sStopTrigger) - 1] = '\0';

        float fDuration = 0.0f;

        if (keyNode->getAttr("Duration", fDuration))
        {
            key.fDuration = fDuration;
        }

        keyNode->getAttr("CustomColor", key.customColor);
    }
    else
    {
        keyNode->setAttr("StartTrigger", key.sStartTrigger);
        keyNode->setAttr("StopTrigger", key.sStopTrigger);
        keyNode->setAttr("Duration", key.fDuration);
        keyNode->setAttr("CustomColor", key.customColor);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSoundTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    assert(key >= 0 && key < (int)m_keys.size());
    CheckValid();
    description = 0;
    duration = m_keys[key].fDuration;

    if (strlen(m_keys[key].sStartTrigger) > 0)
    {
        description = m_keys[key].sStartTrigger;
    }
}
