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

#ifndef CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTPLAN_H
#define CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTPLAN_H
#pragma once


struct IMovementActor;
struct MovementUpdateContext;

#include <MovementBlock.h>

namespace Movement
{
    // This is a plan that will satisfy a movement request such as
    // "reach this destination" or "stop immediately".
    //
    // It's comprised of one or more movement blocks that each take care
    // of a certain part of plan. They are executed in order.
    //
    // As an example a plan to satisfy the request "move to this cover" could
    // be comprised of the following movement blocks:
    // [FollowPath] -> [UseSmartObject] -> [FollowPath] -> [InstallInCover]
    class Plan
    {
    public:
        static const uint32 NoBlockIndex = ~0u;

        Plan()
            : m_current(NoBlockIndex)
        {
        }

        enum Status
        {
            Running,
            Finished,
            CantBeFinished,
        };

        template <typename BlockType>
        void AddBlock()
        {
            m_blocks.push_back(AZStd::shared_ptr<Block>(new BlockType()));
        }

        void AddBlock(const AZStd::shared_ptr<Block>& block)
        {
            m_blocks.push_back(block);
        }

        Status Execute(const MovementUpdateContext& context);
        void ChangeToIndex(const uint newIndex, IMovementActor& actor);
        bool HasBlocks() const { return !m_blocks.empty(); }
        void Clear(IMovementActor& actor);
        void CutOffAfterCurrentBlock();
        bool InterruptibleNow() const;
        uint32 GetCurrentBlockIndex() const { return m_current; }
        uint32 GetBlockCount() const { return m_blocks.size(); }
        const Block* GetBlock(uint32 index) const;

    private:
        typedef std::vector<AZStd::shared_ptr<Block> > Blocks;
        Blocks m_blocks;
        uint32 m_current;
    };
}

#endif // CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTPLAN_H
