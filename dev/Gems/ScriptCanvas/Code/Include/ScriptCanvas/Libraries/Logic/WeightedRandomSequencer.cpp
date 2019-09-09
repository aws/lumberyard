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

#include "WeightedRandomSequencer.h"

#include <AzCore/std/string/string.h>

#include <Include/ScriptCanvas/Libraries/Logic/WeightedRandomSequencer.generated.cpp>
#include <Include/ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
		
        namespace Logic
        {
            ////////////////////////////
            // WeightedRandomSequencer
            ////////////////////////////

            void WeightedRandomSequencer::ReflectDataTypes(AZ::ReflectContext* reflectContext)
            {
                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    if (serializeContext)
                    {
                        serializeContext->Class<WeightedPairing>()
                            ->Version(1)
                            ->Field("WeightSlotId", &WeightedPairing::m_weightSlotId)
                            ->Field("ExecutionSlotId", &WeightedPairing::m_executionSlotId)
                        ;
                    }
                }
            }

            void WeightedRandomSequencer::OnInit()
            {
                for (const WeightedPairing& weightedPairing : m_weightedPairings)
                {
                    EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), weightedPairing.m_weightSlotId });
                    EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), weightedPairing.m_executionSlotId });
                }
                
                // We always want at least one weighted transition state
                if (m_weightedPairings.empty())
                {
                    AddWeightedPair();                    
                }
            }
            
            void WeightedRandomSequencer::OnInputSignal(const SlotId& slotId)
            {
                const SlotId inSlotId = WeightedRandomSequencerProperty::GetInSlotId(this);
                
                if (slotId == inSlotId)
                {
                    int runningTotal = 0;
                    
                    AZStd::vector< WeightedStruct > weightedStructs;
                    weightedStructs.reserve(m_weightedPairings.size());
                    
                    for (const WeightedPairing& weightedPairing : m_weightedPairings)
                    {
                        const Datum* datum = GetInput(weightedPairing.m_weightSlotId);
                        
                        if (datum)
                        {
                            WeightedStruct weightedStruct;
                            weightedStruct.m_executionSlotId = weightedPairing.m_executionSlotId;
                            
                            if (datum->GetType().IS_A(ScriptCanvas::Data::Type::Number()))
                            {
                                int weight = aznumeric_cast<int>((*datum->GetAs<Data::NumberType>()));
                                
                                runningTotal += weight;                                
                                weightedStruct.m_totalWeight = runningTotal;
                            }

                            weightedStructs.emplace_back(weightedStruct);
                        }                        
                    }
                    
                    // We have no weights. So just trigger the first execution output
                    // Weighted pairings is controlled to never be empty.
                    if (runningTotal == 0)
                    {          
                        if (!m_weightedPairings.empty())
                        {
                            SignalOutput(m_weightedPairings.front().m_executionSlotId);
                        }
                        
                        return;
                    }
                    
                    int weightedResult = MathNodeUtilities::GetRandomIntegral<int>(1, runningTotal);                    
                    
                    for (const WeightedStruct& weightedStruct : weightedStructs)
                    {
                        if (weightedResult <= weightedStruct.m_totalWeight)
                        {
                            SignalOutput(weightedStruct.m_executionSlotId);
                            break;
                        }
                    }
                }
            }
            
            void WeightedRandomSequencer::OnEndpointConnected(const Endpoint& endpoint)
            {
                if (AllWeightsFilled())
                {
                    AddWeightedPair();
                }                
            }
            
            void WeightedRandomSequencer::OnEndpointDisconnected(const Endpoint& endpoint)
            {
                // We always want one. So we don't need to do anything else.
                if (m_weightedPairings.size() == 1)
                {
                    return;
                }
                
                const SlotId& currentSlotId = EndpointNotificationBus::GetCurrentBusId()->GetSlotId();
                
                // If there are still connections. We don't need to do anything.
                if (IsConnected(currentSlotId))
                {
                    return;
                }

                if (!HasExcessEndpoints())
                {
                    return;
                }                    
                    
                for (auto pairIter = m_weightedPairings.begin(); pairIter != m_weightedPairings.end(); ++pairIter)
                {
                    if (pairIter->m_executionSlotId == currentSlotId)
                    {
                        if (!IsConnected(pairIter->m_weightSlotId))
                        {
                            RemoveWeightedPair(currentSlotId);
                            break;
                        }
                    }                    
                    else if (pairIter->m_weightSlotId == currentSlotId)
                    {
                        if (!IsConnected(pairIter->m_executionSlotId))
                        {
                            RemoveWeightedPair(currentSlotId);
                            break;
                        }
                    }
                }
            }
            
            void WeightedRandomSequencer::RemoveWeightedPair(SlotId slotId)
            {
                for (auto pairIter = m_weightedPairings.begin(); pairIter != m_weightedPairings.end(); ++pairIter)
                {
                    const WeightedPairing& weightedPairing = (*pairIter);
                    
                    if (pairIter->m_executionSlotId == slotId
                        || pairIter->m_weightSlotId == slotId)                        
                    {
                        EndpointNotificationBus::MultiHandler::BusDisconnect({ GetEntityId(), pairIter->m_executionSlotId});
                        EndpointNotificationBus::MultiHandler::BusDisconnect({ GetEntityId(), pairIter->m_weightSlotId});

                        RemoveSlot(pairIter->m_weightSlotId);
                        RemoveSlot(pairIter->m_executionSlotId);                        
                        
                        m_weightedPairings.erase(pairIter);
                        break;
                    }
                }
                
                FixupStateNames();
            }
            
            bool WeightedRandomSequencer::AllWeightsFilled() const
            {
                bool isFilled = true;
                for (const WeightedPairing& weightedPairing : m_weightedPairings)
                {
                    if (!IsConnected(weightedPairing.m_weightSlotId))
                    {
                        isFilled = false;
                        break;
                    }
                }             

                return isFilled;
            }

            bool WeightedRandomSequencer::HasExcessEndpoints() const
            {
                bool hasExcess = false;
                bool hasEmpty = false;

                for (const WeightedPairing& weightedPairing : m_weightedPairings)
                {
                    if (!IsConnected(weightedPairing.m_weightSlotId) && !IsConnected(weightedPairing.m_executionSlotId))
                    {
                        if (hasEmpty)
                        {
                            hasExcess = true;
                            break;
                        }
                        else
                        {
                            hasEmpty = true;
                        }
                    }
                }

                return hasExcess;
            }
            
            void WeightedRandomSequencer::AddWeightedPair()
            {
                int counterWeight = static_cast<int>(m_weightedPairings.size()) + 1;
            
                WeightedPairing weightedPairing;
                
                DataSlotConfiguration dataSlotConfiguration;
                
                dataSlotConfiguration.m_slotType = SlotType::DataIn;
                dataSlotConfiguration.m_name = GenerateDataName(counterWeight);
                dataSlotConfiguration.m_toolTip = "The weight associated with the execution state.";
                dataSlotConfiguration.m_addUniqueSlotByNameAndType = false;
                dataSlotConfiguration.m_dataType = Data::Type::Number();           

                dataSlotConfiguration.m_displayGroup = "WeightedExecutionGroup";
                
                weightedPairing.m_weightSlotId = AddInputDatumSlot(dataSlotConfiguration);
                
                SlotConfiguration  slotConfiguration;
                
                slotConfiguration.m_name = GenerateOutName(counterWeight);
                slotConfiguration.m_addUniqueSlotByNameAndType = false;
                slotConfiguration.m_slotType = SlotType::ExecutionOut;              

                slotConfiguration.m_displayGroup = "WeightedExecutionGroup";
                
                weightedPairing.m_executionSlotId = AddSlot(slotConfiguration);                
                
                m_weightedPairings.push_back(weightedPairing);
                
                EndpointNotificationBus::MultiHandler::BusConnect({GetEntityId(), weightedPairing.m_weightSlotId });
                EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), weightedPairing.m_executionSlotId });
            }
            
            void WeightedRandomSequencer::FixupStateNames()
            {
                int counter = 1;
                for (const WeightedPairing& weightedPairing : m_weightedPairings)
                {
                    AZStd::string dataName = GenerateDataName(counter);
                    AZStd::string executionName = GenerateOutName(counter);
                    
                    Slot* dataSlot = GetSlot(weightedPairing.m_weightSlotId);
                    
                    if (dataSlot)
                    {
                        dataSlot->Rename(dataName);
                    }
                    
                    Slot* executionSlot = GetSlot(weightedPairing.m_executionSlotId);
                    
                    if (executionSlot)
                    {
                        executionSlot->Rename(executionName);
                    }
                    
                    ++counter;
                }
            }
            
            AZStd::string WeightedRandomSequencer::GenerateDataName(int counter)
            {
                return AZStd::string::format("Weight %i", counter);
            }
            
            AZStd::string WeightedRandomSequencer::GenerateOutName(int counter)
            {
                return AZStd::string::format("Out %i", counter);
            }
        }
    }
}