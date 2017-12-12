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

#ifndef CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_SETUPPIPEUSERCOVERINFORMATION_H
#define CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_SETUPPIPEUSERCOVERINFORMATION_H
#pragma once

#include "MovementPlan.h"

namespace Movement
{
    namespace MovementBlocks
    {
        class SetupActorCoverInformation
            : public Movement::Block
        {
        public:
            virtual void Begin(IMovementActor& actor);
            virtual const char* GetName() const { return "SetCoverInfo"; }
        };
    }
}

#endif // CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_SETUPPIPEUSERCOVERINFORMATION_H
