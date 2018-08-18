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
#include "MovementPlan.h"
#include "MovementUpdateContext.h"

namespace Movement
{
    Plan::Status Plan::Execute(const MovementUpdateContext& context)
    {
        if (m_current == NoBlockIndex)
        {
            ChangeToIndex(0, context.actor);
        }

        while (true)
        {
            assert(m_current != NoBlockIndex);
            assert(m_current < m_blocks.size());

            const Block::Status status = m_blocks[m_current]->Update(context);

            if (status == Block::Finished)
            {
                if (m_current + 1 < m_blocks.size())
                {
                    ChangeToIndex(m_current + 1, context.actor);
                    continue;
                }
                else
                {
                    ChangeToIndex(NoBlockIndex, context.actor);
                    return Finished;
                }
            }
            else if (status == Block::CantBeFinished)
            {
                return CantBeFinished;
            }
            else
            {
                assert(status == Block::Running);
            }

            break;
        }

        return Running;
    }

    void Plan::ChangeToIndex(const uint newIndex, IMovementActor& actor)
    {
        const uint oldIndex = m_current;

        if (oldIndex != NoBlockIndex)
        {
            m_blocks[oldIndex]->End(actor);
        }

        if (newIndex != NoBlockIndex)
        {
            m_blocks[newIndex]->Begin(actor);
        }

        m_current = newIndex;
    }

    void Plan::Clear(IMovementActor& actor)
    {
        ChangeToIndex(NoBlockIndex, actor);
        m_blocks.clear();
    }

    void Plan::CutOffAfterCurrentBlock()
    {
        if (m_current < m_blocks.size())
        {
            m_blocks.resize(m_current + 1);
        }
    }

    bool Plan::InterruptibleNow() const
    {
        if (m_current == NoBlockIndex)
        {
            return true;
        }

        return m_blocks[m_current]->InterruptibleNow();
    }

    const Block* Plan::GetBlock(uint32 index) const
    {
        return m_blocks[index].get();
    }
}