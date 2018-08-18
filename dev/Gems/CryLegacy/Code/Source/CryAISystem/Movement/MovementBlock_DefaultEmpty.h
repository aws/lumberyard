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

#pragma once

#ifndef MovementBlock_UnsupportedCustomNavigationType_h
#define MovementBlock_UnsupportedCustomNavigationType_h

#include "MovementPlan.h"
#include "MovementStyle.h"

namespace Movement
{
    namespace MovementBlocks
    {
        class DefaultEmpty
            : public Movement::Block
        {
        public:
            DefaultEmpty() {}

            // Movement::Block
            virtual Movement::Block::Status Update(const MovementUpdateContext& context) override
            {
                AIError("Trying to use the DefaultEmpty block, an undefined implementation of a movement block.");
                return CantBeFinished;
            };
            virtual const char* GetName() const override { return "DefaultEmpty"; }
            // ~Movement::Block
        };
    }
}

#endif // MovementBlock_UnsupportedCustomNavigationType_h