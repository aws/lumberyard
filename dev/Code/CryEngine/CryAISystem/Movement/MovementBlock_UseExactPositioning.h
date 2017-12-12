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

#ifndef CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_USEEXACTPOSITIONING_H
#define CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_USEEXACTPOSITIONING_H
#pragma once

#include "MovementBlock_UseExactPositioningBase.h"

namespace Movement
{
    namespace MovementBlocks
    {
        // This block has two responsibilities but the logic was so closely
        // mapped that I decided to combine them into one.
        //
        // The two parts are 'Prepare' and 'Traverse'.
        //
        // If the exact positioning fails to position the character it reports the
        // error through the bubbles and teleports to the position, and then
        // proceeds with the plan.

        class UseExactPositioning
            : public UseExactPositioningBase
        {
        public:
            typedef UseExactPositioningBase Base;

            UseExactPositioning(const CNavPath& path, const MovementStyle& style);
            virtual const char* GetName() const override { return "UseExactPositioning"; }
            virtual bool InterruptibleNow() const { return true; }

        private:
            virtual UseExactPositioningBase::TryRequestingExactPositioningResult TryRequestingExactPositioning(const MovementUpdateContext& context) override;
            virtual void HandleExactPositioningError(const MovementUpdateContext& context) override;
        };
    }
}

#endif // CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_USEEXACTPOSITIONING_H
