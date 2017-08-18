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
#include "LookAtTrack.h"

//////////////////////////////////////////////////////////////////////////
bool CLookAtTrack::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    if (bLoading)
    {
        xmlNode->getAttr("AnimationLayer", m_iAnimationLayer);
    }
    else
    {
        xmlNode->setAttr("AnimationLayer", m_iAnimationLayer);
    }

    return TAnimTrack<ILookAtKey>::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
}

void CLookAtTrack::SerializeKey(ILookAtKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        const char* szSelection;
        szSelection = keyNode->getAttr("node");
        f32 smoothTime;
        if (!keyNode->getAttr("smoothTime", smoothTime))
        {
            smoothTime = 0.0f;
        }

        const char* lookPose  = keyNode->getAttr("lookPose");
        if (lookPose)
        {
            strcpy(key.lookPose, lookPose);
        }

        cry_strcpy(key.szSelection, szSelection);
        key.smoothTime = smoothTime;
    }
    else
    {
        keyNode->setAttr("node", key.szSelection);
        keyNode->setAttr("smoothTime", key.smoothTime);
        keyNode->setAttr("lookPose", key.lookPose);
    }
}

//////////////////////////////////////////////////////////////////////////
void CLookAtTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    assert(key >= 0 && key < (int)m_keys.size());
    CheckValid();
    description = 0;
    duration = m_keys[key].fDuration;
    if (strlen(m_keys[key].szSelection) > 0)
    {
        description = m_keys[key].szSelection;
    }
}