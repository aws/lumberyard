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
#include "MovementBlock_TurnTowardsPosition.h"
#include "MovementUpdateContext.h"
#include "MovementActor.h"
#include "PipeUser.h"

namespace Movement
{
    namespace MovementBlocks
    {
        TurnTowardsPosition::TurnTowardsPosition(const Vec3& position)
            : m_positionToTurnTowards(position)
            , m_timeSpentAligning(0.0f)
            , m_correctBodyDirTime(0.0f)
        {
        }

        Movement::Block::Status TurnTowardsPosition::Update(const MovementUpdateContext& context)
        {
            // Align body towards the move target
            const Vec3 actorPysicalPosition = context.actor.GetAdapter().GetPhysicsPosition();
            Vec3 dirToMoveTarget = m_positionToTurnTowards - actorPysicalPosition;
            dirToMoveTarget.z = 0.0f;
            dirToMoveTarget.Normalize();
            context.actor.GetAdapter().SetBodyTargetDirection(dirToMoveTarget);

            // If animation body direction is within the angle threshold,
            // wait for some time and then mark the agent as ready to move
            const Vec3 actualBodyDir = context.actor.GetAdapter().GetAnimationBodyDirection();
            const bool lookingTowardsMoveTarget = (actualBodyDir.Dot(dirToMoveTarget) > cosf(DEG2RAD(17.0f)));
            if (lookingTowardsMoveTarget)
            {
                m_correctBodyDirTime += context.updateTime;
            }
            else
            {
                m_correctBodyDirTime = 0.0f;
            }

            const float timeSpentAligning = m_timeSpentAligning + context.updateTime;
            m_timeSpentAligning = timeSpentAligning;

            if (m_correctBodyDirTime > 0.2f)
            {
                return Movement::Block::Finished;
            }

            const float timeout = 8.0f;
            if (timeSpentAligning > timeout)
            {
                gEnv->pLog->LogWarning("Agent '%s' at %f %f %f failed to turn towards %f %f %f within %f seconds. Proceeding anyway.",
                    context.actor.GetName(), actorPysicalPosition.x, actorPysicalPosition.y, actorPysicalPosition.z,
                    m_positionToTurnTowards.x, m_positionToTurnTowards.y, m_positionToTurnTowards.z, timeout);
                return Movement::Block::Finished;
            }

            return Movement::Block::Running;
        }

        void TurnTowardsPosition::End(IMovementActor& actor)
        {
            actor.GetAdapter().ResetBodyTarget();
        }
    }
}