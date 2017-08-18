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

#ifndef CRYINCLUDE_CRYAISYSTEM_FLYHELPERS_TACTICAL_H
#define CRYINCLUDE_CRYAISYSTEM_FLYHELPERS_TACTICAL_H
#pragma once

#include "AIPlayer.h"

namespace FlyHelpers
{
    ILINE bool IsVisible(CPipeUser* pPipeUser, IEntity* pTargetEntity)
    {
        CRY_ASSERT(pPipeUser);
        CRY_ASSERT(pTargetEntity);

        const IAIObject* pTargetAiObject = pTargetEntity->GetAI();
        IF_UNLIKELY (!pTargetAiObject)
        {
            // This case is not properly handled because we don't care about it for the moment.
            return false;
        }

        const IVisionMap* pVisionMap = gEnv->pAISystem->GetVisionMap();
        CRY_ASSERT(pVisionMap);

        const VisionID observerVisionId = pPipeUser->GetVisionID();
        const VisionID targetVisitonId = pTargetAiObject->GetVisionID();
        const bool hasLineOfSight = pVisionMap->IsVisible(observerVisionId, targetVisitonId);

        bool isCloaked = false;
        const CAIPlayer* pTargetAiPlayer = pTargetAiObject->CastToCAIPlayer();
        if (pTargetAiPlayer)
        {
            isCloaked = pTargetAiPlayer->IsInvisibleFrom(pPipeUser->GetPos(), true, true);
        }

        const bool isVisible = (hasLineOfSight && !isCloaked);
        return isVisible;
    }
}

#endif // CRYINCLUDE_CRYAISYSTEM_FLYHELPERS_TACTICAL_H
