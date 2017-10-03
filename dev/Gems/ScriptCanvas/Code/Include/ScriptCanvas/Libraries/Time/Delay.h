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

#include <Core/Node.h>
#include <Core/Graph.h>
#include <AzCore/Component/TickBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            class Delay
                : public Node
                , public AZ::TickBus::Handler
            {
            public:

                AZ_COMPONENT(Delay, "{47E4DF81-A668-4ADF-BE0D-E81AC5D69F01}", Node);

                static void Reflect(AZ::ReflectContext* reflection);

                void OnInit() override;

                void OnExecute() override;

                void Visit(NodeVisitor& visitor) const override { visitor.Visit(*this); }

            protected:
                void OnTick(float deltaTime, AZ::ScriptTimePoint scriptTimePoint) override;

                double m_delaySeconds;
                double m_currentTime;
            };
        }
    }
}