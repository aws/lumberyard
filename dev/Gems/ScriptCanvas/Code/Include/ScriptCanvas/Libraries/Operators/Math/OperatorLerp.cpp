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

#include "OperatorLerp.h"

#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/Core.h>

#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorLerp.generated.cpp>
#include <Include/ScriptCanvas/Data/NumericData.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        auto CreateLerpableTypesContract()
        {
            return []() -> RestrictedTypeContract* { return aznew RestrictedTypeContract({ Data::Type::Number(), Data::Type::Vector2(), Data::Type::Vector3(), Data::Type::Vector4() }); };
        }
        
        void LerpBetween::OnInit()
        {
            // Input Configuration
            {
                DataSlotConfiguration startConfiguration;
                
                startConfiguration.m_name = "Start";
                startConfiguration.m_toolTip = "The value to start lerping from.";
                startConfiguration.m_slotType = SlotType::DataIn;                

                startConfiguration.m_contractDescs.emplace_back(CreateLerpableTypesContract());
                
                startConfiguration.m_dynamicDataType = DynamicDataType::Value;                
                startConfiguration.m_displayType = m_displayType;
                
                m_startSlotId = AddInputDatumSlot(startConfiguration, Datum());
            }
            
            {
                DataSlotConfiguration stopConfiguration;
                
                stopConfiguration.m_name = "Stop";
                stopConfiguration.m_toolTip = "The value to stop lerping at.";
                stopConfiguration.m_slotType = SlotType::DataIn;                

                stopConfiguration.m_contractDescs.emplace_back(CreateLerpableTypesContract());
                
                stopConfiguration.m_dynamicDataType = DynamicDataType::Value;
                stopConfiguration.m_displayType = m_displayType;
                
                m_stopSlotId = AddInputDatumSlot(stopConfiguration, Datum());
            }
            
            {
                DataSlotConfiguration speedConfiguration;
                
                speedConfiguration.m_name = "Speed";
                speedConfiguration.m_toolTip = "The speed at which to lerp between the start and stop.";
                speedConfiguration.m_slotType = SlotType::DataIn;                

                speedConfiguration.m_contractDescs.emplace_back(CreateLerpableTypesContract());
                
                speedConfiguration.m_dynamicDataType = DynamicDataType::Value;                
                speedConfiguration.m_displayType = m_displayType;
                
                m_speedSlotId = AddInputDatumSlot(speedConfiguration, Datum());
            }
            
            m_maximumTimeSlotId = AddInputDatumSlot("Maximum Duration", "The maximum amount of time it will take to complete the specified lerp. Negative value implies no limit, 0 implies instant.", Datum::eOriginality::Original, int(-1));
            ////
            
            // Output Configuration
            {
                DataSlotConfiguration stepConfiguration;
                
                stepConfiguration.m_name = "Step";
                stepConfiguration.m_toolTip = "The value of the current step of the lerp.";
                stepConfiguration.m_slotType = SlotType::DataOut;                

                stepConfiguration.m_contractDescs.emplace_back(CreateLerpableTypesContract());
                
                stepConfiguration.m_dynamicDataType = DynamicDataType::Value;                
                stepConfiguration.m_displayType = m_displayType;
                
                m_stepSlotId = AddDataSlot(stepConfiguration);
            }            
            
            m_percentSlotId = AddOutputTypeSlot("Percent", "The perctange of the way through the lerp this tick is on.", Data::Type::Number(), ScriptCanvas::Node::OutputStorage::Optional);
            ////
            
            m_groupedSlotIds = { m_startSlotId, m_stopSlotId, m_speedSlotId, m_stepSlotId };

            for (const SlotId& slotId : m_groupedSlotIds)
            {
                EndpointNotificationBus::MultiHandler::BusConnect(Endpoint(GetEntityId(), slotId));
            }
        }
        
        void LerpBetween::OnSystemTick()
        {
            // Ping pong between the system and the normal tick bus for a consistent starting point for the lerp
            AZ::SystemTickBus::Handler::BusDisconnect();            
            AZ::TickBus::Handler::BusConnect();
        }
        
        void LerpBetween::OnTick(float deltaTime, AZ::ScriptTimePoint timePoint)
        {
            bool lerpComplete = false;
            
            m_counter += deltaTime;
            
            if (m_counter >= m_duration)
            {
                lerpComplete = true;
                m_counter = m_duration;
            }
            
            float percent = m_counter/m_duration;
            
            SignalLerpStep(percent);
        }
        
        void LerpBetween::OnInputSignal(const SlotId& slotId)
        {
            if (slotId == LerpBetweenProperty::GetCancelSlotId(this))
            {
                CancelLerp();
            }
            else if (slotId == LerpBetweenProperty::GetInSlotId(this))
            {
                CancelLerp();
                
                AZ::SystemTickBus::Handler::BusConnect();
                
                float speedOnlyTime = 0.0;
                float maxDuration = 0.0;
                
                const Datum* durationDatum = GetInput(m_maximumTimeSlotId);
                const Datum* speedDatum = GetInput(m_speedSlotId);                
                
                if (durationDatum)
                {
                    maxDuration = static_cast<float>((*durationDatum->GetAs<Data::NumberType>()));
                }

                if (m_displayType == Data::Type::Number())
                {
                    SetupStartStop<Data::NumberType>();

                    if (!m_differenceDatum.GetType().IsValid())
                    {
                        return;
                    }
                    
                    Data::NumberType difference = (*m_differenceDatum.GetAs<Data::NumberType>());                    
                    Data::NumberType speed = (*speedDatum->GetAs<Data::NumberType>());
                    
                    if (AZ::IsClose(speed, 0.0, Data::ToleranceEpsilon()))
                    {
                        speedOnlyTime = -1.0f;
                    }
                    else
                    {
                        speedOnlyTime = fabsf(static_cast<float>(difference)/static_cast<float>(speed));
                    }                        
                }
                else if (m_displayType == Data::Type::Vector2())
                {
                    SetupStartStop<Data::Vector2Type>();
                    speedOnlyTime = CalculateVectorSpeedTime<Data::Vector2Type>(speedDatum);
                }
                else if (m_displayType == Data::Type::Vector3())
                {
                    SetupStartStop<Data::Vector3Type>();
                    speedOnlyTime = CalculateVectorSpeedTime<Data::Vector3Type>(speedDatum);
                }
                else if (m_displayType == Data::Type::Vector4())
                {
                    SetupStartStop<Data::Vector4Type>();
                    speedOnlyTime = CalculateVectorSpeedTime<Data::Vector4Type>(speedDatum);
                }
                
                m_counter = 0;
                
                if (speedOnlyTime > 0 && maxDuration > 0)
                {
                    if (speedOnlyTime > maxDuration)
                    {
                        m_duration = maxDuration;
                    }
                    else
                    {
                        m_duration = speedOnlyTime;
                    }
                }
                else if (speedOnlyTime >= 0.0f)
                {
                    m_duration = speedOnlyTime;
                }
                else if (maxDuration >= 0.0f)
                {
                    m_duration = maxDuration;
                }
                else
                {
                    AZ_Error("ScriptCanvas", false, "Lerp Between not given a valid speed or duration to perofrm the lerp at. Defaulting to 1 second duration");
                    m_duration = 1.0;
                }
                
                if (AZ::IsClose(m_duration, 0.0f, AZ::g_fltEps))
                {
                    CancelLerp();
                    
                    SignalLerpStep(1.0f);
                }
                
                SignalOutput(LerpBetweenProperty::GetOutSlotId(this));
            }
        }
        
        void LerpBetween::OnEndpointConnected(const Endpoint& endpoint)
        {
            const SlotId& currentSlotId = EndpointNotificationBus::GetCurrentBusId()->GetSlotId();
            
            if (IsInGroup(currentSlotId))
            {
                if (m_displayType.IsValid())
                {
                    return;
                }
                
                auto node = AZ::EntityUtils::FindFirstDerivedComponent<Node>(endpoint.GetNodeId());
                
                if (node)
                {
                    Slot* slot = node->GetSlot(endpoint.GetSlotId());

                    if (slot && (!slot->IsDynamicSlot() || slot->HasDisplayType()))
                    {
                        SetDisplayType(slot->GetDataType());
                    }
                }
            }
        }
        
        void LerpBetween::OnEndpointDisconnected(const Endpoint& endpoint)
        {
            const SlotId& currentSlotId = EndpointNotificationBus::GetCurrentBusId()->GetSlotId();
            
            if (IsInGroup(currentSlotId))
            {
                if (!IsGroupConnected())
                {
                    SetDisplayType(Data::Type::Invalid());
                }
            }
        }
        
        void LerpBetween::CancelLerp()
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();            
        }
        
        void LerpBetween::SetDisplayType(ScriptCanvas::Data::Type dataType)
        {
            if (m_displayType != dataType)
            {
                m_displayType = dataType;
                
                for (const SlotId& slotId : m_groupedSlotIds)
                {
                    Slot* groupedSlot = GetSlot(slotId);
                    
                    if (groupedSlot)
                    {
                        groupedSlot->SetDisplayType(m_displayType);
                    }
                }
            }
        }
        
        void LerpBetween::SignalLerpStep(float percent)
        {
            Datum stepDatum(m_displayType, Datum::eOriginality::Original);
            stepDatum.SetToDefaultValueOfType();
            
            if (m_displayType == Data::Type::Number())
            {
                CalculateLerpStep<Data::NumberType>(percent, stepDatum);
            }
            else if (m_displayType == Data::Type::Vector2())
            {
                CalculateLerpStep<Data::Vector2Type>(percent, stepDatum);
            }
            else if (m_displayType == Data::Type::Vector3())
            {
                CalculateLerpStep<Data::Vector3Type>(percent, stepDatum);
            }
            else if (m_displayType == Data::Type::Vector4())
            {
                CalculateLerpStep<Data::Vector4Type>(percent, stepDatum);
            }

            Datum percentDatum(ScriptCanvas::Data::Type::Number(), Datum::eOriginality::Original);
            percentDatum.Set<Data::NumberType>(percent);
            
            PushOutput(percentDatum, *GetSlot(m_percentSlotId));
            PushOutput(stepDatum, *GetSlot(m_stepSlotId));
            
            SignalOutput(LerpBetweenProperty::GetTickSlotId(this));
            
            if (AZ::IsClose(percent, 1.0f, AZ::g_fltEps))
            {
                SignalOutput(LerpBetweenProperty::GetLerpCompleteSlotId(this));
                AZ::TickBus::Handler::BusDisconnect();
            }
        }
        
        bool LerpBetween::IsGroupConnected() const
        {
            bool hasConnections = false;
            for (auto slotId : m_groupedSlotIds)
            {
                if (IsConnected(slotId))
                {
                    hasConnections = true;
                    break;
                }
            }
            
            return hasConnections;
        }
        
        bool LerpBetween::IsInGroup(const SlotId& slotId) const
        {
            return m_groupedSlotIds.count(slotId) > 0;
        }
    }
}