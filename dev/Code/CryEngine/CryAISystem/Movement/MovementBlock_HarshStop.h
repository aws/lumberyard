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

#ifndef CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_HARSHSTOP_H
#define CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_HARSHSTOP_H
#pragma once

#include "MovementPlan.h"

namespace Movement
{
    namespace MovementBlocks
    {
        // Clears the movement variables in the pipe user state and waits
        // until the ai proxy says we are no longer moving.
        class HarshStop
            : public Movement::Block
        {
        public:
            virtual void Begin(IMovementActor& actor);
            virtual Status Update(const MovementUpdateContext& context);
            virtual const char* GetName() const { return "HarshStop"; }
        };
    }
}

#endif // CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_HARSHSTOP_H
