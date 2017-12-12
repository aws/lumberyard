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
#if defined(CODE_GEN_NODES_ENABLED)

#include "LinearInterpolation.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
            LinearInterpolation::LinearInterpolation()
                : Node()
            {}

            AZ::BehaviorValueParameter LinearInterpolation::EvaluateSlot(const ScriptCanvas::SlotId& slotId)
            {
                const SlotId& startSlot = GetSlotId("SetStart");
                const SlotId& endSlot = GetSlotId("SetEnd");
                const SlotId& timeSlot = GetSlotId("SetTime");

                ScriptCanvas::Slot* slot = GetSlot(slotId);
                if (slot)
                {
                    if (slot->GetType() == ScriptCanvas::SlotType::DataIn)
                    {
                        AZStd::vector<Endpoint> connectedEndpoints;
                        GraphRequestBus::EventResult(connectedEndpoints, m_graphId, &GraphRequests::GetConnectedEndpoints, Endpoint{ GetEntityId(), slotId });
                        if (!connectedEndpoints.empty())
                        {
                            if (slot->GetId() == startSlot)
                            {
                                AZ::BehaviorValueParameter parameter;
                                ScriptCanvas::NodeServiceRequestBus::EventResult(parameter, connectedEndpoints[0].GetNodeId(), &NodeServiceRequests::EvaluateSlot, connectedEndpoints[0].GetSlotId());
                                ConvertToValue(m_a, parameter);
                            }
                            else if (slot->GetId() == endSlot)
                            {
                                AZ::BehaviorValueParameter parameter;
                                ScriptCanvas::NodeServiceRequestBus::EventResult(parameter, connectedEndpoints[0].GetNodeId(), &NodeServiceRequests::EvaluateSlot, connectedEndpoints[0].GetSlotId());
                                ConvertToValue(m_b, parameter);
                            }
                            else if (slot->GetId() == timeSlot)
                            {
                                AZ::BehaviorValueParameter parameter;
                                ScriptCanvas::NodeServiceRequestBus::EventResult(parameter, connectedEndpoints[0].GetNodeId(), &NodeServiceRequests::EvaluateSlot, connectedEndpoints[0].GetSlotId());
                                ConvertToValue(m_time, parameter);
                            }
                        }
                    }
                    else if (slot->GetType() == ScriptCanvas::SlotType::DataOut)
                    {
                        const SlotId& resultSlot = GetSlotId("GetResult");

                        EvaluateSlot(startSlot);
                        EvaluateSlot(endSlot);
                        EvaluateSlot(timeSlot);

                        m_result = m_a * (1.0f - m_time) + m_b * m_time;

                        return AZ::BehaviorValueParameter(&m_result);
                    }
                }

                return{};
            }

        }
    }
}

#include <Include/ScriptCanvas/Libraries/Math/LinearInterpolation.generated.cpp>

#endif