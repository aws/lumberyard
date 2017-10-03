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
#include "SelectTrack.h"

//////////////////////////////////////////////////////////////////////////
void CSelectTrack::SerializeKey(ISelectKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        const char* szSelection;
        szSelection = keyNode->getAttr("node");
        cry_strcpy(key.szSelection, szSelection);
        AZ::u64 id64;
        if (keyNode->getAttr("CameraAzEntityId", id64))
        {
            key.cameraAzEntityId = AZ::EntityId(id64);
        }
        keyNode->getAttr("BlendTime", key.fBlendTime);
    }
    else
    {
        keyNode->setAttr("node", key.szSelection);
        if (key.cameraAzEntityId.IsValid())
        {
            AZ::u64 id64 = static_cast<AZ::u64>(key.cameraAzEntityId);
            keyNode->setAttr("CameraAzEntityId", id64);
        }
        keyNode->setAttr("BlendTime", key.fBlendTime);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSelectTrack::GetKeyInfo(int key, const char*& description, float& duration)
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