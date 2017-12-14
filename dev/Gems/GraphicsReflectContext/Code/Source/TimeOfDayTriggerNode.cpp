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

#include "Precompiled.h"

#include "TimeOfDayTriggerNode.h"
#include <ISystem.h>
#include <I3DEngine.h>
#include <ITimeOfDay.h>

#include <Source/TimeOfDayTriggerNode.generated.cpp>

namespace GraphicsReflectContext
{

    TimeOfDayTriggerNode::TimeOfDayTriggerNode()
        : Node()
        , m_triggerTime(0.f)
        , m_lastTime(0.f)
    {}

    void TimeOfDayTriggerNode::OnInputSignal(const ScriptCanvas::SlotId& slotId)
    {
        const ScriptCanvas::SlotId enableSlot = TimeOfDayTriggerNodeProperty::GetEnableSlotId(this);
        const ScriptCanvas::SlotId disableSlot = TimeOfDayTriggerNodeProperty::GetDisableSlotId(this);

        if (slotId == enableSlot)
        {
            m_triggerTime = TimeOfDayTriggerNodeProperty::GetTime(this);
            m_lastTime = GetCurrentTimeOfDay();
            AZ::TickBus::Handler::BusConnect();

        }
        else if (slotId == disableSlot)
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    void TimeOfDayTriggerNode::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        const ScriptCanvas::SlotId outSlot = TimeOfDayTriggerNodeProperty::GetOutSlotId(this);

        const float currentTime = GetCurrentTimeOfDay();

        // if currentTime < lastTime, we wrapped around and have to process two intervals
        // [lastTime, 24.0] and [0, curTime]
        // Otherwise, process interval [lastTime, curTime]
        const bool wrap = m_lastTime > currentTime;
        const float maxTime = wrap ? 24.0f : currentTime;

        // process interval [lastTime, curTime] or in case of wrap [lastTime, 24.0]
        if (m_triggerTime >= m_lastTime && m_triggerTime <= maxTime)
        {
            SignalOutput(outSlot);
        }
        // process interval [0, curTime] in case of wrap
        else if (wrap && m_triggerTime <= currentTime)
        {
            SignalOutput(outSlot);
        }

        m_lastTime = currentTime;
    }

    void TimeOfDayTriggerNode::OnDeactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    float TimeOfDayTriggerNode::GetCurrentTimeOfDay()
    {
        if (gEnv && gEnv->p3DEngine)
        {
            ITimeOfDay* tod = gEnv->p3DEngine->GetTimeOfDay();
            if (tod)
            {
                return tod->GetTime();
            }
        }
        AZ_Warning("Scripting", false, "TimeOfDayTriggerNode: Can not get current time of day!");
        return 0;
    }
}




