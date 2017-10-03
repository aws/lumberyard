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

#include "Delay.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            void Delay::Reflect(AZ::ReflectContext* reflection)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
                if (serializeContext)
                {
                    serializeContext->Class<Delay, Node>()
                        ->Version(1)
                        ->Field("m_delaySeconds", &Delay::m_delaySeconds)
                        ;

                    AZ::EditContext* editContext = serializeContext->GetEditContext();
                    if (editContext)
                    {
                        editContext->Class<Delay>("Delay", "Delays the graph execution for the specified duration in seconds")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Delay.png")
                            ->Attribute(ScriptCanvas::Attributes::Input, "In")
                            ->Attribute(ScriptCanvas::Attributes::Output, "Out")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &Delay::m_delaySeconds, "Delay", "Delay the graph for the specified duration in seconds")
                            ->Attribute(AZ::Edit::Attributes::Suffix, " seconds");
                        ;
                    }
                }
            }

            void Delay::OnInit()
            {
                AddSlot("In", SlotType::ExecutionIn);
                AddSlot("Out", SlotType::ExecutionOut);
            }

            void Delay::OnExecute()
            {
                m_currentTime = 0.f;
                AZ::TickBus::Handler::BusConnect();
            }

            void Delay::OnTick(float deltaTime, AZ::ScriptTimePoint timePoint)
            {
                DebugTrace("Delay");

                if (m_currentTime >= m_delaySeconds)
                {
                    AZ::TickBus::Handler::BusDisconnect();
                    SignalOutput(GetSlotId("Out"));
                }
                else
                {
                    m_currentTime += deltaTime;
                }
            }

        }
    }
}