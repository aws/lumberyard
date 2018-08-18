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

#ifndef CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTHELPERS_H
#define CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTHELPERS_H
#pragma once

#include "MovementPlan.h"
#include "MovementStyle.h"

struct IMovementActor;

namespace Movement
{
    namespace Helpers
    {
        class StuckDetector
        {
        public:
            StuckDetector();
            void Update(const MovementUpdateContext& context);
            void Reset();
            bool IsAgentStuck() const;

        private:
            float m_accumulatedTimeAgentIsStuck;
            float m_agentDistanceToTheEndInPreviousUpdate;
        };

        void BeginPathFollowing(IMovementActor& actor, const MovementStyle& style, const CNavPath& navPath);
        bool UpdatePathFollowing(PathFollowResult& result, const MovementUpdateContext& context, const MovementStyle& style);
    }
}

#endif // CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTHELPERS_H
