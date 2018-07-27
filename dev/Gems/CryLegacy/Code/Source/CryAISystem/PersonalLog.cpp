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

#include "CryLegacy_precompiled.h"
#include "PersonalLog.h"

void PersonalLog::AddMessage(const EntityId entityId, const char* message)
{
    if (m_messages.size() + 1 > 20)
    {
        m_messages.pop_front();
    }

    m_messages.push_back(message);

    if (gAIEnv.CVars.OutputPersonalLogToConsole)
    {
        const char* name = "(null)";

        if (IEntity* entity = gEnv->pEntitySystem->GetEntity(entityId))
        {
            name = entity->GetName();
        }

        gEnv->pLog->Log("Personal Log [%s] %s", name, message);
    }

#ifdef CRYAISYSTEM_DEBUG
    if (IEntity* entity = gEnv->pEntitySystem->GetEntity(entityId))
    {
        if (IAIObject* ai = entity->GetAI())
        {
            IAIRecordable::RecorderEventData recorderEventData(message);
            ai->RecordEvent(IAIRecordable::E_PERSONALLOG, &recorderEventData);
        }
    }
#endif
}
