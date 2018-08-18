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
#include "MovementBlock_UseExactPositioning.h"
#include "MovementActor.h"
#include "MovementUpdateContext.h"
#include "MovementHelpers.h"
#include "Navigation/NavigationSystem/NavigationSystem.h"
#include "PipeUser.h"
#include "AIBubblesSystem/IAIBubblesSystem.h"

namespace Movement
{
    namespace MovementBlocks
    {
        UseExactPositioning::UseExactPositioning(
            const CNavPath& path,
            const MovementStyle& style)
            : Base(path, style)
        {
        }

        UseExactPositioningBase::TryRequestingExactPositioningResult UseExactPositioning::TryRequestingExactPositioning(const MovementUpdateContext& context)
        {
            CRY_ASSERT(m_style.GetExactPositioningRequest());
            if (m_style.GetExactPositioningRequest() == 0)
            {
                return RequestFailed_FinishImmediately;
            }

            const bool useLowerPrecision = false;
            context.actor.GetAdapter().RequestExactPosition(m_style.GetExactPositioningRequest(), useLowerPrecision);

            return RequestSucceeded;
        }

        void UseExactPositioning::HandleExactPositioningError(const MovementUpdateContext& context)
        {
            const EntityId actorEntityId = context.actor.GetEntityId();
            if (IEntity* entity = gEnv->pEntitySystem->GetEntity(actorEntityId))
            {
                // Consider teleporting to the startpoint here, with code like the following
                // (needs to be discussed further)
                //
                //if (m_style.GetExactPositioningRequest() != 0)
                //{
                //Vec3 newForwardDir = m_style.GetExactPositioningRequest()->animDirection.GetNormalizedSafe(FORWARD_DIRECTION);

                //QuatT teleportDestination(
                //  m_style.GetExactPositioningRequest()->animLocation,
                //  Quat::CreateRotationV0V1(FORWARD_DIRECTION, newForwardDir));

                //entity->SetWorldTM(Matrix34(teleportDestination));
                //}

                // Report
                stack_string message;
                message.Format("Exact positioning failed to get me to the start of the actor target or was canceled incorrectly.");
                AIQueueBubbleMessage("PrepareForExactPositioningError", actorEntityId, message.c_str(), eBNS_Balloon | eBNS_LogWarning);
            }
        }
    }
}