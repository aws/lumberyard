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

#ifndef CRYINCLUDE_CRYCOMMON_AISYSTEMLISTENER_H
#define CRYINCLUDE_CRYCOMMON_AISYSTEMLISTENER_H
#pragma once

struct IPuppet;

class IAISystemListener
{
public:
    virtual ~IAISystemListener(){}
    //////////////////////////////////////////////////////////////////////////////////////
    // I'd like to get rid of this. There were in fact no references to it in Crysis 2.

    enum EAISystemEvent
    {
        eAISE_Reset,
    };

    virtual void OnEvent(EAISystemEvent event) {}

    //////////////////////////////////////////////////////////////////////////////////////

    virtual void OnAgentDeath(EntityId deadEntityID, EntityId killerID) {}
    virtual void OnAgentUpdate(EntityId entityID) {}
};

#endif // CRYINCLUDE_CRYCOMMON_AISYSTEMLISTENER_H
