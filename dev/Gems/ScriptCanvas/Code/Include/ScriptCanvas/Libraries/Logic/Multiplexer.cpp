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

#include "Multiplexer.h"

#include <Include/ScriptCanvas/Libraries/Logic/Multiplexer.generated.cpp>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            Multiplexer::Multiplexer()
                : Node()
                , m_selectedIndex(0)
                , m_inputIndex(-1)
            {}

            void  Multiplexer::OnInputSignal(const SlotId& slot)
            {
                auto slotIt = m_slotContainer.m_slotIdSlotMap.find(slot);
                if (slotIt != m_slotContainer.m_slotIdSlotMap.end())
                {
                    m_inputIndex = slotIt->second;
                }

                // These are generated from CodeGen, they essentially
                // check if there's anything connected on the slot and
                // they assign the local member with the value, otherwise
                // they use the default property value.
                m_selectedIndex = MultiplexerProperty::GetIndex(this);

                if (m_selectedIndex == m_inputIndex)
                {
                    const SlotId outSlot = MultiplexerProperty::GetOutSlotId(this);
                    SignalOutput(outSlot);
                }
            }
        }
    }
}