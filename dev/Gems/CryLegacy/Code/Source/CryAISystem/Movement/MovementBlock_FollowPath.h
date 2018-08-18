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

#ifndef CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_FOLLOWPATH_H
#define CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_FOLLOWPATH_H
#pragma once

#include "MovementPlan.h"
#include "MovementStyle.h"
#include "MovementHelpers.h"

namespace Movement
{
    namespace MovementBlocks
    {
        class FollowPath
            : public Movement::Block
        {
        public:
            FollowPath(const CNavPath& path, const float endDistance, const MovementStyle& style, const bool endsInCover);
            virtual bool InterruptibleNow() const { return true; }
            virtual void Begin(IMovementActor& actor);
            virtual void End(IMovementActor& actor);
            virtual Movement::Block::Status Update(const MovementUpdateContext& context);

            void UpdateLooking(const MovementUpdateContext& context, const bool targetReachable, const float pathDistanceToEnd, const Vec3& followTargetPosition);
            virtual const char* GetName() const { return "FollowPath"; }

        private:
            void UpdateCoverLocations(CPipeUser& pipeUser);

        private:
            CNavPath m_path;
            MovementStyle m_style;
            Movement::Helpers::StuckDetector m_stuckDetector;
            AZStd::shared_ptr<Vec3> m_lookTarget;
            float m_finishBlockEndDistance;
            float m_accumulatedPathFollowerFailureTime;
            bool m_endsInCover;
        };
    }
}

#endif // CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_FOLLOWPATH_H
