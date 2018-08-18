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

#ifndef CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_USEEXACTPOSITIONINGBASE_H
#define CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_USEEXACTPOSITIONINGBASE_H
#pragma once

#include "MovementPlan.h"
#include "MovementStyle.h"
#include "MovementHelpers.h"

namespace Movement
{
    namespace MovementBlocks
    {
        // This block has two responsibilities but the logic was so closely
        // mapped that I decided to combine them into one.
        //
        // The two parts are 'Prepare' and 'Traverse'.
        //
        // If the exact positioning fails to position the character it calls
        // HandleExactPositioningError in which the derived class can
        // teleport to an appropriate target

        class UseExactPositioningBase
            : public Movement::Block
        {
        public:
            UseExactPositioningBase(const CNavPath& path, const MovementStyle& style);
            virtual void Begin(IMovementActor& actor) override;
            virtual void End(IMovementActor& actor) override;
            virtual Status Update(const MovementUpdateContext& context) override;

        protected:
            enum TryRequestingExactPositioningResult
            {
                RequestSucceeded,
                RequestDelayed_ContinuePathFollowing,
                RequestDelayed_SkipPathFollowing,
                RequestFailed_FinishImmediately,
            };

            // Called while preparing & when actortarget phase is None.
            // The derived class is supposed to request an actor target in here.
            virtual TryRequestingExactPositioningResult TryRequestingExactPositioning(const MovementUpdateContext& context) = 0;

            // Exact positioning failed to position the character. Teleport to an appropriate target.
            virtual void HandleExactPositioningError(const MovementUpdateContext& context) = 0;

            virtual void OnTraverseStarted(const MovementUpdateContext& context) {}

        private:
            Status UpdatePrepare(const MovementUpdateContext& context);
            Status UpdateTraverse(const MovementUpdateContext& context);

        protected:
            enum State
            {
                Prepare,
                Traverse,
            };

            MovementStyle m_style;
            State m_state;

        private:
            CNavPath m_path;
            float m_accumulatedPathFollowerFailureTime;
            Movement::Helpers::StuckDetector m_stuckDetector;
        };
    }
}

#endif // CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_USEEXACTPOSITIONINGBASE_H
