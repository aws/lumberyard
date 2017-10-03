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

#include "precompiled.h"

#include "Indexer.h"

#include <Include/ScriptCanvas/Libraries/Logic/Indexer.generated.cpp>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            Indexer::Indexer()
                : Node()
                , m_out(-1)
            {}

            void Indexer::OnInputSignal(const SlotId& slot)
            {
                auto slotIt = m_slotContainer.m_slotIdSlotMap.find(slot);
                if (slotIt != m_slotContainer.m_slotIdSlotMap.end())
                {
                    m_out = slotIt->second;
                }

                const Datum output = Datum::CreateInitializedCopy(m_out);
                PushOutput(output, m_slotContainer.m_slots[k_outputIndex]);

                const SlotId outSlot = IndexerProperty::GetOutSlotId(this);
                SignalOutput(outSlot);
            }
        }
    }
}