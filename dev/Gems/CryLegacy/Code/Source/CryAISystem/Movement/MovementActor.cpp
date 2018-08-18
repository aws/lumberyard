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
#include "MovementActor.h"
#include "../PipeUser.h"
#include "MovementPlanner.h"

void MovementActor::SetLowCoverStance()
{
    GetAIActor()->GetState().bodystate = STANCE_LOW_COVER;
}

CAIActor* MovementActor::GetAIActor()
{
    CAIActor* aiActor = NULL;

    if (IEntity* entity = gEnv->pEntitySystem->GetEntity(entityID))
    {
        if (IAIObject* ai = entity->GetAI())
        {
            aiActor = ai->CastToCAIActor();
        }
    }

    return aiActor;
}

const char* MovementActor::GetName() const
{
    const IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityID);
    return pEntity ? pEntity->GetName() : "(none)";
}

void MovementActor::RequestPathTo(const Vec3& destination, float lengthToTrimFromThePathEnd, const MNMDangersFlags dangersFlags /* = eMNMDangers_None */, const bool considerActorsAsPathObstacles)
{
    const bool cutPathAtSmartObject = false;

    const IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityID);
    if (!pEntity)
    {
        return;
    }

    if (callbacks.queuePathRequestFunction)
    {
        MNMPathRequest request(pEntity->GetPos(), destination, Vec3(0, 1, 0), -1, 0.0f, lengthToTrimFromThePathEnd, true, NULL, NavigationAgentTypeID(1), dangersFlags);
        callbacks.queuePathRequestFunction(request);
    }
}

Movement::PathfinderState MovementActor::GetPathfinderState() const
{
    if (callbacks.checkOnPathfinderStateFunction)
    {
        return callbacks.checkOnPathfinderStateFunction();
    }

    return Movement::CouldNotFindPath;
}

void MovementActor::Log(const char* message)
{
    #ifdef AI_COMPILE_WITH_PERSONAL_LOG
    if (GetAIActor())
    {
        GetAIActor()->GetPersonalLog().AddMessage(entityID, message);
    }
    #endif
}

IMovementActorAdapter& MovementActor::GetAdapter() const
{
    assert(pAdapter);
    return *pAdapter;
}
