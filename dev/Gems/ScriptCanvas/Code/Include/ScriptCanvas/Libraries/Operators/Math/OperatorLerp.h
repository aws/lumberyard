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

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/unordered_set.h>

#include <ScriptCanvas/CodeGen/CodeGen.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/GraphBus.h>

#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorLerp.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        class LerpBetween 
            : public Node
            , public AZ::SystemTickBus::Handler
            , public AZ::TickBus::Handler
            , public EndpointNotificationBus::MultiHandler
        {
        public:

            ScriptCanvas_Node(LerpBetween,
                ScriptCanvas_Node::Name("Lerp Between")
                ScriptCanvas_Node::Uuid("{58AF538D-021A-4D0C-A3F1-866C2FFF382E}")
                ScriptCanvas_Node::Description("Performs a lerp between the two specified sources using the speed specified or in the amount of time specified, or the minimum of the two")
                ScriptCanvas_Node::Version(0)
                ScriptCanvas_Node::Category("Math")
            );
            
            // General purpose nodes that we don't need to do anything fancy for
            ScriptCanvas_In(ScriptCanvas_In::Name("In", ""));
            ScriptCanvas_In(ScriptCanvas_In::Name("Cancel", ""));
            
            ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", ""));
            ScriptCanvas_OutLatent(ScriptCanvas_Out::Name("Tick", "Signalled at each step of the lerp"));
            ScriptCanvas_OutLatent(ScriptCanvas_Out::Name("Lerp Complete", "Output signal"));
            ////
            
            // Serialize Properties
            ScriptCanvas_SerializeProperty(Data::Type, m_displayType);

            // Data Input SlotIds
            ScriptCanvas_SerializeProperty(SlotId, m_startSlotId);
            ScriptCanvas_SerializeProperty(SlotId, m_stopSlotId);
            ScriptCanvas_SerializeProperty(SlotId, m_speedSlotId);
            ScriptCanvas_SerializeProperty(SlotId, m_maximumTimeSlotId);
            ////

            // Data Output SlotIds
            ScriptCanvas_SerializeProperty(SlotId, m_stepSlotId);
            ScriptCanvas_SerializeProperty(SlotId, m_percentSlotId);
            ////

            ////

            LerpBetween() = default;
            ~LerpBetween() override = default;
            
            void OnInit() override;
            
            // SystemTickBus
            void OnSystemTick() override;
            ////
            
            // TickBus
            void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;                
            ////
            
            // Node
            void OnInputSignal(const SlotId& slotId) override;
            
            void OnEndpointConnected(const Endpoint& endpoint) override;
            void OnEndpointDisconnected(const Endpoint& endpoint) override;
            ////
            
        private:
        
            void CancelLerp();
            void SetDisplayType(Data::Type dataType);            

            void SignalLerpStep(float percent);
            
            bool IsGroupConnected() const;
            bool IsInGroup(const SlotId& slotId) const;
            
            template<typename DataType>
            void CalculateLerpStep(float percent, Datum& stepDatum)
            {
                const DataType* startValue = m_startDatum.GetAs<DataType>();
                const DataType* differenceValue = m_differenceDatum.GetAs<DataType>();
                
                if (startValue == nullptr || differenceValue == nullptr)
                {
                    return;
                }

                DataType stepValue = (*startValue) + (*differenceValue) * percent;
                stepDatum.Set<DataType>(stepValue);
            }
            
            template<typename DataType>
            void SetupStartStop()
            {
                const Datum* startDatum = GetInput(m_startSlotId);
                const Datum* endDatum = GetInput(m_stopSlotId);
                
                m_startDatum = (*startDatum);
                m_differenceDatum = Datum(m_displayType, Datum::eOriginality::Original);
                
                const DataType* startValue = startDatum->GetAs<DataType>();
                const DataType* endValue = endDatum->GetAs<DataType>();

                if (startValue == nullptr || endValue == nullptr)
                {
                    m_differenceDatum.SetToDefaultValueOfType();
                }
                else
                {
                    DataType difference = (*endValue) - (*startValue);
                    m_differenceDatum.Set<DataType>(difference);
                }
            }
            
            template<typename DataType>
            float CalculateVectorSpeedTime(const Datum* speedDatum)
            {
                const DataType* dataType = speedDatum->GetAs<DataType>();
                
                float speedLength = 0.0f;
                
                if (dataType)
                {
                    speedLength = dataType->GetLength();
                }
                
                if (AZ::IsClose(speedLength, 0.0f, AZ::g_fltEps))
                {
                    return -1.0;
                }
                
                float differenceLength = 0.0;
                const DataType* differenceValue = m_differenceDatum.GetAs<DataType>();
                
                if (differenceValue)
                {
                    differenceLength = differenceValue->GetLength();
                }
                
                return fabsf(static_cast<float>(differenceLength/speedLength));
            }            
            
            AZStd::unordered_set< SlotId > m_groupedSlotIds;            
        
            float m_duration;
            float m_counter;
            
            Datum m_startDatum;
            Datum m_differenceDatum;
        };
    }
}