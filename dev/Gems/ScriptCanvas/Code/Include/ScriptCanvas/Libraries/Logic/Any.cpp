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

#include "Any.h"

#include <Include/ScriptCanvas/Libraries/Logic/Any.generated.cpp>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            Any::Any()
            {
            }

            void Any::OnInit()
            {
                if (m_inputSlots.empty())
                {
                    AZStd::vector< const Slot* > slots = GetSlotsByType(ScriptCanvas::SlotType::ExecutionIn);
                    
                    for (const Slot* slot : slots)
                    {
                        m_inputSlots.emplace_back(slot->GetId());
                    }

                    if (m_inputSlots.empty())
                    {
                        AddInputSlot();
                    }
                    else
                    {
                        // Here to deal with old any nodes that didn't have the dynamic
                        // slot addition.
                        for (const SlotId& slotId : m_inputSlots)
                        {
                            EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), slotId });
                        }
                    }
                }
                else
                {
                    for (const SlotId& slotId : m_inputSlots)
                    {
                        EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), slotId });
                    }
                }

                
            }
            
            void Any::OnInputSignal(const SlotId& slotId)
            {
                const SlotId outSlotId = AnyProperty::GetOutSlotId(this);
                SignalOutput(outSlotId);
            }

            void Any::OnEndpointConnected(const Endpoint& endpoint)
            {
                if (AllInputSlotsFilled())
                {
                    AddInputSlot();
                }
            }

            void Any::OnEndpointDisconnected(const Endpoint& endpoint)
            {
                if (m_inputSlots.size() > 1)
                {
                    if (!AllInputSlotsFilled())
                    {
                        CleanUpInputSlots();
                    }
                }
            }

            AZStd::string Any::GenerateInputName(int counter)
            {
                return AZStd::string::format("Input %i", counter);
            }

            bool Any::AllInputSlotsFilled() const
            {
                bool slotsFilled = true;

                for (const SlotId& slotId : m_inputSlots)
                {
                    if (!IsConnected(slotId))
                    {
                        slotsFilled = false;
                        break;
                    }
                }

                return slotsFilled;
            }

            void Any::AddInputSlot()
            {
                int inputCount = static_cast<int>(m_inputSlots.size());

                SlotConfiguration  slotConfiguration;

                slotConfiguration.m_name = GenerateInputName(inputCount);
                slotConfiguration.m_addUniqueSlotByNameAndType = false;
                slotConfiguration.m_slotType = SlotType::ExecutionIn;

                SlotId inputSlot = AddSlot(slotConfiguration);

                auto inputIter = AZStd::find(m_inputSlots.begin(), m_inputSlots.end(), inputSlot);

                if (inputIter == m_inputSlots.end())
                {
                    EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), inputSlot });
                    m_inputSlots.emplace_back(inputSlot);
                }
            }

            void Any::CleanUpInputSlots()
            {
                if (m_inputSlots.size() <= 1)
                {
                    return;
                }

                bool removedSlot = false;
                bool removeEmptySlot = false;

                for (int counter = static_cast<int>(m_inputSlots.size()) - 1; counter >= 0; --counter)
                {
                    SlotId slotId = m_inputSlots[counter];

                    if (!IsConnected(slotId))
                    {
                        if (removeEmptySlot)
                        {
                            EndpointNotificationBus::MultiHandler::BusDisconnect({ GetEntityId(), slotId });
                            RemoveSlot(slotId);

                            m_inputSlots.erase(m_inputSlots.begin() + counter);

                            removedSlot = true;
                        }
                        else
                        {
                            removeEmptySlot = true;
                        }
                    }
                }

                if (removedSlot)
                {
                    FixupStateNames();
                }
            }

            void Any::FixupStateNames()
            {
                for (int i = 0; i < m_inputSlots.size(); ++i)
                {
                    Slot* slot = GetSlot(m_inputSlots[i]);

                    if (slot)
                    {
                        slot->Rename(GenerateInputName(i));
                    }
                }
            }
        }
    }
}