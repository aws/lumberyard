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

#include <ScriptCanvas/Libraries/Core/Repeater.h>

#include <Libraries/Core/MethodUtility.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            namespace
            {
                AZStd::string StringifyUnits(Repeater::DelayUnits delayUnits)
                {
                    switch (delayUnits)
                    {
                    case Repeater::DelayUnits::Seconds:
                        return "Seconds";
                    case Repeater::DelayUnits::Ticks:
                        return "Ticks";
                    default:
                        break;
                    }
                    
                    return "???";
                }

                AZStd::string GetDelaySlotName(Repeater::DelayUnits delayUnits)
                {
                    AZStd::string stringifiedUnits = StringifyUnits(delayUnits).c_str();
                    return AZStd::string::format("Delay (%s)", stringifiedUnits);
                }
            }
            
            void Repeater::OnSystemTick()
            {
                AZ::SystemTickBus::Handler::BusDisconnect();
                
                if (!AZ::TickBus::Handler::BusIsConnected())
                {
                    AZ::TickBus::Handler::BusConnect();
                }                
            }
            
            void Repeater::OnTick(float delta, AZ::ScriptTimePoint timePoint)
            {
                switch (m_delayUnits)
                {
                case DelayUnits::Seconds:
                    m_delayTimer += delta;
                    break;
                case DelayUnits::Ticks:
                    m_delayTimer += 1;
                    break;
                }

                TriggerOutput();
            }

            void Repeater::OnInit()
            {
                AZStd::string slotName = GetDelaySlotName(static_cast<DelayUnits>(m_delayUnits));

                Slot* slot = GetSlotByName(slotName);

                if (slot)
                {
                    m_timeSlotId = slot->GetId();
                }
                else
                {
                    // Handle older versions and improperly updated names
                    for (DelayUnits testUnit : { DelayUnits::Seconds, DelayUnits::Ticks})
                    {
                        AZStd::string legacyName = StringifyUnits(static_cast<DelayUnits>(testUnit));

                        slot = GetSlotByName(legacyName);

                        if (slot)
                        {
                            m_timeSlotId = slot->GetId();
                            slot->Rename(slotName);
                            break;
                        }
                    }
                }
            }

            void Repeater::OnDeactivate()
            {
                AZ::TickBus::Handler::BusDisconnect();
                AZ::SystemTickBus::Handler::BusDisconnect();
            }
            
            void Repeater::OnConfigured()
            {
                AZStd::string slotName = GetDelaySlotName(static_cast<DelayUnits>(m_delayUnits));

                Slot* slot = GetSlotByName("Delay");

                if (slot)
                {
                    slot->Rename(slotName);
                    m_timeSlotId = slot->GetId();
                }
                else if (!m_timeSlotId.IsValid())
                {
                    DataSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = slotName;
                    slotConfiguration.m_toolTip = "The amount of delay to insert in between each firing.";

                    slotConfiguration.SetConnectionType(ConnectionType::Input);
                    slotConfiguration.SetDefaultValue(0.0);

                    m_timeSlotId = AddSlot(slotConfiguration);
                }
            }

            void Repeater::OnInputSignal(const SlotId& slotId)
            {
                if (slotId == SlotId())
                {
                    TriggerOutput();
                }

                if (slotId != RepeaterProperty::GetInSlotId(this))
                {
                    return;
                }

                const ScriptCanvas::Datum* datum = GetInput(m_timeSlotId);

                if (!datum)
                {
                    return;
                }

                if (AZ::TickBus::Handler::BusIsConnected())
                {
                    AZ::TickBus::Handler::BusDisconnect();
                }

                if (AZ::SystemTickBus::Handler::BusIsConnected())
                {
                    AZ::SystemTickBus::Handler::BusDisconnect();
                }

                m_repetionCount = aznumeric_cast<int>(RepeaterProperty::GetRepetitions(this));

                const Data::NumberType* delayAmount = datum->GetAs<Data::NumberType>();

                if (delayAmount)
                {
                    m_delayAmount = (*delayAmount);
                    m_delayTimer = 0;

                    if (m_delayUnits == DelayUnits::Ticks)
                    {
                        // Remove all of the floating points
                        m_delayTimer = aznumeric_cast<float>(aznumeric_cast<int>(m_delayTimer));
                    }
                }

                if (m_repetionCount >= 0)
                {
                    if (!AZ::IsClose(m_delayAmount, 0.0, 0.0001))
                    {
                        AZ::SystemTickBus::Handler::BusConnect();
                    }
                    else
                    {
                        TriggerOutput();
                    }
                }
                else
                {
                    SignalOutput(RepeaterProperty::GetCompleteSlotId(this));
                }
            }

            AZStd::vector<AZStd::pair<int, AZStd::string>> Repeater::GetDelayUnits() const
            {
                AZStd::vector<AZStd::pair<int, AZStd::string>> comboBoxDisplay;

                for (auto delayUnit : { DelayUnits::Seconds, DelayUnits::Ticks })
                {
                    comboBoxDisplay.emplace_back(static_cast<int>(delayUnit), StringifyUnits(delayUnit));
                }

                return comboBoxDisplay;
            }

            void Repeater::OnDelayUnitsChanged(const int&)
            {
                UpdateTimeName();
            }

            void Repeater::UpdateTimeName()
            {
                Slot* slot = GetSlot(m_timeSlotId);

                if (slot)
                {
                    slot->Rename(GetDelaySlotName(static_cast<DelayUnits>(m_delayUnits)));
                }
            }

            void Repeater::TriggerOutput()
            {
                if (m_repetionCount <= 0)
                {
                    if (AZ::TickBus::Handler::BusIsConnected())
                    {
                        AZ::TickBus::Handler::BusDisconnect();
                    }

                    SignalOutput(RepeaterProperty::GetCompleteSlotId(this));
                }
                else if (m_delayTimer >= m_delayAmount)
                {
                    m_delayTimer -= m_delayAmount;

                    --m_repetionCount;
                    ExecutionRequestBus::Event(GetGraphId(), &ExecutionRequests::AddToExecutionStack, *this, SlotId{});
                    SignalOutput(RepeaterProperty::GetActionSlotId(this));
                }
            }
        }
    }
}

#include <Include/ScriptCanvas/Libraries/Core/Repeater.generated.cpp>