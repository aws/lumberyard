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
#include "ConsoleTrack.h"

//////////////////////////////////////////////////////////////////////////
void CConsoleTrack::SerializeKey(IConsoleKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        const char* str;
        str = keyNode->getAttr("command");
        cry_strcpy(key.command, str);
    }
    else
    {
        if (strlen(key.command) > 0)
        {
            keyNode->setAttr("command", key.command);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CConsoleTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    assert(key >= 0 && key < (int)m_keys.size());
    CheckValid();
    description = 0;
    duration = 0;
    if (strlen(m_keys[key].command) > 0)
    {
        description = m_keys[key].command;
    }
}
