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

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/CodeGen/CodeGen.h>
#include <AzCore/Component/TickBus.h>

#include <Source/TimeOfDayTriggerNode.generated.h>

namespace AZ
{
    class ReflectContext;
}

namespace GraphicsReflectContext
{
    class TimeOfDayTriggerNode
        : public ScriptCanvas::Node
        , AZ::TickBus::Handler
    {
        ScriptCanvas_Node(TimeOfDayTriggerNode,
            ScriptCanvas_Node::Uuid("{3871A0AC-5DED-43FB-A808-F97CA723CE2C}")
            ScriptCanvas_Node::Name("On Time Reached")
            ScriptCanvas_Node::Category("Rendering/Time of Day")
            ScriptCanvas_Node::Description("Signals when a specific time of day is reached")
        );
    public:
        TimeOfDayTriggerNode();

    protected:
        //Inputs
        ScriptCanvas_In(ScriptCanvas_In::Name("Enable", "Enables the node"));
        ScriptCanvas_In(ScriptCanvas_In::Name("Disable", "Disables the node"));
        //Outputs
        ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", "Signals when the specified time of day is reached (if the node is enabled)"));
        //Data
        ScriptCanvas_Property(float,
            ScriptCanvas_Property::Name("Time", "Signal the Out pin at this time of day [0-24] (in hours)")
            ScriptCanvas_Property::Input
            ScriptCanvas_Property::Min(0.f)
            ScriptCanvas_Property::Max(24.f));

        void OnInputSignal(const ScriptCanvas::SlotId& slot) override;
        void OnDeactivate() override;
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:
        float GetCurrentTimeOfDay();
        
        float m_triggerTime;
        float m_lastTime;
    };

}
