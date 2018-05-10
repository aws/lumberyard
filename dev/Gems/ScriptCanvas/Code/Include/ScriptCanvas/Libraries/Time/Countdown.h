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

#include <Include/ScriptCanvas/Libraries/Time/Countdown.generated.h>

#include <AzCore/RTTI/TypeInfo.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            class Countdown 
                : public Node
                , AZ::TickBus::Handler
            {
                ScriptCanvas_Node(Countdown,
                    ScriptCanvas_Node::Name("Delay")
                    ScriptCanvas_Node::Uuid("{FAEADF5A-F7D9-415A-A3E8-F534BD379B9A}")
                    ScriptCanvas_Node::Description("Counts down time from a specified value.")
                );

            public:

                Countdown();

            protected:

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "When signaled, execution is delayed at this node according to the specified properties.")
                                ScriptCanvas_In::Contracts({ DisallowReentrantExecutionContract }));

                ScriptCanvas_In(ScriptCanvas_In::Name("Reset", "Resets the delay.")
                                ScriptCanvas_In::Contracts({ DisallowReentrantExecutionContract }));

                // Outputs
                ScriptCanvas_OutLatent(ScriptCanvas_Out::Name("Out", "Signaled when the delay reaches zero."));

                // Data
                ScriptCanvas_Property(float,
                    ScriptCanvas_Property::Name("Time", "Amount of time to delay, in seconds") 
                    ScriptCanvas_Property::Input);

                ScriptCanvas_Property(bool,
                    ScriptCanvas_Property::Name("Loop", "If true, the delay will restart after triggering the Out slot") ScriptCanvas_Property::ChangeNotify(AZ::Edit::PropertyRefreshLevels::EntireTree) 
                    ScriptCanvas_Property::Input);

                ScriptCanvas_Property(float,
                    ScriptCanvas_Property::Name("Hold", "Amount of time to wait before restarting, in seconds") ScriptCanvas_Property::Visibility(&Countdown::ShowHoldTime) 
                    ScriptCanvas_Property::Input);

                ScriptCanvas_Property(float, 
                    ScriptCanvas_Property::Name("Elapsed", "The amount of time that has elapsed since the delay began.") ScriptCanvas_Property::Visibility(false) 
                    ScriptCanvas_Property::Output
                    ScriptCanvas_Property::OutputStorageSpec
                );

                // temps
                float m_countdownSeconds;
                bool m_looping;
                float m_holdTime;
                float m_elapsedTime;

                //! Internal, used to determine if the node is holding before looping
                bool m_holding;

                //! Internal counter to track time elapsed
                float m_currentTime;

                void OnInputSignal(const SlotId&) override;

                bool ShowHoldTime() const
                {
                    // TODO: This only works on the property grid. If a true value is connected to the "SetLoop" slot,
                    // We need to show the "Hold Before Loop" slot, otherwise we need to hide it.
                    return m_looping;
                }

            protected:

                void OnDeactivate() override;
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            };
        }
    }
}