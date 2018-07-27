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

#ifndef CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_TURNTOWARDSPOSITION_H
#define CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_TURNTOWARDSPOSITION_H
#pragma once

#include "MovementPlan.h"
#include "MovementStyle.h"

namespace Movement
{
    namespace MovementBlocks
    {
        class TurnTowardsPosition
            : public Movement::Block
        {
        public:
            TurnTowardsPosition(const Vec3& position);
            virtual void End(IMovementActor& actor);
            virtual Movement::Block::Status Update(const MovementUpdateContext& context);
            virtual bool InterruptibleNow() const { return true; }
            virtual const char* GetName() const { return "TurnTowardsPosition"; }

        private:
            Vec3 m_positionToTurnTowards;
            float m_timeSpentAligning;
            float m_correctBodyDirTime;
        };
    }
}

#endif // CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_TURNTOWARDSPOSITION_H
